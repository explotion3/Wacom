// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/InvalidationBox.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "UI/Backpack/WacomBackpackCardWidgetTransfer.h"
#include "UI/Backpack/WacomBackpackWorkspaceAccessibility.h"
#include "UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "UI/Backpack/WacomBackpackWorkspaceLayoutSolver.h"
#include "UI/Backpack/WacomBackpackWorkspaceOverlayPainter.h"
#include "UI/Backpack/WacomBackpackWorkspaceRuntime.h"
#include "UI/Backpack/WacomBackpackWorkspaceRuntimeHost.h"
#include "UI/Backpack/WacomBackpackWorkspaceSceneBuilder.h"
#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"
#include "UI/Backpack/WacomBackpackZonePileWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "UI/Card/WacomCardMotionKernel.h"
#include "UI/Card/WacomFirstPersonCardPlayedDissolveStyle.h"

#define LOCTEXT_NAMESPACE "WacomBackpackWorkspace"

namespace
{
constexpr float LayoutGeometryTolerance = 0.5f;
constexpr int32 RequiredStableLayoutSamples = 2;
constexpr float ExpandedPilePointerStationaryTolerance = 0.25f;
constexpr int32 SaleDepartureZOrderBase = 20000;

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

FSlateRect BuildRotatedCardBounds(FVector2D Center, FVector2D Size, float AngleDegrees)
{
	const float Radians = FMath::DegreesToRadians(AngleDegrees);
	const float AbsCos = FMath::Abs(FMath::Cos(Radians));
	const float AbsSin = FMath::Abs(FMath::Sin(Radians));
	const FVector2D HalfSize = Size * 0.5f;
	const FVector2D AxisAlignedHalfSize(
		AbsCos * HalfSize.X + AbsSin * HalfSize.Y,
		AbsSin * HalfSize.X + AbsCos * HalfSize.Y);
	return FSlateRect(
		Center.X - AxisAlignedHalfSize.X,
		Center.Y - AxisAlignedHalfSize.Y,
		Center.X + AxisAlignedHalfSize.X,
		Center.Y + AxisAlignedHalfSize.Y);
}

TArray<FGuid> BuildChangedInstanceIds(
	TConstArrayView<FGuid> Before,
	TConstArrayView<FGuid> After)
{
	TSet<FGuid> BeforeSet;
	TSet<FGuid> AfterSet;
	BeforeSet.Reserve(Before.Num());
	AfterSet.Reserve(After.Num());
	for (const FGuid InstanceId : Before)
	{
		BeforeSet.Add(InstanceId);
	}
	for (const FGuid InstanceId : After)
	{
		AfterSet.Add(InstanceId);
	}
	TArray<FGuid> Changed;
	Changed.Reserve(BeforeSet.Num() + AfterSet.Num());
	for (const FGuid InstanceId : BeforeSet)
	{
		if (!AfterSet.Contains(InstanceId))
		{
			Changed.Add(InstanceId);
		}
	}
	for (const FGuid InstanceId : AfterSet)
	{
		if (!BeforeSet.Contains(InstanceId))
		{
			Changed.Add(InstanceId);
		}
	}
	return Changed;
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
	if (SelectionMarquee)
	{
		SelectionMarquee->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (StaticCardLayer && !GetRuntime().Presentation.bHasStableLayoutSize)
	{
		StaticCardLayer->SetVisibility(ESlateVisibility::Hidden);
	}
	RequestLayoutGeometryRefresh();
	if (GetRuntime().FrameScheduler.IsDeferredCardFaceRenderPending())
	{
		RequestDeferredCardFaceRender();
	}
	EnsureFrameSchedulerRunning();
}

void UWacomBackpackWorkspaceWidget::NativeDestruct()
{
	SetExpandedPileLensInputLocked(false, false);
	StopFrameScheduler();
	CancelInteraction();
	ResetExpandedPileFocusWindow(false);
	for (FWacomBackpackExpandedPileFocusCard& Entry : GetRuntime().Presentation.ExpandedPileFocus.Cards)
	{
		if (UWacomDeckCardWidget* Card = Entry.Card.Get())
		{
			Card->SetWorkspacePointerPassthrough(false);
		}
	}
	GetRuntime().Presentation.ExpandedPileFocus =
		FWacomBackpackWorkspacePresentationController::FExpandedPileFocusState();
	CancelHoverExpandTimer();
	UnbindWorkspaceCards();
	if (Runtime)
	{
		Runtime->Reset(true);
		Runtime.Reset();
	}
	Super::NativeDestruct();
}

void UWacomBackpackWorkspaceWidget::SetInteractionModel(
	TSharedPtr<FWacomBackpackWorkspaceInteractionModel> InModel,
	UWacomBackpackWorkspaceStyle* InStyle)
{
	InteractionModel = MoveTemp(InModel);
	InteractionStyle = InStyle;
	const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
		? InteractionStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	ResolvedSaleDissolveStyle = Style
		? Style->SaleDissolveStyle.LoadSynchronous()
		: nullptr;
	if (!ResolvedSaleDissolveStyle)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Backpack sale dissolve style could not be resolved during Workspace binding."));
	}
}

void UWacomBackpackWorkspaceWidget::SetSimplifiedMotion(bool bSimplified)
{
	if (GetRuntime().Presentation.IsSimplifiedMotion() == bSimplified)
	{
		return;
	}
	GetRuntime().Presentation.SetSimplifiedMotion(bSimplified);
	if (GetRuntime().Presentation.IsSimplifiedMotion())
	{
		ApplyCarryVisualAnchor(0.0f);
		FWacomBackpackWorkspaceRuntimeHost Host(*this);
		Host.AdvanceBaseCardLayoutTransitions(0.0f);
	}
	if (InteractionModel && InteractionModel->IsCarrying())
	{
		// CarryStrip owns the current-card lift/angle targets.  Motion mode
		// changes must re-solve those targets even when carry membership and
		// the current index are otherwise unchanged.
		GetRuntime().Presentation.bCarryStripLayoutDirty = true;
	}
	RequestPresentationRefresh(
		EWacomBackpackWorkspacePresentationDirty::All,
		{},
		true);
	SyncExpandedPileHitLayouts(!GetVisualState().ExpandedFocusLayouts().IsEmpty());
	if (UWacomDeckCardWidget* FocusedCard = GetPresentationFocusedCard())
	{
		OnBrowseFocusChangedNative.Broadcast(FocusedCard);
	}
	WakeFrameScheduler();
}

FVector2D UWacomBackpackWorkspaceWidget::GetLayoutSpaceSize() const
{
	return GetRuntime().Presentation.bHasStableLayoutSize
		? GetRuntime().Presentation.StableLayoutSize
		: FVector2D(1280.0f, 720.0f);
}

void UWacomBackpackWorkspaceWidget::RequestLayoutGeometryRefresh()
{
	GetRuntime().FrameScheduler.RequestGeometryStabilization();
	EnsureFrameSchedulerRunning();
}

FVector2D UWacomBackpackWorkspaceWidget::ToLocalPointer(const FPointerEvent& Event) const
{
	return GetCachedGeometry().AbsoluteToLocal(Event.GetScreenSpacePosition());
}

void UWacomBackpackWorkspaceWidget::BindWorkspaceCards(
	TConstArrayView<TObjectPtr<UWacomDeckCardWidget>> CardWidgets,
	uint64 StorageRevision)
{
	PrepareForWorkspaceCardReconcile();
	GetRuntime().Visuals.ReplaceOrderedCards(CardWidgets);
	BindRegisteredWorkspaceCards(StorageRevision);
}

void UWacomBackpackWorkspaceWidget::PrepareForWorkspaceCardReconcile()
{
	UnbindWorkspaceCards();
}

void UWacomBackpackWorkspaceWidget::UnbindWorkspaceCards()
{
	for (const TWeakObjectPtr<UWacomDeckCardWidget>& CardWidget : GetBoundCardWidgets())
	{
		if (CardWidget.IsValid())
		{
			CardWidget->UnbindWorkspacePointerEvents();
		}
	}
}

TConstArrayView<TWeakObjectPtr<UWacomDeckCardWidget>>
UWacomBackpackWorkspaceWidget::GetBoundCardWidgets() const
{
	return Runtime
		? Runtime->Visuals.GetCardWidgets()
		: TConstArrayView<TWeakObjectPtr<UWacomDeckCardWidget>>();
}

void UWacomBackpackWorkspaceWidget::BindRegisteredWorkspaceCards(uint64 StorageRevision)
{
#if WITH_AUTOMATION_TESTS
	++WorkspaceSceneBindCount;
#endif
	CurrentStorageRevision = StorageRevision;
	TArray<FWacomBackpackWorkspaceCardHitRecord> HitRecords;
	TSet<UWacomDeckCardWidget*> VisibleWidgets;
	TSet<FGuid> VisibleInstanceIds;
	for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCardWidget : GetBoundCardWidgets())
	{
		UWacomDeckCardWidget* CardWidget = WeakCardWidget.Get();
		if (!CardWidget)
		{
			continue;
		}
		CardWidget->OnWorkspacePointerDownNative.BindUObject(this, &UWacomBackpackWorkspaceWidget::HandleCardPointerDown);
		CardWidget->OnWorkspacePointerMoveNative.BindUObject(this, &UWacomBackpackWorkspaceWidget::HandleCardPointerMove);
		CardWidget->OnWorkspacePointerUpNative.BindUObject(this, &UWacomBackpackWorkspaceWidget::HandleCardPointerUp);
		CardWidget->SetBackpackCardFaceRetainedRenderingEnabled(bCardFaceRetainedRenderingEnabled);
		// Collapsed pile cards deliberately do not accept direct pointer presses, but
		// their physical identities remain selectable by a pile-content marquee.
		// Read-only projections/owners/burdens stay outside the selection model.
		if (CardWidget->IsWorkspaceSelectionEnabled())
		{
			FWacomBackpackWorkspaceCardHitRecord Hit;
			Hit.InstanceId = CardWidget->GetCardInstanceId();
			Hit.SourceZone = FWacomBackpackZoneKey::Make(
				CardWidget->GetWorkspaceDisplayZone(),
				CardWidget->GetWorkspaceDisplayOwnerInstanceId());
			Hit.bMovable = true;
			if (const UCanvasPanelSlot* StaticCardSlot = Cast<UCanvasPanelSlot>(CardWidget->Slot))
			{
				const FWacomBackpackWorkspaceCardVisualPose Visual = CaptureCardVisualPose(*CardWidget);
				Hit.CardCenter = Visual.Center;
				Hit.CardSize = StaticCardSlot->GetSize();
				Hit.AngleDegrees = Visual.AngleDegrees;
				Hit.LayerRank = StaticCardSlot->GetZOrder();
			}
			else if (const FWacomBackpackWorkspaceCardLayout* Base = GetVisualState().BaseLayouts().Find(CardWidget))
			{
				Hit.CardCenter = Base->Center;
				Hit.CardSize = Base->Size;
				Hit.AngleDegrees = Base->AngleDegrees;
				Hit.LayerRank = Base->ZOrder;
			}
			HitRecords.Add(Hit);
		}
		VisibleWidgets.Add(CardWidget);
		VisibleInstanceIds.Add(CardWidget->GetCardInstanceId());
	}
	if (GetVisualState().ReconcileVisibleCards(VisibleWidgets, VisibleInstanceIds))
	{
		GetRuntime().Presentation.SelectionFrozenZone = EZoneKind::Backpack;
		GetRuntime().Presentation.SelectionFrozenOwnerInstanceId.Invalidate();
	}
	if (InteractionModel)
	{
		InteractionModel->ReconcileCards(HitRecords);
		UpdateSelectionVisualFreezeLifetime();
		if (!GetVisualState().SelectionFrozenLayouts().IsEmpty())
		{
			SyncExpandedPileHitLayouts(true);
		}
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
		},
		[this](const UWacomDeckCardWidget* Widget)
		{
			return IsSaleDepartureCard(Widget);
		});
	RequestPresentationRefresh(
		EWacomBackpackWorkspacePresentationDirty::All,
		{},
		true);
	if (StaticCardLayer)
	{
		StaticCardLayer->SetVisibility(
			GetRuntime().Presentation.bHasStableLayoutSize
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Hidden);
	}
	if (GetRuntime().Presentation.bHasStableLayoutSize)
	{
		RequestDeferredCardFaceRender();
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
	if (GetRuntime().Presentation.bHasExpandedContentBounds)
	{
		const FVector2D Local = GetCachedGeometry().AbsoluteToLocal(AbsolutePosition);
		if (Local.X >= GetRuntime().Presentation.ExpandedContentBounds.Left && Local.X <= GetRuntime().Presentation.ExpandedContentBounds.Right
			&& Local.Y >= GetRuntime().Presentation.ExpandedContentBounds.Top && Local.Y <= GetRuntime().Presentation.ExpandedContentBounds.Bottom)
		{
			OutZone = GetRuntime().Presentation.ExpandedContentZone;
			OutOwnerInstanceId = GetRuntime().Presentation.ExpandedContentOwnerInstanceId;
			return true;
		}
	}
	return false;
}

bool UWacomBackpackWorkspaceWidget::FindPileView(
	EZoneKind Zone,
	FGuid OwnerInstanceId,
	FWacomBackpackZonePileView& OutView) const
{
	for (const TWeakObjectPtr<UWacomBackpackZonePileWidget>& WeakPile : GetRegisteredPileWidgets())
	{
		const UWacomBackpackZonePileWidget* Pile = WeakPile.Get();
		if (Pile && Pile->GetPileView().HasSameIdentity(Zone, OwnerInstanceId))
		{
			OutView = Pile->GetPileView();
			return true;
		}
	}
	return false;
}

void UWacomBackpackWorkspaceWidget::SetPileDropFeedback(
	EZoneKind Zone,
	FGuid OwnerInstanceId,
	const FWacomBackpackDropFeedbackView& Feedback)
{
	UWacomBackpackZonePileWidget* TargetPile = nullptr;
	for (const TWeakObjectPtr<UWacomBackpackZonePileWidget>& WeakPile : GetRegisteredPileWidgets())
	{
		UWacomBackpackZonePileWidget* Pile = WeakPile.Get();
		const bool bTarget = Pile
			&& Pile->GetPileView().HasSameIdentity(Zone, OwnerInstanceId);
		if (Pile)
		{
			Pile->SetDropFeedbackView(bTarget
				? Feedback
				: FWacomBackpackDropFeedbackView());
		}
		if (bTarget)
		{
			TargetPile = Pile;
		}
	}
	if (!Feedback.IsVisible() || Feedback.IsRejected()
		|| !TargetPile || TargetPile->GetPileView().bExpanded
		|| !InteractionModel || !InteractionModel->IsCarrying())
	{
		CancelHoverExpandTimer();
		return;
	}
	if (GetRuntime().Presentation.bHoverExpandTimerActive && GetRuntime().Presentation.HoverExpandZone == Zone
		&& (Zone != EZoneKind::SpecialZone || GetRuntime().Presentation.HoverExpandOwnerInstanceId == OwnerInstanceId))
	{
		return;
	}
	CancelHoverExpandTimer();
	GetRuntime().Presentation.bHoverExpandTimerActive = true;
	GetRuntime().Presentation.HoverExpandElapsedSeconds = 0.0f;
	GetRuntime().Presentation.HoverExpandZone = Zone;
	GetRuntime().Presentation.HoverExpandOwnerInstanceId = Zone == EZoneKind::SpecialZone ? OwnerInstanceId : FGuid();
	WakeFrameScheduler();
}

void UWacomBackpackWorkspaceWidget::SetCarryDropFeedbackState(
	bool bValid,
	bool bRejected)
{
	FWacomBackpackWorkspacePresentationController& Presentation =
		GetRuntime().Presentation;
	if (Presentation.bCarryDropValid == bValid
		&& Presentation.bCarryDropRejected == bRejected)
	{
		return;
	}
	Presentation.bCarryDropValid = bValid;
	Presentation.bCarryDropRejected = bRejected;
	const TConstArrayView<FGuid> CarriedIds =
		InteractionModel && InteractionModel->IsCarrying()
		? TConstArrayView<FGuid>(
			InteractionModel->GetCarry().RemainingInstanceIds)
		: TConstArrayView<FGuid>();
	RequestPresentationRefresh(
		EWacomBackpackWorkspacePresentationDirty::Accessibility
			| EWacomBackpackWorkspacePresentationDirty::Paint,
		CarriedIds,
		false,
		false);
}

void UWacomBackpackWorkspaceWidget::CancelHoverExpandTimer()
{
	GetRuntime().Presentation.bHoverExpandTimerActive = false;
	GetRuntime().Presentation.HoverExpandElapsedSeconds = 0.0f;
	GetRuntime().Presentation.HoverExpandZone = EZoneKind::Backpack;
	GetRuntime().Presentation.HoverExpandOwnerInstanceId.Invalidate();
}

void UWacomBackpackWorkspaceWidget::SetExpandedContentBounds(
	EZoneKind Zone,
	FGuid OwnerInstanceId,
	const FSlateRect& LocalBounds)
{
	GetRuntime().Presentation.ExpandedContentZone = Zone;
	GetRuntime().Presentation.ExpandedContentOwnerInstanceId = Zone == EZoneKind::SpecialZone ? OwnerInstanceId : FGuid();
	GetRuntime().Presentation.ExpandedContentBounds = LocalBounds;
	GetRuntime().Presentation.bHasExpandedContentBounds = Zone != EZoneKind::Backpack;
}

void UWacomBackpackWorkspaceWidget::SetExpandedPileFocusContract(
	EZoneKind Zone,
	FGuid OwnerInstanceId,
	const FSlateRect& HeaderRect,
	const FSlateRect& FocusCorridorRect,
	TConstArrayView<FWacomBackpackExpandedPileFocusCard> Cards)
{
	for (FWacomBackpackExpandedPileFocusCard& Entry : GetRuntime().Presentation.ExpandedPileFocus.Cards)
	{
		if (UWacomDeckCardWidget* Card = Entry.Card.Get())
		{
			Card->SetWorkspacePointerPassthrough(false);
		}
	}
	const FGuid NormalizedOwner = Zone == EZoneKind::SpecialZone ? OwnerInstanceId : FGuid();
	bool bSameIdentity = GetRuntime().Presentation.ExpandedPileFocus.Zone == Zone
		&& GetRuntime().Presentation.ExpandedPileFocus.OwnerInstanceId == NormalizedOwner
		&& GetRuntime().Presentation.ExpandedPileFocus.Cards.Num() == Cards.Num();
	if (bSameIdentity)
	{
		for (int32 Index = 0; Index < Cards.Num(); ++Index)
		{
			if (GetRuntime().Presentation.ExpandedPileFocus.Cards[Index].Card != Cards[Index].Card)
			{
				bSameIdentity = false;
				break;
			}
		}
	}
	if (!bSameIdentity)
	{
		ResetExpandedPileFocusWindow(false);
	}
	GetRuntime().Presentation.ExpandedPileFocus.Zone = Zone;
	GetRuntime().Presentation.ExpandedPileFocus.OwnerInstanceId = NormalizedOwner;
	GetRuntime().Presentation.ExpandedPileFocus.HeaderRect = HeaderRect;
	GetRuntime().Presentation.ExpandedPileFocus.CorridorRect = FocusCorridorRect;
	GetRuntime().Presentation.ExpandedPileFocus.Cards.Reset(Cards.Num());
	GetRuntime().Presentation.ExpandedPileFocus.Cards.Append(Cards.GetData(), Cards.Num());
	for (FWacomBackpackExpandedPileFocusCard& Entry : GetRuntime().Presentation.ExpandedPileFocus.Cards)
	{
		if (UWacomDeckCardWidget* Card = Entry.Card.Get())
		{
			Card->SetWorkspacePointerPassthrough(true);
		}
	}
	if (!bSameIdentity)
	{
		GetRuntime().Presentation.ExpandedPileFocus.FocusIndex = INDEX_NONE;
		GetRuntime().Presentation.ExpandedPileFocus.LensFocus = Cards.IsEmpty()
			? 0.0f
			: static_cast<float>(Cards.Num() - 1) * 0.5f;
		GetRuntime().Presentation.ExpandedPileFocus.LensLeftStackCount = 0;
		GetRuntime().Presentation.ExpandedPileFocus.LensExpandedStartIndex = INDEX_NONE;
		GetRuntime().Presentation.ExpandedPileFocus.LensExpandedCardCount = 0;
		GetRuntime().Presentation.ExpandedPileFocus.LensRightStackCount = 0;
		GetRuntime().Presentation.ExpandedPileFocus.bHasLensLayout = false;
		GetVisualState().ExpandedFocusLayouts().Reset();
	}
	if (!GetRuntime().Presentation.ExpandedPileFocus.Cards.IsEmpty())
	{
		RebuildExpandedPileFocusLayout();
	}
}

