// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomRunRewardPickupActor.h"

#define LOCTEXT_NAMESPACE "WacomRunRewardPickupActor"

#include "Cards/CardDefinition.h"
#include "GameFramework/WacomPlayerController.h"
#include "Pickups/RunPickupDefinition.h"
#include "RunSession.h"
#include "UI/Foundation/WacomAppToastSubsystem.h"
#include "UI/Run/WacomRunTreasurePresentationSync.h"

namespace
{
	const TCHAR* DebugGoldDefinitionPath =
		TEXT("/Game/Wacom/Data/Pickups/DA_Pickup_DebugGold3.DA_Pickup_DebugGold3");
	const TCHAR* DebugPoisonFangDefinitionPath =
		TEXT("/Game/Wacom/Data/Pickups/DA_Pickup_DebugPoisonFang.DA_Pickup_DebugPoisonFang");

	FName RewardTypeToDebugName(EWacomRunPickupRewardType RewardType)
	{
		switch (RewardType)
		{
		case EWacomRunPickupRewardType::Gold:
			return TEXT("Gold");
		case EWacomRunPickupRewardType::Card:
			return TEXT("Card");
		case EWacomRunPickupRewardType::None:
		default:
			return TEXT("None");
		}
	}

	UCardDefinition* ResolveCardDefinition(const UWacomRunPickupDefinition* Definition)
	{
		return Definition ? Definition->CardDefinition.Get() : nullptr;
	}
}

AWacomRunRewardPickupActor::AWacomRunRewardPickupActor()
{
}

FWacomRunRewardPickupDebugView AWacomRunRewardPickupActor::GetRunRewardPickupDebugView(
	AWacomPlayerController* PC) const
{
	const FWacomRunPickupBaseDebugView BaseView = GetRunPickupBaseDebugView(PC);
	UCardDefinition* CardDefinition = ResolveCardDefinition(PickupDefinition);

	FWacomRunRewardPickupDebugView View;
	View.ActorName = BaseView.ActorName;
	View.PersistentId = BaseView.PersistentId;
	View.PickupDefinitionName = PickupDefinition ? PickupDefinition->GetName() : TEXT("None");
	View.PickupId = PickupDefinition ? PickupDefinition->PickupId : NAME_None;
	View.RewardType = PickupDefinition
		? RewardTypeToDebugName(PickupDefinition->RewardType)
		: FName(TEXT("None"));
	View.GoldAmount = PickupDefinition ? PickupDefinition->GoldAmount : 0;
	View.CardDefinitionName = CardDefinition ? CardDefinition->GetName() : TEXT("None");
	View.CardId = CardDefinition ? CardDefinition->CardId : NAME_None;
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

FString AWacomRunRewardPickupActor::GetRunRewardPickupDebugSummary(
	AWacomPlayerController* PC) const
{
	const FWacomRunRewardPickupDebugView View = GetRunRewardPickupDebugView(PC);
	const FWacomRunWorldClickableInteractableDebugView ClickDebug =
		GetRunWorldClickableDebugView_Implementation(PC);
	return FString::Printf(
		TEXT("RunRewardPickup{Actor=%s PersistentId=%s Definition=%s PickupId=%s RewardType=%s Gold=%d Card=%s CardId=%s HasRun=%s CanInteract=%s Collected=%s TriggerRadius=%.1f BoundsExtent=%s Visual=%s ConfigValid=%s ConfigReason=%s Duplicate=%s HasVisual=%s ClickTarget=%s ClickStableId=%s HoverPrompt=%s CollectedHoverPrompt=%s Last=%s ClickDebug=%s}"),
		*View.ActorName,
		*View.PersistentId.ToString(),
		*View.PickupDefinitionName,
		*View.PickupId.ToString(),
		*View.RewardType.ToString(),
		View.GoldAmount,
		*View.CardDefinitionName,
		*View.CardId.ToString(),
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

void AWacomRunRewardPickupActor::LogRunRewardPickupDebugSummary(
	AWacomPlayerController* PC) const
{
	UE_LOG(LogTemp, Display, TEXT("[RunRewardPickupActor] %s"),
		*GetRunRewardPickupDebugSummary(PC));
}

void AWacomRunRewardPickupActor::ConfigureDebugGoldDefinitionPickupSample()
{
	if (UWacomRunPickupDefinition* LoadedDefinition =
		LoadObject<UWacomRunPickupDefinition>(nullptr, DebugGoldDefinitionPath))
	{
		PickupDefinition = LoadedDefinition;
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunRewardPickupActor] Failed to load debug gold PickupDefinition: %s"),
			DebugGoldDefinitionPath);
	}

	ApplyDebugPickupAuthoringDefaults(
		BuildDebugPersistentIdFromActorName(
			GetName(),
			TEXT("Pickup.Debug.Definition."),
			TEXT("RewardGold")));
}

void AWacomRunRewardPickupActor::ConfigureDebugPoisonFangDefinitionPickupSample()
{
	if (UWacomRunPickupDefinition* LoadedDefinition =
		LoadObject<UWacomRunPickupDefinition>(nullptr, DebugPoisonFangDefinitionPath))
	{
		PickupDefinition = LoadedDefinition;
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunRewardPickupActor] Failed to load debug poison fang PickupDefinition: %s"),
			DebugPoisonFangDefinitionPath);
	}

	ApplyDebugPickupAuthoringDefaults(
		BuildDebugPersistentIdFromActorName(
			GetName(),
			TEXT("Pickup.Debug.Definition.Card."),
			TEXT("RewardPoisonFang")));
}

