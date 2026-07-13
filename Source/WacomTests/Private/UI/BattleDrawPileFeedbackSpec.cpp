// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Events/BattleEvent.h"
#include "GameFramework/PlayerController.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "UI/Common/PileCountView.h"

#if WITH_AUTOMATION_TESTS

namespace WacomBattleDrawPileFeedbackSpec
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

	FWacomFirstPersonCardLayerTransitionHint MakeDrawnHint(const FGuid& CardInstanceId)
	{
		FWacomFirstPersonCardLayerTransitionHint Hint;
		Hint.CardInstanceId = CardInstanceId;
		Hint.TransitionKind = EWacomFirstPersonCardSlotTransitionKind::Drawn;
		return Hint;
	}

	FWacomFirstPersonCardEnterTransitionStartedView MakeStartedView(
		const FGuid& CardInstanceId,
		EWacomFirstPersonCardSlotTransitionKind TransitionKind =
			EWacomFirstPersonCardSlotTransitionKind::Drawn)
	{
		FWacomFirstPersonCardEnterTransitionStartedView View;
		View.CardInstanceId = CardInstanceId;
		View.TransitionKind = TransitionKind;
		View.StartWidgetPosition = FVector2D(100.0f, 300.0f);
		View.TargetWidgetPosition = FVector2D(500.0f, 180.0f);
		return View;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleDrawPileFeedbackPerCardSpec,
	"Wacom.UI.Battle.DrawPileFeedback.PerCardStartDedupAndFinalCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleDrawPileFeedbackPerCardSpec::RunTest(const FString&)
{
	using namespace WacomBattleDrawPileFeedbackSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass());
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("player controller"), PC) || !TestNotNull(TEXT("battle HUD"), HUD))
	{
		return false;
	}
	HUD->SetWorldForTest(World);
	HUD->SetOwningPlayerForTest(PC);
	HUD->CreatePileViewsForTest();
	UPileCountView* DrawPileView = HUD->GetDrawPileViewForTest();
	if (!TestNotNull(TEXT("draw pile view"), DrawPileView))
	{
		PC->Destroy();
		return false;
	}

	const FGuid FirstCardId = FGuid::NewGuid();
	const FGuid SecondCardId = FGuid::NewGuid();
	FBattleEvent DrawEvent;
	DrawEvent.Type = EBattleEventType::CardsDrawn;
	DrawEvent.Sequence = 41;
	DrawEvent.CardInstanceIds = { FirstCardId, SecondCardId };
	DrawEvent.Count = 2;
	DrawEvent.DrawPileCountAfter = 3;
	HUD->StoreFirstPersonCardTransitionEventsForTest({ DrawEvent });
	HUD->PrepareDrawPileFeedbackForTest({ MakeDrawnHint(FirstCardId), MakeDrawnHint(SecondCardId) });
	TestEqual(TEXT("frame restores the draw-before count"), DrawPileView->GetCount(), 5);

	HUD->DispatchEnterTransitionStartedForTest(MakeStartedView(
		FirstCardId, EWacomFirstPersonCardSlotTransitionKind::Gained));
	TestEqual(TEXT("non-Drawn semantic does not decrement"), DrawPileView->GetCount(), 5);
	HUD->DispatchEnterTransitionStartedForTest(MakeStartedView(FirstCardId));
	TestEqual(TEXT("first real Drawn start decrements exactly once"), DrawPileView->GetCount(), 4);
	HUD->DispatchEnterTransitionStartedForTest(MakeStartedView(FirstCardId));
	TestEqual(TEXT("duplicate start does not decrement twice"), DrawPileView->GetCount(), 4);
	HUD->DispatchEnterTransitionStartedForTest(MakeStartedView(SecondCardId));
	TestEqual(TEXT("last visible start restores authoritative final count"), DrawPileView->GetCount(), 3);

	HUD->StoreFirstPersonCardTransitionEventsForTest({ DrawEvent });
	HUD->PrepareDrawPileFeedbackForTest({ MakeDrawnHint(FirstCardId), MakeDrawnHint(SecondCardId) });
	TestEqual(TEXT("completed event sequence cannot replay on lifecycle refresh"), DrawPileView->GetCount(), 3);

	HUD->ResetDrawPileFeedbackForTest(8);
	TestEqual(TEXT("reset restores supplied authoritative count"), DrawPileView->GetCount(), 8);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleDrawPileFeedbackHiddenCardSpec,
	"Wacom.UI.Battle.DrawPileFeedback.HiddenCardDeltaReconcilesWithoutFakePulse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleDrawPileFeedbackHiddenCardSpec::RunTest(const FString&)
{
	using namespace WacomBattleDrawPileFeedbackSpec;
	UWorld* World = FindAutomationWorld();
	if (!World)
	{
		return false;
	}
	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass());
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	HUD->SetWorldForTest(World);
	HUD->SetOwningPlayerForTest(PC);
	HUD->CreatePileViewsForTest();

	const FGuid VisibleId = FGuid::NewGuid();
	const FGuid HiddenId = FGuid::NewGuid();
	FBattleEvent DrawEvent;
	DrawEvent.Type = EBattleEventType::CardsDrawn;
	DrawEvent.Sequence = 52;
	DrawEvent.CardInstanceIds = { VisibleId, HiddenId };
	DrawEvent.Count = 2;
	DrawEvent.DrawPileCountAfter = 6;
	HUD->StoreFirstPersonCardTransitionEventsForTest({ DrawEvent });
	HUD->PrepareDrawPileFeedbackForTest({ MakeDrawnHint(VisibleId) });
	TestEqual(TEXT("batch begins from pre-draw count"), HUD->GetDrawPileViewForTest()->GetCount(), 8);
	HUD->DispatchEnterTransitionStartedForTest(MakeStartedView(VisibleId));
	TestEqual(TEXT("last visible card reconciles hidden delta to final count"),
		HUD->GetDrawPileViewForTest()->GetCount(), 6);

	PC->Destroy();
	return true;
}

#endif
