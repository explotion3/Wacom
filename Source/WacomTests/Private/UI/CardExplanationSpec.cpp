// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Card/WacomCardDetailPlainTextRenderer.h"
#include "UI/Card/WacomCardDetailRichTextRenderer.h"
#include "UI/Card/WacomCardExplanationLexicon.h"
#include "UI/Card/WacomCardPresentationBuilder.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
	const FWacomCardDetailSection* FindSection(
		const FWacomCardDetailViewData& Data,
		EWacomCardDetailSectionKind Kind)
	{
		for (const FWacomCardDetailSection& Section : Data.Sections)
		{
			if (Section.Kind == Kind)
			{
				return &Section;
			}
		}
		return nullptr;
	}

	FString SectionPlainText(
		const FWacomCardDetailViewData& Data,
		EWacomCardDetailSectionKind Kind)
	{
		if (const FWacomCardDetailSection* Section = FindSection(Data, Kind))
		{
			return UWacomCardDetailPlainTextRenderer::RenderSectionPlainText(*Section).ToString();
		}
		return FString();
	}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUICardExplanationLexiconSpec,
	"Wacom.UI.CardExplanation.Lexicon",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUICardExplanationLexiconSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardExplanationLexicon> Lexicon(NewObject<UWacomCardExplanationLexicon>());
	Lexicon->EffectTemplates.Reset();
	Lexicon->PassiveTriggerTemplates.Reset();

	FWacomCardExplanationTemplateEntry ParentEntry;
	ParentEntry.KeyTag = FGameplayTag(WacomTags::Effect_Shuffle_Random).RequestDirectParent();
	ParentEntry.Template = FText::FromString(TEXT("父模板"));
	Lexicon->EffectTemplates.Add(ParentEntry);

	FWacomCardExplanationTemplateEntry ExactEntry;
	ExactEntry.KeyTag = WacomTags::Effect_Shuffle_Random;
	ExactEntry.Template = FText::FromString(TEXT("精确模板"));
	Lexicon->EffectTemplates.Add(ExactEntry);

	FWacomCardExplanationTemplateEntry Found;
	TestTrue(TEXT("Exact effect template wins"),
		Lexicon->FindEffectTemplate(WacomTags::Effect_Shuffle_Random, Found));
	TestEqual(TEXT("Exact template text"), Found.Template.ToString(), TEXT("精确模板"));

	Lexicon->EffectTemplates.RemoveAt(1);
	TestTrue(TEXT("Parent effect template is used as fallback"),
		Lexicon->FindEffectTemplate(WacomTags::Effect_Shuffle_ToRandomZone, Found));
	TestEqual(TEXT("Parent template text"), Found.Template.ToString(), TEXT("父模板"));

	TestFalse(TEXT("Missing effect template reports false"),
		Lexicon->FindEffectTemplate(WacomTags::Effect_Damage, Found));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUICardExplanationEffectBlocksSpec,
	"Wacom.UI.CardExplanation.EffectBlocks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUICardExplanationEffectBlocksSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	Card->CardId = TEXT("Explanation.Effects");
	Card->DisplayName = FText::FromString(TEXT("解释系统效果卡"));

	FCardEffect Damage;
	Damage.EffectType = WacomTags::Effect_Damage;
	Damage.Magnitude = 4;
	Damage.Condition.ConditionType = WacomTags::Condition_Self_InZone;
	Damage.Condition.ParamTag = WacomTags::HandZone_Left;
	FMagnitudeModifier BonusDamage;
	BonusDamage.Condition.ConditionType = WacomTags::Condition_Target_HasStatus;
	BonusDamage.Condition.ParamTag = WacomTags::Status_Poison;
	BonusDamage.Op = EMagnitudeModOp::Add;
	BonusDamage.Value = 3;
	FMagnitudeModifier ZoneMultiplier;
	ZoneMultiplier.Condition.ConditionType = WacomTags::Condition_Self_InZone;
	ZoneMultiplier.Condition.ParamTag = WacomTags::HandZone_Both;
	ZoneMultiplier.Op = EMagnitudeModOp::Multiply;
	ZoneMultiplier.Value = 2;
	Damage.MagnitudeModifiers = { BonusDamage, ZoneMultiplier };
	Card->Effects.Add(Damage);

	FCardEffect Poison;
	Poison.EffectType = WacomTags::Effect_ApplyStatus_Poison;
	Poison.Magnitude = 2;
	Card->Effects.Add(Poison);

	FCardEffect DiscardSelected;
	DiscardSelected.EffectType = WacomTags::Effect_Card_DiscardSelected;
	Card->Effects.Add(DiscardSelected);

	const FWacomCardDetailViewData Data =
		UWacomCardPresentationBuilder::BuildCardDetailViewData(Card.Get());
	const FString Description = SectionPlainText(Data, EWacomCardDetailSectionKind::Description);

	TestTrue(TEXT("Damage effect emits value text"), Description.Contains(TEXT("造成 4 点伤害。")));
	TestTrue(TEXT("Effect condition emits self zone text"), Description.Contains(TEXT("仅当本卡在左手区时")));
	TestTrue(TEXT("Effect modifier emits target status add text"), Description.Contains(TEXT("仅当目标有中毒时，数值 +3")));
	TestTrue(TEXT("Effect modifier emits self zone multiply text"), Description.Contains(TEXT("仅当本卡在双手区时，数值 ×2")));
	TestTrue(TEXT("Poison effect emits status text"), Description.Contains(TEXT("施加 2 层 中毒。")));
	TestTrue(TEXT("Discard selected effect emits action text"), Description.Contains(TEXT("弃置目标手牌。")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUICardExplanationPassiveBlocksSpec,
	"Wacom.UI.CardExplanation.PassiveBlocks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUICardExplanationPassiveBlocksSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	Card->CardId = TEXT("Explanation.Passive");
	Card->DisplayName = FText::FromString(TEXT("解释系统被动卡"));

	FCardPassive Passive;
	Passive.Trigger = WacomTags::Passive_Trigger_OnCompanionCount;
	Passive.TriggerThreshold = 3;
	Passive.Condition.ConditionType = WacomTags::Condition_Target_HasStatus;
	Passive.Condition.ParamTag = WacomTags::Status_Poison;
	Passive.Condition.bNegate = true;
	Passive.DisplayText = FText::FromString(TEXT("这段旧手写说明不应进入详情。"));

	FCardEffect ShuffleSelf;
	ShuffleSelf.EffectType = WacomTags::Effect_Shuffle_ToRandomZone;
	ShuffleSelf.Target = WacomTags::Target_Self;
	Passive.Effects.Add(ShuffleSelf);
	Card->Passives.Add(Passive);

	const FWacomCardDetailViewData Data =
		UWacomCardPresentationBuilder::BuildCardDetailViewData(Card.Get());
	const FString PassiveText = SectionPlainText(Data, EWacomCardDetailSectionKind::Passive);

	TestTrue(TEXT("Passive trigger emits threshold"), PassiveText.Contains(TEXT("每打出 3 张伙伴：")));
	TestTrue(TEXT("Passive condition emits negated target status text"), PassiveText.Contains(TEXT("仅当目标没有中毒时")));
	TestTrue(TEXT("Passive effect emits structured text"), PassiveText.Contains(TEXT("该牌腾挪至随机区域。")));
	TestFalse(TEXT("Passive DisplayText is ignored"), PassiveText.Contains(TEXT("旧手写说明")));
	TestFalse(TEXT("Passive no longer emits vague condition placeholder"), PassiveText.Contains(TEXT("有条件")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUICardExplanationRuntimePreviewSpec,
	"Wacom.UI.CardExplanation.RuntimePreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUICardExplanationRuntimePreviewSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	Card->CardId = TEXT("Explanation.Preview");
	Card->DisplayName = FText::FromString(TEXT("解释系统预览卡"));

	FCardEffect Damage;
	Damage.EffectType = WacomTags::Effect_Damage;
	Damage.Magnitude = 6;
	Card->Effects.Add(Damage);

	FCardEffect Poison;
	Poison.EffectType = WacomTags::Effect_ApplyStatus_Poison;
	Poison.Magnitude = 2;
	Card->Effects.Add(Poison);

	FCardEffect Heal;
	Heal.EffectType = WacomTags::Effect_Heal;
	Heal.Magnitude = 5;
	Card->Effects.Add(Heal);

	FWacomCardPresentationRuntimeContext RuntimeContext;
	FWacomCardPresentationRuntimeContext::FEffectPreview DamagePreview;
	DamagePreview.EffectIndex = 0;
	DamagePreview.bHasMagnitude = true;
	DamagePreview.Magnitude = 8;
	RuntimeContext.EffectPreviews.Add(DamagePreview);

	FWacomCardPresentationRuntimeContext::FEffectPreview PoisonSkip;
	PoisonSkip.EffectIndex = 1;
	PoisonSkip.bSkip = true;
	RuntimeContext.EffectPreviews.Add(PoisonSkip);

	FWacomCardPresentationRuntimeContext::FEffectPreview HealPreview;
	HealPreview.EffectIndex = 2;
	HealPreview.bHasMagnitude = true;
	HealPreview.Magnitude = 3;
	RuntimeContext.EffectPreviews.Add(HealPreview);

	const FWacomCardDetailViewData Data =
		UWacomCardPresentationBuilder::BuildCardDetailViewData(Card.Get(), RuntimeContext);
	const FWacomCardDetailSection* DescriptionSection =
		FindSection(Data, EWacomCardDetailSectionKind::Description);
	TestNotNull(TEXT("Runtime preview emits description section"), DescriptionSection);
	const FString Description = SectionPlainText(Data, EWacomCardDetailSectionKind::Description);

	TestTrue(TEXT("Runtime preview renders final damage value"), Description.Contains(TEXT("造成 8 点伤害。")));
	TestTrue(TEXT("Runtime preview renders final reduced value"), Description.Contains(TEXT("恢复 3 点生命。")));
	TestFalse(TEXT("Runtime preview omits old arrow expression"), Description.Contains(TEXT("6 -> 8")));
	TestTrue(TEXT("Skipped effect gets skipped prefix"), Description.Contains(TEXT("不会生效：施加 2 层 中毒。")));
	if (DescriptionSection)
	{
		const FString RichText =
			UWacomCardDetailRichTextRenderer::RenderSectionRichText(*DescriptionSection).ToString();
		TestTrue(TEXT("Buffed preview value uses buffed rich text style"),
			RichText.Contains(TEXT("<ValueBuffed>8</>")));
		TestTrue(TEXT("Reduced preview value uses nerfed rich text style"),
			RichText.Contains(TEXT("<ValueNerfed>3</>")));
		TestFalse(TEXT("Rich text no longer emits preview arrow style"),
			RichText.Contains(TEXT("PreviewArrow")));
		TestFalse(TEXT("Rich text no longer emits preview value style"),
			RichText.Contains(TEXT("PreviewValue")));
	}

	return true;
}
