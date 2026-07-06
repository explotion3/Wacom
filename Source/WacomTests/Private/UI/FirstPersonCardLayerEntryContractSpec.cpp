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
		Slot.Entry.TargetMode = ECardTargetMode::None;
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
	FWacomFirstPersonCardLayerEntryLegacyTargetProjectionRefreshTest,
	"Wacom.UI.FirstPersonCardLayer.EntryContract.LegacyTargetModeDoesNotDirtySlotRefresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLayerEntryLegacyTargetProjectionRefreshTest::RunTest(
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

	FWacomFirstPersonCardLayerSlotView LegacyProjectionChangedSlot = BaseSlot;
	LegacyProjectionChangedSlot.Entry.TargetMode = ECardTargetMode::HandCard;
	LegacyProjectionChangedSlot.Entry.InteractionIntent =
		BaseSlot.Entry.InteractionIntent;
	Layer->SetCardSlots({ LegacyProjectionChangedSlot });

	const FWacomFirstPersonCardLayerMotionDebugView LegacyProjectionDebug =
		Layer->GetSlotMotionDebugView();
	TestEqual(
		TEXT("Legacy target projection refresh skips full reconcile"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer)
			.SkippedEquivalentSlotRefreshCount,
		InitialSkipCount + 1);
	TestEqual(
		TEXT("Legacy target projection refresh creates no widgets"),
		LegacyProjectionDebug.CreatedThisUpdate,
		0);
	TestEqual(
		TEXT("Legacy target projection refresh reuses no widgets through full reconcile"),
		LegacyProjectionDebug.ReusedThisUpdate,
		0);
	TestEqual(
		TEXT("Legacy target projection keeps widget"),
		Layer->GetSlotWidgetAt(0),
		OriginalWidget);
	TestEqual(
		TEXT("Legacy target projection still updates debug slot view"),
		Layer->GetSlotWidgetAt(0)->GetSlotView().Entry.TargetMode,
		ECardTargetMode::HandCard);

	const int32 AfterLegacyProjectionSkipCount =
		FWacomFirstPersonCardLayerTestAccess::View(*Layer)
			.SkippedEquivalentSlotRefreshCount;
	FWacomFirstPersonCardLayerSlotView IntentChangedSlot =
		LegacyProjectionChangedSlot;
	IntentChangedSlot.Entry.InteractionIntent =
		EWacomFirstPersonCardInteractionIntent::AimCardTarget;
	Layer->SetCardSlots({ IntentChangedSlot });

	const FWacomFirstPersonCardLayerMotionDebugView IntentDebug =
		Layer->GetSlotMotionDebugView();
	TestEqual(
		TEXT("Interaction intent refresh does not skip"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer)
			.SkippedEquivalentSlotRefreshCount,
		AfterLegacyProjectionSkipCount);
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
