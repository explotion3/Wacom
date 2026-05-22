// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/WacomMenuWidgetBase.h"
#include "RunState.h"
#include "WacomRunEventScreen.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class UWacomAppToastSubsystem;
class URunSession;

/** 最小可用探索事件界面。 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomRunEventScreen : public UWacomMenuWidgetBase
{
	GENERATED_BODY()

public:
	void SetRunSessionOverrideForTest(URunSession* InRunSession) { RunSessionOverride = InRunSession; }
	void SetToastSubsystemOverrideForTest(UWacomAppToastSubsystem* InToastSubsystem) { ToastSubsystemOverride = InToastSubsystem; }

	/** 从当前 RunSession 拉取事件快照并重建选项。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|RunEvent")
	void RefreshEvent();

	UFUNCTION(BlueprintPure, Category = "Wacom|RunEvent")
	int32 GetChoiceCountForTest() const { return CachedChoices.Num(); }

	UFUNCTION(BlueprintPure, Category = "Wacom|RunEvent")
	FRunEventChoiceSnapshot GetChoiceSnapshotForTest(int32 Index) const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|RunEvent")
	bool ChooseChoiceByIndexForTest(int32 Index);

	UFUNCTION(BlueprintPure, Category = "Wacom|RunEvent")
	FText GetTitleTextForTest() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|RunEvent")
	FText GetBodyTextForTest() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

	UFUNCTION()
	void HandleCloseClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BodyText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EmptyText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> ChoiceList;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

private:
	URunSession* GetRunSession() const;
	UWacomAppToastSubsystem* GetToastSubsystem() const;
	void RebuildChoices();
	void AddChoiceButton(const FRunEventChoiceSnapshot& Choice);
	void HandleChoiceClicked(FName ChoiceId);
	bool ChooseChoice(FName ChoiceId);

	UPROPERTY(Transient)
	TArray<FRunEventChoiceSnapshot> CachedChoices;

	UPROPERTY(Transient)
	TObjectPtr<URunSession> RunSessionOverride = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UWacomAppToastSubsystem> ToastSubsystemOverride = nullptr;

	bool bDidEndRunEvent = false;
};
