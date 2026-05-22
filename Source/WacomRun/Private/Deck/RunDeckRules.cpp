// Copyright Wacom. All Rights Reserved.

#include "Deck/RunDeckRules.h"

#include "Cards/CardDefinition.h"
#include "RunState.h"

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
	Result.DisabledReason = TEXT("Unknown");

	FCardInstance Found;
	EZoneKind FromZone = EZoneKind::Backpack;
	FGuid FromZoneOwnerInstanceId;
	if (!FindInstance(State, InstanceId, Found, FromZone, FromZoneOwnerInstanceId))
	{
		Result.DisabledReason = TEXT("CardNotFound");
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
				Result.DisabledReason = TEXT("FluxFull");
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
			Result.DisabledReason = TEXT("BattleDeckFull");
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
			Result.DisabledReason = TEXT("SpecialZoneMissing");
			return Result;
		}
		if (InstanceId == ToZoneOwnerInstanceId)
		{
			Result.DisabledReason = TEXT("SelfSpecialZone");
			return Result;
		}
		if (IsTypeBContainerCard(Found.Definition))
		{
			Result.DisabledReason = TEXT("TypeBInSpecialZone");
			return Result;
		}

		const int32 CurrentCount = State.SpecialZones[ToSZIdx].Cards.Num();
		const bool bInPlaceSameSZ =
			(FromZone == EZoneKind::SpecialZone)
			&& (FromZoneOwnerInstanceId == ToZoneOwnerInstanceId);
		const int32 EffectiveCount = bInPlaceSameSZ ? (CurrentCount - 1) : CurrentCount;
		if (EffectiveCount >= GetSpecialZoneCapacityFor(State, ToZoneOwnerInstanceId))
		{
			Result.DisabledReason = TEXT("SpecialZoneFull");
			return Result;
		}
		break;
	}

	case EZoneKind::BurdenZone:
		if (IsTypeBContainerCard(Found.Definition))
		{
			Result.DisabledReason = TEXT("TypeBInBurdenZone");
			return Result;
		}
		break;

	default:
		Result.DisabledReason = TEXT("InvalidTargetZone");
		return Result;
	}

	Result.bCanExecute = true;
	Result.DisabledReason = NAME_None;
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
