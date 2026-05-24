// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/GameInstance.h"
#include "UI/Events/WacomRunEventScreen.h"
#include "UI/Foundation/WacomGameUIManagerSubsystem.h"
#include "UI/Foundation/WacomUIDeveloperSettings.h"
#include "UI/Foundation/WacomPrimaryGameLayout.h"
#include "UI/Foundation/WacomUITags.h"
#include "UI/WacomUITestAccess.h"
#include "UI/Shop/WacomShopScreen.h"
#include "UI/WacomUISettingsTestProbes.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
const FSoftClassPath MissingWidgetClassPath(TEXT("/Game/Wacom/UI/Missing/WBP_Missing.WBP_Missing_C"));

class FWacomScopedUISettingsOverride
{
public:
	FWacomScopedUISettingsOverride()
		: Settings(GetMutableDefault<UWacomUIDeveloperSettings>())
		, SavedPrimaryLayoutClass(Settings->PrimaryLayoutClass)
		, SavedWidgetClasses(Settings->WidgetClasses)
		, SavedAppToastWidgetClass(Settings->AppToastWidgetClass)
	{
		Settings->PrimaryLayoutClass.Reset();
		Settings->WidgetClasses.Reset();
		Settings->AppToastWidgetClass.Reset();
	}

	~FWacomScopedUISettingsOverride()
	{
		Settings->PrimaryLayoutClass = SavedPrimaryLayoutClass;
		Settings->WidgetClasses = SavedWidgetClasses;
		Settings->AppToastWidgetClass = SavedAppToastWidgetClass;
	}

	UWacomUIDeveloperSettings& Get() const
	{
		return *Settings;
	}

private:
	UWacomUIDeveloperSettings* Settings = nullptr;
	TSoftClassPtr<UWacomPrimaryGameLayout> SavedPrimaryLayoutClass;
	TArray<FWacomUIWidgetClassEntry> SavedWidgetClasses;
	TSoftClassPtr<UWacomAppToastWidget> SavedAppToastWidgetClass;
};

UWacomGameUIManagerSubsystem* MakeUIManager(UGameInstance* GameInstance)
{
	return NewObject<UWacomGameUIManagerSubsystem>(GameInstance);
}

FWacomAsyncWidgetPushResult RunAsyncPushRequest(
	UWacomUISettingsGameUIManagerProbe& UIManager,
	UWacomPrimaryGameLayout& Layout,
	FGameplayTag WidgetTag,
	TSubclassOf<UWacomActivatableWidget> FallbackClass)
{
	FWacomAsyncWidgetPushResult CapturedResult;
	FWacomUITestAccess::SetPrimaryLayout(UIManager, &Layout);

	FWacomAsyncWidgetPushRequest Request;
	Request.LayerTag = WacomUITags::UI_Layer_GameMenu.GetTag();
	Request.WidgetTag = WidgetTag;
	Request.FallbackClass = FallbackClass;
	Request.bLogMissingEntry = false;
	Request.OnComplete = [&CapturedResult](const FWacomAsyncWidgetPushResult& Result)
	{
		CapturedResult = Result;
	};
	UIManager.PushRegisteredWidgetToLayerAsync(MoveTemp(Request));
	return CapturedResult;
}

void AddMissingWidgetClassExpectedMessages(FAutomationTestBase& Test)
{
	Test.AddExpectedErrorPlain(
		TEXT("WBP_Missing"),
		EAutomationExpectedErrorFlags::Contains,
		-1);
}

FWacomUIWidgetClassEntry MakeWidgetEntry(
	const FGameplayTag& WidgetTag,
	TSubclassOf<UWacomActivatableWidget> WidgetClass)
{
	FWacomUIWidgetClassEntry Entry;
	Entry.WidgetTag = WidgetTag;
	Entry.WidgetClass = WidgetClass;
	return Entry;
}

bool TestSettingsValid(FAutomationTestBase& Test, const UWacomUIDeveloperSettings& Settings, const TCHAR* What)
{
	TArray<FText> Errors;
	const bool bIsValid = Settings.ValidateSettings(Errors);
	Test.TestTrue(What, bIsValid);
	Test.TestEqual(TEXT("Validation should not emit errors"), Errors.Num(), 0);
	return bIsValid;
}

