// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Actors/WacomShopTriggerActor.h"
#include "Camera/WacomFirstPersonViewStageRequest.h"
#include "WacomWorldShopActor.generated.h"

class UChildActorComponent;
class USceneComponent;
class UWacomRunMapNodeBindingComponent;

/**
 * 可直接放入人工关卡的组合式实体商店。
 *
 * 本 Actor 只把交互 Trigger、镜头 Viewpoint、World Shop Host、地图节点绑定和
 * 场景制作根组合为一个关卡实例。库存、购买、强化、Visit 和 SaveGame 仍由
 * WacomRun 负责；Blueprint 子类不得在 EventGraph 中复制规则。
 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomWorldShopActor : public AWacomShopTriggerActor
{
	GENERATED_BODY()

public:
	AWacomWorldShopActor();

	UFUNCTION(BlueprintPure, Category = "Wacom|World Shop|Authoring")
	USceneComponent* GetPresentationRootComponent() const
	{
		return PresentationRoot;
	}

	UFUNCTION(BlueprintPure, Category = "Wacom|World Shop|Authoring")
	UChildActorComponent* GetWorldShopHostComponent() const
	{
		return WorldShopHostComponent;
	}

	UFUNCTION(BlueprintPure, Category = "Wacom|World Shop|Authoring")
	UChildActorComponent* GetShopEntryViewpointComponent() const
	{
		return ShopEntryViewpointComponent;
	}

	UFUNCTION(BlueprintPure, Category = "Wacom|World Shop|Authoring")
	UWacomRunMapNodeBindingComponent* GetRunMapNodeBindingComponent() const
	{
		return RunMapNodeBinding;
	}

	/** 返回已创建的内部 Host；组件尚未注册时可能为空。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|World Shop|Authoring")
	AWacomWorldShopHostActor* GetInternalWorldShopHost() const;

	/** 返回已创建的内部 Viewpoint；组件尚未注册时可能为空。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|World Shop|Authoring")
	AWacomFirstPersonViewpointActor* GetInternalShopEntryViewpoint() const;

	/** 内部 Viewpoint 的入场混合时长，单位秒。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|World Shop|Camera",
		meta = (Units = "s", ClampMin = "0.0", ToolTip = "进入实体商店内部 Viewpoint 时的镜头混合时长，单位秒。默认 0.25；只影响表现，不修改 Run 状态。"))
	float ShopEntryBlendTimeSeconds = 0.25f;

	/** 内部 Viewpoint 的入场混合曲线。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|World Shop|Camera",
		meta = (ToolTip = "进入实体商店内部 Viewpoint 时使用的镜头混合曲线；只影响表现。"))
	EWacomFirstPersonViewStageBlendCurve ShopEntryBlendCurve =
		EWacomFirstPersonViewStageBlendCurve::SmoothStep;

	/** Ease 曲线强度；SmoothStep 不使用该值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|World Shop|Camera",
		meta = (ClampMin = "0.01", ToolTip = "实体商店 Viewpoint 使用 Ease 曲线时的强度；SmoothStep 不读取该值。"))
	float ShopEntryBlendEasePower = 2.0f;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual AWacomFirstPersonViewpointActor* ResolveShopEntryViewpoint() const override;
	virtual AWacomWorldShopHostActor* ResolveWorldShopHost() const override;

private:
	void ApplyInternalViewpointDefaults() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|World Shop|Authoring",
		meta = (AllowPrivateAccess = "true", ToolTip = "货架、商品 Host 和点击范围的共同制作根。移动它不会改变商店持久化身份。"))
	TObjectPtr<USceneComponent> PresentationRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|World Shop|Authoring",
		meta = (AllowPrivateAccess = "true", ToolTip = "内部 World Shop Host。运行时商品 WidgetComponent 只生成在该 Host 下。"))
	TObjectPtr<UChildActorComponent> WorldShopHostComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|World Shop|Authoring",
		meta = (AllowPrivateAccess = "true", ToolTip = "内部第一人称商店 Viewpoint。无需在地图中额外摆放并手工引用 Viewpoint Actor。"))
	TObjectPtr<UChildActorComponent> ShopEntryViewpointComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run Map",
		meta = (AllowPrivateAccess = "true", ToolTip = "实体商店对应的 Run Map 节点绑定；NodeId 由关卡实例配置，NodeType 固定为 Shop。"))
	TObjectPtr<UWacomRunMapNodeBindingComponent> RunMapNodeBinding = nullptr;
};
