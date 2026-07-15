// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Exploration/RunCampActivity.h"
#include "Exploration/RunExplorationCommand.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "Map/WacomFloorMapDefinition.h"
#include "RunSession.h"

namespace
{
	class FFakeCampActivityHandler final : public IRunCampActivityHandler
	{
	public:
		ERunCampActivityKind Kind = ERunCampActivityKind::CardUpgrade;
		bool bComplete = true;
		bool bFail = false;

		virtual ERunCampActivityKind GetKind() const override
		{
			return Kind;
		}

		virtual FWacomStatus Execute(
			const FRunCampActivityContext& Context,
			FRunCampActivityOutcome& OutOutcome) const override
		{
			if (bFail)
			{
				return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("FakeCampRejected"));
			}
			if (!Context.CampNode.IsValid())
			{
				return FWacomStatus::Fail(EWacomError::InvalidArgument, TEXT("MissingCampNode"));
			}
			OutOutcome.Kind = Kind;
			OutOutcome.bCompleted = bComplete;
			return FWacomStatus::Ok();
		}
	};

	struct FCampFixture
	{
		FWacomRunExplorationFixture Fixture;
		URunSession* Session = nullptr;

		FCampFixture()
		{
			UWacomFloorMapDefinition* Floor =
				Fixture.MakeLinearFloor(TEXT("Camp.Policy.Floor"), 4);
			Floor->Nodes[0].NodeId = TEXT("Current");
			Floor->Nodes[1].NodeId = TEXT("Camp.B");
			Floor->Nodes[2].NodeId = TEXT("Camp.A");
			Floor->Nodes[3].NodeId = TEXT("Far");
			Floor->EntryNodeId = Floor->Nodes[0].NodeId;
			for (FWacomMapNodeDefinition& Node : Floor->Nodes)
			{
				Node.bAllowsCamp = false;
			}
			Floor->Nodes[1].bAllowsCamp = true;
			Floor->Nodes[2].bAllowsCamp = true;
			Floor->Edges.Reset();
			auto AddEdge = [Floor](const FName Id, const FName From, const FName To)
			{
				FWacomMapEdgeDefinition& Edge = Floor->Edges.AddDefaulted_GetRef();
				Edge.EdgeId = Id;
				Edge.FromNodeId = From;
				Edge.ToNodeId = To;
			};
			AddEdge(TEXT("To.B"), TEXT("Current"), TEXT("Camp.B"));
			AddEdge(TEXT("To.A"), TEXT("Current"), TEXT("Camp.A"));
			AddEdge(TEXT("To.Far"), TEXT("Camp.B"), TEXT("Far"));

			Session = Fixture.CreateInitializedSession(
				nullptr,
				Fixture.MakeJourney({ Floor }, TEXT("Camp.Policy.Journey"))).Session;
			FRunState& State = FWacomRunSessionTestAccess::GetMutableRunState(*Session);
			for (FRunMapNodeProgress& Node : State.ExplorationState.FloorProgress[0].Nodes)
			{
				Node.Lifecycle = ERunMapNodeLifecycle::Resolved;
			}
			State.TimeState.CurrentTimePhase = ETimePhase::Night;
			State.TimeState.NightGate = ERunNightGate::AwaitingChoice;
			State.TimeState.RemainingActionPoints = 2;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCampNearestAndCancelTest,
	"Wacom.Run.Camp.NearestDirectedNodeAndCancelKeepsRelocation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCampNearestAndCancelTest::RunTest(const FString& /*Parameters*/)
{
	FCampFixture Camp;
	const FRunExplorationSnapshot Before = Camp.Session->BuildExplorationSnapshot();
	TestTrue(TEXT("Snapshot advertises Camp"), Before.bCanBeginCamp);
	TestEqual(TEXT("Equal distance uses stable NodeId"), Before.NearestCampNode.NodeId, FName(TEXT("Camp.A")));

	const FRunExplorationResolution Begin = Camp.Session->ResolveExplorationCommand(
		FRunExplorationCommand::BeginCamp(Before.StateVersion));
	TestTrue(TEXT("Camp begins"), Begin.IsOk());
	TestTrue(TEXT("Camp returns a ticket"), Begin.CampTicket.IsSet());
	TestEqual(TEXT("Camp relocates to selected node"), Begin.PostSnapshot.CurrentNode.NodeId, FName(TEXT("Camp.A")));
	TestEqual(TEXT("Reservation does not spend AP early"), Begin.PostSnapshot.Time.RemainingActionPoints, 2);
	TestEqual(TEXT("Camp owns the exclusive activity"), Begin.PostSnapshot.ActiveActivityKind, ERunExplorationActivityKind::Camp);

	const FRunExplorationResolution Cancel = Camp.Session->ResolveExplorationCommand(
		FRunExplorationCommand::CancelCamp(Begin.CampTicket.GetValue()));
	TestTrue(TEXT("Camp cancellation succeeds"), Cancel.IsOk());
	TestEqual(TEXT("Cancellation releases activity"), Cancel.PostSnapshot.ActiveActivityKind, ERunExplorationActivityKind::None);
	TestEqual(TEXT("Free relocation is retained"), Cancel.PostSnapshot.CurrentNode.NodeId, FName(TEXT("Camp.A")));
	TestEqual(TEXT("Cancellation spends no AP"), Cancel.PostSnapshot.Time.RemainingActionPoints, 2);

	const int32 VersionAfterCancel = Cancel.PostSnapshot.StateVersion;
	const FRunExplorationResolution Duplicate = Camp.Session->ResolveExplorationCommand(
		FRunExplorationCommand::CancelCamp(Begin.CampTicket.GetValue()));
	TestFalse(TEXT("Duplicate cancellation is rejected"), Duplicate.IsOk());
	TestEqual(TEXT("Duplicate has no state effect"), Duplicate.PostSnapshot.StateVersion, VersionAfterCancel);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCampTypedCompletionTest,
	"Wacom.Run.Camp.TypedCompletionSkipsSunriseAndIsAtomic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCampTypedCompletionTest::RunTest(const FString& /*Parameters*/)
{
	FCampFixture Camp;
	const FRunExplorationResolution Begin = Camp.Session->ResolveExplorationCommand(
		FRunExplorationCommand::BeginCamp(Camp.Session->BuildExplorationSnapshot().StateVersion));
	TestTrue(TEXT("Camp begins"), Begin.IsOk() && Begin.CampTicket.IsSet());

	const int32 FatigueBefore = Camp.Session->GetRunState().Pressure.Get(EWacomPressureType::Fatigue);
	FFakeCampActivityHandler Handler;
	const FRunExplorationResolution Complete =
		Camp.Session->CompleteCampActivity(Begin.CampTicket.GetValue(), Handler);
	TestTrue(TEXT("Typed handler completes Camp"), Complete.IsOk());
	TestEqual(TEXT("Camp enters next Morning"), Complete.PostSnapshot.Time.CurrentTimePhase, ETimePhase::Morning);
	TestEqual(TEXT("Camp advances one day"), Complete.PostSnapshot.Time.CurrentDayNumber, 2);
	TestEqual(TEXT("Morning Planning is already paid"), Complete.PostSnapshot.Time.RemainingActionPoints, 1);
	TestEqual(TEXT("Sunrise Fatigue is skipped"),
		Camp.Session->GetRunState().Pressure.Get(EWacomPressureType::Fatigue), FatigueBefore);
	TestEqual(TEXT("Completion releases activity"), Complete.PostSnapshot.ActiveActivityKind, ERunExplorationActivityKind::None);

	const FRunExplorationSnapshot BeforeDuplicate = Camp.Session->BuildExplorationSnapshot();
	const FRunExplorationResolution Duplicate =
		Camp.Session->CompleteCampActivity(Begin.CampTicket.GetValue(), Handler);
	TestFalse(TEXT("Camp completion is one-shot"), Duplicate.IsOk());
	TestEqual(TEXT("Duplicate preserves version"), Duplicate.PostSnapshot.StateVersion, BeforeDuplicate.StateVersion);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunCampRejectedHandlerRollbackTest,
	"Wacom.Run.Camp.RejectedHandlerPreservesPendingTransaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunCampRejectedHandlerRollbackTest::RunTest(const FString& /*Parameters*/)
{
	FCampFixture Camp;
	const FRunExplorationResolution Begin = Camp.Session->ResolveExplorationCommand(
		FRunExplorationCommand::BeginCamp(Camp.Session->BuildExplorationSnapshot().StateVersion));
	FFakeCampActivityHandler Handler;
	Handler.bFail = true;
	const FRunExplorationResolution Rejected =
		Camp.Session->CompleteCampActivity(Begin.CampTicket.GetValue(), Handler);
	TestFalse(TEXT("Rejected handler fails"), Rejected.IsOk());
	TestTrue(TEXT("Rejected handler emits no events"), Rejected.Events.IsEmpty());
	TestEqual(TEXT("Pending Camp remains active"), Rejected.PostSnapshot.ActiveActivityKind, ERunExplorationActivityKind::Camp);
	TestEqual(TEXT("Time remains Night"), Rejected.PostSnapshot.Time.CurrentTimePhase, ETimePhase::Night);
	TestEqual(TEXT("AP remains reserved but unspent"), Rejected.PostSnapshot.Time.RemainingActionPoints, 2);
	return true;
}

#endif
