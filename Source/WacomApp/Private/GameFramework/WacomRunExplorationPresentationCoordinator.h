// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Exploration/RunExplorationResolution.h"
#include "WacomRunSceneBindingRegistry.h"

class AActor;
class URunSession;
class UWacomRunPathTraversalComponent;

DECLARE_MULTICAST_DELEGATE_TwoParams(
	FWacomRunNodeContentPresentationRequestedNative,
	const FWacomMapNodeHandle&,
	AActor*);

/** Run Resolution 到场景表现的唯一 App-private 协调 seam。 */
class FWacomRunExplorationPresentationCoordinator
{
public:
	~FWacomRunExplorationPresentationCoordinator();

	bool Initialize(
		URunSession& InSession,
		UWacomRunPathTraversalComponent& InTraversal,
		FWacomRunSceneBindingRegistry& InRegistry);
	void Shutdown();

	bool HandleBranchIntent(FName EdgeId);
	bool ApplyMapTravel(const FWacomMapNodeHandle& TargetNode);
	void HandleSessionChanged(URunSession* NewSession);

	int32 GetLastAppliedVersion() const { return LastAppliedVersion; }
	FName GetLastErrorDetail() const { return LastErrorDetail; }
	bool HasActiveTraversal() const { return ActiveTicket.IsSet(); }

	FWacomRunNodeContentPresentationRequestedNative& OnNodeContentPresentationRequestedNative()
	{
		return NodeContentPresentationRequestedNative;
	}

private:
	bool ApplyResolution(const FRunExplorationResolution& Resolution);
	void HandleReachedStart();
	void HandleReachedEnd();
	bool CancelActiveTraversal(FName FailureDetail);
	void RecoverToSource();
	void DisableTraversal(FName FailureDetail);

	TWeakObjectPtr<URunSession> Session;
	TWeakObjectPtr<UWacomRunPathTraversalComponent> Traversal;
	FWacomRunSceneBindingRegistry* Registry = nullptr;
	TOptional<FRunTraversalTicket> ActiveTicket;
	TOptional<FWacomRunTraversalSceneBinding> ActiveSceneBinding;
	int32 LastAppliedVersion = 0;
	FName LastErrorDetail = NAME_None;
	FWacomRunNodeContentPresentationRequestedNative NodeContentPresentationRequestedNative;
};
