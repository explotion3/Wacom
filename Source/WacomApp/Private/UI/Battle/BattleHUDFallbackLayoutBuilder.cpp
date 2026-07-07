// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/BattleHUDFallbackLayoutBuilder.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "UI/Battle/BattleCommandBarWidget.h"
#include "UI/Battle/BattleCombatLogFeedWidget.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/BattlePresentationStackWidget.h"
#include "UI/Battle/EquipmentBar.h"
#include "UI/Battle/PlayerStatusBar.h"
#include "UI/Common/PileCountView.h"

namespace
{
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

	void AddPileView(
		UWidgetTree* WidgetTree,
		UCanvasPanel* Root,
		TObjectPtr<UPileCountView>* OutPileView,
		FName Name,
		const FAnchors& Anchors,
		const FVector2D& Alignment,
		const FMargin& Offsets)
	{
		UPileCountView* PileView = ConstructWidget(WidgetTree, OutPileView, Name);
		if (!PileView || !Root)
		{
			return;
		}

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

	if (UPlayerStatusBar* PlayerStatusBar = ConstructWidget(Context.WidgetTree, Context.PlayerStatusBar, TEXT("PlayerStatusBar")))
	{
		SetCanvasSlot(
			Root->AddChildToCanvas(PlayerStatusBar),
			FAnchors(0.0f, 1.0f),
			FVector2D(0.0f, 1.0f),
			FMargin(20.0f, -140.0f, 220.0f, 120.0f));
	}

	if (UBattleCommandBarWidget* CommandBar = ConstructWidget(Context.WidgetTree, Context.CommandBar, TEXT("CommandBar")))
	{
		SetCanvasSlot(
			Root->AddChildToCanvas(CommandBar),
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
		FAnchors(0.0f, 1.0f),
		FVector2D(0.0f, 1.0f),
		FMargin(20.0f, -20.0f, 80.0f, 80.0f));

	AddPileView(
		Context.WidgetTree,
		Root,
		Context.DiscardPileView,
		TEXT("DiscardPileView"),
		FAnchors(1.0f, 1.0f),
		FVector2D(1.0f, 1.0f),
		FMargin(-200.0f, -20.0f, 80.0f, 80.0f));

	AddPileView(
		Context.WidgetTree,
		Root,
		Context.ExhaustPileView,
		TEXT("ExhaustPileView"),
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
}
