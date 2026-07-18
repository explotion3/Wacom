// Copyright Wacom. All Rights Reserved.

#include "BackpackScreenTestAccess.h"

#if WITH_AUTOMATION_TESTS

#include "RunSession.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomBackpackDeleteConfirmWidget.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"
#include "UI/Backpack/WacomBackpackZonePileWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceSceneBuilder.h"
#include "../../../WacomApp/Private/UI/Backpack/WacomBackpackCommandFlow.h"
#include "../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceMotionCoordinator.h"
#include "../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceRuntime.h"
#include "../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceTypes.h"

UWacomBackpackScreen* FWacomBackpackScreenTestAccess::Create(UObject* Outer, URunSession* RunSession)
{
	return CreateWithClass(Outer, RunSession, UWacomBackpackScreen::StaticClass());
}

UWacomBackpackScreen* FWacomBackpackScreenTestAccess::CreateWithClass(
	UObject* Outer,
	URunSession* RunSession,
	UClass* ScreenClass)
{
	UWacomBackpackScreen* Screen = ScreenClass && ScreenClass->IsChildOf(UWacomBackpackScreen::StaticClass())
		? NewObject<UWacomBackpackScreen>(Outer, ScreenClass)
		: nullptr;
	if (!Screen)
	{
		return nullptr;
	}
	SetRunSession(*Screen, RunSession);
	Screen->TakeWidget();
	Refresh(*Screen);
	if (Screen->WorkspaceWidget)
	{
		Screen->WorkspaceWidget->SetSimplifiedMotion(true);
	}
	return Screen;
}

void FWacomBackpackScreenTestAccess::Refresh(UWacomBackpackScreen& Screen)
{
	Screen.RebuildAllForTest();
}

void FWacomBackpackScreenTestAccess::SetRunSession(UWacomBackpackScreen& Screen, URunSession* RunSession)
{
	Screen.SetRunSessionForTest(RunSession);
}

FWacomBackpackScreenAutomationTestView FWacomBackpackScreenTestAccess::View(const UWacomBackpackScreen& Screen)
{
	return Screen.GetAutomationTestViewForTest();
}

FText FWacomBackpackScreenTestAccess::BuildMoveZoneNameText(EZoneKind Zone)
{
	return FWacomBackpackCommandFlow::BuildMoveZoneNameText(Zone);
}

FText FWacomBackpackScreenTestAccess::BuildMoveFailureToastText(FName DisabledReason)
{
	return FWacomBackpackCommandFlow::BuildMoveFailureToastText(DisabledReason);
}

FText FWacomBackpackScreenTestAccess::BuildDeleteFailureToastText(FName DisabledReason)
{
	return FWacomBackpackCommandFlow::BuildDeleteFailureToastText(DisabledReason);
}

UWacomDeckCardWidget* FWacomBackpackScreenTestAccess::BattleDeckCard(const UWacomBackpackScreen& Screen, int32 Index)
{
	for (UWacomDeckCardWidget* CardWidget : Screen.ActiveWorkspaceCardWidgets)
	{
		if (CardWidget && CardWidget->GetWorkspaceDisplayZone() == EZoneKind::BattleDeck && Index-- == 0)
		{
			return CardWidget;
		}
	}
	return nullptr;
}

UWacomDeckCardWidget* FWacomBackpackScreenTestAccess::FluxContentCard(const UWacomBackpackScreen& Screen, int32 Index)
{
	for (UWacomDeckCardWidget* CardWidget : Screen.ActiveWorkspaceCardWidgets)
	{
		if (CardWidget && CardWidget->GetWorkspaceDisplayZone() == EZoneKind::Backpack && Index-- == 0)
		{
			return CardWidget;
		}
	}
	return nullptr;
}

UWacomDeckCardWidget* FWacomBackpackScreenTestAccess::BurdenCard(const UWacomBackpackScreen& Screen, int32 Index)
{
	for (UWacomDeckCardWidget* CardWidget : Screen.ActiveWorkspaceCardWidgets)
	{
		if (CardWidget && CardWidget->GetWorkspaceDisplayZone() == EZoneKind::BurdenZone && Index-- == 0)
		{
			return CardWidget;
		}
	}
	return nullptr;
}

UWacomDeckCardWidget* FWacomBackpackScreenTestAccess::SpecialOwnerCard(
	const UWacomBackpackScreen& Screen,
	FGuid OwnerInstanceId)
{
	for (UWacomDeckCardWidget* CardWidget : Screen.ActiveWorkspaceCardWidgets)
	{
		if (CardWidget
			&& CardWidget->GetWorkspaceDisplayZone() == EZoneKind::SpecialZone
			&& CardWidget->GetWorkspaceDisplayOwnerInstanceId() == OwnerInstanceId
			&& CardWidget->GetBackpackListReuseRole() == EWacomBackpackDeckCardListReuseRole::SpecialOwner)
		{
			return CardWidget;
		}
	}
	return nullptr;
}

UWacomDeckCardWidget* FWacomBackpackScreenTestAccess::SpecialContentCard(
	const UWacomBackpackScreen& Screen,
	FGuid OwnerInstanceId,
	int32 Index)
{
	for (UWacomDeckCardWidget* CardWidget : Screen.ActiveWorkspaceCardWidgets)
	{
		if (CardWidget
			&& CardWidget->GetWorkspaceDisplayZone() == EZoneKind::SpecialZone
			&& CardWidget->GetWorkspaceDisplayOwnerInstanceId() == OwnerInstanceId
			&& CardWidget->GetBackpackListReuseRole() == EWacomBackpackDeckCardListReuseRole::SpecialContent
			&& Index-- == 0)
		{
			return CardWidget;
		}
	}
	return nullptr;
}

