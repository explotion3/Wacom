// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Map/WacomJourneyDefinition.h"
#include "Map/WacomMapTypes.h"
#include "RunOutcomeTypes.h"
#include "RunStateTypes.h"
#include "RunExplorationTypes.generated.h"

class UWacomJourneyDefinition;

/** 节点生命周期只能单向推进：Hidden -> Revealed -> Visited -> Resolved。 */
UENUM(BlueprintType)
enum class ERunMapNodeLifecycle : uint8
{
	Hidden,
	Revealed,
	Visited,
	Resolved,
};

/** Night 的规则门控；AwaitingChoice 时必须先选择探索或 Camp。 */
UENUM(BlueprintType)
enum class ERunNightGate : uint8
{
	Closed,
	AwaitingChoice,
	ExplorationOpen,
};

/** 当前独占探索事务种类。规则层同一时刻至多拥有一个活动事务。 */
UENUM(BlueprintType)
enum class ERunExplorationActivityKind : uint8
{
	None,
	Traversal,
	NodeActivity,
	Camp,
	FloorTransitionConfirmation,
};

/** NodeActivity 的 typed 内容种类；Camp 有独立生命周期，不属于此枚举。 */
enum class ERunNodeActivityKind : uint8
{
	Encounter,
	RunEvent,
	Shop,
	Treasure,
};

/** Camp 只保留类型安全的活动边界；具体收益规则由后续独立切片提供。 */
enum class ERunCampActivityKind : uint8
{
	Rest,
	CardUpgrade,
	SpecialEvent,
	Backpack,
	Skill,
};

/** 一条节点运行时进度；LandmarkVisible 不会隐式推进 Lifecycle。 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunMapNodeProgress
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	FName NodeId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	ERunMapNodeLifecycle Lifecycle = ERunMapNodeLifecycle::Hidden;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	bool bLandmarkVisible = false;
};

/** 单层运行时进度。数组顺序跟随 Floor DataAsset，便于稳定 Snapshot 和调试。 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunFloorProgress
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	FName FloorId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	TArray<FRunMapNodeProgress> Nodes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	int32 EnteredDayNumber = 0;
};

/** 新探索核心的时间组合状态。迁移期与 FRunState 旧时间字段并存。 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunTimeState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Time")
	int32 CurrentDayNumber = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Time")
	ETimePhase CurrentTimePhase = ETimePhase::Morning;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Time")
	int32 RemainingActionPoints = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Time")
	FWacomRunPhaseActionPointBudgets PhaseBudgets;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Time")
	ERunNightGate NightGate = ERunNightGate::Closed;
};

/** 新探索核心的组合状态；不进入当前 SaveGame schema。 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunExplorationState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	TObjectPtr<UWacomJourneyDefinition> JourneyDefinition = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	FName CurrentFloorId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	FName CurrentNodeId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	TArray<FRunFloorProgress> FloorProgress;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	TSet<FWacomMapNodeHandle> UnlockedEntranceIds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	int32 FloorEnteredDayNumber = 1;

	/** 每日 Decay 已结算到的 Journey Day；只用于保证新 Morning 幂等，不进入当前 SaveGame。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pressure")
	int32 LastDailyDecayAppliedDayNumber = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	int32 ExplorationStateVersion = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	ERunExplorationActivityKind ActiveActivityKind = ERunExplorationActivityKind::None;
};

/** Snapshot 中的一条节点只读事实。 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunMapNodeSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	FWacomMapNodeHandle Handle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	EWacomMapNodeType NodeType = EWacomMapNodeType::Navigation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	ERunMapNodeLifecycle Lifecycle = ERunMapNodeLifecycle::Hidden;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	bool bLandmarkVisible = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	bool bCanMapTravel = false;
};

/** Snapshot 中的一条当前节点出边事实。 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunMapEdgeSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	FWacomMapEdgeHandle Handle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	FWacomMapNodeHandle TargetNode;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	bool bCanTraverse = false;
};

/** 已进入 Floor 的只读历史摘要；旧 Floor 永远不会重新成为 travel target。 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunFloorHistorySnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	FName FloorId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	int32 EnteredDayNumber = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	int32 TotalNodeCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	int32 RevealedNodeCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	int32 ResolvedNodeCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	bool bIsCurrentFloor = false;
};

/** 当前 Floor Entrance 的只读跨层预览事实。 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunFloorTransitionPreview
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	FWacomMapNodeHandle EntranceNode;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	FName TargetFloorId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	int32 KnownUnresolvedNodeCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	int32 HiddenNodeCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pressure")
	int32 CurrentPressure = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	bool bHasUnknownAreas = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	bool bRequirementsMet = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	bool bEntranceUnlocked = false;
};

/** Run/App 共享的纯事实 Snapshot；不包含 Actor、Spline、Widget 或世界坐标。 */
USTRUCT(BlueprintType)
struct WACOMRUN_API FRunExplorationSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	int32 StateVersion = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	FName JourneyId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Completion")
	ERunOutcome Outcome = ERunOutcome::InProgress;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Completion")
	bool bHasCompletionSummary = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Completion")
	FRunCompletionSummary CompletionSummary;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	FWacomMapNodeHandle CurrentNode;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Time")
	FRunTimeState Time;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	int32 FloorDay = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pressure")
	int32 TotalPressure = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	TArray<FRunMapNodeSnapshot> Nodes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	TArray<FRunMapEdgeSnapshot> OutgoingEdges;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	TArray<FRunFloorHistorySnapshot> FloorHistory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Camp")
	bool bCanBeginCamp = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Camp")
	FWacomMapNodeHandle NearestCampNode;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	bool bHasFloorTransitionPreview = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	FRunFloorTransitionPreview FloorTransitionPreview;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Map")
	ERunExplorationActivityKind ActiveActivityKind = ERunExplorationActivityKind::None;
};

