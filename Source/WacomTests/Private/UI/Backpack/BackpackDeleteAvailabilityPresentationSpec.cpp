// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Fixtures/WacomRunExplorationFixture.h"

#if WITH_AUTOMATION_TESTS

#include "../BackpackScreenTestAccess.h"
#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "InputCoreTypes.h"
#include "RunSession.h"
#include "RunState.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
struct FDeleteAvailabilityUiFixture
{
	TStrongObjectPtr<URunSession> Run;
	TStrongObjectPtr<UWacomBackpackScreen> Screen;
	FGuid ProviderId;
	TArray<FGuid> NormalIds;
};

TUniquePtr<FDeleteAvailabilityUiFixture> BuildDeleteAvailabilityUiFixture(
	UObject* Outer,
	bool bIncludeProvider,
	int32 NormalCardCount = 1)
{
	UCardDefinition* Bag = NewObject<UCardDefinition>(Outer);
	Bag->CardId = TEXT("DeleteAvailabilityUI.Bag");
	Bag->Physique.Capacity = NormalCardCount + 4;

	UCardDefinition* Provider = nullptr;
	if (bIncludeProvider)
	{
		Provider = NewObject<UCardDefinition>(Outer);
		Provider->CardId = TEXT("DeleteAvailabilityUI.Provider");
		Provider->Rarity = WacomTags::Card_Rarity_White;
		Provider->Keywords.AddTag(
			WacomTags::Card_Keyword_DeleteProvider);
	}

	UCardDefinition* Normal = NewObject<UCardDefinition>(Outer);
	Normal->CardId = TEXT("DeleteAvailabilityUI.Normal");
	Normal->Rarity = WacomTags::Card_Rarity_White;

	UCharacterDefinition* Character =
		NewObject<UCharacterDefinition>(Outer);
	Character->CharacterId = TEXT("DeleteAvailabilityUI.Character");
	Character->StarterDeck.Add(Bag);
	if (Provider)
	{
		Character->StarterDeck.Add(Provider);
	}

	TUniquePtr<FDeleteAvailabilityUiFixture> Fixture =
		MakeUnique<FDeleteAvailabilityUiFixture>();
	Fixture->Run.Reset(NewObject<URunSession>(Outer));
	if (!InitializeRunSessionForTest(
			*Fixture->Run,
			Character).IsOk())
	{
		return nullptr;
	}

	if (Provider)
	{
		const FCardInstance* ProviderInstance =
			Fixture->Run->GetRunState().BattleDeck.FindByPredicate(
				[Provider](const FCardInstance& Instance)
				{
					return Instance.Definition == Provider;
				});
		if (!ProviderInstance)
		{
			return nullptr;
		}
		Fixture->ProviderId = ProviderInstance->InstanceId;
		if (!Fixture->Run->MoveInstance(
				Fixture->ProviderId,
				EZoneKind::Backpack,
				FGuid()))
		{
			return nullptr;
		}
	}

	for (int32 Index = 0; Index < NormalCardCount; ++Index)
	{
		Fixture->Run->AcquireCardToRun(Normal);
	}
	const FRunBackpackStorageSnapshot Snapshot =
		Fixture->Run->BuildBackpackStorageSnapshot();
	for (const FRunStorageCardView& Card : Snapshot.Flux.ContentCards)
	{
		if (Card.Instance.Definition == Normal)
		{
			Fixture->NormalIds.Add(Card.Instance.InstanceId);
		}
	}
	if (Fixture->NormalIds.Num() != NormalCardCount)
	{
		return nullptr;
	}
	Fixture->Screen.Reset(
		FWacomBackpackScreenTestAccess::Create(
			Outer,
			Fixture->Run.Get()));
	return Fixture;
}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackDeleteTargetUnavailablePresentationSpec,
	"Wacom.UI.Backpack.DeleteAvailability.LockedTargetPresentation",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackDeleteTargetUnavailablePresentationSpec::RunTest(
	const FString& Parameters)
{
	TUniquePtr<FDeleteAvailabilityUiFixture> Fixture =
		BuildDeleteAvailabilityUiFixture(
			GetTransientPackage(),
			false);
	if (!TestNotNull(TEXT("Unavailable fixture initializes"), Fixture.Get())
		|| !TestNotNull(
			TEXT("Unavailable Screen exists"),
			Fixture ? Fixture->Screen.Get() : nullptr))
	{
		return false;
	}

	TestFalse(
		TEXT("Authoritative Snapshot reports delete unavailable"),
		Fixture->Run->BuildBackpackStorageSnapshot()
			.bDeleteFunctionAvailable);
	TestTrue(
		TEXT("Locked delete target remains visible"),
		FWacomBackpackScreenTestAccess::IsDeleteTargetVisible(
			*Fixture->Screen));
	TestTrue(
		TEXT("Locked delete target uses rejected presentation"),
		FWacomBackpackScreenTestAccess::IsDeleteTargetRejected(
			*Fixture->Screen));
	TestEqual(
		TEXT("Locked delete target explains its prerequisite"),
		FWacomBackpackScreenTestAccess::DeleteTargetSecondaryText(
			*Fixture->Screen).ToString(),
		FString(TEXT("需要删牌能力卡")));

	TestTrue(
		TEXT("A normal card can still enter Carry"),
		FWacomBackpackScreenTestAccess::BeginWorkspaceCarryForIds(
			*Fixture->Screen,
			Fixture->NormalIds));
	TestTrue(
		TEXT("Locked delete target remains a semantic focus target"),
		FWacomBackpackScreenTestAccess::FocusWorkspaceDeleteTarget(
			*Fixture->Screen));
	TestTrue(
		TEXT("Focused locked target rejects the carried card"),
		FWacomBackpackScreenTestAccess::IsWorkspaceCarryDropRejected(
			*Fixture->Screen));

	const FWacomBackpackWorkspaceAutomationTestView Before =
		FWacomBackpackScreenTestAccess::WorkspaceView(*Fixture->Screen);
	FWacomBackpackScreenTestAccess::SubmitWorkspaceDelete(
		*Fixture->Screen,
		Fixture->NormalIds);
	const FWacomBackpackWorkspaceAutomationTestView After =
		FWacomBackpackScreenTestAccess::WorkspaceView(*Fixture->Screen);
	TestEqual(
		TEXT("Rejected delete preserves the exact Carry"),
		After.CarriedInstanceIds,
		Before.CarriedInstanceIds);
	TestEqual(
		TEXT("Rejected delete preserves selection"),
		After.SelectedInstanceIds,
		Before.SelectedInstanceIds);
	TestEqual(
		TEXT("Rejected delete starts no departure"),
		After.SaleDepartureQueuedCardCount
			+ After.SaleDepartureActiveCardCount,
		0);
	TestTrue(
		TEXT("Rejected card remains in storage"),
		Fixture->Run->BuildBackpackStorageSnapshot()
			.Flux.ContentCards.ContainsByPredicate(
				[Id = Fixture->NormalIds[0]](
					const FRunStorageCardView& Card)
				{
					return Card.Instance.InstanceId == Id;
				}));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackLastDeleteProviderCarrySpec,
	"Wacom.UI.Backpack.DeleteAvailability.LastProviderCarry",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackLastDeleteProviderCarrySpec::RunTest(
	const FString& Parameters)
{
	TUniquePtr<FDeleteAvailabilityUiFixture> Fixture =
		BuildDeleteAvailabilityUiFixture(
			GetTransientPackage(),
			true);
	if (!TestNotNull(TEXT("Provider fixture initializes"), Fixture.Get())
		|| !TestNotNull(
			TEXT("Provider Screen exists"),
			Fixture ? Fixture->Screen.Get() : nullptr))
	{
		return false;
	}

	const TArray<FGuid> CarryIds{
		Fixture->ProviderId,
		Fixture->NormalIds[0]
	};
	TestTrue(
		TEXT("Provider and normal card enter one Carry"),
		FWacomBackpackScreenTestAccess::BeginWorkspaceCarryForIds(
			*Fixture->Screen,
			CarryIds));
	TestTrue(
		TEXT("Provider can become the primary single-release card"),
		FWacomBackpackScreenTestAccess::SelectWorkspaceCarryInstance(
			*Fixture->Screen,
			Fixture->ProviderId));
	TestTrue(
		TEXT("Delete target remains focusable while available"),
		FWacomBackpackScreenTestAccess::FocusWorkspaceDeleteTarget(
			*Fixture->Screen));
	TestFalse(
		TEXT("Primary single-provider release is presented as valid"),
		FWacomBackpackScreenTestAccess::IsDeleteTargetRejected(
			*Fixture->Screen));

	const FWacomBackpackWorkspaceAutomationTestView BeforeBatch =
		FWacomBackpackScreenTestAccess::WorkspaceView(*Fixture->Screen);
	FWacomBackpackScreenTestAccess::SubmitWorkspaceDelete(
		*Fixture->Screen,
		CarryIds);
	const FWacomBackpackWorkspaceAutomationTestView AfterBatch =
		FWacomBackpackScreenTestAccess::WorkspaceView(*Fixture->Screen);
	TestEqual(
		TEXT("Last-provider batch rejection preserves Carry"),
		AfterBatch.CarriedInstanceIds,
		BeforeBatch.CarriedInstanceIds);
	TestEqual(
		TEXT("Last-provider batch rejection preserves selection"),
		AfterBatch.SelectedInstanceIds,
		BeforeBatch.SelectedInstanceIds);
	TestEqual(
		TEXT("Last-provider batch rejection creates no ghost"),
		AfterBatch.SaleDepartureQueuedCardCount
			+ AfterBatch.SaleDepartureActiveCardCount,
		0);
	TestEqual(
		TEXT("Batch rejection reports the canonical reason"),
		FWacomBackpackScreenTestAccess::DeleteTargetLabelText(
			*Fixture->Screen).ToString(),
		FString(
			TEXT("无法销毁：最后一张删牌能力卡只能单独出售。")));

	FWacomBackpackScreenTestAccess::SubmitWorkspaceDelete(
		*Fixture->Screen,
		TArray<FGuid>{ Fixture->ProviderId });
	const FWacomBackpackWorkspaceAutomationTestView AfterSingle =
		FWacomBackpackScreenTestAccess::WorkspaceView(*Fixture->Screen);
	TestEqual(
		TEXT("Only the unreleased normal card remains carried"),
		AfterSingle.CarriedInstanceIds,
		Fixture->NormalIds);
	TestEqual(
		TEXT("Selection follows the remaining Carry"),
		AfterSingle.SelectedInstanceIds,
		Fixture->NormalIds);
	TestFalse(
		TEXT("Selling the last provider locks the authoritative Snapshot"),
		Fixture->Run->BuildBackpackStorageSnapshot()
			.bDeleteFunctionAvailable);
	TestTrue(
		TEXT("Remaining Carry immediately receives rejected feedback"),
		FWacomBackpackScreenTestAccess::IsWorkspaceCarryDropRejected(
			*Fixture->Screen));
	TestTrue(
		TEXT("Delete target immediately switches to rejected"),
		FWacomBackpackScreenTestAccess::IsDeleteTargetRejected(
			*Fixture->Screen));
	TestEqual(
		TEXT("The sold provider enters one material departure"),
		AfterSingle.SaleDepartureQueuedCardCount
			+ AfterSingle.SaleDepartureActiveCardCount,
		1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackDeleteTargetInputParitySpec,
	"Wacom.UI.Backpack.DeleteAvailability.InputParity",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackDeleteTargetInputParitySpec::RunTest(
	const FString& Parameters)
{
	for (const FKey& ReleaseKey :
		{ EKeys::Enter, EKeys::Gamepad_FaceButton_Bottom })
	{
		TUniquePtr<FDeleteAvailabilityUiFixture> Fixture =
			BuildDeleteAvailabilityUiFixture(
				GetTransientPackage(),
				false);
		if (!TestNotNull(
				TEXT("Input parity fixture initializes"),
				Fixture.Get())
			|| !Fixture->Screen)
		{
			continue;
		}
		TestTrue(
			TEXT("Input parity card enters Carry"),
			FWacomBackpackScreenTestAccess::BeginWorkspaceCarryForIds(
				*Fixture->Screen,
				Fixture->NormalIds));
		TestTrue(
			TEXT("Input parity focuses Delete"),
			FWacomBackpackScreenTestAccess::FocusWorkspaceDeleteTarget(
				*Fixture->Screen));
		TestTrue(
			TEXT("Initial guarded release is handled"),
			FWacomBackpackScreenTestAccess::SendWorkspaceScreenKeyDown(
				*Fixture->Screen,
				ReleaseKey));
		TestTrue(
			TEXT("The next keyboard or gamepad release is handled"),
			FWacomBackpackScreenTestAccess::SendWorkspaceScreenKeyDown(
				*Fixture->Screen,
				ReleaseKey));
		const FWacomBackpackWorkspaceAutomationTestView View =
			FWacomBackpackScreenTestAccess::WorkspaceView(
				*Fixture->Screen);
		TestEqual(
			TEXT("Keyboard and gamepad rejection preserve Carry"),
			View.CarriedInstanceIds,
			Fixture->NormalIds);
		TestTrue(
			TEXT("Keyboard and gamepad rejection use the same feedback"),
			FWacomBackpackScreenTestAccess::
				IsWorkspaceCarryDropRejected(*Fixture->Screen));
	}
	return true;
}

#endif
