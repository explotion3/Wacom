// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattleEnemyPartPresentationComponent.h"

#include "Actors/WacomBattleEnemyPartImpactStyle.h"
#include "Actors/WacomBattleEnemyPartTargetPreviewStyle.h"
#include "Components/WacomBattleEnemyPartCuePlayback.h"
#include "Components/WacomBattleEnemyPartImpactFeedbackController.h"
#include "Components/WacomBattleEnemyPartTargetPreviewFeedbackController.h"
#include "Components/WacomBattleEnemyPartTargetPreviewPlayback.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Enemies/EnemyPartDefinition.h"
#include "GameFramework/Actor.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Snapshots/EnemySnapshot.h"
#include "Settings/WacomPresentationAccessibilityPolicy.h"
#include "Settings/WacomSettingsSubsystem.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "UI/Battle/WacomBattleEnemyPartPredictionWidget.h"
#include "UI/Battle/WacomBattleEventPresentationBuilder.h"

#define LOCTEXT_NAMESPACE "WacomBattleEnemyPartPresentation"

namespace
{
	bool AreStatusStacksEquivalent(
		const TMap<FGameplayTag, int32>& Left,
		const TMap<FGameplayTag, int32>& Right)
	{
		if (Left.Num() != Right.Num())
		{
			return false;
		}

		for (const TPair<FGameplayTag, int32>& Pair : Left)
		{
			const int32* RightValue = Right.Find(Pair.Key);
			if (!RightValue || *RightValue != Pair.Value)
			{
				return false;
			}
		}
		return true;
	}

	bool AreActionPreviewPartViewsEquivalent(
		const FWacomBattleEnemyPartEntryViewData& Left,
		const FWacomBattleEnemyPartEntryViewData& Right)
	{
		return Left.PartInstanceId == Right.PartInstanceId
			&& Left.Identity == Right.Identity
			&& Left.EnemySlotId == Right.EnemySlotId
			&& Left.PartSlotId == Right.PartSlotId
			&& Left.CurrentHp == Right.CurrentHp
			&& Left.MaxHp == Right.MaxHp
			&& Left.Shield == Right.Shield
			&& Left.CurrentInitiative == Right.CurrentInitiative
			&& Left.RuntimeStatuses.Num() == Right.RuntimeStatuses.Num()
			&& Left.RuntimeStatuses.HasAllExact(Right.RuntimeStatuses)
			&& Right.RuntimeStatuses.HasAllExact(Left.RuntimeStatuses)
			&& AreStatusStacksEquivalent(Left.RuntimeStatusStacks, Right.RuntimeStatusStacks)
			&& Left.bDestroyed == Right.bDestroyed
			&& Left.bActionPreviewWillAct == Right.bActionPreviewWillAct;
	}
}

void UWacomBattleEnemyPartPresentationComponent::FCuePlaybackDeleter::operator()(
	FWacomBattleEnemyPartCuePlayback* Playback) const
{
	delete Playback;
}

void UWacomBattleEnemyPartPresentationComponent::FImpactFeedbackControllerDeleter::operator()(
	FWacomBattleEnemyPartImpactFeedbackController* Controller) const
{
	delete Controller;
}

void UWacomBattleEnemyPartPresentationComponent::FTargetPreviewPlaybackDeleter::operator()(
	FWacomBattleEnemyPartTargetPreviewPlayback* Playback) const
{
	delete Playback;
}

void UWacomBattleEnemyPartPresentationComponent::FTargetPreviewFeedbackControllerDeleter::operator()(
	FWacomBattleEnemyPartTargetPreviewFeedbackController* Controller) const
{
	delete Controller;
}

UWacomBattleEnemyPartPresentationComponent::UWacomBattleEnemyPartPresentationComponent()
	: CuePlayback(new FWacomBattleEnemyPartCuePlayback())
	, ImpactFeedbackController(new FWacomBattleEnemyPartImpactFeedbackController())
	, TargetPreviewPlayback(new FWacomBattleEnemyPartTargetPreviewPlayback())
	, TargetPreviewFeedbackController(new FWacomBattleEnemyPartTargetPreviewFeedbackController())
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetComponentTickEnabled(false);
}

UWacomBattleEnemyPartPresentationComponent::~UWacomBattleEnemyPartPresentationComponent() = default;

void UWacomBattleEnemyPartPresentationComponent::BeginPlay()
{
	Super::BeginPlay();
	BindRuntimeSettings();
}

void UWacomBattleEnemyPartPresentationComponent::CacheRuntimePartFacts(
	FName InPartId,
	const FEnemyPartSnapshot& Part)
{
	PartId = InPartId;
	RuntimePartInstanceId = Part.InstanceId;
	bHasRuntimePartFacts = Part.InstanceId.IsValid();
	CurrentInitiative = Part.CurrentInitiative;
	bRuntimePartDestroyed = Part.bDestroyed;
	CurrentIntentId = Part.CurrentIntent.IntentId;
	RefreshPredictionDisplay();
}

