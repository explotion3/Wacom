// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/RichTextBlock.h"
#include "WacomCardDetailRichTextBlock.generated.h"

class UWacomCardDetailTheme;

/**
 * RichTextBlock wrapper for card detail body text.
 */
UCLASS(Blueprintable, meta = (ToolTip = "卡牌详情正文 RichText 控件。只显示 renderer 生成的富文本，不解析卡牌规则。"))
class WACOMAPP_API UWacomCardDetailRichTextBlock : public URichTextBlock
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|CardDetail")
	void SetCardDetailRichText(FText InText, const UWacomCardDetailTheme* InTheme);
};
