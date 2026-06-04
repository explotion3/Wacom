// Copyright Wacom. All Rights Reserved.

#include "Fixtures/GeneratedBattleContentTestAssets.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Misc/AutomationTest.h"
#include "Shops/ShopDefinition.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	template <typename T>
	T* LoadRequiredAsset(const TCHAR* Path, FAutomationTestBase& Test)
	{
		T* Asset = LoadObject<T>(nullptr, Path);
		Test.TestNotNull(FString::Printf(TEXT("Asset loads: %s"), Path), Asset);
		return Asset;
	}
}

const TCHAR* FWacomGeneratedBattleContentAssets::BugGirlPath()
{
	return TEXT("/Game/Wacom/Data/Characters/DA_Character_BugGirl.DA_Character_BugGirl");
}

const TCHAR* FWacomGeneratedBattleContentAssets::LeftHandPath()
{
	return TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_LeftHand.DA_Card_LeftHand");
}

const TCHAR* FWacomGeneratedBattleContentAssets::RightHandPath()
{
	return TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_RightHand.DA_Card_RightHand");
}

const TCHAR* FWacomGeneratedBattleContentAssets::ZhaoguangMudiePath()
{
	return TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_ZhaoguangMudie.DA_Card_ZhaoguangMudie");
}

const TCHAR* FWacomGeneratedBattleContentAssets::FuxiaoFeiePath()
{
	return TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_FuxiaoFeie.DA_Card_FuxiaoFeie");
}

const TCHAR* FWacomGeneratedBattleContentAssets::ChifuGongyiPath()
{
	return TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_ChifuGongyi.DA_Card_ChifuGongyi");
}

const TCHAR* FWacomGeneratedBattleContentAssets::ShuoguangDiePath()
{
	return TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_ShuoguangDie.DA_Card_ShuoguangDie");
}

const TCHAR* FWacomGeneratedBattleContentAssets::MulingPath()
{
	return TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_Muling.DA_Card_Muling");
}

const TCHAR* FWacomGeneratedBattleContentAssets::BugGirlBagPath()
{
	return TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_BugGirlBag.DA_Card_BugGirlBag");
}

const TCHAR* FWacomGeneratedBattleContentAssets::ZhujianRongnangPath()
{
	return TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_ZhujianRongnang.DA_Card_ZhujianRongnang");
}

const TCHAR* FWacomGeneratedBattleContentAssets::MuseiYinchongdengPath()
{
	return TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_MuseiYinchongdeng.DA_Card_MuseiYinchongdeng");
}

const TCHAR* FWacomGeneratedBattleContentAssets::DebugKeyPath()
{
	return TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_DebugKey.DA_Card_DebugKey");
}

const TCHAR* FWacomGeneratedBattleContentAssets::AddCostToSelectedHandCardPath()
{
	return TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_Test_AddCostToSelectedHand.DA_Card_Test_AddCostToSelectedHand");
}

const TCHAR* FWacomGeneratedBattleContentAssets::ReduceCostToSelectedHandCardPath()
{
	return TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_Test_ReduceCostToSelectedHand.DA_Card_Test_ReduceCostToSelectedHand");
}

const TCHAR* FWacomGeneratedBattleContentAssets::TargetCost3CardPath()
{
	return TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_Test_TargetCost3.DA_Card_Test_TargetCost3");
}

const TCHAR* FWacomGeneratedBattleContentAssets::TargetCompanionCardPath()
{
	return TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_Test_TargetCompanion.DA_Card_Test_TargetCompanion");
}

const TCHAR* FWacomGeneratedBattleContentAssets::RequireCompanionTargetCardPath()
{
	return TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_Test_RequireCompanionTarget.DA_Card_Test_RequireCompanionTarget");
}

const TCHAR* FWacomGeneratedBattleContentAssets::BlockWeaponTargetCardPath()
{
	return TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_Test_BlockWeaponTarget.DA_Card_Test_BlockWeaponTarget");
}

const TCHAR* FWacomGeneratedBattleContentAssets::DiscardSelectedHandCardPath()
{
	return TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_Test_DiscardSelectedHandCard.DA_Card_Test_DiscardSelectedHandCard");
}

