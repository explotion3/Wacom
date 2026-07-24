// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattleEnemySceneRuntimeComponent.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartAnimationStyle.h"
#include "Actors/WacomBattleEnemyPartImpactStyle.h"
#include "Actors/WacomBattleEnemyPartTargetPreviewStyle.h"
#include "Components/WacomBattleEnemyActionPlayback.h"
#include "Components/WacomBattleEnemyPartComponent.h"
#include "Components/WacomBattleEnemyPartCuePlayback.h"
#include "Components/WacomBattleEnemyPartFallbackCollisionComponent.h"
#include "Components/WacomBattleEnemyPartFlipbookLayerComponent.h"
#include "Components/WacomBattleEnemyPartImpactAnchorComponent.h"
#include "Components/WacomBattleEnemyPartImpactFeedbackController.h"
#include "Components/WacomBattleEnemyPartOutlineFeedbackController.h"
#include "Components/WacomBattleEnemyPartPresentationBounds.h"
#include "Components/WacomBattleEnemyPartSpriteLayerComponent.h"
#include "Components/WacomBattleEnemyPartTargetPreviewFeedbackController.h"
#include "Components/WacomBattleEnemyPartTargetPreviewPlayback.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "PaperFlipbook.h"
#include "PaperSprite.h"
#include "Settings/WacomPresentationAccessibilityPolicy.h"
#include "Settings/WacomSettingsSubsystem.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	const FName InteractionCollisionSourceNone(TEXT("None"));
	const FName InteractionCollisionSourceStableSprite(TEXT("StableSpriteBodySetup"));
	const FName InteractionCollisionSourceVisualBounds(TEXT("InteractionVisualBoundsFallback"));
	const FName InteractionCollisionSourceVisualUnion(TEXT("DirectVisualUnionFallback"));
	const FName InteractionCollisionSourceDefault(TEXT("DefaultSafetyFallback"));
	const FName PresentationBoundsSourceStableVisual(TEXT("StableInteractionVisual"));

	struct FFlipbookAuthoredState
	{
		TWeakObjectPtr<UWacomBattleEnemyPartFlipbookLayerComponent> Component;
		TStrongObjectPtr<UPaperFlipbook> Flipbook;
		float PlayRate = 1.0f;
		float PlaybackPosition = 0.0f;
		bool bLooping = true;
		bool bPlaying = false;
		bool bVisible = true;
		FVector RelativeScale = FVector::OneVector;
	};

	struct FSpriteAuthoredState
	{
		TWeakObjectPtr<UWacomBattleEnemyPartSpriteLayerComponent> Component;
		TStrongObjectPtr<UPaperSprite> Sprite;
		bool bVisible = true;
		FVector RelativeScale = FVector::OneVector;
	};

	struct FPartRuntimeState
	{
		FPartRuntimeState()
			: CuePlayback(MakeUnique<FWacomBattleEnemyPartCuePlayback>())
			, ImpactFeedback(MakeUnique<FWacomBattleEnemyPartImpactFeedbackController>())
			, TargetPreviewPlayback(MakeUnique<FWacomBattleEnemyPartTargetPreviewPlayback>())
			, TargetPreviewFeedback(MakeUnique<FWacomBattleEnemyPartTargetPreviewFeedbackController>())
			, OutlineFeedback(MakeUnique<FWacomBattleEnemyPartOutlineFeedbackController>())
			, ActionPlayback(MakeUnique<FWacomBattleEnemyActionPlayback>())
		{
		}

		FPartRuntimeState(FPartRuntimeState&&) = default;
		FPartRuntimeState& operator=(FPartRuntimeState&&) = default;
		FPartRuntimeState(const FPartRuntimeState&) = delete;
		FPartRuntimeState& operator=(const FPartRuntimeState&) = delete;

		TWeakObjectPtr<UWacomBattleEnemyPartComponent> Part;
		TArray<FFlipbookAuthoredState> Flipbooks;
		TArray<FSpriteAuthoredState> Sprites;
		TWeakObjectPtr<UWacomBattleEnemyPartImpactAnchorComponent> ImpactAnchor;
		int32 ImpactAnchorCount = 0;
		TWeakObjectPtr<UPrimitiveComponent> InteractionVisual;
		TWeakObjectPtr<UPaperSprite> StableInteractionCollisionSprite;
		TWeakObjectPtr<UWacomBattleEnemyPartFallbackCollisionComponent> FallbackCollision;
		bool bInteractionVisualResolved = false;
		bool bInteractionCollisionReady = false;
		bool bUsingBoxCollisionFallback = false;
		bool bInteractionFallbackLogged = false;
		FName InteractionCollisionSource = InteractionCollisionSourceNone;

		FName EncounterId = NAME_None;
		FName EnemySlotId = NAME_None;
		FName PartSlotId = NAME_None;
		FName PartId = NAME_None;
		FGuid PartInstanceId;
		bool bBound = false;
		bool bRegisteredWithHUD = false;
		bool bTargetable = false;
		bool bTargetSelectionActive = false;
		FName TargetDisabledReason = NAME_None;
		bool bDestroyed = false;
		bool bDestroyedVisualApplied = false;
		bool bAwaitingDestroyedCue = false;
		bool bDestroyedSwapPending = false;
		int32 DestroyedVisualApplyCount = 0;
		int32 CurrentInitiative = 0;
		FName CurrentIntentId = NAME_None;
		int32 SnapshotApplyCount = 0;
		int32 SnapshotNoOpCount = 0;
		int32 TargetableApplyCount = 0;

		bool bDragPreviewActive = false;
		bool bHoverActive = false;
		EWacomFirstPersonCardDragTargetFeedbackState DragPreviewState =
			EWacomFirstPersonCardDragTargetFeedbackState::None;
		FName HoverReason = NAME_None;
		float TargetPreviewFlashScale = 1.0f;

		FName LastCueKind = TEXT("None");
		int32 CuePlayCount = 0;
		float CuePlaybackDurationSeconds = 0.0f;
		FName CurrentAnimationIntentId = NAME_None;
		bool bTerminalPlayback = false;

		TUniquePtr<FWacomBattleEnemyPartCuePlayback> CuePlayback;
		TUniquePtr<FWacomBattleEnemyPartImpactFeedbackController> ImpactFeedback;
		TUniquePtr<FWacomBattleEnemyPartTargetPreviewPlayback> TargetPreviewPlayback;
		TUniquePtr<FWacomBattleEnemyPartTargetPreviewFeedbackController> TargetPreviewFeedback;
		TUniquePtr<FWacomBattleEnemyPartOutlineFeedbackController> OutlineFeedback;
		TUniquePtr<FWacomBattleEnemyActionPlayback> ActionPlayback;
	};

	FPartRuntimeState* FindState(
		TArray<FPartRuntimeState>& States,
		const UWacomBattleEnemyPartComponent& Part)
	{
		return States.FindByPredicate([&Part](const FPartRuntimeState& State)
		{
			return State.Part.Get() == &Part;
		});
	}

	const FPartRuntimeState* FindState(
		const TArray<FPartRuntimeState>& States,
		const UWacomBattleEnemyPartComponent& Part)
	{
		return States.FindByPredicate([&Part](const FPartRuntimeState& State)
		{
			return State.Part.Get() == &Part;
		});
	}

	void RestoreFlipbook(FFlipbookAuthoredState& Authored)
	{
		UWacomBattleEnemyPartFlipbookLayerComponent* Component = Authored.Component.Get();
		if (!Component)
		{
			return;
		}
		Component->SetFlipbook(Authored.Flipbook.Get());
		Component->SetPlayRate(Authored.PlayRate);
		Component->SetLooping(Authored.bLooping);
		Component->SetPlaybackPosition(FMath::Max(0.0f, Authored.PlaybackPosition), false);
		Component->SetVisibility(Authored.bVisible, true);
		Component->SetRelativeScale3D(Authored.RelativeScale);
		if (Authored.bPlaying)
		{
			Component->Play();
		}
		else
		{
			Component->Stop();
		}
	}

	void RestoreSprite(FSpriteAuthoredState& Authored)
	{
		if (UWacomBattleEnemyPartSpriteLayerComponent* Component = Authored.Component.Get())
		{
			Component->SetSprite(Authored.Sprite.Get());
			Component->SetVisibility(Authored.bVisible, true);
			Component->SetRelativeScale3D(Authored.RelativeScale);
		}
	}

	USceneComponent* ResolveImpactAnchor(FPartRuntimeState& State)
	{
		if (UWacomBattleEnemyPartImpactAnchorComponent* Anchor = State.ImpactAnchor.Get())
		{
			return Anchor;
		}
		return State.Part.Get();
	}

	void ResetFeedbackControllers(FPartRuntimeState& State, bool bDestroyComponents)
	{
		if (State.CuePlayback)
		{
			State.CuePlayback->Reset();
		}
		if (State.ImpactFeedback)
		{
			State.ImpactFeedback->ResetImmediate(bDestroyComponents);
		}
		if (State.TargetPreviewPlayback)
		{
			State.TargetPreviewPlayback->Reset();
		}
		if (State.TargetPreviewFeedback)
		{
			State.TargetPreviewFeedback->ResetImmediate(bDestroyComponents);
		}
		if (State.OutlineFeedback)
		{
			State.OutlineFeedback->ResetImmediate(bDestroyComponents);
		}
		State.bDestroyedSwapPending = false;
		State.bAwaitingDestroyedCue = false;
		State.bDragPreviewActive = false;
		State.bHoverActive = false;
		State.DragPreviewState = EWacomFirstPersonCardDragTargetFeedbackState::None;
	}

	void ClearInteractionCollisionConfiguration(FPartRuntimeState& State)
	{
		for (FFlipbookAuthoredState& Layer : State.Flipbooks)
		{
			if (UWacomBattleEnemyPartFlipbookLayerComponent* Component = Layer.Component.Get())
			{
				Component->ClearInteractionCollision();
			}
		}
		for (FSpriteAuthoredState& Layer : State.Sprites)
		{
			if (UWacomBattleEnemyPartSpriteLayerComponent* Component = Layer.Component.Get())
			{
				Component->ClearInteractionCollision();
			}
		}
		State.InteractionVisual.Reset();
		State.StableInteractionCollisionSprite.Reset();
		State.bInteractionVisualResolved = false;
		State.bInteractionCollisionReady = false;
		State.bUsingBoxCollisionFallback = false;
		State.InteractionCollisionSource = InteractionCollisionSourceNone;
		if (UWacomBattleEnemyPartFallbackCollisionComponent* Fallback =
			State.FallbackCollision.Get())
		{
			Fallback->DisableFallbackCollision();
		}
	}

	void DestroyFallbackCollision(FPartRuntimeState& State)
	{
		if (UWacomBattleEnemyPartFallbackCollisionComponent* Fallback =
			State.FallbackCollision.Get())
		{
			Fallback->DestroyComponent();
		}
		State.FallbackCollision.Reset();
		State.bUsingBoxCollisionFallback = false;
		State.InteractionCollisionSource = InteractionCollisionSourceNone;
	}

	bool AccumulatePartLocalBounds(
		FBox& InOutBounds,
		const FBoxSphereBounds& ResourceBounds,
		const USceneComponent& DirectVisual)
	{
		const FBoxSphereBounds PartLocalBounds =
			ResourceBounds.TransformBy(DirectVisual.GetRelativeTransform());
		const FVector Origin = PartLocalBounds.Origin;
		const FVector Extent = PartLocalBounds.BoxExtent.GetAbs();
		if (Origin.ContainsNaN()
			|| Extent.ContainsNaN()
			|| !FMath::IsFinite(Origin.X)
			|| !FMath::IsFinite(Origin.Y)
			|| !FMath::IsFinite(Origin.Z)
			|| Extent.GetMax() <= UE_SMALL_NUMBER)
		{
			return false;
		}
		InOutBounds += Origin - Extent;
		InOutBounds += Origin + Extent;
		return true;
	}

	struct FFallbackBounds
	{
		FVector RelativeCenter = FVector::ZeroVector;
		FVector HalfExtent = FVector(55.0f, 45.0f, 55.0f);
		FName Source = InteractionCollisionSourceDefault;
	};

	FFallbackBounds ResolveFallbackBounds(const FPartRuntimeState& State)
	{
		FBox Bounds(ForceInit);
		if (State.bInteractionVisualResolved)
		{
			if (const UPaperSprite* StableSprite =
				State.StableInteractionCollisionSprite.Get())
			{
				if (const USceneComponent* InteractionVisual =
					State.InteractionVisual.Get())
				{
					if (AccumulatePartLocalBounds(
						Bounds,
						StableSprite->GetRenderBounds(),
						*InteractionVisual))
					{
						return FFallbackBounds{
							Bounds.GetCenter(),
							Bounds.GetExtent().ComponentMax(FVector(6.0f)),
							InteractionCollisionSourceVisualBounds };
					}
				}
			}
		}

		Bounds.Init();
		bool bHasVisualBounds = false;
		for (const FFlipbookAuthoredState& Layer : State.Flipbooks)
		{
			const UWacomBattleEnemyPartFlipbookLayerComponent* Component =
				Layer.Component.Get();
			const UPaperFlipbook* Flipbook = Layer.Flipbook.Get();
			const UPaperSprite* IdleSprite = Flipbook
				? Flipbook->GetSpriteAtFrame(0)
				: nullptr;
			bHasVisualBounds |= Component
				&& IdleSprite
				&& AccumulatePartLocalBounds(
					Bounds,
					IdleSprite->GetRenderBounds(),
					*Component);
		}
		for (const FSpriteAuthoredState& Layer : State.Sprites)
		{
			const UWacomBattleEnemyPartSpriteLayerComponent* Component =
				Layer.Component.Get();
			const UPaperSprite* Sprite = Layer.Sprite.Get();
			bHasVisualBounds |= Component
				&& Sprite
				&& AccumulatePartLocalBounds(
					Bounds,
					Sprite->GetRenderBounds(),
					*Component);
		}
		if (bHasVisualBounds && Bounds.IsValid)
		{
			return FFallbackBounds{
				Bounds.GetCenter(),
				Bounds.GetExtent().ComponentMax(FVector(6.0f)),
				InteractionCollisionSourceVisualUnion };
		}
		return FFallbackBounds();
	}

	FWacomBattleEnemyPartPresentationBounds ResolvePresentationBounds(
		const FPartRuntimeState& State)
	{
		if (State.bInteractionVisualResolved)
		{
			const UPaperSprite* StableSprite =
				State.StableInteractionCollisionSprite.Get();
			const USceneComponent* InteractionVisual =
				State.InteractionVisual.Get();
			if (StableSprite && InteractionVisual)
			{
				const FWacomBattleEnemyPartPresentationBounds StableBounds =
					FWacomBattleEnemyPartPresentationBounds::FromLocalBounds(
						StableSprite->GetRenderBounds(),
						InteractionVisual->GetComponentTransform(),
						State.bInteractionCollisionReady
							? PresentationBoundsSourceStableVisual
							: InteractionCollisionSourceVisualBounds);
				if (StableBounds.IsValid())
				{
					return StableBounds;
				}
			}
		}

		const UWacomBattleEnemyPartComponent* Part = State.Part.Get();
		if (!Part)
		{
			return FWacomBattleEnemyPartPresentationBounds();
		}
		const FFallbackBounds Fallback = ResolveFallbackBounds(State);
		return FWacomBattleEnemyPartPresentationBounds::FromLocalBounds(
			FBoxSphereBounds(FBox::BuildAABB(
				Fallback.RelativeCenter,
				Fallback.HalfExtent)),
			Part->GetComponentTransform(),
			Fallback.Source);
	}

	UWacomBattleEnemyPartFallbackCollisionComponent* EnsureFallbackCollision(
		FPartRuntimeState& State)
	{
		if (UWacomBattleEnemyPartFallbackCollisionComponent* Existing =
			State.FallbackCollision.Get())
		{
			return Existing;
		}
		UWacomBattleEnemyPartComponent* Part = State.Part.Get();
		AActor* Owner = Part ? Part->GetOwner() : nullptr;
		if (!Part || !Owner)
		{
			return nullptr;
		}

		UWacomBattleEnemyPartFallbackCollisionComponent* Fallback =
			NewObject<UWacomBattleEnemyPartFallbackCollisionComponent>(
				Owner,
				NAME_None,
				RF_Transient);
		if (!Fallback)
		{
			return nullptr;
		}
		Owner->AddInstanceComponent(Fallback);
		Fallback->SetupAttachment(Part);
		Fallback->InitializeForPart(*Part);
		Fallback->RegisterComponent();
		State.FallbackCollision = Fallback;
		return Fallback;
	}

	void ResolveInteractionVisual(FPartRuntimeState& State)
	{
		UWacomBattleEnemyPartComponent* Part = State.Part.Get();
		ClearInteractionCollisionConfiguration(State);
		if (!Part || Part->InteractionVisualLayerId.IsNone())
		{
			return;
		}

		int32 MatchCount = 0;
		UWacomBattleEnemyPartFlipbookLayerComponent* FlipbookMatch = nullptr;
		UWacomBattleEnemyPartSpriteLayerComponent* SpriteMatch = nullptr;
		UPaperSprite* StableSprite = nullptr;
		for (FFlipbookAuthoredState& Layer : State.Flipbooks)
		{
			UWacomBattleEnemyPartFlipbookLayerComponent* Component = Layer.Component.Get();
			if (Component && Component->LayerId == Part->InteractionVisualLayerId)
			{
				++MatchCount;
				FlipbookMatch = Component;
				StableSprite = Layer.Flipbook.IsValid()
					? Layer.Flipbook->GetSpriteAtFrame(0)
					: nullptr;
			}
		}
		for (FSpriteAuthoredState& Layer : State.Sprites)
		{
			UWacomBattleEnemyPartSpriteLayerComponent* Component = Layer.Component.Get();
			if (Component && Component->LayerId == Part->InteractionVisualLayerId)
			{
				++MatchCount;
				SpriteMatch = Component;
				StableSprite = Layer.Sprite.Get();
			}
		}

		State.bInteractionVisualResolved = MatchCount == 1;
		if (!State.bInteractionVisualResolved)
		{
			return;
		}
		if (FlipbookMatch)
		{
			FlipbookMatch->ConfigureInteractionCollision(Part, StableSprite, false);
			State.InteractionVisual = FlipbookMatch;
			State.bInteractionCollisionReady = FlipbookMatch->IsInteractionCollisionReady();
		}
		else if (SpriteMatch)
		{
			SpriteMatch->ConfigureInteractionCollision(Part, StableSprite, false);
			State.InteractionVisual = SpriteMatch;
			State.bInteractionCollisionReady = SpriteMatch->IsInteractionCollisionReady();
		}
		State.StableInteractionCollisionSprite = StableSprite;
	}

	void RefreshInteractionCollision(FPartRuntimeState& State, bool bRuntimeRetired)
	{
		UWacomBattleEnemyPartComponent* Part = State.Part.Get();
		if (!Part)
		{
			return;
		}
		const bool bEnable = !bRuntimeRetired
			&& State.bBound
			&& State.bRegisteredWithHUD
			&& !State.bDestroyed;
		if (State.bInteractionCollisionReady)
		{
			if (UWacomBattleEnemyPartFlipbookLayerComponent* FlipbookLayer =
				Cast<UWacomBattleEnemyPartFlipbookLayerComponent>(State.InteractionVisual.Get()))
			{
				FlipbookLayer->ConfigureInteractionCollision(
					Part,
					State.StableInteractionCollisionSprite.Get(),
					bEnable);
			}
			else if (UWacomBattleEnemyPartSpriteLayerComponent* SpriteLayer =
				Cast<UWacomBattleEnemyPartSpriteLayerComponent>(State.InteractionVisual.Get()))
			{
				SpriteLayer->ConfigureInteractionCollision(
					Part,
					State.StableInteractionCollisionSprite.Get(),
					bEnable);
			}
			if (UWacomBattleEnemyPartFallbackCollisionComponent* Fallback =
				State.FallbackCollision.Get())
			{
				Fallback->DisableFallbackCollision();
			}
			State.bUsingBoxCollisionFallback = false;
			State.InteractionCollisionSource =
				InteractionCollisionSourceStableSprite;
			State.bInteractionFallbackLogged = false;
			return;
		}

		for (FFlipbookAuthoredState& Layer : State.Flipbooks)
		{
			if (UWacomBattleEnemyPartFlipbookLayerComponent* Component = Layer.Component.Get())
			{
				Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
		}
		for (FSpriteAuthoredState& Layer : State.Sprites)
		{
			if (UWacomBattleEnemyPartSpriteLayerComponent* Component = Layer.Component.Get())
			{
				Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
		}
		const FFallbackBounds Bounds = ResolveFallbackBounds(State);
		UWacomBattleEnemyPartFallbackCollisionComponent* Fallback =
			State.FallbackCollision.Get();
		if (!Fallback && bEnable)
		{
			Fallback = EnsureFallbackCollision(State);
		}
		if (Fallback)
		{
			Fallback->ConfigureFallbackBounds(
				Bounds.RelativeCenter,
				Bounds.HalfExtent,
				bEnable);
			State.bUsingBoxCollisionFallback = true;
			State.InteractionCollisionSource = Bounds.Source;
		}
		else
		{
			State.bUsingBoxCollisionFallback = false;
			State.InteractionCollisionSource = InteractionCollisionSourceNone;
		}
		if (bEnable && !State.bInteractionFallbackLogged)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[WacomEnemyInteraction] Part '%s' cannot resolve collision-ready InteractionVisualLayerId '%s'; transient fallback enabled from '%s'."),
				*Part->GetPathName(),
				*Part->InteractionVisualLayerId.ToString(),
				*State.InteractionCollisionSource.ToString());
			State.bInteractionFallbackLogged = true;
		}
	}

	EWacomBattleEnemyPartOutlineState ResolveDesiredOutlineState(
		const FPartRuntimeState& State)
	{
		if (!State.bBound || State.bDestroyed || !State.bInteractionVisualResolved)
		{
			return EWacomBattleEnemyPartOutlineState::None;
		}
		if (State.bDragPreviewActive
			&& State.DragPreviewState == EWacomFirstPersonCardDragTargetFeedbackState::Invalid)
		{
			return EWacomBattleEnemyPartOutlineState::None;
		}
		if (State.bHoverActive && (!State.bTargetSelectionActive || State.bTargetable))
		{
			return EWacomBattleEnemyPartOutlineState::Hovered;
		}
		return State.bTargetSelectionActive && State.bTargetable
			? EWacomBattleEnemyPartOutlineState::Selectable
			: EWacomBattleEnemyPartOutlineState::None;
	}

	void RefreshOutlineFeedback(
		UWacomBattleEnemySceneRuntimeComponent& Owner,
		FPartRuntimeState& State)
	{
		if (!State.OutlineFeedback)
		{
			return;
		}
		UWacomBattleEnemyPartComponent* Part = State.Part.Get();
		const EWacomBattleEnemyPartOutlineState Desired = ResolveDesiredOutlineState(State);
		State.OutlineFeedback->BeginOrUpdate(
			Owner,
			State.InteractionVisual.Get(),
			Part ? Part->ResolveTargetPreviewStyle() : nullptr,
			Desired);
		if (Desired != EWacomBattleEnemyPartOutlineState::None
			&& State.OutlineFeedback->GetDebugView().bVisible)
		{
			Owner.SetComponentTickEnabled(true);
		}
	}

	void RefreshAccessibility(UActorComponent& Owner, float& OutFlashScale, bool& bOutSimplifiedMotion)
	{
		OutFlashScale = 1.0f;
		bOutSimplifiedMotion = false;
		UWorld* World = Owner.GetWorld();
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		const UWacomSettingsSubsystem* Settings = GameInstance
			? GameInstance->GetSubsystem<UWacomSettingsSubsystem>()
			: nullptr;
		if (!Settings)
		{
			return;
		}
		const FWacomLocalSettingsSnapshot& Snapshot = Settings->GetCurrentSnapshot();
		OutFlashScale = FWacomPresentationAccessibilityPolicy::GetDecorativeFlashIntensityScale(
			Snapshot.FlashEffectMode);
		bOutSimplifiedMotion = FWacomPresentationAccessibilityPolicy::UsesSimplifiedMotion(
			Snapshot.UIMotionMode);
	}

	EWacomBattleEnemyPartTargetPreviewKind ResolveDesiredTargetPreviewKind(
		const FPartRuntimeState& State)
	{
		if (!State.bBound || State.bDestroyed)
		{
			return EWacomBattleEnemyPartTargetPreviewKind::None;
		}
		if (State.bDragPreviewActive)
		{
			return State.DragPreviewState
				== EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget
					? EWacomBattleEnemyPartTargetPreviewKind::Valid
					: EWacomBattleEnemyPartTargetPreviewKind::Invalid;
		}
		return State.bTargetable
			? EWacomBattleEnemyPartTargetPreviewKind::Available
			: EWacomBattleEnemyPartTargetPreviewKind::None;
	}

	void RefreshTargetPreview(
		UWacomBattleEnemySceneRuntimeComponent& Owner,
		FPartRuntimeState& State)
	{
		UWacomBattleEnemyPartComponent* Part = State.Part.Get();
		if (!Part || !State.TargetPreviewPlayback || !State.TargetPreviewFeedback)
		{
			return;
		}

		const EWacomBattleEnemyPartTargetPreviewKind DesiredKind =
			ResolveDesiredTargetPreviewKind(State);
		const UWacomBattleEnemyPartTargetPreviewStyle* Style = Part->ResolveTargetPreviewStyle();
		if (DesiredKind == EWacomBattleEnemyPartTargetPreviewKind::None
			|| !Part->bEnableTargetPreviewFeedback
			|| !Style
			|| !Style->HasValidVisualAssets())
		{
			State.TargetPreviewPlayback->BeginExit();
			if (State.TargetPreviewPlayback->GetView().bActive)
			{
				Owner.SetComponentTickEnabled(true);
			}
			return;
		}

		float FlashScale = 1.0f;
		bool bSimplifiedMotion = false;
		RefreshAccessibility(Owner, FlashScale, bSimplifiedMotion);
		State.TargetPreviewFlashScale = FlashScale;
		const bool bAvailability =
			DesiredKind == EWacomBattleEnemyPartTargetPreviewKind::Available;
		State.TargetPreviewPlayback->Begin(
			DesiredKind,
			bAvailability ? Style->AvailabilityEnterSeconds : Style->EnterSeconds,
			bAvailability ? Style->AvailabilityExitSeconds : Style->ExitSeconds,
			Style->PulsePeriodSeconds,
			bSimplifiedMotion);
		State.TargetPreviewFeedback->BeginOrUpdate(
			Owner,
			Part,
			ResolvePresentationBounds(State),
			Style,
			State.TargetPreviewPlayback->GetView(),
			FlashScale);
		Owner.SetComponentTickEnabled(true);
	}
}

