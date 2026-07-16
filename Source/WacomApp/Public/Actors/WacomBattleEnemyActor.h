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
		meta = (ToolTip = "敌人头顶聚合面板使用的 Widget 类。为空时使用 C++ fallback UWacomBattleEnemyPanelWidget。"))
	TSubclassOf<UWacomBattleEnemyPanelWidget> EnemyPanelWidgetClass;

	UPROPERTY(EditAnywhere, Category = "Wacom|Battle|Scene Enemy|Presentation|Enemy Panel",
		meta = (ToolTip = "敌人头顶聚合面板相对 Host 根节点的位置，单位 cm。"))
	FVector EnemyPanelRelativeLocation = FVector(0.0f, 0.0f, 180.0f);

	UPROPERTY(EditAnywhere, Category = "Wacom|Battle|Scene Enemy|Presentation|Enemy Panel",
		meta = (ToolTip = "敌人头顶聚合面板的渲染尺寸，单位 Slate 像素。", ClampMin = "1.0", UIMin = "64.0"))
	FVector2D EnemyPanelDrawSize = FVector2D(360.0f, 220.0f);

	UPROPERTY(EditAnywhere, Category = "Wacom|Battle|Scene Enemy|Presentation|Enemy Panel",
		meta = (ToolTip = "是否在有敌人数据时常驻显示头顶聚合面板。关闭后只在部位悬浮等交互状态显示。"))
	bool bEnemyPanelVisibleByDefault = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "Details 只读制作状态缓存。Ready 表示 Host 的 EnemyDefinition、子 PartActor、PartId 和 PartSlotId 当前对齐。"))
	FName AuthoringState = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "Details 只读制作状态缓存。true 表示当前 Host 制作配置完整；不代表已经被 BattleHUD 绑定到运行时 Snapshot。"))
	bool bAuthoringReady = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "当前 Host 整体视觉模式：StaticSprite、Flipbook 或 None。"))
	FName AuthoringHostVisualMode = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "当前 Host 是否有可见整体视觉资源。普通小怪用它承载整只敌人的显示。"))
	bool bAuthoringUsingHostVisual = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "当前 Host 整体视觉资源名。"))
	FName AuthoringHostVisualAssetName = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "当前实际生成的 Host PaperSprite / PaperFlipbook 视觉组件数量。"))
	int32 AuthoringGeneratedHostVisualComponentCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "当前已注册到 World 的 Host 整体视觉组件数量。PIE 中为 0 时通常表示运行时未刷新或组件未成功注册。"))
	int32 AuthoringRegisteredHostVisualComponentCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "当前可见的 Host 整体视觉组件数量。已生成但为 0 时检查可见性、资源和运行时刷新。"))
	int32 AuthoringVisibleHostVisualComponentCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "当前 Host 能扫描到的 BattleEnemyPartActor 数量。"))
	int32 AuthoringPartActorCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "当前 Host 子 PartActor 的 PartId 列表。"))
	TArray<FName> AuthoringPartIds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "当前 Host 子 PartActor 的 PartSlotId 列表。"))
	TArray<FName> AuthoringPartSlotIds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "当前 Host 子 PartActor 组合出的稳定场景目标 ID 列表，例如 Enemy.Head。"))
	TArray<FName> AuthoringStableSceneTargetIds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "PartActor 中存在、但 EnemyDefinition.Parts 未声明的 PartSlotId。"))
	TArray<FName> AuthoringUnknownPartSlotIds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "EnemyDefinition.Parts 声明、但没有映射到 Host 子 PartActor 的 PartSlotId。"))
	TArray<FName> AuthoringMissingDefinitionPartSlotIds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "Host 子 PartActor 中重复的 PartSlotId。重复时场景目标绑定不可靠。"))
	TArray<FName> AuthoringDuplicatePartSlotIds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "PartSlotId 已匹配 EnemyDefinition，但 PartId 与对应 PartDefinition.PartId 不一致的槽位。执行显式同步会修正 PartId。"))
	TArray<FName> AuthoringPartDefinitionMismatchSlotIds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (ToolTip = "未匹配 EnemyDefinition 的多余 PartActor 名称。包括空 PartSlotId、未知 PartSlotId 和重复占用同一槽位的 Actor；同步不会自动删除。"))
	TArray<FString> AuthoringSurplusPartActorNames;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status|Last Sync",
		meta = (ToolTip = "最近一次显式 EnemyDefinition 部位同步结果：NotRun、EditorOnly、MissingEnemyDefinition、NoValidDefinitionParts、NoChanges、Applied 或 AppliedWithInvalidDefinitionSlots。"))
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status",
		meta = (MultiLine = "true", ToolTip = "当前 Host 的一行诊断摘要缓存。手动执行刷新按钮或修改 Details 后会更新。"))
	FString AuthoringDebugSummary;

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

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "按 EnemyDefinition.Parts 显式同步 Host 部位：用 PartSlotId 匹配并派生 PartId，自动新增缺失 ChildActorComponent；保留已有位置、HitBounds、ImpactAnchor 和 VisualLayers；多余部位只标记不删除。可重复执行。"))
	void SyncEnemyPartsFromDefinition();

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "返回当前 Host 的敌人槽位 ID。正式多敌人身份由 BattleTrigger.SceneEnemyHostSlots 注入；这里不根据 Actor 名称或空值推断。"))
	FName GetEffectiveEnemySlotId() const;

	/**
	 * 为 BattleHUD runtime registry 建立一次轻量场景绑定。
	 * 只扫描 live PartActor 并同步身份、Host 表现语境与 Badge 布局；不重建 Host/Part 视觉或制作状态。
	 */
	void InitializeRuntimeSceneBinding(TArray<AWacomBattleEnemyPartActor*>& OutPartActors) const;

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

	void SetEnemyPanelHoveredVisible(bool bVisible);

	UFUNCTION(CallInEditor, Category = "Wacom|Battle|Scene Enemy|Debug Sample",
		meta = (ToolTip = "开发样例按钮：把当前 Host 下已有 Head/Body/Tail PartActor 配置为 Debug 蛇样例。不会自动生成部位 Actor，不会创建正式 sprite 资产，也不会修改 BattleSession。"))
	void ConfigureDebugSnakeHostSample();

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
	void RefreshAuthoringStatusPreview();
	void RefreshHostVisual();
	void RefreshEnemyPanelWidgetComponent();
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
