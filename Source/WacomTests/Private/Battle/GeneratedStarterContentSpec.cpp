// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Commands/BattleCommand.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Events/BattleEvent.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Fixtures/GeneratedBattleContentTestAssets.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomEnums.h"

namespace
{
	constexpr int32 MaxSeedSearch = 160;

	UCharacterDefinition* MakeGeneratedContentCharacter(
		FWacomBattleFixture& Fixture,
		const TArray<UCardDefinition*>& Deck)
	{
		return Fixture.MakeCharacter(
			Fixture.MakeNoopCard(/*Cost*/0),
			Fixture.MakeNoopCard(/*Cost*/0),
			Deck);
	}

	void PadDeckWithNoops(FWacomBattleFixture& Fixture, TArray<UCardDefinition*>& Deck, int32 MinimumDeckSize)
	{
		for (int32 Index = Deck.Num(); Index < MinimumDeckSize; ++Index)
		{
			Deck.Add(Fixture.MakeNoopCard(/*Cost*/0));
		}
	}

	UBattleSession* CreateGeneratedSession(
		FWacomBattleFixture& Fixture,
		UEnemyDefinition* Snake,
		const TArray<UCardDefinition*>& RequiredCards,
		int32 Seed,
		int32 MinimumDeckSize = 8,
		int32 PlayerFingerCount = 50)
	{
		TArray<UCardDefinition*> Deck = RequiredCards;
		PadDeckWithNoops(Fixture, Deck, MinimumDeckSize);
		UCharacterDefinition* Character = MakeGeneratedContentCharacter(Fixture, Deck);
		Character->FingerCount = PlayerFingerCount;
		return Fixture.CreateSession(
			Character,
			Snake,
			Seed);
	}

	UBattleSession* FindSessionWithHandCards(
		FWacomBattleFixture& Fixture,
		UEnemyDefinition* Snake,
		const TArray<UCardDefinition*>& RequiredCards,
		TFunctionRef<bool(const FBattleSnapshot&)> Predicate,
		FAutomationTestBase& Test,
		const TCHAR* ScenarioName,
		int32 MinimumDeckSize = 8)
	{
		for (int32 Seed = 1; Seed <= MaxSeedSearch; ++Seed)
		{
			UBattleSession* Session = CreateGeneratedSession(
				Fixture,
				Snake,
				RequiredCards,
				Seed,
				MinimumDeckSize);
			if (Predicate(Session->BuildSnapshot()))
			{
				return Session;
			}
		}

		Test.AddError(FString::Printf(
			TEXT("%s: no deterministic seed in [1,%d] produced the required opening hand"),
			ScenarioName,
			MaxSeedSearch));
		return nullptr;
	}

