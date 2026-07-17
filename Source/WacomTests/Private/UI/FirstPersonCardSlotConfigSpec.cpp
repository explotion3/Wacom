// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "UI/FirstPersonCardLayerTestAccess.h"

#if WITH_AUTOMATION_TESTS

namespace WacomFirstPersonCardSlotConfigSpec
{
	FWacomFirstPersonCardLayerSlotView MakeSlot(const FGuid& CardId)
	{
		FWacomFirstPersonCardLayerSlotView Slot;
		Slot.Index = 0;
		Slot.Entry.CardInstanceId = CardId;
		Slot.Entry.Zone = EHandZone::Both;
		Slot.Entry.bIsPlayable = true;
		Slot.ScreenPosition = FVector2D(100.0f, 200.0f);
		Slot.WidgetPosition = Slot.ScreenPosition;
		Slot.SnappedWidgetPosition = Slot.ScreenPosition;
		Slot.InputHitCenter = Slot.ScreenPosition;
		Slot.InputHitScale = 1.0f;
		Slot.RenderScale = 1.0f;
		Slot.RenderOpacity = 1.0f;
		Slot.bProjected = true;
		return Slot;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardSlotRuntimeConfigAtomicPropagationTest,
	"Wacom.UI.FirstPersonCardLayer.SlotConfig.AtomicPropagationAndEquivalentRefresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardSlotRuntimeConfigAtomicPropagationTest::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardSlotConfigSpec;
	UWacomFirstPersonCardLayerWidget* Layer = NewObject<UWacomFirstPersonCardLayerWidget>();
	Layer->SetCardSlots({ MakeSlot(FGuid::NewGuid()) });
	UWacomFirstPersonCardLayerSlotWidget* Slot = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Layer slot"), Slot))
	{
		return false;
	}

	FWacomFirstPersonCardSlotRuntimeConfig Config;
	Config.Motion.bEnabled = true;
	Config.Motion.OpacitySpeed = -3.0f;
	Config.Visual.HoverScale = -2.0f;
	Config.Interaction.bEnabled = true;
	Config.Interaction.PressedScale = -4.0f;
	Config.DragPickup.bEnabled = true;
	Config.DragPickup.DurationSeconds = -1.0f;
	Config.Drag.CardDragStartThresholdPixels = -8.0f;

	const int32 BeforeLayer =
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).SlotRuntimeConfigPropagationCount;
	const int32 BeforeSlot =
		FWacomFirstPersonCardLayerTestAccess::View(*Slot).SlotRuntimeConfigApplyCount;
	Layer->SetSlotRuntimeConfig(Config);
	const FWacomFirstPersonCardLayerAutomationTestView LayerView =
		FWacomFirstPersonCardLayerTestAccess::View(*Layer);
	const FWacomFirstPersonCardSlotAutomationTestView SlotView =
		FWacomFirstPersonCardLayerTestAccess::View(*Slot);
	TestEqual(TEXT("One aggregate setter performs one Layer propagation"),
		LayerView.SlotRuntimeConfigPropagationCount, BeforeLayer + 1);
	TestEqual(TEXT("One aggregate setter performs one Slot apply"),
		SlotView.SlotRuntimeConfigApplyCount, BeforeSlot + 1);
	TestTrue(TEXT("Motion is normalized in the aggregate"),
		FMath::IsNearlyZero(SlotView.SlotRuntimeConfig.Motion.OpacitySpeed));
	TestTrue(TEXT("Visual is normalized in the aggregate"),
		FMath::IsNearlyEqual(SlotView.SlotRuntimeConfig.Visual.HoverScale, 0.01f));
	TestTrue(TEXT("Interaction is normalized in the aggregate"),
		FMath::IsNearlyEqual(SlotView.SlotRuntimeConfig.Interaction.PressedScale, 0.01f));
	TestTrue(TEXT("DragPickup is normalized independently"),
		FMath::IsNearlyZero(SlotView.SlotRuntimeConfig.DragPickup.DurationSeconds));
	TestTrue(TEXT("Drag is normalized independently"),
		FMath::IsNearlyZero(SlotView.SlotRuntimeConfig.Drag.CardDragStartThresholdPixels));

	FWacomFirstPersonCardSlotRuntimeConfig Equivalent = SlotView.SlotRuntimeConfig;
	Layer->SetSlotRuntimeConfig(Equivalent);
	TestEqual(TEXT("A normalized-equivalent aggregate skips Layer propagation"),
		FWacomFirstPersonCardLayerTestAccess::View(*Layer).SlotRuntimeConfigPropagationCount,
		LayerView.SlotRuntimeConfigPropagationCount);
	TestEqual(TEXT("A normalized-equivalent aggregate skips Slot reset/apply"),
		FWacomFirstPersonCardLayerTestAccess::View(*Slot).SlotRuntimeConfigApplyCount,
		SlotView.SlotRuntimeConfigApplyCount);

	return true;
}

#endif
