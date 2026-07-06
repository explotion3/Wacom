// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Card/WacomCardPresentationBuilder.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
	TArray<FWacomCardDetailTokenLine> CollectPassiveTokenLines(const FWacomCardDetailViewData& Data)
	{
		TArray<FWacomCardDetailTokenLine> Lines;
		for (const FWacomCardDetailTokenLine& Line : Data.TokenLines)
		{
			if (Line.Kind == EWacomCardDetailTokenLineKind::Passive)
			{
				Lines.Add(Line);
			}
		}
		return Lines;
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

	TestEqual(TEXT("Passive plain text mirror is normalized"), Data.PassiveLines.Num(), 1);
	if (Data.PassiveLines.Num() > 0)
	{
		TestEqual(
			TEXT("Passive plain text drops section label"),
			Data.PassiveLines[0].ToString(),
			TEXT("暮气触发时，使一张中毒卡牌效果 +1。"));
	}

	const TArray<FWacomCardDetailTokenLine> PassiveTokenLines = CollectPassiveTokenLines(Data);
	TestEqual(TEXT("Authored passive emits one passive token line"), PassiveTokenLines.Num(), 1);

	const FString PassiveText = JoinPassiveTokenText(Data);
	TestFalse(TEXT("Authored passive body does not duplicate section label"), PassiveText.Contains(TEXT("被动：")));
	TestTrue(TEXT("Authored passive body keeps full rule text"),
		PassiveText.Contains(TEXT("暮气触发时，使一张中毒卡牌效果 +1。")));

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

	const FString PassiveText = JoinPassiveTokenText(Data);
	TestFalse(TEXT("Structured passive body does not duplicate section label"), PassiveText.Contains(TEXT("被动：")));
	TestTrue(TEXT("Structured passive keeps trigger text"), PassiveText.Contains(TEXT("打出后")));
	TestTrue(TEXT("Structured passive keeps effect text"), PassiveText.Contains(TEXT("该牌腾挪至随机区域。")));

	return true;
}
