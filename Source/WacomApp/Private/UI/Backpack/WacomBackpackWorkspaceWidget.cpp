// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/InvalidationBox.h"
#include "Components/TextBlock.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "UI/Backpack/WacomBackpackWorkspaceLayoutSolver.h"
#include "UI/Backpack/WacomBackpackWorkspaceRuntime.h"
#include "UI/Backpack/WacomBackpackWorkspaceSceneBuilder.h"
#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"
#include "UI/Backpack/WacomBackpackZonePileWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "UI/Card/WacomCardMotionKernel.h"

#define LOCTEXT_NAMESPACE "WacomBackpackWorkspace"

namespace
{
constexpr float LayoutGeometryTolerance = 0.5f;
constexpr int32 RequiredStableLayoutSamples = 2;

FVector2D RotateVector(FVector2D Vector, float AngleDegrees)
{
	const float Radians = FMath::DegreesToRadians(AngleDegrees);
	const float CosAngle = FMath::Cos(Radians);
	const float SinAngle = FMath::Sin(Radians);
	return FVector2D(
		Vector.X * CosAngle - Vector.Y * SinAngle,
		Vector.X * SinAngle + Vector.Y * CosAngle);
}

bool ContainsPoint(const FSlateRect& Rect, FVector2D Point)
{
	return Point.X >= Rect.Left && Point.X <= Rect.Right
		&& Point.Y >= Rect.Top && Point.Y <= Rect.Bottom;
}

}

TSharedRef<SWidget> UWacomBackpackWorkspaceWidget::RebuildWidget()
{
	EnsureFallbackTree();
	SetIsFocusable(true);
	return Super::RebuildWidget();
}

void UWacomBackpackWorkspaceWidget::NativeConstruct()
{
	Super::NativeConstruct();
	GetRuntime();
	if (StaticCardLayer && !bHasStableLayoutSize)
	{
		StaticCardLayer->SetVisibility(ESlateVisibility::Hidden);
	}
	RequestLayoutGeometryRefresh();
	if (bDeferredCardFaceRenderRequested)
	{
		ScheduleBoundCardFaceRender();
	}
}

void UWacomBackpackWorkspaceWidget::NativeDestruct()
{
	StopCardMotionTimer();
	bLayoutGeometryRefreshActive = false;
	bDeferredCardFaceRenderRequested = false;
	bDeferredCardFaceRenderActive = false;
	CancelInteraction();
	ClearExpandedPileFocus(false);
	ExpandedPileFocus = FExpandedPileFocusState();
	for (TWeakObjectPtr<UWacomDeckCardWidget>& CardWidget : BoundCardWidgets)
	{
		if (CardWidget.IsValid())
		{
			CardWidget->UnbindWorkspacePointerEvents();
		}
	}
	BoundCardWidgets.Reset();
	if (Runtime)
	{
		Runtime->Reset(true);
		Runtime.Reset();
	}
	BaseCardLayouts.Reset();
	BaseCardLayoutTransitions.Reset();
	SettlementTargets.Reset();
	PendingReleasedVisualPoses.Reset();
	bPileCollapseAnimationPending = false;
	CancelHoverExpandTimer();
	Super::NativeDestruct();
}

void UWacomBackpackWorkspaceWidget::SetInteractionModel(
	TSharedPtr<FWacomBackpackWorkspaceInteractionModel> InModel,
	UWacomBackpackWorkspaceStyle* InStyle)
{
	InteractionModel = MoveTemp(InModel);
	InteractionStyle = InStyle;
}

void UWacomBackpackWorkspaceWidget::SetSimplifiedMotion(bool bSimplified)
{
	if (bSimplifiedMotion == bSimplified)
	{
		return;
	}
	bSimplifiedMotion = bSimplified;
	if (bSimplifiedMotion)
	{
		ApplyCarryVisualAnchor(0.0f);
		TickBaseCardLayoutTransitions(0.0f);
	}
	RefreshInteractionPresentation();
	StartCardMotionTimer();
}

FVector2D UWacomBackpackWorkspaceWidget::GetLayoutSpaceSize() const
{
	return bHasStableLayoutSize
		? StableLayoutSize
		: FVector2D(1280.0f, 720.0f);
}

void UWacomBackpackWorkspaceWidget::RequestLayoutGeometryRefresh()
{
	if (bLayoutGeometryRefreshActive)
	{
		return;
	}

	const TSharedPtr<SWidget> CachedWidget = GetCachedWidget();
	if (!CachedWidget.IsValid())
	{
		return;
	}

	PendingLayoutSize = FVector2D::ZeroVector;
	StableLayoutSampleCount = 0;
	bLayoutGeometryRefreshActive = true;
	const TWeakObjectPtr<UWacomBackpackWorkspaceWidget> WeakThis(this);
	CachedWidget->RegisterActiveTimer(
		0.0f,
		FWidgetActiveTimerDelegate::CreateLambda(
			[WeakThis](double CurrentTime, float DeltaTime)
			{
				UWacomBackpackWorkspaceWidget* Self = WeakThis.Get();
				if (!Self || !Self->bLayoutGeometryRefreshActive)
				{
					return EActiveTimerReturnType::Stop;
				}

				const FVector2D LayoutSize = Self->GetCachedGeometry().GetLocalSize();
				if (LayoutSize.X <= 1.0f || LayoutSize.Y <= 1.0f
					|| !FMath::IsFinite(LayoutSize.X) || !FMath::IsFinite(LayoutSize.Y))
				{
					return EActiveTimerReturnType::Continue;
				}

				if (!LayoutSize.Equals(Self->PendingLayoutSize, LayoutGeometryTolerance))
				{
					Self->PendingLayoutSize = LayoutSize;
					Self->StableLayoutSampleCount = 1;
					return EActiveTimerReturnType::Continue;
				}

				++Self->StableLayoutSampleCount;
				if (Self->StableLayoutSampleCount < RequiredStableLayoutSamples)
				{
					return EActiveTimerReturnType::Continue;
				}

				Self->bLayoutGeometryRefreshActive = false;
				Self->AcceptStableLayoutGeometry(LayoutSize);
				return EActiveTimerReturnType::Stop;
			}));
}

FVector2D UWacomBackpackWorkspaceWidget::ToLocalPointer(const FPointerEvent& Event) const
{
	return GetCachedGeometry().AbsoluteToLocal(Event.GetScreenSpacePosition());
}

void UWacomBackpackWorkspaceWidget::BindWorkspaceCards(
	TConstArrayView<TObjectPtr<UWacomDeckCardWidget>> CardWidgets,
	uint64 StorageRevision)
{
#if WITH_AUTOMATION_TESTS
	++WorkspaceSceneBindCount;
#endif
	CurrentStorageRevision = StorageRevision;
	for (TWeakObjectPtr<UWacomDeckCardWidget>& CardWidget : BoundCardWidgets)
	{
		if (CardWidget.IsValid())
		{
			CardWidget->UnbindWorkspacePointerEvents();
		}
	}
	BoundCardWidgets.Reset();
	TArray<FWacomBackpackWorkspaceCardHitRecord> HitRecords;
	TSet<UWacomDeckCardWidget*> VisibleWidgets;
	TSet<FGuid> VisibleInstanceIds;
	for (UWacomDeckCardWidget* CardWidget : CardWidgets)
	{
		if (!CardWidget)
		{
			continue;
		}
		CardWidget->OnWorkspacePointerDownNative.BindUObject(this, &UWacomBackpackWorkspaceWidget::HandleCardPointerDown);
		CardWidget->OnWorkspacePointerMoveNative.BindUObject(this, &UWacomBackpackWorkspaceWidget::HandleCardPointerMove);
		CardWidget->OnWorkspacePointerUpNative.BindUObject(this, &UWacomBackpackWorkspaceWidget::HandleCardPointerUp);
		CardWidget->SetBackpackCardFaceRetainedRenderingEnabled(bCardFaceRetainedRenderingEnabled);
		BoundCardWidgets.Add(CardWidget);

		FWacomBackpackWorkspaceCardHitRecord Hit;
		Hit.InstanceId = CardWidget->GetCardInstanceId();
		Hit.SourceZone = FWacomBackpackZoneKey::Make(
			CardWidget->GetWorkspaceDisplayZone(),
			CardWidget->GetWorkspaceDisplayOwnerInstanceId());
		// Collapsed pile cards deliberately do not accept direct pointer presses, but
		// their physical identities remain selectable by a pile-content marquee.
		// Read-only projections/owners/burdens stay outside the selection model.
		Hit.bMovable = CardWidget->IsWorkspaceSelectionEnabled();
		if (const FBaseCardLayout* Base = BaseCardLayouts.Find(CardWidget))
		{
			Hit.CardCenter = Base->Center;
			Hit.LayerRank = Base->ZOrder;
		}
		else if (const UCanvasPanelSlot* StaticCardSlot = Cast<UCanvasPanelSlot>(CardWidget->Slot))
		{
			Hit.CardCenter = StaticCardSlot->GetPosition() + StaticCardSlot->GetSize() * 0.5f;
			Hit.LayerRank = StaticCardSlot->GetZOrder();
		}
		HitRecords.Add(Hit);
		VisibleWidgets.Add(CardWidget);
		VisibleInstanceIds.Add(CardWidget->GetCardInstanceId());
	}
	for (auto It = BaseCardLayouts.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid() || !VisibleWidgets.Contains(It.Key().Get()))
		{
			It.RemoveCurrent();
		}
	}
	for (auto It = BaseCardLayoutTransitions.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid() || !VisibleWidgets.Contains(It.Key().Get()))
		{
			It.RemoveCurrent();
		}
	}
	for (auto It = PendingReleasedVisualHandoffs.CreateIterator(); It; ++It)
	{
		if (!VisibleInstanceIds.Contains(*It))
		{
			PendingReleasedVisualPoses.Remove(*It);
			It.RemoveCurrent();
		}
	}
	if (InteractionModel)
	{
		InteractionModel->ReconcileCards(HitRecords);
	}
	TArray<UPanelWidget*> VisualPanels;
	VisualPanels.Reserve(4);
	VisualPanels.Add(StaticCardLayer);
	VisualPanels.Add(CarryLayer);
	VisualPanels.Add(CarryActiveLayer);
	VisualPanels.Add(SettlementLayer);
	GetRuntime().Visuals.RebuildCardIndexes(
		VisualPanels,
		[this](const UWacomDeckCardWidget* Widget)
		{
			return ShouldPreserveCardParent(Widget);
		});
	SyncCarryLayer();
	RefreshInteractionPresentation();
	if (StaticCardLayer)
	{
		StaticCardLayer->SetVisibility(
			bHasStableLayoutSize
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Hidden);
	}
	if (bHasStableLayoutSize)
	{
		ScheduleBoundCardFaceRender();
	}
	RequestLayoutGeometryRefresh();
}

bool UWacomBackpackWorkspaceWidget::FindPileAtAbsolutePosition(
	FVector2D AbsolutePosition,
	EZoneKind& OutZone,
	FGuid& OutOwnerInstanceId) const
{
	const UWacomBackpackZonePileWidget* BestPile = nullptr;
	int32 BestZOrder = MIN_int32;
	for (const TWeakObjectPtr<UWacomBackpackZonePileWidget>& WeakPile : GetRegisteredPileWidgets())
	{
		const UWacomBackpackZonePileWidget* Pile = WeakPile.Get();
		if (Pile && Pile->GetVisibility() != ESlateVisibility::Collapsed
			&& Pile->GetCachedGeometry().IsUnderLocation(AbsolutePosition))
		{
			const UCanvasPanelSlot* PileCanvasSlot = Cast<UCanvasPanelSlot>(Pile->Slot);
			const int32 ZOrder = PileCanvasSlot ? PileCanvasSlot->GetZOrder() : 0;
			if (!BestPile || ZOrder >= BestZOrder)
			{
				BestPile = Pile;
				BestZOrder = ZOrder;
			}
		}
	}
	if (BestPile)
	{
		OutZone = BestPile->GetPileView().Zone;
		OutOwnerInstanceId = BestPile->GetPileView().OwnerInstanceId;
		return true;
	}
	if (bHasExpandedContentBounds)
	{
		const FVector2D Local = GetCachedGeometry().AbsoluteToLocal(AbsolutePosition);
		if (Local.X >= ExpandedContentBounds.Left && Local.X <= ExpandedContentBounds.Right
			&& Local.Y >= ExpandedContentBounds.Top && Local.Y <= ExpandedContentBounds.Bottom)
		{
			OutZone = ExpandedContentZone;
			OutOwnerInstanceId = ExpandedContentOwnerInstanceId;
			return true;
		}
	}
	return false;
}

void UWacomBackpackWorkspaceWidget::SetPileDropPreview(
	EZoneKind Zone,
	FGuid OwnerInstanceId,
	bool bVisible,
	bool bRejected)
{
	UWacomBackpackZonePileWidget* TargetPile = nullptr;
	for (const TWeakObjectPtr<UWacomBackpackZonePileWidget>& WeakPile : GetRegisteredPileWidgets())
	{
		UWacomBackpackZonePileWidget* Pile = WeakPile.Get();
		const bool bTarget = Pile
			&& Pile->GetPileView().HasSameIdentity(Zone, OwnerInstanceId);
		if (Pile)
		{
			Pile->SetDropPreviewState(bVisible && bTarget, bRejected);
		}
		if (bTarget)
		{
			TargetPile = Pile;
		}
	}
	if (!bVisible || bRejected || !TargetPile || TargetPile->GetPileView().bExpanded
		|| !InteractionModel || !InteractionModel->IsCarrying())
	{
		CancelHoverExpandTimer();
		return;
	}
	if (bHoverExpandTimerActive && HoverExpandZone == Zone
		&& (Zone != EZoneKind::SpecialZone || HoverExpandOwnerInstanceId == OwnerInstanceId))
	{
		return;
	}
	CancelHoverExpandTimer();
	bHoverExpandTimerActive = true;
	HoverExpandElapsedSeconds = 0.0f;
	HoverExpandZone = Zone;
	HoverExpandOwnerInstanceId = Zone == EZoneKind::SpecialZone ? OwnerInstanceId : FGuid();
	StartCardMotionTimer();
}

void UWacomBackpackWorkspaceWidget::CancelHoverExpandTimer()
{
	bHoverExpandTimerActive = false;
	HoverExpandElapsedSeconds = 0.0f;
	HoverExpandZone = EZoneKind::Backpack;
	HoverExpandOwnerInstanceId.Invalidate();
}

void UWacomBackpackWorkspaceWidget::SetExpandedContentBounds(
	EZoneKind Zone,
	FGuid OwnerInstanceId,
	const FSlateRect& LocalBounds)
{
	ExpandedContentZone = Zone;
	ExpandedContentOwnerInstanceId = Zone == EZoneKind::SpecialZone ? OwnerInstanceId : FGuid();
	ExpandedContentBounds = LocalBounds;
	bHasExpandedContentBounds = Zone != EZoneKind::Backpack;
}