void UWacomBattleEnemyPartPresentationComponent::ClearRuntimePartFacts()
{
	ResetBattlePresentationFeedback();
	RuntimePartInstanceId.Invalidate();
	bHasRuntimePartFacts = false;
	CurrentInitiative = 0;
	bRuntimePartDestroyed = false;
	CurrentIntentId = NAME_None;
	RefreshPredictionDisplay();
}

void UWacomBattleEnemyPartPresentationComponent::PlayBattlePresentationCue(
	const FWacomBattlePresentationTargetCue& Cue)
{
	if (!CuePlayback || !CuePlayback->Begin(Cue, CueHoldSeconds))
	{
		return;
	}

	LastCueKind = FWacomBattleEnemyPartCuePlayback::KindToName(CuePlayback->GetView().Kind);
	LastCueType = Cue.SourceEventType;
	LastCueAmount = Cue.Amount;
	++CuePlayCount;

	const FWacomBattleEnemyPartCuePlaybackView& CueView = CuePlayback->GetView();
	if (ImpactFeedbackController)
	{
		if (CueView.Kind == EWacomBattleEnemyPartCuePlaybackKind::Destroyed)
		{
			// Destroyed owns the semantic priority but has no authored visual this round.
			ImpactFeedbackController->ResetImmediate(false);
		}
		else if (bImpactFeedbackEnabled && IsValid(ImpactFeedbackStyle))
		{
			FWacomBattlePresentationTargetCue EffectiveCue = Cue;
			EffectiveCue.Duration = CueView.DurationSeconds;
			ImpactFeedbackController->PlayAcceptedCue(
				*this,
				ResolveImpactAnchorComponent(),
				ResolveImpactExtentSourceComponent(),
				ImpactFeedbackStyle,
				CueView.Kind,
				EffectiveCue,
				RuntimeDecorativeFlashIntensityScale,
				bRuntimeSimplifiedMotion);
		}
	}
	RefreshComponentTickEnabled();
}

void UWacomBattleEnemyPartPresentationComponent::ForceCompleteBattlePresentationCue()
{
	if (CuePlayback)
	{
		CuePlayback->ForceComplete();
	}
	if (ImpactFeedbackController)
	{
		ImpactFeedbackController->ResetImmediate(false);
	}
	RefreshComponentTickEnabled();
}

void UWacomBattleEnemyPartPresentationComponent::ResetBattlePresentationFeedback()
{
	ResetCuePlayback();
	bTargetable = false;
	bDragPreviewActive = false;
	bHoverProbeActive = false;
	bActionPreviewPartActive = false;
	DragPreviewState = EWacomFirstPersonCardDragTargetFeedbackState::None;
	LastDragPredictionDebugInput = FWacomBattleEnemyPartDragPredictionDebugInput();
	LastHoverPredictionInput = FWacomBattleEnemyPartDragPredictionDebugInput();
	ActionPreviewPartView = FWacomBattleEnemyPartEntryViewData();
	HoverReason = NAME_None;
	HoverStableId = NAME_None;
	HoverWorldTargetId.Invalidate();
	HoverScreenPosition = FVector2D::ZeroVector;
	ClearPredictionDisplay(TEXT("PresentationReset"));
	ResetTargetPreviewPlayback(false);
	RestoreBaseScaleIfNeeded();
}

void UWacomBattleEnemyPartPresentationComponent::SetPresentationTargets(
	USceneComponent* InFeedbackTarget,
	USceneComponent* InImpactAnchor,
	UPrimitiveComponent* InImpactExtentSource)
{
	if (FeedbackTargetComponent != InFeedbackTarget)
	{
		RestoreBaseScaleIfNeeded();
		FeedbackTargetComponent = InFeedbackTarget;
		ApplyPersistentScaleState();
	}
	const bool bTargetsChanged = ImpactAnchorComponent != InImpactAnchor
		|| ImpactExtentSourceComponent != InImpactExtentSource;
	if (bTargetsChanged && ImpactFeedbackController)
	{
		ImpactFeedbackController->ResetImmediate(true);
	}
	if (bTargetsChanged && TargetPreviewFeedbackController)
	{
		TargetPreviewFeedbackController->ResetImmediate(true);
	}
	ImpactAnchorComponent = InImpactAnchor;
	ImpactExtentSourceComponent = InImpactExtentSource;
}

