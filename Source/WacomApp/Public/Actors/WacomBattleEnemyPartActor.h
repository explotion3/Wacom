// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WacomBattleEnemyPartPresentationComponent.h"
#include "GameFramework/Actor.h"
#include "Components/WacomBattleEnemyPartWorldTargetBridgeComponent.h"
#include "UI/Battle/WacomBattleEnemyPartVisualLayerTypes.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
#include "WacomBattleEnemyPartActor.generated.h"

class UUserWidget;
class UWidgetComponent;
class UPaperFlipbook;
class UPaperSprite;
class UWacomBattleEnemyPartVisualLayerComponent;
class UWacomBattleEnemyPartPredictionWidget;
class UWacomBattleEnemyPartStatusBadgeWidget;
class UWacomInteractionTargetComponent;

UCLASS(NotBlueprintable, HideDropdown, CollapseCategories,
	HideCategories = (Object, ActorComponent, Physics, Collision, Navigation, Cooking, Events, Tags, AssetUserData,
		ComponentTick, ComponentReplication, Activation, Rendering, HLOD, Mobile, RayTracing, TextureStreaming))
class WACOMAPP_API UWacomBattleEnemyPartHitBoundsComponent : public UBoxComponent
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleSceneEnemyPartDebugView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FString ActorName;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FName PartId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FName PartSlotId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FName EnemySlotId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FName StableSceneTargetId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "部位制作诊断状态：Ready、MissingIdentity、InvalidHitBounds、UsingVisualLayers、HitOnly 或 MissingVisualResource。"))
	FName AuthoringState = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "当前部位是否具备可用于场景目标绑定的身份和命中配置。视觉资源缺失只会影响表现，不会让此值变 false。"))
	bool bAuthoringReady = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "当前视觉路径：VisualLayers、HitOnly 或 None。"))
	FName VisualAuthoringMode = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "该部位是否由 Host 整体视觉承载显示。true 时本部位只提供 HitBounds / target bridge / badge。"))
	bool bUsingHostVisual = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "该部位是否为命中区模式：没有独立可见体，但 Host 提供整体视觉。"))
	bool bHitOnlyVisual = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FVector HitBoundsExtent = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	bool bUsingVisualLayers = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 VisualLayerCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 GeneratedVisualLayerComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "当前已注册到 World 的 VisualLayers 生成组件总数。用于排查蓝图视口正常但 PIE 未显示的问题。"))
	int32 RegisteredVisualLayerComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "当前可见的 VisualLayers 生成组件总数。组件已生成但不可见时优先检查 layer.bVisible、资源和运行时刷新。"))
	int32 VisibleVisualLayerComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	TArray<FName> VisualLayerIds;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	TArray<FName> DuplicateVisualLayerIds;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 GeneratedStaticVisualLayerComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 GeneratedFlipbookVisualLayerComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 RegisteredStaticVisualLayerComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 RegisteredFlipbookVisualLayerComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 VisibleStaticVisualLayerComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 VisibleFlipbookVisualLayerComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 MissingVisualLayerAssetCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 MissingVisualLayerSpriteCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 MissingVisualLayerFlipbookCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	TArray<FName> VisualLayerAssetNames;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FName FeedbackTargetName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	bool bInteractionTargetConfigured = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FGuid InteractionTargetId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FName InteractionTargetStableId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FWacomBattleEnemyPartWorldTargetDebugView BridgeDebugView;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FWacomBattleEnemyPartPresentationDebugView PresentationDebugView;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FName PredictionWidgetName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FName StatusBadgeWidgetName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FVector PredictionBadgeRelativeLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FVector StatusBadgeRelativeLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FVector BadgeLayoutStaggerOffset = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FVector2D PredictionBadgeDrawSize = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FVector2D StatusBadgeDrawSize = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 BadgeLayoutStaggerIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	float PredictionBadgeScale = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	float StatusBadgeScale = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	float StatusBadgeOpacity = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	float DestroyedStatusBadgeOpacity = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	float PredictionBadgeZOffsetWhenVisible = 0.0f;
};

