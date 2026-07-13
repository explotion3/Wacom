// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunStateTypes.h"

/**
 * Run 物理持有区地址。
 *
 * SpecialZone 必须携带有效 OwnerInstanceId；其他 Zone 的 OwnerInstanceId 必须规范化为空。
 * 本类型是 C++ contract，不反射给 Blueprint，避免被动 WBP 直接构造规则命令。
 */
struct WACOMRUN_API FRunDeckZoneAddress
{
	EZoneKind Zone = EZoneKind::Backpack;
	FGuid OwnerInstanceId;
};

/** 背包工作台提交的一次原子批量跨区移动请求。 */
struct WACOMRUN_API FRunDeckBatchMoveRequest
{
	/** 按稳定工作台顺序排列的唯一物理卡 InstanceId。 */
	TArray<FGuid> InstanceIds;

	/** 起手时记录的共同物理来源；提交时所有卡仍须位于此处。 */
	FRunDeckZoneAddress ExpectedSource;

	/** 整组卡牌的唯一目标区。 */
	FRunDeckZoneAddress Target;

	/** 起手 Snapshot 的背包存放区 revision；必须与提交时当前 revision 严格相等。 */
	uint64 ExpectedStorageRevision = 0;
};

/** 背包工作台提交的一次原子批量删牌换金币请求。 */
struct WACOMRUN_API FRunDeckBatchDeleteRequest
{
	/** 按稳定工作台顺序排列的唯一物理卡 InstanceId。 */
	TArray<FGuid> InstanceIds;

	/** 打开确认前记录的共同物理来源；确认提交时必须重新核对。 */
	FRunDeckZoneAddress ExpectedSource;

	/** 确认预览时的背包存放区 revision；确认提交时必须严格相等。 */
	uint64 ExpectedStorageRevision = 0;
};

/** 原子批量规则的只读整组校验结果；失败路径不修改 FRunState。 */
struct WACOMRUN_API FRunDeckBatchOperationValidation
{
	bool bCanExecute = false;
	FName DisabledReason = NAME_None;
	int32 RequestedCount = 0;
	uint64 ValidatedStorageRevision = 0;
};

/** 批量删牌确认框所需的只读预览；不是提交授权。 */
struct WACOMRUN_API FRunDeckBatchDeletePreview
{
	FRunDeckBatchOperationValidation Validation;
	int32 TotalGoldReward = 0;
};

/** 原子批量提交结果；失败时 AffectedCount 和 GoldReward 必须为 0。 */
struct WACOMRUN_API FRunDeckBatchOperationResult
{
	bool bSucceeded = false;
	FName DisabledReason = NAME_None;
	int32 AffectedCount = 0;
	int32 GoldReward = 0;
	uint64 StorageRevision = 0;
};
