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
class USceneComponent;

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleSceneEnemyPartSlot
{
	GENERATED_BODY()

	/** 兼容字段：稳定敌方部位 ID，对应 UEnemyPartDefinition::PartId。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy",
		meta = (ToolTip = "兼容字段：稳定敌方部位 ID，对应 UEnemyPartDefinition::PartId。Host 刷新时会同步到 PartActor.PartId。"))
	FName PartId = NAME_None;

	/** 兼容字段：Host 内的局部部位槽位 ID。为空时回退到 PartId。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy",
		meta = (ToolTip = "兼容字段：Host 内的局部部位槽位 ID，例如 Head、Body、LeftClaw。为空时回退到 PartId。"))
	FName PartSlotId = NAME_None;

	/** 该部位对应的场景 Actor。负责命中、Badge、预测和 cue 表现。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy",
		meta = (ToolTip = "该部位对应的场景 Actor。负责命中、Badge、预测和 cue 表现。"))
	TObjectPtr<AWacomBattleEnemyPartActor> PartActor = nullptr;
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
	int32 AttachedPartActorCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 PartSlotCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	bool bUsingExplicitPartSlots = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 NullSlotActorCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	TArray<FName> AttachedPartIds;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	TArray<FName> AttachedPartSlotIds;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	TArray<FName> StableSceneTargetIds;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	TArray<FName> UnknownPartIds;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	TArray<FName> MissingDefinitionPartIds;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	TArray<FName> DuplicateSlotPartIds;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	TArray<FName> DuplicatePartSlotIds;

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
	int32 StatusBadgeVisiblePartActorCount = 0;

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
 * Host 只负责分组、debug 和制作校验；实际命中和表现由子级
 * AWacomBattleEnemyPartActor 负责。当前 BattleSession 仍是单敌人规则层。
 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomBattleEnemyActor : public AActor
{
	GENERATED_BODY()

public:
	AWacomBattleEnemyActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy",
		meta = (ToolTip = "可选敌人定义。当前只用于制作校验和 debug，不作为运行时多敌人选择器。"))
	TObjectPtr<UEnemyDefinition> EnemyDefinition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy",
		meta = (ToolTip = "敌人槽位 ID。当前单敌人默认 Enemy；未来多敌人 Encounter 会注入 SnakeA、CrabB 等稳定敌人槽位。"))
	FName EnemySlotId = TEXT("Enemy");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Compatibility",
		meta = (ToolTip = "兼容旧场景敌人部位绑定列表。正式 prefab 制作请在 Host 蓝图视口内摆放子 BattleEnemyPartActor；当 Host 没有子部位时才使用这里声明的 PartActor。"))
	TArray<FWacomBattleSceneEnemyPartSlot> PartSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Badge Layout",
		meta = (ToolTip = "是否按 Host 部位顺序给状态/预测 Badge 加稳定错开，降低部位靠近时的重叠。只影响 UI 表现。"))
	bool bApplyAttachedPartBadgeStagger = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Badge Layout",
		meta = (ToolTip = "相邻部位 Badge 的横向错开距离。单位：厘米；与竖向错开一起叠加到 PartActor badge facade 位置。", ClampMin = "0.0", ClampMax = "300.0", UIMin = "0.0", UIMax = "80.0"))
	float BadgeStaggerHorizontalStep = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Badge Layout",
		meta = (ToolTip = "相邻部位 Badge 的竖向错开距离。单位：厘米；与横向错开一起叠加到 PartActor badge facade 位置。", ClampMin = "0.0", ClampMax = "300.0", UIMin = "0.0", UIMax = "80.0"))
	float BadgeStaggerVerticalStep = 18.0f;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy",
		meta = (ToolTip = "返回当前 Host 的战斗敌人部位 Actor。正式路径扫描 Host 蓝图/子 Actor；没有子部位时才兼容使用 PartSlots。"))
	TArray<AWacomBattleEnemyPartActor*> GetBattleEnemyPartActors() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Compatibility",
		meta = (ToolTip = "兼容旧调用名：返回当前 Host 的战斗敌人部位 Actor。新制作请使用 GetBattleEnemyPartActors。"))
	TArray<AWacomBattleEnemyPartActor*> GetAttachedBattleEnemyPartActors() const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "刷新当前 Host 部位 Actor facade。正式路径会扫描子部位并注入 EnemySlotId；PartSlots 只作为兼容 fallback。不会自动生成子 Actor。"))
	void RefreshBattleEnemyPartAuthoringState() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "返回当前 Host 的有效敌人槽位 ID。为空时回退为 Enemy。"))
	FName GetEffectiveEnemySlotId() const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Scene Enemy|Authoring|Compatibility",
		meta = (ToolTip = "兼容旧调用名：刷新当前 Host 部位 Actor facade。新制作请使用 RefreshBattleEnemyPartAuthoringState。"))
	void RefreshAttachedPartAuthoringState() const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "按当前 Host 部位顺序刷新状态/预测 Badge 的稳定错开布局。不会自动生成子 Actor。"))
	void RefreshAttachedPartBadgeLayout() const;

	UFUNCTION(CallInEditor, Category = "Wacom|Battle|Scene Enemy|Prototype",
		meta = (ToolTip = "仅用于 PIE / 开发验证：把当前 Host 下已有 Head/Body/Tail PartActor 配置为 Debug 蛇样例。不会自动生成部位 Actor，不会修改 BattleSession，也不是正式数据入口。"))
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
#endif

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	bool HasExplicitPartSlots() const { return PartSlots.Num() > 0; }
	bool HasAttachedBattleEnemyPartActors() const;
	TArray<AWacomBattleEnemyPartActor*> BuildAttachedBattleEnemyPartActors() const;
	TArray<AWacomBattleEnemyPartActor*> BuildExplicitPartSlotActors() const;
	void SyncExplicitPartSlotsToActors() const;
	void SyncHostIdentityToPartActors() const;
	TSet<FName> BuildDefinitionPartIdSet() const;
	TArray<FName> BuildConfiguredPartIds() const;
	TArray<FName> BuildUnknownPartIds() const;
	TArray<FName> BuildMissingDefinitionPartIds() const;
	TArray<FName> BuildDuplicateSlotPartIds() const;
	TArray<FName> BuildDuplicateConfiguredPartSlotIds() const;
	int32 CountNullSlotActors() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy",
		meta = (AllowPrivateAccess = "true", ToolTip = "Host 根节点。部位 Actor 可附着到本 Actor 下进行分组。"))
	TObjectPtr<USceneComponent> SceneRoot = nullptr;
};
