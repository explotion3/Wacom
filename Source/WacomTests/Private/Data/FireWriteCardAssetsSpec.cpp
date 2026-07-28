// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "ContentBuilders/FireWriteCardContentBuilder.h"
#include "Tags/WacomGameplayTags.h"

namespace
{
	UCardDefinition* LoadFireWriteCard(const TCHAR* EnglishName)
	{
		const FString Package = FString::Printf(
			TEXT("/Game/Wacom/Data/Cards/FireWrite/DA_Card_%s"),
			EnglishName);
		return LoadObject<UCardDefinition>(
			nullptr,
			*(Package + TEXT(".DA_Card_") + EnglishName));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataFireWriteSeedContractSpec,
	"Wacom.Data.FireWrite.SeedDefaultsAndAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataFireWriteSeedContractSpec::RunTest(const FString&)
{
	TArray<FString> Errors;
	TestTrue(TEXT("Transient defaults validate"),
		Wacom::ContentBuilder::ValidateFireWriteTransientDefaults(Errors));
	for (const FString& Error : Errors)
	{
		AddError(Error);
	}
	Errors.Reset();
	TestTrue(TEXT("Loaded assets match frozen seed defaults"),
		Wacom::ContentBuilder::ValidateFireWriteLoadedAssets(
			/*bCompareSeedDefaults=*/true,
			Errors));
	for (const FString& Error : Errors)
	{
		AddError(Error);
	}
	TestEqual(TEXT("Exactly 15 FireWrite card packages"),
		Wacom::ContentBuilder::GetFireWriteCardPackagePaths().Num(), 15);

	const UCardDefinition* Candle = LoadFireWriteCard(TEXT("OilCandle"));
	TestNotNull(TEXT("Oil Candle loads"), Candle);
	if (Candle)
	{
		TestEqual(TEXT("Oil Candle stable CardId"), Candle->CardId,
			FName(TEXT("Card.FireWrite.OilCandle")));
		TestEqual(TEXT("Oil Candle has four tiers"),
			Candle->TierProfiles.Num(), 4);
		TestEqual(TEXT("Oil Candle Purple Burn"),
			Candle->ResolveEffects(EWacomCardUpgradeTier::Purple)[0].Magnitude,
			12);
		TestEqual(TEXT("Oil Candle durability"),
			Candle->ResolvePhysique(EWacomCardUpgradeTier::White).Durability,
			3);
	}

	const UCardDefinition* Warm = LoadFireWriteCard(TEXT("WarmTinderbug"));
	TestNotNull(TEXT("Warm Tinderbug loads"), Warm);
	if (Warm)
	{
		const FWacomCardTierProfile* Purple =
			Warm->FindTierProfile(EWacomCardUpgradeTier::Purple);
		TestNotNull(TEXT("Warm Purple profile"), Purple);
		if (Purple)
		{
			TestEqual(TEXT("Warm Purple has two additive aura effects"),
				Purple->Effects.Num(), 2);
			TestTrue(TEXT("Warm dynamic cost counts card Burn"),
				Purple->DynamicCostRule.CountHandCardsWithStatus
					.MatchesTagExact(WacomTags::Status_Burn));
		}
		TestEqual(
			TEXT("Warm detail combines two rule effects into one visible line"),
			Warm->ExplanationTemplates.EffectTemplates.Num(),
			2);
		if (Warm->ExplanationTemplates.EffectTemplates.Num() == 2)
		{
			TestFalse(
				TEXT("Warm primary aura effect remains visible"),
				Warm->ExplanationTemplates.EffectTemplates[0]
					.bSuppressInDetails);
			TestTrue(
				TEXT("Warm conditional helper effect is hidden from details"),
				Warm->ExplanationTemplates.EffectTemplates[1]
					.bSuppressInDetails);
		}
		TestTrue(
			TEXT("Warm dynamic cost has an authored passive sentence"),
			!Warm->ExplanationTemplates.DynamicCostTemplate.IsEmpty());
		TestTrue(
			TEXT("Warm Retain keyword is projected into details"),
			Warm->ExplanationTemplates.KeywordTemplates.ContainsByPredicate(
				[](const FWacomCardKeywordExplanationTemplate& Entry)
				{
					return Entry.Keyword.MatchesTagExact(
						WacomTags::Card_Keyword_Retain);
				}));
	}

	const UCardDefinition* Seed = LoadFireWriteCard(TEXT("FireflySeed"));
	TestNotNull(TEXT("Firefly Seed loads"), Seed);
	if (Seed)
	{
		const auto& OnDraw =
			Seed->ResolvePassives(EWacomCardUpgradeTier::White);
		TestEqual(TEXT("Firefly Seed has one OnDraw passive"),
			OnDraw.Num(), 1);
		if (!OnDraw.IsEmpty() && !OnDraw[0].Effects.IsEmpty())
		{
			TestEqual(TEXT("Random firefly pool has four members"),
				OnDraw[0].Effects[0].CardPool.Num(), 4);
		}
	}

	const UCardDefinition* Bottle = LoadFireWriteCard(TEXT("EmptyBottle"));
	TestNotNull(TEXT("Empty Bottle loads"), Bottle);
	if (Bottle)
	{
		TestTrue(TEXT("Empty Bottle is a Container"),
			Bottle->Keywords.HasTagExact(WacomTags::Card_Keyword_Container));
		TestEqual(TEXT("Empty Bottle Capacity=1"),
			Bottle->ResolvePhysique(EWacomCardUpgradeTier::White).Capacity,
			1);
	}
	return true;
}
