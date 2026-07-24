// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Commands/BattleCommand.h"
#include "Enemies/EnemyBehaviorDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/IntentEffect.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"

namespace WacomBattleIntentPublicPreviewSpec
{
	FIntentEffect MakeEffect(
		const FGameplayTag EffectType,
		const int32 Magnitude,
		const FGameplayTag Target)
	{
		FIntentEffect Effect;
		Effect.EffectType = EffectType;
		Effect.Magnitude = Magnitude;
		Effect.Target = Target;
		return Effect;
	}

	void SetIntentEffects(
		UEnemyDefinition& Enemy,
		const TArray<FIntentEffect>& Effects)
	{
		UEnemyBehaviorDefinition* Behavior = Enemy.DefaultBehavior.Get();
		check(Behavior && Behavior->Phases.IsValidIndex(0));
		check(Behavior->Phases[0].IntentSets.IsValidIndex(0));
		FWacomEnemyIntentSetDefinition& IntentSet =
			Behavior->Phases[0].IntentSets[0];
		check(IntentSet.Intents.IsValidIndex(0));
		IntentSet.Intents[0].Intent.Effects = Effects;
	}

	UBattleSession* CreateSession(
		FWacomBattleFixture& Fixture,
		UEnemyDefinition* Enemy,
		UCardDefinition* RequiredCard = nullptr)
	{
		TArray<UCardDefinition*> Deck;
		if (RequiredCard)
		{
			Deck.Add(RequiredCard);
		}
		while (Deck.Num() < 5)
		{
			Deck.Add(Fixture.MakeNoopCard(0));
		}
		return Fixture.CreateSession(
			Fixture.MakeCharacter(
				Fixture.MakeNoopCard(0),
				Fixture.MakeNoopCard(0),
				Deck),
			Enemy,
			17);
	}

