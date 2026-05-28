// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomRunTunnelSegmentActor.h"
#include "Components/SplineComponent.h"
#include "Components/WacomBattleCameraLookComponent.h"
#include "Components/WacomCursorLookDriverComponent.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Components/WacomRunTunnelMovementComponent.h"
#include "Cards/CardDefinition.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Fixtures/BattleTestFixtures.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
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
		Slot.ScreenPosition = FVector2D(500.0f, 600.0f);
		Slot.RenderScale = 0.55f;
		Slot.RenderOpacity = 1.0f;
		Slot.ZOrder = 0;
		Slot.bProjected = bProjected;
		return Slot;
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
	TestTrue(TEXT("Second first-person hover keeps detail visible"), HUD->IsCardDetailPanelVisible());
	TestEqual(TEXT("First-person detail replaces source"),
		HUD->GetCardDetailPanelNameText().ToString(),
		FString(TEXT("第一人称详情卡 B")));

	HUD->HandleFirstPersonCardUnhoveredForTest(
		FirstSnapshot.InstanceId,
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FirstSnapshot.InstanceId));
	TestTrue(TEXT("Old first-person source unhover does not hide current detail"), HUD->IsCardDetailPanelVisible());
	TestEqual(TEXT("Old first-person source unhover keeps second detail"),
		HUD->GetCardDetailPanelNameText().ToString(),
		FString(TEXT("第一人称详情卡 B")));

	HUD->HandleFirstPersonCardUnhoveredForTest(
		SecondSnapshot.InstanceId,
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(SecondSnapshot.InstanceId));
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

	FWacomFirstPersonCardLayerSlotView InitialSlot =
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardSnapshot.InstanceId);
	InitialSlot.ScreenPosition = FVector2D(500.0f, 600.0f);
	InitialSlot.RenderScale = 1.0f;
	HUD->HandleFirstPersonCardHoveredForTest(CardSnapshot.InstanceId, InitialSlot);
	const FVector2D InitialPosition = HUD->GetFirstPersonCardDetailPanelPositionForTest();
	TestTrue(TEXT("First-person detail visible before follow update"), HUD->IsFirstPersonCardDetailPanelVisibleForTest());

	FWacomFirstPersonCardLayerSlotView UpdatedSlot = InitialSlot;
	UpdatedSlot.ScreenPosition = FVector2D(700.0f, 600.0f);
	UpdatedSlot.bIsHovered = true;
	HUD->HandleFirstPersonCardLayoutUpdatedForTest(CardSnapshot.InstanceId, UpdatedSlot);
	const FVector2D UpdatedPosition = HUD->GetFirstPersonCardDetailPanelPositionForTest();
	TestNotEqual(TEXT("Hovered detail position follows slot layout update"), UpdatedPosition, InitialPosition);

	FWacomFirstPersonCardLayerSlotView OtherSlot = UpdatedSlot;
	OtherSlot.ScreenPosition = FVector2D(900.0f, 600.0f);
	HUD->HandleFirstPersonCardLayoutUpdatedForTest(FGuid::NewGuid(), OtherSlot);
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
	TestTrue(TEXT("First-person detail is visible before target select"), HUD->IsCardDetailPanelVisible());

	HUD->SetTargetSelectionStateForTest(CardSnapshot.InstanceId);
	TestFalse(TEXT("Entering TargetSelect hides first-person detail"), HUD->IsCardDetailPanelVisible());

	HUD->ClearTargetSelectionStateForTest();
	HUD->RefreshFromSnapshotForTest(Snapshot);
	HUD->HandleFirstPersonCardHoveredForTest(
		CardSnapshot.InstanceId,
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardSnapshot.InstanceId));
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
	Anchor->HandAnchorScale = 0.9f;
	Anchor->DisabledRenderOpacity = 0.6f;

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
		TestTrue(TEXT("Pending card gets higher z-order"), Slots[1].ZOrder > Slots[2].ZOrder);
		TestEqual(TEXT("Disabled anchor applies anchor scale"), Slots[2].RenderScale, 0.5f * 0.9f);
		TestEqual(TEXT("Disabled card applies layer opacity"), Slots[2].RenderOpacity, 0.6f);
		TestTrue(TEXT("Disabled card view data stays disabled"), Slots[2].Entry.CardViewData.bDisabled);
	}

	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (TestNotNull(TEXT("Layer widget"), Layer))
	{
		Layer->SetCardSlots(Slots);
		TestEqual(TEXT("Layer is hit-test-invisible"), Layer->GetVisibility(), ESlateVisibility::HitTestInvisible);
		if (TestNotNull(TEXT("Pending card view exists"), Layer->GetCardViewAt(1)))
		{
			TestEqual(TEXT("Pending widget scale matches slot"), Layer->GetCardRenderTransformAt(1).Scale, FVector2D(0.5f * 1.2f));
			TestTrue(TEXT("Pending widget z-order is raised"), Layer->GetCardZOrderAt(1) > Layer->GetCardZOrderAt(2));
		}
		if (TestNotNull(TEXT("Disabled card view exists"), Layer->GetCardViewAt(2)))
		{
			TestEqual(TEXT("Disabled widget opacity matches slot"), Layer->GetCardRenderOpacityAt(2), 0.6f);
		}
	}

	Anchor->DestroyComponent();
	Character->Destroy();
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
