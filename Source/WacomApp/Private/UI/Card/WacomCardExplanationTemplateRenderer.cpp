// Copyright Wacom. All Rights Reserved.

#include "WacomCardExplanationTemplateRenderer.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Cards/CardExplanationTemplateContract.h"
#include "Cards/CardPassive.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Card/WacomCardExplanationLexicon.h"
#include "WacomCardExplanationLexiconProvider.h"
#include "WacomCardExplanationText.h"

#define LOCTEXT_NAMESPACE "WacomCardExplanationTemplateRenderer"

namespace WacomCardExplanationTemplateRenderer
{
	namespace
	{
		bool UsesRuntimeCostMagnitude(const FCardEffect& Effect)
		{
			return Effect.MagnitudeSource.MatchesTagExact(WacomTags::Magnitude_Source_RuntimeCost)
				|| (!Effect.MagnitudeSource.IsValid() && Effect.bMagnitudeFromRuntimeCost);
		}

		int32 GetBaseDisplayMagnitude(
			const UCardDefinition* Card,
			const FCardEffect& Effect,
			const FWacomCardPresentationRuntimeContext& RuntimeContext,
			const int32 EffectIndex)
		{
			if (EffectIndex != INDEX_NONE)
			{
				for (const FWacomCardPresentationRuntimeContext::FCurrentEffectMagnitude&
					Magnitude : RuntimeContext.CurrentEffectMagnitudes)
				{
					if (Magnitude.EffectIndex == EffectIndex)
					{
						return Magnitude.Magnitude;
					}
				}
			}
			if (UsesRuntimeCostMagnitude(Effect))
			{
				if (RuntimeContext.bHasRuntimeCost)
				{
					return RuntimeContext.RuntimeCost;
				}
				const EWacomCardUpgradeTier Tier = RuntimeContext.bHasUpgradeTier
					? RuntimeContext.UpgradeTier
					: EWacomCardUpgradeTier::White;
				return Card ? Card->ResolveBaseCost(Tier) : Effect.Magnitude;
			}
			return Effect.Magnitude;
		}

		FGameplayTag ResolveMagnitudeSourceTag(const FCardEffect& Effect)
		{
			if (Effect.MagnitudeSource.IsValid())
			{
				return Effect.MagnitudeSource;
			}
			if (Effect.bMagnitudeFromRuntimeCost)
			{
				return WacomTags::Magnitude_Source_RuntimeCost;
			}
			return FGameplayTag();
		}

		FText ResolveMagnitudeSourceText(
			const FCardEffect& Effect,
			const UWacomCardExplanationLexicon* Lexicon)
		{
			const FGameplayTag SourceTag = ResolveMagnitudeSourceTag(Effect);
			if (!SourceTag.IsValid() || SourceTag.MatchesTagExact(WacomTags::Magnitude_Source_Literal))
			{
				return FText::GetEmpty();
			}

			FText Template;
			FWacomCardExplanationTemplateEntry Entry;
			if (Lexicon && Lexicon->FindMagnitudeSourceTemplate(SourceTag, Entry))
			{
				Template = Entry.Template;
			}
			else if (SourceTag.MatchesTagExact(WacomTags::Magnitude_Source_RuntimeCost))
			{
				Template = LOCTEXT("MagnitudeSourceRuntimeCostFallback", "相当于当前费用");
			}
			else if (SourceTag.MatchesTagExact(WacomTags::Magnitude_Source_TargetStatusStacks))
			{
				Template = LOCTEXT("MagnitudeSourceTargetStatusStacksFallback", "相当于目标{Status}层数");
			}
			else if (SourceTag.MatchesTagExact(WacomTags::Magnitude_Source_HandCount))
			{
				Template = LOCTEXT("MagnitudeSourceHandCountFallback", "相当于当前手牌数量");
			}

			if (Template.IsEmpty())
			{
				return FText::GetEmpty();
			}

			FFormatNamedArguments Args;
			Args.Add(
				TEXT("Status"),
				WacomCardExplanationText::GetDisplayStatusName(Effect.TargetZone, Lexicon));
			return FText::Format(Template, Args);
		}

		FName StableRunId(const FString& Prefix, int32 RunIndex, const FString& Suffix)
		{
			return FName(*FString::Printf(TEXT("%s.Run.%d.%s"), *Prefix, RunIndex, *Suffix));
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
			FGameplayTag ValueSourceTag,
			const FText& ValueSourceText,
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
			Run.ValueSourceTag = ValueSourceTag;
			if (!ValueSourceText.IsEmpty())
			{
				Run.ValueSourceText = ValueSourceText;
				Run.bHasValueSourceText = true;
			}
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
			const UWacomCardExplanationLexicon* Lexicon,
			const FString& StableIdPrefix,
			int32& RunIndex)
		{
			FWacomCardDetailRun Run;
			Run.StableId = StableRunId(StableIdPrefix, RunIndex, TEXT("Status"));
			Run.Kind = EWacomCardDetailRunKind::Status;
			Run.Tag = StatusTag;
			Run.Text = WacomCardExplanationText::GetDisplayStatusName(StatusTag, Lexicon);
			Run.bSkipped = Block.bSkipped;
			Block.Runs.Add(MoveTemp(Run));
			++RunIndex;
		}

