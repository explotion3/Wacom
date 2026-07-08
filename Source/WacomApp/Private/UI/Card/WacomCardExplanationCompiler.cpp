// Copyright Wacom. All Rights Reserved.

#include "WacomCardExplanationCompiler.h"

#include "Cards/CardDefinition.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Card/WacomCardExplanationLexicon.h"

#define LOCTEXT_NAMESPACE "WacomCardExplanationCompiler"

DEFINE_LOG_CATEGORY_STATIC(LogWacomCardExplanation, Log, All);

namespace WacomCardExplanationCompiler
{
	namespace
	{
		const FWacomCardPresentationRuntimeContext::FEffectPreview* FindEffectPreview(
			const FWacomCardPresentationRuntimeContext& RuntimeContext,
			int32 EffectIndex)
		{
			for (const FWacomCardPresentationRuntimeContext::FEffectPreview& Preview : RuntimeContext.EffectPreviews)
			{
				if (Preview.EffectIndex == EffectIndex)
				{
					return &Preview;
				}
			}
			return nullptr;
		}

		bool UsesRuntimeCostMagnitude(const FCardEffect& Effect)
		{
			return Effect.MagnitudeSource.MatchesTagExact(WacomTags::Magnitude_Source_RuntimeCost)
				|| (!Effect.MagnitudeSource.IsValid() && Effect.bMagnitudeFromRuntimeCost);
		}

		int32 GetBaseDisplayMagnitude(
			const UCardDefinition* Card,
			const FCardEffect& Effect,
			const FWacomCardPresentationRuntimeContext& RuntimeContext)
		{
			if (UsesRuntimeCostMagnitude(Effect))
			{
				if (RuntimeContext.bHasRuntimeCost)
				{
					return RuntimeContext.RuntimeCost;
				}
				return Card ? Card->BaseCost : Effect.Magnitude;
			}
			return Effect.Magnitude;
		}

		FName StableRunId(const FString& Prefix, int32 RunIndex, const FString& Suffix)
		{
			return FName(*FString::Printf(TEXT("%s.Run.%d.%s"), *Prefix, RunIndex, *Suffix));
		}

		FString ShortTagName(const FGameplayTag& Tag)
		{
			const FString TagText = Tag.IsValid() ? Tag.ToString() : FString();
			int32 DotIndex = INDEX_NONE;
			TagText.FindLastChar(TEXT('.'), DotIndex);
			return DotIndex == INDEX_NONE ? TagText : TagText.Mid(DotIndex + 1);
		}

		FText DisplayStatusName(const FGameplayTag& StatusTag)
		{
			if (StatusTag.MatchesTagExact(WacomTags::Status_Poison))
			{
				return LOCTEXT("StatusPoison", "中毒");
			}
			if (StatusTag.MatchesTagExact(WacomTags::Status_Slow))
			{
				return LOCTEXT("StatusSlow", "迟缓");
			}
			if (StatusTag.MatchesTagExact(WacomTags::Status_Freeze))
			{
				return LOCTEXT("StatusFreeze", "冻结");
			}
			if (StatusTag.MatchesTagExact(WacomTags::Status_Twilight))
			{
				return LOCTEXT("StatusTwilight", "暮气");
			}
			if (StatusTag.MatchesTagExact(WacomTags::Status_Stunned))
			{
				return LOCTEXT("StatusStunned", "眩晕");
			}
			if (StatusTag.MatchesTagExact(WacomTags::Status_Shield))
			{
				return LOCTEXT("StatusShield", "护盾");
			}
			return FText::FromString(ShortTagName(StatusTag));
		}