int32 FWacomBackpackScreenTestAccess::RefreshApplyCount(const UWacomBackpackScreen& Screen)
{
	return View(Screen).ListRefreshApplyCount;
}

int32 FWacomBackpackScreenTestAccess::RefreshSkipCount(const UWacomBackpackScreen& Screen)
{
	return View(Screen).ListRefreshSkipCount;
}

int32 FWacomBackpackScreenTestAccess::SnapshotBuildCount(const UWacomBackpackScreen& Screen)
{
	return View(Screen).SnapshotBuildCount;
}

int32 FWacomBackpackScreenTestAccess::SnapshotRevisionSkipCount(const UWacomBackpackScreen& Screen)
{
	return View(Screen).SnapshotRevisionSkipCount;
}

int32 FWacomBackpackScreenTestAccess::WorkspacePileCount(const UWacomBackpackScreen& Screen)
{
	return View(Screen).WorkspacePileCount;
}

int32 FWacomBackpackScreenTestAccess::WorkspaceCardCount(const UWacomBackpackScreen& Screen)
{
	return View(Screen).WorkspaceCardCount;
}

#if WITH_EDITOR
bool FWacomBackpackScreenTestAccess::UsesEmptyPIEValidationSnapshot(
	const UWacomBackpackScreen& Screen)
{
	return Screen.bPIEValidationEmptySnapshot;
}

bool FWacomBackpackScreenTestAccess::UsesNativeFallbackVisualClasses(
	const UWacomBackpackScreen& Screen)
{
	return Screen.CardWidgetClass == UWacomDeckCardWidget::StaticClass()
		&& Screen.CardDetailPanelClass == UWacomCardDetailPanel::StaticClass()
		&& Screen.WorkspaceWidgetClass == UWacomBackpackWorkspaceWidget::StaticClass()
		&& Screen.DeleteConfirmWidgetClass == UWacomBackpackDeleteConfirmWidget::StaticClass();
}
#endif

bool FWacomBackpackScreenTestAccess::WorkspaceChildFillsHost(const UWacomBackpackScreen& Screen)
{
	if (!Screen.WorkspaceWidget || Screen.WorkspaceWidget->GetParent() != Screen.WorkspaceHost)
	{
		return false;
	}
	if (const UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(Screen.WorkspaceWidget->Slot))
	{
		return OverlaySlot->GetHorizontalAlignment() == HAlign_Fill
			&& OverlaySlot->GetVerticalAlignment() == VAlign_Fill;
	}
	if (const UVerticalBoxSlot* VerticalSlot = Cast<UVerticalBoxSlot>(Screen.WorkspaceWidget->Slot))
	{
		return VerticalSlot->GetHorizontalAlignment() == HAlign_Fill
			&& VerticalSlot->GetSize().SizeRule == ESlateSizeRule::Fill;
	}
	return false;
}

bool FWacomBackpackScreenTestAccess::WorkspaceOwnsPointerInput(const UWacomBackpackScreen& Screen)
{
	return Screen.WorkspaceWidget
		&& Screen.WorkspaceWidget->GetVisibility() == ESlateVisibility::Visible;
}

TArray<FVector2D> FWacomBackpackScreenTestAccess::WorkspaceCardPositions(const UWacomBackpackScreen& Screen)
{
	TArray<FVector2D> Positions;
	Positions.Reserve(Screen.ActiveWorkspaceCardWidgets.Num());
	for (const UWacomDeckCardWidget* CardWidget : Screen.ActiveWorkspaceCardWidgets)
	{
		const UCanvasPanelSlot* CanvasSlot = CardWidget
			? Cast<UCanvasPanelSlot>(CardWidget->Slot)
			: nullptr;
		Positions.Add(CanvasSlot ? CanvasSlot->GetPosition() : FVector2D(-FLT_MAX, -FLT_MAX));
	}
	return Positions;
}

TArray<float> FWacomBackpackScreenTestAccess::WorkspaceCardRenderOpacities(
	const UWacomBackpackScreen& Screen)
{
	TArray<float> Opacities;
	Opacities.Reserve(Screen.ActiveWorkspaceCardWidgets.Num());
	for (const UWacomDeckCardWidget* CardWidget : Screen.ActiveWorkspaceCardWidgets)
	{
		Opacities.Add(CardWidget ? CardWidget->GetRenderOpacity() : -1.0f);
	}
	return Opacities;
}

bool FWacomBackpackScreenTestAccess::ApplyStableWorkspaceGeometry(
	UWacomBackpackScreen& Screen,
	FVector2D LayoutSize)
{
	return Screen.WorkspaceWidget
		&& Screen.WorkspaceWidget->AcceptStableLayoutGeometry(LayoutSize);
}

void FWacomBackpackScreenTestAccess::FlushDeferredWorkspaceCardFaceRender(
	UWacomBackpackScreen& Screen)
{
	if (Screen.WorkspaceWidget)
	{
		Screen.WorkspaceWidget->FlushDeferredCardFaceRender();
	}
}

void FWacomBackpackScreenTestAccess::ApplyWorkspaceLayerTransition(
	UWacomBackpackScreen& Screen,
	bool bTransitioning)
{
	Screen.ApplyOwningLayerTransitionState(bTransitioning);
}

