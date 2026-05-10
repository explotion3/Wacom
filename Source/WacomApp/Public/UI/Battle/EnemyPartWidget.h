// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "Snapshots/EnemySnapshot.h"
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
 * - StatusText      : UCommonTextBlock   BUFF/DEBUFF 简要（第一阶段占位）
 *
 * 蓝图钩子：
 * - BP_OnDataApplied(FEnemyPartSnapshot)
 * - BP_OnTargetableChanged(bool)       HUD 是否让此部位可被选中
 * - BP_OnDestroyedChanged(bool)        部位破坏状态变化
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWacomEnemyPartClicked, FGuid, PartInstanceId);

UCLASS(Blueprintable)
class WACOMAPP_API UEnemyPartWidget : public UWacomBattleWidgetBase
{
	GENERATED_BODY()

public:
	/** 外部注入部位数据。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|UI")
	void ApplyPartSnapshot(const FEnemyPartSnapshot& InSnap);

	/** HUD 决定此部位当前是否可被点击选为目标（Idle 状态不可点，TargetSelect 状态下存活部位可点）。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|UI")
	void SetTargetable(bool bInTargetable);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|UI")
	FGuid GetPartInstanceId() const { return CachedSnap.InstanceId; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|UI")
	const FEnemyPartSnapshot& GetPartSnapshot() const { return CachedSnap; }

	UPROPERTY(BlueprintAssignable, Category = "Wacom|Battle|UI")
	FWacomEnemyPartClicked OnPartClicked;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Battle|UI", DisplayName = "On Data Applied")
	void BP_OnDataApplied(const FEnemyPartSnapshot& Snap);

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Battle|UI", DisplayName = "On Targetable Changed")
	void BP_OnTargetableChanged(bool bTargetable);

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Battle|UI", DisplayName = "On Destroyed Changed")
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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|UI")
	bool bHideShieldWhenZero = true;

private:
	UFUNCTION()
	void HandleRootButtonClicked();

	void UpdateFrameColor();

	FEnemyPartSnapshot CachedSnap;
	bool bLastDestroyed = false;
	bool bLastTargetable = false;
};
