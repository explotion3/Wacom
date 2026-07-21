// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Shop/WacomShopUpgradePresentationBuilder.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomShopUpgradePresentationFactsSpec,
	"Wacom.UI.Shop.UpgradePresentation.QuoteAndDifferenceFacts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomShopUpgradePresentationFactsSpec::RunTest(const FString& Parameters)
{
	UCardDefinition* White = NewObject<UCardDefinition>();
	White->CardId = TEXT("Test.ShopUpgrade.White");
	White->DisplayName = FText::FromString(TEXT("试制毒牙"));
	White->BaseCost = 1;
	White->Rarity = WacomTags::Card_Rarity_White;
	FCardEffect WhiteDamage;
	WhiteDamage.EffectType = WacomTags::Effect_Damage;
	WhiteDamage.Magnitude = 3;
	FCardEffect WhitePoison;
	WhitePoison.EffectType = WacomTags::Effect_ApplyStatus_Poison;
	WhitePoison.Magnitude = 1;
	White->Effects = { WhiteDamage, WhitePoison };

	UCardDefinition* Blue = NewObject<UCardDefinition>();
	Blue->CardId = TEXT("Test.ShopUpgrade.Blue");
	Blue->DisplayName = FText::FromString(TEXT("试制毒牙"));
	Blue->BaseCost = 1;
	Blue->Rarity = WacomTags::Card_Rarity_Blue;
	FCardEffect BlueDamage = WhiteDamage;
	BlueDamage.Magnitude = 5;
	FCardEffect BluePoison = WhitePoison;
	BluePoison.Magnitude = 2;
	Blue->Effects = { BlueDamage, BluePoison };
	White->NextUpgradeDefinition = Blue;

	FRunShopCardUpgradeQuote Quote;
	Quote.InstanceId = FGuid::NewGuid();
	Quote.CurrentDefinition = White;
	Quote.NextDefinition = Blue;
	Quote.CurrentRarity = White->Rarity;
	Quote.NextRarity = Blue->Rarity;
	Quote.Price = 2;
	Quote.bCanUpgrade = true;

	const FWacomShopCardUpgradePresentationView View =
		UWacomShopUpgradePresentationBuilder::BuildUpgradePresentationView(Quote);
	TestEqual(TEXT("Instance identity is preserved"), View.InstanceId, Quote.InstanceId);
	TestEqual(TEXT("Current card name"), View.CurrentCardNameText.ToString(), FString(TEXT("试制毒牙")));
	TestEqual(TEXT("Next card keeps the same name"), View.NextCardNameText.ToString(), FString(TEXT("试制毒牙")));
	TestEqual(TEXT("Action is inline price"), View.ActionText.ToString(), FString(TEXT("支付 2 金币并强化")));
	TestTrue(TEXT("Action remains enabled"), View.bCanUpgrade);
	const FString Summary = View.ChangeSummaryText.ToString();
	TestTrue(TEXT("Rarity difference is shown"), Summary.Contains(TEXT("White → Blue")));
	TestTrue(TEXT("Damage difference is shown"), Summary.Contains(TEXT("伤害：3 → 5")));
	TestTrue(TEXT("Poison difference is shown"), Summary.Contains(TEXT("中毒：1 → 2")));
	TestEqual(TEXT("Current damage remains neutral"),
		View.CurrentCardViewData.EffectBadges[0].ValueEmphasis,
		EWacomCardViewValueEmphasis::Neutral);
	TestEqual(TEXT("Next damage is highlighted as increased"),
		View.NextCardViewData.EffectBadges[0].ValueEmphasis,
		EWacomCardViewValueEmphasis::Increased);
	TestEqual(TEXT("Next poison is highlighted as increased"),
		View.NextCardViewData.EffectBadges[1].ValueEmphasis,
		EWacomCardViewValueEmphasis::Increased);
	TestEqual(TEXT("Same-name success copy reports rarity instead of repeating the name"),
		UWacomShopUpgradePresentationBuilder::BuildUpgradeSuccessText(White, Blue).ToString(),
		FString(TEXT("已强化：试制毒牙（White → Blue）")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomShopUpgradePresentationFilterSpec,
	"Wacom.UI.Shop.UpgradePresentation.FiltersTerminalAndPreservesInstances",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomShopUpgradePresentationFilterSpec::RunTest(const FString& Parameters)
{
	UCardDefinition* Current = NewObject<UCardDefinition>();
	Current->CardId = TEXT("Test.Current");
	UCardDefinition* Next = NewObject<UCardDefinition>();
	Next->CardId = TEXT("Test.Next");
	Current->NextUpgradeDefinition = Next;

	FRunShopSnapshot Snapshot;
	for (int32 Index = 0; Index < 2; ++Index)
	{
		FRunShopCardUpgradeQuote& Quote = Snapshot.CardUpgradeQuotes.AddDefaulted_GetRef();
		Quote.InstanceId = FGuid::NewGuid();
		Quote.CurrentDefinition = Current;
		Quote.NextDefinition = Next;
		Quote.Price = 2;
		Quote.bCanUpgrade = Index == 0;
		Quote.DisabledReason = Index == 0 ? NAME_None : FName(TEXT("InsufficientGold"));
	}
	FRunShopCardUpgradeQuote& Terminal = Snapshot.CardUpgradeQuotes.AddDefaulted_GetRef();
	Terminal.InstanceId = FGuid::NewGuid();
	Terminal.CurrentDefinition = Next;
	Terminal.DisabledReason = TEXT("NoNextUpgrade");

	const TArray<FWacomShopCardUpgradePresentationView> Views =
		UWacomShopUpgradePresentationBuilder::BuildUpgradePresentationViews(Snapshot);
	TestEqual(TEXT("Only cards with a next definition are listed"), Views.Num(), 2);
	TestNotEqual(TEXT("Duplicate definitions keep distinct instances"), Views[0].InstanceId, Views[1].InstanceId);
	TestFalse(TEXT("Insufficient gold remains visible but disabled"), Views[1].bCanUpgrade);
	TestEqual(TEXT("Stable insufficient gold copy"), Views[1].StatusText.ToString(), FString(TEXT("金币不足")));
	return true;
}

#endif