/** 规则结果事件类型。事件只携带稳定身份和语义。 */
enum class ERunExplorationEventType : uint8
{
	Initialized,
	NodeRevealed,
	NodeVisited,
	NodeResolved,
	TraversalStarted,
	TraversalCancelled,
	TraversalCompleted,
	SceneRelocationRequested,
	NodeContentRequested,
	TimeAdvanced,
	PressureChanged,
	FloorTransitionRequested,
	FloorTransitionCompleted,
	CampStarted,
	CampCancelled,
	CampCompleted,
	NodeActivityStarted,
	NodeActivityCancelled,
	NodeActivityCompleted,
	JourneySucceeded,
};

struct WACOMRUN_API FRunExplorationEvent
{
	ERunExplorationEventType Type = ERunExplorationEventType::Initialized;
	FWacomMapNodeHandle Node;
	FWacomMapEdgeHandle Edge;
	FName Detail = NAME_None;
};

/** 由规则层生成、调用方只能原样回传的一次性 token。 */
class WACOMRUN_API FRunExplorationToken
{
public:
	FRunExplorationToken() = default;

	bool IsValid() const { return Value.IsValid(); }
	FString ToString() const { return Value.ToString(); }

	friend bool operator==(const FRunExplorationToken& A, const FRunExplorationToken& B)
	{
		return A.Value == B.Value;
	}

private:
	explicit FRunExplorationToken(const FGuid& InValue) : Value(InValue) {}

	FGuid Value;

	friend class URunSession;
	friend class FRunExplorationCommandResolver;
	friend class FRunCampModule;
	friend class FRunFloorTransitionModule;
};

/** BeginTraversal 成功后返回给 App 的只读通道票据。 */
struct WACOMRUN_API FRunTraversalTicket
{
	FRunExplorationToken Token;
	int32 VersionBefore = 0;
	FWacomMapEdgeHandle Edge;
	FWacomMapNodeHandle SourceNode;
	FWacomMapNodeHandle TargetNode;

	bool IsValid() const { return Token.IsValid() && Edge.IsValid() && SourceNode.IsValid() && TargetNode.IsValid(); }
};

/** NodeActivity 专用 opaque token，不能与 traversal/confirmation token 互换。 */
class WACOMRUN_API FRunNodeActivityToken
{
public:
	FRunNodeActivityToken() = default;

	bool IsValid() const { return Value.IsValid(); }
	FString ToString() const { return Value.ToString(); }

	friend bool operator==(const FRunNodeActivityToken& A, const FRunNodeActivityToken& B)
	{
		return A.Value == B.Value;
	}

private:
	explicit FRunNodeActivityToken(const FGuid& InValue) : Value(InValue) {}

	FGuid Value;

	friend class URunSession;
	friend class FRunNodeActivityModule;
};

/** Begin 成功后由规则签发、App 只能原样回传的活动票据。 */
struct WACOMRUN_API FRunNodeActivityTicket
{
	FRunNodeActivityToken Token;
	int32 VersionBefore = 0;
	FWacomMapNodeHandle Node;
	ERunNodeActivityKind Kind = ERunNodeActivityKind::Encounter;
	int32 ReservedActionPoints = 0;

	bool IsValid() const
	{
		return Token.IsValid() && VersionBefore > 0 && Node.IsValid()
			&& ReservedActionPoints >= 0;
	}
};

/** BeginCamp 成功后由规则签发；预留只锁定合法性，完成前不提前扣点。 */
struct WACOMRUN_API FRunCampTicket
{
	FRunExplorationToken Token;
	int32 VersionBefore = 0;
	FWacomMapNodeHandle CampNode;
	int32 ReservedActionPoints = 0;

	bool IsValid() const
	{
		return Token.IsValid() && VersionBefore > 0 && CampNode.IsValid()
			&& ReservedActionPoints == 1;
	}
};

/** RequestFloorTransition 成功后返回的单次确认票据。 */
struct WACOMRUN_API FRunFloorTransitionConfirmation
{
	FRunExplorationToken Token;
	int32 VersionBefore = 0;
	FRunFloorTransitionPreview Preview;

	bool IsValid() const
	{
		return Token.IsValid() && VersionBefore > 0
			&& Preview.EntranceNode.IsValid() && !Preview.TargetFloorId.IsNone();
	}
};
