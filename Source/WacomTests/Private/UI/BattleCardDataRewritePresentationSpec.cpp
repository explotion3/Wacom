// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "BattleHUDTestHarness.h"
#include "Cards/CardDefinition.h"
#include "Commands/BattleCommand.h"
#include "Engine/Engine.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Session/BattleInitializationResult.h"
#include "Session/BattleResolution.h"
#include "Session/BattleSession.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/BattleWidgetSpecReceiver.h"

#if WITH_AUTOMATION_TESTS

namespace WacomBattleCardDataRewritePresentationSpec
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

	FWacomInitializedBattleSession CreateSession(
		FWacomBattleFixture& Fixture,
		UCardDefinition*& OutSourceCard,
		UCardDefinition*& OutTargetCard)
	{
		OutSourceCard = Fixture.MakeHandCardCostModifierCard(
			/*Cost*/ 0,
			/*Magnitude*/ 2,
			/*bReduceCost*/ false);
		OutTargetCard = Fixture.MakeNoopCard(/*Cost*/ 3);
		FCardEffect RuntimeDamage;
		RuntimeDamage.EffectType = WacomTags::Effect_Damage;
		RuntimeDamage.Magnitude = 1;
		RuntimeDamage.MagnitudeSource = WacomTags::Magnitude_Source_RuntimeCost;
		OutTargetCard->Effects.Add(RuntimeDamage);
		return Fixture.CreateInitializedSession(
			Fixture.MakeCharacter(
				Fixture.MakeNoopCard(0),
				Fixture.MakeNoopCard(0),
				{ OutSourceCard, OutTargetCard, Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0) }),
			Fixture.MakeSinglePartEnemy(/*Hp*/ 100, /*Initiative*/ 50),
			23);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleCardDataRewritePresentationTest,
	"Wacom.UI.Battle.CardDataRewrite.HandTargetCostChange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleCardDataRewritePresentationTest::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleCardDataRewritePresentationSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UCardDefinition* SourceCard = nullptr;
	UCardDefinition* TargetCard = nullptr;
	const FWacomInitializedBattleSession Initialized = CreateSession(
		Fixture,
		SourceCard,
		TargetCard);
	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	if (!TestNotNull(TEXT("Initialized session"), Initialized.Session)
		|| !TestNotNull(TEXT("HUD harness"), Harness.Get()))
	{
		return false;
	}
	if (!TestNotNull(
		TEXT("First-person character supplies the real card Anchor lifecycle"),
		Harness->AttachFirstPersonCharacter()))
	{
		return false;
	}
	Harness->SetInitializedSession(Initialized);
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	if (!TestNotNull(TEXT("Battle HUD"), HUD))
	{
		return false;
	}

	const FBattleSnapshot PreSnapshot = Initialized.Session->BuildSnapshot();
	// The real HUD has already presented this stable hand before a player command.
	// Prime that exact baseline explicitly so the test does not depend on entry
	// animation timing inside the harness.
	HUD->SyncFirstPersonBattleHandLayerForTest(PreSnapshot);
	const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(
		PreSnapshot,
		SourceCard->CardId);
	const FGuid TargetId = FWacomBattleFixture::FindHandInstanceByCardId(
		PreSnapshot,
		TargetCard->CardId);
	const FBattleResolution Resolution = Initialized.Session->ResolveCommand(
		FBattleCommand::MakePlayCardOnHandCard(SourceId, TargetId));
	if (!TestTrue(TEXT("Hand-card cost change succeeds"), Resolution.IsOk()))
	{
		return false;
	}

	HUD->StoreFirstPersonCardTransitionEventsForTest(Resolution.Events);
	const TArray<FWacomFirstPersonCardLayerFeedbackHint> Hints =
		HUD->BuildFirstPersonCardFeedbackHintsForTest(Resolution.PostSnapshot);
	const FWacomFirstPersonCardLayerFeedbackHint* RewriteHint = Hints.FindByPredicate(
		[&TargetId](const FWacomFirstPersonCardLayerFeedbackHint& Hint)
		{
			return Hint.CardInstanceId == TargetId
				&& Hint.FeedbackKind == EWacomFirstPersonCardLayerFeedbackKind::CardDataRewrite;
		});
	if (!TestNotNull(TEXT("Visible target cost change emits a rewrite hint"), RewriteHint))
	{
		return false;
	}
	TestEqual(
		TEXT("Increasing cost uses the detrimental tone"),
		RewriteHint->DataRewriteTone,
		EWacomFirstPersonCardDataRewriteTone::Detrimental);
	TestTrue(
		TEXT("Rewrite includes the cost field"),
		(RewriteHint->DataRewriteFieldMask
			& static_cast<int32>(EWacomFirstPersonCardDataRewriteField::Cost)) != 0);
	TestTrue(
		TEXT("Rewrite carries an authoritative before/after cost pair"),
		RewriteHint->bHasDataRewriteCostValues);
	TestEqual(TEXT("Rewrite preserves the pre-preview cost"), RewriteHint->DataRewriteCostBefore, 3);
	TestEqual(TEXT("Rewrite preserves the committed cost"), RewriteHint->DataRewriteCostAfter, 5);
	TestTrue(TEXT("Rewrite uses a stable non-zero seed"), RewriteHint->DataRewriteSeed != 0);

	const FWacomFirstPersonCardLayerFeedbackHint* BadgeHint = Hints.FindByPredicate(
		[&TargetId](const FWacomFirstPersonCardLayerFeedbackHint& Hint)
		{
			return Hint.CardInstanceId == TargetId
				&& Hint.FeedbackKind == EWacomFirstPersonCardLayerFeedbackKind::EffectBadgeChange;
		});
	if (!TestNotNull(TEXT("Runtime-cost badge changes alongside the cost digit"), BadgeHint))
	{
		return false;
	}
	TestEqual(TEXT("Only the visible runtime badge changes"), BadgeHint->EffectBadgeChanges.Num(), 1);
	if (BadgeHint->EffectBadgeChanges.Num() == 1)
	{
		const FWacomFirstPersonCardEffectBadgeChange& Change = BadgeHint->EffectBadgeChanges[0];
		TestEqual(TEXT("Badge keeps its semantic aggregate identity"),
			Change.PresentationKey, FName(TEXT("Badge.Damage")));
		TestEqual(TEXT("Badge old value follows pre-command runtime cost"), Change.OldValue, 3);
		TestEqual(TEXT("Badge new value follows post-command runtime cost"), Change.NewValue, 5);
		TestEqual(TEXT("Badge direction is increase"), Change.Direction, EWacomFirstPersonCardEffectBadgeValueDirection::Increase);
	}

	HUD->ClearPendingFirstPersonCardTransitionEventsForTest();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleHandAnchorDataRewritePresentationTest,
	"Wacom.UI.Battle.CardDataRewrite.HandAnchorCostChange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleHandAnchorDataRewritePresentationTest::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleCardDataRewritePresentationSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UCardDefinition* SourceCard = nullptr;
	UCardDefinition* UnusedNormalTargetCard = nullptr;
	const FWacomInitializedBattleSession Initialized = CreateSession(
		Fixture,
		SourceCard,
		UnusedNormalTargetCard);
	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	if (!TestNotNull(TEXT("Initialized session"), Initialized.Session)
		|| !TestNotNull(TEXT("HUD harness"), Harness.Get())
		|| !TestNotNull(TEXT("First-person character"), Harness->AttachFirstPersonCharacter()))
	{
		return false;
	}
	Harness->SetInitializedSession(Initialized);
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	if (!TestNotNull(TEXT("Battle HUD"), HUD))
	{
		return false;
	}

	const FBattleSnapshot PreSnapshot = Initialized.Session->BuildSnapshot();
	HUD->SyncFirstPersonBattleHandLayerForTest(PreSnapshot);
	const FGuid SourceId = FWacomBattleFixture::FindHandInstanceByCardId(
		PreSnapshot,
		SourceCard->CardId);
	const FHandCardSnapshot* AnchorCard = PreSnapshot.Hand.Cards.FindByPredicate(
		[](const FHandCardSnapshot& Card)
		{
			return Card.bIsHandAnchor && Card.InstanceId.IsValid();
		});
	if (!TestNotNull(TEXT("Visible hand anchor"), AnchorCard))
	{
		return false;
	}
	const FGuid AnchorId = AnchorCard->InstanceId;
	const int32 CostBefore = AnchorCard->RuntimeCost;
	const FBattleResolution Resolution = Initialized.Session->ResolveCommand(
		FBattleCommand::MakePlayCardOnHandCard(SourceId, AnchorId));
	if (!TestTrue(TEXT("Hand-anchor cost change succeeds"), Resolution.IsOk()))
	{
		return false;
	}

	HUD->StoreFirstPersonCardTransitionEventsForTest(Resolution.Events);
	const TArray<FWacomFirstPersonCardLayerFeedbackHint> Hints =
		HUD->BuildFirstPersonCardFeedbackHintsForTest(Resolution.PostSnapshot);
	const FWacomFirstPersonCardLayerFeedbackHint* RewriteHint = Hints.FindByPredicate(
		[&AnchorId](const FWacomFirstPersonCardLayerFeedbackHint& Hint)
		{
			return Hint.CardInstanceId == AnchorId
				&& Hint.FeedbackKind == EWacomFirstPersonCardLayerFeedbackKind::CardDataRewrite;
		});
	if (!TestNotNull(TEXT("Visible hand-anchor cost change emits a rewrite hint"), RewriteHint))
	{
		return false;
	}
	TestTrue(TEXT("Anchor rewrite carries authoritative costs"), RewriteHint->bHasDataRewriteCostValues);
	TestEqual(TEXT("Anchor rewrite preserves its old cost"), RewriteHint->DataRewriteCostBefore, CostBefore);
	TestEqual(TEXT("Anchor rewrite carries its new cost"), RewriteHint->DataRewriteCostAfter, CostBefore + 2);
	TestEqual(
		TEXT("Increasing anchor cost uses the detrimental tone"),
		RewriteHint->DataRewriteTone,
		EWacomFirstPersonCardDataRewriteTone::Detrimental);

	HUD->ClearPendingFirstPersonCardTransitionEventsForTest();
	return true;
}

#endif
