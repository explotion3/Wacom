// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "BattlePresentationStackEntryWidget.generated.h"

class UPanelWidget;
class USizeBox;
class UWacomCardView;

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattlePresentationStackEntryView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation Stack")
	int32 EntryId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation Stack")
	FGuid CardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation Stack")
	bool bIsExiting = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Presentation Stack")
	FWacomCardViewData CardViewData;
};

/**
 * Read-only mini-card entry for the Battle presentation stack.
 *
 * It renders card view data only; it does not handle click, drag, or command submission.
 */
UCLASS(Blueprintable)
class WACOMAPP_API UBattlePresentationStackEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Presentation Stack")
	void SetPresentationStackEntryData(const FWacomBattlePresentationStackEntryView& InEntry);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Presentation Stack")
	const FWacomBattlePresentationStackEntryView& GetCurrentEntry() const { return CurrentEntry; }

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Presentation Stack")
	void SetMiniCardViewClass(TSubclassOf<UWacomCardView> InClass);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Presentation Stack")
	UWacomCardView* GetMiniCardView() const { return MiniCardView; }

#if WITH_AUTOMATION_TESTS
	bool HasHeaderOrTargetTextWidgetsForTest() const { return false; }
	bool HasMiniCardScaleHostForTest() const { return MiniCardScaleHost != nullptr; }
	void TickExitForTest(float DeltaSeconds);
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Battle|Presentation Stack", DisplayName = "On Entry Updated")
	void BP_OnEntryUpdated(const FWacomBattlePresentationStackEntryView& Entry);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> CardHost;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> MiniCardScaleHost;

private:
	UPROPERTY(Transient)
	FWacomBattlePresentationStackEntryView CurrentEntry;

	UPROPERTY(Transient)
	TObjectPtr<UWacomCardView> MiniCardView;

	UPROPERTY(Transient)
	TSubclassOf<UWacomCardView> RuntimeMiniCardViewClass;

	float ExitElapsedSeconds = 0.0f;
	bool bPreviousExitingState = false;

	void EnsureMiniCardView();
	void ApplyCurrentEntry();
	void ApplyExitVisual(float NormalizedAlpha);
};
