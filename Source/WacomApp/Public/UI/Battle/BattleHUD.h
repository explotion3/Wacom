// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "UI/Battle/WacomBattleEventPresentationBuilder.h"
#include "Types/WacomEnums.h"
#include "BattleHUD.generated.h"

class UWacomBattleWidgetBase;
class UCanvasPanel;
class UCardWidget;
class UBattleEventLogPanel;
class UWacomCardDetailPanel;
class AWacomBattle3DHandPresenter;
class AWacomBattleCardVisualActor;
class APlayerController;
class UWacomBattlePresentationTargetComponent;
class FWacomBattlePresentationTargetRegistry;
struct FBattleHUDFallbackLayoutBuilder;
struct FWacomBattleHUDCommandFlow;
struct FWacomBattleHUDEventFlow;
struct FWacomBattleHUDTargetingFlow;
class FWacomBattleEventPresentationQueue;
struct FBattleCommand;
struct FWacomBattlePresentationTargetCue;

/** 战斗结束时的原生委托。参数为战斗结果。 */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnBattleEndedNative, EBattleOutcome);

/**
 * 单个敌方部位的目标选择表现视图。
 *
 * DisabledReason 约定：
 * - None：当前可选或无禁用原因。
 * - NotTargetSelecting：HUD 当前不在目标选择状态。
 * - PartDestroyed：该敌方部位已破坏。
 */
USTRUCT(BlueprintType)
struct WACOMAPP_API FBattleTargetablePartView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Targeting")
	FGuid PartInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Targeting")
	FName PartId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Targeting")
	FText PartName;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Targeting")
	bool bTargetable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Targeting")
	FName DisabledReason = NAME_None;
};

/**
 * 当前 BattleHUD 目标选择状态的只读表现桥。
 *
 * 临时 UEnemyPartWidget 和未来 HD-2D/PaperZD 敌方部位表现都应读取本视图，
 * 再把点击意图回传到 BattleHUD，而不是各自解析 HUD 内部状态。
 */
USTRUCT(BlueprintType)
struct WACOMAPP_API FBattleTargetSelectionView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Targeting")
	bool bIsTargetSelecting = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Targeting")
	FGuid PendingCardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Targeting")
	TArray<FBattleTargetablePartView> TargetableParts;
};

/**
 * 战斗 UI 根 Widget。
 *
 * 状态机（EBattleUIState）驱动所有子 Widget 的交互模式。
 * 命令提交的唯一入口：子 Widget 发委托给 HUD，HUD 统一调 Session->SubmitCommand。
 *
 * 交互流程：
 *   Idle
 *     ├── 点击 UCardWidget → HUD 判断 TargetMode
 *     │     ├── None / Self → 直接提交 PlayCard（空目标），回 Idle
 *     │     └── SingleEnemyPart → 切 TargetSelect，记录 Pending 卡
 *     ├── 点击 Wait 按钮 → 提交 Wait，回 Idle
 *     └── 点击 EndTurn 按钮 → 提交 EndTurn，回 Idle
 *
 *   TargetSelect
 *     ├── 点击 UEnemyPartWidget → 提交 PlayCard(PendingCard, PartId)，回 Idle
 *     └── 右键 / ESC → 取消，回 Idle
 *
 *   Resolving：命令已提交，等待动画/表现完成。当前同步结算时通常很短。
 *
 *   BattleEnd：战斗结束，显示胜利/失败面板。
 *
 * WBP 子类约定（BindWidget 大部分可选）：
 * - PlayerStatusBar   : UPlayerStatusBar
 * - HandPanel         : UHandPanel
 * - EnemyInfoBar      : UEnemyInfoBar
 * - ActionPanel       : UActionPanel
 * - DrawPileView      : UDrawPileView
 * - DiscardPileView   : UDiscardPileView
 * - EquipmentBar      : UEquipmentBar
 * - EventToast        : UEventToast
 */
UENUM(BlueprintType)
enum class EBattleUIState : uint8
{
	Idle,
	TargetSelect,
	Resolving,
	BattleEnd,
};

