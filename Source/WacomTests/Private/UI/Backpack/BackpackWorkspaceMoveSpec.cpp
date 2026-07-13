// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackCommandFlow.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceStateSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceMoveIntentSpec,
	"Wacom.UI.Backpack.Workspace.MoveIntentContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceMoveIntentSpec::RunTest(const FString& Parameters)
{
	const FGuid First(1, 2, 3, 4);
	const FGuid Second(5, 6, 7, 8);
	const FWacomBackpackZoneKey Source = FWacomBackpackZoneKey::Make(EZoneKind::Backpack);
	const FWacomBackpackZoneKey Target = FWacomBackpackZoneKey::Make(EZoneKind::BattleDeck);
	FWacomBackpackWorkspaceInteractionModel Model;
	TArray<FWacomBackpackWorkspaceCardHitRecord> Cards = {
		{ First, FVector2D(100.f, 100.f), 0, true },
		{ Second, FVector2D(200.f, 100.f), 1, true },
	};
	Model.ReconcileCards(Source, Cards);
	Model.SelectAllMovable();
	TestTrue(TEXT("Move carry begins"), Model.BeginCarry(First, FVector2D(400.f, 300.f), 12));
	Model.BuildReleaseIntent(false); // consume pickup release
	const FWacomBackpackWorkspaceReleaseIntent One = Model.BuildReleaseIntent(false);
	const FRunDeckBatchMoveRequest OneRequest = FWacomBackpackCommandFlow::BuildBatchMoveRequest(
		Model.GetCarry(), Target, One.InstanceIds);
	TestEqual(TEXT("Left release maps current card only"), OneRequest.InstanceIds.Num(), 1);
	TestEqual(TEXT("Move request preserves source revision"), OneRequest.ExpectedStorageRevision, uint64(12));
	TestEqual(TEXT("Rejected preview leaves carry unchanged"), Model.GetCarry().RemainingInstanceIds.Num(), 2);

	const FWacomBackpackWorkspaceReleaseIntent All = Model.BuildReleaseIntent(true);
	const FRunDeckBatchMoveRequest AllRequest = FWacomBackpackCommandFlow::BuildBatchMoveRequest(
		Model.GetCarry(), Target, All.InstanceIds);
	TestEqual(TEXT("Right release maps all remaining cards"), AllRequest.InstanceIds.Num(), 2);

	FWacomBackpackWorkspaceStateStore Store;
	FWacomBackpackWorkspaceLayoutEntry Entry;
	Entry.bHasManualPlacement = true;
	Store.SetLayout(Source, First, Entry);
	Store.SetLayout(Source, Second, Entry);
	const TArray<FGuid> BothIds = { First, Second };
	const TArray<FGuid> FirstOnly = { First };
	TestTrue(TEXT("Same-zone collect is accepted as presentation-only"),
		FWacomBackpackCommandFlow::CollectSameZone(Store, Source, Source, BothIds));
	TestEqual(TEXT("Same-zone collect clears layouts"), Store.GetManualLayoutCount(Source), 0);
	TestFalse(TEXT("Cross-zone collect cannot bypass Run batch path"),
		FWacomBackpackCommandFlow::CollectSameZone(Store, Source, Target, FirstOnly));
	return true;
}

#endif
