// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "UI/Battle/WacomKnockdownChoiceDialog.h"
#include "UI/Menus/WacomConfirmDialog.h"
#include "UI/Menus/WacomPauseMenuScreen.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
struct FWacomMenuBackDelegateProbe
{
	int32 BackRequestedCount = 0;
	int32 DeactivatedCount = 0;

	void HandleBackRequested()
	{
		++BackRequestedCount;
	}

	void HandleDeactivated()
	{
		++DeactivatedCount;
	}
};

bool RunMenuBackRequestedHookTest(FAutomationTestBase& Test, TFunctionRef<FReply(UWacomPauseMenuScreen&)> SendBackKeyDown)
{
	TStrongObjectPtr<UWacomPauseMenuScreen> Widget(NewObject<UWacomPauseMenuScreen>());
	FWacomMenuBackDelegateProbe Probe;

	Widget->OnBackRequestedNative.AddRaw(&Probe, &FWacomMenuBackDelegateProbe::HandleBackRequested);
	Widget->OnDeactivated().AddRaw(&Probe, &FWacomMenuBackDelegateProbe::HandleDeactivated);

	Test.AddExpectedErrorPlain(
		TEXT("PlayerController并非有效的本地玩家，因此它无法聚焦"),
		EAutomationExpectedErrorFlags::Contains,
		1);
	Widget->ActivateWidget();

	Test.TestTrue(TEXT("Widget activates before back request"), Widget->IsActivated());

	const FReply Reply = SendBackKeyDown(*Widget);

	Test.TestTrue(TEXT("Back key is handled"), Reply.IsEventHandled());
	Test.TestEqual(TEXT("Back hook broadcasts exactly once"), Probe.BackRequestedCount, 1);
	Test.TestEqual(TEXT("Back request deactivates the widget exactly once"), Probe.DeactivatedCount, 1);
	Test.TestFalse(TEXT("Widget is deactivated after back request"), Widget->IsActivated());

	Widget->OnBackRequestedNative.RemoveAll(&Probe);
	Widget->OnDeactivated().RemoveAll(&Probe);

	return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIMenuBackRequestedEscapeSpec,
	"Wacom.UI.Menu.BackRequested.Escape",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIMenuBackRequestedEscapeSpec::RunTest(const FString& /*Parameters*/)
{
	return RunMenuBackRequestedHookTest(
		*this,
		[](UWacomPauseMenuScreen& Widget)
		{
			return Widget.HandleEscapeKeyDownForAutomationTest();
		});
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIMenuBackRequestedGamepadBackSpec,
	"Wacom.UI.Menu.BackRequested.GamepadBack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIMenuBackRequestedGamepadBackSpec::RunTest(const FString& /*Parameters*/)
{
	return RunMenuBackRequestedHookTest(
		*this,
		[](UWacomPauseMenuScreen& Widget)
		{
			return Widget.HandleGamepadBackKeyDownForAutomationTest();
		});
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIMenuBackRequestedConfirmDialogGamepadCancelsSpec,
	"Wacom.UI.Menu.BackRequested.ConfirmDialogGamepadCancels",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIMenuBackRequestedConfirmDialogGamepadCancelsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomConfirmDialog> Dialog(NewObject<UWacomConfirmDialog>());
	int32 CancelCount = 0;
	Dialog->SetCallbacksForAutomationTest(nullptr, [&CancelCount]()
	{
		++CancelCount;
	});

	AddExpectedErrorPlain(
		TEXT("PlayerController并非有效的本地玩家，因此它无法聚焦"),
		EAutomationExpectedErrorFlags::Contains,
		1);
	Dialog->ActivateWidget();

	const FReply Reply = Dialog->HandleGamepadBackKeyDownForAutomationTest();
	TestTrue(TEXT("Confirm dialog gamepad back is handled"), Reply.IsEventHandled());
	TestEqual(TEXT("Confirm dialog gamepad back triggers cancel callback"), CancelCount, 1);
	TestFalse(TEXT("Confirm dialog deactivates after cancel"), Dialog->IsActivated());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIMenuBackRequestedKnockdownGamepadBlockedSpec,
	"Wacom.UI.Menu.BackRequested.KnockdownGamepadBlocked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIMenuBackRequestedKnockdownGamepadBlockedSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomKnockdownChoiceDialog> Dialog(NewObject<UWacomKnockdownChoiceDialog>());
	int32 BackRequestedCount = 0;
	Dialog->OnBackRequestedNative.AddLambda([&BackRequestedCount]()
	{
		++BackRequestedCount;
	});

	AddExpectedErrorPlain(
		TEXT("PlayerController并非有效的本地玩家，因此它无法聚焦"),
		EAutomationExpectedErrorFlags::Contains,
		1);
	Dialog->ActivateWidget();

	const FReply Reply = Dialog->HandleGamepadBackKeyDownForAutomationTest();
	TestTrue(TEXT("Knockdown dialog gamepad back is handled"), Reply.IsEventHandled());
	TestEqual(TEXT("Knockdown dialog does not broadcast default back"), BackRequestedCount, 0);
	TestTrue(TEXT("Knockdown dialog remains active after blocked back"), Dialog->IsActivated());

	return true;
}
