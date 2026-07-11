// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Rules/BattleRuleContentContract.h"
#include "Tags/WacomGameplayTags.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleEffectSemanticRegistrySpec,
	"Wacom.Battle.EffectSemantics.RegistryIsSingleAuthoringTruth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleEffectSemanticRegistrySpec::RunTest(const FString& /*Parameters*/)
{
	const TArray<FGameplayTag> CardEffects =
		FWacomBattleRuleContentContract::GetSupportedCardEffectTypes();
	const TArray<FGameplayTag> IntentEffects =
		FWacomBattleRuleContentContract::GetSupportedEnemyIntentEffectTypes();
	TSet<FGameplayTag> UniqueCardEffects;
	TSet<FGameplayTag> UniqueIntentEffects;
	for (const FGameplayTag& EffectType : CardEffects) { UniqueCardEffects.Add(EffectType); }
	for (const FGameplayTag& EffectType : IntentEffects) { UniqueIntentEffects.Add(EffectType); }

	TestEqual(TEXT("Registry contains every current card effect semantic"), CardEffects.Num(), 20);
	TestEqual(TEXT("Registry contains six enemy-intent effect semantics"), IntentEffects.Num(), 6);
	TestEqual(TEXT("Card registry has no duplicate tags"), UniqueCardEffects.Num(), CardEffects.Num());
	TestEqual(TEXT("Intent registry has no duplicate tags"), UniqueIntentEffects.Num(), IntentEffects.Num());

	for (const FGameplayTag& EffectType : CardEffects)
	{
		TestTrue(
			*FString::Printf(TEXT("Enumerated card effect is authoring-supported: %s"), *EffectType.ToString()),
			FWacomBattleRuleContentContract::IsSupportedCardEffectType(EffectType));
	}
	for (const FGameplayTag& EffectType : IntentEffects)
	{
		TestTrue(
			*FString::Printf(TEXT("Enumerated intent effect is authoring-supported: %s"), *EffectType.ToString()),
			FWacomBattleRuleContentContract::IsSupportedEnemyIntentEffectType(EffectType));
	}

	const FGameplayTag ExpectedEffects[] = {
		WacomTags::Effect_Damage,
		WacomTags::Status_Shield,
		WacomTags::Effect_ApplyStatus_Poison,
		WacomTags::Effect_ApplyStatus_Slow,
		WacomTags::Effect_ApplyStatus_Freeze,
		WacomTags::Effect_ApplyStatus_Twilight,
		WacomTags::Effect_RemoveStatus,
		WacomTags::Effect_Heal,
		WacomTags::Effect_ModifyInitiative,
		WacomTags::Effect_Shuffle_Random,
		WacomTags::Effect_Shuffle_FromBothToOther,
		WacomTags::Effect_Shuffle_ToRandomZone,
		WacomTags::Effect_Card_AddCost,
		WacomTags::Effect_Card_ReduceCost,
		WacomTags::Effect_Card_DiscardSelected,
		WacomTags::Effect_Card_ExhaustSelected,
		WacomTags::Effect_Draw,
		WacomTags::Effect_Discard,
		WacomTags::Effect_ExhaustSelf,
		WacomTags::Effect_GainKeyword };
	for (const FGameplayTag& EffectType : ExpectedEffects)
	{
		TestTrue(
			*FString::Printf(TEXT("Expected effect is registered: %s"), *EffectType.ToString()),
			CardEffects.Contains(EffectType));
	}

	return true;
}
