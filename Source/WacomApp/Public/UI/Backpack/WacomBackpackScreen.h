// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunStateTypes.h"
#include "UI/Run/WacomRunMenuWidgetBase.h"
#include "WacomBackpackScreen.generated.h"

class UButton;
class UCanvasPanel;
class UPanelWidget;
class UTextBlock;
class UWrapBox;
class UVerticalBox;
class UCardDefinition;
class URunSession;
class UWacomCardDetailPanel;
class UWacomDeckCardWidget;
class UWacomBackpackZoneSectionWidget;
class UWacomSpecialZoneWidget;
class UWacomRunViewModel;
class UWacomRunViewModelProvider;
class UWacomBackpackWorkspaceStyle;
class UWacomBackpackWorkspaceWidget;
class UWacomBackpackDeleteConfirmWidget;
class UWacomPrimaryGameLayout;
class UWacomSettingsSubsystem;
class FWacomBackpackCardDetailController;
class FWacomBackpackStorageRefreshGate;
class FWacomBackpackWorkspaceInteractionModel;
struct FWacomBackpackWorkspaceStateStore;
struct FWacomBackpackWorkspaceReleaseIntent;
struct FWacomBackpackZoneKey;
struct FWacomBackpackPendingDeleteConfirmation;
struct FWacomBackpackScreenTestAccess;
struct FCardInstance;
struct FWacomLocalSettingsSnapshot;
enum class EWacomRuntimeSettingsChangeReason : uint8;

#if WITH_AUTOMATION_TESTS
struct WACOMAPP_API FWacomBackpackScreenAutomationTestView
{
	int32 ListRefreshApplyCount = 0;
	int32 ListRefreshSkipCount = 0;
	int32 SnapshotBuildCount = 0;
	int32 SnapshotRevisionSkipCount = 0;
	int32 WorkspacePileCount = 0;
	int32 WorkspaceCardCount = 0;
	EZoneKind ActiveWorkspaceZone = EZoneKind::Backpack;
	FGuid ActiveWorkspaceOwnerInstanceId;
};
#endif

