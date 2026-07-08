// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/RichTextBlockDecorator.h"
#include "WacomCardDetailRichTextDecorator.generated.h"

/**
 * Extension point for future inline image / tooltip rich text runs.
 *
 * v1 renderers emit styled text fallback. This decorator reserves Wacom-specific
 * tag names without making data widgets parse card rules.
 */
UCLASS(Blueprintable, meta = (ToolTip = "卡牌详情 RichText 扩展点。后续用于 inline icon/status/keyword tooltip；v1 主要保留合同。"))
class WACOMAPP_API UWacomCardDetailRichTextDecorator : public URichTextBlockDecorator
{
	GENERATED_BODY()

public:
	virtual TSharedPtr<ITextDecorator> CreateDecorator(URichTextBlock* InOwner) override;
};
