// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RunStateTypes.h"
#include "UI/Backpack/WacomBackpackZonePileTypes.h"
#include "WacomBackpackWorkspaceWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UInvalidationBox;
class UTextBlock;
class UWacomBackpackWorkspaceStyle;
class UWacomDeckCardWidget;
class UWacomBackpackZonePileWidget;
struct FWacomBackpackZoneKey;
class FWacomBackpackWorkspaceInteractionModel;
class FWacomBackpackCardPresentationController;
struct FWacomBackpackWorkspaceReleaseIntent;
#if WITH_AUTOMATION_TESTS
struct FWacomBackpackWorkspaceAutomationTestView;
struct FWacomBackpackScreenTestAccess;
#endif

/**
 * 被动中央工作台。只持有可视卡牌、框选层和空状态，并应用 Screen/协调器计算好的布局。
 * 输入语义和 Run 写操作由后续的 Screen flow 统一拥有。
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomBackpackWorkspaceWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnReleaseIntentNative, const FWacomBackpackWorkspaceReleaseIntent&);
	DECLARE_MULTICAST_DELEGATE(FOnInteractionChangedNative);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnLayoutGeometryReadyNative, FVector2D);
	DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnPileExpansionRequestedNative, EZoneKind, FGuid, bool);
	DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnPileMoveCommittedNative, EZoneKind, FGuid, FVector2D);
	DECLARE_MULTICAST_DELEGATE(FOnCollapseExpandedPileRequestedNative);
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPileCollapseAnimationFinishedNative, EZoneKind, FGuid);
	FOnReleaseIntentNative OnReleaseIntentNative;
	FOnInteractionChangedNative OnInteractionChangedNative;
	FOnLayoutGeometryReadyNative OnLayoutGeometryReadyNative;
	FOnPileExpansionRequestedNative OnPileExpansionRequestedNative;
	FOnPileMoveCommittedNative OnPileMoveCommittedNative;
	FOnCollapseExpandedPileRequestedNative OnCollapseExpandedPileRequestedNative;
	FOnPileCollapseAnimationFinishedNative OnPileCollapseAnimationFinishedNative;

	void SetPresentedContentZone(EZoneKind Zone, FGuid OwnerInstanceId);
	void SetInteractionModel(
		TSharedPtr<FWacomBackpackWorkspaceInteractionModel> InModel,
		UWacomBackpackWorkspaceStyle* InStyle);
	void BindWorkspaceCards(TConstArrayView<TObjectPtr<UWacomDeckCardWidget>> CardWidgets, uint64 StorageRevision);
	void ReconcilePiles(
		TConstArrayView<FWacomBackpackZonePileView> PileViews,
		TConstArrayView<FSlateRect> PileFrameRects,
		TConstArrayView<FSlateRect> PileHeaderRects,
		TConstArrayView<int32> PileLayerRanks);
	bool FindPileAtAbsolutePosition(
		FVector2D AbsolutePosition,
		EZoneKind& OutZone,
		FGuid& OutOwnerInstanceId) const;
	void SetPileDropPreview(EZoneKind Zone, FGuid OwnerInstanceId, bool bVisible, bool bRejected);
	void SetExpandedContentBounds(EZoneKind Zone, FGuid OwnerInstanceId, const FSlateRect& LocalBounds);
	bool BeginPileCollapseAnimation(EZoneKind Zone, FGuid OwnerInstanceId);
	void SetHoveredCard(FGuid InstanceId);
	void ClearHoveredCard(FGuid InstanceId);
	void SetSimplifiedMotion(bool bSimplified) { bSimplifiedMotion = bSimplified; }
	void RefreshInteractionPresentation();
	void SetCardFaceRetainedRenderingEnabled(bool bEnabled);
	void SetCarryInputSuspended(bool bSuspended);
	void CancelInteraction();
	void ApplyCardLayout(UWidget& CardWidget, FVector2D CardCenter, FVector2D CardSize, float AngleDegrees, int32 ZOrder);
	void ApplyCardBaseLayout(UWidget& CardWidget, FVector2D CardCenter, FVector2D CardSize, float AngleDegrees, int32 ZOrder);
	bool HasCardBaseLayout(const UWidget& CardWidget) const;
	void PrimeCardBaseLayout(
		UWidget& CardWidget,
		FVector2D CardCenter,
		FVector2D CardSize,
		float AngleDegrees,
		int32 ZOrder);
	void SetEmptyStateVisible(bool bVisible);
	void SetManualLayoutCount(int32 Count) { ManualLayoutCount = FMath::Max(0, Count); }
	UCanvasPanel* GetCardCanvas();
	UCanvasPanel* GetCarryCanvas();
	UCanvasPanel* GetCarryActiveCanvas();
	UCanvasPanel* GetPileCanvas();
	bool ShouldPreserveCardParent(const UWacomDeckCardWidget* CardWidget) const;
	FVector2D GetLayoutSpaceSize() const;
	void RequestLayoutGeometryRefresh();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> WorkspaceCanvas;

	/** 旧 WBP 兼容名；正式资产使用 WorkspaceCanvas。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> CardCanvas;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> PileFrameLayer;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> StaticCardLayer;

	/** 只负责平移携带扇形；缓存层和动态前卡都保持本地坐标不变。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> CarryRoot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> CarryLayer;

	/** 当前上抬卡的独立实时层；不放入静态扇形缓存。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> CarryActiveLayer;

	/**
	 * CarryLayer 的独立 Slate invalidation root。它保持本地变换不变，
	 * 由外层 CarryRoot 平移，避免指针热路径使静态扇形缓存失效。
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UInvalidationBox> CarryCache;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> SelectionMarquee;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EmptyStateText;

private:
	EZoneKind PresentedContentZone = EZoneKind::Backpack;
	FGuid PresentedContentOwnerInstanceId;
	int32 ManualLayoutCount = 0;
	int32 PileCount = 0;
	TSharedPtr<FWacomBackpackWorkspaceInteractionModel> InteractionModel;
	TSharedPtr<FWacomBackpackCardPresentationController> CardPresentationController;
	TWeakObjectPtr<UWacomBackpackWorkspaceStyle> InteractionStyle;
	TArray<TWeakObjectPtr<UWacomDeckCardWidget>> BoundCardWidgets;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UWacomBackpackZonePileWidget>> PileWidgets;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Backpack|Workspace",
		meta = (ToolTip = "工作台内嵌区域牌堆 Widget 类。只显示 ViewData 并转发标题指针意图，不直接访问 RunSession。"))
	TSubclassOf<UWacomBackpackZonePileWidget> PileWidgetClass;

	FGuid PendingCardPressId;
	FVector2D PendingPressPosition = FVector2D::ZeroVector;
	bool bPendingCardPress = false;
	bool bPendingControlDown = false;
	TWeakObjectPtr<UWacomBackpackZonePileWidget> PendingPileWidget;
	FVector2D PendingPilePressPosition = FVector2D::ZeroVector;
	FVector2D PendingPileStartPosition = FVector2D::ZeroVector;
	bool bPendingPilePress = false;
	FGuid HoveredCardInstanceId;
	EZoneKind ExpandedContentZone = EZoneKind::Backpack;
	FGuid ExpandedContentOwnerInstanceId;
	FSlateRect ExpandedContentBounds;
	bool bHasExpandedContentBounds = false;
	EZoneKind HoverExpandZone = EZoneKind::Backpack;
	FGuid HoverExpandOwnerInstanceId;
	bool bHoverExpandTimerActive = false;
	bool bSimplifiedMotion = false;
	bool bPileCollapseAnimationPending = false;
	EZoneKind CollapsingPileZone = EZoneKind::Backpack;
	FGuid CollapsingPileOwnerInstanceId;
	uint64 CurrentStorageRevision = 0;
	FVector2D CarryAnchorLocal = FVector2D::ZeroVector;
	FVector2D QueuedCarryPointerLocal = FVector2D::ZeroVector;
	FVector2D QueuedPilePointerLocal = FVector2D::ZeroVector;
	bool bCarryPointerTrackingActive = false;
	bool bPilePointerTrackingActive = false;
	bool bHasQueuedCarryPointer = false;
	bool bHasQueuedPilePointer = false;
	bool bCarryFanLayoutDirty = false;
	TArray<FGuid> LastCarryFanInstanceIds;
	/** 已提交释放、等待目标 Scene 消费的实体卡；期间禁止恢复来源基础布局。 */
	TSet<FGuid> PendingReleasedVisualHandoffs;
	int32 LastCarryFanCurrentIndex = INDEX_NONE;
	int32 LastCarryFanDefaultIndex = INDEX_NONE;
	int32 CarryFanLayoutRebuildCount = 0;
	int32 StaticCardPresentationUpdateCount = 0;
	int32 CarryVisualAnchorApplyCount = 0;
	bool bCarryInputSuspended = false;
	FVector2D StableLayoutSize = FVector2D::ZeroVector;
	FVector2D PendingLayoutSize = FVector2D::ZeroVector;
	int32 StableLayoutSampleCount = 0;
	bool bHasStableLayoutSize = false;
	bool bLayoutGeometryRefreshActive = false;
	bool bDeferredCardFaceRenderRequested = false;
	bool bDeferredCardFaceRenderActive = false;
	int32 DeferredCardFaceRenderPassCount = 0;
	bool bCardFaceRetainedRenderingEnabled = true;

	struct FBaseCardLayout
	{
		FVector2D Center = FVector2D::ZeroVector;
		FVector2D Size = FVector2D::ZeroVector;
		float AngleDegrees = 0.0f;
		int32 ZOrder = 0;
	};
	TMap<TWeakObjectPtr<UWacomDeckCardWidget>, FBaseCardLayout> BaseCardLayouts;
	struct FBaseCardLayoutTransition
	{
		FBaseCardLayout Start;
		FBaseCardLayout Current;
		FBaseCardLayout Target;
		float ElapsedSeconds = 0.0f;
		float DurationSeconds = 0.0f;
	};
	TMap<TWeakObjectPtr<UWacomDeckCardWidget>, FBaseCardLayoutTransition> BaseCardLayoutTransitions;
	bool bBaseCardLayoutTransitionActive = false;

	void EnsureFallbackTree();
	FReply HandleCardPointerDown(UWacomDeckCardWidget* CardWidget, const FGeometry& Geometry, const FPointerEvent& Event);
	FReply HandleCardPointerMove(UWacomDeckCardWidget* CardWidget, const FGeometry& Geometry, const FPointerEvent& Event);
	FReply HandleCardPointerUp(UWacomDeckCardWidget* CardWidget, const FGeometry& Geometry, const FPointerEvent& Event);
	FReply HandlePilePointerDown(UWacomBackpackZonePileWidget* PileWidget, const FGeometry& Geometry, const FPointerEvent& Event);
	FVector2D ToLocalPointer(const FPointerEvent& Event) const;
	bool TryBeginCarryFromPendingPress(FVector2D Pointer);
	bool TryBeginPileMove(FVector2D Pointer);
	void ApplyActivePileMove();
	void StartPilePointerTracking();
	void QueuePilePointer(FVector2D Pointer);
	void FlushQueuedPilePointer();
	void CommitPileMoveCardLayouts(const FWacomBackpackZoneKey& Zone, FVector2D FinalDelta);
	FWacomBackpackZoneKey ResolveMarqueeSource(FVector2D LocalPointer) const;
	void CancelHoverExpandTimer();
	void StartBaseCardLayoutTransitions();
	FReply BuildHandledPointerReply();
	void BroadcastRelease(bool bReleaseAll);
	void StartCarryPointerTracking();
	void QueueCarryPointer(FVector2D Pointer);
	void FlushQueuedCarryPointer();
	void SyncCarryPointerForRelease(FVector2D Pointer);
	void UpdateCarryAnchor(FVector2D Pointer, bool bUpdateModel = true);
	void SyncCarryLayer();
	void RebuildCarryFanLayout();
	void RestoreStaticCardParents();
	bool IsInCarryVisualLayer(const UWidget* CardWidget) const;
	void RequestBoundCardFaceRenders();
	void ScheduleBoundCardFaceRender();
	void FlushDeferredCardFaceRender();
	bool AcceptStableLayoutGeometry(FVector2D LayoutSize);

