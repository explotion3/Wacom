// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Characters/CharacterDefinition.h"
#include "Commands/BattleCommand.h"
#include "Enemies/EnemyBehaviorDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Events/BattleEvent.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"

namespace
{
	FIntentEffect MakeEnemyBehaviorEffect(
		const FGameplayTag& EffectType,
		int32 Magnitude,
		const FGameplayTag& Target)
	{
		FIntentEffect Effect;
		Effect.EffectType = EffectType;
		Effect.Magnitude = Magnitude;
		Effect.Target = Target;
		return Effect;
	}

	FWacomEnemyBehaviorIntent MakeEnemyBehaviorIntent(
		FName IntentId,
		int32 Initiative,
		int32 Damage,
		int32 PriorityResistance = 0)
	{
		FWacomEnemyBehaviorIntent IntentEntry;
		IntentEntry.Intent.IntentId = IntentId;
		IntentEntry.Intent.DisplayName = FText::FromName(IntentId);
		IntentEntry.Intent.Initiative = Initiative;
		IntentEntry.Intent.ResistanceValue = PriorityResistance;
		IntentEntry.Intent.Effects = {
			MakeEnemyBehaviorEffect(WacomTags::Effect_Damage, Damage, WacomTags::Target_Player)
		};
		return IntentEntry;
	}

	UEnemyBehaviorDefinition* MakeSequenceBehavior(UObject* Outer)
	{
		UEnemyBehaviorDefinition* Behavior = NewObject<UEnemyBehaviorDefinition>(Outer);
		Behavior->BehaviorId = TEXT("Behavior.Sequence");
		Behavior->InitialPhaseId = TEXT("Default");

		FWacomEnemyIntentSetDefinition IntentSet;
		IntentSet.IntentSetId = TEXT("Core.Sequence");
		IntentSet.AppliesToPartSlotId = TEXT("Core");
		IntentSet.SelectorMode = EWacomEnemyIntentSelectorMode::Sequence;
		IntentSet.Intents = {
			MakeEnemyBehaviorIntent(TEXT("Intent.First"), 1, 1),
			MakeEnemyBehaviorIntent(TEXT("Intent.Second"), 9, 2),
		};

		FWacomEnemyPhaseDefinition Phase;
		Phase.PhaseId = TEXT("Default");
		Phase.IntentSets = { IntentSet };
		Behavior->Phases = { Phase };
		return Behavior;
	}

	UEnemyBehaviorDefinition* MakePriorityHpBehavior(UObject* Outer)
	{
		UEnemyBehaviorDefinition* Behavior = NewObject<UEnemyBehaviorDefinition>(Outer);
		Behavior->BehaviorId = TEXT("Behavior.PriorityHp");
		Behavior->InitialPhaseId = TEXT("Default");

		FWacomEnemyIntentSetDefinition IntentSet;
		IntentSet.IntentSetId = TEXT("Core.Priority");
		IntentSet.AppliesToPartSlotId = TEXT("Core");
		IntentSet.SelectorMode = EWacomEnemyIntentSelectorMode::PriorityFirst;
		IntentSet.Intents = {
			MakeEnemyBehaviorIntent(TEXT("Intent.Calm"), 8, 1),
			MakeEnemyBehaviorIntent(TEXT("Intent.Wounded"), 6, 3),
		};

		FWacomEnemyIntentSelectorRule CalmRule;
		CalmRule.RuleId = TEXT("Rule.Calm");
		CalmRule.IntentId = TEXT("Intent.Calm");
		CalmRule.Priority = 1;

		FWacomEnemyIntentCondition WoundedCondition;
		WoundedCondition.Type = EWacomEnemyIntentConditionType::OwnHpAtOrBelowRatio;
		WoundedCondition.HpRatioThreshold = 0.5f;

		FWacomEnemyIntentSelectorRule WoundedRule;
		WoundedRule.RuleId = TEXT("Rule.Wounded");
		WoundedRule.IntentId = TEXT("Intent.Wounded");
		WoundedRule.Priority = 10;
		WoundedRule.Conditions = { WoundedCondition };
		IntentSet.SelectorRules = { CalmRule, WoundedRule };

		FWacomEnemyPhaseDefinition Phase;
		Phase.PhaseId = TEXT("Default");
		Phase.IntentSets = { IntentSet };
		Behavior->Phases = { Phase };
		return Behavior;
	}

