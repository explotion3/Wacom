// Copyright Wacom. All Rights Reserved.

#include "Battle/BattleSessionTestAccess.h"
#include "Fixtures/BattleTestFixtures.h"

#include "Commands/BattleCommand.h"
#include "Characters/CharacterDefinition.h"
#include "Events/BattleEvent.h"
#include "Misc/AutomationTest.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	FBattleInitParams MakeInitParams(
		UCharacterDefinition* Character,
		UEnemyDefinition* Enemy,
		int32 Seed,
		FName EnemySlotId = TEXT("Enemy"))
	{
		FBattleInitParams Params;
		Params.Character = Character;
		Params.RandomSeed = Seed;
		FBattleEnemySlotInit Slot;
		Slot.EnemySlotId = EnemySlotId;
		Slot.Enemy = Enemy;
		Params.EnemySlots.Add(Slot);
		return Params;
	}

	TArray<FGuid> GetInitializationHandIds(const FBattleSnapshot& Snapshot)
	{
		TArray<FGuid> Ids;
		for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
		{
			Ids.Add(Card.InstanceId);
		}
		return Ids;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleInitializationResultAtomicSpec,
	"Wacom.Battle.InitializationResult.AtomicCommitRollbackAndSequence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleInitializationResultAtomicSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCharacterDefinition* OriginalCharacter = Fixture.MakeCharacter(
		Fixture.MakeNoopCard(0),
		Fixture.MakeNoopCard(0),
		{ Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0) });
	UEnemyDefinition* OriginalEnemy =
		Fixture.MakeSinglePartEnemyWithIntentDamage(100, 50, 0);
	TStrongObjectPtr<UBattleSession> Session(NewObject<UBattleSession>());

	const FBattleInitializationResult Initial =
		Session->Initialize(MakeInitParams(OriginalCharacter, OriginalEnemy, 101));
	if (!TestTrue(TEXT("Initial battle succeeds"), Initial.IsOk()))
	{
		return false;
	}
	TestEqual(TEXT("Initial snapshot starts at version one"), Initial.PostSnapshot.Version, 1);
	TestEqual(TEXT("Initial result snapshot is committed live"),
		Session->BuildSnapshot().Version,
		Initial.PostSnapshot.Version);
	TestFalse(TEXT("Initial result owns opening events"), Initial.Events.IsEmpty());
	if (!Initial.Events.IsEmpty())
	{
		TestEqual(TEXT("Opening event sequence starts at zero"), Initial.Events[0].Sequence, 0);
		TestEqual(TEXT("BattleStarted is the first opening event"),
			Initial.Events[0].Type,
			EBattleEventType::BattleStarted);
		for (int32 Index = 0; Index < Initial.Events.Num(); ++Index)
		{
			TestEqual(TEXT("Opening event sequence is contiguous"),
				Initial.Events[Index].Sequence,
				Index);
		}
	}

	const int32 ExpectedFirstCommandSequence = Initial.Events.Num();
	const FBattleResolution FirstWait = Session->ResolveCommand(FBattleCommand::MakeWait());
	TestTrue(TEXT("First command succeeds"), FirstWait.IsOk());
	if (!FirstWait.Events.IsEmpty())
	{
		TestEqual(TEXT("First command continues initialization sequence"),
			FirstWait.Events[0].Sequence,
			ExpectedFirstCommandSequence);
	}

	const FBattleSnapshot BeforeFailedReinitialize = Session->BuildSnapshot();
	const TArray<FGuid> BeforeHandIds = GetInitializationHandIds(BeforeFailedReinitialize);
	const int32 BeforeNextSequence =
		FWacomBattleSessionTestAccess::GetNextEventSequence(Session.Get());
	const int32 BeforeRandomSeed =
		FWacomBattleSessionTestAccess::GetRandomCurrentSeed(Session.Get());
	const int32 BeforeReferencedAssetCount =
		FWacomBattleSessionTestAccess::GetReferencedAssetCount(Session.Get());

	UCharacterDefinition* RejectedCharacter = Fixture.MakeCharacter(
		Fixture.MakeNoopCard(0),
		Fixture.MakeNoopCard(0),
		{ Fixture.MakeNoopCard(0) });
	UEnemyDefinition* RejectedEnemy = Fixture.MakeSinglePartEnemy(25, 4);
	FBattleInitParams RejectedParams =
		MakeInitParams(RejectedCharacter, RejectedEnemy, 202, TEXT("Duplicate"));
	const FBattleEnemySlotInit DuplicateEnemySlot = RejectedParams.EnemySlots[0];
	RejectedParams.EnemySlots.Add(DuplicateEnemySlot);

	const FBattleInitializationResult Rejected = Session->Initialize(RejectedParams);
	TestFalse(TEXT("Duplicate enemy slot reinitialize fails"), Rejected.IsOk());
	TestTrue(TEXT("Failed reinitialize returns no events"), Rejected.Events.IsEmpty());
	TestEqual(TEXT("Failed reinitialize returns old snapshot version"),
		Rejected.PostSnapshot.Version,
		BeforeFailedReinitialize.Version);
	TestTrue(TEXT("Failed reinitialize preserves hand identity"),
		GetInitializationHandIds(Rejected.PostSnapshot) == BeforeHandIds);
	TestTrue(TEXT("Failed reinitialize preserves live hand identity"),
		GetInitializationHandIds(Session->BuildSnapshot()) == BeforeHandIds);
	TestEqual(TEXT("Failed reinitialize preserves event sequence"),
		FWacomBattleSessionTestAccess::GetNextEventSequence(Session.Get()),
		BeforeNextSequence);
	TestEqual(TEXT("Failed reinitialize preserves RNG"),
		FWacomBattleSessionTestAccess::GetRandomCurrentSeed(Session.Get()),
		BeforeRandomSeed);
	TestEqual(TEXT("Failed reinitialize preserves referenced asset count"),
		FWacomBattleSessionTestAccess::GetReferencedAssetCount(Session.Get()),
		BeforeReferencedAssetCount);
	TestTrue(TEXT("Failed reinitialize keeps original character referenced"),
		FWacomBattleSessionTestAccess::ContainsReferencedAsset(Session.Get(), OriginalCharacter));
	TestFalse(TEXT("Failed reinitialize does not retain rejected character"),
		FWacomBattleSessionTestAccess::ContainsReferencedAsset(Session.Get(), RejectedCharacter));

	const int32 BeforeRejectedCommandSequence =
		FWacomBattleSessionTestAccess::GetNextEventSequence(Session.Get());
	const FBattleResolution RejectedCommand = Session->ResolveCommand(
		FBattleCommand::MakePlayCard(FGuid::NewGuid()));
	TestFalse(TEXT("Invalid command fails"), RejectedCommand.IsOk());
	TestTrue(TEXT("Invalid command emits no events"), RejectedCommand.Events.IsEmpty());
	TestEqual(TEXT("Invalid command does not burn event sequence"),
		FWacomBattleSessionTestAccess::GetNextEventSequence(Session.Get()),
		BeforeRejectedCommandSequence);

	const FBattleInitializationResult Reinitialized =
		Session->Initialize(MakeInitParams(RejectedCharacter, RejectedEnemy, 303));
	TestTrue(TEXT("Valid reinitialize succeeds"), Reinitialized.IsOk());
	TestEqual(TEXT("Successful reinitialize starts a new version-one battle"),
		Reinitialized.PostSnapshot.Version,
		1);
	TestFalse(TEXT("Successful reinitialize owns opening events"), Reinitialized.Events.IsEmpty());
	if (!Reinitialized.Events.IsEmpty())
	{
		TestEqual(TEXT("Successful reinitialize resets event sequence"),
			Reinitialized.Events[0].Sequence,
			0);
	}
	const FBattleResolution ReinitializedWait =
		Session->ResolveCommand(FBattleCommand::MakeWait());
	if (!ReinitializedWait.Events.IsEmpty())
	{
		TestEqual(TEXT("Command after reinitialize follows opening events"),
			ReinitializedWait.Events[0].Sequence,
			Reinitialized.Events.Num());
	}

	return true;
}
