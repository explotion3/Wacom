// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Commands/BattleCommand.h"
#include "Events/BattleEvent.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"

namespace
{
	UCardDefinition* MakePlayerShieldThenDamageCard(
		FWacomBattleFixture& Fixture,
		const int32 ShieldAmount,
		const int32 DamageAmount)
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

	UCardDefinition* MakeEnemyShieldThenPoisonCard(
		FWacomBattleFixture& Fixture,
		const int32 ShieldAmount,
		const int32 PoisonStacks)
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

	UCardDefinition* MakeEnemyDamageCard(
		FWacomBattleFixture& Fixture,
		const int32 DamageAmount)
	{
		UCardDefinition* Card = Fixture.MakeNoopCard(/*Cost*/0);
		Card->TargetMode = ECardTargetMode::SingleEnemyPart;

		FCardEffect Damage;
		Damage.EffectType = WacomTags::Effect_Damage;
		Damage.Magnitude = DamageAmount;
		Damage.Target = WacomTags::Target_SingleEnemyPart;
		Card->Effects.Add(Damage);
		return Card;
	}

	UBattleSession* MakeSession(
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
		return Fixture.CreateSession(
			Fixture.MakeCharacter(LeftHand, RightHand, Deck),
			Enemy,
			/*Seed*/1);
	}

	const FBattleEvent* FindEvent(
		const TArray<FBattleEvent>& Events,
		const EBattleEventType Type,
		const FGameplayTag Tag = FGameplayTag())
	{
		return Events.FindByPredicate([Type, Tag](const FBattleEvent& Event)
		{
			return Event.Type == Type && (!Tag.IsValid() || Event.Tag == Tag);
		});
	}

