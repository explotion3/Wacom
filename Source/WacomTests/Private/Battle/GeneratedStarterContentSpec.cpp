// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Commands/BattleCommand.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Events/BattleEvent.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomEnums.h"

namespace
{
	constexpr int32 MaxSeedSearch = 160;

	const TCHAR* PoisonNeedlePath =
		TEXT("/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_PoisonNeedle.DA_Card_Starter_PoisonNeedle");
	const TCHAR* ChitinWardPath =
		TEXT("/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_ChitinWard.DA_Card_Starter_ChitinWard");
	const TCHAR* AntennaSearchPath =
		TEXT("/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_AntennaSearch.DA_Card_Starter_AntennaSearch");
	const TCHAR* MoltCutPath =
		TEXT("/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_MoltCut.DA_Card_Starter_MoltCut");
	const TCHAR* LightHuskPath =
		TEXT("/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_LightHusk.DA_Card_Starter_LightHusk");
	const TCHAR* SilklineFeintPath =
		TEXT("/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_SilklineFeint.DA_Card_Starter_SilklineFeint");
	const TCHAR* PoisonFangPath =
		TEXT("/Game/Wacom/Data/Cards/Rewards/DA_Card_PoisonFang.DA_Card_PoisonFang");
	const TCHAR* FuxiaoFeiePath =
		TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_FuxiaoFeie.DA_Card_FuxiaoFeie");
	const TCHAR* DiscardSelectedPath =
		TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_Test_DiscardSelectedHandCard.DA_Card_Test_DiscardSelectedHandCard");
	const TCHAR* SnakePath =
		TEXT("/Game/Wacom/Data/Enemies/Snake/DA_Enemy_Snake.DA_Enemy_Snake");

	template <typename T>
	T* LoadRequiredAsset(const TCHAR* Path, FAutomationTestBase& Test)
	{
		T* Asset = LoadObject<T>(nullptr, Path);
		Test.TestNotNull(FString::Printf(TEXT("Asset loads: %s"), Path), Asset);
		return Asset;
	}

	int32 CountEvents(const TArray<FBattleEvent>& Events, EBattleEventType Type)
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

	int32 CountEventsWithTag(const TArray<FBattleEvent>& Events, EBattleEventType Type, const FGameplayTag& Tag)
	{
		int32 Count = 0;
		for (const FBattleEvent& Event : Events)
		{
			if (Event.Type == Type && Event.Tag == Tag)
			{
				++Count;
			}
		}
		return Count;
	}

	bool HasEventForActor(
		const TArray<FBattleEvent>& Events,
		EBattleEventType Type,
		const FGuid& ActorInstanceId,
		const FGameplayTag& Tag = FGameplayTag())
	{
		for (const FBattleEvent& Event : Events)
		{
			if (Event.Type != Type)
			{
				continue;
			}
			if (Event.ActorInstanceId != ActorInstanceId)
			{
				continue;
			}
			if (Tag.IsValid() && Event.Tag != Tag)
			{
				continue;
			}
			return true;
		}
		return false;
	}

	int32 SumDamageEventsForCard(const TArray<FBattleEvent>& Events, const FGuid& CardInstanceId)
	{
		int32 Total = 0;
		for (const FBattleEvent& Event : Events)
		{
			if (Event.Type == EBattleEventType::DamageDealt && Event.CardInstanceId == CardInstanceId)
			{
				Total += Event.Amount;
			}
		}
		return Total;
	}

	int32 GetStatusStacks(const TMap<FGameplayTag, int32>& StatusStacks, const FGameplayTag& StatusTag)
	{
		if (const int32* Stacks = StatusStacks.Find(StatusTag))
		{
			return *Stacks;
		}
		return 0;
	}

	const FEnemyPartSnapshot* FindPartByPartId(const FBattleSnapshot& Snapshot, FName PartId)
	{
		for (const FEnemyPartSnapshot& Part : Snapshot.Enemy.Parts)
		{
			if (Part.Definition && Part.Definition->PartId == PartId)
			{
				return &Part;
			}
		}
		return nullptr;
	}

	FGuid FindPartInstanceByPartId(const FBattleSnapshot& Snapshot, FName PartId)
	{
		if (const FEnemyPartSnapshot* Part = FindPartByPartId(Snapshot, PartId))
		{
			return Part->InstanceId;
		}
		return FGuid();
	}

	const FHandCardSnapshot* FindHandCardByCardId(const FBattleSnapshot& Snapshot, FName CardId)
	{
		for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
		{
			if (Card.Definition && Card.Definition->CardId == CardId)
			{
				return &Card;
			}
		}
		return nullptr;
	}

