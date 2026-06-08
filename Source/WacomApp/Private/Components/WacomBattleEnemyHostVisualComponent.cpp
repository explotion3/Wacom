// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattleEnemyHostVisualComponent.h"

#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"

UWacomBattleEnemyHostVisualComponent::UWacomBattleEnemyHostVisualComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWacomBattleEnemyHostVisualComponent::RefreshHostVisual(
	USceneComponent* AttachRoot,
	bool bUseFlipbook,
	UPaperSprite* HostSprite,
	UPaperFlipbook* HostFlipbook,
	const FVector& RelativeLocation,
	const FRotator& RelativeRotation,
	const FVector& RelativeScale3D,
	int32 SortOrder,
	const FLinearColor& Tint,
	UMaterialInterface* MaterialOverride,
	bool bCastShadow,
	bool bVisible,
	float FlipbookPlayRate,
	bool bLoopFlipbook,
	float FlipbookStartTimeSeconds,
	bool bAutoPlayFlipbook)
{
	ClearGeneratedHostVisual();

	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return;
	}

	USceneComponent* AttachParent = AttachRoot;
	if (!AttachParent)
	{
		AttachParent = Owner->GetRootComponent();
	}
	if (!AttachParent)
	{
		return;
	}

	if (bUseFlipbook)
	{
		if (!HostFlipbook || !bVisible)
		{
			return;
		}

		UPaperFlipbookComponent* FlipbookComponent =
			NewObject<UPaperFlipbookComponent>(
				Owner,
				TEXT("HostVisual_Flipbook"),
				RF_Transactional | RF_Transient);
		if (!FlipbookComponent)
		{
			return;
		}

		FlipbookComponent->SetupAttachment(AttachParent);
		FlipbookComponent->SetFlipbook(HostFlipbook);
		FlipbookComponent->SetRelativeLocation(RelativeLocation);
		FlipbookComponent->SetRelativeRotation(RelativeRotation);
		FlipbookComponent->SetRelativeScale3D(RelativeScale3D);
		FlipbookComponent->SetSpriteColor(Tint);
		FlipbookComponent->SetTranslucentSortPriority(SortOrder);
		FlipbookComponent->SetVisibility(bVisible, true);
		if (MaterialOverride)
		{
			FlipbookComponent->SetMaterial(0, MaterialOverride);
		}
		FlipbookComponent->SetCastShadow(bCastShadow);
		FlipbookComponent->bCastDynamicShadow = bCastShadow;
		FlipbookComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		FlipbookComponent->SetGenerateOverlapEvents(false);
		FlipbookComponent->SetLooping(bLoopFlipbook);
		FlipbookComponent->SetPlayRate(FlipbookPlayRate);
		FlipbookComponent->SetPlaybackPosition(FlipbookStartTimeSeconds, false);
		if (bAutoPlayFlipbook && FlipbookPlayRate > 0.0f)
		{
			FlipbookComponent->Play();
		}
		else
		{
			FlipbookComponent->Stop();
		}
		FlipbookComponent->bEditableWhenInherited = false;
		Owner->AddInstanceComponent(FlipbookComponent);
		FlipbookComponent->RegisterComponentWithWorld(World);
		GeneratedHostFlipbookVisualComponent = FlipbookComponent;
		return;
	}

	if (!HostSprite || !bVisible)
	{
		return;
	}

	UPaperSpriteComponent* SpriteComponent =
		NewObject<UPaperSpriteComponent>(
			Owner,
			TEXT("HostVisual_Sprite"),
			RF_Transactional | RF_Transient);
	if (!SpriteComponent)
	{
		return;
	}

	SpriteComponent->SetupAttachment(AttachParent);
	SpriteComponent->SetSprite(HostSprite);
	SpriteComponent->SetRelativeLocation(RelativeLocation);
	SpriteComponent->SetRelativeRotation(RelativeRotation);
	SpriteComponent->SetRelativeScale3D(RelativeScale3D);
	SpriteComponent->SetSpriteColor(Tint);
	SpriteComponent->SetTranslucentSortPriority(SortOrder);
	SpriteComponent->SetVisibility(bVisible, true);
	if (MaterialOverride)
	{
		SpriteComponent->SetMaterial(0, MaterialOverride);
	}
	SpriteComponent->SetCastShadow(bCastShadow);
	SpriteComponent->bCastDynamicShadow = bCastShadow;
	SpriteComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpriteComponent->SetGenerateOverlapEvents(false);
	SpriteComponent->bEditableWhenInherited = false;
	Owner->AddInstanceComponent(SpriteComponent);
	SpriteComponent->RegisterComponentWithWorld(World);
	GeneratedHostSpriteVisualComponent = SpriteComponent;
}

void UWacomBattleEnemyHostVisualComponent::ClearGeneratedHostVisual()
{
	if (GeneratedHostSpriteVisualComponent)
	{
		GeneratedHostSpriteVisualComponent->DestroyComponent();
		GeneratedHostSpriteVisualComponent = nullptr;
	}
	if (GeneratedHostFlipbookVisualComponent)
	{
		GeneratedHostFlipbookVisualComponent->DestroyComponent();
		GeneratedHostFlipbookVisualComponent = nullptr;
	}
}

int32 UWacomBattleEnemyHostVisualComponent::GetGeneratedHostVisualComponentCount() const
{
	return (GeneratedHostSpriteVisualComponent ? 1 : 0)
		+ (GeneratedHostFlipbookVisualComponent ? 1 : 0);
}

int32 UWacomBattleEnemyHostVisualComponent::GetRegisteredHostVisualComponentCount() const
{
	return (GeneratedHostSpriteVisualComponent && GeneratedHostSpriteVisualComponent->IsRegistered() ? 1 : 0)
		+ (GeneratedHostFlipbookVisualComponent && GeneratedHostFlipbookVisualComponent->IsRegistered() ? 1 : 0);
}

int32 UWacomBattleEnemyHostVisualComponent::GetVisibleHostVisualComponentCount() const
{
	return (GeneratedHostSpriteVisualComponent && GeneratedHostSpriteVisualComponent->IsVisible() ? 1 : 0)
		+ (GeneratedHostFlipbookVisualComponent && GeneratedHostFlipbookVisualComponent->IsVisible() ? 1 : 0);
}

void UWacomBattleEnemyHostVisualComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearGeneratedHostVisual();
	Super::EndPlay(EndPlayReason);
}
