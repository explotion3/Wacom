// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomRunPathSegmentActor.h"
#include "Actors/WacomBattleEnemyActor.h"
#include "Components/SplineComponent.h"
#include "Components/WacomBattleCameraLookComponent.h"
#include "Components/WacomCursorLookDriverComponent.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Components/WacomFirstPersonViewStageBlendComponent.h"
#include "Components/WacomRunPathTraversalComponent.h"
#include "UI/RunPathTraversalTestAccess.h"
#include "Cards/CardDefinition.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Fixtures/BattleTestFixtures.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "GameFramework/WacomPlayerController.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/BattleSceneTargetClickTestAccess.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "UI/Card/WacomCardPresentationBuilder.h"
#include "UI/Card/WacomFirstPersonCardLayerSourceIds.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/Card/WacomFirstPersonCardViewWidget.h"
#include "UI/Card/WacomCardView.h"
#include "UI/CardViewTestAccess.h"
#include "UI/FirstPersonCardLayerTestAccess.h"
#include "UI/FirstPersonCardLayerInteractionSpecFixture.h"
#include "UI/FirstPersonCardLayerSpecReceiver.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerValidWorldTargetFeedbackTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.AimingValidWorldTargetShowsValidArrowAndTargetPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerValidWorldTargetFeedbackTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*Layer, DragConfig);
	FWacomFirstPersonCardLayerTestAccess::SetInteractionFeedbackConfig(*Layer, WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot = WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(Slot, EWacomFirstPersonCardInteractionIntent::AimWorldTarget);
	Layer->SetCardSlots({ Slot });

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(540.0f, 590.0f));

	const FVector2D TargetScreenPosition(700.0f, 420.0f);
	const FWacomInteractionTargetHandle TargetHandle =
		FWacomInteractionTargetHandle::ForWorldTarget(FGuid::NewGuid(), PC, FVector::ZeroVector, TargetScreenPosition);
	Layer->SetCardDragFeedbackTarget(
		TargetHandle,
		true,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget,
		TargetScreenPosition);

	const FWacomFirstPersonCardDragView DragView = FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView;
	TestEqual(TEXT("Drag view records valid world feedback"),
		DragView.TargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget);
	TestTrue(TEXT("Drag view has feedback target position"), DragView.bHasFeedbackTargetScreenPosition);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, FVector2D(540.0f, 590.0f));
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerAimArrowFallbackTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.AimArrowFallsBackToPointerWithoutTargetScreenPosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerAimArrowFallbackTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*Layer, DragConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot = WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(Slot, EWacomFirstPersonCardInteractionIntent::AimWorldTarget);
	Layer->SetCardSlots({ Slot });
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	const FVector2D PointerPosition(540.0f, 590.0f);
	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, PointerPosition);
	Layer->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle::ForWorldTarget(FGuid::NewGuid(), PC),
		true,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget);

	TestEqual(TEXT("Missing target position keeps arrow at pointer"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).AimArrowEnd,
		PointerPosition);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, PointerPosition);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerWorldTargetFeedbackPersistsAcrossDragUpdateTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.WorldTargetStatePersistsAcrossDragUpdate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerWorldTargetFeedbackPersistsAcrossDragUpdateTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*Layer, DragConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot = WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(Slot, EWacomFirstPersonCardInteractionIntent::AimWorldTarget);
	Layer->SetCardSlots({ Slot });
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	const FVector2D FirstPointerPosition(540.0f, 590.0f);
	const FVector2D SecondPointerPosition(560.0f, 570.0f);
	const FVector2D TargetPosition(740.0f, 390.0f);
	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FirstPointerPosition);
	Layer->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle::ForWorldTarget(FGuid::NewGuid(), PC, FVector::ZeroVector, TargetPosition),
		true,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget,
		TargetPosition);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, SecondPointerPosition);

	TestEqual(TEXT("Valid feedback state survives next drag update"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.TargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget);
	TestEqual(TEXT("Arrow continues following the pointer after drag update"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).AimArrowEnd,
		SecondPointerPosition);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, SecondPointerPosition);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerCardProbeFeedbackTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.CardTargetShowsProbeFeedbackWithoutSubmit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerCardProbeFeedbackTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*Layer, DragConfig);
	FWacomFirstPersonCardLayerTestAccess::SetInteractionFeedbackConfig(*Layer, WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid TargetCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView SourceSlot = WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(SourceCardId, true, true);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(SourceSlot, EWacomFirstPersonCardInteractionIntent::AimCardTarget);
	FWacomFirstPersonCardLayerSlotView TargetSlot = WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(TargetCardId, true, true);
	TargetSlot.Index = 1;
	TargetSlot.ScreenPosition = FVector2D(650.0f, 600.0f);
	Layer->SetCardSlots({ SourceSlot, TargetSlot });

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* TargetWidget = Layer->GetSlotWidgetAt(1);
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Target slot"), TargetWidget))
	{
		PC->Destroy();
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, FVector2D(540.0f, 590.0f));
	Layer->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle::ForCardTarget(TargetCardId, TargetWidget, TargetSlot.ScreenPosition),
		false,
		EWacomFirstPersonCardDragTargetFeedbackState::CardProbe,
		TargetSlot.ScreenPosition);

	TestTrue(TEXT("Target card shows probe feedback"), FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragProbeFeedback);
	TestEqual(TEXT("Layer records card probe state"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.TargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::CardProbe);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SourceWidget, FVector2D(540.0f, 590.0f));
	TestEqual(TEXT("Card probe release returns idle"), SourceWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::Idle);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDragPointerCardTargetProbeTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.DragPointerOverCardTargetShowsProbeFeedback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDragPointerCardTargetProbeTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*Layer, DragConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid TargetCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView SourceSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(SourceCardId, true, true);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(SourceSlot, EWacomFirstPersonCardInteractionIntent::AimWorldTarget);
	SourceSlot.ScreenPosition = FVector2D(500.0f, 600.0f);
	SourceSlot.WidgetPosition = SourceSlot.ScreenPosition;
	SourceSlot.SnappedWidgetPosition = SourceSlot.ScreenPosition;
	FWacomFirstPersonCardLayerSlotView TargetSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(TargetCardId, true, true);
	TargetSlot.Index = 1;
	TargetSlot.ScreenPosition = FVector2D(650.0f, 600.0f);
	TargetSlot.WidgetPosition = TargetSlot.ScreenPosition;
	TargetSlot.SnappedWidgetPosition = TargetSlot.ScreenPosition;
	TargetSlot.ZOrder = 1;
	Layer->SetCardSlots({ SourceSlot, TargetSlot });

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* TargetWidget = Layer->GetSlotWidgetAt(1);
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Target slot"), TargetWidget))
	{
		PC->Destroy();
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, FVector2D(650.0f, 600.0f));

	const FWacomFirstPersonCardDragView DragView = FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView;
	TestEqual(TEXT("Pointer card target records card kind"),
		DragView.CurrentTarget.TargetKind,
		EWacomInteractionTargetKind::Card);
	TestEqual(TEXT("Pointer card target records target card id"), DragView.CurrentTarget.CardInstanceId, TargetCardId);
	TestEqual(TEXT("Pointer card target state is CardProbe"),
		DragView.TargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::CardProbe);
	TestEqual(TEXT("Pointer card target position uses target visual slot"),
		DragView.FeedbackTargetScreenPosition,
		TargetWidget->GetVisualSlotView().ScreenPosition);
	TestTrue(TEXT("Target card shows probe feedback from drag pointer"), FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragProbeFeedback);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, FVector2D(320.0f, 600.0f));
	TestFalse(TEXT("Moving away from target card clears probe feedback"), FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragProbeFeedback);
	TestNotEqual(TEXT("Moving away from target card clears card probe state"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.TargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::CardProbe);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SourceWidget, FVector2D(650.0f, 600.0f));
	TestEqual(TEXT("Card probe drag release returns idle"), SourceWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::Idle);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerCardTargetValidFeedbackPersistenceTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.CardTargetValidFeedbackPersistsOnSamePointerTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerCardTargetValidFeedbackPersistenceTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardInspectHoldDelaySeconds = 0.0f;
	DragConfig.CardDragStartThresholdPixels = 1.0f;
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*Layer, DragConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid TargetCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView SourceSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SourceCardId, 0, FVector2D(500.0f, 600.0f), 0.0f, 1.0f);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(SourceSlot, EWacomFirstPersonCardInteractionIntent::AimCardTarget);
	FWacomFirstPersonCardLayerSlotView TargetSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(TargetCardId, 1, FVector2D(650.0f, 600.0f), 0.0f, 1.0f);
	Layer->SetCardSlots({ SourceSlot, TargetSlot });

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* TargetWidget = Layer->GetSlotWidgetAt(1);
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Target slot"), TargetWidget))
	{
		PC->Destroy();
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, SourceSlot.ScreenPosition);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, TargetSlot.ScreenPosition);
	TestEqual(TEXT("Initial pointer target waits for HUD card validation"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.TargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::CardProbe);

	Layer->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle::ForCardTarget(TargetCardId, TargetWidget, TargetSlot.ScreenPosition),
		true,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget,
		TargetSlot.ScreenPosition,
		TEXT("CardDrop{Intent=PlayCardCardTarget}"));
	TestEqual(TEXT("HUD valid card target is stored"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.TargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, TargetSlot.ScreenPosition + FVector2D(4.0f, 0.0f));
	const FWacomFirstPersonCardDragView DragView = FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView;
	TestEqual(TEXT("Same pointer card target keeps valid state"),
		DragView.TargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);
	TestTrue(TEXT("Same pointer card target keeps valid flag"), DragView.bTargetValid);
	TestEqual(TEXT("Same pointer card target keeps target id"), DragView.CurrentTarget.CardInstanceId, TargetCardId);
	TestEqual(TEXT("Target widget stays valid after next pointer move"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).DragTargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);
	TestEqual(TEXT("Target widget focus stays valid after next pointer move"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).CardDragTargetFocusFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);
	TestTrue(TEXT("Target widget keeps focus after next pointer move"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragTargetFocusActive);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SourceWidget, TargetSlot.ScreenPosition);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerCardTargetSwitchResetsToProbeTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.SwitchingCardTargetReturnsToCardProbe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerCardTargetSwitchResetsToProbeTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardInspectHoldDelaySeconds = 0.0f;
	DragConfig.CardDragStartThresholdPixels = 1.0f;
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*Layer, DragConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid FirstTargetCardId = FGuid::NewGuid();
	const FGuid SecondTargetCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView SourceSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SourceCardId, 0, FVector2D(500.0f, 600.0f), 0.0f, 1.0f);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(SourceSlot, EWacomFirstPersonCardInteractionIntent::AimCardTarget);
	FWacomFirstPersonCardLayerSlotView FirstTargetSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(FirstTargetCardId, 1, FVector2D(650.0f, 600.0f), 0.0f, 1.0f);
	FWacomFirstPersonCardLayerSlotView SecondTargetSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SecondTargetCardId, 2, FVector2D(820.0f, 600.0f), 0.0f, 1.0f);
	Layer->SetCardSlots({ SourceSlot, FirstTargetSlot, SecondTargetSlot });

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* FirstTargetWidget = Layer->GetSlotWidgetAt(1);
	UWacomFirstPersonCardLayerSlotWidget* SecondTargetWidget = Layer->GetSlotWidgetAt(2);
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("First target slot"), FirstTargetWidget)
		|| !TestNotNull(TEXT("Second target slot"), SecondTargetWidget))
	{
		PC->Destroy();
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, SourceSlot.ScreenPosition);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, FirstTargetSlot.ScreenPosition);
	Layer->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle::ForCardTarget(FirstTargetCardId, FirstTargetWidget, FirstTargetSlot.ScreenPosition),
		true,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget,
		FirstTargetSlot.ScreenPosition,
		TEXT("CardDrop{Intent=PlayCardCardTarget}"));
	TestEqual(TEXT("First target starts valid"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.TargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, SecondTargetSlot.ScreenPosition);
	const FWacomFirstPersonCardDragView DragView = FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView;
	TestEqual(TEXT("New pointer card target records second target"), DragView.CurrentTarget.CardInstanceId, SecondTargetCardId);
	TestEqual(TEXT("New pointer card target returns to probe"),
		DragView.TargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::CardProbe);
	TestFalse(TEXT("New pointer card target waits for HUD validity"), DragView.bTargetValid);
	TestEqual(TEXT("Old target feedback clears without affordance"),
		FWacomFirstPersonCardLayerTestAccess::View(*FirstTargetWidget).DragTargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::None);
	TestFalse(TEXT("Old target focus clears"),
		FWacomFirstPersonCardLayerTestAccess::View(*FirstTargetWidget).bCardDragTargetFocusActive);
	TestEqual(TEXT("New target shows probe"),
		FWacomFirstPersonCardLayerTestAccess::View(*SecondTargetWidget).DragTargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::CardProbe);
	TestTrue(TEXT("New target gains focus"),
		FWacomFirstPersonCardLayerTestAccess::View(*SecondTargetWidget).bCardDragTargetFocusActive);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SourceWidget, SecondTargetSlot.ScreenPosition);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerAllValidAffordanceUniqueFocusTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.AllValidAffordancesKeepUniquePointerFocus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerAllValidAffordanceUniqueFocusTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardInspectHoldDelaySeconds = 0.0f;
	DragConfig.CardDragStartThresholdPixels = 1.0f;
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*Layer, DragConfig);
	FWacomFirstPersonCardSlotMotionConfig MotionConfig;
	MotionConfig.bEnabled = false;
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, MotionConfig);
	FWacomFirstPersonCardSlotVisualConfig VisualConfig;
	FWacomFirstPersonCardLayerTestAccess::SetSlotVisualConfig(*Layer, VisualConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid FirstTargetCardId = FGuid::NewGuid();
	const FGuid SecondTargetCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView SourceSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SourceCardId, 0, FVector2D(500.0f, 600.0f), 0.0f, 1.0f);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(SourceSlot, EWacomFirstPersonCardInteractionIntent::AimCardTarget);
	FWacomFirstPersonCardLayerSlotView FirstTargetSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(FirstTargetCardId, 1, FVector2D(650.0f, 600.0f), 0.0f, 1.0f);
	FWacomFirstPersonCardLayerSlotView SecondTargetSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SecondTargetCardId, 2, FVector2D(820.0f, 600.0f), 0.0f, 1.0f);
	Layer->SetCardSlots({ SourceSlot, FirstTargetSlot, SecondTargetSlot });

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* FirstTargetWidget = Layer->GetSlotWidgetAt(1);
	UWacomFirstPersonCardLayerSlotWidget* SecondTargetWidget = Layer->GetSlotWidgetAt(2);
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("First target slot"), FirstTargetWidget)
		|| !TestNotNull(TEXT("Second target slot"), SecondTargetWidget))
	{
		PC->Destroy();
		return false;
	}

	const FWacomFirstPersonCardLayerSlotView FirstBaseVisual = FirstTargetWidget->GetVisualSlotView();
	const int32 FirstBaseZOrder = Layer->GetCardZOrderAt(1);
	const FWacomFirstPersonCardLayerSlotView SecondBaseVisual = SecondTargetWidget->GetVisualSlotView();
	const int32 SecondBaseZOrder = Layer->GetCardZOrderAt(2);

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, SourceSlot.ScreenPosition);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, FirstTargetSlot.ScreenPosition);

	TArray<FWacomFirstPersonCardTargetAffordance> Affordances;
	FWacomFirstPersonCardTargetAffordance FirstAffordance;
	FirstAffordance.CardInstanceId = FirstTargetCardId;
	FirstAffordance.FeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget;
	FirstAffordance.bCanSubmit = true;
	Affordances.Add(FirstAffordance);
	FWacomFirstPersonCardTargetAffordance SecondAffordance;
	SecondAffordance.CardInstanceId = SecondTargetCardId;
	SecondAffordance.FeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget;
	SecondAffordance.bCanSubmit = true;
	Affordances.Add(SecondAffordance);

	Layer->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle::ForCardTarget(FirstTargetCardId, FirstTargetWidget, FirstTargetSlot.ScreenPosition),
		true,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget,
		FirstTargetSlot.ScreenPosition,
		TEXT("CardDrop{Intent=PlayCardCardTarget}"),
		Affordances);

	const FWacomFirstPersonCardSlotAutomationTestView FirstView =
		FWacomFirstPersonCardLayerTestAccess::View(*FirstTargetWidget);
	const FWacomFirstPersonCardSlotAutomationTestView SecondView =
		FWacomFirstPersonCardLayerTestAccess::View(*SecondTargetWidget);
	TestTrue(TEXT("First target keeps valid affordance"), FirstView.bCardDragTargetAffordanceFeedback);
	TestTrue(TEXT("Second target keeps valid affordance"), SecondView.bCardDragTargetAffordanceFeedback);
	TestEqual(TEXT("First target affordance state"), FirstView.CardDragTargetAffordanceFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);
	TestEqual(TEXT("Second target affordance state"), SecondView.CardDragTargetAffordanceFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);
	TestTrue(TEXT("Pointer target gets focus"), FirstView.bCardDragTargetFocusActive);
	TestFalse(TEXT("Other valid affordance does not get focus"), SecondView.bCardDragTargetFocusActive);
	TestTrue(TEXT("Focused target applies lift"),
		FMath::IsNearlyEqual(
			FirstTargetWidget->GetVisualSlotView().ScreenPosition.Y,
			FirstBaseVisual.ScreenPosition.Y - VisualConfig.DragCardTargetFocusLiftPixels));
	TestTrue(TEXT("Focused target applies scale"),
		FMath::IsNearlyEqual(
			FirstTargetWidget->GetVisualSlotView().RenderScale,
			FirstBaseVisual.RenderScale * VisualConfig.DragCardTargetFocusScale));
	TestEqual(TEXT("Focused target raises z-order"),
		Layer->GetCardZOrderAt(1),
		FirstBaseZOrder + VisualConfig.DragCardTargetFocusZOrderBoost);
	TestTrue(TEXT("Other target keeps base visual position"),
		FMath::IsNearlyEqual(SecondTargetWidget->GetVisualSlotView().ScreenPosition.Y, SecondBaseVisual.ScreenPosition.Y));
	TestTrue(TEXT("Other target keeps base scale"),
		FMath::IsNearlyEqual(SecondTargetWidget->GetVisualSlotView().RenderScale, SecondBaseVisual.RenderScale));
	TestEqual(TEXT("Other target keeps base z-order"), Layer->GetCardZOrderAt(2), SecondBaseZOrder);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, SecondTargetSlot.ScreenPosition);
	const FWacomFirstPersonCardSlotAutomationTestView FirstAfterMove =
		FWacomFirstPersonCardLayerTestAccess::View(*FirstTargetWidget);
	const FWacomFirstPersonCardSlotAutomationTestView SecondAfterMove =
		FWacomFirstPersonCardLayerTestAccess::View(*SecondTargetWidget);
	TestTrue(TEXT("Old target keeps affordance after pointer leaves"), FirstAfterMove.bCardDragTargetAffordanceFeedback);
	TestFalse(TEXT("Old target loses focus after pointer leaves"), FirstAfterMove.bCardDragTargetFocusActive);
	TestTrue(TEXT("New pointer target keeps affordance"), SecondAfterMove.bCardDragTargetAffordanceFeedback);
	TestTrue(TEXT("New pointer target gains focus"), SecondAfterMove.bCardDragTargetFocusActive);
	TestEqual(TEXT("New target waits for HUD validation with probe focus"),
		SecondAfterMove.CardDragTargetFocusFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::CardProbe);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, FVector2D(320.0f, 600.0f));
	TestTrue(TEXT("Affordance remains after pointer leaves card body"),
		FWacomFirstPersonCardLayerTestAccess::View(*FirstTargetWidget).bCardDragTargetAffordanceFeedback);
	TestFalse(TEXT("Focus clears after pointer leaves card body"),
		FWacomFirstPersonCardLayerTestAccess::View(*SecondTargetWidget).bCardDragTargetFocusActive);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SourceWidget, FVector2D(320.0f, 600.0f));
	TestFalse(TEXT("Release clears first affordance"),
		FWacomFirstPersonCardLayerTestAccess::View(*FirstTargetWidget).bCardDragTargetAffordanceFeedback);
	TestFalse(TEXT("Release clears second affordance"),
		FWacomFirstPersonCardLayerTestAccess::View(*SecondTargetWidget).bCardDragTargetAffordanceFeedback);
	TestFalse(TEXT("Release clears focus"),
		FWacomFirstPersonCardLayerTestAccess::View(*SecondTargetWidget).bCardDragTargetFocusActive);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerBleedCardTargetProbeTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.FirstPersonCardLayerCardTargetProbeIgnoresTransparentBleed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerBleedCardTargetProbeTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*Layer, DragConfig);
	Layer->SetCardViewClass(UWacomFirstPersonCardLayerBleedFirstPersonCardViewProbe::StaticClass());
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid TargetCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView SourceSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(SourceCardId, true, true);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(SourceSlot, EWacomFirstPersonCardInteractionIntent::AimWorldTarget);
	SourceSlot.ScreenPosition = FVector2D(500.0f, 600.0f);
	SourceSlot.WidgetPosition = SourceSlot.ScreenPosition;
	SourceSlot.SnappedWidgetPosition = SourceSlot.ScreenPosition;

	FWacomFirstPersonCardLayerSlotView TargetSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(TargetCardId, true, true);
	TargetSlot.Index = 1;
	TargetSlot.ScreenPosition = FVector2D(650.0f, 600.0f);
	TargetSlot.WidgetPosition = TargetSlot.ScreenPosition;
	TargetSlot.SnappedWidgetPosition = TargetSlot.ScreenPosition;
	TargetSlot.RenderScale = 1.0f;
	TargetSlot.ZOrder = 1;
	Layer->SetCardSlots({ SourceSlot, TargetSlot });

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* TargetWidget = Layer->GetSlotWidgetAt(1);
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Target slot"), TargetWidget))
	{
		PC->Destroy();
		return false;
	}
	TargetWidget->SetDesiredSizeInViewport(FVector2D(392.0f, 516.0f));
	TargetWidget->TakeWidget();
	FWacomFirstPersonCardLayerTestAccess::SetLocalHitCanvasSizeOverride(*TargetWidget, FVector2D(392.0f, 516.0f));

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, FVector2D(830.0f, 600.0f));
	TestNotEqual(TEXT("Pointer inside bleed but outside body does not probe card target"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.CurrentTarget.TargetKind,
		EWacomInteractionTargetKind::Card);
	TestFalse(TEXT("Bleed-only pointer does not light target probe"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragProbeFeedback);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, FVector2D(650.0f, 600.0f));
	TestEqual(TEXT("Pointer inside body probes card target"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.CurrentTarget.TargetKind,
		EWacomInteractionTargetKind::Card);
	TestEqual(TEXT("Body pointer records target card id"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.CurrentTarget.CardInstanceId,
		TargetCardId);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerRotatedCardTargetProbeTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.CardTargetProbeUsesRotatedBodyHitBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerRotatedCardTargetProbeTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*Layer, DragConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid TargetCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView SourceSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(SourceCardId, true, true);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(SourceSlot, EWacomFirstPersonCardInteractionIntent::AimWorldTarget);
	SourceSlot.ScreenPosition = FVector2D(500.0f, 600.0f);
	SourceSlot.WidgetPosition = SourceSlot.ScreenPosition;
	SourceSlot.SnappedWidgetPosition = SourceSlot.ScreenPosition;

	FWacomFirstPersonCardLayerSlotView TargetSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(TargetCardId, 1, FVector2D(650.0f, 600.0f), 45.0f, 1.0f);
	Layer->SetCardSlots({ SourceSlot, TargetSlot });

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* TargetWidget = Layer->GetSlotWidgetAt(1);
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Target slot"), TargetWidget))
	{
		PC->Destroy();
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, FVector2D(760.0f, 780.0f));
	TestNotEqual(TEXT("Old axis-aligned target corner does not probe rotated target"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.CurrentTarget.TargetKind,
		EWacomInteractionTargetKind::Card);
	TestFalse(TEXT("Rejected rotated corner does not light target probe"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragProbeFeedback);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, FVector2D(742.0f, 692.0f));
	TestEqual(TEXT("Point inside rotated target body probes card target"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.CurrentTarget.TargetKind,
		EWacomInteractionTargetKind::Card);
	TestEqual(TEXT("Rotated target body records target card id"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.CurrentTarget.CardInstanceId,
		TargetCardId);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDragTargetFeedbackClearTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.ReleaseCancelLayerClearClearsDragTargetFeedback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDragTargetFeedbackClearTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid TargetCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView SourceSlot = WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(SourceCardId, true, true);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(SourceSlot, EWacomFirstPersonCardInteractionIntent::AimCardTarget);
	FWacomFirstPersonCardLayerSlotView TargetSlot = WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(TargetCardId, true, true);
	TargetSlot.Index = 1;
	Layer->SetCardLayerInteractionEnabled(true);
	Layer->SetCardSlots({ SourceSlot, TargetSlot });

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* TargetWidget = Layer->GetSlotWidgetAt(1);
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Target slot"), TargetWidget))
	{
		PC->Destroy();
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, FVector2D(540.0f, 590.0f));
	Layer->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle::ForCardTarget(TargetCardId, TargetWidget, TargetSlot.ScreenPosition),
		false,
		EWacomFirstPersonCardDragTargetFeedbackState::CardProbe,
		TargetSlot.ScreenPosition);
	TestTrue(TEXT("Probe starts before clear"), FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragProbeFeedback);
	TestTrue(TEXT("Focus starts before clear"), FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragTargetFocusActive);

	Layer->CancelCardDragGesture(true);
	TestFalse(TEXT("Probe clears on cancel"), FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragProbeFeedback);
	TestFalse(TEXT("Focus clears on cancel"), FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragTargetFocusActive);
	TestEqual(TEXT("Current drag resets on cancel"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.GestureState,
		EWacomFirstPersonCardGestureState::Idle);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerCardTargetFocusVisualTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.CardTargetFocusUsesIndependentVisualsWithoutHover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerCardTargetFocusVisualTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardInspectHoldDelaySeconds = 0.0f;
	DragConfig.CardDragStartThresholdPixels = 1.0f;
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*Layer, DragConfig);
	FWacomFirstPersonCardSlotMotionConfig MotionConfig;
	MotionConfig.bEnabled = false;
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, MotionConfig);
	FWacomFirstPersonCardSlotVisualConfig VisualConfig;
	FWacomFirstPersonCardLayerTestAccess::SetSlotVisualConfig(*Layer, VisualConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid TargetCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView SourceSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SourceCardId, 0, FVector2D(500.0f, 600.0f), 0.0f, 1.0f);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(SourceSlot, EWacomFirstPersonCardInteractionIntent::AimCardTarget);
	FWacomFirstPersonCardLayerSlotView TargetSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(TargetCardId, 1, FVector2D(650.0f, 600.0f), 0.0f, 1.0f);
	Layer->SetCardSlots({ SourceSlot, TargetSlot });

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* TargetWidget = Layer->GetSlotWidgetAt(1);
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Target slot"), TargetWidget))
	{
		PC->Destroy();
		return false;
	}

	WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver HoverReceiver;
	Layer->OnCardHoveredNative.AddRaw(&HoverReceiver, &WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver::HandleHovered);
	Layer->OnCardUnhoveredNative.AddRaw(&HoverReceiver, &WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver::HandleUnhovered);

	const FWacomFirstPersonCardLayerSlotView BaseVisual = TargetWidget->GetVisualSlotView();
	const int32 BaseZOrder = Layer->GetCardZOrderAt(1);

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, SourceSlot.ScreenPosition);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, TargetSlot.ScreenPosition);

	TestEqual(TEXT("Drag target focus does not broadcast hover"), HoverReceiver.HoverCount, 0);
	TestFalse(TEXT("Target card is not ordinary hovered"), TargetWidget->IsHoveredForFirstPersonLayer());
	TestFalse(TEXT("Layer does not mark hover id during drag focus"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).HoveredCardInstanceId.IsValid());
	TestTrue(TEXT("Probe target focus is active"), FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragTargetFocusActive);
	TestTrue(TEXT("Probe target applies focus lift"),
		FMath::IsNearlyEqual(
			TargetWidget->GetVisualSlotView().ScreenPosition.Y,
			BaseVisual.ScreenPosition.Y - VisualConfig.DragCardTargetFocusLiftPixels));
	TestTrue(TEXT("Probe target applies focus scale"),
		FMath::IsNearlyEqual(
			TargetWidget->GetVisualSlotView().RenderScale,
			BaseVisual.RenderScale * VisualConfig.DragCardTargetFocusScale));
	TestEqual(TEXT("Probe target raises canvas z-order"),
		Layer->GetCardZOrderAt(1),
		BaseZOrder + VisualConfig.DragCardTargetFocusZOrderBoost);

	Layer->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle::ForCardTarget(TargetCardId, TargetWidget, TargetSlot.ScreenPosition),
		false,
		EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget,
		TargetSlot.ScreenPosition,
		TEXT("CardDrop{Intent=PlayCardCardTarget Reject=InvalidTarget}"));
	TestTrue(TEXT("Invalid card target focus remains active"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragTargetFocusActive);
	TestTrue(TEXT("Invalid target keeps focus lift"),
		FMath::IsNearlyEqual(
			TargetWidget->GetVisualSlotView().ScreenPosition.Y,
			BaseVisual.ScreenPosition.Y - VisualConfig.DragCardTargetFocusLiftPixels));
	TestTrue(TEXT("Invalid target applies focus scale without probe scale"),
		FMath::IsNearlyEqual(
			TargetWidget->GetVisualSlotView().RenderScale,
			BaseVisual.RenderScale * VisualConfig.DragCardTargetFocusScale));
	TestEqual(TEXT("Invalid target keeps raised z-order"),
		Layer->GetCardZOrderAt(1),
		BaseZOrder + VisualConfig.DragCardTargetFocusZOrderBoost);
	TestEqual(TEXT("Drag target focus still does not broadcast hover"), HoverReceiver.HoverCount, 0);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SourceWidget, TargetSlot.ScreenPosition);
	TestFalse(TEXT("Release clears target focus"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragTargetFocusActive);
	TestTrue(TEXT("Release restores target visual position"),
		FMath::IsNearlyEqual(TargetWidget->GetVisualSlotView().ScreenPosition.Y, BaseVisual.ScreenPosition.Y));
	TestTrue(TEXT("Release restores target scale"),
		FMath::IsNearlyEqual(TargetWidget->GetVisualSlotView().RenderScale, BaseVisual.RenderScale));
	TestEqual(TEXT("Release restores target z-order"), Layer->GetCardZOrderAt(1), BaseZOrder);

	Layer->OnCardHoveredNative.RemoveAll(&HoverReceiver);
	Layer->OnCardUnhoveredNative.RemoveAll(&HoverReceiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDragTargetSuppressesOrdinaryHoverTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.DragCardTargetSuppressesOrdinaryHoverAnimations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDragTargetSuppressesOrdinaryHoverTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardInspectHoldDelaySeconds = 0.0f;
	DragConfig.CardDragStartThresholdPixels = 1.0f;
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*Layer, DragConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid TargetCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView SourceSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SourceCardId, 0, FVector2D(500.0f, 600.0f), 0.0f, 1.0f);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(SourceSlot, EWacomFirstPersonCardInteractionIntent::AimCardTarget);
	FWacomFirstPersonCardLayerSlotView TargetSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(TargetCardId, 1, FVector2D(650.0f, 600.0f), 0.0f, 1.0f);
	Layer->SetCardSlots({ SourceSlot, TargetSlot });

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* TargetWidget = Layer->GetSlotWidgetAt(1);
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Target slot"), TargetWidget))
	{
		PC->Destroy();
		return false;
	}

	WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver HoverReceiver;
	Layer->OnCardHoveredNative.AddRaw(&HoverReceiver, &WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver::HandleHovered);
	Layer->OnCardUnhoveredNative.AddRaw(&HoverReceiver, &WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver::HandleUnhovered);

	TestTrue(TEXT("Source can ordinary hover before drag"),
		FWacomFirstPersonCardLayerTestAccess::ResolveHoveredCardAtWidgetPosition(*Layer, SourceSlot.ScreenPosition) == SourceCardId);
	TestTrue(TEXT("Source starts ordinary hovered"), SourceWidget->IsHoveredForFirstPersonLayer());
	TestEqual(TEXT("Initial hover broadcasts once"), HoverReceiver.HoverCount, 1);

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, SourceSlot.ScreenPosition);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, TargetSlot.ScreenPosition);
	TestFalse(TEXT("Drag start clears source ordinary hover"), SourceWidget->IsHoveredForFirstPersonLayer());
	TestFalse(TEXT("Drag start clears layer hover id"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).HoveredCardInstanceId.IsValid());
	TestEqual(TEXT("Clearing source hover broadcasts unhover"), HoverReceiver.UnhoverCount, 1);
	TestTrue(TEXT("Target focus is active from card probe"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragTargetFocusActive);

	const int32 HoverCountAfterDragProbe = HoverReceiver.HoverCount;
	TestTrue(TEXT("Pointer enter on target during drag routes to active gesture"),
		FWacomFirstPersonCardLayerTestAccess::HandleSlotPointerEnteredAtWidgetPosition(
			*Layer,
			*TargetWidget,
			TargetSlot.ScreenPosition));
	TestFalse(TEXT("Target does not become ordinary hovered during drag"), TargetWidget->IsHoveredForFirstPersonLayer());
	TestFalse(TEXT("Layer still has no ordinary hover id during drag"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).HoveredCardInstanceId.IsValid());
	TestEqual(TEXT("Target pointer enter during drag does not broadcast hover"),
		HoverReceiver.HoverCount,
		HoverCountAfterDragProbe);
	TestTrue(TEXT("Target keeps drag focus without ordinary hover"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragTargetFocusActive);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SourceWidget, TargetSlot.ScreenPosition);
	TestFalse(TEXT("Release does not immediately hover target"), TargetWidget->IsHoveredForFirstPersonLayer());
	TestFalse(TEXT("Release keeps ordinary hover id clear until next pointer move"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).HoveredCardInstanceId.IsValid());

	Layer->OnCardHoveredNative.RemoveAll(&HoverReceiver);
	Layer->OnCardUnhoveredNative.RemoveAll(&HoverReceiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerCardTargetFocusClearsWhenLeavingBodyTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.CardTargetFocusClearsWhenPointerLeavesBody",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerCardTargetFocusClearsWhenLeavingBodyTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardInspectHoldDelaySeconds = 0.0f;
	DragConfig.CardDragStartThresholdPixels = 1.0f;
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*Layer, DragConfig);
	FWacomFirstPersonCardSlotMotionConfig MotionConfig;
	MotionConfig.bEnabled = false;
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, MotionConfig);
	FWacomFirstPersonCardSlotVisualConfig VisualConfig;
	FWacomFirstPersonCardLayerTestAccess::SetSlotVisualConfig(*Layer, VisualConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid TargetCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView SourceSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SourceCardId, 0, FVector2D(500.0f, 600.0f), 0.0f, 1.0f);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(SourceSlot, EWacomFirstPersonCardInteractionIntent::AimCardTarget);
	FWacomFirstPersonCardLayerSlotView TargetSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(TargetCardId, 1, FVector2D(650.0f, 600.0f), 0.0f, 1.0f);
	Layer->SetCardSlots({ SourceSlot, TargetSlot });

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* TargetWidget = Layer->GetSlotWidgetAt(1);
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Target slot"), TargetWidget))
	{
		PC->Destroy();
		return false;
	}

	const FWacomFirstPersonCardLayerSlotView BaseVisual = TargetWidget->GetVisualSlotView();
	const int32 BaseZOrder = Layer->GetCardZOrderAt(1);

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, SourceSlot.ScreenPosition);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, TargetSlot.ScreenPosition);
	Layer->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle::ForCardTarget(TargetCardId, TargetWidget, TargetSlot.ScreenPosition),
		true,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget,
		TargetSlot.ScreenPosition,
		TEXT("CardDrop{Intent=PlayCardCardTarget}"));
	TestTrue(TEXT("Target focus starts active"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragTargetFocusActive);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, FVector2D(1200.0f, 600.0f));
	TestEqual(TEXT("Leaving card body clears card target kind"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.CurrentTarget.TargetKind,
		EWacomInteractionTargetKind::None);
	TestEqual(TEXT("Leaving card body clears feedback state"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.TargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::None);
	TestFalse(TEXT("Leaving card body clears target focus"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragTargetFocusActive);
	TestTrue(TEXT("Leaving card body restores target visual position"),
		FMath::IsNearlyEqual(TargetWidget->GetVisualSlotView().ScreenPosition.Y, BaseVisual.ScreenPosition.Y));
	TestTrue(TEXT("Leaving card body restores target scale"),
		FMath::IsNearlyEqual(TargetWidget->GetVisualSlotView().RenderScale, BaseVisual.RenderScale));
	TestEqual(TEXT("Leaving card body restores target z-order"), Layer->GetCardZOrderAt(1), BaseZOrder);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SourceWidget, FVector2D(1200.0f, 600.0f));
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDragTargetDebugSummaryTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.DebugSummaryReportsDragTargetFeedbackState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDragTargetDebugSummaryTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot = WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(Slot, EWacomFirstPersonCardInteractionIntent::AimWorldTarget);
	Layer->SetCardLayerInteractionEnabled(true);
	Layer->SetCardSlots({ Slot });
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(540.0f, 590.0f));
	Layer->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle::ForWorldTarget(FGuid::NewGuid(), PC, FVector::ZeroVector, FVector2D(700.0f, 420.0f)),
		true,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget,
		FVector2D(700.0f, 420.0f),
		TEXT("CardDrop{Intent=PlayCardWorldTarget Reject=None}"));

	const FString Summary = Layer->GetDragTargetDebugSummary();
	TestTrue(TEXT("Summary reports drag target section"), Summary.Contains(TEXT("DragTarget")));
	TestTrue(TEXT("Summary reports target position"), Summary.Contains(TEXT("HasTargetPos=true")));
	TestTrue(TEXT("Summary reports valid flag"), Summary.Contains(TEXT("Valid=true")));
	TestTrue(TEXT("Summary reports resolved intent"), Summary.Contains(TEXT("PlayCardWorldTarget")));

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, FVector2D(540.0f, 590.0f));
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerValidCardTargetFeedbackTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.ValidCardTargetsUseValidCardFeedback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerValidCardTargetFeedbackTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*Layer, DragConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid ValidTargetCardId = FGuid::NewGuid();
	const FGuid InvalidTargetCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView SourceSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(SourceCardId, true, true);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(SourceSlot, EWacomFirstPersonCardInteractionIntent::AimCardTarget);
	FWacomFirstPersonCardLayerSlotView ValidTargetSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(ValidTargetCardId, true, true);
	ValidTargetSlot.Index = 1;
	ValidTargetSlot.ScreenPosition = FVector2D(650.0f, 600.0f);
	FWacomFirstPersonCardLayerSlotView InvalidTargetSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(InvalidTargetCardId, true, true);
	InvalidTargetSlot.Index = 2;
	InvalidTargetSlot.ScreenPosition = FVector2D(780.0f, 600.0f);
	Layer->SetCardSlots({ SourceSlot, ValidTargetSlot, InvalidTargetSlot });

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* ValidTargetWidget = Layer->GetSlotWidgetAt(1);
	UWacomFirstPersonCardLayerSlotWidget* InvalidTargetWidget = Layer->GetSlotWidgetAt(2);
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Valid target slot"), ValidTargetWidget)
		|| !TestNotNull(TEXT("Invalid target slot"), InvalidTargetWidget))
	{
		PC->Destroy();
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, FVector2D(540.0f, 590.0f));

	TArray<FWacomFirstPersonCardTargetAffordance> Affordances;
	FWacomFirstPersonCardTargetAffordance ValidAffordance;
	ValidAffordance.CardInstanceId = ValidTargetCardId;
	ValidAffordance.FeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget;
	ValidAffordance.bCanSubmit = true;
	Affordances.Add(ValidAffordance);
	FWacomFirstPersonCardTargetAffordance InvalidAffordance;
	InvalidAffordance.CardInstanceId = InvalidTargetCardId;
	InvalidAffordance.FeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget;
	InvalidAffordance.bCanSubmit = false;
	Affordances.Add(InvalidAffordance);

	Layer->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle::ForCardTarget(ValidTargetCardId, ValidTargetWidget, ValidTargetSlot.ScreenPosition),
		true,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget,
		ValidTargetSlot.ScreenPosition,
		TEXT("CardDrop{Intent=PlayCardCardTarget}"),
		Affordances);

	TestEqual(TEXT("Valid target uses card valid state"),
		FWacomFirstPersonCardLayerTestAccess::View(*ValidTargetWidget).DragTargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);
	TestEqual(TEXT("Valid target records affordance state"),
		FWacomFirstPersonCardLayerTestAccess::View(*ValidTargetWidget).CardDragTargetAffordanceFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);
	TestTrue(TEXT("Focused valid target receives focus"),
		FWacomFirstPersonCardLayerTestAccess::View(*ValidTargetWidget).bCardDragTargetFocusActive);
	TestEqual(TEXT("Invalid target uses card invalid state"),
		FWacomFirstPersonCardLayerTestAccess::View(*InvalidTargetWidget).DragTargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget);
	TestEqual(TEXT("Invalid target records affordance state"),
		FWacomFirstPersonCardLayerTestAccess::View(*InvalidTargetWidget).CardDragTargetAffordanceFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget);
	TestFalse(TEXT("Non-pointer invalid affordance does not receive focus"),
		FWacomFirstPersonCardLayerTestAccess::View(*InvalidTargetWidget).bCardDragTargetFocusActive);
	TestTrue(TEXT("Debug counts valid affordance"), Layer->GetDragTargetDebugSummary().Contains(TEXT("AffordanceValid=1")));
	TestTrue(TEXT("Debug counts invalid affordance"), Layer->GetDragTargetDebugSummary().Contains(TEXT("AffordanceInvalid=1")));

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SourceWidget, FVector2D(540.0f, 590.0f));
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerFocusedCardTargetOverrideTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.CurrentHoveredCardTargetOverridesWithStrongerFeedback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerFocusedCardTargetOverrideTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*Layer, DragConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid TargetCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView SourceSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(SourceCardId, true, true);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(SourceSlot, EWacomFirstPersonCardInteractionIntent::AimCardTarget);
	FWacomFirstPersonCardLayerSlotView TargetSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(TargetCardId, true, true);
	TargetSlot.Index = 1;
	TargetSlot.ScreenPosition = FVector2D(650.0f, 600.0f);
	Layer->SetCardSlots({ SourceSlot, TargetSlot });

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* TargetWidget = Layer->GetSlotWidgetAt(1);
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Target slot"), TargetWidget))
	{
		PC->Destroy();
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, FVector2D(540.0f, 590.0f));

	FWacomFirstPersonCardTargetAffordance InvalidAffordance;
	InvalidAffordance.CardInstanceId = TargetCardId;
	InvalidAffordance.FeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget;
	TArray<FWacomFirstPersonCardTargetAffordance> Affordances;
	Affordances.Add(InvalidAffordance);
	Layer->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle::ForCardTarget(TargetCardId, TargetWidget, TargetSlot.ScreenPosition),
		true,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget,
		TargetSlot.ScreenPosition,
		TEXT("CardDrop{Intent=PlayCardCardTarget}"),
		Affordances);

	TestEqual(TEXT("Focused valid result overrides base invalid affordance"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).DragTargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);
	TestEqual(TEXT("Base invalid affordance is preserved separately"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).CardDragTargetAffordanceFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget);
	TestEqual(TEXT("Focused valid result is stored separately"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).CardDragTargetFocusFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);
	TestTrue(TEXT("Focused target receives focus"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragTargetFocusActive);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SourceWidget, FVector2D(540.0f, 590.0f));
	PC->Destroy();
	return true;
}
