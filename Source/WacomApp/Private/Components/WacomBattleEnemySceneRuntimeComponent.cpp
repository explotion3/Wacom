// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattleEnemySceneRuntimeComponent.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartAnimationStyle.h"
#include "Actors/WacomBattleEnemyPartImpactStyle.h"
#include "Actors/WacomBattleEnemyPartTargetPreviewStyle.h"
#include "Blueprint/UserWidget.h"
#include "Components/WacomBattleEnemyActionPlayback.h"
#include "Components/WacomBattleEnemyPartComponent.h"
#include "Components/WacomBattleEnemyPartCuePlayback.h"
#include "Components/WacomBattleEnemyPartFlipbookLayerComponent.h"
#include "Components/WacomBattleEnemyPartImpactAnchorComponent.h"
#include "Components/WacomBattleEnemyPartImpactFeedbackController.h"
#include "Components/WacomBattleEnemyPartSpriteLayerComponent.h"
#include "Components/WacomBattleEnemyPartTargetPreviewFeedbackController.h"
#include "Components/WacomBattleEnemyPartTargetPreviewPlayback.h"
#include "Components/WidgetComponent.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "PaperFlipbook.h"
#include "PaperSprite.h"
#include "Settings/WacomPresentationAccessibilityPolicy.h"
#include "Settings/WacomSettingsSubsystem.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "UI/Battle/WacomBattleEnemyPanelViewData.h"
#include "UI/Battle/WacomBattleEnemyPartPredictionWidget.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"
#include "UObject/StrongObjectPtr.h"

#define LOCTEXT_NAMESPACE "WacomBattleEnemySceneRuntime"

namespace
{
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
		TWeakObjectPtr<UWidgetComponent> PredictionWidget;
		TSubclassOf<UUserWidget> PredictionWidgetClass;
		FVector AppliedPredictionRelativeLocation = FVector::ZeroVector;
		FIntPoint AppliedPredictionDrawSize = FIntPoint::ZeroValue;
		float AppliedPredictionBadgeScale = 1.0f;
		float AppliedPredictionVisibleZOffset = 0.0f;
		FWacomBattleEnemyPartPredictionView AppliedPredictionView;
		bool bHasAppliedPredictionView = false;
		int32 PredictionWidgetCreateCount = 0;
		int32 PredictionWidgetApplyCount = 0;

		FName EncounterId = NAME_None;
		FName EnemySlotId = NAME_None;
		FName PartSlotId = NAME_None;
		FName PartId = NAME_None;
		FGuid PartInstanceId;
		bool bBound = false;
		bool bRegisteredWithHUD = false;
		bool bTargetable = false;
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
		bool bActionPreviewActive = false;
		EWacomFirstPersonCardDragTargetFeedbackState DragPreviewState =
			EWacomFirstPersonCardDragTargetFeedbackState::None;
		FWacomBattleEnemyPartDragPredictionDebugInput DragPredictionInput;
		FWacomBattleEnemyPartDragPredictionDebugInput HoverPredictionInput;
		FWacomBattleEnemyPartEntryViewData ActionPreviewView;
		FWacomBattleEnemyPartPredictionView PredictionView;
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

	void DestroyPredictionWidget(FPartRuntimeState& State)
	{
		if (UWidgetComponent* Widget = State.PredictionWidget.Get())
		{
			Widget->SetVisibility(false, true);
			Widget->DestroyComponent();
		}
		State.PredictionWidget.Reset();
		State.PredictionWidgetClass = nullptr;
		State.bHasAppliedPredictionView = false;
	}

	bool PredictionInputsEqual(
		const FWacomBattleEnemyPartDragPredictionDebugInput& A,
		const FWacomBattleEnemyPartDragPredictionDebugInput& B)
	{
		return A.bHasSourceCard == B.bHasSourceCard
			&& A.SourceCardInstanceId == B.SourceCardInstanceId
			&& A.SourceCardRuntimeCost == B.SourceCardRuntimeCost
			&& A.bSourceCardSwift == B.bSourceCardSwift
			&& A.bPreviewCanSubmit == B.bPreviewCanSubmit
			&& A.PreviewRejectReason == B.PreviewRejectReason;
	}

