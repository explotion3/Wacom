// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UButton;
class UCanvasPanel;
class UPanelWidget;
class UTextBlock;
class UUserWidget;
class UWidgetTree;
class UWacomBackpackZoneSectionWidget;

struct FBackpackFallbackLayoutBuilderContext
{
	UUserWidget* Owner = nullptr;
	UWidgetTree* WidgetTree = nullptr;

	TSubclassOf<UWacomBackpackZoneSectionWidget> DeleteZoneSectionWidgetClass;
	TSubclassOf<UWacomBackpackZoneSectionWidget> BattleDeckZoneSectionWidgetClass;
	TSubclassOf<UWacomBackpackZoneSectionWidget> FluxContentZoneSectionWidgetClass;
	TSubclassOf<UWacomBackpackZoneSectionWidget> SpecialZonesSectionWidgetClass;
	TSubclassOf<UWacomBackpackZoneSectionWidget> BurdenZoneSectionWidgetClass;

	TObjectPtr<UTextBlock>* TitleText = nullptr;
	TObjectPtr<UTextBlock>* GoldText = nullptr;
	TObjectPtr<UTextBlock>* BackpackTitleText = nullptr;
	TObjectPtr<UPanelWidget>* DeleteZoneHost = nullptr;
	TObjectPtr<UPanelWidget>* BattleDeckZoneHost = nullptr;
	TObjectPtr<UPanelWidget>* FluxContentDropTargetHost = nullptr;
	TObjectPtr<UPanelWidget>* SpecialZonesHost = nullptr;
	TObjectPtr<UPanelWidget>* BurdenZoneHost = nullptr;
	TObjectPtr<UCanvasPanel>* CardDetailLayer = nullptr;
	TObjectPtr<UButton>* CloseButton = nullptr;
	TObjectPtr<UWacomBackpackZoneSectionWidget>* BattleDeckZoneSection = nullptr;
	TObjectPtr<UWacomBackpackZoneSectionWidget>* FluxContentZoneSection = nullptr;
	TObjectPtr<UWacomBackpackZoneSectionWidget>* BurdenZoneSection = nullptr;
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
