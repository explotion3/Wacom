// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/WacomButtonBase.h"
#include "UI/Foundation/WacomMenuWidgetBase.h"
#include "WacomMainMenuScreen.generated.h"

class UTextBlock;
class UWidget;
struct FWacomMainMenuScreenTestAccess;

/**
 * 主菜单导航按钮的 CommonUI 制作入口。
 *
 * 正式 WBP 应继承本类；C++ fallback 也使用同一个类型，确保鼠标、键盘和手柄
 * 始终经过 CommonUI 的焦点与交互状态机。
 */
UCLASS(Blueprintable, meta = (ToolTip = "主菜单导航按钮的 CommonUI 基类。WBP_MainMenuNavButton 应继承本类，只负责按钮视觉、焦点反馈和音效，不提交菜单 Action。"))
class WACOMAPP_API UWacomMainMenuButtonWidget : public UWacomButtonBase
{
	GENERATED_BODY()

public:
	UWacomMainMenuButtonWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
};

/** 主菜单能够上报给 App flow 的玩家意图。 */
UENUM(BlueprintType)
enum class EWacomMainMenuAction : uint8
{
	ContinueJourney,
	StartNewJourney,
	JourneyHistory,
	Settings,
	Credits,
	Quit
};

/** 主菜单的只读显示数据；不包含存档对象或可写 Run 状态。 */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomMainMenuViewData
{
	GENERATED_BODY()

	/** 当前玩家档案中是否存在一个活动旅程。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Main Menu", meta = (ToolTip = "当前玩家档案中是否存在一个活动旅程。它只控制主菜单显示，不允许 Screen 自行读取存档。"))
	bool bHasActiveJourney = false;

	/** 当前活动旅程是否通过完整校验、可以继续。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Main Menu", meta = (ToolTip = "当前活动旅程是否可以继续。存在但不可读取的旅程可以显示 Continue，但按钮保持禁用。"))
	bool bCanContinueJourney = false;

	/** 右侧摘要区域显示的活动旅程标题。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Main Menu", meta = (ToolTip = "右侧摘要区域显示的活动旅程标题；没有活动旅程时由 Screen 使用默认启程文案。"))
	FText ActiveJourneyTitle;

	/** 右侧摘要区域显示的活动旅程摘要。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Main Menu", meta = (ToolTip = "右侧摘要区域显示的活动旅程摘要；只用于表现，不作为恢复旅程的数据来源。"))
	FText ActiveJourneySummary;

	/** 旅程记录页面是否已经接入，可以显示入口。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Main Menu", meta = (ToolTip = "旅程记录页面是否已经接入。未接入时入口保持隐藏，避免出现无响应按钮。"))
	bool bShowJourneyHistory = false;

	/** 设置页面是否已经接入，可以显示入口。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Main Menu", meta = (ToolTip = "设置页面是否已经接入。未接入时入口保持隐藏，避免出现无响应按钮。"))
	bool bShowSettings = false;

	/** 制作人员页面是否已经接入，可以显示入口。 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Main Menu", meta = (ToolTip = "制作人员页面是否已经接入。未接入时入口保持隐藏，避免出现无响应按钮。"))
	bool bShowCredits = false;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FWacomMainMenuActionRequested, EWacomMainMenuAction);

#if WITH_AUTOMATION_TESTS
/** MainMenu Screen 的非反射自动化测试快照。 */
struct WACOMAPP_API FWacomMainMenuScreenAutomationTestView
{
	bool bContinueVisible = false;
	bool bContinueEnabled = false;
	bool bNewJourneyVisible = false;
	bool bJourneyHistoryVisible = false;
	bool bSettingsVisible = false;
	bool bCreditsVisible = false;
	bool bQuitVisible = false;
	bool bAutoRestoreFocus = false;
	FName DesiredFocusTargetName = NAME_None;
	FText SummaryTitle;
	FText SummaryBody;
};
#endif

/**
 * L_MainMenu 的被动顶层 Screen。
 *
 * 输入：由 AWacomMenuGameMode 注入 FWacomMainMenuViewData。
 * 输出：通过 OnActionRequestedNative 上报 EWacomMainMenuAction。
 *
 * Screen 不读取或删除存档，不负责关卡 travel，也不直接退出游戏。
 */
UCLASS(Blueprintable, meta = (ToolTip = "L_MainMenu 的被动顶层 Screen。只应用主菜单 ViewData 并上报玩家 Action；存档、确认、退出和 travel 由 AWacomMenuGameMode 负责。"))
class WACOMAPP_API UWacomMainMenuScreen : public UWacomMenuWidgetBase
{
	GENERATED_BODY()

public:
	UWacomMainMenuScreen(const FObjectInitializer& ObjectInitializer);

	/** 应用主菜单只读显示数据并刷新按钮、摘要和 WBP 表现。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Main Menu", meta = (ToolTip = "应用主菜单只读 ViewData。只刷新按钮、摘要和焦点候选，不读取或修改存档。"))
	void ApplyViewData(const FWacomMainMenuViewData& InViewData);

	/** 返回当前已应用的主菜单只读显示数据。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Main Menu", meta = (ToolTip = "返回当前已应用的主菜单 ViewData 缓存。它不是玩家档案或 Run 状态的写入口。"))
	FWacomMainMenuViewData GetMainMenuViewData() const { return ViewData; }

	/** 请求一个主菜单 Action；不可用或未开放的 Action 会被拒绝。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Main Menu", meta = (ToolTip = "请求一个主菜单 Action。Screen 只在当前 ViewData 允许时广播，实际流程由 AWacomMenuGameMode 处理。"))
	void RequestAction(EWacomMainMenuAction Action);

	/** App flow 监听的原生 Action 出口。 */
	FWacomMainMenuActionRequested OnActionRequestedNative;

#if WITH_AUTOMATION_TESTS
	FWacomMainMenuScreenAutomationTestView GetAutomationTestViewForTest() const;
	friend struct FWacomMainMenuScreenTestAccess;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	/** ViewData 应用后的 WBP 表现钩子；不能在这里提交存档、Run 或 travel 操作。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Main Menu", DisplayName = "On Main Menu View Data Applied", meta = (ToolTip = "主菜单 ViewData 应用后的 WBP 表现钩子。仅用于刷新视觉，不应提交存档、Run 或 travel 操作。"))
	void BP_OnMainMenuViewDataApplied(const FWacomMainMenuViewData& InViewData);

	UFUNCTION()
	void HandleContinueClicked();

	UFUNCTION()
	void HandleNewJourneyClicked();

	UFUNCTION()
	void HandleJourneyHistoryClicked();

	UFUNCTION()
	void HandleSettingsClicked();

	UFUNCTION()
	void HandleCreditsClicked();

	UFUNCTION()
	void HandleQuitClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomMainMenuButtonWidget> ContinueButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomMainMenuButtonWidget> NewJourneyButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomMainMenuButtonWidget> JourneyHistoryButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomMainMenuButtonWidget> SettingsButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomMainMenuButtonWidget> CreditsButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomMainMenuButtonWidget> QuitButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ActiveJourneyTitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ActiveJourneySummaryText;

private:
	void RefreshFromViewData();
	bool IsActionAvailable(EWacomMainMenuAction Action) const;

	UPROPERTY(Transient)
	FWacomMainMenuViewData ViewData;
};
