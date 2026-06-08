// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WacomBattleEnemyHostVisualComponent.generated.h"

class UPaperFlipbook;
class UPaperFlipbookComponent;
class UPaperSprite;
class UPaperSpriteComponent;
class UMaterialInterface;
class USceneComponent;

/**
 * 生成并维护场景敌人 Host 的整体 PaperSprite / PaperFlipbook 视觉。
 *
 * 只负责 Host 整体视觉组件生命周期与统计；不拥有部位身份、HitBounds 或 Battle 目标绑定。
 */
UCLASS(ClassGroup = (Wacom), meta = (BlueprintSpawnableComponent, ToolTip = "生成并维护场景敌人 Host 的整体 PaperSprite / PaperFlipbook 视觉。"))
class WACOMAPP_API UWacomBattleEnemyHostVisualComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWacomBattleEnemyHostVisualComponent();

	void RefreshHostVisual(
		USceneComponent* AttachRoot,
		bool bUseFlipbook,
		UPaperSprite* HostSprite,
		UPaperFlipbook* HostFlipbook,
		const FVector& RelativeLocation,
		const FRotator& RelativeRotation,
		const FVector& RelativeScale3D,
		int32 SortOrder,
		const FLinearColor& Tint,
		UMaterialInterface* MaterialOverride,
		bool bCastShadow,
		bool bVisible,
		float FlipbookPlayRate,
		bool bLoopFlipbook,
		float FlipbookStartTimeSeconds,
		bool bAutoPlayFlipbook);
	void ClearGeneratedHostVisual();

	UPaperSpriteComponent* GetGeneratedHostSpriteVisualComponent() const
	{
		return GeneratedHostSpriteVisualComponent;
	}

	UPaperFlipbookComponent* GetGeneratedHostFlipbookVisualComponent() const
	{
		return GeneratedHostFlipbookVisualComponent;
	}

	int32 GetGeneratedHostVisualComponentCount() const;
	int32 GetRegisteredHostVisualComponentCount() const;
	int32 GetVisibleHostVisualComponentCount() const;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UPaperSpriteComponent> GeneratedHostSpriteVisualComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UPaperFlipbookComponent> GeneratedHostFlipbookVisualComponent = nullptr;
};