void UWacomBackpackWorkspaceWidget::SetExpandedPileFocusContract(
	EZoneKind Zone,
	FGuid OwnerInstanceId,
	const FSlateRect& HeaderRect,
	const FSlateRect& FocusCorridorRect,
	TConstArrayView<FWacomBackpackExpandedPileFocusCard> Cards)
{
	const FGuid NormalizedOwner = Zone == EZoneKind::SpecialZone ? OwnerInstanceId : FGuid();
	bool bSameIdentity = ExpandedPileFocus.Zone == Zone
		&& ExpandedPileFocus.OwnerInstanceId == NormalizedOwner
		&& ExpandedPileFocus.Cards.Num() == Cards.Num();
	if (bSameIdentity)
	{
		for (int32 Index = 0; Index < Cards.Num(); ++Index)
		{
			if (ExpandedPileFocus.Cards[Index].Card != Cards[Index].Card)
			{
				bSameIdentity = false;
				break;
			}
		}
	}
	if (!bSameIdentity)
	{
		ClearExpandedPileFocus(false);
	}
	ExpandedPileFocus.Zone = Zone;
	ExpandedPileFocus.OwnerInstanceId = NormalizedOwner;
	ExpandedPileFocus.HeaderRect = HeaderRect;
	ExpandedPileFocus.CorridorRect = FocusCorridorRect;
	ExpandedPileFocus.Cards.Reset(Cards.Num());
	ExpandedPileFocus.Cards.Append(Cards.GetData(), Cards.Num());
	if (!bSameIdentity || !ExpandedPileFocus.Cards.IsValidIndex(ExpandedPileFocus.FocusIndex))
	{
		ExpandedPileFocus.FocusIndex = INDEX_NONE;
		ExpandedPileFocusTargets.Reset();
	}
	else if (ExpandedPileFocus.FocusIndex != INDEX_NONE)
	{
		RebuildExpandedPileFocusLayout();
	}
}

bool UWacomBackpackWorkspaceWidget::BeginPileCollapseAnimation(
	EZoneKind Zone,
	FGuid OwnerInstanceId)
{
	if (bSimplifiedMotion || bPileCollapseAnimationPending || !bHasExpandedContentBounds
		|| ExpandedContentZone != Zone
		|| (Zone == EZoneKind::SpecialZone && ExpandedContentOwnerInstanceId != OwnerInstanceId))
	{
		return false;
	}
	UWacomBackpackZonePileWidget* TargetPile = nullptr;
	for (const TWeakObjectPtr<UWacomBackpackZonePileWidget>& WeakPile : GetRegisteredPileWidgets())
	{
		UWacomBackpackZonePileWidget* Pile = WeakPile.Get();
		if (Pile && Pile->GetPileView().HasSameIdentity(Zone, OwnerInstanceId))
		{
			TargetPile = Pile;
			break;
		}
	}
	const UCanvasPanelSlot* PileCanvasSlot = TargetPile
		? Cast<UCanvasPanelSlot>(TargetPile->Slot)
		: nullptr;
	if (!PileCanvasSlot)
	{
		return false;
	}
	const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
		? InteractionStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	const FSlateRect TargetHeader = TargetPile->GetResolvedHeaderRect();
	TArray<UWacomDeckCardWidget*> CollapsingCards;
	for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard : BoundCardWidgets)
	{
		UWacomDeckCardWidget* Card = WeakCard.Get();
		if (Card
			&& FWacomBackpackZoneKey::Make(
				Card->GetWorkspaceDisplayZone(),
				Card->GetWorkspaceDisplayOwnerInstanceId())
				== FWacomBackpackZoneKey::Make(Zone, OwnerInstanceId))
		{
			CollapsingCards.Add(Card);
		}
	}
	const FVector2D HeaderTopLeft(TargetHeader.Left, TargetHeader.Top);
	const FVector2D HeaderSize(
		TargetHeader.Right - TargetHeader.Left,
		TargetHeader.Bottom - TargetHeader.Top);
	const FWacomBackpackResolvedPileContentLayout CollapsedLayout =
		FWacomBackpackWorkspaceLayoutSolver::BuildPileContentLayout(
			CollapsingCards.Num(),
			HeaderTopLeft,
			HeaderSize,
			GetLayoutSpaceSize(),
			Style->CardRenderSize,
			false,
			Style->PileCollapsedExposurePixels,
			Style->AdaptiveStripExposurePixels,
			Style->AdaptiveStripFocusSeparationPixels,
			Style->PileEdgeMarginPixels);
	bool bAnimatedAny = false;
	for (int32 CardIndex = 0; CardIndex < CollapsingCards.Num(); ++CardIndex)
	{
		UWacomDeckCardWidget* Card = CollapsingCards[CardIndex];
		const FBaseCardLayout* Base = Card ? BaseCardLayouts.Find(Card) : nullptr;
		if (!Card || !Base || !CollapsedLayout.Cards.IsValidIndex(CardIndex))
		{
			continue;
		}
		ApplyCardBaseLayout(
			*Card,
			CollapsedLayout.Cards[CardIndex].CardCenter,
			Base->Size,
			CollapsedLayout.Cards[CardIndex].AngleDegrees,
			Base->ZOrder);
		bAnimatedAny = true;
	}
	RefreshInteractionPresentation();
	if (!bAnimatedAny || BaseCardLayoutTransitions.IsEmpty()
		|| Style->PileExpandSeconds <= 0.0f)
	{
		return false;
	}
	bPileCollapseAnimationPending = true;
	CollapsingPileZone = Zone;
	CollapsingPileOwnerInstanceId = Zone == EZoneKind::SpecialZone ? OwnerInstanceId : FGuid();
	bCarryInputSuspended = true;
	StartCardMotionTimer();
	return true;
}

void UWacomBackpackWorkspaceWidget::SetHoveredCard(UWacomDeckCardWidget* CardWidget)
{
	HoveredCardWidget = CardWidget;
	RefreshInteractionPresentation();
	StartCardMotionTimer();
}

void UWacomBackpackWorkspaceWidget::ClearHoveredCard(UWacomDeckCardWidget* CardWidget)
{
	if (HoveredCardWidget.Get() == CardWidget)
	{
		HoveredCardWidget.Reset();
		RefreshInteractionPresentation();
		StartCardMotionTimer();
	}
}

bool UWacomBackpackWorkspaceWidget::IsExpandedPileFocusAllowed() const
{
	return InteractionModel
		&& InteractionModel->GetMode() == EWacomBackpackWorkspaceInteractionMode::Idle
		&& InteractionModel->GetSelection().OrderedSelectedInstanceIds.IsEmpty()
		&& !bCarryInputSuspended
		&& !bPileCollapseAnimationPending
		&& ExpandedPileFocus.Zone != EZoneKind::Backpack
		&& !ExpandedPileFocus.Cards.IsEmpty();
}

bool UWacomBackpackWorkspaceWidget::IsExpandedPileFocusCard(
	const UWacomDeckCardWidget* CardWidget,
	int32* OutIndex) const
{
	for (int32 Index = 0; Index < ExpandedPileFocus.Cards.Num(); ++Index)
	{
		if (ExpandedPileFocus.Cards[Index].Card.Get() == CardWidget)
		{
			if (OutIndex)
			{
				*OutIndex = Index;
			}
			return true;
		}
	}
	return false;
}

UWacomDeckCardWidget* UWacomBackpackWorkspaceWidget::GetPresentationFocusedCard() const
{
	if (ExpandedPileFocus.Cards.IsValidIndex(ExpandedPileFocus.FocusIndex))
	{
		return ExpandedPileFocus.Cards[ExpandedPileFocus.FocusIndex].Card.Get();
	}
	return HoveredCardWidget.Get();
}

void UWacomBackpackWorkspaceWidget::UpdateExpandedPileFocus(FVector2D PointerLocal)
{
	ExpandedPileFocus.PointerLocal = PointerLocal;
	if (!IsExpandedPileFocusAllowed())
	{
		ClearExpandedPileFocus(true);
		return;
	}
	// The title/drag handle owns this rectangle even while a focused card is
	// visually returning from its lift. Clear immediately so the card cannot
	// keep the header hidden or steal the next click.
	if (ContainsPoint(ExpandedPileFocus.HeaderRect, PointerLocal))
	{
		ClearExpandedPileFocus(false);
		return;
	}
	const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
		? InteractionStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	int32 HitIndex = INDEX_NONE;
	if (ExpandedPileFocus.Cards.IsValidIndex(ExpandedPileFocus.FocusIndex))
	{
		FSlateRect StickyBand = ExpandedPileFocus.Cards[
			ExpandedPileFocus.FocusIndex].CurrentHitBand;
		const float Hysteresis = FMath::Max(0.0f, Style->FocusHitHysteresisPixels);
		StickyBand.Left -= Hysteresis;
		StickyBand.Right += Hysteresis;
		if (ContainsPoint(StickyBand, PointerLocal))
		{
			HitIndex = ExpandedPileFocus.FocusIndex;
		}
	}
	if (HitIndex == INDEX_NONE)
	{
		for (int32 Index = 0; Index < ExpandedPileFocus.Cards.Num(); ++Index)
		{
			if (ContainsPoint(ExpandedPileFocus.Cards[Index].CurrentHitBand, PointerLocal))
			{
				HitIndex = Index;
				break;
			}
		}
	}
	if (HitIndex != INDEX_NONE)
	{
		ExpandedPileFocus.bExitPending = false;
		ExpandedPileFocus.ExitDelayRemainingSeconds = 0.0f;
		SetExpandedPileFocusIndex(HitIndex);
		GetRuntime().Motion.UpdatePointer(GetCachedGeometry(), PointerLocal, false);
		StartCardMotionTimer();
	}
	else
	{
		BeginExpandedPileFocusExit();
	}
}

void UWacomBackpackWorkspaceWidget::BeginExpandedPileFocusExit()
{
	if (ExpandedPileFocus.FocusIndex == INDEX_NONE || ExpandedPileFocus.bExitPending)
	{
		return;
	}
	const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
		? InteractionStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	if (bSimplifiedMotion || Style->FocusExitDelaySeconds <= 0.0f)
	{
		ClearExpandedPileFocus(true);
		return;
	}
	ExpandedPileFocus.bExitPending = true;
	ExpandedPileFocus.ExitDelayRemainingSeconds = Style->FocusExitDelaySeconds;
	StartCardMotionTimer();
}

void UWacomBackpackWorkspaceWidget::TickExpandedPileFocusExit(float DeltaTime)
{
	if (!ExpandedPileFocus.bExitPending)
	{
		return;
	}
	ExpandedPileFocus.ExitDelayRemainingSeconds -= FMath::Max(0.0f, DeltaTime);
	if (ExpandedPileFocus.ExitDelayRemainingSeconds <= 0.0f)
	{
		ClearExpandedPileFocus(true);
	}
}

void UWacomBackpackWorkspaceWidget::SetExpandedPileFocusIndex(int32 FocusIndex)
{
	if (!ExpandedPileFocus.Cards.IsValidIndex(FocusIndex)
		|| ExpandedPileFocus.FocusIndex == FocusIndex)
	{
		return;
	}
	ExpandedPileFocus.FocusIndex = FocusIndex;
	ExpandedPileFocus.bExitPending = false;
	ExpandedPileFocus.ExitDelayRemainingSeconds = 0.0f;
	RebuildExpandedPileFocusLayout();
	OnBrowseFocusChangedNative.Broadcast(ExpandedPileFocus.Cards[FocusIndex].Card.Get());
	RefreshInteractionPresentation();
	StartCardMotionTimer();
}

void UWacomBackpackWorkspaceWidget::RebuildExpandedPileFocusLayout()
{
	ExpandedPileFocusTargets.Reset();
	if (!ExpandedPileFocus.Cards.IsValidIndex(ExpandedPileFocus.FocusIndex))
	{
		return;
	}
	const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
		? InteractionStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	TArray<FWacomBackpackResolvedLayout> NeutralLayouts;
	NeutralLayouts.Reserve(ExpandedPileFocus.Cards.Num());
	for (const FWacomBackpackExpandedPileFocusCard& Card : ExpandedPileFocus.Cards)
	{
		FWacomBackpackResolvedLayout& Neutral = NeutralLayouts.AddDefaulted_GetRef();
		Neutral.CardCenter = Card.NeutralCenter;
		Neutral.AngleDegrees = Card.NeutralAngleDegrees;
		Neutral.LayerRank = Card.NeutralLayerRank;
	}
	const FWacomBackpackAdaptiveStripLayout Layout =
		FWacomBackpackWorkspaceLayoutSolver::BuildAdaptiveStripLayout(
			NeutralLayouts.Num(),
			ExpandedPileFocus.FocusIndex,
			ExpandedPileFocus.CorridorRect,
			Style->CardRenderSize,
			NeutralLayouts,
			Style->AdaptiveStripExposurePixels,
			Style->AdaptiveStripFocusSeparationPixels);
	for (int32 Index = 0; Index < Layout.Cards.Num(); ++Index)
	{
		UWacomDeckCardWidget* Card = ExpandedPileFocus.Cards[Index].Card.Get();
		if (!Card)
		{
			continue;
		}
		FBaseCardLayout& Target = ExpandedPileFocusTargets.Add(Card);
		Target.Center = Layout.Cards[Index].CardCenter;
		Target.Size = Style->CardRenderSize;
		Target.AngleDegrees = Layout.Cards[Index].AngleDegrees;
		Target.ZOrder = 6000 + Layout.Cards[Index].LayerRank;
		if (Layout.HitBands.IsValidIndex(Index))
		{
			ExpandedPileFocus.Cards[Index].CurrentHitBand = Layout.HitBands[Index];
			if (Index == ExpandedPileFocus.FocusIndex && !bSimplifiedMotion)
			{
				ExpandedPileFocus.Cards[Index].CurrentHitBand.Top -=
					Style->ExpandedCardHoverLiftPixels;
				ExpandedPileFocus.Cards[Index].CurrentHitBand.Bottom -=
					Style->ExpandedCardHoverLiftPixels;
			}
		}
	}
	++ExpandedPileFocusLayoutRebuildCount;
	SyncExpandedPileHitLayouts(true);
}

void UWacomBackpackWorkspaceWidget::SyncExpandedPileHitLayouts(bool bUseFocusedTargets)
{
	if (!InteractionModel || ExpandedPileFocus.Cards.IsEmpty())
	{
		return;
	}
	const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
		? InteractionStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	const FWacomBackpackZoneKey SourceZone = FWacomBackpackZoneKey::Make(
		ExpandedPileFocus.Zone,
		ExpandedPileFocus.OwnerInstanceId);
	TArray<FWacomBackpackWorkspaceCardHitRecord> Updates;
	Updates.Reserve(ExpandedPileFocus.Cards.Num());
	for (int32 Index = 0; Index < ExpandedPileFocus.Cards.Num(); ++Index)
	{
		const FWacomBackpackExpandedPileFocusCard& Entry = ExpandedPileFocus.Cards[Index];
		UWacomDeckCardWidget* Card = Entry.Card.Get();
		if (!Card)
		{
			continue;
		}
		FVector2D Center = Entry.NeutralCenter;
		int32 LayerRank = Entry.NeutralLayerRank;
		if (bUseFocusedTargets)
		{
			if (const FBaseCardLayout* FocusTarget = ExpandedPileFocusTargets.Find(Card))
			{
				Center = FocusTarget->Center;
				LayerRank = FocusTarget->ZOrder;
				if (Index == ExpandedPileFocus.FocusIndex && !bSimplifiedMotion)
				{
					Center.Y -= Style->ExpandedCardHoverLiftPixels;
				}
			}
		}
		Updates.Emplace(
			Card->GetCardInstanceId(),
			SourceZone,
			Center,
			LayerRank,
			Card->IsWorkspaceInteractionEnabled());
	}
	InteractionModel->UpdateCardHitLayouts(Updates);
}

