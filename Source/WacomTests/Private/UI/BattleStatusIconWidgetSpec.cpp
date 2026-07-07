// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GameplayTagContainer.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Battle/PlayerStatusBar.h"
#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"
#include "UI/Battle/WacomBattleEnemyPanelViewData.h"
#include "UI/Battle/WacomBattleStatusIconWidget.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomBattleStatusIconWidgetSpec
{
	UWacomBattleStatusIconListWidget* FindStatusList(UWidgetTree* WidgetTree)
	{
		return WidgetTree
			? Cast<UWacomBattleStatusIconListWidget>(WidgetTree->FindWidget(TEXT("StatusList")))
			: nullptr;
	}

	UTextBlock* FindTextBlock(UWidgetTree* WidgetTree, FName WidgetName)
	{
		return WidgetTree
			? Cast<UTextBlock>(WidgetTree->FindWidget(WidgetName))
			: nullptr;
	}

	FWacomBattleEnemyPartEntryViewData MakeEnemyPartView()
	{
		FWacomBattleEnemyPartEntryViewData View;
		View.EnemySlotId = TEXT("Enemy");
		View.PartSlotId = TEXT("Head");
		View.PartDisplayName = FText::FromString(TEXT("蛇头"));
		View.CurrentHp = 7;
		View.MaxHp = 12;
		View.CurrentInitiative = 3;
		View.CurrentIntentDisplayName = FText::FromString(TEXT("撕咬"));
		return View;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleStatusIconListBuildsSortedViewsSpec,
	"Wacom.UI.Battle.StatusIcons.ListBuildsSortedViews",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleStatusIconListBuildsSortedViewsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleStatusIconListWidget> Widget(NewObject<UWacomBattleStatusIconListWidget>());
	Widget->TakeWidget();

	FGameplayTagContainer Statuses;
	Statuses.AddTag(WacomTags::Status_Twilight);
	Statuses.AddTag(WacomTags::Status_Shield);
	Statuses.AddTag(WacomTags::Status_Poison);
	Statuses.AddTag(WacomTags::Status_Stunned);

	TMap<FGameplayTag, int32> Stacks;
	Stacks.Add(WacomTags::Status_Poison, 3);
	Stacks.Add(WacomTags::Status_Twilight, 0);
	Stacks.Add(WacomTags::Status_Stunned, 2);
	Stacks.Add(WacomTags::Status_Shield, 99);

	Widget->SetStatuses(Statuses, Stacks);

	const TArray<FWacomBattleStatusIconView> Views = Widget->GetStatusIconViews();
	TestEqual(TEXT("Shield is ignored"), Views.Num(), 3);
	if (Views.Num() != 3)
	{
		return false;
	}

	TestTrue(TEXT("Poison sorts first"), Views[0].StatusTag == WacomTags::Status_Poison);
	TestEqual(TEXT("Poison stack"), Views[0].StackCount, 3);
	TestTrue(TEXT("Twilight sorts before stunned"), Views[1].StatusTag == WacomTags::Status_Twilight);
	TestEqual(TEXT("Zero stack displays as one"), Views[1].StackCount, 1);
	TestTrue(TEXT("Stunned sorts last"), Views[2].StatusTag == WacomTags::Status_Stunned);
	TestEqual(TEXT("List visible with statuses"), Widget->GetVisibility(), ESlateVisibility::HitTestInvisible);

	UPanelWidget* Container = Widget->WidgetTree
		? Cast<UPanelWidget>(Widget->WidgetTree->FindWidget(TEXT("StatusContainer")))
		: nullptr;
	if (!TestNotNull(TEXT("StatusContainer"), Container))
	{
		return false;
	}
	TestEqual(TEXT("One icon widget per status"), Container->GetChildrenCount(), 3);

	Widget->SetStatuses(FGameplayTagContainer(), TMap<FGameplayTag, int32>());
	TestEqual(TEXT("Empty status list collapses"), Widget->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Empty status list removes children"), Container->GetChildrenCount(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleStatusIconNormalizesBrushSizeSpec,
	"Wacom.UI.Battle.StatusIcons.IconNormalizesBrushSize",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleStatusIconNormalizesBrushSizeSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleStatusIconWidget> Widget(NewObject<UWacomBattleStatusIconWidget>());
	Widget->TakeWidget();

	FWacomBattleStatusIconView View;
	View.StatusTag = WacomTags::Status_Poison;
	View.DisplayName = FText::FromString(TEXT("中毒"));
	View.StackCount = 2;
	View.IconBrush.DrawAs = ESlateBrushDrawType::Image;
	View.IconBrush.SetImageSize(FVector2f::ZeroVector);
	Widget->SetStatusIconView(View);

	UImage* IconImage = Widget->WidgetTree
		? Cast<UImage>(Widget->WidgetTree->FindWidget(TEXT("IconImage")))
		: nullptr;
	if (!TestNotNull(TEXT("IconImage"), IconImage))
	{
		return false;
	}

	const FVector2f ImageSize = IconImage->GetBrush().GetImageSize();
	TestTrue(TEXT("Icon brush has usable image size"), ImageSize.X > 0.0f && ImageSize.Y > 0.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePlayerStatusBarRefreshesStatusListSpec,
	"Wacom.UI.Battle.StatusIcons.PlayerStatusBarRefreshesStatusList",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePlayerStatusBarRefreshesStatusListSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleStatusIconWidgetSpec;

	TStrongObjectPtr<UPlayerStatusBar> Widget(NewObject<UPlayerStatusBar>());
	Widget->TakeWidget();

	UWacomBattleStatusIconListWidget* StatusList = FindStatusList(Widget->WidgetTree);
	if (!TestNotNull(TEXT("Player StatusList"), StatusList))
	{
		return false;
	}
	TestNull(TEXT("SanText no longer generated"), Widget->WidgetTree->FindWidget(TEXT("SanText")));

	FBattleSnapshot Snap;
	Snap.Player.CurrentHp = 12;
	Snap.Player.MaxHp = 20;
	Snap.Player.Shield = 5;
	Snap.Player.Statuses.AddTag(WacomTags::Status_Freeze);
	Snap.Player.Statuses.AddTag(WacomTags::Status_Poison);
	Snap.Player.StatusStacks.Add(WacomTags::Status_Freeze, 1);
	Snap.Player.StatusStacks.Add(WacomTags::Status_Poison, 4);

	Widget->RefreshFromSnapshot(Snap);

	const TArray<FWacomBattleStatusIconView> Views = StatusList->GetStatusIconViews();
	TestEqual(TEXT("Player statuses applied"), Views.Num(), 2);
	if (Views.Num() != 2)
	{
		return false;
	}

	TestTrue(TEXT("Player poison sorts first"), Views[0].StatusTag == WacomTags::Status_Poison);
	TestEqual(TEXT("Player poison stack"), Views[0].StackCount, 4);
	TestTrue(TEXT("Player freeze second"), Views[1].StatusTag == WacomTags::Status_Freeze);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPartStatusTextFallbackStillWorksSpec,
	"Wacom.UI.Battle.StatusIcons.EnemyPartStatusTextFallbackStillWorks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPartStatusTextFallbackStillWorksSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleStatusIconWidgetSpec;

	TStrongObjectPtr<UWacomBattleEnemyPartEntryWidget> Widget(NewObject<UWacomBattleEnemyPartEntryWidget>());
	Widget->TakeWidget();

	FWacomBattleEnemyPartEntryViewData View = MakeEnemyPartView();
	View.RuntimeStatuses.AddTag(WacomTags::Status_Poison);
	View.RuntimeStatusStacks.Add(WacomTags::Status_Poison, 2);
	Widget->SetPartEntryViewData(View);

	TestNull(TEXT("Generated fallback does not bind icon StatusList"), FindStatusList(Widget->WidgetTree));

	UTextBlock* StatusText = FindTextBlock(Widget->WidgetTree, TEXT("StatusText"));
	if (!TestNotNull(TEXT("StatusText fallback"), StatusText))
	{
		return false;
	}

	TestTrue(TEXT("Status fallback includes localized status name"), StatusText->GetText().ToString().Contains(TEXT("中毒")));
	TestTrue(TEXT("Status fallback includes stack count"), StatusText->GetText().ToString().Contains(TEXT("x2")));
	TestEqual(TEXT("Status fallback visible"), StatusText->GetVisibility(), ESlateVisibility::HitTestInvisible);

	return true;
}
