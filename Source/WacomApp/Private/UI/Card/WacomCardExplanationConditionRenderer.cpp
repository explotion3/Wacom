// Copyright Wacom. All Rights Reserved.

#include "WacomCardExplanationConditionRenderer.h"

#include "Cards/EffectCondition.h"
#include "Tags/WacomGameplayTags.h"
#include "WacomCardExplanationTemplateRenderer.h"
#include "WacomCardExplanationText.h"

#define LOCTEXT_NAMESPACE "WacomCardExplanationConditionRenderer"

namespace WacomCardExplanationConditionRenderer
{
	namespace
	{
		FText BuildConditionBodyText(const FEffectCondition& Condition)
		{
			if (Condition.ConditionType.MatchesTagExact(WacomTags::Condition_Self_InZone))
			{
				const FText ZoneName = Condition.ParamTag.IsValid()
					? WacomCardExplanationText::GetDisplayHandZoneName(Condition.ParamTag)
					: LOCTEXT("UnknownHandZone", "指定区域");
				return Condition.bNegate
					? FText::Format(LOCTEXT("SelfNotInZone", "仅当本卡不在{0}时"), ZoneName)
					: FText::Format(LOCTEXT("SelfInZone", "仅当本卡在{0}时"), ZoneName);
			}

			if (Condition.ConditionType.MatchesTagExact(WacomTags::Condition_Target_HasStatus))
			{
				const FText StatusName = Condition.ParamTag.IsValid()
					? WacomCardExplanationText::GetDisplayStatusName(Condition.ParamTag)
					: LOCTEXT("UnknownStatus", "指定状态");
				return Condition.bNegate
					? FText::Format(LOCTEXT("TargetHasNoStatus", "仅当目标没有{0}时"), StatusName)
					: FText::Format(LOCTEXT("TargetHasStatus", "仅当目标有{0}时"), StatusName);
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
				? FText::Format(LOCTEXT("FallbackNegatedCondition", "仅当不满足{0}时"), FallbackCondition)
				: FText::Format(LOCTEXT("FallbackCondition", "仅当满足{0}时"), FallbackCondition);
		}
	}

	void AppendConditionRuns(
		FWacomCardDetailBlock& Block,
		const FEffectCondition& Condition,
		const FString& StableIdPrefix,
		int32& RunIndex)
	{
		if (!Condition.IsSet())
		{
			return;
		}

		const FString Note = FString::Printf(
			TEXT("（%s）"),
			*BuildConditionBodyText(Condition).ToString());
		WacomCardExplanationTemplateRenderer::AppendTextRun(Block, Note, StableIdPrefix, RunIndex);
	}
}

#undef LOCTEXT_NAMESPACE
