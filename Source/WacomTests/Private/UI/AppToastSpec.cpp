// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "UI/Foundation/WacomAppToastTypes.h"
#include "UI/Foundation/WacomAppToastWidget.h"

#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIAppToastWidgetQueueSpec,
	"Wacom.UI.AppToast.WidgetQueue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIAppToastWidgetQueueSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomAppToastWidget> Widget(NewObject<UWacomAppToastWidget>());
	Widget->MaxVisibleMessages = 2;
	Widget->DefaultMessageLifetime = 3.0f;
	Widget->TakeWidget();
	TestEqual(TEXT("Toast widget starts collapsed"), Widget->GetVisibility(), ESlateVisibility::Collapsed);

	FWacomAppToastView First;
	First.MessageText = FText::FromString(TEXT("第一条"));
	First.Tone = EWacomAppToastTone::Neutral;
	First.IconKey = TEXT("First");

	FWacomAppToastView Second;
	Second.MessageText = FText::FromString(TEXT("第二条"));
	Second.Tone = EWacomAppToastTone::Positive;
	Second.IconKey = TEXT("Second");

	FWacomAppToastView Third;
	Third.MessageText = FText::FromString(TEXT("第三条"));
	Third.Tone = EWacomAppToastTone::Warning;
	Third.IconKey = TEXT("Third");
	Third.LifetimeOverride = 1.5f;

	Widget->EnqueueToast(First);
	Widget->EnqueueToast(Second);
	Widget->EnqueueToast(Third);

	TestEqual(TEXT("Toast widget becomes passive visible when messages exist"), Widget->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("Max visible messages enforced"), Widget->GetVisibleToastCount(), 2);
	const TArray<FWacomAppToastView> Current = Widget->GetCurrentToastsForTest();
	if (!TestEqual(TEXT("Current toast count"), Current.Num(), 2))
	{
		return false;
	}

	TestEqual(TEXT("Oldest toast removed"), Current[0].MessageText.ToString(), FString(TEXT("第二条")));
	TestEqual(TEXT("Newest toast preserved"), Current[1].MessageText.ToString(), FString(TEXT("第三条")));
	TestTrue(TEXT("Tone preserved"), Current[1].Tone == EWacomAppToastTone::Warning);
	TestEqual(TEXT("IconKey preserved"), Current[1].IconKey, FName(TEXT("Third")));
	TestEqual(TEXT("Lifetime override preserved"), Current[1].LifetimeOverride, 1.5f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIAppToastWidgetExpirySpec,
	"Wacom.UI.AppToast.WidgetExpiry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIAppToastWidgetExpirySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomAppToastWidget> Widget(NewObject<UWacomAppToastWidget>());
	Widget->DefaultMessageLifetime = 3.0f;
	Widget->TakeWidget();
	TestTrue(TEXT("Toast widget idle hidden before enqueue"), Widget->IsIdleHiddenForTest());

	FWacomAppToastView ShortToast;
	ShortToast.MessageText = FText::FromString(TEXT("短提示"));
	ShortToast.LifetimeOverride = 0.25f;

	Widget->EnqueueToast(ShortToast);
	TestEqual(TEXT("Toast starts visible"), Widget->GetVisibleToastCount(), 1);
	TestEqual(TEXT("Toast widget is hit-test-invisible while showing"), Widget->GetVisibility(), ESlateVisibility::HitTestInvisible);

	Widget->TickToastsForTest(0.3f);
	TestEqual(TEXT("Toast expires after lifetime"), Widget->GetVisibleToastCount(), 0);
	TestTrue(TEXT("Toast widget hides instead of deactivating when queue becomes empty"), Widget->IsIdleHiddenForTest());
	TestFalse(TEXT("Toast widget never needs CommonUI activation for passive viewport display"), Widget->IsActivated());

	return true;
}
