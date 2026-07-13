// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "RunSession.h"
#include "Tags/WacomGameplayTags.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
UCardDefinition* MakeBatchDeleteCard(
	UObject* Outer,
	FName Id,
	FGameplayTag Rarity = FGameplayTag(),
	int32 Capacity = 0)
{
	UCardDefinition* Card = NewObject<UCardDefinition>(Outer);
	Card->CardId = Id;
	Card->DisplayName = FText::FromName(Id);
	Card->BaseCost = 1;
	Card->Rarity = Rarity;
	Card->Physique.Capacity = Capacity;
	return Card;
}

URunSession* MakeBatchDeleteRun(UObject* Outer, const TArray<UCardDefinition*>& Starter)
{
	UCharacterDefinition* Character = NewObject<UCharacterDefinition>(Outer);
	Character->CharacterId = TEXT("Backpack.BatchDelete.Character");
	Character->StarterDeck = Starter;
	URunSession* Run = NewObject<URunSession>(Outer);
	return Run->Initialize(Character) ? Run : nullptr;
}

FGuid FindDeleteCardId(const FRunState& State, const UCardDefinition* Definition, EZoneKind* OutZone = nullptr)
{
	auto FindIn = [Definition](const TArray<FCardInstance>& Cards)
	{
		const FCardInstance* Found = Cards.FindByPredicate(
			[Definition](const FCardInstance& Card) { return Card.Definition == Definition; });
		return Found ? Found->InstanceId : FGuid();
	};
	if (const FGuid Id = FindIn(State.Backpack); Id.IsValid()) { if (OutZone) *OutZone = EZoneKind::Backpack; return Id; }
	if (const FGuid Id = FindIn(State.BattleDeck); Id.IsValid()) { if (OutZone) *OutZone = EZoneKind::BattleDeck; return Id; }
	if (const FGuid Id = FindIn(State.BurdenZone); Id.IsValid()) { if (OutZone) *OutZone = EZoneKind::BurdenZone; return Id; }
	for (const FSpecialZone& Special : State.SpecialZones)
	{
		if (const FGuid Id = FindIn(Special.Cards); Id.IsValid()) { if (OutZone) *OutZone = EZoneKind::SpecialZone; return Id; }
	}
	return FGuid();
}

