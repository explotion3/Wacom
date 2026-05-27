// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "WacomPlayerController.generated.h"

class UEnemyDefinition;
class UInputMappingContext;
class UInputAction;
class AWacomRunTunnelBranchTargetActor;
class ABattleTriggerActor;
class URunSession;
class UBattleHUD;
class UWacomRunEventDefinition;
struct FRunShopOfferInput;
struct FInputKeyEventArgs;
struct FHitResult;

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
	/** 由 ABattleTriggerActor Overlap 时调用，转发到 GameMode。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|GameFlow")
	void RequestEnterBattle(UEnemyDefinition* EnemyDef, ABattleTriggerActor* Trigger = nullptr);

	/** 由战斗 UI 在 BattleEnd 时调用，转发到 GameMode。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|GameFlow")
	void RequestExitBattle(uint8 Outcome);

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Input|Battle Debug", meta = (ToolTip = "When enabled, battle scene target click routing writes why the left-mouse Release cursor trace did or did not forward to a UWacomBattlePresentationTargetComponent. Default is off; no on-screen debug is shown."))
	bool bLogBattleSceneTargetClickRouting = false;

	/** Console command / IA 共用入口（等同于按 B）。 */
	void TryOpenBackpackFromConsole();

	/** IMC 切换统一入口。GameMode 在 EnterBattle / ExitBattle 时调用。 */
	void PushMappingContext(UInputMappingContext* IMC, int32 Priority = 0);
	void PopMappingContext(UInputMappingContext* IMC);

	/** 当前 Run 的 Session。BeginPlay 时自动创建并 Initialize。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|Run")
	URunSession* GetRunSession() const { return RunSession; }

	/** 战斗场景目标点击路由。由 InputKey 和 BattleHUD 鼠标兜底入口共用。 */
	bool TryRouteBattleSceneTargetClick(bool bRequireTargetSelect = false);

	/** Run tunnel spike branch click route. Active only while the possessed Wacom character tunnel prototype is active. */
	bool TryRouteRunTunnelBranchClick();

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

	/** 由 RunEventTriggerActor 调用：开始事件访问并 Push 事件界面。 */
	bool RequestOpenRunEvent(FName PersistentId, UWacomRunEventDefinition* EventDefinition);

	/** Console command / IA 共用入口（等同于按 E）。 */
	void TryInteractFromConsole();

	/**
	 * 候选 Trigger / GameFlowState 变化后刷新 ExplorationHUD 的 Toast。
	 * GameMode 在 EnterBattle / ExitBattle 时调用。
	 */
	void RefreshInteractToast();

protected:
	virtual void BeginPlay() override;
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
	void OnInteractPressed();

	virtual bool CanRouteBattleSceneTargetClick(UBattleHUD*& OutHUD) const;
	virtual bool BuildBattleSceneClickHitResult(FHitResult& OutHitResult) const;
	virtual bool BuildRunTunnelBranchClickHitResult(FHitResult& OutHitResult) const;

	/** 按当前候选对象计算显示的交互提示文案。 */
	FText BuildCurrentInteractPrompt() const;

	/** 从候选列表中挑距离玩家最近的有效交互对象。 */
	AActor* PickClosestInteractable() const;

private:
	/** 从 GameMode 拿当前 BattleHUD；没战斗时返回 nullptr。 */
	UBattleHUD* GetActiveBattleHUD() const;

	/** 点击手牌 index（1-based，与按键对应）。 */
	void RouteHandIndex(int32 OneBasedIndex);

	UPROPERTY(Transient)
	TObjectPtr<URunSession> RunSession = nullptr;

	/**
	 * 玩家当前在范围内的世界交互对象列表。Sphere Begin/End Overlap 维护。
	 * 按 IA_Interact 时挑距离玩家最近且可交互的一个调用接口 TryInteract。
	 */
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AActor>> CandidateInteractables;
};
