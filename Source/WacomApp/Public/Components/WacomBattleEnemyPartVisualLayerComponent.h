// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UI/Battle/WacomBattleEnemyPartVisualLayerTypes.h"
#include "WacomBattleEnemyPartVisualLayerComponent.generated.h"

class UPaperFlipbookComponent;
class UPaperSpriteComponent;
class USceneComponent;

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleEnemyPartVisualLayerDebugView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	bool bUsingVisualLayers = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	int32 VisualLayerCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	int32 GeneratedVisualLayerComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	int32 RegisteredVisualLayerComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	int32 VisibleVisualLayerComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	int32 GeneratedStaticVisualLayerComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	int32 GeneratedFlipbookVisualLayerComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	int32 RegisteredStaticVisualLayerComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	int32 RegisteredFlipbookVisualLayerComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	int32 VisibleStaticVisualLayerComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	int32 VisibleFlipbookVisualLayerComponentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	int32 MissingVisualLayerAssetCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	int32 MissingVisualLayerSpriteCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	int32 MissingVisualLayerFlipbookCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	TArray<FName> VisualLayerIds;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	TArray<FName> DuplicateVisualLayerIds;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Visual Layers|Debug")
	TArray<FName> VisualLayerAssetNames;
};

/**
 * 生成并维护敌人部位的 PaperSprite / PaperFlipbook 视觉层。
 *
 * 只负责表现组件生命周期与统计；不拥有命中体、Battle 身份或 HUD 命令。
 */
UCLASS(ClassGroup = (Wacom), meta = (BlueprintSpawnableComponent, ToolTip = "生成并维护场景敌人部位的 PaperSprite / PaperFlipbook 视觉层。"))
class WACOMAPP_API UWacomBattleEnemyPartVisualLayerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWacomBattleEnemyPartVisualLayerComponent();

	void RefreshVisualLayers(
		const TArray<FWacomBattleEnemyPartVisualLayer>& VisualLayers,
		USceneComponent* AttachRoot);
	void ClearGeneratedVisualLayers();
	FWacomBattleEnemyPartVisualLayerDebugView BuildVisualLayerDebugView(
		const TArray<FWacomBattleEnemyPartVisualLayer>& VisualLayers) const;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPaperSpriteComponent>> GeneratedVisualLayerComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPaperFlipbookComponent>> GeneratedFlipbookVisualLayerComponents;
};
