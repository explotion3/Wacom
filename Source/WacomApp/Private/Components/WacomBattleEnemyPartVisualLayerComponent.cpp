// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattleEnemyPartVisualLayerComponent.h"

#include "Components/WacomBattleEnemyActionPlayback.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"

namespace
{
	constexpr float RuntimeActionWatchdogGraceSeconds = 0.10f;

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
	: RuntimeActionPlayback(new FWacomBattleEnemyActionPlayback())
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWacomBattleEnemyPartVisualLayerComponent::FActionPlaybackDeleter::operator()(
	FWacomBattleEnemyActionPlayback* Playback) const
{
	delete Playback;
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
			RuntimeLayer.LayerId = Layer.LayerId;
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
			RuntimeLayer.LayerId = Layer.LayerId;
			RuntimeLayer.LayerMode = Layer.LayerMode;
			RuntimeLayer.SpriteComponent = SpriteComponent;
		}
	}
}

bool UWacomBattleEnemyPartVisualLayerComponent::PlayRuntimeActionOneShot(
	const TArray<FWacomBattleEnemyPartVisualLayer>& VisualLayers,
	FName TargetVisualLayerId,
	UPaperFlipbook* Flipbook,
	float PlayRate,
	float ImpactNormalizedTime,
	FName IntentId,
	FWacomBattleEnemyActionPlaybackCallbacks&& Callbacks)
{
	int32 MatchingLayerIndex = INDEX_NONE;
	int32 MatchingLayerCount = 0;
	for (int32 LayerIndex = 0; LayerIndex < VisualLayers.Num(); ++LayerIndex)
	{
		if (VisualLayers[LayerIndex].LayerId == TargetVisualLayerId)
		{
			MatchingLayerIndex = LayerIndex;
			++MatchingLayerCount;
		}
	}

	const FWacomBattleEnemyPartRuntimeVisualLayer* RuntimeLayer =
		MatchingLayerCount == 1
		? RuntimeVisualLayers.FindByPredicate(
			[MatchingLayerIndex](const FWacomBattleEnemyPartRuntimeVisualLayer& Candidate)
			{
				return Candidate.LayerIndex == MatchingLayerIndex;
			})
		: nullptr;
	const FWacomBattleEnemyPartVisualLayer* AuthoredLayer =
		VisualLayers.IsValidIndex(MatchingLayerIndex)
		? &VisualLayers[MatchingLayerIndex]
		: nullptr;
	UPaperFlipbookComponent* FlipbookComponent = RuntimeLayer
		? RuntimeLayer->FlipbookComponent.Get()
		: nullptr;
	const float Duration = IsValid(Flipbook) ? Flipbook->GetTotalDuration() : 0.0f;
	if (bRuntimeDestroyedStateApplied
		|| TargetVisualLayerId.IsNone()
		|| MatchingLayerCount != 1
		|| !AuthoredLayer
		|| AuthoredLayer->LayerMode != EWacomBattleEnemyPartVisualLayerMode::Flipbook
		|| !IsValid(AuthoredLayer->Flipbook)
		|| !RuntimeLayer
		|| RuntimeLayer->LayerMode != EWacomBattleEnemyPartVisualLayerMode::Flipbook
		|| !IsValid(FlipbookComponent)
		|| !IsValid(Flipbook)
		|| !FMath::IsFinite(PlayRate)
		|| PlayRate <= 0.0f
		|| !FMath::IsFinite(ImpactNormalizedTime)
		|| ImpactNormalizedTime < 0.0f
		|| ImpactNormalizedTime > 1.0f
		|| !FMath::IsFinite(Duration)
		|| Duration <= 0.0f)
	{
		Callbacks.CompleteImmediately();
		return false;
	}

	FWacomBattleEnemyActionPlaybackRequest Request;
	Request.LifetimeOwner = this;
	Request.DurationSeconds = Duration / PlayRate;
	Request.ImpactNormalizedTime = ImpactNormalizedTime;
	Request.WatchdogGraceSeconds = RuntimeActionWatchdogGraceSeconds;
	Request.Callbacks = MoveTemp(Callbacks);
	Request.StartVisual = [
		this,
		FlipbookComponent,
		Flipbook,
		PlayRate,
		IntentId,
		TargetVisualLayerId,
		AuthoredFlipbook = TWeakObjectPtr<UPaperFlipbook>(AuthoredLayer->Flipbook),
		AuthoredPlayRate = AuthoredLayer->FlipbookPlayRate,
		AuthoredStartTimeSeconds = AuthoredLayer->FlipbookStartTimeSeconds,
		bAuthoredLooping = AuthoredLayer->bLoopFlipbook,
		bAuthoredAutoPlay = AuthoredLayer->bAutoPlayFlipbook]()
	{
		if (!IsValid(FlipbookComponent)
			|| !IsValid(Flipbook)
			|| !AuthoredFlipbook.IsValid()
			|| bRuntimeDestroyedStateApplied)
		{
			return false;
		}

		CurrentRuntimeActionLayerId = TargetVisualLayerId;
		CurrentRuntimeActionClipName = FName(*Flipbook->GetName());
		CurrentRuntimeActionIntentId = IntentId;
		ActiveRuntimeActionComponent = FlipbookComponent;
		AuthoredRuntimeActionFlipbook = AuthoredFlipbook.Get();
		AuthoredRuntimeActionPlayRate = AuthoredPlayRate;
		AuthoredRuntimeActionStartTimeSeconds = AuthoredStartTimeSeconds;
		bAuthoredRuntimeActionLooping = bAuthoredLooping;
		bAuthoredRuntimeActionAutoPlay = bAuthoredAutoPlay;
		FlipbookComponent->OnFinishedPlaying.AddUniqueDynamic(
			this,
			&UWacomBattleEnemyPartVisualLayerComponent::HandleRuntimeActionFlipbookFinished);
		FlipbookComponent->Stop();
		FlipbookComponent->SetFlipbook(Flipbook);
		FlipbookComponent->SetLooping(false);
		FlipbookComponent->SetPlayRate(PlayRate);
		FlipbookComponent->SetPlaybackPosition(0.0f, false);
		FlipbookComponent->Play();
		return true;
	};
	Request.FinalizeVisual = [this](
		const FWacomBattleEnemyActionPlaybackFinishContext& Context)
	{
		FinalizeRuntimeActionPlayback(Context.bRestoreAuthoredVisual);
	};
	return RuntimeActionPlayback && RuntimeActionPlayback->Begin(MoveTemp(Request));
}

