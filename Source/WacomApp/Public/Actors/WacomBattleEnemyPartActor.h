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
class UWacomBattleEnemyPartImpactStyle;
class UWacomBattleEnemyPartTargetPreviewStyle;
class UWacomBattleEnemyPartPredictionWidget;
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

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "当前部位是否具有可供世界命中特效使用的 ImpactAnchor。"))
	bool bImpactAnchorReady = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "当前 ImpactAnchor 组件名。"))
	FName ImpactAnchorName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "ImpactAnchor 相对 HitBounds 中心的位置，单位：厘米。"))
	FVector ImpactAnchorRelativeLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "当前 ImpactAnchor 世界位置，单位：厘米。"))
	FVector ImpactAnchorWorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "当前部位是否启用世界目标确认/伤害像素反馈。"))
	bool bImpactFeedbackEnabled = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "按 Part Override → Host Default 解析后的命中特效 Style。"))
	FName ResolvedImpactStyleName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "当前部位是否启用拖卡世界目标像素预演。"))
	bool bTargetPreviewFeedbackEnabled = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "按 Part Override → Host Default 解析后的拖卡目标预演 Style。"))
	FName ResolvedTargetPreviewStyleName = NAME_None;

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
	FVector PredictionBadgeRelativeLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FVector BadgeLayoutStaggerOffset = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FVector2D PredictionBadgeDrawSize = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 BadgeLayoutStaggerIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	float PredictionBadgeScale = 1.0f;

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
		meta = (ToolTip = "世界命中特效锚点相对 HitBounds 中心的位置。单位：厘米；推荐按部位视觉中心微调，不影响命中、布局或战斗规则。"))
	FVector ImpactAnchorRelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Feedback|Impact",
		meta = (ToolTip = "是否为本部位启用 TargetConfirmed / Damage 世界像素命中反馈。关闭后仍消费语义 Cue，但不创建 Niagara 或播放命中声音。"))
	bool bEnableImpactFeedback = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Feedback|Impact",
		meta = (ToolTip = "本部位专用的命中特效 Style。为空时使用 Host 的 DefaultImpactStyle；只影响表现。"))
	TObjectPtr<UWacomBattleEnemyPartImpactStyle> ImpactStyleOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Feedback|Target Preview",
		meta = (ToolTip = "是否为本部位启用拖卡世界目标像素锁定框。关闭后仍保留目标验证和预测 Badge，但不创建预演 Niagara。"))
	bool bEnableTargetPreviewFeedback = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Feedback|Target Preview",
		meta = (ToolTip = "本部位专用的拖卡目标预演 Style。为空时使用 Host 的 DefaultTargetPreviewStyle；只影响表现。"))
	TObjectPtr<UWacomBattleEnemyPartTargetPreviewStyle> TargetPreviewStyleOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Feedback",
		meta = (ToolTip = "TargetSelect 中可选部位的轻量提示缩放倍率。", ClampMin = "1.0", ClampMax = "1.5", UIMin = "1.0", UIMax = "1.2"))
	float TargetableAffordanceScale = 1.06f;

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
		meta = (ToolTip = "预测 Widget 显示时额外向上错开的距离。单位：厘米；用于避免覆盖目标反馈。", ClampMin = "0.0", ClampMax = "300.0", UIMin = "0.0", UIMax = "100.0"))
	float PredictionBadgeZOffsetWhenVisible = 42.0f;







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
		meta = (ToolTip = "当前 ImpactAnchor 是否有效。下一轮世界命中特效将以此位置作为首选生成点。"))
	bool bAuthoringImpactAnchorReady = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "当前 ImpactAnchor 组件名。"))
	FName AuthoringImpactAnchorName = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "当前 ImpactAnchor 世界位置，单位：厘米。"))
	FVector AuthoringImpactAnchorWorldLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "当前解析后的世界目标命中特效 Style；None 表示 Cue 仍会消费但没有 Niagara 表现。"))
	FName AuthoringResolvedImpactStyleName = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "当前解析后的拖卡世界目标预演 Style；None 表示仍保留规则和预测 Badge，但没有像素锁定框。"))
	FName AuthoringResolvedTargetPreviewStyleName = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (MultiLine = "true", ToolTip = "当前 PartActor 的一行诊断摘要缓存。手动执行刷新按钮或修改 Details 后会更新。"))
	FString AuthoringDebugSummary;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Part")
	UBoxComponent* GetHitBounds() const { return HitBounds; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Part")
	USceneComponent* GetVisualLayersRoot() const { return VisualLayersRoot; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Part",
		meta = (ToolTip = "返回该部位的世界命中特效锚点。只读；位置由 ImpactAnchorRelativeLocation 制作字段控制。"))
	USceneComponent* GetImpactAnchorComponent() const { return ImpactAnchor; }

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

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "由 Host 调用：设置 Host 默认命中特效 Style。Part 的 ImpactStyleOverride 始终优先。只影响表现。"))
	void SetHostImpactStyle(UWacomBattleEnemyPartImpactStyle* InHostImpactStyle);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "由 Host 调用：设置 Host 默认拖卡目标预演 Style。Part 的 TargetPreviewStyleOverride 始终优先。只影响表现。"))
	void SetHostTargetPreviewStyle(UWacomBattleEnemyPartTargetPreviewStyle* InHostTargetPreviewStyle);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Feedback",
		meta = (ToolTip = "返回 Part Override → Host Default 解析后的命中特效 Style。"))
	UWacomBattleEnemyPartImpactStyle* ResolveImpactStyle() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Feedback",
		meta = (ToolTip = "返回 Part Override → Host Default 解析后的拖卡目标预演 Style。"))
	UWacomBattleEnemyPartTargetPreviewStyle* ResolveTargetPreviewStyle() const;

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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	friend class AWacomBattleEnemyActor;

	void InitializeRuntimePresentationState();
	void ApplyRuntimeFacadeAndPresentationState();
	void ApplyRuntimeHostContext(
		FName InEnemySlotId,
		bool bInHostVisualActive,
		UWacomBattleEnemyPartImpactStyle* InHostImpactStyle,
		UWacomBattleEnemyPartTargetPreviewStyle* InHostTargetPreviewStyle,
		int32 InBadgeStaggerIndex,
		const FVector& InBadgeStaggerOffset);
	void NotifyRuntimePartTopologyChanged() const;
	void ConfigureDebugSnakeSample(
		FName InPartId,
		FName InPartSlotId,
		const FVector& InHitBoundsExtent);
	FVector GetAppliedPredictionBadgeRelativeLocation() const;
	void RefreshAuthoringStatusPreview();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Part",
		meta = (AllowPrivateAccess = "true", ToolTip = "部位鼠标点击和拖卡命中的 Visibility 盒体。不要直接编辑 Collision Details，请改 Actor facade 字段。"))
	TObjectPtr<UWacomBattleEnemyPartHitBoundsComponent> HitBounds = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Part",
		meta = (AllowPrivateAccess = "true", ToolTip = "部位视觉层根节点。VisualLayers 生成组件挂在此节点下，cue 缩放默认作用于整组视觉。"))
	TObjectPtr<USceneComponent> VisualLayersRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Part",
		meta = (AllowPrivateAccess = "true", ToolTip = "世界命中特效锚点。默认位于 HitBounds 中心；无碰撞且不参与目标身份或战斗规则。"))
	TObjectPtr<USceneComponent> ImpactAnchor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Part",
		meta = (AllowPrivateAccess = "true", ToolTip = "部位默认携带的通用交互目标身份组件。"))
	TObjectPtr<UWacomInteractionTargetComponent> InteractionTargetComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Part",
		meta = (AllowPrivateAccess = "true", ToolTip = "把 EncounterId + EnemySlotId + PartSlotId 绑定到当前战斗运行时部位 ID，并接收目标 cue。"))
	TObjectPtr<UWacomBattleEnemyPartWorldTargetBridgeComponent> WorldTargetBridgeComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Part",
		meta = (AllowPrivateAccess = "true", ToolTip = "场景敌人部位表现组件，负责语义 Cue 生命周期、hover/拖卡预览、预测 Widget 和状态 Badge。"))
	TObjectPtr<UWacomBattleEnemyPartPresentationComponent> PresentationComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Part",
		meta = (AllowPrivateAccess = "true", ToolTip = "场景敌人部位视觉层组件，负责生成 PaperSprite / PaperFlipbook 层。"))
	TObjectPtr<UWacomBattleEnemyPartVisualLayerComponent> VisualLayerComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Part",
		meta = (AllowPrivateAccess = "true", ToolTip = "部位上方的只读先机预测 Widget。不要直接编辑内部组件，请改 Actor facade 字段。"))
	TObjectPtr<UWidgetComponent> PredictionWidgetComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UWacomBattleEnemyPartImpactStyle> HostImpactStyle = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UWacomBattleEnemyPartTargetPreviewStyle> HostTargetPreviewStyle = nullptr;

	bool bHostVisualContextActive = false;

	int32 BadgeLayoutStaggerIndex = INDEX_NONE;
	FVector BadgeLayoutStaggerOffset = FVector::ZeroVector;
};
