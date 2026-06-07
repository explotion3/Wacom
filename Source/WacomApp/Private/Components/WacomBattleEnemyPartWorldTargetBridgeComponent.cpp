// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattleEnemyPartWorldTargetBridgeComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "Enemies/EnemyPartDefinition.h"
#include "GameFramework/Actor.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/WacomBattleEnemyPartStatusBadgeWidget.h"
#include "UI/Battle/WacomBattleEnemyPartPredictionWidget.h"
#include "UI/Battle/WacomBattleEventPresentationBuilder.h"

#define LOCTEXT_NAMESPACE "WacomBattleEnemyPartWorldTargetBridge"

UWacomBattleEnemyPartWorldTargetBridgeComponent::UWacomBattleEnemyPartWorldTargetBridgeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::SetPartId(FName InPartId)
{
	PartId = InPartId;
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::SetBattlePartSlotIdentity(
	FName InEncounterId,
	FName InEnemySlotId,
	FName InPartSlotId)
{
	EncounterId = InEncounterId;
	EnemySlotId = InEnemySlotId;
	PartSlotId = InPartSlotId;
}

bool UWacomBattleEnemyPartWorldTargetBridgeComponent::SyncFromBattleHUD(
	UBattleHUD& HUD,
	const FBattleSnapshot& Snapshot,
	const FBattleTargetSelectionView& TargetSelectionView)
{
	if (PartId.IsNone())
	{
		LastBindResult = TEXT("MissingPartId");
		ClearBattleBinding();
		return false;
	}

	const FEnemyPartSnapshot* MatchedPart = nullptr;
	const FName EffectiveEncounterId = EncounterId.IsNone() ? Snapshot.EncounterId : EncounterId;
	const FName EffectiveEnemySlotId = EnemySlotId.IsNone() ? FName(TEXT("Enemy")) : EnemySlotId;
	const FName EffectivePartSlotId = PartSlotId.IsNone() ? PartId : PartSlotId;
	const bool bUseExplicitSlotBinding =
		!EncounterId.IsNone()
		|| EffectiveEnemySlotId != FName(TEXT("Enemy"))
		|| (!EffectivePartSlotId.IsNone() && EffectivePartSlotId != PartId);
	if (bUseExplicitSlotBinding && !EffectivePartSlotId.IsNone())
	{
		for (const FEnemySnapshot& EnemySnapshot : Snapshot.Enemies)
		{
			if (EnemySnapshot.EncounterId != EffectiveEncounterId)
			{
				continue;
			}
			if (EnemySnapshot.EnemySlotId != EffectiveEnemySlotId)
			{
				continue;
			}

			for (const FEnemyPartSnapshot& Part : EnemySnapshot.Parts)
			{
				if (Part.PartSlotId == EffectivePartSlotId)
				{
					MatchedPart = &Part;
					break;
				}
			}
			if (MatchedPart)
			{
				break;
			}
		}
	}

	if (!MatchedPart)
	{
		for (const FEnemySnapshot& EnemySnapshot : Snapshot.Enemies)
		{
			for (const FEnemyPartSnapshot& Part : EnemySnapshot.Parts)
			{
				if (Part.Definition && Part.Definition->PartId == PartId)
				{
					MatchedPart = &Part;
					break;
				}
			}
			if (MatchedPart)
			{
				break;
			}
		}
	}

	if (!MatchedPart || !MatchedPart->InstanceId.IsValid() || MatchedPart->bDestroyed)
	{
		LastBindResult = MatchedPart && MatchedPart->bDestroyed ? TEXT("PartDestroyed") : TEXT("NoMatchingPart");
		if (MatchedPart)
		{
			CacheRuntimePartFacts(*MatchedPart);
			ClearBattleBindingInternal(/*bClearRuntimeFacts=*/false);
		}
		else
		{
			ClearBattleBinding();
		}
		return false;
	}

	PartInstanceId = MatchedPart->InstanceId;
	BoundEncounterId = MatchedPart->EncounterId;
	BoundEnemySlotId = MatchedPart->EnemySlotId;
	BoundPartSlotId = MatchedPart->PartSlotId;
	bBoundToSnapshot = true;
	LastBindResult = MatchedPart->PartSlotId == EffectivePartSlotId && MatchedPart->EnemySlotId == EffectiveEnemySlotId
		? TEXT("MatchedPartSlot")
		: TEXT("MatchedPartId");
	CacheRuntimePartFacts(*MatchedPart);

	bool bNewTargetable = false;
	FName NewDisabledReason = NAME_None;
	for (const FBattleTargetablePartView& PartView : TargetSelectionView.TargetableParts)
	{
		if (PartView.PartInstanceId == PartInstanceId)
		{
			bNewTargetable = PartView.bTargetable;
			NewDisabledReason = PartView.DisabledReason;
			break;
		}
	}
	TargetDisabledReason = NewDisabledReason;
	ApplyTargetableAffordance(bNewTargetable);
	UpdateInteractionTargetComponent();
	RegisterWithBattleHUD(HUD);
	return true;
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::ClearBattleBinding()
{
	ClearBattleBindingInternal(/*bClearRuntimeFacts=*/true);
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::ClearBattleBindingInternal(bool bClearRuntimeFacts)
{
	UnregisterFromBattleHUD();
	ClearDragTargetPreviewState();
	ClearHoverProbeState(TEXT("BindingCleared"));
	PartInstanceId.Invalidate();
	BoundEncounterId = NAME_None;
	BoundEnemySlotId = NAME_None;
	BoundPartSlotId = NAME_None;
	bBoundToSnapshot = false;
	TargetDisabledReason = NAME_None;
	ApplyTargetableAffordance(false);
	if (bClearRuntimeFacts)
	{
		ClearRuntimePartFacts();
	}

	if (UWacomInteractionTargetComponent* TargetComponent = ResolveInteractionTargetComponent())
	{
		TargetComponent->SetTargetId(FGuid());
		TargetComponent->SetBattlePartSlotIdentity(NAME_None, NAME_None, NAME_None);
	}
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::PlayBattlePresentationCue(
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

FWacomBattleEnemyPartWorldTargetDebugView
UWacomBattleEnemyPartWorldTargetBridgeComponent::GetBattleWorldTargetDebugView() const
{
	FWacomBattleEnemyPartWorldTargetDebugView View;
	View.PartId = PartId;
	View.EncounterId = BoundEncounterId;
	View.EnemySlotId = BoundEnemySlotId;
	View.PartSlotId = BoundPartSlotId;
	View.PartInstanceId = PartInstanceId;
	View.bBoundToSnapshot = bBoundToSnapshot;
	View.bRegisteredWithBattleHUD = bRegisteredWithBattleHUD;
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
	View.bTargetable = bTargetable;
	View.TargetDisabledReason = TargetDisabledReason;
	View.LastBindResult = LastBindResult;
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

FString UWacomBattleEnemyPartWorldTargetBridgeComponent::GetBattleWorldTargetDebugSummary() const
{
	const FWacomBattleEnemyPartWorldTargetDebugView View = GetBattleWorldTargetDebugView();
	return FString::Printf(
		TEXT("BattleEnemyPartWorldTarget{Owner=%s PartId=%s EncounterId=%s EnemySlotId=%s PartSlotId=%s PartInstanceId=%s Bound=%s Registered=%s RuntimeFacts=%s RuntimePart=%s Hp=%d MaxHp=%d Shield=%d Initiative=%d Destroyed=%s Intent=%s IntentText=%s IntentInitiative=%d IntentResistance=%d StatusText=%s StatusBadgeVisible=%s StatusBadgeWidget=%s StatusBadgeLocation=%s StatusBadgeDrawSize=%s StatusBadgeScale=%.2f StatusBadgeOpacity=%.2f DestroyedStatusBadgeOpacity=%.2f CurrentStatusBadgeOpacity=%.2f PredictionWidget=%s PredictionBadgeLocation=%s PredictionBadgeDrawSize=%s PredictionBadgeScale=%.2f PredictionBadgeZOffset=%.1f PredictionBadgeOffsetActive=%s BadgeStaggerIndex=%d Targetable=%s Disabled=%s LastBind=%s LastCue=%s CueType=%d CueAmount=%d CueCount=%d DragPreview=%d DragPreviewActive=%s DragSource=%s DragCost=%d DragSwift=%s DragCanSubmit=%s DragReject=%s HoverActive=%s HoverReason=%s HoverStableId=%s HoverWorldTargetId=%s HoverScreen=%s PredictionVisible=%s PredictionMode=%d PredictedInitiative=%d PerfectCandidate=%s ActionRisk=%s PredictionReject=%s}"),
		*GetNameSafe(GetOwner()),
		*View.PartId.ToString(),
		*View.EncounterId.ToString(),
		*View.EnemySlotId.ToString(),
		*View.PartSlotId.ToString(),
		*View.PartInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
		View.bBoundToSnapshot ? TEXT("true") : TEXT("false"),
		View.bRegisteredWithBattleHUD ? TEXT("true") : TEXT("false"),
		View.bHasRuntimePartFacts ? TEXT("true") : TEXT("false"),
		*View.RuntimePartInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
		View.CurrentHp,
		View.MaxHp,
		View.Shield,
		View.CurrentInitiative,
		View.bRuntimePartDestroyed ? TEXT("true") : TEXT("false"),
		*View.CurrentIntentId.ToString(),
		*View.StatusBadgeView.CurrentIntentText.ToString(),
		View.CurrentIntentInitiative,
		View.CurrentIntentResistanceValue,
		*View.StatusBadgeView.StatusText.ToString(),
		View.StatusBadgeView.bVisible ? TEXT("true") : TEXT("false"),
		*View.StatusBadgeWidgetName.ToString(),
		*View.StatusBadgeRelativeLocation.ToCompactString(),
		*View.StatusBadgeDrawSize.ToString(),
		View.StatusBadgeScale,
		View.StatusBadgeOpacity,
		View.DestroyedStatusBadgeOpacity,
		View.CurrentStatusBadgeAppliedOpacity,
		*View.PredictionWidgetName.ToString(),
		*View.PredictionBadgeRelativeLocation.ToCompactString(),
		*View.PredictionBadgeDrawSize.ToString(),
		View.PredictionBadgeScale,
		View.PredictionBadgeZOffsetWhenVisible,
		View.bPredictionBadgeOffsetActive ? TEXT("true") : TEXT("false"),
		View.BadgeLayoutStaggerIndex,
		View.bTargetable ? TEXT("true") : TEXT("false"),
		*View.TargetDisabledReason.ToString(),
		*View.LastBindResult.ToString(),
		*View.LastCueKind.ToString(),
		static_cast<int32>(View.LastCueType),
		View.LastCueAmount,
		View.CuePlayCount,
		static_cast<int32>(View.DragPreviewState),
		View.bDragPreviewActive ? TEXT("true") : TEXT("false"),
		*View.LastDragPredictionDebugInput.SourceCardInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
		View.LastDragPredictionDebugInput.SourceCardRuntimeCost,
		View.LastDragPredictionDebugInput.bSourceCardSwift ? TEXT("true") : TEXT("false"),
		View.LastDragPredictionDebugInput.bPreviewCanSubmit ? TEXT("true") : TEXT("false"),
		*View.LastDragPredictionDebugInput.PreviewRejectReason.ToString(),
		View.bHoverActive ? TEXT("true") : TEXT("false"),
		*View.HoverReason.ToString(),
		*View.HoverStableId.ToString(),
		*View.HoverWorldTargetId.ToString(EGuidFormats::DigitsWithHyphens),
		*View.HoverScreenPosition.ToString(),
		View.PredictionView.bVisible ? TEXT("true") : TEXT("false"),
		static_cast<int32>(View.PredictionView.Mode),
		View.PredictionView.PredictedInitiative,
		View.PredictionView.bPerfectReleaseCandidate ? TEXT("true") : TEXT("false"),
		View.PredictionView.bActionRisk ? TEXT("true") : TEXT("false"),
		*View.PredictionView.RejectReason.ToString());
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::LogBattleWorldTargetDebugSummary() const
{
	UE_LOG(LogTemp, Display, TEXT("[WacomBattleEnemyPartWorldTarget] %s"), *GetBattleWorldTargetDebugSummary());
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearBattleBinding();
	StopFeedbackTimer();
	Super::EndPlay(EndPlayReason);
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::SetDragTargetPreviewState(
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

void UWacomBattleEnemyPartWorldTargetBridgeComponent::ClearDragTargetPreviewState()
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

void UWacomBattleEnemyPartWorldTargetBridgeComponent::SetHoverProbeState(
	const FWacomInteractionTargetHandle& TargetHandle,
	FName Reason,
	const FWacomBattleEnemyPartDragPredictionDebugInput& PredictionInput)
{
	if (TargetHandle.TargetKind != EWacomInteractionTargetKind::World
		|| !TargetHandle.TargetTag.MatchesTagExact(WacomTags::Interaction_Target_Battle_EnemyPart)
		|| !TargetHandle.WorldTargetId.IsValid())
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

void UWacomBattleEnemyPartWorldTargetBridgeComponent::ClearHoverProbeState(FName Reason)
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

void UWacomBattleEnemyPartWorldTargetBridgeComponent::SetPredictionWidgetComponent(
	UWidgetComponent* InPredictionWidgetComponent)
{
	PredictionWidgetComponent = InPredictionWidgetComponent;
	PredictionBadgeBaseRelativeLocation = InPredictionWidgetComponent
		? InPredictionWidgetComponent->GetRelativeLocation()
		: FVector::ZeroVector;
	bHasPredictionBadgeBaseRelativeLocation = InPredictionWidgetComponent != nullptr;
	ApplyPredictionViewToWidget();
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::SetStatusBadgeWidgetComponent(
	UWidgetComponent* InStatusBadgeWidgetComponent)
{
	StatusBadgeWidgetComponent = InStatusBadgeWidgetComponent;
	ApplyStatusBadgeViewToWidget();
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::SetBadgeLayoutDebugState(int32 InStaggerIndex)
{
	BadgeLayoutStaggerIndex = InStaggerIndex;
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::ClearPredictionDisplay(FName Reason)
{
	CurrentPredictionView = FWacomBattleEnemyPartPredictionView();
	CurrentPredictionView.RejectReason = Reason;
	ApplyPredictionViewToWidget();
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::ClearStatusBadgeDisplay(FName /*Reason*/)
{
	CurrentStatusBadgeView = FWacomBattleEnemyPartStatusBadgeView();
	ApplyStatusBadgeViewToWidget();
}

UWacomInteractionTargetComponent*
UWacomBattleEnemyPartWorldTargetBridgeComponent::ResolveInteractionTargetComponent() const
{
	AActor* Owner = GetOwner();
	return Owner ? Owner->FindComponentByClass<UWacomInteractionTargetComponent>() : nullptr;
}

UPrimitiveComponent*
UWacomBattleEnemyPartWorldTargetBridgeComponent::ResolveVisualTargetComponent() const
{
	if (VisualTargetComponent)
	{
		return VisualTargetComponent;
	}

	AActor* Owner = GetOwner();
	return Owner ? Owner->FindComponentByClass<UPrimitiveComponent>() : nullptr;
}

USceneComponent*
UWacomBattleEnemyPartWorldTargetBridgeComponent::ResolveFeedbackTargetComponent() const
{
	if (FeedbackTargetComponent)
	{
		return FeedbackTargetComponent;
	}

	return ResolveVisualTargetComponent();
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::CacheRuntimePartFacts(
	const FEnemyPartSnapshot& Part)
{
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

void UWacomBattleEnemyPartWorldTargetBridgeComponent::ClearRuntimePartFacts()
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

void UWacomBattleEnemyPartWorldTargetBridgeComponent::RegisterWithBattleHUD(UBattleHUD& HUD)
{
	if (!PartInstanceId.IsValid())
	{
		return;
	}

	if (RegisteredBattleHUD.Get() != &HUD)
	{
		UnregisterFromBattleHUD();
	}
	else
	{
		HUD.UnregisterBattlePresentationTargetsForOwner(this);
		bRegisteredWithBattleHUD = false;
	}

	TWeakObjectPtr<UWacomBattleEnemyPartWorldTargetBridgeComponent> WeakThis = this;
	HUD.RegisterBattlePresentationTarget(
		PartInstanceId,
		this,
		[WeakThis](const FWacomBattlePresentationTargetCue& Cue)
		{
			if (UWacomBattleEnemyPartWorldTargetBridgeComponent* StrongThis = WeakThis.Get())
			{
				StrongThis->PlayBattlePresentationCue(Cue);
			}
		});

	RegisteredBattleHUD = &HUD;
	bRegisteredWithBattleHUD = true;
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::UnregisterFromBattleHUD()
{
	if (UBattleHUD* HUD = RegisteredBattleHUD.Get())
	{
		HUD->UnregisterBattlePresentationTargetsForOwner(this);
	}
	RegisteredBattleHUD.Reset();
	bRegisteredWithBattleHUD = false;
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::ApplyTargetableAffordance(bool bInTargetable)
{
	if (bTargetable == bInTargetable)
	{
		return;
	}

	bTargetable = bInTargetable;
	ApplyPersistentScaleState();
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::RefreshPredictionDisplay()
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

void UWacomBattleEnemyPartWorldTargetBridgeComponent::RefreshStatusBadgeDisplay()
{
	if (!bEnableStatusBadgeDisplay || !bHasRuntimePartFacts)
	{
		ClearStatusBadgeDisplay(!bEnableStatusBadgeDisplay ? TEXT("Disabled") : TEXT("MissingRuntimeFacts"));
		return;
	}

	CurrentStatusBadgeView = BuildStatusBadgeView();
	ApplyStatusBadgeViewToWidget();
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::ApplyPredictionViewToWidget()
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

void UWacomBattleEnemyPartWorldTargetBridgeComponent::ApplyStatusBadgeViewToWidget()
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

FWacomBattleEnemyPartPredictionView UWacomBattleEnemyPartWorldTargetBridgeComponent::BuildPredictionView(
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

FWacomBattleEnemyPartStatusBadgeView UWacomBattleEnemyPartWorldTargetBridgeComponent::BuildStatusBadgeView() const
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

FText UWacomBattleEnemyPartWorldTargetBridgeComponent::BuildStatusBadgeStatusText() const
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

void UWacomBattleEnemyPartWorldTargetBridgeComponent::ApplyPersistentScaleState()
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

void UWacomBattleEnemyPartWorldTargetBridgeComponent::BeginScaleFeedback(float ScaleMultiplier, float HoldSeconds)
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
				&UWacomBattleEnemyPartWorldTargetBridgeComponent::ClearScaleFeedback,
				HoldSeconds,
				false);
		}
	}
}

#undef LOCTEXT_NAMESPACE

void UWacomBattleEnemyPartWorldTargetBridgeComponent::ClearScaleFeedback()
{
	StopFeedbackTimer();
	ApplyPersistentScaleState();
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::RestoreBaseScaleIfNeeded()
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

void UWacomBattleEnemyPartWorldTargetBridgeComponent::StopFeedbackTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FeedbackTimerHandle);
	}
	FeedbackTimerHandle = FTimerHandle();
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::UpdateInteractionTargetComponent()
{
	if (!bAutoConfigureInteractionTarget)
	{
		return;
	}

	if (UWacomInteractionTargetComponent* TargetComponent = ResolveInteractionTargetComponent())
	{
		TargetComponent->SetTargetId(PartInstanceId);
		TargetComponent->SetStableTargetId(PartId);
		TargetComponent->SetBattlePartSlotIdentity(
			BoundEncounterId,
			BoundEnemySlotId,
			BoundPartSlotId);
		TargetComponent->SetInteractionTargetTag(WacomTags::Interaction_Target_Battle_EnemyPart);
	}
}
