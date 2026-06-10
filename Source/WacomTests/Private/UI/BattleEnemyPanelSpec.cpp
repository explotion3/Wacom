// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "UI/Battle/WacomBattleEnemyPanelViewData.h"
#include "UI/Battle/WacomBattleEnemyPanelWidget.h"
#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomBattleEnemyPanelSpec
{
	FWacomBattleEnemyPartEntryViewData MakePart(
		FName PartSlotId,
		const FString& DisplayName,
		int32 CurrentHp,
		int32 MaxHp,
		int32 Shield,
		int32 Initiative,
		const FString& Intent,
		bool bDestroyed = false)
	{
		FWacomBattleEnemyPartEntryViewData View;
		View.PartSlotId = PartSlotId;
		View.PartDisplayName = FText::FromString(DisplayName);
		View.CurrentHp = CurrentHp;
		View.MaxHp = MaxHp;
		View.Shield = Shield;
		View.CurrentInitiative = Initiative;
		View.CurrentIntentDisplayName = FText::FromString(Intent);
		View.CurrentIntentInitiative = Initiative;
		View.bDestroyed = bDestroyed;
		return View;
	}

	UTextBlock* FindTextBlock(UWidgetTree* WidgetTree, FName WidgetName)
	{
		if (!WidgetTree)
		{
			return nullptr;
		}
		return Cast<UTextBlock>(WidgetTree->FindWidget(WidgetName));
	}

	UVerticalBox* FindVerticalBox(UWidgetTree* WidgetTree, FName WidgetName)
	{
		if (!WidgetTree)
		{
			return nullptr;
		}
		return Cast<UVerticalBox>(WidgetTree->FindWidget(WidgetName));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPartEntryShowsViewDataSpec,
	"Wacom.UI.Battle.EnemyPanel.PartEntryShowsViewData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPartEntryShowsViewDataSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyPanelSpec;

	TStrongObjectPtr<UWacomBattleEnemyPartEntryWidget> Widget(NewObject<UWacomBattleEnemyPartEntryWidget>());
	Widget->TakeWidget();

	FWacomBattleEnemyPartEntryViewData View = MakePart(
		TEXT("Head"),
		TEXT("蛇头"),
		7,
		12,
		4,
		3,
		TEXT("撕咬"));
	Widget->SetPartEntryViewData(View);

	UWidgetTree* WidgetTree = Widget->WidgetTree;
	UTextBlock* PartNameText = FindTextBlock(WidgetTree, TEXT("PartNameText"));
	UTextBlock* StatsText = FindTextBlock(WidgetTree, TEXT("StatsText"));
	UTextBlock* IntentText = FindTextBlock(WidgetTree, TEXT("IntentText"));
	UTextBlock* StatusText = FindTextBlock(WidgetTree, TEXT("StatusText"));

	if (!TestNotNull(TEXT("PartNameText"), PartNameText)
		|| !TestNotNull(TEXT("StatsText"), StatsText)
		|| !TestNotNull(TEXT("IntentText"), IntentText)
		|| !TestNotNull(TEXT("StatusText"), StatusText))
	{
		return false;
	}

	TestEqual(TEXT("Part name"), PartNameText->GetText().ToString(), FString(TEXT("蛇头")));
	TestTrue(TEXT("Stats include HP"), StatsText->GetText().ToString().Contains(TEXT("7/12")));
	TestTrue(TEXT("Stats include shield"), StatsText->GetText().ToString().Contains(TEXT("SH 4")));
	TestTrue(TEXT("Stats include initiative"), StatsText->GetText().ToString().Contains(TEXT("INIT 3")));
	TestTrue(TEXT("Intent includes display name"), IntentText->GetText().ToString().Contains(TEXT("撕咬")));
	TestEqual(TEXT("No status fallback"), StatusText->GetText().ToString(), FString(TEXT("状态：无")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPanelBuildsGroupedFallbackSpec,
	"Wacom.UI.Battle.EnemyPanel.BuildsGroupedFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPanelBuildsGroupedFallbackSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyPanelSpec;

	TStrongObjectPtr<UWacomBattleEnemyPanelWidget> Widget(NewObject<UWacomBattleEnemyPanelWidget>());
	Widget->TakeWidget();

	FWacomBattleEnemyPanelViewData Snake;
	Snake.EnemySlotId = TEXT("Enemy.A");
	Snake.EnemyDisplayName = FText::FromString(TEXT("林蛇"));
	Snake.EnemyInitiativeSum = 8;
	Snake.Parts.Add(MakePart(TEXT("Head"), TEXT("蛇头"), 10, 12, 0, 5, TEXT("撕咬")));
	Snake.Parts.Add(MakePart(TEXT("Tail"), TEXT("蛇尾"), 6, 8, 2, 3, TEXT("扫尾")));

	FWacomBattleEnemyPanelViewData Guard;
	Guard.EnemySlotId = TEXT("Enemy.B");
	Guard.EnemyDisplayName = FText::FromString(TEXT("守卫"));
	Guard.EnemyInitiativeSum = 4;
	Guard.Parts.Add(MakePart(TEXT("Body"), TEXT("躯干"), 18, 18, 5, 4, TEXT("格挡")));

	Widget->SetEnemyPanelViewData({ Snake, Guard });

	UVerticalBox* EnemyListBox = FindVerticalBox(Widget->WidgetTree, TEXT("EnemyListBox"));
	if (!TestNotNull(TEXT("EnemyListBox"), EnemyListBox))
	{
		return false;
	}

	TestEqual(TEXT("Two enemies rendered"), EnemyListBox->GetChildrenCount(), 2);

	UVerticalBox* FirstEnemyBox = Cast<UVerticalBox>(EnemyListBox->GetChildAt(0));
	UVerticalBox* SecondEnemyBox = Cast<UVerticalBox>(EnemyListBox->GetChildAt(1));
	if (!TestNotNull(TEXT("First enemy box"), FirstEnemyBox)
		|| !TestNotNull(TEXT("Second enemy box"), SecondEnemyBox))
	{
		return false;
	}

	TestEqual(TEXT("First enemy has header and two parts"), FirstEnemyBox->GetChildrenCount(), 3);
	TestEqual(TEXT("Second enemy has header and one part"), SecondEnemyBox->GetChildrenCount(), 2);

	const UTextBlock* FirstHeader = Cast<UTextBlock>(FirstEnemyBox->GetChildAt(0));
	const UTextBlock* SecondHeader = Cast<UTextBlock>(SecondEnemyBox->GetChildAt(0));
	if (!TestNotNull(TEXT("First header"), FirstHeader)
		|| !TestNotNull(TEXT("Second header"), SecondHeader))
	{
		return false;
	}

	TestTrue(TEXT("First header includes name"), FirstHeader->GetText().ToString().Contains(TEXT("林蛇")));
	TestTrue(TEXT("First header includes slot"), FirstHeader->GetText().ToString().Contains(TEXT("Enemy.A")));
	TestTrue(TEXT("First header includes initiative sum"), FirstHeader->GetText().ToString().Contains(TEXT("INIT 8")));
	TestTrue(TEXT("Second header includes name"), SecondHeader->GetText().ToString().Contains(TEXT("守卫")));

	Widget->SetEnemyPanelViewData({});
	TestEqual(TEXT("Empty panel clears enemies"), EnemyListBox->GetChildrenCount(), 0);

	return true;
}
