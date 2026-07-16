// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattleEnemyPartVisualLayerComponent.h"

#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"

namespace
{
	FString BuildVisualLayerComponentName(FName LayerId, int32 LayerIndex)
	{
		const FString LayerName = LayerId.IsNone()
			? FString::Printf(TEXT("Layer%d"), LayerIndex)
			: LayerId.ToString();
		return FString::Printf(TEXT("VisualLayer_%02d_%s"), LayerIndex, *LayerName);
	}

	bool VisualLayerHasAsset(const FWacomBattleEnemyPartVisualLayer& Layer)
	{
		switch (Layer.LayerMode)
		{
		case EWacomBattleEnemyPartVisualLayerMode::Flipbook:
			return Layer.Flipbook != nullptr;
		case EWacomBattleEnemyPartVisualLayerMode::StaticSprite:
		default:
			return Layer.Sprite != nullptr;
		}
	}

	FName GetVisualLayerAssetName(const FWacomBattleEnemyPartVisualLayer& Layer)
	{
		switch (Layer.LayerMode)
		{
		case EWacomBattleEnemyPartVisualLayerMode::Flipbook:
			return Layer.Flipbook ? FName(*Layer.Flipbook->GetName()) : NAME_None;
		case EWacomBattleEnemyPartVisualLayerMode::StaticSprite:
		default:
			return Layer.Sprite ? FName(*Layer.Sprite->GetName()) : NAME_None;
		}
	}

	TArray<FName> BuildDuplicateVisualLayerIds(const TArray<FWacomBattleEnemyPartVisualLayer>& VisualLayers)
	{
		TSet<FName> Seen;
		TSet<FName> Duplicates;
		for (const FWacomBattleEnemyPartVisualLayer& Layer : VisualLayers)
		{
			if (Layer.LayerId.IsNone())
			{
				continue;
			}

			if (Seen.Contains(Layer.LayerId))
			{
				Duplicates.Add(Layer.LayerId);
			}
			else
			{
				Seen.Add(Layer.LayerId);
			}
		}

		TArray<FName> Result = Duplicates.Array();
		Result.Sort([](const FName& Left, const FName& Right)
		{
			return Left.LexicalLess(Right);
		});
		return Result;
	}
}

