// Copyright Wacom. All Rights Reserved.

#include "Commandlets/WacomCreateInputAssetsCommandlet.h"
#include "ContentBuilders/ContentBuilderHelpers.h"

#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "EnhancedActionKeyMapping.h"
#include "InputCoreTypes.h"
#include "InputModifiers.h"

using namespace Wacom::ContentBuilder;

namespace
{
	struct FInputActionDef
	{
		FName AssetName;
		FKey  DefaultKey;
	};

	/** 生成一个 Boolean 类型的 IA 资产。 */
	UInputAction* CreateBoolIA(const FString& BasePath, FName AssetName)
	{
		const FString PkgPath = BasePath / AssetName.ToString();
		UPackage* Pkg = FindOrCreatePackage(PkgPath);
		if (!Pkg) { return nullptr; }

		UInputAction* IA = CreateOrReplaceAsset<UInputAction>(Pkg, AssetName);
		if (!IA) { return nullptr; }

		IA->ValueType = EInputActionValueType::Boolean;

		SaveAssetPackage(Pkg, IA, PkgPath);
		return IA;
	}

	/** 生成一个 Axis2D 类型的 IA 资产（用于 WASD 移动 / 鼠标视角）。 */
	UInputAction* CreateAxis2DIA(const FString& BasePath, FName AssetName)
	{
		const FString PkgPath = BasePath / AssetName.ToString();
		UPackage* Pkg = FindOrCreatePackage(PkgPath);
		if (!Pkg) { return nullptr; }

		UInputAction* IA = CreateOrReplaceAsset<UInputAction>(Pkg, AssetName);
		if (!IA) { return nullptr; }

		IA->ValueType = EInputActionValueType::Axis2D;

		SaveAssetPackage(Pkg, IA, PkgPath);
		return IA;
	}

	UInputModifierNegate* MakeNegate(UObject* Outer, bool bX, bool bY, bool bZ)
	{
		UInputModifierNegate* Mod = NewObject<UInputModifierNegate>(Outer);
		Mod->bX = bX; Mod->bY = bY; Mod->bZ = bZ;
		return Mod;
	}

