// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "UI/FirstPersonCardLayerTestAccess.h"

namespace WacomFirstPersonCardLayerEntryContractSpec
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

	FWacomFirstPersonCardSlotMotionConfig MakeFastSlotMotionConfig()
	{
		FWacomFirstPersonCardSlotMotionConfig Config;
		Config.bEnabled = true;
		Config.MotionSpeed = 1.0f;
		Config.OpacitySpeed = 1.0f;
		Config.EasePower = 1.0f;
		Config.EnterOffsetPixels = FVector2D::ZeroVector;
		Config.EnterOpacity = 1.0f;
		Config.ExitOffsetPixels = FVector2D(0.0f, 30.0f);
		Config.ExitDuration = 0.2f;
		return Config;
	}

	FWacomFirstPersonCardLayerSlotView MakeMotionSlot(
		const FGuid& CardInstanceId,
		const FVector2D& Position)
	{
		FWacomFirstPersonCardLayerSlotView Slot;
		Slot.Index = 0;
		Slot.Entry.CardInstanceId = CardInstanceId;
		Slot.Entry.bIsPlayable = true;
		Slot.Entry.CardViewData.bDisabled = false;
		Slot.Entry.InteractionIntent =
			EWacomFirstPersonCardInteractionIntent::CommitNoTarget;
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLayerEntryInteractionIntentRefreshTest,
	"Wacom.UI.FirstPersonCardLayer.EntryContract.InteractionIntentDrivesSlotRefresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerEntryInteractionIntentRefreshTest::RunTest(
	const FString& Parameters)
{
	UWorld* World = WacomFirstPersonCardLayerEntryContractSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC = World->SpawnActor<APlayerController>(
		APlayerController::StaticClass(),
		FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer =
		NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	Layer->SetSlotMotionConfig(
		WacomFirstPersonCardLayerEntryContractSpec::MakeFastSlotMotionConfig());

	const FGuid CardId = FGuid::NewGuid();
	const FWacomFirstPersonCardLayerSlotView BaseSlot =
		WacomFirstPersonCardLayerEntryContractSpec::MakeMotionSlot(
			CardId,
			FVector2D(100.0f, 200.0f));
	Layer->SetCardSlots({ BaseSlot });
	UWacomFirstPersonCardLayerSlotWidget* OriginalWidget =
		Layer->GetSlotWidgetAt(0);
	const int32 InitialSkipCount =
		FWacomFirstPersonCardLayerTestAccess::View(*Layer)
			.SkippedEquivalentSlotRefreshCount;

	Layer->SetCardSlots({ BaseSlot });

	const FWacomFirstPersonCardLayerMotionDebugView EquivalentDebug =
		Layer->GetSlotMotionDebugView();
	TestEqual(
		TEXT("Equivalent entry refresh skips full reconcile"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer)
			.SkippedEquivalentSlotRefreshCount,
		InitialSkipCount + 1);
	TestEqual(
		TEXT("Equivalent entry refresh creates no widgets"),
		EquivalentDebug.CreatedThisUpdate,
		0);
	TestEqual(
		TEXT("Equivalent entry refresh reuses no widgets through full reconcile"),
		EquivalentDebug.ReusedThisUpdate,
		0);
	TestEqual(
		TEXT("Equivalent entry refresh keeps widget"),
		Layer->GetSlotWidgetAt(0),
		OriginalWidget);

	const int32 AfterEquivalentSkipCount =
		FWacomFirstPersonCardLayerTestAccess::View(*Layer)
			.SkippedEquivalentSlotRefreshCount;
	FWacomFirstPersonCardLayerSlotView IntentChangedSlot =
		BaseSlot;
	IntentChangedSlot.Entry.InteractionIntent =
		EWacomFirstPersonCardInteractionIntent::AimCardTarget;
	Layer->SetCardSlots({ IntentChangedSlot });

	const FWacomFirstPersonCardLayerMotionDebugView IntentDebug =
		Layer->GetSlotMotionDebugView();
	TestEqual(
		TEXT("Interaction intent refresh does not skip"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer)
			.SkippedEquivalentSlotRefreshCount,
		AfterEquivalentSkipCount);
	TestEqual(
		TEXT("Interaction intent refresh reuses widget"),
		IntentDebug.ReusedThisUpdate,
		1);
	TestEqual(
		TEXT("Interaction intent refresh updates slot interface"),
		Layer->GetSlotWidgetAt(0)->GetSlotView().Entry.InteractionIntent,
		EWacomFirstPersonCardInteractionIntent::AimCardTarget);

	PC->Destroy();
	return true;
}
