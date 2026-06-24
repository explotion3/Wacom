// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FAutomationTestBase;
class UCardDefinition;
class UCharacterDefinition;
class UEnemyBehaviorDefinition;
class UEnemyDefinition;
class UEnemyPartDefinition;
class UEncounterDefinition;
class UShopDefinition;

struct FWacomGeneratedBattleContentShopOfferExpectation
{
	const TCHAR* ObjectPath = nullptr;
	int32 Price = 0;
};

/**
 * WacomTests 专用的生成战斗内容资产入口。
 *
 * 这里只集中测试需要的真实 Content 路径；不要把这些路径作为运行时加载、
 * 内容生成或策划配置来源。
 */
struct WACOMTESTS_API FWacomGeneratedBattleContentAssets
{
	static const TCHAR* BugGirlPath();
	static const TCHAR* LeftHandPath();
	static const TCHAR* RightHandPath();
	static const TCHAR* ZhaoguangMudiePath();
	static const TCHAR* FuxiaoFeiePath();
	static const TCHAR* ChifuGongyiPath();
	static const TCHAR* ShuoguangDiePath();
	static const TCHAR* MulingPath();
	static const TCHAR* BugGirlBagPath();
	static const TCHAR* ZhujianRongnangPath();
	static const TCHAR* MuseiYinchongdengPath();
	static const TCHAR* DebugKeyPath();
	static const TCHAR* AddCostToSelectedHandCardPath();
	static const TCHAR* ReduceCostToSelectedHandCardPath();
	static const TCHAR* TargetCost3CardPath();
	static const TCHAR* TargetCompanionCardPath();
	static const TCHAR* RequireCompanionTargetCardPath();
	static const TCHAR* BlockWeaponTargetCardPath();
	static const TCHAR* DiscardSelectedHandCardPath();
	static const TCHAR* ExhaustSelectedHandCardPath();
	static const TCHAR* DrawByCostCardPath();
	static const TCHAR* BadgeDamagePoisonCardPath();
	static const TCHAR* BadgeShieldHealCardPath();
	static const TCHAR* BadgeDamageShieldHealCardPath();
	static const TCHAR* BadgeAllRuntimeSupportedCardPath();
	static const TCHAR* PoisonFangPath();
	static const TCHAR* PoisonNeedlePath();
	static const TCHAR* ChitinWardPath();
	static const TCHAR* AntennaSearchPath();
	static const TCHAR* MoltCutPath();
	static const TCHAR* LightHuskPath();
	static const TCHAR* SilklineFeintPath();
	static const TCHAR* SnakePath();
	static const TCHAR* SnakeBehaviorPath();
	static const TCHAR* SnakeHeadPath();
	static const TCHAR* SnakeBodyPath();
	static const TCHAR* SnakeTailPath();
	static const TCHAR* SnakeSingleEncounterPath();
	static const TCHAR* DebugSnakeShopPath();

	static TArray<const TCHAR*> StarterPackCardPaths();
	static TArray<const TCHAR*> DebugAndTestCardPaths();
	static TArray<const TCHAR*> BadgeDisplayTestCardPaths();
	static TArray<const TCHAR*> GeneratedDefinitionCardPaths();
	static TArray<const TCHAR*> SnakePartPaths();
	static TArray<FWacomGeneratedBattleContentShopOfferExpectation> DebugSnakeShopOfferExpectations();

	static UCharacterDefinition* LoadBugGirl(FAutomationTestBase& Test);
	static UCardDefinition* LoadCardByPath(const TCHAR* Path, FAutomationTestBase& Test);
	static UCardDefinition* LoadLeftHand(FAutomationTestBase& Test);
	static UCardDefinition* LoadRightHand(FAutomationTestBase& Test);
	static UCardDefinition* LoadZhaoguangMudie(FAutomationTestBase& Test);
	static UCardDefinition* LoadFuxiaoFeie(FAutomationTestBase& Test);
	static UCardDefinition* LoadChifuGongyi(FAutomationTestBase& Test);
	static UCardDefinition* LoadBugGirlBag(FAutomationTestBase& Test);
	static UCardDefinition* LoadDebugKey(FAutomationTestBase& Test);
	static UCardDefinition* LoadAddCostToSelectedHandCard(FAutomationTestBase& Test);
	static UCardDefinition* LoadReduceCostToSelectedHandCard(FAutomationTestBase& Test);
	static UCardDefinition* LoadTargetCost3Card(FAutomationTestBase& Test);
	static UCardDefinition* LoadTargetCompanionCard(FAutomationTestBase& Test);
	static UCardDefinition* LoadRequireCompanionTargetCard(FAutomationTestBase& Test);
	static UCardDefinition* LoadBlockWeaponTargetCard(FAutomationTestBase& Test);
	static UCardDefinition* LoadDiscardSelectedHandCard(FAutomationTestBase& Test);
	static UCardDefinition* LoadExhaustSelectedHandCard(FAutomationTestBase& Test);
	static UCardDefinition* LoadDrawByCostCard(FAutomationTestBase& Test);
	static UCardDefinition* LoadBadgeDamagePoisonCard(FAutomationTestBase& Test);
	static UCardDefinition* LoadBadgeShieldHealCard(FAutomationTestBase& Test);
	static UCardDefinition* LoadBadgeDamageShieldHealCard(FAutomationTestBase& Test);
	static UCardDefinition* LoadBadgeAllRuntimeSupportedCard(FAutomationTestBase& Test);
	static UCardDefinition* LoadPoisonFang(FAutomationTestBase& Test);
	static UCardDefinition* LoadPoisonNeedle(FAutomationTestBase& Test);
	static UCardDefinition* LoadChitinWard(FAutomationTestBase& Test);
	static UCardDefinition* LoadAntennaSearch(FAutomationTestBase& Test);
	static UCardDefinition* LoadMoltCut(FAutomationTestBase& Test);
	static UCardDefinition* LoadLightHusk(FAutomationTestBase& Test);
	static UCardDefinition* LoadSilklineFeint(FAutomationTestBase& Test);
	static TArray<UCardDefinition*> LoadStarterPackCards(FAutomationTestBase& Test);
	static TArray<UCardDefinition*> LoadGeneratedDefinitionCards(FAutomationTestBase& Test);

	static UEnemyDefinition* LoadSnake(FAutomationTestBase& Test);
	static UEnemyBehaviorDefinition* LoadSnakeBehavior(FAutomationTestBase& Test);
	static UEnemyPartDefinition* LoadEnemyPartByPath(const TCHAR* Path, FAutomationTestBase& Test);
	static UEnemyPartDefinition* LoadSnakeHead(FAutomationTestBase& Test);
	static UEnemyPartDefinition* LoadSnakeBody(FAutomationTestBase& Test);
	static UEnemyPartDefinition* LoadSnakeTail(FAutomationTestBase& Test);
	static TArray<UEnemyPartDefinition*> LoadSnakeParts(FAutomationTestBase& Test);
	static UEncounterDefinition* LoadSnakeSingleEncounter(FAutomationTestBase& Test);

	static UShopDefinition* LoadDebugSnakeShop(FAutomationTestBase& Test);
};
