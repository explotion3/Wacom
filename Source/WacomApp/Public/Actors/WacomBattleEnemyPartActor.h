// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Components/WacomBattleEnemyPartWorldTargetBridgeComponent.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
#include "WacomBattleEnemyPartActor.generated.h"

class UStaticMesh;
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

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleSceneEnemyPartDebugView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FString ActorName;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FName PartId = NAME_None;

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
	bool bInteractionTargetConfigured = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FGuid InteractionTargetId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FName InteractionTargetStableId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FWacomBattleEnemyPartWorldTargetDebugView BridgeDebugView;
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
		meta = (ToolTip = "默认 cue 保持时间，单位秒。", ClampMin = "0.01", ClampMax = "2.0", UIMin = "0.05", UIMax = "0.5"))
	float CueHoldSeconds = 0.14f;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Part")
	UBoxComponent* GetHitBounds() const { return HitBounds; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Part")
	UStaticMeshComponent* GetPartVisual() const { return PartVisual; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Part")
	UWacomInteractionTargetComponent* GetInteractionTargetComponent() const { return InteractionTargetComponent; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Part")
	UWacomBattleEnemyPartWorldTargetBridgeComponent* GetWorldTargetBridgeComponent() const
	{
		return WorldTargetBridgeComponent;
	}

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Scene Enemy|Authoring",
		meta = (ToolTip = "把 Actor facade 字段同步到内部命中体、可见体、InteractionTarget 和 Battle Part Bridge。"))
	void RefreshAuthoringState();

	UFUNCTION(CallInEditor, Category = "Wacom|Battle|Scene Enemy|Debug")
	void ConfigureDebugSnakeHeadSample();

	UFUNCTION(CallInEditor, Category = "Wacom|Battle|Scene Enemy|Debug")
	void ConfigureDebugSnakeBodySample();

	UFUNCTION(CallInEditor, Category = "Wacom|Battle|Scene Enemy|Debug")
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
		const FVector& InHitBoundsExtent,
		const FVector& InVisualScale,
		const FVector& InVisualRelativeLocation);
	bool HasDuplicatePartIdInWorld() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Part",
		meta = (AllowPrivateAccess = "true", ToolTip = "部位鼠标点击和拖卡命中的 Visibility 盒体。不要直接编辑 Collision Details，请改 Actor facade 字段。"))
	TObjectPtr<UWacomBattleEnemyPartHitBoundsComponent> HitBounds = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Part",
		meta = (AllowPrivateAccess = "true", ToolTip = "部位原型可见体。正式美术可在 Blueprint 或子类里替换。"))
	TObjectPtr<UWacomBattleEnemyPartVisualComponent> PartVisual = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Part",
		meta = (AllowPrivateAccess = "true", ToolTip = "部位默认携带的通用交互目标身份组件。"))
	TObjectPtr<UWacomInteractionTargetComponent> InteractionTargetComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Part",
		meta = (AllowPrivateAccess = "true", ToolTip = "把稳定 PartId 绑定到当前战斗运行时部位 ID，并接收目标 cue。"))
	TObjectPtr<UWacomBattleEnemyPartWorldTargetBridgeComponent> WorldTargetBridgeComponent = nullptr;
};
