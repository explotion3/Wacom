// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunState.h"
#include "UI/Shop/WacomWorldShopEntryBoundsGuard.h"
#include "UI/Shop/WacomWorldShopExplorationHUDVisibilityGuard.h"
#include "UI/Shop/WacomWorldShopWidgetInputRouter.h"
#include "UI/Shop/WacomWorldShopHUDWidget.h"
#include "UI/Shop/WacomWorldShopPresentationHost.h"
#include "UObject/StrongObjectPtr.h"

class AWacomPlayerController;
class UWidgetComponent;
class UWacomWorldShopCardWidget;
struct FWacomFirstPersonViewStageRequest;

enum class EWacomWorldShopOpenResult : uint8
{
	NotEligible,
	Accepted,
	Failed,
};

/** PlayerController-owned World Shop presentation coordinator. */
class FWacomWorldShopActivityCoordinator
{
public:
	EWacomWorldShopOpenResult TryOpen(
		AWacomPlayerController& PlayerController,
		const FRunShopVisitRequest& Request,
		const FWacomFirstPersonViewStageRequest& StageRequest,
		const FWacomWorldShopPresentationHost& Host);

	bool RouteInputKey(const FKey& Key, EInputEvent Event);
	void Close(bool bEndVisit = true);
	void Shutdown();
	bool IsUsingHost(const AActor* Candidate) const
	{
		return Host.IsOwnedBy(Candidate) && State != EState::Inactive;
	}
	bool IsOwningInput() const { return State != EState::Inactive; }
	bool IsActive() const { return State == EState::Active; }
	uint32 GetGeneration() const { return Generation; }

private:
	enum class EState : uint8
	{
		Inactive,
		Staging,
		Active,
		Closing,
	};

	struct FWorldCardRecord
	{
		FGuid OfferId;
		FName SlotId = NAME_None;
		TWeakObjectPtr<UWidgetComponent> Component;
		TWeakObjectPtr<UWacomWorldShopCardWidget> Widget;
	};

	bool BeginVisitAndPresentation(uint32 ExpectedGeneration);
	bool RefreshPresentation();
	void HandleRunStateChanged();
	void HandleCardPrimaryAction(FGuid OfferId, uint32 IntentGeneration);
	void DestroyPresentation();
	void RestoreExplorationPresentation();
	void FinishClose();

	EState State = EState::Inactive;
	uint32 Generation = 0;
	bool bPurchaseInFlight = false;
	bool bVisitOwned = false;
	bool bPreviousShowMouseCursor = true;
	FGuid VisitToken;
	FRunShopVisitRequest PendingRequest;
	TWeakObjectPtr<AWacomPlayerController> PlayerController;
	FWacomWorldShopPresentationHost Host;
	TArray<FWorldCardRecord> WorldCards;
	TStrongObjectPtr<UWacomWorldShopHUDWidget> HUD;
	FWacomWorldShopEntryBoundsGuard EntryBoundsGuard;
	FWacomWorldShopExplorationHUDVisibilityGuard
		ExplorationHUDVisibilityGuard;
	FWacomWorldShopWidgetInputRouter InputRouter;
};
