// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Actors/WacomRunPickupActorBase.h"
#include "WacomRunPickupActor.generated.h"

class AWacomPlayerController;

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomRunPickupDebugView
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	FString ActorName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	FName PersistentId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	int32 GoldAmount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	bool bHasRunSession = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	bool bCanInteract = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	bool bIsCollected = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	float TriggerRadius = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	FVector ClickBoundsExtent = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	FName VisualName = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	bool bConfigValid = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	FName ConfigWarningReason = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	bool bDuplicatePersistentIdDetected = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	bool bHasRenderableVisual = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	bool bClickTargetConfigured = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	FName ClickTargetStableId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	FString HoverPrompt;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	FString CollectedHoverPrompt;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Pickup|Debug")
	FName LastDebugResult = NAME_None;
};

/**
 * Run 场景金币拾取物 V1。
 *
 * 世界交互壳由 AWacomRunPickupActorBase 维护；本类只负责金币配置、结算和金币 toast。
 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomRunPickupActor : public AWacomRunPickupActorBase
{
	GENERATED_BODY()

public:
	AWacomRunPickupActor();

	/** 拾取成功后获得的金币数量。V1 只支持正数金币拾取。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Pickup|Gold",
		meta = (ToolTip = "拾取成功后获得的金币数量。V1 只支持正数金币拾取。单位：金币。",
			ClampMin = "1", UIMin = "1"))
	int32 GoldAmount = 1;

	/** 将当前拾取物配置为金币拾取调试样例。只修改当前 Actor 配置，不修改 RunState。 */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Wacom|Run|Pickup|Authoring",
		meta = (ToolTip = "将当前拾取物配置为金币拾取调试样例：设置默认文案、GoldAmount=3，并按 Actor 名生成 PersistentId；不会修改 RunState。"))
	void ConfigureDebugGoldPickupSample();

	/** 读取当前金币拾取物配置、收集状态和点击目标绑定的只读诊断信息。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Pickup|Debug",
		meta = (ToolTip = "读取当前金币拾取物配置、收集状态和点击目标绑定的只读诊断信息；不会修改 RunState。"))
	FWacomRunPickupDebugView GetRunPickupDebugView(AWacomPlayerController* PC) const;

	/** 返回适合复制到日志或 PIE Details 面板查看的一行金币拾取物诊断摘要。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Pickup|Debug",
		meta = (ToolTip = "返回适合复制到日志或 PIE Details 面板查看的一行金币拾取物诊断摘要。"))
	FString GetRunPickupDebugSummary(AWacomPlayerController* PC) const;

	/** 将当前金币拾取物诊断摘要写入日志。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Pickup|Debug",
		meta = (ToolTip = "将当前金币拾取物诊断摘要写入日志，便于 PIE 排查金币拾取配置和点击目标绑定。"))
	void LogRunPickupDebugSummary(AWacomPlayerController* PC) const;

protected:
	virtual FName GetRewardConfigWarningReason() const override;
	virtual bool TryCollectPickupReward(AWacomPlayerController* PC) override;
	virtual FText GetDefaultInteractPromptText() const override;
	virtual FText GetDefaultHoverPromptText() const override;
	virtual FText GetDefaultCollectedHoverPromptText() const override;
};
