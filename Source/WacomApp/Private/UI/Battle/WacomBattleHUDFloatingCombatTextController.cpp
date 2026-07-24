// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHUDFloatingCombatTextController.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/WacomBattleEnemyPartComponent.h"
#include "Components/WacomBattleEnemySceneRuntimeComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/PlayerStatusBar.h"
#include "UI/Battle/WacomBattleFloatingCombatTextLayerWidget.h"
#include "UI/Battle/WacomBattleFloatingCombatTextStyle.h"
#include "UI/Battle/WacomBattleFloatingCombatTextStyleProvider.h"
#include "UI/Battle/WacomBattleFloatingCombatTextTypes.h"
#include "UI/Battle/WacomBattleHUDRuntime.h"
#include "UI/Battle/WacomBattleHUDSceneEnemyTargetCoordinator.h"

FWacomBattleHUDFloatingCombatTextController::
FWacomBattleHUDFloatingCombatTextController(FWacomBattleHUDRuntime& InRuntime)
	: Runtime(InRuntime)
{
}

void FWacomBattleHUDFloatingCombatTextController::StageResolvedCommand(
	const uint64 PresentationTransactionId,
	const TArray<FBattleEvent>& Events)
{
	Synchronizer.Stage(PresentationTransactionId, Events);
}

void FWacomBattleHUDFloatingCombatTextController::ApplyPresentationProgress(
	const FWacomBattlePresentationProgress& Progress)
{
	bool bFlushedRemainder = false;
	const TArray<FWacomBattleFloatingCombatTextEmission> Emissions =
		Synchronizer.ApplyProgress(Progress, bFlushedRemainder);
	if (bFlushedRemainder)
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("[BattleFloatingCombatText] Transaction %llu completed with unmatched rows; flushed safely."),
			Progress.PresentationTransactionId);
	}
	PresentEmissions(Emissions);
}

void FWacomBattleHUDFloatingCombatTextController::FlushTransaction(
	const uint64 PresentationTransactionId)
{
	if (PresentationTransactionId == 0)
	{
		return;
	}
	FWacomBattlePresentationProgress Progress;
	Progress.Kind = EWacomBattlePresentationProgressKind::PlanCompleted;
	Progress.PresentationTransactionId = PresentationTransactionId;
	ApplyPresentationProgress(Progress);
}

void FWacomBattleHUDFloatingCombatTextController::DiscardTransaction(
	const uint64 PresentationTransactionId)
{
	Synchronizer.Discard(PresentationTransactionId);
}

void FWacomBattleHUDFloatingCombatTextController::Tick(const float DeltaTime)
{
	if (UWacomBattleFloatingCombatTextLayerWidget* Layer =
		Runtime.Host().GetFloatingCombatTextLayer())
	{
		if (Layer->IsPlaybackActive())
		{
			Layer->TickPlayback(DeltaTime);
		}
	}
}

void FWacomBattleHUDFloatingCombatTextController::Clear()
{
	Synchronizer.Clear();
	if (UWacomBattleFloatingCombatTextLayerWidget* Layer =
		Runtime.Host().GetFloatingCombatTextLayer())
	{
		Layer->ClearPlayback();
	}
}

void FWacomBattleHUDFloatingCombatTextController::PresentEmissions(
	const TArray<FWacomBattleFloatingCombatTextEmission>& Emissions)
{
	UWacomBattleFloatingCombatTextLayerWidget* Layer =
		Runtime.Host().GetFloatingCombatTextLayer();
	if (!Layer)
	{
		return;
	}

	TArray<FWacomBattleFloatingCombatTextSpawnRequest> Requests;
	for (const FWacomBattleFloatingCombatTextEmission& Emission : Emissions)
	{
		for (const FWacomBattleFloatingCombatTextRow& Row : Emission.Rows)
		{
			FWacomBattleFloatingCombatTextSpawnRequest Request;
			if (BuildSpawnRequest(Row, Request))
			{
				PlayOptionalWorldAccent(Request);
				Requests.Add(MoveTemp(Request));
			}
		}
	}
	if (!Requests.IsEmpty())
	{
		Layer->Enqueue(Requests);
	}
}

bool FWacomBattleHUDFloatingCombatTextController::BuildSpawnRequest(
	const FWacomBattleFloatingCombatTextRow& Row,
	FWacomBattleFloatingCombatTextSpawnRequest& OutRequest) const
{
	if (Row.Amount == 0)
	{
		return false;
	}
	OutRequest.Row = Row;

	if (Row.Target.Kind == EWacomBattleFloatingCombatTextTargetKind::EnemyPart)
	{
		FVector WorldLocation = FVector::ZeroVector;
		if (ResolveEnemyAnchor(
			Row.Target.EnemyPartKey,
			OutRequest.CapturedScreenPosition,
			WorldLocation))
		{
			OutRequest.WorldAccentLocation = WorldLocation;
			return true;
		}
		OutRequest.CapturedScreenPosition = ResolveViewportFallback(false);
		return true;
	}

	if (!ResolvePlayerAnchor(OutRequest.CapturedScreenPosition))
	{
		OutRequest.CapturedScreenPosition = ResolveViewportFallback(true);
	}
	return true;
}