/**
 * 背包界面。Push 到 GameMenu 层。
 *
 * 由 PlayerController 在按 B 时 Push（或 Console Command）。
 *
 * 三大区域：
 *   - 删牌区（DeleteZone）：玩家拖卡过来 → 置换金币
 *   - 备战区（BattleDeckZone）：BattleDeck 内容
 *   - 背包区：通量存放区 + SpecialZone + 负重区
 *
 * MVVM 数据流：
 *   - 顶部标量（金币 / 备战区 N/M / 背包区 N/M）：读 RunViewModel 字段
 *   - WrapBox 列表内容：读 RunSession.BuildBackpackStorageSnapshot()
 *     （UE MVVM 不擅长数组绑定，列表数据保留命令式重建）
 *   - 刷新触发：订阅 Provider.OnRunViewModelRefreshedNative，事件驱动
 *   - 操作命令：Screen 只转发意图；Move/Delete/Toggle 流程由 Private command flow 提交
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomBackpackScreen : public UWacomRunMenuWidgetBase
{
	GENERATED_BODY()

public:
	UWacomBackpackScreen(const FObjectInitializer& ObjectInitializer);

	/** 从 PC 拿当前 RunSession（每帧都拿，不持引用）。 */
	URunSession* GetRunSession() const;

	static FText BuildSpecialZoneTitleText(const FText& OwnerName, int32 CardCount, int32 Capacity);
	static ESlateVisibility GetSpecialZoneBattleReadyBadgeVisibility(EZoneKind OwnerZone);
	static FText BuildBurdenZoneTitleText(int32 CardCount);
	static FVector2D ComputeCardDetailPanelPosition(
		FVector2D AnchorPosition,
		FVector2D AnchorSize,
		FVector2D LayerSize,
		FVector2D PanelSize,
		float Padding = 12.f);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** 全量重建：顶部读 ViewModel，WrapBox 子项读 Run 层 Snapshot。 */
	void RebuildAll();

	/** Provider 广播回调。 */
	void HandleViewModelRefreshed();

	/** 可测试覆写：生产路径仍从 OwningPlayer 拿当前 RunSession。 */
	virtual URunSession* ResolveRunSession() const;

	/** 订阅 Provider（如果还没订阅）+ 刷新一次。 */
	void TrySubscribeAndRefresh();

	/** SpecialZone 卡右键切换入战标记。 */
	void HandleBattleEnabledToggle(FGuid InstanceId);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GoldText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DeleteZoneTitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BattleDeckTitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BackpackTitleText;

	/** 单一中央自由工作台 Host。WBP 可提供；未绑定时 fallback 自动创建。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> WorkspaceHost;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> DeleteTargetHost;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> DeleteConfirmHost;

	/** 将当前区域的手动位置、角度和层级恢复为确定性默认布局。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ArrangeAllButton;

	/** 恢复所有可移动牌堆的默认位置和层级，不影响通量卡手动布局。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ResetPilePositionsButton;

	/** WBP 可绑定的删牌区运行时内容槽。未绑定时 C++ fallback 会创建。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> DeleteZoneHost;

	/** WBP 可绑定的备战区运行时内容槽。未绑定时 C++ fallback 会创建。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> BattleDeckZoneHost;

	/** WBP 可绑定：通量内容 DropTarget 槽，接收放入 Backpack 的 A 类容器和普通内容卡。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> FluxContentDropTargetHost;

	/** WBP 可绑定的特殊存放区运行时内容槽。未绑定时 C++ fallback 会创建。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> SpecialZonesHost;

	/** WBP 可绑定的负重区运行时内容槽。未绑定时 C++ fallback 会创建；该区只展示负重卡，不创建 DropTarget。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> BurdenZoneHost;

	/** WBP 可绑定的详情悬浮层。推荐 CanvasPanel，详情面板会定位在悬停卡牌旁边。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> CardDetailLayer;

	/** 备战区卡牌容器（WrapBox 自动横向流式 + 换行）。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> BattleDeckCardsBox;

	/** 通量区内容卡容器。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> FluxContentCardsBox;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> SpecialZonesPanel;

	UPROPERTY(Transient)
	TObjectPtr<UWrapBox> BurdenCardsBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BurdenZoneTitleText;

	UPROPERTY(Transient)
	TObjectPtr<UWacomBackpackWorkspaceWidget> WorkspaceWidget;

	UPROPERTY(Transient)
	TObjectPtr<UWacomBackpackDeleteConfirmWidget> DeleteConfirmWidget;

	/** 关闭按钮（点击 = DeactivateWidget）。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleArrangeAllClicked();

	UFUNCTION()
	void HandleResetPilePositionsClicked();

private:
	UWacomRunViewModelProvider* GetProvider() const;
	UWacomRunViewModel* GetViewModel() const;

	UPROPERTY(Transient)
	TObjectPtr<UWacomRunViewModelProvider> SubscribedProvider = nullptr;

	/** 销毁现有 WrapBox 子项。 */
	void ClearCardBoxes();

	/** 为旧 fallback 布局补齐只读列表和详情层；正式输入只由 Workspace 接管。 */
	void EnsureRuntimeZoneWidgets();
	void EnsureWorkspaceWidgets();

	/** 子控件类（默认 UWacomDeckCardWidget，蓝图可覆盖）。 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Backpack")
	TSubclassOf<UWacomDeckCardWidget> CardWidgetClass;

	/** 单个 SpecialZone 区块类（默认 UWacomSpecialZoneWidget，蓝图可覆盖）。 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Backpack")
	TSubclassOf<UWacomSpecialZoneWidget> SpecialZoneWidgetClass;

	/** 卡牌详情悬浮面板类（默认 WBP_CardDetailPanel，失败则用 C++ 类）。 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Backpack")
	TSubclassOf<UWacomCardDetailPanel> CardDetailPanelClass;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Backpack|Workspace",
		meta = (ToolTip = "中央自由工作台 Widget 类。保持被动，只显示 Screen 提供的活动区域与布局。"))
	TSubclassOf<UWacomBackpackWorkspaceWidget> WorkspaceWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Backpack|Workspace",
		meta = (ToolTip = "批量销毁确认 Widget 类。只显示数量和奖励并转发确认、取消意图。"))
	TSubclassOf<UWacomBackpackDeleteConfirmWidget> DeleteConfirmWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Backpack|Workspace Tuning",
		meta = (ToolTip = "背包自由工作台的布局、牌列、颜色和动效制作参数资产。双击引用的 DA_BackpackWorkspaceStyle 可直接调整；不影响 Run 规则。"))
	TObjectPtr<UWacomBackpackWorkspaceStyle> WorkspaceStyle;

	UPROPERTY(Transient)
	TObjectPtr<UWacomCardDetailPanel> CardDetailPanel;

	TWeakObjectPtr<UWacomDeckCardWidget> CardDetailSourceWidget;
	TWeakObjectPtr<UWacomDeckCardWidget> WorkspaceBrowseFocusDetailSource;

	TSharedPtr<FWacomBackpackCardDetailController> CardDetailController;

	UPROPERTY(Transient)
	TObjectPtr<UWacomBackpackZoneSectionWidget> BattleDeckZoneSection;

	UPROPERTY(Transient)
	TObjectPtr<UWacomBackpackZoneSectionWidget> FluxContentZoneSection;

	UPROPERTY(Transient)
	TObjectPtr<UWacomBackpackZoneSectionWidget> BurdenZoneSection;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Backpack|Sections")
	TSubclassOf<UWacomBackpackZoneSectionWidget> DeleteZoneSectionWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Backpack|Sections")
	TSubclassOf<UWacomBackpackZoneSectionWidget> BattleDeckZoneSectionWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Backpack|Sections")
	TSubclassOf<UWacomBackpackZoneSectionWidget> FluxMainZoneSectionWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Backpack|Sections")
	TSubclassOf<UWacomBackpackZoneSectionWidget> FluxContentZoneSectionWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Backpack|Sections")
	TSubclassOf<UWacomBackpackZoneSectionWidget> SpecialZonesSectionWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Backpack|Sections")
	TSubclassOf<UWacomBackpackZoneSectionWidget> BurdenZoneSectionWidgetClass;

	/** 创建一张卡的 widget 并接好回调。 */
	UWacomDeckCardWidget* CreateCardWidget(const FCardInstance& Inst, EZoneKind FromZone, FGuid FromZoneOwnerInstanceId);
	UWacomDeckCardWidget* CreateCardWidget(const FRunStorageCardView& CardView);

	void RebuildTopStats(UWacomRunViewModel* VM);
	void RebuildBattleDeckZone(const FRunBackpackStorageSnapshot& Snapshot);
	void RebuildBackpackZone(const FRunBackpackStorageSnapshot& Snapshot);
	void RebuildFluxContentCards(const FRunBackpackStorageSnapshot& Snapshot);
	void RebuildSpecialZones(const FRunBackpackStorageSnapshot& Snapshot);
	void RebuildBurdenZone(const FRunBackpackStorageSnapshot& Snapshot);
	void RebuildWorkspaceChrome(const FRunBackpackStorageSnapshot& Snapshot);
	void RebuildWorkspaceFromCachedSnapshot();
	void HandlePileExpansionRequested(EZoneKind Zone, FGuid OwnerInstanceId, bool bExpandOnly);
	void HandlePileMoveCommitted(EZoneKind Zone, FGuid OwnerInstanceId, FVector2D NormalizedPosition);
	void HandleCollapseExpandedPileRequested();
	void HandlePileCollapseAnimationFinished(EZoneKind Zone, FGuid OwnerInstanceId);
	void HandleWorkspaceReleaseIntent(const FWacomBackpackWorkspaceReleaseIntent& Intent);
	void HandleWorkspacePileReleaseIntent(
		const FWacomBackpackWorkspaceReleaseIntent& Intent,
		const FWacomBackpackZoneKey& PileTarget);
	void HandleWorkspaceInteractionChanged();
	void HandleWorkspaceBrowseFocusChanged(UWacomDeckCardWidget* SourceWidget);
	void HandleWorkspaceLayoutGeometryReady(FVector2D LayoutSize);
	void ApplyOwningLayerTransitionState(bool bTransitioning);
	void BindOwningLayerTransition();
	void UnbindOwningLayerTransition();
	void HandleOwningLayerTransitioningChanged(FGameplayTag LayerTag, bool bTransitioning);
	void BindRuntimeSettings();
	void UnbindRuntimeSettings();
	void HandleRuntimeSettingsChanged(
		const FWacomLocalSettingsSnapshot& Snapshot,
		EWacomRuntimeSettingsChangeReason Reason);
	void CancelWorkspaceInteraction();
	bool ResolveWorkspacePileTarget(FWacomBackpackZoneKey& OutTarget) const;
	bool IsWorkspaceDeleteTarget() const;
	void BeginWorkspaceDeleteConfirmation(TConstArrayView<FGuid> InstanceIds);
	void HandleWorkspaceDeleteConfirmed();
	void HandleWorkspaceDeleteCancelled();
	FWacomBackpackWorkspaceStateStore& GetWorkspaceStateStore(URunSession* Run);
	void ResetBackpackRefreshDirtyGate();
	void BeginWorkspaceMutationRefreshDeferral();
	void EndWorkspaceMutationRefreshDeferral(bool bForceRefresh);

	void HandleCardHovered(UWacomDeckCardWidget* SourceWidget);
	void HandleCardUnhovered(UWacomDeckCardWidget* SourceWidget);
	bool IsCardDetailPanelVisible() const;
	FText GetCardDetailPanelNameText() const;
	bool ShowCardDetailForCardWidget(UWacomDeckCardWidget* SourceWidget);
	void HideCardDetailPanel();
	UWacomCardDetailPanel* EnsureCardDetailPanel();
	void PositionCardDetailPanelNear(UWacomDeckCardWidget* SourceWidget);
	void HideCardDetailPanelIfSourceRemoved(UWacomDeckCardWidget* RemovedWidget);
	FWacomBackpackCardDetailController& GetCardDetailController();
	const FWacomBackpackCardDetailController& GetCardDetailController() const;
	FWacomBackpackStorageRefreshGate& GetStorageRefreshGate();