const TCHAR* FWacomGeneratedBattleContentAssets::ExhaustSelectedHandCardPath()
{
	return TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_Test_ExhaustSelectedHandCard.DA_Card_Test_ExhaustSelectedHandCard");
}

const TCHAR* FWacomGeneratedBattleContentAssets::BadgeDamagePoisonCardPath()
{
	return TEXT("/Game/Wacom/Data/Cards/BugGirl/BadgeDisplayTests/DA_Card_Test_BadgeDamagePoison.DA_Card_Test_BadgeDamagePoison");
}

const TCHAR* FWacomGeneratedBattleContentAssets::BadgeShieldHealCardPath()
{
	return TEXT("/Game/Wacom/Data/Cards/BugGirl/BadgeDisplayTests/DA_Card_Test_BadgeShieldHeal.DA_Card_Test_BadgeShieldHeal");
}

const TCHAR* FWacomGeneratedBattleContentAssets::BadgeDamageShieldHealCardPath()
{
	return TEXT("/Game/Wacom/Data/Cards/BugGirl/BadgeDisplayTests/DA_Card_Test_BadgeDamageShieldHeal.DA_Card_Test_BadgeDamageShieldHeal");
}

const TCHAR* FWacomGeneratedBattleContentAssets::BadgeAllRuntimeSupportedCardPath()
{
	return TEXT("/Game/Wacom/Data/Cards/BugGirl/BadgeDisplayTests/DA_Card_Test_BadgeAllRuntimeSupported.DA_Card_Test_BadgeAllRuntimeSupported");
}

const TCHAR* FWacomGeneratedBattleContentAssets::PoisonFangPath()
{
	return TEXT("/Game/Wacom/Data/Cards/Rewards/DA_Card_PoisonFang.DA_Card_PoisonFang");
}

const TCHAR* FWacomGeneratedBattleContentAssets::PoisonNeedlePath()
{
	return TEXT("/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_PoisonNeedle.DA_Card_Starter_PoisonNeedle");
}

const TCHAR* FWacomGeneratedBattleContentAssets::ChitinWardPath()
{
	return TEXT("/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_ChitinWard.DA_Card_Starter_ChitinWard");
}

const TCHAR* FWacomGeneratedBattleContentAssets::AntennaSearchPath()
{
	return TEXT("/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_AntennaSearch.DA_Card_Starter_AntennaSearch");
}

const TCHAR* FWacomGeneratedBattleContentAssets::MoltCutPath()
{
	return TEXT("/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_MoltCut.DA_Card_Starter_MoltCut");
}

const TCHAR* FWacomGeneratedBattleContentAssets::LightHuskPath()
{
	return TEXT("/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_LightHusk.DA_Card_Starter_LightHusk");
}

const TCHAR* FWacomGeneratedBattleContentAssets::SilklineFeintPath()
{
	return TEXT("/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_SilklineFeint.DA_Card_Starter_SilklineFeint");
}

const TCHAR* FWacomGeneratedBattleContentAssets::SnakePath()
{
	return TEXT("/Game/Wacom/Data/Enemies/Snake/DA_Enemy_Snake.DA_Enemy_Snake");
}

const TCHAR* FWacomGeneratedBattleContentAssets::SnakeHeadPath()
{
	return TEXT("/Game/Wacom/Data/Enemies/Snake/DA_Part_Snake_Head.DA_Part_Snake_Head");
}

const TCHAR* FWacomGeneratedBattleContentAssets::SnakeBodyPath()
{
	return TEXT("/Game/Wacom/Data/Enemies/Snake/DA_Part_Snake_Body.DA_Part_Snake_Body");
}

const TCHAR* FWacomGeneratedBattleContentAssets::SnakeTailPath()
{
	return TEXT("/Game/Wacom/Data/Enemies/Snake/DA_Part_Snake_Tail.DA_Part_Snake_Tail");
}

const TCHAR* FWacomGeneratedBattleContentAssets::DebugSnakeShopPath()
{
	return TEXT("/Game/Wacom/Data/Shops/DA_Shop_DebugSnake.DA_Shop_DebugSnake");
}

