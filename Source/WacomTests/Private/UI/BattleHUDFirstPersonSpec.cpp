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
#include "UI/Card/WacomCardPresentationBuilder.h"
#include "UI/FirstPersonCardLayerTestAccess.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "UI/Card/WacomFirstPersonCardLayerSourceIds.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
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

	UCardDefinition* MakePreviewCard(UObject* Outer, const TCHAR* Name, int32 Cost)
	{
		UCardDefinition* Card = NewObject<UCardDefinition>(Outer);
		if (!Card)
		{
			return nullptr;
		}

		Card->CardId = FName(Name);
		Card->DisplayName = FText::FromString(Name);
		Card->Description = FText::FromString(TEXT("Battle HUD first-person card"));
		Card->BaseCost = Cost;
		return Card;
	}

	UBattleSession* CreateMinimalSession(FWacomBattleFixture& Fixture)
	{
		UCharacterDefinition* CharacterDefinition = Fixture.MakeCharacter(
			Fixture.MakeNoopCard(0),
			Fixture.MakeNoopCard(0),
			{ Fixture.MakeNoopCard(0) });
		return Fixture.CreateSession(CharacterDefinition, Fixture.MakeSinglePartEnemy(20, 5, 0), 1);
	}

	FBattleSnapshot MakeSnapshotWithHand(const TArray<FHandCardSnapshot>& Cards)
	{
		FBattleSnapshot Snapshot;
		Snapshot.Phase = EBattlePhase::PlayerAction;
		Snapshot.Hand.Cards = Cards;
		Snapshot.Hand.NormalCardCount = Cards.Num();
		return Snapshot;
	}

	FHandCardSnapshot MakeHandCardSnapshot(
		UCardDefinition* Card,
		int32 RuntimeCost,
		bool bPlayable)
	{
		FHandCardSnapshot Snapshot;
		Snapshot.InstanceId = FGuid::NewGuid();
		Snapshot.Definition = Card;
		Snapshot.RuntimeCost = RuntimeCost;
		Snapshot.Zone = EHandZone::Both;
		Snapshot.bIsPlayable = bPlayable;
		return Snapshot;
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

	FWacomFirstPersonCardLayerSlotView MakeProjectedShortcutSlot(
		const FGuid& CardInstanceId,
		int32 Index)
	{
		FWacomFirstPersonCardLayerSlotView Slot;
		Slot.Index = Index;
		Slot.Entry.CardInstanceId = CardInstanceId;
		Slot.Entry.bIsPlayable = true;
		Slot.Entry.InteractionIntent = EWacomFirstPersonCardInteractionIntent::CommitNoTarget;
		Slot.ScreenPosition = FVector2D(500.0f + 42.0f * static_cast<float>(Index), 600.0f);
		Slot.WidgetPosition = Slot.ScreenPosition;
		Slot.SnappedWidgetPosition = Slot.ScreenPosition;
		Slot.InputHitCenter = Slot.ScreenPosition;
		Slot.InputHitScale = 1.0f;
		Slot.InputHitOrder = Index;
		Slot.RenderScale = 0.55f;
		Slot.RenderOpacity = 1.0f;
		Slot.ZOrder = Index;
		Slot.bProjected = true;
		return Slot;
	}

	FWacomFirstPersonCardLayerSlotView MakeProjectedDetailSlot(
		const FGuid& CardInstanceId,
		const FVector2D& ScreenPosition = FVector2D(700.0f, 520.0f))
	{
		FWacomFirstPersonCardLayerSlotView Slot;
		Slot.Entry.CardInstanceId = CardInstanceId;
		Slot.ScreenPosition = ScreenPosition;
		Slot.WidgetPosition = ScreenPosition;
		Slot.SnappedWidgetPosition = ScreenPosition;
		Slot.InputHitCenter = ScreenPosition;
		Slot.RenderScale = 1.0f;
		Slot.RenderOpacity = 1.0f;
		Slot.bProjected = true;
		return Slot;
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

	FBattleSnapshot BattleEndSnapshot = Snapshot;
	BattleEndSnapshot.Phase = EBattlePhase::BattleEnd;
	HUD->SyncFirstPersonBattleHandLayerForTest(BattleEndSnapshot);
	TestFalse(TEXT("BattleEnd clears first-person battle hand source"), Anchor->HasRuntimeCardLayerData());

	HUD->SyncFirstPersonBattleHandLayerForTest(Snapshot);
	TestTrue(TEXT("HUD bridge can restore runtime hand after BattleEnd clear"), Anchor->HasRuntimeCardLayerData());
	TestTrue(TEXT("HUD bridge restores first-person hand interaction"),
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
	FWacomUIBattleHUDFirstPersonHandStateSpec,
	"Wacom.UI.Battle.BattleHUD.HandPresentation.RuntimeEntriesPreserveSnapshotState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDFirstPersonHandStateSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleHUDFirstPersonSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UBattleSession* Session = WacomBattleHUDFirstPersonSpec::CreateMinimalSession(Fx);
	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	if (!TestNotNull(TEXT("Session"), Session)
		|| !TestNotNull(TEXT("HUD harness"), Harness.Get()))
	{
		return false;
	}

	AWacomPlayerCharacter* Character = Harness->AttachFirstPersonCharacter();
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	UWacomFirstPersonCardAnchorComponent* Anchor = Harness->FirstPersonAnchor();
	if (!TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("First-person card anchor"), Anchor))
	{
		return false;
	}

	Harness->SetSession(Session);
	UCardDefinition* FirstCard = WacomBattleHUDFirstPersonSpec::MakePreviewCard(
		GetTransientPackage(),
		TEXT("Battle.Alpha"),
		1);
	UCardDefinition* SecondCard = WacomBattleHUDFirstPersonSpec::MakePreviewCard(
		GetTransientPackage(),
		TEXT("Battle.Beta"),
		2);
	if (!TestNotNull(TEXT("First card"), FirstCard)
		|| !TestNotNull(TEXT("Second card"), SecondCard))
	{
		return false;
	}

	FHandCardSnapshot FirstSnapshot =
		WacomBattleHUDFirstPersonSpec::MakeHandCardSnapshot(FirstCard, 7, true);
	FirstSnapshot.Zone = EHandZone::Left;
	FHandCardSnapshot SecondSnapshot =
		WacomBattleHUDFirstPersonSpec::MakeHandCardSnapshot(SecondCard, 9, false);
	SecondSnapshot.Zone = EHandZone::Right;
	const FBattleSnapshot Snapshot =
		WacomBattleHUDFirstPersonSpec::MakeSnapshotWithHand({ FirstSnapshot, SecondSnapshot });

	HUD->SyncFirstPersonBattleHandLayerForTest(Snapshot);

	TestTrue(TEXT("Runtime hand source is active"), Anchor->HasRuntimeCardLayerData());
	TestEqual(
		TEXT("Runtime hand source id"),
		Anchor->GetRuntimeCardLayerSourceId(),
		WacomFirstPersonCardLayerSourceIds::BattleHand());
	TestEqual(TEXT("Runtime hand card count"), Anchor->GetRuntimeCardLayerCardCount(), 2);
	const TArray<FWacomFirstPersonCardLayerEntry>& RuntimeEntries = Anchor->GetRuntimeCardLayerEntries();
	TestEqual(TEXT("Runtime entry count"), RuntimeEntries.Num(), 2);
	if (RuntimeEntries.Num() == 2)
	{
		TestEqual(TEXT("First identity preserves hand order"), RuntimeEntries[0].CardInstanceId, FirstSnapshot.InstanceId);
		TestEqual(TEXT("Second identity preserves hand order"), RuntimeEntries[1].CardInstanceId, SecondSnapshot.InstanceId);
		TestEqual(TEXT("First zone is preserved"), RuntimeEntries[0].Zone, EHandZone::Left);
		TestEqual(TEXT("Second zone is preserved"), RuntimeEntries[1].Zone, EHandZone::Right);
		TestFalse(TEXT("First card is not anchor"), RuntimeEntries[0].bIsHandAnchor);
		TestFalse(TEXT("Second card is not anchor"), RuntimeEntries[1].bIsHandAnchor);
		TestEqual(TEXT("First card view preserves hand order"), RuntimeEntries[0].CardViewData.Name.ToString(), FString(TEXT("Battle.Alpha")));
		TestEqual(TEXT("Runtime cost overrides base cost"), RuntimeEntries[0].CardViewData.Cost, 7);
		TestFalse(TEXT("Playable card is not disabled"), RuntimeEntries[0].CardViewData.bDisabled);
		TestEqual(TEXT("Second runtime cost overrides base cost"), RuntimeEntries[1].CardViewData.Cost, 9);
		TestTrue(TEXT("Unplayable card is disabled"), RuntimeEntries[1].CardViewData.bDisabled);
		TestFalse(TEXT("First card is not pending by default"), RuntimeEntries[0].bIsPendingTargeting);
		TestFalse(TEXT("Second card is not pending by default"), RuntimeEntries[1].bIsPendingTargeting);
	}

	HUD->SetTargetSelectionStateForTest(SecondSnapshot.InstanceId);
	HUD->SyncFirstPersonBattleHandLayerForTest(Snapshot);
	const TArray<FWacomFirstPersonCardLayerEntry>& PendingEntries = Anchor->GetRuntimeCardLayerEntries();
	TestEqual(TEXT("Entry count after target select"), PendingEntries.Num(), 2);
	if (PendingEntries.Num() == 2)
	{
		TestFalse(TEXT("Non-pending card remains normal"), PendingEntries[0].bIsPendingTargeting);
		TestTrue(TEXT("Pending card is marked by current HUD state"), PendingEntries[1].bIsPendingTargeting);
	}

	HUD->ClearTargetSelectionStateForTest();
	HUD->SyncFirstPersonBattleHandLayerForTest(Snapshot);
	const TArray<FWacomFirstPersonCardLayerEntry>& ClearedEntries = Anchor->GetRuntimeCardLayerEntries();
	TestEqual(TEXT("Entry count after clearing target select"), ClearedEntries.Num(), 2);
	if (ClearedEntries.Num() == 2)
	{
		TestFalse(TEXT("Pending state clears on UI state change"), ClearedEntries[1].bIsPendingTargeting);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDFirstPersonLateCleanupOwnershipSpec,
	"Wacom.UI.Battle.BattleHUD.HandPresentation.LateCleanupKeepsRunRuntimeSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDFirstPersonLateCleanupOwnershipSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleHUDFirstPersonSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* BattleCard = Fx.MakeNoopCard(0);
	UCardDefinition* RunCard = WacomBattleHUDFirstPersonSpec::MakePreviewCard(
		GetTransientPackage(),
		TEXT("Battle.LateCleanup.Run"),
		0);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ BattleCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 5, 0), 1);
	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	if (!TestNotNull(TEXT("Battle card"), BattleCard)
		|| !TestNotNull(TEXT("Run card"), RunCard)
		|| !TestNotNull(TEXT("Session"), Session)
		|| !TestNotNull(TEXT("HUD harness"), Harness.Get()))
	{
		return false;
	}

	AWacomPlayerCharacter* Character = Harness->AttachFirstPersonCharacter();
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	UWacomFirstPersonCardAnchorComponent* Anchor = Harness->FirstPersonAnchor();
	if (!TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("First-person card anchor"), Anchor))
	{
		return false;
	}

	Harness->SetSession(Session);
	const FBattleSnapshot BattleSnapshot = Session->BuildSnapshot();
	const FHandCardSnapshot* BattleHandCard =
		FWacomBattleFixture::FindHandCardByCardId(BattleSnapshot, BattleCard->CardId);
	if (!TestNotNull(TEXT("Battle hand card"), BattleHandCard))
	{
		return false;
	}

	HUD->SyncFirstPersonBattleHandLayerForTest(BattleSnapshot);
	TestEqual(
		TEXT("Battle source owns runtime hand before Run restore"),
		Anchor->GetRuntimeCardLayerSourceId(),
		WacomFirstPersonCardLayerSourceIds::BattleHand());
	TestTrue(
		TEXT("Battle source enables interaction before Run restore"),
		Anchor->IsFirstPersonCardLayerInteractionEnabled());

	FWacomFirstPersonCardLayerEntry RunEntry;
	RunEntry.CardInstanceId = FGuid::NewGuid();
	RunEntry.CardViewData = UWacomCardPresentationBuilder::BuildCardViewData(RunCard);
	RunEntry.bIsPlayable = true;
	RunEntry.CardViewData.bDisabled = false;
	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerEntries(
		*Anchor,
		WacomFirstPersonCardLayerSourceIds::RunDefault(),
		{ RunEntry });
	FWacomFirstPersonCardLayerTestAccess::SetFirstPersonCardLayerInteractionEnabled(*Anchor, true);
	TestEqual(
		TEXT("Run source takes over before late BattleHUD cleanup"),
		Anchor->GetRuntimeCardLayerSourceId(),
		WacomFirstPersonCardLayerSourceIds::RunDefault());

	HUD->ClearFirstPersonBattleHandLayerForTest();
	TestEqual(
		TEXT("Late BattleHUD cleanup keeps Run source ownership"),
		Anchor->GetRuntimeCardLayerSourceId(),
		WacomFirstPersonCardLayerSourceIds::RunDefault());
	TestTrue(TEXT("Late BattleHUD cleanup keeps Run source data"), Anchor->HasRuntimeCardLayerData());
	TestEqual(TEXT("Late BattleHUD cleanup keeps Run source entry"), Anchor->GetRuntimeCardLayerEntries().Num(), 1);
	TestTrue(
		TEXT("Late BattleHUD cleanup keeps Run interaction enabled"),
		Anchor->IsFirstPersonCardLayerInteractionEnabled());

	const int32 VersionBeforeStaleDelegate = Session->BuildSnapshot().Version;
	FWacomFirstPersonCardDragView StaleDragView =
		WacomBattleHUDFirstPersonSpec::MakeCommitDragView(BattleHandCard->InstanceId);
	Anchor->OnFirstPersonCardLayerDragReleased.Broadcast(BattleHandCard->InstanceId, StaleDragView);
	TestEqual(
		TEXT("Late cleanup still unbinds BattleHUD drag delegates"),
		Session->BuildSnapshot().Version,
		VersionBeforeStaleDelegate);
	TestEqual(TEXT("Late cleanup leaves HUD state idle"), HUD->GetUIState(), EBattleUIState::Idle);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDFirstPersonHandShortcutByIndexSpec,
	"Wacom.UI.Battle.BattleHUD.HandPresentation.ShortcutStartsDragByHandIndex",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDFirstPersonHandShortcutByIndexSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleHUDFirstPersonSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeSimpleDamageCard(0, 1), Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 5, 0), 3);

	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get()))
	{
		return false;
	}

	AWacomPlayerCharacter* Character = Harness->AttachFirstPersonCharacter();
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	UWacomFirstPersonCardAnchorComponent* Anchor = Harness->FirstPersonAnchor();
	if (!TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("First-person card anchor"), Anchor))
	{
		return false;
	}

	Harness->SetSession(Session);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	if (!TestTrue(TEXT("Battle snapshot has at least two hand cards"), Snapshot.Hand.Cards.Num() >= 2))
	{
		return false;
	}

	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(HUD);
	if (!TestNotNull(TEXT("Manual first-person card layer"), Layer))
	{
		return false;
	}
	Layer->TakeWidget();
	Layer->SetCardLayerInteractionEnabled(true);
	FWacomFirstPersonCardLayerTestAccess::SetCardLayer(*Anchor, Layer);
	HUD->RefreshFromSnapshotForTest(Snapshot);

	TArray<FWacomFirstPersonCardLayerSlotView> Slots;
	for (int32 Index = 0; Index < Snapshot.Hand.Cards.Num(); ++Index)
	{
		Slots.Add(WacomBattleHUDFirstPersonSpec::MakeProjectedShortcutSlot(
			Snapshot.Hand.Cards[Index].InstanceId,
			Index));
	}
	Layer->SetCardSlots(Slots);

	TestTrue(TEXT("HUD starts external drag from second battle hand card"),
		HUD->TryStartFirstPersonBattleHandDragByIndex(2, FVector2D(560.0f, 500.0f)));
	TestTrue(TEXT("Anchor reports active drag after HUD index shortcut"),
		Anchor->IsFirstPersonCardDragGestureActive());

	Anchor->CancelFirstPersonCardDragGesture(/*bBroadcastCancel*/ true);
	TestFalse(TEXT("Invalid hand index is rejected"),
		HUD->TryStartFirstPersonBattleHandDragByIndex(99, FVector2D(560.0f, 500.0f)));
	TestFalse(TEXT("Invalid hand index does not start drag"),
		Anchor->IsFirstPersonCardDragGestureActive());

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
	FWacomUIBattleHUDCardDetailHoverSourceSpec,
	"Wacom.UI.Battle.BattleHUD.CardDetail.HoverUsesSnapshotCardAndIgnoresStaleUnhover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDCardDetailHoverSourceSpec::RunTest(const FString& /*Parameters*/)
{
	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(WacomBattleHUDFirstPersonSpec::FindAutomationWorld());
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get())
		|| !TestNotNull(TEXT("HUD"), Harness->HUD()))
	{
		return false;
	}

	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	UCardDefinition* FirstCard = WacomBattleHUDFirstPersonSpec::MakePreviewCard(
		GetTransientPackage(),
		TEXT("第一人称详情卡 A"),
		1);
	UCardDefinition* SecondCard = WacomBattleHUDFirstPersonSpec::MakePreviewCard(
		GetTransientPackage(),
		TEXT("第一人称详情卡 B"),
		2);
	if (!TestNotNull(TEXT("First card"), FirstCard)
		|| !TestNotNull(TEXT("Second card"), SecondCard))
	{
		return false;
	}

	HUD->TakeWidget();
	const FHandCardSnapshot FirstSnapshot =
		WacomBattleHUDFirstPersonSpec::MakeHandCardSnapshot(FirstCard, 1, true);
	const FHandCardSnapshot SecondSnapshot =
		WacomBattleHUDFirstPersonSpec::MakeHandCardSnapshot(SecondCard, 2, true);
	const FBattleSnapshot Snapshot =
		WacomBattleHUDFirstPersonSpec::MakeSnapshotWithHand({ FirstSnapshot, SecondSnapshot });
	HUD->RefreshFromSnapshotForTest(Snapshot);

	TestEqual(TEXT("Detail provider starts from Idle"), HUD->GetUIStateForTest(), EBattleUIState::Idle);
	TestTrue(TEXT("Detail provider cached snapshot"), HUD->HasLastBattleSnapshotForTest());
	TestEqual(TEXT("Detail provider cached hand cards"), HUD->GetLastBattleSnapshotHandCountForTest(), 2);
	TestTrue(TEXT("Detail provider can find first card"), HUD->HasLastBattleHandCardForTest(FirstSnapshot.InstanceId));
	TestTrue(TEXT("Detail provider can create first-person detail panel"), HUD->EnsureFirstPersonCardDetailPanelForTest());
	TestTrue(
		TEXT("First-person detail viewport z-order is above card layer"),
		HUD->GetFirstPersonCardDetailViewportZOrderForTest() > 9996);

	const FWacomFirstPersonCardLayerSlotView FirstSlot =
		WacomBattleHUDFirstPersonSpec::MakeProjectedDetailSlot(FirstSnapshot.InstanceId);
	HUD->HandleFirstPersonCardHoveredForTest(FirstSnapshot.InstanceId, FirstSlot);
	TestFalse(TEXT("First-person hover waits before showing detail"), HUD->IsCardDetailPanelVisible());
	HUD->TickCardDetailMotionForTest(0.12f);
	TestTrue(TEXT("First-person hover shows detail"), HUD->IsCardDetailPanelVisible());
	TestTrue(
		TEXT("First-person hover uses viewport detail panel"),
		HUD->IsFirstPersonCardDetailPanelVisibleForTest());
	TestEqual(
		TEXT("First-person detail uses first snapshot definition"),
		HUD->GetCardDetailPanelNameText().ToString(),
		FString(TEXT("第一人称详情卡 A")));
	TestEqual(
		TEXT("First-person specific detail has first card name"),
		HUD->GetFirstPersonCardDetailPanelNameTextForTest().ToString(),
		FString(TEXT("第一人称详情卡 A")));

	const FWacomFirstPersonCardLayerSlotView SecondSlot =
		WacomBattleHUDFirstPersonSpec::MakeProjectedDetailSlot(SecondSnapshot.InstanceId);
	HUD->HandleFirstPersonCardHoveredForTest(SecondSnapshot.InstanceId, SecondSlot);
	HUD->TickCardDetailMotionForTest(0.01f);
	TestTrue(TEXT("Second first-person hover keeps detail visible"), HUD->IsCardDetailPanelVisible());
	TestEqual(
		TEXT("First-person detail replaces source"),
		HUD->GetCardDetailPanelNameText().ToString(),
		FString(TEXT("第一人称详情卡 B")));

	HUD->HandleFirstPersonCardUnhoveredForTest(FirstSnapshot.InstanceId, FirstSlot);
	HUD->TickCardDetailMotionForTest(0.01f);
	TestTrue(TEXT("Old first-person source unhover does not hide current detail"), HUD->IsCardDetailPanelVisible());
	TestEqual(
		TEXT("Old first-person source unhover keeps second detail"),
		HUD->GetCardDetailPanelNameText().ToString(),
		FString(TEXT("第一人称详情卡 B")));

	HUD->HandleFirstPersonCardUnhoveredForTest(SecondSnapshot.InstanceId, SecondSlot);
	HUD->TickCardDetailMotionForTest(0.5f);
	TestFalse(TEXT("Current first-person source unhover hides detail"), HUD->IsCardDetailPanelVisible());
	TestFalse(
		TEXT("First-person viewport detail is hidden on current unhover"),
		HUD->IsFirstPersonCardDetailPanelVisibleForTest());

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