bool UWacomBackpackWorkspaceWidget::BeginPileCollapseAnimation(
	EZoneKind Zone,
	FGuid OwnerInstanceId)
{
	if (GetRuntime().Presentation.IsSimplifiedMotion()
		|| GetRuntime().Presentation.bPileCollapseAnimationPending || !GetRuntime().Presentation.bHasExpandedContentBounds
		|| GetRuntime().Presentation.ExpandedContentZone != Zone
		|| (Zone == EZoneKind::SpecialZone && GetRuntime().Presentation.ExpandedContentOwnerInstanceId != OwnerInstanceId))
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
	for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard : GetBoundCardWidgets())
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
			Style->GetCardDisplaySize(),
			false,
			Style->PileCollapsedExposurePixels,
			Style->HandLensFullGapPixels,
			Style->HandLensCompressedExposurePixels,
			Style->HandLensMinimumExposurePixels,
			Style->HandLensPromotionOverlapTolerancePixels,
			Style->PileEdgeMarginPixels);
	bool bAnimatedAny = false;
	for (int32 CardIndex = 0; CardIndex < CollapsingCards.Num(); ++CardIndex)
	{
		UWacomDeckCardWidget* Card = CollapsingCards[CardIndex];
		const FWacomBackpackWorkspaceCardLayout* Base = Card ? GetVisualState().BaseLayouts().Find(Card) : nullptr;
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
	TArray<FGuid> ChangedInstanceIds;
	ChangedInstanceIds.Reserve(CollapsingCards.Num());
	for (const UWacomDeckCardWidget* Card : CollapsingCards)
	{
		if (Card)
		{
			ChangedInstanceIds.Add(Card->GetCardInstanceId());
		}
	}
	RequestPresentationRefresh(
		EWacomBackpackWorkspacePresentationDirty::NavigationTargets
			| EWacomBackpackWorkspacePresentationDirty::StaticCards
			| EWacomBackpackWorkspacePresentationDirty::MotionTarget
			| EWacomBackpackWorkspacePresentationDirty::NavigationPresentation
			| EWacomBackpackWorkspacePresentationDirty::Accessibility
			| EWacomBackpackWorkspacePresentationDirty::Paint,
		ChangedInstanceIds);
	if (!bAnimatedAny || GetVisualState().BaseTransitions().IsEmpty()
		|| Style->PileExpandSeconds <= 0.0f)
	{
		return false;
	}
	ResetExpandedPileFocusWindow(true);
	GetRuntime().Presentation.bPileCollapseAnimationPending = true;
	GetRuntime().Presentation.CollapsingPileZone = Zone;
	GetRuntime().Presentation.CollapsingPileOwnerInstanceId = Zone == EZoneKind::SpecialZone ? OwnerInstanceId : FGuid();
	GetRuntime().Presentation.SetCarryInputSuspended(true);
	WakeFrameScheduler();
	return true;
}

void UWacomBackpackWorkspaceWidget::SetHoveredCard(UWacomDeckCardWidget* CardWidget)
{
	TArray<FGuid> ChangedInstanceIds;
	if (const UWacomDeckCardWidget* Previous =
		GetRuntime().Presentation.HoveredCardWidget.Get())
	{
		ChangedInstanceIds.Add(Previous->GetCardInstanceId());
	}
	GetRuntime().Presentation.HoveredCardWidget = CardWidget;
	if (CardWidget)
	{
		ChangedInstanceIds.AddUnique(CardWidget->GetCardInstanceId());
	}
	RequestPresentationRefresh(
		EWacomBackpackWorkspacePresentationDirty::StaticCards
			| EWacomBackpackWorkspacePresentationDirty::MotionTarget,
		ChangedInstanceIds);
	WakeFrameScheduler();
}

void UWacomBackpackWorkspaceWidget::ClearHoveredCard(UWacomDeckCardWidget* CardWidget)
{
	if (GetRuntime().Presentation.HoveredCardWidget.Get() == CardWidget)
	{
		const FGuid ChangedInstanceId = CardWidget
			? CardWidget->GetCardInstanceId()
			: FGuid();
		GetRuntime().Presentation.HoveredCardWidget.Reset();
		RequestPresentationRefresh(
			EWacomBackpackWorkspacePresentationDirty::StaticCards
				| EWacomBackpackWorkspacePresentationDirty::MotionTarget,
			MakeArrayView(&ChangedInstanceId, ChangedInstanceId.IsValid() ? 1 : 0));
		WakeFrameScheduler();
	}
}

bool UWacomBackpackWorkspaceWidget::IsExpandedPileFocusAllowed() const
{
	return InteractionModel
		&& InteractionModel->GetMode() == EWacomBackpackWorkspaceInteractionMode::Idle
		&& InteractionModel->GetSelection().OrderedSelectedInstanceIds.IsEmpty()
		&& !GetRuntime().Presentation.IsCarryInputSuspended()
		&& !GetRuntime().Presentation.bPileCollapseAnimationPending
		&& GetRuntime().Presentation.ExpandedPileFocus.Zone != EZoneKind::Backpack
		&& !GetRuntime().Presentation.ExpandedPileFocus.Cards.IsEmpty();
}

bool UWacomBackpackWorkspaceWidget::IsExpandedPileFocusCard(
	const UWacomDeckCardWidget* CardWidget,
	int32* OutIndex) const
{
	for (int32 Index = 0; Index < GetRuntime().Presentation.ExpandedPileFocus.Cards.Num(); ++Index)
	{
		if (GetRuntime().Presentation.ExpandedPileFocus.Cards[Index].Card.Get() == CardWidget)
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
	if (GetRuntime().Presentation.ExpandedPileFocus.Cards.IsValidIndex(GetRuntime().Presentation.ExpandedPileFocus.FocusIndex))
	{
		return GetRuntime().Presentation.ExpandedPileFocus.Cards[GetRuntime().Presentation.ExpandedPileFocus.FocusIndex].Card.Get();
	}
	return GetRuntime().Presentation.HoveredCardWidget.Get();
}

void UWacomBackpackWorkspaceWidget::UpdateExpandedPileFocus(FVector2D PointerLocal)
{
	const FVector2D PreviousPointerLocal =
		GetRuntime().Presentation.ExpandedPileFocus.PointerLocal;
	GetRuntime().Presentation.ExpandedPileFocus.PointerLocal = PointerLocal;
	if (!IsExpandedPileFocusAllowed())
	{
		ClearExpandedPileFocus(true);
		return;
	}
	// The title/drag handle owns this rectangle even while a focused card is
	// visually returning from its lift. Clear immediately so the card cannot
	// keep the header hidden or steal the next click.
	if (ContainsPoint(GetRuntime().Presentation.ExpandedPileFocus.HeaderRect, PointerLocal))
	{
		ClearExpandedPileFocus(false);
		return;
	}
	UpdateExpandedPileLensFocus(PointerLocal);
	const EExpandedPileHitResolveMode ResolveMode =
		PointerLocal.Equals(
			PreviousPointerLocal,
			ExpandedPilePointerStationaryTolerance)
		? EExpandedPileHitResolveMode::StationaryRetention
		: EExpandedPileHitResolveMode::PointerAcquisition;
	const int32 HitIndex = ResolveExpandedPileVisualHitIndex(
		PointerLocal,
		ResolveMode);
	if (HitIndex != INDEX_NONE)
	{
		GetRuntime().Presentation.ExpandedPileFocus.bExitPending = false;
		GetRuntime().Presentation.ExpandedPileFocus.ExitDelayRemainingSeconds = 0.0f;
		SetExpandedPileFocusIndex(HitIndex);
		GetRuntime().Motion.UpdatePointer(GetCachedGeometry(), PointerLocal, false);
		WakeFrameScheduler();
	}
	else
	{
		BeginExpandedPileFocusExit();
	}
}

void UWacomBackpackWorkspaceWidget::BeginExpandedPileFocusExit()
{
	if (GetRuntime().Presentation.ExpandedPileFocus.FocusIndex == INDEX_NONE || GetRuntime().Presentation.ExpandedPileFocus.bExitPending)
	{
		return;
	}
	const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
		? InteractionStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	if (GetRuntime().Presentation.IsSimplifiedMotion()
		|| Style->FocusExitDelaySeconds <= 0.0f)
	{
		ClearExpandedPileFocus(true);
		return;
	}
	GetRuntime().Presentation.ExpandedPileFocus.bExitPending = true;
	GetRuntime().Presentation.ExpandedPileFocus.ExitDelayRemainingSeconds = Style->FocusExitDelaySeconds;
	WakeFrameScheduler();
}

void UWacomBackpackWorkspaceWidget::SetExpandedPileFocusIndex(int32 FocusIndex)
{
	if (!GetRuntime().Presentation.ExpandedPileFocus.Cards.IsValidIndex(FocusIndex)
		|| GetRuntime().Presentation.ExpandedPileFocus.FocusIndex == FocusIndex)
	{
		return;
	}
	TArray<FGuid> ChangedInstanceIds;
	const int32 PreviousFocusIndex =
		GetRuntime().Presentation.ExpandedPileFocus.FocusIndex;
	if (GetRuntime().Presentation.ExpandedPileFocus.Cards.IsValidIndex(
		PreviousFocusIndex))
	{
		if (const UWacomDeckCardWidget* Previous =
			GetRuntime().Presentation.ExpandedPileFocus.Cards[
				PreviousFocusIndex].Card.Get())
		{
			ChangedInstanceIds.Add(Previous->GetCardInstanceId());
		}
	}
	GetRuntime().Presentation.ExpandedPileFocus.FocusIndex = FocusIndex;
	GetRuntime().Presentation.ExpandedPileFocus.bExitPending = false;
	GetRuntime().Presentation.ExpandedPileFocus.ExitDelayRemainingSeconds = 0.0f;
	UWacomDeckCardWidget* FocusedCard =
		GetRuntime().Presentation.ExpandedPileFocus.Cards[FocusIndex].Card.Get();
	if (FocusedCard)
	{
		ChangedInstanceIds.AddUnique(FocusedCard->GetCardInstanceId());
	}
	OnBrowseFocusChangedNative.Broadcast(FocusedCard);
	RequestPresentationRefresh(
		EWacomBackpackWorkspacePresentationDirty::StaticCards
			| EWacomBackpackWorkspacePresentationDirty::MotionTarget
			| EWacomBackpackWorkspacePresentationDirty::Accessibility
			| EWacomBackpackWorkspacePresentationDirty::Paint,
		ChangedInstanceIds,
		false,
		false);
	WakeFrameScheduler();
}

bool UWacomBackpackWorkspaceWidget::RebuildExpandedPileFocusLayout()
{
	if (GetRuntime().Presentation.ExpandedPileFocus.Cards.IsEmpty())
	{
		return false;
	}
	const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
		? InteractionStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	TArray<FWacomBackpackResolvedLayout> NeutralLayouts;
	NeutralLayouts.Reserve(GetRuntime().Presentation.ExpandedPileFocus.Cards.Num());
	for (const FWacomBackpackExpandedPileFocusCard& Card : GetRuntime().Presentation.ExpandedPileFocus.Cards)
	{
		FWacomBackpackResolvedLayout& Neutral = NeutralLayouts.AddDefaulted_GetRef();
		Neutral.CardCenter = Card.NeutralCenter;
		Neutral.AngleDegrees = Card.NeutralAngleDegrees;
		Neutral.LayerRank = Card.NeutralLayerRank;
	}
	const FWacomBackpackHandLensStripLayout Layout =
		FWacomBackpackWorkspaceLayoutSolver::BuildHandLensStripLayout(
			NeutralLayouts.Num(),
			GetRuntime().Presentation.ExpandedPileFocus.LensFocus,
			GetRuntime().Presentation.ExpandedPileFocus.CorridorRect,
			Style->GetCardDisplaySize(),
			NeutralLayouts,
			Style->HandLensFullGapPixels,
			Style->HandLensCompressedExposurePixels,
			Style->HandLensMinimumExposurePixels,
			Style->HandLensPromotionOverlapTolerancePixels);
	bool bSameTargets = GetVisualState().ExpandedFocusLayouts().Num() == Layout.Cards.Num();
	for (int32 Index = 0; bSameTargets && Index < Layout.Cards.Num(); ++Index)
	{
		UWacomDeckCardWidget* Card = GetRuntime().Presentation.ExpandedPileFocus.Cards[Index].Card.Get();
		const FWacomBackpackWorkspaceCardLayout* Existing = Card
			? GetVisualState().ExpandedFocusLayouts().Find(Card)
			: nullptr;
		bSameTargets = Existing
			&& Existing->Center.Equals(Layout.Cards[Index].CardCenter, 0.01f)
			&& FMath::IsNearlyEqual(
				Existing->AngleDegrees, Layout.Cards[Index].AngleDegrees, 0.01f);
	}
	const bool bSameSegments = GetRuntime().Presentation.ExpandedPileFocus.bHasLensLayout
		&& GetRuntime().Presentation.ExpandedPileFocus.LensLeftStackCount == Layout.LeftStackCount
		&& GetRuntime().Presentation.ExpandedPileFocus.LensExpandedStartIndex == Layout.ExpandedStartIndex
		&& GetRuntime().Presentation.ExpandedPileFocus.LensExpandedCardCount == Layout.ExpandedCardCount
		&& GetRuntime().Presentation.ExpandedPileFocus.LensRightStackCount == Layout.RightStackCount
		&& bSameTargets;
	if (bSameSegments)
	{
		return false;
	}
	GetRuntime().Presentation.ExpandedPileFocus.LensLeftStackCount = Layout.LeftStackCount;
	GetRuntime().Presentation.ExpandedPileFocus.LensExpandedStartIndex = Layout.ExpandedStartIndex;
	GetRuntime().Presentation.ExpandedPileFocus.LensExpandedCardCount = Layout.ExpandedCardCount;
	GetRuntime().Presentation.ExpandedPileFocus.LensRightStackCount = Layout.RightStackCount;
	GetRuntime().Presentation.ExpandedPileFocus.bHasLensLayout = true;
	GetVisualState().ExpandedFocusLayouts().Reset();
	for (int32 Index = 0; Index < Layout.Cards.Num(); ++Index)
	{
		UWacomDeckCardWidget* Card = GetRuntime().Presentation.ExpandedPileFocus.Cards[Index].Card.Get();
		if (!Card)
		{
			continue;
		}
		FWacomBackpackWorkspaceCardLayout& Target = GetVisualState().ExpandedFocusLayouts().Add(Card);
		Target.Center = Layout.Cards[Index].CardCenter;
		Target.Size = Style->GetCardDisplaySize();
		Target.AngleDegrees = Layout.Cards[Index].AngleDegrees;
		Target.ZOrder = 6000 + Layout.Cards[Index].LayerRank;
		if (Layout.VisibleBands.IsValidIndex(Index))
		{
			GetRuntime().Presentation.ExpandedPileFocus.Cards[Index].CurrentHitBand = Layout.VisibleBands[Index];
		}
	}
	++GetRuntime().Presentation.ExpandedPileFocusLayoutRebuildCount;
	SyncExpandedPileHitLayouts(true);
	return true;
}

void UWacomBackpackWorkspaceWidget::SyncExpandedPileHitLayouts(bool bUseFocusedTargets)
{
	if (!InteractionModel || GetRuntime().Presentation.ExpandedPileFocus.Cards.IsEmpty())
	{
		return;
	}
	const FWacomBackpackZoneKey SourceZone = FWacomBackpackZoneKey::Make(
		GetRuntime().Presentation.ExpandedPileFocus.Zone,
		GetRuntime().Presentation.ExpandedPileFocus.OwnerInstanceId);
	TArray<FWacomBackpackWorkspaceCardHitRecord> Updates;
	Updates.Reserve(GetRuntime().Presentation.ExpandedPileFocus.Cards.Num());
	for (int32 Index = 0; Index < GetRuntime().Presentation.ExpandedPileFocus.Cards.Num(); ++Index)
	{
		const FWacomBackpackExpandedPileFocusCard& Entry = GetRuntime().Presentation.ExpandedPileFocus.Cards[Index];
		UWacomDeckCardWidget* Card = Entry.Card.Get();
		if (!Card)
		{
			continue;
		}
		FVector2D Center = Entry.NeutralCenter;
		FVector2D Size = InteractionStyle.IsValid()
			? InteractionStyle->GetCardDisplaySize()
			: GetDefault<UWacomBackpackWorkspaceStyle>()->GetCardDisplaySize();
		float AngleDegrees = Entry.NeutralAngleDegrees;
		int32 LayerRank = Entry.NeutralLayerRank;
		if (const FWacomBackpackWorkspaceCardLayout* Frozen = GetVisualState().SelectionFrozenLayouts().Find(Card))
		{
			Center = Frozen->Center;
			Size = Frozen->Size;
			AngleDegrees = Frozen->AngleDegrees;
			LayerRank = Frozen->ZOrder;
		}
		else if (bUseFocusedTargets)
		{
			if (const FWacomBackpackWorkspaceCardLayout* FocusTarget = GetVisualState().ExpandedFocusLayouts().Find(Card))
			{
				Center = FocusTarget->Center;
				Size = FocusTarget->Size;
				AngleDegrees = FocusTarget->AngleDegrees;
				LayerRank = FocusTarget->ZOrder;
			}
		}
		Updates.Emplace(
			Card->GetCardInstanceId(),
			SourceZone,
			Center,
			LayerRank,
			Card->IsWorkspaceInteractionEnabled());
		Updates.Last().CardSize = Size;
		Updates.Last().AngleDegrees = AngleDegrees;
	}
	InteractionModel->UpdateCardHitLayouts(Updates);
}

void UWacomBackpackWorkspaceWidget::ClearExpandedPileFocus(
	bool bAnimateReturn,
	bool bBroadcastChange)
{
	(void)bAnimateReturn;
	const int32 PreviousFocusIndex =
		GetRuntime().Presentation.ExpandedPileFocus.FocusIndex;
	const bool bHadFocus = PreviousFocusIndex != INDEX_NONE;
	FGuid PreviousFocusInstanceId;
	if (GetRuntime().Presentation.ExpandedPileFocus.Cards.IsValidIndex(
		PreviousFocusIndex))
	{
		if (const UWacomDeckCardWidget* Previous =
			GetRuntime().Presentation.ExpandedPileFocus.Cards[
				PreviousFocusIndex].Card.Get())
		{
			PreviousFocusInstanceId = Previous->GetCardInstanceId();
		}
	}
	GetRuntime().Presentation.ExpandedPileFocus.FocusIndex = INDEX_NONE;
	GetRuntime().Presentation.ExpandedPileFocus.bExitPending = false;
	GetRuntime().Presentation.ExpandedPileFocus.ExitDelayRemainingSeconds = 0.0f;
	SyncExpandedPileHitLayouts(!GetVisualState().ExpandedFocusLayouts().IsEmpty());
	if (bHadFocus && bBroadcastChange)
	{
		OnBrowseFocusChangedNative.Broadcast(nullptr);
	}
	if (bHadFocus)
	{
		RequestPresentationRefresh(
			EWacomBackpackWorkspacePresentationDirty::StaticCards
				| EWacomBackpackWorkspacePresentationDirty::MotionTarget
				| EWacomBackpackWorkspacePresentationDirty::Accessibility
				| EWacomBackpackWorkspacePresentationDirty::Paint,
			MakeArrayView(
				&PreviousFocusInstanceId,
				PreviousFocusInstanceId.IsValid() ? 1 : 0),
			false,
			false);
		WakeFrameScheduler();
	}
}

void UWacomBackpackWorkspaceWidget::UpdateExpandedPileLensFocus(FVector2D PointerLocal)
{
	if (GetRuntime().Presentation.bExpandedPileLensInputLocked
		|| !IsExpandedPileFocusAllowed()
		|| !ContainsPoint(GetRuntime().Presentation.ExpandedPileFocus.CorridorRect, PointerLocal)
		|| GetRuntime().Presentation.ExpandedPileFocus.Cards.IsEmpty())
	{
		return;
	}
	const float CorridorWidth = FMath::Max(
		1.0f,
		GetRuntime().Presentation.ExpandedPileFocus.CorridorRect.Right - GetRuntime().Presentation.ExpandedPileFocus.CorridorRect.Left);
	const float Normalized = FMath::Clamp(
		(PointerLocal.X - GetRuntime().Presentation.ExpandedPileFocus.CorridorRect.Left) / CorridorWidth,
		0.0f,
		1.0f);
	GetRuntime().Presentation.ExpandedPileFocus.LensFocus = Normalized
		* static_cast<float>(GetRuntime().Presentation.ExpandedPileFocus.Cards.Num() - 1);
	if (RebuildExpandedPileFocusLayout())
	{
		TArray<FGuid> ChangedInstanceIds;
		for (const FWacomBackpackExpandedPileFocusCard& Entry :
			GetRuntime().Presentation.ExpandedPileFocus.Cards)
		{
			if (const UWacomDeckCardWidget* Card = Entry.Card.Get())
			{
				ChangedInstanceIds.Add(Card->GetCardInstanceId());
			}
		}
		RequestPresentationRefresh(
			EWacomBackpackWorkspacePresentationDirty::StaticCards
				| EWacomBackpackWorkspacePresentationDirty::MotionTarget,
			ChangedInstanceIds,
			false,
			false);
		WakeFrameScheduler();
	}
}

void UWacomBackpackWorkspaceWidget::SetExpandedPileLensInputLocked(
	bool bLocked,
	bool bResumeImmediately)
{
	if (bLocked)
	{
		if (!GetRuntime().Presentation.bExpandedPileLensInputLocked && IsExpandedPileFocusAllowed())
		{
			GetRuntime().Presentation.bExpandedPileLensInputLocked = true;
		}
		return;
	}

	if (!GetRuntime().Presentation.bExpandedPileLensInputLocked)
	{
		return;
	}
	GetRuntime().Presentation.bExpandedPileLensInputLocked = false;
	if (bResumeImmediately
		&& IsExpandedPileFocusAllowed()
		&& ContainsPoint(GetRuntime().Presentation.ExpandedPileFocus.CorridorRect, GetRuntime().Presentation.ExpandedPileFocus.PointerLocal)
		&& !ContainsPoint(GetRuntime().Presentation.ExpandedPileFocus.HeaderRect, GetRuntime().Presentation.ExpandedPileFocus.PointerLocal))
	{
		UpdateExpandedPileFocus(GetRuntime().Presentation.ExpandedPileFocus.PointerLocal);
	}
}

void UWacomBackpackWorkspaceWidget::SyncExpandedPileLensInputLockFromPointerEvent(
	const FPointerEvent& PointerEvent)
{
	const bool bLeftShiftDown = PointerEvent.GetModifierKeys().IsLeftShiftDown();
	if (bLeftShiftDown)
	{
		SetExpandedPileLensInputLocked(true, false);
	}
	else if (GetRuntime().Presentation.bExpandedPileLensInputLocked)
	{
		SetExpandedPileLensInputLocked(false, true);
	}
}

int32 UWacomBackpackWorkspaceWidget::ResolveExpandedPileVisualHitIndex(
	FVector2D PointerLocal,
	EExpandedPileHitResolveMode ResolveMode) const
{
	const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
		? InteractionStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	auto ContainsVisualCard = [this, PointerLocal](
		int32 Index,
		FVector2D Margin)
	{
		if (!GetRuntime().Presentation.ExpandedPileFocus.Cards.IsValidIndex(Index))
		{
			return false;
		}
		UWacomDeckCardWidget* Card = GetRuntime().Presentation.ExpandedPileFocus.Cards[Index].Card.Get();
		const UCanvasPanelSlot* CardSlot = Card ? Cast<UCanvasPanelSlot>(Card->Slot) : nullptr;
		if (!Card || !CardSlot)
		{
			return false;
		}
		const FWacomBackpackWorkspaceCardVisualPose VisualPose = CaptureCardVisualPose(*Card);
		const FVector2D VisualDelta = RotateVector(
			PointerLocal - VisualPose.Center,
			-VisualPose.AngleDegrees);
		const FVector2D HalfSize = CardSlot->GetSize() * 0.5f;
		return FMath::Abs(VisualDelta.X)
				<= HalfSize.X + FMath::Max(0.0f, Margin.X)
			&& FMath::Abs(VisualDelta.Y)
				<= HalfSize.Y + FMath::Max(0.0f, Margin.Y);
	};
	auto ContainsStableFocusBand = [this, PointerLocal](
		int32 Index,
		float Margin)
	{
		if (!GetRuntime().Presentation.ExpandedPileFocus.Cards.IsValidIndex(Index))
		{
			return false;
		}
		const FSlateRect& StableBand =
			GetRuntime().Presentation.ExpandedPileFocus.Cards[Index].CurrentHitBand;
		if (StableBand.Right <= StableBand.Left
			|| StableBand.Bottom <= StableBand.Top)
		{
			return false;
		}
		const float SafeMargin = FMath::Max(0.0f, Margin);
		return PointerLocal.X >= StableBand.Left - SafeMargin
			&& PointerLocal.X <= StableBand.Right + SafeMargin
			&& PointerLocal.Y >= StableBand.Top - SafeMargin
			&& PointerLocal.Y <= StableBand.Bottom + SafeMargin;
	};
	auto ContainsTargetCard = [this, PointerLocal](
		int32 Index,
		FVector2D Margin)
	{
		if (!GetRuntime().Presentation.ExpandedPileFocus.Cards.IsValidIndex(Index))
		{
			return false;
		}
		const UWacomDeckCardWidget* Card =
			GetRuntime().Presentation.ExpandedPileFocus.Cards[Index].Card.Get();
		const FWacomBackpackWorkspaceCardLayout* Target = Card
			? GetVisualState().ExpandedFocusLayouts().Find(Card)
			: nullptr;
		if (!Target)
		{
			return false;
		}
		const FVector2D TargetDelta = RotateVector(
			PointerLocal - Target->Center,
			-Target->AngleDegrees);
		const FVector2D HalfSize = Target->Size * 0.5f;
		return FMath::Abs(TargetDelta.X)
				<= HalfSize.X + FMath::Max(0.0f, Margin.X)
			&& FMath::Abs(TargetDelta.Y)
				<= HalfSize.Y + FMath::Max(0.0f, Margin.Y);
	};
	const int32 CurrentFocusIndex =
		GetRuntime().Presentation.ExpandedPileFocus.FocusIndex;
	const bool bHasCurrentFocus =
		GetRuntime().Presentation.ExpandedPileFocus.Cards.IsValidIndex(
			CurrentFocusIndex);
	const float Hysteresis =
		FMath::Max(0.0f, Style->FocusHitHysteresisPixels);
	const FVector2D HysteresisMargin(Hysteresis, Hysteresis);
	auto ShouldRetainCurrentFocus = [&]()
	{
		return bHasCurrentFocus
			&& (ContainsVisualCard(CurrentFocusIndex, HysteresisMargin)
				|| ContainsStableFocusBand(CurrentFocusIndex, Hysteresis)
				|| ContainsTargetCard(CurrentFocusIndex, HysteresisMargin));
	};

	// A cached pointer is re-evaluated every scheduler frame after local motion.
	// Preserve the identity acquired by the last real pointer move while the
	// pointer remains inside that card's stable target body. Otherwise a lifted
	// card reveals an overlapping neighbour, which then receives the Z boost and
	// lift, creating a self-sustaining identity/Z-order loop.
	if (ResolveMode == EExpandedPileHitResolveMode::StationaryRetention
		&& ShouldRetainCurrentFocus())
	{
		return CurrentFocusIndex;
	}

	int32 HitIndex = INDEX_NONE;
	int32 HighestVisualZOrder = MIN_int32;
	for (int32 Index = 0; Index < GetRuntime().Presentation.ExpandedPileFocus.Cards.Num(); ++Index)
	{
		UWacomDeckCardWidget* Card = GetRuntime().Presentation.ExpandedPileFocus.Cards[Index].Card.Get();
		const UCanvasPanelSlot* CardSlot = Card ? Cast<UCanvasPanelSlot>(Card->Slot) : nullptr;
		if (!CardSlot || !ContainsVisualCard(Index, FVector2D::ZeroVector))
		{
			continue;
		}
		const int32 VisualZOrder = CardSlot->GetZOrder();
		if (VisualZOrder > HighestVisualZOrder
			|| (VisualZOrder == HighestVisualZOrder && Index > HitIndex))
		{
			HighestVisualZOrder = VisualZOrder;
			HitIndex = Index;
		}
	}
	if (HitIndex == INDEX_NONE && ShouldRetainCurrentFocus())
	{
		HitIndex = CurrentFocusIndex;
	}
	return HitIndex;
}

void UWacomBackpackWorkspaceWidget::ResetExpandedPileFocusWindow(
	bool bAnimateReturn,
	bool bBroadcastChange)
{
	SetExpandedPileLensInputLocked(false, false);
	EndSelectionVisualFreeze(false);
	const bool bHadFocus = GetRuntime().Presentation.ExpandedPileFocus.FocusIndex != INDEX_NONE;
	const bool bHadWindow = GetRuntime().Presentation.ExpandedPileFocus.bHasLensLayout
		|| !GetVisualState().ExpandedFocusLayouts().IsEmpty();
	GetRuntime().Presentation.ExpandedPileFocus.FocusIndex = INDEX_NONE;
	GetRuntime().Presentation.ExpandedPileFocus.LensFocus = GetRuntime().Presentation.ExpandedPileFocus.Cards.IsEmpty()
		? 0.0f
		: static_cast<float>(GetRuntime().Presentation.ExpandedPileFocus.Cards.Num() - 1) * 0.5f;
	GetRuntime().Presentation.ExpandedPileFocus.LensLeftStackCount = 0;
	GetRuntime().Presentation.ExpandedPileFocus.LensExpandedStartIndex = INDEX_NONE;
	GetRuntime().Presentation.ExpandedPileFocus.LensExpandedCardCount = 0;
	GetRuntime().Presentation.ExpandedPileFocus.LensRightStackCount = 0;
	GetRuntime().Presentation.ExpandedPileFocus.bHasLensLayout = false;
	GetRuntime().Presentation.ExpandedPileFocus.bExitPending = false;
	GetRuntime().Presentation.ExpandedPileFocus.ExitDelayRemainingSeconds = 0.0f;
	GetVisualState().ExpandedFocusLayouts().Reset();
	for (FWacomBackpackExpandedPileFocusCard& Entry : GetRuntime().Presentation.ExpandedPileFocus.Cards)
	{
		Entry.CurrentHitBand = Entry.NeutralHitBand;
	}
	SyncExpandedPileHitLayouts(false);
	if (Runtime)
	{
		const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
			? InteractionStyle.Get()
			: GetDefault<UWacomBackpackWorkspaceStyle>();
		for (const FWacomBackpackExpandedPileFocusCard& Entry : GetRuntime().Presentation.ExpandedPileFocus.Cards)
		{
			if (UWacomDeckCardWidget* Card = Entry.Card.Get())
			{
				GetRuntime().Motion.SetLocalPoseTarget(
					*Card,
					FVector2D::ZeroVector,
					0.0f,
					bAnimateReturn ? Style->HoverExitSeconds : 0.0f,
					GetRuntime().Presentation.IsSimplifiedMotion() || !bAnimateReturn);
			}
		}
	}
	if (bHadFocus && bBroadcastChange)
	{
		OnBrowseFocusChangedNative.Broadcast(nullptr);
	}
	if (bHadFocus || bHadWindow)
	{
		TArray<FGuid> ChangedInstanceIds;
		for (const FWacomBackpackExpandedPileFocusCard& Entry :
			GetRuntime().Presentation.ExpandedPileFocus.Cards)
		{
			if (const UWacomDeckCardWidget* Card = Entry.Card.Get())
			{
				ChangedInstanceIds.Add(Card->GetCardInstanceId());
			}
		}
		RequestPresentationRefresh(
			EWacomBackpackWorkspacePresentationDirty::StaticCards
				| EWacomBackpackWorkspacePresentationDirty::MotionTarget
			| EWacomBackpackWorkspacePresentationDirty::Accessibility
			| EWacomBackpackWorkspacePresentationDirty::Paint,
			ChangedInstanceIds,
			false,
			false);
		WakeFrameScheduler();
	}
}

void UWacomBackpackWorkspaceWidget::BeginSelectionVisualFreeze(
	const FWacomBackpackZoneKey& SourceZone)
{
	SetExpandedPileLensInputLocked(false, false);
	if (InteractionModel)
	{
		TArray<FWacomBackpackWorkspaceCardHitRecord> CurrentVisualHits;
		for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard : GetBoundCardWidgets())
		{
			UWacomDeckCardWidget* Card = WeakCard.Get();
			const UCanvasPanelSlot* CardSlot = Card ? Cast<UCanvasPanelSlot>(Card->Slot) : nullptr;
			if (!Card || !CardSlot || !Card->IsWorkspaceSelectionEnabled()
				|| FWacomBackpackZoneKey::Make(
					Card->GetWorkspaceDisplayZone(),
					Card->GetWorkspaceDisplayOwnerInstanceId()) != SourceZone)
			{
				continue;
			}
			const FWacomBackpackWorkspaceCardVisualPose Visual = CaptureCardVisualPose(*Card);
			FWacomBackpackWorkspaceCardHitRecord& Hit = CurrentVisualHits.AddDefaulted_GetRef();
			Hit.InstanceId = Card->GetCardInstanceId();
			Hit.SourceZone = SourceZone;
			Hit.CardCenter = Visual.Center;
			Hit.CardSize = CardSlot->GetSize();
			Hit.AngleDegrees = Visual.AngleDegrees;
			Hit.LayerRank = CardSlot->GetZOrder();
			Hit.bMovable = true;
		}
		InteractionModel->UpdateCardHitLayouts(CurrentVisualHits);
	}

	const FWacomBackpackZoneKey FocusZone = FWacomBackpackZoneKey::Make(
		GetRuntime().Presentation.ExpandedPileFocus.Zone,
		GetRuntime().Presentation.ExpandedPileFocus.OwnerInstanceId);
	if (!(SourceZone == FocusZone) || GetRuntime().Presentation.ExpandedPileFocus.Cards.IsEmpty())
	{
		EndSelectionVisualFreeze(true);
		return;
	}
	if (!GetVisualState().SelectionFrozenLayouts().IsEmpty()
		&& GetRuntime().Presentation.SelectionFrozenZone == SourceZone.Zone
		&& GetRuntime().Presentation.SelectionFrozenOwnerInstanceId == SourceZone.OwnerInstanceId)
	{
		return;
	}

	EndSelectionVisualFreeze(false);
	GetRuntime().Presentation.SelectionFrozenZone = SourceZone.Zone;
	GetRuntime().Presentation.SelectionFrozenOwnerInstanceId = SourceZone.OwnerInstanceId;
	TArray<FWacomBackpackWorkspaceCardHitRecord> HitUpdates;
	for (const FWacomBackpackExpandedPileFocusCard& Entry : GetRuntime().Presentation.ExpandedPileFocus.Cards)
	{
		UWacomDeckCardWidget* Card = Entry.Card.Get();
		UCanvasPanelSlot* CardSlot = Card ? Cast<UCanvasPanelSlot>(Card->Slot) : nullptr;
		if (!Card || !CardSlot)
		{
			continue;
		}
		const FWacomBackpackWorkspaceCardVisualPose Visual = CaptureCardVisualPose(*Card);
		FWacomBackpackWorkspaceCardLayout& Frozen = GetVisualState().SelectionFrozenLayouts().Add(Card);
		Frozen.Center = Visual.Center;
		Frozen.Size = CardSlot->GetSize();
		Frozen.AngleDegrees = Visual.AngleDegrees;
		Frozen.ZOrder = CardSlot->GetZOrder();
		GetVisualState().BaseTransitions().Remove(Card);
		GetRuntime().Motion.StopLocalPoseMotionPreservingCurrent(*Card);
		Card->ResetBackpackLocalMotionPose();
		ApplyCardLayout(
			*Card,
			Frozen.Center,
			Frozen.Size,
			Frozen.AngleDegrees,
			Frozen.ZOrder);
		if (Card->IsWorkspaceSelectionEnabled())
		{
			HitUpdates.Emplace(
				Card->GetCardInstanceId(),
				SourceZone,
				Frozen.Center,
				Frozen.ZOrder,
				true);
			HitUpdates.Last().CardSize = Frozen.Size;
			HitUpdates.Last().AngleDegrees = Frozen.AngleDegrees;
		}
	}
	if (InteractionModel)
	{
		InteractionModel->UpdateCardHitLayouts(HitUpdates);
	}
	const bool bHadFocus = GetRuntime().Presentation.ExpandedPileFocus.FocusIndex != INDEX_NONE;
	GetRuntime().Presentation.ExpandedPileFocus.FocusIndex = INDEX_NONE;
	GetRuntime().Presentation.ExpandedPileFocus.bExitPending = false;
	GetRuntime().Presentation.ExpandedPileFocus.ExitDelayRemainingSeconds = 0.0f;
	if (bHadFocus)
	{
		OnBrowseFocusChangedNative.Broadcast(nullptr);
	}
}

