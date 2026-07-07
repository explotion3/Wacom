// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunStateTypes.h"

class UCanvasPanel;
class UPanelWidget;
class UTextBlock;
class UVerticalBox;
class UWidgetTree;
class UWrapBox;
class UWacomBackpackScreen;
class UWacomDeleteZoneDropTarget;
class UWacomZoneDropTarget;

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

	TObjectPtr<UWacomDeleteZoneDropTarget>* DeleteDropTarget = nullptr;
	TObjectPtr<UWacomZoneDropTarget>* BattleDeckDropTarget = nullptr;
	TObjectPtr<UWacomZoneDropTarget>* BackpackDropTarget = nullptr;
};

/**
 * BackpackScreen 的运行时区域控件 builder。
 *
 * 只创建 DropTarget、WrapBox、详情层等运行时子控件，并回填给 Screen。
 */
struct FBackpackRuntimeZoneBuilder
{
	static void Ensure(const FBackpackRuntimeZoneBuilderContext& Context);
};