#if WITH_AUTOMATION_TESTS
public:
	FWacomBackpackWorkspaceAutomationTestView GetAutomationTestView() const;

private:
	friend struct FWacomBackpackScreenTestAccess;
#endif
};

#if WITH_AUTOMATION_TESTS

/**
 * 背包工作台 production 非反射只读测试视图。
 *
 * 只暴露稳定可观察事实；WacomTests/Private wrapper 消费本结构，不通过 Blueprint、反射或
 * 散落 ForTest getter 读取 Widget 私有字段。后续 Workspace Widget 落地时由其构造本视图。
 */
struct WACOMAPP_API FWacomBackpackWorkspaceAutomationTestView
{
	bool bHasActiveZone = false;
	EZoneKind ActiveZone = EZoneKind::Backpack;
	FGuid ActiveZoneOwnerInstanceId;
	TArray<FGuid> SelectedInstanceIds;
	TArray<FGuid> CarriedInstanceIds;
	int32 CurrentCarryIndex = INDEX_NONE;
	int32 DefaultCarryIndex = INDEX_NONE;
	int32 ManualLayoutCount = 0;
	int32 PileCount = 0;
	bool bInitialReleaseGuardArmed = false;
	bool bMouseCaptured = false;
	bool bDeleteConfirmationPending = false;
	bool bDeferredCardFaceRenderPending = false;
	int32 DeferredCardFaceRenderPassCount = 0;
	bool bCardFaceRetainedRenderingEnabled = true;
	FVector2D CarryAnchorLocal = FVector2D::ZeroVector;
	FVector2D CarryRootTranslation = FVector2D::ZeroVector;
	FVector2D CarryCacheTranslation = FVector2D::ZeroVector;
	int32 CachedCarryCardCount = 0;
	int32 ActiveCarryCardCount = 0;
	int32 CarryFanLayoutRebuildCount = 0;
	int32 StaticCardPresentationUpdateCount = 0;
	int32 CarryVisualAnchorApplyCount = 0;
	int32 ActiveBaseCardLayoutTransitionCount = 0;
	TArray<FVector2D> ActiveBaseCardLayoutTransitionTargetCenters;
	bool bHasExpandedContentBounds = false;
	bool bPileCollapseAnimationPending = false;
	EZoneKind ExpandedContentZone = EZoneKind::Backpack;
	FGuid ExpandedContentOwnerInstanceId;
};

#endif
