// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Fixtures/GeneratedBattleContentTestAssets.h"
#include "Shops/ShopDefinition.h"
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
