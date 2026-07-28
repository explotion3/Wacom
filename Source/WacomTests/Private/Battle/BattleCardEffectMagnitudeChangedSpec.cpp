// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Commands/BattleCommand.h"
#include "Events/BattleEvent.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Session/BattleResolution.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleCardRuntimeSnapshot.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"

#if WITH_AUTOMATION_TESTS

namespace WacomBattleCardEffectMagnitudeChangedSpec
{
	UCardDefinition* MakeMagnitudeModifierCard(
		FWacomBattleFixture& Fixture,
		const FGameplayTag& MutationEffect,
		int32 Magnitude)
	{
		UCardDefinition* Card = Fixture.MakeNoopCard(0);
		Card->TargetMode = ECardTargetMode::HandCard;
		Card->HandCardTargetFilter.bUseExplicitHandCardTargetFilter = true;
		Card->HandCardTargetFilter.bAllowNormalHandCards = true;
		Card->HandCardTargetFilter.bAllowHandAnchors = false;

		FCardEffect Effect;
		Effect.EffectType = MutationEffect;
		Effect.Target = WacomTags::Target_SelectedHandCard;
		Effect.AffectedEffectType = WacomTags::Effect_ApplyStatus_Burn;
		Effect.Magnitude = Magnitude;
		Card->Effects.Add(Effect);
		return Card;
	}

	UCardDefinition* MakeBurnCard(FWacomBattleFixture& Fixture)
	{
		UCardDefinition* Card = Fixture.MakeNoopCard(1);
		FCardEffect Burn;
		Burn.EffectType = WacomTags::Effect_ApplyStatus_Burn;
		Burn.Target = WacomTags::Target_AllEnemyParts;
		Burn.Magnitude = 2;
		Card->Effects.Add(Burn);
		return Card;
	}

	bool RunMutationCase(
		FAutomationTestBase& Test,
		const FGameplayTag& MutationEffect,
		int32 MutationMagnitude,
		EBattleCardEffectMagnitudeMutationKind ExpectedKind,
		int32 ExpectedResolvedBurn)
	{
		FWacomBattleFixture Fixture;
		UCardDefinition* Source =
			MakeMagnitudeModifierCard(Fixture, MutationEffect, MutationMagnitude);
		UCardDefinition* Target = MakeBurnCard(Fixture);
		UBattleSession* Session = Fixture.CreateSession(
			Fixture.MakeCharacter(
				Fixture.MakeNoopCard(0),
				Fixture.MakeNoopCard(0),
				{ Source, Target, Fixture.MakeNoopCard(0),
					Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0) }),
			Fixture.MakeSinglePartEnemy(100, 50),
			67);
		if (!Test.TestNotNull(TEXT("Magnitude mutation session"), Session))
		{
			return false;
		}

		const FBattleSnapshot Before = Session->BuildSnapshot();
		const FGuid SourceId =
			FWacomBattleFixture::FindHandInstanceByCardId(Before, Source->CardId);
		const FGuid TargetId =
			FWacomBattleFixture::FindHandInstanceByCardId(Before, Target->CardId);
		const FBattleResolution Resolution = Session->ResolveCommand(
			FBattleCommand::MakePlayCardOnHandCard(SourceId, TargetId));
		if (!Test.TestTrue(
			TEXT("Magnitude mutation command succeeds"),
			Resolution.IsOk()))
		{
			return false;
		}

