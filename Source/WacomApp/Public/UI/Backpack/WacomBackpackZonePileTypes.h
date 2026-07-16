// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunStateTypes.h"

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