UWacomBattleEnemyPartVisualLayerComponent::UWacomBattleEnemyPartVisualLayerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWacomBattleEnemyPartVisualLayerComponent::RefreshVisualLayers(
	const TArray<FWacomBattleEnemyPartVisualLayer>& VisualLayers,
	USceneComponent* AttachRoot)
{
	ClearGeneratedVisualLayers();

	if (VisualLayers.Num() == 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!World || !Owner)
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

	for (int32 LayerIndex = 0; LayerIndex < VisualLayers.Num(); ++LayerIndex)
	{
		const FWacomBattleEnemyPartVisualLayer& Layer = VisualLayers[LayerIndex];
		if (!VisualLayerHasAsset(Layer))
		{
			continue;
		}

		const FName ComponentName(*BuildVisualLayerComponentName(Layer.LayerId, LayerIndex));
		if (Layer.LayerMode == EWacomBattleEnemyPartVisualLayerMode::Flipbook)
		{
			UPaperFlipbookComponent* FlipbookComponent =
				NewObject<UPaperFlipbookComponent>(Owner, ComponentName, RF_Transactional | RF_Transient);
			if (!FlipbookComponent)
			{
				continue;
			}

			FlipbookComponent->SetupAttachment(AttachParent);
			FlipbookComponent->SetFlipbook(Layer.Flipbook);
			FlipbookComponent->SetRelativeLocation(Layer.RelativeLocation);
			FlipbookComponent->SetRelativeRotation(Layer.RelativeRotation);
			FlipbookComponent->SetRelativeScale3D(Layer.RelativeScale3D);
			FlipbookComponent->SetSpriteColor(Layer.Tint);
			FlipbookComponent->SetTranslucentSortPriority(Layer.SortOrder);
			FlipbookComponent->SetVisibility(Layer.bVisible, true);
			if (Layer.MaterialOverride)
			{
				FlipbookComponent->SetMaterial(0, Layer.MaterialOverride);
			}
			FlipbookComponent->SetCastShadow(Layer.bCastShadow);
			FlipbookComponent->bCastDynamicShadow = Layer.bCastShadow;
			FlipbookComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			FlipbookComponent->SetGenerateOverlapEvents(false);
			FlipbookComponent->SetLooping(Layer.bLoopFlipbook);
			FlipbookComponent->SetPlayRate(Layer.FlipbookPlayRate);
			FlipbookComponent->SetPlaybackPosition(Layer.FlipbookStartTimeSeconds, false);
			if (Layer.bAutoPlayFlipbook && Layer.FlipbookPlayRate > 0.0f)
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
			GeneratedFlipbookVisualLayerComponents.Add(FlipbookComponent);
			FWacomBattleEnemyPartRuntimeVisualLayer& RuntimeLayer = RuntimeVisualLayers.AddDefaulted_GetRef();
			RuntimeLayer.LayerIndex = LayerIndex;
			RuntimeLayer.LayerMode = Layer.LayerMode;
			RuntimeLayer.FlipbookComponent = FlipbookComponent;
		}
		else
		{
			UPaperSpriteComponent* SpriteComponent =
				NewObject<UPaperSpriteComponent>(Owner, ComponentName, RF_Transactional | RF_Transient);
			if (!SpriteComponent)
			{
				continue;
			}

			SpriteComponent->SetupAttachment(AttachParent);
			SpriteComponent->SetSprite(Layer.Sprite);
			SpriteComponent->SetRelativeLocation(Layer.RelativeLocation);
			SpriteComponent->SetRelativeRotation(Layer.RelativeRotation);
			SpriteComponent->SetRelativeScale3D(Layer.RelativeScale3D);
			SpriteComponent->SetSpriteColor(Layer.Tint);
			SpriteComponent->SetTranslucentSortPriority(Layer.SortOrder);
			SpriteComponent->SetVisibility(Layer.bVisible, true);
			if (Layer.MaterialOverride)
			{
				SpriteComponent->SetMaterial(0, Layer.MaterialOverride);
			}
			SpriteComponent->SetCastShadow(Layer.bCastShadow);
			SpriteComponent->bCastDynamicShadow = Layer.bCastShadow;
			SpriteComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			SpriteComponent->SetGenerateOverlapEvents(false);
			SpriteComponent->bEditableWhenInherited = false;
			Owner->AddInstanceComponent(SpriteComponent);
			SpriteComponent->RegisterComponentWithWorld(World);
			GeneratedVisualLayerComponents.Add(SpriteComponent);
			FWacomBattleEnemyPartRuntimeVisualLayer& RuntimeLayer = RuntimeVisualLayers.AddDefaulted_GetRef();
			RuntimeLayer.LayerIndex = LayerIndex;
			RuntimeLayer.LayerMode = Layer.LayerMode;
			RuntimeLayer.SpriteComponent = SpriteComponent;
		}
	}
}

