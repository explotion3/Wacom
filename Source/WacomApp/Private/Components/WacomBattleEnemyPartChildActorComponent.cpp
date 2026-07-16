// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattleEnemyPartChildActorComponent.h"

#include "Actors/WacomBattleEnemyPartActor.h"

void UWacomBattleEnemyPartChildActorComponent::SetStoredPartIdentity(
	FName InPartSlotId,
	FName InPartId)
{
	StoredPartSlotId = InPartSlotId;
	StoredPartId = InPartId;
	ApplyStoredPartIdentity();
}

void UWacomBattleEnemyPartChildActorComponent::OnRegister()
{
	Super::OnRegister();
	ApplyStoredPartIdentity();
}

#if WITH_EDITOR
void UWacomBattleEnemyPartChildActorComponent::PostEditUndo()
{
	Super::PostEditUndo();
	ApplyStoredPartIdentity();
}
#endif

void UWacomBattleEnemyPartChildActorComponent::ApplyStoredPartIdentity() const
{
	AWacomBattleEnemyPartActor* PartActor =
		Cast<AWacomBattleEnemyPartActor>(GetChildActor());
	if (!PartActor || StoredPartSlotId.IsNone() || StoredPartId.IsNone())
	{
		return;
	}

	PartActor->PartSlotId = StoredPartSlotId;
	PartActor->PartId = StoredPartId;
	PartActor->RefreshAuthoringState();
}
