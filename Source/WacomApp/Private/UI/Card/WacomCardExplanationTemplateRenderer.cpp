// Copyright Wacom. All Rights Reserved.

#include "WacomCardExplanationTemplateRenderer.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Cards/CardPassive.h"
#include "Tags/WacomGameplayTags.h"
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
			Run.Text = WacomCardExplanationText::GetDisplayStatusName(StatusTag);
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
			Run.Text = FText::FromString(WacomCardExplanationText::GetDisplayTagLeafName(Tag));
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
	}

	void AppendTextRun(
		FWacomCardDetailBlock& Block,
		const FString& Text,
		const FString& StableIdPrefix,
		int32& RunIndex)
	{
		AddTextRun(Block, Text, StableIdPrefix, RunIndex);
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
}

#undef LOCTEXT_NAMESPACE
