// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "UI/Backpack/WacomBackpackWorkspaceLayoutSolver.h"
#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"
#include "UI/Backpack/WacomBackpackScreenPresenter.h"
#include "UI/Backpack/WacomBackpackZonePileWidget.h"
#include "UI/Backpack/WacomBackpackPilePreviewWidget.h"
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
	for (TWeakObjectPtr<UWacomDeckCardWidget>& CardWidget : BoundCardWidgets)
	{
		if (CardWidget.IsValid())
		{
			CardWidget->UnbindWorkspacePointerEvents();
		}
	}
	BoundCardWidgets.Reset();
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
	TSet<FGuid> VisibleIds;
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
			CardWidget->GetFromZone(), CardWidget->GetFromZoneOwnerInstanceId());
		Hit.bMovable = CardWidget->IsMoveEnabled();
		if (const UCanvasPanelSlot* CardCanvasSlot = Cast<UCanvasPanelSlot>(CardWidget->Slot))
		{
			Hit.CardCenter = CardCanvasSlot->GetPosition() + CardCanvasSlot->GetSize() * 0.5f;
			Hit.LayerRank = CardCanvasSlot->GetZOrder();
		}
		HitRecords.Add(Hit);
		VisibleIds.Add(Hit.InstanceId);
	}
	for (auto It = BaseCardLayouts.CreateIterator(); It; ++It)
	{
		if (!VisibleIds.Contains(It.Key()))
		{
			It.RemoveCurrent();
		}
	}
	for (auto It = BaseCardLayoutTransitions.CreateIterator(); It; ++It)
	{
		if (!VisibleIds.Contains(It.Key()))
		{
			It.RemoveCurrent();
		}
	}
	if (InteractionModel)
	{
		InteractionModel->ReconcileCards(HitRecords);
	}
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
	TConstArrayView<FVector2D> PileTopLefts,
	TConstArrayView<int32> PileLayerRanks)
{
	UCanvasPanel* Canvas = GetCardCanvas();
	if (!Canvas || !WidgetTree)
	{
		return;
	}
	TArray<TObjectPtr<UWacomBackpackZonePileWidget>> NewOrder;
	TSet<UWacomBackpackZonePileWidget*> Used;
	UClass* PileClass = PileWidgetClass
		? PileWidgetClass.Get()
		: UWacomBackpackZonePileWidget::StaticClass();
	for (int32 Index = 0; Index < PileViews.Num(); ++Index)
	{
		const FWacomBackpackZonePileView& View = PileViews[Index];
		UWacomBackpackZonePileWidget* Pile = nullptr;
		for (UWacomBackpackZonePileWidget* Candidate : PileWidgets)
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
		Pile->SetPreviewWidgetClass(PilePreviewWidgetClass);
		Pile->SetPileView(View);
		Pile->OnPilePointerDownNative.BindUObject(this, &UWacomBackpackWorkspaceWidget::HandlePilePointerDown);
		if (UCanvasPanelSlot* PileCanvasSlot = Cast<UCanvasPanelSlot>(Pile->Slot))
		{
			PileCanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f));
			PileCanvasSlot->SetAlignment(FVector2D::ZeroVector);
			PileCanvasSlot->SetPosition(PileTopLefts.IsValidIndex(Index) ? PileTopLefts[Index] : FVector2D::ZeroVector);
			const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
				? InteractionStyle.Get()
				: GetDefault<UWacomBackpackWorkspaceStyle>();
			PileCanvasSlot->SetSize(Style->PileCollapsedSize);
			PileCanvasSlot->SetZOrder(2000 + (PileLayerRanks.IsValidIndex(Index) ? PileLayerRanks[Index] : Index));
		}
	}
	for (UWacomBackpackZonePileWidget* Existing : PileWidgets)
	{
		if (Existing && !Used.Contains(Existing))
		{
			Existing->OnPilePointerDownNative.Unbind();
			Canvas->RemoveChild(Existing);
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
	const FVector2D TargetCenter = PileCanvasSlot->GetPosition() + Style->PileCollapsedSize * 0.5f;
	bool bAnimatedAny = false;
	for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard : BoundCardWidgets)
	{
		UWacomDeckCardWidget* Card = WeakCard.Get();
		const FBaseCardLayout* Base = Card
			? BaseCardLayouts.Find(Card->GetCardInstanceId())
			: nullptr;
		if (!Card || !Base || Base->ZOrder < 3000 || Base->ZOrder >= 10000)
		{
			continue;
		}
		ApplyCardBaseLayout(
			*Card,
			TargetCenter,
			Base->Size,
			0.0f,
			Base->ZOrder);
		bAnimatedAny = true;
	}
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
			DisplayedCarryPointer = Pointer;
			bHasDisplayedCarryPointer = true;
			StartCarryInterpolation();
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
		InteractionModel->UpdateCarryPointer(Pointer);
		StartCarryInterpolation();
		OnInteractionChangedNative.Broadcast();
		return BuildHandledPointerReply();
	}
	if (TryBeginCarryFromPendingPress(Pointer))
	{
		return BuildHandledPointerReply();
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
	if (!Intent.bConsumedByInitialReleaseGuard && !Intent.InstanceIds.IsEmpty())
	{
		OnReleaseIntentNative.Broadcast(Intent);
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

	DisplayedCarryPointer = Pointer;
	bHasDisplayedCarryPointer = true;
	StartCarryInterpolation();
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
	if (const UCanvasPanelSlot* PileCanvasSlot = Cast<UCanvasPanelSlot>(PileWidget->Slot))
	{
		PendingPileStartPosition = PileCanvasSlot->GetPosition();
	}
	InteractionModel->ClickBlank();
	return FReply::Handled().CaptureMouse(TakeWidget()).SetUserFocus(TakeWidget());
}

bool UWacomBackpackWorkspaceWidget::TryBeginPileMove(FVector2D Pointer)
{
	UWacomBackpackZonePileWidget* Pile = PendingPileWidget.Get();
	if (!InteractionModel || !bPendingPilePress || !Pile
		|| !Pile->GetPileView().bMovable
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
	ApplyActivePileMove();
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
		Style->PileCollapsedSize,
		1.0f,
		Style->PileEdgeMarginPixels);
	for (UWacomBackpackZonePileWidget* Pile : PileWidgets)
	{
		if (Pile && Pile->GetPileView().HasSameIdentity(Move.Zone.Zone, Move.Zone.OwnerInstanceId))
		{
			if (UCanvasPanelSlot* PileCanvasSlot = Cast<UCanvasPanelSlot>(Pile->Slot))
			{
				PileCanvasSlot->SetPosition(ClampedPosition);
				PileCanvasSlot->SetZOrder(9000);
			}
			break;
		}
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
		InteractionModel->UpdateCarryPointer(ToLocalPointer(InMouseEvent));
		StartCarryInterpolation();
		OnInteractionChangedNative.Broadcast();
		return BuildHandledPointerReply();
	}
	if (InteractionModel->IsPileMoving())
	{
		InteractionModel->UpdatePileMove(ToLocalPointer(InMouseEvent));
		ApplyActivePileMove();
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
			BroadcastRelease(InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton);
			return BuildHandledPointerReply();
		}
		return FReply::Unhandled();
	}
	if (InteractionModel->IsPileMoving() && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
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
			if (const UCanvasPanelSlot* OtherSlot = Cast<UCanvasPanelSlot>(OtherPile->Slot))
			{
				const FVector2D OtherPosition = OtherSlot->GetPosition();
				OccupiedHeaders.Emplace(
					OtherPosition.X,
					OtherPosition.Y,
					OtherPosition.X + Style->PileCollapsedSize.X,
					OtherPosition.Y + 48.0f);
			}
		}
		const FVector2D Snapped = FWacomBackpackWorkspaceLayoutSolver::ResolvePileHeaderOverlap(
			Completed.CurrentPosition,
			WorkspaceSize,
			Style->PileCollapsedSize,
			FVector2D(Style->PileCollapsedSize.X, 48.0f),
			Style->PileSnapGridPixels,
			Style->PileEdgeMarginPixels,
			OccupiedHeaders);
		for (UWacomBackpackZonePileWidget* Pile : PileWidgets)
		{
			if (Pile && Pile->GetPileView().HasSameIdentity(
				Completed.Zone.Zone, Completed.Zone.OwnerInstanceId))
			{
				if (UCanvasPanelSlot* PileCanvasSlot = Cast<UCanvasPanelSlot>(Pile->Slot))
				{
					const FVector2D VisualStart = PileCanvasSlot->GetPosition();
					PileCanvasSlot->SetPosition(Snapped);
					if (!bSimplifiedMotion && Style->PileSnapSeconds > 0.0f
						&& !VisualStart.Equals(Snapped, 0.5f))
					{
						Pile->SetRenderTranslation(VisualStart - Snapped);
						const TWeakObjectPtr<UWacomBackpackZonePileWidget> WeakPile(Pile);
						const float Duration = Style->PileSnapSeconds;
						const TSharedPtr<SWidget> CachedPile = Pile->GetCachedWidget();
						if (CachedPile)
						{
							const FVector2D InitialOffset = VisualStart - Snapped;
							const TSharedRef<float> Elapsed = MakeShared<float>(0.0f);
							CachedPile->RegisterActiveTimer(
								0.0f,
								FWidgetActiveTimerDelegate::CreateLambda(
									[WeakPile, InitialOffset, Duration, Elapsed](double, float DeltaSeconds)
									{
										UWacomBackpackZonePileWidget* AnimatedPile = WeakPile.Get();
										if (!AnimatedPile)
										{
											return EActiveTimerReturnType::Stop;
										}
										*Elapsed += FMath::Max(0.0f, DeltaSeconds);
										const float Alpha = FMath::Clamp(*Elapsed / Duration, 0.0f, 1.0f);
										AnimatedPile->SetRenderTranslation(
											FMath::Lerp(InitialOffset, FVector2D::ZeroVector,
												FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.0f)));
										return Alpha >= 1.0f
											? EActiveTimerReturnType::Stop
											: EActiveTimerReturnType::Continue;
									}));
						}
					}
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
	const FWacomBackpackWorkspaceCarryState& Carry = InteractionModel->GetCarry();
	const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
		? InteractionStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	if (!Carry.RemainingInstanceIds.IsEmpty() && !bHasDisplayedCarryPointer)
	{
		DisplayedCarryPointer = Carry.PointerPosition;
		bHasDisplayedCarryPointer = true;
	}
	const FVector2D FanPointer = bHasDisplayedCarryPointer
		? DisplayedCarryPointer
		: Carry.PointerPosition;
	const TArray<FWacomBackpackCarriedFanLayout> Fan =
		FWacomBackpackWorkspaceLayoutSolver::BuildCarriedFanLayout(
			Carry.RemainingInstanceIds.Num(),
			Carry.CurrentIndex,
			Carry.DefaultIndex,
			FanPointer,
			Style->FanMaximumAngleDegrees,
			Style->FanCardSpacingPixels,
			Style->CurrentCardLiftPixels);

	for (TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard : BoundCardWidgets)
	{
		UWacomDeckCardWidget* CardWidget = WeakCard.Get();
		if (!CardWidget)
		{
			continue;
		}
		const FGuid InstanceId = CardWidget->GetCardInstanceId();
		const int32 CarryIndex = Carry.RemainingInstanceIds.IndexOfByKey(InstanceId);
		const FBaseCardLayoutTransition* Transition = BaseCardLayoutTransitions.Find(InstanceId);
		const FBaseCardLayout* Base = Transition
			? &Transition->Current
			: BaseCardLayouts.Find(InstanceId);
		if (Base)
		{
			FVector2D Center = Base->Center;
			int32 LayerRank = Base->ZOrder;
			if (CarryIndex == INDEX_NONE && HoveredCardInstanceId == InstanceId)
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
		}
		const bool bCurrent = CarryIndex != INDEX_NONE && CarryIndex == Carry.CurrentIndex;
		const bool bSelected = InteractionModel->IsSelected(InstanceId);
		CardWidget->SetWorkspaceVisualState(bSelected, bCurrent, !CardWidget->IsMoveEnabled());
		CardWidget->ApplyWorkspaceVisualState(
			UWacomBackpackScreenPresenter::BuildWorkspaceCardVisualState(
				Style,
				bSelected,
				bCurrent,
				!CardWidget->IsMoveEnabled()));
		if (Fan.IsValidIndex(CarryIndex))
		{
			ApplyCardLayout(
				*CardWidget,
				Fan[CarryIndex].Transform.CardCenter,
				Style->CardRenderSize,
				Fan[CarryIndex].Transform.AngleDegrees,
				10000 + Fan[CarryIndex].Transform.LayerRank);
		}
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
	if (InteractionModel->IsCarrying() && !bCarryInputSuspended && !bCarryInterpolationActive)
	{
		StartCarryInterpolation();
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
		StartCarryInterpolation();
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
	bHasDisplayedCarryPointer = false;
	CancelHoverExpandTimer();
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().ReleaseAllPointerCapture(0);
	}
	RefreshInteractionPresentation();
}

void UWacomBackpackWorkspaceWidget::StartCarryInterpolation()
{
	if (!InteractionModel || !InteractionModel->IsCarrying() || bCarryInputSuspended)
	{
		return;
	}
	if (!bHasDisplayedCarryPointer)
	{
		DisplayedCarryPointer = InteractionModel->GetCarry().PointerPosition;
		bHasDisplayedCarryPointer = true;
	}
	if (bCarryInterpolationActive)
	{
		return;
	}
	TSharedPtr<SWidget> CachedWidget = GetCachedWidget();
	if (!CachedWidget.IsValid())
	{
		DisplayedCarryPointer = InteractionModel->GetCarry().PointerPosition;
		return;
	}
	bCarryInterpolationActive = true;
	const TWeakObjectPtr<UWacomBackpackWorkspaceWidget> WeakThis(this);
	CachedWidget->RegisterActiveTimer(
		0.0f,
		FWidgetActiveTimerDelegate::CreateLambda(
			[WeakThis](double CurrentTime, float DeltaTime)
			{
				UWacomBackpackWorkspaceWidget* Self = WeakThis.Get();
				if (Self && Self->bCarryInputSuspended)
				{
					Self->bCarryInterpolationActive = false;
					return EActiveTimerReturnType::Stop;
				}
				if (!Self || !Self->InteractionModel || !Self->InteractionModel->IsCarrying())
				{
					if (Self)
					{
						Self->bCarryInterpolationActive = false;
						Self->bHasDisplayedCarryPointer = false;
					}
					return EActiveTimerReturnType::Stop;
				}
				FVector2D Target = Self->InteractionModel->GetCarry().PointerPosition;
				if (FSlateApplication::IsInitialized())
				{
					const FVector2D LatestPointer = Self->GetCachedGeometry().AbsoluteToLocal(
						FSlateApplication::Get().GetCursorPos());
					if (!LatestPointer.Equals(Target, 0.1f))
					{
						Self->InteractionModel->UpdateCarryPointer(LatestPointer);
						Target = LatestPointer;
						Self->OnInteractionChangedNative.Broadcast();
					}
				}
				const UWacomBackpackWorkspaceStyle* Style = Self->InteractionStyle.IsValid()
					? Self->InteractionStyle.Get()
					: GetDefault<UWacomBackpackWorkspaceStyle>();
				const float FollowSeconds = FMath::Max(0.001f, Style->PointerFollowSeconds);
				const float Alpha = 1.0f - FMath::Exp(-DeltaTime / FollowSeconds);
				if (!Self->DisplayedCarryPointer.Equals(Target, 0.1f))
				{
					Self->DisplayedCarryPointer = FMath::Lerp(Self->DisplayedCarryPointer, Target, Alpha);
					if (FVector2D::Distance(Self->DisplayedCarryPointer, Target) <= 0.5f)
					{
						Self->DisplayedCarryPointer = Target;
					}
					Self->RefreshInteractionPresentation();
				}
				return EActiveTimerReturnType::Continue;
			}));
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
	if (!WorkspaceCanvas)
	{
		WorkspaceCanvas = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("WorkspaceCanvas")));
	}
	if (!CardCanvas)
	{
		CardCanvas = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("CardCanvas")));
	}
	if (!WorkspaceCanvas)
	{
		WorkspaceCanvas = CardCanvas;
	}
	if (!CardCanvas)
	{
		CardCanvas = WorkspaceCanvas;
	}
	if (WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("WorkspaceRoot"));
	WidgetTree->RootWidget = Root;
	WorkspaceCanvas = Root;
	CardCanvas = Root;

	SelectionMarquee = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SelectionMarquee"));
	SelectionMarquee->SetBrushColor(FLinearColor(0.2f, 0.72f, 1.0f, 0.2f));
	SelectionMarquee->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* MarqueeSlot = Root->AddChildToCanvas(SelectionMarquee))
	{
		MarqueeSlot->SetPosition(FVector2D::ZeroVector);
		MarqueeSlot->SetSize(FVector2D::ZeroVector);
		MarqueeSlot->SetZOrder(100000);
	}

	EmptyStateText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EmptyStateText"));
	EmptyStateText->SetText(LOCTEXT("EmptyWorkspace", "该区域暂无卡牌"));
	EmptyStateText->SetJustification(ETextJustify::Center);
	if (UCanvasPanelSlot* EmptySlot = Root->AddChildToCanvas(EmptyStateText))
	{
		EmptySlot->SetAnchors(FAnchors(0.5f, 0.5f));
		EmptySlot->SetAlignment(FVector2D(0.5f, 0.5f));
		EmptySlot->SetAutoSize(true);
		EmptySlot->SetZOrder(100001);
	}
}

