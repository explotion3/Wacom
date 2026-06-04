// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RunStateTypes.h"
#include "WacomSpecialZoneWidget.generated.h"

class UPanelWidget;
class UTextBlock;
class UDragDropOperation;
class UWrapBox;
class UWacomBackpackScreen;
class UWacomDeckCardWidget;
class UWacomZoneDropTarget;

/**
 * 单个 B 类特殊存放区 UI。
 *
 * 数据源：父 BackpackScreen 传入的 FRunSpecialStorageView，只读。
 * 命令出口：DropTarget 仍通过父 BackpackScreen 访问 RunSession；右键入战 toggle 以 delegate 回传。
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomSpecialZoneWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnBattleEnabledToggleRequestedNative, FGuid);
	FOnBattleEnabledToggleRequestedNative OnBattleEnabledToggleRequestedNative;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnCardHoverStateChangedNative, UWacomDeckCardWidget*);
	FOnCardHoverStateChangedNative OnCardHoveredNative;
	FOnCardHoverStateChangedNative OnCardUnhoveredNative;

	void SetSpecialZoneView(
		const FRunSpecialStorageView& InView,
		UWacomBackpackScreen* InOwnerScreen,
		TSubclassOf<UWacomDeckCardWidget> InCardWidgetClass);

	/** 测试/诊断用：当前标题文本。 */
	FText GetZoneTitleText() const;

	/** 测试/诊断用：主卡已入战标记当前是否可见。 */
	bool IsBattleReadyBadgeVisible() const;

	/** 测试/诊断用：构造主卡拖拽 payload。 */
	UDragDropOperation* BuildOwnerCardDragOperation() const;

	/** 测试/诊断用：构造内容卡拖拽 payload。 */
	UDragDropOperation* BuildContentCardDragOperation(int32 Index) const;

	/** 测试/诊断用：请求内容卡右键入战 toggle。 */
	bool RequestContentCardBattleEnabledToggle(int32 Index) const;

	/** 当前 SpecialZone owner instance id。用于父列表做增量复用，不暴露给蓝图。 */
	FGuid GetOwnerCardInstanceId() const;

	/** 当前区块是否持有该卡 widget。用于父 BackpackScreen 判断详情 source 是否已被移除。 */
	bool ContainsCardWidget(const UWacomDeckCardWidget* Widget) const;

#if WITH_AUTOMATION_TESTS
	UWacomDeckCardWidget* GetOwnerCardWidgetForTest() const { return OwnerCardWidget; }
	UWacomDeckCardWidget* GetContentCardWidgetForTest(int32 Index) const;
	int32 GetContentCardWidgetCountForTest() const { return ContentCardWidgets.Num(); }
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BattleReadyBadge;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> OwnerCardHost;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> ContentDropTargetHost;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> ContentCardsBox;

private:
	FRunSpecialStorageView CurrentView;

	UPROPERTY(Transient)
	TWeakObjectPtr<UWacomBackpackScreen> OwnerScreen;

	UPROPERTY(Transient)
	TObjectPtr<UWacomZoneDropTarget> ContentDropTarget;

	UPROPERTY(Transient)
	TObjectPtr<UWacomDeckCardWidget> OwnerCardWidget;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWacomDeckCardWidget>> ContentCardWidgets;

	UPROPERTY(Transient)
	TSubclassOf<UWacomDeckCardWidget> CardWidgetClass;

	void EnsureRuntimeWidgets();
	void RebuildFromCurrentView();
	UWacomDeckCardWidget* CreateCardWidget(const FRunStorageCardView& CardView);
	void HandleBattleEnabledToggleRequested(FGuid InstanceId);
	void HandleCardHovered(UWacomDeckCardWidget* SourceWidget);
	void HandleCardUnhovered(UWacomDeckCardWidget* SourceWidget);
};
