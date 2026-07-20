// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunStateTypes.h"

/** 背包投放目标的纯表现状态；不参与 Run 命令和合法性判定。 */
enum class EWacomBackpackDropFeedbackState : uint8
{
	None,
	Valid,
	Rejected,
	Destructive
};

/** Screen 已完成规则预览后交给被动目标 Widget 的表现数据。 */
struct WACOMAPP_API FWacomBackpackDropFeedbackView
{
	EWacomBackpackDropFeedbackState State = EWacomBackpackDropFeedbackState::None;
	FText Message;
	int32 CurrentCount = 0;
	int32 IncomingCount = 0;
	int32 Capacity = 0;
	bool bHasCapacity = false;

	bool IsVisible() const
	{
		return State != EWacomBackpackDropFeedbackState::None;
	}

	bool IsRejected() const
	{
		return State == EWacomBackpackDropFeedbackState::Rejected;
	}
};

/** 工作台内嵌区域牌堆的被动 Scene ViewData。完整卡牌由 Workspace 统一持有。 */
struct WACOMAPP_API FWacomBackpackZonePileView
{
	EZoneKind Zone = EZoneKind::BattleDeck;
	FGuid OwnerInstanceId;
	FText Title;
	int32 CardCount = 0;
	int32 Capacity = 0;
	int32 ProjectedCount = 0;
	bool bHasCapacity = false;
	bool bMovable = true;
	bool bWarning = false;
	bool bExpanded = false;

	bool HasSameIdentity(EZoneKind OtherZone, FGuid OtherOwnerInstanceId) const
	{
		const FGuid NormalizedOwner = OtherZone == EZoneKind::SpecialZone
			? OtherOwnerInstanceId
			: FGuid();
		return Zone == OtherZone && OwnerInstanceId == NormalizedOwner;
	}
};
