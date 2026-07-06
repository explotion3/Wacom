// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "WacomCardPresentationTypes.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class EWacomCardDetailTokenKind : uint8
{
	Text UMETA(DisplayName = "Text"),
	Icon UMETA(DisplayName = "Icon"),
	Number UMETA(DisplayName = "Number"),
	Keyword UMETA(DisplayName = "Keyword")
};

UENUM(BlueprintType)
enum class EWacomCardDetailIcon : uint8
{
	None UMETA(DisplayName = "None"),
	Damage UMETA(DisplayName = "Damage"),
	Heal UMETA(DisplayName = "Heal"),
	Shield UMETA(DisplayName = "Shield"),
	Poison UMETA(DisplayName = "Poison"),
	Cost UMETA(DisplayName = "Cost"),
	Initiative UMETA(DisplayName = "Initiative"),
	Draw UMETA(DisplayName = "Draw"),
	Discard UMETA(DisplayName = "Discard"),
	Exhaust UMETA(DisplayName = "Exhaust"),
	Keyword UMETA(DisplayName = "Keyword")
};

UENUM(BlueprintType)
enum class EWacomCardDetailTokenLineKind : uint8
{
	Effect UMETA(DisplayName = "Effect"),
	Passive UMETA(DisplayName = "Passive"),
	Change UMETA(DisplayName = "Change"),
	Description UMETA(DisplayName = "Description"),
	Flavor UMETA(DisplayName = "Flavor")
};

UENUM(BlueprintType)
enum class EWacomCardDetailSectionKind : uint8
{
	Description UMETA(DisplayName = "Description"),
	Task UMETA(DisplayName = "Task"),
	Passive UMETA(DisplayName = "Passive"),
	Preview UMETA(DisplayName = "Preview"),
	Flavor UMETA(DisplayName = "Flavor")
};

/**
 * One semantic run inside a card detail line.
 *
 * First-pass fallback renders these tokens as text. Later WBP/RichText work can
 * map Icon/Number tokens to inline images, highlights, and animations by StableId.
 */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomCardDetailToken
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	FName StableId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	EWacomCardDetailTokenKind Kind = EWacomCardDetailTokenKind::Text;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	FText Text;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	EWacomCardDetailIcon Icon = EWacomCardDetailIcon::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	int32 Value = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	bool bHasValue = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	int32 PreviewValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	bool bHasPreviewValue = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	bool bSkipped = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	bool bEmphasized = false;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomCardDetailTokenLine
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	FName LineId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	EWacomCardDetailTokenLineKind Kind = EWacomCardDetailTokenLineKind::Effect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	TArray<FWacomCardDetailToken> Tokens;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomCardDetailSection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	FName SectionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	EWacomCardDetailSectionKind Kind = EWacomCardDetailSectionKind::Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	TArray<FWacomCardDetailTokenLine> TokenLines;
};

/**
 * Hidden/expanded card detail data.
 *
 * Small card faces should keep using FWacomCardViewData. Sections are the
 * canonical document model for hover panels, selected-card inspectors, and
 * other expanded detail surfaces. Legacy flat mirrors are kept only where
 * existing WBP accessors still need them; display widgets should not infer
 * sections from those mirrors.
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
	TArray<FWacomCardDetailSection> Sections;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	TArray<FText> TaskLines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	TArray<FWacomCardDetailTokenLine> TokenLines;
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
 * Optional UI-only runtime facts for a visible card.
 *
 * This is deliberately not a reflected type yet: Battle/App code can pass
 * authoritative snapshot-derived facts into presentation builders without
 * expanding the WBP contract or making widgets read gameplay state.
 */
struct WACOMAPP_API FWacomCardPresentationRuntimeContext
{
	struct FEffectPreview
	{
		int32 EffectIndex = INDEX_NONE;
		bool bSkip = false;
		bool bHasMagnitude = false;
		int32 Magnitude = 0;
	};

	bool bHasRuntimeCost = false;
	int32 RuntimeCost = 0;

	bool bHasPlayableState = false;
	bool bIsPlayable = true;

	TArray<FEffectPreview> EffectPreviews;
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
