// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Components/WacomCursorLookDriverComponent.h"
#include "Components/WacomRunPathTraversalComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomWorldShopCursorLookProfileSpec,
	"Wacom.UI.WorldShop.CursorLookProfile.LiveRunCopyAndSanitize",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomWorldShopCursorLookProfileSpec::RunTest(const FString& Parameters)
{
	UWacomRunPathTraversalComponent* RunPath = NewObject<UWacomRunPathTraversalComponent>();
	RunPath->YawClampDegrees = 17.0f;
	RunPath->PitchClampDegrees = 9.0f;
	RunPath->LookYawScale = 1.25f;
	RunPath->LookPitchScale = 0.75f;
	RunPath->LookInterpSpeed = 14.0f;
	const FWacomCursorLookProfile Profile = RunPath->GetLiveCursorLookProfile();
	TestEqual(TEXT("live yaw copied"), Profile.YawClampDegrees, 17.0f);
	TestEqual(TEXT("live pitch copied"), Profile.PitchClampDegrees, 9.0f);
	TestEqual(TEXT("live interpolation copied"), Profile.LookInterpSpeed, 14.0f);

	Profile.Sanitized();
	TestEqual(TEXT("copy does not mutate authored yaw"), RunPath->YawClampDegrees, 17.0f);

	FWacomCursorLookProfile Invalid;
	Invalid.YawClampDegrees = -5.0f;
	Invalid.PitchClampDegrees = -3.0f;
	Invalid.CursorDeadZoneNormalized = FVector2D(-2.0f, 1.0f);
	Invalid.CursorResponseExponent = -4.0f;
	Invalid.LookInterpSpeed = -2.0f;
	const FWacomCursorLookProfile Sanitized = Invalid.Sanitized();
	TestEqual(TEXT("yaw becomes positive"), Sanitized.YawClampDegrees, 5.0f);
	TestEqual(TEXT("pitch becomes positive"), Sanitized.PitchClampDegrees, 3.0f);
	TestTrue(TEXT("horizontal deadzone clamps below one"),
		FMath::IsNearlyEqual(
			Sanitized.CursorDeadZoneNormalized.X,
			0.99,
			0.0001));
	TestTrue(TEXT("vertical deadzone clamps below one"),
		FMath::IsNearlyEqual(
			Sanitized.CursorDeadZoneNormalized.Y,
			0.99,
			0.0001));
	TestEqual(TEXT("response exponent remains positive"),
		Sanitized.CursorResponseExponent, 0.01f);
	TestEqual(TEXT("interpolation cannot be negative"), Sanitized.LookInterpSpeed, 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomWorldShopCursorLookResponseCurveSpec,
	"Wacom.UI.WorldShop.CursorLookProfile.DeadZoneExponentAndExactClamp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomWorldShopCursorLookResponseCurveSpec::RunTest(
	const FString& Parameters)
{
	UWacomCursorLookDriverComponent* Driver =
		NewObject<UWacomCursorLookDriverComponent>();
	FWacomCursorLookProfile Profile;
	Profile.YawClampDegrees = 30.0f;
	Profile.PitchClampDegrees = 12.0f;
	Profile.CursorDeadZoneNormalized = FVector2D(0.12f, 0.18f);
	Profile.CursorResponseExponent = 1.35f;
	Profile.LookInterpSpeed = 0.0f;

	Driver->UpdateFromNormalizedCursor(FVector2D(0.10f, -0.17f), 0.0f, Profile);
	TestTrue(TEXT("both axes are stable inside deadzone"),
		Driver->GetCurrentLookOffset2D().IsNearlyZero());

	Driver->UpdateFromNormalizedCursor(FVector2D(1.0f, -1.0f), 0.0f, Profile);
	TestTrue(TEXT("positive edge reaches exact yaw clamp"),
		FMath::IsNearlyEqual(Driver->GetCurrentLookOffset2D().X, 30.0f));
	TestTrue(TEXT("negative screen Y reaches exact positive pitch clamp"),
		FMath::IsNearlyEqual(Driver->GetCurrentLookOffset2D().Y, 12.0f));

	Driver->UpdateFromNormalizedCursor(FVector2D(-1.0f, 1.0f), 0.0f, Profile);
	TestTrue(TEXT("negative edge reaches exact yaw clamp"),
		FMath::IsNearlyEqual(Driver->GetCurrentLookOffset2D().X, -30.0f));
	TestTrue(TEXT("positive screen Y reaches exact negative pitch clamp"),
		FMath::IsNearlyEqual(Driver->GetCurrentLookOffset2D().Y, -12.0f));

	const float AxisInput = 0.56f;
	const float ExpectedYaw = 30.0f * FMath::Pow(
		(AxisInput - 0.12f) / (1.0f - 0.12f),
		1.35f);
	Driver->UpdateFromNormalizedCursor(
		FVector2D(AxisInput, 0.0f),
		0.0f,
		Profile);
	TestTrue(TEXT("positive curve follows remap and exponent"),
		FMath::IsNearlyEqual(
			Driver->GetCurrentLookOffset2D().X,
			ExpectedYaw,
			0.001f));
	Driver->UpdateFromNormalizedCursor(
		FVector2D(-AxisInput, 0.0f),
		0.0f,
		Profile);
	TestTrue(TEXT("response curve is symmetric"),
		FMath::IsNearlyEqual(
			Driver->GetCurrentLookOffset2D().X,
			-ExpectedYaw,
			0.001f));

	FWacomCursorLookProfile DefaultProfile;
	DefaultProfile.YawClampDegrees = 20.0f;
	DefaultProfile.PitchClampDegrees = 10.0f;
	DefaultProfile.LookInterpSpeed = 0.0f;
	Driver->UpdateFromNormalizedCursor(
		FVector2D(0.5f, -0.5f),
		0.0f,
		DefaultProfile);
	TestTrue(TEXT("default profile preserves legacy linear mapping"),
		Driver->GetCurrentLookOffset2D().Equals(
			FVector2D(10.0f, 5.0f),
			0.001f));
	return true;
}