TArray<const TCHAR*> FWacomGeneratedBattleContentAssets::StarterPackCardPaths()
{
	return {
		PoisonNeedlePath(),
		ChitinWardPath(),
		AntennaSearchPath(),
		MoltCutPath(),
		LightHuskPath(),
		SilklineFeintPath()
	};
}

TArray<const TCHAR*> FWacomGeneratedBattleContentAssets::DebugAndTestCardPaths()
{
	return {
		DebugKeyPath(),
		AddCostToSelectedHandCardPath(),
		ReduceCostToSelectedHandCardPath(),
		TargetCost3CardPath(),
		TargetCompanionCardPath(),
		RequireCompanionTargetCardPath(),
		BlockWeaponTargetCardPath(),
		DiscardSelectedHandCardPath(),
		ExhaustSelectedHandCardPath()
	};
}

TArray<const TCHAR*> FWacomGeneratedBattleContentAssets::BadgeDisplayTestCardPaths()
{
	return {
		BadgeDamagePoisonCardPath(),
		BadgeShieldHealCardPath(),
		BadgeDamageShieldHealCardPath(),
		BadgeAllRuntimeSupportedCardPath()
	};
}

TArray<const TCHAR*> FWacomGeneratedBattleContentAssets::GeneratedDefinitionCardPaths()
{
	return {
		LeftHandPath(),
		RightHandPath(),
		ZhaoguangMudiePath(),
		FuxiaoFeiePath(),
		ChifuGongyiPath(),
		ShuoguangDiePath(),
		MulingPath(),
		BugGirlBagPath(),
		ZhujianRongnangPath(),
		MuseiYinchongdengPath(),
		PoisonFangPath(),
		PoisonNeedlePath(),
		ChitinWardPath(),
		AntennaSearchPath(),
		MoltCutPath(),
		LightHuskPath(),
		SilklineFeintPath(),
		DebugKeyPath(),
		AddCostToSelectedHandCardPath(),
		ReduceCostToSelectedHandCardPath(),
		TargetCost3CardPath(),
		TargetCompanionCardPath(),
		RequireCompanionTargetCardPath(),
		BlockWeaponTargetCardPath(),
		DiscardSelectedHandCardPath(),
		ExhaustSelectedHandCardPath(),
		BadgeDamagePoisonCardPath(),
		BadgeShieldHealCardPath(),
		BadgeDamageShieldHealCardPath(),
		BadgeAllRuntimeSupportedCardPath()
	};
}

TArray<const TCHAR*> FWacomGeneratedBattleContentAssets::SnakePartPaths()
{
	return {
		SnakeHeadPath(),
		SnakeBodyPath(),
		SnakeTailPath()
	};
}

TArray<FWacomGeneratedBattleContentShopOfferExpectation> FWacomGeneratedBattleContentAssets::DebugSnakeShopOfferExpectations()
{
	return {
		{ PoisonFangPath(), 0 },
		{ ChifuGongyiPath(), 2 },
		{ ZhaoguangMudiePath(), 2 },
		{ BugGirlBagPath(), 3 },
		{ PoisonNeedlePath(), 2 },
		{ ChitinWardPath(), 1 },
		{ AntennaSearchPath(), 2 },
		{ MoltCutPath(), 2 },
		{ LightHuskPath(), 1 },
		{ SilklineFeintPath(), 2 },
		{ DebugKeyPath(), 0 },
		{ AddCostToSelectedHandCardPath(), 0 },
		{ ReduceCostToSelectedHandCardPath(), 0 },
		{ TargetCost3CardPath(), 0 },
		{ TargetCompanionCardPath(), 0 },
		{ RequireCompanionTargetCardPath(), 0 },
		{ BlockWeaponTargetCardPath(), 0 },
		{ DiscardSelectedHandCardPath(), 0 },
		{ ExhaustSelectedHandCardPath(), 0 },
		{ BadgeDamagePoisonCardPath(), 0 },
		{ BadgeShieldHealCardPath(), 0 },
		{ BadgeDamageShieldHealCardPath(), 0 },
		{ BadgeAllRuntimeSupportedCardPath(), 0 }
	};
}

UCharacterDefinition* FWacomGeneratedBattleContentAssets::LoadBugGirl(FAutomationTestBase& Test)
{
	return LoadRequiredAsset<UCharacterDefinition>(BugGirlPath(), Test);
}

