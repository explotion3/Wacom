// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomRunPathSegmentActor.h"
#include "Actors/WacomBattleEnemyActor.h"
#include "Components/WacomBattleEnemyPartComponent.h"
#include "Components/SplineComponent.h"
#include "Components/WacomBattleCameraLookComponent.h"
#include "Components/WacomCursorLookDriverComponent.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Components/WacomFirstPersonViewStageBlendComponent.h"
#include "Components/WacomRunPathTraversalComponent.h"
#include "UI/RunPathTraversalTestAccess.h"
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
#include "UI/BattleSceneTargetClickTestAccess.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "UI/Card/WacomCardPresentationBuilder.h"
#include "UI/Card/WacomFirstPersonCardLayerSourceIds.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/Card/WacomFirstPersonCardViewWidget.h"
#include "UI/Card/WacomCardView.h"
#include "UI/CardViewTestAccess.h"
#include "UI/FirstPersonCardLayerTestAccess.h"
#include "UI/FirstPersonCardLayerInteractionSpecFixture.h"
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

	AWacomRunPathSegmentActor* SpawnTestSegment(UWorld& World, const FVector& Start, const FVector& End)
	{
		AWacomRunPathSegmentActor* Segment = World.SpawnActor<AWacomRunPathSegmentActor>(
			AWacomRunPathSegmentActor::StaticClass(),
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

	UCardDefinition* MakeFixtureCard(UObject* Outer, const TCHAR* Name, int32 Cost)
	{
		UCardDefinition* Card = NewObject<UCardDefinition>(Outer);
		if (!Card)
		{
			return nullptr;
		}

		Card->CardId = FName(Name);
		Card->DisplayName = FText::FromString(Name);
		Card->Description = FText::FromString(TEXT("First-person card test fixture"));
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

	EWacomFirstPersonCardInteractionIntent BattleInteractionIntentForTargetModeProjection(
		ECardTargetMode TargetMode)
	{
		switch (TargetMode)
		{
		case ECardTargetMode::SingleEnemyPart:
			return EWacomFirstPersonCardInteractionIntent::AimWorldTarget;
		case ECardTargetMode::HandCard:
			return EWacomFirstPersonCardInteractionIntent::AimCardTarget;
		case ECardTargetMode::None:
		case ECardTargetMode::Self:
		case ECardTargetMode::AllEnemyParts:
		default:
			return EWacomFirstPersonCardInteractionIntent::DragToDropTarget;
		}
	}

	void SetEntryInteractionIntent(
		FWacomFirstPersonCardLayerEntry& Entry,
		EWacomFirstPersonCardInteractionIntent InteractionIntent)
	{
		Entry.InteractionIntent = InteractionIntent;
	}

	void SetSlotInteractionIntent(
		FWacomFirstPersonCardLayerSlotView& Slot,
		EWacomFirstPersonCardInteractionIntent InteractionIntent)
	{
		SetEntryInteractionIntent(Slot.Entry, InteractionIntent);
	}

	void SetEntryBattleTargetModeProjection(
		FWacomFirstPersonCardLayerEntry& Entry,
		ECardTargetMode TargetMode)
	{
		SetEntryInteractionIntent(
			Entry,
			BattleInteractionIntentForTargetModeProjection(TargetMode));
	}

	FWacomFirstPersonCardLayerSlotView MakeProjectedInteractionSlot(
		const FGuid& CardInstanceId,
		bool bPlayable,
		bool bProjected)
	{
		FWacomFirstPersonCardLayerSlotView Slot;
		Slot.Index = 0;
		Slot.Entry.CardInstanceId = CardInstanceId;
		Slot.Entry.bIsPlayable = bPlayable;
		Slot.Entry.CardViewData.bDisabled = !bPlayable;
		SetSlotInteractionIntent(
			Slot,
			EWacomFirstPersonCardInteractionIntent::DragToDropTarget);
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
		float Angle,
		float Scale,
		float Opacity)
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

	FWacomFirstPersonCardInteractionFeedbackConfig MakeTestFeedbackConfig()
	{
		FWacomFirstPersonCardInteractionFeedbackConfig Config;
		Config.bEnabled = true;
		Config.PressedScale = 0.9f;
		Config.PressedTranslationYPixels = 2.0f;
		Config.PressedInDurationSeconds = 0.045f;
		Config.PressedOutDurationSeconds = 0.08f;
		Config.PressedContactShadowLiftMultiplier = 0.35f;
		Config.DenyDuration = 0.2f;
		Config.DenyShakePixels = 8.0f;
		Config.DenyColor = FLinearColor::Red;
		Config.DenyOpacity = 0.5f;
		Config.DenyCornerInsetPixels = 8.0f;
		Config.DenyCornerLengthPixels = 14.0f;
		Config.DenyCornerThicknessPixels = 3.0f;
		Config.bEnablePlayCommitFeedback = true;
		Config.PlayCommitDuration = 0.12f;
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

	struct FSceneEnemyHostActors
	{
		AWacomBattleEnemyActor* Host = nullptr;
		TArray<UWacomBattleEnemyPartComponent*> Parts;
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
			UWacomBattleEnemyPartComponent* PartComponent =
				NewObject<UWacomBattleEnemyPartComponent>(
					Result.Host,
					*FString::Printf(TEXT("LayerTestPart_%d"), Index),
					RF_Transient);
			if (!PartComponent)
			{
				continue;
			}

			Result.Host->AddInstanceComponent(PartComponent);
			PartComponent->SetupAttachment(Result.Host->GetRootComponent());
			PartComponent->SetRelativeLocation(
				FVector(100.f * static_cast<float>(Index + 1), 0.f, 0.f));
			PartComponent->SetBoxExtent(FVector(40.f));
			PartComponent->SetDerivedPartId(PartIds[Index]);
			PartComponent->PartSlotId = PartIds[Index];
			PartComponent->RegisterComponent();
			Result.Parts.Add(PartComponent);
		}

		Result.Host->NotifyEnemySceneComponentTopologyChanged();
		return Result;
	}

	void DestroySceneEnemyHost(FSceneEnemyHostActors& Actors)
	{
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
	const FWacomFirstPersonCardAnchorAutomationTestView View = FWacomFirstPersonCardLayerTestAccess::View(*Anchor);
	TestTrue(TEXT("Fallback anchor is valid"), View.bHasValidAnchor);
	TestEqual(TEXT("No active run/battle uses camera fallback"), View.Mode, EWacomFirstPersonCardAnchorMode::CameraFallback);
	TestEqual(TEXT("Fallback reason is recorded"), View.LastFallbackReason, FName(TEXT("CameraFallback")));

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerRunPathAnchorTest,
	"Wacom.UI.FirstPersonCardLayer.Anchor.RunPathAnchorUsesSplineBase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerRunPathAnchorTest::RunTest(const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	AWacomRunPathSegmentActor* Segment = WacomFirstPersonCardLayerSpec::SpawnTestSegment(
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

	UWacomRunPathTraversalComponent* RunPath = Character->GetRunPathTraversalComponent();
	TestTrue(TEXT("Run Path activates"), RunPath && FWacomRunPathTraversalTestAccess::BeginTraversalAtDistance(*RunPath, Segment, 200.0f));
	Anchor->RefreshAnchor(0.0f);
	const FWacomFirstPersonCardAnchorAutomationTestView View = FWacomFirstPersonCardLayerTestAccess::View(*Anchor);
	TestTrue(TEXT("Run Path anchor is valid"), View.bHasValidAnchor);
	TestEqual(TEXT("Run Path mode is selected"), View.Mode, EWacomFirstPersonCardAnchorMode::RunPath);
	TestEqual(TEXT("Run Path anchor uses spline distance before layout offset"), Character->GetRunPathTraversalComponent()->GetDistanceAlongSpline(), 200.0f);
	TestEqual(TEXT("Run Path default projection is body locked"), View.ProjectionMode, EWacomFirstPersonCardProjectionMode::BodyLocked);
	TestFalse(TEXT("Run Path body locked layout ignores cursor look"), View.bLookOffsetAppliedToLayout);

	Anchor->DestroyComponent();
	Segment->Destroy();
	Character->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerSuspendedRunPathFallbackAnchorTest,
	"Wacom.UI.FirstPersonCardLayer.Anchor.SuspendedRunPathDoesNotOwnAnchor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerSuspendedRunPathFallbackAnchorTest::RunTest(const FString& Parameters)
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
	AWacomRunPathSegmentActor* Segment = WacomFirstPersonCardLayerSpec::SpawnTestSegment(
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
	UWacomRunPathTraversalComponent* RunPath = Character->GetRunPathTraversalComponent();
	TestTrue(TEXT("Run Path activates"), RunPath && FWacomRunPathTraversalTestAccess::BeginTraversalAtDistance(*RunPath, Segment, 200.0f));
	Character->SetExplorationInputEnabled(false);
	TestTrue(TEXT("Run Path is suspended"), RunPath && RunPath->GetTraversalState() == EWacomRunPathTraversalState::Suspended);

	Anchor->ProbeCameraTransform = FTransform(
		FRotator(4.0f, 33.0f, 0.0f),
		FVector(500.0f, 600.0f, 700.0f),
		FVector::OneVector);
	Anchor->RefreshAnchor(0.0f);
	const FWacomFirstPersonCardAnchorAutomationTestView View =
		FWacomFirstPersonCardLayerTestAccess::View(*Anchor);
	TestTrue(TEXT("Suspended Run Path keeps anchor valid through fallback"), View.bHasValidAnchor);
	TestEqual(TEXT("Suspended Run Path does not own anchor mode"),
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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*SlotWidget, Config);
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
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*Layer, DragConfig);
	Layer->SetCardViewClass(UWacomFirstPersonCardLayerBleedFirstPersonCardViewProbe::StaticClass());
	Layer->SetCardLayerInteractionEnabled(true);

	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid(), true, true);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(Slot, EWacomFirstPersonCardInteractionIntent::DragToDropTarget);
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
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*Layer, DragConfig);
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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);
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
	AWacomRunPathSegmentActor* Segment = WacomFirstPersonCardLayerSpec::SpawnTestSegment(
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
	TestTrue(TEXT("Run Path activates"), FWacomRunPathTraversalTestAccess::BeginTraversalAtDistance(*Character->GetRunPathTraversalComponent(), Segment, 100.0f));
	Character->SetExplorationInputEnabled(false);
	PC->SetControlRotation(FRotator(2.0f, 55.0f, 0.0f));
	TestTrue(TEXT("Battle camera activates"), Character->GetBattleCameraLookComponent()->ActivateBattleCameraLook());
	Anchor->RefreshAnchor(0.0f);
	const FWacomFirstPersonCardAnchorAutomationTestView View = FWacomFirstPersonCardLayerTestAccess::View(*Anchor);
	TestEqual(TEXT("Battle camera mode takes priority over suspended Run Path"), View.Mode, EWacomFirstPersonCardAnchorMode::BattleCamera);
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
	const FWacomFirstPersonCardAnchorAutomationTestView View = FWacomFirstPersonCardLayerTestAccess::View(*Anchor);
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
	Anchor->LayoutFixtureCardCount = 5;
	Anchor->HandMaxEdgeDropPixels = 0.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	Anchor->bUseCameraTransformProjection = true;
	Anchor->ProjectionMode = EWacomFirstPersonCardProjectionMode::BodyLocked;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	Anchor->ProbeCameraTransform = FTransform(
		FRotator::ZeroRotator,
		Anchor->ProbeCameraTransform.GetLocation(),
		FVector::OneVector);
	const TArray<FWacomFirstPersonCardLayerSlotView> InitialSlots = Anchor->BuildLayoutFixtureCardSlotViews();

	Anchor->ProbeCameraTransform = FTransform(
		FRotator(0.0f, 8.0f, 0.0f),
		Anchor->ProbeCameraTransform.GetLocation(),
		FVector::OneVector);
	const TArray<FWacomFirstPersonCardLayerSlotView> RotatedCameraSlots = Anchor->BuildLayoutFixtureCardSlotViews();

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

	Anchor->LayoutFixtureCardCount = 5;
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
	Anchor->LayoutFixtureCardCount = 5;
	Anchor->HandMaxEdgeDropPixels = 0.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	Anchor->ProjectionMode = EWacomFirstPersonCardProjectionMode::LegacyWorldProjected;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	const FTransform InitialCenterTransform = Anchor->ComputeCardTransform(5, 2);
	const TArray<FWacomFirstPersonCardLayerSlotView> InitialSlots = Anchor->BuildLayoutFixtureCardSlotViews();

	Character->GetCursorLookDriverComponent()->UpdateFromNormalizedCursor(
		FVector2D(1.0f, -1.0f),
		0.0f,
		20.0f,
		10.0f);
	Anchor->RefreshAnchor(0.0f);
	const FTransform LookCenterTransform = Anchor->ComputeCardTransform(5, 2);
	const TArray<FWacomFirstPersonCardLayerSlotView> LookSlots = Anchor->BuildLayoutFixtureCardSlotViews();

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
	"Wacom.UI.FirstPersonCardLayer.LayoutFixture.CreatesCardViewsFromPlaceholderData",
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

	Anchor->LayoutFixtureCardCount = 5;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildLayoutFixtureCardSlotViews();
	TestEqual(TEXT("Layout fixture creates five placeholder slots"), Slots.Num(), 5);
	if (Slots.Num() > 0)
	{
		TestEqual(TEXT("Layout fixture card has placeholder name"), Slots[0].Entry.CardViewData.Name.ToString(), FString(TEXT("Layout Card 1")));
		TestTrue(TEXT("Layout fixture slot is projected"), Slots[0].bProjected);
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
	"Wacom.UI.FirstPersonCardLayer.LayoutFixture.BuildsCardViewsFromDefinitions",
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
	UCardDefinition* FirstCard = WacomFirstPersonCardLayerSpec::MakeFixtureCard(GetTransientPackage(), TEXT("Preview.Alpha"), 2);
	UCardDefinition* SecondCard = WacomFirstPersonCardLayerSpec::MakeFixtureCard(GetTransientPackage(), TEXT("Preview.Beta"), 4);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("Anchor probe"), Anchor)
		|| !TestNotNull(TEXT("First preview card"), FirstCard)
		|| !TestNotNull(TEXT("Second preview card"), SecondCard))
	{
		return false;
	}

	Anchor->LayoutFixtureCardDefinitions = {
		TSoftObjectPtr<UCardDefinition>(FirstCard),
		TSoftObjectPtr<UCardDefinition>(SecondCard)
	};
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildLayoutFixtureCardSlotViews();
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

	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildLayoutFixtureCardSlotViews();
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
	Anchor->LayoutFixtureCardCount = 1;
	Anchor->HandMaxEdgeDropPixels = 0.0f;
	Anchor->ProjectionMode = EWacomFirstPersonCardProjectionMode::LegacyWorldProjected;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildLayoutFixtureCardSlotViews();
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
	Anchor->LayoutFixtureCardCount = 1;
	Anchor->HandMaxEdgeDropPixels = 0.0f;
	Anchor->ProjectionMode = EWacomFirstPersonCardProjectionMode::LegacyWorldProjected;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildLayoutFixtureCardSlotViews();
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

	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildLayoutFixtureCardSlotViews();
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

	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildLayoutFixtureCardSlotViews();
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
	FWacomFirstPersonCardLayerStaticSlotOrderTest,
	"Wacom.UI.FirstPersonCardLayer.LayoutFixture.ProjectedSlotsStayOrdered",
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

	Anchor->LayoutFixtureCardCount = 5;
	Anchor->HandMaxEdgeDropPixels = 80.0f;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildLayoutFixtureCardSlotViews();
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
	Anchor->LayoutFixtureCardCount = 1;
	Anchor->HandMaxEdgeDropPixels = 0.0f;
	Anchor->AuthoredHandScreenOffset = FVector2D::ZeroVector;
	Anchor->HorizontalOffset = 1236.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildLayoutFixtureCardSlotViews();
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
	Anchor->LayoutFixtureCardCount = 5;
	Anchor->HandMaxEdgeDropPixels = 0.0f;
	Anchor->AuthoredCardSpacingPixels = 100.0f;
	Anchor->AuthoredMaxHandWidthPixels = 0.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const TArray<FWacomFirstPersonCardLayerSlotView> InitialSlots = Anchor->BuildLayoutFixtureCardSlotViews();
	Anchor->CardSpacing = 360.0f;
	const TArray<FWacomFirstPersonCardLayerSlotView> ChangedLegacySpacingSlots = Anchor->BuildLayoutFixtureCardSlotViews();

	TestEqual(TEXT("Initial slot count"), InitialSlots.Num(), 5);
	TestEqual(TEXT("Changed spacing slot count"), ChangedLegacySpacingSlots.Num(), 5);
	if (InitialSlots.Num() == 5 && ChangedLegacySpacingSlots.Num() == 5)
	{
		TestEqual(TEXT("Left slot uses projected hand center"), InitialSlots[0].AnchorWidgetPosition, InitialSlots[2].AnchorWidgetPosition);
		TestEqual(TEXT("Right slot uses projected hand center"), InitialSlots[4].AnchorWidgetPosition, InitialSlots[2].AnchorWidgetPosition);
		TestEqual(TEXT("Old 3D CardSpacing does not affect authored screen position"), ChangedLegacySpacingSlots[4].ScreenPosition, InitialSlots[4].ScreenPosition);
		TestTrue(TEXT("Authored spacing controls screen offset"),
			FMath::IsNearlyEqual(
				InitialSlots[4].ScreenPosition.X - InitialSlots[2].ScreenPosition.X,
				200.0f * InitialSlots[2].PresentationScale,
				0.001f));
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerRunPathTickPrerequisiteTest,
	"Wacom.UI.FirstPersonCardLayer.AnchorMotionStability.AnchorTicksAfterRunPathMovement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerRunPathTickPrerequisiteTest::RunTest(const FString& Parameters)
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
		|| !TestNotNull(TEXT("Run Path component"), Character ? Character->GetRunPathTraversalComponent() : nullptr))
	{
		return false;
	}

	Anchor->BeginPlayForTest();
	const UWacomRunPathTraversalComponent* RunPath = Character->GetRunPathTraversalComponent();
	TestTrue(
		TEXT("Anchor tick has RunPath movement prerequisite"),
		WacomFirstPersonCardLayerSpec::HasTickPrerequisite(
			Anchor->PrimaryComponentTick,
			RunPath,
			RunPath->PrimaryComponentTick));

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
	Anchor->LayoutFixtureCardCount = 1;
	Anchor->HandMaxEdgeDropPixels = 0.0f;
	Anchor->AuthoredHandScreenOffset = FVector2D::ZeroVector;
	Anchor->bEnableCardLayerPixelSnapping = false;
	Anchor->bEnableAnchorScreenSmoothing = true;
	Anchor->AnchorScreenSmoothingSpeed = 1.0f;
	Anchor->AnchorScreenSmoothingResetDistancePixels = 320.0f;
	Anchor->ProbeAnchorSmoothingDeltaTime = 1.0f / 60.0f;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const TArray<FWacomFirstPersonCardLayerSlotView> InitialSlots = Anchor->BuildLayoutFixtureCardSlotViews();
	Anchor->HorizontalOffset = 60.0f;
	Anchor->RefreshAnchor(0.0f);
	const TArray<FWacomFirstPersonCardLayerSlotView> SmoothedSlots = Anchor->BuildLayoutFixtureCardSlotViews();

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
	Anchor->LayoutFixtureCardCount = 5;
	Anchor->HandMaxEdgeDropPixels = 80.0f;
	Anchor->AuthoredCardSpacingPixels = 100.0f;
	Anchor->AuthoredMaxHandWidthPixels = 0.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	Anchor->bEnableAnchorScreenSmoothing = true;
	Anchor->AnchorScreenSmoothingSpeed = 1.0f;
	Anchor->ProbeAnchorSmoothingDeltaTime = 1.0f / 60.0f;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const TArray<FWacomFirstPersonCardLayerSlotView> BaseSlots = Anchor->BuildLayoutFixtureCardSlotViews();
	Anchor->HorizontalOffset = 40.0f;
	Anchor->RefreshAnchor(0.0f);
	const TArray<FWacomFirstPersonCardLayerSlotView> MovedSlots = Anchor->BuildLayoutFixtureCardSlotViews();

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
	Anchor->LayoutFixtureCardCount = 1;
	Anchor->HandMaxEdgeDropPixels = 0.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	Anchor->bEnableAnchorScreenSmoothing = true;
	Anchor->AnchorScreenSmoothingSpeed = 1.0f;
	Anchor->AnchorScreenSmoothingResetDistancePixels = 80.0f;
	Anchor->ProbeAnchorSmoothingDeltaTime = 1.0f / 60.0f;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	Anchor->BuildLayoutFixtureCardSlotViews();
	Anchor->HorizontalOffset = 200.0f;
	Anchor->RefreshAnchor(0.0f);
	const TArray<FWacomFirstPersonCardLayerSlotView> JumpSlots = Anchor->BuildLayoutFixtureCardSlotViews();

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

	Anchor->LayoutFixtureCardCount = 1;
	Anchor->bEnableAnchorScreenSmoothing = true;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);
	Anchor->BuildLayoutFixtureCardSlotViews();
	Anchor->bProjectionSucceeds = false;
	const TArray<FWacomFirstPersonCardLayerSlotView> FailedSlots = Anchor->BuildLayoutFixtureCardSlotViews();

	TestEqual(TEXT("Projection failure still builds slot"), FailedSlots.Num(), 1);
	if (FailedSlots.Num() == 1)
	{
		TestFalse(TEXT("Projection failure hides slot"), FailedSlots[0].bProjected);
		TestFalse(TEXT("Projection failure is not smoothed"), FailedSlots[0].bAnchorScreenSmoothed);
	}

	Anchor->bProjectionSucceeds = true;
	Anchor->HorizontalOffset = 40.0f;
	Anchor->RefreshAnchor(0.0f);
	const TArray<FWacomFirstPersonCardLayerSlotView> RecoveredSlots = Anchor->BuildLayoutFixtureCardSlotViews();
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
	Anchor->LayoutFixtureCardCount = 7;
	Anchor->AuthoredCardSpacingPixels = 160.0f;
	Anchor->AuthoredMaxHandWidthPixels = 480.0f;
	Anchor->HandMaxEdgeDropPixels = 0.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildLayoutFixtureCardSlotViews();
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
			FMath::IsNearlyEqual(
				Slots.Last().ScreenPosition.X - Slots[0].ScreenPosition.X,
				480.0f * Slots[0].PresentationScale,
				0.001f));
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
		FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerEntries(*Anchor, TEXT("EdgeDropScale"), Entries);
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
			FMath::IsNearlyEqual(FiveSlots[0].AuthoredLayoutOffset.Y, 64.0f * FiveSlots[0].PresentationScale, 0.001f));
		TestTrue(TEXT("Eight-card edge uses smooth interpolated drop"),
			FMath::IsNearlyEqual(EightSlots[0].AuthoredLayoutOffset.Y, ExpectedEightDrop * EightSlots[0].PresentationScale, 0.001f));
		TestTrue(TEXT("Twelve-card edge uses max drop"),
			FMath::IsNearlyEqual(TwelveSlots[0].AuthoredLayoutOffset.Y, 110.0f * TwelveSlots[0].PresentationScale, 0.001f));
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
			FMath::IsNearlyEqual(
				UnscaledFiveSlots[0].AuthoredLayoutOffset.Y,
				110.0f * UnscaledFiveSlots[0].PresentationScale,
				0.001f));
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

	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerEntries(*Anchor, WacomFirstPersonCardLayerSourceIds::BattleHand(), { LeftAnchor, NormalA, NormalB, NormalC, RightAnchor });
	const TArray<FWacomFirstPersonCardLayerSlotView> AnchorEdgeSlots =
		Anchor->BuildActiveCardLayerSlotViewsForTest();

	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerEntries(*Anchor, WacomFirstPersonCardLayerSourceIds::BattleHand(), { LeftNormal, NormalA, NormalB, NormalC, RightNormal });
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
			FMath::IsNearlyEqual(AnchorEdgeSlots[0].AuthoredLayoutOffset.Y, 100.0f * AnchorEdgeSlots[0].PresentationScale, 0.001f));
		TestTrue(TEXT("Right hand anchor uses normal edge drop"),
			FMath::IsNearlyEqual(AnchorEdgeSlots[4].AuthoredLayoutOffset.Y, 100.0f * AnchorEdgeSlots[4].PresentationScale, 0.001f));
		TestTrue(TEXT("Normal edge card applies same edge drop"),
			FMath::IsNearlyEqual(NormalEdgeSlots[4].AuthoredLayoutOffset.Y, 100.0f * NormalEdgeSlots[4].PresentationScale, 0.001f));
		TestEqual(TEXT("Left hand anchor matches normal slot position"),
			AnchorEdgeSlots[0].AuthoredLayoutOffset,
			NormalEdgeSlots[0].AuthoredLayoutOffset);
		TestEqual(TEXT("Right hand anchor matches normal slot position"),
			AnchorEdgeSlots[4].AuthoredLayoutOffset,
			NormalEdgeSlots[4].AuthoredLayoutOffset);
		TestEqual(TEXT("Left hand anchor uses normal render scale"), AnchorEdgeSlots[0].RenderScale, 0.5f * AnchorEdgeSlots[0].PresentationScale);
		TestEqual(TEXT("Right hand anchor uses normal render scale"), AnchorEdgeSlots[4].RenderScale, 0.5f * AnchorEdgeSlots[4].PresentationScale);
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
	Anchor->LayoutFixtureCardCount = 5;
	Anchor->AuthoredCardSpacingPixels = 100.0f;
	Anchor->AuthoredMaxHandWidthPixels = 0.0f;
	Anchor->HandMaxEdgeDropPixels = 80.0f;
	Anchor->ShortHandEdgeDropPixels = 80.0f;
	Anchor->AuthoredCenterLiftPixels = 20.0f;
	Anchor->AuthoredDropCurveExponent = 2.0f;
	Anchor->bEnableCardLayerPixelSnapping = false;
	WacomFirstPersonCardLayerSpec::PrimeFallbackAnchor(PC, Character, Anchor);

	const TArray<FWacomFirstPersonCardLayerSlotView> SquareSlots = Anchor->BuildLayoutFixtureCardSlotViews();
	Anchor->AuthoredDropCurveExponent = 1.0f;
	const TArray<FWacomFirstPersonCardLayerSlotView> LinearSlots = Anchor->BuildLayoutFixtureCardSlotViews();

	TestEqual(TEXT("Square slot count"), SquareSlots.Num(), 5);
	TestEqual(TEXT("Linear slot count"), LinearSlots.Num(), 5);
	if (SquareSlots.Num() == 5 && LinearSlots.Num() == 5)
	{
		TestTrue(TEXT("Edge card drops lower than center"), SquareSlots[0].ScreenPosition.Y > SquareSlots[2].ScreenPosition.Y);
		TestTrue(TEXT("Center lift is applied"),
			FMath::IsNearlyEqual(
				SquareSlots[2].AuthoredLayoutOffset.Y,
				-20.0f * SquareSlots[2].PresentationScale,
				0.001f));
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

	const TArray<FWacomFirstPersonCardLayerSlotView> ClampedSlots = Anchor->BuildLayoutFixtureCardSlotViews();
	Anchor->bClampCardLayerRenderAngle = false;
	const TArray<FWacomFirstPersonCardLayerSlotView> UnclampedSlots = Anchor->BuildLayoutFixtureCardSlotViews();

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
	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerEntries(*Anchor, WacomFirstPersonCardLayerSourceIds::BattleHand(), { Pending, Hovered });
	FWacomFirstPersonCardLayerTestAccess::SetHoveredCardInstanceId(*Anchor, HoveredId);
	const TArray<FWacomFirstPersonCardLayerSlotView> NearSlots = Anchor->BuildActiveCardLayerSlotViewsForTest();

	Anchor->DistanceFromView = 500.0f;
	Anchor->RefreshAnchor(0.0f);
	const TArray<FWacomFirstPersonCardLayerSlotView> FarSlots = Anchor->BuildActiveCardLayerSlotViewsForTest();

	TestEqual(TEXT("Near slot count"), NearSlots.Num(), 2);
	TestEqual(TEXT("Far slot count"), FarSlots.Num(), 2);
	if (NearSlots.Num() == 2 && FarSlots.Num() == 2)
	{
		TestEqual(TEXT("Pending base scale stays stable"), NearSlots[0].RenderScale, NearSlots[0].PresentationScale);
		TestEqual(TEXT("Hovered base scale stays stable"), NearSlots[1].RenderScale, NearSlots[1].PresentationScale);
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
	Anchor->LayoutFixtureCardCount = 5;
	Anchor->HoverZOrderBoost = 500;

	const TArray<FWacomFirstPersonCardLayerSlotView> BaseSlots = Anchor->BuildLayoutFixtureCardSlotViews();
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
	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerEntries(*Anchor, WacomFirstPersonCardLayerSourceIds::BattleHand(), { A, B, C, D, E });
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
			FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*HoveredSlotWidget, MotionConfig);
			FWacomFirstPersonCardSlotVisualConfig VisualConfig;
			VisualConfig.HoverZOrderBoost = Anchor->HoverZOrderBoost;
			FWacomFirstPersonCardLayerTestAccess::SetSlotVisualConfig(*HoveredSlotWidget, VisualConfig);
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
	FWacomFirstPersonCardLayerStaticHitTestTest,
	"Wacom.UI.FirstPersonCardLayer.LayoutFixture.HitTestInvisibleAndNoInputBindings",
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
	"Wacom.UI.FirstPersonCardLayer.LayoutFixture.ProjectionFailureHidesCards",
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
	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildLayoutFixtureCardSlotViews();
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
	FWacomFirstPersonCardLayerProjectionRecoveryShowsReusedSlotTest,
	"Wacom.UI.FirstPersonCardLayer.LayoutFixture.ProjectionRecoveryShowsReusedSlot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerProjectionRecoveryShowsReusedSlotTest::RunTest(const FString& Parameters)
{
	const FGuid CardId = FGuid::NewGuid();
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>();
	if (!TestNotNull(TEXT("Layer widget"), Layer))
	{
		return false;
	}

	FWacomFirstPersonCardLayerSlotView HiddenSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D::ZeroVector);
	HiddenSlot.bProjected = false;
	HiddenSlot.ScreenPosition = FVector2D::ZeroVector;
	HiddenSlot.WidgetPosition = FVector2D::ZeroVector;
	HiddenSlot.SnappedWidgetPosition = FVector2D::ZeroVector;
	Layer->SetCardSlots({ HiddenSlot });

	UWacomFirstPersonCardLayerSlotWidget* HiddenSlotWidget = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Hidden slot widget is created"), HiddenSlotWidget))
	{
		return false;
	}
	TestFalse(TEXT("Unprojected slot is hidden"), Layer->IsCardSlotVisible(0));
	TestEqual(TEXT("Unprojected widget is collapsed"), HiddenSlotWidget->GetVisibility(), ESlateVisibility::Collapsed);

	FWacomFirstPersonCardLayerSlotView ProjectedSlot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(420.0f, 560.0f));
	Layer->SetCardSlots({ ProjectedSlot });

	UWacomFirstPersonCardLayerSlotWidget* ProjectedSlotWidget = Layer->GetSlotWidgetAt(0);
	TestEqual(TEXT("Same slot widget is reused"), ProjectedSlotWidget, HiddenSlotWidget);
	if (!TestNotNull(TEXT("Projected slot widget"), ProjectedSlotWidget))
	{
		return false;
	}
	TestTrue(TEXT("Projection recovery makes card visible immediately"), Layer->IsCardSlotVisible(0));
	TestNotEqual(TEXT("Recovered widget is no longer collapsed"),
		ProjectedSlotWidget->GetVisibility(),
		ESlateVisibility::Collapsed);
	TestTrue(TEXT("Recovered visual slot is projected"), ProjectedSlotWidget->GetVisualSlotView().bProjected);
	TestEqual(TEXT("Recovered visual slot snaps to latest target"),
		ProjectedSlotWidget->GetVisualSlotView().ScreenPosition,
		ProjectedSlot.ScreenPosition);

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
	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerEntries(*Anchor, WacomFirstPersonCardLayerSourceIds::BattleHand(), { LeftEdge, PendingCenter, HoveredRightEdge });
	const TArray<FWacomFirstPersonCardLayerSlotView> BaseSlots = Anchor->BuildActiveCardLayerSlotViewsForTest();

	PendingCenter.bIsPendingTargeting = true;
	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerEntries(*Anchor, WacomFirstPersonCardLayerSourceIds::BattleHand(), { LeftEdge, PendingCenter, HoveredRightEdge });
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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);
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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);
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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);

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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);

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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);
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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);

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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);

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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);
	FWacomFirstPersonCardSlotVisualConfig VisualConfig;
	VisualConfig.HoverLiftPixels = 40.0f;
	VisualConfig.HoverScale = 1.10f;
	FWacomFirstPersonCardLayerTestAccess::SetSlotVisualConfig(*Layer, VisualConfig);

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
	Config.DragTargetFocusMotionProfile.MotionSpeed = 100.0f;
	Config.DragTargetFocusMotionProfile.OpacitySpeed = 100.0f;
	Config.DragTargetFocusMotionProfile.EasePower = 1.0f;
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);
	FWacomFirstPersonCardSlotVisualConfig VisualConfig;
	VisualConfig.DragCardTargetFocusLiftPixels = 24.0f;
	VisualConfig.DragCardTargetFocusScale = 1.08f;
	FWacomFirstPersonCardLayerTestAccess::SetSlotVisualConfig(*Layer, VisualConfig);
	FWacomFirstPersonCardDragConfig DragConfig;
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*Layer, DragConfig);

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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);

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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);
	FWacomFirstPersonCardSlotVisualConfig VisualConfig;
	VisualConfig.PendingTargetingLiftPixels = 60.0f;
	VisualConfig.PendingTargetingScale = 1.15f;
	VisualConfig.PendingTargetingZOrderBoost = 1000;
	FWacomFirstPersonCardLayerTestAccess::SetSlotVisualConfig(*Layer, VisualConfig);
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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, MotionConfig);
	FWacomFirstPersonCardSlotVisualConfig VisualConfig;
	VisualConfig.HoverLiftPixels = 0.0f;
	VisualConfig.HoverScale = 1.0f;
	VisualConfig.HoverZOrderBoost = 250;
	FWacomFirstPersonCardLayerTestAccess::SetSlotVisualConfig(*Layer, VisualConfig);

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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);
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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);

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
	FWacomFirstPersonCardLayerMotionLargeDistanceLayoutAnimatesTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotion.LargeDistanceLayoutAnimates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerMotionLargeDistanceLayoutAnimatesTest::RunTest(const FString& Parameters)
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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);
	const FGuid CardId = FGuid::NewGuid();
	Layer->SetCardSlots({ WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(100.0f, 200.0f)) });
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
	Layer->SetCardSlots({ WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(400.0f, 500.0f)) });

	UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0);
	if (TestNotNull(TEXT("Slot widget"), SlotWidget))
	{
		TestEqual(
			TEXT("Large-distance layout update keeps current visual position"),
			SlotWidget->GetVisualSlotView().ScreenPosition,
			FVector2D(100.0f, 200.0f));
		FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.25f);
		const FVector2D MidMotionPosition = SlotWidget->GetVisualSlotView().ScreenPosition;
		TestTrue(
			TEXT("Large-distance layout update starts moving smoothly"),
			FVector2D::Distance(MidMotionPosition, FVector2D(100.0f, 200.0f)) > 0.1f);
		TestTrue(
			TEXT("Large-distance layout update does not snap on first tick"),
			FVector2D::Distance(MidMotionPosition, FVector2D(400.0f, 500.0f)) > 0.1f);
		FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
		TestEqual(
			TEXT("Large-distance layout update eventually reaches target"),
			SlotWidget->GetVisualSlotView().ScreenPosition,
			FVector2D(400.0f, 500.0f));
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerMotionInsertedCardKeepsExistingReflowSmoothTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotion.InsertedCardKeepsExistingReflowSmooth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerMotionInsertedCardKeepsExistingReflowSmoothTest::RunTest(const FString& Parameters)
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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);

	const FGuid FirstCardId = FGuid::NewGuid();
	const FGuid SecondCardId = FGuid::NewGuid();
	const FGuid InsertedCardId = FGuid::NewGuid();
	const FVector2D FirstInitialPosition(100.0f, 200.0f);
	const FVector2D SecondInitialPosition(220.0f, 200.0f);
	const FVector2D FirstReflowPosition(500.0f, 200.0f);
	const FVector2D SecondReflowPosition(620.0f, 200.0f);

	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(FirstCardId, 0, FirstInitialPosition),
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SecondCardId, 1, SecondInitialPosition)
	});
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 1.0f);
	Layer->SetCardSlots({
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(InsertedCardId, 0, FVector2D(100.0f, 200.0f)),
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(FirstCardId, 1, FirstReflowPosition),
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(SecondCardId, 2, SecondReflowPosition)
	});

	UWacomFirstPersonCardLayerSlotWidget* FirstWidget =
		FWacomFirstPersonCardLayerTestAccess::FindSlotWidgetByKey(
			*Layer,
			FirstCardId.ToString(EGuidFormats::DigitsWithHyphensLower));
	if (TestNotNull(TEXT("Existing first card widget"), FirstWidget))
	{
		TestEqual(
			TEXT("Inserted card reflow keeps existing card visual at old position"),
			FirstWidget->GetVisualSlotView().ScreenPosition,
			FirstInitialPosition);
		TestEqual(
			TEXT("Inserted card reflow updates existing card target"),
			FirstWidget->GetSlotView().ScreenPosition,
			FirstReflowPosition);
		FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Layer, 0.25f);
		const FVector2D MidMotionPosition = FirstWidget->GetVisualSlotView().ScreenPosition;
		TestTrue(
			TEXT("Existing card begins smooth reflow after insertion"),
			FVector2D::Distance(MidMotionPosition, FirstInitialPosition) > 0.1f);
		TestTrue(
			TEXT("Existing card does not snap to far reflow target"),
			FVector2D::Distance(MidMotionPosition, FirstReflowPosition) > 0.1f);
	}

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerMotionLayoutFixtureKeyTest,
	"Wacom.UI.FirstPersonCardLayer.SlotMotion.LayoutFixtureUsesStableIndexKeys",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerMotionLayoutFixtureKeyTest::RunTest(const FString& Parameters)
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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);
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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);
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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);
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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);
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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, MotionConfig);
	FWacomFirstPersonCardLayerTestAccess::SetInteractionFeedbackConfig(*Layer, WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());

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
			TEXT("Commit does not create a decorative cue"),
			FWacomFirstPersonCardLayerTestAccess::View(*Outgoing).InteractionCueKind,
			EWacomFirstPersonCardInteractionCueKind::None);
		TestTrue(
			TEXT("Commit applies its authoritative motion scale"),
			Outgoing->GetRenderTransform().Scale.X > 0.55f);
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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, MotionConfig);
	FWacomFirstPersonCardLayerTestAccess::SetInteractionFeedbackConfig(*Layer, WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());

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
	FWacomFirstPersonCardLayerUnknownCardPlayedEventNoCommitHintTest,
	"Wacom.UI.FirstPersonCardLayer.PlayCommit.UnknownCardPlayedEventDoesNotTriggerCommitOrTargetConfirm",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerUnknownCardPlayedEventNoCommitHintTest::RunTest(const FString& Parameters)
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
	HUD->StoreFirstPersonCardTransitionEventsForTest({
		WacomFirstPersonCardLayerSpec::MakeBattleEvent(EBattleEventType::CardPlayed, FakeCardId)
	});
	const TArray<FWacomFirstPersonCardLayerTransitionHint> Hints =
		HUD->BuildFirstPersonCardTransitionHintsForTest(Previous, Session->BuildSnapshot());

	TestEqual(TEXT("Unknown played card event records no transition/commit hints"), Hints.Num(), 0);
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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, MotionConfig);
	FWacomFirstPersonCardLayerTestAccess::SetInteractionFeedbackConfig(*Layer, WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());

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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, MotionConfig);
	FWacomFirstPersonCardLayerTestAccess::SetInteractionFeedbackConfig(*Layer, WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());

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

	FWacomFirstPersonCardLayerTestAccess::SetInteractionFeedbackConfig(*SlotWidget, WacomFirstPersonCardLayerSpec::MakeTestFeedbackConfig());
	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid()));
	SlotWidget->TriggerCommitFeedback();
	TestTrue(TEXT("Commit feedback starts"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bCommitFeedbackActive);
	TestEqual(
		TEXT("Commit has no decorative cue"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionCueKind,
		EWacomFirstPersonCardInteractionCueKind::None);

	SlotWidget->SetCardLayerInteractionEnabled(false);
	TestFalse(TEXT("Interaction disabled clears commit"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bCommitFeedbackActive);
	TestEqual(
		TEXT("Interaction disabled keeps cue clear"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionCueKind,
		EWacomFirstPersonCardInteractionCueKind::None);

	SlotWidget->SetCardLayerInteractionEnabled(true);
	SlotWidget->TriggerCommitFeedback();
	TestTrue(TEXT("Commit feedback restarts"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bCommitFeedbackActive);
	SlotWidget->SetSlotViewImmediate(WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FGuid::NewGuid()));
	TestFalse(TEXT("Slot reuse clears commit"), FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bCommitFeedbackActive);
	TestEqual(
		TEXT("Slot reuse keeps cue clear"),
		FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).InteractionCueKind,
		EWacomFirstPersonCardInteractionCueKind::None);

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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);

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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);

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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);

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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);
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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);
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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);
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
	FWacomFirstPersonCardLayerRunHandEnteredDrawnProfileTest,
	"Wacom.UI.FirstPersonCardLayer.TransitionOrigin.RunHandEnteredUsesDrawnEnterProfile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerRunHandEnteredDrawnProfileTest::RunTest(const FString& /*Parameters*/)
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
	Config.DrawnEnterOffsetPixels = FVector2D(0.0f, 88.0f);
	Config.DrawnEnterDurationSeconds = 0.2f;
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);

	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(240.0f, 280.0f));
	Slot.AnchorWidgetPosition = FVector2D(160.0f, 210.0f);
	Layer->SetCardTransitionHints({
		WacomFirstPersonCardLayerSpec::MakeTransitionHint(
			CardId,
			EWacomFirstPersonCardSlotTransitionKind::RunHandEntered)
	});
	Layer->SetCardSlots({ Slot });

	if (UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0))
	{
		TestTrue(TEXT("Run hand enter starts enter playback"),
			FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bEnterTransitionPlaybackActive);
		TestTrue(TEXT("Layer reports active presentation playback"),
			Layer->HasActivePresentationPlayback());
		TestEqual(TEXT("Run hand enter starts from Drawn hand anchor origin"),
			SlotWidget->GetVisualSlotView().ScreenPosition,
			Slot.AnchorWidgetPosition + Config.DrawnEnterOffsetPixels);

		FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(
			*Layer,
			Config.DrawnEnterDurationSeconds + 0.05f);
		TestFalse(TEXT("Run hand enter playback ends"),
			FWacomFirstPersonCardLayerTestAccess::View(*SlotWidget).bEnterTransitionPlaybackActive);
		TestEqual(TEXT("Run hand enter lands on target slot"),
			SlotWidget->GetVisualSlotView().ScreenPosition,
			Slot.ScreenPosition);
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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);
	const FGuid CardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeMotionSlot(CardId, 0, FVector2D(260.0f, 300.0f));
	Slot.AnchorWidgetPosition = FVector2D(180.0f, 230.0f);
	Layer->SetCardTransitionHints({
		WacomFirstPersonCardLayerSpec::MakeTransitionHint(CardId, EWacomFirstPersonCardSlotTransitionKind::Drawn)
	});
	Layer->SetCardSlots({ Slot });

	if (UWacomFirstPersonCardLayerSlotWidget* SlotWidget = Layer->GetSlotWidgetAt(0))
	{
		TestTrue(TEXT("Drawn card starts from configured origin"), SlotWidget->GetVisualSlotView().bProjected);
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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);
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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);
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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);
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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);

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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);
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
	"Wacom.UI.FirstPersonCardLayer.EventAwareTransitions.CardExhaustedEventAssignsExhaustedExitHint",
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
		TestEqual(TEXT("CardExhausted maps to exhausted transition"),
			Hint->TransitionKind, EWacomFirstPersonCardSlotTransitionKind::Exhausted);
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

	UCardDefinition* PoisonFang = WacomFirstPersonCardLayerSpec::MakeFixtureCard(
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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);
	FWacomFirstPersonCardSlotVisualConfig VisualConfig;
	VisualConfig.PendingTargetingLiftPixels = 80.0f;
	VisualConfig.PendingTargetingScale = 1.15f;
	FWacomFirstPersonCardLayerTestAccess::SetSlotVisualConfig(*Layer, VisualConfig);
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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);

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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);

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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);

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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);

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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);

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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);

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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);
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
	Anchor->LayoutFixtureCardCount = 1;
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("Layer"), Layer))
	{
		Anchor->DestroyComponent();
		PC->Destroy();
		Character->Destroy();
		return false;
	}
	FWacomFirstPersonCardLayerTestAccess::SetCardLayer(*Anchor, Layer);
	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerEntries(
		*Anchor,
		WacomFirstPersonCardLayerSourceIds::BattleHand(),
		{});
	FWacomFirstPersonCardLayerTestAccess::SetFirstPersonCardLayerInteractionEnabled(*Anchor, true);
	Anchor->CardSlotMotionSpeed = 11.0f;
	Anchor->CardSlotOpacitySpeed = 12.0f;
	Anchor->CardSlotMotionEasePower = 1.3f;
	Anchor->RefreshAnchor(1.0f / 60.0f);
	Anchor->RefreshCardLayerForTest();

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
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).SlotRuntimeConfig.Motion;
	ExpectProfile(TEXT("Layout inherits global"), MotionConfig.LayoutMotionProfile, 11.0f, 12.0f, 1.3f);
	ExpectProfile(TEXT("Hover inherits global"), MotionConfig.HoverMotionProfile, 11.0f, 12.0f, 1.3f);
	ExpectProfile(TEXT("Pending inherits global"), MotionConfig.PendingMotionProfile, 11.0f, 12.0f, 1.3f);
	ExpectProfile(TEXT("Enter inherits global"), MotionConfig.EnterMotionProfile, 11.0f, 12.0f, 1.3f);
	ExpectProfile(TEXT("Exit inherits global"), MotionConfig.ExitMotionProfile, 11.0f, 12.0f, 1.3f);

	const int32 ApplyCountBeforeOverrides =
		FWacomFirstPersonCardLayerTestAccess::View(*Anchor).CardLayerConfigApplyCount;
	Anchor->bOverrideHoverMotionProfile = true;
	Anchor->HoverMotionSpeed = 21.0f;
	Anchor->HoverOpacitySpeed = 22.0f;
	Anchor->HoverMotionEasePower = 1.4f;
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

	MotionConfig = FWacomFirstPersonCardLayerTestAccess::View(*Layer).SlotRuntimeConfig.Motion;
	ExpectProfile(TEXT("Layout still inherits global"), MotionConfig.LayoutMotionProfile, 11.0f, 12.0f, 1.3f);
	ExpectProfile(TEXT("Pending still inherits global"), MotionConfig.PendingMotionProfile, 11.0f, 12.0f, 1.3f);
	ExpectProfile(TEXT("Hover uses override"), MotionConfig.HoverMotionProfile, 21.0f, 22.0f, 1.4f);
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
	FWacomFirstPersonCardLayerTestAccess::SetFirstPersonCardLayerInteractionEnabled(*Anchor, true);

	const FGuid NoTargetCardId = FGuid::NewGuid();
	const FGuid TargetedCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerEntry NoTargetEntry;
	NoTargetEntry.CardInstanceId = NoTargetCardId;
	NoTargetEntry.CardViewData.Name = FText::FromString(TEXT("No Target"));
	NoTargetEntry.bIsPlayable = true;
	WacomFirstPersonCardLayerSpec::SetEntryInteractionIntent(
		NoTargetEntry,
		EWacomFirstPersonCardInteractionIntent::DragToDropTarget);
	FWacomFirstPersonCardLayerEntry TargetedEntry;
	TargetedEntry.CardInstanceId = TargetedCardId;
	TargetedEntry.CardViewData.Name = FText::FromString(TEXT("Targeted"));
	TargetedEntry.bIsPlayable = true;
	WacomFirstPersonCardLayerSpec::SetEntryInteractionIntent(
		TargetedEntry,
		EWacomFirstPersonCardInteractionIntent::AimWorldTarget);
	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerEntries(*Anchor, WacomFirstPersonCardLayerSourceIds::BattleHand(), { NoTargetEntry, TargetedEntry });
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
	Anchor->LayoutFixtureCardCount = 2;
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("Layer"), Layer))
	{
		Anchor->DestroyComponent();
		PC->Destroy();
		Character->Destroy();
		return false;
	}
	FWacomFirstPersonCardLayerTestAccess::SetCardLayer(*Anchor, Layer);
	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerEntries(
		*Anchor,
		WacomFirstPersonCardLayerSourceIds::BattleHand(),
		{});
	FWacomFirstPersonCardLayerTestAccess::SetFirstPersonCardLayerInteractionEnabled(*Anchor, true);
	Anchor->RefreshAnchor(1.0f / 60.0f);
	Anchor->RefreshCardLayerForTest();

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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);

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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);

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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);

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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);
	FWacomFirstPersonCardSlotVisualConfig VisualConfig;
	VisualConfig.PendingTargetingLiftPixels = 40.0f;
	VisualConfig.PendingTargetingScale = 1.08f;
	FWacomFirstPersonCardLayerTestAccess::SetSlotVisualConfig(*Layer, VisualConfig);

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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, Config);

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
	Anchor->HandCardRenderScale = 0.5f;
	Anchor->HoverLiftPixels = 30.0f;
	Anchor->HoverScale = 1.1f;
	Anchor->HoverZOrderBoost = 250;

	const FGuid CardInstanceId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerEntry Entry;
	Entry.CardInstanceId = CardInstanceId;
	Entry.CardViewData.Name = FText::FromString(TEXT("Hover"));
	Entry.bIsPlayable = true;
	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerEntries(*Anchor, WacomFirstPersonCardLayerSourceIds::BattleHand(), { Entry });

	const TArray<FWacomFirstPersonCardLayerSlotView> BaseSlots = Anchor->BuildActiveCardLayerSlotViewsForTest();
	TestEqual(TEXT("Base slot count"), BaseSlots.Num(), 1);
	FWacomFirstPersonCardLayerTestAccess::SetFirstPersonCardLayerInteractionEnabled(*Anchor, true);
	Anchor->RefreshCardLayerForTest();

	UWacomFirstPersonCardLayerWidget* Layer = FWacomFirstPersonCardLayerTestAccess::CardLayer(*Anchor);
	if (Layer)
	{
		FWacomFirstPersonCardSlotMotionConfig MotionConfig;
		MotionConfig.bEnabled = false;
		FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, MotionConfig);
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
			TestEqual(TEXT("Hover visual lifts card"), HoverVisualSlot.ScreenPosition.Y, BaseSlots[0].ScreenPosition.Y - 30.0f * BaseSlots[0].PresentationScale);
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
	Anchor->HandCardRenderScale = 0.5f;
	const FGuid CardInstanceId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerEntry Entry;
	Entry.CardInstanceId = CardInstanceId;
	Entry.CardViewData.Name = FText::FromString(TEXT("Hovered layout"));
	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerEntries(*Anchor, WacomFirstPersonCardLayerSourceIds::BattleHand(), { Entry });
	FWacomFirstPersonCardLayerTestAccess::SetFirstPersonCardLayerInteractionEnabled(*Anchor, true);
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
	Anchor->HandCardRenderScale = 0.5f;
	const FGuid CardInstanceId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerEntry Entry;
	Entry.CardInstanceId = CardInstanceId;
	Entry.CardViewData.Name = FText::FromString(TEXT("Card Target"));
	Entry.bIsPlayable = false;
	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerEntries(*Anchor, WacomFirstPersonCardLayerSourceIds::BattleHand(), { Entry });
	FWacomFirstPersonCardLayerTestAccess::SetFirstPersonCardLayerInteractionEnabled(*Anchor, true);
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

	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig());
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

	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig());
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

	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig());
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
			== EWacomFirstPersonCardGestureState::DraggingNoTargetCard);
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

	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig());
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid CardInstanceId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardInstanceId);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(Slot, EWacomFirstPersonCardInteractionIntent::AimWorldTarget);
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

	FWacomFirstPersonCardLayerTestAccess::SetFirstPersonCardLayerInteractionEnabled(*Anchor, true);

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
	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, MotionConfig);
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

	FWacomFirstPersonCardLayerTestAccess::SetFirstPersonCardLayerInteractionEnabled(*Anchor, true);

	const FGuid CardId = FGuid::NewGuid();
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("First-person layer"), Layer))
	{
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig());
	Layer->SetCardLayerInteractionEnabled(true);
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardId);
	Slot.Entry.CardViewData.Name = FText::FromString(TEXT("Targeted Shortcut Cancel"));
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(Slot, EWacomFirstPersonCardInteractionIntent::AimWorldTarget);
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

	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig());
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid CardInstanceId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardInstanceId);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(Slot, EWacomFirstPersonCardInteractionIntent::DragToDropTarget);
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

	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig());
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid TargetCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView SourceSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(SourceCardId);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(SourceSlot, EWacomFirstPersonCardInteractionIntent::AimCardTarget);
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

	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig());
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid TargetCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView SourceSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(SourceCardId);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(SourceSlot, EWacomFirstPersonCardInteractionIntent::AimCardTarget);
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

	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig());
	Layer->SetCardLayerInteractionEnabled(true);
	FWacomFirstPersonCardLayerTestAccess::SetViewportSizeOverride(*Layer, FVector2D(1920.0f, 1080.0f));

	const FGuid SourceCardId = FGuid::NewGuid();
	const FGuid TargetCardId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView SourceSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(SourceCardId);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(SourceSlot, EWacomFirstPersonCardInteractionIntent::AimCardTarget);
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

	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig());
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid CardInstanceId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardInstanceId);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(Slot, EWacomFirstPersonCardInteractionIntent::AimWorldTarget);
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

	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig());
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid CardInstanceId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardInstanceId);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(Slot, EWacomFirstPersonCardInteractionIntent::AimWorldTarget);
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

	FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, WacomFirstPersonCardLayerSpec::MakeFastSlotMotionConfig());
	Layer->SetCardLayerInteractionEnabled(true);

	const FGuid CardInstanceId = FGuid::NewGuid();
	FWacomFirstPersonCardLayerSlotView Slot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardInstanceId);
	WacomFirstPersonCardLayerSpec::SetSlotInteractionIntent(Slot, EWacomFirstPersonCardInteractionIntent::AimWorldTarget);
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
	"Wacom.UI.FirstPersonCardLayer.RuntimeSource.ActiveSourceProvidesCardsAndClearReturnsEmpty",
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
	FWacomCardViewData RuntimeCard;
	RuntimeCard.Name = FText::FromString(TEXT("Runtime Battle Card"));
	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerData(*Anchor, WacomFirstPersonCardLayerSourceIds::BattleHand(), { RuntimeCard });
	const TArray<FWacomFirstPersonCardLayerSlotView> RuntimeSlots = Anchor->BuildActiveCardLayerSlotViewsForTest();
	TestEqual(TEXT("Active runtime source provides one card"), RuntimeSlots.Num(), 1);
	if (RuntimeSlots.Num() == 1)
	{
		TestEqual(TEXT("Runtime card data is used"), RuntimeSlots[0].Entry.CardViewData.Name.ToString(), FString(TEXT("Runtime Battle Card")));
	}

	FWacomFirstPersonCardLayerTestAccess::ClearRuntimeCardLayerData(*Anchor, WacomFirstPersonCardLayerSourceIds::BattleHand());
	TestEqual(TEXT("Clearing runtime source does not synthesize fallback cards"), Anchor->BuildActiveCardLayerSlotViewsForTest().Num(), 0);

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerEmptyRuntimeSourceTest,
	"Wacom.UI.FirstPersonCardLayer.RuntimeSource.EmptySourceOwnsEmptyHand",
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
	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerData(*Anchor, WacomFirstPersonCardLayerSourceIds::BattleHand(), {});
	TestTrue(TEXT("Empty runtime source is still active"), Anchor->HasRuntimeCardLayerData());
	TestEqual(TEXT("Empty runtime hand shows no cards"), Anchor->BuildActiveCardLayerSlotViewsForTest().Num(), 0);

	FWacomFirstPersonCardLayerTestAccess::ClearRuntimeCardLayerData(*Anchor, WacomFirstPersonCardLayerSourceIds::BattleHand());
	TestEqual(TEXT("Cleared runtime source leaves the formal layer empty"), Anchor->BuildActiveCardLayerSlotViewsForTest().Num(), 0);

	Anchor->DestroyComponent();
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

	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerEntries(*Anchor, WacomFirstPersonCardLayerSourceIds::BattleHand(), { NormalEntry, PendingEntry, DisabledAnchorEntry });
	const TArray<FWacomFirstPersonCardLayerSlotView> Slots = Anchor->BuildActiveCardLayerSlotViewsForTest();
	TestEqual(TEXT("Runtime entries produce slots"), Slots.Num(), 3);
	if (Slots.Num() == 3)
	{
		TestTrue(TEXT("All slots know target-select is active"), Slots[0].bHasPendingTargetingCardInHand);
		TestTrue(TEXT("Pending card keeps pending state marker"), Slots[1].Entry.bIsPendingTargeting);
		TestEqual(TEXT("Pending card keeps base scale in anchor slot"), Slots[1].RenderScale, 0.5f * Slots[1].PresentationScale);
		TestEqual(TEXT("Non-pending keeps base opacity in anchor slot"), Slots[0].RenderOpacity, 1.0f);
		TestEqual(TEXT("Disabled anchor keeps normal card scale"), Slots[2].RenderScale, 0.5f * Slots[2].PresentationScale);
		TestEqual(TEXT("Disabled card keeps disabled base opacity in anchor slot"), Slots[2].RenderOpacity, 0.6f);
		TestTrue(TEXT("Disabled card view data stays disabled"), Slots[2].Entry.CardViewData.bDisabled);
	}

	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (TestNotNull(TEXT("Layer widget"), Layer))
	{
		FWacomFirstPersonCardSlotMotionConfig MotionConfig;
		MotionConfig.bEnabled = false;
		FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*Layer, MotionConfig);
		FWacomFirstPersonCardSlotVisualConfig VisualConfig;
		VisualConfig.PendingTargetingLiftPixels = 40.0f;
		VisualConfig.PendingTargetingScale = 1.2f;
		VisualConfig.PendingTargetingZOrderBoost = 1200;
		VisualConfig.TargetSelectNonPendingOpacityMultiplier = 0.5f;
		FWacomFirstPersonCardLayerTestAccess::SetSlotVisualConfig(*Layer, VisualConfig);
		Layer->SetCardSlots(Slots);
		TestEqual(TEXT("Layer is hit-test-invisible"), Layer->GetVisibility(), ESlateVisibility::HitTestInvisible);
		if (TestNotNull(TEXT("Pending card view exists"), Layer->GetCardViewAt(1)))
		{
			TestTrue(TEXT("Pending visual lifts above normal card"), Layer->GetSlotWidgetAt(1)->GetVisualSlotView().ScreenPosition.Y < Slots[1].ScreenPosition.Y);
			TestEqual(TEXT("Pending widget scale uses visual presentation"), Layer->GetCardRenderTransformAt(1).Scale, FVector2D(Slots[1].RenderScale * 1.2f));
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

	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerEntries(*Anchor, WacomFirstPersonCardLayerSourceIds::BattleHand(), { PendingEntry, NormalEntry });
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
			FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*SlotWidget, MotionConfig);
			FWacomFirstPersonCardSlotVisualConfig VisualConfig;
			VisualConfig.bPendingTargetingStraightenAngle = true;
			VisualConfig.PendingTargetingAngleBlend = 0.75f;
			FWacomFirstPersonCardLayerTestAccess::SetSlotVisualConfig(*SlotWidget, VisualConfig);
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

	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerEntries(*Anchor, WacomFirstPersonCardLayerSourceIds::BattleHand(), { PendingEntry });
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
			FWacomFirstPersonCardLayerTestAccess::SetSlotMotionConfig(*SlotWidget, MotionConfig);
			FWacomFirstPersonCardSlotVisualConfig VisualConfig;
			VisualConfig.PendingTargetingLiftPixels = 40.0f;
			VisualConfig.PendingTargetingScale = 1.2f;
			VisualConfig.PendingTargetingZOrderBoost = 1000;
			VisualConfig.HoverLiftPixels = 30.0f;
			VisualConfig.HoverScale = 1.1f;
			VisualConfig.HoverZOrderBoost = 250;
			FWacomFirstPersonCardLayerTestAccess::SetSlotVisualConfig(*SlotWidget, VisualConfig);
			SlotWidget->SetCardLayerInteractionEnabled(true);
			SlotWidget->SetSlotViewImmediate(PendingHoverSlots[0]);
			TestEqual(TEXT("Pending visual does not add hover scale"), SlotWidget->GetVisualSlotView().RenderScale, PendingSlots[0].RenderScale * 1.2f);
			TestEqual(TEXT("Pending visual does not add hover lift"), SlotWidget->GetVisualSlotView().ScreenPosition.Y, PendingSlots[0].ScreenPosition.Y - 40.0f);
			TestEqual(TEXT("Pending visual does not add hover z-order"), SlotWidget->GetVisualSlotView().ZOrder, PendingSlots[0].ZOrder + 1000);
		}
	}

	Anchor->DestroyComponent();
	Character->Destroy();
	PC->Destroy();
	return true;
}
