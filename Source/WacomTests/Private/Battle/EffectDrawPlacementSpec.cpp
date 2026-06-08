// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Commands/BattleCommand.h"
#include "Events/BattleEvent.h"
#include "Misc/AutomationTest.h"
#include "Session/BattleSession.h"
#include "Snapshots/HandSnapshot.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"

namespace
{
	UCardDefinition* MakeDrawCard(FWacomBattleFixture& Fx, int32 DrawCount)
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

/**
 * Effect.Draw 是中途入手，不应把新牌固定追加到手牌最右侧。
 * 规则口径：抽到的卡逐张随机插入当前 Hand，不重建整条队列。
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleEffectDrawInsertsCardsAtRandomSpec,
	"Wacom.Battle.Effect.DrawInsertsCardsAtRandomPosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleEffectDrawInsertsCardsAtRandomSpec::RunTest(const FString& /*Parameters*/)
{
	bool bCoveredNonTailInsertion = false;

	for (int32 Seed = 1; Seed <= 80 && !bCoveredNonTailInsertion; ++Seed)
	{
		FWacomBattleFixture Fx;

		UCardDefinition* LH = Fx.MakeNoopCard(/*Cost*/0);
		UCardDefinition* RH = Fx.MakeNoopCard(/*Cost*/0);
		UCardDefinition* DrawCard = MakeDrawCard(Fx, /*DrawCount*/1);

		TArray<UCardDefinition*> Deck = { DrawCard };
		for (int32 Index = 0; Index < 9; ++Index)
		{
			Deck.Add(Fx.MakeNoopCard(/*Cost*/0));
		}

		UCharacterDefinition* Character = Fx.MakeCharacter(LH, RH, Deck);
		UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(/*Hp*/50, /*Initiative*/100, /*IntentResist*/0);
		UBattleSession* Session = Fx.CreateSession(Character, Enemy, Seed);

		FBattleSnapshot Snap = Session->BuildSnapshot();
		const int32 HandCountBefore = Snap.Hand.Cards.Num();
		TSet<FGuid> HandIdsBefore;
		HandIdsBefore.Reserve(Snap.Hand.Cards.Num());
		for (const FHandCardSnapshot& Card : Snap.Hand.Cards)
		{
			HandIdsBefore.Add(Card.InstanceId);
		}

		const FGuid DrawCardId = FWacomBattleFixture::FindHandInstanceByCardId(Snap, DrawCard->CardId);
		if (!DrawCardId.IsValid())
		{
			continue;
		}

		const int32 DrawPileBefore = Snap.PileCounts.DrawCount;
		TestTrue(FString::Printf(TEXT("Seed=%d has draw pile card"), Seed), DrawPileBefore > 0);

		TestTrue(TEXT("Play draw card"),
			Session->SubmitCommand(FBattleCommand::MakePlayCard(DrawCardId)).IsOk());

		const TArray<FBattleEvent> Events = Session->ConsumeEvents();

		Snap = Session->BuildSnapshot();
		TestEqual(FString::Printf(TEXT("Seed=%d hand count stable after play one draw one"), Seed),
			Snap.Hand.Cards.Num(), HandCountBefore);
		TestEqual(FString::Printf(TEXT("Seed=%d draw pile consumed one"), Seed),
			Snap.PileCounts.DrawCount, DrawPileBefore - 1);

		FGuid InsertedCardId;
		int32 InsertedIndex = INDEX_NONE;
		for (int32 Index = 0; Index < Snap.Hand.Cards.Num(); ++Index)
		{
			const FHandCardSnapshot& Card = Snap.Hand.Cards[Index];
			if (!HandIdsBefore.Contains(Card.InstanceId))
			{
				InsertedCardId = Card.InstanceId;
				InsertedIndex = Index;
				break;
			}
		}
		TestTrue(FString::Printf(TEXT("Seed=%d found newly drawn card in hand"), Seed),
			InsertedCardId.IsValid());
		if (InsertedCardId.IsValid() && InsertedIndex != Snap.Hand.Cards.Num() - 1)
		{
			bCoveredNonTailInsertion = true;
		}
	}

	TestTrue(TEXT("At least one deterministic seed inserts drawn card away from tail"), bCoveredNonTailInsertion);
	return true;
}

