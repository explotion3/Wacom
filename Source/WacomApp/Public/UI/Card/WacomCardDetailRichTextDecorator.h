// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/RichTextBlockDecorator.h"
#include "WacomCardDetailRichTextDecorator.generated.h"

/**
 * RichText decorator for card detail inline icon/status markup.
 *
 * Renderers emit Wacom-specific tags; this decorator resolves them through the
 * active card detail theme without making widgets parse card rules.
 */
UCLASS(Blueprintable, meta = (ToolTip = "卡牌详情 RichText 装饰器。负责把详情正文中的 inline icon/status 标签渲染成主题配置的图标，Widget 不解析卡牌规则。"))
class WACOMAPP_API UWacomCardDetailRichTextDecorator : public URichTextBlockDecorator
{
	GENERATED_BODY()

public:
	virtual TSharedPtr<ITextDecorator> CreateDecorator(URichTextBlock* InOwner) override;
};
