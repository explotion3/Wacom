// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Exploration/RunExplorationTypes.h"
#include "GameFramework/GameModeBase.h"
#include "Types/WacomEnums.h"
#include "GameFramework/WacomGameFlowTypes.h"
#include "UI/Menus/WacomJourneySummaryScreen.h"
#include "WacomGameMode.generated.h"

class UCharacterDefinition;
class UWacomJourneyDefinition;
class UBattleSession;
class UWacomBattleWidgetBase;
class UWacomExplorationHUD;
class UBattleHUD;
class ABattleTriggerActor;
class AWacomPlayerController;
class URunSession;
struct FWacomJourneySummaryGameModeTestAccess;
struct FRunExplorationResolution;

#if WITH_AUTOMATION_TESTS
/** Journey 总结交接的非反射自动化快照。 */
struct WACOMAPP_API FWacomJourneySummaryHandoffAutomationTestView
{
	bool bSuccessEventConsumed = false;
	bool bBarrierCompleted = false;
	bool bRunPresentationRestoreRequested = false;
	bool bSummaryPushAttempted = false;
	bool bSummaryPushSucceeded = false;
	bool bHandoffRequested = false;
	bool bPrimaryLayoutTeardownRequested = false;
	bool bPrimaryLayoutTeardownCompleted = false;
	bool bTravelScheduled = false;
	bool bTravelExecuted = false;
	bool bActualTravelSuppressed = false;
	int32 HandoffRequestCount = 0;
	int32 TeardownOrder = 0;
	int32 ScheduleOrder = 0;
	int32 ExecuteOrder = 0;
	FName TravelLevelName = NAME_None;
	FWacomJourneySummaryViewData ViewData;
};
#endif

/**
 * Wacom 游戏 GameMode。
 *
 * 职责：
 *   - 持有当前 EGameFlowState（Exploration / Battle）
 *   - EnterBattle：创建 BattleSession；通过 UIManager Push BattleHUD；切 IMC；禁用探索输入；记录触发器
 *   - ExitBattle：Pop BattleHUD；清 Session；恢复 IMC；恢复探索输入；真胜利时销毁触发器
 *   - 订阅 BattleHUD::OnBattleEndedNative，让战斗结束自动触发 ExitBattle
 *
 * UI 生命周期由 UWacomGameUIManagerSubsystem 管理；切关卡时会拆除并重建 PrimaryLayout。
 *
 * DefaultPawnClass = AWacomPlayerCharacter
 * PlayerControllerClass = AWacomPlayerController
 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWacomGameMode();

	// ---- 存档系统总开关 ----

	/**
	 * 存档系统总开关。
	 *
	 * 关闭后：
	 *   - Bootstrap 不读盘，直接走新 Run
	 *   - SaveRunToSlot / 战斗结束自动存档 静默 no-op
	 *   - PauseMenu Save 按钮隐藏
	 *   - MainMenu Continue 按钮永远禁用，New Game 不再弹 ConfirmDialog
	 *
	 * 底层 UWacomSaveGame / FRunState 拷贝 / 迁移机制不动，自动化测试照常跑。
	 */
	static constexpr bool bSaveSystemEnabled = false;

	// ---- 存档 Slot 名常量 ----

	/** 主存档 slot。玩家每次正常操作写入这里。 */
	static const FString SlotName_Main;

	/** 自动备份 slot。和 Main 同时写入，Main 损坏时回退。 */
	static const FString SlotName_Auto;

	// ---- 配置（默认通过 LoadObject 填好，蓝图/关卡可覆盖）----

	/** 新 Run bootstrap 使用的默认玩家角色配置；正式进入战斗时角色来自 RunSession / RunState。 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Battle")
	TObjectPtr<UCharacterDefinition> DefaultCharacter;

	/** 新 Run 使用的 Journey/Floor Logical Map Graph；规则初始化不从场景 Actor 反向生成图。 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Run Map",
		meta = (ToolTip = "新 Run 使用的 Journey 定义。必须配置有效 Floor 图；场景 Anchor/Path/Host 只映射这里的稳定 ID。"))
	TObjectPtr<UWacomJourneyDefinition> DefaultJourneyDefinition = nullptr;

	/** Deprecated legacy prototype field. 正式进入战斗时随机种子来自 RunSession / RunState。 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Battle",
		meta = (DeprecatedProperty,
			DeprecationMessage = "正式进入战斗时随机种子来自 RunSession / RunState。该字段已不参与战斗初始化，请不要在新资产中使用。"))
	int32 DefaultRandomSeed = 0;

	/**
	 * 战斗 HUD WBP，Push 到 Game Layer。
	 * PrimaryLayout 类由 UWacomGameUIManagerSubsystem 管理，不在这里配置。
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|UI")
	TSubclassOf<UWacomBattleWidgetBase> BattleHUDClass;

	/**
	 * 探索 HUD（ViewModel 驱动）。
	 * BeginPlay 时若蓝图未配，回退 C++ 父类 UWacomExplorationHUD。
	 * 蓝图子类（如 BP_GameMode）可在 Details 面板拖 WBP_ExplorationHUD 覆盖。
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|UI")
	TSubclassOf<UWacomExplorationHUD> ExplorationHUDClass;

	/** Journey 成功总结页；未配置 WBP 时回退原生 UWacomJourneySummaryScreen。 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|UI", meta = (ToolTip = "Journey 成功后 Push 到 GameMenu layer 的被动总结 Screen。正式 WBP 可继承 UWacomJourneySummaryScreen 覆盖；为空时使用原生 fallback。"))
	TSubclassOf<UWacomJourneySummaryScreen> JourneySummaryScreenClass;

	// ---- 状态 ----

	UFUNCTION(BlueprintPure, Category = "Wacom|GameFlow")
	EGameFlowState GetGameFlowState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle")
	UBattleSession* GetActiveBattleSession() const { return ActiveSession; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle")
	UBattleHUD* GetActiveBattleHUD() const { return BattleHUD; }

	/** Journey 总结确认后使用的稳定主菜单 package path。 */
	static FName GetJourneySummaryMainMenuLevelPackagePathForTravel();

