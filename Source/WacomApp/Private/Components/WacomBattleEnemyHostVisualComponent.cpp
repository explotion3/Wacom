// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattleEnemyHostVisualComponent.h"

#include "Actors/WacomBattleEnemyActionPlayback.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "TimerManager.h"

namespace
{
	constexpr float RuntimeAnimationWatchdogGraceSeconds = 0.10f;
}

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

	CancelRuntimePlayback();
	PendingRuntimeImpact = MoveTemp(Callbacks.OnImpact);
	PendingRuntimeCompletion = MoveTemp(Callbacks.OnCompleted);
	bRuntimePlaybackActive = true;
	bRuntimeTerminalState = bTerminal;
	bRuntimeImpactFired = false;
	CurrentRuntimeImpactNormalizedTime = bTerminal ? 0.0f : ImpactNormalizedTime;
	CurrentRuntimeIntentId = bTerminal ? NAME_None : IntentId;
	CurrentRuntimeClipName = FName(*Flipbook->GetName());
	ActivePlaybackSerial = ++PlaybackSerial;
	++RuntimePlaybackCount;

	FlipbookComponent->Stop();
	FlipbookComponent->SetFlipbook(Flipbook);
	FlipbookComponent->SetLooping(false);
	FlipbookComponent->SetPlayRate(PlayRate);
	FlipbookComponent->SetPlaybackPosition(0.0f, false);
	FlipbookComponent->Play();

	if (UWorld* World = GetWorld())
	{
		if (!bTerminal && PendingRuntimeImpact)
		{
			const float ImpactSeconds = Flipbook->GetTotalDuration() / PlayRate
				* ImpactNormalizedTime;
			if (ImpactSeconds <= 0.0f)
			{
				HandleRuntimeImpact(ActivePlaybackSerial);
			}
			else
			{
				World->GetTimerManager().SetTimer(
					RuntimeImpactTimerHandle,
					FTimerDelegate::CreateUObject(
						this,
						&UWacomBattleEnemyHostVisualComponent::HandleRuntimeImpact,
						ActivePlaybackSerial),
					FMath::Max(0.01f, ImpactSeconds),
					false);
			}
		}
		const float WatchdogSeconds = Flipbook->GetTotalDuration() / PlayRate
			+ RuntimeAnimationWatchdogGraceSeconds;
		World->GetTimerManager().SetTimer(
			RuntimeWatchdogTimerHandle,
			FTimerDelegate::CreateUObject(
				this,
				&UWacomBattleEnemyHostVisualComponent::HandleRuntimeWatchdogExpired,
				ActivePlaybackSerial),
			FMath::Max(0.01f, WatchdogSeconds),
			false);
	}
	else
	{
		CompleteRuntimePlayback(ActivePlaybackSerial, true, true);
		return false;
	}

	return true;
}

void UWacomBattleEnemyHostVisualComponent::ResetRuntimePlaybackToIdle()
{
	if (!bRuntimePlaybackActive && !bRuntimeTerminalState)
	{
		return;
	}

	CancelRuntimePlayback();
	bRuntimeTerminalState = false;
	RestoreAuthoredIdlePlayback();
}

void UWacomBattleEnemyHostVisualComponent::CancelRuntimePlayback()
{
	if (!bRuntimePlaybackActive)
	{
		StopRuntimeWatchdog();
		StopRuntimeImpactTimer();
		PendingRuntimeImpact = nullptr;
		return;
	}

	const uint64 SerialToComplete = ActivePlaybackSerial;
	CompleteRuntimePlayback(SerialToComplete, false, false);
}

void UWacomBattleEnemyHostVisualComponent::HandleRuntimeFlipbookFinished()
{
	CompleteRuntimePlayback(ActivePlaybackSerial, false, true);
}

void UWacomBattleEnemyHostVisualComponent::HandleRuntimeWatchdogExpired(
	uint64 ExpectedPlaybackSerial)
{
	CompleteRuntimePlayback(ExpectedPlaybackSerial, true, true);
}

void UWacomBattleEnemyHostVisualComponent::HandleRuntimeImpact(
	uint64 ExpectedPlaybackSerial)
{
	if (!bRuntimePlaybackActive
		|| ExpectedPlaybackSerial != ActivePlaybackSerial
		|| bRuntimeImpactFired
		|| !PendingRuntimeImpact)
	{
		return;
	}

	StopRuntimeImpactTimer();
	bRuntimeImpactFired = true;
	++RuntimeImpactCount;
	TFunction<void()> Callback = MoveTemp(PendingRuntimeImpact);
	PendingRuntimeImpact = nullptr;
	Callback();
}

void UWacomBattleEnemyHostVisualComponent::CompleteRuntimePlayback(
	uint64 ExpectedPlaybackSerial,
	bool bWatchdogCompletion,
	bool bDeliverPendingImpact)
{
	if (!bRuntimePlaybackActive || ExpectedPlaybackSerial != ActivePlaybackSerial)
	{
		return;
	}

	StopRuntimeWatchdog();
	StopRuntimeImpactTimer();
	const bool bHadPendingImpact = !bRuntimeImpactFired && PendingRuntimeImpact;
	if (bDeliverPendingImpact)
	{
		HandleRuntimeImpact(ExpectedPlaybackSerial);
		if (bWatchdogCompletion && bHadPendingImpact && bRuntimeImpactFired)
		{
			++RuntimeWatchdogForcedImpactCount;
		}
	}
	else
	{
		PendingRuntimeImpact = nullptr;
	}
	if (bWatchdogCompletion)
	{
		++RuntimeWatchdogCompletionCount;
	}

	bRuntimePlaybackActive = false;
	++PlaybackSerial;
	if (UPaperFlipbookComponent* FlipbookComponent = GeneratedHostFlipbookVisualComponent.Get())
	{
		if (bRuntimeTerminalState)
		{
			FlipbookComponent->Stop();
			FlipbookComponent->SetPlaybackPosition(FlipbookComponent->GetFlipbookLength(), false);
		}
		else
		{
			RestoreAuthoredIdlePlayback();
		}
	}
	CurrentRuntimeIntentId = NAME_None;
	CompletePendingCallback();
}

void UWacomBattleEnemyHostVisualComponent::StopRuntimeWatchdog()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RuntimeWatchdogTimerHandle);
	}
	RuntimeWatchdogTimerHandle = FTimerHandle();
}

void UWacomBattleEnemyHostVisualComponent::StopRuntimeImpactTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RuntimeImpactTimerHandle);
	}
	RuntimeImpactTimerHandle = FTimerHandle();
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

void UWacomBattleEnemyHostVisualComponent::CompletePendingCallback()
{
	TFunction<void()> Completion = MoveTemp(PendingRuntimeCompletion);
	PendingRuntimeCompletion = TFunction<void()>();
	if (Completion)
	{
		Completion();
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