bool FWacomBackpackScreenTestAccess::MarqueeCrossingCardPreservesMouseCapture(
	UWacomBackpackScreen& Screen,
	int32 CardIndex)
{
	if (!Screen.WorkspaceWidget || !Screen.WorkspaceInteractionModel
		|| !Screen.ActiveWorkspaceCardWidgets.IsValidIndex(CardIndex)
		|| !Screen.ActiveWorkspaceCardWidgets[CardIndex])
	{
		return false;
	}

	Screen.WorkspaceInteractionModel->BeginMarquee(FVector2D(10.0f, 10.0f), false);
	const TSharedRef<SWidget> WorkspaceSlateWidget = Screen.WorkspaceWidget->TakeWidget();
	TSet<FKey> PressedButtons{ EKeys::LeftMouseButton };
	const FPointerEvent PointerMove(
		0,
		FVector2D(150.0f, 150.0f),
		FVector2D(10.0f, 10.0f),
		PressedButtons,
		EKeys::LeftMouseButton,
		0.0f,
		FModifierKeysState());
	const FReply Reply = Screen.WorkspaceWidget->HandleCardPointerMove(
		Screen.ActiveWorkspaceCardWidgets[CardIndex],
		FGeometry(),
		PointerMove);
	const bool bPreserved = Screen.WorkspaceInteractionModel->IsMarqueeActive()
		&& Screen.WorkspaceInteractionModel->IsMouseCaptured()
		&& !Reply.ShouldReleaseMouse()
		&& Reply.GetMouseCaptor().IsValid();
	Screen.WorkspaceWidget->CancelInteraction();
	return bPreserved;
}

bool FWacomBackpackScreenTestAccess::MarqueeCompletesWhenReleasedOverCard(
	UWacomBackpackScreen& Screen,
	int32 CardIndex)
{
	if (!Screen.WorkspaceWidget || !Screen.WorkspaceInteractionModel
		|| !Screen.ActiveWorkspaceCardWidgets.IsValidIndex(CardIndex)
		|| !Screen.ActiveWorkspaceCardWidgets[CardIndex])
	{
		return false;
	}

	Screen.WorkspaceInteractionModel->BeginMarquee(FVector2D(10.0f, 10.0f), false);
	TSet<FKey> PressedButtons;
	const FPointerEvent PointerUp(
		0,
		FVector2D(150.0f, 150.0f),
		FVector2D(140.0f, 140.0f),
		PressedButtons,
		EKeys::LeftMouseButton,
		0.0f,
		FModifierKeysState());
	const FReply Reply = Screen.WorkspaceWidget->HandleCardPointerUp(
		Screen.ActiveWorkspaceCardWidgets[CardIndex],
		FGeometry(),
		PointerUp);
	const bool bCompleted = !Screen.WorkspaceInteractionModel->IsMarqueeActive()
		&& !Screen.WorkspaceInteractionModel->IsMouseCaptured()
		&& Reply.ShouldReleaseMouse();
	Screen.WorkspaceWidget->CancelInteraction();
	return bCompleted;
}

FWacomBackpackPickupPointerSequenceProbe
FWacomBackpackScreenTestAccess::ProbeCardPickupPointerSequence(
	UWacomBackpackWorkspaceWidget& Workspace,
	UWacomDeckCardWidget& CardWidget,
	bool bPreselectCard)
{
	FWacomBackpackPickupPointerSequenceProbe Probe;
	if (!Workspace.InteractionModel || !CardWidget.GetCardInstanceId().IsValid())
	{
		return Probe;
	}

	Probe.bCardMovable = CardWidget.IsMoveEnabled();
	if (Workspace.InteractionModel->IsSelected(CardWidget.GetCardInstanceId()) != bPreselectCard)
	{
		if (bPreselectCard)
		{
			Workspace.InteractionModel->ClickCard(CardWidget.GetCardInstanceId(), false);
		}
		else
		{
			Workspace.InteractionModel->ClickBlank();
		}
	}
	Probe.bSelectedBeforePointerDown =
		Workspace.InteractionModel->IsSelected(CardWidget.GetCardInstanceId());
	TSet<FKey> PressedButtons{ EKeys::LeftMouseButton };
	const FPointerEvent PointerDown(
		0,
		FVector2D(240.0f, 180.0f),
		FVector2D(240.0f, 180.0f),
		PressedButtons,
		EKeys::LeftMouseButton,
		0.0f,
		FModifierKeysState());
	Probe.bPointerEventIsLeftMouseButton =
		PointerDown.GetEffectingButton() == EKeys::LeftMouseButton;
	Probe.bPointerEventControlDown = PointerDown.IsControlDown();
	Probe.bCarryingBeforePointerDown = Workspace.InteractionModel->IsCarrying();
	const FReply PointerDownReply =
		Workspace.HandleCardPointerDown(&CardWidget, FGeometry(), PointerDown);
	Probe.bPointerDownHandled = PointerDownReply.IsEventHandled();
	Probe.bCarryStartedOnPointerDown = Workspace.InteractionModel->IsCarrying();

	PressedButtons.Reset();
	const FPointerEvent PointerUp(
		0,
		FVector2D(240.0f, 180.0f),
		FVector2D(240.0f, 180.0f),
		PressedButtons,
		EKeys::LeftMouseButton,
		0.0f,
		FModifierKeysState());
	Workspace.HandleCardPointerUp(&CardWidget, FGeometry(), PointerUp);
	Probe.bPickupReleaseKeptCarry = Workspace.InteractionModel->IsCarrying();
	Probe.bInitialReleaseGuardCleared = Workspace.InteractionModel->IsCarrying()
		&& !Workspace.InteractionModel->GetCarry().bInitialReleaseGuardArmed;

	const FWacomBackpackWorkspaceReleaseIntent LeftIntent =
		Workspace.InteractionModel->BuildReleaseIntent(false);
	Probe.NextLeftReleaseCount = LeftIntent.InstanceIds.Num();
	const FWacomBackpackWorkspaceReleaseIntent RightIntent =
		Workspace.InteractionModel->BuildReleaseIntent(true);
	Probe.NextRightReleaseCount = RightIntent.InstanceIds.Num();
	Workspace.CancelInteraction();

	int32 BroadcastReleaseCount = 0;
	Workspace.OnReleaseIntentNative.AddLambda(
		[&BroadcastReleaseCount](const FWacomBackpackWorkspaceReleaseIntent& Intent)
		{
			BroadcastReleaseCount += Intent.InstanceIds.Num();
		});
	Workspace.InteractionModel->ClickCard(CardWidget.GetCardInstanceId(), false);
	Workspace.HandleCardPointerDown(&CardWidget, FGeometry(), PointerDown);
	Workspace.HandleCardPointerDown(&CardWidget, FGeometry(), PointerDown);
	Workspace.HandleCardPointerUp(&CardWidget, FGeometry(), PointerUp);
	Probe.FirstLeftReleaseAfterMissedPickupUpCount = BroadcastReleaseCount;
	Workspace.CancelInteraction();

	BroadcastReleaseCount = 0;
	Workspace.InteractionModel->ClickCard(CardWidget.GetCardInstanceId(), false);
	Workspace.HandleCardPointerDown(&CardWidget, FGeometry(), PointerDown);
	TSet<FKey> RightPressedButtons{ EKeys::RightMouseButton };
	const FPointerEvent RightPointerDown(
		0,
		FVector2D(240.0f, 180.0f),
		FVector2D(240.0f, 180.0f),
		RightPressedButtons,
		EKeys::RightMouseButton,
		0.0f,
		FModifierKeysState());
	const FPointerEvent RightPointerUp(
		0,
		FVector2D(240.0f, 180.0f),
		FVector2D(240.0f, 180.0f),
		TSet<FKey>(),
		EKeys::RightMouseButton,
		0.0f,
		FModifierKeysState());
	Workspace.HandleCardPointerDown(&CardWidget, FGeometry(), RightPointerDown);
	Workspace.HandleCardPointerUp(&CardWidget, FGeometry(), RightPointerUp);
	Probe.FirstRightReleaseAfterMissedPickupUpCount = BroadcastReleaseCount;
	Workspace.OnReleaseIntentNative.Clear();
	Workspace.CancelInteraction();
	return Probe;
}