void UWacomBattleEnemyPartPresentationComponent::SetTargetPreviewFeedbackStyle(
	UWacomBattleEnemyPartTargetPreviewStyle* InTargetPreviewStyle,
	bool bInTargetPreviewFeedbackEnabled)
{
	if (TargetPreviewFeedbackStyle == InTargetPreviewStyle
		&& bTargetPreviewFeedbackEnabled == bInTargetPreviewFeedbackEnabled)
	{
		return;
	}

	ResetTargetPreviewPlayback(true);
	TargetPreviewFeedbackStyle = InTargetPreviewStyle;
	bTargetPreviewFeedbackEnabled = bInTargetPreviewFeedbackEnabled;
}

void UWacomBattleEnemyPartPresentationComponent::SetImpactFeedbackStyle(
	UWacomBattleEnemyPartImpactStyle* InImpactStyle,
	bool bInImpactFeedbackEnabled)
{
	if (ImpactFeedbackStyle != InImpactStyle
		|| bImpactFeedbackEnabled != bInImpactFeedbackEnabled)
	{
		if (ImpactFeedbackController)
		{
			ImpactFeedbackController->ResetImmediate(true);
		}
		ImpactFeedbackStyle = InImpactStyle;
		bImpactFeedbackEnabled = bInImpactFeedbackEnabled;
	}
}

void UWacomBattleEnemyPartPresentationComponent::SetTargetableAffordance(bool bInTargetable)
{
	if (bTargetable == bInTargetable)
	{
		return;
	}

	bTargetable = bInTargetable;
	ApplyPersistentScaleState();
}

void UWacomBattleEnemyPartPresentationComponent::SetDragTargetPreviewState(
	EWacomFirstPersonCardDragTargetFeedbackState PreviewState,
	const FWacomBattleEnemyPartDragPredictionDebugInput& PredictionDebugInput)
{
	if (PreviewState != EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget
		&& PreviewState != EWacomFirstPersonCardDragTargetFeedbackState::Invalid)
	{
		ClearDragTargetPreviewState();
		return;
	}

	DragPreviewState = PreviewState;
	LastDragPredictionDebugInput = PredictionDebugInput;
	bDragPreviewActive = true;
	ApplyPersistentScaleState();
	RefreshPredictionDisplay();

	if (TargetPreviewPlayback
		&& bTargetPreviewFeedbackEnabled
		&& IsValid(TargetPreviewFeedbackStyle)
		&& TargetPreviewFeedbackStyle->HasValidVisualAssets())
	{
		const EWacomBattleEnemyPartTargetPreviewKind PreviewKind =
			PreviewState == EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget
				? EWacomBattleEnemyPartTargetPreviewKind::Valid
				: EWacomBattleEnemyPartTargetPreviewKind::Invalid;
		const UWacomBattleEnemyPartTargetPreviewStyle* Style = TargetPreviewFeedbackStyle;
		TargetPreviewPlayback->Begin(
			PreviewKind,
			Style ? Style->EnterSeconds : 0.18f,
			Style ? Style->ExitSeconds : 0.10f,
			Style ? Style->PulsePeriodSeconds : 0.95f,
			bRuntimeSimplifiedMotion);
		if (TargetPreviewFeedbackController)
		{
			TargetPreviewFeedbackController->BeginOrUpdate(
				*this,
				ResolveImpactAnchorComponent(),
				ResolveImpactExtentSourceComponent(),
				Style,
				TargetPreviewPlayback->GetView(),
				RuntimeDecorativeFlashIntensityScale);
		}
	}
	else
	{
		ResetTargetPreviewPlayback(false);
	}
	RefreshComponentTickEnabled();
}

void UWacomBattleEnemyPartPresentationComponent::ClearDragTargetPreviewState()
{
	if (!bDragPreviewActive && DragPreviewState == EWacomFirstPersonCardDragTargetFeedbackState::None)
	{
		return;
	}

	bDragPreviewActive = false;
	DragPreviewState = EWacomFirstPersonCardDragTargetFeedbackState::None;
	LastDragPredictionDebugInput = FWacomBattleEnemyPartDragPredictionDebugInput();
	ApplyPersistentScaleState();
	RefreshPredictionDisplay();
	if (TargetPreviewPlayback)
	{
		TargetPreviewPlayback->BeginExit();
	}
	RefreshComponentTickEnabled();
}