void UWacomBattleEnemyPartVisualLayerComponent::CancelRuntimeActionPlayback(
	bool bRestoreAuthoredLayer)
{
	if (RuntimeActionPlayback)
	{
		RuntimeActionPlayback->Cancel(bRestoreAuthoredLayer);
	}
}

void UWacomBattleEnemyPartVisualLayerComponent::HandleRuntimeActionFlipbookFinished()
{
	if (RuntimeActionPlayback)
	{
		RuntimeActionPlayback->NotifyFinished();
	}
}

void UWacomBattleEnemyPartVisualLayerComponent::FinalizeRuntimeActionPlayback(
	bool bRestoreAuthoredLayer)
{
	UnbindRuntimeActionFinishedDelegate();
	if (bRestoreAuthoredLayer && !bRuntimeDestroyedStateApplied)
	{
		RestoreActiveRuntimeActionLayer();
	}
	CurrentRuntimeActionLayerId = NAME_None;
	CurrentRuntimeActionClipName = NAME_None;
	CurrentRuntimeActionIntentId = NAME_None;
	ActiveRuntimeActionComponent.Reset();
	AuthoredRuntimeActionFlipbook = nullptr;
}

void UWacomBattleEnemyPartVisualLayerComponent::RestoreActiveRuntimeActionLayer()
{
	UPaperFlipbookComponent* Component = ActiveRuntimeActionComponent.Get();
	if (!IsValid(Component) || !IsValid(AuthoredRuntimeActionFlipbook))
	{
		return;
	}

	Component->Stop();
	Component->SetFlipbook(AuthoredRuntimeActionFlipbook);
	Component->SetLooping(bAuthoredRuntimeActionLooping);
	Component->SetPlayRate(AuthoredRuntimeActionPlayRate);
	Component->SetPlaybackPosition(AuthoredRuntimeActionStartTimeSeconds, false);
	if (bAuthoredRuntimeActionAutoPlay && AuthoredRuntimeActionPlayRate > 0.0f)
	{
		Component->Play();
	}
}

