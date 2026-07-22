// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Cards/CardZoneHook.h"
#include "Characters/CharacterDefinition.h"
#include "Commands/BattleCommand.h"
#include "Enemies/EnemyBehaviorDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/IntentEffect.h"
#include "Events/BattleEvent.h"
#include "Resolution/BattleCardActionPreview.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomEnums.h"
#include "Types/WacomInteractionTargetTypes.h"

#include "UObject/StrongObjectPtr.h"

namespace WacomBattleResistanceSpec
{
	FCardEffect MakeCardEffect(
		const FGameplayTag& EffectType,
		const int32 Magnitude,
		const FGameplayTag& Target = WacomTags::Target_SingleEnemyPart)
	{
		FCardEffect Effect;
		Effect.EffectType = EffectType;
		Effect.Magnitude = Magnitude;
		Effect.Target = Target;
		return Effect;
	}

	FIntentEffect MakeIntentEffect(
		const FGameplayTag& EffectType,
		const int32 Magnitude,
		const FGameplayTag& Target = WacomTags::Target_Player)
	{
		FIntentEffect Effect;
		Effect.EffectType = EffectType;
		Effect.Magnitude = Magnitude;
		Effect.Target = Target;
		return Effect;
	}

	UBattleSession* CreateSession(
		FWacomBattleFixture& Fixture,
		const TArray<UCardDefinition*>& RequiredCards,
		UEnemyDefinition* Enemy,
		const int32 Seed = 7)
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

	TStrongObjectPtr<UBattleSession> CreateCapacitySession(
		FWacomBattleFixture& Fixture,
		UCardDefinition* Card,
		UEnemyDefinition* Enemy)
	{
		UCharacterDefinition* Character = Fixture.MakeCharacter(
			Fixture.MakeNoopCard(0),
			Fixture.MakeNoopCard(0),
			{});

		TStrongObjectPtr<UBattleSession> Session(NewObject<UBattleSession>());
		FBattleInitParams Params;
		Params.Character = Character;
		Params.RandomSeed = 17;

		FBattleEnemySlotInit EnemySlot;
		EnemySlot.EnemySlotId = TEXT("Enemy");
		EnemySlot.Enemy = Enemy;
		Params.EnemySlots.Add(EnemySlot);

		FBattleDeckEntry Entry;
		Entry.Definition = Card;
		Entry.CapacityEffectTags.AddTag(WacomTags::Card_CapacityEffect_WeaponDamagePlus3);
		Params.BattleDeckEntries.Add(Entry);

		const FBattleInitializationResult Initialization = Session->Initialize(Params);
		check(Initialization.IsOk());
		return Session;
	}

	const FBattleEvent* FindFirstEvent(
		const FBattleResolution& Resolution,
		const EBattleEventType Type)
	{
		return Resolution.Events.FindByPredicate(
			[Type](const FBattleEvent& Event)
			{
				return Event.Type == Type;
			});
	}

	TArray<const FBattleEvent*> FindEvents(
		const FBattleResolution& Resolution,
		const EBattleEventType Type)
	{
		TArray<const FBattleEvent*> Result;
		for (const FBattleEvent& Event : Resolution.Events)
		{
			if (Event.Type == Type)
			{
				Result.Add(&Event);
			}
		}
		return Result;
	}

