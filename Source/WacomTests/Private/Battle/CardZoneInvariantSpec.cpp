// Copyright Wacom. All Rights Reserved.

#include "Battle/BattleSessionTestAccess.h"
#include "Fixtures/BattleTestFixtures.h"

#include "Cards/CardDefinition.h"
#include "Commands/BattleCommand.h"
#include "Misc/AutomationTest.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"

#if WITH_AUTOMATION_TESTS

namespace
{
	bool TestCardZoneInvariant(
		FAutomationTestBase& Test,
		const TCHAR* Stage,
		const UBattleSession* Session)
	{
		FString Error;
		const bool bValid = FWacomBattleSessionTestAccess::ValidateCardZoneInvariants(Session, Error);
		const FString Label = Error.IsEmpty()
			? FString::Printf(TEXT("%s keeps one card-zone truth"), Stage)
			: FString::Printf(TEXT("%s keeps one card-zone truth: %s"), Stage, *Error);
		return Test.TestTrue(*Label, bValid);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCardZoneInvariantLifecycleSpec,
	"Wacom.Battle.CardZone.InvariantAcrossLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCardZoneInvariantLifecycleSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* LeftHand = Fixture.MakeNoopCard(0);
	LeftHand->CardId = TEXT("CardZoneInvariant.Left");
	UCardDefinition* RightHand = Fixture.MakeNoopCard(0);
	RightHand->CardId = TEXT("CardZoneInvariant.Right");
	UCardDefinition* Normal = Fixture.MakeNoopCard(0);
	Normal->CardId = TEXT("CardZoneInvariant.Normal");
	UCardDefinition* DiscardSource = Fixture.MakeRandomDiscardCard(0, 1);
	DiscardSource->CardId = TEXT("CardZoneInvariant.DiscardSource");
	UCardDefinition* ExhaustSource = Fixture.MakeSelectedHandCardZoneMoveCard(0, true);
	ExhaustSource->CardId = TEXT("CardZoneInvariant.ExhaustSource");
	UCardDefinition* ExhaustTarget = Fixture.MakeNoopCard(0);
	ExhaustTarget->CardId = TEXT("CardZoneInvariant.ExhaustTarget");

	UBattleSession* Session = Fixture.CreateSession(
		Fixture.MakeCharacter(
			LeftHand,
			RightHand,
			{ Normal, DiscardSource, ExhaustSource, ExhaustTarget }),
		Fixture.MakeSinglePartEnemyWithIntentDamage(100, 50, 0, 0),
		/*Seed*/23);
	if (!TestCardZoneInvariant(*this, TEXT("Initialize/initial draw"), Session))
	{
		return false;
	}

	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid NormalId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, Normal->CardId);
	if (NormalId.IsValid())
	{
		TestTrue(TEXT("Normal card play succeeds"),
			Session->ResolveCommand(FBattleCommand::MakePlayCard(NormalId)).IsOk());
		TestCardZoneInvariant(*this, TEXT("Hand to Played"), Session);
	}

	Snapshot = Session->BuildSnapshot();
	const FGuid LeftId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, LeftHand->CardId);
	if (LeftId.IsValid())
	{
		TestTrue(TEXT("Anchor play succeeds"),
			Session->ResolveCommand(FBattleCommand::MakePlayCard(LeftId)).IsOk());
		TestCardZoneInvariant(*this, TEXT("Hand to Limbo"), Session);
	}

	Snapshot = Session->BuildSnapshot();
	const FGuid ExhaustSourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, ExhaustSource->CardId);
	const FGuid ExhaustTargetId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, ExhaustTarget->CardId);
	if (ExhaustSourceId.IsValid() && ExhaustTargetId.IsValid())
	{
		TestTrue(TEXT("Selected-card exhaust succeeds"),
			Session->ResolveCommand(
				FBattleCommand::MakePlayCardOnHandCard(ExhaustSourceId, ExhaustTargetId)).IsOk());
		TestCardZoneInvariant(*this, TEXT("Hand to Exhaust"), Session);
	}

	if (Session->GetPhase() == EBattlePhase::PlayerAction)
	{
		TestTrue(TEXT("EndTurn succeeds"),
			Session->ResolveCommand(FBattleCommand::MakeEndTurn()).IsOk());
		TestCardZoneInvariant(*this, TEXT("Turn cleanup and next draw"), Session);
	}

	return true;
}

#endif