struct UWacomBattleEnemySceneRuntimeComponent::FImpl
{
	TArray<FPartRuntimeState> Parts;
	TWeakObjectPtr<UWacomBattleEnemyPartComponent> ActivePlaybackPart;
	TWeakObjectPtr<UWacomBattleEnemyPartFlipbookLayerComponent> ActivePlaybackLayer;
	uint32 TopologyRevision = 0;
	bool bRuntimeRetired = false;
};

UWacomBattleEnemySceneRuntimeComponent::UWacomBattleEnemySceneRuntimeComponent()
	: Impl(new FImpl())
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetComponentTickEnabled(false);
}

UWacomBattleEnemySceneRuntimeComponent::~UWacomBattleEnemySceneRuntimeComponent()
{
	delete Impl;
	Impl = nullptr;
}

void UWacomBattleEnemySceneRuntimeComponent::BeginPlay()
{
	Super::BeginPlay();
	RefreshTypedHierarchy();
}

void UWacomBattleEnemySceneRuntimeComponent::RefreshTypedHierarchy()
{
	AWacomBattleEnemyActor* Host = Cast<AWacomBattleEnemyActor>(GetOwner());
	if (!Host || !Impl)
	{
		return;
	}

	TArray<UWacomBattleEnemyPartComponent*> AllParts;
	Host->GetComponents(AllParts);
	AllParts.RemoveAll([](const UWacomBattleEnemyPartComponent* Part)
	{
		return !IsValid(Part) || !Part->IsRegistered() || Part->GetOwner() == nullptr;
	});
	AllParts.Sort([](const UWacomBattleEnemyPartComponent& Left, const UWacomBattleEnemyPartComponent& Right)
	{
		return Left.GetName() < Right.GetName();
	});

	TArray<UWacomBattleEnemyPartComponent*> OrderedParts;
	if (Host->EnemyDefinition)
	{
		for (const FEnemyPartSlot& DefinitionSlot : Host->EnemyDefinition->Parts)
		{
			if (DefinitionSlot.PartSlotId.IsNone() || !DefinitionSlot.PartDef)
			{
				continue;
			}
			TArray<UWacomBattleEnemyPartComponent*> Matches = AllParts.FilterByPredicate(
				[&DefinitionSlot](const UWacomBattleEnemyPartComponent* Part)
				{
					return Part && Part->PartSlotId == DefinitionSlot.PartSlotId;
				});
			if (Matches.Num() == 1)
			{
				OrderedParts.Add(Matches[0]);
			}
		}
	}

	bool bTopologyChanged = OrderedParts.Num() != Impl->Parts.Num();
	if (!bTopologyChanged)
	{
		for (int32 Index = 0; Index < OrderedParts.Num(); ++Index)
		{
			bTopologyChanged |= Impl->Parts[Index].Part.Get() != OrderedParts[Index];
		}
	}

	TArray<FPartRuntimeState> Previous = MoveTemp(Impl->Parts);
	Impl->Parts.Reset(OrderedParts.Num());
	TArray<UWacomBattleEnemyPartFlipbookLayerComponent*> AllFlipbooks;
	TArray<UWacomBattleEnemyPartSpriteLayerComponent*> AllSprites;
	TArray<UWacomBattleEnemyPartImpactAnchorComponent*> AllAnchors;
	Host->GetComponents(AllFlipbooks);
	Host->GetComponents(AllSprites);
	Host->GetComponents(AllAnchors);

	for (UWacomBattleEnemyPartComponent* Part : OrderedParts)
	{
		const int32 PreviousIndex = Previous.IndexOfByPredicate([Part](const FPartRuntimeState& State)
		{
			return State.Part.Get() == Part;
		});
		FPartRuntimeState State;
		if (PreviousIndex != INDEX_NONE)
		{
			State = MoveTemp(Previous[PreviousIndex]);
			Previous.RemoveAt(PreviousIndex);
		}
		State.Part = Part;
		State.PartSlotId = Part->PartSlotId;
		State.PartId = Part->PartId;

		TArray<UWacomBattleEnemyPartFlipbookLayerComponent*> DirectFlipbooks =
			AllFlipbooks.FilterByPredicate([Part](const UWacomBattleEnemyPartFlipbookLayerComponent* Layer)
			{
				return IsValid(Layer)
					&& Layer->IsRegistered()
					&& Layer->GetAttachParent() == Part;
			});
		TArray<UWacomBattleEnemyPartSpriteLayerComponent*> DirectSprites =
			AllSprites.FilterByPredicate([Part](const UWacomBattleEnemyPartSpriteLayerComponent* Layer)
			{
				return IsValid(Layer)
					&& Layer->IsRegistered()
					&& Layer->GetAttachParent() == Part;
			});
		TArray<UWacomBattleEnemyPartImpactAnchorComponent*> DirectAnchors =
			AllAnchors.FilterByPredicate([Part](const UWacomBattleEnemyPartImpactAnchorComponent* Anchor)
			{
				return IsValid(Anchor) && Anchor->GetAttachParent() == Part;
			});

		DirectFlipbooks.Sort([](const auto& Left, const auto& Right) { return Left.GetName() < Right.GetName(); });
		DirectSprites.Sort([](const auto& Left, const auto& Right) { return Left.GetName() < Right.GetName(); });
		DirectAnchors.Sort([](const auto& Left, const auto& Right) { return Left.GetName() < Right.GetName(); });
		bTopologyChanged |= State.Flipbooks.Num() != DirectFlipbooks.Num()
			|| State.Sprites.Num() != DirectSprites.Num()
			|| State.ImpactAnchorCount != DirectAnchors.Num();

		TArray<FFlipbookAuthoredState> PreviousFlipbooks = MoveTemp(State.Flipbooks);
		for (UWacomBattleEnemyPartFlipbookLayerComponent* Layer : DirectFlipbooks)
		{
			const int32 ExistingIndex = PreviousFlipbooks.IndexOfByPredicate([Layer](const FFlipbookAuthoredState& Entry)
			{
				return Entry.Component.Get() == Layer;
			});
			if (ExistingIndex != INDEX_NONE)
			{
				State.Flipbooks.Add(MoveTemp(PreviousFlipbooks[ExistingIndex]));
				PreviousFlipbooks.RemoveAt(ExistingIndex);
				if (!State.bBound)
				{
					FFlipbookAuthoredState& Authored = State.Flipbooks.Last();
					Authored.Flipbook.Reset(Layer->GetFlipbook());
					Authored.PlayRate = Layer->GetPlayRate();
					Authored.PlaybackPosition = FMath::Max(
						0.0f, Layer->InitialPlaybackPositionSeconds);
					Authored.bLooping = Layer->IsLooping();
					Authored.bPlaying = Layer->IsPlaying();
					Authored.bVisible = Layer->IsVisible();
					Authored.RelativeScale = Layer->GetRelativeScale3D();
				}
			}
			else
			{
				FFlipbookAuthoredState& Authored = State.Flipbooks.AddDefaulted_GetRef();
				Authored.Component = Layer;
				Authored.Flipbook.Reset(Layer->GetFlipbook());
				Authored.PlayRate = Layer->GetPlayRate();
				Authored.PlaybackPosition = FMath::Max(0.0f, Layer->InitialPlaybackPositionSeconds);
				Authored.bLooping = Layer->IsLooping();
				Authored.bPlaying = Layer->IsPlaying();
				Authored.bVisible = Layer->IsVisible();
				Authored.RelativeScale = Layer->GetRelativeScale3D();
				bTopologyChanged = true;
			}
		}

		TArray<FSpriteAuthoredState> PreviousSprites = MoveTemp(State.Sprites);
		for (UWacomBattleEnemyPartSpriteLayerComponent* Layer : DirectSprites)
		{
			const int32 ExistingIndex = PreviousSprites.IndexOfByPredicate([Layer](const FSpriteAuthoredState& Entry)
			{
				return Entry.Component.Get() == Layer;
			});
			if (ExistingIndex != INDEX_NONE)
			{
				State.Sprites.Add(MoveTemp(PreviousSprites[ExistingIndex]));
				PreviousSprites.RemoveAt(ExistingIndex);
				if (!State.bBound)
				{
					FSpriteAuthoredState& Authored = State.Sprites.Last();
					Authored.Sprite.Reset(Layer->GetSprite());
					Authored.bVisible = Layer->IsVisible();
					Authored.RelativeScale = Layer->GetRelativeScale3D();
				}
			}
			else
			{
				FSpriteAuthoredState& Authored = State.Sprites.AddDefaulted_GetRef();
				Authored.Component = Layer;
				Authored.Sprite.Reset(Layer->GetSprite());
				Authored.bVisible = Layer->IsVisible();
				Authored.RelativeScale = Layer->GetRelativeScale3D();
				bTopologyChanged = true;
			}
		}
		State.ImpactAnchor = DirectAnchors.IsEmpty() ? nullptr : DirectAnchors[0];
		State.ImpactAnchorCount = DirectAnchors.Num();
		UPrimitiveComponent* PreviousInteractionVisual = State.InteractionVisual.Get();
		ResolveInteractionVisual(State);
		bTopologyChanged |= PreviousInteractionVisual != State.InteractionVisual.Get();
		RefreshInteractionCollision(State, Impl->bRuntimeRetired);
		RefreshOutlineFeedback(*this, State);
		Impl->Parts.Add(MoveTemp(State));
	}

	for (FPartRuntimeState& Removed : Previous)
	{
		if (Removed.ActionPlayback)
		{
			Removed.ActionPlayback->Cancel(false);
		}
		ClearInteractionCollisionConfiguration(Removed);
		DestroyFallbackCollision(Removed);
		ResetFeedbackControllers(Removed, true);
	}
	if (bTopologyChanged)
	{
		++Impl->TopologyRevision;
	}
}

