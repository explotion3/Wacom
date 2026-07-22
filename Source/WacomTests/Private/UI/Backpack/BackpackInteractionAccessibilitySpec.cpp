// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "CommonInputBaseTypes.h"
#include "../BackpackScreenTestAccess.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackInteractionHintPresenter.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceAccessibility.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceTypes.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackInteractionAccessibilitySpec,
	"Wacom.UI.Backpack.Accessibility.ContextHints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackInteractionAccessibilitySpec::RunTest(const FString& Parameters)
{
	const FWacomBackpackInteractionHintView KeyboardIdle =
		FWacomBackpackInteractionHintPresenter::Build(
			ECommonInputType::MouseAndKeyboard,
			EWacomBackpackWorkspaceInteractionMode::Idle,
			false);
	TestTrue(TEXT("Keyboard hint exposes help discovery"),
		KeyboardIdle.ContextHint.ToString().Contains(TEXT("F1")));
	TestTrue(TEXT("Keyboard help documents non-mouse selection"),
		KeyboardIdle.HelpText.ToString().Contains(TEXT("Space")));

	const FWacomBackpackInteractionHintView GamepadCarry =
		FWacomBackpackInteractionHintPresenter::Build(
			ECommonInputType::Gamepad,
			EWacomBackpackWorkspaceInteractionMode::Carry,
			false);
	TestTrue(TEXT("Gamepad carry hint exposes primary release"),
		GamepadCarry.ContextHint.ToString().Contains(TEXT("A")));
	TestTrue(TEXT("Gamepad carry hint exposes card cycling"),
		GamepadCarry.ContextHint.ToString().Contains(TEXT("LB/RB")));
	TestTrue(TEXT("Gamepad help documents back/cancel"),
		GamepadCarry.HelpText.ToString().Contains(TEXT("B")));

	TestEqual(TEXT("Selected is the base semantic icon"),
		FWacomBackpackWorkspaceAccessibility::ResolveCardSemanticIcon(
			true, false, false),
		EWacomBackpackWorkspaceCardSemanticIcon::Selected);
	TestEqual(TEXT("Valid drop overrides selected"),
		FWacomBackpackWorkspaceAccessibility::ResolveCardSemanticIcon(
			true, true, false),
		EWacomBackpackWorkspaceCardSemanticIcon::ValidDrop);
	TestEqual(TEXT("Rejected drop overrides valid and selected"),
		FWacomBackpackWorkspaceAccessibility::ResolveCardSemanticIcon(
			true, true, true),
		EWacomBackpackWorkspaceCardSemanticIcon::RejectedDrop);

	TStrongObjectPtr<UWacomBackpackScreen> Screen(
		FWacomBackpackScreenTestAccess::Create(GetTransientPackage(), nullptr));
	TestNotNull(TEXT("Fallback Backpack screen exists for help lifecycle"), Screen.Get());
	if (Screen)
	{
		const FWacomBackpackControlsHelpLifecycleProbe HelpProbe =
			FWacomBackpackScreenTestAccess::ProbeControlsHelpLifecycle(*Screen);
		TestTrue(TEXT("Controls help opens as a modal layer"), HelpProbe.bOpened);
		TestTrue(TEXT("Controls help captures the previous focus owner"),
			HelpProbe.bPreviousFocusCaptured);
		TestTrue(TEXT("Closing controls help restores the previous focus"),
			HelpProbe.bFocusRestored);
		TestTrue(TEXT("Screen deactivation closes controls help"),
			HelpProbe.bHiddenAfterDeactivate);
	}
	return true;
}

#endif
