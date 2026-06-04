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
		const FString BadgeDisplayRoot = BugGirlBadgeDisplayTestCardsRoot();
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
		UCardDefinition* DebugKey = LoadGeneratedCard(
			MakeObjectPath(MakePackagePath(BugGirlRoot, TEXT("DA_Card_DebugKey"))));
		UCardDefinition* TestAddCostToSelectedHand = LoadGeneratedCard(
			MakeObjectPath(MakePackagePath(BugGirlRoot, TEXT("DA_Card_Test_AddCostToSelectedHand"))));
		UCardDefinition* TestReduceCostToSelectedHand = LoadGeneratedCard(
			MakeObjectPath(MakePackagePath(BugGirlRoot, TEXT("DA_Card_Test_ReduceCostToSelectedHand"))));
		UCardDefinition* TestTargetCost3 = LoadGeneratedCard(
			MakeObjectPath(MakePackagePath(BugGirlRoot, TEXT("DA_Card_Test_TargetCost3"))));
		UCardDefinition* TestTargetCompanion = LoadGeneratedCard(
			MakeObjectPath(MakePackagePath(BugGirlRoot, TEXT("DA_Card_Test_TargetCompanion"))));
		UCardDefinition* TestRequireCompanionTarget = LoadGeneratedCard(
			MakeObjectPath(MakePackagePath(BugGirlRoot, TEXT("DA_Card_Test_RequireCompanionTarget"))));
		UCardDefinition* TestBlockWeaponTarget = LoadGeneratedCard(
			MakeObjectPath(MakePackagePath(BugGirlRoot, TEXT("DA_Card_Test_BlockWeaponTarget"))));
		UCardDefinition* TestDiscardSelectedHandCard = LoadGeneratedCard(
			MakeObjectPath(MakePackagePath(BugGirlRoot, TEXT("DA_Card_Test_DiscardSelectedHandCard"))));
		UCardDefinition* TestExhaustSelectedHandCard = LoadGeneratedCard(
			MakeObjectPath(MakePackagePath(BugGirlRoot, TEXT("DA_Card_Test_ExhaustSelectedHandCard"))));
		UCardDefinition* BadgeDamagePoison = LoadGeneratedCard(
			MakeObjectPath(MakePackagePath(BadgeDisplayRoot, TEXT("DA_Card_Test_BadgeDamagePoison"))));
		UCardDefinition* BadgeShieldHeal = LoadGeneratedCard(
			MakeObjectPath(MakePackagePath(BadgeDisplayRoot, TEXT("DA_Card_Test_BadgeShieldHeal"))));
		UCardDefinition* BadgeDamageShieldHeal = LoadGeneratedCard(
			MakeObjectPath(MakePackagePath(BadgeDisplayRoot, TEXT("DA_Card_Test_BadgeDamageShieldHeal"))));
		UCardDefinition* BadgeAllRuntimeSupported = LoadGeneratedCard(
			MakeObjectPath(MakePackagePath(BadgeDisplayRoot, TEXT("DA_Card_Test_BadgeAllRuntimeSupported"))));
		if (!PoisonFang || !ChifuGongyi || !ZhaoguangMudie || !BugGirlBag
			|| !PoisonNeedle || !ChitinWard || !AntennaSearch || !MoltCut || !LightHusk || !SilklineFeint
			|| !DebugKey
			|| !TestAddCostToSelectedHand || !TestReduceCostToSelectedHand || !TestTargetCost3
			|| !TestTargetCompanion || !TestRequireCompanionTarget || !TestBlockWeaponTarget
			|| !TestDiscardSelectedHandCard || !TestExhaustSelectedHandCard
			|| !BadgeDamagePoison || !BadgeShieldHeal || !BadgeDamageShieldHeal || !BadgeAllRuntimeSupported)
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
			MakeOffer(DebugKey, 0),
			MakeOffer(TestAddCostToSelectedHand, 0),
			MakeOffer(TestReduceCostToSelectedHand, 0),
			MakeOffer(TestTargetCost3, 0),
			MakeOffer(TestTargetCompanion, 0),
			MakeOffer(TestRequireCompanionTarget, 0),
			MakeOffer(TestBlockWeaponTarget, 0),
			MakeOffer(TestDiscardSelectedHandCard, 0),
			MakeOffer(TestExhaustSelectedHandCard, 0),
			MakeOffer(BadgeDamagePoison, 0),
			MakeOffer(BadgeShieldHeal, 0),
			MakeOffer(BadgeDamageShieldHeal, 0),
			MakeOffer(BadgeAllRuntimeSupported, 0),
		};

		SaveAssetPackage(Pkg, Shop, PackagePath);
		return Shop;
	}
}
