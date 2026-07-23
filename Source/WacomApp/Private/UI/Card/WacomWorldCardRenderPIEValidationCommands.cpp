// Copyright Wacom. All Rights Reserved.

#if WITH_EDITOR

#include "UI/Card/WacomWorldCardRenderExperimentActor.h"
#include "UI/Card/WacomWorldCardRenderExperimentPolicy.h"

#include "Actors/WacomShopTriggerActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Cards/CardDefinition.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UI/Card/WacomCardPresentationBuilder.h"
#include "UI/Card/WacomCardView.h"
#include "UI/Card/WacomWorldCardSurfaceMaterialAdapter.h"

namespace
{
	constexpr uint64 ScreenMessageKey = 0x574352450001ull;
	constexpr uint64 ModeMessageKey = 0x574352450002ull;
	const FVector2D ScreenReferencePosition(36.0f, 120.0f);
	const FVector2D ScreenReferenceSize(360.0f, 488.0f);

	struct FWorldCardRenderExperimentState
	{
		TWeakObjectPtr<AWacomWorldCardRenderExperimentActor> Actor;
		TWeakObjectPtr<UWacomCardView> ScreenReference;
		TWeakObjectPtr<UWorld> World;
		FString CardSource;
	};

	FWorldCardRenderExperimentState ExperimentState;

	APlayerController* ResolveLocalPlayer(UWorld* World)
	{
		APlayerController* PlayerController =
			FWacomWorldCardRenderExperimentPolicy::IsSupportedPIEWorld(World)
				? World->GetFirstPlayerController()
				: nullptr;
		return PlayerController && PlayerController->IsLocalController()
			? PlayerController
			: nullptr;
	}

	FString NormalizeObjectPath(const FString& RequestedPath)
	{
		FString ObjectPath = RequestedPath;
		ObjectPath.TrimStartAndEndInline();
		if (ObjectPath.StartsWith(TEXT("/Game/"))
			&& !ObjectPath.Contains(TEXT(".")))
		{
			ObjectPath += TEXT(".");
			ObjectPath += FPackageName::GetShortName(ObjectPath.LeftChop(1));
		}
		return ObjectPath;
	}

	UCardDefinition* ResolveCardDefinition(
		UWorld& World,
		const TArray<FString>& Args,
		FString& OutSource)
	{
		OutSource.Reset();
		if (!Args.IsEmpty())
		{
			const FString ObjectPath = NormalizeObjectPath(Args[0]);
			UCardDefinition* ExplicitCard =
				LoadObject<UCardDefinition>(nullptr, *ObjectPath);
			if (!ExplicitCard)
			{
				OutSource = FString::Printf(
					TEXT("ExplicitLoadFailed:%s"),
					*ObjectPath);
				return nullptr;
			}
			OutSource = FString::Printf(TEXT("Explicit:%s"), *ObjectPath);
			return ExplicitCard;
		}

		for (TActorIterator<AWacomShopTriggerActor> It(&World); It; ++It)
		{
			if (!IsValid(*It))
			{
				continue;
			}

			for (const FRunShopOfferInput& Offer : It->BuildResolvedOffers())
			{
				if (IsValid(Offer.CardDefinition))
				{
					OutSource = FString::Printf(
						TEXT("ShopTrigger:%s Card:%s"),
						*It->GetName(),
						*Offer.CardDefinition->GetPathName());
					return Offer.CardDefinition;
				}
			}
		}

		OutSource = TEXT("NoExplicitCardOrValidShopOffer");
		return nullptr;
	}

	void RemoveScreenMessages()
	{
		if (GEngine)
		{
			GEngine->RemoveOnScreenDebugMessage(ScreenMessageKey);
			GEngine->RemoveOnScreenDebugMessage(ModeMessageKey);
		}
	}

	void ClearWorldCardRenderPIEValidation(UWorld* World)
	{
		if (UWacomCardView* ScreenReference =
			ExperimentState.ScreenReference.Get())
		{
			ScreenReference->RemoveFromParent();
		}
		if (AWacomWorldCardRenderExperimentActor* Actor =
			ExperimentState.Actor.Get())
		{
			Actor->Destroy();
		}

		ExperimentState = FWorldCardRenderExperimentState();
		RemoveScreenMessages();
		UE_LOG(
			LogTemp,
			Display,
			TEXT("[WorldCardRenderPIE] transient 渲染实验台已清理 World=%s"),
			World ? *World->GetName() : TEXT("None"));
	}

