// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

class AWacomPlayerController;
class AWacomRunMapNodeAnchorActor;
class URunSession;
class UWacomRunMapScreen;
class UWacomRunPathTraversalComponent;

struct FWacomRunMapOpenGuardAutomationFacts
{
	bool bExplorationFlow = false;
	bool bHasSession = false;
	bool bHasCoordinator = false;
	bool bHasFlow = false;
	bool bHasTraversal = false;
	bool bTraversalAnchored = false;
	bool bCoordinatorTraversalActive = false;
	bool bSnapshotValid = false;
	bool bActiveActivity = false;
	bool bVersionsMatch = false;
	bool bDeadEnd = false;
};

/** Run Map App-private flow 的窄、非反射 automation seam。 */
struct WACOMAPP_API FWacomRunMapScreenFlowAutomationTestView
{
	FWacomRunMapScreenFlowAutomationTestView();
	~FWacomRunMapScreenFlowAutomationTestView();

	FWacomRunMapScreenFlowAutomationTestView(
		const FWacomRunMapScreenFlowAutomationTestView&) = delete;
	FWacomRunMapScreenFlowAutomationTestView& operator=(
		const FWacomRunMapScreenFlowAutomationTestView&) = delete;

	void ResetRegistry(FName FloorId);
	bool RegisterNodeAnchor(AWacomRunMapNodeAnchorActor& Anchor);
	bool Initialize(
		AWacomPlayerController& Owner,
		URunSession& Session,
		UWacomRunPathTraversalComponent& Traversal);
	int32 BeginOpenRequest();
	bool IsOpenRequestCurrent(int32 RequestGeneration) const;
	void CancelOpenRequest(int32 RequestGeneration);
	bool AttachScreen(
		URunSession& Session,
		UWacomRunMapScreen& Screen,
		bool bPreferRecommended,
		int32 RequestGeneration = 0);
	void SetForceInvalidTargetTransform(bool bEnabled);
	void SetForceCommittedPresentationFailure(bool bEnabled);
	void HandleSessionChanged(URunSession* NewSession);
	void Shutdown();

	bool IsFlowActive() const;
	bool IsOpening() const;
	bool IsTravelSubmissionPending() const;
	int32 GetGeneration() const;
	int32 GetLastPresentedVersion() const;
	int32 GetCoordinatorVersion() const;
	FName GetLastOutcomeDetail() const;
	static bool EvaluateOpenGuard(
		const FWacomRunMapOpenGuardAutomationFacts& Facts,
		bool& bOutPreferRecommendedTarget,
		FName* OutRejectDetail = nullptr);
	static bool IsGameMenuSlotAvailable(
		bool bHasOtherActiveGameMenu,
		bool bHasPendingGameMenu);

private:
	struct FImpl;
	TUniquePtr<FImpl> Impl;
};

#endif
