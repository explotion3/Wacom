// Copyright Wacom. All Rights Reserved.

#include "Deck/RunDeckRules.h"

#include "Cards/CardDefinition.h"
#include "RunState.h"
#include "Tags/WacomGameplayTags.h"

namespace
{
	namespace DeckReasons = WacomRunDeckOperationReasons;

	TArray<FCardInstance>* FindMutableMovePile(FRunState& State, EZoneKind Zone, FGuid ZoneOwnerInstanceId)
	{
		switch (Zone)
		{
		case EZoneKind::Backpack:
			return &State.Backpack;
		case EZoneKind::BattleDeck:
			return &State.BattleDeck;
		case EZoneKind::BurdenZone:
			return &State.BurdenZone;
		case EZoneKind::SpecialZone:
		{
			for (FSpecialZone& SpecialZone : State.SpecialZones)
			{
				if (SpecialZone.OwnerInstanceId == ZoneOwnerInstanceId)
				{
					return &SpecialZone.Cards;
				}
			}
			return nullptr;
		}
		default:
			return nullptr;
		}
	}

	void SetMoveDisabledReason(FName* OutDisabledReason, FName DisabledReason)
	{
		if (OutDisabledReason)
		{
			*OutDisabledReason = DisabledReason;
		}
	}

	void SetSpecialZoneBattleFlagDisabledReason(FName* OutDisabledReason, FName DisabledReason)
	{
		if (OutDisabledReason)
		{
			*OutDisabledReason = DisabledReason;
		}
	}
}

bool FRunDeckRules::IsContainerCard(const UCardDefinition* Card)
{
	return Card != nullptr && Card->Physique.Capacity > 0;
}

bool FRunDeckRules::IsTypeAContainerCard(const UCardDefinition* Card)
{
	return IsContainerCard(Card) && !Card->Physique.CapacityEffect.IsValid();
}

bool FRunDeckRules::IsTypeBContainerCard(const UCardDefinition* Card)
{
	return IsContainerCard(Card) && Card->Physique.CapacityEffect.IsValid();
}

int32 FRunDeckRules::GetSpecialZoneCapacity(const UCardDefinition* BCard)
{
	if (!BCard)
	{
		return 0;
	}
	return FMath::Max(0, BCard->Physique.Capacity - 1);
}

bool FRunDeckRules::IsFluxContentCardDefinition(const UCardDefinition* Card)
{
	// 通量存放区没有 A 类主卡槽；A 类容器和普通卡都作为通量内容。
	return Card && !IsTypeBContainerCard(Card);
}

bool FRunDeckRules::IsPreferredBurdenOverflowCandidate(const UCardDefinition* Card)
{
	// 容量来源卡尽量留在原区；容量缩小时优先把普通内容卡挪到负重区。
	return Card && !IsContainerCard(Card);
}

void FRunDeckRules::EnsureSpecialZoneEntryFor(FRunState& State, const FCardInstance& Inst)
{
	if (!IsTypeBContainerCard(Inst.Definition) || !Inst.InstanceId.IsValid())
	{
		return;
	}

	for (const FSpecialZone& SZ : State.SpecialZones)
	{
		if (SZ.OwnerInstanceId == Inst.InstanceId)
		{
			return;
		}
	}

	FSpecialZone NewEntry;
	NewEntry.OwnerInstanceId = Inst.InstanceId;
	State.SpecialZones.Add(MoveTemp(NewEntry));
}

bool FRunDeckRules::FindInstance(const FRunState& State, FGuid InstanceId, FCardInstance& OutInstance, EZoneKind& OutZone, FGuid& OutZoneOwnerInstanceId)
{
	if (!InstanceId.IsValid())
	{
		return false;
	}

	for (const FCardInstance& Inst : State.Backpack)
	{
		if (Inst.InstanceId == InstanceId)
		{
			OutInstance = Inst;
			OutZone = EZoneKind::Backpack;
			OutZoneOwnerInstanceId = FGuid();
			return true;
		}
	}

	for (const FCardInstance& Inst : State.BattleDeck)
	{
		if (Inst.InstanceId == InstanceId)
		{
			OutInstance = Inst;
			OutZone = EZoneKind::BattleDeck;
			OutZoneOwnerInstanceId = FGuid();
			return true;
		}
	}

	for (const FCardInstance& Inst : State.BurdenZone)
	{
		if (Inst.InstanceId == InstanceId)
		{
			OutInstance = Inst;
			OutZone = EZoneKind::BurdenZone;
			OutZoneOwnerInstanceId = FGuid();
			return true;
		}
	}

	for (const FSpecialZone& SZ : State.SpecialZones)
	{
		for (const FCardInstance& Inst : SZ.Cards)
		{
			if (Inst.InstanceId == InstanceId)
			{
				OutInstance = Inst;
				OutZone = EZoneKind::SpecialZone;
				OutZoneOwnerInstanceId = SZ.OwnerInstanceId;
				return true;
			}
		}
	}

	return false;
}

bool FRunDeckRules::GetSpecialZone(const FRunState& State, FGuid OwnerInstanceId, FSpecialZone& Out)
{
	for (const FSpecialZone& SZ : State.SpecialZones)
	{
		if (SZ.OwnerInstanceId == OwnerInstanceId)
		{
			Out = SZ;
			return true;
		}
	}
	return false;
}

int32 FRunDeckRules::GetSpecialZoneCapacityFor(const FRunState& State, FGuid OwnerInstanceId)
{
	if (!OwnerInstanceId.IsValid())
	{
		return 0;
	}

	bool bFoundEntry = false;
	for (const FSpecialZone& SZ : State.SpecialZones)
	{
		if (SZ.OwnerInstanceId == OwnerInstanceId)
		{
			bFoundEntry = true;
			break;
		}
	}
	if (!bFoundEntry)
	{
		return 0;
	}

	FCardInstance OwnerInst;
	EZoneKind OwnerZone = EZoneKind::Backpack;
	FGuid OwnerSelfOwner;
	if (!FindInstance(State, OwnerInstanceId, OwnerInst, OwnerZone, OwnerSelfOwner))
	{
		return 0;
	}

	return GetSpecialZoneCapacity(OwnerInst.Definition);
}

