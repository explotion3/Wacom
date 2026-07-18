// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameplayTagContainer.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Battle/PlayerStatusBar.h"
#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"
#include "UI/Battle/WacomBattleEnemyPanelViewData.h"
#include "UI/Battle/WacomBattleStatusIconWidget.h"
#include "UI/Common/WacomProgressBar.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomBattleStatusIconWidgetSpec
{
	constexpr TCHAR EnemyPartEntryClassPath[] =
		TEXT("/Game/Wacom/UI/Enemy/BP_WacomBattleEnemyPartEntryWidget.BP_WacomBattleEnemyPartEntryWidget_C");

	UWorld* FindAutomationWorld()
	{
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (UWorld* World = Context.World())
				{
					return World;
				}
			}
		}
		return GWorld;
	}

	UWacomBattleStatusIconListWidget* FindStatusList(UWidgetTree* WidgetTree)
	{
		return WidgetTree
			? Cast<UWacomBattleStatusIconListWidget>(WidgetTree->FindWidget(TEXT("StatusList")))
			: nullptr;
	}

	UWacomProgressBar* FindProgressBar(UWidgetTree* WidgetTree, FName WidgetName)
	{
		return WidgetTree
			? Cast<UWacomProgressBar>(WidgetTree->FindWidget(WidgetName))
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
	FWacomUIBattlePlayerStatusBarActionPreviewSpec,
	"Wacom.UI.Battle.StatusIcons.PlayerStatusBarActionPreviewApplyAndClear",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePlayerStatusBarActionPreviewSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleStatusIconWidgetSpec;

	TStrongObjectPtr<UPlayerStatusBar> Widget(NewObject<UPlayerStatusBar>());
	Widget->TakeWidget();

	UWacomProgressBar* HpBar = FindProgressBar(Widget->WidgetTree, TEXT("HpBar"));
	UTextBlock* ShieldText = FindTextBlock(Widget->WidgetTree, TEXT("ShieldText"));
	UWacomBattleStatusIconListWidget* StatusList = FindStatusList(Widget->WidgetTree);
	if (!TestNotNull(TEXT("HpBar"), HpBar)
		|| !TestNotNull(TEXT("ShieldText"), ShieldText)
		|| !TestNotNull(TEXT("StatusList"), StatusList))
	{
		return false;
	}

	FBattleSnapshot Snap;
	Snap.Player.CurrentHp = 12;
	Snap.Player.MaxHp = 20;
	Snap.Player.Shield = 5;
	Snap.Player.Statuses.AddTag(WacomTags::Status_Poison);
	Snap.Player.StatusStacks.Add(WacomTags::Status_Poison, 4);
	Widget->RefreshFromSnapshot(Snap);

	FPlayerSnapshot PreviewPlayer = Snap.Player;
	PreviewPlayer.CurrentHp = 9;
	PreviewPlayer.Shield = 0;
	PreviewPlayer.Statuses.Reset();
	PreviewPlayer.StatusStacks.Reset();
	PreviewPlayer.Statuses.AddTag(WacomTags::Status_Freeze);
	PreviewPlayer.StatusStacks.Add(WacomTags::Status_Freeze, 2);
	Widget->SetActionPreview(PreviewPlayer);

	TestEqual(TEXT("Preview HP current"), HpBar->GetCurrent(), 9);
	TestEqual(TEXT("Preview keeps the projected shield break visible"),
		ShieldText->GetVisibility(),
		ESlateVisibility::HitTestInvisible);
	TArray<FWacomBattleStatusIconView> PreviewViews = StatusList->GetStatusIconViews();
	TestEqual(TEXT("Preview has one status"), PreviewViews.Num(), 1);
	if (PreviewViews.Num() == 1)
	{
		TestTrue(TEXT("Preview status is freeze"), PreviewViews[0].StatusTag == WacomTags::Status_Freeze);
		TestEqual(TEXT("Preview freeze stack"), PreviewViews[0].StackCount, 2);
	}

	Widget->ClearActionPreview();

	TestEqual(TEXT("Base HP restored"), HpBar->GetCurrent(), 12);
	TestTrue(TEXT("Base shield text restored"), ShieldText->GetText().ToString().Contains(TEXT("5")));
	TArray<FWacomBattleStatusIconView> BaseViews = StatusList->GetStatusIconViews();
	TestEqual(TEXT("Base has one status"), BaseViews.Num(), 1);
	if (BaseViews.Num() == 1)
	{
		TestTrue(TEXT("Base status is poison"), BaseViews[0].StatusTag == WacomTags::Status_Poison);
		TestEqual(TEXT("Base poison stack"), BaseViews[0].StackCount, 4);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPartUsesFormalStatusListSpec,
	"Wacom.UI.Battle.StatusIcons.EnemyPartUsesFormalStatusList",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPartUsesFormalStatusListSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleStatusIconWidgetSpec;

	UWorld* World = FindAutomationWorld();
	UClass* PartEntryClass = LoadClass<UWacomBattleEnemyPartEntryWidget>(nullptr, EnemyPartEntryClassPath);
	if (!TestNotNull(TEXT("Automation world"), World)
		|| !TestNotNull(TEXT("Formal enemy part-entry WBP"), PartEntryClass))
	{
		return false;
	}

	TStrongObjectPtr<UWacomBattleEnemyPartEntryWidget> Widget(
		CreateWidget<UWacomBattleEnemyPartEntryWidget>(World, PartEntryClass));
	if (!TestNotNull(TEXT("Formal enemy part-entry instance"), Widget.Get()))
	{
		return false;
	}
	Widget->TakeWidget();

	FWacomBattleEnemyPartEntryViewData View = MakeEnemyPartView();
	View.RuntimeStatuses.AddTag(WacomTags::Status_Poison);
	View.RuntimeStatusStacks.Add(WacomTags::Status_Poison, 2);
	Widget->SetPartEntryViewData(View);

	UWacomBattleStatusIconListWidget* StatusList = FindStatusList(Widget->WidgetTree);
	if (!TestNotNull(TEXT("Formal StatusList binding"), StatusList))
	{
		return false;
	}

	const TArray<FWacomBattleStatusIconView> Views = StatusList->GetStatusIconViews();
	TestEqual(TEXT("Enemy part has one status icon"), Views.Num(), 1);
	if (Views.Num() == 1)
	{
		TestTrue(TEXT("Enemy part status is poison"), Views[0].StatusTag == WacomTags::Status_Poison);
		TestEqual(TEXT("Enemy part poison stack"), Views[0].StackCount, 2);
	}
	TestEqual(TEXT("Formal status list visible"), StatusList->GetVisibility(), ESlateVisibility::HitTestInvisible);

	return true;
}
