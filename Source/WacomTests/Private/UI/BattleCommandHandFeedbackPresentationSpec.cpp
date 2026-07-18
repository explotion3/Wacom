// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "BattleHUDTestHarness.h"
#include "Cards/CardDefinition.h"
#include "Commands/BattleCommand.h"
#include "Engine/Engine.h"
#include "Events/BattleEvent.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Presentation/BattlePresentationJournal.h"
#include "Session/BattleInitializationResult.h"
#include "Session/BattleResolution.h"
#include "Session/BattleSession.h"
#include "Tags/WacomGameplayTags.h"
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
		Initialized.Session,
		PlayedCardId);

	const TArray<FName> StartedPhases =
		HUD->GetStartedPresentationPlanPhaseNamesForTest();
	TestTrue(
		TEXT("PlayCard command starts with source-out"),
		StartedPhases.Contains(FName(TEXT("CommandSourceOut"))));
	TestTrue(
		TEXT("Retained source reforms only after deck outcomes"),
		StartedPhases.Contains(FName(TEXT("CommandSourceReturn"))));
	const TArray<FWacomFirstPersonCardLayerFeedbackHint> FeedbackHints =
		HUD->GetSubmittedPresentationPlanFeedbackHintsForTest();
	const FWacomFirstPersonCardLayerFeedbackHint* ReformHint = FeedbackHints.FindByPredicate(
		[&PlayedCardId](const FWacomFirstPersonCardLayerFeedbackHint& Hint)
		{
			return Hint.CardInstanceId == PlayedCardId
				&& Hint.FeedbackKind ==
					EWacomFirstPersonCardLayerFeedbackKind::CardUseReformOut;
		});
	TestNotNull(
		TEXT("Source-out phase converts CardUseReform into its outbound half"),
		ReformHint);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCommandCostRewritePhaseOrderTest,
	"Wacom.UI.Battle.CommandPresentation.HandTargetCostRewritePhaseOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCommandCostRewritePhaseOrderTest::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleCommandHandFeedbackPresentationSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UCardDefinition* SourceDefinition = Fixture.MakeHandCardCostModifierCard(
		/*Cost*/ 0,
		/*Magnitude*/ 2,
		/*bReduceCost*/ false);
	UCardDefinition* TargetDefinition = Fixture.MakeNoopCard(/*Cost*/ 3);
	FCardEffect RuntimeDamage;
	RuntimeDamage.EffectType = WacomTags::Effect_Damage;
	RuntimeDamage.Magnitude = 1;
	RuntimeDamage.MagnitudeSource = WacomTags::Magnitude_Source_RuntimeCost;
	TargetDefinition->Effects.Add(RuntimeDamage);
	const FWacomInitializedBattleSession Initialized = Fixture.CreateInitializedSession(
		Fixture.MakeCharacter(
			Fixture.MakeNoopCard(0),
			Fixture.MakeNoopCard(0),
			{ SourceDefinition, TargetDefinition, Fixture.MakeNoopCard(0),
				Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0) }),
		Fixture.MakeSinglePartEnemy(20, 50, 0),
		43);
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
	const FGuid SourceCardId = FWacomBattleFixture::FindHandInstanceByCardId(
		PreSnapshot,
		SourceDefinition->CardId);
	const FGuid TargetCardId = FWacomBattleFixture::FindHandInstanceByCardId(
		PreSnapshot,
		TargetDefinition->CardId);
	const FBattleResolution Resolution = Initialized.Session->ResolveCommand(
		FBattleCommand::MakePlayCardOnHandCard(SourceCardId, TargetCardId));
	if (!TestTrue(TEXT("Cost-modifier hand target command succeeds"), Resolution.IsOk()))
	{
		return false;
	}

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

	for (int32 Step = 0; Step < 12 && HUD->IsPresentationPlanActiveForTest(); ++Step)
	{
		HUD->AdvanceBattlePresentationQueueForTest();
	}

	const TArray<FName> StartedPhases =
		HUD->GetStartedPresentationPlanPhaseNamesForTest();
	const int32 SourceIndex = StartedPhases.IndexOfByKey(FName(TEXT("CommandSourceOut")));
	const int32 TargetIndex = StartedPhases.IndexOfByKey(FName(TEXT("CommandPrimaryTarget")));
	const int32 OutcomeIndex = StartedPhases.IndexOfByKey(FName(TEXT("CommandOutcome")));
	TestTrue(
		TEXT("Cost rewrite follows source-out then primary-target"),
		SourceIndex != INDEX_NONE
			&& TargetIndex > SourceIndex
			&& OutcomeIndex > TargetIndex);

	const TArray<FWacomFirstPersonCardLayerFeedbackHint> FeedbackHints =
		HUD->GetSubmittedPresentationPlanFeedbackHintsForTest();
	const FWacomFirstPersonCardLayerFeedbackHint* RewriteHint =
		FeedbackHints.FindByPredicate(
			[&TargetCardId](const FWacomFirstPersonCardLayerFeedbackHint& Hint)
			{
				return Hint.CardInstanceId == TargetCardId
					&& Hint.FeedbackKind
						== EWacomFirstPersonCardLayerFeedbackKind::CardDataRewrite;
			});
	if (!TestNotNull(TEXT("Outcome contains the target cost rewrite"), RewriteHint))
	{
		return false;
	}
	TestTrue(
		TEXT("Command-owned cost rewrite blocks source return until complete"),
		RewriteHint->bBlocksPresentationPhase);
	const FWacomFirstPersonCardLayerFeedbackHint* BadgeHint =
		FeedbackHints.FindByPredicate(
			[&TargetCardId](const FWacomFirstPersonCardLayerFeedbackHint& Hint)
			{
				return Hint.CardInstanceId == TargetCardId
					&& Hint.FeedbackKind
						== EWacomFirstPersonCardLayerFeedbackKind::EffectBadgeChange;
			});
	if (!TestNotNull(TEXT("Outcome contains the parallel effect-badge rewrite"), BadgeHint))
	{
		return false;
	}
	TestTrue(
		TEXT("Command-owned badge rewrite blocks the same outcome phase"),
		BadgeHint->bBlocksPresentationPhase);
	TestEqual(TEXT("Outcome badge rewrite contains one stable item"), BadgeHint->EffectBadgeChanges.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCommandHandAnchorCostRewriteTest,
	"Wacom.UI.Battle.CommandPresentation.HandAnchorCostRewrite",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCommandHandAnchorCostRewriteTest::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleCommandHandFeedbackPresentationSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UCardDefinition* SourceDefinition = Fixture.MakeHandCardCostModifierCard(
		/*Cost*/ 0,
		/*Magnitude*/ 2,
		/*bReduceCost*/ false);
	const FWacomInitializedBattleSession Initialized = Fixture.CreateInitializedSession(
		Fixture.MakeCharacter(
			Fixture.MakeNoopCard(0),
			Fixture.MakeNoopCard(0),
			{ SourceDefinition, Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0) }),
		Fixture.MakeSinglePartEnemy(20, 50, 0),
		44);
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
	const FGuid SourceCardId = FWacomBattleFixture::FindHandInstanceByCardId(
		PreSnapshot,
		SourceDefinition->CardId);
	const FHandCardSnapshot* AnchorCard = PreSnapshot.Hand.Cards.FindByPredicate(
		[](const FHandCardSnapshot& Card)
		{
			return Card.bIsHandAnchor && Card.InstanceId.IsValid();
		});
	if (!TestNotNull(TEXT("Visible hand anchor"), AnchorCard))
	{
		return false;
	}
	const FGuid AnchorCardId = AnchorCard->InstanceId;
	const int32 CostBefore = AnchorCard->RuntimeCost;
	const FBattleResolution Resolution = Initialized.Session->ResolveCommand(
		FBattleCommand::MakePlayCardOnHandCard(SourceCardId, AnchorCardId));
	if (!TestTrue(TEXT("Cost-modifier anchor target command succeeds"), Resolution.IsOk()))
	{
		return false;
	}

	FWacomBattleCombatLogCommandContext LogContext =
		UWacomBattleCombatLogBuilder::BuildPlayCardCommandContext(
			PreSnapshot,
			SourceCardId,
			FBattlePartSlotIdentity(),
			AnchorCardId);
	LogContext.CardTargetPreview.bHasPreview = true;
	LogContext.CardTargetPreview.TargetKind = EWacomBattleCardPreviewTargetKind::HandCard;
	LogContext.CardTargetPreview.SourceCardInstanceId = SourceCardId;
	LogContext.CardTargetPreview.TargetHandCardInstanceId = AnchorCardId;
	HUD->ApplyCommandResolutionForTest(
		LogContext,
		PreSnapshot,
		Resolution,
		Initialized.Session,
		SourceCardId);

	for (int32 Step = 0; Step < 12 && HUD->IsPresentationPlanActiveForTest(); ++Step)
	{
		HUD->AdvanceBattlePresentationQueueForTest();
	}

	const TArray<FWacomFirstPersonCardLayerFeedbackHint> FeedbackHints =
		HUD->GetSubmittedPresentationPlanFeedbackHintsForTest();
	const FWacomFirstPersonCardLayerFeedbackHint* RewriteHint =
		FeedbackHints.FindByPredicate(
			[&AnchorCardId](const FWacomFirstPersonCardLayerFeedbackHint& Hint)
			{
				return Hint.CardInstanceId == AnchorCardId
					&& Hint.FeedbackKind
						== EWacomFirstPersonCardLayerFeedbackKind::CardDataRewrite;
			});
	if (!TestNotNull(TEXT("Command outcome contains the anchor cost rewrite"), RewriteHint))
	{
		return false;
	}
	TestTrue(
		TEXT("Command-owned anchor rewrite blocks its outcome phase"),
		RewriteHint->bBlocksPresentationPhase);
	TestEqual(TEXT("Anchor rewrite keeps the old cost"), RewriteHint->DataRewriteCostBefore, CostBefore);
	TestEqual(TEXT("Anchor rewrite carries the new cost"), RewriteHint->DataRewriteCostAfter, CostBefore + 2);
	return true;
}

#endif