#if WITH_AUTOMATION_TESTS
	friend struct FWacomBackpackScreenTestAccess;
#endif
	friend class FWacomBackpackCardDetailController;

#if WITH_AUTOMATION_TESTS
	UWacomDeckCardWidget* GetBattleDeckCardWidgetForTest(int32 Index) const;
	UWacomDeckCardWidget* GetFluxContentCardWidgetForTest(int32 Index) const;
	UWacomDeckCardWidget* GetBurdenCardWidgetForTest(int32 Index) const;
	UWacomSpecialZoneWidget* GetSpecialZoneWidgetForTest(int32 Index) const;
	void RebuildAllForTest() { RebuildAll(); }
	FWacomBackpackScreenAutomationTestView GetAutomationTestViewForTest() const;
	static FText BuildMoveZoneNameTextForTest(EZoneKind Zone);
	static FText BuildMoveFailureToastTextForTest(FName DisabledReason);
	static FText BuildDeleteFailureToastTextForTest(FName DisabledReason);

	void SetRunSessionForTest(URunSession* InRunSession)
	{
		if (RunSessionOverrideForTest != InRunSession)
		{
			RunSessionOverrideForTest = InRunSession;
			ResetBackpackRefreshDirtyGate();
		}
	}
#endif

	TSharedPtr<FWacomBackpackStorageRefreshGate> StorageRefreshGate;
	TSharedPtr<FWacomBackpackWorkspaceStateStore> WorkspaceStateFallback;
	TSharedPtr<FWacomBackpackWorkspaceInteractionModel> WorkspaceInteractionModel;
	TSharedPtr<FWacomBackpackPendingDeleteConfirmation> PendingDeleteConfirmation;
	TWeakObjectPtr<UWacomPrimaryGameLayout> BoundPrimaryLayout;
	TWeakObjectPtr<UWacomSettingsSubsystem> BoundSettingsSubsystem;
	FDelegateHandle RuntimeSettingsChangedHandle;
	bool bOwningLayerTransitioning = false;
	bool bHasPendingPileExpansionAfterCollapse = false;
	bool bPendingPileExpansionRequiresCarryHover = false;
	EZoneKind PendingPileExpansionZone = EZoneKind::Backpack;
	FGuid PendingPileExpansionOwnerInstanceId;

#if WITH_EDITOR
	/** 仅由编辑器 PIE 验收入口在本实例构造时设置；不反射、不序列化、不修改 Run。 */
	bool bPIEValidationEmptySnapshot = false;
#endif

	FRunBackpackStorageSnapshot LastAppliedStorageSnapshot;
	bool bHasLastAppliedStorageSnapshot = false;
	int32 WorkspaceMutationRefreshDeferralDepth = 0;
	bool bWorkspaceMutationRefreshDeferred = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWacomDeckCardWidget>> ActiveWorkspaceCardWidgets;

#if WITH_AUTOMATION_TESTS
	URunSession* RunSessionOverrideForTest = nullptr;
#endif
};
