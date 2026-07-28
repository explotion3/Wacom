// Copyright Wacom. All Rights Reserved.

#include "Cards/CardDefinition.h"

bool UCardDefinition::UsesTierProfiles() const
{
	return TierProfiles.Num() == WacomCardUpgrade::TierCount;
}

bool UCardDefinition::MatchesCardIdOrUpgradeFamily(const FName Candidate) const
{
	return !Candidate.IsNone() && CardId == Candidate;
}

const FWacomCardTierProfile* UCardDefinition::FindTierProfile(
	const EWacomCardUpgradeTier Tier) const
{
	if (!UsesTierProfiles())
	{
		return nullptr;
	}
	const int32 Index = WacomCardUpgrade::ToIndex(Tier);
	return TierProfiles.IsValidIndex(Index) ? &TierProfiles[Index] : nullptr;
}

FWacomResolvedCardProfile UCardDefinition::ResolveProfile(
	const EWacomCardUpgradeTier Tier) const
{
	FWacomResolvedCardProfile Result;
	Result.UpgradeTier = Tier;
	Result.Rarity = ResolveRarity(Tier);

	if (const FWacomCardTierProfile* Profile = FindTierProfile(Tier))
	{
		Result.bUsesTierProfile = true;
		Result.Description = &Profile->Description;
		Result.BaseCost = Profile->BaseCost;
		Result.BaseCriticalChancePercent =
			FMath::Clamp(Profile->BaseCriticalChancePercent, 0, 100);
		Result.DynamicCostRule = &Profile->DynamicCostRule;
		Result.Physique = &Profile->Physique;
		Result.Effects = &Profile->Effects;
		Result.PerfectReleaseEffects = &Profile->PerfectReleaseEffects;
		Result.ZoneHooks = &Profile->ZoneHooks;
		Result.Passives = &Profile->Passives;
		return Result;
	}

	Result.Description = &Description;
	Result.BaseCost = BaseCost;
	Result.DynamicCostRule = nullptr;
	Result.Physique = &Physique;
	Result.Effects = &Effects;
	Result.PerfectReleaseEffects = &PerfectReleaseEffects;
	Result.ZoneHooks = &ZoneHooks;
	Result.Passives = &Passives;
	return Result;
}

FText UCardDefinition::ResolveDescription(const EWacomCardUpgradeTier Tier) const
{
	const FWacomResolvedCardProfile Profile = ResolveProfile(Tier);
	return Profile.Description ? *Profile.Description : FText::GetEmpty();
}

int32 UCardDefinition::ResolveBaseCost(const EWacomCardUpgradeTier Tier) const
{
	return ResolveProfile(Tier).BaseCost;
}

int32 UCardDefinition::ResolveBaseCriticalChancePercent(
	const EWacomCardUpgradeTier Tier) const
{
	return ResolveProfile(Tier).BaseCriticalChancePercent;
}

const FCardPhysique& UCardDefinition::ResolvePhysique(
	const EWacomCardUpgradeTier Tier) const
{
	return *ResolveProfile(Tier).Physique;
}

const TArray<FCardEffect>& UCardDefinition::ResolveEffects(
	const EWacomCardUpgradeTier Tier) const
{
	return *ResolveProfile(Tier).Effects;
}

const TArray<FCardEffect>& UCardDefinition::ResolvePerfectReleaseEffects(
	const EWacomCardUpgradeTier Tier) const
{
	return *ResolveProfile(Tier).PerfectReleaseEffects;
}

const TArray<FCardZoneHook>& UCardDefinition::ResolveZoneHooks(
	const EWacomCardUpgradeTier Tier) const
{
	return *ResolveProfile(Tier).ZoneHooks;
}

const TArray<FCardPassive>& UCardDefinition::ResolvePassives(
	const EWacomCardUpgradeTier Tier) const
{
	return *ResolveProfile(Tier).Passives;
}

FGameplayTag UCardDefinition::ResolveRarity(const EWacomCardUpgradeTier Tier) const
{
	return UsesTierProfiles() ? WacomCardUpgrade::ResolveRarityTag(Tier) : Rarity;
}

bool UCardDefinition::HasEnabledRunFace() const
{
	return RunFace.bEnabled;
}
