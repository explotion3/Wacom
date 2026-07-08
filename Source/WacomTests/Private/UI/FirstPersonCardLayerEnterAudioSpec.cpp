// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Sound/SoundWave.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "UI/FirstPersonCardLayerTestAccess.h"

#if WITH_AUTOMATION_TESTS

namespace WacomFirstPersonCardLayerEnterAudioSpec
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

	FWacomFirstPersonCardLayerTransitionHint MakeHint(
		const FGuid& CardInstanceId,
		int32 SequenceIndex = 0,
		int32 SequenceCount = 1)
	{
		FWacomFirstPersonCardLayerTransitionHint Hint;
		Hint.CardInstanceId = CardInstanceId;
		Hint.TransitionKind = EWacomFirstPersonCardSlotTransitionKind::Drawn;
		Hint.SequenceIndex = SequenceIndex;
		Hint.SequenceCount = SequenceCount;
		return Hint;
	}

	FWacomFirstPersonCardSlotMotionConfig MakeMotionConfig(USoundBase* DrawnSound)
	{
		FWacomFirstPersonCardSlotMotionConfig Config;
		Config.bEnabled = true;
		Config.bEnableEventAwareTransitions = true;
		Config.bEnableReadableTransitionOrigins = false;
		Config.DrawnEnterSound = DrawnSound;
		Config.DrawnEnterOffsetPixels = FVector2D(0.0f, 80.0f);
		Config.DrawnEnterDurationSeconds = 0.2f;
		Config.DrawnEnterStaggerSeconds = 0.0f;
		Config.DrawnEnterArcLiftPixels = 0.0f;
		Config.DrawnEnterEasePower = 1.0f;
		Config.EnterSoundVolumeMultiplier = 0.0f;
		return Config;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerOrdinaryEnterDoesNotPlaySoundTest,
	"Wacom.UI.FirstPersonCardLayer.EnterAudio.OrdinaryEnterDoesNotRequestSound",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerOrdinaryEnterDoesNotPlaySoundTest::RunTest(const FString& Parameters)
{
	using namespace WacomFirstPersonCardLayerEnterAudioSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	if (!TestNotNull(TEXT("PlayerController"), PC))
	{
		return false;
	}

	USoundWave* Sound = NewObject<USoundWave>(GetTransientPackage());
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	Layer->SetSlotMotionConfig(MakeMotionConfig(Sound));

	const FGuid CardId = FGuid::NewGuid();
	Layer->SetCardSlots({ MakeSlot(CardId, FVector2D(160.0f, 240.0f)) });
	UWacomFirstPersonCardLayerSlotWidget* Slot = Layer->GetSlotWidgetAt(0);
	if (TestNotNull(TEXT("Slot"), Slot))
	{
		TestEqual(
			TEXT("Ordinary new slot does not request enter sound"),
			FWacomFirstPersonCardLayerTestAccess::View(*Slot).EnterTransitionSoundRequestCount,
			0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDrawnEnterSoundStartsImmediatelyTest,
	"Wacom.UI.FirstPersonCardLayer.EnterAudio.DrawnEnterRequestsSoundAtStart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDrawnEnterSoundStartsImmediatelyTest::RunTest(const FString& Parameters)
{
	using namespace WacomFirstPersonCardLayerEnterAudioSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	if (!TestNotNull(TEXT("PlayerController"), PC))
	{
		return false;
	}

	USoundWave* Sound = NewObject<USoundWave>(GetTransientPackage());
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	Layer->SetSlotMotionConfig(MakeMotionConfig(Sound));

	const FGuid CardId = FGuid::NewGuid();
	Layer->SetCardTransitionHints({ MakeHint(CardId) });
	Layer->SetCardSlots({ MakeSlot(CardId, FVector2D(160.0f, 240.0f)) });
	UWacomFirstPersonCardLayerSlotWidget* Slot = Layer->GetSlotWidgetAt(0);
	if (TestNotNull(TEXT("Slot"), Slot))
	{
		const FWacomFirstPersonCardSlotAutomationTestView View =
			FWacomFirstPersonCardLayerTestAccess::View(*Slot);
		TestEqual(TEXT("Drawn enter requests sound once"), View.EnterTransitionSoundRequestCount, 1);
		TestEqual(
			TEXT("Drawn enter records sound kind"),
			View.LastEnterTransitionSoundKind,
			EWacomFirstPersonCardSlotTransitionKind::Drawn);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDrawnEnterSoundHonorsStaggerTest,
	"Wacom.UI.FirstPersonCardLayer.EnterAudio.DrawnEnterSoundHonorsStagger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDrawnEnterSoundHonorsStaggerTest::RunTest(const FString& Parameters)
{
	using namespace WacomFirstPersonCardLayerEnterAudioSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	if (!TestNotNull(TEXT("PlayerController"), PC))
	{
		return false;
	}

	USoundWave* Sound = NewObject<USoundWave>(GetTransientPackage());
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	FWacomFirstPersonCardSlotMotionConfig Config = MakeMotionConfig(Sound);
	Config.DrawnEnterStaggerSeconds = 0.1f;
	Layer->SetSlotMotionConfig(Config);

	const FGuid CardId = FGuid::NewGuid();
	Layer->SetCardTransitionHints({ MakeHint(CardId, 2, 3) });
	Layer->SetCardSlots({ MakeSlot(CardId, FVector2D(160.0f, 240.0f)) });
	UWacomFirstPersonCardLayerSlotWidget* Slot = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot"), Slot))
	{
		return false;
	}

	TestEqual(
		TEXT("Sound waits for stagger delay"),
		FWacomFirstPersonCardLayerTestAccess::View(*Slot).EnterTransitionSoundRequestCount,
		0);

	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.19f);
	TestEqual(
		TEXT("Sound still waits before delay"),
		FWacomFirstPersonCardLayerTestAccess::View(*Slot).EnterTransitionSoundRequestCount,
		0);

	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.02f);
	TestEqual(
		TEXT("Sound requests when stagger starts"),
		FWacomFirstPersonCardLayerTestAccess::View(*Slot).EnterTransitionSoundRequestCount,
		1);

	Layer->SetCardSlots({ MakeSlot(CardId, FVector2D(160.0f, 240.0f)) });
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.1f);
	TestEqual(
		TEXT("Ordinary refresh does not repeat sound"),
		FWacomFirstPersonCardLayerTestAccess::View(*Slot).EnterTransitionSoundRequestCount,
		1);

	return true;
}

#endif
