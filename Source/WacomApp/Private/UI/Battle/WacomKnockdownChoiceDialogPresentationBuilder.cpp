// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomKnockdownChoiceDialogPresentationBuilder.h"

#include "Cards/CardDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/Card/WacomCardPresentationBuilder.h"

#define LOCTEXT_NAMESPACE "WacomKnockdownChoicePresentation"

namespace
{
	const FEnemyPartSnapshot* FindPartSnapshot(
		const FBattleSnapshot& Snapshot,
		const FGuid& PartInstanceId)
	{
		if (!PartInstanceId.IsValid())
		{
			return nullptr;
		}
		for (const FEnemySnapshot& Enemy : Snapshot.Enemies)
		{
			if (const FEnemyPartSnapshot* Part = Enemy.Parts.FindByPredicate(
				[&PartInstanceId](const FEnemyPartSnapshot& Candidate)
				{
					return Candidate.InstanceId == PartInstanceId;
				}))
			{
				return Part;
			}
		}
		return nullptr;
	}

	FText BuildDisabledReasonText(
		const FKnockdownChoiceOptionView& RuleOption)
	{
		if (RuleOption.bAvailable || RuleOption.DisabledReason.IsNone())
		{
			return FText::GetEmpty();
		}
		if (RuleOption.DisabledReason == TEXT("NoLivingEnemyPart"))
		{
			return LOCTEXT("NoLivingEnemyPart", "敌人已无存活部位，无法撤离");
		}
		if (RuleOption.DisabledReason == TEXT("LeftHandMissing"))
		{
			return LOCTEXT("LeftHandMissing", "左手已永久失去，无法援助");
		}
		if (RuleOption.DisabledReason == TEXT("RightHandMissing"))
		{
			return LOCTEXT("RightHandMissing", "右手已永久失去，无法破坏");
		}
		return LOCTEXT("ChoiceUnavailable", "当前无法选择");
	}

	FText BuildRewardFallbackText(
		const FKnockdownChoiceOptionView& RuleOption)
	{
		if (!RuleOption.bHasRewardCard)
		{
			return LOCTEXT("NoCardReward", "无卡牌奖励");
		}

		const FText RewardName = !RuleOption.RewardCardName.IsEmpty()
			? RuleOption.RewardCardName
			: (RuleOption.RewardCardId.IsNone()
				? FText::GetEmpty()
				: FText::FromName(RuleOption.RewardCardId));
		return RewardName.IsEmpty()
			? LOCTEXT("RewardUnavailable", "奖励卡面暂不可用")
			: FText::Format(LOCTEXT("RewardNameFormat", "奖励：{0}"), RewardName);
	}

	FWacomKnockdownChoiceOptionViewData BuildOption(
		const FKnockdownChoiceOptionView& RuleOption,
		const UEnemyPartDefinition* PartDefinition)
	{
		FWacomKnockdownChoiceOptionViewData Result;
		Result.Choice = RuleOption.Choice;
		Result.bAvailable = RuleOption.bAvailable;
		Result.DisabledReasonText = BuildDisabledReasonText(RuleOption);
		Result.bHasRewardCard = RuleOption.bHasRewardCard;
		Result.RewardFallbackText = BuildRewardFallbackText(RuleOption);

		switch (RuleOption.Choice)
		{
		case EKnockdownChoice::Aid:
			Result.BranchLabel = LOCTEXT("AidBranch", "左手");
			Result.ChoiceLabel = LOCTEXT("AidChoice", "援助");
			Result.DescriptionText = LOCTEXT(
				"AidDescription",
				"接纳此部位的援助奖励\n不消耗左手牌");
			break;
		case EKnockdownChoice::Withdraw:
			Result.BranchLabel = LOCTEXT("WithdrawBranch", "撤离");
			Result.ChoiceLabel = LOCTEXT("WithdrawChoice", "结束战斗");
			Result.DescriptionText = LOCTEXT(
				"WithdrawDescription",
				"放弃本次卡牌奖励并立即撤离");
			break;
		case EKnockdownChoice::Destroy:
			Result.BranchLabel = LOCTEXT("DestroyBranch", "右手");
			Result.ChoiceLabel = LOCTEXT("DestroyChoice", "破坏");
			Result.DescriptionText = LOCTEXT(
				"DestroyDescription",
				"夺取此部位的破坏奖励\n不消耗右手牌");
			break;
		case EKnockdownChoice::None:
		default:
			break;
		}

		if (!RuleOption.bHasRewardCard
			|| RuleOption.Choice == EKnockdownChoice::Withdraw)
		{
			return Result;
		}

		if (!PartDefinition)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[KnockdownChoiceUI] Reward card fallback: missing part definition Choice=%d RuleCardId=%s"),
				static_cast<int32>(RuleOption.Choice),
				*RuleOption.RewardCardId.ToString());
			return Result;
		}

		const UCardDefinition* RewardCard =
			PartDefinition->ResolveKnockdownRewardCard(RuleOption.Choice);
		if (!RewardCard)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[KnockdownChoiceUI] Reward card fallback: definition has no reward Choice=%d Part=%s RuleCardId=%s"),
				static_cast<int32>(RuleOption.Choice),
				*PartDefinition->PartId.ToString(),
				*RuleOption.RewardCardId.ToString());
			return Result;
		}

		if (RewardCard->CardId != RuleOption.RewardCardId)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[KnockdownChoiceUI] Reward card fallback: CardId mismatch Choice=%d Part=%s Rule=%s Definition=%s"),
				static_cast<int32>(RuleOption.Choice),
				*PartDefinition->PartId.ToString(),
				*RuleOption.RewardCardId.ToString(),
				*RewardCard->CardId.ToString());
			return Result;
		}

		Result.RewardCardViewData =
			UWacomCardPresentationBuilder::BuildCardViewData(RewardCard);
		Result.bHasRewardCardView = true;
		return Result;
	}
}

FWacomKnockdownChoiceDialogViewData
FWacomKnockdownChoiceDialogPresentationBuilder::Build(
	const FKnockdownChoiceView& ChoiceView,
	const FBattleSnapshot& Snapshot)
{
	FWacomKnockdownChoiceDialogViewData Result;
	Result.TitleText = LOCTEXT("DialogTitle", "选择击倒结果");
	Result.PartNameText = ChoiceView.PartName.IsEmpty()
		? LOCTEXT("UnknownPart", "已击倒部位")
		: FText::Format(LOCTEXT("PartNameFormat", "已击倒：{0}"), ChoiceView.PartName);

	const FEnemyPartSnapshot* PartSnapshot =
		FindPartSnapshot(Snapshot, ChoiceView.PartInstanceId);
	const UEnemyPartDefinition* PartDefinition =
		PartSnapshot ? PartSnapshot->Definition.Get() : nullptr;
	if (!PartSnapshot)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[KnockdownChoiceUI] PartInstanceId not found in Snapshot; rewards use text fallback PartInstanceId=%s PartId=%s"),
			*ChoiceView.PartInstanceId.ToString(),
			*ChoiceView.PartId.ToString());
	}

	Result.AidOption = BuildOption(ChoiceView.AidOption, PartDefinition);
	Result.WithdrawOption = BuildOption(ChoiceView.WithdrawOption, PartDefinition);
	Result.DestroyOption = BuildOption(ChoiceView.DestroyOption, PartDefinition);
	return Result;
}

#undef LOCTEXT_NAMESPACE