void UWacomBackpackWorkspaceWidget::EndSelectionVisualFreeze(bool bAnimateReturn)
{
	if (GetVisualState().SelectionFrozenLayouts().IsEmpty())
	{
		GetRuntime().Presentation.SelectionFrozenZone = EZoneKind::Backpack;
		GetRuntime().Presentation.SelectionFrozenOwnerInstanceId.Invalidate();
		return;
	}
	const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
		? InteractionStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	const TMap<TWeakObjectPtr<UWacomDeckCardWidget>, FWacomBackpackWorkspaceCardLayout> FrozenLayouts =
		MoveTemp(GetVisualState().SelectionFrozenLayouts());
	GetVisualState().SelectionFrozenLayouts().Reset();
	GetRuntime().Presentation.SelectionFrozenZone = EZoneKind::Backpack;
	GetRuntime().Presentation.SelectionFrozenOwnerInstanceId.Invalidate();
	for (const TPair<TWeakObjectPtr<UWacomDeckCardWidget>, FWacomBackpackWorkspaceCardLayout>& Pair : FrozenLayouts)
	{
		UWacomDeckCardWidget* Card = Pair.Key.Get();
		const FWacomBackpackWorkspaceCardLayout* Base = Card ? GetVisualState().BaseLayouts().Find(Card) : nullptr;
		if (!Card || !Base || IsInCarryVisualLayer(Card))
		{
			continue;
		}
		FVector2D TargetTranslation = FVector2D::ZeroVector;
		float TargetAngle = 0.0f;
		if (const FWacomBackpackWorkspaceCardLayout* FocusTarget = GetVisualState().ExpandedFocusLayouts().Find(Card))
		{
			TargetTranslation = RotateVector(
				FocusTarget->Center - Base->Center,
				-Base->AngleDegrees);
			TargetAngle = FMath::FindDeltaAngleDegrees(
				Base->AngleDegrees,
				FocusTarget->AngleDegrees);
		}
		ApplyCardLayout(
			*Card,
			Base->Center,
			Base->Size,
			Base->AngleDegrees,
			Base->ZOrder);
		RetargetCardLocalPoseFromVisual(
			*Card,
			FWacomBackpackWorkspaceCardVisualPose{Pair.Value.Center, Pair.Value.AngleDegrees},
			*Base,
			TargetTranslation,
			TargetAngle,
			bAnimateReturn ? Style->FocusReflowSeconds : 0.0f);
	}
	SyncExpandedPileHitLayouts(!GetVisualState().ExpandedFocusLayouts().IsEmpty());
	WakeFrameScheduler();
}

void UWacomBackpackWorkspaceWidget::UpdateSelectionVisualFreezeLifetime()
{
	if (GetVisualState().SelectionFrozenLayouts().IsEmpty() || !InteractionModel)
	{
		return;
	}
	const FWacomBackpackWorkspaceSelectionState& Selection = InteractionModel->GetSelection();
	const FWacomBackpackZoneKey FrozenZone = FWacomBackpackZoneKey::Make(
		GetRuntime().Presentation.SelectionFrozenZone,
		GetRuntime().Presentation.SelectionFrozenOwnerInstanceId);
	if (InteractionModel->IsCarrying()
		|| !Selection.bHasSourceZone
		|| Selection.OrderedSelectedInstanceIds.IsEmpty()
		|| !(Selection.SourceZone == FrozenZone))
	{
		EndSelectionVisualFreeze(!InteractionModel->IsCarrying());
	}
}

void UWacomBackpackWorkspaceWidget::SetCardFaceRetainedRenderingEnabled(bool bEnabled)
{
	bCardFaceRetainedRenderingEnabled = bEnabled;
	for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCardWidget : GetBoundCardWidgets())
	{
		if (UWacomDeckCardWidget* CardWidget = WeakCardWidget.Get())
		{
			CardWidget->SetBackpackCardFaceRetainedRenderingEnabled(bEnabled);
		}
	}
	if (Runtime)
	{
		GetRuntime().SaleDeparture.SetRetainedRenderingEnabled(bEnabled);
	}
	if (bEnabled && GetRuntime().FrameScheduler.IsDeferredCardFaceRenderPending())
	{
		RequestDeferredCardFaceRender();
	}
	else if (!bEnabled)
	{
		GetRuntime().FrameScheduler.SuspendDeferredCardFaceRender();
	}
	WakeFrameScheduler();
}

FReply UWacomBackpackWorkspaceWidget::HandleCardPointerDown(
	UWacomDeckCardWidget* CardWidget,
	const FGeometry& Geometry,
	const FPointerEvent& Event)
{
	return HandleCardPointerDownAtLocal(
		CardWidget, ToLocalPointer(Event), Event, true);
}

FReply UWacomBackpackWorkspaceWidget::HandleCardPointerDownAtLocal(
	UWacomDeckCardWidget* CardWidget,
	FVector2D Pointer,
	const FPointerEvent& Event,
	bool bAllowPileHeaderReroute)
{
	RelinquishSemanticNavigationForPointerInput();
	SyncExpandedPileLensInputLockFromPointerEvent(Event);
	if (!InteractionModel || !CardWidget || GetRuntime().Presentation.IsCarryInputSuspended())
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
	if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		// Static/settling cards render above the pile frame. A lifted or freshly
		// released card can therefore receive the Slate pointer event even though
		// the pointer is inside a pile header. The header owns that semantic region:
		// reroute before card pickup so collapse and title dragging remain reachable.
		if (bAllowPileHeaderReroute
			&& TryBeginPileHeaderPress(Pointer, Event, Event.IsControlDown()))
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
			SetExpandedPileLensInputLocked(false, false);
			EndSelectionVisualFreeze(false);
			GetRuntime().Presentation.bCarryCurrentExplicitlySelectedByWheel = false;
			GetRuntime().Gesture.ClearCardPress();
			ClearExpandedPileFocus(true);
			UpdateCarryAnchor(Pointer);
			GetRuntime().Presentation.bCarryStripLayoutDirty = true;
			BeginCarryPickupFeedback();
			WakeFrameScheduler();
			RequestPresentationRefresh(
				EWacomBackpackWorkspacePresentationDirty::NavigationTargets
					| EWacomBackpackWorkspacePresentationDirty::CarryTopology
					| EWacomBackpackWorkspacePresentationDirty::CarryStrip
					| EWacomBackpackWorkspacePresentationDirty::StaticCards
					| EWacomBackpackWorkspacePresentationDirty::CardSemantics
					| EWacomBackpackWorkspacePresentationDirty::MotionTarget
					| EWacomBackpackWorkspacePresentationDirty::NavigationPresentation
					| EWacomBackpackWorkspacePresentationDirty::Accessibility
					| EWacomBackpackWorkspacePresentationDirty::Paint,
				InteractionModel->GetCarry().RemainingInstanceIds);
			OnInteractionChangedNative.Broadcast();
			return BuildHandledPointerReply();
		}
	}
	if (Event.IsControlDown())
	{
		BeginSelectionVisualFreeze(FWacomBackpackZoneKey::Make(
			CardWidget->GetWorkspaceDisplayZone(),
			CardWidget->GetWorkspaceDisplayOwnerInstanceId()));
	}
	ClearExpandedPileFocus(true);
	InteractionModel->SetCardPressActive(true);
	GetRuntime().Gesture.BeginCardPress(
		CardWidget->GetCardInstanceId(),
		Pointer,
		Event.GetScreenSpacePosition(),
		Event.IsControlDown());
	return FReply::Handled().CaptureMouse(TakeWidget());
}

FReply UWacomBackpackWorkspaceWidget::TryHandleExpandedPileVisualPointerDown(
	FVector2D PointerLocal,
	const FPointerEvent& Event)
{
	if (ContainsPoint(GetRuntime().Presentation.ExpandedPileFocus.HeaderRect, PointerLocal))
	{
		return FReply::Unhandled();
	}
	const EExpandedPileHitResolveMode ResolveMode = PointerLocal.Equals(
		GetRuntime().Presentation.ExpandedPileFocus.PointerLocal,
		ExpandedPilePointerStationaryTolerance)
		? EExpandedPileHitResolveMode::StationaryRetention
		: EExpandedPileHitResolveMode::PointerAcquisition;
	const int32 VisualHitIndex = ResolveExpandedPileVisualHitIndex(
		PointerLocal,
		ResolveMode);
	if (!GetRuntime().Presentation.ExpandedPileFocus.Cards.IsValidIndex(VisualHitIndex))
	{
		return FReply::Unhandled();
	}
	UWacomDeckCardWidget* VisualHitCard =
		GetRuntime().Presentation.ExpandedPileFocus.Cards[VisualHitIndex].Card.Get();
	return HandleCardPointerDownAtLocal(
		VisualHitCard, PointerLocal, Event, false);
}

