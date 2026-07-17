// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattleEnemyHostVisualComponent.h"

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
	constexpr float RuntimeAnimationWatchdogGraceSeconds = 0.10f;
}

UWacomBattleEnemyHostVisualComponent::UWacomBattleEnemyHostVisualComponent()
	: RuntimePlayback(new FWacomBattleEnemyActionPlayback())
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWacomBattleEnemyHostVisualComponent::FActionPlaybackDeleter::operator()(
	FWacomBattleEnemyActionPlayback* Playback) const
{
	delete Playback;
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
	AuthoredIdleFlipbook = bUseFlipbook ? HostFlipbook : nullptr;
	AuthoredIdlePlayRate = FlipbookPlayRate;
	AuthoredIdleStartTimeSeconds = FlipbookStartTimeSeconds;
	bAuthoredIdleLooping = bLoopFlipbook;
	bAuthoredIdleAutoPlay = bAutoPlayFlipbook;
	bRuntimeTerminalState = false;
	CurrentRuntimeClipName = AuthoredIdleFlipbook
		? FName(*AuthoredIdleFlipbook->GetName())
		: NAME_None;

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
		FlipbookComponent->OnFinishedPlaying.AddUniqueDynamic(
			this,
			&UWacomBattleEnemyHostVisualComponent::HandleRuntimeFlipbookFinished);
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
	CancelRuntimePlayback();
	if (GeneratedHostSpriteVisualComponent)
	{
		GeneratedHostSpriteVisualComponent->DestroyComponent();
		GeneratedHostSpriteVisualComponent = nullptr;
	}
	if (GeneratedHostFlipbookVisualComponent)
	{
		GeneratedHostFlipbookVisualComponent->OnFinishedPlaying.RemoveDynamic(
			this,
			&UWacomBattleEnemyHostVisualComponent::HandleRuntimeFlipbookFinished);
		GeneratedHostFlipbookVisualComponent->DestroyComponent();
		GeneratedHostFlipbookVisualComponent = nullptr;
	}
}

bool UWacomBattleEnemyHostVisualComponent::PlayRuntimeOneShot(
	UPaperFlipbook* Flipbook,
	float PlayRate,
	float ImpactNormalizedTime,
	FName IntentId,
	bool bTerminal,
	FWacomBattleEnemyActionPlaybackCallbacks&& Callbacks)
{
	UPaperFlipbookComponent* FlipbookComponent = GeneratedHostFlipbookVisualComponent.Get();
	if (!IsValid(FlipbookComponent)
		|| !IsValid(Flipbook)
		|| !FMath::IsFinite(PlayRate)
		|| PlayRate <= 0.0f
		|| (!bTerminal
			&& (!FMath::IsFinite(ImpactNormalizedTime)
				|| ImpactNormalizedTime < 0.0f
				|| ImpactNormalizedTime > 1.0f))
		|| !FMath::IsFinite(Flipbook->GetTotalDuration())
		|| Flipbook->GetTotalDuration() <= 0.0f)
	{
		Callbacks.CompleteImmediately();
		return false;
	}

	FWacomBattleEnemyActionPlaybackRequest Request;
	Request.LifetimeOwner = this;
	Request.DurationSeconds = Flipbook->GetTotalDuration() / PlayRate;
	if (!bTerminal)
	{
		Request.ImpactNormalizedTime = ImpactNormalizedTime;
	}
	Request.WatchdogGraceSeconds = RuntimeAnimationWatchdogGraceSeconds;
	Request.Callbacks = MoveTemp(Callbacks);
	Request.StartVisual = [this, FlipbookComponent, Flipbook, PlayRate, IntentId, bTerminal]()
	{
		if (!IsValid(FlipbookComponent) || !IsValid(Flipbook))
		{
			return false;
		}

		bRuntimeTerminalState = bTerminal;
		CurrentRuntimeIntentId = bTerminal ? NAME_None : IntentId;
		CurrentRuntimeClipName = FName(*Flipbook->GetName());
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
		FinalizeRuntimePlayback(Context.bRestoreAuthoredVisual);
	};
	return RuntimePlayback && RuntimePlayback->Begin(MoveTemp(Request));
}

