// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Actors/WacomRunPickupActorBase.h"
#include "WacomRunRewardPickupActor.generated.h"

class AWacomPlayerController;
class UCardDefinition;
class UWacomRunPickupDefinition;

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomRunRewardPickupDebugView
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Reward Pickup|Debug")
	FString ActorName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Reward Pickup|Debug")
	FName PersistentId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Reward Pickup|Debug")
	FString PickupDefinitionName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Reward Pickup|Debug")
	FName PickupId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Reward Pickup|Debug")
	FName RewardType = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Reward Pickup|Debug")
	int32 GoldAmount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Reward Pickup|Debug")
	FString CardDefinitionName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Reward Pickup|Debug")
	FName CardId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Reward Pickup|Debug")
	bool bHasRunSession = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Reward Pickup|Debug")
	bool bCanInteract = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Reward Pickup|Debug")
	bool bIsCollected = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Reward Pickup|Debug")
	float TriggerRadius = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Reward Pickup|Debug")
	FVector ClickBoundsExtent = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Reward Pickup|Debug")
	FName VisualName = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Reward Pickup|Debug")
	bool bConfigValid = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Reward Pickup|Debug")
	FName ConfigWarningReason = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Reward Pickup|Debug")
	bool bDuplicatePersistentIdDetected = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Reward Pickup|Debug")
	bool bHasRenderableVisual = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Reward Pickup|Debug")
	bool bClickTargetConfigured = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Reward Pickup|Debug")
	FName ClickTargetStableId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Reward Pickup|Debug")
	FString HoverPrompt;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Reward Pickup|Debug")
	FString CollectedHoverPrompt;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Reward Pickup|Debug")
	FName LastDebugResult = NAME_None;
};

/**
 * Run 场景数据驱动拾取物 V1。
 *
 * 世界交互壳由 AWacomRunPickupActorBase 维护；本类只负责读取 PickupDefinition 并调用现有 RunSession 拾取结算入口。
 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomRunRewardPickupActor : public AWacomRunPickupActorBase
{
	GENERATED_BODY()

public:
	AWacomRunRewardPickupActor();

	/** 静态拾取奖励定义。PersistentId 仍来自场景 Actor，用于当前 Run 防重复拾取。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Pickup|Definition",
		meta = (ToolTip = "静态拾取奖励定义。PersistentId 仍来自场景 Actor，用于当前 Run 防重复拾取；Definition.PickupId 只用于内容识别和 debug。"))
	TObjectPtr<UWacomRunPickupDefinition> PickupDefinition = nullptr;

	/** 读取当前数据驱动拾取物配置、收集状态和点击目标绑定的只读诊断信息。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Reward Pickup|Debug",
		meta = (ToolTip = "读取当前数据驱动拾取物配置、收集状态和点击目标绑定的只读诊断信息；不会修改 RunState。"))
	FWacomRunRewardPickupDebugView GetRunRewardPickupDebugView(AWacomPlayerController* PC) const;

	/** 返回适合复制到日志或 PIE Details 面板查看的一行数据驱动拾取物诊断摘要。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Reward Pickup|Debug",
		meta = (ToolTip = "返回适合复制到日志或 PIE Details 面板查看的一行数据驱动拾取物诊断摘要。"))
	FString GetRunRewardPickupDebugSummary(AWacomPlayerController* PC) const;

	/** 将当前数据驱动拾取物诊断摘要写入日志。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Reward Pickup|Debug",
		meta = (ToolTip = "将当前数据驱动拾取物诊断摘要写入日志，便于 PIE 排查 Definition、点击目标和已拾取状态。"))
	void LogRunRewardPickupDebugSummary(AWacomPlayerController* PC) const;

	/** Details 面板调试按钮：配置为 3 金币 PickupDefinition 样例，只修改当前 Actor 配置，不修改 RunState。 */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Wacom|Run|Reward Pickup|Authoring",
		meta = (ToolTip = "配置为 3 金币 PickupDefinition 调试样例。只修改当前 Actor 的 PersistentId、PickupDefinition、默认提示、生命周期和点击 stable id；不会修改 RunState。"))
	void ConfigureDebugGoldDefinitionPickupSample();

	/** Details 面板调试按钮：配置为毒牙卡牌 PickupDefinition 样例，只修改当前 Actor 配置，不修改 RunState。 */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Wacom|Run|Reward Pickup|Authoring",
		meta = (ToolTip = "配置为毒牙卡牌 PickupDefinition 调试样例。只修改当前 Actor 的 PersistentId、PickupDefinition、默认提示、生命周期和点击 stable id；不会修改 RunState。"))
	void ConfigureDebugPoisonFangDefinitionPickupSample();

protected:
	virtual FName GetRewardConfigWarningReason() const override;
	virtual bool TryCollectPickupReward(AWacomPlayerController* PC) override;
	virtual FText GetDefaultInteractPromptText() const override;
	virtual FText GetDefaultHoverPromptText() const override;
	virtual FText GetDefaultCollectedHoverPromptText() const override;
};
