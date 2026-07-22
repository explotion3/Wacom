// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WacomBackpackControlsHelpWidget.generated.h"

class UButton;
class UTextBlock;

/** 被动操作说明层。只显示 Screen 提供的文案并转发关闭意图。 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomBackpackControlsHelpWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	DECLARE_MULTICAST_DELEGATE(FOnCloseRequestedNative);
	FOnCloseRequestedNative OnCloseRequestedNative;

	void SetHelpText(const FText& InText);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HelpText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseHelpButton;

private:
	UFUNCTION()
	void HandleCloseClicked();

	void EnsureFallbackTree();
	FText PendingHelpText;
};
