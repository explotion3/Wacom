// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WacomBackpackDeleteConfirmWidget.generated.h"

class UButton;
class UTextBlock;

/** 被动批量销毁确认框；只显示预览并转发确认/取消意图。 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomBackpackDeleteConfirmWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	DECLARE_MULTICAST_DELEGATE(FOnConfirmNative);
	DECLARE_MULTICAST_DELEGATE(FOnCancelNative);
	FOnConfirmNative OnConfirmNative;
	FOnCancelNative OnCancelNative;

	void SetPreview(int32 CardCount, int32 TotalGoldReward);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> SummaryText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> ConfirmButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> CancelButton;

private:
	int32 PreviewCardCount = 0;
	int32 PreviewGoldReward = 0;
	void ApplyPreview();
	UFUNCTION() void HandleConfirm();
	UFUNCTION() void HandleCancel();
};
