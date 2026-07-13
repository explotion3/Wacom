// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Deck/RunDeckBatchTypes.h"
#include "RunStateTypes.h"

class UCardDefinition;
struct FRunState;

struct FRunOwnedCardLocation
{
	FCardInstance Instance;
	EZoneKind Zone = EZoneKind::Backpack;
	FGuid ZoneOwnerInstanceId;
	int32 CardIndex = INDEX_NONE;
};

/**
 * 背包、备战区、特殊存放区、负重区的私有规则 helper。
 *
 * 只操作 FRunState，不广播、不访问 UI、不处理商店 / RunEvent / SaveGame。
 * URunSession 仍负责 public API、命令提交和 NotifyRunStateChanged。
 */
struct FRunDeckRules
{
	static bool IsContainerCard(const UCardDefinition* Card);
	static bool IsTypeAContainerCard(const UCardDefinition* Card);
	static bool IsTypeBContainerCard(const UCardDefinition* Card);
	static int32 GetSpecialZoneCapacity(const UCardDefinition* BCard);

	static bool IsFluxContentCardDefinition(const UCardDefinition* Card);
	static bool IsPreferredBurdenOverflowCandidate(const UCardDefinition* Card);

	static void EnsureSpecialZoneEntryFor(FRunState& State, const FCardInstance& Inst);

	static bool FindInstance(const FRunState& State, FGuid InstanceId, FCardInstance& OutInstance, EZoneKind& OutZone, FGuid& OutZoneOwnerInstanceId);
	static bool GetSpecialZone(const FRunState& State, FGuid OwnerInstanceId, FSpecialZone& Out);
	static int32 GetSpecialZoneCapacityFor(const FRunState& State, FGuid OwnerInstanceId);
	static void CollectTypeBContainers(const FRunState& State, TArray<FGuid>& OutOwnerInstanceIds);

	static bool FindFirstOwnedCardDefinition(const FRunState& State, const UCardDefinition* Card, FRunOwnedCardLocation& OutLocation);
	static bool FindOwnedCardInstance(const FRunState& State, FGuid InstanceId, FRunOwnedCardLocation& OutLocation);
	static bool DoesRunOwnCardDefinition(const FRunState& State, const UCardDefinition* Card);
	static bool HasCapacityProviderAfterDestroyingFirstOwnedInstance(const FRunState& State, const UCardDefinition* Card);
	static bool HasCapacityProviderAfterDestroyingOwnedInstance(const FRunState& State, FGuid InstanceId);
	static FRunDeckOperationValidation ValidatePermanentRemoveCard(const FRunState& State, const UCardDefinition* Card);
	static FRunDeckOperationValidation ValidatePermanentRemoveInstance(const FRunState& State, FGuid InstanceId);
	static bool PermanentRemoveOwnedCard(FRunState& State, UCardDefinition* Card, FName* OutDisabledReason = nullptr);
	static bool PermanentRemoveOwnedInstance(FRunState& State, FGuid InstanceId, FName* OutDisabledReason = nullptr);
	static int32 GetDeleteGoldRewardForCard(const UCardDefinition* Card);

	static int32 SumOwnedCardCapacity(const FRunState& State, bool bTypeAOnly);
	static int32 CountFluxContentCards(const TArray<FCardInstance>& Pile);

	static FRunDeckOperationValidation ValidateMoveInstance(const FRunState& State, FGuid InstanceId, EZoneKind ToZone, FGuid ToZoneOwnerInstanceId);
	static bool MoveInstance(FRunState& State, FGuid InstanceId, EZoneKind ToZone, FGuid ToZoneOwnerInstanceId, FRunOwnedCardLocation* OutFromLocation = nullptr, FName* OutDisabledReason = nullptr);

	/** 规范化 Zone 地址：非 SpecialZone 强制清空 OwnerInstanceId。 */
	static FRunDeckZoneAddress NormalizeZoneAddress(const FRunDeckZoneAddress& Address);

	/** 校验空集合、重复/无效 InstanceId、严格 revision 和共同来源；不修改 State。 */
	static FRunDeckBatchOperationValidation ValidateBatchInstanceSet(
		const FRunState& State,
		uint64 CurrentStorageRevision,
		const TArray<FGuid>& InstanceIds,
		const FRunDeckZoneAddress& ExpectedSource,
		uint64 ExpectedStorageRevision);

	/** 在只读权威 State 上预演整组移动；不修改 State。 */
	static FRunDeckBatchOperationValidation ValidateMoveInstancesAtomic(
		const FRunState& State,
		uint64 CurrentStorageRevision,
		const FRunDeckBatchMoveRequest& Request);

	/**
	 * 在调用方提供的 working state 上执行整组移动。
	 * 调用方只能在返回成功后提交 working state；失败时不得提交任何中间修改。
	 */
	static FRunDeckBatchOperationResult ApplyMoveInstancesAtomic(
		FRunState& InOutWorkingState,
		uint64 CurrentStorageRevision,
		const FRunDeckBatchMoveRequest& Request);

	/** 在只读权威 State 上计算整组永久移除合法性和总金币；不修改 State。 */
	static FRunDeckBatchDeletePreview ValidateDeleteCardsForGoldAtomic(
		const FRunState& State,
		uint64 CurrentStorageRevision,
		const FRunDeckBatchDeleteRequest& Request);

	/** 在 working state 上执行整组永久移除；失败时调用方丢弃 working state。 */
	static FRunDeckBatchOperationResult ApplyDeleteCardsForGoldAtomic(
		FRunState& InOutWorkingState,
		uint64 CurrentStorageRevision,
		const FRunDeckBatchDeleteRequest& Request);

	static FRunDeckOperationValidation ValidateSetSpecialZoneCardBattleEnabled(const FRunState& State, FGuid InstanceId, bool bEnabled);
	static FRunDeckOperationValidation ValidateToggleSpecialZoneCardBattleEnabled(const FRunState& State, FGuid InstanceId);
	static bool SetSpecialZoneCardBattleEnabled(FRunState& State, FGuid InstanceId, bool bEnabled, FName* OutDisabledReason = nullptr);
	static bool ToggleSpecialZoneCardBattleEnabled(FRunState& State, FGuid InstanceId, FName* OutDisabledReason = nullptr);
	static void RecomputeBurden(FRunState& State, bool bAllowBurdenRefill);
};
