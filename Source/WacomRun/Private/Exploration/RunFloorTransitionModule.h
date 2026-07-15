// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Exploration/RunExplorationTypes.h"
#include "Types/WacomResult.h"

struct FRunState;

/** Floor Entrance 预览、确认 token 与不可逆跨层提交的私有深层模块。 */
class FRunFloorTransitionModule
{
public:
	static bool BuildCurrentPreview(
		const FRunState& State,
		FRunFloorTransitionPreview& OutPreview);

	static FWacomStatus Request(
		FRunState& State,
		TOptional<FRunFloorTransitionConfirmation>& PendingConfirmation,
		FRunFloorTransitionConfirmation& OutConfirmation,
		TArray<FRunExplorationEvent>& OutEvents);

	static FWacomStatus Confirm(
		FRunState& State,
		TOptional<FRunFloorTransitionConfirmation>& PendingConfirmation,
		const FRunFloorTransitionConfirmation& Confirmation,
		TArray<FRunExplorationEvent>& OutEvents);

	static FWacomStatus Cancel(
		FRunState& State,
		TOptional<FRunFloorTransitionConfirmation>& PendingConfirmation,
		const FRunFloorTransitionConfirmation& Confirmation,
		TArray<FRunExplorationEvent>& OutEvents);

	static bool Matches(
		const FRunState& State,
		const TOptional<FRunFloorTransitionConfirmation>& PendingConfirmation,
		const FRunFloorTransitionConfirmation& Confirmation);
};