void UWacomBattleEnemyPartPresentationComponent::SetHoverProbeState(
	const FWacomInteractionTargetHandle& TargetHandle,
	FName Reason,
	const FWacomBattleEnemyPartDragPredictionDebugInput& PredictionInput)
{
	if (TargetHandle.TargetKind != EWacomInteractionTargetKind::World
		|| !TargetHandle.TargetTag.MatchesTagExact(WacomTags::Interaction_Target_Battle_EnemyPart)
		|| !TargetHandle.HasBattlePartSlotIdentity())
	{
		ClearHoverProbeState(TEXT("InvalidTarget"));
		return;
	}

	bHoverProbeActive = true;
	HoverReason = Reason.IsNone() ? FName(TEXT("Hovered")) : Reason;
	HoverStableId = TargetHandle.StableTargetId;
	HoverWorldTargetId = TargetHandle.WorldTargetId;
	HoverScreenPosition = TargetHandle.ScreenPosition;
	LastHoverPredictionInput = PredictionInput;
	ApplyPersistentScaleState();
	RefreshPredictionDisplay();
}

void UWacomBattleEnemyPartPresentationComponent::ClearHoverProbeState(FName Reason)
{
	if (!bHoverProbeActive
		&& HoverStableId.IsNone()
		&& !HoverWorldTargetId.IsValid()
		&& HoverScreenPosition.IsNearlyZero())
	{
		HoverReason = Reason;
		return;
	}

	bHoverProbeActive = false;
	HoverReason = Reason;
	HoverStableId = NAME_None;
	HoverWorldTargetId.Invalidate();
	HoverScreenPosition = FVector2D::ZeroVector;
	LastHoverPredictionInput = FWacomBattleEnemyPartDragPredictionDebugInput();
	ApplyPersistentScaleState();
	RefreshPredictionDisplay();
}

void UWacomBattleEnemyPartPresentationComponent::SetActionPreviewPartView(
	const FWacomBattleEnemyPartEntryViewData& InPreviewView)
{
	if (bActionPreviewPartActive
		&& AreActionPreviewPartViewsEquivalent(ActionPreviewPartView, InPreviewView))
	{
		return;
	}

	ActionPreviewPartView = InPreviewView;
	bActionPreviewPartActive = true;
	RefreshPredictionDisplay();
}

void UWacomBattleEnemyPartPresentationComponent::ClearActionPreviewPartView()
{
	if (!bActionPreviewPartActive)
	{
		return;
	}

	bActionPreviewPartActive = false;
	ActionPreviewPartView = FWacomBattleEnemyPartEntryViewData();
	RefreshPredictionDisplay();
}

void UWacomBattleEnemyPartPresentationComponent::SetPredictionWidgetComponent(
	UWidgetComponent* InPredictionWidgetComponent)
{
	PredictionWidgetComponent = InPredictionWidgetComponent;
	PredictionBadgeBaseRelativeLocation = InPredictionWidgetComponent
		? InPredictionWidgetComponent->GetRelativeLocation()
		: FVector::ZeroVector;
	bHasPredictionBadgeBaseRelativeLocation = InPredictionWidgetComponent != nullptr;
	ApplyPredictionViewToWidget();
}

void UWacomBattleEnemyPartPresentationComponent::SetBadgeLayoutDebugState(int32 InStaggerIndex)
{
	BadgeLayoutStaggerIndex = InStaggerIndex;
}

void UWacomBattleEnemyPartPresentationComponent::ClearPredictionDisplay(FName Reason)
{
	CurrentPredictionView = FWacomBattleEnemyPartPredictionView();
	CurrentPredictionView.RejectReason = Reason;
	ApplyPredictionViewToWidget();
}

