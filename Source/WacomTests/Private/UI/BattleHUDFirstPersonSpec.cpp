// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Components/WacomBattleCameraLookComponent.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Fixtures/BattleTestFixtures.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "BattleHUDTestHarness.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomBattleHUDFirstPersonSpec
{
	UWorld* FindAutomationWorld()
	{
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (UWorld* World = Context.World())
				{
					return World;
				}
			}
		}

		return GWorld;
	}

	FGuid FindFirstHandCardByTargetMode(const FBattleSnapshot& Snapshot, ECardTargetMode TargetMode)
	{
		for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
		{
			if (Card.Definition && Card.Definition->TargetMode == TargetMode)
			{
				return Card.InstanceId;
			}
		}
		return FGuid();
	}

	FWacomFirstPersonCardDragView MakeCommitDragView(const FGuid& CardInstanceId)
	{
		FWacomFirstPersonCardDragView DragView;
		DragView.CardInstanceId = CardInstanceId;
		DragView.GestureState = EWacomFirstPersonCardGestureState::ArmedForCommit;
		DragView.bCommitArmed = true;
		DragView.PressScreenPosition = FVector2D(500.0f, 600.0f);
		DragView.CurrentScreenPosition = FVector2D(540.0f, 590.0f);
		DragView.PointerViewportPosition = DragView.CurrentScreenPosition;
		DragView.PointerNormalizedViewportPosition = FVector2D(0.65f, 0.42f);
		DragView.bHasPointerViewportPosition = true;
		return DragView;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDFirstPersonHandBridgeContractSpec,
	"Wacom.UI.Battle.BattleHUD.HandPresentation.FirstPersonBridgeCleansRuntimeStateOnClear",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDFirstPersonHandBridgeContractSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleHUDFirstPersonSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ TargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 5, 0), 1);

	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get())
		|| !TestNotNull(TEXT("PlayerController"), Harness->PlayerController()))
	{
		return false;
	}
	AWacomPlayerCharacter* Character = Harness->AttachFirstPersonCharacter();
	if (!TestNotNull(TEXT("Character"), Character))
	{
		return false;
	}

	UWacomFirstPersonCardAnchorComponent* Anchor = Harness->FirstPersonAnchor();
	UWacomBattleCameraLookComponent* BattleCamera = Harness->BattleCameraLook();
	if (!TestNotNull(TEXT("First-person card anchor"), Anchor)
		|| !TestNotNull(TEXT("Battle camera look"), BattleCamera))
	{
		return false;
	}

	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	Harness->SetSession(Session);
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomBattleHUDFirstPersonSpec::FindFirstHandCardByTargetMode(
		Snapshot,
		ECardTargetMode::SingleEnemyPart);
	if (!TestTrue(TEXT("Target card exists"), CardId.IsValid()))
	{
		return false;
	}

	HUD->SyncFirstPersonBattleHandLayerForTest(Snapshot);
	TestTrue(TEXT("HUD bridge writes runtime hand to anchor"), Anchor->HasRuntimeCardLayerData());
	TestTrue(TEXT("HUD bridge enables first-person hand interaction"),
		Anchor->IsFirstPersonCardLayerInteractionEnabled());

	TestTrue(TEXT("Battle camera activates for drag override"), BattleCamera->ActivateBattleCameraLook());
	FWacomFirstPersonCardPointerView PointerView;
	PointerView.CardInstanceId = CardId;
	PointerView.bHasPointerViewportPosition = true;
	PointerView.PointerNormalizedViewportPosition = FVector2D(0.35f, -0.45f);
	HUD->HandleFirstPersonCardPointerMovedForTest(PointerView);
	TestTrue(TEXT("Hover pointer writes camera look override"), BattleCamera->HasCursorLookOverrideForTest());
	TestEqual(
		TEXT("Hover pointer override stores normalized pointer"),
		BattleCamera->GetCursorLookOverrideNormalizedForTest(),
		FVector2D(0.35f, -0.45f));
	HUD->HandleFirstPersonCardPointerLeftForTest();
	TestFalse(TEXT("Hover pointer leave clears camera look override"), BattleCamera->HasCursorLookOverrideForTest());

	FWacomFirstPersonCardDragView DragView = WacomBattleHUDFirstPersonSpec::MakeCommitDragView(CardId);
	HUD->HandleFirstPersonCardDragStartedForTest(CardId, DragView);
	HUD->HandleFirstPersonCardDragUpdatedForTest(CardId, DragView);
	TestTrue(TEXT("Drag update writes camera look override"), BattleCamera->HasCursorLookOverrideForTest());

	HUD->ClearFirstPersonBattleHandLayerForTest();
	TestFalse(TEXT("HUD bridge clear removes runtime hand"), Anchor->HasRuntimeCardLayerData());
	TestFalse(TEXT("HUD bridge clear disables interaction"), Anchor->IsFirstPersonCardLayerInteractionEnabled());
	TestFalse(TEXT("HUD bridge clear removes camera look override"), BattleCamera->HasCursorLookOverrideForTest());
	TestFalse(TEXT("HUD bridge clear hides first-person detail"),
		HUD->IsFirstPersonCardDetailPanelVisibleForTest());

	const int32 VersionBeforeStaleDelegate = Session->BuildSnapshot().Version;
	FWacomFirstPersonCardDragView StaleDragView = WacomBattleHUDFirstPersonSpec::MakeCommitDragView(CardId);
	Anchor->OnFirstPersonCardLayerDragReleased.Broadcast(CardId, StaleDragView);
	TestEqual(TEXT("Cleared bridge unbinds anchor drag delegate"),
		Session->BuildSnapshot().Version,
		VersionBeforeStaleDelegate);
	TestEqual(TEXT("Cleared bridge does not set target select"),
		HUD->GetUIState(),
		EBattleUIState::Idle);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDCardDetailControllerContractSpec,
	"Wacom.UI.Battle.BattleHUD.CardDetail.ControllerUsesFirstPersonViewportOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDCardDetailControllerContractSpec::RunTest(const FString& /*Parameters*/)
{
	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDOnly(WacomBattleHUDFirstPersonSpec::FindAutomationWorld());
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get())
		|| !TestNotNull(TEXT("HUD"), Harness->HUD()))
	{
		return false;
	}
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	TStrongObjectPtr<UCardDefinition> FirstPersonCard(NewObject<UCardDefinition>());
	FirstPersonCard->CardId = TEXT("Contract.FirstPerson.Detail");
	FirstPersonCard->DisplayName = FText::FromString(TEXT("第一人称详情合同卡"));
	FHandCardSnapshot FirstPersonSnap;
	FirstPersonSnap.InstanceId = FGuid::NewGuid();
	FirstPersonSnap.Definition = FirstPersonCard.Get();
	FirstPersonSnap.RuntimeCost = 1;
	FirstPersonSnap.bIsPlayable = true;

	FBattleSnapshot Snapshot;
	Snapshot.Phase = EBattlePhase::PlayerAction;
	Snapshot.Hand.Cards.Add(FirstPersonSnap);
	Snapshot.Hand.NormalCardCount = 1;
	HUD->RefreshFromSnapshotForTest(Snapshot);

	FWacomFirstPersonCardLayerSlotView SlotView;
	SlotView.Entry.CardInstanceId = FirstPersonSnap.InstanceId;
	SlotView.ScreenPosition = FVector2D(700.0f, 520.0f);
	SlotView.RenderScale = 1.0f;
	SlotView.RenderOpacity = 1.0f;
	SlotView.bProjected = true;
	HUD->HandleFirstPersonCardHoveredForTest(FirstPersonSnap.InstanceId, SlotView);
	HUD->TickCardDetailMotionForTest(0.12f);
	TestTrue(TEXT("First-person detail is visible through HUD wrapper"),
		HUD->IsFirstPersonCardDetailPanelVisibleForTest());
	TestEqual(TEXT("First-person detail name is exposed through HUD wrapper"),
		HUD->GetFirstPersonCardDetailPanelNameTextForTest().ToString(),
		FString(TEXT("第一人称详情合同卡")));

	HUD->HideCardDetailForTest();
	TestFalse(TEXT("HUD hide clears first-person detail host"),
		HUD->IsFirstPersonCardDetailPanelVisibleForTest());
	TestFalse(TEXT("HUD hide reports no visible detail"),
		HUD->IsCardDetailPanelVisible());

	HUD->HandleFirstPersonCardHoveredForTest(FirstPersonSnap.InstanceId, SlotView);
	HUD->TickCardDetailMotionForTest(0.12f);
	TestTrue(TEXT("First-person detail can show again before BattleEnd"),
		HUD->IsFirstPersonCardDetailPanelVisibleForTest());

	FBattleSnapshot BattleEndSnapshot = Snapshot;
	BattleEndSnapshot.Phase = EBattlePhase::BattleEnd;
	HUD->RefreshFromSnapshotForTest(BattleEndSnapshot);
	TestFalse(TEXT("BattleEnd refresh clears first-person detail"),
		HUD->IsFirstPersonCardDetailPanelVisibleForTest());
	TestFalse(TEXT("BattleEnd refresh reports no visible detail"),
		HUD->IsCardDetailPanelVisible());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDCardDetailReadabilityMotionSpec,
	"Wacom.UI.Battle.BattleHUD.CardDetail.FirstPersonReadabilityMotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDCardDetailReadabilityMotionSpec::RunTest(const FString& /*Parameters*/)
{
	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDOnly(WacomBattleHUDFirstPersonSpec::FindAutomationWorld());
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get())
		|| !TestNotNull(TEXT("HUD"), Harness->HUD()))
	{
		return false;
	}
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	Card->CardId = TEXT("BattleDetailMotionCard");
	Card->DisplayName = FText::FromString(TEXT("详情动效卡"));

	FHandCardSnapshot Snap;
	Snap.InstanceId = FGuid::NewGuid();
	Snap.Definition = Card.Get();
	Snap.RuntimeCost = 1;
	Snap.bIsPlayable = true;

	FBattleSnapshot BattleSnapshot;
	BattleSnapshot.Phase = EBattlePhase::PlayerAction;
	BattleSnapshot.Hand.Cards.Add(Snap);
	BattleSnapshot.Hand.NormalCardCount = 1;

	FWacomFirstPersonCardLayerSlotView SlotView;
	SlotView.Entry.CardInstanceId = Snap.InstanceId;
	SlotView.ScreenPosition = FVector2D(700.0f, 520.0f);
	SlotView.RenderScale = 1.0f;
	SlotView.RenderOpacity = 1.0f;
	SlotView.bProjected = true;

	HUD->TakeWidget();
	HUD->RefreshFromSnapshotForTest(BattleSnapshot);

	HUD->HandleFirstPersonCardHoveredForTest(Snap.InstanceId, SlotView);
	TestFalse(TEXT("Initial hover waits for delay"), HUD->IsCardDetailPanelVisible());
	HUD->TickCardDetailMotionForTest(0.05f);
	TestFalse(TEXT("Detail is still hidden before delay finishes"), HUD->IsCardDetailPanelVisible());
	HUD->HandleFirstPersonCardUnhoveredForTest(Snap.InstanceId, SlotView);
	HUD->TickCardDetailMotionForTest(0.20f);
	TestFalse(TEXT("Hover leave before delay cancels detail"), HUD->IsCardDetailPanelVisible());

	HUD->SetCardDetailReadabilityPolishForTest(false);
	HUD->HandleFirstPersonCardHoveredForTest(Snap.InstanceId, SlotView);
	TestTrue(TEXT("Motion disabled shows immediately"), HUD->IsCardDetailPanelVisible());
	HUD->HandleFirstPersonCardUnhoveredForTest(Snap.InstanceId, SlotView);
	TestFalse(TEXT("Motion disabled hides immediately"), HUD->IsCardDetailPanelVisible());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDFirstPersonInspectDetailUnhoverGuardSpec,
	"Wacom.UI.Battle.BattleHUD.CardDetail.FirstPersonInspectUnhoverGuard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDFirstPersonInspectDetailUnhoverGuardSpec::RunTest(const FString& /*Parameters*/)
{
	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDOnly(WacomBattleHUDFirstPersonSpec::FindAutomationWorld());
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get())
		|| !TestNotNull(TEXT("HUD"), Harness->HUD()))
	{
		return false;
	}
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	Card->CardId = TEXT("FirstPersonInspectDetailCard");
	Card->DisplayName = FText::FromString(TEXT("读牌详情卡"));

	FHandCardSnapshot Snap;
	Snap.InstanceId = FGuid::NewGuid();
	Snap.Definition = Card.Get();
	Snap.RuntimeCost = 1;
	Snap.bIsPlayable = true;

	FBattleSnapshot BattleSnapshot;
	BattleSnapshot.Phase = EBattlePhase::PlayerAction;
	BattleSnapshot.Hand.Cards.Add(Snap);
	BattleSnapshot.Hand.NormalCardCount = 1;

	HUD->TakeWidget();
	HUD->RefreshFromSnapshotForTest(BattleSnapshot);

	FWacomFirstPersonCardLayerSlotView HoverSlot;
	HoverSlot.Entry.CardInstanceId = Snap.InstanceId;
	HoverSlot.ScreenPosition = FVector2D(500.0f, 600.0f);
	HoverSlot.RenderScale = 1.0f;
	HoverSlot.RenderOpacity = 1.0f;
	HoverSlot.bProjected = true;
	HUD->HandleFirstPersonCardHoveredForTest(Snap.InstanceId, HoverSlot);
	HUD->TickCardDetailMotionForTest(0.12f);
	TestTrue(TEXT("First-person hover detail is visible"), HUD->IsFirstPersonCardDetailPanelVisibleForTest());

	FWacomFirstPersonCardLayerSlotView InspectSlot = HoverSlot;
	InspectSlot.ScreenPosition = FVector2D(960.0f, 496.0f);
	InspectSlot.RenderScale = 1.18f;
	InspectSlot.GestureState = EWacomFirstPersonCardGestureState::Inspecting;
	HUD->HandleFirstPersonCardLayoutUpdatedForTest(Snap.InstanceId, InspectSlot);
	HUD->TickCardDetailMotionForTest(0.02f);

	HUD->HandleFirstPersonCardUnhoveredForTest(Snap.InstanceId, HoverSlot);
	HUD->TickCardDetailMotionForTest(0.5f);
	TestTrue(TEXT("Inspect detail survives same-card hover loss"), HUD->IsFirstPersonCardDetailPanelVisibleForTest());
	TestEqual(TEXT("Inspect detail keeps card data"),
		HUD->GetFirstPersonCardDetailPanelNameTextForTest().ToString(),
		TEXT("读牌详情卡"));

	return true;
}
