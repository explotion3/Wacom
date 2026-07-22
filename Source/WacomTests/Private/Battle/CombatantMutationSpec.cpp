// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Commands/BattleCommand.h"
#include "Events/BattleEvent.h"
#include "Session/BattleResultPacket.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomEnums.h"

namespace
{
	UCardDefinition* MakeSelfShieldDamageCard(
		FWacomBattleFixture& Fixture,
		int32 ShieldAmount,
		int32 DamageAmount)
	{
		UCardDefinition* Card = Fixture.MakeNoopCard(/*Cost*/0);
		Card->TargetMode = ECardTargetMode::Self;

		FCardEffect Shield;
		Shield.EffectType = WacomTags::Status_Shield;
		Shield.Magnitude = ShieldAmount;
		Shield.Target = WacomTags::Target_Player;
		Card->Effects.Add(Shield);

		FCardEffect Damage;
		Damage.EffectType = WacomTags::Effect_Damage;
		Damage.Magnitude = DamageAmount;
		Damage.Target = WacomTags::Target_Player;
		Card->Effects.Add(Damage);
		return Card;
	}

	UCardDefinition* MakeApplyThenRemovePoisonCard(FWacomBattleFixture& Fixture, int32 Stacks)
	{
		UCardDefinition* Card = Fixture.MakeNoopCard(/*Cost*/0);
		Card->TargetMode = ECardTargetMode::Self;

		FCardEffect Apply;
		Apply.EffectType = WacomTags::Effect_ApplyStatus_Poison;
		Apply.Magnitude = Stacks;
		Apply.Target = WacomTags::Target_Player;
		Card->Effects.Add(Apply);

		FCardEffect Remove;
		Remove.EffectType = WacomTags::Effect_RemoveStatus;
		Remove.Magnitude = Stacks;
		Remove.Target = WacomTags::Target_Player;
		Remove.TargetZone = WacomTags::Status_Poison;
		Card->Effects.Add(Remove);
		return Card;
	}

	UCardDefinition* MakeShieldThenPoisonEnemyPartCard(
		FWacomBattleFixture& Fixture,
		int32 ShieldAmount,
		int32 PoisonStacks)
	{
		UCardDefinition* Card = Fixture.MakeNoopCard(/*Cost*/0);
		Card->TargetMode = ECardTargetMode::SingleEnemyPart;

		FCardEffect Shield;
		Shield.EffectType = WacomTags::Status_Shield;
		Shield.Magnitude = ShieldAmount;
		Shield.Target = WacomTags::Target_SingleEnemyPart;
		Card->Effects.Add(Shield);

		FCardEffect Poison;
		Poison.EffectType = WacomTags::Effect_ApplyStatus_Poison;
		Poison.Magnitude = PoisonStacks;
		Poison.Target = WacomTags::Target_SingleEnemyPart;
		Card->Effects.Add(Poison);
		return Card;
	}

	UBattleSession* MakeSessionWithCard(
		FWacomBattleFixture& Fixture,
		UCardDefinition* Card,
		UEnemyDefinition* Enemy)
	{
		UCardDefinition* LeftHand = Fixture.MakeNoopCard(/*Cost*/1);
		UCardDefinition* RightHand = Fixture.MakeNoopCard(/*Cost*/1);
		TArray<UCardDefinition*> Deck = { Card };
		for (int32 Index = 0; Index < 4; ++Index)
		{
			Deck.Add(Fixture.MakeNoopCard(/*Cost*/0));
		}
		return Fixture.CreateSession(Fixture.MakeCharacter(LeftHand, RightHand, Deck), Enemy, /*Seed*/1);
	}

	const FBattleEvent* FindFirstEvent(const TArray<FBattleEvent>& Events, EBattleEventType Type)
	{
		return Events.FindByPredicate([Type](const FBattleEvent& Event)
		{
			return Event.Type == Type;
		});
	}

