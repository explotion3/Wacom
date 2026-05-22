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
	/** 从当前 RunSession 拉取事件快照并重建选项。 */
	UFUNCTION(BlueprintCallable, Category = "Wacom|RunEvent")
	void RefreshEvent();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

	virtual URunSession* ResolveRunSession() const;
	virtual UWacomAppToastSubsystem* ResolveToastSubsystem() const;

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
#if WITH_AUTOMATION_TESTS
	friend class UWacomRunEventScreenProbe;

	int32 GetChoiceCount() const { return CachedChoices.Num(); }
	FRunEventChoiceSnapshot GetCachedChoiceSnapshot(int32 Index) const;
	bool ChooseChoiceByIndex(int32 Index);
	FText GetDisplayedTitleText() const;
	FText GetDisplayedBodyText() const;
#endif

	void RebuildChoices();
	void AddChoiceButton(const FRunEventChoiceSnapshot& Choice);
	void HandleChoiceClicked(FName ChoiceId);
	bool ChooseChoice(FName ChoiceId);

	UPROPERTY(Transient)
	TArray<FRunEventChoiceSnapshot> CachedChoices;

	bool bDidEndRunEvent = false;
};
