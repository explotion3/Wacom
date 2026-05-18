// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WacomCardView.generated.h"

class UBorder;
class UCardDefinition;
class UImage;
class UTextBlock;
class UTexture2D;

/**
 * Lightweight data used by reusable card display widgets.
 *
 * This is intentionally UI-only: it is a view model for one visible card, not
 * the authoritative card runtime state. Combat, backpack, shop, reward, and
 * drag-preview widgets can all build this data from their own source.
 */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomCardViewData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardView")
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardView")
	FText TypeText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardView", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardView")
	int32 Cost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardView")
	bool bShowCost = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardView")
	bool bDisabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardView")
	TObjectPtr<UTexture2D> Art = nullptr;
};

/**
 * Reusable visual-only card widget.
 *
 * Responsibilities:
 * - Display card view data.
 * - Provide a C++ fallback layout for early development and tests.
 * - Serve as the parent class for WBP_CardView.
 *
 * Non-responsibilities:
 * - No battle command submission.
 * - No backpack MoveInstance/DeleteCardForGold calls.
 * - No drag/drop source or target behavior.
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomCardView : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|CardView")
	void SetCardViewData(const FWacomCardViewData& InData);

	UFUNCTION(BlueprintPure, Category = "Wacom|CardView")
	const FWacomCardViewData& GetCardViewData() const { return CurrentData; }

	UFUNCTION(BlueprintCallable, Category = "Wacom|CardView")
	static FWacomCardViewData BuildFromCardDefinition(const UCardDefinition* Card);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CostText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TypeText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DescriptionText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CardArt;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> DisabledOverlay;

private:
	UPROPERTY(Transient)
	FWacomCardViewData CurrentData;

	void ApplyCurrentDataToWidgets();
};
