// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Characters/CharacterDefinition.h"
#include "Commands/BattleCommand.h"
#include "Resolution/BattleCardTargetPreview.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Snapshots/EnemySnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomInteractionTargetTypes.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
	FCardEffect MakePreviewEffect(
		const FGameplayTag& EffectType,
		int32 Magnitude,
		const FGameplayTag& Target = WacomTags::Target_SingleEnemyPart)
	{
		FCardEffect Effect;
		Effect.EffectType = EffectType;
		Effect.Magnitude = Magnitude;
		Effect.Target = Target;
		return Effect;
	}

	UCardDefinition* MakeEnemyPartPreviewCard(
		FWacomBattleFixture& Fixture,
		const TCHAR* IdPrefix,
		const TArray<FCardEffect>& Effects,
		int32 Cost = 0,
		const TArray<FGameplayTag>& Keywords = {})
	{
		UCardDefinition* Card = Fixture.MakeSimpleDamageCard(Cost, 0);
		Card->CardId = FName(*FString::Printf(
			TEXT("%s.%s"),
			IdPrefix,
			*FGuid::NewGuid().ToString(EGuidFormats::Short)));
		Card->Effects = Effects;
		for (const FGameplayTag& Keyword : Keywords)
		{
			Card->Keywords.AddTag(Keyword);
		}
		return Card;
	}

	UCardDefinition* MakeGainKeywordHandTargetCard(FWacomBattleFixture& Fixture)
	{
		UCardDefinition* Card = Fixture.MakeHandCardCostModifierCard(/*Cost*/0, /*Magnitude*/0, /*bReduceCost*/false);
		Card->CardId = FName(*FString::Printf(
			TEXT("Preview.GainKeyword.%s"),
			*FGuid::NewGuid().ToString(EGuidFormats::Short)));
		Card->Effects.Reset();

		FCardEffect Effect;
		Effect.EffectType = WacomTags::Effect_GainKeyword;
		Effect.Target = WacomTags::Target_SelectedHandCard;
		Effect.TargetZone = WacomTags::Card_Keyword_Companion;
		Card->Effects.Add(Effect);
		return Card;
	}

	FWacomInteractionTargetHandle MakeTargetPreviewPartHandle(const FEnemyPartSnapshot& Part)
	{
		return FWacomInteractionTargetHandle::ForWorldTarget(
			Part.InstanceId,
			nullptr,
			FVector::ZeroVector,
			FVector2D::ZeroVector,
			FGameplayTag(),
			NAME_None,
			Part.EncounterId,
			Part.EnemySlotId,
			Part.PartSlotId);
	}

	FWacomInteractionTargetHandle MakeTargetPreviewFirstPartHandle(const FBattleSnapshot& Snapshot)
	{
		const FEnemyPartSnapshot* Part = FWacomBattleFixture::GetEnemyPartSnapshot(Snapshot, 0);
		return Part ? MakeTargetPreviewPartHandle(*Part) : FWacomInteractionTargetHandle();
	}

	UBattleSession* CreateDeckSession(
		FWacomBattleFixture& Fixture,
		const TArray<UCardDefinition*>& RequiredCards,
		UEnemyDefinition* Enemy,
		int32 Seed = 1)
	{
		TArray<UCardDefinition*> Deck = RequiredCards;
		while (Deck.Num() < 5)
		{
			Deck.Add(Fixture.MakeNoopCard(0));
		}

		return Fixture.CreateSession(
			Fixture.MakeCharacter(Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0), Deck),
			Enemy,
			Seed);
	}

	TStrongObjectPtr<UBattleSession> CreateCapacityPreviewSession(
		FWacomBattleFixture& Fixture,
		UCardDefinition* Card,
		int32 EnemyHp = 20)
	{
		UCharacterDefinition* Character = Fixture.MakeCharacter(
			Fixture.MakeNoopCard(0),
			Fixture.MakeNoopCard(0),
			{});
		UEnemyDefinition* Enemy =
			Fixture.MakeSinglePartEnemy(/*Hp*/EnemyHp, /*Initiative*/10, /*IntentResist*/0);

		TStrongObjectPtr<UBattleSession> Session(NewObject<UBattleSession>());
		FBattleInitParams Params;
		Params.Character = Character;
		Params.RandomSeed = 11;

		FBattleEnemySlotInit EnemySlot;
		EnemySlot.EnemySlotId = TEXT("Enemy");
		EnemySlot.Enemy = Enemy;
		Params.EnemySlots.Add(EnemySlot);

		FBattleDeckEntry Entry;
		Entry.Definition = Card;
		Entry.CapacityEffectTags.AddTag(WacomTags::Card_CapacityEffect_WeaponDamagePlus3);
		Params.BattleDeckEntries.Add(Entry);

		const FWacomStatus Status = Session->Initialize(Params);
		check(Status.IsOk());
		return Session;
	}

	int32 FindEffectMagnitude(
		const FBattleCardTargetPreview& Preview,
		const FGameplayTag& EffectType,
		int32 EffectIndex = INDEX_NONE)
	{
		for (const FBattleCardTargetPreviewEffect& EffectPreview : Preview.Effects)
		{
			if (EffectPreview.EffectType == EffectType
				&& (EffectIndex == INDEX_NONE || EffectPreview.EffectIndex == EffectIndex))
			{
				return EffectPreview.Magnitude;
			}
		}
		return INDEX_NONE;
	}

	int32 GetRuntimeCostInHand(const FBattleSnapshot& Snapshot, const FGuid& CardId)
	{
		if (const FHandCardSnapshot* Card = FWacomBattleFixture::FindHandCardByInstanceId(Snapshot, CardId))
		{
			return Card->RuntimeCost;
		}
		return INDEX_NONE;
	}

	void TestTargetStatusAndModifiers(FAutomationTestBase& Test)
	{
		FWacomBattleFixture Fixture;
		FCardEffect PoisonEffect =
			MakePreviewEffect(WacomTags::Effect_ApplyStatus_Poison, 4);
		UCardDefinition* PoisonCard =
			MakeEnemyPartPreviewCard(Fixture, TEXT("Preview.PoisonSeed"), { PoisonEffect });

		FCardEffect StackDamageEffect =
			MakePreviewEffect(WacomTags::Effect_Damage, 0);
		StackDamageEffect.MagnitudeSource = WacomTags::Magnitude_Source_TargetStatusStacks;
		StackDamageEffect.TargetZone = WacomTags::Status_Poison;
		UCardDefinition* StackDamageCard =
			MakeEnemyPartPreviewCard(Fixture, TEXT("Preview.StackDamage"), { StackDamageEffect });

		FCardEffect ModifierDamageEffect =
			MakePreviewEffect(WacomTags::Effect_Damage, 2);
		FMagnitudeModifier AddMod;
		AddMod.Condition.ConditionType = WacomTags::Condition_Target_HasStatus;
		AddMod.Condition.ParamTag = WacomTags::Status_Poison;
		AddMod.Op = EMagnitudeModOp::Add;
		AddMod.Value = 3;
		FMagnitudeModifier MultiplyMod;
		MultiplyMod.Condition.ConditionType = WacomTags::Condition_Target_HasStatus;
		MultiplyMod.Condition.ParamTag = WacomTags::Status_Poison;
		MultiplyMod.Op = EMagnitudeModOp::Multiply;
		MultiplyMod.Value = 2;
		ModifierDamageEffect.MagnitudeModifiers = { AddMod, MultiplyMod };
		UCardDefinition* ModifierDamageCard =
			MakeEnemyPartPreviewCard(Fixture, TEXT("Preview.ModifierDamage"), { ModifierDamageEffect });

		UBattleSession* Session = CreateDeckSession(
			Fixture,
			{ PoisonCard, StackDamageCard, ModifierDamageCard },
			Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/100, /*Initiative*/50, /*IntentResist*/0, /*Damage*/0));
		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		FWacomInteractionTargetHandle TargetHandle = MakeTargetPreviewFirstPartHandle(Snapshot);

		const FGuid ModifierCardId =
			FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, ModifierDamageCard->CardId);
		const FBattleCardTargetPreview ModifierPreviewWithoutPoison =
			Session->BuildCardTargetPreview(ModifierCardId, TargetHandle);
		Test.TestTrue(TEXT("Modifier preview without poison exists"), ModifierPreviewWithoutPoison.bHasPreview);
		Test.TestEqual(TEXT("Condition modifiers do not apply when target lacks status"),
			FindEffectMagnitude(ModifierPreviewWithoutPoison, WacomTags::Effect_Damage),
			2);

		const FGuid PoisonCardId =
			FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, PoisonCard->CardId);
		Test.TestTrue(TEXT("Seed poison card plays"),
			Session->SubmitCommand(FWacomBattleFixture::MakePlayCardOnPart(Snapshot, PoisonCardId, 0)).IsOk());
		Snapshot = Session->BuildSnapshot();
		TargetHandle = MakeTargetPreviewFirstPartHandle(Snapshot);

		const FGuid StackDamageCardId =
			FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, StackDamageCard->CardId);
		const FBattleCardTargetPreview StackPreview =
			Session->BuildCardTargetPreview(StackDamageCardId, TargetHandle);
		Test.TestTrue(TEXT("TargetStatusStacks preview exists"), StackPreview.bHasPreview);
		Test.TestEqual(TEXT("TargetStatusStacks reads target poison stacks"),
			FindEffectMagnitude(StackPreview, WacomTags::Effect_Damage),
			4);

		const FBattleCardTargetPreview ModifierPreviewWithPoison =
			Session->BuildCardTargetPreview(ModifierCardId, TargetHandle);
		Test.TestTrue(TEXT("Modifier preview with poison exists"), ModifierPreviewWithPoison.bHasPreview);
		Test.TestEqual(TEXT("Conditional add and multiply modifiers apply in order"),
			FindEffectMagnitude(ModifierPreviewWithPoison, WacomTags::Effect_Damage),
			10);
	}

	void TestWeaponCapacityAndClamp(FAutomationTestBase& Test)
	{
		{
			FWacomBattleFixture Fixture;
			UCardDefinition* Weapon = Fixture.MakeDamageCardWithKeywords(
				/*Cost*/1,
				/*Damage*/4,
				{ WacomTags::Card_Keyword_Weapon });
			TStrongObjectPtr<UBattleSession> Session =
				CreateCapacityPreviewSession(Fixture, Weapon, /*EnemyHp*/20);
			const FBattleSnapshot Before = Session->BuildSnapshot();
			const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Before, Weapon->CardId);

			const FBattleCardTargetPreview Preview =
				Session->BuildCardTargetPreview(CardId, MakeTargetPreviewFirstPartHandle(Before));
			Test.TestTrue(TEXT("Weapon preview exists"), Preview.bHasPreview);
			Test.TestEqual(TEXT("WeaponDamagePlus3 preview applies to weapon damage"),
				FindEffectMagnitude(Preview, WacomTags::Effect_Damage),
				7);

			Test.TestTrue(TEXT("Play weapon with capacity effect"),
				Session->SubmitCommand(FWacomBattleFixture::MakePlayCardOnPart(Before, CardId, 0)).IsOk());
			Test.TestEqual(TEXT("Formal damage matches preview weapon capacity bonus"),
				FWacomBattleFixture::FindPartHp(Session->BuildSnapshot(), 0),
				13);
		}

		{
			FWacomBattleFixture Fixture;
			UCardDefinition* NegativeWeapon = Fixture.MakeDamageCardWithKeywords(
				/*Cost*/1,
				/*Damage*/-5,
				{ WacomTags::Card_Keyword_Weapon });
			TStrongObjectPtr<UBattleSession> Session =
				CreateCapacityPreviewSession(Fixture, NegativeWeapon, /*EnemyHp*/20);
			const FBattleSnapshot Before = Session->BuildSnapshot();
			const FGuid CardId =
				FWacomBattleFixture::FindHandInstanceByCardId(Before, NegativeWeapon->CardId);

			const FBattleCardTargetPreview Preview =
				Session->BuildCardTargetPreview(CardId, MakeTargetPreviewFirstPartHandle(Before));
			Test.TestTrue(TEXT("Negative weapon preview exists"), Preview.bHasPreview);
			Test.TestEqual(TEXT("Damage preview clamps after weapon capacity modifier"),
				FindEffectMagnitude(Preview, WacomTags::Effect_Damage),
				0);

			Test.TestTrue(TEXT("Play negative weapon with capacity effect"),
				Session->SubmitCommand(FWacomBattleFixture::MakePlayCardOnPart(Before, CardId, 0)).IsOk());
			Test.TestEqual(TEXT("Formal damage clamp keeps HP unchanged"),
				FWacomBattleFixture::FindPartHp(Session->BuildSnapshot(), 0),
				20);
		}
	}

	void TestInvalidTarget(FAutomationTestBase& Test)
	{
		FWacomBattleFixture Fixture;
		UCardDefinition* DamageCard = Fixture.MakeSimpleDamageCard(/*Cost*/0, /*Damage*/3);
		UBattleSession* Session = CreateDeckSession(
			Fixture,
			{ DamageCard },
			Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/50, /*Initiative*/50, /*IntentResist*/0, /*Damage*/0));
		const FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, DamageCard->CardId);

		const FBattleCardTargetPreview Preview =
			Session->BuildCardTargetPreview(CardId, FWacomInteractionTargetHandle());
		Test.TestFalse(TEXT("Invalid target returns no usable preview"), Preview.bHasPreview);
		Test.TestFalse(TEXT("Invalid target validation rejects"), Preview.Validation.bCanTarget);
		Test.TestEqual(TEXT("Invalid target emits no effect preview"), Preview.Effects.Num(), 0);
	}

	void TestHandCardCostPreview(FAutomationTestBase& Test)
	{
		FWacomBattleFixture Fixture;
		UCardDefinition* SourceCard =
			Fixture.MakeHandCardCostModifierCard(/*Cost*/0, /*Magnitude*/2, /*bReduceCost*/false);
		UCardDefinition* TargetCard = Fixture.MakeNoopCard(/*Cost*/3);
		UBattleSession* Session = CreateDeckSession(
			Fixture,
			{ SourceCard, TargetCard },
			Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/50, /*Initiative*/50, /*IntentResist*/0, /*Damage*/0));
		FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, SourceCard->CardId);
		const FGuid TargetId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetCard->CardId);

		const FBattleCardTargetPreview Preview =
			Session->BuildCardTargetPreview(SourceId, FWacomInteractionTargetHandle::ForCardTarget(TargetId, nullptr));
		Test.TestTrue(TEXT("Hand-card cost preview exists"), Preview.bHasPreview);
		Test.TestEqual(TEXT("Target hand-card cost before preview"), Preview.TargetHandCardRuntimeCostBefore, 3);
		Test.TestEqual(TEXT("Target hand-card cost after AddCost preview"), Preview.TargetHandCardRuntimeCostAfter, 5);
		if (Test.TestTrue(TEXT("Cost preview includes one effect"), Preview.Effects.IsValidIndex(0)))
		{
			Test.TestEqual(TEXT("Per-effect cost preview before"), Preview.Effects[0].TargetHandCardRuntimeCostBefore, 3);
			Test.TestEqual(TEXT("Per-effect cost preview after"), Preview.Effects[0].TargetHandCardRuntimeCostAfter, 5);
		}
		Test.TestEqual(TEXT("Preview does not mutate target runtime cost"),
			GetRuntimeCostInHand(Session->BuildSnapshot(), TargetId),
			3);

		Test.TestTrue(TEXT("Formal AddCost play succeeds"),
			Session->SubmitCommand(FBattleCommand::MakePlayCardOnHandCard(SourceId, TargetId)).IsOk());
		Test.TestEqual(TEXT("Formal AddCost matches preview"),
			GetRuntimeCostInHand(Session->BuildSnapshot(), TargetId),
			5);
	}

	void TestHandCardActionPreview(FAutomationTestBase& Test)
	{
		{
			FWacomBattleFixture Fixture;
			UCardDefinition* SourceCard = Fixture.MakeSelectedHandCardZoneMoveCard(/*Cost*/0, /*bExhaust*/false);
			UCardDefinition* TargetCard = Fixture.MakeNoopCard(/*Cost*/3);
			UBattleSession* Session = CreateDeckSession(
				Fixture,
				{ SourceCard, TargetCard },
				Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/50, /*Initiative*/50, /*IntentResist*/0, /*Damage*/0));
			const FBattleSnapshot Snapshot = Session->BuildSnapshot();
			const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, SourceCard->CardId);
			const FGuid TargetId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetCard->CardId);

			const FBattleCardTargetPreview Preview =
				Session->BuildCardTargetPreview(SourceId, FWacomInteractionTargetHandle::ForCardTarget(TargetId, nullptr));
			Test.TestTrue(TEXT("Discard selected preview exists"), Preview.bHasPreview);
			Test.TestTrue(TEXT("Discard selected preview marks target discard"), Preview.bWouldDiscardTargetHandCard);
			Test.TestFalse(TEXT("Discard selected preview does not exhaust"), Preview.bWouldExhaustTargetHandCard);
			Test.TestNotNull(TEXT("Preview discard does not remove target"),
				FWacomBattleFixture::FindHandCardByInstanceId(Session->BuildSnapshot(), TargetId));
		}

		{
			FWacomBattleFixture Fixture;
			UCardDefinition* SourceCard = Fixture.MakeSelectedHandCardZoneMoveCard(/*Cost*/0, /*bExhaust*/true);
			UCardDefinition* TargetCard = Fixture.MakeNoopCard(/*Cost*/3);
			UBattleSession* Session = CreateDeckSession(
				Fixture,
				{ SourceCard, TargetCard },
				Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/50, /*Initiative*/50, /*IntentResist*/0, /*Damage*/0));
			const FBattleSnapshot Snapshot = Session->BuildSnapshot();
			const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, SourceCard->CardId);
			const FGuid TargetId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetCard->CardId);

			const FBattleCardTargetPreview Preview =
				Session->BuildCardTargetPreview(SourceId, FWacomInteractionTargetHandle::ForCardTarget(TargetId, nullptr));
			Test.TestTrue(TEXT("Exhaust selected preview exists"), Preview.bHasPreview);
			Test.TestTrue(TEXT("Exhaust selected preview marks target exhaust"), Preview.bWouldExhaustTargetHandCard);
			Test.TestFalse(TEXT("Exhaust selected preview does not discard"), Preview.bWouldDiscardTargetHandCard);
			Test.TestNotNull(TEXT("Preview exhaust does not remove target"),
				FWacomBattleFixture::FindHandCardByInstanceId(Session->BuildSnapshot(), TargetId));
		}

		{
			FWacomBattleFixture Fixture;
			UCardDefinition* SourceCard = MakeGainKeywordHandTargetCard(Fixture);
			UCardDefinition* TargetCard = Fixture.MakeNoopCard(/*Cost*/3);
			UBattleSession* Session = CreateDeckSession(
				Fixture,
				{ SourceCard, TargetCard },
				Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/50, /*Initiative*/50, /*IntentResist*/0, /*Damage*/0));
			const FBattleSnapshot Snapshot = Session->BuildSnapshot();
			const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, SourceCard->CardId);
			const FGuid TargetId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetCard->CardId);

			const FBattleCardTargetPreview Preview =
				Session->BuildCardTargetPreview(SourceId, FWacomInteractionTargetHandle::ForCardTarget(TargetId, nullptr));
			Test.TestTrue(TEXT("GainKeyword selected preview exists"), Preview.bHasPreview);
			Test.TestTrue(TEXT("GainKeyword selected preview marks keyword action"), Preview.bWouldGainTargetHandCardKeyword);
			Test.TestTrue(TEXT("GainKeyword selected preview carries keyword"),
				Preview.TargetHandCardKeyword.MatchesTagExact(WacomTags::Card_Keyword_Companion));
			Test.TestNotNull(TEXT("Preview gain keyword does not remove target"),
				FWacomBattleFixture::FindHandCardByInstanceId(Session->BuildSnapshot(), TargetId));
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCardTargetPreviewSpec,
	"Wacom.Battle.CardTargetPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCardTargetPreviewSpec::RunTest(const FString& /*Parameters*/)
{
	TestTargetStatusAndModifiers(*this);
	TestWeaponCapacityAndClamp(*this);
	TestInvalidTarget(*this);
	TestHandCardCostPreview(*this);
	TestHandCardActionPreview(*this);
	return true;
}
