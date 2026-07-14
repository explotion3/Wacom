// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "UI/Common/PileCountView.h"
#include "UI/PileCountViewTestAccess.h"

#include "Components/TextBlock.h"

#if WITH_AUTOMATION_TESTS

namespace
{
	bool IsSendFeedbackTransformEqual(
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
	FWacomPileCountViewSendFeedbackCurveSpec,
	"Wacom.UI.Common.PileCount.SendFeedback.DirectionalCurveAndDisplayContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomPileCountViewSendFeedbackCurveSpec::RunTest(const FString&)
{
	UPileCountView* Widget = FWacomPileCountViewTestAccess::CreateWidget();
	TestNotNull(TEXT("pile count widget is created"), Widget);
	if (!Widget)
	{
		return false;
	}

	const FWidgetTransform AuthoredTransform = Widget->GetRenderTransform();
	Widget->PlaySendFeedback(1, false, FVector2D(1.0f, 0.0f), false);
	FWacomPileCountViewTestAccess::Tick(*Widget, 0.025f);
	TestTrue(TEXT("send compression follows launch direction"),
		Widget->GetRenderTransform().Translation.Equals(FVector2D(1.5f, 0.0f), 0.001f));
	TestTrue(TEXT("send compression reaches authored scale"),
		Widget->GetRenderTransform().Scale.Equals(FVector2D(1.03f, 0.95f), 0.001f));
	if (UTextBlock* CountText = FWacomPileCountViewTestAccess::GetCountText(*Widget))
	{
		TestTrue(TEXT("count text compresses with the departing card"),
			CountText->GetRenderTransform().Scale.Equals(FVector2D(0.94f, 0.94f), 0.001f));
	}

	Widget->ResetSendFeedback();
	Widget->PlaySendFeedback(1, false, FVector2D(1.0f, 0.0f), false);
	FWacomPileCountViewTestAccess::Tick(*Widget, 0.075f);
	TestTrue(TEXT("send recoil moves opposite launch direction"),
		Widget->GetRenderTransform().Translation.Equals(FVector2D(-3.0f, 0.0f), 0.001f));
	TestTrue(TEXT("send recoil reaches one rebound scale"),
		Widget->GetRenderTransform().Scale.Equals(FVector2D(1.06f, 1.06f), 0.001f));

	FWacomPileCountViewTestAccess::Tick(*Widget, 0.10f);
	TestTrue(TEXT("completed send feedback restores authored transform"),
		IsSendFeedbackTransformEqual(Widget->GetRenderTransform(), AuthoredTransform));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomPileCountViewSendFeedbackCompositionSpec,
	"Wacom.UI.Common.PileCount.SendFeedback.StackingReceiveCompositionAndCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomPileCountViewSendFeedbackCompositionSpec::RunTest(const FString&)
{
	UPileCountView* Widget = FWacomPileCountViewTestAccess::CreateWidget();
	if (!Widget)
	{
		return false;
	}

	Widget->PlaySendFeedback(1, false, FVector2D(1.0f, 0.0f), false);
	Widget->PlaySendFeedback(1, true, FVector2D(1.0f, 0.0f), false);
	FWacomPileCountViewTestAccess::Tick(*Widget, 0.025f);
	TestTrue(TEXT("same-frame departures combine once and respect the 1.30 cap"),
		FMath::IsNearlyEqual(Widget->GetRenderTransform().Scale.Y, 0.935f, 0.001f));

	Widget->ResetSendFeedback();
	Widget->PlayReceiveFeedback(1, false, false);
	FWacomPileCountViewTestAccess::Tick(*Widget, 0.04f);
	const FWidgetTransform ReceiveOnlyTransform = Widget->GetRenderTransform();
	Widget->PlaySendFeedback(1, false, FVector2D(0.0f, -1.0f), false);
	Widget->ResetSendFeedback();
	TestTrue(TEXT("resetting send preserves the active receive pulse"),
		IsSendFeedbackTransformEqual(Widget->GetRenderTransform(), ReceiveOnlyTransform));
	Widget->ResetReceiveFeedback();
	TestTrue(TEXT("resetting both channels restores authored transform"),
		IsSendFeedbackTransformEqual(Widget->GetRenderTransform(), FWidgetTransform()));

	Widget->PlaySendFeedback(1, false, FVector2D::ZeroVector, true);
	FWacomPileCountViewTestAccess::Tick(*Widget, 0.075f);
	TestTrue(TEXT("reduced motion produces no pile transform"),
		IsSendFeedbackTransformEqual(Widget->GetRenderTransform(), FWidgetTransform()));
	return true;
}

#endif
