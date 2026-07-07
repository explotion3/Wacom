// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Events/BattleEvent.h"
#include "Resolution/BattleTargetValidationResult.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "Types/WacomEnums.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "BattleHUD.generated.h"

class UWacomBattleWidgetBase;
class UBattleCombatLogFeedWidget;
class UBattlePresentationStackWidget;
class UWacomCardDetailPanel;
class UWacomBattleEnemyPartPresentationComponent;
class UWacomBattleEnemyPartWorldTargetBridgeComponent;
class AWacomBattleEnemyActor;
class APlayerController;
class FWacomBattleHUDRuntime;
class FWacomBattleHUDRuntimeHost;
class UWacomFirstPersonCardAnchorComponent;
class FWacomBattlePresentationTargetRegistry;
struct FBattleHUDFallbackLayoutBuilder;
struct FBattleCommand;
struct FBattlePresentationJournal;
struct FWacomBattlePresentationTargetCue;
struct FWacomBattlePresentationStackEntryView;
struct FWacomFirstPersonCardLayerSlotView;
struct FWacomFirstPersonCardDragView;
struct FWacomCardDetailViewData;
struct FWacomBattleEnemyPartDragPredictionDebugInput;
enum class EWacomBattleHUDCardDetailHost : uint8;
enum class EWacomBattleHUDTurnBoundaryCommand : uint8;

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
 * HD-2D/PaperZD 敌方部位表现读取本视图，再把点击意图回传到 BattleHUD，
 * 而不是各自解析 HUD 内部状态。
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

UENUM(BlueprintType)
enum class EWacomBattleCardDropIntentKind : uint8
{
	None UMETA(DisplayName = "None"),
	PlayCardNoTarget UMETA(DisplayName = "PlayCardNoTarget"),
	PlayCardWorldTarget UMETA(DisplayName = "PlayCardWorldTarget"),
	PlayCardCardTarget UMETA(DisplayName = "PlayCardCardTarget"),
	ProbeCardTarget UMETA(DisplayName = "ProbeCardTarget"),
	Reject UMETA(DisplayName = "Reject"),
};

UENUM(BlueprintType)
enum class EWacomBattleCardDropRejectReason : uint8
{
	None UMETA(DisplayName = "None"),
	UIBlocked UMETA(DisplayName = "UIBlocked"),
	MissingSession UMETA(DisplayName = "MissingSession"),
	SourceCardInvalid UMETA(DisplayName = "SourceCardInvalid"),
	SourceCardNotPlayable UMETA(DisplayName = "SourceCardNotPlayable"),
	NotArmed UMETA(DisplayName = "NotArmed"),
	MissingTarget UMETA(DisplayName = "MissingTarget"),
	InvalidWorldTarget UMETA(DisplayName = "InvalidWorldTarget"),
	UnsupportedCardTarget UMETA(DisplayName = "UnsupportedCardTarget"),
	UnsupportedZoneTarget UMETA(DisplayName = "UnsupportedZoneTarget"),
	SelfTarget UMETA(DisplayName = "SelfTarget"),
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleCardDropResolveResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Card Drop")
	FGuid SourceCardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Card Drop")
	FWacomInteractionTargetHandle TargetHandle;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Card Drop")
	EWacomBattleCardDropIntentKind IntentKind = EWacomBattleCardDropIntentKind::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Card Drop")
	EWacomBattleCardDropRejectReason RejectReason = EWacomBattleCardDropRejectReason::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Card Drop")
	bool bCanSubmit = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Card Drop")
	bool bHasFeedbackTargetScreenPosition = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Card Drop")
	FVector2D FeedbackTargetScreenPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Card Drop")
	EWacomBattleTargetRejectReason TargetValidationRejectReason = EWacomBattleTargetRejectReason::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Card Drop")
	FString TargetValidationDebugSummary;

	FString ToDebugString() const;
};

#if WITH_AUTOMATION_TESTS
struct WACOMAPP_API FWacomBattleHUDAutomationTestView
{
	int32 PresentationTargetCount = 0;
	int32 SceneEnemyPartWorldTargetBridgeCount = 0;
	const TArray<FWacomBattlePresentationStackEntryView>* PresentationStackEntries = nullptr;
	const TArray<FWacomBattleCombatLogBlockView>* CombatLogHistory = nullptr;
	bool bPresentationPlanActive = false;
	int32 PresentationPlanPendingPhaseCount = 0;
	FName ActivePresentationPlanPhaseName = NAME_None;
	const TArray<FName>* PresentationPlanStartedPhaseNames = nullptr;
	bool bHasLastBattleSnapshot = false;
	int32 LastBattleSnapshotHandCount = 0;
};
#endif