		FGameplayTag ResolveEffectStatusTag(const FCardEffect& Effect)
		{
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Poison))
			{
				return WacomTags::Status_Poison;
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Slow))
			{
				return WacomTags::Status_Slow;
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Freeze))
			{
				return WacomTags::Status_Freeze;
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Twilight))
			{
				return WacomTags::Status_Twilight;
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Status_Shield))
			{
				return WacomTags::Status_Shield;
			}
			return FGameplayTag();
		}

		EWacomCardDetailIcon ResolveEffectIcon(const FCardEffect& Effect)
		{
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Damage))
			{
				return EWacomCardDetailIcon::Damage;
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Heal))
			{
				return EWacomCardDetailIcon::Heal;
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Status_Shield))
			{
				return EWacomCardDetailIcon::Shield;
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Poison))
			{
				return EWacomCardDetailIcon::Poison;
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Slow))
			{
				return EWacomCardDetailIcon::Slow;
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Freeze))
			{
				return EWacomCardDetailIcon::Freeze;
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Twilight))
			{
				return EWacomCardDetailIcon::Twilight;
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Card_AddCost)
				|| Effect.EffectType.MatchesTagExact(WacomTags::Effect_Card_ReduceCost))
			{
				return EWacomCardDetailIcon::Cost;
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Draw))
			{
				return EWacomCardDetailIcon::Draw;
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Discard)
				|| Effect.EffectType.MatchesTagExact(WacomTags::Effect_Card_DiscardSelected))
			{
				return EWacomCardDetailIcon::Discard;
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ExhaustSelf)
				|| Effect.EffectType.MatchesTagExact(WacomTags::Effect_Card_ExhaustSelected))
			{
				return EWacomCardDetailIcon::Exhaust;
			}
			return EWacomCardDetailIcon::Keyword;
		}

		FText DefaultEffectTemplate(const FCardEffect& Effect)
		{
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Damage))
			{
				return LOCTEXT("TemplateDamage", "造成 {value:Magnitude} 点伤害。");
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Heal))
			{
				return LOCTEXT("TemplateHeal", "恢复 {value:Magnitude} 点生命。");
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Status_Shield))
			{
				return LOCTEXT("TemplateShield", "获得 {value:Magnitude} 点护盾。");
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Poison)
				|| Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Slow)
				|| Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Freeze)
				|| Effect.EffectType.MatchesTagExact(WacomTags::Effect_ApplyStatus_Twilight))
			{
				return LOCTEXT("TemplateApplyStatus", "施加 {value:Magnitude} 层 {status:EffectStatus}。");
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Card_AddCost))
			{
				return LOCTEXT("TemplateAddCost", "目标手牌费用增加 {value:Magnitude}。");
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Card_ReduceCost))
			{
				return LOCTEXT("TemplateReduceCost", "目标手牌费用降低 {value:Magnitude}。");
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Card_DiscardSelected))
			{
				return LOCTEXT("TemplateDiscardSelected", "弃置目标手牌。");
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Card_ExhaustSelected))
			{
				return LOCTEXT("TemplateExhaustSelected", "消耗目标手牌。");
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Shuffle_Random))
			{
				return LOCTEXT("TemplateShuffleRandom", "随机腾挪 1 张手牌。");
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Shuffle_FromBothToOther))
			{
				return LOCTEXT("TemplateShuffleFromBothToOther", "将双手区随机 1 张卡牌腾挪至其他区域。");
			}
			if (Effect.EffectType.MatchesTagExact(WacomTags::Effect_Shuffle_ToRandomZone))
			{
				return LOCTEXT("TemplateShuffleToRandomZone", "该牌腾挪至随机区域。");
			}
			return FText::Format(
				LOCTEXT("TemplateUnknownEffect", "{0}。"),
				FText::FromString(ShortTagName(Effect.EffectType)));
		}

		FText DefaultPassiveTriggerTemplate(const FCardPassive& Passive)
		{
			if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_AfterPlayed))
			{
				return LOCTEXT("TemplatePassiveAfterPlayed", "打出后：");
			}
			if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnCompanionCount))
			{
				return LOCTEXT("TemplatePassiveCompanionCount", "每打出 {value:TriggerThreshold} 张伙伴：");
			}
			if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnTwilightTriggered))
			{
				return LOCTEXT("TemplatePassiveTwilight", "暮气触发时：");
			}
			if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnTurnStart))
			{
				return LOCTEXT("TemplatePassiveTurnStart", "回合开始：");
			}
			if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnTurnEnd))
			{
				return LOCTEXT("TemplatePassiveTurnEnd", "回合结束：");
			}
			if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnDraw))
			{
				return LOCTEXT("TemplatePassiveDraw", "抽到时：");
			}
			if (Passive.Trigger.MatchesTagExact(WacomTags::Passive_Trigger_OnDiscard))
			{
				return LOCTEXT("TemplatePassiveDiscard", "弃掉时：");
			}
			return FText::Format(
				LOCTEXT("TemplatePassiveUnknown", "{0}："),
				FText::FromString(ShortTagName(Passive.Trigger)));
		}

		void AddTextRun(
			FWacomCardDetailBlock& Block,
			const FString& Text,
			const FString& StableIdPrefix,
			int32& RunIndex)
		{
			if (Text.IsEmpty())
			{
				return;
			}

			FWacomCardDetailRun Run;
			Run.StableId = StableRunId(StableIdPrefix, RunIndex, TEXT("Text"));
			Run.Kind = EWacomCardDetailRunKind::Text;
			Run.Text = FText::FromString(Text);
			Block.Runs.Add(MoveTemp(Run));
			++RunIndex;
		}

		void AddValueRun(
			FWacomCardDetailBlock& Block,
			const FName SlotName,
			int32 Value,
			const FWacomCardPresentationRuntimeContext::FEffectPreview* Preview,
			const FString& StableIdPrefix,
			int32& RunIndex)
		{
			FWacomCardDetailRun Run;
			Run.StableId = StableRunId(StableIdPrefix, RunIndex, SlotName.ToString());
			Run.Kind = EWacomCardDetailRunKind::Value;
			Run.SlotName = SlotName;
			Run.Value = Value;
			Run.bHasValue = true;
			if (Preview && !Preview->bSkip && Preview->bHasMagnitude && Preview->Magnitude != Value)
			{
				Run.PreviewValue = Preview->Magnitude;
				Run.bHasPreviewValue = true;
				Run.bEmphasized = true;
			}
			Run.bSkipped = Block.bSkipped;
			Block.Runs.Add(MoveTemp(Run));
			++RunIndex;
		}

		void AddIconRun(
			FWacomCardDetailBlock& Block,
			EWacomCardDetailIcon Icon,
			const FString& StableIdPrefix,
			int32& RunIndex)
		{
			FWacomCardDetailRun Run;
			Run.StableId = StableRunId(StableIdPrefix, RunIndex, TEXT("Icon"));
			Run.Kind = EWacomCardDetailRunKind::Icon;
			Run.Icon = Icon;
			Run.bSkipped = Block.bSkipped;
			Block.Runs.Add(MoveTemp(Run));
			++RunIndex;
		}

		void AddStatusRun(
			FWacomCardDetailBlock& Block,
			FGameplayTag StatusTag,
			const FString& StableIdPrefix,
			int32& RunIndex)
		{
			FWacomCardDetailRun Run;
			Run.StableId = StableRunId(StableIdPrefix, RunIndex, TEXT("Status"));
			Run.Kind = EWacomCardDetailRunKind::Status;
			Run.Tag = StatusTag;
			Run.Text = DisplayStatusName(StatusTag);
			Run.bSkipped = Block.bSkipped;
			Block.Runs.Add(MoveTemp(Run));
			++RunIndex;
		}

		void AddKeywordRun(
			FWacomCardDetailBlock& Block,
			FGameplayTag Tag,
			const FString& StableIdPrefix,
			int32& RunIndex)
		{
			FWacomCardDetailRun Run;
			Run.StableId = StableRunId(StableIdPrefix, RunIndex, TEXT("Keyword"));
			Run.Kind = EWacomCardDetailRunKind::Keyword;
			Run.Tag = Tag;
			Run.Text = FText::FromString(ShortTagName(Tag));
			Run.bSkipped = Block.bSkipped;
			Block.Runs.Add(MoveTemp(Run));
			++RunIndex;
		}

		bool TryAppendSlot(
			FWacomCardDetailBlock& Block,
			const FString& Slot,
			const UCardDefinition* Card,
			const FCardEffect* Effect,
			const FCardPassive* Passive,
			const FWacomCardPresentationRuntimeContext& RuntimeContext,
			const FWacomCardPresentationRuntimeContext::FEffectPreview* Preview,
			const FString& StableIdPrefix,
			int32& RunIndex)
		{
			FString SlotType;
			FString SlotNameText;
			if (!Slot.Split(TEXT(":"), &SlotType, &SlotNameText))
			{
				return false;
			}

			const FName SlotName(*SlotNameText);
			if (SlotType == TEXT("value"))
			{
				if (SlotNameText == TEXT("Magnitude") && Effect)
				{
					AddValueRun(
						Block,
						SlotName,
						GetBaseDisplayMagnitude(Card, *Effect, RuntimeContext),
						Preview,
						StableIdPrefix,
						RunIndex);
					return true;
				}
				if (SlotNameText == TEXT("TriggerThreshold") && Passive)
				{
					AddValueRun(
						Block,
						SlotName,
						Passive->TriggerThreshold,
						nullptr,
						StableIdPrefix,
						RunIndex);
					return true;
				}
			}
			else if (SlotType == TEXT("icon") && SlotNameText == TEXT("EffectIcon") && Effect)
			{
				AddIconRun(Block, ResolveEffectIcon(*Effect), StableIdPrefix, RunIndex);
				return true;
			}
			else if (SlotType == TEXT("status") && SlotNameText == TEXT("EffectStatus") && Effect)
			{
				const FGameplayTag StatusTag = ResolveEffectStatusTag(*Effect);
				if (StatusTag.IsValid())
				{
					AddStatusRun(Block, StatusTag, StableIdPrefix, RunIndex);
					return true;
				}
			}
			else if (SlotType == TEXT("keyword") && SlotNameText == TEXT("Tag"))
			{
				if (Effect && Effect->Target.IsValid())
				{
					AddKeywordRun(Block, Effect->Target, StableIdPrefix, RunIndex);
					return true;
				}
				if (Passive && Passive->Trigger.IsValid())
				{
					AddKeywordRun(Block, Passive->Trigger, StableIdPrefix, RunIndex);
					return true;
				}
			}

			return false;
		}

		void CompileTemplate(
			FWacomCardDetailBlock& Block,
			const FText& Template,
			const UCardDefinition* Card,
			const FCardEffect* Effect,
			const FCardPassive* Passive,
			const FWacomCardPresentationRuntimeContext& RuntimeContext,
			const FWacomCardPresentationRuntimeContext::FEffectPreview* Preview,
			const FString& StableIdPrefix)
		{
			const FString Source = Template.ToString();
			int32 Cursor = 0;
			int32 RunIndex = 0;
			while (Cursor < Source.Len())
			{
				const int32 OpenIndex = Source.Find(TEXT("{"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Cursor);
				if (OpenIndex == INDEX_NONE)
				{
					AddTextRun(Block, Source.Mid(Cursor), StableIdPrefix, RunIndex);
					break;
				}

				AddTextRun(Block, Source.Mid(Cursor, OpenIndex - Cursor), StableIdPrefix, RunIndex);

				const int32 CloseIndex = Source.Find(TEXT("}"), ESearchCase::CaseSensitive, ESearchDir::FromStart, OpenIndex + 1);
				if (CloseIndex == INDEX_NONE)
				{
					AddTextRun(Block, Source.Mid(OpenIndex), StableIdPrefix, RunIndex);
					break;
				}

				const FString Slot = Source.Mid(OpenIndex + 1, CloseIndex - OpenIndex - 1);
				if (!TryAppendSlot(
					Block,
					Slot,
					Card,
					Effect,
					Passive,
					RuntimeContext,
					Preview,
					StableIdPrefix,
					RunIndex))
				{
					AddTextRun(Block, Source.Mid(OpenIndex, CloseIndex - OpenIndex + 1), StableIdPrefix, RunIndex);
				}

				Cursor = CloseIndex + 1;
			}
		}

		FText ResolveEffectTemplate(
			const FCardEffect& Effect,
			const UWacomCardExplanationLexicon* Lexicon)
		{
			FWacomCardExplanationTemplateEntry Entry;
			if (Lexicon && Lexicon->FindEffectTemplate(Effect.EffectType, Entry))
			{
				return Entry.Template;
			}
			if (Lexicon)
			{
				UE_LOG(
					LogWacomCardExplanation,
					Verbose,
					TEXT("Missing card effect explanation template for '%s'; using generated fallback."),
					*Effect.EffectType.ToString());
			}
			return DefaultEffectTemplate(Effect);
		}

		FText ResolvePassiveTriggerTemplate(
			const FCardPassive& Passive,
			const UWacomCardExplanationLexicon* Lexicon)
		{
			FWacomCardExplanationTemplateEntry Entry;
			if (Lexicon && Lexicon->FindPassiveTriggerTemplate(Passive.Trigger, Entry))
			{
				return Entry.Template;
			}
			if (Lexicon)
			{
				UE_LOG(
					LogWacomCardExplanation,
					Verbose,
					TEXT("Missing card passive trigger explanation template for '%s'; using generated fallback."),
					*Passive.Trigger.ToString());
			}
			return DefaultPassiveTriggerTemplate(Passive);
		}
	}

	FWacomCardDetailBlock BuildEffectBlock(
		const UCardDefinition* Card,
		const FCardEffect& Effect,
		const FWacomCardPresentationRuntimeContext& RuntimeContext,
		const UWacomCardExplanationLexicon* Lexicon,
		int32 EffectIndex,
		const FString& StableIdPrefix,
		EWacomCardDetailBlockKind BlockKind)
	{
		FWacomCardDetailBlock Block;
		Block.BlockId = FName(*FString::Printf(TEXT("%s.Block"), *StableIdPrefix));
		Block.Kind = BlockKind;

		const FWacomCardPresentationRuntimeContext::FEffectPreview* Preview =
			FindEffectPreview(RuntimeContext, EffectIndex);
		Block.bSkipped = Preview && Preview->bSkip;

		CompileTemplate(
			Block,
			ResolveEffectTemplate(Effect, Lexicon),
			Card,
			&Effect,
			nullptr,
			RuntimeContext,
			Preview,
			StableIdPrefix);
		return Block;
	}

	FWacomCardDetailBlock BuildPassiveTriggerBlock(
		const FCardPassive& Passive,
		const UWacomCardExplanationLexicon* Lexicon,
		int32 PassiveIndex)
	{
		const FString StableIdPrefix = FString::Printf(TEXT("Passive.%d.Trigger"), PassiveIndex);
		FWacomCardDetailBlock Block;
		Block.BlockId = FName(*FString::Printf(TEXT("%s.Block"), *StableIdPrefix));
		Block.Kind = EWacomCardDetailBlockKind::PassiveTrigger;

		FWacomCardPresentationRuntimeContext EmptyContext;
		CompileTemplate(
			Block,
			ResolvePassiveTriggerTemplate(Passive, Lexicon),
			nullptr,
			nullptr,
			&Passive,
			EmptyContext,
			nullptr,
			StableIdPrefix);

		if (Passive.Condition.IsSet())
		{
			int32 RunIndex = Block.Runs.Num();
			AddTextRun(Block, TEXT("（有条件）"), StableIdPrefix, RunIndex);
		}
		return Block;
	}

	void AddCardDetailSection(
		FWacomCardDetailViewData& Data,
		FName SectionId,
		EWacomCardDetailSectionKind Kind,
		const FText& Title,
		TArray<FWacomCardDetailBlock>&& Blocks)
	{
		TArray<FWacomCardDetailBlock> NonEmptyBlocks;
		for (FWacomCardDetailBlock& Block : Blocks)
		{
			if (!Block.Runs.IsEmpty())
			{
				NonEmptyBlocks.Add(MoveTemp(Block));
			}
		}
		if (NonEmptyBlocks.IsEmpty())
		{
			return;
		}

		FWacomCardDetailSection Section;
		Section.SectionId = SectionId;
		Section.Kind = Kind;
		Section.Title = Title;
		Section.Blocks = MoveTemp(NonEmptyBlocks);
		Data.Sections.Add(MoveTemp(Section));
	}
}

#undef LOCTEXT_NAMESPACE
