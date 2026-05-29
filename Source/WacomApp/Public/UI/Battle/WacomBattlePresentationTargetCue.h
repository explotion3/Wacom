// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Events/BattleEvent.h"

enum class EWacomBattlePresentationTargetCueKind : uint8
{
	BattleEvent,
	TargetConfirmed,
};

inline FName WacomBattlePresentationTargetCueKindToName(EWacomBattlePresentationTargetCueKind CueKind)
{
	switch (CueKind)
	{
	case EWacomBattlePresentationTargetCueKind::TargetConfirmed:
		return TEXT("TargetConfirmed");
	case EWacomBattlePresentationTargetCueKind::BattleEvent:
	default:
		return TEXT("BattleEvent");
	}
}

struct FWacomBattlePresentationTargetCue
{
	EWacomBattlePresentationTargetCueKind CueKind = EWacomBattlePresentationTargetCueKind::BattleEvent;
	EBattleEventType SourceEventType = EBattleEventType::None;
	FGuid TargetPartInstanceId;
	int32 Amount = 0;
	float Duration = 0.0f;
};
