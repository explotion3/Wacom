// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commands/BattleCommand.h"
#include "Cards/BattleCardPlacementFacts.h"
#include "Resolution/BattleTargetValidationResult.h"
#include "Types/WacomResult.h"

class UCardDefinition;
class FBattleCardTargetPreviewBuilder;
struct FBattleState;
struct FWacomInteractionTargetHandle;

/** PlayCard Evaluation 内部拒绝事实。 */
enum class EPlayCardEvaluationReject : uint8
{
	None,
	NotPlayerAction,
	NotPlayCardCommand,
	NoCardInstanceId,
	CardInstanceNotFound,
	CardNotInHand,
	CardHasNoDefinition,
	CardFrozen,
	UnsupportedTargetMode,
	InvalidInteractionTarget,
	UnsupportedWorldTarget,
	UnsupportedCardTarget,
	UnsupportedZoneTarget,
	MissingEnemyPartTarget,
	EnemyPartNotFound,
	EnemyPartDestroyed,
	EnemyPartIdentityMismatch,
	MissingHandCardTarget,
	HandCardNotFound,
	HandCardNotInHand,
	SelfTargetCard,
	NormalHandCardUnsupported,
	HandAnchorUnsupported,
	MissingRequiredTargetKeyword,
	BlockedTargetKeyword,
	HandCardFilterUnsupported,
	NotEnoughInitiative,
	StaleEvaluation,
};

enum class EPlayCardTargetBindingKind : uint8
{
	None,
	EnemyPart,
	HandCard,
};

/** 已解析的执行目标或只用于表现的 Preview Focus。 */
struct FPlayCardTargetFacts
{
	EPlayCardTargetBindingKind Kind = EPlayCardTargetBindingKind::None;
	FGuid EnemyPartInstanceId;
	FBattlePartSlotIdentity EnemyPartIdentity;
	FBattleEnemyPartKey EnemyPartKey;
	FGuid HandCardInstanceId;

	bool HasEnemyPart() const
	{
		return Kind == EPlayCardTargetBindingKind::EnemyPart
			&& EnemyPartInstanceId.IsValid()
			&& EnemyPartKey.IsValidKey();
	}

	bool HasHandCard() const
	{
		return Kind == EPlayCardTargetBindingKind::HandCard
			&& HandCardInstanceId.IsValid();
	}
};

/** ValidateTargetWithCard 使用的严格对象探测结果。 */
struct FPlayCardTargetProbeResult
{
	EPlayCardEvaluationReject Reject = EPlayCardEvaluationReject::InvalidInteractionTarget;
	FPlayCardTargetFacts Target;
	FWacomBattleTargetValidationResult Validation;
};

/**
 * Target Preview / Action Preview 共用的结构性候选。
 *
 * ExecutionTarget 会进入规范化命令；FocusTarget 只服务表现，永远不进入正式命令。
 */
class FPlayCardPreviewCandidate
{
	friend class FPlayCardEvaluator;
	friend class FBattleCardTargetPreviewBuilder;

public:
	FPlayCardPreviewCandidate(const FPlayCardPreviewCandidate&) = default;
	FPlayCardPreviewCandidate(FPlayCardPreviewCandidate&&) = default;
	FPlayCardPreviewCandidate& operator=(const FPlayCardPreviewCandidate&) = default;
	FPlayCardPreviewCandidate& operator=(FPlayCardPreviewCandidate&&) = default;

private:
	FPlayCardPreviewCandidate() = default;

	int32 EvaluatedStateVersion = INDEX_NONE;
	EPlayCardEvaluationReject Reject = EPlayCardEvaluationReject::InvalidInteractionTarget;
	EPlayCardEvaluationReject IgnoredFocusReject = EPlayCardEvaluationReject::None;
	bool bCanPreview = false;
	bool bFocusIgnored = false;

	FGuid SourceCardInstanceId;
	ECardTargetMode TargetMode = ECardTargetMode::None;
	const UCardDefinition* SourceDefinition = nullptr;
	int32 RuntimeCost = 0;
	bool bAnchor = false;
	bool bSwift = false;
	bool bCombo = false;

