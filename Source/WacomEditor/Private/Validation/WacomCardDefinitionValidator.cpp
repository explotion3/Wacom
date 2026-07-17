// Copyright Wacom. All Rights Reserved.

#include "Validation/WacomCardDefinitionValidator.h"

#include "Cards/CardDefinition.h"
#include "Engine/Texture2D.h"
#include "Validation/CardDefinitionValidation.h"

bool UWacomCardDefinitionValidator::CanValidateAsset_Implementation(
	const FAssetData& /*InAssetData*/,
	UObject* InAsset,
	FDataValidationContext& /*InContext*/) const
{
	return InAsset && InAsset->IsA<UCardDefinition>();
}

EDataValidationResult UWacomCardDefinitionValidator::ValidateLoadedAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InAsset,
	FDataValidationContext& /*InContext*/)
{
	const UCardDefinition* CardDefinition = Cast<UCardDefinition>(InAsset);
	if (!CardDefinition)
	{
		return EDataValidationResult::NotValidated;
	}

	if (const UTexture2D* DepthMap = CardDefinition->CardIllustrationDepthMap)
	{
		const bool bUsesRecommendedDepthMapSettings =
			DepthMap->CompressionSettings == TC_Masks
			&& !DepthMap->SRGB
			&& DepthMap->Filter == TF_Nearest
			&& DepthMap->MipGenSettings == TMGS_NoMipmaps
			&& DepthMap->LODGroup == TEXTUREGROUP_UI;
		if (!bUsesRecommendedDepthMapSettings)
		{
			AssetMessage(
				InAssetData,
				EMessageSeverity::Warning,
				FText::FromString(TEXT(
					"CardIllustrationDepthMap 推荐设置为 Compression=Masks、sRGB=false、Filter=Nearest、MipGen=NoMipmaps、LODGroup=UI；当前设置仍可运行，但可能产生灰度偏差或像素边缘游动。")));
		}
	}

	TArray<FText> Errors;
	if (FWacomCardDefinitionValidation::Validate(CardDefinition, Errors))
	{
		AssetPasses(InAsset);
		return EDataValidationResult::Valid;
	}

	for (const FText& Error : Errors)
	{
		AssetMessage(InAssetData, EMessageSeverity::Error, Error);
	}
	return EDataValidationResult::Invalid;
}