/**
 * Battle 场景敌人部位 Actor。
 *
 * 每个部位是一个独立可命中的 World target，复用现有
 * UWacomInteractionTargetComponent + UWacomBattleEnemyPartWorldTargetBridgeComponent。
 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomBattleEnemyPartActor : public AActor
{
	GENERATED_BODY()

public:
	AWacomBattleEnemyPartActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Identity",
		meta = (ToolTip = "稳定敌方部位 ID，对应 UEnemyPartDefinition::PartId，例如 Snake.Head。"))
	FName PartId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Identity",
		meta = (ToolTip = "敌人 Host 内的局部部位槽位 ID，例如 Head、Body、LeftClaw。必须显式填写，并对应 EnemyDefinition.Parts[].PartSlotId。"))
	FName PartSlotId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Identity",
		meta = (ToolTip = "该部位所属的敌人槽位 ID，由 Host 刷新时注入，例如 Enemy、SnakeA、CrabB。", AllowPrivateAccess = "true"))
	FName EnemySlotId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Collision",
		meta = (ToolTip = "部位鼠标点击、拖卡命中的隐藏盒体半径。单位：厘米；会同步到内部 HitBounds。",
			ClampMin = "1.0", UIMin = "1.0"))
	FVector HitBoundsExtent = FVector(55.f, 45.f, 55.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual Layers",
		meta = (ToolTip = "正式 2D 视觉层。非空时按 LayerMode 在 VisualLayersRoot 下生成 PaperSprite / PaperFlipbook 表现层；视觉层不影响 HitBounds、目标身份或战斗规则。"))
	TArray<FWacomBattleEnemyPartVisualLayer> VisualLayers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Feedback",
		meta = (ToolTip = "目标确认 cue 的缩放倍率。", ClampMin = "1.0", ClampMax = "2.0", UIMin = "1.0", UIMax = "1.3"))
	float TargetConfirmPulseScale = 1.10f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Feedback",
		meta = (ToolTip = "伤害 cue 的缩放倍率。", ClampMin = "1.0", ClampMax = "2.0", UIMin = "1.0", UIMax = "1.4"))
	float DamagePulseScale = 1.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Feedback",
		meta = (ToolTip = "破坏 cue 的缩放倍率。", ClampMin = "1.0", ClampMax = "2.5", UIMin = "1.0", UIMax = "1.6"))
	float DestroyedPulseScale = 1.32f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Feedback",
		meta = (ToolTip = "TargetSelect 中可选部位的轻量提示缩放倍率。", ClampMin = "1.0", ClampMax = "1.5", UIMin = "1.0", UIMax = "1.2"))
	float TargetableAffordanceScale = 1.06f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Feedback",
		meta = (ToolTip = "第一人称卡牌拖拽指向该部位时的轻量预览缩放倍率。", ClampMin = "1.0", ClampMax = "1.5", UIMin = "1.0", UIMax = "1.2"))
	float DragTargetPreviewScale = 1.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Feedback",
		meta = (ToolTip = "鼠标悬停该部位时的轻量探测缩放倍率。只表示当前 hover 目标，不影响战斗规则。", ClampMin = "1.0", ClampMax = "1.5", UIMin = "1.0", UIMax = "1.15"))
	float HoverProbeScale = 1.04f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Feedback",
		meta = (ToolTip = "默认 cue 保持时间，单位秒。", ClampMin = "0.01", ClampMax = "2.0", UIMin = "0.05", UIMax = "0.5"))
	float CueHoldSeconds = 0.14f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Prediction",
		meta = (ToolTip = "是否在部位上方显示 UI 近似先机预测。只影响表现，不影响 BattleSession 规则。"))
	bool bEnablePredictionWidget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Prediction",
		meta = (ToolTip = "部位预测 Widget 类。为空时使用 C++ fallback UWacomBattleEnemyPartPredictionWidget。"))
	TSubclassOf<UWacomBattleEnemyPartPredictionWidget> PredictionWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Prediction",
		meta = (ToolTip = "预测 Widget 相对 HitBounds 的位置。单位：厘米；默认显示在部位上方。"))
	FVector PredictionRelativeLocation = FVector(0.f, 0.f, 90.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Prediction",
		meta = (ToolTip = "预测 Widget 的绘制尺寸，单位：Slate 像素。", ClampMin = "1.0", UIMin = "32.0"))
	FVector2D PredictionDrawSize = FVector2D(168.f, 58.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Prediction",
		meta = (ToolTip = "预测 Widget 的整体渲染缩放。只影响 UI 可读性，不影响命中。", ClampMin = "0.25", ClampMax = "2.0", UIMin = "0.6", UIMax = "1.2"))
	float PredictionBadgeScale = 0.92f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Prediction",
		meta = (ToolTip = "预测 Widget 显示时额外向上错开的距离。单位：厘米；用于避免覆盖常驻状态 Badge。", ClampMin = "0.0", ClampMax = "300.0", UIMin = "0.0", UIMax = "100.0"))
	float PredictionBadgeZOffsetWhenVisible = 42.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Status",
		meta = (ToolTip = "是否在部位上方显示常驻状态 Badge。只影响表现，不影响 BattleSession 规则。"))
	bool bEnableStatusBadgeWidget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Status",
		meta = (ToolTip = "部位状态 Badge Widget 类。为空时使用 C++ fallback UWacomBattleEnemyPartStatusBadgeWidget。"))
	TSubclassOf<UWacomBattleEnemyPartStatusBadgeWidget> StatusBadgeWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Status",
		meta = (ToolTip = "状态 Badge 相对 HitBounds 的位置。单位：厘米；默认显示在部位上方，和预测 Widget 错开。"))
	FVector StatusBadgeRelativeLocation = FVector(0.f, 0.f, 108.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Status",
		meta = (ToolTip = "状态 Badge 的绘制尺寸，单位：Slate 像素。", ClampMin = "1.0", UIMin = "32.0"))
	FVector2D StatusBadgeDrawSize = FVector2D(204.f, 112.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Status",
		meta = (ToolTip = "状态 Badge 的整体渲染缩放。只影响 UI 可读性，不影响命中。", ClampMin = "0.25", ClampMax = "2.0", UIMin = "0.6", UIMax = "1.2"))
	float StatusBadgeScale = 0.86f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Status",
		meta = (ToolTip = "普通状态 Badge 的透明度。只影响 UI 表现，不影响命中。", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.35", UIMax = "1.0"))
	float StatusBadgeOpacity = 0.92f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Status",
		meta = (ToolTip = "部位破坏后状态 Badge 的透明度。破坏态仍常驻显示，但视觉弱化。", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.25", UIMax = "1.0"))
	float DestroyedStatusBadgeOpacity = 0.58f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "Details 只读制作状态缓存。UsingVisualLayers 表示正式 2D 视觉层路径；HitOnly 表示由 Host 整体视觉承载显示；MissingIdentity / InvalidHitBounds 需要优先修复。"))
	FName AuthoringState = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "Details 只读制作状态缓存。true 表示身份和命中配置足以作为场景目标；视觉资源缺失只影响表现。"))
	bool bAuthoringReady = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "当前部位视觉制作路径：VisualLayers、HitOnly 或 None。"))
	FName VisualAuthoringMode = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "当前部位是否由 Host 整体视觉承载显示。普通小怪命中区模式下为 true。"))
	bool bAuthoringUsingHostVisual = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "当前部位是否为 HitOnly 命中区模式：自身不生成可见体，但保留 HitBounds、target bridge、预测和状态 Badge。"))
	bool bAuthoringHitOnlyVisual = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "当前稳定场景目标 ID，例如 Enemy.Head。由 Host 注入 EnemySlotId 后会更新。"))
	FName AuthoringStableSceneTargetId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "当前 VisualLayers 配置数量。"))
	int32 AuthoringVisualLayerCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "当前实际生成的 PaperSprite / PaperFlipbook 视觉组件总数。缺资源的 layer 不会生成组件。"))
	int32 AuthoringGeneratedVisualLayerComponentCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "当前已注册到 World 的 VisualLayers 生成组件总数。PIE 中为 0 时通常表示运行时未刷新或组件未成功注册。"))
	int32 AuthoringRegisteredVisualLayerComponentCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "当前可见的 VisualLayers 生成组件总数。已生成但为 0 时检查 layer 可见性、资源和运行时刷新。"))
	int32 AuthoringVisibleVisualLayerComponentCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "VisualLayers 中缺少对应 Sprite / Flipbook 资源的 layer 数量。"))
	int32 AuthoringMissingVisualLayerAssetCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "VisualLayers 中重复的 LayerId。重复时编辑器校验会报错。"))
	TArray<FName> AuthoringDuplicateVisualLayerIds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "当前部位 cue / hover / drag preview 缩放作用的组件名。VisualLayers 路径通常为 VisualLayersRoot。"))
	FName AuthoringFeedbackTargetName = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (MultiLine = "true", ToolTip = "当前 PartActor 的一行诊断摘要缓存。手动执行刷新按钮或修改 Details 后会更新。"))
	FString AuthoringDebugSummary;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Part")
	UBoxComponent* GetHitBounds() const { return HitBounds; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Part")
	USceneComponent* GetVisualLayersRoot() const { return VisualLayersRoot; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Part")
	UWacomInteractionTargetComponent* GetInteractionTargetComponent() const { return InteractionTargetComponent; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Part")
	UWacomBattleEnemyPartWorldTargetBridgeComponent* GetWorldTargetBridgeComponent() const
	{
		return WorldTargetBridgeComponent;
	}

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Part")
	UWacomBattleEnemyPartPresentationComponent* GetPresentationComponent() const
	{
		return PresentationComponent;
	}

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Part")
	UWacomBattleEnemyPartVisualLayerComponent* GetVisualLayerComponent() const
	{
		return VisualLayerComponent;
	}

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Part")
	UWidgetComponent* GetPredictionWidgetComponent() const { return PredictionWidgetComponent; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Part")
	UWidgetComponent* GetStatusBadgeWidgetComponent() const { return StatusBadgeWidgetComponent; }

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "把 Actor facade 字段同步到内部命中体、可见体、InteractionTarget 和 Battle Part Bridge。"))
	void RefreshAuthoringState();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Wacom|Battle|Scene Enemy|Visual Layers",
		meta = (ToolTip = "按 VisualLayers 重新生成 PaperSprite / PaperFlipbook 视觉层。只影响显示，不影响 HitBounds、目标身份或战斗规则。"))
	void RefreshVisualLayers();

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "由 Host 调用：设置该部位所属的敌人槽位 ID。只更新制作/调试身份，不改变 BattleSession 规则。"))
	void SetEnemySlotId(FName InEnemySlotId);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "由 Host 调用：说明 Host 是否提供整体视觉。为 true 且本部位没有 VisualLayers 时，部位进入 HitOnly 命中区模式。"))
	void SetHostVisualContext(bool bInHostVisualActive);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "返回 Host 是否为该部位提供整体视觉语境。只影响视觉诊断，不影响 HitBounds 或 BattleSession。"))
	bool IsHostVisualContextActive() const { return bHostVisualContextActive; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "返回 Host 内局部部位槽位 ID。必须显式配置；为空时该场景部位不能绑定 Battle Snapshot。"))
	FName GetEffectivePartSlotId() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "返回当前部位的静态部位定义 ID，对应 UEnemyPartDefinition::PartId；不作为场景目标 identity。"))
	FName GetEffectivePartDefinitionId() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "返回场景目标稳定身份，例如 Enemy.Head。由 EnemySlotId + 显式 PartSlotId 组成。"))
	FName GetStableSceneTargetId() const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "由 Host 调用：设置当前部位在场景敌人状态/预测 Badge 布局中的稳定错开序号和偏移。不会修改 Actor facade 基础位置。"))
	void SetBadgeLayoutStagger(int32 InStaggerIndex, const FVector& InStaggerOffset);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "当前 Host 布局错开序号。-1 表示未由 Host 应用错开。"))
	int32 GetBadgeLayoutStaggerIndex() const { return BadgeLayoutStaggerIndex; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "当前 Host 布局错开偏移。会叠加到 Prediction / Status Badge 的 facade 相对位置。"))
	FVector GetBadgeLayoutStaggerOffset() const { return BadgeLayoutStaggerOffset; }

	UFUNCTION(CallInEditor, Category = "Wacom|Battle|Scene Enemy|Debug Sample",
		meta = (ToolTip = "开发样例按钮：配置为 Debug 蛇敌人 Head 部位。只修改当前 Actor facade 字段，不会创建正式 sprite 资产，也不是战斗规则入口。"))
	void ConfigureDebugSnakeHeadSample();

	UFUNCTION(CallInEditor, Category = "Wacom|Battle|Scene Enemy|Debug Sample",
		meta = (ToolTip = "开发样例按钮：配置为 Debug 蛇敌人 Body 部位。只修改当前 Actor facade 字段，不会创建正式 sprite 资产，也不是战斗规则入口。"))
	void ConfigureDebugSnakeBodySample();

	UFUNCTION(CallInEditor, Category = "Wacom|Battle|Scene Enemy|Debug Sample",
		meta = (ToolTip = "开发样例按钮：配置为 Debug 蛇敌人 Tail 部位。只修改当前 Actor facade 字段，不会创建正式 sprite 资产，也不是战斗规则入口。"))
	void ConfigureDebugSnakeTailSample();

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "读取战斗场景敌人部位 Actor 的配置、桥接和表现诊断；不会修改 BattleSession。"))
	FWacomBattleSceneEnemyPartDebugView GetBattleSceneEnemyPartDebugView() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "返回战斗场景敌人部位 Actor 的一行诊断摘要。"))
	FString GetBattleSceneEnemyPartDebugSummary() const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "将战斗场景敌人部位 Actor 的诊断摘要写入日志。"))
	void LogBattleSceneEnemyPartDebugSummary() const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	void ConfigureDebugSnakeSample(
		FName InPartId,
		FName InPartSlotId,
		const FVector& InHitBoundsExtent);
	FVector GetAppliedPredictionBadgeRelativeLocation() const;
	FVector GetAppliedStatusBadgeRelativeLocation() const;
	void RefreshAuthoringStatusPreview();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Part",
		meta = (AllowPrivateAccess = "true", ToolTip = "部位鼠标点击和拖卡命中的 Visibility 盒体。不要直接编辑 Collision Details，请改 Actor facade 字段。"))
	TObjectPtr<UWacomBattleEnemyPartHitBoundsComponent> HitBounds = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Part",
		meta = (AllowPrivateAccess = "true", ToolTip = "部位视觉层根节点。VisualLayers 生成组件挂在此节点下，cue 缩放默认作用于整组视觉。"))
	TObjectPtr<USceneComponent> VisualLayersRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Part",
		meta = (AllowPrivateAccess = "true", ToolTip = "部位默认携带的通用交互目标身份组件。"))
	TObjectPtr<UWacomInteractionTargetComponent> InteractionTargetComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Part",
		meta = (AllowPrivateAccess = "true", ToolTip = "把 EncounterId + EnemySlotId + PartSlotId 绑定到当前战斗运行时部位 ID，并接收目标 cue。"))
	TObjectPtr<UWacomBattleEnemyPartWorldTargetBridgeComponent> WorldTargetBridgeComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Part",
		meta = (AllowPrivateAccess = "true", ToolTip = "场景敌人部位表现组件，负责 cue 缩放、hover/拖卡预览、预测 Widget 和状态 Badge。"))
	TObjectPtr<UWacomBattleEnemyPartPresentationComponent> PresentationComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Part",
		meta = (AllowPrivateAccess = "true", ToolTip = "场景敌人部位视觉层组件，负责生成 PaperSprite / PaperFlipbook 层。"))
	TObjectPtr<UWacomBattleEnemyPartVisualLayerComponent> VisualLayerComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Part",
		meta = (AllowPrivateAccess = "true", ToolTip = "部位上方的只读先机预测 Widget。不要直接编辑内部组件，请改 Actor facade 字段。"))
	TObjectPtr<UWidgetComponent> PredictionWidgetComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Part",
		meta = (AllowPrivateAccess = "true", ToolTip = "部位上方的常驻状态 Badge Widget。不要直接编辑内部组件，请改 Actor facade 字段。"))
	TObjectPtr<UWidgetComponent> StatusBadgeWidgetComponent = nullptr;

	bool bHostVisualContextActive = false;

	int32 BadgeLayoutStaggerIndex = INDEX_NONE;
	FVector BadgeLayoutStaggerOffset = FVector::ZeroVector;
};
