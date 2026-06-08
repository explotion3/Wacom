// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UBoxComponent;
class UWacomBattleEnemyPartWorldTargetBridgeComponent;
class UWacomInteractionTargetComponent;

namespace WacomBattleEnemyPartTargetAuthoring
{
	struct FInteractionTargetDebug
	{
		bool bConfigured = false;
		FGuid TargetId;
		FName StableTargetId = NAME_None;
	};

	void ConfigureHitBoundsComponent(
		UBoxComponent* HitBounds,
		const FVector& HitBoundsExtent);

	void SyncTargetFacade(
		UBoxComponent* HitBounds,
		UWacomInteractionTargetComponent* InteractionTargetComponent,
		UWacomBattleEnemyPartWorldTargetBridgeComponent* WorldTargetBridgeComponent,
		FName EffectivePartId,
		FName EnemySlotId,
		FName PartSlotId,
		const FVector& HitBoundsExtent);

	FInteractionTargetDebug BuildInteractionTargetDebug(
		const UWacomInteractionTargetComponent* InteractionTargetComponent,
		FName EffectivePartId);
}
