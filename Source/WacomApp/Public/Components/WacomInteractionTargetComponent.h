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

	/** 当前目标的稳定标识（如敌人部位 ID 或 Run 物体 ID）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Interaction|Target", meta = (ToolTip = "当前目标的稳定标识；战斗中填 PartInstanceId，Run 中填物体 ID。"))
	FGuid TargetId;

	/** 可选的 GameplayTag 过滤，将来用于拖拽系统筛选"只接受特定类型卡牌的目标"。当前仅存储，不消费。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Interaction|Target", meta = (ToolTip = "可选过滤标签；后续拖拽系统可据此筛选目标类型。"))
	FGameplayTag InteractionTargetTag;

	// ---- IWacomInteractionTargetProvider ----
	virtual FWacomInteractionTargetHandle BuildWorldTargetHandle() const override;

	UFUNCTION(BlueprintPure, Category = "Wacom|Interaction|Target")
	FGuid GetTargetId() const { return TargetId; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Interaction|Target")
	const FGameplayTag& GetInteractionTargetTag() const { return InteractionTargetTag; }

	UFUNCTION(BlueprintCallable, Category = "Wacom|Interaction|Target")
	void SetTargetId(const FGuid& InTargetId);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Interaction|Target")
	void SetInteractionTargetTag(const FGameplayTag& InTag);

	/** 调试：在 Output Log 中打印当前组件的 BuildWorldTargetHandle 结果。选中此组件后在 Details 面板点击按钮即可验证。 */
	UFUNCTION(CallInEditor, Category = "Wacom|Interaction|Target|Debug")
	void LogHandleToConsole();
};
