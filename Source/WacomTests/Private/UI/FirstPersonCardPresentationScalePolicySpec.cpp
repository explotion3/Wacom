// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "UI/Card/WacomFirstPersonCardPileTransferStyle.h"
#include "UI/FirstPersonCardLayerSpecReceiver.h"
#include "UI/FirstPersonCardLayerTestAccess.h"
#include "../../../WacomApp/Private/UI/Card/WacomFirstPersonCardPresentationScalePolicy.h"

#if WITH_AUTOMATION_TESTS

namespace
{
	struct FPresentationScaleCase
	{
		const TCHAR* Label;
		FVector2D ViewportPixels;
		float GlobalUIScale;
		float ExpectedPhysicalScale;
		float ExpectedPresentationScale;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardPresentationScalePolicySpec,
	"Wacom.UI.FirstPersonCardLayer.PresentationScale.Policy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardPresentationScalePolicySpec::RunTest(const FString&)
{
	const TArray<FPresentationScaleCase> Cases = {
		{ TEXT("720p"), FVector2D(1280.0f, 720.0f), 2.0f / 3.0f, 0.5f, 0.75f },
		{ TEXT("768p"), FVector2D(1366.0f, 768.0f), 768.0f / 1080.0f, 0.533f, 0.75f },
		{ TEXT("900p"), FVector2D(1600.0f, 900.0f), 900.0f / 1080.0f, 0.625f, 0.75f },
		{ TEXT("1080p"), FVector2D(1920.0f, 1080.0f), 1.0f, 0.75f, 0.75f },
		{ TEXT("1440p"), FVector2D(2560.0f, 1440.0f), 1.0f, 1.0f, 1.0f },
		{ TEXT("4K"), FVector2D(3840.0f, 2160.0f), 1.0f, 1.0f, 1.0f },
		{ TEXT("16:10"), FVector2D(1280.0f, 800.0f), 2.0f / 3.0f, 0.5f, 0.75f },
		{ TEXT("ultrawide 1080"), FVector2D(2560.0f, 1080.0f), 1.0f, 0.75f, 0.75f },
		{ TEXT("ultrawide 1440"), FVector2D(3440.0f, 1440.0f), 1.0f, 1.0f, 1.0f },
	};

	for (const FPresentationScaleCase& TestCase : Cases)
	{
		const FWacomFirstPersonCardPresentationScaleResult Result =
			FWacomFirstPersonCardPresentationScalePolicy::Resolve(
				TestCase.ViewportPixels,
				TestCase.GlobalUIScale);
		TestEqual(
			FString::Printf(TEXT("%s target physical scale"), TestCase.Label),
			Result.TargetPhysicalScale,
			TestCase.ExpectedPhysicalScale);
		TestEqual(
			FString::Printf(TEXT("%s compensated presentation scale"), TestCase.Label),
			Result.PresentationScale,
			TestCase.ExpectedPresentationScale);
	}

	const FWacomFirstPersonCardPresentationScaleResult Compensated =
		FWacomFirstPersonCardPresentationScalePolicy::Resolve(
			FVector2D(1920.0f, 1080.0f),
			0.75f);
	TestEqual(TEXT("global DPI is compensated before the local scale is applied"),
		Compensated.PresentationScale, 1.0f);

	for (const TPair<FVector2D, float>& InvalidInput : {
		TPair<FVector2D, float>(FVector2D::ZeroVector, 1.0f),
		TPair<FVector2D, float>(FVector2D(1920.0f, 1080.0f), 0.0f),
		TPair<FVector2D, float>(FVector2D(-1.0f, 1080.0f), 1.0f) })
	{
		const FWacomFirstPersonCardPresentationScaleResult Result =
			FWacomFirstPersonCardPresentationScalePolicy::Resolve(
				InvalidInput.Key,
				InvalidInput.Value);
		TestEqual(TEXT("invalid input falls back to unit physical scale"),
			Result.TargetPhysicalScale, 1.0f);
		TestEqual(TEXT("invalid input falls back to unit presentation scale"),
			Result.PresentationScale, 1.0f);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardPresentationScaleResolvedConfigSpec,
	"Wacom.UI.FirstPersonCardLayer.PresentationScale.ResolvedRuntimeSpatialConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardPresentationScaleResolvedConfigSpec::RunTest(const FString&)
{
	UWacomFirstPersonCardAnchorSpecProbeComponent* Anchor =
		NewObject<UWacomFirstPersonCardAnchorSpecProbeComponent>();
	TestNotNull(TEXT("anchor probe exists"), Anchor);
	if (!Anchor)
	{
		return false;
	}

	Anchor->ProbeViewportSize = FVector2D(1920.0f, 1080.0f);
	Anchor->ProbeViewportScale = 1.0f;
	Anchor->HandCardRenderScale = 0.55f;
	Anchor->AuthoredCardSpacingPixels = 120.0f;
	Anchor->HoverLiftPixels = 28.0f;
	Anchor->CardDragPickupLiftPixels = 12.0f;
	Anchor->DenyFeedbackShakePixels = 8.0f;
	Anchor->RetainedFeedbackLiftPixels = 12.0f;
	Anchor->DrawnCardEnterOffsetPixels = FVector2D(0.0f, 96.0f);
	Anchor->DrawnCardEnterArcLiftPixels = 42.0f;
	Anchor->CardDragStartThresholdPixels = 10.0f;

	const FWacomFirstPersonCardRestingPresentationProfile RestingProfile =
		Anchor->BuildRestingCardPresentationProfile();
	TestTrue(TEXT("resting hand profile is valid"), RestingProfile.IsValid());
	TestTrue(TEXT("resting hand profile exposes the formal card body"),
		RestingProfile.AuthoredCardBodySize.Equals(FVector2D(296.0f, 420.0f)));
	TestEqual(TEXT("resting hand profile excludes hover and keeps authored base scale"),
		RestingProfile.AuthoredRenderScale, 0.55f);

	UWacomFirstPersonCardPileTransferStyle* AuthoredStyle =
		NewObject<UWacomFirstPersonCardPileTransferStyle>(Anchor);
	AuthoredStyle->Style.GlyphSize = FVector2D(42.0f, 66.0f);
	AuthoredStyle->Style.MinArcHeightPixels = 48.0f;
	AuthoredStyle->Style.MaxArcHeightPixels = 128.0f;
	AuthoredStyle->Style.TrailHeadWidthPixels = 10.5f;
	AuthoredStyle->Style.MoteMaxSizePixels = 13.5f;
	AuthoredStyle->Style.SafeViewportPaddingPixels = 36.0f;
	Anchor->CardPileTransferStyle = AuthoredStyle;

	const FWacomFirstPersonCardAnchorAutomationTestView View =
		FWacomFirstPersonCardLayerTestAccess::View(*Anchor);
	TestEqual(TEXT("1080p presentation scale is 0.75"), View.PresentationScale, 0.75f);
	TestTrue(TEXT("card body render scale is scaled once"),
		FMath::IsNearlyEqual(View.ResolvedHandCardRenderScale, 0.55f * 0.75f));
	TestTrue(TEXT("hand spacing is scaled once"),
		FMath::IsNearlyEqual(View.ResolvedCardSpacingPixels, 120.0f * 0.75f));
	TestTrue(TEXT("hover lift is scaled once"),
		FMath::IsNearlyEqual(View.ResolvedHoverLiftPixels, 28.0f * 0.75f));
	TestTrue(TEXT("pickup lift is scaled once"),
		FMath::IsNearlyEqual(View.ResolvedDragPickupLiftPixels, 12.0f * 0.75f));
	TestTrue(TEXT("deny shake is scaled once"),
		FMath::IsNearlyEqual(View.ResolvedDenyShakePixels, 8.0f * 0.75f));
	TestTrue(TEXT("retained lift is scaled once"),
		FMath::IsNearlyEqual(View.ResolvedRetainedLiftPixels, 12.0f * 0.75f));
	TestTrue(TEXT("drawn entry offset is scaled once"),
		View.ResolvedDrawnEnterOffsetPixels.Equals(FVector2D(0.0f, 72.0f), 0.001f));
	TestTrue(TEXT("drawn arc is scaled once"),
		FMath::IsNearlyEqual(View.ResolvedDrawnEnterArcLiftPixels, 31.5f));

	TestTrue(TEXT("pile glyph size uses the runtime presentation copy"),
		View.ResolvedPileTransferStyle.GlyphSize.Equals(FVector2D(31.5f, 49.5f), 0.001f));
	TestTrue(TEXT("pile min/max arc heights scale"),
		FMath::IsNearlyEqual(View.ResolvedPileTransferStyle.MinArcHeightPixels, 36.0f)
		&& FMath::IsNearlyEqual(View.ResolvedPileTransferStyle.MaxArcHeightPixels, 96.0f));
	TestTrue(TEXT("pile trail and mote dimensions scale"),
		FMath::IsNearlyEqual(View.ResolvedPileTransferStyle.TrailHeadWidthPixels, 7.875f)
		&& FMath::IsNearlyEqual(View.ResolvedPileTransferStyle.MoteMaxSizePixels, 10.125f));
	TestEqual(TEXT("pile safe viewport padding is not presentation-scaled"),
		View.ResolvedPileTransferStyle.SafeViewportPaddingPixels, 36.0f);

	TestTrue(TEXT("the source DataAsset glyph size is unchanged"),
		AuthoredStyle->Style.GlyphSize.Equals(FVector2D(42.0f, 66.0f), 0.001f));
	TestEqual(TEXT("the source DataAsset arc height is unchanged"),
		AuthoredStyle->Style.MaxArcHeightPixels, 128.0f);
	TestEqual(TEXT("drag threshold remains an authored input contract"),
		Anchor->CardDragStartThresholdPixels, 10.0f);

	struct FExpectedCardPixels
	{
		FVector2D Viewport;
		float GlobalUIScale;
		FVector2D ExpectedSize;
	};
	for (const FExpectedCardPixels& Expected : {
		FExpectedCardPixels{ FVector2D(1280.0f, 720.0f), 2.0f / 3.0f, FVector2D(81.0f, 116.0f) },
		FExpectedCardPixels{ FVector2D(1920.0f, 1080.0f), 1.0f, FVector2D(122.0f, 173.0f) },
		FExpectedCardPixels{ FVector2D(2560.0f, 1440.0f), 1.0f, FVector2D(163.0f, 231.0f) } })
	{
		const FWacomFirstPersonCardPresentationScaleResult Scale =
			FWacomFirstPersonCardPresentationScalePolicy::Resolve(
				Expected.Viewport,
				Expected.GlobalUIScale);
		const FVector2D PhysicalSize = FVector2D(296.0f, 420.0f)
			* 0.55f
			* Scale.PresentationScale
			* Expected.GlobalUIScale;
		TestEqual(TEXT("card physical width follows the 1440p presentation reference"),
			FMath::RoundToInt(PhysicalSize.X), FMath::RoundToInt(Expected.ExpectedSize.X));
		TestEqual(TEXT("card physical height follows the 1440p presentation reference"),
			FMath::RoundToInt(PhysicalSize.Y), FMath::RoundToInt(Expected.ExpectedSize.Y));
	}

	return true;
}

#endif