bool FWacomBattleHUDFloatingCombatTextController::ResolvePlayerAnchor(
	FVector2D& OutWidgetPosition) const
{
	const UPlayerStatusBar* StatusBar = Runtime.Host().GetPlayerStatusBar();
	const UBattleHUD* HUD = Cast<UBattleHUD>(Runtime.Host().AsObject());
	if (!StatusBar || !HUD || StatusBar->GetVisibility() == ESlateVisibility::Collapsed)
	{
		return false;
	}
	const FGeometry Geometry = StatusBar->GetCachedGeometry();
	const FVector2D LocalSize = Geometry.GetLocalSize();
	if (LocalSize.X <= 0.0f || LocalSize.Y <= 0.0f)
	{
		return false;
	}
	const FVector2D AbsoluteCenter = Geometry.LocalToAbsolute(LocalSize * 0.5f);
	FVector2D PixelPosition = FVector2D::ZeroVector;
	USlateBlueprintLibrary::AbsoluteToViewport(
		HUD,
		AbsoluteCenter,
		PixelPosition,
		OutWidgetPosition);
	OutWidgetPosition +=
		WacomBattleFloatingCombatTextStyleProvider::GetStyle().PlayerAnchorOffset;
	return FMath::IsFinite(OutWidgetPosition.X) && FMath::IsFinite(OutWidgetPosition.Y);
}

bool FWacomBattleHUDFloatingCombatTextController::ResolveEnemyAnchor(
	const FBattleEnemyPartKey& PartKey,
	FVector2D& OutWidgetPosition,
	FVector& OutWorldLocation) const
{
	UWacomBattleEnemyPartComponent* Part =
		Runtime.GetSceneEnemyTargetCoordinator().ResolvePartComponent(PartKey);
	AWacomBattleEnemyActor* Host = Part ? Part->GetOwningEnemyHost() : nullptr;
	UWacomBattleEnemySceneRuntimeComponent* SceneRuntime =
		Host ? Host->GetEnemySceneRuntimeComponent() : nullptr;
	APlayerController* PlayerController = Runtime.GetOwningPlayer();
	if (!Part || !SceneRuntime || !PlayerController
		|| !SceneRuntime->TryResolvePartPresentationAnchorWorldLocation(
			*Part,
			OutWorldLocation))
	{
		return false;
	}

	if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
		PlayerController,
		OutWorldLocation,
		OutWidgetPosition,
		false))
	{
		return false;
	}
	OutWidgetPosition +=
		WacomBattleFloatingCombatTextStyleProvider::GetStyle().EnemyAnchorOffset;
	return FMath::IsFinite(OutWidgetPosition.X) && FMath::IsFinite(OutWidgetPosition.Y);
}

FVector2D FWacomBattleHUDFloatingCombatTextController::ResolveViewportFallback(
	const bool bPlayer) const
{
	FVector2D ViewportSize =
		FVector2D(UWidgetLayoutLibrary::GetViewportSize(Runtime.Host().AsObject()));
	const float Scale = UWidgetLayoutLibrary::GetViewportScale(Runtime.Host().AsObject());
	if (Scale > UE_SMALL_NUMBER)
	{
		ViewportSize /= Scale;
	}
	if (ViewportSize.X <= 0.0f || ViewportSize.Y <= 0.0f)
	{
		ViewportSize = FVector2D(1920.0f, 1080.0f);
	}
	return bPlayer
		? FVector2D(190.0f, 116.0f)
		: FVector2D(ViewportSize.X * 0.5f, ViewportSize.Y * 0.36f);
}

void FWacomBattleHUDFloatingCombatTextController::PlayOptionalWorldAccent(
	const FWacomBattleFloatingCombatTextSpawnRequest& Request) const
{
	if (!Request.WorldAccentLocation.IsSet())
	{
		return;
	}

	const UWacomBattleFloatingCombatTextStyle& Style =
		WacomBattleFloatingCombatTextStyleProvider::GetStyle();
	UNiagaraSystem* System = nullptr;
	switch (Request.Row.Kind)
	{
	case EWacomBattleFloatingCombatTextKind::ShieldAbsorbed:
	case EWacomBattleFloatingCombatTextKind::ShieldChanged:
		System = Style.ShieldNiagara;
		break;
	case EWacomBattleFloatingCombatTextKind::PeriodicDamage:
		System = Style.PeriodicNiagara;
		break;
	case EWacomBattleFloatingCombatTextKind::CriticalDamage:
		System = Style.CriticalNiagara;
		break;
	default:
		return;
	}

	UWorld* World = Runtime.GetWorld();
	if (System && World)
	{
		if (UNiagaraComponent* Component =
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
			System,
			Request.WorldAccentLocation.GetValue(),
			FRotator::ZeroRotator,
			FVector::OneVector,
			true,
			true,
			ENCPoolMethod::AutoRelease,
			true))
		{
			Component->SetVariableBool(
				TEXT("User.ShieldBroken"),
				Request.Row.bShieldBroken);
		}
	}
}
