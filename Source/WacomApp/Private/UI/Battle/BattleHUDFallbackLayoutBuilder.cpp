// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/BattleHUDFallbackLayoutBuilder.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "UI/Battle/ActionPanel.h"
#include "UI/Battle/BattleCombatLogFeedWidget.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/BattlePresentationStackWidget.h"
#include "UI/Battle/EnemyInfoBar.h"
#include "UI/Battle/EquipmentBar.h"
#include "UI/Battle/HandPanel.h"
#include "UI/Battle/PlayerStatusBar.h"
#include "UI/Common/PileCountView.h"

#define LOCTEXT_NAMESPACE "WacomBattleHUD"

namespace
{
	const TCHAR* HandPanelPath = TEXT("/Game/Wacom/UI/Battle/WBP_HandPanel.WBP_HandPanel_C");

	template <typename TWidget>
	TWidget* ConstructWidget(UWidgetTree* WidgetTree, TObjectPtr<TWidget>* OutWidget, FName Name)
	{
		if (!WidgetTree || !OutWidget)
		{
			return nullptr;
		}

		TWidget* Widget = WidgetTree->ConstructWidget<TWidget>(TWidget::StaticClass(), Name);
		*OutWidget = Widget;
		return Widget;
	}

	void SetCanvasSlot(
		UCanvasPanelSlot* Slot,
		const FAnchors& Anchors,
		const FVector2D& Alignment,
		const FMargin& Offsets,
		int32 ZOrder = 0)
	{
		if (!Slot)
		{
			return;
		}

		Slot->SetAnchors(Anchors);
		Slot->SetAlignment(Alignment);
		Slot->SetOffsets(Offsets);
		Slot->SetAutoSize(false);
		if (ZOrder != 0)
		{
			Slot->SetZOrder(ZOrder);
		}
	}

	UHandPanel* ConstructHandPanel(UWidgetTree* WidgetTree)
	{
		TSubclassOf<UHandPanel> HandPanelClass = UHandPanel::StaticClass();
		if (UClass* LoadedHandPanelClass = LoadClass<UHandPanel>(nullptr, HandPanelPath))
		{
			HandPanelClass = LoadedHandPanelClass;
		}

		return WidgetTree
			? WidgetTree->ConstructWidget<UHandPanel>(HandPanelClass, TEXT("HandPanel"))
			: nullptr;
	}

	void AddPileView(
		UWidgetTree* WidgetTree,
		UCanvasPanel* Root,
		TObjectPtr<UPileCountView>* OutPileView,
		FName Name,
		const FText& Label,
		const FAnchors& Anchors,
		const FVector2D& Alignment,
		const FMargin& Offsets)
	{
		UPileCountView* PileView = ConstructWidget(WidgetTree, OutPileView, Name);
		if (!PileView || !Root)
		{
			return;
		}

		PileView->SetLabel(Label);
		SetCanvasSlot(Root->AddChildToCanvas(PileView), Anchors, Alignment, Offsets);
	}
}

