// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Exploration/RunExplorationTypes.h"
#include "Types/WacomResult.h"

/** C++-only 探索命令结果；不存在到 FWacomStatus 的隐式转换。 */
struct WACOMRUN_API FRunExplorationResolution
{
	FWacomStatus Status;
	int32 VersionBefore = 0;
	int32 VersionAfter = 0;
	TArray<FRunExplorationEvent> Events;
	FRunExplorationSnapshot PostSnapshot;
	TOptional<FRunTraversalTicket> TraversalTicket;
	TOptional<FRunNodeActivityTicket> NodeActivityTicket;
	TOptional<FRunCampTicket> CampTicket;
	TOptional<FRunFloorTransitionConfirmation> FloorTransitionConfirmation;

	bool IsOk() const { return Status.IsOk(); }
};

/** 新 Run 初始化的正式参数。 */
struct WACOMRUN_API FRunInitializationParams
{
	class UCharacterDefinition* Character = nullptr;
	class UWacomJourneyDefinition* Journey = nullptr;
};

/** 初始化显式结果；失败时 Events 为空且 PostSnapshot 是 Session 原状态。 */
struct WACOMRUN_API FRunInitializationResult
{
	FWacomStatus Status;
	TArray<FRunExplorationEvent> Events;
	FRunExplorationSnapshot PostSnapshot;

	bool IsOk() const { return Status.IsOk(); }
};
