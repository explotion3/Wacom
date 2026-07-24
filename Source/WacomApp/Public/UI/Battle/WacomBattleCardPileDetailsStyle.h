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
		meta = (ToolTip = "卡面缩略 Host 的制作参考宽度，单位逻辑像素；默认 178。正式 BattleHUD 会按当前未悬停手牌的卡体物理尺寸覆盖它；缺少有效手牌 Anchor 时才使用该参考值及响应式回退。正式卡面仍以 296 像素制作，并由 ScaleBox 等比缩放。"))
	float CardWidthPixels = 178.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Layout",
		meta = (ToolTip = "卡面缩略 Host 的制作参考高度，单位逻辑像素；默认 252。正式 BattleHUD 会按当前未悬停手牌卡体覆盖实际尺寸；运行时布局、命中和选框仍使用同一局部倍率。"))
	float CardHeightPixels = 252.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Responsive Layout",
		meta = (ToolTip = "没有可用 first-person hand Anchor 时，响应式缩略卡回退策略使用的参考物理视口尺寸，单位像素；默认 1920×1080。宽高分别比较并取较小倍率，因此超宽屏不会只因横向空间增加而放大。"))
	FVector2D ResponsiveReferenceViewportPixels = FVector2D(1920.0f, 1080.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Responsive Layout",
		meta = (ToolTip = "没有可用 first-person hand Anchor 时，缩略卡回退策略允许的最小物理倍率；默认 0.90。正式 BattleHUD 的牌堆卡体改为匹配未悬停手牌，不受此下限裁切。"))
	float MinimumCardPhysicalScale = 0.90f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Responsive Layout",
		meta = (ToolTip = "没有可用 first-person hand Anchor 时，缩略卡回退策略允许的最大物理倍率；默认 1.15。正式 BattleHUD 的牌堆卡体改为匹配未悬停手牌，不受此上限裁切。"))
	float MaximumCardPhysicalScale = 1.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Layout",
		meta = (ToolTip = "1080p 参考尺寸下每张卡四周的选中边框留白，单位像素；默认 4，推荐 2 到 10。运行时与缩略卡使用同一局部倍率。"))
	float CardEntryPaddingPixels = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Layout",
		meta = (ToolTip = "1080p 参考尺寸下卡牌网格列间距，单位像素；默认 12，推荐 8 到 24。运行时与缩略卡使用同一局部倍率。"))
	float CardHorizontalSpacingPixels = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Layout",
		meta = (ToolTip = "1080p 参考尺寸下卡牌网格行间距，单位像素；默认 14，推荐 8 到 28。运行时与缩略卡使用同一局部倍率。"))
	float CardVerticalSpacingPixels = 14.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Layout",
		meta = (ToolTip = "全屏牌堆页面与视口边缘的安全间距，单位像素；默认 24，推荐 16 到 48。"))
	float ScreenSafeMarginPixels = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Layout",
		meta = (ToolTip = "左侧牌堆分页导航栏宽度，单位像素；默认 96，推荐 88 到 128。影响内容区可用宽度。"))
	float NavigationRailWidthPixels = 96.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Selection",
		meta = (ToolTip = "临时悬浮或焦点状态写入流光材质的强度；推荐 0.5 到 1.0，不影响布局。"))
	float HoverOutlineAmount = 0.72f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Selection",
		meta = (ToolTip = "点击锁定选择状态写入流光材质的强度；推荐 0.7 到 1.2，不影响布局。"))
	float LockedOutlineAmount = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Pile Details|Selection",
		meta = (ToolTip = "1080p 参考尺寸下流光外框伸出卡体四周的距离，单位像素；默认 4，推荐 2 到 8。运行时与缩略卡使用同一局部倍率。"))
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
