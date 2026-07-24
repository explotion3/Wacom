// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RunStateTypes.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "UI/Run/WacomRunMenuCardLeaseTypes.h"
#include "WacomRunFirstPersonCardSourceComponent.generated.h"

class URunSession;
class UWacomFirstPersonCardAnchorComponent;
class FWacomFirstPersonCardPresentationPrewarmController;
struct FWacomFirstPersonCardLayerTestAccess;

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomRunFirstPersonCardSourceDebugView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "当前写入 Anchor 的 runtime source id。"))
	FName SourceId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "组件配置是否允许探索期第一人称卡牌 source。"))
	bool bEnabled = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "组件当前是否处于激活状态。"))
	bool bActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "当前是否被 GameMenu 压制。"))
	bool bSuppressedByGameMenu = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "World Activity 是否冻结并收起了当前 Run 手牌。卡牌 entries、Widget 身份和 transition hints 不变。"))
	bool bWorldActivitySuppressed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "当前是否存在活动菜单卡牌租约。"))
	bool bHasActiveMenuLease = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "当前活动菜单卡牌租约 ID。"))
	FName ActiveMenuLeaseId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "当前活动菜单卡牌租约写入的 runtime source id。"))
	FName ActiveMenuLeaseSourceId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "当前活动菜单租约写入的卡牌 entry 数量。"))
	int32 ActiveMenuLeaseEntryCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "当前是否绑定了 RunSession。"))
	bool bHasRunSession = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "当前是否能解析第一人称卡牌 Anchor。"))
	bool bHasAnchor = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "最近一次默认源刷新看到的 BattleDeck 物理卡数量。"))
	int32 BattleDeckPhysicalCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "最近一次默认源刷新看到的投影入战卡数量。"))
	int32 BattleDeckProjectedCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "最近一次写入 Anchor 的卡牌 entry 数量。"))
	int32 EntryCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "最近一次刷新结果，用于区分 Applied / Skipped / Suppressed 等状态。"))
	FName LastRefreshResult = TEXT("NotAttempted");

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "最近一次 provider-backed menu lease 的租约 ID。"))
	FName LastMenuLeaseProviderLeaseId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "最近一次 provider-backed menu lease 的 source id。"))
	FName LastMenuLeaseProviderSourceId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "最近一次 provider-backed menu lease 候选重建结果。"))
	FName LastMenuLeaseProviderResult = TEXT("NotAttempted");

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "最近一次 provider-backed menu lease 的候选卡数量。"))
	int32 LastMenuLeaseProviderCandidateCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "最近一次 provider-backed menu lease 检查过的卡数量。"))
	int32 LastMenuLeaseProviderConsideredCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "最近一次 provider-backed menu lease 的调试摘要。"))
	FString LastMenuLeaseProviderDebugSummary;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "默认 Run 手牌源是否还有一次待完成的状态对齐。常见原因是 RunSession 或第一人称 Anchor 尚未就绪。"))
	bool bHasPendingDefaultSourceReconcile = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "默认 Run 手牌源最近一次待完成状态对齐的阻塞原因。"))
	FName PendingDefaultSourceBlockReason = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "菜单租约源是否还有一次待完成的状态对齐。常见原因是 RunSession 或第一人称 Anchor 尚未就绪。"))
	bool bHasPendingMenuLeaseReconcile = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "菜单租约源最近一次待完成状态对齐的阻塞原因。"))
	FName PendingMenuLeaseBlockReason = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "第一人称卡牌资源预热状态。0=Inactive，1=Loading，2=Ready，3=TimedOut，4=Failed，5=Cancelled。"))
	int32 PresentationPrewarmState = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "当前卡牌资源预热代次。"))
	int32 PresentationPrewarmGeneration = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "当前是否因资源预热冻结新的卡牌来源 Frame；探索输入不受影响。"))
	bool bPresentationSourceFrozenForPrewarm = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "当前预热所需视觉资源数量。"))
	int32 PresentationRequiredAssetCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "当前预热可选音频资源数量。"))
	int32 PresentationOptionalAssetCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "当前预热已耗时，单位为秒。"))
	float PresentationPrewarmElapsedSeconds = 0.0f;
};

#if WITH_AUTOMATION_TESTS
struct WACOMAPP_API FWacomRunFirstPersonCardSourceRefreshCountersForTest
{
	int32 RevisionSkipCount = 0;
	int32 DataBuildCount = 0;
	int32 RuntimeApplyCount = 0;

	void Reset()
	{
		RevisionSkipCount = 0;
		DataBuildCount = 0;
		RuntimeApplyCount = 0;
	}
};
#endif

