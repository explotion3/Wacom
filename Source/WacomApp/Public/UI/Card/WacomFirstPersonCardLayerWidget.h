// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "WacomFirstPersonCardLayerWidget.generated.h"

class UCanvasPanel;
class UWacomCardView;

/**
 * Non-interactive HUD card layer driven by first-person card anchor projection.
 * V0-B renders static UWacomCardView instances only; it does not own commands.
 */
UCLASS()
class WACOMAPP_API UWacomFirstPersonCardLayerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetCardViewClass(TSubclassOf<UWacomCardView> InCardViewClass);
	void SetStaticCardSlots(const TArray<FWacomFirstPersonStaticCardSlotView>& InSlots);

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	int32 GetCardViewCount() const { return CardViews.Num(); }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	UWacomCardView* GetCardViewAt(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	bool IsCardSlotVisible(int32 Index) const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWacomCardView>> CardViews;

	UPROPERTY(Transient)
	TArray<FWacomFirstPersonStaticCardSlotView> LastSlots;

	UPROPERTY(Transient)
	TSubclassOf<UWacomCardView> CardViewClass;

	void EnsureCardViewCount(int32 DesiredCount);
	UWacomCardView* CreateCardView(int32 Index);
};