int32 UWacomBattleEnemyPartVisualLayerComponent::ApplyRuntimeDestroyedState(
	const TArray<FWacomBattleEnemyPartVisualLayer>& VisualLayers)
{
	if (bRuntimeDestroyedStateApplied)
	{
		return RuntimeDestroyedVisualLayerCount;
	}

	bRuntimeDestroyedStateApplied = true;
	RuntimeDestroyedVisualLayerCount = 0;
	++RuntimeDestroyedVisualApplyCount;
	for (const FWacomBattleEnemyPartRuntimeVisualLayer& RuntimeLayer : RuntimeVisualLayers)
	{
		if (!VisualLayers.IsValidIndex(RuntimeLayer.LayerIndex))
		{
			continue;
		}

		const FWacomBattleEnemyPartVisualLayer& Layer = VisualLayers[RuntimeLayer.LayerIndex];
		if (RuntimeLayer.LayerMode == EWacomBattleEnemyPartVisualLayerMode::Flipbook)
		{
			UPaperFlipbookComponent* Component = RuntimeLayer.FlipbookComponent.Get();
			if (!Component || !Layer.DestroyedFlipbook
				|| !FMath::IsFinite(Layer.DestroyedFlipbookPlayRate)
				|| Layer.DestroyedFlipbookPlayRate <= 0.0f)
			{
				continue;
			}

			Component->Stop();
			Component->SetFlipbook(Layer.DestroyedFlipbook);
			Component->SetLooping(false);
			Component->SetPlayRate(Layer.DestroyedFlipbookPlayRate);
			Component->SetPlaybackPosition(0.0f, false);
			Component->Play();
			++RuntimeDestroyedVisualLayerCount;
		}
		else if (UPaperSpriteComponent* Component = RuntimeLayer.SpriteComponent.Get())
		{
			if (Layer.DestroyedSprite)
			{
				Component->SetSprite(Layer.DestroyedSprite);
				++RuntimeDestroyedVisualLayerCount;
			}
		}
	}
	return RuntimeDestroyedVisualLayerCount;
}

void UWacomBattleEnemyPartVisualLayerComponent::RestoreRuntimeAuthoredState(
	const TArray<FWacomBattleEnemyPartVisualLayer>& VisualLayers)
{
	if (!bRuntimeDestroyedStateApplied)
	{
		return;
	}

	for (const FWacomBattleEnemyPartRuntimeVisualLayer& RuntimeLayer : RuntimeVisualLayers)
	{
		if (!VisualLayers.IsValidIndex(RuntimeLayer.LayerIndex))
		{
			continue;
		}

		const FWacomBattleEnemyPartVisualLayer& Layer = VisualLayers[RuntimeLayer.LayerIndex];
		if (RuntimeLayer.LayerMode == EWacomBattleEnemyPartVisualLayerMode::Flipbook)
		{
			if (UPaperFlipbookComponent* Component = RuntimeLayer.FlipbookComponent.Get())
			{
				Component->Stop();
				Component->SetFlipbook(Layer.Flipbook);
				Component->SetLooping(Layer.bLoopFlipbook);
				Component->SetPlayRate(Layer.FlipbookPlayRate);
				Component->SetPlaybackPosition(Layer.FlipbookStartTimeSeconds, false);
				if (Layer.bAutoPlayFlipbook && Layer.FlipbookPlayRate > 0.0f)
				{
					Component->Play();
				}
			}
		}
		else if (UPaperSpriteComponent* Component = RuntimeLayer.SpriteComponent.Get())
		{
			Component->SetSprite(Layer.Sprite);
		}
	}

	bRuntimeDestroyedStateApplied = false;
	RuntimeDestroyedVisualLayerCount = 0;
}

void UWacomBattleEnemyPartVisualLayerComponent::ClearGeneratedVisualLayers()
{
	for (UPaperSpriteComponent* SpriteComponent : GeneratedVisualLayerComponents)
	{
		if (SpriteComponent)
		{
			SpriteComponent->DestroyComponent();
		}
	}
	GeneratedVisualLayerComponents.Reset();

	for (UPaperFlipbookComponent* FlipbookComponent : GeneratedFlipbookVisualLayerComponents)
	{
		if (FlipbookComponent)
		{
			FlipbookComponent->DestroyComponent();
		}
	}
	GeneratedFlipbookVisualLayerComponents.Reset();
	RuntimeVisualLayers.Reset();
	bRuntimeDestroyedStateApplied = false;
	RuntimeDestroyedVisualLayerCount = 0;
	RuntimeDestroyedVisualApplyCount = 0;
}