UCardDefinition* FWacomGeneratedBattleContentAssets::LoadCardByPath(const TCHAR* Path, FAutomationTestBase& Test)
{
	return LoadRequiredAsset<UCardDefinition>(Path, Test);
}

UCardDefinition* FWacomGeneratedBattleContentAssets::LoadLeftHand(FAutomationTestBase& Test)
{
	return LoadCardByPath(LeftHandPath(), Test);
}

UCardDefinition* FWacomGeneratedBattleContentAssets::LoadRightHand(FAutomationTestBase& Test)
{
	return LoadCardByPath(RightHandPath(), Test);
}

UCardDefinition* FWacomGeneratedBattleContentAssets::LoadZhaoguangMudie(FAutomationTestBase& Test)
{
	return LoadCardByPath(ZhaoguangMudiePath(), Test);
}

UCardDefinition* FWacomGeneratedBattleContentAssets::LoadFuxiaoFeie(FAutomationTestBase& Test)
{
	return LoadCardByPath(FuxiaoFeiePath(), Test);
}

UCardDefinition* FWacomGeneratedBattleContentAssets::LoadChifuGongyi(FAutomationTestBase& Test)
{
	return LoadCardByPath(ChifuGongyiPath(), Test);
}

UCardDefinition* FWacomGeneratedBattleContentAssets::LoadBugGirlBag(FAutomationTestBase& Test)
{
	return LoadCardByPath(BugGirlBagPath(), Test);
}

UCardDefinition* FWacomGeneratedBattleContentAssets::LoadDebugKey(FAutomationTestBase& Test)
{
	return LoadCardByPath(DebugKeyPath(), Test);
}

UCardDefinition* FWacomGeneratedBattleContentAssets::LoadAddCostToSelectedHandCard(FAutomationTestBase& Test)
{
	return LoadCardByPath(AddCostToSelectedHandCardPath(), Test);
}

UCardDefinition* FWacomGeneratedBattleContentAssets::LoadReduceCostToSelectedHandCard(FAutomationTestBase& Test)
{
	return LoadCardByPath(ReduceCostToSelectedHandCardPath(), Test);
}

UCardDefinition* FWacomGeneratedBattleContentAssets::LoadTargetCost3Card(FAutomationTestBase& Test)
{
	return LoadCardByPath(TargetCost3CardPath(), Test);
}

UCardDefinition* FWacomGeneratedBattleContentAssets::LoadTargetCompanionCard(FAutomationTestBase& Test)
{
	return LoadCardByPath(TargetCompanionCardPath(), Test);
}

UCardDefinition* FWacomGeneratedBattleContentAssets::LoadRequireCompanionTargetCard(FAutomationTestBase& Test)
{
	return LoadCardByPath(RequireCompanionTargetCardPath(), Test);
}

UCardDefinition* FWacomGeneratedBattleContentAssets::LoadBlockWeaponTargetCard(FAutomationTestBase& Test)
{
	return LoadCardByPath(BlockWeaponTargetCardPath(), Test);
}

UCardDefinition* FWacomGeneratedBattleContentAssets::LoadDiscardSelectedHandCard(FAutomationTestBase& Test)
{
	return LoadCardByPath(DiscardSelectedHandCardPath(), Test);
}

UCardDefinition* FWacomGeneratedBattleContentAssets::LoadExhaustSelectedHandCard(FAutomationTestBase& Test)
{
	return LoadCardByPath(ExhaustSelectedHandCardPath(), Test);
}

UCardDefinition* FWacomGeneratedBattleContentAssets::LoadBadgeDamagePoisonCard(FAutomationTestBase& Test)
{
	return LoadCardByPath(BadgeDamagePoisonCardPath(), Test);
}

UCardDefinition* FWacomGeneratedBattleContentAssets::LoadBadgeShieldHealCard(FAutomationTestBase& Test)
{
	return LoadCardByPath(BadgeShieldHealCardPath(), Test);
}

UCardDefinition* FWacomGeneratedBattleContentAssets::LoadBadgeDamageShieldHealCard(FAutomationTestBase& Test)
{
	return LoadCardByPath(BadgeDamageShieldHealCardPath(), Test);
}

