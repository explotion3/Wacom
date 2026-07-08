// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Styling/SlateBrush.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "WacomCardDetailTheme.generated.h"

class UCommonTextStyle;
class UDataTable;

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomCardDetailIconBrushEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Card Detail Theme")
	EWacomCardDetailIcon Icon = EWacomCardDetailIcon::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Card Detail Theme")
	FSlateBrush Brush;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomCardDetailStatusBrushEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Card Detail Theme")
	FGameplayTag StatusTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Card Detail Theme")
	FSlateBrush Brush;
};

/**
 * Card detail visual theme shared by C++ fallback and WBP rich text rendering.
 */
UCLASS(BlueprintType, meta = (ToolTip = "卡牌详情视觉主题。配置标题 CommonTextStyle、正文 RichText style 和图标/状态 Brush。只影响 UI 展示。"))
class WACOMAPP_API UWacomCardDetailTheme : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Card Detail Theme", meta = (ToolTip = "详情区块标题 CommonTextStyle。WBP_CardDetailSection 的 TitleText 可使用。"))
	TSubclassOf<UCommonTextStyle> TitleTextStyle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Card Detail Theme", meta = (ToolTip = "详情正文 RichText style set。用于 BodyText 的 TextStyleSet。"))
	TObjectPtr<UDataTable> BodyTextStyleSet = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Card Detail Theme", meta = (TitleProperty = "Icon", ToolTip = "详情正文内 icon slot 使用的 Brush。缺少 Brush 时 RichText decorator 会回退到标签文本。"))
	TArray<FWacomCardDetailIconBrushEntry> IconBrushes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Card Detail Theme", meta = (TitleProperty = "StatusTag", ToolTip = "详情正文内 status slot 使用的 Brush。"))
	TArray<FWacomCardDetailStatusBrushEntry> StatusBrushes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Card Detail Theme", meta = (ToolTip = "未配置 icon/status brush 时的 fallback Brush。"))
	FSlateBrush FallbackInlineBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Card Detail Theme", meta = (ToolTip = "详情正文内联图标的渲染偏移，单位为 Slate unit。Y 为负数时图标上移；推荐从 (0, -2) 到 (0, -4) 微调，只影响显示位置，不改变文字排版占位。"))
	FVector2D InlineIconRenderOffset = FVector2D(0.0f, -2.0f);

	const FSlateBrush* ResolveIconBrush(EWacomCardDetailIcon Icon) const;
	const FSlateBrush* ResolveStatusBrush(FGameplayTag StatusTag) const;
	const FSlateBrush* ResolveFallbackInlineBrush() const;

	static bool IsInlineBrushConfigured(const FSlateBrush& Brush);
};
