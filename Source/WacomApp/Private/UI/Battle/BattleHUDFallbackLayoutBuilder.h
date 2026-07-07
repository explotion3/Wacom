// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UBattleCommandBarWidget;
class UBattleCombatLogFeedWidget;
class UBattlePresentationStackWidget;
class UPlayerStatusBar;
class UPileCountView;
class UWidgetTree;
class UBattleHUD;

struct FBattleHUDFallbackLayoutBuilderContext
{
	UBattleHUD* Owner = nullptr;
	UWidgetTree* WidgetTree = nullptr;

	TObjectPtr<UPlayerStatusBar>* PlayerStatusBar = nullptr;
	TObjectPtr<UBattleCommandBarWidget>* CommandBar = nullptr;
	TObjectPtr<UPileCountView>* DrawPileView = nullptr;
	TObjectPtr<UPileCountView>* DiscardPileView = nullptr;
	TObjectPtr<UPileCountView>* ExhaustPileView = nullptr;
	TObjectPtr<UBattleCombatLogFeedWidget>* CombatLogFeed = nullptr;
	TObjectPtr<UBattlePresentationStackWidget>* BattlePresentationStack = nullptr;
};

/**
 * BattleHUD 的 C++ fallback widget tree builder。
 *
 * 只在没有完整 BattleHUD WBP 根布局时搭默认 CanvasPanel，并把 BindWidget 字段回填给 HUD。
 */
struct FBattleHUDFallbackLayoutBuilder
{
	static void Build(const FBattleHUDFallbackLayoutBuilderContext& Context);
};
