// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "FirstPersonCardLayerSpecReceiver.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "UI/Card/WacomFirstPersonCardLayerSourceIds.h"
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
		Config.GainedEnterOffsetPixels = FVector2D(0.0f, -80.0f);
		Config.GainedEnterDurationSeconds = 0.2f;
		Config.GainedEnterStaggerSeconds = 0.0f;
		Config.GainedEnterArcLiftPixels = 30.0f;
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
			Anchor->SetRuntimeCardLayerTransitionPresentationEnabled(WacomFirstPersonCardLayerSourceIds::BattleHand(), true);
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

	FWacomFirstPersonCardLayerPresentationFrame MakeCommitFrame(
		FName SourceId,
		const TArray<FWacomFirstPersonCardLayerEntry>& Entries,
		EWacomFirstPersonCardLayerFrameCommitMode CommitMode)
	{
		FWacomFirstPersonCardLayerPresentationFrame Frame;
		Frame.SourceId = SourceId;
		Frame.Entries = Entries;
		Frame.CommitMode = CommitMode;
		return Frame;
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
	FWacomFirstPersonCardLayerGainedEnterPlaybackActiveContractTest,
	"Wacom.UI.FirstPersonCardLayer.PresentationPlayback.GainedEnterReportsActive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerGainedEnterPlaybackActiveContractTest::RunTest(const FString& /*Parameters*/)
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
	Layer->SetCardTransitionHints({ MakeTransitionHint(CardId, EWacomFirstPersonCardSlotTransitionKind::Gained) });
	Layer->SetCardSlots({ MakeSlot(CardId, FVector2D(140.0f, 250.0f)) });

	TestTrue(TEXT("Gained enter reports active playback"), Layer->HasActivePresentationPlayback());
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.3f);
	TestFalse(TEXT("Gained enter clears active playback"), Layer->HasActivePresentationPlayback());

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

	const FName SourceId = WacomFirstPersonCardLayerSourceIds::BattleHand();
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardAnchorCommitFrameModesContractTest,
	"Wacom.UI.FirstPersonCardLayer.PresentationPlayback.AnchorCommitFrameModes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardAnchorCommitFrameModesContractTest::RunTest(const FString& /*Parameters*/)
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

	const FName SourceId = WacomFirstPersonCardLayerSourceIds::BattleHand();
	const FGuid DrawnId = FGuid::NewGuid();
	const FGuid RetainedId = FGuid::NewGuid();
	const FGuid AddedId = FGuid::NewGuid();
	const FGuid PreviewId = FGuid::NewGuid();
	const FWacomFirstPersonCardLayerEntry DrawnEntry = MakeEntry(DrawnId, TEXT("Drawn"));
	const FWacomFirstPersonCardLayerEntry RetainedEntry = MakeEntry(RetainedId, TEXT("Retained"));
	const FWacomFirstPersonCardLayerEntry AddedEntry = MakeEntry(AddedId, TEXT("Added"));
	const FWacomFirstPersonCardLayerEntry PreviewEntry = MakeEntry(PreviewId, TEXT("Preview"));

	FWacomFirstPersonCardLayerPresentationFrame PresentationFrame =
		MakeCommitFrame(
			SourceId,
			{ DrawnEntry, RetainedEntry },
			EWacomFirstPersonCardLayerFrameCommitMode::PresentationFrame);
	PresentationFrame.TransitionHints.Add(
		MakeTransitionHint(DrawnId, EWacomFirstPersonCardSlotTransitionKind::Drawn));
	PresentationFrame.FeedbackHints.Add(
		MakeFeedbackHint(RetainedId, EWacomFirstPersonCardLayerFeedbackKind::Retained));
	Anchor->CommitRuntimeCardLayerFrame(PresentationFrame);

	const FWacomFirstPersonCardAnchorAutomationTestView PendingAfterPresentation =
		Anchor->GetAutomationTestViewForTest();
	TestEqual(
		TEXT("Presentation frame writes one pending transition"),
		PendingAfterPresentation.PendingTransitionHintCardIds.Num(),
		1);
	TestEqual(
		TEXT("Presentation frame writes one pending feedback"),
		PendingAfterPresentation.PendingFeedbackHintCardIds.Num(),
		1);
	TestTrue(
		TEXT("Presentation frame stores draw hint"),
		PendingAfterPresentation.PendingTransitionHintCardIds.Contains(DrawnId));
	TestTrue(
		TEXT("Presentation frame stores feedback hint"),
		PendingAfterPresentation.PendingFeedbackHintCardIds.Contains(RetainedId));

	const FWacomFirstPersonCardLayerPresentationFrame StateRefreshFrame =
		MakeCommitFrame(
			SourceId,
			{ DrawnEntry, RetainedEntry, AddedEntry },
			EWacomFirstPersonCardLayerFrameCommitMode::StateRefresh);
	Anchor->CommitRuntimeCardLayerFrame(StateRefreshFrame);

	const FWacomFirstPersonCardAnchorAutomationTestView PendingAfterStateRefresh =
		Anchor->GetAutomationTestViewForTest();
	TestEqual(
		TEXT("State refresh updates entries"),
		Anchor->GetRuntimeCardLayerCardCount(),
		3);
	TestTrue(
		TEXT("State refresh preserves pending draw hint"),
		PendingAfterStateRefresh.PendingTransitionHintCardIds.Contains(DrawnId));
	TestTrue(
		TEXT("State refresh preserves pending feedback hint"),
		PendingAfterStateRefresh.PendingFeedbackHintCardIds.Contains(RetainedId));

	const FWacomFirstPersonCardLayerPresentationFrame PreviewFrame =
		MakeCommitFrame(
			SourceId,
			{ PreviewEntry },
			EWacomFirstPersonCardLayerFrameCommitMode::PreviewOverlay);
	Anchor->CommitRuntimeCardLayerFrame(PreviewFrame);

	const FWacomFirstPersonCardAnchorAutomationTestView PendingAfterPreview =
		Anchor->GetAutomationTestViewForTest();
	TestEqual(
		TEXT("Preview overlay updates entries"),
		Anchor->GetRuntimeCardLayerCardCount(),
		1);
	TestTrue(
		TEXT("Preview overlay preserves pending draw hint"),
		PendingAfterPreview.PendingTransitionHintCardIds.Contains(DrawnId));
	TestTrue(
		TEXT("Preview overlay preserves pending feedback hint"),
		PendingAfterPreview.PendingFeedbackHintCardIds.Contains(RetainedId));

	const FWacomFirstPersonCardLayerPresentationFrame SuppressedFrame =
		MakeCommitFrame(
			SourceId,
			{},
			EWacomFirstPersonCardLayerFrameCommitMode::Suppressed);
	Anchor->CommitRuntimeCardLayerFrame(SuppressedFrame);

	const FWacomFirstPersonCardAnchorAutomationTestView PendingAfterSuppressed =
		Anchor->GetAutomationTestViewForTest();
	TestEqual(
		TEXT("Suppressed frame clears entries"),
		Anchor->GetRuntimeCardLayerCardCount(),
		0);
	TestEqual(
		TEXT("Suppressed frame clears pending transitions"),
		PendingAfterSuppressed.PendingTransitionHintCardIds.Num(),
		0);
	TestEqual(
		TEXT("Suppressed frame clears pending feedback"),
		PendingAfterSuppressed.PendingFeedbackHintCardIds.Num(),
		0);

	PC->Destroy();
	Character->Destroy();
	return true;
}

#endif
