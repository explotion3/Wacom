// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Battle/BattleSessionTestAccess.h"
#include "Cards/CardDefinition.h"
#include "Commands/BattleCommand.h"
#include "Events/BattleEvent.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Runtime/RuntimeCardInstance.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"

#include "UObject/StrongObjectPtr.h"

namespace WacomBattleFireWriteCardsSpec
{
	UCardDefinition* LoadCard(const TCHAR* EnglishName)
	{
		const FString ObjectPath = FString::Printf(
			TEXT("/Game/Wacom/Data/Cards/FireWrite/DA_Card_%s."
				"DA_Card_%s"),
			EnglishName,
			EnglishName);
		return LoadObject<UCardDefinition>(nullptr, *ObjectPath);
	}

	TStrongObjectPtr<UBattleSession> CreateSession(
		FWacomBattleFixture& Fixture,
		const TArray<UCardDefinition*>& Cards,
		const int32 Seed = 91)
	{
		TStrongObjectPtr<UBattleSession> Session(
			NewObject<UBattleSession>());
		FBattleInitParams Params;
		Params.Character = Fixture.MakeCharacter(
			Fixture.MakeNoopCard(0),
			Fixture.MakeNoopCard(0),
			{});
		Params.RandomSeed = Seed;
		for (UCardDefinition* Card : Cards)
		{
			FBattleDeckEntry Entry;
			Entry.Definition = Card;
			Params.BattleDeckEntries.Add(Entry);
		}
		FBattleEnemySlotInit EnemySlot;
		EnemySlot.EnemySlotId = TEXT("Enemy");
		EnemySlot.Enemy =
			Fixture.MakeSinglePartEnemyWithIntentDamage(300, 30, 0);
		Params.EnemySlots.Add(EnemySlot);
		const FBattleInitializationResult Initialization =
			Session->Initialize(Params);
		check(Initialization.IsOk());
		return Session;
	}