	FBattleResolution PlayCardOnPart(
		UBattleSession& Session,
		const UCardDefinition& Card,
		const int32 PartIndex = 0)
	{
		const FBattleSnapshot Snapshot = Session.BuildSnapshot();
		const FGuid CardId =
			FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, Card.CardId);
		return Session.ResolveCommand(
			FWacomBattleFixture::MakePlayCardOnPart(Snapshot, CardId, PartIndex));
	}

	void SetPartIntentEffects(
		UEnemyDefinition& Enemy,
		const int32 PartIndex,
		const TArray<FIntentEffect>& Effects)
	{
		UEnemyBehaviorDefinition* Behavior = Enemy.DefaultBehavior.Get();
		check(Behavior && Behavior->Phases.IsValidIndex(0));
		check(Behavior->Phases[0].IntentSets.IsValidIndex(PartIndex));
		FWacomEnemyIntentSetDefinition& IntentSet =
			Behavior->Phases[0].IntentSets[PartIndex];
		check(IntentSet.Intents.IsValidIndex(0));
		IntentSet.Intents[0].Intent.Effects = Effects;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleResistanceStrictComparisonSpec,
	"Wacom.Battle.Resistance.StrictPeakComparison",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleResistanceStrictComparisonSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleResistanceSpec;
	struct FCase
	{
		int32 PlayerDamage;
		int32 EnemyDamage;
		bool bExpectedSuccess;
	};
	const TArray<FCase> Cases = {
		{ 6, 5, true },
		{ 5, 5, false },
		{ 4, 5, false },
	};

	for (const FCase& Case : Cases)
	{
		FWacomBattleFixture Fixture;
		UCardDefinition* Card = Fixture.MakeSimpleDamageCard(/*Cost*/5, Case.PlayerDamage);
		UBattleSession* Session = CreateSession(
			Fixture,
			{ Card },
			Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/100, /*Initiative*/5, Case.EnemyDamage));
		const FBattleSnapshot Before = Session->BuildSnapshot();
		const FEnemyPartSnapshot* BeforePart = FWacomBattleFixture::GetEnemyPartSnapshot(Before, 0);
		if (!TestNotNull(TEXT("Intent snapshot exists"), BeforePart))
		{
			return false;
		}
		TestTrue(TEXT("Damage intent is marked as attack"), BeforePart->CurrentIntent.bIsAttackIntent);
		TestEqual(TEXT("Intent snapshot exposes peak attack damage"),
			BeforePart->CurrentIntent.PeakAttackDamage, Case.EnemyDamage);

		const FBattleResolution Resolution = PlayCardOnPart(*Session, *Card);
		TestTrue(TEXT("Perfect-release card resolves"), Resolution.IsOk());
		const FBattleEvent* Resistance =
			FindFirstEvent(Resolution, EBattleEventType::ResistanceResolved);
		if (!TestNotNull(TEXT("Eligible comparison emits ResistanceResolved"), Resistance))
		{
			return false;
		}
		TestEqual(TEXT("Event stores player peak single-hit damage"),
			Resistance->Amount, Case.PlayerDamage);
		TestEqual(TEXT("Event stores enemy peak single-hit damage"),
			Resistance->Count, Case.EnemyDamage);
		TestEqual(TEXT("Only strict greater-than succeeds"),
			Resistance->bSuccess, Case.bExpectedSuccess);
		TestEqual(TEXT("Successful comparison identifies the applied status"),
			Resistance->Tag == WacomTags::Status_Stunned, Case.bExpectedSuccess);

		const FBattleEvent* EnemyAction =
			FindFirstEvent(Resolution, EBattleEventType::EnemyPartActed);
		if (!TestNotNull(TEXT("Initiative-zero boundary is reached"), EnemyAction))
		{
			return false;
		}
		TestEqual(TEXT("Success skips while failure executes the enemy action"),
			EnemyAction->Count, Case.bExpectedSuccess ? 0 : 1);
		TestEqual(TEXT("Player damage agrees with resistance result"),
			Session->BuildSnapshot().Player.CurrentHp,
			Case.bExpectedSuccess ? 100 : 100 - Case.EnemyDamage);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleResistanceEligibilitySpec,
	"Wacom.Battle.Resistance.EligibilityFilters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleResistanceEligibilitySpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleResistanceSpec;
	auto TestNoComparison = [this](
		const TCHAR* Label,
		FWacomBattleFixture& Fixture,
		UCardDefinition* Card,
		UEnemyDefinition* Enemy)
	{
		UBattleSession* Session = CreateSession(Fixture, { Card }, Enemy);
		const FBattleResolution Resolution = PlayCardOnPart(*Session, *Card);
		TestTrue(FString::Printf(TEXT("%s resolves"), Label), Resolution.IsOk());
		TestNull(FString::Printf(TEXT("%s emits no resistance comparison"), Label),
			FindFirstEvent(Resolution, EBattleEventType::ResistanceResolved));
	};

	{
		FWacomBattleFixture Fixture;
		UCardDefinition* ZeroDamage = Fixture.MakeSimpleDamageCard(/*Cost*/5, /*Damage*/0);
		TestNoComparison(TEXT("Zero damage"), Fixture, ZeroDamage,
			Fixture.MakeSinglePartEnemyWithIntentDamage(100, 5, 3));
	}
	{
		FWacomBattleFixture Fixture;
		UCardDefinition* ConditionalDamage = Fixture.MakeSimpleDamageCard(/*Cost*/5, /*Damage*/9);
		ConditionalDamage->Effects[0].Condition.ConditionType = WacomTags::Condition_Target_HasStatus;
		ConditionalDamage->Effects[0].Condition.ParamTag = WacomTags::Status_Poison;
		TestNoComparison(TEXT("Failed damage condition"), Fixture, ConditionalDamage,
			Fixture.MakeSinglePartEnemyWithIntentDamage(100, 5, 3));
	}
	{
		FWacomBattleFixture Fixture;
		UCardDefinition* Damage = Fixture.MakeSimpleDamageCard(/*Cost*/5, /*Damage*/9);
		TestNoComparison(TEXT("Non-attack intent"), Fixture, Damage,
			Fixture.MakeSinglePartEnemyWithIntentDamage(100, 5, 0));
	}
	{
		FWacomBattleFixture Fixture;
		UCardDefinition* SwiftDamage = Fixture.MakeDamageCardWithKeywords(
			/*Cost*/5,
			/*Damage*/9,
			{ WacomTags::Card_Keyword_Swift });
		UBattleSession* Session = CreateSession(
			Fixture,
			{ SwiftDamage },
			Fixture.MakeSinglePartEnemyWithIntentDamage(100, 5, 3));
		const FBattleResolution Resolution = PlayCardOnPart(*Session, *SwiftDamage);
		TestNull(TEXT("Swift emits no InitiativeHit"),
			FindFirstEvent(Resolution, EBattleEventType::InitiativeHit));
		TestNull(TEXT("Swift emits no resistance comparison"),
			FindFirstEvent(Resolution, EBattleEventType::ResistanceResolved));
	}
	{
		FWacomBattleFixture Fixture;
		UCardDefinition* Freeze = Fixture.MakeSimpleDamageCard(/*Cost*/0, /*Damage*/0);
		Freeze->Effects = {
			MakeCardEffect(WacomTags::Effect_ApplyStatus_Freeze, 1)
		};
		UCardDefinition* Damage = Fixture.MakeSimpleDamageCard(/*Cost*/5, /*Damage*/9);
		UBattleSession* Session = CreateSession(
			Fixture,
			{ Freeze, Damage },
			Fixture.MakeSinglePartEnemyWithIntentDamage(100, 5, 3));
		TestTrue(TEXT("Freeze seed resolves"), PlayCardOnPart(*Session, *Freeze).IsOk());
		const FBattleResolution Resolution = PlayCardOnPart(*Session, *Damage);
		TestNull(TEXT("Part frozen before play emits no resistance comparison"),
			FindFirstEvent(Resolution, EBattleEventType::ResistanceResolved));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleResistanceTargetScopeSpec,
	"Wacom.Battle.Resistance.TargetScopeAndPerPartPeak",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleResistanceTargetScopeSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleResistanceSpec;
	{
		FWacomBattleFixture Fixture;
		UEnemyDefinition* Enemy = Fixture.MakeThreePartEnemy(100, 100, 100, 5, 5, 5);
		UCardDefinition* Single = Fixture.MakeSimpleDamageCard(/*Cost*/5, /*Damage*/9);
		Single->PerfectReleaseEffects = {
			MakeCardEffect(WacomTags::Status_Shield, 1, WacomTags::Target_Player)
		};
		UBattleSession* Session = CreateSession(Fixture, { Single }, Enemy);
		const FBattleSnapshot Before = Session->BuildSnapshot();
		const FGuid BodyId = FWacomBattleFixture::FindPartInstanceId(Before, 1);
		const FBattleResolution Resolution = PlayCardOnPart(*Session, *Single, 1);
		const TArray<const FBattleEvent*> ResistanceEvents =
			FindEvents(Resolution, EBattleEventType::ResistanceResolved);
		TestEqual(TEXT("Single-target card compares only one actual damage target"),
			ResistanceEvents.Num(), 1);
		if (ResistanceEvents.Num() == 1)
		{
			TestEqual(TEXT("Single-target comparison belongs to selected body"),
				ResistanceEvents[0]->ActorInstanceId, BodyId);
		}
		TestEqual(TEXT("PerfectReleaseEffects retain the global initiative-hit scope"),
			FindEvents(Resolution, EBattleEventType::PerfectReleaseResolved).Num(), 3);
	}

	{
		FWacomBattleFixture Fixture;
		UEnemyDefinition* Enemy = Fixture.MakeThreePartEnemy(100, 100, 100, 5, 5, 5);
		SetPartIntentEffects(*Enemy, 0, { MakeIntentEffect(WacomTags::Effect_Damage, 6) });
		SetPartIntentEffects(*Enemy, 1, { MakeIntentEffect(WacomTags::Effect_Damage, 4) });
		SetPartIntentEffects(*Enemy, 2, { MakeIntentEffect(WacomTags::Status_Shield, 3, WacomTags::Target_Self) });

		UCardDefinition* Poison = Fixture.MakeSimpleDamageCard(/*Cost*/0, /*Damage*/0);
		Poison->Effects = { MakeCardEffect(WacomTags::Effect_ApplyStatus_Poison, 1) };
		UCardDefinition* All = Fixture.MakeSimpleDamageCard(/*Cost*/5, /*Damage*/0);
		All->TargetMode = ECardTargetMode::AllEnemyParts;
		FCardEffect PerTargetDamage =
			MakeCardEffect(WacomTags::Effect_Damage, 2, WacomTags::Target_AllEnemyParts);
		FMagnitudeModifier PoisonBonus;
		PoisonBonus.Condition.ConditionType = WacomTags::Condition_Target_HasStatus;
		PoisonBonus.Condition.ParamTag = WacomTags::Status_Poison;
		PoisonBonus.Op = EMagnitudeModOp::Add;
		PoisonBonus.Value = 4;
		PerTargetDamage.MagnitudeModifiers = { PoisonBonus };
		All->Effects = {
			PerTargetDamage,
			MakeCardEffect(WacomTags::Effect_Damage, 5, WacomTags::Target_AllEnemyParts),
		};

		UBattleSession* Session = CreateSession(Fixture, { Poison, All }, Enemy);
		TestTrue(TEXT("Poison only the body"), PlayCardOnPart(*Session, *Poison, 1).IsOk());
		const FBattleSnapshot Before = Session->BuildSnapshot();
		const FGuid HeadId = FWacomBattleFixture::FindPartInstanceId(Before, 0);
		const FGuid BodyId = FWacomBattleFixture::FindPartInstanceId(Before, 1);
		const FBattleResolution Resolution = PlayCardOnPart(*Session, *All, 0);
		const TArray<const FBattleEvent*> ResistanceEvents =
			FindEvents(Resolution, EBattleEventType::ResistanceResolved);
		TestEqual(TEXT("All-target card compares attack intents only"), ResistanceEvents.Num(), 2);
		if (ResistanceEvents.Num() == 2)
		{
			TestEqual(TEXT("Stable part order keeps head comparison first"),
				ResistanceEvents[0]->ActorInstanceId, HeadId);
			TestEqual(TEXT("Head uses peak segment 5 rather than summed damage 7"),
				ResistanceEvents[0]->Amount, 5);
			TestEqual(TEXT("Head attack peak is 6"), ResistanceEvents[0]->Count, 6);
			TestFalse(TEXT("Head comparison fails despite summed damage being greater"),
				ResistanceEvents[0]->Tag.IsValid());

			TestEqual(TEXT("Body comparison remains in its own target group"),
				ResistanceEvents[1]->ActorInstanceId, BodyId);
			TestEqual(TEXT("Body evaluates its poison modifier independently"),
				ResistanceEvents[1]->Amount, 6);
			TestEqual(TEXT("Body attack peak is 4"), ResistanceEvents[1]->Count, 4);
			TestTrue(TEXT("Body succeeds independently"),
				ResistanceEvents[1]->Tag == WacomTags::Status_Stunned);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleResistanceFinalMagnitudeSpec,
	"Wacom.Battle.Resistance.FinalMagnitudeSources",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleResistanceFinalMagnitudeSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleResistanceSpec;
	{
		FWacomBattleFixture Fixture;
		UCardDefinition* RuntimeCostCard = Fixture.MakeSimpleDamageCard(/*Cost*/5, /*Damage*/0);
		RuntimeCostCard->Effects[0].MagnitudeSource = WacomTags::Magnitude_Source_RuntimeCost;
		UBattleSession* Session = CreateSession(
			Fixture,
			{ RuntimeCostCard },
			Fixture.MakeSinglePartEnemyWithIntentDamage(100, 5, 4));
		const FBattleResolution Resolution = PlayCardOnPart(*Session, *RuntimeCostCard);
		const FBattleEvent* Resistance =
			FindFirstEvent(Resolution, EBattleEventType::ResistanceResolved);
		if (!TestNotNull(TEXT("RuntimeCost comparison exists"), Resistance))
		{
			return false;
		}
		TestEqual(TEXT("RuntimeCost is the resistance damage segment"), Resistance->Amount, 5);
	}

	{
		FWacomBattleFixture Fixture;
		UCardDefinition* Weapon = Fixture.MakeDamageCardWithKeywords(
			/*Cost*/5,
			/*Damage*/2,
			{ WacomTags::Card_Keyword_Weapon });
		TStrongObjectPtr<UBattleSession> Session = CreateCapacitySession(
			Fixture,
			Weapon,
			Fixture.MakeSinglePartEnemyWithIntentDamage(100, 5, 4));
		const FBattleResolution Resolution = PlayCardOnPart(*Session, *Weapon);
		const FBattleEvent* Resistance =
			FindFirstEvent(Resolution, EBattleEventType::ResistanceResolved);
		if (!TestNotNull(TEXT("Weapon capacity comparison exists"), Resistance))
		{
			return false;
		}
		TestEqual(TEXT("Weapon +3 contributes to resistance damage"), Resistance->Amount, 5);
		TestTrue(TEXT("Weapon +3 can make resistance succeed"),
			Resistance->Tag == WacomTags::Status_Stunned);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleResistanceStunLifecycleSpec,
	"Wacom.Battle.Resistance.StunStacksAndConsumesPerAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleResistanceStunLifecycleSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleResistanceSpec;
	FWacomBattleFixture Fixture;
	UCardDefinition* FirstResistanceCard =
		Fixture.MakeSimpleDamageCard(/*Cost*/5, /*Damage*/8);
	UCardDefinition* SecondResistanceCard =
		Fixture.MakeSimpleDamageCard(/*Cost*/5, /*Damage*/8);
	const TArray<FGameplayTag> HandZones = {
		FGameplayTag(WacomTags::HandZone_Left),
		FGameplayTag(WacomTags::HandZone_Both),
		FGameplayTag(WacomTags::HandZone_Right),
	};
	for (const FGameplayTag& Zone : HandZones)
	{
		for (UCardDefinition* Card : { FirstResistanceCard, SecondResistanceCard })
		{
			FCardZoneHook Hook;
			Hook.Zone = Zone;
			Hook.Trigger = WacomTags::ZoneHook_Trigger_OnPerfectReleaseHit;
			Card->ZoneHooks.Add(Hook);
		}
	}
	UCardDefinition* TriggerFirstAction = Fixture.MakeNoopCard(/*Cost*/5);
	UCardDefinition* TriggerSecondAction = Fixture.MakeNoopCard(/*Cost*/5);
	UBattleSession* Session = CreateSession(
		Fixture,
		{ FirstResistanceCard, SecondResistanceCard, TriggerFirstAction, TriggerSecondAction },
		Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/100, /*Initiative*/5, /*Damage*/4));

	const FBattleResolution FirstResistance =
		PlayCardOnPart(*Session, *FirstResistanceCard);
	TestTrue(TEXT("First resistance card resolves"), FirstResistance.IsOk());
	const FBattleEvent* FirstResistanceEvent =
		FindFirstEvent(FirstResistance, EBattleEventType::ResistanceResolved);
	if (!TestNotNull(TEXT("First resistance card compares"), FirstResistanceEvent))
	{
		return false;
	}
	TestEqual(TEXT("First resistance player peak"), FirstResistanceEvent->Amount, 8);
	TestEqual(TEXT("First resistance enemy peak"), FirstResistanceEvent->Count, 4);
	TestTrue(TEXT("First resistance card succeeds"),
		FirstResistanceEvent->Tag == WacomTags::Status_Stunned);
	TestEqual(TEXT("Skip-push resistance does not reach action boundary"),
		FindEvents(FirstResistance, EBattleEventType::EnemyPartActed).Num(), 0);
	FBattleSnapshot CurrentSnapshot = Session->BuildSnapshot();
	const FEnemyPartSnapshot* Part =
		FWacomBattleFixture::GetEnemyPartSnapshot(CurrentSnapshot, 0);
	TestEqual(TEXT("First success persists one stun layer"),
		Part ? FWacomBattleFixture::GetStatusStacks(Part->StatusStacks, WacomTags::Status_Stunned) : -1,
		1);

	const FBattleResolution SecondResistance =
		PlayCardOnPart(*Session, *SecondResistanceCard);
	TestTrue(TEXT("Second resistance card resolves"), SecondResistance.IsOk());
	const FBattleEvent* SecondResistanceEvent =
		FindFirstEvent(SecondResistance, EBattleEventType::ResistanceResolved);
	if (!TestNotNull(TEXT("Second resistance card compares"), SecondResistanceEvent))
	{
		return false;
	}
	TestTrue(TEXT("Second resistance card succeeds"),
		SecondResistanceEvent->Tag == WacomTags::Status_Stunned);
	TestEqual(TEXT("Second skip-push resistance still does not act"),
		FindEvents(SecondResistance, EBattleEventType::EnemyPartActed).Num(), 0);
	CurrentSnapshot = Session->BuildSnapshot();
	Part = FWacomBattleFixture::GetEnemyPartSnapshot(CurrentSnapshot, 0);
	TestEqual(TEXT("Repeated success stacks stun"),
		Part ? FWacomBattleFixture::GetStatusStacks(Part->StatusStacks, WacomTags::Status_Stunned) : -1,
		2);

	const FBattleResolution FirstAction = PlayCardOnPart(*Session, *TriggerFirstAction);
	const FBattleEvent* FirstActed = FindFirstEvent(FirstAction, EBattleEventType::EnemyPartActed);
	TestTrue(TEXT("First action attempt is skipped"), FirstActed && FirstActed->Count == 0);
	CurrentSnapshot = Session->BuildSnapshot();
	Part = FWacomBattleFixture::GetEnemyPartSnapshot(CurrentSnapshot, 0);
	TestEqual(TEXT("First skipped action consumes one layer"),
		Part ? FWacomBattleFixture::GetStatusStacks(Part->StatusStacks, WacomTags::Status_Stunned) : -1,
		1);

	const FBattleResolution SecondAction = PlayCardOnPart(*Session, *TriggerSecondAction);
	const FBattleEvent* SecondActed = FindFirstEvent(SecondAction, EBattleEventType::EnemyPartActed);
	TestTrue(TEXT("Second action attempt is skipped"), SecondActed && SecondActed->Count == 0);
	CurrentSnapshot = Session->BuildSnapshot();
	Part = FWacomBattleFixture::GetEnemyPartSnapshot(CurrentSnapshot, 0);
	TestEqual(TEXT("Second skipped action consumes the final layer"),
		Part ? FWacomBattleFixture::GetStatusStacks(Part->StatusStacks, WacomTags::Status_Stunned) : -1,
		0);
	TestEqual(TEXT("Skipped actions deal no player damage"),
		Session->BuildSnapshot().Player.CurrentHp, 100);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleResistanceActionPreviewSpec,
	"Wacom.Battle.Resistance.ActionPreviewParity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleResistanceActionPreviewSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleResistanceSpec;
	for (const int32 PlayerDamage : { 6, 5 })
	{
		FWacomBattleFixture Fixture;
		UCardDefinition* Card = Fixture.MakeSimpleDamageCard(/*Cost*/5, PlayerDamage);
		UBattleSession* Session = CreateSession(
			Fixture,
			{ Card },
			Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/100, /*Initiative*/5, /*Damage*/5));
		const FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const FGuid CardId =
			FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, Card->CardId);
		const FEnemyPartSnapshot* Target = FWacomBattleFixture::GetEnemyPartSnapshot(Snapshot, 0);
		if (!TestTrue(TEXT("Preview target exists"), CardId.IsValid() && Target))
		{
			return false;
		}
		const FBattleCardActionPreview Preview = Session->BuildCardActionPreview(
			CardId,
			FWacomInteractionTargetHandle::ForWorldTarget(
				Target->InstanceId,
				nullptr,
				FVector::ZeroVector,
				FVector2D::ZeroVector,
				FGameplayTag(),
				NAME_None,
				Target->EncounterId,
				Target->EnemySlotId,
				Target->PartSlotId));
		TestTrue(TEXT("Action preview exists"), Preview.bHasPreview);
		if (!TestEqual(TEXT("Eligible comparison is always exposed"),
			Preview.ResistancePreviews.Num(), 1))
		{
			return false;
		}
		const FBattleCardResistancePreview& Resistance = Preview.ResistancePreviews[0];
		TestEqual(TEXT("Preview player peak"), Resistance.PlayerPeakSingleHitDamage, PlayerDamage);
		TestEqual(TEXT("Preview enemy peak"), Resistance.EnemyPeakSingleHitDamage, 5);
		TestEqual(TEXT("Preview strict result"), Resistance.bWillStun, PlayerDamage > 5);

		const FBattleCardActionPreviewEnemyPartState* ProjectedPart =
			Preview.ProjectedEnemyParts.FindByPredicate(
				[Target](const FBattleCardActionPreviewEnemyPartState& Candidate)
				{
					return Candidate.Snapshot.InstanceId == Target->InstanceId;
				});
		if (!TestNotNull(TEXT("Compared part remains visible in action preview"), ProjectedPart))
		{
			return false;
		}
		TestEqual(TEXT("Successful resistance is a skip, not an attack risk"),
			ProjectedPart->bWillSkipActionDueToStun, PlayerDamage > 5);
		TestEqual(TEXT("Failed resistance still marks the real enemy action"),
			ProjectedPart->bWillAct, PlayerDamage <= 5);

		const FBattleResolution Resolution = Session->ResolveCommand(
			FWacomBattleFixture::MakePlayCardOnPart(Snapshot, CardId, 0));
		const FBattleEvent* Resolved =
			FindFirstEvent(Resolution, EBattleEventType::ResistanceResolved);
		if (!TestNotNull(TEXT("Formal resolution emits comparison"), Resolved))
		{
			return false;
		}
		TestEqual(TEXT("Preview player peak matches resolution"),
			Resistance.PlayerPeakSingleHitDamage, Resolved->Amount);
		TestEqual(TEXT("Preview enemy peak matches resolution"),
			Resistance.EnemyPeakSingleHitDamage, Resolved->Count);
		TestEqual(TEXT("Preview result matches resolution"),
			Resistance.bWillStun, Resolved->Tag == WacomTags::Status_Stunned);
	}
	return true;
}
