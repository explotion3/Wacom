// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "UI/Foundation/WacomExplorationHUD.h"
#include "UObject/StrongObjectPtr.h"
#include "../../../../WacomApp/Private/UI/Shop/WacomWorldShopExplorationHUDVisibilityGuard.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomWorldShopExplorationHUDVisibilityGuardSpec,
	"Wacom.UI.WorldShop.HUD.ExplorationVisibilityIsReversible",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomWorldShopExplorationHUDVisibilityGuardSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomExplorationHUD> ExplorationHUD(
		NewObject<UWacomExplorationHUD>());
	if (!TestNotNull(TEXT("Exploration HUD exists"), ExplorationHUD.Get()))
	{
		return false;
	}

	ExplorationHUD->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	{
		FWacomWorldShopExplorationHUDVisibilityGuard Guard;
		TestTrue(TEXT("Guard accepts exploration HUD"),
			Guard.Suppress(*ExplorationHUD));
		TestTrue(TEXT("Guard reports active suppression"),
			Guard.IsSuppressing());
		TestEqual(TEXT("Run HUD is collapsed while shop is active"),
			ExplorationHUD->GetVisibility(),
			ESlateVisibility::Collapsed);
		TestTrue(TEXT("Repeated suppression is idempotent"),
			Guard.Suppress(*ExplorationHUD));
		TestEqual(TEXT("Repeated suppression does not replace saved visibility"),
			ExplorationHUD->GetVisibility(),
			ESlateVisibility::Collapsed);

		Guard.Restore();
		TestFalse(TEXT("Guard reports restored state"),
			Guard.IsSuppressing());
		TestEqual(TEXT("Exact pre-shop visibility is restored"),
			ExplorationHUD->GetVisibility(),
			ESlateVisibility::SelfHitTestInvisible);

		Guard.Restore();
		TestEqual(TEXT("Repeated restore remains idempotent"),
			ExplorationHUD->GetVisibility(),
			ESlateVisibility::SelfHitTestInvisible);

		ExplorationHUD->SetVisibility(ESlateVisibility::Hidden);
		TestTrue(TEXT("Guard can suppress a second visit"),
			Guard.Suppress(*ExplorationHUD));
	}
	TestEqual(TEXT("Guard destructor restores exact visibility"),
		ExplorationHUD->GetVisibility(),
		ESlateVisibility::Hidden);
	return true;
}

#endif
