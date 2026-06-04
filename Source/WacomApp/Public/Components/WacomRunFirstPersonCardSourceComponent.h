// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "GameplayTagContainer.h"
#include "WacomRunFirstPersonCardSourceComponent.generated.h"

class UCardDefinition;
class URunSession;

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomRunMenuCardLeaseRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards", meta = (ToolTip = "本次菜单卡牌租约 ID。同一个菜单重复调用应使用同一个 LeaseId；留空时菜单基类会自动生成。"))
	FName LeaseId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards", meta = (ToolTip = "写入第一人称卡牌层的 runtime source id。留空时菜单基类会自动生成。"))
	FName SourceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards", meta = (ToolTip = "允许显示的卡牌定义资产。与 AllowedCardIds 是 OR 关系；为空表示不按定义资产筛选。"))
	TArray<TObjectPtr<UCardDefinition>> AllowedCardDefinitions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards", meta = (ToolTip = "允许显示的 CardId 列表。与 AllowedCardDefinitions 是 OR 关系；为空表示不按 CardId 筛选。"))
	TArray<FName> AllowedCardIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards", meta = (ToolTip = "显式允许的卡牌实例 ID 白名单。非空时，候选卡必须命中这里的 InstanceId。"))
	TArray<FGuid> ExplicitCardInstanceIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards", meta = (ToolTip = "候选卡必须全部拥有的关键词。读取玩家持有卡实例对应定义上的 Card.Keyword。"))
	FGameplayTagContainer RequiredKeywords;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards", meta = (ToolTip = "候选卡不能拥有的关键词。命中任意一个即被排除。"))
	FGameplayTagContainer BlockedKeywords;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards", meta = (ToolTip = "是否从 Backpack 真实物理持有区收集候选卡。"))
	bool bIncludeBackpack = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards", meta = (ToolTip = "是否从 BattleDeck 真实物理持有区收集候选卡；不会包含投影入战卡。"))
	bool bIncludeBattleDeck = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards", meta = (ToolTip = "是否从 BurdenZone 真实物理持有区收集候选卡。"))
	bool bIncludeBurdenZone = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards", meta = (ToolTip = "是否从所有 SpecialZones.Cards 真实物理持有区收集候选卡；不会包含 BattleDeckProjectedCards。"))
	bool bIncludeSpecialZones = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards", meta = (ToolTip = "空筛选时是否允许显示玩家全部持有卡。默认关闭，避免菜单误把所有卡暴露到第一人称卡层。"))
	bool bAllowAllOwnedCardsWhenNoFilter = false;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomRunMenuCardLeaseResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	bool bLeaseSet = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	FName RejectReason = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	FName LeaseId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	FName SourceId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	int32 CandidateCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	int32 ConsideredCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	FString DebugSummary;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomRunFirstPersonCardSourceDebugView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	FName SourceId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	bool bEnabled = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	bool bActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	bool bSuppressedByGameMenu = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	bool bHasActiveMenuLease = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	FName ActiveMenuLeaseId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	FName ActiveMenuLeaseSourceId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	int32 ActiveMenuLeaseEntryCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	bool bHasRunSession = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	bool bHasAnchor = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	int32 BattleDeckPhysicalCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	int32 BattleDeckProjectedCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	int32 EntryCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	FName LastRefreshResult = TEXT("NotAttempted");

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	FName LastMenuLeaseProviderLeaseId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	FName LastMenuLeaseProviderSourceId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	FName LastMenuLeaseProviderResult = TEXT("NotAttempted");

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	int32 LastMenuLeaseProviderCandidateCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	int32 LastMenuLeaseProviderConsideredCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	FString LastMenuLeaseProviderDebugSummary;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	bool bActiveMenuLeaseBackedByProvider = false;
};

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards", meta = (ToolTip = "是否在探索期把 RunSession 的备战卡组写入第一人称卡牌层。关闭后会清理本 source，不影响战斗手牌 source。"))
	bool bEnableRunFirstPersonCardLayer = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards", meta = (ToolTip = "是否把 B 类特殊区中已勾选入战的投影卡也显示在探索期第一人称备战手牌中。"))
	bool bIncludeProjectedRunBattleDeckCards = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards", meta = (ToolTip = "写入 Anchor 的 runtime source id。用于和 BattleHUD 的战斗手牌 source 区分。"))
	FName RunFirstPersonCardLayerSourceId = TEXT("RunFirstPersonBattleDeck");

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

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|First Person Cards")
	void SetRunFirstPersonCardLayerSuppressedByGameMenu(bool bSuppressed);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|First Person Cards")
	bool SetRunFirstPersonCardLayerMenuLease(
		FName LeaseId,
		FName SourceId,
		const TArray<FWacomFirstPersonCardLayerEntry>& Entries);

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

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|First Person Cards")
	FWacomRunFirstPersonCardSourceDebugView GetRunFirstPersonCardSourceDebugView() const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|First Person Cards|Debug")
	FString GetRunFirstPersonCardSourceDebugSummary() const;

	UFUNCTION(CallInEditor, Category = "Wacom|Run|First Person Cards|Debug")
	void LogRunFirstPersonCardSourceDebugSummary() const;

	bool BuildRunFirstPersonCardEntries(
		const URunSession& Run,
		TArray<FWacomFirstPersonCardLayerEntry>& OutEntries) const;

