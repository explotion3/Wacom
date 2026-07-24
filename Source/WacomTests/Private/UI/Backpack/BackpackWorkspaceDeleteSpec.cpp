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
	FWacomUIBackpackWorkspaceImmediateDeleteSpec,
	"Wacom.UI.Backpack.Workspace.ImmediateDeleteAtomic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceImmediateDeleteSpec::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	UCardDefinition* Bag = NewObject<UCardDefinition>(Outer);
	Bag->CardId = TEXT("WorkspaceDelete.Bag");
	Bag->Physique.Capacity = 5;
	UCardDefinition* DeleteProvider = NewObject<UCardDefinition>(Outer);
	DeleteProvider->CardId = TEXT("WorkspaceDelete.Provider");
	DeleteProvider->Keywords.AddTag(
		WacomTags::Card_Keyword_DeleteProvider);
	UCardDefinition* White = NewObject<UCardDefinition>(Outer);
	White->CardId = TEXT("WorkspaceDelete.White");
	White->Rarity = WacomTags::Card_Rarity_White;
	UCardDefinition* Blue = NewObject<UCardDefinition>(Outer);
	Blue->CardId = TEXT("WorkspaceDelete.Blue");
	Blue->Rarity = WacomTags::Card_Rarity_Blue;
	UCharacterDefinition* Character = NewObject<UCharacterDefinition>(Outer);
	Character->CharacterId = TEXT("WorkspaceDelete.Character");
	Character->StarterDeck.Add(Bag);
	Character->StarterDeck.Add(DeleteProvider);
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
	const FWacomBackpackWorkspaceAutomationTestView BeforeRejectedDelete =
		FWacomBackpackScreenTestAccess::WorkspaceView(*Screen);
	TestFalse(TEXT("Delete confirmation widget is not created"),
		FWacomBackpackScreenTestAccess::HasRuntimeDeleteConfirmationWidget(*Screen));
	TestFalse(TEXT("Delete confirmation host stays collapsed"),
		FWacomBackpackScreenTestAccess::IsDeleteConfirmationHostVisible(*Screen));

	UCardDefinition* ExternalChange = NewObject<UCardDefinition>(Outer);
	ExternalChange->CardId = TEXT("WorkspaceDelete.ExternalChange");
	Run->AcquireCardToRun(ExternalChange);
	FWacomBackpackScreenTestAccess::SubmitWorkspaceDelete(*Screen, DeleteIds);
	const FWacomBackpackWorkspaceAutomationTestView AfterRejectedDelete =
		FWacomBackpackScreenTestAccess::WorkspaceView(*Screen);
	TestEqual(TEXT("Stale immediate delete preserves exact carry order"),
		AfterRejectedDelete.CarriedInstanceIds,
		BeforeRejectedDelete.CarriedInstanceIds);
	TestEqual(TEXT("Stale immediate delete preserves current card"),
		AfterRejectedDelete.CurrentCarryIndex,
		BeforeRejectedDelete.CurrentCarryIndex);
	TestTrue(TEXT("Stale immediate delete removes zero cards"),
		Run->BuildBackpackStorageSnapshot().Flux.ContentCards.ContainsByPredicate(
			[White](const FRunStorageCardView& View) { return View.Instance.Definition == White; }));
	TestEqual(TEXT("Rejected immediate delete grants zero gold"), Run->GetGold(), 0);

	FWacomBackpackScreenTestAccess::Refresh(*Screen);
	FWacomBackpackScreenTestAccess::SubmitWorkspaceDelete(*Screen, DeleteIds);
	TestTrue(TEXT("Successful immediate delete exits carry"),
		FWacomBackpackScreenTestAccess::WorkspaceView(*Screen).CarriedInstanceIds.IsEmpty());
	TestFalse(TEXT("Successful immediate delete removes white"),
		Run->BuildBackpackStorageSnapshot().Flux.ContentCards.ContainsByPredicate(
			[White](const FRunStorageCardView& View) { return View.Instance.Definition == White; }));
	TestFalse(TEXT("Successful immediate delete removes blue"),
		Run->BuildBackpackStorageSnapshot().Flux.ContentCards.ContainsByPredicate(
			[Blue](const FRunStorageCardView& View) { return View.Instance.Definition == Blue; }));
	TestEqual(TEXT("Successful immediate delete grants exact batch reward"), Run->GetGold(), 3);
	TestFalse(TEXT("Immediate delete never opens the compatibility host"),
		FWacomBackpackScreenTestAccess::IsDeleteConfirmationHostVisible(*Screen));
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
	UCardDefinition* DeleteProvider = NewObject<UCardDefinition>(Outer);
	DeleteProvider->CardId = TEXT("WorkspacePartialDelete.Provider");
	DeleteProvider->Keywords.AddTag(
		WacomTags::Card_Keyword_DeleteProvider);
	UCardDefinition* FirstCard = NewObject<UCardDefinition>(Outer);
	FirstCard->CardId = TEXT("WorkspacePartialDelete.First");
	FirstCard->Rarity = WacomTags::Card_Rarity_White;
	UCardDefinition* SecondCard = NewObject<UCardDefinition>(Outer);
	SecondCard->CardId = TEXT("WorkspacePartialDelete.Second");
	SecondCard->Rarity = WacomTags::Card_Rarity_Blue;
	UCharacterDefinition* Character = NewObject<UCharacterDefinition>(Outer);
	Character->CharacterId = TEXT("WorkspacePartialDelete.Character");
	Character->StarterDeck.Add(Bag);
	Character->StarterDeck.Add(DeleteProvider);
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
	const FWacomBackpackWorkspaceAutomationTestView BeforeDelete =
		FWacomBackpackScreenTestAccess::WorkspaceView(*Screen);
	const FGuid CurrentId = BeforeDelete.CarriedInstanceIds.IsValidIndex(BeforeDelete.CurrentCarryIndex)
		? BeforeDelete.CarriedInstanceIds[BeforeDelete.CurrentCarryIndex]
		: FGuid();
	const TArray<FGuid> CurrentOnly{ CurrentId };
	FWacomBackpackScreenTestAccess::SubmitWorkspaceDelete(*Screen, CurrentOnly);

	const FWacomBackpackWorkspaceAutomationTestView AfterDelete =
		FWacomBackpackScreenTestAccess::WorkspaceView(*Screen);
	TestEqual(TEXT("The unreleased card resumes carry after current-card deletion"),
		AfterDelete.CarriedInstanceIds.Num(), 1);
	TestFalse(TEXT("The deleted current card is removed from carry"),
		AfterDelete.CarriedInstanceIds.Contains(CurrentId));
	TestFalse(TEXT("The released current card is removed from storage"),
		Run->BuildBackpackStorageSnapshot().Flux.ContentCards.ContainsByPredicate(
			[CurrentId](const FRunStorageCardView& View)
			{
				return View.Instance.InstanceId == CurrentId;
			}));
	TestFalse(TEXT("Partial immediate delete does not suspend carry input"),
		AfterDelete.bCarryInputSuspended);
	TestFalse(TEXT("Partial immediate delete never creates confirmation"),
		FWacomBackpackScreenTestAccess::HasRuntimeDeleteConfirmationWidget(*Screen));
	return true;
}

#endif
