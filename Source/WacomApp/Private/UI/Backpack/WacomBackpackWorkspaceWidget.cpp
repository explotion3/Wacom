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
#include "UI/Backpack/WacomDeckCardWidget.h"

#define LOCTEXT_NAMESPACE "WacomBackpackWorkspace"

TSharedRef<SWidget> UWacomBackpackWorkspaceWidget::RebuildWidget()
{
	EnsureFallbackTree();
	SetIsFocusable(true);
	return Super::RebuildWidget();
}

void UWacomBackpackWorkspaceWidget::NativeDestruct()
{
	CancelInteraction();
	for (TWeakObjectPtr<UWacomDeckCardWidget>& CardWidget : BoundCardWidgets)
	{
		if (CardWidget.IsValid())
		{
			CardWidget->SetWorkspaceInputOwned(false);
		}
	}
	BoundCardWidgets.Reset();
	Super::NativeDestruct();
}

void UWacomBackpackWorkspaceWidget::SetInteractionModel(
	TSharedPtr<FWacomBackpackWorkspaceInteractionModel> InModel,
	UWacomBackpackWorkspaceStyle* InStyle)
{
	InteractionModel = MoveTemp(InModel);
	InteractionStyle = InStyle;
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
	BoundCardWidgets.Reset();
	TArray<FWacomBackpackWorkspaceCardHitRecord> HitRecords;
	for (UWacomDeckCardWidget* CardWidget : CardWidgets)
	{
		if (!CardWidget)
		{
			continue;
		}
		CardWidget->SetWorkspaceInputOwned(true);
		CardWidget->OnWorkspacePointerDownNative.BindUObject(this, &UWacomBackpackWorkspaceWidget::HandleCardPointerDown);
		CardWidget->OnWorkspacePointerMoveNative.BindUObject(this, &UWacomBackpackWorkspaceWidget::HandleCardPointerMove);
		CardWidget->OnWorkspacePointerUpNative.BindUObject(this, &UWacomBackpackWorkspaceWidget::HandleCardPointerUp);
		BoundCardWidgets.Add(CardWidget);

		FWacomBackpackWorkspaceCardHitRecord Hit;
		Hit.InstanceId = CardWidget->GetCardInstanceId();
		Hit.bMovable = CardWidget->IsMoveEnabled();
		if (const UCanvasPanelSlot* CardCanvasSlot = Cast<UCanvasPanelSlot>(CardWidget->Slot))
		{
			Hit.CardCenter = CardCanvasSlot->GetPosition() + CardCanvasSlot->GetSize() * 0.5f;
			Hit.LayerRank = CardCanvasSlot->GetZOrder();
		}
		HitRecords.Add(Hit);
	}
	if (InteractionModel)
	{
		InteractionModel->ReconcileCards(FWacomBackpackZoneKey::Make(ActiveZone, ActiveZoneOwnerInstanceId), HitRecords);
	}
	RefreshInteractionPresentation();
}

