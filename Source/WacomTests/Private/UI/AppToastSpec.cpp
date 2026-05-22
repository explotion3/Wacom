// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UI/Foundation/WacomAppToastTypes.h"
#include "UI/Foundation/WacomAppToastSubsystem.h"
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
	FWacomUIAppToastLifecycleOwnerPolicySpec,
	"Wacom.UI.AppToast.Lifecycle.OwnerPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIAppToastLifecycleOwnerPolicySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomAppToastSubsystem> ToastSubsystem(NewObject<UWacomAppToastSubsystem>(GameInstance.Get()));
	TStrongObjectPtr<UWorld> OldWorld(NewObject<UWorld>(GetTransientPackage(), TEXT("AppToastOldWorld")));
	TStrongObjectPtr<UWorld> CurrentWorld(NewObject<UWorld>(GetTransientPackage(), TEXT("AppToastCurrentWorld")));
	TStrongObjectPtr<APlayerController> OldPC(NewObject<APlayerController>());
	TStrongObjectPtr<APlayerController> CurrentPC(NewObject<APlayerController>());

	TestTrue(TEXT("Matching owner pair is reusable"),
		ToastSubsystem->IsToastOwnerPairUsableForTest(CurrentWorld.Get(), CurrentPC.Get(), CurrentWorld.Get(), CurrentPC.Get()));
	TestFalse(TEXT("Old world owner pair is stale"),
		ToastSubsystem->IsToastOwnerPairUsableForTest(OldWorld.Get(), CurrentPC.Get(), CurrentWorld.Get(), CurrentPC.Get()));
	TestFalse(TEXT("Old player controller owner pair is stale"),
		ToastSubsystem->IsToastOwnerPairUsableForTest(CurrentWorld.Get(), OldPC.Get(), CurrentWorld.Get(), CurrentPC.Get()));
	TestFalse(TEXT("Runtime-owned widget is stale when current PC is missing"),
		ToastSubsystem->IsToastOwnerPairUsableForTest(CurrentWorld.Get(), CurrentPC.Get(), CurrentWorld.Get(), nullptr));
	TestTrue(TEXT("Transient override without runtime owner is reusable"),
		ToastSubsystem->IsToastOwnerPairUsableForTest(nullptr, nullptr, nullptr, nullptr));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIAppToastLifecycleStaleWidgetClearedSpec,
	"Wacom.UI.AppToast.Lifecycle.StaleWidgetCleared",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIAppToastLifecycleStaleWidgetClearedSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomAppToastSubsystem> ToastSubsystem(NewObject<UWacomAppToastSubsystem>(GameInstance.Get()));
	TStrongObjectPtr<UWorld> OldWorld(NewObject<UWorld>(GetTransientPackage(), TEXT("AppToastOldWorld")));
	TStrongObjectPtr<UWacomAppToastWidget> StaleWidget(NewObject<UWacomAppToastWidget>(OldWorld.Get()));
	ToastSubsystem->SetToastWidgetOverrideForTest(StaleWidget.Get());

	UWacomAppToastWidget* ReadyWidget = ToastSubsystem->EnsureAppToastReady();
	TestNull(TEXT("Stale widget is not reused when current local PC is unavailable"), ReadyWidget);
	TestNull(TEXT("Stale widget cache is cleared"), ToastSubsystem->GetToastWidgetForTest());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIAppToastLifecycleTransientOverrideSpec,
	"Wacom.UI.AppToast.Lifecycle.TransientOverride",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIAppToastLifecycleTransientOverrideSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomAppToastSubsystem> ToastSubsystem(NewObject<UWacomAppToastSubsystem>(GameInstance.Get()));
	TStrongObjectPtr<UWacomAppToastWidget> ToastWidget(NewObject<UWacomAppToastWidget>());
	ToastWidget->TakeWidget();
	ToastSubsystem->SetToastWidgetOverrideForTest(ToastWidget.Get());

	UWacomAppToastWidget* ReadyWidget = ToastSubsystem->EnsureAppToastReady();
	TestEqual(TEXT("Transient override is reused without real world or PC"), ReadyWidget, ToastWidget.Get());

	FWacomAppToastView View;
	View.MessageText = FText::FromString(TEXT("测试复用"));
	ToastSubsystem->ShowToast(View);
	TestEqual(TEXT("Reused transient override receives toast"), ToastWidget->GetVisibleToastCount(), 1);

	ToastSubsystem->Deinitialize();
	TestNull(TEXT("Deinitialize clears cached toast widget"), ToastSubsystem->GetToastWidgetForTest());

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
