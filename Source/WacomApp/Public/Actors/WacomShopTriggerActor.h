// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/WacomRunWorldClickableInteractable.h"
#include "Interaction/WacomWorldInteractable.h"
#include "Misc/DataValidation.h"
#include "RunState.h"
#include "WacomShopTriggerActor.generated.h"

class USphereComponent;
class UBoxComponent;
class AWacomFirstPersonViewpointActor;
class AWacomWorldShopHostActor;
class UShopDefinition;
class UWacomInteractionTargetComponent;
class UWacomRunWorldInteractionTargetBridgeComponent;
struct FWacomFirstPersonViewStageRequest;

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomShopTriggerDebugView
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Shop|Debug")
	FString ActorName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Shop|Debug")
	FName PersistentId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Shop|Debug")
	FString ShopDefinitionName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Shop|Debug")
	int32 ResolvedOfferCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Shop|Debug")
	FString WorldShopHostName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Shop|Debug")
	bool bWorldRouteEligible = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Shop|Debug")
	FName WorldRouteReason = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Shop|Debug")
	bool bCanInteract = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Shop|Debug")
	bool bClickTargetConfigured = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Shop|Debug")
	FName ClickTargetStableId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Shop|Debug")
	FString HoverPrompt;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Shop|Debug")
	FName LastDebugResult = NAME_None;
};