void FWacomBackpackScreenTestAccess::FlushWorkspaceCarryPointer(
	UWacomBackpackWorkspaceWidget& Workspace)
{
	Workspace.FlushQueuedCarryPointer();
	Workspace.ApplyCarryVisualAnchor(1.0f / 60.0f);
	if (Workspace.Runtime)
	{
		const UWacomBackpackWorkspaceStyle* Style = Workspace.InteractionStyle.IsValid()
			? Workspace.InteractionStyle.Get()
			: GetDefault<UWacomBackpackWorkspaceStyle>();
		Workspace.GetRuntime().Motion.Tick(
			1.0f / 60.0f,
			Workspace.GetCachedGeometry(),
			*Style,
			Workspace.bSimplifiedMotion);
	}
	Workspace.FinalizeCompletedSettlements();
}

void FWacomBackpackScreenTestAccess::SendWorkspaceCarryPointerEvents(
	UWacomBackpackWorkspaceWidget& Workspace,
	UWacomDeckCardWidget& CardWidget,
	TConstArrayView<FVector2D> PointerLocals)
{
	FVector2D Previous = Workspace.CarryAnchorLocal;
	for (const FVector2D Pointer : PointerLocals)
	{
		const FVector2D PointerAbsolute =
			Workspace.GetCachedGeometry().LocalToAbsolute(Pointer);
		const FVector2D PreviousAbsolute =
			Workspace.GetCachedGeometry().LocalToAbsolute(Previous);
		const FPointerEvent PointerMove(
			0,
			PointerAbsolute,
			PreviousAbsolute,
			TSet<FKey>{ EKeys::LeftMouseButton },
			EKeys::LeftMouseButton,
			0.0f,
			FModifierKeysState());
		Workspace.HandleCardPointerMove(&CardWidget, FGeometry(), PointerMove);
		Previous = Pointer;
	}
}

bool FWacomBackpackScreenTestAccess::StepWorkspaceCarryCurrentByWheel(
	UWacomBackpackWorkspaceWidget& Workspace,
	float WheelDelta)
{
	const FPointerEvent WheelEvent(
		0,
		FVector2D::ZeroVector,
		FVector2D::ZeroVector,
		TSet<FKey>(),
		EKeys::Invalid,
		WheelDelta,
		FModifierKeysState());
	return Workspace.NativeOnMouseWheel(FGeometry(), WheelEvent).IsEventHandled();
}

void FWacomBackpackScreenTestAccess::TickWorkspaceCardMotion(
	UWacomBackpackWorkspaceWidget& Workspace,
	float DeltaSeconds)
{
	const UWacomBackpackWorkspaceStyle* Style = Workspace.InteractionStyle.IsValid()
		? Workspace.InteractionStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	float RemainingSeconds = FMath::Max(0.0f, DeltaSeconds);
	while (RemainingSeconds > UE_SMALL_NUMBER)
	{
		const float StepSeconds = FMath::Min(RemainingSeconds, 1.0f / 60.0f);
		Workspace.GetRuntime().Motion.Tick(
			StepSeconds,
			Workspace.GetCachedGeometry(),
			*Style,
			Workspace.bSimplifiedMotion);
		RemainingSeconds -= StepSeconds;
	}
	Workspace.FinalizeCompletedSettlements();
}

void FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointer(
	UWacomBackpackWorkspaceWidget& Workspace,
	FVector2D PointerLocal)
{
	Workspace.UpdateExpandedPileFocus(PointerLocal);
}

bool FWacomBackpackScreenTestAccess::PressExpandedPileVisualCard(
	UWacomBackpackWorkspaceWidget& Workspace,
	FVector2D PointerLocal)
{
	TSet<FKey> PressedButtons{ EKeys::LeftMouseButton };
	const FPointerEvent PointerDown(
		0,
		FVector2D::ZeroVector,
		FVector2D::ZeroVector,
		PressedButtons,
		EKeys::LeftMouseButton,
		0.0f,
		FModifierKeysState());
	return Workspace.TryHandleExpandedPileVisualPointerDown(
		PointerLocal, PointerDown).IsEventHandled();
}

