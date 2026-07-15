// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "BattleHUDTestHarness.h"
#include "Cards/CardDefinition.h"
#include "Engine/Engine.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Session/BattleResolution.h"
#include "Session/BattleSession.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"
#include "UI/BattleWidgetSpecReceiver.h"

#if WITH_AUTOMATION_TESTS

namespace WacomBattleHandTargetPresentationSpec
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

	FWacomInitializedBattleSession CreateSession(FWacomBattleFixture& Fixture)
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
			37);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleHandTargetImpactSharesDiscardPhaseTest,
	"Wacom.UI.Battle.HandTargetPresentation.SuccessfulTargetImpactSharesDiscardPhase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleHandTargetImpactSharesDiscardPhaseTest::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleHandTargetPresentationSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	const FWacomInitializedBattleSession Initialized = CreateSession(Fixture);
	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	if (!TestNotNull(TEXT("Initialized session"), Initialized.Session)
		|| !TestNotNull(TEXT("HUD harness"), Harness.Get()))
	{
		return false;
	}
	Harness->SetInitializedSession(Initialized);
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}

	const FBattleSnapshot PreSnapshot = Initialized.Session->BuildSnapshot();
	TArray<const FHandCardSnapshot*> NormalCards;
	for (const FHandCardSnapshot& Card : PreSnapshot.Hand.Cards)
	{
		if (Card.InstanceId.IsValid() && !Card.bIsHandAnchor)
		{
			NormalCards.Add(&Card);
		}
	}
	if (!TestTrue(TEXT("Source and target hand cards are available"), NormalCards.Num() >= 2))
	{
		return false;
	}
	const FGuid SourceCardId = NormalCards[0]->InstanceId;
	const FGuid TargetCardId = NormalCards[1]->InstanceId;

	FBattleSnapshot PostSnapshot = PreSnapshot;
	PostSnapshot.Version = PreSnapshot.Version + 1;
	PostSnapshot.Hand.Cards.RemoveAll(
		[&SourceCardId, &TargetCardId](const FHandCardSnapshot& Card)
		{
			return Card.InstanceId == SourceCardId || Card.InstanceId == TargetCardId;
		});
	PostSnapshot.Hand.NormalCardCount = FMath::Max(0, PreSnapshot.Hand.NormalCardCount - 2);
	PostSnapshot.PileCounts.DiscardCount = PreSnapshot.PileCounts.DiscardCount + 2;

	FBattleEvent CardPlayed;
	CardPlayed.Type = EBattleEventType::CardPlayed;
	CardPlayed.Sequence = 10;
	CardPlayed.CardInstanceId = SourceCardId;
	FBattleEvent DestinationResolved;
	DestinationResolved.Type = EBattleEventType::CardPlayDestinationResolved;
	DestinationResolved.Sequence = 11;
	DestinationResolved.CardInstanceId = SourceCardId;
	DestinationResolved.CardDestination = ECardLocation::Discard;
	FBattleEvent TargetDiscarded;
	TargetDiscarded.Type = EBattleEventType::CardDiscarded;
	TargetDiscarded.Sequence = 12;
	TargetDiscarded.HandCardZoneMoveBatchSequence = 12;
	TargetDiscarded.CardInstanceId = TargetCardId;
	TargetDiscarded.CardInstanceIds = { TargetCardId };
	TargetDiscarded.DiscardPileCountAfter = PreSnapshot.PileCounts.DiscardCount + 1;

	FBattleResolution Resolution;
	Resolution.Status = FWacomStatus::Ok();
	Resolution.VersionBefore = PreSnapshot.Version;
	Resolution.VersionAfter = PostSnapshot.Version;
	Resolution.Events = { CardPlayed, DestinationResolved, TargetDiscarded };
	Resolution.PostSnapshot = PostSnapshot;

	FWacomBattleCombatLogCommandContext LogContext =
		UWacomBattleCombatLogBuilder::BuildPlayCardCommandContext(
			PreSnapshot,
			SourceCardId,
			FBattlePartSlotIdentity(),
			TargetCardId);
	LogContext.CardTargetPreview.bHasPreview = true;
	LogContext.CardTargetPreview.TargetKind = EWacomBattleCardPreviewTargetKind::HandCard;
	LogContext.CardTargetPreview.SourceCardInstanceId = SourceCardId;
	LogContext.CardTargetPreview.TargetHandCardInstanceId = TargetCardId;

	HUD->ApplyCommandResolutionForTest(
		LogContext,
		PreSnapshot,
		Resolution,
		Initialized.Session,
		SourceCardId);

	const TArray<FWacomFirstPersonCardLayerFeedbackHint> FeedbackHints =
		HUD->GetSubmittedPresentationPlanFeedbackHintsForTest();
	const FWacomFirstPersonCardLayerFeedbackHint* ImpactHint =
		FeedbackHints.FindByPredicate(
			[&TargetCardId](const FWacomFirstPersonCardLayerFeedbackHint& Hint)
			{
				return Hint.CardInstanceId == TargetCardId
					&& Hint.FeedbackKind
						== EWacomFirstPersonCardLayerFeedbackKind::HandTargetImpact;
			});
	TestNotNull(TEXT("Successful hand target emits one impact hint"), ImpactHint);
	const TArray<FName> StartedPhases =
		HUD->GetStartedPresentationPlanPhaseNamesForTest();
	TestTrue(
		TEXT("Target impact owns a dedicated target phase"),
		StartedPhases.Contains(FName(TEXT("CommandPrimaryTarget"))));
	const int32 SourceIndex = StartedPhases.IndexOfByKey(FName(TEXT("CommandSourceOut")));
	const int32 TargetIndex = StartedPhases.IndexOfByKey(FName(TEXT("CommandPrimaryTarget")));
	const int32 DiscardIndex = StartedPhases.IndexOfByKey(FName(TEXT("HandDiscardGlyphTransfer")));
	TestTrue(TEXT("Source plays before target"), SourceIndex != INDEX_NONE && SourceIndex < TargetIndex);
	TestTrue(TEXT("Target plays before discard result"), TargetIndex < DiscardIndex);
	return true;
}

#endif