	UInputModifierSwizzleAxis* MakeSwizzleYXZ(UObject* Outer)
	{
		UInputModifierSwizzleAxis* Mod = NewObject<UInputModifierSwizzleAxis>(Outer);
		Mod->Order = EInputAxisSwizzle::YXZ;
		return Mod;
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

	// ================ Battle IMC ================

	const TArray<FInputActionDef> BattleActionDefs = {
		{ TEXT("IA_PlayCard1"),  EKeys::One   },
		{ TEXT("IA_PlayCard2"),  EKeys::Two   },
		{ TEXT("IA_PlayCard3"),  EKeys::Three },
		{ TEXT("IA_PlayCard4"),  EKeys::Four  },
		{ TEXT("IA_PlayCard5"),  EKeys::Five  },
		{ TEXT("IA_PlayCard6"),  EKeys::Six   },
		{ TEXT("IA_PlayCard7"),  EKeys::Seven },
		{ TEXT("IA_Wait"),       EKeys::W     },
		{ TEXT("IA_EndTurn"),    EKeys::E     },
	};

	TArray<TPair<UInputAction*, FKey>> BattleActions;
	BattleActions.Reserve(BattleActionDefs.Num());

	for (const FInputActionDef& Def : BattleActionDefs)
	{
		UInputAction* IA = CreateBoolIA(BasePath, Def.AssetName);
		if (!IA)
		{
			UE_LOG(LogTemp, Error, TEXT("[WacomCreateInputAssets] Failed to create %s"),
				*Def.AssetName.ToString());
			return 1;
		}
		BattleActions.Add({ IA, Def.DefaultKey });
		UE_LOG(LogTemp, Display, TEXT("[WacomCreateInputAssets] Created %s"),
			*Def.AssetName.ToString());
	}

	const FString BattleIMCPkgPath = BasePath / TEXT("IMC_Battle");
	UPackage* BattleIMCPkg = FindOrCreatePackage(BattleIMCPkgPath);
	if (!BattleIMCPkg)
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomCreateInputAssets] Failed to create IMC_Battle package"));
		return 2;
	}
	UInputMappingContext* IMC_Battle = CreateOrReplaceAsset<UInputMappingContext>(
		BattleIMCPkg, TEXT("IMC_Battle"));
	if (!IMC_Battle)
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomCreateInputAssets] Failed to create IMC_Battle"));
		return 3;
	}
	for (const auto& Pair : BattleActions)
	{
		IMC_Battle->MapKey(Pair.Key, Pair.Value);
	}
	SaveAssetPackage(BattleIMCPkg, IMC_Battle, BattleIMCPkgPath);
	UE_LOG(LogTemp, Display, TEXT("[WacomCreateInputAssets] Created IMC_Battle with %d mappings"),
		BattleActions.Num());

	// ================ Exploration IMC ================

	UInputAction* IA_Move = CreateAxis2DIA(BasePath, TEXT("IA_Move"));
	UInputAction* IA_Look = CreateAxis2DIA(BasePath, TEXT("IA_Look"));
	if (!IA_Move || !IA_Look)
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomCreateInputAssets] Failed to create IA_Move/IA_Look"));
		return 4;
	}
	UE_LOG(LogTemp, Display, TEXT("[WacomCreateInputAssets] Created IA_Move / IA_Look"));

	const FString ExpIMCPkgPath = BasePath / TEXT("IMC_Exploration");
	UPackage* ExpIMCPkg = FindOrCreatePackage(ExpIMCPkgPath);
	if (!ExpIMCPkg)
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomCreateInputAssets] Failed to create IMC_Exploration package"));
		return 5;
	}
	UInputMappingContext* IMC_Exploration = CreateOrReplaceAsset<UInputMappingContext>(
		ExpIMCPkg, TEXT("IMC_Exploration"));
	if (!IMC_Exploration)
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomCreateInputAssets] Failed to create IMC_Exploration"));
		return 6;
	}

	// ---- IA_Move: WASD -> Axis2D ----
	// X 轴为右方向 / Y 轴为前方向。
	// D = X+, A = X-（Negate X）
	// W = Y+（Swizzle YXZ 把 X 换到 Y）
	// S = Y-（Swizzle YXZ + Negate X 得到 Y-）
	{
		FEnhancedActionKeyMapping& D_Map = IMC_Exploration->MapKey(IA_Move, EKeys::D);
		(void)D_Map;

		FEnhancedActionKeyMapping& A_Map = IMC_Exploration->MapKey(IA_Move, EKeys::A);
		A_Map.Modifiers.Add(MakeNegate(IMC_Exploration, /*X*/true, false, false));

		FEnhancedActionKeyMapping& W_Map = IMC_Exploration->MapKey(IA_Move, EKeys::W);
		W_Map.Modifiers.Add(MakeSwizzleYXZ(IMC_Exploration));

		FEnhancedActionKeyMapping& S_Map = IMC_Exploration->MapKey(IA_Move, EKeys::S);
		S_Map.Modifiers.Add(MakeNegate(IMC_Exploration, /*X*/true, false, false));
		S_Map.Modifiers.Add(MakeSwizzleYXZ(IMC_Exploration));
	}

	// ---- IA_Look: Mouse XY -> Axis2D ----
	// UE 里 pitch+ 表示低头，所以对 Y 取负让"鼠标上移 = 抬头"。
	{
		FEnhancedActionKeyMapping& Look_Map = IMC_Exploration->MapKey(IA_Look, EKeys::Mouse2D);
		Look_Map.Modifiers.Add(MakeNegate(IMC_Exploration, /*X*/false, /*Y*/true, /*Z*/false));
	}

	// ---- Exploration commands ----
	UInputAction* IA_OpenMenu = CreateBoolIA(BasePath, TEXT("IA_OpenMenu"));
	if (!IA_OpenMenu)
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomCreateInputAssets] Failed to create IA_OpenMenu"));
		return 7;
	}
	UE_LOG(LogTemp, Display, TEXT("[WacomCreateInputAssets] Created IA_OpenMenu"));

	UInputAction* IA_Interact = CreateBoolIA(BasePath, TEXT("IA_Interact"));
	if (!IA_Interact)
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomCreateInputAssets] Failed to create IA_Interact"));
		return 8;
	}
	UE_LOG(LogTemp, Display, TEXT("[WacomCreateInputAssets] Created IA_Interact"));

	UInputAction* IA_OpenBackpack = CreateBoolIA(BasePath, TEXT("IA_OpenBackpack"));
	if (!IA_OpenBackpack)
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomCreateInputAssets] Failed to create IA_OpenBackpack"));
		return 9;
	}
	UE_LOG(LogTemp, Display, TEXT("[WacomCreateInputAssets] Created IA_OpenBackpack"));

	IMC_Exploration->MapKey(IA_OpenMenu, EKeys::Escape);
	IMC_Exploration->MapKey(IA_Interact, EKeys::E);
	IMC_Exploration->MapKey(IA_OpenBackpack, EKeys::B);

	// 也加到 IMC_Battle（战斗中也能 ESC 暂停）
	IMC_Battle->MapKey(IA_OpenMenu, EKeys::Escape);

	SaveAssetPackage(ExpIMCPkg, IMC_Exploration, ExpIMCPkgPath);
	// 重新保存 IMC_Battle（刚加了 ESC 映射）
	SaveAssetPackage(BattleIMCPkg, IMC_Battle, BattleIMCPkgPath);

	UE_LOG(LogTemp, Display, TEXT("[WacomCreateInputAssets] Created IMC_Exploration"));

	UE_LOG(LogTemp, Display, TEXT("[WacomCreateInputAssets] Done"));
	return 0;
}
