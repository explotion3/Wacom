// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Commands/BattleCommand.h"
#include "Events/BattleEvent.h"
#include "Misc/AutomationTest.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"

namespace
{
	UCardDefinition* MakePlayedPileDrawCard(FWacomBattleFixture& Fx, int32 DrawCount)
	{
		UCardDefinition* Card = Fx.MakeNoopCard(/*Cost*/0);

		FCardEffect Effect;
		Effect.EffectType = WacomTags::Effect_Draw;
		Effect.Magnitude = DrawCount;
		Effect.TargetZone = WacomTags::CardLocation_Draw;
		Card->Effects.Add(Effect);
		return Card;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattlePlayedPileReceivesOrdinaryPlayedCardsSpec,
	"Wacom.Battle.PlayedPile.OrdinaryPlayedCardWaitsUntilTurnEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattlePlayedPileReceivesOrdinaryPlayedCardsSpec::RunTest(const FString& /*Parameters*/)
{
	for (int32 Seed = 1; Seed <= 120; ++Seed)
	{
		FWacomBattleFixture Fx;
		UCardDefinition* PlayedCard = Fx.MakeNoopCard(/*Cost*/0);
		TArray<UCardDefinition*> Deck = { PlayedCard };
		for (int32 Index = 0; Index < 12; ++Index)
		{
			Deck.Add(Fx.MakeNoopCard(/*Cost*/0));
		}

		UBattleSession* Session = Fx.CreateSession(
			Fx.MakeCharacter(Fx.MakeNoopCard(/*Cost*/0), Fx.MakeNoopCard(/*Cost*/0), Deck),
			Fx.MakeSinglePartEnemyWithIntentDamage(/*Hp*/100, /*Initiative*/50, /*IntentResist*/0, /*Damage*/0),
			Seed);

		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const FGuid PlayedId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, PlayedCard->CardId);
		if (!PlayedId.IsValid())
		{
			continue;
		}

		const int32 PlayedBefore = Snapshot.PileCounts.PlayedCount;
		const int32 DiscardBefore = Snapshot.PileCounts.DiscardCount;

		Session->ConsumeEvents();
		TestTrue(TEXT("Play ordinary card"), Session->SubmitCommand(FBattleCommand::MakePlayCard(PlayedId)).IsOk());
		const TArray<FBattleEvent> PlayEvents = Session->ConsumeEvents();
		Snapshot = Session->BuildSnapshot();

		TestEqual(TEXT("Ordinary played card leaves hand"),
			FWacomBattleFixture::FindHandIndex(Snapshot, PlayedId),
			INDEX_NONE);
		TestEqual(TEXT("Played pile gains ordinary played card"),
			Snapshot.PileCounts.PlayedCount,
			PlayedBefore + 1);
		TestEqual(TEXT("Discard pile does not gain ordinary played card immediately"),
			Snapshot.PileCounts.DiscardCount,
			DiscardBefore);
		TestEqual(TEXT("Natural played destination emits no CardDiscarded"),
			FWacomBattleFixture::CountEvents(PlayEvents, EBattleEventType::CardDiscarded),
			0);

		TestTrue(TEXT("End turn"), Session->SubmitCommand(FBattleCommand::MakeEndTurn()).IsOk());
		Snapshot = Session->BuildSnapshot();
		TestEqual(TEXT("Played pile drains at end turn"), Snapshot.PileCounts.PlayedCount, 0);
		TestTrue(TEXT("Discard pile receives played card by end turn"),
			Snapshot.PileCounts.DiscardCount >= DiscardBefore + 1);
		return true;
	}

	AddError(TEXT("No deterministic seed placed ordinary card in opening hand"));
	return false;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattlePlayedPileExcludedFromSameTurnReshuffleSpec,
	"Wacom.Battle.PlayedPile.ExcludedFromSameTurnReshuffle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattlePlayedPileExcludedFromSameTurnReshuffleSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* FirstPlayedCard = Fx.MakeNoopCard(/*Cost*/0);
	UCardDefinition* DiscardSource = Fx.MakeSelectedHandCardZoneMoveCard(/*Cost*/0, /*bExhaust*/false);
	UCardDefinition* DiscardTarget = Fx.MakeNoopCard(/*Cost*/0);
	UCardDefinition* DrawCard = MakePlayedPileDrawCard(Fx, /*DrawCount*/20);

	TArray<UCardDefinition*> Deck = {
		FirstPlayedCard,
		DiscardSource,
		DiscardTarget,
		DrawCard,
	};

	UBattleSession* Session = Fx.CreateSession(
		Fx.MakeCharacter(Fx.MakeNoopCard(/*Cost*/0), Fx.MakeNoopCard(/*Cost*/0), Deck),
		Fx.MakeSinglePartEnemyWithIntentDamage(/*Hp*/100, /*Initiative*/100, /*IntentResist*/0, /*Damage*/0),
		1);

	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid FirstPlayedId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, FirstPlayedCard->CardId);
	TestTrue(TEXT("First played card starts in hand"), FirstPlayedId.IsValid());
	const FGuid DiscardSourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, DiscardSource->CardId);
	const FGuid DiscardTargetId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, DiscardTarget->CardId);
	TestTrue(TEXT("Discard source starts in hand"), DiscardSourceId.IsValid());
	TestTrue(TEXT("Discard target starts in hand"), DiscardTargetId.IsValid());
	Session->ConsumeEvents();

	TestTrue(TEXT("Play first ordinary card"),
		Session->SubmitCommand(FBattleCommand::MakePlayCard(FirstPlayedId)).IsOk());
	Snapshot = Session->BuildSnapshot();
	TestEqual(TEXT("First ordinary card is in played pile before reshuffle"),
		Snapshot.PileCounts.PlayedCount,
		1);
	TestEqual(TEXT("First ordinary card is not in discard before reshuffle"),
		Snapshot.PileCounts.DiscardCount,
		0);

	TestTrue(TEXT("Move one target to discard pile"),
		Session->SubmitCommand(FBattleCommand::MakePlayCardOnHandCard(DiscardSourceId, DiscardTargetId)).IsOk());
	Snapshot = Session->BuildSnapshot();
	TestEqual(TEXT("Discard target waits in discard pile before reshuffle"),
		Snapshot.PileCounts.DiscardCount,
		1);
	TestEqual(TEXT("Two source cards wait in played pile before reshuffle"),
		Snapshot.PileCounts.PlayedCount,
		2);

	const FGuid DrawCardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, DrawCard->CardId);
	TestTrue(TEXT("Draw card remains in hand"), DrawCardId.IsValid());
	if (!DrawCardId.IsValid())
	{
		return false;
	}

	TestTrue(TEXT("Play draw card to reshuffle discard only"),
		Session->SubmitCommand(FBattleCommand::MakePlayCard(DrawCardId)).IsOk());
	Snapshot = Session->BuildSnapshot();

	TestEqual(TEXT("Draw cannot pull from same-turn played pile"),
		FWacomBattleFixture::FindHandIndex(Snapshot, FirstPlayedId),
		INDEX_NONE);
	TestTrue(TEXT("Draw can pull card that was truly in discard"),
		FWacomBattleFixture::FindHandIndex(Snapshot, DiscardTargetId) != INDEX_NONE);
	TestEqual(TEXT("All played source cards wait in played pile"),
		Snapshot.PileCounts.PlayedCount,
		3);
	TestEqual(TEXT("Discard pile is consumed by reshuffle"),
		Snapshot.PileCounts.DiscardCount,
		0);
	return true;
}
