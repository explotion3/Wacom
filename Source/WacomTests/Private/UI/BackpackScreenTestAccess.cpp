// Copyright Wacom. All Rights Reserved.

#include "BackpackScreenTestAccess.h"

#if WITH_AUTOMATION_TESTS

#include "RunSession.h"
#include "Blueprint/WidgetTree.h"
#include "CommonInputSubsystem.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "UI/Backpack/WacomBackpackControlsHelpWidget.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomBackpackDeleteConfirmWidget.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"
#include "UI/Backpack/WacomBackpackZonePileWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Card/WacomFirstPersonCardViewWidget.h"
#include "Settings/WacomSettingsSubsystem.h"
#include "UI/Foundation/WacomPrimaryGameLayout.h"
#include "UI/ViewModels/WacomRunViewModelProvider.h"
#include "../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceSceneBuilder.h"
#include "../../../WacomApp/Private/UI/Backpack/WacomBackpackCommandFlow.h"
#include "../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceMotionCoordinator.h"
#include "../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceRuntime.h"
#include "../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceRuntimeHost.h"
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

TArray<UWacomDeckCardWidget*> FWacomBackpackScreenTestAccess::WorkspaceCards(
	const UWacomBackpackScreen& Screen)
{
	TArray<UWacomDeckCardWidget*> Cards;
	if (!Screen.WorkspaceWidget)
	{
		return Cards;
	}
	const TConstArrayView<TWeakObjectPtr<UWacomDeckCardWidget>> Registered =
		Screen.WorkspaceWidget->GetBoundCardWidgets();
	Cards.Reserve(Registered.Num());
	for (const TWeakObjectPtr<UWacomDeckCardWidget>& Card : Registered)
	{
		if (UWacomDeckCardWidget* Widget = Card.Get())
		{
			Cards.Add(Widget);
		}
	}
	return Cards;
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
	for (UWacomDeckCardWidget* CardWidget : WorkspaceCards(Screen))
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
	for (UWacomDeckCardWidget* CardWidget : WorkspaceCards(Screen))
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
	for (UWacomDeckCardWidget* CardWidget : WorkspaceCards(Screen))
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
	for (UWacomDeckCardWidget* CardWidget : WorkspaceCards(Screen))
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
	for (UWacomDeckCardWidget* CardWidget : WorkspaceCards(Screen))
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
	const TArray<UWacomDeckCardWidget*> Cards = WorkspaceCards(Screen);
	Positions.Reserve(Cards.Num());
	for (const UWacomDeckCardWidget* CardWidget : Cards)
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
	const TArray<UWacomDeckCardWidget*> Cards = WorkspaceCards(Screen);
	Opacities.Reserve(Cards.Num());
	for (const UWacomDeckCardWidget* CardWidget : Cards)
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
		Screen.WorkspaceWidget->GetRuntime().FrameScheduler.BeginFrame();
		FWacomBackpackWorkspaceRuntimeHost Host(
			*Screen.WorkspaceWidget);
		Host.FlushPresentation();
		Host.ExecuteDeferredCardFaceRender();
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
	const TArray<UWacomDeckCardWidget*> Cards = WorkspaceCards(Screen);
	if (!Screen.WorkspaceWidget || !Screen.WorkspaceInteractionModel
		|| !Cards.IsValidIndex(CardIndex)
		|| !Cards[CardIndex])
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
		Cards[CardIndex],
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
	const TArray<UWacomDeckCardWidget*> Cards = WorkspaceCards(Screen);
	if (!Screen.WorkspaceWidget || !Screen.WorkspaceInteractionModel
		|| !Cards.IsValidIndex(CardIndex)
		|| !Cards[CardIndex])
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
		Cards[CardIndex],
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
			Workspace.GetRuntime().Presentation.IsSimplifiedMotion());
	}
	FWacomBackpackWorkspaceRuntimeHost Host(Workspace);
	Host.FinalizeCompletedSettlements();
}

void FWacomBackpackScreenTestAccess::RefreshWorkspacePresentation(
	UWacomBackpackWorkspaceWidget& Workspace)
{
	Workspace.RequestPresentationRefresh(
		EWacomBackpackWorkspacePresentationDirty::All,
		{},
		true);
}

