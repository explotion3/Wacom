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
#include "UI/Battle/WacomBattleEnemyPartStatusBadgeWidget.h"
#include "UI/Battle/WacomBattleEventPresentationBuilder.h"

#define LOCTEXT_NAMESPACE "WacomBattleEnemyPartPresentation"

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
	RuntimePartDisplayName =
		Part.Definition && !Part.Definition->DisplayName.IsEmpty()
			? Part.Definition->DisplayName
			: FText::FromName(PartId);
	CurrentHp = Part.CurrentHp;
	MaxHp = Part.MaxHp;
	Shield = Part.Shield;
	CurrentInitiative = Part.CurrentInitiative;
	bRuntimePartDestroyed = Part.bDestroyed;
	CurrentIntentId = Part.CurrentIntent.IntentId;
	CurrentIntentDisplayName = Part.CurrentIntent.DisplayName;
	CurrentIntentInitiative = Part.CurrentIntent.Initiative;
	CurrentIntentResistanceValue = Part.CurrentIntent.ResistanceValue;
	RuntimeStatuses = Part.Statuses;
	RuntimeStatusStacks = Part.StatusStacks;
	RefreshStatusBadgeDisplay();
	RefreshPredictionDisplay();
}

void UWacomBattleEnemyPartPresentationComponent::ClearRuntimePartFacts()
{
	RuntimePartInstanceId.Invalidate();
	bHasRuntimePartFacts = false;
	RuntimePartDisplayName = FText::GetEmpty();
	CurrentHp = 0;
	MaxHp = 0;
	Shield = 0;
	CurrentInitiative = 0;
	bRuntimePartDestroyed = false;
	CurrentIntentId = NAME_None;
	CurrentIntentDisplayName = FText::GetEmpty();
	CurrentIntentInitiative = 0;
	CurrentIntentResistanceValue = 0;
	RuntimeStatuses.Reset();
	RuntimeStatusStacks.Reset();
	RefreshStatusBadgeDisplay();
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

void UWacomBattleEnemyPartPresentationComponent::SetStatusBadgeWidgetComponent(
	UWidgetComponent* InStatusBadgeWidgetComponent)
{
	StatusBadgeWidgetComponent = InStatusBadgeWidgetComponent;
	ApplyStatusBadgeViewToWidget();
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

void UWacomBattleEnemyPartPresentationComponent::ClearStatusBadgeDisplay(FName /*Reason*/)
{
	CurrentStatusBadgeView = FWacomBattleEnemyPartStatusBadgeView();
	ApplyStatusBadgeViewToWidget();
}

FWacomBattleEnemyPartPresentationDebugView
UWacomBattleEnemyPartPresentationComponent::GetBattleEnemyPartPresentationDebugView() const
{
	FWacomBattleEnemyPartPresentationDebugView View;
	View.bHasRuntimePartFacts = bHasRuntimePartFacts;
	View.RuntimePartInstanceId = RuntimePartInstanceId;
	View.RuntimePartDisplayName = RuntimePartDisplayName;
	View.CurrentHp = CurrentHp;
	View.MaxHp = MaxHp;
	View.Shield = Shield;
	View.CurrentInitiative = CurrentInitiative;
	View.bRuntimePartDestroyed = bRuntimePartDestroyed;
	View.CurrentIntentId = CurrentIntentId;
	View.CurrentIntentDisplayName = CurrentIntentDisplayName;
	View.CurrentIntentInitiative = CurrentIntentInitiative;
	View.CurrentIntentResistanceValue = CurrentIntentResistanceValue;
	View.LastCueKind = LastCueKind;
	View.LastCueType = LastCueType;
	View.LastCueAmount = LastCueAmount;
	View.CuePlayCount = CuePlayCount;
	View.DragPreviewState = DragPreviewState;
	View.bDragPreviewActive = bDragPreviewActive;
	View.LastDragPredictionDebugInput = LastDragPredictionDebugInput;
	View.bHoverActive = bHoverProbeActive;
	View.HoverReason = HoverReason;
	View.HoverStableId = HoverStableId;
	View.HoverWorldTargetId = HoverWorldTargetId;
	View.HoverScreenPosition = HoverScreenPosition;
	View.PredictionView = CurrentPredictionView;
	View.StatusBadgeView = CurrentStatusBadgeView;
	if (const UWidgetComponent* PredictionWidget = PredictionWidgetComponent.Get())
	{
		View.PredictionWidgetName = FName(*PredictionWidget->GetName());
		View.PredictionBadgeRelativeLocation = PredictionWidget->GetRelativeLocation();
		View.PredictionBadgeDrawSize = PredictionWidget->GetDrawSize();
	}
	if (const UWidgetComponent* StatusWidgetComponent = StatusBadgeWidgetComponent.Get())
	{
		View.StatusBadgeWidgetName = FName(*StatusWidgetComponent->GetName());
		View.StatusBadgeRelativeLocation = StatusWidgetComponent->GetRelativeLocation();
		View.StatusBadgeDrawSize = StatusWidgetComponent->GetDrawSize();
	}
	View.PredictionBadgeScale = PredictionBadgeScale;
	View.StatusBadgeScale = StatusBadgeScale;
	View.StatusBadgeOpacity = StatusBadgeOpacity;
	View.DestroyedStatusBadgeOpacity = DestroyedStatusBadgeOpacity;
	View.CurrentStatusBadgeAppliedOpacity = CurrentStatusBadgeView.bDestroyed
		? DestroyedStatusBadgeOpacity
		: StatusBadgeOpacity;
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

	if (bDragPreviewActive)
	{
		CurrentPredictionView = BuildPredictionView(LastDragPredictionDebugInput);
	}
	else if (bHoverProbeActive)
	{
		CurrentPredictionView = BuildPredictionView(LastHoverPredictionInput);
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

void UWacomBattleEnemyPartPresentationComponent::RefreshStatusBadgeDisplay()
{
	if (!bEnableStatusBadgeDisplay || !bHasRuntimePartFacts)
	{
		ClearStatusBadgeDisplay(!bEnableStatusBadgeDisplay ? TEXT("Disabled") : TEXT("MissingRuntimeFacts"));
		return;
	}

	CurrentStatusBadgeView = BuildStatusBadgeView();
	ApplyStatusBadgeViewToWidget();
}

void UWacomBattleEnemyPartPresentationComponent::ApplyStatusBadgeViewToWidget()
{
	UWidgetComponent* WidgetComponent = StatusBadgeWidgetComponent.Get();
	if (!WidgetComponent)
	{
		return;
	}

	WidgetComponent->SetVisibility(CurrentStatusBadgeView.bVisible, true);
	const float AppliedOpacity = CurrentStatusBadgeView.bDestroyed
		? DestroyedStatusBadgeOpacity
		: StatusBadgeOpacity;
	WidgetComponent->InitWidget();
	if (UUserWidget* UserWidget = WidgetComponent->GetUserWidgetObject())
	{
		UserWidget->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		UserWidget->SetRenderScale(FVector2D(StatusBadgeScale, StatusBadgeScale));
		UserWidget->SetRenderOpacity(AppliedOpacity);
		if (UWacomBattleEnemyPartStatusBadgeWidget* StatusBadgeWidget =
			Cast<UWacomBattleEnemyPartStatusBadgeWidget>(UserWidget))
		{
			StatusBadgeWidget->SetStatusBadgeView(CurrentStatusBadgeView);
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

FWacomBattleEnemyPartStatusBadgeView
UWacomBattleEnemyPartPresentationComponent::BuildStatusBadgeView() const
{
	FWacomBattleEnemyPartStatusBadgeView View;
	View.bVisible = bHasRuntimePartFacts;
	View.PartId = PartId;
	View.PartInstanceId = RuntimePartInstanceId;
	View.PartNameText = RuntimePartDisplayName.IsEmpty() ? FText::FromName(PartId) : RuntimePartDisplayName;
	View.CurrentHp = CurrentHp;
	View.MaxHp = MaxHp;
	View.Shield = Shield;
	View.CurrentInitiative = CurrentInitiative;
	View.CurrentIntentId = CurrentIntentId;
	View.bDestroyed = bRuntimePartDestroyed;
	View.HpText = FText::Format(
		LOCTEXT("StatusBadgeHpFmt", "{0}/{1}"),
		FText::AsNumber(CurrentHp),
		FText::AsNumber(MaxHp));
	View.InitiativeText = FText::Format(
		LOCTEXT("StatusBadgeInitiativeFmt", "先机 {0}"),
		FText::AsNumber(CurrentInitiative));
	if (bRuntimePartDestroyed)
	{
		View.CurrentIntentText = LOCTEXT("StatusBadgeDestroyedIntent", "已破坏");
	}
	else
	{
		const FText IntentName = CurrentIntentDisplayName.IsEmpty()
			? FText::FromString(TEXT("--"))
			: CurrentIntentDisplayName;
		View.CurrentIntentText = FText::Format(
			LOCTEXT("StatusBadgeIntentFmt", "意图 {0}"),
			IntentName);
	}
	if (Shield > 0)
	{
		View.ShieldText = FText::Format(
			LOCTEXT("StatusBadgeShieldFmt", "护盾 {0}"),
			FText::AsNumber(Shield));
	}
	View.StatusText = BuildStatusBadgeStatusText();
	return View;
}

FText UWacomBattleEnemyPartPresentationComponent::BuildStatusBadgeStatusText() const
{
	TArray<FString> Parts;
	TArray<FGameplayTag> Tags;
	RuntimeStatuses.GetGameplayTagArray(Tags);
	Tags.Sort([](const FGameplayTag& Left, const FGameplayTag& Right)
	{
		return Left.GetTagName().LexicalLess(Right.GetTagName());
	});
	for (const FGameplayTag& Tag : Tags)
	{
		const int32* Stack = RuntimeStatusStacks.Find(Tag);
		const int32 StackCount = Stack ? *Stack : 0;
		const FString StatusName = UWacomBattleEventPresentationBuilder::FormatStatusName(Tag);
		Parts.Add(StackCount > 1
			? FString::Printf(TEXT("%s x%d"), *StatusName, StackCount)
			: StatusName);
	}
	return Parts.Num() > 0
		? FText::FromString(FString::Join(Parts, TEXT(" / ")))
		: FText::GetEmpty();
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
