// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomRunPickupActor.h"

#define LOCTEXT_NAMESPACE "WacomRunPickupActor"

#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"
#include "UI/Foundation/WacomAppToastSubsystem.h"

AWacomRunPickupActor::AWacomRunPickupActor()
{
	InteractPromptText = GetDefaultInteractPromptText();
	HoverPromptText = GetDefaultHoverPromptText();
	CollectedHoverPromptText = GetDefaultCollectedHoverPromptText();
}

void AWacomRunPickupActor::ConfigureDebugGoldPickupSample()
{
	Modify();
	GoldAmount = 3;
	ApplyDebugPickupAuthoringDefaults(
		BuildDebugPersistentIdFromActorName(GetName(), TEXT("Pickup.Debug."), TEXT("Gold")));
}

FWacomRunPickupDebugView AWacomRunPickupActor::GetRunPickupDebugView(
	AWacomPlayerController* PC) const
{
	const FWacomRunPickupBaseDebugView BaseView = GetRunPickupBaseDebugView(PC);

	FWacomRunPickupDebugView View;
	View.ActorName = BaseView.ActorName;
	View.PersistentId = BaseView.PersistentId;
	View.GoldAmount = GoldAmount;
	View.bHasRunSession = BaseView.bHasRunSession;
	View.bCanInteract = BaseView.bCanInteract;
	View.bIsCollected = BaseView.bIsCollected;
	View.TriggerRadius = BaseView.TriggerRadius;
	View.ClickBoundsExtent = BaseView.ClickBoundsExtent;
	View.VisualName = BaseView.VisualName;
	View.bConfigValid = BaseView.bConfigValid;
	View.ConfigWarningReason = BaseView.ConfigWarningReason;
	View.bDuplicatePersistentIdDetected = BaseView.bDuplicatePersistentIdDetected;
	View.bHasRenderableVisual = BaseView.bHasRenderableVisual;
	View.bClickTargetConfigured = BaseView.bClickTargetConfigured;
	View.ClickTargetStableId = BaseView.ClickTargetStableId;
	View.HoverPrompt = BaseView.HoverPrompt;
	View.CollectedHoverPrompt = BaseView.CollectedHoverPrompt;
	View.LastDebugResult = BaseView.LastDebugResult;
	return View;
}

FString AWacomRunPickupActor::GetRunPickupDebugSummary(AWacomPlayerController* PC) const
{
	const FWacomRunPickupDebugView View = GetRunPickupDebugView(PC);
	const FWacomRunWorldClickableInteractableDebugView ClickDebug =
		GetRunWorldClickableDebugView_Implementation(PC);
	return FString::Printf(
		TEXT("RunPickup{Actor=%s PersistentId=%s Gold=%d HasRun=%s CanInteract=%s Collected=%s TriggerRadius=%.1f BoundsExtent=%s Visual=%s ConfigValid=%s ConfigReason=%s Duplicate=%s HasVisual=%s ClickTarget=%s ClickStableId=%s HoverPrompt=%s CollectedHoverPrompt=%s Last=%s ClickDebug=%s}"),
		*View.ActorName,
		*View.PersistentId.ToString(),
		View.GoldAmount,
		View.bHasRunSession ? TEXT("true") : TEXT("false"),
		View.bCanInteract ? TEXT("true") : TEXT("false"),
		View.bIsCollected ? TEXT("true") : TEXT("false"),
		View.TriggerRadius,
		*View.ClickBoundsExtent.ToCompactString(),
		*View.VisualName.ToString(),
		View.bConfigValid ? TEXT("true") : TEXT("false"),
		*View.ConfigWarningReason.ToString(),
		View.bDuplicatePersistentIdDetected ? TEXT("true") : TEXT("false"),
		View.bHasRenderableVisual ? TEXT("true") : TEXT("false"),
		View.bClickTargetConfigured ? TEXT("true") : TEXT("false"),
		*View.ClickTargetStableId.ToString(),
		*View.HoverPrompt,
		*View.CollectedHoverPrompt,
		*View.LastDebugResult.ToString(),
		*FWacomRunWorldClickableInteractableHelper::BuildDebugSummary(ClickDebug));
}

void AWacomRunPickupActor::LogRunPickupDebugSummary(AWacomPlayerController* PC) const
{
	UE_LOG(LogTemp, Display, TEXT("[RunPickupActor] %s"),
		*GetRunPickupDebugSummary(PC));
}

FName AWacomRunPickupActor::GetRewardConfigWarningReason() const
{
	if (GoldAmount <= 0)
	{
		return TEXT("InvalidGoldAmount");
	}
	return NAME_None;
}

bool AWacomRunPickupActor::TryCollectPickupReward(AWacomPlayerController* PC)
{
	URunSession* Run = PC ? PC->GetRunSession() : nullptr;
	if (!Run || !Run->CollectGoldPickup(PersistentId, GoldAmount))
	{
		return false;
	}

	if (UGameInstance* GameInstance = PC ? PC->GetGameInstance() : nullptr)
	{
		if (UWacomAppToastSubsystem* ToastSubsystem =
			GameInstance->GetSubsystem<UWacomAppToastSubsystem>())
		{
			ToastSubsystem->ShowGoldChanged(GoldAmount);
		}
	}
	return true;
}

FText AWacomRunPickupActor::GetDefaultInteractPromptText() const
{
	return LOCTEXT("DefaultInteractPrompt", "按 E 拾取");
}

FText AWacomRunPickupActor::GetDefaultHoverPromptText() const
{
	return LOCTEXT("DefaultHoverPrompt", "点击拾取");
}

FText AWacomRunPickupActor::GetDefaultCollectedHoverPromptText() const
{
	return LOCTEXT("DefaultCollectedHoverPrompt", "已拾取");
}

#undef LOCTEXT_NAMESPACE
