// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include <limits>

#include "Editor/RunFloorSceneValidationTestFixture.h"

using namespace WacomRunFloorSceneValidationTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunFloorSceneSplineShapeSpec,
	"Wacom.Editor.RunSceneValidation.Geometry.ShapeAndDirection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunFloorSceneSplineShapeSpec::RunTest(const FString& Parameters)
{
	{
		FFixture Fixture;
		FFixture::SetSplinePoints(*Fixture.Path, {FVector::ZeroVector});
		TestTrue(TEXT("Spline requires at least two points"), HasCode(
			FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World),
			EWacomRunSceneBindingDiagnosticCode::SplinePointCountInvalid));
	}
	{
		FFixture Fixture;
		FFixture::SetSplinePoints(*Fixture.Path,
			{FVector::ZeroVector, FVector(10.0, 0.0, 0.0)});
		TestTrue(TEXT("Spline length must be greater than 10cm"), HasCode(
			FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World),
			EWacomRunSceneBindingDiagnosticCode::SplineLengthTooShort));
	}
	{
		FFixture Fixture;
		const float NaN = std::numeric_limits<float>::quiet_NaN();
		Fixture.Path->GetPathSpline()->SetScaleAtSplinePoint(
			0, FVector(NaN, 1.0, 1.0), false);
		Fixture.Path->GetPathSpline()->UpdateSpline();
		TestTrue(TEXT("Non-finite spline point transform fails"), HasCode(
			FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World),
			EWacomRunSceneBindingDiagnosticCode::SplineTransformNonFinite));
	}
	{
		FFixture Fixture;
		FFixture::SetSplinePoints(*Fixture.Path,
			{Fixture.EventAnchor->GetActorLocation(), Fixture.EntryAnchor->GetActorLocation()});
		TestTrue(TEXT("Reversed source/target direction fails"), HasCode(
			FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World),
			EWacomRunSceneBindingDiagnosticCode::SplineDirectionReversed));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunFloorSceneSplineEndpointThresholdSpec,
	"Wacom.Editor.RunSceneValidation.Geometry.EndpointThresholds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunFloorSceneSplineEndpointThresholdSpec::RunTest(const FString& Parameters)
{
	auto ValidateSourceOffset = [](const double Distance)
	{
		FFixture Fixture;
		FFixture::SetSplinePoints(*Fixture.Path,
			{FVector(0.0, Distance, 0.0), Fixture.EventAnchor->GetActorLocation()});
		return FWacomRunSceneBindingValidation::ValidateLoadedWorld(Fixture.World);
	};

	const FWacomRunSceneBindingValidationReport At100 = ValidateSourceOffset(100.0);
	TestFalse(TEXT("100cm source endpoint passes without warning"),
		HasCode(At100, EWacomRunSceneBindingDiagnosticCode::SplineSourceEndpointWarning));
	TestFalse(TEXT("100cm source endpoint passes without error"),
		HasCode(At100, EWacomRunSceneBindingDiagnosticCode::SplineSourceEndpointError));

	const FWacomRunSceneBindingValidationReport Above100 = ValidateSourceOffset(100.01);
	TestTrue(TEXT("Above 100cm warns"), HasDiagnostic(Above100,
		EWacomRunSceneBindingDiagnosticSeverity::Warning,
		EWacomRunSceneBindingDiagnosticCode::SplineSourceEndpointWarning));
	TestTrue(TEXT("Endpoint warning does not invalidate the scene"), Above100.IsValid());

	const FWacomRunSceneBindingValidationReport At300 = ValidateSourceOffset(300.0);
	TestTrue(TEXT("300cm remains a warning"), HasDiagnostic(At300,
		EWacomRunSceneBindingDiagnosticSeverity::Warning,
		EWacomRunSceneBindingDiagnosticCode::SplineSourceEndpointWarning));

	const FWacomRunSceneBindingValidationReport Above300 = ValidateSourceOffset(300.01);
	TestTrue(TEXT("Above 300cm is an error"), HasDiagnostic(Above300,
		EWacomRunSceneBindingDiagnosticSeverity::Error,
		EWacomRunSceneBindingDiagnosticCode::SplineSourceEndpointError));
	TestFalse(TEXT("Endpoint error invalidates the scene"), Above300.IsValid());
	return true;
}

#endif
