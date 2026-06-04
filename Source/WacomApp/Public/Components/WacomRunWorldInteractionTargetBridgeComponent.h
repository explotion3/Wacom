// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "WacomRunWorldInteractionTargetBridgeComponent.generated.h"

class UPrimitiveComponent;
class UWacomInteractionTargetComponent;

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomRunWorldInteractionTargetDebugView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Target|Debug", meta = (ToolTip = "当前 Run World Target 的稳定 ID，只用于 PIE / 蓝图诊断，不参与规则结算。"))
	FName RunTargetStableId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Target|Debug", meta = (ToolTip = "当前 Run World Target 的运行时 ID，只用于验证通用交互 handle。"))
	FGuid RuntimeTargetId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Target|Debug", meta = (ToolTip = "当前写入通用交互目标的 GameplayTag，用于排查 target 类型。"))
	FGameplayTag TargetTag;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Target|Debug", meta = (ToolTip = "Owner 上是否能解析到 WacomInteractionTargetComponent。"))
	bool bHasInteractionTargetComponent = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Target|Debug", meta = (ToolTip = "通用交互目标组件是否已由本 Bridge 成功配置。"))
	bool bInteractionTargetConfigured = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Target|Debug", meta = (ToolTip = "当前是否能解析到 probe 预览使用的目标组件。"))
	bool bHasVisualTarget = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Target|Debug", meta = (ToolTip = "当前预览目标是否是可渲染 Primitive，用于排查 CustomDepth / 缩放反馈。"))
	bool bHasRenderableVisualTarget = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Target|Debug", meta = (ToolTip = "当前预览目标组件名称，用于在 PIE 中核对绑定。"))
	FName VisualTargetName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Target|Debug", meta = (ToolTip = "鼠标 probe 预览是否处于激活状态。"))
	bool bProbePreviewActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Target|Debug", meta = (ToolTip = "probe 预览缩放反馈是否开启。"))
	bool bProbeScaleSignalEnabled = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Target|Debug", meta = (ToolTip = "probe 预览 CustomDepth / Stencil 反馈是否开启。"))
	bool bProbeCustomDepthSignalEnabled = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Target|Debug", meta = (ToolTip = "probe 预览写入 CustomDepth 的 stencil 值。"))
	int32 ProbeCustomDepthStencilValue = 250;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Target|Debug", meta = (ToolTip = "最近一次配置通用交互目标的结果。"))
	FName LastConfigureResult = TEXT("NotAttempted");

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|World Target|Debug", meta = (ToolTip = "最近一次 probe 预览状态变化的结果。"))
	FName LastPreviewResult = TEXT("NotAttempted");
};

/**
 * Run 场景 Actor 到通用 World interaction target 的桥接组件。
 *
 * 本组件只负责 Run / 探索目标的身份写入、鼠标 probe 预览和调试信息；
 * 通用命中 handle 仍由同 Actor 上的 UWacomInteractionTargetComponent 提供。
 */
