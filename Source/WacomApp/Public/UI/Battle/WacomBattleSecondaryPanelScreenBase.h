// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/WacomActivatableWidget.h"
#include "WacomBattleSecondaryPanelScreenBase.generated.h"

class UButton;
class UWidget;

/**
 * Battle 二级面板基类。
 *
 * 与菜单基类不同，本类保持 All + NoCapture，使镜头与后台表现继续运行；
 * Battle 命令门控由 HUD coordinator 独立持有。
 */
UCLASS(Abstract, Blueprintable)
class WACOMAPP_API UWacomBattleSecondaryPanelScreenBase : public UWacomActivatableWidget
{
	GENERATED_BODY()

public:
	UWacomBattleSecondaryPanelScreenBase(const FObjectInitializer& ObjectInitializer);

	DECLARE_MULTICAST_DELEGATE(FOnSecondaryPanelClosedNative);
	FOnSecondaryPanelClosedNative& OnSecondaryPanelClosedNative() { return SecondaryPanelClosedNative; }

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Secondary Panel",
		meta = (ToolTip = "关闭当前 Battle 二级面板。只处理 UI 生命周期，不提交战斗命令。"))
	void RequestClose();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BackdropButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> PanelRoot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

private:
	UFUNCTION()
	void HandleBackdropClicked();

	UFUNCTION()
	void HandleCloseClicked();

	void BroadcastClosedOnce();
	void RestoreGameViewportFocusNextTick();

	FOnSecondaryPanelClosedNative SecondaryPanelClosedNative;
	bool bClosedBroadcast = false;
};
