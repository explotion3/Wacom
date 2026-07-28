// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Battle/BattleSessionTestAccess.h"
#include "Cards/CardDefinition.h"
#include "Commands/BattleCommand.h"
#include "Events/BattleEvent.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattlePileInspectionSnapshot.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"

namespace WacomBattleBurnSpec
{
	UBattleSession* CreateSession(
		FWacomBattleFixture& Fixture,
		const TArray<UCardDefinition*>& RequiredCards,
		UEnemyDefinition* Enemy)
	{
		TArray<UCardDefinition*> Deck = RequiredCards;
		while (Deck.Num() < 5)
		{
			Deck.Add(Fixture.MakeNoopCard(0));
		}
		return Fixture.CreateSession(
			Fixture.MakeCharacter(
				Fixture.MakeNoopCard(0),
				Fixture.MakeNoopCard(0),
				Deck),
			Enemy,
			71);
	}

	const FBattleEvent* FindDamageByCause(
		const TArray<FBattleEvent>& Events,
		const FGameplayTag& Cause)
	{
		return Events.FindByPredicate([Cause](const FBattleEvent& Event)
		{
			return Event.Type == EBattleEventType::DamageDealt
				&& Event.Tag.MatchesTagExact(Cause);
		});
	}

	const FBattlePileCardSnapshot* FindPileCard(
		const FBattlePileInspectionSnapshot& Snapshot,
		const FGuid& InstanceId)
	{
		for (const FBattlePileInspectionSectionSnapshot& Section :
			Snapshot.Sections)
		{
			for (const FBattlePileCardSnapshot& Card : Section.Cards)
			{
				if (Card.InstanceId == InstanceId)
				{
					return &Card;
				}
			}
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleBurnEnemyActionBoundarySpec,
	"Wacom.Battle.Burn.EnemyActionBoundaryShieldAndStunOrdering",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleBurnEnemyActionBoundarySpec::RunTest(const FString&)
{
	using namespace WacomBattleBurnSpec;
	FWacomBattleFixture Fixture;
	UBattleSession* Session = CreateSession(
		Fixture,
		{},
		Fixture.MakeSinglePartEnemyWithIntentDamage(
			/*Hp=*/20,
			/*Initiative=*/9,
			/*Damage=*/7));
	const FBattleSnapshot Before = Session->BuildSnapshot();
	const FBattleEnemyPartKey PartKey =
		FWacomBattleFixture::FindPartKey(Before, 0);
	TestTrue(TEXT("Set enemy Burn"),
		FWacomBattleSessionTestAccess::SetEnemyPartStatusStacks(
			Session, PartKey, WacomTags::Status_Burn, 5));
	TestTrue(TEXT("Set enemy Stunned"),
		FWacomBattleSessionTestAccess::SetEnemyPartStatusStacks(
			Session, PartKey, WacomTags::Status_Stunned, 1));
	TestTrue(TEXT("Set enemy shield"),
		FWacomBattleSessionTestAccess::SetEnemyPartShield(
			Session, PartKey, 2));

	const FBattleResolution Resolution =
		Session->ResolveCommand(FBattleCommand::MakeEndTurn());
	TestTrue(TEXT("End turn succeeds"), Resolution.IsOk());
	const FBattleEvent* BurnDamage =
		FindDamageByCause(Resolution.Events, WacomTags::Status_Burn);
	if (!TestNotNull(TEXT("Burn emits periodic damage"), BurnDamage))
	{
		return false;
	}
	TestEqual(TEXT("Burn uses current stacks as requested damage"),
		BurnDamage->DamageResolution.RequestedDamage, 5);
	TestEqual(TEXT("Burn damage passes through shield"),
		BurnDamage->DamageResolution.ShieldAbsorbed, 2);
	TestEqual(TEXT("Burn HP loss is post-shield"),
		BurnDamage->Amount, 3);
	TestTrue(TEXT("Burn is periodic"),
		BurnDamage->DamageResolution.Kind == EBattleDamageKind::Periodic);

	const FBattleSnapshot After = Session->BuildSnapshot();
	const FEnemyPartSnapshot* Part =
		FWacomBattleFixture::GetEnemyPartSnapshot(After, 0);
	if (!TestNotNull(TEXT("Enemy part remains"), Part))
	{
		return false;
	}
	TestEqual(TEXT("Enemy HP reflects Burn only"), Part->CurrentHp, 17);
	TestEqual(TEXT("Shield is consumed"), Part->Shield, 0);
	TestEqual(TEXT("Burn halves with floor semantics"),
		FWacomBattleFixture::GetStatusStacks(
			Part->StatusStacks, WacomTags::Status_Burn),
		2);
	TestEqual(TEXT("Stun is consumed after Burn"),
		FWacomBattleFixture::GetStatusStacks(
			Part->StatusStacks, WacomTags::Status_Stunned),
		0);
	TestEqual(TEXT("Stunned enemy deals no Intent damage"),
		After.Player.CurrentHp, 100);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleBurnLethalPreventsIntentSpec,
	"Wacom.Battle.Burn.LethalPreventsIntent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleBurnLethalPreventsIntentSpec::RunTest(const FString&)
{
	using namespace WacomBattleBurnSpec;
	FWacomBattleFixture Fixture;
	UBattleSession* Session = CreateSession(
		Fixture,
		{},
		Fixture.MakeSinglePartEnemyWithIntentDamage(
			/*Hp=*/4,
			/*Initiative=*/6,
			/*Damage=*/20));
	const FBattleEnemyPartKey PartKey =
		FWacomBattleFixture::FindPartKey(Session->BuildSnapshot(), 0);
	TestTrue(TEXT("Set lethal Burn"),
		FWacomBattleSessionTestAccess::SetEnemyPartStatusStacks(
			Session, PartKey, WacomTags::Status_Burn, 5));

	const FBattleResolution Resolution =
		Session->ResolveCommand(FBattleCommand::MakeEndTurn());
	TestTrue(TEXT("End turn succeeds"), Resolution.IsOk());
	const FEnemyPartSnapshot* Part =
		FWacomBattleFixture::GetEnemyPartSnapshot(
			Session->BuildSnapshot(), 0);
	TestTrue(TEXT("Burn destroys the part"), Part && Part->bDestroyed);
	TestEqual(TEXT("Destroyed part does not execute Intent"),
		Session->BuildSnapshot().Player.CurrentHp, 100);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleBurnPlayerDrawTransferSpec,
	"Wacom.Battle.Burn.PlayerDrawTransferAndThreeStackExhaust",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleBurnPlayerDrawTransferSpec::RunTest(const FString&)
{
	using namespace WacomBattleBurnSpec;
	FWacomBattleFixture Fixture;
	TArray<UCardDefinition*> Cards;
	for (int32 Index = 0; Index < 5; ++Index)
	{
		Cards.Add(Fixture.MakeNoopCard(0));
	}
	UBattleSession* Session = CreateSession(
		Fixture,
		Cards,
		Fixture.MakeSinglePartEnemyWithIntentDamage(100, 10, 0));
	const FBattleSnapshot Initial = Session->BuildSnapshot();
	for (const FHandCardSnapshot& Card : Initial.Hand.Cards)
	{
		if (!Card.bIsHandAnchor)
		{
			TestTrue(TEXT("Seed every potential redraw at two Burn"),
				FWacomBattleSessionTestAccess::SetCardStatusStacks(
					Session,
					Card.InstanceId,
					WacomTags::Status_Burn,
					2));
		}
	}
	TestTrue(TEXT("Seed player Burn for every draw"),
		FWacomBattleSessionTestAccess::SetPlayerStatusStacks(
			Session, WacomTags::Status_Burn, 5));

	const FBattleResolution Resolution =
		Session->ResolveCommand(FBattleCommand::MakeEndTurn());
	TestTrue(TEXT("End turn and redraw succeed"), Resolution.IsOk());
	const FBattleSnapshot After = Session->BuildSnapshot();
	const FBattleEvent* Drawn = Resolution.Events.FindByPredicate(
		[](const FBattleEvent& Event)
		{
			return Event.Type == EBattleEventType::CardsDrawn;
		});
	if (!TestNotNull(TEXT("Turn start performs a real draw"), Drawn))
	{
		return false;
	}
	TestTrue(TEXT("At least one card is redrawn"),
		Drawn->CardInstanceIds.Num() > 0);
	TestEqual(TEXT("Player Burn is consumed in draw order"),
		FWacomBattleFixture::GetStatusStacks(
			After.Player.StatusStacks, WacomTags::Status_Burn),
		5 - Drawn->CardInstanceIds.Num());
	TestEqual(TEXT("Every three-stack drawn card immediately exhausts"),
		After.PileCounts.ExhaustCount,
		Drawn->CardInstanceIds.Num());

	const FBattlePileInspectionSnapshot Piles =
		Session->BuildPileInspectionSnapshot();
	for (const FGuid& DrawnCardId : Drawn->CardInstanceIds)
	{
		const FBattlePileCardSnapshot* Exhausted =
			FindPileCard(Piles, DrawnCardId);
		if (!TestNotNull(TEXT("Exhausted card remains inspectable"),
			Exhausted))
		{
			return false;
		}
		TestTrue(TEXT("Burned-out card is in exhaust"),
			Exhausted->Location == ECardLocation::Exhaust);
		TestEqual(TEXT("Burned-out card preserves three stacks"),
			FWacomBattleFixture::GetStatusStacks(
				Exhausted->StatusStacks, WacomTags::Status_Burn),
			3);
	}
	return true;
}
