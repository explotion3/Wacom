// Copyright Wacom. All Rights Reserved.

#include "Components/WacomInteractionTargetComponent.h"

#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "Tags/WacomGameplayTags.h"

UWacomInteractionTargetComponent::UWacomInteractionTargetComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FWacomInteractionTargetHandle UWacomInteractionTargetComponent::BuildWorldTargetHandle() const
{
	const bool bHasBattlePartSlotIdentity =
		!EncounterId.IsNone() && !EnemySlotId.IsNone() && !PartSlotId.IsNone();
	const bool bCanBuildKeyOnlyBattlePartHandle =
		InteractionTargetTag.MatchesTagExact(WacomTags::Interaction_Target_Battle_EnemyPart)
		&& bHasBattlePartSlotIdentity;
	if (!TargetId.IsValid() && !bCanBuildKeyOnlyBattlePartHandle)
	{
		return FWacomInteractionTargetHandle();
	}

	AActor* Owner = GetOwner();
	const FVector WorldLoc = Owner ? Owner->GetActorLocation() : FVector::ZeroVector;

	return FWacomInteractionTargetHandle::ForWorldTarget(
		TargetId,
		const_cast<UWacomInteractionTargetComponent*>(this),
		WorldLoc,
		FVector2D::ZeroVector,
		InteractionTargetTag,
		StableTargetId,
		EncounterId,
		EnemySlotId,
		PartSlotId);
}

void UWacomInteractionTargetComponent::SetTargetId(const FGuid& InTargetId)
{
	TargetId = InTargetId;
}

void UWacomInteractionTargetComponent::SetInteractionTargetTag(const FGameplayTag& InTag)
{
	InteractionTargetTag = InTag;
}

void UWacomInteractionTargetComponent::SetStableTargetId(FName InStableTargetId)
{
	StableTargetId = InStableTargetId;
}

void UWacomInteractionTargetComponent::SetBattlePartSlotIdentity(
	FName InEncounterId,
	FName InEnemySlotId,
	FName InPartSlotId)
{
	EncounterId = InEncounterId;
	EnemySlotId = InEnemySlotId;
	PartSlotId = InPartSlotId;
}

void UWacomInteractionTargetComponent::LogHandleToConsole()
{
	const FWacomInteractionTargetHandle Handle = BuildWorldTargetHandle();

	if (!Handle.IsValid())
	{
		UE_LOG(LogTemp, Display,
			TEXT("[WacomInteractionTarget] %s :: TargetId 无效且没有完整 Battle 部位 Key，BuildWorldTargetHandle 返回无效 handle。请设置 TargetId 或 EncounterId/EnemySlotId/PartSlotId。"),
			*GetNameSafe(GetOwner()));
		return;
	}

	UE_LOG(LogTemp, Display,
		TEXT("[WacomInteractionTarget] %s :: Handle=%s"),
		*GetNameSafe(GetOwner()), *Handle.ToString());
}
