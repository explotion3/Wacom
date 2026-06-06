// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Components/WacomBattleEnemyPartWorldTargetBridgeComponent.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
#include "WacomBattleEnemyPartActor.generated.h"

class UStaticMesh;
class UUserWidget;
class UWidgetComponent;
class UPaperFlipbook;
class UPaperFlipbookComponent;
class UPaperSprite;
class UPaperSpriteComponent;
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

UCLASS(NotBlueprintable, HideDropdown, CollapseCategories,
	HideCategories = (Object, ActorComponent, Physics, Collision, Navigation, Cooking, Events, Tags, AssetUserData,
		ComponentTick, ComponentReplication, Activation, HLOD, Mobile, RayTracing, TextureStreaming))
class WACOMAPP_API UWacomBattleEnemyPartVisualComponent : public UStaticMeshComponent
{
	GENERATED_BODY()
};

UENUM(BlueprintType)
enum class EWacomBattleEnemyPartVisualLayerMode : uint8
{
	StaticSprite,
	Flipbook,
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleEnemyPartVisualLayer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual",
		meta = (ToolTip = "部位视觉层稳定 ID，例如 Head.Main、Body.Shadow。用于 debug / validation，不影响战斗规则。"))
	FName LayerId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual",
		meta = (ToolTip = "视觉层类型。StaticSprite 使用单张 PaperSprite；Flipbook 使用 PaperFlipbook 播放序列帧。只影响表现，不影响命中或战斗规则。"))
	EWacomBattleEnemyPartVisualLayerMode LayerMode = EWacomBattleEnemyPartVisualLayerMode::StaticSprite;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual",
		meta = (ToolTip = "StaticSprite 层使用的 PaperSprite。LayerMode=StaticSprite 且留空时不生成组件，但会进入 debug / validation。"))
	TObjectPtr<UPaperSprite> Sprite = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual",
		meta = (ToolTip = "Flipbook 层使用的 PaperFlipbook。LayerMode=Flipbook 且留空时不生成组件，但会进入 debug / validation。"))
	TObjectPtr<UPaperFlipbook> Flipbook = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual",
		meta = (ToolTip = "Flipbook 层播放倍率。只影响视觉播放速度；1 表示原速。", ClampMin = "0.0", ClampMax = "8.0", UIMin = "0.0", UIMax = "3.0"))
	float FlipbookPlayRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual",
		meta = (ToolTip = "Flipbook 层是否循环播放。只影响视觉表现。"))
	bool bLoopFlipbook = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual",
		meta = (ToolTip = "Flipbook 层初始播放时间，单位秒。用于让多层动画错帧。", ClampMin = "0.0", UIMin = "0.0"))
	float FlipbookStartTimeSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual",
		meta = (ToolTip = "Flipbook 层是否在生成后立即播放。关闭时停在初始播放时间。"))
	bool bAutoPlayFlipbook = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual",
		meta = (ToolTip = "视觉层相对 VisualLayersRoot 的位置。单位：厘米；只影响显示，不影响 HitBounds。"))
	FVector RelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual",
		meta = (ToolTip = "视觉层相对 VisualLayersRoot 的旋转。只影响显示，不影响 HitBounds。"))
	FRotator RelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual",
		meta = (ToolTip = "视觉层相对缩放。任一轴不能为 0；只影响显示，不影响 HitBounds。"))
	FVector RelativeScale3D = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual",
		meta = (ToolTip = "视觉层半透明排序优先级。数值越大越靠前。", UIMin = "-100", UIMax = "100"))
	int32 SortOrder = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual",
		meta = (ToolTip = "视觉层颜色和透明度。Alpha 会作为 sprite 透明度。"))
	FLinearColor Tint = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual",
		meta = (ToolTip = "是否显示该视觉层。关闭时仍保留配置，但生成组件默认隐藏。"))
	bool bVisible = true;
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

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FVector HitBoundsExtent = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FName VisualName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FName VisualMeshName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FVector VisualScale = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FVector VisualRelativeLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	bool bUsingVisualLayers = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 VisualLayerCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 GeneratedVisualLayerComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	TArray<FName> VisualLayerIds;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	TArray<FName> DuplicateVisualLayerIds;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 GeneratedStaticVisualLayerComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 GeneratedFlipbookVisualLayerComponentCount = 0;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Part",
		meta = (ToolTip = "稳定敌方部位 ID，对应 UEnemyPartDefinition::PartId，例如 Snake.Head。"))
	FName PartId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Part",
		meta = (ToolTip = "敌人 Host 内的局部部位槽位 ID，例如 Head、Body、LeftClaw。为空时兼容使用 PartId。"))
	FName PartSlotId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Part",
		meta = (ToolTip = "该部位所属的敌人槽位 ID，由 Host 刷新时注入，例如 Enemy、SnakeA、CrabB。", AllowPrivateAccess = "true"))
	FName EnemySlotId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "部位鼠标点击、拖卡命中的隐藏盒体半径。单位：厘米；会同步到内部 HitBounds。",
			ClampMin = "1.0", UIMin = "1.0"))
	FVector HitBoundsExtent = FVector(55.f, 45.f, 55.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "部位原型可见网格。只影响内部 PartVisual；留空时仅保留命中体和诊断。"))
	TObjectPtr<UStaticMesh> VisualMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "部位原型可见网格相对缩放。只影响内部 PartVisual，不影响 HitBounds 命中范围。"))
	FVector VisualScale = FVector(0.55f, 0.45f, 0.55f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "部位原型可见网格相对位置。单位：厘米；只影响内部 PartVisual。"))
	FVector VisualRelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual",
		meta = (ToolTip = "部位 2D 视觉层。非空时按 LayerMode 生成 PaperSprite / PaperFlipbook 表现层并隐藏旧 PartVisual 原型网格；视觉层不影响 HitBounds、目标身份或战斗规则。"))
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

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Part")
	UBoxComponent* GetHitBounds() const { return HitBounds; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Part")
	UStaticMeshComponent* GetPartVisual() const { return PartVisual; }

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
	UWidgetComponent* GetPredictionWidgetComponent() const { return PredictionWidgetComponent; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Part")
	UWidgetComponent* GetStatusBadgeWidgetComponent() const { return StatusBadgeWidgetComponent; }

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "把 Actor facade 字段同步到内部命中体、可见体、InteractionTarget 和 Battle Part Bridge。"))
	void RefreshAuthoringState();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Wacom|Battle|Scene Enemy|Visual",
		meta = (ToolTip = "按 VisualLayers 重新生成 PaperSprite / PaperFlipbook 视觉层。只影响显示，不影响 HitBounds、目标身份或战斗规则。"))
	void RefreshVisualLayers();

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "由 Host 调用：设置该部位所属的敌人槽位 ID。只更新制作/调试身份，不改变 BattleSession 规则。"))
	void SetEnemySlotId(FName InEnemySlotId);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "返回 Host 内局部部位槽位 ID；PartSlotId 为空时回退到 PartId。"))
	FName GetEffectivePartSlotId() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "返回当前单敌人规则绑定用的部位 ID。第一阶段仍必须配置 PartId，不能只靠 PartSlotId 绑定 BattleSession。"))
	FName GetEffectivePartDefinitionId() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "返回未来多敌人场景目标稳定身份，例如 Enemy.Head。当前 BattleSession 仍使用 PartId 绑定运行时部位。"))
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

	UFUNCTION(CallInEditor, Category = "Wacom|Battle|Scene Enemy|Prototype",
		meta = (ToolTip = "仅用于 PIE / 开发验证：配置为 Debug 蛇敌人 Head 部位样例。只修改当前 Actor facade 字段，不会修改 BattleSession，也不是正式数据入口。"))
	void ConfigureDebugSnakeHeadSample();

	UFUNCTION(CallInEditor, Category = "Wacom|Battle|Scene Enemy|Prototype",
		meta = (ToolTip = "仅用于 PIE / 开发验证：配置为 Debug 蛇敌人 Body 部位样例。只修改当前 Actor facade 字段，不会修改 BattleSession，也不是正式数据入口。"))
	void ConfigureDebugSnakeBodySample();

	UFUNCTION(CallInEditor, Category = "Wacom|Battle|Scene Enemy|Prototype",
		meta = (ToolTip = "仅用于 PIE / 开发验证：配置为 Debug 蛇敌人 Tail 部位样例。只修改当前 Actor facade 字段，不会修改 BattleSession，也不是正式数据入口。"))
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
#endif

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	void ConfigureDebugSnakeSample(
		FName InPartId,
		FName InPartSlotId,
		const FVector& InHitBoundsExtent,
		const FVector& InVisualScale,
		const FVector& InVisualRelativeLocation);
	TArray<FName> BuildDuplicateVisualLayerIds() const;
	int32 CountMissingVisualLayerAssets() const;
	int32 CountMissingVisualLayerSprites() const;
	int32 CountMissingVisualLayerFlipbooks() const;
	FVector GetAppliedPredictionBadgeRelativeLocation() const;
	FVector GetAppliedStatusBadgeRelativeLocation() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Part",
		meta = (AllowPrivateAccess = "true", ToolTip = "部位鼠标点击和拖卡命中的 Visibility 盒体。不要直接编辑 Collision Details，请改 Actor facade 字段。"))
	TObjectPtr<UWacomBattleEnemyPartHitBoundsComponent> HitBounds = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Part",
		meta = (AllowPrivateAccess = "true", ToolTip = "部位视觉层根节点。视觉层和旧原型 PartVisual 都挂在此节点下，cue 缩放默认作用于整组视觉。"))
	TObjectPtr<USceneComponent> VisualLayersRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Part",
		meta = (AllowPrivateAccess = "true", ToolTip = "部位原型可见体。正式美术可在 Blueprint 或子类里替换。"))
	TObjectPtr<UWacomBattleEnemyPartVisualComponent> PartVisual = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPaperSpriteComponent>> GeneratedVisualLayerComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPaperFlipbookComponent>> GeneratedFlipbookVisualLayerComponents;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Part",
		meta = (AllowPrivateAccess = "true", ToolTip = "部位默认携带的通用交互目标身份组件。"))
	TObjectPtr<UWacomInteractionTargetComponent> InteractionTargetComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Part",
		meta = (AllowPrivateAccess = "true", ToolTip = "把稳定 PartId 绑定到当前战斗运行时部位 ID，并接收目标 cue。"))
	TObjectPtr<UWacomBattleEnemyPartWorldTargetBridgeComponent> WorldTargetBridgeComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Part",
		meta = (AllowPrivateAccess = "true", ToolTip = "部位上方的只读先机预测 Widget。不要直接编辑内部组件，请改 Actor facade 字段。"))
	TObjectPtr<UWidgetComponent> PredictionWidgetComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Part",
		meta = (AllowPrivateAccess = "true", ToolTip = "部位上方的常驻状态 Badge Widget。不要直接编辑内部组件，请改 Actor facade 字段。"))
	TObjectPtr<UWidgetComponent> StatusBadgeWidgetComponent = nullptr;

	int32 BadgeLayoutStaggerIndex = INDEX_NONE;
	FVector BadgeLayoutStaggerOffset = FVector::ZeroVector;
};
