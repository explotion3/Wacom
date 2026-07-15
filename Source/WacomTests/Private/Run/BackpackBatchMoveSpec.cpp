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
UCardDefinition* MakeBatchMoveCard(UObject* Outer, FName Id, int32 Capacity = 0, bool bTypeB = false)
{
	UCardDefinition* Card = NewObject<UCardDefinition>(Outer);
	Card->CardId = Id;
	Card->DisplayName = FText::FromName(Id);
	Card->BaseCost = 1;
	Card->Physique.Capacity = Capacity;
	if (bTypeB)
	{
		Card->Physique.CapacityEffect = WacomTags::Card_CapacityEffect_WeaponDamagePlus3;
	}
	return Card;
}

URunSession* MakeBatchMoveRun(UObject* Outer, UCardDefinition* CapacityCard)
{
	UCharacterDefinition* Character = NewObject<UCharacterDefinition>(Outer);
	Character->CharacterId = TEXT("Backpack.BatchMove.Character");
	Character->StarterDeck.Add(CapacityCard);
	URunSession* Run = NewObject<URunSession>(Outer);
	return InitializeRunSessionForTest(*Run, Character).IsOk() ? Run : nullptr;
}

FGuid FindCardId(const FRunState& State, const UCardDefinition* Definition, EZoneKind Zone)
{
	auto FindIn = [Definition](const TArray<FCardInstance>& Cards)
	{
		const FCardInstance* Found = Cards.FindByPredicate(
			[Definition](const FCardInstance& Card) { return Card.Definition == Definition; });
		return Found ? Found->InstanceId : FGuid();
	};
	switch (Zone)
	{
	case EZoneKind::Backpack: return FindIn(State.Backpack);
	case EZoneKind::BattleDeck: return FindIn(State.BattleDeck);
	case EZoneKind::BurdenZone: return FindIn(State.BurdenZone);
	case EZoneKind::SpecialZone:
		for (const FSpecialZone& Special : State.SpecialZones)
		{
			if (const FGuid Id = FindIn(Special.Cards); Id.IsValid()) return Id;
		}
		return FGuid();
	default: return FGuid();
	}
}

