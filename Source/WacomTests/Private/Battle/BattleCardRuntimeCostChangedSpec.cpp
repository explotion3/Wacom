// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Commands/BattleCommand.h"
#include "Events/BattleEvent.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Session/BattleResolution.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"

#if WITH_AUTOMATION_TESTS

namespace WacomBattleCardRuntimeCostChangedSpec
{
	const FBattleEvent* FindRuntimeCostEvent(
		const FBattleResolution& Resolution,
		const FGuid& TargetCardInstanceId)
	{
		return Resolution.Events.FindByPredicate(
			[&TargetCardInstanceId](const FBattleEvent& Event)
			{
				return Event.Type == EBattleEventType::CardRuntimeCostChanged
					&& Event.CardInstanceId == TargetCardInstanceId;
			});
	}

	bool RunCostCase(
		FAutomationTestBase& Test,
		bool bReduceCost,
		int32 Magnitude,
		int32 ExpectedDelta,
		int32 ExpectedRuntimeCost,
		const FGameplayTag& ExpectedEffectTag)
	{
		FWacomBattleFixture Fixture;
		UCardDefinition* SourceCard = Fixture.MakeHandCardCostModifierCard(
			/*Cost*/ 0,
			Magnitude,
			bReduceCost);
		UCardDefinition* TargetCard = Fixture.MakeNoopCard(/*Cost*/ 3);
		UBattleSession* Session = Fixture.CreateSession(
			Fixture.MakeCharacter(
				Fixture.MakeNoopCard(0),
				Fixture.MakeNoopCard(0),
				{ SourceCard, TargetCard, Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0) }),
			Fixture.MakeSinglePartEnemy(/*Hp*/ 100, /*Initiative*/ 50),
			19);
		if (!Test.TestNotNull(TEXT("Runtime-cost session"), Session))
		{
			return false;
		}

		const FBattleSnapshot Snapshot = Session->BuildSnapshot();
		const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(
			Snapshot,
			SourceCard->CardId);
		const FGuid TargetId = FWacomBattleFixture::FindHandInstanceByCardId(
			Snapshot,
			TargetCard->CardId);
		const FBattleResolution Resolution = Session->ResolveCommand(
			FBattleCommand::MakePlayCardOnHandCard(SourceId, TargetId));
		if (!Test.TestTrue(TEXT("Cost modifier command succeeds"), Resolution.IsOk()))
		{
			return false;
		}

		const FBattleEvent* Event = FindRuntimeCostEvent(Resolution, TargetId);
		if (!Test.TestNotNull(TEXT("Runtime-cost change emits an explicit event"), Event))
		{
			return false;
		}
		Test.TestEqual(TEXT("Event keeps the target card identity"), Event->CardInstanceId, TargetId);
		Test.TestEqual(TEXT("Event keeps the source card identity"), Event->ActorInstanceId, SourceId);
		Test.TestEqual(TEXT("Event keeps the raw modifier delta"), Event->Amount, ExpectedDelta);
		Test.TestEqual(TEXT("Event stores the clamped effective cost"), Event->Count, ExpectedRuntimeCost);
		Test.TestEqual(TEXT("Event keeps the source effect tag"), Event->Tag, ExpectedEffectTag);
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCardRuntimeCostChangedEventTest,
	"Wacom.Battle.CardRuntimeCostChanged.EventContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCardRuntimeCostChangedEventTest::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleCardRuntimeCostChangedSpec;
	const bool bAddPassed = RunCostCase(
		*this,
		/*bReduceCost*/ false,
		/*Magnitude*/ 2,
		/*ExpectedDelta*/ 2,
		/*ExpectedRuntimeCost*/ 5,
		WacomTags::Effect_Card_AddCost);
	const bool bReducePassed = RunCostCase(
		*this,
		/*bReduceCost*/ true,
		/*Magnitude*/ 5,
		/*ExpectedDelta*/ -5,
		/*ExpectedRuntimeCost*/ 0,
		WacomTags::Effect_Card_ReduceCost);
	return bAddPassed && bReducePassed;
}

#endif