#if WITH_AUTOMATION_TESTS
	FWacomJourneySummaryHandoffAutomationTestView GetJourneySummaryHandoffAutomationTestView() const;
	void SetSuppressJourneySummaryTravelForAutomation(bool bSuppress)
	{
		bSuppressJourneySummaryTravelForAutomation = bSuppress;
	}
	void FlushJourneySummaryTravelForAutomation()
	{
		ExecuteJourneySummaryMainMenuTravel();
	}
#endif

	// ---- 切换入口 ----

	/**
	 * 进入战斗。由 AWacomPlayerController::RequestEnterBattle 转发。
	 * Trigger 必须提供 EncounterDefinition；真胜利后被 Destroy，撤离时保留以支持重入。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|GameFlow")
	void EnterBattle(ABattleTriggerActor* Trigger);

	/**
	 * 退出战斗。战斗 UI 检测到 Phase == BattleEnd 时自动广播触发。
	 * 也可以由外部手动调用（例如玩家认输）。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|GameFlow")
	void ExitBattle(EBattleOutcome Outcome);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** BattleHUD::OnBattleEndedNative 回调。 */
	void HandleBattleEnded(EBattleOutcome Outcome);

	/**
	 * 启动时的存档引导：优先 Main → Auto → 新开 Run。
	 * 由 BeginPlay 末尾调用，因为此时 PlayerController 的 RunSession 已就位。
	 */
	void BootstrapRunFromSave();

	/** 存档到指定 slot，用到 PlayerController 的 RunSession。 */
	bool SaveRunToSlot(const FString& SlotName, bool bQuiet = false) const;

private:
	bool ConsumeJourneySucceededEvent(
		const FRunExplorationResolution& Resolution,
		const URunSession& RunSession);
	void HandleExitBattlePostRunBarrier(
		bool bJourneySucceeded,
		AWacomPlayerController* WacomPC);
	void ShowJourneySummaryOrTravel();
	void BindJourneySummaryScreen(UWacomJourneySummaryScreen& Screen);
	void UnbindJourneySummaryScreen();
	void HandleJourneySummaryContinueRequested();
	void RequestJourneySummaryMainMenuHandoff();
	void ExecuteJourneySummaryMainMenuTravel();

	UPROPERTY(VisibleInstanceOnly, Category = "Wacom|GameFlow", Transient)
	EGameFlowState CurrentState = EGameFlowState::Exploration;

	UPROPERTY(Transient)
	TObjectPtr<UBattleSession> ActiveSession = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UBattleHUD> BattleHUD = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ABattleTriggerActor> PendingTrigger = nullptr;

	/** 当前 Battle 唯一持有的 Encounter NodeActivity 票据；退出时必须提交或取消。 */
	TOptional<FRunNodeActivityTicket> PendingEncounterActivity;

	/** HUD::OnBattleEndedNative 的订阅句柄，ExitBattle 时反注册。 */
	FDelegateHandle BattleEndedHandle;

	TOptional<FWacomJourneySummaryViewData> PendingJourneySummaryViewData;

	UPROPERTY(Transient)
	TWeakObjectPtr<UWacomJourneySummaryScreen> ActiveJourneySummaryScreen;

	FName PendingJourneySummaryTravelLevelName = NAME_None;
	bool bJourneySummarySuccessEventConsumed = false;
	bool bJourneySummaryBarrierCompleted = false;
	bool bJourneySummaryRunPresentationRestoreRequested = false;
	bool bJourneySummaryPushAttempted = false;
	bool bJourneySummaryPushSucceeded = false;
	bool bJourneySummaryHandoffRequested = false;
	bool bJourneySummaryPrimaryLayoutTeardownRequested = false;
	bool bJourneySummaryPrimaryLayoutTeardownCompleted = false;
	bool bJourneySummaryTravelScheduled = false;
	bool bJourneySummaryTravelExecuted = false;
	bool bSuppressJourneySummaryTravelForAutomation = false;
	int32 JourneySummaryHandoffRequestCount = 0;
	int32 JourneySummaryTravelOrderCounter = 0;
	int32 JourneySummaryTeardownOrder = 0;
	int32 JourneySummaryScheduleOrder = 0;
	int32 JourneySummaryExecuteOrder = 0;

#if WITH_AUTOMATION_TESTS
	friend struct FWacomJourneySummaryGameModeTestAccess;
#endif
};
