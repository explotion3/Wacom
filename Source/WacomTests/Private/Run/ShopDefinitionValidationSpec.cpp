// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Fixtures/GeneratedBattleContentTestAssets.h"
#include "Shops/ShopDefinition.h"
#include "Tags/WacomGameplayTags.h"
#include "Validation/ShopDefinitionValidation.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
	UCardDefinition* MakeShopValidationCard(UObject* Outer, FName CardId)
	{
		UCardDefinition* Card = NewObject<UCardDefinition>(Outer);
		Card->CardId = CardId;
		Card->DisplayName = FText::FromName(CardId);
		return Card;
	}

	UShopDefinition* MakeValidShopForValidation(UObject* Outer)
	{
		UShopDefinition* Shop = NewObject<UShopDefinition>(Outer);
		Shop->ShopId = TEXT("Shop.Validation");
		Shop->DisplayName = FText::FromString(TEXT("校验商店"));

		FShopOfferDefinition FreeOffer;
		FreeOffer.CardDefinition = MakeShopValidationCard(Shop, TEXT("Card.Validation.Free"));
		FreeOffer.Price = 0;

		FShopOfferDefinition PaidOffer;
		PaidOffer.CardDefinition = MakeShopValidationCard(Shop, TEXT("Card.Validation.Paid"));
		PaidOffer.Price = 2;

		Shop->Offers = { FreeOffer, PaidOffer };
		return Shop;
	}

	bool ValidateShopForTest(const UShopDefinition* Shop, TArray<FText>& OutErrors)
	{
		return FWacomShopDefinitionValidation::Validate(Shop, OutErrors);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataShopValidationValidSpec,
	"Wacom.Data.Shop.Validation.ValidShop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataShopValidationValidSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UShopDefinition> Shop(MakeValidShopForValidation(GetTransientPackage()));
	TArray<FText> Errors;
	TestTrue(TEXT("Valid ShopDefinition passes validation"), ValidateShopForTest(Shop.Get(), Errors));
	TestEqual(TEXT("No validation errors"), Errors.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataShopValidationRequiredFieldsSpec,
	"Wacom.Data.Shop.Validation.RequiredFields",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataShopValidationRequiredFieldsSpec::RunTest(const FString& /*Parameters*/)
{
	TArray<FText> Errors;

	{
		TStrongObjectPtr<UShopDefinition> Shop(MakeValidShopForValidation(GetTransientPackage()));
		Shop->ShopId = NAME_None;
		TestFalse(TEXT("Missing ShopId fails"), ValidateShopForTest(Shop.Get(), Errors));
		TestTrue(TEXT("Missing ShopId has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UShopDefinition> Shop(MakeValidShopForValidation(GetTransientPackage()));
		Shop->Offers.Reset();
		TestFalse(TEXT("Empty Offers fails"), ValidateShopForTest(Shop.Get(), Errors));
		TestTrue(TEXT("Empty Offers has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UShopDefinition> Shop(MakeValidShopForValidation(GetTransientPackage()));
		Shop->Offers[0].CardDefinition = nullptr;
		TestFalse(TEXT("Missing offer card fails"), ValidateShopForTest(Shop.Get(), Errors));
		TestTrue(TEXT("Missing offer card has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UShopDefinition> Shop(MakeValidShopForValidation(GetTransientPackage()));
		Shop->Offers[0].Price = -1;
		TestFalse(TEXT("Negative price fails"), ValidateShopForTest(Shop.Get(), Errors));
		TestTrue(TEXT("Negative price has error"), Errors.Num() > 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataShopUpgradeServiceValidationSpec,
	"Wacom.Data.Shop.Validation.CardUpgradeService",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataShopUpgradeServiceValidationSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UShopDefinition> Shop(MakeValidShopForValidation(GetTransientPackage()));
	Shop->CardUpgradeService.bEnabled = true;
	Shop->CardUpgradeService.Prices = {
		{ WacomTags::Card_Rarity_White, 0 },
		{ WacomTags::Card_Rarity_Blue, 2 },
		{ WacomTags::Card_Rarity_Yellow, 4 },
	};

	TArray<FText> Errors;
	TestTrue(TEXT("Enabled service with three unique transition prices passes"),
		ValidateShopForTest(Shop.Get(), Errors));
	TestEqual(TEXT("Valid service has no errors"), Errors.Num(), 0);

	Shop->CardUpgradeService.Prices.Add({ WacomTags::Card_Rarity_Blue, 9 });
	TestFalse(TEXT("Duplicate source rarity fails"), ValidateShopForTest(Shop.Get(), Errors));

	Shop->CardUpgradeService.Prices.SetNum(1);
	Shop->CardUpgradeService.Prices[0].FromRarity = WacomTags::Card_Rarity_Purple;
	TestFalse(TEXT("Purple source rarity fails"), ValidateShopForTest(Shop.Get(), Errors));

	Shop->CardUpgradeService.Prices[0].FromRarity = WacomTags::Card_Rarity_White;
	Shop->CardUpgradeService.Prices[0].Price = -1;
	TestFalse(TEXT("Negative upgrade price fails"), ValidateShopForTest(Shop.Get(), Errors));

	Shop->CardUpgradeService.Prices.Reset();
	TestFalse(TEXT("Enabled service requires prices"), ValidateShopForTest(Shop.Get(), Errors));

	Shop->CardUpgradeService.bEnabled = false;
	TestTrue(TEXT("Disabled service ignores an empty price table"), ValidateShopForTest(Shop.Get(), Errors));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataShopDebugSnakeAssetValidationSpec,
	"Wacom.Data.Shop.DebugSnakeAssetValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataShopDebugSnakeAssetValidationSpec::RunTest(const FString& /*Parameters*/)
{
	UShopDefinition* DebugShop = FWacomGeneratedBattleContentAssets::LoadDebugSnakeShop(*this);

	if (!TestNotNull(TEXT("DebugSnake shop asset loads"), DebugShop))
	{
		return false;
	}

	TArray<FText> Errors;
	TestTrue(TEXT("DebugSnake shop asset passes validation"), ValidateShopForTest(DebugShop, Errors));
	TestEqual(TEXT("DebugSnake shop validation errors"), Errors.Num(), 0);
	return true;
}
