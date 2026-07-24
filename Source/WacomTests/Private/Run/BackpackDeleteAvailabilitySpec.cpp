// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Fixtures/WacomRunExplorationFixture.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "RunSession.h"
#include "Tags/WacomGameplayTags.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
UCardDefinition* MakeDeleteAvailabilityCard(
	UObject* Outer,
	FName CardId,
	int32 Capacity = 0,
	bool bDeleteProvider = false)
{
	UCardDefinition* Card = NewObject<UCardDefinition>(Outer);
	Card->CardId = CardId;
	Card->DisplayName = FText::FromName(CardId);
	Card->BaseCost = 1;
	Card->Rarity = WacomTags::Card_Rarity_White;
	Card->Physique.Capacity = Capacity;
	if (bDeleteProvider)
	{
		Card->Keywords.AddTag(WacomTags::Card_Keyword_DeleteProvider);
	}
	return Card;
}

UCardDefinition* MakeDeleteAvailabilitySpecialOwner(UObject* Outer)
{
	UCardDefinition* Card = MakeDeleteAvailabilityCard(
		Outer, TEXT("DeleteAvailability.SpecialOwner"), 4);
	Card->Physique.CapacityEffect =
		WacomTags::Card_CapacityEffect_Placeholder;
	return Card;
}

URunSession* MakeDeleteAvailabilityRun(
	UObject* Outer,
	const TArray<UCardDefinition*>& Starter)
{
	UCharacterDefinition* Character = NewObject<UCharacterDefinition>(Outer);
	Character->CharacterId = TEXT("DeleteAvailability.Character");
	Character->StarterDeck = Starter;
	URunSession* Run = NewObject<URunSession>(Outer);
	return InitializeRunSessionForTest(*Run, Character).IsOk() ? Run : nullptr;
}

bool FindDeleteAvailabilityCard(
	const FRunState& State,
	const UCardDefinition* Definition,
	FGuid& OutInstanceId,
	EZoneKind& OutZone,
	FGuid& OutZoneOwner)
{
	auto FindInPile = [&](const TArray<FCardInstance>& Pile, EZoneKind Zone, FGuid Owner)
	{
		const FCardInstance* Found = Pile.FindByPredicate(
			[Definition](const FCardInstance& Instance)
			{
				return Instance.Definition == Definition;
			});
		if (!Found)
		{
			return false;
		}
		OutInstanceId = Found->InstanceId;
		OutZone = Zone;
		OutZoneOwner = Owner;
		return true;
	};

	if (FindInPile(State.Backpack, EZoneKind::Backpack, FGuid())
		|| FindInPile(State.BattleDeck, EZoneKind::BattleDeck, FGuid())
		|| FindInPile(State.BurdenZone, EZoneKind::BurdenZone, FGuid()))
	{
		return true;
	}
	for (const FSpecialZone& SpecialZone : State.SpecialZones)
	{
		if (FindInPile(
			SpecialZone.Cards,
			EZoneKind::SpecialZone,
			SpecialZone.OwnerInstanceId))
		{
			return true;
		}
	}
	return false;
}

