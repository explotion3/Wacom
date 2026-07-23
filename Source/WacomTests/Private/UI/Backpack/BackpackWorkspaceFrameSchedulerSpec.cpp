// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceFrameScheduler.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceFrameSchedulerContractSpec,
	"Wacom.UI.Backpack.Workspace.FrameScheduler.Contract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceFrameSchedulerContractSpec::RunTest(
	const FString& /*Parameters*/)
{
	FWacomBackpackWorkspaceFrameScheduler Scheduler;
	const FGuid First(1, 2, 3, 4);
	const FGuid Second(5, 6, 7, 8);

	Scheduler.RequestPresentation(
		EWacomBackpackWorkspacePresentationDirty::StaticCards,
		MakeArrayView(&First, 1));
	Scheduler.RequestPresentation(
		EWacomBackpackWorkspacePresentationDirty::Accessibility,
		MakeArrayView(&Second, 1));
	TestTrue(TEXT("Presentation reasons merge within one frame"),
		Scheduler.PeekPresentation().Has(
			EWacomBackpackWorkspacePresentationDirty::StaticCards
			| EWacomBackpackWorkspacePresentationDirty::Accessibility));
	TestTrue(TEXT("Scoped card refreshes merge their InstanceIds"),
		Scheduler.PeekPresentation().IncludesCard(First)
		&& Scheduler.PeekPresentation().IncludesCard(Second));

	Scheduler.RequestPresentation(
		EWacomBackpackWorkspacePresentationDirty::MotionTarget,
		{},
		true);
	TestTrue(TEXT("An all-card request dominates scoped identities"),
		Scheduler.PeekPresentation().bAllCards
		&& Scheduler.PeekPresentation().CardInstanceIds.IsEmpty());
	const FWacomBackpackWorkspacePresentationRequest Consumed =
		Scheduler.ConsumePresentation();
	TestTrue(TEXT("Consuming returns the complete merged request"),
		Consumed.Has(EWacomBackpackWorkspacePresentationDirty::MotionTarget)
		&& Consumed.bAllCards);
	TestTrue(TEXT("Consuming clears pending presentation work"),
		Scheduler.PeekPresentation().IsEmpty());

	Scheduler.SetWork(EWacomBackpackWorkspaceFrameWork::Motion, true);
	Scheduler.SetWork(EWacomBackpackWorkspaceFrameWork::Settlement, true);
	TestTrue(TEXT("Independent continuous work reasons coexist"),
		Scheduler.HasWork(EWacomBackpackWorkspaceFrameWork::Motion)
		&& Scheduler.HasWork(EWacomBackpackWorkspaceFrameWork::Settlement));
	Scheduler.SetWork(EWacomBackpackWorkspaceFrameWork::Motion, false);
	TestFalse(TEXT("Clearing one work reason preserves the other"),
		Scheduler.HasWork(EWacomBackpackWorkspaceFrameWork::Motion));
	TestTrue(TEXT("Remaining work keeps the scheduler awake"),
		Scheduler.WantsFrame());

	const uint64 FirstGeneration = Scheduler.MarkTimerRegistered();
	TestEqual(TEXT("Repeated wake does not create another timer generation"),
		Scheduler.MarkTimerRegistered(), FirstGeneration);
	Scheduler.MarkTimerStopped(FirstGeneration + 1);
	TestTrue(TEXT("A stale timer cannot stop the active generation"),
		Scheduler.IsTimerCurrent(FirstGeneration));
	Scheduler.MarkTimerStopped(FirstGeneration);
	TestFalse(TEXT("The current generation may stop itself"),
		Scheduler.IsTimerRegistered());
	Scheduler.SetWork(EWacomBackpackWorkspaceFrameWork::Settlement, false);

	FVector2D StableSize = FVector2D::ZeroVector;
	TestTrue(TEXT("Geometry stabilization starts once"),
		Scheduler.RequestGeometryStabilization());
	TestFalse(TEXT("Repeated geometry requests coalesce"),
		Scheduler.RequestGeometryStabilization());
	TestFalse(TEXT("First valid geometry sample is not yet stable"),
		Scheduler.PushGeometrySample(
			FVector2D(920.0f, 580.0f), 0.5f, 2, StableSize));
	TestTrue(TEXT("Second equal geometry sample is accepted"),
		Scheduler.PushGeometrySample(
			FVector2D(920.0f, 580.0f), 0.5f, 2, StableSize));
	TestTrue(TEXT("Accepted geometry preserves the sampled size"),
		StableSize.Equals(FVector2D(920.0f, 580.0f)));
	TestFalse(TEXT("Accepted geometry clears its continuous work"),
		Scheduler.HasWork(
			EWacomBackpackWorkspaceFrameWork::GeometryStabilization));

	Scheduler.RequestDeferredCardFaceRender();
	TestFalse(TEXT("Deferred card render cannot execute in its request frame"),
		Scheduler.IsDeferredCardFaceRenderReady());
	Scheduler.BeginFrame();
	TestTrue(TEXT("Deferred card render becomes eligible on the next frame"),
		Scheduler.IsDeferredCardFaceRenderReady());
	Scheduler.RequestDeferredCardFaceRender();
	Scheduler.CompleteDeferredCardFaceRender();
	TestFalse(TEXT("Coalesced deferred render completes as one request"),
		Scheduler.IsDeferredCardFaceRenderPending());

	Scheduler.BeginFrame();
	Scheduler.RequestDeferredCardFaceRender();
	TestFalse(TEXT("A request created during a frame waits for another frame"),
		Scheduler.IsDeferredCardFaceRenderReady());
	Scheduler.BeginFrame();
	TestTrue(TEXT("The in-frame request becomes ready one frame later"),
		Scheduler.IsDeferredCardFaceRenderReady());
	Scheduler.SuspendDeferredCardFaceRender();
	TestFalse(TEXT("A hidden card layer suspends deferred work without losing it"),
		Scheduler.HasWork(
			EWacomBackpackWorkspaceFrameWork::DeferredCardFaceRender));
	TestTrue(TEXT("Suspended deferred work remains pending"),
		Scheduler.IsDeferredCardFaceRenderPending());
	TestFalse(TEXT("Suspended deferred work does not create an empty frame loop"),
		Scheduler.WantsFrame());
	Scheduler.ResumeDeferredCardFaceRender();
	TestTrue(TEXT("Visible retained rendering wakes the pending request"),
		Scheduler.WantsFrame());

	const uint64 ResetGeneration = Scheduler.MarkTimerRegistered();
	Scheduler.Reset();
	TestFalse(TEXT("Reset clears every pending work source"),
		Scheduler.WantsFrame());
	TestFalse(TEXT("Reset invalidates an already registered timer"),
		Scheduler.IsTimerCurrent(ResetGeneration));
	return true;
}

#endif
