// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WacomBattleEnemyPartWorldTargetBridgeComponent.generated.h"

class UWacomInteractionTargetComponent;
struct FBattleSnapshot;
struct FEnemyPartSnapshot;

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleEnemyPartWorldTargetDebugView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug", meta = (ToolTip = "敌方部位稳定 PartId，对应制作数据，只用于 PIE / 蓝图诊断。"))
	FName PartId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug", meta = (ToolTip = "当前绑定的 Battle Encounter ID。"))
	FName EncounterId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug", meta = (ToolTip = "当前绑定的敌人槽位 ID。"))
	FName EnemySlotId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug", meta = (ToolTip = "当前绑定的部位槽位 ID。"))
	FName PartSlotId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug", meta = (ToolTip = "当前战斗运行时部位实例 ID。"))
	FGuid PartInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug", meta = (ToolTip = "Bridge 当前是否绑定到战斗 Snapshot 中的部位。"))
	bool bBoundToSnapshot = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug", meta = (ToolTip = "Bridge 当前是否注册到 BattleHUD 的 scene enemy target registry。"))
	bool bRegisteredWithBattleHUD = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug", meta = (ToolTip = "Bridge 是否缓存过最近匹配到的运行时部位实例 ID。该字段只用于绑定诊断，不包含血量、意图、hover 或 cue 表现状态。"))
	bool bHasRuntimePartFacts = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	FGuid RuntimePartInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	bool bTargetable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	FName TargetDisabledReason = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|World Target|Debug")
	FName LastBindResult = TEXT("NotAttempted");
};

/**
 * Battle enemy part 到通用 World interaction target 的桥接组件。
 *
 * 本组件只负责 Battle 专用的 EncounterId + EnemySlotId + PartSlotId -> PartInstanceId 绑定和 HUD target 注册；
 * 通用命中身份仍由同 Actor 上的 UWacomInteractionTargetComponent 提供，视觉表现委托给
 * UWacomBattleEnemyPartPresentationComponent。
 */
UCLASS(ClassGroup = (Wacom), meta = (BlueprintSpawnableComponent, ToolTip = "把场景 Actor 绑定为当前战斗敌方部位 World Target。表现 cue、hover 和拖卡预览由 Presentation 组件处理。"))
class WACOMAPP_API UWacomBattleEnemyPartWorldTargetBridgeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWacomBattleEnemyPartWorldTargetBridgeComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|World Target", meta = (ToolTip = "静态部位定义 ID，对应 UEnemyPartDefinition::PartId，仅用于制作诊断；运行时绑定身份使用 EncounterId + EnemySlotId + PartSlotId。"))
	FName PartId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|World Target",
		meta = (ToolTip = "Battle Encounter 稳定 ID。场景敌人部位必须由 Host 注入完整 EncounterId / EnemySlotId / PartSlotId，Bridge 不再按 PartId 回退匹配。"))
	FName EncounterId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|World Target",
		meta = (ToolTip = "敌人槽位 ID。场景敌人部位必须由 Host 注入，不能留空后按 PartId 猜测。"))
	FName EnemySlotId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|World Target",
		meta = (ToolTip = "敌人内局部部位槽位 ID，例如 Head、Body、LeftClaw。场景目标绑定只按该槽位身份匹配，不按 PartId 回退。"))
	FName PartSlotId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|World Target", meta = (ToolTip = "是否在同步时自动把同 Actor 上的 WacomInteractionTargetComponent 标记为 Battle EnemyPart target。"))
	bool bAutoConfigureInteractionTarget = true;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|World Target")
	void SetPartId(FName InPartId);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|World Target")
	void SetBattlePartSlotIdentity(FName InEncounterId, FName InEnemySlotId, FName InPartSlotId);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|World Target")
	FGuid GetPartInstanceId() const { return PartInstanceId; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|World Target")
	FName GetBoundEncounterId() const { return BoundEncounterId; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|World Target")
	FName GetBoundEnemySlotId() const { return BoundEnemySlotId; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|World Target")
	FName GetBoundPartSlotId() const { return BoundPartSlotId; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|World Target")
	bool IsBoundToBattlePart() const { return bBoundToSnapshot && PartInstanceId.IsValid(); }

	bool SyncFromBattleSnapshot(const FBattleSnapshot& Snapshot, FEnemyPartSnapshot* OutMatchedPart = nullptr);
	void ClearBattleBinding();
	void SetBattleHUDSceneRegistryState(bool bInRegisteredWithBattleHUD);
	void SetBattleTargetableState(bool bInTargetable, FName InDisabledReason = NAME_None);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|World Target|Debug", meta = (ToolTip = "获取当前 Battle World Target 的只读调试快照；只用于 PIE / 蓝图排查，不影响战斗规则。"))
	FWacomBattleEnemyPartWorldTargetDebugView GetBattleWorldTargetDebugView() const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|World Target|Debug", meta = (ToolTip = "获取当前 Battle World Target 的单行调试摘要；用于排查部位绑定、hover、预测和 cue 状态。"))
	FString GetBattleWorldTargetDebugSummary() const;

	UFUNCTION(CallInEditor, Category = "Wacom|Battle|World Target|Debug", meta = (ToolTip = "在编辑器或 PIE 中把当前 Battle World Target 调试摘要写入 Output Log；不改变战斗或 UI 状态。"))
	void LogBattleWorldTargetDebugSummary() const;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UWacomInteractionTargetComponent* ResolveInteractionTargetComponent() const;
	void CacheRuntimePartBindingFacts(const FEnemyPartSnapshot& Part);
	void ClearRuntimePartBindingFacts();
	void ClearBattleBindingInternal(bool bClearRuntimeFacts);
	void UpdateInteractionTargetComponent();

	UPROPERTY(Transient)
	FGuid PartInstanceId;

	UPROPERTY(Transient)
	FName BoundEncounterId = NAME_None;

	UPROPERTY(Transient)
	FName BoundEnemySlotId = NAME_None;

	UPROPERTY(Transient)
	FName BoundPartSlotId = NAME_None;

	UPROPERTY(Transient)
	FGuid RuntimePartInstanceId;

	UPROPERTY(Transient)
	bool bBoundToSnapshot = false;
	bool bRegisteredWithBattleHUD = false;
	bool bTargetable = false;
	FName TargetDisabledReason = NAME_None;
	FName LastBindResult = TEXT("NotAttempted");
};