	bool PredictionViewsEqual(
		const FWacomBattleEnemyPartPredictionView& A,
		const FWacomBattleEnemyPartPredictionView& B)
	{
		return A.bVisible == B.bVisible
			&& A.Mode == B.Mode
			&& A.CurrentInitiative == B.CurrentInitiative
			&& A.PredictedInitiative == B.PredictedInitiative
			&& A.bHasSourceCard == B.bHasSourceCard
			&& A.SourceCardRuntimeCost == B.SourceCardRuntimeCost
			&& A.bSourceCardSwift == B.bSourceCardSwift
			&& A.bPerfectReleaseCandidate == B.bPerfectReleaseCandidate
			&& A.bActionRisk == B.bActionRisk
			&& A.RejectReason == B.RejectReason
			&& A.MainText.EqualTo(B.MainText)
			&& A.DetailText.EqualTo(B.DetailText);
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
		State.bDestroyedSwapPending = false;
		State.bAwaitingDestroyedCue = false;
		State.bDragPreviewActive = false;
		State.bHoverActive = false;
		State.bActionPreviewActive = false;
		State.DragPreviewState = EWacomFirstPersonCardDragTargetFeedbackState::None;
		State.DragPredictionInput = FWacomBattleEnemyPartDragPredictionDebugInput();
		State.HoverPredictionInput = FWacomBattleEnemyPartDragPredictionDebugInput();
		State.ActionPreviewView = FWacomBattleEnemyPartEntryViewData();
		State.PredictionView = FWacomBattleEnemyPartPredictionView();
	}

	void ApplyVisualScale(FPartRuntimeState& State, float Scale)
	{
		const float SafeScale = FMath::Max(1.0f, Scale);
		for (FFlipbookAuthoredState& Layer : State.Flipbooks)
		{
			if (UWacomBattleEnemyPartFlipbookLayerComponent* Component = Layer.Component.Get())
			{
				Component->SetRelativeScale3D(Layer.RelativeScale * SafeScale);
			}
		}
		for (FSpriteAuthoredState& Layer : State.Sprites)
		{
			if (UWacomBattleEnemyPartSpriteLayerComponent* Component = Layer.Component.Get())
			{
				Component->SetRelativeScale3D(Layer.RelativeScale * SafeScale);
			}
		}
	}

	void RefreshPersistentScale(FPartRuntimeState& State)
	{
		UWacomBattleEnemyPartComponent* Part = State.Part.Get();
		if (!Part || State.bDragPreviewActive)
		{
			ApplyVisualScale(State, 1.0f);
			return;
		}
		if (State.bTargetable)
		{
			ApplyVisualScale(State, Part->TargetableAffordanceScale);
		}
		else if (State.bHoverActive)
		{
			ApplyVisualScale(State, Part->HoverProbeScale);
		}
		else
		{
			ApplyVisualScale(State, 1.0f);
		}
	}

	FWacomBattleEnemyPartPredictionView BuildPredictionView(
		const FPartRuntimeState& State,
		const FWacomBattleEnemyPartDragPredictionDebugInput& Input)
	{
		FWacomBattleEnemyPartPredictionView View;
		View.bVisible = true;
		View.CurrentInitiative = State.CurrentInitiative;
		View.PredictedInitiative = State.CurrentInitiative;
		View.bHasSourceCard = Input.bHasSourceCard;
		View.SourceCardRuntimeCost = Input.SourceCardRuntimeCost;
		View.bSourceCardSwift = Input.bSourceCardSwift;
		if (!Input.bHasSourceCard)
		{
			View.Mode = EWacomBattleEnemyPartPredictionMode::HoverInitiative;
			View.MainText = FText::Format(
				LOCTEXT("HoverInitiative", "先机 {0}"),
				FText::AsNumber(State.CurrentInitiative));
			return View;
		}
		if (!Input.bPreviewCanSubmit)
		{
			View.Mode = EWacomBattleEnemyPartPredictionMode::Rejected;
			View.RejectReason = Input.PreviewRejectReason.IsNone()
				? FName(TEXT("Rejected"))
				: Input.PreviewRejectReason;
			View.MainText = LOCTEXT("Rejected", "不可释放");
			return View;
		}
		View.Mode = EWacomBattleEnemyPartPredictionMode::CardPrediction;
		if (!Input.bSourceCardSwift)
		{
			View.PredictedInitiative = State.CurrentInitiative - Input.SourceCardRuntimeCost;
			View.bPerfectReleaseCandidate = Input.SourceCardRuntimeCost == State.CurrentInitiative;
			View.bActionRisk = View.PredictedInitiative <= 0;
			View.MainText = FText::Format(
				LOCTEXT("Prediction", "先机 {0} -> {1}"),
				FText::AsNumber(State.CurrentInitiative),
				FText::AsNumber(View.PredictedInitiative));
		}
		else
		{
			View.MainText = FText::Format(
				LOCTEXT("SwiftPrediction", "先机 {0}"),
				FText::AsNumber(State.CurrentInitiative));
			View.DetailText = LOCTEXT("Swift", "迅捷：不推进");
		}
		if (View.bPerfectReleaseCandidate)
		{
			View.DetailText = LOCTEXT("PerfectRelease", "完美释放");
		}
		else if (View.bActionRisk)
		{
			View.DetailText = LOCTEXT("ActionRisk", "行动风险");
		}
		return View;
	}