/**
 * 战斗 UI 根 Widget。
 *
 * 状态机（EBattleUIState）驱动所有子 Widget 的交互模式。
 * 命令提交的唯一入口：子 Widget 发委托给 HUD，HUD 统一调 Session->SubmitCommand。
 *
 * 交互流程：
 *   Idle
 *     ├── 点击 first-person hand 卡牌 → HUD 判断 TargetMode
 *     │     ├── None / Self → 直接提交 PlayCard（空目标），回 Idle
 *     │     └── SingleEnemyPart → 切 TargetSelect，记录 Pending 卡
 *     ├── 点击 Wait 按钮 → 提交 Wait，回 Idle
 *     └── 点击 EndTurn 按钮 → 提交 EndTurn，回 Idle
 *
 *   TargetSelect
 *     ├── 点击场景敌人 PartActor world target → 提交 PlayCard(PendingCard, PartId)，回 Idle
 *     └── 右键 / ESC → 取消，回 Idle
 *
 *   Resolving：保留给未来真正阻塞的战斗表现。普通事件表现队列不再进入该状态。
 *
 *   BattleEnd：战斗结束，显示胜利/失败面板。
 *
 * WBP 子类约定（BindWidget 大部分可选）：
 * - PlayerStatusBar   : UPlayerStatusBar
 * - ActionPanel       : UActionPanel
 * - DrawPileView      : UPileCountView
 * - DiscardPileView   : UPileCountView
 * - ExhaustPileView   : UPileCountView
 * - EquipmentBar      : UEquipmentBar
 * - CombatLogFeed     : UBattleCombatLogFeedWidget
 * - BattlePresentationStack : UBattlePresentationStackWidget
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
	virtual ~UBattleHUD() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Card Detail|Authoring", meta = (ClampMin = "1.0", UIMin = "120.0", UIMax = "900.0", ToolTip = "战斗手牌悬浮详情面板的估算宽高，单位为 Slate 像素。用于 CanvasPanel Slot 尺寸和边界 clamp；实际样式仍由 WBP_CardDetailPanel 决定。"))
	FVector2D CardDetailPanelEstimatedSize = FVector2D(360.0f, 420.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Card Detail|Authoring", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "80.0", ToolTip = "战斗手牌悬浮详情面板与卡牌之间的间距，单位为 Slate 像素。面板默认显示在卡牌左侧，左侧空间不足时换到右侧。"))
	float CardDetailPanelPadding = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Card Detail|Motion", meta = (ToolTip = "是否启用战斗卡牌详情读牌动效。开启后第一人称手牌详情会使用短延迟、淡入淡出、位置平滑和贴边稳定；关闭后恢复立即显示/隐藏。"))
	bool bEnableCardDetailReadabilityPolish = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Card Detail|Motion", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "0.4", ToolTip = "卡牌 hover 后详情出现前的停留时间，单位为秒。用于减少鼠标快速扫过手牌时的详情闪烁。"))
	float CardDetailHoverDelaySeconds = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Card Detail|Motion", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "60.0", ToolTip = "卡牌详情淡入速度。数值越大越快，0 表示直接贴合目标透明度。"))
	float CardDetailFadeInSpeed = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Card Detail|Motion", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "80.0", ToolTip = "卡牌详情淡出速度。数值越大越快，0 表示直接隐藏。"))
	float CardDetailFadeOutSpeed = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Card Detail|Motion", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "80.0", ToolTip = "卡牌详情跟随 hover 锚点位置的平滑速度。数值越大越跟手，0 表示直接贴合目标位置。"))
	float CardDetailFollowSpeed = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Card Detail|Motion", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1600.0", ToolTip = "卡牌详情目标位置跳变超过该距离时直接贴合新位置，单位为 Slate 像素。用于避免切场景、切来源或窗口变化后慢慢漂过去。"))
	float CardDetailPositionResetDistancePixels = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Card Detail|Motion", meta = (ClampMin = "0.5", ClampMax = "1.0", UIMin = "0.85", UIMax = "1.0", ToolTip = "卡牌详情淡入起始缩放。1 表示不缩放；小于 1 时详情会从轻微缩小状态淡入。"))
	float CardDetailAppearStartScale = 0.97f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Card Detail|Motion", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "240.0", ToolTip = "卡牌详情贴近视口边缘时保持当前左右摆放侧的缓冲距离，单位为 Slate 像素。用于避免锚点在边缘附近轻微移动时详情左右反复跳。"))
	float CardDetailSideSwitchHysteresisPixels = 72.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Combat Log|Authoring", meta = (ClampMin = "1", UIMin = "10", UIMax = "300", ToolTip = "BattleHUD 内部保存的玩家可读战斗记录最大命令块数量。超过后只保留最近 N 条，并同步到常驻滚动记录。"))
	int32 BattleCombatLogMaxBlocks = 80;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Presentation Stack|Authoring", meta = (ClampMin = "0.01", UIMin = "0.05", UIMax = "1.0", ToolTip = "打出的卡牌没有目标 cue 或延迟表现时，在表现栈中最短停留多久，单位为秒。用于避免无表现卡牌一闪而过。"))
	float CardPresentationStackMinimumHoldSeconds = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|First Person Card Layer|Authoring", meta = (ClampMin = "0", UIMin = "0", UIMax = "20000", ToolTip = "第一人称手牌 hover 详情面板添加到 Viewport 时使用的层级。需要高于 FirstPersonCardAnchorComponent.CardLayerZOrder，避免详情被第一人称卡牌遮挡。"))
	int32 FirstPersonCardDetailViewportZOrder = 9999;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|First Person Card Layer|Authoring", meta = (ClampMin = "1.0", UIMin = "120.0", UIMax = "900.0", ToolTip = "第一人称手牌详情定位时使用的卡牌锚点基础尺寸，单位为 UMG 布局像素。通常应与 WBP_FPCardView 或 WBP_CardView 的设计尺寸一致。"))
	FVector2D FirstPersonCardDetailAnchorBaseSize = FVector2D(296.0f, 420.0f);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|HUD State", meta = (ToolTip = "当前 BattleHUD UI 状态。只读查询，不修改战斗或 UI 流程。"))
	EBattleUIState GetUIState() const;

	static FVector2D ComputeCardDetailPanelPositionBeside(
		const FVector2D& AnchorPosition,
		const FVector2D& AnchorSize,
		const FVector2D& LayerSize,
		const FVector2D& PanelSize,
		float Padding);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Card Detail", meta = (ToolTip = "当前战斗卡牌详情面板是否可见。只读查询。"))
	bool IsCardDetailPanelVisible() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Card Detail", meta = (ToolTip = "当前战斗卡牌详情面板显示的卡名文本。只读查询。"))
	FText GetCardDetailPanelNameText() const;

	// ---- 子 Widget 交互入口 ----
	// 子 Widget 通过这些方法通知 HUD 玩家意图。HUD 按状态机决策。

	/**
	 * 某张手牌被点击。
	 * - TargetMode == None / Self：立即提交 PlayCard
	 * - TargetMode == SingleEnemyPart：进入 TargetSelect 状态
	 * - 其他：当前不支持，忽略
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Commands", meta = (ToolTip = "玩家点击手牌后的 BattleHUD 命令入口。子 Widget 只上报意图，BattleHUD 负责状态机判断并提交合法 BattleSession 命令。"))
	void OnCardClickedByUser(const FGuid& CardInstanceId);

	/**
	 * TargetSelect 状态下玩家点击了某个敌方部位。
	 * 提交 PlayCard(PendingCardId, EnemyPartKey)，回到 Idle。
	 */
	void OnEnemyPartClickedByUser(const FWacomInteractionTargetHandle& TargetHandle);

	/** 等待按钮点击。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Commands", meta = (ToolTip = "玩家点击 Wait 后的 BattleHUD 命令入口。BattleHUD 会按表现栈和战斗阶段决定立即提交或进入 pending。"))
	void OnWaitRequested();

	/** 结束回合按钮点击。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Commands", meta = (ToolTip = "玩家点击 EndTurn 后的 BattleHUD 命令入口。BattleHUD 会按表现栈和战斗阶段决定立即提交或进入 pending。"))
	void OnEndTurnRequested();

	/** 取消目标选择（ESC、右键、再次点同一张牌等）。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Commands", meta = (ToolTip = "取消当前目标选择 UI 状态。只影响 HUD 选择流程，不直接修改 BattleSession。"))
	void CancelTargetSelect();

	/**
	 * 击倒事件 dialog 的选择入口（GDD §6）。
	 * dialog 不直接调 Session->SubmitCommand，而是走这里，
	 * 让 HUD 在提交后统一调 AfterCommand（事件消费 + Snapshot 刷新 + BattleEnd 广播）。
	 * 与 OnWaitRequested / OnEndTurnRequested 对称。
	 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Commands", meta = (ToolTip = "击倒选择 dialog 的 BattleHUD 命令入口。Dialog 不直接提交 BattleSession，统一由 HUD 做命令后刷新和事件消费。"))
	void OnKnockdownChoiceSelected(EKnockdownChoice Choice);

	/**
	 * 战斗结束时广播（保证只广播一次）。
	 * GameMode 绑定这个来触发 ExitBattle。
	 */
	FOnBattleEndedNative OnBattleEndedNative;

	// ---- 状态机查询（供子 Widget 做视觉反馈）----

	/** 当前是否正在选目标。UI 可据此高亮可选敌方部位。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Targeting", meta = (ToolTip = "当前 BattleHUD 是否正在等待玩家选择目标。UI / 场景表现只读消费。"))
	bool IsInTargetSelect() const;

	/** 当前等待目标的卡 ID。IsInTargetSelect == false 时返回 invalid。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Targeting", meta = (ToolTip = "当前等待目标的卡牌实例 ID；不在 TargetSelect 时返回 invalid。"))
	FGuid GetPendingTargetingCardId() const;

	/** 构建当前目标选择表现视图。UI/场景表现只读消费，不修改战斗状态。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Targeting")
	FBattleTargetSelectionView BuildTargetSelectionView() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Combat Log")
	int32 GetBattleCombatLogBlockCount() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Presentation Flow", meta = (ToolTip = "当前 BattleHUD 表现队列或表现栈是否仍在播放。只读查询，不提交命令。"))
	bool IsBattlePresentationBusy() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Presentation Flow", meta = (ToolTip = "当前 BattleHUD 是否允许提交普通玩家行动命令。只读查询，包含战斗阶段、击倒选择和表现 pending gate。"))
	bool CanSubmitPlayerActionCommand() const;

	void SetBattleInputReady(bool bReady);
	bool IsBattleInputReady() const;
	void SetFirstPersonBattleHandSuppressedForEntry(bool bSuppressed);
	bool IsFirstPersonBattleHandSuppressedForEntry() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Presentation Flow", meta = (ToolTip = "是否存在等待表现栈清空后再提交的 Wait / EndTurn 命令。"))
	bool HasPendingTurnBoundaryCommand() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Presentation Flow", meta = (ToolTip = "当前 pending Wait / EndTurn 命令的 UI 文案。没有 pending 命令时返回空文本。"))
	FText GetPendingTurnBoundaryCommandText() const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Scene Enemy", meta = (ToolTip = "设置当前战斗绑定的场景敌人 Host 列表。用于 Encounter 多敌人；BattleHUD 只会同步这些 Host 下的 PartActor world target。"))
	void SetBattleSceneEnemyHosts(const TArray<AWacomBattleEnemyActor*>& InHosts);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy", meta = (ToolTip = "给调试和制作校验使用：判断指定 SceneEnemyHost 是否属于当前 BattleHUD 的场景敌人 registry。"))
	bool IsBattleSceneEnemyHostInCurrentRegistry(const AWacomBattleEnemyActor* Host) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Scene Enemy", meta = (ToolTip = "给输入路由使用：判断 World target handle 是否来自当前 SceneEnemyHost 注册的部位。"))
	bool IsBattleSceneEnemyPartWorldTargetInCurrentRegistry(
		const FWacomInteractionTargetHandle& TargetHandle) const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
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

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Battle|HUD State", DisplayName = "On UI State Changed", meta = (ToolTip = "BattleHUD UI 状态切换后的 WBP 表现事件。只用于刷新表现，不应直接修改 BattleSession。"))
	void BP_OnUIStateChanged(EBattleUIState OldState, EBattleUIState NewState);

	// ---- BindWidget ----

	/** 玩家状态条。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UPlayerStatusBar> PlayerStatusBar;

	/** 操作面板。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UActionPanel> ActionPanel;

	/** 装备条。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UEquipmentBar> EquipmentBar;

	/** 抽牌堆计数。PileCountView 不是 BattleWidget，Refresh 时手动更新。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UPileCountView> DrawPileView;

	/** 弃牌堆计数显示。PlayedCount > 0 时显示为 "弃牌堆数+本回合使用牌堆数"。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UPileCountView> DiscardPileView;

	/** 消耗牌堆计数。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UPileCountView> ExhaustPileView;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBattleCombatLogFeedWidget> CombatLogFeed;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBattlePresentationStackWidget> BattlePresentationStack;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Battle|Card Detail|Authoring", meta = (ToolTip = "战斗第一人称手牌详情面板 Widget 类。"))
	TSubclassOf<UWacomCardDetailPanel> CardDetailPanelClass;

private:
	UPROPERTY(Transient)
	TObjectPtr<UWacomCardDetailPanel> FirstPersonCardDetailPanel;

	FWacomBattleHUDRuntime* BattleHUDRuntime = nullptr;

	/** 内部状态切换入口，同时触发 Native + BP 钩子。 */
	void SetUIState(EBattleUIState NewState);
	FWacomBattleHUDRuntime& GetBattleHUDRuntime();
	const FWacomBattleHUDRuntime& GetBattleHUDRuntime() const;
	void RebuildChildBattleWidgetsForRuntime();
	void RefreshChildBattleWidgetsFromSnapshotForRuntime(const FBattleSnapshot& Snap);
	void BindFirstPersonCardLayerInteractionsForRuntime(UWacomFirstPersonCardAnchorComponent& Anchor);
	void UnbindFirstPersonCardLayerInteractionsForRuntime(UWacomFirstPersonCardAnchorComponent& Anchor);

	/** 内部：提交 PlayCard 命令 + 事件消费 + 刷新。 */
	void SubmitPlayCard(const FGuid& CardId, const FGuid& TargetPartId);
	void SubmitPlayCardOnHandCard(const FGuid& CardId, const FGuid& TargetCardId);

	/** 内部：消费 Session 事件并分发给战斗记录 / 表现队列 / 击倒 dialog。 */
	void ConsumeAndLogEvents();

	void AppendBattleCombatLogBlock(const FWacomBattleCombatLogBlockView& Block);
	void StoreFirstPersonCardTransitionEvents(const TArray<struct FBattleEvent>& Events);
	void ClearPendingFirstPersonCardTransitionEvents();
	void RecordFirstPersonPlayCommit(const FGuid& CardInstanceId, const FBattlePartSlotIdentity& TargetPartKey);
	TArray<FWacomFirstPersonCardLayerTransitionHint> BuildFirstPersonCardTransitionHints(
		const FBattleSnapshot& PreviousSnapshot,
		const FBattleSnapshot& NextSnapshot) const;
	TArray<FWacomFirstPersonCardLayerFeedbackHint> BuildFirstPersonCardFeedbackHints(
		const FBattleSnapshot& NextSnapshot) const;
	int32 AppendBattlePresentationStackEntry(
		const FWacomBattleCombatLogCommandContext& CommandContext,
		const FBattleSnapshot& PreCommandSnapshot);
	void BeginBattlePresentationStackEntryExit(int32 EntryId);
	void FinishBattlePresentationStackEntryExit(int32 EntryId);
	void ClearBattlePresentationStack();
	bool HasBattlePresentationStackEntries() const;
	void EnqueueBattlePresentationEvents(
		const TArray<struct FBattleEvent>& Events,
		int32 PresentationStackEntryId = INDEX_NONE);
	void ClearBattlePresentationQueue();
	bool IsBattlePresentationQueueBusy() const;
	void QueuePendingTurnBoundaryCommand(EWacomBattleHUDTurnBoundaryCommand Command);
	void ClearPendingTurnBoundaryCommand();
	void TryExecutePendingTurnBoundaryCommand();
	FWacomBattlePresentationTargetRegistry& GetBattlePresentationTargetRegistry();
	void ClearBattlePresentationTargetRegistry();
	void RegisterBattlePresentationTarget(
		const FBattlePartSlotIdentity& TargetPartKey,
		UObject* Owner,
		TFunction<void(const FWacomBattlePresentationTargetCue&)> Handler);
	void UnregisterBattlePresentationTargetsForOwner(const UObject* Owner);
	bool IsBattlePresentationTargetRegisteredForOwner(const UObject* Owner) const;
	void PlayBattlePresentationCue(const FWacomBattlePresentationTargetCue& Cue);
	void PushPendingKnockdownChoiceDialog();
	void AdvanceBattlePresentationQueueOnce();