void FRunDeckRules::CollectTypeBContainers(const FRunState& State, TArray<FGuid>& OutOwnerInstanceIds)
{
	OutOwnerInstanceIds.Reset();

	TSet<FGuid> Seen;
	for (const FSpecialZone& SZ : State.SpecialZones)
	{
		const FGuid& OwnerId = SZ.OwnerInstanceId;
		if (!OwnerId.IsValid() || Seen.Contains(OwnerId))
		{
			continue;
		}

		FCardInstance OwnerInst;
		EZoneKind OwnerZone = EZoneKind::Backpack;
		FGuid OwnerSelfOwner;
		if (!FindInstance(State, OwnerId, OwnerInst, OwnerZone, OwnerSelfOwner))
		{
			continue;
		}

		OutOwnerInstanceIds.Add(OwnerId);
		Seen.Add(OwnerId);
	}
}

bool FRunDeckRules::FindFirstOwnedCardDefinition(const FRunState& State, const UCardDefinition* Card, FRunOwnedCardLocation& OutLocation)
{
	OutLocation = FRunOwnedCardLocation{};
	if (!Card)
	{
		return false;
	}

	auto FindInPile = [Card, &OutLocation](const TArray<FCardInstance>& Pile, EZoneKind Zone, FGuid ZoneOwnerInstanceId) -> bool
	{
		for (int32 i = 0; i < Pile.Num(); ++i)
		{
			if (Pile[i].Definition == Card)
			{
				OutLocation.Instance = Pile[i];
				OutLocation.Zone = Zone;
				OutLocation.ZoneOwnerInstanceId = ZoneOwnerInstanceId;
				OutLocation.CardIndex = i;
				return true;
			}
		}
		return false;
	};

	if (FindInPile(State.Backpack, EZoneKind::Backpack, FGuid()))
	{
		return true;
	}
	if (FindInPile(State.BattleDeck, EZoneKind::BattleDeck, FGuid()))
	{
		return true;
	}
	if (FindInPile(State.BurdenZone, EZoneKind::BurdenZone, FGuid()))
	{
		return true;
	}

	for (const FSpecialZone& SpecialZone : State.SpecialZones)
	{
		if (FindInPile(SpecialZone.Cards, EZoneKind::SpecialZone, SpecialZone.OwnerInstanceId))
		{
			return true;
		}
	}

	return false;
}

bool FRunDeckRules::FindOwnedCardInstance(const FRunState& State, FGuid InstanceId, FRunOwnedCardLocation& OutLocation)
{
	OutLocation = FRunOwnedCardLocation{};
	if (!InstanceId.IsValid())
	{
		return false;
	}

	auto FindInPile = [InstanceId, &OutLocation](const TArray<FCardInstance>& Pile, EZoneKind Zone, FGuid ZoneOwnerInstanceId) -> bool
	{
		for (int32 i = 0; i < Pile.Num(); ++i)
		{
			if (Pile[i].InstanceId == InstanceId)
			{
				OutLocation.Instance = Pile[i];
				OutLocation.Zone = Zone;
				OutLocation.ZoneOwnerInstanceId = ZoneOwnerInstanceId;
				OutLocation.CardIndex = i;
				return true;
			}
		}
		return false;
	};

	if (FindInPile(State.Backpack, EZoneKind::Backpack, FGuid()))
	{
		return true;
	}
	if (FindInPile(State.BattleDeck, EZoneKind::BattleDeck, FGuid()))
	{
		return true;
	}
	if (FindInPile(State.BurdenZone, EZoneKind::BurdenZone, FGuid()))
	{
		return true;
	}

	for (const FSpecialZone& SpecialZone : State.SpecialZones)
	{
		if (FindInPile(SpecialZone.Cards, EZoneKind::SpecialZone, SpecialZone.OwnerInstanceId))
		{
			return true;
		}
	}

	return false;
}

bool FRunDeckRules::DoesRunOwnCardDefinition(const FRunState& State, const UCardDefinition* Card)
{
	FRunOwnedCardLocation Location;
	return FindFirstOwnedCardDefinition(State, Card, Location);
}

bool FRunDeckRules::HasCapacityProviderAfterDestroyingFirstOwnedInstance(const FRunState& State, const UCardDefinition* Card)
{
	FRunOwnedCardLocation TargetLocation;
	if (!FindFirstOwnedCardDefinition(State, Card, TargetLocation))
	{
		return false;
	}

	return HasCapacityProviderAfterDestroyingOwnedInstance(State, TargetLocation.Instance.InstanceId);
}

bool FRunDeckRules::HasCapacityProviderAfterDestroyingOwnedInstance(const FRunState& State, FGuid InstanceId)
{
	FRunOwnedCardLocation TargetLocation;
	if (!FindOwnedCardInstance(State, InstanceId, TargetLocation))
	{
		return false;
	}

	bool bSkippedTargetInstance = false;
	auto HasProviderAfterSkippingTarget = [&TargetLocation, &bSkippedTargetInstance](const TArray<FCardInstance>& Pile) -> bool
	{
		for (const FCardInstance& Inst : Pile)
		{
			const bool bIsTargetInstance = TargetLocation.Instance.InstanceId.IsValid()
				? Inst.InstanceId == TargetLocation.Instance.InstanceId
				: false;
			if (bIsTargetInstance)
			{
				bSkippedTargetInstance = true;
				continue;
			}
			if (IsContainerCard(Inst.Definition))
			{
				return true;
			}
		}
		return false;
	};

	if (HasProviderAfterSkippingTarget(State.Backpack)
		|| HasProviderAfterSkippingTarget(State.BattleDeck)
		|| HasProviderAfterSkippingTarget(State.BurdenZone))
	{
		return true;
	}

	for (const FSpecialZone& SpecialZone : State.SpecialZones)
	{
		if (HasProviderAfterSkippingTarget(SpecialZone.Cards))
		{
			return true;
		}
	}

	return false;
}