void UWacomBattleEnemySceneRuntimeComponent::NotifyTypedHierarchyChanged()
{
	RefreshTypedHierarchy();
}

void UWacomBattleEnemySceneRuntimeComponent::GetOrderedPartComponents(
	TArray<UWacomBattleEnemyPartComponent*>& OutParts) const
{
	OutParts.Reset();
	if (!Impl)
	{
		return;
	}
	for (const FPartRuntimeState& State : Impl->Parts)
	{
		if (UWacomBattleEnemyPartComponent* Part = State.Part.Get())
		{
			OutParts.Add(Part);
		}
	}
}

uint32 UWacomBattleEnemySceneRuntimeComponent::GetTopologyRevision() const
{
	return Impl ? Impl->TopologyRevision : 0;
}

void UWacomBattleEnemySceneRuntimeComponent::InitializeRuntimeSceneBinding(
	FName EncounterId,
	FName EnemySlotId)
{
	RefreshTypedHierarchy();
	if (!Impl)
	{
		return;
	}
	for (FPartRuntimeState& State : Impl->Parts)
	{
		State.EncounterId = EncounterId;
		State.EnemySlotId = EnemySlotId;
		if (UWacomBattleEnemyPartComponent* Part = State.Part.Get())
		{
			State.PartSlotId = Part->PartSlotId;
			State.PartId = Part->PartId;
		}
	}
}

