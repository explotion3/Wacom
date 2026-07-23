// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Components/BoxComponent.h"
#include "Interaction/WacomInteractionTargetProvider.h"
#include "WacomBattleEnemyPartFallbackCollisionComponent.generated.h"

class UWacomBattleEnemyPartComponent;

/**
 * Runtime-only emergency collision for a Battle enemy Part whose formal Sprite
 * collision cannot be resolved. Never authored or serialized into a Host.
 */
UCLASS(Transient, NotBlueprintable)
class UWacomBattleEnemyPartFallbackCollisionComponent final : public UBoxComponent,
	public IWacomInteractionTargetProvider
{
	GENERATED_BODY()

public:
	UWacomBattleEnemyPartFallbackCollisionComponent();

	void InitializeForPart(UWacomBattleEnemyPartComponent& InPart);
	void ConfigureFallbackBounds(
		const FVector& RelativeCenter,
		const FVector& HalfExtent,
		bool bEnableCollision);
	void DisableFallbackCollision();

	virtual FWacomInteractionTargetHandle BuildWorldTargetHandle() const override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UWacomBattleEnemyPartComponent> InteractionPart = nullptr;
};
