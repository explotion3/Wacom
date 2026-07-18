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
	FWacomUIBackpackWorkspacePartialPileReleaseSpec,
	"Wacom.UI.Backpack.Workspace.PartialPileReleasePreservesCarry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspacePartialPileReleaseSpec::RunTest(const FString& Parameters)
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
	TestTrue(TEXT("Partial pile release Run initializes"), InitializeRunSessionForTest(*Run, Character).IsOk());
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

	TestTrue(TEXT("Left release reaches the BattleDeck pile through the production Screen flow"),
		FWacomBackpackScreenTestAccess::ReleaseCurrentToPileWithSynchronousRefresh(
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
		FWacomBackpackScreenTestAccess::ReleaseCurrentToPileWithSynchronousRefresh(
			*Screen,
			EZoneKind::BattleDeck));
	TestTrue(TEXT("The second successful left release completes carry"),
		FWacomBackpackScreenTestAccess::WorkspaceView(*Screen).CarriedInstanceIds.IsEmpty());
	TestEqual(TEXT("Both cards reach the target through consecutive left releases"),
		Run->BuildBackpackStorageSnapshot().BattleDeckPhysicalCards.Num(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceEmptyPileBatchReceiveCollapseSpec,
	"Wacom.UI.Backpack.Workspace.EmptyPileBatchReceiveCanCollapse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceEmptyPileBatchReceiveCollapseSpec::RunTest(
	const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	UCardDefinition* Bag = NewObject<UCardDefinition>(Outer);
	Bag->CardId = TEXT("WorkspaceEmptyPile.Bag");
	Bag->Physique.Capacity = 5;
	UCardDefinition* FirstCard = NewObject<UCardDefinition>(Outer);
	FirstCard->CardId = TEXT("WorkspaceEmptyPile.First");
	UCardDefinition* SecondCard = NewObject<UCardDefinition>(Outer);
	SecondCard->CardId = TEXT("WorkspaceEmptyPile.Second");
	UCharacterDefinition* Character = NewObject<UCharacterDefinition>(Outer);
	Character->CharacterId = TEXT("WorkspaceEmptyPile.Character");
	Character->StarterDeck.Add(Bag);
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(Outer));
	TestTrue(TEXT("Empty-pile regression Run initializes"),
		InitializeRunSessionForTest(*Run, Character).IsOk());
	Run->AcquireCardToRun(FirstCard);
	Run->AcquireCardToRun(SecondCard);

	const FRunBackpackStorageSnapshot Snapshot = Run->BuildBackpackStorageSnapshot();
	TestTrue(TEXT("Regression fixture starts with an empty BattleDeck"),
		Snapshot.BattleDeckPhysicalCards.IsEmpty());
	TArray<FGuid> SourceIds;
	for (const FRunStorageCardView& Card : Snapshot.Flux.ContentCards)
	{
		if (Card.Instance.Definition == FirstCard || Card.Instance.Definition == SecondCard)
		{
			SourceIds.Add(Card.Instance.InstanceId);
		}
	}
	TestEqual(TEXT("Two source cards are available for the batch move"), SourceIds.Num(), 2);
	if (SourceIds.Num() != 2)
	{
		return false;
	}

	TStrongObjectPtr<UWacomBackpackScreen> Screen(
		FWacomBackpackScreenTestAccess::Create(Outer, Run.Get()));
	FWacomBackpackScreenTestAccess::ActivateZone(*Screen, EZoneKind::BattleDeck);
	TestEqual(TEXT("The empty target pile expands before receiving cards"),
		FWacomBackpackScreenTestAccess::ActiveWorkspaceZone(*Screen),
		EZoneKind::BattleDeck);
	TestTrue(TEXT("Both cards enter carry"),
		FWacomBackpackScreenTestAccess::BeginWorkspaceCarryForIds(*Screen, SourceIds));
	TestTrue(TEXT("A single batch release reaches the empty target pile"),
		FWacomBackpackScreenTestAccess::ReleaseAllToPileWithSynchronousRefresh(
			*Screen, EZoneKind::BattleDeck));
	TestEqual(TEXT("The target pile now owns both cards"),
		Run->BuildBackpackStorageSnapshot().BattleDeckPhysicalCards.Num(), 2);
	TestEqual(TEXT("Receiving cards keeps the target pile expanded"),
		FWacomBackpackScreenTestAccess::ActiveWorkspaceZone(*Screen),
		EZoneKind::BattleDeck);
	TestTrue(TEXT("The batch release completes carry before the collapse click"),
		FWacomBackpackScreenTestAccess::WorkspaceView(*Screen).CarriedInstanceIds.IsEmpty());

	TestTrue(TEXT("A card intercepting the header still routes the click to the pile"),
		FWacomBackpackScreenTestAccess::ClickExpandedPileHeaderThroughOverlappingCard(
			*Screen, EZoneKind::BattleDeck));
	TestEqual(TEXT("The populated target pile can collapse immediately after the batch move"),
		FWacomBackpackScreenTestAccess::ActiveWorkspaceZone(*Screen),
		EZoneKind::Backpack);
	return true;
}

#endif
