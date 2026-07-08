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
class UWacomDeleteZoneDropTarget;
class UWacomCardDetailPanel;
class UWacomDeckCardWidget;
class UWacomBackpackZoneSectionWidget;
class UWacomSpecialZoneWidget;
class UWacomZoneDropTarget;
class UWacomRunViewModel;
class UWacomRunViewModelProvider;
class UWacomCardDragOperation;
class FWacomBackpackCardDetailController;
class FWacomBackpackStorageRefreshGate;
struct FWacomBackpackScreenTestAccess;
struct FCardInstance;

#if WITH_AUTOMATION_TESTS
struct WACOMAPP_API FWacomBackpackScreenAutomationTestView
{
	int32 ListRefreshApplyCount = 0;
	int32 ListRefreshSkipCount = 0;
	int32 SnapshotBuildCount = 0;
	int32 SnapshotRevisionSkipCount = 0;
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

	/** DropTarget 命令出口：移动卡牌。DropTarget 只转发拖拽意图，规则提交和 Toast 由 Private command flow 处理。 */
	bool HandleZoneDropRequested(const UWacomCardDragOperation& CardOp, EZoneKind TargetZone, FGuid TargetZoneOwnerInstanceId);

	/** DropTarget 命令出口：请求销毁卡牌。确认框和 Run 命令提交由 Private command flow 处理。 */
	bool HandleDeleteDropRequested(const UWacomCardDragOperation& CardOp);

	/** DropTarget 预览出口：只读判断移动意图是否可提交；Widget 不直接读取 RunSession。 */
	bool CanPreviewZoneDrop(const UWacomCardDragOperation& CardOp, EZoneKind TargetZone, FGuid TargetZoneOwnerInstanceId) const;

	/** DropTarget 预览出口：只读判断删牌意图是否可提交；Widget 不直接读取 RunSession。 */
	bool CanPreviewDeleteDrop(const UWacomCardDragOperation& CardOp) const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnActivated() override;
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
	TObjectPtr<UWacomZoneDropTarget> BattleDeckDropTarget;

	UPROPERTY(Transient)
	TObjectPtr<UWacomZoneDropTarget> BackpackDropTarget;

	UPROPERTY(Transient)
	TObjectPtr<UWacomDeleteZoneDropTarget> DeleteDropTarget;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> SpecialZonesPanel;

	UPROPERTY(Transient)
	TObjectPtr<UWrapBox> BurdenCardsBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BurdenZoneTitleText;

	/** 关闭按钮（点击 = DeactivateWidget）。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	UFUNCTION()
	void HandleCloseClicked();

private:
	UWacomRunViewModelProvider* GetProvider() const;
	UWacomRunViewModel* GetViewModel() const;

	UPROPERTY(Transient)
	TObjectPtr<UWacomRunViewModelProvider> SubscribedProvider = nullptr;

	/** 销毁现有 WrapBox 子项。 */
	void ClearCardBoxes();

	/** 默认 C++ 布局只负责搭出三大区和 Host，实际 DropTarget 由 EnsureRuntimeZoneWidgets 填充。 */
	void EnsureRuntimeZoneWidgets();

	/** 子控件类（默认 UWacomDeckCardWidget，蓝图可覆盖）。 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Backpack")
	TSubclassOf<UWacomDeckCardWidget> CardWidgetClass;

	/** 单个 SpecialZone 区块类（默认 UWacomSpecialZoneWidget，蓝图可覆盖）。 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Backpack")
	TSubclassOf<UWacomSpecialZoneWidget> SpecialZoneWidgetClass;

	/** 卡牌详情悬浮面板类（默认 WBP_CardDetailPanel，失败则用 C++ 类）。 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Backpack")
	TSubclassOf<UWacomCardDetailPanel> CardDetailPanelClass;

	UPROPERTY(Transient)
	TObjectPtr<UWacomCardDetailPanel> CardDetailPanel;

	TWeakObjectPtr<UWacomDeckCardWidget> CardDetailSourceWidget;

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
	void ResetBackpackRefreshDirtyGate();

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

#if WITH_AUTOMATION_TESTS
	URunSession* RunSessionOverrideForTest = nullptr;
#endif
};
