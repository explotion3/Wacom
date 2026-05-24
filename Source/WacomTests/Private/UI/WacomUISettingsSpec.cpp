// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/GameInstance.h"
#include "UI/Events/WacomRunEventScreen.h"
#include "UI/Foundation/WacomGameUIManagerSubsystem.h"
#include "UI/Foundation/WacomUIDeveloperSettings.h"
#include "UI/Foundation/WacomPrimaryGameLayout.h"
#include "UI/Foundation/WacomUITags.h"
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
