// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Events/BattleEvent.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "Snapshots/EnemySnapshot.h"
#include "TimerManager.h"
#include "EnemyPartWidget.generated.h"

/**
 * 单个敌方部位的 UI。显示 HP / Initiative / Intent / Status 简要信息。
 *
 * 职责：
 * - 展示 FEnemyPartSnapshot 内容
 * - 在 TargetSelect 状态下作为"可选目标"响应点击
 * - 破坏 / 晕厥等状态触发视觉变化（蓝图钩子）
 *
 * 不持有 Session。点击通过委托上报给 UEnemyInfoBar → UBattleHUD。
 *
 * WBP 子类约定（BindWidget）：
 * - RootButton : UWacomButtonBase
 * - HpBar      : UWacomProgressBar
 *
 * 可选（BindWidgetOptional）：
 * - NameText        : UCommonTextBlock   部位名
 * - InitiativeText  : UCommonTextBlock   当前先机
 * - IntentText      : UCommonTextBlock   意图展示（"Bite(6)" 之类）
 * - ShieldText      : UCommonTextBlock   Shield 数值（0 时折叠）
 * - StatusText      : UCommonTextBlock   BUFF/DEBUFF 简要
 *
 * 蓝图钩子：
 * - BP_OnDataApplied(FEnemyPartSnapshot)
 * - BP_OnTargetableChanged(bool)       HUD 是否让此部位可被选中
 * - BP_OnDestroyedChanged(bool)        部位破坏状态变化
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWacomEnemyPartClicked, FGuid, PartInstanceId);

UCLASS(Blueprintable, meta = (ToolTip = "Legacy 2D 敌方部位 Widget。缺少 SceneEnemyHost / PartActor 时作为 fallback/debug 显示和目标点击入口，不是 HD-2D 场景敌人的正式制作入口。"))
class WACOMAPP_API UEnemyPartWidget : public UWacomBattleWidgetBase
{
	GENERATED_BODY()

public:
	/** 外部注入部位数据。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Enemy 2D Fallback|Compatibility", meta = (ToolTip = "把敌方部位快照应用到 legacy 2D fallback Widget。只刷新 UI，不修改 BattleSession。"))
	void ApplyPartSnapshot(const FEnemyPartSnapshot& InSnap);

	/** HUD 决定此部位当前是否可被点击选为目标（Idle 状态不可点，TargetSelect 状态下存活部位可点）。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Enemy 2D Fallback|Compatibility", meta = (ToolTip = "设置 legacy 2D 敌方部位 fallback 当前是否可作为目标点击。点击仍通过 HUD 上报玩家意图，不直接提交规则命令。"))
	void SetTargetable(bool bInTargetable);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Enemy 2D Fallback|Compatibility", meta = (ToolTip = "当前 legacy 2D 敌方部位 fallback 对应的运行时部位实例 ID。"))
	FGuid GetPartInstanceId() const { return CachedSnap.InstanceId; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Enemy 2D Fallback|Compatibility", meta = (ToolTip = "当前 legacy 2D 敌方部位 fallback 缓存的部位快照。只用于显示或调试读取。"))
	const FEnemyPartSnapshot& GetPartSnapshot() const { return CachedSnap; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Enemy 2D Fallback|Compatibility", meta = (ToolTip = "当前 legacy 2D 敌方部位 fallback 是否可被选为目标。"))
	bool IsTargetable() const { return bLastTargetable; }

	UPROPERTY(BlueprintAssignable, Category = "Wacom|Battle|Enemy 2D Fallback|Compatibility", meta = (ToolTip = "legacy 2D 敌方部位被点击时广播给 EnemyInfoBar / BattleHUD 的事件。监听方负责提交玩家意图。"))
	FWacomEnemyPartClicked OnPartClicked;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Battle|Enemy 2D Fallback|Compatibility", DisplayName = "On Data Applied", meta = (ToolTip = "legacy 2D 敌方部位 fallback 快照已应用后的 WBP 表现事件。只用于更新样式，不应修改 BattleSession。"))
	void BP_OnDataApplied(const FEnemyPartSnapshot& Snap);

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Battle|Enemy 2D Fallback|Compatibility", DisplayName = "On Targetable Changed", meta = (ToolTip = "legacy 2D 敌方部位 fallback 可选目标状态变化后的 WBP 表现事件。只用于更新样式。"))
	void BP_OnTargetableChanged(bool bTargetable);

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Battle|Enemy 2D Fallback|Compatibility", DisplayName = "On Destroyed Changed", meta = (ToolTip = "legacy 2D 敌方部位 fallback 破坏状态变化后的 WBP 表现事件。只用于更新样式。"))
	void BP_OnDestroyedChanged(bool bDestroyed);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> RootButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UWacomProgressBar> HpBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock> NameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock> InitiativeText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock> IntentText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock> ShieldText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock> StatusText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UBorder> FrameBorder;

	/** Shield == 0 时隐藏 ShieldText。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy 2D Fallback|Compatibility", meta = (ToolTip = "legacy 2D 敌方部位 fallback 中护盾为 0 时是否隐藏 ShieldText。只影响旧 2D fallback 显示。"))
	bool bHideShieldWhenZero = true;

private:
	UFUNCTION()
	void HandleRootButtonClicked();

	void PlayBattlePresentationCue(const FWacomBattlePresentationTargetCue& Cue);
	void ClearBattlePresentationCue();
	void StopBattlePresentationCueTimer();
	FLinearColor BuildBaseFrameColor() const;
	FLinearColor BuildPresentationCueFrameColor(const FWacomBattlePresentationTargetCue& Cue) const;
	void UpdateFrameColor();

	FEnemyPartSnapshot CachedSnap;
	bool bLastDestroyed = false;
	bool bLastTargetable = false;
	bool bBattlePresentationCueActive = false;
	EWacomBattlePresentationTargetCueKind LastBattlePresentationCueKind =
		EWacomBattlePresentationTargetCueKind::BattleEvent;
	EBattleEventType LastBattlePresentationCueType = EBattleEventType::None;
	int32 LastBattlePresentationCueAmount = 0;
	int32 BattlePresentationCuePlayCount = 0;
	FTimerHandle BattlePresentationCueTimerHandle;

	friend class UEnemyInfoBar;
	friend class UWacomBattleEnemyPartWidgetPresentationProbe;
};