FWacomBattleEnemyPartVisualLayerDebugView
UWacomBattleEnemyPartVisualLayerComponent::BuildVisualLayerDebugView(
	const TArray<FWacomBattleEnemyPartVisualLayer>& VisualLayers) const
{
	FWacomBattleEnemyPartVisualLayerDebugView View;
	View.bUsingVisualLayers = VisualLayers.Num() > 0;
	View.VisualLayerCount = VisualLayers.Num();
	View.GeneratedStaticVisualLayerComponentCount = GeneratedVisualLayerComponents.Num();
	View.GeneratedFlipbookVisualLayerComponentCount = GeneratedFlipbookVisualLayerComponents.Num();
	View.GeneratedVisualLayerComponentCount =
		View.GeneratedStaticVisualLayerComponentCount + View.GeneratedFlipbookVisualLayerComponentCount;

	for (const UPaperSpriteComponent* SpriteComponent : GeneratedVisualLayerComponents)
	{
		if (SpriteComponent && SpriteComponent->IsRegistered())
		{
			++View.RegisteredStaticVisualLayerComponentCount;
		}
		if (SpriteComponent && SpriteComponent->IsVisible())
		{
			++View.VisibleStaticVisualLayerComponentCount;
		}
	}
	for (const UPaperFlipbookComponent* FlipbookComponent : GeneratedFlipbookVisualLayerComponents)
	{
		if (FlipbookComponent && FlipbookComponent->IsRegistered())
		{
			++View.RegisteredFlipbookVisualLayerComponentCount;
		}
		if (FlipbookComponent && FlipbookComponent->IsVisible())
		{
			++View.VisibleFlipbookVisualLayerComponentCount;
		}
	}

	View.RegisteredVisualLayerComponentCount =
		View.RegisteredStaticVisualLayerComponentCount + View.RegisteredFlipbookVisualLayerComponentCount;
	View.VisibleVisualLayerComponentCount =
		View.VisibleStaticVisualLayerComponentCount + View.VisibleFlipbookVisualLayerComponentCount;
	View.VisualLayerIds.Reserve(VisualLayers.Num());
	View.VisualLayerAssetNames.Reserve(VisualLayers.Num());
	for (const FWacomBattleEnemyPartVisualLayer& Layer : VisualLayers)
	{
		View.VisualLayerIds.Add(Layer.LayerId);
		View.VisualLayerAssetNames.Add(GetVisualLayerAssetName(Layer));
		if (!VisualLayerHasAsset(Layer))
		{
			++View.MissingVisualLayerAssetCount;
		}
		if (Layer.LayerMode == EWacomBattleEnemyPartVisualLayerMode::StaticSprite && !Layer.Sprite)
		{
			++View.MissingVisualLayerSpriteCount;
		}
		if (Layer.LayerMode == EWacomBattleEnemyPartVisualLayerMode::Flipbook && !Layer.Flipbook)
		{
			++View.MissingVisualLayerFlipbookCount;
		}
		if ((Layer.LayerMode == EWacomBattleEnemyPartVisualLayerMode::StaticSprite && Layer.DestroyedSprite)
			|| (Layer.LayerMode == EWacomBattleEnemyPartVisualLayerMode::Flipbook && Layer.DestroyedFlipbook))
		{
			++View.DestroyedVisualResourceCount;
		}
	}
	View.bRuntimeDestroyedStateApplied = bRuntimeDestroyedStateApplied;
	View.RuntimeDestroyedVisualLayerCount = RuntimeDestroyedVisualLayerCount;
	View.RuntimeDestroyedVisualApplyCount = RuntimeDestroyedVisualApplyCount;
	View.DuplicateVisualLayerIds = BuildDuplicateVisualLayerIds(VisualLayers);
	return View;
}

void UWacomBattleEnemyPartVisualLayerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearGeneratedVisualLayers();
	Super::EndPlay(EndPlayReason);
}