#if WITH_AUTOMATION_TESTS
	int32 GetDefaultSourceRevisionSkipCountForTest() const { return DefaultSourceRevisionSkipCountForTest; }
	int32 GetDefaultSourceSnapshotBuildCountForTest() const { return DefaultSourceSnapshotBuildCountForTest; }
	int32 GetDefaultSourceApplyCountForTest() const { return DefaultSourceApplyCountForTest; }
	int32 GetProviderLeaseRevisionSkipCountForTest() const { return ProviderLeaseRevisionSkipCountForTest; }
	int32 GetProviderLeaseRebuildCountForTest() const { return ProviderLeaseRebuildCountForTest; }
	int32 GetProviderLeaseApplyCountForTest() const { return ProviderLeaseApplyCountForTest; }
	void ResetRunFirstPersonCardSourcePerfCountersForTest();
#endif

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual UWacomFirstPersonCardAnchorComponent* ResolveFirstPersonCardAnchor() const;

	virtual void WriteRuntimeCardLayerEntries(
		UWacomFirstPersonCardAnchorComponent& Anchor,
		FName SourceId,
		const TArray<FWacomFirstPersonCardLayerEntry>& Entries);

	virtual void ClearRuntimeCardLayerEntries(
		UWacomFirstPersonCardAnchorComponent& Anchor,
		FName SourceId);

private:
	bool RefreshRunFirstPersonCardLayerInternal(
		bool bAllowDefaultSourceRevisionSkip,
		bool bAllowProviderLeaseRevisionSkip);
	bool RefreshActiveMenuLease(bool bAllowRevisionSkip);
	bool RebuildActiveMenuLeaseFromProviderRequest();
	bool RefreshDefaultBattleDeckSource(bool bAllowRevisionSkip);
	bool CanSkipDefaultBattleDeckSourceRefresh(
		const UWacomFirstPersonCardAnchorComponent& Anchor) const;
	void StoreDefaultBattleDeckSourceRefreshKey();
	void ResetDefaultBattleDeckSourceRevisionGate();
	bool CanSkipProviderBackedMenuLeaseRefresh(
		const UWacomFirstPersonCardAnchorComponent& Anchor) const;
	void StoreProviderBackedMenuLeaseRefreshKey();
	void ResetProviderBackedMenuLeaseRevisionGate();
	bool AreRunMenuCardLeaseRequestsEquivalent(
		const FWacomRunMenuCardLeaseRequest& Left,
		const FWacomRunMenuCardLeaseRequest& Right) const;
	void ResetBattleDeckRefreshDebugCounts();
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

	UPROPERTY(Transient)
	TObjectPtr<URunSession> BoundRunSession = nullptr;

	UPROPERTY(Transient)
	TArray<FWacomFirstPersonCardLayerEntry> ActiveMenuLeaseEntries;

	bool bRuntimeSourceActive = false;
	bool bSuppressedByGameMenu = false;
	bool bActiveMenuLeaseBackedByProvider = false;
	bool bHasMenuLeaseClickOverride = false;
	bool bMenuLeasePreviousClickToPlayCard = true;
	bool bHasLastDefaultSourceRefreshKey = false;
	bool bLastDefaultSourceIncludedProjectedCards = false;
	bool bHasLastProviderLeaseRefreshKey = false;
	FName ActiveMenuLeaseId = NAME_None;
	FName ActiveMenuLeaseSourceId = NAME_None;
	FName LastDefaultSourceId = NAME_None;
	FName LastProviderLeaseId = NAME_None;
	FName LastProviderLeaseSourceId = NAME_None;
	FWacomRunMenuCardLeaseRequest ActiveMenuLeaseProviderRequest;
	FWacomRunMenuCardLeaseRequest LastProviderLeaseRequest;
	FName LastWrittenRuntimeSourceId = NAME_None;
	TWeakObjectPtr<UWacomFirstPersonCardAnchorComponent> MenuLeaseClickOverrideAnchor;
	uint64 LastDefaultSourceBackpackStorageRevision = 0;
	uint64 LastProviderLeaseBackpackStorageRevision = 0;
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
#if WITH_AUTOMATION_TESTS
	int32 DefaultSourceRevisionSkipCountForTest = 0;
	int32 DefaultSourceSnapshotBuildCountForTest = 0;
	int32 DefaultSourceApplyCountForTest = 0;
	int32 ProviderLeaseRevisionSkipCountForTest = 0;
	int32 ProviderLeaseRebuildCountForTest = 0;
	int32 ProviderLeaseApplyCountForTest = 0;
#endif
};
