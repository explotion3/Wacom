// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Engine/GameInstance.h"
#include "UI/Foundation/WacomGameUIManagerSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "../../../WacomApp/Private/UI/Battle/WacomBattleViewportLayerPolicy.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePrimaryLayoutViewportLeaseSpec,
	"Wacom.UI.Battle.ViewportLayer.PrimaryLayoutLeaseOrdering",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePrimaryLayoutViewportLeaseSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomGameUIManagerSubsystem> UIManager(
		NewObject<UWacomGameUIManagerSubsystem>(GameInstance.Get()));
	if (!TestNotNull(TEXT("UI manager"), UIManager.Get()))
	{
		return false;
	}

	TestEqual(
		TEXT("PrimaryLayout starts at its base viewport depth"),
		UIManager->GetEffectivePrimaryLayoutViewportZOrder(),
		0);

	const uint64 LowerLease =
		UIManager->AcquirePrimaryLayoutViewportZOrderLease(8501);
	const uint64 HigherLease =
		UIManager->AcquirePrimaryLayoutViewportZOrderLease(9998);
	TestNotEqual(TEXT("First lease handle is valid"), LowerLease, uint64(0));
	TestNotEqual(TEXT("Lease handles are unique"), HigherLease, LowerLease);
	TestEqual(
		TEXT("Greatest active viewport-depth request wins"),
		UIManager->GetEffectivePrimaryLayoutViewportZOrder(),
		9998);

	UIManager->ReleasePrimaryLayoutViewportZOrderLease(HigherLease);
	TestEqual(
		TEXT("Releasing the greatest request restores the remaining lease"),
		UIManager->GetEffectivePrimaryLayoutViewportZOrder(),
		8501);

	UIManager->ReleasePrimaryLayoutViewportZOrderLease(HigherLease);
	TestEqual(
		TEXT("Duplicate release is idempotent"),
		UIManager->GetEffectivePrimaryLayoutViewportZOrder(),
		8501);

	UIManager->ReleasePrimaryLayoutViewportZOrderLease(LowerLease);
	TestEqual(
		TEXT("Last release restores the base viewport depth"),
		UIManager->GetEffectivePrimaryLayoutViewportZOrder(),
		0);

	UIManager->AcquirePrimaryLayoutViewportZOrderLease(9000);
	UIManager->TearDownPrimaryLayout();
	TestEqual(
		TEXT("PrimaryLayout teardown discards stale viewport-depth requests"),
		UIManager->GetEffectivePrimaryLayoutViewportZOrder(),
		0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleFirstPersonOverlayHierarchySpec,
	"Wacom.UI.Battle.ViewportLayer.FirstPersonHandHierarchy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleFirstPersonOverlayHierarchySpec::RunTest(
	const FString& /*Parameters*/)
{
	const UWacomFirstPersonCardAnchorComponent* Anchor =
		GetDefault<UWacomFirstPersonCardAnchorComponent>();
	if (!TestNotNull(TEXT("First-person card anchor CDO"), Anchor))
	{
		return false;
	}

	const int32 InspectionZOrder =
		WacomBattleViewportLayerPolicy::ResolveInspectionPanelZOrder(
			Anchor->CardLayerZOrder);
	const int32 SecondaryPanelZOrder =
		WacomBattleViewportLayerPolicy::ResolveSecondaryPanelZOrder(
			Anchor->CardLayerZOrder);

	TestTrue(
		TEXT("Enemy inspection renders above the authored hand viewport layer"),
		InspectionZOrder > Anchor->CardLayerZOrder);
	TestTrue(
		TEXT("Battle secondary panels render above the authored hand viewport layer"),
		SecondaryPanelZOrder > Anchor->CardLayerZOrder);
	TestTrue(
		TEXT("Full-screen Battle secondary panels render above enemy inspection"),
		SecondaryPanelZOrder > InspectionZOrder);
	TestTrue(
		TEXT("Enemy compact panels stay below inspection"),
		WacomBattleViewportLayerPolicy::CompactPanelZOrder < InspectionZOrder);
	return true;
}
