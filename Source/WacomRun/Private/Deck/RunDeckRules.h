// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunStateTypes.h"

class UCardDefinition;
struct FRunState;

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

	static int32 SumOwnedCardCapacity(const FRunState& State, bool bTypeAOnly);
	static int32 CountFluxContentCards(const TArray<FCardInstance>& Pile);

	static FRunDeckOperationValidation ValidateMoveInstance(const FRunState& State, FGuid InstanceId, EZoneKind ToZone, FGuid ToZoneOwnerInstanceId);
	static void RecomputeBurden(FRunState& State, bool bAllowBurdenRefill);
};