FRunDeckBatchDeleteRequest MakeDeleteRequest(
	URunSession& Run,
	TArray<FGuid> Ids,
	EZoneKind Source,
	FGuid SourceOwner = FGuid())
{
	FRunDeckBatchDeleteRequest Request;
	Request.InstanceIds = MoveTemp(Ids);
	Request.ExpectedSource = { Source, SourceOwner };
	Request.ExpectedStorageRevision = Run.GetBackpackStorageSnapshotRevision();
	return Request;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunBackpackBatchDeleteAtomicSpec,
	"Wacom.Run.Backpack.BatchDelete.AtomicContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunBackpackBatchDeleteAtomicSpec::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	UCardDefinition* Bag = MakeBatchDeleteCard(Outer, TEXT("BatchDelete.Bag"), FGameplayTag(), 5);
	UCardDefinition* White = MakeBatchDeleteCard(Outer, TEXT("BatchDelete.White"), WacomTags::Card_Rarity_White);
	UCardDefinition* Blue = MakeBatchDeleteCard(Outer, TEXT("BatchDelete.Blue"), WacomTags::Card_Rarity_Blue);
	TStrongObjectPtr<URunSession> Run(MakeBatchDeleteRun(Outer, { Bag }));
	Run->AcquireCardToRun(White);
	Run->AcquireCardToRun(Blue);
	const FGuid WhiteId = FindDeleteCardId(Run->GetRunState(), White);
	const FGuid BlueId = FindDeleteCardId(Run->GetRunState(), Blue);
	FRunDeckBatchDeleteRequest Request = MakeDeleteRequest(
		*Run, { WhiteId, BlueId }, EZoneKind::Backpack);
	const FRunDeckBatchDeletePreview Preview = Run->ValidateDeleteCardsForGoldAtomic(Request);
	TestTrue(TEXT("Batch delete preview succeeds"), Preview.Validation.bCanExecute);
	TestEqual(TEXT("Preview reports summed white plus blue reward"), Preview.TotalGoldReward, 3);
	const int32 GoldBefore = Run->GetGold();
	const uint64 RevisionBefore = Run->GetBackpackStorageSnapshotRevision();
	int32 BroadcastCount = 0;
	Run->OnRunStateChangedNative.AddLambda([&BroadcastCount]() { ++BroadcastCount; });
	const FRunDeckBatchOperationResult Success = Run->DeleteCardsForGoldAtomic(Request);
	TestTrue(TEXT("Batch delete succeeds"), Success.bSucceeded);
	TestEqual(TEXT("Batch delete removes every requested card"), Success.AffectedCount, 2);
	TestEqual(TEXT("Batch delete grants exact reward once"), Run->GetGold(), GoldBefore + 3);
	TestEqual(TEXT("Batch delete broadcasts exactly once"), BroadcastCount, 1);
	TestEqual(TEXT("Batch delete advances storage revision once"),
		Run->GetBackpackStorageSnapshotRevision(), RevisionBefore + 1);
	TestFalse(TEXT("White card is gone"), FindDeleteCardId(Run->GetRunState(), White).IsValid());
	TestFalse(TEXT("Blue card is gone"), FindDeleteCardId(Run->GetRunState(), Blue).IsValid());

	UCardDefinition* RollbackBag = MakeBatchDeleteCard(Outer, TEXT("BatchDelete.RollbackBag"), FGameplayTag(), 5);
	UCardDefinition* Valid = MakeBatchDeleteCard(Outer, TEXT("BatchDelete.Valid"), WacomTags::Card_Rarity_White);
	UCardDefinition* Intrinsic = MakeBatchDeleteCard(Outer, TEXT("BatchDelete.Intrinsic"), WacomTags::Card_Rarity_Intrinsic);
	TStrongObjectPtr<URunSession> RollbackRun(MakeBatchDeleteRun(Outer, { RollbackBag }));
	RollbackRun->AcquireCardToRun(Valid);
	RollbackRun->AcquireCardToRun(Intrinsic);
	const FGuid ValidId = FindDeleteCardId(RollbackRun->GetRunState(), Valid);
	const FGuid IntrinsicId = FindDeleteCardId(RollbackRun->GetRunState(), Intrinsic);
	const int32 RollbackGold = RollbackRun->GetGold();
	const uint64 RollbackRevision = RollbackRun->GetBackpackStorageSnapshotRevision();
	int32 RollbackBroadcasts = 0;
	RollbackRun->OnRunStateChangedNative.AddLambda([&RollbackBroadcasts]() { ++RollbackBroadcasts; });
	const FRunDeckBatchOperationResult InvalidLast = RollbackRun->DeleteCardsForGoldAtomic(
		MakeDeleteRequest(*RollbackRun, { ValidId, IntrinsicId }, EZoneKind::Backpack));
	TestFalse(TEXT("Invalid last delete item rejects whole transaction"), InvalidLast.bSucceeded);
	TestTrue(TEXT("Valid first item remains after rollback"), FindDeleteCardId(RollbackRun->GetRunState(), Valid).IsValid());
	TestTrue(TEXT("Invalid item remains after rollback"), FindDeleteCardId(RollbackRun->GetRunState(), Intrinsic).IsValid());
	TestEqual(TEXT("Rollback grants zero gold"), RollbackRun->GetGold(), RollbackGold);
	TestEqual(TEXT("Rollback preserves revision"), RollbackRun->GetBackpackStorageSnapshotRevision(), RollbackRevision);
	TestEqual(TEXT("Rollback broadcasts zero times"), RollbackBroadcasts, 0);

	for (int32 Repetition = 0; Repetition < 50; ++Repetition)
	{
		const FRunDeckBatchOperationResult Rejected = RollbackRun->DeleteCardsForGoldAtomic(
			MakeDeleteRequest(*RollbackRun, { ValidId, ValidId }, EZoneKind::Backpack));
		TestFalse(TEXT("Duplicate delete request is rejected"), Rejected.bSucceeded);
	}
	TestEqual(TEXT("Fifty rejected deletes grant zero gold"), RollbackRun->GetGold(), RollbackGold);
	TestEqual(TEXT("Fifty rejected deletes keep revision"), RollbackRun->GetBackpackStorageSnapshotRevision(), RollbackRevision);

	FRunDeckBatchDeleteRequest StaleRevision = MakeDeleteRequest(
		*RollbackRun, { ValidId }, EZoneKind::Backpack);
	StaleRevision.ExpectedStorageRevision--;
	TestFalse(TEXT("Stale delete revision is rejected"), RollbackRun->DeleteCardsForGoldAtomic(StaleRevision).bSucceeded);
	FRunDeckBatchDeleteRequest StaleSource = MakeDeleteRequest(
		*RollbackRun, { ValidId }, EZoneKind::BattleDeck);
	TestFalse(TEXT("Stale delete source is rejected"), RollbackRun->DeleteCardsForGoldAtomic(StaleSource).bSucceeded);

	UCardDefinition* CapacityA = MakeBatchDeleteCard(Outer, TEXT("BatchDelete.CapacityA"), FGameplayTag(), 1);
	UCardDefinition* CapacityB = MakeBatchDeleteCard(Outer, TEXT("BatchDelete.CapacityB"), FGameplayTag(), 1);
	UCardDefinition* LoadA = MakeBatchDeleteCard(Outer, TEXT("BatchDelete.LoadA"));
	UCardDefinition* LoadB = MakeBatchDeleteCard(Outer, TEXT("BatchDelete.LoadB"));
	TStrongObjectPtr<URunSession> BurdenRun(MakeBatchDeleteRun(Outer, { CapacityA, CapacityB }));
	BurdenRun->AcquireCardToRun(LoadA);
	BurdenRun->AcquireCardToRun(LoadB);
	EZoneKind CapacityZone = EZoneKind::Backpack;
	const FGuid CapacityAId = FindDeleteCardId(BurdenRun->GetRunState(), CapacityA, &CapacityZone);
	const FRunDeckBatchOperationResult CapacityDelete = BurdenRun->DeleteCardsForGoldAtomic(
		MakeDeleteRequest(*BurdenRun, { CapacityAId }, CapacityZone));
	TestTrue(TEXT("Deleting one of multiple capacity providers succeeds"), CapacityDelete.bSucceeded);
	TestTrue(TEXT("Capacity delete recomputes final burden"), BurdenRun->GetRunState().BurdenZone.Num() >= 1);
	return true;
}
