// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Resolution/BattleTargetValidationResult.h"

struct FBattleState;
struct FWacomInteractionTargetHandle;

/**
 * 统一的战斗目标合法性判断。
 *
 * 只判断"结构性兼容"（这张卡能不能指向这个类型的目标），
 * 不校验费用、晕厥、冻结、手牌位置等运行时规则——那些是 PlayCardResolver::Resolve 的事。
 *
 * 调用方（BattleHUD、拖拽系统、测试）通过 UBattleSession::CanTargetWithCard() 入口调用，
 * 不要直接引用私有 FBattleState。
 */
struct FBattleTargetResolver
{
	static FWacomBattleTargetValidationResult ValidateTargetWithCard(const FBattleState& State, const FGuid& CardInstanceId,
		const FWacomInteractionTargetHandle& Target);

	/**
	 * 给定的卡牌实例能否作用到给定的交互目标。
	 *
	 * @return true 表示"规则允许以这个目标打出这张牌"，提交 PlayCard 后可能被 Resolver 进一步拒绝。
	 */
	static bool CanTargetWithCard(const FBattleState& State, const FGuid& CardInstanceId,
		const FWacomInteractionTargetHandle& Target);
};
