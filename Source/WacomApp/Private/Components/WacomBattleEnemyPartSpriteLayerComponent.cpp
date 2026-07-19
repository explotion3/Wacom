// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattleEnemyPartSpriteLayerComponent.h"

#include "Components/WacomBattleEnemyPartComponent.h"

UWacomBattleEnemyPartSpriteLayerComponent::UWacomBattleEnemyPartSpriteLayerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	SetCastShadow(false);
}

void UWacomBattleEnemyPartSpriteLayerComponent::OnRegister()
{
	Super::OnRegister();
	if (UWacomBattleEnemyPartComponent* Part = Cast<UWacomBattleEnemyPartComponent>(GetAttachParent()))
	{
		Part->NotifyTypedChildTopologyChanged();
	}
}

void UWacomBattleEnemyPartSpriteLayerComponent::OnUnregister()
{
	if (UWacomBattleEnemyPartComponent* Part = Cast<UWacomBattleEnemyPartComponent>(GetAttachParent()))
	{
		Part->NotifyTypedChildTopologyChanged();
	}
	Super::OnUnregister();
}

#if WITH_EDITOR
EDataValidationResult UWacomBattleEnemyPartSpriteLayerComponent::IsDataValid(
	FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (LayerId.IsNone())
	{
		Context.AddError(FText::FromString(TEXT("Enemy Part Sprite Layer 的 LayerId 不能为空。")));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
#endif
