// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "EquipmentBar.generated.h"

class UTextBlock;
class UBorder;

/**
 * 装备条占位。对齐美术布局图顶部"装备/及其条件"位置。
 *
 * 第一阶段 Snapshot 没有装备数据，显示 "装备：无"。
 * 后续 Run 系统接入装备后 override NativeRefreshFromSnapshot 填真实数据。
 *
 * WBP 约定（可选）：
 * - TitleText : UTextBlock
 * - FrameBorder : UBorder
 */
UCLASS(Blueprintable)
class WACOMAPP_API UEquipmentBar : public UWacomBattleWidgetBase
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeRefreshFromSnapshot(const struct FBattleSnapshot& Snap) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> FrameBorder;
};
