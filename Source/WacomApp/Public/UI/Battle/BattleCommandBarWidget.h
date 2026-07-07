// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/BattleCommandBarTypes.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "UI/Foundation/WacomButtonBase.h"
#include "BattleCommandBarWidget.generated.h"

class UImage;
class UPanelWidget;
class UTextBlock;
class UWidget;

UCLASS(Blueprintable, meta = (ToolTip = "Battle CommandBar 的单个命令按钮。只显示一个命令并广播 UI 意图，不直接提交 BattleSession。"))
class WACOMAPP_API UWacomBattleCommandButtonWidget : public UWacomButtonBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Command Button", meta = (ToolTip = "应用命令按钮 ViewData。只刷新 UI 文案、图标、可用性和 pending 表现。"))
	void SetCommandView(const FWacomBattleCommandButtonView& InView);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Command Button", meta = (ToolTip = "返回该按钮当前显示的命令 ViewData。只读缓存。"))
	FWacomBattleCommandButtonView GetCommandView() const { return CurrentView; }

	UPROPERTY(BlueprintAssignable, Category = "Wacom|Battle|Command Button")
	FWacomBattleCommandButtonClickedSignature OnCommandButtonClicked;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnClicked() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IconImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InputHintText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> PendingIndicator;

private:
	UPROPERTY(Transient)
	FWacomBattleCommandButtonView CurrentView;
};

UCLASS(Blueprintable, meta = (ToolTip = "BattleHUD 的被动命令条。由 runtime presenter 推送 ViewData，只广播 Wait / EndTurn 等玩家意图。"))
class WACOMAPP_API UBattleCommandBarWidget : public UWacomBattleWidgetBase
{
	GENERATED_BODY()

public:
	UBattleCommandBarWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Command Bar|Authoring", meta = (ToolTip = "CommandBar 生成命令按钮时使用的 Widget 类。应继承 UWacomBattleCommandButtonWidget；为空时回退到 C++ 默认按钮。"))
	TSubclassOf<UWacomBattleCommandButtonWidget> CommandButtonWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Command Bar|Authoring", meta = (ToolTip = "等待按钮的图标 Brush。配置后会写入 WaitButton 的 IconImage；留空时隐藏图标。"))
	FSlateBrush WaitIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Command Bar|Authoring", meta = (ToolTip = "结束回合按钮的图标 Brush。配置后会写入 EndTurnButton 的 IconImage；留空时隐藏图标。"))
	FSlateBrush EndTurnIconBrush;

	UPROPERTY(BlueprintAssignable, Category = "Wacom|Battle|Command Bar")
	FWacomBattleCommandRequestedSignature OnBattleCommandRequested;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Command Bar", meta = (ToolTip = "应用 CommandBar ViewData。只刷新按钮、等待值和 pending 文案，不提交战斗命令。"))
	void SetCommandBarViewData(const FWacomBattleCommandBarViewData& InViewData);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Command Bar", meta = (ToolTip = "返回当前 CommandBar ViewData 只读缓存。"))
	FWacomBattleCommandBarViewData GetCurrentViewData() const { return CurrentViewData; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Command Bar", meta = (ToolTip = "查找指定命令的按钮 ViewData。用于 WBP 表现或自动化验证。"))
	bool FindCommandButtonView(EWacomBattleCommandId CommandId, FWacomBattleCommandButtonView& OutView) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Command Bar", meta = (ToolTip = "指定命令当前是否可用。不可见或不存在时返回 false。"))
	bool IsCommandEnabled(EWacomBattleCommandId CommandId) const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Command Bar", meta = (ToolTip = "请求执行指定命令。只广播 UI 意图；真正命令提交仍由 BattleHUD / runtime 处理。"))
	void RequestCommand(EWacomBattleCommandId CommandId);

	const TArray<TObjectPtr<UWacomBattleCommandButtonWidget>>& GetGeneratedCommandButtons() const
	{
		return CommandButtons;
	}

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Command Bar", meta = (ToolTip = "当前 CommandBar 是否使用 WBP 里直接绑定的 WaitButton / EndTurnButton。为 true 时不会从 CommandButtonContainer 动态生成按钮。"))
	bool UsesAuthoredCommandButtons() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> CommandButtonContainer;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomBattleCommandButtonWidget> WaitButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomBattleCommandButtonWidget> EndTurnButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WaitValueText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PendingText;

private:
	UPROPERTY(Transient)
	FWacomBattleCommandBarViewData CurrentViewData;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWacomBattleCommandButtonWidget>> CommandButtons;

	UFUNCTION()
	void HandleCommandButtonClicked(EWacomBattleCommandId CommandId);

	void ApplyAuthoringIconBrushes(FWacomBattleCommandBarViewData& ViewData) const;
	void BindCommandButton(UWacomBattleCommandButtonWidget* Button);
	void ClearGeneratedCommandButtons();
	void ApplyAuthoredCommandButtons();
	void RebuildCommandButtons();
	UWacomBattleCommandButtonWidget* CreateCommandButtonWidget();
};
