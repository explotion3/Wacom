// Copyright Wacom. All Rights Reserved.

#include "Commandlets/WacomCreateInputAssetsCommandlet.h"
#include "ContentBuilders/ContentBuilderHelpers.h"

#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "EnhancedActionKeyMapping.h"
#include "InputCoreTypes.h"

using namespace Wacom::ContentBuilder;

namespace
{
	struct FInputActionDef
	{
		FName AssetName;
		FKey DefaultKey;
	};

	UInputAction* CreateIA(const FString& BasePath, FName AssetName)
	{
		const FString PkgPath = BasePath / AssetName.ToString();
		UPackage* Pkg = FindOrCreatePackage(PkgPath);
		if (!Pkg) { return nullptr; }

		UInputAction* IA = CreateOrReplaceAsset<UInputAction>(Pkg, AssetName);
		if (!IA) { return nullptr; }

		// Digital (bool) trigger — default for button presses.
		IA->ValueType = EInputActionValueType::Boolean;

		SaveAssetPackage(Pkg, IA, PkgPath);
		return IA;
	}
}

UWacomCreateInputAssetsCommandlet::UWacomCreateInputAssetsCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UWacomCreateInputAssetsCommandlet::Main(const FString& /*Params*/)
{
	UE_LOG(LogTemp, Display, TEXT("[WacomCreateInputAssets] Start"));

	const FString BasePath = TEXT("/Game/Wacom/Input");

	// ---- Define all InputActions ----
	const TArray<FInputActionDef> ActionDefs = {
		{ TEXT("IA_PlayCard1"),  EKeys::One   },
		{ TEXT("IA_PlayCard2"),  EKeys::Two   },
		{ TEXT("IA_PlayCard3"),  EKeys::Three },
		{ TEXT("IA_PlayCard4"),  EKeys::Four  },
		{ TEXT("IA_PlayCard5"),  EKeys::Five  },
		{ TEXT("IA_PlayCard6"),  EKeys::Six   },
		{ TEXT("IA_PlayCard7"),  EKeys::Seven },
		{ TEXT("IA_Wait"),       EKeys::W     },
		{ TEXT("IA_EndTurn"),    EKeys::E     },
		{ TEXT("IA_Restart"),    EKeys::R     },
		{ TEXT("IA_RefreshHUD"), EKeys::P     },
	};

	// ---- Create InputActions ----
	TArray<TPair<UInputAction*, FKey>> CreatedActions;
	CreatedActions.Reserve(ActionDefs.Num());

	for (const FInputActionDef& Def : ActionDefs)
	{
		UInputAction* IA = CreateIA(BasePath, Def.AssetName);
		if (!IA)
		{
			UE_LOG(LogTemp, Error, TEXT("[WacomCreateInputAssets] Failed to create %s"), *Def.AssetName.ToString());
			return 1;
		}
		CreatedActions.Add({ IA, Def.DefaultKey });
		UE_LOG(LogTemp, Display, TEXT("[WacomCreateInputAssets] Created %s"), *Def.AssetName.ToString());
	}

	// ---- Create InputMappingContext ----
	const FString IMCPkgPath = BasePath / TEXT("IMC_Battle");
	UPackage* IMCPkg = FindOrCreatePackage(IMCPkgPath);
	if (!IMCPkg)
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomCreateInputAssets] Failed to create IMC package"));
		return 2;
	}

	UInputMappingContext* IMC = CreateOrReplaceAsset<UInputMappingContext>(IMCPkg, TEXT("IMC_Battle"));
	if (!IMC)
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomCreateInputAssets] Failed to create IMC_Battle"));
		return 3;
	}

	// Add mappings: each IA → its default key.
	for (const auto& Pair : CreatedActions)
	{
		FEnhancedActionKeyMapping& Mapping = IMC->MapKey(Pair.Key, Pair.Value);
		(void)Mapping;  // No modifiers/triggers needed for simple digital press.
	}

	SaveAssetPackage(IMCPkg, IMC, IMCPkgPath);
	UE_LOG(LogTemp, Display, TEXT("[WacomCreateInputAssets] Created IMC_Battle with %d mappings"), CreatedActions.Num());

	UE_LOG(LogTemp, Display, TEXT("[WacomCreateInputAssets] Done"));
	return 0;
}
