// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "UI/Battle/BattlePresentationStackEntryWidget.h"
#include "UI/Battle/BattlePresentationStackWidget.h"
#include "UI/Card/WacomCardView.h"
#include "UI/Card/WacomFirstPersonCardViewWidget.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationStackWidgetOrderSpec,
	"Wacom.UI.Battle.BattlePresentationStackWidgetStacksOldestOnTopNewestOnBottom",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationStackWidgetOrderSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UBattlePresentationStackWidget> Stack(NewObject<UBattlePresentationStackWidget>());
	Stack->MaxVisibleEntries = 3;
	Stack->TakeWidget();

	TArray<FWacomBattlePresentationStackEntryView> Entries;
	for (int32 Index = 0; Index < 5; ++Index)
	{
		FWacomBattlePresentationStackEntryView Entry;
		Entry.EntryId = Index + 1;
		Entry.CardInstanceId = FGuid::NewGuid();
		Entry.CardViewData.Name = FText::FromString(FString::Printf(TEXT("Card%d"), Index + 1));
		Entries.Add(Entry);
	}

	Stack->SetPresentationStackEntries(Entries);
	TestEqual(TEXT("Internal entries preserve oldest-to-newest order"), Stack->GetCurrentEntries()[0].EntryId, 1);
	TestEqual(TEXT("Visible entry count trims to max"), Stack->GetVisibleEntryCount(), 3);
	TestEqual(TEXT("All entries retained internally"), Stack->GetCurrentEntries().Num(), 5);
	TestFalse(TEXT("Oldest visible entry does not need text fields"), Stack->GetCurrentEntries()[0].CardViewData.Name.IsEmpty());

	Stack->ClearPresentationStack();
	TestEqual(TEXT("Clear removes entries"), Stack->GetCurrentEntries().Num(), 0);
	TestEqual(TEXT("Clear removes visible entries"), Stack->GetVisibleEntryCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationStackFallbackSpec,
	"Wacom.UI.Battle.BattlePresentationStackUsesConfigurableMiniCardViewFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationStackFallbackSpec::RunTest(const FString& /*Parameters*/)
{
	UClass* DefaultCardViewClass = LoadClass<UWacomCardView>(
		nullptr,
		TEXT("/Game/Wacom/UI/Card/WBP_CardView.WBP_CardView_C"));
	UClass* FirstPersonCardViewClass = LoadClass<UWacomFirstPersonCardViewWidget>(
		nullptr,
		TEXT("/Game/Wacom/UI/Card/WBP_FPCardView.WBP_FPCardView_C"));

	TStrongObjectPtr<UBattlePresentationStackWidget> DefaultStack(NewObject<UBattlePresentationStackWidget>());
	if (TestNotNull(TEXT("WBP_CardView loads for presentation stack default"), DefaultCardViewClass))
	{
		TestEqual(
			TEXT("Presentation stack defaults to WBP_CardView"),
			DefaultStack->MiniCardViewClass.Get(),
			DefaultCardViewClass);
	}
	if (FirstPersonCardViewClass)
	{
		TestNotEqual(
			TEXT("Presentation stack does not default to WBP_FPCardView"),
			DefaultStack->MiniCardViewClass.Get(),
			FirstPersonCardViewClass);
	}

	TStrongObjectPtr<UBattlePresentationStackWidget> Stack(NewObject<UBattlePresentationStackWidget>());
	Stack->MiniCardViewClass = UWacomCardView::StaticClass();
	Stack->TakeWidget();

	FWacomBattlePresentationStackEntryView Entry;
	Entry.EntryId = 1;
	Entry.CardViewData.Name = FText::FromString(TEXT("毒牙"));
	Stack->SetPresentationStackEntries({ Entry });

	TestEqual(TEXT("Entry retained"), Stack->GetCurrentEntries().Num(), 1);
	TestEqual(TEXT("Configured fallback class is preserved"), Stack->MiniCardViewClass.Get(), UWacomCardView::StaticClass());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationStackPureCardEntrySpec,
	"Wacom.UI.Battle.BattlePresentationStackEntryIsPureScaledCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationStackPureCardEntrySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UBattlePresentationStackEntryWidget> EntryWidget(NewObject<UBattlePresentationStackEntryWidget>());
	EntryWidget->SetMiniCardViewClass(UWacomCardView::StaticClass());
	EntryWidget->TakeWidget();

	FWacomBattlePresentationStackEntryView Entry;
	Entry.EntryId = 1;
	Entry.CardInstanceId = FGuid::NewGuid();
	Entry.CardViewData.Name = FText::FromString(TEXT("毒牙"));
	EntryWidget->SetPresentationStackEntryData(Entry);

	TestNotNull(TEXT("Entry creates mini card view"), EntryWidget->GetMiniCardView());
	TestTrue(TEXT("Entry uses whole-card scale host"), EntryWidget->HasMiniCardScaleHostForTest());
	TestFalse(TEXT("Entry has no header/target text widgets"), EntryWidget->HasHeaderOrTargetTextWidgetsForTest());
	TestEqual(TEXT("Entry remains hit-test invisible"), EntryWidget->GetVisibility(), ESlateVisibility::HitTestInvisible);

	Entry.bIsExiting = true;
	EntryWidget->SetPresentationStackEntryData(Entry);
	EntryWidget->TickExitForTest(0.08f);
	TestTrue(TEXT("Exit motion fades card"), EntryWidget->GetRenderOpacity() < 1.0f);
	TestTrue(TEXT("Exit motion moves card upward"), EntryWidget->GetRenderTransform().Translation.Y < 0.0f);

	return true;
}