	UWidgetComponent* EnsurePredictionWidget(
		UWacomBattleEnemySceneRuntimeComponent& Owner,
		FPartRuntimeState& State)
	{
		UWacomBattleEnemyPartComponent* Part = State.Part.Get();
		AWacomBattleEnemyActor* Host = Cast<AWacomBattleEnemyActor>(Owner.GetOwner());
		if (!Part || !Host || !Owner.GetWorld())
		{
			return nullptr;
		}
		const TSubclassOf<UUserWidget> DesiredClass = Part->PredictionWidgetClass
			? TSubclassOf<UUserWidget>(Part->PredictionWidgetClass)
			: TSubclassOf<UUserWidget>(UWacomBattleEnemyPartPredictionWidget::StaticClass());
		if (UWidgetComponent* Existing = State.PredictionWidget.Get())
		{
			if (State.PredictionWidgetClass == DesiredClass)
			{
				return Existing;
			}
			DestroyPredictionWidget(State);
		}

		UWidgetComponent* Widget = NewObject<UWidgetComponent>(
			Host,
			MakeUniqueObjectName(Host, UWidgetComponent::StaticClass(), TEXT("EnemyPartPrediction")),
			RF_Transient);
		if (!Widget)
		{
			return nullptr;
		}
		Host->AddInstanceComponent(Widget);
		Widget->CreationMethod = EComponentCreationMethod::Instance;
		Widget->SetupAttachment(Part);
		Widget->SetWidgetSpace(EWidgetSpace::Screen);
		Widget->SetWidgetClass(DesiredClass);
		Widget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Widget->SetGenerateOverlapEvents(false);
		Widget->SetPivot(FVector2D(0.5f, 0.5f));
		Widget->SetVisibility(false, true);
		Widget->RegisterComponent();
		State.PredictionWidget = Widget;
		State.PredictionWidgetClass = DesiredClass;
		State.bHasAppliedPredictionView = false;
		++State.PredictionWidgetCreateCount;
		return Widget;
	}

	void ApplyPredictionWidget(
		UWacomBattleEnemySceneRuntimeComponent& Owner,
		FPartRuntimeState& State)
	{
		UWacomBattleEnemyPartComponent* Part = State.Part.Get();
		if (!Part)
		{
			return;
		}
		UWidgetComponent* Widget = State.PredictionWidget.Get();
		if (!State.PredictionView.bVisible && !Widget)
		{
			return;
		}
		if (State.PredictionView.bVisible)
		{
			Widget = EnsurePredictionWidget(Owner, State);
		}
		if (!Widget)
		{
			return;
		}

		const FIntPoint DrawSize(
			FMath::Max(1, FMath::RoundToInt(Part->PredictionDrawSize.X)),
			FMath::Max(1, FMath::RoundToInt(Part->PredictionDrawSize.Y)));
		const FVector Location = Part->PredictionRelativeLocation + FVector(
			0.0f,
			0.0f,
			State.PredictionView.bVisible
				? FMath::Max(0.0f, Part->PredictionBadgeZOffsetWhenVisible)
				: 0.0f);
		const bool bConfigMatches = State.AppliedPredictionRelativeLocation == Location
			&& State.AppliedPredictionDrawSize == DrawSize
			&& FMath::IsNearlyEqual(State.AppliedPredictionBadgeScale, Part->PredictionBadgeScale)
			&& FMath::IsNearlyEqual(
				State.AppliedPredictionVisibleZOffset,
				Part->PredictionBadgeZOffsetWhenVisible);
		if (State.bHasAppliedPredictionView
			&& bConfigMatches
			&& PredictionViewsEqual(State.AppliedPredictionView, State.PredictionView))
		{
			return;
		}

		Widget->SetVisibility(State.PredictionView.bVisible, true);
		Widget->SetRelativeLocation(Location);
		Widget->SetDrawSize(DrawSize);
		if (!Widget->GetUserWidgetObject())
		{
			Widget->InitWidget();
		}
		if (UUserWidget* UserWidget = Widget->GetUserWidgetObject())
		{
			UserWidget->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			UserWidget->SetRenderScale(FVector2D(Part->PredictionBadgeScale));
			if (UWacomBattleEnemyPartPredictionWidget* Prediction =
				Cast<UWacomBattleEnemyPartPredictionWidget>(UserWidget))
			{
				Prediction->SetPredictionView(State.PredictionView);
			}
		}
		State.AppliedPredictionRelativeLocation = Location;
		State.AppliedPredictionDrawSize = DrawSize;
		State.AppliedPredictionBadgeScale = Part->PredictionBadgeScale;
		State.AppliedPredictionVisibleZOffset = Part->PredictionBadgeZOffsetWhenVisible;
		State.AppliedPredictionView = State.PredictionView;
		State.bHasAppliedPredictionView = true;
		++State.PredictionWidgetApplyCount;
	}