FName AWacomRunRewardPickupActor::GetRewardConfigWarningReason() const
{
	if (!PickupDefinition)
	{
		return TEXT("MissingPickupDefinition");
	}
	return PickupDefinition->GetRewardConfigWarningReason();
}

bool AWacomRunRewardPickupActor::TryCollectPickupReward(AWacomPlayerController* PC)
{
	if (!PickupDefinition)
	{
		return false;
	}

	URunSession* Run = PC ? PC->GetRunSession() : nullptr;
	if (!Run)
	{
		return false;
	}

	const FRunTreasureSettlementResult Result =
		Run->CollectPickupFromDefinition(PersistentId, PickupDefinition);
	if (!Result.bSucceeded)
	{
		return false;
	}

	const TCHAR* PresentationSource = PickupDefinition->RewardType == EWacomRunPickupRewardType::Card
		? TEXT("RunRewardPickup.Card")
		: TEXT("RunRewardPickup.Gold");
	FWacomRunTreasurePresentationSync::ApplySettlement(PC, Result, PresentationSource);

	switch (PickupDefinition->RewardType)
	{
	case EWacomRunPickupRewardType::Gold:
	{
		if (UGameInstance* GameInstance = PC ? PC->GetGameInstance() : nullptr)
		{
			if (UWacomAppToastSubsystem* ToastSubsystem =
				GameInstance->GetSubsystem<UWacomAppToastSubsystem>())
			{
				ToastSubsystem->ShowGoldChanged(PickupDefinition->GoldAmount);
			}
		}
		return true;
	}
	case EWacomRunPickupRewardType::Card:
	{
		if (UGameInstance* GameInstance = PC ? PC->GetGameInstance() : nullptr)
		{
			if (UWacomAppToastSubsystem* ToastSubsystem =
				GameInstance->GetSubsystem<UWacomAppToastSubsystem>())
			{
				ToastSubsystem->ShowCardGained(PickupDefinition->CardDefinition);
			}
		}
		return true;
	}
	case EWacomRunPickupRewardType::None:
	default:
		return false;
	}
}

FText AWacomRunRewardPickupActor::GetDefaultInteractPromptText() const
{
	if (PickupDefinition
		&& PickupDefinition->RewardType == EWacomRunPickupRewardType::Card)
	{
		return LOCTEXT("DefaultCardInteractPrompt", "按 E 拾取卡牌");
	}
	return LOCTEXT("DefaultInteractPrompt", "按 E 拾取");
}

FText AWacomRunRewardPickupActor::GetDefaultHoverPromptText() const
{
	if (PickupDefinition
		&& PickupDefinition->RewardType == EWacomRunPickupRewardType::Card)
	{
		return LOCTEXT("DefaultCardHoverPrompt", "点击拾取卡牌");
	}
	return LOCTEXT("DefaultHoverPrompt", "点击拾取");
}

FText AWacomRunRewardPickupActor::GetDefaultCollectedHoverPromptText() const
{
	return LOCTEXT("DefaultCollectedHoverPrompt", "已拾取");
}

#undef LOCTEXT_NAMESPACE
