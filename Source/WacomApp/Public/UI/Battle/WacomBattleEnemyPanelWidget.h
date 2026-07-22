// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CommonUserWidget.h"
#include "CoreMinimal.h"
#include "WacomBattleEnemyPanelViewData.h"
#include "WacomBattleEnemyPanelWidget.generated.h"

class UPanelWidget;
class USizeBox;
class UWacomBattleEnemyPartEntryWidget;
class UWacomSettingsSubsystem;
struct FWacomLocalSettingsSnapshot;
enum class EWacomRuntimeSettingsChangeReason : uint8;

DECLARE_MULTICAST_DELEGATE_OneParam(
	FWacomBattleEnemyPanelInspectionRequestedNative,
	const FBattlePartSlotIdentity&);

/**
 * 单个 Scene Enemy Host 的头顶聚合面板。
 *
 * 每个实例只渲染一个 FWacomBattleEnemyPanelViewData。C++ 负责稳定部位条目复用，
 * 正式 WBP 负责全部布局、皮肤和动画。
 */
UCLASS(Abstract, Blueprintable, meta = (ToolTip = "单个 Scene Enemy Host 的被动聚合面板。只消费一个 Enemy ViewData，布局由正式 WBP 提供。"))
class WACOMAPP_API UWacomBattleEnemyPanelWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void SetEnemyPanelViewData(const FWacomBattleEnemyPanelViewData& InView);

	void ClearEnemyPanelViewData();

	bool HasEnemyPanelViewData() const { return bHasCurrentView; }

	const FWacomBattleEnemyPanelViewData& GetEnemyPanelViewData() const { return CurrentView; }

	bool SetActionPreviewPartViews(const TArray<FWacomBattleEnemyPartEntryViewData>& InPreviewParts);

	void ClearActionPreview();

	void SetHoveredPartSlotId(FName InPartSlotId);

	TSubclassOf<UWacomBattleEnemyPartEntryWidget> GetPartEntryWidgetClass() const
	{
		return PartEntryWidgetClass;
	}

	/** HUD runtime 的事件驱动输入门禁；禁用时整块头顶面板点击穿透。 */
	void SetInspectionInteractionEnabled(bool bEnabled);
	bool IsInspectionInteractionEnabled() const { return bInspectionInteractionEnabled; }

	FWacomBattleEnemyPanelInspectionRequestedNative OnInspectionRequestedNative;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Panel", meta = (ToolTip = "面板内每个部位条目使用的正式 WBP 类。必须继承 UWacomBattleEnemyPartEntryWidget。"))
	TSubclassOf<UWacomBattleEnemyPartEntryWidget> PartEntryWidgetClass;

private:
	void ApplyAuthoredGeometry();
	void SyncPartEntries();
	void ClearPartEntries();
	void ApplyInspectionInteractionState();
	void ApplyRuntimePresentationPolicyToEntries();
	void BindRuntimeSettings();
	void UnbindRuntimeSettings();
	void HandleRuntimeSettingsChanged(
		const FWacomLocalSettingsSnapshot& Snapshot,
		EWacomRuntimeSettingsChangeReason Reason);
	void HandlePartInspectionRequested(const FBattlePartSlotIdentity& PartIdentity);
	UWacomBattleEnemyPartEntryWidget* FindOrCreatePartEntryWidget(
		const FWacomBattleEnemyPartEntryViewData& PartView);
	FName BuildPartEntryWidgetKey(const FWacomBattleEnemyPartEntryViewData& PartView) const;
	bool DoesPartBelongToCurrentEnemy(const FWacomBattleEnemyPartEntryViewData& PartView) const;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> PanelRoot = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> PartList = nullptr;

	UPROPERTY(Transient)
	FWacomBattleEnemyPanelViewData CurrentView;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UWacomBattleEnemyPartEntryWidget>> PartEntryWidgets;

	UPROPERTY(Transient)
	TSet<FName> AnimatedPartEntryKeys;

	UPROPERTY(Transient)
	TWeakObjectPtr<UWacomSettingsSubsystem> BoundSettingsSubsystem;

	FDelegateHandle RuntimeSettingsChangedHandle;

	FName HoveredPartSlotId = NAME_None;
	float RuntimeFlashIntensity = 1.0f;
	bool bHasCurrentView = false;
	bool bHasActionPreview = false;
	bool bSyncingPartEntries = false;
	bool bInspectionInteractionEnabled = false;
	bool bRuntimeSimplifiedMotion = false;

	friend struct FWacomBattleEnemyPanelWidgetTestAccess;
};
