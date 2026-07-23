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
class FWacomBackpackWorkspaceRuntime;
class FWacomBackpackWorkspaceVisualState;
class FWacomBackpackCardDetailController;
struct FWacomBackpackWorkspaceReconciler;
struct FWacomBackpackWorkspaceReleaseIntent;
enum class EWacomBackpackWorkspaceReleaseTargetKind : uint8;
enum class EWacomBackpackWorkspaceCardSemanticIcon : uint8;
enum class EWacomBackpackWorkspacePresentationDirty : uint16;
struct FWacomBackpackWorkspaceCardLayout;
struct FWacomBackpackWorkspaceCardVisualPose;
struct FWacomBackpackWorkspacePresentationRequest;
#if WITH_AUTOMATION_TESTS
struct FWacomBackpackWorkspaceAutomationTestView;
struct FWacomBackpackScreenTestAccess;
#endif

/** Reconciler 提供给 Workspace 的展开牌堆浏览合同。 */
struct WACOMAPP_API FWacomBackpackExpandedPileFocusCard
{
	TWeakObjectPtr<UWacomDeckCardWidget> Card;
	FVector2D NeutralCenter = FVector2D::ZeroVector;
	float NeutralAngleDegrees = 0.0f;
	int32 NeutralLayerRank = 0;
	/** 无焦点水平布局的命中条带。 */
	FSlateRect NeutralHitBand;
	/** 当前展开后实际目标卡位的命中条带；每次让位重排后更新。 */
	FSlateRect CurrentHitBand;
};

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
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnBrowseFocusChangedNative, UWacomDeckCardWidget*);
	DECLARE_MULTICAST_DELEGATE(FOnControlsHelpRequestedNative);
	FOnReleaseIntentNative OnReleaseIntentNative;
	FOnInteractionChangedNative OnInteractionChangedNative;
	FOnLayoutGeometryReadyNative OnLayoutGeometryReadyNative;
	FOnPileExpansionRequestedNative OnPileExpansionRequestedNative;
	FOnPileMoveCommittedNative OnPileMoveCommittedNative;
	FOnCollapseExpandedPileRequestedNative OnCollapseExpandedPileRequestedNative;
	FOnPileCollapseAnimationFinishedNative OnPileCollapseAnimationFinishedNative;
	FOnBrowseFocusChangedNative OnBrowseFocusChangedNative;
	FOnControlsHelpRequestedNative OnControlsHelpRequestedNative;

	void SetPresentedContentZone(EZoneKind Zone, FGuid OwnerInstanceId);
	void SetInteractionModel(
		TSharedPtr<FWacomBackpackWorkspaceInteractionModel> InModel,
		UWacomBackpackWorkspaceStyle* InStyle);
	void BindWorkspaceCards(TConstArrayView<TObjectPtr<UWacomDeckCardWidget>> CardWidgets, uint64 StorageRevision);
	bool FindPileAtAbsolutePosition(
		FVector2D AbsolutePosition,
		EZoneKind& OutZone,
		FGuid& OutOwnerInstanceId) const;
	bool FindPileView(
		EZoneKind Zone,
		FGuid OwnerInstanceId,
		FWacomBackpackZonePileView& OutView) const;
	bool GetFocusedReleaseTarget(
		EWacomBackpackWorkspaceReleaseTargetKind& OutKind,
		FWacomBackpackZoneKey& OutZone) const;
	void SetPileDropFeedback(
		EZoneKind Zone,
		FGuid OwnerInstanceId,
		const FWacomBackpackDropFeedbackView& Feedback);
	/** Screen 校验目标后回写携带卡的非颜色语义；Rejected 优先于 Valid。 */
	void SetCarryDropFeedbackState(bool bValid, bool bRejected);
	void SetExpandedContentBounds(EZoneKind Zone, FGuid OwnerInstanceId, const FSlateRect& LocalBounds);
	void SetExpandedPileFocusContract(
		EZoneKind Zone,
		FGuid OwnerInstanceId,
		const FSlateRect& HeaderRect,
		const FSlateRect& FocusCorridorRect,
		TConstArrayView<FWacomBackpackExpandedPileFocusCard> Cards);
	bool BeginPileCollapseAnimation(EZoneKind Zone, FGuid OwnerInstanceId);
	void SetHoveredCard(UWacomDeckCardWidget* CardWidget);
	void ClearHoveredCard(UWacomDeckCardWidget* CardWidget);
	void SetSimplifiedMotion(bool bSimplified);
	void SetCardFaceRetainedRenderingEnabled(bool bEnabled);
	void SetCarryInputSuspended(bool bSuspended);
	void CancelInteraction();
	/** Run/Scene 失效时清空所有运行时视觉与交互身份，但保留 WBP 层级本身。 */
	void ResetWorkspaceScene();
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
	UCanvasPanel* GetStaticCardLayer();
	UCanvasPanel* GetCarryCanvas();
	UCanvasPanel* GetCarryActiveCanvas();
	UCanvasPanel* GetSettlementCanvas();
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
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FNavigationReply NativeOnNavigation(
		const FGeometry& InGeometry,
		const FNavigationEvent& InNavigationEvent) override;
	virtual void NativeOnFocusLost(const FFocusEvent& InFocusEvent) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> WorkspaceCanvas;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> PileFrameLayer;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> StaticCardLayer;

	/** 成功释放和 ESC 返回使用的高层；复用原卡牌 Widget，不复制实例。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> SettlementLayer;

	/** 只负责平移携带紧凑牌列；缓存层和动态前卡都保持本地坐标不变。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> CarryRoot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> CarryLayer;

	/** 当前上抬卡的独立实时层；不放入静态牌列缓存。 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> CarryActiveLayer;

	/**
	 * CarryLayer 的独立 Slate invalidation root。它保持本地变换不变，
	 * 由外层 CarryRoot 平移，避免指针热路径使静态牌列缓存失效。
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
	TSharedPtr<FWacomBackpackWorkspaceRuntime> Runtime;
	TWeakObjectPtr<UWacomBackpackWorkspaceStyle> InteractionStyle;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Backpack|Workspace",
		meta = (ToolTip = "工作台内嵌区域牌堆 Widget 类。只显示 ViewData 并转发标题指针意图，不直接访问 RunSession。"))
	TSubclassOf<UWacomBackpackZonePileWidget> PileWidgetClass;

	uint64 CurrentStorageRevision = 0;
	int32 DeferredCardFaceRenderPassCount = 0;
	bool bCardFaceRetainedRenderingEnabled = true;
#if WITH_AUTOMATION_TESTS
	/** 兼容旧测试视图；定向管线接入后必须保持为零。 */
	int32 FullPresentationRefreshCount = 0;
	/** Screen 完成一次权威 Scene reconcile 后绑定卡牌集合的次数。 */
	int32 WorkspaceSceneBindCount = 0;
	/** 基础布局过渡的帧数；与全量刷新计数配合验证定向更新合同。 */
	int32 BaseCardLayoutTransitionTickCount = 0;
	/** 基础布局过渡实际更新的卡牌次数。 */
	int32 BaseCardLayoutTransitionApplyCount = 0;
	int32 PresentationFlushCount = 0;
	int32 NavigationTargetsApplyCount = 0;
	int32 CarryTopologyApplyCount = 0;
	int32 CarryStripApplyCount = 0;
	int32 StaticCardStageApplyCount = 0;
	int32 CardSemanticsStageApplyCount = 0;
	int32 MotionTargetApplyCount = 0;
	int32 NavigationPresentationApplyCount = 0;
	int32 AccessibilityApplyCount = 0;
	int32 PaintInvalidationApplyCount = 0;
	int32 LocalCardApplyCount = 0;
	bool bLastPresentationAppliedAllCards = false;
	TArray<FGuid> LastPresentationAppliedInstanceIds;
	int32 FrameSchedulerTickCount = 0;