FRunDeckBatchMoveRequest MakeMoveRequest(
	URunSession& Run,
	TArray<FGuid> Ids,
	EZoneKind Source,
	EZoneKind Target,
	FGuid SourceOwner = FGuid(),
	FGuid TargetOwner = FGuid())
{
	FRunDeckBatchMoveRequest Request;
	Request.InstanceIds = MoveTemp(Ids);
	Request.ExpectedSource = { Source, SourceOwner };
	Request.Target = { Target, TargetOwner };
	Request.ExpectedStorageRevision = Run.GetBackpackStorageSnapshotRevision();
	return Request;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunBackpackBatchMoveAtomicSpec,
	"Wacom.Run.Backpack.BatchMove.AtomicContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunBackpackBatchMoveAtomicSpec::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	UCardDefinition* Bag = MakeBatchMoveCard(Outer, TEXT("BatchMove.Bag"), 4);
	UCardDefinition* First = MakeBatchMoveCard(Outer, TEXT("BatchMove.First"));
	UCardDefinition* Second = MakeBatchMoveCard(Outer, TEXT("BatchMove.Second"));
	TStrongObjectPtr<URunSession> Run(MakeBatchMoveRun(Outer, Bag));
	TestNotNull(TEXT("Batch move Run initializes"), Run.Get());
	Run->AcquireCardToRun(First);
	Run->AcquireCardToRun(Second);
	const FGuid FirstId = FindCardId(Run->GetRunState(), First, EZoneKind::Backpack);
	const FGuid SecondId = FindCardId(Run->GetRunState(), Second, EZoneKind::Backpack);
	TestTrue(TEXT("Batch source ids exist"), FirstId.IsValid() && SecondId.IsValid());

	int32 BroadcastCount = 0;
	Run->OnRunStateChangedNative.AddLambda([&BroadcastCount]() { ++BroadcastCount; });
	const uint64 BeforeRevision = Run->GetBackpackStorageSnapshotRevision();
	const FRunDeckBatchOperationResult Success = Run->MoveInstancesAtomic(
		MakeMoveRequest(*Run, { FirstId, SecondId }, EZoneKind::Backpack, EZoneKind::BattleDeck));
	TestTrue(TEXT("Two-card move succeeds atomically"), Success.bSucceeded);
	TestEqual(TEXT("Success reports every affected card"), Success.AffectedCount, 2);
	TestEqual(TEXT("Success broadcasts exactly once"), BroadcastCount, 1);
	TestEqual(TEXT("Success advances storage revision exactly once"),
		Run->GetBackpackStorageSnapshotRevision(), BeforeRevision + 1);
	TestTrue(TEXT("First moved to target"), FindCardId(Run->GetRunState(), First, EZoneKind::BattleDeck).IsValid());
	TestTrue(TEXT("Second moved to target"), FindCardId(Run->GetRunState(), Second, EZoneKind::BattleDeck).IsValid());

	UCardDefinition* SmallBag = MakeBatchMoveCard(Outer, TEXT("BatchMove.SmallBag"), 4);
	UCardDefinition* CapacityOwner = MakeBatchMoveCard(Outer, TEXT("BatchMove.CapacityOwner"), 2, true);
	UCardDefinition* CapacityFirst = MakeBatchMoveCard(Outer, TEXT("BatchMove.CapacityFirst"));
	UCardDefinition* CapacityLast = MakeBatchMoveCard(Outer, TEXT("BatchMove.CapacityLast"));
	TStrongObjectPtr<URunSession> CapacityRun(MakeBatchMoveRun(Outer, SmallBag));
	CapacityRun->AcquireCardToRun(CapacityOwner);
	CapacityRun->AcquireCardToRun(CapacityFirst);
	CapacityRun->AcquireCardToRun(CapacityLast);
	const FGuid CapacityOwnerId = FindCardId(CapacityRun->GetRunState(), CapacityOwner, EZoneKind::Backpack);
	const FGuid CapacityFirstId = FindCardId(CapacityRun->GetRunState(), CapacityFirst, EZoneKind::Backpack);
	const FGuid CapacityLastId = FindCardId(CapacityRun->GetRunState(), CapacityLast, EZoneKind::Backpack);
	const uint64 CapacityRevision = CapacityRun->GetBackpackStorageSnapshotRevision();
	int32 FailedBroadcasts = 0;
	CapacityRun->OnRunStateChangedNative.AddLambda([&FailedBroadcasts]() { ++FailedBroadcasts; });
	const FRunDeckBatchOperationResult CapacityFailure = CapacityRun->MoveInstancesAtomic(
		MakeMoveRequest(
			*CapacityRun,
			{ CapacityFirstId, CapacityLastId },
			EZoneKind::Backpack,
			EZoneKind::SpecialZone,
			FGuid(),
			CapacityOwnerId));
	TestFalse(TEXT("Capacity failure on last card rejects whole move"), CapacityFailure.bSucceeded);
	TestTrue(TEXT("Capacity failure moves first card zero times"),
		FindCardId(CapacityRun->GetRunState(), CapacityFirst, EZoneKind::Backpack).IsValid());
	TestTrue(TEXT("Capacity failure leaves last card in source"),
		FindCardId(CapacityRun->GetRunState(), CapacityLast, EZoneKind::Backpack).IsValid());
	TestEqual(TEXT("Rejected move keeps revision"), CapacityRun->GetBackpackStorageSnapshotRevision(), CapacityRevision);
	TestEqual(TEXT("Rejected move broadcasts zero times"), FailedBroadcasts, 0);

	for (int32 Repetition = 0; Repetition < 50; ++Repetition)
	{
		FRunDeckBatchMoveRequest Duplicate = MakeMoveRequest(
			*CapacityRun,
			{ CapacityFirstId, CapacityFirstId },
			EZoneKind::Backpack,
			EZoneKind::SpecialZone,
			FGuid(),
			CapacityOwnerId);
		const FRunDeckBatchOperationResult Rejected = CapacityRun->MoveInstancesAtomic(Duplicate);
		TestFalse(TEXT("Duplicate batch is rejected"), Rejected.bSucceeded);
	}
	TestEqual(TEXT("Fifty rejected transactions keep source intact"),
		CapacityRun->GetBackpackStorageSnapshotRevision(), CapacityRevision);
	TestEqual(TEXT("Fifty rejected transactions emit zero broadcasts"), FailedBroadcasts, 0);

	FRunDeckBatchMoveRequest StaleSource = MakeMoveRequest(
		*Run,
		{ FirstId },
		EZoneKind::Backpack,
		EZoneKind::BurdenZone);
	TestFalse(TEXT("Stale source is rejected"), Run->MoveInstancesAtomic(StaleSource).bSucceeded);
	FRunDeckBatchMoveRequest StaleRevision = MakeMoveRequest(
		*Run,
		{ FirstId },
		EZoneKind::BattleDeck,
		EZoneKind::BurdenZone);
	StaleRevision.ExpectedStorageRevision--;
	TestFalse(TEXT("Strict stale revision is rejected"), Run->MoveInstancesAtomic(StaleRevision).bSucceeded);
	TestTrue(TEXT("Valid burden batch move succeeds"),
		Run->MoveInstancesAtomic(MakeMoveRequest(
			*Run, { FirstId }, EZoneKind::BattleDeck, EZoneKind::BurdenZone)).bSucceeded);

	FRunDeckBatchMoveRequest InvalidId = MakeMoveRequest(
		*Run, { FGuid::NewGuid() }, EZoneKind::BattleDeck, EZoneKind::Backpack);
	TestFalse(TEXT("Unknown instance id is rejected"), Run->MoveInstancesAtomic(InvalidId).bSucceeded);

	UCardDefinition* SpecialBag = MakeBatchMoveCard(Outer, TEXT("BatchMove.SpecialBag"), 5);
	UCardDefinition* SpecialOwner = MakeBatchMoveCard(Outer, TEXT("BatchMove.SpecialOwner"), 3, true);
	UCardDefinition* SpecialContent = MakeBatchMoveCard(Outer, TEXT("BatchMove.SpecialContent"));
	TStrongObjectPtr<URunSession> SpecialRun(MakeBatchMoveRun(Outer, SpecialBag));
	SpecialRun->AcquireCardToRun(SpecialOwner);
	SpecialRun->AcquireCardToRun(SpecialContent);
	const FGuid OwnerId = FindCardId(SpecialRun->GetRunState(), SpecialOwner, EZoneKind::Backpack);
	const FGuid ContentId = FindCardId(SpecialRun->GetRunState(), SpecialContent, EZoneKind::Backpack);
	FRunDeckBatchMoveRequest ToSpecial = MakeMoveRequest(
		*SpecialRun,
		{ ContentId },
		EZoneKind::Backpack,
		EZoneKind::SpecialZone,
		FGuid(),
		OwnerId);
	TestTrue(TEXT("Special target preview succeeds while owner exists"),
		SpecialRun->ValidateMoveInstancesAtomic(ToSpecial).bCanExecute);
	TestTrue(TEXT("Special owner can disappear before commit"), SpecialRun->DestroyCardByInstance(OwnerId));
	ToSpecial.ExpectedStorageRevision = SpecialRun->GetBackpackStorageSnapshotRevision();
	TestFalse(TEXT("Missing SpecialZone owner rejects commit atomically"),
		SpecialRun->MoveInstancesAtomic(ToSpecial).bSucceeded);
	TestTrue(TEXT("Missing owner rejection leaves content in source"),
		FindCardId(SpecialRun->GetRunState(), SpecialContent, EZoneKind::Backpack).IsValid());

	UCardDefinition* ParityBag = MakeBatchMoveCard(Outer, TEXT("BatchMove.ParityBag"), 2);
	UCardDefinition* ParityCard = MakeBatchMoveCard(Outer, TEXT("BatchMove.ParityCard"));
	TStrongObjectPtr<URunSession> SingleRun(MakeBatchMoveRun(Outer, ParityBag));
	TStrongObjectPtr<URunSession> OneItemBatchRun(MakeBatchMoveRun(Outer, ParityBag));
	SingleRun->AcquireCardToRun(ParityCard);
	OneItemBatchRun->AcquireCardToRun(ParityCard);
	const FGuid SingleId = FindCardId(SingleRun->GetRunState(), ParityCard, EZoneKind::Backpack);
	const FGuid BatchId = FindCardId(OneItemBatchRun->GetRunState(), ParityCard, EZoneKind::Backpack);
	const uint64 SingleRevisionBefore = SingleRun->GetBackpackStorageSnapshotRevision();
	const uint64 BatchRevisionBefore = OneItemBatchRun->GetBackpackStorageSnapshotRevision();
	int32 SingleBroadcasts = 0;
	int32 BatchBroadcasts = 0;
	SingleRun->OnRunStateChangedNative.AddLambda([&SingleBroadcasts]() { ++SingleBroadcasts; });
	OneItemBatchRun->OnRunStateChangedNative.AddLambda([&BatchBroadcasts]() { ++BatchBroadcasts; });
	const bool bSingleSucceeded = SingleRun->MoveInstance(SingleId, EZoneKind::BattleDeck, FGuid());
	const FRunDeckBatchOperationResult OneItemBatchResult = OneItemBatchRun->MoveInstancesAtomic(
		MakeMoveRequest(*OneItemBatchRun, { BatchId }, EZoneKind::Backpack, EZoneKind::BattleDeck));
	TestEqual(TEXT("One-item batch keeps single-card success parity"), OneItemBatchResult.bSucceeded, bSingleSucceeded);
	TestEqual(TEXT("One-item batch keeps source-count parity"),
		OneItemBatchRun->GetRunState().Backpack.Num(), SingleRun->GetRunState().Backpack.Num());
	TestEqual(TEXT("One-item batch keeps target-count parity"),
		OneItemBatchRun->GetRunState().BattleDeck.Num(), SingleRun->GetRunState().BattleDeck.Num());
	TestEqual(TEXT("Single-card API advances storage revision once"),
		SingleRun->GetBackpackStorageSnapshotRevision(), SingleRevisionBefore + 1);
	TestEqual(TEXT("One-item batch advances storage revision once"),
		OneItemBatchRun->GetBackpackStorageSnapshotRevision(), BatchRevisionBefore + 1);
	TestEqual(TEXT("Single-card API broadcasts once"), SingleBroadcasts, 1);
	TestEqual(TEXT("One-item batch broadcasts once"), BatchBroadcasts, 1);
	return true;
}
