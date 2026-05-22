// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/WacomMenuWidgetBase.h"
#include "WacomPauseMenuScreen.generated.h"

class UButton;

/**
 * 暂停菜单。ESC 打开，Push 到 GameMenu 层。
 *
 * 按钮：
 *   - Resume：Pop 自身
 *   - Save：RunSession->SaveToSlot(Main)
 *   - Quit to Main Menu：OpenLevel(L_MainMenu)
 *
 * Settings 按钮后续按菜单需求接入。
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomPauseMenuScreen : public UWacomMenuWidgetBase
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleResumeClicked();

	UFUNCTION()
	void HandleSaveClicked();

	UFUNCTION()
	void HandleQuitToMenuClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ResumeButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SaveButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> QuitToMenuButton;
};
