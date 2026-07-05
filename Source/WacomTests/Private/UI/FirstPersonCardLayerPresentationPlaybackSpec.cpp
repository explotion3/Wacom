// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "FirstPersonCardLayerSpecReceiver.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "UI/FirstPersonCardLayerTestAccess.h"

#if WITH_AUTOMATION_TESTS

namespace WacomFirstPersonCardLayerPresentationPlaybackSpec
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

	FWacomFirstPersonCardLayerSlotView MakeSlot(
		const FGuid& CardInstanceId,
		const FVector2D& Position)
	{
		FWacomFirstPersonCardLayerSlotView Slot;
		Slot.Index = 0;
		Slot.Entry.CardInstanceId = CardInstanceId;
		Slot.Entry.Zone = EHandZone::Both;
		Slot.Entry.bIsPlayable = true;
		Slot.ScreenPosition = Position;
		Slot.WidgetPosition = Position;
		Slot.SnappedWidgetPosition = Position;
		Slot.InputHitCenter = Position;
		Slot.InputHitScale = 1.0f;
		Slot.InputHitOrder = 0;
		Slot.RenderScale = 1.0f;
		Slot.RenderOpacity = 1.0f;
		Slot.AnchorWidgetPosition = FVector2D(500.0f, 500.0f);
		Slot.bProjected = true;
		return Slot;
	}

	FWacomFirstPersonCardLayerTransitionHint MakeTransitionHint(
		const FGuid& CardInstanceId,
		EWacomFirstPersonCardSlotTransitionKind TransitionKind)
	{
		FWacomFirstPersonCardLayerTransitionHint Hint;
		Hint.CardInstanceId = CardInstanceId;
		Hint.TransitionKind = TransitionKind;
		Hint.SequenceIndex = 0;
		Hint.SequenceCount = 1;
		return Hint;
	}

	FWacomFirstPersonCardLayerFeedbackHint MakeRetainedHint(const FGuid& CardInstanceId)
	{
		FWacomFirstPersonCardLayerFeedbackHint Hint;
		Hint.CardInstanceId = CardInstanceId;
		Hint.FeedbackKind = EWacomFirstPersonCardLayerFeedbackKind::Retained;
		Hint.SequenceIndex = 0;
		Hint.SequenceCount = 1;
		return Hint;
	}

	FWacomFirstPersonCardSlotMotionConfig MakeMotionConfig()
	{
		FWacomFirstPersonCardSlotMotionConfig Config;
		Config.bEnabled = true;
		Config.DrawnEnterOffsetPixels = FVector2D(0.0f, 80.0f);
		Config.DrawnEnterDurationSeconds = 0.2f;
		Config.DrawnEnterStaggerSeconds = 0.0f;
		Config.HandAnchorEnterOffsetPixels = FVector2D(0.0f, -80.0f);
		Config.HandAnchorEnterDurationSeconds = 0.2f;
		Config.HandAnchorEnterStaggerSeconds = 0.0f;
		Config.ExitDuration = 0.12f;
		return Config;
	}

	FWacomFirstPersonCardSlotFeedbackConfig MakeFeedbackConfig()
	{
		FWacomFirstPersonCardSlotFeedbackConfig Config;
		Config.bEnabled = true;
		Config.bEnableRetainedFeedback = true;
		Config.RetainedFeedbackDuration = 0.18f;
		Config.RetainedFeedbackStaggerSeconds = 0.0f;
		return Config;
	}

	UWacomFirstPersonCardAnchorSpecProbeComponent* AddProbeAnchor(AWacomPlayerCharacter* Character)
	{
		UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor =
			NewObject<UWacomFirstPersonCardAnchorSpecProbeComponent>(Character);
		if (Anchor)
		{
			Anchor->RegisterComponent();
			Anchor->FollowInterpSpeed = 0.0f;
			Anchor->bEnableCardLayerPixelSnapping = false;
			Anchor->SetRuntimeCardLayerTransitionPresentationEnabled(TEXT("BattleHand"), true);
		}
		return Anchor;
	}

	FWacomFirstPersonCardLayerEntry MakeEntry(
		const FGuid& CardInstanceId,
		const TCHAR* Name)
	{
		FWacomFirstPersonCardLayerEntry Entry;
		Entry.CardInstanceId = CardInstanceId;
		Entry.CardViewData.Name = FText::FromString(Name);
		Entry.Zone = EHandZone::Both;
		Entry.bIsPlayable = true;
		return Entry;
	}

	FWacomFirstPersonCardLayerFeedbackHint MakeFeedbackHint(
		const FGuid& CardInstanceId,
		EWacomFirstPersonCardLayerFeedbackKind FeedbackKind)
	{
		FWacomFirstPersonCardLayerFeedbackHint Hint;
		Hint.CardInstanceId = CardInstanceId;
		Hint.FeedbackKind = FeedbackKind;
		Hint.SequenceIndex = 0;
		Hint.SequenceCount = 1;
		return Hint;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerEnterPlaybackActiveContractTest,
	"Wacom.UI.FirstPersonCardLayer.PresentationPlayback.DrawnEnterReportsActive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerEnterPlaybackActiveContractTest::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardLayerPresentationPlaybackSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	Layer->SetSlotMotionConfig(MakeMotionConfig());
	const FGuid CardId = FGuid::NewGuid();
	Layer->SetCardTransitionHints({ MakeTransitionHint(CardId, EWacomFirstPersonCardSlotTransitionKind::Drawn) });
	Layer->SetCardSlots({ MakeSlot(CardId, FVector2D(120.0f, 240.0f)) });

	TestTrue(TEXT("Drawn enter reports active playback"), Layer->HasActivePresentationPlayback());
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.3f);
	TestFalse(TEXT("Drawn enter clears active playback"), Layer->HasActivePresentationPlayback());

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerHandAnchorEnterPlaybackActiveContractTest,
	"Wacom.UI.FirstPersonCardLayer.PresentationPlayback.HandAnchorEnterReportsActive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerHandAnchorEnterPlaybackActiveContractTest::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardLayerPresentationPlaybackSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	Layer->SetSlotMotionConfig(MakeMotionConfig());
	const FGuid CardId = FGuid::NewGuid();
	Layer->SetCardTransitionHints({ MakeTransitionHint(CardId, EWacomFirstPersonCardSlotTransitionKind::HandAnchorEntered) });
	Layer->SetCardSlots({ MakeSlot(CardId, FVector2D(160.0f, 260.0f)) });

	TestTrue(TEXT("Hand anchor enter reports active playback"), Layer->HasActivePresentationPlayback());
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.3f);
	TestFalse(TEXT("Hand anchor enter clears active playback"), Layer->HasActivePresentationPlayback());

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerExitPlaybackActiveContractTest,
	"Wacom.UI.FirstPersonCardLayer.PresentationPlayback.DiscardExitReportsActive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerExitPlaybackActiveContractTest::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardLayerPresentationPlaybackSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	Layer->SetSlotMotionConfig(MakeMotionConfig());
	const FGuid CardId = FGuid::NewGuid();
	Layer->SetCardSlots({ MakeSlot(CardId, FVector2D(180.0f, 260.0f)) });
	Layer->SetCardTransitionHints({ MakeTransitionHint(CardId, EWacomFirstPersonCardSlotTransitionKind::Discarded) });
	Layer->SetCardSlots({});

	TestTrue(TEXT("Discard exit reports active playback"), Layer->HasActivePresentationPlayback());
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.2f);
	TestFalse(TEXT("Discard exit clears active playback"), Layer->HasActivePresentationPlayback());

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerRetainedPlaybackActiveContractTest,
	"Wacom.UI.FirstPersonCardLayer.PresentationPlayback.RetainedFeedbackReportsActive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerRetainedPlaybackActiveContractTest::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardLayerPresentationPlaybackSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	Layer->SetSlotFeedbackConfig(MakeFeedbackConfig());
	const FGuid CardId = FGuid::NewGuid();
	Layer->SetCardFeedbackHints({ MakeRetainedHint(CardId) });
	Layer->SetCardSlots({ MakeSlot(CardId, FVector2D(220.0f, 280.0f)) });

	TestTrue(TEXT("Retained feedback reports active playback"), Layer->HasActivePresentationPlayback());
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.24f);
	TestFalse(TEXT("Retained feedback clears active playback"), Layer->HasActivePresentationPlayback());

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardAnchorRefreshCardLayerNowConsumesPresentationFrameTest,
	"Wacom.UI.FirstPersonCardLayer.PresentationPlayback.AnchorRefreshCardLayerNowConsumesPresentationFrame",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardAnchorRefreshCardLayerNowConsumesPresentationFrameTest::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardLayerPresentationPlaybackSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character =
		World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character))
	{
		return false;
	}
	PC->Possess(Character);

	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = AddProbeAnchor(Character);
	if (!TestNotNull(TEXT("Probe anchor"), Anchor))
	{
		PC->Destroy();
		Character->Destroy();
		return false;
	}

	const FName SourceId(TEXT("BattleHand"));
	const FGuid DiscardedId = FGuid::NewGuid();
	const FGuid RetainedId = FGuid::NewGuid();
	const FGuid DrawnId = FGuid::NewGuid();
	const FWacomFirstPersonCardLayerEntry DiscardedEntry = MakeEntry(DiscardedId, TEXT("Discarded"));
	const FWacomFirstPersonCardLayerEntry RetainedEntry = MakeEntry(RetainedId, TEXT("Retained"));
	const FWacomFirstPersonCardLayerEntry DrawnEntry = MakeEntry(DrawnId, TEXT("Drawn"));

	Anchor->SetRuntimeCardLayerEntries(SourceId, { DiscardedEntry, RetainedEntry });
	Anchor->RefreshCardLayerNow(0.0f);
	UWacomFirstPersonCardLayerWidget* Layer = Anchor->GetAutomationTestViewForTest().CardLayerWidget;
	if (!TestNotNull(TEXT("Card layer widget"), Layer))
	{
		PC->Destroy();
		Character->Destroy();
		return false;
	}
	TestFalse(TEXT("Initial hand has no active playback"), Anchor->HasActiveCardLayerPresentationPlayback());

	Anchor->SetRuntimeCardLayerPresentationFrame(
		SourceId,
		{ RetainedEntry },
		{ MakeTransitionHint(DiscardedId, EWacomFirstPersonCardSlotTransitionKind::Discarded) },
		{});
	Anchor->RefreshCardLayerNow(0.0f);
	TestTrue(TEXT("Discard presentation frame starts playback immediately"), Anchor->HasActiveCardLayerPresentationPlayback());
	TestTrue(TEXT("Discard presentation starts outgoing slot"), Layer->GetSlotMotionDebugView().OutgoingStartedThisUpdate > 0);

	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.40f);
	TestFalse(TEXT("Discard playback completes"), Anchor->HasActiveCardLayerPresentationPlayback());

	Anchor->SetRuntimeCardLayerPresentationFrame(
		SourceId,
		{ RetainedEntry },
		{},
		{ MakeFeedbackHint(RetainedId, EWacomFirstPersonCardLayerFeedbackKind::Retained) });
	Anchor->RefreshCardLayerNow(0.0f);
	TestTrue(TEXT("Retain feedback frame starts playback immediately"), Anchor->HasActiveCardLayerPresentationPlayback());

	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.40f);
	TestFalse(TEXT("Retain feedback completes"), Anchor->HasActiveCardLayerPresentationPlayback());

	Anchor->SetRuntimeCardLayerPresentationFrame(
		SourceId,
		{ RetainedEntry, DrawnEntry },
		{ MakeTransitionHint(DrawnId, EWacomFirstPersonCardSlotTransitionKind::Drawn) },
		{});
	Anchor->RefreshCardLayerNow(0.0f);
	TestTrue(TEXT("Draw presentation frame starts playback immediately"), Anchor->HasActiveCardLayerPresentationPlayback());

	PC->Destroy();
	Character->Destroy();
	return true;
}

#endif
