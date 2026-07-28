// Copyright Wacom. All Rights Reserved.

#include "Battle/BattleSessionTestAccess.h"

#if WITH_AUTOMATION_TESTS

#include "Session/BattleSession.h"

bool FWacomBattleSessionTestAccess::ValidateCardZoneInvariants(
	const UBattleSession* Session,
	FString& OutError)
{
	return Session && Session->ValidateCardZoneInvariantsForAutomationTest(OutError);
}

int32 FWacomBattleSessionTestAccess::GetReferencedAssetCount(const UBattleSession* Session)
{
	return Session ? Session->GetReferencedAssetCountForAutomationTest() : INDEX_NONE;
}

bool FWacomBattleSessionTestAccess::ContainsReferencedAsset(
	const UBattleSession* Session,
	const UObject* Asset)
{
	return Session && Session->ContainsReferencedAssetForAutomationTest(Asset);
}

int32 FWacomBattleSessionTestAccess::GetNextEventSequence(const UBattleSession* Session)
{
	return Session ? Session->GetNextEventSequenceForAutomationTest() : INDEX_NONE;
}

int32 FWacomBattleSessionTestAccess::GetRandomCurrentSeed(const UBattleSession* Session)
{
	return Session ? Session->GetRandomCurrentSeedForAutomationTest() : INDEX_NONE;
}

bool FWacomBattleSessionTestAccess::SetPlayerStatusStacks(
	UBattleSession* Session,
	const FGameplayTag& Status,
	const int32 Stacks)
{
	return Session
		&& Session->SetPlayerStatusStacksForAutomationTest(Status, Stacks);
}

bool FWacomBattleSessionTestAccess::SetEnemyPartStatusStacks(
	UBattleSession* Session,
	const FBattleEnemyPartKey& PartKey,
	const FGameplayTag& Status,
	const int32 Stacks)
{
	return Session
		&& Session->SetEnemyPartStatusStacksForAutomationTest(
			PartKey, Status, Stacks);
}

bool FWacomBattleSessionTestAccess::SetEnemyPartShield(
	UBattleSession* Session,
	const FBattleEnemyPartKey& PartKey,
	const int32 Shield)
{
	return Session
		&& Session->SetEnemyPartShieldForAutomationTest(PartKey, Shield);
}

bool FWacomBattleSessionTestAccess::SetCardStatusStacks(
	UBattleSession* Session,
	const FGuid& CardInstanceId,
	const FGameplayTag& Status,
	const int32 Stacks)
{
	return Session
		&& Session->SetCardStatusStacksForAutomationTest(
			CardInstanceId, Status, Stacks);
}

bool FWacomBattleSessionTestAccess::SetCardRuntimeCostModifier(
	UBattleSession* Session,
	const FGuid& CardInstanceId,
	const int32 Modifier)
{
	return Session
		&& Session->SetCardRuntimeCostModifierForAutomationTest(
			CardInstanceId, Modifier);
}

bool FWacomBattleSessionTestAccess::SetCardCriticalChanceBonus(
	UBattleSession* Session,
	const FGuid& CardInstanceId,
	const int32 BonusPercent)
{
	return Session
		&& Session->SetCardCriticalChanceBonusForAutomationTest(
			CardInstanceId, BonusPercent);
}

bool FWacomBattleSessionTestAccess::ResolveSettlementPassives(
	UBattleSession* Session)
{
	return Session
		&& Session->ResolveSettlementPassivesForAutomationTest();
}

bool FWacomBattleSessionTestAccess::GetCardRuntimeState(
	const UBattleSession* Session,
	const FGuid& CardInstanceId,
	FRuntimeCardInstance& OutCard)
{
	return Session
		&& Session->GetCardRuntimeStateForAutomationTest(
			CardInstanceId, OutCard);
}

#endif
