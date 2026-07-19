// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Components/WacomBattleEnemyPartFlipbookLayerComponent.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleEnemyFlipbookLayerPlaybackContractSpec,
	"Wacom.UI.Battle.EnemyScene.FlipbookPlayback.PreservesPaperFlipbookTickContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleEnemyFlipbookLayerPlaybackContractSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleEnemyPartFlipbookLayerComponent> Layer(
		NewObject<UWacomBattleEnemyPartFlipbookLayerComponent>(
			GetTransientPackage(), NAME_None, RF_Transient));

	if (!TestNotNull(TEXT("Enemy Part Flipbook Layer"), Layer.Get()))
	{
		return false;
	}

	TestTrue(
		TEXT("Enemy Part Flipbook Layer retains the Paper2D component tick used to advance frames"),
		Layer->PrimaryComponentTick.bCanEverTick);
	TestTrue(
		TEXT("Enemy Part Flipbook Layer animates in editor and Blueprint viewports"),
		Layer->bTickInEditor);
	TestTrue(
		TEXT("Enemy Part Flipbook Layer preserves Paper2D autoplay by default"),
		Layer->IsPlaying());

	return true;
}
