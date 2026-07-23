// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
#include "WacomBattleEnemyActor.generated.h"

class UEnemyDefinition;
class USceneComponent;
class UWidgetComponent;
class UWacomBattleEnemyPanelWidget;
class UWacomBattleEnemyPartComponent;
class UWacomBattleEnemyPartImpactStyle;
class UWacomBattleEnemyPartTargetPreviewStyle;
class UWacomBattleEnemySceneRuntimeComponent;
struct FBattlePartSlotIdentity;
struct FWacomBattleEnemyPanelViewData;
struct FWacomBattleEnemyPartEntryViewData;

DECLARE_MULTICAST_DELEGATE_TwoParams(
	FWacomBattleEnemyHostInspectionRequestedNative,
	AWacomBattleEnemyActor*,
	const FBattlePartSlotIdentity&);

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
	FName AuthoringState = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	bool bAuthoringReady = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	bool bRuntimeEncounterPresentationRetired = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	int32 PartComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	TArray<FName> PartSlotIds;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	TArray<FName> PartIds;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	bool bUsedByBattleHUD = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Debug")
	FString ActiveBattleHUDName;
};

/**
 * 敌人场景表现的唯一 Host。
 *
 * 所有可见体、命中盒和反馈锚点都是 Host Blueprint 中的真实 typed Component；
 * Host 不拥有第二套 Sprite/Flipbook 制作数据，也不生成 ChildActor。
 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomBattleEnemyActor : public AActor
{
	GENERATED_BODY()

public:
	AWacomBattleEnemyActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Identity",
		meta = (ToolTip = "Host 对应的敌人定义。部位顺序、PartId 派生和 UI 顺序都以 EnemyDefinition.Parts 为唯一真相。"))
	TObjectPtr<UEnemyDefinition> EnemyDefinition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Identity",
		meta = (ToolTip = "Host 默认敌人槽位 ID。Encounter Node 的场景绑定组件在战斗准备时按 Encounter 槽位注入。"))
	FName EnemySlotId = TEXT("Enemy");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Presentation|Impact",
		meta = (ToolTip = "所有 Part 默认使用的世界命中反馈 Style；Part override 优先。"))
	TObjectPtr<UWacomBattleEnemyPartImpactStyle> DefaultImpactStyle = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Presentation|Target Preview",
		meta = (ToolTip = "所有 Part 默认使用的拖卡目标预演 Style；Part override 优先。"))
	TObjectPtr<UWacomBattleEnemyPartTargetPreviewStyle> DefaultTargetPreviewStyle = nullptr;

	UPROPERTY(EditAnywhere, Category = "Wacom|Battle|Scene Enemy|Presentation|Enemy Panel",
		meta = (ToolTip = "Host 专用敌人面板 WBP override。为空时按有效部位数量使用项目默认单部位或多部位面板。"))
	TSubclassOf<UWacomBattleEnemyPanelWidget> EnemyPanelWidgetClass;

	UPROPERTY(EditAnywhere, Category = "Wacom|Battle|Scene Enemy|Presentation|Enemy Panel",
		meta = (ToolTip = "敌人头顶聚合面板相对 Host 根节点的位置，单位厘米。"))
	FVector EnemyPanelRelativeLocation = FVector(0.0f, 0.0f, 180.0f);

	UPROPERTY(EditAnywhere, Category = "Wacom|Battle|Scene Enemy|Presentation|Enemy Panel",
		meta = (ToolTip = "关闭 Desired Size 时的固定绘制尺寸，单位 Slate 像素。"))
	FVector2D EnemyPanelDrawSize = FVector2D(360.0f, 220.0f);

	UPROPERTY(EditAnywhere, Category = "Wacom|Battle|Scene Enemy|Presentation|Enemy Panel",
		meta = (ToolTip = "是否让 WidgetComponent 使用 WBP Desired Size，并从底部中心向上增长。"))
	bool bEnemyPanelDrawAtDesiredSize = true;

	UPROPERTY(EditAnywhere, Category = "Wacom|Battle|Scene Enemy|Presentation|Enemy Panel",
		meta = (ToolTip = "是否在存在敌人数据时常驻显示紧凑面板；关闭后仅在 hover/preview 上下文显示。"))
	bool bEnemyPanelVisibleByDefault = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status|Last Sync")
	FName AuthoringLastPartSyncResult = TEXT("NotRun");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status|Last Sync")
	TArray<FName> AuthoringLastAddedPartSlotIds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status|Last Sync")
	TArray<FName> AuthoringLastUpdatedPartSlotIds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Wacom|Battle|Scene Enemy|Authoring Status|Last Sync")
	TArray<FName> AuthoringLastInvalidDefinitionPartSlotIds;

	FName GetEffectiveEnemySlotId() const { return EnemySlotId; }
	UWacomBattleEnemySceneRuntimeComponent* GetEnemySceneRuntimeComponent() const { return EnemySceneRuntimeComponent; }
	TArray<UWacomBattleEnemyPartComponent*> GetBattleEnemyPartComponents() const;
	void NotifyEnemySceneComponentTopologyChanged();
	uint32 GetEnemySceneComponentTopologyRevision() const;

	void ResetRuntimeScenePresentationForBattle();
	void RetireRuntimeEncounterPresentation();
	bool IsRuntimeEncounterPresentationRetired() const;

	void SetEnemyPanelViewData(const FWacomBattleEnemyPanelViewData& ViewData);
	void ClearEnemyPanelViewData();
	void SetEnemyPanelActionPreview(const TArray<FWacomBattleEnemyPartEntryViewData>& PreviewParts);
	void ClearEnemyPanelActionPreview();
	void SetEnemyPanelHoveredPart(FName PartSlotId);
	void SetEnemyPanelInspectionInteractionEnabled(bool bEnabled);
	bool IsEnemyPanelInspectionInteractionEnabled() const { return bEnemyPanelInspectionInteractionEnabled; }

	FWacomBattleEnemyHostInspectionRequestedNative OnEnemyPanelInspectionRequestedNative;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Debug")
	FWacomBattleSceneEnemyDebugView GetBattleSceneEnemyDebugView() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Debug")
	FWacomBattleSceneEnemyDebugView GetBattleSceneEnemyDebugViewForHUD(const class UBattleHUD* HUD) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy|Debug")
	FString GetBattleSceneEnemyDebugSummary() const;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Wacom|Battle|Scene Enemy|Debug")
	void LogBattleSceneEnemyDebugSummary() const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	void RefreshEnemyPanelWidgetComponent();
	void RefreshEnemyPanelVisibility();
	void BindEnemyPanelInspectionDelegate(UWacomBattleEnemyPanelWidget& PanelWidget);
	void HandleEnemyPanelInspectionRequested(const FBattlePartSlotIdentity& PartIdentity);

	bool bEnemyPanelHasViewData = false;
	bool bEnemyPanelHasActionPreview = false;
	bool bEnemyPanelInspectionInteractionEnabled = false;
	FName EnemyPanelHoveredPartSlotId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Internal",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Wacom|Battle|Scene Enemy|Internal")
	TObjectPtr<UWidgetComponent> EnemyPanelWidgetComponent = nullptr;

	UPROPERTY(VisibleAnywhere, Transient, Category = "Wacom|Battle|Scene Enemy|Internal")
	TObjectPtr<UWacomBattleEnemySceneRuntimeComponent> EnemySceneRuntimeComponent = nullptr;
};
