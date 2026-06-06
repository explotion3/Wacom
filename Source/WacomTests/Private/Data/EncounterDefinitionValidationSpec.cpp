// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Encounters/EncounterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Validation/EncounterDefinitionValidation.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
	UEnemyDefinition* MakeEncounterValidationEnemy(UObject* Outer, FName EnemyId = TEXT("Enemy.Validation"))
	{
		UEnemyDefinition* Enemy = NewObject<UEnemyDefinition>(Outer);
		Enemy->EnemyId = EnemyId;
		Enemy->DisplayName = FText::FromName(EnemyId);

		UEnemyPartDefinition* Part = NewObject<UEnemyPartDefinition>(Enemy);
		Part->PartId = *(EnemyId.ToString() + TEXT(".Part"));
		Part->DisplayName = FText::FromName(Part->PartId);
		Part->MaxHp = 10;

		FEnemyPartSlot PartSlot;
		PartSlot.PartSlotId = Part->PartId;
		PartSlot.PartDef = Part;
		Enemy->Parts = { PartSlot };
		return Enemy;
	}

	UEncounterDefinition* MakeValidEncounterForValidation(UObject* Outer)
	{
		UEncounterDefinition* Encounter = NewObject<UEncounterDefinition>(Outer);
		Encounter->EncounterDefinitionId = TEXT("Encounter.Validation");
		Encounter->DisplayName = FText::FromString(TEXT("校验战斗入口"));

		FEncounterEnemySlot Slot;
		Slot.EnemySlotId = TEXT("Enemy");
		Slot.EnemyDefinition = MakeEncounterValidationEnemy(Encounter);
		Encounter->EnemySlots = { Slot };
		return Encounter;
	}

	bool ValidateEncounterForTest(const UEncounterDefinition* Encounter, TArray<FText>& OutErrors)
	{
		return FWacomEncounterDefinitionValidation::Validate(Encounter, OutErrors);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataEncounterDefinitionValidationValidSpec,
	"Wacom.Data.Validation.EncounterDefinition.ValidSingleEnemy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataEncounterDefinitionValidationValidSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UEncounterDefinition> Encounter(MakeValidEncounterForValidation(GetTransientPackage()));
	TArray<FText> Errors;
	TestTrue(TEXT("Valid EncounterDefinition passes validation"), ValidateEncounterForTest(Encounter.Get(), Errors));
	TestEqual(TEXT("No validation errors"), Errors.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataEncounterDefinitionValidationRequiredFieldsSpec,
	"Wacom.Data.Validation.EncounterDefinition.RequiredFields",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataEncounterDefinitionValidationRequiredFieldsSpec::RunTest(const FString& /*Parameters*/)
{
	TArray<FText> Errors;

	{
		TStrongObjectPtr<UEncounterDefinition> Encounter(MakeValidEncounterForValidation(GetTransientPackage()));
		Encounter->EncounterDefinitionId = NAME_None;
		TestFalse(TEXT("Missing EncounterDefinitionId fails"),
			ValidateEncounterForTest(Encounter.Get(), Errors));
		TestTrue(TEXT("Missing EncounterDefinitionId has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UEncounterDefinition> Encounter(MakeValidEncounterForValidation(GetTransientPackage()));
		Encounter->EnemySlots.Reset();
		TestFalse(TEXT("Empty EnemySlots fails"), ValidateEncounterForTest(Encounter.Get(), Errors));
		TestTrue(TEXT("Empty EnemySlots has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UEncounterDefinition> Encounter(MakeValidEncounterForValidation(GetTransientPackage()));
		Encounter->EnemySlots[0].EnemySlotId = NAME_None;
		TestFalse(TEXT("Missing EnemySlotId fails"), ValidateEncounterForTest(Encounter.Get(), Errors));
		TestTrue(TEXT("Missing EnemySlotId has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UEncounterDefinition> Encounter(MakeValidEncounterForValidation(GetTransientPackage()));
		Encounter->EnemySlots[0].EnemyDefinition = nullptr;
		TestFalse(TEXT("Missing EnemyDefinition fails"), ValidateEncounterForTest(Encounter.Get(), Errors));
		TestTrue(TEXT("Missing EnemyDefinition has error"), Errors.Num() > 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataEncounterDefinitionValidationDuplicateSlotSpec,
	"Wacom.Data.Validation.EncounterDefinition.DuplicateEnemySlotIdFails",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataEncounterDefinitionValidationDuplicateSlotSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UEncounterDefinition> Encounter(MakeValidEncounterForValidation(GetTransientPackage()));

	FEncounterEnemySlot SecondSlot;
	SecondSlot.EnemySlotId = TEXT("Enemy");
	SecondSlot.EnemyDefinition = MakeEncounterValidationEnemy(Encounter.Get(), TEXT("Enemy.Validation.Second"));
	Encounter->EnemySlots.Add(SecondSlot);

	TArray<FText> Errors;
	TestFalse(TEXT("Duplicate EnemySlotId fails"), ValidateEncounterForTest(Encounter.Get(), Errors));
	TestTrue(TEXT("Duplicate EnemySlotId has error"), Errors.Num() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataEncounterDefinitionValidationSharedEnemyDefinitionSpec,
	"Wacom.Data.Validation.EncounterDefinition.SharedEnemyDefinitionAllowed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataEncounterDefinitionValidationSharedEnemyDefinitionSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UEncounterDefinition> Encounter(MakeValidEncounterForValidation(GetTransientPackage()));

	UEnemyDefinition* SharedEnemy = Encounter->EnemySlots[0].EnemyDefinition.Get();
	Encounter->EnemySlots[0].EnemySlotId = TEXT("Left");

	FEncounterEnemySlot SecondSlot;
	SecondSlot.EnemySlotId = TEXT("Right");
	SecondSlot.EnemyDefinition = SharedEnemy;
	Encounter->EnemySlots.Add(SecondSlot);

	TArray<FText> Errors;
	TestTrue(TEXT("Different slots can share one EnemyDefinition"),
		ValidateEncounterForTest(Encounter.Get(), Errors));
	TestEqual(TEXT("No validation errors"), Errors.Num(), 0);
	return true;
}