	bool LoadCoreGeneratedAssets(
		FAutomationTestBase& Test,
		UCardDefinition*& OutPoisonNeedle,
		UCardDefinition*& OutChitinWard,
		UCardDefinition*& OutAntennaSearch,
		UCardDefinition*& OutMoltCut,
		UCardDefinition*& OutLightHusk,
		UCardDefinition*& OutSilklineFeint,
		UCardDefinition*& OutPoisonFang,
		UCardDefinition*& OutFuxiaoFeie,
		UCardDefinition*& OutDiscardSelected,
		UEnemyDefinition*& OutSnake)
	{
		OutPoisonNeedle = FWacomGeneratedBattleContentAssets::LoadPoisonNeedle(Test);
		OutChitinWard = FWacomGeneratedBattleContentAssets::LoadChitinWard(Test);
		OutAntennaSearch = FWacomGeneratedBattleContentAssets::LoadAntennaSearch(Test);
		OutMoltCut = FWacomGeneratedBattleContentAssets::LoadMoltCut(Test);
		OutLightHusk = FWacomGeneratedBattleContentAssets::LoadLightHusk(Test);
		OutSilklineFeint = FWacomGeneratedBattleContentAssets::LoadSilklineFeint(Test);
		OutPoisonFang = FWacomGeneratedBattleContentAssets::LoadPoisonFang(Test);
		OutFuxiaoFeie = FWacomGeneratedBattleContentAssets::LoadFuxiaoFeie(Test);
		OutDiscardSelected = FWacomGeneratedBattleContentAssets::LoadDiscardSelectedHandCard(Test);
		OutSnake = FWacomGeneratedBattleContentAssets::LoadSnake(Test);

		return OutPoisonNeedle
			&& OutChitinWard
			&& OutAntennaSearch
			&& OutMoltCut
			&& OutLightHusk
			&& OutSilklineFeint
			&& OutPoisonFang
			&& OutFuxiaoFeie
			&& OutDiscardSelected
			&& OutSnake;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleGeneratedStarterCardsExecuteSpec,
	"Wacom.Battle.GeneratedStarterContent.GeneratedStarterCardsExecuteInBattleSession",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleGeneratedStarterCardsExecuteSpec::RunTest(const FString& /*Parameters*/)
{
	UCardDefinition* PoisonNeedle = nullptr;
	UCardDefinition* ChitinWard = nullptr;
	UCardDefinition* AntennaSearch = nullptr;
	UCardDefinition* MoltCut = nullptr;
	UCardDefinition* LightHusk = nullptr;
	UCardDefinition* SilklineFeint = nullptr;
	UCardDefinition* PoisonFang = nullptr;
	UCardDefinition* FuxiaoFeie = nullptr;
	UCardDefinition* DiscardSelected = nullptr;
	UEnemyDefinition* Snake = nullptr;
	if (!LoadCoreGeneratedAssets(
		*this,
		PoisonNeedle,
		ChitinWard,
		AntennaSearch,
		MoltCut,
		LightHusk,
		SilklineFeint,
		PoisonFang,
		FuxiaoFeie,
		DiscardSelected,
		Snake))
	{
		return false;
	}
	UCardDefinition* DrawByCost = FWacomGeneratedBattleContentAssets::LoadDrawByCostCard(*this);
	if (!TestNotNull(TEXT("DrawByCost loads"), DrawByCost))
	{
		return false;
	}

	{
		FWacomBattleFixture Fixture;
		UBattleSession* Session = FindSessionWithHandCards(
			Fixture,
			Snake,
			{ PoisonNeedle },
			[PoisonNeedle](const FBattleSnapshot& Snapshot)
			{
				return FWacomBattleFixture::FindHandCardByCardId(Snapshot, PoisonNeedle->CardId) != nullptr
					&& FWacomBattleFixture::FindPartByPartId(Snapshot, TEXT("Snake.Head")) != nullptr;
			},
			*this,
			TEXT("PoisonNeedle basic damage"));
		if (!Session)
		{
			return false;
		}

		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, PoisonNeedle->CardId);
		const FEnemyPartSnapshot* HeadBefore = FWacomBattleFixture::FindPartByPartId(Snapshot, TEXT("Snake.Head"));
		if (!TestNotNull(TEXT("Snake.Head exists before PoisonNeedle"), HeadBefore))
		{
			return false;
		}
		const FGuid HeadInstanceId = HeadBefore->InstanceId;
		const int32 HeadHpBefore = HeadBefore->CurrentHp;
		TestTrue(TEXT("PoisonNeedle is in hand"), CardId.IsValid());
		const FBattleResolution Resolution = Session->ResolveCommand(
			FBattleCommand::MakePlayCardOnEnemyPartKey(CardId, HeadBefore->PartKey));
		TestTrue(TEXT("Play PoisonNeedle without poison"), Resolution.IsOk());
		const TArray<FBattleEvent>& Events = Resolution.Events;
		Snapshot = Session->BuildSnapshot();
		const FEnemyPartSnapshot* HeadAfter = FWacomBattleFixture::FindPartByPartId(Snapshot, TEXT("Snake.Head"));
		if (!TestNotNull(TEXT("Snake.Head exists after PoisonNeedle"), HeadAfter))
		{
			return false;
		}
		TestEqual(TEXT("PoisonNeedle deals base damage when target has no poison"),
			HeadHpBefore - HeadAfter->CurrentHp,
			4);
		TestEqual(TEXT("PoisonNeedle emits one non-poison damage event"),
			FWacomBattleFixture::CountEventsWithTag(Events, EBattleEventType::DamageDealt, WacomTags::Status_Poison),
			0);
	}

	{
		FWacomBattleFixture Fixture;
		UBattleSession* Session = FindSessionWithHandCards(
			Fixture,
			Snake,
			{ ChitinWard },
			[ChitinWard](const FBattleSnapshot& Snapshot)
			{
				return FWacomBattleFixture::FindHandInstanceByCardIdInZone(Snapshot, ChitinWard->CardId, EHandZone::Both).IsValid();
			},
			*this,
			TEXT("ChitinWard shield and heal"));
		if (!Session)
		{
			return false;
		}

		TestTrue(TEXT("EndTurn keeps Both-zone ChitinWard while real Snake damages player"),
			Session->ResolveCommand(FBattleCommand::MakeEndTurn()).IsOk());
		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const int32 HpBefore = Snapshot.Player.CurrentHp;
		const int32 ShieldBefore = Snapshot.Player.Shield;
		TestTrue(TEXT("Player was damaged before ChitinWard"), HpBefore < Snapshot.Player.MaxHp);

		const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, ChitinWard->CardId);
		TestTrue(TEXT("ChitinWard remains in hand after one EndTurn"), CardId.IsValid());
		TestTrue(TEXT("Play ChitinWard"), Session->ResolveCommand(FBattleCommand::MakePlayCard(CardId)).IsOk());
		Snapshot = Session->BuildSnapshot();
		TestEqual(TEXT("ChitinWard adds player shield"), Snapshot.Player.Shield - ShieldBefore, 5);
		TestEqual(TEXT("ChitinWard heals player HP"), Snapshot.Player.CurrentHp - HpBefore, 2);
	}