	UEnemyBehaviorDefinition* MakePlayerStatusBehavior(UObject* Outer)
	{
		UEnemyBehaviorDefinition* Behavior = NewObject<UEnemyBehaviorDefinition>(Outer);
		Behavior->BehaviorId = TEXT("Behavior.PlayerStatus");
		Behavior->InitialPhaseId = TEXT("Default");

		FWacomEnemyIntentSetDefinition IntentSet;
		IntentSet.IntentSetId = TEXT("Core.Status");
		IntentSet.AppliesToPartSlotId = TEXT("Core");
		IntentSet.SelectorMode = EWacomEnemyIntentSelectorMode::PriorityFirst;
		IntentSet.Intents = {
			MakeEnemyBehaviorIntent(TEXT("Intent.Normal"), 8, 1),
			MakeEnemyBehaviorIntent(TEXT("Intent.PoisonFollowup"), 5, 4),
		};

		FWacomEnemyIntentSelectorRule NormalRule;
		NormalRule.RuleId = TEXT("Rule.Normal");
		NormalRule.IntentId = TEXT("Intent.Normal");
		NormalRule.Priority = 1;

		FWacomEnemyIntentCondition PoisonCondition;
		PoisonCondition.Type = EWacomEnemyIntentConditionType::PlayerStatusPresent;
		PoisonCondition.StatusTag = WacomTags::Status_Poison;

		FWacomEnemyIntentSelectorRule PoisonRule;
		PoisonRule.RuleId = TEXT("Rule.PoisonFollowup");
		PoisonRule.IntentId = TEXT("Intent.PoisonFollowup");
		PoisonRule.Priority = 10;
		PoisonRule.Conditions = { PoisonCondition };
		IntentSet.SelectorRules = { NormalRule, PoisonRule };

		FWacomEnemyPhaseDefinition Phase;
		Phase.PhaseId = TEXT("Default");
		Phase.IntentSets = { IntentSet };
		Behavior->Phases = { Phase };
		return Behavior;
	}

	UEnemyDefinition* MakeBehaviorEnemy(FWacomBattleFixture& Fx, UEnemyBehaviorDefinition* Behavior, int32 Hp = 30)
	{
		UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemyWithIntentDamage(Hp, /*Initiative*/20, /*IntentResist*/0, /*Damage*/1);
		Enemy->DefaultBehavior = Behavior;
		Enemy->DefaultPhaseId = TEXT("Default");
		Enemy->Parts[0].PartSlotId = TEXT("Core");
		Enemy->Parts[0].PartDef->PartId = TEXT("Enemy.Behavior.Core");
		return Enemy;
	}

	UCharacterDefinition* MakeBehaviorCharacter(FWacomBattleFixture& Fx, const TArray<UCardDefinition*>& Cards)
	{
		TArray<UCardDefinition*> Deck = Cards;
		while (Deck.Num() < 5)
		{
			Deck.Add(Fx.MakeNoopCard(/*Cost*/0));
		}
		return Fx.MakeCharacter(Fx.MakeNoopCard(/*Cost*/0), Fx.MakeNoopCard(/*Cost*/0), Deck);
	}

	UCardDefinition* MakeBehaviorDamageCard(FWacomBattleFixture& Fx, int32 Damage)
	{
		return Fx.MakeSimpleDamageCard(/*Cost*/0, Damage);
	}

