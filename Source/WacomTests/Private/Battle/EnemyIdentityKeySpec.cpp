// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Runtime/BattleEnemyKeys.h"
#include "Runtime/BattlePartSlotIdentity.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleEnemyIdentityKeyContractSpec,
	"Wacom.Battle.TargetIdentity.EnemyKeyContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleEnemyIdentityKeyContractSpec::RunTest(const FString& /*Parameters*/)
{
	const FBattleEnemyUnitKey UnitA =
		FBattleEnemyUnitKey::Make(TEXT("Encounter.A"), TEXT("Enemy.Left"));
	const FBattleEnemyUnitKey UnitA2 =
		FBattleEnemyUnitKey::Make(TEXT("Encounter.A"), TEXT("Enemy.Left"));
	const FBattleEnemyUnitKey UnitB =
		FBattleEnemyUnitKey::Make(TEXT("Encounter.A"), TEXT("Enemy.Right"));

	TestTrue(TEXT("Unit key is valid"), UnitA.IsValidKey());
	TestTrue(TEXT("Same unit keys are equal"), UnitA == UnitA2);
	TestFalse(TEXT("Different enemy unit slots are not equal"), UnitA == UnitB);
	TestEqual(TEXT("Unit debug string"), UnitA.ToDebugString(), FString(TEXT("Encounter.A.Enemy.Left")));

	TMap<FBattleEnemyUnitKey, int32> UnitMap;
	UnitMap.Add(UnitA, 7);
	TestEqual(TEXT("Unit key hashes for map lookup"), UnitMap.FindRef(UnitA2), 7);
	TestFalse(TEXT("Different unit key misses map lookup"), UnitMap.Contains(UnitB));

	const FBattleEnemyPartKey PartA =
		FBattleEnemyPartKey::Make(TEXT("Encounter.A"), TEXT("Enemy.Left"), TEXT("Head"));
	const FBattleEnemyPartKey PartA2 =
		FBattleEnemyPartKey::Make(TEXT("Encounter.A"), TEXT("Enemy.Left"), TEXT("Head"));
	const FBattleEnemyPartKey PartB =
		FBattleEnemyPartKey::Make(TEXT("Encounter.A"), TEXT("Enemy.Left"), TEXT("Tail"));
	const FBattleEnemyPartKey PartC =
		FBattleEnemyPartKey::Make(TEXT("Encounter.A"), TEXT("Enemy.Right"), TEXT("Head"));

	TestTrue(TEXT("Part key is valid"), PartA.IsValidKey());
	TestTrue(TEXT("Same part keys are equal"), PartA == PartA2);
	TestFalse(TEXT("Different part slots are not equal"), PartA == PartB);
	TestFalse(TEXT("Different enemy unit slots are not equal"), PartA == PartC);
	TestEqual(TEXT("Part debug string"), PartA.ToDebugString(), FString(TEXT("Encounter.A.Enemy.Left.Head")));
	TestEqual(TEXT("Part exposes unit key"), PartA.GetUnitKey().ToDebugString(), UnitA.ToDebugString());

	TSet<FBattleEnemyPartKey> PartSet;
	PartSet.Add(PartA);
	TestTrue(TEXT("Part key hashes for set lookup"), PartSet.Contains(PartA2));
	TestFalse(TEXT("Different part slot misses set lookup"), PartSet.Contains(PartB));
	TestFalse(TEXT("Different enemy slot misses set lookup"), PartSet.Contains(PartC));

	const FBattlePartSlotIdentity RuntimeIdentity =
		FBattlePartSlotIdentity::Make(TEXT("Encounter.A"), TEXT("Enemy.Left"), TEXT("Head"));
	const FBattleEnemyPartKey RuntimeKey = RuntimeIdentity.ToEnemyPartKey();
	TestEqual(TEXT("Runtime identity converts to public key encounter"),
		RuntimeKey.GetEffectiveEncounterId(),
		FName(TEXT("Encounter.A")));
	TestEqual(TEXT("Runtime identity converts to public key unit"),
		RuntimeKey.GetEffectiveEnemyUnitSlotId(),
		FName(TEXT("Enemy.Left")));
	TestEqual(TEXT("Runtime identity converts to public key part"),
		RuntimeKey.GetEffectivePartSlotId(),
		FName(TEXT("Head")));

	const FBattlePartSlotIdentity RoundTripIdentity =
		FBattlePartSlotIdentity::FromEnemyPartKey(RuntimeKey);
	TestTrue(TEXT("Public key round-trips into runtime slot identity"),
		RoundTripIdentity.MatchesRuntimeSlot(RuntimeIdentity));

	const FBattleEnemyPartKey MissingPart =
		FBattleEnemyPartKey::Make(TEXT("Encounter.A"), TEXT("Enemy.Left"), NAME_None);
	TestFalse(TEXT("Part key without part slot is invalid"), MissingPart.IsValidKey());

	return true;
}
