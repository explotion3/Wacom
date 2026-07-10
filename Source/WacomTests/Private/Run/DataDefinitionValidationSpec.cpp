// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardPassive.h"
#include "Cards/CardZoneHook.h"
#include "Characters/CharacterDefinition.h"
#include "Encounters/EncounterDefinition.h"
#include "Enemies/EnemyBehaviorDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Enemies/IntentDefinition.h"
#include "Enemies/IntentEffect.h"
#include "Fixtures/GeneratedBattleContentTestAssets.h"
#include "Shops/ShopDefinition.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Card/WacomCardPresentationBuilder.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "Validation/CardDefinitionValidation.h"
#include "Validation/CharacterDefinitionValidation.h"
#include "Validation/EnemyBehaviorDefinitionValidation.h"
#include "Validation/EnemyDefinitionValidation.h"
#include "Validation/EnemyPartDefinitionValidation.h"
#include "Validation/EncounterDefinitionValidation.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
	FCardEffect MakeValidCardEffect()
	{
		FCardEffect Effect;
		Effect.EffectType = WacomTags::Effect_Damage;
		Effect.Magnitude = 3;
		Effect.Target = WacomTags::Target_SingleEnemyPart;
		return Effect;
	}

	FIntentEffect MakeValidIntentEffect()
	{
		FIntentEffect Effect;
		Effect.EffectType = WacomTags::Effect_Damage;
		Effect.Magnitude = 2;
		Effect.Target = WacomTags::Target_Player;
		return Effect;
	}

	UCardDefinition* MakeValidCardForValidation(UObject* Outer, FName CardId = TEXT("Card.Validation"))
	{
		UCardDefinition* Card = NewObject<UCardDefinition>(Outer);
		Card->CardId = CardId;
		Card->DisplayName = FText::FromName(CardId);
		Card->BaseCost = 1;
		Card->Rarity = WacomTags::Card_Rarity_White;
		Card->TargetMode = ECardTargetMode::SingleEnemyPart;
		Card->Effects = { MakeValidCardEffect() };
		return Card;
	}

	UEnemyPartDefinition* MakeValidEnemyPartForValidation(UObject* Outer, FName PartId = TEXT("EnemyPart.Validation"))
	{
		UEnemyPartDefinition* Part = NewObject<UEnemyPartDefinition>(Outer);
		Part->PartId = PartId;
		Part->DisplayName = FText::FromName(PartId);
		Part->MaxHp = 10;
		Part->ExperienceReward = 1;
		return Part;
	}

	UEnemyDefinition* MakeValidEnemyForValidation(UObject* Outer)
	{
		UEnemyDefinition* Enemy = NewObject<UEnemyDefinition>(Outer);
		Enemy->EnemyId = TEXT("Enemy.Validation");
		Enemy->DisplayName = FText::FromString(TEXT("校验敌人"));

		FEnemyPartSlot Slot;
		Slot.PartSlotId = TEXT("Core");
		Slot.PartDef = MakeValidEnemyPartForValidation(Enemy);
		Enemy->Parts = { Slot };
		return Enemy;
	}

	UCharacterDefinition* MakeValidCharacterForValidation(UObject* Outer)
	{
		UCharacterDefinition* Character = NewObject<UCharacterDefinition>(Outer);
		Character->CharacterId = TEXT("Character.Validation");
		Character->DisplayName = FText::FromString(TEXT("校验角色"));
		Character->FingerCount = 10;
		Character->HpPerFinger = 2;
		Character->LeftHandCard = MakeValidCardForValidation(Character, TEXT("Card.Validation.LeftHand"));
		Character->RightHandCard = MakeValidCardForValidation(Character, TEXT("Card.Validation.RightHand"));
		Character->StarterDeck = {
			MakeValidCardForValidation(Character, TEXT("Card.Validation.Starter"))
		};
		return Character;
	}

	FIntentDefinition MakeValidIntentDefinition()
	{
		FIntentDefinition Intent;
		Intent.IntentId = TEXT("Intent.Validation");
		Intent.DisplayName = FText::FromName(Intent.IntentId);
		Intent.Effects = { MakeValidIntentEffect() };
		return Intent;
	}

	bool ValidateCardForTest(const UCardDefinition* Card, TArray<FText>& OutErrors)
	{
		return FWacomCardDefinitionValidation::Validate(Card, OutErrors);
	}

	bool ValidateEnemyPartForTest(const UEnemyPartDefinition* Part, TArray<FText>& OutErrors)
	{
		return FWacomEnemyPartDefinitionValidation::Validate(Part, OutErrors);
	}

	bool ValidateEnemyForTest(const UEnemyDefinition* Enemy, TArray<FText>& OutErrors)
	{
		return FWacomEnemyDefinitionValidation::Validate(Enemy, OutErrors);
	}

	bool ValidateEnemyBehaviorForTest(
		const UEnemyBehaviorDefinition* Behavior,
		TArray<FText>& OutErrors,
		const UEnemyDefinition* Enemy = nullptr)
	{
		return FWacomEnemyBehaviorDefinitionValidation::Validate(Behavior, OutErrors, Enemy);
	}

	bool ValidateCharacterForTest(const UCharacterDefinition* Character, TArray<FText>& OutErrors)
	{
		return FWacomCharacterDefinitionValidation::Validate(Character, OutErrors);
	}

	bool ValidateRunEncounterForTest(const UEncounterDefinition* Encounter, TArray<FText>& OutErrors)
	{
		return FWacomEncounterDefinitionValidation::Validate(Encounter, OutErrors);
	}

	bool ErrorsContain(const TArray<FText>& Errors, const FString& Needle)
	{
		for (const FText& Error : Errors)
		{
			if (Error.ToString().Contains(Needle))
			{
				return true;
			}
		}
		return false;
	}

	const FWacomEnemyIntentSetDefinition* FindIntentSet(
		const UEnemyBehaviorDefinition* Behavior,
		FName PhaseId,
		FName IntentSetId)
	{
		if (!Behavior)
		{
			return nullptr;
		}

		for (const FWacomEnemyPhaseDefinition& Phase : Behavior->Phases)
		{
			if (Phase.PhaseId != PhaseId)
			{
				continue;
			}

			for (const FWacomEnemyIntentSetDefinition& IntentSet : Phase.IntentSets)
			{
				if (IntentSet.IntentSetId == IntentSetId)
				{
					return &IntentSet;
				}
			}
		}
		return nullptr;
	}

	bool HasBehaviorIntentEffect(
		const FWacomEnemyIntentSetDefinition* IntentSet,
		const FGameplayTag& EffectType,
		const FGameplayTag& Target)
	{
		if (!IntentSet)
		{
			return false;
		}

		for (const FWacomEnemyBehaviorIntent& IntentEntry : IntentSet->Intents)
		{
			for (const FIntentEffect& Effect : IntentEntry.Intent.Effects)
			{
				if (Effect.EffectType == EffectType && Effect.Target == Target)
				{
					return true;
				}
			}
		}
		return false;
	}

	bool ContainsOffer(const UShopDefinition* Shop, const UCardDefinition* Card, int32 ExpectedPrice)
	{
		return Shop && Card && Shop->Offers.ContainsByPredicate(
			[Card, ExpectedPrice](const FShopOfferDefinition& Offer)
			{
				return Offer.CardDefinition.Get() == Card && Offer.Price == ExpectedPrice;
			});
	}

	bool AssertBadge(
		FAutomationTestBase& Test,
		const TArray<FWacomCardViewEffectBadge>& Badges,
		int32 Index,
		EWacomCardViewEffectBadgeKind ExpectedKind,
		int32 ExpectedValue)
	{
		if (!Test.TestTrue(FString::Printf(TEXT("Badge index %d exists"), Index), Badges.IsValidIndex(Index)))
		{
			return false;
		}

		Test.TestTrue(FString::Printf(TEXT("Badge %d kind"), Index), Badges[Index].Kind == ExpectedKind);
		Test.TestEqual(FString::Printf(TEXT("Badge %d value"), Index), Badges[Index].Value, ExpectedValue);
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataCardValidationValidSpec,
	"Wacom.Data.Card.Validation.ValidCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataCardValidationValidSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> Card(MakeValidCardForValidation(GetTransientPackage()));
	TArray<FText> Errors;
	TestTrue(TEXT("Valid CardDefinition passes validation"), ValidateCardForTest(Card.Get(), Errors));
	TestEqual(TEXT("No validation errors"), Errors.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataCardValidationRequiredFieldsSpec,
	"Wacom.Data.Card.Validation.RequiredFields",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataCardValidationRequiredFieldsSpec::RunTest(const FString& /*Parameters*/)
{
	{
		TStrongObjectPtr<UCardDefinition> Card(MakeValidCardForValidation(GetTransientPackage()));
		Card->CardId = NAME_None;
		TArray<FText> Errors;
		TestFalse(TEXT("Missing CardId fails"), ValidateCardForTest(Card.Get(), Errors));
		TestTrue(TEXT("Missing CardId has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UCardDefinition> Card(MakeValidCardForValidation(GetTransientPackage()));
		Card->BaseCost = -1;
		TArray<FText> Errors;
		TestFalse(TEXT("Negative BaseCost fails"), ValidateCardForTest(Card.Get(), Errors));
		TestTrue(TEXT("Negative BaseCost has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UCardDefinition> Card(MakeValidCardForValidation(GetTransientPackage()));
		Card->Rarity = WacomTags::Effect_Damage;
		TArray<FText> Errors;
		TestFalse(TEXT("Invalid Rarity fails"), ValidateCardForTest(Card.Get(), Errors));
		TestTrue(TEXT("Invalid Rarity has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UCardDefinition> Card(MakeValidCardForValidation(GetTransientPackage()));
		Card->Physique.MaxHpBonus = -1;
		TArray<FText> Errors;
		TestFalse(TEXT("Negative Physique MaxHpBonus fails"), ValidateCardForTest(Card.Get(), Errors));
		TestTrue(TEXT("Negative Physique MaxHpBonus has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UCardDefinition> Card(MakeValidCardForValidation(GetTransientPackage()));
		Card->Physique.Durability = -1;
		TArray<FText> Errors;
		TestFalse(TEXT("Negative Physique Durability fails"), ValidateCardForTest(Card.Get(), Errors));
		TestTrue(TEXT("Negative Physique Durability has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UCardDefinition> Card(MakeValidCardForValidation(GetTransientPackage()));
		Card->Physique.Capacity = -1;
		TArray<FText> Errors;
		TestFalse(TEXT("Negative Physique Capacity fails"), ValidateCardForTest(Card.Get(), Errors));
		TestTrue(TEXT("Negative Physique Capacity has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UCardDefinition> Card(MakeValidCardForValidation(GetTransientPackage()));
		Card->Effects[0].EffectType = FGameplayTag();
		TArray<FText> Errors;
		TestFalse(TEXT("Invalid EffectType fails"), ValidateCardForTest(Card.Get(), Errors));
		TestTrue(TEXT("Invalid EffectType has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UCardDefinition> Card(MakeValidCardForValidation(GetTransientPackage()));
		FCardZoneHook Hook;
		Hook.Zone = WacomTags::Effect_Damage;
		Hook.Trigger = WacomTags::ZoneHook_Trigger_OnPlay;
		Hook.ExtraEffects = { MakeValidCardEffect() };
		Card->ZoneHooks = { Hook };
		TArray<FText> Errors;
		TestFalse(TEXT("Invalid ZoneHook Zone fails"), ValidateCardForTest(Card.Get(), Errors));
		TestTrue(TEXT("Invalid ZoneHook Zone has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UCardDefinition> Card(MakeValidCardForValidation(GetTransientPackage()));
		FCardZoneHook Hook;
		Hook.Zone = WacomTags::HandZone_Left;
		Hook.Trigger = WacomTags::Effect_Damage;
		Hook.ExtraEffects = { MakeValidCardEffect() };
		Card->ZoneHooks = { Hook };
		TArray<FText> Errors;
		TestFalse(TEXT("Invalid ZoneHook Trigger fails"), ValidateCardForTest(Card.Get(), Errors));
		TestTrue(TEXT("Invalid ZoneHook Trigger has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UCardDefinition> Card(MakeValidCardForValidation(GetTransientPackage()));
		FCardPassive Passive;
		Passive.Trigger = WacomTags::Effect_Damage;
		Passive.Effects = { MakeValidCardEffect() };
		Card->Passives = { Passive };
		TArray<FText> Errors;
		TestFalse(TEXT("Invalid Passive Trigger fails"), ValidateCardForTest(Card.Get(), Errors));
		TestTrue(TEXT("Invalid Passive Trigger has error"), Errors.Num() > 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataCardValidationBattleRuleContentContractSpec,
	"Wacom.Data.Card.Validation.BattleRuleContentContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataCardValidationBattleRuleContentContractSpec::RunTest(const FString& /*Parameters*/)
{
	{
		TStrongObjectPtr<UCardDefinition> Card(MakeValidCardForValidation(GetTransientPackage()));
		Card->Effects[0].EffectType = WacomTags::CardLocation_Hand;
		TArray<FText> Errors;
		TestFalse(TEXT("Unregistered EffectType fails"), ValidateCardForTest(Card.Get(), Errors));
		TestTrue(TEXT("Unregistered effect reports rule registration"), ErrorsContain(Errors, TEXT("未注册")));
	}

	{
		TStrongObjectPtr<UCardDefinition> Card(MakeValidCardForValidation(GetTransientPackage()));
		Card->TargetMode = ECardTargetMode::None;
		Card->Effects[0].Target = WacomTags::Target_SingleEnemyPart;
		TArray<FText> Errors;
		TestFalse(TEXT("Selected enemy target without TargetMode fails"), ValidateCardForTest(Card.Get(), Errors));
		TestTrue(TEXT("Invalid target reports context"), ErrorsContain(Errors, TEXT("Target")));
	}

	{
		TStrongObjectPtr<UCardDefinition> Card(MakeValidCardForValidation(GetTransientPackage()));
		FCardEffect Effect;
		Effect.EffectType = WacomTags::Effect_Shuffle_FromBothToOther;
		Effect.Target = WacomTags::Target_ZoneHandCard;
		Effect.Magnitude = 0;
		Card->TargetMode = ECardTargetMode::None;
		Card->Effects = { Effect };
		TArray<FText> Errors;
		TestFalse(TEXT("Missing TargetZone for zone shuffle fails"), ValidateCardForTest(Card.Get(), Errors));
		TestTrue(TEXT("Missing TargetZone is reported"), ErrorsContain(Errors, TEXT("TargetZone")));
	}

	{
		TStrongObjectPtr<UCardDefinition> Card(MakeValidCardForValidation(GetTransientPackage()));
		Card->Effects[0].MagnitudeSource = WacomTags::Magnitude_Source_TargetStatusStacks;
		Card->Effects[0].TargetZone = WacomTags::Status_Shield;
		TArray<FText> Errors;
		TestFalse(TEXT("TargetStatusStacks cannot read Shield"), ValidateCardForTest(Card.Get(), Errors));
		TestTrue(TEXT("Shield status stack issue reported"), ErrorsContain(Errors, TEXT("Status.Shield")));
	}

	{
		TStrongObjectPtr<UCardDefinition> Card(MakeValidCardForValidation(GetTransientPackage()));
		Card->Effects[0].EffectType = WacomTags::Effect_Draw;
		Card->Effects[0].Target = WacomTags::Target_Player;
		Card->Effects[0].TargetZone = WacomTags::CardLocation_Draw;
		Card->Effects[0].Magnitude = 0;
		Card->Effects[0].MagnitudeSource = WacomTags::Magnitude_Source_RuntimeCost;
		TArray<FText> Errors;
		TestTrue(TEXT("RuntimeCost source can drive draw"), ValidateCardForTest(Card.Get(), Errors));
		TestEqual(TEXT("RuntimeCost draw has no validation errors"), Errors.Num(), 0);
	}

	{
		TStrongObjectPtr<UCardDefinition> Card(MakeValidCardForValidation(GetTransientPackage()));
		Card->Effects[0].Magnitude = 0;
		Card->Effects[0].MagnitudeSource = WacomTags::Magnitude_Source_HandCount;
		TArray<FText> Errors;
		TestFalse(TEXT("HandCount source is reserved for authoring"), ValidateCardForTest(Card.Get(), Errors));
		TestTrue(TEXT("HandCount source mismatch reported"), ErrorsContain(Errors, TEXT("MagnitudeSource")));
	}

	{
		TStrongObjectPtr<UCardDefinition> Card(MakeValidCardForValidation(GetTransientPackage()));
		Card->Effects[0].Condition.ConditionType = WacomTags::Condition_Target_HasStatus;
		Card->Effects[0].Condition.ParamTag = WacomTags::Status_Shield;
		TArray<FText> Errors;
		TestFalse(TEXT("HasStatus cannot read Shield"), ValidateCardForTest(Card.Get(), Errors));
		TestTrue(TEXT("Shield condition issue reported"), ErrorsContain(Errors, TEXT("Status.Shield")));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataCardValidationPassiveTriggerContractSpec,
	"Wacom.Data.Card.Validation.PassiveTriggerContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataCardValidationPassiveTriggerContractSpec::RunTest(const FString& /*Parameters*/)
{
	{
		TStrongObjectPtr<UCardDefinition> Card(MakeValidCardForValidation(GetTransientPackage()));
		FCardPassive Passive;
		Passive.Trigger = WacomTags::Passive_Trigger_OnTwilightTriggered;
		Passive.DisplayText = FText::FromString(TEXT("只展示"));
		Card->Passives = { Passive };
		TArray<FText> Errors;
		TestTrue(TEXT("OnTwilight display-only passive passes"), ValidateCardForTest(Card.Get(), Errors));
		TestEqual(TEXT("No OnTwilight display-only errors"), Errors.Num(), 0);
	}

	{
		TStrongObjectPtr<UCardDefinition> Card(MakeValidCardForValidation(GetTransientPackage()));
		FCardPassive Passive;
		Passive.Trigger = WacomTags::Passive_Trigger_OnTwilightTriggered;
		Passive.Effects = { MakeValidCardEffect() };
		Card->Passives = { Passive };
		TArray<FText> Errors;
		TestFalse(TEXT("OnTwilight with effects fails"), ValidateCardForTest(Card.Get(), Errors));
		TestTrue(TEXT("OnTwilight effects issue reported"), ErrorsContain(Errors, TEXT("不执行 Effects")));
	}

	{
		TStrongObjectPtr<UCardDefinition> Card(MakeValidCardForValidation(GetTransientPackage()));
		FCardPassive Passive;
		Passive.Trigger = WacomTags::Passive_Trigger_OnTurnStart;
		Card->Passives = { Passive };
		TArray<FText> Errors;
		TestFalse(TEXT("Reserved OnTurnStart fails"), ValidateCardForTest(Card.Get(), Errors));
		TestTrue(TEXT("Reserved trigger issue reported"), ErrorsContain(Errors, TEXT("保留触发点")));
	}

	{
		TStrongObjectPtr<UCardDefinition> Card(MakeValidCardForValidation(GetTransientPackage()));
		FCardPassive Passive;
		Passive.Trigger = WacomTags::Passive_Trigger_OnCompanionCount;
		Passive.TriggerThreshold = 3;
		Passive.DisplayText = FText::FromString(TEXT("每三张伙伴回手"));
		Card->Passives = { Passive };
		TArray<FText> Errors;
		TestTrue(TEXT("OnCompanionCount threshold passive passes"), ValidateCardForTest(Card.Get(), Errors));
		TestEqual(TEXT("No OnCompanionCount errors"), Errors.Num(), 0);
	}

	{
		TStrongObjectPtr<UCardDefinition> Card(MakeValidCardForValidation(GetTransientPackage()));
		FCardZoneHook Hook;
		Hook.Zone = WacomTags::HandZone_Left;
		Hook.Trigger = WacomTags::ZoneHook_Trigger_OnPerfectReleaseHit;
		Card->ZoneHooks = { Hook };
		TArray<FText> Errors;
		TestTrue(TEXT("Empty OnPerfectReleaseHit zone hook remains legal"), ValidateCardForTest(Card.Get(), Errors));
		TestEqual(TEXT("No empty OnPerfectReleaseHit hook errors"), Errors.Num(), 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataEnemyPartValidationValidSpec,
	"Wacom.Data.EnemyPart.Validation.ValidPart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataEnemyPartValidationValidSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UEnemyPartDefinition> Part(MakeValidEnemyPartForValidation(GetTransientPackage()));
	TArray<FText> Errors;
	TestTrue(TEXT("Valid EnemyPartDefinition static data passes validation"), ValidateEnemyPartForTest(Part.Get(), Errors));
	TestEqual(TEXT("No validation errors"), Errors.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataEnemyPartValidationRequiredFieldsSpec,
	"Wacom.Data.EnemyPart.Validation.RequiredFields",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataEnemyPartValidationRequiredFieldsSpec::RunTest(const FString& /*Parameters*/)
{
	{
		TStrongObjectPtr<UEnemyPartDefinition> Part(MakeValidEnemyPartForValidation(GetTransientPackage()));
		Part->PartId = NAME_None;
		TArray<FText> Errors;
		TestFalse(TEXT("Missing PartId fails"), ValidateEnemyPartForTest(Part.Get(), Errors));
		TestTrue(TEXT("Missing PartId has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UEnemyPartDefinition> Part(MakeValidEnemyPartForValidation(GetTransientPackage()));
		Part->MaxHp = 0;
		TArray<FText> Errors;
		TestFalse(TEXT("Zero MaxHp fails"), ValidateEnemyPartForTest(Part.Get(), Errors));
		TestTrue(TEXT("Zero MaxHp has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UEnemyPartDefinition> Part(MakeValidEnemyPartForValidation(GetTransientPackage()));
		Part->ExperienceReward = -1;
		TArray<FText> Errors;
		TestFalse(TEXT("Negative ExperienceReward fails"), ValidateEnemyPartForTest(Part.Get(), Errors));
		TestTrue(TEXT("Negative ExperienceReward has error"), Errors.Num() > 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataEnemyValidationValidSpec,
	"Wacom.Data.Enemy.Validation.ValidEnemy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataEnemyValidationValidSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UEnemyDefinition> Enemy(MakeValidEnemyForValidation(GetTransientPackage()));
	TArray<FText> Errors;
	TestTrue(TEXT("Valid EnemyDefinition passes validation"), ValidateEnemyForTest(Enemy.Get(), Errors));
	TestEqual(TEXT("No validation errors"), Errors.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataEnemyValidationRequiredFieldsSpec,
	"Wacom.Data.Enemy.Validation.RequiredFields",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataEnemyValidationRequiredFieldsSpec::RunTest(const FString& /*Parameters*/)
{
	{
		TStrongObjectPtr<UEnemyDefinition> Enemy(MakeValidEnemyForValidation(GetTransientPackage()));
		Enemy->EnemyId = NAME_None;
		TArray<FText> Errors;
		TestFalse(TEXT("Missing EnemyId fails"), ValidateEnemyForTest(Enemy.Get(), Errors));
		TestTrue(TEXT("Missing EnemyId has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UEnemyDefinition> Enemy(MakeValidEnemyForValidation(GetTransientPackage()));
		Enemy->Parts.Reset();
		TArray<FText> Errors;
		TestFalse(TEXT("Empty Parts fails"), ValidateEnemyForTest(Enemy.Get(), Errors));
		TestTrue(TEXT("Empty Parts has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UEnemyDefinition> Enemy(MakeValidEnemyForValidation(GetTransientPackage()));
		Enemy->Parts[0].PartDef = nullptr;
		TArray<FText> Errors;
		TestFalse(TEXT("Missing PartDef fails"), ValidateEnemyForTest(Enemy.Get(), Errors));
		TestTrue(TEXT("Missing PartDef has error"), Errors.Num() > 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataCharacterValidationValidSpec,
	"Wacom.Data.Character.Validation.ValidCharacter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataCharacterValidationValidSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCharacterDefinition> Character(MakeValidCharacterForValidation(GetTransientPackage()));
	TArray<FText> Errors;
	TestTrue(TEXT("Valid CharacterDefinition passes validation"), ValidateCharacterForTest(Character.Get(), Errors));
	TestEqual(TEXT("No validation errors"), Errors.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataCharacterValidationRequiredFieldsSpec,
	"Wacom.Data.Character.Validation.RequiredFields",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataCharacterValidationRequiredFieldsSpec::RunTest(const FString& /*Parameters*/)
{
	{
		TStrongObjectPtr<UCharacterDefinition> Character(MakeValidCharacterForValidation(GetTransientPackage()));
		Character->CharacterId = NAME_None;
		TArray<FText> Errors;
		TestFalse(TEXT("Missing CharacterId fails"), ValidateCharacterForTest(Character.Get(), Errors));
		TestTrue(TEXT("Missing CharacterId has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UCharacterDefinition> Character(MakeValidCharacterForValidation(GetTransientPackage()));
		Character->FingerCount = 0;
		TArray<FText> Errors;
		TestFalse(TEXT("Zero FingerCount fails"), ValidateCharacterForTest(Character.Get(), Errors));
		TestTrue(TEXT("Zero FingerCount has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UCharacterDefinition> Character(MakeValidCharacterForValidation(GetTransientPackage()));
		Character->HpPerFinger = 0;
		TArray<FText> Errors;
		TestFalse(TEXT("Zero HpPerFinger fails"), ValidateCharacterForTest(Character.Get(), Errors));
		TestTrue(TEXT("Zero HpPerFinger has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UCharacterDefinition> Character(MakeValidCharacterForValidation(GetTransientPackage()));
		Character->LeftHandCard = nullptr;
		TArray<FText> Errors;
		TestFalse(TEXT("Missing LeftHandCard fails"), ValidateCharacterForTest(Character.Get(), Errors));
		TestTrue(TEXT("Missing LeftHandCard has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UCharacterDefinition> Character(MakeValidCharacterForValidation(GetTransientPackage()));
		Character->RightHandCard = nullptr;
		TArray<FText> Errors;
		TestFalse(TEXT("Missing RightHandCard fails"), ValidateCharacterForTest(Character.Get(), Errors));
		TestTrue(TEXT("Missing RightHandCard has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UCharacterDefinition> Character(MakeValidCharacterForValidation(GetTransientPackage()));
		Character->StarterDeck.Reset();
		TArray<FText> Errors;
		TestFalse(TEXT("Empty StarterDeck fails"), ValidateCharacterForTest(Character.Get(), Errors));
		TestTrue(TEXT("Empty StarterDeck has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UCharacterDefinition> Character(MakeValidCharacterForValidation(GetTransientPackage()));
		Character->StarterDeck[0] = nullptr;
		TArray<FText> Errors;
		TestFalse(TEXT("Null StarterDeck card fails"), ValidateCharacterForTest(Character.Get(), Errors));
		TestTrue(TEXT("Null StarterDeck card has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UCharacterDefinition> Character(MakeValidCharacterForValidation(GetTransientPackage()));
		Character->StarterDeck[0] = Character->LeftHandCard;
		TArray<FText> Errors;
		TestFalse(TEXT("LeftHandCard in StarterDeck fails"), ValidateCharacterForTest(Character.Get(), Errors));
		TestTrue(TEXT("LeftHandCard in StarterDeck has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UCharacterDefinition> Character(MakeValidCharacterForValidation(GetTransientPackage()));
		Character->StarterDeck[0] = Character->RightHandCard;
		TArray<FText> Errors;
		TestFalse(TEXT("RightHandCard in StarterDeck fails"), ValidateCharacterForTest(Character.Get(), Errors));
		TestTrue(TEXT("RightHandCard in StarterDeck has error"), Errors.Num() > 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataGeneratedContentDefinitionAssetValidationSpec,
	"Wacom.Data.GeneratedContent.DefinitionAssetValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataGeneratedContentDefinitionAssetValidationSpec::RunTest(const FString& /*Parameters*/)
{
	UCharacterDefinition* BugGirl = FWacomGeneratedBattleContentAssets::LoadBugGirl(*this);
	UEnemyDefinition* Snake = FWacomGeneratedBattleContentAssets::LoadSnake(*this);
	UEnemyBehaviorDefinition* SnakeBehavior = FWacomGeneratedBattleContentAssets::LoadSnakeBehavior(*this);
	UEnemyPartDefinition* SnakeHead = FWacomGeneratedBattleContentAssets::LoadSnakeHead(*this);
	UEnemyPartDefinition* SnakeBody = FWacomGeneratedBattleContentAssets::LoadSnakeBody(*this);
	UEnemyPartDefinition* SnakeTail = FWacomGeneratedBattleContentAssets::LoadSnakeTail(*this);
	UEncounterDefinition* SnakeSingleEncounter =
		FWacomGeneratedBattleContentAssets::LoadSnakeSingleEncounter(*this);

	bool bAllAssetsLoaded = true;
	bAllAssetsLoaded &= TestNotNull(TEXT("BugGirl character asset loads"), BugGirl);
	bAllAssetsLoaded &= TestNotNull(TEXT("Snake enemy asset loads"), Snake);
	bAllAssetsLoaded &= TestNotNull(TEXT("Snake behavior asset loads"), SnakeBehavior);
	bAllAssetsLoaded &= TestNotNull(TEXT("Snake head part asset loads"), SnakeHead);
	bAllAssetsLoaded &= TestNotNull(TEXT("Snake body part asset loads"), SnakeBody);
	bAllAssetsLoaded &= TestNotNull(TEXT("Snake tail part asset loads"), SnakeTail);
	bAllAssetsLoaded &= TestNotNull(TEXT("Snake single encounter asset loads"), SnakeSingleEncounter);

	TArray<UCardDefinition*> GeneratedCards;
	for (const TCHAR* CardAssetPath : FWacomGeneratedBattleContentAssets::GeneratedDefinitionCardPaths())
	{
		UCardDefinition* Card = FWacomGeneratedBattleContentAssets::LoadCardByPath(CardAssetPath, *this);
		bAllAssetsLoaded &= TestNotNull(*FString::Printf(TEXT("%s loads"), CardAssetPath), Card);
		GeneratedCards.Add(Card);
	}

	if (!bAllAssetsLoaded)
	{
		return false;
	}

	TArray<FText> Errors;

	Errors.Reset();
	TestTrue(TEXT("BugGirl character asset passes validation"), ValidateCharacterForTest(BugGirl, Errors));
	TestEqual(TEXT("BugGirl character validation errors"), Errors.Num(), 0);

	for (const UCardDefinition* Card : GeneratedCards)
	{
		Errors.Reset();
		TestTrue(*FString::Printf(TEXT("%s card asset passes validation"), *GetNameSafe(Card)), ValidateCardForTest(Card, Errors));
		TestEqual(*FString::Printf(TEXT("%s card validation errors"), *GetNameSafe(Card)), Errors.Num(), 0);
	}

	Errors.Reset();
	TestTrue(TEXT("Snake enemy asset passes validation"), ValidateEnemyForTest(Snake, Errors));
	TestEqual(TEXT("Snake enemy validation errors"), Errors.Num(), 0);
	if (Snake && Snake->Parts.Num() == 3)
	{
		TestTrue(TEXT("Snake references generated behavior"),
			Snake->DefaultBehavior.Get() == SnakeBehavior);
		TestEqual(TEXT("Snake default phase"), Snake->DefaultPhaseId, FName(TEXT("Default")));
		TestEqual(TEXT("Snake head slot id"), Snake->Parts[0].PartSlotId, FName(TEXT("Head")));
		TestEqual(TEXT("Snake body slot id"), Snake->Parts[1].PartSlotId, FName(TEXT("Body")));
		TestEqual(TEXT("Snake tail slot id"), Snake->Parts[2].PartSlotId, FName(TEXT("Tail")));
		TestEqual(TEXT("Snake head intent set"), Snake->Parts[0].InitialIntentSetId, FName(TEXT("Snake.Head.Sequence")));
		TestEqual(TEXT("Snake body intent set"), Snake->Parts[1].InitialIntentSetId, FName(TEXT("Snake.Body.Sequence")));
		TestEqual(TEXT("Snake tail intent set"), Snake->Parts[2].InitialIntentSetId, FName(TEXT("Snake.Tail.Sequence")));
	}

	Errors.Reset();
	TestTrue(TEXT("Snake behavior asset passes validation"),
		ValidateEnemyBehaviorForTest(SnakeBehavior, Errors, Snake));
	TestEqual(TEXT("Snake behavior validation errors"), Errors.Num(), 0);

	Errors.Reset();
	TestTrue(TEXT("Snake head asset passes validation"), ValidateEnemyPartForTest(SnakeHead, Errors));
	TestEqual(TEXT("Snake head validation errors"), Errors.Num(), 0);

	Errors.Reset();
	TestTrue(TEXT("Snake body asset passes validation"), ValidateEnemyPartForTest(SnakeBody, Errors));
	TestEqual(TEXT("Snake body validation errors"), Errors.Num(), 0);

	Errors.Reset();
	TestTrue(TEXT("Snake tail asset passes validation"), ValidateEnemyPartForTest(SnakeTail, Errors));
	TestEqual(TEXT("Snake tail validation errors"), Errors.Num(), 0);

	Errors.Reset();
	TestTrue(TEXT("Snake single encounter asset passes validation"),
		ValidateRunEncounterForTest(SnakeSingleEncounter, Errors));
	TestEqual(TEXT("Snake single encounter validation errors"), Errors.Num(), 0);
	if (SnakeSingleEncounter && SnakeSingleEncounter->EnemySlots.Num() == 1)
	{
		TestEqual(TEXT("Snake encounter id"),
			SnakeSingleEncounter->EncounterDefinitionId,
			FName(TEXT("Encounter.Snake.Single")));
		TestEqual(TEXT("Snake encounter enemy slot"),
			SnakeSingleEncounter->EnemySlots[0].EnemySlotId,
			FName(TEXT("Enemy")));
		TestTrue(TEXT("Snake encounter references Snake enemy"),
			SnakeSingleEncounter->EnemySlots[0].EnemyDefinition.Get() == Snake);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataDebugKeyAssetLoadsSpec,
	"Wacom.Data.DebugKeyAsset.DebugKeyAssetLoads",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataDebugKeyAssetLoadsSpec::RunTest(const FString& /*Parameters*/)
{
	UCardDefinition* DebugKey = FWacomGeneratedBattleContentAssets::LoadDebugKey(*this);
	if (!TestNotNull(TEXT("DebugKey asset loads"), DebugKey))
	{
		return false;
	}

	TestEqual(TEXT("CardId"), DebugKey->CardId, FName(TEXT("DebugKey")));
	TestEqual(TEXT("DisplayName"), DebugKey->DisplayName.ToString(), FString(TEXT("钥匙")));
	TestTrue(TEXT("Rarity"), DebugKey->Rarity.MatchesTagExact(WacomTags::Card_Rarity_White));
	TestTrue(TEXT("Has Tool keyword"), DebugKey->Keywords.HasTagExact(WacomTags::Card_Keyword_Tool));
	TestEqual(TEXT("TargetMode None"), static_cast<int32>(DebugKey->TargetMode), static_cast<int32>(ECardTargetMode::None));
	TestEqual(TEXT("No battle effects"), DebugKey->Effects.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataDebugKeyBugGirlStarterDeckSpec,
	"Wacom.Data.DebugKeyAsset.DebugKeyIsInDebugSnakeShopNotBugGirlStarterDeck",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataDebugKeyBugGirlStarterDeckSpec::RunTest(const FString& /*Parameters*/)
{
	UCardDefinition* DebugKey = FWacomGeneratedBattleContentAssets::LoadDebugKey(*this);
	UCharacterDefinition* BugGirl = FWacomGeneratedBattleContentAssets::LoadBugGirl(*this);
	UShopDefinition* DebugShop = FWacomGeneratedBattleContentAssets::LoadDebugSnakeShop(*this);
	if (!TestNotNull(TEXT("DebugKey asset loads"), DebugKey)
		|| !TestNotNull(TEXT("BugGirl character asset loads"), BugGirl)
		|| !TestNotNull(TEXT("DebugSnake shop asset loads"), DebugShop))
	{
		return false;
	}

	TestFalse(TEXT("BugGirl starter deck does not contain DebugKey"),
		BugGirl->StarterDeck.Contains(DebugKey));
	TestTrue(TEXT("DebugSnake shop sells DebugKey for free"),
		ContainsOffer(DebugShop, DebugKey, 0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataBattleStarterContentAssetValidationSpec,
	"Wacom.Data.BattleStarterContent.StarterPackAssetValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataBattleStarterContentAssetValidationSpec::RunTest(const FString& /*Parameters*/)
{
	UCardDefinition* PoisonNeedle = FWacomGeneratedBattleContentAssets::LoadPoisonNeedle(*this);
	UCardDefinition* ChitinWard = FWacomGeneratedBattleContentAssets::LoadChitinWard(*this);
	UCardDefinition* AntennaSearch = FWacomGeneratedBattleContentAssets::LoadAntennaSearch(*this);
	UCardDefinition* MoltCut = FWacomGeneratedBattleContentAssets::LoadMoltCut(*this);
	UCardDefinition* LightHusk = FWacomGeneratedBattleContentAssets::LoadLightHusk(*this);
	UCardDefinition* SilklineFeint = FWacomGeneratedBattleContentAssets::LoadSilklineFeint(*this);

	if (!TestNotNull(TEXT("PoisonNeedle loads"), PoisonNeedle)
		|| !TestNotNull(TEXT("ChitinWard loads"), ChitinWard)
		|| !TestNotNull(TEXT("AntennaSearch loads"), AntennaSearch)
		|| !TestNotNull(TEXT("MoltCut loads"), MoltCut)
		|| !TestNotNull(TEXT("LightHusk loads"), LightHusk)
		|| !TestNotNull(TEXT("SilklineFeint loads"), SilklineFeint))
	{
		return false;
	}

	TArray<FText> Errors;
	for (const UCardDefinition* Card : { PoisonNeedle, ChitinWard, AntennaSearch, MoltCut, LightHusk, SilklineFeint })
	{
		Errors.Reset();
		TestTrue(*FString::Printf(TEXT("%s passes validation"), *GetNameSafe(Card)), ValidateCardForTest(Card, Errors));
		TestEqual(*FString::Printf(TEXT("%s validation errors"), *GetNameSafe(Card)), Errors.Num(), 0);
	}

	TestEqual(TEXT("PoisonNeedle id"), PoisonNeedle->CardId, FName(TEXT("Starter.PoisonNeedle")));
	TestEqual(TEXT("PoisonNeedle target mode"), PoisonNeedle->TargetMode, ECardTargetMode::SingleEnemyPart);
	TestEqual(TEXT("PoisonNeedle effects"), PoisonNeedle->Effects.Num(), 2);
	if (PoisonNeedle->Effects.IsValidIndex(1))
	{
		TestEqual(TEXT("PoisonNeedle bonus condition"),
			PoisonNeedle->Effects[1].Condition.ConditionType,
			FGameplayTag(WacomTags::Condition_Target_HasStatus));
		TestEqual(TEXT("PoisonNeedle bonus checks poison"),
			PoisonNeedle->Effects[1].Condition.ParamTag,
			FGameplayTag(WacomTags::Status_Poison));
	}

	TestEqual(TEXT("ChitinWard target mode"), ChitinWard->TargetMode, ECardTargetMode::None);
	TestEqual(TEXT("ChitinWard effects"), ChitinWard->Effects.Num(), 2);
	if (ChitinWard->Effects.Num() >= 2)
	{
		TestEqual(TEXT("ChitinWard shield"), ChitinWard->Effects[0].EffectType, FGameplayTag(WacomTags::Status_Shield));
		TestEqual(TEXT("ChitinWard heal"), ChitinWard->Effects[1].EffectType, FGameplayTag(WacomTags::Effect_Heal));
	}

	TestEqual(TEXT("AntennaSearch effects"), AntennaSearch->Effects.Num(), 2);
	if (AntennaSearch->Effects.Num() >= 2)
	{
		TestEqual(TEXT("AntennaSearch draw"), AntennaSearch->Effects[0].EffectType, FGameplayTag(WacomTags::Effect_Draw));
		TestEqual(TEXT("AntennaSearch draw source"),
			AntennaSearch->Effects[0].TargetZone,
			FGameplayTag(WacomTags::CardLocation_Draw));
		TestEqual(TEXT("AntennaSearch discard"), AntennaSearch->Effects[1].EffectType, FGameplayTag(WacomTags::Effect_Discard));
	}

	TestEqual(TEXT("MoltCut target mode"), MoltCut->TargetMode, ECardTargetMode::SingleEnemyPart);
	TestEqual(TEXT("MoltCut effects"), MoltCut->Effects.Num(), 2);
	if (MoltCut->Effects.Num() >= 2)
	{
		TestEqual(TEXT("MoltCut remove status"), MoltCut->Effects[0].EffectType, FGameplayTag(WacomTags::Effect_RemoveStatus));
		TestEqual(TEXT("MoltCut removes freeze"), MoltCut->Effects[0].TargetZone, FGameplayTag(WacomTags::Status_Freeze));
		TestEqual(TEXT("MoltCut initiative"), MoltCut->Effects[1].EffectType, FGameplayTag(WacomTags::Effect_ModifyInitiative));
		TestEqual(TEXT("MoltCut initiative amount"), MoltCut->Effects[1].Magnitude, -2);
	}

	TestEqual(TEXT("LightHusk passives"), LightHusk->Passives.Num(), 1);
	if (LightHusk->Passives.IsValidIndex(0))
	{
		TestEqual(TEXT("LightHusk OnDiscard"),
			LightHusk->Passives[0].Trigger,
			FGameplayTag(WacomTags::Passive_Trigger_OnDiscard));
		TestEqual(TEXT("LightHusk passive effect count"), LightHusk->Passives[0].Effects.Num(), 1);
	}

	TestEqual(TEXT("SilklineFeint target mode"), SilklineFeint->TargetMode, ECardTargetMode::SingleEnemyPart);
	TestEqual(TEXT("SilklineFeint zone hooks"), SilklineFeint->ZoneHooks.Num(), 1);
	if (SilklineFeint->ZoneHooks.IsValidIndex(0))
	{
		TestEqual(TEXT("SilklineFeint perfect release hook"),
			SilklineFeint->ZoneHooks[0].Trigger,
			FGameplayTag(WacomTags::ZoneHook_Trigger_OnPerfectReleaseHit));
		TestEqual(TEXT("SilklineFeint hook has no extra effects"), SilklineFeint->ZoneHooks[0].ExtraEffects.Num(), 0);
	}

	UCharacterDefinition* BugGirl = FWacomGeneratedBattleContentAssets::LoadBugGirl(*this);
	if (!TestNotNull(TEXT("BugGirl character loads"), BugGirl))
	{
		return false;
	}
	TestFalse(TEXT("PoisonNeedle is not in starter deck"), BugGirl->StarterDeck.Contains(PoisonNeedle));
	TestFalse(TEXT("ChitinWard is not in starter deck"), BugGirl->StarterDeck.Contains(ChitinWard));
	TestFalse(TEXT("AntennaSearch is not in starter deck"), BugGirl->StarterDeck.Contains(AntennaSearch));
	TestFalse(TEXT("MoltCut is not in starter deck"), BugGirl->StarterDeck.Contains(MoltCut));
	TestFalse(TEXT("LightHusk is not in starter deck"), BugGirl->StarterDeck.Contains(LightHusk));
	TestFalse(TEXT("SilklineFeint is not in starter deck"), BugGirl->StarterDeck.Contains(SilklineFeint));
	for (const TCHAR* TestCardPath : FWacomGeneratedBattleContentAssets::DebugAndTestCardPaths())
	{
		UCardDefinition* TestCard = FWacomGeneratedBattleContentAssets::LoadCardByPath(TestCardPath, *this);
		if (!TestNotNull(FString::Printf(TEXT("Debug/test card loads: %s"), TestCardPath), TestCard))
		{
			return false;
		}
		TestFalse(FString::Printf(TEXT("BugGirl starter deck excludes debug/test card: %s"), TestCardPath),
			BugGirl->StarterDeck.Contains(TestCard));
	}
	UCardDefinition* DrawByCost = FWacomGeneratedBattleContentAssets::LoadDrawByCostCard(*this);
	if (!TestNotNull(TEXT("DrawByCost test card loads"), DrawByCost))
	{
		return false;
	}
	TestEqual(TEXT("DrawByCost id"), DrawByCost->CardId, FName(TEXT("Test.DrawByCost")));
	TestEqual(TEXT("DrawByCost default cost"), DrawByCost->BaseCost, 2);
	TestEqual(TEXT("DrawByCost target mode"), DrawByCost->TargetMode, ECardTargetMode::None);
	TestEqual(TEXT("DrawByCost effects"), DrawByCost->Effects.Num(), 1);
	if (DrawByCost->Effects.IsValidIndex(0))
	{
		TestEqual(TEXT("DrawByCost effect type"), DrawByCost->Effects[0].EffectType, FGameplayTag(WacomTags::Effect_Draw));
		TestEqual(TEXT("DrawByCost source pile"), DrawByCost->Effects[0].TargetZone, FGameplayTag(WacomTags::CardLocation_Draw));
		TestEqual(TEXT("DrawByCost magnitude source"),
			DrawByCost->Effects[0].MagnitudeSource,
			FGameplayTag(WacomTags::Magnitude_Source_RuntimeCost));
	}
	for (const TCHAR* BadgeCardPath : FWacomGeneratedBattleContentAssets::BadgeDisplayTestCardPaths())
	{
		UCardDefinition* BadgeCard = FWacomGeneratedBattleContentAssets::LoadCardByPath(BadgeCardPath, *this);
		if (!TestNotNull(FString::Printf(TEXT("Badge display test card loads: %s"), BadgeCardPath), BadgeCard))
		{
			return false;
		}
		TestFalse(FString::Printf(TEXT("BugGirl starter deck excludes badge display test card: %s"), BadgeCardPath),
			BugGirl->StarterDeck.Contains(BadgeCard));
	}

	UEnemyPartDefinition* SnakeHead = FWacomGeneratedBattleContentAssets::LoadSnakeHead(*this);
	UEnemyPartDefinition* SnakeBody = FWacomGeneratedBattleContentAssets::LoadSnakeBody(*this);
	UEnemyPartDefinition* SnakeTail = FWacomGeneratedBattleContentAssets::LoadSnakeTail(*this);
	UEnemyBehaviorDefinition* SnakeBehavior = FWacomGeneratedBattleContentAssets::LoadSnakeBehavior(*this);
	if (!TestNotNull(TEXT("SnakeHead loads"), SnakeHead)
		|| !TestNotNull(TEXT("SnakeBody loads"), SnakeBody)
		|| !TestNotNull(TEXT("SnakeTail loads"), SnakeTail)
		|| !TestNotNull(TEXT("Snake behavior loads"), SnakeBehavior))
	{
		return false;
	}

	for (const UEnemyPartDefinition* Part : { SnakeHead, SnakeBody, SnakeTail })
	{
		Errors.Reset();
		TestTrue(*FString::Printf(TEXT("%s passes validation"), *GetNameSafe(Part)), ValidateEnemyPartForTest(Part, Errors));
		TestEqual(*FString::Printf(TEXT("%s validation errors"), *GetNameSafe(Part)), Errors.Num(), 0);
	}

	const FWacomEnemyIntentSetDefinition* HeadIntentSet =
		FindIntentSet(SnakeBehavior, TEXT("Default"), TEXT("Snake.Head.Sequence"));
	const FWacomEnemyIntentSetDefinition* BodyIntentSet =
		FindIntentSet(SnakeBehavior, TEXT("Default"), TEXT("Snake.Body.Sequence"));
	const FWacomEnemyIntentSetDefinition* TailIntentSet =
		FindIntentSet(SnakeBehavior, TEXT("Default"), TEXT("Snake.Tail.Sequence"));
	if (!TestNotNull(TEXT("Snake head behavior intent set"), HeadIntentSet)
		|| !TestNotNull(TEXT("Snake body behavior intent set"), BodyIntentSet)
		|| !TestNotNull(TEXT("Snake tail behavior intent set"), TailIntentSet))
	{
		return false;
	}

	TestEqual(TEXT("Snake head behavior intent count"), HeadIntentSet->Intents.Num(), 4);
	TestEqual(TEXT("Snake body behavior intent count"), BodyIntentSet->Intents.Num(), 4);
	TestEqual(TEXT("Snake tail behavior intent count"), TailIntentSet->Intents.Num(), 5);
	for (const FWacomEnemyIntentSetDefinition* IntentSet : { HeadIntentSet, BodyIntentSet, TailIntentSet })
	{
		TestTrue(*FString::Printf(TEXT("%s has player damage intent"), *IntentSet->IntentSetId.ToString()),
			HasBehaviorIntentEffect(IntentSet, WacomTags::Effect_Damage, WacomTags::Target_Player));
		TestTrue(*FString::Printf(TEXT("%s has self shield intent"), *IntentSet->IntentSetId.ToString()),
			HasBehaviorIntentEffect(IntentSet, WacomTags::Status_Shield, WacomTags::Target_Self));
		TestTrue(*FString::Printf(TEXT("%s has player status intent"), *IntentSet->IntentSetId.ToString()),
			HasBehaviorIntentEffect(IntentSet, WacomTags::Effect_ApplyStatus_Poison, WacomTags::Target_Player)
			|| HasBehaviorIntentEffect(IntentSet, WacomTags::Effect_ApplyStatus_Slow, WacomTags::Target_Player));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataBattleStarterContentBadgeDisplayAssetValidationSpec,
	"Wacom.Data.BattleStarterContent.BadgeDisplayTestCardAssetValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataBattleStarterContentBadgeDisplayAssetValidationSpec::RunTest(const FString& /*Parameters*/)
{
	UCardDefinition* BadgeDamagePoison = FWacomGeneratedBattleContentAssets::LoadBadgeDamagePoisonCard(*this);
	UCardDefinition* BadgeShieldHeal = FWacomGeneratedBattleContentAssets::LoadBadgeShieldHealCard(*this);
	UCardDefinition* BadgeDamageShieldHeal = FWacomGeneratedBattleContentAssets::LoadBadgeDamageShieldHealCard(*this);
	UCardDefinition* BadgeAllRuntimeSupported = FWacomGeneratedBattleContentAssets::LoadBadgeAllRuntimeSupportedCard(*this);
	UShopDefinition* DebugShop = FWacomGeneratedBattleContentAssets::LoadDebugSnakeShop(*this);
	if (!TestNotNull(TEXT("BadgeDamagePoison loads"), BadgeDamagePoison)
		|| !TestNotNull(TEXT("BadgeShieldHeal loads"), BadgeShieldHeal)
		|| !TestNotNull(TEXT("BadgeDamageShieldHeal loads"), BadgeDamageShieldHeal)
		|| !TestNotNull(TEXT("BadgeAllRuntimeSupported loads"), BadgeAllRuntimeSupported)
		|| !TestNotNull(TEXT("DebugSnake shop loads"), DebugShop))
	{
		return false;
	}

	TArray<FText> Errors;
	for (const UCardDefinition* Card : { BadgeDamagePoison, BadgeShieldHeal, BadgeDamageShieldHeal, BadgeAllRuntimeSupported })
	{
		Errors.Reset();
		TestTrue(*FString::Printf(TEXT("%s passes validation"), *GetNameSafe(Card)), ValidateCardForTest(Card, Errors));
		TestEqual(*FString::Printf(TEXT("%s validation errors"), *GetNameSafe(Card)), Errors.Num(), 0);
		TestTrue(*FString::Printf(TEXT("%s is sold free in DebugSnake shop"), *GetNameSafe(Card)),
			ContainsOffer(DebugShop, Card, 0));
	}

	{
		const TArray<FWacomCardViewEffectBadge> Badges =
			UWacomCardPresentationBuilder::BuildCardViewData(BadgeDamagePoison).EffectBadges;
		TestEqual(TEXT("DamagePoison badge count"), Badges.Num(), 2);
		AssertBadge(*this, Badges, 0, EWacomCardViewEffectBadgeKind::Damage, 300);
		AssertBadge(*this, Badges, 1, EWacomCardViewEffectBadgeKind::Poison, 7);
	}
	{
		const TArray<FWacomCardViewEffectBadge> Badges =
			UWacomCardPresentationBuilder::BuildCardViewData(BadgeShieldHeal).EffectBadges;
		TestEqual(TEXT("ShieldHeal badge count"), Badges.Num(), 2);
		AssertBadge(*this, Badges, 0, EWacomCardViewEffectBadgeKind::Shield, 12);
		AssertBadge(*this, Badges, 1, EWacomCardViewEffectBadgeKind::Heal, 8);
	}
	{
		const TArray<FWacomCardViewEffectBadge> Badges =
			UWacomCardPresentationBuilder::BuildCardViewData(BadgeDamageShieldHeal).EffectBadges;
		TestEqual(TEXT("DamageShieldHeal badge count"), Badges.Num(), 3);
		AssertBadge(*this, Badges, 0, EWacomCardViewEffectBadgeKind::Damage, 25);
		AssertBadge(*this, Badges, 1, EWacomCardViewEffectBadgeKind::Shield, 30);
		AssertBadge(*this, Badges, 2, EWacomCardViewEffectBadgeKind::Heal, 5);
	}
	{
		const TArray<FWacomCardViewEffectBadge> Badges =
			UWacomCardPresentationBuilder::BuildCardViewData(BadgeAllRuntimeSupported).EffectBadges;
		TestEqual(TEXT("AllRuntimeSupported badge count"), Badges.Num(), 4);
		AssertBadge(*this, Badges, 0, EWacomCardViewEffectBadgeKind::Damage, 1);
		AssertBadge(*this, Badges, 1, EWacomCardViewEffectBadgeKind::Poison, 2);
		AssertBadge(*this, Badges, 2, EWacomCardViewEffectBadgeKind::Shield, 3);
		AssertBadge(*this, Badges, 3, EWacomCardViewEffectBadgeKind::Heal, 4);
	}

	return true;
}
