// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "FirstPersonCardLayerSpecReceiver.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "Sound/SoundWave.h"
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
		Config.DiscardedExitStaggerSeconds = 0.08f;
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
			FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerTransitionPresentationEnabled(*Anchor, WacomFirstPersonCardLayerSourceIds::BattleHand(), true);
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
	FWacomFirstPersonCardLayerDiscardExitStaggersBySequenceTest,
	"Wacom.UI.FirstPersonCardLayer.PresentationPlayback.DiscardExitStaggersBySequence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDiscardExitStaggersBySequenceTest::RunTest(
	const FString& /*Parameters*/)
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
	const FGuid FirstCardId = FGuid::NewGuid();
	const FGuid SecondCardId = FGuid::NewGuid();
	const FVector2D FirstStart(180.0f, 260.0f);
	const FVector2D SecondStart(320.0f, 260.0f);
	Layer->SetCardSlots({ MakeSlot(FirstCardId, FirstStart), MakeSlot(SecondCardId, SecondStart) });
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
	FWacomFirstPersonCardLayerTransitionHint FirstHint =
		MakeTransitionHint(FirstCardId, EWacomFirstPersonCardSlotTransitionKind::Discarded);
	FirstHint.SequenceIndex = 0;
	FirstHint.SequenceCount = 2;
	FWacomFirstPersonCardLayerTransitionHint SecondHint =
		MakeTransitionHint(SecondCardId, EWacomFirstPersonCardSlotTransitionKind::Discarded);
	SecondHint.SequenceIndex = 1;
	SecondHint.SequenceCount = 2;
	Layer->SetCardTransitionHints({ FirstHint, SecondHint });
	Layer->SetCardSlots({});

	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.04f);
	UWacomFirstPersonCardLayerSlotWidget* FirstOutgoing =
		FWacomFirstPersonCardLayerTestAccess::OutgoingSlotAt(*Layer, 0);
	UWacomFirstPersonCardLayerSlotWidget* SecondOutgoing =
		FWacomFirstPersonCardLayerTestAccess::OutgoingSlotAt(*Layer, 1);
	if (TestNotNull(TEXT("First outgoing card"), FirstOutgoing)
		&& TestNotNull(TEXT("Second outgoing card"), SecondOutgoing))
	{
		TestTrue(
			TEXT("First discard has begun moving"),
			FirstOutgoing->GetVisualSlotView().ScreenPosition.Y > FirstStart.Y);
		TestEqual(
			TEXT("Second discard waits for its sequence delay"),
			SecondOutgoing->GetVisualSlotView().ScreenPosition,
			SecondStart);
	}

	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.3f);
	TestFalse(TEXT("Staggered discard playback eventually settles"), Layer->HasActivePresentationPlayback());
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerForceSettlePresentationPlaybackTest,
	"Wacom.UI.FirstPersonCardLayer.PresentationPlayback.ForceSettleClearsOutgoingPlayback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerForceSettlePresentationPlaybackTest::RunTest(
	const FString& /*Parameters*/)
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
	Layer->SetCardTransitionHints({
		MakeTransitionHint(CardId, EWacomFirstPersonCardSlotTransitionKind::Discarded)
	});
	Layer->SetCardSlots({});
	TestTrue(TEXT("Exit is active before force settle"), Layer->HasActivePresentationPlayback());
	Layer->ForceSettlePresentationPlayback();
	TestFalse(TEXT("Force settle clears active presentation playback"), Layer->HasActivePresentationPlayback());
	TestEqual(TEXT("Force settle removes completed outgoing slots"), Layer->GetOutgoingCardViewCount(), 0);

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

	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerEntries(*Anchor, SourceId, { DiscardedEntry, RetainedEntry });
	Anchor->RefreshCardLayerNow(0.0f);
	UWacomFirstPersonCardLayerWidget* Layer =
		FWacomFirstPersonCardLayerTestAccess::CardLayer(*Anchor);
	if (!TestNotNull(TEXT("Card layer widget"), Layer))
	{
		PC->Destroy();
		Character->Destroy();
		return false;
	}
	TestFalse(TEXT("Initial hand has no active playback"), Anchor->HasActiveCardLayerPresentationPlayback());

	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerPresentationFrame(*Anchor,
		SourceId,
		{ RetainedEntry },
		{ MakeTransitionHint(DiscardedId, EWacomFirstPersonCardSlotTransitionKind::Discarded) });
	Anchor->RefreshCardLayerNow(0.0f);
	TestTrue(TEXT("Discard presentation frame starts playback immediately"), Anchor->HasActiveCardLayerPresentationPlayback());
	TestTrue(TEXT("Discard presentation starts outgoing slot"), Layer->GetSlotMotionDebugView().OutgoingStartedThisUpdate > 0);

	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.40f);
	TestFalse(TEXT("Discard playback completes"), Anchor->HasActiveCardLayerPresentationPlayback());

	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerPresentationFrame(*Anchor,
		SourceId,
		{ RetainedEntry, DrawnEntry },
		{ MakeTransitionHint(DrawnId, EWacomFirstPersonCardSlotTransitionKind::Drawn) });
	Anchor->RefreshCardLayerNow(0.0f);
	TestTrue(TEXT("Draw presentation frame starts playback immediately"), Anchor->HasActiveCardLayerPresentationPlayback());

	PC->Destroy();
	Character->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardAnchorEmptyPresentationFrameReplacesPendingHintsTest,
	"Wacom.UI.FirstPersonCardLayer.PresentationPlayback.EmptyPresentationFrameReplacesPendingHints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardAnchorEmptyPresentationFrameReplacesPendingHintsTest::RunTest(
	const FString& /*Parameters*/)
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
	const FGuid DeferredHintCardId = FGuid::NewGuid();
	const FGuid InitiallyVisibleCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerPresentationFrame DeferredHintFrame = MakeCommitFrame(
		SourceId,
		{ MakeEntry(InitiallyVisibleCardId, TEXT("InitiallyVisible")) },
		EWacomFirstPersonCardLayerFrameCommitMode::PresentationFrame);
	DeferredHintFrame.TransitionHints.Add(
		MakeTransitionHint(DeferredHintCardId, EWacomFirstPersonCardSlotTransitionKind::Drawn));
	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerPresentationFrame(*Anchor, DeferredHintFrame);
	Anchor->RefreshCardLayerNow(0.0f);

	FWacomFirstPersonCardLayerPresentationFrame EmptyReplacementFrame = MakeCommitFrame(
		SourceId,
		{ MakeEntry(DeferredHintCardId, TEXT("Replacement")) },
		EWacomFirstPersonCardLayerFrameCommitMode::PresentationFrame);
	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerPresentationFrame(*Anchor, EmptyReplacementFrame);
	Anchor->RefreshCardLayerNow(0.0f);

	UWacomFirstPersonCardLayerWidget* Layer =
		FWacomFirstPersonCardLayerTestAccess::CardLayer(*Anchor);
	UWacomFirstPersonCardLayerSlotWidget* ReplacementSlot =
		Layer ? Layer->GetSlotWidgetAt(0) : nullptr;
	if (TestNotNull(TEXT("Card layer widget"), Layer)
		&& TestNotNull(TEXT("Replacement slot"), ReplacementSlot))
	{
		const FWacomFirstPersonCardSlotAutomationTestView SlotView =
			FWacomFirstPersonCardLayerTestAccess::View(*ReplacementSlot);
		TestFalse(
			TEXT("Explicit empty presentation frame clears the older deferred transition hint"),
			SlotView.bEnterTransitionPlaybackActive);
	}

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
	FWacomFirstPersonCardLayerTestAccess::CommitRuntimeCardLayerFrame(*Anchor, PresentationFrame);

	const FWacomFirstPersonCardAnchorAutomationTestView PendingAfterPresentation =
		FWacomFirstPersonCardLayerTestAccess::View(*Anchor);
	TestEqual(
		TEXT("Presentation frame writes one pending transition"),
		PendingAfterPresentation.PendingTransitionHintCardIds.Num(),
		1);
	TestTrue(
		TEXT("Presentation frame stores draw hint"),
		PendingAfterPresentation.PendingTransitionHintCardIds.Contains(DrawnId));

	const FWacomFirstPersonCardLayerPresentationFrame StateRefreshFrame =
		MakeCommitFrame(
			SourceId,
			{ DrawnEntry, RetainedEntry, AddedEntry },
			EWacomFirstPersonCardLayerFrameCommitMode::StateRefresh);
	FWacomFirstPersonCardLayerTestAccess::CommitRuntimeCardLayerFrame(*Anchor, StateRefreshFrame);

	const FWacomFirstPersonCardAnchorAutomationTestView PendingAfterStateRefresh =
		FWacomFirstPersonCardLayerTestAccess::View(*Anchor);
	TestEqual(
		TEXT("State refresh updates entries"),
		Anchor->GetRuntimeCardLayerCardCount(),
		3);
	TestTrue(
		TEXT("State refresh preserves pending draw hint"),
		PendingAfterStateRefresh.PendingTransitionHintCardIds.Contains(DrawnId));

	const FWacomFirstPersonCardLayerPresentationFrame PreviewFrame =
		MakeCommitFrame(
			SourceId,
			{ PreviewEntry },
			EWacomFirstPersonCardLayerFrameCommitMode::PreviewOverlay);
	FWacomFirstPersonCardLayerTestAccess::CommitRuntimeCardLayerFrame(*Anchor, PreviewFrame);

	const FWacomFirstPersonCardAnchorAutomationTestView PendingAfterPreview =
		FWacomFirstPersonCardLayerTestAccess::View(*Anchor);
	TestEqual(
		TEXT("Preview overlay updates entries"),
		Anchor->GetRuntimeCardLayerCardCount(),
		1);
	TestTrue(
		TEXT("Preview overlay preserves pending draw hint"),
		PendingAfterPreview.PendingTransitionHintCardIds.Contains(DrawnId));

	const FWacomFirstPersonCardLayerPresentationFrame SuppressedFrame =
		MakeCommitFrame(
			SourceId,
			{},
			EWacomFirstPersonCardLayerFrameCommitMode::Suppressed);
	FWacomFirstPersonCardLayerTestAccess::CommitRuntimeCardLayerFrame(*Anchor, SuppressedFrame);

	const FWacomFirstPersonCardAnchorAutomationTestView PendingAfterSuppressed =
		FWacomFirstPersonCardLayerTestAccess::View(*Anchor);
	TestEqual(
		TEXT("Suppressed frame clears entries"),
		Anchor->GetRuntimeCardLayerCardCount(),
		0);
	TestEqual(
		TEXT("Suppressed frame clears pending transitions"),
		PendingAfterSuppressed.PendingTransitionHintCardIds.Num(),
		0);

	PC->Destroy();
	Character->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerGainedAndRetainedPlaybackTest,
	"Wacom.UI.FirstPersonCardLayer.PresentationPlayback.GainedAndRetainedSemanticMotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerGainedAndRetainedPlaybackTest::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardLayerPresentationPlaybackSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World)) return false;
	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("Layer"), Layer)) return false;
	FWacomFirstPersonCardSlotMotionConfig Motion = MakeMotionConfig();
	Motion.GainedEnterDurationSeconds = 0.2f;
	Motion.GainedEnterSound = NewObject<USoundWave>(Layer);
	Layer->SetSlotMotionConfig(Motion);
	const FGuid CardId = FGuid::NewGuid();
	Layer->SetCardTransitionHints({ MakeTransitionHint(CardId, EWacomFirstPersonCardSlotTransitionKind::Gained) });
	Layer->SetCardSlots({ MakeSlot(CardId, FVector2D(220.0f, 280.0f)) });
	TestTrue(TEXT("Gained uses finite semantic enter playback"), Layer->HasActivePresentationPlayback());
	const FWacomFirstPersonCardSlotAutomationTestView GainedView =
		FWacomFirstPersonCardLayerTestAccess::View(*Layer->GetSlotWidgetAt(0));
	TestEqual(TEXT("Gained enter requests its sound once"), GainedView.EnterTransitionSoundRequestCount, 1);
	TestEqual(TEXT("Gained sound keeps semantic kind"), GainedView.LastEnterTransitionSoundKind, EWacomFirstPersonCardSlotTransitionKind::Gained);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.3f);
	FWacomFirstPersonCardLayerFeedbackHint RetainedHint;
	RetainedHint.CardInstanceId = CardId;
	RetainedHint.FeedbackKind = EWacomFirstPersonCardLayerFeedbackKind::Retained;
	Layer->SetCardFeedbackHints({ RetainedHint });
	Layer->SetCardSlots({ MakeSlot(CardId, FVector2D(220.0f, 280.0f)) });
	TestTrue(TEXT("Retained reports active semantic motion"), Layer->HasActivePresentationPlayback());
	const FWacomFirstPersonCardSlotAutomationTestView RetainedView =
		FWacomFirstPersonCardLayerTestAccess::View(*Layer->GetSlotWidgetAt(0));
	TestTrue(TEXT("Retained motion starts without requiring an overlay"), RetainedView.bRetainedFeedbackActive);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerEnterStartedEdgeTest,
	"Wacom.UI.FirstPersonCardLayer.DrawTransition.EnterStartedUsesActualPlaybackEdge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerEnterStartedEdgeTest::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardLayerPresentationPlaybackSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World)) return false;
	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("Layer"), Layer)) return false;

	TArray<FWacomFirstPersonCardEnterTransitionStartedView> StartedViews;
	Layer->OnEnterTransitionStartedNative.AddLambda(
		[&StartedViews](const FWacomFirstPersonCardEnterTransitionStartedView& View)
		{
			StartedViews.Add(View);
		});
	FWacomFirstPersonCardSlotMotionConfig Motion = MakeMotionConfig();
	Motion.DrawnEnterStaggerSeconds = 0.0f;
	Layer->SetSlotMotionConfig(Motion);
	const FGuid CardId = FGuid::NewGuid();
	const FVector2D TargetPosition(320.0f, 240.0f);
	Layer->SetCardTransitionHints({ MakeTransitionHint(CardId, EWacomFirstPersonCardSlotTransitionKind::Drawn) });
	Layer->SetCardSlots({ MakeSlot(CardId, TargetPosition) });

	TestEqual(TEXT("zero-delay Enter notifies once at BeginEnter"), StartedViews.Num(), 1);
	if (StartedViews.Num() == 1)
	{
		TestEqual(TEXT("started view keeps card identity"), StartedViews[0].CardInstanceId, CardId);
		TestEqual(TEXT("started view keeps semantic kind"), StartedViews[0].TransitionKind,
			EWacomFirstPersonCardSlotTransitionKind::Drawn);
		TestTrue(TEXT("started view carries the live target position"),
			StartedViews[0].TargetWidgetPosition.Equals(TargetPosition, 0.001f));
		TestFalse(TEXT("started view carries the authored enter origin"),
			StartedViews[0].StartWidgetPosition.Equals(TargetPosition, 0.001f));
	}
	Layer->SetCardSlots({ MakeSlot(CardId, TargetPosition) });
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.05f);
	TestEqual(TEXT("ordinary refresh does not replay the start edge"), StartedViews.Num(), 1);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDelayedEnterStartedEdgeTest,
	"Wacom.UI.FirstPersonCardLayer.DrawTransition.DelayedEnterStartsOnceAndCanBeCancelled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDelayedEnterStartedEdgeTest::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardLayerPresentationPlaybackSpec;
	UWorld* World = FindAutomationWorld();
	if (!World) return false;
	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!Layer) return false;

	int32 StartedCount = 0;
	Layer->OnEnterTransitionStartedNative.AddLambda(
		[&StartedCount](const FWacomFirstPersonCardEnterTransitionStartedView&)
		{
			++StartedCount;
		});
	FWacomFirstPersonCardSlotMotionConfig Motion = MakeMotionConfig();
	Motion.DrawnEnterStaggerSeconds = 0.10f;
	Layer->SetSlotMotionConfig(Motion);
	const FGuid FirstCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerTransitionHint DelayedHint =
		MakeTransitionHint(FirstCardId, EWacomFirstPersonCardSlotTransitionKind::Drawn);
	DelayedHint.SequenceIndex = 1;
	DelayedHint.SequenceCount = 2;
	Layer->SetCardTransitionHints({ DelayedHint });
	Layer->SetCardSlots({ MakeSlot(FirstCardId, FVector2D(420.0f, 260.0f)) });
	TestEqual(TEXT("delayed Enter does not notify during authored wait"), StartedCount, 0);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.05f);
	TestEqual(TEXT("partial delay still has no start edge"), StartedCount, 0);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.06f);
	TestEqual(TEXT("crossing delay emits exactly one start edge"), StartedCount, 1);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.05f);
	TestEqual(TEXT("running Enter does not repeat the edge"), StartedCount, 1);

	const FGuid CancelledCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerTransitionHint CancelledHint =
		MakeTransitionHint(CancelledCardId, EWacomFirstPersonCardSlotTransitionKind::Drawn);
	CancelledHint.SequenceIndex = 1;
	CancelledHint.SequenceCount = 2;
	Layer->SetCardTransitionHints({ CancelledHint });
	Layer->SetCardSlots({ MakeSlot(CancelledCardId, FVector2D(520.0f, 260.0f)) });
	if (UWacomFirstPersonCardLayerSlotWidget* Slot = Layer->GetSlotWidgetAt(0))
	{
		Slot->ForceCompletePresentationPlayback();
	}
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.20f);
	TestEqual(TEXT("force completion before delay does not emit a false start"), StartedCount, 1);

	PC->Destroy();
	return true;
}

#endif