bool FWacomBackpackScreenTestAccess::ResolveWorkspaceCardDetailAnchorRect(
	UWacomBackpackWorkspaceWidget& Workspace,
	UWacomDeckCardWidget& Card,
	FSlateRect& OutWorkspaceLocalRect)
{
	return Workspace.ResolveCardDetailAnchorRect(Card, OutWorkspaceLocalRect);
}

void FWacomBackpackScreenTestAccess::TickWorkspaceBrowseExit(
	UWacomBackpackWorkspaceWidget& Workspace,
	float DeltaSeconds)
{
	Workspace.TickExpandedPileFocusExit(DeltaSeconds);
}

bool FWacomBackpackScreenTestAccess::MarqueeWorkspacePileContents(
	UWacomBackpackWorkspaceWidget& Workspace,
	EZoneKind Zone,
	FGuid OwnerInstanceId)
{
	UWacomBackpackZonePileWidget* TargetPile = nullptr;
	for (const TWeakObjectPtr<UWacomBackpackZonePileWidget>& WeakPile :
		Workspace.GetRegisteredPileWidgets())
	{
		UWacomBackpackZonePileWidget* Pile = WeakPile.Get();
		if (Pile && Pile->GetPileView().HasSameIdentity(Zone, OwnerInstanceId))
		{
			TargetPile = Pile;
			break;
		}
	}
	if (!TargetPile || !Workspace.InteractionModel)
	{
		return false;
	}

	const FSlateRect Frame = TargetPile->GetResolvedFrameRect();
	const FSlateRect Header = TargetPile->GetResolvedHeaderRect();
	const FVector2D Start(
		Frame.Left + 4.0f,
		FMath::Min(Frame.Bottom - 4.0f, Header.Bottom + 4.0f));
	const FVector2D End(Frame.Right - 4.0f, Frame.Bottom - 4.0f);
	// Drive the same local-space state transition used by the runtime pointer
	// handler without constructing an FReply for this unarranged unit fixture.
	Workspace.BeginPendingPilePress(*TargetPile, Start, false);
	const bool bBeganMarquee = Workspace.TryBeginMarqueeFromPendingPilePress(End);
	Workspace.InteractionModel->UpdateMarquee(End);
	Workspace.InteractionModel->CompleteMarquee();
	Workspace.UpdateSelectionVisualFreezeLifetime();
	return bBeganMarquee && !Workspace.InteractionModel->IsMarqueeActive();
}

void FWacomBackpackScreenTestAccess::ClearWorkspaceSelection(
	UWacomBackpackWorkspaceWidget& Workspace)
{
	if (!Workspace.InteractionModel)
	{
		return;
	}
	Workspace.InteractionModel->ClickBlank();
	Workspace.UpdateSelectionVisualFreezeLifetime();
	Workspace.RefreshInteractionPresentation();
}

bool FWacomBackpackScreenTestAccess::CommitWorkspacePileMoveWithSynchronousTargetReconcile(
	UWacomBackpackWorkspaceWidget& Workspace,
	UWacomDeckCardWidget& CardWidget,
	EZoneKind Zone,
	FVector2D HeaderStart,
	FVector2D PointerEnd,
	FVector2D TargetCardCenter)
{
	if (!Workspace.InteractionModel)
	{
		return false;
	}
	const FWacomBackpackZoneKey Key = FWacomBackpackZoneKey::Make(Zone);
	if (!Workspace.InteractionModel->BeginPileMove(Key, HeaderStart, HeaderStart))
	{
		return false;
	}
	Workspace.PendingPileStartPosition = HeaderStart;
	Workspace.InteractionModel->UpdatePileMove(PointerEnd);
	Workspace.ApplyActivePileMove();
	bool bReconciled = false;
	const FDelegateHandle Handle = Workspace.OnPileMoveCommittedNative.AddLambda(
		[&Workspace, &CardWidget, TargetCardCenter, &bReconciled](EZoneKind, FGuid, FVector2D)
		{
			Workspace.ApplyCardBaseLayout(
				CardWidget,
				TargetCardCenter,
				FVector2D(220.0f, 320.0f),
				0.0f,
				4000);
			bReconciled = true;
		});
	const FVector2D PointerEndAbsolute =
		Workspace.GetCachedGeometry().LocalToAbsolute(PointerEnd);
	const FPointerEvent PointerUp(
		0,
		PointerEndAbsolute,
		PointerEndAbsolute,
		TSet<FKey>(),
		EKeys::LeftMouseButton,
		0.0f,
		FModifierKeysState());
	Workspace.NativeOnMouseButtonUp(FGeometry(), PointerUp);
	Workspace.OnPileMoveCommittedNative.Remove(Handle);
	return bReconciled;
}

