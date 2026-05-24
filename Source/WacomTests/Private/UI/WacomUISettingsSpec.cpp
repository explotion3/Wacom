// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/GameInstance.h"
#include "UI/Foundation/WacomGameUIManagerSubsystem.h"
#include "UI/Foundation/WacomUIDeveloperSettings.h"
#include "UI/Foundation/WacomUITags.h"
#include "UI/WacomUISettingsTestProbes.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
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
		UWacomUISettingsFallbackWidgetProbe::StaticClass());

	TestEqual(TEXT("Missing settings entry falls back without crashing"),
		ResolvedClass.Get(),
		UWacomUISettingsFallbackWidgetProbe::StaticClass());

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
	Entry.WidgetClass = UWacomUISettingsConfiguredWidgetProbe::StaticClass();
	SettingsOverride.Get().WidgetClasses.Add(Entry);

	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomGameUIManagerSubsystem> UIManager(MakeUIManager(GameInstance.Get()));

	const TSubclassOf<UWacomActivatableWidget> ResolvedClass = UIManager->ResolveWidgetClass(
		WacomUITags::UI_Widget_ShopScreen.GetTag(),
		UWacomUISettingsFallbackWidgetProbe::StaticClass());

	TestEqual(TEXT("Configured widget tag resolves to settings class"),
		ResolvedClass.Get(),
		UWacomUISettingsConfiguredWidgetProbe::StaticClass());

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
		UWacomUISettingsFallbackWidgetProbe::StaticClass());

	TestEqual(TEXT("Empty settings widget class falls back"),
		ResolvedClass.Get(),
		UWacomUISettingsFallbackWidgetProbe::StaticClass());

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

	TestNotNull(TEXT("Empty settings resolves a non-null toast fallback"), ResolvedClass.Get());
	if (ResolvedClass)
	{
		TestTrue(TEXT("Toast fallback remains a UWacomAppToastWidget class"),
			ResolvedClass->IsChildOf(UWacomAppToastWidget::StaticClass()));
	}

	return true;
}
