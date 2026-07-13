// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "UI/Foundation/WacomMenuWidgetBase.h"
#include "WacomSettingsConfirmationDialog.generated.h"

class UCommonTextBlock;
class UWacomMenuButtonWidget;

enum class EWacomSettingsConfirmationMode : uint8
{
	DiscardChanges,
	VideoMode
};

enum class EWacomSettingsConfirmationDecision : uint8
{
	Confirm,
	Cancel,
	TimedOut
};

DECLARE_DELEGATE_OneParam(
	FWacomSettingsConfirmationDecisionDelegate,
	EWacomSettingsConfirmationDecision);

/** Passive settings modal. The owning Settings Screen interprets every decision. */
UCLASS(Blueprintable, meta = (ToolTip = "设置页面专用确认 Modal。负责放弃修改或视频模式倒计时显示，只上报决定，不直接调用设置服务。"))
class WACOMAPP_API UWacomSettingsConfirmationDialog : public UWacomMenuWidgetBase
{
	GENERATED_BODY()

public:
	void Configure(
		EWacomSettingsConfirmationMode InMode,
		float InRemainingSeconds,
		FWacomSettingsConfirmationDecisionDelegate InOnDecision);

	/** Programmatic teardown that must not synthesize a user decision. */
	void CloseWithoutDecision();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> MessageText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomMenuButtonWidget> ConfirmButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomMenuButtonWidget> CancelButton;

private:
	void HandleConfirmClicked();
	void HandleCancelClicked();
	void RefreshContent();
	void StartCountdown();
	void StopCountdown();
	bool TickCountdown(float DeltaTime);
	void ResolveDecision(EWacomSettingsConfirmationDecision Decision);

	EWacomSettingsConfirmationMode Mode = EWacomSettingsConfirmationMode::DiscardChanges;
	FWacomSettingsConfirmationDecisionDelegate OnDecision;
	FTSTicker::FDelegateHandle CountdownTickerHandle;
	double CountdownDeadlineSeconds = 0.0;
	float InitialRemainingSeconds = 0.0f;
	bool bClosingWithoutDecision = false;
};
