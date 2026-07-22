// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceNavigationController.h"

namespace
{
FWacomBackpackWorkspaceNavigationTarget MakeCardTarget(
	FGuid InstanceId,
	FVector2D Center,
	int32 LayerRank = 0)
{
	FWacomBackpackWorkspaceNavigationTarget Target;
	Target.Kind = EWacomBackpackWorkspaceNavigationTargetKind::Card;
	Target.InstanceId = InstanceId;
	Target.Center = Center;
	Target.LayerRank = LayerRank;
	return Target;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceNavigationSpec,
	"Wacom.UI.Backpack.Workspace.Navigation.StableSpatialFocus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceNavigationSpec::RunTest(const FString& Parameters)
{
	const FGuid LeftId(1, 0, 0, 0);
	const FGuid NearRightId(2, 0, 0, 0);
	const FGuid FarRightId(3, 0, 0, 0);
	FWacomBackpackWorkspaceNavigationController Navigation;
	Navigation.ReconcileTargets(TArray<FWacomBackpackWorkspaceNavigationTarget>{
		MakeCardTarget(LeftId, FVector2D(0.0f, 0.0f)),
		MakeCardTarget(NearRightId, FVector2D(100.0f, 10.0f)),
		MakeCardTarget(FarRightId, FVector2D(100.0f, 90.0f)) });

	TestTrue(TEXT("Right navigation finds a spatial neighbor"),
		Navigation.Move(EUINavigation::Right));
	TestTrue(TEXT("Right navigation prefers the least perpendicular candidate"),
		Navigation.IsCardFocused(NearRightId));
	TestTrue(TEXT("Navigation activates semantic focus"),
		Navigation.IsSemanticFocusActive());

	Navigation.ReconcileTargets(TArray<FWacomBackpackWorkspaceNavigationTarget>{
		MakeCardTarget(FarRightId, FVector2D(100.0f, 90.0f)),
		MakeCardTarget(NearRightId, FVector2D(105.0f, 12.0f)),
		MakeCardTarget(LeftId, FVector2D(0.0f, 0.0f)) });
	TestTrue(TEXT("Equivalent refresh preserves focus by InstanceId"),
		Navigation.IsCardFocused(NearRightId));

	Navigation.ReconcileTargets(TArray<FWacomBackpackWorkspaceNavigationTarget>{
		MakeCardTarget(FarRightId, FVector2D(115.0f, 45.0f)),
		MakeCardTarget(LeftId, FVector2D(0.0f, 0.0f)) });
	TestTrue(TEXT("Removed focus falls back to the nearest legal target"),
		Navigation.IsCardFocused(FarRightId));

	Navigation.NotifyPointerInput();
	TestFalse(TEXT("Pointer input relinquishes semantic targeting"),
		Navigation.IsSemanticFocusActive());
	Navigation.ActivateSemanticFocus();
	TestTrue(TEXT("Keyboard or gamepad action restores semantic targeting"),
		Navigation.IsSemanticFocusActive());
	return true;
}

#endif
