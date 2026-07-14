// Copyright Wacom. All Rights Reserved.

#include "BackpackScreenTestAccess.h"

#if WITH_AUTOMATION_TESTS

#include "RunSession.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "UI/Backpack/WacomSpecialZoneWidget.h"
#include "../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
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
	return UWacomBackpackScreen::BuildMoveZoneNameTextForTest(Zone);
}

FText FWacomBackpackScreenTestAccess::BuildMoveFailureToastText(FName DisabledReason)
{
	return UWacomBackpackScreen::BuildMoveFailureToastTextForTest(DisabledReason);
}

FText FWacomBackpackScreenTestAccess::BuildDeleteFailureToastText(FName DisabledReason)
{
	return UWacomBackpackScreen::BuildDeleteFailureToastTextForTest(DisabledReason);
}

UWacomDeckCardWidget* FWacomBackpackScreenTestAccess::BattleDeckCard(const UWacomBackpackScreen& Screen, int32 Index)
{
	return Screen.GetBattleDeckCardWidgetForTest(Index);
}

UWacomDeckCardWidget* FWacomBackpackScreenTestAccess::FluxContentCard(const UWacomBackpackScreen& Screen, int32 Index)
{
	return Screen.GetFluxContentCardWidgetForTest(Index);
}

UWacomDeckCardWidget* FWacomBackpackScreenTestAccess::BurdenCard(const UWacomBackpackScreen& Screen, int32 Index)
{
	return Screen.GetBurdenCardWidgetForTest(Index);
}

UWacomSpecialZoneWidget* FWacomBackpackScreenTestAccess::SpecialZone(const UWacomBackpackScreen& Screen, int32 Index)
{
	return Screen.GetSpecialZoneWidgetForTest(Index);
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

int32 FWacomBackpackScreenTestAccess::ZoneRackEntryCount(const UWacomBackpackScreen& Screen)
{
	return View(Screen).ZoneRackEntryCount;
}

int32 FWacomBackpackScreenTestAccess::WorkspaceCardCount(const UWacomBackpackScreen& Screen)
{
	return View(Screen).WorkspaceCardCount;
}

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
FWacomBackpackScreenTestAccess::ProbeSelectedCardPickupPointerSequence(
	UWacomBackpackWorkspaceWidget& Workspace,
	UWacomDeckCardWidget& CardWidget)
{
	FWacomBackpackPickupPointerSequenceProbe Probe;
	if (!Workspace.InteractionModel || !CardWidget.GetCardInstanceId().IsValid())
	{
		return Probe;
	}

	Probe.bCardMovable = CardWidget.IsMoveEnabled();
	Workspace.InteractionModel->ClickCard(CardWidget.GetCardInstanceId(), false);
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
	Screen.HandleZoneActivated(Zone, OwnerInstanceId);
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

bool FWacomBackpackScreenTestAccess::ReleaseCurrentToRackWithSynchronousRefresh(
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
	Screen.HandleWorkspaceRackReleaseIntent(
		Intent,
		FWacomBackpackZoneKey::Make(TargetZone, TargetOwnerInstanceId));
	Run->OnRunStateChangedNative.RemoveAll(&Screen);
	return true;
}

void FWacomBackpackScreenTestAccess::DeactivateWorkspaceScreen(UWacomBackpackScreen& Screen)
{
	Screen.NativeOnDeactivated();
}

FWacomBackpackWorkspaceAutomationTestView FWacomBackpackScreenTestAccess::WorkspaceView(
	const UWacomBackpackScreen& Screen)
{
	return Screen.WorkspaceWidget
		? Screen.WorkspaceWidget->GetAutomationTestView()
		: FWacomBackpackWorkspaceAutomationTestView();
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

FText FWacomBackpackScreenTestAccess::ZoneTitleText(const UWacomSpecialZoneWidget& Zone)
{
	return Zone.GetZoneTitleTextForTest();
}

bool FWacomBackpackScreenTestAccess::IsBattleReadyBadgeVisible(const UWacomSpecialZoneWidget& Zone)
{
	return Zone.IsBattleReadyBadgeVisibleForTest();
}

bool FWacomBackpackScreenTestAccess::RequestContentCardBattleEnabledToggle(
	const UWacomSpecialZoneWidget& Zone,
	int32 Index)
{
	return Zone.RequestContentCardBattleEnabledToggleForTest(Index);
}

UWacomDeckCardWidget* FWacomBackpackScreenTestAccess::OwnerCard(const UWacomSpecialZoneWidget& Zone)
{
	return Zone.GetOwnerCardWidgetForTest();
}

UWacomDeckCardWidget* FWacomBackpackScreenTestAccess::ContentCard(const UWacomSpecialZoneWidget& Zone, int32 Index)
{
	return Zone.GetContentCardWidgetForTest(Index);
}

int32 FWacomBackpackScreenTestAccess::ContentCardCount(const UWacomSpecialZoneWidget& Zone)
{
	return Zone.GetContentCardWidgetCountForTest();
}

#endif
