// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Characters/CharacterDefinition.h"
#include "Commands/BattleCommand.h"
#include "Enemies/EnemyBehaviorDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/IntentEffect.h"
#include "Resolution/BattleCardActionPreview.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomInteractionTargetTypes.h"

namespace
{
	UCardDefinition* MakeEnemyStatusCard(
		FWacomBattleFixture& Fixture,
		const FGameplayTag& EffectType,
		int32 Cost,
		int32 Stacks)
	{
		UCardDefinition* Card = Fixture.MakeNoopCard(Cost);
		Card->TargetMode = ECardTargetMode::SingleEnemyPart;
		FCardEffect Effect;
		Effect.EffectType = EffectType;
		Effect.Magnitude = Stacks;
		Effect.Target = WacomTags::Target_SingleEnemyPart;
		Card->Effects.Add(Effect);
		return Card;
	}

	FIntentEffect MakePlayerStatusIntentEffect(
		const FGameplayTag& EffectType,
		int32 Stacks,
		int32 TargetCardCount = 1)
	{
		FIntentEffect Effect;
		Effect.EffectType = EffectType;
		Effect.Magnitude = Stacks;
		Effect.Target = WacomTags::Target_Player;
		Effect.HandAffliction.TargetCardCount = TargetCardCount;
		return Effect;
	}

	FWacomEnemyBehaviorIntent MakeIntent(
		FName IntentId,
		int32 Initiative,
		TArray<FIntentEffect> Effects)
	{
		FWacomEnemyBehaviorIntent Entry;
		Entry.Intent.IntentId = IntentId;
		Entry.Intent.DisplayName = FText::FromName(IntentId);
		Entry.Intent.Initiative = Initiative;
		Entry.Intent.Effects = MoveTemp(Effects);
		return Entry;
	}

	void ReplaceSinglePartIntentSequence(
		UEnemyDefinition& Enemy,
		TArray<FWacomEnemyBehaviorIntent> Intents)
	{
		check(Enemy.DefaultBehavior);
		check(!Enemy.DefaultBehavior->Phases.IsEmpty());
		check(!Enemy.DefaultBehavior->Phases[0].IntentSets.IsEmpty());
		FWacomEnemyIntentSetDefinition& IntentSet =
			Enemy.DefaultBehavior->Phases[0].IntentSets[0];
		IntentSet.SelectorMode = EWacomEnemyIntentSelectorMode::Sequence;
		IntentSet.Intents = MoveTemp(Intents);
		IntentSet.SelectorRules.Reset();
		IntentSet.FallbackIntentId = NAME_None;
	}

	UCharacterDefinition* MakeFiveCardCharacter(
		FWacomBattleFixture& Fixture,
		const TArray<UCardDefinition*>& RequiredCards,
		int32 FillCost = 0)
	{
		TArray<UCardDefinition*> Deck = RequiredCards;
		while (Deck.Num() < 5)
		{
			Deck.Add(Fixture.MakeNoopCard(FillCost));
		}
		return Fixture.MakeCharacter(
			Fixture.MakeNoopCard(FillCost),
			Fixture.MakeNoopCard(FillCost),
			Deck);
	}