	void RefreshPrediction(
		UWacomBattleEnemySceneRuntimeComponent& Owner,
		FPartRuntimeState& State)
	{
		UWacomBattleEnemyPartComponent* Part = State.Part.Get();
		if (!Part || !Part->bEnablePredictionWidget || !State.PartInstanceId.IsValid() || State.bDestroyed)
		{
			State.PredictionView = FWacomBattleEnemyPartPredictionView();
			ApplyPredictionWidget(Owner, State);
			return;
		}
		if (State.bActionPreviewActive)
		{
			State.PredictionView.bVisible = true;
			State.PredictionView.Mode = EWacomBattleEnemyPartPredictionMode::CardPrediction;
			State.PredictionView.CurrentInitiative = State.CurrentInitiative;
			State.PredictionView.PredictedInitiative = State.ActionPreviewView.CurrentInitiative;
			State.PredictionView.bActionRisk = State.ActionPreviewView.bActionPreviewWillAct;
			State.PredictionView.MainText = FText::Format(
				LOCTEXT("ActionPreview", "先机 {0} -> {1}"),
				FText::AsNumber(State.CurrentInitiative),
				FText::AsNumber(State.ActionPreviewView.CurrentInitiative));
		}
		else if (State.bDragPreviewActive)
		{
			State.PredictionView = BuildPredictionView(State, State.DragPredictionInput);
		}
		else if (State.bHoverActive)
		{
			State.PredictionView = BuildPredictionView(State, State.HoverPredictionInput);
		}
		else
		{
			State.PredictionView = FWacomBattleEnemyPartPredictionView();
		}
		ApplyPredictionWidget(Owner, State);
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
		return !IsValid(Part) || Part->GetOwner() == nullptr;
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
				return IsValid(Layer) && Layer->GetAttachParent() == Part;
			});
		TArray<UWacomBattleEnemyPartSpriteLayerComponent*> DirectSprites =
			AllSprites.FilterByPredicate([Part](const UWacomBattleEnemyPartSpriteLayerComponent* Layer)
			{
				return IsValid(Layer) && Layer->GetAttachParent() == Part;
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
		Impl->Parts.Add(MoveTemp(State));
	}

	for (FPartRuntimeState& Removed : Previous)
	{
		if (Removed.ActionPlayback)
		{
			Removed.ActionPlayback->Cancel(false);
		}
		ResetFeedbackControllers(Removed, true);
		DestroyPredictionWidget(Removed);
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
		const bool bTargetableChanged = State->bTargetable || !State->TargetDisabledReason.IsNone();
		State->bBound = false;
		State->PartInstanceId.Invalidate();
		State->CurrentInitiative = 0;
		State->CurrentIntentId = NAME_None;
		State->bDestroyed = false;
		State->bAwaitingDestroyedCue = false;
		State->bTargetable = false;
		State->TargetDisabledReason = NAME_None;
		if (bTargetableChanged)
		{
			++State->TargetableApplyCount;
			RefreshPersistentScale(*State);
		}
		if (bFactsChanged)
		{
			++State->SnapshotApplyCount;
			RefreshPrediction(*this, *State);
		}
		else
		{
			++State->SnapshotNoOpCount;
		}
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
	const bool bTargetableChanged = State->bTargetable != bEffectiveTargetable
		|| State->TargetDisabledReason != EffectiveReason;
	State->bTargetable = bEffectiveTargetable;
	State->TargetDisabledReason = EffectiveReason;
	if (bTargetableChanged)
	{
		++State->TargetableApplyCount;
		RefreshPersistentScale(*State);
	}
	if (bFactsChanged)
	{
		++State->SnapshotApplyCount;
		RefreshPrediction(*this, *State);
	}
	else
	{
		++State->SnapshotNoOpCount;
	}
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
	State->TargetDisabledReason = NAME_None;
	if (bClearRuntimeFacts)
	{
		State->PartInstanceId.Invalidate();
		State->CurrentInitiative = 0;
		State->CurrentIntentId = NAME_None;
		State->bDestroyed = false;
	}
	ResetFeedbackControllers(*State, false);
	RefreshPersistentScale(*State);
	RefreshPrediction(*this, *State);
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
		RefreshPersistentScale(*State);
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
	View.bPredictionWidgetCreated = State->PredictionWidget.IsValid();
	View.bPredictionWidgetVisible = State->PredictionWidget.IsValid()
		&& State->PredictionWidget->IsVisible();
	View.PredictionWidgetCreateCount = State->PredictionWidgetCreateCount;
	View.PredictionWidgetApplyCount = State->PredictionWidgetApplyCount;
	return View;
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
			&Part,
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
		RefreshPersistentScale(*State);
		RefreshPrediction(*this, *State);
	}
}

