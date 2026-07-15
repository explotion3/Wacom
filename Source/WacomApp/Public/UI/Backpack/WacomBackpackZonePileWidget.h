// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Backpack/WacomBackpackPilePreviewWidget.h"
#include "WacomBackpackZonePileWidget.generated.h"

class UBorder;
class UOverlay;
class UTextBlock;
class UWacomBackpackPilePreviewWidget;

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
	void SetDropPreviewState(bool bVisible, bool bRejected);
	void SetPreviewWidgetClass(TSubclassOf<UWacomBackpackPilePreviewWidget> InClass);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnMouseButtonDown(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> DragHandle;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CountText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UOverlay> PreviewHost;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> DropPreviewBorder;

private:
	FWacomBackpackZonePileView PileView;
	TSubclassOf<UWacomBackpackPilePreviewWidget> PreviewWidgetClass;
	bool bDropPreviewVisible = false;
	bool bDropPreviewRejected = false;

	void EnsureFallbackTree();
	void ApplyView();
	void RebuildPreviews();
};

