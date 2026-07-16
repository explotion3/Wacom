// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RunStateTypes.h"
#include "Types/WacomEnums.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "UI/Run/WacomRunMenuCardLeaseTypes.h"
#include "UI/Run/WacomRunMenuCardDropIntentTypes.h"
#include "WacomPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UWacomMenuWidgetBase;
class UWacomRunMenuWidgetBase;
class AWacomRunPathBranchTargetActor;
class ABattleTriggerActor;
class URunSession;
class UBattleHUD;
class UWacomRunEventDefinition;
class UWacomRunWorldInteractionTargetBridgeComponent;
class UWacomRunWorldCardDropReceiverComponent;
class UWacomRunPathTraversalComponent;
class UWacomGameViewportClient;
class UWacomRunMenuDropTargetWidget;
class UWacomAppToastSubsystem;
class FWacomRunExplorationPresentationCoordinator;
class FWacomRunPathBranchSelectionController;
class FWacomRunMapScreenFlow;
class FWacomRunSceneBindingRegistry;
class UWacomRunMapScreen;
class UWacomFirstPersonCardAnchorComponent;
class UWacomRunFirstPersonCardSourceComponent;
class UWacomCardDetailPanel;
class FWacomBattleSceneInteractionRouter;
class FWacomRunWorldInteractionRouter;
class FWacomRunFirstPersonCardDetailController;
class FWacomRunFirstPersonCardDragController;
class FWacomRunFirstPersonCardDropCoordinator;
#if WITH_AUTOMATION_TESTS
class AWacomPlayerControllerProbe;
struct FWacomPlayerControllerRunInteractionTestAccess;
#endif
#if WITH_DEV_AUTOMATION_TESTS
struct FWacomRunFloorSceneBindingAutomationTestView;
#endif
struct FWacomCardDetailViewData;
struct FWacomFirstPersonViewStageRequest;
struct FRunShopOfferInput;
struct FInputKeyEventArgs;
struct FHitResult;
struct FWacomExplorationScreenRouter;
struct FRunExplorationResolution;

/**
 * Wacom PlayerController。
 *
 * 职责：
 *   - 持有 URunSession
 *   - 管理 Enhanced Input 的 MappingContext 切换（IMC_Exploration <-> IMC_Battle）
 *   - 绑定战斗相关 IA（1..7 / W / E / ESC）到内部回调，转发到当前 BattleHUD
 *   - 把世界交互对象的请求转发给对应系统（战斗 / 商店等）
 *   - 把战斗 UI 的"退出战斗请求"转发给 GameMode
 *
 * 为什么由 Controller 绑定战斗 IA：
 *   - 玩家 Pawn 已绑定探索期 IA（Move/Look）
 *   - 战斗 IA 和当前 Pawn 无关，而且 HUD 是 Widget 不能绑 IA
 *   - Controller 持久存在，HUD 可能动态创建销毁，绑在 Controller 最稳
 *   - 战斗 IMC Pop 后按键无 mapping，IA 不会触发（无需运行时解除绑定）
 */