FReply UWacomBackpackWorkspaceWidget::HandleCardPointerMove(
	UWacomDeckCardWidget* CardWidget,
	const FGeometry& Geometry,
	const FPointerEvent& Event)
{
	RelinquishSemanticNavigationForPointerInput();
	SyncExpandedPileLensInputLockFromPointerEvent(Event);
	if (!InteractionModel || GetRuntime().Presentation.IsCarryInputSuspended())
	{
		return FReply::Unhandled();
	}
	const FVector2D Pointer = ToLocalPointer(Event);
	if (InteractionModel->IsMarqueeActive())
	{
		InteractionModel->UpdateMarquee(Pointer);
		Invalidate(EInvalidateWidgetReason::Paint);
		return BuildHandledPointerReply();
	}
	if (InteractionModel->IsCarrying())
	{
		QueueCarryPointer(Pointer);
		OnInteractionChangedNative.Broadcast();
		return BuildHandledPointerReply();
	}
	if (TryBeginCarryFromPendingPress(Pointer, Event))
	{
		return BuildHandledPointerReply();
	}
	UpdateExpandedPileFocus(Pointer);
	if (GetPresentationFocusedCard())
	{
		GetRuntime().Motion.UpdatePointer(GetCachedGeometry(), Pointer, false);
		WakeFrameScheduler();
	}
	return BuildHandledPointerReply();
}

FReply UWacomBackpackWorkspaceWidget::HandleCardPointerUp(
	UWacomDeckCardWidget* CardWidget,
	const FGeometry& Geometry,
	const FPointerEvent& Event)
{
	RelinquishSemanticNavigationForPointerInput();
	if (!InteractionModel || GetRuntime().Presentation.IsCarryInputSuspended())
	{
		return FReply::Unhandled();
	}
	if (InteractionModel->IsMarqueeActive() && Event.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const TArray<FGuid> PreviousSelection =
			InteractionModel->GetSelection().OrderedSelectedInstanceIds;
		InteractionModel->UpdateMarquee(ToLocalPointer(Event));
		InteractionModel->CompleteMarquee();
		UpdateSelectionVisualFreezeLifetime();
		ClearExpandedPileFocus(true);
		const TArray<FGuid> ChangedInstanceIds = BuildChangedInstanceIds(
			PreviousSelection,
			InteractionModel->GetSelection().OrderedSelectedInstanceIds);
		RequestPresentationRefresh(
			EWacomBackpackWorkspacePresentationDirty::StaticCards
				| EWacomBackpackWorkspacePresentationDirty::CardSemantics
				| EWacomBackpackWorkspacePresentationDirty::MotionTarget
				| EWacomBackpackWorkspacePresentationDirty::Accessibility
				| EWacomBackpackWorkspacePresentationDirty::Paint,
			ChangedInstanceIds);
		OnInteractionChangedNative.Broadcast();
		return FReply::Handled().ReleaseMouseCapture();
	}
	if (InteractionModel->IsCarrying())
	{
		SyncCarryPointerForRelease(ToLocalPointer(Event));
		BroadcastPointerRelease(Event.GetEffectingButton() == EKeys::RightMouseButton);
		return BuildHandledPointerReply();
	}
	const FWacomBackpackPendingCardPress CardPress =
		GetRuntime().Gesture.GetCardPress();
	if (CardPress.bActive && Event.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const TArray<FGuid> PreviousSelection =
			InteractionModel->GetSelection().OrderedSelectedInstanceIds;
		InteractionModel->ClickCard(CardPress.InstanceId, CardPress.bControlDown);
		InteractionModel->SetCardPressActive(false);
		GetRuntime().Gesture.ClearCardPress();
		UpdateSelectionVisualFreezeLifetime();
		const TArray<FGuid> ChangedInstanceIds = BuildChangedInstanceIds(
			PreviousSelection,
			InteractionModel->GetSelection().OrderedSelectedInstanceIds);
		RequestPresentationRefresh(
			EWacomBackpackWorkspacePresentationDirty::StaticCards
				| EWacomBackpackWorkspacePresentationDirty::CardSemantics
				| EWacomBackpackWorkspacePresentationDirty::MotionTarget
				| EWacomBackpackWorkspacePresentationDirty::Accessibility
				| EWacomBackpackWorkspacePresentationDirty::Paint,
			ChangedInstanceIds);
		OnInteractionChangedNative.Broadcast();
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}

void UWacomBackpackWorkspaceWidget::BroadcastRelease(
	bool bReleaseAll,
	EWacomBackpackWorkspaceReleaseTargetKind TargetKind,
	const FWacomBackpackZoneKey& TargetZone)
{
	if (!InteractionModel)
	{
		return;
	}
	const TArray<FGuid> PreviousCarryIds =
		InteractionModel->GetCarry().RemainingInstanceIds;
	const FWacomBackpackWorkspaceReleaseIntent Intent = InteractionModel->BuildReleaseIntent(
		bReleaseAll,
		TargetKind,
		TargetZone);
	const bool bIssuedRelease = !Intent.bConsumedByInitialReleaseGuard && !Intent.InstanceIds.IsEmpty();
	if (bIssuedRelease)
	{
		CaptureReleasedVisualPoses(Intent.InstanceIds);
		for (const FGuid InstanceId : Intent.InstanceIds)
		{
			GetVisualState().MarkReleasedHandoff(InstanceId);
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
					GetVisualState().RemoveReleasedHandoff(InstanceId);
				}
			}
		}
	}
	TArray<FGuid> ChangedInstanceIds = PreviousCarryIds;
	if (InteractionModel->IsCarrying())
	{
		for (const FGuid InstanceId :
			InteractionModel->GetCarry().RemainingInstanceIds)
		{
			ChangedInstanceIds.AddUnique(InstanceId);
		}
	}
	RequestPresentationRefresh(
		EWacomBackpackWorkspacePresentationDirty::NavigationTargets
			| EWacomBackpackWorkspacePresentationDirty::CarryTopology
			| EWacomBackpackWorkspacePresentationDirty::CarryStrip
			| EWacomBackpackWorkspacePresentationDirty::StaticCards
			| EWacomBackpackWorkspacePresentationDirty::CardSemantics
			| EWacomBackpackWorkspacePresentationDirty::MotionTarget
			| EWacomBackpackWorkspacePresentationDirty::NavigationPresentation
			| EWacomBackpackWorkspacePresentationDirty::Accessibility
			| EWacomBackpackWorkspacePresentationDirty::Paint,
		ChangedInstanceIds);
	WakeFrameScheduler();
	OnInteractionChangedNative.Broadcast();
}

void UWacomBackpackWorkspaceWidget::BroadcastPointerRelease(bool bReleaseAll)
{
	BroadcastRelease(
		bReleaseAll,
		EWacomBackpackWorkspaceReleaseTargetKind::Pointer,
		FWacomBackpackZoneKey());
}

bool UWacomBackpackWorkspaceWidget::TryBeginCarryFromPendingPress(
	FVector2D Pointer,
	const FPointerEvent& Event)
{
	FWacomBackpackWorkspaceGestureController& Gesture = GetRuntime().Gesture;
	const FWacomBackpackPendingCardPress CardPress = Gesture.GetCardPress();
	if (!InteractionModel || !CardPress.bActive
		|| !Gesture.HasCardDragThreshold(Event))
	{
		return false;
	}

	if (!InteractionModel->IsSelected(CardPress.InstanceId))
	{
		InteractionModel->ClickCard(CardPress.InstanceId, false);
	}
	const bool bStarted = InteractionModel->BeginCarry(
		CardPress.InstanceId,
		Pointer,
		CurrentStorageRevision);
	Gesture.ClearCardPress();
	InteractionModel->SetCardPressActive(false);
	if (!bStarted)
	{
		return false;
	}

	EndSelectionVisualFreeze(false);
	GetRuntime().Presentation.bCarryCurrentExplicitlySelectedByWheel = false;
	SetExpandedPileLensInputLocked(false, false);
	ClearExpandedPileFocus(true);
	UpdateCarryAnchor(Pointer);
	GetRuntime().Presentation.bCarryStripLayoutDirty = true;
	BeginCarryPickupFeedback();
	WakeFrameScheduler();
	RequestPresentationRefresh(
		EWacomBackpackWorkspacePresentationDirty::NavigationTargets
			| EWacomBackpackWorkspacePresentationDirty::CarryTopology
			| EWacomBackpackWorkspacePresentationDirty::CarryStrip
			| EWacomBackpackWorkspacePresentationDirty::StaticCards
			| EWacomBackpackWorkspacePresentationDirty::CardSemantics
			| EWacomBackpackWorkspacePresentationDirty::MotionTarget
			| EWacomBackpackWorkspacePresentationDirty::NavigationPresentation
			| EWacomBackpackWorkspacePresentationDirty::Accessibility
			| EWacomBackpackWorkspacePresentationDirty::Paint,
		InteractionModel->GetCarry().RemainingInstanceIds);
	OnInteractionChangedNative.Broadcast();
	return true;
}

FReply UWacomBackpackWorkspaceWidget::HandlePilePointerDown(
	UWacomBackpackZonePileWidget* PileWidget,
	const FGeometry& Geometry,
	const FPointerEvent& Event)
{
	RelinquishSemanticNavigationForPointerInput();
	if (!InteractionModel || !PileWidget || GetRuntime().Presentation.IsCarryInputSuspended())
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
	const FVector2D Pointer = ToLocalPointer(Event);
	const FWacomBackpackZonePileView& PileView = PileWidget->GetPileView();
	const bool bMatchesExpandedPile = PileView.bExpanded
		&& PileView.Zone == GetRuntime().Presentation.ExpandedPileFocus.Zone
		&& (PileView.Zone != EZoneKind::SpecialZone
			|| PileView.OwnerInstanceId == GetRuntime().Presentation.ExpandedPileFocus.OwnerInstanceId);
	if (!PileWidget->WasLastPointerDownOnDragHandle() && bMatchesExpandedPile)
	{
		const FReply CardReply = TryHandleExpandedPileVisualPointerDown(Pointer, Event);
		if (CardReply.IsEventHandled())
		{
			return CardReply;
		}
	}
	BeginPendingPilePress(
		*PileWidget,
		Pointer,
		Event,
		Event.IsControlDown(),
		PileWidget->WasLastPointerDownOnDragHandle());
	return FReply::Handled().CaptureMouse(TakeWidget()).SetUserFocus(TakeWidget());
}

void UWacomBackpackWorkspaceWidget::BeginPendingPilePress(
	UWacomBackpackZonePileWidget& PileWidget,
	FVector2D LocalPointer,
	const FPointerEvent& Event,
	bool bControlDown,
	bool bOnDragHandle)
{
	SetExpandedPileLensInputLocked(false, false);
	const FSlateRect HeaderRect = PileWidget.GetResolvedHeaderRect();
	GetRuntime().Gesture.BeginPilePress(
		PileWidget,
		LocalPointer,
		Event.GetScreenSpacePosition(),
		FVector2D(HeaderRect.Left, HeaderRect.Top),
		bControlDown,
		bOnDragHandle);
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
	const FPointerEvent& Event,
	bool bControlDown)
{
	UWacomBackpackZonePileWidget* HeaderPile = FindPileHeaderAt(LocalPointer);
	if (!HeaderPile)
	{
		return false;
	}
	ClearExpandedPileFocus(true);
	BeginPendingPilePress(*HeaderPile, LocalPointer, Event, bControlDown, true);
	return true;
}

bool UWacomBackpackWorkspaceWidget::TryBeginPileMove(
	FVector2D Pointer,
	const FPointerEvent& Event)
{
	FWacomBackpackWorkspaceGestureController& Gesture = GetRuntime().Gesture;
	const FWacomBackpackPendingPilePress PilePress = Gesture.GetPilePress();
	UWacomBackpackZonePileWidget* Pile = PilePress.Pile.Get();
	if (!InteractionModel || !PilePress.bActive || !Pile
		|| !Pile->GetPileView().bMovable
		|| !PilePress.bOnDragHandle
		|| !Gesture.HasPileDragThreshold(Event))
	{
		return false;
	}
	const FWacomBackpackZoneKey Zone = FWacomBackpackZoneKey::Make(
		Pile->GetPileView().Zone, Pile->GetPileView().OwnerInstanceId);
	EndSelectionVisualFreeze(false);
	ClearExpandedPileFocus(true);
	CapturePileMoveVisualSnapshot(*Pile, Zone);
	if (!InteractionModel->BeginPileMove(
		Zone,
		PilePress.LocalPosition,
		PilePress.PileStartPosition))
	{
		Gesture.ClearPileMoveSnapshot();
		return false;
	}
	Gesture.ClearPilePress();
	QueuePilePointer(Pointer);
	FlushQueuedPilePointer();
	StartPilePointerTracking();
	OnInteractionChangedNative.Broadcast();
	return true;
}

bool UWacomBackpackWorkspaceWidget::TryBeginMarqueeFromPendingPilePress(
	FVector2D Pointer,
	const FPointerEvent& Event)
{
	FWacomBackpackWorkspaceGestureController& Gesture = GetRuntime().Gesture;
	const FWacomBackpackPendingPilePress PilePress = Gesture.GetPilePress();
	UWacomBackpackZonePileWidget* Pile = PilePress.Pile.Get();
	if (!InteractionModel || !PilePress.bActive || !Pile
		|| PilePress.bOnDragHandle
		|| !Gesture.HasPileDragThreshold(Event))
	{
		return false;
	}

	const FWacomBackpackZoneKey SourceZone = FWacomBackpackZoneKey::Make(
		Pile->GetPileView().Zone,
		Pile->GetPileView().OwnerInstanceId);
	const TArray<FGuid> PreviousSelection =
		InteractionModel->GetSelection().OrderedSelectedInstanceIds;
	BeginSelectionVisualFreeze(SourceZone);
	InteractionModel->BeginMarquee(
		SourceZone,
		PilePress.LocalPosition,
		PilePress.bControlDown);
	InteractionModel->UpdateMarquee(Pointer);
	Gesture.ClearPilePress();
	const TArray<FGuid> ChangedInstanceIds = BuildChangedInstanceIds(
		PreviousSelection,
		InteractionModel->GetSelection().OrderedSelectedInstanceIds);
	RequestPresentationRefresh(
		EWacomBackpackWorkspacePresentationDirty::StaticCards
			| EWacomBackpackWorkspacePresentationDirty::CardSemantics
			| EWacomBackpackWorkspacePresentationDirty::MotionTarget
			| EWacomBackpackWorkspacePresentationDirty::Accessibility
			| EWacomBackpackWorkspacePresentationDirty::Paint,
		ChangedInstanceIds);
	return InteractionModel->IsMarqueeActive();
}

bool UWacomBackpackWorkspaceWidget::TryBeginMarqueeFromPendingBlankPress(
	FVector2D Pointer,
	const FPointerEvent& Event)
{
	FWacomBackpackWorkspaceGestureController& Gesture = GetRuntime().Gesture;
	const FWacomBackpackPendingMarqueePress MarqueePress =
		Gesture.GetMarqueePress();
	if (!InteractionModel || !MarqueePress.bActive
		|| !Gesture.HasMarqueeDragThreshold(Event))
	{
		return false;
	}

	const TArray<FGuid> PreviousSelection =
		InteractionModel->GetSelection().OrderedSelectedInstanceIds;
	BeginSelectionVisualFreeze(MarqueePress.SourceZone);
	InteractionModel->BeginMarquee(
		MarqueePress.SourceZone,
		MarqueePress.LocalPosition,
		MarqueePress.bControlDown);
	InteractionModel->UpdateMarquee(Pointer);
	const FWacomBackpackZoneKey FocusZone = FWacomBackpackZoneKey::Make(
		GetRuntime().Presentation.ExpandedPileFocus.Zone,
		GetRuntime().Presentation.ExpandedPileFocus.OwnerInstanceId);
	if (!(MarqueePress.SourceZone == FocusZone) || GetRuntime().Presentation.ExpandedPileFocus.FocusIndex == INDEX_NONE)
	{
		ClearExpandedPileFocus(true);
	}
	Gesture.ClearMarqueePress();
	const TArray<FGuid> ChangedInstanceIds = BuildChangedInstanceIds(
		PreviousSelection,
		InteractionModel->GetSelection().OrderedSelectedInstanceIds);
	RequestPresentationRefresh(
		EWacomBackpackWorkspacePresentationDirty::StaticCards
			| EWacomBackpackWorkspacePresentationDirty::CardSemantics
			| EWacomBackpackWorkspacePresentationDirty::MotionTarget
			| EWacomBackpackWorkspacePresentationDirty::Accessibility
			| EWacomBackpackWorkspacePresentationDirty::Paint,
		ChangedInstanceIds);
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
			const FVector2D Delta = ClampedPosition - Move.PileStart;
			for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard : GetBoundCardWidgets())
			{
				UWacomDeckCardWidget* Card = WeakCard.Get();
				const FWacomBackpackWorkspaceCardLayout* Base = Card ? GetVisualState().BaseLayouts().Find(Card) : nullptr;
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
	WakeFrameScheduler();
}

void UWacomBackpackWorkspaceWidget::QueuePilePointer(FVector2D Pointer)
{
	if (!InteractionModel || !InteractionModel->IsPileMoving())
	{
		return;
	}
	GetRuntime().Presentation.QueuedPilePointerLocal = Pointer;
	GetRuntime().Presentation.bHasQueuedPilePointer = true;
}

void UWacomBackpackWorkspaceWidget::FlushQueuedPilePointer()
{
	if (!GetRuntime().Presentation.bHasQueuedPilePointer || !InteractionModel || !InteractionModel->IsPileMoving())
	{
		return;
	}
	GetRuntime().Presentation.bHasQueuedPilePointer = false;
	InteractionModel->UpdatePileMove(GetRuntime().Presentation.QueuedPilePointerLocal);
	ApplyActivePileMove();
}

void UWacomBackpackWorkspaceWidget::CommitPileMoveCardLayouts(
	const FWacomBackpackZoneKey& Zone,
	FVector2D FinalDelta)
{
	for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard : GetBoundCardWidgets())
	{
		UWacomDeckCardWidget* Card = WeakCard.Get();
		FWacomBackpackWorkspaceCardLayout* Base = Card ? GetVisualState().BaseLayouts().Find(Card) : nullptr;
		if (!Card || !Base
			|| FWacomBackpackZoneKey::Make(
				Card->GetWorkspaceDisplayZone(),
				Card->GetWorkspaceDisplayOwnerInstanceId()) != Zone)
		{
			continue;
		}
		Base->Center += FinalDelta;
		GetVisualState().BaseTransitions().Remove(Card);
		ApplyCardLayout(
			*Card,
			Base->Center,
			Base->Size,
			Base->AngleDegrees,
			Base->ZOrder);
	}
}

void UWacomBackpackWorkspaceWidget::CapturePileMoveVisualSnapshot(
	UWacomBackpackZonePileWidget& Pile,
	const FWacomBackpackZoneKey& Zone)
{
	FWacomBackpackPileMoveVisualSnapshot Snapshot;
	if (const UCanvasPanelSlot* PileCanvasSlot = Cast<UCanvasPanelSlot>(Pile.Slot))
	{
		Snapshot.Pile = &Pile;
		Snapshot.Zone = Zone.Zone;
		Snapshot.OwnerInstanceId = Zone.OwnerInstanceId;
		Snapshot.CanvasPosition = PileCanvasSlot->GetPosition();
		Snapshot.ZOrder = PileCanvasSlot->GetZOrder();
		Snapshot.bValid = true;
	}
	GetRuntime().Gesture.SetPileMoveSnapshot(Snapshot);
}

void UWacomBackpackWorkspaceWidget::RestoreAndClearPileMoveVisualSnapshot()
{
	const FWacomBackpackPileMoveVisualSnapshot Snapshot =
		GetRuntime().Gesture.GetPileMoveSnapshot();
	if (Snapshot.bValid)
	{
		if (UWacomBackpackZonePileWidget* Pile = Snapshot.Pile.Get())
		{
			if (UCanvasPanelSlot* PileCanvasSlot = Cast<UCanvasPanelSlot>(Pile->Slot))
			{
				PileCanvasSlot->SetPosition(Snapshot.CanvasPosition);
				PileCanvasSlot->SetZOrder(Snapshot.ZOrder);
			}
			Pile->SetRenderTranslation(FVector2D::ZeroVector);
		}
	}
	GetRuntime().Gesture.ClearPileMoveSnapshot();
}

FWacomBackpackZoneKey UWacomBackpackWorkspaceWidget::ResolveMarqueeSource(FVector2D LocalPointer) const
{
	if (GetRuntime().Presentation.bHasExpandedContentBounds
		&& LocalPointer.X >= GetRuntime().Presentation.ExpandedContentBounds.Left && LocalPointer.X <= GetRuntime().Presentation.ExpandedContentBounds.Right
		&& LocalPointer.Y >= GetRuntime().Presentation.ExpandedContentBounds.Top && LocalPointer.Y <= GetRuntime().Presentation.ExpandedContentBounds.Bottom)
	{
		return FWacomBackpackZoneKey::Make(GetRuntime().Presentation.ExpandedContentZone, GetRuntime().Presentation.ExpandedContentOwnerInstanceId);
	}
	return FWacomBackpackZoneKey::Make(EZoneKind::Backpack);
}

FReply UWacomBackpackWorkspaceWidget::BuildHandledPointerReply()
{
	FReply Reply = FReply::Handled();
	if (GetRuntime().Presentation.IsCarryInputSuspended())
	{
		return Reply.ReleaseMouseCapture();
	}
	if ((InteractionModel && (InteractionModel->IsCarrying()
			|| InteractionModel->IsMarqueeActive()
			|| InteractionModel->IsPileMoving()))
		|| GetRuntime().Gesture.HasAnyPendingPress())
	{
		return Reply.CaptureMouse(TakeWidget()).SetUserFocus(TakeWidget());
	}
	return Reply.ReleaseMouseCapture();
}

FReply UWacomBackpackWorkspaceWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	RelinquishSemanticNavigationForPointerInput();
	SyncExpandedPileLensInputLockFromPointerEvent(InMouseEvent);
	if (GetRuntime().Presentation.IsCarryInputSuspended())
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}
	if (InteractionModel && !InteractionModel->IsCarrying()
		&& (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
			|| InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton))
	{
		const FReply CardReply = TryHandleExpandedPileVisualPointerDown(
			ToLocalPointer(InMouseEvent), InMouseEvent);
		if (CardReply.IsEventHandled())
		{
			return CardReply;
		}
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
		GetRuntime().Gesture.BeginMarqueePress(
			SourceZone,
			Pointer,
			InMouseEvent.GetScreenSpacePosition(),
			InMouseEvent.IsControlDown());
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
	RelinquishSemanticNavigationForPointerInput();
	SyncExpandedPileLensInputLockFromPointerEvent(InMouseEvent);
	if (!InteractionModel || GetRuntime().Presentation.IsCarryInputSuspended())
	{
		return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
	}
	if (InteractionModel->IsCarrying())
	{
		QueueCarryPointer(ToLocalPointer(InMouseEvent));
		OnInteractionChangedNative.Broadcast();
		return BuildHandledPointerReply();
	}
	if (InteractionModel->IsPileMoving())
	{
		QueuePilePointer(ToLocalPointer(InMouseEvent));
		return BuildHandledPointerReply();
	}
	if (TryBeginPileMove(ToLocalPointer(InMouseEvent), InMouseEvent))
	{
		return BuildHandledPointerReply();
	}
	if (TryBeginMarqueeFromPendingPilePress(ToLocalPointer(InMouseEvent), InMouseEvent))
	{
		return BuildHandledPointerReply();
	}
	if (TryBeginMarqueeFromPendingBlankPress(ToLocalPointer(InMouseEvent), InMouseEvent))
	{
		return BuildHandledPointerReply();
	}
	if (TryBeginCarryFromPendingPress(ToLocalPointer(InMouseEvent), InMouseEvent))
	{
		return BuildHandledPointerReply();
	}
	if (InteractionModel->IsMarqueeActive())
	{
		InteractionModel->UpdateMarquee(ToLocalPointer(InMouseEvent));
		Invalidate(EInvalidateWidgetReason::Paint);
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
	RelinquishSemanticNavigationForPointerInput();
	if (!InteractionModel || GetRuntime().Presentation.IsCarryInputSuspended())
	{
		return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
	}
	if (InteractionModel->IsCarrying())
	{
		if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
			|| InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			SyncCarryPointerForRelease(ToLocalPointer(InMouseEvent));
			BroadcastPointerRelease(InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton);
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
		const FVector2D FinalDelta = Snapped - Completed.PileStart;
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
		GetRuntime().Gesture.ClearPileMoveSnapshot();
		OnPileMoveCommittedNative.Broadcast(
			Completed.Zone.Zone,
			Completed.Zone.OwnerInstanceId,
			FVector2D(
				WorkspaceSize.X > 1.0f ? Snapped.X / WorkspaceSize.X : 0.0f,
				WorkspaceSize.Y > 1.0f ? Snapped.Y / WorkspaceSize.Y : 0.0f));
		GetRuntime().Gesture.ClearPilePress();
		GetRuntime().Presentation.bHasQueuedPilePointer = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	const FWacomBackpackPendingPilePress PilePress =
		GetRuntime().Gesture.GetPilePress();
	if (PilePress.bActive && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		ClearExpandedPileFocus(true);
		InteractionModel->ClickBlank();
		UpdateSelectionVisualFreezeLifetime();
		if (UWacomBackpackZonePileWidget* Pile = PilePress.Pile.Get())
		{
			OnPileExpansionRequestedNative.Broadcast(
				Pile->GetPileView().Zone, Pile->GetPileView().OwnerInstanceId, false);
		}
		GetRuntime().Gesture.ClearPilePress();
		return FReply::Handled().ReleaseMouseCapture();
	}
	if (InteractionModel->IsMarqueeActive() && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const TArray<FGuid> PreviousSelection =
			InteractionModel->GetSelection().OrderedSelectedInstanceIds;
		InteractionModel->UpdateMarquee(ToLocalPointer(InMouseEvent));
		InteractionModel->CompleteMarquee();
		UpdateSelectionVisualFreezeLifetime();
		ClearExpandedPileFocus(true);
		const TArray<FGuid> ChangedInstanceIds = BuildChangedInstanceIds(
			PreviousSelection,
			InteractionModel->GetSelection().OrderedSelectedInstanceIds);
		RequestPresentationRefresh(
			EWacomBackpackWorkspacePresentationDirty::StaticCards
				| EWacomBackpackWorkspacePresentationDirty::CardSemantics
				| EWacomBackpackWorkspacePresentationDirty::MotionTarget
				| EWacomBackpackWorkspacePresentationDirty::Accessibility
				| EWacomBackpackWorkspacePresentationDirty::Paint,
			ChangedInstanceIds);
		OnInteractionChangedNative.Broadcast();
		return FReply::Handled().ReleaseMouseCapture();
	}
	const FWacomBackpackPendingMarqueePress MarqueePress =
		GetRuntime().Gesture.GetMarqueePress();
	if (MarqueePress.bActive && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const TArray<FGuid> PreviousSelection =
			InteractionModel->GetSelection().OrderedSelectedInstanceIds;
		InteractionModel->ClickBlank();
		GetRuntime().Gesture.ClearMarqueePress();
		UpdateSelectionVisualFreezeLifetime();
		ClearExpandedPileFocus(true);
		const TArray<FGuid> ChangedInstanceIds = BuildChangedInstanceIds(
			PreviousSelection,
			InteractionModel->GetSelection().OrderedSelectedInstanceIds);
		RequestPresentationRefresh(
			EWacomBackpackWorkspacePresentationDirty::StaticCards
				| EWacomBackpackWorkspacePresentationDirty::CardSemantics
				| EWacomBackpackWorkspacePresentationDirty::MotionTarget
				| EWacomBackpackWorkspacePresentationDirty::Accessibility
				| EWacomBackpackWorkspacePresentationDirty::Paint,
			ChangedInstanceIds);
		OnInteractionChangedNative.Broadcast();
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UWacomBackpackWorkspaceWidget::NativeOnMouseWheel(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (!GetRuntime().Presentation.IsCarryInputSuspended()
		&& InteractionModel && InteractionModel->IsCarrying())
	{
		const FWacomBackpackWorkspaceCarryState& PreviousCarry = InteractionModel->GetCarry();
		TArray<FGuid> ChangedInstanceIds;
		if (PreviousCarry.RemainingInstanceIds.IsValidIndex(PreviousCarry.CurrentIndex))
		{
			const FGuid PreviousId = PreviousCarry.RemainingInstanceIds[PreviousCarry.CurrentIndex];
			ChangedInstanceIds.Add(PreviousId);
			for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard : GetBoundCardWidgets())
			{
				UWacomDeckCardWidget* Card = WeakCard.Get();
				if (Card && IsInCarryVisualLayer(Card)
					&& Card->GetCardInstanceId() == PreviousId)
				{
					GetRuntime().Presentation.PreviousCarryCurrentCard = Card;
					break;
				}
			}
		}
		const int32 PreviousIndex = PreviousCarry.CurrentIndex;
		InteractionModel->StepCurrentByWheel(InMouseEvent.GetWheelDelta());
		if (InteractionModel->GetCarry().CurrentIndex != PreviousIndex)
		{
			GetRuntime().Presentation.bCarryCurrentExplicitlySelectedByWheel = true;
		}
		GetRuntime().Presentation.bCarryStripLayoutDirty = true;
		const FWacomBackpackWorkspaceCarryState& CurrentCarry =
			InteractionModel->GetCarry();
		if (CurrentCarry.RemainingInstanceIds.IsValidIndex(
			CurrentCarry.CurrentIndex))
		{
			ChangedInstanceIds.AddUnique(
				CurrentCarry.RemainingInstanceIds[CurrentCarry.CurrentIndex]);
		}
		RequestPresentationRefresh(
			EWacomBackpackWorkspacePresentationDirty::CarryTopology
				| EWacomBackpackWorkspacePresentationDirty::CarryStrip
				| EWacomBackpackWorkspacePresentationDirty::StaticCards
				| EWacomBackpackWorkspacePresentationDirty::CardSemantics
				| EWacomBackpackWorkspacePresentationDirty::MotionTarget
				| EWacomBackpackWorkspacePresentationDirty::Accessibility
				| EWacomBackpackWorkspacePresentationDirty::Paint,
			ChangedInstanceIds);
		WakeFrameScheduler();
		OnInteractionChangedNative.Broadcast();
		return FReply::Handled();
	}
	return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}

int32 UWacomBackpackWorkspaceWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const int32 ChildMaxLayerId = Super::NativePaint(
		Args,
		AllottedGeometry,
		MyCullingRect,
		OutDrawElements,
		LayerId,
		InWidgetStyle,
		bParentEnabled);
	const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
		? InteractionStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();

	// Card-local layer ids are only comparable inside one SObjectWidget subtree.
	// Draw every marker from the complete Workspace child maximum so no later
	// sibling Retainer can randomly cover it. Marker/card occlusion is resolved
	// explicitly below so the centralized pass still obeys visual card stacking.
	struct FOverlayCardEntry
	{
		const UWacomDeckCardWidget* Card = nullptr;
		FWacomBackpackCardMarkerOccluder Body;
	};
	TArray<FOverlayCardEntry> OverlayCards;
	OverlayCards.Reserve(GetBoundCardWidgets().Num());
	int32 StableIndex = 0;
	for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard : GetBoundCardWidgets())
	{
		const UWacomDeckCardWidget* Card = WeakCard.Get();
		const UCanvasPanelSlot* CardSlot = Card ? Cast<UCanvasPanelSlot>(Card->Slot) : nullptr;
		if (!Card || !CardSlot
			|| Card->GetVisibility() == ESlateVisibility::Collapsed
			|| Card->GetVisibility() == ESlateVisibility::Hidden)
		{
			++StableIndex;
			continue;
		}

		FOverlayCardEntry& Entry = OverlayCards.AddDefaulted_GetRef();
		Entry.Card = Card;
		const UPanelWidget* Parent = Card->GetParent();
		Entry.Body.Order.LayerPriority = Parent == CarryActiveLayer
			? 4
			: Parent == CarryLayer
				? 3
				: Parent == SettlementLayer
					? 2
					: Parent == StaticCardLayer
						? 1
						: 0;
		Entry.Body.Order.ZOrder = CardSlot->GetZOrder();
		Entry.Body.Order.ChildIndex = Parent ? Parent->GetChildIndex(Card) : INDEX_NONE;
		Entry.Body.Order.StableIndex = StableIndex++;
		const FWacomBackpackWorkspaceCardVisualPose Pose = CaptureCardVisualPose(*Card);
		Entry.Body.Center = Pose.Center;
		Entry.Body.Size = CardSlot->GetSize();
		Entry.Body.AngleDegrees = Pose.AngleDegrees;
	}

	TArray<FWacomBackpackCardMarkerOccluder> CardBodies;
	CardBodies.Reserve(OverlayCards.Num());
	for (const FOverlayCardEntry& Entry : OverlayCards)
	{
		CardBodies.Add(Entry.Body);
	}

	int32 OverlayMaxLayerId = ChildMaxLayerId;
	for (const FOverlayCardEntry& Entry : OverlayCards)
	{
		const UWacomDeckCardWidget* Card = Entry.Card;
		FWacomBackpackCardOverlayPaintView CardView;
		CardView.FocusBrush = Card->bWorkspaceNavigationFocused
			? &Card->WorkspaceFocusPaintBrush
			: nullptr;
		CardView.SemanticBrush = Card->WorkspaceSemanticIcon
			!= EWacomBackpackWorkspaceCardSemanticIcon::None
			? &Card->WorkspaceSemanticPaintBrush
			: nullptr;
		CardView.LocalMotionTranslation = Card->GetBackpackLocalMotionTranslation();
		CardView.LocalMotionAngleDegrees = Card->GetBackpackLocalMotionAngle();
		if (CardView.FocusBrush)
		{
			const FVector2D FocusCenter =
				FWacomBackpackWorkspaceOverlayPainter::ResolveCardMarkerCenter(
					Entry.Body.Center,
					Entry.Body.Size,
					Entry.Body.AngleDegrees,
					false,
					CardView.IconSize,
					CardView.Padding);
			if (FWacomBackpackWorkspaceOverlayPainter::IsMarkerOccludedByHigherCard(
					FocusCenter,
					CardView.IconSize,
					Entry.Body.AngleDegrees,
					Entry.Body.Order,
					CardBodies))
			{
				CardView.FocusBrush = nullptr;
			}
		}
		if (CardView.SemanticBrush)
		{
			const FVector2D SemanticCenter =
				FWacomBackpackWorkspaceOverlayPainter::ResolveCardMarkerCenter(
					Entry.Body.Center,
					Entry.Body.Size,
					Entry.Body.AngleDegrees,
					true,
					CardView.IconSize,
					CardView.Padding);
			if (FWacomBackpackWorkspaceOverlayPainter::IsMarkerOccludedByHigherCard(
					SemanticCenter,
					CardView.IconSize,
					Entry.Body.AngleDegrees,
					Entry.Body.Order,
					CardBodies))
			{
				CardView.SemanticBrush = nullptr;
			}
		}
		FLinearColor CardTint = InWidgetStyle.GetColorAndOpacityTint();
		CardTint.A *= Card->GetRenderOpacity();
		OverlayMaxLayerId = FMath::Max(
			OverlayMaxLayerId,
			FWacomBackpackWorkspaceOverlayPainter::PaintCardMarkers(
				Card->GetCachedGeometry(),
				OutDrawElements,
				ChildMaxLayerId,
				CardView,
				CardTint,
				bParentEnabled));
	}

	if (!InteractionModel || !InteractionModel->IsMarqueeActive())
	{
		return OverlayMaxLayerId;
	}
	const FWacomBackpackWorkspaceSelectionState& Selection =
		InteractionModel->GetSelection();
	FWacomBackpackWorkspaceMarqueePaintView View;
	View.bVisible = true;
	View.Start = Selection.MarqueeStart;
	View.End = Selection.MarqueeCurrent;
	View.Color = Style->SelectionColor;
	View.FillOpacity = Style->CardStateOverlayOpacity;
	View.BorderThickness = 2.0f;
	return FWacomBackpackWorkspaceOverlayPainter::PaintMarquee(
		AllottedGeometry,
		OutDrawElements,
		OverlayMaxLayerId,
		View,
		InWidgetStyle.GetColorAndOpacityTint().A,
		bParentEnabled);
}

FReply UWacomBackpackWorkspaceWidget::NativeOnKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::F1)
	{
		OnControlsHelpRequestedNative.Broadcast();
		return FReply::Handled();
	}
	if (Key == EKeys::Enter || Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		return HandleNavigationPrimary(false)
			? FReply::Handled()
			: Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}
	if (Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Left)
	{
		return HandleNavigationSelection()
			? FReply::Handled()
			: Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}
	if (Key == EKeys::T || Key == EKeys::Gamepad_FaceButton_Top)
	{
		return HandleNavigationContextAction()
			? FReply::Handled()
			: Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}
	if (Key == EKeys::Q || Key == EKeys::Gamepad_LeftShoulder)
	{
		return StepCarriedCard(-1)
			? FReply::Handled()
			: Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}
	if (Key == EKeys::E || Key == EKeys::Gamepad_RightShoulder)
	{
		return StepCarriedCard(1)
			? FReply::Handled()
			: Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}
	if (InKeyEvent.GetKey() == EKeys::LeftShift)
	{
		SetExpandedPileLensInputLocked(true, false);
		return GetRuntime().Presentation.bExpandedPileLensInputLocked
			? FReply::Handled()
			: Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}
	if (InteractionModel && InKeyEvent.IsControlDown() && InKeyEvent.GetKey() == EKeys::A)
	{
		const TArray<FGuid> PreviousSelection =
			InteractionModel->GetSelection().OrderedSelectedInstanceIds;
		if (InteractionModel->GetSelection().bHasSourceZone)
		{
			BeginSelectionVisualFreeze(InteractionModel->GetSelection().SourceZone);
		}
		InteractionModel->SelectAllMovable();
		UpdateSelectionVisualFreezeLifetime();
		const TArray<FGuid> ChangedInstanceIds = BuildChangedInstanceIds(
			PreviousSelection,
			InteractionModel->GetSelection().OrderedSelectedInstanceIds);
		RequestPresentationRefresh(
			EWacomBackpackWorkspacePresentationDirty::StaticCards
				| EWacomBackpackWorkspacePresentationDirty::CardSemantics
				| EWacomBackpackWorkspacePresentationDirty::MotionTarget
				| EWacomBackpackWorkspacePresentationDirty::Accessibility
				| EWacomBackpackWorkspacePresentationDirty::Paint,
			ChangedInstanceIds);
		OnInteractionChangedNative.Broadcast();
		return FReply::Handled();
	}
	const bool bHasCancelablePointerInteraction =
		(InteractionModel
			&& (InteractionModel->IsCarrying() || InteractionModel->IsMarqueeActive() || InteractionModel->IsPileMoving()))
		|| GetRuntime().Gesture.HasPendingCardPress()
		|| GetRuntime().Gesture.HasPendingPilePress();
	if ((Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
		&& bHasCancelablePointerInteraction)
	{
		CancelInteractionWithReturn();
		OnInteractionChangedNative.Broadcast();
		return FReply::Handled();
	}
	if ((Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
		&& GetRuntime().Presentation.bHasExpandedContentBounds)
	{
		OnCollapseExpandedPileRequestedNative.Broadcast();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FNavigationReply UWacomBackpackWorkspaceWidget::NativeOnNavigation(
	const FGeometry& InGeometry,
	const FNavigationEvent& InNavigationEvent)
{
	ReconcileNavigationTargets();
	TArray<FGuid> ChangedInstanceIds;
	if (const FWacomBackpackWorkspaceNavigationTarget* Previous =
		GetRuntime().Navigation.GetFocusedTarget())
	{
		if (Previous->Kind == EWacomBackpackWorkspaceNavigationTargetKind::Card)
		{
			ChangedInstanceIds.Add(Previous->InstanceId);
		}
	}
	if (GetRuntime().Navigation.Move(InNavigationEvent.GetNavigationType()))
	{
		if (const FWacomBackpackWorkspaceNavigationTarget* Current =
			GetRuntime().Navigation.GetFocusedTarget())
		{
			if (Current->Kind
				== EWacomBackpackWorkspaceNavigationTargetKind::Card)
			{
				ChangedInstanceIds.AddUnique(Current->InstanceId);
			}
		}
		RequestPresentationRefresh(
			EWacomBackpackWorkspacePresentationDirty::StaticCards
				| EWacomBackpackWorkspacePresentationDirty::MotionTarget
				| EWacomBackpackWorkspacePresentationDirty::NavigationPresentation
				| EWacomBackpackWorkspacePresentationDirty::Accessibility
				| EWacomBackpackWorkspacePresentationDirty::Paint,
			ChangedInstanceIds);
		OnInteractionChangedNative.Broadcast();
		return FNavigationReply::Stop();
	}
	return Super::NativeOnNavigation(InGeometry, InNavigationEvent);
}

void UWacomBackpackWorkspaceWidget::ReconcileNavigationTargets()
{
	TArray<FWacomBackpackWorkspaceNavigationTarget> Targets;
	const bool bCarrying = InteractionModel && InteractionModel->IsCarrying();
	const FVector2D WorkspaceSize = GetLayoutSpaceSize();
	if (!bCarrying)
	{
		for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard : GetBoundCardWidgets())
		{
			UWacomDeckCardWidget* Card = WeakCard.Get();
			if (!Card || !Card->GetCardInstanceId().IsValid()
				|| Card->GetVisibility() == ESlateVisibility::Collapsed)
			{
				continue;
			}
			const FWacomBackpackWorkspaceCardVisualPose Pose = CaptureCardVisualPose(*Card);
			FWacomBackpackWorkspaceNavigationTarget Target;
			Target.Kind = EWacomBackpackWorkspaceNavigationTargetKind::Card;
			Target.InstanceId = Card->GetCardInstanceId();
			Target.Zone = FWacomBackpackZoneKey::Make(
				Card->GetWorkspaceDisplayZone(),
				Card->GetWorkspaceDisplayOwnerInstanceId());
			Target.Center = Pose.Center;
			Target.LayerRank = Cast<UCanvasPanelSlot>(Card->Slot)
				? CastChecked<UCanvasPanelSlot>(Card->Slot)->GetZOrder()
				: 0;
			Target.bActionable = Card->IsWorkspaceSelectionEnabled();
			Targets.Add(Target);
		}
	}
	else
	{
		FWacomBackpackWorkspaceNavigationTarget Flux;
		Flux.Kind = EWacomBackpackWorkspaceNavigationTargetKind::Flux;
		Flux.Zone = FWacomBackpackZoneKey::Make(EZoneKind::Backpack);
		Flux.Center = WorkspaceSize * FVector2D(0.45f, 0.55f);
		Targets.Add(Flux);
	}

	for (const TWeakObjectPtr<UWacomBackpackZonePileWidget>& WeakPile : GetRegisteredPileWidgets())
	{
		const UWacomBackpackZonePileWidget* Pile = WeakPile.Get();
		if (!Pile || Pile->GetVisibility() == ESlateVisibility::Collapsed)
		{
			continue;
		}
		const FSlateRect Header = Pile->GetResolvedHeaderRect();
		FWacomBackpackWorkspaceNavigationTarget Target;
		Target.Kind = EWacomBackpackWorkspaceNavigationTargetKind::Pile;
		Target.Zone = FWacomBackpackZoneKey::Make(
			Pile->GetPileView().Zone,
			Pile->GetPileView().OwnerInstanceId);
		Target.Center = FVector2D(
			(Header.Left + Header.Right) * 0.5f,
			(Header.Top + Header.Bottom) * 0.5f);
		Target.LayerRank = Cast<UCanvasPanelSlot>(Pile->Slot)
			? CastChecked<UCanvasPanelSlot>(Pile->Slot)->GetZOrder()
			: 0;
		Targets.Add(Target);
	}
	if (bCarrying)
	{
		FWacomBackpackWorkspaceNavigationTarget Delete;
		Delete.Kind = EWacomBackpackWorkspaceNavigationTargetKind::Delete;
		Delete.Center = FVector2D(
			FMath::Max(0.0f, WorkspaceSize.X - 110.0f),
			FMath::Max(0.0f, WorkspaceSize.Y - 60.0f));
		Delete.LayerRank = MAX_int32;
		Targets.Add(Delete);
	}
	Targets.Sort([](
		const FWacomBackpackWorkspaceNavigationTarget& Left,
		const FWacomBackpackWorkspaceNavigationTarget& Right)
	{
		if (!FMath::IsNearlyEqual(Left.Center.Y, Right.Center.Y))
		{
			return Left.Center.Y < Right.Center.Y;
		}
		if (!FMath::IsNearlyEqual(Left.Center.X, Right.Center.X))
		{
			return Left.Center.X < Right.Center.X;
		}
		if (Left.LayerRank != Right.LayerRank)
		{
			return Left.LayerRank > Right.LayerRank;
		}
		return Left.InstanceId.ToString(EGuidFormats::Digits)
			< Right.InstanceId.ToString(EGuidFormats::Digits);
	});
	GetRuntime().Navigation.ReconcileTargets(Targets);
}

void UWacomBackpackWorkspaceWidget::RefreshNavigationPresentation(
	const FWacomBackpackWorkspacePresentationRequest& Request)
{
	(void)Request;
	const bool bShowNavigationFocus =
		GetRuntime().Navigation.IsSemanticFocusActive();
	UWacomDeckCardWidget* FocusedCard = nullptr;
	if (bShowNavigationFocus)
	{
		if (const FWacomBackpackWorkspaceNavigationTarget* Focused =
			GetRuntime().Navigation.GetFocusedTarget())
		{
			if (Focused->Kind
				== EWacomBackpackWorkspaceNavigationTargetKind::Card)
			{
				FocusedCard =
					GetRuntime().Visuals.FindPhysicalCard(Focused->InstanceId);
			}
		}
	}
	const FWacomBackpackWorkspaceNavigationTarget* Focused =
		GetRuntime().Navigation.GetFocusedTarget();
	for (const TWeakObjectPtr<UWacomBackpackZonePileWidget>& WeakPile : GetRegisteredPileWidgets())
	{
		if (UWacomBackpackZonePileWidget* Pile = WeakPile.Get())
		{
			const FWacomBackpackZoneKey PileZone = FWacomBackpackZoneKey::Make(
				Pile->GetPileView().Zone,
				Pile->GetPileView().OwnerInstanceId);
			Pile->SetNavigationFocused(bShowNavigationFocus && Focused
				&& Focused->Kind == EWacomBackpackWorkspaceNavigationTargetKind::Pile
				&& Focused->Zone == PileZone);
		}
	}
	if (bShowNavigationFocus)
	{
		OnBrowseFocusChangedNative.Broadcast(FocusedCard);
	}
}

void UWacomBackpackWorkspaceWidget::RefreshCardAccessibilityPresentation(
	const FWacomBackpackWorkspacePresentationRequest& Request)
{
	const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
		? InteractionStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	ForEachPresentationCard(
		Request,
		[this, Style](UWacomDeckCardWidget& Card)
	{
		Card.SetWorkspaceAccessibilityState(
			IsCardAccessibilityFocused(Card),
			ResolveCardAccessibilitySemanticIcon(Card),
			*Style);
	});
	Invalidate(EInvalidateWidgetReason::Paint);
}

bool UWacomBackpackWorkspaceWidget::IsCardAccessibilityFocused(
	const UWacomDeckCardWidget& Card) const
{
	return GetRuntime().Navigation.IsSemanticFocusActive()
		&& GetRuntime().Navigation.IsCardFocused(Card.GetCardInstanceId());
}

EWacomBackpackWorkspaceCardSemanticIcon
UWacomBackpackWorkspaceWidget::ResolveCardAccessibilitySemanticIcon(
	const UWacomDeckCardWidget& Card) const
{
	const FGuid InstanceId = Card.GetCardInstanceId();
	const bool bSelected = InteractionModel
		&& Card.GetWorkspaceReadOnlyKind()
			== EWacomBackpackWorkspaceCardReadOnlyKind::None
		&& InteractionModel->IsSelected(InstanceId);
	const bool bCarried = InteractionModel
		&& InteractionModel->IsCarrying()
		&& IsInCarryVisualLayer(&Card);
	return FWacomBackpackWorkspaceAccessibility::ResolveCardSemanticIcon(
		bSelected,
		bCarried && GetRuntime().Presentation.bCarryDropValid,
		bCarried && GetRuntime().Presentation.bCarryDropRejected);
}

void UWacomBackpackWorkspaceWidget::RelinquishSemanticNavigationForPointerInput()
{
	FWacomBackpackWorkspaceNavigationController& NavigationController =
		GetRuntime().Navigation;
	const bool bHadSemanticFocus = NavigationController.IsSemanticFocusActive();
	NavigationController.NotifyPointerInput();
	if (bHadSemanticFocus)
	{
		OnBrowseFocusChangedNative.Broadcast(nullptr);
	}
}

bool UWacomBackpackWorkspaceWidget::GetFocusedReleaseTarget(
	EWacomBackpackWorkspaceReleaseTargetKind& OutKind,
	FWacomBackpackZoneKey& OutZone) const
{
	const FWacomBackpackWorkspaceNavigationTarget* Target =
		GetRuntime().Navigation.GetFocusedTarget();
	if (!Target || !GetRuntime().Navigation.IsSemanticFocusActive()
		|| !InteractionModel || !InteractionModel->IsCarrying())
	{
		return false;
	}
	OutZone = Target->Zone;
	switch (Target->Kind)
	{
	case EWacomBackpackWorkspaceNavigationTargetKind::Flux:
		OutKind = EWacomBackpackWorkspaceReleaseTargetKind::Flux;
		return true;
	case EWacomBackpackWorkspaceNavigationTargetKind::Pile:
		OutKind = EWacomBackpackWorkspaceReleaseTargetKind::Pile;
		return OutZone.IsValid();
	case EWacomBackpackWorkspaceNavigationTargetKind::Delete:
		OutKind = EWacomBackpackWorkspaceReleaseTargetKind::Delete;
		return true;
	default:
		return false;
	}
}

bool UWacomBackpackWorkspaceWidget::HandleNavigationPrimary(bool bReleaseAll)
{
	if (!InteractionModel || GetRuntime().Presentation.IsCarryInputSuspended())
	{
		return false;
	}
	ReconcileNavigationTargets();
	GetRuntime().Navigation.ActivateSemanticFocus();
	const FWacomBackpackWorkspaceNavigationTarget* Target =
		GetRuntime().Navigation.GetFocusedTarget();
	if (!Target)
	{
		return false;
	}
	if (InteractionModel->IsCarrying())
	{
		EWacomBackpackWorkspaceReleaseTargetKind TargetKind;
		FWacomBackpackZoneKey TargetZone;
		if (!GetFocusedReleaseTarget(TargetKind, TargetZone))
		{
			return false;
		}
		BroadcastRelease(bReleaseAll, TargetKind, TargetZone);
		return true;
	}
	if (Target->Kind == EWacomBackpackWorkspaceNavigationTargetKind::Pile)
	{
		OnPileExpansionRequestedNative.Broadcast(
			Target->Zone.Zone, Target->Zone.OwnerInstanceId, false);
		return true;
	}
	if (Target->Kind != EWacomBackpackWorkspaceNavigationTargetKind::Card
		|| !Target->bActionable)
	{
		return false;
	}
	if (!InteractionModel->BeginCarry(Target->InstanceId, Target->Center, CurrentStorageRevision))
	{
		return false;
	}
	// 键盘/手柄拾取没有对应的起手 PointerUp，下一次主操作必须直接释放。
	InteractionModel->NotifyReleaseGestureStarted();
	EndSelectionVisualFreeze(false);
	GetRuntime().Presentation.bCarryCurrentExplicitlySelectedByWheel = false;
	ClearExpandedPileFocus(true);
	UpdateCarryAnchor(Target->Center);
	GetRuntime().Presentation.bCarryStripLayoutDirty = true;
	BeginCarryPickupFeedback();
	RequestPresentationRefresh(
		EWacomBackpackWorkspacePresentationDirty::NavigationTargets
			| EWacomBackpackWorkspacePresentationDirty::CarryTopology
			| EWacomBackpackWorkspacePresentationDirty::CarryStrip
			| EWacomBackpackWorkspacePresentationDirty::StaticCards
			| EWacomBackpackWorkspacePresentationDirty::CardSemantics
			| EWacomBackpackWorkspacePresentationDirty::MotionTarget
			| EWacomBackpackWorkspacePresentationDirty::NavigationPresentation
			| EWacomBackpackWorkspacePresentationDirty::Accessibility
			| EWacomBackpackWorkspacePresentationDirty::Paint,
		InteractionModel->GetCarry().RemainingInstanceIds);
	WakeFrameScheduler();
	OnInteractionChangedNative.Broadcast();
	return true;
}

bool UWacomBackpackWorkspaceWidget::HandleNavigationSelection()
{
	if (!InteractionModel || InteractionModel->IsCarrying())
	{
		return false;
	}
	ReconcileNavigationTargets();
	const FWacomBackpackWorkspaceNavigationTarget* Target =
		GetRuntime().Navigation.GetFocusedTarget();
	if (!Target || Target->Kind != EWacomBackpackWorkspaceNavigationTargetKind::Card
		|| !Target->bActionable)
	{
		return false;
	}
	InteractionModel->ClickCard(Target->InstanceId, true);
	UpdateSelectionVisualFreezeLifetime();
	RequestPresentationRefresh(
		EWacomBackpackWorkspacePresentationDirty::StaticCards
			| EWacomBackpackWorkspacePresentationDirty::CardSemantics
			| EWacomBackpackWorkspacePresentationDirty::MotionTarget
			| EWacomBackpackWorkspacePresentationDirty::Accessibility
			| EWacomBackpackWorkspacePresentationDirty::Paint,
		MakeArrayView(&Target->InstanceId, 1));
	OnInteractionChangedNative.Broadcast();
	return true;
}

bool UWacomBackpackWorkspaceWidget::HandleNavigationContextAction()
{
	if (!InteractionModel)
	{
		return false;
	}
	if (InteractionModel->IsCarrying())
	{
		return HandleNavigationPrimary(true);
	}
	ReconcileNavigationTargets();
	const FWacomBackpackWorkspaceNavigationTarget* Target =
		GetRuntime().Navigation.GetFocusedTarget();
	if (!Target || Target->Kind != EWacomBackpackWorkspaceNavigationTargetKind::Card)
	{
		return false;
	}
	for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard : GetBoundCardWidgets())
	{
		UWacomDeckCardWidget* Card = WeakCard.Get();
		if (Card && Card->GetCardInstanceId() == Target->InstanceId)
		{
			return Card->RequestBattleEnabledToggle();
		}
	}
	return false;
}

bool UWacomBackpackWorkspaceWidget::StepCarriedCard(int32 Direction)
{
	if (!InteractionModel || !InteractionModel->IsCarrying()
		|| GetRuntime().Presentation.IsCarryInputSuspended() || Direction == 0)
	{
		return false;
	}
	TArray<FGuid> ChangedInstanceIds;
	const int32 PreviousIndex = InteractionModel->GetCarry().CurrentIndex;
	if (InteractionModel->GetCarry().RemainingInstanceIds.IsValidIndex(
		PreviousIndex))
	{
		ChangedInstanceIds.Add(
			InteractionModel->GetCarry().RemainingInstanceIds[PreviousIndex]);
	}
	InteractionModel->StepCurrentByWheel(Direction < 0 ? 1.0f : -1.0f);
	if (InteractionModel->GetCarry().CurrentIndex == PreviousIndex)
	{
		return true;
	}
	if (InteractionModel->GetCarry().RemainingInstanceIds.IsValidIndex(
		InteractionModel->GetCarry().CurrentIndex))
	{
		ChangedInstanceIds.AddUnique(
			InteractionModel->GetCarry().RemainingInstanceIds[
				InteractionModel->GetCarry().CurrentIndex]);
	}
	GetRuntime().Presentation.bCarryCurrentExplicitlySelectedByWheel = true;
	GetRuntime().Presentation.bCarryStripLayoutDirty = true;
	RequestPresentationRefresh(
		EWacomBackpackWorkspacePresentationDirty::CarryStrip
			| EWacomBackpackWorkspacePresentationDirty::StaticCards
			| EWacomBackpackWorkspacePresentationDirty::CardSemantics
			| EWacomBackpackWorkspacePresentationDirty::MotionTarget
			| EWacomBackpackWorkspacePresentationDirty::Accessibility
			| EWacomBackpackWorkspacePresentationDirty::Paint,
		ChangedInstanceIds);
	WakeFrameScheduler();
	OnInteractionChangedNative.Broadcast();
	return true;
}

FReply UWacomBackpackWorkspaceWidget::NativeOnKeyUp(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::LeftShift && GetRuntime().Presentation.bExpandedPileLensInputLocked)
	{
		SetExpandedPileLensInputLocked(false, true);
		return FReply::Handled();
	}
	return Super::NativeOnKeyUp(InGeometry, InKeyEvent);
}

