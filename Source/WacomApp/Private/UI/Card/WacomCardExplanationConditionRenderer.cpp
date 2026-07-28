// Copyright Wacom. All Rights Reserved.

#include "WacomCardExplanationConditionRenderer.h"

#include "Cards/CardEffect.h"
#include "Cards/EffectCondition.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Card/WacomCardExplanationLexicon.h"
#include "WacomCardExplanationTemplateRenderer.h"
#include "WacomCardExplanationText.h"

#define LOCTEXT_NAMESPACE "WacomCardExplanationConditionRenderer"

namespace WacomCardExplanationConditionRenderer
{
	namespace
	{
		FText FormatLexiconText(
			const UWacomCardExplanationLexicon* Lexicon,
			FName Key,
			const FText& Fallback,
			const FText& Arg0)
		{
			FFormatOrderedArguments Args;
			Args.Add(Arg0);
			return WacomCardExplanationText::FormatNamedText(Lexicon, Key, Fallback, Args);
		}

		FText BuildConditionBodyText(
			const FEffectCondition& Condition,
			const UWacomCardExplanationLexicon* Lexicon)
		{
			if (Condition.ConditionType.MatchesTagExact(WacomTags::Condition_Self_InZone))
			{
				const FText ZoneName = Condition.ParamTag.IsValid()
					? WacomCardExplanationText::GetDisplayHandZoneName(Condition.ParamTag, Lexicon)
					: WacomCardExplanationText::ResolveNamedText(
						Lexicon,
						WacomCardExplanationLexiconKeys::ConditionUnknownHandZone,
						LOCTEXT("UnknownHandZone", "指定区域"));
				return Condition.bNegate
					? FormatLexiconText(
						Lexicon,
						WacomCardExplanationLexiconKeys::ConditionSelfNotInZone,
						LOCTEXT("SelfNotInZone", "仅当本卡不在{0}时"),
						ZoneName)
					: FormatLexiconText(
						Lexicon,
						WacomCardExplanationLexiconKeys::ConditionSelfInZone,
						LOCTEXT("SelfInZone", "仅当本卡在{0}时"),
						ZoneName);
			}

			if (Condition.ConditionType.MatchesTagExact(WacomTags::Condition_Target_HasStatus))
			{
				const FText StatusName = Condition.ParamTag.IsValid()
					? WacomCardExplanationText::GetDisplayStatusName(Condition.ParamTag, Lexicon)
					: WacomCardExplanationText::ResolveNamedText(
						Lexicon,
						WacomCardExplanationLexiconKeys::ConditionUnknownStatus,
						LOCTEXT("UnknownStatus", "指定状态"));
				return Condition.bNegate
					? FormatLexiconText(
						Lexicon,
						WacomCardExplanationLexiconKeys::ConditionTargetHasNoStatus,
						LOCTEXT("TargetHasNoStatus", "仅当目标没有{0}时"),
						StatusName)
					: FormatLexiconText(
						Lexicon,
						WacomCardExplanationLexiconKeys::ConditionTargetHasStatus,
						LOCTEXT("TargetHasStatus", "仅当目标有{0}时"),
						StatusName);
			}

			if (Condition.ConditionType.MatchesTagExact(
				WacomTags::Condition_Self_InCardLocation))
			{
				const FText LocationName = Condition.ParamTag.IsValid()
					? WacomCardExplanationText::GetDisplayTagName(
						Condition.ParamTag,
						Lexicon)
					: LOCTEXT("UnknownCardLocation", "指定牌堆");
				return Condition.bNegate
					? FormatLexiconText(
						Lexicon,
						WacomCardExplanationLexiconKeys::ConditionSelfNotInCardLocation,
						LOCTEXT("SelfNotInCardLocation", "仅当本卡不位于{0}时"),
						LocationName)
					: FormatLexiconText(
						Lexicon,
						WacomCardExplanationLexiconKeys::ConditionSelfInCardLocation,
						LOCTEXT("SelfInCardLocation", "仅当本卡位于{0}时"),
						LocationName);
			}

			if (Condition.ConditionType.MatchesTagExact(
				WacomTags::Condition_Self_EverEnteredExhaust))
			{
				return Condition.bNegate
					? WacomCardExplanationText::ResolveNamedText(
						Lexicon,
						WacomCardExplanationLexiconKeys::ConditionSelfNeverEnteredExhaust,
						LOCTEXT("SelfNeverEnteredExhaust", "仅当本卡本场从未进入过消耗区时"))
					: WacomCardExplanationText::ResolveNamedText(
						Lexicon,
						WacomCardExplanationLexiconKeys::ConditionSelfEverEnteredExhaust,
						LOCTEXT("SelfEverEnteredExhaust", "仅当本卡本场曾进入过消耗区时"));
			}

			FString ConditionSummary = WacomCardExplanationText::GetDisplayTagLeafName(Condition.ConditionType);
			if (Condition.ParamTag.IsValid())
			{
				ConditionSummary += FString::Printf(
					TEXT(":%s"),
					*WacomCardExplanationText::GetDisplayTagLeafName(Condition.ParamTag));
			}
			if (Condition.ParamInt != 0)
			{
				ConditionSummary += FString::Printf(TEXT(":%d"), Condition.ParamInt);
			}

			const FText FallbackCondition = FText::FromString(ConditionSummary);
			return Condition.bNegate
				? FormatLexiconText(
					Lexicon,
					WacomCardExplanationLexiconKeys::ConditionFallbackNegated,
					LOCTEXT("FallbackNegatedCondition", "仅当不满足{0}时"),
					FallbackCondition)
				: FormatLexiconText(
					Lexicon,
					WacomCardExplanationLexiconKeys::ConditionFallback,
					LOCTEXT("FallbackCondition", "仅当满足{0}时"),
					FallbackCondition);
		}