#endif

	void EnsureFallbackTree();
	void PrepareForWorkspaceCardReconcile();
	void BindRegisteredWorkspaceCards(uint64 StorageRevision);
	void UnbindWorkspaceCards();
	TConstArrayView<TWeakObjectPtr<UWacomDeckCardWidget>> GetBoundCardWidgets() const;
	FReply HandleCardPointerDown(UWacomDeckCardWidget* CardWidget, const FGeometry& Geometry, const FPointerEvent& Event);
	FReply HandleCardPointerDownAtLocal(
		UWacomDeckCardWidget* CardWidget,
		FVector2D PointerLocal,
		const FPointerEvent& Event,
		bool bAllowPileHeaderReroute);
	FReply TryHandleExpandedPileVisualPointerDown(
		FVector2D PointerLocal,
		const FPointerEvent& Event);
	FReply HandleCardPointerMove(UWacomDeckCardWidget* CardWidget, const FGeometry& Geometry, const FPointerEvent& Event);
	FReply HandleCardPointerUp(UWacomDeckCardWidget* CardWidget, const FGeometry& Geometry, const FPointerEvent& Event);
	FReply HandlePilePointerDown(UWacomBackpackZonePileWidget* PileWidget, const FGeometry& Geometry, const FPointerEvent& Event);
	FVector2D ToLocalPointer(const FPointerEvent& Event) const;
	void BeginPendingPilePress(
		UWacomBackpackZonePileWidget& PileWidget,
		FVector2D LocalPointer,
		const FPointerEvent& Event,
		bool bControlDown,
		bool bOnDragHandle = false);
	UWacomBackpackZonePileWidget* FindPileHeaderAt(FVector2D LocalPointer) const;
	bool TryBeginPileHeaderPress(
		FVector2D LocalPointer,
		const FPointerEvent& Event,
		bool bControlDown);
	bool TryBeginCarryFromPendingPress(FVector2D Pointer, const FPointerEvent& Event);
	bool TryBeginPileMove(FVector2D Pointer, const FPointerEvent& Event);
	bool TryBeginMarqueeFromPendingPilePress(FVector2D Pointer, const FPointerEvent& Event);
	bool TryBeginMarqueeFromPendingBlankPress(FVector2D Pointer, const FPointerEvent& Event);
	void ApplyActivePileMove();
	void StartPilePointerTracking();
	void QueuePilePointer(FVector2D Pointer);
	void FlushQueuedPilePointer();
	void CommitPileMoveCardLayouts(const FWacomBackpackZoneKey& Zone, FVector2D FinalDelta);
	void CapturePileMoveVisualSnapshot(
		UWacomBackpackZonePileWidget& Pile,
		const FWacomBackpackZoneKey& Zone);
	void RestoreAndClearPileMoveVisualSnapshot();
	FWacomBackpackZoneKey ResolveMarqueeSource(FVector2D LocalPointer) const;
	void CancelHoverExpandTimer();
	FReply BuildHandledPointerReply();
	void BroadcastRelease(
		bool bReleaseAll,
		EWacomBackpackWorkspaceReleaseTargetKind TargetKind,
		const FWacomBackpackZoneKey& TargetZone);
	void BroadcastPointerRelease(bool bReleaseAll);
	void RequestPresentationRefresh(
		EWacomBackpackWorkspacePresentationDirty Reasons,
		TConstArrayView<FGuid> CardInstanceIds = {},
		bool bAllCards = false,
		bool bFlushImmediately = true);
	void FlushPresentationRefresh();
	void ForEachPresentationCard(
		const FWacomBackpackWorkspacePresentationRequest& Request,
		TFunctionRef<void(UWacomDeckCardWidget&)> Apply);
	void ReconcileNavigationTargets();
	void RefreshNavigationPresentation(
		const FWacomBackpackWorkspacePresentationRequest& Request);
	void RefreshCardAccessibilityPresentation(
		const FWacomBackpackWorkspacePresentationRequest& Request);
	void ApplyCardSemanticsPresentation(
		const FWacomBackpackWorkspacePresentationRequest& Request);
	void ReconcileMotionTarget();
	bool IsCardAccessibilityFocused(const UWacomDeckCardWidget& Card) const;
	EWacomBackpackWorkspaceCardSemanticIcon ResolveCardAccessibilitySemanticIcon(
		const UWacomDeckCardWidget& Card) const;
	bool HandleNavigationPrimary(bool bReleaseAll);
	bool HandleNavigationSelection();
	bool HandleNavigationContextAction();
	bool StepCarriedCard(int32 Direction);
	void RelinquishSemanticNavigationForPointerInput();
	void WakeFrameScheduler();
	void EnsureFrameSchedulerRunning();
	void StopFrameScheduler();
	void RefreshFrameWorkFromState();
	EActiveTimerReturnType TickFrameScheduler(
		uint64 TimerGeneration,
		float DeltaSeconds);
	bool TickBaseCardLayoutTransitions(float DeltaSeconds);
	void ApplyStaticCardPresentation(
		UWacomDeckCardWidget& CardWidget,
		const UWacomBackpackWorkspaceStyle& Style);
	void QueueCarryPointer(FVector2D Pointer);
	void FlushQueuedCarryPointer();
	void SyncCarryPointerForRelease(FVector2D Pointer);
	void UpdateCarryAnchor(FVector2D Pointer, bool bUpdateModel = true);
	void ApplyCarryVisualAnchor(float DeltaTime);
	void UpdateExpandedPileFocus(FVector2D PointerLocal);
	void UpdateExpandedPileLensFocus(FVector2D PointerLocal);
	void SetExpandedPileLensInputLocked(bool bLocked, bool bResumeImmediately);
	void SyncExpandedPileLensInputLockFromPointerEvent(const FPointerEvent& PointerEvent);
	void RefreshExpandedPileVisualHitAtCachedPointer();
	int32 ResolveExpandedPileVisualHitIndex(
		FVector2D PointerLocal,
		bool bAllowCurrentFocusHysteresis) const;
	void BeginExpandedPileFocusExit();
	void TickExpandedPileFocusExit(float DeltaTime);
	void SetExpandedPileFocusIndex(int32 FocusIndex);
	bool RebuildExpandedPileFocusLayout();
	void SyncExpandedPileHitLayouts(bool bUseFocusedTargets);
	void ClearExpandedPileFocus(bool bAnimateReturn, bool bBroadcastChange = true);
	void ResetExpandedPileFocusWindow(bool bAnimateReturn, bool bBroadcastChange = true);
	void BeginSelectionVisualFreeze(const FWacomBackpackZoneKey& SourceZone);
	void EndSelectionVisualFreeze(bool bAnimateReturn);
	void UpdateSelectionVisualFreezeLifetime();
	bool IsExpandedPileFocusAllowed() const;
	bool IsExpandedPileFocusCard(const UWacomDeckCardWidget* CardWidget, int32* OutIndex = nullptr) const;
	UWacomDeckCardWidget* GetPresentationFocusedCard() const;
	void RetargetCardLocalPoseFromVisual(
		UWacomDeckCardWidget& Card,
		const FWacomBackpackWorkspaceCardVisualPose& VisualPose,
		const FWacomBackpackWorkspaceCardLayout& TargetBase,
		FVector2D TargetLocalTranslation,
		float TargetLocalAngle,
		float DurationSeconds);
	void SyncCarryLayer();
	void RebuildCarryStripLayout();
	void BeginCarryPickupFeedback();
	void CaptureReleasedVisualPoses(TConstArrayView<FGuid> InstanceIds);
	FWacomBackpackWorkspaceCardVisualPose CaptureCardVisualPose(
		const UWacomDeckCardWidget& Card) const;
	bool ResolveCardDetailAnchorRect(
		const UWacomDeckCardWidget& Card,
		FSlateRect& OutWorkspaceLocalRect) const;
	void FinalizeCompletedSettlements();
	void CancelInteractionWithReturn();
	void RestoreStaticCardParents();
	bool IsInCarryVisualLayer(const UWidget* CardWidget) const;
	bool IsInSettlementVisualLayer(const UWidget* CardWidget) const;
	void RequestBoundCardFaceRenders();
	void RequestDeferredCardFaceRender();
	void ExecuteDeferredCardFaceRender();
	bool AcceptStableLayoutGeometry(FVector2D LayoutSize);
	FWacomBackpackWorkspaceRuntime& GetRuntime();
	const FWacomBackpackWorkspaceRuntime& GetRuntime() const;
	FWacomBackpackWorkspaceVisualState& GetVisualState();
	const FWacomBackpackWorkspaceVisualState& GetVisualState() const;
	const TArray<TWeakObjectPtr<UWacomBackpackZonePileWidget>>& GetRegisteredPileWidgets() const;
	friend class FWacomBackpackCardDetailController;
	friend struct FWacomBackpackWorkspaceReconciler;

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
	int32 WorkspaceCardCount = 0;
	bool bInitialReleaseGuardArmed = false;
	bool bMouseCaptured = false;
	bool bDeleteConfirmationPending = false;
	bool bDeferredCardFaceRenderPending = false;
	int32 DeferredCardFaceRenderPassCount = 0;
	bool bCardFaceRetainedRenderingEnabled = true;
	bool bSimplifiedMotion = false;
	FVector2D CarryAnchorLocal = FVector2D::ZeroVector;
	FVector2D CarryRootTranslation = FVector2D::ZeroVector;
	FVector2D CarryVisualAnchorLocal = FVector2D::ZeroVector;
	FVector2D CarryCacheTranslation = FVector2D::ZeroVector;
	int32 CachedCarryCardCount = 0;
	int32 ActiveCarryCardCount = 0;
	int32 SettlementCardCount = 0;
	int32 ActiveSettlementTargetCount = 0;
	int32 ActiveLocalMotionCardCount = 0;
	int32 RealtimeCardCount = 0;
	int32 CarryStripLayoutRebuildCount = 0;
	int32 StaticCardPresentationUpdateCount = 0;
	int32 CarryVisualAnchorApplyCount = 0;
	int32 ActiveBaseCardLayoutTransitionCount = 0;
	int32 ExpandedPileFocusIndex = INDEX_NONE;
	float ExpandedPileLensFocus = 0.0f;
	int32 ExpandedPileLensLeftStackCount = 0;
	int32 ExpandedPileLensExpandedStartIndex = INDEX_NONE;
	int32 ExpandedPileLensExpandedCardCount = 0;
	int32 ExpandedPileLensRightStackCount = 0;
	bool bExpandedPileLensInputLocked = false;
	int32 SelectionFrozenCardCount = 0;
	bool bPileMoveRollbackSnapshotActive = false;
	int32 ExpandedPileFocusLayoutRebuildCount = 0;
	bool bExpandedPileFocusExitPending = false;
	int32 FullPresentationRefreshCount = 0;
	int32 PresentationFlushCount = 0;
	int32 NavigationTargetsApplyCount = 0;
	int32 CarryTopologyApplyCount = 0;
	int32 CarryStripApplyCount = 0;
	int32 StaticCardStageApplyCount = 0;
	int32 CardSemanticsStageApplyCount = 0;
	int32 MotionTargetApplyCount = 0;
	int32 NavigationPresentationApplyCount = 0;
	int32 AccessibilityApplyCount = 0;
	int32 PaintInvalidationApplyCount = 0;
	int32 LocalCardApplyCount = 0;
	bool bLastPresentationAppliedAllCards = false;
	TArray<FGuid> LastPresentationAppliedInstanceIds;
	int32 WorkspaceSceneBindCount = 0;
	int32 BaseCardLayoutTransitionTickCount = 0;
	int32 BaseCardLayoutTransitionApplyCount = 0;
	bool bFrameSchedulerActive = false;
	uint64 FrameSchedulerGeneration = 0;
	uint64 FrameSchedulerFrameSerial = 0;
	int32 FrameSchedulerTickCount = 0;
	TArray<FVector2D> ActiveBaseCardLayoutTransitionTargetCenters;
	bool bHasExpandedContentBounds = false;
	bool bPileCollapseAnimationPending = false;
	EZoneKind ExpandedContentZone = EZoneKind::Backpack;
	FGuid ExpandedContentOwnerInstanceId;
};

#endif
