// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Tags/WacomGameplayTags.h"
#include "Validation/CardDefinitionValidation.h"
#include "Validation/CharacterDefinitionValidation.h"
#include "Validation/EnemyDefinitionValidation.h"
#include "Validation/EnemyPartDefinitionValidation.h"

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
		Part->InitialIntentIndex = 0;
		Part->ExperienceReward = 1;
		return Part;
	}

	UEnemyDefinition* MakeValidEnemyForValidation(UObject* Outer)
	{
		UEnemyDefinition* Enemy = NewObject<UEnemyDefinition>(Outer);
		Enemy->EnemyId = TEXT("Enemy.Validation");
		Enemy->DisplayName = FText::FromString(TEXT("校验敌人"));

		FEnemyPartSlot Slot;
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

	bool ValidateCharacterForTest(const UCharacterDefinition* Character, TArray<FText>& OutErrors)
	{
		return FWacomCharacterDefinitionValidation::Validate(Character, OutErrors);
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
		Card->Effects[0].Magnitude = 0;
		Card->Effects[0].MagnitudeSource = WacomTags::Magnitude_Source_RuntimeCost;
		TArray<FText> Errors;
		TestFalse(TEXT("RuntimeCost source cannot drive draw"), ValidateCardForTest(Card.Get(), Errors));
		TestTrue(TEXT("RuntimeCost source mismatch reported"), ErrorsContain(Errors, TEXT("MagnitudeSource")));
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
	TestTrue(TEXT("Valid EnemyPartDefinition with empty IntentSequence passes validation"), ValidateEnemyPartForTest(Part.Get(), Errors));
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
		Part->InitialIntentIndex = -1;
		TArray<FText> Errors;
		TestFalse(TEXT("Negative InitialIntentIndex fails"), ValidateEnemyPartForTest(Part.Get(), Errors));
		TestTrue(TEXT("Negative InitialIntentIndex has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UEnemyPartDefinition> Part(MakeValidEnemyPartForValidation(GetTransientPackage()));
		Part->IntentSequence = { MakeValidIntentDefinition() };
		Part->InitialIntentIndex = 1;
		TArray<FText> Errors;
		TestFalse(TEXT("InitialIntentIndex out of bounds fails"), ValidateEnemyPartForTest(Part.Get(), Errors));
		TestTrue(TEXT("InitialIntentIndex out of bounds has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UEnemyPartDefinition> Part(MakeValidEnemyPartForValidation(GetTransientPackage()));
		Part->ExperienceReward = -1;
		TArray<FText> Errors;
		TestFalse(TEXT("Negative ExperienceReward fails"), ValidateEnemyPartForTest(Part.Get(), Errors));
		TestTrue(TEXT("Negative ExperienceReward has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UEnemyPartDefinition> Part(MakeValidEnemyPartForValidation(GetTransientPackage()));
		FIntentDefinition Intent = MakeValidIntentDefinition();
		Intent.IntentId = NAME_None;
		Part->IntentSequence = { Intent };
		TArray<FText> Errors;
		TestFalse(TEXT("Missing IntentId fails"), ValidateEnemyPartForTest(Part.Get(), Errors));
		TestTrue(TEXT("Missing IntentId has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UEnemyPartDefinition> Part(MakeValidEnemyPartForValidation(GetTransientPackage()));
		FIntentDefinition Intent = MakeValidIntentDefinition();
		Intent.Effects[0].EffectType = FGameplayTag();
		Part->IntentSequence = { Intent };
		TArray<FText> Errors;
		TestFalse(TEXT("Invalid IntentEffect EffectType fails"), ValidateEnemyPartForTest(Part.Get(), Errors));
		TestTrue(TEXT("Invalid IntentEffect EffectType has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UEnemyPartDefinition> Part(MakeValidEnemyPartForValidation(GetTransientPackage()));
		FIntentDefinition Intent = MakeValidIntentDefinition();
		Intent.Effects[0].Magnitude = -1;
		Part->IntentSequence = { Intent };
		TArray<FText> Errors;
		TestFalse(TEXT("Negative IntentEffect Magnitude fails"), ValidateEnemyPartForTest(Part.Get(), Errors));
		TestTrue(TEXT("Negative IntentEffect Magnitude has error"), Errors.Num() > 0);
	}

	{
		TStrongObjectPtr<UEnemyPartDefinition> Part(MakeValidEnemyPartForValidation(GetTransientPackage()));
		FIntentDefinition Intent = MakeValidIntentDefinition();
		Intent.Effects[0].Duration = -1;
		Part->IntentSequence = { Intent };
		TArray<FText> Errors;
		TestFalse(TEXT("Negative IntentEffect Duration fails"), ValidateEnemyPartForTest(Part.Get(), Errors));
		TestTrue(TEXT("Negative IntentEffect Duration has error"), Errors.Num() > 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataEnemyPartValidationIntentRuleContentContractSpec,
	"Wacom.Data.EnemyPart.Validation.IntentRuleContentContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataEnemyPartValidationIntentRuleContentContractSpec::RunTest(const FString& /*Parameters*/)
{
	{
		TStrongObjectPtr<UEnemyPartDefinition> Part(MakeValidEnemyPartForValidation(GetTransientPackage()));
		FIntentDefinition Intent = MakeValidIntentDefinition();
		Intent.Effects[0].EffectType = WacomTags::Effect_Card_AddCost;
		Intent.Effects[0].Target = WacomTags::Target_SelectedHandCard;
		Part->IntentSequence = { Intent };
		TArray<FText> Errors;
		TestFalse(TEXT("Card-only effect in enemy intent fails"), ValidateEnemyPartForTest(Part.Get(), Errors));
		TestTrue(TEXT("Enemy intent unsupported effect reported"), ErrorsContain(Errors, TEXT("未支持")));
	}

	{
		TStrongObjectPtr<UEnemyPartDefinition> Part(MakeValidEnemyPartForValidation(GetTransientPackage()));
		FIntentDefinition Intent = MakeValidIntentDefinition();
		Intent.Effects[0].Target = WacomTags::Target_SingleEnemyPart;
		Part->IntentSequence = { Intent };
		TArray<FText> Errors;
		TestFalse(TEXT("Enemy intent cannot target enemy part"), ValidateEnemyPartForTest(Part.Get(), Errors));
		TestTrue(TEXT("Enemy intent target issue reported"), ErrorsContain(Errors, TEXT("Target")));
	}

	{
		TStrongObjectPtr<UEnemyPartDefinition> Part(MakeValidEnemyPartForValidation(GetTransientPackage()));
		FIntentDefinition Intent = MakeValidIntentDefinition();
		Intent.Effects[0].EffectType = WacomTags::Status_Shield;
		Intent.Effects[0].Magnitude = 4;
		Intent.Effects[0].Target = WacomTags::Target_Player;
		Part->IntentSequence = { Intent };
		TArray<FText> Errors;
		TestFalse(TEXT("Enemy intent shield cannot target player"), ValidateEnemyPartForTest(Part.Get(), Errors));
		TestTrue(TEXT("Enemy player shield target issue reported"), ErrorsContain(Errors, TEXT("Target")));
	}

	{
		TStrongObjectPtr<UEnemyPartDefinition> Part(MakeValidEnemyPartForValidation(GetTransientPackage()));
		FIntentDefinition Intent = MakeValidIntentDefinition();
		Intent.Effects[0].Target = WacomTags::Target_Self;
		Part->IntentSequence = { Intent };
		TArray<FText> Errors;
		TestFalse(TEXT("Enemy intent damage cannot target self"), ValidateEnemyPartForTest(Part.Get(), Errors));
		TestTrue(TEXT("Enemy self damage target issue reported"), ErrorsContain(Errors, TEXT("Target")));
	}

	{
		TStrongObjectPtr<UEnemyPartDefinition> Part(MakeValidEnemyPartForValidation(GetTransientPackage()));
		FIntentDefinition Intent = MakeValidIntentDefinition();
		Intent.Effects[0].EffectType = WacomTags::Status_Shield;
		Intent.Effects[0].Magnitude = 4;
		Intent.Effects[0].Target = WacomTags::Target_Self;
		Part->IntentSequence = { Intent };
		TArray<FText> Errors;
		TestTrue(TEXT("Enemy intent self shield passes"), ValidateEnemyPartForTest(Part.Get(), Errors));
		TestEqual(TEXT("No enemy self shield errors"), Errors.Num(), 0);
	}

	{
		TStrongObjectPtr<UEnemyPartDefinition> Part(MakeValidEnemyPartForValidation(GetTransientPackage()));
		FIntentDefinition Intent = MakeValidIntentDefinition();
		Intent.Effects[0].Magnitude = 0;
		Part->IntentSequence = { Intent };
		TArray<FText> Errors;
		TestFalse(TEXT("Enemy intent positive effect magnitude must be positive"), ValidateEnemyPartForTest(Part.Get(), Errors));
		TestTrue(TEXT("Enemy intent magnitude issue reported"), ErrorsContain(Errors, TEXT("Magnitude")));
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
	UCharacterDefinition* BugGirl = LoadObject<UCharacterDefinition>(
		nullptr,
		TEXT("/Game/Wacom/Data/Characters/DA_Character_BugGirl.DA_Character_BugGirl"));
	UEnemyDefinition* Snake = LoadObject<UEnemyDefinition>(
		nullptr,
		TEXT("/Game/Wacom/Data/Enemies/Snake/DA_Enemy_Snake.DA_Enemy_Snake"));

	UEnemyPartDefinition* SnakeHead = LoadObject<UEnemyPartDefinition>(
		nullptr,
		TEXT("/Game/Wacom/Data/Enemies/Snake/DA_Part_Snake_Head.DA_Part_Snake_Head"));
	UEnemyPartDefinition* SnakeBody = LoadObject<UEnemyPartDefinition>(
		nullptr,
		TEXT("/Game/Wacom/Data/Enemies/Snake/DA_Part_Snake_Body.DA_Part_Snake_Body"));
	UEnemyPartDefinition* SnakeTail = LoadObject<UEnemyPartDefinition>(
		nullptr,
		TEXT("/Game/Wacom/Data/Enemies/Snake/DA_Part_Snake_Tail.DA_Part_Snake_Tail"));

	bool bAllAssetsLoaded = true;
	bAllAssetsLoaded &= TestNotNull(TEXT("BugGirl character asset loads"), BugGirl);
	bAllAssetsLoaded &= TestNotNull(TEXT("Snake enemy asset loads"), Snake);
	bAllAssetsLoaded &= TestNotNull(TEXT("Snake head part asset loads"), SnakeHead);
	bAllAssetsLoaded &= TestNotNull(TEXT("Snake body part asset loads"), SnakeBody);
	bAllAssetsLoaded &= TestNotNull(TEXT("Snake tail part asset loads"), SnakeTail);

	const TCHAR* CardAssetPaths[] = {
		TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_LeftHand.DA_Card_LeftHand"),
		TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_RightHand.DA_Card_RightHand"),
		TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_ZhaoguangMudie.DA_Card_ZhaoguangMudie"),
		TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_FuxiaoFeie.DA_Card_FuxiaoFeie"),
		TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_ChifuGongyi.DA_Card_ChifuGongyi"),
		TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_ShuoguangDie.DA_Card_ShuoguangDie"),
		TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_Muling.DA_Card_Muling"),
		TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_BugGirlBag.DA_Card_BugGirlBag"),
		TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_ZhujianRongnang.DA_Card_ZhujianRongnang"),
		TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_MuseiYinchongdeng.DA_Card_MuseiYinchongdeng"),
		TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_DebugKey.DA_Card_DebugKey"),
		TEXT("/Game/Wacom/Data/Cards/Rewards/DA_Card_PoisonFang.DA_Card_PoisonFang")
	};

	TArray<UCardDefinition*> GeneratedCards;
	for (const TCHAR* CardAssetPath : CardAssetPaths)
	{
		UCardDefinition* Card = LoadObject<UCardDefinition>(nullptr, CardAssetPath);
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

	Errors.Reset();
	TestTrue(TEXT("Snake head asset passes validation"), ValidateEnemyPartForTest(SnakeHead, Errors));
	TestEqual(TEXT("Snake head validation errors"), Errors.Num(), 0);

	Errors.Reset();
	TestTrue(TEXT("Snake body asset passes validation"), ValidateEnemyPartForTest(SnakeBody, Errors));
	TestEqual(TEXT("Snake body validation errors"), Errors.Num(), 0);

	Errors.Reset();
	TestTrue(TEXT("Snake tail asset passes validation"), ValidateEnemyPartForTest(SnakeTail, Errors));
	TestEqual(TEXT("Snake tail validation errors"), Errors.Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataDebugKeyAssetLoadsSpec,
	"Wacom.Data.DebugKeyAsset.DebugKeyAssetLoads",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataDebugKeyAssetLoadsSpec::RunTest(const FString& /*Parameters*/)
{
	UCardDefinition* DebugKey = LoadObject<UCardDefinition>(
		nullptr,
		TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_DebugKey.DA_Card_DebugKey"));
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
	"Wacom.Data.DebugKeyAsset.DebugKeyIsInBugGirlStarterDeck",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataDebugKeyBugGirlStarterDeckSpec::RunTest(const FString& /*Parameters*/)
{
	UCardDefinition* DebugKey = LoadObject<UCardDefinition>(
		nullptr,
		TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_DebugKey.DA_Card_DebugKey"));
	UCharacterDefinition* BugGirl = LoadObject<UCharacterDefinition>(
		nullptr,
		TEXT("/Game/Wacom/Data/Characters/DA_Character_BugGirl.DA_Character_BugGirl"));
	if (!TestNotNull(TEXT("DebugKey asset loads"), DebugKey)
		|| !TestNotNull(TEXT("BugGirl character asset loads"), BugGirl))
	{
		return false;
	}

	TestTrue(TEXT("BugGirl starter deck contains DebugKey"),
		BugGirl->StarterDeck.Contains(DebugKey));
	return true;
}