UCardDefinition* FWacomGeneratedBattleContentAssets::LoadBadgeAllRuntimeSupportedCard(FAutomationTestBase& Test)
{
	return LoadCardByPath(BadgeAllRuntimeSupportedCardPath(), Test);
}

UCardDefinition* FWacomGeneratedBattleContentAssets::LoadPoisonFang(FAutomationTestBase& Test)
{
	return LoadCardByPath(PoisonFangPath(), Test);
}

UCardDefinition* FWacomGeneratedBattleContentAssets::LoadPoisonNeedle(FAutomationTestBase& Test)
{
	return LoadCardByPath(PoisonNeedlePath(), Test);
}

UCardDefinition* FWacomGeneratedBattleContentAssets::LoadChitinWard(FAutomationTestBase& Test)
{
	return LoadCardByPath(ChitinWardPath(), Test);
}

UCardDefinition* FWacomGeneratedBattleContentAssets::LoadAntennaSearch(FAutomationTestBase& Test)
{
	return LoadCardByPath(AntennaSearchPath(), Test);
}

UCardDefinition* FWacomGeneratedBattleContentAssets::LoadMoltCut(FAutomationTestBase& Test)
{
	return LoadCardByPath(MoltCutPath(), Test);
}

UCardDefinition* FWacomGeneratedBattleContentAssets::LoadLightHusk(FAutomationTestBase& Test)
{
	return LoadCardByPath(LightHuskPath(), Test);
}

UCardDefinition* FWacomGeneratedBattleContentAssets::LoadSilklineFeint(FAutomationTestBase& Test)
{
	return LoadCardByPath(SilklineFeintPath(), Test);
}

TArray<UCardDefinition*> FWacomGeneratedBattleContentAssets::LoadStarterPackCards(FAutomationTestBase& Test)
{
	TArray<UCardDefinition*> Cards;
	for (const TCHAR* Path : StarterPackCardPaths())
	{
		Cards.Add(LoadCardByPath(Path, Test));
	}
	return Cards;
}

TArray<UCardDefinition*> FWacomGeneratedBattleContentAssets::LoadGeneratedDefinitionCards(FAutomationTestBase& Test)
{
	TArray<UCardDefinition*> Cards;
	for (const TCHAR* Path : GeneratedDefinitionCardPaths())
	{
		Cards.Add(LoadCardByPath(Path, Test));
	}
	return Cards;
}

UEnemyDefinition* FWacomGeneratedBattleContentAssets::LoadSnake(FAutomationTestBase& Test)
{
	return LoadRequiredAsset<UEnemyDefinition>(SnakePath(), Test);
}

UEnemyPartDefinition* FWacomGeneratedBattleContentAssets::LoadEnemyPartByPath(const TCHAR* Path, FAutomationTestBase& Test)
{
	return LoadRequiredAsset<UEnemyPartDefinition>(Path, Test);
}

UEnemyPartDefinition* FWacomGeneratedBattleContentAssets::LoadSnakeHead(FAutomationTestBase& Test)
{
	return LoadEnemyPartByPath(SnakeHeadPath(), Test);
}

UEnemyPartDefinition* FWacomGeneratedBattleContentAssets::LoadSnakeBody(FAutomationTestBase& Test)
{
	return LoadEnemyPartByPath(SnakeBodyPath(), Test);
}

UEnemyPartDefinition* FWacomGeneratedBattleContentAssets::LoadSnakeTail(FAutomationTestBase& Test)
{
	return LoadEnemyPartByPath(SnakeTailPath(), Test);
}

TArray<UEnemyPartDefinition*> FWacomGeneratedBattleContentAssets::LoadSnakeParts(FAutomationTestBase& Test)
{
	TArray<UEnemyPartDefinition*> Parts;
	for (const TCHAR* Path : SnakePartPaths())
	{
		Parts.Add(LoadEnemyPartByPath(Path, Test));
	}
	return Parts;
}

UShopDefinition* FWacomGeneratedBattleContentAssets::LoadDebugSnakeShop(FAutomationTestBase& Test)
{
	return LoadRequiredAsset<UShopDefinition>(DebugSnakeShopPath(), Test);
}