UCanvasPanel* UWacomBackpackWorkspaceWidget::GetCardCanvas()
{
	EnsureFallbackTree();
	return WorkspaceCanvas ? WorkspaceCanvas.Get() : CardCanvas.Get();
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
		const FGuid InstanceId = DeckCard->GetCardInstanceId();
		FBaseCardLayout Target;
		Target.Center = CardCenter;
		Target.Size = CardSize;
		Target.AngleDegrees = AngleDegrees;
		Target.ZOrder = ZOrder;
		const FBaseCardLayout* Previous = BaseCardLayouts.Find(InstanceId);
		const UWacomBackpackWorkspaceStyle* Style = InteractionStyle.IsValid()
			? InteractionStyle.Get()
			: GetDefault<UWacomBackpackWorkspaceStyle>();
		const bool bChanged = Previous
			&& (!Previous->Center.Equals(Target.Center, 0.5f)
				|| !FMath::IsNearlyEqual(Previous->AngleDegrees, Target.AngleDegrees, 0.1f));
		if (!bSimplifiedMotion && bChanged && Style->PileExpandSeconds > 0.0f)
		{
			const FBaseCardLayoutTransition* ExistingTransition = BaseCardLayoutTransitions.Find(InstanceId);
			const FBaseCardLayout Start = ExistingTransition
				? ExistingTransition->Current
				: *Previous;
			FBaseCardLayoutTransition& Transition = BaseCardLayoutTransitions.FindOrAdd(InstanceId);
			Transition.Current = Start;
			Transition.Target = Target;
			Transition.ElapsedSeconds = 0.0f;
			Transition.DurationSeconds = Style->PileExpandSeconds;
			StartBaseCardLayoutTransitions();
		}
		else
		{
			BaseCardLayoutTransitions.Remove(InstanceId);
		}
		BaseCardLayouts.Add(InstanceId, Target);
	}
	RefreshInteractionPresentation();
}

void UWacomBackpackWorkspaceWidget::PrimeCardBaseLayout(
	UWidget& CardWidget,
	FVector2D CardCenter,
	FVector2D CardSize,
	float AngleDegrees,
	int32 ZOrder)
{
	const UWacomDeckCardWidget* DeckCard = Cast<UWacomDeckCardWidget>(&CardWidget);
	if (!DeckCard || BaseCardLayouts.Contains(DeckCard->GetCardInstanceId()))
	{
		return;
	}
	FBaseCardLayout Base;
	Base.Center = CardCenter;
	Base.Size = CardSize;
	Base.AngleDegrees = AngleDegrees;
	Base.ZOrder = ZOrder;
	BaseCardLayouts.Add(DeckCard->GetCardInstanceId(), Base);
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
						Transition.Current.Center, Transition.Target.Center, Smoothed);
					Transition.Current.Size = Transition.Target.Size;
					Transition.Current.AngleDegrees = FMath::Lerp(
						Transition.Current.AngleDegrees, Transition.Target.AngleDegrees, Smoothed);
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
