// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../BackpackScreenTestAccess.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/WrapBox.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomSpecialZoneWidget.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
UClass* LoadWidgetClass(const TCHAR* ObjectPath)
{
	return LoadObject<UClass>(nullptr, ObjectPath);
}

UWidgetTree* GetWidgetTree(UClass* WidgetClass)
{
	const UWidgetBlueprintGeneratedClass* GeneratedClass = Cast<UWidgetBlueprintGeneratedClass>(WidgetClass);
	return GeneratedClass ? GeneratedClass->GetWidgetTreeArchetype() : nullptr;
}

UObject* ReadObjectDefault(UClass* WidgetClass, FName PropertyName)
{
	UObject* CDO = WidgetClass ? WidgetClass->GetDefaultObject() : nullptr;
	const FObjectPropertyBase* Property = CDO
		? FindFProperty<FObjectPropertyBase>(CDO->GetClass(), PropertyName)
		: nullptr;
	return Property ? Property->GetObjectPropertyValue_InContainer(CDO) : nullptr;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackSpecialZoneCompatibilityAssetBindingSpec,
	"Wacom.UI.Backpack.SpecialZone.CompatibilityAssetBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackSpecialZoneCompatibilityAssetBindingSpec::RunTest(const FString& Parameters)
{
	UClass* ScreenClass = LoadWidgetClass(
		TEXT("/Game/Wacom/UI/Backpack/WBP_BackpackScreen.WBP_BackpackScreen_C"));
	UClass* SpecialZoneClass = LoadWidgetClass(
		TEXT("/Game/Wacom/UI/Backpack/WBP_WacomSpecialZoneWidget.WBP_WacomSpecialZoneWidget_C"));

	TestTrue(TEXT("Formal Backpack screen loads"),
		ScreenClass && ScreenClass->IsChildOf(UWacomBackpackScreen::StaticClass()));
	TestTrue(TEXT("Compatibility SpecialZone asset uses the passive SpecialZone parent"),
		SpecialZoneClass && SpecialZoneClass->IsChildOf(UWacomSpecialZoneWidget::StaticClass()));

	UWidgetTree* SpecialZoneTree = GetWidgetTree(SpecialZoneClass);
	TestNotNull(TEXT("Compatibility SpecialZone asset has a compiled widget tree"), SpecialZoneTree);
	if (SpecialZoneTree)
	{
		TestNotNull(TEXT("SpecialZone binds TitleText"),
			Cast<UTextBlock>(SpecialZoneTree->FindWidget(TEXT("TitleText"))));
		TestNotNull(TEXT("SpecialZone binds BattleReadyBadge"),
			Cast<UTextBlock>(SpecialZoneTree->FindWidget(TEXT("BattleReadyBadge"))));
		TestNotNull(TEXT("SpecialZone binds OwnerCardHost"),
			Cast<UPanelWidget>(SpecialZoneTree->FindWidget(TEXT("OwnerCardHost"))));
		TestNotNull(TEXT("SpecialZone binds the read-only ContentDropTargetHost"),
			Cast<UPanelWidget>(SpecialZoneTree->FindWidget(TEXT("ContentDropTargetHost"))));
		TestNotNull(TEXT("SpecialZone binds its read-only content card container"),
			Cast<UWrapBox>(SpecialZoneTree->FindWidget(TEXT("ContentCardsBox"))));
	}

	if (ScreenClass && SpecialZoneClass)
	{
		TestEqual(
			TEXT("Formal Screen CDO selects the generated compatibility SpecialZone class"),
			ReadObjectDefault(ScreenClass, TEXT("SpecialZoneWidgetClass")),
			static_cast<UObject*>(SpecialZoneClass));
	}

	UWidgetTree* ScreenTree = GetWidgetTree(ScreenClass);
	TestNotNull(TEXT("Formal Backpack screen has a compiled widget tree"), ScreenTree);
	if (ScreenTree)
	{
		TestNull(
			TEXT("Compatibility SpecialZone asset does not restore the old simultaneous-zone host"),
			ScreenTree->FindWidget(TEXT("SpecialZonesHost")));
	}

	TStrongObjectPtr<UWacomSpecialZoneWidget> NativeFallback(
		NewObject<UWacomSpecialZoneWidget>(GetTransientPackage()));
	NativeFallback->TakeWidget();
	TestFalse(
		TEXT("Native SpecialZone fallback builds an explicit diagnostic title instead of an empty panel"),
		FWacomBackpackScreenTestAccess::ZoneTitleText(*NativeFallback).IsEmpty());
	TestEqual(
		TEXT("Native SpecialZone fallback starts without invented content cards"),
		FWacomBackpackScreenTestAccess::ContentCardCount(*NativeFallback),
		0);
	return true;
}

#endif
