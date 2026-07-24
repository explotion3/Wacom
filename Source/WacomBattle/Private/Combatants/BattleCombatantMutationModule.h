// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Events/BattleEvent.h"
#include "GameplayTagContainer.h"

struct FBattleEventBus;
struct FBattleState;

enum class EBattleCombatantKind : uint8
{
	Player,
	EnemyPart,
};

struct FBattleCombatantHandle
{
	EBattleCombatantKind Kind = EBattleCombatantKind::Player;
	FGuid EnemyPartInstanceId;

	static FBattleCombatantHandle Player()
	{
		return {};
	}

	static FBattleCombatantHandle EnemyPart(const FGuid& InstanceId)
	{
		FBattleCombatantHandle Handle;
		Handle.Kind = EBattleCombatantKind::EnemyPart;
		Handle.EnemyPartInstanceId = InstanceId;
		return Handle;
	}
};

enum class EDamageShieldInteraction : uint8
{
	ConsumeShield,
	BypassShield,
};

enum class ECombatantMutationReject : uint8
{
	None,
	InvalidTarget,
	DestroyedTarget,
	NonPositiveAmount,
	InvalidStatus,
	MissingStatus,
};

struct FDamageMutationIntent
{
	FBattleCombatantHandle Target;
	int32 RequestedDamage = 0;
	EDamageShieldInteraction ShieldInteraction = EDamageShieldInteraction::ConsumeShield;
	FGuid SourceCardInstanceId;
	FGameplayTag CauseTag;
	EBattleDamageKind DamageKind = EBattleDamageKind::Direct;
	bool bCritical = false;
};

struct FShieldMutationIntent
{
	FBattleCombatantHandle Target;
	int32 RequestedShield = 0;
	FGuid SourceCardInstanceId;
	FGameplayTag CauseTag;
};

struct FStatusApplicationIntent
{
	FBattleCombatantHandle Target;
	FGameplayTag Status;
	int32 Stacks = 0;
	FGuid EventSourceCardInstanceId;
};

struct FDamageMutationResult
{
	ECombatantMutationReject Reject = ECombatantMutationReject::None;
	int32 RequestedDamage = 0;
	int32 ShieldBefore = 0;
	int32 ShieldAbsorbed = 0;
	int32 ShieldAfter = 0;
	int32 HpBefore = 0;
	int32 HpLost = 0;
	int32 HpAfter = 0;
	int32 Overkill = 0;
	bool bCrossedHighHpThreshold = false;
	bool bCrossedLowHpThreshold = false;
	bool bDestroyedNow = false;

	bool IsAccepted() const { return Reject == ECombatantMutationReject::None; }
};

struct FHealingMutationResult
{
	ECombatantMutationReject Reject = ECombatantMutationReject::None;
	int32 RequestedHealing = 0;
	int32 HpBefore = 0;
	int32 HpRestored = 0;
	int32 HpAfter = 0;

	bool IsAccepted() const { return Reject == ECombatantMutationReject::None; }
};

struct FShieldMutationResult
{
	ECombatantMutationReject Reject = ECombatantMutationReject::None;
	int32 RequestedShield = 0;
	int32 ShieldBefore = 0;
	int32 ShieldAdded = 0;
	int32 ShieldAfter = 0;

	bool IsAccepted() const { return Reject == ECombatantMutationReject::None; }
};

struct FStatusMutationResult
{
	ECombatantMutationReject Reject = ECombatantMutationReject::None;
	int32 StacksBefore = 0;
	int32 AppliedDelta = 0;
	int32 StacksAfter = 0;

	bool IsAccepted() const { return Reject == ECombatantMutationReject::None; }
};

/**
 * Stack-status read model shared by rules and Snapshot projection.
 * Runtime mutation remains owned by FBattleCombatantMutationModule.
 */
struct FBattleCombatantStatusFacts
{
	static int32 GetStacks(const TMap<FGameplayTag, int32>& StatusStacks, const FGameplayTag& Status);
	static bool HasStatusExact(const TMap<FGameplayTag, int32>& StatusStacks, const FGameplayTag& Status);
	static bool HasStatus(const TMap<FGameplayTag, int32>& StatusStacks, const FGameplayTag& Status);
	static FGameplayTagContainer BuildTagProjection(const TMap<FGameplayTag, int32>& StatusStacks);
};

/**
 * Runtime combatant mutation authority.
 *
 * This Module owns HP, Shield, stack status, player HP threshold and runtime
 * enemy-part destruction writes. Operation Adapter and transaction flow remain
 * at their existing upstream seams.
 */
class FBattleCombatantMutationModule final
{
public:
	static FDamageMutationResult ApplyDamage(
		FBattleState& State,
		FBattleEventBus& Events,
		const FDamageMutationIntent& Intent);

	static FHealingMutationResult RestoreHealth(
		FBattleState& State,
		const FBattleCombatantHandle& Target,
		int32 Amount);

	static FShieldMutationResult AddShield(
		FBattleState& State,
		FBattleEventBus& Events,
		const FShieldMutationIntent& Intent);

	static FStatusMutationResult ApplyStatusStacks(
		FBattleState& State,
		FBattleEventBus& Events,
		const FStatusApplicationIntent& Intent);

	static FStatusMutationResult RemoveStatusStacks(
		FBattleState& State,
		const FBattleCombatantHandle& Target,
		const FGameplayTag& Status,
		int32 Stacks);

	/** Initialization-only path. It deliberately has no EventBus. */
	static ECombatantMutationReject InitializePreDestroyedEnemyPart(
		FBattleState& State,
		const FGuid& EnemyPartInstanceId);
};
