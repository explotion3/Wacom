// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomBattleHUDCommandFlowSpec
{
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDTargetSelectionViewSpec,
	"Wacom.UI.Battle.BattleHUD.TargetingController.TargetSelectionView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDTargetSelectionViewSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* LeftHand = Fx.MakeNoopCard(0);
	UCardDefinition* RightHand = Fx.MakeNoopCard(0);
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(1, 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(LeftHand, RightHand, { TargetCard });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);

	TStrongObjectPtr<UBattleSession> Session(NewObject<UBattleSession>());
	FBattleInitParams Params;
	Params.Character = Character;
	Params.RandomSeed = 1;
	FBattleEnemySlotInit EnemySlot;
	EnemySlot.EnemySlotId = TEXT("Enemy");
	EnemySlot.Enemy = Enemy;
	Params.EnemySlots.Add(EnemySlot);
	FBattlePartSlotIdentity DestroyedPart;
	DestroyedPart.EncounterId = TEXT("Encounter");
	DestroyedPart.EnemySlotId = TEXT("Enemy");
	DestroyedPart.PartSlotId = TEXT("Test.Part.Body");
	Params.PreDestroyedParts.Add(DestroyedPart);
	TestTrue(TEXT("Session initialize"), Session->Initialize(Params).IsOk());

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetSession(Session.Get());
	HUD->TakeWidget();

	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FEnemySnapshot* EnemySnapshot = FWacomBattleFixture::GetEnemySnapshot(Snapshot, 0);
	TestNotNull(TEXT("Enemy snapshot exists"), EnemySnapshot);
	TestEqual(TEXT("Enemy part count"), EnemySnapshot ? EnemySnapshot->Parts.Num() : 0, 3);
	if (!EnemySnapshot || EnemySnapshot->Parts.Num() != 3)
	{
		return false;
	}

	const FBattleTargetSelectionView IdleView = HUD->BuildTargetSelectionView();
	TestFalse(TEXT("Idle view is not selecting"), IdleView.bIsTargetSelecting);
	TestEqual(TEXT("Idle view includes all parts"), IdleView.TargetableParts.Num(), 3);
	TestFalse(TEXT("Idle head not targetable"), IdleView.TargetableParts[0].bTargetable);
	TestEqual(TEXT("Idle disabled reason"), IdleView.TargetableParts[0].DisabledReason, FName(TEXT("NotTargetSelecting")));

	const FGuid TargetCardId = WacomBattleHUDCommandFlowSpec::FindFirstHandCardByTargetMode(
		Snapshot,
		ECardTargetMode::SingleEnemyPart);
	TestTrue(TEXT("Targeting card is in hand"), TargetCardId.IsValid());
	if (!TargetCardId.IsValid())
	{
		return false;
	}

	HUD->SetTargetSelectionStateForTest(TargetCardId);
	const FBattleTargetSelectionView TargetView = HUD->BuildTargetSelectionView();
	TestTrue(TEXT("Target view is selecting"), TargetView.bIsTargetSelecting);
	TestTrue(TEXT("Target view pending card valid"), TargetView.PendingCardInstanceId.IsValid());
	TestEqual(TEXT("Target view includes all parts"), TargetView.TargetableParts.Num(), 3);
	TestTrue(TEXT("Living head is targetable"), TargetView.TargetableParts[0].bTargetable);
	TestEqual(TEXT("Living head reason none"), TargetView.TargetableParts[0].DisabledReason, NAME_None);
	TestFalse(TEXT("Destroyed body is not targetable"), TargetView.TargetableParts[1].bTargetable);
	TestEqual(TEXT("Destroyed body reason"), TargetView.TargetableParts[1].DisabledReason, FName(TEXT("PartDestroyed")));
	TestTrue(TEXT("Living tail is targetable"), TargetView.TargetableParts[2].bTargetable);

	HUD->ClearTargetSelectionStateForTest();
	const FBattleTargetSelectionView ClearedView = HUD->BuildTargetSelectionView();
	TestFalse(TEXT("Cleared view is not selecting"), ClearedView.bIsTargetSelecting);
	TestFalse(TEXT("Cleared view invalid pending card"), ClearedView.PendingCardInstanceId.IsValid());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDWaitEndTurnCancelTargetSelectSpec,
	"Wacom.UI.Battle.BattleHUD.CommandController.WaitEndTurnCancelTargetSelect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDWaitEndTurnCancelTargetSelectSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);

	{
		UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
		TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
		HUD->SetSession(Session);
		HUD->TakeWidget();

		HUD->SetTargetSelectionStateForTest(FGuid::NewGuid());
		TestEqual(TEXT("Wait precondition target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);
		const int32 WaitValueBefore = Session->BuildSnapshot().CurrentWaitValue;

		HUD->OnWaitRequested();

		TestEqual(TEXT("Wait cancels target select and returns idle"), HUD->GetUIState(), EBattleUIState::Idle);
		TestFalse(TEXT("Wait clears pending target card"), HUD->GetPendingTargetingCardId().IsValid());
		TestEqual(TEXT("Wait command still resolves"), Session->BuildSnapshot().CurrentWaitValue, WaitValueBefore + 1);
	}

	{
		FWacomBattleFixture SecondFx;
		UCharacterDefinition* SecondCharacter = SecondFx.MakeCharacter(
			SecondFx.MakeNoopCard(0),
			SecondFx.MakeNoopCard(0),
			{ SecondFx.MakeNoopCard(0), SecondFx.MakeNoopCard(0), SecondFx.MakeNoopCard(0) });
		UEnemyDefinition* SecondEnemy = SecondFx.MakeSinglePartEnemy(20, 5, 0);
		UBattleSession* Session = SecondFx.CreateSession(SecondCharacter, SecondEnemy, 1);
		TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
		HUD->SetSession(Session);
		HUD->TakeWidget();

		HUD->SetTargetSelectionStateForTest(FGuid::NewGuid());
		TestEqual(TEXT("EndTurn precondition target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);
		const FBattleSnapshot SnapshotBefore = Session->BuildSnapshot();

		HUD->OnEndTurnRequested();

		TestEqual(TEXT("EndTurn cancels target select and returns idle"), HUD->GetUIState(), EBattleUIState::Idle);
		TestFalse(TEXT("EndTurn clears pending target card"), HUD->GetPendingTargetingCardId().IsValid());
		TestTrue(TEXT("EndTurn command still resolves"),
			Session->BuildSnapshot().Version > SnapshotBefore.Version);
	}

	return true;
}
