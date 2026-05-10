// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "DebugBattleHUD.generated.h"

class UCommonTextBlock;

/**
 * P1 占位 HUD。
 *
 * 只做一件事：把 FBattleSnapshot 格式化成多行文字，显示在屏幕上。
 * P2 的正式 UBattleHUD 就绪后可以删除。
 *
 * WBP 子类约定：
 * - SnapshotText : UCommonTextBlock（BindWidget）
 *   建议 Wrap Text Auto，放在屏幕一侧的 ScrollBox 或 VerticalBox 中。
 */
UCLASS(Abstract, Blueprintable)
class WACOMAPP_API UDebugBattleHUD : public UWacomBattleWidgetBase
{
	GENERATED_BODY()

protected:
	virtual void NativeRefreshFromSnapshot(const struct FBattleSnapshot& Snap) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> SnapshotText;
};
