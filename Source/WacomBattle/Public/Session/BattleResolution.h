// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Events/BattleEvent.h"
#include "Presentation/BattlePresentationJournal.h"
#include "Snapshots/BattleSnapshot.h"
#include "Types/WacomResult.h"

/**
 * 一条战斗命令的原子输出。
 *
 * Events、PresentationJournal 与 PostSnapshot 必定来自同一次成功 commit。
 * 失败时 VersionAfter == VersionBefore，Events/Journal 为空，PostSnapshot 是未改变的当前快照。
 */
struct WACOMBATTLE_API FBattleResolution
{
	FWacomStatus Status;
	int32 VersionBefore = INDEX_NONE;
	int32 VersionAfter = INDEX_NONE;
	TArray<FBattleEvent> Events;
	FBattlePresentationJournal PresentationJournal;
	FBattleSnapshot PostSnapshot;

	bool IsOk() const { return Status.IsOk(); }
	operator const FWacomStatus&() const { return Status; }
};
