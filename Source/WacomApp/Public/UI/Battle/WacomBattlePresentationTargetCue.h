// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Events/BattleEvent.h"
#include "Runtime/BattlePartSlotIdentity.h"
#include "WacomBattlePresentationTargetCue.generated.h"

/** 战斗表现目标 cue 的类型。 */
UENUM()
enum class EWacomBattlePresentationTargetCueKind : uint8
{
	BattleEvent,
	DamageDealt,
	EnemyPartHpEmptied,
	TargetConfirmed,
};

/** 战斗表现目标 cue。BattleHUD 的事件表现队列通过本结构向注册的 2D/3D 表现目标分发反馈。 */
USTRUCT()
struct WACOMAPP_API FWacomBattlePresentationTargetCue
{
	GENERATED_BODY()

	UPROPERTY()
	EWacomBattlePresentationTargetCueKind CueKind = EWacomBattlePresentationTargetCueKind::BattleEvent;

	UPROPERTY()
	FBattlePartSlotIdentity TargetPartKey;

	UPROPERTY()
	EBattleEventType SourceEventType = EBattleEventType::None;

	UPROPERTY()
	int32 Amount = 0;

	UPROPERTY()
	float Duration = 0.0f;

	/** 只用于表现随机的稳定种子；不得作为规则 RNG 或目标身份。 */
	UPROPERTY()
	int32 Seed = 0;
};

inline FName WacomBattlePresentationTargetCueKindToName(EWacomBattlePresentationTargetCueKind Kind)
{
	switch (Kind)
	{
	case EWacomBattlePresentationTargetCueKind::BattleEvent:
		return TEXT("BattleEvent");
	case EWacomBattlePresentationTargetCueKind::DamageDealt:
		return TEXT("DamageDealt");
	case EWacomBattlePresentationTargetCueKind::EnemyPartHpEmptied:
		return TEXT("EnemyPartHpEmptied");
	case EWacomBattlePresentationTargetCueKind::TargetConfirmed:
		return TEXT("TargetConfirmed");
	default:
		return TEXT("Unknown");
	}
}
