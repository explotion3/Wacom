// Copyright Wacom. All Rights Reserved.

#include "Testing/WacomRunMapScreenFlowAutomationTestView.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameFramework/WacomRunExplorationPresentationCoordinator.h"
#include "GameFramework/WacomRunSceneBindingRegistry.h"
#include "UI/Map/WacomRunMapOpenGuard.h"
#include "UI/Map/WacomRunMapScreenFlow.h"

struct FWacomRunMapScreenFlowAutomationTestView::FImpl
{
	FWacomRunSceneBindingRegistry Registry;
	FWacomRunExplorationPresentationCoordinator Coordinator;
	FWacomRunMapScreenFlow Flow;
};

FWacomRunMapScreenFlowAutomationTestView::FWacomRunMapScreenFlowAutomationTestView()
	: Impl(MakeUnique<FImpl>())
{
}

FWacomRunMapScreenFlowAutomationTestView::~FWacomRunMapScreenFlowAutomationTestView() = default;

void FWacomRunMapScreenFlowAutomationTestView::ResetRegistry(const FName FloorId)
{
	Impl->Registry.Reset(FloorId);
}

bool FWacomRunMapScreenFlowAutomationTestView::RegisterNodeAnchor(
	AWacomRunMapNodeAnchorActor& Anchor)
{
	return Impl->Registry.RegisterNodeAnchor(Anchor);
}

bool FWacomRunMapScreenFlowAutomationTestView::Initialize(
	AWacomPlayerController& Owner,
	URunSession& Session,
	UWacomRunPathTraversalComponent& Traversal)
{
	if (!Impl->Coordinator.Initialize(Session, Traversal, Impl->Registry))
	{
		return false;
	}
	Impl->Flow.Initialize(Owner, Impl->Coordinator);
	return true;
}

bool FWacomRunMapScreenFlowAutomationTestView::AttachScreen(
	URunSession& Session,
	UWacomRunMapScreen& Screen,
	const bool bPreferRecommended,
	const int32 RequestGeneration)
{
	return Impl->Flow.AttachScreen(
		Session, Screen, bPreferRecommended, RequestGeneration);
}

int32 FWacomRunMapScreenFlowAutomationTestView::BeginOpenRequest()
{
	return Impl->Flow.BeginOpenRequest();
}

bool FWacomRunMapScreenFlowAutomationTestView::IsOpenRequestCurrent(
	const int32 RequestGeneration) const
{
	return Impl->Flow.IsOpenRequestCurrent(RequestGeneration);
}

void FWacomRunMapScreenFlowAutomationTestView::CancelOpenRequest(
	const int32 RequestGeneration)
{
	Impl->Flow.CancelOpenRequest(RequestGeneration);
}

void FWacomRunMapScreenFlowAutomationTestView::SetForceInvalidTargetTransform(
	const bool bEnabled)
{
	Impl->Coordinator.bForceMapTravelTransformInvalidForAutomation = bEnabled;
}

void FWacomRunMapScreenFlowAutomationTestView::SetForceCommittedPresentationFailure(
	const bool bEnabled)
{
	Impl->Coordinator.bForceMapTravelAnchorApplyFailureForAutomation = bEnabled;
}

void FWacomRunMapScreenFlowAutomationTestView::HandleSessionChanged(URunSession* NewSession)
{
	Impl->Flow.HandleSessionChanged(NewSession);
}

void FWacomRunMapScreenFlowAutomationTestView::Shutdown()
{
	Impl->Flow.Shutdown();
	Impl->Coordinator.Shutdown();
}

bool FWacomRunMapScreenFlowAutomationTestView::IsFlowActive() const
{
	return Impl->Flow.IsActive();
}

bool FWacomRunMapScreenFlowAutomationTestView::IsOpening() const
{
	return Impl->Flow.IsOpening();
}

bool FWacomRunMapScreenFlowAutomationTestView::IsTravelSubmissionPending() const
{
	return Impl->Flow.IsTravelSubmissionPending();
}

int32 FWacomRunMapScreenFlowAutomationTestView::GetGeneration() const
{
	return Impl->Flow.GetGeneration();
}

int32 FWacomRunMapScreenFlowAutomationTestView::GetLastPresentedVersion() const
{
	return Impl->Flow.GetLastPresentedVersion();
}

int32 FWacomRunMapScreenFlowAutomationTestView::GetCoordinatorVersion() const
{
	return Impl->Coordinator.GetLastAppliedVersion();
}

FName FWacomRunMapScreenFlowAutomationTestView::GetLastOutcomeDetail() const
{
	return Impl->Flow.GetLastOutcomeDetail();
}

bool FWacomRunMapScreenFlowAutomationTestView::EvaluateOpenGuard(
	const FWacomRunMapOpenGuardAutomationFacts& Facts,
	bool& bOutPreferRecommendedTarget,
	FName* OutRejectDetail)
{
	FWacomRunMapOpenGuardFacts ProductionFacts;
	ProductionFacts.bExplorationFlow = Facts.bExplorationFlow;
	ProductionFacts.bHasSession = Facts.bHasSession;
	ProductionFacts.bHasCoordinator = Facts.bHasCoordinator;
	ProductionFacts.bHasFlow = Facts.bHasFlow;
	ProductionFacts.bHasTraversal = Facts.bHasTraversal;
	ProductionFacts.bTraversalAnchored = Facts.bTraversalAnchored;
	ProductionFacts.bCoordinatorTraversalActive = Facts.bCoordinatorTraversalActive;
	ProductionFacts.bSnapshotValid = Facts.bSnapshotValid;
	ProductionFacts.bActiveActivity = Facts.bActiveActivity;
	ProductionFacts.bVersionsMatch = Facts.bVersionsMatch;
	ProductionFacts.bDeadEnd = Facts.bDeadEnd;
	const FWacomRunMapOpenGuardDecision Decision =
		FWacomRunMapOpenGuard::Evaluate(ProductionFacts);
	bOutPreferRecommendedTarget = Decision.bPreferRecommendedTarget;
	if (OutRejectDetail)
	{
		*OutRejectDetail = Decision.RejectDetail;
	}
	return Decision.bCanOpen;
}

bool FWacomRunMapScreenFlowAutomationTestView::IsGameMenuSlotAvailable(
	const bool bHasOtherActiveGameMenu,
	const bool bHasPendingGameMenu)
{
	return FWacomRunMapOpenGuard::IsGameMenuSlotAvailable(
		bHasOtherActiveGameMenu,
		bHasPendingGameMenu);
}

#endif
