// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleCombatLogBuilder.h"

#include "Cards/CardDefinition.h"
#include "Enemies/EnemyPartDefinition.h"

#define LOCTEXT_NAMESPACE "WacomBattleCombatLogBuilder"

namespace
{
	FText MakeNameTextOrFallback(const FText& Text, const FString& Fallback)
	{
		return Text.IsEmpty() ? FText::FromString(Fallback) : Text;
	}

	FText MakeCardName(const FHandCardSnapshot* Card)
	{
		if (!Card || !Card->Definition)
		{
			return LOCTEXT("UnknownCard", "未知卡牌");
		}
		return FText::FromString(UWacomBattleEventPresentationBuilder::FormatCardName(Card->Definition.Get()));
	}

	FText MakePartName(const FEnemyPartSnapshot* Part)
	{
		if (!Part || !Part->Definition)
		{
			return LOCTEXT("UnknownTarget", "未知目标");
		}
		return MakeNameTextOrFallback(Part->Definition->DisplayName, Part->Definition->PartId.ToString());
	}

	const FHandCardSnapshot* FindHandCard(const FBattleSnapshot& Snapshot, const FGuid& CardInstanceId)
	{
		if (!CardInstanceId.IsValid())
		{
			return nullptr;
		}

		for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
		{
			if (Card.InstanceId == CardInstanceId)
			{
				return &Card;
			}
		}
		return nullptr;
	}

const FEnemyPartSnapshot* FindEnemyPart(const FBattleSnapshot& Snapshot, const FBattlePartSlotIdentity& PartKey)
{
	if (!PartKey.IsValidSlot())
	{
		return nullptr;
	}

		for (const FEnemySnapshot& Enemy : Snapshot.Enemies)
		{
			for (const FEnemyPartSnapshot& Part : Enemy.Parts)
			{
			if (Part.Identity == PartKey)
			{
				return &Part;
			}
			}
		}
	return nullptr;
}

EWacomBattleEventVisualTone ToneForCommand(EWacomBattleCombatLogCommandKind CommandKind)
	{
		switch (CommandKind)
		{
		case EWacomBattleCombatLogCommandKind::System:
		case EWacomBattleCombatLogCommandKind::EndTurn:
			return EWacomBattleEventVisualTone::System;
		case EWacomBattleCombatLogCommandKind::KnockdownChoice:
			return EWacomBattleEventVisualTone::Warning;
		default:
			return EWacomBattleEventVisualTone::Neutral;
		}
	}

	FName IconForCommand(EWacomBattleCombatLogCommandKind CommandKind)
	{
		switch (CommandKind)
		{
		case EWacomBattleCombatLogCommandKind::System:
			return TEXT("System");
		case EWacomBattleCombatLogCommandKind::PlayCard:
			return TEXT("PlayCard");
		case EWacomBattleCombatLogCommandKind::Wait:
			return TEXT("Wait");
		case EWacomBattleCombatLogCommandKind::EndTurn:
			return TEXT("EndTurn");
		case EWacomBattleCombatLogCommandKind::KnockdownChoice:
			return TEXT("KnockdownChoice");
		default:
			return TEXT("CombatLog");
		}
	}

	bool ShouldShowCombatLogDetailForEvent(EBattleEventType Type)
	{
		switch (Type)
		{
		case EBattleEventType::HandZoneChanged:
		case EBattleEventType::CardPlayed:
			return false;
		default:
			return true;
		}
	}

	FString FormatCardZoneMoveReason(EHandCardZoneMoveReason Reason)
	{
		switch (Reason)
		{
		case EHandCardZoneMoveReason::Effect:
			return TEXT("效果");
		case EHandCardZoneMoveReason::HandLimit:
			return TEXT("手牌上限");
		case EHandCardZoneMoveReason::TurnEnd:
			return TEXT("回合结束");
		default:
			return TEXT("规则");
		}
	}

	FString FormatCombatLogEventMessage(const FBattleEvent& Event)
	{
		switch (Event.Type)
		{
		case EBattleEventType::CardDiscarded:
			return FString::Printf(TEXT("%s弃置 1 张牌"), *FormatCardZoneMoveReason(Event.HandCardZoneMoveReason));
		case EBattleEventType::CardExhausted:
			return FString::Printf(TEXT("%s消耗 1 张牌"), *FormatCardZoneMoveReason(Event.HandCardZoneMoveReason));
		default:
			return UWacomBattleEventPresentationBuilder::FormatEventForPlayer(Event);
		}
	}