FReply UWacomBackpackWorkspaceWidget::HandleCardPointerDown(
	UWacomDeckCardWidget* CardWidget,
	const FGeometry& Geometry,
	const FPointerEvent& Event)
{
	if (!InteractionModel || !CardWidget)
	{
		return FReply::Unhandled();
	}
	if (InteractionModel->IsCarrying()
		&& (Event.GetEffectingButton() == EKeys::LeftMouseButton
			|| Event.GetEffectingButton() == EKeys::RightMouseButton))
	{
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
	bPendingCardPress = true;
	PendingCardPressId = CardWidget->GetCardInstanceId();
	PendingPressPosition = ToLocalPointer(Event);
	bPendingControlDown = Event.IsControlDown();
	return FReply::Handled().CaptureMouse(TakeWidget());
}

FReply UWacomBackpackWorkspaceWidget::HandleCardPointerMove(
	UWacomDeckCardWidget* CardWidget,
	const FGeometry& Geometry,
	const FPointerEvent& Event)
{
	if (!InteractionModel)
	{
		return FReply::Unhandled();
	}
	const FVector2D Pointer = ToLocalPointer(Event);
	if (InteractionModel->IsCarrying())
	{
		InteractionModel->UpdateCarryPointer(Pointer);
		StartCarryInterpolation();
		OnInteractionChangedNative.Broadcast();
		return FReply::Handled();
	}
	if (bPendingCardPress && FVector2D::Distance(PendingPressPosition, Pointer) >= 5.0f)
	{
		if (!InteractionModel->IsSelected(PendingCardPressId))
		{
			InteractionModel->ClickCard(PendingCardPressId, false);
		}
		InteractionModel->BeginCarry(PendingCardPressId, Pointer, CurrentStorageRevision);
		DisplayedCarryPointer = Pointer;
		bHasDisplayedCarryPointer = true;
		bPendingCardPress = false;
		RefreshInteractionPresentation();
		OnInteractionChangedNative.Broadcast();
		return FReply::Handled();
	}
	return FReply::Handled();
}

FReply UWacomBackpackWorkspaceWidget::HandleCardPointerUp(
	UWacomDeckCardWidget* CardWidget,
	const FGeometry& Geometry,
	const FPointerEvent& Event)
{
	if (!InteractionModel)
	{
		return FReply::Unhandled();
	}
	if (InteractionModel->IsCarrying())
	{
		BroadcastRelease(Event.GetEffectingButton() == EKeys::RightMouseButton);
		return FReply::Handled();
	}
	if (bPendingCardPress && Event.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		InteractionModel->ClickCard(PendingCardPressId, bPendingControlDown);
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

FReply UWacomBackpackWorkspaceWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InteractionModel && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (InteractionModel->IsCarrying())
		{
			return FReply::Handled().CaptureMouse(TakeWidget()).SetUserFocus(TakeWidget());
		}
		InteractionModel->BeginMarquee(ToLocalPointer(InMouseEvent), InMouseEvent.IsControlDown());
		RefreshInteractionPresentation();
		return FReply::Handled().CaptureMouse(TakeWidget()).SetUserFocus(TakeWidget());
	}
	if (InteractionModel && InteractionModel->IsCarrying()
		&& InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		return FReply::Handled().CaptureMouse(TakeWidget()).SetUserFocus(TakeWidget());
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UWacomBackpackWorkspaceWidget::NativeOnMouseMove(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (!InteractionModel)
	{
		return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
	}
	if (InteractionModel->IsCarrying())
	{
		InteractionModel->UpdateCarryPointer(ToLocalPointer(InMouseEvent));
		StartCarryInterpolation();
		OnInteractionChangedNative.Broadcast();
		return FReply::Handled();
	}
	if (InteractionModel->IsMarqueeActive())
	{
		InteractionModel->UpdateMarquee(ToLocalPointer(InMouseEvent));
		RefreshInteractionPresentation();
		return FReply::Handled();
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UWacomBackpackWorkspaceWidget::NativeOnMouseButtonUp(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (!InteractionModel)
	{
		return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
	}
	if (InteractionModel->IsCarrying())
	{
		if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
			|| InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			BroadcastRelease(InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton);
			return FReply::Handled();
		}
		return FReply::Unhandled();
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
	if (InteractionModel && InteractionModel->IsCarrying())
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
	if (InteractionModel && InKeyEvent.GetKey() == EKeys::Escape)
	{
		CancelInteraction();
		OnInteractionChangedNative.Broadcast();
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
}

void UWacomBackpackWorkspaceWidget::CancelInteraction()
{
	bPendingCardPress = false;
	PendingCardPressId.Invalidate();
	if (InteractionModel)
	{
		InteractionModel->CancelTransientState();
	}
	bHasDisplayedCarryPointer = false;
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().ReleaseAllPointerCapture(0);
	}
	RefreshInteractionPresentation();
}

void UWacomBackpackWorkspaceWidget::StartCarryInterpolation()
{
	if (!InteractionModel || !InteractionModel->IsCarrying())
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
		RefreshInteractionPresentation();
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
				if (!Self || !Self->InteractionModel || !Self->InteractionModel->IsCarrying())
				{
					if (Self)
					{
						Self->bCarryInterpolationActive = false;
						Self->bHasDisplayedCarryPointer = false;
					}
					return EActiveTimerReturnType::Stop;
				}
				const FVector2D Target = Self->InteractionModel->GetCarry().PointerPosition;
				const UWacomBackpackWorkspaceStyle* Style = Self->InteractionStyle.IsValid()
					? Self->InteractionStyle.Get()
					: GetDefault<UWacomBackpackWorkspaceStyle>();
				const float FollowSeconds = FMath::Max(0.001f, Style->PointerFollowSeconds);
				const float Alpha = 1.0f - FMath::Exp(-DeltaTime / FollowSeconds);
				Self->DisplayedCarryPointer = FMath::Lerp(Self->DisplayedCarryPointer, Target, Alpha);
				Self->RefreshInteractionPresentation();
				if (FVector2D::Distance(Self->DisplayedCarryPointer, Target) <= 0.5f)
				{
					Self->DisplayedCarryPointer = Target;
					Self->bCarryInterpolationActive = false;
					Self->RefreshInteractionPresentation();
					return EActiveTimerReturnType::Stop;
				}
				return EActiveTimerReturnType::Continue;
			}));
}

void UWacomBackpackWorkspaceWidget::EnsureFallbackTree()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
	}
	if (!CardCanvas)
	{
		CardCanvas = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("CardCanvas")));
	}
	if (WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("WorkspaceRoot"));
	WidgetTree->RootWidget = Root;

	CardCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CardCanvas"));
	if (UCanvasPanelSlot* CardCanvasSlot = Root->AddChildToCanvas(CardCanvas))
	{
		CardCanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		CardCanvasSlot->SetOffsets(FMargin(0.0f));
	}

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
	return CardCanvas;
}

void UWacomBackpackWorkspaceWidget::SetActiveZone(EZoneKind Zone, FGuid OwnerInstanceId)
{
	ActiveZone = Zone;
	ActiveZoneOwnerInstanceId = Zone == EZoneKind::SpecialZone ? OwnerInstanceId : FGuid();
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
	CanvasSlot->SetPosition(CardCenter - CardSize * 0.5f);
	CanvasSlot->SetSize(CardSize);
	CanvasSlot->SetZOrder(ZOrder);
	CardWidget.SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	CardWidget.SetRenderTransformAngle(AngleDegrees);
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
	View.ActiveZone = ActiveZone;
	View.ActiveZoneOwnerInstanceId = ActiveZoneOwnerInstanceId;
	View.ManualLayoutCount = ManualLayoutCount;
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
