// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "EnemyInfoBar.generated.h"

class UEnemyPartWidget;
class UPanelWidget;

/**
 * 敌人信息条。动态生成 N 个 UEnemyPartWidget。
 * 每次 Refresh 全清全建，类似 HandPanel 的策略。
 *
 * C++ 内置默认外观：HorizontalBox 横排。
 *
 * WBP 约定：
 * - PartsContainer : UPanelWidget (BindWidget)
 */
UCLASS(Blueprintable)
class WACOMAPP_API UEnemyInfoBar : public UWacomBattleWidgetBase
{
	GENERATED_BODY()

public:
	UEnemyInfoBar();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI")
	TSubclassOf<UEnemyPartWidget> PartWidgetClass;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeRefreshFromSnapshot(const struct FBattleSnapshot& Snap) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> PartsContainer;

private:
	UFUNCTION()
	void HandlePartClicked(FGuid PartInstanceId);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UEnemyPartWidget>> SpawnedParts;

	void ApplyTargetableFromHUDState();

	friend class UWacomBattleEnemyInfoBarTest;
};