	FText BuildHeaderText(const FWacomBattleCombatLogCommandContext& Context, const TArray<FBattleEvent>& Events)
	{
		switch (Context.CommandKind)
		{
		case EWacomBattleCombatLogCommandKind::System:
			return Context.TurnNumber > 0
				? FText::Format(LOCTEXT("SystemHeaderWithTurn", "战斗记录 · 第 {0} 回合"), FText::AsNumber(Context.TurnNumber))
				: LOCTEXT("SystemHeader", "战斗记录");
		case EWacomBattleCombatLogCommandKind::PlayCard:
		{
			int32 RuntimeCost = 0;
			for (const FBattleEvent& Event : Events)
			{
				if (Event.Type == EBattleEventType::CardPlayed)
				{
					RuntimeCost = Event.Amount;
					break;
				}
			}

			FText BaseText = Context.TargetName.IsEmpty()
				? FText::Format(LOCTEXT("PlayCardHeader", "打出「{0}」"), Context.CardName)
				: FText::Format(LOCTEXT("PlayCardTargetHeader", "打出「{0}」 -> {1}"), Context.CardName, Context.TargetName);
			return RuntimeCost > 0
				? FText::Format(LOCTEXT("PlayCardHeaderWithCost", "{0}，消耗 {1} 先机"), BaseText, FText::AsNumber(RuntimeCost))
				: BaseText;
		}
		case EWacomBattleCombatLogCommandKind::Wait:
			for (const FBattleEvent& Event : Events)
			{
				if (Event.Type == EBattleEventType::WaitPerformed)
				{
					return FText::Format(LOCTEXT("WaitHeader", "等待：敌方先机 -{0}"), FText::AsNumber(Event.Amount));
				}
			}
			return LOCTEXT("WaitHeaderFallback", "等待");
		case EWacomBattleCombatLogCommandKind::EndTurn:
			return LOCTEXT("EndTurnHeader", "结束回合");
		case EWacomBattleCombatLogCommandKind::KnockdownChoice:
			return FText::Format(
				LOCTEXT("KnockdownChoiceHeader", "击倒选择：{0}"),
				FText::FromString(UWacomBattleEventPresentationBuilder::FormatKnockdownChoice(Context.KnockdownChoice)));
		default:
			return LOCTEXT("UnknownHeader", "战斗事件");
		}
	}

	void ApplyEventSequenceRange(FWacomBattleCombatLogBlockView& Block, const TArray<FBattleEvent>& Events)
	{
		for (const FBattleEvent& Event : Events)
		{
			if (Event.Sequence <= 0)
			{
				continue;
			}

			if (Block.FirstEventSequence <= 0 || Event.Sequence < Block.FirstEventSequence)
			{
				Block.FirstEventSequence = Event.Sequence;
			}
			if (Event.Sequence > Block.LastEventSequence)
			{
				Block.LastEventSequence = Event.Sequence;
			}
		}
	}
}

FWacomBattleCombatLogCommandContext UWacomBattleCombatLogBuilder::BuildSystemCommandContext(
	const FBattleSnapshot& Snapshot)
{
	FWacomBattleCombatLogCommandContext Context;
	Context.CommandKind = EWacomBattleCombatLogCommandKind::System;
	Context.TurnNumber = Snapshot.TurnNumber;
	return Context;
}

FWacomBattleCombatLogCommandContext UWacomBattleCombatLogBuilder::BuildPlayCardCommandContext(
	const FBattleSnapshot& Snapshot,
	const FGuid& CardInstanceId,
	const FBattlePartSlotIdentity& TargetPartKey,
	const FGuid& TargetCardInstanceId)
{
	FWacomBattleCombatLogCommandContext Context;
	Context.CommandKind = EWacomBattleCombatLogCommandKind::PlayCard;
	Context.TurnNumber = Snapshot.TurnNumber;
	Context.CardInstanceId = CardInstanceId;
	Context.TargetPartKey = TargetPartKey;
	Context.TargetCardInstanceId = TargetCardInstanceId;
	Context.CardName = MakeCardName(FindHandCard(Snapshot, CardInstanceId));

	const FEnemyPartSnapshot* TargetPart = FindEnemyPart(Snapshot, TargetPartKey);
	if (TargetPart)
	{
		Context.TargetName = MakePartName(TargetPart);
	}
	else if (const FHandCardSnapshot* TargetCard = FindHandCard(Snapshot, TargetCardInstanceId))
	{
		Context.TargetName = MakeCardName(TargetCard);
	}

	return Context;
}