		void AddKeywordRun(
			FWacomCardDetailBlock& Block,
			FGameplayTag Tag,
			const UWacomCardExplanationLexicon* Lexicon,
			const FString& StableIdPrefix,
			int32& RunIndex)
		{
			FWacomCardDetailRun Run;
			Run.StableId = StableRunId(StableIdPrefix, RunIndex, TEXT("Keyword"));
			Run.Kind = EWacomCardDetailRunKind::Keyword;
			Run.Tag = Tag;
			FWacomCardFaceSemanticLexiconEntry SemanticEntry;
			Run.Text =
				WacomCardExplanationLexiconProvider::FindCardFaceSemantic(
					Tag.GetTagName(),
					Tag,
					SemanticEntry)
				&& !SemanticEntry.DisplayName.IsEmpty()
				? SemanticEntry.DisplayName
				: WacomCardExplanationText::GetDisplayTagName(Tag, Lexicon);
			Run.bSkipped = Block.bSkipped;
			Block.Runs.Add(MoveTemp(Run));
			++RunIndex;
		}

		bool TryAppendSlot(
			FWacomCardDetailBlock& Block,
			const FString& Slot,
			const EWacomCardExplanationTemplateContext TemplateContext,
			const UCardDefinition* Card,
			const FCardEffect* Effect,
			const FCardPassive* Passive,
			const int32 EffectIndex,
			const FGameplayTag Keyword,
			const FWacomCardDynamicCostRule* DynamicCostRule,
			const FWacomCardPresentationRuntimeContext& RuntimeContext,
			const FWacomCardPresentationRuntimeContext::FEffectPreview* Preview,
			const UWacomCardExplanationLexicon* Lexicon,
			const FString& StableIdPrefix,
			int32& RunIndex)
		{
			FWacomCardExplanationTemplateSlot ParsedSlot;
			if (!WacomCardExplanationTemplateContract::TryParseSlot(
				Slot,
				TemplateContext,
				ParsedSlot))
			{
				return false;
			}

			switch (ParsedSlot.Kind)
			{
			case EWacomCardExplanationTemplateSlotKind::EffectMagnitude:
				if (Effect)
				{
					AddValueRun(
						Block,
						ParsedSlot.SlotName,
						GetBaseDisplayMagnitude(
							Card,
							*Effect,
							RuntimeContext,
							EffectIndex),
						ResolveMagnitudeSourceTag(*Effect),
						ResolveMagnitudeSourceText(*Effect, Lexicon),
						Preview,
						StableIdPrefix,
						RunIndex);
					return true;
				}
				break;
			case EWacomCardExplanationTemplateSlotKind::EffectIcon:
				if (Effect)
				{
					AddIconRun(
						Block,
						ResolveEffectIcon(*Effect),
						StableIdPrefix,
						RunIndex);
					return true;
				}
				break;
			case EWacomCardExplanationTemplateSlotKind::EffectStatus:
				if (Effect)
				{
					const FGameplayTag StatusTag =
						WacomCardExplanationTemplateContract::ResolveEffectStatusTag(*Effect);
					if (StatusTag.IsValid())
					{
						AddStatusRun(
							Block,
							StatusTag,
							Lexicon,
							StableIdPrefix,
							RunIndex);
						return true;
					}
				}
				break;
			case EWacomCardExplanationTemplateSlotKind::EffectTag:
				if (Effect && Effect->Target.IsValid())
				{
					AddKeywordRun(Block, Effect->Target, Lexicon, StableIdPrefix, RunIndex);
					return true;
				}
				break;
			case EWacomCardExplanationTemplateSlotKind::TriggerThreshold:
				if (Passive)
				{
					AddValueRun(
						Block,
						ParsedSlot.SlotName,
						Passive->TriggerThreshold,
						FGameplayTag(),
						FText::GetEmpty(),
						nullptr,
						StableIdPrefix,
						RunIndex);
					return true;
				}
				break;
			case EWacomCardExplanationTemplateSlotKind::PassiveEffectMagnitude:
			case EWacomCardExplanationTemplateSlotKind::PassiveEffectIcon:
			case EWacomCardExplanationTemplateSlotKind::PassiveEffectStatus:
				if (!Passive
					|| !Passive->Effects.IsValidIndex(ParsedSlot.PassiveEffectIndex))
				{
					break;
				}

				{
					const FCardEffect& PassiveEffect =
						Passive->Effects[ParsedSlot.PassiveEffectIndex];
					if (ParsedSlot.Kind
						== EWacomCardExplanationTemplateSlotKind::PassiveEffectMagnitude)
					{
						AddValueRun(
							Block,
							ParsedSlot.SlotName,
							GetBaseDisplayMagnitude(
								Card,
								PassiveEffect,
								RuntimeContext,
								INDEX_NONE),
							ResolveMagnitudeSourceTag(PassiveEffect),
							ResolveMagnitudeSourceText(PassiveEffect, Lexicon),
							nullptr,
							StableIdPrefix,
							RunIndex);
						return true;
					}
					if (ParsedSlot.Kind
						== EWacomCardExplanationTemplateSlotKind::PassiveEffectIcon)
					{
						AddIconRun(
							Block,
							ResolveEffectIcon(PassiveEffect),
							StableIdPrefix,
							RunIndex);
						return true;
					}

					const FGameplayTag StatusTag =
						WacomCardExplanationTemplateContract::ResolveEffectStatusTag(
							PassiveEffect);
					if (StatusTag.IsValid())
					{
						AddStatusRun(
							Block,
							StatusTag,
							Lexicon,
							StableIdPrefix,
							RunIndex);
						return true;
					}
				}
				break;
			case EWacomCardExplanationTemplateSlotKind::Keyword:
				if (Keyword.IsValid())
				{
					AddKeywordRun(
						Block,
						Keyword,
						Lexicon,
						StableIdPrefix,
						RunIndex);
					return true;
				}
				break;
			case EWacomCardExplanationTemplateSlotKind::DynamicCostStatus:
				if (DynamicCostRule
					&& DynamicCostRule->CountHandCardsWithStatus.IsValid())
				{
					AddStatusRun(
						Block,
						DynamicCostRule->CountHandCardsWithStatus,
						Lexicon,
						StableIdPrefix,
						RunIndex);
					return true;
				}
				break;
			case EWacomCardExplanationTemplateSlotKind::
				DynamicCostReductionPerMatchingCard:
				if (DynamicCostRule)
				{
					AddValueRun(
						Block,
						ParsedSlot.SlotName,
						DynamicCostRule->ReductionPerMatchingCard,
						FGameplayTag(),
						FText::GetEmpty(),
						nullptr,
						StableIdPrefix,
						RunIndex);
					return true;
				}
				break;
			case EWacomCardExplanationTemplateSlotKind::DynamicCostMinimumCost:
				if (DynamicCostRule)
				{
					AddValueRun(
						Block,
						ParsedSlot.SlotName,
						DynamicCostRule->MinimumCost,
						FGameplayTag(),
						FText::GetEmpty(),
						nullptr,
						StableIdPrefix,
						RunIndex);
					return true;
				}
				break;
			}

			return false;
		}
	}

