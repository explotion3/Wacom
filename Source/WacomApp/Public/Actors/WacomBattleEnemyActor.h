// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
#include "WacomBattleEnemyActor.generated.h"

class AWacomBattleEnemyPartActor;
class UEnemyDefinition;
class UMaterialInterface;
class UPaperFlipbook;
class UPaperFlipbookComponent;
class UPaperSprite;
class UPaperSpriteComponent;
class USceneComponent;
class UWidgetComponent;
class UWacomBattleEnemyHostVisualComponent;
class UWacomBattleEnemyHostAnimationStyle;
class UWacomBattleEnemyPartImpactStyle;
class UWacomBattleEnemyPartTargetPreviewStyle;
class UWacomBattleEnemyPanelWidget;
struct FWacomBattleEnemyPartEntryViewData;
struct FWacomBattleEnemyPanelViewData;

UENUM(BlueprintType)
enum class EWacomBattleEnemyHostVisualMode : uint8
{
	StaticSprite UMETA(DisplayName = "Static Sprite"),
	Flipbook UMETA(DisplayName = "Flipbook"),
};

UENUM(BlueprintType)
enum class EWacomBattleEnemyHostAuthoringMode : uint8
{
	SimpleHostVisual UMETA(DisplayName = "Simple Enemy (Host Visual)"),
	MultiPartVisualLayers UMETA(DisplayName = "Multi-Part / Boss (Part Visual Layers)"),
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleSceneEnemyDebugView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FString ActorName;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FName EnemyDefinitionName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FName EnemyId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FName EnemySlotId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FName AuthoringMode = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "当前 Host 整体视觉模式：StaticSprite、Flipbook 或 None。整体视觉只影响显示，不参与 HitBounds 命中。"))
	FName HostVisualMode = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "当前 Host 是否有可见整体视觉资源。普通小怪可以用它承载整只敌人的显示。"))
	bool bUsingHostVisual = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FName HostVisualAssetName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "当前 Host 语义动画 Style 资产名；未配置时为 None。"))
	FName HostAnimationStyleAssetName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "当前 Host Flipbook 组件正在显示的 authored Idle、Action 或 Destroyed Clip 名。"))
	FName CurrentHostAnimationClipName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "当前行动动画对应的显式 IntentId；Idle、Destroyed 或无活动播放时为 None。"))
	FName CurrentHostAnimationIntentId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	bool bHostAnimationPlaybackActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	bool bHostAnimationTerminalState = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "当前 Host 是否已由完成的 Encounter 退役并隐藏。"))
	bool bRuntimeEncounterPresentationRetired = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 HostAnimationPlayCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 HostAnimationWatchdogCompletionCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 GeneratedHostVisualComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "当前已注册到 World 的 Host 整体视觉组件数量。用于排查蓝图视口正常但 PIE 未显示的问题。"))
	int32 RegisteredHostVisualComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "当前可见的 Host 整体视觉组件数量。组件已生成但不可见时优先检查 bHostVisualVisible、资源和运行时刷新。"))
	int32 VisibleHostVisualComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "Host 制作诊断状态：Ready、MissingEnemyDefinition、NoPartActors、DuplicatePartSlotIds、PartSlotMismatch 或 PartDefinitionMismatch。"))
	FName AuthoringState = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "当前 Host 制作配置是否完整可用。Ready 时为 true；仍不代表 BattleSession 已绑定运行时 Snapshot。"))
	bool bAuthoringReady = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 AttachedPartActorCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	TArray<FName> AttachedPartIds;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	TArray<FName> AttachedPartSlotIds;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	TArray<FName> StableSceneTargetIds;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	TArray<FName> UnknownPartIds;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	TArray<FName> UnknownPartSlotIds;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	TArray<FName> MissingDefinitionPartIds;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	TArray<FName> MissingDefinitionPartSlotIds;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	TArray<FName> DuplicatePartSlotIds;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	TArray<FName> PartDefinitionMismatchSlotIds;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	TArray<FString> SurplusPartActorNames;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 BoundPartActorCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 UnboundPartActorCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 RuntimeFactsPartActorCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 RuntimeInitiativeTotal = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 HoveredPartActorCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 PredictionVisiblePartActorCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 BadgeLayoutAppliedPartActorCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	bool bUsedByBattleHUD = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FString ActiveBattleHUDName;
};