void UWacomBattleEnemyHostVisualComponent::ResetRuntimePlaybackToIdle()
{
	if (!IsRuntimePlaybackActive() && !bRuntimeTerminalState)
	{
		return;
	}

	CancelRuntimePlayback();
	bRuntimeTerminalState = false;
	RestoreAuthoredIdlePlayback();
}

void UWacomBattleEnemyHostVisualComponent::CancelRuntimePlayback()
{
	if (RuntimePlayback)
	{
		RuntimePlayback->Cancel(true);
	}
}

void UWacomBattleEnemyHostVisualComponent::HandleRuntimeFlipbookFinished()
{
	if (RuntimePlayback)
	{
		RuntimePlayback->NotifyFinished();
	}
}

void UWacomBattleEnemyHostVisualComponent::FinalizeRuntimePlayback(
	bool bRestoreAuthoredVisual)
{
	UPaperFlipbookComponent* FlipbookComponent = GeneratedHostFlipbookVisualComponent.Get();
	if (IsValid(FlipbookComponent))
	{
		if (bRuntimeTerminalState)
		{
			FlipbookComponent->Stop();
			FlipbookComponent->SetPlaybackPosition(FlipbookComponent->GetFlipbookLength(), false);
		}
		else if (bRestoreAuthoredVisual)
		{
			RestoreAuthoredIdlePlayback();
		}
	}
	CurrentRuntimeIntentId = NAME_None;
}

void UWacomBattleEnemyHostVisualComponent::RestoreAuthoredIdlePlayback()
{
	UPaperFlipbookComponent* FlipbookComponent = GeneratedHostFlipbookVisualComponent.Get();
	if (!IsValid(FlipbookComponent) || !IsValid(AuthoredIdleFlipbook))
	{
		CurrentRuntimeClipName = NAME_None;
		return;
	}

	FlipbookComponent->Stop();
	FlipbookComponent->SetFlipbook(AuthoredIdleFlipbook);
	FlipbookComponent->SetLooping(bAuthoredIdleLooping);
	FlipbookComponent->SetPlayRate(AuthoredIdlePlayRate);
	FlipbookComponent->SetPlaybackPosition(AuthoredIdleStartTimeSeconds, false);
	if (bAuthoredIdleAutoPlay && AuthoredIdlePlayRate > 0.0f)
	{
		FlipbookComponent->Play();
	}
	CurrentRuntimeClipName = FName(*AuthoredIdleFlipbook->GetName());
}

bool UWacomBattleEnemyHostVisualComponent::IsRuntimePlaybackActive() const
{
	return RuntimePlayback && RuntimePlayback->GetView().bActive;
}

int32 UWacomBattleEnemyHostVisualComponent::GetRuntimePlaybackCount() const
{
	return RuntimePlayback ? RuntimePlayback->GetView().PlaybackCount : 0;
}

int32 UWacomBattleEnemyHostVisualComponent::GetRuntimeWatchdogCompletionCount() const
{
	return RuntimePlayback ? RuntimePlayback->GetView().WatchdogCompletionCount : 0;
}

float UWacomBattleEnemyHostVisualComponent::GetCurrentRuntimeImpactNormalizedTime() const
{
	return RuntimePlayback ? RuntimePlayback->GetView().ImpactNormalizedTime : 0.0f;
}

bool UWacomBattleEnemyHostVisualComponent::HasRuntimeImpactFired() const
{
	return RuntimePlayback && RuntimePlayback->GetView().bImpactFired;
}

int32 UWacomBattleEnemyHostVisualComponent::GetRuntimeImpactCount() const
{
	return RuntimePlayback ? RuntimePlayback->GetView().ImpactCount : 0;
}

int32 UWacomBattleEnemyHostVisualComponent::GetRuntimeWatchdogForcedImpactCount() const
{
	return RuntimePlayback ? RuntimePlayback->GetView().WatchdogForcedImpactCount : 0;
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
