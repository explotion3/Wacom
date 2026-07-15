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

	FWacomBackpackScreenTestAccess::ActivateZone(*Screen, EZoneKind::Backpack);
	FWacomBackpackScreenTestAccess::Refresh(*Screen);
	TestTrue(TEXT("Fresh carry starts after stale restore"),
		FWacomBackpackScreenTestAccess::BeginWorkspaceCarryForIds(*Screen, DeleteIds));
	TestTrue(TEXT("Fresh confirmation opens"), FWacomBackpackScreenTestAccess::BeginDeleteConfirmation(*Screen));
	FWacomBackpackScreenTestAccess::ConfirmDelete(*Screen);
	TestTrue(TEXT("Successful confirm exits carry"),
		FWacomBackpackScreenTestAccess::WorkspaceView(*Screen).CarriedInstanceIds.IsEmpty());
	TestFalse(TEXT("Successful confirm removes white"),
		Run->BuildBackpackStorageSnapshot().Flux.ContentCards.ContainsByPredicate(
			[White](const FRunStorageCardView& View) { return View.Instance.Definition == White; }));
	return true;
}

#endif