FWacomBackpackPileMoveCancelProbe FWacomBackpackScreenTestAccess::CancelWorkspacePileMove(
	UWacomBackpackWorkspaceWidget& Workspace,
	EZoneKind Zone,
	FGuid OwnerInstanceId,
	FVector2D HeaderStart,
	FVector2D PointerEnd)
{
	FWacomBackpackPileMoveCancelProbe Probe;
	UWacomBackpackZonePileWidget* TargetPile = nullptr;
	for (const TWeakObjectPtr<UWacomBackpackZonePileWidget>& WeakPile :
		Workspace.GetRegisteredPileWidgets())
	{
		UWacomBackpackZonePileWidget* Pile = WeakPile.Get();
		if (Pile && Pile->GetPileView().HasSameIdentity(Zone, OwnerInstanceId))
		{
			TargetPile = Pile;
			break;
		}
	}
	UCanvasPanelSlot* PileSlot = TargetPile
		? Cast<UCanvasPanelSlot>(TargetPile->Slot)
		: nullptr;
	if (!TargetPile || !PileSlot)
	{
		return Probe;
	}

	Probe.PilePositionBefore = PileSlot->GetPosition();
	Probe.PileZOrderBefore = PileSlot->GetZOrder();
	Workspace.BeginPendingPilePress(*TargetPile, HeaderStart, false, true);
	Probe.bBeganMove = Workspace.TryBeginPileMove(PointerEnd);
	Probe.PilePositionWhileMoving = PileSlot->GetPosition();
	Probe.PileZOrderWhileMoving = PileSlot->GetZOrder();
	Workspace.CancelInteraction();
	Probe.PilePositionAfterCancel = PileSlot->GetPosition();
	Probe.PileZOrderAfterCancel = PileSlot->GetZOrder();
	return Probe;
}

void FWacomBackpackScreenTestAccess::ReconcileWorkspacePilesForTest(
	UWacomBackpackWorkspaceWidget& Workspace,
	TConstArrayView<FWacomBackpackZonePileView> Views,
	TConstArrayView<FSlateRect> Frames,
	TConstArrayView<FSlateRect> Headers,
	TConstArrayView<int32> LayerRanks)
{
	UCanvasPanel* Canvas = Workspace.GetPileCanvas();
	if (!Canvas)
	{
		return;
	}
	TArray<FWacomBackpackWorkspaceScenePileEntry> Entries;
	Entries.Reserve(Views.Num());
	for (int32 Index = 0; Index < Views.Num(); ++Index)
	{
		FWacomBackpackWorkspaceScenePileEntry& Entry = Entries.AddDefaulted_GetRef();
		Entry.Zone = FWacomBackpackZoneKey::Make(
			Views[Index].Zone, Views[Index].OwnerInstanceId);
		Entry.View = Views[Index];
		Entry.FrameRect = Frames.IsValidIndex(Index)
			? Frames[Index]
			: FSlateRect(0.0f, 0.0f, 260.0f, 380.0f);
		Entry.HeaderRect = Headers.IsValidIndex(Index)
			? Headers[Index]
			: FSlateRect(
				Entry.FrameRect.Left,
				Entry.FrameRect.Top,
				Entry.FrameRect.Left + 260.0f,
				Entry.FrameRect.Top + 48.0f);
		Entry.LayerRank = LayerRanks.IsValidIndex(Index) ? LayerRanks[Index] : Index;
	}
	Workspace.GetRuntime().Visuals.ReconcilePiles(
		Workspace,
		*Canvas,
		Workspace.PileWidgetClass,
		Entries,
		[&Workspace](UWacomBackpackZonePileWidget& Pile)
		{
			Pile.OnPilePointerDownNative.BindUObject(
				&Workspace,
				&UWacomBackpackWorkspaceWidget::HandlePilePointerDown);
		});
	Workspace.PileCount = Workspace.GetRuntime().Visuals.GetPileWidgets().Num();
}

void FWacomBackpackScreenTestAccess::ForgetWorkspacePileRegistry(
	UWacomBackpackWorkspaceWidget& Workspace)
{
	Workspace.GetRuntime().Visuals.ResetPiles(false);
}

bool FWacomBackpackScreenTestAccess::CommitWorkspaceReleaseBeforeTargetReconcile(
	UWacomBackpackWorkspaceWidget& Workspace)
{
	const TSharedPtr<FWacomBackpackWorkspaceInteractionModel> Model = Workspace.InteractionModel;
	if (!Model || !Model->IsCarrying())
	{
		return false;
	}

	bool bCommitted = false;
	const FDelegateHandle Handle = Workspace.OnReleaseIntentNative.AddLambda(
		[Model, &bCommitted](const FWacomBackpackWorkspaceReleaseIntent& Intent)
		{
			Model->CommitReleasedCards(Intent.InstanceIds);
			bCommitted = !Intent.InstanceIds.IsEmpty();
		});
	Model->NotifyReleaseGestureStarted();
	Workspace.BroadcastRelease(false);
	Workspace.OnReleaseIntentNative.Remove(Handle);
	return bCommitted;
}

EZoneKind FWacomBackpackScreenTestAccess::ActiveWorkspaceZone(const UWacomBackpackScreen& Screen)
{
	return View(Screen).ActiveWorkspaceZone;
}

FGuid FWacomBackpackScreenTestAccess::ActiveWorkspaceOwnerInstanceId(const UWacomBackpackScreen& Screen)
{
	return View(Screen).ActiveWorkspaceOwnerInstanceId;
}

void FWacomBackpackScreenTestAccess::ActivateZone(
	UWacomBackpackScreen& Screen,
	EZoneKind Zone,
	FGuid OwnerInstanceId)
{
	if (Zone == EZoneKind::Backpack)
	{
		Screen.HandleCollapseExpandedPileRequested();
	}
	else
	{
		Screen.HandlePileExpansionRequested(Zone, OwnerInstanceId, false);
	}
}

bool FWacomBackpackScreenTestAccess::BeginWorkspaceCarry(UWacomBackpackScreen& Screen, int32 CardIndex)
{
	if (!Screen.WorkspaceInteractionModel || !Screen.ActiveWorkspaceCardWidgets.IsValidIndex(CardIndex)
		|| !Screen.ActiveWorkspaceCardWidgets[CardIndex])
	{
		return false;
	}
	Screen.WorkspaceInteractionModel->SelectAllMovable();
	const bool bStarted = Screen.WorkspaceInteractionModel->BeginCarry(
		Screen.ActiveWorkspaceCardWidgets[CardIndex]->GetCardInstanceId(),
		FVector2D(500.0f, 350.0f),
		Screen.GetRunSession() ? Screen.GetRunSession()->GetBackpackStorageSnapshotRevision() : 0);
	if (Screen.WorkspaceWidget)
	{
		Screen.WorkspaceWidget->RefreshInteractionPresentation();
	}
	return bStarted;
}

