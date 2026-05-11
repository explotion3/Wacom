// Copyright Wacom. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

/**
 * Wacom 项目的原生 GameplayTag 声明。
 *
 * 对齐 Data_Schema_Draft.md §2。
 *
 * 原则：
 * - 所有 tag 必须在此处声明，禁止在业务代码里硬编码字符串。
 * - 新增 tag 同步更新 Data_Schema_Draft.md §2。
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
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_ApplyStatus_Poison);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_ApplyStatus_Slow);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_ApplyStatus_Freeze);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_ApplyStatus_Twilight);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Shuffle_Random);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Shuffle_FromBothToOther);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Shuffle_ToRandomZone);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Card_AddCost);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Card_ReduceCost);

	// -------- Magnitude.Source --------
	// FCardEffect::MagnitudeSource 决定 FinalMagnitude 怎么算。
	// 未设置（invalid tag）时默认 = Literal，即直接用 FCardEffect::Magnitude 字段。
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Magnitude_Source_Literal);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Magnitude_Source_RuntimeCost);

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

	// -------- ZoneHook --------
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ZoneHook_Trigger_OnPlay);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ZoneHook_Trigger_OnPerfectReleaseHit);

	// -------- Passive.Trigger --------
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Passive_Trigger_AfterPlayed);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Passive_Trigger_OnCompanionCount);
	WACOMCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Passive_Trigger_OnTwilightTriggered);
}
