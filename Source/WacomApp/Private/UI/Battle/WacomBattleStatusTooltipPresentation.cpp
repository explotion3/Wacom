// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleStatusTooltipPresentation.h"

#include "Statuses/BattleStatusRuleConstants.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Battle/WacomBattleStatusIconWidget.h"

#define LOCTEXT_NAMESPACE "WacomBattleStatusTooltipPresentation"

namespace
{
	void SetRuleText(
		FWacomBattleStatusIconView& View,
		const FText& CoreEffect,
		const FText& TriggerTiming,
		const FText& StackPolicy)
	{
		View.CoreEffectText = CoreEffect;
		View.TriggerTimingText = TriggerTiming;
		View.StackPolicyText = StackPolicy;
	}

	void PopulatePoison(FWacomBattleStatusIconView& View)
	{
		SetRuleText(
			View,
			FText::Format(
				LOCTEXT("PoisonCore", "每层在结算时造成 {0} 点生命伤害，并穿透护盾。"),
				FText::AsNumber(WacomBattleStatusRuleConstants::PoisonDamagePerStack)),
			LOCTEXT("PoisonTiming", "玩家每打出一张牌或任一敌方部位行动后结算。"),
			View.InspectionHost == EWacomBattleStatusInspectionHost::Player
				? FText::Format(
					LOCTEXT("PlayerPoisonPolicy", "结算不减层；玩家治疗时移除治疗量的 {0}% 层数，向下取整。"),
					FText::AsNumber(FMath::RoundToInt(
						WacomBattleStatusRuleConstants::PlayerHealPoisonRemovalRatio * 100.0f)))
				: LOCTEXT("EnemyPoisonPolicy", "结算不减层；可通过移除状态效果降低层数。"));
	}

	void PopulateSlow(FWacomBattleStatusIconView& View)
	{
		if (View.InspectionHost == EWacomBattleStatusInspectionHost::EnemyPart)
		{
			SetRuleText(
				View,
				LOCTEXT("EnemySlowCore", "施加时按层数延后该部位当前意图的先机。"),
				LOCTEXT("EnemySlowTiming", "效果成功施加时立即结算。"),
				LOCTEXT("EnemySlowPolicy", "不保留为敌方部位的持久状态。"));
			return;
		}

		SetRuleText(
			View,
			LOCTEXT("PlayerSlowCore", "下回合随机若干张手牌获得减速；每层使该卡费用 +1。"),
			LOCTEXT("PlayerSlowTiming", "下回合抽牌并重建手牌后物化到目标卡牌。"),
			LOCTEXT("PlayerSlowPolicy", "卡牌上的减速在该回合结束时清除。"));
	}

	void PopulateFreeze(FWacomBattleStatusIconView& View)
	{
		if (View.InspectionHost == EWacomBattleStatusInspectionHost::EnemyPart)
		{
			SetRuleText(
				View,
				LOCTEXT("EnemyFreezeCore", "每层拦截下一张会真实推进先机的非迅捷卡。"),
				LOCTEXT("EnemyFreezeTiming", "该卡推进对应部位先机时触发。"),
				LOCTEXT("EnemyFreezePolicy", "每次触发消耗 1 层；不会使敌人跳过行动。"));
			return;
		}

		SetRuleText(
			View,
			LOCTEXT("PlayerFreezeCore", "下回合随机若干张手牌被冻结；被冻结卡无法打出。"),
			LOCTEXT("PlayerFreezeTiming", "下回合抽牌并重建手牌后物化到目标卡牌。"),
			LOCTEXT("PlayerFreezePolicy", "打出其相邻卡可全部解除；回合结束仍未解除的冻结会清除。"));
	}

	void PopulateTwilight(FWacomBattleStatusIconView& View)
	{
		if (View.InspectionHost == EWacomBattleStatusInspectionHost::EnemyPart)
		{
			SetRuleText(
				View,
				LOCTEXT("EnemyTwilightCore", "下一意图的先机增加当前暮气层数。"),
				LOCTEXT("EnemyTwilightTiming", "安装下一意图的基础先机后触发。"),
				LOCTEXT("EnemyTwilightPolicy", "触发后层数向下减半；剩余层数继续保留。"));
			return;
		}

		SetRuleText(
			View,
			LOCTEXT("PlayerTwilightCore", "下回合整手牌获得暮气；每层使卡牌费用 +1。"),
			LOCTEXT("PlayerTwilightTiming", "下回合抽牌并重建手牌后物化到当前手牌。"),
			LOCTEXT("PlayerTwilightPolicy", "卡牌成功打出后层数向下减半，并随卡牌跨区域保留。"));
	}

	void PopulateStunned(FWacomBattleStatusIconView& View)
	{
		if (View.InspectionHost == EWacomBattleStatusInspectionHost::EnemyPart)
		{
			SetRuleText(
				View,
				LOCTEXT("EnemyStunnedCore", "使该部位跳过下一次敌方行动。"),
				LOCTEXT("EnemyStunnedTiming", "该部位到达行动边界、准备执行意图时触发。"),
				LOCTEXT("EnemyStunnedPolicy", "每次触发消耗 1 层；叠层可连续跳过多次行动。"));
			return;
		}

		SetRuleText(
			View,
			LOCTEXT("PlayerStunnedCore", "当前没有玩家宿主的行动跳过规则。"),
			LOCTEXT("PlayerStunnedTiming", "仅作为玩家状态层数记录，不会拦截出牌。"),
			LOCTEXT("PlayerStunnedPolicy", "可通过移除状态效果降低层数。"));
	}

	void PopulateUnknown(FWacomBattleStatusIconView& View)
	{
		SetRuleText(
			View,
			LOCTEXT("UnknownCore", "当前状态暂无详细规则说明。"),
			LOCTEXT("UnknownTiming", "具体触发时机由战斗规则决定。"),
			LOCTEXT("UnknownPolicy", "显示层数来自当前战斗快照。"));
	}
}

void FWacomBattleStatusTooltipPresentationBuilder::PopulateRuleText(
	FWacomBattleStatusIconView& InOutView)
{
	if (InOutView.StatusTag == WacomTags::Status_Poison)
	{
		PopulatePoison(InOutView);
	}
	else if (InOutView.StatusTag == WacomTags::Status_Slow)
	{
		PopulateSlow(InOutView);
	}
	else if (InOutView.StatusTag == WacomTags::Status_Freeze)
	{
		PopulateFreeze(InOutView);
	}
	else if (InOutView.StatusTag == WacomTags::Status_Twilight)
	{
		PopulateTwilight(InOutView);
	}
	else if (InOutView.StatusTag == WacomTags::Status_Stunned)
	{
		PopulateStunned(InOutView);
	}
	else
	{
		PopulateUnknown(InOutView);
	}
}

FText FWacomBattleStatusTooltipPresentationBuilder::BuildOverflowBody(
	const TConstArrayView<FWacomBattleStatusIconView> HiddenViews)
{
	TArray<FString> Lines;
	Lines.Reserve(HiddenViews.Num());
	for (const FWacomBattleStatusIconView& View : HiddenViews)
	{
		const FString Name = View.DisplayName.IsEmpty()
			? View.StatusTag.GetTagName().ToString()
			: View.DisplayName.ToString();
		Lines.Add(FString::Printf(
			TEXT("%s ×%d  ·  %s"),
			*Name,
			FMath::Max(1, View.StackCount),
			*View.CoreEffectText.ToString()));
	}
	return FText::FromString(FString::Join(Lines, TEXT("\n")));
}

#undef LOCTEXT_NAMESPACE
