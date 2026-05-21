// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WacomWorldInteractable.generated.h"

class AWacomPlayerController;

/**
 * 探索期世界交互对象接口。
 *
 * PlayerController 只负责维护候选对象、选择最近对象和转发交互请求；
 * 具体交互行为由实现者决定，例如战斗触发器、商店、拾取物或事件点。
 */
UINTERFACE(BlueprintType)
class WACOMAPP_API UWacomWorldInteractable : public UInterface
{
	GENERATED_BODY()
};

class WACOMAPP_API IWacomWorldInteractable
{
	GENERATED_BODY()

public:
	/** 返回探索 HUD 上显示的交互提示文本，例如“按 E 战斗”。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Wacom|Interaction")
	FText GetInteractPromptText(AWacomPlayerController* PC) const;

	/** 返回用于“多个候选交互对象”距离排序的位置。默认通常是 Actor 位置。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Wacom|Interaction")
	FVector GetInteractLocation(AWacomPlayerController* PC) const;

	/** 当前是否允许交互。不可交互对象不会显示 Toast，也不会响应 E。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Wacom|Interaction")
	bool CanInteract(AWacomPlayerController* PC) const;

	/** 执行交互。返回 true 表示请求已被接收或成功处理。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Wacom|Interaction")
	bool TryInteract(AWacomPlayerController* PC);
};