		const FBattleEvent* Event = Resolution.Events.FindByPredicate(
			[&TargetId](const FBattleEvent& Candidate)
			{
				return Candidate.Type ==
						EBattleEventType::CardEffectMagnitudeChanged
					&& Candidate.CardInstanceId == TargetId;
			});
		if (!Test.TestNotNull(
			TEXT("Target card publishes a typed magnitude event"),
			Event))
		{
			return false;
		}
		Test.TestEqual(
			TEXT("Event keeps source identity"),
			Event->ActorInstanceId,
			SourceId);
		Test.TestEqual(
			TEXT("Event exposes the affected effect"),
			Event->CardEffectMagnitudeChange.AffectedEffectType,
			FGameplayTag(WacomTags::Effect_ApplyStatus_Burn));
		Test.TestEqual(
			TEXT("Event exposes the mutation effect"),
			Event->CardEffectMagnitudeChange.SourceEffectType,
			MutationEffect);
		Test.TestEqual(
			TEXT("Event exposes the mutation kind"),
			Event->CardEffectMagnitudeChange.MutationKind,
			ExpectedKind);
		if (ExpectedKind ==
			EBattleCardEffectMagnitudeMutationKind::AdditiveBonus)
		{
			Test.TestEqual(
				TEXT("Additive mutation records its previous bonus"),
				Event->CardEffectMagnitudeChange.AdditiveBonusBefore,
				0);
			Test.TestEqual(
				TEXT("Additive mutation records its committed bonus"),
				Event->CardEffectMagnitudeChange.AdditiveBonusAfter,
				MutationMagnitude);
			Test.TestTrue(
				TEXT("Additive mutation preserves the multiplier"),
				FMath::IsNearlyEqual(
					Event->CardEffectMagnitudeChange.MultiplierBefore,
					1.0f)
				&& FMath::IsNearlyEqual(
					Event->CardEffectMagnitudeChange.MultiplierAfter,
					1.0f));
		}
		else
		{
			Test.TestEqual(
				TEXT("Multiplier mutation preserves the additive bonus"),
				Event->CardEffectMagnitudeChange.AdditiveBonusBefore,
				Event->CardEffectMagnitudeChange.AdditiveBonusAfter);
			Test.TestTrue(
				TEXT("Multiplier mutation records the committed factor"),
				FMath::IsNearlyEqual(
					Event->CardEffectMagnitudeChange.MultiplierBefore,
					1.0f)
				&& FMath::IsNearlyEqual(
					Event->CardEffectMagnitudeChange.MultiplierAfter,
					static_cast<float>(MutationMagnitude)));
		}

		const FHandCardSnapshot* TargetAfter =
			Resolution.PostSnapshot.Hand.Cards.FindByPredicate(
				[&TargetId](const FHandCardSnapshot& Card)
				{
					return Card.InstanceId == TargetId;
				});
		if (!Test.TestNotNull(
			TEXT("Target remains visible after source play"),
			TargetAfter))
		{
			return false;
		}
		const FBattleCardEffectMagnitudeSnapshot* BurnAfter =
			TargetAfter->CurrentEffectMagnitudes.FindByPredicate(
				[](const FBattleCardEffectMagnitudeSnapshot& Effect)
				{
					return Effect.EffectType ==
						WacomTags::Effect_ApplyStatus_Burn;
				});
		if (!Test.TestNotNull(
			TEXT("Target snapshot exposes current Burn"),
			BurnAfter))
		{
			return false;
		}
		Test.TestEqual(
			TEXT("Snapshot and event describe the same committed result"),
			BurnAfter->Magnitude,
			ExpectedResolvedBurn);
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCardEffectMagnitudeChangedEventTest,
	"Wacom.Battle.CardEffectMagnitudeChanged.EventContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCardEffectMagnitudeChangedEventTest::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleCardEffectMagnitudeChangedSpec;
	const bool bAddPassed = RunMutationCase(
		*this,
		WacomTags::Effect_Card_AddEffectMagnitude,
		3,
		EBattleCardEffectMagnitudeMutationKind::AdditiveBonus,
		5);
	const bool bMultiplyPassed = RunMutationCase(
		*this,
		WacomTags::Effect_Card_MultiplyEffectMagnitude,
		2,
		EBattleCardEffectMagnitudeMutationKind::Multiplier,
		4);
	return bAddPassed && bMultiplyPassed;
}

#endif
