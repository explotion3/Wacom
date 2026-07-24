// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "UI/Card/WacomFirstPersonCardPresentationMetrics.h"
#include "../../../WacomApp/Private/UI/Battle/WacomBattleCardPileThumbnailScalePolicy.h"
#include "../../../WacomApp/Private/UI/Common/WacomViewportPresentationScalePolicy.h"

#if WITH_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIViewportPresentationScalePolicySpec,
	"Wacom.UI.PresentationScale.SharedKernel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIViewportPresentationScalePolicySpec::RunTest(const FString&)
{
	FWacomViewportPresentationScaleProfile Profile;
	Profile.ReferenceViewportPixels = FVector2D(1920.0f, 1080.0f);
	Profile.MinimumTargetPhysicalScale = 0.90f;
	Profile.MaximumTargetPhysicalScale = 1.15f;
	Profile.MinimumLocalScale = 0.5f;
	Profile.MaximumLocalScale = 2.0f;

	const FWacomViewportPresentationScaleResult Result =
		FWacomViewportPresentationScalePolicy::Resolve(
			FVector2D(1280.0f, 720.0f),
			2.0f / 3.0f,
			Profile);
	TestEqual(TEXT("shared kernel clamps physical scale before DPI compensation"),
		Result.TargetPhysicalScale, 0.90f);
	TestEqual(TEXT("shared kernel compensates global DPI exactly once"),
		Result.LocalScale, 1.35f);

	Profile.MaximumTargetPhysicalScale = 0.80f;
	const FWacomViewportPresentationScaleResult InvalidProfile =
		FWacomViewportPresentationScalePolicy::Resolve(
			FVector2D(1920.0f, 1080.0f),
			1.0f,
			Profile);
	TestEqual(TEXT("invalid profiles return unit physical scale"),
		InvalidProfile.TargetPhysicalScale, 1.0f);
	TestEqual(TEXT("invalid profiles return unit local scale"),
		InvalidProfile.LocalScale, 1.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCardPileThumbnailScalePolicySpec,
	"Wacom.UI.Battle.CardPileDetails.Responsive.Policy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCardPileThumbnailScalePolicySpec::RunTest(const FString&)
{
	struct FCase
	{
		const TCHAR* Label;
		FVector2D Viewport;
		float GlobalUIScale;
		float ExpectedPhysicalScale;
		float ExpectedLocalScale;
	};
	for (const FCase& TestCase : {
		FCase{ TEXT("720p"), FVector2D(1280.0f, 720.0f), 2.0f / 3.0f, 0.90f, 1.35f },
		FCase{ TEXT("1080p"), FVector2D(1920.0f, 1080.0f), 1.0f, 1.0f, 1.0f },
		FCase{ TEXT("1440p"), FVector2D(2560.0f, 1440.0f), 1.0f, 1.15f, 1.15f },
		FCase{ TEXT("4K"), FVector2D(3840.0f, 2160.0f), 1.0f, 1.15f, 1.15f },
		FCase{ TEXT("1080p ultrawide"), FVector2D(2560.0f, 1080.0f), 1.0f, 1.0f, 1.0f } })
	{
		const FWacomBattleCardPileThumbnailScaleResult Result =
			FWacomBattleCardPileThumbnailScalePolicy::Resolve(
				TestCase.Viewport,
				TestCase.GlobalUIScale);
		TestEqual(
			FString::Printf(TEXT("%s physical scale"), TestCase.Label),
			Result.TargetPhysicalScale,
			TestCase.ExpectedPhysicalScale);
		TestEqual(
			FString::Printf(TEXT("%s local scale"), TestCase.Label),
			Result.LocalScale,
			TestCase.ExpectedLocalScale);
	}

	const FWacomBattleCardPileThumbnailScaleResult Invalid =
		FWacomBattleCardPileThumbnailScalePolicy::Resolve(
			FVector2D::ZeroVector,
			0.0f);
	TestEqual(TEXT("invalid pile viewport falls back to reference physical scale"),
		Invalid.TargetPhysicalScale, 1.0f);
	TestEqual(TEXT("invalid pile viewport falls back to reference local scale"),
		Invalid.LocalScale, 1.0f);

	FWacomFirstPersonCardRestingPresentationProfile HandProfile;
	HandProfile.AuthoredRenderScale = 0.92f;
	struct FHandMatchCase
	{
		const TCHAR* Label;
		FVector2D Viewport;
		float GlobalUIScale;
		FVector2D ExpectedPhysicalSize;
		FVector2D ExpectedLogicalSize;
	};
	for (const FHandMatchCase& TestCase : {
		FHandMatchCase{
			TEXT("720p hand parity"),
			FVector2D(1280.0f, 720.0f),
			2.0f / 3.0f,
			FVector2D(136.16f, 193.20f),
			FVector2D(204.24f, 289.80f) },
		FHandMatchCase{
			TEXT("1080p hand parity"),
			FVector2D(1920.0f, 1080.0f),
			1.0f,
			FVector2D(204.24f, 289.80f),
			FVector2D(204.24f, 289.80f) },
		FHandMatchCase{
			TEXT("1440p hand parity"),
			FVector2D(2560.0f, 1440.0f),
			1.0f,
			FVector2D(272.32f, 386.40f),
			FVector2D(272.32f, 386.40f) },
		FHandMatchCase{
			TEXT("4K hand parity"),
			FVector2D(3840.0f, 2160.0f),
			1.0f,
			FVector2D(272.32f, 386.40f),
			FVector2D(272.32f, 386.40f) },
		FHandMatchCase{
			TEXT("1080p ultrawide hand parity"),
			FVector2D(2560.0f, 1080.0f),
			1.0f,
			FVector2D(204.24f, 289.80f),
			FVector2D(204.24f, 289.80f) } })
	{
		const FWacomBattleCardPileHandSizeMatchResult Match =
			FWacomBattleCardPileThumbnailScalePolicy::ResolveMatchingRestingHand(
				HandProfile,
				TestCase.Viewport,
				TestCase.GlobalUIScale);
		TestTrue(
			FString::Printf(TEXT("%s produces a valid match"), TestCase.Label),
			Match.bValid);
		TestTrue(
			FString::Printf(TEXT("%s matches the resting hand physical card body"), TestCase.Label),
			Match.PhysicalCardBodySize.Equals(TestCase.ExpectedPhysicalSize, 0.01f));
		TestTrue(
			FString::Printf(TEXT("%s compensates global DPI exactly once"), TestCase.Label),
			Match.LogicalCardBodySize.Equals(TestCase.ExpectedLogicalSize, 0.01f));
	}

	const FWacomBattleCardPileHandSizeMatchResult MissingHandProfile =
		FWacomBattleCardPileThumbnailScalePolicy::ResolveMatchingRestingHand(
			FWacomFirstPersonCardRestingPresentationProfile(),
			FVector2D(1920.0f, 1080.0f),
			1.0f);
	TestFalse(TEXT("missing active hand profile falls back to pile policy"),
		MissingHandProfile.bValid);
	return true;
}

#endif
