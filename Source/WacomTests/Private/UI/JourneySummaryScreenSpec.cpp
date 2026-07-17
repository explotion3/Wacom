// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "UI/JourneySummaryScreenTestAccess.h"
#include "UI/Menus/WacomJourneySummaryScreen.h"

#include "UObject/StrongObjectPtr.h"

namespace WacomJourneySummaryScreenSpec
{
	TStrongObjectPtr<UWacomJourneySummaryScreen> MakeScreen()
	{
		TStrongObjectPtr<UWacomJourneySummaryScreen> Screen(
			NewObject<UWacomJourneySummaryScreen>());
		FWacomJourneySummaryScreenTestAccess::Build(*Screen);
		return Screen;
	}

	FWacomJourneySummaryViewData MakeViewData()
	{
		FWacomJourneySummaryViewData ViewData;
		ViewData.StatusTitle = FText::FromString(TEXT("Journey 成功"));
		ViewData.JourneyTitle = FText::FromString(TEXT("蛇巢之路"));
		ViewData.CompletionDay = 6;
		ViewData.EnteredFloorCount = 3;
		ViewData.TotalFloorCount = 3;
		ViewData.ResolvedNodeCount = 42;
		ViewData.TotalNodeCount = 60;
		ViewData.FinalPressure = 104;
		return ViewData;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIJourneySummaryFallbackAndViewDataSpec,
	"Wacom.UI.JourneySummary.Screen.FallbackAndViewData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIJourneySummaryFallbackAndViewDataSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomJourneySummaryScreenSpec;
	TStrongObjectPtr<UWacomJourneySummaryScreen> Screen = MakeScreen();
	Screen->ApplyViewData(MakeViewData());

	const FWacomJourneySummaryScreenAutomationTestView View =
		FWacomJourneySummaryScreenTestAccess::View(*Screen);
	TestEqual(TEXT("Status title"), View.StatusTitle.ToString(), FString(TEXT("Journey 成功")));
	TestEqual(TEXT("Journey title"), View.JourneyTitle.ToString(), FString(TEXT("蛇巢之路")));
	TestEqual(TEXT("Completion day"), View.DayText.ToString(), FString(TEXT("完成天数：6")));
	TestEqual(TEXT("Floor progress"), View.FloorProgressText.ToString(), FString(TEXT("Floor 进度：3 / 3")));
	TestEqual(TEXT("Node progress"), View.NodeProgressText.ToString(), FString(TEXT("Node 进度：42 / 60")));
	TestEqual(TEXT("Final pressure"), View.PressureText.ToString(), FString(TEXT("最终压力：104")));
	TestTrue(TEXT("Native fallback has Continue button"), View.bHasContinueButton);
	TestTrue(TEXT("Continue button is focusable"), View.bContinueButtonFocusable);
	TestTrue(TEXT("Screen auto-restores focus"), View.bAutoRestoreFocus);
	TestEqual(TEXT("Desired focus is Continue"), View.DesiredFocusTargetName,
		FName(TEXT("ContinueButton")));

	const TArray<FName> Names = FWacomJourneySummaryScreenTestAccess::WidgetNames(*Screen);
	for (const FName Expected : {
		FName(TEXT("StatusTitleText")),
		FName(TEXT("JourneyTitleText")),
		FName(TEXT("CompletionDayText")),
		FName(TEXT("FloorProgressText")),
		FName(TEXT("NodeProgressText")),
		FName(TEXT("FinalPressureText")),
		FName(TEXT("ContinueButton")) })
	{
		TestTrue(*FString::Printf(TEXT("Fallback contains %s"), *Expected.ToString()),
			Names.Contains(Expected));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIJourneySummaryInputAndLifecycleSpec,
	"Wacom.UI.JourneySummary.Screen.InputLifecycleAndSingleIntent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIJourneySummaryInputAndLifecycleSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomJourneySummaryScreenSpec;
	TStrongObjectPtr<UWacomJourneySummaryScreen> Screen = MakeScreen();
	int32 IntentCount = 0;
	Screen->OnContinueRequestedNative.AddLambda([&IntentCount]() { ++IntentCount; });

	FWacomJourneySummaryScreenTestAccess::Construct(*Screen);
	FWacomJourneySummaryScreenTestAccess::Construct(*Screen);
	FWacomJourneySummaryScreenTestAccess::ClickContinue(*Screen);
	TestTrue(TEXT("Escape is handled"),
		FWacomJourneySummaryScreenTestAccess::SendEscapeKeyDown(*Screen).IsEventHandled());
	Screen->RequestContinue();
	TestEqual(TEXT("Click, Back and direct request emit one intent"), IntentCount, 1);
	TestTrue(TEXT("One-shot state is visible to automation view"),
		FWacomJourneySummaryScreenTestAccess::View(*Screen).bContinueIntentSent);

	FWacomJourneySummaryScreenTestAccess::Destruct(*Screen);
	FWacomJourneySummaryScreenTestAccess::ClickContinue(*Screen);
	TestEqual(TEXT("Destruct removes button and outward delegate bindings"), IntentCount, 1);
	return true;
}
