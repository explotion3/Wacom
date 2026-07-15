// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

class AWacomRunMapNodeAnchorActor;
class AWacomRunPathSegmentActor;
class AActor;
class URunSession;
class UWacomRunPathTraversalComponent;
enum class EWacomMapNodeType : uint8;

/**
 * Narrow, non-reflected automation seam for the App-private exploration presentation flow.
 * Runtime callers must use the owning PlayerController flow instead of this test view.
 */
struct WACOMAPP_API FWacomRunExplorationPresentationAutomationTestView
{
	FWacomRunExplorationPresentationAutomationTestView();
	~FWacomRunExplorationPresentationAutomationTestView();

	FWacomRunExplorationPresentationAutomationTestView(
		const FWacomRunExplorationPresentationAutomationTestView&) = delete;
	FWacomRunExplorationPresentationAutomationTestView& operator=(
		const FWacomRunExplorationPresentationAutomationTestView&) = delete;

	void ResetRegistry(FName FloorId);
	bool RegisterPath(AWacomRunPathSegmentActor& Path);
	bool RegisterNodeAnchor(AWacomRunMapNodeAnchorActor& Anchor);
	bool RegisterContentHost(FName NodeId, EWacomMapNodeType NodeType, AActor& Host);
	void UnregisterNodeAnchor(const AWacomRunMapNodeAnchorActor& Anchor);
	void UnregisterContentHost(const AActor& Host);

	bool Initialize(
		URunSession& Session,
		UWacomRunPathTraversalComponent& TraversalComponent);
	void Shutdown();
	bool HandleBranchIntent(FName EdgeId);

	bool HasActiveTraversal() const;
	int32 GetLastAppliedVersion() const;
	FName GetLastErrorDetail() const;

private:
	struct FImpl;
	TUniquePtr<FImpl> Impl;
};

#endif
