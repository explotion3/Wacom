// Copyright Wacom. All Rights Reserved.

#if WITH_EDITOR

#include "Actors/WacomShopTriggerActor.h"
#include "Actors/WacomWorldShopHostActor.h"
#include "Camera/WacomFirstPersonViewStageRequest.h"
#include "Camera/CameraComponent.h"
#include "Cards/CardDefinition.h"
#include "Components/WidgetComponent.h"
#include "Components/WidgetInteractionComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "GameFramework/WacomPlayerController.h"
#include "HAL/IConsoleManager.h"
#include "RunSession.h"

namespace
{
	struct FWorldShopPIEValidationState
	{
		TWeakObjectPtr<AWacomWorldShopHostActor> Host;
		TWeakObjectPtr<UWorld> World;
		TWeakObjectPtr<UCardDefinition> FirstCardDefinition;
		FName ShopId = NAME_None;
		int32 BackpackCountBaseline = 0;
		int32 FirstCardOwnedBaseline = 0;
		uint64 StorageRevisionBaseline = 0;
	};

	FWorldShopPIEValidationState ValidationState;

	AWacomPlayerController* ResolvePlayer(UWorld* World)
	{
		return World && World->WorldType == EWorldType::PIE
			? Cast<AWacomPlayerController>(World->GetFirstPlayerController())
			: nullptr;
	}

	AWacomShopTriggerActor* FindSourceTrigger(UWorld& World)
	{
		for (TActorIterator<AWacomShopTriggerActor> It(&World); It; ++It)
		{
			if (IsValid(*It) && !It->BuildResolvedOffers().IsEmpty())
			{
				return *It;
			}
		}
		return nullptr;
	}

	int32 CountBackpackDefinition(
		const URunSession& Run,
		const UCardDefinition* Definition)
	{
		int32 Count = 0;
		for (const FCardInstance& Instance : Run.GetBackpack())
		{
			Count += Instance.Definition == Definition ? 1 : 0;
		}
		return Count;
	}

	UWidgetInteractionComponent* FindWorldShopInteraction(
		const AWacomPlayerCharacter* Pawn)
	{
		if (!Pawn)
		{
			return nullptr;
		}
		TInlineComponentArray<UWidgetInteractionComponent*> Interactions;
		Pawn->GetComponents(Interactions);
		for (UWidgetInteractionComponent* Interaction : Interactions)
		{
			if (Interaction
				&& Interaction->GetFName() == TEXT("WorldShopWidgetInteraction"))
			{
				return Interaction;
			}
		}
		return nullptr;
	}

	void ClearWorldShopPIEValidation(UWorld* World)
	{
		if (AWacomPlayerController* PC = ResolvePlayer(World))
		{
			PC->CloseWorldShop();
		}
		if (AWacomWorldShopHostActor* Host = ValidationState.Host.Get())
		{
			Host->Destroy();
		}
		ValidationState = FWorldShopPIEValidationState();
		UE_LOG(LogTemp, Display, TEXT("[WorldShopPIE] transient 验证内容已清理"));
	}

