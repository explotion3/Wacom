// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "UI/FirstPersonCardLayerTestAccess.h"

#if WITH_AUTOMATION_TESTS

namespace WacomFirstPersonCardLayerPresentationAnchorSpec
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

	FWacomFirstPersonCardLayerSlotView MakeSlot(const FGuid& CardId, const FVector2D& Position)
	{
		FWacomFirstPersonCardLayerSlotView Slot;
		Slot.Index = 0;
		Slot.Entry.CardInstanceId = CardId;
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
		Slot.bProjected = true;
		return Slot;
	}

	FWacomFirstPersonCardLayerEntry MakeEntry(const FGuid& CardId)
	{
		FWacomFirstPersonCardLayerEntry Entry;
		Entry.CardInstanceId = CardId;
		Entry.Zone = EHandZone::Both;
		Entry.bIsPlayable = true;
		return Entry;
	}

	FWacomFirstPersonCardLayerTransitionHint MakeHint(
		const FGuid& CardId,
		EWacomFirstPersonCardSlotTransitionKind Kind)
	{
		FWacomFirstPersonCardLayerTransitionHint Hint;
		Hint.CardInstanceId = CardId;
		Hint.TransitionKind = Kind;
		return Hint;
	}

	FWacomFirstPersonCardSlotMotionConfig MakeMotionConfig()
	{
		FWacomFirstPersonCardSlotMotionConfig Config;
		Config.bEnabled = true;
		Config.bEnableEventAwareTransitions = true;
		Config.bEnableReadableTransitionOrigins = false;
		Config.EnterOffsetPixels = FVector2D::ZeroVector;
		Config.EnterOpacity = 1.0f;
		Config.DrawnEnterOffsetPixels = FVector2D(0.0f, 120.0f);
		Config.DrawnEnterDurationSeconds = 0.4f;
		Config.DrawnEnterArcLiftPixels = 0.0f;
		Config.DrawnEnterEasePower = 1.0f;
		Config.ExitDuration = 0.4f;
		Config.ExitMotionProfile.EasePower = 1.0f;
		return Config;
	}

	UWacomFirstPersonCardLayerWidget* MakeLayer(APlayerController& PC)
	{
		UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(&PC);
		FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, MakeMotionConfig());
		return Layer;
	}

	UWacomFirstPersonCardLayerSlotWidget* BeginExit(
		UWacomFirstPersonCardLayerWidget& Layer,
		const FGuid& CardId,
		const FVector2D& BasePosition,
		const FWacomFirstPersonCardLayerTransitionHint& Hint)
	{
		Layer.SetCardSlots({ MakeSlot(CardId, BasePosition) });
		FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(Layer, 1.0f);
		Layer.SetCardTransitionHints({ Hint });
		Layer.SetCardSlots({});
		return FWacomFirstPersonCardLayerTestAccess::OutgoingSlotAt(Layer, 0);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDrawPresentationAnchorTest,
	"Wacom.UI.FirstPersonCardLayer.PresentationAnchors.DrawnUsesDrawPileAnchor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDrawPresentationAnchorTest::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardLayerPresentationAnchorSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}
	APlayerController* PC = World->SpawnActor<APlayerController>();
	UWacomFirstPersonCardLayerWidget* Layer = PC ? MakeLayer(*PC) : nullptr;
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	FWacomFirstPersonCardPresentationAnchorSet Anchors;
	Anchors.DrawPile.bValid = true;
	Anchors.DrawPile.WidgetPosition = FVector2D(48.0f, 720.0f);
	Layer->SetPresentationAnchors(Anchors);
	const FGuid CardId = FGuid::NewGuid();
	Layer->SetCardTransitionHints({ MakeHint(CardId, EWacomFirstPersonCardSlotTransitionKind::Drawn) });
	Layer->SetCardSlots({ MakeSlot(CardId, FVector2D(420.0f, 640.0f)) });
	if (UWacomFirstPersonCardLayerSlotWidget* Slot = Layer->GetSlotWidgetAt(0))
	{
		TestEqual(
			TEXT("Drawn card starts at complete draw pile anchor without authored offset"),
			Slot->GetVisualSlotView().ScreenPosition,
			Anchors.DrawPile.WidgetPosition);
	}
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerExitPresentationAnchorTest,
	"Wacom.UI.FirstPersonCardLayer.PresentationAnchors.ExitPriorityAndDestinations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerExitPresentationAnchorTest::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardLayerPresentationAnchorSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}
	APlayerController* PC = World->SpawnActor<APlayerController>();
	if (!TestNotNull(TEXT("PlayerController"), PC))
	{
		return false;
	}

	FWacomFirstPersonCardPresentationAnchorSet Anchors;
	Anchors.PlayTarget.bValid = true;
	Anchors.PlayTarget.WidgetPosition = FVector2D(640.0f, 300.0f);
	Anchors.DiscardPile.bValid = true;
	Anchors.DiscardPile.WidgetPosition = FVector2D(1180.0f, 700.0f);
	const FVector2D BasePosition(420.0f, 640.0f);

	UWacomFirstPersonCardLayerWidget* NoTargetLayer = MakeLayer(*PC);
	NoTargetLayer->SetPresentationAnchors(Anchors);
	const FGuid NoTargetCardId = FGuid::NewGuid();
	UWacomFirstPersonCardLayerSlotWidget* NoTargetOutgoing = BeginExit(
		*NoTargetLayer,
		NoTargetCardId,
		BasePosition,
		MakeHint(NoTargetCardId, EWacomFirstPersonCardSlotTransitionKind::Played));
	if (TestNotNull(TEXT("No-target outgoing card"), NoTargetOutgoing))
	{
		FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*NoTargetOutgoing, 1.0f);
		TestEqual(TEXT("No-target played card finishes at PlayTarget"),
			NoTargetOutgoing->GetVisualSlotView().ScreenPosition, Anchors.PlayTarget.WidgetPosition);
	}

	UWacomFirstPersonCardLayerWidget* TargetedLayer = MakeLayer(*PC);
	TargetedLayer->SetPresentationAnchors(Anchors);
	const FGuid TargetedCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerTransitionHint TargetedHint =
		MakeHint(TargetedCardId, EWacomFirstPersonCardSlotTransitionKind::Played);
	TargetedHint.bHasPlayedExitTargetWidgetPosition = true;
	TargetedHint.PlayedExitTargetWidgetPosition = FVector2D(900.0f, 180.0f);
	UWacomFirstPersonCardLayerSlotWidget* TargetedOutgoing = BeginExit(
		*TargetedLayer, TargetedCardId, BasePosition, TargetedHint);
	if (TestNotNull(TEXT("Targeted outgoing card"), TargetedOutgoing))
	{
		FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*TargetedOutgoing, 1.0f);
		TestEqual(TEXT("Committed target overrides PlayTarget"),
			TargetedOutgoing->GetVisualSlotView().ScreenPosition,
			TargetedHint.PlayedExitTargetWidgetPosition);
	}

	UWacomFirstPersonCardLayerWidget* DiscardLayer = MakeLayer(*PC);
	DiscardLayer->SetPresentationAnchors(Anchors);
	const FGuid DiscardCardId = FGuid::NewGuid();
	UWacomFirstPersonCardLayerSlotWidget* DiscardOutgoing = BeginExit(
		*DiscardLayer,
		DiscardCardId,
		BasePosition,
		MakeHint(DiscardCardId, EWacomFirstPersonCardSlotTransitionKind::Discarded));
	if (TestNotNull(TEXT("Discard outgoing card"), DiscardOutgoing))
	{
		FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*DiscardOutgoing, 1.0f);
		TestEqual(TEXT("Discarded card finishes at discard pile anchor"),
			DiscardOutgoing->GetVisualSlotView().ScreenPosition, Anchors.DiscardPile.WidgetPosition);
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerAnchorLifecycleTest,
	"Wacom.UI.FirstPersonCardLayer.PresentationAnchors.SourceLifecycleDoesNotLeak",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerAnchorLifecycleTest::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardLayerPresentationAnchorSpec;
	UWacomFirstPersonCardAnchorComponent* Anchor =
		NewObject<UWacomFirstPersonCardAnchorComponent>(GetTransientPackage());
	if (!TestNotNull(TEXT("Anchor component"), Anchor))
	{
		return false;
	}

	const FName SourceA(TEXT("PresentationAnchorSourceA"));
	const FName SourceB(TEXT("PresentationAnchorSourceB"));
	FWacomFirstPersonCardLayerPresentationFrame PresentationFrame;
	PresentationFrame.SourceId = SourceA;
	PresentationFrame.Entries = { MakeEntry(FGuid::NewGuid()) };
	FWacomFirstPersonCardLayerSourceLifecycleFrame LifecycleFrame =
		FWacomFirstPersonCardLayerSourceLifecycleFrame::FromPresentationFrame(PresentationFrame);
	LifecycleFrame.bSetPresentationAnchors = true;
	LifecycleFrame.PresentationAnchors.DrawPile.bValid = true;
	LifecycleFrame.PresentationAnchors.DrawPile.WidgetPosition = FVector2D(100.0f, 200.0f);
	Anchor->ApplyRuntimeCardLayerSourceLifecycleFrame(LifecycleFrame);

	FWacomFirstPersonCardAnchorAutomationTestView View =
		FWacomFirstPersonCardLayerTestAccess::View(*Anchor);
	TestEqual(TEXT("Anchor set belongs to source A"), View.PresentationAnchorSourceId, SourceA);
	TestTrue(TEXT("Source A draw anchor is valid"), View.PresentationAnchors.DrawPile.bValid);

	FWacomFirstPersonCardLayerPresentationFrame RefreshFrame = PresentationFrame;
	RefreshFrame.Entries = { MakeEntry(FGuid::NewGuid()) };
	Anchor->ApplyRuntimeCardLayerSourceLifecycleFrame(
		FWacomFirstPersonCardLayerSourceLifecycleFrame::FromPresentationFrame(RefreshFrame));
	View = FWacomFirstPersonCardLayerTestAccess::View(*Anchor);
	TestEqual(TEXT("Ordinary same-source refresh preserves anchors"),
		View.PresentationAnchorSourceId, SourceA);

	FWacomFirstPersonCardLayerPresentationFrame OtherSourceFrame;
	OtherSourceFrame.SourceId = SourceB;
	OtherSourceFrame.Entries = { MakeEntry(FGuid::NewGuid()) };
	Anchor->ApplyRuntimeCardLayerSourceLifecycleFrame(
		FWacomFirstPersonCardLayerSourceLifecycleFrame::FromPresentationFrame(OtherSourceFrame));
	View = FWacomFirstPersonCardLayerTestAccess::View(*Anchor);
	TestTrue(TEXT("Source switch clears stale source A anchors"),
		View.PresentationAnchorSourceId.IsNone());
	TestFalse(TEXT("Source switch clears stale draw anchor value"),
		View.PresentationAnchors.DrawPile.bValid);
	return true;
}

#endif