/**
 * Battle 场景敌人 Host。
 *
 * Host 是敌人整体 prefab 根 Actor：负责 EnemyDefinition 校验、EnemySlotId 注入、
 * 子 PartActor 扫描和 debug；实际命中和视觉表现由子级 AWacomBattleEnemyPartActor 负责。
 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomBattleEnemyActor : public AActor
{
	GENERATED_BODY()

public:
	AWacomBattleEnemyActor();
	virtual void PostLoad() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Identity",
		meta = (ToolTip = "Host 对应的敌人定义。用于校验子 PartActor 的 PartId / PartSlotId 是否对应 EnemyDefinition.Parts；运行时敌人列表仍由 BattleTrigger.EncounterDefinition 提供。"))
	TObjectPtr<UEnemyDefinition> EnemyDefinition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Identity",
		meta = (ToolTip = "Host 默认敌人槽位 ID。单敌人通常为 Enemy；关卡 BattleTrigger.SceneEnemyHostSlots 会在进入战斗前按 Encounter enemy slot 注入或覆盖它。"))
	FName EnemySlotId = TEXT("Enemy");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "敌人场景制作模式。Simple Enemy 使用 Host 整体视觉并让部位保持 HitOnly；Multi-Part / Boss 使用各 PartActor.VisualLayers。同步入口不会清空或改写任何已有视觉资源。"))
	EWacomBattleEnemyHostAuthoringMode HostAuthoringMode =
		EWacomBattleEnemyHostAuthoringMode::SimpleHostVisual;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Presentation|Impact",
		meta = (ToolTip = "Host 下所有部位默认使用的世界像素命中特效 Style。单个 PartActor 的 ImpactStyleOverride 优先；为空时仍消费 Cue，但不播放 Niagara。"))
	TObjectPtr<UWacomBattleEnemyPartImpactStyle> DefaultImpactStyle = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Presentation|Target Preview",
		meta = (ToolTip = "Host 下所有部位默认使用的拖卡世界目标像素预演 Style。单个 PartActor 的 TargetPreviewStyleOverride 优先；为空时保留规则和预测 Badge，但不播放目标框。"))
	TObjectPtr<UWacomBattleEnemyPartTargetPreviewStyle> DefaultTargetPreviewStyle = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Presentation|Host Visual",
		meta = (ToolTip = "Host 整体视觉类型。普通小怪推荐在 Host 上放一张整体 Sprite 或 Flipbook；部位命中仍由子 PartActor.HitBounds 决定。"))
	EWacomBattleEnemyHostVisualMode HostVisualMode = EWacomBattleEnemyHostVisualMode::StaticSprite;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Presentation|Host Visual",
		meta = (ToolTip = "Host 整体静态 PaperSprite。HostVisualMode=StaticSprite 时使用；只影响显示，不影响命中或战斗规则。"))
	TObjectPtr<UPaperSprite> HostSprite = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Presentation|Host Visual",
		meta = (ToolTip = "Host 整体 PaperFlipbook。HostVisualMode=Flipbook 时使用；适合普通小怪整体 idle。只影响显示，不影响命中或战斗规则。"))
	TObjectPtr<UPaperFlipbook> HostFlipbook = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Presentation|Host Animation",
		meta = (ToolTip = "SimpleHostVisual 的语义动画 Style。HostFlipbook 继续作为 Idle；本资产只按 EnemyPartActed.IntentId 播放 Action，并在敌人全部部位破坏后播放 Destroyed。不会修改战斗规则或根据名称猜动画。"))
	TObjectPtr<UWacomBattleEnemyHostAnimationStyle> HostAnimationStyle = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Presentation|Host Visual|Flipbook",
		meta = (ToolTip = "Host 整体 Flipbook 播放倍率。1 表示原速；只影响显示。", ClampMin = "0.0", ClampMax = "8.0", UIMin = "0.0", UIMax = "3.0"))
	float HostFlipbookPlayRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Presentation|Host Visual|Flipbook",
		meta = (ToolTip = "Host 整体 Flipbook 是否循环播放。只影响显示。"))
	bool bLoopHostFlipbook = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Presentation|Host Visual|Flipbook",
		meta = (ToolTip = "Host 整体 Flipbook 初始播放时间，单位秒。", ClampMin = "0.0", UIMin = "0.0"))
	float HostFlipbookStartTimeSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Presentation|Host Visual|Flipbook",
		meta = (ToolTip = "Host 整体 Flipbook 是否生成后立即播放。关闭时停在初始播放时间。"))
	bool bAutoPlayHostFlipbook = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Presentation|Host Visual|Transform",
		meta = (ToolTip = "Host 整体视觉相对 HostVisualRoot 的位置。单位：厘米；只影响显示，不影响子 PartActor.HitBounds。"))
	FVector HostVisualRelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Presentation|Host Visual|Transform",
		meta = (ToolTip = "Host 整体视觉相对 HostVisualRoot 的旋转。只影响显示，不影响命中。"))
	FRotator HostVisualRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Presentation|Host Visual|Transform",
		meta = (ToolTip = "Host 整体视觉相对缩放。只影响显示，不影响命中。"))
	FVector HostVisualRelativeScale3D = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Presentation|Host Visual|Rendering",
		meta = (ToolTip = "Host 整体视觉半透明排序优先级。数值越大越靠前。", UIMin = "-100", UIMax = "100"))
	int32 HostVisualSortOrder = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Presentation|Host Visual|Rendering",
		meta = (ToolTip = "Host 整体视觉颜色和透明度。Alpha 会作为 sprite / flipbook 透明度。"))
	FLinearColor HostVisualTint = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Presentation|Host Visual|Rendering",
		meta = (ToolTip = "Host 整体 PaperSprite / PaperFlipbook 的材质覆盖。需要投射阴影时可指定 Paper2D 的 MaskedLitSpriteMaterial 或等效 lit masked 材质。"))
	TObjectPtr<UMaterialInterface> HostVisualMaterialOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Presentation|Host Visual|Rendering",
		meta = (ToolTip = "Host 整体 PaperSprite / PaperFlipbook 是否投射阴影。需要材质支持光照/Masked，并确保场景光源开启阴影。"))
	bool bHostVisualCastShadow = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Presentation|Host Visual|Rendering",
		meta = (ToolTip = "是否显示 Host 整体视觉。关闭时保留配置，但不会让子 PartActor 进入 HitOnly 视觉模式。"))
	bool bHostVisualVisible = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Presentation|Badge Layout",
		meta = (ToolTip = "是否按 Host 部位顺序给状态/预测 Badge 加稳定错开，降低部位靠近时的重叠。只影响 UI 表现。"))
	bool bApplyAttachedPartBadgeStagger = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Presentation|Badge Layout",
		meta = (ToolTip = "相邻部位 Badge 的横向错开距离。单位：厘米；与竖向错开一起叠加到 PartActor badge facade 位置。", ClampMin = "0.0", ClampMax = "300.0", UIMin = "0.0", UIMax = "80.0"))
	float BadgeStaggerHorizontalStep = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Presentation|Badge Layout",
		meta = (ToolTip = "相邻部位 Badge 的竖向错开距离。单位：厘米；与横向错开一起叠加到 PartActor badge facade 位置。", ClampMin = "0.0", ClampMax = "300.0", UIMin = "0.0", UIMax = "80.0"))
	float BadgeStaggerVerticalStep = 18.0f;

	UPROPERTY(EditAnywhere, Category = "Wacom|Battle|Scene Enemy|Presentation|Enemy Panel",
		meta = (ToolTip = "敌人头顶聚合面板的 Host 专用 WBP override。为空时，恰好一个有效 Definition PartSlot 使用项目单部位默认类，其他情况使用项目普通默认类。"))
	TSubclassOf<UWacomBattleEnemyPanelWidget> EnemyPanelWidgetClass;

	UPROPERTY(EditAnywhere, Category = "Wacom|Battle|Scene Enemy|Presentation|Enemy Panel",
		meta = (ToolTip = "敌人头顶聚合面板相对 Host 根节点的位置，单位 cm。"))
	FVector EnemyPanelRelativeLocation = FVector(0.0f, 0.0f, 180.0f);

	UPROPERTY(EditAnywhere, Category = "Wacom|Battle|Scene Enemy|Presentation|Enemy Panel",
		meta = (ToolTip = "关闭 Desired Size 时敌人头顶聚合面板的固定渲染尺寸，单位 Slate 像素；建议约 320×180 至 560×420，会直接影响固定画布布局。", ClampMin = "1.0", UIMin = "64.0"))
	FVector2D EnemyPanelDrawSize = FVector2D(360.0f, 220.0f);

	UPROPERTY(EditAnywhere, Category = "Wacom|Battle|Scene Enemy|Presentation|Enemy Panel",
		meta = (ToolTip = "是否让 WidgetComponent 使用 WBP Desired Size，并以底部中心为 Pivot 向上增长。关闭时使用 EnemyPanelDrawSize 固定画布。"))
	bool bEnemyPanelDrawAtDesiredSize = true;

	UPROPERTY(EditAnywhere, Category = "Wacom|Battle|Scene Enemy|Presentation|Enemy Panel",
		meta = (ToolTip = "是否在有敌人数据时常驻显示头顶聚合面板。关闭后只在部位悬浮等交互状态显示。"))
	bool bEnemyPanelVisibleByDefault = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status|Last Sync",
		meta = (ToolTip = "最近一次通过 Editor Details 显式同步 EnemyDefinition 部位的结果：NotRun、EditorOnly、MissingEnemyDefinition、NoValidDefinitionParts、NoChanges、Applied、AppliedWithInvalidDefinitionSlots、ApplyFailed 或 PartiallyApplied。"))
	FName AuthoringLastPartSyncResult = TEXT("NotRun");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status|Last Sync",
		meta = (ToolTip = "最近一次显式同步自动创建的 PartSlotId。新部位使用零相对变换和 PartActor 默认 facade，等待内容人员继续摆放。"))
	TArray<FName> AuthoringLastAddedPartSlotIds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status|Last Sync",
		meta = (ToolTip = "最近一次显式同步从 EnemyDefinition 派生并修正 PartId 的已有 PartSlotId。"))
	TArray<FName> AuthoringLastUpdatedPartSlotIds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status|Last Sync",
		meta = (ToolTip = "EnemyDefinition 中 PartSlotId 为空、重复、缺 PartDefinition 或 PartDefinition.PartId 为空的槽位；显式同步不会为这些无效定义生成部位。"))
	TArray<FName> AuthoringLastInvalidDefinitionPartSlotIds;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy",
		meta = (ToolTip = "返回当前 Host 的战斗敌人部位 Actor。正式路径扫描 Host 蓝图/子 Actor；不会自动生成部位。"))
	TArray<AWacomBattleEnemyPartActor*> GetBattleEnemyPartActors() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Presentation",
		meta = (ToolTip = "返回 Host 整体视觉根节点。整体视觉只负责显示，不负责命中。"))
	USceneComponent* GetHostVisualRoot() const { return HostVisualRoot; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Presentation",
		meta = (ToolTip = "返回当前生成的 Host 静态 Sprite 组件；没有或模式不匹配时为空。"))
	UPaperSpriteComponent* GetGeneratedHostSpriteVisualComponent() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Presentation",
		meta = (ToolTip = "返回当前生成的 Host Flipbook 组件；没有或模式不匹配时为空。"))
	UPaperFlipbookComponent* GetGeneratedHostFlipbookVisualComponent() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Presentation",
		meta = (ToolTip = "当前 Host 是否配置了可生成的整体视觉资源。"))
	bool HasHostVisualResource() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Presentation",
		meta = (ToolTip = "当前 Host 是否有可见整体视觉资源。只有为 true 时，子 PartActor 会被视为可走 HitOnly 视觉模式。"))
	bool IsHostVisualActive() const;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "刷新当前 Host 子部位 Actor facade。会扫描子部位并注入 EnemySlotId；不会自动生成子 Actor。"))
	void RefreshBattleEnemyPartAuthoringState() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "返回当前 Host 的敌人槽位 ID。正式多敌人身份由 BattleTrigger.SceneEnemyHostSlots 注入；这里不根据 Actor 名称或空值推断。"))
	FName GetEffectiveEnemySlotId() const;

	/**
	 * 为 BattleHUD runtime registry 建立一次轻量场景绑定。
	 * 只扫描 live PartActor 并同步身份、Host 表现语境与 Badge 布局；不重建 Host/Part 视觉或制作状态。
	 */
	void InitializeRuntimeSceneBinding(TArray<AWacomBattleEnemyPartActor*>& OutPartActors) const;

	/** 由 Battle presentation queue 请求一次行动动画；缺配置时同步完成。 */
	void PlayRuntimeHostActionAnimation(FName IntentId, TFunction<void()>&& Completion);

	/** 由 Battle presentation queue 请求整体终态动画；缺配置时同步完成。 */
	void PlayRuntimeHostDestroyedAnimation(TFunction<void()>&& Completion);

	/** 新战斗首次接管该 Host 时，仅在残留 runtime 播放或终态存在时恢复 authored Idle。 */
	void ResetRuntimeHostAnimation();

	/** 新战斗首次接管该 Host 时恢复 Host Idle 与全部 Part authored 资源。 */
	void ResetRuntimeScenePresentationForBattle();

	/** Host/source/session 清理时安全结束当前动画 barrier。 */
	void CancelRuntimeHostAnimation();

	/** Encounter 正式完成后清理运行时表现、退役全部 Part 并隐藏 Host；不重建视觉组件。 */
	void RetireRuntimeEncounterPresentation();

	/** 当前 Host 是否已被完成的 Encounter 场景退役。 */
	bool IsRuntimeEncounterPresentationRetired() const
	{
		return bRuntimeEncounterPresentationRetired;
	}

	/** 标记 live PartActor 拓扑发生变化；供 PartActor BeginPlay/EndPlay 和显式运行时装配调用。 */
	void InvalidateRuntimePartTopology();

	/** 返回 runtime PartActor 拓扑版本；普通 Snapshot 只比较版本，不扫描 Actor 层级。 */
	uint32 GetRuntimePartTopologyRevision() const { return RuntimePartTopologyRevision; }

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "按当前 Host 部位顺序刷新状态/预测 Badge 的稳定错开布局。不会自动生成子 Actor。"))
	void RefreshAttachedPartBadgeLayout() const;

	void SetEnemyPanelViewData(const FWacomBattleEnemyPanelViewData& ViewData);

	void ClearEnemyPanelViewData();

	void SetEnemyPanelActionPreview(const TArray<FWacomBattleEnemyPartEntryViewData>& PreviewParts);

	void ClearEnemyPanelActionPreview();

	/** 设置当前 hover 的稳定 PartSlotId；NAME_None 清除 hover 上下文。 */
	void SetEnemyPanelHoveredPart(FName PartSlotId);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "读取战斗场景敌人 Host 的只读诊断信息。"))
	FWacomBattleSceneEnemyDebugView GetBattleSceneEnemyDebugView() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "读取战斗场景敌人 Host 的只读诊断信息，并标记它是否被指定 BattleHUD 使用。"))
	FWacomBattleSceneEnemyDebugView GetBattleSceneEnemyDebugViewForHUD(const class UBattleHUD* HUD) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "返回战斗场景敌人 Host 的一行诊断摘要。"))
	FString GetBattleSceneEnemyDebugSummary() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "返回战斗场景敌人 Host 相对指定 BattleHUD 的一行诊断摘要。"))
	FString GetBattleSceneEnemyDebugSummaryForHUD(const class UBattleHUD* HUD) const;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "将战斗场景敌人 Host 的诊断摘要写入日志。"))
	void LogBattleSceneEnemyDebugSummary() const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	void RefreshHostVisual();
	void RefreshEnemyPanelWidgetComponent();
	void RefreshEnemyPanelVisibility();
	FName GetHostVisualModeDebugName() const;
	FName GetHostVisualAssetName() const;
	int32 GetGeneratedHostVisualComponentCount() const;
	int32 GetRegisteredHostVisualComponentCount() const;
	int32 GetVisibleHostVisualComponentCount() const;
	TArray<AWacomBattleEnemyPartActor*> BuildAttachedBattleEnemyPartActors() const;
	void SyncHostIdentityToPartActors() const;
	void ApplyRuntimeBadgeLayout(
		const TArray<AWacomBattleEnemyPartActor*>& PartActors,
		TArray<FVector>& OutOffsets,
		TArray<int32>& OutIndices) const;

	bool bRuntimeEncounterPresentationRetired = false;
	bool bEnemyPanelHasViewData = false;
	bool bEnemyPanelHasActionPreview = false;
	FName EnemyPanelHoveredPartSlotId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Internal",
		meta = (AllowPrivateAccess = "true", ToolTip = "Host 根节点。部位 Actor 可附着到本 Actor 下进行分组。"))
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Wacom|Battle|Scene Enemy|Internal",
		meta = (ToolTip = "敌人头顶聚合状态面板的世界空间 WidgetComponent。"))
	TObjectPtr<UWidgetComponent> EnemyPanelWidgetComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Internal",
		meta = (AllowPrivateAccess = "true", ToolTip = "Host 整体视觉根节点。普通小怪整体图挂在这里；不参与命中。"))
	TObjectPtr<USceneComponent> HostVisualRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Internal",
		meta = (AllowPrivateAccess = "true", ToolTip = "Host 整体视觉生成组件。只负责 PaperSprite / PaperFlipbook 生命周期，不参与命中或战斗目标绑定。"))
	TObjectPtr<UWacomBattleEnemyHostVisualComponent> HostVisualComponent = nullptr;

	uint32 RuntimePartTopologyRevision = 0;
};