void UWacomBackpackWorkspaceWidget::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	SetExpandedPileLensInputLocked(false, false);
	Super::NativeOnFocusLost(InFocusEvent);
}

void UWacomBackpackWorkspaceWidget::ApplyStaticCardPresentation(
	UWacomDeckCardWidget& CardWidget,
	const UWacomBackpackWorkspaceStyle& Style)
{
	const bool bInCarryLayer = IsInCarryVisualLayer(&CardWidget);
	const FWacomBackpackWorkspaceCardLayoutTransition* Transition = GetVisualState().BaseTransitions().Find(&CardWidget);
	const FWacomBackpackWorkspaceCardLayout* Base = Transition
		? &Transition->Current
		: GetVisualState().BaseLayouts().Find(&CardWidget);
	if (!Base || bInCarryLayer || IsInSettlementVisualLayer(&CardWidget))
	{
		return;
	}
	if (const FWacomBackpackWorkspaceCardLayout* Frozen = GetVisualState().SelectionFrozenLayouts().Find(&CardWidget))
	{
		ApplyCardLayout(
			CardWidget,
			Frozen->Center,
			Frozen->Size,
			Frozen->AngleDegrees,
			Frozen->ZOrder);
		GetRuntime().Motion.SnapLocalPose(
			CardWidget,
			FVector2D::ZeroVector,
			0.0f);
		++GetRuntime().Presentation.StaticCardPresentationUpdateCount;
		return;
	}

	int32 PileFocusCardIndex = INDEX_NONE;
	const bool bPileFocusCard = IsExpandedPileFocusCard(&CardWidget, &PileFocusCardIndex);
	const FWacomBackpackWorkspaceCardLayout* FocusTarget = GetVisualState().ExpandedFocusLayouts().Find(&CardWidget);
	const bool bPileFocused = FocusTarget
		&& PileFocusCardIndex == GetRuntime().Presentation.ExpandedPileFocus.FocusIndex;
	const bool bHovered = !InteractionModel->IsCarrying()
		&& !bPileFocusCard
		&& CardWidget.IsWorkspaceInteractionEnabled()
		&& CardWidget.GetWorkspaceReadOnlyKind()
			== EWacomBackpackWorkspaceCardReadOnlyKind::None
		&& GetRuntime().Presentation.HoveredCardWidget.Get() == &CardWidget;
	const int32 PresentationZ = FocusTarget
		? FocusTarget->ZOrder + (bPileFocused ? Style.CurrentCardZOrderBoost : 0)
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
		if (bPileFocused && !GetRuntime().Presentation.IsSimplifiedMotion())
		{
			ScreenDelta.Y -= Style.ExpandedCardHoverLiftPixels;
			TargetLocalAngle = -Base->AngleDegrees;
		}
		TargetLocalTranslation = RotateVector(ScreenDelta, -Base->AngleDegrees);
		TargetDuration = GetRuntime().Presentation.ExpandedPileFocus.FocusIndex != INDEX_NONE
			? Style.FocusReflowSeconds
			: Style.HoverExitSeconds;
	}
	else if (bHovered && !GetRuntime().Presentation.IsSimplifiedMotion())
	{
		TargetLocalTranslation = FVector2D(0.0f, -Style.ExpandedCardHoverLiftPixels);
		TargetLocalAngle = -Base->AngleDegrees;
		TargetDuration = Style.HoverEnterSeconds;
	}
	else if (bPileFocusCard)
	{
		TargetDuration = Style.HoverExitSeconds;
	}
	GetRuntime().Motion.SetLocalPoseTarget(
		CardWidget,
		TargetLocalTranslation,
		TargetLocalAngle,
		TargetDuration,
		GetRuntime().Presentation.IsSimplifiedMotion());
	++GetRuntime().Presentation.StaticCardPresentationUpdateCount;
}

void UWacomBackpackWorkspaceWidget::RequestPresentationRefresh(
	EWacomBackpackWorkspacePresentationDirty Reasons,
	TConstArrayView<FGuid> CardInstanceIds,
	bool bAllCards,
	bool bFlushImmediately)
{
	FWacomBackpackWorkspaceRuntimeHost Host(*this);
	GetRuntime().Presentation.RequestRefresh(
		Host,
		Reasons,
		CardInstanceIds,
		bAllCards,
		bFlushImmediately);
}

void UWacomBackpackWorkspaceWidget::ForEachPresentationCard(
	const FWacomBackpackWorkspacePresentationRequest& Request,
	TFunctionRef<void(UWacomDeckCardWidget&)> Apply)
{
	if (Request.bAllCards)
	{
		for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard :
			GetBoundCardWidgets())
		{
			if (UWacomDeckCardWidget* Card = WeakCard.Get())
			{
				Apply(*Card);
#if WITH_AUTOMATION_TESTS
				++LocalCardApplyCount;
#endif
			}
		}
		return;
	}

	for (const FGuid InstanceId : Request.CardInstanceIds)
	{
		if (UWacomDeckCardWidget* Card =
			GetRuntime().Visuals.FindPhysicalCard(InstanceId))
		{
			Apply(*Card);
#if WITH_AUTOMATION_TESTS
			++LocalCardApplyCount;
#endif
		}
	}
}

void UWacomBackpackWorkspaceWidget::ApplyCardSemanticsPresentation(
	const FWacomBackpackWorkspacePresentationRequest& Request)
{
	if (!InteractionModel)
	{
		return;
	}
	const FWacomBackpackWorkspaceCarryState& Carry = InteractionModel->GetCarry();
	const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
		? InteractionStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	ForEachPresentationCard(
		Request,
		[this, &Carry, Style](UWacomDeckCardWidget& CardWidget)
	{
		const FGuid InstanceId = CardWidget.GetCardInstanceId();
		const int32 CarryIndex = IsInCarryVisualLayer(&CardWidget)
			? Carry.RemainingInstanceIds.IndexOfByKey(InstanceId)
			: INDEX_NONE;
		const bool bCurrent = CarryIndex != INDEX_NONE && CarryIndex == Carry.CurrentIndex;
		const bool bSelected = InteractionModel->IsSelected(InstanceId)
			&& CardWidget.GetWorkspaceReadOnlyKind()
				== EWacomBackpackWorkspaceCardReadOnlyKind::None;
		const bool bUseReadOnlyOpacity = CardWidget.UsesReadOnlyOpacity();
		CardWidget.SetWorkspaceInteractionState(bSelected, bCurrent);
		CardWidget.ApplyWorkspaceVisualState(
			FWacomBackpackWorkspaceMotionCoordinator::BuildVisualState(
				*Style,
				bSelected,
				bCurrent,
				bUseReadOnlyOpacity));
	});
}

void UWacomBackpackWorkspaceWidget::ReconcileMotionTarget()
{
	if (!InteractionModel)
	{
		return;
	}

	const FWacomBackpackWorkspaceCarryState& Carry = InteractionModel->GetCarry();
	UWacomDeckCardWidget* DesiredCard = nullptr;
	bool bDesiredCarrying = false;
	if (InteractionModel->IsCarrying()
		&& Carry.RemainingInstanceIds.IsValidIndex(Carry.CurrentIndex))
	{
		DesiredCard = GetRuntime().Visuals.FindPhysicalCard(
			Carry.RemainingInstanceIds[Carry.CurrentIndex]);
		bDesiredCarrying = DesiredCard && IsInCarryVisualLayer(DesiredCard);
		if (!bDesiredCarrying)
		{
			DesiredCard = nullptr;
		}
	}
	if (!DesiredCard && !GetRuntime().SaleDeparture.HasWork())
	{
		DesiredCard = GetPresentationFocusedCard();
	}
	if (GetRuntime().SaleDeparture.HasWork())
	{
		DesiredCard = nullptr;
		bDesiredCarrying = false;
	}
	const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
		? InteractionStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();

	FVector2D PresentationPointer = Carry.PointerPosition;
	if (!InteractionModel->IsCarrying() && FSlateApplication::IsInitialized())
	{
		PresentationPointer = GetCachedGeometry().AbsoluteToLocal(
			FSlateApplication::Get().GetCursorPos());
	}
	GetRuntime().Motion.ReconcileActiveCard(
		DesiredCard,
		bDesiredCarrying,
		GetCachedGeometry(),
		PresentationPointer,
		*Style,
		GetRuntime().Presentation.IsSimplifiedMotion());
	if (GetRuntime().Motion.WantsTick()
		|| !GetVisualState().BaseTransitions().IsEmpty())
	{
		WakeFrameScheduler();
	}
}

void UWacomBackpackWorkspaceWidget::SetCarryInputSuspended(bool bSuspended)
{
	GetRuntime().Presentation.SetCarryInputSuspended(bSuspended);
	if (InteractionModel)
	{
		InteractionModel->SetCarryInputSuspended(bSuspended);
	}
	if (bSuspended)
	{
		SetExpandedPileLensInputLocked(false, false);
		ClearExpandedPileFocus(true);
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().ReleaseAllPointerCapture(0);
		}
	}
	RequestPresentationRefresh(
		EWacomBackpackWorkspacePresentationDirty::All,
		{},
		true);
	WakeFrameScheduler();
}

