// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "UI/Battle/BattleCombatActivityRowWidget.h"
#include "UI/Battle/BattleCombatLogFeedWidget.h"
#include "UI/Battle/WacomBattleCombatActivityStyle.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomBattleCombatActivityWidgetIdentitySpec
{
	FWacomBattleCombatActivityRowView MakeRow(
		const EWacomBattleCombatActivityRowKind Kind,
		const int32 EventSequence)
	{
		FWacomBattleCombatActivityRowView Row;
		Row.RowKind = Kind;
		Row.SourceEventType = Kind == EWacomBattleCombatActivityRowKind::RootAction
			? EBattleEventType::CardPlayed
			: EBattleEventType::DamageDealt;
		Row.EventSequence = EventSequence;
		Row.MessageText = FText::AsNumber(EventSequence);
		return Row;
	}

	UBattleCombatActivityRowWidget* FindRowWidgetBySequence(
		const UBattleCombatLogFeedWidget& Feed,
		const int32 EventSequence)
	{
		const UPanelWidget* RowsPanel = Feed.WidgetTree
			? Cast<UPanelWidget>(Feed.WidgetTree->FindWidget(TEXT("ActivityRowsBox")))
			: nullptr;
		if (!RowsPanel)
		{
			return nullptr;
		}
		for (int32 ChildIndex = 0; ChildIndex < RowsPanel->GetChildrenCount(); ++ChildIndex)
		{
			UBattleCombatActivityRowWidget* RowWidget =
				Cast<UBattleCombatActivityRowWidget>(RowsPanel->GetChildAt(ChildIndex));
			if (RowWidget
				&& RowWidget->GetCurrentRow().EventSequence == EventSequence
				&& RowWidget->GetVisibility() != ESlateVisibility::Collapsed)
			{
				return RowWidget;
			}
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatActivityStableWidgetIdentitySpec,
	"Wacom.UI.Battle.CombatActivity.Widget.StablePlaybackIdentitySurvivesRetirement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatActivityStableWidgetIdentitySpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleCombatActivityWidgetIdentitySpec;

	TStrongObjectPtr<UWacomBattleCombatActivityStyle> Style(
		NewObject<UWacomBattleCombatActivityStyle>(GetTransientPackage()));
	Style->EnterSeconds = 0.0f;
	Style->ResultStaggerSeconds = 0.0f;
	Style->MinimumResultStaggerSeconds = 0.0f;
	Style->MinimumReadableSeconds = 0.0f;
	Style->MinimumResultVisibleSeconds = 10.0f;
	Style->ShiftSeconds = 0.0f;
	Style->BottomRowHoldSeconds = 10.0f;
	Style->TopRowHoldSeconds = 10.0f;
	Style->BottomRowFadeSeconds = 0.10f;
	Style->TopRowFadeSeconds = 0.10f;
	Style->ActivityViewportHeightPixels = 220.0f;
	Style->MinimumVisibleResultRows = 5;
	Style->RowHeightPixels = 40.0f;

	TStrongObjectPtr<UBattleCombatLogFeedWidget> Feed(
		NewObject<UBattleCombatLogFeedWidget>(GetTransientPackage()));
	Feed->ActivityStyle = Style.Get();
	const TSharedRef<SWidget> SlateWidget = Feed->TakeWidget();

	const FWacomBattleCombatActivityRowView Root =
		MakeRow(EWacomBattleCombatActivityRowKind::RootAction, 10);
	const TArray<FWacomBattleCombatActivityRowView> Results{
		MakeRow(EWacomBattleCombatActivityRowKind::Result, 11),
		MakeRow(EWacomBattleCombatActivityRowKind::Result, 12),
		MakeRow(EWacomBattleCombatActivityRowKind::Result, 13),
	};
	Feed->BeginSynchronizedCombatActivityGroup(1, 0, Root, 1);
	Feed->ReleaseSynchronizedCombatActivityResults(1, 0, Results);
	Feed->CompleteSynchronizedCombatActivityTransaction(1);
	Feed->AdvanceActivityPlaybackForTest(0.0f);
	Feed->AdvanceActivityPlaybackForTest(0.0f);

	UBattleCombatActivityRowWidget* SecondBefore =
		FindRowWidgetBySequence(*Feed, 12);
	UBattleCombatActivityRowWidget* ThirdBefore =
		FindRowWidgetBySequence(*Feed, 13);
	TestNotNull(TEXT("Second surviving result has a widget before retirement"), SecondBefore);
	TestNotNull(TEXT("Third surviving result has a widget before retirement"), ThirdBefore);

	Style->MinimumResultVisibleSeconds = 0.0f;
	Style->BottomRowHoldSeconds = 0.0f;
	Style->TopRowHoldSeconds = 0.0f;
	Feed->AdvanceActivityPlaybackForTest(0.0f);
	Feed->AdvanceActivityPlaybackForTest(0.11f);

	TestNull(TEXT("The oldest result completes retirement"),
		FindRowWidgetBySequence(*Feed, 11));
	UBattleCombatActivityRowWidget* SecondAfter =
		FindRowWidgetBySequence(*Feed, 12);
	UBattleCombatActivityRowWidget* ThirdAfter =
		FindRowWidgetBySequence(*Feed, 13);
	TestTrue(TEXT("Second result keeps the same widget identity"),
		SecondBefore && SecondAfter == SecondBefore);
	TestTrue(TEXT("Third result keeps the same widget identity"),
		ThirdBefore && ThirdAfter == ThirdBefore);
	TestTrue(TEXT("Surviving rows never receive horizontal render translation"),
		SecondAfter
			&& FMath::IsNearlyZero(SecondAfter->GetRenderTransform().Translation.X)
			&& ThirdAfter
			&& FMath::IsNearlyZero(ThirdAfter->GetRenderTransform().Translation.X));
	return true;
}