	int32 FindFirstEventIndex(const TArray<FBattleEvent>& Events, EBattleEventType Type)
	{
		return Events.IndexOfByPredicate([Type](const FBattleEvent& Event)
		{
			return Event.Type == Type;
		});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCombatantMutationDamageFactsSpec,
	"Wacom.Battle.CombatantMutation.DamageFacts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCombatantMutationDamageFactsSpec::RunTest(const FString& /*Parameters*/)
{
	{
		FWacomBattleFixture Fixture;
		UCardDefinition* Card = MakeSelfShieldDamageCard(Fixture, /*Shield*/6, /*Damage*/10);
		UBattleSession* Session = MakeSessionWithCard(
			Fixture,
			Card,
			Fixture.MakeSinglePartEnemy(/*Hp*/100, /*Initiative*/100));

		const FBattleSnapshot Before = Session->BuildSnapshot();
		const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Before, Card->CardId);
		TestTrue(TEXT("Partial-shield card is in hand"), CardId.IsValid());
		const FBattleResolution Resolution =
			Session->ResolveCommand(FBattleCommand::MakePlayCard(CardId));
		TestTrue(TEXT("Partial-shield play succeeds"), Resolution.IsOk());

		const TArray<FBattleEvent>& Events = Resolution.Events;
		const FBattleEvent* Damage = FindFirstEvent(Events, EBattleEventType::DamageDealt);
		const FBattleSnapshot After = Session->BuildSnapshot();
		TestNotNull(TEXT("Partial-shield damage event exists"), Damage);
		TestEqual(TEXT("Partial shield leaves four actual HP damage"), After.Player.CurrentHp, 96);
		TestEqual(TEXT("Partial shield is consumed"), After.Player.Shield, 0);
		if (Damage)
		{
			TestEqual(TEXT("DamageDealt.Amount is actual HP loss"), Damage->Amount, 4);
			TestEqual(TEXT("Player damage keeps empty actor id"), Damage->ActorInstanceId, FGuid());
			TestEqual(TEXT("Player damage keeps empty enemy key"), Damage->ActorEnemyPartKey, FBattleEnemyPartKey());
			TestEqual(TEXT("Card damage keeps source card id"), Damage->CardInstanceId, CardId);
		}
	}

	{
		FWacomBattleFixture Fixture;
		UCardDefinition* Card = MakeSelfShieldDamageCard(Fixture, /*Shield*/10, /*Damage*/6);
		UBattleSession* Session = MakeSessionWithCard(
			Fixture,
			Card,
			Fixture.MakeSinglePartEnemy(/*Hp*/100, /*Initiative*/100));

		const FBattleSnapshot Before = Session->BuildSnapshot();
		const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Before, Card->CardId);
		const FBattleResolution Resolution =
			Session->ResolveCommand(FBattleCommand::MakePlayCard(CardId));
		TestTrue(TEXT("Full-shield play succeeds"), Resolution.IsOk());

		const TArray<FBattleEvent>& Events = Resolution.Events;
		const FBattleEvent* Damage = FindFirstEvent(Events, EBattleEventType::DamageDealt);
		const FBattleSnapshot After = Session->BuildSnapshot();
		TestNotNull(TEXT("Full-shield damage event still exists"), Damage);
		TestEqual(TEXT("Full shield prevents HP loss"), After.Player.CurrentHp, 100);
		TestEqual(TEXT("Unused shield remains"), After.Player.Shield, 4);
		if (Damage)
		{
			TestEqual(TEXT("Fully absorbed damage reports zero actual HP loss"), Damage->Amount, 0);
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCombatantMutationDestructionOrderSpec,
	"Wacom.Battle.CombatantMutation.DestructionOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCombatantMutationDestructionOrderSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* Card = Fixture.MakeSimpleDamageCard(/*Cost*/0, /*Damage*/10);
	UBattleSession* Session = MakeSessionWithCard(
		Fixture,
		Card,
		Fixture.MakeSinglePartEnemy(/*Hp*/3, /*Initiative*/7));

	const FBattleSnapshot Before = Session->BuildSnapshot();
	const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Before, Card->CardId);
	const FGuid PartId = FWacomBattleFixture::FindPartInstanceId(Before, 0);
	const FBattleEnemyPartKey PartKey = FWacomBattleFixture::FindPartKeyByInstanceId(Before, PartId);
	const FBattleResolution Resolution = Session->ResolveCommand(
		FWacomBattleFixture::MakePlayCardOnPartInstance(Before, CardId, PartId));
	TestTrue(TEXT("Lethal play succeeds"), Resolution.IsOk());

	const TArray<FBattleEvent>& Events = Resolution.Events;
	const int32 DamageIndex = FindFirstEventIndex(Events, EBattleEventType::DamageDealt);
	const int32 DestroyedIndex = FindFirstEventIndex(Events, EBattleEventType::EnemyPartHpEmptied);
	const int32 ChoiceIndex = FindFirstEventIndex(Events, EBattleEventType::KnockdownChoiceRequested);
	const FBattleEvent* Damage = Events.IsValidIndex(DamageIndex) ? &Events[DamageIndex] : nullptr;

	TestTrue(TEXT("Damage event precedes destroyed event"), DamageIndex != INDEX_NONE && DamageIndex < DestroyedIndex);
	TestTrue(TEXT("Destroyed event precedes knockdown request"), DestroyedIndex != INDEX_NONE && DestroyedIndex < ChoiceIndex);
	if (Damage)
	{
		TestEqual(TEXT("Overkill reports remaining HP"), Damage->Amount, 3);
		TestEqual(TEXT("Enemy damage keeps runtime id"), Damage->ActorInstanceId, PartId);
		TestEqual(TEXT("Enemy damage keeps stable key"), Damage->ActorEnemyPartKey, PartKey);
		TestEqual(TEXT("Enemy damage keeps source card"), Damage->CardInstanceId, CardId);
	}

	const FBattleSnapshot After = Session->BuildSnapshot();
	const FEnemyPartSnapshot* Part = FWacomBattleFixture::GetEnemyPartSnapshot(After, 0);
	TestNotNull(TEXT("Destroyed part remains in snapshot"), Part);
	if (Part)
	{
		TestEqual(TEXT("Destroyed part HP is zero"), Part->CurrentHp, 0);
		TestEqual(TEXT("Destroyed part initiative is zero"), Part->CurrentInitiative, 0);
		TestTrue(TEXT("Destroyed edge is visible"), Part->bDestroyed);
	}

	const FBattleResultPacket Packet = Session->BuildResultPacket();
	TestEqual(TEXT("Destruction records one experience entry"), Packet.KnockdownExpGains.Num(), 1);
	TestEqual(TEXT("Destruction records one stable identity"), Packet.DestroyedParts.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCombatantMutationStatusProjectionSpec,
	"Wacom.Battle.CombatantMutation.StatusProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCombatantMutationStatusProjectionSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* Card = MakeApplyThenRemovePoisonCard(Fixture, /*Stacks*/2);
	UBattleSession* Session = MakeSessionWithCard(
		Fixture,
		Card,
		Fixture.MakeSinglePartEnemy(/*Hp*/100, /*Initiative*/100));

	const FBattleSnapshot Before = Session->BuildSnapshot();
	const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Before, Card->CardId);
	const FBattleResolution Resolution =
		Session->ResolveCommand(FBattleCommand::MakePlayCard(CardId));
	TestTrue(TEXT("Status card play succeeds"), Resolution.IsOk());

	const TArray<FBattleEvent>& Events = Resolution.Events;
	const FBattleEvent* StatusApplied = FindFirstEvent(Events, EBattleEventType::StatusApplied);
	TestNotNull(TEXT("Status application event exists"), StatusApplied);
	if (StatusApplied)
	{
		TestEqual(TEXT("StatusApplied reports applied delta"), StatusApplied->Amount, 2);
		TestTrue(TEXT("StatusApplied keeps poison tag"), StatusApplied->Tag == WacomTags::Status_Poison);
		TestEqual(TEXT("Effect status application keeps legacy empty source card"), StatusApplied->CardInstanceId, FGuid());
	}

	const FBattleSnapshot After = Session->BuildSnapshot();
	TestFalse(TEXT("Removed status has no stack entry"), After.Player.StatusStacks.Contains(WacomTags::Status_Poison));
	TestFalse(TEXT("Removed status has no tag projection"), After.Player.Statuses.HasTagExact(WacomTags::Status_Poison));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCombatantMutationPoisonOverkillSpec,
	"Wacom.Battle.CombatantMutation.PoisonBypassesShieldAndClampsOverkill",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCombatantMutationPoisonOverkillSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* Card = MakeShieldThenPoisonEnemyPartCard(Fixture, /*Shield*/10, /*Poison*/3);
	UBattleSession* Session = MakeSessionWithCard(
		Fixture,
		Card,
		Fixture.MakeSinglePartEnemy(/*Hp*/2, /*Initiative*/100));

	const FBattleSnapshot Before = Session->BuildSnapshot();
	const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Before, Card->CardId);
	const FGuid PartId = FWacomBattleFixture::FindPartInstanceId(Before, 0);
	const FBattleResolution Resolution = Session->ResolveCommand(
		FWacomBattleFixture::MakePlayCardOnPartInstance(Before, CardId, PartId));
	TestTrue(TEXT("Shield-poison play succeeds"), Resolution.IsOk());

	const TArray<FBattleEvent>& Events = Resolution.Events;
	const FBattleEvent* PoisonDamage = Events.FindByPredicate([](const FBattleEvent& Event)
	{
		return Event.Type == EBattleEventType::DamageDealt && Event.Tag == WacomTags::Status_Poison;
	});
	TestNotNull(TEXT("Poison damage event exists"), PoisonDamage);
	if (PoisonDamage)
	{
		TestEqual(TEXT("Poison overkill reports remaining HP"), PoisonDamage->Amount, 2);
		TestEqual(TEXT("Poison damage stays unattributed to the source card"), PoisonDamage->CardInstanceId, FGuid());
	}

	const FBattleSnapshot After = Session->BuildSnapshot();
	const FEnemyPartSnapshot* Part = FWacomBattleFixture::GetEnemyPartSnapshot(After, 0);
	TestNotNull(TEXT("Poison target remains in snapshot"), Part);
	if (Part)
	{
		TestEqual(TEXT("Poison bypass leaves shield untouched"), Part->Shield, 10);
		TestEqual(TEXT("Poison clamps HP to zero"), Part->CurrentHp, 0);
		TestTrue(TEXT("Poison triggers canonical destruction edge"), Part->bDestroyed);
	}
	return true;
}
