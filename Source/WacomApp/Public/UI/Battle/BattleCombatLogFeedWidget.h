// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Settings/WacomLocalSettingsTypes.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"
#include "BattleCombatLogFeedWidget.generated.h"

class UBattleCombatActivityRowWidget;
class UButton;
class UImage;
class UPanelWidget;
class USizeBox;
class UTextBlock;
class UWacomBattleCombatActivityStyle;
class UWacomSettingsSubsystem;
class FWacomBattleCombatActivityPlayback;

DECLARE_MULTICAST_DELEGATE(FWacomBattleCombatLogDetailsRequestedNative);

/** Fixed-viewport BattleHUD streaming activity broadcaster plus persistent footer. */
UCLASS(Blueprintable, meta = (ToolTip = "BattleHUD 常驻流式活动播报器。完整战斗日志由 HUD Controller 另行保存；本 Widget 只播放短时活动并发送详情打开意图。"))
class WACOMAPP_API UBattleCombatLogFeedWidget : public UWacomBattleWidgetBase
{
	GENERATED_BODY()

public:
	UBattleCombatLogFeedWidget(const FObjectInitializer& ObjectInitializer);
	virtual ~UBattleCombatLogFeedWidget() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Combat Activity|Authoring",
		meta = (ToolTip = "常驻活动播报器的图标与播放样式。为空时使用 C++ 安全默认值和通用图标。"))
	TObjectPtr<UWacomBattleCombatActivityStyle> ActivityStyle = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Combat Activity|Authoring",
		meta = (ToolTip = "活动播报单行使用的 Widget 类。为空时使用 C++ fallback UBattleCombatActivityRowWidget。"))
	TSubclassOf<UBattleCombatActivityRowWidget> ActivityRowWidgetClass;

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Combat Activity", meta = (ToolTip = "把一次已结算命令的活动批次加入非阻塞 FIFO 播报队列。不会提交或阻塞 Battle 命令。"))
	void EnqueueCombatActivityBatch(const FWacomBattleCombatActivityBatchView& Batch);

	/** C++ runtime path: begin a group at the presentation clock's semantic action boundary. */
	void BeginSynchronizedCombatActivityGroup(
		uint64 TransactionId,
		int32 GroupIndex,
		const FWacomBattleCombatActivityRowView& RootAction,
		int32 TurnNumber);
	/** C++ runtime path: release outcome rows reached by the presentation clock. */
	void ReleaseSynchronizedCombatActivityResults(
		uint64 TransactionId,
		int32 GroupIndex,
		const TArray<FWacomBattleCombatActivityRowView>& ResultRows);
	/** C++ runtime path: mark every visual group in one presentation transaction complete. */
	void CompleteSynchronizedCombatActivityTransaction(uint64 TransactionId);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Combat Activity", meta = (ToolTip = "设置 Footer 显示的表现回合数。正式 EndTurn 路径在首个新回合表现阶段开始前调用。"))
	void SetPresentedTurnNumber(int32 TurnNumber);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Combat Activity", meta = (ToolTip = "清空短时活动、Footer 与播放状态。只影响 UI。"))
	void ClearCombatActivity();

	void RestorePersistentState(int32 TurnNumber, const FWacomBattleCombatActivityRowView* LastRootAction);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Combat Activity")
	int32 GetVisibleActivityRowCount() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Combat Activity")
	int32 GetPresentedTurnNumber() const;

	FWacomBattleCombatLogDetailsRequestedNative& OnCombatLogDetailsRequestedNative()
	{
		return CombatLogDetailsRequestedNative;
	}

#if WITH_AUTOMATION_TESTS
	void AdvanceActivityPlaybackForTest(float DeltaTime);
	TArray<FWacomBattleCombatActivityRowView> GetVisibleActivityRowsForTest() const;
	const FWacomBattleCombatActivityRowView* GetLastRootActionForTest() const;
	bool IsPlaybackPendingForTest() const;
	bool IsReducedMotionForTest() const { return bRuntimeSimplifiedMotion; }
	void RequestDetailsForTest() { HandleLastActionClicked(); }
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeOnSessionChanged(UBattleSession* OldSession, UBattleSession* NewSession) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> ActivityRowsBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> ActivityRowsViewport;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> LastActionButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> TurnRoot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> TurnIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TurnText;

private:
	FWacomBattleCombatActivityPlayback* Playback = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBattleCombatActivityRowWidget>> ActivityRowWidgets;
	TMap<uint64, TWeakObjectPtr<UBattleCombatActivityRowWidget>>
		ActivityRowWidgetsByPlaybackId;
	TWeakObjectPtr<UClass> CachedActivityRowWidgetClass;

	TWeakObjectPtr<UWacomSettingsSubsystem> BoundSettingsSubsystem;
	FDelegateHandle RuntimeSettingsChangedHandle;
	bool bRuntimeSimplifiedMotion = false;
	FWacomBattleCombatLogDetailsRequestedNative CombatLogDetailsRequestedNative;

	UFUNCTION()
	void HandleLastActionClicked();

	void EnsureRuntimeBindings();
	void ApplyRuntimeGeometry();
	void EnsureRowWidgets(int32 RequiredCount);
	UBattleCombatActivityRowWidget* AcquireRowWidget(
		uint64 PlaybackId,
		TSet<UBattleCombatActivityRowWidget*>& ReservedWidgets);
	void ReleaseRetiredRowWidgets(const TSet<uint64>& VisiblePlaybackIds);
	void RefreshPlaybackPresentation();
	void RefreshFooter();
	void BindRuntimeSettings();
	void UnbindRuntimeSettings();
	void HandleRuntimeSettingsChanged(const FWacomLocalSettingsSnapshot& Snapshot, EWacomRuntimeSettingsChangeReason Reason);
	FSlateBrush ResolveActivityIconBrush(const FWacomBattleCombatActivityRowView& Row) const;
};