/**
 * 场景中的商店交互触发器。
 *
 * 玩家进入范围后，探索期按 E 打开商店 UI。库存、已购买状态和关闭时节点消耗由 URunSession 管理；
 * 本 Actor 只负责把关卡配置的 ShopId 和 Offers 传入 Run 层。
 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomShopTriggerActor : public AActor, public IWacomWorldInteractable, public IWacomRunWorldClickableInteractable
{
	GENERATED_BODY()

public:
	AWacomShopTriggerActor();

	/**
	 * 商店节点持久化 ID。当前 Run 内必须唯一。
	 *
	 * RunSession 使用该 ID 保存库存和已购买状态；再次打开同一 ID 会复用旧库存。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop",
		meta = (ToolTip = "商店节点在当前 Run 内的唯一 ID。RunSession 使用它保存库存和已购买状态；None 会拒绝打开商店。"))
	FName PersistentId;

	/**
	 * 商店静态内容定义。
	 *
	 * 配置后优先使用本资产中的商品列表；PersistentId 仍来自本 Actor，用于当前 Run 的库存持久化。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop",
		meta = (ToolTip = "商店静态内容定义。配置后优先使用本资产的商品列表；库存和已购买状态仍由本 Actor 的 PersistentId 持久化。"))
	TObjectPtr<UShopDefinition> ShopDefinition = nullptr;

	/**
	 * 关卡中手动配置的商店商品列表。
	 *
	 * 同一个 PersistentId 第一次打开时用本列表初始化库存；如果配置了 ShopDefinition，则本列表只作为旧关卡兼容兜底。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop",
		meta = (ToolTip = "该商店第一次打开时用于初始化库存的手动商品列表。配置 ShopDefinition 后会优先使用资产商品，本列表仅作兼容兜底。"))
	TArray<FRunShopOfferInput> Offers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop|Camera",
		meta = (ToolTip = "可选商店入口第一人称镜头站位。配置后，玩家打开商店时会先移动到该 View Pose，再显示商店界面。"))
	TObjectPtr<AWacomFirstPersonViewpointActor> ShopEntryViewpoint = nullptr;

	/** 可选实体商店宿主；合法 purchase-only 请求使用 World route，其它情况完整回退 ShopScreen。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop|World",
		meta = (ToolTip = "可选第一人称实体商店 Host。必须有足够有效 Anchor；强化服务或非法 Host 会继续打开既有 ShopScreen，不会截断商店。"))
	TObjectPtr<AWacomWorldShopHostActor> WorldShopHost = nullptr;

	/** 触发半径（cm）。玩家进入该范围后，探索期按 E 可以打开商店。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop",
		meta = (ToolTip = "玩家进入该半径后，探索期按 E 可以打开商店。单位：厘米。建议范围 50-1000。",
			ClampMin = "50.0", UIMin = "50.0", UIMax = "1000.0"))
	float TriggerRadius = 200.f;

	/** 探索 HUD 上显示的交互提示文本。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop",
		meta = (ToolTip = "玩家处于商店交互范围内时显示在探索 HUD 上的提示文本。"))
	FText InteractPromptText;

	/** 鼠标指向 ClickBounds 时探索 HUD 上显示的点击提示文本。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Shop|Click",
		meta = (ToolTip = "鼠标指向商店点击命中体时显示的提示文本。只影响 hover 提示；点击后仍走 IWacomWorldInteractable。"))
	FText HoverPromptText;

	UFUNCTION(BlueprintPure, Category = "Wacom|Shop")
	USphereComponent* GetTriggerSphere() const { return TriggerSphere; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Shop")
	UBoxComponent* GetClickBounds() const { return ClickBounds; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Shop")
	UWacomInteractionTargetComponent* GetClickInteractionTargetComponent() const
	{
		return ClickInteractionTargetComponent;
	}

	UFUNCTION(BlueprintPure, Category = "Wacom|Shop")
	UWacomRunWorldInteractionTargetBridgeComponent* GetClickTargetBridgeComponent() const
	{
		return ClickTargetBridgeComponent;
	}

	/** 解析当前将传给 RunSession 的商品列表：优先 ShopDefinition，未配置时使用手动 Offers。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Shop")
	TArray<FRunShopOfferInput> BuildResolvedOffers() const;

	/** 解析商品与可选强化服务，构建传给 RunSession 的规范商店访问请求。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Shop")
	FRunShopVisitRequest BuildResolvedVisitRequest() const;

	bool TryBuildShopEntryViewStageRequest(FWacomFirstPersonViewStageRequest& OutRequest) const;

	/** 返回鼠标 hover 到 ClickBounds 时应显示的提示文本。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Shop|Click",
		meta = (ToolTip = "返回鼠标 hover 到 ClickBounds 时应显示的提示文本。"))
	FText GetHoverPromptText(AWacomPlayerController* PC) const;

	/** 读取当前商店触发器配置的只读诊断信息。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Shop|Debug",
		meta = (ToolTip = "读取当前商店触发器配置和点击目标绑定的只读诊断信息；不会修改 RunState。"))
	FWacomShopTriggerDebugView GetShopTriggerDebugView(AWacomPlayerController* PC) const;

	/** 返回适合复制到日志或 PIE Details 面板查看的一行诊断摘要。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Shop|Debug",
		meta = (ToolTip = "返回适合复制到日志或 PIE Details 面板查看的一行商店触发器诊断摘要。"))
	FString GetShopTriggerDebugSummary(AWacomPlayerController* PC) const;

	/** 将当前商店触发器诊断摘要写入日志。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Shop|Debug",
		meta = (ToolTip = "将当前商店触发器诊断摘要写入日志，便于 PIE 排查商店配置和点击目标绑定。"))
	void LogShopTriggerDebugSummary(AWacomPlayerController* PC) const;

	// ---- IWacomWorldInteractable ----
	virtual FText GetInteractPromptText_Implementation(AWacomPlayerController* PC) const override;
	virtual FVector GetInteractLocation_Implementation(AWacomPlayerController* PC) const override;
	virtual bool CanInteract_Implementation(AWacomPlayerController* PC) const override;
	virtual bool TryInteract_Implementation(AWacomPlayerController* PC) override;

	// ---- IWacomRunWorldClickableInteractable ----
	virtual FText GetRunWorldClickHoverPrompt_Implementation(AWacomPlayerController* PC) const override;
	virtual FWacomRunWorldClickableInteractableDebugView GetRunWorldClickableDebugView_Implementation(
		AWacomPlayerController* PC) const override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * 解析本次商店访问使用的入口 Viewpoint。
	 *
	 * 旧 Trigger 默认返回关卡手工引用；组合式正式商店可重写为内部 ChildActor，
	 * 从而不需要 Blueprint Construction Script 拼接引用。
	 */
	virtual AWacomFirstPersonViewpointActor* ResolveShopEntryViewpoint() const;

	/**
	 * 解析本次商店访问使用的 World Shop Host。
	 *
	 * 旧 Trigger 默认返回关卡手工引用；组合式正式商店可重写为内部 ChildActor。
	 */
	virtual AWacomWorldShopHostActor* ResolveWorldShopHost() const;

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
	void RefreshClickTargetBinding();
	bool HasDuplicatePersistentIdInWorld() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Shop",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> TriggerSphere = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Shop|Click",
		meta = (AllowPrivateAccess = "true", ToolTip = "鼠标点击命中体。只用于 Visibility trace，不产生 overlap；点击后仍走 IWacomWorldInteractable。"))
	TObjectPtr<UBoxComponent> ClickBounds = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Shop|Click",
		meta = (AllowPrivateAccess = "true", ToolTip = "Shop 触发器默认携带的通用交互目标身份组件。"))
	TObjectPtr<UWacomInteractionTargetComponent> ClickInteractionTargetComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Shop|Click",
		meta = (AllowPrivateAccess = "true", ToolTip = "把 Shop 触发器标记为 Run World Target，供鼠标 probe 和点击桥接识别。"))
	TObjectPtr<UWacomRunWorldInteractionTargetBridgeComponent> ClickTargetBridgeComponent = nullptr;
};