	UCardDefinition* MakePoisonPlayerCard(FWacomBattleFixture& Fx)
	{
		UCardDefinition* Card = Fx.MakeNoopCard(/*Cost*/0);
		Card->TargetMode = ECardTargetMode::Self;

		FCardEffect Effect;
		Effect.EffectType = WacomTags::Effect_ApplyStatus_Poison;
		Effect.Magnitude = 1;
		Effect.Target = WacomTags::Target_Player;
		Card->Effects = { Effect };
		return Card;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleEnemyBehaviorSequenceInitialAndAdvanceSpec,
	"Wacom.Battle.EnemyBehavior.SequenceInitialAndAdvance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleEnemyBehaviorSequenceInitialAndAdvanceSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UEnemyBehaviorDefinition* Behavior = MakeSequenceBehavior(GetTransientPackage());
	UEnemyDefinition* Enemy = MakeBehaviorEnemy(Fx, Behavior);
	UCharacterDefinition* Character = MakeBehaviorCharacter(Fx, {});
	const FWacomInitializedBattleSession Initialized =
		Fx.CreateInitializedSession(Character, Enemy, /*Seed*/1);
	UBattleSession* Session = Initialized.Session;

	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FEnemyPartSnapshot* Part = FWacomBattleFixture::GetEnemyPartSnapshot(Snapshot, 0);
	TestNotNull(TEXT("Part snapshot exists"), Part);
	if (!Part)
	{
		return false;
	}
	TestEqual(TEXT("Initial intent comes from behavior sequence"),
		Part->CurrentIntent.IntentId,
		FName(TEXT("Intent.First")));
	TestEqual(TEXT("Snapshot exposes current phase"),
		Part->CurrentPhaseId,
		FName(TEXT("Default")));
	TestEqual(TEXT("Snapshot exposes current intent set"),
		Part->CurrentIntentSetId,
		FName(TEXT("Core.Sequence")));
	TestEqual(TEXT("Snapshot exposes selected intent id"),
		Part->CurrentIntentId,
		FName(TEXT("Intent.First")));
	TestEqual(TEXT("Initial initiative comes from behavior intent"),
		Part->CurrentInitiative,
		1);

	const TArray<FBattleEvent>& InitialEvents = Initialized.Initialization.Events;
	TestTrue(TEXT("Initial phase event emitted"),
		InitialEvents.ContainsByPredicate([](const FBattleEvent& Event)
		{
			return Event.Type == EBattleEventType::EnemyPhaseChanged
				&& Event.EnemyPhaseId == FName(TEXT("Default"));
		}));
	TestTrue(TEXT("Initial intent selected event emitted"),
		InitialEvents.ContainsByPredicate([](const FBattleEvent& Event)
		{
			return Event.Type == EBattleEventType::EnemyIntentSelected
				&& Event.IntentId == FName(TEXT("Intent.First"))
				&& Event.IntentSetId == FName(TEXT("Core.Sequence"))
				&& Event.EnemyPhaseId == FName(TEXT("Default"))
				&& Event.Amount == 1;
		}));

	const FBattleResolution WaitResolution = Session->ResolveCommand(FBattleCommand::MakeWait());
	TestTrue(TEXT("Wait triggers first intent"), WaitResolution.IsOk());
	const TArray<FBattleEvent>& WaitEvents = WaitResolution.Events;
	Snapshot = Session->BuildSnapshot();
	Part = FWacomBattleFixture::GetEnemyPartSnapshot(Snapshot, 0);
	TestNotNull(TEXT("Part snapshot exists after action"), Part);
	if (Part)
	{
		TestEqual(TEXT("Action refresh advances sequence"),
			Part->CurrentIntent.IntentId,
			FName(TEXT("Intent.Second")));
		TestEqual(TEXT("Snapshot updates selected intent id after refresh"),
			Part->CurrentIntentId,
			FName(TEXT("Intent.Second")));
		TestEqual(TEXT("Snapshot keeps current intent set after refresh"),
			Part->CurrentIntentSetId,
			FName(TEXT("Core.Sequence")));
		TestEqual(TEXT("Player took first intent damage"),
			Snapshot.Player.CurrentHp,
			99);
	}
	TestTrue(TEXT("Action refresh emits selected intent event"),
		WaitEvents.ContainsByPredicate([](const FBattleEvent& Event)
		{
			return Event.Type == EBattleEventType::EnemyIntentSelected
				&& Event.IntentId == FName(TEXT("Intent.Second"))
				&& Event.IntentSetId == FName(TEXT("Core.Sequence"))
				&& Event.EnemyPhaseId == FName(TEXT("Default"))
				&& Event.Amount == 9;
		}));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleEnemyBehaviorPriorityHpConditionSpec,
	"Wacom.Battle.EnemyBehavior.PriorityHpCondition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleEnemyBehaviorPriorityHpConditionSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UEnemyBehaviorDefinition* Behavior = MakePriorityHpBehavior(GetTransientPackage());
	UCardDefinition* DamageCard = MakeBehaviorDamageCard(Fx, /*Damage*/20);
	UEnemyDefinition* Enemy = MakeBehaviorEnemy(Fx, Behavior, /*Hp*/30);
	UCharacterDefinition* Character = MakeBehaviorCharacter(Fx, { DamageCard });
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, /*Seed*/1);

	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FEnemyPartSnapshot* Part = FWacomBattleFixture::GetEnemyPartSnapshot(Snapshot, 0);
	TestNotNull(TEXT("Initial part exists"), Part);
	if (!Part)
	{
		return false;
	}
	TestEqual(TEXT("Initial priority intent is calm"),
		Part->CurrentIntent.IntentId,
		FName(TEXT("Intent.Calm")));

