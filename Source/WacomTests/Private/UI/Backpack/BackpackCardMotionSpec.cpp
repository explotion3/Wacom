// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../../../../WacomApp/Private/UI/Card/WacomCardMotionKernel.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardMotionKernelSpec,
	"Wacom.UI.Backpack.Motion.SharedKernel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardMotionKernelSpec::RunTest(const FString& Parameters)
{
	auto Simulate = [](int32 FramesPerSecond)
	{
		FVector2D Position = FVector2D::ZeroVector;
		const FVector2D Target(480.0f, 270.0f);
		const float DeltaTime = 1.0f / static_cast<float>(FramesPerSecond);
		for (int32 Frame = 0; Frame < FramesPerSecond; ++Frame)
		{
			Position = FWacomCardMotionKernel::StepExponential(
				Position, Target, 12.0f, DeltaTime);
		}
		return Position;
	};
	const FVector2D At30 = Simulate(30);
	const FVector2D At60 = Simulate(60);
	const FVector2D At120 = Simulate(120);
	TestTrue(TEXT("Exponential motion has the same one-second result at 30 and 60 FPS"),
		At30.Equals(At60, 0.01f));
	TestTrue(TEXT("Exponential motion has the same one-second result at 60 and 120 FPS"),
		At60.Equals(At120, 0.01f));

	const FVector2D Lagged = FWacomCardMotionKernel::StepExponentialWithMaximumLag(
		FVector2D::ZeroVector,
		FVector2D(1000.0f, 0.0f),
		34.0f,
		14.0f,
		1.0f / 120.0f);
	TestTrue(TEXT("Visual carry spring never exceeds its maximum lag"),
		FVector2D::Distance(Lagged, FVector2D(1000.0f, 0.0f)) <= 14.01f);

	const float MidAngle = FWacomCardMotionKernel::LerpAngleShortest(170.0f, -170.0f, 0.5f);
	TestTrue(TEXT("Angle interpolation takes the shortest path across 180 degrees"),
		FMath::Abs(FMath::Abs(MidAngle) - 180.0f) <= 0.01f);

	const FVector2D BeforeRetarget = FWacomCardMotionKernel::StepExponential(
		FVector2D::ZeroVector, FVector2D(100.0f, 0.0f), 10.0f, 1.0f / 60.0f);
	const FVector2D AfterRetarget = FWacomCardMotionKernel::StepExponential(
		BeforeRetarget, FVector2D(25.0f, 80.0f), 10.0f, 1.0f / 60.0f);
	TestTrue(TEXT("Retargeting continues from the current visual position"),
		FVector2D::Distance(AfterRetarget, BeforeRetarget)
			< FVector2D::Distance(FVector2D(25.0f, 80.0f), BeforeRetarget));
	return true;
}

#endif