UCLASS(Blueprintable)
class WACOMAPP_API UBattleHUD : public UWacomBattleWidgetBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI|Layout", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "2400.0", ToolTip = "C++ fallback BattleHUD 中手牌面板的宽高。只影响未用完整 BattleHUD WBP 覆盖布局时的默认 CanvasPanel Slot 尺寸。"))
	FVector2D HandPanelSize = FVector2D(1700.0f, 420.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI|Layout", meta = (UIMin = "-400.0", UIMax = "400.0", ToolTip = "C++ fallback BattleHUD 中手牌面板相对屏幕底部的上移距离。正数会让手牌面板离底部更远。"))
	float HandPanelBottomOffset = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI|CardDetail", meta = (ClampMin = "1.0", UIMin = "120.0", UIMax = "900.0", ToolTip = "战斗手牌悬浮详情面板的估算宽高，单位为 Slate 像素。用于 CanvasPanel Slot 尺寸和边界 clamp；实际样式仍由 WBP_CardDetailPanel 决定。"))
	FVector2D CardDetailPanelEstimatedSize = FVector2D(360.0f, 420.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI|CardDetail", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "80.0", ToolTip = "战斗手牌悬浮详情面板与卡牌之间的间距，单位为 Slate 像素。面板默认显示在卡牌左侧，左侧空间不足时换到右侧。"))
	float CardDetailPanelPadding = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI|BattleEventLog", meta = (ClampMin = "1", UIMin = "10", UIMax = "300", ToolTip = "BattleHUD 内部保存的战斗事件日志最大条数。超过后只保留最近 N 条，并同步到日志抽屉。"))
	int32 BattleEventLogMaxEntries = 80;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI|3D Hand Prototype", meta = (ToolTip = "是否启用 CardActor + WidgetComponent 的 3D 手牌原型。默认关闭；开启后 BattleHUD 会在有战斗 Session 时创建 3D 手牌 Presenter，并继续保留现有 2D HandPanel 和 hover 详情。"))
	bool bEnable3DHandPrototype = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI|3D Hand Prototype", meta = (ToolTip = "3D 手牌原型的 Presenter Actor 类。仅在 bEnable3DHandPrototype 开启且 BattleHUD 持有有效战斗 Session 时生成；负责管理 3D 手牌 Actor 的表现和点击转发。"))
	TSubclassOf<AWacomBattle3DHandPresenter> Battle3DHandPresenterClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI|3D Hand Prototype", meta = (ToolTip = "3D 手牌原型使用的单张卡牌 Actor 类。BattleHUD 只把该类交给 Presenter，不直接生成或管理单卡 Actor。"))
	TSubclassOf<AWacomBattleCardVisualActor> Battle3DCardActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|UI|Scene Enemy Target Prototype", meta = (ToolTip = "是否启用场景敌方目标表现绑定原型。默认关闭；开启后 BattleHUD 会按当前 BattleSnapshot 的 UEnemyPartDefinition::PartId，把场景中的 UWacomBattlePresentationTargetComponent 自动绑定到运行时 PartInstanceId。"))
	bool bEnableSceneEnemyTargetBindingPrototype = false;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|UI")
	EBattleUIState GetUIState() const { return UIState; }

	static FVector2D ComputeCardDetailPanelPositionBeside(
		const FVector2D& AnchorPosition,
		const FVector2D& AnchorSize,
		const FVector2D& LayerSize,
		const FVector2D& PanelSize,
		float Padding);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|UI")
	bool IsCardDetailPanelVisible() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|UI")
	FText GetCardDetailPanelNameText() const;

	// ---- 子 Widget 交互入口 ----
	// 子 Widget 通过这些方法通知 HUD 玩家意图。HUD 按状态机决策。

	/**
	 * 某张手牌被点击。
	 * - TargetMode == None / Self：立即提交 PlayCard
	 * - TargetMode == SingleEnemyPart：进入 TargetSelect 状态
	 * - 其他：当前不支持，忽略
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|UI")
	void OnCardClickedByUser(const FGuid& CardInstanceId);

	/**
	 * TargetSelect 状态下玩家点击了某个敌方部位。
	 * 提交 PlayCard(PendingCardId, PartId)，回到 Idle。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|UI")
	void OnEnemyPartClickedByUser(const FGuid& PartInstanceId);

	/** 等待按钮点击。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|UI")
	void OnWaitRequested();

	/** 结束回合按钮点击。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|UI")
	void OnEndTurnRequested();

	/** 取消目标选择（ESC、右键、再次点同一张牌等）。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|UI")
	void CancelTargetSelect();

	/**
	 * 击倒事件 dialog 的选择入口（GDD §6）。
	 * dialog 不直接调 Session->SubmitCommand，而是走这里，
	 * 让 HUD 在提交后统一调 AfterCommand（事件消费 + Snapshot 刷新 + BattleEnd 广播）。
	 * 与 OnWaitRequested / OnEndTurnRequested 对称。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|UI")
	void OnKnockdownChoiceSelected(EKnockdownChoice Choice);

	/**
	 * 战斗结束时广播（保证只广播一次）。
	 * GameMode 绑定这个来触发 ExitBattle。
	 */
	FOnBattleEndedNative OnBattleEndedNative;

	// ---- 状态机查询（供子 Widget 做视觉反馈）----

	/** 当前是否正在选目标。UI 可据此高亮可选敌方部位。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|UI")
	bool IsInTargetSelect() const { return UIState == EBattleUIState::TargetSelect; }

	/** 当前等待目标的卡 ID。IsInTargetSelect == false 时返回 invalid。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|UI")
	FGuid GetPendingTargetingCardId() const { return PendingTargetingCardId; }

	/** 构建当前目标选择表现视图。UI/场景表现只读消费，不修改战斗状态。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Targeting")
	FBattleTargetSelectionView BuildTargetSelectionView() const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|EventLog")
	void ToggleBattleEventLog();

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|EventLog")
	void SetBattleEventLogOpen(bool bOpen);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|EventLog")
	bool IsBattleEventLogOpen() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|EventLog")
	int32 GetBattleEventLogEntryCount() const { return BattleEventLogHistory.Num(); }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|UI")
	bool IsBattlePresentationBusy() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeRefreshFromSnapshot(const FBattleSnapshot& Snap) override;
	virtual void NativeOnSessionChanged(class UBattleSession* OldSession, class UBattleSession* NewSession) override;

	/** 告诉 CommonUI：本 HUD 希望鼠标可见 + 游戏输入透传（键盘快捷键仍工作）。 */
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	/**
	 * 子类 override 可以在状态切换时做额外处理。
	 * 默认空实现。WBP 可以通过 BP_OnUIStateChanged 做表现反馈。
	 */
	virtual void NativeOnUIStateChanged(EBattleUIState OldState, EBattleUIState NewState);

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Battle|UI", DisplayName = "On UI State Changed")
	void BP_OnUIStateChanged(EBattleUIState OldState, EBattleUIState NewState);

	// ---- BindWidget ----

	/** 玩家状态条。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UPlayerStatusBar> PlayerStatusBar;

	/** 手牌面板。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UHandPanel> HandPanel;

	/** 敌人信息条。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UEnemyInfoBar> EnemyInfoBar;

	/** 操作面板。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UActionPanel> ActionPanel;

	/** 装备条。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UEquipmentBar> EquipmentBar;

	/** 抽牌堆计数。PileCountView 不是 BattleWidget，Refresh 时手动更新。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UPileCountView> DrawPileView;

	/** 弃牌堆计数。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UPileCountView> DiscardPileView;

	/** 消耗区计数。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UPileCountView> ExhaustPileView;

	/** 事件 Toast。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UEventToast> EventToast;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBattleEventLogPanel> EventLogPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> CardDetailLayer;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|UI|CardDetail")
	TSubclassOf<UWacomCardDetailPanel> CardDetailPanelClass;

private:
	EBattleUIState UIState = EBattleUIState::Idle;

	/** TargetSelect 状态下待确认目标的卡实例 ID。 */
	FGuid PendingTargetingCardId;

	/** 战斗结束回调是否已广播过。保证只广播一次。 */
	bool bHasBroadcastBattleEnd = false;

	UPROPERTY(Transient)
	TObjectPtr<UWacomCardDetailPanel> CardDetailPanel;

	UPROPERTY(Transient)
	TArray<FBattleEventPresentationView> BattleEventLogHistory;

	TSharedPtr<FWacomBattleEventPresentationQueue> BattleEventPresentationQueue;
	TSharedPtr<FWacomBattlePresentationTargetRegistry> BattlePresentationTargetRegistry;

	UPROPERTY(Transient)
	TObjectPtr<AWacomBattle3DHandPresenter> Battle3DHandPresenter;

	TWeakObjectPtr<UCardWidget> CurrentCardDetailSource;
	bool bLoggedMissingCardDetailLayer = false;
	bool bHasSavedPlayerControllerInteractionEventState = false;
	bool bSavedPlayerControllerClickEvents = false;
	bool bSavedPlayerControllerMouseOverEvents = false;
	int32 PlayerControllerClickEventAcquireCount = 0;
	int32 PlayerControllerMouseOverEventAcquireCount = 0;
	TWeakObjectPtr<APlayerController> SavedPlayerControllerForInteractionEvents;

	/** 内部状态切换入口，同时触发 Native + BP 钩子。 */
	void SetUIState(EBattleUIState NewState);

	/** 内部：提交 PlayCard 命令 + 事件消费 + 刷新。 */
	void SubmitPlayCard(const FGuid& CardId, const FGuid& TargetPartId);

	/** 内部：消费 Session 事件并分发给 Toast / 日志抽屉 / 击倒 dialog。 */
	void ConsumeAndLogEvents();

	void AppendBattleEventLogEntries(const TArray<struct FBattleEvent>& Events);
	void TrimBattleEventLogHistory();
	void SyncBattleEventLogPanel();
	void EnqueueBattlePresentationEvents(const TArray<struct FBattleEvent>& Events);
	void ClearBattlePresentationQueue();
	bool IsBattlePresentationQueueBusy() const;
	TSharedPtr<FWacomBattleEventPresentationQueue> GetBattlePresentationQueueSelfKeepAlive() const;
	FWacomBattlePresentationTargetRegistry& GetBattlePresentationTargetRegistry();
	void ClearBattlePresentationTargetRegistry();
	void RegisterBattlePresentationTarget(
		const FGuid& PartInstanceId,
		UObject* Owner,
		TFunction<void(const FWacomBattlePresentationTargetCue&)> Handler);
	void UnregisterBattlePresentationTargetsForOwner(const UObject* Owner);
	bool IsBattlePresentationTargetRegisteredForOwner(const UObject* Owner) const;
	void PlayBattlePresentationCue(const FWacomBattlePresentationTargetCue& Cue);
	void EnqueueBattlePresentationToast(const FBattleEventPresentationView& View);
	void PushPendingKnockdownChoiceDialog();
	void HandleBattlePresentationQueueStarted();
	void HandleBattlePresentationQueueFinished();
	void HandleBattlePresentationBattleEndStep();
	void AdvanceBattlePresentationQueueOnce();

#if WITH_AUTOMATION_TESTS
	void PlayBattlePresentationCueForTest(EBattleEventType SourceEventType, const FGuid& TargetPartInstanceId, int32 Amount);
	int32 GetBattlePresentationTargetCountForTest() const;
#endif

	UFUNCTION()
	void HandleBattleEventLogButtonClicked();

	/** 内部：提交命令后的通用收尾（刷新 + 战斗结束检测）。 */
	void AfterCommand();

	void HandleHandCardHovered(UCardWidget* SourceWidget);
	void HandleHandCardUnhovered(UCardWidget* SourceWidget);
	bool ShowCardDetailForCardWidget(UCardWidget* SourceWidget);
	void HideCardDetailPanel();
	void HideCardDetailPanelForSource(UCardWidget* SourceWidget);
	UWacomCardDetailPanel* EnsureCardDetailPanel();
	void EnsureCardDetailLayer();
	void PositionCardDetailPanelNear(UCardWidget* SourceWidget);
	AWacomBattle3DHandPresenter* EnsureBattle3DHandPresenter();
	void DestroyBattle3DHandPresenter();
	void SyncBattle3DHandPresenterTargeting();
	void AcquirePlayerControllerClickEvents();
	void ReleasePlayerControllerClickEvents();
	void AcquirePlayerControllerMouseOverEvents();
	void ReleasePlayerControllerMouseOverEvents();
	void ReleaseAllPlayerControllerInteractionEvents();
	void SyncSceneEnemyPresentationTargets(const FBattleSnapshot& Snap);
	void UnregisterSceneEnemyPresentationTargets(bool bOnlyAutoBoundTargets = true);

	friend struct FBattleHUDFallbackLayoutBuilder;
	friend struct FWacomBattleHUDCommandFlow;
	friend struct FWacomBattleHUDEventFlow;
	friend struct FWacomBattleHUDTargetingFlow;
	friend class FWacomBattleEventPresentationQueue;
	friend class UEnemyInfoBar;
	friend class UWacomBattlePresentationTargetComponent;
	friend class UWacomBattleHUDDetailTest;
};