bool UWacomBattleEnemySceneRuntimeComponent::ApplyPartSnapshotFacts(
	UWacomBattleEnemyPartComponent& Part,
	const FEnemyPartSnapshot* SnapshotPart,
	bool bInTargetSelectionActive,
	bool bInTargetable,
	FName InTargetDisabledReason)
{
	FPartRuntimeState* State = Impl ? FindState(Impl->Parts, Part) : nullptr;
	if (!State || State->EncounterId.IsNone() || State->EnemySlotId.IsNone() || State->PartSlotId.IsNone())
	{
		return false;
	}
	if (!SnapshotPart)
	{
		const bool bFactsChanged = State->bBound
			|| State->PartInstanceId.IsValid()
			|| State->CurrentInitiative != 0
			|| !State->CurrentIntentId.IsNone()
			|| State->bDestroyed;
		const bool bTargetableChanged = State->bTargetSelectionActive
			|| State->bTargetable
			|| !State->TargetDisabledReason.IsNone();
		State->bBound = false;
		State->PartInstanceId.Invalidate();
		State->CurrentInitiative = 0;
		State->CurrentIntentId = NAME_None;
		State->bDestroyed = false;
		State->bAwaitingDestroyedCue = false;
		State->bTargetable = false;
		State->bTargetSelectionActive = false;
		State->TargetDisabledReason = NAME_None;
		if (bTargetableChanged)
		{
			++State->TargetableApplyCount;
		}
		if (bFactsChanged)
		{
			++State->SnapshotApplyCount;
		}
		else
		{
			++State->SnapshotNoOpCount;
		}
		RefreshTargetPreview(*this, *State);
		RefreshInteractionCollision(*State, Impl->bRuntimeRetired);
		RefreshOutlineFeedback(*this, *State);
		return false;
	}

	const bool bHadFacts = State->PartInstanceId.IsValid();
	const bool bInstanceChanged = bHadFacts && State->PartInstanceId != SnapshotPart->InstanceId;
	const bool bWasDestroyed = State->bDestroyed;
	const bool bNewBound = SnapshotPart->InstanceId.IsValid() && !SnapshotPart->bDestroyed;
	const bool bFactsChanged = State->PartInstanceId != SnapshotPart->InstanceId
		|| State->CurrentInitiative != SnapshotPart->CurrentInitiative
		|| State->CurrentIntentId != SnapshotPart->CurrentIntentId
		|| State->bDestroyed != SnapshotPart->bDestroyed
		|| State->bBound != bNewBound;
	if (bInstanceChanged)
	{
		RestorePartAuthoredVisualState(Part);
	}
	State->PartInstanceId = SnapshotPart->InstanceId;
	State->CurrentInitiative = SnapshotPart->CurrentInitiative;
	State->CurrentIntentId = SnapshotPart->CurrentIntentId;
	State->bDestroyed = SnapshotPart->bDestroyed;
	if (SnapshotPart->bDestroyed)
	{
		if (!bHadFacts || bInstanceChanged)
		{
			ApplyPartDestroyedVisualState(Part);
			State->bAwaitingDestroyedCue = false;
		}
		else if (!bWasDestroyed)
		{
			State->bAwaitingDestroyedCue = true;
		}
	}
	else
	{
		State->bAwaitingDestroyedCue = false;
	}
	State->bBound = bNewBound;
	const bool bEffectiveTargetable = bNewBound && bInTargetable;
	const FName EffectiveReason = bNewBound ? InTargetDisabledReason : NAME_None;
	const bool bTargetableChanged = State->bTargetSelectionActive != bInTargetSelectionActive
		|| State->bTargetable != bEffectiveTargetable
		|| State->TargetDisabledReason != EffectiveReason;
	State->bTargetSelectionActive = bInTargetSelectionActive;
	State->bTargetable = bEffectiveTargetable;
	State->TargetDisabledReason = EffectiveReason;
	if (bTargetableChanged)
	{
		++State->TargetableApplyCount;
	}
	if (bFactsChanged)
	{
		++State->SnapshotApplyCount;
	}
	else
	{
		++State->SnapshotNoOpCount;
	}
	RefreshTargetPreview(*this, *State);
	RefreshInteractionCollision(*State, Impl->bRuntimeRetired);
	RefreshOutlineFeedback(*this, *State);
	return State->bBound;
}

