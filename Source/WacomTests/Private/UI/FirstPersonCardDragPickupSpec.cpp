// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Materials/Material.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Sound/SoundWave.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/FirstPersonCardLayerTestAccess.h"

#if WITH_AUTOMATION_TESTS

namespace WacomFirstPersonCardDragPickupSpec
{
	FWacomFirstPersonCardLayerSlotView MakeSlot(
		EWacomFirstPersonCardInteractionIntent Intent =
			EWacomFirstPersonCardInteractionIntent::DragToDropTarget)
	{
		FWacomFirstPersonCardLayerSlotView Slot;
		Slot.Index = 0;
		Slot.Entry.CardInstanceId = FGuid::NewGuid();
		Slot.Entry.Zone = EHandZone::Both;
		Slot.Entry.bIsPlayable = true;
		Slot.Entry.InteractionIntent = Intent;
		Slot.ScreenPosition = FVector2D(500.0f, 500.0f);
		Slot.WidgetPosition = Slot.ScreenPosition;
		Slot.SnappedWidgetPosition = Slot.ScreenPosition;
		Slot.InputHitCenter = Slot.ScreenPosition;
		Slot.InputHitScale = 1.0f;
		Slot.RenderScale = 1.0f;
		Slot.RenderOpacity = 1.0f;
		Slot.bProjected = true;
		return Slot;
	}

	FWacomFirstPersonCardDragPickupConfig MakeFeedbackConfig(
		bool bReducedMotion = false,
		USoundBase* Sound = nullptr)
	{
		FWacomFirstPersonCardDragPickupConfig Config;
		Config.bEnabled = true;
		Config.DurationSeconds = 0.14f;
		Config.RiseSeconds = 0.02f;
		Config.LiftPixels = 12.0f;
		Config.ScaleMultiplier = 1.03f;
		Config.bReducedMotion = bReducedMotion;
		Config.Sound = Sound;
		Config.SoundVolumeMultiplier = 1.0f;
		Config.SoundPitchMultiplier = 1.0f;
		Config.SoundPitchVariation = 0.03f;
		return Config;
	}

	UWacomFirstPersonCardLayerSlotWidget* MakeWidget(
		EWacomFirstPersonCardInteractionIntent Intent =
			EWacomFirstPersonCardInteractionIntent::DragToDropTarget,
		bool bReducedMotion = false,
		USoundBase* Sound = nullptr)
	{
		UWacomFirstPersonCardLayerSlotWidget* Widget =
			NewObject<UWacomFirstPersonCardLayerSlotWidget>();
		Widget->SetCardLayerInteractionEnabled(true);
		FWacomFirstPersonCardLayerTestAccess::SetDragPickupConfig(
			*Widget,
			MakeFeedbackConfig(bReducedMotion, Sound));

		FWacomFirstPersonCardSlotVisualConfig VisualConfig;
		VisualConfig.HoverLiftPixels = 0.0f;
		VisualConfig.HoverScale = 1.0f;
		VisualConfig.Selection.bEnabled = true;
		FWacomFirstPersonCardLayerTestAccess::SetSlotVisualConfig(*Widget, VisualConfig);

		FWacomFirstPersonCardDragConfig DragConfig;
		DragConfig.bEnableFirstPersonCardDragCommit = true;
		DragConfig.CardDragStartThresholdPixels = 1.0f;
		FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(*Widget, DragConfig);
		Widget->SetSlotViewImmediate(MakeSlot(Intent));
		return Widget;
	}

