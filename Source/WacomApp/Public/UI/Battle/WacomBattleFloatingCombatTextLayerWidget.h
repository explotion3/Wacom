// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "WacomBattleFloatingCombatTextLayerWidget.generated.h"

class UCanvasPanel;
class UWacomSettingsSubsystem;
class UWacomBattleFloatingCombatTextEntryWidget;
class UWacomBattleFloatingCombatTextStyle;
enum class EWacomRuntimeSettingsChangeReason : uint8;
struct FWacomLocalSettingsSnapshot;
struct FWacomBattleFloatingCombatTextSpawnRequest;

/** HUD-owned、池化且不可命中的战斗飘字层。 */
UCLASS(Blueprintable, meta = (ToolTip = "BattleHUD 全屏战斗飘字层。集中推进池化 Entry，不由单条飘字 Tick，也不修改战斗状态。"))
class WACOMAPP_API UWacomBattleFloatingCombatTextLayerWidget : public UWacomBattleWidgetBase
{
	GENERATED_BODY()

public:
	virtual ~UWacomBattleFloatingCombatTextLayerWidget() override;

	void Enqueue(const TArray<FWacomBattleFloatingCombatTextSpawnRequest>& Requests);
	void TickPlayback(float DeltaTime);
	void ClearPlayback();
	bool IsPlaybackActive() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeRefreshFromSnapshot(const FBattleSnapshot& Snapshot) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> EntryCanvas = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Battle|Floating Text|Authoring",
		meta = (ToolTip = "池化飘字 Entry 的正式 WBP 类。为空时使用 C++ fallback。"))
	TSubclassOf<UWacomBattleFloatingCombatTextEntryWidget> EntryWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Battle|Floating Text|Authoring",
		meta = (ToolTip = "可选的局部 Style 覆盖。为空时读取 Wacom UI Settings 中的默认 Floating Text Style。"))
	TObjectPtr<UWacomBattleFloatingCombatTextStyle> StyleOverride = nullptr;

private:
	struct FActiveEntry;
	struct FTargetPlaybackState;
	struct FPlaybackState;
	struct FPlaybackStateDeleter
	{
		void operator()(FPlaybackState* State) const;
	};

	const UWacomBattleFloatingCombatTextStyle& ResolveStyle() const;
	UWacomBattleFloatingCombatTextEntryWidget* AcquireEntry();
	void ReleaseEntry(UWacomBattleFloatingCombatTextEntryWidget& Entry);
	void AdmitPending();
	void RefreshActive(float DeltaTime);
	void BindRuntimeSettings();
	void UnbindRuntimeSettings();
	void HandleRuntimeSettingsChanged(
		const FWacomLocalSettingsSnapshot& Snapshot,
		EWacomRuntimeSettingsChangeReason Reason);
	FVector2D ClampEntryPosition(
		const FVector2D& DesiredPosition,
		const FVector2D& EntrySize) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWacomBattleFloatingCombatTextEntryWidget>> OwnedEntries;

	TUniquePtr<FPlaybackState, FPlaybackStateDeleter> Playback;
	TWeakObjectPtr<UWacomSettingsSubsystem> BoundSettingsSubsystem;
	FDelegateHandle RuntimeSettingsChangedHandle;
	bool bReducedMotion = false;
};
