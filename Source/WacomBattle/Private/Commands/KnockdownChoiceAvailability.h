// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Session/BattleSession.h"

struct FBattleState;

/**
 * 击倒事件选项可用性计算。
 *
 * BattleSession 公开 ViewData 与 KnockdownChoiceResolver 规则校验共用本 helper，
 * 避免 UI 看到的可用性和规则层最终拒绝条件不一致。
 */
struct FKnockdownChoiceAvailability
{
	static const FName Reason_None;
	static const FName Reason_NoLivingEnemyPart;
	static const FName Reason_LeftHandMissing;
	static const FName Reason_RightHandMissing;

	static bool HasAnyLivingEnemyPart(const FBattleState& State);
	static FKnockdownChoiceView BuildView(const FBattleState& State);
	static int32 BuildLegacyEventMask(const FKnockdownChoiceView& View);
	static bool IsChoiceAvailable(const FKnockdownChoiceView& View, EKnockdownChoice Choice);
	static FName GetDisabledReason(const FKnockdownChoiceView& View, EKnockdownChoice Choice);
};