void UWacomBackpackWorkspaceWidget::ClearExpandedPileFocus(
	bool bAnimateReturn,
	bool bBroadcastChange)
{
	const bool bHadFocus = ExpandedPileFocus.FocusIndex != INDEX_NONE;
	ExpandedPileFocus.FocusIndex = INDEX_NONE;
	ExpandedPileFocus.bExitPending = false;
	ExpandedPileFocus.ExitDelayRemainingSeconds = 0.0f;
	ExpandedPileFocusTargets.Reset();
	for (FWacomBackpackExpandedPileFocusCard& Entry : ExpandedPileFocus.Cards)
	{
		Entry.CurrentHitBand = Entry.NeutralHitBand;
	}
	SyncExpandedPileHitLayouts(false);
	if (Runtime)
	{
		const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
			? InteractionStyle.Get()
			: GetDefault<UWacomBackpackWorkspaceStyle>();
		for (const FWacomBackpackExpandedPileFocusCard& Entry : ExpandedPileFocus.Cards)
		{
			if (UWacomDeckCardWidget* Card = Entry.Card.Get())
			{
				GetRuntime().Motion.SetLocalPoseTarget(
					*Card,
					FVector2D::ZeroVector,
					0.0f,
					bAnimateReturn ? Style->FocusReturnSeconds : 0.0f,
					bSimplifiedMotion || !bAnimateReturn);
			}
		}
	}
	if (bHadFocus && bBroadcastChange)
	{
		OnBrowseFocusChangedNative.Broadcast(nullptr);
	}
	if (bHadFocus)
	{
		RefreshInteractionPresentation();
		StartCardMotionTimer();
	}
}

void UWacomBackpackWorkspaceWidget::SetCardFaceRetainedRenderingEnabled(bool bEnabled)
{
	bCardFaceRetainedRenderingEnabled = bEnabled;
	for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCardWidget : BoundCardWidgets)
	{
		if (UWacomDeckCardWidget* CardWidget = WeakCardWidget.Get())
		{
			CardWidget->SetBackpackCardFaceRetainedRenderingEnabled(bEnabled);
		}
	}
}

FReply UWacomBackpackWorkspaceWidget::HandleCardPointerDown(
	UWacomDeckCardWidget* CardWidget,
	const FGeometry& Geometry,
	const FPointerEvent& Event)
{
	if (!InteractionModel || !CardWidget || bCarryInputSuspended)
	{
		return FReply::Unhandled();
	}
	if (InteractionModel->IsCarrying()
		&& (Event.GetEffectingButton() == EKeys::LeftMouseButton
			|| Event.GetEffectingButton() == EKeys::RightMouseButton))
	{
		InteractionModel->NotifyReleaseGestureStarted();
		return FReply::Handled().CaptureMouse(TakeWidget());
	}
	if (Event.GetEffectingButton() == EKeys::RightMouseButton)
	{
		return CardWidget->RequestBattleEnabledToggle() ? FReply::Handled() : FReply::Unhandled();
	}
	const FVector2D Pointer = ToLocalPointer(Event);
	if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		// Static/settling cards render above the pile frame. A lifted or freshly
		// released card can therefore receive the Slate pointer event even though
		// the pointer is inside a pile header. The header owns that semantic region:
		// reroute before card pickup so collapse and title dragging remain reachable.
		if (TryBeginPileHeaderPress(Pointer, Event.IsControlDown()))
		{
			return FReply::Handled().CaptureMouse(TakeWidget()).SetUserFocus(TakeWidget());
		}
	}
	if (Event.GetEffectingButton() != EKeys::LeftMouseButton || !CardWidget->IsMoveEnabled())
	{
		return FReply::Unhandled();
	}
	if (!Event.IsControlDown())
	{
		const bool bStarted = InteractionModel->BeginCarry(
			CardWidget->GetCardInstanceId(),
			Pointer,
			CurrentStorageRevision);
		if (bStarted)
		{
			bCarryCurrentExplicitlySelectedByWheel = false;
			bPendingCardPress = false;
			ClearExpandedPileFocus(true);
			UpdateCarryAnchor(Pointer);
			bCarryStripLayoutDirty = true;
			SyncCarryLayer();
			RebuildCarryStripLayout();
			BeginCarryPickupFeedback();
			StartCardMotionTimer();
			RefreshInteractionPresentation();
			OnInteractionChangedNative.Broadcast();
			return BuildHandledPointerReply();
		}
	}
	ClearExpandedPileFocus(true);
	bPendingCardPress = true;
	InteractionModel->SetCardPressActive(true);
	PendingCardPressId = CardWidget->GetCardInstanceId();
	PendingPressPosition = Pointer;
	bPendingControlDown = Event.IsControlDown();
	return FReply::Handled().CaptureMouse(TakeWidget());
}

FReply UWacomBackpackWorkspaceWidget::HandleCardPointerMove(
	UWacomDeckCardWidget* CardWidget,
	const FGeometry& Geometry,
	const FPointerEvent& Event)
{
	if (!InteractionModel || bCarryInputSuspended)
	{
		return FReply::Unhandled();
	}
	const FVector2D Pointer = ToLocalPointer(Event);
	if (InteractionModel->IsMarqueeActive())
	{
		InteractionModel->UpdateMarquee(Pointer);
		RefreshInteractionPresentation();
		return BuildHandledPointerReply();
	}
	if (InteractionModel->IsCarrying())
	{
		QueueCarryPointer(Pointer);
		return BuildHandledPointerReply();
	}
	if (TryBeginCarryFromPendingPress(Pointer))
	{
		return BuildHandledPointerReply();
	}
	UpdateExpandedPileFocus(Pointer);
	if (GetPresentationFocusedCard())
	{
		GetRuntime().Motion.UpdatePointer(GetCachedGeometry(), Pointer, false);
		StartCardMotionTimer();
	}
	return BuildHandledPointerReply();
}

FReply UWacomBackpackWorkspaceWidget::HandleCardPointerUp(
	UWacomDeckCardWidget* CardWidget,
	const FGeometry& Geometry,
	const FPointerEvent& Event)
{
	if (!InteractionModel || bCarryInputSuspended)
	{
		return FReply::Unhandled();
	}
	if (InteractionModel->IsMarqueeActive() && Event.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		InteractionModel->UpdateMarquee(ToLocalPointer(Event));
		InteractionModel->CompleteMarquee();
		ClearExpandedPileFocus(true);
		RefreshInteractionPresentation();
		OnInteractionChangedNative.Broadcast();
		return FReply::Handled().ReleaseMouseCapture();
	}
	if (InteractionModel->IsCarrying())
	{
		SyncCarryPointerForRelease(ToLocalPointer(Event));
		BroadcastRelease(Event.GetEffectingButton() == EKeys::RightMouseButton);
		return BuildHandledPointerReply();
	}
	if (bPendingCardPress && Event.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		InteractionModel->ClickCard(PendingCardPressId, bPendingControlDown);
		InteractionModel->SetCardPressActive(false);
		bPendingCardPress = false;
		RefreshInteractionPresentation();
		OnInteractionChangedNative.Broadcast();
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}

void UWacomBackpackWorkspaceWidget::BroadcastRelease(bool bReleaseAll)
{
	if (!InteractionModel)
	{
		return;
	}
	const FWacomBackpackWorkspaceReleaseIntent Intent = InteractionModel->BuildReleaseIntent(bReleaseAll);
	const bool bIssuedRelease = !Intent.bConsumedByInitialReleaseGuard && !Intent.InstanceIds.IsEmpty();
	if (bIssuedRelease)
	{
		CaptureReleasedVisualPoses(Intent.InstanceIds);
		for (const FGuid InstanceId : Intent.InstanceIds)
		{
			PendingReleasedVisualHandoffs.Add(InstanceId);
		}
		OnReleaseIntentNative.Broadcast(Intent);
		// Atomic rejection and delete-confirm suspension keep the instances in the
		// carry model. They never entered a target handoff, so do not retain a stale
		// pending marker after the coordinator returns.
		if (InteractionModel->IsCarrying())
		{
			const TArray<FGuid>& Remaining = InteractionModel->GetCarry().RemainingInstanceIds;
			for (const FGuid InstanceId : Intent.InstanceIds)
			{
				if (Remaining.Contains(InstanceId))
				{
					PendingReleasedVisualHandoffs.Remove(InstanceId);
					PendingReleasedVisualPoses.Remove(InstanceId);
				}
			}
		}
	}
	RefreshInteractionPresentation();
	StartCardMotionTimer();
	OnInteractionChangedNative.Broadcast();
}

bool UWacomBackpackWorkspaceWidget::TryBeginCarryFromPendingPress(FVector2D Pointer)
{
	if (!InteractionModel || !bPendingCardPress
		|| FVector2D::Distance(PendingPressPosition, Pointer) < 5.0f)
	{
		return false;
	}

	if (!InteractionModel->IsSelected(PendingCardPressId))
	{
		InteractionModel->ClickCard(PendingCardPressId, false);
	}
	const bool bStarted = InteractionModel->BeginCarry(
		PendingCardPressId,
		Pointer,
		CurrentStorageRevision);
	bPendingCardPress = false;
	InteractionModel->SetCardPressActive(false);
	if (!bStarted)
	{
		return false;
	}

	bCarryCurrentExplicitlySelectedByWheel = false;
	ClearExpandedPileFocus(true);
	UpdateCarryAnchor(Pointer);
	bCarryStripLayoutDirty = true;
	SyncCarryLayer();
	RebuildCarryStripLayout();
	BeginCarryPickupFeedback();
	StartCardMotionTimer();
	RefreshInteractionPresentation();
	OnInteractionChangedNative.Broadcast();
	return true;
}

FReply UWacomBackpackWorkspaceWidget::HandlePilePointerDown(
	UWacomBackpackZonePileWidget* PileWidget,
	const FGeometry& Geometry,
	const FPointerEvent& Event)
{
	if (!InteractionModel || !PileWidget || bCarryInputSuspended)
	{
		return FReply::Unhandled();
	}
	if (InteractionModel->IsCarrying())
	{
		InteractionModel->NotifyReleaseGestureStarted();
		return FReply::Handled().CaptureMouse(TakeWidget()).SetUserFocus(TakeWidget());
	}
	if (Event.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}
	BeginPendingPilePress(
		*PileWidget,
		ToLocalPointer(Event),
		Event.IsControlDown(),
		PileWidget->WasLastPointerDownOnDragHandle());
	return FReply::Handled().CaptureMouse(TakeWidget()).SetUserFocus(TakeWidget());
}

void UWacomBackpackWorkspaceWidget::BeginPendingPilePress(
	UWacomBackpackZonePileWidget& PileWidget,
	FVector2D LocalPointer,
	bool bControlDown,
	bool bOnDragHandle)
{
	bPendingPilePress = true;
	PendingPileWidget = &PileWidget;
	PendingPilePressPosition = LocalPointer;
	bPendingPileControlDown = bControlDown;
	bPendingPilePressOnDragHandle = bOnDragHandle;
	const FSlateRect HeaderRect = PileWidget.GetResolvedHeaderRect();
	PendingPileStartPosition = FVector2D(HeaderRect.Left, HeaderRect.Top);
}

UWacomBackpackZonePileWidget* UWacomBackpackWorkspaceWidget::FindPileHeaderAt(
	FVector2D LocalPointer) const
{
	UWacomBackpackZonePileWidget* BestPile = nullptr;
	int32 BestLayerRank = MIN_int32;
	for (const TWeakObjectPtr<UWacomBackpackZonePileWidget>& WeakPile : GetRegisteredPileWidgets())
	{
		UWacomBackpackZonePileWidget* Pile = WeakPile.Get();
		if (!Pile || !ContainsPoint(Pile->GetResolvedHeaderRect(), LocalPointer))
		{
			continue;
		}
		const UCanvasPanelSlot* PileSlot = Cast<UCanvasPanelSlot>(Pile->Slot);
		const int32 LayerRank = PileSlot ? PileSlot->GetZOrder() : 0;
		if (!BestPile || LayerRank > BestLayerRank)
		{
			BestPile = Pile;
			BestLayerRank = LayerRank;
		}
	}
	return BestPile;
}

bool UWacomBackpackWorkspaceWidget::TryBeginPileHeaderPress(
	FVector2D LocalPointer,
	bool bControlDown)
{
	UWacomBackpackZonePileWidget* HeaderPile = FindPileHeaderAt(LocalPointer);
	if (!HeaderPile)
	{
		return false;
	}
	ClearExpandedPileFocus(true);
	BeginPendingPilePress(*HeaderPile, LocalPointer, bControlDown, true);
	return true;
}

bool UWacomBackpackWorkspaceWidget::TryBeginPileMove(FVector2D Pointer)
{
	UWacomBackpackZonePileWidget* Pile = PendingPileWidget.Get();
	if (!InteractionModel || !bPendingPilePress || !Pile
		|| !Pile->GetPileView().bMovable
		|| !bPendingPilePressOnDragHandle
		|| FVector2D::Distance(PendingPilePressPosition, Pointer) < 5.0f)
	{
		return false;
	}
	const FWacomBackpackZoneKey Zone = FWacomBackpackZoneKey::Make(
		Pile->GetPileView().Zone, Pile->GetPileView().OwnerInstanceId);
	ClearExpandedPileFocus(true);
	if (!InteractionModel->BeginPileMove(Zone, PendingPilePressPosition, PendingPileStartPosition))
	{
		return false;
	}
	bPendingPilePress = false;
	bPendingPileControlDown = false;
	bPendingPilePressOnDragHandle = false;
	QueuePilePointer(Pointer);
	FlushQueuedPilePointer();
	StartPilePointerTracking();
	OnInteractionChangedNative.Broadcast();
	return true;
}

bool UWacomBackpackWorkspaceWidget::TryBeginMarqueeFromPendingPilePress(FVector2D Pointer)
{
	UWacomBackpackZonePileWidget* Pile = PendingPileWidget.Get();
	if (!InteractionModel || !bPendingPilePress || !Pile
		|| bPendingPilePressOnDragHandle
		|| FVector2D::Distance(PendingPilePressPosition, Pointer) < 5.0f)
	{
		return false;
	}

	const FWacomBackpackZoneKey SourceZone = FWacomBackpackZoneKey::Make(
		Pile->GetPileView().Zone,
		Pile->GetPileView().OwnerInstanceId);
	InteractionModel->BeginMarquee(
		SourceZone,
		PendingPilePressPosition,
		bPendingPileControlDown);
	InteractionModel->UpdateMarquee(Pointer);
	bPendingPilePress = false;
	bPendingPileControlDown = false;
	bPendingPilePressOnDragHandle = false;
	PendingPileWidget.Reset();
	RefreshInteractionPresentation();
	return InteractionModel->IsMarqueeActive();
}

void UWacomBackpackWorkspaceWidget::ApplyActivePileMove()
{
	if (!InteractionModel || !InteractionModel->IsPileMoving())
	{
		return;
	}
	const FWacomBackpackWorkspacePileMoveState& Move = InteractionModel->GetPileMove();
	const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
		? InteractionStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	const FVector2D ClampedPosition = FWacomBackpackWorkspaceLayoutSolver::SnapPileTopLeft(
		Move.CurrentPosition,
		GetLayoutSpaceSize(),
		FVector2D(FMath::Max(260.0f, Style->PileCollapsedSize.X), 48.0f),
		1.0f,
		Style->PileEdgeMarginPixels);
	for (const TWeakObjectPtr<UWacomBackpackZonePileWidget>& WeakPile : GetRegisteredPileWidgets())
	{
		UWacomBackpackZonePileWidget* Pile = WeakPile.Get();
		if (Pile && Pile->GetPileView().HasSameIdentity(Move.Zone.Zone, Move.Zone.OwnerInstanceId))
		{
			const FSlateRect Frame = Pile->GetResolvedFrameRect();
			const FSlateRect Header = Pile->GetResolvedHeaderRect();
			const FVector2D FrameOffset(Frame.Left - Header.Left, Frame.Top - Header.Top);
			if (UCanvasPanelSlot* PileCanvasSlot = Cast<UCanvasPanelSlot>(Pile->Slot))
			{
				PileCanvasSlot->SetPosition(ClampedPosition + FrameOffset);
				PileCanvasSlot->SetZOrder(9000);
			}
			const FVector2D Delta = ClampedPosition - PendingPileStartPosition;
			for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard : BoundCardWidgets)
			{
				UWacomDeckCardWidget* Card = WeakCard.Get();
				const FBaseCardLayout* Base = Card ? BaseCardLayouts.Find(Card) : nullptr;
				if (Card && Base && Card->GetParent() != CarryLayer
					&& FWacomBackpackZoneKey::Make(
						Card->GetWorkspaceDisplayZone(),
						Card->GetWorkspaceDisplayOwnerInstanceId()) == Move.Zone)
				{
					ApplyCardLayout(
						*Card, Base->Center + Delta, Base->Size,
						Base->AngleDegrees, 9000 + Base->ZOrder);
				}
			}
			break;
		}
	}
}

