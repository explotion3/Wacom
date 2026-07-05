// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomRunTunnelSegmentActor.h"
#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartActor.h"
#include "Components/SplineComponent.h"
#include "Components/WacomBattleCameraLookComponent.h"
#include "Components/WacomCursorLookDriverComponent.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Components/WacomFirstPersonViewStageBlendComponent.h"
#include "Components/WacomRunTunnelMovementComponent.h"
#include "Cards/CardDefinition.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "Engine/Engine.h"
#include "Materials/Material.h"
#include "Engine/World.h"
#include "Fixtures/BattleTestFixtures.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "GameFramework/WacomPlayerController.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/BattleSceneTargetClickTestAccess.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "UI/Card/WacomCardPresentationBuilder.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/Card/WacomFirstPersonCardViewWidget.h"
#include "UI/Card/WacomCardView.h"
#include "UI/CardViewTestAccess.h"
#include "UI/FirstPersonCardLayerTestAccess.h"
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
		Card->Description = FText::FromString(TEXT("Preview layer card"));
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

	FWacomCardViewData BuildBattleCardViewDataForTest(const FHandCardSnapshot& CardSnapshot)
	{
		FWacomCardPresentationRuntimeContext RuntimeContext;
		RuntimeContext.bHasRuntimeCost = true;
		RuntimeContext.RuntimeCost = CardSnapshot.RuntimeCost;
		RuntimeContext.bHasPlayableState = true;
		RuntimeContext.bIsPlayable = CardSnapshot.bIsPlayable;
		return UWacomCardPresentationBuilder::BuildCardViewData(CardSnapshot.Definition, RuntimeContext);
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
		Slot.InputHitCenter = Position;
		Slot.InputHitScale = Scale;
		Slot.InputHitAngleDegrees = Angle;
		Slot.InputHitOrder = Index;
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
		Config.EasePower = 1.0f;
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
		Config.InteractionFeedbackEdgeWidth = 0.048f;
		Config.InteractionFeedbackEdgeSoftness = 0.024f;
		Config.InteractionFeedbackVignetteStrength = 0.22f;
		Config.InteractionFeedbackVignetteRadius = 0.58f;
		Config.InteractionFeedbackVignetteSoftness = 0.28f;
		Config.bEnablePlayCommitFeedback = true;
		Config.PlayCommitDuration = 0.12f;
		Config.PlayCommitOpacity = 0.6f;
		Config.PlayCommitColor = FLinearColor::Green;
		Config.PlayCommitScale = 1.1f;
		return Config;
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
		int32 HoverCount = 0;
		int32 UnhoverCount = 0;
		FGuid LastCardId;

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

	struct FSceneEnemyHostActors
	{
		AWacomBattleEnemyActor* Host = nullptr;
		TArray<AWacomBattleEnemyPartActor*> Parts;
	};

	FSceneEnemyHostActors SpawnSceneEnemyHost(
		UWorld& World,
		UEnemyDefinition* EnemyDefinition,
		const TArray<FName>& PartIds)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags |= RF_Transient;

		FSceneEnemyHostActors Result;
		Result.Host = World.SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
		if (!Result.Host)
		{
			return Result;
		}

		Result.Host->EnemyDefinition = EnemyDefinition;
		for (int32 Index = 0; Index < PartIds.Num(); ++Index)
		{
			AWacomBattleEnemyPartActor* PartActor =
				World.SpawnActor<AWacomBattleEnemyPartActor>(
					AWacomBattleEnemyPartActor::StaticClass(),
					FTransform(FVector(100.f * static_cast<float>(Index + 1), 0.f, 0.f)),
					SpawnParams);
			if (!PartActor)
			{
				continue;
			}

			Result.Parts.Add(PartActor);
			PartActor->PartId = PartIds[Index];
			PartActor->PartSlotId = PartIds[Index];
			PartActor->AttachToActor(Result.Host, FAttachmentTransformRules::KeepWorldTransform);
		}

		Result.Host->RefreshBattleEnemyPartAuthoringState();
		return Result;
	}

	void DestroySceneEnemyHost(FSceneEnemyHostActors& Actors)
	{
		for (AWacomBattleEnemyPartActor* PartActor : Actors.Parts)
		{
			if (IsValid(PartActor))
			{
				PartActor->Destroy();
			}
		}
		Actors.Parts.Reset();

		if (IsValid(Actors.Host))
		{
			Actors.Host->Destroy();
		}
		Actors.Host = nullptr;
	}
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
	FWacomFirstPersonCardLayerSuspendedRunTunnelFallbackAnchorTest,
	"Wacom.UI.FirstPersonCardLayer.Anchor.SuspendedRunTunnelDoesNotOwnAnchor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerSuspendedRunTunnelFallbackAnchorTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(
		APlayerController::StaticClass(),
		FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(
		AWacomPlayerCharacter::StaticClass(),
		FTransform::Identity);
	AWacomRunTunnelSegmentActor* Segment = WacomFirstPersonCardLayerSpec::SpawnTestSegment(
		*World,
		FVector::ZeroVector,
		FVector(1000.0f, 0.0f, 0.0f));
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor =
		WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Segment"), Segment)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		if (Anchor)
		{
			Anchor->DestroyComponent();
		}
		if (Segment)
		{
			Segment->Destroy();
		}
		if (Character)
		{
			Character->Destroy();
		}
		if (PC)
		{
			PC->Destroy();
		}
		return false;
	}

	PC->Possess(Character);
	UWacomRunTunnelMovementComponent* RunTunnel = Character->GetRunTunnelMovementComponent();
	TestTrue(TEXT("Run tunnel activates"), RunTunnel && RunTunnel->ActivateRunTunnel(Segment, 200.0f));
	Character->SetExplorationInputEnabled(false);
	TestTrue(TEXT("Run tunnel is suspended"), RunTunnel && RunTunnel->IsRunTunnelSuspended());

	Anchor->ProbeCameraTransform = FTransform(
		FRotator(4.0f, 33.0f, 0.0f),
		FVector(500.0f, 600.0f, 700.0f),
		FVector::OneVector);
	Anchor->RefreshAnchor(0.0f);
	const FWacomFirstPersonCardAnchorDebugView View =
		Anchor->GetFirstPersonCardAnchorDebugView();
	TestTrue(TEXT("Suspended run tunnel keeps anchor valid through fallback"), View.bHasValidAnchor);
	TestEqual(TEXT("Suspended run tunnel does not own anchor mode"),
		View.Mode,
		EWacomFirstPersonCardAnchorMode::CameraFallback);
	TestEqual(TEXT("Fallback reason is recorded"),
		View.LastFallbackReason,
		FName(TEXT("CameraFallback")));

	Anchor->DestroyComponent();
	Segment->Destroy();
	Character->Destroy();
	PC->Destroy();
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
	FWacomFirstPersonCardLayerBleedHoverHitBoundsTest,
	"Wacom.UI.FirstPersonCardLayer.CardTarget.FirstPersonCardViewBleedDoesNotExpandHoverHitBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerBleedHoverHitBoundsTest::RunTest(const FString& Parameters)
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
	Layer->SetCardViewClass(UWacomFirstPersonCardLayerBleedFirstPersonCardViewProbe::StaticClass());
	Layer->SetCardLayerInteractionEnabled(true);
	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true)
	});
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}
	SlotWidget->SetDesiredSizeInViewport(FVector2D(392.0f, 516.0f));
	SlotWidget->TakeWidget();
	FWacomFirstPersonCardLayerTestAccess::SetLocalHitCanvasSizeOverride(*SlotWidget, FVector2D(392.0f, 516.0f));

	WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver Receiver;
	SlotWidget->OnCardHoveredNative.AddRaw(
		&Receiver,
		&WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver::HandleHovered);
	SlotWidget->OnCardUnhoveredNative.AddRaw(
		&Receiver,
		&WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver::HandleUnhovered);

	TestFalse(TEXT("Bleed-only hover request is rejected"),
		FWacomFirstPersonCardLayerTestAccess::RequestHoverAtLocalPosition(*SlotWidget, FVector2D(20.0f, 20.0f)));
	TestFalse(TEXT("Bleed-only hover does not mark slot hovered"), SlotWidget->IsHoveredForFirstPersonLayer());
	TestEqual(TEXT("Bleed-only hover does not broadcast"), Receiver.HoverCount, 0);

	TestTrue(TEXT("Card body hover request succeeds"),
		FWacomFirstPersonCardLayerTestAccess::RequestHoverAtLocalPosition(*SlotWidget, FVector2D(196.0f, 258.0f)));
	TestTrue(TEXT("Card body hover marks slot hovered"), SlotWidget->IsHoveredForFirstPersonLayer());
	TestEqual(TEXT("Card body hover broadcasts once"), Receiver.HoverCount, 1);

	FWacomFirstPersonCardLayerTestAccess::RequestMoveAtLocalPosition(*SlotWidget, FVector2D(385.0f, 500.0f));
	TestFalse(TEXT("Moving from body to bleed clears hover"), SlotWidget->IsHoveredForFirstPersonLayer());
	TestEqual(TEXT("Moving from body to bleed broadcasts unhover"), Receiver.UnhoverCount, 1);

	SlotWidget->OnCardHoveredNative.RemoveAll(&Receiver);
	SlotWidget->OnCardUnhoveredNative.RemoveAll(&Receiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerBleedPressHitBoundsTest,
	"Wacom.UI.FirstPersonCardLayer.CardTarget.FirstPersonCardViewBleedDoesNotStartClickOrDragOutsideCardSizeBox",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerBleedPressHitBoundsTest::RunTest(const FString& Parameters)
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
	Layer->SetCardDragConfig(DragConfig);
	Layer->SetCardViewClass(UWacomFirstPersonCardLayerBleedFirstPersonCardViewProbe::StaticClass());
	Layer->SetCardLayerInteractionEnabled(true);

	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid(), true, true);
	Slot.Entry.TargetMode = ECardTargetMode::None;
	Layer->SetCardSlots({ Slot });
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}
	SlotWidget->SetDesiredSizeInViewport(FVector2D(392.0f, 516.0f));
	SlotWidget->TakeWidget();
	FWacomFirstPersonCardLayerTestAccess::SetLocalHitCanvasSizeOverride(*SlotWidget, FVector2D(392.0f, 516.0f));

	TestFalse(TEXT("Bleed-only press does not start gesture"),
		FWacomFirstPersonCardLayerTestAccess::RequestPressAtLocalPosition(*SlotWidget, FVector2D(20.0f, 20.0f)));
	TestEqual(TEXT("Bleed-only press leaves gesture idle"),
		SlotWidget->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::Idle);

	TestTrue(TEXT("Card body press starts gesture"),
		FWacomFirstPersonCardLayerTestAccess::RequestPressAtLocalPosition(*SlotWidget, FVector2D(196.0f, 258.0f)));
	TestEqual(TEXT("Card body press enters pressed gesture"),
		SlotWidget->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::Pressed);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(196.0f, 120.0f));
	TestTrue(TEXT("Started drag can move outside body"),
		SlotWidget->GetGestureStateForFirstPersonLayer() != EWacomFirstPersonCardGestureState::Idle);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerCompressedBleedHitBoundsTest,
	"Wacom.UI.FirstPersonCardLayer.CardTarget.FirstPersonCardViewCompressedBleedKeepsStableBodyHitBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerCompressedBleedHitBoundsTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UWacomFirstPersonCardLayerBleedCardViewProbe> CardView(
		NewObject<UWacomFirstPersonCardLayerBleedCardViewProbe>());
	if (!TestNotNull(TEXT("CardView"), CardView.Get()))
	{
		return false;
	}

	CardView->TakeWidget();

	TestEqual(TEXT("Stable body hit size stays at design size"),
		CardView->GetCardBodyHitSize(),
		UWacomCardView::GetDefaultCardBodyHitSize());
	TestTrue(TEXT("Compressed cached body still accepts stable right edge"),
		FWacomCardViewTestAccess::IsLocalPositionInsideCardBodyWithBounds(
			*CardView,
			FVector2D(278.0f, 190.0f),
			FVector2D(260.0f, 380.0f)));
	TestFalse(TEXT("Compressed cached body still rejects outside stable right edge"),
		FWacomCardViewTestAccess::IsLocalPositionInsideCardBodyWithBounds(
			*CardView,
			FVector2D(282.0f, 190.0f),
			FVector2D(260.0f, 380.0f)));
	TestTrue(TEXT("Compressed cached body still accepts stable top edge"),
		FWacomCardViewTestAccess::IsLocalPositionInsideCardBodyWithBounds(
			*CardView,
			FVector2D(130.0f, -18.0f),
			FVector2D(260.0f, 380.0f)));
	TestFalse(TEXT("Compressed cached body still rejects above stable top edge"),
		FWacomCardViewTestAccess::IsLocalPositionInsideCardBodyWithBounds(
			*CardView,
			FVector2D(130.0f, -22.0f),
			FVector2D(260.0f, 380.0f)));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerFallbackHitBoundsTest,
	"Wacom.UI.FirstPersonCardLayer.CardTarget.FirstPersonCardLayerFallsBackToLegacyHitSizeWhenCardSizeBoxMissing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerFallbackHitBoundsTest::RunTest(const FString& Parameters)
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

	Layer->SetCardViewClass(UWacomFirstPersonCardLayerLegacyBleedFirstPersonCardViewProbe::StaticClass());
	Layer->SetCardLayerInteractionEnabled(true);
	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid(), true, true)
	});

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}
	SlotWidget->SetDesiredSizeInViewport(FVector2D(392.0f, 516.0f));
	SlotWidget->TakeWidget();
	FWacomFirstPersonCardLayerTestAccess::SetLocalHitCanvasSizeOverride(*SlotWidget, FVector2D(392.0f, 516.0f));

	TestEqual(TEXT("Missing CardSizeBox falls back to legacy body size"),
		SlotWidget->GetCardBodyHitSizeForFirstPersonLayer(),
		UWacomCardView::GetDefaultCardBodyHitSize());
	TestFalse(TEXT("Legacy fallback still rejects bleed-only local position"),
		FWacomFirstPersonCardLayerTestAccess::RequestHoverAtLocalPosition(*SlotWidget, FVector2D(20.0f, 20.0f)));
	TestTrue(TEXT("Legacy fallback accepts centered card body local position"),
		FWacomFirstPersonCardLayerTestAccess::RequestHoverAtLocalPosition(*SlotWidget, FVector2D(196.0f, 258.0f)));

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerRotatedBodyHitBoundsTest,
	"Wacom.UI.FirstPersonCardLayer.CardTarget.FirstPersonCardViewBodyHitBoundsFollowFanRotation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerRotatedBodyHitBoundsTest::RunTest(const FString& Parameters)
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
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(500.0f, 600.0f), 45.0f, 1.0f)
	});

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	TestFalse(TEXT("Old axis-aligned corner no longer hits rotated body"),
		SlotWidget->IsWidgetPositionInsideCardBodyForFirstPersonLayer(FVector2D(610.0f, 780.0f)));
	TestTrue(TEXT("Point inside rotated visual body hits"),
		SlotWidget->IsWidgetPositionInsideCardBodyForFirstPersonLayer(FVector2D(592.0f, 692.0f)));

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

	TestTrue(TEXT("Slot hover succeeds"), FWacomFirstPersonCardLayerTestAccess::RequestHover(*SlotWidget));
	TestEqual(TEXT("Card target hover broadcasts once"), Receiver.HoverCount, 1);
	TestEqual(TEXT("Hovered target id"), Receiver.LastHandle.CardInstanceId, CardId);
	TestEqual(TEXT("Layer stores hovered target"), Layer->BuildHoveredCardTargetHandle().CardInstanceId, CardId);
	TestEqual(TEXT("Hovered target uses visual position"), Receiver.LastHandle.ScreenPosition, SlotWidget->GetVisualSlotView().ScreenPosition);

	FWacomFirstPersonCardLayerTestAccess::RequestUnhover(*SlotWidget);
	TestEqual(TEXT("Card target unhover broadcasts once"), Receiver.UnhoverCount, 1);
	TestFalse(TEXT("Layer clears hovered target"), Layer->BuildHoveredCardTargetHandle().IsValid());

	Layer->OnCardTargetHoveredNative.RemoveAll(&Receiver);
	Layer->OnCardTargetUnhoveredNative.RemoveAll(&Receiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerStableHoverHitTargetTest,
	"Wacom.UI.FirstPersonCardLayer.CardTarget.StableHoverHitUsesBaseGeometryWhenHoveredCardOverlapsNeighbor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerStableHoverHitTargetTest::RunTest(const FString& Parameters)
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

	const FGuid LeftId = FGuid::NewGuid();
	const FGuid RightId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Left =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(LeftId, 0, FVector2D(500.0f, 600.0f), 0.0f, 1.6f);
	FWacomFirstPersonCardLayerSlotView Right =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(RightId, 1, FVector2D(620.0f, 600.0f), 0.0f, 1.0f);
	Left.InputHitScale = 1.0f;
	Left.ZOrder = 500;
	Left.bIsHovered = true;
	Right.ZOrder = 1;
	Layer->SetCardLayerInteractionEnabled(true);
	Layer->SetCardSlots({ Left, Right });

	TestEqual(
		TEXT("Pointer right of base midpoint resolves right card even when left visual overlaps"),
		FWacomFirstPersonCardLayerTestAccess::ResolveHoveredCardAtWidgetPosition(*Layer, FVector2D(622.0f, 600.0f)),
		RightId);
	TestFalse(TEXT("Left slot is no longer hovered"), Layer->GetSlotWidgetAt(0)->IsHoveredForFirstPersonLayer());
	TestTrue(TEXT("Right slot becomes hovered"), Layer->GetSlotWidgetAt(1)->IsHoveredForFirstPersonLayer());

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerStableHoverBoundaryTest,
	"Wacom.UI.FirstPersonCardLayer.CardTarget.StableHoverHitUsesNeighborBoundaryAndHysteresis",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerStableHoverBoundaryTest::RunTest(const FString& Parameters)
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
	DragConfig.HoverHitHysteresisPixels = 16.0f;
	Layer->SetCardDragConfig(DragConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid LeftId = FGuid::NewGuid();
	const FGuid RightId = FGuid::NewGuid();
	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(LeftId, 0, FVector2D(500.0f, 600.0f), 0.0f, 1.0f),
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(RightId, 1, FVector2D(620.0f, 600.0f), 0.0f, 1.0f)
	});

	TestEqual(
		TEXT("Pointer left of midpoint resolves left card"),
		FWacomFirstPersonCardLayerTestAccess::ResolveHoveredCardAtWidgetPosition(*Layer, FVector2D(552.0f, 600.0f)),
		LeftId);
	TestEqual(
		TEXT("Pointer inside hysteresis keeps current left hover"),
		FWacomFirstPersonCardLayerTestAccess::ResolveHoveredCardAtWidgetPosition(*Layer, FVector2D(570.0f, 600.0f)),
		LeftId);
	TestEqual(
		TEXT("Pointer beyond hysteresis switches to right card"),
		FWacomFirstPersonCardLayerTestAccess::ResolveHoveredCardAtWidgetPosition(*Layer, FVector2D(578.0f, 600.0f)),
		RightId);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerStablePressTargetTest,
	"Wacom.UI.FirstPersonCardLayer.CardTarget.StablePressUsesResolvedBaseGeometryTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerStablePressTargetTest::RunTest(const FString& Parameters)
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

	const FGuid LeftId = FGuid::NewGuid();
	const FGuid RightId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Left =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(LeftId, 0, FVector2D(500.0f, 600.0f), 0.0f, 1.6f);
	FWacomFirstPersonCardLayerSlotView Right =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(RightId, 1, FVector2D(620.0f, 600.0f), 0.0f, 1.0f);
	Left.InputHitScale = 1.0f;
	Left.ZOrder = 500;
	Right.ZOrder = 1;
	Layer->SetCardLayerInteractionEnabled(true);
	Layer->SetCardSlots({ Left, Right });

	TestTrue(
		TEXT("Press over right base geometry succeeds"),
		FWacomFirstPersonCardLayerTestAccess::RequestPressAtWidgetPosition(*Layer, FVector2D(622.0f, 600.0f)));
	TestEqual(
		TEXT("Left slot remains idle"),
		Layer->GetSlotWidgetAt(0)->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::Idle);
	TestEqual(
		TEXT("Right slot receives pressed gesture"),
		Layer->GetSlotWidgetAt(1)->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::Pressed);

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
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);

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
	TestTrue(TEXT("Slot hover succeeds"), FWacomFirstPersonCardLayerTestAccess::RequestHover(*SlotWidget));
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
	"Wacom.UI.FirstPersonCardLayer.Anchor.LookResponsive.PartialLookInfluence",
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
	TestEqual(TEXT("Debug raw cursor yaw is preserved"), View.RawCursorLookOffset.Yaw, 20.0);
	TestEqual(TEXT("Debug raw cursor pitch is preserved"), View.RawCursorLookOffset.Pitch, 10.0);
	TestEqual(TEXT("Debug applied anchor yaw matches used offset"), View.AppliedAnchorLookOffset.Yaw, 5.0);
	TestEqual(TEXT("Debug applied anchor pitch matches used offset"), View.AppliedAnchorLookOffset.Pitch, 5.0);
	TestEqual(TEXT("Debug yaw influence is resolved"), View.LookInfluenceYaw, 0.25f);
	TestEqual(TEXT("Debug pitch influence is resolved"), View.LookInfluencePitch, 0.5f);
	TestTrue(TEXT("Look responsive projection reports look used for layout"), View.bLookOffsetAppliedToLayout);
	TestTrue(TEXT("Debug reports look responsive projection"), View.bLookResponsiveProjection);

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
	Anchor->PreviewCardCountFallback = 5;
	Anchor->HandMaxEdgeDropPixels = 0.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
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
	const TArray<FWacomFirstPersonCardLayerSlotView> LookSlots = Anchor->BuildPreviewCardSlotViews();

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
	TestFalse(TEXT("Debug reports body locked is not look responsive"), View.bLookResponsiveProjection);

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
	Anchor->PreviewCardCountFallback = 5;
	Anchor->HandMaxEdgeDropPixels = 0.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	Anchor->bUseCameraTransformProjection = true;
	Anchor->ProjectionMode = EWacomFirstPersonCardProjectionMode::BodyLocked;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	Anchor->ProbeCameraTransform = FTransform(
		FRotator::ZeroRotator,
		Anchor->ProbeCameraTransform.GetLocation(),
		FVector::OneVector);
	const TArray<FWacomFirstPersonCardLayerSlotView> InitialSlots = Anchor->BuildPreviewCardSlotViews();

	Anchor->ProbeCameraTransform = FTransform(
		FRotator(0.0f, 8.0f, 0.0f),
		Anchor->ProbeCameraTransform.GetLocation(),
		FVector::OneVector);
	const TArray<FWacomFirstPersonCardLayerSlotView> RotatedCameraSlots = Anchor->BuildPreviewCardSlotViews();

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

	Anchor->PreviewCardCountFallback = 5;
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
	FWacomFirstPersonCardLayerLookResponsiveProjectionTest,
	"Wacom.UI.FirstPersonCardLayer.Projection.LookResponsive.WorldProjectedAppliesLookInfluence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerLookResponsiveProjectionTest::RunTest(const FString& Parameters)
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
	Anchor->PreviewCardCountFallback = 5;
	Anchor->HandMaxEdgeDropPixels = 0.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	Anchor->ProjectionMode = EWacomFirstPersonCardProjectionMode::LegacyWorldProjected;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	const FTransform InitialCenterTransform = Anchor->ComputeCardTransform(5, 2);
	const TArray<FWacomFirstPersonCardLayerSlotView> InitialSlots = Anchor->BuildPreviewCardSlotViews();

	Character->GetCursorLookDriverComponent()->UpdateFromNormalizedCursor(
		FVector2D(1.0f, -1.0f),
		0.0f,
		20.0f,
		10.0f);
	Anchor->RefreshAnchor(0.0f);
	const FTransform LookCenterTransform = Anchor->ComputeCardTransform(5, 2);
	const TArray<FWacomFirstPersonCardLayerSlotView> LookSlots = Anchor->BuildPreviewCardSlotViews();

	TestEqual(TEXT("Initial slot count"), InitialSlots.Num(), 5);
	TestEqual(TEXT("Look slot count"), LookSlots.Num(), 5);
	TestFalse(TEXT("Look responsive influence changes center world location"), LookCenterTransform.GetLocation().Equals(InitialCenterTransform.GetLocation(), KINDA_SMALL_NUMBER));
	if (InitialSlots.Num() == 5 && LookSlots.Num() == 5)
	{
		TestNotEqual(TEXT("Look responsive projected center card can move with look"), LookSlots[2].ScreenPosition, InitialSlots[2].ScreenPosition);
		TestTrue(TEXT("Look responsive projection reports look used for layout"), LookSlots[2].bLookOffsetAppliedToLayout);
		TestFalse(TEXT("Look responsive body locked layout flag is false"), LookSlots[2].bBodyLockedLayout);
		TestTrue(TEXT("Look responsive still uses current camera projection"), LookSlots[2].bCurrentCameraProjection);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerPreviewPlaceholderCardsTest,
	"Wacom.UI.FirstPersonCardLayer.PreviewLayer.CreatesCardViewsFromPlaceholderData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerPreviewPlaceholderCardsTest::RunTest(const FString& Parameters)
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

	Anchor->PreviewCardCountFallback = 5;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildPreviewCardSlotViews();
	TestEqual(TEXT("Preview creates five placeholder slots"), Slots.Num(), 5);
	if (Slots.Num() > 0)
	{
		TestEqual(TEXT("Preview card has placeholder name"), Slots[0].Entry.CardViewData.Name.ToString(), FString(TEXT("Anchor Card 1")));
		TestTrue(TEXT("Preview slot is projected"), Slots[0].bProjected);
	}

	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (TestNotNull(TEXT("Layer widget"), Layer))
	{
		Layer->SetCardSlots(Slots);
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
	"Wacom.UI.FirstPersonCardLayer.PreviewLayer.BuildsCardViewsFromDefinitions",
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

	Anchor->PreviewCardDefinitions = {
		TSoftObjectPtr<UCardDefinition>(FirstCard),
		TSoftObjectPtr<UCardDefinition>(SecondCard)
	};
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildPreviewCardSlotViews();
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
	Anchor->ProjectionMode = EWacomFirstPersonCardProjectionMode::LegacyWorldProjected;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildPreviewCardSlotViews();
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
	Anchor->PreviewCardCountFallback = 1;
	Anchor->HandMaxEdgeDropPixels = 0.0f;
	Anchor->ProjectionMode = EWacomFirstPersonCardProjectionMode::LegacyWorldProjected;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildPreviewCardSlotViews();
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
	Anchor->PreviewCardCountFallback = 1;
	Anchor->HandMaxEdgeDropPixels = 0.0f;
	Anchor->ProjectionMode = EWacomFirstPersonCardProjectionMode::LegacyWorldProjected;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildPreviewCardSlotViews();
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

	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildPreviewCardSlotViews();
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

	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildPreviewCardSlotViews();
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
	Anchor->HandMaxEdgeDropPixels = 0.0f;
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
	}

	const FString Summary = Anchor->GetDebugSummary();
	TestTrue(TEXT("Summary reports pixel snap"), Summary.Contains(TEXT("PixelSnap=true")));
	TestTrue(TEXT("Summary reports angle clamp"), Summary.Contains(TEXT("AngleClamp=true")));
	TestTrue(TEXT("Summary reports viewport scale"), Summary.Contains(TEXT("ViewportScale=2.00")));
	TestTrue(TEXT("Summary reports projection mode"), Summary.Contains(TEXT("ProjectionMode=LookResponsiveProjected")));

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerStaticSlotOrderTest,
	"Wacom.UI.FirstPersonCardLayer.PreviewLayer.ProjectedSlotsStayOrdered",
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

	Anchor->PreviewCardCountFallback = 5;
	Anchor->HandMaxEdgeDropPixels = 80.0f;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildPreviewCardSlotViews();
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
	FWacomFirstPersonCardLayerAuthoredReadableBottomClampTest,
	"Wacom.UI.FirstPersonCardLayer.AuthoredLayout.KeepsCardBodyBottomReadableNearViewportEdge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerAuthoredReadableBottomClampTest::RunTest(const FString& Parameters)
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
	Anchor->ProbeViewportSize = FVector2D(1920.0f, 1080.0f);
	Anchor->ProbeViewportScale = 1.0f;
	Anchor->ProbeCameraTransform = FTransform(
		FRotator::ZeroRotator,
		FVector(100.0f, 200.0f, -470.0f),
		FVector::OneVector);
	Anchor->RefreshAnchor(0.0f);
	Anchor->ViewportClampMode = EWacomFirstPersonCardViewportClampMode::AllowOffscreen;
	Anchor->HandCardRenderScale = 1.0f;
	Anchor->PreviewCardCountFallback = 5;
	Anchor->HandMaxEdgeDropPixels = 72.0f;
	Anchor->ShortHandEdgeDropPixels = 72.0f;
	Anchor->AuthoredHandScreenOffset = FVector2D::ZeroVector;
	Anchor->bEnableAnchorScreenSmoothing = false;
	Anchor->bEnableCardLayerPixelSnapping = false;
	Anchor->bKeepAuthoredCardBodyBottomInViewport = true;
	Anchor->AuthoredCardBodyBottomViewportPaddingPixels = 8.0f;

	const TArray<FWacomFirstPersonCardLayerSlotView> ClampedSlots = Anchor->BuildPreviewCardSlotViews();
	Anchor->bKeepAuthoredCardBodyBottomInViewport = false;
	const TArray<FWacomFirstPersonCardLayerSlotView> UnclampedSlots = Anchor->BuildPreviewCardSlotViews();

	TestEqual(TEXT("Clamped slot count"), ClampedSlots.Num(), 5);
	TestEqual(TEXT("Unclamped slot count"), UnclampedSlots.Num(), 5);
	if (ClampedSlots.Num() == 5 && UnclampedSlots.Num() == 5)
	{
		constexpr float CardBodyHalfHeight = 210.0f;
		constexpr float ViewportBottom = 1080.0f;
		constexpr float Padding = 8.0f;
		TestTrue(TEXT("Unclamped edge card body can extend beyond viewport bottom"),
			UnclampedSlots[0].ScreenPosition.Y + CardBodyHalfHeight > ViewportBottom);
		for (const FWacomFirstPersonCardLayerSlotView& Slot : ClampedSlots)
		{
			TestTrue(TEXT("Readable clamp keeps card body bottom inside viewport"),
				Slot.ScreenPosition.Y + CardBodyHalfHeight <= ViewportBottom - Padding + KINDA_SMALL_NUMBER);
		}
		TestTrue(TEXT("Readable clamp records adjusted edge slot"), ClampedSlots[0].bBodyBottomViewportAdjusted);
		TestTrue(TEXT("Readable clamp records adjusted center slot"), ClampedSlots[2].bBodyBottomViewportAdjusted);
	}

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
	Anchor->PreviewCardCountFallback = 1;
	Anchor->HandMaxEdgeDropPixels = 0.0f;
	Anchor->AuthoredHandScreenOffset = FVector2D::ZeroVector;
	Anchor->HorizontalOffset = 1236.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildPreviewCardSlotViews();
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

	Anchor->ProjectionPadding = 0.0f;
	Anchor->PreviewCardCountFallback = 5;
	Anchor->HandMaxEdgeDropPixels = 0.0f;
	Anchor->AuthoredCardSpacingPixels = 100.0f;
	Anchor->AuthoredMaxHandWidthPixels = 0.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const TArray<FWacomFirstPersonCardLayerSlotView> InitialSlots = Anchor->BuildPreviewCardSlotViews();
	Anchor->CardSpacing = 360.0f;
	const TArray<FWacomFirstPersonCardLayerSlotView> ChangedLegacySpacingSlots = Anchor->BuildPreviewCardSlotViews();

	TestEqual(TEXT("Initial slot count"), InitialSlots.Num(), 5);
	TestEqual(TEXT("Changed spacing slot count"), ChangedLegacySpacingSlots.Num(), 5);
	if (InitialSlots.Num() == 5 && ChangedLegacySpacingSlots.Num() == 5)
	{
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

	Anchor->ProjectionPadding = 0.0f;
	Anchor->PreviewCardCountFallback = 1;
	Anchor->HandMaxEdgeDropPixels = 0.0f;
	Anchor->AuthoredHandScreenOffset = FVector2D::ZeroVector;
	Anchor->bEnableCardLayerPixelSnapping = false;
	Anchor->bEnableAnchorScreenSmoothing = true;
	Anchor->AnchorScreenSmoothingSpeed = 1.0f;
	Anchor->AnchorScreenSmoothingResetDistancePixels = 320.0f;
	Anchor->ProbeAnchorSmoothingDeltaTime = 1.0f / 60.0f;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const TArray<FWacomFirstPersonCardLayerSlotView> InitialSlots = Anchor->BuildPreviewCardSlotViews();
	Anchor->HorizontalOffset = 60.0f;
	Anchor->RefreshAnchor(0.0f);
	const TArray<FWacomFirstPersonCardLayerSlotView> SmoothedSlots = Anchor->BuildPreviewCardSlotViews();

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

	Anchor->ProjectionPadding = 0.0f;
	Anchor->PreviewCardCountFallback = 5;
	Anchor->HandMaxEdgeDropPixels = 80.0f;
	Anchor->AuthoredCardSpacingPixels = 100.0f;
	Anchor->AuthoredMaxHandWidthPixels = 0.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	Anchor->bEnableAnchorScreenSmoothing = true;
	Anchor->AnchorScreenSmoothingSpeed = 1.0f;
	Anchor->ProbeAnchorSmoothingDeltaTime = 1.0f / 60.0f;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const TArray<FWacomFirstPersonCardLayerSlotView> BaseSlots = Anchor->BuildPreviewCardSlotViews();
	Anchor->HorizontalOffset = 40.0f;
	Anchor->RefreshAnchor(0.0f);
	const TArray<FWacomFirstPersonCardLayerSlotView> MovedSlots = Anchor->BuildPreviewCardSlotViews();

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

	Anchor->ProjectionPadding = 0.0f;
	Anchor->PreviewCardCountFallback = 1;
	Anchor->HandMaxEdgeDropPixels = 0.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	Anchor->bEnableAnchorScreenSmoothing = true;
	Anchor->AnchorScreenSmoothingSpeed = 1.0f;
	Anchor->AnchorScreenSmoothingResetDistancePixels = 80.0f;
	Anchor->ProbeAnchorSmoothingDeltaTime = 1.0f / 60.0f;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	Anchor->BuildPreviewCardSlotViews();
	Anchor->HorizontalOffset = 200.0f;
	Anchor->RefreshAnchor(0.0f);
	const TArray<FWacomFirstPersonCardLayerSlotView> JumpSlots = Anchor->BuildPreviewCardSlotViews();

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

	Anchor->PreviewCardCountFallback = 1;
	Anchor->bEnableAnchorScreenSmoothing = true;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	Anchor->BuildPreviewCardSlotViews();
	Anchor->bProjectionSucceeds = false;
	const TArray<FWacomFirstPersonCardLayerSlotView> FailedSlots = Anchor->BuildPreviewCardSlotViews();

	TestEqual(TEXT("Projection failure still builds slot"), FailedSlots.Num(), 1);
	if (FailedSlots.Num() == 1)
	{
		TestFalse(TEXT("Projection failure hides slot"), FailedSlots[0].bProjected);
		TestFalse(TEXT("Projection failure is not smoothed"), FailedSlots[0].bAnchorScreenSmoothed);
	}

	Anchor->bProjectionSucceeds = true;
	Anchor->HorizontalOffset = 40.0f;
	Anchor->RefreshAnchor(0.0f);
	const TArray<FWacomFirstPersonCardLayerSlotView> RecoveredSlots = Anchor->BuildPreviewCardSlotViews();
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

	Anchor->ProjectionPadding = 0.0f;
	Anchor->HandMaxEdgeDropPixels = 0.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	Anchor->bEnableAnchorScreenSmoothing = true;
	Anchor->AnchorScreenSmoothingSpeed = 1.0f;
	Anchor->ProbeAnchorSmoothingDeltaTime = 1.0f / 60.0f;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	Anchor->BuildPreviewCardSlotViews();
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

	Anchor->ProjectionPadding = 0.0f;
	Anchor->PreviewCardCountFallback = 7;
	Anchor->AuthoredCardSpacingPixels = 160.0f;
	Anchor->AuthoredMaxHandWidthPixels = 480.0f;
	Anchor->HandMaxEdgeDropPixels = 0.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildPreviewCardSlotViews();
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
	FWacomFirstPersonCardLayerAuthoredEdgeDropHandCountScaleTest,
	"Wacom.UI.FirstPersonCardLayer.AuthoredLayout.EdgeDropScalesByHandCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerAuthoredEdgeDropHandCountScaleTest::RunTest(const FString& Parameters)
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

	auto BuildRuntimeSlots = [Anchor](int32 CardCount)
	{
		TArray<FWacomFirstPersonCardLayerEntry> Entries;
		Entries.SetNum(CardCount);
		Anchor->SetRuntimeCardLayerEntries(TEXT("EdgeDropScale"), Entries);
		return Anchor->BuildActiveCardLayerSlotViewsForTest();
	};

	Anchor->ProjectionPadding = 0.0f;
	Anchor->AuthoredCardSpacingPixels = 80.0f;
	Anchor->AuthoredMaxHandWidthPixels = 0.0f;
	Anchor->AuthoredHandScreenOffset = FVector2D::ZeroVector;
	Anchor->AuthoredCenterLiftPixels = 0.0f;
	Anchor->AuthoredDropCurveExponent = 1.0f;
	Anchor->HandMaxEdgeDropPixels = 110.0f;
	Anchor->ShortHandEdgeDropPixels = 64.0f;
	Anchor->EdgeDropScaleMinCardCount = 5;
	Anchor->EdgeDropScaleMaxCardCount = 12;
	Anchor->bScaleEdgeDropByHandCount = true;
	Anchor->bEnableCardLayerPixelSnapping = false;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const TArray<FWacomFirstPersonCardLayerSlotView> FiveSlots = BuildRuntimeSlots(5);
	const TArray<FWacomFirstPersonCardLayerSlotView> EightSlots = BuildRuntimeSlots(8);
	const TArray<FWacomFirstPersonCardLayerSlotView> TwelveSlots = BuildRuntimeSlots(12);

	TestEqual(TEXT("Five-card slot count"), FiveSlots.Num(), 5);
	TestEqual(TEXT("Eight-card slot count"), EightSlots.Num(), 8);
	TestEqual(TEXT("Twelve-card slot count"), TwelveSlots.Num(), 12);
	if (FiveSlots.Num() == 5 && EightSlots.Num() == 8 && TwelveSlots.Num() == 12)
	{
		const float EightAlpha = FMath::SmoothStep(0.0f, 1.0f, 3.0f / 7.0f);
		const float ExpectedEightDrop = FMath::Lerp(64.0f, 110.0f, EightAlpha);
		TestTrue(TEXT("Five-card edge uses short-hand drop"),
			FMath::IsNearlyEqual(FiveSlots[0].AuthoredLayoutOffset.Y, 64.0f, 0.001f));
		TestTrue(TEXT("Eight-card edge uses smooth interpolated drop"),
			FMath::IsNearlyEqual(EightSlots[0].AuthoredLayoutOffset.Y, ExpectedEightDrop, 0.001f));
		TestTrue(TEXT("Twelve-card edge uses max drop"),
			FMath::IsNearlyEqual(TwelveSlots[0].AuthoredLayoutOffset.Y, 110.0f, 0.001f));
		TestTrue(TEXT("Interpolated drop is between short and max"),
			EightSlots[0].AuthoredLayoutOffset.Y > FiveSlots[0].AuthoredLayoutOffset.Y
			&& EightSlots[0].AuthoredLayoutOffset.Y < TwelveSlots[0].AuthoredLayoutOffset.Y);
	}

	Anchor->bScaleEdgeDropByHandCount = false;
	const TArray<FWacomFirstPersonCardLayerSlotView> UnscaledFiveSlots = BuildRuntimeSlots(5);
	TestEqual(TEXT("Unscaled five-card slot count"), UnscaledFiveSlots.Num(), 5);
	if (UnscaledFiveSlots.Num() == 5)
	{
		TestTrue(TEXT("Disabled scaling keeps full edge drop"),
			FMath::IsNearlyEqual(UnscaledFiveSlots[0].AuthoredLayoutOffset.Y, 110.0f, 0.001f));
	}

	Anchor->bScaleEdgeDropByHandCount = true;
	Anchor->HandMaxEdgeDropPixels = 0.0f;
	Anchor->ShortHandEdgeDropPixels = 64.0f;
	const TArray<FWacomFirstPersonCardLayerSlotView> ZeroMaxFiveSlots = BuildRuntimeSlots(5);
	TestEqual(TEXT("Zero max five-card slot count"), ZeroMaxFiveSlots.Num(), 5);
	if (ZeroMaxFiveSlots.Num() == 5)
	{
		TestTrue(TEXT("Zero max edge drop still disables drop"),
			FMath::IsNearlyEqual(ZeroMaxFiveSlots[0].AuthoredLayoutOffset.Y, 0.0f, 0.001f));
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerAuthoredHandAnchorNormalLayoutTest,
	"Wacom.UI.FirstPersonCardLayer.AuthoredLayout.HandAnchorsUseNormalCardLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerAuthoredHandAnchorNormalLayoutTest::RunTest(const FString& Parameters)
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

	FWacomFirstPersonCardLayerEntry LeftAnchor;
	LeftAnchor.CardViewData.Name = FText::FromString(TEXT("Left Anchor"));
	LeftAnchor.bIsHandAnchor = true;

	FWacomFirstPersonCardLayerEntry NormalA;
	NormalA.CardViewData.Name = FText::FromString(TEXT("Normal A"));

	FWacomFirstPersonCardLayerEntry NormalB;
	NormalB.CardViewData.Name = FText::FromString(TEXT("Normal B"));

	FWacomFirstPersonCardLayerEntry NormalC;
	NormalC.CardViewData.Name = FText::FromString(TEXT("Normal C"));

	FWacomFirstPersonCardLayerEntry RightAnchor;
	RightAnchor.CardViewData.Name = FText::FromString(TEXT("Right Anchor"));
	RightAnchor.bIsHandAnchor = true;

	FWacomFirstPersonCardLayerEntry RightNormal;
	RightNormal.CardViewData.Name = FText::FromString(TEXT("Right Normal"));

	FWacomFirstPersonCardLayerEntry LeftNormal;
	LeftNormal.CardViewData.Name = FText::FromString(TEXT("Left Normal"));

	Anchor->ProjectionPadding = 0.0f;
	Anchor->AuthoredCardSpacingPixels = 80.0f;
	Anchor->AuthoredMaxHandWidthPixels = 0.0f;
	Anchor->AuthoredHandScreenOffset = FVector2D::ZeroVector;
	Anchor->AuthoredCenterLiftPixels = 0.0f;
	Anchor->AuthoredDropCurveExponent = 1.0f;
	Anchor->HandMaxEdgeDropPixels = 100.0f;
	Anchor->ShortHandEdgeDropPixels = 100.0f;
	Anchor->HandCardRenderScale = 0.5f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	Anchor->SetRuntimeCardLayerEntries(TEXT("BattleHand"), { LeftAnchor, NormalA, NormalB, NormalC, RightAnchor });
	const TArray<FWacomFirstPersonCardLayerSlotView> AnchorEdgeSlots =
		Anchor->BuildActiveCardLayerSlotViewsForTest();

	Anchor->SetRuntimeCardLayerEntries(TEXT("BattleHand"), { LeftNormal, NormalA, NormalB, NormalC, RightNormal });
	const TArray<FWacomFirstPersonCardLayerSlotView> NormalEdgeSlots =
		Anchor->BuildActiveCardLayerSlotViewsForTest();

	TestEqual(TEXT("Anchor-edge slot count"), AnchorEdgeSlots.Num(), 5);
	TestEqual(TEXT("Normal-edge slot count"), NormalEdgeSlots.Num(), 5);
	if (AnchorEdgeSlots.Num() == 5 && NormalEdgeSlots.Num() == 5)
	{
		TestTrue(TEXT("Left hand anchor keeps anchor identity"), AnchorEdgeSlots[0].Entry.bIsHandAnchor);
		TestTrue(TEXT("Right hand anchor keeps anchor identity"), AnchorEdgeSlots[4].Entry.bIsHandAnchor);
		TestFalse(TEXT("Comparison left card is normal"), NormalEdgeSlots[0].Entry.bIsHandAnchor);
		TestFalse(TEXT("Comparison right card is normal"), NormalEdgeSlots[4].Entry.bIsHandAnchor);
		TestTrue(TEXT("Left hand anchor uses normal edge drop"),
			FMath::IsNearlyEqual(AnchorEdgeSlots[0].AuthoredLayoutOffset.Y, 100.0f, 0.001f));
		TestTrue(TEXT("Right hand anchor uses normal edge drop"),
			FMath::IsNearlyEqual(AnchorEdgeSlots[4].AuthoredLayoutOffset.Y, 100.0f, 0.001f));
		TestTrue(TEXT("Normal edge card applies same edge drop"),
			FMath::IsNearlyEqual(NormalEdgeSlots[4].AuthoredLayoutOffset.Y, 100.0f, 0.001f));
		TestEqual(TEXT("Left hand anchor matches normal slot position"),
			AnchorEdgeSlots[0].AuthoredLayoutOffset,
			NormalEdgeSlots[0].AuthoredLayoutOffset);
		TestEqual(TEXT("Right hand anchor matches normal slot position"),
			AnchorEdgeSlots[4].AuthoredLayoutOffset,
			NormalEdgeSlots[4].AuthoredLayoutOffset);
		TestEqual(TEXT("Left hand anchor uses normal render scale"), AnchorEdgeSlots[0].RenderScale, 0.5f);
		TestEqual(TEXT("Right hand anchor uses normal render scale"), AnchorEdgeSlots[4].RenderScale, 0.5f);
		TestEqual(TEXT("Left hand anchor matches normal render angle"),
			AnchorEdgeSlots[0].RenderAngleDegrees,
			NormalEdgeSlots[0].RenderAngleDegrees);
		TestEqual(TEXT("Right hand anchor matches normal render angle"),
			AnchorEdgeSlots[4].RenderAngleDegrees,
			NormalEdgeSlots[4].RenderAngleDegrees);
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

	Anchor->ProjectionPadding = 0.0f;
	Anchor->PreviewCardCountFallback = 5;
	Anchor->AuthoredCardSpacingPixels = 100.0f;
	Anchor->AuthoredMaxHandWidthPixels = 0.0f;
	Anchor->HandMaxEdgeDropPixels = 80.0f;
	Anchor->ShortHandEdgeDropPixels = 80.0f;
	Anchor->AuthoredCenterLiftPixels = 20.0f;
	Anchor->AuthoredDropCurveExponent = 2.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const TArray<FWacomFirstPersonCardLayerSlotView> SquareSlots = Anchor->BuildPreviewCardSlotViews();
	Anchor->AuthoredDropCurveExponent = 1.0f;
	const TArray<FWacomFirstPersonCardLayerSlotView> LinearSlots = Anchor->BuildPreviewCardSlotViews();

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

	Anchor->FanYawDegrees = 6.0f;
	Anchor->AuthoredFanCurveExponent = 2.0f;
	Anchor->MaxCardLayerRenderAngleDegrees = 4.0f;
	Anchor->bClampCardLayerRenderAngle = true;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const TArray<FWacomFirstPersonCardLayerSlotView> ClampedSlots = Anchor->BuildPreviewCardSlotViews();
	Anchor->bClampCardLayerRenderAngle = false;
	const TArray<FWacomFirstPersonCardLayerSlotView> UnclampedSlots = Anchor->BuildPreviewCardSlotViews();

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
	Anchor->HandCardRenderScale = 1.0f;
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
	FWacomFirstPersonCardLayerTestAccess::SetHoveredCardInstanceId(*Anchor, HoveredId);
	const TArray<FWacomFirstPersonCardLayerSlotView> NearSlots = Anchor->BuildActiveCardLayerSlotViewsForTest();

	Anchor->DistanceFromView = 500.0f;
	Anchor->RefreshAnchor(0.0f);
	const TArray<FWacomFirstPersonCardLayerSlotView> FarSlots = Anchor->BuildActiveCardLayerSlotViewsForTest();

	TestEqual(TEXT("Near slot count"), NearSlots.Num(), 2);
	TestEqual(TEXT("Far slot count"), FarSlots.Num(), 2);
	if (NearSlots.Num() == 2 && FarSlots.Num() == 2)
	{
		TestEqual(TEXT("Pending base scale stays stable"), NearSlots[0].RenderScale, 1.0f);
		TestEqual(TEXT("Hovered base scale stays stable"), NearSlots[1].RenderScale, 1.0f);
		TestTrue(TEXT("Pending slot keeps pending marker"), NearSlots[0].Entry.bIsPendingTargeting);
		TestTrue(TEXT("Hovered slot keeps hover marker"), NearSlots[1].bIsHovered);
		TestEqual(TEXT("Changing projection distance does not change pending base scale"), FarSlots[0].RenderScale, NearSlots[0].RenderScale);
		TestEqual(TEXT("Changing projection distance does not change hovered base scale"), FarSlots[1].RenderScale, NearSlots[1].RenderScale);
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
	Anchor->PreviewCardCountFallback = 5;
	Anchor->HoverZOrderBoost = 500;

	const TArray<FWacomFirstPersonCardLayerSlotView> BaseSlots = Anchor->BuildPreviewCardSlotViews();
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
	FWacomFirstPersonCardLayerTestAccess::SetHoveredCardInstanceId(*Anchor, HoveredId);
	const TArray<FWacomFirstPersonCardLayerSlotView> HoverSlots = Anchor->BuildActiveCardLayerSlotViewsForTest();

	TestEqual(TEXT("Base slot count"), BaseSlots.Num(), 5);
	TestEqual(TEXT("Hover slot count"), HoverSlots.Num(), 5);
	if (BaseSlots.Num() == 5 && HoverSlots.Num() == 5)
	{
		TestTrue(TEXT("Center card draws above left edge"), BaseSlots[2].ZOrder > BaseSlots[0].ZOrder);
		TestTrue(TEXT("Center card draws above right edge"), BaseSlots[2].ZOrder > BaseSlots[4].ZOrder);
		TestTrue(TEXT("Hovered edge keeps base z-order in anchor slot"), HoverSlots[4].ZOrder < HoverSlots[2].ZOrder);
		TestTrue(TEXT("Hovered edge is marked for presentation z-order"), HoverSlots[4].bIsHovered);

		UWacomFirstPersonCardLayerSlotWidget* HoveredSlotWidget =
			NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
		if (TestNotNull(TEXT("Hovered edge slot widget"), HoveredSlotWidget))
		{
			FWacomFirstPersonCardSlotMotionConfig MotionConfig;
			MotionConfig.bEnabled = false;
			HoveredSlotWidget->SetSlotMotionConfig(MotionConfig);
			FWacomFirstPersonCardSlotVisualConfig VisualConfig;
			VisualConfig.HoverZOrderBoost = Anchor->HoverZOrderBoost;
			HoveredSlotWidget->SetSlotVisualConfig(VisualConfig);
			HoveredSlotWidget->SetCardLayerInteractionEnabled(true);
			HoveredSlotWidget->SetSlotViewImmediate(HoverSlots[4]);
			TestTrue(TEXT("Hover presentation z-order still wins over center default z-order"),
				HoveredSlotWidget->GetVisualSlotView().ZOrder > HoverSlots[2].ZOrder);
		}
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
	Anchor->ProjectionPadding = 0.0f;
	Anchor->PreviewCardCountFallback = 5;
	Anchor->HandMaxEdgeDropPixels = 40.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildPreviewCardSlotViews();
	const FWacomFirstPersonCardAnchorDebugView View = Anchor->GetFirstPersonCardAnchorDebugView(5);

	TestEqual(TEXT("Slot count"), Slots.Num(), 5);
	TestEqual(TEXT("Debug point count"), View.ProjectedPoints.Num(), 5);
	if (Slots.Num() == 5 && View.ProjectedPoints.Num() == 5)
	{
		TestEqual(TEXT("Debug point matches authored slot position"), View.ProjectedPoints[3].ScreenPosition, Slots[3].ScreenPosition);
		TestEqual(TEXT("Debug point matches authored offset"), View.ProjectedPoints[3].AuthoredLayoutOffset, Slots[3].AuthoredLayoutOffset);
		TestEqual(TEXT("Debug point records normalized offset"), View.ProjectedPoints[3].NormalizedHandOffset, Slots[3].NormalizedHandOffset);
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerStaticHitTestTest,
	"Wacom.UI.FirstPersonCardLayer.PreviewLayer.HitTestInvisibleAndNoInputBindings",
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
	Layer->SetCardSlots({ Slot });
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
	"Wacom.UI.FirstPersonCardLayer.PreviewLayer.ProjectionFailureHidesCards",
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
	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildPreviewCardSlotViews();
	TestEqual(TEXT("Projection failure still builds slot data"), Slots.Num(), 5);
	if (Slots.Num() > 0)
	{
		TestFalse(TEXT("Slot is not projected"), Slots[0].bProjected);
	}

	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (TestNotNull(TEXT("Layer widget"), Layer))
	{
		Layer->SetCardSlots(Slots);
		TestFalse(TEXT("Failed projection hides card"), Layer->IsCardSlotVisible(0));
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerToggleRemovesWidgetTest,
	"Wacom.UI.FirstPersonCardLayer.PreviewLayer.ToggleRemovesWidget",
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
	Anchor->bDrawPreviewCardLayer = true;
	Anchor->RefreshCardLayerForTest();
	TestTrue(TEXT("Preview layer is created"), Anchor->IsCardLayerWidgetActive());

	Anchor->bDrawPreviewCardLayer = false;
	Anchor->RefreshCardLayerForTest();
	TestFalse(TEXT("Preview layer is removed after toggle off"), Anchor->IsCardLayerWidgetActive());

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
	Anchor->HandCardRenderScale = 1.0f;
	Anchor->HandMaxEdgeDropPixels = 80.0f;
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
	FWacomFirstPersonCardLayerTestAccess::SetHoveredCardInstanceId(*Anchor, HoveredCardId);

	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildActiveCardLayerSlotViewsForTest();
	TestEqual(TEXT("Base slot count"), BaseSlots.Num(), 3);
	TestEqual(TEXT("Slot count"), Slots.Num(), 3);
	if (BaseSlots.Num() == 3 && Slots.Num() == 3)
	{
		TestTrue(TEXT("Body locked layout is recorded"), Slots[1].bBodyLockedLayout);
		TestTrue(TEXT("Current camera projection is recorded"), Slots[1].bCurrentCameraProjection);
		TestTrue(TEXT("Left edge drop still lowers edge card"), BaseSlots[0].ScreenPosition.Y > BaseSlots[1].ScreenPosition.Y);
		TestEqual(TEXT("Pending keeps base center position in anchor slot"), Slots[1].ScreenPosition, BaseSlots[1].ScreenPosition);
		TestEqual(TEXT("Hover keeps base edge position in anchor slot"), Slots[2].ScreenPosition, BaseSlots[2].ScreenPosition);
		TestTrue(TEXT("Pending state is marked for presentation"), Slots[1].Entry.bIsPendingTargeting);
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

	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid())
	});

	TestEqual(TEXT("Interaction off keeps layer hit-test-invisible"), Layer->GetVisibility(), ESlateVisibility::HitTestInvisible);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		TestEqual(TEXT("Interaction off keeps slot hit-test-invisible"), SlotWidget->GetVisibility(), ESlateVisibility::HitTestInvisible);
		TestFalse(TEXT("Interaction off rejects programmatic drag"), Layer->TryStartCardDragGesture(SlotWidget->GetSlotView().Entry.CardInstanceId));
	}

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
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
	Layer->SetCardSlots({ WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(200.0f, 300.0f), 10.0f, 1.2f, 0.4f) });
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.25f);

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
	FWacomFirstPersonCardLayerMotionEasePowerDefaultTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotion.EasePowerOneKeepsLinearMotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerMotionEasePowerDefaultTest::RunTest(const FString& Parameters)
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
	Config.MotionSpeed = 1.0f;
	Config.OpacitySpeed = 1.0f;
	Config.EasePower = 1.0f;
	Config.EnterOffsetPixels = FVector2D::ZeroVector;
	Config.EnterOpacity = 1.0f;
	Layer->SetSlotMotionConfig(Config);

	const FGuid CardId = FGuid::NewGuid();
	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(100.0f, 200.0f), 0.0f, 1.0f, 1.0f)
	});
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(200.0f, 300.0f), 10.0f, 1.2f, 0.4f)
	});
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.25f);

	if (UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0))
	{
		const FWacomFirstPersonCardLayerSlotView& Visual = SlotWidget->GetVisualSlotView();
		TestTrue(TEXT("Linear ease position X"),
			FMath::IsNearlyEqual(Visual.ScreenPosition.X, 125.0f, 0.01f));
		TestTrue(TEXT("Linear ease position Y"),
			FMath::IsNearlyEqual(Visual.ScreenPosition.Y, 225.0f, 0.01f));
		TestTrue(TEXT("Linear ease angle"),
			FMath::IsNearlyEqual(Visual.RenderAngleDegrees, 2.5f, 0.01f));
		TestTrue(TEXT("Linear ease scale"),
			FMath::IsNearlyEqual(Visual.RenderScale, 1.05f, 0.001f));
		TestTrue(TEXT("Linear ease opacity"),
			FMath::IsNearlyEqual(Visual.RenderOpacity, 0.85f, 0.001f));
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerMotionEasePowerSoftensTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotion.EasePowerAboveOneSoftensMotionStep",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerMotionEasePowerSoftensTest::RunTest(const FString& Parameters)
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
	Config.MotionSpeed = 1.0f;
	Config.OpacitySpeed = 1.0f;
	Config.EasePower = 2.0f;
	Config.EnterOffsetPixels = FVector2D::ZeroVector;
	Config.EnterOpacity = 1.0f;
	Layer->SetSlotMotionConfig(Config);

	const FGuid CardId = FGuid::NewGuid();
	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(100.0f, 200.0f), 0.0f, 1.0f, 1.0f)
	});
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(200.0f, 300.0f), 10.0f, 1.2f, 0.4f)
	});
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.25f);

	if (UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0))
	{
		const FWacomFirstPersonCardLayerSlotView& Visual = SlotWidget->GetVisualSlotView();
		TestTrue(TEXT("Ease power position X"),
			FMath::IsNearlyEqual(Visual.ScreenPosition.X, 106.25f, 0.01f));
		TestTrue(TEXT("Ease power position Y"),
			FMath::IsNearlyEqual(Visual.ScreenPosition.Y, 206.25f, 0.01f));
		TestTrue(TEXT("Ease power angle"),
			FMath::IsNearlyEqual(Visual.RenderAngleDegrees, 0.625f, 0.01f));
		TestTrue(TEXT("Ease power scale"),
			FMath::IsNearlyEqual(Visual.RenderScale, 1.0125f, 0.001f));
		TestTrue(TEXT("Ease power opacity"),
			FMath::IsNearlyEqual(Visual.RenderOpacity, 0.9625f, 0.001f));
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerMotionHoverProfileTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotion.HoverProfileOnlyAffectsHoverIntent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerMotionHoverProfileTest::RunTest(const FString& Parameters)
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
	Config.LayoutMotionProfile.MotionSpeed = 1.0f;
	Config.LayoutMotionProfile.OpacitySpeed = 1.0f;
	Config.LayoutMotionProfile.EasePower = 1.0f;
	Config.HoverMotionProfile.MotionSpeed = 4.0f;
	Config.HoverMotionProfile.OpacitySpeed = 4.0f;
	Config.HoverMotionProfile.EasePower = 1.0f;
	Layer->SetSlotMotionConfig(Config);
	FWacomFirstPersonCardSlotVisualConfig VisualConfig;
	VisualConfig.HoverLiftPixels = 40.0f;
	VisualConfig.HoverScale = 1.10f;
	Layer->SetSlotVisualConfig(VisualConfig);

	const FGuid CardId = FGuid::NewGuid();
	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(100.0f, 200.0f), 0.0f, 1.0f, 1.0f)
	});
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(200.0f, 300.0f), 0.0f, 1.0f, 1.0f)
	});
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.25f);

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}
	TestTrue(TEXT("Layout motion still uses layout profile"),
		FMath::IsNearlyEqual(SlotWidget->GetVisualSlotView().ScreenPosition.X, 125.0f, 0.01f));
	TestEqual(TEXT("Layout intent selected"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).ActiveMotionIntent,
		EWacomFirstPersonCardMotionIntent::Layout);

	FWacomFirstPersonCardLayerSlotView HoverSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(200.0f, 300.0f), 0.0f, 1.0f, 1.0f);
	HoverSlot.bIsHovered = true;
	Layer->SetCardSlots({ HoverSlot });
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.25f);
	TestEqual(TEXT("Hover intent selected"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).ActiveMotionIntent,
		EWacomFirstPersonCardMotionIntent::Hover);
	TestTrue(TEXT("Hover profile moves to hover lift immediately"),
		FMath::IsNearlyEqual(SlotWidget->GetVisualSlotView().ScreenPosition.Y, 260.0f, 0.01f));
	TestTrue(TEXT("Hover profile moves to hover scale immediately"),
		FMath::IsNearlyEqual(SlotWidget->GetVisualSlotView().RenderScale, 1.10f, 0.001f));

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerMotionDragTargetFocusProfileTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotion.DragTargetFocusProfileOnlyAffectsFocusedTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerMotionDragTargetFocusProfileTest::RunTest(const FString& Parameters)
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
	Config.DragTargetFocusMotionProfile.MotionSpeed = 4.0f;
	Config.DragTargetFocusMotionProfile.OpacitySpeed = 4.0f;
	Config.DragTargetFocusMotionProfile.EasePower = 1.0f;
	Layer->SetSlotMotionConfig(Config);
	FWacomFirstPersonCardSlotVisualConfig VisualConfig;
	VisualConfig.DragCardTargetFocusLiftPixels = 24.0f;
	VisualConfig.DragCardTargetFocusScale = 1.08f;
	Layer->SetSlotVisualConfig(VisualConfig);
	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.bEnableDragTargetFeedback = true;
	Layer->SetCardDragConfig(DragConfig);

	const FGuid CardId = FGuid::NewGuid();
	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(300.0f, 400.0f), 0.0f, 1.0f, 1.0f)
	});
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	SlotWidget->SetCardDragTargetAffordanceFeedback(
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget,
		true);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.25f);
	TestTrue(TEXT("Affordance does not lift target"),
		FMath::IsNearlyEqual(SlotWidget->GetVisualSlotView().ScreenPosition.Y, 400.0f, 0.01f));

	SlotWidget->SetCardDragProbeFeedback(true, true);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.25f);
	TestEqual(TEXT("Focus intent selected"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).ActiveMotionIntent,
		EWacomFirstPersonCardMotionIntent::DragTargetFocus);
	TestTrue(TEXT("Focus profile moves to focus lift immediately"),
		FMath::IsNearlyEqual(SlotWidget->GetVisualSlotView().ScreenPosition.Y, 376.0f, 0.01f));
	TestTrue(TEXT("Focus profile moves to focus scale immediately"),
		FMath::IsNearlyEqual(SlotWidget->GetVisualSlotView().RenderScale, 1.08f, 0.001f));

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerMotionEnterExitProfilesTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotion.EnterAndExitUseDedicatedProfiles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerMotionEnterExitProfilesTest::RunTest(const FString& Parameters)
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
	Config.EnterOffsetPixels = FVector2D(0.0f, 40.0f);
	Config.EnterOpacity = 0.0f;
	Config.ExitOffsetPixels = FVector2D(0.0f, 40.0f);
	Config.ExitDuration = 1.0f;
	Config.EnterMotionProfile.MotionSpeed = 4.0f;
	Config.EnterMotionProfile.OpacitySpeed = 4.0f;
	Config.EnterMotionProfile.EasePower = 1.0f;
	Config.ExitMotionProfile.MotionSpeed = 0.5f;
	Config.ExitMotionProfile.OpacitySpeed = 0.5f;
	Config.ExitMotionProfile.EasePower = 1.0f;
	Layer->SetSlotMotionConfig(Config);

	const FGuid CardId = FGuid::NewGuid();
	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(100.0f, 200.0f), 0.0f, 1.0f, 1.0f)
	});
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.25f);
	TestEqual(TEXT("Enter intent selected"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).ActiveMotionIntent,
		EWacomFirstPersonCardMotionIntent::Enter);
	TestTrue(TEXT("Enter profile reaches target in one tick"),
		FMath::IsNearlyEqual(SlotWidget->GetVisualSlotView().ScreenPosition.Y, 200.0f, 0.01f));
	TestTrue(TEXT("Enter profile reaches full opacity"),
		FMath::IsNearlyEqual(SlotWidget->GetVisualSlotView().RenderOpacity, 1.0f, 0.001f));

	Layer->SetCardSlots({});
	UWacomFirstPersonCardLayerSlotWidget* Outgoing = FWacomFirstPersonCardLayerTestAccess::OutgoingSlotAt(*Layer, 0);
	if (!TestNotNull(TEXT("Outgoing slot"), Outgoing))
	{
		PC->Destroy();
		return false;
	}
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.25f);
	TestEqual(TEXT("Exit intent selected"),
		FWacomFirstPersonCardLayerTestAccess::View(*Outgoing).ActiveMotionIntent,
		EWacomFirstPersonCardMotionIntent::Exit);
	TestTrue(TEXT("Exit profile moves slowly toward exit target"),
		FMath::IsNearlyEqual(Outgoing->GetVisualSlotView().ScreenPosition.Y, 205.0f, 0.01f));
	TestTrue(TEXT("Exit profile fades slowly"),
		FMath::IsNearlyEqual(Outgoing->GetVisualSlotView().RenderOpacity, 0.875f, 0.001f));

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
	FWacomFirstPersonCardSlotVisualConfig VisualConfig;
	VisualConfig.PendingTargetingLiftPixels = 60.0f;
	VisualConfig.PendingTargetingScale = 1.15f;
	VisualConfig.PendingTargetingZOrderBoost = 1000;
	Layer->SetSlotVisualConfig(VisualConfig);
	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView BaseSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(100.0f, 300.0f), 0.0f, 1.0f);
	Layer->SetCardSlots({ BaseSlot });
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
	FWacomFirstPersonCardLayerSlotView PendingHoverSlot = BaseSlot;
	PendingHoverSlot.bIsHovered = true;
	PendingHoverSlot.Entry.bIsPendingTargeting = true;
	Layer->SetCardLayerInteractionEnabled(true);
	Layer->SetCardSlots({ PendingHoverSlot });

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.25f);
		TestTrue(TEXT("Hover/pending lift animates"), SlotWidget->GetVisualSlotView().ScreenPosition.Y < BaseSlot.ScreenPosition.Y);
		TestTrue(TEXT("Hover/pending lift has not snapped immediately"), SlotWidget->GetVisualSlotView().ScreenPosition.Y > BaseSlot.ScreenPosition.Y - VisualConfig.PendingTargetingLiftPixels);
		TestTrue(TEXT("Programmatic drag still uses original card id"), Layer->TryStartCardDragGesture(CardId));
		TestEqual(TEXT("Drag forwards original card id"), FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.CardInstanceId, CardId);
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerMotionZOrderOnlyPresentationTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotion.ZOrderOnlyPresentationUpdatesCanvas",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerMotionZOrderOnlyPresentationTest::RunTest(const FString& Parameters)
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
	FWacomFirstPersonCardSlotVisualConfig VisualConfig;
	VisualConfig.HoverLiftPixels = 0.0f;
	VisualConfig.HoverScale = 1.0f;
	VisualConfig.HoverZOrderBoost = 250;
	Layer->SetSlotVisualConfig(VisualConfig);

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView BaseSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(100.0f, 300.0f), 0.0f, 1.0f);
	BaseSlot.ZOrder = 12;
	Layer->SetCardSlots({ BaseSlot });
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
	TestEqual(TEXT("Base canvas z-order"), Layer->GetCardZOrderAt(0), BaseSlot.ZOrder);

	FWacomFirstPersonCardLayerSlotView HoverSlot = BaseSlot;
	HoverSlot.bIsHovered = true;
	Layer->SetCardSlots({ HoverSlot });
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.01f);
	TestEqual(TEXT("Z-order-only presentation updates canvas"),
		Layer->GetCardZOrderAt(0),
		BaseSlot.ZOrder + VisualConfig.HoverZOrderBoost);

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
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
	Layer->SetCardSlots({});

	TestEqual(TEXT("Active slot removed from hand"), Layer->GetCardViewCount(), 0);
	TestEqual(TEXT("Outgoing slot is retained"), Layer->GetOutgoingCardViewCount(), 1);
	UWacomFirstPersonCardLayerSlotWidget* Outgoing = FWacomFirstPersonCardLayerTestAccess::OutgoingSlotAt(*Layer, 0);
	if (TestNotNull(TEXT("Outgoing slot"), Outgoing))
	{
		TestTrue(TEXT("Outgoing slot is exiting"), Outgoing->IsExitingForFirstPersonLayer());
	}
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.25f);
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
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);

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
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.25f);
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
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
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
	FWacomFirstPersonCardLayerMotionDevelopmentPreviewKeyTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotion.DevelopmentPreviewUsesStableIndexKeys",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerMotionDevelopmentPreviewKeyTest::RunTest(const FString& Parameters)
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
	Layer->SetCardSlots({ FirstSlot, SecondSlot });
	UWacomFirstPersonCardLayerSlotWidget* FirstWidget = Layer->GetSlotWidgetAt(0);

	FirstSlot.ScreenPosition = FVector2D(140.0f, 220.0f);
	FirstSlot.WidgetPosition = FirstSlot.ScreenPosition;
	FirstSlot.SnappedWidgetPosition = FirstSlot.ScreenPosition;
	Layer->SetCardSlots({ FirstSlot, SecondSlot });
	TestEqual(TEXT("Static index key reuses widget"), Layer->GetSlotWidgetAt(0), FirstWidget);
	TestNotNull(TEXT("StaticIndex key can be found"), FWacomFirstPersonCardLayerTestAccess::FindSlotWidgetByKey(*Layer, TEXT("StaticIndex:0")));

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
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);

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
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);

	FWacomFirstPersonCardLayerTransitionHint Hint =
		WacomFirstPersonCardLayerSpec::MakeTransitionHint(
			CardId,
			EWacomFirstPersonCardSlotTransitionKind::Played);
	Hint.bPlayCommitFeedback = true;
	Layer->SetCardTransitionHints({ Hint });
	Layer->SetCardSlots({});

	UWacomFirstPersonCardLayerSlotWidget* Outgoing = FWacomFirstPersonCardLayerTestAccess::OutgoingSlotAt(*Layer, 0);
	if (TestNotNull(TEXT("Outgoing played slot"), Outgoing))
	{
		TestTrue(TEXT("Commit feedback starts on played outgoing"), FWacomFirstPersonCardLayerTestAccess::View(*Outgoing).bCommitFeedbackActive);
		TestEqual(
			TEXT("Commit uses interaction feedback"),
			FWacomFirstPersonCardLayerTestAccess::View(*Outgoing).InteractionFeedbackKind,
			EWacomFirstPersonCardInteractionFeedbackKind::Commit);
		TestEqual(TEXT("Commit interaction feedback opacity"), FWacomFirstPersonCardLayerTestAccess::View(*Outgoing).InteractionFeedbackOpacity, 0.6f);
		TestEqual(TEXT("Commit does not use full-card overlay"), FWacomFirstPersonCardLayerTestAccess::View(*Outgoing).FeedbackOverlayOpacity, 0.0f);
		FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.1f);
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
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);

	FWacomFirstPersonCardLayerTransitionHint Hint =
		WacomFirstPersonCardLayerSpec::MakeTransitionHint(
			CardId,
			EWacomFirstPersonCardSlotTransitionKind::Played);
	Hint.bPlayCommitFeedback = true;
	Layer->SetCardTransitionHints({ Hint });
	Layer->SetCardSlots({});

	UWacomFirstPersonCardLayerSlotWidget* Outgoing = FWacomFirstPersonCardLayerTestAccess::OutgoingSlotAt(*Layer, 0);
	if (TestNotNull(TEXT("Outgoing played slot"), Outgoing))
	{
		FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
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
	HUD->ClearPendingFirstPersonCardTransitionEventsForTest();
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
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);

	FWacomFirstPersonCardLayerTransitionHint Hint =
		WacomFirstPersonCardLayerSpec::MakeTransitionHint(
			FirstCardId,
			EWacomFirstPersonCardSlotTransitionKind::Played);
	Hint.bPlayCommitFeedback = true;
	Layer->SetCardTransitionHints({ Hint });
	Layer->SetCardSlots({});
	UWacomFirstPersonCardLayerSlotWidget* FirstOutgoing = FWacomFirstPersonCardLayerTestAccess::OutgoingSlotAt(*Layer, 0);
	TestTrue(TEXT("First outgoing gets commit"), FirstOutgoing && FWacomFirstPersonCardLayerTestAccess::View(*FirstOutgoing).bCommitFeedbackActive);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.25f);

	Layer->SetCardSlots({ WacomFirstPersonCardLayerSpec::MakeMotionSlot(SecondCardId, 0, FVector2D(100.0f, 200.0f)) });
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
	Layer->SetCardSlots({});
	UWacomFirstPersonCardLayerSlotWidget* SecondOutgoing = FWacomFirstPersonCardLayerTestAccess::OutgoingSlotAt(*Layer, 0);
	if (TestNotNull(TEXT("Second outgoing slot"), SecondOutgoing))
	{
		TestFalse(TEXT("Commit hint does not leak to next refresh"), FWacomFirstPersonCardLayerTestAccess::View(*SecondOutgoing).bCommitFeedbackActive);
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
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);

	FWacomFirstPersonCardLayerTransitionHint Hint =
		WacomFirstPersonCardLayerSpec::MakeTransitionHint(
			CardId,
			EWacomFirstPersonCardSlotTransitionKind::Played);
	Hint.bPlayCommitFeedback = true;
	Hint.bHasPlayedExitTargetWidgetPosition = true;
	Hint.PlayedExitTargetWidgetPosition = FVector2D(600.0f, 100.0f);
	Layer->SetCardTransitionHints({ Hint });
	Layer->SetCardSlots({});

	UWacomFirstPersonCardLayerSlotWidget* Outgoing = FWacomFirstPersonCardLayerTestAccess::OutgoingSlotAt(*Layer, 0);
	if (TestNotNull(TEXT("Outgoing played slot"), Outgoing))
	{
		FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.1f);
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
	TestTrue(TEXT("Commit feedback starts"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bCommitFeedbackActive);
	TestEqual(
		TEXT("Commit starts as interaction feedback"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionFeedbackKind,
		EWacomFirstPersonCardInteractionFeedbackKind::Commit);

	SlotWidget->SetCardLayerInteractionEnabled(false);
	TestFalse(TEXT("Interaction disabled clears commit"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bCommitFeedbackActive);
	TestEqual(
		TEXT("Interaction disabled clears interaction feedback"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionFeedbackKind,
		EWacomFirstPersonCardInteractionFeedbackKind::None);

	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->TriggerCommitFeedback();
	TestTrue(TEXT("Commit feedback restarts"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bCommitFeedbackActive);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid()));
	TestFalse(TEXT("Slot reuse clears commit"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bCommitFeedbackActive);
	TestEqual(
		TEXT("Slot reuse clears interaction feedback"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionFeedbackKind,
		EWacomFirstPersonCardInteractionFeedbackKind::None);

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
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
	Layer->SetCardTransitionHints({
		WacomFirstPersonCardLayerSpec::MakeTransitionHint(CardId, EWacomFirstPersonCardSlotTransitionKind::Played)
	});
	Layer->SetCardSlots({});

	UWacomFirstPersonCardLayerSlotWidget* Outgoing = FWacomFirstPersonCardLayerTestAccess::OutgoingSlotAt(*Layer, 0);
	if (TestNotNull(TEXT("Outgoing played slot"), Outgoing))
	{
		FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.1f);
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
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
	Layer->SetCardTransitionHints({
		WacomFirstPersonCardLayerSpec::MakeTransitionHint(CardId, EWacomFirstPersonCardSlotTransitionKind::Discarded)
	});
	Layer->SetCardSlots({});

	UWacomFirstPersonCardLayerSlotWidget* Outgoing = FWacomFirstPersonCardLayerTestAccess::OutgoingSlotAt(*Layer, 0);
	if (TestNotNull(TEXT("Outgoing discarded slot"), Outgoing))
	{
		FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.1f);
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
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
	UWacomFirstPersonCardLayerSlotWidget* FirstWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* SecondWidget = Layer->GetSlotWidgetAt(1);

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

	FWacomFirstPersonCardLayerTestAccess::SetViewportSizeOverride(*Layer, FVector2D(1000.0f, 800.0f));
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
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
	Layer->SetCardTransitionHints({
		WacomFirstPersonCardLayerSpec::MakeTransitionHint(PlayedId, EWacomFirstPersonCardSlotTransitionKind::Played),
		WacomFirstPersonCardLayerSpec::MakeTransitionHint(DiscardedId, EWacomFirstPersonCardSlotTransitionKind::Discarded)
	});
	Layer->SetCardSlots({});

	UWacomFirstPersonCardLayerSlotWidget* PlayedOutgoing = FWacomFirstPersonCardLayerTestAccess::OutgoingSlotAt(*Layer, 0);
	UWacomFirstPersonCardLayerSlotWidget* DiscardedOutgoing = FWacomFirstPersonCardLayerTestAccess::OutgoingSlotAt(*Layer, 1);
	if (TestNotNull(TEXT("Played outgoing"), PlayedOutgoing))
	{
		FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.1f);
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
	FWacomFirstPersonCardLayerExactDrawIdsOverrideSnapshotGuessTest,
	"Wacom.UI.FirstPersonCardLayer.EventAwareTransitions.CardsDrawnExactIdsOverrideSnapshotGuess",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerExactDrawIdsOverrideSnapshotGuessTest::RunTest(const FString& Parameters)
{
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}

	FHandCardSnapshot Existing = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(nullptr, 1, true);
	FHandCardSnapshot Redrawn = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(nullptr, 1, true);
	FHandCardSnapshot UnrelatedNew = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(nullptr, 1, true);
	const FBattleSnapshot Previous = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({ Existing, Redrawn });
	const FBattleSnapshot Next = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({ Existing, Redrawn, UnrelatedNew });

	FBattleEvent DrawEvent = WacomFirstPersonCardLayerSpec::MakeBattleEvent(EBattleEventType::CardsDrawn);
	DrawEvent.CardInstanceIds = { Redrawn.InstanceId };
	DrawEvent.Count = DrawEvent.CardInstanceIds.Num();
	HUD->StoreFirstPersonCardTransitionEventsForTest({ DrawEvent });
	const TArray<FWacomFirstPersonCardLayerTransitionHint> Hints =
		HUD->BuildFirstPersonCardTransitionHintsForTest(Previous, Next);

	TestEqual(TEXT("Only exact drawn card gets a hint"), Hints.Num(), 1);
	const FWacomFirstPersonCardLayerTransitionHint* RedrawnHint =
		WacomFirstPersonCardLayerSpec::FindTransitionHint(Hints, Redrawn.InstanceId);
	TestNotNull(TEXT("Exact redrawn card gets draw hint"), RedrawnHint);
	if (RedrawnHint)
	{
		TestEqual(TEXT("Exact redrawn transition kind"), RedrawnHint->TransitionKind, EWacomFirstPersonCardSlotTransitionKind::Drawn);
		TestEqual(TEXT("Exact redrawn sequence index"), RedrawnHint->SequenceIndex, 0);
		TestEqual(TEXT("Exact redrawn sequence count"), RedrawnHint->SequenceCount, 1);
	}
	TestNull(
		TEXT("Snapshot new card outside CardInstanceIds does not get draw hint"),
		WacomFirstPersonCardLayerSpec::FindTransitionHint(Hints, UnrelatedNew.InstanceId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerExactDrawIdsSkipInvisibleCardsTest,
	"Wacom.UI.FirstPersonCardLayer.EventAwareTransitions.CardsDrawnExactIdsSkipInvisibleCards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerExactDrawIdsSkipInvisibleCardsTest::RunTest(const FString& Parameters)
{
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}

	FHandCardSnapshot Existing = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(nullptr, 1, true);
	FHandCardSnapshot FirstVisibleDrawn = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(nullptr, 1, true);
	FHandCardSnapshot SecondVisibleDrawn = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(nullptr, 1, true);
	FHandCardSnapshot UnrelatedNew = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(nullptr, 1, true);
	const FGuid DiscardedByLimitId = FGuid::NewGuid();
	const FBattleSnapshot Previous = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({ Existing });
	const FBattleSnapshot Next = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({
		Existing,
		FirstVisibleDrawn,
		UnrelatedNew,
		SecondVisibleDrawn
	});

	FBattleEvent DrawEvent = WacomFirstPersonCardLayerSpec::MakeBattleEvent(EBattleEventType::CardsDrawn);
	DrawEvent.CardInstanceIds = {
		DiscardedByLimitId,
		FirstVisibleDrawn.InstanceId,
		SecondVisibleDrawn.InstanceId
	};
	DrawEvent.Count = DrawEvent.CardInstanceIds.Num();
	HUD->StoreFirstPersonCardTransitionEventsForTest({ DrawEvent });
	const TArray<FWacomFirstPersonCardLayerTransitionHint> Hints =
		HUD->BuildFirstPersonCardTransitionHintsForTest(Previous, Next);

	TestEqual(TEXT("Only visible exact drawn cards get hints"), Hints.Num(), 2);
	const FWacomFirstPersonCardLayerTransitionHint* FirstHint =
		WacomFirstPersonCardLayerSpec::FindTransitionHint(Hints, FirstVisibleDrawn.InstanceId);
	const FWacomFirstPersonCardLayerTransitionHint* SecondHint =
		WacomFirstPersonCardLayerSpec::FindTransitionHint(Hints, SecondVisibleDrawn.InstanceId);
	if (TestNotNull(TEXT("First visible drawn hint"), FirstHint))
	{
		TestEqual(TEXT("First visible drawn sequence index"), FirstHint->SequenceIndex, 0);
		TestEqual(TEXT("First visible drawn sequence count"), FirstHint->SequenceCount, 2);
	}
	if (TestNotNull(TEXT("Second visible drawn hint"), SecondHint))
	{
		TestEqual(TEXT("Second visible drawn sequence index"), SecondHint->SequenceIndex, 1);
		TestEqual(TEXT("Second visible drawn sequence count"), SecondHint->SequenceCount, 2);
	}
	TestNull(
		TEXT("Unrelated snapshot-new card does not get exact draw hint"),
		WacomFirstPersonCardLayerSpec::FindTransitionHint(Hints, UnrelatedNew.InstanceId));
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
	FWacomFirstPersonCardLayerCardDiscardedHintAssignmentTest,
	"Wacom.UI.FirstPersonCardLayer.EventAwareTransitions.CardDiscardedEventAssignsDiscardedExitHint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerCardDiscardedHintAssignmentTest::RunTest(const FString& Parameters)
{
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}

	FHandCardSnapshot Discarded = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(nullptr, 1, true);
	const FBattleSnapshot Previous = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({ Discarded });
	const FBattleSnapshot Next = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({});

	HUD->StoreFirstPersonCardTransitionEventsForTest({
		WacomFirstPersonCardLayerSpec::MakeBattleEvent(EBattleEventType::CardDiscarded, Discarded.InstanceId)
	});
	const TArray<FWacomFirstPersonCardLayerTransitionHint> Hints =
		HUD->BuildFirstPersonCardTransitionHintsForTest(Previous, Next);

	const FWacomFirstPersonCardLayerTransitionHint* Hint =
		WacomFirstPersonCardLayerSpec::FindTransitionHint(Hints, Discarded.InstanceId);
	TestNotNull(TEXT("CardDiscarded hint exists"), Hint);
	if (Hint)
	{
		TestEqual(TEXT("CardDiscarded maps to discarded transition"),
			Hint->TransitionKind, EWacomFirstPersonCardSlotTransitionKind::Discarded);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerCardExhaustedHintAssignmentTest,
	"Wacom.UI.FirstPersonCardLayer.EventAwareTransitions.CardExhaustedEventAssignsDiscardedExitHint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerCardExhaustedHintAssignmentTest::RunTest(const FString& Parameters)
{
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}

	FHandCardSnapshot Exhausted = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(nullptr, 1, true);
	const FBattleSnapshot Previous = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({ Exhausted });
	const FBattleSnapshot Next = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({});

	HUD->StoreFirstPersonCardTransitionEventsForTest({
		WacomFirstPersonCardLayerSpec::MakeBattleEvent(EBattleEventType::CardExhausted, Exhausted.InstanceId)
	});
	const TArray<FWacomFirstPersonCardLayerTransitionHint> Hints =
		HUD->BuildFirstPersonCardTransitionHintsForTest(Previous, Next);

	const FWacomFirstPersonCardLayerTransitionHint* Hint =
		WacomFirstPersonCardLayerSpec::FindTransitionHint(Hints, Exhausted.InstanceId);
	TestNotNull(TEXT("CardExhausted hint exists"), Hint);
	if (Hint)
	{
		TestEqual(TEXT("CardExhausted maps to discarded transition"),
			Hint->TransitionKind, EWacomFirstPersonCardSlotTransitionKind::Discarded);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerHandLimitCompatibilityHintAssignmentTest,
	"Wacom.UI.FirstPersonCardLayer.EventAwareTransitions.HandLimitDiscardedCompatibilityDoesNotDuplicateExitHint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerHandLimitCompatibilityHintAssignmentTest::RunTest(const FString& Parameters)
{
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	if (!TestNotNull(TEXT("HUD"), HUD))
	{
		return false;
	}

	FHandCardSnapshot Discarded = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(nullptr, 1, true);
	const FBattleSnapshot Previous = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({ Discarded });
	const FBattleSnapshot Next = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({});

	HUD->StoreFirstPersonCardTransitionEventsForTest({
		WacomFirstPersonCardLayerSpec::MakeBattleEvent(EBattleEventType::HandLimitDiscarded, Discarded.InstanceId),
		WacomFirstPersonCardLayerSpec::MakeBattleEvent(EBattleEventType::CardDiscarded, Discarded.InstanceId)
	});
	const TArray<FWacomFirstPersonCardLayerTransitionHint> Hints =
		HUD->BuildFirstPersonCardTransitionHintsForTest(Previous, Next);

	int32 MatchingHints = 0;
	for (const FWacomFirstPersonCardLayerTransitionHint& Hint : Hints)
	{
		if (Hint.CardInstanceId != Discarded.InstanceId)
		{
			continue;
		}
		++MatchingHints;
		TestEqual(TEXT("Compatibility hint uses discarded transition"),
			Hint.TransitionKind, EWacomFirstPersonCardSlotTransitionKind::Discarded);
	}
	TestEqual(TEXT("Compatibility events produce one exit hint"), MatchingHints, 1);
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
	FWacomFirstPersonCardSlotVisualConfig VisualConfig;
	VisualConfig.PendingTargetingLiftPixels = 80.0f;
	VisualConfig.PendingTargetingScale = 1.15f;
	Layer->SetSlotVisualConfig(VisualConfig);
	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView BaseSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(100.0f, 300.0f));
	BaseSlot.bIsHovered = true;
	Layer->SetCardSlots({ BaseSlot });
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);

	WacomFirstPersonCardLayerSpec::FLayerLayoutUpdateReceiver Receiver;
	Layer->OnHoveredCardSlotUpdatedNative.AddRaw(
		&Receiver,
		&WacomFirstPersonCardLayerSpec::FLayerLayoutUpdateReceiver::HandleUpdated);
	FWacomFirstPersonCardLayerSlotView PendingSlot = BaseSlot;
	PendingSlot.Entry.bIsPendingTargeting = true;
	Layer->SetCardSlots({ PendingSlot });

	TestEqual(TEXT("Hovered layout update fired"), Receiver.UpdateCount, 1);
	TestTrue(TEXT("Detail follow uses animated visual position"),
		Receiver.LastSlotView.ScreenPosition.Y > BaseSlot.ScreenPosition.Y - VisualConfig.PendingTargetingLiftPixels);
	TestTrue(TEXT("Detail follow moves toward pending presentation"),
		Receiver.LastSlotView.ScreenPosition.Y < BaseSlot.ScreenPosition.Y);
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
	FWacomFirstPersonCardLayerEquivalentSlotRefreshSkippedTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotionRobustness.FirstPersonCardLayerSkipsFullRefreshForEquivalentSlots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerEquivalentSlotRefreshSkippedTest::RunTest(const FString& Parameters)
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

	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = {
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(FGuid::NewGuid(), 0, FVector2D(100.0f, 200.0f)),
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(FGuid::NewGuid(), 1, FVector2D(220.0f, 200.0f))
	};
	Layer->SetCardSlots(Slots);
	UWacomFirstPersonCardLayerSlotWidget* FirstWidget = Layer->GetSlotWidgetAt(0);
	const int32 InitialSkipCount = FWacomFirstPersonCardLayerTestAccess::View(*Layer).SkippedEquivalentSlotRefreshCount;

	Layer->SetCardSlots(Slots);
	const FWacomFirstPersonCardLayerMotionDebugView Debug = Layer->GetSlotMotionDebugView();
	TestEqual(TEXT("Equivalent refresh increments skip count"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).SkippedEquivalentSlotRefreshCount,
		InitialSkipCount + 1);
	TestEqual(TEXT("Equivalent refresh keeps active count"), Debug.ActiveSlotCount, 2);
	TestEqual(TEXT("Equivalent refresh creates no widgets"), Debug.CreatedThisUpdate, 0);
	TestEqual(TEXT("Equivalent refresh reuses no widgets through full reconcile"), Debug.ReusedThisUpdate, 0);
	TestEqual(TEXT("Equivalent refresh keeps first widget"), Layer->GetSlotWidgetAt(0), FirstWidget);
	TestFalse(TEXT("Equivalent refresh reports no invariant violation"), Debug.bHadInvariantViolation);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDirtyGateMovedSlotRefreshesTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotionRobustness.FirstPersonCardLayerDirtyGateRefreshesMovedSlots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDirtyGateMovedSlotRefreshesTest::RunTest(const FString& Parameters)
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
	const FWacomFirstPersonCardLayerSlotView BaseSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(100.0f, 200.0f));
	Layer->SetCardSlots({ BaseSlot });
	const int32 InitialSkipCount = FWacomFirstPersonCardLayerTestAccess::View(*Layer).SkippedEquivalentSlotRefreshCount;

	FWacomFirstPersonCardLayerSlotView MovedSlot = BaseSlot;
	MovedSlot.ScreenPosition = FVector2D(180.0f, 240.0f);
	MovedSlot.WidgetPosition = MovedSlot.ScreenPosition;
	MovedSlot.SnappedWidgetPosition = MovedSlot.ScreenPosition;
	MovedSlot.RenderAngleDegrees = 5.0f;
	MovedSlot.RenderScale = 1.1f;
	Layer->SetCardSlots({ MovedSlot });

	const FWacomFirstPersonCardLayerMotionDebugView Debug = Layer->GetSlotMotionDebugView();
	TestEqual(TEXT("Moved refresh does not skip"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).SkippedEquivalentSlotRefreshCount,
		InitialSkipCount);
	TestEqual(TEXT("Moved refresh reuses widget"), Debug.ReusedThisUpdate, 1);
	TestEqual(TEXT("Moved refresh keeps active count"), Debug.ActiveSlotCount, 1);
	TestTrue(TEXT("Moved refresh updates slot target"),
		Layer->GetSlotWidgetAt(0)->GetSlotView().ScreenPosition.Equals(MovedSlot.ScreenPosition, 0.01f));

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDirtyGateChangedCardDataRefreshesTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotionRobustness.FirstPersonCardLayerDirtyGateRefreshesChangedCardData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDirtyGateChangedCardDataRefreshesTest::RunTest(const FString& Parameters)
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
	FWacomFirstPersonCardLayerSlotView BaseSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(100.0f, 200.0f));
	BaseSlot.Entry.CardViewData.Name = FText::FromString(TEXT("Before"));
	BaseSlot.Entry.CardViewData.Cost = 1;
	Layer->SetCardSlots({ BaseSlot });
	const int32 InitialSkipCount = FWacomFirstPersonCardLayerTestAccess::View(*Layer).SkippedEquivalentSlotRefreshCount;

	FWacomFirstPersonCardLayerSlotView ChangedSlot = BaseSlot;
	ChangedSlot.Entry.CardViewData.Name = FText::FromString(TEXT("After"));
	ChangedSlot.Entry.CardViewData.Cost = 2;
	Layer->SetCardSlots({ ChangedSlot });

	const FWacomFirstPersonCardLayerMotionDebugView Debug = Layer->GetSlotMotionDebugView();
	TestEqual(TEXT("Card data refresh does not skip"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).SkippedEquivalentSlotRefreshCount,
		InitialSkipCount);
	TestEqual(TEXT("Card data refresh reuses widget"), Debug.ReusedThisUpdate, 1);
	TestEqual(TEXT("Card data refresh updates slot data"),
		Layer->GetSlotWidgetAt(0)->GetSlotView().Entry.CardViewData.Cost,
		2);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDirtyGateTransitionHintsRefreshTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotionRobustness.FirstPersonCardLayerDirtyGateDoesNotSwallowTransitionHints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDirtyGateTransitionHintsRefreshTest::RunTest(const FString& Parameters)
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
	Config.EnterOpacity = 0.0f;
	Config.DrawnEnterOffsetPixels = FVector2D(0.0f, 96.0f);
	Layer->SetSlotMotionConfig(Config);

	const FGuid CardId = FGuid::NewGuid();
	const FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(100.0f, 200.0f));
	Layer->SetCardSlots({ Slot });
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
	const int32 InitialSkipCount = FWacomFirstPersonCardLayerTestAccess::View(*Layer).SkippedEquivalentSlotRefreshCount;

	Layer->SetCardTransitionHints({
		WacomFirstPersonCardLayerSpec::MakeTransitionHint(CardId, EWacomFirstPersonCardSlotTransitionKind::Drawn)
	});
	Layer->SetCardSlots({ Slot });

	TestEqual(TEXT("Transition hint refresh does not skip"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).SkippedEquivalentSlotRefreshCount,
		InitialSkipCount);
	TestEqual(TEXT("Transition hint refresh reuses widget"), Layer->GetSlotMotionDebugView().ReusedThisUpdate, 1);
	TestEqual(TEXT("Transition hint refresh keeps active count"), Layer->GetSlotMotionDebugView().ActiveSlotCount, 1);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDirtyGateHoveredMotionUpdatesTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotionRobustness.FirstPersonCardLayerDirtyGateKeepsHoveredMotionLayoutUpdates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDirtyGateHoveredMotionUpdatesTest::RunTest(const FString& Parameters)
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
	Config.MotionSpeed = 4.0f;
	Layer->SetSlotMotionConfig(Config);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid CardId = FGuid::NewGuid();
	const FWacomFirstPersonCardLayerSlotView BaseSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(100.0f, 200.0f));
	Layer->SetCardSlots({ BaseSlot });
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("SlotWidget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	int32 HoveredLayoutUpdateCount = 0;
	FVector2D LastHoveredLayoutPosition = FVector2D::ZeroVector;
	Layer->OnHoveredCardSlotUpdatedNative.AddLambda(
		[&HoveredLayoutUpdateCount, &LastHoveredLayoutPosition](
			const FGuid&,
			const FWacomFirstPersonCardLayerSlotView& SlotView)
		{
			++HoveredLayoutUpdateCount;
			LastHoveredLayoutPosition = SlotView.ScreenPosition;
		});
	TestTrue(TEXT("Slot hover succeeds"), FWacomFirstPersonCardLayerTestAccess::RequestHover(*SlotWidget));

	FWacomFirstPersonCardLayerSlotView MovedSlot = BaseSlot;
	MovedSlot.ScreenPosition = FVector2D(220.0f, 260.0f);
	MovedSlot.WidgetPosition = MovedSlot.ScreenPosition;
	MovedSlot.SnappedWidgetPosition = MovedSlot.ScreenPosition;
	Layer->SetCardSlots({ MovedSlot });
	const int32 InitialSkipCount = FWacomFirstPersonCardLayerTestAccess::View(*Layer).SkippedEquivalentSlotRefreshCount;
	const int32 InitialHoveredLayoutUpdateCount = HoveredLayoutUpdateCount;

	Layer->SetCardSlots({ MovedSlot });
	TestEqual(TEXT("Equivalent moved target refresh skips full reconcile"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).SkippedEquivalentSlotRefreshCount,
		InitialSkipCount + 1);

	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.1f);
	TestTrue(TEXT("Hovered motion tick broadcasts layout update"),
		HoveredLayoutUpdateCount > InitialHoveredLayoutUpdateCount);
	TestTrue(TEXT("Hovered layout update follows motion toward target"),
		FVector2D::Distance(LastHoveredLayoutPosition, BaseSlot.ScreenPosition)
		< FVector2D::Distance(MovedSlot.ScreenPosition, BaseSlot.ScreenPosition));

	Layer->OnHoveredCardSlotUpdatedNative.Clear();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerConfigSettersSkipEquivalentPropagationTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotionRobustness.FirstPersonCardLayerConfigSettersSkipEquivalentPropagation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerConfigSettersSkipEquivalentPropagationTest::RunTest(const FString& Parameters)
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
	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(100.0f, 200.0f))
	});
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("SlotWidget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	FWacomFirstPersonCardSlotMotionConfig MotionConfig = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	MotionConfig.MotionSpeed = 8.0f;
	MotionConfig.OpacitySpeed = -9.0f;
	MotionConfig.EasePower = -2.0f;
	MotionConfig.HoverMotionProfile.MotionSpeed = -4.0f;
	MotionConfig.HoverMotionProfile.OpacitySpeed = -5.0f;
	MotionConfig.HoverMotionProfile.EasePower = -6.0f;
	MotionConfig.EnterOpacity = 4.0f;
	MotionConfig.DrawnEnterViewportAnchor = FVector2D(-1.0f, 2.0f);
	MotionConfig.DrawnEnterScaleMultiplier = -3.0f;
	Layer->SetSlotMotionConfig(MotionConfig);
	const int32 MotionPropagationCount = FWacomFirstPersonCardLayerTestAccess::View(*Layer).SlotMotionConfigPropagationCount;
	const int32 SlotMotionApplyCount = FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).SlotMotionConfigApplyCount;
	FWacomFirstPersonCardSlotMotionConfig NormalizedEquivalentMotionConfig = MotionConfig;
	NormalizedEquivalentMotionConfig.OpacitySpeed = 0.0f;
	NormalizedEquivalentMotionConfig.EasePower = 0.1f;
	NormalizedEquivalentMotionConfig.HoverMotionProfile.MotionSpeed = 0.0f;
	NormalizedEquivalentMotionConfig.HoverMotionProfile.OpacitySpeed = 0.0f;
	NormalizedEquivalentMotionConfig.HoverMotionProfile.EasePower = 0.1f;
	NormalizedEquivalentMotionConfig.EnterOpacity = 1.0f;
	NormalizedEquivalentMotionConfig.DrawnEnterViewportAnchor = FVector2D(0.0f, 1.0f);
	NormalizedEquivalentMotionConfig.DrawnEnterScaleMultiplier = 0.01f;
	Layer->SetSlotMotionConfig(NormalizedEquivalentMotionConfig);
	TestEqual(TEXT("Normalized-equivalent motion config skipped at layer"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).SlotMotionConfigPropagationCount,
		MotionPropagationCount);
	TestEqual(TEXT("Normalized-equivalent motion config skipped at slot"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).SlotMotionConfigApplyCount,
		SlotMotionApplyCount);

	FWacomFirstPersonCardSlotVisualConfig VisualConfig;
	VisualConfig.HoverLiftPixels = -2.0f;
	VisualConfig.HoverScale = -1.0f;
	VisualConfig.HoverZOrderBoost = -5;
	VisualConfig.PendingTargetingLiftPixels = -4.0f;
	VisualConfig.PendingTargetingScale = -3.0f;
	VisualConfig.PendingTargetingZOrderBoost = -8;
	VisualConfig.PendingTargetingAngleBlend = 3.0f;
	VisualConfig.TargetSelectNonPendingOpacityMultiplier = -2.0f;
	VisualConfig.DragCardTargetFocusLiftPixels = -8.0f;
	VisualConfig.DragCardTargetFocusScale = -2.0f;
	VisualConfig.DragCardTargetFocusZOrderBoost = -12;
	Layer->SetSlotVisualConfig(VisualConfig);
	const int32 VisualPropagationCount = FWacomFirstPersonCardLayerTestAccess::View(*Layer).SlotVisualConfigPropagationCount;
	const int32 SlotVisualApplyCount = FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).SlotVisualConfigApplyCount;
	FWacomFirstPersonCardSlotVisualConfig NormalizedEquivalentVisualConfig = VisualConfig;
	NormalizedEquivalentVisualConfig.HoverLiftPixels = 0.0f;
	NormalizedEquivalentVisualConfig.HoverScale = 0.01f;
	NormalizedEquivalentVisualConfig.HoverZOrderBoost = 0;
	NormalizedEquivalentVisualConfig.PendingTargetingLiftPixels = 0.0f;
	NormalizedEquivalentVisualConfig.PendingTargetingScale = 0.01f;
	NormalizedEquivalentVisualConfig.PendingTargetingZOrderBoost = 0;
	NormalizedEquivalentVisualConfig.PendingTargetingAngleBlend = 1.0f;
	NormalizedEquivalentVisualConfig.TargetSelectNonPendingOpacityMultiplier = 0.0f;
	NormalizedEquivalentVisualConfig.DragCardTargetFocusLiftPixels = 0.0f;
	NormalizedEquivalentVisualConfig.DragCardTargetFocusScale = 0.01f;
	NormalizedEquivalentVisualConfig.DragCardTargetFocusZOrderBoost = 0;
	Layer->SetSlotVisualConfig(NormalizedEquivalentVisualConfig);
	TestEqual(TEXT("Normalized-equivalent visual config skipped at layer"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).SlotVisualConfigPropagationCount,
		VisualPropagationCount);
	TestEqual(TEXT("Normalized-equivalent visual config skipped at slot"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).SlotVisualConfigApplyCount,
		SlotVisualApplyCount);

	FWacomFirstPersonCardSlotFeedbackConfig FeedbackConfig = WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig();
	FeedbackConfig.PlayableHoverOpacity = 2.0f;
	FeedbackConfig.PressedScale = -4.0f;
	FeedbackConfig.PressedOpacity = -1.0f;
	FeedbackConfig.DenyShakePixels = -8.0f;
	FeedbackConfig.InteractionFeedbackEdgeWidth = -0.1f;
	FeedbackConfig.InteractionFeedbackEdgeSoftness = -0.2f;
	FeedbackConfig.InteractionFeedbackVignetteStrength = -0.3f;
	FeedbackConfig.InteractionFeedbackVignetteRadius = -0.4f;
	FeedbackConfig.InteractionFeedbackVignetteSoftness = -0.5f;
	Layer->SetSlotFeedbackConfig(FeedbackConfig);
	const int32 FeedbackPropagationCount = FWacomFirstPersonCardLayerTestAccess::View(*Layer).SlotFeedbackConfigPropagationCount;
	const int32 SlotFeedbackApplyCount = FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).SlotFeedbackConfigApplyCount;
	FWacomFirstPersonCardSlotFeedbackConfig NormalizedEquivalentFeedbackConfig = FeedbackConfig;
	NormalizedEquivalentFeedbackConfig.PlayableHoverOpacity = 1.0f;
	NormalizedEquivalentFeedbackConfig.PressedScale = 0.01f;
	NormalizedEquivalentFeedbackConfig.PressedOpacity = 0.0f;
	NormalizedEquivalentFeedbackConfig.DenyShakePixels = 0.0f;
	NormalizedEquivalentFeedbackConfig.InteractionFeedbackEdgeWidth = 0.0f;
	NormalizedEquivalentFeedbackConfig.InteractionFeedbackEdgeSoftness = 0.0f;
	NormalizedEquivalentFeedbackConfig.InteractionFeedbackVignetteStrength = 0.0f;
	NormalizedEquivalentFeedbackConfig.InteractionFeedbackVignetteRadius = 0.0f;
	NormalizedEquivalentFeedbackConfig.InteractionFeedbackVignetteSoftness = 0.0f;
	Layer->SetSlotFeedbackConfig(NormalizedEquivalentFeedbackConfig);
	TestEqual(TEXT("Normalized-equivalent feedback config skipped at layer"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).SlotFeedbackConfigPropagationCount,
		FeedbackPropagationCount);
	TestEqual(TEXT("Normalized-equivalent feedback config skipped at slot"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).SlotFeedbackConfigApplyCount,
		SlotFeedbackApplyCount);

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardDragStartThresholdPixels = -18.0f;
	DragConfig.CardInspectScreenPosition = FVector2D(-2.0f, 4.0f);
	DragConfig.CardInspectScale = -1.0f;
	DragConfig.DragTargetFeedbackOpacity = 3.0f;
	DragConfig.DragCardTargetFocusLiftPixels = -8.0f;
	DragConfig.DragCardTargetFocusScale = -2.0f;
	DragConfig.DragCardTargetFocusZOrderBoost = -12;
	DragConfig.SelectedSourceZOrderBoost = -10;
	Layer->SetCardDragConfig(DragConfig);
	const int32 DragPropagationCount = FWacomFirstPersonCardLayerTestAccess::View(*Layer).CardDragConfigPropagationCount;
	const int32 SlotDragApplyCount = FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).CardDragConfigApplyCount;
	FWacomFirstPersonCardDragConfig NormalizedEquivalentDragConfig = DragConfig;
	NormalizedEquivalentDragConfig.CardDragStartThresholdPixels = 0.0f;
	NormalizedEquivalentDragConfig.CardInspectScreenPosition = FVector2D(0.0f, 1.0f);
	NormalizedEquivalentDragConfig.CardInspectScale = 0.01f;
	NormalizedEquivalentDragConfig.DragTargetFeedbackOpacity = 1.0f;
	NormalizedEquivalentDragConfig.DragCardTargetFocusLiftPixels = 0.0f;
	NormalizedEquivalentDragConfig.DragCardTargetFocusScale = 0.01f;
	NormalizedEquivalentDragConfig.DragCardTargetFocusZOrderBoost = 0;
	NormalizedEquivalentDragConfig.SelectedSourceZOrderBoost = 0;
	Layer->SetCardDragConfig(NormalizedEquivalentDragConfig);
	TestEqual(TEXT("Normalized-equivalent drag config skipped at layer"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CardDragConfigPropagationCount,
		DragPropagationCount);
	TestEqual(TEXT("Normalized-equivalent drag config skipped at slot"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).CardDragConfigApplyCount,
		SlotDragApplyCount);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerSharedConfigNormalizationTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotionRobustness.FirstPersonCardLayerAndSlotUseSharedConfigNormalization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerSharedConfigNormalizationTest::RunTest(const FString& Parameters)
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
	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(100.0f, 200.0f))
	});
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("SlotWidget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	FWacomFirstPersonCardSlotMotionConfig MotionConfig = WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	MotionConfig.MotionSpeed = -4.0f;
	MotionConfig.EasePower = -2.0f;
	MotionConfig.PendingMotionProfile.MotionSpeed = -2.0f;
	MotionConfig.PendingMotionProfile.OpacitySpeed = -3.0f;
	MotionConfig.PendingMotionProfile.EasePower = -4.0f;
	MotionConfig.EnterOpacity = 2.0f;
	MotionConfig.GainedEnterViewportAnchor = FVector2D(3.0f, -2.0f);
	MotionConfig.GainedEnterScaleMultiplier = -5.0f;
	SlotWidget->SetSlotMotionConfig(MotionConfig);
	const int32 SlotMotionApplyCount = FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).SlotMotionConfigApplyCount;
	FWacomFirstPersonCardSlotMotionConfig NormalizedMotionConfig = MotionConfig;
	NormalizedMotionConfig.MotionSpeed = 0.0f;
	NormalizedMotionConfig.EasePower = 0.1f;
	NormalizedMotionConfig.PendingMotionProfile.MotionSpeed = 0.0f;
	NormalizedMotionConfig.PendingMotionProfile.OpacitySpeed = 0.0f;
	NormalizedMotionConfig.PendingMotionProfile.EasePower = 0.1f;
	NormalizedMotionConfig.EnterOpacity = 1.0f;
	NormalizedMotionConfig.GainedEnterViewportAnchor = FVector2D(1.0f, 0.0f);
	NormalizedMotionConfig.GainedEnterScaleMultiplier = 0.01f;
	Layer->SetSlotMotionConfig(NormalizedMotionConfig);
	TestEqual(TEXT("Layer-normalized motion matches slot-normalized motion"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).SlotMotionConfigApplyCount,
		SlotMotionApplyCount);

	FWacomFirstPersonCardSlotVisualConfig VisualConfig;
	VisualConfig.HoverScale = -2.0f;
	VisualConfig.PendingTargetingAngleBlend = -0.5f;
	VisualConfig.TargetSelectNonPendingOpacityMultiplier = 4.0f;
	VisualConfig.DragCardTargetFocusZOrderBoost = -12;
	SlotWidget->SetSlotVisualConfig(VisualConfig);
	const int32 SlotVisualApplyCount = FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).SlotVisualConfigApplyCount;
	FWacomFirstPersonCardSlotVisualConfig NormalizedVisualConfig = VisualConfig;
	NormalizedVisualConfig.HoverScale = 0.01f;
	NormalizedVisualConfig.PendingTargetingAngleBlend = 0.0f;
	NormalizedVisualConfig.TargetSelectNonPendingOpacityMultiplier = 1.0f;
	NormalizedVisualConfig.DragCardTargetFocusZOrderBoost = 0;
	Layer->SetSlotVisualConfig(NormalizedVisualConfig);
	TestEqual(TEXT("Layer-normalized visual matches slot-normalized visual"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).SlotVisualConfigApplyCount,
		SlotVisualApplyCount);

	FWacomFirstPersonCardSlotFeedbackConfig FeedbackConfig = WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig();
	FeedbackConfig.ConfirmDuration = -1.0f;
	FeedbackConfig.ConfirmOpacity = 2.0f;
	FeedbackConfig.PlayCommitScale = -4.0f;
	FeedbackConfig.InteractionFeedbackEdgeWidth = -0.1f;
	FeedbackConfig.InteractionFeedbackEdgeSoftness = -0.2f;
	FeedbackConfig.InteractionFeedbackVignetteStrength = -0.3f;
	FeedbackConfig.InteractionFeedbackVignetteRadius = -0.4f;
	FeedbackConfig.InteractionFeedbackVignetteSoftness = -0.5f;
	SlotWidget->SetSlotFeedbackConfig(FeedbackConfig);
	const int32 SlotFeedbackApplyCount = FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).SlotFeedbackConfigApplyCount;
	FWacomFirstPersonCardSlotFeedbackConfig NormalizedFeedbackConfig = FeedbackConfig;
	NormalizedFeedbackConfig.ConfirmDuration = 0.0f;
	NormalizedFeedbackConfig.ConfirmOpacity = 1.0f;
	NormalizedFeedbackConfig.PlayCommitScale = 0.01f;
	NormalizedFeedbackConfig.InteractionFeedbackEdgeWidth = 0.0f;
	NormalizedFeedbackConfig.InteractionFeedbackEdgeSoftness = 0.0f;
	NormalizedFeedbackConfig.InteractionFeedbackVignetteStrength = 0.0f;
	NormalizedFeedbackConfig.InteractionFeedbackVignetteRadius = 0.0f;
	NormalizedFeedbackConfig.InteractionFeedbackVignetteSoftness = 0.0f;
	Layer->SetSlotFeedbackConfig(NormalizedFeedbackConfig);
	TestEqual(TEXT("Layer-normalized feedback matches slot-normalized feedback"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).SlotFeedbackConfigApplyCount,
		SlotFeedbackApplyCount);

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardInspectHoldDelaySeconds = -1.0f;
	DragConfig.CardDragCameraLookScale = -2.0f;
	DragConfig.DragAimArrowSnapBlend = 3.0f;
	DragConfig.DragCardTargetFocusLiftPixels = -8.0f;
	DragConfig.DragCardTargetFocusScale = -2.0f;
	DragConfig.DragCardTargetFocusZOrderBoost = -12;
	DragConfig.SelectedSourceScale = -4.0f;
	DragConfig.SelectedSourceAngleBlend = -0.5f;
	SlotWidget->SetCardDragConfig(DragConfig);
	const int32 SlotDragApplyCount = FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).CardDragConfigApplyCount;
	FWacomFirstPersonCardDragConfig NormalizedDragConfig = DragConfig;
	NormalizedDragConfig.CardInspectHoldDelaySeconds = 0.0f;
	NormalizedDragConfig.CardDragCameraLookScale = 0.0f;
	NormalizedDragConfig.DragAimArrowSnapBlend = 1.0f;
	NormalizedDragConfig.DragCardTargetFocusLiftPixels = 0.0f;
	NormalizedDragConfig.DragCardTargetFocusScale = 0.01f;
	NormalizedDragConfig.DragCardTargetFocusZOrderBoost = 0;
	NormalizedDragConfig.SelectedSourceScale = 0.01f;
	NormalizedDragConfig.SelectedSourceAngleBlend = 0.0f;
	Layer->SetCardDragConfig(NormalizedDragConfig);
	TestEqual(TEXT("Layer-normalized drag matches slot-normalized drag"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).CardDragConfigApplyCount,
		SlotDragApplyCount);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardAnchorMotionProfileOverridesTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotion.AnchorMotionProfileOverridesMapToLayerConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardAnchorMotionProfileOverridesTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character =
		World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Character"), Character))
	{
		return false;
	}
	PC->Possess(Character);

	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor =
		NewObject<UWacomFirstPersonCardAnchorSpecProbeComponent>(Character);
	if (!TestNotNull(TEXT("Anchor"), Anchor))
	{
		PC->Destroy();
		Character->Destroy();
		return false;
	}
	Anchor->RegisterComponent();
	Anchor->bDrawPreviewCardLayer = true;
	Anchor->PreviewCardCountFallback = 1;
	Anchor->SetBattleHandInteractionEnabled(true);
	Anchor->CardSlotMotionSpeed = 11.0f;
	Anchor->CardSlotOpacitySpeed = 12.0f;
	Anchor->CardSlotMotionEasePower = 1.3f;
	Anchor->RefreshAnchor(1.0f / 60.0f);
	Anchor->RefreshCardLayerForTest();

	UWacomFirstPersonCardLayerWidget* Layer = FWacomFirstPersonCardLayerTestAccess::CardLayer(*Anchor);
	if (!TestNotNull(TEXT("Layer"), Layer))
	{
		Anchor->DestroyComponent();
		PC->Destroy();
		Character->Destroy();
		return false;
	}

	const auto ExpectProfile = [this](
		const TCHAR* Label,
		const FWacomFirstPersonCardMotionProfile& Profile,
		float MotionSpeed,
		float OpacitySpeed,
		float EasePower)
	{
		TestTrue(FString::Printf(TEXT("%s motion speed"), Label),
			FMath::IsNearlyEqual(Profile.MotionSpeed, MotionSpeed, 0.001f));
		TestTrue(FString::Printf(TEXT("%s opacity speed"), Label),
			FMath::IsNearlyEqual(Profile.OpacitySpeed, OpacitySpeed, 0.001f));
		TestTrue(FString::Printf(TEXT("%s ease power"), Label),
			FMath::IsNearlyEqual(Profile.EasePower, EasePower, 0.001f));
	};

	FWacomFirstPersonCardSlotMotionConfig MotionConfig =
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).SlotMotionConfig;
	ExpectProfile(TEXT("Layout inherits global"), MotionConfig.LayoutMotionProfile, 11.0f, 12.0f, 1.3f);
	ExpectProfile(TEXT("Hover inherits global"), MotionConfig.HoverMotionProfile, 11.0f, 12.0f, 1.3f);
	ExpectProfile(TEXT("Pending inherits global"), MotionConfig.PendingMotionProfile, 11.0f, 12.0f, 1.3f);
	ExpectProfile(TEXT("Drag focus inherits global"), MotionConfig.DragTargetFocusMotionProfile, 11.0f, 12.0f, 1.3f);
	ExpectProfile(TEXT("Enter inherits global"), MotionConfig.EnterMotionProfile, 11.0f, 12.0f, 1.3f);
	ExpectProfile(TEXT("Exit inherits global"), MotionConfig.ExitMotionProfile, 11.0f, 12.0f, 1.3f);

	const int32 ApplyCountBeforeOverrides =
		FWacomFirstPersonCardLayerTestAccess::View(*Anchor).CardLayerConfigApplyCount;
	Anchor->bOverrideHoverMotionProfile = true;
	Anchor->HoverMotionSpeed = 21.0f;
	Anchor->HoverOpacitySpeed = 22.0f;
	Anchor->HoverMotionEasePower = 1.4f;
	Anchor->bOverrideDragTargetFocusMotionProfile = true;
	Anchor->DragTargetFocusMotionSpeed = 31.0f;
	Anchor->DragTargetFocusOpacitySpeed = 32.0f;
	Anchor->DragTargetFocusMotionEasePower = 1.5f;
	Anchor->bOverrideEnterExitMotionProfile = true;
	Anchor->EnterMotionSpeed = 41.0f;
	Anchor->EnterOpacitySpeed = 42.0f;
	Anchor->EnterMotionEasePower = 1.6f;
	Anchor->ExitMotionSpeed = 51.0f;
	Anchor->ExitOpacitySpeed = 52.0f;
	Anchor->ExitMotionEasePower = 1.7f;
	Anchor->RefreshAnchor(1.0f / 60.0f);
	Anchor->RefreshCardLayerForTest();

	TestEqual(TEXT("Motion profile override changes reapply layer config"),
		FWacomFirstPersonCardLayerTestAccess::View(*Anchor).CardLayerConfigApplyCount,
		ApplyCountBeforeOverrides + 1);

	MotionConfig = FWacomFirstPersonCardLayerTestAccess::View(*Layer).SlotMotionConfig;
	ExpectProfile(TEXT("Layout still inherits global"), MotionConfig.LayoutMotionProfile, 11.0f, 12.0f, 1.3f);
	ExpectProfile(TEXT("Pending still inherits global"), MotionConfig.PendingMotionProfile, 11.0f, 12.0f, 1.3f);
	ExpectProfile(TEXT("Hover uses override"), MotionConfig.HoverMotionProfile, 21.0f, 22.0f, 1.4f);
	ExpectProfile(TEXT("Drag focus uses override"), MotionConfig.DragTargetFocusMotionProfile, 31.0f, 32.0f, 1.5f);
	ExpectProfile(TEXT("Enter uses override"), MotionConfig.EnterMotionProfile, 41.0f, 42.0f, 1.6f);
	ExpectProfile(TEXT("Exit uses override"), MotionConfig.ExitMotionProfile, 51.0f, 52.0f, 1.7f);

	Anchor->DestroyComponent();
	PC->Destroy();
	Character->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardAnchorProgrammaticDragGestureTest,
	"Wacom.UI.FirstPersonCardLayer.Interaction.AnchorProgrammaticDragGestureStartsSelectedCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardAnchorProgrammaticDragGestureTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character =
		World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Character"), Character))
	{
		return false;
	}
	PC->Possess(Character);

	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor =
		NewObject<UWacomFirstPersonCardAnchorSpecProbeComponent>(Character);
	if (!TestNotNull(TEXT("Anchor"), Anchor))
	{
		PC->Destroy();
		Character->Destroy();
		return false;
	}
	Anchor->RegisterComponent();
	Anchor->SetBattleHandInteractionEnabled(true);

	const FGuid NoTargetCardId = FGuid::NewGuid();
	const FGuid TargetedCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerEntry NoTargetEntry;
	NoTargetEntry.CardInstanceId = NoTargetCardId;
	NoTargetEntry.CardViewData.Name = FText::FromString(TEXT("No Target"));
	NoTargetEntry.bIsPlayable = true;
	NoTargetEntry.TargetMode = ECardTargetMode::None;
	FWacomFirstPersonCardLayerEntry TargetedEntry;
	TargetedEntry.CardInstanceId = TargetedCardId;
	TargetedEntry.CardViewData.Name = FText::FromString(TEXT("Targeted"));
	TargetedEntry.bIsPlayable = true;
	TargetedEntry.TargetMode = ECardTargetMode::SingleEnemyPart;
	Anchor->SetRuntimeCardLayerEntries(TEXT("BattleHand"), { NoTargetEntry, TargetedEntry });
	Anchor->RefreshAnchor(1.0f / 60.0f);
	Anchor->RefreshCardLayerForTest();

	UWacomFirstPersonCardLayerWidget* Layer = FWacomFirstPersonCardLayerTestAccess::CardLayer(*Anchor);
	if (!TestNotNull(TEXT("Layer"), Layer))
	{
		Anchor->DestroyComponent();
		PC->Destroy();
		Character->Destroy();
		return false;
	}

	TestTrue(TEXT("No-target card programmatic drag starts"),
		Anchor->TryStartFirstPersonCardDragGesture(NoTargetCardId));
	UWacomFirstPersonCardLayerSlotWidget* NoTargetSlot =
		FWacomFirstPersonCardLayerTestAccess::FindSlotWidgetByKey(*Layer, NoTargetCardId.ToString(EGuidFormats::DigitsWithHyphensLower));
	if (TestNotNull(TEXT("No-target slot"), NoTargetSlot))
	{
		TestEqual(TEXT("No-target card enters drag state"),
			NoTargetSlot->GetGestureStateForFirstPersonLayer(),
			EWacomFirstPersonCardGestureState::DraggingNoTargetCard);
	}

	TestTrue(TEXT("Targeted card programmatic drag starts"),
		Anchor->TryStartFirstPersonCardDragGesture(TargetedCardId));
	UWacomFirstPersonCardLayerSlotWidget* TargetedSlot =
		FWacomFirstPersonCardLayerTestAccess::FindSlotWidgetByKey(*Layer, TargetedCardId.ToString(EGuidFormats::DigitsWithHyphensLower));
	if (TestNotNull(TEXT("Targeted slot"), TargetedSlot))
	{
		TestEqual(TEXT("Targeted card enters aim state"),
			TargetedSlot->GetGestureStateForFirstPersonLayer(),
			EWacomFirstPersonCardGestureState::AimingTargetedCard);
	}
	const FVector2D PumpedPointerPosition(760.0f, 420.0f);
	TestTrue(TEXT("Anchor reports active drag"),
		Anchor->IsFirstPersonCardDragGestureActive());
	TestTrue(TEXT("Anchor pointer pump forwards to layer"),
		Anchor->UpdateFirstPersonCardDragPointer(PumpedPointerPosition));
	const FWacomFirstPersonCardLayerAutomationTestView LayerViewAfterPump =
		FWacomFirstPersonCardLayerTestAccess::View(*Layer);
	TestEqual(TEXT("Anchor pointer pump updates layer drag pointer"),
		LayerViewAfterPump.CurrentDragView.CurrentScreenPosition,
		PumpedPointerPosition);

	TestFalse(TEXT("Invalid card id cannot start drag"),
		Anchor->TryStartFirstPersonCardDragGesture(FGuid::NewGuid()));
	if (TargetedSlot)
	{
		TestEqual(TEXT("Missing card id does not cancel current drag"),
			TargetedSlot->GetGestureStateForFirstPersonLayer(),
			EWacomFirstPersonCardGestureState::AimingTargetedCard);
	}

	WacomFirstPersonCardLayerSpec::FLayerDragReceiver ReleaseReceiver;
	Anchor->OnFirstPersonCardLayerDragReleased.AddRaw(
		&ReleaseReceiver,
		&WacomFirstPersonCardLayerSpec::FLayerDragReceiver::HandleReleased);
	const FVector2D ReleasePointerPosition(800.0f, 380.0f);
	TestTrue(TEXT("Anchor release forwards to layer"),
		Anchor->ReleaseFirstPersonCardDragGesture(ReleasePointerPosition));
	TestFalse(TEXT("Anchor reports inactive drag after release"),
		Anchor->IsFirstPersonCardDragGestureActive());
	TestEqual(TEXT("Anchor release broadcasts released drag"),
		ReleaseReceiver.ReleasedCount,
		1);
	TestEqual(TEXT("Anchor release uses release pointer"),
		ReleaseReceiver.LastDragView.CurrentScreenPosition,
		ReleasePointerPosition);
	Anchor->OnFirstPersonCardLayerDragReleased.RemoveAll(&ReleaseReceiver);

	Anchor->DestroyComponent();
	PC->Destroy();
	Character->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardAnchorCardLayerConfigDirtyGateTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotionRobustness.FirstPersonCardAnchorSkipsCardLayerConfigRefreshWhenResolvedConfigUnchanged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardAnchorCardLayerConfigDirtyGateTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	AWacomPlayerCharacter* Character =
		World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	if (!TestNotNull(TEXT("PlayerController"), PC) || !TestNotNull(TEXT("Character"), Character))
	{
		return false;
	}
	PC->Possess(Character);

	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor =
		NewObject<UWacomFirstPersonCardAnchorSpecProbeComponent>(Character);
	if (!TestNotNull(TEXT("Anchor"), Anchor))
	{
		PC->Destroy();
		Character->Destroy();
		return false;
	}
	Anchor->RegisterComponent();
	Anchor->bDrawPreviewCardLayer = true;
	Anchor->PreviewCardCountFallback = 2;
	Anchor->SetBattleHandInteractionEnabled(true);
	Anchor->RefreshAnchor(1.0f / 60.0f);
	Anchor->RefreshCardLayerForTest();

	UWacomFirstPersonCardLayerWidget* Layer = FWacomFirstPersonCardLayerTestAccess::CardLayer(*Anchor);
	if (!TestNotNull(TEXT("Layer"), Layer))
	{
		Anchor->DestroyComponent();
		PC->Destroy();
		Character->Destroy();
		return false;
	}

	const int32 InitialApplyCount = FWacomFirstPersonCardLayerTestAccess::View(*Anchor).CardLayerConfigApplyCount;
	const int32 InitialSlotSkipCount = FWacomFirstPersonCardLayerTestAccess::View(*Layer).SkippedEquivalentSlotRefreshCount;
	Anchor->RefreshCardLayerForTest();
	TestEqual(TEXT("Equivalent anchor refresh does not reapply config"),
		FWacomFirstPersonCardLayerTestAccess::View(*Anchor).CardLayerConfigApplyCount,
		InitialApplyCount);
	TestEqual(TEXT("Equivalent anchor refresh can skip layer slot refresh"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).SkippedEquivalentSlotRefreshCount,
		InitialSlotSkipCount + 1);

	Anchor->AuthoredCardSpacingPixels += 12.0f;
	Anchor->RefreshAnchor(1.0f / 60.0f);
	Anchor->RefreshCardLayerForTest();
	TestEqual(TEXT("Changed resolved config reapplies once"),
		FWacomFirstPersonCardLayerTestAccess::View(*Anchor).CardLayerConfigApplyCount,
		InitialApplyCount + 1);

	Anchor->CardSlotMotionEasePower += 1.0f;
	Anchor->RefreshAnchor(1.0f / 60.0f);
	Anchor->RefreshCardLayerForTest();
	TestEqual(TEXT("Changed motion ease reapplies config"),
		FWacomFirstPersonCardLayerTestAccess::View(*Anchor).CardLayerConfigApplyCount,
		InitialApplyCount + 2);

	Anchor->DestroyComponent();
	PC->Destroy();
	Character->Destroy();
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
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SecondId, 0, FVector2D(100.0f, 200.0f))
	});
	TestEqual(TEXT("One active after remove"), Layer->GetSlotMotionDebugView().ActiveSlotCount, 1);
	TestEqual(TEXT("One outgoing after remove"), Layer->GetSlotMotionDebugView().OutgoingSlotCount, 1);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.25f);
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
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
	UWacomFirstPersonCardLayerSlotWidget* OriginalWidget = Layer->GetSlotWidgetAt(0);

	Layer->SetCardSlots({});
	TestEqual(TEXT("Card is outgoing after remove"), Layer->GetSlotMotionDebugView().OutgoingSlotCount, 1);
	TestEqual(TEXT("Outgoing keeps original widget"), FWacomFirstPersonCardLayerTestAccess::OutgoingSlotAt(*Layer, 0), OriginalWidget);

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
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);

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
	FWacomFirstPersonCardSlotVisualConfig VisualConfig;
	VisualConfig.PendingTargetingLiftPixels = 40.0f;
	VisualConfig.PendingTargetingScale = 1.08f;
	Layer->SetSlotVisualConfig(VisualConfig);

	const FGuid PendingId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Base =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(PendingId, 0, FVector2D(100.0f, 200.0f));
	Layer->SetCardSlots({ Base });
	UWacomFirstPersonCardLayerSlotWidget* BaseWidget = Layer->GetSlotWidgetAt(0);

	FWacomFirstPersonCardLayerSlotView Pending = Base;
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
	FWacomFirstPersonCardLayerTestAccess::AddUntrackedSlotChild(*Layer);
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
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
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
	Anchor->bDrawPreviewCardLayer = true;
	Anchor->HandCardRenderScale = 0.5f;
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
	Anchor->SetBattleHandInteractionEnabled(true);
	Anchor->RefreshCardLayerForTest();

	UWacomFirstPersonCardLayerWidget* Layer = FWacomFirstPersonCardLayerTestAccess::CardLayer(*Anchor);
	if (Layer)
	{
		FWacomFirstPersonCardSlotMotionConfig MotionConfig;
		MotionConfig.bEnabled = false;
		Layer->SetSlotMotionConfig(MotionConfig);
	}
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

		TestTrue(TEXT("Slot hover request succeeds"), FWacomFirstPersonCardLayerTestAccess::RequestHover(*SlotWidget));
		TestEqual(TEXT("Anchor records hovered card"), Anchor->GetHoveredCardInstanceId(), CardInstanceId);
		TestEqual(TEXT("Hover broadcasts once"), Receiver.HoverCount, 1);

		const TArray<FWacomFirstPersonCardLayerSlotView> HoverSlots = Anchor->BuildActiveCardLayerSlotViewsForTest();
		TestEqual(TEXT("Hover slot count"), HoverSlots.Num(), 1);
		if (HoverSlots.Num() == 1)
		{
			TestEqual(TEXT("Hover keeps base scale in anchor slot"), HoverSlots[0].RenderScale, BaseSlots[0].RenderScale);
			TestEqual(TEXT("Hover keeps base position in anchor slot"), HoverSlots[0].ScreenPosition, BaseSlots[0].ScreenPosition);
			TestEqual(TEXT("Hover keeps base z-order in anchor slot"), HoverSlots[0].ZOrder, BaseSlots[0].ZOrder);
			TestTrue(TEXT("Hover slot is marked for presentation updates"), HoverSlots[0].bIsHovered);
			const FWacomFirstPersonCardLayerSlotView& HoverVisualSlot = SlotWidget->GetVisualSlotView();
			TestEqual(TEXT("Hover visual scales card"), HoverVisualSlot.RenderScale, BaseSlots[0].RenderScale * 1.1f);
			TestEqual(TEXT("Hover visual lifts card"), HoverVisualSlot.ScreenPosition.Y, BaseSlots[0].ScreenPosition.Y - 30.0f);
			TestEqual(TEXT("Hover visual raises z-order"), HoverVisualSlot.ZOrder, BaseSlots[0].ZOrder + 250);
		}

		FWacomFirstPersonCardLayerTestAccess::RequestUnhover(*SlotWidget);
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
	Anchor->bDrawPreviewCardLayer = true;
	Anchor->HandCardRenderScale = 0.5f;
	const FGuid CardInstanceId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerEntry Entry;
	Entry.CardInstanceId = CardInstanceId;
	Entry.CardViewData.Name = FText::FromString(TEXT("Hovered layout"));
	Anchor->SetRuntimeCardLayerEntries(TEXT("BattleHand"), { Entry });
	Anchor->SetBattleHandInteractionEnabled(true);
	Anchor->RefreshCardLayerForTest();

	UWacomFirstPersonCardLayerWidget* Layer = FWacomFirstPersonCardLayerTestAccess::CardLayer(*Anchor);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer ? Layer->GetSlotWidgetAt(0) : nullptr;
	if (TestNotNull(TEXT("Layer widget"), Layer)
		&& TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		WacomFirstPersonCardLayerSpec::FLayerLayoutUpdateReceiver Receiver;
		Anchor->OnFirstPersonCardLayerHoveredCardLayoutUpdated.AddRaw(
			&Receiver,
			&WacomFirstPersonCardLayerSpec::FLayerLayoutUpdateReceiver::HandleUpdated);

		TestTrue(TEXT("Slot hover succeeds"), FWacomFirstPersonCardLayerTestAccess::RequestHover(*SlotWidget));
		Anchor->RefreshCardLayerForTest();
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
	Anchor->bDrawPreviewCardLayer = true;
	Anchor->HandCardRenderScale = 0.5f;
	const FGuid CardInstanceId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerEntry Entry;
	Entry.CardInstanceId = CardInstanceId;
	Entry.CardViewData.Name = FText::FromString(TEXT("Card Target"));
	Entry.bIsPlayable = false;
	Anchor->SetRuntimeCardLayerEntries(TEXT("BattleHand"), { Entry });
	Anchor->SetBattleHandInteractionEnabled(true);
	Anchor->RefreshCardLayerForTest();

	UWacomFirstPersonCardLayerWidget* Layer = FWacomFirstPersonCardLayerTestAccess::CardLayer(*Anchor);
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

		TestTrue(TEXT("Non-playable slot hover still succeeds"), FWacomFirstPersonCardLayerTestAccess::RequestHover(*SlotWidget));
		TestEqual(TEXT("Anchor forwards target hover"), Receiver.HoverCount, 1);
		TestEqual(TEXT("Anchor hovered target id"), Receiver.LastHandle.CardInstanceId, CardInstanceId);
		TestEqual(TEXT("Anchor BuildCardTargetHandle returns current handle"),
			Anchor->BuildCardTargetHandle().CardInstanceId, CardInstanceId);
		TestTrue(TEXT("Anchor target source remains slot widget"),
			Anchor->BuildCardTargetHandle().SourceObject.Get() == SlotWidget);

		FWacomFirstPersonCardLayerTestAccess::RequestUnhover(*SlotWidget);
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
	FWacomFirstPersonCardLayerProgrammaticDragStartTest,
	"Wacom.UI.FirstPersonCardLayer.Interaction.ProgrammaticDragStartsValidPlayableCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerProgrammaticDragStartTest::RunTest(const FString& Parameters)
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
	WacomFirstPersonCardLayerSpec::FLayerDragReceiver Receiver;
	Layer->OnCardDragStartedNative.AddRaw(
		&Receiver,
		&WacomFirstPersonCardLayerSpec::FLayerDragReceiver::HandleStarted);
	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardInstanceId)
	});
	Layer->SetCardLayerInteractionEnabled(true);

	TestEqual(TEXT("Interaction on makes layer self-hit-test-invisible"), Layer->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		TestEqual(TEXT("Interaction on makes slot visible"), SlotWidget->GetVisibility(), ESlateVisibility::Visible);
		TestTrue(TEXT("Playable projected slot programmatic drag succeeds"), Layer->TryStartCardDragGesture(CardInstanceId));
		TestEqual(TEXT("Playable projected slot enters no-target drag"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::DraggingNoTargetCard);
	}
	TestEqual(TEXT("Drag starts once"), Receiver.StartedCount, 1);
	TestEqual(TEXT("Drag forwards card id"), Receiver.LastCardId, CardInstanceId);
	TestEqual(TEXT("Drag view uses no-target state"), Receiver.LastDragView.GestureState, EWacomFirstPersonCardGestureState::DraggingNoTargetCard);
	TestEqual(TEXT("Fallback press uses card position"), Receiver.LastDragView.PressScreenPosition, FVector2D(500.0f, 600.0f));
	TestEqual(TEXT("Fallback current pointer uses card position"), Receiver.LastDragView.CurrentScreenPosition, FVector2D(500.0f, 600.0f));

	Layer->OnCardDragStartedNative.RemoveAll(&Receiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerProgrammaticDragUsesInitialPointerTest,
	"Wacom.UI.FirstPersonCardLayer.Interaction.ProgrammaticDragUsesSeparateOriginAndPointer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerProgrammaticDragUsesInitialPointerTest::RunTest(const FString& Parameters)
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

	Layer->SetSlotMotionConfig(WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig());
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid CardInstanceId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardInstanceId);
	Slot.ScreenPosition = FVector2D(500.0f, 600.0f);
	Slot.WidgetPosition = Slot.ScreenPosition;
	Slot.SnappedWidgetPosition = Slot.ScreenPosition;
	Layer->SetCardSlots({ Slot });
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);

	WacomFirstPersonCardLayerSpec::FLayerDragReceiver Receiver;
	Layer->OnCardDragStartedNative.AddRaw(
		&Receiver,
		&WacomFirstPersonCardLayerSpec::FLayerDragReceiver::HandleStarted);
	Layer->OnCardDragUpdatedNative.AddRaw(
		&Receiver,
		&WacomFirstPersonCardLayerSpec::FLayerDragReceiver::HandleUpdated);

	const FVector2D PointerPosition(620.0f, 520.0f);
	TestTrue(TEXT("Programmatic drag with pointer succeeds"),
		Layer->TryStartCardDragGesture(CardInstanceId, PointerPosition));

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*SlotWidget, 1.0f);
		TestEqual(TEXT("No-target visual follows pointer delta from card origin"),
			SlotWidget->GetVisualSlotView().ScreenPosition,
			PointerPosition);
	}

	TestEqual(TEXT("Drag starts once"), Receiver.StartedCount, 1);
	TestEqual(TEXT("Drag updates once"), Receiver.UpdatedCount, 1);
	TestEqual(TEXT("Press remains card origin"), Receiver.LastDragView.PressScreenPosition, FVector2D(500.0f, 600.0f));
	TestEqual(TEXT("Current pointer uses provided position"), Receiver.LastDragView.CurrentScreenPosition, PointerPosition);
	TestTrue(TEXT("Drag view has pointer viewport position"), Receiver.LastDragView.bHasPointerViewportPosition);
	TestEqual(TEXT("Pointer viewport uses provided position"), Receiver.LastDragView.PointerViewportPosition, PointerPosition);

	Layer->OnCardDragStartedNative.RemoveAll(&Receiver);
	Layer->OnCardDragUpdatedNative.RemoveAll(&Receiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerProgrammaticDragIgnoresStalePointerViewTest,
	"Wacom.UI.FirstPersonCardLayer.Interaction.ProgrammaticDragIgnoresStalePointerView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerProgrammaticDragIgnoresStalePointerViewTest::RunTest(const FString& Parameters)
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

	Layer->SetSlotMotionConfig(WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig());
	Layer->SetCardLayerInteractionEnabled(true);
	FWacomFirstPersonCardLayerTestAccess::SetViewportSizeOverride(*Layer, FVector2D(1000.0f, 1000.0f));

	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid HoveredCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView SourceSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(SourceCardId);
	SourceSlot.Index = 0;
	SourceSlot.ScreenPosition = FVector2D(360.0f, 620.0f);
	SourceSlot.WidgetPosition = SourceSlot.ScreenPosition;
	SourceSlot.SnappedWidgetPosition = SourceSlot.ScreenPosition;
	FWacomFirstPersonCardLayerSlotView HoveredSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(HoveredCardId);
	HoveredSlot.Index = 1;
	HoveredSlot.ScreenPosition = FVector2D(720.0f, 620.0f);
	HoveredSlot.WidgetPosition = HoveredSlot.ScreenPosition;
	HoveredSlot.SnappedWidgetPosition = HoveredSlot.ScreenPosition;
	Layer->SetCardSlots({ SourceSlot, HoveredSlot });
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);

	UWacomFirstPersonCardLayerSlotWidget* HoveredSlotWidget = Layer->GetSlotWidgetAt(1);
	if (!TestNotNull(TEXT("Hovered slot widget"), HoveredSlotWidget))
	{
		PC->Destroy();
		return false;
	}

	const FVector2D StalePointerPosition(720.0f, 620.0f);
	TestTrue(TEXT("Pointer enters hovered card"),
		FWacomFirstPersonCardLayerTestAccess::HandleSlotPointerEnteredAtWidgetPosition(
			*Layer,
			*HoveredSlotWidget,
			StalePointerPosition));
	const FWacomFirstPersonCardLayerAutomationTestView PointerView =
		FWacomFirstPersonCardLayerTestAccess::View(*Layer);
	TestTrue(TEXT("Layer has pointer view before programmatic drag"), PointerView.bHasCurrentPointerView);
	TestEqual(TEXT("Pointer view belongs to hovered card"), PointerView.CurrentPointerView.CardInstanceId, HoveredCardId);

	WacomFirstPersonCardLayerSpec::FLayerDragReceiver Receiver;
	Layer->OnCardDragStartedNative.AddRaw(
		&Receiver,
		&WacomFirstPersonCardLayerSpec::FLayerDragReceiver::HandleStarted);

	TestTrue(TEXT("Programmatic source drag succeeds without explicit pointer"),
		Layer->TryStartCardDragGesture(SourceCardId));

	TestEqual(TEXT("Drag source uses requested card"), Receiver.LastCardId, SourceCardId);
	TestEqual(TEXT("Fallback press uses requested card position"),
		Receiver.LastDragView.PressScreenPosition,
		SourceSlot.ScreenPosition);
	TestEqual(TEXT("Fallback current ignores stale hovered pointer"),
		Receiver.LastDragView.CurrentScreenPosition,
		SourceSlot.ScreenPosition);
	TestNotEqual(TEXT("Fallback current is not stale pointer"),
		Receiver.LastDragView.CurrentScreenPosition,
		StalePointerPosition);

	Layer->OnCardDragStartedNative.RemoveAll(&Receiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerMouseDragIgnoresExternalPointerPumpTest,
	"Wacom.UI.FirstPersonCardLayer.Interaction.MouseDragIgnoresExternalPointerPump",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerMouseDragIgnoresExternalPointerPumpTest::RunTest(const FString& Parameters)
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

	Layer->SetSlotMotionConfig(WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig());
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid CardInstanceId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardInstanceId);
	Slot.ScreenPosition = FVector2D(500.0f, 600.0f);
	Slot.WidgetPosition = Slot.ScreenPosition;
	Slot.SnappedWidgetPosition = Slot.ScreenPosition;
	Layer->SetCardSlots({ Slot });
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	const FVector2D PressPosition(500.0f, 600.0f);
	const FVector2D MouseDragPosition(500.0f, 300.0f);
	const FVector2D StaleExternalPumpPosition(500.0f, 600.0f);
	TestTrue(TEXT("Mouse press starts layer gesture"),
		FWacomFirstPersonCardLayerTestAccess::RequestPressAtWidgetPosition(*Layer, PressPosition));
	TestEqual(TEXT("Mouse press records mouse gesture source"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).GestureSource,
		EWacomFirstPersonCardGestureSource::MousePress);
	TestTrue(TEXT("Mouse drag updates through layer pointer route"),
		FWacomFirstPersonCardLayerTestAccess::HandleSlotPointerMovedAtWidgetPosition(
			*Layer,
			*SlotWidget,
			MouseDragPosition));

	const FWacomFirstPersonCardLayerAutomationTestView ViewAfterMouseMove =
		FWacomFirstPersonCardLayerTestAccess::View(*Layer);
	TestEqual(TEXT("Mouse drag updates current pointer"),
		ViewAfterMouseMove.CurrentDragView.CurrentScreenPosition,
		MouseDragPosition);
	TestEqual(TEXT("Mouse drag view keeps mouse gesture source"),
		ViewAfterMouseMove.CurrentDragView.GestureSource,
		EWacomFirstPersonCardGestureSource::MousePress);
	TestTrue(TEXT("Mouse no-target drag enters an active drag state"),
		SlotWidget->GetGestureStateForFirstPersonLayer()
			== EWacomFirstPersonCardGestureState::DraggingNoTargetCard
		|| SlotWidget->GetGestureStateForFirstPersonLayer()
			== EWacomFirstPersonCardGestureState::ArmedForCommit);
	TestEqual(TEXT("Mouse no-target drag uses source-card layout motion"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).ActiveMotionIntent,
		EWacomFirstPersonCardMotionIntent::Layout);
	TestFalse(TEXT("External pump does not override mouse-origin drag"),
		Layer->UpdateActiveDragPointerFromWidgetPosition(StaleExternalPumpPosition));

	const FWacomFirstPersonCardLayerAutomationTestView ViewAfterPump =
		FWacomFirstPersonCardLayerTestAccess::View(*Layer);
	TestEqual(TEXT("Mouse drag pointer remains after ignored pump"),
		ViewAfterPump.CurrentDragView.CurrentScreenPosition,
		MouseDragPosition);
	TestEqual(TEXT("Mouse drag arrow remains after ignored pump"),
		ViewAfterPump.AimArrowEnd,
		MouseDragPosition);

	const FVector2D NextMouseDragPosition(500.0f, 260.0f);
	TestTrue(TEXT("Mouse-origin drag continues through slot pointer route"),
		FWacomFirstPersonCardLayerTestAccess::HandleSlotPointerMovedAtWidgetPosition(
			*Layer,
			*SlotWidget,
			NextMouseDragPosition));
	const FWacomFirstPersonCardLayerAutomationTestView ViewAfterNextMouseMove =
		FWacomFirstPersonCardLayerTestAccess::View(*Layer);
	TestEqual(TEXT("Mouse-origin drag pointer follows next slot pointer move"),
		ViewAfterNextMouseMove.CurrentDragView.CurrentScreenPosition,
		NextMouseDragPosition);
	TestEqual(TEXT("Mouse-origin drag arrow follows next slot pointer move"),
		ViewAfterNextMouseMove.AimArrowEnd,
		NextMouseDragPosition);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerProgrammaticDragPointerPumpTest,
	"Wacom.UI.FirstPersonCardLayer.Interaction.ProgrammaticDragPointerPumpUpdatesActiveDrag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerProgrammaticDragPointerPumpTest::RunTest(const FString& Parameters)
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

	Layer->SetSlotMotionConfig(WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig());
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid CardInstanceId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardInstanceId);
	Slot.Entry.TargetMode = ECardTargetMode::SingleEnemyPart;
	Slot.ScreenPosition = FVector2D(500.0f, 600.0f);
	Slot.WidgetPosition = Slot.ScreenPosition;
	Slot.SnappedWidgetPosition = Slot.ScreenPosition;
	Layer->SetCardSlots({ Slot });
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	WacomFirstPersonCardLayerSpec::FLayerDragReceiver Receiver;
	Layer->OnCardDragUpdatedNative.AddRaw(
		&Receiver,
		&WacomFirstPersonCardLayerSpec::FLayerDragReceiver::HandleUpdated);

	const FVector2D InitialPointerPosition(620.0f, 520.0f);
	const FVector2D PumpedPointerPosition(780.0f, 420.0f);
	TestTrue(TEXT("Programmatic targeted drag starts"),
		Layer->TryStartCardDragGesture(CardInstanceId, InitialPointerPosition));
	TestTrue(TEXT("Active drag is reported after start"), Layer->IsCardDragGestureActive());
	TestEqual(TEXT("Programmatic drag records keyboard gesture source"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).GestureSource,
		EWacomFirstPersonCardGestureSource::KeyboardShortcut);
	TestEqual(TEXT("Programmatic targeted drag uses pending source-card motion"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).ActiveMotionIntent,
		EWacomFirstPersonCardMotionIntent::Pending);
	TestTrue(TEXT("Pointer pump updates active drag"),
		Layer->UpdateActiveDragPointerFromWidgetPosition(PumpedPointerPosition));

	const FWacomFirstPersonCardLayerAutomationTestView LayerView =
		FWacomFirstPersonCardLayerTestAccess::View(*Layer);
	TestEqual(TEXT("Pump keeps press at card origin"),
		LayerView.CurrentDragView.PressScreenPosition,
		Slot.ScreenPosition);
	TestEqual(TEXT("Pump drag view keeps keyboard gesture source"),
		LayerView.CurrentDragView.GestureSource,
		EWacomFirstPersonCardGestureSource::KeyboardShortcut);
	TestEqual(TEXT("Pump updates current drag pointer"),
		LayerView.CurrentDragView.CurrentScreenPosition,
		PumpedPointerPosition);
	TestTrue(TEXT("Pump drag view has pointer viewport position"),
		LayerView.CurrentDragView.bHasPointerViewportPosition);
	TestEqual(TEXT("Pump updates pointer viewport position"),
		LayerView.CurrentDragView.PointerViewportPosition,
		PumpedPointerPosition);
	TestEqual(TEXT("Aim arrow endpoint follows pumped pointer"),
		LayerView.AimArrowEnd,
		PumpedPointerPosition);
	TestEqual(TEXT("Pump broadcasts drag update"),
		Receiver.LastDragView.CurrentScreenPosition,
		PumpedPointerPosition);

	Layer->OnCardDragUpdatedNative.RemoveAll(&Receiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerEndTurnShortcutCancelsActiveDragTest,
	"Wacom.UI.FirstPersonCardLayer.Interaction.EndTurnShortcutCancelsActiveDrag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerEndTurnShortcutCancelsActiveDragTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity);
	AWacomPlayerCharacter* Character =
		World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Character))
		{
			Character->Destroy();
		}
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	PC->Possess(Character);
	UWacomFirstPersonCardAnchorComponent* Anchor = Character->GetFirstPersonCardAnchorComponent();
	if (!TestNotNull(TEXT("Anchor"), Anchor))
	{
		return false;
	}

	Anchor->SetBattleHandInteractionEnabled(true);

	const FGuid CardId = FGuid::NewGuid();
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("First-person layer"), Layer))
	{
		return false;
	}

	FWacomFirstPersonCardSlotMotionConfig MotionConfig =
		WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig();
	MotionConfig.LayoutMotionProfile.MotionSpeed = 1.0f;
	MotionConfig.LayoutMotionProfile.OpacitySpeed = 1.0f;
	MotionConfig.LayoutMotionProfile.EasePower = 1.0f;
	MotionConfig.ResetDistancePixels = 120.0f;
	Layer->SetSlotMotionConfig(MotionConfig);
	Layer->SetCardLayerInteractionEnabled(true);
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId);
	Slot.ScreenPosition = FVector2D(500.0f, 600.0f);
	Slot.WidgetPosition = Slot.ScreenPosition;
	Slot.SnappedWidgetPosition = Slot.ScreenPosition;
	Slot.Entry.CardViewData.Name = FText::FromString(TEXT("Shortcut Cancel"));
	Layer->SetCardSlots({ Slot });
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
	FWacomFirstPersonCardLayerTestAccess::SetCardLayer(*Anchor, Layer);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	const FVector2D DragPointerPosition(500.0f, 60.0f);
	TestTrue(TEXT("Shortcut test drag starts"),
		Anchor->TryStartFirstPersonCardDragGesture(CardId, DragPointerPosition));
	TestTrue(TEXT("Anchor reports active drag before EndTurn shortcut"),
		Anchor->IsFirstPersonCardDragGestureActive());
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
	TestEqual(TEXT("No-target visual reaches drag pointer before cancel"),
		SlotWidget->GetVisualSlotView().ScreenPosition,
		DragPointerPosition);

	FWacomBattleSceneTargetClickTestAccess::PressEndTurnShortcut(PC);

	TestFalse(TEXT("EndTurn shortcut cancels active first-person drag"),
		Anchor->IsFirstPersonCardDragGestureActive());
	TestFalse(TEXT("Layer no longer reports active drag after EndTurn shortcut"),
		Layer->IsCardDragGestureActive());
	TestEqual(TEXT("Cancel keeps no-target visual at drag position for return motion"),
		SlotWidget->GetVisualSlotView().ScreenPosition,
		DragPointerPosition);

	FWacomFirstPersonCardLayerSlotView RefreshedSlot = Slot;
	RefreshedSlot.ScreenPosition += FVector2D(2.0f, 0.0f);
	RefreshedSlot.WidgetPosition = RefreshedSlot.ScreenPosition;
	RefreshedSlot.SnappedWidgetPosition = RefreshedSlot.ScreenPosition;
	Layer->SetCardSlots({ RefreshedSlot });
	TestEqual(TEXT("Post-cancel layer refresh preserves return motion"),
		SlotWidget->GetVisualSlotView().ScreenPosition,
		DragPointerPosition);

	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.1f);
	const FVector2D ReturnPosition = SlotWidget->GetVisualSlotView().ScreenPosition;
	TestTrue(TEXT("Post-cancel return starts moving toward hand"),
		ReturnPosition.Y > DragPointerPosition.Y);
	TestTrue(TEXT("Post-cancel return does not snap to hand"),
		ReturnPosition.Y < RefreshedSlot.ScreenPosition.Y);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerWaitShortcutCancelsTargetedAimTest,
	"Wacom.UI.FirstPersonCardLayer.Interaction.WaitShortcutCancelsTargetedAim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerWaitShortcutCancelsTargetedAimTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity);
	AWacomPlayerCharacter* Character =
		World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Character))
		{
			Character->Destroy();
		}
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	PC->Possess(Character);
	UWacomFirstPersonCardAnchorComponent* Anchor = Character->GetFirstPersonCardAnchorComponent();
	if (!TestNotNull(TEXT("Anchor"), Anchor))
	{
		return false;
	}

	Anchor->SetBattleHandInteractionEnabled(true);

	const FGuid CardId = FGuid::NewGuid();
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("First-person layer"), Layer))
	{
		return false;
	}

	Layer->SetSlotMotionConfig(WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig());
	Layer->SetCardLayerInteractionEnabled(true);
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId);
	Slot.Entry.CardViewData.Name = FText::FromString(TEXT("Targeted Shortcut Cancel"));
	Slot.Entry.TargetMode = ECardTargetMode::SingleEnemyPart;
	Layer->SetCardSlots({ Slot });
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
	FWacomFirstPersonCardLayerTestAccess::SetCardLayer(*Anchor, Layer);

	TestTrue(TEXT("Targeted shortcut test drag starts"),
		Anchor->TryStartFirstPersonCardDragGesture(CardId, FVector2D(560.0f, 500.0f)));
	TestTrue(TEXT("Anchor reports active aim before Wait shortcut"),
		Anchor->IsFirstPersonCardDragGestureActive());

	FWacomBattleSceneTargetClickTestAccess::PressWaitShortcut(PC);

	TestFalse(TEXT("Wait shortcut cancels active first-person aim"),
		Anchor->IsFirstPersonCardDragGestureActive());
	TestFalse(TEXT("Layer no longer reports active aim after Wait shortcut"),
		Layer->IsCardDragGestureActive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerProgrammaticNoTargetDragAnimatesToPumpTest,
	"Wacom.UI.FirstPersonCardLayer.Interaction.ProgrammaticNoTargetDragAnimatesToPointerPump",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerProgrammaticNoTargetDragAnimatesToPumpTest::RunTest(const FString& Parameters)
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

	Layer->SetSlotMotionConfig(WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig());
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid CardInstanceId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardInstanceId);
	Slot.Entry.TargetMode = ECardTargetMode::None;
	Slot.ScreenPosition = FVector2D(500.0f, 600.0f);
	Slot.WidgetPosition = Slot.ScreenPosition;
	Slot.SnappedWidgetPosition = Slot.ScreenPosition;
	Slot.RenderAngleDegrees = 12.0f;
	Layer->SetCardSlots({ Slot });
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	const FVector2D InitialPointerPosition(500.0f, 600.0f);
	const FVector2D PumpedPointerPosition(500.0f, 430.0f);
	TestTrue(TEXT("Programmatic no-target drag starts"),
		Layer->TryStartCardDragGesture(CardInstanceId, InitialPointerPosition));
	TestTrue(TEXT("Pointer pump updates active no-target drag"),
		Layer->UpdateActiveDragPointerFromWidgetPosition(PumpedPointerPosition));

	TestEqual(TEXT("Programmatic no-target drag uses source-card layout motion"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).ActiveMotionIntent,
		EWacomFirstPersonCardMotionIntent::Layout);
	TestEqual(TEXT("No-target visual does not hard-snap on pointer pump"),
		SlotWidget->GetVisualSlotView().ScreenPosition,
		InitialPointerPosition);
	const FWacomFirstPersonCardLayerAutomationTestView LayerView =
		FWacomFirstPersonCardLayerTestAccess::View(*Layer);
	TestEqual(TEXT("No-target drag view follows pumped pointer"),
		LayerView.CurrentDragView.CurrentScreenPosition,
		PumpedPointerPosition);

	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*SlotWidget, 1.0f);
	TestEqual(TEXT("No-target visual reaches pumped pointer through slot motion"),
		SlotWidget->GetVisualSlotView().ScreenPosition,
		PumpedPointerPosition);
	TestEqual(TEXT("No-target drag visual straightens to zero angle"),
		SlotWidget->GetVisualSlotView().RenderAngleDegrees,
		0.0f);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerProgrammaticDragClickCardTargetReleasesSourceTest,
	"Wacom.UI.FirstPersonCardLayer.Interaction.ProgrammaticDragClickCardTargetReleasesSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerProgrammaticDragClickCardTargetReleasesSourceTest::RunTest(const FString& Parameters)
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

	Layer->SetSlotMotionConfig(WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig());
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid TargetCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView SourceSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(SourceCardId);
	SourceSlot.Entry.TargetMode = ECardTargetMode::HandCard;
	SourceSlot.ScreenPosition = FVector2D(500.0f, 600.0f);
	SourceSlot.WidgetPosition = SourceSlot.ScreenPosition;
	SourceSlot.SnappedWidgetPosition = SourceSlot.ScreenPosition;
	FWacomFirstPersonCardLayerSlotView TargetSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(TargetCardId);
	TargetSlot.Index = 1;
	TargetSlot.ScreenPosition = FVector2D(650.0f, 600.0f);
	TargetSlot.WidgetPosition = TargetSlot.ScreenPosition;
	TargetSlot.SnappedWidgetPosition = TargetSlot.ScreenPosition;
	TargetSlot.ZOrder = 1;
	Layer->SetCardSlots({ SourceSlot, TargetSlot });
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* TargetWidget = Layer->GetSlotWidgetAt(1);
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Target slot"), TargetWidget))
	{
		PC->Destroy();
		return false;
	}

	WacomFirstPersonCardLayerSpec::FLayerDragReceiver ReleaseReceiver;
	Layer->OnCardDragReleasedNative.AddRaw(
		&ReleaseReceiver,
		&WacomFirstPersonCardLayerSpec::FLayerDragReceiver::HandleReleased);

	const FVector2D SourcePosition = SourceWidget->GetVisualSlotView().ScreenPosition;
	const FVector2D TargetPosition = TargetWidget->GetVisualSlotView().ScreenPosition;
	TestTrue(TEXT("Programmatic hand-card drag starts"),
		Layer->TryStartCardDragGesture(SourceCardId, SourcePosition));
	TestEqual(TEXT("Source enters aiming state"),
		SourceWidget->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::AimingTargetedCard);

	TestTrue(TEXT("Click press over target card is consumed by active source drag"),
		FWacomFirstPersonCardLayerTestAccess::RequestPressAtWidgetPosition(*Layer, TargetPosition));
	TestEqual(TEXT("Target card does not steal pressed gesture"),
		TargetWidget->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::Idle);
	TestEqual(TEXT("Active drag remains on source card after target press"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.CardInstanceId,
		SourceCardId);
	TestEqual(TEXT("Target press updates source drag card target"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.CurrentTarget.CardInstanceId,
		TargetCardId);

	TestTrue(TEXT("Click release over target card releases active source drag"),
		FWacomFirstPersonCardLayerTestAccess::RequestReleaseAtWidgetPosition(*Layer, TargetPosition));
	TestEqual(TEXT("Released drag reports source card"),
		ReleaseReceiver.LastCardId,
		SourceCardId);
	TestEqual(TEXT("Released drag keeps target card"),
		ReleaseReceiver.LastDragView.CurrentTarget.CardInstanceId,
		TargetCardId);
	TestFalse(TEXT("Layer no longer has active drag after click release"),
		Layer->IsCardDragGestureActive());

	Layer->OnCardDragReleasedNative.RemoveAll(&ReleaseReceiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerPointerRouteActionsPreserveActiveSourceTest,
	"Wacom.UI.FirstPersonCardLayer.Interaction.PointerRouteActionsPreserveActiveSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerPointerRouteActionsPreserveActiveSourceTest::RunTest(const FString& Parameters)
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

	Layer->SetSlotMotionConfig(WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig());
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid TargetCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView SourceSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(SourceCardId);
	SourceSlot.Entry.TargetMode = ECardTargetMode::HandCard;
	SourceSlot.ScreenPosition = FVector2D(500.0f, 600.0f);
	SourceSlot.WidgetPosition = SourceSlot.ScreenPosition;
	SourceSlot.SnappedWidgetPosition = SourceSlot.ScreenPosition;
	FWacomFirstPersonCardLayerSlotView TargetSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(TargetCardId);
	TargetSlot.Index = 1;
	TargetSlot.ScreenPosition = FVector2D(650.0f, 600.0f);
	TargetSlot.WidgetPosition = TargetSlot.ScreenPosition;
	TargetSlot.SnappedWidgetPosition = TargetSlot.ScreenPosition;
	TargetSlot.ZOrder = 1;
	Layer->SetCardSlots({ SourceSlot, TargetSlot });
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* TargetWidget = Layer->GetSlotWidgetAt(1);
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Target slot"), TargetWidget))
	{
		PC->Destroy();
		return false;
	}

	const FVector2D SourcePosition = SourceWidget->GetVisualSlotView().ScreenPosition;
	const FVector2D TargetPosition = TargetWidget->GetVisualSlotView().ScreenPosition;
	TestEqual(
		TEXT("Idle slot press requests mouse capture"),
		FWacomFirstPersonCardLayerTestAccess::RequestPressRouteActionAtWidgetPosition(*Layer, SourcePosition),
		EWacomFirstPersonCardPointerRouteAction::CaptureMouse);
	TestEqual(
		TEXT("Pressed source owns the mouse gesture"),
		SourceWidget->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::Pressed);
	TestEqual(
		TEXT("Pressed slot release requests mouse capture release"),
		FWacomFirstPersonCardLayerTestAccess::RequestReleaseRouteActionAtWidgetPosition(*Layer, SourcePosition),
		EWacomFirstPersonCardPointerRouteAction::ReleaseMouseCapture);
	TestEqual(
		TEXT("Source returns to idle after mouse release"),
		SourceWidget->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::Idle);

	WacomFirstPersonCardLayerSpec::FLayerDragReceiver ReleaseReceiver;
	Layer->OnCardDragReleasedNative.AddRaw(
		&ReleaseReceiver,
		&WacomFirstPersonCardLayerSpec::FLayerDragReceiver::HandleReleased);

	TestTrue(TEXT("Programmatic hand-card drag starts"),
		Layer->TryStartCardDragGesture(SourceCardId, SourcePosition));
	TestEqual(
		TEXT("Programmatic source enters aiming state"),
		SourceWidget->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	TestEqual(
		TEXT("Active external drag target press is handled without mouse capture"),
		FWacomFirstPersonCardLayerTestAccess::RequestPressRouteActionAtWidgetPosition(*Layer, TargetPosition),
		EWacomFirstPersonCardPointerRouteAction::Handled);
	TestEqual(
		TEXT("Target card does not steal active gesture"),
		TargetWidget->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::Idle);
	TestEqual(
		TEXT("Active drag remains on source card"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.CardInstanceId,
		SourceCardId);
	TestEqual(
		TEXT("External target press updates source drag target"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.CurrentTarget.CardInstanceId,
		TargetCardId);
	TestEqual(
		TEXT("External drag target release requests mouse capture release"),
		FWacomFirstPersonCardLayerTestAccess::RequestReleaseRouteActionAtWidgetPosition(*Layer, TargetPosition),
		EWacomFirstPersonCardPointerRouteAction::ReleaseMouseCapture);
	TestEqual(TEXT("Released external drag reports source card"), ReleaseReceiver.LastCardId, SourceCardId);
	TestFalse(TEXT("Layer no longer has active drag after target release"), Layer->IsCardDragGestureActive());

	Layer->OnCardDragReleasedNative.RemoveAll(&ReleaseReceiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerMouseCardTargetReleaseRefreshesPointerViewTest,
	"Wacom.UI.FirstPersonCardLayer.Interaction.MouseCardTargetReleaseRefreshesPointerView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerMouseCardTargetReleaseRefreshesPointerViewTest::RunTest(const FString& Parameters)
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

	Layer->SetSlotMotionConfig(WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig());
	Layer->SetCardLayerInteractionEnabled(true);
	FWacomFirstPersonCardLayerTestAccess::SetViewportSizeOverride(*Layer, FVector2D(1920.0f, 1080.0f));

	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid TargetCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView SourceSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(SourceCardId);
	SourceSlot.Entry.TargetMode = ECardTargetMode::HandCard;
	SourceSlot.ScreenPosition = FVector2D(500.0f, 600.0f);
	SourceSlot.WidgetPosition = SourceSlot.ScreenPosition;
	SourceSlot.SnappedWidgetPosition = SourceSlot.ScreenPosition;
	FWacomFirstPersonCardLayerSlotView TargetSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(TargetCardId);
	TargetSlot.Index = 1;
	TargetSlot.ScreenPosition = FVector2D(650.0f, 600.0f);
	TargetSlot.WidgetPosition = TargetSlot.ScreenPosition;
	TargetSlot.SnappedWidgetPosition = TargetSlot.ScreenPosition;
	TargetSlot.ZOrder = 1;
	Layer->SetCardSlots({ SourceSlot, TargetSlot });
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* TargetWidget = Layer->GetSlotWidgetAt(1);
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Target slot"), TargetWidget))
	{
		PC->Destroy();
		return false;
	}

	const FVector2D SourcePosition = SourceWidget->GetVisualSlotView().ScreenPosition;
	const FVector2D TargetPosition = TargetWidget->GetVisualSlotView().ScreenPosition;
	TestTrue(TEXT("Mouse press starts source gesture"),
		FWacomFirstPersonCardLayerTestAccess::RequestPressAtWidgetPosition(*Layer, SourcePosition));
	TestTrue(TEXT("Mouse drag over hand target updates source gesture"),
		FWacomFirstPersonCardLayerTestAccess::HandleSlotPointerMovedAtWidgetPosition(
			*Layer,
			*TargetWidget,
			TargetPosition));
	TestEqual(
		TEXT("Source enters hand-card aim state"),
		SourceWidget->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::AimingTargetedCard);

	TestTrue(TEXT("Mouse release over target releases source drag"),
		FWacomFirstPersonCardLayerTestAccess::RequestReleaseAtWidgetPosition(*Layer, TargetPosition));
	const FWacomFirstPersonCardLayerAutomationTestView LayerView =
		FWacomFirstPersonCardLayerTestAccess::View(*Layer);
	TestFalse(TEXT("Layer no longer has active drag after release"), Layer->IsCardDragGestureActive());
	TestTrue(TEXT("Release refreshes card pointer view in the same route"),
		LayerView.bHasCurrentPointerView);
	TestEqual(
		TEXT("Refreshed pointer view belongs to release target"),
		LayerView.CurrentPointerView.CardInstanceId,
		TargetCardId);
	TestEqual(
		TEXT("Refreshed pointer view uses release pointer position"),
		LayerView.CurrentPointerView.PointerViewportPosition,
		TargetPosition);
	TestFalse(
		TEXT("Release refresh does not restore ordinary hover"),
		LayerView.CurrentPointerView.SlotView.bIsHovered);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerProgrammaticDragPointerPumpOutsideSlotsTest,
	"Wacom.UI.FirstPersonCardLayer.Interaction.ProgrammaticDragPointerPumpContinuesOutsideSlots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerProgrammaticDragPointerPumpOutsideSlotsTest::RunTest(const FString& Parameters)
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

	Layer->SetSlotMotionConfig(WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig());
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid CardInstanceId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardInstanceId);
	Slot.Entry.TargetMode = ECardTargetMode::SingleEnemyPart;
	Slot.ScreenPosition = FVector2D(500.0f, 600.0f);
	Slot.WidgetPosition = Slot.ScreenPosition;
	Slot.SnappedWidgetPosition = Slot.ScreenPosition;
	Layer->SetCardSlots({ Slot });
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);

	const FVector2D InitialPointerPosition(500.0f, 600.0f);
	const FVector2D OutsideSlotPointerPosition(1120.0f, 160.0f);
	TestTrue(TEXT("Programmatic targeted drag starts"),
		Layer->TryStartCardDragGesture(CardInstanceId, InitialPointerPosition));
	TestTrue(TEXT("Pump updates even when pointer is not over a slot"),
		Layer->UpdateActiveDragPointerFromWidgetPosition(OutsideSlotPointerPosition));

	const FWacomFirstPersonCardLayerAutomationTestView LayerView =
		FWacomFirstPersonCardLayerTestAccess::View(*Layer);
	TestEqual(TEXT("Outside-slot pump updates current drag pointer"),
		LayerView.CurrentDragView.CurrentScreenPosition,
		OutsideSlotPointerPosition);
	TestEqual(TEXT("Outside-slot pump updates aim arrow"),
		LayerView.AimArrowEnd,
		OutsideSlotPointerPosition);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerProgrammaticDragIgnoresSlotPointerEventsTest,
	"Wacom.UI.FirstPersonCardLayer.Interaction.ProgrammaticDragIgnoresSlotPointerEvents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerProgrammaticDragIgnoresSlotPointerEventsTest::RunTest(const FString& Parameters)
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

	Layer->SetSlotMotionConfig(WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig());
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid CardInstanceId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardInstanceId);
	Slot.Entry.TargetMode = ECardTargetMode::SingleEnemyPart;
	Slot.ScreenPosition = FVector2D(500.0f, 600.0f);
	Slot.WidgetPosition = Slot.ScreenPosition;
	Slot.SnappedWidgetPosition = Slot.ScreenPosition;
	Layer->SetCardSlots({ Slot });
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	const FVector2D InitialPointerPosition(780.0f, 420.0f);
	const FVector2D SlotPointerEventPosition(500.0f, 600.0f);
	const FVector2D PumpedPointerPosition(840.0f, 360.0f);
	TestTrue(TEXT("Programmatic targeted drag starts"),
		Layer->TryStartCardDragGesture(CardInstanceId, InitialPointerPosition));
	TestTrue(TEXT("Slot pointer enter is consumed while an external drag is active"),
		FWacomFirstPersonCardLayerTestAccess::HandleSlotPointerEnteredAtWidgetPosition(
			*Layer,
			*SlotWidget,
			SlotPointerEventPosition));

	const FWacomFirstPersonCardLayerAutomationTestView ViewAfterSlotEnter =
		FWacomFirstPersonCardLayerTestAccess::View(*Layer);
	TestEqual(TEXT("Slot pointer enter does not override external drag pointer"),
		ViewAfterSlotEnter.CurrentDragView.CurrentScreenPosition,
		InitialPointerPosition);
	TestEqual(TEXT("Slot pointer enter does not override external aim arrow"),
		ViewAfterSlotEnter.AimArrowEnd,
		InitialPointerPosition);

	TestTrue(TEXT("Slot pointer move is consumed while an external drag is active"),
		FWacomFirstPersonCardLayerTestAccess::HandleSlotPointerMovedAtWidgetPosition(
			*Layer,
			*SlotWidget,
			SlotPointerEventPosition));

	const FWacomFirstPersonCardLayerAutomationTestView ViewAfterSlotEvent =
		FWacomFirstPersonCardLayerTestAccess::View(*Layer);
	TestEqual(TEXT("Slot pointer event does not override external drag pointer"),
		ViewAfterSlotEvent.CurrentDragView.CurrentScreenPosition,
		InitialPointerPosition);
	TestEqual(TEXT("Slot pointer event does not override external aim arrow"),
		ViewAfterSlotEvent.AimArrowEnd,
		InitialPointerPosition);

	TestTrue(TEXT("External pump still updates programmatic drag"),
		Layer->UpdateActiveDragPointerFromWidgetPosition(PumpedPointerPosition));
	const FWacomFirstPersonCardLayerAutomationTestView ViewAfterPump =
		FWacomFirstPersonCardLayerTestAccess::View(*Layer);
	TestEqual(TEXT("External pump updates drag pointer after slot event"),
		ViewAfterPump.CurrentDragView.CurrentScreenPosition,
		PumpedPointerPosition);
	TestEqual(TEXT("External pump updates aim arrow after slot event"),
		ViewAfterPump.AimArrowEnd,
		PumpedPointerPosition);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerProgrammaticDragPointerPumpNoopAfterReleaseOrCancelTest,
	"Wacom.UI.FirstPersonCardLayer.Interaction.ProgrammaticDragPointerPumpNoopsAfterReleaseOrCancel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerProgrammaticDragPointerPumpNoopAfterReleaseOrCancelTest::RunTest(const FString& Parameters)
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

	Layer->SetSlotMotionConfig(WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig());
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid CardInstanceId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardInstanceId);
	Slot.Entry.TargetMode = ECardTargetMode::SingleEnemyPart;
	Slot.ScreenPosition = FVector2D(500.0f, 600.0f);
	Slot.WidgetPosition = Slot.ScreenPosition;
	Slot.SnappedWidgetPosition = Slot.ScreenPosition;
	Layer->SetCardSlots({ Slot });
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	const FVector2D InitialPointerPosition(620.0f, 520.0f);
	const FVector2D ReleasePointerPosition(700.0f, 460.0f);
	WacomFirstPersonCardLayerSpec::FLayerDragReceiver ReleaseReceiver;
	Layer->OnCardDragReleasedNative.AddRaw(
		&ReleaseReceiver,
		&WacomFirstPersonCardLayerSpec::FLayerDragReceiver::HandleReleased);
	TestTrue(TEXT("Programmatic targeted drag starts"),
		Layer->TryStartCardDragGesture(CardInstanceId, InitialPointerPosition));
	TestTrue(TEXT("Pointer pump updates active drag before fallback release"),
		Layer->UpdateActiveDragPointerFromWidgetPosition(ReleasePointerPosition));
	TestTrue(TEXT("Release active gesture at current pointer"),
		Layer->ReleaseActiveDragGestureAtCurrentPointer());
	TestFalse(TEXT("Active drag is cleared after release"), Layer->IsCardDragGestureActive());
	TestEqual(TEXT("Programmatic release broadcasts released drag"),
		ReleaseReceiver.ReleasedCount,
		1);
	TestEqual(TEXT("Programmatic release uses release pointer"),
		ReleaseReceiver.LastDragView.CurrentScreenPosition,
		ReleasePointerPosition);
	TestFalse(TEXT("Pump noops after release"),
		Layer->UpdateActiveDragPointerFromWidgetPosition(FVector2D(880.0f, 360.0f)));

	const FWacomFirstPersonCardLayerAutomationTestView LayerView =
		FWacomFirstPersonCardLayerTestAccess::View(*Layer);
	TestEqual(TEXT("Released layer drag view is reset"),
		LayerView.CurrentDragView.GestureState,
		EWacomFirstPersonCardGestureState::Idle);
	TestEqual(TEXT("Released layer drag source is reset"),
		LayerView.CurrentDragView.GestureSource,
		EWacomFirstPersonCardGestureSource::None);
	TestEqual(TEXT("Released slot gesture source is reset"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).GestureSource,
		EWacomFirstPersonCardGestureSource::None);

	TestTrue(TEXT("Programmatic targeted drag restarts before cancel"),
		Layer->TryStartCardDragGesture(CardInstanceId, InitialPointerPosition));
	Layer->CancelCardDragGesture(true);
	TestFalse(TEXT("Active drag is cleared after cancel"), Layer->IsCardDragGestureActive());
	TestFalse(TEXT("Pump noops after cancel"),
		Layer->UpdateActiveDragPointerFromWidgetPosition(FVector2D(920.0f, 320.0f)));
	const FWacomFirstPersonCardLayerAutomationTestView LayerViewAfterCancel =
		FWacomFirstPersonCardLayerTestAccess::View(*Layer);
	TestEqual(TEXT("Cancelled layer drag view is reset"),
		LayerViewAfterCancel.CurrentDragView.GestureState,
		EWacomFirstPersonCardGestureState::Idle);
	TestEqual(TEXT("Cancelled layer drag source is reset"),
		LayerViewAfterCancel.CurrentDragView.GestureSource,
		EWacomFirstPersonCardGestureSource::None);
	TestEqual(TEXT("Cancelled slot gesture source is reset"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).GestureSource,
		EWacomFirstPersonCardGestureSource::None);

	Layer->OnCardDragReleasedNative.RemoveAll(&ReleaseReceiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerInvalidProgrammaticDragNoopTest,
	"Wacom.UI.FirstPersonCardLayer.Interaction.InvalidOrUnplayableProgrammaticDragNoops",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerInvalidProgrammaticDragNoopTest::RunTest(const FString& Parameters)
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

	SlotWidget->SetCardLayerInteractionEnabled(true);

	SlotWidget->SetSlotView(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid()));
	TestFalse(TEXT("Invalid card id drag noops"), SlotWidget->BeginDragGestureFromFirstPersonLayer(FVector2D(500.0f, 600.0f)));

	SlotWidget->SetSlotView(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid(), true, false));
	TestFalse(TEXT("Unprojected slot drag noops"), SlotWidget->BeginDragGestureFromFirstPersonLayer(FVector2D(500.0f, 600.0f)));

	SlotWidget->SetSlotView(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid(), false, true));
	TestFalse(TEXT("Unplayable slot drag noops"), SlotWidget->BeginDragGestureFromFirstPersonLayer(FVector2D(500.0f, 600.0f)));
	TestEqual(TEXT("Invalid drags leave gesture idle"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::Idle);
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
	Anchor->HandCardRenderScale = 1.0f;
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
	FWacomFirstPersonCardLayerTestAccess::SetHoveredCardInstanceId(*Anchor, CardId);
	const TArray<FWacomFirstPersonCardLayerSlotView> HoverSlots = Anchor->BuildActiveCardLayerSlotViewsForTest();

	TestEqual(TEXT("Base slot count"), BaseSlots.Num(), 1);
	TestEqual(TEXT("Hover slot count"), HoverSlots.Num(), 1);
	if (BaseSlots.Num() == 1 && HoverSlots.Num() == 1)
	{
		TestTrue(TEXT("Playable hover marks hovered"), HoverSlots[0].bIsHovered);
		TestEqual(TEXT("Playable hover keeps base position in anchor slot"), HoverSlots[0].ScreenPosition, BaseSlots[0].ScreenPosition);
		TestEqual(TEXT("Playable hover keeps base scale in anchor slot"), HoverSlots[0].RenderScale, BaseSlots[0].RenderScale);
		TestEqual(TEXT("Playable hover keeps base z-order in anchor slot"), HoverSlots[0].ZOrder, BaseSlots[0].ZOrder);
	}

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
	if (TestNotNull(TEXT("Slot widget"), SlotWidget) && HoverSlots.Num() == 1)
	{
		FWacomFirstPersonCardSlotVisualConfig VisualConfig;
		VisualConfig.HoverLiftPixels = 30.0f;
		VisualConfig.HoverScale = 1.1f;
		VisualConfig.HoverZOrderBoost = 250;
		SlotWidget->SetSlotVisualConfig(VisualConfig);
		SlotWidget->SetSlotFeedbackConfig(WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());
		SlotWidget->SetCardLayerInteractionEnabled(true);
		SlotWidget->SetSlotViewImmediate(HoverSlots[0]);
		TestTrue(TEXT("Playable hover request succeeds"), FWacomFirstPersonCardLayerTestAccess::RequestHover(*SlotWidget));
		TestTrue(TEXT("Playable hover visual raises card"), SlotWidget->GetVisualSlotView().ScreenPosition.Y < BaseSlots[0].ScreenPosition.Y);
		TestEqual(TEXT("Playable hover visual applies scale"), SlotWidget->GetVisualSlotView().RenderScale, 1.1f);
		TestTrue(TEXT("Playable hover visual boosts z-order"), SlotWidget->GetVisualSlotView().ZOrder > BaseSlots[0].ZOrder);
		TestEqual(TEXT("Playable hover tint opacity"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).FeedbackOverlayOpacity, 0.2f);
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
	Anchor->HandCardRenderScale = 1.0f;
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
	FWacomFirstPersonCardLayerTestAccess::SetHoveredCardInstanceId(*Anchor, CardId);
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
		TestTrue(TEXT("Non-playable hover request succeeds"), FWacomFirstPersonCardLayerTestAccess::RequestHover(*SlotWidget));
		TestEqual(TEXT("Non-playable hover does not tint as playable"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).FeedbackOverlayOpacity, 0.0f);
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

	SlotWidget->SetSlotFeedbackConfig(WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid()));

	TestTrue(TEXT("Press succeeds"), FWacomFirstPersonCardLayerTestAccess::RequestPress(*SlotWidget));
	TestTrue(TEXT("Pressed flag is set"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bPressed);
	TestTrue(TEXT("Pressed scale applies"), FMath::IsNearlyEqual(SlotWidget->GetRenderTransform().Scale.X, 0.55f * 0.9f, KINDA_SMALL_NUMBER));
	TestEqual(
		TEXT("Pressed uses interaction feedback"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionFeedbackKind,
		EWacomFirstPersonCardInteractionFeedbackKind::Pressed);
	TestEqual(TEXT("Pressed interaction feedback opacity"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionFeedbackOpacity, 0.3f);
	TestEqual(TEXT("Pressed does not use full-card overlay"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).FeedbackOverlayOpacity, 0.0f);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerMouseUpNeutralFeedbackTest,
	"Wacom.UI.FirstPersonCardLayer.PlayableFeedback.PlayableMouseUpClearsPressWithoutConfirm",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerMouseUpNeutralFeedbackTest::RunTest(const FString& Parameters)
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
	SlotWidget->SetSlotFeedbackConfig(WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId));

	TestTrue(TEXT("Press succeeds"), FWacomFirstPersonCardLayerTestAccess::RequestPress(*SlotWidget));
	TestTrue(TEXT("Mouse up succeeds"), FWacomFirstPersonCardLayerTestAccess::RequestMouseUp(*SlotWidget));
	TestFalse(TEXT("Mouse up clears pressed"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bPressed);
	TestFalse(TEXT("Mouse up does not start confirm feedback"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bConfirmFeedbackActive);
	TestEqual(
		TEXT("Mouse up clears interaction feedback"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionFeedbackKind,
		EWacomFirstPersonCardInteractionFeedbackKind::None);
	TestEqual(TEXT("Mouse up keeps full-card overlay hidden"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).FeedbackOverlayOpacity, 0.0f);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDenyFeedbackTest,
	"Wacom.UI.FirstPersonCardLayer.PlayableFeedback.NonPlayableMouseUpReturnsNeutral",
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

	SlotWidget->SetSlotFeedbackConfig(WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid(), false, true));

	TestTrue(TEXT("Press succeeds for non-playable interactable slot"), FWacomFirstPersonCardLayerTestAccess::RequestPress(*SlotWidget));
	TestTrue(TEXT("Mouse up is consumed"), FWacomFirstPersonCardLayerTestAccess::RequestMouseUp(*SlotWidget));
	TestFalse(TEXT("Quick non-playable mouse up does not start deny feedback"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bDenyFeedbackActive);
	TestEqual(TEXT("Quick non-playable mouse up does not use full-card overlay"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).FeedbackOverlayOpacity, 0.0f);
	TestTrue(TEXT("Fallback wrapper still exposes interaction feedback image"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bHasInteractionFeedbackImage);
	TestFalse(TEXT("Interaction feedback material is optional"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bInteractionFeedbackMaterialConfigured);
	TestFalse(TEXT("Interaction feedback material is not loaded when unset"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bInteractionFeedbackMaterialLoaded);
	TestFalse(TEXT("Unset material does not use Anchor override"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bInteractionFeedbackUsesOverrideMaterial);
	TestFalse(TEXT("Unset material does not use WBP brush material"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bInteractionFeedbackUsesBrushMaterial);
	TestEqual(
		TEXT("Quick mouse up clears interaction feedback state"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionFeedbackKind,
		EWacomFirstPersonCardInteractionFeedbackKind::None);
	TestEqual(TEXT("Unset material keeps interaction feedback hidden"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionFeedbackOpacity, 0.0f);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*SlotWidget, 0.05f);
	TestEqual(TEXT("Interaction feedback expires"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionFeedbackOpacity, 0.0f);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerInteractionFeedbackMaterialTest,
	"Wacom.UI.FirstPersonCardLayer.PlayableFeedback.ConfiguredInteractionFeedbackMaterialRendersFeedback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerInteractionFeedbackMaterialTest::RunTest(const FString& Parameters)
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
	UMaterial* TestMaterial = NewObject<UMaterial>(PC, TEXT("InteractionFeedbackTestMaterial"));
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget)
		|| !TestNotNull(TEXT("Test material"), TestMaterial))
	{
		return false;
	}

	FWacomFirstPersonCardSlotFeedbackConfig FeedbackConfig =
		WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig();
	FeedbackConfig.InteractionFeedbackMaterial = TestMaterial;
	FeedbackConfig.InteractionFeedbackEdgeWidth = 0.07f;
	FeedbackConfig.InteractionFeedbackEdgeSoftness = 0.03f;
	FeedbackConfig.InteractionFeedbackVignetteStrength = 0.4f;
	FeedbackConfig.InteractionFeedbackVignetteRadius = 0.52f;
	FeedbackConfig.InteractionFeedbackVignetteSoftness = 0.22f;

	SlotWidget->SetSlotFeedbackConfig(FeedbackConfig);
	SlotWidget->SetCardLayerInteractionEnabled(true);
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid(), true, true);
	Slot.Entry.TargetMode = ECardTargetMode::SingleEnemyPart;
	SlotWidget->SetSlotViewImmediate(Slot);

	TestTrue(TEXT("Gesture press starts for configured interaction material slot"),
		FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f)));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(540.0f, 590.0f));
	TestEqual(TEXT("Aim starts before deny release"),
		SlotWidget->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	TestTrue(TEXT("Invalid aim release is consumed"),
		FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, FVector2D(540.0f, 590.0f)));
	const FWacomFirstPersonCardSlotAutomationTestView FeedbackView =
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget);
	TestEqual(
		TEXT("Configured material uses unified interaction feedback"),
		FeedbackView.InteractionFeedbackKind,
		EWacomFirstPersonCardInteractionFeedbackKind::Deny);
	TestTrue(TEXT("Interaction feedback material is configured"), FeedbackView.bInteractionFeedbackMaterialConfigured);
	TestTrue(TEXT("Interaction feedback material instance is loaded"), FeedbackView.bInteractionFeedbackMaterialLoaded);
	TestTrue(TEXT("Interaction feedback material comes from Anchor override"), FeedbackView.bInteractionFeedbackUsesOverrideMaterial);
	TestFalse(TEXT("Interaction feedback material does not come from WBP brush"), FeedbackView.bInteractionFeedbackUsesBrushMaterial);
	TestTrue(TEXT("Interaction feedback layer is above full-card feedback overlay"),
		FeedbackView.bInteractionFeedbackLayerAboveFeedbackOverlay);
	TestEqual(TEXT("Configured material drives interaction feedback opacity"), FeedbackView.InteractionFeedbackOpacity, 0.5f);
	TestEqual(TEXT("Configured material keeps full-card overlay hidden"), FeedbackView.FeedbackOverlayOpacity, 0.0f);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerInteractionFeedbackBrushMaterialTest,
	"Wacom.UI.FirstPersonCardLayer.PlayableFeedback.WBPBrushInteractionFeedbackMaterialRendersFeedback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerInteractionFeedbackBrushMaterialTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleHUDLocalPlayerControllerTest* PC = World->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
		AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
		FTransform::Identity);
	UWacomFirstPersonCardLayerBrushFeedbackFirstPersonCardViewProbe* CardView =
		NewObject<UWacomFirstPersonCardLayerBrushFeedbackFirstPersonCardViewProbe>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Card view"), CardView))
	{
		return false;
	}

	CardView->TakeWidget();

	FWacomFirstPersonCardInteractionFeedbackView InteractionView;
	InteractionView.Kind = EWacomFirstPersonCardInteractionFeedbackKind::Deny;
	InteractionView.Color = FLinearColor::Red;
	InteractionView.Opacity = 0.5f;
	InteractionView.Pulse = 1.0f;
	CardView->SetInteractionFeedbackView(InteractionView);

	const FWacomFirstPersonCardViewAutomationTestView FeedbackView =
		CardView->GetAutomationTestViewForTest();
	TestEqual(
		TEXT("Brush material uses unified interaction feedback"),
		FeedbackView.InteractionFeedbackKind,
		EWacomFirstPersonCardInteractionFeedbackKind::Deny);
	TestTrue(TEXT("Interaction feedback image exists"), FeedbackView.bHasInteractionFeedbackImage);
	TestTrue(TEXT("Interaction feedback material is configured from WBP brush"), FeedbackView.bInteractionFeedbackMaterialConfigured);
	TestTrue(TEXT("Interaction feedback material instance is loaded from WBP brush"), FeedbackView.bInteractionFeedbackMaterialLoaded);
	TestFalse(TEXT("WBP brush material does not use Anchor override"), FeedbackView.bInteractionFeedbackUsesOverrideMaterial);
	TestTrue(TEXT("WBP brush material source is reported"), FeedbackView.bInteractionFeedbackUsesBrushMaterial);
	TestEqual(TEXT("WBP brush material drives deny opacity"), FeedbackView.InteractionFeedbackOpacity, 0.5f);
	TestEqual(TEXT("WBP brush material keeps full-card overlay hidden"), FeedbackView.FeedbackOverlayOpacity, 0.0f);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDenySuppressesFullCardOverlayTest,
	"Wacom.UI.FirstPersonCardLayer.PlayableFeedback.DenySuppressesExistingFullCardOverlay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDenySuppressesFullCardOverlayTest::RunTest(const FString& Parameters)
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
	UMaterial* TestMaterial = NewObject<UMaterial>(PC, TEXT("DenySuppressesOverlayTestMaterial"));
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Slot widget"), SlotWidget)
		|| !TestNotNull(TEXT("Test material"), TestMaterial))
	{
		return false;
	}

	FWacomFirstPersonCardSlotFeedbackConfig FeedbackConfig =
		WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig();
	FeedbackConfig.InteractionFeedbackMaterial = TestMaterial;
	SlotWidget->SetSlotFeedbackConfig(FeedbackConfig);

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.bEnableDragTargetFeedback = true;
	DragConfig.DragTargetFeedbackOpacity = 0.8f;
	DragConfig.DragInvalidTargetColor = FLinearColor::Red;
	SlotWidget->SetCardDragConfig(DragConfig);
	SlotWidget->SetCardLayerInteractionEnabled(true);
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid(), true, true);
	Slot.Entry.TargetMode = ECardTargetMode::SingleEnemyPart;
	SlotWidget->SetSlotViewImmediate(Slot);
	TestTrue(TEXT("Gesture press succeeds before invalid overlay"),
		FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f)));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(540.0f, 590.0f));
	TestEqual(TEXT("Aim starts before invalid overlay"),
		SlotWidget->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	SlotWidget->SetCardDragTargetAffordanceFeedback(
		EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget,
		false);

	TestEqual(TEXT("Invalid drag target starts full-card overlay"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).FeedbackOverlayOpacity,
		0.48f);

	TestTrue(TEXT("Invalid aim release triggers deny"),
		FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, FVector2D(540.0f, 590.0f)));
	const FWacomFirstPersonCardSlotAutomationTestView DenyView =
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget);
	TestTrue(TEXT("Deny starts while previous overlay existed"), DenyView.bDenyFeedbackActive);
	TestEqual(TEXT("Deny suppresses full-card overlay"), DenyView.FeedbackOverlayOpacity, 0.0f);
	TestEqual(TEXT("Interaction feedback remains visible"), DenyView.InteractionFeedbackOpacity, 0.5f);
	TestTrue(TEXT("Interaction feedback remains topmost"), DenyView.bInteractionFeedbackLayerAboveFeedbackOverlay);

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
	TestTrue(TEXT("Press starts"), FWacomFirstPersonCardLayerTestAccess::RequestPress(*SlotWidget));
	FWacomFirstPersonCardLayerTestAccess::RequestUnhover(*SlotWidget);
	TestFalse(TEXT("Unhover clears pressed"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bPressed);
	TestEqual(
		TEXT("Unhover clears interaction feedback"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionFeedbackKind,
		EWacomFirstPersonCardInteractionFeedbackKind::None);

	TestTrue(TEXT("Press restarts"), FWacomFirstPersonCardLayerTestAccess::RequestPress(*SlotWidget));
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid()));
	TestFalse(TEXT("Reuse clears pressed"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bPressed);
	TestEqual(
		TEXT("Reuse clears interaction feedback"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionFeedbackKind,
		EWacomFirstPersonCardInteractionFeedbackKind::None);

	TestTrue(TEXT("Press starts before exit"), FWacomFirstPersonCardLayerTestAccess::RequestPress(*SlotWidget));
	SlotWidget->BeginExitMotion(SlotWidget->GetSlotView());
	TestFalse(TEXT("Exit clears pressed"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bPressed);
	TestEqual(TEXT("Exit clears overlay"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).FeedbackOverlayOpacity, 0.0f);
	TestEqual(
		TEXT("Exit clears interaction feedback"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionFeedbackKind,
		EWacomFirstPersonCardInteractionFeedbackKind::None);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid()));

	TestTrue(TEXT("Press starts before disable"), FWacomFirstPersonCardLayerTestAccess::RequestPress(*SlotWidget));
	SlotWidget->SetCardLayerInteractionEnabled(false);
	TestFalse(TEXT("Disabling interaction clears pressed"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bPressed);
	TestEqual(TEXT("Disabling interaction clears overlay"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).FeedbackOverlayOpacity, 0.0f);
	TestEqual(
		TEXT("Disabling interaction clears interaction feedback"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionFeedbackKind,
		EWacomFirstPersonCardInteractionFeedbackKind::None);

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

	FWacomFirstPersonCardSlotFeedbackConfig FeedbackConfig = WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig();
	FeedbackConfig.bEnabled = false;
	SlotWidget->SetSlotFeedbackConfig(FeedbackConfig);
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid()));

	TestTrue(TEXT("Press still consumes interactable slot"), FWacomFirstPersonCardLayerTestAccess::RequestPress(*SlotWidget));
	TestEqual(TEXT("Feedback overlay stays hidden"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).FeedbackOverlayOpacity, 0.0f);
	TestTrue(TEXT("Mouse up still returns neutral"), FWacomFirstPersonCardLayerTestAccess::RequestMouseUp(*SlotWidget));
	TestFalse(TEXT("Confirm feedback stays disabled"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bConfirmFeedbackActive);

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
	"Wacom.UI.FirstPersonCardLayer.BattleHandAdapter.RuntimeSourceOverridesPreviewLayer",
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
	Anchor->bDrawPreviewCardLayer = true;
	Anchor->PreviewCardCountFallback = 5;
	TestEqual(TEXT("Development preview has five cards"), Anchor->BuildActiveCardLayerSlotViewsForTest().Num(), 5);

	FWacomCardViewData RuntimeCard;
	RuntimeCard.Name = FText::FromString(TEXT("Runtime Battle Card"));
	Anchor->SetRuntimeCardLayerData(TEXT("BattleHand"), { RuntimeCard });
	const TArray<FWacomFirstPersonCardLayerSlotView> RuntimeSlots = Anchor->BuildActiveCardLayerSlotViewsForTest();
	TestEqual(TEXT("Runtime source overrides development preview"), RuntimeSlots.Num(), 1);
	if (RuntimeSlots.Num() == 1)
	{
		TestEqual(TEXT("Runtime card data is used"), RuntimeSlots[0].Entry.CardViewData.Name.ToString(), FString(TEXT("Runtime Battle Card")));
	}

	Anchor->ClearRuntimeCardLayerData(TEXT("BattleHand"));
	TestEqual(TEXT("Clearing runtime source restores development preview"), Anchor->BuildActiveCardLayerSlotViewsForTest().Num(), 5);

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
	Anchor->bDrawPreviewCardLayer = true;
	Anchor->PreviewCardCountFallback = 5;
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
	HUD->ClearPendingFirstPersonCardTransitionEventsForTest();
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
	FWacomFirstPersonCardLayerBattleHUDLegacyClickIntentTest,
	"Wacom.UI.FirstPersonCardLayer.Interaction.BattleHUDLegacyClickIntentEntersExistingFlow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerBattleHUDLegacyClickIntentTest::RunTest(const FString& Parameters)
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
	TestTrue(TEXT("First-person hand interaction is enabled on anchor"), Anchor->IsBattleHandInteractionEnabled());

	HUD->OnCardClickedByUser(TargetCardId);
	TestEqual(TEXT("Legacy HUD targeting card enters target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);
	TestEqual(TEXT("Legacy HUD targeting card becomes pending"), HUD->GetPendingTargetingCardId(), TargetCardId);

	const int32 VersionBeforeNoTarget = Session->BuildSnapshot().Version;
	HUD->OnCardClickedByUser(NoTargetCardId);
	WacomFirstPersonCardLayerSpec::SettleBattlePresentationQueue(*HUD);
	TestEqual(TEXT("Legacy HUD no-target card returns idle after submit"), HUD->GetUIState(), EBattleUIState::Idle);
	TestFalse(TEXT("Legacy HUD no-target submit clears pending card"), HUD->GetPendingTargetingCardId().IsValid());
	TestTrue(TEXT("Legacy HUD no-target submit changes battle state"),
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
	TestFalse(TEXT("Anchor interaction is disabled"), Anchor->IsBattleHandInteractionEnabled());

	FWacomFirstPersonCardDragView DragView =
		WacomFirstPersonCardLayerSpec::MakeDropDragView(TargetCardId, EWacomFirstPersonCardGestureState::AimingTargetedCard);
	Anchor->OnFirstPersonCardLayerDragReleased.Broadcast(TargetCardId, DragView);
	TestEqual(TEXT("Cleared drag release no longer reaches HUD"), HUD->GetUIState(), EBattleUIState::Idle);
	TestFalse(TEXT("Cleared drag release does not set pending card"), HUD->GetPendingTargetingCardId().IsValid());

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
	TestTrue(TEXT("Detail provider can create first-person detail panel"), HUD->EnsureFirstPersonCardDetailPanelForTest());
	TestTrue(TEXT("First-person detail viewport z-order is above card layer"), HUD->GetFirstPersonCardDetailViewportZOrderForTest() > 9996);

	HUD->HandleFirstPersonCardHoveredForTest(
		FirstSnapshot.InstanceId,
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FirstSnapshot.InstanceId));
	TestFalse(TEXT("First-person hover waits before showing detail"), HUD->IsCardDetailPanelVisible());
	HUD->TickCardDetailMotionForTest(0.12f);
	TestTrue(TEXT("First-person hover shows detail"), HUD->IsCardDetailPanelVisible());
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
	UWacomFirstPersonCardAnchorComponent* Anchor = Character->GetFirstPersonCardAnchorComponent();
	FHandCardSnapshot FirstSnapshot = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(FirstCard, 3, true);
	FHandCardSnapshot SecondSnapshot = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(SecondCard, 4, true);
	const FBattleSnapshot Snapshot = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({ FirstSnapshot, SecondSnapshot });

	HUD->SyncFirstPersonBattleHandLayerForTest(Snapshot);
	TestTrue(TEXT("BattleHUD writes first-person runtime hand by default"), Anchor->HasRuntimeCardLayerData());
	TestTrue(TEXT("BattleHUD enables first-person hand interaction by default"), Anchor->IsBattleHandInteractionEnabled());
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
	Anchor->HandCardRenderScale = 0.5f;
	Anchor->PendingTargetingLiftPixels = 40.0f;
	Anchor->PendingTargetingScale = 1.2f;
	Anchor->PendingTargetingZOrderBoost = 1200;
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
		TestTrue(TEXT("All slots know target-select is active"), Slots[0].bHasPendingTargetingCardInHand);
		TestTrue(TEXT("Pending card keeps pending state marker"), Slots[1].Entry.bIsPendingTargeting);
		TestEqual(TEXT("Pending card keeps base scale in anchor slot"), Slots[1].RenderScale, 0.5f);
		TestEqual(TEXT("Non-pending keeps base opacity in anchor slot"), Slots[0].RenderOpacity, 1.0f);
		TestEqual(TEXT("Disabled anchor keeps normal card scale"), Slots[2].RenderScale, 0.5f);
		TestEqual(TEXT("Disabled card keeps disabled base opacity in anchor slot"), Slots[2].RenderOpacity, 0.6f);
		TestTrue(TEXT("Disabled card view data stays disabled"), Slots[2].Entry.CardViewData.bDisabled);
	}

	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (TestNotNull(TEXT("Layer widget"), Layer))
	{
		FWacomFirstPersonCardSlotMotionConfig MotionConfig;
		MotionConfig.bEnabled = false;
		Layer->SetSlotMotionConfig(MotionConfig);
		FWacomFirstPersonCardSlotVisualConfig VisualConfig;
		VisualConfig.PendingTargetingLiftPixels = 40.0f;
		VisualConfig.PendingTargetingScale = 1.2f;
		VisualConfig.PendingTargetingZOrderBoost = 1200;
		VisualConfig.TargetSelectNonPendingOpacityMultiplier = 0.5f;
		Layer->SetSlotVisualConfig(VisualConfig);
		Layer->SetCardSlots(Slots);
		TestEqual(TEXT("Layer is hit-test-invisible"), Layer->GetVisibility(), ESlateVisibility::HitTestInvisible);
		if (TestNotNull(TEXT("Pending card view exists"), Layer->GetCardViewAt(1)))
		{
			TestTrue(TEXT("Pending visual lifts above normal card"), Layer->GetSlotWidgetAt(1)->GetVisualSlotView().ScreenPosition.Y < Slots[1].ScreenPosition.Y);
			TestEqual(TEXT("Pending widget scale uses visual presentation"), Layer->GetCardRenderTransformAt(1).Scale, FVector2D(0.5f * 1.2f));
			TestTrue(TEXT("Pending widget z-order is raised"), Layer->GetCardZOrderAt(1) > Layer->GetCardZOrderAt(2));
		}
		if (TestNotNull(TEXT("Normal card view exists"), Layer->GetCardViewAt(0)))
		{
			TestEqual(TEXT("Non-pending card is visually deemphasized during target select"), Layer->GetCardRenderOpacityAt(0), 0.5f);
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
		TestEqual(TEXT("Pending angle keeps base fan angle in anchor slot"), BlendedSlots[0].RenderAngleDegrees, -6.0f);
		TestEqual(TEXT("Non-pending angle remains unchanged"), BlendedSlots[1].RenderAngleDegrees, UnblendedSlots[1].RenderAngleDegrees);

		UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
		if (TestNotNull(TEXT("Pending slot widget"), SlotWidget))
		{
			FWacomFirstPersonCardSlotMotionConfig MotionConfig;
			MotionConfig.bEnabled = false;
			SlotWidget->SetSlotMotionConfig(MotionConfig);
			FWacomFirstPersonCardSlotVisualConfig VisualConfig;
			VisualConfig.bPendingTargetingStraightenAngle = true;
			VisualConfig.PendingTargetingAngleBlend = 0.75f;
			SlotWidget->SetSlotVisualConfig(VisualConfig);
			SlotWidget->SetSlotViewImmediate(BlendedSlots[0]);
			TestEqual(TEXT("Pending visual angle blends toward zero"), SlotWidget->GetVisualSlotView().RenderAngleDegrees, -1.5f);
		}
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
	Anchor->HandCardRenderScale = 1.0f;
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

	FWacomFirstPersonCardLayerTestAccess::SetHoveredCardInstanceId(*Anchor, PendingId);
	const TArray<FWacomFirstPersonCardLayerSlotView> PendingHoverSlots = Anchor->BuildActiveCardLayerSlotViewsForTest();

	TestEqual(TEXT("Pending slot count"), PendingSlots.Num(), 1);
	TestEqual(TEXT("Pending hover slot count"), PendingHoverSlots.Num(), 1);
	if (PendingSlots.Num() == 1 && PendingHoverSlots.Num() == 1)
	{
		TestTrue(TEXT("Pending hover still marks hovered slot"), PendingHoverSlots[0].bIsHovered);
		TestEqual(TEXT("Pending hover keeps base scale in anchor slot"), PendingHoverSlots[0].RenderScale, PendingSlots[0].RenderScale);
		TestEqual(TEXT("Pending hover keeps base position in anchor slot"), PendingHoverSlots[0].ScreenPosition, PendingSlots[0].ScreenPosition);
		TestEqual(TEXT("Pending hover keeps base z-order in anchor slot"), PendingHoverSlots[0].ZOrder, PendingSlots[0].ZOrder);
		UWacomFirstPersonCardLayerSlotWidget* SlotWidget = NewObject<UWacomFirstPersonCardLayerSlotWidget>(PC);
		if (TestNotNull(TEXT("Pending slot widget"), SlotWidget))
		{
			FWacomFirstPersonCardSlotMotionConfig MotionConfig;
			MotionConfig.bEnabled = false;
			SlotWidget->SetSlotMotionConfig(MotionConfig);
			FWacomFirstPersonCardSlotVisualConfig VisualConfig;
			VisualConfig.PendingTargetingLiftPixels = 40.0f;
			VisualConfig.PendingTargetingScale = 1.2f;
			VisualConfig.PendingTargetingZOrderBoost = 1000;
			VisualConfig.HoverLiftPixels = 30.0f;
			VisualConfig.HoverScale = 1.1f;
			VisualConfig.HoverZOrderBoost = 250;
			SlotWidget->SetSlotVisualConfig(VisualConfig);
			SlotWidget->SetCardLayerInteractionEnabled(true);
			SlotWidget->SetSlotViewImmediate(PendingHoverSlots[0]);
			TestEqual(TEXT("Pending visual does not add hover scale"), SlotWidget->GetVisualSlotView().RenderScale, 1.2f);
			TestEqual(TEXT("Pending visual does not add hover lift"), SlotWidget->GetVisualSlotView().ScreenPosition.Y, PendingSlots[0].ScreenPosition.Y - 40.0f);
			TestEqual(TEXT("Pending visual does not add hover z-order"), SlotWidget->GetVisualSlotView().ZOrder, PendingSlots[0].ZOrder + 1000);
		}
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
	PendingSlot.ScreenPosition = FVector2D(100.0f, 200.0f);

	FWacomFirstPersonCardSlotVisualConfig VisualConfig;
	VisualConfig.PendingTargetingScale = 1.2f;
	SlotWidget->SetSlotVisualConfig(VisualConfig);
	SlotWidget->SetSlotFeedbackConfig(WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(PendingSlot);

	TestTrue(TEXT("Pending hover request succeeds"), FWacomFirstPersonCardLayerTestAccess::RequestHover(*SlotWidget));
	TestEqual(TEXT("Pending hover does not use playable hover tint"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).FeedbackOverlayOpacity, 0.0f);
	const float VisualScaleBeforePress = SlotWidget->GetVisualSlotView().RenderScale;
	TestTrue(TEXT("Pending press succeeds"), FWacomFirstPersonCardLayerTestAccess::RequestPress(*SlotWidget));
	TestTrue(TEXT("Pending press applies only press scale on top of visual presentation"),
		FMath::IsNearlyEqual(SlotWidget->GetRenderTransform().Scale.X, VisualScaleBeforePress * 0.9f, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Pending mouse up returns neutral"), FWacomFirstPersonCardLayerTestAccess::RequestMouseUp(*SlotWidget));
	TestFalse(TEXT("Pending mouse up clears pressed"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bPressed);
	TestFalse(TEXT("Pending mouse up does not confirm"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bConfirmFeedbackActive);

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
	FWacomFirstPersonCardLayerBattleHUDLateCleanupKeepsRunSourceTest,
	"Wacom.UI.FirstPersonCardLayer.BattleHandAdapter.LateCleanupDoesNotDisableRunRuntimeSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerBattleHUDLateCleanupKeepsRunSourceTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(
		APlayerController::StaticClass(),
		FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(
		AWacomPlayerCharacter::StaticClass(),
		FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage());
	UCardDefinition* BattleCard = WacomFirstPersonCardLayerSpec::MakePreviewCard(
		GetTransientPackage(),
		TEXT("Battle.LateCleanup.Battle"),
		1);
	UCardDefinition* RunCard = WacomFirstPersonCardLayerSpec::MakePreviewCard(
		GetTransientPackage(),
		TEXT("Battle.LateCleanup.Run"),
		0);
	FWacomBattleFixture Fixture;
	UBattleSession* Session = WacomFirstPersonCardLayerSpec::CreateMinimalBattleSession(Fixture);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("BattleCard"), BattleCard)
		|| !TestNotNull(TEXT("RunCard"), RunCard)
		|| !TestNotNull(TEXT("Session"), Session))
	{
		if (Character)
		{
			Character->Destroy();
		}
		if (PC)
		{
			PC->Destroy();
		}
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeBattleHUDWithCharacter(HUD, PC, Character, World);
	HUD->SetSession(Session);
	UWacomFirstPersonCardAnchorComponent* Anchor = Character->GetFirstPersonCardAnchorComponent();
	if (!TestNotNull(TEXT("Anchor"), Anchor))
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	const FHandCardSnapshot BattleSnapshot =
		WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(BattleCard, 1, true);
	HUD->SyncFirstPersonBattleHandLayerForTest(
		WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({ BattleSnapshot }));
	TestEqual(TEXT("Battle source owns runtime hand before Run restore"),
		Anchor->GetRuntimeCardLayerSourceId(),
		FName(TEXT("BattleHand")));
	TestTrue(TEXT("Battle source enables interaction before Run restore"),
		Anchor->IsBattleHandInteractionEnabled());

	FWacomFirstPersonCardLayerEntry RunEntry;
	RunEntry.CardInstanceId = FGuid::NewGuid();
	RunEntry.CardViewData = UWacomCardPresentationBuilder::BuildCardViewData(RunCard);
	RunEntry.bIsPlayable = true;
	RunEntry.CardViewData.bDisabled = false;
	Anchor->SetRuntimeCardLayerEntries(TEXT("RunFirstPersonBattleDeck"), { RunEntry });
	Anchor->SetBattleHandInteractionEnabled(true);
	TestEqual(TEXT("Run source takes over before late BattleHUD cleanup"),
		Anchor->GetRuntimeCardLayerSourceId(),
		FName(TEXT("RunFirstPersonBattleDeck")));

	HUD->ClearFirstPersonBattleHandLayerForTest();
	TestEqual(TEXT("Late BattleHUD cleanup keeps Run source ownership"),
		Anchor->GetRuntimeCardLayerSourceId(),
		FName(TEXT("RunFirstPersonBattleDeck")));
	TestTrue(TEXT("Late BattleHUD cleanup keeps Run source data"),
		Anchor->HasRuntimeCardLayerData());
	TestEqual(TEXT("Late BattleHUD cleanup keeps Run source entry"),
		Anchor->GetRuntimeCardLayerEntries().Num(),
		1);
	TestTrue(TEXT("Late BattleHUD cleanup keeps Run interaction enabled"),
		Anchor->IsBattleHandInteractionEnabled());

	FWacomFirstPersonCardDragView DragView = WacomFirstPersonCardLayerSpec::MakeDropDragView(
		BattleSnapshot.InstanceId,
		EWacomFirstPersonCardGestureState::DraggingNoTargetCard,
		true);
	Anchor->OnFirstPersonCardLayerDragReleased.Broadcast(BattleSnapshot.InstanceId, DragView);
	TestEqual(TEXT("Late cleanup still unbinds BattleHUD drag delegates"),
		HUD->GetUIState(),
		EBattleUIState::Idle);

	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerQuickReleaseNeutralTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.QuickReleaseBeforeHoldDelayIsNeutral",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerQuickReleaseNeutralTest::RunTest(const FString& Parameters)
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

	TestTrue(TEXT("Gesture press starts"), FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f)));
	TestTrue(TEXT("Release before delay succeeds"), FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, FVector2D(500.0f, 600.0f)));
	TestEqual(TEXT("Quick release returns to idle"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::Idle);
	TestFalse(TEXT("Quick release does not confirm"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bConfirmFeedbackActive);
	TestFalse(TEXT("Quick release does not deny"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bDenyFeedbackActive);

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

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(960.0f, 540.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(1440.0f, 270.0f));
	const FWacomFirstPersonCardDragView DragView = SlotWidget->BuildDragView();

	TestTrue(TEXT("Drag view has pointer viewport position"), DragView.bHasPointerViewportPosition);
	TestEqual(TEXT("Pointer viewport position follows gesture pointer"), DragView.PointerViewportPosition, FVector2D(1440.0f, 270.0f));
	TestEqual(TEXT("Pointer normalized X uses widget viewport"), static_cast<float>(DragView.PointerNormalizedViewportPosition.X), 0.5f);
	TestEqual(TEXT("Pointer normalized Y uses widget viewport"), static_cast<float>(DragView.PointerNormalizedViewportPosition.Y), -0.5f);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, FVector2D(1440.0f, 270.0f));
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerHoverPointerViewportTest,
	"Wacom.UI.FirstPersonCardLayer.CardPointerCameraLook.HoverReportsPointerViewportPosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerHoverPointerViewportTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true);
	Slot.ScreenPosition = FVector2D(750.0f, 250.0f);
	Slot.InputHitCenter = Slot.ScreenPosition;
	Slot.InputHitScale = 1.0f;
	Slot.InputHitAngleDegrees = 0.0f;
	Slot.InputHitOrder = 0;
	Layer->SetCardLayerInteractionEnabled(true);
	FWacomFirstPersonCardLayerTestAccess::SetViewportSizeOverride(*Layer, FVector2D(1000.0f, 1000.0f));
	Layer->SetCardSlots({ Slot });

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		return false;
	}

	const bool bHandled =
		FWacomFirstPersonCardLayerTestAccess::HandleSlotPointerEnteredAtWidgetPosition(
			*Layer,
			*SlotWidget,
			FVector2D(750.0f, 250.0f));
	const FWacomFirstPersonCardLayerAutomationTestView LayerView =
		FWacomFirstPersonCardLayerTestAccess::View(*Layer);
	TestTrue(TEXT("Hover pointer handled by layer"), bHandled);
	TestTrue(TEXT("Layer stores current pointer view"), LayerView.bHasCurrentPointerView);
	TestEqual(TEXT("Pointer card id"), LayerView.CurrentPointerView.CardInstanceId, CardId);
	TestTrue(TEXT("Pointer has viewport position"), LayerView.CurrentPointerView.bHasPointerViewportPosition);
	TestEqual(TEXT("Pointer viewport position"), LayerView.CurrentPointerView.PointerViewportPosition, FVector2D(750.0f, 250.0f));
	TestEqual(TEXT("Pointer normalized viewport position"), LayerView.CurrentPointerView.PointerNormalizedViewportPosition, FVector2D(0.5f, -0.5f));

	Layer->SetCardSlots(TArray<FWacomFirstPersonCardLayerSlotView>());
	const FWacomFirstPersonCardLayerAutomationTestView ClearedView =
		FWacomFirstPersonCardLayerTestAccess::View(*Layer);
	TestFalse(TEXT("Removing hovered card clears pointer view"), ClearedView.bHasCurrentPointerView);

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

	TestTrue(TEXT("Gesture press starts"), FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f)));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.12f, FVector2D(500.0f, 600.0f));
	TestEqual(TEXT("Hold enters inspect state"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::Inspecting);
	TestEqual(TEXT("Inspect start broadcasts"), Receiver.StartedCount, 1);
	TestTrue(TEXT("Inspect visual scales up"), SlotWidget->GetVisualSlotView().RenderScale >= 0.55f);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, FVector2D(500.0f, 600.0f));
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
	TestTrue(TEXT("Gesture press starts"), FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, PressPosition));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.12f, PressPosition);
	TestEqual(TEXT("Hold enters inspect state"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::Inspecting);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*SlotWidget, 0.25f);
	TestNotEqual(TEXT("Inspect motion moves visual slot away from source"),
		SlotWidget->GetVisualSlotView().ScreenPosition,
		SlotWidget->GetSlotView().ScreenPosition);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.0f, PressPosition);
	const FWacomFirstPersonCardDragView DragView = SlotWidget->BuildDragView();
	TestEqual(TEXT("Press remains in widget-space"), DragView.PressScreenPosition, PressPosition);
	TestEqual(TEXT("Current pointer remains in widget-space"), DragView.CurrentScreenPosition, PressPosition);
	TestEqual(TEXT("Moving inspect slot does not self-trigger drag"),
		SlotWidget->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::Inspecting);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, PressPosition);
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
	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, PressPosition);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.12f, PressPosition);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*SlotWidget, 1.0f);
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

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, PressPosition);
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
	TestTrue(TEXT("Gesture press starts"), FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, PressPosition));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.12f, PressPosition);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*SlotWidget, 1.0f);
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

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, PressPosition);
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
	TestTrue(TEXT("Gesture press starts"), FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, PressPosition));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.12f, PressPosition);
	const int32 UpdatesAfterEnteringInspect = DragReceiver.UpdatedCount;
	const FVector2D VisualPositionBeforeMotion = SlotWidget->GetVisualSlotView().ScreenPosition;

	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*SlotWidget, 0.16f);
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
	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, PressPosition);
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

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.12f, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, FVector2D(500.0f, 600.0f));
	TestFalse(TEXT("Inspect release does not play deny feedback"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bDenyFeedbackActive);

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

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(505.0f, 575.0f));
	TestEqual(TEXT("No-target drag state"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::DraggingNoTargetCard);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, FVector2D(505.0f, 575.0f));
	TestEqual(TEXT("Short drag release returns idle"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::Idle);

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

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(500.0f, 540.0f));
	TestEqual(TEXT("Below commit distance stays dragging"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::DraggingNoTargetCard);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(500.0f, 480.0f));
	TestEqual(TEXT("Past upward distance arms commit"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::ArmedForCommit);
	TestTrue(TEXT("Drag view reports armed"), SlotWidget->BuildDragView().bCommitArmed);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, FVector2D(500.0f, 480.0f));
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

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(500.0f, 540.0f));
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*SlotWidget, 1.0f);
	TestEqual(TEXT("Drag starts"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::DraggingNoTargetCard);
	TestEqual(TEXT("Initial drag follows pointer delta from frozen visual start"),
		SlotWidget->GetVisualSlotView().ScreenPosition,
		FVector2D(500.0f, 540.0f));

	FWacomFirstPersonCardLayerSlotView LiveMovedSlot = InitialSlot;
	LiveMovedSlot.ScreenPosition = FVector2D(660.0f, 720.0f);
	LiveMovedSlot.WidgetPosition = LiveMovedSlot.ScreenPosition;
	LiveMovedSlot.SnappedWidgetPosition = LiveMovedSlot.ScreenPosition;
	SlotWidget->SetSlotView(LiveMovedSlot);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.0f, FVector2D(500.0f, 520.0f));
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*SlotWidget, 1.0f);
	TestEqual(TEXT("Live slot refresh does not add anchor drift to drag visual"),
		SlotWidget->GetVisualSlotView().ScreenPosition,
		FVector2D(500.0f, 520.0f));

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, FVector2D(500.0f, 520.0f));
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

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(500.0f, 300.0f));
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*SlotWidget, 1.0f);
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

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, FVector2D(500.0f, 300.0f));
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

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(540.0f, 590.0f));
	TestEqual(TEXT("Targeted card enters aim state"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::AimingTargetedCard);
	TestEqual(TEXT("Layer drag started"), DragReceiver.StartedCount, 1);
	TestEqual(TEXT("Layer current drag is aim"), FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.GestureState, EWacomFirstPersonCardGestureState::AimingTargetedCard);

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
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(560.0f, 590.0f));
	TestEqual(TEXT("Aim starts"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::AimingTargetedCard);
	TestEqual(TEXT("Aim arrow starts at frozen visual source"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.SourceSlotView.ScreenPosition,
		FVector2D(500.0f, 600.0f));
	TestEqual(TEXT("Aim arrow ends at pointer"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.CurrentScreenPosition,
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
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.0f, FVector2D(580.0f, 570.0f));

	const FWacomFirstPersonCardDragView DragView = FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView;
	TestEqual(TEXT("Live slot refresh does not move aim source"),
		DragView.SourceSlotView.ScreenPosition,
		FVector2D(500.0f, 600.0f));
	TestEqual(TEXT("Aim endpoint remains current pointer"),
		DragView.CurrentScreenPosition,
		FVector2D(580.0f, 570.0f));

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, FVector2D(580.0f, 570.0f));
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerAimArrowStartFollowsVisualSourceTest,
	"Wacom.UI.FirstPersonCardLayer.CardDragInspect.TargetedAimArrowStartFollowsVisualSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerAimArrowStartFollowsVisualSourceTest::RunTest(const FString& Parameters)
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

	FWacomFirstPersonCardDragConfig DragConfig;
	DragConfig.CardDragStartThresholdPixels = 10.0f;
	DragConfig.CardInspectHoldDelaySeconds = 0.1f;
	Layer->SetCardDragConfig(DragConfig);

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId, true, true);
	Slot.Entry.TargetMode = ECardTargetMode::SingleEnemyPart;
	Slot.ScreenPosition = FVector2D(500.0f, 600.0f);
	Slot.WidgetPosition = Slot.ScreenPosition;
	Slot.SnappedWidgetPosition = Slot.ScreenPosition;
	Slot.AnchorWidgetPosition = Slot.ScreenPosition;
	Layer->SetCardSlots({ Slot });
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		PC->Destroy();
		return false;
	}

	const FVector2D PressPosition = Slot.ScreenPosition;
	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, PressPosition);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.12f, PressPosition);
	TestEqual(TEXT("Card enters inspect before aim"),
		SlotWidget->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::Inspecting);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*SlotWidget, 1.0f);

	const FVector2D AimPointerPosition = PressPosition + FVector2D(60.0f, -20.0f);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.0f, AimPointerPosition);
	const FWacomFirstPersonCardLayerAutomationTestView ViewAtAimStart =
		FWacomFirstPersonCardLayerTestAccess::View(*Layer);
	TestEqual(TEXT("Card enters aim"),
		ViewAtAimStart.CurrentDragView.GestureState,
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	TestEqual(TEXT("Cached drag source records aim promotion visual"),
		ViewAtAimStart.AimArrowStart,
		ViewAtAimStart.CurrentDragView.SourceSlotView.ScreenPosition);

	const FVector2D CachedDragSource = ViewAtAimStart.CurrentDragView.SourceSlotView.ScreenPosition;
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*SlotWidget, 0.16f);
	const FVector2D CurrentVisualSource = SlotWidget->GetVisualSlotView().ScreenPosition;
	TestNotEqual(TEXT("Aim visual moves after promotion"),
		CurrentVisualSource,
		CachedDragSource);

	const FWacomFirstPersonCardLayerAutomationTestView ViewAfterAimMotion =
		FWacomFirstPersonCardLayerTestAccess::View(*Layer);
	TestEqual(TEXT("Drag view source remains the promotion snapshot"),
		ViewAfterAimMotion.CurrentDragView.SourceSlotView.ScreenPosition,
		CachedDragSource);
	TestEqual(TEXT("Aim arrow start follows current source visual"),
		ViewAfterAimMotion.AimArrowStart,
		CurrentVisualSource);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, AimPointerPosition);
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

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(540.0f, 590.0f));
	TestEqual(TEXT("HandCard enters aim/probe state"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::AimingTargetedCard);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, FVector2D(540.0f, 590.0f));
	TestEqual(TEXT("HandCard invalid drag release returns idle"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::Idle);

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

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(540.0f, 590.0f));
	TestEqual(TEXT("Aim state active"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::AimingTargetedCard);
	Layer->SetCardLayerInteractionEnabled(false);
	TestEqual(TEXT("Interaction disabled clears gesture"), SlotWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::Idle);

	Layer->SetCardLayerInteractionEnabled(true);
	Layer->SetCardSlots({ Slot });
	SlotWidget = Layer->GetSlotWidgetAt(0);
	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(540.0f, 590.0f));
	Layer->ClearSlotMotionState();
	TestEqual(TEXT("Layer clear resets current drag"), FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.GestureState, EWacomFirstPersonCardGestureState::Idle);

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

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(500.0f, 300.0f));

	TestEqual(TEXT("Armed drag exposes commit ready state"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).DragTargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::CommitReady);
	TestEqual(TEXT("Commit ready overlay opacity"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).FeedbackOverlayOpacity, 0.25f);
	TestEqual(TEXT("Commit ready overlay color"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).FeedbackOverlayColor, FLinearColor::Green);
	TestTrue(TEXT("Commit ready scales source card"),
		SlotWidget->GetRenderTransform().Scale.X > SlotWidget->GetVisualSlotView().RenderScale);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, FVector2D(500.0f, 300.0f));
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

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(540.0f, 590.0f));

	const FVector2D TargetScreenPosition(700.0f, 420.0f);
	const FWacomInteractionTargetHandle TargetHandle =
		FWacomInteractionTargetHandle::ForWorldTarget(FGuid::NewGuid(), PC, FVector::ZeroVector, TargetScreenPosition);
	Layer->SetCardDragFeedbackTarget(
		TargetHandle,
		true,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget,
		TargetScreenPosition);

	const FWacomFirstPersonCardDragView DragView = FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView;
	TestEqual(TEXT("Drag view records valid world feedback"),
		DragView.TargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget);
	TestTrue(TEXT("Drag view has feedback target position"), DragView.bHasFeedbackTargetScreenPosition);
	TestEqual(TEXT("Source slot gets valid overlay"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).FeedbackOverlayColor, FLinearColor::Green);
	TestEqual(TEXT("Source slot valid opacity"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).FeedbackOverlayOpacity, 0.22f);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, FVector2D(540.0f, 590.0f));
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
	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, PointerPosition);
	Layer->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle::ForWorldTarget(FGuid::NewGuid(), PC, FVector::ZeroVector, TargetPosition),
		true,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget,
		TargetPosition);

	TestEqual(TEXT("Arrow end lerps from pointer toward target"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).AimArrowEnd,
		FMath::Lerp(PointerPosition, TargetPosition, 0.5f));

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, PointerPosition);
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
	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, PointerPosition);
	Layer->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle::ForWorldTarget(FGuid::NewGuid(), PC),
		true,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget);

	TestEqual(TEXT("Missing target position keeps arrow at pointer"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).AimArrowEnd,
		PointerPosition);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, PointerPosition);
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
	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FirstPointerPosition);
	Layer->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle::ForWorldTarget(FGuid::NewGuid(), PC, FVector::ZeroVector, TargetPosition),
		true,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget,
		TargetPosition);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, SecondPointerPosition);

	TestEqual(TEXT("Valid feedback state survives next drag update"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.TargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget);
	TestEqual(TEXT("Arrow keeps valid color after drag update"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).AimArrowColor,
		FLinearColor::Green);
	TestEqual(TEXT("Arrow keeps snapping after drag update"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).AimArrowEnd,
		FMath::Lerp(SecondPointerPosition, TargetPosition, 0.5f));

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, SecondPointerPosition);
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

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, FVector2D(540.0f, 590.0f));
	Layer->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle::ForCardTarget(TargetCardId, TargetWidget, TargetSlot.ScreenPosition),
		false,
		EWacomFirstPersonCardDragTargetFeedbackState::CardProbe,
		TargetSlot.ScreenPosition);

	TestTrue(TEXT("Target card shows probe feedback"), FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragProbeFeedback);
	TestEqual(TEXT("Target card probe overlay color"), FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).FeedbackOverlayColor, FLinearColor::Blue);
	TestEqual(TEXT("Target card probe overlay opacity"), FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).FeedbackOverlayOpacity, 0.3f);
	TestEqual(TEXT("Layer records card probe state"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.TargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::CardProbe);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SourceWidget, FVector2D(540.0f, 590.0f));
	TestEqual(TEXT("Card probe release returns idle"), SourceWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::Idle);

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

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, FVector2D(650.0f, 600.0f));

	const FWacomFirstPersonCardDragView DragView = FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView;
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
	TestTrue(TEXT("Target card shows probe feedback from drag pointer"), FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragProbeFeedback);
	TestEqual(TEXT("Aim arrow uses card probe color"), FWacomFirstPersonCardLayerTestAccess::View(*Layer).AimArrowColor, FLinearColor::Blue);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, FVector2D(320.0f, 600.0f));
	TestFalse(TEXT("Moving away from target card clears probe feedback"), FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragProbeFeedback);
	TestNotEqual(TEXT("Moving away from target card clears card probe state"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.TargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::CardProbe);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SourceWidget, FVector2D(650.0f, 600.0f));
	TestEqual(TEXT("Card probe drag release returns idle"), SourceWidget->GetGestureStateForFirstPersonLayer(), EWacomFirstPersonCardGestureState::Idle);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerCardTargetValidFeedbackPersistenceTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.CardTargetValidFeedbackPersistsOnSamePointerTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerCardTargetValidFeedbackPersistenceTest::RunTest(const FString& Parameters)
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
	DragConfig.CardInspectHoldDelaySeconds = 0.0f;
	DragConfig.CardDragStartThresholdPixels = 1.0f;
	DragConfig.DragTargetFeedbackOpacity = 0.3f;
	DragConfig.DragValidTargetColor = FLinearColor::Green;
	DragConfig.DragCardProbeTargetColor = FLinearColor::Blue;
	Layer->SetCardDragConfig(DragConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid TargetCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView SourceSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SourceCardId, 0, FVector2D(500.0f, 600.0f), 0.0f, 1.0f);
	SourceSlot.Entry.TargetMode = ECardTargetMode::HandCard;
	FWacomFirstPersonCardLayerSlotView TargetSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(TargetCardId, 1, FVector2D(650.0f, 600.0f), 0.0f, 1.0f);
	Layer->SetCardSlots({ SourceSlot, TargetSlot });

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* TargetWidget = Layer->GetSlotWidgetAt(1);
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Target slot"), TargetWidget))
	{
		PC->Destroy();
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, SourceSlot.ScreenPosition);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, TargetSlot.ScreenPosition);
	TestEqual(TEXT("Initial pointer target waits for HUD card validation"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.TargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::CardProbe);

	Layer->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle::ForCardTarget(TargetCardId, TargetWidget, TargetSlot.ScreenPosition),
		true,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget,
		TargetSlot.ScreenPosition,
		TEXT("CardDrop{Intent=PlayCardCardTarget}"));
	TestEqual(TEXT("HUD valid card target is stored"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.TargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, TargetSlot.ScreenPosition + FVector2D(4.0f, 0.0f));
	const FWacomFirstPersonCardDragView DragView = FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView;
	TestEqual(TEXT("Same pointer card target keeps valid state"),
		DragView.TargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);
	TestTrue(TEXT("Same pointer card target keeps valid flag"), DragView.bTargetValid);
	TestEqual(TEXT("Same pointer card target keeps target id"), DragView.CurrentTarget.CardInstanceId, TargetCardId);
	TestEqual(TEXT("Target widget stays valid after next pointer move"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).DragTargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);
	TestEqual(TEXT("Target widget focus stays valid after next pointer move"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).CardDragTargetFocusFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);
	TestTrue(TEXT("Target widget keeps focus after next pointer move"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragTargetFocusActive);
	TestEqual(TEXT("Target widget keeps valid overlay"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).FeedbackOverlayColor,
		FLinearColor::Green);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SourceWidget, TargetSlot.ScreenPosition);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerCardTargetSwitchResetsToProbeTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.SwitchingCardTargetReturnsToCardProbe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerCardTargetSwitchResetsToProbeTest::RunTest(const FString& Parameters)
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
	DragConfig.CardInspectHoldDelaySeconds = 0.0f;
	DragConfig.CardDragStartThresholdPixels = 1.0f;
	DragConfig.DragTargetFeedbackOpacity = 0.3f;
	DragConfig.DragValidTargetColor = FLinearColor::Green;
	DragConfig.DragCardProbeTargetColor = FLinearColor::Blue;
	Layer->SetCardDragConfig(DragConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid FirstTargetCardId = FGuid::NewGuid();
	const FGuid SecondTargetCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView SourceSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SourceCardId, 0, FVector2D(500.0f, 600.0f), 0.0f, 1.0f);
	SourceSlot.Entry.TargetMode = ECardTargetMode::HandCard;
	FWacomFirstPersonCardLayerSlotView FirstTargetSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(FirstTargetCardId, 1, FVector2D(650.0f, 600.0f), 0.0f, 1.0f);
	FWacomFirstPersonCardLayerSlotView SecondTargetSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SecondTargetCardId, 2, FVector2D(820.0f, 600.0f), 0.0f, 1.0f);
	Layer->SetCardSlots({ SourceSlot, FirstTargetSlot, SecondTargetSlot });

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* FirstTargetWidget = Layer->GetSlotWidgetAt(1);
	UWacomFirstPersonCardLayerSlotWidget* SecondTargetWidget = Layer->GetSlotWidgetAt(2);
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("First target slot"), FirstTargetWidget)
		|| !TestNotNull(TEXT("Second target slot"), SecondTargetWidget))
	{
		PC->Destroy();
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, SourceSlot.ScreenPosition);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, FirstTargetSlot.ScreenPosition);
	Layer->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle::ForCardTarget(FirstTargetCardId, FirstTargetWidget, FirstTargetSlot.ScreenPosition),
		true,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget,
		FirstTargetSlot.ScreenPosition,
		TEXT("CardDrop{Intent=PlayCardCardTarget}"));
	TestEqual(TEXT("First target starts valid"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.TargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, SecondTargetSlot.ScreenPosition);
	const FWacomFirstPersonCardDragView DragView = FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView;
	TestEqual(TEXT("New pointer card target records second target"), DragView.CurrentTarget.CardInstanceId, SecondTargetCardId);
	TestEqual(TEXT("New pointer card target returns to probe"),
		DragView.TargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::CardProbe);
	TestFalse(TEXT("New pointer card target waits for HUD validity"), DragView.bTargetValid);
	TestEqual(TEXT("Old target feedback clears without affordance"),
		FWacomFirstPersonCardLayerTestAccess::View(*FirstTargetWidget).DragTargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::None);
	TestFalse(TEXT("Old target focus clears"),
		FWacomFirstPersonCardLayerTestAccess::View(*FirstTargetWidget).bCardDragTargetFocusActive);
	TestEqual(TEXT("New target shows probe"),
		FWacomFirstPersonCardLayerTestAccess::View(*SecondTargetWidget).DragTargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::CardProbe);
	TestTrue(TEXT("New target gains focus"),
		FWacomFirstPersonCardLayerTestAccess::View(*SecondTargetWidget).bCardDragTargetFocusActive);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SourceWidget, SecondTargetSlot.ScreenPosition);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerAllValidAffordanceUniqueFocusTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.AllValidAffordancesKeepUniquePointerFocus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerAllValidAffordanceUniqueFocusTest::RunTest(const FString& Parameters)
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
	DragConfig.CardInspectHoldDelaySeconds = 0.0f;
	DragConfig.CardDragStartThresholdPixels = 1.0f;
	DragConfig.DragTargetFeedbackOpacity = 0.3f;
	DragConfig.DragValidTargetColor = FLinearColor::Green;
	DragConfig.DragCardProbeTargetColor = FLinearColor::Blue;
	DragConfig.DragCardTargetFocusLiftPixels = 18.0f;
	DragConfig.DragCardTargetFocusScale = 1.045f;
	DragConfig.DragCardTargetFocusZOrderBoost = 650;
	Layer->SetCardDragConfig(DragConfig);
	FWacomFirstPersonCardSlotMotionConfig MotionConfig;
	MotionConfig.bEnabled = false;
	Layer->SetSlotMotionConfig(MotionConfig);
	FWacomFirstPersonCardSlotVisualConfig VisualConfig;
	VisualConfig.DragCardTargetFocusLiftPixels = DragConfig.DragCardTargetFocusLiftPixels;
	VisualConfig.DragCardTargetFocusScale = DragConfig.DragCardTargetFocusScale;
	VisualConfig.DragCardTargetFocusZOrderBoost = DragConfig.DragCardTargetFocusZOrderBoost;
	Layer->SetSlotVisualConfig(VisualConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid FirstTargetCardId = FGuid::NewGuid();
	const FGuid SecondTargetCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView SourceSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SourceCardId, 0, FVector2D(500.0f, 600.0f), 0.0f, 1.0f);
	SourceSlot.Entry.TargetMode = ECardTargetMode::HandCard;
	FWacomFirstPersonCardLayerSlotView FirstTargetSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(FirstTargetCardId, 1, FVector2D(650.0f, 600.0f), 0.0f, 1.0f);
	FWacomFirstPersonCardLayerSlotView SecondTargetSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SecondTargetCardId, 2, FVector2D(820.0f, 600.0f), 0.0f, 1.0f);
	Layer->SetCardSlots({ SourceSlot, FirstTargetSlot, SecondTargetSlot });

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* FirstTargetWidget = Layer->GetSlotWidgetAt(1);
	UWacomFirstPersonCardLayerSlotWidget* SecondTargetWidget = Layer->GetSlotWidgetAt(2);
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("First target slot"), FirstTargetWidget)
		|| !TestNotNull(TEXT("Second target slot"), SecondTargetWidget))
	{
		PC->Destroy();
		return false;
	}

	const FWacomFirstPersonCardLayerSlotView FirstBaseVisual = FirstTargetWidget->GetVisualSlotView();
	const int32 FirstBaseZOrder = Layer->GetCardZOrderAt(1);
	const FWacomFirstPersonCardLayerSlotView SecondBaseVisual = SecondTargetWidget->GetVisualSlotView();
	const int32 SecondBaseZOrder = Layer->GetCardZOrderAt(2);

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, SourceSlot.ScreenPosition);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, FirstTargetSlot.ScreenPosition);

	TArray<FWacomFirstPersonCardTargetAffordance> Affordances;
	FWacomFirstPersonCardTargetAffordance FirstAffordance;
	FirstAffordance.CardInstanceId = FirstTargetCardId;
	FirstAffordance.FeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget;
	FirstAffordance.bCanSubmit = true;
	Affordances.Add(FirstAffordance);
	FWacomFirstPersonCardTargetAffordance SecondAffordance;
	SecondAffordance.CardInstanceId = SecondTargetCardId;
	SecondAffordance.FeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget;
	SecondAffordance.bCanSubmit = true;
	Affordances.Add(SecondAffordance);

	Layer->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle::ForCardTarget(FirstTargetCardId, FirstTargetWidget, FirstTargetSlot.ScreenPosition),
		true,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget,
		FirstTargetSlot.ScreenPosition,
		TEXT("CardDrop{Intent=PlayCardCardTarget}"),
		Affordances);

	const FWacomFirstPersonCardSlotAutomationTestView FirstView =
		FWacomFirstPersonCardLayerTestAccess::View(*FirstTargetWidget);
	const FWacomFirstPersonCardSlotAutomationTestView SecondView =
		FWacomFirstPersonCardLayerTestAccess::View(*SecondTargetWidget);
	TestTrue(TEXT("First target keeps valid affordance"), FirstView.bCardDragTargetAffordanceFeedback);
	TestTrue(TEXT("Second target keeps valid affordance"), SecondView.bCardDragTargetAffordanceFeedback);
	TestEqual(TEXT("First target affordance state"), FirstView.CardDragTargetAffordanceFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);
	TestEqual(TEXT("Second target affordance state"), SecondView.CardDragTargetAffordanceFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);
	TestTrue(TEXT("Pointer target gets focus"), FirstView.bCardDragTargetFocusActive);
	TestFalse(TEXT("Other valid affordance does not get focus"), SecondView.bCardDragTargetFocusActive);
	TestEqual(TEXT("Both targets show valid overlay"), FirstView.FeedbackOverlayColor, FLinearColor::Green);
	TestEqual(TEXT("Other affordance shows valid overlay"), SecondView.FeedbackOverlayColor, FLinearColor::Green);
	TestTrue(TEXT("Focused target applies lift"),
		FMath::IsNearlyEqual(
			FirstTargetWidget->GetVisualSlotView().ScreenPosition.Y,
			FirstBaseVisual.ScreenPosition.Y - VisualConfig.DragCardTargetFocusLiftPixels));
	TestTrue(TEXT("Focused target applies scale"),
		FMath::IsNearlyEqual(
			FirstTargetWidget->GetVisualSlotView().RenderScale,
			FirstBaseVisual.RenderScale * VisualConfig.DragCardTargetFocusScale));
	TestEqual(TEXT("Focused target raises z-order"),
		Layer->GetCardZOrderAt(1),
		FirstBaseZOrder + VisualConfig.DragCardTargetFocusZOrderBoost);
	TestTrue(TEXT("Other target keeps base visual position"),
		FMath::IsNearlyEqual(SecondTargetWidget->GetVisualSlotView().ScreenPosition.Y, SecondBaseVisual.ScreenPosition.Y));
	TestTrue(TEXT("Other target keeps base scale"),
		FMath::IsNearlyEqual(SecondTargetWidget->GetVisualSlotView().RenderScale, SecondBaseVisual.RenderScale));
	TestEqual(TEXT("Other target keeps base z-order"), Layer->GetCardZOrderAt(2), SecondBaseZOrder);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, SecondTargetSlot.ScreenPosition);
	const FWacomFirstPersonCardSlotAutomationTestView FirstAfterMove =
		FWacomFirstPersonCardLayerTestAccess::View(*FirstTargetWidget);
	const FWacomFirstPersonCardSlotAutomationTestView SecondAfterMove =
		FWacomFirstPersonCardLayerTestAccess::View(*SecondTargetWidget);
	TestTrue(TEXT("Old target keeps affordance after pointer leaves"), FirstAfterMove.bCardDragTargetAffordanceFeedback);
	TestFalse(TEXT("Old target loses focus after pointer leaves"), FirstAfterMove.bCardDragTargetFocusActive);
	TestTrue(TEXT("New pointer target keeps affordance"), SecondAfterMove.bCardDragTargetAffordanceFeedback);
	TestTrue(TEXT("New pointer target gains focus"), SecondAfterMove.bCardDragTargetFocusActive);
	TestEqual(TEXT("New target waits for HUD validation with probe focus"),
		SecondAfterMove.CardDragTargetFocusFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::CardProbe);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, FVector2D(320.0f, 600.0f));
	TestTrue(TEXT("Affordance remains after pointer leaves card body"),
		FWacomFirstPersonCardLayerTestAccess::View(*FirstTargetWidget).bCardDragTargetAffordanceFeedback);
	TestFalse(TEXT("Focus clears after pointer leaves card body"),
		FWacomFirstPersonCardLayerTestAccess::View(*SecondTargetWidget).bCardDragTargetFocusActive);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SourceWidget, FVector2D(320.0f, 600.0f));
	TestFalse(TEXT("Release clears first affordance"),
		FWacomFirstPersonCardLayerTestAccess::View(*FirstTargetWidget).bCardDragTargetAffordanceFeedback);
	TestFalse(TEXT("Release clears second affordance"),
		FWacomFirstPersonCardLayerTestAccess::View(*SecondTargetWidget).bCardDragTargetAffordanceFeedback);
	TestFalse(TEXT("Release clears focus"),
		FWacomFirstPersonCardLayerTestAccess::View(*SecondTargetWidget).bCardDragTargetFocusActive);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerBleedCardTargetProbeTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.FirstPersonCardLayerCardTargetProbeIgnoresTransparentBleed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerBleedCardTargetProbeTest::RunTest(const FString& Parameters)
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
	Layer->SetCardDragConfig(DragConfig);
	Layer->SetCardViewClass(UWacomFirstPersonCardLayerBleedFirstPersonCardViewProbe::StaticClass());
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
	TargetSlot.RenderScale = 1.0f;
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
	TargetWidget->SetDesiredSizeInViewport(FVector2D(392.0f, 516.0f));
	TargetWidget->TakeWidget();
	FWacomFirstPersonCardLayerTestAccess::SetLocalHitCanvasSizeOverride(*TargetWidget, FVector2D(392.0f, 516.0f));

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, FVector2D(830.0f, 600.0f));
	TestNotEqual(TEXT("Pointer inside bleed but outside body does not probe card target"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.CurrentTarget.TargetKind,
		EWacomInteractionTargetKind::Card);
	TestFalse(TEXT("Bleed-only pointer does not light target probe"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragProbeFeedback);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, FVector2D(650.0f, 600.0f));
	TestEqual(TEXT("Pointer inside body probes card target"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.CurrentTarget.TargetKind,
		EWacomInteractionTargetKind::Card);
	TestEqual(TEXT("Body pointer records target card id"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.CurrentTarget.CardInstanceId,
		TargetCardId);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerRotatedCardTargetProbeTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.CardTargetProbeUsesRotatedBodyHitBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerRotatedCardTargetProbeTest::RunTest(const FString& Parameters)
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
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(TargetCardId, 1, FVector2D(650.0f, 600.0f), 45.0f, 1.0f);
	Layer->SetCardSlots({ SourceSlot, TargetSlot });

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* TargetWidget = Layer->GetSlotWidgetAt(1);
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Target slot"), TargetWidget))
	{
		PC->Destroy();
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, FVector2D(760.0f, 780.0f));
	TestNotEqual(TEXT("Old axis-aligned target corner does not probe rotated target"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.CurrentTarget.TargetKind,
		EWacomInteractionTargetKind::Card);
	TestFalse(TEXT("Rejected rotated corner does not light target probe"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragProbeFeedback);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, FVector2D(742.0f, 692.0f));
	TestEqual(TEXT("Point inside rotated target body probes card target"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.CurrentTarget.TargetKind,
		EWacomInteractionTargetKind::Card);
	TestEqual(TEXT("Rotated target body records target card id"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.CurrentTarget.CardInstanceId,
		TargetCardId);

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

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, FVector2D(540.0f, 590.0f));
	Layer->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle::ForCardTarget(TargetCardId, TargetWidget, TargetSlot.ScreenPosition),
		false,
		EWacomFirstPersonCardDragTargetFeedbackState::CardProbe,
		TargetSlot.ScreenPosition);
	TestTrue(TEXT("Probe starts before clear"), FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragProbeFeedback);
	TestTrue(TEXT("Focus starts before clear"), FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragTargetFocusActive);

	Layer->CancelCardDragGesture(true);
	TestFalse(TEXT("Probe clears on cancel"), FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragProbeFeedback);
	TestFalse(TEXT("Focus clears on cancel"), FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragTargetFocusActive);
	TestEqual(TEXT("Current drag resets on cancel"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.GestureState,
		EWacomFirstPersonCardGestureState::Idle);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerCardTargetFocusVisualTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.CardTargetFocusUsesIndependentVisualsWithoutHover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerCardTargetFocusVisualTest::RunTest(const FString& Parameters)
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
	DragConfig.CardInspectHoldDelaySeconds = 0.0f;
	DragConfig.CardDragStartThresholdPixels = 1.0f;
	DragConfig.DragTargetFeedbackOpacity = 0.3f;
	DragConfig.DragCardTargetProbeScale = 1.02f;
	DragConfig.DragCardTargetFocusLiftPixels = 18.0f;
	DragConfig.DragCardTargetFocusScale = 1.045f;
	DragConfig.DragCardTargetFocusZOrderBoost = 650;
	DragConfig.DragValidTargetColor = FLinearColor::Green;
	DragConfig.DragInvalidTargetColor = FLinearColor::Red;
	DragConfig.DragCardProbeTargetColor = FLinearColor::Blue;
	Layer->SetCardDragConfig(DragConfig);
	FWacomFirstPersonCardSlotMotionConfig MotionConfig;
	MotionConfig.bEnabled = false;
	Layer->SetSlotMotionConfig(MotionConfig);
	FWacomFirstPersonCardSlotVisualConfig VisualConfig;
	VisualConfig.DragCardTargetFocusLiftPixels = DragConfig.DragCardTargetFocusLiftPixels;
	VisualConfig.DragCardTargetFocusScale = DragConfig.DragCardTargetFocusScale;
	VisualConfig.DragCardTargetFocusZOrderBoost = DragConfig.DragCardTargetFocusZOrderBoost;
	Layer->SetSlotVisualConfig(VisualConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid TargetCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView SourceSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SourceCardId, 0, FVector2D(500.0f, 600.0f), 0.0f, 1.0f);
	SourceSlot.Entry.TargetMode = ECardTargetMode::HandCard;
	FWacomFirstPersonCardLayerSlotView TargetSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(TargetCardId, 1, FVector2D(650.0f, 600.0f), 0.0f, 1.0f);
	Layer->SetCardSlots({ SourceSlot, TargetSlot });

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* TargetWidget = Layer->GetSlotWidgetAt(1);
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Target slot"), TargetWidget))
	{
		PC->Destroy();
		return false;
	}

	WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver HoverReceiver;
	Layer->OnCardHoveredNative.AddRaw(&HoverReceiver, &WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver::HandleHovered);
	Layer->OnCardUnhoveredNative.AddRaw(&HoverReceiver, &WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver::HandleUnhovered);

	const FWacomFirstPersonCardLayerSlotView BaseVisual = TargetWidget->GetVisualSlotView();
	const int32 BaseZOrder = Layer->GetCardZOrderAt(1);

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, SourceSlot.ScreenPosition);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, TargetSlot.ScreenPosition);

	TestEqual(TEXT("Drag target focus does not broadcast hover"), HoverReceiver.HoverCount, 0);
	TestFalse(TEXT("Target card is not ordinary hovered"), TargetWidget->IsHoveredForFirstPersonLayer());
	TestFalse(TEXT("Layer does not mark hover id during drag focus"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).HoveredCardInstanceId.IsValid());
	TestTrue(TEXT("Probe target focus is active"), FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragTargetFocusActive);
	TestTrue(TEXT("Probe target applies focus lift"),
		FMath::IsNearlyEqual(
			TargetWidget->GetVisualSlotView().ScreenPosition.Y,
			BaseVisual.ScreenPosition.Y - VisualConfig.DragCardTargetFocusLiftPixels));
	TestTrue(TEXT("Probe target applies focus scale"),
		FMath::IsNearlyEqual(
			TargetWidget->GetVisualSlotView().RenderScale,
			BaseVisual.RenderScale * VisualConfig.DragCardTargetFocusScale));
	TestEqual(TEXT("Probe target raises canvas z-order"),
		Layer->GetCardZOrderAt(1),
		BaseZOrder + VisualConfig.DragCardTargetFocusZOrderBoost);
	TestEqual(TEXT("Probe target overlay color"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).FeedbackOverlayColor,
		FLinearColor::Blue);

	Layer->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle::ForCardTarget(TargetCardId, TargetWidget, TargetSlot.ScreenPosition),
		false,
		EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget,
		TargetSlot.ScreenPosition,
		TEXT("CardDrop{Intent=PlayCardCardTarget Reject=InvalidTarget}"));
	TestTrue(TEXT("Invalid card target focus remains active"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragTargetFocusActive);
	TestTrue(TEXT("Invalid target keeps focus lift"),
		FMath::IsNearlyEqual(
			TargetWidget->GetVisualSlotView().ScreenPosition.Y,
			BaseVisual.ScreenPosition.Y - VisualConfig.DragCardTargetFocusLiftPixels));
	TestTrue(TEXT("Invalid target applies focus scale without probe scale"),
		FMath::IsNearlyEqual(
			TargetWidget->GetVisualSlotView().RenderScale,
			BaseVisual.RenderScale * VisualConfig.DragCardTargetFocusScale));
	TestEqual(TEXT("Invalid target keeps raised z-order"),
		Layer->GetCardZOrderAt(1),
		BaseZOrder + VisualConfig.DragCardTargetFocusZOrderBoost);
	TestEqual(TEXT("Invalid target overlay color"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).FeedbackOverlayColor,
		FLinearColor::Red);
	TestEqual(TEXT("Drag target focus still does not broadcast hover"), HoverReceiver.HoverCount, 0);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SourceWidget, TargetSlot.ScreenPosition);
	TestFalse(TEXT("Release clears target focus"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragTargetFocusActive);
	TestTrue(TEXT("Release restores target visual position"),
		FMath::IsNearlyEqual(TargetWidget->GetVisualSlotView().ScreenPosition.Y, BaseVisual.ScreenPosition.Y));
	TestTrue(TEXT("Release restores target scale"),
		FMath::IsNearlyEqual(TargetWidget->GetVisualSlotView().RenderScale, BaseVisual.RenderScale));
	TestEqual(TEXT("Release restores target z-order"), Layer->GetCardZOrderAt(1), BaseZOrder);

	Layer->OnCardHoveredNative.RemoveAll(&HoverReceiver);
	Layer->OnCardUnhoveredNative.RemoveAll(&HoverReceiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerDragTargetSuppressesOrdinaryHoverTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.DragCardTargetSuppressesOrdinaryHoverAnimations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerDragTargetSuppressesOrdinaryHoverTest::RunTest(const FString& Parameters)
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
	DragConfig.CardInspectHoldDelaySeconds = 0.0f;
	DragConfig.CardDragStartThresholdPixels = 1.0f;
	DragConfig.DragTargetFeedbackOpacity = 0.3f;
	DragConfig.DragCardProbeTargetColor = FLinearColor::Blue;
	DragConfig.DragCardTargetFocusScale = 1.045f;
	Layer->SetCardDragConfig(DragConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid TargetCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView SourceSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SourceCardId, 0, FVector2D(500.0f, 600.0f), 0.0f, 1.0f);
	SourceSlot.Entry.TargetMode = ECardTargetMode::HandCard;
	FWacomFirstPersonCardLayerSlotView TargetSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(TargetCardId, 1, FVector2D(650.0f, 600.0f), 0.0f, 1.0f);
	Layer->SetCardSlots({ SourceSlot, TargetSlot });

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* TargetWidget = Layer->GetSlotWidgetAt(1);
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Target slot"), TargetWidget))
	{
		PC->Destroy();
		return false;
	}

	WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver HoverReceiver;
	Layer->OnCardHoveredNative.AddRaw(&HoverReceiver, &WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver::HandleHovered);
	Layer->OnCardUnhoveredNative.AddRaw(&HoverReceiver, &WacomFirstPersonCardLayerSpec::FLayerInteractionReceiver::HandleUnhovered);

	TestTrue(TEXT("Source can ordinary hover before drag"),
		FWacomFirstPersonCardLayerTestAccess::ResolveHoveredCardAtWidgetPosition(*Layer, SourceSlot.ScreenPosition) == SourceCardId);
	TestTrue(TEXT("Source starts ordinary hovered"), SourceWidget->IsHoveredForFirstPersonLayer());
	TestEqual(TEXT("Initial hover broadcasts once"), HoverReceiver.HoverCount, 1);

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, SourceSlot.ScreenPosition);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, TargetSlot.ScreenPosition);
	TestFalse(TEXT("Drag start clears source ordinary hover"), SourceWidget->IsHoveredForFirstPersonLayer());
	TestFalse(TEXT("Drag start clears layer hover id"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).HoveredCardInstanceId.IsValid());
	TestEqual(TEXT("Clearing source hover broadcasts unhover"), HoverReceiver.UnhoverCount, 1);
	TestTrue(TEXT("Target focus is active from card probe"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragTargetFocusActive);

	const int32 HoverCountAfterDragProbe = HoverReceiver.HoverCount;
	TestTrue(TEXT("Pointer enter on target during drag routes to active gesture"),
		FWacomFirstPersonCardLayerTestAccess::HandleSlotPointerEnteredAtWidgetPosition(
			*Layer,
			*TargetWidget,
			TargetSlot.ScreenPosition));
	TestFalse(TEXT("Target does not become ordinary hovered during drag"), TargetWidget->IsHoveredForFirstPersonLayer());
	TestFalse(TEXT("Layer still has no ordinary hover id during drag"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).HoveredCardInstanceId.IsValid());
	TestEqual(TEXT("Target pointer enter during drag does not broadcast hover"),
		HoverReceiver.HoverCount,
		HoverCountAfterDragProbe);
	TestTrue(TEXT("Target keeps drag focus without ordinary hover"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragTargetFocusActive);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SourceWidget, TargetSlot.ScreenPosition);
	TestFalse(TEXT("Release does not immediately hover target"), TargetWidget->IsHoveredForFirstPersonLayer());
	TestFalse(TEXT("Release keeps ordinary hover id clear until next pointer move"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).HoveredCardInstanceId.IsValid());

	Layer->OnCardHoveredNative.RemoveAll(&HoverReceiver);
	Layer->OnCardUnhoveredNative.RemoveAll(&HoverReceiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerCardTargetFocusClearsWhenLeavingBodyTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.CardTargetFocusClearsWhenPointerLeavesBody",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerCardTargetFocusClearsWhenLeavingBodyTest::RunTest(const FString& Parameters)
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
	DragConfig.CardInspectHoldDelaySeconds = 0.0f;
	DragConfig.CardDragStartThresholdPixels = 1.0f;
	DragConfig.DragCardTargetFocusLiftPixels = 18.0f;
	DragConfig.DragCardTargetFocusScale = 1.045f;
	DragConfig.DragCardTargetFocusZOrderBoost = 650;
	Layer->SetCardDragConfig(DragConfig);
	FWacomFirstPersonCardSlotMotionConfig MotionConfig;
	MotionConfig.bEnabled = false;
	Layer->SetSlotMotionConfig(MotionConfig);
	FWacomFirstPersonCardSlotVisualConfig VisualConfig;
	VisualConfig.DragCardTargetFocusLiftPixels = DragConfig.DragCardTargetFocusLiftPixels;
	VisualConfig.DragCardTargetFocusScale = DragConfig.DragCardTargetFocusScale;
	VisualConfig.DragCardTargetFocusZOrderBoost = DragConfig.DragCardTargetFocusZOrderBoost;
	Layer->SetSlotVisualConfig(VisualConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid TargetCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView SourceSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SourceCardId, 0, FVector2D(500.0f, 600.0f), 0.0f, 1.0f);
	SourceSlot.Entry.TargetMode = ECardTargetMode::HandCard;
	FWacomFirstPersonCardLayerSlotView TargetSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(TargetCardId, 1, FVector2D(650.0f, 600.0f), 0.0f, 1.0f);
	Layer->SetCardSlots({ SourceSlot, TargetSlot });

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* TargetWidget = Layer->GetSlotWidgetAt(1);
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Target slot"), TargetWidget))
	{
		PC->Destroy();
		return false;
	}

	const FWacomFirstPersonCardLayerSlotView BaseVisual = TargetWidget->GetVisualSlotView();
	const int32 BaseZOrder = Layer->GetCardZOrderAt(1);

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, SourceSlot.ScreenPosition);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, TargetSlot.ScreenPosition);
	Layer->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle::ForCardTarget(TargetCardId, TargetWidget, TargetSlot.ScreenPosition),
		true,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget,
		TargetSlot.ScreenPosition,
		TEXT("CardDrop{Intent=PlayCardCardTarget}"));
	TestTrue(TEXT("Target focus starts active"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragTargetFocusActive);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, FVector2D(1200.0f, 600.0f));
	TestEqual(TEXT("Leaving card body clears card target kind"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.CurrentTarget.TargetKind,
		EWacomInteractionTargetKind::None);
	TestEqual(TEXT("Leaving card body clears feedback state"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.TargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::None);
	TestFalse(TEXT("Leaving card body clears target focus"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragTargetFocusActive);
	TestTrue(TEXT("Leaving card body restores target visual position"),
		FMath::IsNearlyEqual(TargetWidget->GetVisualSlotView().ScreenPosition.Y, BaseVisual.ScreenPosition.Y));
	TestTrue(TEXT("Leaving card body restores target scale"),
		FMath::IsNearlyEqual(TargetWidget->GetVisualSlotView().RenderScale, BaseVisual.RenderScale));
	TestEqual(TEXT("Leaving card body restores target z-order"), Layer->GetCardZOrderAt(1), BaseZOrder);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SourceWidget, FVector2D(1200.0f, 600.0f));
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

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SlotWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SlotWidget, 0.01f, FVector2D(540.0f, 590.0f));
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

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SlotWidget, FVector2D(540.0f, 590.0f));
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerValidCardTargetFeedbackTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.ValidCardTargetsUseValidCardFeedback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerValidCardTargetFeedbackTest::RunTest(const FString& Parameters)
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
	DragConfig.DragValidTargetColor = FLinearColor::Green;
	DragConfig.DragInvalidTargetColor = FLinearColor::Red;
	Layer->SetCardDragConfig(DragConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid ValidTargetCardId = FGuid::NewGuid();
	const FGuid InvalidTargetCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView SourceSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(SourceCardId, true, true);
	SourceSlot.Entry.TargetMode = ECardTargetMode::HandCard;
	FWacomFirstPersonCardLayerSlotView ValidTargetSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(ValidTargetCardId, true, true);
	ValidTargetSlot.Index = 1;
	ValidTargetSlot.ScreenPosition = FVector2D(650.0f, 600.0f);
	FWacomFirstPersonCardLayerSlotView InvalidTargetSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(InvalidTargetCardId, true, true);
	InvalidTargetSlot.Index = 2;
	InvalidTargetSlot.ScreenPosition = FVector2D(780.0f, 600.0f);
	Layer->SetCardSlots({ SourceSlot, ValidTargetSlot, InvalidTargetSlot });

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = Layer->GetSlotWidgetAt(0);
	UWacomFirstPersonCardLayerSlotWidget* ValidTargetWidget = Layer->GetSlotWidgetAt(1);
	UWacomFirstPersonCardLayerSlotWidget* InvalidTargetWidget = Layer->GetSlotWidgetAt(2);
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Valid target slot"), ValidTargetWidget)
		|| !TestNotNull(TEXT("Invalid target slot"), InvalidTargetWidget))
	{
		PC->Destroy();
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, FVector2D(540.0f, 590.0f));

	TArray<FWacomFirstPersonCardTargetAffordance> Affordances;
	FWacomFirstPersonCardTargetAffordance ValidAffordance;
	ValidAffordance.CardInstanceId = ValidTargetCardId;
	ValidAffordance.FeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget;
	ValidAffordance.bCanSubmit = true;
	Affordances.Add(ValidAffordance);
	FWacomFirstPersonCardTargetAffordance InvalidAffordance;
	InvalidAffordance.CardInstanceId = InvalidTargetCardId;
	InvalidAffordance.FeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget;
	InvalidAffordance.bCanSubmit = false;
	Affordances.Add(InvalidAffordance);

	Layer->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle::ForCardTarget(ValidTargetCardId, ValidTargetWidget, ValidTargetSlot.ScreenPosition),
		true,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget,
		ValidTargetSlot.ScreenPosition,
		TEXT("CardDrop{Intent=PlayCardCardTarget}"),
		Affordances);

	TestEqual(TEXT("Valid target uses card valid state"),
		FWacomFirstPersonCardLayerTestAccess::View(*ValidTargetWidget).DragTargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);
	TestEqual(TEXT("Valid target records affordance state"),
		FWacomFirstPersonCardLayerTestAccess::View(*ValidTargetWidget).CardDragTargetAffordanceFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);
	TestTrue(TEXT("Focused valid target receives focus"),
		FWacomFirstPersonCardLayerTestAccess::View(*ValidTargetWidget).bCardDragTargetFocusActive);
	TestEqual(TEXT("Valid target color"), FWacomFirstPersonCardLayerTestAccess::View(*ValidTargetWidget).FeedbackOverlayColor, FLinearColor::Green);
	TestEqual(TEXT("Invalid target uses card invalid state"),
		FWacomFirstPersonCardLayerTestAccess::View(*InvalidTargetWidget).DragTargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget);
	TestEqual(TEXT("Invalid target records affordance state"),
		FWacomFirstPersonCardLayerTestAccess::View(*InvalidTargetWidget).CardDragTargetAffordanceFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget);
	TestFalse(TEXT("Non-pointer invalid affordance does not receive focus"),
		FWacomFirstPersonCardLayerTestAccess::View(*InvalidTargetWidget).bCardDragTargetFocusActive);
	TestEqual(TEXT("Invalid target color"), FWacomFirstPersonCardLayerTestAccess::View(*InvalidTargetWidget).FeedbackOverlayColor, FLinearColor::Red);
	TestTrue(TEXT("Debug counts valid affordance"), Layer->GetDragTargetDebugSummary().Contains(TEXT("AffordanceValid=1")));
	TestTrue(TEXT("Debug counts invalid affordance"), Layer->GetDragTargetDebugSummary().Contains(TEXT("AffordanceInvalid=1")));

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SourceWidget, FVector2D(540.0f, 590.0f));
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerFocusedCardTargetOverrideTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.CurrentHoveredCardTargetOverridesWithStrongerFeedback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerFocusedCardTargetOverrideTest::RunTest(const FString& Parameters)
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
	Layer->SetCardDragConfig(DragConfig);
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid TargetCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView SourceSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(SourceCardId, true, true);
	SourceSlot.Entry.TargetMode = ECardTargetMode::HandCard;
	FWacomFirstPersonCardLayerSlotView TargetSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(TargetCardId, true, true);
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

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, FVector2D(540.0f, 590.0f));

	FWacomFirstPersonCardTargetAffordance InvalidAffordance;
	InvalidAffordance.CardInstanceId = TargetCardId;
	InvalidAffordance.FeedbackState = EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget;
	TArray<FWacomFirstPersonCardTargetAffordance> Affordances;
	Affordances.Add(InvalidAffordance);
	Layer->SetCardDragFeedbackTarget(
		FWacomInteractionTargetHandle::ForCardTarget(TargetCardId, TargetWidget, TargetSlot.ScreenPosition),
		true,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget,
		TargetSlot.ScreenPosition,
		TEXT("CardDrop{Intent=PlayCardCardTarget}"),
		Affordances);

	TestEqual(TEXT("Focused valid result overrides base invalid affordance"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).DragTargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);
	TestEqual(TEXT("Base invalid affordance is preserved separately"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).CardDragTargetAffordanceFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget);
	TestEqual(TEXT("Focused valid result is stored separately"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).CardDragTargetFocusFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);
	TestTrue(TEXT("Focused target receives focus"),
		FWacomFirstPersonCardLayerTestAccess::View(*TargetWidget).bCardDragTargetFocusActive);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SourceWidget, FVector2D(540.0f, 590.0f));
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
	TestEqual(TEXT("Card target records validation reason"),
		Result.TargetValidationRejectReason,
		EWacomBattleTargetRejectReason::UnsupportedCardTarget);
	TestTrue(TEXT("Card target debug includes validation"),
		Result.ToDebugString().Contains(TEXT("ValidationReject=UnsupportedCardTarget")));
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
	TestEqual(TEXT("No validation reject reason"),
		Result.TargetValidationRejectReason,
		EWacomBattleTargetRejectReason::None);
	TestTrue(TEXT("Valid card target debug includes validation"),
		Result.ToDebugString().Contains(TEXT("ValidationReject=None")));
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
	TestEqual(TEXT("Anchor target records validation reason"),
		Result.TargetValidationRejectReason,
		EWacomBattleTargetRejectReason::UnsupportedHandAnchorTarget);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentFilterRejectedCardTargetTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.FilterRejectedCardTargetShowsInvalidFeedback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentFilterRejectedCardTargetTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* SourceCard = Fx.MakeHandCardCostModifierCard(/*Cost*/0, /*Magnitude*/2, /*bReduceCost*/false);
	SourceCard->HandCardTargetFilter.bUseExplicitHandCardTargetFilter = true;
	SourceCard->HandCardTargetFilter.bAllowNormalHandCards = false;
	SourceCard->HandCardTargetFilter.bAllowHandAnchors = true;
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
		PC->Destroy();
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
	TestEqual(TEXT("Filter-rejected card target rejects"),
		Result.IntentKind,
		EWacomBattleCardDropIntentKind::Reject);
	TestFalse(TEXT("Filter-rejected card target cannot submit"), Result.bCanSubmit);
	TestEqual(TEXT("Filter-rejected card target maps to unsupported card target"),
		Result.RejectReason,
		EWacomBattleCardDropRejectReason::UnsupportedCardTarget);
	TestEqual(TEXT("Filter rejection carries validation reason"),
		Result.TargetValidationRejectReason,
		EWacomBattleTargetRejectReason::UnsupportedNormalHandCardTarget);
	TestTrue(TEXT("Debug includes filter validation reason"),
		Result.ToDebugString().Contains(TEXT("ValidationReject=UnsupportedNormalHandCardTarget")));

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonDropIntentKeywordRejectDebugTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.DropResolverDebugIncludesKeywordRejectReason",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonDropIntentKeywordRejectDebugTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FGameplayTagContainer RequiredKeywords;
	RequiredKeywords.AddTag(WacomTags::Card_Keyword_Companion);
	FWacomBattleFixture Fx;
	UCardDefinition* SourceCard = Fx.MakeHandCardCostModifierCardWithTargetKeywordFilter(
		/*Cost*/0,
		/*Magnitude*/2,
		/*bReduceCost*/false,
		RequiredKeywords,
		FGameplayTagContainer());
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
		PC->Destroy();
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

	TestEqual(TEXT("Keyword-rejected card target rejects"),
		Result.IntentKind,
		EWacomBattleCardDropIntentKind::Reject);
	TestEqual(TEXT("Keyword rejection maps to unsupported card target"),
		Result.RejectReason,
		EWacomBattleCardDropRejectReason::UnsupportedCardTarget);
	TestEqual(TEXT("Keyword rejection carries validation reason"),
		Result.TargetValidationRejectReason,
		EWacomBattleTargetRejectReason::MissingRequiredTargetKeyword);
	TestTrue(TEXT("Debug includes keyword validation reason"),
		Result.ToDebugString().Contains(TEXT("ValidationReject=MissingRequiredTargetKeyword")));

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonLayerDraggingHandCardBuildsAffordanceTest,
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.DraggingHandCardSourceBuildsFullHandCardAffordance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonLayerDraggingHandCardBuildsAffordanceTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* SourceCard = Fx.MakeSelectedHandCardZoneMoveCard(/*Cost*/0, /*bExhaust*/true);
	UCardDefinition* NormalTargetCard = Fx.MakeNoopCard(3);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ SourceCard, NormalTargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 50, 0), 1);
	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceCardId = WacomFirstPersonCardLayerSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::HandCard);
	const FGuid NormalTargetCardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, NormalTargetCard->CardId);
	const FGuid AnchorCardId = WacomFirstPersonCardLayerSpec::FindFirstHandAnchor(Snapshot);

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
		|| !TestTrue(TEXT("Normal target exists"), NormalTargetCardId.IsValid())
		|| !TestTrue(TEXT("Anchor target exists"), AnchorCardId.IsValid()))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeBattleHUDWithCharacter(HUD, PC, Character, World);
	HUD->TakeWidget();
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerSpec::SettleBattlePresentationQueue(*HUD);
	HUD->SyncFirstPersonBattleHandLayerForTest(Snapshot);

	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	Anchor->SetBattleHandInteractionEnabled(true);
	TArray<FWacomFirstPersonCardLayerEntry> CardEntries;
	for (const FHandCardSnapshot& CardSnapshot : Snapshot.Hand.Cards)
	{
		FWacomFirstPersonCardLayerEntry Entry;
		Entry.CardInstanceId = CardSnapshot.InstanceId;
		Entry.CardViewData = WacomFirstPersonCardLayerSpec::BuildBattleCardViewDataForTest(CardSnapshot);
		Entry.bIsPlayable = CardSnapshot.bIsPlayable;
		Entry.TargetMode = CardSnapshot.Definition
			? CardSnapshot.Definition->TargetMode
			: ECardTargetMode::None;
		Entry.Zone = CardSnapshot.Zone;
		Entry.bIsHandAnchor = CardSnapshot.bIsHandAnchor;
		CardEntries.Add(MoveTemp(Entry));
	}
	Anchor->SetRuntimeCardLayerEntries(TEXT("BattleHand"), CardEntries);
	HUD->SetFirstPersonCardAnchorForTest(Anchor);
	Anchor->RefreshAnchor(0.0f);
	Anchor->PrimaryComponentTick.ExecuteTick(
		0.0f,
		LEVELTICK_All,
		ENamedThreads::GameThread,
		FGraphEventRef());
	UWacomFirstPersonCardLayerWidget* Layer = FWacomFirstPersonCardLayerTestAccess::CardLayer(*Anchor);
	if (!TestNotNull(TEXT("First-person layer"), Layer))
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = nullptr;
	UWacomFirstPersonCardLayerSlotWidget* NormalTargetWidget = nullptr;
	UWacomFirstPersonCardLayerSlotWidget* AnchorTargetWidget = nullptr;
	for (int32 Index = 0; Index < Layer->GetCardViewCount(); ++Index)
	{
		UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(Index);
		if (!SlotWidget)
		{
			continue;
		}
		const FGuid SlotCardId = SlotWidget->GetSlotView().Entry.CardInstanceId;
		if (SlotCardId == SourceCardId)
		{
			SourceWidget = SlotWidget;
		}
		else if (SlotCardId == NormalTargetCardId)
		{
			NormalTargetWidget = SlotWidget;
		}
		else if (SlotCardId == AnchorCardId)
		{
			AnchorTargetWidget = SlotWidget;
		}
	}
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Normal target slot"), NormalTargetWidget)
		|| !TestNotNull(TEXT("Anchor target slot"), AnchorTargetWidget))
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	const FVector2D SourcePosition = SourceWidget->GetVisualSlotView().ScreenPosition;
	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, SourcePosition);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, SourcePosition + FVector2D(80.0f, -20.0f));
	TestEqual(TEXT("Aiming drag records selected-source gesture state"),
		SourceWidget->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	Anchor->OnFirstPersonCardLayerDragUpdated.Broadcast(SourceCardId, FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView);

	TestEqual(TEXT("Normal hand card shows valid affordance"),
		FWacomFirstPersonCardLayerTestAccess::View(*NormalTargetWidget).DragTargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);
	TestEqual(TEXT("Normal hand card records affordance state"),
		FWacomFirstPersonCardLayerTestAccess::View(*NormalTargetWidget).CardDragTargetAffordanceFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);
	TestFalse(TEXT("Normal hand card affordance is not pointer focus"),
		FWacomFirstPersonCardLayerTestAccess::View(*NormalTargetWidget).bCardDragTargetFocusActive);
	TestEqual(TEXT("Hand anchor shows invalid affordance for selected zone move"),
		FWacomFirstPersonCardLayerTestAccess::View(*AnchorTargetWidget).DragTargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget);
	TestEqual(TEXT("Hand anchor records affordance state"),
		FWacomFirstPersonCardLayerTestAccess::View(*AnchorTargetWidget).CardDragTargetAffordanceFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget);
	TestFalse(TEXT("Hand anchor affordance is not pointer focus"),
		FWacomFirstPersonCardLayerTestAccess::View(*AnchorTargetWidget).bCardDragTargetFocusActive);
	TestTrue(TEXT("Debug reports affordance counts"), Layer->GetDragTargetDebugSummary().Contains(TEXT("AffordanceValid=")));

	Layer->CancelCardDragGesture(true);
	TestEqual(TEXT("Normal affordance clears on cancel"),
		FWacomFirstPersonCardLayerTestAccess::View(*NormalTargetWidget).DragTargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::None);

	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonLayerKeywordFilterAffordanceTest,
	"Wacom.UI.FirstPersonCardLayer.DragTargetFeedback.FullHandAffordanceUsesKeywordFilter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonLayerKeywordFilterAffordanceTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FGameplayTagContainer RequiredKeywords;
	RequiredKeywords.AddTag(WacomTags::Card_Keyword_Companion);
	FWacomBattleFixture Fx;
	UCardDefinition* SourceCard = Fx.MakeHandCardCostModifierCardWithTargetKeywordFilter(
		/*Cost*/0,
		/*Magnitude*/1,
		/*bReduceCost*/false,
		RequiredKeywords,
		FGameplayTagContainer());
	UCardDefinition* CompanionTargetCard = Fx.MakeDamageCardWithKeywords(
		/*Cost*/3,
		/*Damage*/1,
		{ WacomTags::Card_Keyword_Companion });
	UCardDefinition* PlainTargetCard = Fx.MakeNoopCard(3);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ SourceCard, CompanionTargetCard, PlainTargetCard, Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Fx.MakeSinglePartEnemy(20, 50, 0), 1);
	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceCardId = WacomFirstPersonCardLayerSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::HandCard);
	const FGuid CompanionTargetCardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, CompanionTargetCard->CardId);
	const FGuid PlainTargetCardId = FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, PlainTargetCard->CardId);

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
		|| !TestTrue(TEXT("Companion target exists"), CompanionTargetCardId.IsValid())
		|| !TestTrue(TEXT("Plain target exists"), PlainTargetCardId.IsValid()))
	{
		return false;
	}

	WacomFirstPersonCardLayerSpec::PrimeBattleHUDWithCharacter(HUD, PC, Character, World);
	HUD->TakeWidget();
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerSpec::SettleBattlePresentationQueue(*HUD);
	HUD->SyncFirstPersonBattleHandLayerForTest(Snapshot);

	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor = WacomFirstPersonCardLayerSpec::AddProbe(Character);
	if (!TestNotNull(TEXT("Anchor probe"), Anchor))
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	Anchor->SetBattleHandInteractionEnabled(true);
	TArray<FWacomFirstPersonCardLayerEntry> CardEntries;
	for (const FHandCardSnapshot& CardSnapshot : Snapshot.Hand.Cards)
	{
		FWacomFirstPersonCardLayerEntry Entry;
		Entry.CardInstanceId = CardSnapshot.InstanceId;
		Entry.CardViewData = WacomFirstPersonCardLayerSpec::BuildBattleCardViewDataForTest(CardSnapshot);
		Entry.bIsPlayable = CardSnapshot.bIsPlayable;
		Entry.TargetMode = CardSnapshot.Definition
			? CardSnapshot.Definition->TargetMode
			: ECardTargetMode::None;
		Entry.Zone = CardSnapshot.Zone;
		Entry.bIsHandAnchor = CardSnapshot.bIsHandAnchor;
		CardEntries.Add(MoveTemp(Entry));
	}
	Anchor->SetRuntimeCardLayerEntries(TEXT("BattleHand"), CardEntries);
	HUD->SetFirstPersonCardAnchorForTest(Anchor);
	Anchor->RefreshAnchor(0.0f);
	Anchor->PrimaryComponentTick.ExecuteTick(
		0.0f,
		LEVELTICK_All,
		ENamedThreads::GameThread,
		FGraphEventRef());
	UWacomFirstPersonCardLayerWidget* Layer = FWacomFirstPersonCardLayerTestAccess::CardLayer(*Anchor);
	if (!TestNotNull(TEXT("First-person layer"), Layer))
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	UWacomFirstPersonCardLayerSlotWidget* SourceWidget = nullptr;
	UWacomFirstPersonCardLayerSlotWidget* CompanionTargetWidget = nullptr;
	UWacomFirstPersonCardLayerSlotWidget* PlainTargetWidget = nullptr;
	for (int32 Index = 0; Index < Layer->GetCardViewCount(); ++Index)
	{
		UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(Index);
		if (!SlotWidget)
		{
			continue;
		}
		const FGuid SlotCardId = SlotWidget->GetSlotView().Entry.CardInstanceId;
		if (SlotCardId == SourceCardId)
		{
			SourceWidget = SlotWidget;
		}
		else if (SlotCardId == CompanionTargetCardId)
		{
			CompanionTargetWidget = SlotWidget;
		}
		else if (SlotCardId == PlainTargetCardId)
		{
			PlainTargetWidget = SlotWidget;
		}
	}
	if (!TestNotNull(TEXT("Source slot"), SourceWidget)
		|| !TestNotNull(TEXT("Companion target slot"), CompanionTargetWidget)
		|| !TestNotNull(TEXT("Plain target slot"), PlainTargetWidget))
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	const FVector2D SourcePosition = SourceWidget->GetVisualSlotView().ScreenPosition;
	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, SourcePosition);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, SourcePosition + FVector2D(80.0f, -20.0f));
	Anchor->OnFirstPersonCardLayerDragUpdated.Broadcast(SourceCardId, FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView);

	TestEqual(TEXT("Keyword-allowed card shows valid card feedback"),
		FWacomFirstPersonCardLayerTestAccess::View(*CompanionTargetWidget).DragTargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);
	TestEqual(TEXT("Keyword-rejected card shows invalid card feedback"),
		FWacomFirstPersonCardLayerTestAccess::View(*PlainTargetWidget).DragTargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::InvalidCardTarget);

	Layer->CancelCardDragGesture(true);
	Character->Destroy();
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
	"Wacom.UI.FirstPersonCardLayer.DropIntentResolver.PhaseBlockedOrMissingSessionRejects",
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
	HUD->SetUIStateForTest(EBattleUIState::BattleEnd);
	FWacomBattleCardDropResolveResult UIBlocked = HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	TestEqual(TEXT("BattleEnd UI rejects"), UIBlocked.IntentKind, EWacomBattleCardDropIntentKind::Reject);
	TestEqual(TEXT("BattleEnd reject reason"), UIBlocked.RejectReason, EWacomBattleCardDropRejectReason::UIBlocked);

	HUD->SetUIStateForTest(EBattleUIState::Idle);
	FBattleEvent Event;
	Event.Type = EBattleEventType::DamageDealt;
	Event.Sequence = 1;
	Event.ActorEnemyPartKey = FWacomBattleFixture::FindPartKey(Snapshot, 0);
	Event.Amount = 1;
	HUD->EnqueueBattlePresentationEventsForTest({ Event });
	World->GetTimerManager().Tick(0.01f);
	TestTrue(TEXT("Presentation queue is busy"), HUD->IsBattlePresentationBusy());
	FWacomBattleCardDropResolveResult PresentationBusy = HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	TestEqual(TEXT("Presentation busy no longer blocks drop intent"),
		PresentationBusy.IntentKind,
		EWacomBattleCardDropIntentKind::PlayCardNoTarget);
	TestTrue(TEXT("Presentation busy drop can submit"), PresentationBusy.bCanSubmit);

	HUD->HandleFirstPersonCardDragReleasedForTest(CardId, DragView);
	TestTrue(TEXT("PlayCard creates presentation stack"), HUD->GetPresentationStackEntryCountForTest() > 0);
	HUD->OnWaitRequested();
	TestTrue(TEXT("Wait while stack pending creates turn-boundary barrier"), HUD->HasPendingTurnBoundaryCommandForTest());
	FWacomBattleCardDropResolveResult PendingBarrier = HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	TestEqual(TEXT("Pending turn boundary blocks first-person drop"),
		PendingBarrier.IntentKind,
		EWacomBattleCardDropIntentKind::Reject);
	TestEqual(TEXT("Pending turn boundary reject reason"),
		PendingBarrier.RejectReason,
		EWacomBattleCardDropRejectReason::UIBlocked);

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
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 0, 0);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), { Fx.MakeSimpleDamageCard(0, 1) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomFirstPersonCardLayerSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::SingleEnemyPart);
	const FGuid PartId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);

	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity);
	WacomFirstPersonCardLayerSpec::FSceneEnemyHostActors SceneEnemy =
		WacomFirstPersonCardLayerSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Solo") });
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestTrue(TEXT("Scene enemy part exists"), SceneEnemy.Parts.Num() > 0)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Card exists"), CardId.IsValid())
		|| !TestTrue(TEXT("Part exists"), PartId.IsValid()))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomFirstPersonCardLayerSpec::DestroySceneEnemyHost(SceneEnemy);
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });
	WacomFirstPersonCardLayerSpec::SettleBattlePresentationQueue(*HUD);
	FWacomBattleSceneTargetClickTestAccess::SetHUD(PC, HUD);
	FWacomBattleSceneTargetClickTestAccess::SetHit(PC, SceneEnemy.Parts[0], SceneEnemy.Parts[0]->GetHitBounds());

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
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 0, 0);
	UCharacterDefinition* CharacterDefinition = Fx.MakeCharacter(
		Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), { Fx.MakeSimpleDamageCard(0, 1) });
	UBattleSession* Session = Fx.CreateSession(CharacterDefinition, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomFirstPersonCardLayerSpec::FindFirstHandCardByTargetMode(Snapshot, ECardTargetMode::SingleEnemyPart);
	const FGuid PartId = FWacomBattleFixture::FindPartInstanceId(Snapshot, 0);

	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity);
	WacomFirstPersonCardLayerSpec::FSceneEnemyHostActors CurrentHost =
		WacomFirstPersonCardLayerSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Solo") });
	WacomFirstPersonCardLayerSpec::FSceneEnemyHostActors OtherHost =
		WacomFirstPersonCardLayerSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Solo") });
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Current host"), CurrentHost.Host)
		|| !TestTrue(TEXT("Current host part exists"), CurrentHost.Parts.Num() > 0)
		|| !TestNotNull(TEXT("Other host"), OtherHost.Host)
		|| !TestTrue(TEXT("Other host part exists"), OtherHost.Parts.Num() > 0)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Card exists"), CardId.IsValid())
		|| !TestTrue(TEXT("Part exists"), PartId.IsValid()))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomFirstPersonCardLayerSpec::DestroySceneEnemyHost(OtherHost);
		WacomFirstPersonCardLayerSpec::DestroySceneEnemyHost(CurrentHost);
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ CurrentHost.Host });
	WacomFirstPersonCardLayerSpec::SettleBattlePresentationQueue(*HUD);
	FWacomBattleSceneTargetClickTestAccess::SetHUD(PC, HUD);
	OtherHost.Parts[0]->GetInteractionTargetComponent()->SetTargetId(PartId);
	FWacomBattleSceneTargetClickTestAccess::SetHit(PC, OtherHost.Parts[0], OtherHost.Parts[0]->GetHitBounds());

	const FWacomFirstPersonCardDragView DragView = WacomFirstPersonCardLayerSpec::MakeDropDragView(
		CardId,
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	const FWacomBattleCardDropResolveResult Result = HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	TestEqual(TEXT("Invalid world target rejects"), Result.IntentKind, EWacomBattleCardDropIntentKind::Reject);
	TestEqual(TEXT("Invalid world reason"), Result.RejectReason, EWacomBattleCardDropRejectReason::InvalidWorldTarget);
	TestFalse(TEXT("Invalid world target cannot submit"), Result.bCanSubmit);

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
	HUD->SyncFirstPersonBattleHandLayerForTest(Snapshot);

	TArray<FWacomFirstPersonCardLayerEntry> CardEntries;
	CardEntries.Reserve(Snapshot.Hand.Cards.Num());
	for (const FHandCardSnapshot& CardSnapshot : Snapshot.Hand.Cards)
	{
		FWacomFirstPersonCardLayerEntry Entry;
		Entry.CardInstanceId = CardSnapshot.InstanceId;
		Entry.CardViewData = WacomFirstPersonCardLayerSpec::BuildBattleCardViewDataForTest(CardSnapshot);
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

	Anchor->SetBattleHandInteractionEnabled(true);
	Anchor->SetRuntimeCardLayerEntries(TEXT("BattleHand"), CardEntries);
	HUD->SetFirstPersonCardAnchorForTest(Anchor);
	Anchor->RefreshAnchor(0.0f);
	Anchor->PrimaryComponentTick.ExecuteTick(
		0.0f,
		LEVELTICK_All,
		ENamedThreads::GameThread,
		FGraphEventRef());
	UWacomFirstPersonCardLayerWidget* Layer = FWacomFirstPersonCardLayerTestAccess::CardLayer(*Anchor);
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

	FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(*SourceWidget, SourcePosition);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(*SourceWidget, 0.01f, TargetPosition);
	TestEqual(TEXT("Layer gesture resolves pointer card target"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.CurrentTarget.CardInstanceId,
		TargetCardId);
	Anchor->OnFirstPersonCardLayerDragUpdated.Broadcast(SourceCardId, FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView);
	TestEqual(TEXT("HUD marks card target as valid before release"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).CurrentDragView.TargetFeedbackState,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidCardTarget);

	FWacomFirstPersonCardLayerTestAccess::RequestGestureRelease(*SourceWidget, TargetPosition);
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
	TestTrue(TEXT("Source release used confirm feedback"), FWacomFirstPersonCardLayerTestAccess::View(*SourceWidget).bConfirmFeedbackActive);
	TestFalse(TEXT("Source release did not play deny feedback"), FWacomFirstPersonCardLayerTestAccess::View(*SourceWidget).bDenyFeedbackActive);

	Character->Destroy();
	PC->Destroy();
	return true;
}
