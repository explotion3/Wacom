// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "BattleHUDTestHarness.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Session/BattleSession.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "UI/PileCountViewTestAccess.h"

#if WITH_AUTOMATION_TESTS

namespace WacomBattleReshufflePileFeedbackSpec
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

	UBattleSession* CreateSession(FWacomBattleFixture& Fixture)
	{
		UCardDefinition* LeftHand = Fixture.MakeNoopCard(0);
		UCardDefinition* RightHand = Fixture.MakeNoopCard(0);
		UCharacterDefinition* Character = Fixture.MakeCharacter(
			LeftHand,
			RightHand,
			{ Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0) });
		return Fixture.CreateSession(Character, Fixture.MakeSinglePartEnemy(20, 50, 0), 1);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleReshufflePileFeedbackSpec,
	"Wacom.UI.Battle.PresentationPlan.ReshufflePileFeedbackUsesLaunchAndArrivalEdges",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleReshufflePileFeedbackSpec::RunTest(const FString&)
{
	using namespace WacomBattleReshufflePileFeedbackSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UBattleSession* Session = CreateSession(Fixture);
	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	if (!TestNotNull(TEXT("Battle session"), Session)
		|| !TestNotNull(TEXT("HUD harness"), Harness.Get()))
	{
		return false;
	}

	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	Harness->SetSession(Session);
	HUD->CreatePileViewsForTest();
	UPileCountView* DrawPileView = HUD->GetDrawPileViewForTest();
	UPileCountView* DiscardPileView = HUD->GetDiscardPileViewForTest();
	if (!TestNotNull(TEXT("DrawPileView"), DrawPileView)
		|| !TestNotNull(TEXT("DiscardPileView"), DiscardPileView))
	{
		return false;
	}

	HUD->PrimeReshufflePileFeedbackForTest(31, 2, 0, 2, 2, 0, 3);
	DrawPileView->SetCount(0);
	DiscardPileView->SetCount(2);

	FWacomFirstPersonCardPileTransferProgressView Progress;
	Progress.EventSequence = 31;
	Progress.TransferKind = FWacomFirstPersonCardPileTransferHint::ETransferKind::DiscardPileToDraw;
	Progress.TotalCount = 2;
	Progress.LaunchedCount = 1;
	Progress.LaunchDirection = FVector2D(1.0f, -0.25f).GetSafeNormal();
	HUD->HandlePileTransferProgressForTest(Progress);
	HUD->HandlePileTransferProgressForTest(Progress);
	TestEqual(TEXT("first real launch removes exactly one discard"), DiscardPileView->GetCount(), 1);
	TestEqual(TEXT("played count remains visible during reshuffle"),
		DiscardPileView->GetCountDisplayText().ToString(), FString(TEXT("1+3")));
	TestEqual(TEXT("launch alone does not add to the draw pile"), DrawPileView->GetCount(), 0);
	FWacomPileCountViewTestAccess::Tick(*DiscardPileView, 0.075f);
	TestTrue(TEXT("discard pile recoils opposite the launch direction"),
		DiscardPileView->GetRenderTransform().Translation.X < 0.0f);

	Progress.LaunchedCount = 2;
	Progress.ArrivedCount = 1;
	Progress.LaunchDirection = FVector2D(0.8f, -0.6f).GetSafeNormal();
	HUD->HandlePileTransferProgressForTest(Progress);
	TestEqual(TEXT("final real launch empties only the reshuffled discard cards"),
		DiscardPileView->GetCount(), 0);
	TestEqual(TEXT("played cards remain composed after source pile reaches zero"),
		DiscardPileView->GetCountDisplayText().ToString(), FString(TEXT("0+3")));
	TestEqual(TEXT("first arrival adds exactly one draw card"), DrawPileView->GetCount(), 1);

	DrawPileView->ResetReceiveFeedback();
	Progress.ArrivedCount = 2;
	Progress.bCompleted = true;
	HUD->HandlePileTransferProgressForTest(Progress);
	HUD->HandlePileTransferProgressForTest(Progress);
	TestEqual(TEXT("final arrival restores the exact draw-pile result"), DrawPileView->GetCount(), 2);
	FWacomPileCountViewTestAccess::Tick(*DrawPileView, 0.04f);
	TestTrue(TEXT("final arrival uses the stronger receive pulse exactly once"),
		FMath::IsNearlyEqual(DrawPileView->GetRenderTransform().Scale.Y, 0.928f, 0.001f));

	HUD->PrimeReshufflePileFeedbackForTest(32, 2, 0, 2, 2, 0, 3);
	Progress.EventSequence = 32;
	Progress.LaunchedCount = 2;
	Progress.ArrivedCount = 2;
	Progress.bWasForceCompleted = true;
	HUD->HandlePileTransferProgressForTest(Progress);
	TestEqual(TEXT("force complete restores the final source count"), DiscardPileView->GetCount(), 0);
	TestEqual(TEXT("force complete restores the final target count"), DrawPileView->GetCount(), 2);
	TestTrue(TEXT("force complete clears source feedback"),
		DiscardPileView->GetRenderTransform().Scale.Equals(FVector2D(1.0f, 1.0f), 0.001f));
	TestTrue(TEXT("force complete clears target feedback"),
		DrawPileView->GetRenderTransform().Scale.Equals(FVector2D(1.0f, 1.0f), 0.001f));
	return true;
}

#endif