	FBattleCommand CanonicalCommand;
	FPlayCardTargetFacts ExecutionTarget;
	FPlayCardTargetFacts FocusTarget;
	FBattleCardPlacementFacts PrePlayPlacement;
	FWacomBattleTargetValidationResult Validation;
};

/** 仅可由 FPlayCardEvaluator 构造的可执行 PlayCard 事实。 */
class FPreparedPlayCard
{
	friend class FPlayCardEvaluator;

public:
	FPreparedPlayCard(const FPreparedPlayCard&) = default;
	FPreparedPlayCard(FPreparedPlayCard&&) = default;
	FPreparedPlayCard& operator=(const FPreparedPlayCard&) = default;
	FPreparedPlayCard& operator=(FPreparedPlayCard&&) = default;

	int32 GetEvaluatedStateVersion() const { return EvaluatedStateVersion; }
	const FBattleCommand& GetCanonicalCommand() const { return CanonicalCommand; }
	const UCardDefinition* GetSourceDefinition() const { return SourceDefinition; }
	int32 GetRuntimeCost() const { return RuntimeCost; }
	bool IsAnchor() const { return bAnchor; }
	bool IsSwift() const { return bSwift; }
	bool IsCombo() const { return bCombo; }
	const FPlayCardTargetFacts& GetExecutionTarget() const { return ExecutionTarget; }
	const FBattleCardPlacementFacts& GetPrePlayPlacement() const { return PrePlayPlacement; }

private:
	FPreparedPlayCard() = default;

	int32 EvaluatedStateVersion = INDEX_NONE;
	FBattleCommand CanonicalCommand;
	const UCardDefinition* SourceDefinition = nullptr;
	int32 RuntimeCost = 0;
	bool bAnchor = false;
	bool bSwift = false;
	bool bCombo = false;
	FPlayCardTargetFacts ExecutionTarget;
	FBattleCardPlacementFacts PrePlayPlacement;
};

/** 完整可提交性求值；成功时携带唯一可执行 Prepared PlayCard。 */
class FPlayCardCommitResult
{
	friend class FPlayCardEvaluator;

public:
	bool CanCommit() const { return Status.IsOk() && Prepared.IsSet(); }
	const FWacomStatus& GetStatus() const { return Status; }
	const FPreparedPlayCard& GetPrepared() const
	{
		check(Prepared.IsSet());
		return Prepared.GetValue();
	}

private:
	FWacomStatus Status = FWacomStatus::Fail(
		EWacomError::InvalidState,
		TEXT("UninitializedPlayCardEvaluation"));
	TOptional<FPreparedPlayCard> Prepared;
};

/**
 * PlayCard 前置求值的唯一 Private Module。
 *
 * 只读取 BattleState；不发事件、不修改状态、不消费 RNG，也不接触 Operation Adapter。
 */
class FPlayCardEvaluator
{
public:
	static FPlayCardTargetProbeResult EvaluateTargetProbe(
		const FBattleState& State,
		const FGuid& CardInstanceId,
		const FWacomInteractionTargetHandle& Target);

	static FPlayCardPreviewCandidate EvaluatePreviewCandidate(
		const FBattleState& State,
		const FGuid& CardInstanceId,
		const FWacomInteractionTargetHandle& Focus);

	static FPlayCardCommitResult EvaluateCommit(
		const FBattleState& State,
		const FBattleCommand& Command);

	static FPlayCardCommitResult EvaluateCommit(
		const FBattleState& State,
		const FPlayCardPreviewCandidate& Candidate);

private:
	static FPlayCardCommitResult MakeCommitFailure(EPlayCardEvaluationReject Reject);

	static FPlayCardCommitResult MakeCommitSuccess(
		int32 StateVersion,
		const FBattleCommand& CanonicalCommand,
		const UCardDefinition* SourceDefinition,
		int32 RuntimeCost,
		bool bAnchor,
		bool bSwift,
		bool bCombo,
		const FPlayCardTargetFacts& ExecutionTarget,
		const FBattleCardPlacementFacts& PrePlayPlacement);
};