FWacomBattleEnemyPartPresentationDebugView
UWacomBattleEnemyPartPresentationComponent::GetBattleEnemyPartPresentationDebugView() const
{
	FWacomBattleEnemyPartPresentationDebugView View;
	View.bHasRuntimePartFacts = bHasRuntimePartFacts;
	View.RuntimePartInstanceId = RuntimePartInstanceId;
	View.CurrentInitiative = CurrentInitiative;
	View.bRuntimePartDestroyed = bRuntimePartDestroyed;
	View.CurrentIntentId = CurrentIntentId;
	View.LastCueKind = LastCueKind;
	View.LastCueType = LastCueType;
	View.LastCueAmount = LastCueAmount;
	View.CuePlayCount = CuePlayCount;
	if (CuePlayback)
	{
		const FWacomBattleEnemyPartCuePlaybackView& CueView = CuePlayback->GetView();
		View.ActiveCueKind = CueView.bActive
			? FWacomBattleEnemyPartCuePlayback::KindToName(CueView.Kind)
			: FName(TEXT("None"));
		View.bCuePlaybackActive = CueView.bActive;
		View.CuePlaybackProgress = CueView.Progress;
		View.CuePlaybackDurationSeconds = CueView.DurationSeconds;
	}
	if (const USceneComponent* ImpactAnchor = ResolveImpactAnchorComponent())
	{
		View.bImpactAnchorReady = true;
		View.ImpactAnchorName = FName(*ImpactAnchor->GetName());
		View.ImpactAnchorWorldLocation = ImpactAnchor->GetComponentLocation();
	}
	else if (const AActor* Owner = GetOwner())
	{
		View.bImpactAnchorReady = true;
		View.ImpactAnchorName = FName(*Owner->GetName());
		View.ImpactAnchorWorldLocation = Owner->GetActorLocation();
	}
	View.ResolvedImpactStyleName = ImpactFeedbackStyle
		? FName(*ImpactFeedbackStyle->GetName())
		: NAME_None;
	View.bImpactFeedbackEnabled = bImpactFeedbackEnabled;
	if (ImpactFeedbackController)
	{
		const FWacomBattleEnemyPartImpactFeedbackDebugView& ImpactView =
			ImpactFeedbackController->GetDebugView();
		View.bImpactNiagaraReady = ImpactView.bNiagaraReady;
		View.bImpactEffectActive = ImpactView.bEffectActive;
		View.LastImpactIntensity = ImpactView.LastIntensity;
		View.LastImpactTargetDiameterCentimeters = ImpactView.LastTargetDiameterCentimeters;
		View.LastImpactEffectKind = ImpactView.LastEffectKind;
		View.LastImpactSeed = ImpactView.LastSeed;
		View.ImpactEffectPlayCount = ImpactView.EffectPlayCount;
		View.ImpactSoundRequestCount = ImpactView.SoundRequestCount;
		View.bImpactReducedMotion = ImpactView.bLastReducedMotion;
	View.ImpactDecorativeIntensity = ImpactView.LastDecorativeIntensity;
	}
	View.ResolvedTargetPreviewStyleName = TargetPreviewFeedbackStyle
		? FName(*TargetPreviewFeedbackStyle->GetName())
		: NAME_None;
	View.bTargetPreviewFeedbackEnabled = bTargetPreviewFeedbackEnabled;
	if (TargetPreviewFeedbackController)
	{
		const FWacomBattleEnemyPartTargetPreviewFeedbackDebugView& PreviewView =
			TargetPreviewFeedbackController->GetDebugView();
		View.bTargetPreviewNiagaraReady = PreviewView.bNiagaraReady;
		View.bTargetPreviewEffectActive = PreviewView.bEffectActive;
		View.TargetPreviewAmount = PreviewView.Amount;
		View.TargetPreviewPulse = PreviewView.Pulse;
		View.TargetPreviewSizeCentimeters = PreviewView.TargetSizeCentimeters;
		View.TargetPreviewActivationCount = PreviewView.ActivationCount;
	}
	View.DragPreviewState = DragPreviewState;
	View.bDragPreviewActive = bDragPreviewActive;
	View.LastDragPredictionDebugInput = LastDragPredictionDebugInput;
	View.bHoverActive = bHoverProbeActive;
	View.LastHoverPredictionInput = LastHoverPredictionInput;
	View.HoverReason = HoverReason;
	View.HoverStableId = HoverStableId;
	View.HoverWorldTargetId = HoverWorldTargetId;
	View.HoverScreenPosition = HoverScreenPosition;
	View.PredictionView = CurrentPredictionView;
	View.bActionPreviewPartActive = bActionPreviewPartActive;
	View.ActionPreviewPartView = ActionPreviewPartView;
	if (const UWidgetComponent* PredictionWidget = PredictionWidgetComponent.Get())
	{
		View.PredictionWidgetName = FName(*PredictionWidget->GetName());
		View.PredictionBadgeRelativeLocation = PredictionWidget->GetRelativeLocation();
		View.PredictionBadgeDrawSize = PredictionWidget->GetDrawSize();
	}
	View.PredictionBadgeScale = PredictionBadgeScale;
	View.PredictionBadgeZOffsetWhenVisible = PredictionBadgeZOffsetWhenVisible;
	View.bPredictionBadgeOffsetActive = CurrentPredictionView.bVisible && PredictionBadgeZOffsetWhenVisible > 0.0f;
	View.BadgeLayoutStaggerIndex = BadgeLayoutStaggerIndex;
	return View;
}

void UWacomBattleEnemyPartPresentationComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (CuePlayback && CuePlayback->GetView().bActive)
	{
		const FWacomBattleEnemyPartCuePlaybackView CueView = CuePlayback->Tick(DeltaTime);
		if (!CueView.bActive && ImpactFeedbackController)
		{
			ImpactFeedbackController->FinishNaturally();
		}
	}

	if (TargetPreviewPlayback && TargetPreviewPlayback->GetView().bActive)
	{
		const FWacomBattleEnemyPartTargetPreviewPlaybackView PreviewView =
			TargetPreviewPlayback->Tick(DeltaTime);
		if (PreviewView.bActive)
		{
			if (bTargetPreviewFeedbackEnabled
				&& IsValid(TargetPreviewFeedbackStyle)
				&& TargetPreviewFeedbackController)
			{
				TargetPreviewFeedbackController->BeginOrUpdate(
					*this,
					ResolveImpactAnchorComponent(),
					ResolveImpactExtentSourceComponent(),
					TargetPreviewFeedbackStyle,
					PreviewView,
					RuntimeDecorativeFlashIntensityScale);
			}
		}
		else if (TargetPreviewFeedbackController)
		{
			TargetPreviewFeedbackController->FinishNaturally();
		}
	}
	RefreshComponentTickEnabled();
}

void UWacomBattleEnemyPartPresentationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindRuntimeSettings();
	ResetBattlePresentationFeedback();
	if (ImpactFeedbackController)
	{
		ImpactFeedbackController->ResetImmediate(true);
	}
	ResetTargetPreviewPlayback(true);
	Super::EndPlay(EndPlayReason);
}

UPrimitiveComponent* UWacomBattleEnemyPartPresentationComponent::ResolveVisualTargetComponent() const
{
	if (VisualTargetComponent)
	{
		return VisualTargetComponent;
	}

	AActor* Owner = GetOwner();
	return Owner ? Owner->FindComponentByClass<UPrimitiveComponent>() : nullptr;
}

USceneComponent* UWacomBattleEnemyPartPresentationComponent::ResolveFeedbackTargetComponent() const
{
	if (FeedbackTargetComponent)
	{
		return FeedbackTargetComponent;
	}

	return ResolveVisualTargetComponent();
}

USceneComponent* UWacomBattleEnemyPartPresentationComponent::ResolveImpactAnchorComponent() const
{
	if (IsValid(ImpactAnchorComponent))
	{
		return ImpactAnchorComponent;
	}

	if (const AActor* Owner = GetOwner())
	{
		return Owner->GetRootComponent();
	}
	return nullptr;
}

UPrimitiveComponent* UWacomBattleEnemyPartPresentationComponent::ResolveImpactExtentSourceComponent() const
{
	if (IsValid(ImpactExtentSourceComponent))
	{
		return ImpactExtentSourceComponent;
	}
	return ResolveVisualTargetComponent();
}

void UWacomBattleEnemyPartPresentationComponent::RefreshPredictionDisplay()
{
	if (!bEnablePredictionDisplay)
	{
		ClearPredictionDisplay(TEXT("Disabled"));
		return;
	}

	if (!bHasRuntimePartFacts || bRuntimePartDestroyed)
	{
		ClearPredictionDisplay(bRuntimePartDestroyed ? TEXT("PartDestroyed") : TEXT("MissingRuntimeFacts"));
		return;
	}

	if (bActionPreviewPartActive)
	{
		CurrentPredictionView = BuildActionPreviewPredictionView();
	}
	else if (bDragPreviewActive)
	{
		CurrentPredictionView = BuildPredictionView(LastDragPredictionDebugInput);
	}
	else if (bHoverProbeActive)
	{
		ClearPredictionDisplay(TEXT("EnemyPanelHover"));
		return;
	}
	else
	{
		ClearPredictionDisplay(TEXT("NoProbe"));
		return;
	}

	ApplyPredictionViewToWidget();
}

void UWacomBattleEnemyPartPresentationComponent::ApplyPredictionViewToWidget()
{
	UWidgetComponent* WidgetComponent = PredictionWidgetComponent.Get();
	if (!WidgetComponent)
	{
		return;
	}

	WidgetComponent->SetVisibility(CurrentPredictionView.bVisible, true);
	if (!bHasPredictionBadgeBaseRelativeLocation)
	{
		PredictionBadgeBaseRelativeLocation = WidgetComponent->GetRelativeLocation();
		bHasPredictionBadgeBaseRelativeLocation = true;
	}
	const FVector BaseLocation = PredictionBadgeBaseRelativeLocation;
	const FVector DesiredLocation(
		BaseLocation.X,
		BaseLocation.Y,
		CurrentPredictionView.bVisible
			? BaseLocation.Z + FMath::Max(0.0f, PredictionBadgeZOffsetWhenVisible)
			: BaseLocation.Z);
	WidgetComponent->SetRelativeLocation(DesiredLocation);
	WidgetComponent->InitWidget();
	if (UUserWidget* UserWidget = WidgetComponent->GetUserWidgetObject())
	{
		UserWidget->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		UserWidget->SetRenderScale(FVector2D(PredictionBadgeScale, PredictionBadgeScale));
		UserWidget->SetRenderOpacity(1.0f);
		if (UWacomBattleEnemyPartPredictionWidget* PredictionWidget =
			Cast<UWacomBattleEnemyPartPredictionWidget>(UserWidget))
		{
			PredictionWidget->SetPredictionView(CurrentPredictionView);
		}
	}
}

