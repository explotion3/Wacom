// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Map/WacomRunMapNodeWidget.h"
#include "UI/Map/WacomRunMapScreen.h"
#include "UI/RunMapScreenTestAccess.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunMapAssetContractSpec,
	"Wacom.UI.RunMap.AssetContract.RegisteredBlueprintsAndFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunMapAssetContractSpec::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Run Map Core widget tag is registered"),
		WacomTags::UI_Widget_RunMapScreen.GetTag().IsValid());
	TestEqual(TEXT("Run Map Core widget tag has stable identity"),
		WacomTags::UI_Widget_RunMapScreen.GetTag().ToString(),
		FString(TEXT("UI.Widget.RunMapScreen")));

	UWacomRunMapScreen* Fallback = NewObject<UWacomRunMapScreen>();
	FWacomRunMapScreenTestAccess::BuildAndConstruct(*Fallback);
	TestTrue(TEXT("C++ fallback provides every required binding"),
		FWacomRunMapScreenTestAccess::HasRequiredBindings(*Fallback));
	TestEqual(TEXT("C++ fallback uses native node class"),
		FWacomRunMapScreenTestAccess::GetNodeWidgetClass(*Fallback),
		UWacomRunMapNodeWidget::StaticClass());
	FWacomRunMapScreenTestAccess::Destruct(*Fallback);

	UWidgetBlueprintGeneratedClass* NodeClass = LoadObject<UWidgetBlueprintGeneratedClass>(
		nullptr,
		TEXT("/Game/Wacom/UI/Map/WBP_RunMapNode.WBP_RunMapNode_C"));
	UWidgetBlueprintGeneratedClass* ScreenClass = LoadObject<UWidgetBlueprintGeneratedClass>(
		nullptr,
		TEXT("/Game/Wacom/UI/Map/WBP_RunMapScreen.WBP_RunMapScreen_C"));
	if (!TestNotNull(TEXT("WBP_RunMapNode generated class"), NodeClass)
		|| !TestNotNull(TEXT("WBP_RunMapScreen generated class"), ScreenClass))
	{
		return false;
	}
	TestTrue(TEXT("Node WBP has the required parent"),
		NodeClass->IsChildOf(UWacomRunMapNodeWidget::StaticClass()));
	TestTrue(TEXT("Screen WBP has the required parent"),
		ScreenClass->IsChildOf(UWacomRunMapScreen::StaticClass()));

	UWacomRunMapScreen* Authored = NewObject<UWacomRunMapScreen>(
		GetTransientPackage(), ScreenClass);
	FWacomRunMapScreenTestAccess::BuildAndConstruct(*Authored);
	TestTrue(TEXT("Authored Screen resolves every optional binding"),
		FWacomRunMapScreenTestAccess::HasRequiredBindings(*Authored));
	TestEqual(TEXT("Authored Screen creates the authored node WBP"),
		FWacomRunMapScreenTestAccess::GetNodeWidgetClass(*Authored),
		static_cast<UClass*>(NodeClass));
	FWacomRunMapScreenTestAccess::Destruct(*Authored);
	return true;
}

#endif
