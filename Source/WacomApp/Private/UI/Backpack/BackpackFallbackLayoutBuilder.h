// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UButton;
class UCanvasPanel;
class UPanelWidget;
class UTextBlock;
class UUserWidget;
class UWidgetTree;

struct FBackpackFallbackLayoutBuilderContext
{
	UUserWidget* Owner = nullptr;
	UWidgetTree* WidgetTree = nullptr;

	TObjectPtr<UTextBlock>* TitleText = nullptr;
	TObjectPtr<UTextBlock>* GoldText = nullptr;
	TObjectPtr<UPanelWidget>* WorkspaceHost = nullptr;
	TObjectPtr<UPanelWidget>* DeleteTargetHost = nullptr;
	TObjectPtr<UPanelWidget>* DeleteConfirmHost = nullptr;
	TObjectPtr<UButton>* ArrangeAllButton = nullptr;
	TObjectPtr<UButton>* ResetPilePositionsButton = nullptr;
	TObjectPtr<UCanvasPanel>* CardDetailLayer = nullptr;
	TObjectPtr<UButton>* CloseButton = nullptr;
};

/**
 * BackpackScreen 的 C++ fallback widget tree builder。
 *
 * 只在没有 WBP RootWidget 时搭默认布局，并把绑定字段回填给 Screen。
 */
struct FBackpackFallbackLayoutBuilder
{
	static void Build(const FBackpackFallbackLayoutBuilderContext& Context);
	static UTextBlock* CreateBackpackText(UWidgetTree* WidgetTree, FName Name, const FText& Text, int32 FontSize);
};