/**
 * Exploration-only source bridge from RunSession BattleDeck cards to the
 * shared first-person UMG card layer.
 *
 * It is a presentation adapter only: it does not submit Run commands and does
 * not enable the battle hand click / drag interaction path.
 */
UCLASS(ClassGroup = (Wacom), meta = (BlueprintSpawnableComponent, ToolTip = "把探索期 RunSession 备战卡组显示到第一人称卡牌层；只做展示和诊断，不提交 Run 规则。"))
class WACOMAPP_API UWacomRunFirstPersonCardSourceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWacomRunFirstPersonCardSourceComponent();
	virtual ~UWacomRunFirstPersonCardSourceComponent() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards", meta = (ToolTip = "是否在探索期把 RunSession 的备战卡组写入第一人称卡牌层。关闭后会清理本 source，不影响战斗手牌 source。"))
	bool bEnableRunFirstPersonCardLayer = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards", meta = (ToolTip = "是否把 B 类特殊区中已勾选入战的投影卡也显示在探索期第一人称备战手牌中。"))
	bool bIncludeProjectedRunBattleDeckCards = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards", meta = (ToolTip = "写入 Anchor 的 runtime source id。用于和 BattleHUD 的战斗手牌 source 区分；默认使用 WacomFirstPersonCardLayerSourceIds::RunDefault()。"))
	FName RunFirstPersonCardLayerSourceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "开启后，探索期第一人称卡牌 source 刷新和清理会输出简短日志。默认关闭。"))
	bool bLogRunFirstPersonCardLayer = false;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|First Person Cards")
	void BindRunSession(URunSession* InRunSession);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|First Person Cards")
	void SetRunFirstPersonCardLayerActive(bool bInActive);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|First Person Cards")
	bool RefreshRunFirstPersonCardLayer();

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|First Person Cards")
	void ClearRunFirstPersonCardLayer();

	void ResetRunFirstPersonCardLayerMenuContext();

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|First Person Cards")
	void SetRunFirstPersonCardLayerSuppressedByGameMenu(bool bSuppressed);

	/** 收起或恢复当前默认 Run 手牌，不清空 entries、不创建 menu lease、不播放入场 hint。 */
	void SetRunFirstPersonCardLayerWorldActivitySuppressed(
		bool bSuppressed,
		bool bAnimate = true);
	bool IsRunFirstPersonCardLayerWorldActivitySuppressed() const
	{
		return bWorldActivitySuppressed;
	}

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|First Person Cards")
	bool SetRunFirstPersonCardLayerMenuLeaseFromRunCards(
		const FWacomRunMenuCardLeaseRequest& Request,
		FWacomRunMenuCardLeaseResult& OutResult);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|First Person Cards")
	bool ClearRunFirstPersonCardLayerMenuLease(FName LeaseId);

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|First Person Cards")
	bool IsRunFirstPersonCardLayerActive() const { return bRuntimeSourceActive; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|First Person Cards")
	bool HasActiveMenuLease() const { return !ActiveMenuLeaseId.IsNone(); }

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|First Person Cards")
	FName GetActiveMenuLeaseId() const { return ActiveMenuLeaseId; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|First Person Cards")
	FName GetActiveMenuLeaseSourceId() const { return ActiveMenuLeaseSourceId; }

	bool IsDefaultRunFirstPersonCardLayerSource(FName SourceId) const;
	bool IsActiveMenuLeaseSource(FName SourceId) const;
	bool IsSuppressedRunFirstPersonCardLayerSource(FName SourceId) const;
	bool CanHandleRunFirstPersonCardLayerSource(FName SourceId) const;
	bool FindCurrentRunFirstPersonCardWorkspaceEntry(
		FGuid CardInstanceId,
		FRunCardWorkspaceEntry& OutEntry) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "获取探索期第一人称卡牌 source 的只读调试快照；用于 PIE / 蓝图排查，不提交 Run 规则。"))
	FWacomRunFirstPersonCardSourceDebugView GetRunFirstPersonCardSourceDebugView() const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "获取探索期第一人称卡牌 source 的单行调试摘要；用于排查默认源、菜单租约和 revision gate。"))
	FString GetRunFirstPersonCardSourceDebugSummary() const;

	UFUNCTION(CallInEditor, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "在编辑器或 PIE 中把探索期第一人称卡牌 source 调试摘要写入 Output Log；不改变 Run 或 UI 状态。"))
	void LogRunFirstPersonCardSourceDebugSummary() const;

	bool BuildRunFirstPersonCardEntries(
		const URunSession& Run,
		TArray<FWacomFirstPersonCardLayerEntry>& OutEntries) const;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	virtual UWacomFirstPersonCardAnchorComponent* ResolveFirstPersonCardAnchor() const;

	virtual void WriteRuntimeCardLayerFrame(
		UWacomFirstPersonCardAnchorComponent& Anchor,
		const FWacomFirstPersonCardLayerPresentationFrame& Frame);

	virtual void ClearRuntimeCardLayerEntries(
		UWacomFirstPersonCardAnchorComponent& Anchor,
		FName SourceId);