FRunDeckBatchDeleteRequest MakeDeleteAvailabilityRequest(
	URunSession& Run,
	TArray<FGuid> InstanceIds,
	EZoneKind SourceZone,
	FGuid SourceOwner = FGuid())
{
	FRunDeckBatchDeleteRequest Request;
	Request.InstanceIds = MoveTemp(InstanceIds);
	Request.ExpectedSource = { SourceZone, SourceOwner };
	Request.ExpectedStorageRevision =
		Run.GetBackpackStorageSnapshotRevision();
	return Request;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunBackpackDeleteProviderPhysicalZonesSpec,
	"Wacom.Run.Backpack.DeleteAvailability.PhysicalZones",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunBackpackDeleteProviderPhysicalZonesSpec::RunTest(
	const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	UCardDefinition* Bag = MakeDeleteAvailabilityCard(
		Outer, TEXT("DeleteAvailability.Bag"), 12);
	UCardDefinition* SpecialOwner = MakeDeleteAvailabilitySpecialOwner(Outer);
	UCardDefinition* Provider = MakeDeleteAvailabilityCard(
		Outer, TEXT("DeleteAvailability.Provider"), 0, true);
	TStrongObjectPtr<URunSession> Run(
		MakeDeleteAvailabilityRun(Outer, { Bag, SpecialOwner, Provider }));
	if (!TestNotNull(TEXT("Run initializes"), Run.Get()))
	{
		return false;
	}

	FGuid ProviderId;
	EZoneKind ProviderZone = EZoneKind::Backpack;
	FGuid ProviderOwner;
	TestTrue(TEXT("Provider starts in a physical zone"),
		FindDeleteAvailabilityCard(
			Run->GetRunState(), Provider, ProviderId, ProviderZone, ProviderOwner));
	TestEqual(TEXT("Non-container provider starts in BattleDeck"),
		ProviderZone, EZoneKind::BattleDeck);
	TestTrue(TEXT("BattleDeck provider enables delete"),
		Run->IsDeleteFunctionAvailable());
	TestTrue(TEXT("Snapshot exposes BattleDeck provider"),
		Run->BuildBackpackStorageSnapshot().bDeleteFunctionAvailable);

	TestTrue(TEXT("Provider moves to Backpack"),
		Run->MoveInstance(ProviderId, EZoneKind::Backpack, FGuid()));
	TestTrue(TEXT("Backpack provider enables delete"),
		Run->IsDeleteFunctionAvailable());

	TestTrue(TEXT("Provider moves to BurdenZone"),
		Run->MoveInstance(ProviderId, EZoneKind::BurdenZone, FGuid()));
	TestTrue(TEXT("Burden provider enables delete"),
		Run->IsDeleteFunctionAvailable());

	const FGuid SpecialOwnerId =
		Run->GetRunState().SpecialZones.IsEmpty()
			? FGuid()
			: Run->GetRunState().SpecialZones[0].OwnerInstanceId;
	if (!TestTrue(TEXT("Special owner exists"), SpecialOwnerId.IsValid()))
	{
		return false;
	}
	TestTrue(TEXT("Provider moves to SpecialZone"),
		Run->MoveInstance(
			ProviderId, EZoneKind::SpecialZone, SpecialOwnerId));
	TestTrue(TEXT("SpecialZone provider enables delete"),
		Run->IsDeleteFunctionAvailable());
	TestTrue(TEXT("Snapshot exposes SpecialZone provider"),
		Run->BuildBackpackStorageSnapshot().bDeleteFunctionAvailable);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunBackpackDeleteFunctionUnavailableSpec,
	"Wacom.Run.Backpack.DeleteAvailability.UnavailableAndDestroyIndependent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunBackpackDeleteFunctionUnavailableSpec::RunTest(
	const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	UCardDefinition* Bag = MakeDeleteAvailabilityCard(
		Outer, TEXT("DeleteAvailability.NoProviderBag"), 8);
	UCardDefinition* Target = MakeDeleteAvailabilityCard(
		Outer, TEXT("DeleteAvailability.NoProviderTarget"));
	TStrongObjectPtr<URunSession> Run(
		MakeDeleteAvailabilityRun(Outer, { Bag, Target }));
	if (!TestNotNull(TEXT("Run initializes"), Run.Get()))
	{
		return false;
	}

	FGuid TargetId;
	EZoneKind TargetZone = EZoneKind::Backpack;
	FGuid TargetOwner;
	TestTrue(TEXT("Target exists"),
		FindDeleteAvailabilityCard(
			Run->GetRunState(), Target, TargetId, TargetZone, TargetOwner));
	TestFalse(TEXT("Delete function starts unavailable"),
		Run->IsDeleteFunctionAvailable());
	TestFalse(TEXT("Snapshot reports unavailable"),
		Run->BuildBackpackStorageSnapshot().bDeleteFunctionAvailable);
	TestEqual(TEXT("Single preview reports provider requirement"),
		Run->ValidateDeleteCardForGoldByInstance(TargetId).DisabledReason,
		WacomRunDeckOperationReasons::DeleteFunctionUnavailable());

	const int32 GoldBefore = Run->GetGold();
	const uint64 RevisionBefore =
		Run->GetBackpackStorageSnapshotRevision();
	int32 BroadcastCount = 0;
	Run->OnRunStateChangedNative.AddLambda(
		[&BroadcastCount]() { ++BroadcastCount; });

	TestFalse(TEXT("Single delete is rejected"),
		Run->DeleteCardForGoldByInstance(TargetId));
	const FRunDeckBatchDeleteRequest Request =
		MakeDeleteAvailabilityRequest(
			*Run, { TargetId }, TargetZone, TargetOwner);
	const FRunDeckBatchDeletePreview Preview =
		Run->ValidateDeleteCardsForGoldAtomic(Request);
	TestEqual(TEXT("Batch preview reports provider requirement"),
		Preview.Validation.DisabledReason,
		WacomRunDeckOperationReasons::DeleteFunctionUnavailable());
	TestFalse(TEXT("Batch delete is rejected"),
		Run->DeleteCardsForGoldAtomic(Request).bSucceeded);
	TestEqual(TEXT("Rejected deletes grant no gold"),
		Run->GetGold(), GoldBefore);
	TestEqual(TEXT("Rejected deletes preserve storage revision"),
		Run->GetBackpackStorageSnapshotRevision(), RevisionBefore);
	TestEqual(TEXT("Rejected deletes do not broadcast"),
		BroadcastCount, 0);

	TestTrue(TEXT("Non-sale permanent destroy remains available"),
		Run->DestroyCardByInstance(TargetId));
	TestEqual(TEXT("Destroy still grants no gold"),
		Run->GetGold(), GoldBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunBackpackLastDeleteProviderTransactionSpec,
	"Wacom.Run.Backpack.DeleteAvailability.LastProviderTransaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunBackpackLastDeleteProviderTransactionSpec::RunTest(
	const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	auto MakeRunWithCards = [&](FName Prefix, int32 ProviderCount)
	{
		TArray<UCardDefinition*> Starter;
		Starter.Add(MakeDeleteAvailabilityCard(
			Outer, FName(*(Prefix.ToString() + TEXT(".Bag"))), 12));
		for (int32 Index = 0; Index < ProviderCount; ++Index)
		{
			Starter.Add(MakeDeleteAvailabilityCard(
				Outer,
				FName(*FString::Printf(
					TEXT("%s.Provider%d"), *Prefix.ToString(), Index)),
				0,
				true));
		}
		Starter.Add(MakeDeleteAvailabilityCard(
			Outer, FName(*(Prefix.ToString() + TEXT(".Normal")))));
		return TStrongObjectPtr<URunSession>(
			MakeDeleteAvailabilityRun(Outer, Starter));
	};

	TStrongObjectPtr<URunSession> BatchRun =
		MakeRunWithCards(TEXT("DeleteAvailability.BatchLast"), 1);
	const UCardDefinition* BatchProvider =
		BatchRun->GetRunState().BattleDeck[0].Definition;
	const UCardDefinition* BatchNormal =
		BatchRun->GetRunState().BattleDeck[1].Definition;
	FGuid BatchProviderId;
	FGuid BatchNormalId;
	EZoneKind BatchProviderZone = EZoneKind::Backpack;
	EZoneKind BatchNormalZone = EZoneKind::Backpack;
	FGuid BatchProviderOwner;
	FGuid BatchNormalOwner;
	TestTrue(TEXT("Batch provider exists"),
		FindDeleteAvailabilityCard(
			BatchRun->GetRunState(),
			BatchProvider,
			BatchProviderId,
			BatchProviderZone,
			BatchProviderOwner));
	TestTrue(TEXT("Batch normal exists"),
		FindDeleteAvailabilityCard(
			BatchRun->GetRunState(),
			BatchNormal,
			BatchNormalId,
			BatchNormalZone,
			BatchNormalOwner));

	const uint64 BatchRevision =
		BatchRun->GetBackpackStorageSnapshotRevision();
	const int32 BatchGold = BatchRun->GetGold();
	for (const TArray<FGuid>& Order :
		{ TArray<FGuid>{ BatchProviderId, BatchNormalId },
		  TArray<FGuid>{ BatchNormalId, BatchProviderId } })
	{
		const FRunDeckBatchDeleteRequest Request =
			MakeDeleteAvailabilityRequest(
				*BatchRun, Order, EZoneKind::BattleDeck);
		const FRunDeckBatchDeletePreview Preview =
			BatchRun->ValidateDeleteCardsForGoldAtomic(Request);
		TestEqual(TEXT("Clearing provider in a multi-card batch is rejected"),
			Preview.Validation.DisabledReason,
			WacomRunDeckOperationReasons::
				LastDeleteProviderRequiresSingleCard());
		TestFalse(TEXT("Rejected multi-card batch does not commit"),
			BatchRun->DeleteCardsForGoldAtomic(Request).bSucceeded);
	}
	TestEqual(TEXT("Rejected orders preserve revision"),
		BatchRun->GetBackpackStorageSnapshotRevision(), BatchRevision);
	TestEqual(TEXT("Rejected orders preserve gold"),
		BatchRun->GetGold(), BatchGold);

	TStrongObjectPtr<URunSession> SingleRun =
		MakeRunWithCards(TEXT("DeleteAvailability.SingleLast"), 1);
	const UCardDefinition* SingleProvider =
		SingleRun->GetRunState().BattleDeck[0].Definition;
	FGuid SingleProviderId;
	EZoneKind SingleProviderZone = EZoneKind::Backpack;
	FGuid SingleProviderOwner;
	TestTrue(TEXT("Single provider exists"),
		FindDeleteAvailabilityCard(
			SingleRun->GetRunState(),
			SingleProvider,
			SingleProviderId,
			SingleProviderZone,
			SingleProviderOwner));
	const FRunDeckBatchOperationResult SingleResult =
		SingleRun->DeleteCardsForGoldAtomic(
			MakeDeleteAvailabilityRequest(
				*SingleRun,
				{ SingleProviderId },
				SingleProviderZone,
				SingleProviderOwner));
	TestTrue(TEXT("Last provider may be sold alone"),
		SingleResult.bSucceeded);
	TestFalse(TEXT("Selling the last provider disables delete"),
		SingleRun->IsDeleteFunctionAvailable());
	TestFalse(TEXT("Snapshot locks delete after the sale"),
		SingleRun->BuildBackpackStorageSnapshot()
			.bDeleteFunctionAvailable);

	TStrongObjectPtr<URunSession> TwoProviderRun =
		MakeRunWithCards(TEXT("DeleteAvailability.TwoProviders"), 2);
	const TArray<FCardInstance>& TwoProviderDeck =
		TwoProviderRun->GetRunState().BattleDeck;
	const FGuid FirstProviderId = TwoProviderDeck[0].InstanceId;
	const FGuid SecondProviderId = TwoProviderDeck[1].InstanceId;
	const FGuid TwoProviderNormalId = TwoProviderDeck[2].InstanceId;
	TestTrue(TEXT("One provider and a normal may be sold while another remains"),
		TwoProviderRun->DeleteCardsForGoldAtomic(
			MakeDeleteAvailabilityRequest(
				*TwoProviderRun,
				{ FirstProviderId, TwoProviderNormalId },
				EZoneKind::BattleDeck)).bSucceeded);
	TestTrue(TEXT("Second provider keeps delete available"),
		TwoProviderRun->IsDeleteFunctionAvailable());

	TStrongObjectPtr<URunSession> ClearTwoRun =
		MakeRunWithCards(TEXT("DeleteAvailability.ClearTwo"), 2);
	const TArray<FCardInstance>& ClearTwoDeck =
		ClearTwoRun->GetRunState().BattleDeck;
	const FRunDeckBatchDeletePreview ClearTwoPreview =
		ClearTwoRun->ValidateDeleteCardsForGoldAtomic(
			MakeDeleteAvailabilityRequest(
				*ClearTwoRun,
				{ ClearTwoDeck[0].InstanceId, ClearTwoDeck[1].InstanceId },
				EZoneKind::BattleDeck));
	TestEqual(TEXT("Two providers cannot be sold together if none remains"),
		ClearTwoPreview.Validation.DisabledReason,
		WacomRunDeckOperationReasons::
			LastDeleteProviderRequiresSingleCard());
	return true;
}
