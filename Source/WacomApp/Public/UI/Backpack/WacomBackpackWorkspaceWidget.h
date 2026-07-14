// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RunStateTypes.h"
#include "WacomBackpackWorkspaceWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UTextBlock;
class UWacomBackpackWorkspaceStyle;
class UWacomDeckCardWidget;
class FWacomBackpackWorkspaceInteractionModel;
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
	FOnReleaseIntentNative OnReleaseIntentNative;
	FOnInteractionChangedNative OnInteractionChangedNative;

	void SetActiveZone(EZoneKind Zone, FGuid OwnerInstanceId);
	void SetInteractionModel(
		TSharedPtr<FWacomBackpackWorkspaceInteractionModel> InModel,
		UWacomBackpackWorkspaceStyle* InStyle);
	void BindWorkspaceCards(TConstArrayView<TObjectPtr<UWacomDeckCardWidget>> CardWidgets, uint64 StorageRevision);
	void RefreshInteractionPresentation();
	void CancelInteraction();
	void ApplyCardLayout(UWidget& CardWidget, FVector2D CardCenter, FVector2D CardSize, float AngleDegrees, int32 ZOrder);
	void SetEmptyStateVisible(bool bVisible);
	void SetManualLayoutCount(int32 Count) { ManualLayoutCount = FMath::Max(0, Count); }
	UCanvasPanel* GetCardCanvas();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> CardCanvas;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> SelectionMarquee;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EmptyStateText;

private:
	EZoneKind ActiveZone = EZoneKind::Backpack;
	FGuid ActiveZoneOwnerInstanceId;
	int32 ManualLayoutCount = 0;
	TSharedPtr<FWacomBackpackWorkspaceInteractionModel> InteractionModel;
	TWeakObjectPtr<UWacomBackpackWorkspaceStyle> InteractionStyle;
	TArray<TWeakObjectPtr<UWacomDeckCardWidget>> BoundCardWidgets;
	FGuid PendingCardPressId;
	FVector2D PendingPressPosition = FVector2D::ZeroVector;
	bool bPendingCardPress = false;
	bool bPendingControlDown = false;
	uint64 CurrentStorageRevision = 0;
	FVector2D DisplayedCarryPointer = FVector2D::ZeroVector;
	bool bHasDisplayedCarryPointer = false;
	bool bCarryInterpolationActive = false;

	void EnsureFallbackTree();
	FReply HandleCardPointerDown(UWacomDeckCardWidget* CardWidget, const FGeometry& Geometry, const FPointerEvent& Event);
	FReply HandleCardPointerMove(UWacomDeckCardWidget* CardWidget, const FGeometry& Geometry, const FPointerEvent& Event);
	FReply HandleCardPointerUp(UWacomDeckCardWidget* CardWidget, const FGeometry& Geometry, const FPointerEvent& Event);
	FVector2D ToLocalPointer(const FPointerEvent& Event) const;
	bool TryBeginCarryFromPendingPress(FVector2D Pointer);
	FReply BuildHandledPointerReply();
	void BroadcastRelease(bool bReleaseAll);
	void StartCarryInterpolation();

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
	bool bInitialReleaseGuardArmed = false;
	bool bMouseCaptured = false;
	bool bDeleteConfirmationPending = false;
};

#endif
