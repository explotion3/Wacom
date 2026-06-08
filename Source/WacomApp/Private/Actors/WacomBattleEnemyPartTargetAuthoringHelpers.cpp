// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomBattleEnemyPartTargetAuthoringHelpers.h"

#include "Components/BoxComponent.h"
#include "Components/WacomBattleEnemyPartWorldTargetBridgeComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "Tags/WacomGameplayTags.h"

namespace WacomBattleEnemyPartTargetAuthoring
{
	void ConfigureHitBoundsComponent(
		UBoxComponent* HitBounds,
		const FVector& HitBoundsExtent)
	{
		if (!HitBounds)
		{
			return;
		}

		HitBounds->SetBoxExtent(HitBoundsExtent);
		HitBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		HitBounds->SetCollisionObjectType(ECC_WorldDynamic);
		HitBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
		HitBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		HitBounds->SetGenerateOverlapEvents(false);
	}

	void SyncTargetFacade(
		UBoxComponent* HitBounds,
		UWacomInteractionTargetComponent* InteractionTargetComponent,
		UWacomBattleEnemyPartWorldTargetBridgeComponent* WorldTargetBridgeComponent,
		FName EffectivePartId,
		FName EnemySlotId,
		FName PartSlotId,
		const FVector& HitBoundsExtent)
	{
		ConfigureHitBoundsComponent(HitBounds, HitBoundsExtent);

		if (InteractionTargetComponent)
		{
			InteractionTargetComponent->SetStableTargetId(EffectivePartId);
			InteractionTargetComponent->SetInteractionTargetTag(WacomTags::Interaction_Target_Battle_EnemyPart);
		}

		if (WorldTargetBridgeComponent)
		{
			WorldTargetBridgeComponent->SetPartId(EffectivePartId);
			WorldTargetBridgeComponent->SetBattlePartSlotIdentity(
				NAME_None,
				EnemySlotId,
				PartSlotId);
			WorldTargetBridgeComponent->bAutoConfigureInteractionTarget = true;
		}
	}

	FInteractionTargetDebug BuildInteractionTargetDebug(
		const UWacomInteractionTargetComponent* InteractionTargetComponent,
		FName EffectivePartId)
	{
		FInteractionTargetDebug Debug;
		if (!InteractionTargetComponent)
		{
			return Debug;
		}

		Debug.bConfigured =
			InteractionTargetComponent->GetInteractionTargetTag().MatchesTagExact(
				WacomTags::Interaction_Target_Battle_EnemyPart)
			&& InteractionTargetComponent->GetStableTargetId() == EffectivePartId;
		Debug.TargetId = InteractionTargetComponent->GetTargetId();
		Debug.StableTargetId = InteractionTargetComponent->GetStableTargetId();
		return Debug;
	}
}
