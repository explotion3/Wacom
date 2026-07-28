// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_AUTOMATION_TESTS

class UBattleSession;
struct FBattleEnemyPartKey;
struct FGameplayTag;
struct FRuntimeCardInstance;

struct FWacomBattleSessionTestAccess
{
	static bool ValidateCardZoneInvariants(
		const UBattleSession* Session,
		FString& OutError);
	static int32 GetReferencedAssetCount(const UBattleSession* Session);
	static bool ContainsReferencedAsset(const UBattleSession* Session, const UObject* Asset);
	static int32 GetNextEventSequence(const UBattleSession* Session);
	static int32 GetRandomCurrentSeed(const UBattleSession* Session);
	static bool SetPlayerStatusStacks(
		UBattleSession* Session,
		const FGameplayTag& Status,
		int32 Stacks);
	static bool SetEnemyPartStatusStacks(
		UBattleSession* Session,
		const FBattleEnemyPartKey& PartKey,
		const FGameplayTag& Status,
		int32 Stacks);
	static bool SetEnemyPartShield(
		UBattleSession* Session,
		const FBattleEnemyPartKey& PartKey,
		int32 Shield);
	static bool SetCardStatusStacks(
		UBattleSession* Session,
		const FGuid& CardInstanceId,
		const FGameplayTag& Status,
		int32 Stacks);
	static bool SetCardRuntimeCostModifier(
		UBattleSession* Session,
		const FGuid& CardInstanceId,
		int32 Modifier);
	static bool SetCardCriticalChanceBonus(
		UBattleSession* Session,
		const FGuid& CardInstanceId,
		int32 BonusPercent);
	static bool ResolveSettlementPassives(UBattleSession* Session);
	static bool GetCardRuntimeState(
		const UBattleSession* Session,
		const FGuid& CardInstanceId,
		FRuntimeCardInstance& OutCard);
};

#endif
