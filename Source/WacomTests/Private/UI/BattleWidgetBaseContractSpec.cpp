// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "UI/Battle/WacomBattleWidgetBase.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleWidgetBaseSessionBlueprintSurfaceRemovedSpec,
	"Wacom.UI.Battle.WidgetBase.SessionBlueprintSurfaceRemoved",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleWidgetBaseSessionBlueprintSurfaceRemovedSpec::RunTest(const FString& /*Parameters*/)
{
	const UClass* WidgetBaseClass = UWacomBattleWidgetBase::StaticClass();
	if (!TestNotNull(TEXT("UWacomBattleWidgetBase class exists"), WidgetBaseClass))
	{
		return false;
	}

	TestTrue(TEXT("Legacy SetSession is C++ only"),
		WidgetBaseClass->FindFunctionByName(
			GET_FUNCTION_NAME_CHECKED(UWacomBattleWidgetBase, SetSession)) == nullptr);
	TestTrue(TEXT("Legacy GetSession is C++ only"),
		WidgetBaseClass->FindFunctionByName(
			GET_FUNCTION_NAME_CHECKED(UWacomBattleWidgetBase, GetSession)) == nullptr);

	TestTrue(TEXT("SetInjectedBattleSession stays C++ only"),
		WidgetBaseClass->FindFunctionByName(TEXT("SetInjectedBattleSession")) == nullptr);
	TestTrue(TEXT("GetInjectedBattleSession stays C++ only"),
		WidgetBaseClass->FindFunctionByName(TEXT("GetInjectedBattleSession")) == nullptr);

	return true;
}
