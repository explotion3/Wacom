// Copyright Wacom. All Rights Reserved.

#include "Commands/KnockdownChoiceResolver.h"
#include "Commands/KnockdownChoiceAvailability.h"
#include "Commands/KnockdownFlowService.h"
#include "Core/BattleState.h"
#include "Core/BattleRules.h"
#include "Commands/BattleCommand.h"
#include "Events/BattleEventBus.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Presentation/BattlePresentationJournal.h"
#include "Rewards/BattleCardGrantService.h"
#include "Runtime/RuntimeEnemyPart.h"

FWacomStatus FKnockdownChoiceResolver::Resolve(
	FBattleState& State,
	FBattleEventBus& Events,
	FBattlePresentationJournal& PresentationJournal,
	const FBattleCommand& Command)
{
	if (State.PendingKnockdownEvents.Num() <= 0)
	{
		return FWacomStatus::Fail(EWacomError::InvalidState, TEXT("NoPendingKnockdown"));
	}

	const EKnockdownChoice Choice = Command.KnockdownChoiceValue;
	if (Choice == EKnockdownChoice::None)
	{
		return FWacomStatus::Fail(EWacomError::InvalidArgument, TEXT("NoneChoice"));
	}

	// 取队头
	const FBattleState::FPendingKnockdownEvent Head = State.PendingKnockdownEvents[0];
	const FKnockdownChoiceView ChoiceView = FKnockdownChoiceAvailability::BuildView(State);

	// 可选性校验
	if (!FKnockdownChoiceAvailability::IsChoiceAvailable(ChoiceView, Choice))
	{
		return FWacomStatus::Fail(
			EWacomError::InvalidArgument,
			FKnockdownChoiceAvailability::GetDisabledReason(ChoiceView, Choice));
	}

	// dequeue + 记账
	State.PendingKnockdownEvents.RemoveAt(0);
	{
		FKnockdownChoice Choice_;
		Choice_.PartId = Head.PartId;
		Choice_.Identity = Head.Identity;
		Choice_.PartKey = Head.Identity.ToEnemyPartKey();
		Choice_.Choice = Choice;
		State.PendingKnockdownChoices.Add(Choice_);
	}

	// 发事件让 UI 能反应
	{
		FBattleEvent Ev;
		Ev.Type            = EBattleEventType::KnockdownChoiceMade;
		Ev.ActorInstanceId = Head.PartInstanceId;
		Ev.ActorEnemyPartKey = Head.Identity.ToEnemyPartKey();
		Ev.Count           = static_cast<int32>(Choice);
		Events.Emit(Ev);
	}

	// 撤离：结束战斗。Outcome = Victory（packet 用 bWithdrawn 标记区分真胜利）
	if (Choice == EKnockdownChoice::Withdraw)
	{
		State.Outcome  = EBattleOutcome::Victory;
		State.Phase    = EBattlePhase::BattleEnd;

		FBattleEvent Ended;
		Ended.Type  = EBattleEventType::BattleEnded;
		Ended.Count = 1; // 1 = 胜利（按现有惯例，packet 区分撤离/真胜利）
		Events.Emit(Ended);

		return FWacomStatus::Ok();
	}

	// 援助 / 破坏：从统一分支查询取得奖励，立即获得战斗内卡并记入战后包。
	if (Choice == EKnockdownChoice::Aid || Choice == EKnockdownChoice::Destroy)
	{
		const FRuntimeEnemyPart* SourcePart = State.Enemy.Parts.FindByPredicate(
			[&Head](const FRuntimeEnemyPart& Part)
			{
				return Part.InstanceId == Head.PartInstanceId;
			});
		UCardDefinition* RewardCard = (SourcePart && SourcePart->Definition)
			? SourcePart->Definition->ResolveKnockdownRewardCard(Choice)
			: nullptr;

		if (RewardCard)
		{
			FBattleCardGrantService::GrantCardToHand(
				State,
				Events,
				PresentationJournal,
				RewardCard,
				Head.PartInstanceId,
				Head.Identity.ToEnemyPartKey(),
				Choice);

			FBattleGainedCard GainedCard;
			GainedCard.Definition = RewardCard;
			GainedCard.SourcePartId = Head.PartId;
			GainedCard.SourceIdentity = Head.Identity;
			GainedCard.SourcePartKey = Head.Identity.ToEnemyPartKey();
			GainedCard.SourceChoice = Choice;
			State.PendingGainedCards.Add(GainedCard);
		}
	}

	// 队列仍非空 → 继续等下一条；队列空 → 回 PlayerAction
	if (State.PendingKnockdownEvents.Num() == 0)
	{
		// 队列清空后再判终局：可能本场所有部位都破坏了，
		// 之前 CheckAndApplyBattleEnd 因为队列非空被跳过；现在该判 Victory。
		State.Phase = EBattlePhase::PlayerAction;
		FBattleRules::CheckAndApplyBattleEnd(State, Events);
	}
	else
	{
		// 队列还有未处理的击倒事件 → 维持 PendingKnockdownChoice 阶段
		// 同时主动发新一条 KnockdownChoiceRequested，让 UI push 下一个 dialog。
		// （命令 pipeline 的事件分派只在"首次入队"时触发；后续连续选项必须由 Resolver 自己推送。）
		FKnockdownFlowService::RequestCurrentChoiceIfPending(State, Events);
	}
	return FWacomStatus::Ok();
}