	{
		FWacomBattleFixture Fixture;
		UBattleSession* Session = FindSessionWithHandCards(
			Fixture,
			Snake,
			{ AntennaSearch },
			[AntennaSearch](const FBattleSnapshot& Snapshot)
			{
				return FWacomBattleFixture::FindHandCardByCardId(Snapshot, AntennaSearch->CardId) != nullptr
					&& Snapshot.PileCounts.DrawCount >= 2;
			},
			*this,
			TEXT("AntennaSearch draw and discard"),
			/*MinimumDeckSize*/ 12);
		if (!Session)
		{
			return false;
		}

		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const int32 DrawBefore = Snapshot.PileCounts.DrawCount;
		const int32 DiscardBefore = Snapshot.PileCounts.DiscardCount;
		const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, AntennaSearch->CardId);
		TestTrue(TEXT("AntennaSearch is in hand"), CardId.IsValid());
		const FBattleResolution Resolution =
			Session->ResolveCommand(FBattleCommand::MakePlayCard(CardId));
		TestTrue(TEXT("Play AntennaSearch"), Resolution.IsOk());
		const TArray<FBattleEvent>& Events = Resolution.Events;
		Snapshot = Session->BuildSnapshot();
		TestEqual(TEXT("AntennaSearch draws two cards from draw pile"), FWacomBattleFixture::CountEvents(Events, EBattleEventType::CardsDrawn), 1);
		TestEqual(TEXT("AntennaSearch draw event count"), Events.ContainsByPredicate([](const FBattleEvent& Event)
		{
			return Event.Type == EBattleEventType::CardsDrawn
				&& Event.Count == 2
				&& Event.CardInstanceIds.Num() == Event.Count;
		}), true);
		TestTrue(TEXT("AntennaSearch emits random discard event"),
			FWacomBattleFixture::CountEvents(Events, EBattleEventType::CardDiscarded) >= 1);
		TestEqual(TEXT("AntennaSearch net draw pile consumption"), DrawBefore - Snapshot.PileCounts.DrawCount, 2);
		TestTrue(TEXT("AntennaSearch discard pile increases by random discard"),
			Snapshot.PileCounts.DiscardCount >= DiscardBefore + 1);
		TestEqual(TEXT("AntennaSearch waits in played pile"),
			Snapshot.PileCounts.PlayedCount,
			1);
	}

	{
		FWacomBattleFixture Fixture;
		UBattleSession* Session = FindSessionWithHandCards(
			Fixture,
			Snake,
			{ DrawByCost },
			[DrawByCost](const FBattleSnapshot& Snapshot)
			{
				return FWacomBattleFixture::FindHandCardByCardId(Snapshot, DrawByCost->CardId) != nullptr
					&& Snapshot.PileCounts.DrawCount >= 2;
			},
			*this,
			TEXT("DrawByCost runtime-cost draw"),
			/*MinimumDeckSize*/ 12);
		if (!Session)
		{
			return false;
		}

		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const int32 DrawBefore = Snapshot.PileCounts.DrawCount;
		const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, DrawByCost->CardId);
		TestTrue(TEXT("DrawByCost is in hand"), CardId.IsValid());
		const FBattleResolution Resolution =
			Session->ResolveCommand(FBattleCommand::MakePlayCard(CardId));
		TestTrue(TEXT("Play DrawByCost"), Resolution.IsOk());
		const TArray<FBattleEvent>& Events = Resolution.Events;
		Snapshot = Session->BuildSnapshot();
		TestEqual(TEXT("DrawByCost emits draw count equal to default cost"), Events.ContainsByPredicate([](const FBattleEvent& Event)
		{
			return Event.Type == EBattleEventType::CardsDrawn
				&& Event.Count == 2
				&& Event.CardInstanceIds.Num() == Event.Count;
		}), true);
		TestEqual(TEXT("DrawByCost consumes two cards from draw pile"),
			DrawBefore - Snapshot.PileCounts.DrawCount,
			2);
	}

	{
		FWacomBattleFixture Fixture;
		UBattleSession* Session = FindSessionWithHandCards(
			Fixture,
			Snake,
			{ FuxiaoFeie, MoltCut },
			[FuxiaoFeie, MoltCut](const FBattleSnapshot& Snapshot)
			{
				return FWacomBattleFixture::FindHandCardByCardId(Snapshot, FuxiaoFeie->CardId) != nullptr
					&& FWacomBattleFixture::FindHandCardByCardId(Snapshot, MoltCut->CardId) != nullptr
					&& FWacomBattleFixture::FindPartByPartId(Snapshot, TEXT("Snake.Body")) != nullptr;
			},
			*this,
			TEXT("MoltCut removes slow and lowers initiative"));
		if (!Session)
		{
			return false;
		}

		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const FEnemyPartSnapshot* BodyBefore = FWacomBattleFixture::FindPartByPartId(Snapshot, TEXT("Snake.Body"));
		if (!TestNotNull(TEXT("Snake.Body exists before Fuxiao/MoltCut"), BodyBefore))
		{
			return false;
		}
		const int32 InitiativeBeforeFuxiao = BodyBefore->CurrentInitiative;

		FGuid FuxiaoId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, FuxiaoFeie->CardId);
		TestTrue(TEXT("FuxiaoFeie is in hand"), FuxiaoId.IsValid());
		TestTrue(TEXT("Play FuxiaoFeie to apply Slow"),
			Session->ResolveCommand(FBattleCommand::MakePlayCardOnEnemyPartKey(FuxiaoId, BodyBefore->PartKey)).IsOk());
		Snapshot = Session->BuildSnapshot();
		const FEnemyPartSnapshot* BodyAfterSlow = FWacomBattleFixture::FindPartByPartId(Snapshot, TEXT("Snake.Body"));
		if (!TestNotNull(TEXT("Snake.Body exists after Fuxiao"), BodyAfterSlow))
		{
			return false;
		}
		TestEqual(TEXT("FuxiaoFeie Slow delays the current intent immediately"),
			BodyAfterSlow->CurrentInitiative,
			InitiativeBeforeFuxiao + 1);
		TestEqual(TEXT("Enemy Slow does not leave a redundant stack"),
			FWacomBattleFixture::GetStatusStacks(BodyAfterSlow->StatusStacks, WacomTags::Status_Slow),
			0);
		const int32 InitiativeBeforeMolt = BodyAfterSlow->CurrentInitiative;

		const FGuid MoltCutId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, MoltCut->CardId);
		TestTrue(TEXT("MoltCut is in hand"), MoltCutId.IsValid());
		TestTrue(TEXT("Play MoltCut"),
			Session->ResolveCommand(FBattleCommand::MakePlayCardOnEnemyPartKey(MoltCutId, BodyAfterSlow->PartKey)).IsOk());
		Snapshot = Session->BuildSnapshot();
		const FEnemyPartSnapshot* BodyAfterMolt = FWacomBattleFixture::FindPartByPartId(Snapshot, TEXT("Snake.Body"));
		if (!TestNotNull(TEXT("Snake.Body exists after MoltCut"), BodyAfterMolt))
		{
			return false;
		}
		TestEqual(TEXT("MoltCut leaves no legacy Slow stack"), FWacomBattleFixture::GetStatusStacks(BodyAfterMolt->StatusStacks, WacomTags::Status_Slow), 0);
		TestEqual(TEXT("MoltCut lowers initiative by modify effect plus normal cost push"),
			InitiativeBeforeMolt - BodyAfterMolt->CurrentInitiative,
			3);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleGeneratedStarterConditionalPassiveZoneHookSpec,
	"Wacom.Battle.GeneratedStarterContent.GeneratedStarterCardsRespectConditionalPassiveAndZoneHookRuntime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleGeneratedStarterConditionalPassiveZoneHookSpec::RunTest(const FString& /*Parameters*/)
{
	UCardDefinition* PoisonNeedle = nullptr;
	UCardDefinition* ChitinWard = nullptr;
	UCardDefinition* AntennaSearch = nullptr;
	UCardDefinition* MoltCut = nullptr;
	UCardDefinition* LightHusk = nullptr;
	UCardDefinition* SilklineFeint = nullptr;
	UCardDefinition* PoisonFang = nullptr;
	UCardDefinition* FuxiaoFeie = nullptr;
	UCardDefinition* DiscardSelected = nullptr;
	UEnemyDefinition* Snake = nullptr;
	if (!LoadCoreGeneratedAssets(
		*this,
		PoisonNeedle,
		ChitinWard,
		AntennaSearch,
		MoltCut,
		LightHusk,
		SilklineFeint,
		PoisonFang,
		FuxiaoFeie,
		DiscardSelected,
		Snake))
	{
		return false;
	}

	{
		FWacomBattleFixture Fixture;
		UBattleSession* Session = FindSessionWithHandCards(
			Fixture,
			Snake,
			{ PoisonFang, PoisonNeedle },
			[PoisonFang, PoisonNeedle](const FBattleSnapshot& Snapshot)
			{
				return FWacomBattleFixture::FindHandCardByCardId(Snapshot, PoisonFang->CardId) != nullptr
					&& FWacomBattleFixture::FindHandCardByCardId(Snapshot, PoisonNeedle->CardId) != nullptr
					&& FWacomBattleFixture::FindPartByPartId(Snapshot, TEXT("Snake.Body")) != nullptr;
			},
			*this,
			TEXT("PoisonNeedle conditional damage"));
		if (!Session)
		{
			return false;
		}

		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const FEnemyPartSnapshot* TargetPart = FWacomBattleFixture::FindPartByPartId(Snapshot, TEXT("Snake.Body"));
		if (!TestNotNull(TEXT("Snake.Body exists for conditional PoisonNeedle"), TargetPart))
		{
			return false;
		}

		FGuid PoisonFangId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, PoisonFang->CardId);
		TestTrue(TEXT("PoisonFang is in hand"), PoisonFangId.IsValid());
		TestTrue(TEXT("Play PoisonFang"),
			Session->ResolveCommand(FBattleCommand::MakePlayCardOnEnemyPartKey(PoisonFangId, TargetPart->PartKey)).IsOk());
		Snapshot = Session->BuildSnapshot();
		TargetPart = FWacomBattleFixture::FindPartByPartId(Snapshot, TEXT("Snake.Body"));
		if (!TestNotNull(TEXT("Snake.Body exists after PoisonFang"), TargetPart))
		{
			return false;
		}
		TestEqual(TEXT("PoisonFang leaves Poison on target"),
			FWacomBattleFixture::GetStatusStacks(TargetPart->StatusStacks, WacomTags::Status_Poison),
			1);
		const int32 HpBeforeNeedle = TargetPart->CurrentHp;
		const FGuid TargetPartInstanceId = TargetPart->InstanceId;

		const FGuid PoisonNeedleId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, PoisonNeedle->CardId);
		TestTrue(TEXT("PoisonNeedle is in hand after PoisonFang"), PoisonNeedleId.IsValid());
		const FBattleResolution Resolution = Session->ResolveCommand(
			FBattleCommand::MakePlayCardOnEnemyPartKey(PoisonNeedleId, TargetPart->PartKey));
		TestTrue(TEXT("Play PoisonNeedle against poisoned target"), Resolution.IsOk());
		const TArray<FBattleEvent>& Events = Resolution.Events;
		Snapshot = Session->BuildSnapshot();
		TargetPart = FWacomBattleFixture::FindPartByPartId(Snapshot, TEXT("Snake.Body"));
		if (!TestNotNull(TEXT("Snake.Body exists after conditional PoisonNeedle"), TargetPart))
		{
			return false;
		}
		TestEqual(TEXT("PoisonNeedle card damage includes conditional bonus"),
			FWacomBattleFixture::SumDamageEventsForCard(Events, PoisonNeedleId),
			9);
		TestTrue(TEXT("PoisonNeedle follow-up poison contributes additional HP loss"),
			HpBeforeNeedle - TargetPart->CurrentHp > 9);
		TestTrue(TEXT("Conditional PoisonNeedle still includes poison tick event"),
			FWacomBattleFixture::HasEventForActor(Events, EBattleEventType::DamageDealt, TargetPartInstanceId, WacomTags::Status_Poison));
	}

	{
		FWacomBattleFixture Fixture;
		UBattleSession* Session = FindSessionWithHandCards(
			Fixture,
			Snake,
			{ DiscardSelected, LightHusk },
			[DiscardSelected, LightHusk](const FBattleSnapshot& Snapshot)
			{
				return FWacomBattleFixture::FindHandCardByCardId(Snapshot, DiscardSelected->CardId) != nullptr
					&& FWacomBattleFixture::FindHandCardByCardId(Snapshot, LightHusk->CardId) != nullptr;
			},
			*this,
			TEXT("LightHusk OnDiscard passive"));
		if (!Session)
		{
			return false;
		}

		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, DiscardSelected->CardId);
		const FGuid TargetId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, LightHusk->CardId);
		TestTrue(TEXT("DiscardSelected is in hand"), SourceId.IsValid());
		TestTrue(TEXT("LightHusk is in hand"), TargetId.IsValid());
		const FBattleResolution Resolution = Session->ResolveCommand(
			FBattleCommand::MakePlayCardOnHandCard(SourceId, TargetId));
		TestTrue(TEXT("DiscardSelected discards LightHusk"), Resolution.IsOk());
		const TArray<FBattleEvent>& Events = Resolution.Events;
		Snapshot = Session->BuildSnapshot();
		TestEqual(TEXT("LightHusk OnDiscard grants shield"), Snapshot.Player.Shield, 4);
		TestEqual(TEXT("Selected discard emitted one discard event"), FWacomBattleFixture::CountEvents(Events, EBattleEventType::CardDiscarded), 1);
	}

	{
		FWacomBattleFixture Fixture;
		UBattleSession* Session = FindSessionWithHandCards(
			Fixture,
			Snake,
			{ SilklineFeint },
			[SilklineFeint](const FBattleSnapshot& Snapshot)
			{
				return FWacomBattleFixture::FindHandInstanceByCardIdInZone(Snapshot, SilklineFeint->CardId, EHandZone::Left).IsValid()
					&& FWacomBattleFixture::FindPartByPartId(Snapshot, TEXT("Snake.Tail")) != nullptr;
			},
			*this,
			TEXT("SilklineFeint left-zone perfect release"));
		if (!Session)
		{
			return false;
		}

		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardIdInZone(Snapshot, SilklineFeint->CardId, EHandZone::Left);
		const FEnemyPartSnapshot* TailBefore = FWacomBattleFixture::FindPartByPartId(Snapshot, TEXT("Snake.Tail"));
		if (!TestNotNull(TEXT("Snake.Tail exists before SilklineFeint"), TailBefore))
		{
			return false;
		}
		const FGuid TailInstanceId = TailBefore->InstanceId;
		const int32 TailInitiativeBefore = TailBefore->CurrentInitiative;
		const int32 TailHpBefore = TailBefore->CurrentHp;
		TestEqual(TEXT("Tail starts at perfect-release initiative for SilklineFeint"),
			TailInitiativeBefore,
			1);
		const FBattleResolution Resolution = Session->ResolveCommand(
			FBattleCommand::MakePlayCardOnEnemyPartKey(CardId, TailBefore->PartKey));
		TestTrue(TEXT("Play left-zone SilklineFeint on Tail"), Resolution.IsOk());
		const TArray<FBattleEvent>& Events = Resolution.Events;
		Snapshot = Session->BuildSnapshot();
		const FEnemyPartSnapshot* TailAfter = FWacomBattleFixture::FindPartByPartId(Snapshot, TEXT("Snake.Tail"));
		if (!TestNotNull(TEXT("Snake.Tail exists after SilklineFeint"), TailAfter))
		{
			return false;
		}
		TestTrue(TEXT("SilklineFeint emits InitiativeHit"), FWacomBattleFixture::HasEventForActor(
			Events,
			EBattleEventType::InitiativeHit,
			TailInstanceId));
		TestEqual(TEXT("SilklineFeint empty perfect hook skips initiative push"),
			FWacomBattleFixture::CountEvents(Events, EBattleEventType::InitiativePushed),
			0);
		TestEqual(TEXT("SilklineFeint deals damage but leaves initiative unchanged"),
			TailAfter->CurrentInitiative,
			TailInitiativeBefore);
		TestEqual(TEXT("SilklineFeint damage"), TailHpBefore - TailAfter->CurrentHp, 3);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleGeneratedSnakeIntentVariantsSpec,
	"Wacom.Battle.GeneratedStarterContent.GeneratedSnakeIntentVariantsExecuteOnEndTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleGeneratedSnakeIntentVariantsSpec::RunTest(const FString& /*Parameters*/)
{
	UCardDefinition* PoisonNeedle = nullptr;
	UCardDefinition* ChitinWard = nullptr;
	UCardDefinition* AntennaSearch = nullptr;
	UCardDefinition* MoltCut = nullptr;
	UCardDefinition* LightHusk = nullptr;
	UCardDefinition* SilklineFeint = nullptr;
	UCardDefinition* PoisonFang = nullptr;
	UCardDefinition* FuxiaoFeie = nullptr;
	UCardDefinition* DiscardSelected = nullptr;
	UEnemyDefinition* Snake = nullptr;
	if (!LoadCoreGeneratedAssets(
		*this,
		PoisonNeedle,
		ChitinWard,
		AntennaSearch,
		MoltCut,
		LightHusk,
		SilklineFeint,
		PoisonFang,
		FuxiaoFeie,
		DiscardSelected,
		Snake))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UBattleSession* Session = CreateGeneratedSession(
		Fixture,
		Snake,
		{ Fixture.MakeNoopCard(/*Cost*/0), Fixture.MakeNoopCard(/*Cost*/0) },
		/*Seed*/ 1,
		/*MinimumDeckSize*/ 8,
		/*PlayerFingerCount*/ 500);

	bool bSawPlayerDamage = false;
	bool bSawPlayerPoison = false;
	bool bSawPlayerSlow = false;
	bool bSawEnemyShield = false;

	for (int32 Turn = 0; Turn < 4; ++Turn)
	{
		const FBattleSnapshot Before = Session->BuildSnapshot();
		const FBattleResolution Resolution = Session->ResolveCommand(FBattleCommand::MakeEndTurn());
		TestTrue(*FString::Printf(TEXT("EndTurn %d succeeds"), Turn + 1), Resolution.IsOk());
		const TArray<FBattleEvent>& Events = Resolution.Events;
		const FBattleSnapshot After = Session->BuildSnapshot();

		bSawPlayerDamage = bSawPlayerDamage
			|| After.Player.CurrentHp < Before.Player.CurrentHp
			|| Events.ContainsByPredicate([](const FBattleEvent& Event)
			{
				return Event.Type == EBattleEventType::DamageDealt && !Event.ActorInstanceId.IsValid();
			});
		bSawPlayerPoison = bSawPlayerPoison
			|| FWacomBattleFixture::GetStatusStacks(After.Player.StatusStacks, WacomTags::Status_Poison) > 0;
		bSawPlayerSlow = bSawPlayerSlow
			|| After.Hand.Cards.ContainsByPredicate([](const FHandCardSnapshot& Card)
			{
				return FWacomBattleFixture::GetStatusStacks(
					Card.StatusStacks,
					WacomTags::Status_Slow) > 0;
			});
		for (const FEnemySnapshot& EnemySnapshot : After.Enemies)
		{
			for (const FEnemyPartSnapshot& Part : EnemySnapshot.Parts)
			{
				if (Part.Shield > 0)
				{
					bSawEnemyShield = true;
					break;
				}
			}
			if (bSawEnemyShield)
			{
				break;
			}
		}

		TestTrue(*FString::Printf(TEXT("EndTurn %d emits enemy action events"), Turn + 1),
			FWacomBattleFixture::CountEvents(Events, EBattleEventType::EnemyPartActed) > 0);
		if (After.Phase == EBattlePhase::BattleEnd)
		{
			break;
		}
	}

	const FBattleSnapshot Final = Session->BuildSnapshot();
	TestTrue(TEXT("Generated Snake intent sequence deals player damage"), bSawPlayerDamage);
	TestTrue(TEXT("Generated Snake intent sequence applies Poison to player"), bSawPlayerPoison);
	TestTrue(TEXT("Generated Snake intent sequence materializes Slow on a hand card"), bSawPlayerSlow);
	TestTrue(TEXT("Generated Snake intent sequence applies self Shield"), bSawEnemyShield);
	TestTrue(TEXT("Elevated HP fixture survives generated Snake smoke"), Final.Player.CurrentHp > 0);
	return true;
}