	int32 CountHandCardsWithStatus(
		const FBattleSnapshot& Snapshot,
		const FGameplayTag& Status,
		int32 ExpectedStacks = INDEX_NONE)
	{
		int32 Count = 0;
		for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
		{
			const int32* Stacks = Card.StatusStacks.Find(Status);
			if (Stacks && *Stacks > 0 && (ExpectedStacks == INDEX_NONE || *Stacks == ExpectedStacks))
			{
				++Count;
			}
		}
		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleStatusEnemySlowFreezeSpec,
	"Wacom.Battle.StatusSemantics.EnemySlowAndFreezeOwnInitiativeTiming",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleStatusEnemySlowFreezeSpec::RunTest(const FString&)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* Freeze = MakeEnemyStatusCard(
		Fixture, WacomTags::Effect_ApplyStatus_Freeze, 1, 1);
	UCardDefinition* Slow = MakeEnemyStatusCard(
		Fixture, WacomTags::Effect_ApplyStatus_Slow, 1, 3);
	UCardDefinition* Noop = Fixture.MakeNoopCard(2);
	UCharacterDefinition* Character = MakeFiveCardCharacter(Fixture, { Freeze, Slow, Noop });
	UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemy(50, 10, 0);
	UBattleSession* Session = Fixture.CreateSession(Character, Enemy, 101);

	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid PartId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);
	const FGuid FreezeId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, Freeze->CardId);
	const FGuid SlowId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, Slow->CardId);
	const FGuid NoopId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, Noop->CardId);

	TestTrue(TEXT("Apply Freeze"), Session->ResolveCommand(
		FWacomBattleFixture::MakePlayCardOnPartInstance(Snapshot, FreezeId, PartId)).IsOk());
	Snapshot = Session->BuildSnapshot();
	TestEqual(TEXT("Freeze does not block its own card"),
		FWacomBattleFixture::FindPartInitiative(Snapshot, 0), 9);
	TestEqual(TEXT("Freeze remains for next card"),
		FWacomBattleFixture::GetStatusStacks(
			FWacomBattleFixture::GetEnemyPartSnapshot(Snapshot, 0)->StatusStacks,
			WacomTags::Status_Freeze), 1);

	TestTrue(TEXT("Play Slow through Freeze"), Session->ResolveCommand(
		FWacomBattleFixture::MakePlayCardOnPartInstance(Snapshot, SlowId, PartId)).IsOk());
	Snapshot = Session->BuildSnapshot();
	TestEqual(TEXT("Slow delays current intent and Freeze blocks this push"),
		FWacomBattleFixture::FindPartInitiative(Snapshot, 0), 12);
	TestEqual(TEXT("Freeze consumed"),
		FWacomBattleFixture::GetStatusStacks(
			FWacomBattleFixture::GetEnemyPartSnapshot(Snapshot, 0)->StatusStacks,
			WacomTags::Status_Freeze), 0);
	TestEqual(TEXT("Enemy Slow is an immediate operation, not a lingering stack"),
		FWacomBattleFixture::GetStatusStacks(
			FWacomBattleFixture::GetEnemyPartSnapshot(Snapshot, 0)->StatusStacks,
			WacomTags::Status_Slow), 0);

	TestTrue(TEXT("Next card pushes normally"),
		Session->ResolveCommand(FBattleCommand::MakePlayCard(NoopId)).IsOk());
	Snapshot = Session->BuildSnapshot();
	TestEqual(TEXT("Normal push after Freeze"),
		FWacomBattleFixture::FindPartInitiative(Snapshot, 0), 10);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleStatusEnemyTwilightSpec,
	"Wacom.Battle.StatusSemantics.EnemyTwilightDelaysNextIntentAndHalves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleStatusEnemyTwilightSpec::RunTest(const FString&)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* Twilight = MakeEnemyStatusCard(
		Fixture, WacomTags::Effect_ApplyStatus_Twilight, 1, 3);
	UCharacterDefinition* Character = MakeFiveCardCharacter(Fixture, { Twilight });
	UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemy(50, 1, 0);
	UBattleSession* Session = Fixture.CreateSession(Character, Enemy, 102);

	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, Twilight->CardId);
	const FGuid PartId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);
	TestTrue(TEXT("Play Twilight"), Session->ResolveCommand(
		FWacomBattleFixture::MakePlayCardOnPartInstance(Snapshot, CardId, PartId)).IsOk());

	Snapshot = Session->BuildSnapshot();
	const FEnemyPartSnapshot* Part = FWacomBattleFixture::GetEnemyPartSnapshot(Snapshot, 0);
	TestEqual(TEXT("Next intent receives full Twilight delay"), Part->CurrentInitiative, 4);
	TestEqual(TEXT("Twilight halves after triggering"),
		FWacomBattleFixture::GetStatusStacks(Part->StatusStacks, WacomTags::Status_Twilight), 1);

	TestTrue(TEXT("End turn triggers following intent"),
		Session->ResolveCommand(FBattleCommand::MakeEndTurn()).IsOk());
	Snapshot = Session->BuildSnapshot();
	Part = FWacomBattleFixture::GetEnemyPartSnapshot(Snapshot, 0);
	TestEqual(TEXT("Remaining Twilight delays one more intent"), Part->CurrentInitiative, 2);
	TestEqual(TEXT("One Twilight stack decays to zero"),
		FWacomBattleFixture::GetStatusStacks(Part->StatusStacks, WacomTags::Status_Twilight), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleStatusPlayerSlowSpec,
	"Wacom.Battle.StatusSemantics.PlayerSlowSelectsCardsAndExpires",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleStatusPlayerSlowSpec::RunTest(const FString&)
{
	FWacomBattleFixture Fixture;
	UCharacterDefinition* Character = MakeFiveCardCharacter(Fixture, {}, 1);
	UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemy(50, 20, 0);
	ReplaceSinglePartIntentSequence(*Enemy, {
		MakeIntent(TEXT("Intent.Slow"), 20,
			{ MakePlayerStatusIntentEffect(WacomTags::Effect_ApplyStatus_Slow, 2, 2) }),
		MakeIntent(TEXT("Intent.Clear"), 20, {})
	});
	UBattleSession* Session = Fixture.CreateSession(Character, Enemy, 103);

	TestTrue(TEXT("End turn applies pending Slow"),
		Session->ResolveCommand(FBattleCommand::MakeEndTurn()).IsOk());
	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	TestEqual(TEXT("Exactly two cards selected"),
		CountHandCardsWithStatus(Snapshot, WacomTags::Status_Slow, 2), 2);
	for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
	{
		if (FWacomBattleFixture::GetStatusStacks(Card.StatusStacks, WacomTags::Status_Slow) == 2)
		{
			TestEqual(TEXT("Slow contributes to authoritative RuntimeCost"),
				Card.RuntimeCost,
				(Card.Definition ? Card.Definition->BaseCost : 0) + 2);
		}
	}

	TestTrue(TEXT("Second turn expires Slow"),
		Session->ResolveCommand(FBattleCommand::MakeEndTurn()).IsOk());
	Snapshot = Session->BuildSnapshot();
	TestEqual(TEXT("Turn-scoped Slow removed"),
		CountHandCardsWithStatus(Snapshot, WacomTags::Status_Slow), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleStatusPlayerTwilightSpec,
	"Wacom.Battle.StatusSemantics.PlayerTwilightPersistsAndHalvesOnPlay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleStatusPlayerTwilightSpec::RunTest(const FString&)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* Combo = Fixture.MakeNoopCard(0);
	Combo->Keywords.AddTag(WacomTags::Card_Keyword_Combo);
	UCharacterDefinition* Character = MakeFiveCardCharacter(Fixture, { Combo }, 0);
	UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemy(50, 20, 0);
	ReplaceSinglePartIntentSequence(*Enemy, {
		MakeIntent(TEXT("Intent.Twilight"), 20,
			{ MakePlayerStatusIntentEffect(WacomTags::Effect_ApplyStatus_Twilight, 2) }),
		MakeIntent(TEXT("Intent.Clear"), 20, {})
	});
	UBattleSession* Session = Fixture.CreateSession(Character, Enemy, 104);

	TestTrue(TEXT("End turn applies Twilight to the whole hand"),
		Session->ResolveCommand(FBattleCommand::MakeEndTurn()).IsOk());
	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	TestEqual(TEXT("Every current hand card receives Twilight"),
		CountHandCardsWithStatus(Snapshot, WacomTags::Status_Twilight, 2),
		Snapshot.Hand.Cards.Num());

	const FGuid ComboId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, Combo->CardId);
	const FBattleCardActionPreview TwilightPreview = Session->BuildCardActionPreview(
		ComboId,
		FWacomInteractionTargetHandle());
	TestTrue(TEXT("Twilight play produces projected hand facts"),
		TwilightPreview.bHasProjectedHand);
	const FHandCardSnapshot* ProjectedCombo = TwilightPreview.ProjectedHand.Cards.FindByPredicate(
		[ComboId](const FHandCardSnapshot& Card)
		{
			return Card.InstanceId == ComboId;
		});
	TestNotNull(TEXT("Projected Combo exists"), ProjectedCombo);
	if (ProjectedCombo)
	{
		TestEqual(TEXT("Action Preview halves Twilight with the formal transaction"),
			FWacomBattleFixture::GetStatusStacks(
				ProjectedCombo->StatusStacks,
				WacomTags::Status_Twilight), 1);
	}
	const FHandCardSnapshot* LiveComboBeforeCommit =
		FWacomBattleFixture::FindHandCardByInstanceId(Snapshot, ComboId);
	TestEqual(TEXT("Action Preview does not mutate live Twilight"),
		LiveComboBeforeCommit
			? FWacomBattleFixture::GetStatusStacks(
				LiveComboBeforeCommit->StatusStacks,
				WacomTags::Status_Twilight)
			: -1,
		2);
	TestTrue(TEXT("Play Twilight Combo"),
		Session->ResolveCommand(FBattleCommand::MakePlayCard(ComboId)).IsOk());
	Snapshot = Session->BuildSnapshot();
	const FHandCardSnapshot* ComboAfter =
		FWacomBattleFixture::FindHandCardByInstanceId(Snapshot, ComboId);
	TestNotNull(TEXT("Combo returned to hand"), ComboAfter);
	if (ComboAfter)
	{
		TestEqual(TEXT("Played card Twilight halves"),
			FWacomBattleFixture::GetStatusStacks(
				ComboAfter->StatusStacks,
				WacomTags::Status_Twilight), 1);
		TestEqual(TEXT("Cost reflects remaining Twilight"), ComboAfter->RuntimeCost, 1);
	}

	TestTrue(TEXT("End turn does not clear Twilight"),
		Session->ResolveCommand(FBattleCommand::MakeEndTurn()).IsOk());
	Snapshot = Session->BuildSnapshot();
	const FHandCardSnapshot* ComboRedrawn =
		FWacomBattleFixture::FindHandCardByCardId(Snapshot, Combo->CardId);
	TestNotNull(TEXT("Combo drawn again"), ComboRedrawn);
	if (ComboRedrawn)
	{
		TestEqual(TEXT("Twilight persists across discard/draw"),
			FWacomBattleFixture::GetStatusStacks(
				ComboRedrawn->StatusStacks,
				WacomTags::Status_Twilight), 1);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleStatusPlayerFreezeSpec,
	"Wacom.Battle.StatusSemantics.PlayerFreezeRejectsPlayAndAdjacentCardThaws",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleStatusPlayerFreezeSpec::RunTest(const FString&)
{
	FWacomBattleFixture Fixture;
	UCharacterDefinition* Character = MakeFiveCardCharacter(Fixture, {}, 0);
	UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemy(50, 20, 0);
	ReplaceSinglePartIntentSequence(*Enemy, {
		MakeIntent(TEXT("Intent.Freeze"), 20,
			{ MakePlayerStatusIntentEffect(WacomTags::Effect_ApplyStatus_Freeze, 1, 1) }),
		MakeIntent(TEXT("Intent.Clear"), 20, {})
	});
	UBattleSession* Session = Fixture.CreateSession(Character, Enemy, 105);

	TestTrue(TEXT("End turn materializes one frozen card"),
		Session->ResolveCommand(FBattleCommand::MakeEndTurn()).IsOk());
	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	int32 FrozenIndex = INDEX_NONE;
	for (int32 Index = 0; Index < Snapshot.Hand.Cards.Num(); ++Index)
	{
		if (Snapshot.Hand.Cards[Index].bIsFrozen)
		{
			FrozenIndex = Index;
			break;
		}
	}
	TestTrue(TEXT("Frozen card exists"), FrozenIndex != INDEX_NONE);
	if (FrozenIndex == INDEX_NONE)
	{
		return false;
	}

	const FGuid FrozenId = Snapshot.Hand.Cards[FrozenIndex].InstanceId;
	const FBattleCardActionPreview FrozenPreview = Session->BuildCardActionPreview(
		FrozenId,
		FWacomInteractionTargetHandle());
	TestFalse(TEXT("Frozen card has no action preview"), FrozenPreview.bHasPreview);
	TestEqual(TEXT("Action preview exposes frozen source reject"),
		FrozenPreview.TargetPreview.Validation.RejectReason,
		EWacomBattleTargetRejectReason::SourceCardFrozen);

	const FBattleResolution FrozenCommit = Session->ResolveCommand(FBattleCommand::MakePlayCard(FrozenId));
	TestEqual(TEXT("Frozen commit uses CardForbidden"), FrozenCommit.Status.Code, EWacomError::CardForbidden);
	TestEqual(TEXT("Frozen commit detail"), FrozenCommit.Status.Detail, FName(TEXT("CardFrozen")));

	const int32 NeighborIndex = FrozenIndex > 0 ? FrozenIndex - 1 : FrozenIndex + 1;
	TestTrue(TEXT("Frozen card has a neighbor"), Snapshot.Hand.Cards.IsValidIndex(NeighborIndex));
	if (!Snapshot.Hand.Cards.IsValidIndex(NeighborIndex))
	{
		return false;
	}
	const FGuid NeighborId = Snapshot.Hand.Cards[NeighborIndex].InstanceId;
	TestTrue(TEXT("Playing an adjacent card succeeds"),
		Session->ResolveCommand(FBattleCommand::MakePlayCard(NeighborId)).IsOk());
	Snapshot = Session->BuildSnapshot();
	const FHandCardSnapshot* Thawed =
		FWacomBattleFixture::FindHandCardByInstanceId(Snapshot, FrozenId);
	TestNotNull(TEXT("Frozen card remains in hand"), Thawed);
	if (Thawed)
	{
		TestFalse(TEXT("Adjacent play removes Freeze"), Thawed->bIsFrozen);
	}
	return true;
}
