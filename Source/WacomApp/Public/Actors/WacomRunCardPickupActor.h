// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Actors/WacomRunPickupActorBase.h"
#include "WacomRunCardPickupActor.generated.h"

class AWacomPlayerController;
class UCardDefinition;

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomRunCardPickupDebugView
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Card Pickup|Debug")
	FString ActorName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Card Pickup|Debug")
	FName PersistentId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Card Pickup|Debug")
	FString CardDefinitionName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Card Pickup|Debug")
	FName CardId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Card Pickup|Debug")
	bool bHasRunSession = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Card Pickup|Debug")
	bool bCanInteract = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Card Pickup|Debug")
	bool bIsCollected = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Card Pickup|Debug")
	float TriggerRadius = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Card Pickup|Debug")
	FVector ClickBoundsExtent = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Card Pickup|Debug")
	FName VisualName = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Card Pickup|Debug")
	bool bConfigValid = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Card Pickup|Debug")
	FName ConfigWarningReason = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Card Pickup|Debug")
	bool bDuplicatePersistentIdDetected = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Card Pickup|Debug")
	bool bHasRenderableVisual = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Card Pickup|Debug")
	bool bClickTargetConfigured = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Card Pickup|Debug")
	FName ClickTargetStableId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Card Pickup|Debug")
	FString HoverPrompt;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Card Pickup|Debug")
	FString CollectedHoverPrompt;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Card Pickup|Debug")
	FName LastDebugResult = NAME_None;
};

/**
 * Run 场景固定卡牌拾取物 V1。
 *
 * 世界交互壳由 AWacomRunPickupActorBase 维护；本类只负责固定卡配置、结算和卡牌 toast。
 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomRunCardPickupActor : public AWacomRunPickupActorBase
{
	GENERATED_BODY()

public:
	AWacomRunCardPickupActor();

	/** 拾取成功后获得的固定卡牌定义。V1 只支持一张固定卡。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|Card Pickup",
		meta = (ToolTip = "拾取成功后获得的固定卡牌定义。V1 只支持一张固定卡，不支持掉落表或多卡。"))
	TObjectPtr<UCardDefinition> CardDefinition = nullptr;

	/** 将当前拾取物配置为毒牙卡牌拾取调试样例。只修改当前 Actor 配置，不修改 RunState。 */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Wacom|Run|Card Pickup|Authoring",
		meta = (ToolTip = "将当前拾取物配置为毒牙卡牌拾取调试样例：设置默认文案，并按 Actor 名生成 PersistentId；不会修改 RunState。"))
	void ConfigureDebugCardPickupSample();

	/** 读取当前卡牌拾取物配置、收集状态和点击目标绑定的只读诊断信息。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Card Pickup|Debug",
		meta = (ToolTip = "读取当前卡牌拾取物配置、收集状态和点击目标绑定的只读诊断信息；不会修改 RunState。"))
	FWacomRunCardPickupDebugView GetRunCardPickupDebugView(AWacomPlayerController* PC) const;

	/** 返回适合复制到日志或 PIE Details 面板查看的一行卡牌拾取物诊断摘要。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run|Card Pickup|Debug",
		meta = (ToolTip = "返回适合复制到日志或 PIE Details 面板查看的一行卡牌拾取物诊断摘要。"))
	FString GetRunCardPickupDebugSummary(AWacomPlayerController* PC) const;

	/** 将当前卡牌拾取物诊断摘要写入日志。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|Card Pickup|Debug",
		meta = (ToolTip = "将当前卡牌拾取物诊断摘要写入日志，便于 PIE 排查卡牌拾取配置和点击目标绑定。"))
	void LogRunCardPickupDebugSummary(AWacomPlayerController* PC) const;

protected:
	virtual FName GetRewardConfigWarningReason() const override;
	virtual bool TryCollectPickupReward(AWacomPlayerController* PC) override;
	virtual FText GetDefaultInteractPromptText() const override;
	virtual FText GetDefaultHoverPromptText() const override;
	virtual FText GetDefaultCollectedHoverPromptText() const override;
};
