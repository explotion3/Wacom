// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "WacomRunFirstPersonCardSourceComponent.generated.h"

class URunSession;

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

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|First Person Cards")
	bool IsRunFirstPersonCardLayerActive() const { return bRuntimeSourceActive; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Run|First Person Cards")
	FWacomRunFirstPersonCardSourceDebugView GetRunFirstPersonCardSourceDebugView() const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Run|First Person Cards|Debug")
	FString GetRunFirstPersonCardSourceDebugSummary() const;

	UFUNCTION(CallInEditor, Category = "Wacom|Run|First Person Cards|Debug")
	void LogRunFirstPersonCardSourceDebugSummary() const;

	bool BuildRunFirstPersonCardEntries(
		const URunSession& Run,
		TArray<FWacomFirstPersonCardLayerEntry>& OutEntries) const;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual UWacomFirstPersonCardAnchorComponent* ResolveFirstPersonCardAnchor() const;

	virtual void WriteRuntimeCardLayerEntries(
		UWacomFirstPersonCardAnchorComponent& Anchor,
		const TArray<FWacomFirstPersonCardLayerEntry>& Entries);

	virtual void ClearRuntimeCardLayerEntries(UWacomFirstPersonCardAnchorComponent& Anchor);

private:
	void ClearRunFirstPersonCardLayerWithResult(FName Result);
	void UnbindRunSession();
	void HandleRunStateChanged();
	void LogDebugState(const TCHAR* Prefix) const;

	UPROPERTY(Transient)
	TObjectPtr<URunSession> BoundRunSession = nullptr;

	bool bRuntimeSourceActive = false;
	mutable int32 LastBattleDeckPhysicalCount = 0;
	mutable int32 LastBattleDeckProjectedCount = 0;
	mutable int32 LastEntryCount = 0;
	mutable bool bLastHadAnchor = false;
	mutable FName LastRefreshResult = TEXT("NotAttempted");
};