	void DumpWorldShopPIEValidation(UWorld* World)
	{
		AWacomPlayerController* PC = ResolvePlayer(World);
		URunSession* Run = PC ? PC->GetRunSession() : nullptr;
		AWacomPlayerCharacter* Pawn = PC ? PC->GetPawn<AWacomPlayerCharacter>() : nullptr;
		AWacomWorldShopHostActor* Host = ValidationState.Host.Get();
		UCardDefinition* FirstCard = ValidationState.FirstCardDefinition.Get();
		const FRunShopSnapshot Snapshot = Run
			? Run->BuildCurrentShopSnapshot()
			: FRunShopSnapshot();
		const FRunShopOffer* FirstOffer =
			Snapshot.Offers.IsEmpty() ? nullptr : &Snapshot.Offers[0];
		const int32 BackpackCount = Run ? Run->GetBackpack().Num() : 0;
		const int32 FirstCardOwned = Run
			? CountBackpackDefinition(*Run, FirstCard)
			: 0;
		const uint64 StorageRevision = Run
			? Run->GetBackpackStorageSnapshotRevision()
			: 0;

		UWidgetInteractionComponent* Interaction =
			FindWorldShopInteraction(Pawn);
		const FHitResult Hit = Interaction
			? Interaction->GetLastHitResult()
			: FHitResult();
		const FString HoveredComponentName = Interaction
			? GetNameSafe(Interaction->GetHoveredWidgetComponent())
			: TEXT("None");
		UE_LOG(LogTemp, Display,
			TEXT("[WorldShopPIE] RouteActive=%s Host=%s HostLocation=%s ShopId=%s ShopActive=%s Offers=%d"),
			(PC && PC->IsWorldShopActive()) ? TEXT("true") : TEXT("false"),
			*GetNameSafe(Host),
			Host ? *Host->GetActorLocation().ToCompactString() : TEXT("None"),
			*ValidationState.ShopId.ToString(),
			Snapshot.bIsActive ? TEXT("true") : TEXT("false"),
			Snapshot.Offers.Num());
		UE_LOG(LogTemp, Display,
			TEXT("[WorldShopPIE] WIC=%s HoveredComponent=%s HitTestVisible=%s HitActor=%s HitComponent=%s HitDistance=%.1f"),
			*GetNameSafe(Interaction),
			*HoveredComponentName,
			(Interaction && Interaction->IsOverHitTestVisibleWidget())
				? TEXT("true")
				: TEXT("false"),
			*GetNameSafe(Hit.GetActor()),
			*GetNameSafe(Hit.GetComponent()),
			Hit.Distance);
		UE_LOG(LogTemp, Display,
			TEXT("[WorldShopPIE] FirstOfferId=%s FirstCard=%s Purchased=%s Price=%d Gold=%d Backpack=%d(%+d) StorageRevision=%llu(%+lld) FirstCardOwned=%d(%+d)"),
			FirstOffer ? *FirstOffer->OfferId.ToString() : TEXT("None"),
			*GetNameSafe(FirstCard),
			(FirstOffer && FirstOffer->bPurchased) ? TEXT("true") : TEXT("false"),
			FirstOffer ? FirstOffer->Price : -1,
			Run ? Run->GetGold() : 0,
			BackpackCount,
			BackpackCount - ValidationState.BackpackCountBaseline,
			StorageRevision,
			static_cast<int64>(StorageRevision)
				- static_cast<int64>(ValidationState.StorageRevisionBaseline),
			FirstCardOwned,
			FirstCardOwned - ValidationState.FirstCardOwnedBaseline);
	}

