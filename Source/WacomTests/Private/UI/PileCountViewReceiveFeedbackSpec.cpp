// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "UI/Common/PileCountView.h"
#include "UI/PileCountViewTestAccess.h"

#include "Components/TextBlock.h"

#if WITH_AUTOMATION_TESTS

namespace
{
	bool IsTransformEqual(
		const FWidgetTransform& Actual,
		const FWidgetTransform& Expected,
		float Tolerance = 0.001f)
	{
		return Actual.Translation.Equals(Expected.Translation, Tolerance)
			&& Actual.Scale.Equals(Expected.Scale, Tolerance)
			&& Actual.Shear.Equals(Expected.Shear, Tolerance)
			&& FMath::IsNearlyEqual(Actual.Angle, Expected.Angle, Tolerance);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomPileCountViewReceiveFeedbackCurveSpec,
	"Wacom.UI.Common.PileCount.ReceiveFeedback.CurveAndDisplayContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomPileCountViewReceiveFeedbackCurveSpec::RunTest(const FString&)
{
	UPileCountView* Widget = FWacomPileCountViewTestAccess::CreateWidget();
	TestNotNull(TEXT("pile count widget is created"), Widget);
	if (!Widget)
	{
		return false;
	}

	const FWidgetTransform AuthoredTransform = Widget->GetRenderTransform();
	Widget->SetCount(7);
	TestTrue(TEXT("SetCount does not start receive feedback"),
		IsTransformEqual(Widget->GetRenderTransform(), AuthoredTransform));

	Widget->PlayReceiveFeedback(1, false, false);
	FWacomPileCountViewTestAccess::Tick(*Widget, 0.04f);
	TestTrue(TEXT("single arrival reaches authored compression scale"),
		Widget->GetRenderTransform().Scale.Equals(FVector2D(1.03f, 0.94f), 0.001f));
	TestTrue(TEXT("single arrival reaches authored compression translation"),
		FMath::IsNearlyEqual(Widget->GetRenderTransform().Translation.Y, 2.0f, 0.001f));

	Widget->ResetReceiveFeedback();
	Widget->PlayReceiveFeedback(1, false, false);
	FWacomPileCountViewTestAccess::Tick(*Widget, 0.09f);
	TestTrue(TEXT("single arrival reaches one rebound peak"),
		Widget->GetRenderTransform().Scale.Equals(FVector2D(1.08f, 1.08f), 0.001f));
	if (UTextBlock* CountText = FWacomPileCountViewTestAccess::GetCountText(*Widget))
	{
		TestTrue(TEXT("count text receives its local rebound pulse"),
			CountText->GetRenderTransform().Scale.Equals(FVector2D(1.12f, 1.12f), 0.001f));
	}
	else
	{
		AddError(TEXT("default PileCountView did not construct CountText"));
	}

	FWacomPileCountViewTestAccess::Tick(*Widget, 0.10f);
	TestTrue(TEXT("completed feedback restores the authored transform"),
		IsTransformEqual(Widget->GetRenderTransform(), AuthoredTransform));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomPileCountViewReceiveFeedbackStackSpec,
	"Wacom.UI.Common.PileCount.ReceiveFeedback.StackingAndStrength",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomPileCountViewReceiveFeedbackStackSpec::RunTest(const FString&)
{
	UPileCountView* Widget = FWacomPileCountViewTestAccess::CreateWidget();
	if (!Widget)
	{
		return false;
	}

	Widget->PlayReceiveFeedback(1, false, false);
	FWacomPileCountViewTestAccess::Tick(*Widget, 0.02f);
	Widget->PlayReceiveFeedback(1, false, false);
	FWacomPileCountViewTestAccess::Tick(*Widget, 0.02f);
	TestTrue(TEXT("overlapping arrivals add without restarting and clamp at 1.35"),
		FMath::IsNearlyEqual(Widget->GetRenderTransform().Scale.Y, 0.919f, 0.001f));

	Widget->ResetReceiveFeedback();
	Widget->PlayReceiveFeedback(4, false, false);
	FWacomPileCountViewTestAccess::Tick(*Widget, 0.04f);
	TestTrue(TEXT("same-frame batch uses square-root strength and the same cap"),
		FMath::IsNearlyEqual(Widget->GetRenderTransform().Scale.Y, 0.919f, 0.001f));

	Widget->ResetReceiveFeedback();
	Widget->PlayReceiveFeedback(1, true, false);
	FWacomPileCountViewTestAccess::Tick(*Widget, 0.04f);
	TestTrue(TEXT("final arrival applies the 1.20 strength multiplier"),
		FMath::IsNearlyEqual(Widget->GetRenderTransform().Scale.Y, 0.928f, 0.001f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomPileCountViewReceiveFeedbackCleanupSpec,
	"Wacom.UI.Common.PileCount.ReceiveFeedback.ReducedMotionAndCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomPileCountViewReceiveFeedbackCleanupSpec::RunTest(const FString&)
{
	UPileCountView* Widget = FWacomPileCountViewTestAccess::CreateWidget();
	if (!Widget)
	{
		return false;
	}

	FWidgetTransform AuthoredTransform;
	AuthoredTransform.Translation = FVector2D(4.0f, -3.0f);
	AuthoredTransform.Scale = FVector2D(0.9f, 1.1f);
	AuthoredTransform.Shear = FVector2D(0.02f, -0.03f);
	AuthoredTransform.Angle = 2.0f;
	const FVector2D AuthoredPivot(0.25f, 0.75f);
	Widget->SetRenderTransform(AuthoredTransform);
	Widget->SetRenderTransformPivot(AuthoredPivot);

	Widget->PlayReceiveFeedback(1, false, true);
	FWacomPileCountViewTestAccess::Tick(*Widget, 0.09f);
	TestTrue(TEXT("reduced motion does not change scale or translation"),
		IsTransformEqual(Widget->GetRenderTransform(), AuthoredTransform));
	TestTrue(TEXT("reduced motion does not change pivot"),
		Widget->GetRenderTransformPivot().Equals(AuthoredPivot, 0.001f));

	Widget->PlayReceiveFeedback(1, false, false);
	FWacomPileCountViewTestAccess::Tick(*Widget, 0.04f);
	Widget->ResetReceiveFeedback();
	TestTrue(TEXT("explicit reset restores a non-default authored transform"),
		IsTransformEqual(Widget->GetRenderTransform(), AuthoredTransform));
	TestTrue(TEXT("explicit reset restores a non-default authored pivot"),
		Widget->GetRenderTransformPivot().Equals(AuthoredPivot, 0.001f));

	Widget->PlayReceiveFeedback(1, false, false);
	FWacomPileCountViewTestAccess::Tick(*Widget, 0.04f);
	FWacomPileCountViewTestAccess::Destruct(*Widget);
	TestTrue(TEXT("destruct restores authored transform through the fallback target"),
		IsTransformEqual(Widget->GetRenderTransform(), AuthoredTransform));
	TestTrue(TEXT("destruct restores authored pivot through the fallback target"),
		Widget->GetRenderTransformPivot().Equals(AuthoredPivot, 0.001f));
	return true;
}

#endif