FRunDeckOperationValidation FRunDeckRules::ValidatePermanentRemoveCard(const FRunState& State, const UCardDefinition* Card)
{
	FRunDeckOperationValidation Result;
	Result.DisabledReason = DeckReasons::Unknown();

	if (!Card)
	{
		Result.DisabledReason = DeckReasons::MissingCard();
		return Result;
	}

	if (!DoesRunOwnCardDefinition(State, Card))
	{
		Result.DisabledReason = DeckReasons::CardNotOwned();
		return Result;
	}

	if (Card->Rarity.MatchesTagExact(WacomTags::Card_Rarity_Intrinsic))
	{
		Result.DisabledReason = DeckReasons::Intrinsic();
		return Result;
	}

	if (IsContainerCard(Card) && !HasCapacityProviderAfterDestroyingFirstOwnedInstance(State, Card))
	{
		Result.DisabledReason = DeckReasons::LastCapacityProvider();
		return Result;
	}

	Result.bCanExecute = true;
	Result.DisabledReason = NAME_None;
	return Result;
}

FRunDeckOperationValidation FRunDeckRules::ValidatePermanentRemoveInstance(const FRunState& State, FGuid InstanceId)
{
	FRunDeckOperationValidation Result;
	Result.DisabledReason = DeckReasons::Unknown();

	FRunOwnedCardLocation Location;
	if (!FindOwnedCardInstance(State, InstanceId, Location))
	{
		Result.DisabledReason = DeckReasons::CardNotOwned();
		return Result;
	}

	const UCardDefinition* Card = Location.Instance.Definition;
	if (!Card)
	{
		Result.DisabledReason = DeckReasons::MissingCard();
		return Result;
	}

	if (Card->Rarity.MatchesTagExact(WacomTags::Card_Rarity_Intrinsic))
	{
		Result.DisabledReason = DeckReasons::Intrinsic();
		return Result;
	}

	if (IsContainerCard(Card) && !HasCapacityProviderAfterDestroyingOwnedInstance(State, InstanceId))
	{
		Result.DisabledReason = DeckReasons::LastCapacityProvider();
		return Result;
	}

	Result.bCanExecute = true;
	Result.DisabledReason = NAME_None;
	return Result;
}

bool FRunDeckRules::PermanentRemoveOwnedCard(FRunState& State, UCardDefinition* Card, FName* OutDisabledReason)
{
	if (OutDisabledReason)
	{
		*OutDisabledReason = NAME_None;
	}

	FRunOwnedCardLocation Location;
	if (!FindFirstOwnedCardDefinition(State, Card, Location))
	{
		if (OutDisabledReason)
		{
			*OutDisabledReason = Card ? DeckReasons::CardNotOwned() : DeckReasons::MissingCard();
		}
		return false;
	}

	return PermanentRemoveOwnedInstance(State, Location.Instance.InstanceId, OutDisabledReason);
}

bool FRunDeckRules::PermanentRemoveOwnedInstance(FRunState& State, FGuid InstanceId, FName* OutDisabledReason)
{
	if (OutDisabledReason)
	{
		*OutDisabledReason = NAME_None;
	}

	const FRunDeckOperationValidation Validation = ValidatePermanentRemoveInstance(State, InstanceId);
	if (!Validation.bCanExecute)
	{
		if (OutDisabledReason)
		{
			*OutDisabledReason = Validation.DisabledReason;
		}
		UE_LOG(LogTemp, Warning,
			TEXT("[RunDeckRules] PermanentRemoveOwnedInstance: 拒绝 InstanceId=%s Reason=%s"),
			*InstanceId.ToString(),
			*Validation.DisabledReason.ToString());
		return false;
	}

	FRunOwnedCardLocation Location;
	if (!FindOwnedCardInstance(State, InstanceId, Location))
	{
		if (OutDisabledReason)
		{
			*OutDisabledReason = DeckReasons::CardNotOwned();
		}
		return false;
	}

	FCardInstance RemovedInst = Location.Instance;
	UCardDefinition* Card = RemovedInst.Definition.Get();
	switch (Location.Zone)
	{
	case EZoneKind::Backpack:
		if (!State.Backpack.IsValidIndex(Location.CardIndex))
		{
			if (OutDisabledReason) { *OutDisabledReason = DeckReasons::CardNotOwned(); }
			return false;
		}
		State.Backpack.RemoveAt(Location.CardIndex);
		break;

	case EZoneKind::BattleDeck:
		if (!State.BattleDeck.IsValidIndex(Location.CardIndex))
		{
			if (OutDisabledReason) { *OutDisabledReason = DeckReasons::CardNotOwned(); }
			return false;
		}
		State.BattleDeck.RemoveAt(Location.CardIndex);
		break;

	case EZoneKind::BurdenZone:
		if (!State.BurdenZone.IsValidIndex(Location.CardIndex))
		{
			if (OutDisabledReason) { *OutDisabledReason = DeckReasons::CardNotOwned(); }
			return false;
		}
		State.BurdenZone.RemoveAt(Location.CardIndex);
		break;

	case EZoneKind::SpecialZone:
	{
		const int32 SpecialZoneIndex = State.SpecialZones.IndexOfByPredicate(
			[&Location](const FSpecialZone& SpecialZone)
			{
				return SpecialZone.OwnerInstanceId == Location.ZoneOwnerInstanceId;
			});
		if (SpecialZoneIndex == INDEX_NONE || !State.SpecialZones[SpecialZoneIndex].Cards.IsValidIndex(Location.CardIndex))
		{
			if (OutDisabledReason) { *OutDisabledReason = DeckReasons::CardNotOwned(); }
			return false;
		}
		State.SpecialZones[SpecialZoneIndex].Cards.RemoveAt(Location.CardIndex);
		break;
	}

	default:
		if (OutDisabledReason)
		{
			*OutDisabledReason = DeckReasons::CardNotOwned();
		}
		return false;
	}

	if (Card && Card->Keywords.HasTagExact(WacomTags::Card_Keyword_Companion))
	{
		State.Pressure.Add(EWacomPressureType::Bloodlust, 1);
	}

	if (IsTypeBContainerCard(Card) && RemovedInst.InstanceId.IsValid())
	{
		const int32 SpecialZoneIndex = State.SpecialZones.IndexOfByPredicate(
			[&RemovedInst](const FSpecialZone& SpecialZone)
			{
				return SpecialZone.OwnerInstanceId == RemovedInst.InstanceId;
			});
		if (SpecialZoneIndex != INDEX_NONE)
		{
			TArray<FCardInstance> CardsToReturn = MoveTemp(State.SpecialZones[SpecialZoneIndex].Cards);
			State.SpecialZones.RemoveAt(SpecialZoneIndex);

			const int32 FluxCapacity = SumOwnedCardCapacity(State, /*bTypeAOnly=*/true);
			for (FCardInstance& Inst : CardsToReturn)
			{
				Inst.bBattleEnabledInSpecialZone = false;
				if (CountFluxContentCards(State.Backpack) < FluxCapacity)
				{
					State.Backpack.Add(MoveTemp(Inst));
				}
				else
				{
					State.BurdenZone.Add(MoveTemp(Inst));
				}
			}
		}
	}

	RecomputeBurden(State, /*bAllowBurdenRefill=*/!IsContainerCard(Card));
	UE_LOG(LogTemp, Display,
		TEXT("[RunDeckRules] PermanentRemoveOwnedInstance: %s (%s), Backpack=%d, BattleDeck=%d, Burden=%d"),
		*GetNameSafe(Card),
		*InstanceId.ToString(),
		State.Backpack.Num(),
		State.BattleDeck.Num(),
		State.BurdenZone.Num());
	return true;
}

