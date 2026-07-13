// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/WacomMenuWidgetBase.h"
#include "WacomPauseMenuScreen.generated.h"

class UWacomMenuButtonWidget;

/**
 * 暂停菜单。ESC 打开，Push 到 GameMenu 层。
 *
 * 按钮：
 *   - Resume：Pop 自身
 *   - Save：RunSession->SaveToSlot(Main)
 *   - Quit to Main Menu：拆 UI 后在下一帧 OpenLevel(/Game/Wacom/Maps/L_MainMenu)
 *
 *   - Settings：通过共享 Settings Screen flow Push 同一个设置页
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomPauseMenuScreen : public UWacomMenuWidgetBase
{
	GENERATED_BODY()

public:
	UWacomPauseMenuScreen(const FObjectInitializer& ObjectInitializer);

	static FName GetMainMenuLevelPackagePathForTravel();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void HandleResumeClicked();

	UFUNCTION()
	void HandleSaveClicked();

	void HandleSettingsClicked();

	UFUNCTION()
	void HandleQuitToMenuClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomMenuButtonWidget> ResumeButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomMenuButtonWidget> SaveButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomMenuButtonWidget> SettingsButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomMenuButtonWidget> QuitToMenuButton;
};
