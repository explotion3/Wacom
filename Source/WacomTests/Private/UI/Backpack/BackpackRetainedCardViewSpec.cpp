// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "UI/Card/WacomCardView.h"
#include "UI/Card/WacomRetainedCardViewWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackStaticRetainerDisablesSurfaceFoilSpec,
	"Wacom.UI.Backpack.CardView.StaticRetainerDisablesSurfaceFoil",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackStaticRetainerDisablesSurfaceFoilSpec::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UWacomRetainedCardViewWidget> RetainedCardView(
		NewObject<UWacomRetainedCardViewWidget>());
	const TSharedRef<SWidget> SlateWidget = RetainedCardView->TakeWidget();
	UWacomCardView* InnerCardView = RetainedCardView->GetInnerCardView();
	TestNotNull(TEXT("Static retained card creates its inner CardView"), InnerCardView);
	TestFalse(TEXT("Static retained card disables animated surface foil by default"),
		RetainedCardView->IsSurfaceFoilEnabled());
	if (!InnerCardView)
	{
		return false;
	}

	const FWacomCardViewAutomationTestView View = InnerCardView->GetAutomationTestViewForTest();
	TestFalse(TEXT("Backpack wrapper applies its disabled foil policy to the inner card"),
		View.bSurfaceFoilEnabled);
	TestTrue(TEXT("Fallback card still owns the optional foil widget"), View.bHasSurfaceFoilOverlay);
	TestFalse(TEXT("Disabled backpack foil is not visible"), View.bSurfaceFoilVisible);
	TestFalse(TEXT("Disabled backpack foil releases its material brush"),
		View.bSurfaceFoilBrushConfigured);
	return true;
}

#endif
