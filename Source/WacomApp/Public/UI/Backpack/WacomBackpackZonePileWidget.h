// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Backpack/WacomBackpackZonePileTypes.h"
#include "WacomBackpackZonePileWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class UWacomBackpackWorkspaceStyle;

/** 工作台内的被动区域牌堆；只显示 ViewData 并把标题指针事件转交给 Workspace。 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomBackpackZonePileWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	DECLARE_DELEGATE_RetVal_ThreeParams(
		FReply,
		FOnPilePointerDownNative,
		UWacomBackpackZonePileWidget*,
		const FGeometry&,
		const FPointerEvent&);
	FOnPilePointerDownNative OnPilePointerDownNative;

	void SetPileView(const FWacomBackpackZonePileView& InView);
	const FWacomBackpackZonePileView& GetPileView() const { return PileView; }
	void SetVisualStyle(UWacomBackpackWorkspaceStyle* InStyle);
	void SetDropFeedbackView(const FWacomBackpackDropFeedbackView& InView);
	const FWacomBackpackDropFeedbackView& GetDropFeedbackView() const { return DropFeedbackView; }
	void SetResolvedGeometry(const FSlateRect& InFrameRect, const FSlateRect& InHeaderRect);
	FSlateRect GetResolvedFrameRect() const { return ResolvedFrameRect; }
	FSlateRect GetResolvedHeaderRect() const { return ResolvedHeaderRect; }
	bool WasLastPointerDownOnDragHandle() const { return bLastPointerDownOnDragHandle; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnMouseButtonDown(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> FrameBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> DragHandle;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> AccentStrip;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ZoneIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CountText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> CountBadge;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> DropFeedback;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DropFeedbackText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DropFeedbackCountText;

private:
	FWacomBackpackZonePileView PileView;
	FSlateRect ResolvedFrameRect;
	FSlateRect ResolvedHeaderRect;
	TWeakObjectPtr<UWacomBackpackWorkspaceStyle> VisualStyle;
	FWacomBackpackDropFeedbackView DropFeedbackView;
	bool bLastPointerDownOnDragHandle = false;

	void EnsureFallbackTree();
	void ApplyView();
	void ApplyDropFeedback();
};
