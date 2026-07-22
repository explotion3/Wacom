// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Enemies/EnemyBehaviorDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Tags/WacomGameplayTags.h"
#include "Validation/EnemyBehaviorDefinitionValidation.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
	FIntentEffect MakeBehaviorValidationIntentEffect()
	{
		FIntentEffect Effect;
		Effect.EffectType = WacomTags::Effect_Damage;
		Effect.Magnitude = 2;
		Effect.Target = WacomTags::Target_Player;
		return Effect;
	}

	FWacomEnemyBehaviorIntent MakeBehaviorValidationIntent(FName IntentId)
	{
		FWacomEnemyBehaviorIntent IntentEntry;
		IntentEntry.Intent.IntentId = IntentId;
		IntentEntry.Intent.DisplayName = FText::FromName(IntentId);
		IntentEntry.Intent.Initiative = 5;
		IntentEntry.Intent.Effects = { MakeBehaviorValidationIntentEffect() };
		return IntentEntry;
	}

	UEnemyDefinition* MakeBehaviorValidationEnemy(UObject* Outer)
	{
		UEnemyDefinition* Enemy = NewObject<UEnemyDefinition>(Outer);
		Enemy->EnemyId = TEXT("Enemy.Behavior.Validation");

		UEnemyPartDefinition* Part = NewObject<UEnemyPartDefinition>(Enemy);
		Part->PartId = TEXT("Enemy.Behavior.Validation.Head");
		Part->MaxHp = 10;

		FEnemyPartSlot Slot;
		Slot.PartSlotId = TEXT("Head");
		Slot.PartDef = Part;
		Enemy->Parts = { Slot };
		return Enemy;
	}

	UEnemyBehaviorDefinition* MakeValidBehavior(UObject* Outer)
	{
		UEnemyBehaviorDefinition* Behavior = NewObject<UEnemyBehaviorDefinition>(Outer);
		Behavior->BehaviorId = TEXT("Behavior.Validation");
		Behavior->InitialPhaseId = TEXT("Default");

		FWacomEnemyIntentSetDefinition IntentSet;
		IntentSet.IntentSetId = TEXT("Head.Main");
		IntentSet.AppliesToPartSlotId = TEXT("Head");
		IntentSet.SelectorMode = EWacomEnemyIntentSelectorMode::PriorityFirst;
		IntentSet.Intents = {
			MakeBehaviorValidationIntent(TEXT("Intent.Basic")),
			MakeBehaviorValidationIntent(TEXT("Intent.Enraged")),
		};

		FWacomEnemyIntentSelectorRule Rule;
		Rule.RuleId = TEXT("Rule.Basic");
		Rule.IntentId = TEXT("Intent.Basic");
		Rule.Priority = 1;
		IntentSet.SelectorRules = { Rule };

		FWacomEnemyPhaseDefinition Phase;
		Phase.PhaseId = TEXT("Default");
		Phase.IntentSets = { IntentSet };
		Behavior->Phases = { Phase };
		return Behavior;
	}

	bool ValidateBehaviorForTest(
		const UEnemyBehaviorDefinition* Behavior,
		TArray<FText>& OutErrors,
		const UEnemyDefinition* Enemy = nullptr)
	{
		return FWacomEnemyBehaviorDefinitionValidation::Validate(Behavior, OutErrors, Enemy);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataEnemyBehaviorValidationValidSpec,
	"Wacom.Data.EnemyValidation.Behavior.Valid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataEnemyBehaviorValidationValidSpec::RunTest(const FString& /*Parameters*/)
{
	UObject* Root = GetTransientPackage();
	UEnemyDefinition* Enemy = MakeBehaviorValidationEnemy(Root);
	UEnemyBehaviorDefinition* Behavior = MakeValidBehavior(Root);

	TArray<FText> Errors;
	TestTrue(TEXT("Valid EnemyBehaviorDefinition passes validation"),
		ValidateBehaviorForTest(Behavior, Errors, Enemy));
	TestEqual(TEXT("No validation errors"), Errors.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataEnemyBehaviorValidationRequiredFieldsSpec,
	"Wacom.Data.EnemyValidation.Behavior.RequiredFields",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataEnemyBehaviorValidationRequiredFieldsSpec::RunTest(const FString& /*Parameters*/)
{
	UObject* Root = GetTransientPackage();
	UEnemyDefinition* Enemy = MakeBehaviorValidationEnemy(Root);
	TArray<FText> Errors;

	{
		UEnemyBehaviorDefinition* Behavior = MakeValidBehavior(Root);
		Behavior->BehaviorId = NAME_None;
		TestFalse(TEXT("Missing BehaviorId fails"), ValidateBehaviorForTest(Behavior, Errors, Enemy));
		TestTrue(TEXT("Missing BehaviorId has error"), Errors.Num() > 0);
	}

	{
		UEnemyBehaviorDefinition* Behavior = MakeValidBehavior(Root);
		Behavior->Phases.Reset();
		TestFalse(TEXT("Empty Phases fails"), ValidateBehaviorForTest(Behavior, Errors, Enemy));
		TestTrue(TEXT("Empty Phases has error"), Errors.Num() > 0);
	}

	{
		UEnemyBehaviorDefinition* Behavior = MakeValidBehavior(Root);
		Behavior->InitialPhaseId = TEXT("MissingPhase");
		TestFalse(TEXT("Missing initial phase fails"), ValidateBehaviorForTest(Behavior, Errors, Enemy));
		TestTrue(TEXT("Missing initial phase has error"), Errors.Num() > 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataEnemyBehaviorValidationSelectorRulesSpec,
	"Wacom.Data.EnemyValidation.Behavior.SelectorRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataEnemyBehaviorValidationSelectorRulesSpec::RunTest(const FString& /*Parameters*/)
{
	UObject* Root = GetTransientPackage();
	UEnemyDefinition* Enemy = MakeBehaviorValidationEnemy(Root);
	TArray<FText> Errors;

	{
		UEnemyBehaviorDefinition* Behavior = MakeValidBehavior(Root);
		Behavior->Phases[0].IntentSets[0].SelectorRules[0].IntentId = TEXT("Intent.Missing");
		TestFalse(TEXT("Rule referencing missing intent fails"), ValidateBehaviorForTest(Behavior, Errors, Enemy));
		TestTrue(TEXT("Missing intent rule has error"), Errors.Num() > 0);
	}

	{
		UEnemyBehaviorDefinition* Behavior = MakeValidBehavior(Root);
		Behavior->Phases[0].IntentSets[0].SelectorMode = EWacomEnemyIntentSelectorMode::Weighted;
		Behavior->Phases[0].IntentSets[0].SelectorRules[0].Weight = 0;
		TestFalse(TEXT("Weighted rule with zero weight fails"), ValidateBehaviorForTest(Behavior, Errors, Enemy));
		TestTrue(TEXT("Zero weight has error"), Errors.Num() > 0);
	}

	{
		UEnemyBehaviorDefinition* Behavior = MakeValidBehavior(Root);
		Behavior->Phases[0].IntentSets[0].AppliesToPartSlotId = TEXT("MissingPart");
		TestFalse(TEXT("IntentSet referencing missing part slot fails"),
			ValidateBehaviorForTest(Behavior, Errors, Enemy));
		TestTrue(TEXT("Missing part slot has error"), Errors.Num() > 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataEnemyBehaviorValidationConditionsSpec,
	"Wacom.Data.EnemyValidation.Behavior.Conditions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataEnemyBehaviorValidationConditionsSpec::RunTest(const FString& /*Parameters*/)
{
	UObject* Root = GetTransientPackage();
	UEnemyDefinition* Enemy = MakeBehaviorValidationEnemy(Root);
	TArray<FText> Errors;

	{
		UEnemyBehaviorDefinition* Behavior = MakeValidBehavior(Root);
		FWacomEnemyIntentCondition Condition;
		Condition.Type = EWacomEnemyIntentConditionType::PartDestroyed;
		Condition.PartSlotId = TEXT("MissingPart");
		Behavior->Phases[0].IntentSets[0].SelectorRules[0].Conditions = { Condition };
		TestFalse(TEXT("PartDestroyed missing part slot fails"),
			ValidateBehaviorForTest(Behavior, Errors, Enemy));
		TestTrue(TEXT("Missing part condition has error"), Errors.Num() > 0);
	}

	{
		UEnemyBehaviorDefinition* Behavior = MakeValidBehavior(Root);
		FWacomEnemyIntentCondition Condition;
		Condition.Type = EWacomEnemyIntentConditionType::PlayerStatusPresent;
		Behavior->Phases[0].IntentSets[0].SelectorRules[0].Conditions = { Condition };
		TestFalse(TEXT("Status condition without tag fails"),
			ValidateBehaviorForTest(Behavior, Errors, Enemy));
		TestTrue(TEXT("Missing status tag has error"), Errors.Num() > 0);
	}

	{
		UEnemyBehaviorDefinition* Behavior = MakeValidBehavior(Root);
		Behavior->Phases[0].IntentSets[0].Intents[0].Intent.Effects[0].EffectType = WacomTags::Effect_Draw;
		TestFalse(TEXT("Unsupported enemy intent effect fails"),
			ValidateBehaviorForTest(Behavior, Errors, Enemy));
		TestTrue(TEXT("Unsupported effect has error"), Errors.Num() > 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataEnemyBehaviorValidationHandAfflictionSpec,
	"Wacom.Data.EnemyValidation.Behavior.HandAfflictionContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataEnemyBehaviorValidationHandAfflictionSpec::RunTest(const FString&)
{
	UObject* Root = GetTransientPackage();
	UEnemyDefinition* Enemy = MakeBehaviorValidationEnemy(Root);
	TArray<FText> Errors;

	{
		UEnemyBehaviorDefinition* Behavior = MakeValidBehavior(Root);
		FIntentEffect& Effect = Behavior->Phases[0].IntentSets[0].Intents[0].Intent.Effects[0];
		Effect.EffectType = WacomTags::Effect_ApplyStatus_Slow;
		Effect.Magnitude = 2;
		Effect.HandAffliction.TargetCardCount = 0;
		TestFalse(TEXT("Slow requires a positive target-card count"),
			ValidateBehaviorForTest(Behavior, Errors, Enemy));
	}

	{
		UEnemyBehaviorDefinition* Behavior = MakeValidBehavior(Root);
		FIntentEffect& Effect = Behavior->Phases[0].IntentSets[0].Intents[0].Intent.Effects[0];
		Effect.EffectType = WacomTags::Effect_ApplyStatus_Twilight;
		Effect.Magnitude = 2;
		Effect.HandAffliction.Selection = EHandAfflictionSelection::RandomUnique;
		TestFalse(TEXT("Twilight must target the whole current hand"),
			ValidateBehaviorForTest(Behavior, Errors, Enemy));
	}

	{
		UEnemyBehaviorDefinition* Behavior = MakeValidBehavior(Root);
		FIntentEffect& Effect = Behavior->Phases[0].IntentSets[0].Intents[0].Intent.Effects[0];
		Effect.HandAffliction.Selection = EHandAfflictionSelection::RandomUnique;
		TestFalse(TEXT("Non-status intent cannot author hand-affliction delivery"),
			ValidateBehaviorForTest(Behavior, Errors, Enemy));
	}

	return true;
}