void UWacomBackpackWorkspaceWidget::StartPilePointerTracking()
{
	if (!InteractionModel || !InteractionModel->IsPileMoving())
	{
		return;
	}
	StartCardMotionTimer();
}

void UWacomBackpackWorkspaceWidget::QueuePilePointer(FVector2D Pointer)
{
	if (!InteractionModel || !InteractionModel->IsPileMoving())
	{
		return;
	}
	QueuedPilePointerLocal = Pointer;
	bHasQueuedPilePointer = true;
}

void UWacomBackpackWorkspaceWidget::FlushQueuedPilePointer()
{
	if (!bHasQueuedPilePointer || !InteractionModel || !InteractionModel->IsPileMoving())
	{
		return;
	}
	bHasQueuedPilePointer = false;
	InteractionModel->UpdatePileMove(QueuedPilePointerLocal);
	ApplyActivePileMove();
}

void UWacomBackpackWorkspaceWidget::CommitPileMoveCardLayouts(
	const FWacomBackpackZoneKey& Zone,
	FVector2D FinalDelta)
{
	for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard : BoundCardWidgets)
	{
		UWacomDeckCardWidget* Card = WeakCard.Get();
		FBaseCardLayout* Base = Card ? BaseCardLayouts.Find(Card) : nullptr;
		if (!Card || !Base
			|| FWacomBackpackZoneKey::Make(
				Card->GetWorkspaceDisplayZone(),
				Card->GetWorkspaceDisplayOwnerInstanceId()) != Zone)
		{
			continue;
		}
		Base->Center += FinalDelta;
		BaseCardLayoutTransitions.Remove(Card);
		ApplyCardLayout(
			*Card,
			Base->Center,
			Base->Size,
			Base->AngleDegrees,
			Base->ZOrder);
	}
}

FWacomBackpackZoneKey UWacomBackpackWorkspaceWidget::ResolveMarqueeSource(FVector2D LocalPointer) const
{
	if (bHasExpandedContentBounds
		&& LocalPointer.X >= ExpandedContentBounds.Left && LocalPointer.X <= ExpandedContentBounds.Right
		&& LocalPointer.Y >= ExpandedContentBounds.Top && LocalPointer.Y <= ExpandedContentBounds.Bottom)
	{
		return FWacomBackpackZoneKey::Make(ExpandedContentZone, ExpandedContentOwnerInstanceId);
	}
	return FWacomBackpackZoneKey::Make(EZoneKind::Backpack);
}

FReply UWacomBackpackWorkspaceWidget::BuildHandledPointerReply()
{
	FReply Reply = FReply::Handled();
	if (bCarryInputSuspended)
	{
		return Reply.ReleaseMouseCapture();
	}
	if ((InteractionModel && (InteractionModel->IsCarrying()
			|| InteractionModel->IsMarqueeActive()
			|| InteractionModel->IsPileMoving()))
		|| bPendingCardPress || bPendingPilePress)
	{
		return Reply.CaptureMouse(TakeWidget()).SetUserFocus(TakeWidget());
	}
	return Reply.ReleaseMouseCapture();
}

FReply UWacomBackpackWorkspaceWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (bCarryInputSuspended)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}
	if (InteractionModel && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (InteractionModel->IsCarrying())
		{
			InteractionModel->NotifyReleaseGestureStarted();
			return FReply::Handled().CaptureMouse(TakeWidget()).SetUserFocus(TakeWidget());
		}
		const FVector2D Pointer = ToLocalPointer(InMouseEvent);
		const FWacomBackpackZoneKey SourceZone = ResolveMarqueeSource(Pointer);
		InteractionModel->BeginMarquee(
			SourceZone, Pointer, InMouseEvent.IsControlDown());
		const FWacomBackpackZoneKey FocusZone = FWacomBackpackZoneKey::Make(
			ExpandedPileFocus.Zone,
			ExpandedPileFocus.OwnerInstanceId);
		if (!(SourceZone == FocusZone) || ExpandedPileFocus.FocusIndex == INDEX_NONE)
		{
			ClearExpandedPileFocus(true);
		}
		RefreshInteractionPresentation();
		return FReply::Handled().CaptureMouse(TakeWidget()).SetUserFocus(TakeWidget());
	}
	if (InteractionModel && InteractionModel->IsCarrying()
		&& InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		InteractionModel->NotifyReleaseGestureStarted();
		return FReply::Handled().CaptureMouse(TakeWidget()).SetUserFocus(TakeWidget());
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UWacomBackpackWorkspaceWidget::NativeOnMouseMove(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (!InteractionModel || bCarryInputSuspended)
	{
		return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
	}
	if (InteractionModel->IsCarrying())
	{
		QueueCarryPointer(ToLocalPointer(InMouseEvent));
		return BuildHandledPointerReply();
	}
	if (InteractionModel->IsPileMoving())
	{
		QueuePilePointer(ToLocalPointer(InMouseEvent));
		return BuildHandledPointerReply();
	}
	if (TryBeginPileMove(ToLocalPointer(InMouseEvent)))
	{
		return BuildHandledPointerReply();
	}
	if (TryBeginMarqueeFromPendingPilePress(ToLocalPointer(InMouseEvent)))
	{
		return BuildHandledPointerReply();
	}
	if (TryBeginCarryFromPendingPress(ToLocalPointer(InMouseEvent)))
	{
		return BuildHandledPointerReply();
	}
	if (InteractionModel->IsMarqueeActive())
	{
		InteractionModel->UpdateMarquee(ToLocalPointer(InMouseEvent));
		RefreshInteractionPresentation();
		return BuildHandledPointerReply();
	}
	UpdateExpandedPileFocus(ToLocalPointer(InMouseEvent));
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void UWacomBackpackWorkspaceWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	BeginExpandedPileFocusExit();
	Super::NativeOnMouseLeave(InMouseEvent);
}

FReply UWacomBackpackWorkspaceWidget::NativeOnMouseButtonUp(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (!InteractionModel || bCarryInputSuspended)
	{
		return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
	}
	if (InteractionModel->IsCarrying())
	{
		if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
			|| InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			SyncCarryPointerForRelease(ToLocalPointer(InMouseEvent));
			BroadcastRelease(InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton);
			return BuildHandledPointerReply();
		}
		return FReply::Unhandled();
	}
	if (InteractionModel->IsPileMoving() && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		QueuePilePointer(ToLocalPointer(InMouseEvent));
		FlushQueuedPilePointer();
		const FWacomBackpackWorkspacePileMoveState Completed = InteractionModel->CompletePileMove();
		const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
			? InteractionStyle.Get()
			: GetDefault<UWacomBackpackWorkspaceStyle>();
		const FVector2D WorkspaceSize = GetLayoutSpaceSize();
		TArray<FSlateRect> OccupiedHeaders;
		for (const TWeakObjectPtr<UWacomBackpackZonePileWidget>& WeakPile : GetRegisteredPileWidgets())
		{
			const UWacomBackpackZonePileWidget* OtherPile = WeakPile.Get();
			if (!OtherPile || OtherPile->GetPileView().HasSameIdentity(
				Completed.Zone.Zone, Completed.Zone.OwnerInstanceId))
			{
				continue;
			}
			OccupiedHeaders.Add(OtherPile->GetResolvedHeaderRect());
		}
		const FVector2D Snapped = FWacomBackpackWorkspaceLayoutSolver::ResolvePileHeaderOverlap(
			Completed.CurrentPosition,
			WorkspaceSize,
			FVector2D(FMath::Max(260.0f, Style->PileCollapsedSize.X), 48.0f),
			FVector2D(FMath::Max(260.0f, Style->PileCollapsedSize.X), 48.0f),
			Style->PileSnapGridPixels,
			Style->PileEdgeMarginPixels,
			OccupiedHeaders);
		const FVector2D FinalDelta = Snapped - PendingPileStartPosition;
		CommitPileMoveCardLayouts(Completed.Zone, FinalDelta);
		for (const TWeakObjectPtr<UWacomBackpackZonePileWidget>& WeakPile : GetRegisteredPileWidgets())
		{
			UWacomBackpackZonePileWidget* Pile = WeakPile.Get();
			if (Pile && Pile->GetPileView().HasSameIdentity(
				Completed.Zone.Zone, Completed.Zone.OwnerInstanceId))
			{
				const FSlateRect Frame = Pile->GetResolvedFrameRect();
				const FSlateRect Header = Pile->GetResolvedHeaderRect();
				const FVector2D FrameOffset(Frame.Left - Header.Left, Frame.Top - Header.Top);
				if (UCanvasPanelSlot* PileCanvasSlot = Cast<UCanvasPanelSlot>(Pile->Slot))
				{
					const FVector2D SnappedFrame = Snapped + FrameOffset;
					PileCanvasSlot->SetPosition(SnappedFrame);
					Pile->SetRenderTranslation(FVector2D::ZeroVector);
				}
				break;
			}
		}
		OnPileMoveCommittedNative.Broadcast(
			Completed.Zone.Zone,
			Completed.Zone.OwnerInstanceId,
			FVector2D(
				WorkspaceSize.X > 1.0f ? Snapped.X / WorkspaceSize.X : 0.0f,
				WorkspaceSize.Y > 1.0f ? Snapped.Y / WorkspaceSize.Y : 0.0f));
		PendingPileWidget.Reset();
		bPendingPilePress = false;
		bPendingPileControlDown = false;
		bPendingPilePressOnDragHandle = false;
		bHasQueuedPilePointer = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	if (bPendingPilePress && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		ClearExpandedPileFocus(true);
		InteractionModel->ClickBlank();
		if (UWacomBackpackZonePileWidget* Pile = PendingPileWidget.Get())
		{
			OnPileExpansionRequestedNative.Broadcast(
				Pile->GetPileView().Zone, Pile->GetPileView().OwnerInstanceId, false);
		}
		PendingPileWidget.Reset();
		bPendingPilePress = false;
		bPendingPileControlDown = false;
		bPendingPilePressOnDragHandle = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	if (InteractionModel->IsMarqueeActive() && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		InteractionModel->UpdateMarquee(ToLocalPointer(InMouseEvent));
		InteractionModel->CompleteMarquee();
		ClearExpandedPileFocus(true);
		RefreshInteractionPresentation();
		OnInteractionChangedNative.Broadcast();
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UWacomBackpackWorkspaceWidget::NativeOnMouseWheel(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (!bCarryInputSuspended && InteractionModel && InteractionModel->IsCarrying())
	{
		const FWacomBackpackWorkspaceCarryState& PreviousCarry = InteractionModel->GetCarry();
		if (PreviousCarry.RemainingInstanceIds.IsValidIndex(PreviousCarry.CurrentIndex))
		{
			const FGuid PreviousId = PreviousCarry.RemainingInstanceIds[PreviousCarry.CurrentIndex];
			for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard : BoundCardWidgets)
			{
				UWacomDeckCardWidget* Card = WeakCard.Get();
				if (Card && IsInCarryVisualLayer(Card)
					&& Card->GetCardInstanceId() == PreviousId)
				{
					PreviousCarryCurrentCard = Card;
					break;
				}
			}
		}
		const int32 PreviousIndex = PreviousCarry.CurrentIndex;
		InteractionModel->StepCurrentByWheel(InMouseEvent.GetWheelDelta());
		if (InteractionModel->GetCarry().CurrentIndex != PreviousIndex)
		{
			bCarryCurrentExplicitlySelectedByWheel = true;
		}
		bCarryStripLayoutDirty = true;
		RebuildCarryStripLayout();
		RefreshInteractionPresentation();
		StartCardMotionTimer();
		OnInteractionChangedNative.Broadcast();
		return FReply::Handled();
	}
	return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}

FReply UWacomBackpackWorkspaceWidget::NativeOnKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	if (InteractionModel && InKeyEvent.IsControlDown() && InKeyEvent.GetKey() == EKeys::A)
	{
		InteractionModel->SelectAllMovable();
		RefreshInteractionPresentation();
		OnInteractionChangedNative.Broadcast();
		return FReply::Handled();
	}
	const bool bHasCancelablePointerInteraction =
		(InteractionModel
			&& (InteractionModel->IsCarrying() || InteractionModel->IsMarqueeActive() || InteractionModel->IsPileMoving()))
		|| bPendingCardPress || bPendingPilePress;
	if (InKeyEvent.GetKey() == EKeys::Escape && bHasCancelablePointerInteraction)
	{
		CancelInteractionWithReturn();
		OnInteractionChangedNative.Broadcast();
		return FReply::Handled();
	}
	if (InKeyEvent.GetKey() == EKeys::Escape && bHasExpandedContentBounds)
	{
		OnCollapseExpandedPileRequestedNative.Broadcast();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UWacomBackpackWorkspaceWidget::ApplyStaticCardPresentation(
	UWacomDeckCardWidget& CardWidget,
	const UWacomBackpackWorkspaceStyle& Style)
{
	const bool bInCarryLayer = IsInCarryVisualLayer(&CardWidget);
	const FBaseCardLayoutTransition* Transition = BaseCardLayoutTransitions.Find(&CardWidget);
	const FBaseCardLayout* Base = Transition
		? &Transition->Current
		: BaseCardLayouts.Find(&CardWidget);
	if (!Base || bInCarryLayer || IsInSettlementVisualLayer(&CardWidget))
	{
		return;
	}

	int32 PileFocusCardIndex = INDEX_NONE;
	const bool bPileFocusCard = IsExpandedPileFocusCard(&CardWidget, &PileFocusCardIndex);
	const FBaseCardLayout* FocusTarget = ExpandedPileFocusTargets.Find(&CardWidget);
	const bool bPileFocused = FocusTarget
		&& PileFocusCardIndex == ExpandedPileFocus.FocusIndex;
	const bool bHovered = !InteractionModel->IsCarrying()
		&& !bPileFocusCard
		&& CardWidget.IsWorkspaceInteractionEnabled()
		&& CardWidget.GetWorkspaceReadOnlyKind()
			== EWacomBackpackWorkspaceCardReadOnlyKind::None
		&& HoveredCardWidget.Get() == &CardWidget;
	const int32 PresentationZ = FocusTarget
		? FocusTarget->ZOrder
		: Base->ZOrder + (bHovered ? 5000 : 0);
	ApplyCardLayout(
		CardWidget,
		Base->Center,
		Base->Size,
		Base->AngleDegrees,
		PresentationZ);

	FVector2D TargetLocalTranslation = FVector2D::ZeroVector;
	float TargetLocalAngle = 0.0f;
	float TargetDuration = Style.HoverExitSeconds;
	if (FocusTarget)
	{
		FVector2D ScreenDelta = FocusTarget->Center - Base->Center;
		if (bPileFocused && !bSimplifiedMotion)
		{
			ScreenDelta.Y -= Style.ExpandedCardHoverLiftPixels;
			TargetLocalAngle = -Base->AngleDegrees;
		}
		TargetLocalTranslation = RotateVector(ScreenDelta, -Base->AngleDegrees);
		TargetDuration = Style.FocusReflowSeconds;
	}
	else if (bHovered)
	{
		TargetLocalTranslation = FVector2D(0.0f, -Style.ExpandedCardHoverLiftPixels);
		TargetLocalAngle = -Base->AngleDegrees;
		TargetDuration = Style.HoverEnterSeconds;
	}
	else if (bPileFocusCard)
	{
		TargetDuration = Style.FocusReturnSeconds;
	}
	GetRuntime().Motion.SetLocalPoseTarget(
		CardWidget,
		TargetLocalTranslation,
		TargetLocalAngle,
		TargetDuration,
		bSimplifiedMotion);
	++StaticCardPresentationUpdateCount;
}

void UWacomBackpackWorkspaceWidget::RefreshInteractionPresentation()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(Wacom_Backpack_FullPresentationRefresh);
#if WITH_AUTOMATION_TESTS
	++FullPresentationRefreshCount;
#endif
	if (!InteractionModel)
	{
		return;
	}
	SyncCarryLayer();
	if (bCarryStripLayoutDirty)
	{
		RebuildCarryStripLayout();
	}
	const FWacomBackpackWorkspaceCarryState& Carry = InteractionModel->GetCarry();
	const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
		? InteractionStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	FWacomBackpackWorkspaceMotionCoordinator& Motion = GetRuntime().Motion;
	TMap<FGuid, int32> CarryIndices;
	CarryIndices.Reserve(Carry.RemainingInstanceIds.Num());
	for (int32 CarryIndex = 0; CarryIndex < Carry.RemainingInstanceIds.Num(); ++CarryIndex)
	{
		CarryIndices.Add(Carry.RemainingInstanceIds[CarryIndex], CarryIndex);
	}

	for (TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard : BoundCardWidgets)
	{
		UWacomDeckCardWidget* CardWidget = WeakCard.Get();
		if (!CardWidget)
		{
			continue;
		}
		const FGuid InstanceId = CardWidget->GetCardInstanceId();
		const bool bInCarryLayer = IsInCarryVisualLayer(CardWidget);
		const int32* FoundCarryIndex = bInCarryLayer ? CarryIndices.Find(InstanceId) : nullptr;
		const int32 CarryIndex = FoundCarryIndex ? *FoundCarryIndex : INDEX_NONE;
		ApplyStaticCardPresentation(*CardWidget, *Style);
		const bool bCurrent = CarryIndex != INDEX_NONE && CarryIndex == Carry.CurrentIndex;
		const bool bSelected = InteractionModel->IsSelected(InstanceId)
			&& CardWidget->GetWorkspaceReadOnlyKind()
				== EWacomBackpackWorkspaceCardReadOnlyKind::None;
		const bool bUseReadOnlyOpacity = CardWidget->UsesReadOnlyOpacity();
		CardWidget->SetWorkspaceInteractionState(bSelected, bCurrent);
		CardWidget->ApplyWorkspaceVisualState(
			FWacomBackpackWorkspaceMotionCoordinator::BuildVisualState(
				*Style,
				bSelected,
				bCurrent,
				bUseReadOnlyOpacity));
	}

	FVector2D PresentationPointer = Carry.PointerPosition;
	if (!InteractionModel->IsCarrying() && FSlateApplication::IsInitialized())
	{
		PresentationPointer = GetCachedGeometry().AbsoluteToLocal(
			FSlateApplication::Get().GetCursorPos());
	}
	Motion.Reconcile(
		BoundCardWidgets,
		GetPresentationFocusedCard(),
		InteractionModel->IsCarrying() ? &Carry : nullptr,
		CarryActiveLayer ? CarryActiveLayer.Get() : CarryLayer.Get(),
		GetCachedGeometry(),
		PresentationPointer,
		*Style,
		bSimplifiedMotion);
	if (Motion.WantsTick() || !BaseCardLayoutTransitions.IsEmpty())
	{
		StartCardMotionTimer();
	}

	if (SelectionMarquee)
	{
		SelectionMarquee->SetVisibility(InteractionModel->IsMarqueeActive()
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
		if (InteractionModel->IsMarqueeActive())
		{
			const FWacomBackpackWorkspaceSelectionState& Selection = InteractionModel->GetSelection();
			const FVector2D Minimum(
				FMath::Min(Selection.MarqueeStart.X, Selection.MarqueeCurrent.X),
				FMath::Min(Selection.MarqueeStart.Y, Selection.MarqueeCurrent.Y));
			const FVector2D Size(
				FMath::Abs(Selection.MarqueeCurrent.X - Selection.MarqueeStart.X),
				FMath::Abs(Selection.MarqueeCurrent.Y - Selection.MarqueeStart.Y));
			if (UCanvasPanelSlot* MarqueeCanvasSlot = Cast<UCanvasPanelSlot>(SelectionMarquee->Slot))
			{
				MarqueeCanvasSlot->SetPosition(Minimum);
				MarqueeCanvasSlot->SetSize(Size);
			}
		}
	}
}

void UWacomBackpackWorkspaceWidget::SetCarryInputSuspended(bool bSuspended)
{
	bCarryInputSuspended = bSuspended;
	if (InteractionModel)
	{
		InteractionModel->SetCarryInputSuspended(bSuspended);
	}
	if (bSuspended)
	{
		ClearExpandedPileFocus(true);
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().ReleaseAllPointerCapture(0);
		}
	}
	else if (InteractionModel && InteractionModel->IsCarrying())
	{
		StartCardMotionTimer();
	}
	RefreshInteractionPresentation();
}

void UWacomBackpackWorkspaceWidget::CancelInteraction()
{
	SetPileDropPreview(EZoneKind::Backpack, FGuid(), false, false);
	CancelHoverExpandTimer();
	ClearExpandedPileFocus(false);
	bPendingCardPress = false;
	PendingCardPressId.Invalidate();
	bPendingPilePress = false;
	bPendingPileControlDown = false;
	bPendingPilePressOnDragHandle = false;
	PendingPileWidget.Reset();
	bCarryInputSuspended = false;
	bPileCollapseAnimationPending = false;
	if (InteractionModel)
	{
		InteractionModel->CancelTransientState();
	}
	StopCardMotionTimer();
	bHasQueuedCarryPointer = false;
	bHasQueuedPilePointer = false;
	bCarryStripLayoutDirty = false;
	bCarryCurrentExplicitlySelectedByWheel = false;
	PreviousCarryCurrentCard.Reset();
	PendingReleasedVisualHandoffs.Reset();
	PendingReleasedVisualPoses.Reset();
	SettlementTargets.Reset();
	BaseCardLayoutTransitions.Reset();
	if (Runtime)
	{
		GetRuntime().Motion.Reset();
	}
	if (SettlementLayer && StaticCardLayer)
	{
		TArray<UWacomDeckCardWidget*> SettlingCards;
		for (int32 Index = 0; Index < SettlementLayer->GetChildrenCount(); ++Index)
		{
			if (UWacomDeckCardWidget* Card = Cast<UWacomDeckCardWidget>(
				SettlementLayer->GetChildAt(Index)))
			{
				SettlingCards.Add(Card);
			}
		}
		for (UWacomDeckCardWidget* Card : SettlingCards)
		{
			Card->RemoveFromParent();
			StaticCardLayer->AddChildToCanvas(Card);
			if (const FBaseCardLayout* Base = BaseCardLayouts.Find(Card))
			{
				ApplyCardLayout(*Card, Base->Center, Base->Size, Base->AngleDegrees, Base->ZOrder);
			}
		}
	}
	RestoreStaticCardParents();
	bCarryVisualAnchorInitialized = false;
	CarryAnchorLocal = FVector2D::ZeroVector;
	CarryVisualAnchorLocal = FVector2D::ZeroVector;
	CancelHoverExpandTimer();
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().ReleaseAllPointerCapture(0);
	}
	RefreshInteractionPresentation();
}

void UWacomBackpackWorkspaceWidget::CancelInteractionWithReturn()
{
	if (!InteractionModel || !InteractionModel->IsCarrying() || bSimplifiedMotion)
	{
		CancelInteraction();
		return;
	}
	if (!SettlementLayer)
	{
		EnsureFallbackTree();
	}
	const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
		? InteractionStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	const TArray<FGuid> ReturningIds = InteractionModel->GetCarry().RemainingInstanceIds;
	CaptureReleasedVisualPoses(ReturningIds);
	for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard : BoundCardWidgets)
	{
		UWacomDeckCardWidget* Card = WeakCard.Get();
		if (!Card || !ReturningIds.Contains(Card->GetCardInstanceId()))
		{
			continue;
		}
		const FBaseCardLayout* Target = BaseCardLayouts.Find(Card);
		const FCardVisualPose* Start = PendingReleasedVisualPoses.Find(Card->GetCardInstanceId());
		if (!Target || !Start || !SettlementLayer)
		{
			continue;
		}
		Card->RemoveFromParent();
		SettlementLayer->AddChildToCanvas(Card);
		ApplyCardLayout(*Card, Target->Center, Target->Size, Target->AngleDegrees, Target->ZOrder);
		SettlementTargets.Add(Card, *Target);
		GetRuntime().Motion.BeginSettlement(
			*Card,
			RotateVector(Start->Center - Target->Center, -Target->AngleDegrees),
			FMath::FindDeltaAngleDegrees(Target->AngleDegrees, Start->AngleDegrees),
			Style->CancelReturnSeconds,
			false);
	}
	PendingReleasedVisualPoses.Reset();
	InteractionModel->CancelTransientState();
	StopCardMotionTimer();
	bHasQueuedCarryPointer = false;
	bCarryStripLayoutDirty = false;
	PendingReleasedVisualHandoffs.Reset();
	bCarryVisualAnchorInitialized = false;
	if (CarryRoot)
	{
		CarryRoot->SetRenderTranslation(FVector2D::ZeroVector);
	}
	RefreshInteractionPresentation();
	StartCardMotionTimer();
}

void UWacomBackpackWorkspaceWidget::QueueCarryPointer(FVector2D Pointer)
{
	if (!InteractionModel || !InteractionModel->IsCarrying() || bCarryInputSuspended)
	{
		return;
	}
	QueuedCarryPointerLocal = Pointer;
	bHasQueuedCarryPointer = true;
	// Rule/target truth is updated immediately. Only the visual CarryRoot is delayed.
	UpdateCarryAnchor(Pointer);
}

void UWacomBackpackWorkspaceWidget::FlushQueuedCarryPointer()
{
	if (!bHasQueuedCarryPointer || !InteractionModel || !InteractionModel->IsCarrying()
		|| bCarryInputSuspended)
	{
		return;
	}
	bHasQueuedCarryPointer = false;
	UpdateCarryAnchor(QueuedCarryPointerLocal);
}

void UWacomBackpackWorkspaceWidget::SyncCarryPointerForRelease(FVector2D Pointer)
{
	if (!InteractionModel || !InteractionModel->IsCarrying() || bCarryInputSuspended)
	{
		return;
	}
	QueuedCarryPointerLocal = Pointer;
	bHasQueuedCarryPointer = false;
	// The command uses the exact mouse-up position; the release animation captures
	// the independently smoothed visual anchor.
	UpdateCarryAnchor(Pointer);
}

void UWacomBackpackWorkspaceWidget::UpdateCarryAnchor(FVector2D Pointer, bool bUpdateModel)
{
	if (!InteractionModel || !InteractionModel->IsCarrying() || bCarryInputSuspended)
	{
		return;
	}
	CarryAnchorLocal = Pointer;
	if (bUpdateModel)
	{
		InteractionModel->UpdateCarryPointer(Pointer);
	}
	if (!bCarryVisualAnchorInitialized || bSimplifiedMotion)
	{
		CarryVisualAnchorLocal = Pointer;
		bCarryVisualAnchorInitialized = true;
		if (CarryRoot)
		{
			CarryRoot->SetRenderTranslation(CarryVisualAnchorLocal);
		}
		++CarryVisualAnchorApplyCount;
	}
	GetRuntime().Motion.UpdatePointer(GetCachedGeometry(), Pointer, true);
}

void UWacomBackpackWorkspaceWidget::ApplyCarryVisualAnchor(float DeltaTime)
{
	if (!InteractionModel || !InteractionModel->IsCarrying() || !bCarryVisualAnchorInitialized)
	{
		return;
	}
	const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
		? InteractionStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	const FVector2D Previous = CarryVisualAnchorLocal;
	if (bSimplifiedMotion)
	{
		CarryVisualAnchorLocal = CarryAnchorLocal;
	}
	else
	{
		CarryVisualAnchorLocal = FWacomCardMotionKernel::StepExponentialWithMaximumLag(
			CarryVisualAnchorLocal,
			CarryAnchorLocal,
			Style->CarryFollowResponseSpeed,
			Style->CarryMaximumVisualLagPixels,
			DeltaTime);
		if (CarryVisualAnchorLocal.Equals(CarryAnchorLocal, 0.5f))
		{
			CarryVisualAnchorLocal = CarryAnchorLocal;
		}
	}
	if (!Previous.Equals(CarryVisualAnchorLocal, 0.01f) && CarryRoot)
	{
		CarryRoot->SetRenderTranslation(CarryVisualAnchorLocal);
		++CarryVisualAnchorApplyCount;
	}
}

void UWacomBackpackWorkspaceWidget::StartCardMotionTimer()
{
	const bool bHasCarry = InteractionModel && InteractionModel->IsCarrying()
		&& !bCarryInputSuspended;
	const bool bHasPresentationMotion = Runtime
		&& GetRuntime().Motion.WantsTick();
	const bool bHasFocusExitDelay = ExpandedPileFocus.bExitPending;
	const bool bHasPileMove = InteractionModel && InteractionModel->IsPileMoving();
	if ((!bHasCarry && !bHasPresentationMotion && SettlementTargets.IsEmpty()
			&& !bHasFocusExitDelay && BaseCardLayoutTransitions.IsEmpty()
			&& !bHasPileMove && !bHoverExpandTimerActive
			&& !bPileCollapseAnimationPending)
		|| bCardMotionTimerActive)
	{
		return;
	}
	TSharedPtr<SWidget> CachedWidget = GetCachedWidget();
	if (!CachedWidget.IsValid())
	{
		return;
	}
	bCardMotionTimerActive = true;
	const uint64 TimerGeneration = ++CardMotionTimerGeneration;
	bHasQueuedCarryPointer = false;
	const TWeakObjectPtr<UWacomBackpackWorkspaceWidget> WeakThis(this);
	CachedWidget->RegisterActiveTimer(
		0.0f,
		FWidgetActiveTimerDelegate::CreateLambda(
			[WeakThis, TimerGeneration](double, float DeltaSeconds)
			{
				UWacomBackpackWorkspaceWidget* Self = WeakThis.Get();
				if (!Self || !Self->bCardMotionTimerActive
					|| Self->CardMotionTimerGeneration != TimerGeneration)
				{
					return EActiveTimerReturnType::Stop;
				}
				const bool bCarrying = Self->InteractionModel
					&& Self->InteractionModel->IsCarrying()
					&& !Self->bCarryInputSuspended;
				const bool bPileMoving = Self->InteractionModel
					&& Self->InteractionModel->IsPileMoving();
				if (bCarrying && FSlateApplication::IsInitialized())
				{
					const FVector2D LatestPointer = Self->GetCachedGeometry().AbsoluteToLocal(
						FSlateApplication::Get().GetCursorPos());
					Self->UpdateCarryAnchor(LatestPointer);
				}
				if (bCarrying)
				{
					Self->ApplyCarryVisualAnchor(DeltaSeconds);
				}
				if (bPileMoving && FSlateApplication::IsInitialized())
				{
					Self->QueuePilePointer(Self->GetCachedGeometry().AbsoluteToLocal(
						FSlateApplication::Get().GetCursorPos()));
					Self->FlushQueuedPilePointer();
				}
				else if (!bPileMoving)
				{
					Self->bHasQueuedPilePointer = false;
				}
				const UWacomBackpackWorkspaceStyle* Style = Self->InteractionStyle.IsValid()
					? Self->InteractionStyle.Get()
					: GetDefault<UWacomBackpackWorkspaceStyle>();
				if (Self->bHoverExpandTimerActive)
				{
					if (!Self->InteractionModel || !Self->InteractionModel->IsCarrying())
					{
						Self->CancelHoverExpandTimer();
					}
					else
					{
						Self->HoverExpandElapsedSeconds += FMath::Max(0.0f, DeltaSeconds);
						if (Self->HoverExpandElapsedSeconds
							>= FMath::Max(0.0f, Style->PileHoverExpandDelaySeconds))
						{
							const EZoneKind ExpandZone = Self->HoverExpandZone;
							const FGuid ExpandOwner = Self->HoverExpandOwnerInstanceId;
							Self->CancelHoverExpandTimer();
							Self->OnPileExpansionRequestedNative.Broadcast(
								ExpandZone, ExpandOwner, true);
						}
					}
				}
				Self->TickBaseCardLayoutTransitions(DeltaSeconds);
				if (Self->bPileCollapseAnimationPending
					&& Self->BaseCardLayoutTransitions.IsEmpty())
				{
					Self->bPileCollapseAnimationPending = false;
					Self->bCarryInputSuspended = false;
					Self->OnPileCollapseAnimationFinishedNative.Broadcast(
						Self->CollapsingPileZone,
						Self->CollapsingPileOwnerInstanceId);
				}
				if (Self->Runtime)
				{
					Self->GetRuntime().Motion.Tick(
						DeltaSeconds,
						Self->GetCachedGeometry(),
						*Style,
						Self->bSimplifiedMotion);
				}
				Self->TickExpandedPileFocusExit(DeltaSeconds);
				Self->FinalizeCompletedSettlements();
				const bool bKeepTicking = bCarrying
					|| bPileMoving
					|| (Self->Runtime
						&& Self->GetRuntime().Motion.WantsTick())
					|| !Self->SettlementTargets.IsEmpty()
					|| Self->ExpandedPileFocus.bExitPending
					|| !Self->BaseCardLayoutTransitions.IsEmpty()
					|| Self->bHoverExpandTimerActive
					|| Self->bPileCollapseAnimationPending;
				if (!bKeepTicking)
				{
					Self->bCardMotionTimerActive = false;
					Self->SyncCarryLayer();
					return EActiveTimerReturnType::Stop;
				}
				return EActiveTimerReturnType::Continue;
			}));
}

void UWacomBackpackWorkspaceWidget::StopCardMotionTimer()
{
	bCardMotionTimerActive = false;
	++CardMotionTimerGeneration;
}

bool UWacomBackpackWorkspaceWidget::ShouldPreserveCardParent(
	const UWacomDeckCardWidget* CardWidget) const
{
	// Carry layers own an existing card until ApplyCardBaseLayout consumes the new
	// scene target. This also covers the short successful-release interval after
	// the interaction model has removed the card but before static placement.
	return IsInCarryVisualLayer(CardWidget) || IsInSettlementVisualLayer(CardWidget);
}

bool UWacomBackpackWorkspaceWidget::IsInCarryVisualLayer(const UWidget* CardWidget) const
{
	if (!CardWidget)
	{
		return false;
	}
	const UPanelWidget* Parent = CardWidget->GetParent();
	return (CarryLayer && Parent == CarryLayer)
		|| (CarryActiveLayer && Parent == CarryActiveLayer);
}

bool UWacomBackpackWorkspaceWidget::IsInSettlementVisualLayer(const UWidget* CardWidget) const
{
	return CardWidget && SettlementLayer && CardWidget->GetParent() == SettlementLayer;
}

void UWacomBackpackWorkspaceWidget::SyncCarryLayer()
{
	if (!StaticCardLayer || !CarryLayer || !CarryActiveLayer)
	{
		EnsureFallbackTree();
	}
	if (!StaticCardLayer || !CarryLayer || !CarryActiveLayer)
	{
		return;
	}
	if (!InteractionModel || !InteractionModel->IsCarrying())
	{
		PreviousCarryCurrentCard.Reset();
		RestoreStaticCardParents();
		return;
	}

	const FWacomBackpackWorkspaceCarryState& Carry = InteractionModel->GetCarry();
	if (LastCarryStripInstanceIds != Carry.RemainingInstanceIds
		|| LastCarryStripCurrentIndex != Carry.CurrentIndex
		|| LastCarryStripDefaultIndex != Carry.DefaultIndex)
	{
		bCarryStripLayoutDirty = true;
	}
	const FGuid CurrentInstanceId = Carry.RemainingInstanceIds.IsValidIndex(Carry.CurrentIndex)
		? Carry.RemainingInstanceIds[Carry.CurrentIndex]
		: FGuid();
	TSet<FGuid> CarriedIds;
	CarriedIds.Reserve(Carry.RemainingInstanceIds.Num());
	for (const FGuid InstanceId : Carry.RemainingInstanceIds)
	{
		CarriedIds.Add(InstanceId);
	}
	bool bChanged = false;
	for (const FGuid InstanceId : Carry.RemainingInstanceIds)
	{
		UWacomDeckCardWidget* Card = GetRuntime().Visuals.FindPhysicalCard(InstanceId);
		if (!Card
			|| Card->GetWorkspaceReadOnlyKind()
				!= EWacomBackpackWorkspaceCardReadOnlyKind::None
			|| Carry.SourceZone != FWacomBackpackZoneKey::Make(
				Card->GetWorkspaceDisplayZone(),
				Card->GetWorkspaceDisplayOwnerInstanceId()))
		{
			continue;
		}
		// Keep the outgoing current card in the active branch for this sync even
		// when the newly solved pose happens to be a no-op.  The wheel handoff is
		// intentionally a one-frame ownership contract: both the outgoing and
		// incoming current cards must share the active branch before the outgoing
		// card is allowed to return to the cached strip.
		const bool bPreviousCurrentStillMoving = PreviousCarryCurrentCard.Get() == Card;
		const bool bKeepInActiveMotionLayer = InstanceId == CurrentInstanceId
			|| bPreviousCurrentStillMoving;
		UCanvasPanel* DesiredCarryLayer = bKeepInActiveMotionLayer
			? CarryActiveLayer.Get()
			: CarryLayer.Get();
		if (Card->GetParent() != DesiredCarryLayer)
		{
			Card->RemoveFromParent();
			DesiredCarryLayer->AddChildToCanvas(Card);
			Card->SetVisibility(ESlateVisibility::Visible);
			bChanged = true;
		}
	}
	TArray<UWacomDeckCardWidget*> LayerCards;
	const auto GatherLayerCards = [&LayerCards](const UCanvasPanel* Layer)
	{
		if (!Layer)
		{
			return;
		}
		for (int32 Index = 0; Index < Layer->GetChildrenCount(); ++Index)
		{
			if (UWacomDeckCardWidget* Card = Cast<UWacomDeckCardWidget>(Layer->GetChildAt(Index)))
			{
				LayerCards.Add(Card);
			}
		}
	};
	GatherLayerCards(CarryLayer);
	GatherLayerCards(CarryActiveLayer);
	for (UWacomDeckCardWidget* Card : LayerCards)
	{
		if (!Card || CarriedIds.Contains(Card->GetCardInstanceId())
			|| PendingReleasedVisualHandoffs.Contains(Card->GetCardInstanceId()))
		{
			continue;
		}
		Card->RemoveFromParent();
		StaticCardLayer->AddChildToCanvas(Card);
		if (const FBaseCardLayout* Base = BaseCardLayouts.Find(Card))
		{
			ApplyCardLayout(*Card, Base->Center, Base->Size, Base->AngleDegrees, Base->ZOrder);
		}
		bChanged = true;
	}
	if (bChanged)
	{
		bCarryStripLayoutDirty = true;
	}
	if (PreviousCarryCurrentCard.IsValid()
		&& (!Runtime
			|| !GetRuntime().Motion.IsCardMoving(*PreviousCarryCurrentCard.Get())))
	{
		PreviousCarryCurrentCard.Reset();
	}
	UpdateCarryAnchor(Carry.PointerPosition, false);
}

void UWacomBackpackWorkspaceWidget::RebuildCarryStripLayout()
{
	if (!InteractionModel || !InteractionModel->IsCarrying()
		|| !CarryLayer || !CarryActiveLayer)
	{
		bCarryStripLayoutDirty = false;
		return;
	}
	const FWacomBackpackWorkspaceCarryState& Carry = InteractionModel->GetCarry();
	const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
		? InteractionStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	const float AvailableWidth = FMath::Max(
		Style->CardRenderSize.X,
		GetLayoutSpaceSize().X - FMath::Max(0.0f, Style->PileEdgeMarginPixels) * 2.0f);
	const TArray<FWacomBackpackCarriedStripLayout> Strip =
		FWacomBackpackWorkspaceLayoutSolver::BuildCarriedStripLayout(
			Carry.RemainingInstanceIds.Num(),
			Carry.CurrentIndex,
			Carry.DefaultIndex,
			FVector2D::ZeroVector,
			AvailableWidth,
			Style->CardRenderSize.X,
			Style->AdaptiveStripExposurePixels,
			Style->AdaptiveStripFocusSeparationPixels,
			0.0f);
	const bool bAnimateReflow = !LastCarryStripInstanceIds.IsEmpty()
		&& !Carry.bInitialReleaseGuardArmed
		&& Runtime
		&& (LastCarryStripInstanceIds != Carry.RemainingInstanceIds
			|| LastCarryStripCurrentIndex != Carry.CurrentIndex
			|| LastCarryStripDefaultIndex != Carry.DefaultIndex);
	TMap<TWeakObjectPtr<UWacomDeckCardWidget>, FCardVisualPose> VisualStarts;
	for (int32 Index = 0; Index < Carry.RemainingInstanceIds.Num(); ++Index)
	{
		if (!Strip.IsValidIndex(Index))
		{
			continue;
		}
		UWacomDeckCardWidget* Card = GetRuntime().Visuals.FindPhysicalCard(
			Carry.RemainingInstanceIds[Index]);
		if (Card && !IsInCarryVisualLayer(Card))
		{
			Card = nullptr;
		}
		if (Card)
		{
			if (bAnimateReflow)
			{
				VisualStarts.Add(Card, CaptureCardVisualPose(*Card));
			}
			const bool bCurrent = Index == Carry.CurrentIndex;
			const int32 CarryZOrder = bCurrent
				? Carry.RemainingInstanceIds.Num() * 4
					+ FMath::Max(0, Style->CurrentCardZOrderBoost)
				: Strip[Index].Transform.LayerRank;
			FBaseCardLayout TargetBase;
			TargetBase.Center = Strip[Index].Transform.CardCenter;
			TargetBase.Size = Style->CardRenderSize;
			TargetBase.AngleDegrees = Strip[Index].Transform.AngleDegrees;
			TargetBase.ZOrder = CarryZOrder;
			ApplyCardLayout(
				*Card,
				TargetBase.Center,
				TargetBase.Size,
				TargetBase.AngleDegrees,
				TargetBase.ZOrder);
			if (Runtime)
			{
				const bool bCurrentShouldLift = bCurrent
					&& (Carry.CurrentIndex != Carry.DefaultIndex
						|| bCarryCurrentExplicitlySelectedByWheel);
				const FVector2D TargetLocalTranslation = bCurrentShouldLift && !bSimplifiedMotion
					? RotateVector(
						FVector2D(0.0f, -Style->CurrentCardLiftPixels),
						-TargetBase.AngleDegrees)
					: FVector2D::ZeroVector;
				const float TargetLocalAngle = bCurrentShouldLift && !bSimplifiedMotion
					? -TargetBase.AngleDegrees
					: 0.0f;
				if (bAnimateReflow)
				{
					RetargetCardLocalPoseFromVisual(
						*Card,
						VisualStarts.FindChecked(Card),
						TargetBase,
						TargetLocalTranslation,
						TargetLocalAngle,
						Style->CarryCurrentTransitionSeconds);
				}
				else
				{
					GetRuntime().Motion.SetLocalPoseTarget(
						*Card,
						TargetLocalTranslation,
						TargetLocalAngle,
						Style->CarryCurrentTransitionSeconds,
						bSimplifiedMotion);
				}
			}
		}
	}
	bCarryStripLayoutDirty = false;
	LastCarryStripInstanceIds = Carry.RemainingInstanceIds;
	LastCarryStripCurrentIndex = Carry.CurrentIndex;
	LastCarryStripDefaultIndex = Carry.DefaultIndex;
	++CarryStripLayoutRebuildCount;
	StartCardMotionTimer();
}

void UWacomBackpackWorkspaceWidget::RetargetCardLocalPoseFromVisual(
	UWacomDeckCardWidget& Card,
	const FCardVisualPose& VisualPose,
	const FBaseCardLayout& TargetBase,
	FVector2D TargetLocalTranslation,
	float TargetLocalAngle,
	float DurationSeconds)
{
	FWacomBackpackWorkspaceMotionCoordinator& Motion = GetRuntime().Motion;
	FVector2D TargetParentCenter = TargetBase.Center;
	if (IsInCarryVisualLayer(&Card))
	{
		TargetParentCenter += CarryVisualAnchorLocal;
	}
	const FVector2D StartLocalTranslation = RotateVector(
		VisualPose.Center - TargetParentCenter,
		-TargetBase.AngleDegrees);
	const float StartLocalAngle = FMath::FindDeltaAngleDegrees(
		TargetBase.AngleDegrees,
		VisualPose.AngleDegrees);
	Motion.SnapLocalPose(
		Card, StartLocalTranslation, StartLocalAngle);
	Motion.SetLocalPoseTarget(
		Card,
		TargetLocalTranslation,
		TargetLocalAngle,
		DurationSeconds,
		bSimplifiedMotion);
}

void UWacomBackpackWorkspaceWidget::BeginCarryPickupFeedback()
{
	if (!InteractionModel || !InteractionModel->IsCarrying())
	{
		return;
	}
	const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
		? InteractionStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	const FWacomBackpackWorkspaceCarryState& Carry = InteractionModel->GetCarry();
	TArray<TWeakObjectPtr<UWacomDeckCardWidget>> CarryCards;
	CarryCards.Reserve(Carry.RemainingInstanceIds.Num());
	for (const FGuid InstanceId : Carry.RemainingInstanceIds)
	{
		if (UWacomDeckCardWidget* Card = GetRuntime().Visuals.FindPhysicalCard(InstanceId);
			Card && IsInCarryVisualLayer(Card))
		{
			CarryCards.Add(Card);
		}
	}
	GetRuntime().Motion.BeginCarryPickup(
		CarryCards,
		Style->CarryPickupLiftPixels,
		Style->CarryPickupSeconds,
		bSimplifiedMotion);
}

void UWacomBackpackWorkspaceWidget::CaptureReleasedVisualPoses(
	TConstArrayView<FGuid> InstanceIds)
{
	for (const FGuid InstanceId : InstanceIds)
	{
		if (UWacomDeckCardWidget* Card = GetRuntime().Visuals.FindPhysicalCard(InstanceId))
		{
			PendingReleasedVisualPoses.Add(
				InstanceId,
				CaptureCardVisualPose(*Card));
		}
	}
}

UWacomBackpackWorkspaceWidget::FCardVisualPose
UWacomBackpackWorkspaceWidget::CaptureCardVisualPose(const UWacomDeckCardWidget& Card) const
{
	FCardVisualPose Pose;
	const UCanvasPanelSlot* StaticCardSlot = Cast<UCanvasPanelSlot>(Card.Slot);
	if (!StaticCardSlot)
	{
		return Pose;
	}
	const float BaseAngle = Card.GetRenderTransformAngle();
	Pose.Center = StaticCardSlot->GetPosition() + StaticCardSlot->GetSize() * 0.5f;
	if (IsInCarryVisualLayer(&Card))
	{
		Pose.Center += CarryVisualAnchorLocal;
	}
	Pose.Center += RotateVector(Card.GetBackpackLocalMotionTranslation(), BaseAngle);
	Pose.AngleDegrees = BaseAngle + Card.GetBackpackLocalMotionAngle();
	return Pose;
}

void UWacomBackpackWorkspaceWidget::FinalizeCompletedSettlements()
{
	if (!Runtime || !StaticCardLayer)
	{
		return;
	}
	TArray<TWeakObjectPtr<UWacomDeckCardWidget>> Completed;
	GetRuntime().Motion.ConsumeCompletedSettlements(Completed);
	for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard : Completed)
	{
		UWacomDeckCardWidget* Card = WeakCard.Get();
		const FBaseCardLayout* Target = Card ? SettlementTargets.Find(Card) : nullptr;
		if (!Card || !Target)
		{
			SettlementTargets.Remove(WeakCard);
			continue;
		}
		const FBaseCardLayout Final = *Target;
		SettlementTargets.Remove(Card);
		Card->RemoveFromParent();
		StaticCardLayer->AddChildToCanvas(Card);
		Card->ResetBackpackLocalMotionPose();
		ApplyCardLayout(*Card, Final.Center, Final.Size, Final.AngleDegrees, Final.ZOrder);
	}
	if (SettlementTargets.IsEmpty()
		&& PendingReleasedVisualHandoffs.IsEmpty()
		&& (!InteractionModel || !InteractionModel->IsCarrying()))
	{
		if (CarryRoot)
		{
			CarryRoot->SetRenderTranslation(FVector2D::ZeroVector);
		}
		bCarryVisualAnchorInitialized = false;
	}
}

