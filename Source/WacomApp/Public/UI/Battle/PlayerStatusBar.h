// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "PlayerStatusBar.generated.h"

class UWacomProgressBar;
class UWacomBattleStatusIconListWidget;
class UTextBlock;
class UWidgetAnimation;
class USoundBase;

UCLASS(Blueprintable, meta = (ToolTip = "Battle 玩家状态条 Widget。继承 UWacomBattleWidgetBase，只根据 Snapshot 显示玩家 HP、Shield 和 runtime 状态图标，不提交玩家命令或修改 BattleSession。"))
class WACOMAPP_API UPlayerStatusBar : public UWacomBattleWidgetBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Player Status", meta = (ToolTip = "应用战斗行动预览。只覆盖本控件显示，不修改 BattleSession；ClearActionPreview 后恢复最近一次 Snapshot。"))
	void SetActionPreview(const FPlayerSnapshot& ProjectedPlayer);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Player Status", meta = (ToolTip = "清除战斗行动预览，恢复最近一次 Snapshot 显示。"))
	void ClearActionPreview();

	/** 只播放一次敌方行动命中反馈；不修改 BattleSession 或 Snapshot 真相。 */
	void PlayEnemyActionImpactFeedback(
		const FPlayerSnapshot& PreviousPlayer,
		const FPlayerSnapshot& CurrentPlayer);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeRefreshFromSnapshot(const struct FBattleSnapshot& Snap) override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWacomProgressBar> HpBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ShieldText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomBattleStatusIconListWidget> StatusList;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> DamagePulseAnimation;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> ShieldPulseAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Player Status|Authoring", meta = (ToolTip = "护盾为 0 时是否隐藏 ShieldText。只影响玩家状态条显示，不改变 BattleSession 中的护盾数值。"))
	bool bHideShieldWhenZero = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Player Status|Preview", meta = (ToolTip = "Action Preview 激活时整个玩家状态条的渲染透明度。单位：0-1；推荐 0.7-1.0，仅提示这是预览态，不影响布局或规则。"))
	float ActionPreviewRenderOpacity = 0.82f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Player Status|Impact",
		meta = (ToolTip = "玩家 HP 因敌方行动下降时播放的可选 2D 音效。为空时保持静音，不影响状态条脉冲。"))
	TObjectPtr<USoundBase> DamageImpactSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Player Status|Impact",
		meta = (ToolTip = "敌方行动只削减玩家护盾时播放的可选 2D 音效。为空时保持静音；若同次命中也扣 HP，则优先使用 DamageImpactSound。"))
	TObjectPtr<USoundBase> ShieldImpactSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Player Status|Impact",
		meta = (ToolTip = "敌方行动命中音效音量倍率。单位：线性倍率；推荐 0.6–1.2，不影响布局或规则。"))
	float ImpactSoundVolume = 1.0f;

private:
	void RefreshDisplay();
	void RefreshFromPlayerSnapshot(const FPlayerSnapshot& PlayerView);
	UWacomBattleStatusIconListWidget* ResolveStatusListWidget();

	UPROPERTY(Transient)
	FPlayerSnapshot BasePlayerView;

	UPROPERTY(Transient)
	FPlayerSnapshot ActionPreviewPlayerView;

	float BaseRenderOpacity = 1.0f;
	bool bCapturedBaseRenderOpacity = false;
	bool bHasBasePlayerView = false;
	bool bHasActionPreview = false;
};
