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
		TEXT("/Game/Wacom/Characters/DA_Character_BugGirl.DA_Character_BugGirl"));
	UEnemyDefinition* Snake = LoadObject<UEnemyDefinition>(
		nullptr,
		TEXT("/Game/Wacom/Enemies/Snake/DA_Enemy_Snake.DA_Enemy_Snake"));

	UEnemyPartDefinition* SnakeHead = LoadObject<UEnemyPartDefinition>(
		nullptr,
		TEXT("/Game/Wacom/Enemies/Snake/DA_Part_Snake_Head.DA_Part_Snake_Head"));
	UEnemyPartDefinition* SnakeBody = LoadObject<UEnemyPartDefinition>(
		nullptr,
		TEXT("/Game/Wacom/Enemies/Snake/DA_Part_Snake_Body.DA_Part_Snake_Body"));
	UEnemyPartDefinition* SnakeTail = LoadObject<UEnemyPartDefinition>(
		nullptr,
		TEXT("/Game/Wacom/Enemies/Snake/DA_Part_Snake_Tail.DA_Part_Snake_Tail"));

	bool bAllAssetsLoaded = true;
	bAllAssetsLoaded &= TestNotNull(TEXT("BugGirl character asset loads"), BugGirl);
	bAllAssetsLoaded &= TestNotNull(TEXT("Snake enemy asset loads"), Snake);
	bAllAssetsLoaded &= TestNotNull(TEXT("Snake head part asset loads"), SnakeHead);
	bAllAssetsLoaded &= TestNotNull(TEXT("Snake body part asset loads"), SnakeBody);
	bAllAssetsLoaded &= TestNotNull(TEXT("Snake tail part asset loads"), SnakeTail);

	const TCHAR* CardAssetPaths[] = {
		TEXT("/Game/Wacom/Cards/BugGirl/DA_Card_LeftHand.DA_Card_LeftHand"),
		TEXT("/Game/Wacom/Cards/BugGirl/DA_Card_RightHand.DA_Card_RightHand"),
		TEXT("/Game/Wacom/Cards/BugGirl/DA_Card_ZhaoguangMudie.DA_Card_ZhaoguangMudie"),
		TEXT("/Game/Wacom/Cards/BugGirl/DA_Card_FuxiaoFeie.DA_Card_FuxiaoFeie"),
		TEXT("/Game/Wacom/Cards/BugGirl/DA_Card_ChifuGongyi.DA_Card_ChifuGongyi"),
		TEXT("/Game/Wacom/Cards/BugGirl/DA_Card_ShuoguangDie.DA_Card_ShuoguangDie"),
		TEXT("/Game/Wacom/Cards/BugGirl/DA_Card_Muling.DA_Card_Muling"),
		TEXT("/Game/Wacom/Cards/BugGirl/DA_Card_BugGirlBag.DA_Card_BugGirlBag"),
		TEXT("/Game/Wacom/Cards/BugGirl/DA_Card_ZhujianRongnang.DA_Card_ZhujianRongnang"),
		TEXT("/Game/Wacom/Cards/BugGirl/DA_Card_MuseiYinchongdeng.DA_Card_MuseiYinchongdeng"),
		TEXT("/Game/Wacom/Cards/Rewards/DA_Card_PoisonFang.DA_Card_PoisonFang")
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
