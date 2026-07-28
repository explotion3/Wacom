// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleStatusTooltipPresentation.h"

#include "Statuses/BattleStatusRuleConstants.h"
#include "UI/Battle/WacomBattleStatusPresentationCatalog.h"
#include "UI/Battle/WacomBattleStatusIconWidget.h"
#include "UI/Battle/WacomBattleStatusPresentationCatalogProvider.h"

#define LOCTEXT_NAMESPACE "WacomBattleStatusTooltipPresentation"

namespace
{
	FText FormatRuleTemplate(const FText& Template)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(
			TEXT("PoisonDamagePerStack"),
			FText::AsNumber(
				WacomBattleStatusRuleConstants::PoisonDamagePerStack));
		Arguments.Add(
			TEXT("PlayerHealPoisonRemovalPercent"),
			FText::AsNumber(FMath::RoundToInt(
				WacomBattleStatusRuleConstants::PlayerHealPoisonRemovalRatio
				* 100.0f)));
		return FText::Format(Template, Arguments);
	}

	void ApplyRules(
		FWacomBattleStatusIconView& View,
		const FWacomBattleStatusRuleTextSet& Rules)
	{
		View.CoreEffectText = FormatRuleTemplate(Rules.CoreEffectText);
		View.TriggerTimingText = FormatRuleTemplate(Rules.TriggerTimingText);
		View.StackPolicyText = FormatRuleTemplate(Rules.StackPolicyText);
	}
}

void FWacomBattleStatusTooltipPresentationBuilder::PopulateRuleText(
	FWacomBattleStatusIconView& InOutView)
{
	const UWacomBattleStatusPresentationCatalog& Catalog =
		WacomBattleStatusPresentationCatalogProvider::GetCatalog();
	const FWacomBattleStatusPresentationEntry* Entry =
		Catalog.FindEntry(InOutView.StatusTag);
	if (!Entry)
	{
		ApplyRules(InOutView, Catalog.UnknownRules);
		return;
	}

	if (InOutView.InspectionHost == EWacomBattleStatusInspectionHost::Player)
	{
		ApplyRules(InOutView, Entry->PlayerRules);
		return;
	}
	if (InOutView.InspectionHost == EWacomBattleStatusInspectionHost::EnemyPart)
	{
		ApplyRules(InOutView, Entry->EnemyPartRules);
		return;
	}
	if (InOutView.InspectionHost == EWacomBattleStatusInspectionHost::Card)
	{
		ApplyRules(InOutView, Entry->CardRules.IsComplete()
			? Entry->CardRules
			: Catalog.UnknownRules);
		return;
	}
	ApplyRules(InOutView, Catalog.UnknownRules);
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
