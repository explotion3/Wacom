// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunStateTypes.h"

/** 背包右侧牌匣和中央工作台共享的区域身份。 */
struct FWacomBackpackZoneKey
{
	EZoneKind Zone = EZoneKind::Backpack;
	FGuid OwnerInstanceId;

	static FWacomBackpackZoneKey Make(EZoneKind InZone, FGuid InOwnerInstanceId = FGuid())
	{
		FWacomBackpackZoneKey Result;
		Result.Zone = InZone;
		Result.OwnerInstanceId = InZone == EZoneKind::SpecialZone ? InOwnerInstanceId : FGuid();
		return Result;
	}

	bool IsValid() const
	{
		return Zone != EZoneKind::SpecialZone || OwnerInstanceId.IsValid();
	}

	friend bool operator==(const FWacomBackpackZoneKey& Left, const FWacomBackpackZoneKey& Right)
	{
		return Left.Zone == Right.Zone && Left.OwnerInstanceId == Right.OwnerInstanceId;
	}

	friend uint32 GetTypeHash(const FWacomBackpackZoneKey& Key)
	{
		return HashCombine(GetTypeHash(static_cast<uint8>(Key.Zone)), GetTypeHash(Key.OwnerInstanceId));
	}
};

/** 当前 Run 内、非 SaveGame 的单卡自由布局记录。 */
struct FWacomBackpackWorkspaceLayoutEntry
{
	FVector2D NormalizedPosition = FVector2D(0.5f, 0.5f);
	float AngleDegrees = 0.0f;
	int32 LayerRank = 0;
	bool bHasManualPlacement = false;
};

enum class EWacomBackpackSelectionMode : uint8
{
	Replace,
	Toggle,
};

/** 活动工作台的一次选择/框选状态。 */
struct FWacomBackpackWorkspaceSelectionState
{
	TArray<FGuid> OrderedSelectedInstanceIds;
	FGuid AnchorInstanceId;
	FVector2D MarqueeStart = FVector2D::ZeroVector;
	FVector2D MarqueeCurrent = FVector2D::ZeroVector;
	EWacomBackpackSelectionMode MarqueeMode = EWacomBackpackSelectionMode::Replace;
	bool bMarqueeActive = false;
};

/** 一次持续扇形携带的全部瞬态状态。 */
struct FWacomBackpackWorkspaceCarryState
{
	TArray<FGuid> RemainingInstanceIds;
	int32 CurrentIndex = INDEX_NONE;
	int32 DefaultIndex = INDEX_NONE;
	FVector2D PointerPosition = FVector2D::ZeroVector;
	bool bInitialReleaseGuardArmed = false;
	bool bMouseCaptured = false;
	FWacomBackpackZoneKey SourceZone;
	uint64 SourceStorageRevision = 0;
	TMap<FGuid, FWacomBackpackWorkspaceLayoutEntry> OriginalLayouts;
};

/** Delete confirm 暂停携带时保存的不可变恢复快照。 */
struct FWacomBackpackPendingDeleteConfirmation
{
	bool bPending = false;
	FWacomBackpackWorkspaceCarryState SuspendedCarry;
	TArray<FGuid> RequestedInstanceIds;
	int32 PreviewCardCount = 0;
	int32 PreviewGoldReward = 0;
};
