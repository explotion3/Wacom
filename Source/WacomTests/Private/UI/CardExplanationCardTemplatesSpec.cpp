// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardExplanationTemplateContract.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Card/WacomCardDetailPlainTextRenderer.h"
#include "UI/Card/WacomCardDetailRichTextRenderer.h"
#include "UI/Card/WacomCardPresentationBuilder.h"
#include "Validation/CardDefinitionValidation.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
	const FWacomCardDetailSection* FindSection(
		const FWacomCardDetailViewData& Data,
		const EWacomCardDetailSectionKind Kind)
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

	FString SectionText(
		const FWacomCardDetailViewData& Data,
		const EWacomCardDetailSectionKind Kind)
	{
		const FWacomCardDetailSection* Section = FindSection(Data, Kind);
		return Section
			? UWacomCardDetailPlainTextRenderer::RenderSectionPlainText(*Section).ToString()
			: FString();
	}

	bool ContainsValidationError(
		const TArray<FText>& Errors,
		const FString& Needle)
	{
		return Errors.ContainsByPredicate([&Needle](const FText& Error)
		{
			return Error.ToString().Contains(Needle);
		});
	}

	FCardEffect MakeDamage(const int32 Magnitude)
	{
		FCardEffect Effect;
		Effect.EffectType = WacomTags::Effect_Damage;
		Effect.Magnitude = Magnitude;
		Effect.Target = WacomTags::Target_SingleEnemyPart;
		return Effect;
	}

	FCardEffect MakePoison(const int32 Magnitude)
	{
		FCardEffect Effect;
		Effect.EffectType = WacomTags::Effect_ApplyStatus_Poison;
		Effect.Magnitude = Magnitude;
		Effect.Target = WacomTags::Target_SingleEnemyPart;
		return Effect;
	}

	void MakeValidBaseCard(UCardDefinition& Card)
	{
		Card.CardId = TEXT("Card.Test.ExplanationTemplate");
		Card.DisplayName = FText::FromString(TEXT("专属模板测试卡"));
		Card.Rarity = WacomTags::Card_Rarity_White;
		Card.TargetMode = ECardTargetMode::SingleEnemyPart;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUICardExplanationCardEffectTemplatesSpec,
	"Wacom.UI.CardExplanation.CardTemplates.EffectPrecedenceAndTierValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUICardExplanationCardEffectTemplatesSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	MakeValidBaseCard(*Card);
	Card->TierProfiles.SetNum(WacomCardUpgrade::TierCount);
	for (int32 TierIndex = 0;
		TierIndex < WacomCardUpgrade::TierCount;
		++TierIndex)
	{
		Card->TierProfiles[TierIndex].BaseCost = 1;
		Card->TierProfiles[TierIndex].Effects.Add(
			MakeDamage(5 + TierIndex * 3));
	}

	Card->ExplanationTemplates.EffectTemplates.SetNum(1);
	Card->ExplanationTemplates.EffectTemplates[0].Template =
		FText::FromString(
			TEXT("专属句式：{icon:EffectIcon} 造成 {value:Magnitude} 伤害。"));

	FWacomCardPresentationRuntimeContext RuntimeContext;
	RuntimeContext.bHasUpgradeTier = true;
	RuntimeContext.UpgradeTier = EWacomCardUpgradeTier::Blue;
	const FWacomCardDetailViewData BlueData =
		UWacomCardPresentationBuilder::BuildCardDetailViewData(
			Card.Get(),
			RuntimeContext);
	const FString BlueText =
		SectionText(BlueData, EWacomCardDetailSectionKind::Description);
	TestTrue(
		TEXT("Card-specific effect template overrides the shared lexicon"),
		BlueText.Contains(TEXT("专属句式：")));
	TestTrue(
		TEXT("Shared template resolves the active tier magnitude"),
		BlueText.Contains(TEXT("造成 8 伤害。")));

	FWacomCardPresentationRuntimeContext::FEffectPreview Preview;
	Preview.EffectIndex = 0;
	Preview.Magnitude = 13;
	Preview.bHasMagnitude = true;
	RuntimeContext.EffectPreviews.Add(Preview);
	const FWacomCardDetailViewData PreviewData =
		UWacomCardPresentationBuilder::BuildCardDetailViewData(
			Card.Get(),
			RuntimeContext);
	TestTrue(
		TEXT("Runtime preview still overrides the tier magnitude"),
		SectionText(
			PreviewData,
			EWacomCardDetailSectionKind::Description).Contains(
				TEXT("造成 13 伤害。")));

	const FWacomCardDetailSection* Description =
		FindSection(PreviewData, EWacomCardDetailSectionKind::Description);
	TestNotNull(TEXT("Card-specific effect creates a description section"), Description);
	if (Description)
	{
		const FString RichText =
			UWacomCardDetailRichTextRenderer::RenderSectionRichText(
				*Description).ToString();
		TestTrue(
			TEXT("Card-specific icon token remains a typed icon run"),
			RichText.Contains(TEXT("<wacom-icon id=\"Damage\"")));
		TestTrue(
			TEXT("Preview value remains a typed emphasized value run"),
			RichText.Contains(TEXT("<ValueBuffed>13</>")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUICardExplanationCardPassiveTemplatesSpec,
	"Wacom.UI.CardExplanation.CardTemplates.PassiveNestedEffectsAndFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUICardExplanationCardPassiveTemplatesSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	MakeValidBaseCard(*Card);

	FCardPassive Passive;
	Passive.Trigger = WacomTags::Passive_Trigger_OnDraw;
	Passive.TriggerThreshold = 2;
	Passive.DisplayText = FText::FromString(TEXT("旧被动文案不应显示。"));
	Passive.Effects = { MakeDamage(4), MakePoison(3) };
	Card->Passives.Add(Passive);
	Card->ExplanationTemplates.PassiveTemplates.SetNum(1);
	Card->ExplanationTemplates.PassiveTemplates[0].Template =
		FText::FromString(
			TEXT("每 {value:TriggerThreshold} 次抽到："
				"{icon:PassiveEffect[0].Icon} 伤害 +{value:PassiveEffect[0].Magnitude}；"
				"施加 {value:PassiveEffect[1].Magnitude} "
				"{status:PassiveEffect[1].Status}。"));

	const FWacomCardDetailViewData Data =
		UWacomCardPresentationBuilder::BuildCardDetailViewData(Card.Get());
	const FString PassiveText =
		SectionText(Data, EWacomCardDetailSectionKind::Passive);
	TestTrue(
		TEXT("Passive template resolves its trigger threshold"),
		PassiveText.Contains(TEXT("每 2 次抽到")));
	TestTrue(
		TEXT("Passive template resolves multiple nested effect magnitudes"),
		PassiveText.Contains(TEXT("伤害 +4"))
			&& PassiveText.Contains(TEXT("施加 3 中毒")));
	TestFalse(
		TEXT("Card-specific passive suppresses legacy DisplayText"),
		PassiveText.Contains(TEXT("旧被动文案")));
	TestFalse(
		TEXT("Card-specific passive suppresses generated trigger/effect duplication"),
		PassiveText.Contains(TEXT("抽到时：\n")));

	const FWacomCardDetailSection* PassiveSection =
		FindSection(Data, EWacomCardDetailSectionKind::Passive);
	TestNotNull(TEXT("Card-specific passive creates a passive section"), PassiveSection);
	if (PassiveSection)
	{
		const FString RichText =
			UWacomCardDetailRichTextRenderer::RenderSectionRichText(
				*PassiveSection).ToString();
		TestTrue(
			TEXT("Nested passive icon remains a typed icon run"),
			RichText.Contains(TEXT("<wacom-icon id=\"Damage\"")));
		TestTrue(
			TEXT("Nested passive status preserves its gameplay tag"),
			RichText.Contains(TEXT("tag=\"Status.Poison\"")));
	}

	Card->ExplanationTemplates.PassiveTemplates[0].Template = FText::GetEmpty();
	const FWacomCardDetailViewData FallbackData =
		UWacomCardPresentationBuilder::BuildCardDetailViewData(Card.Get());
	TestTrue(
		TEXT("An empty card template falls back to legacy DisplayText"),
		SectionText(
			FallbackData,
			EWacomCardDetailSectionKind::Passive).Contains(
				TEXT("旧被动文案不应显示。")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUICardExplanationCardSupplementalTemplatesSpec,
	"Wacom.UI.CardExplanation.CardTemplates.KeywordDynamicCostAndSuppression",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUICardExplanationCardSupplementalTemplatesSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	MakeValidBaseCard(*Card);
	Card->Keywords.AddTag(WacomTags::Card_Keyword_Retain);
	Card->TierProfiles.SetNum(WacomCardUpgrade::TierCount);
	for (FWacomCardTierProfile& Profile : Card->TierProfiles)
	{
		Profile.BaseCost = 5;
		Profile.DynamicCostRule.CountHandCardsWithStatus =
			WacomTags::Status_Burn;
		Profile.DynamicCostRule.ReductionPerMatchingCard = 1;
		Profile.DynamicCostRule.MinimumCost = 0;
		Profile.Effects = { MakeDamage(5), MakeDamage(5) };
	}

	Card->ExplanationTemplates.EffectTemplates.SetNum(2);
	Card->ExplanationTemplates.EffectTemplates[0].Template =
		FText::FromString(TEXT("合并后的效果 {value:Magnitude}。"));
	Card->ExplanationTemplates.EffectTemplates[1].bSuppressInDetails = true;
	FWacomCardKeywordExplanationTemplate& KeywordTemplate =
		Card->ExplanationTemplates.KeywordTemplates.AddDefaulted_GetRef();
	KeywordTemplate.Keyword = WacomTags::Card_Keyword_Retain;
	KeywordTemplate.Template =
		FText::FromString(TEXT("{keyword:Keyword}。"));
	Card->ExplanationTemplates.DynamicCostTemplate = FText::FromString(
		TEXT("每有一张 {status:CountedStatus} 卡牌，费用 -{value:ReductionPerMatchingCard}。"));

	const FWacomCardDetailViewData Data =
		UWacomCardPresentationBuilder::BuildCardDetailViewData(Card.Get());
	const FWacomCardDetailSection* Description =
		FindSection(Data, EWacomCardDetailSectionKind::Description);
	TestNotNull(TEXT("Keyword and effect create description section"), Description);
	if (Description)
	{
		TestEqual(
			TEXT("Suppressed helper effect does not create a duplicate block"),
			Description->Blocks.Num(),
			2);
		bool bHasTypedRetain = false;
		for (const FWacomCardDetailBlock& Block : Description->Blocks)
		{
			for (const FWacomCardDetailRun& Run : Block.Runs)
			{
				bHasTypedRetain |=
					Run.Kind == EWacomCardDetailRunKind::Keyword
					&& Run.Tag.MatchesTagExact(
						WacomTags::Card_Keyword_Retain);
			}
		}
		TestTrue(
			TEXT("Keyword template preserves the authored GameplayTag"),
			bHasTypedRetain);
	}

	const FWacomCardDetailSection* Passive =
		FindSection(Data, EWacomCardDetailSectionKind::Passive);
	TestNotNull(TEXT("Dynamic cost creates a passive section"), Passive);
	if (Passive)
	{
		const FString PassiveText =
			UWacomCardDetailPlainTextRenderer::RenderSectionPlainText(
				*Passive).ToString();
		TestTrue(
			TEXT("Dynamic cost template resolves its reduction"),
			PassiveText.Contains(TEXT("费用 -1")));
		bool bHasTypedBurn = false;
		for (const FWacomCardDetailBlock& Block : Passive->Blocks)
		{
			for (const FWacomCardDetailRun& Run : Block.Runs)
			{
				bHasTypedBurn |=
					Run.Kind == EWacomCardDetailRunKind::Status
					&& Run.Tag.MatchesTagExact(WacomTags::Status_Burn);
			}
		}
		TestTrue(
			TEXT("Dynamic cost counted status remains a typed status run"),
			bHasTypedBurn);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUICardExplanationCardTemplateValidationSpec,
	"Wacom.UI.CardExplanation.CardTemplates.Validation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUICardExplanationCardTemplateValidationSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	MakeValidBaseCard(*Card);
	Card->Effects.Add(MakeDamage(5));
	Card->ExplanationTemplates.EffectTemplates.SetNum(2);

	TArray<FText> Errors;
	TestFalse(
		TEXT("Mismatched effect-template count is rejected"),
		FWacomCardDefinitionValidation::Validate(Card.Get(), Errors));
	TestTrue(
		TEXT("Count validation names the dedicated explanation field"),
		ContainsValidationError(
			Errors,
			TEXT("ExplanationTemplates.EffectTemplates 数量必须与 Effects 一致")));

	Card->ExplanationTemplates.EffectTemplates.SetNum(1);
	Card->ExplanationTemplates.EffectTemplates[0].Template =
		FText::FromString(TEXT("{Status:EffectStatus"));
	Errors.Reset();
	TestFalse(
		TEXT("Unclosed and unknown tokens are rejected"),
		FWacomCardDefinitionValidation::Validate(Card.Get(), Errors));
	TestTrue(
		TEXT("Unclosed token reports a template validation error"),
		ContainsValidationError(Errors, TEXT("没有闭合")));

	FCardPassive Passive;
	Passive.Trigger = WacomTags::Passive_Trigger_OnDraw;
	Passive.Effects.Add(MakePoison(2));
	Card->Passives.Add(Passive);
	Card->ExplanationTemplates.EffectTemplates[0].Template =
		FText::FromString(TEXT("{icon:EffectIcon} 造成 {value:Magnitude} 伤害。"));
	Card->ExplanationTemplates.PassiveTemplates.SetNum(1);
	Card->ExplanationTemplates.PassiveTemplates[0].Template =
		FText::FromString(TEXT("{value:PassiveEffect[2].Magnitude}"));
	Errors.Reset();
	TestFalse(
		TEXT("Out-of-range passive effect indices are rejected"),
		FWacomCardDefinitionValidation::Validate(Card.Get(), Errors));
	TestTrue(
		TEXT("Passive effect index validation reports the authored index"),
		ContainsValidationError(Errors, TEXT("PassiveEffect[2] 超出")));

	Card->ExplanationTemplates.PassiveTemplates[0].Template =
		FText::FromString(TEXT("{value:PassiveEffect[0].Magnitude}"));
	FWacomCardKeywordExplanationTemplate& KeywordTemplate =
		Card->ExplanationTemplates.KeywordTemplates.AddDefaulted_GetRef();
	KeywordTemplate.Keyword = WacomTags::Card_Keyword_Retain;
	KeywordTemplate.Template =
		FText::FromString(TEXT("{keyword:Keyword}。"));
	Errors.Reset();
	TestFalse(
		TEXT("A keyword template not owned by the card is rejected"),
		FWacomCardDefinitionValidation::Validate(Card.Get(), Errors));
	TestTrue(
		TEXT("Keyword validation names the missing card keyword"),
		ContainsValidationError(Errors, TEXT("不存在于 CardDefinition.Keywords")));

	TArray<FString> ContractErrors;
	WacomCardExplanationTemplateContract::ValidateEffectTemplate(
		FText::FromString(TEXT("{mystery:Magnitude}")),
		Card->Effects[0],
		ContractErrors);
	TestTrue(
		TEXT("Unknown token types are rejected by the shared grammar"),
		ContractErrors.ContainsByPredicate([](const FString& Error)
		{
			return Error.Contains(TEXT("不支持参数"));
	}));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUICardExplanationFireWriteAuthoredPresentationSpec,
	"Wacom.UI.CardExplanation.CardTemplates.FireWriteAuthoredPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUICardExplanationFireWriteAuthoredPresentationSpec::RunTest(
	const FString& /*Parameters*/)
{
	const UCardDefinition* Warm = LoadObject<UCardDefinition>(
		nullptr,
		TEXT("/Game/Wacom/Data/Cards/FireWrite/"
			"DA_Card_WarmTinderbug.DA_Card_WarmTinderbug"));
	TestNotNull(TEXT("Warm Tinderbug asset loads"), Warm);
	if (Warm)
	{
		const FWacomCardDetailViewData Data =
			UWacomCardPresentationBuilder::BuildCardDetailViewData(Warm);
		const FString Description =
			SectionText(Data, EWacomCardDetailSectionKind::Description);
		const FString Passive =
			SectionText(Data, EWacomCardDetailSectionKind::Passive);
		TestTrue(
			TEXT("Warm Tinderbug exposes Retain"),
			Description.Contains(TEXT("保留")));
		TestTrue(
			TEXT("Warm Tinderbug combines its deterministic Burn aura"),
			Description.Contains(
				TEXT("使手牌中所有卡（包含自身）的灼烧效果 +1"))
			&& Description.Contains(TEXT("已有灼烧的卡牌获得双倍加成")));
		const FString DoubleBonusText =
			TEXT("已有灼烧的卡牌获得双倍加成");
		int32 DoubleBonusCount = 0;
		int32 SearchFrom = 0;
		while ((SearchFrom = Description.Find(
			DoubleBonusText,
			ESearchCase::CaseSensitive,
			ESearchDir::FromStart,
			SearchFrom)) != INDEX_NONE)
		{
			++DoubleBonusCount;
			SearchFrom += DoubleBonusText.Len();
		}
		TestEqual(
			TEXT("Warm Tinderbug helper modifier is not duplicated"),
			DoubleBonusCount,
			1);
		TestTrue(
			TEXT("Warm Tinderbug exposes its dynamic cost passive"),
			Passive.Contains(TEXT("手牌中每有一张"))
				&& Passive.Contains(TEXT("灼烧"))
				&& Passive.Contains(TEXT("本卡费用 -1")));
		TestFalse(
			TEXT("Warm Tinderbug detail does not leak template tokens"),
			Description.Contains(TEXT("{")) || Passive.Contains(TEXT("{")));
	}

	const UCardDefinition* Obsidian = LoadObject<UCardDefinition>(
		nullptr,
		TEXT("/Game/Wacom/Data/Cards/FireWrite/"
			"DA_Card_ObsidianBeetle.DA_Card_ObsidianBeetle"));
	TestNotNull(TEXT("Obsidian Beetle asset loads"), Obsidian);
	if (Obsidian)
	{
		const FWacomCardDetailViewData Data =
			UWacomCardPresentationBuilder::BuildCardDetailViewData(Obsidian);
		TestTrue(
			TEXT("Obsidian Beetle uses its authored damage sentence"),
			SectionText(Data, EWacomCardDetailSectionKind::Description)
				.Contains(TEXT("造成 5 伤害")));
		TestTrue(
			TEXT("Obsidian Beetle exposes its draw-doubling passive"),
			SectionText(Data, EWacomCardDetailSectionKind::Passive)
				.Contains(TEXT("每次抽到本卡时：本场自身伤害翻倍")));
	}

	const TArray<FString> FireWriteNames = {
		TEXT("OilCandle"),
		TEXT("AshBug"),
		TEXT("SaltMaggot"),
		TEXT("WarmTinderbug"),
		TEXT("FireflySeed"),
		TEXT("HungryFireflyMaiden"),
		TEXT("BlazingEyeFirefly"),
		TEXT("RottenFirefly"),
		TEXT("GlimmerFirefly"),
		TEXT("SlothFirefly"),
		TEXT("EmptyBottle"),
		TEXT("MoltenSalt"),
		TEXT("JadeBeetle"),
		TEXT("ObsidianBeetle"),
		TEXT("BlindSpider")
	};
	for (const FString& EnglishName : FireWriteNames)
	{
		const FString ObjectPath = FString::Printf(
			TEXT("/Game/Wacom/Data/Cards/FireWrite/DA_Card_%s."
				"DA_Card_%s"),
			*EnglishName,
			*EnglishName);
		const UCardDefinition* Card =
			LoadObject<UCardDefinition>(nullptr, *ObjectPath);
		TestNotNull(
			*FString::Printf(TEXT("%s asset loads"), *EnglishName),
			Card);
		if (!Card)
		{
			continue;
		}
		for (int32 TierIndex = 0;
			TierIndex < WacomCardUpgrade::TierCount;
			++TierIndex)
		{
			FWacomCardPresentationRuntimeContext RuntimeContext;
			RuntimeContext.bHasUpgradeTier = true;
			RuntimeContext.UpgradeTier =
				static_cast<EWacomCardUpgradeTier>(TierIndex);
			const FWacomCardDetailViewData Data =
				UWacomCardPresentationBuilder::BuildCardDetailViewData(
					Card,
					RuntimeContext);
			FString PlayerText;
			for (const FWacomCardDetailSection& Section : Data.Sections)
			{
				PlayerText +=
					UWacomCardDetailPlainTextRenderer::
						RenderSectionPlainText(Section).ToString();
			}
			TestFalse(
				*FString::Printf(
					TEXT("%s tier %d has no unresolved template token"),
					*EnglishName,
					TierIndex),
				PlayerText.Contains(TEXT("{"))
					|| PlayerText.Contains(TEXT("EveryEnteredExhaust"))
					|| PlayerText.Contains(TEXT("EffectStatus")));
		}
	}
	return true;
}
