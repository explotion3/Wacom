// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_AUTOMATION_TESTS

class AActor;
class AWacomBattleSceneClickRouterPlayerControllerTest;
class UBattleHUD;
class UPrimitiveComponent;
struct FWacomInteractionTargetHandle;

struct FWacomBattleSceneTargetClickTestAccess
{
	static void SetHUD(
		AWacomBattleSceneClickRouterPlayerControllerTest* PC,
		UBattleHUD* HUD);
	static void SetHit(
		AWacomBattleSceneClickRouterPlayerControllerTest* PC,
		AActor* Actor,
		UPrimitiveComponent* Component = nullptr);
	static void ClearHit(AWacomBattleSceneClickRouterPlayerControllerTest* PC);
	static bool RouteClick(AWacomBattleSceneClickRouterPlayerControllerTest* PC);
	static bool ProbeTarget(
		const AWacomBattleSceneClickRouterPlayerControllerTest* PC,
		FWacomInteractionTargetHandle& OutHandle);
	static bool ProbeTargetAtWidgetPosition(
		const AWacomBattleSceneClickRouterPlayerControllerTest* PC,
		const FVector2D& WidgetPosition,
		FWacomInteractionTargetHandle& OutHandle);
	static bool InputRightMousePressed(AWacomBattleSceneClickRouterPlayerControllerTest* PC);
	static bool InputLeftMouseReleased(AWacomBattleSceneClickRouterPlayerControllerTest* PC);
};

#endif
