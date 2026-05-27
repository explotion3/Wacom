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

	Character->GetBattleCameraLookComponent()->DeactivateBattleCameraLook();
	Anchor->DestroyComponent();
	Segment->Destroy();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerPartialLookTest,
	"Wacom.UI.FirstPersonCardLayer.Anchor.PartialLookInfluence",
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
	Character->GetCursorLookDriverComponent()->UpdateFromNormalizedCursor(
		FVector2D(1.0f, -1.0f),
		0.0f,
		20.0f,
		10.0f);

	Anchor->RefreshAnchor(0.0f);
	const FWacomFirstPersonCardAnchorDebugView View = Anchor->GetFirstPersonCardAnchorDebugView();
	TestEqual(TEXT("Yaw uses partial cursor influence"), View.LookOffsetUsed.Yaw, 5.0);
	TestEqual(TEXT("Pitch uses partial cursor influence"), View.LookOffsetUsed.Pitch, 5.0);

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
	"Wacom.UI.FirstPersonCardLayer.BattleHandAdapter.DisabledDoesNotWriteRuntimeHand",
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
	HUD->SetSession(Session);
	const FBattleSnapshot Snapshot = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({
		WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(Card, 3, true)
	});
	HUD->SyncFirstPersonBattleHandLayerForTest(Snapshot);
	TestFalse(TEXT("Prototype switch defaults off"), Character->GetFirstPersonCardAnchorComponent()->HasRuntimeCardLayerData());

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
	HUD->EnableFirstPersonBattleHandLayerPrototypeForTest();
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
	HUD->SetSession(Session);
	WacomFirstPersonCardLayerSpec::SettleBattlePresentationQueue(*HUD);
	HUD->EnableFirstPersonBattleHandLayerPrototypeForTest();
	HUD->EnableFirstPersonBattleHandInteractionPrototypeForTest();
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
	TestTrue(TEXT("Interaction prototype is enabled on anchor"), Anchor->IsBattleHandInteractionPrototypeEnabled());

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
	HUD->EnableFirstPersonBattleHandLayerPrototypeForTest();
	HUD->EnableFirstPersonBattleHandInteractionPrototypeForTest();
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
	const FHandCardSnapshot CardSnapshot = WacomFirstPersonCardLayerSpec::MakeHandCardSnapshot(Card, 2, true);
	const FBattleSnapshot Snapshot = WacomFirstPersonCardLayerSpec::MakeSnapshotWithHand({ CardSnapshot });
	HUD->RefreshFromSnapshotForTest(Snapshot);

	HUD->HandleFirstPersonCardHoveredForTest(
		CardSnapshot.InstanceId,
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(CardSnapshot.InstanceId));
	TestFalse(TEXT("First-person detail stays hidden when switches are off"), HUD->IsCardDetailPanelVisible());

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
	HUD->TakeWidget();
	HUD->EnableFirstPersonBattleHandLayerPrototypeForTest();
	HUD->EnableFirstPersonBattleHandInteractionPrototypeForTest();
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

	HUD->HandleFirstPersonCardHoveredForTest(
		FirstSnapshot.InstanceId,
		WacomFirstPersonCardLayerSpec::MakeProjectedInteractionSlot(FirstSnapshot.InstanceId));
	TestTrue(TEXT("First-person hover shows detail"), HUD->IsCardDetailPanelVisible());
	TestEqual(TEXT("First-person detail uses first snapshot definition"),
		HUD->GetCardDetailPanelNameText().ToString(),
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
	HUD->EnableFirstPersonBattleHandLayerPrototypeForTest();
	HUD->EnableFirstPersonBattleHandInteractionPrototypeForTest();
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
	HUD->EnableFirstPersonBattleHandLayerPrototypeForTest();
	HUD->EnableFirstPersonBattleHandInteractionPrototypeForTest();
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
	HUD->EnableFirstPersonBattleHandLayerPrototypeForTest();
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
	HUD->EnableFirstPersonBattleHandLayerPrototypeForTest();
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
