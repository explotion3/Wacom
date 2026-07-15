// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "WacomCardPresentationTypes.generated.h"

class UTexture2D;

/**
 * Visual-only perspective state consumed by a reusable card surface.
 *
 * First-person hands provide live tilt and attachment displacement. Other card
 * contexts keep the default neutral view and therefore pay no per-frame cost.
 */
struct WACOMAPP_API FWacomCardSurfacePerspectiveView
{
	bool bEnabled = false;
	FVector2D TiltDegrees = FVector2D::ZeroVector;
	float Strength = 0.0f;
	FVector2D AttachmentOffsetPixels = FVector2D::ZeroVector;
	bool bReducedMotion = false;
};

UENUM(BlueprintType)
enum class EWacomCardDetailRunKind : uint8
{
	Text UMETA(DisplayName = "Text"),
	Value UMETA(DisplayName = "Value"),
	Icon UMETA(DisplayName = "Icon"),
	Status UMETA(DisplayName = "Status"),
	Keyword UMETA(DisplayName = "Keyword"),
	PreviewDelta UMETA(DisplayName = "Preview Delta"),
	Muted UMETA(DisplayName = "Muted")
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
	Slow UMETA(DisplayName = "Slow"),
	Freeze UMETA(DisplayName = "Freeze"),
	Twilight UMETA(DisplayName = "Twilight"),
	Keyword UMETA(DisplayName = "Keyword")
};

UENUM(BlueprintType)
enum class EWacomCardDetailBlockKind : uint8
{
	Paragraph UMETA(DisplayName = "Paragraph"),
	EffectSentence UMETA(DisplayName = "Effect Sentence"),
	PassiveTrigger UMETA(DisplayName = "Passive Trigger"),
	PassiveEffect UMETA(DisplayName = "Passive Effect"),
	Warning UMETA(DisplayName = "Warning"),
	Flavor UMETA(DisplayName = "Flavor"),
	PassiveOutcome UMETA(DisplayName = "Passive Outcome")
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
 * One semantic run inside a card detail block.
 *
 * Runs are UI-only explanation facts. Renderers decide whether they become
 * rich text markup, fallback plain text, inline images, or tooltip anchors.
 */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomCardDetailRun
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	FName StableId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	EWacomCardDetailRunKind Kind = EWacomCardDetailRunKind::Text;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	FText Text;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	FName SlotName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	FGameplayTag ValueSourceTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	FText ValueSourceText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	bool bHasValueSourceText = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	EWacomCardDetailIcon Icon = EWacomCardDetailIcon::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	FGameplayTag Tag;

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
struct WACOMAPP_API FWacomCardDetailBlock
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	FName BlockId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	EWacomCardDetailBlockKind Kind = EWacomCardDetailBlockKind::Paragraph;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	TArray<FWacomCardDetailRun> Runs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	bool bSkipped = false;
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
	TArray<FWacomCardDetailBlock> Blocks;
};

/**
 * Hidden/expanded card detail data.
 *
 * Small card faces should keep using FWacomCardViewData. Sections are the
 * canonical document model for hover panels, selected-card inspectors, and
 * other expanded detail surfaces. Display widgets should render sections
 * directly instead of inferring their own detail document from flat text.
 */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomCardDetailViewData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|CardDetail")
	TArray<FWacomCardDetailSection> Sections;
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
	bool bHasRuntimeCostPreview = false;
	int32 RuntimeCostPreview = 0;

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

	/** UI-only target preview. It never replaces the authoritative Cost field. */
	bool bHasCostPreview = false;
	int32 PreviewCost = 0;

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
