// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattleEnemyPartFlipbookLayerComponent.h"

#include "Components/WacomBattleEnemyPartComponent.h"

UWacomBattleEnemyPartFlipbookLayerComponent::UWacomBattleEnemyPartFlipbookLayerComponent()
{
	// UPaperFlipbookComponent advances playback from its component tick. Keep that
	// base contract in both PIE and editor viewports; runtime state controls
	// playback through Play/Stop instead of disabling the component tick.
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	SetCastShadow(false);
}

void UWacomBattleEnemyPartFlipbookLayerComponent::OnRegister()
{
	Super::OnRegister();
	SetPlaybackPosition(FMath::Max(0.0f, InitialPlaybackPositionSeconds), false);
	if (UWacomBattleEnemyPartComponent* Part = Cast<UWacomBattleEnemyPartComponent>(GetAttachParent()))
	{
		Part->NotifyTypedChildTopologyChanged();
	}
}

void UWacomBattleEnemyPartFlipbookLayerComponent::OnUnregister()
{
	if (UWacomBattleEnemyPartComponent* Part = Cast<UWacomBattleEnemyPartComponent>(GetAttachParent()))
	{
		Part->NotifyTypedChildTopologyChanged();
	}
	Super::OnUnregister();
}

#if WITH_EDITOR
EDataValidationResult UWacomBattleEnemyPartFlipbookLayerComponent::IsDataValid(
	FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (LayerId.IsNone())
	{
		Context.AddError(FText::FromString(TEXT("Enemy Part Flipbook Layer 的 LayerId 不能为空。")));
		Result = EDataValidationResult::Invalid;
	}
	if (!FMath::IsFinite(InitialPlaybackPositionSeconds) || InitialPlaybackPositionSeconds < 0.0f)
	{
		Context.AddError(FText::FromString(TEXT("InitialPlaybackPositionSeconds 必须是非负有限秒数。")));
		Result = EDataValidationResult::Invalid;
	}
	if (DestroyedFlipbook
		&& (!FMath::IsFinite(DestroyedPlayRate) || DestroyedPlayRate <= 0.0f))
	{
		Context.AddError(FText::FromString(TEXT("DestroyedPlayRate 必须是有限正数。")));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
#endif
