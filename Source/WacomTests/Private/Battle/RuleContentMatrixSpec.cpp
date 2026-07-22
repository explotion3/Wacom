// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Cards/CardPassive.h"
#include "Cards/CardZoneHook.h"
#include "Cards/EffectCondition.h"
#include "Characters/CharacterDefinition.h"
#include "Commands/BattleCommand.h"
#include "Enemies/EnemyBehaviorDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Enemies/IntentDefinition.h"
#include "Enemies/IntentEffect.h"
#include "Events/BattleEvent.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomEnums.h"
#include "UObject/StrongObjectPtr.h"
#include "Validation/CardDefinitionValidation.h"
#include "Validation/EnemyBehaviorDefinitionValidation.h"

namespace
{
	template <typename T>
	T* NewMatrixObject(UObject* Outer)
	{
		return NewObject<T>(Outer ? Outer : GetTransientPackage(), NAME_None, RF_Transient);
	}

	FCardEffect MakeEffect(
		const FGameplayTag& EffectType,
		int32 Magnitude,
		const FGameplayTag& Target,
		const FGameplayTag& TargetZone = FGameplayTag())
	{
		FCardEffect Effect;
		Effect.EffectType = EffectType;
		Effect.Magnitude = Magnitude;
		Effect.Target = Target;
		Effect.TargetZone = TargetZone;
		return Effect;
	}

	UCardDefinition* MakeMatrixCard(
		UObject* Outer,
		FName CardId,
		int32 Cost,
		ECardTargetMode TargetMode,
		const TArray<FCardEffect>& Effects)
	{
		UCardDefinition* Card = NewMatrixObject<UCardDefinition>(Outer);
		Card->CardId = CardId;
		Card->DisplayName = FText::FromName(CardId);
		Card->BaseCost = Cost;
		Card->Rarity = WacomTags::Card_Rarity_White;
		Card->TargetMode = TargetMode;
		Card->Effects = Effects;
		return Card;
	}

	UCardDefinition* MakeNoopMatrixCard(UObject* Outer, FName CardId, int32 Cost = 0)
	{
		return MakeMatrixCard(Outer, CardId, Cost, ECardTargetMode::None, {});
	}

	UCardDefinition* MakeEnemyPartCard(
		UObject* Outer,
		FName CardId,
		const TArray<FCardEffect>& Effects,
		int32 Cost = 0)
	{
		return MakeMatrixCard(Outer, CardId, Cost, ECardTargetMode::SingleEnemyPart, Effects);
	}

	UCardDefinition* MakeHandTargetCard(
		UObject* Outer,
		FName CardId,
		const FCardEffect& Effect)
	{
		UCardDefinition* Card = MakeMatrixCard(Outer, CardId, 0, ECardTargetMode::HandCard, { Effect });
		Card->HandCardTargetFilter.bUseExplicitHandCardTargetFilter = true;
		Card->HandCardTargetFilter.bAllowNormalHandCards = true;
		Card->HandCardTargetFilter.bAllowHandAnchors = false;
		return Card;
	}

	bool ValidateCardForMatrix(UCardDefinition* Card, FAutomationTestBase& Test)
	{
		TArray<FText> Errors;
		const bool bValid = FWacomCardDefinitionValidation::Validate(Card, Errors);
		if (!bValid)
		{
			for (const FText& Error : Errors)
			{
				Test.AddError(FString::Printf(TEXT("%s failed validation: %s"),
					*Card->CardId.ToString(),
					*Error.ToString()));
			}
		}
		Test.TestTrue(FString::Printf(TEXT("%s passes card validation"), *Card->CardId.ToString()), bValid);
		return bValid;
	}

	bool ValidateEnemyBehaviorForMatrix(
		UEnemyBehaviorDefinition* Behavior,
		UEnemyDefinition* Enemy,
		FAutomationTestBase& Test)
	{
		TArray<FText> Errors;
		const bool bValid = FWacomEnemyBehaviorDefinitionValidation::Validate(Behavior, Errors, Enemy);
		if (!bValid)
		{
			for (const FText& Error : Errors)
			{
				Test.AddError(FString::Printf(TEXT("%s failed validation: %s"),
					*GetNameSafe(Behavior),
					*Error.ToString()));
			}
		}
		Test.TestTrue(FString::Printf(TEXT("%s passes enemy behavior validation"), *GetNameSafe(Behavior)), bValid);
		return bValid;
	}

	bool ValidateCardRejected(UCardDefinition* Card)
	{
		TArray<FText> Errors;
		return !FWacomCardDefinitionValidation::Validate(Card, Errors) && Errors.Num() > 0;
	}

	bool ValidateEnemyBehaviorRejected(UEnemyBehaviorDefinition* Behavior, UEnemyDefinition* Enemy)
	{
		TArray<FText> Errors;
		return !FWacomEnemyBehaviorDefinitionValidation::Validate(Behavior, Errors, Enemy) && Errors.Num() > 0;
	}

	UBattleSession* CreateMatrixSession(
		FWacomBattleFixture& Fixture,
		UCardDefinition* LeftHand,
		UCardDefinition* RightHand,
		const TArray<UCardDefinition*>& Deck,
		UEnemyDefinition* Enemy,
		int32 Seed = 1)
	{
		UCharacterDefinition* Character = Fixture.MakeCharacter(LeftHand, RightHand, Deck);
		return Fixture.CreateSession(Character, Enemy, Seed);
	}

	UBattleSession* CreateSessionWithRequiredCards(
		FWacomBattleFixture& Fixture,
		UObject* Outer,
		const TArray<UCardDefinition*>& RequiredCards,
		UEnemyDefinition* Enemy,
		int32 Seed = 1)
	{
		TArray<UCardDefinition*> Deck = RequiredCards;
		for (int32 Index = Deck.Num(); Index < 5; ++Index)
		{
			Deck.Add(MakeNoopMatrixCard(Outer, FName(*FString::Printf(TEXT("Matrix.Filler.%d.%s"), Index, *FGuid::NewGuid().ToString(EGuidFormats::Short)))));
		}

		return CreateMatrixSession(
			Fixture,
			MakeNoopMatrixCard(Outer, TEXT("Matrix.Left")),
			MakeNoopMatrixCard(Outer, TEXT("Matrix.Right")),
			Deck,
			Enemy,
			Seed);
	}

