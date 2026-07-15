// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WacomRunPathBranchTargetActor.generated.h"

class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;
class UMaterialInterface;
struct FWacomLocalSettingsSnapshot;
enum class EWacomRuntimeSettingsChangeReason : uint8;

UENUM(BlueprintType)
enum class EWacomRunPathBranchPresentationState : uint8
{
	Hidden,
	Available,
	Focused
};

DECLARE_MULTICAST_DELEGATE_OneParam(FWacomRunPathBranchRequestedNative, FName);

/** 分支点击目标只上报 EdgeId；不保存目标 Segment、Node 或成本。 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomRunPathBranchTargetActor : public AActor
{
	GENERATED_BODY()

public:
	AWacomRunPathBranchTargetActor();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Run Path",
		meta = (ToolTip = "点击时上报的当前 Floor EdgeId。Coordinator 会从规则 Snapshot 和场景 Registry 解析路径与目标，Actor 不保存规则连接。"))
	FName EdgeId = NAME_None;

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Path",
		meta = (ToolTip = "返回用于鼠标命中和分支提示定位的碰撞盒；不表示分支规则是否合法。"))
	UBoxComponent* GetClickBounds() const { return ClickBounds; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Run Path",
		meta = (ToolTip = "返回当前道路入口的只读表现状态；合法性仍由 Run Snapshot 决定。"))
	EWacomRunPathBranchPresentationState GetPresentationState() const
	{
		return PresentationState;
	}

	/** App-private selection controller 的唯一写入口。Blueprint 只能响应状态变化。 */
	void SetPresentationState(EWacomRunPathBranchPresentationState NewState);

	/** 多出口节点按 W 时播放一次非语义注意力提示；关闭闪光时保持稳定颜色。 */
	void PlayAttentionPulse();

	/** 返回 false 表示 EdgeId 无效；成功只广播意图。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Run Path",
		meta = (ToolTip = "广播当前 EdgeId 的分支选择意图。返回 false 表示 EdgeId 无效；规则合法性由 Coordinator 和 RunSession 判断。"))
	bool RequestBranch() const;

	FWacomRunPathBranchRequestedNative& OnBranchRequestedNative() { return BranchRequestedNative; }

private:
	void ApplyPresentationState();
	void HandleRuntimeSettingsChanged(
		const FWacomLocalSettingsSnapshot& Snapshot,
		EWacomRuntimeSettingsChangeReason Reason);
	void ClearAttentionPulse();
	void ApplyEntranceMaterial();

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Run Path",
		meta = (DisplayName = "On Branch Presentation State Changed",
			ToolTip = "道路入口的 Hidden、Available 或 Focused 状态发生变化。事件只负责视觉，不得提交 Run 规则。"))
	void BP_OnBranchPresentationStateChanged(EWacomRunPathBranchPresentationState NewState);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run Path",
		meta = (AllowPrivateAccess = "true", ToolTip = "道路入口根组件。"))
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run Path",
		meta = (AllowPrivateAccess = "true", ToolTip = "分支目标的鼠标命中范围，单位厘米；只影响场景交互区域，不改变地图连接。"))
	TObjectPtr<UBoxComponent> ClickBounds = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run Path",
		meta = (AllowPrivateAccess = "true", ToolTip = "道路入口视觉根组件；Hidden 状态会整体隐藏。"))
	TObjectPtr<USceneComponent> VisualRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run Path",
		meta = (AllowPrivateAccess = "true", ToolTip = "道路入口左侧像素立柱。"))
	TObjectPtr<UStaticMeshComponent> LeftPost = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run Path",
		meta = (AllowPrivateAccess = "true", ToolTip = "道路入口右侧像素立柱。"))
	TObjectPtr<UStaticMeshComponent> RightPost = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run Path",
		meta = (AllowPrivateAccess = "true", ToolTip = "沿 Edge 初始方向延伸的地面光带。"))
	TObjectPtr<UStaticMeshComponent> GroundGuide = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Run Path|Presentation",
		meta = (ToolTip = "可选道路的稳定语义颜色。推荐青色；即使关闭装饰性闪光仍会保留。"))
	FLinearColor AvailableColor = FLinearColor(0.08f, 0.78f, 0.88f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Run Path|Presentation",
		meta = (ToolTip = "当前聚焦道路的稳定语义颜色。推荐琥珀色；即使关闭装饰性闪光仍会保留。"))
	FLinearColor FocusedColor = FLinearColor(1.0f, 0.55f, 0.08f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Run Path|Presentation",
		meta = (ToolTip = "可选道路的基础自发光强度。推荐 2 到 8；只影响材质表现，不影响碰撞或布局。"))
	float AvailableGlowStrength = 3.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Run Path|Presentation",
		meta = (ToolTip = "聚焦道路的基础自发光强度。推荐 5 到 12；只影响材质表现，不影响碰撞或布局。"))
	float FocusedGlowStrength = 7.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Run Path|Presentation",
		meta = (ToolTip = "道路入口使用的双面 Unlit 自发光材质软引用。生成资产缺失时安全退化为基础网格，不阻止规则或编辑器命令启动。"))
	TSoftObjectPtr<UMaterialInterface> EntranceMaterial;

	EWacomRunPathBranchPresentationState PresentationState =
		EWacomRunPathBranchPresentationState::Hidden;
	float DecorativeFlashScale = 1.0f;
	FDelegateHandle RuntimeSettingsChangedHandle;
	FTimerHandle AttentionPulseTimerHandle;

	mutable FWacomRunPathBranchRequestedNative BranchRequestedNative;
};