#if WITH_AUTOMATION_TESTS
	void PlayBattlePresentationCueForTest(EBattleEventType SourceEventType, const FBattlePartSlotIdentity& TargetPartKey, int32 Amount);
	void PlayTargetConfirmedCueForTest(const FBattlePartSlotIdentity& TargetPartKey);
	bool EnqueueEndTurnPresentationPlanForTest(
		const FBattlePresentationJournal& Journal,
		const TArray<FBattleEvent>& Events,
		const FBattleSnapshot& PostCommandSnapshot);
	FWacomBattleHUDAutomationTestView GetAutomationTestViewForTest() const;
	TArray<FWacomFirstPersonCardLayerTransitionHint> BuildFirstPersonCardTransitionHintsForRefreshForTest(
		const FBattleSnapshot& NextSnapshot) const;
	void SetFirstPersonCardTransitionSnapshotForTest(const FBattleSnapshot& Snapshot);
	void SetTargetSelectionStateForAutomationTest(const FGuid& PendingCardId);
	void ClearTargetSelectionStateForAutomationTest();
	void QueuePendingTurnBoundaryWaitForAutomationTest();
#endif

	/** 内部：提交命令后的通用收尾（刷新 + 战斗结束检测）。 */
	void AfterCommand();

	void HideCardDetailPanel();
	void HideFirstPersonCardDetailPanelForSource(const FGuid& CardInstanceId);
	bool IsFirstPersonCardInspectDetailActiveForSource(const FGuid& CardInstanceId) const;
	UWacomCardDetailPanel* EnsureFirstPersonCardDetailPanel();
	bool ShowFirstPersonCardDetailAtSlot(
		const FWacomCardDetailViewData& DetailData,
		const FWacomFirstPersonCardLayerSlotView& SlotView);
	void PositionFirstPersonCardDetailPanelBesideSlot(const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HideFirstPersonCardDetailPanel();
	void TickCardDetailMotion(float DeltaTime);
	void ForceHideCardDetailHost(EWacomBattleHUDCardDetailHost Host);
	bool ComputeFirstPersonCardDetailTarget(const FWacomFirstPersonCardLayerSlotView& SlotView, FVector2D& OutPosition);
	FVector2D ComputeCardDetailPanelPositionBesideStable(
		const FVector2D& AnchorPosition,
		const FVector2D& AnchorSize,
		const FVector2D& LayerSize,
		const FVector2D& PanelSize,
		float DetailPadding);
	FVector2D GetFirstPersonCardDetailViewportSize() const;
	void SetFirstPersonCardDetailSource(const FGuid& CardInstanceId);
	void ClearFirstPersonCardDetailSource();
	bool IsCurrentFirstPersonCardDetailSource(const FGuid& CardInstanceId) const;
	void UpdateFirstPersonCardDetailSlot(const FWacomFirstPersonCardLayerSlotView& SlotView);
	FVector2D GetLastFirstPersonCardDetailPanelPosition() const;
	const FHandCardSnapshot* FindLastBattleHandCardSnapshot(const FGuid& CardInstanceId) const;
	void RebuildBattleSceneEnemyPartWorldTargetRegistry();
	bool IsBattleSceneEnemyPartBridgeInCurrentRegistry(
		const UWacomBattleEnemyPartWorldTargetBridgeComponent* Bridge) const;
	void SyncBattleEnemyPartWorldTargets(const FBattleSnapshot& Snap);
	void ClearBattleEnemyPartWorldTargets();
	bool CanUpdateBattleSceneEnemyPartHoverProbe() const;
	FWacomBattleEnemyPartDragPredictionDebugInput BuildBattleSceneEnemyPartHoverPredictionInput(
		const FWacomInteractionTargetHandle& TargetHandle) const;
	void TickBattleSceneEnemyPartHoverProbe(float DeltaTime);
	void UpdateBattleSceneEnemyPartHoverProbe();
	void ClearBattleSceneEnemyPartHoverProbe(FName Reason);
	UWacomFirstPersonCardAnchorComponent* ResolveFirstPersonCardAnchor() const;
	UWacomFirstPersonCardAnchorComponent* ResolveActiveFirstPersonCardAnchor() const;
	void SyncFirstPersonBattleHandLayer(const FBattleSnapshot& Snap);
	void SyncFirstPersonBattleHandLayer(
		const FBattleSnapshot& Snap,
		const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints);
	void SyncFirstPersonBattleHandLayer(
		const FBattleSnapshot& Snap,
		const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints,
		const TArray<FWacomFirstPersonCardLayerFeedbackHint>& FeedbackHints);
	void RefreshFromPresentationPhase(
		const FBattleSnapshot& Snap,
		const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints,
		const TArray<FWacomFirstPersonCardLayerFeedbackHint>& FeedbackHints);
	void ClearFirstPersonBattleHandLayer();
	bool ShouldUseFirstPersonBattleHandLayer() const;
	bool ShouldEnableFirstPersonBattleHandInteraction() const;
	void BindFirstPersonBattleHandLayerInteractions(UWacomFirstPersonCardAnchorComponent* Anchor);
	void UnbindFirstPersonBattleHandLayerInteractions(UWacomFirstPersonCardAnchorComponent* Anchor);
	void HandleFirstPersonCardLayerCardHovered(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleFirstPersonCardLayerCardUnhovered(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleFirstPersonCardLayerHoveredCardLayoutUpdated(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleFirstPersonCardLayerCardTargetHovered(const FWacomInteractionTargetHandle& CardTargetHandle, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleFirstPersonCardLayerCardTargetUnhovered(const FWacomInteractionTargetHandle& CardTargetHandle, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleFirstPersonCardLayerHoveredCardTargetUpdated(const FWacomInteractionTargetHandle& CardTargetHandle, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleFirstPersonCardLayerDragStarted(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void HandleFirstPersonCardLayerDragUpdated(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void HandleFirstPersonCardLayerDragReleased(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void HandleFirstPersonCardLayerDragCancelled(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void HandleFirstPersonCardLayerPointerMoved(const FWacomFirstPersonCardPointerView& PointerView);
	void HandleFirstPersonCardLayerPointerLeft();
	void ApplyFirstPersonCardDragCameraLookOverride(const FWacomFirstPersonCardDragView& DragView);
	void ClearFirstPersonCardDragCameraLookOverride();
	void UpdateFirstPersonCardDragTargetFeedback(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView);
	void ClearFirstPersonCardDragTargetFeedback();
	bool IsFirstPersonCardDragActiveForBattleSceneHover() const;
	FWacomBattleCardDropResolveResult ResolveFirstPersonCardDropIntent(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView) const;
	TArray<FWacomFirstPersonCardTargetAffordance> BuildFirstPersonCardTargetAffordances(
		const FGuid& SourceCardId,
		const FBattleSnapshot& Snapshot,
		const UBattleSession& BattleSession) const;
	UWacomBattleEnemyPartWorldTargetBridgeComponent* ResolveBattleEnemyPartWorldTargetBridge(
		const FWacomInteractionTargetHandle& TargetHandle) const;
	UWacomBattleEnemyPartPresentationComponent* ResolveBattleEnemyPartWorldTargetPresentation(
		const FWacomInteractionTargetHandle& TargetHandle) const;
	bool ProbeFirstPersonCardDragTarget(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView,
		FWacomInteractionTargetHandle& OutTargetHandle,
		bool& bOutValidTarget) const;
	bool ShouldShowFirstPersonDragInspectDetail(const FWacomFirstPersonCardDragView& DragView) const;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Battle|Scene Enemy", meta = (ToolTip = "战斗场景敌方部位 hover probe 的最小间隔，单位秒。只更新 UI 诊断和轻量缩放，不影响战斗规则。", ClampMin = "0.01", ClampMax = "0.25", UIMin = "0.01", UIMax = "0.1"))
	float BattleSceneEnemyPartHoverProbeIntervalSeconds = 0.03f;

	friend struct FBattleHUDFallbackLayoutBuilder;
	friend class FWacomBattleHUDRuntimeHost;
	friend class UWacomBattleHUDDetailTest;
};
