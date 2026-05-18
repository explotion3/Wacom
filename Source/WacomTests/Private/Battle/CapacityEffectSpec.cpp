// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"

#include "Commands/BattleCommand.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Tags/WacomGameplayTags.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
	UBattleSession* CreateCapacityEffectSession(
		FWacomBattleFixture& Fx,
		UCardDefinition* Card,
		const FGameplayTagContainer& CapacityEffectTags)
	{
		UCharacterDefinition* Character = Fx.MakeCharacter(
			Fx.MakeNoopCard(0),
			Fx.MakeNoopCard(0),
			{});
		UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(/*Hp*/20, /*Initiative*/10, /*IntentResist*/0);

		TStrongObjectPtr<UBattleSession> Session(NewObject<UBattleSession>());
		FBattleInitParams Params;
		Params.Character = Character;
		Params.Enemy = Enemy;
		Params.RandomSeed = 11;

		FBattleDeckEntry Entry;
		Entry.Definition = Card;
		Entry.CapacityEffectTags = CapacityEffectTags;
		Params.BattleDeckEntries.Add(Entry);

		const FWacomStatus Status = Session->Initialize(Params);
		check(Status.IsOk());
		return Session.Get();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCapacityEffectWeaponDamagePlus3Spec,
	"Wacom.Battle.CapacityEffect.WeaponDamagePlus3AppliesToWeapon",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCapacityEffectWeaponDamagePlus3Spec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Weapon = Fx.MakeDamageCardWithKeywords(
		/*Cost*/1,
		/*Damage*/4,
		{ WacomTags::Card_Keyword_Weapon });

	FGameplayTagContainer Tags;
	Tags.AddTag(WacomTags::Card_CapacityEffect_WeaponDamagePlus3);
	UBattleSession* Session = CreateCapacityEffectSession(Fx, Weapon, Tags);

	FBattleSnapshot Before = Session->BuildSnapshot();
	const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Before, Weapon->CardId);
	const FGuid TargetId = FWacomBattleFixture::FindPartInstanceId(Before, 0);
	TestTrue(TEXT("Weapon card in hand"), CardId.IsValid());
	TestTrue(TEXT("Target valid"), TargetId.IsValid());

	TestTrue(TEXT("Play weapon"), Session->SubmitCommand(FBattleCommand::MakePlayCard(CardId, TargetId)).IsOk());
	const FBattleSnapshot After = Session->BuildSnapshot();
	TestEqual(TEXT("Damage 4 + WeaponDamagePlus3 = 7"),
		FWacomBattleFixture::FindPartHp(After, 0), 13);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCapacityEffectNonWeaponNoBonusSpec,
	"Wacom.Battle.CapacityEffect.WeaponDamagePlus3IgnoresNonWeapon",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCapacityEffectNonWeaponNoBonusSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* NonWeapon = Fx.MakeSimpleDamageCard(/*Cost*/1, /*Damage*/4);

	FGameplayTagContainer Tags;
	Tags.AddTag(WacomTags::Card_CapacityEffect_WeaponDamagePlus3);
	UBattleSession* Session = CreateCapacityEffectSession(Fx, NonWeapon, Tags);

	FBattleSnapshot Before = Session->BuildSnapshot();
	const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Before, NonWeapon->CardId);
	const FGuid TargetId = FWacomBattleFixture::FindPartInstanceId(Before, 0);
	TestTrue(TEXT("Non-weapon card in hand"), CardId.IsValid());
	TestTrue(TEXT("Target valid"), TargetId.IsValid());

	TestTrue(TEXT("Play non-weapon"), Session->SubmitCommand(FBattleCommand::MakePlayCard(CardId, TargetId)).IsOk());
	const FBattleSnapshot After = Session->BuildSnapshot();
	TestEqual(TEXT("Damage remains 4 without Weapon keyword"),
		FWacomBattleFixture::FindPartHp(After, 0), 16);

	return true;
}
