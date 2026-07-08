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

	int32 CountOccurrences(const FString& Text, const FString& Needle)
	{
		if (Needle.IsEmpty())
		{
			return 0;
		}

		int32 Count = 0;
		int32 SearchStart = 0;
		while (SearchStart < Text.Len())
		{
			const int32 FoundAt = Text.Find(Needle, ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchStart);
			if (FoundAt == INDEX_NONE)
			{
				break;
			}

			++Count;
			SearchStart = FoundAt + Needle.Len();
		}

		return Count;
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
	Lexicon->PassiveOutcomeTemplates.Reset();
	Lexicon->MagnitudeSourceTemplates.Reset();
	Lexicon->TagDisplayNames.Reset();
	Lexicon->NamedTexts.Reset();

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

	FWacomCardExplanationTemplateEntry OutcomeParent;
	OutcomeParent.KeyTag = FGameplayTag(WacomTags::Passive_Trigger_OnCompanionCount).RequestDirectParent();
	OutcomeParent.Template = FText::FromString(TEXT("父被动结果"));
	Lexicon->PassiveOutcomeTemplates.Add(OutcomeParent);

	FWacomCardExplanationTemplateEntry OutcomeExact;
	OutcomeExact.KeyTag = WacomTags::Passive_Trigger_OnCompanionCount;
	OutcomeExact.Template = FText::FromString(TEXT("精确被动结果"));
	Lexicon->PassiveOutcomeTemplates.Add(OutcomeExact);

	TestTrue(TEXT("Exact passive outcome template wins"),
		Lexicon->FindPassiveOutcomeTemplate(WacomTags::Passive_Trigger_OnCompanionCount, Found));
	TestEqual(TEXT("Exact passive outcome text"), Found.Template.ToString(), TEXT("精确被动结果"));

	Lexicon->PassiveOutcomeTemplates.RemoveAt(1);
	TestTrue(TEXT("Parent passive outcome is used as fallback"),
		Lexicon->FindPassiveOutcomeTemplate(WacomTags::Passive_Trigger_OnTwilightTriggered, Found));
	TestEqual(TEXT("Parent passive outcome text"), Found.Template.ToString(), TEXT("父被动结果"));

	Lexicon->PassiveOutcomeTemplates.Reset();
	TestFalse(TEXT("Missing passive outcome reports false"),
		Lexicon->FindPassiveOutcomeTemplate(WacomTags::Passive_Trigger_OnCompanionCount, Found));

	FWacomCardExplanationTemplateEntry SourceParent;
	SourceParent.KeyTag = FGameplayTag(WacomTags::Magnitude_Source_RuntimeCost).RequestDirectParent();
	SourceParent.Template = FText::FromString(TEXT("父数值来源"));
	Lexicon->MagnitudeSourceTemplates.Add(SourceParent);

	FWacomCardExplanationTemplateEntry SourceExact;
	SourceExact.KeyTag = WacomTags::Magnitude_Source_RuntimeCost;
	SourceExact.Template = FText::FromString(TEXT("精确数值来源"));
	Lexicon->MagnitudeSourceTemplates.Add(SourceExact);

	TestTrue(TEXT("Exact magnitude source template wins"),
		Lexicon->FindMagnitudeSourceTemplate(WacomTags::Magnitude_Source_RuntimeCost, Found));
	TestEqual(TEXT("Exact magnitude source text"), Found.Template.ToString(), TEXT("精确数值来源"));

	Lexicon->MagnitudeSourceTemplates.RemoveAt(1);
	TestTrue(TEXT("Parent magnitude source template is used as fallback"),
		Lexicon->FindMagnitudeSourceTemplate(WacomTags::Magnitude_Source_TargetStatusStacks, Found));
	TestEqual(TEXT("Parent magnitude source text"), Found.Template.ToString(), TEXT("父数值来源"));

	Lexicon->MagnitudeSourceTemplates.Reset();
	TestFalse(TEXT("Missing magnitude source template reports false"),
		Lexicon->FindMagnitudeSourceTemplate(WacomTags::Magnitude_Source_RuntimeCost, Found));

	FWacomCardExplanationTagDisplayEntry StatusDisplay;
	StatusDisplay.KeyTag = WacomTags::Status_Poison;
	StatusDisplay.DisplayName = FText::FromString(TEXT("猛毒"));
	Lexicon->TagDisplayNames.Add(StatusDisplay);
	FText DisplayName;
	TestTrue(TEXT("Tag display override can be found"),
		Lexicon->FindTagDisplayName(WacomTags::Status_Poison, DisplayName));
	TestEqual(TEXT("Tag display override text"), DisplayName.ToString(), TEXT("猛毒"));

	FWacomCardExplanationNamedTextEntry NamedText;
	NamedText.Key = WacomCardExplanationLexiconKeys::SectionDescriptionTitle;
	NamedText.Text = FText::FromString(TEXT("规则"));
	Lexicon->NamedTexts.Add(NamedText);
	FText ResolvedText;
	TestTrue(TEXT("Named detail text can be found"),
		Lexicon->FindNamedText(WacomCardExplanationLexiconKeys::SectionDescriptionTitle, ResolvedText));
	TestEqual(TEXT("Named detail text"), ResolvedText.ToString(), TEXT("规则"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUICardExplanationRichTextInlineIconSpec,
	"Wacom.UI.CardExplanation.RichTextInlineIconMarkup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUICardExplanationRichTextInlineIconSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomCardDetailBlock Block;
	Block.BlockId = TEXT("InlineIcon.Block");
	Block.Kind = EWacomCardDetailBlockKind::EffectSentence;

	FWacomCardDetailRun IconRun;
	IconRun.StableId = TEXT("InlineIcon.Icon");
	IconRun.Kind = EWacomCardDetailRunKind::Icon;
	IconRun.Icon = EWacomCardDetailIcon::Damage;
	Block.Runs.Add(IconRun);

	FWacomCardDetailRun TextRun;
	TextRun.StableId = TEXT("InlineIcon.Text");
	TextRun.Kind = EWacomCardDetailRunKind::Text;
	TextRun.Text = FText::FromString(TEXT("造成 "));
	Block.Runs.Add(TextRun);

	FWacomCardDetailRun ValueRun;
	ValueRun.StableId = TEXT("InlineIcon.Value");
	ValueRun.Kind = EWacomCardDetailRunKind::Value;
	ValueRun.Value = 4;
	ValueRun.bHasValue = true;
	Block.Runs.Add(ValueRun);

	FWacomCardDetailRun StatusRun;
	StatusRun.StableId = TEXT("InlineIcon.Status");
	StatusRun.Kind = EWacomCardDetailRunKind::Status;
	StatusRun.Tag = WacomTags::Status_Poison;
	StatusRun.Text = FText::FromString(TEXT("中毒"));
	Block.Runs.Add(StatusRun);

	const FString Plain = UWacomCardDetailPlainTextRenderer::RenderBlockPlainText(Block).ToString();
	TestTrue(TEXT("Plain text keeps icon fallback"),
		Plain.Contains(TEXT("[伤]")));
	TestTrue(TEXT("Plain text keeps status text once"),
		Plain.Contains(TEXT("中毒")));

	const FString RichText = UWacomCardDetailRichTextRenderer::RenderBlockRichText(Block).ToString();
	TestTrue(TEXT("Rich text emits wacom-icon markup"),
		RichText.Contains(TEXT("<wacom-icon id=\"Damage\" label=\"[伤]\"/>")));
	TestTrue(TEXT("Rich text emits wacom-status markup"),
		RichText.Contains(TEXT("<wacom-status tag=\"Status.Poison\" label=\"中毒\"/>")));
	TestTrue(TEXT("Rich text separates status icon from readable text"),
		RichText.Contains(TEXT("<wacom-status tag=\"Status.Poison\" label=\"中毒\"/> <Status>中毒</>")));
	TestTrue(TEXT("Rich text keeps value style"),
		RichText.Contains(TEXT("<Value>4</>")));
	TestTrue(TEXT("Rich text keeps readable status style"),
		RichText.Contains(TEXT("<Status>中毒</>")));

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

	FCardEffect Heal;
	Heal.EffectType = WacomTags::Effect_Heal;
	Heal.Magnitude = 3;
	Card->Effects.Add(Heal);

	FCardEffect Shield;
	Shield.EffectType = WacomTags::Status_Shield;
	Shield.Magnitude = 5;
	Card->Effects.Add(Shield);

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

	TestTrue(TEXT("Damage effect emits value text"),
		Description.Contains(TEXT("造成 4 点")) && Description.Contains(TEXT("伤害。")) && Description.Contains(TEXT("[伤]")));
	TestTrue(TEXT("Heal effect emits value text"),
		Description.Contains(TEXT("恢复 3 点")) && Description.Contains(TEXT("生命。")) && Description.Contains(TEXT("[疗]")));
	TestTrue(TEXT("Shield effect emits value text"),
		Description.Contains(TEXT("获得 5 点")) && Description.Contains(TEXT("护盾。")) && Description.Contains(TEXT("[盾]")));
	TestTrue(TEXT("Effect condition emits self zone text"), Description.Contains(TEXT("仅当本卡在左手区时")));
	TestTrue(TEXT("Effect modifier emits target status add text"), Description.Contains(TEXT("仅当目标有中毒时，数值 +3")));
	TestTrue(TEXT("Effect modifier emits self zone multiply text"), Description.Contains(TEXT("仅当本卡在双手区时，数值 ×2")));
	TestTrue(TEXT("Poison effect emits status text"), Description.Contains(TEXT("施加 2 层 中毒。")));
	TestTrue(TEXT("Discard selected effect emits action text"), Description.Contains(TEXT("弃置目标手牌。")));

	if (const FWacomCardDetailSection* DescriptionSection =
		FindSection(Data, EWacomCardDetailSectionKind::Description))
	{
		const FString RichText =
			UWacomCardDetailRichTextRenderer::RenderSectionRichText(*DescriptionSection).ToString();
		TestTrue(TEXT("Damage template emits effect icon markup"),
			RichText.Contains(TEXT("<wacom-icon id=\"Damage\" label=\"[伤]\"/>")));
		TestTrue(TEXT("Damage template emits value markup"),
			RichText.Contains(TEXT("造成 <Value>4</> 点")) && RichText.Contains(TEXT("伤害。")));
		TestTrue(TEXT("Heal template emits effect icon markup"),
			RichText.Contains(TEXT("<wacom-icon id=\"Heal\" label=\"[疗]\"/>")));
		TestTrue(TEXT("Heal template emits value markup"),
			RichText.Contains(TEXT("恢复 <Value>3</> 点")) && RichText.Contains(TEXT("生命。")));
		TestTrue(TEXT("Shield template emits effect icon markup"),
			RichText.Contains(TEXT("<wacom-icon id=\"Shield\" label=\"[盾]\"/>")));
		TestTrue(TEXT("Shield template emits value markup"),
			RichText.Contains(TEXT("获得 <Value>5</> 点")) && RichText.Contains(TEXT("护盾。")));
	}

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

	FCardPassive AfterPlayed;
	AfterPlayed.Trigger = WacomTags::Passive_Trigger_AfterPlayed;
	AfterPlayed.Effects.Add(ShuffleSelf);
	Card->Passives.Add(AfterPlayed);

	const FWacomCardDetailViewData Data =
		UWacomCardPresentationBuilder::BuildCardDetailViewData(Card.Get());
	const FString PassiveText = SectionPlainText(Data, EWacomCardDetailSectionKind::Passive);

	TestTrue(TEXT("Passive trigger emits threshold"), PassiveText.Contains(TEXT("每打出 3 张伙伴：")));
	TestTrue(TEXT("Passive outcome emits current runtime result"), PassiveText.Contains(TEXT("使此牌回到手中。")));
	TestTrue(TEXT("Passive condition emits negated target status text"), PassiveText.Contains(TEXT("仅当目标没有中毒时")));
	TestTrue(TEXT("Executable passive effect still emits structured text"), PassiveText.Contains(TEXT("打出后：\n该牌腾挪至随机区域。")));
	TestFalse(TEXT("Passive DisplayText is ignored"), PassiveText.Contains(TEXT("旧手写说明")));
	TestEqual(TEXT("Only executable passive effects are rendered"),
		CountOccurrences(PassiveText, TEXT("该牌腾挪至随机区域。")),
		1);
	TestFalse(TEXT("Passive no longer emits vague condition placeholder"), PassiveText.Contains(TEXT("有条件")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUICardExplanationMagnitudeSourceSpec,
	"Wacom.UI.CardExplanation.MagnitudeSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUICardExplanationMagnitudeSourceSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	Card->CardId = TEXT("Explanation.MagnitudeSource");
	Card->DisplayName = FText::FromString(TEXT("解释系统数值来源卡"));
	Card->BaseCost = 0;

	FCardEffect RuntimePoison;
	RuntimePoison.EffectType = WacomTags::Effect_ApplyStatus_Poison;
	RuntimePoison.Magnitude = 99;
	RuntimePoison.MagnitudeSource = WacomTags::Magnitude_Source_RuntimeCost;
	Card->Effects.Add(RuntimePoison);

	FCardEffect LiteralPoison;
	LiteralPoison.EffectType = WacomTags::Effect_ApplyStatus_Poison;
	LiteralPoison.Magnitude = 2;
	Card->Effects.Add(LiteralPoison);

	FCardEffect LegacyRuntimeHeal;
	LegacyRuntimeHeal.EffectType = WacomTags::Effect_Heal;
	LegacyRuntimeHeal.Magnitude = 7;
	LegacyRuntimeHeal.bMagnitudeFromRuntimeCost = true;
	Card->Effects.Add(LegacyRuntimeHeal);

	FCardEffect TargetStacksDamage;
	TargetStacksDamage.EffectType = WacomTags::Effect_Damage;
	TargetStacksDamage.Magnitude = 4;
	TargetStacksDamage.MagnitudeSource = WacomTags::Magnitude_Source_TargetStatusStacks;
	TargetStacksDamage.TargetZone = WacomTags::Status_Poison;
	Card->Effects.Add(TargetStacksDamage);

	FCardEffect RuntimeDraw;
	RuntimeDraw.EffectType = WacomTags::Effect_Draw;
	RuntimeDraw.Magnitude = 99;
	RuntimeDraw.MagnitudeSource = WacomTags::Magnitude_Source_RuntimeCost;
	Card->Effects.Add(RuntimeDraw);

	FWacomCardPresentationRuntimeContext RuntimeContext;
	RuntimeContext.bHasRuntimeCost = true;
	RuntimeContext.RuntimeCost = 2;

	const FWacomCardDetailViewData Data =
		UWacomCardPresentationBuilder::BuildCardDetailViewData(Card.Get(), RuntimeContext);
	const FWacomCardDetailSection* DescriptionSection =
		FindSection(Data, EWacomCardDetailSectionKind::Description);
	TestNotNull(TEXT("Magnitude source detail emits description section"), DescriptionSection);
	const FString Description = SectionPlainText(Data, EWacomCardDetailSectionKind::Description);

	TestTrue(TEXT("RuntimeCost source renders source phrase with current value"),
		Description.Contains(TEXT("施加 相当于当前费用 2 层 中毒。")));
	TestTrue(TEXT("Literal magnitude keeps existing wording"),
		Description.Contains(TEXT("施加 2 层 中毒。")));
	TestTrue(TEXT("Legacy runtime cost flag renders source phrase"),
		Description.Contains(TEXT("恢复 相当于当前费用 2 点")) &&
		Description.Contains(TEXT("生命。")) &&
		Description.Contains(TEXT("[疗]")));
	TestTrue(TEXT("TargetStatusStacks source renders status display name"),
		Description.Contains(TEXT("造成 相当于目标中毒层数 4 点")) &&
		Description.Contains(TEXT("伤害。")) &&
		Description.Contains(TEXT("[伤]")));
	TestTrue(TEXT("RuntimeCost draw renders source phrase with current value"),
		Description.Contains(TEXT("抽 相当于当前费用 2 张牌。")));

	if (DescriptionSection)
	{
		const FString RichText =
			UWacomCardDetailRichTextRenderer::RenderSectionRichText(*DescriptionSection).ToString();
		TestTrue(TEXT("Rich text keeps source phrase outside value style"),
			RichText.Contains(TEXT("相当于当前费用 <Value>2</>")));
		TestTrue(TEXT("Rich text emits inline status icon markup"),
			RichText.Contains(TEXT("<wacom-status tag=\"Status.Poison\" label=\"中毒\"/>")));
		TestTrue(TEXT("Rich text separates inline status icon from status text"),
			RichText.Contains(TEXT("<wacom-status tag=\"Status.Poison\" label=\"中毒\"/> <Status>中毒</>")));
		TestTrue(TEXT("Rich text keeps status styling"),
			RichText.Contains(TEXT("<Status>中毒</>")));
		TestTrue(TEXT("Rich text renders RuntimeCost draw without effect icon"),
			RichText.Contains(TEXT("抽 相当于当前费用 <Value>2</> 张牌。")));
	}

	FWacomCardPresentationRuntimeContext PreviewContext = RuntimeContext;
	FWacomCardPresentationRuntimeContext::FEffectPreview RuntimePoisonPreview;
	RuntimePoisonPreview.EffectIndex = 0;
	RuntimePoisonPreview.bHasMagnitude = true;
	RuntimePoisonPreview.Magnitude = 5;
	PreviewContext.EffectPreviews.Add(RuntimePoisonPreview);

	const FWacomCardDetailViewData PreviewData =
		UWacomCardPresentationBuilder::BuildCardDetailViewData(Card.Get(), PreviewContext);
	const FString PreviewDescription =
		SectionPlainText(PreviewData, EWacomCardDetailSectionKind::Description);
	TestTrue(TEXT("Preview override renders final value without source phrase"),
		PreviewDescription.Contains(TEXT("施加 5 层 中毒。")));
	TestFalse(TEXT("Preview override does not attach stale RuntimeCost source phrase"),
		PreviewDescription.Contains(TEXT("施加 相当于当前费用 5 层 中毒。")));
	if (const FWacomCardDetailSection* PreviewDescriptionSection =
		FindSection(PreviewData, EWacomCardDetailSectionKind::Description))
	{
		const FString RichText =
			UWacomCardDetailRichTextRenderer::RenderSectionRichText(*PreviewDescriptionSection).ToString();
		TestTrue(TEXT("Preview override keeps buffed value style"),
			RichText.Contains(TEXT("<ValueBuffed>5</>")));
		TestFalse(TEXT("Preview override keeps source phrase out of changed preview value"),
			RichText.Contains(TEXT("相当于当前费用 <ValueBuffed>5</>")));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUICardExplanationDescriptionFallbackSpec,
	"Wacom.UI.CardExplanation.DescriptionFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUICardExplanationDescriptionFallbackSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> EmptyDetailCard(NewObject<UCardDefinition>());
	EmptyDetailCard->CardId = TEXT("Explanation.DescriptionFallback");
	EmptyDetailCard->DisplayName = FText::FromString(TEXT("空详情卡"));
	EmptyDetailCard->Description = FText::FromString(TEXT("A 类容器卡，容量为 3。\n携带删牌能力。\n不会解析 {Effect.0}。"));

	const FWacomCardDetailViewData FallbackData =
		UWacomCardPresentationBuilder::BuildCardDetailViewData(EmptyDetailCard.Get());
	const FString FallbackDescription =
		SectionPlainText(FallbackData, EWacomCardDetailSectionKind::Description);
	TestTrue(TEXT("Empty structured detail falls back to Description"),
		FallbackDescription.Contains(TEXT("A 类容器卡，容量为 3。")));
	TestTrue(TEXT("Description fallback keeps authored braces as text"),
		FallbackDescription.Contains(TEXT("{Effect.0}")));

	TStrongObjectPtr<UCardDefinition> StructuredCard(NewObject<UCardDefinition>());
	StructuredCard->CardId = TEXT("Explanation.StructuredIgnoresDescription");
	StructuredCard->DisplayName = FText::FromString(TEXT("结构化卡"));
	StructuredCard->Description = FText::FromString(TEXT("旧描述不应追加。"));
	FCardEffect Damage;
	Damage.EffectType = WacomTags::Effect_Damage;
	Damage.Magnitude = 3;
	StructuredCard->Effects.Add(Damage);

	const FWacomCardDetailViewData StructuredData =
		UWacomCardPresentationBuilder::BuildCardDetailViewData(StructuredCard.Get());
	const FString StructuredDescription =
		SectionPlainText(StructuredData, EWacomCardDetailSectionKind::Description);
	TestTrue(TEXT("Structured detail still renders effect"),
		StructuredDescription.Contains(TEXT("造成 3 点")) &&
		StructuredDescription.Contains(TEXT("伤害。")) &&
		StructuredDescription.Contains(TEXT("[伤]")));
	TestFalse(TEXT("Structured detail does not append legacy Description"),
		StructuredDescription.Contains(TEXT("旧描述")));

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

	TestTrue(TEXT("Runtime preview renders final damage value"),
		Description.Contains(TEXT("造成 8 点")) && Description.Contains(TEXT("伤害。")) && Description.Contains(TEXT("[伤]")));
	TestTrue(TEXT("Runtime preview renders final reduced value"),
		Description.Contains(TEXT("恢复 3 点")) && Description.Contains(TEXT("生命。")) && Description.Contains(TEXT("[疗]")));
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
