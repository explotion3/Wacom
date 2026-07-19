// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattleEnemyPartImpactAnchorComponent.h"

#include "Components/WacomBattleEnemyPartComponent.h"

UWacomBattleEnemyPartImpactAnchorComponent::UWacomBattleEnemyPartImpactAnchorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWacomBattleEnemyPartImpactAnchorComponent::OnRegister()
{
	Super::OnRegister();
	if (UWacomBattleEnemyPartComponent* Part = Cast<UWacomBattleEnemyPartComponent>(GetAttachParent()))
	{
		Part->NotifyTypedChildTopologyChanged();
	}
}

void UWacomBattleEnemyPartImpactAnchorComponent::OnUnregister()
{
	if (UWacomBattleEnemyPartComponent* Part = Cast<UWacomBattleEnemyPartComponent>(GetAttachParent()))
	{
		Part->NotifyTypedChildTopologyChanged();
	}
	Super::OnUnregister();
}