void UWacomBackpackWorkspaceWidget::RestoreStaticCardParents()
{
	if ((!CarryLayer && !CarryActiveLayer) || !StaticCardLayer)
	{
		return;
	}
	TArray<UWacomDeckCardWidget*> ToRestore;
	auto GatherLayer = [&ToRestore](const UCanvasPanel* Layer)
	{
		if (!Layer)
		{
			return;
		}
		for (int32 Index = 0; Index < Layer->GetChildrenCount(); ++Index)
		{
			if (UWacomDeckCardWidget* Card = Cast<UWacomDeckCardWidget>(Layer->GetChildAt(Index)))
			{
				ToRestore.Add(Card);
			}
		}
	};
	GatherLayer(CarryLayer);
	GatherLayer(CarryActiveLayer);
	bool bPreservedPendingHandoff = false;
	for (UWacomDeckCardWidget* Card : ToRestore)
	{
		if (PendingReleasedVisualHandoffs.Contains(Card->GetCardInstanceId()))
		{
			bPreservedPendingHandoff = true;
			continue;
		}
		Card->RemoveFromParent();
		StaticCardLayer->AddChildToCanvas(Card);
		if (const FBaseCardLayout* Base = BaseCardLayouts.Find(Card))
		{
			Card->ResetBackpackLocalMotionPose();
			ApplyCardLayout(*Card, Base->Center, Base->Size, Base->AngleDegrees, Base->ZOrder);
		}
	}
	if (bPreservedPendingHandoff)
	{
		return;
	}
	if (CarryRoot)
	{
		CarryRoot->SetRenderTranslation(FVector2D::ZeroVector);
	}
	if (CarryCache)
	{
		CarryCache->SetRenderTranslation(FVector2D::ZeroVector);
	}
	else if (CarryLayer)
	{
		CarryLayer->SetRenderTranslation(FVector2D::ZeroVector);
	}
	LastCarryStripInstanceIds.Reset();
	bCarryCurrentExplicitlySelectedByWheel = false;
	LastCarryStripCurrentIndex = INDEX_NONE;
	LastCarryStripDefaultIndex = INDEX_NONE;
	if (!InteractionModel || !InteractionModel->IsCarrying())
	{
		bCarryVisualAnchorInitialized = false;
		CarryAnchorLocal = FVector2D::ZeroVector;
		CarryVisualAnchorLocal = FVector2D::ZeroVector;
	}
}