UCLASS(ClassGroup = (Wacom), meta = (BlueprintSpawnableComponent, ToolTip = "把 Run / 探索场景 Actor 标记为可被鼠标 probe 或未来卡牌拖拽识别的 World Target。"))
class WACOMAPP_API UWacomRunWorldInteractionTargetBridgeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWacomRunWorldInteractionTargetBridgeComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|World Target", meta = (ToolTip = "Run 场景目标的稳定 ID。建议填写关卡中唯一的 PersistentId，用于后续 resolver / 存档语义识别。"))
	FName RunTargetStableId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|World Target", meta = (ToolTip = "用于播放鼠标 probe 预览的 Primitive；为空时自动使用 Owner 上第一个 PrimitiveComponent。"))
	TObjectPtr<UPrimitiveComponent> VisualTargetComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|World Target", meta = (ToolTip = "是否自动把同 Actor 上的 WacomInteractionTargetComponent 标记为 Run Object target。"))
	bool bAutoConfigureInteractionTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|World Target", meta = (ToolTip = "如果通用 target 组件还没有有效运行时 ID，是否自动生成一个临时运行时 ID。"))
	bool bAutoGenerateRuntimeTargetId = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|World Target", meta = (ToolTip = "鼠标 probe 指向该目标时的轻量预览缩放倍率。", ClampMin = "1.0", ClampMax = "1.5", UIMin = "1.0", UIMax = "1.2"))
	float ProbePreviewScale = 1.06f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|World Target", meta = (ToolTip = "鼠标 probe 指向该目标时，是否启用轻量缩放视觉信号。"))
	bool bEnableProbeScaleSignal = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|World Target", meta = (ToolTip = "鼠标 probe 指向该目标时，是否启用 CustomDepth / Stencil 视觉信号，供后续描边材质使用。"))
	bool bEnableProbeCustomDepthSignal = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|World Target", meta = (ToolTip = "鼠标 probe 视觉信号写入 CustomDepth 的 stencil 值。需要项目描边材质读取时才会产生可见描边。", ClampMin = "0", ClampMax = "255", UIMin = "0", UIMax = "255"))
	int32 ProbeCustomDepthStencilValue = 250;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|World Target|Debug", meta = (ToolTip = "开启后，目标配置和 probe 预览状态变化会输出到日志。默认关闭。"))
	bool bLogRunWorldTargetDebug = false;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|World Target")
	void SetRunTargetStableId(FName InStableId);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|World Target")
	bool RefreshRunWorldTargetBinding();

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|World Target")
	void SetProbePreviewActive(bool bActive);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|World Target")
	void ClearProbePreview();

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|World Target")
	FGuid GetRuntimeTargetId() const { return RuntimeTargetId; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|World Target")
	bool IsProbePreviewActive() const { return bProbePreviewActive; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|World Target|Debug", meta = (ToolTip = "获取当前 Run World Target 的只读调试快照；只用于 PIE / 蓝图排查，不影响规则。"))
	FWacomRunWorldInteractionTargetDebugView GetRunWorldTargetDebugView() const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|World Target|Debug", meta = (ToolTip = "获取当前 Run World Target 的单行调试摘要；只用于复制日志或自动化排查。"))
	FString GetRunWorldTargetDebugSummary() const;

	UFUNCTION(CallInEditor, Category = "Wacom|Run|World Target|Debug", meta = (ToolTip = "在编辑器或 PIE 中把当前 Run World Target 调试摘要写入 Output Log；不改变运行时状态。"))
	void LogRunWorldTargetDebugSummary() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UWacomInteractionTargetComponent* ResolveInteractionTargetComponent() const;
	UPrimitiveComponent* ResolveVisualTargetComponent() const;
	UPrimitiveComponent* ResolveRenderableOwnerPrimitive() const;
	bool IsRenderableProbeVisualTarget(const UPrimitiveComponent* Primitive) const;
	void BeginProbeVisualFeedback(UPrimitiveComponent* Primitive);
	void RestoreProbeVisualFeedbackIfNeeded();
	void LogDebugState(const TCHAR* Prefix) const;

	UPROPERTY(Transient)
	FGuid RuntimeTargetId;

	UPROPERTY(Transient)
	FVector CachedBaseScale = FVector::OneVector;

	UPROPERTY(Transient)
	TWeakObjectPtr<UPrimitiveComponent> CachedVisualTarget;

	bool bInteractionTargetConfigured = false;
	bool bHasCachedBaseScale = false;
	bool bHasCachedCustomDepth = false;
	bool bCachedRenderCustomDepth = false;
	int32 CachedCustomDepthStencilValue = 0;
	bool bCachedVisualWasRenderable = false;
	bool bProbePreviewActive = false;
	FName LastConfigureResult = TEXT("NotAttempted");
	FName LastPreviewResult = TEXT("NotAttempted");
};