/**
 * 中途抽牌会立刻执行普通卡上限 10。
 * 注意：正在打出的抽牌卡随后会离开手牌，不能被这次上限检查计入。
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleEffectDrawImmediatelyEnforcesHandLimitSpec,
	"Wacom.Battle.Effect.DrawImmediatelyEnforcesHandLimit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleEffectDrawImmediatelyEnforcesHandLimitSpec::RunTest(const FString& /*Parameters*/)
{
	for (int32 Seed = 1; Seed <= 80; ++Seed)
	{
		FWacomBattleFixture Fx;

		UCardDefinition* LH = Fx.MakeNoopCard(/*Cost*/0);
		UCardDefinition* RH = Fx.MakeNoopCard(/*Cost*/0);
		UCardDefinition* DrawCard = MakeDrawCard(Fx, /*DrawCount*/7);

		TArray<UCardDefinition*> Deck = { DrawCard };
		for (int32 Index = 0; Index < 14; ++Index)
		{
			Deck.Add(Fx.MakeNoopCard(/*Cost*/0));
		}

		UCharacterDefinition* Character = Fx.MakeCharacter(LH, RH, Deck);
		UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(/*Hp*/50, /*Initiative*/100, /*IntentResist*/0);
		UBattleSession* Session = Fx.CreateSession(Character, Enemy, Seed);

		FBattleSnapshot Snap = Session->BuildSnapshot();
		const FGuid DrawCardId = FWacomBattleFixture::FindHandInstanceByCardId(Snap, DrawCard->CardId);
		if (!DrawCardId.IsValid())
		{
			continue;
		}

		TestEqual(FString::Printf(TEXT("Seed=%d starts with first turn draw count"), Seed),
			Snap.Hand.NormalCardCount, 5);
		const int32 DiscardBefore = Snap.PileCounts.DiscardCount;

		TestTrue(TEXT("Play draw card"),
			Session->SubmitCommand(FBattleCommand::MakePlayCard(DrawCardId)).IsOk());
		const TArray<FBattleEvent> Events = Session->ConsumeEvents();

		Snap = Session->BuildSnapshot();
		TestEqual(FString::Printf(TEXT("Seed=%d normal count remains limited immediately"), Seed),
			Snap.Hand.NormalCardCount, Snap.Hand.NormalCardLimit);
		TestTrue(FString::Printf(TEXT("Seed=%d extra card discarded by limit"), Seed),
			Snap.PileCounts.DiscardCount >= DiscardBefore + 1);
		TestEqual(FString::Printf(TEXT("Seed=%d played draw card waits in played pile"), Seed),
			Snap.PileCounts.PlayedCount, 1);
		TestEqual(FString::Printf(TEXT("Seed=%d played draw card leaves hand"), Seed),
			FWacomBattleFixture::FindHandIndex(Snap, DrawCardId), INDEX_NONE);

		int32 LimitDiscardEvents = 0;
		for (const FBattleEvent& Event : Events)
		{
			if (Event.Type != EBattleEventType::HandLimitDiscarded)
			{
				continue;
			}

			++LimitDiscardEvents;
			TestTrue(TEXT("Limit discard card id is valid"), Event.CardInstanceId.IsValid());
			TestNotEqual(TEXT("Limit discard is not the played draw card"), Event.CardInstanceId, DrawCardId);
			TestEqual(TEXT("Limit discard source is EffectDraw"),
				Event.HandLimitDiscardSource, EHandLimitDiscardSource::EffectDraw);
			TestEqual(TEXT("Limit discard actor is draw source card"), Event.ActorInstanceId, DrawCardId);
		}
		TestTrue(TEXT("Effect draw emits per-card hand limit discard event"), LimitDiscardEvents > 0);
		return true;
	}

	AddError(TEXT("No seed placed the draw effect card in opening hand"));
	return false;
}
