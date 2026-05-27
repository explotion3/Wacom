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
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
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
	const TArray<FWacomFirstPersonStaticCardSlotView> Slots = Anchor->BuildStaticCardSlotViews();
	TestEqual(TEXT("Fallback creates five static slots"), Slots.Num(), 5);
	if (Slots.Num() > 0)
	{
		TestEqual(TEXT("Fallback card has placeholder name"), Slots[0].CardViewData.Name.ToString(), FString(TEXT("Anchor Card 1")));
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
	const TArray<FWacomFirstPersonStaticCardSlotView> Slots = Anchor->BuildStaticCardSlotViews();
	TestEqual(TEXT("Definitions choose slot count"), Slots.Num(), 2);
	if (Slots.Num() == 2)
	{
		TestEqual(TEXT("First definition name is used"), Slots[0].CardViewData.Name.ToString(), FString(TEXT("Preview.Alpha")));
		TestEqual(TEXT("Second definition cost is used"), Slots[1].CardViewData.Cost, 4);
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
	const TArray<FWacomFirstPersonStaticCardSlotView> Slots = Anchor->BuildStaticCardSlotViews();
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

	FWacomFirstPersonStaticCardSlotView Slot;
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
	const TArray<FWacomFirstPersonStaticCardSlotView> Slots = Anchor->BuildStaticCardSlotViews();
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
