// Copyright Wacom. All Rights Reserved.

#include "Fixtures/BattleTestFixtures.h"
#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Commands/BattleCommand.h"
#include "Events/BattleEvent.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"

namespace WacomBattleCardPlayedDestinationSpec
{
	const FBattleEvent* FindEvent(
		const TArray<FBattleEvent>& Events,
		EBattleEventType Type,
		const FGuid& CardId)
	{
		return Events.FindByPredicate(
			[Type, &CardId](const FBattleEvent& Event)
			{
				return Event.Type == Type && Event.CardInstanceId == CardId;
			});
	}

	UCardDefinition* MakeCard(
		FWacomBattleFixture& Fixture,
		const TCHAR* CardId)
	{
		UCardDefinition* Card = Fixture.MakeNoopCard(0);
		Card->CardId = CardId;
		return Card;
	}

	void AddEffect(UCardDefinition& Card, const FGameplayTag& EffectType)
	{
		FCardEffect Effect;
		Effect.EffectType = EffectType;
		Effect.Target = WacomTags::Target_Self;
		Card.Effects.Add(Effect);
	}

	void TestDestination(
		FAutomationTestBase& Test,
		UBattleSession& Session,
		const FGuid& CardId,
		ECardLocation ExpectedDestination)
	{
		const FBattleResolution Resolution =
			Session.ResolveCommand(FBattleCommand::MakePlayCard(CardId));
		if (!Test.TestTrue(TEXT("Play command succeeds"), Resolution.IsOk()))
		{
			return;
		}

		const FBattleEvent* PlayedEvent = FindEvent(
			Resolution.Events,
			EBattleEventType::CardPlayed,
			CardId);
		const FBattleEvent* DestinationEvent = FindEvent(
			Resolution.Events,
			EBattleEventType::CardPlayDestinationResolved,
			CardId);
		if (Test.TestNotNull(TEXT("CardPlayed event exists"), PlayedEvent)
			&& Test.TestNotNull(TEXT("Destination event exists"), DestinationEvent))
		{
			Test.TestTrue(
				TEXT("CardPlayed remains before destination resolution"),
				PlayedEvent->Sequence < DestinationEvent->Sequence);
			Test.TestEqual(
				TEXT("Destination event reports final zone"),
				DestinationEvent->CardDestination,
				ExpectedDestination);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCardPlayedDestinationEventTest,
	"Wacom.Battle.CardZoneTransition.PlayedDestinationPublishesFinalZone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCardPlayedDestinationEventTest::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleCardPlayedDestinationSpec;
	FWacomBattleFixture Fixture;
	UCardDefinition* LeftAnchor = MakeCard(Fixture, TEXT("PlayedDestination.LeftAnchor"));
	UCardDefinition* RightAnchor = MakeCard(Fixture, TEXT("PlayedDestination.RightAnchor"));
	UCardDefinition* Normal = MakeCard(Fixture, TEXT("PlayedDestination.Normal"));
	UCardDefinition* ExhaustSelf = MakeCard(Fixture, TEXT("PlayedDestination.ExhaustSelf"));
	AddEffect(*ExhaustSelf, WacomTags::Effect_ExhaustSelf);
	UCardDefinition* Combo = MakeCard(Fixture, TEXT("PlayedDestination.Combo"));
	Combo->Keywords.AddTag(WacomTags::Card_Keyword_Combo);
	UCardDefinition* ExplicitShuffle = MakeCard(Fixture, TEXT("PlayedDestination.ExplicitShuffle"));
	AddEffect(*ExplicitShuffle, WacomTags::Effect_Shuffle_ToRandomZone);
	UCardDefinition* Filler = MakeCard(Fixture, TEXT("PlayedDestination.Filler"));

	UBattleSession* Session = Fixture.CreateSession(
		Fixture.MakeCharacter(
			LeftAnchor,
			RightAnchor,
			{ Normal, ExhaustSelf, Combo, ExplicitShuffle, Filler }),
		Fixture.MakeSinglePartEnemy(/*Hp*/100, /*Initiative*/50, /*IntentResist*/0),
		/*Seed*/37);
	const FBattleSnapshot Initial = Session->BuildSnapshot();
	const FGuid NormalId = FWacomBattleFixture::FindHandInstanceByCardId(Initial, Normal->CardId);
	const FGuid ExhaustId = FWacomBattleFixture::FindHandInstanceByCardId(Initial, ExhaustSelf->CardId);
	const FGuid ComboId = FWacomBattleFixture::FindHandInstanceByCardId(Initial, Combo->CardId);
	const FGuid ShuffleId = FWacomBattleFixture::FindHandInstanceByCardId(Initial, ExplicitShuffle->CardId);
	const FGuid AnchorId = FWacomBattleFixture::FindHandInstanceByCardId(Initial, LeftAnchor->CardId);
	if (!TestTrue(
		TEXT("Destination fixture contains every source card"),
		NormalId.IsValid()
			&& ExhaustId.IsValid()
			&& ComboId.IsValid()
			&& ShuffleId.IsValid()
			&& AnchorId.IsValid()))
	{
		return false;
	}

	TestDestination(*this, *Session, NormalId, ECardLocation::Played);
	TestDestination(*this, *Session, ExhaustId, ECardLocation::Exhaust);
	TestDestination(*this, *Session, ComboId, ECardLocation::Hand);
	TestDestination(*this, *Session, ShuffleId, ECardLocation::Hand);
	TestDestination(*this, *Session, AnchorId, ECardLocation::Limbo);
	return true;
}