void FBattleHUDFallbackLayoutBuilder::Build(const FBattleHUDFallbackLayoutBuilderContext& Context)
{
	if (!Context.Owner || !Context.WidgetTree)
	{
		return;
	}

	UCanvasPanel* Root = Context.WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	Context.WidgetTree->RootWidget = Root;
	if (!Root)
	{
		return;
	}

	if (UEnemyInfoBar* EnemyInfoBar = ConstructWidget(Context.WidgetTree, Context.EnemyInfoBar, TEXT("EnemyInfoBar")))
	{
		SetCanvasSlot(
			Root->AddChildToCanvas(EnemyInfoBar),
			FAnchors(0.5f, 0.0f),
			FVector2D(0.5f, 0.0f),
			FMargin(0.0f, 20.0f, 720.0f, 130.0f));
	}

	if (UPlayerStatusBar* PlayerStatusBar = ConstructWidget(Context.WidgetTree, Context.PlayerStatusBar, TEXT("PlayerStatusBar")))
	{
		SetCanvasSlot(
			Root->AddChildToCanvas(PlayerStatusBar),
			FAnchors(0.0f, 1.0f),
			FVector2D(0.0f, 1.0f),
			FMargin(20.0f, -140.0f, 220.0f, 120.0f));
	}

	if (Context.HandPanel)
	{
		*Context.HandPanel = ConstructHandPanel(Context.WidgetTree);
		if (UHandPanel* HandPanel = Context.HandPanel->Get())
		{
			SetCanvasSlot(
				Root->AddChildToCanvas(HandPanel),
				FAnchors(0.5f, 1.0f),
				FVector2D(0.5f, 1.0f),
				FMargin(0.0f, -Context.HandPanelBottomOffset, Context.HandPanelSize.X, Context.HandPanelSize.Y));
		}
	}

	if (UActionPanel* ActionPanel = ConstructWidget(Context.WidgetTree, Context.ActionPanel, TEXT("ActionPanel")))
	{
		SetCanvasSlot(
			Root->AddChildToCanvas(ActionPanel),
			FAnchors(1.0f, 1.0f),
			FVector2D(1.0f, 1.0f),
			FMargin(-30.0f, -200.0f, 160.0f, 140.0f));
	}

	if (UEquipmentBar* EquipmentBar = ConstructWidget(Context.WidgetTree, Context.EquipmentBar, TEXT("EquipmentBar")))
	{
		SetCanvasSlot(
			Root->AddChildToCanvas(EquipmentBar),
			FAnchors(0.0f, 0.0f),
			FVector2D(0.0f, 0.0f),
			FMargin(20.0f, 20.0f, 240.0f, 32.0f));
	}

	AddPileView(
		Context.WidgetTree,
		Root,
		Context.DrawPileView,
		TEXT("DrawPileView"),
		LOCTEXT("DrawPile", "抽牌堆"),
		FAnchors(0.0f, 1.0f),
		FVector2D(0.0f, 1.0f),
		FMargin(20.0f, -20.0f, 80.0f, 80.0f));

	AddPileView(
		Context.WidgetTree,
		Root,
		Context.DiscardPileView,
		TEXT("DiscardPileView"),
		LOCTEXT("DiscardPile", "弃牌堆"),
		FAnchors(1.0f, 1.0f),
		FVector2D(1.0f, 1.0f),
		FMargin(-200.0f, -20.0f, 80.0f, 80.0f));

	AddPileView(
		Context.WidgetTree,
		Root,
		Context.ExhaustPileView,
		TEXT("ExhaustPileView"),
		LOCTEXT("ExhaustPile", "消耗牌堆"),
		FAnchors(1.0f, 1.0f),
		FVector2D(1.0f, 1.0f),
		FMargin(-200.0f, -110.0f, 80.0f, 80.0f));

	if (UBattleCombatLogFeedWidget* CombatLogFeed = ConstructWidget(Context.WidgetTree, Context.CombatLogFeed, TEXT("CombatLogFeed")))
	{
		CombatLogFeed->SetVisibility(ESlateVisibility::Collapsed);
		SetCanvasSlot(
			Root->AddChildToCanvas(CombatLogFeed),
			FAnchors(1.0f, 0.24f),
			FVector2D(1.0f, 0.0f),
			FMargin(-20.0f, 0.0f, 420.0f, 360.0f),
			7);
	}

	if (UBattlePresentationStackWidget* PresentationStack = ConstructWidget(Context.WidgetTree, Context.BattlePresentationStack, TEXT("BattlePresentationStack")))
	{
		PresentationStack->SetVisibility(ESlateVisibility::Collapsed);
		SetCanvasSlot(
			Root->AddChildToCanvas(PresentationStack),
			FAnchors(1.0f, 0.24f),
			FVector2D(1.0f, 0.0f),
			FMargin(-455.0f, 12.0f, 220.0f, 260.0f),
			8);
	}

	if (UCanvasPanel* CardDetailLayer = ConstructWidget(Context.WidgetTree, Context.CardDetailLayer, TEXT("CardDetailLayer")))
	{
		CardDetailLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
		SetCanvasSlot(
			Root->AddChildToCanvas(CardDetailLayer),
			FAnchors(0.0f, 0.0f, 1.0f, 1.0f),
			FVector2D(0.0f, 0.0f),
			FMargin(0.0f),
			10);
	}
}

#undef LOCTEXT_NAMESPACE
