// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Fixtures/WacomRunExplorationFixture.h"

#if WITH_AUTOMATION_TESTS

#include "../BackpackScreenTestAccess.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackCommandFlow.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceStateSubsystem.h"
#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "RunSession.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UObject/StrongObjectPtr.h"

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspacePartialRackReleaseSpec,
	"Wacom.UI.Backpack.Workspace.PartialRackReleasePreservesCarry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspacePartialRackReleaseSpec::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	UCardDefinition* Bag = NewObject<UCardDefinition>(Outer);
	Bag->CardId = TEXT("WorkspaceMove.Bag");
	Bag->Physique.Capacity = 5;
	UCardDefinition* FirstCard = NewObject<UCardDefinition>(Outer);
	FirstCard->CardId = TEXT("WorkspaceMove.First");
	UCardDefinition* SecondCard = NewObject<UCardDefinition>(Outer);
	SecondCard->CardId = TEXT("WorkspaceMove.Second");
	UCharacterDefinition* Character = NewObject<UCharacterDefinition>(Outer);
	Character->CharacterId = TEXT("WorkspaceMove.Character");
	Character->StarterDeck.Add(Bag);
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(Outer));
	TestTrue(TEXT("Partial rack release Run initializes"), InitializeRunSessionForTest(*Run, Character).IsOk());
	Run->AcquireCardToRun(FirstCard);
	Run->AcquireCardToRun(SecondCard);

	const FRunBackpackStorageSnapshot Snapshot = Run->BuildBackpackStorageSnapshot();
	const FRunStorageCardView* FirstView = Snapshot.Flux.ContentCards.FindByPredicate(
		[FirstCard](const FRunStorageCardView& View)
		{
			return View.Instance.Definition == FirstCard;
		});
	const FRunStorageCardView* SecondView = Snapshot.Flux.ContentCards.FindByPredicate(
		[SecondCard](const FRunStorageCardView& View)
		{
			return View.Instance.Definition == SecondCard;
		});
	TestTrue(TEXT("Two movable cards exist in the source workspace"), FirstView && SecondView);
	if (!FirstView || !SecondView)
	{
		return false;
	}
	const TArray<FGuid> SourceIds = {
		FirstView->Instance.InstanceId,
		SecondView->Instance.InstanceId,
	};

	TStrongObjectPtr<UWacomBackpackScreen> Screen(
		FWacomBackpackScreenTestAccess::Create(Outer, Run.Get()));
	TestTrue(TEXT("Two source cards enter carry"),
		FWacomBackpackScreenTestAccess::BeginWorkspaceCarryForIds(*Screen, SourceIds));
	const FWacomBackpackWorkspaceAutomationTestView BeforeRelease =
		FWacomBackpackScreenTestAccess::WorkspaceView(*Screen);
	TestEqual(TEXT("Fixture carries two cards"), BeforeRelease.CarriedInstanceIds.Num(), 2);
	const FGuid ReleasedId = BeforeRelease.CarriedInstanceIds.IsValidIndex(BeforeRelease.CurrentCarryIndex)
		? BeforeRelease.CarriedInstanceIds[BeforeRelease.CurrentCarryIndex]
		: FGuid();

	TestTrue(TEXT("Left release reaches the BattleDeck rack through the production Screen flow"),
		FWacomBackpackScreenTestAccess::ReleaseCurrentToRackWithSynchronousRefresh(
			*Screen,
			EZoneKind::BattleDeck));
	const FWacomBackpackWorkspaceAutomationTestView AfterRelease =
		FWacomBackpackScreenTestAccess::WorkspaceView(*Screen);
	TestEqual(TEXT("The unreleased card remains carried after synchronous Run refresh"),
		AfterRelease.CarriedInstanceIds.Num(), 1);
	TestFalse(TEXT("The released card is removed from carry"),
		AfterRelease.CarriedInstanceIds.Contains(ReleasedId));
	TestTrue(TEXT("The released card reaches the target zone"),
		Run->BuildBackpackStorageSnapshot().BattleDeckPhysicalCards.ContainsByPredicate(
			[ReleasedId](const FRunStorageCardView& View)
			{
				return View.Instance.InstanceId == ReleasedId;
			}));
	TestTrue(TEXT("The remaining card can use the refreshed storage revision immediately"),
		FWacomBackpackScreenTestAccess::ReleaseCurrentToRackWithSynchronousRefresh(
			*Screen,
			EZoneKind::BattleDeck));
	TestTrue(TEXT("The second successful left release completes carry"),
		FWacomBackpackScreenTestAccess::WorkspaceView(*Screen).CarriedInstanceIds.IsEmpty());
	TestEqual(TEXT("Both cards reach the target through consecutive left releases"),
		Run->BuildBackpackStorageSnapshot().BattleDeckPhysicalCards.Num(), 2);
	return true;
}

#endif
