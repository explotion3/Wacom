// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "PlayerStatusBar.generated.h"

class UWacomProgressBar;
class UTextBlock;

UCLASS(Blueprintable, meta = (ToolTip = "Battle 玩家状态条 Widget。继承 UWacomBattleWidgetBase，只根据 Snapshot 显示玩家 HP / Shield / San，不提交玩家命令或修改 BattleSession。"))
class WACOMAPP_API UPlayerStatusBar : public UWacomBattleWidgetBase
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeRefreshFromSnapshot(const struct FBattleSnapshot& Snap) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWacomProgressBar> HpBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ShieldText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SanText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Player Status|Authoring", meta = (ToolTip = "护盾为 0 时是否隐藏 ShieldText。只影响玩家状态条显示，不改变 BattleSession 中的护盾数值。"))
	bool bHideShieldWhenZero = true;
};