void UWacomBattleEnemySceneRuntimeComponent::SetPartDragTargetPreviewState(
	UWacomBattleEnemyPartComponent& Part,
	EWacomFirstPersonCardDragTargetFeedbackState PreviewState,
	const FWacomBattleEnemyPartDragPredictionDebugInput& PredictionInput)
{
	FPartRuntimeState* State = Impl ? FindState(Impl->Parts, Part) : nullptr;
	if (!State)
	{
		return;
	}
	const bool bNewActive = PreviewState == EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget
		|| PreviewState == EWacomFirstPersonCardDragTargetFeedbackState::Invalid;
	if (State->DragPreviewState == PreviewState
		&& State->bDragPreviewActive == bNewActive
		&& PredictionInputsEqual(State->DragPredictionInput, PredictionInput))
	{
		return;
	}
	State->DragPreviewState = PreviewState;
	State->DragPredictionInput = PredictionInput;
	State->bDragPreviewActive = bNewActive;
	RefreshPersistentScale(*State);
	RefreshPrediction(*this, *State);
	if (!State->bDragPreviewActive)
	{
		if (State->TargetPreviewPlayback) State->TargetPreviewPlayback->BeginExit();
		SetComponentTickEnabled(true);
		return;
	}
	const UWacomBattleEnemyPartTargetPreviewStyle* Style = Part.ResolveTargetPreviewStyle();
	if (!Part.bEnableTargetPreviewFeedback || !Style || !Style->HasValidVisualAssets())
	{
		return;
	}
	float FlashScale = 1.0f;
	bool bSimplifiedMotion = false;
	RefreshAccessibility(*this, FlashScale, bSimplifiedMotion);
	State->TargetPreviewFlashScale = FlashScale;
	const EWacomBattleEnemyPartTargetPreviewKind Kind =
		PreviewState == EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget
			? EWacomBattleEnemyPartTargetPreviewKind::Valid
			: EWacomBattleEnemyPartTargetPreviewKind::Invalid;
	State->TargetPreviewPlayback->Begin(
		Kind,
		Style->EnterSeconds,
		Style->ExitSeconds,
		Style->PulsePeriodSeconds,
		bSimplifiedMotion);
	State->TargetPreviewFeedback->BeginOrUpdate(
		*this,
		ResolveImpactAnchor(*State),
		&Part,
		Style,
		State->TargetPreviewPlayback->GetView(),
		FlashScale);
	SetComponentTickEnabled(true);
}

