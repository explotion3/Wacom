// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Commands/BattleCommand.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Events/BattleEvent.h"
#include "Session/BattleResultPacket.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomEnums.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
	UCharacterDefinition* MakeCharacterWithRightHandKiller(FWacomBattleFixture& Fx, UCardDefinition** OutKiller)
	{
		UCardDefinition* LeftHand = Fx.MakeNoopCard(/*Cost*/0);
		UCardDefinition* Killer = Fx.MakeSimpleDamageCard(/*Cost*/0, /*Damage*/100);
		if (OutKiller)
		{
			*OutKiller = Killer;
		}

		TArray<UCardDefinition*> Deck;
		for (int32 Index = 0; Index < 6; ++Index)
		{
			Deck.Add(Fx.MakeNoopCard(/*Cost*/0));
		}
		return Fx.MakeCharacter(LeftHand, Killer, Deck);
	}

	int32 CountEventsOfType(const TArray<FBattleEvent>& Events, EBattleEventType Type)
	{
		int32 Count = 0;
		for (const FBattleEvent& Event : Events)
		{
			if (Event.Type == Type)
			{
				++Count;
			}
		}
		return Count;
	}

	bool KnockdownChoicesContain(const TArray<FKnockdownChoice>& Choices, EKnockdownChoice Choice)
	{
		for (const FKnockdownChoice& RecordedChoice : Choices)
		{
			if (RecordedChoice.Choice == Choice)
			{
				return true;
			}
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleSessionCompanionMaxHpBonusOnlyForCompanionSpec,
	"Wacom.Battle.Session.Initialize.CompanionMaxHpBonusOnlyForCompanion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleSessionCompanionMaxHpBonusOnlyForCompanionSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* LeftHand = Fx.MakeNoopCard(/*Cost*/0);
	UCardDefinition* RightHand = Fx.MakeNoopCard(/*Cost*/0);

	UCardDefinition* Companion = Fx.MakeNoopCard(/*Cost*/0);
	Companion->Keywords.AddTag(WacomTags::Card_Keyword_Companion);
	Companion->Physique.MaxHpBonus = 7;

	UCardDefinition* NonCompanion = Fx.MakeNoopCard(/*Cost*/0);
	NonCompanion->Physique.MaxHpBonus = 9;

	TArray<UCardDefinition*> Deck = { Companion, NonCompanion };
	for (int32 Index = 0; Index < 6; ++Index)
	{
		Deck.Add(Fx.MakeNoopCard(/*Cost*/0));
	}

	UCharacterDefinition* Character = Fx.MakeCharacter(LeftHand, RightHand, Deck);
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(/*Hp*/200, /*Initiative*/100, /*IntentResist*/0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, /*Seed*/1);

	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	TestEqual(TEXT("Only Companion MaxHpBonus applies to MaxHp"), Snapshot.Player.MaxHp, 107);
	TestEqual(TEXT("Only Companion MaxHpBonus applies to CurrentHp"), Snapshot.Player.CurrentHp, 107);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleSessionPreDestroyedPartsDoNotRequestKnockdownSpec,
	"Wacom.Battle.Session.Initialize.PreDestroyedPartsDoNotRequestKnockdown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleSessionPreDestroyedPartsDoNotRequestKnockdownSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCharacterDefinition* Character = MakeCharacterWithRightHandKiller(Fx, nullptr);
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(/*HH*/50, /*HB*/50, /*HT*/50, /*IH*/100, /*IB*/100, /*IT*/100);

	FBattleInitParams Params;
	Params.Character = Character;
	Params.RandomSeed = 1;
	FBattleEnemySlotInit EnemySlot;
	EnemySlot.EnemySlotId = TEXT("Enemy");
	EnemySlot.Enemy = Enemy;
	Params.EnemySlots.Add(EnemySlot);
	Params.PreDestroyedParts.Add(FBattlePartSlotIdentity::Make(
		TEXT("Encounter"),
		TEXT("Enemy"),
		TEXT("Test.Part.Head")));

	TStrongObjectPtr<UBattleSession> Session(NewObject<UBattleSession>());
	const FWacomStatus Status = Session->Initialize(Params);
	TestTrue(TEXT("Initialize succeeds"), Status.IsOk());
	if (!Status.IsOk())
	{
		return false;
	}

	const TArray<FBattleEvent> InitialEvents = Session->ConsumeEvents();
	TestEqual(TEXT("Pre-destroyed part does not request knockdown choice"),
		CountEventsOfType(InitialEvents, EBattleEventType::KnockdownChoiceRequested),
		0);
	TestEqual(TEXT("Battle starts in PlayerAction"), Session->GetPhase(), EBattlePhase::PlayerAction);

	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FEnemyPartSnapshot* HeadSnapshot = FWacomBattleFixture::GetEnemyPartSnapshot(Snapshot, 0);
	TestNotNull(TEXT("Head snapshot exists"), HeadSnapshot);
	if (HeadSnapshot)
	{
		TestTrue(TEXT("Head is destroyed in snapshot"), HeadSnapshot->bDestroyed);
		TestEqual(TEXT("Head HP is zero in snapshot"), HeadSnapshot->CurrentHp, 0);
	}

	const FBattleResultPacket Packet = Session->BuildResultPacket();
	TestTrue(TEXT("Result packet records pre-destroyed Head"),
		Packet.DestroyedParts.Contains(FBattlePartSlotIdentity::Make(
			TEXT("Encounter"),
			TEXT("Enemy"),
			TEXT("Test.Part.Head"))));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleKnockdownRequestFlowSingleInitialRequestSpec,
	"Wacom.Battle.Knockdown.RequestFlow.SingleInitialRequest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleKnockdownRequestFlowSingleInitialRequestSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* Killer = nullptr;
	UCharacterDefinition* Character = MakeCharacterWithRightHandKiller(Fx, &Killer);
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(/*HH*/50, /*HB*/50, /*HT*/50, /*IH*/100, /*IB*/100, /*IT*/100);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, /*Seed*/1);
	Session->ConsumeEvents();

	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid HeadId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);
	const FGuid KillerId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, Killer->CardId);

	TestTrue(TEXT("Killer is in opening hand"), KillerId.IsValid());
	TestTrue(TEXT("Head exists"), HeadId.IsValid());
	if (!KillerId.IsValid() || !HeadId.IsValid())
	{
		return false;
	}

	const FWacomStatus Status = Session->SubmitCommand(
		FWacomBattleFixture::MakePlayCardOnPartInstance(Snapshot, KillerId, HeadId));
	TestTrue(TEXT("Killer play succeeds"), Status.IsOk());

	const TArray<FBattleEvent> Events = Session->ConsumeEvents();
	TestEqual(TEXT("Exactly one initial KnockdownChoiceRequested event"),
		CountEventsOfType(Events, EBattleEventType::KnockdownChoiceRequested),
		1);
	TestEqual(TEXT("Phase is pending knockdown choice"), Session->GetPhase(), EBattlePhase::PendingKnockdownChoice);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleResultPacketWithdrawnDerivedFromChoicesSpec,
	"Wacom.Battle.ResultPacket.WithdrawnDerivedFromChoices",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleResultPacketWithdrawnDerivedFromChoicesSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* Killer = nullptr;
	UCharacterDefinition* Character = MakeCharacterWithRightHandKiller(Fx, &Killer);
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(/*HH*/50, /*HB*/50, /*HT*/50, /*IH*/100, /*IB*/100, /*IT*/100);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, /*Seed*/1);

	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid HeadId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);
	const FGuid KillerId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, Killer->CardId);

	TestTrue(TEXT("Killer is in opening hand"), KillerId.IsValid());
	TestTrue(TEXT("Head exists"), HeadId.IsValid());
	if (!KillerId.IsValid() || !HeadId.IsValid())
	{
		return false;
	}

	TestTrue(TEXT("Killer play succeeds"),
		Session->SubmitCommand(FWacomBattleFixture::MakePlayCardOnPartInstance(Snapshot, KillerId, HeadId)).IsOk());
	TestTrue(TEXT("Withdraw choice succeeds"),
		Session->SubmitCommand(FBattleCommand::MakeKnockdownChoice(EKnockdownChoice::Withdraw)).IsOk());

	const FBattleResultPacket Packet = Session->BuildResultPacket();
	TestTrue(TEXT("Packet bWithdrawn is derived from choices"), Packet.bWithdrawn);
	TestEqual(TEXT("Withdraw outcome is Victory"), Packet.Outcome, EBattleOutcome::Victory);
	TestTrue(TEXT("Packet choices contain Withdraw"),
		KnockdownChoicesContain(Packet.KnockdownChoices, EKnockdownChoice::Withdraw));

	return true;
}