void UWacomBackpackWorkspaceWidget::BeginSaleDeparture(
	TConstArrayView<FGuid> InstanceIds)
{
	if (!InteractionModel || !InteractionModel->IsCarrying()
		|| InstanceIds.IsEmpty())
	{
		return;
	}
	if (!SettlementLayer)
	{
		EnsureFallbackTree();
	}
	if (!SettlementLayer)
	{
		return;
	}

	TSet<FGuid> RequestedIds;
	RequestedIds.Reserve(InstanceIds.Num());
	for (const FGuid InstanceId : InstanceIds)
	{
		if (InstanceId.IsValid())
		{
			RequestedIds.Add(InstanceId);
		}
	}
	if (RequestedIds.IsEmpty())
	{
		return;
	}

	const FWacomBackpackWorkspaceCarryState& Carry =
		InteractionModel->GetCarry();
	TArray<FGuid> OrderedIds;
	OrderedIds.Reserve(RequestedIds.Num());
	if (Carry.RemainingInstanceIds.IsValidIndex(Carry.CurrentIndex))
	{
		const FGuid CurrentId = Carry.RemainingInstanceIds[Carry.CurrentIndex];
		if (RequestedIds.Contains(CurrentId))
		{
			OrderedIds.Add(CurrentId);
		}
	}
	for (const FGuid InstanceId : Carry.RemainingInstanceIds)
	{
		if (RequestedIds.Contains(InstanceId) && !OrderedIds.Contains(InstanceId))
		{
			OrderedIds.Add(InstanceId);
		}
	}
	for (const FGuid InstanceId : InstanceIds)
	{
		if (RequestedIds.Contains(InstanceId) && !OrderedIds.Contains(InstanceId))
		{
			OrderedIds.Add(InstanceId);
		}
	}

	const FWacomFirstPersonCardPlayedDissolveStyleData* DissolveStyle =
		ResolvedSaleDissolveStyle
			? &ResolvedSaleDissolveStyle->Style
			: nullptr;
	bool bQueuedAny = false;
	const int32 FirstNewPendingIndex =
		GetRuntime().SaleDeparture.GetQueuedCardCount();
	int32 DepartureIndex = 0;
	for (const FGuid InstanceId : OrderedIds)
	{
		UWacomDeckCardWidget* Card =
			GetRuntime().Visuals.FindPhysicalCard(InstanceId);
		UCanvasPanelSlot* ExistingSlot =
			Card ? Cast<UCanvasPanelSlot>(Card->Slot) : nullptr;
		if (!Card || !ExistingSlot)
		{
			continue;
		}

		const FWacomBackpackWorkspaceCardVisualPose Pose =
			CaptureCardVisualPose(*Card);
		const FVector2D CardSize = ExistingSlot->GetSize();
		GetRuntime().Motion.ForgetCard(*Card);
		GetVisualState().CancelSettlementTarget(*Card);
		GetVisualState().BaseLayouts().Remove(Card);
		GetVisualState().BaseTransitions().Remove(Card);
		GetVisualState().SelectionFrozenLayouts().Remove(Card);
		GetVisualState().ExpandedFocusLayouts().Remove(Card);
		GetVisualState().RemoveReleasedHandoff(InstanceId);
		Card->UnbindWorkspacePointerEvents();
		Card->SetWorkspaceInteractionEnabled(false);
		Card->SetWorkspacePointerPassthrough(true);
		Card->PrepareForBackpackSaleDeparturePresentation();
		Wacom::Backpack::ReparentCardPreservingSlate(
			*SettlementLayer,
			*Card);
		Card->ResetBackpackLocalMotionPose();
		ApplyCardLayout(
			*Card,
			Pose.Center,
			CardSize,
			Pose.AngleDegrees,
			SaleDepartureZOrderBase + DepartureIndex++);
		Card->SetVisibility(ESlateVisibility::HitTestInvisible);

		if (!DissolveStyle
			|| !GetRuntime().SaleDeparture.Enqueue(
				*Card,
				InstanceId,
				*DissolveStyle,
				GetRuntime().Presentation.IsSimplifiedMotion()))
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Backpack sold card %s has no valid dissolve contract; removing its committed sale visual."),
				*InstanceId.ToString());
			Card->RemoveFromParent();
			continue;
		}
		bQueuedAny = true;
	}

	if (bQueuedAny)
	{
		GetRuntime().SaleDeparture.RandomizePendingTail(
			FirstNewPendingIndex);
		// The sale group owns all realtime Retainers while active. Reconcile now
		// so Hover/Carry depth cannot briefly register a fifth realtime card.
		ReconcileMotionTarget();
		WakeFrameScheduler();
	}
}

void UWacomBackpackWorkspaceWidget::ResetSaleDepartures()
{
	if (!Runtime)
	{
		return;
	}
	GetRuntime().SaleDeparture.Reset(true);
	FWacomBackpackWorkspaceRuntimeHost Host(*this);
	GetRuntime().Presentation.RefreshFrameWork(Host);
}

void UWacomBackpackWorkspaceWidget::CancelInteraction()
{
	SetExpandedPileLensInputLocked(false, false);
	SetPileDropFeedback(
		EZoneKind::Backpack,
		FGuid(),
		FWacomBackpackDropFeedbackView());
	CancelHoverExpandTimer();
	ClearExpandedPileFocus(false);
	EndSelectionVisualFreeze(false);
	RestoreAndClearPileMoveVisualSnapshot();
	GetRuntime().Gesture.ResetPendingPresses();
	GetRuntime().Presentation.SetCarryInputSuspended(false);
	GetRuntime().Presentation.bPileCollapseAnimationPending = false;
	if (InteractionModel)
	{
		InteractionModel->CancelTransientState();
	}
	StopFrameScheduler();
	GetRuntime().Presentation.bHasQueuedCarryPointer = false;
	GetRuntime().Presentation.bHasQueuedPilePointer = false;
	GetRuntime().Presentation.bCarryStripLayoutDirty = false;
	GetRuntime().Presentation.bCarryCurrentExplicitlySelectedByWheel = false;
	GetRuntime().Presentation.PreviousCarryCurrentCard.Reset();
	GetVisualState().ResetTransientMotion();
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
				if (!IsSaleDepartureCard(Card))
				{
					SettlingCards.Add(Card);
				}
			}
		}
		for (UWacomDeckCardWidget* Card : SettlingCards)
		{
			Wacom::Backpack::ReparentCardPreservingSlate(*StaticCardLayer, *Card);
			if (const FWacomBackpackWorkspaceCardLayout* Base = GetVisualState().BaseLayouts().Find(Card))
			{
				ApplyCardLayout(*Card, Base->Center, Base->Size, Base->AngleDegrees, Base->ZOrder);
			}
		}
	}
	RestoreStaticCardParents();
	GetRuntime().Presentation.bCarryVisualAnchorInitialized = false;
	GetRuntime().Presentation.CarryAnchorLocal = FVector2D::ZeroVector;
	GetRuntime().Presentation.CarryVisualAnchorLocal = FVector2D::ZeroVector;
	CancelHoverExpandTimer();
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().ReleaseAllPointerCapture(0);
	}
	RequestPresentationRefresh(
		EWacomBackpackWorkspacePresentationDirty::All,
		{},
		true);
	WakeFrameScheduler();
}

void UWacomBackpackWorkspaceWidget::ResetWorkspaceScene()
{
	ResetSaleDepartures();
	CancelInteraction();
	ResetExpandedPileFocusWindow(false, false);
	for (FWacomBackpackExpandedPileFocusCard& Entry : GetRuntime().Presentation.ExpandedPileFocus.Cards)
	{
		if (UWacomDeckCardWidget* Card = Entry.Card.Get())
		{
			Card->SetWorkspacePointerPassthrough(false);
		}
	}
	GetRuntime().Presentation.ExpandedPileFocus =
		FWacomBackpackWorkspacePresentationController::FExpandedPileFocusState();
	GetRuntime().Presentation.HoveredCardWidget.Reset();
	UnbindWorkspaceCards();

	if (InteractionModel)
	{
		InteractionModel->ReconcileCards({});
	}
	if (StaticCardLayer)
	{
		StaticCardLayer->ClearChildren();
	}
	if (CarryLayer)
	{
		CarryLayer->ClearChildren();
	}
	if (CarryActiveLayer)
	{
		CarryActiveLayer->ClearChildren();
	}
	if (SettlementLayer)
	{
		SettlementLayer->ClearChildren();
	}
	if (Runtime)
	{
		Runtime->Reset(true);
	}

	PresentedContentZone = EZoneKind::Backpack;
	PresentedContentOwnerInstanceId.Invalidate();
	GetRuntime().Presentation.ExpandedContentZone = EZoneKind::Backpack;
	GetRuntime().Presentation.ExpandedContentOwnerInstanceId.Invalidate();
	GetRuntime().Presentation.ExpandedContentBounds = FSlateRect();
	GetRuntime().Presentation.bHasExpandedContentBounds = false;
	CurrentStorageRevision = 0;
	ManualLayoutCount = 0;
	PileCount = 0;
	SetEmptyStateVisible(true);
}

void UWacomBackpackWorkspaceWidget::CancelInteractionWithReturn()
{
	if (!InteractionModel || !InteractionModel->IsCarrying()
		|| GetRuntime().Presentation.IsSimplifiedMotion())
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
	for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard : GetBoundCardWidgets())
	{
		UWacomDeckCardWidget* Card = WeakCard.Get();
		if (!Card || !ReturningIds.Contains(Card->GetCardInstanceId()))
		{
			continue;
		}
		const FWacomBackpackWorkspaceCardLayout* Target = GetVisualState().BaseLayouts().Find(Card);
		const FWacomBackpackWorkspaceCardVisualPose* Start =
			GetVisualState().FindReleasedVisualPose(Card->GetCardInstanceId());
		if (!Target || !Start || !SettlementLayer)
		{
			continue;
		}
		Wacom::Backpack::ReparentCardPreservingSlate(*SettlementLayer, *Card);
		ApplyCardLayout(*Card, Target->Center, Target->Size, Target->AngleDegrees, Target->ZOrder);
		GetVisualState().SetSettlementTarget(*Card, *Target);
		GetRuntime().Motion.BeginSettlement(
			*Card,
			RotateVector(Start->Center - Target->Center, -Target->AngleDegrees),
			FMath::FindDeltaAngleDegrees(Target->AngleDegrees, Start->AngleDegrees),
			Style->CancelReturnSeconds,
			false);
	}
	InteractionModel->CancelTransientState();
	StopFrameScheduler();
	GetRuntime().Presentation.bHasQueuedCarryPointer = false;
	GetRuntime().Presentation.bCarryStripLayoutDirty = false;
	GetVisualState().ResetReleasedHandoffs();
	GetRuntime().Presentation.bCarryVisualAnchorInitialized = false;
	if (CarryRoot)
	{
		CarryRoot->SetRenderTranslation(FVector2D::ZeroVector);
	}
	RequestPresentationRefresh(
		EWacomBackpackWorkspacePresentationDirty::All,
		{},
		true);
	WakeFrameScheduler();
}

void UWacomBackpackWorkspaceWidget::QueueCarryPointer(FVector2D Pointer)
{
	if (!InteractionModel || !InteractionModel->IsCarrying()
		|| GetRuntime().Presentation.IsCarryInputSuspended())
	{
		return;
	}
	GetRuntime().Presentation.QueuedCarryPointerLocal = Pointer;
	GetRuntime().Presentation.bHasQueuedCarryPointer = true;
	// Rule/target truth is updated immediately. Only the visual CarryRoot is delayed.
	UpdateCarryAnchor(Pointer);
}

void UWacomBackpackWorkspaceWidget::FlushQueuedCarryPointer()
{
	if (!GetRuntime().Presentation.bHasQueuedCarryPointer || !InteractionModel || !InteractionModel->IsCarrying()
		|| GetRuntime().Presentation.IsCarryInputSuspended())
	{
		return;
	}
	GetRuntime().Presentation.bHasQueuedCarryPointer = false;
	UpdateCarryAnchor(GetRuntime().Presentation.QueuedCarryPointerLocal);
}

void UWacomBackpackWorkspaceWidget::SyncCarryPointerForRelease(FVector2D Pointer)
{
	if (!InteractionModel || !InteractionModel->IsCarrying()
		|| GetRuntime().Presentation.IsCarryInputSuspended())
	{
		return;
	}
	GetRuntime().Presentation.QueuedCarryPointerLocal = Pointer;
	GetRuntime().Presentation.bHasQueuedCarryPointer = false;
	// The command uses the exact mouse-up position; the release animation captures
	// the independently smoothed visual anchor.
	UpdateCarryAnchor(Pointer);
}

void UWacomBackpackWorkspaceWidget::UpdateCarryAnchor(FVector2D Pointer, bool bUpdateModel)
{
	if (!InteractionModel || !InteractionModel->IsCarrying()
		|| GetRuntime().Presentation.IsCarryInputSuspended())
	{
		return;
	}
	GetRuntime().Presentation.CarryAnchorLocal = Pointer;
	if (bUpdateModel)
	{
		InteractionModel->UpdateCarryPointer(Pointer);
	}
	if (!GetRuntime().Presentation.bCarryVisualAnchorInitialized || GetRuntime().Presentation.IsSimplifiedMotion())
	{
		GetRuntime().Presentation.CarryVisualAnchorLocal = Pointer;
		GetRuntime().Presentation.bCarryVisualAnchorInitialized = true;
		if (CarryRoot)
		{
			CarryRoot->SetRenderTranslation(GetRuntime().Presentation.CarryVisualAnchorLocal);
		}
		++GetRuntime().Presentation.CarryVisualAnchorApplyCount;
	}
	GetRuntime().Motion.UpdatePointer(GetCachedGeometry(), Pointer, true);
}

void UWacomBackpackWorkspaceWidget::ApplyCarryVisualAnchor(float DeltaTime)
{
	if (!InteractionModel || !InteractionModel->IsCarrying() || !GetRuntime().Presentation.bCarryVisualAnchorInitialized)
	{
		return;
	}
	const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
		? InteractionStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	const FVector2D Previous = GetRuntime().Presentation.CarryVisualAnchorLocal;
	if (GetRuntime().Presentation.IsSimplifiedMotion())
	{
		GetRuntime().Presentation.CarryVisualAnchorLocal = GetRuntime().Presentation.CarryAnchorLocal;
	}
	else
	{
		GetRuntime().Presentation.CarryVisualAnchorLocal = FWacomCardMotionKernel::StepExponentialWithMaximumLag(
			GetRuntime().Presentation.CarryVisualAnchorLocal,
			GetRuntime().Presentation.CarryAnchorLocal,
			Style->CarryFollowResponseSpeed,
			Style->CarryMaximumVisualLagPixels,
			DeltaTime);
		if (GetRuntime().Presentation.CarryVisualAnchorLocal.Equals(GetRuntime().Presentation.CarryAnchorLocal, 0.5f))
		{
			GetRuntime().Presentation.CarryVisualAnchorLocal = GetRuntime().Presentation.CarryAnchorLocal;
		}
	}
	if (!Previous.Equals(GetRuntime().Presentation.CarryVisualAnchorLocal, 0.01f) && CarryRoot)
	{
		CarryRoot->SetRenderTranslation(GetRuntime().Presentation.CarryVisualAnchorLocal);
		++GetRuntime().Presentation.CarryVisualAnchorApplyCount;
	}
}

void UWacomBackpackWorkspaceWidget::WakeFrameScheduler()
{
	FWacomBackpackWorkspaceRuntimeHost Host(*this);
	GetRuntime().Presentation.WakeFrame(Host);
}

void UWacomBackpackWorkspaceWidget::EnsureFrameSchedulerRunning()
{
	FWacomBackpackWorkspaceFrameScheduler& Scheduler =
		GetRuntime().FrameScheduler;
	if (!Scheduler.WantsFrame() || Scheduler.IsTimerRegistered())
	{
		return;
	}
	const TSharedPtr<SWidget> CachedWidget = GetCachedWidget();
	if (!CachedWidget.IsValid())
	{
		return;
	}

	const uint64 TimerGeneration = Scheduler.MarkTimerRegistered();
	const TWeakObjectPtr<UWacomBackpackWorkspaceWidget> WeakThis(this);
	CachedWidget->RegisterActiveTimer(
		0.0f,
		FWidgetActiveTimerDelegate::CreateLambda(
			[WeakThis, TimerGeneration](double, float DeltaSeconds)
			{
				UWacomBackpackWorkspaceWidget* Self = WeakThis.Get();
				return Self
					? Self->TickFrameScheduler(TimerGeneration, DeltaSeconds)
					: EActiveTimerReturnType::Stop;
			}));
}

void UWacomBackpackWorkspaceWidget::StopFrameScheduler()
{
	GetRuntime().FrameScheduler.InvalidateTimer();
}

EActiveTimerReturnType UWacomBackpackWorkspaceWidget::TickFrameScheduler(
	uint64 TimerGeneration,
	float DeltaSeconds)
{
	if (!Runtime)
	{
		return EActiveTimerReturnType::Stop;
	}
	FWacomBackpackWorkspaceRuntimeHost Host(*this);
	return Runtime->Presentation.TickFrame(
		Host,
		TimerGeneration,
		DeltaSeconds);
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

bool UWacomBackpackWorkspaceWidget::IsSaleDepartureCard(
	const UWacomDeckCardWidget* CardWidget) const
{
	return Runtime && GetRuntime().SaleDeparture.ContainsCard(CardWidget);
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
		GetRuntime().Presentation.PreviousCarryCurrentCard.Reset();
		RestoreStaticCardParents();
		return;
	}

	const FWacomBackpackWorkspaceCarryState& Carry = InteractionModel->GetCarry();
	if (GetRuntime().Presentation.LastCarryStripInstanceIds != Carry.RemainingInstanceIds
		|| GetRuntime().Presentation.LastCarryStripCurrentIndex != Carry.CurrentIndex
		|| GetRuntime().Presentation.LastCarryStripDefaultIndex != Carry.DefaultIndex)
	{
		GetRuntime().Presentation.bCarryStripLayoutDirty = true;
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
		const bool bPreviousCurrentStillMoving = GetRuntime().Presentation.PreviousCarryCurrentCard.Get() == Card;
		const bool bKeepInActiveMotionLayer = InstanceId == CurrentInstanceId
			|| bPreviousCurrentStillMoving;
		UCanvasPanel* DesiredCarryLayer = bKeepInActiveMotionLayer
			? CarryActiveLayer.Get()
			: CarryLayer.Get();
		const bool bWasInSettlementLayer = IsInSettlementVisualLayer(Card);
		const bool bCancelledSettlementTarget =
			GetVisualState().CancelSettlementTarget(*Card);
		if (bWasInSettlementLayer || bCancelledSettlementTarget)
		{
			// 快速再次拿起时，Carry 必须原子接管仍在收落的同一 Widget。
			// 若只重挂父级而保留旧目标，新的携带姿态会覆盖旧 Motion，
			// 但按需帧 Timer 会继续等待一个永远不会完成的 Settlement。
			GetRuntime().Motion.StopLocalPoseMotionPreservingCurrent(*Card);
		}
		if (Card->GetParent() != DesiredCarryLayer)
		{
			Wacom::Backpack::ReparentCardPreservingSlate(*DesiredCarryLayer, *Card);
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
			|| GetVisualState().IsReleasedHandoffPending(Card->GetCardInstanceId()))
		{
			continue;
		}
		Wacom::Backpack::ReparentCardPreservingSlate(*StaticCardLayer, *Card);
		if (const FWacomBackpackWorkspaceCardLayout* Base = GetVisualState().BaseLayouts().Find(Card))
		{
			ApplyCardLayout(*Card, Base->Center, Base->Size, Base->AngleDegrees, Base->ZOrder);
		}
		bChanged = true;
	}
	if (bChanged)
	{
		GetRuntime().Presentation.bCarryStripLayoutDirty = true;
	}
	if (GetRuntime().Presentation.PreviousCarryCurrentCard.IsValid()
		&& (!Runtime
			|| !GetRuntime().Motion.IsCardMoving(*GetRuntime().Presentation.PreviousCarryCurrentCard.Get())))
	{
		GetRuntime().Presentation.PreviousCarryCurrentCard.Reset();
	}
	UpdateCarryAnchor(Carry.PointerPosition, false);
}