int32 FRunDeckRules::GetDeleteGoldRewardForCard(const UCardDefinition* Card)
{
	if (!Card)
	{
		return 0;
	}
	if (Card->Rarity.MatchesTagExact(WacomTags::Card_Rarity_White))
	{
		return 1;
	}
	if (Card->Rarity.MatchesTagExact(WacomTags::Card_Rarity_Blue))
	{
		return 2;
	}
	return 0;
}

int32 FRunDeckRules::SumOwnedCardCapacity(const FRunState& State, bool bTypeAOnly)
{
	auto ShouldCount = [bTypeAOnly](const UCardDefinition* Card)
	{
		return bTypeAOnly
			? FRunDeckRules::IsTypeAContainerCard(Card)
			: FRunDeckRules::IsContainerCard(Card);
	};

	int32 Sum = 0;
	auto AccumulatePile = [&Sum, &ShouldCount](const TArray<FCardInstance>& Pile)
	{
		for (const FCardInstance& Inst : Pile)
		{
			if (ShouldCount(Inst.Definition))
			{
				Sum += Inst.Definition->Physique.Capacity;
			}
		}
	};

	AccumulatePile(State.Backpack);
	AccumulatePile(State.BattleDeck);
	AccumulatePile(State.BurdenZone);
	for (const FSpecialZone& SpecialZone : State.SpecialZones)
	{
		AccumulatePile(SpecialZone.Cards);
	}

	return Sum;
}

int32 FRunDeckRules::CountFluxContentCards(const TArray<FCardInstance>& Pile)
{
	int32 Count = 0;
	for (const FCardInstance& Inst : Pile)
	{
		if (IsFluxContentCardDefinition(Inst.Definition))
		{
			++Count;
		}
	}
	return Count;
}