	void OpenWorldShopPIEValidation(UWorld* World)
	{
		if (ValidationState.Host.IsValid())
		{
			ClearWorldShopPIEValidation(World);
		}
		AWacomPlayerController* PC = ResolvePlayer(World);
		URunSession* Run = PC ? PC->GetRunSession() : nullptr;
		AWacomPlayerCharacter* Pawn = PC ? PC->GetPawn<AWacomPlayerCharacter>() : nullptr;
		AWacomShopTriggerActor* SourceTrigger = World ? FindSourceTrigger(*World) : nullptr;
		if (!PC || !Run || !Pawn || !Run->IsRunActive() || !SourceTrigger
			|| Run->IsShopVisitActive())
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[WorldShopPIE] 打开被拒绝：PIE=%s PC=%s Run=%s RunActive=%s Pawn=%s Trigger=%s ShopActive=%s"),
				(World && World->WorldType == EWorldType::PIE) ? TEXT("true") : TEXT("false"),
				PC ? TEXT("OK") : TEXT("Missing"),
				Run ? TEXT("OK") : TEXT("Missing"),
				(Run && Run->IsRunActive()) ? TEXT("true") : TEXT("false"),
				Pawn ? TEXT("OK") : TEXT("Missing"),
				SourceTrigger ? *SourceTrigger->GetName() : TEXT("Missing"),
				(Run && Run->IsShopVisitActive()) ? TEXT("true") : TEXT("false"));
			return;
		}

		FRunShopVisitRequest Request = SourceTrigger->BuildResolvedVisitRequest();
		Request.ShopId = FName(*FString::Printf(
			TEXT("WorldShop.PIE.%s"),
			*FGuid::NewGuid().ToString(EGuidFormats::Digits)));
		Request.CardUpgradeService = FRunShopCardUpgradeServiceInput();
		if (Request.Offers.Num() > 8)
		{
			Request.Offers.SetNum(8);
		}
		if (!Request.Offers.IsEmpty())
		{
			Request.Offers[0].Price = 0;
		}
		UCardDefinition* FirstCardDefinition = Request.Offers.IsEmpty()
			? nullptr
			: Request.Offers[0].CardDefinition.Get();
		const int32 BackpackCountBaseline = Run->GetBackpack().Num();
		const int32 FirstCardOwnedBaseline =
			CountBackpackDefinition(*Run, FirstCardDefinition);
		const uint64 StorageRevisionBaseline =
			Run->GetBackpackStorageSnapshotRevision();

		FActorSpawnParameters SpawnParams;
		SpawnParams.Name = MakeUniqueObjectName(
			World,
			AWacomWorldShopHostActor::StaticClass(),
			TEXT("WorldShopPIEHost"));
		SpawnParams.ObjectFlags |= RF_Transient;
		SpawnParams.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FVector ViewLocation = Pawn->GetFirstPersonCamera()
			? Pawn->GetFirstPersonCamera()->GetComponentLocation()
			: Pawn->GetActorLocation();
		const FRotator ViewRotation = PC->GetControlRotation();
		const FVector HostLocation = ViewLocation
			+ ViewRotation.Vector() * 320.0f
			+ FVector(0.0f, 0.0f, 20.0f);
		const FRotator HostRotation(0.0f, ViewRotation.Yaw + 180.0f, 0.0f);
		AWacomWorldShopHostActor* Host = World->SpawnActor<AWacomWorldShopHostActor>(
			AWacomWorldShopHostActor::StaticClass(),
			HostLocation,
			HostRotation,
			SpawnParams);
		if (!Host)
		{
			UE_LOG(LogTemp, Warning, TEXT("[WorldShopPIE] transient Host 创建失败"));
			return;
		}
		FWacomFirstPersonViewStageRequest CurrentView;
		if (!PC->RequestOpenShop(Request, CurrentView, Host))
		{
			Host->Destroy();
			UE_LOG(LogTemp, Warning,
				TEXT("[WorldShopPIE] World route 打开失败 ShopId=%s Offers=%d"),
				*Request.ShopId.ToString(),
				Request.Offers.Num());
			return;
		}
		ValidationState.Host = Host;
		ValidationState.World = World;
		ValidationState.FirstCardDefinition = FirstCardDefinition;
		ValidationState.ShopId = Request.ShopId;
		ValidationState.BackpackCountBaseline = BackpackCountBaseline;
		ValidationState.FirstCardOwnedBaseline = FirstCardOwnedBaseline;
		ValidationState.StorageRevisionBaseline = StorageRevisionBaseline;
		UE_LOG(LogTemp, Display,
			TEXT("[WorldShopPIE] 已打开 transient World Shop：ShopId=%s SourceTrigger=%s Offers=%d FirstOfferFree=true Host=%s Distance=320cm"),
			*Request.ShopId.ToString(),
			*SourceTrigger->GetName(),
			Request.Offers.Num(),
			*Host->GetName());
	}

	FAutoConsoleCommandWithWorld GOpenWorldShopPIEValidationCommand(
		TEXT("Wacom.WorldShop.OpenPIEValidation"),
		TEXT("仅编辑器 PIE：复制第一个有效 Shop Trigger 的前 8 个商品，创建 RF_Transient 2x4 World Shop；首件免费，不保存地图或资产。"),
		FConsoleCommandWithWorldDelegate::CreateStatic(&OpenWorldShopPIEValidation));

	FAutoConsoleCommandWithWorld GDumpWorldShopPIEValidationCommand(
		TEXT("Wacom.WorldShop.DumpPIEValidation"),
		TEXT("仅编辑器 PIE：输出 World route/Host、鼠标 WIC 命中、首件商品与 Backpack/StorageRevision 增量。"),
		FConsoleCommandWithWorldDelegate::CreateStatic(&DumpWorldShopPIEValidation));

	FAutoConsoleCommandWithWorld GClearWorldShopPIEValidationCommand(
		TEXT("Wacom.WorldShop.ClearPIEValidation"),
		TEXT("仅编辑器 PIE：关闭并销毁本命令创建的 transient World Shop，不保存地图或资产。"),
		FConsoleCommandWithWorldDelegate::CreateStatic(&ClearWorldShopPIEValidation));
}

#endif
