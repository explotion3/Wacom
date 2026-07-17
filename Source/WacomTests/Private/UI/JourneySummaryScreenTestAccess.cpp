// Copyright Wacom. All Rights Reserved.

#include "UI/JourneySummaryScreenTestAccess.h"

#if WITH_AUTOMATION_TESTS

#include "Blueprint/WidgetTree.h"
#include "Input/Events.h"
#include "UI/Foundation/WacomMenuButtonWidget.h"

void FWacomJourneySummaryScreenTestAccess::Build(UWacomJourneySummaryScreen& Screen)
{
	Screen.Initialize();
	Screen.TakeWidget();
}

void FWacomJourneySummaryScreenTestAccess::Construct(UWacomJourneySummaryScreen& Screen)
{
	Screen.NativeConstruct();
}

void FWacomJourneySummaryScreenTestAccess::Destruct(UWacomJourneySummaryScreen& Screen)
{
	Screen.NativeDestruct();
}

void FWacomJourneySummaryScreenTestAccess::ClickContinue(UWacomJourneySummaryScreen& Screen)
{
	if (Screen.ContinueButton)
	{
		Screen.ContinueButton->OnClicked().Broadcast();
	}
}

FReply FWacomJourneySummaryScreenTestAccess::SendEscapeKeyDown(
	UWacomJourneySummaryScreen& Screen)
{
	const FKeyEvent KeyEvent(
		EKeys::Escape,
		FModifierKeysState(),
		0,
		false,
		0,
		0);
	return Screen.NativeOnKeyDown(FGeometry(), KeyEvent);
}

TArray<FName> FWacomJourneySummaryScreenTestAccess::WidgetNames(
	const UWacomJourneySummaryScreen& Screen)
{
	TArray<FName> Names;
	if (Screen.WidgetTree)
	{
		Screen.WidgetTree->ForEachWidget(
			[&Names](UWidget* Widget)
			{
				if (Widget)
				{
					Names.Add(Widget->GetFName());
				}
			});
	}
	return Names;
}

FWacomJourneySummaryScreenAutomationTestView
FWacomJourneySummaryScreenTestAccess::View(const UWacomJourneySummaryScreen& Screen)
{
	return Screen.GetAutomationTestViewForTest();
}

#endif
