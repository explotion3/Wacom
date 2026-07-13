// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UCanvasPanel;
class UPanelWidget;
class UTextBlock;
class UVerticalBox;
class UWidgetTree;
class UWrapBox;
class UWacomBackpackScreen;

struct FBackpackRuntimeZoneBuilderContext
{
	UWacomBackpackScreen* OwnerScreen = nullptr;
	UWidgetTree* WidgetTree = nullptr;

	TObjectPtr<UCanvasPanel>* CardDetailLayer = nullptr;
	TObjectPtr<UPanelWidget>* DeleteZoneHost = nullptr;
	TObjectPtr<UPanelWidget>* BattleDeckZoneHost = nullptr;
	TObjectPtr<UPanelWidget>* FluxContentDropTargetHost = nullptr;
	TObjectPtr<UPanelWidget>* SpecialZonesHost = nullptr;
	TObjectPtr<UPanelWidget>* BurdenZoneHost = nullptr;

	TObjectPtr<UTextBlock>* DeleteZoneTitleText = nullptr;
	TObjectPtr<UTextBlock>* BurdenZoneTitleText = nullptr;
	TObjectPtr<UWrapBox>* BattleDeckCardsBox = nullptr;
	TObjectPtr<UWrapBox>* FluxContentCardsBox = nullptr;
	TObjectPtr<UVerticalBox>* SpecialZonesPanel = nullptr;
	TObjectPtr<UWrapBox>* BurdenCardsBox = nullptr;
};

/**
 * 旧 C++ fallback 的只读区域 builder。
 *
 * 正式交互只有 Workspace/Screen 一条路径；这里仅补齐详情层和迁移期只读列表，绝不创建 DragDrop owner。
 */
struct FBackpackRuntimeZoneBuilder
{
	static void Ensure(const FBackpackRuntimeZoneBuilderContext& Context);
};
