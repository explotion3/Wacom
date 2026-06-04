// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "WacomCardPresentationTypes.generated.h"

class UTexture2D;

/**
 * Hidden/expanded card detail data.
 *
 * Small card faces should keep using FWacomCardViewData. This structure is for
 * hover panels, selected-card inspectors, and other expanded detail surfaces.
 */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomCardDetailViewData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	TArray<FText> TaskLines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	TArray<FText> ChangeLines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	TArray<FText> PassiveLines;
};

UENUM(BlueprintType)
enum class EWacomCardViewEffectBadgeKind : uint8
{
	Generic UMETA(DisplayName = "Generic"),
	Damage UMETA(DisplayName = "Damage"),
	Heal UMETA(DisplayName = "Heal"),
	Poison UMETA(DisplayName = "Poison"),
	Burn UMETA(DisplayName = "Burn"),
	Slow UMETA(DisplayName = "Slow"),
	Freeze UMETA(DisplayName = "Freeze"),
	Twilight UMETA(DisplayName = "Twilight"),
	Draw UMETA(DisplayName = "Draw"),
	Discard UMETA(DisplayName = "Discard"),
	Initiative UMETA(DisplayName = "Initiative"),
	Cost UMETA(DisplayName = "Cost"),
	Shield UMETA(DisplayName = "Shield")
};

/**
 * One compact numeric badge shown around the card body.
 *
 * The art-facing WBP can replace DisplayText with an icon+number treatment by
 * inspecting Kind. The C++ fallback renders DisplayText directly.
 */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomCardViewEffectBadge
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardView")
	EWacomCardViewEffectBadgeKind Kind = EWacomCardViewEffectBadgeKind::Generic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardView")
	int32 Value = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardView")
	FText DisplayText;
};

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

	/** Detailed rule text. Default small card faces should usually hide this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardView", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardView")
	int32 Cost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardView")
	bool bShowCost = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardView")
	FGameplayTag Rarity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardView")
	int32 Value = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardView")
	bool bShowValue = false;

	/** Compact body/capacity line, e.g. "1耐久/3容量" or "+6生命". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardView")
	FText PhysiqueText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardView")
	bool bShowPhysique = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardView")
	TArray<FWacomCardViewEffectBadge> EffectBadges;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardView")
	bool bDisabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardView")
	int32 Durability = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardView")
	bool bShowDurability = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardView")
	TObjectPtr<UTexture2D> Art = nullptr;
};
