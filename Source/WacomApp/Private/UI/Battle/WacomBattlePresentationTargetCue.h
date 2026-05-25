// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Events/BattleEvent.h"

struct FWacomBattlePresentationTargetCue
{
	EBattleEventType SourceEventType = EBattleEventType::None;
	FGuid TargetPartInstanceId;
	int32 Amount = 0;
	float Duration = 0.0f;
};