FWacomBattleEnemyPartPredictionView UWacomBattleEnemyPartPresentationComponent::BuildPredictionView(
	const FWacomBattleEnemyPartDragPredictionDebugInput& PredictionInput) const
{
	FWacomBattleEnemyPartPredictionView View;
	View.bVisible = true;
	View.CurrentInitiative = CurrentInitiative;
	View.PredictedInitiative = CurrentInitiative;
	View.bHasSourceCard = PredictionInput.bHasSourceCard;
	View.SourceCardRuntimeCost = PredictionInput.SourceCardRuntimeCost;
	View.bSourceCardSwift = PredictionInput.bSourceCardSwift;

	if (!PredictionInput.bHasSourceCard)
	{
		View.Mode = EWacomBattleEnemyPartPredictionMode::HoverInitiative;
		View.MainText = FText::Format(
			LOCTEXT("HoverInitiativeFmt", "先机 {0}"),
			FText::AsNumber(CurrentInitiative));
		return View;
	}

	if (!PredictionInput.bPreviewCanSubmit)
	{
		View.Mode = EWacomBattleEnemyPartPredictionMode::Rejected;
		View.RejectReason = PredictionInput.PreviewRejectReason.IsNone()
			? FName(TEXT("Rejected"))
			: PredictionInput.PreviewRejectReason;
		View.MainText = LOCTEXT("RejectedMain", "不可释放");
		View.DetailText = FText::FromName(View.RejectReason);
		return View;
	}

	View.Mode = EWacomBattleEnemyPartPredictionMode::CardPrediction;
	if (!PredictionInput.bSourceCardSwift)
	{
		View.PredictedInitiative = CurrentInitiative - PredictionInput.SourceCardRuntimeCost;
		View.bPerfectReleaseCandidate = PredictionInput.SourceCardRuntimeCost == CurrentInitiative;
		View.bActionRisk = View.PredictedInitiative <= 0;
		View.MainText = FText::Format(
			LOCTEXT("PredictionFmt", "先机 {0} -> {1}"),
			FText::AsNumber(CurrentInitiative),
			FText::AsNumber(View.PredictedInitiative));
	}
	else
	{
		View.MainText = FText::Format(
			LOCTEXT("SwiftPredictionFmt", "先机 {0}"),
			FText::AsNumber(CurrentInitiative));
		View.DetailText = LOCTEXT("SwiftDetail", "迅捷：不推进");
	}

	if (View.bPerfectReleaseCandidate)
	{
		View.DetailText = LOCTEXT("PerfectReleaseCandidate", "完美释放");
	}
	else if (View.bActionRisk)
	{
		View.DetailText = LOCTEXT("ActionRisk", "行动风险");
	}
	return View;
}

FWacomBattleEnemyPartPredictionView UWacomBattleEnemyPartPresentationComponent::BuildActionPreviewPredictionView() const
{
	FWacomBattleEnemyPartPredictionView View;
	View.bVisible = true;
	View.Mode = EWacomBattleEnemyPartPredictionMode::CardPrediction;
	View.CurrentInitiative = CurrentInitiative;
	View.PredictedInitiative = ActionPreviewPartView.CurrentInitiative;
	View.bActionRisk = ActionPreviewPartView.bActionPreviewWillAct;
	View.MainText = FText::Format(
		LOCTEXT("ActionPreviewPredictionFmt", "先机 {0} -> {1}"),
		FText::AsNumber(CurrentInitiative),
		FText::AsNumber(ActionPreviewPartView.CurrentInitiative));
	if (ActionPreviewPartView.bActionPreviewWillAct)
	{
		View.DetailText = LOCTEXT("ActionPreviewActionRisk", "行动风险");
	}
	else if (ActionPreviewPartView.bDestroyed)
	{
		View.DetailText = LOCTEXT("ActionPreviewDestroyed", "将被破坏");
	}
	return View;
}

void UWacomBattleEnemyPartPresentationComponent::ApplyPersistentScaleState()
{
	if (bDragPreviewActive)
	{
		// Drag Preview uses the dedicated pixel lock frame. Suppress the broader
		// targetable scale while the pointer owns a concrete part target.
		RestoreBaseScaleIfNeeded();
		return;
	}
	if (bTargetable)
	{
		ApplyPersistentScaleMultiplier(TargetableAffordanceScale);
		return;
	}
	if (bHoverProbeActive)
	{
		ApplyPersistentScaleMultiplier(HoverProbeScale);
		return;
	}
	RestoreBaseScaleIfNeeded();
}