	const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, DamageCard->CardId);
	const FGuid PartId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);
	TestTrue(TEXT("Damage card in hand"), CardId.IsValid());
	TestTrue(TEXT("Part id valid"), PartId.IsValid());
	if (!CardId.IsValid() || !PartId.IsValid())
	{
		return false;
	}

	TestTrue(TEXT("Damage part below hp threshold"),
		Session->ResolveCommand(FWacomBattleFixture::MakePlayCardOnPartInstance(Snapshot, CardId, PartId)).IsOk());

	TestTrue(TEXT("EndTurn triggers calm action then refreshes to wounded"),
		Session->ResolveCommand(FBattleCommand::MakeEndTurn()).IsOk());
	Snapshot = Session->BuildSnapshot();
	Part = FWacomBattleFixture::GetEnemyPartSnapshot(Snapshot, 0);
	TestNotNull(TEXT("Part exists after end turn"), Part);
	if (Part)
	{
		TestEqual(TEXT("HP condition switches to wounded intent"),
			Part->CurrentIntent.IntentId,
			FName(TEXT("Intent.Wounded")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleEnemyBehaviorPlayerStatusConditionSpec,
	"Wacom.Battle.EnemyBehavior.PlayerStatusCondition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleEnemyBehaviorPlayerStatusConditionSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UEnemyBehaviorDefinition* Behavior = MakePlayerStatusBehavior(GetTransientPackage());
	UCardDefinition* PoisonPlayer = MakePoisonPlayerCard(Fx);
	UEnemyDefinition* Enemy = MakeBehaviorEnemy(Fx, Behavior, /*Hp*/30);
	UCharacterDefinition* Character = MakeBehaviorCharacter(Fx, { PoisonPlayer });
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, /*Seed*/1);

	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FEnemyPartSnapshot* Part = FWacomBattleFixture::GetEnemyPartSnapshot(Snapshot, 0);
	TestNotNull(TEXT("Initial part exists"), Part);
	if (!Part)
	{
		return false;
	}
	TestEqual(TEXT("Initial status condition intent is normal"),
		Part->CurrentIntent.IntentId,
		FName(TEXT("Intent.Normal")));

	const FGuid PoisonCardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, PoisonPlayer->CardId);
	TestTrue(TEXT("Poison player card in hand"), PoisonCardId.IsValid());
	if (!PoisonCardId.IsValid())
	{
		return false;
	}
	TestTrue(TEXT("Apply poison to player"),
		Session->ResolveCommand(FBattleCommand::MakePlayCard(PoisonCardId)).IsOk());

	TestTrue(TEXT("EndTurn triggers normal action then refreshes by player status"),
		Session->ResolveCommand(FBattleCommand::MakeEndTurn()).IsOk());
	Snapshot = Session->BuildSnapshot();
	Part = FWacomBattleFixture::GetEnemyPartSnapshot(Snapshot, 0);
	TestNotNull(TEXT("Part exists after status refresh"), Part);
	if (Part)
	{
		TestEqual(TEXT("Player status condition selects followup intent"),
			Part->CurrentIntent.IntentId,
			FName(TEXT("Intent.PoisonFollowup")));
	}
	return true;
}
