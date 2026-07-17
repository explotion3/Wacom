// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/World.h"
#include "GameFramework/WacomGameMode.h"
#include "Map/WacomJourneyDefinition.h"
#include "RunSession.h"
#include "RunState.h"
#include "UI/JourneySummaryGameModeTestAccess.h"
#include "UI/JourneySummaryScreenTestAccess.h"
#include "UI/Menus/WacomJourneySummaryScreen.h"

#include "UObject/StrongObjectPtr.h"

namespace WacomJourneySummaryHandoffSpec
{
	struct FHarness
	{
		UWorld* World = nullptr;
		AWacomGameMode* GameMode = nullptr;

		FHarness()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false);
			if (World)
			{
				GameMode = World->SpawnActor<AWacomGameMode>();
				if (GameMode)
				{
					GameMode->SetSuppressJourneySummaryTravelForAutomation(true);
					FWacomJourneySummaryGameModeTestAccess::UseNativeFallback(*GameMode);
				}
			}
		}

		~FHarness()
		{
			if (World)
			{
				World->DestroyWorld(false);
				World = nullptr;
				GameMode = nullptr;
			}
		}
	};

	FRunExplorationResolution MakeSuccessResolution(bool bIncludeEvent)
	{
		FRunExplorationResolution Resolution;
		Resolution.PostSnapshot.Outcome = ERunOutcome::Succeeded;
		Resolution.PostSnapshot.bHasCompletionSummary = true;
		FRunCompletionSummary& Summary = Resolution.PostSnapshot.CompletionSummary;
		Summary.JourneyId = TEXT("Journey.Main.01");
		Summary.TerminalNode.FloorId = TEXT("Floor.Main.03");
		Summary.TerminalNode.NodeId = TEXT("Node.Guardian.01");
		Summary.CompletionDay = 6;
		Summary.EnteredFloorCount = 3;
		Summary.TotalFloorCount = 3;
		Summary.ResolvedNodeCount = 42;
		Summary.TotalNodeCount = 60;
		Summary.FinalPressure = 104;
		if (bIncludeEvent)
		{
			FRunExplorationEvent Event;
			Event.Type = ERunExplorationEventType::JourneySucceeded;
			Event.Node = Summary.TerminalNode;
			Event.Detail = Summary.JourneyId;
			Resolution.Events.Add(Event);
		}
		return Resolution;
	}

	TStrongObjectPtr<URunSession> MakeRunWithJourneyTitle()
	{
		TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
		UWacomJourneyDefinition* Journey = NewObject<UWacomJourneyDefinition>(Run.Get());
		Journey->JourneyId = TEXT("Journey.Main.01");
		Journey->DisplayName = FText::FromString(TEXT("蛇巢之路"));
		FWacomRunSessionTestAccess::GetMutableRunState(*Run).ExplorationState.JourneyDefinition = Journey;
		return Run;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIJourneySummaryEventAndPushFallbackSpec,
	"Wacom.UI.JourneySummary.Handoff.EventBarrierAndPushFailureFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIJourneySummaryEventAndPushFallbackSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomJourneySummaryHandoffSpec;
	FHarness Harness;
	if (!TestNotNull(TEXT("World"), Harness.World)
		|| !TestNotNull(TEXT("GameMode"), Harness.GameMode))
	{
		return false;
	}

	TStrongObjectPtr<URunSession> Run = MakeRunWithJourneyTitle();
	TestFalse(TEXT("Succeeded snapshot without JourneySucceeded event is ignored"),
		FWacomJourneySummaryGameModeTestAccess::Consume(
			*Harness.GameMode,
			MakeSuccessResolution(false),
			*Run));
	TestFalse(TEXT("No event consumption recorded"),
		Harness.GameMode->GetJourneySummaryHandoffAutomationTestView().bSuccessEventConsumed);

	TestTrue(TEXT("JourneySucceeded event is consumed"),
		FWacomJourneySummaryGameModeTestAccess::Consume(
			*Harness.GameMode,
			MakeSuccessResolution(true),
			*Run));
	FWacomJourneySummaryHandoffAutomationTestView View =
		Harness.GameMode->GetJourneySummaryHandoffAutomationTestView();
	TestEqual(TEXT("Journey display title comes from definition"),
		View.ViewData.JourneyTitle.ToString(), FString(TEXT("蛇巢之路")));
	TestEqual(TEXT("Completion day projected"), View.ViewData.CompletionDay, 6);
	TestEqual(TEXT("Final pressure projected"), View.ViewData.FinalPressure, 104);

	AddExpectedError(
		TEXT("Journey Summary 缺少 PlayerController/UIManager"),
		EAutomationExpectedErrorFlags::Contains,
		1);
	FWacomJourneySummaryGameModeTestAccess::CompleteSuccessBarrier(*Harness.GameMode);
	View = Harness.GameMode->GetJourneySummaryHandoffAutomationTestView();
	TestEqual(TEXT("Flow enters JourneySummary"), Harness.GameMode->GetGameFlowState(),
		EGameFlowState::JourneySummary);
	TestTrue(TEXT("Return-to-Run barrier completes before summary"), View.bBarrierCompleted);
	TestFalse(TEXT("Success path does not restore Run hand or toast"),
		View.bRunPresentationRestoreRequested);
	TestTrue(TEXT("Summary push is attempted"), View.bSummaryPushAttempted);
	TestFalse(TEXT("Harness intentionally has no UI shell"), View.bSummaryPushSucceeded);
	TestTrue(TEXT("Push failure enters the same handoff"), View.bHandoffRequested);
	TestTrue(TEXT("Primary layout teardown is requested"), View.bPrimaryLayoutTeardownRequested);
	TestTrue(TEXT("Travel is deferred"), View.bTravelScheduled);
	TestEqual(TEXT("Travel target is L_MainMenu package"), View.TravelLevelName,
		AWacomGameMode::GetJourneySummaryMainMenuLevelPackagePathForTravel());
	TestTrue(TEXT("Teardown order precedes schedule"), View.TeardownOrder < View.ScheduleOrder);

	FWacomJourneySummaryGameModeTestAccess::RequestHandoff(*Harness.GameMode);
	TestEqual(TEXT("Repeated handoff request is idempotent"),
		Harness.GameMode->GetJourneySummaryHandoffAutomationTestView().HandoffRequestCount,
		1);

	Harness.GameMode->FlushJourneySummaryTravelForAutomation();
	View = Harness.GameMode->GetJourneySummaryHandoffAutomationTestView();
	TestTrue(TEXT("Deferred travel executes"), View.bTravelExecuted);
	TestTrue(TEXT("Automation suppresses OpenLevel"), View.bActualTravelSuppressed);
	TestTrue(TEXT("Execution remains after scheduling"), View.ScheduleOrder < View.ExecuteOrder);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIJourneySummaryScreenIntentHandoffSpec,
	"Wacom.UI.JourneySummary.Handoff.ScreenIntentIsSingleTravelRequest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIJourneySummaryScreenIntentHandoffSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomJourneySummaryHandoffSpec;
	FHarness Harness;
	if (!TestNotNull(TEXT("World"), Harness.World)
		|| !TestNotNull(TEXT("GameMode"), Harness.GameMode))
	{
		return false;
	}

	TStrongObjectPtr<UWacomJourneySummaryScreen> Screen(
		NewObject<UWacomJourneySummaryScreen>());
	FWacomJourneySummaryScreenTestAccess::Build(*Screen);
	FWacomJourneySummaryScreenTestAccess::Construct(*Screen);
	FWacomJourneySummaryGameModeTestAccess::BindScreen(*Harness.GameMode, *Screen);

	FWacomJourneySummaryScreenTestAccess::ClickContinue(*Screen);
	Screen->RequestContinue();
	const FWacomJourneySummaryHandoffAutomationTestView View =
		Harness.GameMode->GetJourneySummaryHandoffAutomationTestView();
	TestEqual(TEXT("Screen emits one accepted GameMode handoff"), View.HandoffRequestCount, 1);
	TestTrue(TEXT("Screen intent schedules next-tick travel"), View.bTravelScheduled);
	TestTrue(TEXT("Teardown is ordered before scheduling"), View.TeardownOrder < View.ScheduleOrder);

	FWacomJourneySummaryScreenTestAccess::Destruct(*Screen);
	return true;
}
