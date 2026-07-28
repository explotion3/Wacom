// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Tags/WacomGameplayTags.h"
#include "Validation/CardDefinitionValidation.h"
#include "Validation/CardTierProfileValidation.h"

#include "UObject/StrongObjectPtr.h"

namespace WacomCardTierProfilesSpec
{
	FCardEffect Damage(const int32 Amount)
	{
		FCardEffect Effect;
		Effect.EffectType = WacomTags::Effect_Damage;
		Effect.Magnitude = Amount;
		Effect.Target = WacomTags::Target_SingleEnemyPart;
		return Effect;
	}

	TStrongObjectPtr<UCardDefinition> MakeTierCard()
	{
		TStrongObjectPtr<UCardDefinition> Card(
			NewObject<UCardDefinition>());
		Card->CardId = TEXT("Card.Test.TierProfile");
		Card->DisplayName = FText::FromString(TEXT("四阶测试"));
		Card->Rarity = WacomTags::Card_Rarity_White;
		Card->TargetMode = ECardTargetMode::SingleEnemyPart;
		for (int32 Tier = 0; Tier < WacomCardUpgrade::TierCount; ++Tier)
		{
			FWacomCardTierProfile& Profile =
				Card->TierProfiles.AddDefaulted_GetRef();
			Profile.Description = FText::FromString(
				FString::Printf(TEXT("阶级 %d"), Tier));
			Profile.BaseCost = 4 - Tier;
			Profile.Physique.MaxHpBonus = Tier * 2;
			Profile.Effects = { Damage(5 + Tier) };
		}
		return Card;
	}

	bool ContainsError(
		const TArray<FText>& Errors,
		const TCHAR* Fragment)
	{
		return Errors.ContainsByPredicate([Fragment](const FText& Error)
		{
			return Error.ToString().Contains(Fragment);
		});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataCardTierProfilesResolutionSpec,
	"Wacom.Data.CardTierProfiles.ResolutionAndLegacyFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataCardTierProfilesResolutionSpec::RunTest(const FString&)
{
	using namespace WacomCardTierProfilesSpec;
	TStrongObjectPtr<UCardDefinition> TierCard = MakeTierCard();
	TArray<FText> Errors;
	TestTrue(TEXT("Four-tier card validates"),
		FWacomCardDefinitionValidation::Validate(TierCard.Get(), Errors));
	TestEqual(TEXT("Four-tier validation has no errors"), Errors.Num(), 0);
	TestTrue(TEXT("Uses tier profiles"), TierCard->UsesTierProfiles());
	const FWacomResolvedCardProfile PurpleProfile =
		TierCard->ResolveProfile(EWacomCardUpgradeTier::Purple);
	TestTrue(TEXT("Unified profile identifies tier-backed data"),
		PurpleProfile.bUsesTierProfile);
	TestEqual(TEXT("Unified profile carries requested tier"),
		PurpleProfile.UpgradeTier, EWacomCardUpgradeTier::Purple);
	TestEqual(TEXT("Unified profile resolves cost"),
		PurpleProfile.BaseCost, 1);
	TestTrue(TEXT("Unified profile exposes effects without copying"),
		PurpleProfile.Effects == &TierCard->TierProfiles[3].Effects);
	TestEqual(TEXT("Yellow cost resolves from one definition"),
		TierCard->ResolveBaseCost(EWacomCardUpgradeTier::Yellow), 2);
	TestEqual(TEXT("Purple damage resolves from one definition"),
		TierCard->ResolveEffects(EWacomCardUpgradeTier::Purple)[0].Magnitude,
		8);
	TestTrue(TEXT("Purple rarity derives from tier"),
		TierCard->ResolveRarity(EWacomCardUpgradeTier::Purple)
			.MatchesTagExact(WacomTags::Card_Rarity_Purple));
	TestTrue(TEXT("Definition identity remains CardId"),
		TierCard->MatchesCardIdOrUpgradeFamily(TierCard->CardId));

	TStrongObjectPtr<UCardDefinition> Legacy(
		NewObject<UCardDefinition>());
	Legacy->CardId = TEXT("Card.Test.LegacyFlat");
	Legacy->BaseCost = 7;
	Legacy->Rarity = WacomTags::Card_Rarity_Blue;
	Legacy->Effects = { Damage(9) };
	TestFalse(TEXT("Flat card is not upgradeable"), Legacy->UsesTierProfiles());
	const FWacomResolvedCardProfile LegacyProfile =
		Legacy->ResolveProfile(EWacomCardUpgradeTier::Purple);
	TestFalse(TEXT("Unified profile marks flat fallback"),
		LegacyProfile.bUsesTierProfile);
	TestTrue(TEXT("Flat fallback points at legacy effects"),
		LegacyProfile.Effects == &Legacy->Effects);
	TestEqual(TEXT("Flat card cost is a White-compatible fallback"),
		Legacy->ResolveBaseCost(EWacomCardUpgradeTier::Purple), 7);
	TestEqual(TEXT("Flat effects remain playable"),
		Legacy->ResolveEffects(EWacomCardUpgradeTier::Blue)[0].Magnitude, 9);
	TestTrue(TEXT("Flat card keeps authored rarity"),
		Legacy->ResolveRarity(EWacomCardUpgradeTier::Purple)
			.MatchesTagExact(WacomTags::Card_Rarity_Blue));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataCardTierProfilesValidationSpec,
	"Wacom.Data.CardTierProfiles.StructuralValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataCardTierProfilesValidationSpec::RunTest(const FString&)
{
	using namespace WacomCardTierProfilesSpec;
	TStrongObjectPtr<UCardDefinition> Card = MakeTierCard();
	TArray<FText> Errors;

	Card->TierProfiles.Pop();
	TestFalse(TEXT("Partial tier array rejects"),
		FWacomCardTierProfileValidation::Validate({ Card.Get() }, Errors));
	TestTrue(TEXT("Partial tier error is explicit"),
		ContainsError(Errors, TEXT("四项")));

	Card = MakeTierCard();
	Card->TierProfiles[2].Effects[0].EffectType =
		WacomTags::Effect_ApplyStatus_Poison;
	Errors.Reset();
	TestFalse(TEXT("Tier effect structure mismatch rejects"),
		FWacomCardDefinitionValidation::Validate(Card.Get(), Errors));
	TestTrue(TEXT("Structure error names Effects"),
		ContainsError(Errors, TEXT("Effects 结构")));

	Card = MakeTierCard();
	Card->TierProfiles[1].BaseCriticalChancePercent = 101;
	Errors.Reset();
	TestFalse(TEXT("Critical chance above 100 rejects"),
		FWacomCardDefinitionValidation::Validate(Card.Get(), Errors));
	TestTrue(TEXT("Critical range error is explicit"),
		ContainsError(Errors, TEXT("[0,100]")));
	return true;
}