void UWacomBattleEnemySceneRuntimeComponent::ClearPartBattleBinding(
	UWacomBattleEnemyPartComponent& Part,
	bool bClearRuntimeFacts)
{
	FPartRuntimeState* State = Impl ? FindState(Impl->Parts, Part) : nullptr;
	if (!State)
	{
		return;
	}
	State->bBound = false;
	State->bRegisteredWithHUD = false;
	State->bTargetable = false;
	State->bTargetSelectionActive = false;
	State->TargetDisabledReason = NAME_None;
	if (bClearRuntimeFacts)
	{
		State->PartInstanceId.Invalidate();
		State->CurrentInitiative = 0;
		State->CurrentIntentId = NAME_None;
		State->bDestroyed = false;
	}
	ResetFeedbackControllers(*State, false);
	RefreshInteractionCollision(*State, Impl->bRuntimeRetired);
	RefreshOutlineFeedback(*this, *State);
}

void UWacomBattleEnemySceneRuntimeComponent::ClearAllBattleBindings(bool bClearRuntimeFacts)
{
	if (!Impl)
	{
		return;
	}
	for (FPartRuntimeState& State : Impl->Parts)
	{
		if (UWacomBattleEnemyPartComponent* Part = State.Part.Get())
		{
			ClearPartBattleBinding(*Part, bClearRuntimeFacts);
		}
	}
}

bool UWacomBattleEnemySceneRuntimeComponent::IsPartBound(
	const UWacomBattleEnemyPartComponent& Part) const
{
	const FPartRuntimeState* State = Impl ? FindState(Impl->Parts, Part) : nullptr;
	return State && State->bBound;
}

bool UWacomBattleEnemySceneRuntimeComponent::IsPartRegisteredWithHUD(
	const UWacomBattleEnemyPartComponent& Part) const
{
	const FPartRuntimeState* State = Impl ? FindState(Impl->Parts, Part) : nullptr;
	return State && State->bRegisteredWithHUD;
}

void UWacomBattleEnemySceneRuntimeComponent::SetPartRegisteredWithHUD(
	UWacomBattleEnemyPartComponent& Part,
	bool bInRegistered)
{
	if (FPartRuntimeState* State = Impl ? FindState(Impl->Parts, Part) : nullptr)
	{
		State->bRegisteredWithHUD = bInRegistered;
		RefreshInteractionCollision(*State, Impl->bRuntimeRetired);
	}
}

void UWacomBattleEnemySceneRuntimeComponent::SetPartTargetable(
	UWacomBattleEnemyPartComponent& Part,
	bool bTargetable,
	FName DisabledReason)
{
	if (FPartRuntimeState* State = Impl ? FindState(Impl->Parts, Part) : nullptr)
	{
		if (State->bTargetable == bTargetable
			&& State->TargetDisabledReason == DisabledReason)
		{
			return;
		}
		State->bTargetable = bTargetable;
		State->TargetDisabledReason = DisabledReason;
		++State->TargetableApplyCount;
		RefreshTargetPreview(*this, *State);
		RefreshOutlineFeedback(*this, *State);
	}
}

FWacomInteractionTargetHandle UWacomBattleEnemySceneRuntimeComponent::BuildWorldTargetHandle(
	const UWacomBattleEnemyPartComponent& Part) const
{
	const FPartRuntimeState* State = Impl ? FindState(Impl->Parts, Part) : nullptr;
	if (!State || !State->bBound || !State->bRegisteredWithHUD || !State->PartInstanceId.IsValid())
	{
		return FWacomInteractionTargetHandle();
	}
	return FWacomInteractionTargetHandle::ForWorldTarget(
		State->PartInstanceId,
		const_cast<UWacomBattleEnemyPartComponent*>(&Part),
		Part.GetComponentLocation(),
		FVector2D::ZeroVector,
		WacomTags::Interaction_Target_Battle_EnemyPart,
		Part.GetStableSceneTargetId(),
		State->EncounterId,
		State->EnemySlotId,
		State->PartSlotId);
}

