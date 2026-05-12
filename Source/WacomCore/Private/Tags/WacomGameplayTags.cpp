// Copyright Wacom. All Rights Reserved.

#include "Tags/WacomGameplayTags.h"

namespace WacomTags
{
	// -------- Card.Keyword --------
	UE_DEFINE_GAMEPLAY_TAG(Card_Keyword_Swift,     "Card.Keyword.Swift");
	UE_DEFINE_GAMEPLAY_TAG(Card_Keyword_Retain,    "Card.Keyword.Retain");
	UE_DEFINE_GAMEPLAY_TAG(Card_Keyword_Combo,     "Card.Keyword.Combo");
	UE_DEFINE_GAMEPLAY_TAG(Card_Keyword_Companion, "Card.Keyword.Companion");
	UE_DEFINE_GAMEPLAY_TAG(Card_Keyword_Weapon,    "Card.Keyword.Weapon");
	UE_DEFINE_GAMEPLAY_TAG(Card_Keyword_Tool,      "Card.Keyword.Tool");
	UE_DEFINE_GAMEPLAY_TAG(Card_Keyword_Hand,      "Card.Keyword.Hand");
	UE_DEFINE_GAMEPLAY_TAG(Card_Keyword_Exhaust,   "Card.Keyword.Exhaust");

	// -------- Card.Rarity --------
	UE_DEFINE_GAMEPLAY_TAG(Card_Rarity_White,     "Card.Rarity.White");
	UE_DEFINE_GAMEPLAY_TAG(Card_Rarity_Blue,      "Card.Rarity.Blue");
	UE_DEFINE_GAMEPLAY_TAG(Card_Rarity_Intrinsic, "Card.Rarity.Intrinsic");

	// -------- HandZone --------
	UE_DEFINE_GAMEPLAY_TAG(HandZone_Left,  "HandZone.Left");
	UE_DEFINE_GAMEPLAY_TAG(HandZone_Both,  "HandZone.Both");
	UE_DEFINE_GAMEPLAY_TAG(HandZone_Right, "HandZone.Right");

