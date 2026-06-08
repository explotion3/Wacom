// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/BattleTriggerActor.h"
#include "Actors/WacomBattleEnemyActor.h"
#include "Characters/CharacterDefinition.h"
#include "Encounters/EncounterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"

#include "Misc/DataValidation.h"
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

	AWacomBattleEnemyActor* MakeEncounterTriggerSceneEnemyHost(
		UObject* Outer,
		UEnemyDefinition* EnemyDefinition,
		FName EnemySlotId)
	{
		AWacomBattleEnemyActor* Host = NewObject<AWacomBattleEnemyActor>(
			Outer ? Outer : GetTransientPackage(),
			NAME_None,
			RF_Transient);
		Host->EnemyDefinition = EnemyDefinition;
		Host->EnemySlotId = EnemySlotId;
		return Host;
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

	UEncounterDefinition* MakeEncounterWithSlots(
		UObject* Outer,
		const TArray<TPair<FName, UEnemyDefinition*>>& Slots)
	{
		UEncounterDefinition* Encounter = NewObject<UEncounterDefinition>(Outer);
		Encounter->EncounterDefinitionId = TEXT("Encounter.TriggerRuntime.Slots");
		Encounter->DisplayName = FText::FromString(TEXT("Trigger Runtime Slots Encounter"));

		Encounter->EnemySlots.Reserve(Slots.Num());
		for (const TPair<FName, UEnemyDefinition*>& SlotPair : Slots)
		{
			FEncounterEnemySlot Slot;
			Slot.EnemySlotId = SlotPair.Key;
			Slot.EnemyDefinition = SlotPair.Value;
			Encounter->EnemySlots.Add(Slot);
		}
		return Encounter;
	}

	UEncounterDefinition* MakeEncounterWithInvalidRuntimeSlots(
		UObject* Outer,
		UEnemyDefinition* ValidEnemy,
		UEnemyDefinition* MissingSlotIdEnemy)
	{
		UEncounterDefinition* Encounter = NewObject<UEncounterDefinition>(Outer);
		Encounter->EncounterDefinitionId = TEXT("Encounter.TriggerRuntime.InvalidSlots");
		Encounter->DisplayName = FText::FromString(TEXT("Trigger Runtime Invalid Slots Encounter"));

		FEncounterEnemySlot MissingSlotIdSlot;
		MissingSlotIdSlot.EnemySlotId = NAME_None;
		MissingSlotIdSlot.EnemyDefinition = MissingSlotIdEnemy;

		FEncounterEnemySlot MissingEnemySlot;
		MissingEnemySlot.EnemySlotId = TEXT("MissingEnemy");
		MissingEnemySlot.EnemyDefinition = nullptr;

		FEncounterEnemySlot ValidSlot;
		ValidSlot.EnemySlotId = TEXT("ValidEnemy");
		ValidSlot.EnemyDefinition = ValidEnemy;

		Encounter->EnemySlots = { MissingSlotIdSlot, MissingEnemySlot, ValidSlot };
		return Encounter;
	}

	EDataValidationResult ValidateObjectForEncounterTriggerTest(
		const UObject* Object,
		TArray<FText>& OutWarnings,
		TArray<FText>& OutErrors)
	{
		OutWarnings.Reset();
		OutErrors.Reset();
		if (!Object)
		{
			OutErrors.Add(FText::FromString(TEXT("Missing object")));
			return EDataValidationResult::Invalid;
		}

		FDataValidationContext Context;
		const EDataValidationResult Result = Object->IsDataValid(Context);
		Context.SplitIssues(OutWarnings, OutErrors);
		return Result;
	}

	bool ValidationIssuesContain(const TArray<FText>& Issues, const TCHAR* ExpectedText)
	{
		for (const FText& Issue : Issues)
		{
			if (Issue.ToString().Contains(ExpectedText))
			{
				return true;
			}
		}
		return false;
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

	UEnemyDefinition* LeftEnemy = MakeEncounterTriggerEnemy(
		Trigger.Get(),
		TEXT("Enemy.Left"),
		TEXT("Part.Shared"));
	UEnemyDefinition* RightEnemy = MakeEncounterTriggerEnemy(
		Trigger.Get(),
		TEXT("Enemy.Right"),
		TEXT("Part.Shared"));

	Trigger->EncounterDefinition = MakeTwoSlotEncounter(Trigger.Get(), LeftEnemy, RightEnemy);

	TArray<FBattleEnemySlotInit> EnemySlots;
	Trigger->BuildBattleEnemySlots(EnemySlots);
	TestEqual(TEXT("Encounter slots are exported"), EnemySlots.Num(), 2);
	TestEqual(TEXT("First slot id"), EnemySlots[0].EnemySlotId, FName(TEXT("LeftEnemy")));
	TestTrue(TEXT("First slot enemy"), EnemySlots[0].Enemy.Get() == LeftEnemy);
	TestEqual(TEXT("Second slot id"), EnemySlots[1].EnemySlotId, FName(TEXT("RightEnemy")));
	TestTrue(TEXT("Second slot enemy"), EnemySlots[1].Enemy.Get() == RightEnemy);

	FBattleInitParams Params;
	Params.Character = MakeEncounterTriggerCharacter(Trigger.Get());
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
		TestTrue(TEXT("First enemy slot definition is used for battle slot"),
			Snapshot.Enemies[0].Definition.Get() == LeftEnemy);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomAppEncounterTriggerSkipsInvalidBattleEnemySlotsSpec,
	"Wacom.App.BattleTrigger.EncounterDefinition.SkipsInvalidBattleEnemySlots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomAppEncounterTriggerSkipsInvalidBattleEnemySlotsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<ABattleTriggerActor> Trigger(NewObject<ABattleTriggerActor>());
	UEnemyDefinition* ValidEnemy = MakeEncounterTriggerEnemy(
		Trigger.Get(),
		TEXT("Enemy.Valid"),
		TEXT("Part.Valid"));
	UEnemyDefinition* MissingSlotIdEnemy = MakeEncounterTriggerEnemy(
		Trigger.Get(),
		TEXT("Enemy.MissingSlotId"),
		TEXT("Part.MissingSlotId"));

	Trigger->EncounterDefinition = MakeEncounterWithInvalidRuntimeSlots(
		Trigger.Get(),
		ValidEnemy,
		MissingSlotIdEnemy);

	TArray<FBattleEnemySlotInit> EnemySlots;
	Trigger->BuildBattleEnemySlots(EnemySlots);
	TestEqual(TEXT("Only complete Encounter slots are exported"), EnemySlots.Num(), 1);
	if (EnemySlots.Num() == 1)
	{
		TestEqual(TEXT("Valid slot id is preserved"), EnemySlots[0].EnemySlotId, FName(TEXT("ValidEnemy")));
		TestTrue(TEXT("Valid slot enemy is preserved"), EnemySlots[0].Enemy.Get() == ValidEnemy);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomAppEncounterTriggerRequiresEncounterDefinitionSpec,
	"Wacom.App.BattleTrigger.EncounterDefinition.RequiresEncounterDefinition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomAppEncounterTriggerRequiresEncounterDefinitionSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<ABattleTriggerActor> Trigger(NewObject<ABattleTriggerActor>());
	Trigger->PersistentId = TEXT("Trigger.EncounterRuntime.MissingEncounter");

	TArray<FBattleEnemySlotInit> EnemySlots;
	Trigger->BuildBattleEnemySlots(EnemySlots);
	TestEqual(TEXT("No battle slots exported without Encounter"), EnemySlots.Num(), 0);

	TArray<FText> Warnings;
	TArray<FText> Errors;
	const EDataValidationResult Result =
		ValidateObjectForEncounterTriggerTest(Trigger.Get(), Warnings, Errors);
	TestEqual(TEXT("Missing Encounter invalidates placement"), Result, EDataValidationResult::Invalid);
	TestTrue(TEXT("Missing Encounter error mentions EncounterDefinition"),
		ValidationIssuesContain(Errors, TEXT("EncounterDefinition")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomAppEncounterTriggerPlacementValidationRequiresPersistentIdSpec,
	"Wacom.App.BattleTrigger.EncounterDefinition.PlacementValidationRequiresPersistentId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomAppEncounterTriggerPlacementValidationRequiresPersistentIdSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<ABattleTriggerActor> Trigger(NewObject<ABattleTriggerActor>());
	UEnemyDefinition* Enemy = MakeEncounterTriggerEnemy(
		Trigger.Get(),
		TEXT("Enemy.MissingPersistentId"),
		TEXT("Part.MissingPersistentId"));
	Trigger->EncounterDefinition = MakeEncounterWithSlots(
		Trigger.Get(),
		{ TPair<FName, UEnemyDefinition*>(TEXT("Enemy"), Enemy) });
	FWacomBattleSceneEnemyHostSlot HostSlot;
	HostSlot.EnemySlotId = TEXT("Enemy");
	HostSlot.SceneEnemyHost = MakeEncounterTriggerSceneEnemyHost(Trigger.Get(), Enemy, TEXT("Enemy"));
	Trigger->SceneEnemyHostSlots = { HostSlot };

	TArray<FText> Warnings;
	TArray<FText> Errors;
	const EDataValidationResult Result =
		ValidateObjectForEncounterTriggerTest(Trigger.Get(), Warnings, Errors);
	TestEqual(TEXT("Missing PersistentId invalidates placement"), Result, EDataValidationResult::Invalid);
	TestTrue(TEXT("Missing PersistentId error mentions PersistentId"),
		ValidationIssuesContain(Errors, TEXT("PersistentId")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomAppEncounterTriggerPlacementValidationAcceptsCompleteHostMappingSpec,
	"Wacom.App.BattleTrigger.EncounterDefinition.PlacementValidationAcceptsCompleteHostMapping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomAppEncounterTriggerPlacementValidationAcceptsCompleteHostMappingSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<ABattleTriggerActor> Trigger(NewObject<ABattleTriggerActor>());
	Trigger->PersistentId = TEXT("Trigger.EncounterRuntime.ValidHostMapping");
	UEnemyDefinition* Enemy = MakeEncounterTriggerEnemy(
		Trigger.Get(),
		TEXT("Enemy.ValidHostMapping"),
		TEXT("Part.ValidHostMapping"));
	Trigger->EncounterDefinition = MakeEncounterWithSlots(
		Trigger.Get(),
		{ TPair<FName, UEnemyDefinition*>(TEXT("Enemy"), Enemy) });

	FWacomBattleSceneEnemyHostSlot HostSlot;
	HostSlot.EnemySlotId = TEXT("Enemy");
	HostSlot.SceneEnemyHost = MakeEncounterTriggerSceneEnemyHost(Trigger.Get(), Enemy, TEXT("Enemy"));
	Trigger->SceneEnemyHostSlots = { HostSlot };

	TArray<FText> Warnings;
	TArray<FText> Errors;
	const EDataValidationResult Result =
		ValidateObjectForEncounterTriggerTest(Trigger.Get(), Warnings, Errors);
	TestEqual(TEXT("Complete host mapping keeps placement valid"), Result, EDataValidationResult::Valid);
	TestEqual(TEXT("Complete host mapping has no errors"), Errors.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomAppEncounterTriggerPlacementValidationRejectsSceneHostSlotContractBreaksSpec,
	"Wacom.App.BattleTrigger.EncounterDefinition.PlacementValidationRejectsSceneHostSlotContractBreaks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomAppEncounterTriggerPlacementValidationRejectsSceneHostSlotContractBreaksSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<ABattleTriggerActor> Trigger(NewObject<ABattleTriggerActor>());
	Trigger->PersistentId = TEXT("Trigger.EncounterRuntime.Validation");
	UEnemyDefinition* Enemy = MakeEncounterTriggerEnemy(
		Trigger.Get(),
		TEXT("Enemy.Validation"),
		TEXT("Part.Validation"));
	UEnemyDefinition* Support = MakeEncounterTriggerEnemy(
		Trigger.Get(),
		TEXT("Enemy.Support"),
		TEXT("Part.Support"));
	Trigger->EncounterDefinition = MakeEncounterWithSlots(
		Trigger.Get(),
		{
			TPair<FName, UEnemyDefinition*>(TEXT("Enemy"), Enemy),
			TPair<FName, UEnemyDefinition*>(TEXT("Support"), Support),
		});

	AWacomBattleEnemyActor* EnemyHost =
		MakeEncounterTriggerSceneEnemyHost(Trigger.Get(), Enemy, TEXT("Enemy"));
	AWacomBattleEnemyActor* SupportHost =
		MakeEncounterTriggerSceneEnemyHost(Trigger.Get(), Support, TEXT("Support"));
	AWacomBattleEnemyActor* ExtraHost =
		MakeEncounterTriggerSceneEnemyHost(Trigger.Get(), Enemy, TEXT("Extra"));

	{
		FWacomBattleSceneEnemyHostSlot EmptySlot;
		EmptySlot.EnemySlotId = NAME_None;
		EmptySlot.SceneEnemyHost = EnemyHost;
		Trigger->SceneEnemyHostSlots = { EmptySlot };

		TArray<AWacomBattleEnemyActor*> SceneHosts;
		Trigger->BuildBattleSceneEnemyHosts(SceneHosts);
		TestEqual(TEXT("Empty EnemySlotId is not exported as a scene host"),
			SceneHosts.Num(),
			0);

		TArray<FText> Warnings;
		TArray<FText> Errors;
		const EDataValidationResult Result =
			ValidateObjectForEncounterTriggerTest(Trigger.Get(), Warnings, Errors);
		TestEqual(TEXT("Empty EnemySlotId invalidates trigger"), Result, EDataValidationResult::Invalid);
		TestTrue(TEXT("Empty EnemySlotId error mentions EnemySlotId"),
			ValidationIssuesContain(Errors, TEXT("EnemySlotId")));
	}

	{
		FWacomBattleSceneEnemyHostSlot MissingHostSlot;
		MissingHostSlot.EnemySlotId = TEXT("Enemy");
		FWacomBattleSceneEnemyHostSlot SupportSlot;
		SupportSlot.EnemySlotId = TEXT("Support");
		SupportSlot.SceneEnemyHost = SupportHost;
		Trigger->SceneEnemyHostSlots = { MissingHostSlot, SupportSlot };

		TArray<FText> Warnings;
		TArray<FText> Errors;
		const EDataValidationResult Result =
			ValidateObjectForEncounterTriggerTest(Trigger.Get(), Warnings, Errors);
		TestEqual(TEXT("Missing host invalidates trigger"), Result, EDataValidationResult::Invalid);
		TestTrue(TEXT("Missing host error mentions SceneEnemyHost"),
			ValidationIssuesContain(Errors, TEXT("SceneEnemyHost")));
	}

	{
		FWacomBattleSceneEnemyHostSlot EnemySlot;
		EnemySlot.EnemySlotId = TEXT("Enemy");
		EnemySlot.SceneEnemyHost = EnemyHost;
		FWacomBattleSceneEnemyHostSlot SupportSlot;
		SupportSlot.EnemySlotId = TEXT("Support");
		SupportSlot.SceneEnemyHost = EnemyHost;
		Trigger->SceneEnemyHostSlots = { EnemySlot, SupportSlot };

		TArray<FText> Warnings;
		TArray<FText> Errors;
		const EDataValidationResult Result =
			ValidateObjectForEncounterTriggerTest(Trigger.Get(), Warnings, Errors);
		TestEqual(TEXT("Duplicate host invalidates trigger"), Result, EDataValidationResult::Invalid);
		TestTrue(TEXT("Duplicate host error mentions duplicate reference"),
			ValidationIssuesContain(Errors, TEXT("重复引用 Host")));
	}

	{
		FWacomBattleSceneEnemyHostSlot EnemySlot;
		EnemySlot.EnemySlotId = TEXT("Enemy");
		EnemySlot.SceneEnemyHost = EnemyHost;
		FWacomBattleSceneEnemyHostSlot SupportSlot;
		SupportSlot.EnemySlotId = TEXT("Support");
		SupportSlot.SceneEnemyHost = SupportHost;
		FWacomBattleSceneEnemyHostSlot ExtraSlot;
		ExtraSlot.EnemySlotId = TEXT("Extra");
		ExtraSlot.SceneEnemyHost = ExtraHost;
		Trigger->SceneEnemyHostSlots = { EnemySlot, SupportSlot, ExtraSlot };

		TArray<FText> Warnings;
		TArray<FText> Errors;
		const EDataValidationResult Result =
			ValidateObjectForEncounterTriggerTest(Trigger.Get(), Warnings, Errors);
		TestEqual(TEXT("Unknown host slot invalidates trigger"), Result, EDataValidationResult::Invalid);
		TestTrue(TEXT("Unknown host slot error mentions Extra"),
			ValidationIssuesContain(Errors, TEXT("Extra")));
	}

	{
		FWacomBattleSceneEnemyHostSlot EnemySlot;
		EnemySlot.EnemySlotId = TEXT("Enemy");
		EnemySlot.SceneEnemyHost = EnemyHost;
		Trigger->SceneEnemyHostSlots = { EnemySlot };

		TArray<FText> Warnings;
		TArray<FText> Errors;
		const EDataValidationResult Result =
			ValidateObjectForEncounterTriggerTest(Trigger.Get(), Warnings, Errors);
		TestEqual(TEXT("Missing Encounter slot mapping invalidates trigger"),
			Result,
			EDataValidationResult::Invalid);
		TestTrue(TEXT("Missing Encounter slot mapping error mentions Support"),
			ValidationIssuesContain(Errors, TEXT("Support")));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomAppEncounterTriggerDebugSummaryReportsEncounterAndHostSlotDiffsSpec,
	"Wacom.App.BattleTrigger.EncounterDefinition.DebugSummaryReportsEncounterAndHostSlotDiffs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomAppEncounterTriggerDebugSummaryReportsEncounterAndHostSlotDiffsSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<ABattleTriggerActor> Trigger(NewObject<ABattleTriggerActor>());
	Trigger->PersistentId = TEXT("Trigger.EncounterRuntime.Debug");
	UEnemyDefinition* Enemy = MakeEncounterTriggerEnemy(
		Trigger.Get(),
		TEXT("Enemy.Debug"),
		TEXT("Part.Debug"));
	UEnemyDefinition* Support = MakeEncounterTriggerEnemy(
		Trigger.Get(),
		TEXT("Enemy.Debug.Support"),
		TEXT("Part.Debug.Support"));
	Trigger->EncounterDefinition = MakeEncounterWithSlots(
		Trigger.Get(),
		{
			TPair<FName, UEnemyDefinition*>(TEXT("Enemy"), Enemy),
			TPair<FName, UEnemyDefinition*>(TEXT("Support"), Support),
		});

	FWacomBattleSceneEnemyHostSlot EnemySlot;
	EnemySlot.EnemySlotId = TEXT("Enemy");
	EnemySlot.SceneEnemyHost = MakeEncounterTriggerSceneEnemyHost(
		Trigger.Get(),
		Enemy,
		TEXT("Enemy"));
	FWacomBattleSceneEnemyHostSlot ExtraSlot;
	ExtraSlot.EnemySlotId = TEXT("Extra");
	ExtraSlot.SceneEnemyHost = MakeEncounterTriggerSceneEnemyHost(
		Trigger.Get(),
		Enemy,
		TEXT("Extra"));
	Trigger->SceneEnemyHostSlots = { EnemySlot, ExtraSlot };

	const FWacomBattleTriggerDebugView View = Trigger->GetBattleTriggerDebugView(nullptr);
	TestEqual(TEXT("Debug view reports Encounter slot count"), View.EncounterEnemySlotCount, 2);
	TestTrue(TEXT("Debug view reports Enemy host slot"),
		View.SceneEnemyHostSlotIds.Contains(TEXT("Enemy")));
	TestTrue(TEXT("Debug view reports Extra host slot"),
		View.SceneEnemyHostSlotIds.Contains(TEXT("Extra")));
	TestEqual(TEXT("Debug view reports one missing slot"), View.MissingSceneEnemyHostSlotIds.Num(), 1);
	TestEqual(TEXT("Debug view reports missing Support slot"),
		View.MissingSceneEnemyHostSlotIds[0],
		FName(TEXT("Support")));
	TestEqual(TEXT("Debug view reports one extra slot"), View.ExtraSceneEnemyHostSlotIds.Num(), 1);
	TestEqual(TEXT("Debug view reports extra slot id"),
		View.ExtraSceneEnemyHostSlotIds[0],
		FName(TEXT("Extra")));

	const FString Summary = Trigger->GetBattleTriggerDebugSummary(nullptr);
	TestTrue(TEXT("Summary reports Encounter slot count"),
		Summary.Contains(TEXT("EncounterSlots=2")));
	TestTrue(TEXT("Summary reports scene host slots"),
		Summary.Contains(TEXT("SceneEnemyHostSlotIds=[Enemy,Extra]")));
	TestTrue(TEXT("Summary reports missing slot"),
		Summary.Contains(TEXT("MissingSceneEnemyHostSlotIds=[Support]")));
	TestTrue(TEXT("Summary reports extra slot"),
		Summary.Contains(TEXT("ExtraSceneEnemyHostSlotIds=[Extra]")));
	TestFalse(TEXT("Summary does not use old FirstEncounterEnemy wording"),
		Summary.Contains(TEXT("FirstEncounterEnemy")));
	TestFalse(TEXT("Summary does not use Primary enemy wording"),
		Summary.Contains(TEXT("Primary")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomAppEncounterTriggerSyncSceneEnemyHostSlotsPreservesHostsAndRetainsManualExtrasSpec,
	"Wacom.App.BattleTrigger.EncounterDefinition.SyncSceneEnemyHostSlotsPreservesHostsAndRetainsManualExtras",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomAppEncounterTriggerSyncSceneEnemyHostSlotsPreservesHostsAndRetainsManualExtrasSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<ABattleTriggerActor> Trigger(NewObject<ABattleTriggerActor>());
	Trigger->PersistentId = TEXT("Trigger.EncounterRuntime.Sync");
	UEnemyDefinition* Enemy = MakeEncounterTriggerEnemy(
		Trigger.Get(),
		TEXT("Enemy.Sync"),
		TEXT("Part.Sync"));
	UEnemyDefinition* Support = MakeEncounterTriggerEnemy(
		Trigger.Get(),
		TEXT("Enemy.Sync.Support"),
		TEXT("Part.Sync.Support"));
	Trigger->EncounterDefinition = MakeEncounterWithSlots(
		Trigger.Get(),
		{
			TPair<FName, UEnemyDefinition*>(TEXT("Enemy"), Enemy),
			TPair<FName, UEnemyDefinition*>(TEXT("Support"), Support),
		});

	AWacomBattleEnemyActor* SupportHost =
		MakeEncounterTriggerSceneEnemyHost(Trigger.Get(), Support, TEXT("Support"));
	AWacomBattleEnemyActor* ExtraHost =
		MakeEncounterTriggerSceneEnemyHost(Trigger.Get(), Enemy, TEXT("Extra"));

	FWacomBattleSceneEnemyHostSlot ExistingSupportSlot;
	ExistingSupportSlot.EnemySlotId = TEXT("Support");
	ExistingSupportSlot.SceneEnemyHost = SupportHost;
	FWacomBattleSceneEnemyHostSlot ExtraSlot;
	ExtraSlot.EnemySlotId = TEXT("Extra");
	ExtraSlot.SceneEnemyHost = ExtraHost;
	Trigger->SceneEnemyHostSlots = { ExistingSupportSlot, ExtraSlot };

	Trigger->SyncSceneEnemyHostSlotsFromEncounter();

	TestEqual(TEXT("Sync keeps Encounter slots plus manual extra"),
		Trigger->SceneEnemyHostSlots.Num(),
		3);
	TestEqual(TEXT("Sync inserts missing Encounter slot in Encounter order"),
		Trigger->SceneEnemyHostSlots[0].EnemySlotId,
		FName(TEXT("Enemy")));
	TestNull(TEXT("Sync leaves newly inserted slot host empty"),
		Trigger->SceneEnemyHostSlots[0].SceneEnemyHost.Get());
	TestEqual(TEXT("Sync preserves existing Encounter slot id"),
		Trigger->SceneEnemyHostSlots[1].EnemySlotId,
		FName(TEXT("Support")));
	TestEqual(TEXT("Sync preserves existing Host reference"),
		Trigger->SceneEnemyHostSlots[1].SceneEnemyHost.Get(),
		SupportHost);
	TestEqual(TEXT("Sync retains manual extra slot for cleanup"),
		Trigger->SceneEnemyHostSlots[2].EnemySlotId,
		FName(TEXT("Extra")));
	TestEqual(TEXT("Sync retains manual extra Host"),
		Trigger->SceneEnemyHostSlots[2].SceneEnemyHost.Get(),
		ExtraHost);

	const FWacomBattleTriggerDebugView View = Trigger->GetBattleTriggerDebugView(nullptr);
	TestEqual(TEXT("Debug view reports no missing slots after sync"), View.MissingSceneEnemyHostSlotIds.Num(), 0);
	TestEqual(TEXT("Debug view reports retained extra slot after sync"), View.ExtraSceneEnemyHostSlotIds.Num(), 1);
	TestEqual(TEXT("Debug view retained extra slot id"),
		View.ExtraSceneEnemyHostSlotIds[0],
		FName(TEXT("Extra")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomAppEncounterTriggerSceneHostSlotsBindUnitKeysByEncounterOrderSpec,
	"Wacom.App.BattleTrigger.EncounterDefinition.SceneHostSlotsBindUnitKeysByEncounterOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomAppEncounterTriggerSceneHostSlotsBindUnitKeysByEncounterOrderSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<ABattleTriggerActor> Trigger(NewObject<ABattleTriggerActor>());
	Trigger->PersistentId = TEXT("Trigger.EncounterRuntime.HostOrder");
	UEnemyDefinition* Enemy = MakeEncounterTriggerEnemy(
		Trigger.Get(),
		TEXT("Enemy.HostOrder"),
		TEXT("Part.HostOrder"));
	UEnemyDefinition* Support = MakeEncounterTriggerEnemy(
		Trigger.Get(),
		TEXT("Enemy.HostOrder.Support"),
		TEXT("Part.HostOrder.Support"));
	Trigger->EncounterDefinition = MakeEncounterWithSlots(
		Trigger.Get(),
		{
			TPair<FName, UEnemyDefinition*>(TEXT("Enemy"), Enemy),
			TPair<FName, UEnemyDefinition*>(TEXT("Support"), Support),
		});

	AWacomBattleEnemyActor* EnemyHost =
		MakeEncounterTriggerSceneEnemyHost(Trigger.Get(), Enemy, TEXT("WrongBeforeTrigger"));
	AWacomBattleEnemyActor* SupportHost =
		MakeEncounterTriggerSceneEnemyHost(Trigger.Get(), Support, TEXT("AlsoWrongBeforeTrigger"));

	FWacomBattleSceneEnemyHostSlot SupportSlot;
	SupportSlot.EnemySlotId = TEXT("Support");
	SupportSlot.SceneEnemyHost = SupportHost;
	FWacomBattleSceneEnemyHostSlot EnemySlot;
	EnemySlot.EnemySlotId = TEXT("Enemy");
	EnemySlot.SceneEnemyHost = EnemyHost;
	Trigger->SceneEnemyHostSlots = { SupportSlot, EnemySlot };

	TArray<AWacomBattleEnemyActor*> SceneHosts;
	Trigger->BuildBattleSceneEnemyHosts(SceneHosts);

	TestEqual(TEXT("Scene host export follows Encounter slot order"), SceneHosts.Num(), 2);
	if (SceneHosts.Num() == 2)
	{
		TestTrue(TEXT("Encounter first host is exported first"), SceneHosts[0] == EnemyHost);
		TestTrue(TEXT("Encounter second host is exported second"), SceneHosts[1] == SupportHost);
	}
	TestEqual(TEXT("Trigger slot overwrites host-authored EnemySlotId for first unit"),
		EnemyHost->GetEffectiveEnemySlotId(),
		FName(TEXT("Enemy")));
	TestEqual(TEXT("Trigger slot overwrites host-authored EnemySlotId for second unit"),
		SupportHost->GetEffectiveEnemySlotId(),
		FName(TEXT("Support")));

	const FWacomBattleTriggerDebugView View = Trigger->GetBattleTriggerDebugView(nullptr);
	TestEqual(TEXT("Debug view reports both exported hosts"), View.SceneEnemyHostCount, 2);
	TestEqual(TEXT("Debug view has no missing scene host slots"), View.MissingSceneEnemyHostSlotIds.Num(), 0);
	TestEqual(TEXT("Debug view has no extra scene host slots"), View.ExtraSceneEnemyHostSlotIds.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomAppEncounterTriggerDoesNotGuessMissingHostEnemySlotIdSpec,
	"Wacom.App.BattleTrigger.EncounterDefinition.DoesNotGuessMissingHostEnemySlotId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomAppEncounterTriggerDoesNotGuessMissingHostEnemySlotIdSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<ABattleTriggerActor> Trigger(NewObject<ABattleTriggerActor>());
	Trigger->PersistentId = TEXT("Trigger.EncounterRuntime.MissingHostSlot");
	UEnemyDefinition* Enemy = MakeEncounterTriggerEnemy(
		Trigger.Get(),
		TEXT("Enemy.MissingHostSlot"),
		TEXT("Part.MissingHostSlot"));
	Trigger->EncounterDefinition = MakeEncounterWithSlots(
		Trigger.Get(),
		{ TPair<FName, UEnemyDefinition*>(TEXT("Enemy"), Enemy) });

	AWacomBattleEnemyActor* Host =
		MakeEncounterTriggerSceneEnemyHost(Trigger.Get(), Enemy, TEXT("Enemy"));
	FWacomBattleSceneEnemyHostSlot Slot;
	Slot.EnemySlotId = NAME_None;
	Slot.SceneEnemyHost = Host;
	Trigger->SceneEnemyHostSlots = { Slot };

	TArray<AWacomBattleEnemyActor*> SceneHosts;
	Trigger->BuildBattleSceneEnemyHosts(SceneHosts);

	TestEqual(TEXT("Slot without EnemySlotId is not exported"), SceneHosts.Num(), 0);
	TestEqual(TEXT("Host-authored EnemySlotId is not used as a trigger-slot fallback"),
		Host->GetEffectiveEnemySlotId(),
		FName(TEXT("Enemy")));
	const FWacomBattleTriggerDebugView View = Trigger->GetBattleTriggerDebugView(nullptr);
	TestEqual(TEXT("Missing Encounter slot is reported"), View.MissingSceneEnemyHostSlotIds.Num(), 1);
	TestTrue(TEXT("Missing host slot is reported"), View.MissingSceneEnemyHostSlotIds.Contains(TEXT("Enemy")));
	return true;
}