	int32 CountEvents(const TArray<FBattleEvent>& Events, const EBattleEventType Type)
	{
		int32 Count = 0;
		for (const FBattleEvent& Event : Events)
		{
			Count += Event.Type == Type ? 1 : 0;
		}
		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCombatPresentationDamageShieldFactsSpec,
	"Wacom.Battle.CombatPresentationFacts.DamageAndShield",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCombatPresentationDamageShieldFactsSpec::RunTest(
	const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* Card =
		MakePlayerShieldThenDamageCard(Fixture, /*Shield*/6, /*Damage*/10);
	UBattleSession* Session = MakeSession(
		Fixture,
		Card,
		Fixture.MakeSinglePartEnemy(/*Hp*/100, /*Initiative*/100));

	const FBattleSnapshot Before = Session->BuildSnapshot();
	const FGuid CardId =
		FWacomBattleFixture::FindHandInstanceByCardId(Before, Card->CardId);
	const FBattleResolution Resolution =
		Session->ResolveCommand(FBattleCommand::MakePlayCard(CardId));
	TestTrue(TEXT("Shield then damage command succeeds"), Resolution.IsOk());

	const FBattleEvent* Shield =
		FindEvent(Resolution.Events, EBattleEventType::ShieldChanged);
	const FBattleEvent* Damage =
		FindEvent(Resolution.Events, EBattleEventType::DamageDealt);
	TestNotNull(TEXT("Shield gain emits a public fact"), Shield);
	TestNotNull(TEXT("Damage emits a public fact"), Damage);
	TestEqual(
		TEXT("Damage absorption does not emit a second shield event"),
		CountEvents(Resolution.Events, EBattleEventType::ShieldChanged),
		1);

	if (Shield)
	{
		TestEqual(TEXT("Shield delta is actual gain"), Shield->Amount, 6);
		TestEqual(TEXT("Shield Count is resulting value"), Shield->Count, 6);
		TestEqual(TEXT("Shield source card is retained"), Shield->CardInstanceId, CardId);
	}
	if (Damage)
	{
		TestEqual(TEXT("Legacy Amount remains actual HP loss"), Damage->Amount, 4);
		TestEqual(TEXT("Requested damage is retained"), Damage->DamageResolution.RequestedDamage, 10);
		TestEqual(TEXT("Shield before is retained"), Damage->DamageResolution.ShieldBefore, 6);
		TestEqual(TEXT("Absorbed shield is retained"), Damage->DamageResolution.ShieldAbsorbed, 6);
		TestEqual(TEXT("Shield after is retained"), Damage->DamageResolution.ShieldAfter, 0);
		TestEqual(TEXT("Direct damage has no overkill"), Damage->DamageResolution.Overkill, 0);
		TestEqual(
			TEXT("Card damage is direct"),
			Damage->DamageResolution.Kind,
			EBattleDamageKind::Direct);
		TestFalse(TEXT("Current rules do not invent critical hits"), Damage->DamageResolution.bCritical);
		TestTrue(TEXT("Direct cause keeps Damage tag"), Damage->Tag == WacomTags::Effect_Damage);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCombatPresentationPeriodicFactsSpec,
	"Wacom.Battle.CombatPresentationFacts.PeriodicDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCombatPresentationPeriodicFactsSpec::RunTest(
	const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* Card =
		MakeEnemyShieldThenPoisonCard(Fixture, /*Shield*/10, /*Poison*/3);
	UBattleSession* Session = MakeSession(
		Fixture,
		Card,
		Fixture.MakeSinglePartEnemy(/*Hp*/2, /*Initiative*/100));

	const FBattleSnapshot Before = Session->BuildSnapshot();
	const FGuid CardId =
		FWacomBattleFixture::FindHandInstanceByCardId(Before, Card->CardId);
	const FGuid PartId = FWacomBattleFixture::FindPartInstanceId(Before, 0);
	const FBattleEnemyPartKey PartKey =
		FWacomBattleFixture::FindPartKeyByInstanceId(Before, PartId);
	const FBattleResolution Resolution = Session->ResolveCommand(
		FWacomBattleFixture::MakePlayCardOnPartInstance(Before, CardId, PartId));
	TestTrue(TEXT("Poison command succeeds"), Resolution.IsOk());

	const FBattleEvent* Shield =
		FindEvent(Resolution.Events, EBattleEventType::ShieldChanged);
	const FBattleEvent* Poison = FindEvent(
		Resolution.Events,
		EBattleEventType::DamageDealt,
		WacomTags::Status_Poison);
	TestNotNull(TEXT("Enemy shield gain fact exists"), Shield);
	TestNotNull(TEXT("Poison damage fact exists"), Poison);

	if (Shield)
	{
		TestEqual(TEXT("Enemy shield identifies runtime part"), Shield->ActorInstanceId, PartId);
		TestEqual(TEXT("Enemy shield identifies stable part"), Shield->ActorEnemyPartKey, PartKey);
		TestEqual(TEXT("Enemy shield actual gain"), Shield->Amount, 10);
		TestEqual(TEXT("Enemy shield resulting value"), Shield->Count, 10);
	}
	if (Poison)
	{
		TestEqual(TEXT("Poison requested damage is stack-scaled"), Poison->DamageResolution.RequestedDamage, 24);
		TestEqual(TEXT("Poison bypasses shield"), Poison->DamageResolution.ShieldAbsorbed, 0);
		TestEqual(TEXT("Poison leaves shield unchanged"), Poison->DamageResolution.ShieldAfter, 10);
		TestEqual(TEXT("Poison Amount clamps to remaining HP"), Poison->Amount, 2);
		TestEqual(TEXT("Poison overkill is explicit"), Poison->DamageResolution.Overkill, 22);
		TestEqual(
			TEXT("Poison is periodic"),
			Poison->DamageResolution.Kind,
			EBattleDamageKind::Periodic);
		TestTrue(TEXT("Poison status cause is retained"), Poison->Tag == WacomTags::Status_Poison);
		TestEqual(TEXT("Poison tick is not attributed to source card"), Poison->CardInstanceId, FGuid());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCombatPresentationDamageBoundaryFactsSpec,
	"Wacom.Battle.CombatPresentationFacts.DamageBoundaries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCombatPresentationDamageBoundaryFactsSpec::RunTest(
	const FString& /*Parameters*/)
{
	{
		FWacomBattleFixture Fixture;
		UCardDefinition* Card =
			MakePlayerShieldThenDamageCard(Fixture, /*Shield*/10, /*Damage*/7);
		UBattleSession* Session = MakeSession(
			Fixture,
			Card,
			Fixture.MakeSinglePartEnemy(/*Hp*/100, /*Initiative*/100));
		const FBattleSnapshot Before = Session->BuildSnapshot();
		const FGuid CardId =
			FWacomBattleFixture::FindHandInstanceByCardId(Before, Card->CardId);
		const FBattleResolution Resolution =
			Session->ResolveCommand(FBattleCommand::MakePlayCard(CardId));
		TestTrue(TEXT("Fully absorbed command succeeds"), Resolution.IsOk());

		const FBattleEvent* Damage =
			FindEvent(Resolution.Events, EBattleEventType::DamageDealt);
		TestNotNull(TEXT("Fully absorbed damage still emits its fact"), Damage);
		if (Damage)
		{
			TestEqual(TEXT("Fully absorbed legacy HP loss is zero"), Damage->Amount, 0);
			TestEqual(TEXT("Fully absorbed requested damage"), Damage->DamageResolution.RequestedDamage, 7);
			TestEqual(TEXT("Fully absorbed shield before"), Damage->DamageResolution.ShieldBefore, 10);
			TestEqual(TEXT("Fully absorbed shield consumption"), Damage->DamageResolution.ShieldAbsorbed, 7);
			TestEqual(TEXT("Fully absorbed shield after"), Damage->DamageResolution.ShieldAfter, 3);
			TestEqual(TEXT("Fully absorbed damage has no overkill"), Damage->DamageResolution.Overkill, 0);
		}
		TestEqual(
			TEXT("Shield absorption does not duplicate ShieldChanged"),
			CountEvents(Resolution.Events, EBattleEventType::ShieldChanged),
			1);
	}

	{
		FWacomBattleFixture Fixture;
		UCardDefinition* Card = MakeEnemyDamageCard(Fixture, /*Damage*/7);
		UBattleSession* Session = MakeSession(
			Fixture,
			Card,
			Fixture.MakeSinglePartEnemy(/*Hp*/3, /*Initiative*/100));
		const FBattleSnapshot Before = Session->BuildSnapshot();
		const FGuid CardId =
			FWacomBattleFixture::FindHandInstanceByCardId(Before, Card->CardId);
		const FGuid PartId = FWacomBattleFixture::FindPartInstanceId(Before, 0);
		const FBattleResolution Resolution = Session->ResolveCommand(
			FWacomBattleFixture::MakePlayCardOnPartInstance(Before, CardId, PartId));
		TestTrue(TEXT("Direct overkill command succeeds"), Resolution.IsOk());

		const FBattleEvent* Damage =
			FindEvent(Resolution.Events, EBattleEventType::DamageDealt);
		TestNotNull(TEXT("Direct overkill damage fact exists"), Damage);
		if (Damage)
		{
			TestEqual(TEXT("Direct overkill clamps legacy HP loss"), Damage->Amount, 3);
			TestEqual(TEXT("Direct overkill requested damage"), Damage->DamageResolution.RequestedDamage, 7);
			TestEqual(TEXT("Direct overkill is explicit"), Damage->DamageResolution.Overkill, 4);
			TestEqual(
				TEXT("Direct overkill retains direct kind"),
				Damage->DamageResolution.Kind,
				EBattleDamageKind::Direct);
		}
	}
	return true;
}
