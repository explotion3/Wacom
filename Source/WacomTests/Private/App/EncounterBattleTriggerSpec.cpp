// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/BattleTriggerActor.h"
#include "Characters/CharacterDefinition.h"
#include "Encounters/EncounterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
	UEnemyDefinition* MakeEncounterTriggerEnemy(UObject* Outer, FName EnemyId, FName PartId)
	{
		UEnemyPartDefinition* Part = NewObject<UEnemyPartDefinition>(Outer);
		Part->PartId = PartId;
		Part->DisplayName = FText::FromName(PartId);
		Part->MaxHp = 10;

		UEnemyDefinition* Enemy = NewObject<UEnemyDefinition>(Outer);
		Enemy->EnemyId = EnemyId;
		Enemy->DisplayName = FText::FromName(EnemyId);

		FEnemyPartSlot PartSlot;
		PartSlot.PartSlotId = TEXT("Core");
		PartSlot.PartDef = Part;
		Enemy->Parts = { PartSlot };
		return Enemy;
	}

	UCharacterDefinition* MakeEncounterTriggerCharacter(UObject* Outer)
	{
		UCharacterDefinition* Character = NewObject<UCharacterDefinition>(Outer);
		Character->CharacterId = TEXT("Character.EncounterTrigger");
		Character->DisplayName = FText::FromString(TEXT("Encounter Trigger Character"));
		Character->FingerCount = 10;
		Character->HpPerFinger = 2;
		return Character;
	}

	UEncounterDefinition* MakeTwoSlotEncounter(
		UObject* Outer,
		UEnemyDefinition* LeftEnemy,
		UEnemyDefinition* RightEnemy)
	{
		UEncounterDefinition* Encounter = NewObject<UEncounterDefinition>(Outer);
		Encounter->EncounterDefinitionId = TEXT("Encounter.TriggerRuntime");
		Encounter->DisplayName = FText::FromString(TEXT("Trigger Runtime Encounter"));

		FEncounterEnemySlot LeftSlot;
		LeftSlot.EnemySlotId = TEXT("LeftEnemy");
		LeftSlot.EnemyDefinition = LeftEnemy;

		FEncounterEnemySlot RightSlot;
		RightSlot.EnemySlotId = TEXT("RightEnemy");
		RightSlot.EnemyDefinition = RightEnemy;

		Encounter->EnemySlots = { LeftSlot, RightSlot };
		return Encounter;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomAppEncounterTriggerBuildsBattleEnemySlotsSpec,
	"Wacom.App.BattleTrigger.EncounterDefinition.BuildsBattleEnemySlots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomAppEncounterTriggerBuildsBattleEnemySlotsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<ABattleTriggerActor> Trigger(NewObject<ABattleTriggerActor>());
	Trigger->PersistentId = TEXT("Trigger.EncounterRuntime");

	UEnemyDefinition* LegacyEnemy = MakeEncounterTriggerEnemy(
		Trigger.Get(),
		TEXT("Enemy.Legacy"),
		TEXT("Part.Legacy"));
	UEnemyDefinition* LeftEnemy = MakeEncounterTriggerEnemy(
		Trigger.Get(),
		TEXT("Enemy.Left"),
		TEXT("Part.Shared"));
	UEnemyDefinition* RightEnemy = MakeEncounterTriggerEnemy(
		Trigger.Get(),
		TEXT("Enemy.Right"),
		TEXT("Part.Shared"));

	Trigger->EnemyDef = LegacyEnemy;
	Trigger->EncounterDefinition = MakeTwoSlotEncounter(Trigger.Get(), LeftEnemy, RightEnemy);

	TArray<FBattleEnemySlotInit> EnemySlots;
	Trigger->BuildBattleEnemySlots(EnemySlots);
	TestEqual(TEXT("Encounter slots are exported"), EnemySlots.Num(), 2);
	TestEqual(TEXT("First slot id"), EnemySlots[0].EnemySlotId, FName(TEXT("LeftEnemy")));
	TestTrue(TEXT("First slot enemy"), EnemySlots[0].Enemy.Get() == LeftEnemy);
	TestEqual(TEXT("Second slot id"), EnemySlots[1].EnemySlotId, FName(TEXT("RightEnemy")));
	TestTrue(TEXT("Second slot enemy"), EnemySlots[1].Enemy.Get() == RightEnemy);
	TestTrue(TEXT("Resolved battle enemy uses first Encounter slot"),
		Trigger->ResolveBattleEnemyDefinition() == LeftEnemy);

	FBattleInitParams Params;
	Params.Character = MakeEncounterTriggerCharacter(Trigger.Get());
	Params.Enemy = Trigger->ResolveBattleEnemyDefinition();
	Params.EncounterId = Trigger->PersistentId;
	Params.EnemySlots = EnemySlots;

	TStrongObjectPtr<UBattleSession> Session(NewObject<UBattleSession>());
	const FWacomStatus InitStatus = Session->Initialize(Params);
	TestTrue(TEXT("Battle initializes from Encounter slots"), InitStatus.IsOk());
	if (!InitStatus.IsOk())
	{
		return false;
	}

	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	TestEqual(TEXT("Snapshot encounter id uses trigger persistent id"),
		Snapshot.EncounterId,
		FName(TEXT("Trigger.EncounterRuntime")));
	TestEqual(TEXT("Snapshot has two enemies"), Snapshot.Enemies.Num(), 2);
	if (Snapshot.Enemies.Num() == 2)
	{
		TestEqual(TEXT("Left slot preserved"), Snapshot.Enemies[0].EnemySlotId, FName(TEXT("LeftEnemy")));
		TestEqual(TEXT("Right slot preserved"), Snapshot.Enemies[1].EnemySlotId, FName(TEXT("RightEnemy")));
		TestTrue(TEXT("Legacy enemy not used for Encounter slot"),
			Snapshot.Enemies[0].Definition.Get() == LeftEnemy);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomAppEncounterTriggerFallsBackToLegacyEnemySpec,
	"Wacom.App.BattleTrigger.EncounterDefinition.FallsBackToLegacyEnemy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomAppEncounterTriggerFallsBackToLegacyEnemySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<ABattleTriggerActor> Trigger(NewObject<ABattleTriggerActor>());
	UEnemyDefinition* LegacyEnemy = MakeEncounterTriggerEnemy(
		Trigger.Get(),
		TEXT("Enemy.LegacyFallback"),
		TEXT("Part.LegacyFallback"));
	Trigger->EnemyDef = LegacyEnemy;

	TArray<FBattleEnemySlotInit> EnemySlots;
	Trigger->BuildBattleEnemySlots(EnemySlots);
	TestEqual(TEXT("No Encounter slots exported"), EnemySlots.Num(), 0);
	TestTrue(TEXT("Resolved battle enemy falls back to legacy EnemyDef"),
		Trigger->ResolveBattleEnemyDefinition() == LegacyEnemy);
	return true;
}
