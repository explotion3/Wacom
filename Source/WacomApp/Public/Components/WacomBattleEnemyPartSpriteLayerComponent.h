// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/WacomInteractionTargetProvider.h"
#include "PaperSpriteComponent.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
#include "WacomBattleEnemyPartSpriteLayerComponent.generated.h"

class UPaperSprite;
class UWacomBattleEnemyPartComponent;

/** 敌人部位可直接在 Host Blueprint 视口中制作的静态 Sprite 层。 */
UCLASS(ClassGroup = (Wacom), meta = (BlueprintSpawnableComponent,
	ToolTip = "敌人部位的真实 Sprite 视觉层。必须直接挂在 Enemy Part 组件下，可在 Host Blueprint 视口直接调整。"))
class WACOMAPP_API UWacomBattleEnemyPartSpriteLayerComponent : public UPaperSpriteComponent,
	public IWacomInteractionTargetProvider
{
	GENERATED_BODY()

public:
	UWacomBattleEnemyPartSpriteLayerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual Layer",
		meta = (ToolTip = "部位内稳定视觉层 ID，例如 Snake.Head.Main。运行时只按此 ID 精确寻址。"))
	FName LayerId = TEXT("Main");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual Layer|Destroyed",
		meta = (ToolTip = "本层可选破损终态 Sprite。为空时部位 Destroyed 后保持原资源。"))
	TObjectPtr<UPaperSprite> DestroyedSprite = nullptr;

	/** Runtime-only：把本层绑定为 Part 的正式交互层，并固定其碰撞源。 */
	void ConfigureInteractionCollision(
		UWacomBattleEnemyPartComponent* InPart,
		UPaperSprite* InStableCollisionSprite,
		bool bEnableCollision);
	void ClearInteractionCollision();
	bool IsInteractionCollisionReady() const;
	UPaperSprite* GetStableInteractionCollisionSprite() const;

	virtual FWacomInteractionTargetHandle BuildWorldTargetHandle() const override;
	virtual class UBodySetup* GetBodySetup() override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UWacomBattleEnemyPartComponent> InteractionPart = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UPaperSprite> StableInteractionCollisionSprite = nullptr;
};
