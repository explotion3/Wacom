// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomWorldShopHostActor.h"
#include "HAL/IConsoleManager.h"
#include "RunState.h"
#include "UI/Shop/WacomWorldShopRoutePolicy.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomWorldShopRouteAndFallbackSpec,
	"Wacom.UI.WorldShop.RouteAndFallback.PurchaseOnlyMatrix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomWorldShopRouteAndFallbackSpec::RunTest(const FString& Parameters)
{
	FRunShopVisitRequest Request;
	Request.ShopId = TEXT("WorldShop.Route.Test");
	Request.Offers.SetNum(1);
	AWacomWorldShopHostActor* Host = NewObject<AWacomWorldShopHostActor>();
	const UWorld* HostWorld = Host->GetWorld();

	TestEqual(TEXT("null host falls back"),
		FWacomWorldShopRoutePolicy::Evaluate(Request, nullptr, HostWorld).Reason,
		FName(TEXT("MissingHost")));
	TestTrue(TEXT("purchase-only host is eligible"),
		FWacomWorldShopRoutePolicy::Evaluate(Request, Host, HostWorld).bUseWorldRoute);

	Request.CardUpgradeService.bEnabled = true;
	TestEqual(TEXT("upgrade keeps screen route"),
		FWacomWorldShopRoutePolicy::Evaluate(Request, Host, HostWorld).Reason,
		FName(TEXT("UpgradeRequiresScreen")));
	Request.CardUpgradeService.bEnabled = false;

	Request.Offers.SetNum(9);
	TestEqual(TEXT("capacity failure falls back"),
		FWacomWorldShopRoutePolicy::Evaluate(Request, Host, HostWorld).Reason,
		FName(TEXT("InsufficientAnchorCapacity")));
	Request.Offers.SetNum(1);
	Host->CardWorldScale = 0.0f;
	TestEqual(TEXT("invalid host falls back"),
		FWacomWorldShopRoutePolicy::Evaluate(Request, Host, HostWorld).Reason,
		FName(TEXT("InvalidWidgetProfile")));
	TestNotNull(
		TEXT("PIE purchase diagnostic command is registered"),
		IConsoleManager::Get().FindConsoleObject(
			TEXT("Wacom.WorldShop.DumpPIEValidation")));
	return true;
}
