// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Actors/WacomShopTriggerActor.h"
#include "Camera/WacomFirstPersonViewStageRequest.h"
#include "Components/WacomCursorLookDriverComponent.h"
#include "UI/Shop/WacomWorldShopPresentationHost.h"
#include "WacomWorldShopActor.generated.h"

class UArrowComponent;
class UChildActorComponent;
class UDrawFrustumComponent;
class USceneComponent;
class UWacomRunMapNodeBindingComponent;
class UWacomWorldShopLayoutAnchorComponent;

/**
 * 可直接放入人工关卡的组合式实体商店。
 *
 * 本 Actor 把交互 Trigger、镜头 Viewpoint、真实 Offer Anchor、地图节点绑定和
 * 场景制作根组合为一个关卡实例。库存、购买、强化、Visit 和 SaveGame 仍由
 * WacomRun 负责；Blueprint 子类只制作表现，不得在 EventGraph 中复制规则。
 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomWorldShopActor : public AWacomShopTriggerActor
{
	GENERATED_BODY()

public:
	AWacomWorldShopActor();

	UFUNCTION(BlueprintPure, Category = "Wacom|World Shop")
	USceneComponent* GetPresentationRootComponent() const
	{
		return PresentationRoot;
	}

	UFUNCTION(BlueprintPure, Category = "Wacom|World Shop")
	USceneComponent* GetCardLayoutRootComponent() const
	{
		return CardLayoutRoot;
	}

	/** 返回按固定槽顺序排列的 BP 可视化商品 Anchor。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|World Shop")
	TArray<UWacomWorldShopLayoutAnchorComponent*>
		GetOfferLayoutAnchorsSorted() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|World Shop")
	UChildActorComponent* GetShopEntryViewpointComponent() const
	{
		return ShopEntryViewpointComponent;
	}

	UFUNCTION(BlueprintPure, Category = "Wacom|World Shop")
	UWacomRunMapNodeBindingComponent* GetRunMapNodeBindingComponent() const
	{
		return RunMapNodeBinding;
	}

	UFUNCTION(BlueprintPure, Category = "Wacom|World Shop")
	USceneComponent* GetShopFocusAnchorComponent() const
	{
		return ShopFocusAnchor;
	}

	UDrawFrustumComponent* GetShopViewFrustumComponent() const;
	UArrowComponent* GetShopFocusDirectionComponent() const;
	FWacomWorldShopPresentationHost BuildPresentationHost() const;

	/** 返回已创建的内部 Viewpoint；组件尚未注册时可能为空。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|World Shop")
	AWacomFirstPersonViewpointActor* GetInternalShopEntryViewpoint() const;

	/** 内部 Viewpoint 的入场混合时长，单位秒。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|World Shop",
		meta = (Units = "s", ClampMin = "0.0", ToolTip = "进入实体商店内部 Viewpoint 时的镜头混合时长，单位秒。默认 0.25；只影响表现，不修改 Run 状态。"))
	float ShopEntryBlendTimeSeconds = 0.25f;

	/** 内部 Viewpoint 的入场混合曲线。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|World Shop",
		meta = (ToolTip = "进入实体商店内部 Viewpoint 时使用的镜头混合曲线；只影响表现。"))
	EWacomFirstPersonViewStageBlendCurve ShopEntryBlendCurve =
		EWacomFirstPersonViewStageBlendCurve::SmoothStep;

	/** Ease 曲线强度；SmoothStep 不使用该值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|World Shop",
		meta = (ClampMin = "0.01", ToolTip = "实体商店 Viewpoint 使用 Ease 曲线时的强度；SmoothStep 不读取该值。"))
	float ShopEntryBlendEasePower = 2.0f;

	/** 正式商店是否覆盖玩家当前 Run Path 的鼠标观察参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|World Shop",
		meta = (ToolTip = "启用后，正式商店表现使用本 Actor 的浏览参数；关闭时继续复制玩家当前 Run Path 的 live 参数。"))
	bool bOverrideCursorLookProfile = false;

	/** 正式商店浏览时使用的有限鼠标观察参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|World Shop",
		meta = (EditCondition = "bOverrideCursorLookProfile", ToolTip = "组合式实体商店使用的临时 LookOnly 参数；离开后不会写回 Run 或 Battle。"))
	FWacomCursorLookProfile CursorLookProfileOverride;

	/** 正式商店全部世界商品卡使用的统一世界缩放。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "Wacom|World Shop",
		meta = (ToolTip = "正式商店全部世界卡统一使用的 Slate 像素到世界厘米绝对缩放。默认 0.13，使完整渲染平面为 93.6×126.9 厘米、可见卡面与价格框约为 77.0×122.7 厘米；建议 0.10-0.16。该值不继承地图 Actor 的非均匀缩放，不允许逐槽缩放。"))
	float CardWorldScale = 0.13f;

	/** 世界商品卡 Hover、关键词 Tooltip 与固定详情的统一表现参数。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|World Shop",
		meta = (ShowOnlyInnerProperties, ToolTip = "正式世界商店的卡牌浏览表现参数。只影响 Hover、Tooltip 和 Inspect，不修改购买或 Run 规则。"))
	FWacomWorldCardInteractionStyle WorldCardInteractionStyle;

	/** Close Browse 制作预设使用的镜头到 ShopFocus 距离。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|World Shop",
		meta = (Units = "cm", ToolTip = "执行 Apply Close Browse Preset 时使用的 Viewpoint 到 ShopFocus 距离，单位厘米。默认 220；推荐 180-320。按钮执行后仍可手工微调 Viewpoint。"))
	float CloseBrowsePresetDistanceCm = 220.0f;

	UFUNCTION(CallInEditor, Category = "Wacom|World Shop",
		meta = (ToolTip = "把内部 Viewpoint 放到 ShopFocus 正前方的 Close Browse 距离并朝向 Focus。只在按钮执行时写入 Transform，Construction 不会覆盖后续手工调整。"))
	void ApplyCloseBrowsePreset();

	UFUNCTION(CallInEditor, Category = "Wacom|World Shop",
		meta = (ToolTip = "只旋转当前内部 Viewpoint 使其朝向 ShopFocus，不修改 Viewpoint 位置。"))
	void AlignViewpointToShopFocus();

	UFUNCTION(CallInEditor, Category = "Wacom|World Shop",
		meta = (ToolTip = "输出当前镜头距离、八张卡中心所需最大 Yaw/Pitch、中心画面完整可见数量和最远交互距离；不修改场景。"))
	void DumpShopComposition() const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual AWacomFirstPersonViewpointActor* ResolveShopEntryViewpoint() const override;
	virtual FWacomWorldShopPresentationHost ResolveWorldShopHost() const override;

private:
	void ApplyInternalAuthoringDefaults() const;
	void RefreshEditorCompositionVisuals() const;
	void NormalizeLayoutAnchorContracts() const;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Wacom|World Shop Components",
		meta = (AllowPrivateAccess = "true", ToolTip = "货架、商品 Anchor 和点击范围的共同制作根。移动它不会改变商店持久化身份。"))
	TObjectPtr<USceneComponent> PresentationRoot = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Wacom|World Shop Components",
		meta = (AllowPrivateAccess = "true", ToolTip = "八个可视化商品槽的共同制作根。可在 BP Viewport 中移动或旋转；缩放会被固定为 1，卡牌大小请使用 CardWorldScale。"))
	TObjectPtr<USceneComponent> CardLayoutRoot = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Wacom|World Shop Components",
		meta = (AllowPrivateAccess = "true", ToolTip = "正式商店第 1 个商品槽。可在继承 BP 的 Viewport 中移动或旋转；槽身份与缩放由 C++ 固定。"))
	TObjectPtr<UWacomWorldShopLayoutAnchorComponent> OfferLayoutAnchor01 = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Wacom|World Shop Components",
		meta = (AllowPrivateAccess = "true", ToolTip = "正式商店第 2 个商品槽。可在继承 BP 的 Viewport 中移动或旋转；槽身份与缩放由 C++ 固定。"))
	TObjectPtr<UWacomWorldShopLayoutAnchorComponent> OfferLayoutAnchor02 = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Wacom|World Shop Components",
		meta = (AllowPrivateAccess = "true", ToolTip = "正式商店第 3 个商品槽。可在继承 BP 的 Viewport 中移动或旋转；槽身份与缩放由 C++ 固定。"))
	TObjectPtr<UWacomWorldShopLayoutAnchorComponent> OfferLayoutAnchor03 = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Wacom|World Shop Components",
		meta = (AllowPrivateAccess = "true", ToolTip = "正式商店第 4 个商品槽。可在继承 BP 的 Viewport 中移动或旋转；槽身份与缩放由 C++ 固定。"))
	TObjectPtr<UWacomWorldShopLayoutAnchorComponent> OfferLayoutAnchor04 = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Wacom|World Shop Components",
		meta = (AllowPrivateAccess = "true", ToolTip = "正式商店第 5 个商品槽。可在继承 BP 的 Viewport 中移动或旋转；槽身份与缩放由 C++ 固定。"))
	TObjectPtr<UWacomWorldShopLayoutAnchorComponent> OfferLayoutAnchor05 = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Wacom|World Shop Components",
		meta = (AllowPrivateAccess = "true", ToolTip = "正式商店第 6 个商品槽。可在继承 BP 的 Viewport 中移动或旋转；槽身份与缩放由 C++ 固定。"))
	TObjectPtr<UWacomWorldShopLayoutAnchorComponent> OfferLayoutAnchor06 = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Wacom|World Shop Components",
		meta = (AllowPrivateAccess = "true", ToolTip = "正式商店第 7 个商品槽。可在继承 BP 的 Viewport 中移动或旋转；槽身份与缩放由 C++ 固定。"))
	TObjectPtr<UWacomWorldShopLayoutAnchorComponent> OfferLayoutAnchor07 = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Wacom|World Shop Components",
		meta = (AllowPrivateAccess = "true", ToolTip = "正式商店第 8 个商品槽。可在继承 BP 的 Viewport 中移动或旋转；槽身份与缩放由 C++ 固定。"))
	TObjectPtr<UWacomWorldShopLayoutAnchorComponent> OfferLayoutAnchor08 = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Wacom|World Shop Components",
		meta = (AllowPrivateAccess = "true", ToolTip = "商店构图焦点。默认横向偏向第二列中心；Close Browse 和朝向按钮以它为目标。"))
	TObjectPtr<USceneComponent> ShopFocusAnchor = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Wacom|World Shop Components",
		meta = (AllowPrivateAccess = "true", ToolTip = "内部第一人称商店 Viewpoint。无需在地图中额外摆放并手工引用 Viewpoint Actor。"))
	TObjectPtr<UChildActorComponent> ShopEntryViewpointComponent = nullptr;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleDefaultsOnly, Category = "Wacom|World Shop Components")
	TObjectPtr<UDrawFrustumComponent> ShopViewFrustum = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = "Wacom|World Shop Components")
	TObjectPtr<UArrowComponent> ShopFocusDirection = nullptr;
#endif

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Run Map",
		meta = (AllowPrivateAccess = "true", ToolTip = "实体商店对应的 Run Map 节点绑定；NodeId 由关卡实例配置，NodeType 固定为 Shop。"))
	TObjectPtr<UWacomRunMapNodeBindingComponent> RunMapNodeBinding = nullptr;
};
