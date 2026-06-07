// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "UI/Card/WacomFirstPersonCardLayoutPreset.h"
#include "Validation/FirstPersonCardLayoutPresetValidation.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
	UWacomFirstPersonCardLayoutPreset* MakeValidFirstPersonCardLayoutPreset(UObject* Outer)
	{
		UWacomFirstPersonCardLayoutPreset* Preset =
			NewObject<UWacomFirstPersonCardLayoutPreset>(Outer);
		Preset->ProjectionMode = EWacomFirstPersonCardProjectionMode::LegacyWorldProjected;
		Preset->LookInfluenceYaw = 0.25f;
		Preset->LookInfluencePitch = 0.15f;
		return Preset;
	}

	FWacomFirstPersonCardLayoutPresetValidationReport BuildLayoutPresetReportForTest(
		const UWacomFirstPersonCardLayoutPreset* Preset)
	{
		return FWacomFirstPersonCardLayoutPresetValidation::BuildReport(Preset);
	}

	bool TextArrayContains(const TArray<FText>& Texts, const TCHAR* ExpectedText)
	{
		for (const FText& Text : Texts)
		{
			if (Text.ToString().Contains(ExpectedText))
			{
				return true;
			}
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIFirstPersonCardLayoutPresetValidationValidSpec,
	"Wacom.UI.FirstPersonCardLayoutPreset.Validation.ValidDefaultPresetPasses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIFirstPersonCardLayoutPresetValidationValidSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomFirstPersonCardLayoutPreset> Preset(
		MakeValidFirstPersonCardLayoutPreset(GetTransientPackage()));

	const FWacomFirstPersonCardLayoutPresetValidationReport Report =
		BuildLayoutPresetReportForTest(Preset.Get());
	TestTrue(TEXT("Valid first-person card layout preset passes"), Report.IsValid());
	TestEqual(TEXT("No validation errors"), Report.Errors.Num(), 0);
	TestEqual(TEXT("No validation warnings"), Report.Warnings.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIFirstPersonCardLayoutPresetValidationInvalidMathSpec,
	"Wacom.UI.FirstPersonCardLayoutPreset.Validation.InvalidMathValuesFail",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIFirstPersonCardLayoutPresetValidationInvalidMathSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomFirstPersonCardLayoutPreset> Preset(
		MakeValidFirstPersonCardLayoutPreset(GetTransientPackage()));
	Preset->StaticCardRenderScale = 0.0f;
	Preset->CardLayerPixelSnapGrid = 0.0f;
	Preset->EdgeDropScaleMinCardCount = 12;
	Preset->EdgeDropScaleMaxCardCount = 5;

	const FWacomFirstPersonCardLayoutPresetValidationReport Report =
		BuildLayoutPresetReportForTest(Preset.Get());
	TestFalse(TEXT("Invalid math values fail"), Report.IsValid());
	TestTrue(TEXT("Scale error is reported"), TextArrayContains(Report.Errors, TEXT("StaticCardRenderScale")));
	TestTrue(TEXT("Snap grid error is reported"), TextArrayContains(Report.Errors, TEXT("CardLayerPixelSnapGrid")));
	TestTrue(TEXT("Edge drop range error is reported"), TextArrayContains(Report.Errors, TEXT("EdgeDropScaleMaxCardCount")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIFirstPersonCardLayoutPresetValidationViewportAnchorSpec,
	"Wacom.UI.FirstPersonCardLayoutPreset.Validation.InvalidViewportAnchorsFail",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIFirstPersonCardLayoutPresetValidationViewportAnchorSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomFirstPersonCardLayoutPreset> Preset(
		MakeValidFirstPersonCardLayoutPreset(GetTransientPackage()));
	Preset->DrawnCardEnterViewportAnchor = FVector2D(1.2f, 0.5f);

	const FWacomFirstPersonCardLayoutPresetValidationReport Report =
		BuildLayoutPresetReportForTest(Preset.Get());
	TestFalse(TEXT("Invalid viewport anchor fails"), Report.IsValid());
	TestTrue(TEXT("Viewport anchor field is reported"),
		TextArrayContains(Report.Errors, TEXT("DrawnCardEnterViewportAnchor")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIFirstPersonCardLayoutPresetValidationWarningsSpec,
	"Wacom.UI.FirstPersonCardLayoutPreset.Validation.AuthoringRiskWarningsDoNotFail",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIFirstPersonCardLayoutPresetValidationWarningsSpec::RunTest(const FString& /*Parameters*/)
{
	{
		TStrongObjectPtr<UWacomFirstPersonCardLayoutPreset> Preset(
			MakeValidFirstPersonCardLayoutPreset(GetTransientPackage()));
		Preset->ProjectionMode = EWacomFirstPersonCardProjectionMode::BodyLocked;
		Preset->LookInfluenceYaw = 0.25f;
		Preset->LookInfluencePitch = 0.15f;

		const FWacomFirstPersonCardLayoutPresetValidationReport Report =
			BuildLayoutPresetReportForTest(Preset.Get());
		TestTrue(TEXT("BodyLocked look influence warning does not fail"), Report.IsValid());
		TestTrue(TEXT("BodyLocked look warning is reported"),
			TextArrayContains(Report.Warnings, TEXT("BodyLocked")));
	}

	{
		TStrongObjectPtr<UWacomFirstPersonCardLayoutPreset> Preset(
			MakeValidFirstPersonCardLayoutPreset(GetTransientPackage()));
		Preset->ProjectionMode = EWacomFirstPersonCardProjectionMode::LegacyWorldProjected;
		Preset->LookInfluenceYaw = 0.5f;
		Preset->LookInfluencePitch = 0.35f;
		Preset->StaticCardEdgeDropPixels = 220.0f;

		const FWacomFirstPersonCardLayoutPresetValidationReport Report =
			BuildLayoutPresetReportForTest(Preset.Get());
		TestTrue(TEXT("High risk values warn but remain valid"), Report.IsValid());
		TestTrue(TEXT("High yaw warning is reported"),
			TextArrayContains(Report.Warnings, TEXT("LookInfluenceYaw")));
		TestTrue(TEXT("High pitch warning is reported"),
			TextArrayContains(Report.Warnings, TEXT("LookInfluencePitch")));
		TestTrue(TEXT("High edge drop warning is reported"),
			TextArrayContains(Report.Warnings, TEXT("StaticCardEdgeDropPixels")));
	}

	return true;
}