bool FWacomBackpackScreenTestAccess::BeginWorkspaceCarryForIds(
	UWacomBackpackScreen& Screen,
	TConstArrayView<FGuid> InstanceIds)
{
	if (!Screen.WorkspaceInteractionModel || InstanceIds.IsEmpty())
	{
		return false;
	}
	Screen.WorkspaceInteractionModel->ClickCard(InstanceIds[0], false);
	for (int32 Index = 1; Index < InstanceIds.Num(); ++Index)
	{
		Screen.WorkspaceInteractionModel->ClickCard(InstanceIds[Index], true);
	}
	const bool bStarted = Screen.WorkspaceInteractionModel->BeginCarry(
		InstanceIds[0],
		FVector2D(500.0f, 350.0f),
		Screen.GetRunSession() ? Screen.GetRunSession()->GetBackpackStorageSnapshotRevision() : 0);
	if (Screen.WorkspaceWidget) Screen.WorkspaceWidget->RefreshInteractionPresentation();
	return bStarted;
}

bool FWacomBackpackScreenTestAccess::BeginWorkspaceMarquee(UWacomBackpackScreen& Screen)
{
	if (!Screen.WorkspaceInteractionModel || !Screen.WorkspaceWidget)
	{
		return false;
	}

	Screen.WorkspaceInteractionModel->BeginMarquee(FVector2D(20.0f, 20.0f), false);
	Screen.WorkspaceWidget->RefreshInteractionPresentation();
	return Screen.WorkspaceInteractionModel->IsMarqueeActive();
}

bool FWacomBackpackScreenTestAccess::IsWorkspaceMarqueeActive(
	const UWacomBackpackScreen& Screen)
{
	return Screen.WorkspaceInteractionModel
		&& Screen.WorkspaceInteractionModel->IsMarqueeActive();
}

bool FWacomBackpackScreenTestAccess::PressWorkspaceEscape(UWacomBackpackScreen& Screen)
{
	if (!Screen.WorkspaceWidget)
	{
		return false;
	}

	const FKeyEvent EscapeEvent(
		EKeys::Escape,
		FModifierKeysState(),
		/*UserIndex*/ 0,
		/*bIsRepeat*/ false,
		/*CharacterCode*/ 0,
		/*KeyCode*/ 0);
	return Screen.WorkspaceWidget->NativeOnKeyDown(FGeometry(), EscapeEvent).IsEventHandled();
}

bool FWacomBackpackScreenTestAccess::ReleaseCurrentToPileWithSynchronousRefresh(
	UWacomBackpackScreen& Screen,
	EZoneKind TargetZone,
	FGuid TargetOwnerInstanceId)
{
	URunSession* Run = Screen.GetRunSession();
	if (!Run || !Screen.WorkspaceInteractionModel || !Screen.WorkspaceInteractionModel->IsCarrying())
	{
		return false;
	}

	FWacomBackpackWorkspaceReleaseIntent Intent =
		Screen.WorkspaceInteractionModel->BuildReleaseIntent(false);
	if (Intent.bConsumedByInitialReleaseGuard)
	{
		Intent = Screen.WorkspaceInteractionModel->BuildReleaseIntent(false);
	}
	if (Intent.InstanceIds.IsEmpty())
	{
		return false;
	}

	Run->OnRunStateChangedNative.AddUObject(&Screen, &UWacomBackpackScreen::HandleViewModelRefreshed);
	Screen.HandleWorkspacePileReleaseIntent(
		Intent,
		FWacomBackpackZoneKey::Make(TargetZone, TargetOwnerInstanceId));
	Run->OnRunStateChangedNative.RemoveAll(&Screen);
	return true;
}

bool FWacomBackpackScreenTestAccess::ReleaseAllToPileWithSynchronousRefresh(
	UWacomBackpackScreen& Screen,
	EZoneKind TargetZone,
	FGuid TargetOwnerInstanceId)
{
	URunSession* Run = Screen.GetRunSession();
	if (!Run || !Screen.WorkspaceInteractionModel || !Screen.WorkspaceInteractionModel->IsCarrying())
	{
		return false;
	}

	FWacomBackpackWorkspaceReleaseIntent Intent =
		Screen.WorkspaceInteractionModel->BuildReleaseIntent(true);
	if (Intent.bConsumedByInitialReleaseGuard)
	{
		Intent = Screen.WorkspaceInteractionModel->BuildReleaseIntent(true);
	}
	if (Intent.InstanceIds.IsEmpty())
	{
		return false;
	}

	Run->OnRunStateChangedNative.AddUObject(&Screen, &UWacomBackpackScreen::HandleViewModelRefreshed);
	Screen.HandleWorkspacePileReleaseIntent(
		Intent,
		FWacomBackpackZoneKey::Make(TargetZone, TargetOwnerInstanceId));
	Run->OnRunStateChangedNative.RemoveAll(&Screen);
	return true;
}

