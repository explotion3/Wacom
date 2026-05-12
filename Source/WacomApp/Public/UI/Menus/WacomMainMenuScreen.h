// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/WacomMenuWidgetBase.h"
#include "WacomMainMenuScreen.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class UCanvasPanel;

/**
 * 游戏主菜单。
 *
 * 由 AWacomMenuGameMode 在 L_MainMenu 的 BeginPlay 时 Push 到 GameMenu 层。
 *
 * 三个按钮：
 *   - New Game   ：无存档直开 L_Exploration；有存档 M3 起会弹 ConfirmDialog
 *   - Continue   ：HasSaveInSlot(Main) 为 true 才启用，点击开 L_Exploration（GameMode Bootstrap 负责读档）
 *   - Quit Game  ：ConsoleCommand("quit")
 *
 * 第一版 C++ 自建默认布局。策划给 UI 方案后，WBP 子类 override 即可。
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomMainMenuScreen : public UWacomMenuWidgetBase
{
	GENERATED_BODY()

public:
	/** 点 New Game 后要加载的关卡名。 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|UI")
	FName ExplorationLevelName = FName(TEXT("/Game/Wacom/Maps/L_Exploration.L_Exploration"));

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	// ---- 按钮回调（C++ 入口，WBP 子类可改为 bp 事件触发）----

	UFUNCTION()
	void HandleNewGameClicked();

	UFUNCTION()
	void HandleContinueClicked();

	UFUNCTION()
	void HandleQuitClicked();

	// BindWidgetOptional：WBP 子类如果有对应控件则绑定；C++ 默认布局走 RebuildWidget 创建
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> NewGameButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ContinueButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> QuitButton;

private:
	/** 刷新 Continue 按钮的启用状态——根据 Main slot 是否有存档。 */
	void RefreshContinueEnabled();
};