FWacomBattleEnemyPartRuntimeDebugView UWacomBattleEnemySceneRuntimeComponent::BuildPartDebugView(
	const UWacomBattleEnemyPartComponent& Part) const
{
	FWacomBattleEnemyPartRuntimeDebugView View;
	View.PartSlotId = Part.PartSlotId;
	View.PartId = Part.PartId;
	View.InteractionVisualLayerId = Part.InteractionVisualLayerId;
	const FPartRuntimeState* State = Impl ? FindState(Impl->Parts, Part) : nullptr;
	if (!State)
	{
		return View;
	}
	View.EncounterId = State->EncounterId;
	View.EnemySlotId = State->EnemySlotId;
	View.PartInstanceId = State->PartInstanceId;
	View.bBoundToSnapshot = State->bBound;
	View.bRegisteredWithBattleHUD = State->bRegisteredWithHUD;
	View.bTargetable = State->bTargetable;
	View.bDestroyed = State->bDestroyed;
	View.bRuntimeRetired = Impl->bRuntimeRetired;
	View.FlipbookLayerCount = State->Flipbooks.Num();
	View.SpriteLayerCount = State->Sprites.Num();
	View.ImpactAnchorCount = State->ImpactAnchorCount;
	View.bInteractionVisualResolved = State->bInteractionVisualResolved;
	View.bInteractionCollisionReady = State->bInteractionCollisionReady;
	View.bUsingBoxCollisionFallback = State->bUsingBoxCollisionFallback;
	View.InteractionCollisionSource = State->InteractionCollisionSource;
	View.OutlineState = State->OutlineFeedback
		? State->OutlineFeedback->GetDebugView().State
		: FName(TEXT("None"));
	View.bOutlineComponentCreated = State->OutlineFeedback
		&& State->OutlineFeedback->GetDebugView().bComponentCreated;
	View.OutlineComponentCreateCount = State->OutlineFeedback
		? State->OutlineFeedback->GetDebugView().ComponentCreateCount
		: 0;
	View.ActionPlaybackCount = State->ActionPlayback
		? State->ActionPlayback->GetView().PlaybackCount
		: 0;
	if (State->ActionPlayback)
	{
		const FWacomBattleEnemyActionPlaybackView& PlaybackView =
			State->ActionPlayback->GetView();
		View.bActionPlaybackActive = PlaybackView.bActive;
		View.bActionImpactFired = PlaybackView.bImpactFired;
		View.ActionImpactCount = PlaybackView.ImpactCount;
		View.ActionWatchdogCompletionCount = PlaybackView.WatchdogCompletionCount;
	}
	View.DestroyedVisualApplyCount = State->DestroyedVisualApplyCount;
	View.LastCueKind = State->LastCueKind;
	View.CuePlayCount = State->CuePlayCount;
	View.CuePlaybackDurationSeconds = State->CuePlaybackDurationSeconds;
	View.SnapshotApplyCount = State->SnapshotApplyCount;
	View.SnapshotNoOpCount = State->SnapshotNoOpCount;
	View.TargetableApplyCount = State->TargetableApplyCount;
	View.TargetPreviewKind = FWacomBattleEnemyPartTargetPreviewPlayback::KindToName(
		ResolveDesiredTargetPreviewKind(*State));
	return View;
}

#if WITH_DEV_AUTOMATION_TESTS

FName UWacomBattleEnemySceneRuntimeComponent::GetPartPresentationBoundsSourceForAutomation(
	const UWacomBattleEnemyPartComponent& Part) const
{
	const FPartRuntimeState* State = Impl ? FindState(Impl->Parts, Part) : nullptr;
	return State
		? ResolvePresentationBounds(*State).GetSource()
		: NAME_None;
}

FVector UWacomBattleEnemySceneRuntimeComponent::GetPartPresentationBoundsCenterForAutomation(
	const UWacomBattleEnemyPartComponent& Part) const
{
	const FPartRuntimeState* State = Impl ? FindState(Impl->Parts, Part) : nullptr;
	return State
		? ResolvePresentationBounds(*State).GetWorldCenter()
		: FVector::ZeroVector;
}

FVector2D
UWacomBattleEnemySceneRuntimeComponent::GetPartPresentationBoundsProjectedSizeForAutomation(
	const UWacomBattleEnemyPartComponent& Part,
	const FVector& PlaneRight,
	const FVector& PlaneUp) const
{
	const FPartRuntimeState* State = Impl ? FindState(Impl->Parts, Part) : nullptr;
	return State
		? ResolvePresentationBounds(*State).ProjectSizeCentimeters(
			PlaneRight,
			PlaneUp)
		: FVector2D::ZeroVector;
}

#endif

bool UWacomBattleEnemySceneRuntimeComponent::TryResolvePartPresentationAnchorWorldLocation(
	const UWacomBattleEnemyPartComponent& Part,
	FVector& OutWorldLocation) const
{
	const FPartRuntimeState* State = Impl ? FindState(Impl->Parts, Part) : nullptr;
	if (State)
	{
		if (const UWacomBattleEnemyPartImpactAnchorComponent* ImpactAnchor =
			State->ImpactAnchor.Get())
		{
			OutWorldLocation = ImpactAnchor->GetComponentLocation();
			return true;
		}

		const FVector BoundsCenter = ResolvePresentationBounds(*State).GetWorldCenter();
		if (!BoundsCenter.ContainsNaN())
		{
			OutWorldLocation = BoundsCenter;
			return true;
		}
	}

	OutWorldLocation = Part.GetComponentLocation();
	return !OutWorldLocation.ContainsNaN();
}

void UWacomBattleEnemySceneRuntimeComponent::PlayPartActionAnimation(
	UWacomBattleEnemyPartComponent& Part,
	FName IntentId,
	FWacomBattleEnemyActionPlaybackCallbacks&& Callbacks)
{
	FPartRuntimeState* State = Impl ? FindState(Impl->Parts, Part) : nullptr;
	const UWacomBattleEnemyPartAnimationStyle* Style = Part.PartAnimationStyle;
	const FWacomBattleEnemyPartAnimationClip* Clip = Style
		? Style->ResolveActionClip(IntentId)
		: nullptr;
	if (!State || Impl->bRuntimeRetired || State->bDestroyed || !Style || !Clip)
	{
		Callbacks.CompleteImmediately();
		return;
	}
	FFlipbookAuthoredState* LayerState = State->Flipbooks.FindByPredicate([Style](const FFlipbookAuthoredState& Entry)
	{
		const UWacomBattleEnemyPartFlipbookLayerComponent* Layer = Entry.Component.Get();
		return Layer && Layer->LayerId == Style->TargetVisualLayerId;
	});
	UWacomBattleEnemyPartFlipbookLayerComponent* Layer = LayerState ? LayerState->Component.Get() : nullptr;
	if (!Layer || !Clip->IsRuntimeUsable())
	{
		Callbacks.CompleteImmediately();
		return;
	}

	CancelAllPlayback(true);
	Impl->ActivePlaybackPart = &Part;
	Impl->ActivePlaybackLayer = Layer;
	Layer->OnFinishedPlaying.RemoveDynamic(this, &UWacomBattleEnemySceneRuntimeComponent::HandleActiveFlipbookFinished);
	Layer->OnFinishedPlaying.AddDynamic(this, &UWacomBattleEnemySceneRuntimeComponent::HandleActiveFlipbookFinished);
	State->CurrentAnimationIntentId = IntentId;
	State->bTerminalPlayback = false;
	const float Duration = Clip->Flipbook->GetTotalDuration() / Clip->PlayRate;
	FWacomBattleEnemyActionPlaybackRequest Request;
	Request.LifetimeOwner = this;
	Request.DurationSeconds = Duration;
	Request.ImpactNormalizedTime = Clip->ImpactNormalizedTime;
	Request.Callbacks = MoveTemp(Callbacks);
	Request.StartVisual = [Layer, Clip]()
	{
		Layer->SetFlipbook(Clip->Flipbook);
		Layer->SetPlayRate(Clip->PlayRate);
		Layer->SetLooping(false);
		Layer->SetPlaybackPosition(0.0f, false);
		Layer->Play();
		return true;
	};
	Request.FinalizeVisual = [this, WeakPart = TWeakObjectPtr<UWacomBattleEnemyPartComponent>(&Part)](
		const FWacomBattleEnemyActionPlaybackFinishContext& Context)
	{
		UWacomBattleEnemyPartComponent* StrongPart = WeakPart.Get();
		FPartRuntimeState* StrongState = StrongPart && Impl ? FindState(Impl->Parts, *StrongPart) : nullptr;
		if (UWacomBattleEnemyPartFlipbookLayerComponent* ActiveLayer = Impl->ActivePlaybackLayer.Get())
		{
			ActiveLayer->OnFinishedPlaying.RemoveDynamic(
				this,
				&UWacomBattleEnemySceneRuntimeComponent::HandleActiveFlipbookFinished);
		}
		if (StrongState && Context.bRestoreAuthoredVisual && !StrongState->bDestroyed)
		{
			for (FFlipbookAuthoredState& Authored : StrongState->Flipbooks)
			{
				if (Authored.Component.Get() == Impl->ActivePlaybackLayer.Get())
				{
					RestoreFlipbook(Authored);
					break;
				}
			}
		}
		if (StrongState)
		{
			StrongState->CurrentAnimationIntentId = NAME_None;
			StrongState->bTerminalPlayback = false;
		}
		Impl->ActivePlaybackLayer.Reset();
		Impl->ActivePlaybackPart.Reset();
	};
	State->ActionPlayback->Begin(MoveTemp(Request));
}