	void PadDeck(
		FWacomBattleFixture& Fixture,
		TArray<UCardDefinition*>& Cards)
	{
		while (Cards.Num() < 5)
		{
			Cards.Add(Fixture.MakeNoopCard(0));
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleFireWriteOnDrawSpec,
	"Wacom.Battle.FireWriteCards.OnDrawCostAndDamageGrowth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleFireWriteOnDrawSpec::RunTest(const FString&)
{
	using namespace WacomBattleFireWriteCardsSpec;
	FWacomBattleFixture Fixture;
	UCardDefinition* Jade = LoadCard(TEXT("JadeBeetle"));
	UCardDefinition* Obsidian = LoadCard(TEXT("ObsidianBeetle"));
	if (!TestTrue(TEXT("Beetle assets load"), Jade && Obsidian))
	{
		return false;
	}
	TArray<UCardDefinition*> Deck = { Jade, Obsidian };
	PadDeck(Fixture, Deck);
	TStrongObjectPtr<UBattleSession> Session =
		CreateSession(Fixture, Deck);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FHandCardSnapshot* JadeCard =
		FWacomBattleFixture::FindHandCardByCardId(
			Snapshot, Jade->CardId);
	const FHandCardSnapshot* ObsidianCard =
		FWacomBattleFixture::FindHandCardByCardId(
			Snapshot, Obsidian->CardId);
	if (!TestTrue(TEXT("Both Beetles are drawn"),
		JadeCard && ObsidianCard))
	{
		return false;
	}
	TestEqual(TEXT("Jade OnDraw reduces cost once"),
		JadeCard->RuntimeCost, 3);
	FRuntimeCardInstance ObsidianRuntime;
	TestTrue(TEXT("Obsidian runtime exists"),
		FWacomBattleSessionTestAccess::GetCardRuntimeState(
			Session.Get(),
			ObsidianCard->InstanceId,
			ObsidianRuntime));
	TestEqual(TEXT("Obsidian OnDraw doubles current Damage multiplier"),
		ObsidianRuntime.EffectMagnitudeMultipliers.FindRef(
			WacomTags::Effect_Damage),
		2.0f);
	const FBattleCardEffectMagnitudeSnapshot* ObsidianDamage =
		ObsidianCard->CurrentEffectMagnitudes.FindByPredicate(
			[](const FBattleCardEffectMagnitudeSnapshot& Candidate)
			{
				return Candidate.EffectIndex == 0
					&& Candidate.EffectType.MatchesTagExact(
						WacomTags::Effect_Damage);
			});
	if (TestNotNull(TEXT("Snapshot exposes Obsidian current Damage"),
		ObsidianDamage))
	{
		TestEqual(TEXT("Snapshot includes the OnDraw Damage multiplier"),
			ObsidianDamage->Magnitude, 10);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleFireWriteWarmAuraSpec,
	"Wacom.Battle.FireWriteCards.WarmAuraDoublesProvidedBonusOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleFireWriteWarmAuraSpec::RunTest(const FString&)
{
	using namespace WacomBattleFireWriteCardsSpec;
	FWacomBattleFixture Fixture;
	UCardDefinition* Warm = LoadCard(TEXT("WarmTinderbug"));
	UCardDefinition* Oil = LoadCard(TEXT("OilCandle"));
	UCardDefinition* Bottle = LoadCard(TEXT("EmptyBottle"));
	if (!TestTrue(TEXT("Warm aura assets load"), Warm && Oil && Bottle))
	{
		return false;
	}
	TArray<UCardDefinition*> Deck = { Warm, Oil, Bottle };
	PadDeck(Fixture, Deck);
	TStrongObjectPtr<UBattleSession> Session =
		CreateSession(Fixture, Deck);
	const FBattleSnapshot Before = Session->BuildSnapshot();
	const FGuid WarmId =
		FWacomBattleFixture::FindHandInstanceByCardId(
			Before, Warm->CardId);
	const FGuid OilId =
		FWacomBattleFixture::FindHandInstanceByCardId(
			Before, Oil->CardId);
	const FGuid BottleId =
		FWacomBattleFixture::FindHandInstanceByCardId(
			Before, Bottle->CardId);
	TestTrue(TEXT("Warm, Oil and Bottle are in hand"),
		WarmId.IsValid() && OilId.IsValid() && BottleId.IsValid());
	TestTrue(TEXT("Oil starts with card Burn"),
		FWacomBattleSessionTestAccess::SetCardStatusStacks(
			Session.Get(), OilId, WacomTags::Status_Burn, 1));
	TestTrue(TEXT("Warm play resolves"),
		Session->ResolveCommand(
			FBattleCommand::MakePlayCard(WarmId)).IsOk());

	FRuntimeCardInstance WarmRuntime;
	FRuntimeCardInstance OilRuntime;
	FRuntimeCardInstance BottleRuntime;
	TestTrue(TEXT("Warm runtime"), FWacomBattleSessionTestAccess::GetCardRuntimeState(
		Session.Get(), WarmId, WarmRuntime));
	TestTrue(TEXT("Oil runtime"), FWacomBattleSessionTestAccess::GetCardRuntimeState(
		Session.Get(), OilId, OilRuntime));
	TestTrue(TEXT("Bottle runtime"), FWacomBattleSessionTestAccess::GetCardRuntimeState(
		Session.Get(), BottleId, BottleRuntime));
	TestEqual(TEXT("Warm includes itself in the base aura"),
		WarmRuntime.EffectMagnitudeBonuses.FindRef(
			WacomTags::Effect_ApplyStatus_Burn),
		1);
	TestEqual(TEXT("Burned card doubles only the provided +1"),
		OilRuntime.EffectMagnitudeBonuses.FindRef(
			WacomTags::Effect_ApplyStatus_Burn),
		2);
	TestEqual(TEXT("Unburned hand card receives the base +1"),
		BottleRuntime.EffectMagnitudeBonuses.FindRef(
			WacomTags::Effect_ApplyStatus_Burn),
		1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleFireWriteCompanionPassivesSpec,
	"Wacom.Battle.FireWriteCards.CompanionAdjacencyAndOtherCompanion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleFireWriteCompanionPassivesSpec::RunTest(const FString&)
{
	using namespace WacomBattleFireWriteCardsSpec;
	FWacomBattleFixture Fixture;
	UCardDefinition* Blazing = LoadCard(TEXT("BlazingEyeFirefly"));
	UCardDefinition* Blind = LoadCard(TEXT("BlindSpider"));
	if (!TestTrue(TEXT("Companion passive assets load"),
		Blazing && Blind))
	{
		return false;
	}
	UCardDefinition* PlayedCompanion =
		Fixture.MakeDamageCardWithKeywords(
			0, 0, { WacomTags::Card_Keyword_Companion });
	TArray<UCardDefinition*> Deck = {
		Blazing,
		Blind,
		PlayedCompanion,
		Fixture.MakeDamageCardWithKeywords(
			0, 0, { WacomTags::Card_Keyword_Companion }),
		Fixture.MakeDamageCardWithKeywords(
			0, 0, { WacomTags::Card_Keyword_Companion }),
	};
	TStrongObjectPtr<UBattleSession> Session =
		CreateSession(Fixture, Deck, 97);
	const FBattleSnapshot Before = Session->BuildSnapshot();
	const FGuid BlazingId =
		FWacomBattleFixture::FindHandInstanceByCardId(
			Before, Blazing->CardId);
	const FGuid BlindId =
		FWacomBattleFixture::FindHandInstanceByCardId(
			Before, Blind->CardId);

	FGuid AdjacentCompanionId;
	const int32 BlazingIndex =
		FWacomBattleFixture::FindHandIndex(Before, BlazingId);
	for (const int32 NeighborIndex : {
		BlazingIndex - 1,
		BlazingIndex + 1 })
	{
		if (!Before.Hand.Cards.IsValidIndex(NeighborIndex))
		{
			continue;
		}
		const FHandCardSnapshot& Neighbor =
			Before.Hand.Cards[NeighborIndex];
		if (!Neighbor.bIsHandAnchor
			&& Neighbor.Definition
			&& Neighbor.Definition->Keywords.HasTagExact(
				WacomTags::Card_Keyword_Companion))
		{
			AdjacentCompanionId = Neighbor.InstanceId;
			break;
		}
	}
	if (!TestTrue(TEXT("Seeded hand has an adjacent companion"),
		AdjacentCompanionId.IsValid()))
	{
		return false;
	}
	const FBattleResolution Resolution = Session->ResolveCommand(
		FWacomBattleFixture::MakePlayCardOnPart(
			Before, AdjacentCompanionId, 0));
	TestTrue(TEXT("Adjacent companion resolves"), Resolution.IsOk());
	const FBattleEvent* BlazingMagnitudeEvent =
		Resolution.Events.FindByPredicate(
			[&BlazingId](const FBattleEvent& Event)
			{
				return Event.Type ==
						EBattleEventType::CardEffectMagnitudeChanged
					&& Event.CardInstanceId == BlazingId
					&& Event.Tag ==
						WacomTags::Effect_ApplyStatus_Burn;
			});
	TestNotNull(
		TEXT("Adjacent Burn gain publishes a Badge animation license"),
		BlazingMagnitudeEvent);

	const FHandCardSnapshot* BlazingAfter =
		FWacomBattleFixture::FindHandCardByCardId(
			Resolution.PostSnapshot,
			Blazing->CardId);
	if (TestNotNull(TEXT("Blazing remains in the post-command hand snapshot"),
		BlazingAfter))
	{
		const FBattleCardEffectMagnitudeSnapshot* BurnMagnitude =
			BlazingAfter->CurrentEffectMagnitudes.FindByPredicate(
				[](const FBattleCardEffectMagnitudeSnapshot& Candidate)
				{
					return Candidate.EffectIndex == 0
						&& Candidate.EffectType.MatchesTagExact(
							WacomTags::Effect_ApplyStatus_Burn);
				});
		if (TestNotNull(TEXT("Post-command snapshot exposes Blazing Burn magnitude"),
			BurnMagnitude))
		{
			TestEqual(TEXT("Post-command snapshot includes adjacent Burn bonus"),
				BurnMagnitude->Magnitude, 2);
		}
	}

	FRuntimeCardInstance BlazingRuntime;
	FRuntimeCardInstance BlindRuntime;
	TestTrue(TEXT("Blazing runtime"), FWacomBattleSessionTestAccess::GetCardRuntimeState(
		Session.Get(), BlazingId, BlazingRuntime));
	TestTrue(TEXT("Blind runtime"), FWacomBattleSessionTestAccess::GetCardRuntimeState(
		Session.Get(), BlindId, BlindRuntime));
	TestEqual(TEXT("Blazing gains White adjacent Burn bonus"),
		BlazingRuntime.EffectMagnitudeBonuses.FindRef(
			WacomTags::Effect_ApplyStatus_Burn),
		1);
	TestEqual(TEXT("Blind reacts to any other Companion"),
		BlindRuntime.RuntimeCostModifier, -1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleFireWriteAshTurnEndSpec,
	"Wacom.Battle.FireWriteCards.AshBugAutoPlaysFromExhaust",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleFireWriteAshTurnEndSpec::RunTest(const FString&)
{
	using namespace WacomBattleFireWriteCardsSpec;
	FWacomBattleFixture Fixture;
	UCardDefinition* Ash = LoadCard(TEXT("AshBug"));
	if (!TestNotNull(TEXT("Ash Bug asset loads"), Ash))
	{
		return false;
	}
	UCardDefinition* Exhauster =
		Fixture.MakeSelectedHandCardZoneMoveCard(0, true);
	TArray<UCardDefinition*> Deck = { Ash, Exhauster };
	PadDeck(Fixture, Deck);
	TStrongObjectPtr<UBattleSession> Session =
		CreateSession(Fixture, Deck);
	const FBattleSnapshot Before = Session->BuildSnapshot();
	const FGuid AshId =
		FWacomBattleFixture::FindHandInstanceByCardId(
			Before, Ash->CardId);
	const FGuid ExhausterId =
		FWacomBattleFixture::FindHandInstanceByCardId(
			Before, Exhauster->CardId);
	TestTrue(TEXT("Ash and exhauster are in hand"),
		AshId.IsValid() && ExhausterId.IsValid());
	TestTrue(TEXT("Ash is directly exhausted"),
		Session->ResolveCommand(
			FBattleCommand::MakePlayCardOnHandCard(
				ExhausterId, AshId)).IsOk());
	FRuntimeCardInstance AshRuntime;
	TestTrue(TEXT("Ash reaches exhaust"),
		FWacomBattleSessionTestAccess::GetCardRuntimeState(
			Session.Get(), AshId, AshRuntime)
			&& AshRuntime.Location == ECardLocation::Exhaust);

	const FBattleResolution EndTurn =
		Session->ResolveCommand(FBattleCommand::MakeEndTurn());
	TestTrue(TEXT("End turn succeeds"), EndTurn.IsOk());
	const FBattleEvent* AutoPlayed = EndTurn.Events.FindByPredicate(
		[AshId](const FBattleEvent& Event)
		{
			return Event.Type == EBattleEventType::CardPlayed
				&& Event.CardInstanceId == AshId
				&& Event.Amount == 0;
		});
	TestNotNull(TEXT("Ash auto-play is an explicit free action"),
		AutoPlayed);
	const FBattleEvent* AutoDestination =
		EndTurn.Events.FindByPredicate(
			[AshId](const FBattleEvent& Event)
			{
				return Event.Type
						== EBattleEventType::CardPlayDestinationResolved
					&& Event.CardInstanceId == AshId
					&& Event.CardDestination == ECardLocation::Discard;
			});
	TestNotNull(TEXT("Ash auto-play resolves to discard before deck cleanup"),
		AutoDestination);
	TestTrue(TEXT("Ash runtime remains inspectable"),
		FWacomBattleSessionTestAccess::GetCardRuntimeState(
			Session.Get(), AshId, AshRuntime));
	const FEnemyPartSnapshot* Part =
		FWacomBattleFixture::GetEnemyPartSnapshot(
			Session->BuildSnapshot(), 0);
	if (!TestNotNull(TEXT("Enemy survives for inspection"), Part))
	{
		return false;
	}
	TestEqual(TEXT("Auto-applied Burn resolves at action boundary"),
		Part->CurrentHp, 290);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleFireWriteOilSettlementSpec,
	"Wacom.Battle.FireWriteCards.OilCandleSettlementGrowth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleFireWriteOilSettlementSpec::RunTest(const FString&)
{
	using namespace WacomBattleFireWriteCardsSpec;
	FWacomBattleFixture Fixture;
	UCardDefinition* Oil = LoadCard(TEXT("OilCandle"));
	if (!TestNotNull(TEXT("Oil Candle asset loads"), Oil))
	{
		return false;
	}
	UCardDefinition* Exhauster =
		Fixture.MakeSelectedHandCardZoneMoveCard(0, true);
	TArray<UCardDefinition*> Cards = { Oil, Exhauster };
	PadDeck(Fixture, Cards);

	TStrongObjectPtr<UBattleSession> Session(
		NewObject<UBattleSession>());
	FBattleInitParams Params;
	Params.Character = Fixture.MakeCharacter(
		Fixture.MakeNoopCard(0),
		Fixture.MakeNoopCard(0),
		{});
	Params.RandomSeed = 103;
	const FGuid SourceRunId = FGuid::NewGuid();
	for (UCardDefinition* Definition : Cards)
	{
		FBattleDeckEntry& Entry =
			Params.BattleDeckEntries.AddDefaulted_GetRef();
		Entry.Definition = Definition;
		if (Definition == Oil)
		{
			Entry.SourceRunInstanceId = SourceRunId;
		}
	}
	FBattleEnemySlotInit& EnemySlot =
		Params.EnemySlots.AddDefaulted_GetRef();
	EnemySlot.EnemySlotId = TEXT("Enemy");
	EnemySlot.Enemy =
		Fixture.MakeSinglePartEnemyWithIntentDamage(300, 30, 0);
	TestTrue(TEXT("Settlement fixture initializes"),
		Session->Initialize(Params).IsOk());

	const FBattleSnapshot Before = Session->BuildSnapshot();
	const FGuid OilId =
		FWacomBattleFixture::FindHandInstanceByCardId(
			Before, Oil->CardId);
	const FGuid ExhausterId =
		FWacomBattleFixture::FindHandInstanceByCardId(
			Before, Exhauster->CardId);
	TestTrue(TEXT("Oil and exhauster are in hand"),
		OilId.IsValid() && ExhausterId.IsValid());
	TestTrue(TEXT("Oil enters exhaust before settlement"),
		Session->ResolveCommand(
			FBattleCommand::MakePlayCardOnHandCard(
				ExhausterId, OilId)).IsOk());

	FRuntimeCardInstance OilRuntime;
	TestTrue(TEXT("Exhaust history is latched"),
		FWacomBattleSessionTestAccess::GetCardRuntimeState(
			Session.Get(), OilId, OilRuntime)
			&& OilRuntime.bEverEnteredExhaust);
	TestTrue(TEXT("Settlement passive resolves"),
		FWacomBattleSessionTestAccess::ResolveSettlementPassives(
			Session.Get()));
	TestTrue(TEXT("Duplicate settlement resolution is idempotent"),
		FWacomBattleSessionTestAccess::ResolveSettlementPassives(
			Session.Get()));
	TestTrue(TEXT("Oil runtime remains inspectable"),
		FWacomBattleSessionTestAccess::GetCardRuntimeState(
			Session.Get(), OilId, OilRuntime));
	TestEqual(TEXT("Oil gains permanent durability once"),
		OilRuntime.PersistentModifiers.DurabilityBonus,
		1);
	TestEqual(TEXT("Oil gains permanent Burn magnitude once"),
		OilRuntime.PersistentModifiers.EffectMagnitudeBonuses.FindRef(
			WacomTags::Effect_ApplyStatus_Burn),
		1);

	const FBattleResultPacket Packet = Session->BuildResultPacket();
	const FBattlePersistentCardMutation* Mutation =
		Packet.PersistentCardMutations.FindByPredicate(
			[SourceRunId](const FBattlePersistentCardMutation& Candidate)
			{
				return Candidate.SourceRunInstanceId == SourceRunId;
			});
	if (!TestNotNull(TEXT("Result packet carries source Run mutation"),
		Mutation))
	{
		return false;
	}
	TestEqual(TEXT("Packet carries durability growth"),
		Mutation->PersistentModifiers.DurabilityBonus,
		1);
	TestEqual(TEXT("Packet carries Burn growth"),
		Mutation->PersistentModifiers.EffectMagnitudeBonuses.FindRef(
			WacomTags::Effect_ApplyStatus_Burn),
		1);
	return true;
}