FRunDeckOperationValidation FRunDeckRules::ValidateMoveInstance(const FRunState& State, FGuid InstanceId, EZoneKind ToZone, FGuid ToZoneOwnerInstanceId)
{
	FRunDeckOperationValidation Result;
	Result.DisabledReason = DeckReasons::Unknown();

	FCardInstance Found;
	EZoneKind FromZone = EZoneKind::Backpack;
	FGuid FromZoneOwnerInstanceId;
	if (!FindInstance(State, InstanceId, Found, FromZone, FromZoneOwnerInstanceId))
	{
		Result.DisabledReason = DeckReasons::CardNotFound();
		return Result;
	}

	switch (ToZone)
	{
	case EZoneKind::Backpack:
	{
		if (IsFluxContentCardDefinition(Found.Definition))
		{
			const int32 CurrentCount = CountFluxContentCards(State.Backpack);
			const bool bInPlaceBackpack = FromZone == EZoneKind::Backpack;
			const int32 EffectiveCount = bInPlaceBackpack ? (CurrentCount - 1) : CurrentCount;
			if (EffectiveCount >= SumOwnedCardCapacity(State, /*bTypeAOnly=*/true))
			{
				Result.DisabledReason = DeckReasons::FluxFull();
				return Result;
			}
		}
		break;
	}

	case EZoneKind::BattleDeck:
	{
		const int32 EffectiveCount = (FromZone == EZoneKind::BattleDeck)
			? (State.BattleDeck.Num() - 1)
			: State.BattleDeck.Num();
		if (EffectiveCount >= SumOwnedCardCapacity(State, /*bTypeAOnly=*/false))
		{
			Result.DisabledReason = DeckReasons::BattleDeckFull();
			return Result;
		}
		break;
	}

	case EZoneKind::SpecialZone:
	{
		const int32 ToSZIdx = State.SpecialZones.IndexOfByPredicate(
			[&](const FSpecialZone& SZ) { return SZ.OwnerInstanceId == ToZoneOwnerInstanceId; });
		if (ToSZIdx == INDEX_NONE)
		{
			Result.DisabledReason = DeckReasons::SpecialZoneMissing();
			return Result;
		}
		if (InstanceId == ToZoneOwnerInstanceId)
		{
			Result.DisabledReason = DeckReasons::SelfSpecialZone();
			return Result;
		}
		if (IsTypeBContainerCard(Found.Definition))
		{
			Result.DisabledReason = DeckReasons::TypeBInSpecialZone();
			return Result;
		}

		const int32 CurrentCount = State.SpecialZones[ToSZIdx].Cards.Num();
		const bool bInPlaceSameSZ =
			(FromZone == EZoneKind::SpecialZone)
			&& (FromZoneOwnerInstanceId == ToZoneOwnerInstanceId);
		const int32 EffectiveCount = bInPlaceSameSZ ? (CurrentCount - 1) : CurrentCount;
		if (EffectiveCount >= GetSpecialZoneCapacityFor(State, ToZoneOwnerInstanceId))
		{
			Result.DisabledReason = DeckReasons::SpecialZoneFull();
			return Result;
		}
		break;
	}

	case EZoneKind::BurdenZone:
		if (IsTypeBContainerCard(Found.Definition))
		{
			Result.DisabledReason = DeckReasons::TypeBInBurdenZone();
			return Result;
		}
		break;

	default:
		Result.DisabledReason = DeckReasons::InvalidTargetZone();
		return Result;
	}

	Result.bCanExecute = true;
	Result.DisabledReason = NAME_None;
	return Result;
}

FRunDeckOperationValidation FRunDeckRules::ValidateSetSpecialZoneCardBattleEnabled(
	const FRunState& State,
	FGuid InstanceId,
	bool /*bEnabled*/)
{
	return ValidateToggleSpecialZoneCardBattleEnabled(State, InstanceId);
}

FRunDeckOperationValidation FRunDeckRules::ValidateToggleSpecialZoneCardBattleEnabled(
	const FRunState& State,
	FGuid InstanceId)
{
	FRunDeckOperationValidation Result;
	Result.DisabledReason = DeckReasons::Unknown();

	FCardInstance Found;
	EZoneKind Zone = EZoneKind::Backpack;
	FGuid ZoneOwnerInstanceId;
	if (!FindInstance(State, InstanceId, Found, Zone, ZoneOwnerInstanceId))
	{
		Result.DisabledReason = DeckReasons::CardNotFound();
		return Result;
	}

	if (Zone != EZoneKind::SpecialZone)
	{
		Result.DisabledReason = DeckReasons::NotInSpecialZone();
		return Result;
	}

	Result.bCanExecute = true;
	Result.DisabledReason = NAME_None;
	return Result;
}

bool FRunDeckRules::SetSpecialZoneCardBattleEnabled(
	FRunState& State,
	FGuid InstanceId,
	bool bEnabled,
	FName* OutDisabledReason)
{
	SetSpecialZoneBattleFlagDisabledReason(OutDisabledReason, NAME_None);

	const FRunDeckOperationValidation Validation =
		ValidateSetSpecialZoneCardBattleEnabled(State, InstanceId, bEnabled);
	if (!Validation.bCanExecute)
	{
		SetSpecialZoneBattleFlagDisabledReason(OutDisabledReason, Validation.DisabledReason);
		return false;
	}

	for (FSpecialZone& SpecialZone : State.SpecialZones)
	{
		for (FCardInstance& Inst : SpecialZone.Cards)
		{
			if (Inst.InstanceId == InstanceId)
			{
				Inst.bBattleEnabledInSpecialZone = bEnabled;
				return true;
			}
		}
	}

	SetSpecialZoneBattleFlagDisabledReason(OutDisabledReason, DeckReasons::CardNotFound());
	return false;
}

bool FRunDeckRules::ToggleSpecialZoneCardBattleEnabled(
	FRunState& State,
	FGuid InstanceId,
	FName* OutDisabledReason)
{
	SetSpecialZoneBattleFlagDisabledReason(OutDisabledReason, NAME_None);

	const FRunDeckOperationValidation Validation =
		ValidateToggleSpecialZoneCardBattleEnabled(State, InstanceId);
	if (!Validation.bCanExecute)
	{
		SetSpecialZoneBattleFlagDisabledReason(OutDisabledReason, Validation.DisabledReason);
		return false;
	}

	for (FSpecialZone& SpecialZone : State.SpecialZones)
	{
		for (FCardInstance& Inst : SpecialZone.Cards)
		{
			if (Inst.InstanceId == InstanceId)
			{
				Inst.bBattleEnabledInSpecialZone = !Inst.bBattleEnabledInSpecialZone;
				return true;
			}
		}
	}

	SetSpecialZoneBattleFlagDisabledReason(OutDisabledReason, DeckReasons::CardNotFound());
	return false;
}

