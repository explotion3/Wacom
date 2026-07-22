// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleEventPresentationBuilder.h"

#include "Cards/CardDefinition.h"
#include "Tags/WacomGameplayTags.h"

namespace
{
	FName MakeEventIconKey(EBattleEventType Type)
	{
		switch (Type)
		{
		case EBattleEventType::CardGained:
			return TEXT("CardGained");
		case EBattleEventType::StatusApplied:
			return TEXT("StatusApplied");
		case EBattleEventType::CardStatusChanged:
			return TEXT("CardStatusChanged");
		case EBattleEventType::EnemyInitiativeChanged:
			return TEXT("EnemyInitiativeChanged");
		case EBattleEventType::DamageDealt:
			return TEXT("DamageDealt");
		case EBattleEventType::HandLimitDiscarded:
			return TEXT("HandLimitDiscarded");
		case EBattleEventType::BattleEnded:
			return TEXT("BattleEnded");
		default:
			return *StaticEnum<EBattleEventType>()->GetNameStringByValue(static_cast<int64>(Type));
		}
	}

	EWacomBattleEventVisualTone MakeEventVisualTone(const FBattleEvent& E)
	{
		switch (E.Type)
		{
		case EBattleEventType::CardGained:
		case EBattleEventType::PerfectReleaseResolved:
			return EWacomBattleEventVisualTone::Positive;
		case EBattleEventType::BattleEnded:
			return E.Count == 1
				? EWacomBattleEventVisualTone::Positive
				: EWacomBattleEventVisualTone::Danger;
		case EBattleEventType::HandLimitDiscarded:
		case EBattleEventType::KnockdownChoiceRequested:
			return EWacomBattleEventVisualTone::Warning;
		case EBattleEventType::EnemyPartActed:
			return EWacomBattleEventVisualTone::Danger;
		case EBattleEventType::BattleStarted:
		case EBattleEventType::TurnStarted:
		case EBattleEventType::TurnEnded:
		case EBattleEventType::CardsDrawn:
		case EBattleEventType::CardsRetained:
		case EBattleEventType::EnemyIntentSelected:
		case EBattleEventType::EnemyPhaseChanged:
			return EWacomBattleEventVisualTone::System;
		default:
			return EWacomBattleEventVisualTone::Neutral;
		}
	}

	FString FormatEventMessageForPlayer(const FBattleEvent& E)
	{
		switch (E.Type)
		{
		case EBattleEventType::BattleStarted:
			return TEXT("战斗开始");
		case EBattleEventType::TurnStarted:
			return FString::Printf(TEXT("第 %d 回合开始"), E.Count);
		case EBattleEventType::CardsDrawn:
			return FString::Printf(TEXT("抽取 %d 张牌"), E.Count);
		case EBattleEventType::CardsRetained:
			return FString::Printf(TEXT("保留 %d 张牌"), E.Count);
		case EBattleEventType::CardPlayed:
			return FString::Printf(TEXT("打出卡牌，消耗 %d 先机"), E.Amount);
		case EBattleEventType::InitiativeHit:
			return FString::Printf(TEXT("命中先机：%d"), E.Amount);
		case EBattleEventType::ResistanceResolved:
			return E.bSuccess
				? FString::Printf(TEXT("抵抗成功：眩晕 +1（卡牌单段 %d / 敌方单段 %d）"), E.Amount, E.Count)
				: FString::Printf(TEXT("抵抗失败（卡牌单段 %d / 敌方单段 %d）"), E.Amount, E.Count);
		case EBattleEventType::PerfectReleaseResolved:
			return TEXT("完美释放");
		case EBattleEventType::DamageDealt:
			return E.Tag.IsValid()
				? FString::Printf(TEXT("%s造成 %d 点伤害"), *UWacomBattleEventPresentationBuilder::FormatStatusName(E.Tag), E.Amount)
				: FString::Printf(TEXT("造成 %d 点伤害"), E.Amount);
		case EBattleEventType::StatusApplied:
			return FString::Printf(TEXT("施加%s %d 层"), *UWacomBattleEventPresentationBuilder::FormatStatusName(E.Tag), E.Amount);
		case EBattleEventType::InitiativePushed:
			return FString::Printf(TEXT("敌方先机 -%d"), E.Amount);
		case EBattleEventType::WaitPerformed:
			return FString::Printf(TEXT("等待：敌方先机 -%d"), E.Amount);
		case EBattleEventType::EnemyPartActed:
			return E.Count > 0 ? TEXT("敌方部位行动") : TEXT("敌方部位因眩晕跳过行动");
		case EBattleEventType::EnemyIntentSelected:
		case EBattleEventType::EnemyPhaseChanged:
			return FString();
		case EBattleEventType::EnemyPartHpEmptied:
			return TEXT("敌方部位被击破");
		case EBattleEventType::EnemyKnockdown:
			return TEXT("触发击倒事件");
		case EBattleEventType::KnockdownChoiceRequested:
			return TEXT("选择击倒事件结果");
		case EBattleEventType::KnockdownChoiceMade:
			return FString::Printf(TEXT("击倒选择：%s"), *UWacomBattleEventPresentationBuilder::FormatKnockdownChoice(static_cast<EKnockdownChoice>(E.Count)));
		case EBattleEventType::TurnEnded:
			return FString::Printf(TEXT("第 %d 回合结束"), E.Count);
		case EBattleEventType::PassiveTriggered:
			return TEXT("被动效果触发");
		case EBattleEventType::HandLimitDiscarded:
			return FString::Printf(TEXT("因%s弃置 1 张牌"), *UWacomBattleEventPresentationBuilder::FormatHandLimitDiscardSource(E.HandLimitDiscardSource));
		case EBattleEventType::BattleEnded:
			return E.Count == 1 ? TEXT("战斗胜利") : TEXT("战斗失败");
		case EBattleEventType::HandZoneChanged:
		case EBattleEventType::CardStatusChanged:
		case EBattleEventType::EnemyInitiativeChanged:
		case EBattleEventType::CardDiscarded:
		case EBattleEventType::CardExhausted:
			return FString();  // 太频繁，不弹提示
		case EBattleEventType::CardGained:
			return E.CardDefinition
				? FString::Printf(TEXT("获得卡牌：%s"), *UWacomBattleEventPresentationBuilder::FormatCardName(E.CardDefinition.Get()))
				: TEXT("获得卡牌");
		default:
			return FString();
		}
	}
}