void UWacomBattleEnemyPartPresentationComponent::ApplyPersistentScaleMultiplier(
	float ScaleMultiplier)
{
	USceneComponent* FeedbackTarget = ResolveFeedbackTargetComponent();
	if (!FeedbackTarget)
	{
		RestoreBaseScaleIfNeeded();
		return;
	}

	if (!bHasCachedBaseScale || CachedFeedbackTarget.Get() != FeedbackTarget)
	{
		RestoreBaseScaleIfNeeded();
		CachedFeedbackTarget = FeedbackTarget;
		CachedBaseScale = FeedbackTarget->GetRelativeScale3D();
		bHasCachedBaseScale = true;
	}

	FeedbackTarget->SetRelativeScale3D(CachedBaseScale * FMath::Max(1.0f, ScaleMultiplier));
}

void UWacomBattleEnemyPartPresentationComponent::ResetCuePlayback()
{
	if (CuePlayback)
	{
		CuePlayback->Reset();
	}
	if (ImpactFeedbackController)
	{
		ImpactFeedbackController->ResetImmediate(false);
	}
	RefreshComponentTickEnabled();
}

void UWacomBattleEnemyPartPresentationComponent::ResetTargetPreviewPlayback(bool bDestroyComponent)
{
	if (TargetPreviewPlayback)
	{
		TargetPreviewPlayback->Reset();
	}
	if (TargetPreviewFeedbackController)
	{
		TargetPreviewFeedbackController->ResetImmediate(bDestroyComponent);
	}
	RefreshComponentTickEnabled();
}

void UWacomBattleEnemyPartPresentationComponent::RefreshComponentTickEnabled()
{
	const bool bCueActive = CuePlayback && CuePlayback->GetView().bActive;
	const bool bTargetPreviewActive = TargetPreviewPlayback
		&& TargetPreviewPlayback->GetView().bActive;
	SetComponentTickEnabled(bCueActive || bTargetPreviewActive);
}

void UWacomBattleEnemyPartPresentationComponent::BindRuntimeSettings()
{
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UWacomSettingsSubsystem* SettingsSubsystem = GameInstance
		? GameInstance->GetSubsystem<UWacomSettingsSubsystem>()
		: nullptr;
	if (!SettingsSubsystem)
	{
		return;
	}

	RuntimeSettingsChangedHandle = SettingsSubsystem->OnRuntimeSettingsChangedNative().AddUObject(
		this,
		&UWacomBattleEnemyPartPresentationComponent::HandleRuntimeSettingsChanged);
	HandleRuntimeSettingsChanged(
		SettingsSubsystem->GetCurrentSnapshot(),
		EWacomRuntimeSettingsChangeReason::Startup);
}

void UWacomBattleEnemyPartPresentationComponent::UnbindRuntimeSettings()
{
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	if (UWacomSettingsSubsystem* SettingsSubsystem = GameInstance
		? GameInstance->GetSubsystem<UWacomSettingsSubsystem>()
		: nullptr)
	{
		SettingsSubsystem->OnRuntimeSettingsChangedNative().Remove(RuntimeSettingsChangedHandle);
	}
	RuntimeSettingsChangedHandle.Reset();
}

void UWacomBattleEnemyPartPresentationComponent::HandleRuntimeSettingsChanged(
	const FWacomLocalSettingsSnapshot& Snapshot,
	EWacomRuntimeSettingsChangeReason /*Reason*/)
{
	RuntimeDecorativeFlashIntensityScale =
		FWacomPresentationAccessibilityPolicy::GetDecorativeFlashIntensityScale(
			Snapshot.FlashEffectMode);
	bRuntimeSimplifiedMotion =
		FWacomPresentationAccessibilityPolicy::UsesSimplifiedMotion(Snapshot.UIMotionMode);

	if (TargetPreviewPlayback && TargetPreviewPlayback->GetView().bActive)
	{
		const FWacomBattleEnemyPartTargetPreviewPlaybackView CurrentView =
			TargetPreviewPlayback->GetView();
		const UWacomBattleEnemyPartTargetPreviewStyle* Style = TargetPreviewFeedbackStyle;
		TargetPreviewPlayback->Begin(
			CurrentView.Kind,
			Style ? Style->EnterSeconds : 0.18f,
			Style ? Style->ExitSeconds : 0.10f,
			Style ? Style->PulsePeriodSeconds : 0.95f,
			bRuntimeSimplifiedMotion);
	}
}

void UWacomBattleEnemyPartPresentationComponent::RestoreBaseScaleIfNeeded()
{
	if (bHasCachedBaseScale)
	{
		if (USceneComponent* FeedbackTarget = CachedFeedbackTarget.Get())
		{
			FeedbackTarget->SetRelativeScale3D(CachedBaseScale);
		}
	}
	CachedFeedbackTarget.Reset();
	CachedBaseScale = FVector::OneVector;
	bHasCachedBaseScale = false;
}

#undef LOCTEXT_NAMESPACE