bool FRunDeckRules::MoveInstance(
	FRunState& State,
	FGuid InstanceId,
	EZoneKind ToZone,
	FGuid ToZoneOwnerInstanceId,
	FRunOwnedCardLocation* OutFromLocation,
	FName* OutDisabledReason)
{
	if (OutFromLocation)
	{
		*OutFromLocation = FRunOwnedCardLocation{};
	}
	SetMoveDisabledReason(OutDisabledReason, NAME_None);

	const FRunDeckOperationValidation Validation =
		ValidateMoveInstance(State, InstanceId, ToZone, ToZoneOwnerInstanceId);
	if (!Validation.bCanExecute)
	{
		SetMoveDisabledReason(OutDisabledReason, Validation.DisabledReason);
		return false;
	}

	FRunOwnedCardLocation FromLocation;
	if (!FindOwnedCardInstance(State, InstanceId, FromLocation))
	{
		SetMoveDisabledReason(OutDisabledReason, DeckReasons::CardNotFound());
		return false;
	}

	TArray<FCardInstance>* SourcePile = FindMutableMovePile(
		State,
		FromLocation.Zone,
		FromLocation.ZoneOwnerInstanceId);
	TArray<FCardInstance>* TargetPile = FindMutableMovePile(
		State,
		ToZone,
		ToZoneOwnerInstanceId);
	if (!SourcePile
		|| !SourcePile->IsValidIndex(FromLocation.CardIndex)
		|| (*SourcePile)[FromLocation.CardIndex].InstanceId != InstanceId)
	{
		ensureMsgf(false,
			TEXT("[RunDeckRules] MoveInstance: 源 zone 查找漂移 InstanceId=%s Zone=%d Index=%d"),
			*InstanceId.ToString(),
			(int32)FromLocation.Zone,
			FromLocation.CardIndex);
		SetMoveDisabledReason(OutDisabledReason, DeckReasons::CardNotFound());
		return false;
	}
	if (!TargetPile)
	{
		ensureMsgf(false,
			TEXT("[RunDeckRules] MoveInstance: 目标 zone 查找漂移 ToZone=%d Owner=%s"),
			(int32)ToZone,
			*ToZoneOwnerInstanceId.ToString());
		SetMoveDisabledReason(OutDisabledReason, DeckReasons::InvalidTargetZone());
		return false;
	}

	FCardInstance Found = (*SourcePile)[FromLocation.CardIndex];
	SourcePile->RemoveAt(FromLocation.CardIndex);

	const bool bSameSpecialZoneMove =
		FromLocation.Zone == EZoneKind::SpecialZone
		&& ToZone == EZoneKind::SpecialZone
		&& FromLocation.ZoneOwnerInstanceId == ToZoneOwnerInstanceId;
	if (!bSameSpecialZoneMove
		&& (FromLocation.Zone == EZoneKind::SpecialZone || ToZone == EZoneKind::SpecialZone))
	{
		Found.bBattleEnabledInSpecialZone = false;
	}

	TargetPile->Add(Found);

	if (ToZone == EZoneKind::Backpack || ToZone == EZoneKind::BattleDeck)
	{
		EnsureSpecialZoneEntryFor(State, Found);
	}

	RecomputeBurden(State, /*bAllowBurdenRefill=*/ToZone != EZoneKind::BurdenZone);

	if (OutFromLocation)
	{
		*OutFromLocation = FromLocation;
	}
	return true;
}

FRunDeckZoneAddress FRunDeckRules::NormalizeZoneAddress(const FRunDeckZoneAddress& Address)
{
	FRunDeckZoneAddress Normalized = Address;
	if (Normalized.Zone != EZoneKind::SpecialZone)
	{
		Normalized.OwnerInstanceId.Invalidate();
	}
	return Normalized;
}

FRunDeckBatchOperationValidation FRunDeckRules::ValidateBatchInstanceSet(
	const FRunState& State,
	uint64 CurrentStorageRevision,
	const TArray<FGuid>& InstanceIds,
	const FRunDeckZoneAddress& ExpectedSource,
	uint64 ExpectedStorageRevision)
{
	FRunDeckBatchOperationValidation Result;
	Result.RequestedCount = InstanceIds.Num();
	Result.ValidatedStorageRevision = CurrentStorageRevision;
	if (InstanceIds.IsEmpty())
	{
		Result.DisabledReason = DeckReasons::EmptyBatchRequest();
		return Result;
	}
	if (ExpectedStorageRevision != CurrentStorageRevision)
	{
		Result.DisabledReason = DeckReasons::StaleStorageRevision();
		return Result;
	}

	const FRunDeckZoneAddress Source = NormalizeZoneAddress(ExpectedSource);
	if (Source.Zone == EZoneKind::SpecialZone)
	{
		if (!Source.OwnerInstanceId.IsValid()
			|| State.SpecialZones.IndexOfByPredicate(
				[&Source](const FSpecialZone& Zone) { return Zone.OwnerInstanceId == Source.OwnerInstanceId; }) == INDEX_NONE)
		{
			Result.DisabledReason = DeckReasons::SpecialZoneMissing();
			return Result;
		}
	}

	TSet<FGuid> UniqueIds;
	for (const FGuid InstanceId : InstanceIds)
	{
		if (!InstanceId.IsValid())
		{
			Result.DisabledReason = DeckReasons::CardNotFound();
			return Result;
		}
		if (UniqueIds.Contains(InstanceId))
		{
			Result.DisabledReason = DeckReasons::DuplicateInstanceId();
			return Result;
		}
		UniqueIds.Add(InstanceId);

		FRunOwnedCardLocation Location;
		if (!FindOwnedCardInstance(State, InstanceId, Location) || !Location.Instance.Definition)
		{
			Result.DisabledReason = DeckReasons::CardNotFound();
			return Result;
		}
		FRunDeckZoneAddress Actual;
		Actual.Zone = Location.Zone;
		Actual.OwnerInstanceId = Location.ZoneOwnerInstanceId;
		Actual = NormalizeZoneAddress(Actual);
		if (Actual.Zone != Source.Zone || Actual.OwnerInstanceId != Source.OwnerInstanceId)
		{
			Result.DisabledReason = DeckReasons::SourceZoneMismatch();
			return Result;
		}
	}

	Result.bCanExecute = true;
	Result.DisabledReason = NAME_None;
	return Result;
}