bool TestSettingsInvalid(FAutomationTestBase& Test, const UWacomUIDeveloperSettings& Settings, const TCHAR* What)
{
	TArray<FText> Errors;
	const bool bIsValid = Settings.ValidateSettings(Errors);
	Test.TestFalse(What, bIsValid);
	Test.TestTrue(TEXT("Invalid settings should emit at least one error"), Errors.Num() > 0);
	return !bIsValid && Errors.Num() > 0;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsResolveWidgetClassEmptySettingsSpec,
	"Wacom.UI.Settings.ResolveWidgetClass.EmptySettingsFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsResolveWidgetClassEmptySettingsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomScopedUISettingsOverride SettingsOverride;
	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomGameUIManagerSubsystem> UIManager(MakeUIManager(GameInstance.Get()));

	const TSubclassOf<UWacomActivatableWidget> ResolvedClass = UIManager->ResolveWidgetClass(
		WacomUITags::UI_Widget_ShopScreen.GetTag(),
		UWacomShopScreen::StaticClass());

	TestEqual(TEXT("Missing settings entry falls back without crashing"),
		ResolvedClass.Get(),
		UWacomShopScreen::StaticClass());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsResolveWidgetClassConfiguredTagSpec,
	"Wacom.UI.Settings.ResolveWidgetClass.ConfiguredTag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsResolveWidgetClassConfiguredTagSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomScopedUISettingsOverride SettingsOverride;
	FWacomUIWidgetClassEntry Entry;
	Entry.WidgetTag = WacomUITags::UI_Widget_ShopScreen.GetTag();
	Entry.WidgetClass = UWacomUISettingsShopScreenProbe::StaticClass();
	SettingsOverride.Get().WidgetClasses.Add(Entry);

	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomGameUIManagerSubsystem> UIManager(MakeUIManager(GameInstance.Get()));

	const TSubclassOf<UWacomActivatableWidget> ResolvedClass = UIManager->ResolveWidgetClass(
		WacomUITags::UI_Widget_ShopScreen.GetTag(),
		UWacomShopScreen::StaticClass());

	TestEqual(TEXT("Configured widget tag resolves to settings class"),
		ResolvedClass.Get(),
		UWacomUISettingsShopScreenProbe::StaticClass());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsResolveWidgetClassInvalidTagFallbackSpec,
	"Wacom.UI.Settings.ResolveWidgetClass.InvalidTagFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsResolveWidgetClassInvalidTagFallbackSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomScopedUISettingsOverride SettingsOverride;
	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomGameUIManagerSubsystem> UIManager(MakeUIManager(GameInstance.Get()));

	const TSubclassOf<UWacomActivatableWidget> ResolvedClass = UIManager->ResolveWidgetClass(
		FGameplayTag(),
		UWacomUISettingsFallbackWidgetProbe::StaticClass());

	TestEqual(TEXT("Invalid widget tag falls back"),
		ResolvedClass.Get(),
		UWacomUISettingsFallbackWidgetProbe::StaticClass());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsResolveWidgetClassEmptyWidgetClassFallbackSpec,
	"Wacom.UI.Settings.ResolveWidgetClass.EmptyWidgetClassFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsResolveWidgetClassEmptyWidgetClassFallbackSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomScopedUISettingsOverride SettingsOverride;
	FWacomUIWidgetClassEntry Entry;
	Entry.WidgetTag = WacomUITags::UI_Widget_RunEventScreen.GetTag();
	SettingsOverride.Get().WidgetClasses.Add(Entry);

	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomGameUIManagerSubsystem> UIManager(MakeUIManager(GameInstance.Get()));

	const TSubclassOf<UWacomActivatableWidget> ResolvedClass = UIManager->ResolveWidgetClass(
		WacomUITags::UI_Widget_RunEventScreen.GetTag(),
		UWacomRunEventScreen::StaticClass());

	TestEqual(TEXT("Empty settings widget class falls back"),
		ResolvedClass.Get(),
		UWacomRunEventScreen::StaticClass());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsResolveToastWidgetClassEmptySettingsSpec,
	"Wacom.UI.Settings.ResolveToastWidgetClass.EmptySettingsFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsResolveToastWidgetClassEmptySettingsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomScopedUISettingsOverride SettingsOverride;
	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomGameUIManagerSubsystem> UIManager(MakeUIManager(GameInstance.Get()));

	const TSubclassOf<UWacomAppToastWidget> ResolvedClass = UIManager->ResolveToastWidgetClass();

	TestEqual(TEXT("Empty settings resolves the C++ toast fallback"),
		ResolvedClass.Get(),
		UWacomAppToastWidget::StaticClass());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsAsyncPushConfiguredLoadedSoftClassSpec,
	"Wacom.UI.Settings.AsyncPush.ConfiguredLoadedSoftClass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsAsyncPushConfiguredLoadedSoftClassSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomScopedUISettingsOverride SettingsOverride;
	SettingsOverride.Get().WidgetClasses.Add(MakeWidgetEntry(
		WacomUITags::UI_Widget_BackpackScreen.GetTag(),
		UWacomUISettingsBackpackScreenProbe::StaticClass()));

	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomUISettingsGameUIManagerProbe> UIManager(NewObject<UWacomUISettingsGameUIManagerProbe>(GameInstance.Get()));
	TStrongObjectPtr<UWacomUISettingsPrimaryLayoutProbe> Layout(NewObject<UWacomUISettingsPrimaryLayoutProbe>(GameInstance.Get()));

	const FWacomAsyncWidgetPushResult Result = RunAsyncPushRequest(
		*UIManager,
		*Layout,
		WacomUITags::UI_Widget_BackpackScreen.GetTag(),
		UWacomBackpackScreen::StaticClass());

	TestTrue(TEXT("Async push succeeds with configured loaded class"), Result.bSucceeded);
	TestEqual(TEXT("Async push resolves settings class"),
		Result.ResolvedClass.Get(),
		UWacomUISettingsBackpackScreenProbe::StaticClass());
	TestNotNull(TEXT("Async push returns pushed widget"), Result.PushedWidget);
	if (Result.PushedWidget)
	{
		TestTrue(TEXT("Pushed widget is settings probe"),
			Result.PushedWidget->IsA(UWacomUISettingsBackpackScreenProbe::StaticClass()));
	}
	TestFalse(TEXT("Layer no longer has pending async push"),
		UIManager->HasPendingAsyncPushToLayer(WacomUITags::UI_Layer_GameMenu.GetTag()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsAsyncPushMissingEntryFallbackSpec,
	"Wacom.UI.Settings.AsyncPush.MissingEntryFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsAsyncPushMissingEntryFallbackSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomScopedUISettingsOverride SettingsOverride;
	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomUISettingsGameUIManagerProbe> UIManager(NewObject<UWacomUISettingsGameUIManagerProbe>(GameInstance.Get()));
	TStrongObjectPtr<UWacomUISettingsPrimaryLayoutProbe> Layout(NewObject<UWacomUISettingsPrimaryLayoutProbe>(GameInstance.Get()));

	const FWacomAsyncWidgetPushResult Result = RunAsyncPushRequest(
		*UIManager,
		*Layout,
		WacomUITags::UI_Widget_PauseMenuScreen.GetTag(),
		UWacomUISettingsPauseMenuScreenProbe::StaticClass());

	TestTrue(TEXT("Async push succeeds via fallback when settings entry is missing"), Result.bSucceeded);
	TestEqual(TEXT("Missing entry resolves fallback class"),
		Result.ResolvedClass.Get(),
		UWacomUISettingsPauseMenuScreenProbe::StaticClass());
	TestNotNull(TEXT("Fallback push returns widget"), Result.PushedWidget);
	if (Result.PushedWidget)
	{
		TestTrue(TEXT("Pushed widget is fallback class"),
			Result.PushedWidget->IsA(UWacomUISettingsPauseMenuScreenProbe::StaticClass()));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsAsyncPushWrongParentFallbackSpec,
	"Wacom.UI.Settings.AsyncPush.WrongParentFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsAsyncPushWrongParentFallbackSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomScopedUISettingsOverride SettingsOverride;
	FWacomUIWidgetClassEntry Entry;
	Entry.WidgetTag = WacomUITags::UI_Widget_BackpackScreen.GetTag();
	Entry.WidgetClass = UWacomUISettingsConfiguredWidgetProbe::StaticClass();
	SettingsOverride.Get().WidgetClasses.Add(Entry);

	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomUISettingsGameUIManagerProbe> UIManager(NewObject<UWacomUISettingsGameUIManagerProbe>(GameInstance.Get()));
	TStrongObjectPtr<UWacomUISettingsPrimaryLayoutProbe> Layout(NewObject<UWacomUISettingsPrimaryLayoutProbe>(GameInstance.Get()));

	const FWacomAsyncWidgetPushResult Result = RunAsyncPushRequest(
		*UIManager,
		*Layout,
		WacomUITags::UI_Widget_BackpackScreen.GetTag(),
		UWacomUISettingsBackpackScreenProbe::StaticClass());

	TestTrue(TEXT("Wrong parent settings class falls back and still pushes"), Result.bSucceeded);
	TestEqual(TEXT("Wrong parent resolves fallback class"),
		Result.ResolvedClass.Get(),
		UWacomUISettingsBackpackScreenProbe::StaticClass());
	TestNotNull(TEXT("Wrong parent fallback returns widget"), Result.PushedWidget);
	if (Result.PushedWidget)
	{
		TestTrue(TEXT("Pushed widget is fallback after wrong parent"),
			Result.PushedWidget->IsA(UWacomUISettingsBackpackScreenProbe::StaticClass()));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsAsyncPushBadPathFallbackSpec,
	"Wacom.UI.Settings.AsyncPush.BadPathFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsAsyncPushBadPathFallbackSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomScopedUISettingsOverride SettingsOverride;
	FWacomUIWidgetClassEntry Entry;
	Entry.WidgetTag = WacomUITags::UI_Widget_BackpackScreen.GetTag();
	Entry.WidgetClass = MissingWidgetClassPath;
	SettingsOverride.Get().WidgetClasses.Add(Entry);
	AddMissingWidgetClassExpectedMessages(*this);

	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomUISettingsGameUIManagerProbe> UIManager(NewObject<UWacomUISettingsGameUIManagerProbe>(GameInstance.Get()));
	TStrongObjectPtr<UWacomUISettingsPrimaryLayoutProbe> Layout(NewObject<UWacomUISettingsPrimaryLayoutProbe>(GameInstance.Get()));

	FWacomAsyncWidgetPushResult CapturedResult;
	FWacomUITestAccess::SetPrimaryLayout(*UIManager, Layout.Get());

	FWacomAsyncWidgetPushRequest Request;
	Request.LayerTag = WacomUITags::UI_Layer_GameMenu.GetTag();
	Request.WidgetTag = WacomUITags::UI_Widget_BackpackScreen.GetTag();
	Request.FallbackClass = UWacomUISettingsBackpackScreenProbe::StaticClass();
	Request.bLogMissingEntry = false;
	Request.OnComplete = [&CapturedResult](const FWacomAsyncWidgetPushResult& Result)
	{
		CapturedResult = Result;
	};

	UIManager->PushRegisteredWidgetToLayerAsync(MoveTemp(Request));
	TestTrue(TEXT("Bad path creates a pending async push before callback"),
		UIManager->HasPendingAsyncPushToLayer(WacomUITags::UI_Layer_GameMenu.GetTag()));
	FWacomUITestAccess::CancelAllPendingAsyncPushes(*UIManager);
	TestFalse(TEXT("Cancel clears pending bad path async push"),
		UIManager->HasPendingAsyncPushToLayer(WacomUITags::UI_Layer_GameMenu.GetTag()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsAsyncPushLayerPendingSpec,
	"Wacom.UI.Settings.AsyncPush.LayerPending",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsAsyncPushLayerPendingSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomScopedUISettingsOverride SettingsOverride;
	FWacomUIWidgetClassEntry Entry;
	Entry.WidgetTag = WacomUITags::UI_Widget_BackpackScreen.GetTag();
	Entry.WidgetClass = MissingWidgetClassPath;
	SettingsOverride.Get().WidgetClasses.Add(Entry);

	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomUISettingsGameUIManagerProbe> UIManager(NewObject<UWacomUISettingsGameUIManagerProbe>(GameInstance.Get()));
	TStrongObjectPtr<UWacomUISettingsPrimaryLayoutProbe> Layout(NewObject<UWacomUISettingsPrimaryLayoutProbe>(GameInstance.Get()));
	FWacomUITestAccess::SetPrimaryLayout(*UIManager, Layout.Get());

	FWacomAsyncWidgetPushRequest FirstRequest;
	FirstRequest.LayerTag = WacomUITags::UI_Layer_GameMenu.GetTag();
	FirstRequest.WidgetTag = WacomUITags::UI_Widget_BackpackScreen.GetTag();
	FirstRequest.FallbackClass = UWacomUISettingsBackpackScreenProbe::StaticClass();
	FirstRequest.bLogMissingEntry = false;
	UIManager->PushRegisteredWidgetToLayerAsync(MoveTemp(FirstRequest));
	TestTrue(TEXT("First bad-path async request remains pending"),
		UIManager->HasPendingAsyncPushToLayer(WacomUITags::UI_Layer_GameMenu.GetTag()));

	FWacomAsyncWidgetPushResult CapturedResult;
	FWacomAsyncWidgetPushRequest SecondRequest;
	SecondRequest.LayerTag = WacomUITags::UI_Layer_GameMenu.GetTag();
	SecondRequest.WidgetTag = WacomUITags::UI_Widget_PauseMenuScreen.GetTag();
	SecondRequest.FallbackClass = UWacomUISettingsPauseMenuScreenProbe::StaticClass();
	SecondRequest.bLogMissingEntry = false;
	SecondRequest.OnComplete = [&CapturedResult](const FWacomAsyncWidgetPushResult& Result)
	{
		CapturedResult = Result;
	};

	UIManager->PushRegisteredWidgetToLayerAsync(MoveTemp(SecondRequest));
	TestFalse(TEXT("Second same-layer request fails while first is pending"), CapturedResult.bSucceeded);
	TestEqual(TEXT("Second same-layer request reports LayerPending"),
		CapturedResult.FailureReason,
		FName(TEXT("LayerPending")));

	FWacomUITestAccess::CancelAllPendingAsyncPushes(*UIManager);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsAsyncPushPrePushGuardRejectedSpec,
	"Wacom.UI.Settings.AsyncPush.PrePushGuardRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsAsyncPushPrePushGuardRejectedSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomScopedUISettingsOverride SettingsOverride;
	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomUISettingsGameUIManagerProbe> UIManager(NewObject<UWacomUISettingsGameUIManagerProbe>(GameInstance.Get()));
	TStrongObjectPtr<UWacomUISettingsPrimaryLayoutProbe> Layout(NewObject<UWacomUISettingsPrimaryLayoutProbe>(GameInstance.Get()));
	FWacomUITestAccess::SetPrimaryLayout(*UIManager, Layout.Get());

	FWacomAsyncWidgetPushResult CapturedResult;
	FWacomAsyncWidgetPushRequest Request;
	Request.LayerTag = WacomUITags::UI_Layer_GameMenu.GetTag();
	Request.WidgetTag = WacomUITags::UI_Widget_BackpackScreen.GetTag();
	Request.FallbackClass = UWacomUISettingsBackpackScreenProbe::StaticClass();
	Request.bLogMissingEntry = false;
	Request.CanPush = []()
	{
		return false;
	};
	Request.OnComplete = [&CapturedResult](const FWacomAsyncWidgetPushResult& Result)
	{
		CapturedResult = Result;
	};

	UIManager->PushRegisteredWidgetToLayerAsync(MoveTemp(Request));
	TestFalse(TEXT("Rejected guard does not push"), CapturedResult.bSucceeded);
	TestEqual(TEXT("Rejected guard reports stable failure reason"),
		CapturedResult.FailureReason,
		FName(TEXT("PrePushGuardRejected")));
	TestNull(TEXT("Rejected guard does not create widget"), UIManager->LastPushedWidget);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsAsyncPushStalePrimaryLayoutSpec,
	"Wacom.UI.Settings.AsyncPush.StalePrimaryLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsAsyncPushStalePrimaryLayoutSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomScopedUISettingsOverride SettingsOverride;
	FWacomUIWidgetClassEntry Entry;
	Entry.WidgetTag = WacomUITags::UI_Widget_BackpackScreen.GetTag();
	Entry.WidgetClass = MissingWidgetClassPath;
	SettingsOverride.Get().WidgetClasses.Add(Entry);

	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomUISettingsGameUIManagerProbe> UIManager(NewObject<UWacomUISettingsGameUIManagerProbe>(GameInstance.Get()));
	TStrongObjectPtr<UWacomUISettingsPrimaryLayoutProbe> OriginalLayout(NewObject<UWacomUISettingsPrimaryLayoutProbe>(GameInstance.Get()));
	TStrongObjectPtr<UWacomUISettingsPrimaryLayoutProbe> ReplacementLayout(NewObject<UWacomUISettingsPrimaryLayoutProbe>(GameInstance.Get()));
	FWacomUITestAccess::SetPrimaryLayout(*UIManager, OriginalLayout.Get());

	FWacomAsyncWidgetPushResult CapturedResult;
	FWacomAsyncWidgetPushRequest Request;
	Request.LayerTag = WacomUITags::UI_Layer_GameMenu.GetTag();
	Request.WidgetTag = WacomUITags::UI_Widget_BackpackScreen.GetTag();
	Request.FallbackClass = UWacomUISettingsBackpackScreenProbe::StaticClass();
	Request.bLogMissingEntry = false;
	Request.OnComplete = [&CapturedResult](const FWacomAsyncWidgetPushResult& Result)
	{
		CapturedResult = Result;
	};

	UIManager->PushRegisteredWidgetToLayerAsync(MoveTemp(Request));
	TestTrue(TEXT("Bad path request is pending before stale layout simulation"),
		UIManager->HasPendingAsyncPushToLayer(WacomUITags::UI_Layer_GameMenu.GetTag()));
	FWacomUITestAccess::SetPrimaryLayout(*UIManager, ReplacementLayout.Get());

	FWacomUITestAccess::CompleteAsyncWidgetPush(
		*UIManager,
		WacomUITags::UI_Layer_GameMenu.GetTag(),
		UWacomUISettingsBackpackScreenProbe::StaticClass());
	TestFalse(TEXT("Stale PrimaryLayout prevents push"), CapturedResult.bSucceeded);
	TestEqual(TEXT("Stale PrimaryLayout reports stable failure reason"),
		CapturedResult.FailureReason,
		FName(TEXT("StalePrimaryLayout")));
	TestNull(TEXT("Stale PrimaryLayout does not create widget"), UIManager->LastPushedWidget);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsAsyncPushStaleOwningPlayerSpec,
	"Wacom.UI.Settings.AsyncPush.StaleOwningPlayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsAsyncPushStaleOwningPlayerSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomScopedUISettingsOverride SettingsOverride;
	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomUISettingsGameUIManagerProbe> UIManager(NewObject<UWacomUISettingsGameUIManagerProbe>(GameInstance.Get()));
	TStrongObjectPtr<UWacomUISettingsPrimaryLayoutProbe> Layout(NewObject<UWacomUISettingsPrimaryLayoutProbe>(GameInstance.Get()));
	TStrongObjectPtr<APlayerController> OtherPlayer(NewObject<APlayerController>(GameInstance.Get()));
	FWacomUITestAccess::SetPrimaryLayout(*UIManager, Layout.Get());

	FWacomAsyncWidgetPushResult CapturedResult;
	FWacomAsyncWidgetPushRequest Request;
	Request.LayerTag = WacomUITags::UI_Layer_GameMenu.GetTag();
	Request.WidgetTag = WacomUITags::UI_Widget_BackpackScreen.GetTag();
	Request.FallbackClass = UWacomUISettingsBackpackScreenProbe::StaticClass();
	Request.OwningPlayer = OtherPlayer.Get();
	Request.bLogMissingEntry = false;
	Request.OnComplete = [&CapturedResult](const FWacomAsyncWidgetPushResult& Result)
	{
		CapturedResult = Result;
	};

	UIManager->PushRegisteredWidgetToLayerAsync(MoveTemp(Request));
	TestFalse(TEXT("Mismatched owning player prevents push"), CapturedResult.bSucceeded);
	TestEqual(TEXT("Mismatched owning player reports stable failure reason"),
		CapturedResult.FailureReason,
		FName(TEXT("StaleOwningPlayer")));
	TestNull(TEXT("Mismatched owning player does not create widget"), UIManager->LastPushedWidget);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsAsyncPushMissingPrimaryLayoutFailsSpec,
	"Wacom.UI.Settings.AsyncPush.MissingPrimaryLayoutFails",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsAsyncPushMissingPrimaryLayoutFailsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomScopedUISettingsOverride SettingsOverride;
	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomGameUIManagerSubsystem> UIManager(MakeUIManager(GameInstance.Get()));

	FWacomAsyncWidgetPushResult CapturedResult;
	FWacomAsyncWidgetPushRequest Request;
	Request.LayerTag = WacomUITags::UI_Layer_GameMenu.GetTag();
	Request.WidgetTag = WacomUITags::UI_Widget_BackpackScreen.GetTag();
	Request.FallbackClass = UWacomUISettingsBackpackScreenProbe::StaticClass();
	Request.bLogMissingEntry = false;
	Request.OnComplete = [&CapturedResult](const FWacomAsyncWidgetPushResult& Result)
	{
		CapturedResult = Result;
	};

	UIManager->PushRegisteredWidgetToLayerAsync(MoveTemp(Request));

	TestFalse(TEXT("Async push fails when PrimaryLayout is missing"), CapturedResult.bSucceeded);
	TestEqual(TEXT("Missing PrimaryLayout reports a stable failure reason"),
		CapturedResult.FailureReason,
		FName(TEXT("MissingPrimaryLayout")));
	TestFalse(TEXT("Missing PrimaryLayout does not leave a pending push"),
		UIManager->HasPendingAsyncPushToLayer(WacomUITags::UI_Layer_GameMenu.GetTag()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsValidationValidDefaultsSpec,
	"Wacom.UI.Settings.Validation.ValidDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsValidationValidDefaultsSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomScopedUISettingsOverride SettingsOverride;

	SettingsOverride.Get().WidgetClasses.Add(MakeWidgetEntry(
		WacomUITags::UI_Widget_BackpackScreen.GetTag(),
		UWacomUISettingsBackpackScreenProbe::StaticClass()));
	SettingsOverride.Get().WidgetClasses.Add(MakeWidgetEntry(
		WacomUITags::UI_Widget_ShopScreen.GetTag(),
		UWacomUISettingsShopScreenProbe::StaticClass()));
	SettingsOverride.Get().WidgetClasses.Add(MakeWidgetEntry(
		WacomUITags::UI_Widget_RunEventScreen.GetTag(),
		UWacomUISettingsRunEventScreenProbe::StaticClass()));
	SettingsOverride.Get().WidgetClasses.Add(MakeWidgetEntry(
		WacomUITags::UI_Widget_PauseMenuScreen.GetTag(),
		UWacomUISettingsPauseMenuScreenProbe::StaticClass()));

	return TestSettingsValid(
		*this,
		SettingsOverride.Get(),
		TEXT("Scoped valid settings pass ValidateSettings"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsValidationConfiguredPrimaryLayoutInvalidPathSpec,
	"Wacom.UI.Settings.Validation.ConfiguredPrimaryLayoutInvalidPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsValidationConfiguredPrimaryLayoutInvalidPathSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomScopedUISettingsOverride SettingsOverride;
	SettingsOverride.Get().PrimaryLayoutClass = MissingWidgetClassPath;
	AddMissingWidgetClassExpectedMessages(*this);

	TestSettingsInvalid(
		*this,
		SettingsOverride.Get(),
		TEXT("Bad PrimaryLayoutClass path fails ValidateSettings"));

	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomGameUIManagerSubsystem> UIManager(MakeUIManager(GameInstance.Get()));

	const TSubclassOf<UWacomPrimaryGameLayout> ResolvedClass = UIManager->ResolvePrimaryLayoutClass();
	TestNotNull(TEXT("Bad PrimaryLayoutClass falls back to a non-null class"), ResolvedClass.Get());
	if (ResolvedClass)
	{
		TestTrue(TEXT("Primary layout fallback remains a UWacomPrimaryGameLayout class"),
			ResolvedClass->IsChildOf(UWacomPrimaryGameLayout::StaticClass()));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsValidationConfiguredToastInvalidPathSpec,
	"Wacom.UI.Settings.Validation.ConfiguredToastInvalidPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsValidationConfiguredToastInvalidPathSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomScopedUISettingsOverride SettingsOverride;
	SettingsOverride.Get().AppToastWidgetClass = MissingWidgetClassPath;
	AddMissingWidgetClassExpectedMessages(*this);

	TestSettingsInvalid(
		*this,
		SettingsOverride.Get(),
		TEXT("Bad AppToastWidgetClass path fails ValidateSettings"));

	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomGameUIManagerSubsystem> UIManager(MakeUIManager(GameInstance.Get()));

	const TSubclassOf<UWacomAppToastWidget> ResolvedClass = UIManager->ResolveToastWidgetClass();
	TestEqual(TEXT("Bad AppToastWidgetClass falls back to the C++ toast class"),
		ResolvedClass.Get(),
		UWacomAppToastWidget::StaticClass());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsValidationWidgetEntryMissingTagOrClassSpec,
	"Wacom.UI.Settings.Validation.WidgetEntryMissingTagOrClass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsValidationWidgetEntryMissingTagOrClassSpec::RunTest(const FString& /*Parameters*/)
{
	{
		FWacomScopedUISettingsOverride SettingsOverride;
		SettingsOverride.Get().WidgetClasses.Add(MakeWidgetEntry(
			FGameplayTag(),
			UWacomUISettingsConfiguredWidgetProbe::StaticClass()));

		TestSettingsInvalid(
			*this,
			SettingsOverride.Get(),
			TEXT("Widget entry with an empty tag fails ValidateSettings"));
	}

	{
		FWacomScopedUISettingsOverride SettingsOverride;
		FWacomUIWidgetClassEntry Entry;
		Entry.WidgetTag = WacomUITags::UI_Widget_ShopScreen.GetTag();
		SettingsOverride.Get().WidgetClasses.Add(Entry);

		TestSettingsInvalid(
			*this,
			SettingsOverride.Get(),
			TEXT("Widget entry with an empty class fails ValidateSettings"));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsValidationWidgetEntryDuplicateTagSpec,
	"Wacom.UI.Settings.Validation.WidgetEntryDuplicateTag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsValidationWidgetEntryDuplicateTagSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomScopedUISettingsOverride SettingsOverride;
	SettingsOverride.Get().WidgetClasses.Add(MakeWidgetEntry(
		WacomUITags::UI_Widget_ShopScreen.GetTag(),
		UWacomUISettingsShopScreenProbe::StaticClass()));
	SettingsOverride.Get().WidgetClasses.Add(MakeWidgetEntry(
		WacomUITags::UI_Widget_ShopScreen.GetTag(),
		UWacomUISettingsShopScreenProbe::StaticClass()));

	return TestSettingsInvalid(
		*this,
		SettingsOverride.Get(),
		TEXT("Duplicate widget tags fail ValidateSettings"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsValidationWidgetEntryOutsideUIWidgetNamespaceSpec,
	"Wacom.UI.Settings.Validation.WidgetEntryOutsideUIWidgetNamespace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsValidationWidgetEntryOutsideUIWidgetNamespaceSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomScopedUISettingsOverride SettingsOverride;
	SettingsOverride.Get().WidgetClasses.Add(MakeWidgetEntry(
		WacomUITags::UI_Layer_Game.GetTag(),
		UWacomUISettingsConfiguredWidgetProbe::StaticClass()));

	return TestSettingsInvalid(
		*this,
		SettingsOverride.Get(),
		TEXT("Widget tags outside UI.Widget.* fail ValidateSettings"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsValidationWrongParentClassesSpec,
	"Wacom.UI.Settings.Validation.WrongParentClasses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsValidationWrongParentClassesSpec::RunTest(const FString& /*Parameters*/)
{
	{
		FWacomScopedUISettingsOverride SettingsOverride;
		FWacomUIWidgetClassEntry Entry;
		Entry.WidgetTag = WacomUITags::UI_Widget_ShopScreen.GetTag();
		Entry.WidgetClass = UWacomUISettingsWrongParentWidgetProbe::StaticClass();
		SettingsOverride.Get().WidgetClasses.Add(Entry);

		TestSettingsInvalid(
			*this,
			SettingsOverride.Get(),
			TEXT("WidgetClass that does not inherit UWacomActivatableWidget fails ValidateSettings"));
	}

	{
		FWacomScopedUISettingsOverride SettingsOverride;
		SettingsOverride.Get().PrimaryLayoutClass = UWacomUISettingsWrongParentWidgetProbe::StaticClass();

		TestSettingsInvalid(
			*this,
			SettingsOverride.Get(),
			TEXT("PrimaryLayoutClass that does not inherit UWacomPrimaryGameLayout fails ValidateSettings"));
	}

	{
		FWacomScopedUISettingsOverride SettingsOverride;
		SettingsOverride.Get().AppToastWidgetClass = UWacomUISettingsWrongParentWidgetProbe::StaticClass();

		TestSettingsInvalid(
			*this,
			SettingsOverride.Get(),
			TEXT("AppToastWidgetClass that does not inherit UWacomAppToastWidget fails ValidateSettings"));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsValidationWidgetEntryWrongScreenParentFallbackSpec,
	"Wacom.UI.Settings.Validation.WidgetEntryWrongScreenParentFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsValidationWidgetEntryWrongScreenParentFallbackSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomScopedUISettingsOverride SettingsOverride;
	FWacomUIWidgetClassEntry Entry;
	Entry.WidgetTag = WacomUITags::UI_Widget_ShopScreen.GetTag();
	Entry.WidgetClass = UWacomUISettingsConfiguredWidgetProbe::StaticClass();
	SettingsOverride.Get().WidgetClasses.Add(Entry);

	TestSettingsInvalid(
		*this,
		SettingsOverride.Get(),
		TEXT("ShopScreen tag with generic activatable class fails ValidateSettings"));

	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomGameUIManagerSubsystem> UIManager(MakeUIManager(GameInstance.Get()));

	const TSubclassOf<UWacomActivatableWidget> ResolvedClass = UIManager->ResolveWidgetClass(
		WacomUITags::UI_Widget_ShopScreen.GetTag(),
		UWacomShopScreen::StaticClass());
	TestEqual(TEXT("ShopScreen tag with generic activatable class falls back at runtime"),
		ResolvedClass.Get(),
		UWacomShopScreen::StaticClass());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUISettingsValidationOptionalWidgetMissingIsValidSpec,
	"Wacom.UI.Settings.Validation.OptionalWidgetMissingIsValid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUISettingsValidationOptionalWidgetMissingIsValidSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomScopedUISettingsOverride SettingsOverride;
	SettingsOverride.Get().WidgetClasses.Add(MakeWidgetEntry(
		WacomUITags::UI_Widget_BackpackScreen.GetTag(),
		UWacomUISettingsBackpackScreenProbe::StaticClass()));

	return TestSettingsValid(
		*this,
		SettingsOverride.Get(),
		TEXT("Optional Shop/RunEvent/PauseMenu widget entries can be missing"));
}
