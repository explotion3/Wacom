// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "UI/Backpack/WacomBackpackScreenPresenter.h"
#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceCardFeedbackSpec,
	"Wacom.UI.Backpack.Workspace.CardFeedback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceCardFeedbackSpec::RunTest(const FString& Parameters)
{
	UWacomBackpackWorkspaceStyle* Style = NewObject<UWacomBackpackWorkspaceStyle>();
	Style->CardStateOverlayOpacity = 0.24f;
	Style->SelectionColor = FLinearColor(0.1f, 0.2f, 0.8f, 0.9f);
	Style->ValidTargetColor = FLinearColor(0.1f, 0.8f, 0.2f, 0.9f);
	Style->RejectedTargetColor = FLinearColor(0.9f, 0.1f, 0.2f, 0.9f);

	const FWacomBackpackWorkspaceCardVisualState Neutral =
		UWacomBackpackScreenPresenter::BuildWorkspaceCardVisualState(
			Style, false, false, false);
	TestEqual(TEXT("Neutral card keeps the feedback overlay hidden"), Neutral.FeedbackOpacity, 0.0f);

	const FWacomBackpackWorkspaceCardVisualState Selected =
		UWacomBackpackScreenPresenter::BuildWorkspaceCardVisualState(
			Style, true, false, false);
	TestEqual(TEXT("Selected card uses the configured feedback opacity"), Selected.FeedbackOpacity, 0.24f);
	TestEqual(TEXT("Selected card uses the configured selection color"), Selected.Tint, Style->SelectionColor);

	const FWacomBackpackWorkspaceCardVisualState Valid =
		UWacomBackpackScreenPresenter::BuildWorkspaceCardVisualState(
			Style, true, false, false, true, false);
	TestEqual(TEXT("Valid target color overrides selection color"), Valid.Tint, Style->ValidTargetColor);

	const FWacomBackpackWorkspaceCardVisualState Rejected =
		UWacomBackpackScreenPresenter::BuildWorkspaceCardVisualState(
			Style, true, false, true, true, true);
	TestEqual(TEXT("Rejected target color has the highest feedback priority"), Rejected.Tint, Style->RejectedTargetColor);
	TestEqual(TEXT("Read-only state preserves its lower opacity"), Rejected.Opacity, 0.72f);

	return true;
}

#endif
