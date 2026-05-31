// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WacomRunWorldClickableInteractable.generated.h"

class AWacomPlayerController;
class UBoxComponent;
class UPrimitiveComponent;
class UWacomInteractionTargetComponent;
class UWacomRunWorldInteractionTargetBridgeComponent;

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomRunWorldClickableInteractableDebugView
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|World Click|Debug")
	FString ActorName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|World Click|Debug")
	FName StableId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|World Click|Debug")
	bool bHasStableId = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|World Click|Debug")
	bool bImplementsWorldInteractable = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|World Click|Debug")
	bool bImplementsClickableContract = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|World Click|Debug")
	bool bCanInteract = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|World Click|Debug")
	bool bHasCompletionState = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|World Click|Debug")
	bool bIsCompleted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|World Click|Debug")
	bool bClickTargetConfigured = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|World Click|Debug")
	bool bClickBoundsConfigured = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|World Click|Debug")
	bool bHasInteractionTargetComponent = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|World Click|Debug")
	bool bHasBridgeComponent = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|World Click|Debug")
	bool bHasVisualTarget = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|World Click|Debug")
	bool bHasRenderableVisualTarget = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|World Click|Debug")
	FName VisualTargetName = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|World Click|Debug")
	bool bProbePreviewActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|World Click|Debug")
	FName ClickTargetStableId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|World Click|Debug")
	FString HoverPrompt;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|World Click|Debug")
	FName RejectReason = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|World Click|Debug")
	FName LastDebugResult = NAME_None;
};

/**
 * Run 场景中允许鼠标 hover/click 走 IWacomWorldInteractable 的显式 opt-in 合同。
 *
 * 实现者仍需要同时实现 IWacomWorldInteractable；本接口只描述鼠标点击入口的提示和调试信息。
 */
UINTERFACE(BlueprintType)
class WACOMAPP_API UWacomRunWorldClickableInteractable : public UInterface
{
	GENERATED_BODY()
};

class WACOMAPP_API IWacomRunWorldClickableInteractable
{
	GENERATED_BODY()

public:
	/** 返回鼠标 hover 到 Run World ClickBounds 时应显示在 ExplorationHUD 的提示文本。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Wacom|Run|World Click")
	FText GetRunWorldClickHoverPrompt(AWacomPlayerController* PC) const;

	/** 返回 Run world click/hover 合同的一行摘要所需的通用调试字段。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Wacom|Run|World Click|Debug")
	FWacomRunWorldClickableInteractableDebugView GetRunWorldClickableDebugView(
		AWacomPlayerController* PC) const;
};

/** 共享 Run world click target 组件配置和 debug fact 构建逻辑。 */
class WACOMAPP_API FWacomRunWorldClickableInteractableHelper
{
public:
	static void ConfigureClickBounds(UBoxComponent* ClickBounds);

	static void BindClickTarget(
		FName StableId,
		UPrimitiveComponent* VisualTargetComponent,
		UWacomInteractionTargetComponent* InteractionTargetComponent,
		UWacomRunWorldInteractionTargetBridgeComponent* BridgeComponent);

	static FWacomRunWorldClickableInteractableDebugView BuildDebugView(
		const AActor* Actor,
		FName StableId,
		const FText& HoverPrompt,
		bool bCanInteract,
		bool bHasCompletionState,
		bool bIsCompleted,
		FName LastDebugResult,
		const UWacomInteractionTargetComponent* InteractionTargetComponent,
		const UWacomRunWorldInteractionTargetBridgeComponent* BridgeComponent,
		const UBoxComponent* ClickBounds = nullptr);

	static FString BuildDebugSummary(
		const FWacomRunWorldClickableInteractableDebugView& View);
};