	void DumpWorldCardRenderPIEValidation(UWorld* World)
	{
		const AWacomWorldCardRenderExperimentActor* Actor =
			ExperimentState.Actor.Get();
		const UWacomCardView* ScreenReference =
			ExperimentState.ScreenReference.Get();
		UE_LOG(
			LogTemp,
			Display,
			TEXT("[WorldCardRenderPIE] Active=%s PIE=%s ScreenReference=%s CardSource=%s %s"),
			Actor && ScreenReference ? TEXT("true") : TEXT("false"),
			FWacomWorldCardRenderExperimentPolicy::IsSupportedPIEWorld(World)
				? TEXT("true")
				: TEXT("false"),
			ScreenReference ? *ScreenReference->GetClass()->GetPathName() : TEXT("None"),
			*ExperimentState.CardSource,
			Actor ? *Actor->BuildDebugSummary() : TEXT("Actor=None"));

		for (const FWacomWorldCardRenderExperimentModeConfig& Mode :
			FWacomWorldCardRenderExperimentPolicy::GetModes())
		{
			UE_LOG(
				LogTemp,
				Display,
				TEXT("[WorldCardRenderPIE] Mode=%s Blend=%s CustomMaterial=%s DefaultExposure=%.3f"),
				Mode.Label,
				Mode.BlendMode == EWidgetBlendMode::Masked
					? TEXT("Masked")
					: TEXT("Transparent"),
				Mode.bUseWacomMaterial ? TEXT("true") : TEXT("false"),
				Mode.ExposureCompensationStrength);
		}
	}

	void OpenWorldCardRenderPIEValidation(
		const TArray<FString>& Args,
		UWorld* World)
	{
		if (ExperimentState.Actor.IsValid()
			|| ExperimentState.ScreenReference.IsValid())
		{
			ClearWorldCardRenderPIEValidation(World);
		}

		if (Args.Num() > 1)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[WorldCardRenderPIE] 打开被拒绝：只接受一个可选 CardDefinition object path"));
			return;
		}

