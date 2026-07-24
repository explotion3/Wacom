// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Cards/WacomCardFaceTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "WacomCardPresentationBuilder.generated.h"

class UCardDefinition;

/**
 * Public facade for UI-only presentation data from card definitions.
 *
 * Widgets should render the resulting data and avoid parsing UCardDefinition
 * directly. App-private builders own the card face and detail document assembly.
 */
UCLASS()
class WACOMAPP_API UWacomCardPresentationBuilder : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Wacom|CardView")
	static FWacomCardViewData BuildCardViewData(const UCardDefinition* Card);

	/** 为指定 Battle / Run 表面生成静态卡面；旧入口固定等价于 Battle。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|CardView")
	static FWacomCardViewData BuildCardViewDataForFace(
		const UCardDefinition* Card,
		EWacomCardFaceContext FaceContext);

	static FWacomCardViewData BuildCardViewData(
		const UCardDefinition* Card,
		const FWacomCardPresentationRuntimeContext& RuntimeContext);

	static FWacomCardViewData BuildCardViewData(
		const UCardDefinition* Card,
		EWacomCardFaceContext FaceContext,
		const FWacomCardPresentationRuntimeContext& RuntimeContext);

	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	static FWacomCardDetailViewData BuildCardDetailViewData(const UCardDefinition* Card);

	/** 为指定 Battle / Run 表面生成详情文档；旧入口固定等价于 Battle。 */
	UFUNCTION(BlueprintPure, Category = "Wacom|CardDetail")
	static FWacomCardDetailViewData BuildCardDetailViewDataForFace(
		const UCardDefinition* Card,
		EWacomCardFaceContext FaceContext);

	static FWacomCardDetailViewData BuildCardDetailViewData(
		const UCardDefinition* Card,
		const FWacomCardPresentationRuntimeContext& RuntimeContext);

	static FWacomCardDetailViewData BuildCardDetailViewData(
		const UCardDefinition* Card,
		EWacomCardFaceContext FaceContext,
		const FWacomCardPresentationRuntimeContext& RuntimeContext);

	UFUNCTION(BlueprintPure, Category = "Wacom|CardView")
	static TArray<FWacomCardViewEffectBadge> BuildEffectBadges(const UCardDefinition* Card);

	static TArray<FWacomCardViewEffectBadge> BuildEffectBadges(
		const UCardDefinition* Card,
		const FWacomCardPresentationRuntimeContext& RuntimeContext);

	static TArray<FWacomCardViewEffectBadge> BuildEffectBadges(
		const UCardDefinition* Card,
		EWacomCardFaceContext FaceContext,
		const FWacomCardPresentationRuntimeContext& RuntimeContext);
};
