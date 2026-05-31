// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomRunCardPickupActor.h"

#define LOCTEXT_NAMESPACE "WacomRunCardPickupActor"

#include "Cards/CardDefinition.h"
#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"
#include "UI/Foundation/WacomAppToastSubsystem.h"

namespace
{
	const TCHAR* DebugPoisonFangPath =
		TEXT("/Game/Wacom/Data/Cards/Rewards/DA_Card_PoisonFang.DA_Card_PoisonFang");
}

AWacomRunCardPickupActor::AWacomRunCardPickupActor()
{
	InteractPromptText = GetDefaultInteractPromptText();
	HoverPromptText = GetDefaultHoverPromptText();
	CollectedHoverPromptText = GetDefaultCollectedHoverPromptText();
}

void AWacomRunCardPickupActor::ConfigureDebugCardPickupSample()
{
	Modify();
	PersistentId = BuildDebugPersistentIdFromActorName(
		GetName(),
		TEXT("Pickup.Debug.Card."),
		TEXT("Card"));
	if (UCardDefinition* LoadedCard = LoadObject<UCardDefinition>(nullptr, DebugPoisonFangPath))
	{
		CardDefinition = LoadedCard;
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RunCardPickupActor] %s: 无法加载调试卡牌资产 %s，保留当前 CardDefinition"),
			*GetName(),
			DebugPoisonFangPath);
	}
	TriggerRadius = 160.f;
	bDestroyWhenCollected = true;
	InteractPromptText = GetDefaultInteractPromptText();
	HoverPromptText = GetDefaultHoverPromptText();
	CollectedHoverPromptText = GetDefaultCollectedHoverPromptText();
	RefreshClickTargetBindingAndRuntimeTarget();
}

FWacomRunCardPickupDebugView AWacomRunCardPickupActor::GetRunCardPickupDebugView(
	AWacomPlayerController* PC) const
{
	const FWacomRunPickupBaseDebugView BaseView = GetRunPickupBaseDebugView(PC);

	FWacomRunCardPickupDebugView View;
	View.ActorName = BaseView.ActorName;
	View.PersistentId = BaseView.PersistentId;
	View.CardDefinitionName = CardDefinition ? CardDefinition->GetName() : TEXT("None");
	View.CardId = CardDefinition ? CardDefinition->CardId : NAME_None;
	View.bHasRunSession = BaseView.bHasRunSession;
	View.bCanInteract = BaseView.bCanInteract;
	View.bIsCollected = BaseView.bIsCollected;
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

FString AWacomRunCardPickupActor::GetRunCardPickupDebugSummary(
	AWacomPlayerController* PC) const
{
	const FWacomRunCardPickupDebugView View = GetRunCardPickupDebugView(PC);
	const FWacomRunWorldClickableInteractableDebugView ClickDebug =
		GetRunWorldClickableDebugView_Implementation(PC);
	return FString::Printf(
		TEXT("RunCardPickup{Actor=%s PersistentId=%s Card=%s CardId=%s HasRun=%s CanInteract=%s Collected=%s ConfigValid=%s ConfigReason=%s Duplicate=%s HasVisual=%s ClickTarget=%s ClickStableId=%s HoverPrompt=%s CollectedHoverPrompt=%s Last=%s ClickDebug=%s}"),
		*View.ActorName,
		*View.PersistentId.ToString(),
		*View.CardDefinitionName,
		*View.CardId.ToString(),
		View.bHasRunSession ? TEXT("true") : TEXT("false"),
		View.bCanInteract ? TEXT("true") : TEXT("false"),
		View.bIsCollected ? TEXT("true") : TEXT("false"),
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

void AWacomRunCardPickupActor::LogRunCardPickupDebugSummary(AWacomPlayerController* PC) const
{
	UE_LOG(LogTemp, Display, TEXT("[RunCardPickupActor] %s"),
		*GetRunCardPickupDebugSummary(PC));
}

FName AWacomRunCardPickupActor::GetRewardConfigWarningReason() const
{
	if (!CardDefinition)
	{
		return TEXT("MissingCardDefinition");
	}
	return NAME_None;
}

bool AWacomRunCardPickupActor::TryCollectPickupReward(AWacomPlayerController* PC)
{
	URunSession* Run = PC ? PC->GetRunSession() : nullptr;
	if (!Run || !Run->CollectCardPickup(PersistentId, CardDefinition))
	{
		return false;
	}

	if (UGameInstance* GameInstance = PC ? PC->GetGameInstance() : nullptr)
	{
		if (UWacomAppToastSubsystem* ToastSubsystem =
			GameInstance->GetSubsystem<UWacomAppToastSubsystem>())
		{
			ToastSubsystem->ShowCardGained(CardDefinition);
		}
	}
	return true;
}

FText AWacomRunCardPickupActor::GetDefaultInteractPromptText() const
{
	return LOCTEXT("DefaultInteractPrompt", "按 E 拾取卡牌");
}

FText AWacomRunCardPickupActor::GetDefaultHoverPromptText() const
{
	return LOCTEXT("DefaultHoverPrompt", "点击拾取卡牌");
}

FText AWacomRunCardPickupActor::GetDefaultCollectedHoverPromptText() const
{
	return LOCTEXT("DefaultCollectedHoverPrompt", "已拾取");
}

#undef LOCTEXT_NAMESPACE