bool FWacomBackpackScreenTestAccess::ToggleWorkspaceCardSelection(
	UWacomBackpackWorkspaceWidget& Workspace,
	UWacomDeckCardWidget& Card)
{
	const FVector2D Pointer(120.0f, 120.0f);
	const FModifierKeysState ControlModifier(
		false, false, true, false, false, false, false, false, false);
	const FPointerEvent PointerDown(
		0,
		Pointer,
		Pointer,
		TSet<FKey>{ EKeys::LeftMouseButton },
		EKeys::LeftMouseButton,
		0.0f,
		ControlModifier);
	const FPointerEvent PointerUp(
		0,
		Pointer,
		Pointer,
		TSet<FKey>(),
		EKeys::LeftMouseButton,
		0.0f,
		ControlModifier);
	const bool bDownHandled = Workspace.HandleCardPointerDownAtLocal(
		&Card,
		Pointer,
		PointerDown,
		false).IsEventHandled();
	const bool bUpHandled = Workspace.HandleCardPointerUp(
		&Card,
		FGeometry(),
		PointerUp).IsEventHandled();
	return bDownHandled && bUpHandled;
}

void FWacomBackpackScreenTestAccess::SendWorkspaceCarryPointerEvents(
	UWacomBackpackWorkspaceWidget& Workspace,
	UWacomDeckCardWidget& CardWidget,
	TConstArrayView<FVector2D> PointerLocals)
{
	FVector2D Previous = Workspace.GetRuntime().Presentation.CarryAnchorLocal;
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
	float RemainingSeconds = FMath::Max(0.0f, DeltaSeconds);
	while (RemainingSeconds > UE_SMALL_NUMBER)
	{
		const float StepSeconds = FMath::Min(RemainingSeconds, 1.0f / 60.0f);
		FWacomBackpackWorkspaceRuntimeHost Host(Workspace);
		Host.RefreshFrameWork();
		FWacomBackpackWorkspaceFrameScheduler& Scheduler =
			Workspace.GetRuntime().FrameScheduler;
		if (!Scheduler.WantsFrame())
		{
			break;
		}
		const uint64 Generation = Scheduler.IsTimerRegistered()
			? Scheduler.GetTimerGeneration()
			: Scheduler.MarkTimerRegistered();
		Workspace.TickFrameScheduler(Generation, StepSeconds);
		RemainingSeconds -= StepSeconds;
	}
}

bool FWacomBackpackScreenTestAccess::TickWorkspaceFrameScheduler(
	UWacomBackpackWorkspaceWidget& Workspace,
	const uint64 TimerGeneration,
	const float DeltaSeconds)
{
	return Workspace.TickFrameScheduler(TimerGeneration, DeltaSeconds)
		== EActiveTimerReturnType::Continue;
}

uint64 FWacomBackpackScreenTestAccess::PrepareIdleWorkspaceFrameScheduler(
	UWacomBackpackWorkspaceWidget& Workspace)
{
	Workspace.GetRuntime().FrameScheduler.Reset();
	return Workspace.GetRuntime().FrameScheduler.MarkTimerRegistered();
}

bool FWacomBackpackScreenTestAccess::HasWorkspaceRuntime(
	const UWacomBackpackWorkspaceWidget& Workspace)
{
	return Workspace.Runtime.IsValid();
}

void FWacomBackpackScreenTestAccess::DestructWorkspace(
	UWacomBackpackWorkspaceWidget& Workspace)
{
	Workspace.NativeDestruct();
}

void FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointer(
	UWacomBackpackWorkspaceWidget& Workspace,
	FVector2D PointerLocal)
{
	Workspace.UpdateExpandedPileFocus(PointerLocal);
}

void FWacomBackpackScreenTestAccess::MoveWorkspaceBrowsePointerWithShiftState(
	UWacomBackpackWorkspaceWidget& Workspace,
	FVector2D PointerLocal,
	bool bLeftShiftDown,
	bool bRightShiftDown)
{
	const FModifierKeysState Modifiers(
		bLeftShiftDown,
		bRightShiftDown,
		false,
		false,
		false,
		false,
		false,
		false,
		false);
	const FPointerEvent PointerMove(
		0,
		PointerLocal,
		PointerLocal,
		TSet<FKey>(),
		EKeys::Invalid,
		0.0f,
		Modifiers);
	Workspace.NativeOnMouseMove(FGeometry(), PointerMove);
}

bool FWacomBackpackScreenTestAccess::SetWorkspaceHandLensLock(
	UWacomBackpackWorkspaceWidget& Workspace,
	bool bLocked,
	bool bRepeat)
{
	const FModifierKeysState Modifiers(
		bLocked,
		false,
		false,
		false,
		false,
		false,
		false,
		false,
		false);
	const FKeyEvent ShiftEvent(
		EKeys::LeftShift,
		Modifiers,
		/*UserIndex*/ 0,
		bRepeat,
		/*CharacterCode*/ 0,
		/*KeyCode*/ 0);
	const FReply Reply = bLocked
		? Workspace.NativeOnKeyDown(FGeometry(), ShiftEvent)
		: Workspace.NativeOnKeyUp(FGeometry(), ShiftEvent);
	return Reply.IsEventHandled();
}

