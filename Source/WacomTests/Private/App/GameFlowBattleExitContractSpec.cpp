// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "GameFramework/WacomGameMode.h"
#include "GameFramework/WacomPlayerController.h"
#include "Types/WacomEnums.h"

#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomAppGameFlowBattleExitRequestUsesTypedOutcomeSpec,
	"Wacom.App.GameFlow.BattleExit.RequestExitBattleUsesTypedOutcome",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomAppGameFlowBattleExitRequestUsesTypedOutcomeSpec::RunTest(const FString& /*Parameters*/)
{
	UFunction* Function = AWacomPlayerController::StaticClass()->FindFunctionByName(
		GET_FUNCTION_NAME_CHECKED(AWacomPlayerController, RequestExitBattle));
	if (!TestNotNull(TEXT("RequestExitBattle function exists"), Function))
	{
		return false;
	}

	const FProperty* OutcomeProperty = nullptr;
	for (TFieldIterator<FProperty> It(Function); It; ++It)
	{
		const FProperty* Property = *It;
		if (Property
			&& Property->HasAnyPropertyFlags(CPF_Parm)
			&& Property->GetFName() == TEXT("Outcome"))
		{
			OutcomeProperty = Property;
			break;
		}
	}

	if (!TestNotNull(TEXT("RequestExitBattle has Outcome parameter"), OutcomeProperty))
	{
		return false;
	}

	const FEnumProperty* EnumProperty = CastField<FEnumProperty>(OutcomeProperty);
	if (!TestNotNull(TEXT("Outcome is reflected as an enum property"), EnumProperty))
	{
		return false;
	}

	TestEqual(TEXT("Outcome uses EBattleOutcome"),
		EnumProperty->GetEnum(),
		StaticEnum<EBattleOutcome>());
	TestFalse(TEXT("Outcome is not the legacy raw byte property"),
		OutcomeProperty->IsA<FByteProperty>());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomAppGameFlowDefaultRandomSeedDeprecatedSpec,
	"Wacom.App.GameFlow.LegacyDefaults.DefaultRandomSeedDeprecated",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomAppGameFlowDefaultRandomSeedDeprecatedSpec::RunTest(const FString& /*Parameters*/)
{
	FProperty* Property = AWacomGameMode::StaticClass()->FindPropertyByName(
		GET_MEMBER_NAME_CHECKED(AWacomGameMode, DefaultRandomSeed));
	if (!TestNotNull(TEXT("DefaultRandomSeed property exists for asset compatibility"), Property))
	{
		return false;
	}

	TestTrue(TEXT("DefaultRandomSeed is marked deprecated"),
		Property->HasMetaData(TEXT("DeprecatedProperty")));
	TestTrue(TEXT("Deprecation message points to RunSession / RunState"),
		Property->GetMetaData(TEXT("DeprecationMessage")).Contains(TEXT("RunSession")));

	return true;
}