void UWacomBattleEnemySceneRuntimeComponent::ClearPartDragTargetPreviewState(
	UWacomBattleEnemyPartComponent& Part)
{
	if (FPartRuntimeState* State = Impl ? FindState(Impl->Parts, Part) : nullptr)
	{
		if (!State->bDragPreviewActive
			&& State->DragPreviewState == EWacomFirstPersonCardDragTargetFeedbackState::None
			&& PredictionInputsEqual(
				State->DragPredictionInput,
				FWacomBattleEnemyPartDragPredictionDebugInput()))
		{
			return;
		}
		State->bDragPreviewActive = false;
		State->DragPreviewState = EWacomFirstPersonCardDragTargetFeedbackState::None;
		State->DragPredictionInput = FWacomBattleEnemyPartDragPredictionDebugInput();
		RefreshPersistentScale(*State);
		RefreshPrediction(*this, *State);
		if (State->TargetPreviewPlayback) State->TargetPreviewPlayback->BeginExit();
		SetComponentTickEnabled(true);
	}
}

void UWacomBattleEnemySceneRuntimeComponent::SetPartHoverProbeState(
	UWacomBattleEnemyPartComponent& Part,
	const FWacomInteractionTargetHandle& TargetHandle,
	FName Reason,
	const FWacomBattleEnemyPartDragPredictionDebugInput& PredictionInput)
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
			&& State->HoverReason == EffectiveReason
			&& PredictionInputsEqual(State->HoverPredictionInput, PredictionInput))
		{
			return;
		}
		State->bHoverActive = true;
		State->HoverReason = EffectiveReason;
		State->HoverPredictionInput = PredictionInput;
		RefreshPersistentScale(*State);
		RefreshPrediction(*this, *State);
	}
}

void UWacomBattleEnemySceneRuntimeComponent::ClearPartHoverProbeState(
	UWacomBattleEnemyPartComponent& Part,
	FName Reason)
{
	if (FPartRuntimeState* State = Impl ? FindState(Impl->Parts, Part) : nullptr)
	{
		if (!State->bHoverActive
			&& State->HoverReason == Reason
			&& PredictionInputsEqual(
				State->HoverPredictionInput,
				FWacomBattleEnemyPartDragPredictionDebugInput()))
		{
			return;
		}
		State->bHoverActive = false;
		State->HoverReason = Reason;
		State->HoverPredictionInput = FWacomBattleEnemyPartDragPredictionDebugInput();
		RefreshPersistentScale(*State);
		RefreshPrediction(*this, *State);
	}
}

void UWacomBattleEnemySceneRuntimeComponent::SetPartActionPreview(
	UWacomBattleEnemyPartComponent& Part,
	const FWacomBattleEnemyPartEntryViewData& PreviewView)
{
	if (FPartRuntimeState* State = Impl ? FindState(Impl->Parts, Part) : nullptr)
	{
		if (State->bActionPreviewActive
			&& State->ActionPreviewView.CurrentInitiative == PreviewView.CurrentInitiative
			&& State->ActionPreviewView.bActionPreviewWillAct == PreviewView.bActionPreviewWillAct)
		{
			return;
		}
		State->ActionPreviewView = PreviewView;
		State->bActionPreviewActive = true;
		RefreshPrediction(*this, *State);
	}
}

void UWacomBattleEnemySceneRuntimeComponent::ClearPartActionPreview(
	UWacomBattleEnemyPartComponent& Part)
{
	if (FPartRuntimeState* State = Impl ? FindState(Impl->Parts, Part) : nullptr)
	{
		if (!State->bActionPreviewActive)
		{
			return;
		}
		State->bActionPreviewActive = false;
		State->ActionPreviewView = FWacomBattleEnemyPartEntryViewData();
		RefreshPrediction(*this, *State);
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
			Part->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
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
		RefreshPrediction(*this, State);
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
		DestroyPredictionWidget(State);
		State.bBound = false;
		State.bRegisteredWithHUD = false;
		State.bTargetable = false;
		if (UWacomBattleEnemyPartComponent* Part = State.Part.Get())
		{
			Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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
					ResolveImpactAnchor(State),
					Part,
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
			ResetFeedbackControllers(State, true);
			DestroyPredictionWidget(State);
		}
		Impl->Parts.Reset();
		Impl->ActivePlaybackLayer.Reset();
		Impl->ActivePlaybackPart.Reset();
	}
	SetComponentTickEnabled(false);
	Super::EndPlay(EndPlayReason);
}

#undef LOCTEXT_NAMESPACE
