// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattleEnemyPartPresentationComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Enemies/EnemyPartDefinition.h"
#include "GameFramework/Actor.h"
#include "Snapshots/EnemySnapshot.h"
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

UWacomBattleEnemyPartPresentationComponent::UWacomBattleEnemyPartPresentationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
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
	if (Cue.CueKind == EWacomBattlePresentationTargetCueKind::BattleEvent
		&& Cue.SourceEventType != EBattleEventType::DamageDealt
		&& Cue.SourceEventType != EBattleEventType::EnemyPartHpEmptied)
	{
		return;
	}

	LastCueKind = WacomBattlePresentationTargetCueKindToName(Cue.CueKind);
	LastCueType = Cue.SourceEventType;
	LastCueAmount = Cue.Amount;
	++CuePlayCount;

	float ScaleMultiplier = DamagePulseScale;
	if (Cue.CueKind == EWacomBattlePresentationTargetCueKind::TargetConfirmed)
	{
		ScaleMultiplier = TargetConfirmPulseScale;
	}
	else if (Cue.SourceEventType == EBattleEventType::EnemyPartHpEmptied)
	{
		ScaleMultiplier = DestroyedPulseScale;
	}

	BeginScaleFeedback(ScaleMultiplier, Cue.Duration > 0.0f ? Cue.Duration : CueHoldSeconds);
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

void UWacomBattleEnemyPartPresentationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopFeedbackTimer();
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
		BeginScaleFeedback(DragTargetPreviewScale, 0.0f);
		return;
	}
	if (bTargetable)
	{
		BeginScaleFeedback(TargetableAffordanceScale, 0.0f);
		return;
	}
	if (bHoverProbeActive)
	{
		BeginScaleFeedback(HoverProbeScale, 0.0f);
		return;
	}
	RestoreBaseScaleIfNeeded();
}

void UWacomBattleEnemyPartPresentationComponent::BeginScaleFeedback(
	float ScaleMultiplier,
	float HoldSeconds)
{
	USceneComponent* FeedbackTarget = ResolveFeedbackTargetComponent();
	if (!FeedbackTarget)
	{
		return;
	}

	if (!bHasCachedBaseScale || CachedFeedbackTarget.Get() != FeedbackTarget)
	{
		CachedFeedbackTarget = FeedbackTarget;
		CachedBaseScale = FeedbackTarget->GetRelativeScale3D();
		bHasCachedBaseScale = true;
	}

	FeedbackTarget->SetRelativeScale3D(CachedBaseScale * FMath::Max(1.0f, ScaleMultiplier));

	if (HoldSeconds > 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				FeedbackTimerHandle,
				this,
				&UWacomBattleEnemyPartPresentationComponent::ClearScaleFeedback,
				HoldSeconds,
				false);
		}
	}
}

void UWacomBattleEnemyPartPresentationComponent::ClearScaleFeedback()
{
	StopFeedbackTimer();
	ApplyPersistentScaleState();
}

void UWacomBattleEnemyPartPresentationComponent::RestoreBaseScaleIfNeeded()
{
	StopFeedbackTimer();
	if (bHasCachedBaseScale)
	{
		if (USceneComponent* FeedbackTarget = CachedFeedbackTarget.Get())
		{
			FeedbackTarget->SetRelativeScale3D(CachedBaseScale);
		}
	}
}

void UWacomBattleEnemyPartPresentationComponent::StopFeedbackTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FeedbackTimerHandle);
	}
	FeedbackTimerHandle = FTimerHandle();
}

#undef LOCTEXT_NAMESPACE
