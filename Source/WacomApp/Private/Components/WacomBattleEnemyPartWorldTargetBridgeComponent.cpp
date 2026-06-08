// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattleEnemyPartWorldTargetBridgeComponent.h"

#include "Components/WacomInteractionTargetComponent.h"
#include "Enemies/EnemyPartDefinition.h"
#include "GameFramework/Actor.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomInteractionTargetTypes.h"

#define LOCTEXT_NAMESPACE "WacomBattleEnemyPartWorldTargetBridge"

UWacomBattleEnemyPartWorldTargetBridgeComponent::UWacomBattleEnemyPartWorldTargetBridgeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::SetPartId(FName InPartId)
{
	PartId = InPartId;
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::SetBattlePartSlotIdentity(
	FName InEncounterId,
	FName InEnemySlotId,
	FName InPartSlotId)
{
	EncounterId = InEncounterId;
	EnemySlotId = InEnemySlotId;
	PartSlotId = InPartSlotId;
}

bool UWacomBattleEnemyPartWorldTargetBridgeComponent::SyncFromBattleSnapshot(
	const FBattleSnapshot& Snapshot,
	FEnemyPartSnapshot* OutMatchedPart)
{
	if (EncounterId.IsNone() || EnemySlotId.IsNone() || PartSlotId.IsNone())
	{
		LastBindResult = TEXT("MissingBattlePartSlotIdentity");
		ClearBattleBinding();
		return false;
	}

	const FEnemyPartSnapshot* MatchedPart = nullptr;
	for (const FEnemySnapshot& EnemySnapshot : Snapshot.Enemies)
	{
		if (EnemySnapshot.EncounterId != EncounterId)
		{
			continue;
		}
		if (EnemySnapshot.EnemySlotId != EnemySlotId)
		{
			continue;
		}

		for (const FEnemyPartSnapshot& Part : EnemySnapshot.Parts)
		{
			if (Part.PartSlotId == PartSlotId)
			{
				MatchedPart = &Part;
				break;
			}
		}
		if (MatchedPart)
		{
			break;
		}
	}

	if (!MatchedPart || !MatchedPart->InstanceId.IsValid() || MatchedPart->bDestroyed)
	{
		LastBindResult = MatchedPart && MatchedPart->bDestroyed ? TEXT("PartDestroyed") : TEXT("NoMatchingPart");
		if (MatchedPart)
		{
			CacheRuntimePartBindingFacts(*MatchedPart);
			ClearBattleBindingInternal(/*bClearRuntimeFacts=*/false);
			if (OutMatchedPart)
			{
				*OutMatchedPart = *MatchedPart;
			}
		}
		else
		{
			ClearBattleBinding();
		}
		return false;
	}

	PartInstanceId = MatchedPart->InstanceId;
	BoundEncounterId = MatchedPart->EncounterId;
	BoundEnemySlotId = MatchedPart->EnemySlotId;
	BoundPartSlotId = MatchedPart->PartSlotId;
	bBoundToSnapshot = true;
	LastBindResult = TEXT("MatchedPartSlot");
	CacheRuntimePartBindingFacts(*MatchedPart);
	UpdateInteractionTargetComponent();
	if (OutMatchedPart)
	{
		*OutMatchedPart = *MatchedPart;
	}
	return true;
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::ClearBattleBinding()
{
	ClearBattleBindingInternal(/*bClearRuntimeFacts=*/true);
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::ClearBattleBindingInternal(bool bClearRuntimeFacts)
{
	bRegisteredWithBattleHUD = false;
	PartInstanceId.Invalidate();
	BoundEncounterId = NAME_None;
	BoundEnemySlotId = NAME_None;
	BoundPartSlotId = NAME_None;
	bBoundToSnapshot = false;
	TargetDisabledReason = NAME_None;
	bTargetable = false;
	if (bClearRuntimeFacts)
	{
		ClearRuntimePartBindingFacts();
	}

	if (UWacomInteractionTargetComponent* TargetComponent = ResolveInteractionTargetComponent())
	{
		TargetComponent->SetTargetId(FGuid());
		TargetComponent->SetBattlePartSlotIdentity(NAME_None, NAME_None, NAME_None);
	}
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::SetBattleHUDSceneRegistryState(
	bool bInRegisteredWithBattleHUD)
{
	bRegisteredWithBattleHUD = bInRegisteredWithBattleHUD;
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::SetBattleTargetableState(
	bool bInTargetable,
	FName InDisabledReason)
{
	bTargetable = bInTargetable;
	TargetDisabledReason = InDisabledReason;
}

FWacomBattleEnemyPartWorldTargetDebugView
UWacomBattleEnemyPartWorldTargetBridgeComponent::GetBattleWorldTargetDebugView() const
{
	FWacomBattleEnemyPartWorldTargetDebugView View;
	View.PartId = PartId;
	View.EncounterId = BoundEncounterId;
	View.EnemySlotId = BoundEnemySlotId;
	View.PartSlotId = BoundPartSlotId;
	View.PartInstanceId = PartInstanceId;
	View.bBoundToSnapshot = bBoundToSnapshot;
	View.bRegisteredWithBattleHUD = bRegisteredWithBattleHUD;
	View.RuntimePartInstanceId = RuntimePartInstanceId;
	View.bHasRuntimePartFacts = RuntimePartInstanceId.IsValid();
	View.bTargetable = bTargetable;
	View.TargetDisabledReason = TargetDisabledReason;
	View.LastBindResult = LastBindResult;
	return View;
}

FString UWacomBattleEnemyPartWorldTargetBridgeComponent::GetBattleWorldTargetDebugSummary() const
{
	const FWacomBattleEnemyPartWorldTargetDebugView View = GetBattleWorldTargetDebugView();
	return FString::Printf(
		TEXT("BattleEnemyPartWorldTarget{Owner=%s PartId=%s EncounterId=%s EnemySlotId=%s PartSlotId=%s PartInstanceId=%s Bound=%s Registered=%s RuntimeFacts=%s RuntimePart=%s Targetable=%s Disabled=%s LastBind=%s}"),
		*GetNameSafe(GetOwner()),
		*View.PartId.ToString(),
		*View.EncounterId.ToString(),
		*View.EnemySlotId.ToString(),
		*View.PartSlotId.ToString(),
		*View.PartInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
		View.bBoundToSnapshot ? TEXT("true") : TEXT("false"),
		View.bRegisteredWithBattleHUD ? TEXT("true") : TEXT("false"),
		View.bHasRuntimePartFacts ? TEXT("true") : TEXT("false"),
		*View.RuntimePartInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
		View.bTargetable ? TEXT("true") : TEXT("false"),
		*View.TargetDisabledReason.ToString(),
		*View.LastBindResult.ToString());
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::LogBattleWorldTargetDebugSummary() const
{
	UE_LOG(LogTemp, Display, TEXT("[WacomBattleEnemyPartWorldTarget] %s"), *GetBattleWorldTargetDebugSummary());
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearBattleBinding();
	Super::EndPlay(EndPlayReason);
}

UWacomInteractionTargetComponent*
UWacomBattleEnemyPartWorldTargetBridgeComponent::ResolveInteractionTargetComponent() const
{
	AActor* Owner = GetOwner();
	return Owner ? Owner->FindComponentByClass<UWacomInteractionTargetComponent>() : nullptr;
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::CacheRuntimePartBindingFacts(
	const FEnemyPartSnapshot& Part)
{
	RuntimePartInstanceId = Part.InstanceId;
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::ClearRuntimePartBindingFacts()
{
	RuntimePartInstanceId.Invalidate();
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::UpdateInteractionTargetComponent()
{
	if (!bAutoConfigureInteractionTarget)
	{
		return;
	}

	if (UWacomInteractionTargetComponent* TargetComponent = ResolveInteractionTargetComponent())
	{
		TargetComponent->SetTargetId(PartInstanceId);
		TargetComponent->SetStableTargetId(PartId);
		TargetComponent->SetBattlePartSlotIdentity(
			BoundEncounterId,
			BoundEnemySlotId,
			BoundPartSlotId);
		TargetComponent->SetInteractionTargetTag(WacomTags::Interaction_Target_Battle_EnemyPart);
	}
}

#undef LOCTEXT_NAMESPACE