	// -------- Effect --------
	UE_DEFINE_GAMEPLAY_TAG(Effect_Damage,                    "Effect.Damage");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Heal,                      "Effect.Heal");
	UE_DEFINE_GAMEPLAY_TAG(Effect_ApplyStatus_Poison,        "Effect.ApplyStatus.Poison");
	UE_DEFINE_GAMEPLAY_TAG(Effect_ApplyStatus_Slow,          "Effect.ApplyStatus.Slow");
	UE_DEFINE_GAMEPLAY_TAG(Effect_ApplyStatus_Freeze,        "Effect.ApplyStatus.Freeze");
	UE_DEFINE_GAMEPLAY_TAG(Effect_ApplyStatus_Twilight,      "Effect.ApplyStatus.Twilight");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Shuffle_Random,            "Effect.Shuffle.Random");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Shuffle_FromBothToOther,   "Effect.Shuffle.FromBothToOther");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Shuffle_ToRandomZone,      "Effect.Shuffle.ToRandomZone");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Card_AddCost,              "Effect.Card.AddCost");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Card_ReduceCost,           "Effect.Card.ReduceCost");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Draw,                      "Effect.Draw");
	UE_DEFINE_GAMEPLAY_TAG(Effect_Discard,                   "Effect.Discard");
	UE_DEFINE_GAMEPLAY_TAG(Effect_ExhaustSelf,               "Effect.ExhaustSelf");
	UE_DEFINE_GAMEPLAY_TAG(Effect_GainKeyword,              "Effect.GainKeyword");
	UE_DEFINE_GAMEPLAY_TAG(Effect_RemoveStatus,             "Effect.RemoveStatus");
	UE_DEFINE_GAMEPLAY_TAG(Effect_ModifyInitiative,         "Effect.ModifyInitiative");

	// -------- Magnitude.Source --------
	UE_DEFINE_GAMEPLAY_TAG(Magnitude_Source_Literal,         "Magnitude.Source.Literal");
	UE_DEFINE_GAMEPLAY_TAG(Magnitude_Source_RuntimeCost,     "Magnitude.Source.RuntimeCost");
	UE_DEFINE_GAMEPLAY_TAG(Magnitude_Source_HandCount,       "Magnitude.Source.HandCount");
	UE_DEFINE_GAMEPLAY_TAG(Magnitude_Source_TargetStatusStacks, "Magnitude.Source.TargetStatusStacks");

	// -------- Condition --------
	UE_DEFINE_GAMEPLAY_TAG(Condition_Self_InZone,            "Condition.Self.InZone");
	UE_DEFINE_GAMEPLAY_TAG(Condition_Target_HasStatus,       "Condition.Target.HasStatus");

	// -------- Status --------
	UE_DEFINE_GAMEPLAY_TAG(Status_Poison,   "Status.Poison");
	UE_DEFINE_GAMEPLAY_TAG(Status_Slow,     "Status.Slow");
	UE_DEFINE_GAMEPLAY_TAG(Status_Freeze,   "Status.Freeze");
	UE_DEFINE_GAMEPLAY_TAG(Status_Twilight, "Status.Twilight");
	UE_DEFINE_GAMEPLAY_TAG(Status_Stunned,  "Status.Stunned");
	UE_DEFINE_GAMEPLAY_TAG(Status_Shield,   "Status.Shield");

	// -------- Target --------
	UE_DEFINE_GAMEPLAY_TAG(Target_Self,             "Target.Self");
	UE_DEFINE_GAMEPLAY_TAG(Target_Player,           "Target.Player");
	UE_DEFINE_GAMEPLAY_TAG(Target_SingleEnemyPart,  "Target.SingleEnemyPart");
	UE_DEFINE_GAMEPLAY_TAG(Target_AllEnemyParts,    "Target.AllEnemyParts");
	UE_DEFINE_GAMEPLAY_TAG(Target_RandomHandCard,   "Target.RandomHandCard");
	UE_DEFINE_GAMEPLAY_TAG(Target_ZoneHandCard,     "Target.ZoneHandCard");
	UE_DEFINE_GAMEPLAY_TAG(Target_Adjacent_Right,   "Target.Adjacent.Right");
	UE_DEFINE_GAMEPLAY_TAG(Target_LastShuffledCard, "Target.LastShuffledCard");

	// -------- ZoneHook --------
	UE_DEFINE_GAMEPLAY_TAG(ZoneHook_Trigger_OnPlay,               "ZoneHook.Trigger.OnPlay");
	UE_DEFINE_GAMEPLAY_TAG(ZoneHook_Trigger_OnPerfectReleaseHit,  "ZoneHook.Trigger.OnPerfectReleaseHit");

	// -------- Passive.Trigger --------
	UE_DEFINE_GAMEPLAY_TAG(Passive_Trigger_AfterPlayed,         "Passive.Trigger.AfterPlayed");
	UE_DEFINE_GAMEPLAY_TAG(Passive_Trigger_OnCompanionCount,    "Passive.Trigger.OnCompanionCount");
	UE_DEFINE_GAMEPLAY_TAG(Passive_Trigger_OnTwilightTriggered, "Passive.Trigger.OnTwilightTriggered");
	UE_DEFINE_GAMEPLAY_TAG(Passive_Trigger_OnTurnStart,          "Passive.Trigger.OnTurnStart");
	UE_DEFINE_GAMEPLAY_TAG(Passive_Trigger_OnTurnEnd,            "Passive.Trigger.OnTurnEnd");
	UE_DEFINE_GAMEPLAY_TAG(Passive_Trigger_OnDraw,               "Passive.Trigger.OnDraw");
	UE_DEFINE_GAMEPLAY_TAG(Passive_Trigger_OnDiscard,            "Passive.Trigger.OnDiscard");

	// -------- CardLocation --------
	UE_DEFINE_GAMEPLAY_TAG(CardLocation_Draw,       "CardLocation.Draw");
	UE_DEFINE_GAMEPLAY_TAG(CardLocation_Discard,    "CardLocation.Discard");
	UE_DEFINE_GAMEPLAY_TAG(CardLocation_Exhaust,    "CardLocation.Exhaust");
	UE_DEFINE_GAMEPLAY_TAG(CardLocation_Hand,       "CardLocation.Hand");
}