void FWacomBackpackScreenTestAccess::LoseWorkspaceKeyboardFocus(
	UWacomBackpackWorkspaceWidget& Workspace)
{
	Workspace.NativeOnFocusLost(FFocusEvent());
}

bool FWacomBackpackScreenTestAccess::PressExpandedPileVisualCard(
	UWacomBackpackWorkspaceWidget& Workspace,
	FVector2D PointerLocal,
	bool bLeftShiftDown)
{
	TSet<FKey> PressedButtons{ EKeys::LeftMouseButton };
	const FModifierKeysState Modifiers(
		bLeftShiftDown,
		false,
		false,
		false,
		false,
		false,
		false,
		false,
		false);
	const FPointerEvent PointerDown(
		0,
		FVector2D::ZeroVector,
		FVector2D::ZeroVector,
		PressedButtons,
		EKeys::LeftMouseButton,
		0.0f,
		Modifiers);
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
	FWacomBackpackWorkspaceRuntimeHost Host(Workspace);
	Host.AdvanceExpandedPileFocusExit(DeltaSeconds);
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
	const FPointerEvent PointerDown(
		0, Start, Start, TSet<FKey>{ EKeys::LeftMouseButton },
		EKeys::LeftMouseButton, 0.0f, FModifierKeysState());
	const FPointerEvent PointerMove(
		0, End, Start, TSet<FKey>{ EKeys::LeftMouseButton },
		EKeys::LeftMouseButton, 0.0f, FModifierKeysState());
	Workspace.BeginPendingPilePress(*TargetPile, Start, PointerDown, false);
	const bool bBeganMarquee = Workspace.TryBeginMarqueeFromPendingPilePress(End, PointerMove);
	Workspace.InteractionModel->UpdateMarquee(End);
	Workspace.InteractionModel->CompleteMarquee();
	Workspace.UpdateSelectionVisualFreezeLifetime();
	return bBeganMarquee && !Workspace.InteractionModel->IsMarqueeActive();
}

FWacomBackpackMarqueePaintHotPathProbe
FWacomBackpackScreenTestAccess::ProbeMarqueePaintHotPath(
	UWacomBackpackWorkspaceWidget& Workspace,
	FVector2D Start,
	FVector2D End)
{
	FWacomBackpackMarqueePaintHotPathProbe Probe;
	if (!Workspace.InteractionModel)
	{
		return Probe;
	}

	Workspace.InteractionModel->BeginMarquee(
		FWacomBackpackZoneKey::Make(EZoneKind::Backpack),
		Start,
		false);
	Workspace.RequestPresentationRefresh(
		EWacomBackpackWorkspacePresentationDirty::All,
		{},
		true);
	const FWacomBackpackWorkspaceAutomationTestView Before =
		Workspace.GetAutomationTestView();
	Probe.FullPresentationRefreshCountBefore = Before.FullPresentationRefreshCount;
	Probe.WorkspaceSceneBindCountBefore = Before.WorkspaceSceneBindCount;
	Probe.CarryStripLayoutRebuildCountBefore = Before.CarryStripLayoutRebuildCount;

	const FPointerEvent PointerMove(
		0,
		End,
		Start,
		TSet<FKey>{ EKeys::LeftMouseButton },
		EKeys::Invalid,
		0.0f,
		FModifierKeysState());
	Probe.bMoveHandled = Workspace.NativeOnMouseMove(
		Workspace.GetCachedGeometry(),
		PointerMove).IsEventHandled();
	const FWacomBackpackWorkspaceAutomationTestView After =
		Workspace.GetAutomationTestView();
	Probe.bMarqueeRemainsActive = Workspace.InteractionModel->IsMarqueeActive();
	Probe.FullPresentationRefreshCountAfter = After.FullPresentationRefreshCount;
	Probe.WorkspaceSceneBindCountAfter = After.WorkspaceSceneBindCount;
	Probe.CarryStripLayoutRebuildCountAfter = After.CarryStripLayoutRebuildCount;
	return Probe;
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
	Workspace.RequestPresentationRefresh(
		EWacomBackpackWorkspacePresentationDirty::All,
		{},
		true);
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
	const FPointerEvent PointerDown(
		0, HeaderStart, HeaderStart, TSet<FKey>{ EKeys::LeftMouseButton },
		EKeys::LeftMouseButton, 0.0f, FModifierKeysState());
	const FPointerEvent PointerMove(
		0, PointerEnd, HeaderStart, TSet<FKey>{ EKeys::LeftMouseButton },
		EKeys::LeftMouseButton, 0.0f, FModifierKeysState());
	Workspace.BeginPendingPilePress(*TargetPile, HeaderStart, PointerDown, false, true);
	Probe.bBeganMove = Workspace.TryBeginPileMove(PointerEnd, PointerMove);
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
	UWacomBackpackWorkspaceWidget& Workspace,
	bool bReleaseAll)
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
	Workspace.BroadcastPointerRelease(bReleaseAll);
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
	const TArray<UWacomDeckCardWidget*> Cards = WorkspaceCards(Screen);
	if (!Screen.WorkspaceInteractionModel || !Cards.IsValidIndex(CardIndex)
		|| !Cards[CardIndex])
	{
		return false;
	}
	Screen.WorkspaceInteractionModel->SelectAllMovable();
	const bool bStarted = Screen.WorkspaceInteractionModel->BeginCarry(
		Cards[CardIndex]->GetCardInstanceId(),
		FVector2D(500.0f, 350.0f),
		Screen.GetRunSession() ? Screen.GetRunSession()->GetBackpackStorageSnapshotRevision() : 0);
	if (Screen.WorkspaceWidget)
	{
		Screen.WorkspaceWidget->RequestPresentationRefresh(
			EWacomBackpackWorkspacePresentationDirty::All,
			{},
			true);
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
	if (Screen.WorkspaceWidget)
	{
		Screen.WorkspaceWidget->RequestPresentationRefresh(
			EWacomBackpackWorkspacePresentationDirty::All,
			{},
			true);
	}
	return bStarted;
}

