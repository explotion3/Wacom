// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Interaction/WacomInteractionTargetProvider.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
#include "WacomBattleEnemyPartComponent.generated.h"

class AWacomBattleEnemyActor;
class UUserWidget;
class UWacomBattleEnemyPartAnimationStyle;
class UWacomBattleEnemyPartImpactStyle;
class UWacomBattleEnemyPartTargetPreviewStyle;

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleEnemyPartRuntimeDebugView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FName PartSlotId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FName PartId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FName EncounterId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FName EnemySlotId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FGuid PartInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	bool bBoundToSnapshot = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	bool bRegisteredWithBattleHUD = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	bool bTargetable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	bool bDestroyed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	bool bRuntimeRetired = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 FlipbookLayerCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 SpriteLayerCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 ImpactAnchorCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FName InteractionVisualLayerId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	bool bInteractionVisualResolved = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	bool bInteractionCollisionReady = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	bool bUsingBoxCollisionFallback = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "当前交互碰撞来源：None、StableSpriteBodySetup、InteractionVisualBoundsFallback、DirectVisualUnionFallback 或 DefaultSafetyFallback。"))
	FName InteractionCollisionSource = TEXT("None");

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FName OutlineState = TEXT("None");

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	bool bOutlineComponentCreated = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 OutlineComponentCreateCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 ActionPlaybackCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	bool bActionPlaybackActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	bool bActionImpactFired = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 ActionImpactCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 ActionWatchdogCompletionCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 DestroyedVisualApplyCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FName LastCueKind = TEXT("None");

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 CuePlayCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	float CuePlaybackDurationSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 SnapshotApplyCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 SnapshotNoOpCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 TargetableApplyCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FName TargetPreviewKind = TEXT("None");

};

/**
 * 敌人场景部位的唯一身份与运行时所有者组件。
 *
 * 组件 Transform 是部位位置真相；InteractionVisualLayerId 指定的 typed visual layer
 * 提供正式精灵轮廓命中。视觉层与 ImpactAnchor 必须作为此组件的直接子组件；
 * 运行时身份和反馈由 Host runtime 管理，配置异常时由 Runtime 创建临时碰撞兜底。
 */
UCLASS(ClassGroup = (Wacom), meta = (BlueprintSpawnableComponent,
	ToolTip = "敌人场景部位身份组件。Transform 与规则身份属于 Part；正式命中由 InteractionVisualLayerId 指定的 typed visual layer 提供。"))
class WACOMAPP_API UWacomBattleEnemyPartComponent : public USceneComponent,
	public IWacomInteractionTargetProvider
{
	GENERATED_BODY()

public:
	UWacomBattleEnemyPartComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Identity",
		meta = (ToolTip = "Host 内稳定部位槽位 ID，例如 Body、Head、Left。必须对应 EnemyDefinition.Parts[].PartSlotId。"))
	FName PartSlotId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Interaction",
		meta = (ToolTip = "本部位唯一正式交互视觉层 ID。运行时只在 Part 的直接 typed Sprite/Flipbook 子组件中精确解析；该层的 authored Idle Sprite 或 Idle Flipbook 第一帧提供稳定轮廓碰撞。"))
	FName InteractionVisualLayerId = TEXT("Main");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Identity",
		meta = (ToolTip = "由 EnemyDefinition 对应 PartDefinition 派生的只读 PartId；请填写 PartSlotId 后使用 Host Details 同步。"))
	FName PartId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Animation",
		meta = (ToolTip = "此部位的语义行动动画 Style。精确驱动 TargetVisualLayerId；可选 EnemyDestroyedClip 负责整只敌人的终态。"))
	TObjectPtr<UWacomBattleEnemyPartAnimationStyle> PartAnimationStyle = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Destroyed",
		meta = (ToolTip = "Destroyed Cue 达到此归一化进度时原地切换子视觉层破损资源。推荐 0.35；合法范围固定为 0–1。"))
	float DestroyedVisualSwapNormalizedTime = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Feedback|Impact",
		meta = (ToolTip = "是否播放本部位 TargetConfirmed、Damage 与 Destroyed 世界反馈。关闭后仍消费 Cue 并应用破损视觉。"))
	bool bEnableImpactFeedback = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Feedback|Impact",
		meta = (ToolTip = "本部位命中特效 Style override。为空时使用 Host DefaultImpactStyle。"))
	TObjectPtr<UWacomBattleEnemyPartImpactStyle> ImpactStyleOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Feedback|Target Preview",
		meta = (ToolTip = "是否播放拖卡目标预演。关闭后目标验证与预测数据仍保持。"))
	bool bEnableTargetPreviewFeedback = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Feedback|Target Preview",
		meta = (ToolTip = "本部位拖卡目标预演 Style override。为空时使用 Host DefaultTargetPreviewStyle。"))
	TObjectPtr<UWacomBattleEnemyPartTargetPreviewStyle> TargetPreviewStyleOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Feedback",
		meta = (ToolTip = "确认、伤害和破坏 Cue 的默认保持时间，单位秒；建议 0.05–0.5 秒。"))
	float CueHoldSeconds = 0.14f;

	FName GetEffectivePartSlotId() const { return PartSlotId; }
	FName GetEffectivePartDefinitionId() const { return PartId; }
	FName GetStableSceneTargetId() const;
	AWacomBattleEnemyActor* GetOwningEnemyHost() const;

	/** Editor authoring service 的非反射派生身份写入口。 */
	void SetDerivedPartId(FName InPartId) { PartId = InPartId; }

	UWacomBattleEnemyPartImpactStyle* ResolveImpactStyle() const;
	UWacomBattleEnemyPartTargetPreviewStyle* ResolveTargetPreviewStyle() const;

	virtual FWacomInteractionTargetHandle BuildWorldTargetHandle() const override;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Debug",
		meta = (ToolTip = "读取此真实 Part 组件的运行时绑定与 typed 子组件诊断；只读。"))
	FWacomBattleEnemyPartRuntimeDebugView GetRuntimeDebugView() const;

	/** typed Visual/Anchor 注册变化时通知 Host runtime；不供 Blueprint 调用。 */
	void NotifyTypedChildTopologyChanged();

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
};
