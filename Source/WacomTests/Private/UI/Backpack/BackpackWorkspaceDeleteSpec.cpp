// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Fixtures/WacomRunExplorationFixture.h"

#if WITH_AUTOMATION_TESTS

#include "../BackpackScreenTestAccess.h"
#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "RunSession.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceDeleteRestoreSpec,
	"Wacom.UI.Backpack.Workspace.DeleteConfirmationRestore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceDeleteRestoreSpec::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	UCardDefinition* Bag = NewObject<UCardDefinition>(Outer);
	Bag->CardId = TEXT("WorkspaceDelete.Bag");
	Bag->Physique.Capacity = 5;
	UCardDefinition* White = NewObject<UCardDefinition>(Outer);
	White->CardId = TEXT("WorkspaceDelete.White");
	White->Rarity = WacomTags::Card_Rarity_White;
	UCardDefinition* Blue = NewObject<UCardDefinition>(Outer);
	Blue->CardId = TEXT("WorkspaceDelete.Blue");
	Blue->Rarity = WacomTags::Card_Rarity_Blue;
	UCharacterDefinition* Character = NewObject<UCharacterDefinition>(Outer);
	Character->CharacterId = TEXT("WorkspaceDelete.Character");
	Character->StarterDeck.Add(Bag);
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(Outer));
	TestTrue(TEXT("Delete restore Run initializes"), InitializeRunSessionForTest(*Run, Character).IsOk());
	Run->AcquireCardToRun(White);
	Run->AcquireCardToRun(Blue);
	const FRunBackpackStorageSnapshot Snapshot = Run->BuildBackpackStorageSnapshot();
	const FRunStorageCardView* WhiteView = Snapshot.Flux.ContentCards.FindByPredicate(
		[White](const FRunStorageCardView& View) { return View.Instance.Definition == White; });
	const FRunStorageCardView* BlueView = Snapshot.Flux.ContentCards.FindByPredicate(
		[Blue](const FRunStorageCardView& View) { return View.Instance.Definition == Blue; });
	TestTrue(TEXT("Delete candidates exist in active workspace"), WhiteView && BlueView);
	const TArray<FGuid> DeleteIds = { WhiteView->Instance.InstanceId, BlueView->Instance.InstanceId };

	TStrongObjectPtr<UWacomBackpackScreen> Screen(
		FWacomBackpackScreenTestAccess::Create(Outer, Run.Get()));
	TestTrue(TEXT("Selected delete cards enter carry"),
		FWacomBackpackScreenTestAccess::BeginWorkspaceCarryForIds(*Screen, DeleteIds));
	const FWacomBackpackWorkspaceAutomationTestView BeforeConfirm =
		FWacomBackpackScreenTestAccess::WorkspaceView(*Screen);
	TestTrue(TEXT("Delete confirmation opens"), FWacomBackpackScreenTestAccess::BeginDeleteConfirmation(*Screen));
	TestEqual(TEXT("Confirmation shows batch count"), FWacomBackpackScreenTestAccess::DeletePreviewCardCount(*Screen), 2);
	TestEqual(TEXT("Confirmation shows summed reward"), FWacomBackpackScreenTestAccess::DeletePreviewGoldReward(*Screen), 3);
	FWacomBackpackScreenTestAccess::CancelDelete(*Screen);
	const FWacomBackpackWorkspaceAutomationTestView AfterCancel =
		FWacomBackpackScreenTestAccess::WorkspaceView(*Screen);
	TestEqual(TEXT("Cancel restores exact carry order"), AfterCancel.CarriedInstanceIds, BeforeConfirm.CarriedInstanceIds);
	TestEqual(TEXT("Cancel restores current card"), AfterCancel.CurrentCarryIndex, BeforeConfirm.CurrentCarryIndex);

	TestTrue(TEXT("Confirmation reopens"), FWacomBackpackScreenTestAccess::BeginDeleteConfirmation(*Screen));
	UCardDefinition* ExternalChange = NewObject<UCardDefinition>(Outer);
	ExternalChange->CardId = TEXT("WorkspaceDelete.ExternalChange");
	Run->AcquireCardToRun(ExternalChange);
	FWacomBackpackScreenTestAccess::ConfirmDelete(*Screen);
	TestTrue(TEXT("Stale confirm restores carry"),
		!FWacomBackpackScreenTestAccess::WorkspaceView(*Screen).CarriedInstanceIds.IsEmpty());
	TestTrue(TEXT("Stale confirm deletes zero cards"),
		Run->BuildBackpackStorageSnapshot().Flux.ContentCards.ContainsByPredicate(
			[White](const FRunStorageCardView& View) { return View.Instance.Definition == White; }));

	FWacomBackpackScreenTestAccess::Refresh(*Screen);
	TestTrue(TEXT("Restored carry retries after revision reconcile"),
		FWacomBackpackScreenTestAccess::BeginDeleteConfirmation(*Screen));
	FWacomBackpackScreenTestAccess::ConfirmDelete(*Screen);
	TestTrue(TEXT("Successful confirm exits carry"),
		FWacomBackpackScreenTestAccess::WorkspaceView(*Screen).CarriedInstanceIds.IsEmpty());
	TestFalse(TEXT("Successful confirm removes white"),
		Run->BuildBackpackStorageSnapshot().Flux.ContentCards.ContainsByPredicate(
			[White](const FRunStorageCardView& View) { return View.Instance.Definition == White; }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspacePartialDeletePreservesCarrySpec,
	"Wacom.UI.Backpack.Workspace.PartialDeletePreservesCarry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspacePartialDeletePreservesCarrySpec::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	UCardDefinition* Bag = NewObject<UCardDefinition>(Outer);
	Bag->CardId = TEXT("WorkspacePartialDelete.Bag");
	Bag->Physique.Capacity = 5;
	UCardDefinition* FirstCard = NewObject<UCardDefinition>(Outer);
	FirstCard->CardId = TEXT("WorkspacePartialDelete.First");
	FirstCard->Rarity = WacomTags::Card_Rarity_White;
	UCardDefinition* SecondCard = NewObject<UCardDefinition>(Outer);
	SecondCard->CardId = TEXT("WorkspacePartialDelete.Second");
	SecondCard->Rarity = WacomTags::Card_Rarity_Blue;
	UCharacterDefinition* Character = NewObject<UCharacterDefinition>(Outer);
	Character->CharacterId = TEXT("WorkspacePartialDelete.Character");
	Character->StarterDeck.Add(Bag);
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(Outer));
	TestTrue(TEXT("Partial delete Run initializes"), InitializeRunSessionForTest(*Run, Character).IsOk());
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
	TestTrue(TEXT("Two delete candidates exist"), FirstView && SecondView);
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
	TestTrue(TEXT("Two delete candidates enter carry"),
		FWacomBackpackScreenTestAccess::BeginWorkspaceCarryForIds(*Screen, SourceIds));
	const FWacomBackpackWorkspaceAutomationTestView BeforeConfirm =
		FWacomBackpackScreenTestAccess::WorkspaceView(*Screen);
	const FGuid CurrentId = BeforeConfirm.CarriedInstanceIds.IsValidIndex(BeforeConfirm.CurrentCarryIndex)
		? BeforeConfirm.CarriedInstanceIds[BeforeConfirm.CurrentCarryIndex]
		: FGuid();
	const TArray<FGuid> CurrentOnly{ CurrentId };
	TestTrue(TEXT("Left delete release opens confirmation for the current card only"),
		FWacomBackpackScreenTestAccess::BeginDeleteConfirmationForIds(*Screen, CurrentOnly));
	TestEqual(TEXT("Single-card delete confirmation previews one card"),
		FWacomBackpackScreenTestAccess::DeletePreviewCardCount(*Screen), 1);
	const FWacomBackpackWorkspaceAutomationTestView DuringConfirm =
		FWacomBackpackScreenTestAccess::WorkspaceView(*Screen);
	TestEqual(TEXT("The carried fan stays visible while delete input is suspended"),
		DuringConfirm.CarriedInstanceIds, BeforeConfirm.CarriedInstanceIds);
	TestFalse(TEXT("Delete confirmation releases Workspace mouse capture"),
		DuringConfirm.bMouseCaptured);
	FWacomBackpackScreenTestAccess::ConfirmDelete(*Screen);

	const FWacomBackpackWorkspaceAutomationTestView AfterConfirm =
		FWacomBackpackScreenTestAccess::WorkspaceView(*Screen);
	TestEqual(TEXT("The unreleased card resumes carry after current-card deletion"),
		AfterConfirm.CarriedInstanceIds.Num(), 1);
	TestFalse(TEXT("The deleted current card is removed from carry"),
		AfterConfirm.CarriedInstanceIds.Contains(CurrentId));
	TestFalse(TEXT("The confirmed current card is removed from storage"),
		Run->BuildBackpackStorageSnapshot().Flux.ContentCards.ContainsByPredicate(
			[CurrentId](const FRunStorageCardView& View)
			{
				return View.Instance.InstanceId == CurrentId;
			}));
	return true;
}

#endif