		FText BuildModifierOperationText(
			const FMagnitudeModifier& Modifier,
			const UWacomCardExplanationLexicon* Lexicon)
		{
			const FText ValueText = FText::AsNumber(Modifier.Value);
			switch (Modifier.Op)
			{
			case EMagnitudeModOp::Add:
				return Modifier.Value >= 0
					? FormatLexiconText(
						Lexicon,
						WacomCardExplanationLexiconKeys::ModifierAddPositive,
						LOCTEXT("ModifierAddPositive", "数值 +{0}"),
						ValueText)
					: FormatLexiconText(
						Lexicon,
						WacomCardExplanationLexiconKeys::ModifierAddNegative,
						LOCTEXT("ModifierAddNegative", "数值 {0}"),
						ValueText);
			case EMagnitudeModOp::Multiply:
				return FormatLexiconText(
					Lexicon,
					WacomCardExplanationLexiconKeys::ModifierMultiply,
					LOCTEXT("ModifierMultiply", "数值 x{0}"),
					ValueText);
			default:
				return FormatLexiconText(
					Lexicon,
					WacomCardExplanationLexiconKeys::ModifierUnknown,
					LOCTEXT("ModifierUnknown", "数值修正 {0}"),
					ValueText);
			}
		}

		FText BuildModifierNoteText(
			const FMagnitudeModifier& Modifier,
			const UWacomCardExplanationLexicon* Lexicon)
		{
			const FText OperationText = BuildModifierOperationText(Modifier, Lexicon);
			if (!Modifier.Condition.IsSet())
			{
				return OperationText;
			}

			FFormatOrderedArguments Args;
			Args.Add(BuildConditionBodyText(Modifier.Condition, Lexicon));
			Args.Add(OperationText);
			return WacomCardExplanationText::FormatNamedText(
				Lexicon,
				WacomCardExplanationLexiconKeys::ModifierConditional,
				LOCTEXT("ConditionalModifier", "{0}，{1}"),
				Args);
		}

		FString WrapNoteText(
			const UWacomCardExplanationLexicon* Lexicon,
			const FText& Note)
		{
			FFormatOrderedArguments Args;
			Args.Add(Note);
			return WacomCardExplanationText::FormatNamedText(
				Lexicon,
				WacomCardExplanationLexiconKeys::NoteParenthesized,
				LOCTEXT("ParenthesizedNote", "（{0}）"),
				Args).ToString();
		}
	}

	void AppendConditionRuns(
		FWacomCardDetailBlock& Block,
		const FEffectCondition& Condition,
		const UWacomCardExplanationLexicon* Lexicon,
		const FString& StableIdPrefix,
		int32& RunIndex)
	{
		if (!Condition.IsSet())
		{
			return;
		}

		const FString Note = WrapNoteText(Lexicon, BuildConditionBodyText(Condition, Lexicon));
		WacomCardExplanationTemplateRenderer::AppendTextRun(Block, Note, StableIdPrefix, RunIndex);
	}

	void AppendMagnitudeModifierRuns(
		FWacomCardDetailBlock& Block,
		const TArray<FMagnitudeModifier>& Modifiers,
		const UWacomCardExplanationLexicon* Lexicon,
		const FString& StableIdPrefix,
		int32& RunIndex)
	{
		for (const FMagnitudeModifier& Modifier : Modifiers)
		{
			const FString Note = WrapNoteText(Lexicon, BuildModifierNoteText(Modifier, Lexicon));
			WacomCardExplanationTemplateRenderer::AppendTextRun(Block, Note, StableIdPrefix, RunIndex);
		}
	}
}

#undef LOCTEXT_NAMESPACE