UCLASS(Blueprintable)
class WACOMAPP_API AWacomPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AWacomPlayerController();

	/** 由 ABattleTriggerActor 交互时调用，转发到 GameMode。Trigger 必须携带 EncounterDefinition。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|GameFlow")
	void RequestEnterBattle(ABattleTriggerActor* Trigger);

	/** 外部手动结束战斗时调用，转发到 GameMode；正式 BattleEnd 主链路由 BattleHUD typed signal 驱动。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|GameFlow")
	void RequestExitBattle(EBattleOutcome Outcome);

	virtual void SetPawn(APawn* InPawn) override;

	// ---- IMC 资产（LoadObject 填默认，蓝图可覆盖）----

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputMappingContext> ExplorationMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputMappingContext> BattleMappingContext;

	// ---- 战斗 IA（LoadObject 填默认）----

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_PlayCard1;
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_PlayCard2;
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_PlayCard3;
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_PlayCard4;
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_PlayCard5;
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_PlayCard6;
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_PlayCard7;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_Wait;
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_EndTurn;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_OpenMenu;

	/**
	 * 交互键（默认 E）。探索期在世界交互对象范围内按此键触发最近对象。
	 *
	 * 默认资产由 BeginPlay/SetupInputComponent 懒加载；未加载时控制台命令
	 * `Wacom.Interact` 可作为调试入口。
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_Interact;

	/**
	 * 打开背包 IA。
	 *
	 * 默认资产由懒加载路径填充；控制台命令 Wacom.OpenBackpack / Wacom.CloseBackpack
	 * 保留为调试入口。战斗 IMC 不绑定该 IA。
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
	TObjectPtr<UInputAction> IA_OpenBackpack;

	/** 打开当前 Floor 地图（默认 M / 手柄 View）；仅在 Exploration Anchored 且没有互斥菜单或事务时生效。 */
	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input",
		meta = (ToolTip = "打开当前 Floor 地图的输入动作。默认映射 M 与手柄 View；地图开启期间由 CommonUI 直接处理关闭和确认。"))
	TObjectPtr<UInputAction> IA_OpenMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Input|Battle Debug", meta = (ToolTip = "开启后，战斗场景目标点击路由会输出 cursor trace 和句柄转发日志。默认关闭，不显示屏幕调试输出。"))
	bool bLogBattleSceneTargetClickRouting = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Input|Run World Target", meta = (ToolTip = "开启后，探索期会低频 probe 鼠标下方的 Run World Target，并播放轻量预览。不会提交 Run 规则或替代 E 交互。"))
	bool bEnableRunWorldTargetProbePreview = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Input|Run World Target", meta = (ToolTip = "Run World Target 鼠标 probe 预览的刷新间隔，单位秒。", ClampMin = "0.01", ClampMax = "1.0", UIMin = "0.02", UIMax = "0.2"))
	float RunWorldTargetProbePreviewIntervalSeconds = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Input|Run World Target|Debug", meta = (ToolTip = "开启后，Run World Target probe 预览切换会输出 handle 和清理日志。默认关闭。"))
	bool bLogRunWorldTargetProbePreview = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Input|Run World Target", meta = (ToolTip = "开启后，探索期鼠标左键点击实现 Run world clickable 合同的世界目标会走 IWacomWorldInteractable 交互入口。不会替代 E 键。"))
	bool bEnableRunWorldInteractableClick = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Input|Run World Target|Debug", meta = (ToolTip = "开启后，Run World Target 点击路由会输出命中、拒绝原因和交互结果。默认关闭。"))
	bool bLogRunWorldInteractableClick = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Input|Run World Target", meta = (ToolTip = "开启后，探索期鼠标 hover 到实现 Run world clickable 合同的世界目标时，会在 ExplorationHUD 交互提示位显示点击提示。"))
	bool bEnableRunWorldInteractableHoverPrompt = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Input|Run World Target|Debug", meta = (ToolTip = "开启后，Run World Target hover 提示切换会输出目标、提示文案和拒绝原因。默认关闭。"))
	bool bLogRunWorldInteractableHoverPrompt = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Input|Run World Card Drop", meta = (ToolTip = "开启后，探索期第一人称手牌可拖拽到实现 Run world card drop receiver 的场景目标。菜单卡牌租约仍优先处理 UI Zone。"))
	bool bEnableRunWorldCardDrop = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Input|Run World Card Drop|Debug", meta = (ToolTip = "开启后，探索期拖卡到场景目标会输出预览、释放和拒绝原因。默认关闭。"))
	bool bLogRunWorldCardDrop = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards|Detail", meta = (ToolTip = "Run 第一人称手牌悬浮详情面板类。为空时会尝试加载 WBP_CardDetailPanel；加载失败则使用 C++ 默认 UWacomCardDetailPanel。"))
	TSubclassOf<UWacomCardDetailPanel> RunFirstPersonCardDetailPanelClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards|Detail", meta = (ToolTip = "Run 第一人称手牌悬浮详情面板的估算宽高，单位为 Slate 像素。用于 viewport clamp 和 SetDesiredSizeInViewport；建议从 300x380 到 440x520 区间调试。"))
	FVector2D RunFirstPersonCardDetailPanelEstimatedSize = FVector2D(360.0f, 420.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards|Detail", meta = (ToolTip = "Run 第一人称手牌悬浮详情面板与卡牌之间的间距，单位为 Slate 像素。面板默认显示在卡牌左侧，左侧空间不足时换到右侧。"))
	float RunFirstPersonCardDetailPanelPadding = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards|Detail", meta = (ToolTip = "Run 第一人称手牌悬浮详情面板添加到 Viewport 时使用的层级。需要高于 FirstPersonCardAnchorComponent.CardLayerZOrder，避免详情被卡牌层遮挡。"))
	int32 RunFirstPersonCardDetailViewportZOrder = 9999;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards|Detail", meta = (ToolTip = "Run 第一人称手牌详情定位时使用的卡牌锚点基础尺寸，单位为 UMG 布局像素。通常应与 WBP_FPCardView 或 WBP_CardView 的设计尺寸一致。"))
	FVector2D RunFirstPersonCardDetailAnchorBaseSize = FVector2D(296.0f, 420.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards|Detail|Motion", meta = (ToolTip = "开启后，Run 第一人称手牌详情使用预创建面板、淡入淡出、轻微缩放和跟随运动。关闭时回到立即显示/隐藏，主要用于排查表现问题。"))
	bool bEnableRunFirstPersonCardDetailReadabilityPolish = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards|Detail|Motion", meta = (ToolTip = "Run 第一人称手牌 hover 或 inspect 后显示详情前的延迟，单位为秒。建议 0.05 到 0.18，用于避免突兀闪现。"))
	float RunFirstPersonCardDetailHoverDelaySeconds = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards|Detail|Motion", meta = (ToolTip = "Run 第一人称手牌详情淡入插值速度，单位为 1/秒。值越大出现越快；建议 12 到 24。"))
	float RunFirstPersonCardDetailFadeInSpeed = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards|Detail|Motion", meta = (ToolTip = "Run 第一人称手牌详情淡出插值速度，单位为 1/秒。值越大消失越快；建议 16 到 32。"))
	float RunFirstPersonCardDetailFadeOutSpeed = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards|Detail|Motion", meta = (ToolTip = "Run 第一人称手牌详情跟随卡槽目标位置的插值速度，单位为 1/秒。值越大越贴近目标；建议 18 到 32。"))
	float RunFirstPersonCardDetailFollowSpeed = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards|Detail|Motion", meta = (ToolTip = "Run 第一人称手牌详情出现时的起始缩放。1 表示不缩放；建议 0.94 到 0.99，用于减轻突然弹出的感觉。"))
	float RunFirstPersonCardDetailAppearStartScale = 0.97f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards|Detail|Motion", meta = (ToolTip = "Run 第一人称手牌详情左右换边的滞后距离，单位为 Slate 像素。建议 48 到 96，避免卡牌轻微移动时面板左右跳。"))
	float RunFirstPersonCardDetailSideSwitchHysteresisPixels = 72.0f;

	/** Console command / IA 共用入口（等同于按 B）。 */
	void TryOpenBackpackFromConsole();

	/** Console command / IA 共用入口（等同于按 M）。 */
	void TryToggleRunMapFromConsole();

	/** IMC 切换统一入口。GameMode 在 EnterBattle / ExitBattle 时调用。 */
	void PushMappingContext(UInputMappingContext* IMC, int32 Priority = 0);
	void PopMappingContext(UInputMappingContext* IMC);

	/** 当前 Run 的 Session。BeginPlay 时自动创建并 Initialize。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run")
	URunSession* GetRunSession() const { return RunSession; }

	/**
	 * 将 GameMode 持有的节点活动规则结果显式交给 Run 表现协调器。
	 * 返回 false 表示结果未按顺序应用；此时函数会重建绑定作为可玩性恢复，但调用方仍应处理失败。
	 */
	bool ApplyRunNodeActivityResolutionForPresentation(
		const FRunExplorationResolution& Resolution);

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|First Person Cards")
	UWacomRunFirstPersonCardSourceComponent* GetRunFirstPersonCardSourceComponent() const
	{
		return RunFirstPersonCardSourceComponent;
	}

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|First Person Cards")
	void SetRunFirstPersonCardLayerActive(bool bActive);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|First Person Cards")
	bool RefreshRunFirstPersonCardLayer();

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|First Person Cards")
	void ClearRunFirstPersonCardLayer();

	void PrepareExplorationRunFirstPersonCardLayer();

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|First Person Cards")
	void SetRunFirstPersonCardLayerSuppressedByGameMenu(bool bSuppressed);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|First Person Cards")
	bool SetRunFirstPersonCardLayerMenuLeaseFromRunCards(
		const FWacomRunMenuCardLeaseRequest& Request,
		FWacomRunMenuCardLeaseResult& OutResult);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|First Person Cards")
	bool ClearRunFirstPersonCardLayerMenuLease(FName LeaseId);

	void RegisterActiveGameMenuWidget(UWacomMenuWidgetBase* MenuWidget);
	void UnregisterActiveGameMenuWidget(UWacomMenuWidgetBase* MenuWidget);
	void SetRunFirstPersonCardLayerTransitionSuppressedByGameMenu(bool bSuppressed);
	void RegisterRunMenuDropTarget(UWacomRunMenuDropTargetWidget* DropTarget);
	void UnregisterRunMenuDropTarget(UWacomRunMenuDropTargetWidget* DropTarget);

	/** 战斗场景目标点击路由。由 InputKey 和 BattleHUD 鼠标兜底入口共用。 */
	bool TryRouteBattleSceneTargetClick(bool bRequireTargetSelect = false);
	bool TryProbeBattleSceneInteractionTarget(FWacomInteractionTargetHandle& OutHandle) const;
	bool TryProbeBattleSceneInteractionTargetAtWidgetPosition(
		const FVector2D& WidgetPosition,
		FWacomInteractionTargetHandle& OutHandle) const;

	bool TryProbeRunSceneInteractionTarget(FWacomInteractionTargetHandle& OutHandle) const;
	bool TryProbeRunSceneInteractionTargetAtWidgetPosition(
		const FVector2D& WidgetPosition,
		FWacomInteractionTargetHandle& OutHandle) const;

	/** Exploration 下点击 Run World Target，并把实现 clickable 合同的 Actor 转发到现有 IWacomWorldInteractable 入口。 */
	bool TryRouteRunWorldInteractableClick();

	UFUNCTION(BlueprintPure, Category = "Wacom|Input|Run World Target|Debug",
		meta = (ToolTip = "返回当前 Run world hover 提示的一行诊断摘要。只读，不修改 hover 或 RunState。"))
	FString GetRunWorldInteractableHoverDebugSummary() const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Input|Run World Target|Debug",
		meta = (ToolTip = "把当前 Run world hover 提示诊断摘要写入日志。"))
	void LogRunWorldInteractableHoverDebugSummary() const;

	/** Run Path 分支点击路由；只有正式 Coordinator 绑定且当前锚定在节点时可提交。 */
	bool TryRouteRunPathBranchClick();

	// ---- 候选交互对象（use-key 模型）----

	/** 世界交互对象进入玩家交互范围时调用。 */
	void RegisterCandidateInteractable(AActor* InteractableActor);

	/** 世界交互对象离开玩家交互范围或销毁时调用。 */
	void UnregisterCandidateInteractable(AActor* InteractableActor);

	/** Trigger 进入玩家 Sphere 时调用（由 BattleTriggerActor::HandleBeginOverlap）。兼容旧调用点。 */
	void RegisterCandidateTrigger(ABattleTriggerActor* Trigger);

	/** Trigger 离开玩家 Sphere 或销毁时调用。兼容旧调用点。 */
	void UnregisterCandidateTrigger(ABattleTriggerActor* Trigger);

	/** 由 ShopTriggerActor 调用：开始商店访问并 Push 商店界面。 */
	bool RequestOpenShop(FName ShopId, const TArray<FRunShopOfferInput>& Offers);
	bool RequestOpenShop(
		FName ShopId,
		const TArray<FRunShopOfferInput>& Offers,
		const FWacomFirstPersonViewStageRequest& StageRequest);
	bool IsGameMenuViewpointStageTransitionActive() const { return bGameMenuViewpointStageTransitionActive; }
	bool IsGameMenuViewpointReturnArmed() const { return bGameMenuViewpointReturnArmed; }
	void BeginGameMenuViewpointStageTransition(FName DebugReason);
	void ArmGameMenuViewpointReturnForMenu(UWacomMenuWidgetBase* MenuWidget);
	void ReturnFromGameMenuViewpointStageAfterFailedOpen();

	/** 由 RunEventTriggerActor 调用：开始事件访问并 Push 事件界面。 */
	bool RequestOpenRunEvent(FName PersistentId, UWacomRunEventDefinition* EventDefinition);
	bool RequestOpenRunEvent(
		FName PersistentId,
		UWacomRunEventDefinition* EventDefinition,
		const FWacomFirstPersonViewStageRequest& StageRequest);

	/** Console command / IA 共用入口（等同于按 E）。 */
	void TryInteractFromConsole();

	/**
	 * 候选 Trigger / GameFlowState 变化后刷新 ExplorationHUD 的 Toast。
	 * GameMode 在 EnterBattle / ExitBattle 时调用。
	 */
	void RefreshInteractToast();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;
	virtual bool InputKey(const FInputKeyEventArgs& Params) override;

	// IA 回调：路由到当前 BattleHUD
	void OnPlayCard1();
	void OnPlayCard2();
	void OnPlayCard3();
	void OnPlayCard4();
	void OnPlayCard5();
	void OnPlayCard6();
	void OnPlayCard7();
	void OnWaitPressed();
	void OnEndTurnPressed();
	void OnOpenMenuPressed();
	void OnOpenBackpackPressed();
	void OnOpenMapPressed();
	void OnInteractPressed();

	virtual bool CanRouteBattleSceneTargetClick(UBattleHUD*& OutHUD) const;
	virtual bool BuildBattleSceneClickHitResult(FHitResult& OutHitResult) const;
	virtual bool BuildBattleSceneInteractionTargetHitResultAtWidgetPosition(
		const FVector2D& WidgetPosition,
		FHitResult& OutHitResult) const;
	virtual bool BuildRunSceneClickHitResult(FHitResult& OutHitResult) const;
	virtual bool BuildRunSceneInteractionTargetHitResultAtWidgetPosition(
		const FVector2D& WidgetPosition,
		FHitResult& OutHitResult) const;
	virtual bool BuildRunPathBranchClickHitResult(FHitResult& OutHitResult) const;
	virtual bool IsInExplorationFlow() const;
	/** Run 场景指针路由是否拥有当前左键；GameMenu 与其过渡阶段必须优先。 */
	bool CanRouteRunScenePointerInput() const;
	void UpdateRunWorldTargetProbePreview();
	void ClearRunWorldTargetProbePreview();
	void ClearRunMenuDropTargetProbe();
	void HandleRunFirstPersonCardLayerCardHovered(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleRunFirstPersonCardLayerCardUnhovered(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleRunFirstPersonCardLayerHoveredCardLayoutUpdated(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleRunFirstPersonCardLayerPointerMoved(const FWacomFirstPersonCardPointerView& PointerView);
	void HandleRunFirstPersonCardLayerPointerLeft();
	void HandleRunFirstPersonCardLayerDragStarted(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void HandleRunFirstPersonCardLayerDragUpdated(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void HandleRunFirstPersonCardLayerDragReleased(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void HandleRunFirstPersonCardLayerDragCancelled(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	virtual UWacomAppToastSubsystem* ResolveAppToastSubsystem() const;

	/** 按当前候选对象计算显示的交互提示文案。 */
	FText BuildCurrentInteractPrompt() const;

	/** 从候选列表中挑距离玩家最近的有效交互对象。 */
	AActor* PickClosestInteractable() const;

	virtual URunSession* ResolveRunSessionForFirstPersonCardSource() const;

private:
	void ApplyRunFirstPersonCardPointerCameraLookOverride(
		const FWacomFirstPersonCardPointerView& PointerView);
	void ClearRunFirstPersonCardPointerCameraLookOverride();
	void ApplyRunFirstPersonCardDragCameraLookOverride(
		const FWacomFirstPersonCardDragView& DragView);
	void ClearRunFirstPersonCardDragCameraLookOverride();
	bool RefreshRunExplorationPresentationBinding();
	void TeardownRunExplorationPresentationBinding();
	void HandleRunPathBranchRequested(FName EdgeId);
	void HandleRunPathAnchoredForwardIntent();
	void HandleRunPathAnchoredHorizontalIntent(int32 Direction);
	void HandleRunRouteChoiceStateChanged(const struct FWacomRunRouteChoiceState& State);
	bool CanPresentRunMapScreen(
		bool& bOutPreferRecommendedTarget,
		FName* OutRejectDetail = nullptr) const;
	int32 BeginRunMapScreenOpenRequest();
	bool IsRunMapScreenOpenRequestCurrent(int32 RequestGeneration) const;
	bool AttachRunMapScreen(UWacomRunMapScreen& Screen, int32 RequestGeneration);
	void CancelRunMapScreenOpenRequest(int32 RequestGeneration);

	/** 从 GameMode 拿当前 BattleHUD；没战斗时返回 nullptr。 */
	UBattleHUD* GetActiveBattleHUD() const;
	FWacomBattleSceneInteractionRouter& GetBattleSceneInteractionRouter();
	const FWacomBattleSceneInteractionRouter& GetBattleSceneInteractionRouter() const;
	FWacomRunWorldInteractionRouter& GetRunWorldInteractionRouter();
	const FWacomRunWorldInteractionRouter& GetRunWorldInteractionRouter() const;

	bool TryProbeRunMenuDropTargetAtWidgetPosition(
		const FVector2D& WidgetPosition,
		FWacomInteractionTargetHandle& OutHandle) const;

#if WITH_AUTOMATION_TESTS
	FString GetRunMenuDropProbeDebugSummaryForTest() const;
	FString GetRunWorldCardDropDebugSummaryForTest() const;
	bool ApplyRunMenuDropProbeFeedbackForTest(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView,
		bool bReleased);
	bool ApplyRunWorldCardDropProbeFeedbackForTest(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView,
		bool bReleased);
	FWacomRunMenuCardDropResolveResult ResolveRunMenuCardDropIntentForTest(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView) const;
	FRunWorldCardInteractionValidation ResolveRunWorldCardDropIntentForTest(
		const FGuid& CardInstanceId,
		const FWacomFirstPersonCardDragView& DragView,
		FWacomInteractionTargetHandle& OutTargetHandle,
		AActor*& OutTargetActor,
		UWacomRunWorldInteractionTargetBridgeComponent*& OutTargetBridge,
		UWacomRunWorldCardDropReceiverComponent*& OutReceiver,
		FString& OutDebugSummary) const;
	bool IsRunFirstPersonCardDetailPanelVisibleForTest() const;
	FText GetRunFirstPersonCardDetailPanelNameTextForTest() const;
	FVector2D GetRunFirstPersonCardDetailPanelPositionForTest() const;
	bool IsRunFirstPersonCardDetailPanelPrewarmedForTest() const;
	bool IsRunFirstPersonCardDetailMotionPendingForTest() const;
	float GetRunFirstPersonCardDetailPanelOpacityForTest() const;
	int32 GetRunFirstPersonCardDetailDataApplyCountForTest() const;
	void TickRunFirstPersonCardDetailForTest(float DeltaTime);
#endif

	/** 点击手牌 index（1-based，与按键对应）。 */
	void RouteHandIndex(int32 OneBasedIndex);

	void StartRunWorldTargetProbePreviewLoop();
	void StopRunWorldTargetProbePreviewLoop();
	bool ResolveRunWorldClickableInteractableFromHandle(
		const FWacomInteractionTargetHandle& Handle,
		AActor*& OutInteractableActor,
		UWacomRunWorldInteractionTargetBridgeComponent*& OutBridge,
		FName& OutRejectReason) const;
	UWacomRunWorldCardDropReceiverComponent* ResolveRunWorldCardDropReceiverFromHandle(
		const FWacomInteractionTargetHandle& Handle) const;
	void ClearRunWorldCardDropProbe();
	void RefreshRunFirstPersonCardLayerMenuSuppression();
	void CompactActiveGameMenuWidgets();
	void RemoveActiveGameMenuWidget(UWacomMenuWidgetBase* MenuWidget);
	bool HasAnyActiveGameMenuWidget() const;
	bool HasActiveRunGameMenuOrTransitionSuppression() const;
	void FinishGameMenuViewpointStageTransition();
	FWacomRunFirstPersonCardDetailController& GetRunFirstPersonCardDetailController();
	const FWacomRunFirstPersonCardDetailController& GetRunFirstPersonCardDetailController() const;
	bool BuildRunFirstPersonCardDetailViewData(
		const FGuid& CardInstanceId,
		FWacomCardDetailViewData& OutDetailData) const;
	void PrewarmRunFirstPersonCardDetailPanel();
	void RefreshRunFirstPersonCardDetailBinding();
	void HideRunFirstPersonCardDetailPanel();
	FWacomRunFirstPersonCardDragController& GetRunFirstPersonCardDragController();
	const FWacomRunFirstPersonCardDragController& GetRunFirstPersonCardDragController() const;
	FWacomRunFirstPersonCardDropCoordinator& GetRunFirstPersonCardDropCoordinator();
	const FWacomRunFirstPersonCardDropCoordinator& GetRunFirstPersonCardDropCoordinator() const;
	void RefreshRunFirstPersonMenuLeaseDragBinding();
	void PumpFirstPersonCardActiveDragPointer();
	bool TryReleaseFirstPersonCardActiveDragPointer();
	bool TryCancelFirstPersonCardKeyboardShortcutDrag();
	bool TryCancelFirstPersonCardActiveGestureForTurnBoundaryShortcut();
	bool TryGetMouseWidgetPosition(FVector2D& OutWidgetPosition);
	UWacomFirstPersonCardAnchorComponent* ResolveFirstPersonCardAnchorForRunMenuProbe() const;
	bool ShouldHandleRunFirstPersonMenuDropProbe() const;
	bool ShouldHandleRunWorldCardDropProbe() const;
	UWacomRunMenuWidgetBase* ResolveOwningMenuForActiveRunMenuLease(FName LeaseId) const;
	UPROPERTY(Transient)
	TObjectPtr<URunSession> RunSession = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wacom|Run|First Person Cards",
		meta = (AllowPrivateAccess = "true", ToolTip = "探索期第一人称卡牌 source bridge。把 RunSession 备战卡组写入 FirstPersonCardAnchor；只做展示，不提交 Run 规则。"))
	TObjectPtr<UWacomRunFirstPersonCardSourceComponent> RunFirstPersonCardSourceComponent = nullptr;

	/**
	 * 玩家当前在范围内的世界交互对象列表。Sphere Begin/End Overlap 维护。
	 * 按 IA_Interact 时挑距离玩家最近且可交互的一个调用接口 TryInteract。
	 */
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AActor>> CandidateInteractables;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<UWacomMenuWidgetBase>> ActiveGameMenuWidgets;

	UPROPERTY(Transient)
	TWeakObjectPtr<UWacomMenuWidgetBase> GameMenuViewpointReturnWidget;

	UPROPERTY(Transient)
	TObjectPtr<UWacomCardDetailPanel> RunFirstPersonCardDetailPanel = nullptr;

	TSharedPtr<FWacomBattleSceneInteractionRouter> BattleSceneInteractionRouter;
	TSharedPtr<FWacomRunWorldInteractionRouter> RunWorldInteractionRouter;
	TSharedPtr<FWacomRunFirstPersonCardDetailController> RunFirstPersonCardDetailController;
	TSharedPtr<FWacomRunFirstPersonCardDragController> RunFirstPersonCardDragController;
	TSharedPtr<FWacomRunFirstPersonCardDropCoordinator> RunFirstPersonCardDropCoordinator;
	TSharedPtr<FWacomRunSceneBindingRegistry> RunExplorationSceneBindingRegistry;
	TSharedPtr<FWacomRunExplorationPresentationCoordinator> RunExplorationPresentationCoordinator;
	TSharedPtr<FWacomRunPathBranchSelectionController> RunPathBranchSelectionController;
	TSharedPtr<FWacomRunMapScreenFlow> RunMapScreenFlow;
	TWeakObjectPtr<UWacomRunPathTraversalComponent> BoundRunPathTraversal;
	TArray<TWeakObjectPtr<AWacomRunPathBranchTargetActor>> BoundRunPathBranchTargets;
	uint64 RunExplorationSceneBindingGeneration = 0;
	FName RunExplorationSceneBindingLastFailureDetail = NAME_None;
#if WITH_DEV_AUTOMATION_TESTS
	FName RunExplorationSceneBindingPreCommitFaultForAutomation = NAME_None;
#endif

	bool bRunFirstPersonCardLayerTransitionSuppressedByGameMenu = false;
	bool bGameMenuViewpointStageTransitionActive = false;
	bool bGameMenuViewpointReturnArmed = false;

	FTimerHandle RunWorldTargetProbePreviewTimerHandle;

	friend class FWacomBattleSceneInteractionRouter;
	friend class FWacomRunWorldInteractionRouter;
	friend class FWacomRunFirstPersonCardDetailController;
	friend class FWacomRunFirstPersonCardDragController;
	friend class UWacomGameViewportClient;
	friend struct FWacomExplorationScreenRouter;
#if WITH_AUTOMATION_TESTS
	friend class AWacomPlayerControllerProbe;
	friend struct FWacomPlayerControllerRunInteractionTestAccess;
#endif
#if WITH_DEV_AUTOMATION_TESTS
	friend struct FWacomRunFloorSceneBindingAutomationTestView;
#endif
};
