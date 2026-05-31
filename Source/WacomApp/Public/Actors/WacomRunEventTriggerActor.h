// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/WacomWorldInteractable.h"
#include "WacomRunEventTriggerActor.generated.h"

class USphereComponent;
class UWacomRunEventDefinition;

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomRunEventTriggerDebugView
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|RunEvent|Debug")
	FString ActorName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|RunEvent|Debug")
	FName PersistentId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|RunEvent|Debug")
	FString EventDefinitionName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|RunEvent|Debug")
	FName EventId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|RunEvent|Debug")
	FName StartNodeId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|RunEvent|Debug")
	bool bHasRunSession = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|RunEvent|Debug")
	bool bCanInteract = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|RunEvent|Debug")
	bool bIsActiveEvent = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|RunEvent|Debug")
	bool bIsCompleted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|RunEvent|Debug")
	FName CurrentNodeId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|RunEvent|Debug")
	bool bDuplicatePersistentIdDetected = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|RunEvent|Debug")
	FName LastDebugResult = NAME_None;
};

/** 场景中的轻量 Run 事件交互触发器。 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomRunEventTriggerActor : public AActor, public IWacomWorldInteractable
{
	GENERATED_BODY()

public:
	AWacomRunEventTriggerActor();

	/** 事件节点在当前 Run 内的唯一 ID。RunSession 使用它保存完成状态和当前节点。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|RunEvent",
		meta = (ToolTip = "事件节点在当前 Run 内的唯一 ID。RunSession 使用它保存完成状态和当前节点；None 会拒绝打开事件。"))
	FName PersistentId;

	/** 静态事件图定义。多个场景 Actor 可以引用同一事件定义，但运行态由各自 PersistentId 区分。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|RunEvent",
		meta = (ToolTip = "静态事件图定义。多个场景 Actor 可以引用同一事件定义，但运行态由各自 PersistentId 区分。"))
	TObjectPtr<UWacomRunEventDefinition> EventDefinition = nullptr;

	/** 触发半径（cm）。玩家进入该范围后，探索期按 E 可以打开事件。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|RunEvent",
		meta = (ToolTip = "玩家进入该半径后，探索期按 E 可以打开事件。单位：厘米。建议范围 50-1000。",
			ClampMin = "50.0", UIMin = "50.0", UIMax = "1000.0"))
	float TriggerRadius = 200.f;

	/** 探索 HUD 上显示的交互提示文本。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|RunEvent",
		meta = (ToolTip = "玩家处于事件交互范围内时显示在探索 HUD 上的提示文本。"))
	FText InteractPromptText;

	/** 事件已完成时探索 HUD 上显示的弱提示文本。按 E 不会重新打开事件，只会提示已完成。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|RunEvent",
		meta = (ToolTip = "事件已完成时，玩家处于交互范围内显示的弱提示文本。按 E 不会重新打开事件，只会显示已完成提示。"))
	FText CompletedPromptText;

	/** 事件已完成时按 E 弹出的 Toast 文本。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|RunEvent",
		meta = (ToolTip = "事件已完成时玩家按 E 弹出的 Toast 文本，用于说明该事件不是交互失效，而是已经完成。"))
	FText CompletedToastText;

	UFUNCTION(BlueprintPure, Category = "Wacom|RunEvent")
	USphereComponent* GetTriggerSphere() const { return TriggerSphere; }

	/** 将当前触发器配置为蛇巢卡牌支付调试事件样例。只修改当前 Actor 配置，不打开事件。 */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Wacom|RunEvent|Authoring",
		meta = (ToolTip = "将当前触发器配置为 DA_Event_DebugSnakeGift 样例。只修改当前 Actor 配置，不打开事件、不修改 RunState。"))
	void ConfigureDebugSnakeGiftSample();

	/** 将当前触发器配置为 RunFlag + 金币门槛奖励调试事件样例。只修改当前 Actor 配置，不打开事件。 */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Wacom|RunEvent|Authoring",
		meta = (ToolTip = "将当前触发器配置为 DA_Event_DebugFlagReward 样例。只修改当前 Actor 配置，不打开事件、不修改 RunState。"))
	void ConfigureDebugFlagRewardSample();

	/** 读取当前触发器和对应 RunSession 中事件状态的只读诊断信息。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|RunEvent|Debug",
		meta = (ToolTip = "读取当前触发器和对应 RunSession 中事件状态的只读诊断信息；不会修改 RunState。"))
	FWacomRunEventTriggerDebugView GetRunEventTriggerDebugView(AWacomPlayerController* PC) const;

	/** 返回适合复制到日志或 PIE Details 面板查看的一行诊断摘要。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|RunEvent|Debug",
		meta = (ToolTip = "返回适合复制到日志或 PIE Details 面板查看的一行诊断摘要。"))
	FString GetRunEventTriggerDebugSummary(AWacomPlayerController* PC) const;

	/** 将当前触发器诊断摘要写入日志。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|RunEvent|Debug",
		meta = (ToolTip = "将当前触发器诊断摘要写入日志，便于 PIE 排查事件配置和运行态。"))
	void LogRunEventTriggerDebugSummary(AWacomPlayerController* PC) const;

	// ---- IWacomWorldInteractable ----
	virtual FText GetInteractPromptText_Implementation(AWacomPlayerController* PC) const override;
	virtual FVector GetInteractLocation_Implementation(AWacomPlayerController* PC) const override;
	virtual bool CanInteract_Implementation(AWacomPlayerController* PC) const override;
	virtual bool TryInteract_Implementation(AWacomPlayerController* PC) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

private:
	bool ConfigureDebugSample(FName InPersistentId, const TCHAR* EventDefinitionObjectPath);
	bool HasDuplicatePersistentIdInWorld() const;
	bool IsEventCompletedFor(AWacomPlayerController* PC) const;
	void ShowCompletedToast(AWacomPlayerController* PC) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|RunEvent",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> TriggerSphere = nullptr;
};
