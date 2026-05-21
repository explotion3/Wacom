// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RunState.h"
#include "WacomRunEventChoiceButton.generated.h"

class UButton;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnRunEventChoiceClickedNative, FName /*ChoiceId*/);

/** 最小事件选项按钮。WBP 可继承替换视觉，逻辑仍只广播 ChoiceId。 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomRunEventChoiceButton : public UUserWidget
{
	GENERATED_BODY()

public:
	FOnRunEventChoiceClickedNative OnChoiceClickedNative;

	UFUNCTION(BlueprintCallable, Category = "Wacom|RunEvent")
	void SetChoiceSnapshot(const FRunEventChoiceSnapshot& InChoice);

	UFUNCTION(BlueprintPure, Category = "Wacom|RunEvent")
	FRunEventChoiceSnapshot GetChoiceSnapshot() const { return ChoiceSnapshot; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ChoiceButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LabelText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DisabledReasonText;

private:
	void RefreshVisuals();

	UPROPERTY(Transient)
	FRunEventChoiceSnapshot ChoiceSnapshot;
};