		APlayerController* PlayerController = ResolveLocalPlayer(World);
		if (!PlayerController || !PlayerController->PlayerCameraManager)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[WorldCardRenderPIE] 打开被拒绝：PIE=%s LocalPlayer=%s Camera=%s"),
				FWacomWorldCardRenderExperimentPolicy::IsSupportedPIEWorld(World)
					? TEXT("true")
					: TEXT("false"),
				PlayerController ? TEXT("OK") : TEXT("Missing"),
				PlayerController && PlayerController->PlayerCameraManager
					? TEXT("OK")
					: TEXT("Missing"));
			return;
		}

		FString CardSource;
		UCardDefinition* CardDefinition =
			ResolveCardDefinition(*World, Args, CardSource);
		if (!CardDefinition)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[WorldCardRenderPIE] 打开被拒绝：CardSource=%s"),
				*CardSource);
			return;
		}

		UClass* CardViewClass = LoadClass<UWacomCardView>(
			nullptr,
			FWacomWorldCardRenderExperimentPolicy::GetCardViewClassPath());
		UMaterialInterface* WorldCardMaterial =
			FWacomWorldCardSurfaceMaterialAdapter::ResolveMaterial();
		if (!CardViewClass
			|| !CardViewClass->IsChildOf(UWacomCardView::StaticClass())
			|| !WorldCardMaterial)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[WorldCardRenderPIE] 打开被拒绝：CardView=%s Material=%s"),
				CardViewClass ? *CardViewClass->GetPathName() : TEXT("Missing"),
				WorldCardMaterial ? *WorldCardMaterial->GetPathName() : TEXT("Missing"));
			return;
		}

		const FWacomCardViewData CardViewData =
			UWacomCardPresentationBuilder::BuildCardViewData(CardDefinition);
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Name = MakeUniqueObjectName(
			World,
			AWacomWorldCardRenderExperimentActor::StaticClass(),
			TEXT("WorldCardRenderPIE"));
		SpawnParameters.ObjectFlags |= RF_Transient;
		SpawnParameters.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AWacomWorldCardRenderExperimentActor* Actor =
			World->SpawnActor<AWacomWorldCardRenderExperimentActor>(
				AWacomWorldCardRenderExperimentActor::StaticClass(),
				FTransform::Identity,
				SpawnParameters);
		if (!Actor
			|| !Actor->InitializeExperiment(
				*PlayerController,
				CardViewClass,
				CardViewData))
		{
			if (Actor)
			{
				Actor->Destroy();
			}
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[WorldCardRenderPIE] 打开失败：世界卡组件初始化失败"));
			return;
		}

		UWacomCardView* ScreenReference =
			CreateWidget<UWacomCardView>(PlayerController, CardViewClass);
		if (!ScreenReference)
		{
			Actor->Destroy();
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[WorldCardRenderPIE] 打开失败：屏幕参考卡创建失败"));
			return;
		}
		ScreenReference->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		ScreenReference->SetCardViewData(CardViewData);
		ScreenReference->AddToViewport(10000);
		ScreenReference->SetDesiredSizeInViewport(ScreenReferenceSize);
		ScreenReference->SetPositionInViewport(
			ScreenReferencePosition,
			false);
		ScreenReference->SetAlignmentInViewport(FVector2D::ZeroVector);

		ExperimentState.Actor = Actor;
		ExperimentState.ScreenReference = ScreenReference;
		ExperimentState.World = World;
		ExperimentState.CardSource = CardSource;

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				ScreenMessageKey,
				-1.0f,
				FColor::Cyan,
				TEXT("SCREEN REFERENCE (left) | Wacom.WorldCardRender.ClearPIEValidation"));
			GEngine->AddOnScreenDebugMessage(
				ModeMessageKey,
				-1.0f,
				FColor::White,
				TEXT("World modes: Transparent | Masked | Wacom Raw | Wacom Exposure"));
		}

		UE_LOG(
			LogTemp,
			Display,
			TEXT("[WorldCardRenderPIE] 已打开：CardSource=%s CardView=%s Material=%s %s"),
			*CardSource,
			*CardViewClass->GetPathName(),
			*WorldCardMaterial->GetPathName(),
			*Actor->BuildDebugSummary());
	}

	void SetWorldCardRenderExposureStrength(
		const TArray<FString>& Args,
		UWorld* World)
	{
		float Strength = 0.0f;
		if (Args.Num() != 1
			|| !LexTryParseString(Strength, *Args[0])
			|| Strength < 0.0f
			|| Strength > 1.0f)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[WorldCardRenderPIE] ExposureStrength 必须是 0..1 的单个数值"));
			return;
		}

		AWacomWorldCardRenderExperimentActor* Actor =
			ExperimentState.Actor.Get();
		if (!Actor
			|| !FWacomWorldCardRenderExperimentPolicy::IsSupportedPIEWorld(World)
			|| ExperimentState.World.Get() != World
			|| !Actor->SetExposureCompensationStrength(Strength))
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[WorldCardRenderPIE] ExposureStrength 更新被拒绝：实验台未在当前 PIE World 激活"));
			return;
		}

		UE_LOG(
			LogTemp,
			Display,
			TEXT("[WorldCardRenderPIE] Wacom Masked Exposure 强度已更新为 %.3f"),
			Strength);
	}

	FAutoConsoleCommandWithWorldAndArgs GOpenWorldCardRenderPIEValidationCommand(
		TEXT("Wacom.WorldCardRender.OpenPIEValidation"),
		TEXT("仅编辑器 PIE：显示一张屏幕参考卡和四张相机相对的世界材质对照卡。可选参数为 CardDefinition object path。"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
			&OpenWorldCardRenderPIEValidation));

	FAutoConsoleCommandWithWorldAndArgs GSetWorldCardRenderExposureStrengthCommand(
		TEXT("Wacom.WorldCardRender.SetExposureStrength"),
		TEXT("仅编辑器 PIE：设置 Wacom Masked Exposure 模式的反曝光补偿强度，范围 0..1。"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
			&SetWorldCardRenderExposureStrength));

	FAutoConsoleCommandWithWorld GDumpWorldCardRenderPIEValidationCommand(
		TEXT("Wacom.WorldCardRender.DumpPIEValidation"),
		TEXT("仅编辑器 PIE：打印 transient 世界卡渲染实验台的当前状态与四种模式。"),
		FConsoleCommandWithWorldDelegate::CreateStatic(
			&DumpWorldCardRenderPIEValidation));

	FAutoConsoleCommandWithWorld GClearWorldCardRenderPIEValidationCommand(
		TEXT("Wacom.WorldCardRender.ClearPIEValidation"),
		TEXT("仅编辑器 PIE：移除屏幕参考卡并销毁本命令创建的 transient 世界卡 Actor。"),
		FConsoleCommandWithWorldDelegate::CreateStatic(
			&ClearWorldCardRenderPIEValidation));
}

#endif
