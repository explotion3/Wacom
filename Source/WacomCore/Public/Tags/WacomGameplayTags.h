// Copyright Wacom. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

/**
 * Wacom 项目的原生 GameplayTag 声明。
 *
 * 原则：
 * - 所有 tag 必须在此处声明，禁止在业务代码里硬编码字符串。
 * - 新增 tag 同步更新 Docs/WacomData.md。
 * - tag 命名空间层级即代码命名空间层级。
 */
namespace WacomTags
{
	// -------- Card.Keyword --------
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Card_Keyword_Swift);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Card_Keyword_Retain);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Card_Keyword_Combo);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Card_Keyword_Companion);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Card_Keyword_Weapon);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Card_Keyword_Tool);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Card_Keyword_Hand);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Card_Keyword_Exhaust);
	// 历史兼容关键词。当前容量与最后容量来源保护以 CardPhysique.Capacity 为准。
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Card_Keyword_BagProvider);
	// 删牌能力提供者。当前 UI 不依赖该 tag 判定删牌入口；保留给后续规则接入。
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Card_Keyword_DeleteProvider);

	// -------- Card.Rarity --------
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Card_Rarity_White);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Card_Rarity_Blue);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Card_Rarity_Intrinsic);

	// -------- HandZone --------
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(HandZone_Left);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(HandZone_Both);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(HandZone_Right);

	// -------- Effect --------
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Damage);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Heal);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Draw);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Discard);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_ExhaustSelf);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_GainKeyword);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_RemoveStatus);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_ModifyInitiative);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_ApplyStatus_Poison);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_ApplyStatus_Slow);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_ApplyStatus_Freeze);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_ApplyStatus_Twilight);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Shuffle_Random);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Shuffle_FromBothToOther);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Shuffle_ToRandomZone);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Card_AddCost);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Card_ReduceCost);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Card_DiscardSelected);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Card_ExhaustSelected);

	// -------- Magnitude.Source --------
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Magnitude_Source_Literal);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Magnitude_Source_RuntimeCost);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Magnitude_Source_HandCount);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Magnitude_Source_TargetStatusStacks);

	// -------- Condition --------
	// FEffectCondition::ConditionType 的取值。Invalid 视为永真。
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Condition_Self_InZone);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Condition_Target_HasStatus);

	// -------- Status --------
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Poison);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Slow);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Freeze);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Twilight);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Stunned);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Shield);

	// -------- Target --------
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Target_Self);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Target_Player);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Target_SingleEnemyPart);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Target_AllEnemyParts);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Target_RandomHandCard);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Target_ZoneHandCard);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Target_Adjacent_Right);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Target_LastShuffledCard);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Target_SelectedHandCard);

	// -------- Interaction.Target --------
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Interaction_Target_Battle_EnemyPart);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Interaction_Target_Run_Object);

	// -------- ZoneHook --------
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ZoneHook_Trigger_OnPlay);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ZoneHook_Trigger_OnPerfectReleaseHit);

	// -------- Passive.Trigger --------
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Passive_Trigger_AfterPlayed);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Passive_Trigger_OnCompanionCount);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Passive_Trigger_OnTwilightTriggered);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Passive_Trigger_OnTurnStart);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Passive_Trigger_OnTurnEnd);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Passive_Trigger_OnDraw);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Passive_Trigger_OnDiscard);

	// -------- CardLocation（Effect.Draw 的源/目标区域参数）--------
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(CardLocation_Draw);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(CardLocation_Discard);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(CardLocation_Exhaust);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(CardLocation_Hand);

	// -------- SkillSlot（Run 层角色技能池占位 tag）--------
	// 技能列表未正式化前，用 SkillSlot.Placeholder 累计已获得技能数。
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SkillSlot_Placeholder);

	// -------- Card.CapacityEffect（B 类容器卡的容量效果）--------
	// FCardPhysique::CapacityEffect 字段的取值。空 tag = A 类容器卡（无容量效果）。
	// Placeholder：早期骨架占位，未挂任何具体效果。
	// WeaponDamagePlus3：蛛茧绒囊的具体效果。
	// 当带本 tag 的入战 instance 同时具有 Card.Keyword.Weapon 关键词时，
	// 其 Effect.Damage 最终结算 +3。
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Card_CapacityEffect_Placeholder);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Card_CapacityEffect_WeaponDamagePlus3);
}
