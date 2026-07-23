// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Exploration/RunExplorationResolution.h"
#include "WacomRunSceneBindingRegistry.h"

class URunSession;
class UWacomRunPathTraversalComponent;
struct FWacomRunMapScreenFlowAutomationTestView;

enum class EWacomRunRouteChoiceMode : uint8
{
	Unavailable,
	DeadEnd,
	Automatic,
	ChoiceRequired,
};

enum class EWacomRunForwardIntentResult : uint8
{
	Started,
	ChoiceRequired,
	DeadEnd,
	Unavailable,
	Rejected,
};

struct FWacomRunRouteChoiceState
{
	int32 SnapshotVersion = 0;
	EWacomRunRouteChoiceMode Mode = EWacomRunRouteChoiceMode::Unavailable;
	TArray<FName> LegalEdgeIds;

	bool operator==(const FWacomRunRouteChoiceState& Other) const
	{
		return SnapshotVersion == Other.SnapshotVersion
			&& Mode == Other.Mode
			&& LegalEdgeIds == Other.LegalEdgeIds;
	}
};

enum class EWacomRunMapTravelPresentationOutcome : uint8
{
	Rejected,
	Applied,
	CommittedPresentationFailed,
};

/** 区分提交前拒绝与规则已提交后的场景表现故障，避免 UI 重复提交。 */
struct FWacomRunMapTravelPresentationResult
{
	EWacomRunMapTravelPresentationOutcome Outcome =
		EWacomRunMapTravelPresentationOutcome::Rejected;
	FName Detail = NAME_None;
	int32 AppliedVersion = 0;

	bool DidCommit() const
	{
		return Outcome != EWacomRunMapTravelPresentationOutcome::Rejected;
	}

	bool IsApplied() const
	{
		return Outcome == EWacomRunMapTravelPresentationOutcome::Applied;
	}
};

struct FWacomRunNodeContentArrivalRequest
{
	FWacomMapNodeHandle Node;
	EWacomMapNodeType NodeType = EWacomMapNodeType::Navigation;
	int32 AppliedVersion = 0;
};

DECLARE_MULTICAST_DELEGATE_OneParam(
	FWacomRunNodeContentPresentationRequestedNative,
	const FWacomRunNodeContentArrivalRequest&);
DECLARE_MULTICAST_DELEGATE_OneParam(
	FWacomRunRouteChoiceStateChangedNative,
	const FWacomRunRouteChoiceState&);

/** Run Resolution 到场景表现的唯一 App-private 协调 seam。 */
class FWacomRunExplorationPresentationCoordinator
{
public:
	~FWacomRunExplorationPresentationCoordinator();

	bool Initialize(
		URunSession& InSession,
		UWacomRunPathTraversalComponent& InTraversal,
		FWacomRunSceneBindingRegistry& InRegistry);
	bool InitializeFromValidatedSnapshot(
		URunSession& InSession,
		UWacomRunPathTraversalComponent& InTraversal,
		FWacomRunSceneBindingRegistry& InRegistry,
		const FRunExplorationSnapshot& Snapshot);
	bool PrepareFromValidatedSnapshot(
		URunSession& InSession,
		UWacomRunPathTraversalComponent& InTraversal,
		FWacomRunSceneBindingRegistry& InRegistry,
		const FRunExplorationSnapshot& Snapshot);
	void CommitPreparedInitialization();
	void Shutdown();

	bool HandleBranchIntent(FName EdgeId);
	EWacomRunForwardIntentResult HandleForwardIntent();
	/**
	 * 应用由节点内容流程提交的显式规则结果，例如 Encounter Begin/Settlement。
	 * 该入口只推进版本与路线表现，不创建 traversal ticket，也不从 Session 猜测遗漏结果。
	 */
	bool ApplyNodeActivityResolution(const FRunExplorationResolution& Resolution);
	FWacomRunMapTravelPresentationResult ApplyMapTravel(
		const FWacomMapNodeHandle& TargetNode);
	void HandleSessionChanged(URunSession* NewSession);

	int32 GetLastAppliedVersion() const { return LastAppliedVersion; }
	FName GetLastErrorDetail() const { return LastErrorDetail; }
	bool HasActiveTraversal() const { return ActiveTicket.IsSet(); }
	const FWacomRunRouteChoiceState& GetRouteChoiceState() const { return RouteChoiceState; }
	FWacomRunRouteChoiceStateChangedNative& OnRouteChoiceStateChangedNative()
	{
		return RouteChoiceStateChangedNative;
	}

	FWacomRunNodeContentPresentationRequestedNative& OnNodeContentPresentationRequestedNative()
	{
		return NodeContentPresentationRequestedNative;
	}

private:
	bool ApplyResolution(const FRunExplorationResolution& Resolution);
	void RefreshRouteChoiceState(const FRunExplorationSnapshot& Snapshot);
	void HideRouteChoices(int32 SnapshotVersion);
	void SetRouteChoiceState(FWacomRunRouteChoiceState NewState);
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
	FWacomRunRouteChoiceState RouteChoiceState;
	FTransform PreparedInitialAnchorTransform = FTransform::Identity;
	bool bPreparedForInitializationCommit = false;
	FWacomRunNodeContentPresentationRequestedNative NodeContentPresentationRequestedNative;
	FWacomRunRouteChoiceStateChangedNative RouteChoiceStateChangedNative;

#if WITH_DEV_AUTOMATION_TESTS
	bool bForceMapTravelTransformInvalidForAutomation = false;
	bool bForceMapTravelAnchorApplyFailureForAutomation = false;
	friend struct FWacomRunMapScreenFlowAutomationTestView;
#endif
};