	UEnemyDefinition* MakeEnemyWithIntent(
		UObject* Outer,
		const TArray<FIntentEffect>& Effects,
		int32 Hp = 100,
		int32 Initiative = 5)
	{
		UEnemyPartDefinition* Part = NewMatrixObject<UEnemyPartDefinition>(Outer);
		Part->PartId = FName(*FString::Printf(TEXT("Matrix.Part.%s"), *FGuid::NewGuid().ToString(EGuidFormats::Short)));
		Part->DisplayName = FText::FromName(Part->PartId);
		Part->MaxHp = Hp;
		Part->ExperienceReward = 1;

		UEnemyBehaviorDefinition* Behavior = NewMatrixObject<UEnemyBehaviorDefinition>(Outer);
		Behavior->BehaviorId = FName(*FString::Printf(TEXT("Matrix.Behavior.%s"), *FGuid::NewGuid().ToString(EGuidFormats::Short)));
		Behavior->InitialPhaseId = TEXT("Default");

		FWacomEnemyBehaviorIntent IntentEntry;
		IntentEntry.Intent.IntentId = FName(*FString::Printf(TEXT("Matrix.Intent.%s"), *FGuid::NewGuid().ToString(EGuidFormats::Short)));
		IntentEntry.Intent.DisplayName = FText::FromName(IntentEntry.Intent.IntentId);
		IntentEntry.Intent.Initiative = Initiative;
		IntentEntry.Intent.Effects = Effects;

		FWacomEnemyIntentSetDefinition IntentSet;
		IntentSet.IntentSetId = TEXT("Matrix.IntentSet.Core");
		IntentSet.AppliesToPartSlotId = Part->PartId;
		IntentSet.SelectorMode = EWacomEnemyIntentSelectorMode::Sequence;
		IntentSet.Intents = { IntentEntry };

		FWacomEnemyPhaseDefinition Phase;
		Phase.PhaseId = TEXT("Default");
		Phase.IntentSets = { IntentSet };
		Behavior->Phases = { Phase };

		UEnemyDefinition* Enemy = NewMatrixObject<UEnemyDefinition>(Outer);
		Enemy->EnemyId = FName(*FString::Printf(TEXT("Matrix.Enemy.%s"), *FGuid::NewGuid().ToString(EGuidFormats::Short)));
		Enemy->DisplayName = FText::FromName(Enemy->EnemyId);
		Enemy->DefaultBehavior = Behavior;
		Enemy->DefaultPhaseId = TEXT("Default");
		FEnemyPartSlot Slot;
		Slot.PartSlotId = Part->PartId;
		Slot.PartDef = Part;
		Slot.InitialIntentSetId = TEXT("Matrix.IntentSet.Core");
		Enemy->Parts.Add(Slot);
		return Enemy;
	}

	FIntentEffect MakeIntentEffect(
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

	FBattleResolution PlayCardByDefinition(
		UBattleSession* Session,
		const FBattleSnapshot& Snapshot,
		UCardDefinition* Card,
		const FGuid& TargetPartId,
		FAutomationTestBase& Test)
	{
		const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, Card->CardId);
		Test.TestTrue(FString::Printf(TEXT("%s is in hand"), *Card->CardId.ToString()), CardId.IsValid());
		if (!CardId.IsValid())
		{
			FBattleResolution MissingCard;
			MissingCard.Status = FWacomStatus::Fail(EWacomError::NotFound, TEXT("MatrixCardNotInHand"));
			return MissingCard;
		}
		FBattleResolution Resolution = Session->ResolveCommand(
			FWacomBattleFixture::MakePlayCardOnPartInstance(Snapshot, CardId, TargetPartId));
		Test.TestTrue(
			FString::Printf(TEXT("%s play command succeeds"), *Card->CardId.ToString()),
			Resolution.IsOk());
		return Resolution;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleRuleContentMatrixAllowedCardEffectsSpec,
	"Wacom.Battle.RuleContentMatrix.ValidatorAllowedCardEffectsExecuteInBattleSession",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleRuleContentMatrixAllowedCardEffectsSpec::RunTest(const FString& /*Parameters*/)
{
	UObject* Outer = GetTransientPackage();

	{
		FWacomBattleFixture Fixture;
		UCardDefinition* Card = MakeEnemyPartCard(Outer, TEXT("Matrix.Damage"), {
			MakeEffect(WacomTags::Effect_Damage, 4, WacomTags::Target_SingleEnemyPart)
		});
		ValidateCardForMatrix(Card, *this);
		UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/50, /*Initiative*/50, /*Damage*/0);
		UBattleSession* Session = CreateSessionWithRequiredCards(Fixture, Outer, { Card }, Enemy);
		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const FGuid PartId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);
		PlayCardByDefinition(Session, Snapshot, Card, PartId, *this);
		Snapshot = Session->BuildSnapshot();
		TestEqual(TEXT("Damage effect lowers enemy part HP"), FWacomBattleFixture::FindPartHp(Snapshot, 0), 46);
	}

