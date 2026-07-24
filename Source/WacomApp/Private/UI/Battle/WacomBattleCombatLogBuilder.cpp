// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleCombatLogBuilder.h"

#include "Cards/CardDefinition.h"
#include "Enemies/EnemyDefinition.h"
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

	const FHandCardSnapshot* FindHandCard(
		const FBattleSnapshot& PreCommandSnapshot,
		const FBattleSnapshot& PostCommandSnapshot,
		const FGuid& CardInstanceId)
	{
		if (const FHandCardSnapshot* Card = FindHandCard(PostCommandSnapshot, CardInstanceId))
		{
			return Card;
		}
		return FindHandCard(PreCommandSnapshot, CardInstanceId);
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

	const FEnemyPartSnapshot* FindEnemyPart(
		const FBattleSnapshot& Snapshot,
		const FBattleEnemyPartKey& PartKey)
	{
		if (!PartKey.IsValidKey())
		{
			return nullptr;
		}
		for (const FEnemySnapshot& Enemy : Snapshot.Enemies)
		{
			for (const FEnemyPartSnapshot& Part : Enemy.Parts)
			{
				if (Part.PartKey == PartKey)
				{
					return &Part;
				}
			}
		}
		return nullptr;
	}

	const FEnemyPartSnapshot* FindEnemyPart(
		const FBattleSnapshot& PreCommandSnapshot,
		const FBattleSnapshot& PostCommandSnapshot,
		const FBattleEnemyPartKey& PartKey)
	{
		if (const FEnemyPartSnapshot* Part = FindEnemyPart(PostCommandSnapshot, PartKey))
		{
			return Part;
		}
		return FindEnemyPart(PreCommandSnapshot, PartKey);
	}

	struct FEnemyPartPresentationContext
	{
		const FEnemySnapshot* Enemy = nullptr;
		const FEnemyPartSnapshot* Part = nullptr;
	};

	FEnemyPartPresentationContext FindEnemyPartPresentationContext(
		const FBattleSnapshot& Snapshot,
		const FBattleEnemyPartKey& PartKey)
	{
		if (!PartKey.IsValidKey())
		{
			return {};
		}
		for (const FEnemySnapshot& Enemy : Snapshot.Enemies)
		{
			for (const FEnemyPartSnapshot& Part : Enemy.Parts)
			{
				if (Part.PartKey == PartKey)
				{
					return { &Enemy, &Part };
				}
			}
		}
		return {};
	}

	FEnemyPartPresentationContext FindEnemyPartPresentationContext(
		const FBattleSnapshot& PreCommandSnapshot,
		const FBattleSnapshot& PostCommandSnapshot,
		const FBattleEnemyPartKey& PartKey)
	{
		FEnemyPartPresentationContext Result =
			FindEnemyPartPresentationContext(PostCommandSnapshot, PartKey);
		if (Result.Part)
		{
			return Result;
		}
		return FindEnemyPartPresentationContext(PreCommandSnapshot, PartKey);
	}

	FText MakeBracketedLabel(const FText& Label)
	{
		return Label.IsEmpty()
			? FText::GetEmpty()
			: FText::Format(LOCTEXT("DetailsBracketedLabel", "[{0}]"), Label);
	}

	FText MakeEnemyPartTargetLabel(
		const FBattleSnapshot& PreCommandSnapshot,
		const FBattleSnapshot& PostCommandSnapshot,
		const FBattleEnemyPartKey& PartKey)
	{
		const FEnemyPartPresentationContext Context =
			FindEnemyPartPresentationContext(
				PreCommandSnapshot,
				PostCommandSnapshot,
				PartKey);
		if (!Context.Enemy || !Context.Part)
		{
			return PartKey.IsValidKey()
				? MakeBracketedLabel(FText::FromString(PartKey.ToDebugString()))
				: FText::GetEmpty();
		}

		const FText EnemyName = Context.Enemy->Definition
			? MakeNameTextOrFallback(
				Context.Enemy->Definition->DisplayName,
				Context.Enemy->Definition->EnemyId.ToString())
			: FText::FromName(Context.Enemy->EnemySlotId);
		if (Context.Enemy->Parts.Num() <= 1)
		{
			return MakeBracketedLabel(EnemyName);
		}

		const FText PartName = MakePartName(Context.Part);
		return MakeBracketedLabel(FText::Format(
			LOCTEXT("DetailsEnemyPartLabel", "{0}·{1}"),
			EnemyName,
			PartName));
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
		case EBattleEventType::EnemyInitiativeChanged:
			return FString::Printf(
				TEXT("EnemyInitiativeChanged Delta=%+d Result=%d"),
				Event.Amount,
				Event.Count);
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

	int32 ResolveRootSequence(const TArray<FBattleEvent>& Events, EBattleEventType PreferredType)
	{
		for (const FBattleEvent& Event : Events)
		{
			if (Event.Type == PreferredType)
			{
				return Event.Sequence;
			}
		}
		for (const FBattleEvent& Event : Events)
		{
			if (Event.Sequence > 0)
			{
				return Event.Sequence;
			}
		}
		return 0;
	}

	FWacomBattleCombatActivityRowView MakePlayerRootRow(
		const FWacomBattleCombatLogCommandContext& Context,
		const TArray<FBattleEvent>& Events)
	{
		FWacomBattleCombatActivityRowView Row;
		Row.RowKind = EWacomBattleCombatActivityRowKind::RootAction;
		Row.VisualTone = ToneForCommand(Context.CommandKind);
		switch (Context.CommandKind)
		{
		case EWacomBattleCombatLogCommandKind::PlayCard:
			Row.SourceEventType = EBattleEventType::CardPlayed;
			Row.MessageText = Context.CardName.IsEmpty() ? LOCTEXT("ActivityUnknownCard", "未知卡牌") : Context.CardName;
			Row.IconKey = TEXT("Player");
			Row.EventSequence = ResolveRootSequence(Events, EBattleEventType::CardPlayed);
			break;
		case EWacomBattleCombatLogCommandKind::Wait:
			Row.SourceEventType = EBattleEventType::WaitPerformed;
			Row.MessageText = LOCTEXT("ActivityWait", "等待");
			Row.IconKey = TEXT("Wait");
			Row.EventSequence = ResolveRootSequence(Events, EBattleEventType::WaitPerformed);
			break;
		case EWacomBattleCombatLogCommandKind::KnockdownChoice:
			Row.SourceEventType = EBattleEventType::KnockdownChoiceMade;
			Row.MessageText = FText::FromString(UWacomBattleEventPresentationBuilder::FormatKnockdownChoice(Context.KnockdownChoice));
			Row.IconKey = TEXT("KnockdownChoice");
			Row.EventSequence = ResolveRootSequence(Events, EBattleEventType::KnockdownChoiceMade);
			break;
		default:
			break;
		}
		return Row;
	}

	FText ResolveEnemyIntentName(
		const FBattleEvent& Event,
		const FBattleSnapshot& PreCommandSnapshot,
		const FBattleSnapshot& PostCommandSnapshot)
	{
		const FEnemyPartSnapshot* Part = FindEnemyPart(PreCommandSnapshot, Event.ActorEnemyPartKey);
		if (!Part)
		{
			Part = FindEnemyPart(PostCommandSnapshot, Event.ActorEnemyPartKey);
		}
		if (Part && Part->CurrentIntent.IntentId == Event.IntentId && !Part->CurrentIntent.DisplayName.IsEmpty())
		{
			return Part->CurrentIntent.DisplayName;
		}
		return Event.IntentId.IsNone()
			? LOCTEXT("ActivityEnemyAction", "敌人行动")
			: FText::FromName(Event.IntentId);
	}

	FWacomBattleCombatActivityGroupView MakeEnemyActionGroup(
		const FBattleEvent& Event,
		const FBattleSnapshot& PreCommandSnapshot,
		const FBattleSnapshot& PostCommandSnapshot)
	{
		FWacomBattleCombatActivityGroupView Group;
		Group.TurnNumber = FMath::Max(PreCommandSnapshot.TurnNumber, 1);
		Group.RootAction.RowKind = EWacomBattleCombatActivityRowKind::RootAction;
		Group.RootAction.SourceEventType = EBattleEventType::EnemyPartActed;
		Group.RootAction.MessageText = ResolveEnemyIntentName(Event, PreCommandSnapshot, PostCommandSnapshot);
		if (Event.Count <= 0)
		{
			Group.RootAction.MessageText = FText::Format(
				LOCTEXT("ActivityEnemyActionSkipped", "{0}（跳过）"),
				Group.RootAction.MessageText);
		}
		Group.RootAction.VisualTone = EWacomBattleEventVisualTone::Danger;
		Group.RootAction.IconKey = TEXT("Intent");
		Group.RootAction.IntentId = Event.IntentId;
		Group.RootAction.EventSequence = Event.Sequence;
		return Group;
	}

	bool IsCombatLogDetailsResult(const EBattleEventType Type)
	{
		switch (Type)
		{
		case EBattleEventType::ResistanceResolved:
		case EBattleEventType::PerfectReleaseResolved:
		case EBattleEventType::DamageDealt:
		case EBattleEventType::ShieldChanged:
		case EBattleEventType::StatusApplied:
		case EBattleEventType::CardStatusChanged:
		case EBattleEventType::EnemyPartHpEmptied:
		case EBattleEventType::EnemyKnockdown:
		case EBattleEventType::KnockdownChoiceRequested:
		case EBattleEventType::PassiveTriggered:
		case EBattleEventType::CardDiscarded:
		case EBattleEventType::CardExhausted:
		case EBattleEventType::CardGained:
		case EBattleEventType::CardRuntimeCostChanged:
			return true;
		default:
			return false;
		}
	}

	bool IsShortActivityResult(const FBattleEvent& Event)
	{
		switch (Event.Type)
		{
		case EBattleEventType::DamageDealt:
			return Event.Amount > 0;
		case EBattleEventType::StatusApplied:
			return Event.Tag.IsValid() && Event.Amount != 0;
		case EBattleEventType::ResistanceResolved:
		case EBattleEventType::EnemyPartHpEmptied:
		case EBattleEventType::EnemyKnockdown:
			return true;
		default:
			return false;
		}
	}

	FText ResolveEventTargetName(
		const FBattleEvent& Event,
		const FBattleSnapshot& PreCommandSnapshot,
		const FBattleSnapshot& PostCommandSnapshot)
	{
		if (const FEnemyPartSnapshot* Part = FindEnemyPart(
			PreCommandSnapshot,
			PostCommandSnapshot,
			Event.ActorEnemyPartKey))
		{
			return MakePartName(Part);
		}
		if (const FHandCardSnapshot* Card = FindHandCard(
			PreCommandSnapshot,
			PostCommandSnapshot,
			Event.CardInstanceId))
		{
			return MakeCardName(Card);
		}
		return FText::GetEmpty();
	}

	FText MakeSignedDeltaText(int32 Amount)
	{
		return FText::FromString(Amount > 0
			? FString::Printf(TEXT("+%d"), Amount)
			: FString::FromInt(Amount));
	}

	FText BuildActivityResultText(
		const FBattleEvent& Event,
		const FBattleSnapshot& PreCommandSnapshot,
		const FBattleSnapshot& PostCommandSnapshot)
	{
		const FText TargetName = ResolveEventTargetName(Event, PreCommandSnapshot, PostCommandSnapshot);
		FText Result;
		switch (Event.Type)
		{
		case EBattleEventType::DamageDealt:
			Result = Event.Tag.IsValid()
				? FText::Format(LOCTEXT("ActivityTaggedDamage", "{0}造成 {1} 点伤害"), FText::FromString(UWacomBattleEventPresentationBuilder::FormatStatusName(Event.Tag)), FText::AsNumber(Event.Amount))
				: FText::Format(LOCTEXT("ActivityDamage", "受到 {0} 点伤害"), FText::AsNumber(Event.Amount));
			break;
		case EBattleEventType::StatusApplied:
			Result = FText::Format(
				LOCTEXT("ActivityStatusApplied", "{0} {1}"),
				FText::FromString(UWacomBattleEventPresentationBuilder::FormatStatusName(Event.Tag)),
				MakeSignedDeltaText(Event.Amount));
			break;
		case EBattleEventType::CardStatusChanged:
			Result = FText::Format(
				LOCTEXT("ActivityCardStatusChanged", "{0} {1}"),
				FText::FromString(UWacomBattleEventPresentationBuilder::FormatStatusName(Event.Tag)),
				MakeSignedDeltaText(Event.Amount));
			break;
		case EBattleEventType::EnemyInitiativeChanged:
			Result = FText::Format(LOCTEXT("ActivityInitiativeChanged", "先机 {0}"), MakeSignedDeltaText(Event.Amount));
			break;
		case EBattleEventType::PassiveTriggered:
			Result = Event.Tag.IsValid()
				? FText::Format(LOCTEXT("ActivityPassive", "{0}触发"), FText::FromString(UWacomBattleEventPresentationBuilder::FormatStatusName(Event.Tag)))
				: LOCTEXT("ActivityPassiveFallback", "被动效果触发");
			break;
		case EBattleEventType::CardDiscarded:
			Result = LOCTEXT("ActivityCardDiscarded", "弃置");
			break;
		case EBattleEventType::CardExhausted:
			Result = LOCTEXT("ActivityCardExhausted", "消耗");
			break;
		case EBattleEventType::CardGained:
			Result = Event.CardDefinition
				? FText::Format(LOCTEXT("ActivityCardGainedNamed", "获得「{0}」"), FText::FromString(UWacomBattleEventPresentationBuilder::FormatCardName(Event.CardDefinition.Get())))
				: LOCTEXT("ActivityCardGained", "获得卡牌");
			break;
		case EBattleEventType::CardRuntimeCostChanged:
			Result = FText::Format(LOCTEXT("ActivityCostChanged", "费用 {0}"), MakeSignedDeltaText(Event.Amount));
			break;
		default:
			Result = FText::FromString(UWacomBattleEventPresentationBuilder::FormatEventForPlayer(Event));
			break;
		}

		if (!TargetName.IsEmpty() && !Result.IsEmpty())
		{
			return FText::Format(LOCTEXT("ActivityTargetResult", "{0}：{1}"), TargetName, Result);
		}
		return Result;
	}

	FWacomBattleCombatActivityRowView MakeResultRow(
		const FBattleEvent& Event,
		const FBattleSnapshot& PreCommandSnapshot,
		const FBattleSnapshot& PostCommandSnapshot)
	{
		const FBattleEventPresentationView EventView =
			UWacomBattleEventPresentationBuilder::BuildEventPresentationView(Event);
		FWacomBattleCombatActivityRowView Row;
		Row.RowKind = EWacomBattleCombatActivityRowKind::Result;
		Row.SourceEventType = Event.Type;
		Row.MessageText = BuildActivityResultText(Event, PreCommandSnapshot, PostCommandSnapshot);
		Row.VisualTone = EventView.VisualTone;
		Row.IconKey = EventView.IconKey != NAME_None
			? EventView.IconKey
			: *StaticEnum<EBattleEventType>()->GetNameStringByValue(static_cast<int64>(Event.Type));
		Row.IconTag = Event.Tag;
		Row.EventSequence = Event.Sequence;
		return Row;
	}

	FText ResolveDetailsTargetLabel(
		const FBattleEvent& Event,
		const FBattleSnapshot& PreCommandSnapshot,
		const FBattleSnapshot& PostCommandSnapshot)
	{
		if (Event.ActorEnemyPartKey.IsValidKey())
		{
			return MakeEnemyPartTargetLabel(
				PreCommandSnapshot,
				PostCommandSnapshot,
				Event.ActorEnemyPartKey);
		}
		if (const FHandCardSnapshot* Card = FindHandCard(
			PreCommandSnapshot,
			PostCommandSnapshot,
			Event.CardInstanceId))
		{
			return MakeBracketedLabel(MakeCardName(Card));
		}
		if (Event.CardDefinition)
		{
			return MakeBracketedLabel(FText::FromString(
				UWacomBattleEventPresentationBuilder::FormatCardName(
					Event.CardDefinition.Get())));
		}
		if (Event.CardInstanceId.IsValid())
		{
			return MakeBracketedLabel(FText::FromString(
				Event.CardInstanceId.ToString(
					EGuidFormats::DigitsWithHyphensLower)));
		}
		if (Event.Type == EBattleEventType::DamageDealt
			|| Event.Type == EBattleEventType::ShieldChanged
			|| Event.Type == EBattleEventType::StatusApplied)
		{
			return MakeBracketedLabel(LOCTEXT("DetailsPlayerTarget", "玩家"));
		}
		return FText::GetEmpty();
	}

	FText ResolveDetailsCardName(
		const FBattleEvent& Event,
		const FBattleSnapshot& PreCommandSnapshot,
		const FBattleSnapshot& PostCommandSnapshot)
	{
		if (Event.CardDefinition)
		{
			return FText::FromString(
				UWacomBattleEventPresentationBuilder::FormatCardName(
					Event.CardDefinition.Get()));
		}
		if (const FHandCardSnapshot* Card = FindHandCard(
			PreCommandSnapshot,
			PostCommandSnapshot,
			Event.CardInstanceId))
		{
			return MakeCardName(Card);
		}
		if (Event.CardInstanceId.IsValid())
		{
			return FText::FromString(Event.CardInstanceId.ToString(
				EGuidFormats::DigitsWithHyphensLower));
		}
		return LOCTEXT("DetailsUnknownCard", "未知卡牌");
	}

	FWacomBattleCombatLogDetailsEntryView MakeDetailsRootEntry(
		const FWacomBattleCombatActivityRowView& RootRow)
	{
		FWacomBattleCombatLogDetailsEntryView Entry;
		Entry.EntryKind = EWacomBattleCombatLogDetailsEntryKind::RootAction;
		Entry.Depth = 0;
		Entry.SourceEventType = RootRow.SourceEventType;
		Entry.MessageText = RootRow.MessageText;
		Entry.VisualTone = RootRow.VisualTone;
		Entry.IconKey = RootRow.IconKey;
		Entry.IconTag = RootRow.IconTag;
		Entry.IntentId = RootRow.IntentId;
		Entry.EventSequence = RootRow.EventSequence;
		return Entry;
	}

	void AppendDetailsEntriesForEvent(
		FWacomBattleCombatLogDetailsGroupView& Group,
		const FBattleEvent& Event,
		const FBattleSnapshot& PreCommandSnapshot,
		const FBattleSnapshot& PostCommandSnapshot)
	{
		const FBattleEventPresentationView EventView =
			UWacomBattleEventPresentationBuilder::BuildEventPresentationView(Event);
		FWacomBattleCombatLogDetailsEntryView Entry;
		Entry.EntryKind = EWacomBattleCombatLogDetailsEntryKind::Result;
		Entry.Depth = 1;
		Entry.SourceEventType = Event.Type;
		Entry.TargetLabel = ResolveDetailsTargetLabel(
			Event,
			PreCommandSnapshot,
			PostCommandSnapshot);
		Entry.VisualTone = EventView.VisualTone;
		Entry.IconKey = EventView.IconKey != NAME_None
			? EventView.IconKey
			: *StaticEnum<EBattleEventType>()->GetNameStringByValue(
				static_cast<int64>(Event.Type));
		Entry.IconTag = Event.Tag;
		Entry.EventSequence = Event.Sequence;

		if (Event.Type == EBattleEventType::DamageDealt)
		{
			if (Event.DamageResolution.ShieldAbsorbed > 0)
			{
				FWacomBattleCombatLogDetailsEntryView ShieldEntry = Entry;
				ShieldEntry.MessageText = Event.DamageResolution.ShieldAfter <= 0
					? LOCTEXT("DetailsShieldAbsorbedBroken", "护盾吸收（击破）")
					: LOCTEXT("DetailsShieldAbsorbed", "护盾吸收");
				ShieldEntry.ValueText =
					FText::AsNumber(Event.DamageResolution.ShieldAbsorbed);
				ShieldEntry.IconKey = TEXT("ShieldChanged");
				ShieldEntry.VisualTone = EWacomBattleEventVisualTone::Warning;
				Group.Entries.Add(MoveTemp(ShieldEntry));
			}

			if (Event.Amount > 0)
			{
				if (Event.DamageResolution.bCritical)
				{
					Entry.MessageText = LOCTEXT("DetailsCriticalDamage", "暴击伤害");
					Entry.VisualTone = EWacomBattleEventVisualTone::Positive;
				}
				else if (Event.DamageResolution.Kind == EBattleDamageKind::Periodic)
				{
					Entry.MessageText = Event.Tag.IsValid()
						? FText::Format(
							LOCTEXT("DetailsPeriodicDamageNamed", "{0}伤害"),
							FText::FromString(
								UWacomBattleEventPresentationBuilder::FormatStatusName(
									Event.Tag)))
						: LOCTEXT("DetailsPeriodicDamage", "周期伤害");
				}
				else
				{
					Entry.MessageText = LOCTEXT("DetailsDamage", "受到伤害");
				}
				Entry.ValueText = FText::AsNumber(Event.Amount);
				Group.Entries.Add(Entry);

				if (Event.DamageResolution.Overkill > 0)
				{
					FWacomBattleCombatLogDetailsEntryView& Overkill =
						Group.Entries.AddDefaulted_GetRef();
					Overkill.EntryKind = EWacomBattleCombatLogDetailsEntryKind::Fact;
					Overkill.Depth = 2;
					Overkill.SourceEventType = Event.Type;
					Overkill.MessageText = FText::Format(
						LOCTEXT("DetailsOverkill", "溢出伤害 {0}"),
						FText::AsNumber(Event.DamageResolution.Overkill));
					Overkill.VisualTone = Entry.VisualTone;
					Overkill.EventSequence = Event.Sequence;
				}
			}
			else if (Event.DamageResolution.ShieldAbsorbed <= 0)
			{
				// Preserve legacy and diagnostic zero-damage facts. A structured
				// fully absorbed hit already emitted its shield row above and must
				// not add a misleading second "-0" HP row.
				Entry.MessageText = LOCTEXT("DetailsZeroDamage", "受到伤害");
				Entry.ValueText = FText::AsNumber(0);
				Group.Entries.Add(Entry);
			}
			return;
		}

		switch (Event.Type)
		{
		case EBattleEventType::ResistanceResolved:
			Entry.MessageText = Event.bSuccess
				? LOCTEXT("DetailsResistanceSucceeded", "抵抗成功")
				: LOCTEXT("DetailsResistanceFailed", "抵抗失败");
			Entry.VisualTone = Event.bSuccess
				? EWacomBattleEventVisualTone::Positive
				: EWacomBattleEventVisualTone::Danger;
			break;

		case EBattleEventType::ShieldChanged:
			Entry.MessageText = LOCTEXT("DetailsShieldChanged", "护盾");
			Entry.ValueText = MakeSignedDeltaText(Event.Amount);
			Entry.VisualTone = Event.Amount > 0
				? EWacomBattleEventVisualTone::Positive
				: EWacomBattleEventVisualTone::Neutral;
			Entry.IconKey = TEXT("ShieldChanged");
			break;

		case EBattleEventType::StatusApplied:
			Entry.MessageText = FText::FromString(
				UWacomBattleEventPresentationBuilder::FormatStatusName(Event.Tag));
			Entry.ValueText = MakeSignedDeltaText(Event.Amount);
			Entry.StatusInspectionHost = Event.ActorEnemyPartKey.IsValidKey()
				? EWacomBattleStatusInspectionHost::EnemyPart
				: EWacomBattleStatusInspectionHost::Player;
			Entry.StatusDelta = Event.Amount;
			Entry.bShowStatusTooltip = Event.Tag.IsValid();
			break;

		case EBattleEventType::CardStatusChanged:
			Entry.MessageText = FText::FromString(
				UWacomBattleEventPresentationBuilder::FormatStatusName(Event.Tag));
			Entry.ValueText = MakeSignedDeltaText(Event.Amount);
			break;

		case EBattleEventType::PassiveTriggered:
			Entry.MessageText = Event.Tag.IsValid()
				? FText::Format(
					LOCTEXT("DetailsPassive", "{0}触发"),
					FText::FromString(
						UWacomBattleEventPresentationBuilder::FormatStatusName(
							Event.Tag)))
				: LOCTEXT("DetailsPassiveFallback", "被动效果触发");
			break;

		case EBattleEventType::CardDiscarded:
			Entry.MessageText = LOCTEXT("DetailsCardDiscarded", "弃置");
			break;

		case EBattleEventType::CardExhausted:
			Entry.MessageText = LOCTEXT("DetailsCardExhausted", "消耗");
			break;

		case EBattleEventType::CardGained:
			Entry.MessageText =
				LOCTEXT("DetailsCardGained", "获得卡牌");
			Entry.ValueText = FText::Format(
				LOCTEXT("DetailsCardGainedName", "「{0}」"),
				ResolveDetailsCardName(
					Event,
					PreCommandSnapshot,
					PostCommandSnapshot));
			break;

		case EBattleEventType::CardRuntimeCostChanged:
			Entry.MessageText = LOCTEXT("DetailsCardCostChanged", "费用");
			Entry.ValueText = MakeSignedDeltaText(Event.Amount);
			break;

		case EBattleEventType::EnemyPartHpEmptied:
			Entry.MessageText = LOCTEXT("DetailsEnemyPartDestroyed", "被击破");
			break;

		case EBattleEventType::EnemyKnockdown:
			Entry.MessageText = LOCTEXT("DetailsEnemyKnockdown", "触发击倒");
			break;

		default:
			Entry.MessageText = EventView.MessageText;
			break;
		}

		if (Entry.MessageText.IsEmpty() && Entry.ValueText.IsEmpty())
		{
			return;
		}
		Group.Entries.Add(Entry);

		if (Event.Type == EBattleEventType::ResistanceResolved)
		{
			FWacomBattleCombatLogDetailsEntryView& Fact =
				Group.Entries.AddDefaulted_GetRef();
			Fact.EntryKind = EWacomBattleCombatLogDetailsEntryKind::Fact;
			Fact.Depth = 2;
			Fact.SourceEventType = Event.Type;
			Fact.MessageText = FText::Format(
				LOCTEXT(
					"DetailsResistanceComparison",
					"卡牌单段 {0} {1} 敌方单段 {2}"),
				FText::AsNumber(Event.Amount),
				Event.bSuccess
					? LOCTEXT("DetailsResistanceGreater", ">")
					: LOCTEXT("DetailsResistanceLessOrEqual", "≤"),
				FText::AsNumber(Event.Count));
			Fact.VisualTone = Entry.VisualTone;
			Fact.EventSequence = Event.Sequence;
		}
	}

	FWacomBattleCombatLogDetailsGroupView MakeEnemyActionDetailsGroup(
		const FBattleEvent& Event,
		const FBattleSnapshot& PreCommandSnapshot,
		const FBattleSnapshot& PostCommandSnapshot)
	{
		const FWacomBattleCombatActivityGroupView ActivityGroup =
			MakeEnemyActionGroup(
				Event,
				PreCommandSnapshot,
				PostCommandSnapshot);
		FWacomBattleCombatLogDetailsGroupView Group;
		Group.TurnNumber = ActivityGroup.TurnNumber;
		Group.RootAction = MakeDetailsRootEntry(ActivityGroup.RootAction);
		return Group;
	}

	FWacomBattleCombatActivityBatchView BuildActivityBatch(
		const FWacomBattleCombatLogCommandContext& Context,
		const TArray<FBattleEvent>& Events,
		const FBattleSnapshot& PreCommandSnapshot,
		const FBattleSnapshot& PostCommandSnapshot)
	{
		FWacomBattleCombatActivityBatchView Batch;
		if (Context.CommandKind == EWacomBattleCombatLogCommandKind::System)
		{
			Batch.bSetTurnImmediately = true;
			Batch.PresentedTurnNumber = FMath::Max(PostCommandSnapshot.TurnNumber, 1);
			return Batch;
		}

		FWacomBattleCombatActivityGroupView* CurrentGroup = nullptr;
		if (Context.CommandKind == EWacomBattleCombatLogCommandKind::PlayCard
			|| Context.CommandKind == EWacomBattleCombatLogCommandKind::Wait
			|| Context.CommandKind == EWacomBattleCombatLogCommandKind::KnockdownChoice)
		{
			FWacomBattleCombatActivityGroupView& Group = Batch.Groups.AddDefaulted_GetRef();
			Group.TurnNumber = FMath::Max(PreCommandSnapshot.TurnNumber, 1);
			Group.RootAction = MakePlayerRootRow(Context, Events);
			CurrentGroup = &Group;
		}

		for (const FBattleEvent& Event : Events)
		{
			if (Event.Type == EBattleEventType::EnemyPartActed)
			{
				CurrentGroup = &Batch.Groups.Add_GetRef(MakeEnemyActionGroup(
					Event,
					PreCommandSnapshot,
					PostCommandSnapshot));
				continue;
			}
			if (!CurrentGroup || !IsShortActivityResult(Event))
			{
				continue;
			}
			FWacomBattleCombatActivityRowView Row = MakeResultRow(
				Event,
				PreCommandSnapshot,
				PostCommandSnapshot);
			if (!Row.MessageText.IsEmpty())
			{
				CurrentGroup->ResultRows.Add(MoveTemp(Row));
			}
		}

		if (Context.CommandKind == EWacomBattleCombatLogCommandKind::EndTurn
			&& PostCommandSnapshot.TurnNumber > PreCommandSnapshot.TurnNumber)
		{
			Batch.bAdvanceTurnAfterPlayback = true;
			Batch.PresentedTurnNumber = PostCommandSnapshot.TurnNumber;
		}
		return Batch;
	}

	FWacomBattleCombatLogDetailsBatchView BuildDetailsBatch(
		const FWacomBattleCombatLogCommandContext& Context,
		const TArray<FBattleEvent>& Events,
		const FBattleSnapshot& PreCommandSnapshot,
		const FBattleSnapshot& PostCommandSnapshot)
	{
		FWacomBattleCombatLogDetailsBatchView Batch;
		if (Context.CommandKind == EWacomBattleCombatLogCommandKind::System)
		{
			Batch.bSetTurnImmediately = true;
			Batch.PresentedTurnNumber = FMath::Max(
				PostCommandSnapshot.TurnNumber,
				1);
			return Batch;
		}

		FWacomBattleCombatLogDetailsGroupView* CurrentGroup = nullptr;
		if (Context.CommandKind == EWacomBattleCombatLogCommandKind::PlayCard
			|| Context.CommandKind == EWacomBattleCombatLogCommandKind::Wait
			|| Context.CommandKind
				== EWacomBattleCombatLogCommandKind::KnockdownChoice)
		{
			FWacomBattleCombatLogDetailsGroupView& Group =
				Batch.Groups.AddDefaulted_GetRef();
			Group.TurnNumber = FMath::Max(PreCommandSnapshot.TurnNumber, 1);
			Group.RootAction = MakeDetailsRootEntry(
				MakePlayerRootRow(Context, Events));
			CurrentGroup = &Group;
		}

		for (const FBattleEvent& Event : Events)
		{
			if (Event.Type == EBattleEventType::EnemyPartActed)
			{
				CurrentGroup = &Batch.Groups.Add_GetRef(
					MakeEnemyActionDetailsGroup(
						Event,
						PreCommandSnapshot,
						PostCommandSnapshot));
				continue;
			}
			if (!CurrentGroup || !IsCombatLogDetailsResult(Event.Type))
			{
				continue;
			}
			AppendDetailsEntriesForEvent(
				*CurrentGroup,
				Event,
				PreCommandSnapshot,
				PostCommandSnapshot);
		}

		if (Context.CommandKind == EWacomBattleCombatLogCommandKind::EndTurn
			&& PostCommandSnapshot.TurnNumber > PreCommandSnapshot.TurnNumber)
		{
			Batch.bAdvanceTurnAfterPlayback = true;
			Batch.PresentedTurnNumber = PostCommandSnapshot.TurnNumber;
		}
		return Batch;
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

FWacomBattleCombatActivityBatchView UWacomBattleCombatLogBuilder::BuildCombatActivityBatch(
	const FWacomBattleCombatLogCommandContext& Context,
	const TArray<FBattleEvent>& Events,
	const FBattleSnapshot& PreCommandSnapshot,
	const FBattleSnapshot& PostCommandSnapshot)
{
	return BuildActivityBatch(
		Context,
		Events,
		PreCommandSnapshot,
		PostCommandSnapshot);
}

FWacomBattleCombatLogDetailsBatchView UWacomBattleCombatLogBuilder::BuildCombatLogDetailsBatch(
	const FWacomBattleCombatLogCommandContext& Context,
	const TArray<FBattleEvent>& Events,
	const FBattleSnapshot& PreCommandSnapshot,
	const FBattleSnapshot& PostCommandSnapshot)
{
	return BuildDetailsBatch(
		Context,
		Events,
		PreCommandSnapshot,
		PostCommandSnapshot);
}

FWacomBattleCombatActivityBatchView UWacomBattleCombatLogBuilder::BuildInitialTurnActivityBatch(
	const int32 TurnNumber)
{
	const int32 SafeTurnNumber = FMath::Max(TurnNumber, 1);
	FWacomBattleCombatActivityBatchView Batch;
	Batch.bSetTurnImmediately = true;
	Batch.PresentedTurnNumber = SafeTurnNumber;

	FWacomBattleCombatActivityGroupView& Group = Batch.Groups.AddDefaulted_GetRef();
	Group.TurnNumber = SafeTurnNumber;
	Group.RootAction.RowKind = EWacomBattleCombatActivityRowKind::RootAction;
	Group.RootAction.SourceEventType = EBattleEventType::TurnStarted;
	Group.RootAction.MessageText = FText::Format(
		LOCTEXT("ActivityInitialTurnStarted", "第{0}回合开始"),
		FText::AsNumber(SafeTurnNumber));
	Group.RootAction.VisualTone = EWacomBattleEventVisualTone::System;
	Group.RootAction.IconKey = TEXT("TurnStart");
	return Batch;
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