bool FWacomBackpackScreenTestAccess::BeginWorkspaceMarquee(UWacomBackpackScreen& Screen)
{
	if (!Screen.WorkspaceInteractionModel || !Screen.WorkspaceWidget)
	{
		return false;
	}

	Screen.WorkspaceInteractionModel->BeginMarquee(FVector2D(20.0f, 20.0f), false);
	Screen.WorkspaceWidget->RequestPresentationRefresh(
		EWacomBackpackWorkspacePresentationDirty::All,
		{},
		true);
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
	for (const TWeakObjectPtr<UWacomDeckCardWidget>& WeakCard : Workspace->GetBoundCardWidgets())
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
	const FPointerEvent PointerDown(
		0,
		HeaderCenter,
		HeaderCenter,
		TSet<FKey>{ EKeys::LeftMouseButton },
		EKeys::LeftMouseButton,
		0.0f,
		FModifierKeysState());
	const bool bRoutedHeaderPress =
		Workspace->TryBeginPileHeaderPress(HeaderCenter, PointerDown, false);
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

void FWacomBackpackScreenTestAccess::DestructWorkspaceScreen(UWacomBackpackScreen& Screen)
{
	Screen.NativeDestruct();
}

void FWacomBackpackScreenTestAccess::SetActiveSubscriptionSources(
	UWacomBackpackScreen& Screen,
	UWacomRunViewModelProvider* Provider,
	UWacomSettingsSubsystem* Settings,
	UCommonInputSubsystem* CommonInput,
	UWacomPrimaryGameLayout* PrimaryLayout)
{
	Screen.RunViewModelProviderOverrideForTest = Provider;
	Screen.SettingsSubsystemOverrideForTest = Settings;
	Screen.CommonInputSubsystemOverrideForTest = CommonInput;
	Screen.PrimaryLayoutOverrideForTest = PrimaryLayout;
}

FWacomBackpackControlsHelpLifecycleProbe
FWacomBackpackScreenTestAccess::ProbeControlsHelpLifecycle(
	UWacomBackpackScreen& Screen)
{
	FWacomBackpackControlsHelpLifecycleProbe Probe;
	if (!FSlateApplication::IsInitialized())
	{
		return Probe;
	}
	const TSharedRef<SWidget> ScreenSlate = Screen.TakeWidget();
	FSlateApplication::Get().SetUserFocus(0, ScreenSlate, EFocusCause::SetDirectly);
	const TWeakPtr<SWidget> FocusBefore =
		FSlateApplication::Get().GetUserFocusedWidget(0);
	Screen.ShowControlsHelp();
	Probe.bOpened = Screen.IsControlsHelpVisible();
	Probe.bPreviousFocusCaptured = Screen.ControlsHelpWidget
		&& Screen.FocusBeforeControlsHelp.IsValid();
	Screen.HideControlsHelp(true);
	Probe.bFocusRestored = FocusBefore.IsValid()
		&& FSlateApplication::Get().GetUserFocusedWidget(0) == FocusBefore.Pin();
	Screen.ActivateWidget();
	Screen.ShowControlsHelp();
	Screen.DeactivateWidget();
	Probe.bHiddenAfterDeactivate = !Screen.IsControlsHelpVisible();
	return Probe;
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
	FWacomBackpackWorkspaceRuntimeHost Host(Workspace);
	return Host.AdvanceBaseCardLayoutTransitions(DeltaSeconds);
}

void FWacomBackpackScreenTestAccess::SubmitWorkspaceDelete(
	UWacomBackpackScreen& Screen,
	TConstArrayView<FGuid> InstanceIds)
{
	Screen.SubmitWorkspaceDelete(InstanceIds);
}

void FWacomBackpackScreenTestAccess::SetWorkspaceSimplifiedMotion(
	UWacomBackpackScreen& Screen,
	bool bSimplified)
{
	if (Screen.WorkspaceWidget)
	{
		Screen.WorkspaceWidget->SetSimplifiedMotion(bSimplified);
	}
}

void FWacomBackpackScreenTestAccess::TickWorkspaceSaleDeparture(
	UWacomBackpackScreen& Screen,
	float DeltaSeconds)
{
	if (Screen.WorkspaceWidget)
	{
		Screen.WorkspaceWidget->TickSaleDepartureForTest(DeltaSeconds);
	}
}

void FWacomBackpackScreenTestAccess::ForceWorkspaceSaleReadiness(
	UWacomBackpackScreen& Screen)
{
	if (Screen.WorkspaceWidget)
	{
		Screen.WorkspaceWidget->ForceSaleDepartureReadinessForTest();
	}
}

TArray<FWacomBackpackSaleCardSurfaceProbe>
FWacomBackpackScreenTestAccess::WorkspaceSaleSurfaceProbes(
	UWacomBackpackScreen& Screen)
{
	TArray<FWacomBackpackSaleCardSurfaceProbe> Result;
	if (!Screen.WorkspaceWidget)
	{
		return Result;
	}
	for (UWacomDeckCardWidget* Card :
		Screen.WorkspaceWidget->GetActiveSaleDepartureCardsForTest())
	{
		if (!Card || !Card->BackpackCardView)
		{
			continue;
		}
		const FWacomFirstPersonCardViewAutomationTestView View =
			Card->BackpackCardView->GetAutomationTestViewForTest();
		FWacomBackpackSaleCardSurfaceProbe& Probe =
			Result.AddDefaulted_GetRef();
		Probe.InstanceId = Card->GetCardInstanceId();
		Probe.bPlayedDissolveActive =
			View.SurfaceEffectView.PlayedDissolve.bActive;
		Probe.bUsingSurfaceEffectMaterial =
			View.bUsingSurfaceEffectMaterial;
		Probe.bRealtimePresentationEnabled =
			View.bRealtimePresentationEnabled;
		Probe.bSelectionPresentationCleared =
			!Card->bWorkspaceSelected
			&& !Card->bWorkspaceCurrent;
		Probe.bFeedbackOverlayCollapsed =
			!Card->WorkspaceFeedbackOverlay
			|| Card->WorkspaceFeedbackOverlay->GetVisibility()
				== ESlateVisibility::Collapsed;
		Probe.bAccessibilityPresentationCleared =
			!Card->bWorkspaceNavigationFocused
			&& Card->WorkspaceSemanticIcon
				== EWacomBackpackWorkspaceCardSemanticIcon::None
			&& Card->WorkspaceFocusPaintBrush.GetResourceObject() == nullptr
			&& Card->WorkspaceSemanticPaintBrush.GetResourceObject() == nullptr;
		Probe.Amount = View.SurfaceEffectView.PlayedDissolve.Amount;
		Probe.Seed = View.SurfaceEffectView.PlayedDissolve.Seed;
	}
	return Result;
}

void FWacomBackpackScreenTestAccess::ResetWorkspaceSaleDepartures(
	UWacomBackpackScreen& Screen)
{
	if (Screen.WorkspaceWidget)
	{
		Screen.WorkspaceWidget->ResetSaleDepartures();
	}
}

bool FWacomBackpackScreenTestAccess::HasRuntimeDeleteConfirmationWidget(
	const UWacomBackpackScreen& Screen)
{
	return Screen.DeleteConfirmWidget != nullptr;
}

bool FWacomBackpackScreenTestAccess::IsDeleteConfirmationHostVisible(
	const UWacomBackpackScreen& Screen)
{
	return Screen.DeleteConfirmHost
		&& Screen.DeleteConfirmHost->GetVisibility() != ESlateVisibility::Collapsed;
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
