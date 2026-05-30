// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomRunTunnelSegmentActor.h"
#include "Components/SplineComponent.h"
#include "Components/WacomBattleCameraLookComponent.h"
#include "Components/WacomCursorLookDriverComponent.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Components/WacomRunTunnelMovementComponent.h"
#include "Cards/CardDefinition.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Fixtures/BattleTestFixtures.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "GameFramework/WacomPlayerController.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "UI/Card/WacomCardPresentationBuilder.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/Card/WacomFirstPersonCardLayoutPreset.h"
#include "UI/Card/WacomCardView.h"
#include "UI/FirstPersonCardLayerSpecReceiver.h"

namespace WacomFirstPersonCardLayerSpec
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

	AWacomRunTunnelSegmentActor* SpawnTestSegment(UWorld& World, const FVector& Start, const FVector& End)
	{
		AWacomRunTunnelSegmentActor* Segment = World.SpawnActor<AWacomRunTunnelSegmentActor>(
			AWacomRunTunnelSegmentActor::StaticClass(),
			FTransform::Identity);
		if (!Segment || !Segment->GetPathSpline())
		{
			return Segment;
		}

		USplineComponent* Spline = Segment->GetPathSpline();
		Spline->ClearSplinePoints(false);
		Spline->AddSplinePoint(Start, ESplineCoordinateSpace::World, false);
		Spline->AddSplinePoint(End, ESplineCoordinateSpace::World, false);
		Spline->SetSplinePointType(0, ESplinePointType::Linear, false);
		Spline->SetSplinePointType(1, ESplinePointType::Linear, false);
		Spline->UpdateSpline();
		return Segment;
	}

	UWacomFirstPersonCardAnchorSpecProbeComponent* AddProbe(AWacomPlayerCharacter* Character)
	{
		UWacomFirstPersonCardAnchorSpecProbeComponent* Probe =
			NewObject<UWacomFirstPersonCardAnchorSpecProbeComponent>(Character);
		if (Probe)
		{
			Probe->RegisterComponent();
			Probe->FollowInterpSpeed = 0.0f;
		}
		return Probe;
	}

	void PrimeFallbackAnchor(APlayerController* PC, AWacomPlayerCharacter* Character, UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor)
	{
		if (PC && Character)
		{
			PC->Possess(Character);
		}
		if (Anchor)
		{
			Anchor->ProbeCameraTransform = FTransform(
				FRotator::ZeroRotator,
				FVector(100.0f, 200.0f, 300.0f),
				FVector::OneVector);
			Anchor->RefreshAnchor(0.0f);
		}
	}

	UCardDefinition* MakePreviewCard(UObject* Outer, const TCHAR* Name, int32 Cost)
	{
		UCardDefinition* Card = NewObject<UCardDefinition>(Outer);
		if (!Card)
		{
			return nullptr;
		}

		Card->CardId = FName(Name);
		Card->DisplayName = FText::FromString(Name);
		Card->Description = FText::FromString(TEXT("Static layer preview card"));
		Card->BaseCost = Cost;
		return Card;
	}

	void PrimeBattleHUDWithCharacter(
		UWacomBattleHUDDetailTest* HUD,
		APlayerController* PC,
		AWacomPlayerCharacter* Character,
		UWorld* World)
	{
		if (PC && Character)
		{
			PC->Possess(Character);
		}
		if (HUD)
		{
			HUD->SetOwningPlayerForTest(PC);
			HUD->SetWorldForTest(World);
		}
	}

	void SettleBattlePresentationQueue(UWacomBattleHUDDetailTest& HUD, int32 MaxSteps = 32)
	{
		for (int32 Iteration = 0; HUD.IsBattlePresentationBusy() && Iteration < MaxSteps; ++Iteration)
		{
			HUD.AdvanceBattlePresentationQueueForTest();
		}
	}

	bool HasTickPrerequisite(const FTickFunction& TickFunction, const UObject* Object, const FTickFunction& PrerequisiteTick)
	{
		for (const FTickPrerequisite& Prerequisite : TickFunction.GetPrerequisites())
		{
			if (Prerequisite.PrerequisiteObject.Get() == Object
				&& Prerequisite.Get() == &PrerequisiteTick)
			{
				return true;
			}
		}
		return false;
	}

	UBattleSession* CreateMinimalBattleSession(FWacomBattleFixture& Fixture)
	{
		UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemy(20, 0, 0);
		UCharacterDefinition* CharacterDefinition = Fixture.MakeCharacter(
			Fixture.MakeNoopCard(0),
			Fixture.MakeNoopCard(0),
			{ Fixture.MakeNoopCard(0) });
		return Fixture.CreateSession(CharacterDefinition, Enemy, 1);
	}

	FBattleSnapshot MakeSnapshotWithHand(const TArray<FHandCardSnapshot>& Cards, EBattlePhase Phase = EBattlePhase::PlayerAction)
	{
		FBattleSnapshot Snapshot;
		Snapshot.Phase = Phase;
		Snapshot.Hand.Cards = Cards;
		Snapshot.Hand.NormalCardCount = Cards.Num();
		return Snapshot;
	}

	FHandCardSnapshot MakeHandCardSnapshot(
		UCardDefinition* Card,
		int32 RuntimeCost,
		bool bPlayable)
	{
		FHandCardSnapshot Snapshot;
		Snapshot.InstanceId = FGuid::NewGuid();
		Snapshot.Definition = Card;
		Snapshot.RuntimeCost = RuntimeCost;
		Snapshot.Zone = EHandZone::Both;
		Snapshot.bIsPlayable = bPlayable;
		return Snapshot;
	}

	FGuid FindFirstHandCardByTargetMode(const FBattleSnapshot& Snapshot, ECardTargetMode TargetMode)
	{
		for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
		{
			if (Card.Definition && Card.Definition->TargetMode == TargetMode)
			{
				return Card.InstanceId;
			}
		}
		return FGuid();
	}

	FGuid FindFirstHandAnchor(const FBattleSnapshot& Snapshot)
	{
		for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
		{
			if (Card.bIsHandAnchor)
			{
				return Card.InstanceId;
			}
		}
		return FGuid();
	}

	FWacomFirstPersonCardDragView MakeDropDragView(
		const FGuid& CardInstanceId,
		EWacomFirstPersonCardGestureState GestureState,
		bool bCommitArmed = false)
	{
		FWacomFirstPersonCardDragView DragView;
		DragView.CardInstanceId = CardInstanceId;
		DragView.GestureState = GestureState;
		DragView.bCommitArmed = bCommitArmed;
		DragView.PressScreenPosition = FVector2D(500.0f, 600.0f);
		DragView.CurrentScreenPosition = FVector2D(540.0f, 590.0f);
		DragView.PointerViewportPosition = DragView.CurrentScreenPosition;
		DragView.PointerNormalizedViewportPosition = FVector2D::ZeroVector;
		DragView.bHasPointerViewportPosition = true;
		return DragView;
	}

	FWacomFirstPersonCardLayerSlotView MakeProjectedInteractionSlot(
		const FGuid& CardInstanceId,
		bool bPlayable = true,
		bool bProjected = true)
	{
		FWacomFirstPersonCardLayerSlotView Slot;
		Slot.Index = 0;
		Slot.Entry.CardInstanceId = CardInstanceId;
		Slot.Entry.bIsPlayable = bPlayable;
		Slot.Entry.CardViewData.bDisabled = !bPlayable;
		Slot.Entry.TargetMode = ECardTargetMode::None;
		Slot.ScreenPosition = FVector2D(500.0f, 600.0f);
		Slot.RenderScale = 0.55f;
		Slot.RenderOpacity = 1.0f;
		Slot.ZOrder = 0;
		Slot.bProjected = bProjected;
		return Slot;
	}

	FWacomFirstPersonCardLayerSlotView MakeMotionSlot(
		const FGuid& CardInstanceId,
		int32 Index,
		const FVector2D& Position,
		float Angle = 0.0f,
		float Scale = 1.0f,
		float Opacity = 1.0f)
	{
		FWacomFirstPersonCardLayerSlotView Slot = MakeProjectedInteractionSlot(CardInstanceId);
		Slot.Index = Index;
		Slot.ScreenPosition = Position;
		Slot.WidgetPosition = Position;
		Slot.SnappedWidgetPosition = Position;
		Slot.RenderAngleDegrees = Angle;
		Slot.RenderScale = Scale;
		Slot.RenderOpacity = Opacity;
		Slot.ZOrder = Index;
		return Slot;
	}

	FWacomFirstPersonCardSlotMotionConfig MakeFastSlotMotionConfig()
	{
		FWacomFirstPersonCardSlotMotionConfig Config;
		Config.bEnabled = true;
		Config.MotionSpeed = 1.0f;
		Config.OpacitySpeed = 1.0f;
		Config.EnterOffsetPixels = FVector2D(0.0f, 40.0f);
		Config.EnterOpacity = 0.0f;
		Config.ExitOffsetPixels = FVector2D(0.0f, 30.0f);
		Config.ExitDuration = 0.2f;
		Config.ResetDistancePixels = 420.0f;
		Config.bEnableEventAwareTransitions = true;
		Config.bEnableReadableTransitionOrigins = true;
		Config.DrawnEnterOffsetPixels = FVector2D(0.0f, 96.0f);
		Config.DrawnEnterOriginMode = EWacomFirstPersonCardTransitionOriginMode::HandAnchorOffset;
		Config.DrawnEnterViewportAnchor = FVector2D(0.5f, 1.0f);
		Config.DrawnEnterScaleMultiplier = 0.96f;
		Config.DrawnEnterAngleOffsetDegrees = 0.0f;
		Config.GainedEnterOffsetPixels = FVector2D(0.0f, -120.0f);
		Config.GainedEnterOriginMode = EWacomFirstPersonCardTransitionOriginMode::HandAnchorOffset;
		Config.GainedEnterViewportAnchor = FVector2D(0.5f, 0.0f);
		Config.GainedEnterScaleMultiplier = 0.96f;
		Config.GainedEnterAngleOffsetDegrees = 0.0f;
		Config.PlayedExitOffsetPixels = FVector2D(0.0f, -120.0f);
		Config.PlayedExitOriginMode = EWacomFirstPersonCardTransitionOriginMode::SlotOffset;
		Config.PlayedExitViewportAnchor = FVector2D(0.5f, 0.0f);
		Config.PlayedExitScaleMultiplier = 0.96f;
		Config.PlayedExitAngleOffsetDegrees = 0.0f;
		Config.DiscardedExitOffsetPixels = FVector2D(0.0f, 120.0f);
		Config.DiscardedExitOriginMode = EWacomFirstPersonCardTransitionOriginMode::SlotOffset;
		Config.DiscardedExitViewportAnchor = FVector2D(0.5f, 1.0f);
		Config.DiscardedExitScaleMultiplier = 0.96f;
		Config.DiscardedExitAngleOffsetDegrees = 0.0f;
		return Config;
	}

	FWacomFirstPersonCardSlotFeedbackConfig MakeTestFeedbackConfig()
	{
		FWacomFirstPersonCardSlotFeedbackConfig Config;
		Config.bEnabled = true;
		Config.PlayableHoverColor = FLinearColor(1.0f, 0.92f, 0.45f, 1.0f);
		Config.PlayableHoverOpacity = 0.2f;
		Config.PressedScale = 0.9f;
		Config.PressedColor = FLinearColor::White;
		Config.PressedOpacity = 0.3f;
	Config.ConfirmDuration = 0.1f;
	Config.ConfirmOpacity = 0.4f;
	Config.DenyDuration = 0.2f;
	Config.DenyShakePixels = 8.0f;
	Config.DenyColor = FLinearColor::Red;
	Config.DenyOpacity = 0.5f;
	Config.bEnablePlayCommitFeedback = true;
	Config.PlayCommitDuration = 0.12f;
	Config.PlayCommitOpacity = 0.6f;
	Config.PlayCommitColor = FLinearColor::Green;
	Config.PlayCommitScale = 1.1f;
	return Config;
}

	UWacomFirstPersonCardLayoutPreset* MakeLayoutPreset(UObject* Outer)
	{
		UWacomFirstPersonCardLayoutPreset* Preset = NewObject<UWacomFirstPersonCardLayoutPreset>(Outer);
		if (!Preset)
		{
			return nullptr;
		}

		Preset->CardLayoutMode = EWacomFirstPersonCardLayoutMode::Authored2D;
		Preset->ProjectionMode = EWacomFirstPersonCardProjectionMode::BodyLocked;
		Preset->ViewportClampMode = EWacomFirstPersonCardViewportClampMode::SoftClampToViewport;
		Preset->AuthoredCardSpacingPixels = 160.0f;
		Preset->AuthoredMaxHandWidthPixels = 640.0f;
		Preset->AuthoredHandScreenOffset = FVector2D(24.0f, -12.0f);
		Preset->AuthoredCenterLiftPixels = 32.0f;
		Preset->AuthoredDropCurveExponent = 3.0f;
		Preset->AuthoredFanCurveExponent = 2.0f;
		Preset->StaticCardRenderScale = 0.9f;
		Preset->StaticCardEdgeDropPixels = 96.0f;
		Preset->FanYawDegrees = 8.0f;
		Preset->bClampCardLayerRenderAngle = true;
		Preset->MaxCardLayerRenderAngleDegrees = 20.0f;
		Preset->bEnableAnchorScreenSmoothing = true;
		Preset->AnchorScreenSmoothingSpeed = 5.0f;
		Preset->AnchorScreenSmoothingResetDistancePixels = 120.0f;
		Preset->bEnableCardSlotMotion = true;
		Preset->CardSlotMotionSpeed = 7.0f;
		Preset->CardSlotOpacitySpeed = 9.0f;
		Preset->CardSlotEnterOffsetPixels = FVector2D(11.0f, 22.0f);
		Preset->CardSlotEnterOpacity = 0.25f;
		Preset->CardSlotExitOffsetPixels = FVector2D(33.0f, 44.0f);
		Preset->CardSlotExitDuration = 0.31f;
		Preset->CardSlotMotionResetDistancePixels = 222.0f;
		Preset->bEnableEventAwareCardTransitions = true;
		Preset->bEnableReadableTransitionOrigins = true;
		Preset->DrawnCardEnterOffsetPixels = FVector2D(0.0f, 111.0f);
		Preset->DrawnCardEnterOriginMode = EWacomFirstPersonCardTransitionOriginMode::HandAnchorOffset;
		Preset->DrawnCardEnterViewportAnchor = FVector2D(0.25f, 1.0f);
		Preset->DrawnCardEnterScaleMultiplier = 0.82f;
		Preset->DrawnCardEnterAngleOffsetDegrees = -3.0f;
		Preset->GainedCardEnterOffsetPixels = FVector2D(0.0f, -133.0f);
		Preset->GainedCardEnterOriginMode = EWacomFirstPersonCardTransitionOriginMode::HandAnchorOffset;
		Preset->GainedCardEnterViewportAnchor = FVector2D(0.5f, 0.0f);
		Preset->GainedCardEnterScaleMultiplier = 0.84f;
		Preset->GainedCardEnterAngleOffsetDegrees = 4.0f;
		Preset->PlayedCardExitOffsetPixels = FVector2D(0.0f, -155.0f);
		Preset->PlayedCardExitOriginMode = EWacomFirstPersonCardTransitionOriginMode::SlotOffset;
		Preset->PlayedCardExitViewportAnchor = FVector2D(0.5f, 0.0f);
		Preset->PlayedCardExitScaleMultiplier = 0.86f;
		Preset->PlayedCardExitAngleOffsetDegrees = 5.0f;
		Preset->DiscardedCardExitOffsetPixels = FVector2D(0.0f, 177.0f);
		Preset->DiscardedCardExitOriginMode = EWacomFirstPersonCardTransitionOriginMode::SlotOffset;
		Preset->DiscardedCardExitViewportAnchor = FVector2D(0.5f, 1.0f);
		Preset->DiscardedCardExitScaleMultiplier = 0.88f;
		Preset->DiscardedCardExitAngleOffsetDegrees = -5.0f;
		Preset->PendingTargetingLiftPixels = 42.0f;
		Preset->PendingTargetingScale = 1.2f;
		Preset->PendingTargetingZOrderBoost = 1600;
		Preset->bPendingTargetingStraightenAngle = true;
		Preset->PendingTargetingAngleBlend = 0.5f;
		Preset->bEnableTargetSelectHandDeemphasis = true;
		Preset->TargetSelectNonPendingOpacityMultiplier = 0.6f;
		Preset->HandAnchorScale = 0.7f;
		Preset->DisabledRenderOpacity = 0.4f;
		Preset->HoverLiftPixels = 46.0f;
		Preset->HoverScale = 1.18f;
		Preset->HoverZOrderBoost = 900;
		Preset->bEnableCardInteractionFeedback = true;
		Preset->PlayableHoverFeedbackColor = FLinearColor::Green;
		Preset->PlayableHoverFeedbackOpacity = 0.3f;
		Preset->PressedFeedbackScale = 0.91f;
		Preset->PressedFeedbackColor = FLinearColor::Blue;
		Preset->PressedFeedbackOpacity = 0.2f;
		Preset->ConfirmFeedbackDuration = 0.12f;
		Preset->ConfirmFeedbackOpacity = 0.22f;
		Preset->DenyFeedbackDuration = 0.24f;
		Preset->DenyFeedbackShakePixels = 14.0f;
		Preset->DenyFeedbackColor = FLinearColor::Red;
		Preset->DenyFeedbackOpacity = 0.33f;
		return Preset;
	}

	FWacomFirstPersonCardLayerTransitionHint MakeTransitionHint(
		const FGuid& CardInstanceId,
		EWacomFirstPersonCardSlotTransitionKind TransitionKind)
	{
		FWacomFirstPersonCardLayerTransitionHint Hint;
		Hint.CardInstanceId = CardInstanceId;
		Hint.TransitionKind = TransitionKind;
		return Hint;
	}

	FBattleEvent MakeBattleEvent(
		EBattleEventType Type,
		const FGuid& CardInstanceId = FGuid(),
		int32 Count = 0)
	{
		FBattleEvent Event;
		Event.Type = Type;
		Event.CardInstanceId = CardInstanceId;
		Event.Count = Count;
		return Event;
	}

	const FWacomFirstPersonCardLayerTransitionHint* FindTransitionHint(
		const TArray<FWacomFirstPersonCardLayerTransitionHint>& Hints,
		const FGuid& CardInstanceId)
	{
		return Hints.FindByPredicate(
			[&CardInstanceId](const FWacomFirstPersonCardLayerTransitionHint& Hint)
			{
				return Hint.CardInstanceId == CardInstanceId;
			});
	}

	class FLayerInteractionReceiver
	{
	public:
		int32 ClickCount = 0;
		int32 HoverCount = 0;
		int32 UnhoverCount = 0;
		FGuid LastCardId;

		void HandleClicked(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView&)
		{
			++ClickCount;
			LastCardId = CardInstanceId;
		}

		void HandleHovered(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView&)
		{
			++HoverCount;
			LastCardId = CardInstanceId;
		}

		void HandleUnhovered(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView&)
		{
			++UnhoverCount;
			LastCardId = CardInstanceId;
		}
	};

	class FLayerLayoutUpdateReceiver
	{
	public:
		int32 UpdateCount = 0;
		FGuid LastCardId;
		FWacomFirstPersonCardLayerSlotView LastSlotView;

		void HandleUpdated(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView)
		{
			++UpdateCount;
			LastCardId = CardInstanceId;
			LastSlotView = SlotView;
		}
	};

	class FLayerCardTargetReceiver
	{
	public:
		int32 HoverCount = 0;
		int32 UnhoverCount = 0;
		int32 UpdateCount = 0;
		FWacomInteractionTargetHandle LastHandle;
		FWacomFirstPersonCardLayerSlotView LastSlotView;

		void HandleHovered(const FWacomInteractionTargetHandle& CardTargetHandle, const FWacomFirstPersonCardLayerSlotView& SlotView)
		{
			++HoverCount;
			LastHandle = CardTargetHandle;
			LastSlotView = SlotView;
		}

		void HandleUnhovered(const FWacomInteractionTargetHandle& CardTargetHandle, const FWacomFirstPersonCardLayerSlotView& SlotView)
		{
			++UnhoverCount;
			LastHandle = CardTargetHandle;
			LastSlotView = SlotView;
		}

		void HandleUpdated(const FWacomInteractionTargetHandle& CardTargetHandle, const FWacomFirstPersonCardLayerSlotView& SlotView)
		{
			++UpdateCount;
			LastHandle = CardTargetHandle;
			LastSlotView = SlotView;
		}
	};

	class FLayerDragReceiver
	{
	public:
		int32 StartedCount = 0;
		int32 UpdatedCount = 0;
		int32 ReleasedCount = 0;
		int32 CancelledCount = 0;
		FGuid LastCardId;
		FWacomFirstPersonCardDragView LastDragView;

		void HandleStarted(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView)
		{
			++StartedCount;
			LastCardId = CardInstanceId;
			LastDragView = DragView;
		}

		void HandleUpdated(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView)
		{
			++UpdatedCount;
			LastCardId = CardInstanceId;
			LastDragView = DragView;
		}

		void HandleReleased(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView)
		{
			++ReleasedCount;
			LastCardId = CardInstanceId;
			LastDragView = DragView;
		}

		void HandleCancelled(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView)
		{
			++CancelledCount;
			LastCardId = CardInstanceId;
			LastDragView = DragView;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerFallbackAnchorTest,
	"Wacom.UI.FirstPersonCardLayer.Anchor.FallbackAnchorUsesCameraTransform",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerFallbackAnchorTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	PC->Possess(Character);
	Anchor->ProbeCameraTransform = FTransform(
		FRotator(0.0f, 25.0f, 0.0f),
		FVector(100.0f, 200.0f, 300.0f),
		FVector::OneVector);
	Anchor->RefreshAnchor(0.0f);
	const FWacomFirstPersonCardAnchorDebugView View = Anchor->GetFirstPersonCardAnchorDebugView();
	TestTrue(TEXT("Fallback anchor is valid"), View.bHasValidAnchor);
	TestEqual(TEXT("No active run/battle uses camera fallback"), View.Mode, EWacomFirstPersonCardAnchorMode::CameraFallback);
	TestEqual(TEXT("Fallback reason is recorded"), View.LastFallbackReason, FName(TEXT("CameraFallback")));

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerRunTunnelAnchorTest,
	"Wacom.UI.FirstPersonCardLayer.Anchor.RunTunnelAnchorUsesSplineBase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerRunTunnelAnchorTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	AWacomRunTunnelSegmentActor* Segment = WacomFirstPersonCardLayerSpec::SpawnTestSegment(
		*World,
		FVector::ZeroVector,
		FVector(1000.0f, 0.0f, 0.0f));
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Segment"), Segment)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	UWacomRunTunnelMovementComponent* RunTunnel = Character->GetRunTunnelMovementComponent();
	TestTrue(TEXT("Run tunnel activates"), RunTunnel && RunTunnel->ActivateRunTunnel(Segment, 200.0f));
	Anchor->RefreshAnchor(0.0f);
	const FWacomFirstPersonCardAnchorDebugView View = Anchor->GetFirstPersonCardAnchorDebugView();
	TestTrue(TEXT("Run tunnel anchor is valid"), View.bHasValidAnchor);
	TestEqual(TEXT("Run tunnel mode is selected"), View.Mode, EWacomFirstPersonCardAnchorMode::RunTunnel);
	TestEqual(TEXT("Run tunnel anchor uses spline distance before layout offset"), Character->GetRunTunnelMovementComponent()->GetDistanceAlongSpline(), 200.0f);
	TestEqual(TEXT("Run tunnel default projection is body locked"), View.ProjectionMode, EWacomFirstPersonCardProjectionMode::BodyLocked);
	TestFalse(TEXT("Run tunnel body locked layout ignores cursor look"), View.bLookOffsetAppliedToLayout);

	Anchor->DestroyComponent();
	Segment->Destroy();
	Character->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerSlotCardTargetHandleTest,
	"Wacom.UI.FirstPersonCardLayer.CardTarget.SlotBuildsCardTargetHandleFromVisualPosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerSlotCardTargetHandleTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView TargetSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(300.0f, 400.0f));
	TargetSlot.ScreenPosition = FVector2D(500.0f, 600.0f);
	TargetSlot.WidgetPosition = TargetSlot.ScreenPosition;
	TargetSlot.SnappedWidgetPosition = TargetSlot.ScreenPosition;
	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.EnterOffsetPixels = FVector2D(0.0f, 80.0f);
	Config.EnterOpacity = 1.0f;
	SlotWidget->SetSlotMotionConfig(Config);
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->BeginSlotMotion(TargetSlot, true);

	const FWacomInteractionTargetHandle Handle = SlotWidget->BuildCardTargetHandle();
	TestTrue(TEXT("Active visible slot exposes card target"), Handle.IsValid());
	TestEqual(TEXT("Card target kind"), Handle.TargetKind, EWacomInteractionTargetKind::Card);
	TestEqual(TEXT("Card target preserves id"), Handle.CardInstanceId, CardId);
	TestTrue(TEXT("Card target source is slot widget"), Handle.SourceObject.Get() == SlotWidget);
	TestEqual(TEXT("Card target uses visual position, not target position"),
		Handle.ScreenPosition, FVector2D(500.0f, 680.0f));

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerSlotInvalidCardTargetTest,
	"Wacom.UI.FirstPersonCardLayer.CardTarget.NonProjectedOrMissingCardIdReturnsInvalidCardTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerSlotInvalidCardTargetTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeMotionSlot(FGuid::NewGuid(), 0, FVector2D(100.0f, 100.0f)));
	TestTrue(TEXT("Valid projected slot starts valid"), SlotWidget->BuildCardTargetHandle().IsValid());

	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid(), true, false));
	TestFalse(TEXT("Non-projected slot returns invalid target"), SlotWidget->BuildCardTargetHandle().IsValid());

	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid(), true, true));
	TestFalse(TEXT("Missing CardInstanceId returns invalid target"), SlotWidget->BuildCardTargetHandle().IsValid());

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerSlotExitingCardTargetTest,
	"Wacom.UI.FirstPersonCardLayer.CardTarget.ExitingSlotDoesNotExposeCardTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerSlotExitingCardTargetTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	const FGuid CardId = FGuid::NewGuid();
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(100.0f, 100.0f)));
	TestTrue(TEXT("Slot target valid before exit"), SlotWidget->BuildCardTargetHandle().IsValid());
	SlotWidget->BeginExitMotion(SlotWidget->GetVisualSlotView());
	TestFalse(TEXT("Exiting slot target is invalid"), SlotWidget->BuildCardTargetHandle().IsValid());

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerSlotNonPlayableCardTargetTest,
	"Wacom.UI.FirstPersonCardLayer.CardTarget.CardTargetDoesNotRequirePlayableCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerSlotNonPlayableCardTargetTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	const FGuid CardId = FGuid::NewGuid();
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, false, true));

	const FWacomInteractionTargetHandle Handle = SlotWidget->BuildCardTargetHandle();
	TestTrue(TEXT("Non-playable visible slot still exposes card target"), Handle.IsValid());
	TestEqual(TEXT("Non-playable target preserves card id"), Handle.CardInstanceId, CardId);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerCardTargetHoverBridgeTest,
	"Wacom.UI.FirstPersonCardLayer.CardTarget.LayerBroadcastsCardTargetHoverAndUnhover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerCardTargetHoverBridgeTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	const FGuid CardId = FGuid::NewGuid();
	Layer->SetCardLayerInteractionEnabled(true);
	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(320.0f, 420.0f))
	});
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	WacomFirstPersonCardLayerSpec::FLayerCardTargetReceiver Receiver;
	Layer->OnCardTargetHoveredNative.AddRaw(
		&Receiver,
		&WacomFirstPersonCardLayerSpec::FLayerCardTargetReceiver::HandleHovered);
	Layer->OnCardTargetUnhoveredNative.AddRaw(
		&Receiver,
		&WacomFirstPersonCardLayerSpec::FLayerCardTargetReceiver::HandleUnhovered);

	TestTrue(TEXT("Slot hover succeeds"), SlotWidget->RequestHoverForTest());
	TestEqual(TEXT("Card target hover broadcasts once"), Receiver.HoverCount, 1);
	TestEqual(TEXT("Hovered target id"), Receiver.LastHandle.CardInstanceId, CardId);
	TestEqual(TEXT("Layer stores hovered target"), Layer->BuildHoveredCardTargetHandle().CardInstanceId, CardId);
	TestEqual(TEXT("Hovered target uses visual position"), Receiver.LastHandle.ScreenPosition, SlotWidget->GetVisualSlotView().ScreenPosition);

	SlotWidget->RequestUnhoverForTest();
	TestEqual(TEXT("Card target unhover broadcasts once"), Receiver.UnhoverCount, 1);
	TestFalse(TEXT("Layer clears hovered target"), Layer->BuildHoveredCardTargetHandle().IsValid());

	Layer->OnCardTargetHoveredNative.RemoveAll(&Receiver);
	Layer->OnCardTargetUnhoveredNative.RemoveAll(&Receiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerCardTargetVisualUpdateTest,
	"Wacom.UI.FirstPersonCardLayer.CardTarget.LayerBroadcastsHoveredCardTargetVisualUpdate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerCardTargetVisualUpdateTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.EnterOffsetPixels = FVector2D::ZeroVector;
	Config.EnterOpacity = 1.0f;
	Layer->SetSlotMotionConfig(Config);
	Layer->SetCardLayerInteractionEnabled(true);
	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView BaseSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(100.0f, 200.0f));
	BaseSlot.bIsHovered = true;
	Layer->SetCardSlots({ BaseSlot });
	Layer->TickSlotMotionForTest(1.0f);

	WacomFirstPersonCardLayerSpec::FLayerCardTargetReceiver Receiver;
	Layer->OnHoveredCardTargetUpdatedNative.AddRaw(
		&Receiver,
		&WacomFirstPersonCardLayerSpec::FLayerCardTargetReceiver::HandleUpdated);

	FWacomFirstPersonCardLayerSlotView MovedSlot = BaseSlot;
	MovedSlot.ScreenPosition = FVector2D(240.0f, 220.0f);
	MovedSlot.WidgetPosition = MovedSlot.ScreenPosition;
	MovedSlot.SnappedWidgetPosition = MovedSlot.ScreenPosition;
	Layer->SetCardSlots({ MovedSlot });

	TestEqual(TEXT("Hovered card target visual update broadcasts"), Receiver.UpdateCount, 1);
	TestEqual(TEXT("Updated handle keeps card id"), Receiver.LastHandle.CardInstanceId, CardId);
	TestEqual(TEXT("Updated handle uses visual slot position"), Receiver.LastHandle.ScreenPosition, Receiver.LastSlotView.ScreenPosition);
	TestTrue(TEXT("Visual target update happens during interpolation"), Receiver.LastHandle.ScreenPosition.X < 240.0f);

	Layer->OnHoveredCardTargetUpdatedNative.RemoveAll(&Receiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerClearCardTargetStateTest,
	"Wacom.UI.FirstPersonCardLayer.CardTarget.LayerClearRemovesHoveredCardTargetState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerClearCardTargetStateTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	const FGuid CardId = FGuid::NewGuid();
	Layer->SetCardLayerInteractionEnabled(true);
	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(320.0f, 420.0f))
	});
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}
	TestTrue(TEXT("Slot hover succeeds"), SlotWidget->RequestHoverForTest());
	TestTrue(TEXT("Layer has hovered card target"), Layer->BuildHoveredCardTargetHandle().IsValid());

	WacomFirstPersonCardLayerSpec::FLayerCardTargetReceiver Receiver;
	Layer->OnCardTargetUnhoveredNative.AddRaw(
		&Receiver,
		&WacomFirstPersonCardLayerSpec::FLayerCardTargetReceiver::HandleUnhovered);
	Layer->ClearSlotMotionState();
	TestFalse(TEXT("Layer clear removes hovered card target"), Layer->BuildHoveredCardTargetHandle().IsValid());
	TestEqual(TEXT("Layer clear broadcasts target unhover"), Receiver.UnhoverCount, 1);

	Layer->OnCardTargetUnhoveredNative.RemoveAll(&Receiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerBattlePriorityTest,
	"Wacom.UI.FirstPersonCardLayer.Anchor.BattleAnchorUsesBattleBaseRotation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerBattlePriorityTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	AWacomRunTunnelSegmentActor* Segment = WacomFirstPersonCardLayerSpec::SpawnTestSegment(
		*World,
		FVector::ZeroVector,
		FVector(1000.0f, 0.0f, 0.0f));
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Segment"), Segment)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	PC->Possess(Character);
	TestTrue(TEXT("Run tunnel activates"), Character->GetRunTunnelMovementComponent()->ActivateRunTunnel(Segment, 100.0f));
	Character->SetExplorationInputEnabled(false);
	PC->SetControlRotation(FRotator(2.0f, 55.0f, 0.0f));
	TestTrue(TEXT("Battle camera activates"), Character->GetBattleCameraLookComponent()->ActivateBattleCameraLook());
	Anchor->RefreshAnchor(0.0f);
	const FWacomFirstPersonCardAnchorDebugView View = Anchor->GetFirstPersonCardAnchorDebugView();
	TestEqual(TEXT("Battle camera mode takes priority over suspended run tunnel"), View.Mode, EWacomFirstPersonCardAnchorMode::BattleCamera);
	TestEqual(TEXT("Battle base yaw is used"), View.AnchorTransform.Rotator().Yaw, 55.0);
	TestEqual(TEXT("Battle default projection is body locked"), View.ProjectionMode, EWacomFirstPersonCardProjectionMode::BodyLocked);
	TestFalse(TEXT("Battle body locked layout ignores cursor look"), View.bLookOffsetAppliedToLayout);

	Character->GetBattleCameraLookComponent()->DeactivateBattleCameraLook();
	Anchor->DestroyComponent();
	Segment->Destroy();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerPartialLookTest,
	"Wacom.UI.FirstPersonCardLayer.Anchor.LegacyPartialLookInfluence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerPartialLookTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	PC->Possess(Character);
	Anchor->ProbeCameraTransform = FTransform(
		FRotator::ZeroRotator,
		FVector(100.0f, 200.0f, 300.0f),
		FVector::OneVector);
	Anchor->LookInfluenceYaw = 0.25f;
	Anchor->LookInfluencePitch = 0.5f;
	Anchor->ProjectionMode = EWacomFirstPersonCardProjectionMode::LegacyWorldProjected;
	Character->GetCursorLookDriverComponent()->UpdateFromNormalizedCursor(
		FVector2D(1.0f, -1.0f),
		0.0f,
		20.0f,
		10.0f);

	Anchor->RefreshAnchor(0.0f);
	const FWacomFirstPersonCardAnchorDebugView View = Anchor->GetFirstPersonCardAnchorDebugView();
	TestEqual(TEXT("Yaw uses partial cursor influence"), View.LookOffsetUsed.Yaw, 5.0);
	TestEqual(TEXT("Pitch uses partial cursor influence"), View.LookOffsetUsed.Pitch, 5.0);
	TestTrue(TEXT("Legacy projection reports look used for layout"), View.bLookOffsetAppliedToLayout);

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerBodyLockedWorldLayoutTest,
	"Wacom.UI.FirstPersonCardLayer.Projection.BodyLockedKeepsWorldLayoutStableUnderCursorLook",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerBodyLockedWorldLayoutTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	Anchor->ProjectionPadding = 0.0f;
	Anchor->StaticCardCountFallback = 5;
	Anchor->StaticCardEdgeDropPixels = 0.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	Anchor->CardLayoutMode = EWacomFirstPersonCardLayoutMode::LegacyProjectedFan2D;
	Anchor->ProjectionMode = EWacomFirstPersonCardProjectionMode::BodyLocked;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	const FTransform InitialLeft = Anchor->ComputeCardTransform(5, 0);
	const FTransform InitialCenter = Anchor->ComputeCardTransform(5, 2);
	const FTransform InitialRight = Anchor->ComputeCardTransform(5, 4);

	Character->GetCursorLookDriverComponent()->UpdateFromNormalizedCursor(
		FVector2D(1.0f, -1.0f),
		0.0f,
		20.0f,
		10.0f);
	Anchor->RefreshAnchor(0.0f);
	const FTransform LookLeft = Anchor->ComputeCardTransform(5, 0);
	const FTransform LookCenter = Anchor->ComputeCardTransform(5, 2);
	const FTransform LookRight = Anchor->ComputeCardTransform(5, 4);
	const TArray<FWacomFirstPersonCardLayerSlotView> LookSlots = Anchor->BuildStaticCardSlotViews();

	TestEqual(TEXT("Look slot count"), LookSlots.Num(), 5);
	TestTrue(TEXT("Body locked left world location stays stable"), LookLeft.GetLocation().Equals(InitialLeft.GetLocation(), KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Body locked center world location stays stable"), LookCenter.GetLocation().Equals(InitialCenter.GetLocation(), KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Body locked right world location stays stable"), LookRight.GetLocation().Equals(InitialRight.GetLocation(), KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Body locked center world rotation stays stable"), LookCenter.Rotator().Equals(InitialCenter.Rotator(), KINDA_SMALL_NUMBER));
	TestEqual(
		TEXT("Body locked fan spacing remains stable"),
		(LookRight.GetLocation() - LookCenter.GetLocation()).Size(),
		(InitialRight.GetLocation() - InitialCenter.GetLocation()).Size());
	if (LookSlots.Num() == 5)
	{
		TestFalse(TEXT("Body locked does not apply look to layout"), LookSlots[2].bLookOffsetAppliedToLayout);
		TestTrue(TEXT("Body locked layout flag is recorded"), LookSlots[2].bBodyLockedLayout);
		TestTrue(TEXT("Current camera projection flag is recorded"), LookSlots[2].bCurrentCameraProjection);
	}

	const FWacomFirstPersonCardAnchorDebugView View = Anchor->GetFirstPersonCardAnchorDebugView(1);
	TestEqual(TEXT("Debug projection mode is BodyLocked"), View.ProjectionMode, EWacomFirstPersonCardProjectionMode::BodyLocked);
	TestTrue(TEXT("Debug reports body locked layout"), View.bBodyLockedLayout);
	TestTrue(TEXT("Debug reports current camera projection"), View.bCurrentCameraProjection);
	TestFalse(TEXT("Debug reports look is not used for layout"), View.bLookOffsetAppliedToLayout);

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerBodyLockedCurrentCameraProjectionTest,
	"Wacom.UI.FirstPersonCardLayer.Projection.BodyLockedProjectsThroughCurrentCamera",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerBodyLockedCurrentCameraProjectionTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	Anchor->ProjectionPadding = 0.0f;
	Anchor->StaticCardCountFallback = 5;
	Anchor->StaticCardEdgeDropPixels = 0.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	Anchor->bUseCameraTransformProjection = true;
	Anchor->CardLayoutMode = EWacomFirstPersonCardLayoutMode::LegacyProjectedFan2D;
	Anchor->ProjectionMode = EWacomFirstPersonCardProjectionMode::BodyLocked;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	Anchor->ProbeCameraTransform = FTransform(
		FRotator::ZeroRotator,
		Anchor->ProbeCameraTransform.GetLocation(),
		FVector::OneVector);
	const TArray<FWacomFirstPersonCardLayerSlotView> InitialSlots = Anchor->BuildStaticCardSlotViews();

	Anchor->ProbeCameraTransform = FTransform(
		FRotator(0.0f, 8.0f, 0.0f),
		Anchor->ProbeCameraTransform.GetLocation(),
		FVector::OneVector);
	const TArray<FWacomFirstPersonCardLayerSlotView> RotatedCameraSlots = Anchor->BuildStaticCardSlotViews();

	TestEqual(TEXT("Initial slot count"), InitialSlots.Num(), 5);
	TestEqual(TEXT("Rotated camera slot count"), RotatedCameraSlots.Num(), 5);
	if (InitialSlots.Num() == 5 && RotatedCameraSlots.Num() == 5)
	{
		TestNotEqual(TEXT("Body locked layout still projects through current camera"), RotatedCameraSlots[2].ScreenPosition, InitialSlots[2].ScreenPosition);
		TestFalse(TEXT("Current camera projection does not mean look is used for layout"), RotatedCameraSlots[2].bLookOffsetAppliedToLayout);
		TestTrue(TEXT("Body locked layout flag is recorded"), RotatedCameraSlots[2].bBodyLockedLayout);
		TestTrue(TEXT("Current camera projection flag is recorded"), RotatedCameraSlots[2].bCurrentCameraProjection);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerBodyLockedFanShapeTest,
	"Wacom.UI.FirstPersonCardLayer.Projection.BodyLockedPreservesFanShapeUnderCameraLook",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerBodyLockedFanShapeTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	Anchor->StaticCardCountFallback = 5;
	Anchor->ProjectionMode = EWacomFirstPersonCardProjectionMode::BodyLocked;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	TArray<FVector> InitialLocations;
	TArray<float> InitialYaws;
	for (int32 Index = 0; Index < 5; ++Index)
	{
		const FTransform Transform = Anchor->ComputeCardTransform(5, Index);
		InitialLocations.Add(Transform.GetLocation());
		InitialYaws.Add(Transform.Rotator().Yaw);
	}

	Character->GetCursorLookDriverComponent()->UpdateFromNormalizedCursor(
		FVector2D(1.0f, -1.0f),
		0.0f,
		20.0f,
		10.0f);
	Anchor->RefreshAnchor(0.0f);

	for (int32 Index = 0; Index < 5; ++Index)
	{
		const FTransform Transform = Anchor->ComputeCardTransform(5, Index);
		TestTrue(
			FString::Printf(TEXT("Slot %d world location remains in same fan"), Index),
			Transform.GetLocation().Equals(InitialLocations[Index], KINDA_SMALL_NUMBER));
		TestTrue(
			FString::Printf(TEXT("Slot %d yaw remains in same fan"), Index),
			FMath::IsNearlyEqual(Transform.Rotator().Yaw, InitialYaws[Index], KINDA_SMALL_NUMBER));
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerLegacyLookProjectionTest,
	"Wacom.UI.FirstPersonCardLayer.Projection.LegacyWorldProjectedStillAppliesLookInfluence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerLegacyLookProjectionTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	Anchor->ProjectionPadding = 0.0f;
	Anchor->StaticCardCountFallback = 5;
	Anchor->StaticCardEdgeDropPixels = 0.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	Anchor->CardLayoutMode = EWacomFirstPersonCardLayoutMode::LegacyProjectedFan2D;
	Anchor->ProjectionMode = EWacomFirstPersonCardProjectionMode::LegacyWorldProjected;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	const FTransform InitialCenterTransform = Anchor->ComputeCardTransform(5, 2);
	const TArray<FWacomFirstPersonCardLayerSlotView> InitialSlots = Anchor->BuildStaticCardSlotViews();

	Character->GetCursorLookDriverComponent()->UpdateFromNormalizedCursor(
		FVector2D(1.0f, -1.0f),
		0.0f,
		20.0f,
		10.0f);
	Anchor->RefreshAnchor(0.0f);
	const FTransform LookCenterTransform = Anchor->ComputeCardTransform(5, 2);
	const TArray<FWacomFirstPersonCardLayerSlotView> LookSlots = Anchor->BuildStaticCardSlotViews();

	TestEqual(TEXT("Initial slot count"), InitialSlots.Num(), 5);
	TestEqual(TEXT("Look slot count"), LookSlots.Num(), 5);
	TestFalse(TEXT("Legacy look influence changes center world location"), LookCenterTransform.GetLocation().Equals(InitialCenterTransform.GetLocation(), KINDA_SMALL_NUMBER));
	if (InitialSlots.Num() == 5 && LookSlots.Num() == 5)
	{
		TestNotEqual(TEXT("Legacy world-projected center card can move with look"), LookSlots[2].ScreenPosition, InitialSlots[2].ScreenPosition);
		TestTrue(TEXT("Legacy projection reports look used for layout"), LookSlots[2].bLookOffsetAppliedToLayout);
		TestFalse(TEXT("Legacy body locked layout flag is false"), LookSlots[2].bBodyLockedLayout);
		TestTrue(TEXT("Legacy still uses current camera projection"), LookSlots[2].bCurrentCameraProjection);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerFallbackStaticCardsTest,
	"Wacom.UI.FirstPersonCardLayer.StaticLayer.CreatesCardViewsFromFallbackData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerFallbackStaticCardsTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	Anchor->StaticCardCountFallback = 5;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildStaticCardSlotViews();
	TestEqual(TEXT("Fallback creates five static slots"), Slots.Num(), 5);
	if (Slots.Num() > 0)
	{
		TestEqual(TEXT("Fallback card has placeholder name"), Slots[0].Entry.CardViewData.Name.ToString(), FString(TEXT("Anchor Card 1")));
		TestTrue(TEXT("Fallback slot is projected"), Slots[0].bProjected);
	}

	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (TestNotNull(TEXT("Layer widget"), Layer))
	{
		Layer->SetStaticCardSlots(Slots);
		TestEqual(TEXT("Layer creates card views"), Layer->GetCardViewCount(), 5);
		TestTrue(TEXT("First card is visible"), Layer->IsCardSlotVisible(0));
		TestNotNull(TEXT("Layer uses card view widgets"), Layer->GetCardViewAt(0));
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDefinitionStaticCardsTest,
	"Wacom.UI.FirstPersonCardLayer.StaticLayer.BuildsCardViewsFromDefinitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDefinitionStaticCardsTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	UCardDefinition* FirstCard = WacomFirstPersonCardLayerSpec::MakePreviewCard(GetTransientPackage(), TEXT("Preview.Alpha"), 2);
	UCardDefinition* SecondCard = WacomFirstPersonCardLayerSpec::MakePreviewCard(GetTransientPackage(), TEXT("Preview.Beta"), 4);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor)
		|| !TestNotNull(TEXT("First preview card"), FirstCard)
		|| !TestNotNull(TEXT("Second preview card"), SecondCard))
	{
		return false;
	}

	Anchor->StaticPreviewCardDefinitions = {
		TSoftObjectPtr<UCardDefinition>(FirstCard),
		TSoftObjectPtr<UCardDefinition>(SecondCard)
	};
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildStaticCardSlotViews();
	TestEqual(TEXT("Definitions choose slot count"), Slots.Num(), 2);
	if (Slots.Num() == 2)
	{
		TestEqual(TEXT("First definition name is used"), Slots[0].Entry.CardViewData.Name.ToString(), FString(TEXT("Preview.Alpha")));
		TestEqual(TEXT("Second definition cost is used"), Slots[1].Entry.CardViewData.Cost, 4);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerWidgetProjectionTest,
	"Wacom.UI.FirstPersonCardLayer.RenderQuality.WidgetProjectionProducesLayoutPosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerWidgetProjectionTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	Anchor->ProjectionPadding = 0.0f;
	Anchor->ProbeViewportScale = 2.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	Anchor->CardLayoutMode = EWacomFirstPersonCardLayoutMode::LegacyProjectedFan2D;
	Anchor->ProjectionMode = EWacomFirstPersonCardProjectionMode::LegacyWorldProjected;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildStaticCardSlotViews();
	TestEqual(TEXT("Slot count"), Slots.Num(), 5);
	if (Slots.Num() == 5)
	{
		TestEqual(TEXT("Raw screen position is preserved"), Slots[2].RawScreenPosition, FVector2D(1160.0f, 310.0f));
		TestEqual(TEXT("Widget position is DPI-aware"), Slots[2].WidgetPosition, FVector2D(580.0f, 155.0f));
		TestEqual(TEXT("Final screen position uses widget-space layout"), Slots[2].ScreenPosition, Slots[2].WidgetPosition);
		TestEqual(TEXT("Viewport scale is recorded"), Slots[2].ViewportScale, 2.0f);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerPixelSnappingTest,
	"Wacom.UI.FirstPersonCardLayer.RenderQuality.PixelSnappingRoundsFinalPosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerPixelSnappingTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	Anchor->ProjectionPadding = 0.0f;
	Anchor->ProbeViewportScale = 3.0f;
	Anchor->bEnableCardLayerPixelSnapping = true;
	Anchor->CardLayerPixelSnapGrid = 1.0f;
	Anchor->StaticCardCountFallback = 1;
	Anchor->StaticCardEdgeDropPixels = 0.0f;
	Anchor->CardLayoutMode = EWacomFirstPersonCardLayoutMode::LegacyProjectedFan2D;
	Anchor->ProjectionMode = EWacomFirstPersonCardProjectionMode::LegacyWorldProjected;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildStaticCardSlotViews();
	TestEqual(TEXT("Slot count"), Slots.Num(), 1);
	if (Slots.Num() == 1)
	{
		TestEqual(TEXT("Final widget position is snapped"), Slots[0].ScreenPosition, FVector2D(387.0f, 103.0f));
		TestTrue(TEXT("Unsnapped X keeps fractional layout"), FMath::IsNearlyEqual(Slots[0].WidgetPosition.X, 386.6667f, 0.001f));
		TestTrue(TEXT("Unsnapped Y keeps fractional layout"), FMath::IsNearlyEqual(Slots[0].WidgetPosition.Y, 103.3333f, 0.001f));
		TestEqual(TEXT("Snapped position is recorded"), Slots[0].SnappedWidgetPosition, Slots[0].ScreenPosition);
		TestTrue(TEXT("Pixel snap flag records changed position"), Slots[0].bPixelSnapped);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerPixelSnappingDisabledTest,
	"Wacom.UI.FirstPersonCardLayer.RenderQuality.PixelSnappingCanBeDisabled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerPixelSnappingDisabledTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	Anchor->ProjectionPadding = 0.0f;
	Anchor->ProbeViewportScale = 3.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	Anchor->StaticCardCountFallback = 1;
	Anchor->StaticCardEdgeDropPixels = 0.0f;
	Anchor->CardLayoutMode = EWacomFirstPersonCardLayoutMode::LegacyProjectedFan2D;
	Anchor->ProjectionMode = EWacomFirstPersonCardProjectionMode::LegacyWorldProjected;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildStaticCardSlotViews();
	TestEqual(TEXT("Slot count"), Slots.Num(), 1);
	if (Slots.Num() == 1)
	{
		TestTrue(TEXT("Final X keeps fractional layout"), FMath::IsNearlyEqual(Slots[0].ScreenPosition.X, 386.6667f, 0.001f));
		TestTrue(TEXT("Final Y keeps fractional layout"), FMath::IsNearlyEqual(Slots[0].ScreenPosition.Y, 103.3333f, 0.001f));
		TestFalse(TEXT("Pixel snap flag remains false"), Slots[0].bPixelSnapped);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerRenderAngleClampTest,
	"Wacom.UI.FirstPersonCardLayer.RenderQuality.RenderAngleClampLimitsEdgeCards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerRenderAngleClampTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	Anchor->FanYawDegrees = 6.0f;
	Anchor->MaxCardLayerRenderAngleDegrees = 4.0f;
	Anchor->bClampCardLayerRenderAngle = true;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildStaticCardSlotViews();
	TestEqual(TEXT("Slot count"), Slots.Num(), 5);
	if (Slots.Num() == 5)
	{
		TestEqual(TEXT("Left edge angle is clamped"), Slots[0].RenderAngleDegrees, -4.0f);
		TestEqual(TEXT("Inner left angle is clamped"), Slots[1].RenderAngleDegrees, -4.0f);
		TestEqual(TEXT("Center remains straight"), Slots[2].RenderAngleDegrees, 0.0f);
		TestEqual(TEXT("Right edge angle is clamped"), Slots[4].RenderAngleDegrees, 4.0f);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerRenderAngleClampDisabledTest,
	"Wacom.UI.FirstPersonCardLayer.RenderQuality.RenderAngleClampCanBeDisabled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerRenderAngleClampDisabledTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	Anchor->FanYawDegrees = 6.0f;
	Anchor->MaxCardLayerRenderAngleDegrees = 4.0f;
	Anchor->bClampCardLayerRenderAngle = false;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildStaticCardSlotViews();
	TestEqual(TEXT("Slot count"), Slots.Num(), 5);
	if (Slots.Num() == 5)
	{
		TestEqual(TEXT("Left edge angle uses full fan"), Slots[0].RenderAngleDegrees, -12.0f);
		TestEqual(TEXT("Right edge angle uses full fan"), Slots[4].RenderAngleDegrees, 12.0f);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDebugProjectionQualityTest,
	"Wacom.UI.FirstPersonCardLayer.RenderQuality.DebugViewReportsProjectionQuality",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDebugProjectionQualityTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	Anchor->ProjectionPadding = 0.0f;
	Anchor->ProbeViewportScale = 2.0f;
	Anchor->bEnableCardLayerPixelSnapping = true;
	Anchor->StaticCardEdgeDropPixels = 0.0f;
	Anchor->CardLayoutMode = EWacomFirstPersonCardLayoutMode::LegacyProjectedFan2D;
	Anchor->ProjectionMode = EWacomFirstPersonCardProjectionMode::LegacyWorldProjected;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const FWacomFirstPersonCardAnchorDebugView View = Anchor->GetFirstPersonCardAnchorDebugView(1);
	TestEqual(TEXT("One debug point"), View.ProjectedPoints.Num(), 1);
	if (View.ProjectedPoints.Num() == 1)
	{
		const FWacomFirstPersonCardProjectedPoint& Point = View.ProjectedPoints[0];
		TestTrue(TEXT("Debug point projected"), Point.bProjected);
		TestEqual(TEXT("Debug raw screen position"), Point.RawScreenPosition, FVector2D(1160.0f, 310.0f));
		TestEqual(TEXT("Debug widget position"), Point.WidgetPosition, FVector2D(580.0f, 155.0f));
		TestEqual(TEXT("Debug snapped position"), Point.SnappedWidgetPosition, FVector2D(580.0f, 155.0f));
		TestEqual(TEXT("Debug viewport scale"), Point.ViewportScale, 2.0f);
		TestEqual(TEXT("Debug point records layout mode"), Point.LayoutMode, EWacomFirstPersonCardLayoutMode::LegacyProjectedFan2D);
	}

	const FString Summary = Anchor->GetDebugSummary();
	TestTrue(TEXT("Summary reports pixel snap"), Summary.Contains(TEXT("PixelSnap=true")));
	TestTrue(TEXT("Summary reports angle clamp"), Summary.Contains(TEXT("AngleClamp=true")));
	TestTrue(TEXT("Summary reports viewport scale"), Summary.Contains(TEXT("ViewportScale=2.00")));
	TestTrue(TEXT("Summary reports projection mode"), Summary.Contains(TEXT("ProjectionMode=LegacyWorldProjected")));
	TestTrue(TEXT("Summary reports layout mode"), Summary.Contains(TEXT("LayoutMode=LegacyProjectedFan2D")));

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerStaticSlotOrderTest,
	"Wacom.UI.FirstPersonCardLayer.StaticLayer.ProjectedSlotsStayOrdered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerStaticSlotOrderTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	Anchor->StaticCardCountFallback = 5;
	Anchor->StaticCardEdgeDropPixels = 80.0f;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildStaticCardSlotViews();
	TestEqual(TEXT("Slot count"), Slots.Num(), 5);
	if (Slots.Num() == 5)
	{
		TestTrue(TEXT("Left slot screen X is before center"), Slots[0].ScreenPosition.X < Slots[2].ScreenPosition.X);
		TestTrue(TEXT("Right slot screen X is after center"), Slots[4].ScreenPosition.X > Slots[2].ScreenPosition.X);
		TestTrue(TEXT("Left edge drops lower than center"), Slots[0].ScreenPosition.Y > Slots[2].ScreenPosition.Y);
		TestTrue(TEXT("Right edge drops lower than center"), Slots[4].ScreenPosition.Y > Slots[2].ScreenPosition.Y);
		TestEqual(TEXT("Center card has no fan angle"), Slots[2].RenderAngleDegrees, 0.0f);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerPresetLayoutOverrideTest,
	"Wacom.UI.FirstPersonCardLayer.LayoutPreset.PresetOverridesComponentLayoutValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerPresetLayoutOverrideTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	UWacomFirstPersonCardLayoutPreset* Preset = WacomFirstPersonCardLayerSpec::MakeLayoutPreset(GetTransientPackage());
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor)
		|| !TestNotNull(TEXT("Preset"), Preset))
	{
		return false;
	}

	Anchor->AuthoredCardSpacingPixels = 40.0f;
	Anchor->AuthoredMaxHandWidthPixels = 0.0f;
	Anchor->StaticCardEdgeDropPixels = 4.0f;
	Anchor->FanYawDegrees = 1.0f;
	Anchor->StaticCardRenderScale = 0.5f;
	Anchor->bUseFirstPersonCardLayoutPreset = true;
	Anchor->FirstPersonCardLayoutPreset = Preset;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	TArray<FWacomFirstPersonCardLayerEntry> Entries;
	Entries.SetNum(5);
	Anchor->SetRuntimeCardLayerEntries(TEXT("PresetLayout"), Entries);
	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildActiveCardLayerSlotViewsForTest();

	TestEqual(TEXT("Preset slot count"), Slots.Num(), 5);
	if (Slots.Num() == 5)
	{
		TestEqual(TEXT("Preset spacing applies"), Slots[3].AuthoredLayoutOffset.X - Slots[2].AuthoredLayoutOffset.X, 160.0);
		TestEqual(TEXT("Preset hand width clamps spacing"), Slots.Last().AuthoredLayoutOffset.X - Slots[0].AuthoredLayoutOffset.X, 640.0);
		TestEqual(TEXT("Preset edge drop applies"), Slots[0].AuthoredLayoutOffset.Y, Preset->StaticCardEdgeDropPixels + Preset->AuthoredHandScreenOffset.Y);
		TestEqual(TEXT("Preset scale applies"), Slots[2].RenderScale, Preset->StaticCardRenderScale);
		TestEqual(TEXT("Preset fan angle applies"), Slots.Last().RenderAngleDegrees, 16.0f);
	}
	TestTrue(TEXT("Preset is active"), Anchor->IsUsingResolvedCardLayoutPresetForTest());

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerPresetFallbackTest,
	"Wacom.UI.FirstPersonCardLayer.LayoutPreset.PresetCanBeDisabledToUseComponentFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerPresetFallbackTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	UWacomFirstPersonCardLayoutPreset* Preset = WacomFirstPersonCardLayerSpec::MakeLayoutPreset(GetTransientPackage());
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor)
		|| !TestNotNull(TEXT("Preset"), Preset))
	{
		return false;
	}

	Anchor->AuthoredCardSpacingPixels = 80.0f;
	Anchor->AuthoredMaxHandWidthPixels = 0.0f;
	Anchor->StaticCardRenderScale = 0.66f;
	Anchor->FanYawDegrees = 2.0f;
	Anchor->bClampCardLayerRenderAngle = false;
	Anchor->FirstPersonCardLayoutPreset = Preset;
	Anchor->bUseFirstPersonCardLayoutPreset = false;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	TArray<FWacomFirstPersonCardLayerEntry> Entries;
	Entries.SetNum(3);
	Anchor->SetRuntimeCardLayerEntries(TEXT("PresetFallback"), Entries);
	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildActiveCardLayerSlotViewsForTest();

	if (Slots.Num() == 3)
	{
		TestEqual(TEXT("Component spacing is fallback"), Slots[2].AuthoredLayoutOffset.X - Slots[1].AuthoredLayoutOffset.X, 80.0);
		TestEqual(TEXT("Component scale is fallback"), Slots[1].RenderScale, 0.66f);
		TestEqual(TEXT("Component angle is fallback"), Slots[2].RenderAngleDegrees, 2.0f);
	}
	TestFalse(TEXT("Preset disabled"), Anchor->IsUsingResolvedCardLayoutPresetForTest());

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerNullPresetFallbackTest,
	"Wacom.UI.FirstPersonCardLayer.LayoutPreset.NullPresetFallsBackSafely",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerNullPresetFallbackTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	Anchor->bUseFirstPersonCardLayoutPreset = true;
	Anchor->FirstPersonCardLayoutPreset = nullptr;
	Anchor->AuthoredCardSpacingPixels = 72.0f;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	TArray<FWacomFirstPersonCardLayerEntry> Entries;
	Entries.SetNum(2);
	Anchor->SetRuntimeCardLayerEntries(TEXT("NullPreset"), Entries);
	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildActiveCardLayerSlotViewsForTest();

	if (Slots.Num() == 2)
	{
		TestEqual(TEXT("Null preset uses component spacing"), Slots[1].AuthoredLayoutOffset.X - Slots[0].AuthoredLayoutOffset.X, 72.0);
	}
	TestFalse(TEXT("Null preset is not active"), Anchor->IsUsingResolvedCardLayoutPresetForTest());
	TestTrue(TEXT("Debug summary reports missing preset"), Anchor->GetDebugSummary().Contains(TEXT("PresetName=Missing")));

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerPresetMotionFeedbackTest,
	"Wacom.UI.FirstPersonCardLayer.LayoutPreset.PresetOverridesMotionAndFeedbackConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerPresetMotionFeedbackTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	UWacomFirstPersonCardLayoutPreset* Preset = WacomFirstPersonCardLayerSpec::MakeLayoutPreset(GetTransientPackage());
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor)
		|| !TestNotNull(TEXT("Preset"), Preset))
	{
		return false;
	}

	Anchor->bDrawStaticCardLayer = true;
	Anchor->bUseFirstPersonCardLayoutPreset = true;
	Anchor->FirstPersonCardLayoutPreset = Preset;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	Anchor->RefreshStaticLayerForTest();

	UWacomFirstPersonCardLayerWidget* Layer = Anchor->GetStaticCardLayerWidgetForTest();
	if (TestNotNull(TEXT("Static layer"), Layer))
	{
		const FWacomFirstPersonCardSlotMotionConfig& MotionConfig = Layer->GetSlotMotionConfigForTest();
		const FWacomFirstPersonCardSlotFeedbackConfig& FeedbackConfig = Layer->GetSlotFeedbackConfigForTest();
		TestEqual(TEXT("Preset motion speed"), MotionConfig.MotionSpeed, Preset->CardSlotMotionSpeed);
		TestEqual(TEXT("Preset opacity speed"), MotionConfig.OpacitySpeed, Preset->CardSlotOpacitySpeed);
		TestEqual(TEXT("Preset enter offset"), MotionConfig.EnterOffsetPixels, Preset->CardSlotEnterOffsetPixels);
		TestEqual(TEXT("Preset readable origins enabled"), MotionConfig.bEnableReadableTransitionOrigins, Preset->bEnableReadableTransitionOrigins);
		TestEqual(TEXT("Preset gained enter offset"), MotionConfig.GainedEnterOffsetPixels, Preset->GainedCardEnterOffsetPixels);
		TestEqual(TEXT("Preset drawn origin mode"), MotionConfig.DrawnEnterOriginMode, Preset->DrawnCardEnterOriginMode);
		TestEqual(TEXT("Preset drawn viewport anchor"), MotionConfig.DrawnEnterViewportAnchor, Preset->DrawnCardEnterViewportAnchor);
		TestEqual(TEXT("Preset drawn scale accent"), MotionConfig.DrawnEnterScaleMultiplier, Preset->DrawnCardEnterScaleMultiplier);
		TestEqual(TEXT("Preset drawn angle accent"), MotionConfig.DrawnEnterAngleOffsetDegrees, Preset->DrawnCardEnterAngleOffsetDegrees);
		TestEqual(TEXT("Preset played origin mode"), MotionConfig.PlayedExitOriginMode, Preset->PlayedCardExitOriginMode);
		TestEqual(TEXT("Preset discarded scale accent"), MotionConfig.DiscardedExitScaleMultiplier, Preset->DiscardedCardExitScaleMultiplier);
		TestEqual(TEXT("Preset feedback enabled"), FeedbackConfig.bEnabled, Preset->bEnableCardInteractionFeedback);
		TestEqual(TEXT("Preset hover feedback color"), FeedbackConfig.PlayableHoverColor, Preset->PlayableHoverFeedbackColor);
		TestEqual(TEXT("Preset deny opacity"), FeedbackConfig.DenyOpacity, Preset->DenyFeedbackOpacity);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerPresetScopeTest,
	"Wacom.UI.FirstPersonCardLayer.LayoutPreset.PresetDoesNotOverrideViewClassOrDebugToggles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerPresetScopeTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	UWacomFirstPersonCardLayoutPreset* Preset = WacomFirstPersonCardLayerSpec::MakeLayoutPreset(GetTransientPackage());
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor)
		|| !TestNotNull(TEXT("Preset"), Preset))
	{
		return false;
	}

	Anchor->bDrawStaticCardLayer = true;
	Anchor->bDrawDebugProjection = false;
	Anchor->StaticCardLayerZOrder = 1234;
	Anchor->FirstPersonCardViewClass = UWacomFirstPersonCardLayerPresetViewClassProbe::StaticClass();
	Anchor->bUseFirstPersonCardLayoutPreset = true;
	Anchor->FirstPersonCardLayoutPreset = Preset;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	Anchor->RefreshStaticLayerForTest();

	UWacomFirstPersonCardLayerWidget* Layer = Anchor->GetStaticCardLayerWidgetForTest();
	if (TestNotNull(TEXT("Static layer"), Layer))
	{
		TestEqual(
			TEXT("Preset does not override card view class"),
			Layer->GetCardViewClassForTest(),
			TSubclassOf<UWacomCardView>(UWacomFirstPersonCardLayerPresetViewClassProbe::StaticClass()));
	}
	TestFalse(TEXT("Preset does not enable debug projection"), Anchor->bDrawDebugProjection);
	TestEqual(TEXT("Component z order remains configured"), Anchor->StaticCardLayerZOrder, 1234);

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerPresetSwitchResetTest,
	"Wacom.UI.FirstPersonCardLayer.LayoutPreset.SwitchingPresetResetsSmoothingAndLargeMotionState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerPresetSwitchResetTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	UWacomFirstPersonCardLayoutPreset* PresetA = WacomFirstPersonCardLayerSpec::MakeLayoutPreset(GetTransientPackage());
	UWacomFirstPersonCardLayoutPreset* PresetB = WacomFirstPersonCardLayerSpec::MakeLayoutPreset(GetTransientPackage());
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor)
		|| !TestNotNull(TEXT("PresetA"), PresetA)
		|| !TestNotNull(TEXT("PresetB"), PresetB))
	{
		return false;
	}

	PresetB->AuthoredCardSpacingPixels = 220.0f;
	PresetB->AuthoredMaxHandWidthPixels = 0.0f;
	Anchor->bDrawStaticCardLayer = true;
	Anchor->bUseFirstPersonCardLayoutPreset = true;
	Anchor->FirstPersonCardLayoutPreset = PresetA;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	Anchor->RefreshStaticLayerForTest();

	UWacomFirstPersonCardLayerWidget* Layer = Anchor->GetStaticCardLayerWidgetForTest();
	if (!TestNotNull(TEXT("Static layer"), Layer))
	{
		return false;
	}
	TestTrue(TEXT("Slots created before switch"), Layer->GetCardViewCount() > 0);

	Anchor->FirstPersonCardLayoutPreset = PresetB;
	Anchor->RefreshStaticLayerForTest();
	TestEqual(TEXT("Preset switch rebuilds active slot count"), Layer->GetSlotMotionDebugView().ActiveSlotCount, Layer->GetCardViewCount());
	TestEqual(TEXT("Preset switch does not leave outgoing slots"), Layer->GetSlotMotionDebugView().OutgoingSlotCount, 0);

	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildActiveCardLayerSlotViewsForTest();
	if (Slots.Num() >= 2)
	{
		TestEqual(TEXT("New preset spacing applies after switch"), Slots[1].AuthoredLayoutOffset.X - Slots[0].AuthoredLayoutOffset.X, 220.0);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerPresetDebugSummaryTest,
	"Wacom.UI.FirstPersonCardLayer.LayoutPreset.DebugSummaryReportsResolvedPreset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerPresetDebugSummaryTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	UWacomFirstPersonCardLayoutPreset* Preset = WacomFirstPersonCardLayerSpec::MakeLayoutPreset(GetTransientPackage());
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor)
		|| !TestNotNull(TEXT("Preset"), Preset))
	{
		return false;
	}

	Anchor->bUseFirstPersonCardLayoutPreset = true;
	Anchor->FirstPersonCardLayoutPreset = Preset;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const FString Summary = Anchor->GetDebugSummary();
	TestTrue(TEXT("Summary reports preset enabled"), Summary.Contains(TEXT("PresetEnabled=true")));
	TestTrue(TEXT("Summary reports preset active"), Summary.Contains(TEXT("PresetActive=true")));
	TestTrue(TEXT("Summary reports preset fallback false"), Summary.Contains(TEXT("PresetFallback=false")));
	TestTrue(TEXT("Summary reports preset name"), Summary.Contains(Preset->GetName()));

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerHardClampTest,
	"Wacom.UI.FirstPersonCardLayer.ViewportClamp.HardClampPreservesCurrentViewportClamp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerHardClampTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	Anchor->ViewportClampMode = EWacomFirstPersonCardViewportClampMode::HardClampToViewport;
	Anchor->ProjectionPadding = 24.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	FWacomFirstPersonCardProjectedPoint Point;
	const bool bProjected = Anchor->ProjectCardTransformToScreen(
		FTransform(FRotator::ZeroRotator, FVector(0.0f, 1140.0f, 0.0f), FVector::OneVector),
		Point);

	TestTrue(TEXT("Offscreen point still projects"), bProjected);
	TestEqual(TEXT("Hard clamp records raw widget position"), Point.UnclampedWidgetPosition, FVector2D(2100.0f, 540.0f));
	TestEqual(TEXT("Hard clamp keeps X inside padding"), Point.WidgetPosition, FVector2D(1896.0f, 540.0f));
	TestTrue(TEXT("Hard clamp reports clamped"), Point.bClamped);
	TestTrue(TEXT("Hard clamp reports outside viewport"), Point.bOutsideViewport);
	TestEqual(TEXT("Hard clamp mode recorded"), Point.ViewportClampMode, EWacomFirstPersonCardViewportClampMode::HardClampToViewport);

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerAllowOffscreenTest,
	"Wacom.UI.FirstPersonCardLayer.ViewportClamp.AllowOffscreenKeepsUnclampedWidgetPosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerAllowOffscreenTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	Anchor->ViewportClampMode = EWacomFirstPersonCardViewportClampMode::AllowOffscreen;
	Anchor->ProjectionPadding = 24.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	FWacomFirstPersonCardProjectedPoint Point;
	const bool bProjected = Anchor->ProjectCardTransformToScreen(
		FTransform(FRotator::ZeroRotator, FVector(0.0f, 1140.0f, 0.0f), FVector::OneVector),
		Point);

	TestTrue(TEXT("AllowOffscreen point projects"), bProjected);
	TestTrue(TEXT("Projected flag remains true"), Point.bProjected);
	TestEqual(TEXT("AllowOffscreen keeps widget position offscreen"), Point.WidgetPosition, FVector2D(2100.0f, 540.0f));
	TestEqual(TEXT("AllowOffscreen final position is unclamped"), Point.ScreenPosition, Point.UnclampedWidgetPosition);
	TestFalse(TEXT("AllowOffscreen does not clamp"), Point.bClamped);
	TestTrue(TEXT("AllowOffscreen still reports outside viewport"), Point.bOutsideViewport);
	TestEqual(TEXT("AllowOffscreen mode recorded"), Point.ViewportClampMode, EWacomFirstPersonCardViewportClampMode::AllowOffscreen);

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerSoftClampAllowanceTest,
	"Wacom.UI.FirstPersonCardLayer.ViewportClamp.SoftClampAllowsConfiguredOffscreenRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerSoftClampAllowanceTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	Anchor->ViewportClampMode = EWacomFirstPersonCardViewportClampMode::SoftClampToViewport;
	Anchor->ProjectionPadding = 24.0f;
	Anchor->SoftClampOffscreenAllowancePixels = 260.0f;
	Anchor->SoftClampBlendRangePixels = 240.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	FWacomFirstPersonCardProjectedPoint Point;
	const bool bProjected = Anchor->ProjectCardTransformToScreen(
		FTransform(FRotator::ZeroRotator, FVector(0.0f, 1140.0f, 0.0f), FVector::OneVector),
		Point);

	TestTrue(TEXT("SoftClamp point projects"), bProjected);
	TestEqual(TEXT("Point inside soft rect is not pulled back"), Point.WidgetPosition, FVector2D(2100.0f, 540.0f));
	TestFalse(TEXT("Point inside soft rect is not clamped"), Point.bClamped);
	TestTrue(TEXT("Point inside soft rect still reports outside viewport"), Point.bOutsideViewport);
	TestTrue(TEXT("Offscreen distance is measured from safe rect"),
		FMath::IsNearlyEqual(Point.OffscreenDistancePixels, 204.0f, 0.001f));

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerSoftClampBoundaryTest,
	"Wacom.UI.FirstPersonCardLayer.ViewportClamp.SoftClampStopsAtExpandedBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerSoftClampBoundaryTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	Anchor->ViewportClampMode = EWacomFirstPersonCardViewportClampMode::SoftClampToViewport;
	Anchor->ProjectionPadding = 24.0f;
	Anchor->SoftClampOffscreenAllowancePixels = 260.0f;
	Anchor->SoftClampBlendRangePixels = 240.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	FWacomFirstPersonCardProjectedPoint HalfBlendPoint;
	const bool bHalfBlendProjected = Anchor->ProjectCardTransformToScreen(
		FTransform(FRotator::ZeroRotator, FVector(0.0f, 1316.0f, 0.0f), FVector::OneVector),
		HalfBlendPoint);
	TestTrue(TEXT("Half blend point projects"), bHalfBlendProjected);
	TestTrue(TEXT("Half blend point clamps softly"), HalfBlendPoint.bClamped);
	TestTrue(TEXT("Half blend point is halfway toward soft boundary"),
		FMath::IsNearlyEqual(HalfBlendPoint.WidgetPosition.X, 2216.0f, 0.001f));

	FWacomFirstPersonCardProjectedPoint BoundaryPoint;
	const bool bBoundaryProjected = Anchor->ProjectCardTransformToScreen(
		FTransform(FRotator::ZeroRotator, FVector(0.0f, 1436.0f, 0.0f), FVector::OneVector),
		BoundaryPoint);
	TestTrue(TEXT("Boundary point projects"), bBoundaryProjected);
	TestEqual(TEXT("Beyond blend range stops at soft boundary"), BoundaryPoint.WidgetPosition, FVector2D(2156.0f, 540.0f));
	TestTrue(TEXT("Boundary point reports clamped"), BoundaryPoint.bClamped);

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerAuthoredSoftClampedAnchorTest,
	"Wacom.UI.FirstPersonCardLayer.ViewportClamp.Authored2DUsesSoftClampedAnchorCenter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerAuthoredSoftClampedAnchorTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	Anchor->ViewportClampMode = EWacomFirstPersonCardViewportClampMode::SoftClampToViewport;
	Anchor->ProjectionPadding = 24.0f;
	Anchor->SoftClampOffscreenAllowancePixels = 260.0f;
	Anchor->SoftClampBlendRangePixels = 240.0f;
	Anchor->CardLayoutMode = EWacomFirstPersonCardLayoutMode::Authored2D;
	Anchor->StaticCardCountFallback = 1;
	Anchor->StaticCardEdgeDropPixels = 0.0f;
	Anchor->AuthoredHandScreenOffset = FVector2D::ZeroVector;
	Anchor->HorizontalOffset = 1236.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildStaticCardSlotViews();
	TestEqual(TEXT("Slot count"), Slots.Num(), 1);
	if (Slots.Num() == 1)
	{
		TestEqual(TEXT("Authored slot uses soft-clamped anchor center"), Slots[0].AnchorWidgetPosition, FVector2D(2156.0f, 310.0f));
		TestEqual(TEXT("Authored slot final position starts at soft-clamped anchor center"), Slots[0].ScreenPosition, FVector2D(2156.0f, 310.0f));
		TestEqual(TEXT("Authored slot records unclamped anchor position"), Slots[0].UnclampedWidgetPosition, FVector2D(2396.0f, 310.0f));
		TestTrue(TEXT("Authored slot reports clamped anchor"), Slots[0].bClamped);
		TestTrue(TEXT("Authored slot reports outside viewport"), Slots[0].bOutsideViewport);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerAuthoredProjectionTest,
	"Wacom.UI.FirstPersonCardLayer.AuthoredLayout.Authored2DProjectsOnlyHandCenter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerAuthoredProjectionTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	Anchor->CardLayoutMode = EWacomFirstPersonCardLayoutMode::Authored2D;
	Anchor->ProjectionPadding = 0.0f;
	Anchor->StaticCardCountFallback = 5;
	Anchor->StaticCardEdgeDropPixels = 0.0f;
	Anchor->AuthoredCardSpacingPixels = 100.0f;
	Anchor->AuthoredMaxHandWidthPixels = 0.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const TArray<FWacomFirstPersonCardLayerSlotView> InitialSlots = Anchor->BuildStaticCardSlotViews();
	Anchor->CardSpacing = 360.0f;
	const TArray<FWacomFirstPersonCardLayerSlotView> ChangedLegacySpacingSlots = Anchor->BuildStaticCardSlotViews();

	TestEqual(TEXT("Initial slot count"), InitialSlots.Num(), 5);
	TestEqual(TEXT("Changed spacing slot count"), ChangedLegacySpacingSlots.Num(), 5);
	if (InitialSlots.Num() == 5 && ChangedLegacySpacingSlots.Num() == 5)
	{
		TestEqual(TEXT("Authored layout mode is recorded"), InitialSlots[2].LayoutMode, EWacomFirstPersonCardLayoutMode::Authored2D);
		TestEqual(TEXT("Left slot uses projected hand center"), InitialSlots[0].AnchorWidgetPosition, InitialSlots[2].AnchorWidgetPosition);
		TestEqual(TEXT("Right slot uses projected hand center"), InitialSlots[4].AnchorWidgetPosition, InitialSlots[2].AnchorWidgetPosition);
		TestEqual(TEXT("Old 3D CardSpacing does not affect authored screen position"), ChangedLegacySpacingSlots[4].ScreenPosition, InitialSlots[4].ScreenPosition);
		TestTrue(TEXT("Authored spacing controls screen offset"),
			FMath::IsNearlyEqual(InitialSlots[4].ScreenPosition.X - InitialSlots[2].ScreenPosition.X, 200.0f, 0.001f));
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerRunTunnelTickPrerequisiteTest,
	"Wacom.UI.FirstPersonCardLayer.AnchorMotionStability.AnchorTicksAfterRunTunnelMovement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerRunTunnelTickPrerequisiteTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor)
		|| !TestNotNull(TEXT("Run tunnel component"), Character ? Character->GetRunTunnelMovementComponent() : nullptr))
	{
		return false;
	}

	Anchor->BeginPlayForTest();
	const UWacomRunTunnelMovementComponent* RunTunnel = Character->GetRunTunnelMovementComponent();
	TestTrue(
		TEXT("Anchor tick has RunTunnel movement prerequisite"),
		WacomFirstPersonCardLayerSpec::HasTickPrerequisite(
			Anchor->PrimaryComponentTick,
			RunTunnel,
			RunTunnel->PrimaryComponentTick));

	Anchor->DestroyComponent();
	Character->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerBattleCameraTickPrerequisiteTest,
	"Wacom.UI.FirstPersonCardLayer.AnchorMotionStability.AnchorTicksAfterBattleCameraLook",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerBattleCameraTickPrerequisiteTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor)
		|| !TestNotNull(TEXT("Battle camera component"), Character ? Character->GetBattleCameraLookComponent() : nullptr))
	{
		return false;
	}

	Anchor->BeginPlayForTest();
	const UWacomBattleCameraLookComponent* BattleCamera = Character->GetBattleCameraLookComponent();
	TestTrue(
		TEXT("Anchor tick has BattleCamera prerequisite"),
		WacomFirstPersonCardLayerSpec::HasTickPrerequisite(
			Anchor->PrimaryComponentTick,
			BattleCamera,
			BattleCamera->PrimaryComponentTick));

	Anchor->DestroyComponent();
	Character->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerAuthoredAnchorSmoothingTest,
	"Wacom.UI.FirstPersonCardLayer.AnchorMotionStability.Authored2DSmoothsAnchorCenter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerAuthoredAnchorSmoothingTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	Anchor->CardLayoutMode = EWacomFirstPersonCardLayoutMode::Authored2D;
	Anchor->ProjectionPadding = 0.0f;
	Anchor->StaticCardCountFallback = 1;
	Anchor->StaticCardEdgeDropPixels = 0.0f;
	Anchor->AuthoredHandScreenOffset = FVector2D::ZeroVector;
	Anchor->bEnableCardLayerPixelSnapping = false;
	Anchor->bEnableAnchorScreenSmoothing = true;
	Anchor->AnchorScreenSmoothingSpeed = 1.0f;
	Anchor->AnchorScreenSmoothingResetDistancePixels = 320.0f;
	Anchor->ProbeAnchorSmoothingDeltaTime = 1.0f / 60.0f;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const TArray<FWacomFirstPersonCardLayerSlotView> InitialSlots = Anchor->BuildStaticCardSlotViews();
	Anchor->HorizontalOffset = 60.0f;
	Anchor->RefreshAnchor(0.0f);
	const TArray<FWacomFirstPersonCardLayerSlotView> SmoothedSlots = Anchor->BuildStaticCardSlotViews();

	TestEqual(TEXT("Initial slot count"), InitialSlots.Num(), 1);
	TestEqual(TEXT("Smoothed slot count"), SmoothedSlots.Num(), 1);
	if (InitialSlots.Num() == 1 && SmoothedSlots.Num() == 1)
	{
		TestEqual(TEXT("Initial center is unsmoothed"), InitialSlots[0].ScreenPosition, FVector2D(1160.0f, 310.0f));
		TestTrue(TEXT("Smoothing is applied"), SmoothedSlots[0].bAnchorScreenSmoothed);
		TestEqual(TEXT("Raw target records moved center"), SmoothedSlots[0].UnsmoothedAnchorWidgetPosition, FVector2D(1220.0f, 310.0f));
		TestTrue(TEXT("Smoothed center remains between previous and target"),
			SmoothedSlots[0].AnchorWidgetPosition.X > 1160.0f && SmoothedSlots[0].AnchorWidgetPosition.X < 1220.0f);
		TestEqual(TEXT("Screen position uses smoothed center"), SmoothedSlots[0].ScreenPosition, SmoothedSlots[0].AnchorWidgetPosition);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerAuthoredSmoothingOffsetTest,
	"Wacom.UI.FirstPersonCardLayer.AnchorMotionStability.SmoothingDoesNotChangeAuthoredOffsets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerAuthoredSmoothingOffsetTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	Anchor->CardLayoutMode = EWacomFirstPersonCardLayoutMode::Authored2D;
	Anchor->ProjectionPadding = 0.0f;
	Anchor->StaticCardCountFallback = 5;
	Anchor->StaticCardEdgeDropPixels = 80.0f;
	Anchor->AuthoredCardSpacingPixels = 100.0f;
	Anchor->AuthoredMaxHandWidthPixels = 0.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	Anchor->bEnableAnchorScreenSmoothing = true;
	Anchor->AnchorScreenSmoothingSpeed = 1.0f;
	Anchor->ProbeAnchorSmoothingDeltaTime = 1.0f / 60.0f;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const TArray<FWacomFirstPersonCardLayerSlotView> BaseSlots = Anchor->BuildStaticCardSlotViews();
	Anchor->HorizontalOffset = 40.0f;
	Anchor->RefreshAnchor(0.0f);
	const TArray<FWacomFirstPersonCardLayerSlotView> MovedSlots = Anchor->BuildStaticCardSlotViews();

	TestEqual(TEXT("Base slot count"), BaseSlots.Num(), 5);
	TestEqual(TEXT("Moved slot count"), MovedSlots.Num(), 5);
	if (BaseSlots.Num() == 5 && MovedSlots.Num() == 5)
	{
		TestEqual(TEXT("Left offset is unchanged"), MovedSlots[0].AuthoredLayoutOffset, BaseSlots[0].AuthoredLayoutOffset);
		TestEqual(TEXT("Center offset is unchanged"), MovedSlots[2].AuthoredLayoutOffset, BaseSlots[2].AuthoredLayoutOffset);
		TestEqual(TEXT("Right offset is unchanged"), MovedSlots[4].AuthoredLayoutOffset, BaseSlots[4].AuthoredLayoutOffset);
		TestEqual(TEXT("Fan angle is unchanged"), MovedSlots[0].RenderAngleDegrees, BaseSlots[0].RenderAngleDegrees);
		TestEqual(TEXT("ZOrder is unchanged"), MovedSlots[2].ZOrder, BaseSlots[2].ZOrder);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerAnchorSmoothingResetTest,
	"Wacom.UI.FirstPersonCardLayer.AnchorMotionStability.LargeAnchorJumpResetsSmoothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerAnchorSmoothingResetTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	Anchor->CardLayoutMode = EWacomFirstPersonCardLayoutMode::Authored2D;
	Anchor->ProjectionPadding = 0.0f;
	Anchor->StaticCardCountFallback = 1;
	Anchor->StaticCardEdgeDropPixels = 0.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	Anchor->bEnableAnchorScreenSmoothing = true;
	Anchor->AnchorScreenSmoothingSpeed = 1.0f;
	Anchor->AnchorScreenSmoothingResetDistancePixels = 80.0f;
	Anchor->ProbeAnchorSmoothingDeltaTime = 1.0f / 60.0f;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	Anchor->BuildStaticCardSlotViews();
	Anchor->HorizontalOffset = 200.0f;
	Anchor->RefreshAnchor(0.0f);
	const TArray<FWacomFirstPersonCardLayerSlotView> JumpSlots = Anchor->BuildStaticCardSlotViews();

	TestEqual(TEXT("Jump slot count"), JumpSlots.Num(), 1);
	if (JumpSlots.Num() == 1)
	{
		TestFalse(TEXT("Large jump resets smoothing"), JumpSlots[0].bAnchorScreenSmoothed);
		TestEqual(TEXT("Large jump uses target immediately"), JumpSlots[0].ScreenPosition, JumpSlots[0].UnsmoothedAnchorWidgetPosition);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerAnchorSmoothingProjectionFailureTest,
	"Wacom.UI.FirstPersonCardLayer.AnchorMotionStability.ProjectionFailureResetsSmoothingAndHidesSlots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerAnchorSmoothingProjectionFailureTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	Anchor->CardLayoutMode = EWacomFirstPersonCardLayoutMode::Authored2D;
	Anchor->StaticCardCountFallback = 1;
	Anchor->bEnableAnchorScreenSmoothing = true;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	Anchor->BuildStaticCardSlotViews();
	Anchor->bProjectionSucceeds = false;
	const TArray<FWacomFirstPersonCardLayerSlotView> FailedSlots = Anchor->BuildStaticCardSlotViews();

	TestEqual(TEXT("Projection failure still builds slot"), FailedSlots.Num(), 1);
	if (FailedSlots.Num() == 1)
	{
		TestFalse(TEXT("Projection failure hides slot"), FailedSlots[0].bProjected);
		TestFalse(TEXT("Projection failure is not smoothed"), FailedSlots[0].bAnchorScreenSmoothed);
	}

	Anchor->bProjectionSucceeds = true;
	Anchor->HorizontalOffset = 40.0f;
	Anchor->RefreshAnchor(0.0f);
	const TArray<FWacomFirstPersonCardLayerSlotView> RecoveredSlots = Anchor->BuildStaticCardSlotViews();
	TestEqual(TEXT("Recovered slot count"), RecoveredSlots.Num(), 1);
	if (RecoveredSlots.Num() == 1)
	{
		TestFalse(TEXT("Recovered projection starts fresh after reset"), RecoveredSlots[0].bAnchorScreenSmoothed);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerLegacySkipsSmoothingTest,
	"Wacom.UI.FirstPersonCardLayer.AnchorMotionStability.LegacyProjectedFan2DSkipsAnchorSmoothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerLegacySkipsSmoothingTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	Anchor->CardLayoutMode = EWacomFirstPersonCardLayoutMode::LegacyProjectedFan2D;
	Anchor->ProjectionPadding = 0.0f;
	Anchor->StaticCardCountFallback = 1;
	Anchor->StaticCardEdgeDropPixels = 0.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	Anchor->bEnableAnchorScreenSmoothing = true;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	const TArray<FWacomFirstPersonCardLayerSlotView> InitialSlots = Anchor->BuildStaticCardSlotViews();
	Anchor->HorizontalOffset = 40.0f;
	Anchor->RefreshAnchor(0.0f);
	const TArray<FWacomFirstPersonCardLayerSlotView> MovedSlots = Anchor->BuildStaticCardSlotViews();

	TestEqual(TEXT("Initial slot count"), InitialSlots.Num(), 1);
	TestEqual(TEXT("Moved slot count"), MovedSlots.Num(), 1);
	if (InitialSlots.Num() == 1 && MovedSlots.Num() == 1)
	{
		TestFalse(TEXT("Legacy slot is not smoothed"), MovedSlots[0].bAnchorScreenSmoothed);
		TestEqual(TEXT("Legacy slot uses projected point directly"), MovedSlots[0].AnchorWidgetPosition, MovedSlots[0].WidgetPosition);
		TestNotEqual(TEXT("Legacy slot still moves with projected anchor"), MovedSlots[0].ScreenPosition, InitialSlots[0].ScreenPosition);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerAnchorSmoothingDebugTest,
	"Wacom.UI.FirstPersonCardLayer.AnchorMotionStability.DebugSummaryReportsAnchorSmoothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerAnchorSmoothingDebugTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	Anchor->CardLayoutMode = EWacomFirstPersonCardLayoutMode::Authored2D;
	Anchor->ProjectionPadding = 0.0f;
	Anchor->StaticCardEdgeDropPixels = 0.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	Anchor->bEnableAnchorScreenSmoothing = true;
	Anchor->AnchorScreenSmoothingSpeed = 1.0f;
	Anchor->ProbeAnchorSmoothingDeltaTime = 1.0f / 60.0f;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	Anchor->BuildStaticCardSlotViews();
	Anchor->HorizontalOffset = 40.0f;
	Anchor->RefreshAnchor(0.0f);
	const FWacomFirstPersonCardAnchorDebugView View = Anchor->GetFirstPersonCardAnchorDebugView(1);

	TestEqual(TEXT("Debug point count"), View.ProjectedPoints.Num(), 1);
	TestTrue(TEXT("Debug view records smoothing"), View.bAnchorScreenSmoothed);
	if (View.ProjectedPoints.Num() == 1)
	{
		TestTrue(TEXT("Debug point records smoothing"), View.ProjectedPoints[0].bAnchorScreenSmoothed);
		TestTrue(TEXT("Debug point records smoothing distance"), View.ProjectedPoints[0].AnchorScreenSmoothingDistancePixels > 0.0f);
	}

	const FString Summary = Anchor->GetDebugSummary();
	TestTrue(TEXT("Summary reports smoothing enabled"), Summary.Contains(TEXT("AnchorScreenSmoothing=true")));
	TestTrue(TEXT("Summary reports smoothing speed"), Summary.Contains(TEXT("AnchorScreenSmoothingSpeed=1.00")));
	TestTrue(TEXT("Summary reports smoothed state"), Summary.Contains(TEXT("AnchorScreenSmoothed=true")));

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerAuthoredSpacingTest,
	"Wacom.UI.FirstPersonCardLayer.AuthoredLayout.Authored2DSpacingAndMaxWidth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerAuthoredSpacingTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	Anchor->CardLayoutMode = EWacomFirstPersonCardLayoutMode::Authored2D;
	Anchor->ProjectionPadding = 0.0f;
	Anchor->StaticCardCountFallback = 7;
	Anchor->AuthoredCardSpacingPixels = 160.0f;
	Anchor->AuthoredMaxHandWidthPixels = 480.0f;
	Anchor->StaticCardEdgeDropPixels = 0.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildStaticCardSlotViews();
	TestEqual(TEXT("Slot count"), Slots.Num(), 7);
	if (Slots.Num() == 7)
	{
		for (int32 Index = 1; Index < Slots.Num(); ++Index)
		{
			TestTrue(
				FString::Printf(TEXT("Slot %d remains to the right of previous slot"), Index),
				Slots[Index].ScreenPosition.X > Slots[Index - 1].ScreenPosition.X);
		}
		TestTrue(TEXT("Authored max hand width compresses layout"),
			FMath::IsNearlyEqual(Slots.Last().ScreenPosition.X - Slots[0].ScreenPosition.X, 480.0f, 0.001f));
		TestTrue(TEXT("Per-card spacing is compressed below authored spacing"),
			(Slots[4].ScreenPosition.X - Slots[3].ScreenPosition.X) < 160.0f);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerAuthoredCurveTest,
	"Wacom.UI.FirstPersonCardLayer.AuthoredLayout.Authored2DEdgeDropAndCenterLift",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerAuthoredCurveTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	Anchor->CardLayoutMode = EWacomFirstPersonCardLayoutMode::Authored2D;
	Anchor->ProjectionPadding = 0.0f;
	Anchor->StaticCardCountFallback = 5;
	Anchor->AuthoredCardSpacingPixels = 100.0f;
	Anchor->AuthoredMaxHandWidthPixels = 0.0f;
	Anchor->StaticCardEdgeDropPixels = 80.0f;
	Anchor->AuthoredCenterLiftPixels = 20.0f;
	Anchor->AuthoredDropCurveExponent = 2.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const TArray<FWacomFirstPersonCardLayerSlotView> SquareSlots = Anchor->BuildStaticCardSlotViews();
	Anchor->AuthoredDropCurveExponent = 1.0f;
	const TArray<FWacomFirstPersonCardLayerSlotView> LinearSlots = Anchor->BuildStaticCardSlotViews();

	TestEqual(TEXT("Square slot count"), SquareSlots.Num(), 5);
	TestEqual(TEXT("Linear slot count"), LinearSlots.Num(), 5);
	if (SquareSlots.Num() == 5 && LinearSlots.Num() == 5)
	{
		TestTrue(TEXT("Edge card drops lower than center"), SquareSlots[0].ScreenPosition.Y > SquareSlots[2].ScreenPosition.Y);
		TestTrue(TEXT("Center lift is applied"),
			FMath::IsNearlyEqual(SquareSlots[2].AuthoredLayoutOffset.Y, -20.0f, 0.001f));
		TestTrue(TEXT("Drop exponent changes inner-card curve"),
			LinearSlots[1].ScreenPosition.Y > SquareSlots[1].ScreenPosition.Y);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerAuthoredFanAngleTest,
	"Wacom.UI.FirstPersonCardLayer.AuthoredLayout.Authored2DFanAngleUsesExistingClamp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerAuthoredFanAngleTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	Anchor->CardLayoutMode = EWacomFirstPersonCardLayoutMode::Authored2D;
	Anchor->FanYawDegrees = 6.0f;
	Anchor->AuthoredFanCurveExponent = 2.0f;
	Anchor->MaxCardLayerRenderAngleDegrees = 4.0f;
	Anchor->bClampCardLayerRenderAngle = true;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const TArray<FWacomFirstPersonCardLayerSlotView> ClampedSlots = Anchor->BuildStaticCardSlotViews();
	Anchor->bClampCardLayerRenderAngle = false;
	const TArray<FWacomFirstPersonCardLayerSlotView> UnclampedSlots = Anchor->BuildStaticCardSlotViews();

	TestEqual(TEXT("Clamped slot count"), ClampedSlots.Num(), 5);
	TestEqual(TEXT("Unclamped slot count"), UnclampedSlots.Num(), 5);
	if (ClampedSlots.Num() == 5 && UnclampedSlots.Num() == 5)
	{
		TestEqual(TEXT("Edge angle is clamped"), ClampedSlots[0].RenderAngleDegrees, -4.0f);
		TestTrue(TEXT("Fan exponent reduces inner card rotation"), FMath::IsNearlyEqual(UnclampedSlots[1].RenderAngleDegrees, -3.0f, 0.001f));
		TestEqual(TEXT("Unclamped edge uses full curved fan"), UnclampedSlots[4].RenderAngleDegrees, 12.0f);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerAuthoredScaleTest,
	"Wacom.UI.FirstPersonCardLayer.AuthoredLayout.Authored2DUsesStableCardScale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerAuthoredScaleTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	Anchor->CardLayoutMode = EWacomFirstPersonCardLayoutMode::Authored2D;
	Anchor->StaticCardRenderScale = 1.0f;
	Anchor->PendingTargetingScale = 1.2f;
	Anchor->HoverScale = 1.1f;

	const FGuid HoveredId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerEntry Pending;
	Pending.CardViewData.Name = FText::FromString(TEXT("Pending"));
	Pending.bIsPendingTargeting = true;
	FWacomFirstPersonCardLayerEntry Hovered;
	Hovered.CardInstanceId = HoveredId;
	Hovered.CardViewData.Name = FText::FromString(TEXT("Hovered"));
	Anchor->SetRuntimeCardLayerEntries(TEXT("BattleHand"), { Pending, Hovered });
	Anchor->SetHoveredCardInstanceIdForTest(HoveredId);
	const TArray<FWacomFirstPersonCardLayerSlotView> NearSlots = Anchor->BuildActiveCardLayerSlotViewsForTest();

	Anchor->DistanceFromView = 500.0f;
	Anchor->RefreshAnchor(0.0f);
	const TArray<FWacomFirstPersonCardLayerSlotView> FarSlots = Anchor->BuildActiveCardLayerSlotViewsForTest();

	TestEqual(TEXT("Near slot count"), NearSlots.Num(), 2);
	TestEqual(TEXT("Far slot count"), FarSlots.Num(), 2);
	if (NearSlots.Num() == 2 && FarSlots.Num() == 2)
	{
		TestEqual(TEXT("Pending scale comes from state multiplier"), NearSlots[0].RenderScale, 1.2f);
		TestEqual(TEXT("Hovered scale comes from state multiplier"), NearSlots[1].RenderScale, 1.1f);
		TestEqual(TEXT("Changing projection distance does not change pending scale"), FarSlots[0].RenderScale, NearSlots[0].RenderScale);
		TestEqual(TEXT("Changing projection distance does not change hovered scale"), FarSlots[1].RenderScale, NearSlots[1].RenderScale);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerAuthoredZOrderTest,
	"Wacom.UI.FirstPersonCardLayer.AuthoredLayout.Authored2DCenterCardsDrawOnTop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerAuthoredZOrderTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	Anchor->CardLayoutMode = EWacomFirstPersonCardLayoutMode::Authored2D;
	Anchor->StaticCardCountFallback = 5;
	Anchor->HoverZOrderBoost = 500;

	const TArray<FWacomFirstPersonCardLayerSlotView> BaseSlots = Anchor->BuildStaticCardSlotViews();
	const FGuid HoveredId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerEntry A;
	A.CardViewData.Name = FText::FromString(TEXT("A"));
	FWacomFirstPersonCardLayerEntry B;
	B.CardViewData.Name = FText::FromString(TEXT("B"));
	FWacomFirstPersonCardLayerEntry C;
	C.CardViewData.Name = FText::FromString(TEXT("C"));
	FWacomFirstPersonCardLayerEntry D;
	D.CardViewData.Name = FText::FromString(TEXT("D"));
	FWacomFirstPersonCardLayerEntry E;
	E.CardInstanceId = HoveredId;
	E.CardViewData.Name = FText::FromString(TEXT("E"));
	Anchor->SetRuntimeCardLayerEntries(TEXT("BattleHand"), { A, B, C, D, E });
	Anchor->SetHoveredCardInstanceIdForTest(HoveredId);
	const TArray<FWacomFirstPersonCardLayerSlotView> HoverSlots = Anchor->BuildActiveCardLayerSlotViewsForTest();

	TestEqual(TEXT("Base slot count"), BaseSlots.Num(), 5);
	TestEqual(TEXT("Hover slot count"), HoverSlots.Num(), 5);
	if (BaseSlots.Num() == 5 && HoverSlots.Num() == 5)
	{
		TestTrue(TEXT("Center card draws above left edge"), BaseSlots[2].ZOrder > BaseSlots[0].ZOrder);
		TestTrue(TEXT("Center card draws above right edge"), BaseSlots[2].ZOrder > BaseSlots[4].ZOrder);
		TestTrue(TEXT("Hover boost still wins over center default z-order"), HoverSlots[4].ZOrder > HoverSlots[2].ZOrder);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerLegacyProjectedFanTest,
	"Wacom.UI.FirstPersonCardLayer.AuthoredLayout.LegacyProjectedFan2DPreservesExistingBehavior",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerLegacyProjectedFanTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	Anchor->ProjectionPadding = 0.0f;
	Anchor->CardLayoutMode = EWacomFirstPersonCardLayoutMode::LegacyProjectedFan2D;
	Anchor->ProjectionMode = EWacomFirstPersonCardProjectionMode::LegacyWorldProjected;
	Anchor->StaticCardEdgeDropPixels = 0.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	Anchor->CardSpacing = 36.0f;
	const TArray<FWacomFirstPersonCardLayerSlotView> InitialSlots = Anchor->BuildStaticCardSlotViews();
	Anchor->CardSpacing = 72.0f;
	const TArray<FWacomFirstPersonCardLayerSlotView> ChangedSlots = Anchor->BuildStaticCardSlotViews();

	TestEqual(TEXT("Initial slot count"), InitialSlots.Num(), 5);
	TestEqual(TEXT("Changed slot count"), ChangedSlots.Num(), 5);
	if (InitialSlots.Num() == 5 && ChangedSlots.Num() == 5)
	{
		TestEqual(TEXT("Legacy layout mode is recorded"), InitialSlots[2].LayoutMode, EWacomFirstPersonCardLayoutMode::LegacyProjectedFan2D);
		TestNotEqual(TEXT("Legacy mode still uses per-card 3D spacing"), ChangedSlots[4].ScreenPosition, InitialSlots[4].ScreenPosition);
		TestEqual(TEXT("Legacy point anchor is its own projected card point"), InitialSlots[4].AnchorWidgetPosition, InitialSlots[4].WidgetPosition);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerAuthoredDebugTest,
	"Wacom.UI.FirstPersonCardLayer.AuthoredLayout.DebugViewMatchesAuthoredLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerAuthoredDebugTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	Anchor->CardLayoutMode = EWacomFirstPersonCardLayoutMode::Authored2D;
	Anchor->ProjectionPadding = 0.0f;
	Anchor->StaticCardCountFallback = 5;
	Anchor->StaticCardEdgeDropPixels = 40.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildStaticCardSlotViews();
	const FWacomFirstPersonCardAnchorDebugView View = Anchor->GetFirstPersonCardAnchorDebugView(5);

	TestEqual(TEXT("Slot count"), Slots.Num(), 5);
	TestEqual(TEXT("Debug point count"), View.ProjectedPoints.Num(), 5);
	TestEqual(TEXT("Debug view records authored layout"), View.LayoutMode, EWacomFirstPersonCardLayoutMode::Authored2D);
	if (Slots.Num() == 5 && View.ProjectedPoints.Num() == 5)
	{
		TestEqual(TEXT("Debug point matches authored slot position"), View.ProjectedPoints[3].ScreenPosition, Slots[3].ScreenPosition);
		TestEqual(TEXT("Debug point matches authored offset"), View.ProjectedPoints[3].AuthoredLayoutOffset, Slots[3].AuthoredLayoutOffset);
		TestEqual(TEXT("Debug point records normalized offset"), View.ProjectedPoints[3].NormalizedHandOffset, Slots[3].NormalizedHandOffset);
	}

	const FString Summary = Anchor->GetDebugSummary();
	TestTrue(TEXT("Summary reports layout mode"), Summary.Contains(TEXT("LayoutMode=Authored2D")));

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerStaticHitTestTest,
	"Wacom.UI.FirstPersonCardLayer.StaticLayer.HitTestInvisibleAndNoInputBindings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerStaticHitTestTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("Layer widget"), Layer))
	{
		return false;
	}

	FWacomFirstPersonCardLayerSlotView Slot;
	Slot.Index = 0;
	Slot.ScreenPosition = FVector2D(500.0f, 600.0f);
	Slot.RenderScale = 0.55f;
	Slot.bProjected = true;
	Layer->SetStaticCardSlots({ Slot });
	TestEqual(TEXT("Layer is hit-test-invisible"), Layer->GetVisibility(), ESlateVisibility::HitTestInvisible);
	UWacomCardView* CardView = Layer->GetCardViewAt(0);
	if (TestNotNull(TEXT("Created card is CardView"), CardView))
	{
		TestEqual(TEXT("Card view is hit-test-invisible"), CardView->GetVisibility(), ESlateVisibility::HitTestInvisible);
	}

	Layer->RemoveFromParent();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerProjectionFailureHidesTest,
	"Wacom.UI.FirstPersonCardLayer.StaticLayer.ProjectionFailureHidesCards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerProjectionFailureHidesTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	Anchor->bProjectionSucceeds = false;
	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildStaticCardSlotViews();
	TestEqual(TEXT("Projection failure still builds slot data"), Slots.Num(), 5);
	if (Slots.Num() > 0)
	{
		TestFalse(TEXT("Slot is not projected"), Slots[0].bProjected);
	}

	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (TestNotNull(TEXT("Layer widget"), Layer))
	{
		Layer->SetStaticCardSlots(Slots);
		TestFalse(TEXT("Failed projection hides card"), Layer->IsCardSlotVisible(0));
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerToggleRemovesWidgetTest,
	"Wacom.UI.FirstPersonCardLayer.StaticLayer.ToggleRemovesWidget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerToggleRemovesWidgetTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	Anchor->bDrawStaticCardLayer = true;
	Anchor->RefreshStaticLayerForTest();
	TestTrue(TEXT("Static layer is created"), Anchor->IsStaticCardLayerWidgetActive());

	Anchor->bDrawStaticCardLayer = false;
	Anchor->RefreshStaticLayerForTest();
	TestFalse(TEXT("Static layer is removed after toggle off"), Anchor->IsStaticCardLayerWidgetActive());

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerBodyLockedVisualOffsetsTest,
	"Wacom.UI.FirstPersonCardLayer.Projection.BodyLockedStillAppliesHoverPendingAndEdgeDrop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerBodyLockedVisualOffsetsTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	Anchor->ProjectionMode = EWacomFirstPersonCardProjectionMode::BodyLocked;
	Anchor->ProjectionPadding = 0.0f;
	Anchor->StaticCardRenderScale = 1.0f;
	Anchor->StaticCardEdgeDropPixels = 80.0f;
	Anchor->PendingTargetingLiftPixels = 32.0f;
	Anchor->HoverLiftPixels = 24.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const FGuid HoveredCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerEntry LeftEdge;
	LeftEdge.CardViewData.Name = FText::FromString(TEXT("Left"));
	FWacomFirstPersonCardLayerEntry PendingCenter;
	PendingCenter.CardViewData.Name = FText::FromString(TEXT("Pending"));
	FWacomFirstPersonCardLayerEntry HoveredRightEdge;
	HoveredRightEdge.CardInstanceId = HoveredCardId;
	HoveredRightEdge.CardViewData.Name = FText::FromString(TEXT("Hovered"));
	Anchor->SetRuntimeCardLayerEntries(TEXT("BattleHand"), { LeftEdge, PendingCenter, HoveredRightEdge });
	const TArray<FWacomFirstPersonCardLayerSlotView> BaseSlots = Anchor->BuildActiveCardLayerSlotViewsForTest();

	PendingCenter.bIsPendingTargeting = true;
	Anchor->SetRuntimeCardLayerEntries(TEXT("BattleHand"), { LeftEdge, PendingCenter, HoveredRightEdge });
	Anchor->SetHoveredCardInstanceIdForTest(HoveredCardId);

	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildActiveCardLayerSlotViewsForTest();
	TestEqual(TEXT("Base slot count"), BaseSlots.Num(), 3);
	TestEqual(TEXT("Slot count"), Slots.Num(), 3);
	if (BaseSlots.Num() == 3 && Slots.Num() == 3)
	{
		TestTrue(TEXT("Body locked layout is recorded"), Slots[1].bBodyLockedLayout);
		TestTrue(TEXT("Current camera projection is recorded"), Slots[1].bCurrentCameraProjection);
		TestTrue(TEXT("Left edge drop still lowers edge card"), BaseSlots[0].ScreenPosition.Y > BaseSlots[1].ScreenPosition.Y);
		TestTrue(TEXT("Pending lift still raises center card"), Slots[1].ScreenPosition.Y < BaseSlots[1].ScreenPosition.Y);
		TestTrue(TEXT("Hover lift still raises hovered edge card"), Slots[2].ScreenPosition.Y < BaseSlots[2].ScreenPosition.Y);
		TestTrue(TEXT("Hovered slot is marked"), Slots[2].bIsHovered);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerInteractionDisabledTest,
	"Wacom.UI.FirstPersonCardLayer.Interaction.InteractionDisabledStaysHitTestInvisible",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerInteractionDisabledTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Layer widget"), Layer))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver Receiver;
	Layer->OnCardClickedNative.AddRaw(
		&Receiver,
		&WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver::HandleClicked);
	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid())
	});

	TestEqual(TEXT("Interaction off keeps layer hit-test-invisible"), Layer->GetVisibility(), ESlateVisibility::HitTestInvisible);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		TestEqual(TEXT("Interaction off keeps slot hit-test-invisible"), SlotWidget->GetVisibility(), ESlateVisibility::HitTestInvisible);
		TestFalse(TEXT("Interaction off rejects click"), SlotWidget->RequestClickForTest());
	}
	TestEqual(TEXT("Interaction off does not broadcast click"), Receiver.ClickCount, 0);

	Layer->OnCardClickedNative.RemoveAll(&Receiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerMotionDisabledImmediateTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotion.MotionDisabledAppliesSlotsImmediately",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerMotionDisabledImmediateTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.bEnabled = false;
	Layer->SetSlotMotionConfig(Config);
	const FGuid CardId = FGuid::NewGuid();
	Layer->SetCardSlots({ WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(100.0f, 200.0f), 4.0f, 0.8f, 0.5f) });

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		TestEqual(TEXT("Position applies immediately"), SlotWidget->GetVisualSlotView().ScreenPosition, FVector2D(100.0f, 200.0f));
		TestEqual(TEXT("Angle applies immediately"), SlotWidget->GetVisualSlotView().RenderAngleDegrees, 4.0f);
		TestEqual(TEXT("Scale applies immediately"), SlotWidget->GetVisualSlotView().RenderScale, 0.8f);
		TestEqual(TEXT("Opacity applies immediately"), SlotWidget->GetVisualSlotView().RenderOpacity, 0.5f);
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerMotionReuseTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotion.MotionReusesWidgetsByCardInstanceId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerMotionReuseTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.bEnableReadableTransitionOrigins = false;
	Layer->SetSlotMotionConfig(Config);
	const FGuid FirstId = FGuid::NewGuid();
	const FGuid SecondId = FGuid::NewGuid();
	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(FirstId, 0, FVector2D(100.0f, 200.0f)),
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SecondId, 1, FVector2D(220.0f, 200.0f))
	});
	UWacomFirstPersonCardLayerSlotWidget* FirstWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* SecondWidget = Layer->GetSlotWidgetAt(1);

	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SecondId, 0, FVector2D(100.0f, 200.0f)),
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(FirstId, 1, FVector2D(220.0f, 200.0f))
	});
	TestEqual(TEXT("Second card widget moves to first index"), Layer->GetSlotWidgetAt(0), SecondWidget);
	TestEqual(TEXT("First card widget moves to second index"), Layer->GetSlotWidgetAt(1), FirstWidget);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerMotionInsertedCardCreatesVisibleWidgetTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotion.InsertedCardCreatesVisibleWidgetWithoutOutgoingLeak",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerMotionInsertedCardCreatesVisibleWidgetTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.EnterOffsetPixels = FVector2D::ZeroVector;
	Config.EnterOpacity = 1.0f;
	Layer->SetSlotMotionConfig(Config);

	const FGuid FirstId = FGuid::NewGuid();
	const FGuid SecondId = FGuid::NewGuid();
	const FGuid InsertedId = FGuid::NewGuid();
	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(FirstId, 0, FVector2D(100.0f, 200.0f)),
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SecondId, 1, FVector2D(220.0f, 200.0f))
	});
	UWacomFirstPersonCardLayerSlotWidget* FirstWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* SecondWidget = Layer->GetSlotWidgetAt(1);

	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(InsertedId, 0, FVector2D(100.0f, 200.0f)),
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(FirstId, 1, FVector2D(220.0f, 200.0f)),
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SecondId, 2, FVector2D(340.0f, 200.0f))
	});

	TestEqual(TEXT("Inserted hand has three active slots"), Layer->GetCardViewCount(), 3);
	TestEqual(TEXT("Pure insertion does not create outgoing widgets"), Layer->GetOutgoingCardViewCount(), 0);
	TestNotEqual(TEXT("Inserted card gets a new widget"), Layer->GetSlotWidgetAt(0), FirstWidget);
	TestEqual(TEXT("Existing first card is reused at new index"), Layer->GetSlotWidgetAt(1), FirstWidget);
	TestEqual(TEXT("Existing second card is reused at new index"), Layer->GetSlotWidgetAt(2), SecondWidget);
	TestTrue(TEXT("Inserted slot is visible"), Layer->IsCardSlotVisible(0));
	TestTrue(TEXT("Shifted first slot is visible"), Layer->IsCardSlotVisible(1));
	TestTrue(TEXT("Shifted second slot is visible"), Layer->IsCardSlotVisible(2));

	TSet<UWacomFirstPersonCardLayerSlotWidget*> UniqueWidgets;
	for (int32 Index = 0; Index < Layer->GetCardViewCount(); ++Index)
	{
		UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(Index);
		TestNotNull(FString::Printf(TEXT("Slot widget %d"), Index), SlotWidget);
		TestNotNull(FString::Printf(TEXT("Slot card view %d"), Index), SlotWidget ? SlotWidget->GetCardView() : nullptr);
		if (SlotWidget)
		{
			TestFalse(FString::Printf(TEXT("Slot widget %d is unique"), Index), UniqueWidgets.Contains(SlotWidget));
			UniqueWidgets.Add(SlotWidget);
		}
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerMotionDuplicateKeyDoesNotReuseSameWidgetTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotion.DuplicateKeysDoNotReuseSameWidgetTwice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerMotionDuplicateKeyDoesNotReuseSameWidgetTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.EnterOffsetPixels = FVector2D::ZeroVector;
	Config.EnterOpacity = 1.0f;
	Layer->SetSlotMotionConfig(Config);

	const FGuid SharedId = FGuid::NewGuid();
	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SharedId, 0, FVector2D(100.0f, 200.0f)),
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SharedId, 1, FVector2D(220.0f, 200.0f))
	});

	TestEqual(TEXT("Duplicate-key hand keeps both active slots"), Layer->GetCardViewCount(), 2);
	TestEqual(TEXT("Duplicate-key hand does not create outgoing widgets"), Layer->GetOutgoingCardViewCount(), 0);
	UWacomFirstPersonCardLayerSlotWidget* FirstWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* SecondWidget = Layer->GetSlotWidgetAt(1);
	if (TestNotNull(TEXT("First duplicate slot"), FirstWidget)
		&& TestNotNull(TEXT("Second duplicate slot"), SecondWidget))
	{
		TestNotEqual(TEXT("Duplicate keys do not reuse the same widget pointer"), FirstWidget, SecondWidget);
		TestTrue(TEXT("First duplicate slot visible"), Layer->IsCardSlotVisible(0));
		TestTrue(TEXT("Second duplicate slot visible"), Layer->IsCardSlotVisible(1));
		TestNotNull(TEXT("First duplicate card view"), FirstWidget->GetCardView());
		TestNotNull(TEXT("Second duplicate card view"), SecondWidget->GetCardView());
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerMotionInterpolatesTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotion.PositionAngleScaleOpacityInterpolateTowardTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerMotionInterpolatesTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.EnterOffsetPixels = FVector2D::ZeroVector;
	Config.EnterOpacity = 1.0f;
	Layer->SetSlotMotionConfig(Config);
	const FGuid CardId = FGuid::NewGuid();
	Layer->SetCardSlots({ WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(100.0f, 200.0f), 0.0f, 1.0f, 1.0f) });
	Layer->TickSlotMotionForTest(1.0f);
	Layer->SetCardSlots({ WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(200.0f, 300.0f), 10.0f, 1.2f, 0.4f) });
	Layer->TickSlotMotionForTest(0.25f);

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		const FWacomFirstPersonCardLayerSlotView& Visual = SlotWidget->GetVisualSlotView();
		TestTrue(TEXT("Position interpolates X"), Visual.ScreenPosition.X > 100.0f && Visual.ScreenPosition.X < 200.0f);
		TestTrue(TEXT("Position interpolates Y"), Visual.ScreenPosition.Y > 200.0f && Visual.ScreenPosition.Y < 300.0f);
		TestTrue(TEXT("Angle interpolates"), Visual.RenderAngleDegrees > 0.0f && Visual.RenderAngleDegrees < 10.0f);
		TestTrue(TEXT("Scale interpolates"), Visual.RenderScale > 1.0f && Visual.RenderScale < 1.2f);
		TestTrue(TEXT("Opacity interpolates"), Visual.RenderOpacity < 1.0f && Visual.RenderOpacity > 0.4f);
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerMotionHoverPendingTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotion.HoverAndPendingAnimateWithoutChangingInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerMotionHoverPendingTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.EnterOffsetPixels = FVector2D::ZeroVector;
	Config.EnterOpacity = 1.0f;
	Layer->SetSlotMotionConfig(Config);
	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView BaseSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(100.0f, 300.0f), 0.0f, 1.0f);
	Layer->SetCardSlots({ BaseSlot });
	Layer->TickSlotMotionForTest(1.0f);
	FWacomFirstPersonCardLayerSlotView PendingHoverSlot = BaseSlot;
	PendingHoverSlot.ScreenPosition = FVector2D(100.0f, 240.0f);
	PendingHoverSlot.RenderScale = 1.15f;
	PendingHoverSlot.ZOrder = 1000;
	PendingHoverSlot.bIsHovered = true;
	PendingHoverSlot.Entry.bIsPendingTargeting = true;
	Layer->SetCardLayerInteractionEnabled(true);
	Layer->SetCardSlots({ PendingHoverSlot });

	WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver Receiver;
	Layer->OnCardClickedNative.AddRaw(
		&Receiver,
		&WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver::HandleClicked);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		Layer->TickSlotMotionForTest(0.25f);
		TestTrue(TEXT("Hover/pending lift animates"), SlotWidget->GetVisualSlotView().ScreenPosition.Y > 240.0f);
		TestTrue(TEXT("Click still succeeds"), SlotWidget->RequestClickForTest());
		TestEqual(TEXT("Click forwards original card id"), Receiver.LastCardId, CardId);
	}

	Layer->OnCardClickedNative.RemoveAll(&Receiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerMotionRemovedExitTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotion.RemovedCardPlaysExitThenRemovesWidget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerMotionRemovedExitTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.EnterOffsetPixels = FVector2D::ZeroVector;
	Config.EnterOpacity = 1.0f;
	Config.ExitDuration = 0.2f;
	Layer->SetSlotMotionConfig(Config);
	const FGuid CardId = FGuid::NewGuid();
	Layer->SetCardSlots({ WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(100.0f, 200.0f)) });
	Layer->TickSlotMotionForTest(1.0f);
	Layer->SetCardSlots({});

	TestEqual(TEXT("Active slot removed from hand"), Layer->GetCardViewCount(), 0);
	TestEqual(TEXT("Outgoing slot is retained"), Layer->GetOutgoingCardViewCount(), 1);
	UWacomFirstPersonCardLayerSlotWidget* Outgoing = Layer->GetOutgoingSlotWidgetAtForTest(0);
	if (TestNotNull(TEXT("Outgoing slot"), Outgoing))
	{
		TestTrue(TEXT("Outgoing slot is exiting"), Outgoing->IsExitingForFirstPersonLayer());
	}
	Layer->TickSlotMotionForTest(0.25f);
	TestEqual(TEXT("Outgoing slot removed after duration"), Layer->GetOutgoingCardViewCount(), 0);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerMotionProjectionExitTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotion.ProjectionFailurePlaysExitForVisibleSlot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerMotionProjectionExitTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.EnterOffsetPixels = FVector2D::ZeroVector;
	Config.EnterOpacity = 1.0f;
	Config.ExitDuration = 0.2f;
	Layer->SetSlotMotionConfig(Config);

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView VisibleSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(100.0f, 200.0f));
	Layer->SetCardSlots({ VisibleSlot });
	Layer->TickSlotMotionForTest(1.0f);

	FWacomFirstPersonCardLayerSlotView FailedSlot = VisibleSlot;
	FailedSlot.bProjected = false;
	Layer->SetCardSlots({ FailedSlot });

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		TestTrue(TEXT("Projection failure keeps previous visual during exit"), SlotWidget->GetVisualSlotView().bProjected);
		TestTrue(TEXT("Projection failure marks slot as exiting"), SlotWidget->IsExitingForFirstPersonLayer());
		TestTrue(TEXT("Projection failure slot remains visible during exit"), Layer->IsCardSlotVisible(0));
	}
	Layer->TickSlotMotionForTest(0.25f);
	TestFalse(TEXT("Projection failure hides after exit duration"), Layer->IsCardSlotVisible(0));

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerMotionLargeJumpTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotion.LargeJumpResetsSlotMotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerMotionLargeJumpTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.EnterOffsetPixels = FVector2D::ZeroVector;
	Config.EnterOpacity = 1.0f;
	Config.ResetDistancePixels = 80.0f;
	Layer->SetSlotMotionConfig(Config);
	const FGuid CardId = FGuid::NewGuid();
	Layer->SetCardSlots({ WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(100.0f, 200.0f)) });
	Layer->TickSlotMotionForTest(1.0f);
	Layer->SetCardSlots({ WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(400.0f, 500.0f)) });

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		TestEqual(TEXT("Large jump resets position immediately"), SlotWidget->GetVisualSlotView().ScreenPosition, FVector2D(400.0f, 500.0f));
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerMotionStaticKeyTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotion.StaticPreviewUsesStableIndexKeys",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerMotionStaticKeyTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.bEnableReadableTransitionOrigins = false;
	Layer->SetSlotMotionConfig(Config);
	FWacomFirstPersonCardLayerSlotView FirstSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(FGuid(), 0, FVector2D(100.0f, 200.0f));
	FWacomFirstPersonCardLayerSlotView SecondSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(FGuid(), 1, FVector2D(220.0f, 200.0f));
	Layer->SetStaticCardSlots({ FirstSlot, SecondSlot });
	UWacomFirstPersonCardLayerSlotWidget* FirstWidget = Layer->GetSlotWidgetAt(0);

	FirstSlot.ScreenPosition = FVector2D(140.0f, 220.0f);
	FirstSlot.WidgetPosition = FirstSlot.ScreenPosition;
	FirstSlot.SnappedWidgetPosition = FirstSlot.ScreenPosition;
	Layer->SetStaticCardSlots({ FirstSlot, SecondSlot });
	TestEqual(TEXT("Static index key reuses widget"), Layer->GetSlotWidgetAt(0), FirstWidget);
	TestNotNull(TEXT("StaticIndex key can be found"), Layer->FindSlotWidgetByKeyForTest(TEXT("StaticIndex:0")));

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerMotionHoveredVisualPositionTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotion.HoveredLayoutUpdateUsesVisualPosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerMotionHoveredVisualPositionTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.bEnableReadableTransitionOrigins = false;
	Config.EnterOffsetPixels = FVector2D::ZeroVector;
	Config.EnterOpacity = 1.0f;
	Layer->SetSlotMotionConfig(Config);
	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView BaseSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(100.0f, 200.0f));
	BaseSlot.bIsHovered = true;
	Layer->SetCardSlots({ BaseSlot });
	Layer->TickSlotMotionForTest(1.0f);

	WacomFirstPersonCardLayerSpec::FLayerLayoutUpdateReceiver Receiver;
	Layer->OnHoveredCardSlotUpdatedNative.AddRaw(
		&Receiver,
		&WacomFirstPersonCardLayerSpec::FLayerLayoutUpdateReceiver::HandleUpdated);
	FWacomFirstPersonCardLayerSlotView MovedSlot = BaseSlot;
	MovedSlot.ScreenPosition = FVector2D(200.0f, 200.0f);
	MovedSlot.WidgetPosition = MovedSlot.ScreenPosition;
	MovedSlot.SnappedWidgetPosition = MovedSlot.ScreenPosition;
	Layer->SetCardSlots({ MovedSlot });

	TestEqual(TEXT("Hovered visual update broadcasts"), Receiver.UpdateCount, 1);
	TestTrue(TEXT("Update uses visual position before final target"), Receiver.LastSlotView.ScreenPosition.X < 200.0f);
	TestEqual(TEXT("Update keeps hovered card id"), Receiver.LastCardId, CardId);
	Layer->OnHoveredCardSlotUpdatedNative.RemoveAll(&Receiver);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDrawnTransitionTest,
	"Wacom.UI.FirstPersonCardLayer.EventAwareTransitions.DrawnCardUsesDrawEnterOffset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDrawnTransitionTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.bEnableReadableTransitionOrigins = false;
	Layer->SetSlotMotionConfig(Config);
	const FGuid CardId = FGuid::NewGuid();
	const FVector2D TargetPosition(100.0f, 200.0f);
	Layer->SetCardTransitionHints({
		WacomFirstPersonCardLayerSpec::MakeTransitionHint(CardId, EWacomFirstPersonCardSlotTransitionKind::Drawn)
	});
	Layer->SetCardSlots({ WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, TargetPosition) });

	if (UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0))
	{
		TestEqual(TEXT("Drawn card starts from draw offset"), SlotWidget->GetVisualSlotView().ScreenPosition, TargetPosition + FVector2D(0.0f, 96.0f));
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerGainedTransitionTest,
	"Wacom.UI.FirstPersonCardLayer.EventAwareTransitions.GainedCardUsesGainEnterOffset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerGainedTransitionTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.bEnableReadableTransitionOrigins = false;
	Layer->SetSlotMotionConfig(Config);
	const FGuid CardId = FGuid::NewGuid();
	const FVector2D TargetPosition(120.0f, 220.0f);
	Layer->SetCardTransitionHints({
		WacomFirstPersonCardLayerSpec::MakeTransitionHint(CardId, EWacomFirstPersonCardSlotTransitionKind::Gained)
	});
	Layer->SetCardSlots({ WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, TargetPosition) });

	if (UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0))
	{
		TestEqual(TEXT("Gained card starts from gain offset"), SlotWidget->GetVisualSlotView().ScreenPosition, TargetPosition + FVector2D(0.0f, -120.0f));
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerNoTargetCommitTest,
	"Wacom.UI.FirstPersonCardLayer.PlayCommit.NoTargetCardPlayedTriggersCommitPulseAndPlayedExit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerNoTargetCommitTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig MotionConfig = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	MotionConfig.bEnableReadableTransitionOrigins = false;
	MotionConfig.EnterOffsetPixels = FVector2D::ZeroVector;
	MotionConfig.EnterOpacity = 1.0f;
	Layer->SetSlotMotionConfig(MotionConfig);
	Layer->SetSlotFeedbackConfig(WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());

	const FGuid CardId = FGuid::NewGuid();
	const FVector2D BasePosition(100.0f, 200.0f);
	Layer->SetCardSlots({ WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, BasePosition) });
	Layer->TickSlotMotionForTest(1.0f);

	FWacomFirstPersonCardLayerTransitionHint Hint =
		WacomFirstPersonCardLayerSpec::MakeTransitionHint(
			CardId,
			EWacomFirstPersonCardSlotTransitionKind::Played);
	Hint.bPlayCommitFeedback = true;
	Layer->SetCardTransitionHints({ Hint });
	Layer->SetCardSlots({});

	UWacomFirstPersonCardLayerSlotWidget* Outgoing = Layer->GetOutgoingSlotWidgetAtForTest(0);
	if (TestNotNull(TEXT("Outgoing played slot"), Outgoing))
	{
		TestTrue(TEXT("Commit feedback starts on played outgoing"), Outgoing->IsCommitFeedbackActiveForTest());
		TestEqual(TEXT("Commit overlay opacity"), Outgoing->GetFeedbackOverlayRenderOpacityForTest(), 0.6f);
		Layer->TickSlotMotionForTest(0.1f);
		TestTrue(TEXT("Played card exits upward"), Outgoing->GetVisualSlotView().ScreenPosition.Y < BasePosition.Y);
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerMissingTargetFallbackTest,
	"Wacom.UI.FirstPersonCardLayer.PlayCommit.MissingTargetWidgetFallsBackToDefaultPlayedExit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerMissingTargetFallbackTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig MotionConfig = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	MotionConfig.bEnableReadableTransitionOrigins = false;
	MotionConfig.PlayedExitOffsetPixels = FVector2D(0.0f, -120.0f);
	MotionConfig.EnterOffsetPixels = FVector2D::ZeroVector;
	MotionConfig.EnterOpacity = 1.0f;
	Layer->SetSlotMotionConfig(MotionConfig);
	Layer->SetSlotFeedbackConfig(WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());

	const FGuid CardId = FGuid::NewGuid();
	const FVector2D BasePosition(100.0f, 200.0f);
	Layer->SetCardSlots({ WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, BasePosition) });
	Layer->TickSlotMotionForTest(1.0f);

	FWacomFirstPersonCardLayerTransitionHint Hint =
		WacomFirstPersonCardLayerSpec::MakeTransitionHint(
			CardId,
			EWacomFirstPersonCardSlotTransitionKind::Played);
	Hint.bPlayCommitFeedback = true;
	Layer->SetCardTransitionHints({ Hint });
	Layer->SetCardSlots({});

	UWacomFirstPersonCardLayerSlotWidget* Outgoing = Layer->GetOutgoingSlotWidgetAtForTest(0);
	if (TestNotNull(TEXT("Outgoing played slot"), Outgoing))
	{
		Layer->TickSlotMotionForTest(1.0f);
		TestEqual(TEXT("Missing target uses default played X"), Outgoing->GetVisualSlotView().ScreenPosition.X, BasePosition.X);
		TestEqual(TEXT("Missing target uses default played Y"), Outgoing->GetVisualSlotView().ScreenPosition.Y, BasePosition.Y - 120.0f);
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerCommandFailureNoCommitHintTest,
	"Wacom.UI.FirstPersonCardLayer.PlayCommit.CommandFailureDoesNotTriggerCommitOrTargetConfirm",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerCommandFailureNoCommitHintTest::RunTest(const FString& Parameters)
{
	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Previous = Session->BuildSnapshot();

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetSession(Session);
	const FGuid FakeCardId = FGuid::NewGuid();
	HUD->OnCardClickedByUser(FakeCardId);
	HUD->StoreFirstPersonCardTransitionEventsForTest({
		WacomFirstPersonCardLayerSpec::MakeBattleEvent(EBattleEventType::CardPlayed, FakeCardId)
	});
	const TArray<FWacomFirstPersonCardLayerTransitionHint> Hints =
		HUD->BuildFirstPersonCardTransitionHintsForTest(Previous, Session->BuildSnapshot());

	TestEqual(TEXT("Failed command records no transition/commit hints"), Hints.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerCommitHintOneShotTest,
	"Wacom.UI.FirstPersonCardLayer.PlayCommit.CommitHintIsOneShot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerCommitHintOneShotTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig MotionConfig = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	MotionConfig.EnterOffsetPixels = FVector2D::ZeroVector;
	MotionConfig.EnterOpacity = 1.0f;
	Layer->SetSlotMotionConfig(MotionConfig);
	Layer->SetSlotFeedbackConfig(WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());

	const FGuid FirstCardId = FGuid::NewGuid();
	const FGuid SecondCardId = FGuid::NewGuid();
	Layer->SetCardSlots({ WacomFirstPersonCardLayerSpec::MakeMotionSlot(FirstCardId, 0, FVector2D(100.0f, 200.0f)) });
	Layer->TickSlotMotionForTest(1.0f);

	FWacomFirstPersonCardLayerTransitionHint Hint =
		WacomFirstPersonCardLayerSpec::MakeTransitionHint(
			FirstCardId,
			EWacomFirstPersonCardSlotTransitionKind::Played);
	Hint.bPlayCommitFeedback = true;
	Layer->SetCardTransitionHints({ Hint });
	Layer->SetCardSlots({});
	UWacomFirstPersonCardLayerSlotWidget* FirstOutgoing = Layer->GetOutgoingSlotWidgetAtForTest(0);
	TestTrue(TEXT("First outgoing gets commit"), FirstOutgoing && FirstOutgoing->IsCommitFeedbackActiveForTest());
	Layer->TickSlotMotionForTest(0.25f);

	Layer->SetCardSlots({ WacomFirstPersonCardLayerSpec::MakeMotionSlot(SecondCardId, 0, FVector2D(100.0f, 200.0f)) });
	Layer->TickSlotMotionForTest(1.0f);
	Layer->SetCardSlots({});
	UWacomFirstPersonCardLayerSlotWidget* SecondOutgoing = Layer->GetOutgoingSlotWidgetAtForTest(0);
	if (TestNotNull(TEXT("Second outgoing slot"), SecondOutgoing))
	{
		TestFalse(TEXT("Commit hint does not leak to next refresh"), SecondOutgoing->IsCommitFeedbackActiveForTest());
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerTargetBiasedCommitExitTest,
	"Wacom.UI.FirstPersonCardLayer.PlayCommit.TargetWidgetPositionCanBiasPlayedExit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerTargetBiasedCommitExitTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig MotionConfig = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	MotionConfig.bEnableReadableTransitionOrigins = false;
	MotionConfig.PlayedExitOffsetPixels = FVector2D(0.0f, -120.0f);
	MotionConfig.EnterOffsetPixels = FVector2D::ZeroVector;
	MotionConfig.EnterOpacity = 1.0f;
	Layer->SetSlotMotionConfig(MotionConfig);
	Layer->SetSlotFeedbackConfig(WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());

	const FGuid CardId = FGuid::NewGuid();
	const FVector2D BasePosition(100.0f, 200.0f);
	Layer->SetCardSlots({ WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, BasePosition) });
	Layer->TickSlotMotionForTest(1.0f);

	FWacomFirstPersonCardLayerTransitionHint Hint =
		WacomFirstPersonCardLayerSpec::MakeTransitionHint(
			CardId,
			EWacomFirstPersonCardSlotTransitionKind::Played);
	Hint.bPlayCommitFeedback = true;
	Hint.bHasPlayedExitTargetWidgetPosition = true;
	Hint.PlayedExitTargetWidgetPosition = FVector2D(600.0f, 100.0f);
	Layer->SetCardTransitionHints({ Hint });
	Layer->SetCardSlots({});

	UWacomFirstPersonCardLayerSlotWidget* Outgoing = Layer->GetOutgoingSlotWidgetAtForTest(0);
	if (TestNotNull(TEXT("Outgoing played slot"), Outgoing))
	{
		Layer->TickSlotMotionForTest(0.1f);
		TestTrue(TEXT("Target bias nudges played exit toward target X"), Outgoing->GetVisualSlotView().ScreenPosition.X > BasePosition.X);
		TestTrue(TEXT("Target-biased played card still exits upward"), Outgoing->GetVisualSlotView().ScreenPosition.Y < BasePosition.Y);
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerCommitFeedbackClearsTest,
	"Wacom.UI.FirstPersonCardLayer.PlayCommit.CommitFeedbackClearsOnReuseExitAndInteractionDisabled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerCommitFeedbackClearsTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	SlotWidget->SetSlotFeedbackConfig(WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid()));
	SlotWidget->TriggerCommitFeedback();
	TestTrue(TEXT("Commit feedback starts"), SlotWidget->IsCommitFeedbackActiveForTest());

	SlotWidget->SetCardLayerInteractionEnabled(false);
	TestFalse(TEXT("Interaction disabled clears commit"), SlotWidget->IsCommitFeedbackActiveForTest());

	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->TriggerCommitFeedback();
	TestTrue(TEXT("Commit feedback restarts"), SlotWidget->IsCommitFeedbackActiveForTest());
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid()));
	TestFalse(TEXT("Slot reuse clears commit"), SlotWidget->IsCommitFeedbackActiveForTest());

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerPlayedTransitionTest,
	"Wacom.UI.FirstPersonCardLayer.EventAwareTransitions.PlayedCardUsesPlayExitOffset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerPlayedTransitionTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.bEnableReadableTransitionOrigins = false;
	Config.EnterOffsetPixels = FVector2D::ZeroVector;
	Config.EnterOpacity = 1.0f;
	Layer->SetSlotMotionConfig(Config);

	const FGuid CardId = FGuid::NewGuid();
	const FVector2D BasePosition(100.0f, 200.0f);
	Layer->SetCardSlots({ WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, BasePosition) });
	Layer->TickSlotMotionForTest(1.0f);
	Layer->SetCardTransitionHints({
		WacomFirstPersonCardLayerSpec::MakeTransitionHint(CardId, EWacomFirstPersonCardSlotTransitionKind::Played)
	});
	Layer->SetCardSlots({});

	UWacomFirstPersonCardLayerSlotWidget* Outgoing = Layer->GetOutgoingSlotWidgetAtForTest(0);
	if (TestNotNull(TEXT("Outgoing played slot"), Outgoing))
	{
		Layer->TickSlotMotionForTest(0.1f);
		TestTrue(TEXT("Played card exits upward"), Outgoing->GetVisualSlotView().ScreenPosition.Y < BasePosition.Y);
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDiscardedTransitionTest,
	"Wacom.UI.FirstPersonCardLayer.EventAwareTransitions.DiscardedCardUsesDiscardExitOffset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDiscardedTransitionTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.bEnableReadableTransitionOrigins = false;
	Config.EnterOffsetPixels = FVector2D::ZeroVector;
	Config.EnterOpacity = 1.0f;
	Layer->SetSlotMotionConfig(Config);

	const FGuid CardId = FGuid::NewGuid();
	const FVector2D BasePosition(100.0f, 200.0f);
	Layer->SetCardSlots({ WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, BasePosition) });
	Layer->TickSlotMotionForTest(1.0f);
	Layer->SetCardTransitionHints({
		WacomFirstPersonCardLayerSpec::MakeTransitionHint(CardId, EWacomFirstPersonCardSlotTransitionKind::Discarded)
	});
	Layer->SetCardSlots({});

	UWacomFirstPersonCardLayerSlotWidget* Outgoing = Layer->GetOutgoingSlotWidgetAtForTest(0);
	if (TestNotNull(TEXT("Outgoing discarded slot"), Outgoing))
	{
		Layer->TickSlotMotionForTest(0.1f);
		TestTrue(TEXT("Discarded card exits downward"), Outgoing->GetVisualSlotView().ScreenPosition.Y > BasePosition.Y);
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerReorderDefaultMotionTest,
	"Wacom.UI.FirstPersonCardLayer.EventAwareTransitions.ReorderUsesDefaultMotionWithoutEnterExit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerReorderDefaultMotionTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.EnterOffsetPixels = FVector2D::ZeroVector;
	Config.EnterOpacity = 1.0f;
	Layer->SetSlotMotionConfig(Config);

	const FGuid FirstId = FGuid::NewGuid();
	const FGuid SecondId = FGuid::NewGuid();
	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(FirstId, 0, FVector2D(100.0f, 200.0f)),
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SecondId, 1, FVector2D(220.0f, 200.0f))
	});
	Layer->TickSlotMotionForTest(1.0f);
	UWacomFirstPersonCardLayerSlotWidget* FirstWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* SecondWidget = Layer->GetSlotWidgetAt(1);

	Layer->SetCardTransitionHints({
		WacomFirstPersonCardLayerSpec::MakeTransitionHint(FirstId, EWacomFirstPersonCardSlotTransitionKind::Drawn)
	});
	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SecondId, 0, FVector2D(100.0f, 200.0f)),
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(FirstId, 1, FVector2D(220.0f, 200.0f))
	});

	TestEqual(TEXT("Reorder reuses second widget"), Layer->GetSlotWidgetAt(0), SecondWidget);
	TestEqual(TEXT("Reorder reuses first widget"), Layer->GetSlotWidgetAt(1), FirstWidget);
	TestEqual(TEXT("Reorder does not create outgoing"), Layer->GetOutgoingCardViewCount(), 0);
	if (UWacomFirstPersonCardLayerSlotWidget* ReorderedFirst = Layer->GetSlotWidgetAt(1))
	{
		TestTrue(TEXT("Existing card does not replay draw enter offset"), ReorderedFirst->GetVisualSlotView().ScreenPosition.Y < 260.0f);
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerTransitionHintsOneShotTest,
	"Wacom.UI.FirstPersonCardLayer.EventAwareTransitions.HintsAreOneShotAndDoNotLeakToNextRefresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerTransitionHintsOneShotTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.bEnableReadableTransitionOrigins = false;
	Layer->SetSlotMotionConfig(Config);
	const FGuid FirstId = FGuid::NewGuid();
	const FGuid SecondId = FGuid::NewGuid();
	const FVector2D TargetPosition(100.0f, 200.0f);
	Layer->SetCardTransitionHints({
		WacomFirstPersonCardLayerSpec::MakeTransitionHint(FirstId, EWacomFirstPersonCardSlotTransitionKind::Drawn)
	});
	Layer->SetCardSlots({ WacomFirstPersonCardLayerSpec::MakeMotionSlot(FirstId, 0, TargetPosition) });
	TestEqual(TEXT("First hinted card uses draw offset"), Layer->GetSlotWidgetAt(0)->GetVisualSlotView().ScreenPosition, TargetPosition + FVector2D(0.0f, 96.0f));
	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(FirstId, 0, TargetPosition),
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SecondId, 1, FVector2D(220.0f, 200.0f))
	});
	if (UWacomFirstPersonCardLayerSlotWidget* SecondWidget = Layer->GetSlotWidgetAt(1))
	{
		TestEqual(TEXT("Second unhinted card uses default enter offset"), SecondWidget->GetVisualSlotView().ScreenPosition, FVector2D(220.0f, 240.0f));
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerSlotOffsetOriginTest,
	"Wacom.UI.FirstPersonCardLayer.TransitionOrigin.SlotOffsetPreservesV0QBehavior",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerSlotOffsetOriginTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.DrawnEnterOriginMode = EWacomFirstPersonCardTransitionOriginMode::SlotOffset;
	Layer->SetSlotMotionConfig(Config);
	const FGuid CardId = FGuid::NewGuid();
	const FVector2D TargetPosition(100.0f, 200.0f);
	Layer->SetCardTransitionHints({
		WacomFirstPersonCardLayerSpec::MakeTransitionHint(CardId, EWacomFirstPersonCardSlotTransitionKind::Drawn)
	});
	Layer->SetCardSlots({ WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, TargetPosition) });

	if (UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0))
	{
		TestEqual(TEXT("SlotOffset uses target plus transition offset"), SlotWidget->GetVisualSlotView().ScreenPosition, TargetPosition + Config.DrawnEnterOffsetPixels);
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDrawnHandAnchorOriginTest,
	"Wacom.UI.FirstPersonCardLayer.TransitionOrigin.DrawnCardCanEnterFromHandAnchorOrigin",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDrawnHandAnchorOriginTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.DrawnEnterOriginMode = EWacomFirstPersonCardTransitionOriginMode::HandAnchorOffset;
	Config.DrawnEnterOffsetPixels = FVector2D(0.0f, 90.0f);
	Layer->SetSlotMotionConfig(Config);
	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(220.0f, 260.0f));
	Slot.AnchorWidgetPosition = FVector2D(140.0f, 180.0f);
	Layer->SetCardTransitionHints({
		WacomFirstPersonCardLayerSpec::MakeTransitionHint(CardId, EWacomFirstPersonCardSlotTransitionKind::Drawn)
	});
	Layer->SetCardSlots({ Slot });

	if (UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0))
	{
		TestEqual(TEXT("Drawn card starts from hand anchor origin"), SlotWidget->GetVisualSlotView().ScreenPosition, Slot.AnchorWidgetPosition + Config.DrawnEnterOffsetPixels);
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerGainedHandAnchorOriginTest,
	"Wacom.UI.FirstPersonCardLayer.TransitionOrigin.GainedCardCanEnterFromHandAnchorOrigin",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerGainedHandAnchorOriginTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.GainedEnterOriginMode = EWacomFirstPersonCardTransitionOriginMode::HandAnchorOffset;
	Config.GainedEnterOffsetPixels = FVector2D(0.0f, -110.0f);
	Layer->SetSlotMotionConfig(Config);
	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(260.0f, 300.0f));
	Slot.AnchorWidgetPosition = FVector2D(180.0f, 230.0f);
	Layer->SetCardTransitionHints({
		WacomFirstPersonCardLayerSpec::MakeTransitionHint(CardId, EWacomFirstPersonCardSlotTransitionKind::Gained)
	});
	Layer->SetCardSlots({ Slot });

	if (UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0))
	{
		TestEqual(TEXT("Gained card starts from hand anchor origin"), SlotWidget->GetVisualSlotView().ScreenPosition, Slot.AnchorWidgetPosition + Config.GainedEnterOffsetPixels);
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerViewportFallbackOriginTest,
	"Wacom.UI.FirstPersonCardLayer.TransitionOrigin.ViewportAnchorFallbackUsesSlotOffsetWhenViewportUnavailable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerViewportFallbackOriginTest::RunTest(const FString& Parameters)
{
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(GetTransientPackage());
	if (!TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.DrawnEnterOriginMode = EWacomFirstPersonCardTransitionOriginMode::ViewportAnchor;
	Config.DrawnEnterOffsetPixels = FVector2D(12.0f, 34.0f);
	Layer->SetSlotMotionConfig(Config);
	const FGuid CardId = FGuid::NewGuid();
	const FVector2D TargetPosition(120.0f, 220.0f);
	Layer->SetCardTransitionHints({
		WacomFirstPersonCardLayerSpec::MakeTransitionHint(CardId, EWacomFirstPersonCardSlotTransitionKind::Drawn)
	});
	Layer->SetCardSlots({ WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, TargetPosition) });

	if (UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0))
	{
		TestEqual(TEXT("Missing viewport falls back to slot offset"), SlotWidget->GetVisualSlotView().ScreenPosition, TargetPosition + Config.DrawnEnterOffsetPixels);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerViewportAnchorOriginTest,
	"Wacom.UI.FirstPersonCardLayer.TransitionOrigin.ViewportAnchorOriginUsesWidgetSpaceViewport",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerViewportAnchorOriginTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	Layer->SetViewportSizeOverrideForTest(FVector2D(1000.0f, 800.0f));
	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.DrawnEnterOriginMode = EWacomFirstPersonCardTransitionOriginMode::ViewportAnchor;
	Config.DrawnEnterViewportAnchor = FVector2D(0.25f, 0.75f);
	Config.DrawnEnterOffsetPixels = FVector2D(10.0f, -20.0f);
	Layer->SetSlotMotionConfig(Config);
	const FGuid CardId = FGuid::NewGuid();
	Layer->SetCardTransitionHints({
		WacomFirstPersonCardLayerSpec::MakeTransitionHint(CardId, EWacomFirstPersonCardSlotTransitionKind::Drawn)
	});
	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(400.0f, 500.0f))
	});

	if (UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0))
	{
		TestEqual(TEXT("Viewport anchor resolves in widget-space"), SlotWidget->GetVisualSlotView().ScreenPosition, FVector2D(260.0f, 580.0f));
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerTransitionAccentTest,
	"Wacom.UI.FirstPersonCardLayer.TransitionOrigin.TransitionScaleAndAngleAccentApplyOnlyToVisualState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerTransitionAccentTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.DrawnEnterOriginMode = EWacomFirstPersonCardTransitionOriginMode::SlotOffset;
	Config.DrawnEnterScaleMultiplier = 0.8f;
	Config.DrawnEnterAngleOffsetDegrees = 6.0f;
	Layer->SetSlotMotionConfig(Config);
	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(180.0f, 240.0f), 10.0f, 0.75f);
	Layer->SetCardTransitionHints({
		WacomFirstPersonCardLayerSpec::MakeTransitionHint(CardId, EWacomFirstPersonCardSlotTransitionKind::Drawn)
	});
	Layer->SetCardSlots({ Slot });

	if (UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0))
	{
		TestEqual(TEXT("Target scale unchanged"), SlotWidget->GetSlotView().RenderScale, Slot.RenderScale);
		TestEqual(TEXT("Target angle unchanged"), SlotWidget->GetSlotView().RenderAngleDegrees, Slot.RenderAngleDegrees);
		TestEqual(TEXT("Visual scale uses transition accent"), SlotWidget->GetVisualSlotView().RenderScale, Slot.RenderScale * Config.DrawnEnterScaleMultiplier);
		TestEqual(TEXT("Visual angle uses transition accent"), SlotWidget->GetVisualSlotView().RenderAngleDegrees, Slot.RenderAngleDegrees + Config.DrawnEnterAngleOffsetDegrees);
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerPlayedDiscardedReadableOriginTest,
	"Wacom.UI.FirstPersonCardLayer.TransitionOrigin.PlayedAndDiscardedExitUseReadableOriginProfiles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerPlayedDiscardedReadableOriginTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.EnterOffsetPixels = FVector2D::ZeroVector;
	Config.EnterOpacity = 1.0f;
	Config.PlayedExitOriginMode = EWacomFirstPersonCardTransitionOriginMode::HandAnchorOffset;
	Config.PlayedExitOffsetPixels = FVector2D(0.0f, -140.0f);
	Config.PlayedExitScaleMultiplier = 0.7f;
	Config.DiscardedExitOriginMode = EWacomFirstPersonCardTransitionOriginMode::SlotOffset;
	Config.DiscardedExitOffsetPixels = FVector2D(0.0f, 140.0f);
	Layer->SetSlotMotionConfig(Config);

	const FGuid PlayedId = FGuid::NewGuid();
	const FGuid DiscardedId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView PlayedSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(PlayedId, 0, FVector2D(120.0f, 260.0f), 0.0f, 1.0f);
	PlayedSlot.AnchorWidgetPosition = FVector2D(200.0f, 220.0f);
	FWacomFirstPersonCardLayerSlotView DiscardedSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(DiscardedId, 1, FVector2D(240.0f, 260.0f), 0.0f, 1.0f);
	DiscardedSlot.AnchorWidgetPosition = FVector2D(200.0f, 220.0f);
	Layer->SetCardSlots({ PlayedSlot, DiscardedSlot });
	Layer->TickSlotMotionForTest(1.0f);
	Layer->SetCardTransitionHints({
		WacomFirstPersonCardLayerSpec::MakeTransitionHint(PlayedId, EWacomFirstPersonCardSlotTransitionKind::Played),
		WacomFirstPersonCardLayerSpec::MakeTransitionHint(DiscardedId, EWacomFirstPersonCardSlotTransitionKind::Discarded)
	});
	Layer->SetCardSlots({});

	UWacomFirstPersonCardLayerSlotWidget* PlayedOutgoing = Layer->GetOutgoingSlotWidgetAtForTest(0);
	UWacomFirstPersonCardLayerSlotWidget* DiscardedOutgoing = Layer->GetOutgoingSlotWidgetAtForTest(1);
	if (TestNotNull(TEXT("Played outgoing"), PlayedOutgoing))
	{
		Layer->TickSlotMotionForTest(0.1f);
		TestTrue(TEXT("Played readable exit moves toward hand anchor upper origin"), PlayedOutgoing->GetVisualSlotView().ScreenPosition.Y < PlayedSlot.ScreenPosition.Y);
		TestTrue(TEXT("Played readable exit scale accent applies"), PlayedOutgoing->GetVisualSlotView().RenderScale < PlayedSlot.RenderScale);
	}
	if (TestNotNull(TEXT("Discarded outgoing"), DiscardedOutgoing))
	{
		TestTrue(TEXT("Discarded readable exit moves downward"), DiscardedOutgoing->GetVisualSlotView().ScreenPosition.Y > DiscardedSlot.ScreenPosition.Y);
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerReadableOriginsDisabledTest,
	"Wacom.UI.FirstPersonCardLayer.TransitionOrigin.ReadableOriginsDisabledFallsBackToEventAwareOffsets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerReadableOriginsDisabledTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.bEnableReadableTransitionOrigins = false;
	Config.DrawnEnterOriginMode = EWacomFirstPersonCardTransitionOriginMode::HandAnchorOffset;
	Config.DrawnEnterOffsetPixels = FVector2D(0.0f, 80.0f);
	Config.DrawnEnterScaleMultiplier = 0.5f;
	Config.DrawnEnterAngleOffsetDegrees = 12.0f;
	Layer->SetSlotMotionConfig(Config);
	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(200.0f, 240.0f), 4.0f, 0.9f);
	Slot.AnchorWidgetPosition = FVector2D(100.0f, 120.0f);
	Layer->SetCardTransitionHints({
		WacomFirstPersonCardLayerSpec::MakeTransitionHint(CardId, EWacomFirstPersonCardSlotTransitionKind::Drawn)
	});
	Layer->SetCardSlots({ Slot });

	if (UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0))
	{
		TestEqual(TEXT("Readable origins disabled uses slot offset"), SlotWidget->GetVisualSlotView().ScreenPosition, Slot.ScreenPosition + Config.DrawnEnterOffsetPixels);
		TestEqual(TEXT("Readable origins disabled ignores scale accent"), SlotWidget->GetVisualSlotView().RenderScale, Slot.RenderScale);
		TestEqual(TEXT("Readable origins disabled ignores angle accent"), SlotWidget->GetVisualSlotView().RenderAngleDegrees, Slot.RenderAngleDegrees);
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerTransitionOriginPresetTest,
	"Wacom.UI.FirstPersonCardLayer.TransitionOrigin.LayoutPresetOverridesTransitionOriginProfiles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerTransitionOriginPresetTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	UWacomFirstPersonCardLayoutPreset* Preset = WacomFirstPersonCardLayerSpec::MakeLayoutPreset(GetTransientPackage());
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor)
		|| !TestNotNull(TEXT("Preset"), Preset))
	{
		return false;
	}

	Anchor->bDrawStaticCardLayer = true;
	Anchor->bUseFirstPersonCardLayoutPreset = true;
	Anchor->FirstPersonCardLayoutPreset = Preset;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	Anchor->RefreshStaticLayerForTest();

	if (const UWacomFirstPersonCardLayerWidget* Layer = Anchor->GetStaticCardLayerWidgetForTest())
	{
		const FWacomFirstPersonCardSlotMotionConfig& MotionConfig = Layer->GetSlotMotionConfigForTest();
		TestEqual(TEXT("Preset readable origins"), MotionConfig.bEnableReadableTransitionOrigins, Preset->bEnableReadableTransitionOrigins);
		TestEqual(TEXT("Preset gained origin mode"), MotionConfig.GainedEnterOriginMode, Preset->GainedCardEnterOriginMode);
		TestEqual(TEXT("Preset gained viewport anchor"), MotionConfig.GainedEnterViewportAnchor, Preset->GainedCardEnterViewportAnchor);
		TestEqual(TEXT("Preset gained scale accent"), MotionConfig.GainedEnterScaleMultiplier, Preset->GainedCardEnterScaleMultiplier);
		TestEqual(TEXT("Preset gained angle accent"), MotionConfig.GainedEnterAngleOffsetDegrees, Preset->GainedCardEnterAngleOffsetDegrees);
		TestEqual(TEXT("Preset played exit origin"), MotionConfig.PlayedExitOriginMode, Preset->PlayedCardExitOriginMode);
		TestEqual(TEXT("Preset played exit viewport anchor"), MotionConfig.PlayedExitViewportAnchor, Preset->PlayedCardExitViewportAnchor);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerExactGainHintTest,
	"Wacom.UI.FirstPersonCardLayer.EventAwareTransitions.ExactCardGainedHintWinsOverDrawCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerExactGainHintTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}

	FHandCardSnapshot Existing = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(nullptr, 1, true);
	FHandCardSnapshot Gained = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(nullptr, 1, true);
	FHandCardSnapshot Drawn = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(nullptr, 1, true);
	const FBattleSnapshot Previous = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({ Existing });
	const FBattleSnapshot Next = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({ Existing, Gained, Drawn });

	HUD->StoreFirstPersonCardTransitionEventsForTest({
		WacomFirstPersonCardLayerSpec::MakeBattleEvent(EBattleEventType::CardsDrawn, FGuid(), 2),
		WacomFirstPersonCardLayerSpec::MakeBattleEvent(EBattleEventType::CardGained, Gained.InstanceId)
	});
	const TArray<FWacomFirstPersonCardLayerTransitionHint> Hints =
		HUD->BuildFirstPersonCardTransitionHintsForTest(Previous, Next);

	const FWacomFirstPersonCardLayerTransitionHint* GainedHint =
		WacomFirstPersonCardLayerSpec::FindTransitionHint(Hints, Gained.InstanceId);
	const FWacomFirstPersonCardLayerTransitionHint* DrawnHint =
		WacomFirstPersonCardLayerSpec::FindTransitionHint(Hints, Drawn.InstanceId);
	TestNotNull(TEXT("Gained hint exists"), GainedHint);
	TestNotNull(TEXT("Drawn hint exists"), DrawnHint);
	if (GainedHint)
	{
		TestEqual(TEXT("Exact gained hint wins"), GainedHint->TransitionKind, EWacomFirstPersonCardSlotTransitionKind::Gained);
	}
	if (DrawnHint)
	{
		TestEqual(TEXT("Remaining new card gets draw hint"), DrawnHint->TransitionKind, EWacomFirstPersonCardSlotTransitionKind::Drawn);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerMissingEventDefaultMotionTest,
	"Wacom.UI.FirstPersonCardLayer.EventAwareTransitions.MissingEventFallsBackToDefaultMotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerMissingEventDefaultMotionTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}

	FHandCardSnapshot Existing = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(nullptr, 1, true);
	FHandCardSnapshot NewCard = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(nullptr, 1, true);
	const FBattleSnapshot Previous = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({ Existing });
	const FBattleSnapshot Next = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({ Existing, NewCard });
	const TArray<FWacomFirstPersonCardLayerTransitionHint> Hints =
		HUD->BuildFirstPersonCardTransitionHintsForTest(Previous, Next);
	TestEqual(TEXT("No event produces no hint"), Hints.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDrawHintAssignmentTest,
	"Wacom.UI.FirstPersonCardLayer.EventAwareTransitions.CardsDrawnEventAssignsDrawHintsToNewSnapshotCards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDrawHintAssignmentTest::RunTest(const FString& Parameters)
{
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}

	FHandCardSnapshot Existing = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(nullptr, 1, true);
	FHandCardSnapshot FirstDrawn = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(nullptr, 1, true);
	FHandCardSnapshot SecondDrawn = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(nullptr, 1, true);
	const FBattleSnapshot Previous = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({ Existing });
	const FBattleSnapshot Next = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({ FirstDrawn, Existing, SecondDrawn });

	HUD->StoreFirstPersonCardTransitionEventsForTest({
		WacomFirstPersonCardLayerSpec::MakeBattleEvent(EBattleEventType::CardsDrawn, FGuid(), 2)
	});
	const TArray<FWacomFirstPersonCardLayerTransitionHint> Hints =
		HUD->BuildFirstPersonCardTransitionHintsForTest(Previous, Next);

	TestEqual(TEXT("Two draw hints assigned"), Hints.Num(), 2);
	if (const FWacomFirstPersonCardLayerTransitionHint* FirstHint =
		WacomFirstPersonCardLayerSpec::FindTransitionHint(Hints, FirstDrawn.InstanceId))
	{
		TestEqual(TEXT("First new card draw hint"), FirstHint->TransitionKind, EWacomFirstPersonCardSlotTransitionKind::Drawn);
	}
	else
	{
		AddError(TEXT("Missing first draw hint"));
	}
	if (const FWacomFirstPersonCardLayerTransitionHint* SecondHint =
		WacomFirstPersonCardLayerSpec::FindTransitionHint(Hints, SecondDrawn.InstanceId))
	{
		TestEqual(TEXT("Second new card draw hint"), SecondHint->TransitionKind, EWacomFirstPersonCardSlotTransitionKind::Drawn);
	}
	else
	{
		AddError(TEXT("Missing second draw hint"));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerRemoveHintAssignmentTest,
	"Wacom.UI.FirstPersonCardLayer.EventAwareTransitions.CardPlayedAndHandLimitDiscardedAssignExitHints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerRemoveHintAssignmentTest::RunTest(const FString& Parameters)
{
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}

	FHandCardSnapshot Played = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(nullptr, 1, true);
	FHandCardSnapshot Kept = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(nullptr, 1, true);
	FHandCardSnapshot Discarded = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(nullptr, 1, true);
	const FBattleSnapshot Previous = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({ Played, Kept, Discarded });
	const FBattleSnapshot Next = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({ Kept });

	HUD->StoreFirstPersonCardTransitionEventsForTest({
		WacomFirstPersonCardLayerSpec::MakeBattleEvent(EBattleEventType::CardPlayed, Played.InstanceId),
		WacomFirstPersonCardLayerSpec::MakeBattleEvent(EBattleEventType::HandLimitDiscarded, Discarded.InstanceId)
	});
	const TArray<FWacomFirstPersonCardLayerTransitionHint> Hints =
		HUD->BuildFirstPersonCardTransitionHintsForTest(Previous, Next);

	const FWacomFirstPersonCardLayerTransitionHint* PlayedHint =
		WacomFirstPersonCardLayerSpec::FindTransitionHint(Hints, Played.InstanceId);
	const FWacomFirstPersonCardLayerTransitionHint* DiscardedHint =
		WacomFirstPersonCardLayerSpec::FindTransitionHint(Hints, Discarded.InstanceId);
	TestNotNull(TEXT("Played hint exists"), PlayedHint);
	TestNotNull(TEXT("Discarded hint exists"), DiscardedHint);
	if (PlayedHint)
	{
		TestEqual(TEXT("Played card gets played hint"), PlayedHint->TransitionKind, EWacomFirstPersonCardSlotTransitionKind::Played);
	}
	if (DiscardedHint)
	{
		TestEqual(TEXT("Discarded card gets discard hint"), DiscardedHint->TransitionKind, EWacomFirstPersonCardSlotTransitionKind::Discarded);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerPoisonFangGainHintTest,
	"Wacom.UI.FirstPersonCardLayer.EventAwareTransitions.CardGainedAssignsGainHintForPoisonFangReward",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerPoisonFangGainHintTest::RunTest(const FString& Parameters)
{
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}

	UCardDefinition* PoisonFang = WacomFirstPersonCardLayerSpec::MakePreviewCard(
		GetTransientPackage(),
		TEXT("Reward.PoisonFang"),
		0);
	FHandCardSnapshot Existing = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(nullptr, 1, true);
	FHandCardSnapshot Gained = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(PoisonFang, 0, true);
	const FBattleSnapshot Previous = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({ Existing });
	const FBattleSnapshot Next = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({ Existing, Gained });

	FBattleEvent GainEvent = WacomFirstPersonCardLayerSpec::MakeBattleEvent(EBattleEventType::CardGained, Gained.InstanceId);
	GainEvent.CardDefinition = PoisonFang;
	HUD->StoreFirstPersonCardTransitionEventsForTest({ GainEvent });
	const TArray<FWacomFirstPersonCardLayerTransitionHint> Hints =
		HUD->BuildFirstPersonCardTransitionHintsForTest(Previous, Next);

	const FWacomFirstPersonCardLayerTransitionHint* GainHint =
		WacomFirstPersonCardLayerSpec::FindTransitionHint(Hints, Gained.InstanceId);
	TestNotNull(TEXT("Poison Fang gain hint exists"), GainHint);
	if (GainHint)
	{
		TestEqual(TEXT("Poison Fang uses gained transition"), GainHint->TransitionKind, EWacomFirstPersonCardSlotTransitionKind::Gained);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerTransitionHoverDetailVisualTest,
	"Wacom.UI.FirstPersonCardLayer.EventAwareTransitions.HoverPendingAndDetailFollowStillUseVisualSlot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerTransitionHoverDetailVisualTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.EnterOffsetPixels = FVector2D::ZeroVector;
	Config.EnterOpacity = 1.0f;
	Layer->SetSlotMotionConfig(Config);
	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView BaseSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(100.0f, 300.0f));
	BaseSlot.bIsHovered = true;
	Layer->SetCardSlots({ BaseSlot });
	Layer->TickSlotMotionForTest(1.0f);

	WacomFirstPersonCardLayerSpec::FLayerLayoutUpdateReceiver Receiver;
	Layer->OnHoveredCardSlotUpdatedNative.AddRaw(
		&Receiver,
		&WacomFirstPersonCardLayerSpec::FLayerLayoutUpdateReceiver::HandleUpdated);
	FWacomFirstPersonCardLayerSlotView PendingSlot = BaseSlot;
	PendingSlot.ScreenPosition = FVector2D(100.0f, 220.0f);
	PendingSlot.WidgetPosition = PendingSlot.ScreenPosition;
	PendingSlot.SnappedWidgetPosition = PendingSlot.ScreenPosition;
	PendingSlot.Entry.bIsPendingTargeting = true;
	PendingSlot.RenderScale = 1.15f;
	Layer->SetCardSlots({ PendingSlot });

	TestEqual(TEXT("Hovered layout update fired"), Receiver.UpdateCount, 1);
	TestTrue(TEXT("Detail follow uses animated visual position"), Receiver.LastSlotView.ScreenPosition.Y > PendingSlot.ScreenPosition.Y);
	TestEqual(TEXT("Detail follow keeps hovered id"), Receiver.LastCardId, CardId);
	Layer->OnHoveredCardSlotUpdatedNative.RemoveAll(&Receiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerMotionDebugCountsTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotionRobustness.DebugViewReportsSlotLifecycleCounts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerMotionDebugCountsTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.EnterOffsetPixels = FVector2D::ZeroVector;
	Config.EnterOpacity = 1.0f;
	Layer->SetSlotMotionConfig(Config);

	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(FGuid::NewGuid(), 0, FVector2D(100.0f, 200.0f)),
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(FGuid::NewGuid(), 1, FVector2D(220.0f, 200.0f))
	});

	const FWacomFirstPersonCardLayerMotionDebugView Debug = Layer->GetSlotMotionDebugView();
	TestEqual(TEXT("Input slot count"), Debug.InputSlotCount, 2);
	TestEqual(TEXT("Active slot count"), Debug.ActiveSlotCount, 2);
	TestEqual(TEXT("Outgoing slot count"), Debug.OutgoingSlotCount, 0);
	TestEqual(TEXT("Root child count"), Debug.RootCanvasChildCount, 2);
	TestEqual(TEXT("Created count"), Debug.CreatedThisUpdate, 2);
	TestEqual(TEXT("Motion ticking count"), Debug.MotionTickSlotCount, 2);
	TestFalse(TEXT("No invariant violation"), Debug.bHadInvariantViolation);
	TestTrue(TEXT("Summary contains counts"), Layer->GetSlotMotionDebugSummary().Contains(TEXT("Active=2")));

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerRepeatedInsertionsNoLeakTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotionRobustness.RepeatedInsertionsDoNotLeakWidgets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerRepeatedInsertionsNoLeakTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.EnterOffsetPixels = FVector2D::ZeroVector;
	Config.EnterOpacity = 1.0f;
	Layer->SetSlotMotionConfig(Config);

	TArray<FGuid> CardIds;
	for (int32 Count = 1; Count <= 10; ++Count)
	{
		CardIds.Insert(FGuid::NewGuid(), 0);
		TArray<FWacomFirstPersonCardLayerSlotView> Slots;
		for (int32 Index = 0; Index < CardIds.Num(); ++Index)
		{
			Slots.Add(WacomFirstPersonCardLayerSpec::MakeMotionSlot(
				CardIds[Index],
				Index,
				FVector2D(100.0f + Index * 80.0f, 200.0f)));
		}
		Layer->SetCardSlots(Slots);
		const FWacomFirstPersonCardLayerMotionDebugView Debug = Layer->GetSlotMotionDebugView();
		TestEqual(FString::Printf(TEXT("Active count after insertion %d"), Count), Debug.ActiveSlotCount, Count);
		TestEqual(FString::Printf(TEXT("Root child count after insertion %d"), Count), Debug.RootCanvasChildCount, Count);
		TestEqual(FString::Printf(TEXT("Outgoing count after insertion %d"), Count), Debug.OutgoingSlotCount, 0);
		TestFalse(FString::Printf(TEXT("No invariant violation after insertion %d"), Count), Debug.bHadInvariantViolation);
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerRemoveReaddNoGhostTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotionRobustness.RepeatedRemoveAndReaddReusesOrCleansCorrectly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerRemoveReaddNoGhostTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.EnterOffsetPixels = FVector2D::ZeroVector;
	Config.EnterOpacity = 1.0f;
	Config.ExitDuration = 0.2f;
	Layer->SetSlotMotionConfig(Config);

	const FGuid FirstId = FGuid::NewGuid();
	const FGuid SecondId = FGuid::NewGuid();
	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(FirstId, 0, FVector2D(100.0f, 200.0f)),
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SecondId, 1, FVector2D(220.0f, 200.0f))
	});
	Layer->TickSlotMotionForTest(1.0f);
	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SecondId, 0, FVector2D(100.0f, 200.0f))
	});
	TestEqual(TEXT("One active after remove"), Layer->GetSlotMotionDebugView().ActiveSlotCount, 1);
	TestEqual(TEXT("One outgoing after remove"), Layer->GetSlotMotionDebugView().OutgoingSlotCount, 1);
	Layer->TickSlotMotionForTest(0.25f);
	TestEqual(TEXT("Outgoing cleaned after duration"), Layer->GetSlotMotionDebugView().OutgoingSlotCount, 0);

	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(FirstId, 0, FVector2D(100.0f, 200.0f)),
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SecondId, 1, FVector2D(220.0f, 200.0f))
	});
	const FWacomFirstPersonCardLayerMotionDebugView Debug = Layer->GetSlotMotionDebugView();
	TestEqual(TEXT("Two active after readd"), Debug.ActiveSlotCount, 2);
	TestEqual(TEXT("Root children match active after readd"), Debug.RootCanvasChildCount, 2);
	TestFalse(TEXT("No invariant violation after readd"), Debug.bHadInvariantViolation);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerReaddOutgoingReclaimsWidgetTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotionRobustness.ReaddedOutgoingCardReclaimsWidget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerReaddOutgoingReclaimsWidgetTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.EnterOffsetPixels = FVector2D::ZeroVector;
	Config.EnterOpacity = 1.0f;
	Config.ExitDuration = 1.0f;
	Layer->SetSlotMotionConfig(Config);

	const FGuid CardId = FGuid::NewGuid();
	Layer->SetCardSlots({ WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(100.0f, 200.0f)) });
	Layer->TickSlotMotionForTest(1.0f);
	UWacomFirstPersonCardLayerSlotWidget* OriginalWidget = Layer->GetSlotWidgetAt(0);

	Layer->SetCardSlots({});
	TestEqual(TEXT("Card is outgoing after remove"), Layer->GetSlotMotionDebugView().OutgoingSlotCount, 1);
	TestEqual(TEXT("Outgoing keeps original widget"), Layer->GetOutgoingSlotWidgetAtForTest(0), OriginalWidget);

	Layer->SetCardSlots({ WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(140.0f, 200.0f)) });

	const FWacomFirstPersonCardLayerMotionDebugView Debug = Layer->GetSlotMotionDebugView();
	TestEqual(TEXT("Readded card reclaims original widget"), Layer->GetSlotWidgetAt(0), OriginalWidget);
	TestEqual(TEXT("Readded card active count"), Debug.ActiveSlotCount, 1);
	TestEqual(TEXT("Readded card outgoing count"), Debug.OutgoingSlotCount, 0);
	TestEqual(TEXT("Readded card root children count"), Debug.RootCanvasChildCount, 1);
	TestEqual(TEXT("Readded card reuses widget"), Debug.ReusedThisUpdate, 1);
	TestFalse(TEXT("Readded outgoing card no invariant violation"), Debug.bHadInvariantViolation);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerHoveredInsertionStableTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotionRobustness.HoveredInsertionKeepsHoverStable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerHoveredInsertionStableTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.EnterOffsetPixels = FVector2D::ZeroVector;
	Config.EnterOpacity = 1.0f;
	Layer->SetSlotMotionConfig(Config);

	const FGuid HoveredId = FGuid::NewGuid();
	const FGuid OtherId = FGuid::NewGuid();
	const FGuid InsertedId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView HoveredSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(HoveredId, 0, FVector2D(100.0f, 180.0f));
	HoveredSlot.bIsHovered = true;
	Layer->SetCardSlots({
		HoveredSlot,
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(OtherId, 1, FVector2D(220.0f, 200.0f))
	});
	UWacomFirstPersonCardLayerSlotWidget* HoveredWidget = Layer->GetSlotWidgetAt(0);
	Layer->TickSlotMotionForTest(1.0f);

	HoveredSlot.Index = 1;
	HoveredSlot.ScreenPosition = FVector2D(220.0f, 180.0f);
	HoveredSlot.WidgetPosition = HoveredSlot.ScreenPosition;
	HoveredSlot.SnappedWidgetPosition = HoveredSlot.ScreenPosition;
	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(InsertedId, 0, FVector2D(100.0f, 200.0f)),
		HoveredSlot,
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(OtherId, 2, FVector2D(340.0f, 200.0f))
	});

	TestEqual(TEXT("Hovered card keeps widget identity after insertion"), Layer->GetSlotWidgetAt(1), HoveredWidget);
	TestEqual(TEXT("Hovered card id remains on same widget"), Layer->GetSlotWidgetAt(1)->GetSlotView().Entry.CardInstanceId, HoveredId);
	TestEqual(TEXT("No outgoing leak on hovered insertion"), Layer->GetSlotMotionDebugView().OutgoingSlotCount, 0);
	TestFalse(TEXT("No invariant violation on hovered insertion"), Layer->GetSlotMotionDebugView().bHadInvariantViolation);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerPendingRefreshOwnershipTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotionRobustness.PendingRefreshKeepsWidgetOwnershipStable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerPendingRefreshOwnershipTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.EnterOffsetPixels = FVector2D::ZeroVector;
	Config.EnterOpacity = 1.0f;
	Layer->SetSlotMotionConfig(Config);

	const FGuid PendingId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Base =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(PendingId, 0, FVector2D(100.0f, 200.0f));
	Layer->SetCardSlots({ Base });
	UWacomFirstPersonCardLayerSlotWidget* BaseWidget = Layer->GetSlotWidgetAt(0);

	FWacomFirstPersonCardLayerSlotView Pending = Base;
	Pending.ScreenPosition = FVector2D(100.0f, 160.0f);
	Pending.WidgetPosition = Pending.ScreenPosition;
	Pending.SnappedWidgetPosition = Pending.ScreenPosition;
	Pending.RenderScale = 1.08f;
	Pending.Entry.bIsPendingTargeting = true;
	Layer->SetCardSlots({ Pending });

	TestEqual(TEXT("Pending refresh reuses widget"), Layer->GetSlotWidgetAt(0), BaseWidget);
	TestEqual(TEXT("Pending refresh active count"), Layer->GetSlotMotionDebugView().ActiveSlotCount, 1);
	TestEqual(TEXT("Pending refresh outgoing count"), Layer->GetSlotMotionDebugView().OutgoingSlotCount, 0);
	TestFalse(TEXT("Pending refresh no invariant violation"), Layer->GetSlotMotionDebugView().bHadInvariantViolation);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDuplicateKeyDiagnosedTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotionRobustness.DuplicateKeysAreDiagnosedAndDisambiguated",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDuplicateKeyDiagnosedTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	const FGuid SharedId = FGuid::NewGuid();
	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SharedId, 0, FVector2D(100.0f, 200.0f)),
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SharedId, 1, FVector2D(220.0f, 200.0f))
	});

	const FWacomFirstPersonCardLayerMotionDebugView Debug = Layer->GetSlotMotionDebugView();
	TestEqual(TEXT("Duplicate key count"), Debug.DuplicateKeyCount, 1);
	TestEqual(TEXT("Duplicate key active slots"), Debug.ActiveSlotCount, 2);
	TestNotEqual(TEXT("Duplicate key uses different widgets"), Layer->GetSlotWidgetAt(0), Layer->GetSlotWidgetAt(1));
	TestFalse(TEXT("Duplicate key disambiguation is not invariant violation"), Debug.bHadInvariantViolation);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerInvariantRepairTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotionRobustness.InvariantRepairRemovesUntrackedChildren",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerInvariantRepairTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	const FGuid CardId = FGuid::NewGuid();
	Layer->SetCardSlots({ WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(100.0f, 200.0f)) });
	Layer->AddUntrackedSlotChildForTest();
	Layer->SetCardSlots({ WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(120.0f, 200.0f)) });

	const FWacomFirstPersonCardLayerMotionDebugView Debug = Layer->GetSlotMotionDebugView();
	TestEqual(TEXT("Untracked child removed"), Debug.UntrackedChildRemovedThisUpdate, 1);
	TestEqual(TEXT("Root child count repaired"), Debug.RootCanvasChildCount, 1);
	TestTrue(TEXT("Repair records invariant violation"), Debug.bHadInvariantViolation);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerOutgoingLimitTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotionRobustness.OutgoingLimitPreventsPerfLeak",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerOutgoingLimitTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig Config = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	Config.EnterOffsetPixels = FVector2D::ZeroVector;
	Config.EnterOpacity = 1.0f;
	Config.ExitDuration = 10.0f;
	Layer->SetSlotMotionConfig(Config);

	TArray<FWacomFirstPersonCardLayerSlotView> Slots;
	for (int32 Index = 0; Index < 24; ++Index)
	{
		Slots.Add(WacomFirstPersonCardLayerSpec::MakeMotionSlot(
			FGuid::NewGuid(),
			Index,
			FVector2D(100.0f + Index * 10.0f, 200.0f)));
	}
	Layer->SetCardSlots(Slots);
	Layer->TickSlotMotionForTest(1.0f);
	Layer->SetCardSlots({});

	const FWacomFirstPersonCardLayerMotionDebugView Debug = Layer->GetSlotMotionDebugView();
	TestEqual(TEXT("No active slots after mass remove"), Debug.ActiveSlotCount, 0);
	TestTrue(TEXT("Outgoing slots are capped"), Debug.OutgoingSlotCount <= 16);
	TestTrue(TEXT("Outgoing cap records repair"), Debug.bHadInvariantViolation);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerHoverVisualStateTest,
	"Wacom.UI.FirstPersonCardLayer.Interaction.HoverAppliesVisualState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerHoverVisualStateTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	Anchor->bDrawStaticCardLayer = true;
	Anchor->StaticCardRenderScale = 0.5f;
	Anchor->HoverLiftPixels = 30.0f;
	Anchor->HoverScale = 1.1f;
	Anchor->HoverZOrderBoost = 250;

	const FGuid CardInstanceId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerEntry Entry;
	Entry.CardInstanceId = CardInstanceId;
	Entry.CardViewData.Name = FText::FromString(TEXT("Hover"));
	Entry.bIsPlayable = true;
	Anchor->SetRuntimeCardLayerEntries(TEXT("BattleHand"), { Entry });

	const TArray<FWacomFirstPersonCardLayerSlotView> BaseSlots = Anchor->BuildActiveCardLayerSlotViewsForTest();
	TestEqual(TEXT("Base slot count"), BaseSlots.Num(), 1);
	Anchor->SetBattleHandInteractionPrototypeEnabled(true);
	Anchor->RefreshStaticLayerForTest();

	UWacomFirstPersonCardLayerWidget* Layer = Anchor->GetStaticCardLayerWidgetForTest();
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer ? Layer->GetSlotWidgetAt(0) : nullptr;
	if (TestNotNull(TEXT("Layer widget"), Layer)
		&& TestNotNull(TEXT("Slot widget"), SlotWidget)
		&& BaseSlots.Num() == 1)
	{
		WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver Receiver;
		Anchor->OnFirstPersonCardLayerCardHovered.AddRaw(
			&Receiver,
			&WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver::HandleHovered);
		Anchor->OnFirstPersonCardLayerCardUnhovered.AddRaw(
			&Receiver,
			&WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver::HandleUnhovered);

		TestTrue(TEXT("Slot hover request succeeds"), SlotWidget->RequestHoverForTest());
		TestEqual(TEXT("Anchor records hovered card"), Anchor->GetHoveredCardInstanceId(), CardInstanceId);
		TestEqual(TEXT("Hover broadcasts once"), Receiver.HoverCount, 1);

		const TArray<FWacomFirstPersonCardLayerSlotView> HoverSlots = Anchor->BuildActiveCardLayerSlotViewsForTest();
		TestEqual(TEXT("Hover slot count"), HoverSlots.Num(), 1);
		if (HoverSlots.Num() == 1)
		{
			TestEqual(TEXT("Hover scales card"), HoverSlots[0].RenderScale, BaseSlots[0].RenderScale * 1.1f);
			TestEqual(TEXT("Hover lifts card"), HoverSlots[0].ScreenPosition.Y, BaseSlots[0].ScreenPosition.Y - 30.0f);
			TestEqual(TEXT("Hover raises z-order"), HoverSlots[0].ZOrder, BaseSlots[0].ZOrder + 250);
			TestTrue(TEXT("Hover slot is marked for layout updates"), HoverSlots[0].bIsHovered);
		}

		SlotWidget->RequestUnhoverForTest();
		TestFalse(TEXT("Unhover clears hovered card"), Anchor->GetHoveredCardInstanceId().IsValid());
		TestEqual(TEXT("Unhover broadcasts once"), Receiver.UnhoverCount, 1);
		Anchor->OnFirstPersonCardLayerCardHovered.RemoveAll(&Receiver);
		Anchor->OnFirstPersonCardLayerCardUnhovered.RemoveAll(&Receiver);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerHoveredSlotLayoutUpdateTest,
	"Wacom.UI.FirstPersonCardLayer.DetailProvider.HoveredSlotLayoutUpdateBroadcasts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerHoveredSlotLayoutUpdateTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	Anchor->bDrawStaticCardLayer = true;
	Anchor->StaticCardRenderScale = 0.5f;
	const FGuid CardInstanceId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerEntry Entry;
	Entry.CardInstanceId = CardInstanceId;
	Entry.CardViewData.Name = FText::FromString(TEXT("Hovered layout"));
	Anchor->SetRuntimeCardLayerEntries(TEXT("BattleHand"), { Entry });
	Anchor->SetBattleHandInteractionPrototypeEnabled(true);
	Anchor->RefreshStaticLayerForTest();

	UWacomFirstPersonCardLayerWidget* Layer = Anchor->GetStaticCardLayerWidgetForTest();
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer ? Layer->GetSlotWidgetAt(0) : nullptr;
	if (TestNotNull(TEXT("Layer widget"), Layer)
		&& TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		WacomFirstPersonCardLayerSpec::FLayerLayoutUpdateReceiver Receiver;
		Anchor->OnFirstPersonCardLayerHoveredCardLayoutUpdated.AddRaw(
			&Receiver,
			&WacomFirstPersonCardLayerSpec::FLayerLayoutUpdateReceiver::HandleUpdated);

		TestTrue(TEXT("Slot hover succeeds"), SlotWidget->RequestHoverForTest());
		Anchor->RefreshStaticLayerForTest();
		TestEqual(TEXT("Hovered layout update broadcasts"), Receiver.UpdateCount, 1);
		TestEqual(TEXT("Hovered layout update keeps card id"), Receiver.LastCardId, CardInstanceId);
		TestTrue(TEXT("Updated slot is marked hovered"), Receiver.LastSlotView.bIsHovered);

		Anchor->OnFirstPersonCardLayerHoveredCardLayoutUpdated.RemoveAll(&Receiver);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerAnchorCardTargetBridgeTest,
	"Wacom.UI.FirstPersonCardLayer.CardTarget.AnchorForwardsCardTargetBridgeEvents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerAnchorCardTargetBridgeTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	Anchor->bDrawStaticCardLayer = true;
	Anchor->StaticCardRenderScale = 0.5f;
	const FGuid CardInstanceId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerEntry Entry;
	Entry.CardInstanceId = CardInstanceId;
	Entry.CardViewData.Name = FText::FromString(TEXT("Card Target"));
	Entry.bIsPlayable = false;
	Anchor->SetRuntimeCardLayerEntries(TEXT("BattleHand"), { Entry });
	Anchor->SetBattleHandInteractionPrototypeEnabled(true);
	Anchor->RefreshStaticLayerForTest();

	UWacomFirstPersonCardLayerWidget* Layer = Anchor->GetStaticCardLayerWidgetForTest();
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer ? Layer->GetSlotWidgetAt(0) : nullptr;
	if (TestNotNull(TEXT("Layer widget"), Layer)
		&& TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		WacomFirstPersonCardLayerSpec::FLayerCardTargetReceiver Receiver;
		Anchor->OnFirstPersonCardLayerCardTargetHovered.AddRaw(
			&Receiver,
			&WacomFirstPersonCardLayerSpec::FLayerCardTargetReceiver::HandleHovered);
		Anchor->OnFirstPersonCardLayerCardTargetUnhovered.AddRaw(
			&Receiver,
			&WacomFirstPersonCardLayerSpec::FLayerCardTargetReceiver::HandleUnhovered);

		TestTrue(TEXT("Non-playable slot hover still succeeds"), SlotWidget->RequestHoverForTest());
		TestEqual(TEXT("Anchor forwards target hover"), Receiver.HoverCount, 1);
		TestEqual(TEXT("Anchor hovered target id"), Receiver.LastHandle.CardInstanceId, CardInstanceId);
		TestEqual(TEXT("Anchor BuildCardTargetHandle returns current handle"),
			Anchor->BuildCardTargetHandle().CardInstanceId, CardInstanceId);
		TestTrue(TEXT("Anchor target source remains slot widget"),
			Anchor->BuildCardTargetHandle().SourceObject.Get() == SlotWidget);

		SlotWidget->RequestUnhoverForTest();
		TestEqual(TEXT("Anchor forwards target unhover"), Receiver.UnhoverCount, 1);
		TestFalse(TEXT("Anchor BuildCardTargetHandle clears after unhover"), Anchor->BuildCardTargetHandle().IsValid());

		Anchor->OnFirstPersonCardLayerCardTargetHovered.RemoveAll(&Receiver);
		Anchor->OnFirstPersonCardLayerCardTargetUnhovered.RemoveAll(&Receiver);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerClickBroadcastTest,
	"Wacom.UI.FirstPersonCardLayer.Interaction.ClickBroadcastsValidPlayableCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerClickBroadcastTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Layer widget"), Layer))
	{
		return false;
	}

	const FGuid CardInstanceId = FGuid::NewGuid();
	WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver Receiver;
	Layer->OnCardClickedNative.AddRaw(
		&Receiver,
		&WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver::HandleClicked);
	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardInstanceId)
	});
	Layer->SetCardLayerInteractionEnabled(true);

	TestEqual(TEXT("Interaction on makes layer self-hit-test-invisible"), Layer->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		TestEqual(TEXT("Interaction on makes slot visible"), SlotWidget->GetVisibility(), ESlateVisibility::Visible);
		TestTrue(TEXT("Playable projected slot click succeeds"), SlotWidget->RequestClickForTest());
	}
	TestEqual(TEXT("Click broadcasts once"), Receiver.ClickCount, 1);
	TestEqual(TEXT("Click forwards card id"), Receiver.LastCardId, CardInstanceId);

	Layer->OnCardClickedNative.RemoveAll(&Receiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerInvalidClickNoopTest,
	"Wacom.UI.FirstPersonCardLayer.Interaction.InvalidOrUnplayableClickNoops",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerInvalidClickNoopTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver Receiver;
	SlotWidget->OnCardClickedNative.AddRaw(
		&Receiver,
		&WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver::HandleClicked);
	SlotWidget->SetCardLayerInteractionEnabled(true);

	SlotWidget->SetSlotView(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid()));
	TestFalse(TEXT("Invalid card id click noops"), SlotWidget->RequestClickForTest());

	SlotWidget->SetSlotView(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid(), true, false));
	TestFalse(TEXT("Unprojected slot click noops"), SlotWidget->RequestClickForTest());

	SlotWidget->SetSlotView(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid(), false, true));
	TestFalse(TEXT("Unplayable slot click noops"), SlotWidget->RequestClickForTest());
	TestEqual(TEXT("Invalid clicks do not broadcast"), Receiver.ClickCount, 0);

	SlotWidget->OnCardClickedNative.RemoveAll(&Receiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerPlayableHoverFeedbackTest,
	"Wacom.UI.FirstPersonCardLayer.PlayableFeedback.PlayableHoverAppliesHoverTransformAndTint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerPlayableHoverFeedbackTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	Anchor->StaticCardRenderScale = 1.0f;
	Anchor->HoverLiftPixels = 30.0f;
	Anchor->HoverScale = 1.1f;
	Anchor->HoverZOrderBoost = 250;
	Anchor->bEnableCardLayerPixelSnapping = false;
	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerEntry Entry;
	Entry.CardInstanceId = CardId;
	Entry.CardViewData.Name = FText::FromString(TEXT("Playable"));
	Entry.bIsPlayable = true;

	Anchor->SetRuntimeCardLayerEntries(TEXT("BattleHand"), { Entry });
	const TArray<FWacomFirstPersonCardLayerSlotView> BaseSlots = Anchor->BuildActiveCardLayerSlotViewsForTest();
	Anchor->SetHoveredCardInstanceIdForTest(CardId);
	const TArray<FWacomFirstPersonCardLayerSlotView> HoverSlots = Anchor->BuildActiveCardLayerSlotViewsForTest();

	TestEqual(TEXT("Base slot count"), BaseSlots.Num(), 1);
	TestEqual(TEXT("Hover slot count"), HoverSlots.Num(), 1);
	if (BaseSlots.Num() == 1 && HoverSlots.Num() == 1)
	{
		TestTrue(TEXT("Playable hover marks hovered"), HoverSlots[0].bIsHovered);
		TestTrue(TEXT("Playable hover raises card"), HoverSlots[0].ScreenPosition.Y < BaseSlots[0].ScreenPosition.Y);
		TestEqual(TEXT("Playable hover applies scale"), HoverSlots[0].RenderScale, 1.1f);
		TestTrue(TEXT("Playable hover boosts z-order"), HoverSlots[0].ZOrder > BaseSlots[0].ZOrder);
	}

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (TestNotNull(TEXT("Slot widget"), SlotWidget) && HoverSlots.Num() == 1)
	{
		SlotWidget->SetSlotFeedbackConfig(WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());
		SlotWidget->SetCardLayerInteractionEnabled(true);
		SlotWidget->SetSlotViewImmediate(HoverSlots[0]);
		TestTrue(TEXT("Playable hover request succeeds"), SlotWidget->RequestHoverForTest());
		TestEqual(TEXT("Playable hover tint opacity"), SlotWidget->GetFeedbackOverlayRenderOpacityForTest(), 0.2f);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerNonPlayableHoverFeedbackTest,
	"Wacom.UI.FirstPersonCardLayer.PlayableFeedback.NonPlayableHoverDoesNotApplyPlayableHoverTransform",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerNonPlayableHoverFeedbackTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	Anchor->StaticCardRenderScale = 1.0f;
	Anchor->HoverLiftPixels = 30.0f;
	Anchor->HoverScale = 1.1f;
	Anchor->HoverZOrderBoost = 250;
	Anchor->DisabledRenderOpacity = 0.7f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerEntry Entry;
	Entry.CardInstanceId = CardId;
	Entry.CardViewData.Name = FText::FromString(TEXT("Blocked"));
	Entry.bIsPlayable = false;

	Anchor->SetRuntimeCardLayerEntries(TEXT("BattleHand"), { Entry });
	const TArray<FWacomFirstPersonCardLayerSlotView> BaseSlots = Anchor->BuildActiveCardLayerSlotViewsForTest();
	Anchor->SetHoveredCardInstanceIdForTest(CardId);
	const TArray<FWacomFirstPersonCardLayerSlotView> HoverSlots = Anchor->BuildActiveCardLayerSlotViewsForTest();

	TestEqual(TEXT("Base slot count"), BaseSlots.Num(), 1);
	TestEqual(TEXT("Hover slot count"), HoverSlots.Num(), 1);
	if (BaseSlots.Num() == 1 && HoverSlots.Num() == 1)
	{
		TestTrue(TEXT("Non-playable hover still marks hovered"), HoverSlots[0].bIsHovered);
		TestEqual(TEXT("Non-playable hover keeps position"), HoverSlots[0].ScreenPosition, BaseSlots[0].ScreenPosition);
		TestEqual(TEXT("Non-playable hover keeps scale"), HoverSlots[0].RenderScale, BaseSlots[0].RenderScale);
		TestEqual(TEXT("Non-playable hover keeps z-order"), HoverSlots[0].ZOrder, BaseSlots[0].ZOrder);
		TestEqual(TEXT("Non-playable keeps disabled opacity"), HoverSlots[0].RenderOpacity, 0.7f);
	}

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (TestNotNull(TEXT("Slot widget"), SlotWidget) && HoverSlots.Num() == 1)
	{
		SlotWidget->SetSlotFeedbackConfig(WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());
		SlotWidget->SetCardLayerInteractionEnabled(true);
		SlotWidget->SetSlotViewImmediate(HoverSlots[0]);
		TestTrue(TEXT("Non-playable hover request succeeds"), SlotWidget->RequestHoverForTest());
		TestEqual(TEXT("Non-playable hover does not tint as playable"), SlotWidget->GetFeedbackOverlayRenderOpacityForTest(), 0.0f);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerPressFeedbackTest,
	"Wacom.UI.FirstPersonCardLayer.PlayableFeedback.PlayablePressDoesNotBroadcastUntilMouseUp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerPressFeedbackTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver Receiver;
	SlotWidget->OnCardClickedNative.AddRaw(
		&Receiver,
		&WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver::HandleClicked);
	SlotWidget->SetSlotFeedbackConfig(WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid()));

	TestTrue(TEXT("Press succeeds"), SlotWidget->RequestPressForTest());
	TestTrue(TEXT("Pressed flag is set"), SlotWidget->IsPressedForFirstPersonLayerForTest());
	TestEqual(TEXT("Press does not broadcast"), Receiver.ClickCount, 0);
	TestTrue(TEXT("Pressed scale applies"), FMath::IsNearlyEqual(SlotWidget->GetRenderTransform().Scale.X, 0.55f * 0.9f, KINDA_SMALL_NUMBER));
	TestEqual(TEXT("Pressed overlay opacity"), SlotWidget->GetFeedbackOverlayRenderOpacityForTest(), 0.3f);

	SlotWidget->OnCardClickedNative.RemoveAll(&Receiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerMouseUpConfirmFeedbackTest,
	"Wacom.UI.FirstPersonCardLayer.PlayableFeedback.PlayableMouseUpBroadcastsClickAndConfirmFeedback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerMouseUpConfirmFeedbackTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	const FGuid CardId = FGuid::NewGuid();
	WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver Receiver;
	SlotWidget->OnCardClickedNative.AddRaw(
		&Receiver,
		&WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver::HandleClicked);
	SlotWidget->SetSlotFeedbackConfig(WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId));

	TestTrue(TEXT("Press succeeds"), SlotWidget->RequestPressForTest());
	TestTrue(TEXT("Mouse up succeeds"), SlotWidget->RequestMouseUpForTest());
	TestFalse(TEXT("Mouse up clears pressed"), SlotWidget->IsPressedForFirstPersonLayerForTest());
	TestEqual(TEXT("Mouse up broadcasts once"), Receiver.ClickCount, 1);
	TestEqual(TEXT("Mouse up forwards card id"), Receiver.LastCardId, CardId);
	TestTrue(TEXT("Confirm feedback starts"), SlotWidget->IsConfirmFeedbackActiveForTest());
	TestEqual(TEXT("Confirm overlay opacity"), SlotWidget->GetFeedbackOverlayRenderOpacityForTest(), 0.4f);
	SlotWidget->TickSlotMotionForTest(0.2f);
	TestFalse(TEXT("Confirm feedback expires"), SlotWidget->IsConfirmFeedbackActiveForTest());

	SlotWidget->OnCardClickedNative.RemoveAll(&Receiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDenyFeedbackTest,
	"Wacom.UI.FirstPersonCardLayer.PlayableFeedback.NonPlayableClickPlaysDenyFeedbackWithoutBroadcast",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDenyFeedbackTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver Receiver;
	SlotWidget->OnCardClickedNative.AddRaw(
		&Receiver,
		&WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver::HandleClicked);
	SlotWidget->SetSlotFeedbackConfig(WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid(), false, true));

	TestTrue(TEXT("Press succeeds for non-playable interactable slot"), SlotWidget->RequestPressForTest());
	TestTrue(TEXT("Mouse up is consumed"), SlotWidget->RequestMouseUpForTest());
	TestEqual(TEXT("Deny does not broadcast"), Receiver.ClickCount, 0);
	TestTrue(TEXT("Deny feedback starts"), SlotWidget->IsDenyFeedbackActiveForTest());
	TestEqual(TEXT("Deny overlay opacity"), SlotWidget->GetFeedbackOverlayRenderOpacityForTest(), 0.5f);
	SlotWidget->TickSlotMotionForTest(0.05f);
	TestTrue(TEXT("Deny shake changes translation"), FMath::Abs(SlotWidget->GetRenderTransform().Translation.X) > KINDA_SMALL_NUMBER);
	SlotWidget->TickSlotMotionForTest(0.2f);
	TestFalse(TEXT("Deny feedback expires"), SlotWidget->IsDenyFeedbackActiveForTest());

	SlotWidget->OnCardClickedNative.RemoveAll(&Receiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerFeedbackClearsTest,
	"Wacom.UI.FirstPersonCardLayer.PlayableFeedback.FeedbackClearsOnLeaveReuseExitAndInteractionDisabled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerFeedbackClearsTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	SlotWidget->SetSlotFeedbackConfig(WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid()));
	TestTrue(TEXT("Press starts"), SlotWidget->RequestPressForTest());
	SlotWidget->RequestUnhoverForTest();
	TestFalse(TEXT("Unhover clears pressed"), SlotWidget->IsPressedForFirstPersonLayerForTest());

	TestTrue(TEXT("Press restarts"), SlotWidget->RequestPressForTest());
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid()));
	TestFalse(TEXT("Reuse clears pressed"), SlotWidget->IsPressedForFirstPersonLayerForTest());

	TestTrue(TEXT("Press starts before exit"), SlotWidget->RequestPressForTest());
	SlotWidget->BeginExitMotion(SlotWidget->GetSlotView());
	TestFalse(TEXT("Exit clears pressed"), SlotWidget->IsPressedForFirstPersonLayerForTest());
	TestEqual(TEXT("Exit clears overlay"), SlotWidget->GetFeedbackOverlayRenderOpacityForTest(), 0.0f);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid()));

	TestTrue(TEXT("Press starts before disable"), SlotWidget->RequestPressForTest());
	SlotWidget->SetCardLayerInteractionEnabled(false);
	TestFalse(TEXT("Disabling interaction clears pressed"), SlotWidget->IsPressedForFirstPersonLayerForTest());
	TestEqual(TEXT("Disabling interaction clears overlay"), SlotWidget->GetFeedbackOverlayRenderOpacityForTest(), 0.0f);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerFeedbackDisabledTest,
	"Wacom.UI.FirstPersonCardLayer.PlayableFeedback.FeedbackDisabledRestoresCurrentBehavior",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerFeedbackDisabledTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver Receiver;
	FWacomFirstPersonCardSlotFeedbackConfig FeedbackConfig = WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig();
	FeedbackConfig.bEnabled = false;
	SlotWidget->OnCardClickedNative.AddRaw(
		&Receiver,
		&WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver::HandleClicked);
	SlotWidget->SetSlotFeedbackConfig(FeedbackConfig);
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid()));

	TestTrue(TEXT("Press still consumes interactable slot"), SlotWidget->RequestPressForTest());
	TestEqual(TEXT("Feedback overlay stays hidden"), SlotWidget->GetFeedbackOverlayRenderOpacityForTest(), 0.0f);
	TestTrue(TEXT("Click still succeeds"), SlotWidget->RequestMouseUpForTest());
	TestEqual(TEXT("Click path still broadcasts"), Receiver.ClickCount, 1);
	TestFalse(TEXT("Confirm feedback stays disabled"), SlotWidget->IsConfirmFeedbackActiveForTest());

	SlotWidget->OnCardClickedNative.RemoveAll(&Receiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerCardOrderTest,
	"Wacom.UI.FirstPersonCardLayer.Anchor.CardTransformsStayOrdered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerCardOrderTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	PC->Possess(Character);
	Anchor->ProbeCameraTransform = FTransform(
		FRotator::ZeroRotator,
		FVector(100.0f, 200.0f, 300.0f),
		FVector::OneVector);
	Anchor->RefreshAnchor(0.0f);
	const FTransform Left = Anchor->ComputeCardTransform(5, 0);
	const FTransform Center = Anchor->ComputeCardTransform(5, 2);
	const FTransform Right = Anchor->ComputeCardTransform(5, 4);
	TestTrue(TEXT("Left slot is left of center in anchor right axis"), Left.GetLocation().Y < Center.GetLocation().Y);
	TestTrue(TEXT("Right slot is right of center in anchor right axis"), Right.GetLocation().Y > Center.GetLocation().Y);

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerInvalidProjectionTest,
	"Wacom.UI.FirstPersonCardLayer.Anchor.InvalidProjectionNoops",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerInvalidProjectionTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	PC->Possess(Character);
	Anchor->bProjectionSucceeds = false;
	Anchor->ProjectionMode = EWacomFirstPersonCardProjectionMode::LegacyWorldProjected;
	Anchor->RefreshAnchor(0.0f);
	FWacomFirstPersonCardProjectedPoint Point;
	TestFalse(TEXT("Projection failure returns false"), Anchor->ProjectCardTransformToScreen(Anchor->ComputeCardTransform(5, 2), Point, 2));
	TestFalse(TEXT("Projection failure leaves point unprojected"), Point.bProjected);

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerRuntimeSourcePriorityTest,
	"Wacom.UI.FirstPersonCardLayer.BattleHandAdapter.RuntimeSourceOverridesStaticLayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerRuntimeSourcePriorityTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	Anchor->bDrawStaticCardLayer = true;
	Anchor->StaticCardCountFallback = 5;
	TestEqual(TEXT("Static fallback has five cards"), Anchor->BuildActiveCardLayerSlotViewsForTest().Num(), 5);

	FWacomCardViewData RuntimeCard;
	RuntimeCard.Name = FText::FromString(TEXT("Runtime Battle Card"));
	Anchor->SetRuntimeCardLayerData(TEXT("BattleHand"), { RuntimeCard });
	const TArray<FWacomFirstPersonCardLayerSlotView> RuntimeSlots = Anchor->BuildActiveCardLayerSlotViewsForTest();
	TestEqual(TEXT("Runtime source overrides static fallback"), RuntimeSlots.Num(), 1);
	if (RuntimeSlots.Num() == 1)
	{
		TestEqual(TEXT("Runtime card data is used"), RuntimeSlots[0].Entry.CardViewData.Name.ToString(), FString(TEXT("Runtime Battle Card")));
	}

	Anchor->ClearRuntimeCardLayerData(TEXT("BattleHand"));
	TestEqual(TEXT("Clearing runtime source restores static fallback"), Anchor->BuildActiveCardLayerSlotViewsForTest().Num(), 5);

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerEmptyRuntimeSourceTest,
	"Wacom.UI.FirstPersonCardLayer.BattleHandAdapter.EmptyRuntimeSourceDoesNotFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerEmptyRuntimeSourceTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	Anchor->bDrawStaticCardLayer = true;
	Anchor->StaticCardCountFallback = 5;
	Anchor->SetRuntimeCardLayerData(TEXT("BattleHand"), {});
	TestTrue(TEXT("Empty runtime source is still active"), Anchor->HasRuntimeCardLayerData());
	TestEqual(TEXT("Empty runtime hand shows no cards"), Anchor->BuildActiveCardLayerSlotViewsForTest().Num(), 0);

	Anchor->ClearRuntimeCardLayerData(TEXT("BattleHand"));
	TestEqual(TEXT("Static fallback returns after clearing empty runtime hand"), Anchor->BuildActiveCardLayerSlotViewsForTest().Num(), 5);

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerBattleHUDDisabledTest,
	"Wacom.UI.FirstPersonCardLayer.BattleHandPresentation.LegacyModeDoesNotWriteRuntimeHand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerBattleHUDDisabledTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	UCardDefinition* Card = WacomFirstPersonCardLayerSpec::MakePreviewCard(GetTransientPackage(), TEXT("Battle.Disabled"), 2);
	FWacomBattleFixture Fixture;
	UBattleSession* Session = WacomFirstPersonCardLayerSpec::CreateMinimalBattleSession(Fixture);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("Card"), Card)
		|| !TestNotNull(TEXT("Session"), Session))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeBattleHUDWithCharacter(HUD, PC, Character, World);
	HUD->SetBattleHandPresentationModeForTest(EWacomBattleHandPresentationMode::LegacyHandPanel);
	HUD->SetSession(Session);
	const FBattleSnapshot Snapshot = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({
		WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(Card, 3, true)
	});
	HUD->SyncFirstPersonBattleHandLayerForTest(Snapshot);
	TestFalse(TEXT("Legacy presentation mode does not write runtime hand"), Character->GetFirstPersonCardAnchorComponent()->HasRuntimeCardLayerData());

	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerBattleHUDWritesHandTest,
	"Wacom.UI.FirstPersonCardLayer.BattleHandState.IdentityPreserved",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerBattleHUDWritesHandTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	UCardDefinition* FirstCard = WacomFirstPersonCardLayerSpec::MakePreviewCard(GetTransientPackage(), TEXT("Battle.Alpha"), 1);
	UCardDefinition* SecondCard = WacomFirstPersonCardLayerSpec::MakePreviewCard(GetTransientPackage(), TEXT("Battle.Beta"), 2);
	FWacomBattleFixture Fixture;
	UBattleSession* Session = WacomFirstPersonCardLayerSpec::CreateMinimalBattleSession(Fixture);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("First card"), FirstCard)
		|| !TestNotNull(TEXT("Second card"), SecondCard)
		|| !TestNotNull(TEXT("Session"), Session))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeBattleHUDWithCharacter(HUD, PC, Character, World);
	HUD->SetSession(Session);
	UWacomFirstPersonCardAnchorComponent* Anchor = Character->GetFirstPersonCardAnchorComponent();
	Anchor->RefreshAnchor(0.0f);
	FHandCardSnapshot FirstSnapshot = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(FirstCard, 7, true);
	FirstSnapshot.Zone = EHandZone::Left;
	FHandCardSnapshot SecondSnapshot = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(SecondCard, 9, false);
	SecondSnapshot.Zone = EHandZone::Right;
	SecondSnapshot.bIsHandAnchor = true;
	const FBattleSnapshot Snapshot = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({ FirstSnapshot, SecondSnapshot });
	HUD->SyncFirstPersonBattleHandLayerForTest(Snapshot);

	TestTrue(TEXT("Runtime hand source is active"), Anchor->HasRuntimeCardLayerData());
	TestEqual(TEXT("Runtime hand source id"), Anchor->GetRuntimeCardLayerSourceId(), FName(TEXT("BattleHand")));
	TestEqual(TEXT("Runtime hand card count"), Anchor->GetRuntimeCardLayerCardCount(), 2);
	const TArray<FWacomFirstPersonCardLayerEntry>& RuntimeEntries = Anchor->GetRuntimeCardLayerEntries();
	TestEqual(TEXT("Runtime entry count"), RuntimeEntries.Num(), 2);
	if (RuntimeEntries.Num() == 2)
	{
		TestEqual(TEXT("First identity preserves hand order"), RuntimeEntries[0].CardInstanceId, FirstSnapshot.InstanceId);
		TestEqual(TEXT("Second identity preserves hand order"), RuntimeEntries[1].CardInstanceId, SecondSnapshot.InstanceId);
		TestEqual(TEXT("First zone is preserved"), RuntimeEntries[0].Zone, EHandZone::Left);
		TestEqual(TEXT("Second zone is preserved"), RuntimeEntries[1].Zone, EHandZone::Right);
		TestFalse(TEXT("First card is not anchor"), RuntimeEntries[0].bIsHandAnchor);
		TestTrue(TEXT("Second hand-anchor state is preserved"), RuntimeEntries[1].bIsHandAnchor);
		TestEqual(TEXT("First card view preserves hand order"), RuntimeEntries[0].CardViewData.Name.ToString(), FString(TEXT("Battle.Alpha")));
		TestEqual(TEXT("Runtime cost overrides base cost"), RuntimeEntries[0].CardViewData.Cost, 7);
		TestFalse(TEXT("Playable card is not disabled"), RuntimeEntries[0].CardViewData.bDisabled);
		TestEqual(TEXT("Second runtime cost overrides base cost"), RuntimeEntries[1].CardViewData.Cost, 9);
		TestTrue(TEXT("Unplayable card is disabled"), RuntimeEntries[1].CardViewData.bDisabled);
		TestFalse(TEXT("First card is not pending by default"), RuntimeEntries[0].bIsPendingTargeting);
		TestFalse(TEXT("Second card is not pending by default"), RuntimeEntries[1].bIsPendingTargeting);
	}

	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerBattleHUDClickIntentTest,
	"Wacom.UI.FirstPersonCardLayer.Interaction.BattleHUDClickIntentEntersExistingFlow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerBattleHUDClickIntentTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* LeftHand = Fx.MakeNoopCard(0);
	UCardDefinition* RightHand = Fx.MakeNoopCard(0);
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCardDefinition* NoTargetCard = Fx.MakeNoopCard(0);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(LeftHand, RightHand, { TargetCard, NoTargetCard });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Enemy, 1);

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("Session"), Session))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeBattleHUDWithCharacter(HUD, PC, Character, World);
	HUD->TakeWidget();
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerSpec::SettleBattlePresentationQueue(*HUD);
	HUD->SetBattleHandPresentationModeForTest(EWacomBattleHandPresentationMode::FirstPersonHandOnly);
	const FBattleSnapshot InitialSnapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = WacomFirstPersonCardLayerSpec::FindFirstHandCardByTargetMode(
		InitialSnapshot,
		ECardTargetMode::SingleEnemyPart);
	const FGuid NoTargetCardId = WacomFirstPersonCardLayerSpec::FindFirstHandCardByTargetMode(
		InitialSnapshot,
		ECardTargetMode::None);
	TestTrue(TEXT("Targeting card is in hand"), TargetCardId.IsValid());
	TestTrue(TEXT("No-target card is in hand"), NoTargetCardId.IsValid());
	if (!TargetCardId.IsValid() || !NoTargetCardId.IsValid())
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	UWacomFirstPersonCardAnchorComponent* Anchor = Character->GetFirstPersonCardAnchorComponent();
	HUD->SyncFirstPersonBattleHandLayerForTest(InitialSnapshot);
	TestTrue(TEXT("First-person hand interaction is enabled on anchor"), Anchor->IsBattleHandInteractionPrototypeEnabled());
	TestEqual(TEXT("FirstPersonHandOnly hides legacy hand while first-person interaction handles clicks"),
		HUD->GetHandPanelVisibilityForTest(),
		ESlateVisibility::Collapsed);

	Anchor->OnFirstPersonCardLayerCardClicked.Broadcast(
		TargetCardId,
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(TargetCardId));
	TestEqual(TEXT("First-person targeting card enters target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);
	TestEqual(TEXT("First-person targeting card becomes pending"), HUD->GetPendingTargetingCardId(), TargetCardId);

	const int32 VersionBeforeNoTarget = Session->BuildSnapshot().Version;
	Anchor->OnFirstPersonCardLayerCardClicked.Broadcast(
		NoTargetCardId,
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(NoTargetCardId));
	WacomFirstPersonCardLayerSpec::SettleBattlePresentationQueue(*HUD);
	TestEqual(TEXT("First-person no-target card returns idle after submit"), HUD->GetUIState(), EBattleUIState::Idle);
	TestFalse(TEXT("First-person no-target submit clears pending card"), HUD->GetPendingTargetingCardId().IsValid());
	TestTrue(TEXT("First-person no-target submit changes battle state"),
		Session->BuildSnapshot().Version > VersionBeforeNoTarget);

	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerBattleHUDCleanupTest,
	"Wacom.UI.FirstPersonCardLayer.Interaction.CleanupUnbindsInteractionDelegates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerBattleHUDCleanupTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(0, 1);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ TargetCard, Fx.MakeNoopCard(0) });
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Enemy, 1);

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("Session"), Session))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeBattleHUDWithCharacter(HUD, PC, Character, World);
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerSpec::SettleBattlePresentationQueue(*HUD);
	const FBattleSnapshot InitialSnapshot = Session->BuildSnapshot();
	const FGuid TargetCardId = WacomFirstPersonCardLayerSpec::FindFirstHandCardByTargetMode(
		InitialSnapshot,
		ECardTargetMode::SingleEnemyPart);
	TestTrue(TEXT("Targeting card is in hand"), TargetCardId.IsValid());
	if (!TargetCardId.IsValid())
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	UWacomFirstPersonCardAnchorComponent* Anchor = Character->GetFirstPersonCardAnchorComponent();
	HUD->SyncFirstPersonBattleHandLayerForTest(InitialSnapshot);
	TestTrue(TEXT("Runtime hand source is active"), Anchor->HasRuntimeCardLayerData());
	HUD->ClearFirstPersonBattleHandLayerForTest();
	TestFalse(TEXT("Runtime hand source is cleared"), Anchor->HasRuntimeCardLayerData());
	TestFalse(TEXT("Anchor interaction is disabled"), Anchor->IsBattleHandInteractionPrototypeEnabled());

	Anchor->OnFirstPersonCardLayerCardClicked.Broadcast(
		TargetCardId,
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(TargetCardId));
	TestEqual(TEXT("Cleared interaction no longer reaches HUD"), HUD->GetUIState(), EBattleUIState::Idle);
	TestFalse(TEXT("Cleared interaction does not set pending card"), HUD->GetPendingTargetingCardId().IsValid());

	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDetailProviderSwitchOffTest,
	"Wacom.UI.FirstPersonCardLayer.DetailProvider.DisabledDoesNotShowDetail",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDetailProviderSwitchOffTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	UCardDefinition* Card = WacomFirstPersonCardLayerSpec::MakePreviewCard(
		GetTransientPackage(),
		TEXT("Battle.Detail.Disabled"),
		1);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("Card"), Card))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeBattleHUDWithCharacter(HUD, PC, Character, World);
	HUD->TakeWidget();
	HUD->SetBattleHandPresentationModeForTest(EWacomBattleHandPresentationMode::LegacyHandPanel);
	const FHandCardSnapshot CardSnapshot = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(Card, 2, true);
	const FBattleSnapshot Snapshot = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({ CardSnapshot });
	HUD->RefreshFromSnapshotForTest(Snapshot);

	HUD->HandleFirstPersonCardHoveredForTest(
		CardSnapshot.InstanceId,
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardSnapshot.InstanceId));
	TestFalse(TEXT("First-person detail stays hidden in legacy presentation mode"), HUD->IsCardDetailPanelVisible());

	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDetailProviderHoverTest,
	"Wacom.UI.FirstPersonCardLayer.DetailProvider.HoverShowsSnapshotCardDetail",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDetailProviderHoverTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	UCardDefinition* FirstCard = WacomFirstPersonCardLayerSpec::MakePreviewCard(
		GetTransientPackage(),
		TEXT("第一人称详情卡 A"),
		1);
	UCardDefinition* SecondCard = WacomFirstPersonCardLayerSpec::MakePreviewCard(
		GetTransientPackage(),
		TEXT("第一人称详情卡 B"),
		2);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("First card"), FirstCard)
		|| !TestNotNull(TEXT("Second card"), SecondCard))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeBattleHUDWithCharacter(HUD, PC, Character, World);
	HUD->TakeWidget();
	const FHandCardSnapshot FirstSnapshot = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(FirstCard, 1, true);
	const FHandCardSnapshot SecondSnapshot = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(SecondCard, 2, true);
	const FBattleSnapshot Snapshot = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({ FirstSnapshot, SecondSnapshot });
	HUD->RefreshFromSnapshotForTest(Snapshot);
	TestEqual(TEXT("Detail provider starts from Idle"), HUD->GetUIStateForTest(), EBattleUIState::Idle);
	TestTrue(TEXT("Detail provider cached snapshot"), HUD->HasLastBattleSnapshotForTest());
	TestEqual(TEXT("Detail provider cached hand cards"), HUD->GetLastBattleSnapshotHandCountForTest(), 2);
	TestTrue(TEXT("Detail provider can find first card"), HUD->HasLastBattleHandCardForTest(FirstSnapshot.InstanceId));
	TestTrue(TEXT("Detail provider has card detail layer"), HUD->HasCardDetailLayerForTest());
	TestTrue(TEXT("Detail provider can create card detail panel"), HUD->EnsureCardDetailPanelForTest());
	TestTrue(TEXT("Detail provider can create first-person detail panel"), HUD->EnsureFirstPersonCardDetailPanelForTest());
	TestTrue(TEXT("First-person detail viewport z-order is above card layer"), HUD->GetFirstPersonCardDetailViewportZOrderForTest() > 9996);

	HUD->HandleFirstPersonCardHoveredForTest(
		FirstSnapshot.InstanceId,
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FirstSnapshot.InstanceId));
	TestFalse(TEXT("First-person hover waits before showing detail"), HUD->IsCardDetailPanelVisible());
	HUD->TickCardDetailMotionForTest(0.12f);
	TestTrue(TEXT("First-person hover shows detail"), HUD->IsCardDetailPanelVisible());
	TestFalse(TEXT("First-person hover does not use legacy detail panel"), HUD->IsLegacyCardDetailPanelVisibleForTest());
	TestTrue(TEXT("First-person hover uses viewport detail panel"), HUD->IsFirstPersonCardDetailPanelVisibleForTest());
	TestEqual(TEXT("First-person detail uses first snapshot definition"),
		HUD->GetCardDetailPanelNameText().ToString(),
		FString(TEXT("第一人称详情卡 A")));
	TestEqual(TEXT("First-person specific detail has first card name"),
		HUD->GetFirstPersonCardDetailPanelNameTextForTest().ToString(),
		FString(TEXT("第一人称详情卡 A")));

	HUD->HandleFirstPersonCardHoveredForTest(
		SecondSnapshot.InstanceId,
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(SecondSnapshot.InstanceId));
	HUD->TickCardDetailMotionForTest(0.01f);
	TestTrue(TEXT("Second first-person hover keeps detail visible"), HUD->IsCardDetailPanelVisible());
	TestEqual(TEXT("First-person detail replaces source"),
		HUD->GetCardDetailPanelNameText().ToString(),
		FString(TEXT("第一人称详情卡 B")));

	HUD->HandleFirstPersonCardUnhoveredForTest(
		FirstSnapshot.InstanceId,
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FirstSnapshot.InstanceId));
	HUD->TickCardDetailMotionForTest(0.01f);
	TestTrue(TEXT("Old first-person source unhover does not hide current detail"), HUD->IsCardDetailPanelVisible());
	TestEqual(TEXT("Old first-person source unhover keeps second detail"),
		HUD->GetCardDetailPanelNameText().ToString(),
		FString(TEXT("第一人称详情卡 B")));

	HUD->HandleFirstPersonCardUnhoveredForTest(
		SecondSnapshot.InstanceId,
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(SecondSnapshot.InstanceId));
	HUD->TickCardDetailMotionForTest(0.5f);
	TestFalse(TEXT("Current first-person source unhover hides detail"), HUD->IsCardDetailPanelVisible());
	TestFalse(TEXT("First-person viewport detail is hidden on current unhover"), HUD->IsFirstPersonCardDetailPanelVisibleForTest());

	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDetailProviderFollowTest,
	"Wacom.UI.FirstPersonCardLayer.DetailProvider.FollowsHoveredSlotLayoutUpdate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDetailProviderFollowTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	UCardDefinition* Card = WacomFirstPersonCardLayerSpec::MakePreviewCard(
		GetTransientPackage(),
		TEXT("第一人称跟随详情卡"),
		1);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("Card"), Card))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeBattleHUDWithCharacter(HUD, PC, Character, World);
	HUD->TakeWidget();
	const FHandCardSnapshot CardSnapshot = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(Card, 1, true);
	const FBattleSnapshot Snapshot = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({ CardSnapshot });
	HUD->RefreshFromSnapshotForTest(Snapshot);
	HUD->SetCardDetailMotionSpeedsForTest(0.0f, 18.0f, 24.0f);

	FWacomFirstPersonCardLayerSlotView InitialSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardSnapshot.InstanceId);
	InitialSlot.ScreenPosition = FVector2D(500.0f, 600.0f);
	InitialSlot.RenderScale = 1.0f;
	HUD->HandleFirstPersonCardHoveredForTest(CardSnapshot.InstanceId, InitialSlot);
	HUD->TickCardDetailMotionForTest(0.12f);
	const FVector2D InitialPosition = HUD->GetFirstPersonCardDetailPanelPositionForTest();
	TestTrue(TEXT("First-person detail visible before follow update"), HUD->IsFirstPersonCardDetailPanelVisibleForTest());

	FWacomFirstPersonCardLayerSlotView UpdatedSlot = InitialSlot;
	UpdatedSlot.ScreenPosition = FVector2D(700.0f, 600.0f);
	UpdatedSlot.bIsHovered = true;
	HUD->HandleFirstPersonCardLayoutUpdatedForTest(CardSnapshot.InstanceId, UpdatedSlot);
	HUD->TickCardDetailMotionForTest(0.01f);
	const FVector2D UpdatedPosition = HUD->GetFirstPersonCardDetailPanelPositionForTest();
	TestNotEqual(TEXT("Hovered detail position follows slot layout update"), UpdatedPosition, InitialPosition);

	FWacomFirstPersonCardLayerSlotView OtherSlot = UpdatedSlot;
	OtherSlot.ScreenPosition = FVector2D(900.0f, 600.0f);
	HUD->HandleFirstPersonCardLayoutUpdatedForTest(FGuid::NewGuid(), OtherSlot);
	HUD->TickCardDetailMotionForTest(0.01f);
	TestEqual(TEXT("Mismatched layout update does not move current detail"),
		HUD->GetFirstPersonCardDetailPanelPositionForTest(),
		UpdatedPosition);

	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDetailProviderInvalidDataTest,
	"Wacom.UI.FirstPersonCardLayer.DetailProvider.InvalidDataNoops",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDetailProviderLargeJumpResetTest,
	"Wacom.UI.FirstPersonCardLayer.DetailProvider.LargeDetailPositionJumpResetsFollow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDetailProviderLargeJumpResetTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	UCardDefinition* Card = WacomFirstPersonCardLayerSpec::MakePreviewCard(
		GetTransientPackage(),
		TEXT("第一人称详情大跳变卡"),
		1);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("Card"), Card))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeBattleHUDWithCharacter(HUD, PC, Character, World);
	HUD->TakeWidget();
	HUD->SetCardDetailMotionSpeedsForTest(1.0f, 18.0f, 24.0f);
	const FHandCardSnapshot CardSnapshot = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(Card, 1, true);
	const FBattleSnapshot Snapshot = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({ CardSnapshot });
	HUD->RefreshFromSnapshotForTest(Snapshot);

	FWacomFirstPersonCardLayerSlotView InitialSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardSnapshot.InstanceId);
	InitialSlot.ScreenPosition = FVector2D(500.0f, 600.0f);
	InitialSlot.RenderScale = 1.0f;
	HUD->HandleFirstPersonCardHoveredForTest(CardSnapshot.InstanceId, InitialSlot);
	HUD->TickCardDetailMotionForTest(0.12f);
	const FVector2D InitialPosition = HUD->GetFirstPersonCardDetailPanelPositionForTest();

	FWacomFirstPersonCardLayerSlotView FarSlot = InitialSlot;
	FarSlot.ScreenPosition = FVector2D(1900.0f, 600.0f);
	HUD->HandleFirstPersonCardLayoutUpdatedForTest(CardSnapshot.InstanceId, FarSlot);
	HUD->TickCardDetailMotionForTest(0.01f);
	const FVector2D JumpedPosition = HUD->GetFirstPersonCardDetailPanelPositionForTest();
	TestTrue(TEXT("Large detail position jump resets instead of slow drifting"),
		FVector2D::Distance(JumpedPosition, InitialPosition) > 500.0f);

	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDetailProviderSideHysteresisTest,
	"Wacom.UI.FirstPersonCardLayer.DetailProvider.SideHysteresisPreventsEdgeFlipFlop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDetailProviderSideHysteresisTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	UCardDefinition* Card = WacomFirstPersonCardLayerSpec::MakePreviewCard(
		GetTransientPackage(),
		TEXT("第一人称详情贴边卡"),
		1);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("Card"), Card))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeBattleHUDWithCharacter(HUD, PC, Character, World);
	HUD->TakeWidget();
	HUD->SetCardDetailMotionSpeedsForTest(0.0f, 18.0f, 24.0f);
	const FHandCardSnapshot CardSnapshot = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(Card, 1, true);
	const FBattleSnapshot Snapshot = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({ CardSnapshot });
	HUD->RefreshFromSnapshotForTest(Snapshot);

	FWacomFirstPersonCardLayerSlotView InitialSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardSnapshot.InstanceId);
	InitialSlot.ScreenPosition = FVector2D(530.0f, 600.0f);
	InitialSlot.RenderScale = 1.0f;
	HUD->HandleFirstPersonCardHoveredForTest(CardSnapshot.InstanceId, InitialSlot);
	HUD->TickCardDetailMotionForTest(0.12f);
	const FVector2D InitialPosition = HUD->GetFirstPersonCardDetailPanelPositionForTest();
	TestTrue(TEXT("Initial detail chooses left side when there is room"), InitialPosition.X < 80.0f);

	FWacomFirstPersonCardLayerSlotView SlightlyLeftSlot = InitialSlot;
	SlightlyLeftSlot.ScreenPosition = FVector2D(490.0f, 600.0f);
	HUD->HandleFirstPersonCardLayoutUpdatedForTest(CardSnapshot.InstanceId, SlightlyLeftSlot);
	HUD->TickCardDetailMotionForTest(0.01f);
	TestTrue(TEXT("Side hysteresis keeps near-edge detail on left side"),
		HUD->GetFirstPersonCardDetailPanelPositionForTest().X < 120.0f);

	Character->Destroy();
	PC->Destroy();
	return true;
}

bool FWacomFirstPersonCardLayerDetailProviderInvalidDataTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	UCardDefinition* Card = WacomFirstPersonCardLayerSpec::MakePreviewCard(
		GetTransientPackage(),
		TEXT("第一人称有效详情卡"),
		1);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("Card"), Card))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeBattleHUDWithCharacter(HUD, PC, Character, World);
	HUD->TakeWidget();
	FHandCardSnapshot ValidSnapshot = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(Card, 1, true);
	FHandCardSnapshot MissingDefinitionSnapshot = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(nullptr, 1, true);
	const FBattleSnapshot Snapshot = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({
		ValidSnapshot,
		MissingDefinitionSnapshot
	});
	HUD->RefreshFromSnapshotForTest(Snapshot);

	HUD->HandleFirstPersonCardHoveredForTest(
		ValidSnapshot.InstanceId,
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(ValidSnapshot.InstanceId));
	HUD->TickCardDetailMotionForTest(0.12f);
	TestTrue(TEXT("Valid first-person detail is visible before invalid hover"), HUD->IsCardDetailPanelVisible());

	HUD->HandleFirstPersonCardHoveredForTest(
		FGuid::NewGuid(),
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid()));
	TestFalse(TEXT("Missing snapshot card hides detail"), HUD->IsCardDetailPanelVisible());

	HUD->HandleFirstPersonCardHoveredForTest(
		ValidSnapshot.InstanceId,
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(ValidSnapshot.InstanceId, true, false));
	TestFalse(TEXT("Unprojected first-person slot does not show detail"), HUD->IsCardDetailPanelVisible());

	HUD->HandleFirstPersonCardHoveredForTest(
		MissingDefinitionSnapshot.InstanceId,
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(MissingDefinitionSnapshot.InstanceId));
	TestFalse(TEXT("Snapshot card without definition does not show detail"), HUD->IsCardDetailPanelVisible());

	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDetailProviderStateClearTest,
	"Wacom.UI.FirstPersonCardLayer.DetailProvider.ClearsOnTargetSelectAndBattleEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDetailProviderStateClearTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	UCardDefinition* Card = WacomFirstPersonCardLayerSpec::MakePreviewCard(
		GetTransientPackage(),
		TEXT("第一人称状态详情卡"),
		1);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("Card"), Card))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeBattleHUDWithCharacter(HUD, PC, Character, World);
	HUD->TakeWidget();
	const FHandCardSnapshot CardSnapshot = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(Card, 1, true);
	const FBattleSnapshot Snapshot = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({ CardSnapshot });
	HUD->RefreshFromSnapshotForTest(Snapshot);

	HUD->HandleFirstPersonCardHoveredForTest(
		CardSnapshot.InstanceId,
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardSnapshot.InstanceId));
	HUD->TickCardDetailMotionForTest(0.12f);
	TestTrue(TEXT("First-person detail is visible before target select"), HUD->IsCardDetailPanelVisible());

	HUD->SetTargetSelectionStateForTest(CardSnapshot.InstanceId);
	TestFalse(TEXT("Entering TargetSelect hides first-person detail"), HUD->IsCardDetailPanelVisible());

	HUD->ClearTargetSelectionStateForTest();
	HUD->RefreshFromSnapshotForTest(Snapshot);
	HUD->HandleFirstPersonCardHoveredForTest(
		CardSnapshot.InstanceId,
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardSnapshot.InstanceId));
	HUD->TickCardDetailMotionForTest(0.12f);
	TestTrue(TEXT("First-person detail can show again after returning Idle"), HUD->IsCardDetailPanelVisible());

	FBattleSnapshot BattleEndSnapshot = Snapshot;
	BattleEndSnapshot.Phase = EBattlePhase::BattleEnd;
	HUD->RefreshFromSnapshotForTest(BattleEndSnapshot);
	TestFalse(TEXT("BattleEnd snapshot hides first-person detail"), HUD->IsCardDetailPanelVisible());

	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerPendingStateRefreshTest,
	"Wacom.UI.FirstPersonCardLayer.BattleHandState.PendingTargetingStateUpdatesOnUIStateChanged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerPendingStateRefreshTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	UCardDefinition* FirstCard = WacomFirstPersonCardLayerSpec::MakePreviewCard(GetTransientPackage(), TEXT("Battle.Pending.A"), 1);
	UCardDefinition* SecondCard = WacomFirstPersonCardLayerSpec::MakePreviewCard(GetTransientPackage(), TEXT("Battle.Pending.B"), 2);
	FWacomBattleFixture Fixture;
	UBattleSession* Session = WacomFirstPersonCardLayerSpec::CreateMinimalBattleSession(Fixture);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("First card"), FirstCard)
		|| !TestNotNull(TEXT("Second card"), SecondCard)
		|| !TestNotNull(TEXT("Session"), Session))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeBattleHUDWithCharacter(HUD, PC, Character, World);
	HUD->SetSession(Session);
	TestTrue(TEXT("Default presentation mode is first-person with legacy fallback"),
		HUD->GetBattleHandPresentationMode() == EWacomBattleHandPresentationMode::FirstPersonHandWithLegacyFallback);
	UWacomFirstPersonCardAnchorComponent* Anchor = Character->GetFirstPersonCardAnchorComponent();
	FHandCardSnapshot FirstSnapshot = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(FirstCard, 3, true);
	FHandCardSnapshot SecondSnapshot = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(SecondCard, 4, true);
	const FBattleSnapshot Snapshot = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({ FirstSnapshot, SecondSnapshot });

	HUD->SyncFirstPersonBattleHandLayerForTest(Snapshot);
	TestFalse(TEXT("No card is pending before target select"), Anchor->GetRuntimeCardLayerEntries()[1].bIsPendingTargeting);

	HUD->SetTargetSelectionStateForTest(SecondSnapshot.InstanceId);
	HUD->SyncFirstPersonBattleHandLayerForTest(Snapshot);
	const TArray<FWacomFirstPersonCardLayerEntry>& PendingEntries = Anchor->GetRuntimeCardLayerEntries();
	TestEqual(TEXT("Entry count after target select"), PendingEntries.Num(), 2);
	if (PendingEntries.Num() == 2)
	{
		TestFalse(TEXT("Non-pending card remains normal"), PendingEntries[0].bIsPendingTargeting);
		TestTrue(TEXT("Pending card is marked by current HUD state"), PendingEntries[1].bIsPendingTargeting);
	}

	HUD->ClearTargetSelectionStateForTest();
	HUD->SyncFirstPersonBattleHandLayerForTest(Snapshot);
	const TArray<FWacomFirstPersonCardLayerEntry>& ClearedEntries = Anchor->GetRuntimeCardLayerEntries();
	TestEqual(TEXT("Entry count after clearing target select"), ClearedEntries.Num(), 2);
	if (ClearedEntries.Num() == 2)
	{
		TestFalse(TEXT("Pending state clears on UI state change"), ClearedEntries[1].bIsPendingTargeting);
	}

	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerVisualStateSlotTest,
	"Wacom.UI.FirstPersonCardLayer.BattleHandState.LayerAppliesPendingTransformWithoutInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerVisualStateSlotTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	Anchor->StaticCardRenderScale = 0.5f;
	Anchor->PendingTargetingLiftPixels = 40.0f;
	Anchor->PendingTargetingScale = 1.2f;
	Anchor->PendingTargetingZOrderBoost = 1200;
	Anchor->HandAnchorScale = 0.9f;
	Anchor->DisabledRenderOpacity = 0.6f;
	Anchor->TargetSelectNonPendingOpacityMultiplier = 0.5f;
	Anchor->bEnableCardLayerPixelSnapping = false;

	FWacomFirstPersonCardLayerEntry NormalEntry;
	NormalEntry.CardViewData.Name = FText::FromString(TEXT("Normal"));

	FWacomFirstPersonCardLayerEntry PendingEntry;
	PendingEntry.CardViewData.Name = FText::FromString(TEXT("Pending"));
	PendingEntry.bIsPendingTargeting = true;

	FWacomFirstPersonCardLayerEntry DisabledAnchorEntry;
	DisabledAnchorEntry.CardViewData.Name = FText::FromString(TEXT("Disabled Anchor"));
	DisabledAnchorEntry.bIsHandAnchor = true;
	DisabledAnchorEntry.bIsPlayable = false;
	DisabledAnchorEntry.CardViewData.bDisabled = true;

	Anchor->SetRuntimeCardLayerEntries(TEXT("BattleHand"), { NormalEntry, PendingEntry, DisabledAnchorEntry });
	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildActiveCardLayerSlotViewsForTest();
	TestEqual(TEXT("Runtime entries produce slots"), Slots.Num(), 3);
	if (Slots.Num() == 3)
	{
		TestTrue(TEXT("Pending card lifts above normal card"), Slots[1].ScreenPosition.Y < Slots[0].ScreenPosition.Y);
		TestEqual(TEXT("Pending card applies scale multiplier"), Slots[1].RenderScale, 0.5f * 1.2f);
		TestTrue(TEXT("Pending card gets configured higher z-order"), Slots[1].ZOrder >= 1200);
		TestEqual(TEXT("Non-pending card is deemphasized during target select"), Slots[0].RenderOpacity, 0.5f);
		TestEqual(TEXT("Disabled anchor applies anchor scale"), Slots[2].RenderScale, 0.5f * 0.9f);
		TestEqual(TEXT("Disabled card opacity composes with target select deemphasis"), Slots[2].RenderOpacity, 0.6f * 0.5f);
		TestTrue(TEXT("Disabled card view data stays disabled"), Slots[2].Entry.CardViewData.bDisabled);
	}

	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (TestNotNull(TEXT("Layer widget"), Layer))
	{
		FWacomFirstPersonCardSlotMotionConfig MotionConfig;
		MotionConfig.bEnabled = false;
		Layer->SetSlotMotionConfig(MotionConfig);
		Layer->SetCardSlots(Slots);
		TestEqual(TEXT("Layer is hit-test-invisible"), Layer->GetVisibility(), ESlateVisibility::HitTestInvisible);
		if (TestNotNull(TEXT("Pending card view exists"), Layer->GetCardViewAt(1)))
		{
			TestEqual(TEXT("Pending widget scale matches slot"), Layer->GetCardRenderTransformAt(1).Scale, FVector2D(0.5f * 1.2f));
			TestTrue(TEXT("Pending widget z-order is raised"), Layer->GetCardZOrderAt(1) > Layer->GetCardZOrderAt(2));
		}
		if (TestNotNull(TEXT("Disabled card view exists"), Layer->GetCardViewAt(2)))
		{
			TestEqual(TEXT("Disabled widget opacity matches slot"), Layer->GetCardRenderOpacityAt(2), 0.6f * 0.5f);
		}
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerPendingFocusAngleTest,
	"Wacom.UI.FirstPersonCardLayer.PendingFocus.PendingCardAngleBlendsTowardZeroWhenEnabled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerPendingFocusAngleTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	Anchor->FanYawDegrees = 12.0f;
	Anchor->bClampCardLayerRenderAngle = false;
	Anchor->bPendingTargetingStraightenAngle = false;
	Anchor->PendingTargetingAngleBlend = 0.75f;

	FWacomFirstPersonCardLayerEntry PendingEntry;
	PendingEntry.CardViewData.Name = FText::FromString(TEXT("Pending"));
	PendingEntry.bIsPendingTargeting = true;

	FWacomFirstPersonCardLayerEntry NormalEntry;
	NormalEntry.CardViewData.Name = FText::FromString(TEXT("Normal"));

	Anchor->SetRuntimeCardLayerEntries(TEXT("BattleHand"), { PendingEntry, NormalEntry });
	const TArray<FWacomFirstPersonCardLayerSlotView> UnblendedSlots = Anchor->BuildActiveCardLayerSlotViewsForTest();

	Anchor->bPendingTargetingStraightenAngle = true;
	const TArray<FWacomFirstPersonCardLayerSlotView> BlendedSlots = Anchor->BuildActiveCardLayerSlotViewsForTest();

	TestEqual(TEXT("Unblended slot count"), UnblendedSlots.Num(), 2);
	TestEqual(TEXT("Blended slot count"), BlendedSlots.Num(), 2);
	if (UnblendedSlots.Num() == 2 && BlendedSlots.Num() == 2)
	{
		TestEqual(TEXT("Pending angle keeps fan angle when disabled"), UnblendedSlots[0].RenderAngleDegrees, -6.0f);
		TestEqual(TEXT("Pending angle blends toward zero when enabled"), BlendedSlots[0].RenderAngleDegrees, -1.5f);
		TestEqual(TEXT("Non-pending angle remains unchanged"), BlendedSlots[1].RenderAngleDegrees, UnblendedSlots[1].RenderAngleDegrees);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerPendingHoverPriorityTest,
	"Wacom.UI.FirstPersonCardLayer.PendingFocus.PendingHoverDoesNotDoubleApplyLiftOrScale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerPendingHoverPriorityTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	Anchor->StaticCardRenderScale = 1.0f;
	Anchor->PendingTargetingLiftPixels = 40.0f;
	Anchor->PendingTargetingScale = 1.2f;
	Anchor->PendingTargetingZOrderBoost = 1000;
	Anchor->HoverLiftPixels = 30.0f;
	Anchor->HoverScale = 1.1f;
	Anchor->HoverZOrderBoost = 250;
	Anchor->bEnableCardLayerPixelSnapping = false;

	const FGuid PendingId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerEntry PendingEntry;
	PendingEntry.CardInstanceId = PendingId;
	PendingEntry.CardViewData.Name = FText::FromString(TEXT("Pending"));
	PendingEntry.bIsPendingTargeting = true;

	Anchor->SetRuntimeCardLayerEntries(TEXT("BattleHand"), { PendingEntry });
	const TArray<FWacomFirstPersonCardLayerSlotView> PendingSlots = Anchor->BuildActiveCardLayerSlotViewsForTest();

	Anchor->SetHoveredCardInstanceIdForTest(PendingId);
	const TArray<FWacomFirstPersonCardLayerSlotView> PendingHoverSlots = Anchor->BuildActiveCardLayerSlotViewsForTest();

	TestEqual(TEXT("Pending slot count"), PendingSlots.Num(), 1);
	TestEqual(TEXT("Pending hover slot count"), PendingHoverSlots.Num(), 1);
	if (PendingSlots.Num() == 1 && PendingHoverSlots.Num() == 1)
	{
		TestTrue(TEXT("Pending hover still marks hovered slot"), PendingHoverSlots[0].bIsHovered);
		TestEqual(TEXT("Pending hover does not add hover scale"), PendingHoverSlots[0].RenderScale, PendingSlots[0].RenderScale);
		TestEqual(TEXT("Pending hover does not add hover lift"), PendingHoverSlots[0].ScreenPosition.Y, PendingSlots[0].ScreenPosition.Y);
		TestEqual(TEXT("Pending hover does not add hover z-order"), PendingHoverSlots[0].ZOrder, PendingSlots[0].ZOrder);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerPendingPressFeedbackTest,
	"Wacom.UI.FirstPersonCardLayer.PlayableFeedback.PendingCardCanPressWithoutHoverDoubleLift",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerPendingPressFeedbackTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	const FGuid PendingId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView PendingSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(PendingId, true, true);
	PendingSlot.Entry.bIsPendingTargeting = true;
	PendingSlot.bIsHovered = true;
	PendingSlot.RenderScale = 1.2f;
	PendingSlot.ScreenPosition = FVector2D(100.0f, 200.0f);

	WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver Receiver;
	SlotWidget->OnCardClickedNative.AddRaw(
		&Receiver,
		&WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver::HandleClicked);
	SlotWidget->SetSlotFeedbackConfig(WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(PendingSlot);

	TestTrue(TEXT("Pending hover request succeeds"), SlotWidget->RequestHoverForTest());
	TestEqual(TEXT("Pending hover does not use playable hover tint"), SlotWidget->GetFeedbackOverlayRenderOpacityForTest(), 0.0f);
	TestTrue(TEXT("Pending press succeeds"), SlotWidget->RequestPressForTest());
	TestTrue(TEXT("Pending press applies only press scale"), FMath::IsNearlyEqual(SlotWidget->GetRenderTransform().Scale.X, 1.2f * 0.9f, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Pending click succeeds"), SlotWidget->RequestMouseUpForTest());
	TestEqual(TEXT("Pending click broadcasts"), Receiver.ClickCount, 1);
	TestEqual(TEXT("Pending click forwards id"), Receiver.LastCardId, PendingId);

	SlotWidget->OnCardClickedNative.RemoveAll(&Receiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerBattleHUDClearsTest,
	"Wacom.UI.FirstPersonCardLayer.BattleHandAdapter.ClearsOnBattleEndAndExplicitClear",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerBattleHUDClearsTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	UCardDefinition* Card = WacomFirstPersonCardLayerSpec::MakePreviewCard(GetTransientPackage(), TEXT("Battle.Clear"), 1);
	FWacomBattleFixture Fixture;
	UBattleSession* Session = WacomFirstPersonCardLayerSpec::CreateMinimalBattleSession(Fixture);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("Card"), Card)
		|| !TestNotNull(TEXT("Session"), Session))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeBattleHUDWithCharacter(HUD, PC, Character, World);
	HUD->SetSession(Session);
	UWacomFirstPersonCardAnchorComponent* Anchor = Character->GetFirstPersonCardAnchorComponent();
	const FBattleSnapshot ActiveSnapshot = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({
		WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(Card, 4, true)
	});
	HUD->SyncFirstPersonBattleHandLayerForTest(ActiveSnapshot);
	TestTrue(TEXT("Runtime hand source is active before clear"), Anchor->HasRuntimeCardLayerData());

	FBattleSnapshot BattleEndSnapshot = ActiveSnapshot;
	BattleEndSnapshot.Phase = EBattlePhase::BattleEnd;
	HUD->SyncFirstPersonBattleHandLayerForTest(BattleEndSnapshot);
	TestFalse(TEXT("BattleEnd clears runtime hand source"), Anchor->HasRuntimeCardLayerData());

	HUD->SyncFirstPersonBattleHandLayerForTest(ActiveSnapshot);
	TestTrue(TEXT("Runtime hand source can be set again"), Anchor->HasRuntimeCardLayerData());
	HUD->ClearFirstPersonBattleHandLayerForTest();
	TestFalse(TEXT("Explicit clear removes runtime hand source"), Anchor->HasRuntimeCardLayerData());

	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerLegacyHandVisibilityTest,
	"Wacom.UI.FirstPersonCardLayer.BattleHandPresentation.PresentationModesDriveLegacyVisibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerLegacyHandVisibilityTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	UCardDefinition* Card = WacomFirstPersonCardLayerSpec::MakePreviewCard(
		GetTransientPackage(),
		TEXT("Battle.LegacyHand.Toggle"),
		1);
	FWacomBattleFixture Fixture;
	UBattleSession* Session = WacomFirstPersonCardLayerSpec::CreateMinimalBattleSession(Fixture);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("Card"), Card)
		|| !TestNotNull(TEXT("Session"), Session))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeBattleHUDWithCharacter(HUD, PC, Character, World);
	HUD->TakeWidget();
	HUD->SetSession(Session);
	TestTrue(TEXT("Fallback HUD builds hand panel"), HUD->HasHandPanelForTest());
	const ESlateVisibility InitialHandVisibility = HUD->GetHandPanelVisibilityForTest();
	TestNotEqual(TEXT("Legacy hand starts non-collapsed"), InitialHandVisibility, ESlateVisibility::Collapsed);

	const FBattleSnapshot Snapshot = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({
		WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(Card, 1, true)
	});
	HUD->RefreshFromSnapshotForTest(Snapshot);
	TestTrue(TEXT("Default first-person fallback writes runtime hand"),
		Character->GetFirstPersonCardAnchorComponent()->HasRuntimeCardLayerData());
	TestEqual(TEXT("First-person fallback keeps legacy hand visible"),
		HUD->GetHandPanelVisibilityForTest(),
		InitialHandVisibility);

	HUD->SetBattleHandPresentationModeForTest(EWacomBattleHandPresentationMode::LegacyHandPanel);
	HUD->RefreshFromSnapshotForTest(Snapshot);
	TestFalse(TEXT("Legacy mode clears runtime hand"),
		Character->GetFirstPersonCardAnchorComponent()->HasRuntimeCardLayerData());
	TestFalse(TEXT("Legacy mode disables first-person interaction"),
		Character->GetFirstPersonCardAnchorComponent()->IsBattleHandInteractionPrototypeEnabled());
	TestEqual(TEXT("Legacy mode keeps legacy hand visible"),
		HUD->GetHandPanelVisibilityForTest(),
		InitialHandVisibility);

	HUD->SetBattleHandPresentationModeForTest(EWacomBattleHandPresentationMode::FirstPersonHandOnly);
	HUD->RefreshFromSnapshotForTest(Snapshot);
	TestTrue(TEXT("FirstPersonHandOnly writes runtime hand"),
		Character->GetFirstPersonCardAnchorComponent()->HasRuntimeCardLayerData());
	TestEqual(TEXT("FirstPersonHandOnly collapses legacy hand"),
		HUD->GetHandPanelVisibilityForTest(),
		ESlateVisibility::Collapsed);

	HUD->SetBattleHandPresentationModeForTest(EWacomBattleHandPresentationMode::FirstPersonHandWithLegacyFallback);
	HUD->SyncLegacyHandPanelVisibilityForTest();
	TestEqual(TEXT("Returning to fallback mode restores legacy hand visibility"),
		HUD->GetHandPanelVisibilityForTest(),
		InitialHandVisibility);

	HUD->SetBattleHandPresentationModeForTest(EWacomBattleHandPresentationMode::FirstPersonHandOnly);
	HUD->RefreshFromSnapshotForTest(Snapshot);
	TestEqual(TEXT("Legacy hand can collapse again in first-person only mode"), HUD->GetHandPanelVisibilityForTest(), ESlateVisibility::Collapsed);

	HUD->ClearFirstPersonBattleHandLayerForTest();
	TestEqual(TEXT("Clearing first-person runtime hand restores legacy hand"),
		HUD->GetHandPanelVisibilityForTest(),
		InitialHandVisibility);

	FBattleSnapshot BattleEndSnapshot = Snapshot;
	BattleEndSnapshot.Phase = EBattlePhase::BattleEnd;
	HUD->RefreshFromSnapshotForTest(BattleEndSnapshot);
	TestEqual(TEXT("BattleEnd leaves legacy hand restored"),
		HUD->GetHandPanelVisibilityForTest(),
		InitialHandVisibility);

	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerLegacyHandRestoreOriginalVisibilityTest,
	"Wacom.UI.FirstPersonCardLayer.LegacyHandPanelVisibility.RestoresOriginalVisibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerLegacyHandRestoreOriginalVisibilityTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	UCardDefinition* Card = WacomFirstPersonCardLayerSpec::MakePreviewCard(
		GetTransientPackage(),
		TEXT("Battle.LegacyHand.Restore"),
		1);
	FWacomBattleFixture Fixture;
	UBattleSession* Session = WacomFirstPersonCardLayerSpec::CreateMinimalBattleSession(Fixture);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("Card"), Card)
		|| !TestNotNull(TEXT("Session"), Session))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeBattleHUDWithCharacter(HUD, PC, Character, World);
	HUD->TakeWidget();
	HUD->SetHandPanelVisibilityForTest(ESlateVisibility::SelfHitTestInvisible);
	HUD->SyncLegacyHandPanelVisibilityForTest();
	HUD->SetSession(Session);
	HUD->SetBattleHandPresentationModeForTest(EWacomBattleHandPresentationMode::FirstPersonHandOnly);

	const FBattleSnapshot Snapshot = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({
		WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(Card, 1, true)
	});
	HUD->RefreshFromSnapshotForTest(Snapshot);
	TestEqual(TEXT("Interactive runtime hand collapses custom-visibility hand panel"),
		HUD->GetHandPanelVisibilityForTest(),
		ESlateVisibility::Collapsed);

	HUD->SetBattleHandPresentationModeForTest(EWacomBattleHandPresentationMode::FirstPersonHandWithLegacyFallback);
	HUD->SyncLegacyHandPanelVisibilityForTest();
	TestEqual(TEXT("Legacy hand restores captured custom visibility"),
		HUD->GetHandPanelVisibilityForTest(),
		ESlateVisibility::SelfHitTestInvisible);

	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerFirstPersonOnlyAnchorFallbackTest,
	"Wacom.UI.FirstPersonCardLayer.BattleHandPresentation.FirstPersonOnlyFallsBackWhenAnchorMissing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerFirstPersonOnlyAnchorFallbackTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	UCardDefinition* Card = WacomFirstPersonCardLayerSpec::MakePreviewCard(
		GetTransientPackage(),
		TEXT("Battle.LegacyHand.AnchorFallback"),
		1);
	FWacomBattleFixture Fixture;
	UBattleSession* Session = WacomFirstPersonCardLayerSpec::CreateMinimalBattleSession(Fixture);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("Card"), Card)
		|| !TestNotNull(TEXT("Session"), Session))
	{
		return false;
	}

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->TakeWidget();
	HUD->SetSession(Session);
	HUD->SetBattleHandPresentationModeForTest(EWacomBattleHandPresentationMode::FirstPersonHandOnly);
	TestTrue(TEXT("Fallback HUD builds hand panel"), HUD->HasHandPanelForTest());
	const ESlateVisibility InitialHandVisibility = HUD->GetHandPanelVisibilityForTest();

	const FBattleSnapshot Snapshot = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({
		WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(Card, 1, true)
	});
	HUD->RefreshFromSnapshotForTest(Snapshot);
	TestEqual(TEXT("FirstPersonHandOnly restores legacy hand when anchor is missing"),
		HUD->GetHandPanelVisibilityForTest(),
		InitialHandVisibility);
	TestFalse(TEXT("Missing anchor means no first-person anchor can be resolved"),
		HUD->ResolveFirstPersonCardAnchorForTest() != nullptr);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerClickBeforeHoldDelayTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.ClickBeforeHoldDelayPreservesClickPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerClickBeforeHoldDelayTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	const FGuid CardId = FGuid::NewGuid();
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true));

	WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver Receiver;
	SlotWidget->OnCardClickedNative.AddRaw(&Receiver, &WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver::HandleClicked);

	TestTrue(TEXT("Gesture press starts"), SlotWidget->RequestGesturePressForTest(FVector2D(500.0f, 600.0f)));
	TestTrue(TEXT("Release before delay succeeds"), SlotWidget->RequestGestureReleaseForTest(FVector2D(500.0f, 600.0f)));
	TestEqual(TEXT("Quick click broadcasts once"), Receiver.ClickCount, 1);
	TestEqual(TEXT("Quick click card id"), Receiver.LastCardId, CardId);

	SlotWidget->OnCardClickedNative.RemoveAll(&Receiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDragPointerViewportTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragCameraLook.DragViewReportsPointerViewportPosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDragPointerViewportTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	SlotWidget->SetCardDragConfig(DragConfig);
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid(), true, true));

	SlotWidget->RequestGesturePressForTest(FVector2D(960.0f, 540.0f));
	SlotWidget->RequestGestureMoveForTest(0.01f, FVector2D(1440.0f, 270.0f));
	const FWacomFirstPersonCardDragView DragView = SlotWidget->BuildDragView();

	TestTrue(TEXT("Drag view has pointer viewport position"), DragView.bHasPointerViewportPosition);
	TestEqual(TEXT("Pointer viewport position follows gesture pointer"), DragView.PointerViewportPosition, FVector2D(1440.0f, 270.0f));
	TestEqual(TEXT("Pointer normalized X uses widget viewport"), static_cast<float>(DragView.PointerNormalizedViewportPosition.X), 0.5f);
	TestEqual(TEXT("Pointer normalized Y uses widget viewport"), static_cast<float>(DragView.PointerNormalizedViewportPosition.Y), -0.5f);

	SlotWidget->RequestGestureReleaseForTest(FVector2D(1440.0f, 270.0f));
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerHoldInspectTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.HoldPastDelayEntersInspectAndShowsDetail",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerHoldInspectTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardInspectHoldDelaySeconds = 0.1f;
	DragConfig.CardInspectScale = 1.25f;
	SlotWidget->SetCardDragConfig(DragConfig);
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid(), true, true));

	WacomFirstPersonCardLayerSpec::FLayerDragReceiver Receiver;
	SlotWidget->OnCardDragStartedNative.AddRaw(&Receiver, &WacomFirstPersonCardLayerSpec::FLayerDragReceiver::HandleStarted);

	TestTrue(TEXT("Gesture press starts"), SlotWidget->RequestGesturePressForTest(FVector2D(500.0f, 600.0f)));
	SlotWidget->RequestGestureMoveForTest(0.12f, FVector2D(500.0f, 600.0f));
	TestEqual(TEXT("Hold enters inspect state"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::Inspecting);
	TestEqual(TEXT("Inspect start broadcasts"), Receiver.StartedCount, 1);
	TestTrue(TEXT("Inspect visual scales up"), SlotWidget->GetVisualSlotView().RenderScale >= 0.55f);

	SlotWidget->RequestGestureReleaseForTest(FVector2D(500.0f, 600.0f));
	TestEqual(TEXT("Release clears gesture"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::Idle);

	SlotWidget->OnCardDragStartedNative.RemoveAll(&Receiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerHoldInspectKeepsPointerStableTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.HoldInspectKeepsPointerStableWhileSlotMoves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerHoldInspectKeepsPointerStableTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardInspectHoldDelaySeconds = 0.1f;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	DragConfig.CardInspectScreenPosition = FVector2D(0.5f, 0.45f);
	DragConfig.CardInspectScale = 1.25f;
	SlotWidget->SetCardDragConfig(DragConfig);
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotMotionConfig(WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig());
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid(), true, true);
	Slot.AnchorWidgetPosition = FVector2D(760.0f, 760.0f);
	SlotWidget->SetSlotViewImmediate(Slot);

	const FVector2D PressPosition(500.0f, 600.0f);
	TestTrue(TEXT("Gesture press starts"), SlotWidget->RequestGesturePressForTest(PressPosition));
	SlotWidget->RequestGestureMoveForTest(0.12f, PressPosition);
	TestEqual(TEXT("Hold enters inspect state"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::Inspecting);
	SlotWidget->TickSlotMotionForTest(0.25f);
	TestNotEqual(TEXT("Inspect motion moves visual slot away from source"),
		SlotWidget->GetVisualSlotView().ScreenPosition,
		SlotWidget->GetSlotView().ScreenPosition);

	SlotWidget->RequestGestureMoveForTest(0.0f, PressPosition);
	const FWacomFirstPersonCardDragView DragView = SlotWidget->BuildDragView();
	TestEqual(TEXT("Press remains in widget-space"), DragView.PressScreenPosition, PressPosition);
	TestEqual(TEXT("Current pointer remains in widget-space"), DragView.CurrentScreenPosition, PressPosition);
	TestEqual(TEXT("Moving inspect slot does not self-trigger drag"),
		SlotWidget->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::Inspecting);

	SlotWidget->RequestGestureReleaseForTest(PressPosition);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerInspectIgnoresLargeLayoutResetTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.HoldInspectIgnoresLargeLayoutReset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerInspectIgnoresLargeLayoutResetTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	FWacomFirstPersonCardSlotMotionConfig MotionConfig =
		WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	MotionConfig.ResetDistancePixels = 120.0f;
	SlotWidget->SetSlotMotionConfig(MotionConfig);

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardInspectHoldDelaySeconds = 0.1f;
	DragConfig.CardInspectScreenPosition = FVector2D(0.5f, 0.46f);
	SlotWidget->SetCardDragConfig(DragConfig);
	SlotWidget->SetCardLayerInteractionEnabled(true);

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView InitialSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true);
	InitialSlot.ScreenPosition = FVector2D(500.0f, 600.0f);
	InitialSlot.WidgetPosition = InitialSlot.ScreenPosition;
	InitialSlot.SnappedWidgetPosition = InitialSlot.ScreenPosition;
	InitialSlot.AnchorWidgetPosition = FVector2D(760.0f, 760.0f);
	SlotWidget->SetSlotViewImmediate(InitialSlot);

	const FVector2D PressPosition(500.0f, 600.0f);
	SlotWidget->RequestGesturePressForTest(PressPosition);
	SlotWidget->RequestGestureMoveForTest(0.12f, PressPosition);
	SlotWidget->TickSlotMotionForTest(1.0f);
	const FVector2D InspectVisualPosition = SlotWidget->GetVisualSlotView().ScreenPosition;
	TestNotEqual(TEXT("Inspect visual leaves original slot"),
		InspectVisualPosition,
		InitialSlot.ScreenPosition);

	FWacomFirstPersonCardLayerSlotView RefreshedSlot = InitialSlot;
	RefreshedSlot.ScreenPosition = FVector2D(510.0f, 600.0f);
	RefreshedSlot.WidgetPosition = RefreshedSlot.ScreenPosition;
	RefreshedSlot.SnappedWidgetPosition = RefreshedSlot.ScreenPosition;
	SlotWidget->SetSlotView(RefreshedSlot);
	TestEqual(TEXT("Layout refresh during inspect does not reset visual to hand slot"),
		SlotWidget->GetVisualSlotView().ScreenPosition,
		InspectVisualPosition);

	SlotWidget->RequestGestureReleaseForTest(PressPosition);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerInspectUsesVisualCanvasZOrderTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.HoldInspectUsesVisualCanvasZOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerInspectUsesVisualCanvasZOrderTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Layer widget"), Layer))
	{
		return false;
	}

	FWacomFirstPersonCardSlotMotionConfig MotionConfig =
		WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	MotionConfig.ResetDistancePixels = 120.0f;
	Layer->SetSlotMotionConfig(MotionConfig);
	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardInspectHoldDelaySeconds = 0.1f;
	DragConfig.CardInspectScreenPosition = FVector2D(0.5f, 0.46f);
	Layer->SetCardDragConfig(DragConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true);
	Slot.ZOrder = 7;
	Slot.ScreenPosition = FVector2D(500.0f, 600.0f);
	Slot.WidgetPosition = Slot.ScreenPosition;
	Slot.SnappedWidgetPosition = Slot.ScreenPosition;
	Slot.AnchorWidgetPosition = FVector2D(760.0f, 760.0f);
	Layer->SetCardSlots({ Slot });

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	const FVector2D PressPosition(500.0f, 600.0f);
	TestTrue(TEXT("Gesture press starts"), SlotWidget->RequestGesturePressForTest(PressPosition));
	SlotWidget->RequestGestureMoveForTest(0.12f, PressPosition);
	SlotWidget->TickSlotMotionForTest(1.0f);
	const int32 InspectVisualZOrder = SlotWidget->GetVisualSlotView().ZOrder;
	TestTrue(TEXT("Inspect visual raises z-order"), InspectVisualZOrder > Slot.ZOrder);

	FWacomFirstPersonCardLayerSlotView RefreshedSlot = Slot;
	RefreshedSlot.ScreenPosition = FVector2D(510.0f, 600.0f);
	RefreshedSlot.WidgetPosition = RefreshedSlot.ScreenPosition;
	RefreshedSlot.SnappedWidgetPosition = RefreshedSlot.ScreenPosition;
	Layer->SetCardSlots({ RefreshedSlot });
	TestEqual(TEXT("Layer refresh keeps inspect visual z-order in canvas"),
		Layer->GetCardZOrderAt(0),
		SlotWidget->GetVisualSlotView().ZOrder);

	SlotWidget->RequestGestureReleaseForTest(PressPosition);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerInspectBroadcastsVisualMotionUpdateTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.HoldInspectBroadcastsVisualMotionUpdateWithoutPointerMove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerInspectBroadcastsVisualMotionUpdateTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Layer widget"), Layer))
	{
		return false;
	}

	FWacomFirstPersonCardSlotMotionConfig MotionConfig =
		WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	MotionConfig.MotionSpeed = 4.0f;
	Layer->SetSlotMotionConfig(MotionConfig);
	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardInspectHoldDelaySeconds = 0.1f;
	DragConfig.CardInspectScreenPosition = FVector2D(0.5f, 0.46f);
	Layer->SetCardDragConfig(DragConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true);
	Slot.ScreenPosition = FVector2D(500.0f, 600.0f);
	Slot.WidgetPosition = Slot.ScreenPosition;
	Slot.SnappedWidgetPosition = Slot.ScreenPosition;
	Slot.AnchorWidgetPosition = FVector2D(500.0f, 500.0f);
	Layer->SetCardSlots({ Slot });

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	WacomFirstPersonCardLayerSpec::FLayerDragReceiver DragReceiver;
	Layer->OnCardDragUpdatedNative.AddRaw(&DragReceiver, &WacomFirstPersonCardLayerSpec::FLayerDragReceiver::HandleUpdated);

	const FVector2D PressPosition(500.0f, 600.0f);
	TestTrue(TEXT("Gesture press starts"), SlotWidget->RequestGesturePressForTest(PressPosition));
	SlotWidget->RequestGestureMoveForTest(0.12f, PressPosition);
	const int32 UpdatesAfterEnteringInspect = DragReceiver.UpdatedCount;
	const FVector2D VisualPositionBeforeMotion = SlotWidget->GetVisualSlotView().ScreenPosition;

	SlotWidget->TickSlotMotionForTest(0.16f);
	TestTrue(TEXT("Inspect slot motion broadcasts visual update without pointer move"),
		DragReceiver.UpdatedCount > UpdatesAfterEnteringInspect);
	TestNotEqual(TEXT("Inspect visual moved while pointer stayed still"),
		SlotWidget->GetVisualSlotView().ScreenPosition,
		VisualPositionBeforeMotion);
	TestEqual(TEXT("Visual update source follows current visual slot"),
		DragReceiver.LastDragView.SourceSlotView.ScreenPosition,
		SlotWidget->GetVisualSlotView().ScreenPosition);
	TestEqual(TEXT("Pointer position remains unchanged"),
		DragReceiver.LastDragView.CurrentScreenPosition,
		PressPosition);

	Layer->OnCardDragUpdatedNative.RemoveAll(&DragReceiver);
	SlotWidget->RequestGestureReleaseForTest(PressPosition);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerInspectReleaseNoSubmitTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.InspectReleaseWithoutDragDoesNotSubmitWhenClickAlreadyExpired",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerInspectReleaseNoSubmitTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardInspectHoldDelaySeconds = 0.1f;
	SlotWidget->SetCardDragConfig(DragConfig);
	SlotWidget->SetSlotFeedbackConfig(WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid(), true, true));

	WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver ClickReceiver;
	SlotWidget->OnCardClickedNative.AddRaw(&ClickReceiver, &WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver::HandleClicked);

	SlotWidget->RequestGesturePressForTest(FVector2D(500.0f, 600.0f));
	SlotWidget->RequestGestureMoveForTest(0.12f, FVector2D(500.0f, 600.0f));
	SlotWidget->RequestGestureReleaseForTest(FVector2D(500.0f, 600.0f));
	TestEqual(TEXT("Inspect release does not click"), ClickReceiver.ClickCount, 0);
	TestFalse(TEXT("Inspect release does not play deny feedback"), SlotWidget->IsDenyFeedbackActiveForTest());

	SlotWidget->OnCardClickedNative.RemoveAll(&ClickReceiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDragSuppressesClickTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.DragPastThresholdSuppressesClick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDragSuppressesClickTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	SlotWidget->SetCardDragConfig(DragConfig);
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid(), true, true));

	WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver ClickReceiver;
	SlotWidget->OnCardClickedNative.AddRaw(&ClickReceiver, &WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver::HandleClicked);

	SlotWidget->RequestGesturePressForTest(FVector2D(500.0f, 600.0f));
	SlotWidget->RequestGestureMoveForTest(0.01f, FVector2D(505.0f, 575.0f));
	TestEqual(TEXT("No-target drag state"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::DraggingNoTargetCard);
	SlotWidget->RequestGestureReleaseForTest(FVector2D(505.0f, 575.0f));
	TestEqual(TEXT("Short drag release does not click"), ClickReceiver.ClickCount, 0);

	SlotWidget->OnCardClickedNative.RemoveAll(&ClickReceiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerNoTargetArmedTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.NoTargetCardArmsOnlyWhenDraggedUpPastThreshold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerNoTargetArmedTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	DragConfig.NoTargetCardDragOutCommitDistancePixels = 100.0f;
	SlotWidget->SetCardDragConfig(DragConfig);
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid(), true, true));

	SlotWidget->RequestGesturePressForTest(FVector2D(500.0f, 600.0f));
	SlotWidget->RequestGestureMoveForTest(0.01f, FVector2D(500.0f, 540.0f));
	TestEqual(TEXT("Below commit distance stays dragging"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::DraggingNoTargetCard);
	SlotWidget->RequestGestureMoveForTest(0.01f, FVector2D(500.0f, 480.0f));
	TestEqual(TEXT("Past upward distance arms commit"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::ArmedForCommit);
	TestTrue(TEXT("Drag view reports armed"), SlotWidget->BuildDragView().bCommitArmed);

	SlotWidget->RequestGestureReleaseForTest(FVector2D(500.0f, 480.0f));
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerNoTargetDragIgnoresLiveAnchorMotionTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.NoTargetDragUsesFrozenVisualStart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerNoTargetDragIgnoresLiveAnchorMotionTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	SlotWidget->SetCardDragConfig(DragConfig);
	SlotWidget->SetCardLayerInteractionEnabled(true);

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView InitialSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true);
	InitialSlot.ScreenPosition = FVector2D(500.0f, 600.0f);
	InitialSlot.WidgetPosition = InitialSlot.ScreenPosition;
	InitialSlot.SnappedWidgetPosition = InitialSlot.ScreenPosition;
	SlotWidget->SetSlotViewImmediate(InitialSlot);

	SlotWidget->RequestGesturePressForTest(FVector2D(500.0f, 600.0f));
	SlotWidget->RequestGestureMoveForTest(0.01f, FVector2D(500.0f, 540.0f));
	SlotWidget->TickSlotMotionForTest(1.0f);
	TestEqual(TEXT("Drag starts"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::DraggingNoTargetCard);
	TestEqual(TEXT("Initial drag follows pointer delta from frozen visual start"),
		SlotWidget->GetVisualSlotView().ScreenPosition,
		FVector2D(500.0f, 540.0f));

	FWacomFirstPersonCardLayerSlotView LiveMovedSlot = InitialSlot;
	LiveMovedSlot.ScreenPosition = FVector2D(660.0f, 720.0f);
	LiveMovedSlot.WidgetPosition = LiveMovedSlot.ScreenPosition;
	LiveMovedSlot.SnappedWidgetPosition = LiveMovedSlot.ScreenPosition;
	SlotWidget->SetSlotView(LiveMovedSlot);
	SlotWidget->RequestGestureMoveForTest(0.0f, FVector2D(500.0f, 520.0f));
	SlotWidget->TickSlotMotionForTest(1.0f);
	TestEqual(TEXT("Live slot refresh does not add anchor drift to drag visual"),
		SlotWidget->GetVisualSlotView().ScreenPosition,
		FVector2D(500.0f, 520.0f));

	SlotWidget->RequestGestureReleaseForTest(FVector2D(500.0f, 520.0f));
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerNoTargetDragIgnoresLargeLayoutResetTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.NoTargetDragIgnoresLargeLayoutReset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerNoTargetDragIgnoresLargeLayoutResetTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	FWacomFirstPersonCardSlotMotionConfig MotionConfig =
		WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	MotionConfig.ResetDistancePixels = 120.0f;
	SlotWidget->SetSlotMotionConfig(MotionConfig);

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	DragConfig.NoTargetCardDragOutCommitDistancePixels = 140.0f;
	SlotWidget->SetCardDragConfig(DragConfig);
	SlotWidget->SetCardLayerInteractionEnabled(true);

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView InitialSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true);
	InitialSlot.ScreenPosition = FVector2D(500.0f, 600.0f);
	InitialSlot.WidgetPosition = InitialSlot.ScreenPosition;
	InitialSlot.SnappedWidgetPosition = InitialSlot.ScreenPosition;
	SlotWidget->SetSlotViewImmediate(InitialSlot);

	SlotWidget->RequestGesturePressForTest(FVector2D(500.0f, 600.0f));
	SlotWidget->RequestGestureMoveForTest(0.01f, FVector2D(500.0f, 300.0f));
	SlotWidget->TickSlotMotionForTest(1.0f);
	TestEqual(TEXT("Drag visual follows pointer delta before refresh"),
		SlotWidget->GetVisualSlotView().ScreenPosition,
		FVector2D(500.0f, 300.0f));

	FWacomFirstPersonCardLayerSlotView RefreshedSlot = InitialSlot;
	RefreshedSlot.ScreenPosition = FVector2D(510.0f, 600.0f);
	RefreshedSlot.WidgetPosition = RefreshedSlot.ScreenPosition;
	RefreshedSlot.SnappedWidgetPosition = RefreshedSlot.ScreenPosition;
	SlotWidget->SetSlotView(RefreshedSlot);
	TestEqual(TEXT("Layout refresh during no-target drag does not reset visual to hand slot"),
		SlotWidget->GetVisualSlotView().ScreenPosition,
		FVector2D(500.0f, 300.0f));

	SlotWidget->RequestGestureReleaseForTest(FVector2D(500.0f, 300.0f));
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerTargetedAimTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.TargetedCardDragShowsAimArrowAndKeepsSourceSelected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerTargetedAimTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot = WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true);
	Slot.Entry.TargetMode = ECardTargetMode::SingleEnemyPart;
	Layer->SetCardLayerInteractionEnabled(true);
	Layer->SetCardSlots({ Slot });
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	WacomFirstPersonCardLayerSpec::FLayerDragReceiver DragReceiver;
	Layer->OnCardDragStartedNative.AddRaw(&DragReceiver, &WacomFirstPersonCardLayerSpec::FLayerDragReceiver::HandleStarted);
	Layer->OnCardDragUpdatedNative.AddRaw(&DragReceiver, &WacomFirstPersonCardLayerSpec::FLayerDragReceiver::HandleUpdated);

	SlotWidget->RequestGesturePressForTest(FVector2D(500.0f, 600.0f));
	SlotWidget->RequestGestureMoveForTest(0.01f, FVector2D(540.0f, 590.0f));
	TestEqual(TEXT("Targeted card enters aim state"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::AimingTargetedCard);
	TestEqual(TEXT("Layer drag started"), DragReceiver.StartedCount, 1);
	TestEqual(TEXT("Layer current drag is aim"), Layer->GetCurrentDragViewForTest().GestureState, EWacomFirstPersonCardGestureState::AimingTargetedCard);

	Layer->OnCardDragStartedNative.RemoveAll(&DragReceiver);
	Layer->OnCardDragUpdatedNative.RemoveAll(&DragReceiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerTargetedAimIgnoresLiveAnchorMotionTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.TargetedAimUsesFrozenVisualStartAndPointer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerTargetedAimIgnoresLiveAnchorMotionTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardSlotMotionConfig MotionConfig =
		WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	MotionConfig.MotionSpeed = 1.0f;
	Layer->SetSlotMotionConfig(MotionConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView InitialSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true);
	InitialSlot.Entry.TargetMode = ECardTargetMode::SingleEnemyPart;
	InitialSlot.ScreenPosition = FVector2D(500.0f, 600.0f);
	InitialSlot.WidgetPosition = InitialSlot.ScreenPosition;
	InitialSlot.SnappedWidgetPosition = InitialSlot.ScreenPosition;
	Layer->SetCardSlots({ InitialSlot });
	Layer->TickSlotMotionForTest(1.0f);

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	SlotWidget->RequestGesturePressForTest(FVector2D(500.0f, 600.0f));
	SlotWidget->RequestGestureMoveForTest(0.01f, FVector2D(560.0f, 590.0f));
	TestEqual(TEXT("Aim starts"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::AimingTargetedCard);
	TestEqual(TEXT("Aim arrow starts at frozen visual source"),
		Layer->GetCurrentDragViewForTest().SourceSlotView.ScreenPosition,
		FVector2D(500.0f, 600.0f));
	TestEqual(TEXT("Aim arrow ends at pointer"),
		Layer->GetCurrentDragViewForTest().CurrentScreenPosition,
		FVector2D(560.0f, 590.0f));

	FWacomFirstPersonCardLayerSlotView LiveMovedSlot = InitialSlot;
	LiveMovedSlot.ScreenPosition = FVector2D(680.0f, 740.0f);
	LiveMovedSlot.WidgetPosition = LiveMovedSlot.ScreenPosition;
	LiveMovedSlot.SnappedWidgetPosition = LiveMovedSlot.ScreenPosition;
	Layer->SetCardSlots({ LiveMovedSlot });
	SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Reused slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}
	SlotWidget->RequestGestureMoveForTest(0.0f, FVector2D(580.0f, 570.0f));

	const FWacomFirstPersonCardDragView DragView = Layer->GetCurrentDragViewForTest();
	TestEqual(TEXT("Live slot refresh does not move aim source"),
		DragView.SourceSlotView.ScreenPosition,
		FVector2D(500.0f, 600.0f));
	TestEqual(TEXT("Aim endpoint remains current pointer"),
		DragView.CurrentScreenPosition,
		FVector2D(580.0f, 570.0f));

	SlotWidget->RequestGestureReleaseForTest(FVector2D(580.0f, 570.0f));
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerHandCardDenyTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.HandCardTargetIsDetectedButDoesNotSubmit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerHandCardDenyTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	FWacomFirstPersonCardLayerSlotView Slot = WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid(), true, true);
	Slot.Entry.TargetMode = ECardTargetMode::HandCard;
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(Slot);

	WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver ClickReceiver;
	SlotWidget->OnCardClickedNative.AddRaw(&ClickReceiver, &WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver::HandleClicked);

	SlotWidget->RequestGesturePressForTest(FVector2D(500.0f, 600.0f));
	SlotWidget->RequestGestureMoveForTest(0.01f, FVector2D(540.0f, 590.0f));
	TestEqual(TEXT("HandCard enters aim/probe state"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::AimingTargetedCard);
	SlotWidget->RequestGestureReleaseForTest(FVector2D(540.0f, 590.0f));
	TestEqual(TEXT("HandCard drag release does not click-submit"), ClickReceiver.ClickCount, 0);

	SlotWidget->OnCardClickedNative.RemoveAll(&ClickReceiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDragClearTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.DragStateClearsOnSlotExitInteractionDisabledAndLayerClear",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDragClearTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot = WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true);
	Slot.Entry.TargetMode = ECardTargetMode::SingleEnemyPart;
	Layer->SetCardLayerInteractionEnabled(true);
	Layer->SetCardSlots({ Slot });
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	SlotWidget->RequestGesturePressForTest(FVector2D(500.0f, 600.0f));
	SlotWidget->RequestGestureMoveForTest(0.01f, FVector2D(540.0f, 590.0f));
	TestEqual(TEXT("Aim state active"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::AimingTargetedCard);
	Layer->SetCardLayerInteractionEnabled(false);
	TestEqual(TEXT("Interaction disabled clears gesture"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::Idle);

	Layer->SetCardLayerInteractionEnabled(true);
	Layer->SetCardSlots({ Slot });
	SlotWidget = Layer->GetSlotWidgetAt(0);
	SlotWidget->RequestGesturePressForTest(FVector2D(500.0f, 600.0f));
	SlotWidget->RequestGestureMoveForTest(0.01f, FVector2D(540.0f, 590.0f));
	Layer->ClearSlotMotionState();
	TestEqual(TEXT("Layer clear resets current drag"), Layer->GetCurrentDragViewForTest().GestureState, EWacomFirstPersonCardGestureState::Idle);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerNoTargetCommitReadyFeedbackTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.NoTargetDragArmedShowsCommitReadyFeedback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerNoTargetCommitReadyFeedbackTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	DragConfig.NoTargetCardDragOutCommitDistancePixels = 140.0f;
	DragConfig.DragTargetFeedbackOpacity = 0.25f;
	DragConfig.DragValidTargetColor = FLinearColor::Green;
	DragConfig.DragCommitReadyScale = 1.08f;
	SlotWidget->SetCardDragConfig(DragConfig);
	SlotWidget->SetSlotFeedbackConfig(WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid(), true, true));

	SlotWidget->RequestGesturePressForTest(FVector2D(500.0f, 600.0f));
	SlotWidget->RequestGestureMoveForTest(0.01f, FVector2D(500.0f, 300.0f));

	TestEqual(TEXT("Armed drag exposes commit ready state"),
		SlotWidget->GetDragTargetFeedbackStateForTest(),
		EWacomFirstPersonCardDragTargetFeedbackState::CommitReady);
	TestEqual(TEXT("Commit ready overlay opacity"), SlotWidget->GetFeedbackOverlayRenderOpacityForTest(), 0.25f);
	TestEqual(TEXT("Commit ready overlay color"), SlotWidget->GetFeedbackOverlayColorForTest(), FLinearColor::Green);
	TestTrue(TEXT("Commit ready scales source card"),
		SlotWidget->GetRenderTransform().Scale.X > SlotWidget->GetVisualSlotView().RenderScale);

	SlotWidget->RequestGestureReleaseForTest(FVector2D(500.0f, 300.0f));
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerValidWorldTargetFeedbackTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.AimingValidWorldTargetShowsValidArrowAndTargetPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerValidWorldTargetFeedbackTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	DragConfig.DragTargetFeedbackOpacity = 0.22f;
	DragConfig.DragValidTargetColor = FLinearColor::Green;
	Layer->SetCardDragConfig(DragConfig);
	Layer->SetSlotFeedbackConfig(WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot = WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true);
	Slot.Entry.TargetMode = ECardTargetMode::SingleEnemyPart;
	Layer->SetCardSlots({ Slot });

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	SlotWidget->RequestGesturePressForTest(FVector2D(500.0f, 600.0f));
	SlotWidget->RequestGestureMoveForTest(0.01f, FVector2D(540.0f, 590.0f));

	const FVector2D TargetScreenPosition(700.0f, 420.0f);
	const FWacomInteractionTargetHandle TargetHandle =
		FWacomInteractionTargetHandle::ForWorldTarget(FGuid::NewGuid(), PC, FVector::ZeroVector, TargetScreenPosition);
	Layer->SetCardDragFeedbackTarget(
		TargetHandle,
		true,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget,
		TargetScreenPosition);

	const FWacomFirstPersonCardDragView DragView = Layer->GetCurrentDragViewForTest();
	TestEqual(TEXT("Drag view records valid world feedback"),
		DragView.TargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget);
	TestTrue(TEXT("Drag view has feedback target position"), DragView.bHasFeedbackTargetScreenPosition);
	TestEqual(TEXT("Source slot gets valid overlay"), SlotWidget->GetFeedbackOverlayColorForTest(), FLinearColor::Green);
	TestEqual(TEXT("Source slot valid opacity"), SlotWidget->GetFeedbackOverlayRenderOpacityForTest(), 0.22f);

	SlotWidget->RequestGestureReleaseForTest(FVector2D(540.0f, 590.0f));
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerAimArrowSnapTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.AimArrowSnapsToValidWorldTargetScreenPosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerAimArrowSnapTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	DragConfig.bSnapAimArrowToValidWorldTarget = true;
	DragConfig.DragAimArrowSnapBlend = 0.5f;
	Layer->SetCardDragConfig(DragConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot = WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true);
	Slot.Entry.TargetMode = ECardTargetMode::SingleEnemyPart;
	Layer->SetCardSlots({ Slot });
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	const FVector2D PointerPosition(540.0f, 590.0f);
	const FVector2D TargetPosition(740.0f, 390.0f);
	SlotWidget->RequestGesturePressForTest(FVector2D(500.0f, 600.0f));
	SlotWidget->RequestGestureMoveForTest(0.01f, PointerPosition);
	Layer->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle::ForWorldTarget(FGuid::NewGuid(), PC, FVector::ZeroVector, TargetPosition),
		true,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget,
		TargetPosition);

	TestEqual(TEXT("Arrow end lerps from pointer toward target"),
		Layer->ResolveAimArrowEndForTest(),
		FMath::Lerp(PointerPosition, TargetPosition, 0.5f));

	SlotWidget->RequestGestureReleaseForTest(PointerPosition);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerAimArrowFallbackTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.AimArrowFallsBackToPointerWithoutTargetScreenPosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerAimArrowFallbackTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	DragConfig.bSnapAimArrowToValidWorldTarget = true;
	Layer->SetCardDragConfig(DragConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot = WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true);
	Slot.Entry.TargetMode = ECardTargetMode::SingleEnemyPart;
	Layer->SetCardSlots({ Slot });
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	const FVector2D PointerPosition(540.0f, 590.0f);
	SlotWidget->RequestGesturePressForTest(FVector2D(500.0f, 600.0f));
	SlotWidget->RequestGestureMoveForTest(0.01f, PointerPosition);
	Layer->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle::ForWorldTarget(FGuid::NewGuid(), PC),
		true,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget);

	TestEqual(TEXT("Missing target position keeps arrow at pointer"),
		Layer->ResolveAimArrowEndForTest(),
		PointerPosition);

	SlotWidget->RequestGestureReleaseForTest(PointerPosition);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerAimArrowFeedbackPersistsAcrossDragUpdateTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.AimArrowFeedbackPersistsAcrossDragUpdate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerAimArrowFeedbackPersistsAcrossDragUpdateTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	DragConfig.bSnapAimArrowToValidWorldTarget = true;
	DragConfig.DragAimArrowSnapBlend = 0.5f;
	DragConfig.DragValidTargetColor = FLinearColor::Green;
	Layer->SetCardDragConfig(DragConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot = WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true);
	Slot.Entry.TargetMode = ECardTargetMode::SingleEnemyPart;
	Layer->SetCardSlots({ Slot });
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	const FVector2D FirstPointerPosition(540.0f, 590.0f);
	const FVector2D SecondPointerPosition(560.0f, 570.0f);
	const FVector2D TargetPosition(740.0f, 390.0f);
	SlotWidget->RequestGesturePressForTest(FVector2D(500.0f, 600.0f));
	SlotWidget->RequestGestureMoveForTest(0.01f, FirstPointerPosition);
	Layer->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle::ForWorldTarget(FGuid::NewGuid(), PC, FVector::ZeroVector, TargetPosition),
		true,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget,
		TargetPosition);

	SlotWidget->RequestGestureMoveForTest(0.01f, SecondPointerPosition);

	TestEqual(TEXT("Valid feedback state survives next drag update"),
		Layer->GetCurrentDragViewForTest().TargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget);
	TestEqual(TEXT("Arrow keeps valid color after drag update"),
		Layer->ResolveAimArrowColorForTest(),
		FLinearColor::Green);
	TestEqual(TEXT("Arrow keeps snapping after drag update"),
		Layer->ResolveAimArrowEndForTest(),
		FMath::Lerp(SecondPointerPosition, TargetPosition, 0.5f));

	SlotWidget->RequestGestureReleaseForTest(SecondPointerPosition);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerCardProbeFeedbackTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.CardTargetShowsProbeFeedbackWithoutSubmit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerCardProbeFeedbackTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	DragConfig.DragTargetFeedbackOpacity = 0.3f;
	DragConfig.DragCardProbeTargetColor = FLinearColor::Blue;
	DragConfig.DragCardTargetProbeScale = 1.07f;
	Layer->SetCardDragConfig(DragConfig);
	Layer->SetSlotFeedbackConfig(WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid TargetCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView SourceSlot = WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(SourceCardId, true, true);
	SourceSlot.Entry.TargetMode = ECardTargetMode::HandCard;
	FWacomFirstPersonCardLayerSlotView TargetSlot = WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(TargetCardId, true, true);
	TargetSlot.Index = 1;
	TargetSlot.ScreenPosition = FVector2D(650.0f, 600.0f);
	Layer->SetCardSlots({ SourceSlot, TargetSlot });

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* TargetWidget = Layer->GetSlotWidgetAt(1);
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Target slot"), TargetWidget))
	{
		PC->Destroy();
		return false;
	}

	WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver ClickReceiver;
	SourceWidget->OnCardClickedNative.AddRaw(&ClickReceiver, &WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver::HandleClicked);

	SourceWidget->RequestGesturePressForTest(FVector2D(500.0f, 600.0f));
	SourceWidget->RequestGestureMoveForTest(0.01f, FVector2D(540.0f, 590.0f));
	Layer->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle::ForCardTarget(TargetCardId, TargetWidget, TargetSlot.ScreenPosition),
		false,
		EWacomFirstPersonCardDragTargetFeedbackState::CardProbe,
		TargetSlot.ScreenPosition);

	TestTrue(TEXT("Target card shows probe feedback"), TargetWidget->HasCardDragProbeFeedbackForTest());
	TestEqual(TEXT("Target card probe overlay color"), TargetWidget->GetFeedbackOverlayColorForTest(), FLinearColor::Blue);
	TestEqual(TEXT("Target card probe overlay opacity"), TargetWidget->GetFeedbackOverlayRenderOpacityForTest(), 0.3f);
	TestEqual(TEXT("Layer records card probe state"),
		Layer->GetCurrentDragViewForTest().TargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::CardProbe);

	SourceWidget->RequestGestureReleaseForTest(FVector2D(540.0f, 590.0f));
	TestEqual(TEXT("Card probe release does not click-submit"), ClickReceiver.ClickCount, 0);

	SourceWidget->OnCardClickedNative.RemoveAll(&ClickReceiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDragPointerCardTargetProbeTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.DragPointerOverCardTargetShowsProbeFeedback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDragPointerCardTargetProbeTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	DragConfig.DragTargetFeedbackOpacity = 0.3f;
	DragConfig.DragCardProbeTargetColor = FLinearColor::Blue;
	Layer->SetCardDragConfig(DragConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid TargetCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView SourceSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(SourceCardId, true, true);
	SourceSlot.Entry.TargetMode = ECardTargetMode::SingleEnemyPart;
	SourceSlot.ScreenPosition = FVector2D(500.0f, 600.0f);
	SourceSlot.WidgetPosition = SourceSlot.ScreenPosition;
	SourceSlot.SnappedWidgetPosition = SourceSlot.ScreenPosition;
	FWacomFirstPersonCardLayerSlotView TargetSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(TargetCardId, true, true);
	TargetSlot.Index = 1;
	TargetSlot.ScreenPosition = FVector2D(650.0f, 600.0f);
	TargetSlot.WidgetPosition = TargetSlot.ScreenPosition;
	TargetSlot.SnappedWidgetPosition = TargetSlot.ScreenPosition;
	TargetSlot.ZOrder = 1;
	Layer->SetCardSlots({ SourceSlot, TargetSlot });

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* TargetWidget = Layer->GetSlotWidgetAt(1);
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Target slot"), TargetWidget))
	{
		PC->Destroy();
		return false;
	}

	WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver ClickReceiver;
	SourceWidget->OnCardClickedNative.AddRaw(&ClickReceiver, &WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver::HandleClicked);

	SourceWidget->RequestGesturePressForTest(FVector2D(500.0f, 600.0f));
	SourceWidget->RequestGestureMoveForTest(0.01f, FVector2D(650.0f, 600.0f));

	const FWacomFirstPersonCardDragView& DragView = Layer->GetCurrentDragViewForTest();
	TestEqual(TEXT("Pointer card target records card kind"),
		DragView.CurrentTarget.TargetKind,
		EWacomInteractionTargetKind::Card);
	TestEqual(TEXT("Pointer card target records target card id"), DragView.CurrentTarget.CardInstanceId, TargetCardId);
	TestEqual(TEXT("Pointer card target state is CardProbe"),
		DragView.TargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::CardProbe);
	TestEqual(TEXT("Pointer card target position uses target visual slot"),
		DragView.FeedbackTargetScreenPosition,
		TargetWidget->GetVisualSlotView().ScreenPosition);
	TestTrue(TEXT("Target card shows probe feedback from drag pointer"), TargetWidget->HasCardDragProbeFeedbackForTest());
	TestEqual(TEXT("Aim arrow uses card probe color"), Layer->ResolveAimArrowColorForTest(), FLinearColor::Blue);

	SourceWidget->RequestGestureMoveForTest(0.01f, FVector2D(320.0f, 600.0f));
	TestFalse(TEXT("Moving away from target card clears probe feedback"), TargetWidget->HasCardDragProbeFeedbackForTest());
	TestNotEqual(TEXT("Moving away from target card clears card probe state"),
		Layer->GetCurrentDragViewForTest().TargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::CardProbe);

	SourceWidget->RequestGestureReleaseForTest(FVector2D(650.0f, 600.0f));
	TestEqual(TEXT("Card probe drag release does not click-submit"), ClickReceiver.ClickCount, 0);

	SourceWidget->OnCardClickedNative.RemoveAll(&ClickReceiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDragTargetFeedbackClearTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.ReleaseCancelLayerClearClearsDragTargetFeedback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDragTargetFeedbackClearTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid TargetCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView SourceSlot = WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(SourceCardId, true, true);
	SourceSlot.Entry.TargetMode = ECardTargetMode::HandCard;
	FWacomFirstPersonCardLayerSlotView TargetSlot = WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(TargetCardId, true, true);
	TargetSlot.Index = 1;
	Layer->SetCardLayerInteractionEnabled(true);
	Layer->SetCardSlots({ SourceSlot, TargetSlot });

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* TargetWidget = Layer->GetSlotWidgetAt(1);
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Target slot"), TargetWidget))
	{
		PC->Destroy();
		return false;
	}

	SourceWidget->RequestGesturePressForTest(FVector2D(500.0f, 600.0f));
	SourceWidget->RequestGestureMoveForTest(0.01f, FVector2D(540.0f, 590.0f));
	Layer->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle::ForCardTarget(TargetCardId, TargetWidget, TargetSlot.ScreenPosition),
		false,
		EWacomFirstPersonCardDragTargetFeedbackState::CardProbe,
		TargetSlot.ScreenPosition);
	TestTrue(TEXT("Probe starts before clear"), TargetWidget->HasCardDragProbeFeedbackForTest());

	Layer->CancelCardDragGesture(true);
	TestFalse(TEXT("Probe clears on cancel"), TargetWidget->HasCardDragProbeFeedbackForTest());
	TestEqual(TEXT("Current drag resets on cancel"),
		Layer->GetCurrentDragViewForTest().GestureState,
		EWacomFirstPersonCardGestureState::Idle);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDragTargetDebugSummaryTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.DebugSummaryReportsDragTargetFeedbackState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDragTargetDebugSummaryTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
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

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot = WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true);
	Slot.Entry.TargetMode = ECardTargetMode::SingleEnemyPart;
	Layer->SetCardLayerInteractionEnabled(true);
	Layer->SetCardSlots({ Slot });
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	SlotWidget->RequestGesturePressForTest(FVector2D(500.0f, 600.0f));
	SlotWidget->RequestGestureMoveForTest(0.01f, FVector2D(540.0f, 590.0f));
	Layer->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle::ForWorldTarget(FGuid::NewGuid(), PC, FVector::ZeroVector, FVector2D(700.0f, 420.0f)),
		true,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget,
		FVector2D(700.0f, 420.0f),
		TEXT("CardDrop{Intent=PlayCardWorldTarget Reject=None}"));

	const FString Summary = Layer->GetDragTargetDebugSummary();
	TestTrue(TEXT("Summary reports drag target section"), Summary.Contains(TEXT("DragTarget")));
	TestTrue(TEXT("Summary reports target position"), Summary.Contains(TEXT("HasTargetPos=true")));
	TestTrue(TEXT("Summary reports valid flag"), Summary.Contains(TEXT("Valid=true")));
	TestTrue(TEXT("Summary reports resolved intent"), Summary.Contains(TEXT("PlayCardWorldTarget")));

	SlotWidget->RequestGestureReleaseForTest(FVector2D(540.0f, 590.0f));
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentNoTargetArmedTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.NoTargetArmedResolvesPlayCardNoTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentNoTargetArmedTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* NoTargetCard = Fx.MakeNoopCard(0);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), { NoTargetCard });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 0, 0), 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomFirstPersonCardLayerSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::None);

	AWacomBattleHUDLocalPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(AWacomBattleHUDLocalPlayerControllerTest::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("No-target card exists"), CardId.IsValid()))
	{
		return false;
	}

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerSpec::SettleBattlePresentationQueue(*HUD);

	const FWacomFirstPersonCardDragView DragView = WacomFirstPersonCardLayerSpec::MakeDropDragView(
		CardId,
		EWacomFirstPersonCardGestureState::ArmedForCommit,
		true);
	const FWacomBattleCardDropResolveResult Result = HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	TestEqual(TEXT("Armed no-target resolves to no-target play"),
		Result.IntentKind,
		EWacomBattleCardDropIntentKind::PlayCardNoTarget);
	TestTrue(TEXT("Armed no-target can submit"), Result.bCanSubmit);
	TestEqual(TEXT("No reject reason"), Result.RejectReason, EWacomBattleCardDropRejectReason::None);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentNoTargetNotArmedTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.NoTargetNotArmedRejectsWithoutCommitReady",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentNoTargetNotArmedTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), { Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 0, 0), 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomFirstPersonCardLayerSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::None);

	AWacomBattleHUDLocalPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(AWacomBattleHUDLocalPlayerControllerTest::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("No-target card exists"), CardId.IsValid()))
	{
		return false;
	}

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerSpec::SettleBattlePresentationQueue(*HUD);

	const FWacomFirstPersonCardDragView DragView = WacomFirstPersonCardLayerSpec::MakeDropDragView(
		CardId,
		EWacomFirstPersonCardGestureState::DraggingNoTargetCard,
		false);
	const FWacomBattleCardDropResolveResult Result = HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	TestEqual(TEXT("Unarmed no-target rejects"), Result.IntentKind, EWacomBattleCardDropIntentKind::Reject);
	TestFalse(TEXT("Unarmed no-target cannot submit"), Result.bCanSubmit);
	TestEqual(TEXT("Unarmed reject reason"), Result.RejectReason, EWacomBattleCardDropRejectReason::NotArmed);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentCardTargetProbeTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.CardTargetUnsupportedSourceRemainsProbeOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentCardTargetProbeTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* SourceCard = Fx.MakeSimpleDamageCard(0, 1);
	UCardDefinition* TargetCard = Fx.MakeNoopCard(0);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), { SourceCard, TargetCard });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 0, 0), 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceCardId = WacomFirstPersonCardLayerSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::SingleEnemyPart);
	const FGuid TargetCardId = WacomFirstPersonCardLayerSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::None);

	AWacomBattleHUDLocalPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(AWacomBattleHUDLocalPlayerControllerTest::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Source card exists"), SourceCardId.IsValid())
		|| !TestTrue(TEXT("Target card exists"), TargetCardId.IsValid()))
	{
		return false;
	}

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerSpec::SettleBattlePresentationQueue(*HUD);

	FWacomFirstPersonCardDragView DragView = WacomFirstPersonCardLayerSpec::MakeDropDragView(
		SourceCardId,
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	DragView.CurrentTarget = FWacomInteractionTargetHandle::ForCardTarget(
		TargetCardId,
		HUD,
		FVector2D(650.0f, 600.0f));
	const FWacomBattleCardDropResolveResult Result = HUD->ResolveFirstPersonCardDropIntentForTest(SourceCardId, DragView);
	TestEqual(TEXT("Card target resolves to probe only"),
		Result.IntentKind,
		EWacomBattleCardDropIntentKind::ProbeCardTarget);
	TestEqual(TEXT("Card target preserves target id"), Result.TargetHandle.CardInstanceId, TargetCardId);
	TestFalse(TEXT("Card target does not submit"), Result.bCanSubmit);
	TestEqual(TEXT("Card target records unsupported reason"),
		Result.RejectReason,
		EWacomBattleCardDropRejectReason::UnsupportedCardTarget);
	TestTrue(TEXT("Card target exposes feedback position"), Result.bHasFeedbackTargetScreenPosition);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentValidHandCardTargetTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.CardTargetValidHandCardResolvesPlayCardCardTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentValidHandCardTargetTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* SourceCard = Fx.MakeHandCardCostModifierCard(/*Cost*/0, /*Magnitude*/2, /*bReduceCost*/false);
	UCardDefinition* TargetCard = Fx.MakeNoopCard(3);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ SourceCard, TargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 50, 0), 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceCardId = WacomFirstPersonCardLayerSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::HandCard);
	const FGuid TargetCardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetCard->CardId);

	AWacomBattleHUDLocalPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(AWacomBattleHUDLocalPlayerControllerTest::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Source card exists"), SourceCardId.IsValid())
		|| !TestTrue(TEXT("Target card exists"), TargetCardId.IsValid()))
	{
		return false;
	}

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerSpec::SettleBattlePresentationQueue(*HUD);

	FWacomFirstPersonCardDragView DragView = WacomFirstPersonCardLayerSpec::MakeDropDragView(
		SourceCardId,
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	DragView.CurrentTarget = FWacomInteractionTargetHandle::ForCardTarget(
		TargetCardId,
		HUD,
		FVector2D(650.0f, 600.0f));
	const FWacomBattleCardDropResolveResult Result =
		HUD->ResolveFirstPersonCardDropIntentForTest(SourceCardId, DragView);
	TestEqual(TEXT("Valid hand-card target resolves to card play"),
		Result.IntentKind,
		EWacomBattleCardDropIntentKind::PlayCardCardTarget);
	TestEqual(TEXT("Card target preserves target id"), Result.TargetHandle.CardInstanceId, TargetCardId);
	TestTrue(TEXT("Card target can submit"), Result.bCanSubmit);
	TestEqual(TEXT("No reject reason"), Result.RejectReason, EWacomBattleCardDropRejectReason::None);
	TestTrue(TEXT("Card target exposes feedback position"), Result.bHasFeedbackTargetScreenPosition);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentSelectedZoneMoveCardTargetTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.SelectedZoneMoveCardTargetResolvesPlayCardCardTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentSelectedZoneMoveCardTargetTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* SourceCard = Fx.MakeSelectedHandCardZoneMoveCard(/*Cost*/0, /*bExhaust*/false);
	UCardDefinition* TargetCard = Fx.MakeNoopCard(3);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ SourceCard, TargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 50, 0), 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceCardId = WacomFirstPersonCardLayerSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::HandCard);
	const FGuid TargetCardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetCard->CardId);

	AWacomBattleHUDLocalPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(AWacomBattleHUDLocalPlayerControllerTest::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Source card exists"), SourceCardId.IsValid())
		|| !TestTrue(TEXT("Target card exists"), TargetCardId.IsValid()))
	{
		return false;
	}

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerSpec::SettleBattlePresentationQueue(*HUD);

	FWacomFirstPersonCardDragView DragView = WacomFirstPersonCardLayerSpec::MakeDropDragView(
		SourceCardId,
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	DragView.CurrentTarget = FWacomInteractionTargetHandle::ForCardTarget(
		TargetCardId,
		HUD,
		FVector2D(650.0f, 600.0f));
	const FWacomBattleCardDropResolveResult Result =
		HUD->ResolveFirstPersonCardDropIntentForTest(SourceCardId, DragView);
	TestEqual(TEXT("Selected zone move normal card target resolves to card play"),
		Result.IntentKind,
		EWacomBattleCardDropIntentKind::PlayCardCardTarget);
	TestTrue(TEXT("Selected zone move normal card target can submit"), Result.bCanSubmit);
	TestEqual(TEXT("No reject reason"), Result.RejectReason, EWacomBattleCardDropRejectReason::None);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentSelectedZoneMoveHandAnchorRejectTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.SelectedZoneMoveHandAnchorRejects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentSelectedZoneMoveHandAnchorRejectTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* SourceCard = Fx.MakeSelectedHandCardZoneMoveCard(/*Cost*/0, /*bExhaust*/true);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ SourceCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 50, 0), 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceCardId = WacomFirstPersonCardLayerSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::HandCard);
	const FGuid AnchorCardId = WacomFirstPersonCardLayerSpec::FindFirstHandAnchor(Snapshot);

	AWacomBattleHUDLocalPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(AWacomBattleHUDLocalPlayerControllerTest::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Source card exists"), SourceCardId.IsValid())
		|| !TestTrue(TEXT("Anchor card exists"), AnchorCardId.IsValid()))
	{
		return false;
	}

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerSpec::SettleBattlePresentationQueue(*HUD);

	FWacomFirstPersonCardDragView DragView = WacomFirstPersonCardLayerSpec::MakeDropDragView(
		SourceCardId,
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	DragView.CurrentTarget = FWacomInteractionTargetHandle::ForCardTarget(
		AnchorCardId,
		HUD,
		FVector2D(650.0f, 600.0f));
	const FWacomBattleCardDropResolveResult Result =
		HUD->ResolveFirstPersonCardDropIntentForTest(SourceCardId, DragView);
	TestEqual(TEXT("Selected zone move anchor target rejects"),
		Result.IntentKind,
		EWacomBattleCardDropIntentKind::Reject);
	TestFalse(TEXT("Selected zone move anchor target cannot submit"), Result.bCanSubmit);
	TestEqual(TEXT("Anchor target records unsupported reason"),
		Result.RejectReason,
		EWacomBattleCardDropRejectReason::UnsupportedCardTarget);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentSelfCardRejectTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.SameSourceCardTargetRejects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentSelfCardRejectTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), { Fx.MakeSimpleDamageCard(0, 1) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 0, 0), 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomFirstPersonCardLayerSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::SingleEnemyPart);

	AWacomBattleHUDLocalPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(AWacomBattleHUDLocalPlayerControllerTest::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Card exists"), CardId.IsValid()))
	{
		return false;
	}

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerSpec::SettleBattlePresentationQueue(*HUD);

	FWacomFirstPersonCardDragView DragView = WacomFirstPersonCardLayerSpec::MakeDropDragView(
		CardId,
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	DragView.CurrentTarget = FWacomInteractionTargetHandle::ForCardTarget(CardId, HUD, FVector2D(500.0f, 600.0f));
	const FWacomBattleCardDropResolveResult Result = HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	TestEqual(TEXT("Self card rejects"), Result.IntentKind, EWacomBattleCardDropIntentKind::Reject);
	TestEqual(TEXT("Self card reject reason"), Result.RejectReason, EWacomBattleCardDropRejectReason::SelfTarget);
	TestFalse(TEXT("Self card cannot submit"), Result.bCanSubmit);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentZoneRejectTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.ZoneTargetRejectsAsUnsupported",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentZoneRejectTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), { Fx.MakeSimpleDamageCard(0, 1) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 0, 0), 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomFirstPersonCardLayerSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::SingleEnemyPart);

	AWacomBattleHUDLocalPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(AWacomBattleHUDLocalPlayerControllerTest::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Card exists"), CardId.IsValid()))
	{
		return false;
	}

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerSpec::SettleBattlePresentationQueue(*HUD);

	FWacomFirstPersonCardDragView DragView = WacomFirstPersonCardLayerSpec::MakeDropDragView(
		CardId,
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	DragView.CurrentTarget = FWacomInteractionTargetHandle::ForZoneTarget(TEXT("TestZone"), HUD, FVector2D(700.0f, 500.0f));
	const FWacomBattleCardDropResolveResult Result = HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	TestEqual(TEXT("Zone target rejects"), Result.IntentKind, EWacomBattleCardDropIntentKind::Reject);
	TestEqual(TEXT("Zone target reject reason"),
		Result.RejectReason,
		EWacomBattleCardDropRejectReason::UnsupportedZoneTarget);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentUIBlockedTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.UIBlockedOrMissingSessionRejects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentUIBlockedTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), { Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 0, 0), 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomFirstPersonCardLayerSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::None);

	AWacomBattleHUDLocalPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(AWacomBattleHUDLocalPlayerControllerTest::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Card exists"), CardId.IsValid()))
	{
		return false;
	}

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	const FWacomFirstPersonCardDragView DragView = WacomFirstPersonCardLayerSpec::MakeDropDragView(
		CardId,
		EWacomFirstPersonCardGestureState::ArmedForCommit,
		true);
	FWacomBattleCardDropResolveResult MissingSession = HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	TestEqual(TEXT("Missing session rejects"), MissingSession.IntentKind, EWacomBattleCardDropIntentKind::Reject);
	TestEqual(TEXT("Missing session reason"), MissingSession.RejectReason, EWacomBattleCardDropRejectReason::MissingSession);

	HUD->SetSession(Session);
	WacomFirstPersonCardLayerSpec::SettleBattlePresentationQueue(*HUD);
	HUD->SetUIStateForTest(EBattleUIState::Resolving);
	FWacomBattleCardDropResolveResult UIBlocked = HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	TestEqual(TEXT("Resolving UI rejects"), UIBlocked.IntentKind, EWacomBattleCardDropIntentKind::Reject);
	TestEqual(TEXT("Resolving reject reason"), UIBlocked.RejectReason, EWacomBattleCardDropRejectReason::UIBlocked);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentWorldTargetTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.TargetedCardValidWorldResolvesPlayCardWorldTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentWorldTargetTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), { Fx.MakeSimpleDamageCard(0, 1) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 0, 0), 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomFirstPersonCardLayerSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::SingleEnemyPart);
	const FGuid PartId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);

	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity);
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Owner"), Owner)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Card exists"), CardId.IsValid())
		|| !TestTrue(TEXT("Part exists"), PartId.IsValid()))
	{
		return false;
	}

	UStaticMeshComponent* Primitive = NewObject<UStaticMeshComponent>(Owner);
	Owner->SetRootComponent(Primitive);
	Primitive->RegisterComponent();
	UWacomInteractionTargetComponent* InteractionTarget = NewObject<UWacomInteractionTargetComponent>(Owner);
	Owner->AddInstanceComponent(InteractionTarget);
	InteractionTarget->RegisterComponent();
	InteractionTarget->SetTargetId(PartId);
	InteractionTarget->SetInteractionTargetTag(WacomTags::Interaction_Target_Battle_EnemyPart);

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerSpec::SettleBattlePresentationQueue(*HUD);
	PC->SetBattleSceneClickHUDForTest(HUD);
	PC->SetBattleSceneClickHitForTest(Owner, Primitive);

	const FWacomFirstPersonCardDragView DragView = WacomFirstPersonCardLayerSpec::MakeDropDragView(
		CardId,
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	const FWacomBattleCardDropResolveResult Result = HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	TestEqual(TEXT("Valid world target resolves to world play"),
		Result.IntentKind,
		EWacomBattleCardDropIntentKind::PlayCardWorldTarget);
	TestTrue(TEXT("Valid world target can submit"), Result.bCanSubmit);
	TestEqual(TEXT("World target id preserved"), Result.TargetHandle.WorldTargetId, PartId);
	TestTrue(TEXT("World target exposes feedback position"), Result.bHasFeedbackTargetScreenPosition);

	Owner->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentInvalidWorldTargetTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.TargetedCardInvalidWorldRejectsWithoutSubmit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentInvalidWorldTargetTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), { Fx.MakeSimpleDamageCard(0, 1) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 0, 0), 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomFirstPersonCardLayerSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::SingleEnemyPart);

	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity);
	AActor* Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Owner"), Owner)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Card exists"), CardId.IsValid()))
	{
		return false;
	}

	UStaticMeshComponent* Primitive = NewObject<UStaticMeshComponent>(Owner);
	Owner->SetRootComponent(Primitive);
	Primitive->RegisterComponent();
	UWacomInteractionTargetComponent* InteractionTarget = NewObject<UWacomInteractionTargetComponent>(Owner);
	Owner->AddInstanceComponent(InteractionTarget);
	InteractionTarget->RegisterComponent();
	InteractionTarget->SetTargetId(FGuid::NewGuid());
	InteractionTarget->SetInteractionTargetTag(WacomTags::Interaction_Target_Battle_EnemyPart);

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerSpec::SettleBattlePresentationQueue(*HUD);
	PC->SetBattleSceneClickHUDForTest(HUD);
	PC->SetBattleSceneClickHitForTest(Owner, Primitive);

	const FWacomFirstPersonCardDragView DragView = WacomFirstPersonCardLayerSpec::MakeDropDragView(
		CardId,
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	const FWacomBattleCardDropResolveResult Result = HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	TestEqual(TEXT("Invalid world target rejects"), Result.IntentKind, EWacomBattleCardDropIntentKind::Reject);
	TestEqual(TEXT("Invalid world reason"), Result.RejectReason, EWacomBattleCardDropRejectReason::InvalidWorldTarget);
	TestFalse(TEXT("Invalid world target cannot submit"), Result.bCanSubmit);

	Owner->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentPreviewReleaseConsistencyTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.PreviewAndReleaseUseSameResolvedIntent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentPreviewReleaseConsistencyTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), { Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 0, 0), 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomFirstPersonCardLayerSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::None);

	AWacomBattleHUDLocalPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(AWacomBattleHUDLocalPlayerControllerTest::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Card exists"), CardId.IsValid()))
	{
		return false;
	}

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerSpec::SettleBattlePresentationQueue(*HUD);
	const int32 VersionBeforeRelease = Session->BuildSnapshot().Version;

	const FWacomFirstPersonCardDragView DragView = WacomFirstPersonCardLayerSpec::MakeDropDragView(
		CardId,
		EWacomFirstPersonCardGestureState::ArmedForCommit,
		true);
	const FWacomBattleCardDropResolveResult PreviewResult = HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	HUD->HandleFirstPersonCardDragUpdatedForTest(CardId, DragView);
	HUD->HandleFirstPersonCardDragReleasedForTest(CardId, DragView);
	WacomFirstPersonCardLayerSpec::SettleBattlePresentationQueue(*HUD);

	TestEqual(TEXT("Preview used submit intent"),
		PreviewResult.IntentKind,
		EWacomBattleCardDropIntentKind::PlayCardNoTarget);
	TestTrue(TEXT("Release submitted same intent path"),
		Session->BuildSnapshot().Version > VersionBeforeRelease);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentReleaseOnCardTargetSubmitTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.ReleaseOnValidCardTargetSubmitsHandCardCommand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentReleaseOnCardTargetSubmitTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* SourceCard = Fx.MakeHandCardCostModifierCard(/*Cost*/0, /*Magnitude*/2, /*bReduceCost*/false);
	UCardDefinition* TargetCard = Fx.MakeNoopCard(3);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ SourceCard, TargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 50, 0), 1);
	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceCardId = WacomFirstPersonCardLayerSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::HandCard);
	const FGuid TargetCardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetCard->CardId);

	AWacomBattleHUDLocalPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(AWacomBattleHUDLocalPlayerControllerTest::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Source card exists"), SourceCardId.IsValid())
		|| !TestTrue(TEXT("Target card exists"), TargetCardId.IsValid()))
	{
		return false;
	}

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerSpec::SettleBattlePresentationQueue(*HUD);
	const int32 VersionBeforeRelease = Session->BuildSnapshot().Version;

	FWacomFirstPersonCardDragView DragView = WacomFirstPersonCardLayerSpec::MakeDropDragView(
		SourceCardId,
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	DragView.CurrentTarget = FWacomInteractionTargetHandle::ForCardTarget(
		TargetCardId,
		HUD,
		FVector2D(650.0f, 600.0f));
	const FWacomBattleCardDropResolveResult PreviewResult =
		HUD->ResolveFirstPersonCardDropIntentForTest(SourceCardId, DragView);
	HUD->HandleFirstPersonCardDragReleasedForTest(SourceCardId, DragView);
	WacomFirstPersonCardLayerSpec::SettleBattlePresentationQueue(*HUD);
	Snapshot = Session->BuildSnapshot();

	TestEqual(TEXT("Preview used card target submit intent"),
		PreviewResult.IntentKind,
		EWacomBattleCardDropIntentKind::PlayCardCardTarget);
	TestTrue(TEXT("Release submitted card target path"), Snapshot.Version > VersionBeforeRelease);
	int32 TargetRuntimeCost = INDEX_NONE;
	for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
	{
		if (Card.InstanceId == TargetCardId)
		{
			TargetRuntimeCost = Card.RuntimeCost;
			break;
		}
	}
	TestEqual(TEXT("Target card cost updated"), TargetRuntimeCost, 5);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentLayerGestureCardTargetSubmitTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.LayerGestureReleaseOnCardTargetSubmitsHandCardCommand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentLayerGestureCardTargetSubmitTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* SourceCard = Fx.MakeHandCardCostModifierCard(/*Cost*/0, /*Magnitude*/2, /*bReduceCost*/false);
	UCardDefinition* TargetCard = Fx.MakeNoopCard(3);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ SourceCard, TargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 50, 0), 1);
	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceCardId = WacomFirstPersonCardLayerSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::HandCard);
	const FGuid TargetCardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetCard->CardId);

	AWacomBattleHUDLocalPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
			AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
			FTransform::Identity);
	AWacomPlayerCharacter* Character =
		World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Source card exists"), SourceCardId.IsValid())
		|| !TestTrue(TEXT("Target card exists"), TargetCardId.IsValid()))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeBattleHUDWithCharacter(HUD, PC, Character, World);
	HUD->TakeWidget();
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerSpec::SettleBattlePresentationQueue(*HUD);
	HUD->SetBattleHandPresentationModeForTest(EWacomBattleHandPresentationMode::FirstPersonHandOnly);
	HUD->SyncFirstPersonBattleHandLayerForTest(Snapshot);

	TArray<FWacomFirstPersonCardLayerEntry> CardEntries;
	CardEntries.Reserve(Snapshot.Hand.Cards.Num());
	for (const FHandCardSnapshot& CardSnapshot : Snapshot.Hand.Cards)
	{
		FWacomFirstPersonCardLayerEntry Entry;
		Entry.CardInstanceId = CardSnapshot.InstanceId;
		Entry.CardViewData = UWacomCardPresentationBuilder::BuildCardViewData(CardSnapshot.Definition);
		Entry.CardViewData.Cost = CardSnapshot.RuntimeCost;
		Entry.CardViewData.bShowCost = CardSnapshot.Definition != nullptr;
		Entry.CardViewData.bDisabled = !CardSnapshot.bIsPlayable;
		Entry.bIsPlayable = CardSnapshot.bIsPlayable;
		Entry.TargetMode = CardSnapshot.Definition
			? CardSnapshot.Definition->TargetMode
			: ECardTargetMode::None;
		Entry.Zone = CardSnapshot.Zone;
		Entry.bIsHandAnchor = CardSnapshot.bIsHandAnchor;
		CardEntries.Add(MoveTemp(Entry));
	}

	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	Anchor->SetBattleHandInteractionPrototypeEnabled(true);
	Anchor->SetRuntimeCardLayerEntries(TEXT("BattleHand"), CardEntries);
	HUD->SetFirstPersonCardAnchorForTest(Anchor);
	Anchor->RefreshAnchor(0.0f);
	Anchor->PrimaryComponentTick.ExecuteTick(
		0.0f,
		LEVELTICK_All,
		ENamedThreads::GameThread,
		FGraphEventRef());
	UWacomFirstPersonCardLayerWidget* Layer = Anchor->GetStaticCardLayerWidgetForTest();
	if (!TestNotNull(TEXT("First-person layer"), Layer))
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = nullptr;
	UWacomFirstPersonCardLayerSlotWidget* TargetWidget = nullptr;
	for (int32 Index = 0; Index < Snapshot.Hand.Cards.Num(); ++Index)
	{
		UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(Index);
		if (!SlotWidget)
		{
			continue;
		}
		if (SlotWidget->GetSlotView().Entry.CardInstanceId == SourceCardId)
		{
			SourceWidget = SlotWidget;
		}
		else if (SlotWidget->GetSlotView().Entry.CardInstanceId == TargetCardId)
		{
			TargetWidget = SlotWidget;
		}
	}
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Target slot"), TargetWidget))
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	const FVector2D SourcePosition = SourceWidget->GetVisualSlotView().ScreenPosition;
	const FVector2D TargetPosition = TargetWidget->GetVisualSlotView().ScreenPosition;
	const int32 VersionBeforeRelease = Session->BuildSnapshot().Version;

	SourceWidget->RequestGesturePressForTest(SourcePosition);
	SourceWidget->RequestGestureMoveForTest(0.01f, TargetPosition);
	TestEqual(TEXT("Layer gesture resolves pointer card target"),
		Layer->GetCurrentDragViewForTest().CurrentTarget.CardInstanceId,
		TargetCardId);
	Anchor->OnFirstPersonCardLayerDragUpdated.Broadcast(SourceCardId, Layer->GetCurrentDragViewForTest());
	TestEqual(TEXT("HUD marks card target as valid before release"),
		Layer->GetCurrentDragViewForTest().TargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget);

	SourceWidget->RequestGestureReleaseForTest(TargetPosition);
	WacomFirstPersonCardLayerSpec::SettleBattlePresentationQueue(*HUD);
	Snapshot = Session->BuildSnapshot();

	TestTrue(TEXT("Layer gesture release submitted card target path"), Snapshot.Version > VersionBeforeRelease);
	int32 TargetRuntimeCost = INDEX_NONE;
	for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
	{
		if (Card.InstanceId == TargetCardId)
		{
			TargetRuntimeCost = Card.RuntimeCost;
			break;
		}
	}
	TestEqual(TEXT("Target card cost updated through layer gesture"), TargetRuntimeCost, 5);
	TestTrue(TEXT("Source release used confirm feedback"), SourceWidget->IsConfirmFeedbackActiveForTest());
	TestFalse(TEXT("Source release did not play deny feedback"), SourceWidget->IsDenyFeedbackActiveForTest());

	Character->Destroy();
	PC->Destroy();
	return true;
}
