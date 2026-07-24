// Copyright Wacom. All Rights Reserved.

#include "UI/BattleSceneTargetClickTestAccess.h"

#if WITH_AUTOMATION_TESTS

#include "Components/PrimitiveComponent.h"
#include "Components/WacomBattleEnemyPartComponent.h"
#include "../../../WacomApp/Private/Interaction/WacomInteractionTargetHitResolver.h"
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

void FWacomBattleSceneTargetClickTestAccess::SetPartHit(
	AWacomBattleSceneClickRouterPlayerControllerTest* PC,
	AActor* Actor,
	UWacomBattleEnemyPartComponent* Part)
{
	UPrimitiveComponent* InteractionComponent = nullptr;
	if (Part)
	{
		for (USceneComponent* Child : Part->GetAttachChildren())
		{
			UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Child);
			if (!Primitive)
			{
				continue;
			}
			FHitResult CandidateHit;
			CandidateHit.HitObjectHandle = FActorInstanceHandle(Actor);
			CandidateHit.Component = Primitive;
			if (WacomInteractionTargetHitResolver::BuildWorldTargetHandleFromHit(
				CandidateHit).IsValid())
			{
				InteractionComponent = Primitive;
				break;
			}
		}
	}
	SetHit(PC, Actor, InteractionComponent);
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

bool FWacomBattleSceneTargetClickTestAccess::InputKey(
	AWacomBattleSceneClickRouterPlayerControllerTest* PC,
	const FKey& Key,
	EInputEvent Event)
{
	return PC ? PC->InputKeyForTest(Key, Event) : false;
}

void FWacomBattleSceneTargetClickTestAccess::PressWaitShortcut(
	AWacomBattleSceneClickRouterPlayerControllerTest* PC)
{
	if (PC)
	{
		PC->PressWaitShortcutForTest();
	}
}

void FWacomBattleSceneTargetClickTestAccess::PressEndTurnShortcut(
	AWacomBattleSceneClickRouterPlayerControllerTest* PC)
{
	if (PC)
	{
		PC->PressEndTurnShortcutForTest();
	}
}

void FWacomBattleSceneTargetClickTestAccess::PressPlayCardShortcut(
	AWacomBattleSceneClickRouterPlayerControllerTest* PC,
	int32 OneBasedIndex)
{
	if (PC)
	{
		PC->PressPlayCardShortcutForTest(OneBasedIndex);
	}
}

#endif