	{
		FWacomBattleFixture Fixture;
		UCardDefinition* Card = MakeMatrixCard(Outer, TEXT("Matrix.PlayerShield"), 0, ECardTargetMode::None, {
			MakeEffect(WacomTags::Status_Shield, 5, WacomTags::Target_Player)
		});
		ValidateCardForMatrix(Card, *this);
		UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/50, /*Initiative*/50, /*Damage*/0);
		UBattleSession* Session = CreateSessionWithRequiredCards(Fixture, Outer, { Card }, Enemy);
		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		PlayCardByDefinition(Session, Snapshot, Card, FGuid(), *this);
		Snapshot = Session->BuildSnapshot();
		TestEqual(TEXT("Shield effect increases player shield"), Snapshot.Player.Shield, 5);
	}

	{
		FWacomBattleFixture Fixture;
		UCardDefinition* Card = MakeEnemyPartCard(Outer, TEXT("Matrix.Statuses"), {
			MakeEffect(WacomTags::Effect_ApplyStatus_Poison, 2, WacomTags::Target_SingleEnemyPart),
			MakeEffect(WacomTags::Effect_ApplyStatus_Slow, 3, WacomTags::Target_SingleEnemyPart),
			MakeEffect(WacomTags::Effect_ApplyStatus_Freeze, 1, WacomTags::Target_SingleEnemyPart),
			MakeEffect(WacomTags::Effect_ApplyStatus_Twilight, 4, WacomTags::Target_SingleEnemyPart)
		});
		ValidateCardForMatrix(Card, *this);
		UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/80, /*Initiative*/50, /*Damage*/0);
		UBattleSession* Session = CreateSessionWithRequiredCards(Fixture, Outer, { Card }, Enemy);
		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const FGuid PartId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);
		PlayCardByDefinition(Session, Snapshot, Card, PartId, *this);
		Snapshot = Session->BuildSnapshot();
		const FEnemyPartSnapshot& Part = *FWacomBattleFixture::GetEnemyPartSnapshot(Snapshot, 0);
		TestEqual(TEXT("Poison stacks applied"), FWacomBattleFixture::GetStatusStacks(Part.StatusStacks, WacomTags::Status_Poison), 2);
		TestEqual(TEXT("Slow is not retained as an enemy stack"), FWacomBattleFixture::GetStatusStacks(Part.StatusStacks, WacomTags::Status_Slow), 0);
		TestEqual(TEXT("Slow immediately delays the current intent"), Part.CurrentInitiative, 53);
		// Freeze skips the immediate initiative-zero action and consumes one stack during that skipped action.
		TestEqual(TEXT("Freeze stacks applied"), FWacomBattleFixture::GetStatusStacks(Part.StatusStacks, WacomTags::Status_Freeze), 1);
		TestEqual(TEXT("Twilight stacks applied"), FWacomBattleFixture::GetStatusStacks(Part.StatusStacks, WacomTags::Status_Twilight), 4);
	}

	{
		FWacomBattleFixture Fixture;
		UCardDefinition* DrawCard = MakeMatrixCard(Outer, TEXT("Matrix.Draw"), 0, ECardTargetMode::None, {
			MakeEffect(WacomTags::Effect_Draw, 1, WacomTags::Target_Player, WacomTags::CardLocation_Draw)
		});
		UCardDefinition* Filler = MakeNoopMatrixCard(Outer, TEXT("Matrix.Draw.Filler"));
		ValidateCardForMatrix(DrawCard, *this);
		UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/50, /*Initiative*/50, /*Damage*/0);
		UBattleSession* Session = CreateSessionWithRequiredCards(Fixture, Outer, { DrawCard, Filler, Filler, Filler, Filler, Filler }, Enemy);
		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const int32 DrawBefore = Snapshot.PileCounts.DrawCount;
		const FBattleResolution Resolution =
			PlayCardByDefinition(Session, Snapshot, DrawCard, FGuid(), *this);
		const TArray<FBattleEvent>& Events = Resolution.Events;
		Snapshot = Session->BuildSnapshot();
		TestTrue(TEXT("Draw effect emits CardsDrawn"), FWacomBattleFixture::HasEvent(Events, EBattleEventType::CardsDrawn));
		TestTrue(TEXT("Draw effect emits CardsDrawn ids matching count"),
			Events.ContainsByPredicate([](const FBattleEvent& Event)
			{
				return Event.Type == EBattleEventType::CardsDrawn
					&& Event.Count == 1
					&& Event.CardInstanceIds.Num() == Event.Count;
			}));
		TestEqual(TEXT("Draw pile consumed by one"), Snapshot.PileCounts.DrawCount, FMath::Max(0, DrawBefore - 1));
	}

	{
		FWacomBattleFixture Fixture;
		UCardDefinition* DiscardCard = MakeMatrixCard(Outer, TEXT("Matrix.Discard"), 0, ECardTargetMode::None, {
			MakeEffect(WacomTags::Effect_Discard, 1, WacomTags::Target_Player)
		});
		ValidateCardForMatrix(DiscardCard, *this);
		UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/50, /*Initiative*/50, /*Damage*/0);
		UBattleSession* Session = CreateSessionWithRequiredCards(Fixture, Outer, { DiscardCard }, Enemy);
		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const FBattleResolution Resolution =
			PlayCardByDefinition(Session, Snapshot, DiscardCard, FGuid(), *this);
		const TArray<FBattleEvent>& Events = Resolution.Events;
		TestTrue(TEXT("Discard effect emits CardDiscarded"), FWacomBattleFixture::HasEvent(Events, EBattleEventType::CardDiscarded, WacomTags::Effect_Discard));
	}

	{
		FWacomBattleFixture Fixture;
		UCardDefinition* ExhaustCard = MakeMatrixCard(Outer, TEXT("Matrix.ExhaustSelf"), 0, ECardTargetMode::None, {
			MakeEffect(WacomTags::Effect_ExhaustSelf, 0, WacomTags::Target_Self)
		});
		ValidateCardForMatrix(ExhaustCard, *this);
		UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/50, /*Initiative*/50, /*Damage*/0);
		UBattleSession* Session = CreateSessionWithRequiredCards(Fixture, Outer, { ExhaustCard }, Enemy);
		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const int32 ExhaustBefore = Snapshot.PileCounts.ExhaustCount;
		PlayCardByDefinition(Session, Snapshot, ExhaustCard, FGuid(), *this);
		Snapshot = Session->BuildSnapshot();
		TestEqual(TEXT("ExhaustSelf sends played card to exhaust"), Snapshot.PileCounts.ExhaustCount, ExhaustBefore + 1);
	}

	{
		FWacomBattleFixture Fixture;
		UCardDefinition* HealCard = MakeMatrixCard(Outer, TEXT("Matrix.Heal"), 0, ECardTargetMode::None, {
			MakeEffect(WacomTags::Effect_Heal, 6, WacomTags::Target_Player)
		});
		ValidateCardForMatrix(HealCard, *this);
		FIntentEffect DamageIntent = MakeIntentEffect(WacomTags::Effect_Damage, 10, WacomTags::Target_Player);
		UEnemyDefinition* Enemy = MakeEnemyWithIntent(Outer, { DamageIntent }, /*Hp*/50, /*Initiative*/1);
		UBattleSession* Session = CreateSessionWithRequiredCards(Fixture, Outer, { HealCard }, Enemy);
		TestTrue(TEXT("End turn damages player before heal"), Session->ResolveCommand(FBattleCommand::MakeEndTurn()).IsOk());
		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		TestEqual(TEXT("Player took enemy damage"), Snapshot.Player.CurrentHp, Snapshot.Player.MaxHp - 10);
		const FGuid HealId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, HealCard->CardId);
		TestTrue(TEXT("Heal card remains/draws into hand"), HealId.IsValid());
		if (HealId.IsValid())
		{
			TestTrue(TEXT("Play heal"), Session->ResolveCommand(FBattleCommand::MakePlayCard(HealId)).IsOk());
			Snapshot = Session->BuildSnapshot();
			TestEqual(TEXT("Heal effect restores player HP"), Snapshot.Player.CurrentHp, Snapshot.Player.MaxHp - 4);
		}
	}

	{
		FWacomBattleFixture Fixture;
		UCardDefinition* ModifyCard = MakeEnemyPartCard(Outer, TEXT("Matrix.ModifyInitiative"), {
			MakeEffect(WacomTags::Effect_ModifyInitiative, -4, WacomTags::Target_SingleEnemyPart)
		});
		ValidateCardForMatrix(ModifyCard, *this);
		UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/50, /*Initiative*/50, /*Damage*/0);
		UBattleSession* Session = CreateSessionWithRequiredCards(Fixture, Outer, { ModifyCard }, Enemy);
		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const FGuid PartId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);
		PlayCardByDefinition(Session, Snapshot, ModifyCard, PartId, *this);
		Snapshot = Session->BuildSnapshot();
		TestEqual(TEXT("ModifyInitiative applies and normal push also resolves"), FWacomBattleFixture::FindPartInitiative(Snapshot, 0), 46);
	}

	{
		FWacomBattleFixture Fixture;
		UCardDefinition* ApplyRemoveCard = MakeEnemyPartCard(Outer, TEXT("Matrix.RemoveStatus"), {
			MakeEffect(WacomTags::Effect_ApplyStatus_Twilight, 3, WacomTags::Target_SingleEnemyPart),
			MakeEffect(WacomTags::Effect_RemoveStatus, 2, WacomTags::Target_SingleEnemyPart, WacomTags::Status_Twilight)
		});
		ValidateCardForMatrix(ApplyRemoveCard, *this);
		UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/50, /*Initiative*/50, /*Damage*/0);
		UBattleSession* Session = CreateSessionWithRequiredCards(Fixture, Outer, { ApplyRemoveCard }, Enemy);
		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const FGuid PartId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);
		PlayCardByDefinition(Session, Snapshot, ApplyRemoveCard, PartId, *this);
		Snapshot = Session->BuildSnapshot();
		const FEnemyPartSnapshot* Part = FWacomBattleFixture::GetEnemyPartSnapshot(Snapshot, 0);
		TestNotNull(TEXT("Primary part exists"), Part);
		TestEqual(TEXT("RemoveStatus removes requested stacks"), FWacomBattleFixture::GetStatusStacks(Part ? Part->StatusStacks : TMap<FGameplayTag, int32>(), WacomTags::Status_Twilight), 1);
	}

	{
		FWacomBattleFixture Fixture;
		UCardDefinition* TargetCard = MakeNoopMatrixCard(Outer, TEXT("Matrix.TargetKeyword"), 3);
		UCardDefinition* GainKeywordCard = MakeHandTargetCard(Outer, TEXT("Matrix.GainKeyword"),
			MakeEffect(WacomTags::Effect_GainKeyword, 0, WacomTags::Target_SelectedHandCard, WacomTags::Card_Keyword_Companion));
		ValidateCardForMatrix(GainKeywordCard, *this);
		UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/50, /*Initiative*/50, /*Damage*/0);
		UBattleSession* Session = CreateSessionWithRequiredCards(Fixture, Outer, { GainKeywordCard, TargetCard }, Enemy);
		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, GainKeywordCard->CardId);
		const FGuid TargetId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetCard->CardId);
		TestTrue(TEXT("Play GainKeyword on selected hand card"),
			Session->ResolveCommand(FBattleCommand::MakePlayCardOnHandCard(SourceId, TargetId)).IsOk());

		UCardDefinition* FilterCard = MakeHandTargetCard(Outer, TEXT("Matrix.KeywordFilter"),
			MakeEffect(WacomTags::Effect_Card_AddCost, 1, WacomTags::Target_SelectedHandCard));
		FilterCard->HandCardTargetFilter.RequiredTargetKeywords.AddTag(WacomTags::Card_Keyword_Companion);
		ValidateCardForMatrix(FilterCard, *this);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleRuleContentMatrixMagnitudeConditionSpec,
	"Wacom.Battle.RuleContentMatrix.MagnitudeSourcesAndConditionsMatchRuntimeResolver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleRuleContentMatrixMagnitudeConditionSpec::RunTest(const FString& /*Parameters*/)
{
	UObject* Outer = GetTransientPackage();

	{
		FWacomBattleFixture Fixture;
		FCardEffect Effect = MakeEffect(WacomTags::Effect_ApplyStatus_Poison, 0, WacomTags::Target_SingleEnemyPart);
		Effect.MagnitudeSource = WacomTags::Magnitude_Source_RuntimeCost;
		UCardDefinition* Card = MakeEnemyPartCard(Outer, TEXT("Matrix.RuntimeCostPoison"), { Effect }, /*Cost*/3);
		ValidateCardForMatrix(Card, *this);
		UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/80, /*Initiative*/50, /*Damage*/0);
		UBattleSession* Session = CreateSessionWithRequiredCards(Fixture, Outer, { Card }, Enemy);
		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const FGuid PartId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);
		PlayCardByDefinition(Session, Snapshot, Card, PartId, *this);
		Snapshot = Session->BuildSnapshot();
		const FEnemyPartSnapshot* Part = FWacomBattleFixture::GetEnemyPartSnapshot(Snapshot, 0);
		TestNotNull(TEXT("Primary part exists"), Part);
		TestEqual(TEXT("RuntimeCost magnitude applies poison equal to cost"), FWacomBattleFixture::GetStatusStacks(Part ? Part->StatusStacks : TMap<FGameplayTag, int32>(), WacomTags::Status_Poison), 3);
	}

	{
		FWacomBattleFixture Fixture;
		FCardEffect DrawEffect = MakeEffect(
			WacomTags::Effect_Draw,
			0,
			WacomTags::Target_Player,
			WacomTags::CardLocation_Draw);
		DrawEffect.MagnitudeSource = WacomTags::Magnitude_Source_RuntimeCost;
		UCardDefinition* DrawCard = MakeMatrixCard(
			Outer,
			TEXT("Matrix.RuntimeCostDraw"),
			/*Cost*/2,
			ECardTargetMode::None,
			{ DrawEffect });
		UCardDefinition* Filler = MakeNoopMatrixCard(Outer, TEXT("Matrix.RuntimeCostDraw.Filler"));
		ValidateCardForMatrix(DrawCard, *this);
		UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemyWithIntentDamage(
			/*Hp*/80,
			/*Initiative*/50,
			/*Damage*/0);
		UBattleSession* Session = CreateSessionWithRequiredCards(
			Fixture,
			Outer,
			{ DrawCard, Filler, Filler, Filler, Filler, Filler, Filler, Filler, Filler },
			Enemy);
		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const int32 DrawBefore = Snapshot.PileCounts.DrawCount;
		const FBattleResolution Resolution =
			PlayCardByDefinition(Session, Snapshot, DrawCard, FGuid(), *this);
		const TArray<FBattleEvent>& Events = Resolution.Events;
		Snapshot = Session->BuildSnapshot();
		TestEqual(TEXT("RuntimeCost draw event count"), Events.ContainsByPredicate([](const FBattleEvent& Event)
		{
			return Event.Type == EBattleEventType::CardsDrawn
				&& Event.Count == 2
				&& Event.CardInstanceIds.Num() == Event.Count;
		}), true);
		TestEqual(TEXT("RuntimeCost draw consumes cards equal to cost"),
			DrawBefore - Snapshot.PileCounts.DrawCount,
			2);
	}

	{
		FWacomBattleFixture Fixture;
		UCardDefinition* PoisonCard = MakeEnemyPartCard(Outer, TEXT("Matrix.StackSeed"), {
			MakeEffect(WacomTags::Effect_ApplyStatus_Poison, 4, WacomTags::Target_SingleEnemyPart)
		});
		FCardEffect DamageEffect = MakeEffect(WacomTags::Effect_Damage, 0, WacomTags::Target_SingleEnemyPart, WacomTags::Status_Poison);
		DamageEffect.MagnitudeSource = WacomTags::Magnitude_Source_TargetStatusStacks;
		UCardDefinition* StackDamage = MakeEnemyPartCard(Outer, TEXT("Matrix.StackDamage"), { DamageEffect });
		ValidateCardForMatrix(PoisonCard, *this);
		ValidateCardForMatrix(StackDamage, *this);
		UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/100, /*Initiative*/50, /*Damage*/0);
		UBattleSession* Session = CreateSessionWithRequiredCards(Fixture, Outer, { PoisonCard, StackDamage }, Enemy);
		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const FGuid PartId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);
		PlayCardByDefinition(Session, Snapshot, PoisonCard, PartId, *this);
		Snapshot = Session->BuildSnapshot();
		const int32 HpAfterPoison = FWacomBattleFixture::FindPartHp(Snapshot, 0);
		PlayCardByDefinition(Session, Snapshot, StackDamage, PartId, *this);
		Snapshot = Session->BuildSnapshot();
		TestEqual(TEXT("TargetStatusStacks damage uses current poison stacks plus poison tick"), FWacomBattleFixture::FindPartHp(Snapshot, 0), HpAfterPoison - 8);
	}

	{
		bool bCoveredLeftZone = false;
		for (int32 Seed = 1; Seed <= 80 && !bCoveredLeftZone; ++Seed)
		{
			FWacomBattleFixture Fixture;
			FCardEffect DamageEffect = MakeEffect(WacomTags::Effect_Damage, 2, WacomTags::Target_SingleEnemyPart);
			FMagnitudeModifier AddMod;
			AddMod.Condition.ConditionType = WacomTags::Condition_Target_HasStatus;
			AddMod.Condition.ParamTag = WacomTags::Status_Poison;
			AddMod.Op = EMagnitudeModOp::Add;
			AddMod.Value = 3;
			FMagnitudeModifier MultiplyMod;
			MultiplyMod.Condition.ConditionType = WacomTags::Condition_Self_InZone;
			MultiplyMod.Condition.ParamTag = WacomTags::HandZone_Left;
			MultiplyMod.Op = EMagnitudeModOp::Multiply;
			MultiplyMod.Value = 2;
			DamageEffect.MagnitudeModifiers = { AddMod, MultiplyMod };

			UCardDefinition* LeftCard = MakeEnemyPartCard(Outer, FName(*FString::Printf(TEXT("Matrix.LeftConditionalDamage.%d"), Seed)), { DamageEffect });
			UCardDefinition* PoisonCard = MakeEnemyPartCard(Outer, FName(*FString::Printf(TEXT("Matrix.ConditionPoison.%d"), Seed)), {
				MakeEffect(WacomTags::Effect_ApplyStatus_Poison, 1, WacomTags::Target_SingleEnemyPart)
			});
			ValidateCardForMatrix(LeftCard, *this);
			ValidateCardForMatrix(PoisonCard, *this);
			UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/100, /*Initiative*/50, /*Damage*/0);
			UBattleSession* Session = CreateMatrixSession(
				Fixture,
				MakeNoopMatrixCard(Outer, FName(*FString::Printf(TEXT("Matrix.Left.Condition.%d"), Seed))),
				MakeNoopMatrixCard(Outer, FName(*FString::Printf(TEXT("Matrix.Right.Condition.%d"), Seed))),
				{ PoisonCard, LeftCard, MakeNoopMatrixCard(Outer, FName(*FString::Printf(TEXT("Matrix.Filler.Condition.A.%d"), Seed))), MakeNoopMatrixCard(Outer, FName(*FString::Printf(TEXT("Matrix.Filler.Condition.B.%d"), Seed))), MakeNoopMatrixCard(Outer, FName(*FString::Printf(TEXT("Matrix.Filler.Condition.C.%d"), Seed))) },
				Enemy,
				Seed);
			FBattleSnapshot Snapshot = Session->BuildSnapshot();
			const FGuid LeftId = FWacomBattleFixture::FindHandInstanceByCardIdInZone(Snapshot, LeftCard->CardId, EHandZone::Left);
			if (!LeftId.IsValid())
			{
				continue;
			}
			const FGuid PartId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);
			PlayCardByDefinition(Session, Snapshot, PoisonCard, PartId, *this);
			Snapshot = Session->BuildSnapshot();
			const int32 HpAfterPoison = FWacomBattleFixture::FindPartHp(Snapshot, 0);
			TestTrue(TEXT("Play left-zone modifier card"),
				Session->ResolveCommand(FWacomBattleFixture::MakePlayCardOnPartInstance(Snapshot, LeftId, PartId)).IsOk());
			Snapshot = Session->BuildSnapshot();
			TestEqual(TEXT("Condition modifiers run in order with Self.InZone and Target.HasStatus"), FWacomBattleFixture::FindPartHp(Snapshot, 0), HpAfterPoison - 11);
			bCoveredLeftZone = true;
		}
		TestTrue(TEXT("At least one seed placed modifier card in Left zone"), bCoveredLeftZone);
	}

	{
		UCardDefinition* Reserved = MakeEnemyPartCard(Outer, TEXT("Matrix.HandCountRejected"), {
			MakeEffect(WacomTags::Effect_Damage, 0, WacomTags::Target_SingleEnemyPart)
		});
		Reserved->Effects[0].MagnitudeSource = WacomTags::Magnitude_Source_HandCount;
		TestTrue(TEXT("HandCount magnitude remains reserved by validator"), ValidateCardRejected(Reserved));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleRuleContentMatrixPassiveSpec,
	"Wacom.Battle.RuleContentMatrix.CardPassiveTriggerMatrixMatchesRuntimeDispatcher",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleRuleContentMatrixPassiveSpec::RunTest(const FString& /*Parameters*/)
{
	UObject* Outer = GetTransientPackage();

	{
		FWacomBattleFixture Fixture;
		UCardDefinition* Card = MakeMatrixCard(Outer, TEXT("Matrix.AfterPlayed"), 0, ECardTargetMode::None, {});
		Card->Keywords.AddTag(WacomTags::Card_Keyword_Combo);
		FCardPassive Passive;
		Passive.Trigger = WacomTags::Passive_Trigger_AfterPlayed;
		Passive.Effects.Add(MakeEffect(WacomTags::Status_Shield, 4, WacomTags::Target_Player));
		Card->Passives.Add(Passive);
		ValidateCardForMatrix(Card, *this);
		UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/50, /*Initiative*/50, /*Damage*/0);
		UBattleSession* Session = CreateSessionWithRequiredCards(Fixture, Outer, { Card }, Enemy);
		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		PlayCardByDefinition(Session, Snapshot, Card, FGuid(), *this);
		Snapshot = Session->BuildSnapshot();
		TestEqual(TEXT("AfterPlayed passive executes effects"), Snapshot.Player.Shield, 4);
	}

	{
		FWacomBattleFixture Fixture;
		UCardDefinition* Source = MakeHandTargetCard(Outer, TEXT("Matrix.DiscardSource"),
			MakeEffect(WacomTags::Effect_Card_DiscardSelected, 1, WacomTags::Target_SelectedHandCard));
		UCardDefinition* Target = MakeNoopMatrixCard(Outer, TEXT("Matrix.OnDiscardTarget"));
		FCardPassive Passive;
		Passive.Trigger = WacomTags::Passive_Trigger_OnDiscard;
		Passive.Effects.Add(MakeEffect(WacomTags::Status_Shield, 6, WacomTags::Target_Player));
		Target->Passives.Add(Passive);
		ValidateCardForMatrix(Source, *this);
		ValidateCardForMatrix(Target, *this);
		UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/50, /*Initiative*/50, /*Damage*/0);
		UBattleSession* Session = CreateSessionWithRequiredCards(Fixture, Outer, { Source, Target }, Enemy);
		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, Source->CardId);
		const FGuid TargetId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, Target->CardId);
		TestTrue(TEXT("Play selected discard to trigger OnDiscard"),
			Session->ResolveCommand(FBattleCommand::MakePlayCardOnHandCard(SourceId, TargetId)).IsOk());
		Snapshot = Session->BuildSnapshot();
		TestEqual(TEXT("OnDiscard passive executes effects when discarded by effect"), Snapshot.Player.Shield, 6);
	}

	{
		FWacomBattleFixture Fixture;
		UCardDefinition* Companion = MakeNoopMatrixCard(Outer, TEXT("Matrix.CompanionPlayer"));
		Companion->Keywords.AddTag(WacomTags::Card_Keyword_Companion);
		UCardDefinition* ReturnSource = MakeHandTargetCard(Outer, TEXT("Matrix.CompanionReturnSource"),
			MakeEffect(WacomTags::Effect_Card_DiscardSelected, 1, WacomTags::Target_SelectedHandCard));
		UCardDefinition* ReturnCard = MakeNoopMatrixCard(Outer, TEXT("Matrix.CompanionReturn"));
		FCardPassive Passive;
		Passive.Trigger = WacomTags::Passive_Trigger_OnCompanionCount;
		Passive.TriggerThreshold = 1;
		Passive.DisplayText = FText::FromString(TEXT("每打一张伙伴回手"));
		ReturnCard->Passives.Add(Passive);
		ValidateCardForMatrix(Companion, *this);
		ValidateCardForMatrix(ReturnSource, *this);
		ValidateCardForMatrix(ReturnCard, *this);
		UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/50, /*Initiative*/50, /*Damage*/0);
		UBattleSession* Session = CreateSessionWithRequiredCards(Fixture, Outer, { Companion, ReturnSource, ReturnCard }, Enemy);
		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const FGuid ReturnSourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, ReturnSource->CardId);
		const FGuid ReturnId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, ReturnCard->CardId);
		const FGuid CompanionId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, Companion->CardId);
		TestTrue(TEXT("First discard return card by effect"),
			Session->ResolveCommand(FBattleCommand::MakePlayCardOnHandCard(ReturnSourceId, ReturnId)).IsOk());
		Snapshot = Session->BuildSnapshot();
		TestEqual(TEXT("OnCompanionCount effects did not run while moving source out"), Snapshot.Player.Shield, 0);
		TestTrue(TEXT("Play companion to trigger return"), Session->ResolveCommand(FBattleCommand::MakePlayCard(CompanionId)).IsOk());
		Snapshot = Session->BuildSnapshot();
		TestTrue(TEXT("OnCompanionCount moved card back to hand"), FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, ReturnCard->CardId).IsValid());
		TestEqual(TEXT("OnCompanionCount still does not execute Effects"), Snapshot.Player.Shield, 0);
	}

	{
		FWacomBattleFixture Fixture;
		UCardDefinition* TwilightWatcher = MakeNoopMatrixCard(Outer, TEXT("Matrix.TwilightWatcher"));
		FCardPassive Passive;
		Passive.Trigger = WacomTags::Passive_Trigger_OnTwilightTriggered;
		Passive.DisplayText = FText::FromString(TEXT("Event only"));
		TwilightWatcher->Passives.Add(Passive);
		UCardDefinition* TwilightCard = MakeEnemyPartCard(Outer, TEXT("Matrix.TwilightCard"), {
			MakeEffect(WacomTags::Effect_ApplyStatus_Twilight, 1, WacomTags::Target_SingleEnemyPart)
		});
		ValidateCardForMatrix(TwilightWatcher, *this);
		ValidateCardForMatrix(TwilightCard, *this);
		UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/50, /*Initiative*/50, /*Damage*/0);
		UBattleSession* Session = CreateSessionWithRequiredCards(Fixture, Outer, { TwilightWatcher, TwilightCard }, Enemy);
		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const FGuid PartId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);
		const FBattleResolution Resolution =
			PlayCardByDefinition(Session, Snapshot, TwilightCard, PartId, *this);
		const TArray<FBattleEvent>& Events = Resolution.Events;
		TestTrue(TEXT("OnTwilightTriggered emits PassiveTriggered event only"), FWacomBattleFixture::HasEvent(Events, EBattleEventType::PassiveTriggered, WacomTags::Passive_Trigger_OnTwilightTriggered));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleRuleContentMatrixZoneHookSpec,
	"Wacom.Battle.RuleContentMatrix.ZoneHookMatrixMatchesRuntimeResolver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleRuleContentMatrixZoneHookSpec::RunTest(const FString& /*Parameters*/)
{
	UObject* Outer = GetTransientPackage();

	{
		bool bCoveredLeftZone = false;
		for (int32 Seed = 1; Seed <= 80 && !bCoveredLeftZone; ++Seed)
		{
			FWacomBattleFixture Fixture;
			UCardDefinition* LeftCard = MakeMatrixCard(Outer, FName(*FString::Printf(TEXT("Matrix.ZoneHookOnPlay.%d"), Seed)), 0, ECardTargetMode::None, {});
			FCardZoneHook Hook;
			Hook.Zone = WacomTags::HandZone_Left;
			Hook.Trigger = WacomTags::ZoneHook_Trigger_OnPlay;
			Hook.ExtraEffects.Add(MakeEffect(WacomTags::Status_Shield, 3, WacomTags::Target_Player));
			LeftCard->ZoneHooks.Add(Hook);
			ValidateCardForMatrix(LeftCard, *this);
			UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/50, /*Initiative*/50, /*Damage*/0);
			UBattleSession* Session = CreateMatrixSession(
				Fixture,
				MakeNoopMatrixCard(Outer, FName(*FString::Printf(TEXT("Matrix.ZoneHook.LeftAnchor.%d"), Seed))),
				MakeNoopMatrixCard(Outer, FName(*FString::Printf(TEXT("Matrix.ZoneHook.RightAnchor.%d"), Seed))),
				{
					LeftCard,
					MakeNoopMatrixCard(Outer, FName(*FString::Printf(TEXT("Matrix.ZoneHook.FillerA.%d"), Seed))),
					MakeNoopMatrixCard(Outer, FName(*FString::Printf(TEXT("Matrix.ZoneHook.FillerB.%d"), Seed))),
					MakeNoopMatrixCard(Outer, FName(*FString::Printf(TEXT("Matrix.ZoneHook.FillerC.%d"), Seed))),
					MakeNoopMatrixCard(Outer, FName(*FString::Printf(TEXT("Matrix.ZoneHook.FillerD.%d"), Seed)))
				},
				Enemy,
				Seed);
			FBattleSnapshot Snapshot = Session->BuildSnapshot();
			const FGuid LeftId = FWacomBattleFixture::FindHandInstanceByCardIdInZone(Snapshot, LeftCard->CardId, EHandZone::Left);
			if (!LeftId.IsValid())
			{
				continue;
			}
			TestTrue(TEXT("Play left-zone OnPlay hook card"),
				Session->ResolveCommand(FBattleCommand::MakePlayCard(LeftId)).IsOk());
			Snapshot = Session->BuildSnapshot();
			TestEqual(TEXT("OnPlay zone hook executes extra effects"), Snapshot.Player.Shield, 3);
			bCoveredLeftZone = true;
		}
		TestTrue(TEXT("At least one seed placed OnPlay hook card in Left zone"), bCoveredLeftZone);
	}

	{
		bool bCoveredLeftZone = false;
		for (int32 Seed = 1; Seed <= 80 && !bCoveredLeftZone; ++Seed)
		{
			FWacomBattleFixture Fixture;
			UCardDefinition* LeftCard = MakeEnemyPartCard(Outer, FName(*FString::Printf(TEXT("Matrix.ZoneHookPerfectRelease.%d"), Seed)), {
				MakeEffect(WacomTags::Effect_Damage, 1, WacomTags::Target_SingleEnemyPart)
			}, /*Cost*/5);
			FCardZoneHook Hook;
			Hook.Zone = WacomTags::HandZone_Left;
			Hook.Trigger = WacomTags::ZoneHook_Trigger_OnPerfectReleaseHit;
			LeftCard->ZoneHooks.Add(Hook);
			ValidateCardForMatrix(LeftCard, *this);
			UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/50, /*Initiative*/5, /*Damage*/0);
			UBattleSession* Session = CreateMatrixSession(
				Fixture,
				MakeNoopMatrixCard(Outer, FName(*FString::Printf(TEXT("Matrix.ZoneHookPerfect.LeftAnchor.%d"), Seed))),
				MakeNoopMatrixCard(Outer, FName(*FString::Printf(TEXT("Matrix.ZoneHookPerfect.RightAnchor.%d"), Seed))),
				{
					LeftCard,
					MakeNoopMatrixCard(Outer, FName(*FString::Printf(TEXT("Matrix.ZoneHookPerfect.FillerA.%d"), Seed))),
					MakeNoopMatrixCard(Outer, FName(*FString::Printf(TEXT("Matrix.ZoneHookPerfect.FillerB.%d"), Seed))),
					MakeNoopMatrixCard(Outer, FName(*FString::Printf(TEXT("Matrix.ZoneHookPerfect.FillerC.%d"), Seed))),
					MakeNoopMatrixCard(Outer, FName(*FString::Printf(TEXT("Matrix.ZoneHookPerfect.FillerD.%d"), Seed)))
				},
				Enemy,
				Seed);
			FBattleSnapshot Snapshot = Session->BuildSnapshot();
			const FGuid LeftId = FWacomBattleFixture::FindHandInstanceByCardIdInZone(Snapshot, LeftCard->CardId, EHandZone::Left);
			if (!LeftId.IsValid())
			{
				continue;
			}
			const FGuid PartId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);
			const FBattleResolution Resolution = Session->ResolveCommand(
				FWacomBattleFixture::MakePlayCardOnPartInstance(Snapshot, LeftId, PartId));
			TestTrue(TEXT("Play left-zone perfect-release hook card"), Resolution.IsOk());
			const TArray<FBattleEvent>& Events = Resolution.Events;
			Snapshot = Session->BuildSnapshot();
			TestTrue(TEXT("Perfect release hit event emitted"), FWacomBattleFixture::HasEvent(Events, EBattleEventType::InitiativeHit));
			TestEqual(TEXT("Empty OnPerfectReleaseHit hook skips initiative push"), FWacomBattleFixture::FindPartInitiative(Snapshot, 0), 5);
			TestEqual(TEXT("No InitiativePushed event when skip hook applies"), FWacomBattleFixture::CountEvents(Events, EBattleEventType::InitiativePushed), 0);
			bCoveredLeftZone = true;
		}
		TestTrue(TEXT("At least one seed placed perfect hook card in Left zone"), bCoveredLeftZone);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleRuleContentMatrixEnemyIntentSpec,
	"Wacom.Battle.RuleContentMatrix.ValidatorAllowedEnemyIntentEffectsExecuteOnEndTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleRuleContentMatrixEnemyIntentSpec::RunTest(const FString& /*Parameters*/)
{
	UObject* Outer = GetTransientPackage();

	{
		FWacomBattleFixture Fixture;
		UEnemyDefinition* Enemy = MakeEnemyWithIntent(Outer, {
			MakeIntentEffect(WacomTags::Effect_Damage, 7, WacomTags::Target_Player),
			MakeIntentEffect(WacomTags::Effect_ApplyStatus_Poison, 2, WacomTags::Target_Player),
			MakeIntentEffect(WacomTags::Effect_ApplyStatus_Slow, 1, WacomTags::Target_Player)
		});
		UBattleSession* Session = CreateSessionWithRequiredCards(
			Fixture,
			Outer,
			{ MakeNoopMatrixCard(Outer, TEXT("Matrix.EnemyIntent.Filler")) },
			Enemy);
		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		TestTrue(TEXT("End turn resolves player-targeting enemy intent"), Session->ResolveCommand(FBattleCommand::MakeEndTurn()).IsOk());
		Snapshot = Session->BuildSnapshot();
		TestEqual(TEXT("Enemy intent damages player and poison ticks after action"), Snapshot.Player.CurrentHp, Snapshot.Player.MaxHp - 9);
		TestEqual(TEXT("Enemy intent applies poison to player"), FWacomBattleFixture::GetStatusStacks(Snapshot.Player.StatusStacks, WacomTags::Status_Poison), 2);
		int32 SlowCardCount = 0;
		for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
		{
			if (FWacomBattleFixture::GetStatusStacks(
				Card.StatusStacks,
				WacomTags::Status_Slow) == 1)
			{
				++SlowCardCount;
			}
		}
		TestEqual(TEXT("Enemy intent materializes Slow on one hand card"), SlowCardCount, 1);
	}

	{
		FWacomBattleFixture Fixture;
		UEnemyDefinition* Enemy = MakeEnemyWithIntent(Outer, {
			MakeIntentEffect(WacomTags::Status_Shield, 5, WacomTags::Target_Self),
			MakeIntentEffect(WacomTags::Effect_ApplyStatus_Freeze, 1, WacomTags::Target_Self)
		});
		UBattleSession* Session = CreateSessionWithRequiredCards(
			Fixture,
			Outer,
			{ MakeNoopMatrixCard(Outer, TEXT("Matrix.EnemyIntent.SelfFiller")) },
			Enemy);
		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		TestTrue(TEXT("End turn resolves self-targeting enemy intent"), Session->ResolveCommand(FBattleCommand::MakeEndTurn()).IsOk());
		Snapshot = Session->BuildSnapshot();
		const FEnemyPartSnapshot* Part = FWacomBattleFixture::GetEnemyPartSnapshot(Snapshot, 0);
		TestNotNull(TEXT("Primary part exists"), Part);
		TestEqual(TEXT("Enemy intent adds self shield"), Part ? Part->Shield : -1, 5);
		TestEqual(TEXT("Enemy intent applies self freeze then action refresh keeps stack"), FWacomBattleFixture::GetStatusStacks(Part ? Part->StatusStacks : TMap<FGameplayTag, int32>(), WacomTags::Status_Freeze), 1);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleRuleContentMatrixReservedRejectedSpec,
	"Wacom.Battle.RuleContentMatrix.ReservedOrRejectedMatrixEntriesRemainValidationOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleRuleContentMatrixReservedRejectedSpec::RunTest(const FString& /*Parameters*/)
{
	UObject* Outer = GetTransientPackage();

	{
		UCardDefinition* Card = MakeEnemyPartCard(Outer, TEXT("Matrix.Reject.UnknownEffect"), {
			MakeEffect(WacomTags::CardLocation_Hand, 1, WacomTags::Target_SingleEnemyPart)
		});
		TestTrue(TEXT("Unknown/reserved effect tag is rejected"), ValidateCardRejected(Card));
	}

	{
		UCardDefinition* Card = MakeEnemyPartCard(Outer, TEXT("Matrix.Reject.OnTurnStart"), {
			MakeEffect(WacomTags::Effect_Damage, 1, WacomTags::Target_SingleEnemyPart)
		});
		FCardPassive Passive;
		Passive.Trigger = WacomTags::Passive_Trigger_OnTurnStart;
		Passive.Effects.Add(MakeEffect(WacomTags::Status_Shield, 1, WacomTags::Target_Player));
		Card->Passives = { Passive };
		TestTrue(TEXT("Reserved OnTurnStart passive is rejected"), ValidateCardRejected(Card));
		Passive.Trigger = WacomTags::Passive_Trigger_OnTurnEnd;
		Card->Passives = { Passive };
		TestTrue(TEXT("Reserved OnTurnEnd passive is rejected"), ValidateCardRejected(Card));
		Passive.Trigger = WacomTags::Passive_Trigger_OnDraw;
		Card->Passives = { Passive };
		TestTrue(TEXT("Reserved OnDraw passive is rejected"), ValidateCardRejected(Card));
	}

	{
		UCardDefinition* Card = MakeEnemyPartCard(Outer, TEXT("Matrix.Reject.OnTwilightEffects"), {
			MakeEffect(WacomTags::Effect_Damage, 1, WacomTags::Target_SingleEnemyPart)
		});
		FCardPassive Passive;
		Passive.Trigger = WacomTags::Passive_Trigger_OnTwilightTriggered;
		Passive.Effects.Add(MakeEffect(WacomTags::Status_Shield, 1, WacomTags::Target_Player));
		Card->Passives = { Passive };
		TestTrue(TEXT("OnTwilightTriggered with effects is rejected"), ValidateCardRejected(Card));
	}

	{
		UCardDefinition* Card = MakeEnemyPartCard(Outer, TEXT("Matrix.Reject.ShieldAsStackCondition"), {
			MakeEffect(WacomTags::Effect_Damage, 1, WacomTags::Target_SingleEnemyPart)
		});
		Card->Effects[0].Condition.ConditionType = WacomTags::Condition_Target_HasStatus;
		Card->Effects[0].Condition.ParamTag = WacomTags::Status_Shield;
		TestTrue(TEXT("Status.Shield cannot be used as HasStatus condition"), ValidateCardRejected(Card));
	}

	{
		UEnemyDefinition* Enemy = NewMatrixObject<UEnemyDefinition>(Outer);
		Enemy->EnemyId = TEXT("Matrix.Enemy.Reject");

		UEnemyPartDefinition* Part = NewMatrixObject<UEnemyPartDefinition>(Outer);
		Part->PartId = TEXT("Matrix.EnemyPart.Reject");
		Part->DisplayName = FText::FromName(Part->PartId);
		Part->MaxHp = 10;

		FEnemyPartSlot Slot;
		Slot.PartSlotId = TEXT("Core");
		Slot.PartDef = Part;
		Enemy->Parts = { Slot };

		auto MakeRejectedBehavior = [Outer](const FIntentEffect& Effect) -> UEnemyBehaviorDefinition*
		{
			UEnemyBehaviorDefinition* Behavior = NewMatrixObject<UEnemyBehaviorDefinition>(Outer);
			Behavior->BehaviorId = FName(*FString::Printf(TEXT("Matrix.Behavior.Reject.%s"), *FGuid::NewGuid().ToString(EGuidFormats::Short)));
			Behavior->InitialPhaseId = TEXT("Default");

			FWacomEnemyBehaviorIntent IntentEntry;
			IntentEntry.Intent.IntentId = TEXT("Matrix.Intent.Reject");
			IntentEntry.Intent.Initiative = 1;
			IntentEntry.Intent.Effects = { Effect };

			FWacomEnemyIntentSetDefinition IntentSet;
			IntentSet.IntentSetId = TEXT("Core.Main");
			IntentSet.AppliesToPartSlotId = TEXT("Core");
			IntentSet.Intents = { IntentEntry };

			FWacomEnemyPhaseDefinition Phase;
			Phase.PhaseId = TEXT("Default");
			Phase.IntentSets = { IntentSet };
			Behavior->Phases = { Phase };
			return Behavior;
		};

		TestTrue(TEXT("Enemy intent cannot use card-only draw effect"),
			ValidateEnemyBehaviorRejected(
				MakeRejectedBehavior(MakeIntentEffect(WacomTags::Effect_Draw, 1, WacomTags::Target_Player)),
				Enemy));

		TestTrue(TEXT("Enemy intent cannot use hand-card target effect"),
			ValidateEnemyBehaviorRejected(
				MakeRejectedBehavior(MakeIntentEffect(WacomTags::Effect_Card_AddCost, 1, WacomTags::Target_SelectedHandCard)),
				Enemy));

		TestTrue(TEXT("Enemy intent cannot target all enemy parts"),
			ValidateEnemyBehaviorRejected(
				MakeRejectedBehavior(MakeIntentEffect(WacomTags::Effect_Damage, 1, WacomTags::Target_AllEnemyParts)),
				Enemy));

		TestTrue(TEXT("Enemy intent cannot use unknown effect"),
			ValidateEnemyBehaviorRejected(
				MakeRejectedBehavior(MakeIntentEffect(WacomTags::CardLocation_Hand, 1, WacomTags::Target_Player)),
				Enemy));
	}

	return true;
}
