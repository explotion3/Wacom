// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "HAL/IConsoleManager.h"
#include "../../../../WacomApp/Private/UI/Shop/WacomShopUpgradePIEValidationPolicy.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomShopUpgradePIEValidationPolicySpec,
	"Wacom.UI.Shop.UpgradePIEValidation.CommandAndEligibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomShopUpgradePIEValidationPolicySpec::RunTest(const FString& Parameters)
{
	TestNotNull(
		TEXT("PIE command is registered"),
		IConsoleManager::Get().FindConsoleObject(TEXT("Wacom.Shop.SeedUpgradePIEValidation")));

	FWacomShopUpgradePIEValidationFacts Facts;
	Facts.bIsPIEWorld = true;
	Facts.bRunActive = true;
	Facts.JourneyId = TEXT("Journey.Debug");
	Facts.CurrentNode = { TEXT("Floor.Debug.01"), TEXT("Node.Entry") };
	Facts.ActiveActivityKind = ERunExplorationActivityKind::None;
	FName Reason;
	TestTrue(TEXT("Exact Entry facts are accepted"), CanSeedShopUpgradePIEValidation(Facts, Reason));
	TestTrue(TEXT("Accepted facts have no reason"), Reason.IsNone());

	auto ExpectRejected = [this, &Facts](const TCHAR* Label, TFunctionRef<void(FWacomShopUpgradePIEValidationFacts&)> Mutate)
	{
		FWacomShopUpgradePIEValidationFacts Candidate = Facts;
		Mutate(Candidate);
		FName DisabledReason;
		TestFalse(Label, CanSeedShopUpgradePIEValidation(Candidate, DisabledReason));
		TestFalse(FString(Label) + TEXT(" reason"), DisabledReason.IsNone());
	};
	ExpectRejected(TEXT("Non PIE rejected"), [](auto& F) { F.bIsPIEWorld = false; });
	ExpectRejected(TEXT("Inactive Run rejected"), [](auto& F) { F.bRunActive = false; });
	ExpectRejected(TEXT("Wrong Journey rejected"), [](auto& F) { F.JourneyId = TEXT("Journey.Other"); });
	ExpectRejected(TEXT("Wrong Floor rejected"), [](auto& F) { F.CurrentNode.FloorId = TEXT("Floor.Other"); });
	ExpectRejected(TEXT("Wrong Node rejected"), [](auto& F) { F.CurrentNode.NodeId = TEXT("Battle.Snake"); });
	ExpectRejected(TEXT("Active interaction rejected"), [](auto& F)
	{
		F.ActiveActivityKind = ERunExplorationActivityKind::NodeActivity;
	});
	return true;
}

#endif