void UWacomBattleEnemyPartVisualLayerComponent::UnbindRuntimeActionFinishedDelegate()
{
	if (UPaperFlipbookComponent* Component = ActiveRuntimeActionComponent.Get())
	{
		Component->OnFinishedPlaying.RemoveDynamic(
			this,
			&UWacomBattleEnemyPartVisualLayerComponent::HandleRuntimeActionFlipbookFinished);
	}
}

bool UWacomBattleEnemyPartVisualLayerComponent::IsRuntimeActionPlaybackActive() const
{
	return RuntimeActionPlayback && RuntimeActionPlayback->GetView().bActive;
}

int32 UWacomBattleEnemyPartVisualLayerComponent::GetRuntimeActionPlaybackCount() const
{
	return RuntimeActionPlayback ? RuntimeActionPlayback->GetView().PlaybackCount : 0;
}

int32 UWacomBattleEnemyPartVisualLayerComponent::GetRuntimeActionWatchdogCompletionCount() const
{
	return RuntimeActionPlayback ? RuntimeActionPlayback->GetView().WatchdogCompletionCount : 0;
}

float UWacomBattleEnemyPartVisualLayerComponent::GetCurrentRuntimeActionImpactNormalizedTime() const
{
	return RuntimeActionPlayback ? RuntimeActionPlayback->GetView().ImpactNormalizedTime : 0.0f;
}

bool UWacomBattleEnemyPartVisualLayerComponent::HasRuntimeActionImpactFired() const
{
	return RuntimeActionPlayback && RuntimeActionPlayback->GetView().bImpactFired;
}

int32 UWacomBattleEnemyPartVisualLayerComponent::GetRuntimeActionImpactCount() const
{
	return RuntimeActionPlayback ? RuntimeActionPlayback->GetView().ImpactCount : 0;
}

int32 UWacomBattleEnemyPartVisualLayerComponent::GetRuntimeActionWatchdogForcedImpactCount() const
{
	return RuntimeActionPlayback ? RuntimeActionPlayback->GetView().WatchdogForcedImpactCount : 0;
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
	CancelRuntimeActionPlayback(false);
	return RuntimeDestroyedVisualLayerCount;
}

void UWacomBattleEnemyPartVisualLayerComponent::RestoreRuntimeAuthoredState(
	const TArray<FWacomBattleEnemyPartVisualLayer>& VisualLayers)
{
	CancelRuntimeActionPlayback(false);

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
	CancelRuntimeActionPlayback(false);
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
	View.CurrentRuntimeActionLayerId = CurrentRuntimeActionLayerId;
	View.CurrentRuntimeActionClipName = CurrentRuntimeActionClipName;
	View.CurrentRuntimeActionIntentId = CurrentRuntimeActionIntentId;
	View.bRuntimeActionPlaybackActive = IsRuntimeActionPlaybackActive();
	View.RuntimeActionPlaybackCount = GetRuntimeActionPlaybackCount();
	View.RuntimeActionWatchdogCompletionCount = GetRuntimeActionWatchdogCompletionCount();
	View.DuplicateVisualLayerIds = BuildDuplicateVisualLayerIds(VisualLayers);
	return View;
}

void UWacomBattleEnemyPartVisualLayerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearGeneratedVisualLayers();
	Super::EndPlay(EndPlayReason);
}
