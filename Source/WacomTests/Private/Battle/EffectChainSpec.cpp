// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Commands/BattleCommand.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Cards/CardPassive.h"
#include "Tags/WacomGameplayTags.h"

namespace
{
	FCardEffect MakeChainEffect(
		const FGameplayTag& EffectType,
		int32 Magnitude,
		const FGameplayTag& Target,
		const FGameplayTag& Parameter = FGameplayTag())
	{
		FCardEffect Effect;
		Effect.EffectType = EffectType;
		Effect.Magnitude = Magnitude;
		Effect.Target = Target;
		Effect.TargetZone = Parameter;
		return Effect;
	}

	int32 CountNormalCardsWithRuntimeCost(const FBattleSnapshot& Snapshot, int32 RuntimeCost)
	{
		int32 Count = 0;
		for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
		{
			if (!Card.bIsHandAnchor && Card.RuntimeCost == RuntimeCost)
			{
				++Count;
			}
		}
		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleEffectChainContinuesAfterHandlerFailureSpec,
	"Wacom.Battle.EffectChain.CardHandlerFailureContinues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleEffectChainContinuesAfterHandlerFailureSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* Card = Fixture.MakeNoopCard(/*Cost*/0);
	Card->Effects = {
		MakeChainEffect(
			WacomTags::Effect_RemoveStatus,
			/*Magnitude*/1,
			WacomTags::Target_Player,
			WacomTags::Status_Slow),
		MakeChainEffect(
			WacomTags::Status_Shield,
			/*Magnitude*/4,
			WacomTags::Target_Player),
	};

	UCardDefinition* Filler = Fixture.MakeNoopCard(/*Cost*/0);
	UBattleSession* Session = Fixture.CreateSession(
		Fixture.MakeCharacter(
			Fixture.MakeNoopCard(0),
			Fixture.MakeNoopCard(0),
			{ Card, Filler, Filler, Filler, Filler }),
		Fixture.MakeSinglePartEnemy(/*Hp*/100, /*Initiative*/50),
		/*Seed*/1);

	const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(
		Session->BuildSnapshot(),
		Card->CardId);
	TestTrue(TEXT("Source card is in hand"), CardId.IsValid());
	TestTrue(TEXT("Play succeeds although RemoveStatus cannot apply"),
		Session->ResolveCommand(FBattleCommand::MakePlayCard(CardId)).IsOk());
	TestEqual(TEXT("Following shield effect still executes"),
		Session->BuildSnapshot().Player.Shield,
		4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleEffectChainScratchFailureAndIsolationSpec,
	"Wacom.Battle.EffectChain.ScratchSurvivesFailureButDoesNotLeakToAfterPlayed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleEffectChainScratchFailureAndIsolationSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* Card = Fixture.MakeNoopCard(/*Cost*/0);
	Card->Effects = {
		MakeChainEffect(
			WacomTags::Effect_Shuffle_Random,
			/*Magnitude*/0,
			WacomTags::Target_RandomHandCard),
		MakeChainEffect(
			WacomTags::Effect_RemoveStatus,
			/*Magnitude*/1,
			WacomTags::Target_Player,
			WacomTags::Status_Slow),
		MakeChainEffect(
			WacomTags::Effect_Card_ReduceCost,
			/*Magnitude*/1,
			WacomTags::Target_LastShuffledCard),
	};

	FCardPassive AfterPlayed;
	AfterPlayed.Trigger = WacomTags::Passive_Trigger_AfterPlayed;
	AfterPlayed.Effects.Add(MakeChainEffect(
		WacomTags::Effect_Card_ReduceCost,
		/*Magnitude*/1,
		WacomTags::Target_LastShuffledCard));
	Card->Passives.Add(AfterPlayed);

	UCardDefinition* Filler = Fixture.MakeNoopCard(/*Cost*/5);
	UBattleSession* Session = Fixture.CreateSession(
		Fixture.MakeCharacter(
			Fixture.MakeNoopCard(0),
			Fixture.MakeNoopCard(0),
			{ Card, Filler, Filler, Filler, Filler }),
		Fixture.MakeSinglePartEnemy(/*Hp*/100, /*Initiative*/50),
		/*Seed*/1);

	const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(
		Session->BuildSnapshot(),
		Card->CardId);
	TestTrue(TEXT("Source card is in hand"), CardId.IsValid());
	TestTrue(TEXT("Play succeeds"),
		Session->ResolveCommand(FBattleCommand::MakePlayCard(CardId)).IsOk());

	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	TestEqual(TEXT("Main chain keeps LastShuffledCard across a failed effect"),
		CountNormalCardsWithRuntimeCost(Snapshot, 4),
		1);
	TestEqual(TEXT("AfterPlayed starts a fresh chain instead of reducing the same card again"),
		CountNormalCardsWithRuntimeCost(Snapshot, 3),
		0);
	return true;
}
