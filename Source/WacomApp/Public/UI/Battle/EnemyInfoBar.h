// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "EnemyInfoBar.generated.h"

class UEnemyPartWidget;
class UPanelWidget;
class UBattleHUD;

/**
 * 敌人信息条。动态生成 N 个 UEnemyPartWidget。
 * 每次 Refresh 全清全建，类似 HandPanel 的策略。
 *
 * C++ 内置默认外观：HorizontalBox 横排。
 *
 * WBP 约定：
 * - PartsContainer : UPanelWidget (BindWidget)
 */
UCLASS(Blueprintable, meta = (ToolTip = "Legacy 2D 敌方部位信息条。缺少 SceneEnemyHost / PartActor 时作为 fallback/debug 使用，不是 HD-2D 场景敌人的正式制作入口。"))
class WACOMAPP_API UEnemyInfoBar : public UWacomBattleWidgetBase
{
	GENERATED_BODY()

public:
	UEnemyInfoBar();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Enemy 2D Fallback|Compatibility", meta = (ToolTip = "legacy 2D 敌方部位 fallback 使用的单部位 Widget 类。正式场景敌人应优先使用 SceneEnemyHost + PartActor。"))
	TSubclassOf<UEnemyPartWidget> PartWidgetClass;

	bool TryGetPartWidgetCenterInViewport(
		const FGuid& PartInstanceId,
		FVector2D& OutWidgetPosition) const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeRefreshFromSnapshot(const struct FBattleSnapshot& Snap) override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> PartsContainer;

private:
	UFUNCTION()
	void HandlePartClicked(FGuid PartInstanceId);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UEnemyPartWidget>> SpawnedParts;

	UBattleHUD* FindOwningBattleHUD() const;
	void RegisterBattlePresentationTargets(UBattleHUD& HUD);
	void UnregisterBattlePresentationTargets();
	void ApplyTargetableFromHUDState();

	friend class UWacomBattleEnemyInfoBarTest;
};
