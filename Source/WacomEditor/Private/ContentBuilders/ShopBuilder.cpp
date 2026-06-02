// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/ShopBuilder.h"
#include "ContentBuilders/ContentBuilderHelpers.h"

#include "Cards/CardDefinition.h"
#include "Shops/ShopDefinition.h"

namespace
{
	using namespace Wacom::ContentBuilder;

	UCardDefinition* LoadGeneratedCard(const FString& ObjectPath)
	{
		UCardDefinition* Card = LoadObject<UCardDefinition>(nullptr, *ObjectPath);
		if (!Card)
		{
			UE_LOG(LogTemp, Error, TEXT("[ShopBuilder] Failed to load card asset: %s"), *ObjectPath);
		}
		return Card;
	}

	FShopOfferDefinition MakeOffer(UCardDefinition* Card, int32 Price)
	{
		FShopOfferDefinition Offer;
		Offer.CardDefinition = Card;
		Offer.Price = Price;
		return Offer;
	}
}

namespace Wacom::ContentBuilder
{
	UShopDefinition* BuildShopContent()
	{
		const FString RewardsRoot = RewardCardsRoot();
		const FString BugGirlRoot = BugGirlCardsRoot();
		const FString StarterPackRoot = BugGirlStarterPackCardsRoot();
		UCardDefinition* PoisonFang = LoadGeneratedCard(
			MakeObjectPath(MakePackagePath(RewardsRoot, TEXT("DA_Card_PoisonFang"))));
		UCardDefinition* ChifuGongyi = LoadGeneratedCard(
			MakeObjectPath(MakePackagePath(BugGirlRoot, TEXT("DA_Card_ChifuGongyi"))));
		UCardDefinition* ZhaoguangMudie = LoadGeneratedCard(
			MakeObjectPath(MakePackagePath(BugGirlRoot, TEXT("DA_Card_ZhaoguangMudie"))));
		UCardDefinition* BugGirlBag = LoadGeneratedCard(
			MakeObjectPath(MakePackagePath(BugGirlRoot, TEXT("DA_Card_BugGirlBag"))));
		UCardDefinition* PoisonNeedle = LoadGeneratedCard(
			MakeObjectPath(MakePackagePath(StarterPackRoot, TEXT("DA_Card_Starter_PoisonNeedle"))));
		UCardDefinition* ChitinWard = LoadGeneratedCard(
			MakeObjectPath(MakePackagePath(StarterPackRoot, TEXT("DA_Card_Starter_ChitinWard"))));
		UCardDefinition* AntennaSearch = LoadGeneratedCard(
			MakeObjectPath(MakePackagePath(StarterPackRoot, TEXT("DA_Card_Starter_AntennaSearch"))));
		UCardDefinition* MoltCut = LoadGeneratedCard(
			MakeObjectPath(MakePackagePath(StarterPackRoot, TEXT("DA_Card_Starter_MoltCut"))));
		UCardDefinition* LightHusk = LoadGeneratedCard(
			MakeObjectPath(MakePackagePath(StarterPackRoot, TEXT("DA_Card_Starter_LightHusk"))));
		UCardDefinition* SilklineFeint = LoadGeneratedCard(
			MakeObjectPath(MakePackagePath(StarterPackRoot, TEXT("DA_Card_Starter_SilklineFeint"))));
		if (!PoisonFang || !ChifuGongyi || !ZhaoguangMudie || !BugGirlBag
			|| !PoisonNeedle || !ChitinWard || !AntennaSearch || !MoltCut || !LightHusk || !SilklineFeint)
		{
			return nullptr;
		}

		const FString PackagePath = MakePackagePath(ShopsRoot(), TEXT("DA_Shop_DebugSnake"));
		UPackage* Pkg = FindOrCreatePackage(PackagePath);
		if (!Pkg) { return nullptr; }

		UShopDefinition* Shop = CreateOrReplaceAsset<UShopDefinition>(Pkg, TEXT("DA_Shop_DebugSnake"));
		if (!Shop) { return nullptr; }

		Shop->ShopId = TEXT("Shop.DebugSnake");
		Shop->DisplayName = FText::FromString(TEXT("蛇巢调试商店"));
		Shop->Offers =
		{
			MakeOffer(PoisonFang, 0),
			MakeOffer(ChifuGongyi, 2),
			MakeOffer(ZhaoguangMudie, 2),
			MakeOffer(BugGirlBag, 3),
			MakeOffer(PoisonNeedle, 2),
			MakeOffer(ChitinWard, 1),
			MakeOffer(AntennaSearch, 2),
			MakeOffer(MoltCut, 2),
			MakeOffer(LightHusk, 1),
			MakeOffer(SilklineFeint, 2),
		};

		SaveAssetPackage(Pkg, Shop, PackagePath);
		return Shop;
	}
}