	void AppendTextRun(
		FWacomCardDetailBlock& Block,
		const FString& Text,
		const FString& StableIdPrefix,
		int32& RunIndex)
	{
		AddTextRun(Block, Text, StableIdPrefix, RunIndex);
	}

	void AppendMutedRun(
		FWacomCardDetailBlock& Block,
		const FText& Text,
		const FString& StableIdPrefix,
		int32& RunIndex)
	{
		if (Text.IsEmpty())
		{
			return;
		}

		FWacomCardDetailRun Run;
		Run.StableId = StableRunId(StableIdPrefix, RunIndex, TEXT("Muted"));
		Run.Kind = EWacomCardDetailRunKind::Muted;
		Run.Text = Text;
		Run.bSkipped = Block.bSkipped;
		Block.Runs.Add(MoveTemp(Run));
		++RunIndex;
	}

	void CompileTemplate(
		FWacomCardDetailBlock& Block,
		const FText& Template,
		const UCardDefinition* Card,
		const FCardEffect* Effect,
		const FCardPassive* Passive,
		const int32 EffectIndex,
		const FWacomCardPresentationRuntimeContext& RuntimeContext,
		const FWacomCardPresentationRuntimeContext::FEffectPreview* Preview,
		const UWacomCardExplanationLexicon* Lexicon,
		const FString& StableIdPrefix)
	{
		const EWacomCardExplanationTemplateContext TemplateContext = Effect
			? EWacomCardExplanationTemplateContext::Effect
			: EWacomCardExplanationTemplateContext::Passive;
		const FString Source = Template.ToString();
		int32 Cursor = 0;
		int32 RunIndex = Block.Runs.Num();
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
				TemplateContext,
				Card,
				Effect,
				Passive,
				EffectIndex,
				FGameplayTag(),
				nullptr,
				RuntimeContext,
				Preview,
				Lexicon,
				StableIdPrefix,
				RunIndex))
			{
				AddTextRun(Block, Source.Mid(OpenIndex, CloseIndex - OpenIndex + 1), StableIdPrefix, RunIndex);
			}

			Cursor = CloseIndex + 1;
		}
	}

	void CompileKeywordTemplate(
		FWacomCardDetailBlock& Block,
		const FText& Template,
		const FGameplayTag Keyword,
		const UWacomCardExplanationLexicon* Lexicon,
		const FString& StableIdPrefix)
	{
		const FString Source = Template.ToString();
		int32 Cursor = 0;
		int32 RunIndex = Block.Runs.Num();
		while (Cursor < Source.Len())
		{
			const int32 OpenIndex = Source.Find(
				TEXT("{"),
				ESearchCase::CaseSensitive,
				ESearchDir::FromStart,
				Cursor);
			if (OpenIndex == INDEX_NONE)
			{
				AddTextRun(Block, Source.Mid(Cursor), StableIdPrefix, RunIndex);
				break;
			}
			AddTextRun(
				Block,
				Source.Mid(Cursor, OpenIndex - Cursor),
				StableIdPrefix,
				RunIndex);
			const int32 CloseIndex = Source.Find(
				TEXT("}"),
				ESearchCase::CaseSensitive,
				ESearchDir::FromStart,
				OpenIndex + 1);
			if (CloseIndex == INDEX_NONE)
			{
				AddTextRun(
					Block,
					Source.Mid(OpenIndex),
					StableIdPrefix,
					RunIndex);
				break;
			}
			const FString Slot =
				Source.Mid(OpenIndex + 1, CloseIndex - OpenIndex - 1);
			const FWacomCardPresentationRuntimeContext EmptyContext;
			if (!TryAppendSlot(
				Block,
				Slot,
				EWacomCardExplanationTemplateContext::Keyword,
				nullptr,
				nullptr,
				nullptr,
				INDEX_NONE,
				Keyword,
				nullptr,
				EmptyContext,
				nullptr,
				Lexicon,
				StableIdPrefix,
				RunIndex))
			{
				AddTextRun(
					Block,
					Source.Mid(OpenIndex, CloseIndex - OpenIndex + 1),
					StableIdPrefix,
					RunIndex);
			}
			Cursor = CloseIndex + 1;
		}
	}

	void CompileDynamicCostTemplate(
		FWacomCardDetailBlock& Block,
		const FText& Template,
		const FWacomCardDynamicCostRule& DynamicCostRule,
		const UWacomCardExplanationLexicon* Lexicon,
		const FString& StableIdPrefix)
	{
		const FString Source = Template.ToString();
		int32 Cursor = 0;
		int32 RunIndex = Block.Runs.Num();
		while (Cursor < Source.Len())
		{
			const int32 OpenIndex = Source.Find(
				TEXT("{"),
				ESearchCase::CaseSensitive,
				ESearchDir::FromStart,
				Cursor);
			if (OpenIndex == INDEX_NONE)
			{
				AddTextRun(Block, Source.Mid(Cursor), StableIdPrefix, RunIndex);
				break;
			}
			AddTextRun(
				Block,
				Source.Mid(Cursor, OpenIndex - Cursor),
				StableIdPrefix,
				RunIndex);
			const int32 CloseIndex = Source.Find(
				TEXT("}"),
				ESearchCase::CaseSensitive,
				ESearchDir::FromStart,
				OpenIndex + 1);
			if (CloseIndex == INDEX_NONE)
			{
				AddTextRun(
					Block,
					Source.Mid(OpenIndex),
					StableIdPrefix,
					RunIndex);
				break;
			}
			const FString Slot =
				Source.Mid(OpenIndex + 1, CloseIndex - OpenIndex - 1);
			const FWacomCardPresentationRuntimeContext EmptyContext;
			if (!TryAppendSlot(
				Block,
				Slot,
				EWacomCardExplanationTemplateContext::DynamicCost,
				nullptr,
				nullptr,
				nullptr,
				INDEX_NONE,
				FGameplayTag(),
				&DynamicCostRule,
				EmptyContext,
				nullptr,
				Lexicon,
				StableIdPrefix,
				RunIndex))
			{
				AddTextRun(
					Block,
					Source.Mid(OpenIndex, CloseIndex - OpenIndex + 1),
					StableIdPrefix,
					RunIndex);
			}
			Cursor = CloseIndex + 1;
		}
	}
}

#undef LOCTEXT_NAMESPACE