void UWacomBackpackWorkspaceWidget::RebuildCarryStripLayout()
{
	if (!InteractionModel || !InteractionModel->IsCarrying()
		|| !CarryLayer || !CarryActiveLayer)
	{
		GetRuntime().Presentation.bCarryStripLayoutDirty = false;
		return;
	}
	const FWacomBackpackWorkspaceCarryState& Carry = InteractionModel->GetCarry();
	const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
		? InteractionStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	const FVector2D CardDisplaySize = Style->GetCardDisplaySize();
	const float AvailableWidth = FMath::Max(
		CardDisplaySize.X,
		GetLayoutSpaceSize().X - FMath::Max(0.0f, Style->PileEdgeMarginPixels) * 2.0f);
	const TArray<FWacomBackpackCarriedStripLayout> Strip =
		FWacomBackpackWorkspaceLayoutSolver::BuildCarriedFocusWindowLayout(
			Carry.RemainingInstanceIds.Num(),
			Carry.CurrentIndex,
			Carry.DefaultIndex,
			FVector2D::ZeroVector,
			AvailableWidth,
			CardDisplaySize.X,
			Style->FocusWindowMaximumCards,
			Style->FocusWindowFullGapPixels,
			Style->FocusWindowCompressedExposurePixels,
			Style->FocusWindowMinimumExposurePixels,
			0.0f,
			GetRuntime().Presentation.LastCarryStripWindowStartIndex,
			&GetRuntime().Presentation.LastCarryStripWindowStartIndex);
	const bool bAnimateReflow = !GetRuntime().Presentation.LastCarryStripInstanceIds.IsEmpty()
		&& !Carry.bInitialReleaseGuardArmed
		&& Runtime
		&& (GetRuntime().Presentation.LastCarryStripInstanceIds != Carry.RemainingInstanceIds
			|| GetRuntime().Presentation.LastCarryStripCurrentIndex != Carry.CurrentIndex
			|| GetRuntime().Presentation.LastCarryStripDefaultIndex != Carry.DefaultIndex);
	TMap<TWeakObjectPtr<UWacomDeckCardWidget>, FWacomBackpackWorkspaceCardVisualPose> VisualStarts;
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
			FWacomBackpackWorkspaceCardLayout TargetBase;
			TargetBase.Center = Strip[Index].Transform.CardCenter;
			TargetBase.Size = CardDisplaySize;
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
						|| GetRuntime().Presentation.bCarryCurrentExplicitlySelectedByWheel);
				const FVector2D TargetLocalTranslation = bCurrentShouldLift
					&& !GetRuntime().Presentation.IsSimplifiedMotion()
					? RotateVector(
						FVector2D(0.0f, -Style->CurrentCardLiftPixels),
						-TargetBase.AngleDegrees)
					: FVector2D::ZeroVector;
				const float TargetLocalAngle = bCurrentShouldLift
					&& !GetRuntime().Presentation.IsSimplifiedMotion()
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
						GetRuntime().Presentation.IsSimplifiedMotion());
				}
			}
		}
	}
	GetRuntime().Presentation.bCarryStripLayoutDirty = false;
	GetRuntime().Presentation.LastCarryStripInstanceIds = Carry.RemainingInstanceIds;
	GetRuntime().Presentation.LastCarryStripCurrentIndex = Carry.CurrentIndex;
	GetRuntime().Presentation.LastCarryStripDefaultIndex = Carry.DefaultIndex;
	++GetRuntime().Presentation.CarryStripLayoutRebuildCount;
	WakeFrameScheduler();
}

void UWacomBackpackWorkspaceWidget::RetargetCardLocalPoseFromVisual(
	UWacomDeckCardWidget& Card,
	const FWacomBackpackWorkspaceCardVisualPose& VisualPose,
	const FWacomBackpackWorkspaceCardLayout& TargetBase,
	FVector2D TargetLocalTranslation,
	float TargetLocalAngle,
	float DurationSeconds)
{
	FWacomBackpackWorkspaceMotionCoordinator& Motion = GetRuntime().Motion;
	FVector2D TargetParentCenter = TargetBase.Center;
	if (IsInCarryVisualLayer(&Card))
	{
		TargetParentCenter += GetRuntime().Presentation.CarryVisualAnchorLocal;
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
		GetRuntime().Presentation.IsSimplifiedMotion());
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
		GetRuntime().Presentation.IsSimplifiedMotion());
}

void UWacomBackpackWorkspaceWidget::CaptureReleasedVisualPoses(
	TConstArrayView<FGuid> InstanceIds)
{
	for (const FGuid InstanceId : InstanceIds)
	{
		if (UWacomDeckCardWidget* Card = GetRuntime().Visuals.FindPhysicalCard(InstanceId))
		{
			GetVisualState().RecordReleasedVisualPose(
				InstanceId,
				CaptureCardVisualPose(*Card));
		}
	}
}

FWacomBackpackWorkspaceCardVisualPose
UWacomBackpackWorkspaceWidget::CaptureCardVisualPose(const UWacomDeckCardWidget& Card) const
{
	FWacomBackpackWorkspaceCardVisualPose Pose;
	const UCanvasPanelSlot* StaticCardSlot = Cast<UCanvasPanelSlot>(Card.Slot);
	if (!StaticCardSlot)
	{
		return Pose;
	}
	const float BaseAngle = Card.GetRenderTransformAngle();
	Pose.Center = StaticCardSlot->GetPosition() + StaticCardSlot->GetSize() * 0.5f;
	if (IsInCarryVisualLayer(&Card))
	{
		Pose.Center += GetRuntime().Presentation.CarryVisualAnchorLocal;
	}
	Pose.Center += RotateVector(Card.GetBackpackLocalMotionTranslation(), BaseAngle);
	Pose.AngleDegrees = BaseAngle + Card.GetBackpackLocalMotionAngle();
	return Pose;
}

bool UWacomBackpackWorkspaceWidget::ResolveCardDetailAnchorRect(
	const UWacomDeckCardWidget& Card,
	FSlateRect& OutWorkspaceLocalRect) const
{
	const UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Card.Slot);
	if (!CanvasSlot)
	{
		return false;
	}
	FWacomBackpackWorkspaceCardVisualPose Pose = CaptureCardVisualPose(Card);
	FVector2D CardSize = CanvasSlot->GetSize();
	if (GetRuntime().Presentation.HoveredCardWidget.Get() == &Card
		&& !GetRuntime().Presentation.IsSimplifiedMotion())
	{
		const FWacomBackpackWorkspaceCardLayoutTransition* Transition = GetVisualState().BaseTransitions().Find(&Card);
		const FWacomBackpackWorkspaceCardLayout* Base = Transition
			? &Transition->Target
			: GetVisualState().BaseLayouts().Find(&Card);
		if (Base)
		{
			const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
				? InteractionStyle.Get()
				: GetDefault<UWacomBackpackWorkspaceStyle>();
			Pose.Center = Base->Center + RotateVector(
				FVector2D(0.0f, -Style->ExpandedCardHoverLiftPixels),
				Base->AngleDegrees);
			Pose.AngleDegrees = 0.0f;
			CardSize = Base->Size;
		}
	}
	OutWorkspaceLocalRect = BuildRotatedCardBounds(
		Pose.Center,
		CardSize,
		Pose.AngleDegrees);
	return true;
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
		if (GetVisualState().IsReleasedHandoffPending(Card->GetCardInstanceId()))
		{
			bPreservedPendingHandoff = true;
			continue;
		}
		Wacom::Backpack::ReparentCardPreservingSlate(*StaticCardLayer, *Card);
		if (const FWacomBackpackWorkspaceCardLayout* Base = GetVisualState().BaseLayouts().Find(Card))
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
	GetRuntime().Presentation.LastCarryStripInstanceIds.Reset();
	GetRuntime().Presentation.bCarryCurrentExplicitlySelectedByWheel = false;
	GetRuntime().Presentation.LastCarryStripCurrentIndex = INDEX_NONE;
	GetRuntime().Presentation.LastCarryStripDefaultIndex = INDEX_NONE;
	GetRuntime().Presentation.LastCarryStripWindowStartIndex = INDEX_NONE;
	if (!InteractionModel || !InteractionModel->IsCarrying())
	{
		GetRuntime().Presentation.bCarryVisualAnchorInitialized = false;
		GetRuntime().Presentation.CarryAnchorLocal = FVector2D::ZeroVector;
		GetRuntime().Presentation.CarryVisualAnchorLocal = FVector2D::ZeroVector;
	}
}

bool UWacomBackpackWorkspaceWidget::AcceptStableLayoutGeometry(FVector2D LayoutSize)
{
	if (LayoutSize.X <= 1.0f || LayoutSize.Y <= 1.0f
		|| !FMath::IsFinite(LayoutSize.X) || !FMath::IsFinite(LayoutSize.Y)
		|| (GetRuntime().Presentation.bHasStableLayoutSize && GetRuntime().Presentation.StableLayoutSize.Equals(LayoutSize, LayoutGeometryTolerance)))
	{
		return false;
	}

	GetRuntime().Presentation.StableLayoutSize = LayoutSize;
	GetRuntime().Presentation.bHasStableLayoutSize = true;
	if (StaticCardLayer)
	{
		StaticCardLayer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	OnLayoutGeometryReadyNative.Broadcast(GetRuntime().Presentation.StableLayoutSize);
	RequestPresentationRefresh(
		EWacomBackpackWorkspacePresentationDirty::All,
		{},
		true,
		false);
	RequestDeferredCardFaceRender();
	return true;
}

void UWacomBackpackWorkspaceWidget::RequestBoundCardFaceRenders()
{
	for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCardWidget : GetBoundCardWidgets())
	{
		if (UWacomDeckCardWidget* CardWidget = WeakCardWidget.Get())
		{
			CardWidget->RequestBackpackCardFaceRender();
		}
	}
}

void UWacomBackpackWorkspaceWidget::RequestDeferredCardFaceRender()
{
	FWacomBackpackWorkspaceFrameScheduler& Scheduler =
		GetRuntime().FrameScheduler;
	Scheduler.RequestDeferredCardFaceRender(false);
	WakeFrameScheduler();
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

FWacomBackpackWorkspaceVisualState&
UWacomBackpackWorkspaceWidget::GetVisualState()
{
	return GetRuntime().VisualState;
}

const FWacomBackpackWorkspaceVisualState&
UWacomBackpackWorkspaceWidget::GetVisualState() const
{
	return GetRuntime().VisualState;
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
		FWacomBackpackWorkspaceCardLayout Target;
		Target.Center = CardCenter;
		Target.Size = CardSize;
		Target.AngleDegrees = AngleDegrees;
		Target.ZOrder = ZOrder;
		const bool bInCarryLayer = IsInCarryVisualLayer(MutableDeckCard);
		const bool bPendingReleasedHandoff = GetVisualState().IsReleasedHandoffPending(
			MutableDeckCard->GetCardInstanceId());
		bool bStillCarried = false;
		if (bInCarryLayer && InteractionModel && InteractionModel->IsCarrying())
		{
			const FWacomBackpackWorkspaceCarryState& Carry = InteractionModel->GetCarry();
			bStillCarried = Carry.RemainingInstanceIds.Contains(
				MutableDeckCard->GetCardInstanceId());
		}
		if (bPendingReleasedHandoff || (bInCarryLayer && !bStillCarried))
		{
			FWacomBackpackWorkspaceCardVisualPose StartPose =
				CaptureCardVisualPose(*MutableDeckCard);
			GetVisualState().ConsumeReleasedHandoff(
				MutableDeckCard->GetCardInstanceId(), StartPose);
			GetVisualState().BaseTransitions().Remove(MutableDeckCard);
			GetVisualState().BaseLayouts().Add(MutableDeckCard, Target);
			const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
				? InteractionStyle.Get()
				: GetDefault<UWacomBackpackWorkspaceStyle>();
			if (!GetRuntime().Presentation.IsSimplifiedMotion()
				&& Style->SettleSeconds > 0.0f && SettlementLayer)
			{
				Wacom::Backpack::ReparentCardPreservingSlate(
					*SettlementLayer, *MutableDeckCard);
				ApplyCardLayout(
					*MutableDeckCard,
					Target.Center,
					Target.Size,
					Target.AngleDegrees,
					Target.ZOrder);
				GetVisualState().SetSettlementTarget(*MutableDeckCard, Target);
				GetRuntime().Motion.BeginSettlement(
					*MutableDeckCard,
					RotateVector(StartPose.Center - Target.Center, -Target.AngleDegrees),
					FMath::FindDeltaAngleDegrees(Target.AngleDegrees, StartPose.AngleDegrees),
					Style->SettleSeconds,
					false);
				WakeFrameScheduler();
			}
			else
			{
				if (StaticCardLayer)
				{
					Wacom::Backpack::ReparentCardPreservingSlate(
						*StaticCardLayer, *MutableDeckCard);
				}
				MutableDeckCard->ResetBackpackLocalMotionPose();
				ApplyCardLayout(
					*MutableDeckCard,
					Target.Center,
					Target.Size,
					Target.AngleDegrees,
					Target.ZOrder);
			}
			GetRuntime().Presentation.bCarryStripLayoutDirty = true;
			return;
		}
		if (GetVisualState().SelectionFrozenLayouts().Contains(MutableDeckCard))
		{
			GetVisualState().BaseTransitions().Remove(MutableDeckCard);
			GetVisualState().BaseLayouts().Add(MutableDeckCard, Target);
			return;
		}
		const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
			? InteractionStyle.Get()
			: GetDefault<UWacomBackpackWorkspaceStyle>();
		if (GetVisualState().RetargetBaseLayout(
				*MutableDeckCard,
				Target,
				!GetRuntime().Presentation.IsSimplifiedMotion(),
				Style->PileExpandSeconds))
		{
			WakeFrameScheduler();
		}
	}
}

bool UWacomBackpackWorkspaceWidget::HasCardBaseLayout(const UWidget& CardWidget) const
{
	const UWacomDeckCardWidget* DeckCard = Cast<UWacomDeckCardWidget>(&CardWidget);
	return GetVisualState().HasBaseLayout(DeckCard);
}

void UWacomBackpackWorkspaceWidget::PrimeCardBaseLayout(
	UWidget& CardWidget,
	FVector2D CardCenter,
	FVector2D CardSize,
	float AngleDegrees,
	int32 ZOrder)
{
	const UWacomDeckCardWidget* DeckCard = Cast<UWacomDeckCardWidget>(&CardWidget);
	if (!DeckCard)
	{
		return;
	}
	FWacomBackpackWorkspaceCardLayout Base;
	Base.Center = CardCenter;
	Base.Size = CardSize;
	Base.AngleDegrees = AngleDegrees;
	Base.ZOrder = ZOrder;
	if (!GetVisualState().PrimeBaseLayout(
			*const_cast<UWacomDeckCardWidget*>(DeckCard), Base))
	{
		return;
	}
	ApplyCardLayout(CardWidget, CardCenter, CardSize, AngleDegrees, ZOrder);
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
void UWacomBackpackWorkspaceWidget::TickSaleDepartureForTest(
	float DeltaSeconds)
{
	FWacomBackpackWorkspaceRuntimeHost Host(*this);
	Host.AdvanceSaleDeparture(DeltaSeconds);
	GetRuntime().Presentation.RefreshFrameWork(Host);
}

void UWacomBackpackWorkspaceWidget::ForceSaleDepartureReadinessForTest()
{
	GetRuntime().SaleDeparture.ForceActiveReadinessForTest();
}

TArray<UWacomDeckCardWidget*>
UWacomBackpackWorkspaceWidget::GetActiveSaleDepartureCardsForTest() const
{
	return GetRuntime().SaleDeparture.GetActiveCardsForTest();
}

FWacomBackpackWorkspaceAutomationTestView UWacomBackpackWorkspaceWidget::GetAutomationTestView() const
{
	FWacomBackpackWorkspaceAutomationTestView View;
	View.bHasActiveZone = true;
	View.ActiveZone = PresentedContentZone;
	View.ActiveZoneOwnerInstanceId = PresentedContentOwnerInstanceId;
	View.ManualLayoutCount = ManualLayoutCount;
	View.PileCount = GetRegisteredPileWidgets().Num();
	View.WorkspaceCardCount = GetBoundCardWidgets().Num();
	View.bDeferredCardFaceRenderPending =
		GetRuntime().FrameScheduler.IsDeferredCardFaceRenderPending();
	View.DeferredCardFaceRenderPassCount = DeferredCardFaceRenderPassCount;
	View.bCardFaceRetainedRenderingEnabled = bCardFaceRetainedRenderingEnabled;
	View.bSimplifiedMotion = GetRuntime().Presentation.IsSimplifiedMotion();
	View.bCarryInputSuspended = GetRuntime().Presentation.IsCarryInputSuspended();
	View.CarryAnchorLocal = GetRuntime().Presentation.CarryAnchorLocal;
	View.CarryVisualAnchorLocal = GetRuntime().Presentation.CarryVisualAnchorLocal;
	View.CarryRootTranslation = CarryRoot ? CarryRoot->GetRenderTransform().Translation : FVector2D::ZeroVector;
	View.CarryCacheTranslation = CarryCache ? CarryCache->GetRenderTransform().Translation : FVector2D::ZeroVector;
	View.CachedCarryCardCount = CarryLayer ? CarryLayer->GetChildrenCount() : 0;
	View.ActiveCarryCardCount = CarryActiveLayer ? CarryActiveLayer->GetChildrenCount() : 0;
	View.SettlementCardCount = SettlementLayer ? SettlementLayer->GetChildrenCount() : 0;
	View.ActiveSettlementTargetCount = GetVisualState().GetActiveSettlementCount();
	View.ActiveLocalMotionCardCount = Runtime
		? GetRuntime().Motion.GetMovingCardCount()
		: 0;
	View.RealtimeCardCount = Runtime
		? GetRuntime().Motion.GetRealtimeCardCount()
			+ GetRuntime().SaleDeparture.GetRealtimeCardCount()
		: 0;
	View.SaleDepartureQueuedCardCount =
		GetRuntime().SaleDeparture.GetQueuedCardCount();
	View.SaleDepartureActiveCardCount =
		GetRuntime().SaleDeparture.GetActiveCardCount();
	View.SaleDepartureCompletedCardCount =
		GetRuntime().SaleDeparture.GetCompletedCardCount();
	View.SaleDepartureMaximumRealtimeCardCount =
		GetRuntime().SaleDeparture.GetMaximumObservedRealtimeCardCount();
	View.SaleDeparturePendingInstanceIds =
		GetRuntime().SaleDeparture.GetPendingInstanceIdsForTest();
	View.SaleDepartureActiveInstanceIds =
		GetRuntime().SaleDeparture.GetActiveInstanceIdsForTest();
	View.SaleDepartureSeeds =
		GetRuntime().SaleDeparture.GetSeedsForTest();
	View.SaleDepartureNextLaunchDelaySeconds =
		GetRuntime().SaleDeparture.GetNextLaunchDelaySecondsForTest();
	View.CarryStripLayoutRebuildCount = GetRuntime().Presentation.CarryStripLayoutRebuildCount;
	View.StaticCardPresentationUpdateCount = GetRuntime().Presentation.StaticCardPresentationUpdateCount;
	View.CarryVisualAnchorApplyCount = GetRuntime().Presentation.CarryVisualAnchorApplyCount;
	View.ActiveBaseCardLayoutTransitionCount = GetVisualState().BaseTransitions().Num();
	View.ExpandedPileFocusIndex = GetRuntime().Presentation.ExpandedPileFocus.FocusIndex;
	View.ExpandedPileLensFocus = GetRuntime().Presentation.ExpandedPileFocus.LensFocus;
	View.ExpandedPileLensLeftStackCount = GetRuntime().Presentation.ExpandedPileFocus.LensLeftStackCount;
	View.ExpandedPileLensExpandedStartIndex = GetRuntime().Presentation.ExpandedPileFocus.LensExpandedStartIndex;
	View.ExpandedPileLensExpandedCardCount = GetRuntime().Presentation.ExpandedPileFocus.LensExpandedCardCount;
	View.ExpandedPileLensRightStackCount = GetRuntime().Presentation.ExpandedPileFocus.LensRightStackCount;
	View.bExpandedPileLensInputLocked = GetRuntime().Presentation.bExpandedPileLensInputLocked;
	View.SelectionFrozenCardCount = GetVisualState().SelectionFrozenLayouts().Num();
	View.bPileMoveRollbackSnapshotActive =
		GetRuntime().Gesture.HasPileMoveSnapshot();
	View.ExpandedPileFocusLayoutRebuildCount = GetRuntime().Presentation.ExpandedPileFocusLayoutRebuildCount;
	View.bExpandedPileFocusExitPending = GetRuntime().Presentation.ExpandedPileFocus.bExitPending;
	View.FullPresentationRefreshCount = FullPresentationRefreshCount;
	const FWacomBackpackWorkspacePresentationController::FAutomationMetrics&
		Metrics = GetRuntime().Presentation.AutomationMetrics;
	View.PresentationFlushCount = Metrics.PresentationFlushCount;
	View.NavigationTargetsApplyCount = Metrics.NavigationTargetsApplyCount;
	View.CarryTopologyApplyCount = Metrics.CarryTopologyApplyCount;
	View.CarryStripApplyCount = Metrics.CarryStripApplyCount;
	View.StaticCardStageApplyCount = Metrics.StaticCardStageApplyCount;
	View.CardSemanticsStageApplyCount = Metrics.CardSemanticsStageApplyCount;
	View.MotionTargetApplyCount = Metrics.MotionTargetApplyCount;
	View.NavigationPresentationApplyCount =
		Metrics.NavigationPresentationApplyCount;
	View.AccessibilityApplyCount = Metrics.AccessibilityApplyCount;
	View.PaintInvalidationApplyCount = Metrics.PaintInvalidationApplyCount;
	View.LocalCardApplyCount = LocalCardApplyCount;
	View.bLastPresentationAppliedAllCards =
		Metrics.bLastPresentationAppliedAllCards;
	View.LastPresentationAppliedInstanceIds =
		Metrics.LastPresentationAppliedInstanceIds;
	View.WorkspaceSceneBindCount = WorkspaceSceneBindCount;
	View.BaseCardLayoutTransitionTickCount =
		Metrics.BaseCardLayoutTransitionTickCount;
	View.BaseCardLayoutTransitionApplyCount =
		Metrics.BaseCardLayoutTransitionApplyCount;
	View.bFrameSchedulerActive =
		GetRuntime().FrameScheduler.IsTimerRegistered();
	View.FrameSchedulerGeneration =
		GetRuntime().FrameScheduler.GetTimerGeneration();
	View.FrameSchedulerFrameSerial =
		GetRuntime().FrameScheduler.GetFrameSerial();
	View.FrameSchedulerTickCount = Metrics.FrameSchedulerTickCount;
	View.LastFramePhaseOrder = Metrics.LastFramePhaseOrder;
	View.ActiveBaseCardLayoutTransitionTargetCenters.Reserve(GetVisualState().BaseTransitions().Num());
	for (const TPair<TWeakObjectPtr<UWacomDeckCardWidget>, FWacomBackpackWorkspaceCardLayoutTransition>& Pair :
		GetVisualState().BaseTransitions())
	{
		View.ActiveBaseCardLayoutTransitionTargetCenters.Add(Pair.Value.Target.Center);
	}
	View.bHasExpandedContentBounds = GetRuntime().Presentation.bHasExpandedContentBounds;
	View.bPileCollapseAnimationPending = GetRuntime().Presentation.bPileCollapseAnimationPending;
	View.ExpandedContentZone = GetRuntime().Presentation.ExpandedContentZone;
	View.ExpandedContentOwnerInstanceId = GetRuntime().Presentation.ExpandedContentOwnerInstanceId;
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