	FGuid FindCardInstanceInZone(const FBattleSnapshot& Snapshot, FName CardId, EHandZone Zone)
	{
		for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
		{
			if (Card.bIsHandAnchor)
			{
				continue;
			}
			if (Card.Zone != Zone)
			{
				continue;
			}
			if (Card.Definition && Card.Definition->CardId == CardId)
			{
				return Card.InstanceId;
			}
		}
		return FGuid();
	}

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
		int32 MinimumDeckSize = 8)
	{
		TArray<UCardDefinition*> Deck = RequiredCards;
		PadDeckWithNoops(Fixture, Deck, MinimumDeckSize);
		return Fixture.CreateSession(
			MakeGeneratedContentCharacter(Fixture, Deck),
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
		OutPoisonNeedle = LoadRequiredAsset<UCardDefinition>(PoisonNeedlePath, Test);
		OutChitinWard = LoadRequiredAsset<UCardDefinition>(ChitinWardPath, Test);
		OutAntennaSearch = LoadRequiredAsset<UCardDefinition>(AntennaSearchPath, Test);
		OutMoltCut = LoadRequiredAsset<UCardDefinition>(MoltCutPath, Test);
		OutLightHusk = LoadRequiredAsset<UCardDefinition>(LightHuskPath, Test);
		OutSilklineFeint = LoadRequiredAsset<UCardDefinition>(SilklineFeintPath, Test);
		OutPoisonFang = LoadRequiredAsset<UCardDefinition>(PoisonFangPath, Test);
		OutFuxiaoFeie = LoadRequiredAsset<UCardDefinition>(FuxiaoFeiePath, Test);
		OutDiscardSelected = LoadRequiredAsset<UCardDefinition>(DiscardSelectedPath, Test);
		OutSnake = LoadRequiredAsset<UEnemyDefinition>(SnakePath, Test);

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

	{
		FWacomBattleFixture Fixture;
		UBattleSession* Session = FindSessionWithHandCards(
			Fixture,
			Snake,
			{ PoisonNeedle },
			[PoisonNeedle](const FBattleSnapshot& Snapshot)
			{
				return FindHandCardByCardId(Snapshot, PoisonNeedle->CardId) != nullptr
					&& FindPartByPartId(Snapshot, TEXT("Snake.Head")) != nullptr;
			},
			*this,
			TEXT("PoisonNeedle basic damage"));
		if (!Session)
		{
			return false;
		}

		Session->ConsumeEvents();
		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, PoisonNeedle->CardId);
		const FEnemyPartSnapshot* HeadBefore = FindPartByPartId(Snapshot, TEXT("Snake.Head"));
		if (!TestNotNull(TEXT("Snake.Head exists before PoisonNeedle"), HeadBefore))
		{
			return false;
		}
		const FGuid HeadInstanceId = HeadBefore->InstanceId;
		const int32 HeadHpBefore = HeadBefore->CurrentHp;
		TestTrue(TEXT("PoisonNeedle is in hand"), CardId.IsValid());
		TestTrue(TEXT("Play PoisonNeedle without poison"),
			Session->SubmitCommand(FBattleCommand::MakePlayCard(CardId, HeadInstanceId)).IsOk());
		const TArray<FBattleEvent> Events = Session->ConsumeEvents();
		Snapshot = Session->BuildSnapshot();
		const FEnemyPartSnapshot* HeadAfter = FindPartByPartId(Snapshot, TEXT("Snake.Head"));
		if (!TestNotNull(TEXT("Snake.Head exists after PoisonNeedle"), HeadAfter))
		{
			return false;
		}
		TestEqual(TEXT("PoisonNeedle deals base damage when target has no poison"),
			HeadHpBefore - HeadAfter->CurrentHp,
			4);
		TestEqual(TEXT("PoisonNeedle emits one non-poison damage event"),
			CountEventsWithTag(Events, EBattleEventType::DamageDealt, WacomTags::Status_Poison),
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
				return FindCardInstanceInZone(Snapshot, ChitinWard->CardId, EHandZone::Both).IsValid();
			},
			*this,
			TEXT("ChitinWard shield and heal"));
		if (!Session)
		{
			return false;
		}

		Session->ConsumeEvents();
		TestTrue(TEXT("EndTurn keeps Both-zone ChitinWard while real Snake damages player"),
			Session->SubmitCommand(FBattleCommand::MakeEndTurn()).IsOk());
		Session->ConsumeEvents();
		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const int32 HpBefore = Snapshot.Player.CurrentHp;
		const int32 ShieldBefore = Snapshot.Player.Shield;
		TestTrue(TEXT("Player was damaged before ChitinWard"), HpBefore < Snapshot.Player.MaxHp);

		const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, ChitinWard->CardId);
		TestTrue(TEXT("ChitinWard remains in hand after one EndTurn"), CardId.IsValid());
		TestTrue(TEXT("Play ChitinWard"), Session->SubmitCommand(FBattleCommand::MakePlayCard(CardId)).IsOk());
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
				return FindHandCardByCardId(Snapshot, AntennaSearch->CardId) != nullptr
					&& Snapshot.PileCounts.DrawCount >= 2;
			},
			*this,
			TEXT("AntennaSearch draw and discard"),
			/*MinimumDeckSize*/ 12);
		if (!Session)
		{
			return false;
		}

		Session->ConsumeEvents();
		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const int32 DrawBefore = Snapshot.PileCounts.DrawCount;
		const int32 DiscardBefore = Snapshot.PileCounts.DiscardCount;
		const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, AntennaSearch->CardId);
		TestTrue(TEXT("AntennaSearch is in hand"), CardId.IsValid());
		TestTrue(TEXT("Play AntennaSearch"), Session->SubmitCommand(FBattleCommand::MakePlayCard(CardId)).IsOk());
		const TArray<FBattleEvent> Events = Session->ConsumeEvents();
		Snapshot = Session->BuildSnapshot();
		TestEqual(TEXT("AntennaSearch draws two cards from draw pile"), CountEvents(Events, EBattleEventType::CardsDrawn), 1);
		TestEqual(TEXT("AntennaSearch draw event count"), Events.ContainsByPredicate([](const FBattleEvent& Event)
		{
			return Event.Type == EBattleEventType::CardsDrawn && Event.Count == 2;
		}), true);
		TestTrue(TEXT("AntennaSearch emits random discard event"),
			CountEvents(Events, EBattleEventType::CardDiscarded) >= 1);
		TestEqual(TEXT("AntennaSearch net draw pile consumption"), DrawBefore - Snapshot.PileCounts.DrawCount, 2);
		TestTrue(TEXT("AntennaSearch discard pile increases by played card and random discard"),
			Snapshot.PileCounts.DiscardCount >= DiscardBefore + 2);
	}

	{
		FWacomBattleFixture Fixture;
		UBattleSession* Session = FindSessionWithHandCards(
			Fixture,
			Snake,
			{ FuxiaoFeie, MoltCut },
			[FuxiaoFeie, MoltCut](const FBattleSnapshot& Snapshot)
			{
				return FindHandCardByCardId(Snapshot, FuxiaoFeie->CardId) != nullptr
					&& FindHandCardByCardId(Snapshot, MoltCut->CardId) != nullptr
					&& FindPartByPartId(Snapshot, TEXT("Snake.Body")) != nullptr;
			},
			*this,
			TEXT("MoltCut removes slow and lowers initiative"));
		if (!Session)
		{
			return false;
		}

		Session->ConsumeEvents();
		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const FEnemyPartSnapshot* BodyBefore = FindPartByPartId(Snapshot, TEXT("Snake.Body"));
		if (!TestNotNull(TEXT("Snake.Body exists before Fuxiao/MoltCut"), BodyBefore))
		{
			return false;
		}

		FGuid FuxiaoId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, FuxiaoFeie->CardId);
		TestTrue(TEXT("FuxiaoFeie is in hand"), FuxiaoId.IsValid());
		TestTrue(TEXT("Play FuxiaoFeie to apply Slow"),
			Session->SubmitCommand(FBattleCommand::MakePlayCard(FuxiaoId, BodyBefore->InstanceId)).IsOk());
		Session->ConsumeEvents();
		Snapshot = Session->BuildSnapshot();
		const FEnemyPartSnapshot* BodyAfterSlow = FindPartByPartId(Snapshot, TEXT("Snake.Body"));
		if (!TestNotNull(TEXT("Snake.Body exists after Fuxiao"), BodyAfterSlow))
		{
			return false;
		}
		TestEqual(TEXT("FuxiaoFeie applied one Slow"),
			GetStatusStacks(BodyAfterSlow->StatusStacks, WacomTags::Status_Slow),
			1);
		const int32 InitiativeBeforeMolt = BodyAfterSlow->CurrentInitiative;

		const FGuid MoltCutId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, MoltCut->CardId);
		TestTrue(TEXT("MoltCut is in hand"), MoltCutId.IsValid());
		TestTrue(TEXT("Play MoltCut"),
			Session->SubmitCommand(FBattleCommand::MakePlayCard(MoltCutId, BodyAfterSlow->InstanceId)).IsOk());
		Snapshot = Session->BuildSnapshot();
		const FEnemyPartSnapshot* BodyAfterMolt = FindPartByPartId(Snapshot, TEXT("Snake.Body"));
		if (!TestNotNull(TEXT("Snake.Body exists after MoltCut"), BodyAfterMolt))
		{
			return false;
		}
		TestEqual(TEXT("MoltCut removes Slow"), GetStatusStacks(BodyAfterMolt->StatusStacks, WacomTags::Status_Slow), 0);
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
				return FindHandCardByCardId(Snapshot, PoisonFang->CardId) != nullptr
					&& FindHandCardByCardId(Snapshot, PoisonNeedle->CardId) != nullptr
					&& FindPartByPartId(Snapshot, TEXT("Snake.Head")) != nullptr;
			},
			*this,
			TEXT("PoisonNeedle conditional damage"));
		if (!Session)
		{
			return false;
		}

		Session->ConsumeEvents();
		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const FEnemyPartSnapshot* Head = FindPartByPartId(Snapshot, TEXT("Snake.Head"));
		if (!TestNotNull(TEXT("Snake.Head exists for conditional PoisonNeedle"), Head))
		{
			return false;
		}

		FGuid PoisonFangId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, PoisonFang->CardId);
		TestTrue(TEXT("PoisonFang is in hand"), PoisonFangId.IsValid());
		TestTrue(TEXT("Play PoisonFang"),
			Session->SubmitCommand(FBattleCommand::MakePlayCard(PoisonFangId, Head->InstanceId)).IsOk());
		Session->ConsumeEvents();
		Snapshot = Session->BuildSnapshot();
		Head = FindPartByPartId(Snapshot, TEXT("Snake.Head"));
		if (!TestNotNull(TEXT("Snake.Head exists after PoisonFang"), Head))
		{
			return false;
		}
		TestEqual(TEXT("PoisonFang leaves Poison on target"),
			GetStatusStacks(Head->StatusStacks, WacomTags::Status_Poison),
			1);
		const int32 HpBeforeNeedle = Head->CurrentHp;
		const FGuid HeadInstanceId = Head->InstanceId;

		const FGuid PoisonNeedleId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, PoisonNeedle->CardId);
		TestTrue(TEXT("PoisonNeedle is in hand after PoisonFang"), PoisonNeedleId.IsValid());
		TestTrue(TEXT("Play PoisonNeedle against poisoned target"),
			Session->SubmitCommand(FBattleCommand::MakePlayCard(PoisonNeedleId, Head->InstanceId)).IsOk());
		const TArray<FBattleEvent> Events = Session->ConsumeEvents();
		Snapshot = Session->BuildSnapshot();
		Head = FindPartByPartId(Snapshot, TEXT("Snake.Head"));
		if (!TestNotNull(TEXT("Snake.Head exists after conditional PoisonNeedle"), Head))
		{
			return false;
		}
		TestEqual(TEXT("PoisonNeedle card damage includes conditional bonus"),
			SumDamageEventsForCard(Events, PoisonNeedleId),
			9);
		TestTrue(TEXT("PoisonNeedle plus real follow-up poison ticks reduce HP by at least conditional damage plus one tick"),
			HpBeforeNeedle - Head->CurrentHp >= 10);
		TestTrue(TEXT("Conditional PoisonNeedle still includes poison tick event"),
			HasEventForActor(Events, EBattleEventType::DamageDealt, HeadInstanceId, WacomTags::Status_Poison));
	}

	{
		FWacomBattleFixture Fixture;
		UBattleSession* Session = FindSessionWithHandCards(
			Fixture,
			Snake,
			{ DiscardSelected, LightHusk },
			[DiscardSelected, LightHusk](const FBattleSnapshot& Snapshot)
			{
				return FindHandCardByCardId(Snapshot, DiscardSelected->CardId) != nullptr
					&& FindHandCardByCardId(Snapshot, LightHusk->CardId) != nullptr;
			},
			*this,
			TEXT("LightHusk OnDiscard passive"));
		if (!Session)
		{
			return false;
		}

		Session->ConsumeEvents();
		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, DiscardSelected->CardId);
		const FGuid TargetId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, LightHusk->CardId);
		TestTrue(TEXT("DiscardSelected is in hand"), SourceId.IsValid());
		TestTrue(TEXT("LightHusk is in hand"), TargetId.IsValid());
		TestTrue(TEXT("DiscardSelected discards LightHusk"),
			Session->SubmitCommand(FBattleCommand::MakePlayCardOnHandCard(SourceId, TargetId)).IsOk());
		const TArray<FBattleEvent> Events = Session->ConsumeEvents();
		Snapshot = Session->BuildSnapshot();
		TestEqual(TEXT("LightHusk OnDiscard grants shield"), Snapshot.Player.Shield, 4);
		TestEqual(TEXT("Selected discard emitted one discard event"), CountEvents(Events, EBattleEventType::CardDiscarded), 1);
	}

	{
		FWacomBattleFixture Fixture;
		UBattleSession* Session = FindSessionWithHandCards(
			Fixture,
			Snake,
			{ SilklineFeint },
			[SilklineFeint](const FBattleSnapshot& Snapshot)
			{
				return FindCardInstanceInZone(Snapshot, SilklineFeint->CardId, EHandZone::Left).IsValid()
					&& FindPartByPartId(Snapshot, TEXT("Snake.Tail")) != nullptr;
			},
			*this,
			TEXT("SilklineFeint left-zone perfect release"));
		if (!Session)
		{
			return false;
		}

		Session->ConsumeEvents();
		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const FGuid CardId = FindCardInstanceInZone(Snapshot, SilklineFeint->CardId, EHandZone::Left);
		const FEnemyPartSnapshot* TailBefore = FindPartByPartId(Snapshot, TEXT("Snake.Tail"));
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
		TestTrue(TEXT("Play left-zone SilklineFeint on Tail"),
			Session->SubmitCommand(FBattleCommand::MakePlayCard(CardId, TailInstanceId)).IsOk());
		const TArray<FBattleEvent> Events = Session->ConsumeEvents();
		Snapshot = Session->BuildSnapshot();
		const FEnemyPartSnapshot* TailAfter = FindPartByPartId(Snapshot, TEXT("Snake.Tail"));
		if (!TestNotNull(TEXT("Snake.Tail exists after SilklineFeint"), TailAfter))
		{
			return false;
		}
		TestTrue(TEXT("SilklineFeint emits InitiativeHit"), HasEventForActor(
			Events,
			EBattleEventType::InitiativeHit,
			TailInstanceId));
		TestEqual(TEXT("SilklineFeint empty perfect hook skips initiative push"),
			CountEvents(Events, EBattleEventType::InitiativePushed),
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
		/*MinimumDeckSize*/ 8);

	Session->ConsumeEvents();
	bool bSawPlayerDamage = false;
	bool bSawPlayerPoison = false;
	bool bSawPlayerSlow = false;
	bool bSawEnemyShield = false;

	for (int32 Turn = 0; Turn < 4; ++Turn)
	{
		const FBattleSnapshot Before = Session->BuildSnapshot();
		TestTrue(*FString::Printf(TEXT("EndTurn %d succeeds"), Turn + 1),
			Session->SubmitCommand(FBattleCommand::MakeEndTurn()).IsOk());
		const TArray<FBattleEvent> Events = Session->ConsumeEvents();
		const FBattleSnapshot After = Session->BuildSnapshot();

		bSawPlayerDamage = bSawPlayerDamage
			|| After.Player.CurrentHp < Before.Player.CurrentHp
			|| Events.ContainsByPredicate([](const FBattleEvent& Event)
			{
				return Event.Type == EBattleEventType::DamageDealt && !Event.ActorInstanceId.IsValid();
			});
		bSawPlayerPoison = bSawPlayerPoison
			|| GetStatusStacks(After.Player.StatusStacks, WacomTags::Status_Poison) > 0;
		bSawPlayerSlow = bSawPlayerSlow
			|| GetStatusStacks(After.Player.StatusStacks, WacomTags::Status_Slow) > 0;
		for (const FEnemyPartSnapshot& Part : After.Enemy.Parts)
		{
			if (Part.Shield > 0)
			{
				bSawEnemyShield = true;
				break;
			}
		}

		TestTrue(*FString::Printf(TEXT("EndTurn %d emits enemy action events"), Turn + 1),
			CountEvents(Events, EBattleEventType::EnemyPartActed) > 0);
		if (After.Phase == EBattlePhase::BattleEnd)
		{
			break;
		}
	}

	const FBattleSnapshot Final = Session->BuildSnapshot();
	TestTrue(TEXT("Generated Snake intent sequence deals player damage"), bSawPlayerDamage);
	TestTrue(TEXT("Generated Snake intent sequence applies Poison to player"), bSawPlayerPoison);
	TestTrue(TEXT("Generated Snake intent sequence applies Slow to player"), bSawPlayerSlow);
	TestTrue(TEXT("Generated Snake intent sequence applies self Shield"), bSawEnemyShield);
	TestTrue(TEXT("High HP fixture survives generated Snake smoke"), Final.Player.CurrentHp > 0);
	return true;
}
