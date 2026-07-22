// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceReleaseIntentSpec,
	"Wacom.UI.Backpack.Workspace.ReleaseIntent.ExplicitTargets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceReleaseIntentSpec::RunTest(const FString& Parameters)
{
	const FGuid FirstId(11, 0, 0, 0);
	const FGuid SecondId(12, 0, 0, 0);
	const FWacomBackpackZoneKey Source = FWacomBackpackZoneKey::Make(EZoneKind::Backpack);
	const FWacomBackpackZoneKey Pile = FWacomBackpackZoneKey::Make(
		EZoneKind::SpecialZone, FGuid(21, 0, 0, 0));
	FWacomBackpackWorkspaceInteractionModel Model;
	Model.ReconcileCards(Source, TArray<FWacomBackpackWorkspaceCardHitRecord>{
		FWacomBackpackWorkspaceCardHitRecord(FirstId, Source, FVector2D(10.0f, 10.0f), 0, true),
		FWacomBackpackWorkspaceCardHitRecord(SecondId, Source, FVector2D(40.0f, 10.0f), 1, true) });
	Model.SelectAllMovable(Source);
	TestTrue(TEXT("Carry starts for explicit-target validation"),
		Model.BeginCarry(FirstId, FVector2D(100.0f, 100.0f), 7));
	Model.NotifyReleaseGestureStarted();

	const FWacomBackpackWorkspaceReleaseIntent PileIntent = Model.BuildReleaseIntent(
		false, EWacomBackpackWorkspaceReleaseTargetKind::Pile, Pile);
	TestEqual(TEXT("Single release preserves explicit pile kind"),
		PileIntent.TargetKind, EWacomBackpackWorkspaceReleaseTargetKind::Pile);
	TestTrue(TEXT("Single release preserves pile identity"), PileIntent.TargetZone == Pile);
	TestEqual(TEXT("Single release contains one current card"), PileIntent.InstanceIds.Num(), 1);

	const FWacomBackpackWorkspaceReleaseIntent DeleteIntent = Model.BuildReleaseIntent(
		true, EWacomBackpackWorkspaceReleaseTargetKind::Delete);
	TestEqual(TEXT("All release preserves explicit delete kind"),
		DeleteIntent.TargetKind, EWacomBackpackWorkspaceReleaseTargetKind::Delete);
	TestTrue(TEXT("All release marks release-all semantics"), DeleteIntent.bReleaseAll);
	TestEqual(TEXT("All release contains the complete carried set"),
		DeleteIntent.InstanceIds.Num(), 2);

	const FWacomBackpackWorkspaceReleaseIntent PointerIntent = Model.BuildReleaseIntent(false);
	TestEqual(TEXT("Mouse path remains an explicit pointer target"),
		PointerIntent.TargetKind, EWacomBackpackWorkspaceReleaseTargetKind::Pointer);
	return true;
}

#endif
