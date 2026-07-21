// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunState.h"
#include "UI/Events/WacomRunEventChoiceButton.h"
#include "UI/Events/WacomRunEventScreen.h"
#include "UI/Shop/WacomShopScreen.h"

#if WITH_AUTOMATION_TESTS

class UWacomRunMenuDropTargetWidget;
class UWacomShopOfferRowWidget;

struct FWacomShopRunEventTestAccess
{
	static FWacomShopScreenAutomationTestView View(const UWacomShopScreen& Screen);
	static FWacomShopOfferPresentationView OfferView(const UWacomShopScreen& Screen, int32 Index);
	static UWacomShopOfferRowWidget* OfferRow(const UWacomShopScreen& Screen, int32 Index);
	static bool PurchaseOfferAt(UWacomShopScreen& Screen, int32 Index);
	static FWacomShopCardUpgradePresentationView UpgradeView(const UWacomShopScreen& Screen, int32 Index);
	static bool SelectUpgradeAt(UWacomShopScreen& Screen, int32 Index);
	static bool UpgradeSelected(UWacomShopScreen& Screen);
	static FText FormatPurchaseFailureToast(FName DisabledReason);

	static FWacomRunEventScreenAutomationTestView View(const UWacomRunEventScreen& Screen);
	static FRunEventChoiceSnapshot ChoiceSnapshot(const UWacomRunEventScreen& Screen, int32 Index);
	static UWacomRunEventChoiceButton* ChoiceButton(const UWacomRunEventScreen& Screen, int32 Index);
	static UWacomRunMenuDropTargetWidget* PaymentDropTarget(const UWacomRunEventScreen& Screen, int32 Index);
	static bool ChooseChoiceAt(UWacomRunEventScreen& Screen, int32 Index);
	static FWacomRunMenuCardDropResolveResult ResolveDrop(
		const UWacomRunEventScreen& Screen,
		const FWacomRunMenuCardDropResolveResult& Candidate);
	static bool SubmitDrop(
		UWacomRunEventScreen& Screen,
		const FWacomRunMenuCardDropResolveResult& Resolved,
		FWacomRunMenuCardDropResolveResult& OutSubmitted);
	static FWacomRunEventScreenDebugView DebugView(const UWacomRunEventScreen& Screen);
	static FString DebugSummary(const UWacomRunEventScreen& Screen);

	static FWacomRunEventChoiceButtonAutomationTestView View(
		const UWacomRunEventChoiceButton& Button);
	static int32 RequirementItemCount(const UWacomRunEventChoiceButton& Button);
	static FText RequirementItemText(const UWacomRunEventChoiceButton& Button, int32 Index);
	static int32 ConsequenceItemCount(const UWacomRunEventChoiceButton& Button);
	static FText ConsequenceItemText(const UWacomRunEventChoiceButton& Button, int32 Index);
};

#endif
