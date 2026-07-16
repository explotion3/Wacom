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
#include "UI/Backpack/WacomBackpackCardPresentationController.h"
#include "UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "UI/Backpack/WacomBackpackWorkspaceLayoutSolver.h"
#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"
#include "UI/Backpack/WacomBackpackScreenPresenter.h"
#include "UI/Backpack/WacomBackpackZonePileWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"

#define LOCTEXT_NAMESPACE "WacomBackpackWorkspace"

namespace
{
constexpr float LayoutGeometryTolerance = 0.5f;
constexpr int32 RequiredStableLayoutSamples = 2;
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
	if (!CardPresentationController)
	{
		CardPresentationController = MakeShared<FWacomBackpackCardPresentationController>();
	}
	if (CardCanvas && !bHasStableLayoutSize)
	{
		CardCanvas->SetVisibility(ESlateVisibility::Hidden);
	}
	RequestLayoutGeometryRefresh();
	if (bDeferredCardFaceRenderRequested)
	{
		ScheduleBoundCardFaceRender();
	}
}

void UWacomBackpackWorkspaceWidget::NativeDestruct()
{
	bLayoutGeometryRefreshActive = false;
	bDeferredCardFaceRenderRequested = false;
	bDeferredCardFaceRenderActive = false;
	CancelInteraction();
	if (CardPresentationController)
	{
		CardPresentationController->Reset();
	}
	for (TWeakObjectPtr<UWacomDeckCardWidget>& CardWidget : BoundCardWidgets)
	{
		if (CardWidget.IsValid())
		{
			CardWidget->UnbindWorkspacePointerEvents();
		}
	}
	BoundCardWidgets.Reset();
	// Dynamic pile widgets belong to this workspace lifecycle. Remove the actual
	// panel children as well as clearing the transient registry; otherwise a later
	// Construct/Reconcile pass can create a second set while the old inert visuals
	// are still retained by the WidgetTree.
	UCanvasPanel* ExistingPileCanvas = PileFrameLayer
		? PileFrameLayer.Get()
		: WorkspaceCanvas.Get();
	if (ExistingPileCanvas)
	{
		for (int32 ChildIndex = ExistingPileCanvas->GetChildrenCount() - 1;
			ChildIndex >= 0;
			--ChildIndex)
		{
			if (UWacomBackpackZonePileWidget* PileWidget =
				Cast<UWacomBackpackZonePileWidget>(ExistingPileCanvas->GetChildAt(ChildIndex)))
			{
				PileWidget->OnPilePointerDownNative.Unbind();
				ExistingPileCanvas->RemoveChildAt(ChildIndex);
			}
		}
	}
	for (UWacomBackpackZonePileWidget* PileWidget : PileWidgets)
	{
		if (PileWidget)
		{
			PileWidget->OnPilePointerDownNative.Unbind();
		}
	}
	PileWidgets.Reset();
	BaseCardLayouts.Reset();
	BaseCardLayoutTransitions.Reset();
	bBaseCardLayoutTransitionActive = false;
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
		Hit.bMovable = CardWidget->IsMoveEnabled();
		if (const FBaseCardLayout* Base = BaseCardLayouts.Find(CardWidget))
		{
			Hit.CardCenter = Base->Center;
			Hit.LayerRank = Base->ZOrder;
		}
		else if (const UCanvasPanelSlot* CardCanvasSlot = Cast<UCanvasPanelSlot>(CardWidget->Slot))
		{
			Hit.CardCenter = CardCanvasSlot->GetPosition() + CardCanvasSlot->GetSize() * 0.5f;
			Hit.LayerRank = CardCanvasSlot->GetZOrder();
		}
		HitRecords.Add(Hit);
		VisibleWidgets.Add(CardWidget);
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
	if (InteractionModel)
	{
		InteractionModel->ReconcileCards(HitRecords);
	}
	SyncCarryLayer();
	RefreshInteractionPresentation();
	if (CardCanvas)
	{
		CardCanvas->SetVisibility(
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

void UWacomBackpackWorkspaceWidget::ReconcilePiles(
	TConstArrayView<FWacomBackpackZonePileView> PileViews,
	TConstArrayView<FSlateRect> PileFrameRects,
	TConstArrayView<FSlateRect> PileHeaderRects,
	TConstArrayView<int32> PileLayerRanks)
{
	UCanvasPanel* Canvas = GetPileCanvas();
	if (!Canvas || !WidgetTree)
	{
		return;
	}
	// The UMG panel is the durable visual owner. A Slate destruct/reconstruct boundary
	// can clear our transient registry while leaving its dynamic UUserWidget children
	// in the WidgetTree. Reconcile from the actual panel children so the next scene
	// refresh reuses or removes those visuals instead of accumulating inert duplicates.
	TArray<UWacomBackpackZonePileWidget*> ExistingVisualPiles;
	ExistingVisualPiles.Reserve(Canvas->GetChildrenCount());
	for (int32 ChildIndex = 0; ChildIndex < Canvas->GetChildrenCount(); ++ChildIndex)
	{
		if (UWacomBackpackZonePileWidget* Existing =
			Cast<UWacomBackpackZonePileWidget>(Canvas->GetChildAt(ChildIndex)))
		{
			ExistingVisualPiles.Add(Existing);
		}
	}
	TArray<TObjectPtr<UWacomBackpackZonePileWidget>> NewOrder;
	TSet<UWacomBackpackZonePileWidget*> Used;
	TSet<FWacomBackpackZoneKey> UsedIdentities;
	UClass* PileClass = PileWidgetClass
		? PileWidgetClass.Get()
		: UWacomBackpackZonePileWidget::StaticClass();
	for (int32 Index = 0; Index < PileViews.Num(); ++Index)
	{
		const FWacomBackpackZonePileView& View = PileViews[Index];
		const FWacomBackpackZoneKey Identity = FWacomBackpackZoneKey::Make(
			View.Zone, View.OwnerInstanceId);
		if (!Identity.IsValid() || UsedIdentities.Contains(Identity))
		{
			continue;
		}
		UsedIdentities.Add(Identity);
		UWacomBackpackZonePileWidget* Pile = nullptr;
		for (UWacomBackpackZonePileWidget* Candidate : ExistingVisualPiles)
		{
			if (Candidate && !Used.Contains(Candidate)
				&& Candidate->GetPileView().HasSameIdentity(View.Zone, View.OwnerInstanceId))
			{
				Pile = Candidate;
				break;
			}
		}
		if (!Pile)
		{
			// Zone piles are UUserWidgets and need the normal CreateWidget initialization
			// path. WidgetTree::ConstructWidget is appropriate for leaf UWidgets, but it
			// leaves a nested UUserWidget without its generated/fallback tree lifecycle.
			Pile = CreateWidget<UWacomBackpackZonePileWidget>(this, PileClass);
			if (Pile)
			{
				Canvas->AddChildToCanvas(Pile);
			}
		}
		if (!Pile)
		{
			continue;
		}
		Used.Add(Pile);
		NewOrder.Add(Pile);
		Pile->SetPileView(View);
		const FSlateRect FrameRect = PileFrameRects.IsValidIndex(Index)
			? PileFrameRects[Index]
			: FSlateRect(0.0f, 0.0f, 260.0f, 380.0f);
		const FSlateRect HeaderRect = PileHeaderRects.IsValidIndex(Index)
			? PileHeaderRects[Index]
			: FSlateRect(FrameRect.Left, FrameRect.Top, FrameRect.Left + 260.0f, FrameRect.Top + 48.0f);
		Pile->SetResolvedGeometry(FrameRect, HeaderRect);
		Pile->OnPilePointerDownNative.BindUObject(this, &UWacomBackpackWorkspaceWidget::HandlePilePointerDown);
		if (UCanvasPanelSlot* PileCanvasSlot = Cast<UCanvasPanelSlot>(Pile->Slot))
		{
			PileCanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f));
			PileCanvasSlot->SetAlignment(FVector2D::ZeroVector);
			PileCanvasSlot->SetPosition(FVector2D(FrameRect.Left, FrameRect.Top));
			PileCanvasSlot->SetSize(FVector2D(
				FrameRect.Right - FrameRect.Left,
				FrameRect.Bottom - FrameRect.Top));
			PileCanvasSlot->SetZOrder(2000 + (PileLayerRanks.IsValidIndex(Index) ? PileLayerRanks[Index] : Index));
		}
	}
	for (UWacomBackpackZonePileWidget* Existing : ExistingVisualPiles)
	{
		if (Existing && !Used.Contains(Existing))
		{
			Existing->OnPilePointerDownNative.Unbind();
			Existing->RemoveFromParent();
		}
	}
	// Also retire any stale registry entry that is no longer a child of this panel.
	// This keeps delegate ownership deterministic even if another lifecycle path
	// detached a pile before this reconciliation pass.
	for (UWacomBackpackZonePileWidget* Registered : PileWidgets)
	{
		if (Registered && !Used.Contains(Registered)
			&& !ExistingVisualPiles.Contains(Registered))
		{
			Registered->OnPilePointerDownNative.Unbind();
			Registered->RemoveFromParent();
		}
	}
	PileWidgets = MoveTemp(NewOrder);
}

bool UWacomBackpackWorkspaceWidget::FindPileAtAbsolutePosition(
	FVector2D AbsolutePosition,
	EZoneKind& OutZone,
	FGuid& OutOwnerInstanceId) const
{
	const UWacomBackpackZonePileWidget* BestPile = nullptr;
	int32 BestZOrder = MIN_int32;
	for (const UWacomBackpackZonePileWidget* Pile : PileWidgets)
	{
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
	for (UWacomBackpackZonePileWidget* Pile : PileWidgets)
	{
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
	HoverExpandZone = Zone;
	HoverExpandOwnerInstanceId = Zone == EZoneKind::SpecialZone ? OwnerInstanceId : FGuid();
	const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
		? InteractionStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	const TWeakObjectPtr<UWacomBackpackWorkspaceWidget> WeakThis(this);
	if (const TSharedPtr<SWidget> Cached = GetCachedWidget())
	{
		Cached->RegisterActiveTimer(
			FMath::Max(0.0f, Style->PileHoverExpandDelaySeconds),
			FWidgetActiveTimerDelegate::CreateLambda(
				[WeakThis](double, float)
				{
					UWacomBackpackWorkspaceWidget* Self = WeakThis.Get();
					if (!Self || !Self->bHoverExpandTimerActive
						|| !Self->InteractionModel || !Self->InteractionModel->IsCarrying())
					{
						return EActiveTimerReturnType::Stop;
					}
					Self->bHoverExpandTimerActive = false;
					Self->OnPileExpansionRequestedNative.Broadcast(
						Self->HoverExpandZone, Self->HoverExpandOwnerInstanceId, true);
					return EActiveTimerReturnType::Stop;
				}));
	}
}

void UWacomBackpackWorkspaceWidget::CancelHoverExpandTimer()
{
	bHoverExpandTimerActive = false;
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
	for (UWacomBackpackZonePileWidget* Pile : PileWidgets)
	{
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
			Style->AccordionMinimumExposurePixels,
			Style->AccordionMaximumExposurePixels,
			Style->AccordionMaximumAngleDegrees,
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
	const TSharedPtr<SWidget> Cached = GetCachedWidget();
	if (!bAnimatedAny || !Cached || Style->PileExpandSeconds <= 0.0f)
	{
		return false;
	}
	bPileCollapseAnimationPending = true;
	CollapsingPileZone = Zone;
	CollapsingPileOwnerInstanceId = Zone == EZoneKind::SpecialZone ? OwnerInstanceId : FGuid();
	bCarryInputSuspended = true;
	const TWeakObjectPtr<UWacomBackpackWorkspaceWidget> WeakThis(this);
	Cached->RegisterActiveTimer(
		Style->PileExpandSeconds,
		FWidgetActiveTimerDelegate::CreateLambda(
			[WeakThis](double, float)
			{
				UWacomBackpackWorkspaceWidget* Self = WeakThis.Get();
				if (!Self || !Self->bPileCollapseAnimationPending)
				{
					return EActiveTimerReturnType::Stop;
				}
				Self->bPileCollapseAnimationPending = false;
				Self->bCarryInputSuspended = false;
				Self->OnPileCollapseAnimationFinishedNative.Broadcast(
					Self->CollapsingPileZone,
					Self->CollapsingPileOwnerInstanceId);
				return EActiveTimerReturnType::Stop;
			}));
	return true;
}

void UWacomBackpackWorkspaceWidget::SetHoveredCard(FGuid InstanceId)
{
	HoveredCardInstanceId = InstanceId;
	RefreshInteractionPresentation();
}

void UWacomBackpackWorkspaceWidget::ClearHoveredCard(FGuid InstanceId)
{
	if (HoveredCardInstanceId == InstanceId)
	{
		HoveredCardInstanceId.Invalidate();
		RefreshInteractionPresentation();
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
	if (Event.GetEffectingButton() != EKeys::LeftMouseButton || !CardWidget->IsMoveEnabled())
	{
		return FReply::Unhandled();
	}
	const FVector2D Pointer = ToLocalPointer(Event);
	if (!Event.IsControlDown() && InteractionModel->IsSelected(CardWidget->GetCardInstanceId()))
	{
		const bool bStarted = InteractionModel->BeginCarry(
			CardWidget->GetCardInstanceId(),
			Pointer,
			CurrentStorageRevision);
		if (bStarted)
		{
			bPendingCardPress = false;
			UpdateCarryAnchor(Pointer);
			bCarryFanLayoutDirty = true;
			SyncCarryLayer();
			RebuildCarryFanLayout();
			StartCarryPointerTracking();
			RefreshInteractionPresentation();
			OnInteractionChangedNative.Broadcast();
			return BuildHandledPointerReply();
		}
	}
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
	if (CardPresentationController && HoveredCardInstanceId.IsValid())
	{
		CardPresentationController->UpdatePointer(GetCachedGeometry(), Pointer, false);
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
				}
			}
		}
	}
	RefreshInteractionPresentation();
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

	UpdateCarryAnchor(Pointer);
	bCarryFanLayoutDirty = true;
	SyncCarryLayer();
	RebuildCarryFanLayout();
	StartCarryPointerTracking();
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
	bPendingPilePress = true;
	PendingPileWidget = PileWidget;
	PendingPilePressPosition = ToLocalPointer(Event);
	const FSlateRect HeaderRect = PileWidget->GetResolvedHeaderRect();
	PendingPileStartPosition = FVector2D(HeaderRect.Left, HeaderRect.Top);
	InteractionModel->ClickBlank();
	return FReply::Handled().CaptureMouse(TakeWidget()).SetUserFocus(TakeWidget());
}

bool UWacomBackpackWorkspaceWidget::TryBeginPileMove(FVector2D Pointer)
{
	UWacomBackpackZonePileWidget* Pile = PendingPileWidget.Get();
	if (!InteractionModel || !bPendingPilePress || !Pile
		|| !Pile->GetPileView().bMovable
		|| !Pile->WasLastPointerDownOnDragHandle()
		|| FVector2D::Distance(PendingPilePressPosition, Pointer) < 5.0f)
	{
		return false;
	}
	const FWacomBackpackZoneKey Zone = FWacomBackpackZoneKey::Make(
		Pile->GetPileView().Zone, Pile->GetPileView().OwnerInstanceId);
	if (!InteractionModel->BeginPileMove(Zone, PendingPilePressPosition, PendingPileStartPosition))
	{
		return false;
	}
	bPendingPilePress = false;
	QueuePilePointer(Pointer);
	FlushQueuedPilePointer();
	StartPilePointerTracking();
	OnInteractionChangedNative.Broadcast();
	return true;
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
	for (UWacomBackpackZonePileWidget* Pile : PileWidgets)
	{
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
	if (!InteractionModel || !InteractionModel->IsPileMoving() || bPilePointerTrackingActive)
	{
		return;
	}
	const TSharedPtr<SWidget> CachedWidget = GetCachedWidget();
	if (!CachedWidget)
	{
		return;
	}
	bPilePointerTrackingActive = true;
	const TWeakObjectPtr<UWacomBackpackWorkspaceWidget> WeakThis(this);
	CachedWidget->RegisterActiveTimer(
		0.0f,
		FWidgetActiveTimerDelegate::CreateLambda(
			[WeakThis](double, float)
			{
				UWacomBackpackWorkspaceWidget* Self = WeakThis.Get();
				if (!Self || !Self->InteractionModel || !Self->InteractionModel->IsPileMoving())
				{
					if (Self)
					{
						Self->bPilePointerTrackingActive = false;
						Self->bHasQueuedPilePointer = false;
					}
					return EActiveTimerReturnType::Stop;
				}
				if (FSlateApplication::IsInitialized())
				{
					Self->QueuePilePointer(Self->GetCachedGeometry().AbsoluteToLocal(
						FSlateApplication::Get().GetCursorPos()));
				}
				Self->FlushQueuedPilePointer();
				return EActiveTimerReturnType::Continue;
			}));
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
		InteractionModel->BeginMarquee(
			ResolveMarqueeSource(Pointer), Pointer, InMouseEvent.IsControlDown());
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
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
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
		for (const UWacomBackpackZonePileWidget* OtherPile : PileWidgets)
		{
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
		for (UWacomBackpackZonePileWidget* Pile : PileWidgets)
		{
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
		bPilePointerTrackingActive = false;
		bHasQueuedPilePointer = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	if (bPendingPilePress && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (UWacomBackpackZonePileWidget* Pile = PendingPileWidget.Get())
		{
			OnPileExpansionRequestedNative.Broadcast(
				Pile->GetPileView().Zone, Pile->GetPileView().OwnerInstanceId, false);
		}
		PendingPileWidget.Reset();
		bPendingPilePress = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	if (InteractionModel->IsMarqueeActive() && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		InteractionModel->UpdateMarquee(ToLocalPointer(InMouseEvent));
		InteractionModel->CompleteMarquee();
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
		InteractionModel->StepCurrentByWheel(InMouseEvent.GetWheelDelta());
		bCarryFanLayoutDirty = true;
		RebuildCarryFanLayout();
		RefreshInteractionPresentation();
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
		CancelInteraction();
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

void UWacomBackpackWorkspaceWidget::RefreshInteractionPresentation()
{
	if (!InteractionModel)
	{
		return;
	}
	SyncCarryLayer();
	if (bCarryFanLayoutDirty)
	{
		RebuildCarryFanLayout();
	}
	const FWacomBackpackWorkspaceCarryState& Carry = InteractionModel->GetCarry();
	const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
		? InteractionStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();

	for (TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard : BoundCardWidgets)
	{
		UWacomDeckCardWidget* CardWidget = WeakCard.Get();
		if (!CardWidget)
		{
			continue;
		}
		const FGuid InstanceId = CardWidget->GetCardInstanceId();
		const bool bInCarryLayer = IsInCarryVisualLayer(CardWidget);
		const int32 CarryIndex = bInCarryLayer
			? Carry.RemainingInstanceIds.IndexOfByKey(InstanceId)
			: INDEX_NONE;
		const FBaseCardLayoutTransition* Transition = BaseCardLayoutTransitions.Find(CardWidget);
		const FBaseCardLayout* Base = Transition
			? &Transition->Current
			: BaseCardLayouts.Find(CardWidget);
		if (Base && !bInCarryLayer)
		{
			FVector2D Center = Base->Center;
			int32 LayerRank = Base->ZOrder;
			if (!InteractionModel->IsCarrying()
				&& CardWidget->IsWorkspaceInteractionEnabled()
				&& HoveredCardInstanceId == InstanceId)
			{
				Center.Y -= Style->AccordionHoverLiftPixels;
				LayerRank += 5000;
			}
			ApplyCardLayout(
				*CardWidget,
				Center,
				Base->Size,
				Base->AngleDegrees,
				LayerRank);
			++StaticCardPresentationUpdateCount;
		}
		const bool bCurrent = CarryIndex != INDEX_NONE && CarryIndex == Carry.CurrentIndex;
		const bool bSelected = InteractionModel->IsSelected(InstanceId)
			&& CardWidget->GetWorkspaceReadOnlyKind()
				== EWacomBackpackWorkspaceCardReadOnlyKind::None;
		const bool bUseReadOnlyOpacity = CardWidget->UsesReadOnlyOpacity();
		CardWidget->SetWorkspaceVisualState(bSelected, bCurrent, bUseReadOnlyOpacity);
		CardWidget->ApplyWorkspaceVisualState(
			UWacomBackpackScreenPresenter::BuildWorkspaceCardVisualState(
				Style,
				bSelected,
				bCurrent,
				bUseReadOnlyOpacity));
	}

	if (!CardPresentationController)
	{
		CardPresentationController = MakeShared<FWacomBackpackCardPresentationController>();
	}
	FVector2D PresentationPointer = Carry.PointerPosition;
	if (!InteractionModel->IsCarrying() && FSlateApplication::IsInitialized())
	{
		PresentationPointer = GetCachedGeometry().AbsoluteToLocal(
			FSlateApplication::Get().GetCursorPos());
	}
	CardPresentationController->Reconcile(
		BoundCardWidgets,
		HoveredCardInstanceId,
		InteractionModel->IsCarrying() ? &Carry : nullptr,
		CarryActiveLayer ? CarryActiveLayer.Get() : CarryLayer.Get(),
		GetCachedGeometry(),
		PresentationPointer);

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
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().ReleaseAllPointerCapture(0);
		}
	}
	else if (InteractionModel && InteractionModel->IsCarrying())
	{
		StartCarryPointerTracking();
	}
	RefreshInteractionPresentation();
}

void UWacomBackpackWorkspaceWidget::CancelInteraction()
{
	SetPileDropPreview(EZoneKind::Backpack, FGuid(), false, false);
	CancelHoverExpandTimer();
	bPendingCardPress = false;
	PendingCardPressId.Invalidate();
	bPendingPilePress = false;
	PendingPileWidget.Reset();
	bCarryInputSuspended = false;
	bPileCollapseAnimationPending = false;
	if (InteractionModel)
	{
		InteractionModel->CancelTransientState();
	}
	bCarryPointerTrackingActive = false;
	bPilePointerTrackingActive = false;
	bHasQueuedCarryPointer = false;
	bHasQueuedPilePointer = false;
	bCarryFanLayoutDirty = false;
	PendingReleasedVisualHandoffs.Reset();
	RestoreStaticCardParents();
	CancelHoverExpandTimer();
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().ReleaseAllPointerCapture(0);
	}
	RefreshInteractionPresentation();
}

void UWacomBackpackWorkspaceWidget::QueueCarryPointer(FVector2D Pointer)
{
	if (!InteractionModel || !InteractionModel->IsCarrying() || bCarryInputSuspended)
	{
		return;
	}
	QueuedCarryPointerLocal = Pointer;
	bHasQueuedCarryPointer = true;
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
	bHasQueuedCarryPointer = true;
	// The command must use the mouse-up position even when the visual anchor is
	// deliberately waiting for the next Slate-frame coalesced apply.
	InteractionModel->UpdateCarryPointer(Pointer);
}

void UWacomBackpackWorkspaceWidget::UpdateCarryAnchor(FVector2D Pointer, bool bUpdateModel)
{
	if (!InteractionModel || !InteractionModel->IsCarrying() || bCarryInputSuspended)
	{
		return;
	}
	const bool bVisualAnchorChanged = !CarryAnchorLocal.Equals(Pointer, 0.1f)
		|| (CarryRoot && !CarryRoot->GetRenderTransform().Translation.Equals(Pointer, 0.1f));
	CarryAnchorLocal = Pointer;
	if (bUpdateModel)
	{
		InteractionModel->UpdateCarryPointer(Pointer);
	}
	if (bVisualAnchorChanged && CarryRoot)
	{
		CarryRoot->SetRenderTranslation(Pointer);
	}
	else if (bVisualAnchorChanged && CarryCache)
	{
		CarryCache->SetRenderTranslation(Pointer);
	}
	else if (bVisualAnchorChanged && CarryLayer)
	{
		CarryLayer->SetRenderTranslation(Pointer);
	}
	if (bVisualAnchorChanged)
	{
		++CarryVisualAnchorApplyCount;
	}
	if (CardPresentationController)
	{
		CardPresentationController->UpdatePointer(GetCachedGeometry(), Pointer, true);
	}
}

void UWacomBackpackWorkspaceWidget::StartCarryPointerTracking()
{
	if (!InteractionModel || !InteractionModel->IsCarrying()
		|| bCarryInputSuspended || bCarryPointerTrackingActive)
	{
		return;
	}
	TSharedPtr<SWidget> CachedWidget = GetCachedWidget();
	if (!CachedWidget.IsValid())
	{
		return;
	}
	bCarryPointerTrackingActive = true;
	bHasQueuedCarryPointer = false;
	const TWeakObjectPtr<UWacomBackpackWorkspaceWidget> WeakThis(this);
	CachedWidget->RegisterActiveTimer(
		0.0f,
		FWidgetActiveTimerDelegate::CreateLambda(
			[WeakThis](double, float)
			{
				UWacomBackpackWorkspaceWidget* Self = WeakThis.Get();
				if (Self && Self->bCarryInputSuspended)
				{
					Self->bCarryPointerTrackingActive = false;
					return EActiveTimerReturnType::Stop;
				}
				if (!Self || !Self->InteractionModel || !Self->InteractionModel->IsCarrying())
				{
					if (Self)
					{
						Self->bCarryPointerTrackingActive = false;
						Self->RestoreStaticCardParents();
					}
					return EActiveTimerReturnType::Stop;
				}
				if (FSlateApplication::IsInitialized())
				{
					const FVector2D LatestPointer = Self->GetCachedGeometry().AbsoluteToLocal(
						FSlateApplication::Get().GetCursorPos());
					Self->QueueCarryPointer(LatestPointer);
					Self->FlushQueuedCarryPointer();
				}
				return EActiveTimerReturnType::Continue;
			}));
}

bool UWacomBackpackWorkspaceWidget::ShouldPreserveCardParent(
	const UWacomDeckCardWidget* CardWidget) const
{
	// Carry layers own an existing card until ApplyCardBaseLayout consumes the new
	// scene target. This also covers the short successful-release interval after
	// the interaction model has removed the card but before static placement.
	return IsInCarryVisualLayer(CardWidget);
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
		RestoreStaticCardParents();
		return;
	}

	const FWacomBackpackWorkspaceCarryState& Carry = InteractionModel->GetCarry();
	if (LastCarryFanInstanceIds != Carry.RemainingInstanceIds
		|| LastCarryFanCurrentIndex != Carry.CurrentIndex
		|| LastCarryFanDefaultIndex != Carry.DefaultIndex)
	{
		bCarryFanLayoutDirty = true;
	}
	const FGuid CurrentInstanceId = Carry.RemainingInstanceIds.IsValidIndex(Carry.CurrentIndex)
		? Carry.RemainingInstanceIds[Carry.CurrentIndex]
		: FGuid();
	bool bChanged = false;
	for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard : BoundCardWidgets)
	{
		UWacomDeckCardWidget* Card = WeakCard.Get();
		if (!Card)
		{
			continue;
		}
		const bool bShouldCarry =
			Card->GetWorkspaceReadOnlyKind() == EWacomBackpackWorkspaceCardReadOnlyKind::None
			&& Carry.SourceZone == FWacomBackpackZoneKey::Make(
				Card->GetWorkspaceDisplayZone(), Card->GetWorkspaceDisplayOwnerInstanceId())
			&& Carry.RemainingInstanceIds.Contains(Card->GetCardInstanceId());
		UCanvasPanel* DesiredCarryLayer = Card->GetCardInstanceId() == CurrentInstanceId
			? CarryActiveLayer.Get()
			: CarryLayer.Get();
		if (bShouldCarry && Card->GetParent() != DesiredCarryLayer)
		{
			Card->RemoveFromParent();
			DesiredCarryLayer->AddChildToCanvas(Card);
			Card->SetVisibility(ESlateVisibility::Visible);
			bChanged = true;
		}
		else if (!bShouldCarry && IsInCarryVisualLayer(Card)
			&& !PendingReleasedVisualHandoffs.Contains(Card->GetCardInstanceId()))
		{
			Card->RemoveFromParent();
			StaticCardLayer->AddChildToCanvas(Card);
			if (const FBaseCardLayout* Base = BaseCardLayouts.Find(Card))
			{
				ApplyCardLayout(*Card, Base->Center, Base->Size, Base->AngleDegrees, Base->ZOrder);
			}
			bChanged = true;
		}
	}
	if (bChanged)
	{
		bCarryFanLayoutDirty = true;
	}
	UpdateCarryAnchor(Carry.PointerPosition, false);
}

void UWacomBackpackWorkspaceWidget::RebuildCarryFanLayout()
{
	if (!InteractionModel || !InteractionModel->IsCarrying()
		|| !CarryLayer || !CarryActiveLayer)
	{
		bCarryFanLayoutDirty = false;
		return;
	}
	const FWacomBackpackWorkspaceCarryState& Carry = InteractionModel->GetCarry();
	const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
		? InteractionStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	const TArray<FWacomBackpackCarriedFanLayout> Fan =
		FWacomBackpackWorkspaceLayoutSolver::BuildCarriedFanLayout(
			Carry.RemainingInstanceIds.Num(),
			Carry.CurrentIndex,
			Carry.DefaultIndex,
			FVector2D::ZeroVector,
			Style->FanMaximumAngleDegrees,
			Style->FanCardSpacingPixels,
			Style->CurrentCardLiftPixels);
	for (int32 Index = 0; Index < Carry.RemainingInstanceIds.Num(); ++Index)
	{
		if (!Fan.IsValidIndex(Index))
		{
			continue;
		}
		UWacomDeckCardWidget* Card = nullptr;
		for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard : BoundCardWidgets)
		{
			UWacomDeckCardWidget* Candidate = WeakCard.Get();
			if (Candidate && IsInCarryVisualLayer(Candidate)
				&& Candidate->GetCardInstanceId() == Carry.RemainingInstanceIds[Index])
			{
				Card = Candidate;
				break;
			}
		}
		if (Card)
		{
			ApplyCardLayout(
				*Card,
				Fan[Index].Transform.CardCenter,
				Style->CardRenderSize,
				Fan[Index].Transform.AngleDegrees,
				Index);
		}
	}
	bCarryFanLayoutDirty = false;
	LastCarryFanInstanceIds = Carry.RemainingInstanceIds;
	LastCarryFanCurrentIndex = Carry.CurrentIndex;
	LastCarryFanDefaultIndex = Carry.DefaultIndex;
	++CarryFanLayoutRebuildCount;
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
	LastCarryFanInstanceIds.Reset();
	LastCarryFanCurrentIndex = INDEX_NONE;
	LastCarryFanDefaultIndex = INDEX_NONE;
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
	if (CardCanvas)
	{
		CardCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
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
	if (!bHasStableLayoutSize || !CardCanvas
		|| CardCanvas->GetVisibility() == ESlateVisibility::Hidden
		|| CardCanvas->GetVisibility() == ESlateVisibility::Collapsed)
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
	CardCanvas = StaticCardLayer;

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

UCanvasPanel* UWacomBackpackWorkspaceWidget::GetCardCanvas()
{
	EnsureFallbackTree();
	return StaticCardLayer ? StaticCardLayer.Get() : CardCanvas.Get();
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

UCanvasPanel* UWacomBackpackWorkspaceWidget::GetPileCanvas()
{
	EnsureFallbackTree();
	return PileFrameLayer ? PileFrameLayer.Get() : WorkspaceCanvas.Get();
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
			PendingReleasedVisualHandoffs.Remove(MutableDeckCard->GetCardInstanceId());
			BaseCardLayoutTransitions.Remove(MutableDeckCard);
			BaseCardLayouts.Add(MutableDeckCard, Target);
			if (StaticCardLayer)
			{
				MutableDeckCard->RemoveFromParent();
				StaticCardLayer->AddChildToCanvas(MutableDeckCard);
			}
			ApplyCardLayout(
				*MutableDeckCard,
				Target.Center,
				Target.Size,
				Target.AngleDegrees,
				Target.ZOrder);
			bCarryFanLayoutDirty = true;
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
			StartBaseCardLayoutTransitions();
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

void UWacomBackpackWorkspaceWidget::StartBaseCardLayoutTransitions()
{
	if (bBaseCardLayoutTransitionActive || bSimplifiedMotion)
	{
		return;
	}
	const TSharedPtr<SWidget> Cached = GetCachedWidget();
	if (!Cached)
	{
		return;
	}
	bBaseCardLayoutTransitionActive = true;
	const TWeakObjectPtr<UWacomBackpackWorkspaceWidget> WeakThis(this);
	Cached->RegisterActiveTimer(
		0.0f,
		FWidgetActiveTimerDelegate::CreateLambda(
			[WeakThis](double, float DeltaSeconds)
			{
				UWacomBackpackWorkspaceWidget* Self = WeakThis.Get();
				if (!Self || Self->bSimplifiedMotion)
				{
					if (Self)
					{
						Self->BaseCardLayoutTransitions.Reset();
						Self->bBaseCardLayoutTransitionActive = false;
						Self->RefreshInteractionPresentation();
					}
					return EActiveTimerReturnType::Stop;
				}
				for (auto It = Self->BaseCardLayoutTransitions.CreateIterator(); It; ++It)
				{
					FBaseCardLayoutTransition& Transition = It.Value();
					Transition.ElapsedSeconds += FMath::Max(0.0f, DeltaSeconds);
					const float Alpha = Transition.DurationSeconds > 0.0f
						? FMath::Clamp(Transition.ElapsedSeconds / Transition.DurationSeconds, 0.0f, 1.0f)
						: 1.0f;
					const float Smoothed = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);
					Transition.Current.Center = FMath::Lerp(
						Transition.Start.Center, Transition.Target.Center, Smoothed);
					Transition.Current.Size = Transition.Target.Size;
					Transition.Current.AngleDegrees = FMath::Lerp(
						Transition.Start.AngleDegrees, Transition.Target.AngleDegrees, Smoothed);
					Transition.Current.ZOrder = Transition.Target.ZOrder;
					if (Alpha >= 1.0f)
					{
						It.RemoveCurrent();
					}
				}
				Self->RefreshInteractionPresentation();
				if (Self->BaseCardLayoutTransitions.IsEmpty())
				{
					Self->bBaseCardLayoutTransitionActive = false;
					return EActiveTimerReturnType::Stop;
				}
				return EActiveTimerReturnType::Continue;
			}));
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
	View.PileCount = PileWidgets.Num();
	View.bDeferredCardFaceRenderPending =
		bDeferredCardFaceRenderRequested || bDeferredCardFaceRenderActive;
	View.DeferredCardFaceRenderPassCount = DeferredCardFaceRenderPassCount;
	View.bCardFaceRetainedRenderingEnabled = bCardFaceRetainedRenderingEnabled;
	View.CarryAnchorLocal = CarryAnchorLocal;
	View.CarryRootTranslation = CarryRoot ? CarryRoot->GetRenderTransform().Translation : FVector2D::ZeroVector;
	View.CarryCacheTranslation = CarryCache ? CarryCache->GetRenderTransform().Translation : FVector2D::ZeroVector;
	View.CachedCarryCardCount = CarryLayer ? CarryLayer->GetChildrenCount() : 0;
	View.ActiveCarryCardCount = CarryActiveLayer ? CarryActiveLayer->GetChildrenCount() : 0;
	View.CarryFanLayoutRebuildCount = CarryFanLayoutRebuildCount;
	View.StaticCardPresentationUpdateCount = StaticCardPresentationUpdateCount;
	View.CarryVisualAnchorApplyCount = CarryVisualAnchorApplyCount;
	View.ActiveBaseCardLayoutTransitionCount = BaseCardLayoutTransitions.Num();
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
