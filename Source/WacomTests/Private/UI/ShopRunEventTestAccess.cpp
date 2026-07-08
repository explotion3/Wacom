// Copyright Wacom. All Rights Reserved.

#include "UI/ShopRunEventTestAccess.h"

#if WITH_AUTOMATION_TESTS

FWacomShopScreenAutomationTestView FWacomShopRunEventTestAccess::View(const UWacomShopScreen& Screen)
{
	return Screen.GetAutomationTestViewForTest();
}

FWacomShopOfferPresentationView FWacomShopRunEventTestAccess::OfferView(
	const UWacomShopScreen& Screen,
	int32 Index)
{
	return Screen.GetCachedOfferView(Index);
}

UWacomShopOfferRowWidget* FWacomShopRunEventTestAccess::OfferRow(
	const UWacomShopScreen& Screen,
	int32 Index)
{
	return Screen.GetOfferRowWidgetForTest(Index);
}

bool FWacomShopRunEventTestAccess::PurchaseOfferAt(UWacomShopScreen& Screen, int32 Index)
{
	return Screen.PurchaseOfferByIndex(Index);
}

FText FWacomShopRunEventTestAccess::FormatPurchaseFailureToast(FName DisabledReason)
{
	return UWacomShopScreen::BuildPurchaseFailureToastText(DisabledReason);
}

FWacomRunEventScreenAutomationTestView FWacomShopRunEventTestAccess::View(
	const UWacomRunEventScreen& Screen)
{
	return Screen.GetAutomationTestViewForTest();
}

FRunEventChoiceSnapshot FWacomShopRunEventTestAccess::ChoiceSnapshot(
	const UWacomRunEventScreen& Screen,
	int32 Index)
{
	return Screen.GetCachedChoiceSnapshot(Index);
}

UWacomRunEventChoiceButton* FWacomShopRunEventTestAccess::ChoiceButton(
	const UWacomRunEventScreen& Screen,
	int32 Index)
{
	return Screen.GetChoiceButtonWidgetForTest(Index);
}

UWacomRunMenuDropTargetWidget* FWacomShopRunEventTestAccess::PaymentDropTarget(
	const UWacomRunEventScreen& Screen,
	int32 Index)
{
	return Screen.GetPaymentDropTargetForTest(Index);
}

bool FWacomShopRunEventTestAccess::ChooseChoiceAt(UWacomRunEventScreen& Screen, int32 Index)
{
	return Screen.ChooseChoiceByIndex(Index);
}

FWacomRunMenuCardDropResolveResult FWacomShopRunEventTestAccess::ResolveDrop(
	const UWacomRunEventScreen& Screen,
	const FWacomRunMenuCardDropResolveResult& Candidate)
{
	return Screen.ResolveRunMenuCardDropIntent_Implementation(Candidate);
}

bool FWacomShopRunEventTestAccess::SubmitDrop(
	UWacomRunEventScreen& Screen,
	const FWacomRunMenuCardDropResolveResult& Resolved,
	FWacomRunMenuCardDropResolveResult& OutSubmitted)
{
	return Screen.SubmitRunMenuCardDropIntent_Implementation(Resolved, OutSubmitted);
}

FWacomRunEventScreenDebugView FWacomShopRunEventTestAccess::DebugView(
	const UWacomRunEventScreen& Screen)
{
	return Screen.GetRunEventScreenDebugView();
}

FString FWacomShopRunEventTestAccess::DebugSummary(const UWacomRunEventScreen& Screen)
{
	return Screen.GetRunEventScreenDebugSummary();
}

FWacomRunEventChoiceButtonAutomationTestView FWacomShopRunEventTestAccess::View(
	const UWacomRunEventChoiceButton& Button)
{
	return Button.GetAutomationTestViewForTest();
}

int32 FWacomShopRunEventTestAccess::RequirementItemCount(const UWacomRunEventChoiceButton& Button)
{
	const FWacomRunEventChoiceButtonAutomationTestView ButtonView = View(Button);
	return ButtonView.RequirementItemTexts.Num();
}

FText FWacomShopRunEventTestAccess::RequirementItemText(
	const UWacomRunEventChoiceButton& Button,
	int32 Index)
{
	const FWacomRunEventChoiceButtonAutomationTestView ButtonView = View(Button);
	return ButtonView.RequirementItemTexts.IsValidIndex(Index)
		? ButtonView.RequirementItemTexts[Index]
		: FText::GetEmpty();
}

int32 FWacomShopRunEventTestAccess::ConsequenceItemCount(const UWacomRunEventChoiceButton& Button)
{
	const FWacomRunEventChoiceButtonAutomationTestView ButtonView = View(Button);
	return ButtonView.ConsequenceItemTexts.Num();
}

FText FWacomShopRunEventTestAccess::ConsequenceItemText(
	const UWacomRunEventChoiceButton& Button,
	int32 Index)
{
	const FWacomRunEventChoiceButtonAutomationTestView ButtonView = View(Button);
	return ButtonView.ConsequenceItemTexts.IsValidIndex(Index)
		? ButtonView.ConsequenceItemTexts[Index]
		: FText::GetEmpty();
}

#endif
