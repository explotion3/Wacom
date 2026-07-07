// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "UI/Battle/WacomBattleWidgetBase.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleWidgetBaseSessionBlueprintSurfaceDeprecatedSpec,
	"Wacom.UI.Battle.WidgetBase.SessionBlueprintSurfaceDeprecated",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleWidgetBaseSessionBlueprintSurfaceDeprecatedSpec::RunTest(const FString& /*Parameters*/)
{
	const UClass* WidgetBaseClass = UWacomBattleWidgetBase::StaticClass();
	if (!TestNotNull(TEXT("UWacomBattleWidgetBase class exists"), WidgetBaseClass))
	{
		return false;
	}

	const UFunction* SetSessionFunction = WidgetBaseClass->FindFunctionByName(
		GET_FUNCTION_NAME_CHECKED(UWacomBattleWidgetBase, SetSession));
	const UFunction* GetSessionFunction = WidgetBaseClass->FindFunctionByName(
		GET_FUNCTION_NAME_CHECKED(UWacomBattleWidgetBase, GetSession));

	if (!TestNotNull(TEXT("SetSession function exists"), SetSessionFunction)
		|| !TestNotNull(TEXT("GetSession function exists"), GetSessionFunction))
	{
		return false;
	}

	TestTrue(TEXT("SetSession Blueprint surface is deprecated"),
		SetSessionFunction->HasMetaData(TEXT("DeprecatedFunction")));
	TestTrue(TEXT("SetSession deprecation points WBP to Snapshot / ViewData"),
		SetSessionFunction->GetMetaData(TEXT("DeprecationMessage")).Contains(TEXT("Snapshot")));

	TestTrue(TEXT("GetSession Blueprint surface is deprecated"),
		GetSessionFunction->HasMetaData(TEXT("DeprecatedFunction")));
	TestTrue(TEXT("GetSession deprecation points WBP to BattleHUD commands"),
		GetSessionFunction->GetMetaData(TEXT("DeprecationMessage")).Contains(TEXT("BattleHUD")));

	return true;
}