	void Tick(UWacomFirstPersonCardLayerSlotWidget& Widget, float DeltaTime)
	{
		FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(Widget, DeltaTime);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardDragPickupStateEdgeTest,
	"Wacom.UI.FirstPersonCardLayer.DragPickup.FormalStateEdgeAndMotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardDragPickupStateEdgeTest::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardDragPickupSpec;
	UWacomFirstPersonCardLayerSlotWidget* Widget = MakeWidget();
	if (!TestNotNull(TEXT("Drag pickup slot"), Widget))
	{
		return false;
	}

	const FVector2D HitSizeBeforeDrag = Widget->GetCardBodyHitSizeForFirstPersonLayer();
	FWacomFirstPersonCardLayerTestAccess::SetGestureState(
		*Widget,
		EWacomFirstPersonCardGestureState::Pressed);
	TestEqual(
		TEXT("Pressed does not trigger pickup"),
		FWacomFirstPersonCardLayerTestAccess::View(*Widget).DragPickupTriggerCount,
		0);
	FWacomFirstPersonCardLayerTestAccess::SetGestureState(
		*Widget,
		EWacomFirstPersonCardGestureState::Inspecting);
	TestEqual(
		TEXT("Inspect does not trigger pickup"),
		FWacomFirstPersonCardLayerTestAccess::View(*Widget).DragPickupTriggerCount,
		0);

	FWacomFirstPersonCardLayerTestAccess::SetGestureState(
		*Widget,
		EWacomFirstPersonCardGestureState::DraggingNoTargetCard);
	TestEqual(
		TEXT("Inspect to formal drag triggers once"),
		FWacomFirstPersonCardLayerTestAccess::View(*Widget).DragPickupTriggerCount,
		1);
	Tick(*Widget, 0.02f);
	const FWacomFirstPersonCardSlotAutomationTestView PeakView =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestTrue(TEXT("Pickup reaches its peak after the short rise"), PeakView.DragPickupAlpha > 0.99f);
	TestTrue(
		TEXT("Pickup adds the authored twelve-pixel local lift"),
		FMath::IsNearlyEqual(PeakView.RenderTransform.Translation.Y, -12.0f, 0.15f));
	TestTrue(
		TEXT("Pickup scale is multiplicative over the current visual slot"),
		FMath::IsNearlyEqual(
			PeakView.RenderTransform.Scale.X,
			Widget->GetVisualSlotView().RenderScale * 1.03f,
			0.002f));
	TestFalse(TEXT("Formal drag no longer activates Selection"), PeakView.SelectionView.bTargetActive);
	TestTrue(TEXT("Selection amount stays zero"), FMath::IsNearlyZero(PeakView.SelectionView.Amount));

	FWacomFirstPersonCardLayerTestAccess::SetGestureState(
		*Widget,
		EWacomFirstPersonCardGestureState::ArmedForCommit);
	TestEqual(
		TEXT("Formal drag state changes do not replay pickup"),
		FWacomFirstPersonCardLayerTestAccess::View(*Widget).DragPickupTriggerCount,
		1);
	FWacomFirstPersonCardLayerTestAccess::SetGestureState(
		*Widget,
		EWacomFirstPersonCardGestureState::Idle);
	const FWacomFirstPersonCardSlotAutomationTestView ClearedView =
		FWacomFirstPersonCardLayerTestAccess::View(*Widget);
	TestFalse(TEXT("Leaving formal drag clears pickup"), ClearedView.bDragPickupFeedbackActive);
	TestTrue(TEXT("Leaving formal drag clears pickup alpha"), FMath::IsNearlyZero(ClearedView.DragPickupAlpha));
	TestTrue(
		TEXT("Pickup does not change the authored hit body"),
		Widget->GetCardBodyHitSizeForFirstPersonLayer().Equals(HitSizeBeforeDrag, KINDA_SMALL_NUMBER));

	FWacomFirstPersonCardLayerTestAccess::SetGestureState(
		*Widget,
		EWacomFirstPersonCardGestureState::Inspecting);
	FWacomFirstPersonCardLayerTestAccess::SetGestureState(
		*Widget,
		EWacomFirstPersonCardGestureState::AimingTargetedCard);
	TestEqual(
		TEXT("A later inspect-to-aim promotion triggers a new pickup"),
		FWacomFirstPersonCardLayerTestAccess::View(*Widget).DragPickupTriggerCount,
		2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardDragPickupInputAndAudioTest,
	"Wacom.UI.FirstPersonCardLayer.DragPickup.InputPathsReducedMotionAndAudio",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardDragPickupInputAndAudioTest::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardDragPickupSpec;
	USoundWave* PickupSound = NewObject<USoundWave>();
	UWacomFirstPersonCardLayerSlotWidget* ReducedWidget =
		MakeWidget(EWacomFirstPersonCardInteractionIntent::DragToDropTarget, true, PickupSound);
	if (!TestNotNull(TEXT("Reduced pickup slot"), ReducedWidget))
	{
		return false;
	}

	TestTrue(
		TEXT("Keyboard/externally driven formal drag starts"),
		ReducedWidget->BeginDragGestureFromFirstPersonLayer(
			FVector2D(500.0f, 500.0f),
			FVector2D(520.0f, 460.0f)));
	const FWacomFirstPersonCardSlotAutomationTestView ReducedView =
		FWacomFirstPersonCardLayerTestAccess::View(*ReducedWidget);
	TestEqual(TEXT("External drag triggers pickup once"), ReducedView.DragPickupTriggerCount, 1);
	TestEqual(TEXT("Configured pickup sound requests once"), ReducedView.DragPickupSoundRequestCount, 1);
	TestFalse(TEXT("Reduced motion has no transient visual playback"), ReducedView.bDragPickupFeedbackActive);
	TestTrue(TEXT("Reduced motion has zero pickup alpha"), FMath::IsNearlyZero(ReducedView.DragPickupAlpha));
	TestTrue(
		TEXT("Randomized pitch remains inside the authored range"),
		ReducedView.LastDragPickupSoundPitchMultiplier >= 0.97f
			&& ReducedView.LastDragPickupSoundPitchMultiplier <= 1.03f);

	FWacomFirstPersonCardLayerTestAccess::SetGestureState(
		*ReducedWidget,
		EWacomFirstPersonCardGestureState::ArmedForCommit);
	TestEqual(
		TEXT("Armed state does not request another sound"),
		FWacomFirstPersonCardLayerTestAccess::View(*ReducedWidget).DragPickupSoundRequestCount,
		1);

	UWacomFirstPersonCardLayerSlotWidget* MouseWidget =
		MakeWidget(EWacomFirstPersonCardInteractionIntent::CommitNoTarget);
	TestTrue(
		TEXT("Mouse gesture press starts"),
		FWacomFirstPersonCardLayerTestAccess::RequestGesturePress(
			*MouseWidget,
			FVector2D(500.0f, 500.0f)));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(
		*MouseWidget,
		0.0f,
		FVector2D(500.0f, 450.0f));
	TestEqual(
		TEXT("Mouse promotion shares the pickup path"),
		FWacomFirstPersonCardLayerTestAccess::View(*MouseWidget).DragPickupTriggerCount,
		1);
	TestFalse(
		TEXT("Mouse promotion ends the pressed visual before pickup playback"),
		FWacomFirstPersonCardLayerTestAccess::View(*MouseWidget).bPressed);
	Tick(*MouseWidget, 0.02f);
	const FWacomFirstPersonCardSlotAutomationTestView MousePeakView =
		FWacomFirstPersonCardLayerTestAccess::View(*MouseWidget);
	TestTrue(
		TEXT("Mouse drag pickup reaches its visual peak"),
		MousePeakView.DragPickupAlpha > 0.99f);
	TestTrue(
		TEXT("Pressed scale no longer suppresses the mouse drag pickup scale"),
		FMath::IsNearlyEqual(
			MousePeakView.RenderTransform.Scale.X,
			MouseWidget->GetVisualSlotView().RenderScale * 1.03f,
			0.002f));
	TestEqual(
		TEXT("Missing sound asset safely produces no request"),
		FWacomFirstPersonCardLayerTestAccess::View(*MouseWidget).DragPickupSoundRequestCount,
		0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardDragPickupShortcutAcquireTest,
	"Wacom.UI.FirstPersonCardLayer.DragPickup.ShortcutWaitsForPointerAcquire",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardDragPickupShortcutAcquireTest::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomFirstPersonCardDragPickupSpec;
	USoundWave* PickupSound = NewObject<USoundWave>();
	UWacomFirstPersonCardLayerSlotWidget* FarWidget = MakeWidget(
		EWacomFirstPersonCardInteractionIntent::DragToDropTarget,
		false,
		PickupSound);
	if (!TestNotNull(TEXT("Far shortcut pickup slot"), FarWidget))
	{
		return false;
	}

	const FVector2D CardPosition(500.0f, 500.0f);
	const FVector2D FarPointerPosition(500.0f, 100.0f);
	TestTrue(
		TEXT("Far keyboard shortcut starts"),
		FarWidget->BeginDragGestureFromFirstPersonLayer(CardPosition, FarPointerPosition));
	TestEqual(
		TEXT("Far shortcut still requests pickup audio immediately"),
		FWacomFirstPersonCardLayerTestAccess::View(*FarWidget).DragPickupSoundRequestCount,
		1);

	Tick(*FarWidget, 0.02f);
	TestTrue(
		TEXT("Pickup visual waits while the first shortcut card is still acquiring the pointer"),
		FMath::IsNearlyZero(
			FWacomFirstPersonCardLayerTestAccess::View(*FarWidget).DragPickupAlpha));

	Tick(*FarWidget, 0.30f);
	TestEqual(
		TEXT("Far shortcut card reaches the pointer before the pickup pulse"),
		FarWidget->GetVisualSlotView().ScreenPosition,
		FarPointerPosition);
	Tick(*FarWidget, 0.02f);
	TestTrue(
		TEXT("Pickup visual reaches its peak after pointer acquisition"),
		FWacomFirstPersonCardLayerTestAccess::View(*FarWidget).DragPickupAlpha > 0.99f);

	UWacomFirstPersonCardLayerSlotWidget* NearWidget = MakeWidget();
	if (!TestNotNull(TEXT("Near shortcut pickup slot"), NearWidget))
	{
		return false;
	}
	TestTrue(
		TEXT("Near keyboard shortcut starts"),
		NearWidget->BeginDragGestureFromFirstPersonLayer(CardPosition, CardPosition));
	Tick(*NearWidget, 0.02f);
	TestTrue(
		TEXT("A shortcut that is already at the pointer keeps the immediate pickup pulse"),
		FWacomFirstPersonCardLayerTestAccess::View(*NearWidget).DragPickupAlpha > 0.99f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardDormantPixelPrismSourceTest,
	"Wacom.UI.FirstPersonCardLayer.DragPickup.PixelPrismIsDormant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardDormantPixelPrismSourceTest::RunTest(const FString& /*Parameters*/)
{
	FString MaterialSource;
	const FString MaterialSourcePath = FPaths::Combine(
		FPaths::ProjectDir(),
		TEXT("DShader/Material/Card/M_FirstPersonCard_Fake3D.dsm"));
	TestTrue(
		TEXT("Fake3D DreamShader source can be read"),
		FFileHelper::LoadFileToString(MaterialSource, *MaterialSourcePath));
	TestFalse(
		TEXT("Active Fake3D source no longer samples Selection noise"),
		MaterialSource.Contains(TEXT("SelectionNoiseTexture")));
	TestFalse(
		TEXT("Active Fake3D source no longer exposes Selection amount"),
		MaterialSource.Contains(TEXT("SelectionAmount")));

	FString PixelPrismHeader;
	const FString PixelPrismHeaderPath = FPaths::Combine(
		FPaths::ProjectDir(),
		TEXT("DShader/Shared/WacomCardPixelPrism.dsh"));
	TestTrue(
		TEXT("Dormant pixel-prism header can be read"),
		FFileHelper::LoadFileToString(PixelPrismHeader, *PixelPrismHeaderPath));
	TestTrue(
		TEXT("Pixel quantization helper is retained"),
		PixelPrismHeader.Contains(TEXT("WacomCardPixelPrism_QuantizeSurfaceUV")));
	TestTrue(
		TEXT("Pixel sweep helper is retained"),
		PixelPrismHeader.Contains(TEXT("WacomCardPixelPrism_QuantizedSweep")));
	TestTrue(
		TEXT("Pixel glint helper is retained"),
		PixelPrismHeader.Contains(TEXT("WacomCardPixelPrism_GlintClusters")));

	UMaterial* GeneratedMaterial = LoadObject<UMaterial>(
		nullptr,
		TEXT("/Game/DreamMaterials/Card/M_FirstPersonCard_Fake3D.M_FirstPersonCard_Fake3D"));
	if (TestNotNull(TEXT("Generated Fake3D material exists"), GeneratedMaterial))
	{
		bool bFoundSelectionNoiseParameter = false;
		for (const TObjectPtr<UMaterialExpression>& Expression : GeneratedMaterial->GetExpressions())
		{
			const UMaterialExpressionTextureSampleParameter2D* TextureParameter =
				Cast<UMaterialExpressionTextureSampleParameter2D>(Expression.Get());
			bFoundSelectionNoiseParameter |= TextureParameter
				&& TextureParameter->ParameterName == TEXT("SelectionNoiseTexture");
		}
		TestFalse(
			TEXT("Generated Fake3D material no longer contains Selection noise"),
			bFoundSelectionNoiseParameter);
	}
	return true;
}

#endif
