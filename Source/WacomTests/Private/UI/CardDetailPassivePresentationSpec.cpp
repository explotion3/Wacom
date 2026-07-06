// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Card/WacomCardPresentationBuilder.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
	TArray<FWacomCardDetailTokenLine> CollectPassiveTokenLines(const FWacomCardDetailViewData& Data)
	{
		TArray<FWacomCardDetailTokenLine> Lines;
		for (const FWacomCardDetailSection& Section : Data.Sections)
		{
			if (Section.Kind == EWacomCardDetailSectionKind::Passive)
			{
				Lines.Append(Section.TokenLines);
			}
		}
		return Lines;
	}

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

	FWacomCardDetailTokenLine MakeTextLineForTest(
		FName LineId,
		EWacomCardDetailTokenLineKind Kind,
		const FString& Text)
	{
		FWacomCardDetailToken Token;
		Token.StableId = FName(*FString::Printf(TEXT("%s.Text"), *LineId.ToString()));
		Token.Kind = EWacomCardDetailTokenKind::Text;
		Token.Text = FText::FromString(Text);

		FWacomCardDetailTokenLine Line;
		Line.LineId = LineId;
		Line.Kind = Kind;
		Line.Tokens.Add(Token);
		return Line;
	}

	FString TokenText(const FWacomCardDetailToken& Token)
	{
		if (Token.Kind == EWacomCardDetailTokenKind::Number && Token.bHasValue)
		{
			return FString::FromInt(Token.bHasPreviewValue ? Token.PreviewValue : Token.Value);
		}
		return Token.Text.ToString();
	}

	FString JoinTokenLineText(const FWacomCardDetailTokenLine& Line)
	{
		FString Text;
		for (const FWacomCardDetailToken& Token : Line.Tokens)
		{
			Text += TokenText(Token);
		}
		return Text;
	}

	FString JoinPassiveTokenText(const FWacomCardDetailViewData& Data)
	{
		FString Text;
		for (const FWacomCardDetailTokenLine& Line : CollectPassiveTokenLines(Data))
		{
			if (!Text.IsEmpty())
			{
				Text += TEXT("\n");
			}
			Text += JoinTokenLineText(Line);
		}
		return Text;
	}

	bool ContainsIconToken(const FWacomCardDetailTokenLine& Line, EWacomCardDetailIcon Icon)
	{
		for (const FWacomCardDetailToken& Token : Line.Tokens)
		{
			if (Token.Kind == EWacomCardDetailTokenKind::Icon && Token.Icon == Icon)
			{
				return true;
			}
		}
		return false;
	}

	bool ContainsNumberToken(const FWacomCardDetailTokenLine& Line, int32 Value)
	{
		for (const FWacomCardDetailToken& Token : Line.Tokens)
		{
			if (Token.Kind == EWacomCardDetailTokenKind::Number && Token.bHasValue && Token.Value == Value)
			{
				return true;
			}
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUICardDetailPassiveAuthoredTextSpec,
	"Wacom.UI.CardDetail.Passive.AuthoredText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUICardDetailPassiveAuthoredTextSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	Card->CardId = TEXT("PassiveAuthoredTextCard");
	Card->DisplayName = FText::FromString(TEXT("被动手写正文卡"));
	Card->Description = FText::FromString(TEXT("主动描述。"));

	FCardPassive Passive;
	Passive.Trigger = WacomTags::Passive_Trigger_OnTwilightTriggered;
	Passive.DisplayText = FText::FromString(TEXT("被动：暮气触发时，使一张中毒卡牌效果 +1。"));
	Card->Passives.Add(Passive);

	const FWacomCardDetailViewData Data = UWacomCardPresentationBuilder::BuildCardDetailViewData(Card.Get());

	const TArray<FWacomCardDetailTokenLine> PassiveTokenLines = CollectPassiveTokenLines(Data);
	TestEqual(TEXT("Authored passive emits one passive token line"), PassiveTokenLines.Num(), 1);
	TestNotNull(TEXT("Authored passive emits canonical passive section"),
		FindSection(Data, EWacomCardDetailSectionKind::Passive));

	const FString PassiveText = JoinPassiveTokenText(Data);
	TestFalse(TEXT("Authored passive body does not duplicate section label"), PassiveText.Contains(TEXT("被动：")));
	TestTrue(TEXT("Authored passive body keeps full rule text"),
		PassiveText.Contains(TEXT("暮气触发时，使一张中毒卡牌效果 +1。")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUICardDetailPassiveAuthoredEffectPlaceholderSpec,
	"Wacom.UI.CardDetail.Passive.AuthoredEffectPlaceholder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUICardDetailPassiveAuthoredEffectPlaceholderSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	Card->CardId = TEXT("PassiveAuthoredEffectPlaceholderCard");
	Card->DisplayName = FText::FromString(TEXT("被动占位符卡"));
	Card->Description = FText::FromString(TEXT("主动描述。"));

	FCardPassive Passive;
	Passive.Trigger = WacomTags::Passive_Trigger_OnTwilightTriggered;
	Passive.DisplayText = FText::FromString(TEXT("暮气触发时，施加 {Effect.0} 中毒。"));

	FCardEffect Poison;
	Poison.EffectType = WacomTags::Effect_ApplyStatus_Poison;
	Poison.Magnitude = 2;
	Passive.Effects.Add(Poison);
	Card->Passives.Add(Passive);

	const FWacomCardDetailViewData Data = UWacomCardPresentationBuilder::BuildCardDetailViewData(Card.Get());
	const TArray<FWacomCardDetailTokenLine> PassiveTokenLines = CollectPassiveTokenLines(Data);

	TestEqual(TEXT("Authored passive placeholder emits one line"), PassiveTokenLines.Num(), 1);
	if (PassiveTokenLines.Num() > 0)
	{
		const FString PassiveText = JoinTokenLineText(PassiveTokenLines[0]);
		TestFalse(TEXT("Authored passive placeholder is not kept as literal"),
			PassiveText.Contains(TEXT("{Effect.0}")));
		TestTrue(TEXT("Authored passive placeholder keeps surrounding text"),
			PassiveText.Contains(TEXT("暮气触发时，施加")));
		TestTrue(TEXT("Authored passive placeholder includes magnitude"),
			PassiveText.Contains(TEXT("2")));
		TestTrue(TEXT("Authored passive placeholder emits poison icon token"),
			ContainsIconToken(PassiveTokenLines[0], EWacomCardDetailIcon::Poison));
		TestTrue(TEXT("Authored passive placeholder emits number token"),
			ContainsNumberToken(PassiveTokenLines[0], 2));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUICardDetailPassiveStructuredEffectSpec,
	"Wacom.UI.CardDetail.Passive.StructuredEffect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUICardDetailPassiveStructuredEffectSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	Card->CardId = TEXT("PassiveStructuredEffectCard");
	Card->DisplayName = FText::FromString(TEXT("被动结构化效果卡"));
	Card->Description = FText::FromString(TEXT("主动描述。"));

	FCardPassive Passive;
	Passive.Trigger = WacomTags::Passive_Trigger_AfterPlayed;

	FCardEffect ShuffleSelf;
	ShuffleSelf.EffectType = WacomTags::Effect_Shuffle_ToRandomZone;
	ShuffleSelf.Target = WacomTags::Target_Self;
	Passive.Effects.Add(ShuffleSelf);
	Card->Passives.Add(Passive);

	const FWacomCardDetailViewData Data = UWacomCardPresentationBuilder::BuildCardDetailViewData(Card.Get());

	const TArray<FWacomCardDetailTokenLine> PassiveTokenLines = CollectPassiveTokenLines(Data);
	TestEqual(TEXT("Structured passive emits trigger and effect lines"), PassiveTokenLines.Num(), 2);
	TestNotNull(TEXT("Structured passive emits canonical passive section"),
		FindSection(Data, EWacomCardDetailSectionKind::Passive));

	const FString PassiveText = JoinPassiveTokenText(Data);
	TestFalse(TEXT("Structured passive body does not duplicate section label"), PassiveText.Contains(TEXT("被动：")));
	TestTrue(TEXT("Structured passive keeps trigger text"), PassiveText.Contains(TEXT("打出后")));
	TestTrue(TEXT("Structured passive keeps effect text"), PassiveText.Contains(TEXT("该牌腾挪至随机区域。")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUICardDetailSectionsDocumentSpec,
	"Wacom.UI.CardDetail.Sections.Document",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUICardDetailSectionsDocumentSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	Card->CardId = TEXT("SectionDocumentCard");
	Card->DisplayName = FText::FromString(TEXT("详情文档卡"));
	Card->Description = FText::FromString(TEXT("造成 {Effect.0} 伤害。"));

	FCardEffect Damage;
	Damage.EffectType = WacomTags::Effect_Damage;
	Damage.Magnitude = 4;
	Card->Effects.Add(Damage);

	FCardPassive Passive;
	Passive.Trigger = WacomTags::Passive_Trigger_OnTwilightTriggered;
	Passive.DisplayText = FText::FromString(TEXT("暮气触发时，使一张中毒卡牌效果 +1。"));
	Card->Passives.Add(Passive);

	const FWacomCardDetailViewData Data = UWacomCardPresentationBuilder::BuildCardDetailViewData(Card.Get());

	TestEqual(TEXT("Detail document has description and passive sections"), Data.Sections.Num(), 2);
	if (Data.Sections.Num() >= 2)
	{
		TestEqual(TEXT("First section is description"),
			Data.Sections[0].Kind,
			EWacomCardDetailSectionKind::Description);
		TestEqual(TEXT("First section title is localized"),
			Data.Sections[0].Title.ToString(),
			TEXT("描述"));
		TestEqual(TEXT("Second section is passive"),
			Data.Sections[1].Kind,
			EWacomCardDetailSectionKind::Passive);
		TestEqual(TEXT("Second section title is localized"),
			Data.Sections[1].Title.ToString(),
			TEXT("被动"));
	}

	const FWacomCardDetailSection* DescriptionSection =
		FindSection(Data, EWacomCardDetailSectionKind::Description);
	TestNotNull(TEXT("Description section exists"), DescriptionSection);
	if (DescriptionSection)
	{
		TestEqual(TEXT("Description section keeps one authored line"),
			DescriptionSection->TokenLines.Num(),
			1);
		const FString DescriptionText = DescriptionSection->TokenLines.Num() > 0
			? JoinTokenLineText(DescriptionSection->TokenLines[0])
			: FString();
		TestTrue(TEXT("Description section compiles effect placeholder"),
			DescriptionText.Contains(TEXT("造成 ")));
		TestTrue(TEXT("Description section includes runtime number"),
			DescriptionText.Contains(TEXT("4")));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUICardDetailPanelUsesSectionsSpec,
	"Wacom.UI.CardDetail.Sections.PanelUsesSections",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUICardDetailPanelUsesSectionsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardDetailPanel> Panel(NewObject<UWacomCardDetailPanel>());

	FWacomCardDetailViewData Data;
	Data.Name = FText::FromString(TEXT("面板文档卡"));

	FWacomCardDetailSection Section;
	Section.SectionId = FName(TEXT("Canonical"));
	Section.Kind = EWacomCardDetailSectionKind::Task;
	Section.Title = FText::FromString(TEXT("正式区块"));
	Section.TokenLines.Add(MakeTextLineForTest(
		FName(TEXT("Canonical.0")),
		EWacomCardDetailTokenLineKind::Description,
		TEXT("正式正文")));
	Data.Sections.Add(Section);

	Panel->SetCardDetailData(Data);
	Panel->TakeWidget();
	Panel->SetCardDetailData(Data);

	TestEqual(TEXT("Panel renders canonical sections only"), Panel->GetSectionCount(), 1);
	TestEqual(TEXT("Panel uses canonical section title"),
		Panel->GetSectionTitleText(0).ToString(),
		TEXT("正式区块"));

	return true;
}
