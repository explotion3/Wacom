// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "BattleHUDTestHarness.h"
#include "Cards/CardDefinition.h"
#include "Engine/Engine.h"
#include "Events/BattleEvent.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Presentation/BattlePresentationJournal.h"
#include "Session/BattleInitializationResult.h"
#include "Session/BattleResolution.h"
#include "Session/BattleSession.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"
#include "UI/BattleWidgetSpecReceiver.h"

#if WITH_AUTOMATION_TESTS

namespace WacomBattleCommandHandFeedbackPresentationSpec
{
	UWorld* FindAutomationWorld()
	{
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (UWorld* World = Context.World())
				{
					return World;
				}
			}
		}
		return GWorld;
	}

	FWacomInitializedBattleSession CreateInitializedPlayerActionSession(
		FWacomBattleFixture& Fixture)
	{
		UCharacterDefinition* Character = Fixture.MakeCharacter(
			Fixture.MakeNoopCard(0),
			Fixture.MakeNoopCard(0),
			{
				Fixture.MakeNoopCard(0),
				Fixture.MakeNoopCard(0),
				Fixture.MakeNoopCard(0),
				Fixture.MakeNoopCard(0)
			});
		return Fixture.CreateInitializedSession(
			Character,
			Fixture.MakeSinglePartEnemy(20, 50, 0),
			31);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCommandDeckPlanPreservesCardUseReformFeedbackTest,
	"Wacom.UI.Battle.PresentationPlan.CommandDeckStepsPreserveCardUseReformFeedback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCommandDeckPlanPreservesCardUseReformFeedbackTest::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleCommandHandFeedbackPresentationSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	const FWacomInitializedBattleSession Initialized =
		CreateInitializedPlayerActionSession(Fixture);
	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	if (!TestNotNull(TEXT("Initialized session"), Initialized.Session)
		|| !TestNotNull(TEXT("HUD harness"), Harness.Get()))
	{
		return false;
	}

	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	Harness->SetInitializedSession(Initialized);
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}

	const FBattleSnapshot PreSnapshot = Initialized.Session->BuildSnapshot();
	const FHandCardSnapshot* PlayedCard = PreSnapshot.Hand.Cards.FindByPredicate(
		[](const FHandCardSnapshot& Card)
		{
			return Card.InstanceId.IsValid() && !Card.bIsHandAnchor;
		});
	if (!TestNotNull(TEXT("A normal hand card is available"), PlayedCard))
	{
		return false;
	}

	const FGuid PlayedCardId = PlayedCard->InstanceId;
	const FGuid DrawnCardId = FGuid::NewGuid();
	FBattleSnapshot PostSnapshot = PreSnapshot;
	PostSnapshot.Version = PreSnapshot.Version + 1;
	FHandCardSnapshot DrawnCard = *PlayedCard;
	DrawnCard.InstanceId = DrawnCardId;
	DrawnCard.bIsHandAnchor = false;
	PostSnapshot.Hand.Cards.Add(DrawnCard);
	++PostSnapshot.Hand.NormalCardCount;

	FBattleEvent CardPlayed;
	CardPlayed.Type = EBattleEventType::CardPlayed;
	CardPlayed.Sequence = 10;
	CardPlayed.CardInstanceId = PlayedCardId;

	FBattleEvent DestinationResolved;
	DestinationResolved.Type = EBattleEventType::CardPlayDestinationResolved;
	DestinationResolved.Sequence = 11;
	DestinationResolved.CardInstanceId = PlayedCardId;
	DestinationResolved.CardDestination = ECardLocation::Hand;

	FBattleEvent CardsDrawn;
	CardsDrawn.Type = EBattleEventType::CardsDrawn;
	CardsDrawn.Sequence = 12;
	CardsDrawn.CardInstanceIds = { DrawnCardId };
	CardsDrawn.Count = 1;
	CardsDrawn.DrawPileCountAfter = PostSnapshot.PileCounts.DrawCount;
	CardsDrawn.DiscardPileCountAfter = PostSnapshot.PileCounts.DiscardCount;

	FBattleResolution Resolution;
	Resolution.Status = FWacomStatus::Ok();
	Resolution.VersionBefore = PreSnapshot.Version;
	Resolution.VersionAfter = PostSnapshot.Version;
	Resolution.Events = { CardPlayed, DestinationResolved, CardsDrawn };
	Resolution.PresentationJournal.AppendDeckStepsFromEvents(Resolution.Events);
	Resolution.PostSnapshot = PostSnapshot;

	HUD->RecordFirstPersonPlayCommitForTest(
		PlayedCardId,
		FBattlePartSlotIdentity(),
		TOptional<FVector2D>());
	const FWacomBattleCombatLogCommandContext LogContext =
		UWacomBattleCombatLogBuilder::BuildPlayCardCommandContext(
			PreSnapshot,
			PlayedCardId,
			FBattlePartSlotIdentity(),
			FGuid());
	HUD->ApplyCommandResolutionForTest(
		LogContext,
		PreSnapshot,
		Resolution,
		Initialized.Session);

	const TArray<FName> StartedPhases =
		HUD->GetStartedPresentationPlanPhaseNamesForTest();
	TestTrue(
		TEXT("Deck-step command enters CommandHandResolution"),
		StartedPhases.Contains(FName(TEXT("CommandHandResolution"))));
	const TArray<FWacomFirstPersonCardLayerFeedbackHint> FeedbackHints =
		HUD->GetSubmittedPresentationPlanFeedbackHintsForTest();
	const FWacomFirstPersonCardLayerFeedbackHint* ReformHint = FeedbackHints.FindByPredicate(
		[&PlayedCardId](const FWacomFirstPersonCardLayerFeedbackHint& Hint)
		{
			return Hint.CardInstanceId == PlayedCardId
				&& Hint.FeedbackKind ==
					EWacomFirstPersonCardLayerFeedbackKind::CardUseReform;
		});
	TestNotNull(
		TEXT("Command hand phase preserves CardUseReform while deck steps are deferred"),
		ReformHint);
	return true;
}

#endif