bool UWacomBackpackWorkspaceWidget::AcceptStableLayoutGeometry(FVector2D LayoutSize)
{
	if (LayoutSize.X <= 1.0f || LayoutSize.Y <= 1.0f
		|| !FMath::IsFinite(LayoutSize.X) || !FMath::IsFinite(LayoutSize.Y)
		|| (bHasStableLayoutSize && StableLayoutSize.Equals(LayoutSize, LayoutGeometryTolerance)))
	{
		return false;
	}

	StableLayoutSize = LayoutSize;
	bHasStableLayoutSize = true;
	if (StaticCardLayer)
	{
		StaticCardLayer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	OnLayoutGeometryReadyNative.Broadcast(StableLayoutSize);
	RefreshInteractionPresentation();
	ScheduleBoundCardFaceRender();
	return true;
}

void UWacomBackpackWorkspaceWidget::RequestBoundCardFaceRenders()
{
	for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCardWidget : BoundCardWidgets)
	{
		if (UWacomDeckCardWidget* CardWidget = WeakCardWidget.Get())
		{
			CardWidget->RequestBackpackCardFaceRender();
		}
	}
}

void UWacomBackpackWorkspaceWidget::ScheduleBoundCardFaceRender()
{
	bDeferredCardFaceRenderRequested = true;
	if (bDeferredCardFaceRenderActive)
	{
		return;
	}

	const TSharedPtr<SWidget> CachedWidget = GetCachedWidget();
	if (!CachedWidget.IsValid())
	{
		return;
	}

	bDeferredCardFaceRenderRequested = false;
	bDeferredCardFaceRenderActive = true;
	const TWeakObjectPtr<UWacomBackpackWorkspaceWidget> WeakThis(this);
	CachedWidget->RegisterActiveTimer(
		0.0f,
		FWidgetActiveTimerDelegate::CreateLambda(
			[WeakThis](double CurrentTime, float DeltaTime)
			{
				UWacomBackpackWorkspaceWidget* Self = WeakThis.Get();
				if (!Self || !Self->bDeferredCardFaceRenderActive)
				{
					return EActiveTimerReturnType::Stop;
				}
				Self->FlushDeferredCardFaceRender();
				return EActiveTimerReturnType::Stop;
			}));
}

void UWacomBackpackWorkspaceWidget::FlushDeferredCardFaceRender()
{
	if (!bDeferredCardFaceRenderRequested && !bDeferredCardFaceRenderActive)
	{
		return;
	}

	bDeferredCardFaceRenderRequested = false;
	bDeferredCardFaceRenderActive = false;
	if (!bHasStableLayoutSize || !StaticCardLayer
		|| StaticCardLayer->GetVisibility() == ESlateVisibility::Hidden
		|| StaticCardLayer->GetVisibility() == ESlateVisibility::Collapsed)
	{
		return;
	}

	RefreshInteractionPresentation();
	RequestBoundCardFaceRenders();
	++DeferredCardFaceRenderPassCount;
}

void UWacomBackpackWorkspaceWidget::EnsureFallbackTree()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
	}
	if (!WidgetTree)
	{
		return;
	}
	if (!WorkspaceCanvas)
	{
		WorkspaceCanvas = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("WorkspaceCanvas")));
	}
	if (!PileFrameLayer)
	{
		PileFrameLayer = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("PileFrameLayer")));
	}
	if (!StaticCardLayer)
	{
		StaticCardLayer = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("StaticCardLayer")));
	}
	if (!SettlementLayer)
	{
		SettlementLayer = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("SettlementLayer")));
	}
	if (!CarryRoot)
	{
		CarryRoot = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("CarryRoot")));
	}
	if (!CarryLayer)
	{
		CarryLayer = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("CarryLayer")));
	}
	if (!CarryCache)
	{
		CarryCache = Cast<UInvalidationBox>(WidgetTree->FindWidget(TEXT("CarryCache")));
	}
	if (!CarryActiveLayer)
	{
		CarryActiveLayer = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("CarryActiveLayer")));
	}
	if (!WorkspaceCanvas && WidgetTree->RootWidget)
	{
		WorkspaceCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	}
	UCanvasPanel* Root = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!Root)
	{
		Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("WorkspaceRoot"));
		WidgetTree->RootWidget = Root;
	}
	if (!WorkspaceCanvas)
	{
		WorkspaceCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("WorkspaceCanvas"));
		if (UCanvasPanelSlot* WorkspaceSlot = Root->AddChildToCanvas(WorkspaceCanvas))
		{
			WorkspaceSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			WorkspaceSlot->SetOffsets(FMargin(0.0f));
		}
	}
	auto EnsureLayer = [this](TObjectPtr<UCanvasPanel>& Layer, FName Name, int32 ZOrder)
	{
		if (Layer || !WorkspaceCanvas)
		{
			return;
		}
		Layer = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), Name);
		Layer->SetClipping(EWidgetClipping::Inherit);
		if (UCanvasPanelSlot* Slot = WorkspaceCanvas->AddChildToCanvas(Layer))
		{
			Slot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			Slot->SetOffsets(FMargin(0.0f));
			Slot->SetZOrder(ZOrder);
		}
	};
	EnsureLayer(PileFrameLayer, TEXT("PileFrameLayer"), 1000);
	EnsureLayer(StaticCardLayer, TEXT("StaticCardLayer"), 2000);
	EnsureLayer(SettlementLayer, TEXT("SettlementLayer"), 8000);
	if (!CarryRoot && WorkspaceCanvas)
	{
		CarryRoot = WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("CarryRoot"));
		CarryRoot->SetClipping(EWidgetClipping::Inherit);
		if (UCanvasPanelSlot* CarryRootSlot = WorkspaceCanvas->AddChildToCanvas(CarryRoot))
		{
			CarryRootSlot->SetAnchors(FAnchors(0.0f, 0.0f));
			CarryRootSlot->SetAlignment(FVector2D::ZeroVector);
			CarryRootSlot->SetPosition(FVector2D::ZeroVector);
			CarryRootSlot->SetSize(FVector2D(1.0f, 1.0f));
			CarryRootSlot->SetZOrder(10000);
		}
	}
	if (CarryRoot)
	{
		CarryRoot->SetClipping(EWidgetClipping::Inherit);
		if (UCanvasPanelSlot* CarryRootSlot = Cast<UCanvasPanelSlot>(CarryRoot->Slot))
		{
			// RenderTranslation moves a point-sized, unclipped composition anchor.
			// A full-workspace carry root expands every transform invalidation to the
			// entire backpack and makes Retainer composition visibly trail the cursor.
			CarryRootSlot->SetAnchors(FAnchors(0.0f, 0.0f));
			CarryRootSlot->SetAlignment(FVector2D::ZeroVector);
			CarryRootSlot->SetPosition(FVector2D::ZeroVector);
			CarryRootSlot->SetSize(FVector2D(1.0f, 1.0f));
			CarryRootSlot->SetZOrder(10000);
		}
	}
	if (!CarryCache)
	{
		CarryCache = WidgetTree->ConstructWidget<UInvalidationBox>(
			UInvalidationBox::StaticClass(), TEXT("CarryCache"));
		CarryCache->SetCanCache(true);
	}
	if (CarryRoot && CarryCache && CarryCache->GetParent() != CarryRoot)
	{
		CarryCache->RemoveFromParent();
		if (UCanvasPanelSlot* CarryCacheSlot = CarryRoot->AddChildToCanvas(CarryCache))
		{
			CarryCacheSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			CarryCacheSlot->SetOffsets(FMargin(0.0f));
			CarryCacheSlot->SetZOrder(0);
		}
	}
	if (!CarryLayer)
	{
		CarryLayer = WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("CarryLayer"));
		CarryLayer->SetClipping(EWidgetClipping::Inherit);
	}
	if (CarryCache && CarryLayer && CarryLayer->GetParent() != CarryCache)
	{
		CarryLayer->RemoveFromParent();
		CarryCache->SetContent(CarryLayer);
	}
	if (CarryCache)
	{
		CarryCache->SetCanCache(true);
	}
	if (!CarryActiveLayer)
	{
		CarryActiveLayer = WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("CarryActiveLayer"));
		CarryActiveLayer->SetClipping(EWidgetClipping::Inherit);
	}
	if (CarryRoot && CarryActiveLayer && CarryActiveLayer->GetParent() != CarryRoot)
	{
		CarryActiveLayer->RemoveFromParent();
		if (UCanvasPanelSlot* ActiveSlot = CarryRoot->AddChildToCanvas(CarryActiveLayer))
		{
			ActiveSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			ActiveSlot->SetOffsets(FMargin(0.0f));
			ActiveSlot->SetZOrder(1);
		}
	}

	if (!SelectionMarquee)
	{
		SelectionMarquee = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SelectionMarquee"));
		SelectionMarquee->SetBrushColor(FLinearColor(0.2f, 0.72f, 1.0f, 0.2f));
		SelectionMarquee->SetVisibility(ESlateVisibility::Collapsed);
		if (UCanvasPanelSlot* MarqueeSlot = Root->AddChildToCanvas(SelectionMarquee))
		{
			MarqueeSlot->SetPosition(FVector2D::ZeroVector);
			MarqueeSlot->SetSize(FVector2D::ZeroVector);
			MarqueeSlot->SetZOrder(100000);
		}
	}

	if (!EmptyStateText)
	{
		EmptyStateText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EmptyStateText"));
		EmptyStateText->SetText(LOCTEXT("EmptyWorkspace", "通量区暂无卡牌"));
		EmptyStateText->SetJustification(ETextJustify::Center);
		if (UCanvasPanelSlot* EmptySlot = Root->AddChildToCanvas(EmptyStateText))
		{
			EmptySlot->SetAnchors(FAnchors(0.5f, 0.5f));
			EmptySlot->SetAlignment(FVector2D(0.5f, 0.5f));
			EmptySlot->SetAutoSize(true);
			EmptySlot->SetZOrder(100001);
		}
	}
}

UCanvasPanel* UWacomBackpackWorkspaceWidget::GetStaticCardLayer()
{
	EnsureFallbackTree();
	return StaticCardLayer;
}