	FIntentSnapshot GetIntent(const UBattleSession& Session)
	{
		const FBattleSnapshot Snapshot = Session.BuildSnapshot();
		const FEnemyPartSnapshot* Part =
			FWacomBattleFixture::GetEnemyPartSnapshot(Snapshot, 0);
		return Part ? Part->CurrentIntent : FIntentSnapshot();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleIntentPublicPreviewTargetNormalizationSpec,
	"Wacom.Battle.IntentPublicPreview.TargetNormalizationAndOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleIntentPublicPreviewTargetNormalizationSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleIntentPublicPreviewSpec;
	FWacomBattleFixture Fixture;
	UEnemyDefinition* Enemy =
		Fixture.MakeSinglePartEnemyWithIntentDamage(100, 3, 1);

	FIntentEffect Damage = MakeEffect(
		WacomTags::Effect_Damage,
		3,
		WacomTags::Target_Player);
	FIntentEffect Shield = MakeEffect(
		WacomTags::Status_Shield,
		4,
		WacomTags::Target_Self);
	FIntentEffect Slow = MakeEffect(
		WacomTags::Effect_ApplyStatus_Slow,
		2,
		WacomTags::Target_Player);
	Slow.HandAffliction.Selection = EHandAfflictionSelection::Default;
	Slow.HandAffliction.TargetCardCount = 2;
	FIntentEffect Twilight = MakeEffect(
		WacomTags::Effect_ApplyStatus_Twilight,
		1,
		WacomTags::Target_Player);
	Twilight.HandAffliction.Selection = EHandAfflictionSelection::Default;
	Twilight.HandAffliction.TargetCardCount = 9;
	FIntentEffect UnknownTarget = MakeEffect(
		WacomTags::Effect_Damage,
		7,
		WacomTags::Target_SingleEnemyPart);
	SetIntentEffects(
		*Enemy,
		{ Damage, Shield, Slow, Twilight, UnknownTarget });

	UBattleSession* Session = CreateSession(Fixture, Enemy);
	const FIntentSnapshot Intent = GetIntent(*Session);
	if (!TestFalse(TEXT("Intent snapshot exists"), Intent.IntentId.IsNone()))
	{
		return false;
	}
	if (!TestEqual(TEXT("Every authored effect remains in execution order"),
		Intent.Effects.Num(), 5))
	{
		return false;
	}

	TestEqual(TEXT("Damage targets player"),
		Intent.Effects[0].TargetKind,
		EBattleIntentEffectTargetKind::Player);
	TestEqual(TEXT("Shield targets acting part"),
		Intent.Effects[1].TargetKind,
		EBattleIntentEffectTargetKind::SelfEnemyPart);
	TestEqual(TEXT("Default Slow canonicalizes to random hand cards"),
		Intent.Effects[2].TargetKind,
		EBattleIntentEffectTargetKind::RandomPlayerHandCards);
	TestEqual(TEXT("Random target count is preserved"),
		Intent.Effects[2].TargetCount, 2);
	TestEqual(TEXT("Default Twilight canonicalizes to all hand cards"),
		Intent.Effects[3].TargetKind,
		EBattleIntentEffectTargetKind::AllPlayerHandCards);
	TestEqual(TEXT("All-hand target has no count"),
		Intent.Effects[3].TargetCount, 0);
	TestEqual(TEXT("Unsupported target remains visible as unknown"),
		Intent.Effects[4].TargetKind,
		EBattleIntentEffectTargetKind::Unknown);
	TestEqual(TEXT("Magnitude is authoritative"),
		Intent.Effects[4].Magnitude, 7);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleIntentPublicPreviewExecutionParitySpec,
	"Wacom.Battle.IntentPublicPreview.ExecutionParity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleIntentPublicPreviewExecutionParitySpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleIntentPublicPreviewSpec;
	FWacomBattleFixture Fixture;
	UEnemyDefinition* Enemy =
		Fixture.MakeSinglePartEnemyWithIntentDamage(100, 3, 1);
	FIntentEffect Slow = MakeEffect(
		WacomTags::Effect_ApplyStatus_Slow,
		2,
		WacomTags::Target_Player);
	Slow.HandAffliction.Selection = EHandAfflictionSelection::Default;
	Slow.HandAffliction.TargetCardCount = 2;
	SetIntentEffects(
		*Enemy,
		{
			MakeEffect(
				WacomTags::Effect_Damage,
				3,
				WacomTags::Target_Player),
			MakeEffect(
				WacomTags::Status_Shield,
				4,
				WacomTags::Target_Self),
			Slow,
		});

	UBattleSession* Session = CreateSession(Fixture, Enemy);
	const FBattleSnapshot Before = Session->BuildSnapshot();
	const FEnemyPartSnapshot* BeforePart =
		FWacomBattleFixture::GetEnemyPartSnapshot(Before, 0);
	if (!TestNotNull(TEXT("Before part exists"), BeforePart)
		|| !TestEqual(TEXT("Three public effects"), BeforePart->CurrentIntent.Effects.Num(), 3))
	{
		return false;
	}
	TestEqual(TEXT("Peak damage derives from the same damage segment"),
		BeforePart->CurrentIntent.PeakAttackDamage,
		BeforePart->CurrentIntent.Effects[0].Magnitude);

	const FBattleResolution Resolution =
		Session->ResolveCommand(FBattleCommand::MakeEndTurn());
	TestTrue(TEXT("End turn resolves"), Resolution.IsOk());
	const FBattleSnapshot After = Session->BuildSnapshot();
	const FEnemyPartSnapshot* AfterPart =
		FWacomBattleFixture::GetEnemyPartSnapshot(After, 0);
	TestEqual(TEXT("Published damage matches applied player damage"),
		After.Player.CurrentHp,
		Before.Player.CurrentHp - BeforePart->CurrentIntent.Effects[0].Magnitude);
	TestEqual(TEXT("Published shield matches applied self shield"),
		AfterPart ? AfterPart->Shield : -1,
		BeforePart->CurrentIntent.Effects[1].Magnitude);
	int32 SlowedCards = 0;
	for (const FHandCardSnapshot& Card : After.Hand.Cards)
	{
		if (FWacomBattleFixture::GetStatusStacks(
			Card.StatusStacks,
			WacomTags::Status_Slow)
			== BeforePart->CurrentIntent.Effects[2].Magnitude)
		{
			++SlowedCards;
		}
	}
	TestEqual(TEXT("Published random hand count matches applied Slow"),
		SlowedCards,
		BeforePart->CurrentIntent.Effects[2].TargetCount);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleIntentPublicPreviewDestroyedPartSpec,
	"Wacom.Battle.IntentPublicPreview.DestroyedPartClearsEffects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleIntentPublicPreviewDestroyedPartSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleIntentPublicPreviewSpec;
	FWacomBattleFixture Fixture;
	UCardDefinition* LethalCard = Fixture.MakeSimpleDamageCard(0, 100);
	UEnemyDefinition* Enemy =
		Fixture.MakeSinglePartEnemyWithIntentDamage(10, 5, 3);
	UBattleSession* Session = CreateSession(Fixture, Enemy, LethalCard);

	const FBattleSnapshot Before = Session->BuildSnapshot();
	const FGuid CardId =
		FWacomBattleFixture::FindHandInstanceByCardId(Before, LethalCard->CardId);
	const FBattleResolution Resolution = Session->ResolveCommand(
		FWacomBattleFixture::MakePlayCardOnPart(Before, CardId, 0));
	TestTrue(TEXT("Lethal card resolves"), Resolution.IsOk());

	const FBattleSnapshot After = Session->BuildSnapshot();
	const FEnemyPartSnapshot* Part =
		FWacomBattleFixture::GetEnemyPartSnapshot(After, 0);
	if (!TestNotNull(TEXT("Destroyed part remains in snapshot"), Part))
	{
		return false;
	}
	TestTrue(TEXT("Part is destroyed"), Part->bDestroyed);
	TestTrue(TEXT("Destroyed part exposes no current intent"),
		Part->CurrentIntent.IntentId.IsNone());
	TestEqual(TEXT("Destroyed part exposes no effect facts"),
		Part->CurrentIntent.Effects.Num(), 0);
	return true;
}