FWacomBattleCombatLogCommandContext UWacomBattleCombatLogBuilder::BuildWaitCommandContext(
	const FBattleSnapshot& Snapshot)
{
	FWacomBattleCombatLogCommandContext Context;
	Context.CommandKind = EWacomBattleCombatLogCommandKind::Wait;
	Context.TurnNumber = Snapshot.TurnNumber;
	return Context;
}

FWacomBattleCombatLogCommandContext UWacomBattleCombatLogBuilder::BuildEndTurnCommandContext(
	const FBattleSnapshot& Snapshot)
{
	FWacomBattleCombatLogCommandContext Context;
	Context.CommandKind = EWacomBattleCombatLogCommandKind::EndTurn;
	Context.TurnNumber = Snapshot.TurnNumber;
	return Context;
}

FWacomBattleCombatLogCommandContext UWacomBattleCombatLogBuilder::BuildKnockdownChoiceCommandContext(
	const FBattleSnapshot& Snapshot,
	EKnockdownChoice Choice)
{
	FWacomBattleCombatLogCommandContext Context;
	Context.CommandKind = EWacomBattleCombatLogCommandKind::KnockdownChoice;
	Context.TurnNumber = Snapshot.TurnNumber;
	Context.KnockdownChoice = Choice;
	return Context;
}

FWacomBattleCombatLogBlockView UWacomBattleCombatLogBuilder::BuildCombatLogBlock(
	const FWacomBattleCombatLogCommandContext& Context,
	const TArray<FBattleEvent>& Events,
	const FBattleSnapshot& PreCommandSnapshot,
	const FBattleSnapshot& PostCommandSnapshot)
{
	(void)PreCommandSnapshot;
	(void)PostCommandSnapshot;

	FWacomBattleCombatLogBlockView Block;
	Block.CommandKind = Context.CommandKind;
	Block.HeaderText = BuildHeaderText(Context, Events);
	Block.VisualTone = ToneForCommand(Context.CommandKind);
	Block.IconKey = IconForCommand(Context.CommandKind);
	ApplyEventSequenceRange(Block, Events);

	for (const FBattleEvent& Event : Events)
	{
		if (!ShouldShowCombatLogDetailForEvent(Event.Type))
		{
			continue;
		}

		const FString Message = FormatCombatLogEventMessage(Event);
		if (Message.IsEmpty())
		{
			continue;
		}

		const FBattleEventPresentationView EventView =
			UWacomBattleEventPresentationBuilder::BuildEventPresentationView(Event);

		FWacomBattleCombatLogLineView Line;
		Line.SourceEventType = Event.Type;
		Line.MessageText = FText::FromString(Message);
		Line.VisualTone = EventView.VisualTone;
		Line.IconKey = EventView.IconKey != NAME_None
			? EventView.IconKey
			: *StaticEnum<EBattleEventType>()->GetNameStringByValue(static_cast<int64>(Event.Type));
		Block.DetailLines.Add(Line);
	}

	Block.bShouldDisplay = !Block.HeaderText.IsEmpty() || !Block.DetailLines.IsEmpty();
	return Block;
}

FString UWacomBattleCombatLogBuilder::FormatCombatLogBlockForLog(
	const FWacomBattleCombatLogBlockView& Block)
{
	FString Result = Block.HeaderText.ToString();
	for (const FWacomBattleCombatLogLineView& Line : Block.DetailLines)
	{
		if (Line.MessageText.IsEmpty())
		{
			continue;
		}
		if (!Result.IsEmpty())
		{
			Result += TEXT(" | ");
		}
		Result += Line.MessageText.ToString();
	}
	return Result;
}

#undef LOCTEXT_NAMESPACE