FRunDeckBatchOperationValidation FRunDeckRules::ValidateMoveInstancesAtomic(
	const FRunState& State,
	uint64 CurrentStorageRevision,
	const FRunDeckBatchMoveRequest& Request)
{
	FRunDeckBatchOperationValidation Result = ValidateBatchInstanceSet(
		State,
		CurrentStorageRevision,
		Request.InstanceIds,
		Request.ExpectedSource,
		Request.ExpectedStorageRevision);
	if (!Result.bCanExecute)
	{
		return Result;
	}
	const FRunDeckZoneAddress Source = NormalizeZoneAddress(Request.ExpectedSource);
	const FRunDeckZoneAddress Target = NormalizeZoneAddress(Request.Target);
	if (Target.Zone == EZoneKind::SpecialZone && !Target.OwnerInstanceId.IsValid())
	{
		Result.bCanExecute = false;
		Result.DisabledReason = DeckReasons::SpecialZoneMissing();
		return Result;
	}
	if (Source.Zone == Target.Zone && Source.OwnerInstanceId == Target.OwnerInstanceId)
	{
		Result.bCanExecute = false;
		Result.DisabledReason = DeckReasons::SameZoneBatch();
		return Result;
	}

	FRunState WorkingState = State;
	for (const FGuid InstanceId : Request.InstanceIds)
	{
		FName DisabledReason = NAME_None;
		if (!MoveInstance(WorkingState, InstanceId, Target.Zone, Target.OwnerInstanceId, nullptr, &DisabledReason))
		{
			Result.bCanExecute = false;
			Result.DisabledReason = DisabledReason.IsNone() ? DeckReasons::Unknown() : DisabledReason;
			return Result;
		}
	}
	return Result;
}

FRunDeckBatchOperationResult FRunDeckRules::ApplyMoveInstancesAtomic(
	FRunState& InOutWorkingState,
	uint64 CurrentStorageRevision,
	const FRunDeckBatchMoveRequest& Request)
{
	FRunDeckBatchOperationResult Result;
	Result.StorageRevision = CurrentStorageRevision;
	const FRunDeckBatchOperationValidation Validation = ValidateMoveInstancesAtomic(
		InOutWorkingState,
		CurrentStorageRevision,
		Request);
	if (!Validation.bCanExecute)
	{
		Result.DisabledReason = Validation.DisabledReason;
		return Result;
	}
	const FRunDeckZoneAddress Target = NormalizeZoneAddress(Request.Target);
	for (const FGuid InstanceId : Request.InstanceIds)
	{
		FName DisabledReason = NAME_None;
		if (!MoveInstance(InOutWorkingState, InstanceId, Target.Zone, Target.OwnerInstanceId, nullptr, &DisabledReason))
		{
			Result.DisabledReason = DisabledReason.IsNone() ? DeckReasons::Unknown() : DisabledReason;
			return Result;
		}
	}
	Result.bSucceeded = true;
	Result.DisabledReason = NAME_None;
	Result.AffectedCount = Request.InstanceIds.Num();
	return Result;
}

FRunDeckBatchDeletePreview FRunDeckRules::ValidateDeleteCardsForGoldAtomic(
	const FRunState& State,
	uint64 CurrentStorageRevision,
	const FRunDeckBatchDeleteRequest& Request)
{
	FRunDeckBatchDeletePreview Preview;
	Preview.Validation = ValidateBatchInstanceSet(
		State,
		CurrentStorageRevision,
		Request.InstanceIds,
		Request.ExpectedSource,
		Request.ExpectedStorageRevision);
	if (!Preview.Validation.bCanExecute)
	{
		return Preview;
	}

	FRunState WorkingState = State;
	for (const FGuid InstanceId : Request.InstanceIds)
	{
		FRunOwnedCardLocation Location;
		if (!FindOwnedCardInstance(WorkingState, InstanceId, Location) || !Location.Instance.Definition)
		{
			Preview.Validation.bCanExecute = false;
			Preview.Validation.DisabledReason = DeckReasons::CardNotFound();
			Preview.TotalGoldReward = 0;
			return Preview;
		}
		Preview.TotalGoldReward += GetDeleteGoldRewardForCard(Location.Instance.Definition);
		FName DisabledReason = NAME_None;
		if (!PermanentRemoveOwnedInstance(WorkingState, InstanceId, &DisabledReason))
		{
			Preview.Validation.bCanExecute = false;
			Preview.Validation.DisabledReason = DisabledReason.IsNone() ? DeckReasons::Unknown() : DisabledReason;
			Preview.TotalGoldReward = 0;
			return Preview;
		}
	}
	return Preview;
}

FRunDeckBatchOperationResult FRunDeckRules::ApplyDeleteCardsForGoldAtomic(
	FRunState& InOutWorkingState,
	uint64 CurrentStorageRevision,
	const FRunDeckBatchDeleteRequest& Request)
{
	FRunDeckBatchOperationResult Result;
	Result.StorageRevision = CurrentStorageRevision;
	const FRunDeckBatchDeletePreview Preview = ValidateDeleteCardsForGoldAtomic(
		InOutWorkingState,
		CurrentStorageRevision,
		Request);
	if (!Preview.Validation.bCanExecute)
	{
		Result.DisabledReason = Preview.Validation.DisabledReason;
		return Result;
	}
	for (const FGuid InstanceId : Request.InstanceIds)
	{
		FName DisabledReason = NAME_None;
		if (!PermanentRemoveOwnedInstance(InOutWorkingState, InstanceId, &DisabledReason))
		{
			Result.DisabledReason = DisabledReason.IsNone() ? DeckReasons::Unknown() : DisabledReason;
			return Result;
		}
	}
	Result.bSucceeded = true;
	Result.DisabledReason = NAME_None;
	Result.AffectedCount = Request.InstanceIds.Num();
	Result.GoldReward = Preview.TotalGoldReward;
	return Result;
}