FString UWacomBattleEventPresentationBuilder::FormatCardName(const UCardDefinition* Card)
{
	if (!Card)
	{
		return TEXT("未知卡牌");
	}
	if (!Card->DisplayName.IsEmpty())
	{
		return Card->DisplayName.ToString();
	}
	return GetNameSafe(Card);
}

FString UWacomBattleEventPresentationBuilder::FormatStatusName(FGameplayTag Tag)
{
	if (Tag == WacomTags::Status_Poison || Tag == WacomTags::Effect_ApplyStatus_Poison)
	{
		return TEXT("中毒");
	}
	if (Tag == WacomTags::Status_Slow || Tag == WacomTags::Effect_ApplyStatus_Slow)
	{
		return TEXT("减速");
	}
	if (Tag == WacomTags::Status_Freeze || Tag == WacomTags::Effect_ApplyStatus_Freeze)
	{
		return TEXT("冻结");
	}
	if (Tag == WacomTags::Status_Twilight || Tag == WacomTags::Effect_ApplyStatus_Twilight)
	{
		return TEXT("暮气");
	}
	if (Tag == WacomTags::Status_Stunned)
	{
		return TEXT("眩晕");
	}
	if (Tag == WacomTags::Status_Shield)
	{
		return TEXT("护盾");
	}
	return Tag.IsValid() ? Tag.ToString() : TEXT("状态");
}

FString UWacomBattleEventPresentationBuilder::FormatKnockdownChoice(EKnockdownChoice Choice)
{
	switch (Choice)
	{
	case EKnockdownChoice::Aid:
		return TEXT("援助");
	case EKnockdownChoice::Destroy:
		return TEXT("破坏");
	case EKnockdownChoice::Withdraw:
		return TEXT("撤离");
	default:
		return TEXT("未知选择");
	}
}

FString UWacomBattleEventPresentationBuilder::FormatHandLimitDiscardSource(EHandLimitDiscardSource Source)
{
	switch (Source)
	{
	case EHandLimitDiscardSource::TurnStart:
		return TEXT("回合开始抽牌");
	case EHandLimitDiscardSource::EffectDraw:
		return TEXT("抽牌效果");
	case EHandLimitDiscardSource::PassiveOnCompanionCount:
		return TEXT("伙伴被动");
	default:
		return TEXT("手牌上限");
	}
}

FString UWacomBattleEventPresentationBuilder::FormatEventForPlayer(const FBattleEvent& E)
{
	return BuildEventPresentationView(E).MessageText.ToString();
}

FBattleEventPresentationView UWacomBattleEventPresentationBuilder::BuildEventPresentationView(const FBattleEvent& E)
{
	FBattleEventPresentationView View;
	View.EventType = E.Type;
	View.MessageText = FText::FromString(FormatEventMessageForPlayer(E));
	View.bShouldDisplay = !View.MessageText.IsEmpty();
	View.VisualTone = MakeEventVisualTone(E);
	View.IconKey = View.bShouldDisplay ? MakeEventIconKey(E.Type) : NAME_None;
	return View;
}