void UWacomBattleEnemySceneRuntimeComponent::PlayEnemyDestroyedAnimation(
	TFunction<void()>&& Completion)
{
	if (!Impl || Impl->bRuntimeRetired)
	{
		if (Completion) Completion();
		return;
	}
	FPartRuntimeState* TerminalState = nullptr;
	const FWacomBattleEnemyPartAnimationClip* TerminalClip = nullptr;
	UWacomBattleEnemyPartFlipbookLayerComponent* TerminalLayer = nullptr;
	for (FPartRuntimeState& State : Impl->Parts)
	{
		UWacomBattleEnemyPartComponent* Part = State.Part.Get();
		const UWacomBattleEnemyPartAnimationStyle* Style = Part ? Part->PartAnimationStyle : nullptr;
		const FWacomBattleEnemyPartAnimationClip* Clip = Style
			? Style->ResolveEnemyDestroyedClip()
			: nullptr;
		if (!Clip)
		{
			continue;
		}
		FFlipbookAuthoredState* LayerState = State.Flipbooks.FindByPredicate([Style](const FFlipbookAuthoredState& Entry)
		{
			const UWacomBattleEnemyPartFlipbookLayerComponent* Layer = Entry.Component.Get();
			return Layer && Layer->LayerId == Style->TargetVisualLayerId;
		});
		if (!LayerState || !LayerState->Component.IsValid() || TerminalState)
		{
			if (Completion) Completion();
			return;
		}
		TerminalState = &State;
		TerminalClip = Clip;
		TerminalLayer = LayerState->Component.Get();
	}
	if (!TerminalState || !TerminalClip || !TerminalLayer)
	{
		if (Completion) Completion();
		return;
	}

	CancelAllPlayback(false);
	Impl->ActivePlaybackPart = TerminalState->Part;
	Impl->ActivePlaybackLayer = TerminalLayer;
	TerminalLayer->OnFinishedPlaying.RemoveDynamic(this, &UWacomBattleEnemySceneRuntimeComponent::HandleActiveFlipbookFinished);
	TerminalLayer->OnFinishedPlaying.AddDynamic(this, &UWacomBattleEnemySceneRuntimeComponent::HandleActiveFlipbookFinished);
	TerminalState->bTerminalPlayback = true;
	const float Duration = TerminalClip->Flipbook->GetTotalDuration() / TerminalClip->PlayRate;
	FWacomBattleEnemyActionPlaybackRequest Request;
	Request.LifetimeOwner = this;
	Request.DurationSeconds = Duration;
	Request.Callbacks.OnCompleted = MoveTemp(Completion);
	Request.StartVisual = [TerminalLayer, TerminalClip]()
	{
		TerminalLayer->SetFlipbook(TerminalClip->Flipbook);
		TerminalLayer->SetPlayRate(TerminalClip->PlayRate);
		TerminalLayer->SetLooping(false);
		TerminalLayer->SetPlaybackPosition(0.0f, false);
		TerminalLayer->Play();
		return true;
	};
	Request.FinalizeVisual = [this, WeakPart = TerminalState->Part](const FWacomBattleEnemyActionPlaybackFinishContext&)
	{
		if (UWacomBattleEnemyPartFlipbookLayerComponent* Layer = Impl->ActivePlaybackLayer.Get())
		{
			Layer->OnFinishedPlaying.RemoveDynamic(this, &UWacomBattleEnemySceneRuntimeComponent::HandleActiveFlipbookFinished);
			Layer->SetPlaybackPosition(Layer->GetFlipbookLength(), false);
			Layer->Stop();
		}
		if (UWacomBattleEnemyPartComponent* Part = WeakPart.Get())
		{
			if (FPartRuntimeState* State = FindState(Impl->Parts, *Part))
			{
				State->bTerminalPlayback = true;
			}
		}
		Impl->ActivePlaybackLayer.Reset();
		Impl->ActivePlaybackPart.Reset();
	};
	TerminalState->ActionPlayback->Begin(MoveTemp(Request));
}

void UWacomBattleEnemySceneRuntimeComponent::HandleActiveFlipbookFinished()
{
	UWacomBattleEnemyPartComponent* Part = Impl ? Impl->ActivePlaybackPart.Get() : nullptr;
	FPartRuntimeState* State = Part && Impl ? FindState(Impl->Parts, *Part) : nullptr;
	if (State && State->ActionPlayback)
	{
		State->ActionPlayback->NotifyFinished();
	}
}

void UWacomBattleEnemySceneRuntimeComponent::CancelPartActionAnimation(
	UWacomBattleEnemyPartComponent& Part,
	bool bRestoreAuthoredVisual)
{
	if (FPartRuntimeState* State = Impl ? FindState(Impl->Parts, Part) : nullptr)
	{
		if (State->ActionPlayback)
		{
			State->ActionPlayback->Cancel(bRestoreAuthoredVisual);
		}
	}
}

void UWacomBattleEnemySceneRuntimeComponent::CancelAllPlayback(bool bRestoreAuthoredVisual)
{
	if (!Impl)
	{
		return;
	}
	for (FPartRuntimeState& State : Impl->Parts)
	{
		if (State.ActionPlayback)
		{
			State.ActionPlayback->Cancel(bRestoreAuthoredVisual);
		}
	}
}

int32 UWacomBattleEnemySceneRuntimeComponent::ApplyPartDestroyedVisualState(
	UWacomBattleEnemyPartComponent& Part)
{
	FPartRuntimeState* State = Impl ? FindState(Impl->Parts, Part) : nullptr;
	if (!State || State->bDestroyedVisualApplied)
	{
		return 0;
	}
	CancelPartActionAnimation(Part, false);
	int32 Changed = 0;
	for (FSpriteAuthoredState& Authored : State->Sprites)
	{
		UWacomBattleEnemyPartSpriteLayerComponent* Layer = Authored.Component.Get();
		if (Layer && Layer->DestroyedSprite)
		{
			Layer->SetSprite(Layer->DestroyedSprite);
			++Changed;
		}
	}
	for (FFlipbookAuthoredState& Authored : State->Flipbooks)
	{
		UWacomBattleEnemyPartFlipbookLayerComponent* Layer = Authored.Component.Get();
		if (Layer && Layer->DestroyedFlipbook)
		{
			Layer->SetFlipbook(Layer->DestroyedFlipbook);
			Layer->SetLooping(false);
			Layer->SetPlayRate(FMath::Max(UE_SMALL_NUMBER, Layer->DestroyedPlayRate));
			Layer->SetPlaybackPosition(0.0f, false);
			Layer->Play();
			++Changed;
		}
	}
	State->bDestroyedVisualApplied = true;
	State->bDestroyedSwapPending = false;
	++State->DestroyedVisualApplyCount;
	return Changed;
}

void UWacomBattleEnemySceneRuntimeComponent::RestorePartAuthoredVisualState(
	UWacomBattleEnemyPartComponent& Part)
{
	FPartRuntimeState* State = Impl ? FindState(Impl->Parts, Part) : nullptr;
	if (!State)
	{
		return;
	}
	CancelPartActionAnimation(Part, true);
	for (FFlipbookAuthoredState& Authored : State->Flipbooks)
	{
		RestoreFlipbook(Authored);
	}
	for (FSpriteAuthoredState& Authored : State->Sprites)
	{
		RestoreSprite(Authored);
	}
	State->bDestroyedVisualApplied = false;
	State->bDestroyedSwapPending = false;
	State->bAwaitingDestroyedCue = false;
	State->bTerminalPlayback = false;
}

void UWacomBattleEnemySceneRuntimeComponent::PlayPartPresentationCue(
	UWacomBattleEnemyPartComponent& Part,
	const FWacomBattlePresentationTargetCue& Cue)
{
	FPartRuntimeState* State = Impl ? FindState(Impl->Parts, Part) : nullptr;
	if (!State || !State->CuePlayback || !State->CuePlayback->Begin(Cue, Part.CueHoldSeconds))
	{
		return;
	}
	const FWacomBattleEnemyPartCuePlaybackView& CueView = State->CuePlayback->GetView();
	State->LastCueKind = FWacomBattleEnemyPartCuePlayback::KindToName(CueView.Kind);
	++State->CuePlayCount;
	State->CuePlaybackDurationSeconds = CueView.DurationSeconds;
	if (CueView.Kind == EWacomBattleEnemyPartCuePlaybackKind::Destroyed)
	{
		State->bAwaitingDestroyedCue = false;
		State->bDestroyedSwapPending = true;
	}
	float FlashScale = 1.0f;
	bool bSimplifiedMotion = false;
	RefreshAccessibility(*this, FlashScale, bSimplifiedMotion);
	if (Part.bEnableImpactFeedback && Part.ResolveImpactStyle() && State->ImpactFeedback)
	{
		FWacomBattlePresentationTargetCue EffectiveCue = Cue;
		EffectiveCue.Duration = CueView.DurationSeconds;
		State->ImpactFeedback->PlayAcceptedCue(
			*this,
			ResolveImpactAnchor(*State),
			ResolvePresentationBounds(*State),
			Part.ResolveImpactStyle(),
			CueView.Kind,
			EffectiveCue,
			FlashScale,
			bSimplifiedMotion);
	}
	SetComponentTickEnabled(true);
}