private:
#if WITH_AUTOMATION_TESTS
	const FWacomRunFirstPersonCardSourceRefreshCountersForTest&
	GetDefaultSourceRefreshCountersForTest() const { return DefaultSourceRefreshCountersForTest; }
	const FWacomRunFirstPersonCardSourceRefreshCountersForTest&
	GetProviderLeaseRefreshCountersForTest() const { return ProviderLeaseRefreshCountersForTest; }
	void ResetRunFirstPersonCardSourcePerfCountersForTest();
	void SetActiveProviderLeaseRequestForTest(
		const FWacomRunMenuCardLeaseRequest& Request);
#endif

	struct FDefaultSourceRefreshKey
	{
		bool bIsValid = false;
		uint64 BackpackStorageRevision = 0;
		FName SourceId = NAME_None;
		bool bIncludeProjectedCards = false;

		void Reset()
		{
			bIsValid = false;
			BackpackStorageRevision = 0;
			SourceId = NAME_None;
			bIncludeProjectedCards = false;
		}
	};

	struct FProviderLeaseRefreshKey
	{
		bool bIsValid = false;
		uint64 BackpackStorageRevision = 0;
		FName LeaseId = NAME_None;
		FName SourceId = NAME_None;
		FWacomRunMenuCardLeaseRequest ProviderRequest;

		void Reset()
		{
			bIsValid = false;
			BackpackStorageRevision = 0;
			LeaseId = NAME_None;
			SourceId = NAME_None;
			ProviderRequest = FWacomRunMenuCardLeaseRequest();
		}
	};

	bool RefreshRunFirstPersonCardLayerInternal(
		bool bAllowDefaultSourceRevisionSkip,
		bool bAllowProviderLeaseRevisionSkip);
	bool ReconcileRunFirstPersonCardLayer(
		bool bAllowDefaultSourceRevisionSkip,
		bool bAllowProviderLeaseRevisionSkip);
	bool RefreshActiveMenuLease(bool bAllowRevisionSkip);
	bool RebuildActiveMenuLeaseFromProviderRequest();
	bool RefreshDefaultBattleDeckSource(bool bAllowRevisionSkip);
	bool TryBuildCurrentDefaultSourceRefreshKey(
		FDefaultSourceRefreshKey& OutKey) const;
	bool CanSkipDefaultSourceRefresh(
		const UWacomFirstPersonCardAnchorComponent& Anchor) const;
	void StoreDefaultSourceRefreshKey();
	void ResetDefaultSourceRefreshKey();
	bool TryBuildCurrentProviderLeaseRefreshKey(
		FProviderLeaseRefreshKey& OutKey) const;
	bool CanSkipProviderLeaseRefresh(
		const UWacomFirstPersonCardAnchorComponent& Anchor) const;
	void StoreProviderLeaseRefreshKey();
	void ResetProviderLeaseRefreshKey();
	bool AreDefaultSourceRefreshKeysEquivalent(
		const FDefaultSourceRefreshKey& Left,
		const FDefaultSourceRefreshKey& Right) const;
	bool AreProviderLeaseRefreshKeysEquivalent(
		const FProviderLeaseRefreshKey& Left,
		const FProviderLeaseRefreshKey& Right) const;
	bool AreRunMenuCardLeaseRequestsEquivalent(
		const FWacomRunMenuCardLeaseRequest& Left,
		const FWacomRunMenuCardLeaseRequest& Right) const;
	void ResetBattleDeckRefreshDebugCounts();
	void StoreRunCardWorkspaceMetadata(
		const FRunCardWorkspaceSnapshot& Snapshot);
	void ClearRunCardWorkspaceMetadata();
	FWacomFirstPersonCardLayerPresentationFrame BuildDefaultSourcePresentationFrame(
		const UWacomFirstPersonCardAnchorComponent& Anchor,
		TArray<FWacomFirstPersonCardLayerEntry>&& Entries) const;
	FWacomFirstPersonCardLayerPresentationFrame BuildRunHandEnteredPresentationFrame(
		const UWacomFirstPersonCardAnchorComponent& Anchor,
		FName SourceId,
		TArray<FWacomFirstPersonCardLayerEntry>&& Entries) const;
	FWacomFirstPersonCardLayerPresentationFrame BuildSuppressedPresentationFrame() const;
	TSet<FGuid> DetermineRunHandEnteredCardIds(
		const UWacomFirstPersonCardAnchorComponent& Anchor,
		FName SourceId,
		const TArray<FWacomFirstPersonCardLayerEntry>& Entries) const;
	TArray<FWacomFirstPersonCardLayerTransitionHint> BuildRunHandEnteredTransitionHints(
		const TArray<FWacomFirstPersonCardLayerEntry>& Entries,
		const TSet<FGuid>& CardIdsToAnimate) const;
	bool WriteSuppressedRuntimeCardLayerWithResult(FName Result);
	bool ClearVisibleRuntimeCardLayerWithResult(FName Result);
	void ClearRunFirstPersonCardLayerWithResult(FName Result, bool bClearMenuContext);
	void ClearKnownRuntimeSources(UWacomFirstPersonCardAnchorComponent& Anchor);
	void ApplyMenuLeaseInteractionOverrides(
		UWacomFirstPersonCardAnchorComponent& Anchor,
		bool bMenuLeaseSource);
	void RestoreMenuLeaseInteractionOverrides();
	void UnbindRunSession();
	void HandleRunStateChanged();
	void LogDebugState(const TCHAR* Prefix) const;
	void StoreLastMenuLeaseProviderResult(const FWacomRunMenuCardLeaseResult& Result);
	void MarkDefaultSourceReconcileBlocked(FName Reason);
	void ClearDefaultSourceReconcileBlock();
	void MarkMenuLeaseReconcileBlocked(FName Reason);
	void ClearMenuLeaseReconcileBlock();
	void ClearReconcileBlocks();
	bool StageRuntimeCardLayerFrame(
		UWacomFirstPersonCardAnchorComponent& Anchor,
		const FWacomFirstPersonCardLayerPresentationFrame& Frame);
	void TickPresentationPrewarm(float DeltaTime);
	void ResetPresentationPrewarm();

	UPROPERTY(Transient)
	TObjectPtr<URunSession> BoundRunSession = nullptr;

	UPROPERTY(Transient)
	TArray<FWacomFirstPersonCardLayerEntry> ActiveMenuLeaseEntries;

	bool bRuntimeSourceActive = false;
	bool bSuppressedByGameMenu = false;
	bool bWorldActivitySuppressed = false;
	FName ActiveMenuLeaseId = NAME_None;
	FName ActiveMenuLeaseSourceId = NAME_None;
	FWacomRunMenuCardLeaseRequest ActiveMenuLeaseProviderRequest;
	TMap<FGuid, FRunCardWorkspaceEntry> CurrentWorkspaceEntriesByCardId;
	FName CurrentWorkspaceId = NAME_None;
	ERunCardWorkspaceKind CurrentWorkspaceKind =
		ERunCardWorkspaceKind::DefaultExploration;
	FName LastWrittenRuntimeSourceId = NAME_None;
	FDefaultSourceRefreshKey LastDefaultSourceRefreshKey;
	FProviderLeaseRefreshKey LastProviderLeaseRefreshKey;
	mutable int32 LastBattleDeckPhysicalCount = 0;
	mutable int32 LastBattleDeckProjectedCount = 0;
	mutable int32 LastEntryCount = 0;
	mutable bool bLastHadAnchor = false;
	mutable FName LastRefreshResult = TEXT("NotAttempted");
	FName LastMenuLeaseProviderLeaseId = NAME_None;
	FName LastMenuLeaseProviderSourceId = NAME_None;
	FName LastMenuLeaseProviderResult = TEXT("NotAttempted");
	int32 LastMenuLeaseProviderCandidateCount = 0;
	int32 LastMenuLeaseProviderConsideredCount = 0;
	FString LastMenuLeaseProviderDebugSummary;
	bool bHasPendingDefaultSourceReconcile = false;
	FName PendingDefaultSourceBlockReason = NAME_None;
	bool bHasPendingMenuLeaseReconcile = false;
	FName PendingMenuLeaseBlockReason = NAME_None;
	struct FPendingPresentationFrame
	{
		TWeakObjectPtr<UWacomFirstPersonCardAnchorComponent> Anchor;
		FWacomFirstPersonCardLayerPresentationFrame Frame;
		uint32 Generation = 0;
		bool bValid = false;
	};
	TSharedPtr<FWacomFirstPersonCardPresentationPrewarmController> PresentationPrewarm;
	FPendingPresentationFrame PendingPresentationFrame;
	uint32 ActivePresentationAssetSetHash = 0;
#if WITH_AUTOMATION_TESTS
	FWacomRunFirstPersonCardSourceRefreshCountersForTest
		DefaultSourceRefreshCountersForTest;
	FWacomRunFirstPersonCardSourceRefreshCountersForTest
		ProviderLeaseRefreshCountersForTest;
#endif

	friend struct FWacomFirstPersonCardLayerTestAccess;
};