bool FWacomBackpackScreenTestAccess::ClickExpandedPileHeaderThroughOverlappingCard(
	UWacomBackpackScreen& Screen,
	EZoneKind Zone,
	FGuid OwnerInstanceId)
{
	UWacomBackpackWorkspaceWidget* Workspace = Screen.WorkspaceWidget;
	if (!Workspace || !Workspace->InteractionModel)
	{
		return false;
	}

	UWacomBackpackZonePileWidget* TargetPile = nullptr;
	for (const TWeakObjectPtr<UWacomBackpackZonePileWidget>& WeakPile :
		Workspace->GetRegisteredPileWidgets())
	{
		UWacomBackpackZonePileWidget* Pile = WeakPile.Get();
		if (Pile && Pile->GetPileView().HasSameIdentity(Zone, OwnerInstanceId))
		{
			TargetPile = Pile;
			break;
		}
	}
	UWacomDeckCardWidget* OverlappingCard = nullptr;
	for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard : Workspace->BoundCardWidgets)
	{
		UWacomDeckCardWidget* Card = WeakCard.Get();
		if (Card && Card->IsMoveEnabled()
			&& FWacomBackpackZoneKey::Make(
				Card->GetWorkspaceDisplayZone(),
				Card->GetWorkspaceDisplayOwnerInstanceId())
				== FWacomBackpackZoneKey::Make(Zone, OwnerInstanceId))
		{
			OverlappingCard = Card;
			break;
		}
	}
	if (!TargetPile || !OverlappingCard)
	{
		return false;
	}

	const FSlateRect Header = TargetPile->GetResolvedHeaderRect();
	const FVector2D HeaderCenter(
		(Header.Left + Header.Right) * 0.5f,
		(Header.Top + Header.Bottom) * 0.5f);
	const bool bRoutedHeaderPress =
		Workspace->TryBeginPileHeaderPress(HeaderCenter, false);
	const FPointerEvent PointerUp(
		0,
		HeaderCenter,
		HeaderCenter,
		TSet<FKey>(),
		EKeys::LeftMouseButton,
		0.0f,
		FModifierKeysState());
	const FReply UpReply = Workspace->NativeOnMouseButtonUp(FGeometry(), PointerUp);
	return bRoutedHeaderPress && UpReply.IsEventHandled();
}

void FWacomBackpackScreenTestAccess::ActivateWorkspaceScreen(UWacomBackpackScreen& Screen)
{
	Screen.ActivateWidget();
}

void FWacomBackpackScreenTestAccess::DeactivateWorkspaceScreen(UWacomBackpackScreen& Screen)
{
	// Transient automation widgets are not pushed through a CommonUI stack. Establish
	// a valid active state before exercising the public deactivation path so the test
	// observes the same lifecycle contract without calling NativeOn* directly.
	if (!Screen.IsActivated())
	{
		Screen.ActivateWidget();
	}
	Screen.DeactivateWidget();
}

FWacomBackpackWorkspaceAutomationTestView FWacomBackpackScreenTestAccess::WorkspaceView(
	const UWacomBackpackScreen& Screen)
{
	return Screen.WorkspaceWidget
		? Screen.WorkspaceWidget->GetAutomationTestView()
		: FWacomBackpackWorkspaceAutomationTestView();
}

bool FWacomBackpackScreenTestAccess::TickWorkspaceBaseCardLayoutTransitions(
	UWacomBackpackWorkspaceWidget& Workspace,
	float DeltaSeconds)
{
	return Workspace.TickBaseCardLayoutTransitions(DeltaSeconds);
}

bool FWacomBackpackScreenTestAccess::BeginDeleteConfirmation(UWacomBackpackScreen& Screen)
{
	if (!Screen.WorkspaceInteractionModel || !Screen.WorkspaceInteractionModel->IsCarrying()) return false;
	Screen.BeginWorkspaceDeleteConfirmation(Screen.WorkspaceInteractionModel->GetCarry().RemainingInstanceIds);
	return IsDeleteConfirmationPending(Screen);
}

bool FWacomBackpackScreenTestAccess::BeginDeleteConfirmationForIds(
	UWacomBackpackScreen& Screen,
	TConstArrayView<FGuid> InstanceIds)
{
	if (!Screen.WorkspaceInteractionModel || !Screen.WorkspaceInteractionModel->IsCarrying()
		|| InstanceIds.IsEmpty())
	{
		return false;
	}
	Screen.BeginWorkspaceDeleteConfirmation(InstanceIds);
	return IsDeleteConfirmationPending(Screen);
}

void FWacomBackpackScreenTestAccess::ConfirmDelete(UWacomBackpackScreen& Screen) { Screen.HandleWorkspaceDeleteConfirmed(); }
void FWacomBackpackScreenTestAccess::CancelDelete(UWacomBackpackScreen& Screen) { Screen.HandleWorkspaceDeleteCancelled(); }
bool FWacomBackpackScreenTestAccess::IsDeleteConfirmationPending(const UWacomBackpackScreen& Screen)
{
	return Screen.PendingDeleteConfirmation && Screen.PendingDeleteConfirmation->bPending;
}
int32 FWacomBackpackScreenTestAccess::DeletePreviewCardCount(const UWacomBackpackScreen& Screen)
{
	return Screen.PendingDeleteConfirmation ? Screen.PendingDeleteConfirmation->PreviewCardCount : 0;
}
int32 FWacomBackpackScreenTestAccess::DeletePreviewGoldReward(const UWacomBackpackScreen& Screen)
{
	return Screen.PendingDeleteConfirmation ? Screen.PendingDeleteConfirmation->PreviewGoldReward : 0;
}

bool FWacomBackpackScreenTestAccess::IsDetailVisible(const UWacomBackpackScreen& Screen)
{
	return Screen.IsCardDetailPanelVisible();
}

FText FWacomBackpackScreenTestAccess::DetailNameText(const UWacomBackpackScreen& Screen)
{
	return Screen.GetCardDetailPanelNameText();
}

bool FWacomBackpackScreenTestAccess::ShowDetailForCardWidget(
	UWacomBackpackScreen& Screen,
	UWacomDeckCardWidget* SourceWidget)
{
	return Screen.ShowCardDetailForCardWidget(SourceWidget);
}

void FWacomBackpackScreenTestAccess::HideDetail(UWacomBackpackScreen& Screen)
{
	Screen.HideCardDetailPanel();
}

#endif
