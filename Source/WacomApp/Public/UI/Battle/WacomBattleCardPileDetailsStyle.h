// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Styling/SlateBrush.h"
#include "WacomBattleCardPileDetailsStyle.generated.h"

class UBattleCardPileEntryWidget;
class UMaterialInterface;
class UWacomCardDetailPanel;
class UWacomCardView;

/** UI-only authoring values for the Battle pile details browser. */
UCLASS(BlueprintType, Const)
class WACOMAPP_API UWacomBattleCardPileDetailsStyle : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Classes",
		meta = (ToolTip = "虚拟化牌堆网格使用的卡牌条目 Widget 类。必须实现 UserObjectListEntry。"))
	TSubclassOf<UBattleCardPileEntryWidget> EntryWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Classes",
		meta = (ToolTip = "牌堆条目使用的正式卡面 WBP 类。默认应为 WBP_CardView；为空时回退通用 UWacomCardView。"))
	TSubclassOf<UWacomCardView> CardViewClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Classes",
		meta = (ToolTip = "悬浮或手柄焦点卡牌旁复用的只读详情面板类。默认应为 WBP_CardDetailPanel。"))
	TSubclassOf<UWacomCardDetailPanel> CardDetailPanelClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Selection",
		meta = (ToolTip = "卡牌临时悬浮、焦点或点击锁定时使用的 UI 流光外框材质实例；只作用于条目外框，不进入卡面 Retainer。"))
	TObjectPtr<UMaterialInterface> SelectionOutlineMaterialInstance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Navigation",
		meta = (ToolTip = "抽牌堆分页图标。默认复用 BattleHUD 的抽牌堆图标资产。"))
	FSlateBrush DrawPileIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Navigation",
		meta = (ToolTip = "弃牌堆分页图标。默认复用 BattleHUD 的弃牌堆图标资产。"))
	FSlateBrush DiscardPileIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Navigation",
		meta = (ToolTip = "消耗区分页图标。默认复用 BattleHUD 的消耗牌堆图标资产。"))
	FSlateBrush ExhaustPileIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Layout",
		meta = (ToolTip = "网格中卡面的原始宽度，单位像素；默认 296。只影响浏览布局，不改变卡牌命中或规则。"))
	float CardWidthPixels = 296.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Layout",
		meta = (ToolTip = "网格中卡面的原始高度，单位像素；默认 420。只影响浏览布局。"))
	float CardHeightPixels = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Layout",
		meta = (ToolTip = "每张卡四周的选中边框留白，单位像素；默认 4，推荐 2 到 10。"))
	float CardEntryPaddingPixels = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Layout",
		meta = (ToolTip = "卡牌网格列间距，单位像素；默认 16，推荐 8 到 32。不会缩放卡牌。"))
	float CardHorizontalSpacingPixels = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Layout",
		meta = (ToolTip = "卡牌网格行间距，单位像素；默认 20，推荐 8 到 36。不会缩放卡牌。"))
	float CardVerticalSpacingPixels = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Layout",
		meta = (ToolTip = "全屏牌堆页面与视口边缘的安全间距，单位像素；默认 24，推荐 16 到 48。"))
	float ScreenSafeMarginPixels = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Layout",
		meta = (ToolTip = "左侧牌堆分页导航栏宽度，单位像素；默认 128，推荐 96 到 160。影响内容区可用宽度。"))
	float NavigationRailWidthPixels = 128.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Selection",
		meta = (ToolTip = "临时悬浮或焦点状态写入流光材质的强度；推荐 0.5 到 1.0，不影响布局。"))
	float HoverOutlineAmount = 0.72f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Selection",
		meta = (ToolTip = "点击锁定选择状态写入流光材质的强度；推荐 0.7 到 1.2，不影响布局。"))
	float LockedOutlineAmount = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Selection",
		meta = (ToolTip = "流光外框伸出卡体四周的距离，单位像素；默认 4，推荐 2 到 8。该值必须小于条目留白，不改变卡牌尺寸。"))
	float SelectionOutlineExtentPixels = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Detail",
		meta = (ToolTip = "详情面板逻辑宽度，单位像素；默认 360，推荐 300 到 440。"))
	float DetailPanelWidthPixels = 360.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Detail",
		meta = (ToolTip = "详情面板逻辑高度，单位像素；默认 420，推荐 360 到 520。"))
	float DetailPanelHeightPixels = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Detail",
		meta = (ToolTip = "详情面板与悬浮卡牌之间的间距，单位像素；默认 12。"))
	float DetailPanelGapPixels = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Detail",
		meta = (ToolTip = "悬浮或焦点持续多久后显示详情，单位秒；默认 0.10，推荐 0 到 0.25。"))
	float DetailHoverDelaySeconds = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Detail",
		meta = (ToolTip = "详情面板淡入时长，单位秒；默认 0.10。简化动效时直接显示。"))
	float DetailFadeInSeconds = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Detail",
		meta = (ToolTip = "详情面板淡出时长，单位秒；默认 0.08。简化动效时直接隐藏。"))
	float DetailFadeOutSeconds = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Colors",
		meta = (ToolTip = "未选中牌堆分页图标与底板颜色。"))
	FLinearColor NavigationIdleColor = FLinearColor(0.20f, 0.25f, 0.30f, 0.62f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Colors",
		meta = (ToolTip = "当前牌堆分页的高亮颜色。"))
	FLinearColor NavigationSelectedColor = FLinearColor(0.66f, 0.84f, 1.0f, 1.0f);
};