UCanvasPanel* UWacomBackpackWorkspaceWidget::GetCarryCanvas()
{
	EnsureFallbackTree();
	return CarryLayer;
}

UCanvasPanel* UWacomBackpackWorkspaceWidget::GetCarryActiveCanvas()
{
	EnsureFallbackTree();
	return CarryActiveLayer;
}

UCanvasPanel* UWacomBackpackWorkspaceWidget::GetSettlementCanvas()
{
	EnsureFallbackTree();
	return SettlementLayer;
}

UCanvasPanel* UWacomBackpackWorkspaceWidget::GetPileCanvas()
{
	EnsureFallbackTree();
	return PileFrameLayer ? PileFrameLayer.Get() : WorkspaceCanvas.Get();
}

FWacomBackpackWorkspaceRuntime& UWacomBackpackWorkspaceWidget::GetRuntime()
{
	if (!Runtime)
	{
		Runtime = MakeShared<FWacomBackpackWorkspaceRuntime>();
	}
	return *Runtime;
}

const FWacomBackpackWorkspaceRuntime&
UWacomBackpackWorkspaceWidget::GetRuntime() const
{
	return const_cast<UWacomBackpackWorkspaceWidget*>(this)->GetRuntime();
}

const TArray<TWeakObjectPtr<UWacomBackpackZonePileWidget>>&
UWacomBackpackWorkspaceWidget::GetRegisteredPileWidgets() const
{
	return GetRuntime().Visuals.GetPileWidgets();
}

void UWacomBackpackWorkspaceWidget::SetPresentedContentZone(EZoneKind Zone, FGuid OwnerInstanceId)
{
	PresentedContentZone = Zone;
	PresentedContentOwnerInstanceId = Zone == EZoneKind::SpecialZone ? OwnerInstanceId : FGuid();
}

void UWacomBackpackWorkspaceWidget::ApplyCardLayout(
	UWidget& CardWidget,
	FVector2D CardCenter,
	FVector2D CardSize,
	float AngleDegrees,
	int32 ZOrder)
{
	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(CardWidget.Slot);
	if (!CanvasSlot)
	{
		return;
	}
	CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f));
	CanvasSlot->SetAlignment(FVector2D::ZeroVector);
	const FVector2D PixelAlignedPosition(
		FMath::RoundToFloat(CardCenter.X - CardSize.X * 0.5f),
		FMath::RoundToFloat(CardCenter.Y - CardSize.Y * 0.5f));
	CanvasSlot->SetPosition(PixelAlignedPosition);
	CanvasSlot->SetSize(CardSize);
	CanvasSlot->SetZOrder(ZOrder);
	CardWidget.SetPixelSnapping(EWidgetPixelSnapping::SnapToPixel);
	CardWidget.SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	CardWidget.SetRenderTransformAngle(AngleDegrees);
}

void UWacomBackpackWorkspaceWidget::ApplyCardBaseLayout(
	UWidget& CardWidget,
	FVector2D CardCenter,
	FVector2D CardSize,
	float AngleDegrees,
	int32 ZOrder)
{
	if (const UWacomDeckCardWidget* DeckCard = Cast<UWacomDeckCardWidget>(&CardWidget))
	{
		UWacomDeckCardWidget* MutableDeckCard = const_cast<UWacomDeckCardWidget*>(DeckCard);
		FBaseCardLayout Target;
		Target.Center = CardCenter;
		Target.Size = CardSize;
		Target.AngleDegrees = AngleDegrees;
		Target.ZOrder = ZOrder;
		const bool bInCarryLayer = IsInCarryVisualLayer(MutableDeckCard);
		const bool bPendingReleasedHandoff =
			PendingReleasedVisualHandoffs.Contains(MutableDeckCard->GetCardInstanceId());
		bool bStillCarried = false;
		if (bInCarryLayer && InteractionModel && InteractionModel->IsCarrying())
		{
			const FWacomBackpackWorkspaceCarryState& Carry = InteractionModel->GetCarry();
			bStillCarried = Carry.RemainingInstanceIds.Contains(
				MutableDeckCard->GetCardInstanceId());
		}
		if (bPendingReleasedHandoff || (bInCarryLayer && !bStillCarried))
		{
			const FCardVisualPose StartPose = PendingReleasedVisualPoses.Contains(
				MutableDeckCard->GetCardInstanceId())
				? PendingReleasedVisualPoses.FindChecked(MutableDeckCard->GetCardInstanceId())
				: CaptureCardVisualPose(*MutableDeckCard);
			PendingReleasedVisualHandoffs.Remove(MutableDeckCard->GetCardInstanceId());
			PendingReleasedVisualPoses.Remove(MutableDeckCard->GetCardInstanceId());
			BaseCardLayoutTransitions.Remove(MutableDeckCard);
			BaseCardLayouts.Add(MutableDeckCard, Target);
			const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
				? InteractionStyle.Get()
				: GetDefault<UWacomBackpackWorkspaceStyle>();
			if (!bSimplifiedMotion && Style->SettleSeconds > 0.0f && SettlementLayer)
			{
				MutableDeckCard->RemoveFromParent();
				SettlementLayer->AddChildToCanvas(MutableDeckCard);
				ApplyCardLayout(
					*MutableDeckCard,
					Target.Center,
					Target.Size,
					Target.AngleDegrees,
					Target.ZOrder);
				SettlementTargets.Add(MutableDeckCard, Target);
				GetRuntime().Motion.BeginSettlement(
					*MutableDeckCard,
					RotateVector(StartPose.Center - Target.Center, -Target.AngleDegrees),
					FMath::FindDeltaAngleDegrees(Target.AngleDegrees, StartPose.AngleDegrees),
					Style->SettleSeconds,
					false);
				StartCardMotionTimer();
			}
			else
			{
				if (StaticCardLayer)
				{
					MutableDeckCard->RemoveFromParent();
					StaticCardLayer->AddChildToCanvas(MutableDeckCard);
				}
				MutableDeckCard->ResetBackpackLocalMotionPose();
				ApplyCardLayout(
					*MutableDeckCard,
					Target.Center,
					Target.Size,
					Target.AngleDegrees,
					Target.ZOrder);
			}
			bCarryStripLayoutDirty = true;
			return;
		}
		const FBaseCardLayout* Previous = BaseCardLayouts.Find(MutableDeckCard);
		const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
			? InteractionStyle.Get()
			: GetDefault<UWacomBackpackWorkspaceStyle>();
		const bool bChanged = Previous
			&& (!Previous->Center.Equals(Target.Center, 0.5f)
				|| !FMath::IsNearlyEqual(Previous->AngleDegrees, Target.AngleDegrees, 0.1f));
		if (bChanged && !bSimplifiedMotion && Style->PileExpandSeconds > 0.0f)
		{
			const FBaseCardLayoutTransition* ExistingTransition = BaseCardLayoutTransitions.Find(MutableDeckCard);
			const FBaseCardLayout Start = ExistingTransition
				? ExistingTransition->Current
				: *Previous;
			FBaseCardLayoutTransition& Transition = BaseCardLayoutTransitions.FindOrAdd(MutableDeckCard);
			Transition.Start = Start;
			Transition.Current = Start;
			Transition.Target = Target;
			Transition.ElapsedSeconds = 0.0f;
			Transition.DurationSeconds = Style->PileExpandSeconds;
			StartCardMotionTimer();
		}
		else if (bChanged || !Previous || bSimplifiedMotion || Style->PileExpandSeconds <= 0.0f)
		{
			BaseCardLayoutTransitions.Remove(MutableDeckCard);
		}
		else if (FBaseCardLayoutTransition* ExistingTransition =
			BaseCardLayoutTransitions.Find(MutableDeckCard))
		{
			// Geometry stabilisation can reconcile the same expanded scene before the
			// current transition completes. Preserve its elapsed path instead of
			// snapping the card to an identical target on the next presentation pass.
			ExistingTransition->Target = Target;
		}
		BaseCardLayouts.Add(MutableDeckCard, Target);
	}
}

bool UWacomBackpackWorkspaceWidget::HasCardBaseLayout(const UWidget& CardWidget) const
{
	const UWacomDeckCardWidget* DeckCard = Cast<UWacomDeckCardWidget>(&CardWidget);
	return DeckCard && BaseCardLayouts.Contains(
		const_cast<UWacomDeckCardWidget*>(DeckCard));
}

void UWacomBackpackWorkspaceWidget::PrimeCardBaseLayout(
	UWidget& CardWidget,
	FVector2D CardCenter,
	FVector2D CardSize,
	float AngleDegrees,
	int32 ZOrder)
{
	const UWacomDeckCardWidget* DeckCard = Cast<UWacomDeckCardWidget>(&CardWidget);
	if (!DeckCard || BaseCardLayouts.Contains(const_cast<UWacomDeckCardWidget*>(DeckCard)))
	{
		return;
	}
	FBaseCardLayout Base;
	Base.Center = CardCenter;
	Base.Size = CardSize;
	Base.AngleDegrees = AngleDegrees;
	Base.ZOrder = ZOrder;
	BaseCardLayouts.Add(const_cast<UWacomDeckCardWidget*>(DeckCard), Base);
	ApplyCardLayout(CardWidget, CardCenter, CardSize, AngleDegrees, ZOrder);
}

bool UWacomBackpackWorkspaceWidget::TickBaseCardLayoutTransitions(float DeltaSeconds)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(Wacom_Backpack_BaseLayoutTransitions);
	if (BaseCardLayoutTransitions.IsEmpty())
	{
		return false;
	}
#if WITH_AUTOMATION_TESTS
	++BaseCardLayoutTransitionTickCount;
#endif
	const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
		? InteractionStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	for (auto It = BaseCardLayoutTransitions.CreateIterator(); It; ++It)
	{
		UWacomDeckCardWidget* CardWidget = It.Key().Get();
		if (!CardWidget)
		{
			It.RemoveCurrent();
			continue;
		}

		FBaseCardLayoutTransition& Transition = It.Value();
		Transition.ElapsedSeconds += FMath::Max(0.0f, DeltaSeconds);
		const float Alpha = bSimplifiedMotion
			? 1.0f
			: (Transition.DurationSeconds > 0.0f
				? FMath::Clamp(
					Transition.ElapsedSeconds / Transition.DurationSeconds,
					0.0f,
					1.0f)
				: 1.0f);
		const float Smoothed = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);
		Transition.Current.Center = FMath::Lerp(
			Transition.Start.Center, Transition.Target.Center, Smoothed);
		Transition.Current.Size = Transition.Target.Size;
		Transition.Current.AngleDegrees = FMath::Lerp(
			Transition.Start.AngleDegrees, Transition.Target.AngleDegrees, Smoothed);
		Transition.Current.ZOrder = Transition.Target.ZOrder;
		ApplyStaticCardPresentation(*CardWidget, *Style);
#if WITH_AUTOMATION_TESTS
		++BaseCardLayoutTransitionApplyCount;
#endif
		if (Alpha >= 1.0f)
		{
			It.RemoveCurrent();
		}
	}
	return !BaseCardLayoutTransitions.IsEmpty();
}

void UWacomBackpackWorkspaceWidget::SetEmptyStateVisible(bool bVisible)
{
	EnsureFallbackTree();
	if (EmptyStateText)
	{
		EmptyStateText->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

#if WITH_AUTOMATION_TESTS
FWacomBackpackWorkspaceAutomationTestView UWacomBackpackWorkspaceWidget::GetAutomationTestView() const
{
	FWacomBackpackWorkspaceAutomationTestView View;
	View.bHasActiveZone = true;
	View.ActiveZone = PresentedContentZone;
	View.ActiveZoneOwnerInstanceId = PresentedContentOwnerInstanceId;
	View.ManualLayoutCount = ManualLayoutCount;
	View.PileCount = GetRegisteredPileWidgets().Num();
	View.bDeferredCardFaceRenderPending =
		bDeferredCardFaceRenderRequested || bDeferredCardFaceRenderActive;
	View.DeferredCardFaceRenderPassCount = DeferredCardFaceRenderPassCount;
	View.bCardFaceRetainedRenderingEnabled = bCardFaceRetainedRenderingEnabled;
	View.CarryAnchorLocal = CarryAnchorLocal;
	View.CarryVisualAnchorLocal = CarryVisualAnchorLocal;
	View.CarryRootTranslation = CarryRoot ? CarryRoot->GetRenderTransform().Translation : FVector2D::ZeroVector;
	View.CarryCacheTranslation = CarryCache ? CarryCache->GetRenderTransform().Translation : FVector2D::ZeroVector;
	View.CachedCarryCardCount = CarryLayer ? CarryLayer->GetChildrenCount() : 0;
	View.ActiveCarryCardCount = CarryActiveLayer ? CarryActiveLayer->GetChildrenCount() : 0;
	View.SettlementCardCount = SettlementLayer ? SettlementLayer->GetChildrenCount() : 0;
	View.ActiveLocalMotionCardCount = Runtime
		? GetRuntime().Motion.GetMovingCardCount()
		: 0;
	View.RealtimeCardCount = Runtime
		? GetRuntime().Motion.GetRealtimeCardCount()
		: 0;
	View.CarryStripLayoutRebuildCount = CarryStripLayoutRebuildCount;
	View.StaticCardPresentationUpdateCount = StaticCardPresentationUpdateCount;
	View.CarryVisualAnchorApplyCount = CarryVisualAnchorApplyCount;
	View.ActiveBaseCardLayoutTransitionCount = BaseCardLayoutTransitions.Num();
	View.ExpandedPileFocusIndex = ExpandedPileFocus.FocusIndex;
	View.ExpandedPileFocusLayoutRebuildCount = ExpandedPileFocusLayoutRebuildCount;
	View.bExpandedPileFocusExitPending = ExpandedPileFocus.bExitPending;
	View.FullPresentationRefreshCount = FullPresentationRefreshCount;
	View.WorkspaceSceneBindCount = WorkspaceSceneBindCount;
	View.BaseCardLayoutTransitionTickCount = BaseCardLayoutTransitionTickCount;
	View.BaseCardLayoutTransitionApplyCount = BaseCardLayoutTransitionApplyCount;
	View.bCardMotionTimerActive = bCardMotionTimerActive;
	View.ActiveBaseCardLayoutTransitionTargetCenters.Reserve(BaseCardLayoutTransitions.Num());
	for (const TPair<TWeakObjectPtr<UWacomDeckCardWidget>, FBaseCardLayoutTransition>& Pair :
		BaseCardLayoutTransitions)
	{
		View.ActiveBaseCardLayoutTransitionTargetCenters.Add(Pair.Value.Target.Center);
	}
	View.bHasExpandedContentBounds = bHasExpandedContentBounds;
	View.bPileCollapseAnimationPending = bPileCollapseAnimationPending;
	View.ExpandedContentZone = ExpandedContentZone;
	View.ExpandedContentOwnerInstanceId = ExpandedContentOwnerInstanceId;
	if (InteractionModel)
	{
		View.SelectedInstanceIds = InteractionModel->GetSelection().OrderedSelectedInstanceIds;
		View.CarriedInstanceIds = InteractionModel->GetCarry().RemainingInstanceIds;
		View.CurrentCarryIndex = InteractionModel->GetCarry().CurrentIndex;
		View.DefaultCarryIndex = InteractionModel->GetCarry().DefaultIndex;
		View.bInitialReleaseGuardArmed = InteractionModel->GetCarry().bInitialReleaseGuardArmed;
		View.bMouseCaptured = InteractionModel->IsMouseCaptured();
	}
	return View;
}
#endif

#undef LOCTEXT_NAMESPACE