void FRunDeckRules::RecomputeBurden(FRunState& State, bool bAllowBurdenRefill)
{
	{
		auto PopFluxContent = [&State](TArray<FCardInstance>& Pile) -> bool
		{
			for (int32 i = Pile.Num() - 1; i >= 0; --i)
			{
				if (IsPreferredBurdenOverflowCandidate(Pile[i].Definition))
				{
					State.BurdenZone.Add(Pile[i]);
					Pile.RemoveAt(i);
					return true;
				}
			}
			for (int32 i = Pile.Num() - 1; i >= 0; --i)
			{
				if (IsFluxContentCardDefinition(Pile[i].Definition))
				{
					State.BurdenZone.Add(Pile[i]);
					Pile.RemoveAt(i);
					return true;
				}
			}
			return false;
		};

		auto CanMoveOverflowCardToFlux = [&State](const FCardInstance& Instance) -> bool
		{
			if (!Instance.Definition)
			{
				return false;
			}
			if (IsTypeBContainerCard(Instance.Definition))
			{
				return true;
			}
			return IsFluxContentCardDefinition(Instance.Definition)
				&& CountFluxContentCards(State.Backpack) < SumOwnedCardCapacity(State, /*bTypeAOnly=*/true);
		};

		auto MoveBattleDeckOverflowAt = [&State, &CanMoveOverflowCardToFlux](TArray<FCardInstance>& Pile, int32 Index) -> bool
		{
			if (!Pile.IsValidIndex(Index))
			{
				return false;
			}

			FCardInstance Instance = Pile[Index];
			Pile.RemoveAt(Index);

			if (CanMoveOverflowCardToFlux(Instance))
			{
				State.Backpack.Add(Instance);
				EnsureSpecialZoneEntryFor(State, Instance);
				return true;
			}

			if (IsTypeBContainerCard(Instance.Definition))
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[RunSession] RecomputeBurden: BattleDeck 溢出卡 %s 是 B 主卡且无法进入通量区，临时保持在 BattleDeck 外不可行"),
					*GetNameSafe(Instance.Definition));
				Pile.Insert(Instance, Index);
				return false;
			}

			State.BurdenZone.Add(Instance);
			return true;
		};

		auto PopBattleDeckOverflow = [&MoveBattleDeckOverflowAt](TArray<FCardInstance>& Pile) -> bool
		{
			for (int32 i = Pile.Num() - 1; i >= 0; --i)
			{
				if (IsPreferredBurdenOverflowCandidate(Pile[i].Definition))
				{
					return MoveBattleDeckOverflowAt(Pile, i);
				}
			}
			for (int32 i = Pile.Num() - 1; i >= 0; --i)
			{
				if (!IsTypeBContainerCard(Pile[i].Definition))
				{
					return MoveBattleDeckOverflowAt(Pile, i);
				}
			}
			return false;
		};

		while (CountFluxContentCards(State.Backpack) > SumOwnedCardCapacity(State, /*bTypeAOnly=*/true))
		{
			if (!PopFluxContent(State.Backpack))
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[RunSession] RecomputeBurden: Backpack 没有可溢出的通量内容卡（Content=%d > FluxCapacity=%d），临时超容"),
					CountFluxContentCards(State.Backpack), SumOwnedCardCapacity(State, /*bTypeAOnly=*/true));
				break;
			}
		}
		while (State.BattleDeck.Num() > SumOwnedCardCapacity(State, /*bTypeAOnly=*/false))
		{
			if (!PopBattleDeckOverflow(State.BattleDeck))
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[RunSession] RecomputeBurden: BattleDeck 全是 B 主卡 instance（Num=%d > BattleDeckCapacity=%d），无法溢出，临时超容"),
					State.BattleDeck.Num(), SumOwnedCardCapacity(State, /*bTypeAOnly=*/false));
				break;
			}
		}
	}

	while (bAllowBurdenRefill && State.BurdenZone.Num() > 0)
	{
		FCardInstance Instance = State.BurdenZone[0];

		if (IsTypeBContainerCard(Instance.Definition))
		{
			State.Backpack.Add(Instance);
			EnsureSpecialZoneEntryFor(State, Instance);
			State.BurdenZone.RemoveAt(0);
			continue;
		}
		if (IsFluxContentCardDefinition(Instance.Definition)
			&& CountFluxContentCards(State.Backpack) < SumOwnedCardCapacity(State, /*bTypeAOnly=*/true))
		{
			State.Backpack.Add(Instance);
			EnsureSpecialZoneEntryFor(State, Instance);
			State.BurdenZone.RemoveAt(0);
			continue;
		}

		if (State.BattleDeck.Num() < SumOwnedCardCapacity(State, /*bTypeAOnly=*/false))
		{
			State.BattleDeck.Add(Instance);
			State.BurdenZone.RemoveAt(0);
			continue;
		}

		int32 PickedIdx = INDEX_NONE;
		for (int32 i = 0; i < State.SpecialZones.Num(); ++i)
		{
			const FSpecialZone& SZ = State.SpecialZones[i];
			const int32 Capacity = GetSpecialZoneCapacityFor(State, SZ.OwnerInstanceId);
			if (Capacity <= 0)
			{
				continue;
			}
			if (SZ.Cards.Num() < Capacity)
			{
				PickedIdx = i;
				break;
			}
		}

		if (PickedIdx != INDEX_NONE)
		{
			Instance.bBattleEnabledInSpecialZone = false;
			State.SpecialZones[PickedIdx].Cards.Add(Instance);
			State.BurdenZone.RemoveAt(0);
			continue;
		}

		break;
	}

	const int32 N = State.BurdenZone.Num();
	const int32 Pressure = FMath::Clamp(N * (N + 1) / 2, 0, 100);
	State.Pressure.Set(EWacomPressureType::Burden, Pressure);
}
