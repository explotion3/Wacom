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
	TStrongObjectPtr<UBattleSession> CreateCapacityEffectSession(
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
		return Session;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCapacityEffectTagsValidSpec,
	"Wacom.Battle.CapacityEffect.TagsValid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCapacityEffectTagsValidSpec::RunTest(const FString& /*Parameters*/)
{
	const FGameplayTag CapacityTag = WacomTags::Card_CapacityEffect_WeaponDamagePlus3;
	const FGameplayTag WeaponTag = WacomTags::Card_Keyword_Weapon;
	TestTrue(TEXT("WeaponDamagePlus3 tag valid"),
		CapacityTag.IsValid());
	TestTrue(TEXT("Weapon keyword tag valid"),
		WeaponTag.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCapacityEffectBugGirlCocoonAssetSpec,
	"Wacom.Battle.CapacityEffect.BugGirlCocoonAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCapacityEffectBugGirlCocoonAssetSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, EXAMPLE R4.2:
	// 蛛茧绒囊落盘资产必须使用首个具体容量效果，而不是早期 Placeholder。
	UCardDefinition* Cocoon = LoadObject<UCardDefinition>(
		nullptr,
		TEXT("/Game/Wacom/Cards/BugGirl/DA_Card_ZhujianRongnang.DA_Card_ZhujianRongnang"));

	if (!TestNotNull(TEXT("BugGirl cocoon card asset loads"), Cocoon))
	{
		return false;
	}

	const FGameplayTag ExpectedCapacityEffect = WacomTags::Card_CapacityEffect_WeaponDamagePlus3;
	TestEqual(TEXT("ZhujianRongnang CapacityEffect == WeaponDamagePlus3"),
		Cocoon->Physique.CapacityEffect,
		ExpectedCapacityEffect);
	TestEqual(TEXT("ZhujianRongnang SpecialZone capacity source = 3"),
		Cocoon->Physique.Capacity,
		3);

	return true;
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
	TStrongObjectPtr<UBattleSession> Session = CreateCapacityEffectSession(Fx, Weapon, Tags);

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
	TStrongObjectPtr<UBattleSession> Session = CreateCapacityEffectSession(Fx, NonWeapon, Tags);

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCapacityEffectWeaponWithoutTagNoBonusSpec,
	"Wacom.Battle.CapacityEffect.WeaponDamagePlus3RequiresTag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCapacityEffectWeaponWithoutTagNoBonusSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Weapon = Fx.MakeDamageCardWithKeywords(
		/*Cost*/1,
		/*Damage*/4,
		{ WacomTags::Card_Keyword_Weapon });

	FGameplayTagContainer EmptyTags;
	TStrongObjectPtr<UBattleSession> Session = CreateCapacityEffectSession(Fx, Weapon, EmptyTags);

	FBattleSnapshot Before = Session->BuildSnapshot();
	const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Before, Weapon->CardId);
	const FGuid TargetId = FWacomBattleFixture::FindPartInstanceId(Before, 0);
	TestTrue(TEXT("Weapon card in hand"), CardId.IsValid());
	TestTrue(TEXT("Target valid"), TargetId.IsValid());

	TestTrue(TEXT("Play weapon without capacity tag"), Session->SubmitCommand(FBattleCommand::MakePlayCard(CardId, TargetId)).IsOk());
	const FBattleSnapshot After = Session->BuildSnapshot();
	TestEqual(TEXT("Damage remains 4 without WeaponDamagePlus3 tag"),
		FWacomBattleFixture::FindPartHp(After, 0), 16);

	return true;
}
