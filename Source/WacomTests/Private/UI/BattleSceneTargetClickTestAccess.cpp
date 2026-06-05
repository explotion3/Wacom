// Copyright Wacom. All Rights Reserved.

#include "UI/BattleSceneTargetClickTestAccess.h"

#if WITH_AUTOMATION_TESTS

#include "UI/BattleWidgetSpecReceiver.h"

void FWacomBattleSceneTargetClickTestAccess::SetHUD(
	AWacomBattleSceneClickRouterPlayerControllerTest* PC,
	UBattleHUD* HUD)
{
	if (PC)
	{
		PC->SetBattleSceneClickHUDForTest(HUD);
	}
}

void FWacomBattleSceneTargetClickTestAccess::SetHit(
	AWacomBattleSceneClickRouterPlayerControllerTest* PC,
	AActor* Actor,
	UPrimitiveComponent* Component)
{
	if (PC)
	{
		PC->SetBattleSceneClickHitForTest(Actor, Component);
	}
}

void FWacomBattleSceneTargetClickTestAccess::ClearHit(
	AWacomBattleSceneClickRouterPlayerControllerTest* PC)
{
	if (PC)
	{
		PC->ClearBattleSceneClickHitForTest();
	}
}

bool FWacomBattleSceneTargetClickTestAccess::RouteClick(
	AWacomBattleSceneClickRouterPlayerControllerTest* PC)
{
	return PC ? PC->RouteBattleSceneTargetClickForTest() : false;
}

bool FWacomBattleSceneTargetClickTestAccess::ProbeTarget(
	const AWacomBattleSceneClickRouterPlayerControllerTest* PC,
	FWacomInteractionTargetHandle& OutHandle)
{
	return PC ? PC->ProbeBattleSceneTargetForTest(OutHandle) : false;
}

bool FWacomBattleSceneTargetClickTestAccess::ProbeTargetAtWidgetPosition(
	const AWacomBattleSceneClickRouterPlayerControllerTest* PC,
	const FVector2D& WidgetPosition,
	FWacomInteractionTargetHandle& OutHandle)
{
	return PC
		? PC->ProbeBattleSceneTargetAtWidgetPositionForTest(WidgetPosition, OutHandle)
		: false;
}

bool FWacomBattleSceneTargetClickTestAccess::InputRightMousePressed(
	AWacomBattleSceneClickRouterPlayerControllerTest* PC)
{
	return PC ? PC->InputRightMousePressedForTest() : false;
}

bool FWacomBattleSceneTargetClickTestAccess::InputLeftMouseReleased(
	AWacomBattleSceneClickRouterPlayerControllerTest* PC)
{
	return PC ? PC->InputLeftMouseReleasedForTest() : false;
}

#endif
