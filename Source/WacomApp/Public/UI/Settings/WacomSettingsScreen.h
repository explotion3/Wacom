// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Settings/WacomLocalSettingsTypes.h"
#include "UI/Foundation/WacomMenuWidgetBase.h"
#include "WacomSettingsScreen.generated.h"

class UCommonTextBlock;
class UVerticalBox;
class UWacomMenuButtonWidget;
class UWacomSettingsConfirmationDialog;
class UWacomSettingsOptionRow;
class UWacomSettingsSubsystem;
enum class EWacomSettingsConfirmationDecision : uint8;
enum class EWacomSettingsConfirmationMode : uint8;

enum class EWacomSettingsCategory : uint8
{
	Display,
	Graphics,
	Audio,
	View,
	Accessibility
};

enum class EWacomSettingsField : uint8
{
	ScreenResolution,
	WindowMode,
	VSync,
	FrameRateLimit,
	GraphicsQuality,
	MasterVolume,
	MusicVolume,
	SFXVolume,
	UISoundVolume,
	LookResponseStrength,
	InvertLookY,
	CameraMotionStrength,
	FlashEffectMode,
	UIMotionMode
};

#if WITH_AUTOMATION_TESTS
struct FWacomSettingsScreenAutomationTestView
{
	bool bHasValidEditSession = false;
	bool bDirty = false;
	bool bAwaitingVideoConfirmation = false;
	bool bRestoreDefaultsEnabled = false;
	EWacomSettingsCategory SelectedCategory = EWacomSettingsCategory::Display;
	int32 VisibleOptionCount = 0;
	FGuid Token;
	FWacomLocalSettingsSnapshot Baseline;
	FWacomLocalSettingsSnapshot Draft;
	FWacomLocalSettingsSnapshot DefaultSnapshot;
};
#endif

/** CommonUI coordinator for one tokenized local-settings edit session. */
UCLASS(Blueprintable, meta = (ToolTip = "本地设置 CommonUI Screen。持有一次 token 化编辑事务与本地 draft，统一协调 Preview、Apply、Cancel 和视频确认；不访问 SaveGame。"))
class WACOMAPP_API UWacomSettingsScreen : public UWacomMenuWidgetBase
{
	GENERATED_BODY()

public:
	UWacomSettingsScreen(const FObjectInitializer& ObjectInitializer);

#if WITH_AUTOMATION_TESTS
	FWacomSettingsScreenAutomationTestView GetAutomationTestViewForTest() const;
	friend struct FWacomSettingsScreenTestAccess;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual FReply NativeHandleBackRequested() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> CategoryContainer;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> OptionsContainer;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> CategoryTitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> StatusText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomMenuButtonWidget> RestoreDefaultsButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomMenuButtonWidget> ApplyButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomMenuButtonWidget> BackButton;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Settings|Widget Classes", meta = (ToolTip = "设置分类与底部操作使用的按钮类；为空时使用 UWacomMenuButtonWidget C++ fallback。"))
	TSubclassOf<UWacomMenuButtonWidget> SettingsButtonClass;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Settings|Widget Classes", meta = (ToolTip = "设置选项行类；为空时使用 UWacomSettingsOptionRow C++ fallback。"))
	TSubclassOf<UWacomSettingsOptionRow> OptionRowClass;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Settings|Widget Classes", meta = (ToolTip = "放弃修改与视频模式倒计时使用的 Modal 类；为空时使用 UWacomSettingsConfirmationDialog C++ fallback。"))
	TSubclassOf<UWacomSettingsConfirmationDialog> ConfirmationDialogClass;

private:
	void BuildCategoryButtons();
	void RebuildOptionRows();
	void RefreshFromDraft();
	void RefreshOptionRows();
	void RefreshInteractionState();
	void SelectCategory(EWacomSettingsCategory Category, bool bMoveFocusToFirstRow = false);
	void HandleOptionStep(EWacomSettingsField Field, int32 Direction);
	void HandleOptionNormalizedValue(EWacomSettingsField Field, float NormalizedValue);
	void CommitDraftMutation(
		EWacomSettingsField Field,
		const FWacomLocalSettingsSnapshot& PreviousDraft);
	void HandleApplyClicked();
	void HandleRestoreDefaultsClicked();
	void HandleBackClicked();
	void RequestClose();
	bool BeginEditSession();
	void RestartEditSession(const FText& SuccessMessage);
	void EndOwnedSessionForTeardown();
	void BindRuntimeSettings();
	void UnbindRuntimeSettings();
	void HandleRuntimeSettingsChanged(
		const FWacomLocalSettingsSnapshot& Snapshot,
		EWacomRuntimeSettingsChangeReason Reason);
	bool ShowConfirmationDialog(EWacomSettingsConfirmationMode Mode);
	void HandleConfirmationDecision(EWacomSettingsConfirmationDecision Decision);
	void CompleteVideoConfirmation(EWacomRuntimeSettingsChangeReason Reason);
	void CloseActiveDialogWithoutDecision();
	void SetStatus(const FText& Message);
	void HandleFatalSessionFailure(const FText& Message);
	bool IsDirty() const;

	UPROPERTY(Transient)
	TObjectPtr<UWacomSettingsSubsystem> SettingsSubsystem;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWacomMenuButtonWidget>> CategoryButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWacomSettingsOptionRow>> ActiveOptionRows;

	TMap<EWacomSettingsField, TWeakObjectPtr<UWacomSettingsOptionRow>> OptionRowsByField;
	TArray<FDelegateHandle> CategoryButtonDelegateHandles;
	TArray<FIntPoint> SupportedResolutions;
	FGuid EditToken;
	FWacomLocalSettingsSnapshot Baseline;
	FWacomLocalSettingsSnapshot Draft;
	FWacomLocalSettingsSnapshot DefaultSnapshot;
	EWacomSettingsCategory SelectedCategory = EWacomSettingsCategory::Display;
	EWacomSettingsConfirmationMode ActiveConfirmationMode;
	TWeakObjectPtr<UWacomSettingsConfirmationDialog> ActiveConfirmationDialog;
	FDelegateHandle RuntimeSettingsChangedHandle;
	FText StatusMessage;
	bool bAwaitingVideoConfirmation = false;
	bool bTearingDown = false;
	bool bFatalSessionFailureScheduled = false;
};
