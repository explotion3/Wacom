// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Interaction/WacomInteractionTargetProvider.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "WacomInteractionTargetComponent.generated.h"

/**
 * 通用交互目标组件。
 *
 * 挂载到任意 Actor 上，标记该 Actor 可以被 cursor trace 命中并识别为一个交互目标。
 * 组件只做身份描述（目标 ID、过滤标签），不写任何规则或表现。
 *
 * 后续拖拽系统拿到 FWacomInteractionTargetHandle 后，
 * 由对应域层的 Target Resolver 判断"当前卡能否作用到这个目标"。
 */
UCLASS(ClassGroup = (Wacom), meta = (BlueprintSpawnableComponent, ToolTip = "挂载到任意 Actor 上使其成为交互目标，供 cursor trace / 拖拽系统识别。"))
class WACOMAPP_API UWacomInteractionTargetComponent : public UActorComponent, public IWacomInteractionTargetProvider
{
	GENERATED_BODY()

public:
	UWacomInteractionTargetComponent();

	/** 当前目标的运行时标识；战斗敌方部位由 Bridge 写入 PartInstanceId，Run 中可写物体运行时 ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Interaction|Target", meta = (ToolTip = "当前目标的运行时标识；战斗敌方部位由 Bridge 写入 PartInstanceId，Run 中可写物体运行时 ID。"))
	FGuid TargetId;

	/** 目标语义标签；用于区分战斗敌方部位、Run 物体等 World target 类型。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Interaction|Target", meta = (ToolTip = "目标语义标签；用于区分战斗敌方部位、Run 物体等 World target 类型。"))
	FGameplayTag InteractionTargetTag;

	/** 美术/数据层稳定 ID，例如敌人部位 PartId 或 Run 物体 PersistentId。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Interaction|Target", meta = (ToolTip = "美术/数据层稳定 ID，例如敌人部位 PartId 或 Run 物体 PersistentId。"))
	FName StableTargetId = NAME_None;

	// ---- IWacomInteractionTargetProvider ----
	virtual FWacomInteractionTargetHandle BuildWorldTargetHandle() const override;

	UFUNCTION(BlueprintPure, Category = "Wacom|Interaction|Target")
	FGuid GetTargetId() const { return TargetId; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Interaction|Target")
	const FGameplayTag& GetInteractionTargetTag() const { return InteractionTargetTag; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Interaction|Target")
	FName GetStableTargetId() const { return StableTargetId; }

	UFUNCTION(BlueprintCallable, Category = "Wacom|Interaction|Target")
	void SetTargetId(const FGuid& InTargetId);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Interaction|Target")
	void SetInteractionTargetTag(const FGameplayTag& InTag);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Interaction|Target")
	void SetStableTargetId(FName InStableTargetId);

	/** 调试：在 Output Log 中打印当前组件的 BuildWorldTargetHandle 结果。选中此组件后在 Details 面板点击按钮即可验证。 */
	UFUNCTION(CallInEditor, Category = "Wacom|Interaction|Target|Debug")
	void LogHandleToConsole();
};