void UWacomBattleEnemySceneRuntimeComponent::ForceCompletePartPresentationCue(
	UWacomBattleEnemyPartComponent& Part)
{
	FPartRuntimeState* State = Impl ? FindState(Impl->Parts, Part) : nullptr;
	if (!State)
	{
		return;
	}
	if (State->bDestroyedSwapPending)
	{
		ApplyPartDestroyedVisualState(Part);
	}
	if (State->CuePlayback)
	{
		State->CuePlayback->ForceComplete();
	}
	if (State->ImpactFeedback)
	{
		State->ImpactFeedback->ResetImmediate(false);
	}
}

void UWacomBattleEnemySceneRuntimeComponent::ClearPartPresentation(
	UWacomBattleEnemyPartComponent& Part,
	FName Reason)
{
	if (FPartRuntimeState* State = Impl ? FindState(Impl->Parts, Part) : nullptr)
	{
		ResetFeedbackControllers(*State, false);
		State->HoverReason = Reason;
		RefreshOutlineFeedback(*this, *State);
	}
}

void UWacomBattleEnemySceneRuntimeComponent::SetPartDragTargetPreviewState(
	UWacomBattleEnemyPartComponent& Part,
	EWacomFirstPersonCardDragTargetFeedbackState PreviewState)
{
	FPartRuntimeState* State = Impl ? FindState(Impl->Parts, Part) : nullptr;
	if (!State)
	{
		return;
	}
	const bool bNewActive = PreviewState == EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget
		|| PreviewState == EWacomFirstPersonCardDragTargetFeedbackState::Invalid;
	if (State->DragPreviewState == PreviewState
		&& State->bDragPreviewActive == bNewActive)
	{
		return;
	}
	State->DragPreviewState = PreviewState;
	State->bDragPreviewActive = bNewActive;
	RefreshTargetPreview(*this, *State);
	RefreshOutlineFeedback(*this, *State);
}

void UWacomBattleEnemySceneRuntimeComponent::ClearPartDragTargetPreviewState(
	UWacomBattleEnemyPartComponent& Part)
{
	if (FPartRuntimeState* State = Impl ? FindState(Impl->Parts, Part) : nullptr)
	{
		if (!State->bDragPreviewActive
			&& State->DragPreviewState == EWacomFirstPersonCardDragTargetFeedbackState::None)
		{
			return;
		}
		State->bDragPreviewActive = false;
		State->DragPreviewState = EWacomFirstPersonCardDragTargetFeedbackState::None;
		RefreshTargetPreview(*this, *State);
		RefreshOutlineFeedback(*this, *State);
	}
}

void UWacomBattleEnemySceneRuntimeComponent::SetPartHoverProbeState(
	UWacomBattleEnemyPartComponent& Part,
	const FWacomInteractionTargetHandle& TargetHandle,
	FName Reason)
{
	if (!TargetHandle.HasBattlePartSlotIdentity())
	{
		ClearPartHoverProbeState(Part, TEXT("InvalidTarget"));
		return;
	}
	if (FPartRuntimeState* State = Impl ? FindState(Impl->Parts, Part) : nullptr)
	{
		const FName EffectiveReason = Reason.IsNone() ? FName(TEXT("Hovered")) : Reason;
		if (State->bHoverActive
			&& State->HoverReason == EffectiveReason)
		{
			return;
		}
		State->bHoverActive = true;
		State->HoverReason = EffectiveReason;
		RefreshOutlineFeedback(*this, *State);
	}
}

void UWacomBattleEnemySceneRuntimeComponent::ClearPartHoverProbeState(
	UWacomBattleEnemyPartComponent& Part,
	FName Reason)
{
	if (FPartRuntimeState* State = Impl ? FindState(Impl->Parts, Part) : nullptr)
	{
		if (!State->bHoverActive
			&& State->HoverReason == Reason)
		{
			return;
		}
		State->bHoverActive = false;
		State->HoverReason = Reason;
		RefreshOutlineFeedback(*this, *State);
	}
}

void UWacomBattleEnemySceneRuntimeComponent::ResetRuntimeScenePresentationForBattle()
{
	if (!Impl)
	{
		return;
	}
	Impl->bRuntimeRetired = false;
	CancelAllPlayback(true);
	for (FPartRuntimeState& State : Impl->Parts)
	{
		if (UWacomBattleEnemyPartComponent* Part = State.Part.Get())
		{
			RestorePartAuthoredVisualState(*Part);
			for (FFlipbookAuthoredState& Layer : State.Flipbooks)
			{
				if (auto* Component = Layer.Component.Get()) Component->SetVisibility(Layer.bVisible, true);
			}
			for (FSpriteAuthoredState& Layer : State.Sprites)
			{
				if (auto* Component = Layer.Component.Get()) Component->SetVisibility(Layer.bVisible, true);
			}
		}
		ResetFeedbackControllers(State, false);
		RefreshInteractionCollision(State, Impl->bRuntimeRetired);
		RefreshOutlineFeedback(*this, State);
	}
}

void UWacomBattleEnemySceneRuntimeComponent::RetireRuntimeEncounterPresentation()
{
	if (!Impl || Impl->bRuntimeRetired)
	{
		return;
	}
	Impl->bRuntimeRetired = true;
	CancelAllPlayback(false);
	for (FPartRuntimeState& State : Impl->Parts)
	{
		ResetFeedbackControllers(State, true);
		State.bBound = false;
		State.bRegisteredWithHUD = false;
		State.bTargetable = false;
		State.bTargetSelectionActive = false;
		if (UWacomBattleEnemyPartComponent* Part = State.Part.Get())
		{
			RefreshInteractionCollision(State, true);
			for (FFlipbookAuthoredState& Layer : State.Flipbooks)
			{
				if (auto* Component = Layer.Component.Get()) Component->SetVisibility(false, true);
			}
			for (FSpriteAuthoredState& Layer : State.Sprites)
			{
				if (auto* Component = Layer.Component.Get()) Component->SetVisibility(false, true);
			}
		}
	}
	SetComponentTickEnabled(false);
}

bool UWacomBattleEnemySceneRuntimeComponent::IsRuntimeRetired() const
{
	return Impl && Impl->bRuntimeRetired;
}

void UWacomBattleEnemySceneRuntimeComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	bool bNeedsTick = false;
	if (!Impl)
	{
		SetComponentTickEnabled(false);
		return;
	}
	for (FPartRuntimeState& State : Impl->Parts)
	{
		UWacomBattleEnemyPartComponent* Part = State.Part.Get();
		if (!Part)
		{
			continue;
		}
		if (State.CuePlayback && State.CuePlayback->GetView().bActive)
		{
			const FWacomBattleEnemyPartCuePlaybackView View = State.CuePlayback->Tick(DeltaTime);
			if (View.Kind == EWacomBattleEnemyPartCuePlaybackKind::Destroyed
				&& State.bDestroyedSwapPending
				&& View.Progress + UE_KINDA_SMALL_NUMBER >= FMath::Clamp(
					Part->DestroyedVisualSwapNormalizedTime,
					0.0f,
					1.0f))
			{
				ApplyPartDestroyedVisualState(*Part);
			}
			if (!View.bActive && State.ImpactFeedback)
			{
				State.ImpactFeedback->FinishNaturally();
			}
			bNeedsTick |= View.bActive;
		}
		if (State.TargetPreviewPlayback && State.TargetPreviewPlayback->GetView().bActive)
		{
			const FWacomBattleEnemyPartTargetPreviewPlaybackView View =
				State.TargetPreviewPlayback->Tick(DeltaTime);
			if (View.bActive && Part->bEnableTargetPreviewFeedback && Part->ResolveTargetPreviewStyle())
			{
				State.TargetPreviewFeedback->BeginOrUpdate(
					*this,
					Part,
					ResolvePresentationBounds(State),
					Part->ResolveTargetPreviewStyle(),
					View,
					State.TargetPreviewFlashScale);
			}
			else if (!View.bActive)
			{
				State.TargetPreviewFeedback->FinishNaturally();
			}
			bNeedsTick |= View.bActive;
		}
		if (State.OutlineFeedback)
		{
			bNeedsTick |= State.OutlineFeedback->Tick();
		}
	}
	SetComponentTickEnabled(bNeedsTick);
}

void UWacomBattleEnemySceneRuntimeComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	CancelAllPlayback(false);
	if (Impl)
	{
		if (UWacomBattleEnemyPartFlipbookLayerComponent* Layer = Impl->ActivePlaybackLayer.Get())
		{
			Layer->OnFinishedPlaying.RemoveDynamic(this, &UWacomBattleEnemySceneRuntimeComponent::HandleActiveFlipbookFinished);
		}
		for (FPartRuntimeState& State : Impl->Parts)
		{
			ClearInteractionCollisionConfiguration(State);
			DestroyFallbackCollision(State);
			ResetFeedbackControllers(State, true);
		}
		Impl->Parts.Reset();
		Impl->ActivePlaybackLayer.Reset();
		Impl->ActivePlaybackPart.Reset();
	}
	SetComponentTickEnabled(false);
	Super::EndPlay(EndPlayReason);
}
