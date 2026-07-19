// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/CombatActivityUIBuilder.h"

#include "ContentBuilders/ContentBuilderHelpers.h"

#include "Animation/WidgetAnimation.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CheckBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Styling/SlateTypes.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Battle/BattleCombatActivityRowWidget.h"
#include "UI/Battle/BattleCombatLogFeedWidget.h"
#include "UI/Battle/BattleCombatLogTurnDividerWidget.h"
#include "UI/Battle/WacomBattleCombatActivityStyle.h"
#include "UI/Battle/WacomBattleCombatLogDetailsScreen.h"
#include "UI/Battle/WacomBattleEnemyIntentPresentationStyle.h"
#include "UI/Battle/WacomBattleStatusIconWidget.h"
#include "UObject/MetaData.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"

namespace
{
	using namespace Wacom::ContentBuilder;

	constexpr TCHAR AssetRoot[] = TEXT("/Game/Wacom/UI/Battle/CombatLog");
	constexpr TCHAR FeedAssetName[] = TEXT("WBP_BattleCombatLogFeed");
	constexpr TCHAR RowAssetName[] = TEXT("WBP_BattleCombatActivityRow");
	constexpr TCHAR DividerAssetName[] = TEXT("WBP_BattleCombatLogTurnDivider");
	constexpr TCHAR DetailsAssetName[] = TEXT("WBP_BattleCombatLogDetailsScreen");
	constexpr TCHAR StyleAssetName[] = TEXT("DA_BattleCombatActivityStyle_Default");
	constexpr TCHAR AtlasAssetName[] = TEXT("T_BattleCombatActivityIcons_Default");
	constexpr TCHAR BattleHudObjectPath[] =
		TEXT("/Game/Wacom/UI/Battle/BP_BattleHUD.BP_BattleHUD");
	constexpr TCHAR IntentStyleObjectPath[] =
		TEXT("/Game/Wacom/UI/Enemy/Intent/DA_EnemyIntentPresentation_Default.DA_EnemyIntentPresentation_Default");
	constexpr TCHAR StatusListClassPath[] =
		TEXT("/Game/Wacom/UI/Battle/PlayerStatusBar/WBP_BattleStatusIconList.WBP_BattleStatusIconList_C");
	constexpr TCHAR WidgetContractMarker[] =
		TEXT("WacomCombatActivityWBP.ContractVersion=1");
	constexpr TCHAR HudPlacementMarker[] =
		TEXT("WacomCombatActivityHUDPlacement.ContractVersion=1");
	constexpr TCHAR ContentContractKey[] = TEXT("WacomContract");
	constexpr TCHAR ContentContractValue[] =
		TEXT("WacomCombatActivityUI.ContractVersion=1");

	constexpr int32 IconSize = 32;
	constexpr int32 IconCount = 6;
	constexpr int32 AtlasWidth = IconSize * IconCount;
	constexpr int32 AtlasHeight = IconSize;

	enum class EActivityIcon : uint8
	{
		Player,
		Damage,
		CardFlow,
		Wait,
		System,
		Hourglass,
	};

	struct FWidgetBlueprintAsset
	{
		UWidgetBlueprint* Blueprint = nullptr;
		FString PackagePath;
		bool bCreated = false;
	};

	struct FCombatActivityAssets
	{
		UTexture2D* Atlas = nullptr;
		UWacomBattleCombatActivityStyle* Style = nullptr;
		UWidgetBlueprint* RowBlueprint = nullptr;
		UWidgetBlueprint* FeedBlueprint = nullptr;
		UWidgetBlueprint* DividerBlueprint = nullptr;
		UWidgetBlueprint* DetailsBlueprint = nullptr;
	};

	void SetPixel(TArray<FColor>& Pixels, int32 X, int32 Y, const FColor& Color, int32 Radius = 0)
	{
		for (int32 OffsetY = -Radius; OffsetY <= Radius; ++OffsetY)
		{
			for (int32 OffsetX = -Radius; OffsetX <= Radius; ++OffsetX)
			{
				const int32 PixelX = X + OffsetX;
				const int32 PixelY = Y + OffsetY;
				if (PixelX >= 0 && PixelX < AtlasWidth && PixelY >= 0 && PixelY < AtlasHeight)
				{
					Pixels[PixelY * AtlasWidth + PixelX] = Color;
				}
			}
		}
	}

	void DrawLine(TArray<FColor>& Pixels, int32 X0, int32 Y0, int32 X1, int32 Y1,
		const FColor& Color, int32 Radius = 0)
	{
		const int32 DeltaX = FMath::Abs(X1 - X0);
		const int32 StepX = X0 < X1 ? 1 : -1;
		const int32 DeltaY = -FMath::Abs(Y1 - Y0);
		const int32 StepY = Y0 < Y1 ? 1 : -1;
		int32 Error = DeltaX + DeltaY;
		while (true)
		{
			SetPixel(Pixels, X0, Y0, Color, Radius);
			if (X0 == X1 && Y0 == Y1)
			{
				break;
			}
			const int32 TwiceError = Error * 2;
			if (TwiceError >= DeltaY)
			{
				Error += DeltaY;
				X0 += StepX;
			}
			if (TwiceError <= DeltaX)
			{
				Error += DeltaX;
				Y0 += StepY;
			}
		}
	}

	void DrawRectOutline(TArray<FColor>& Pixels, int32 X0, int32 Y0, int32 X1, int32 Y1,
		const FColor& Color)
	{
		DrawLine(Pixels, X0, Y0, X1, Y0, Color);
		DrawLine(Pixels, X1, Y0, X1, Y1, Color);
		DrawLine(Pixels, X1, Y1, X0, Y1, Color);
		DrawLine(Pixels, X0, Y1, X0, Y0, Color);
	}

	TArray<FColor> BuildAtlasPixels()
	{
		TArray<FColor> Pixels;
		Pixels.Init(FColor(0, 0, 0, 0), AtlasWidth * AtlasHeight);
		const FColor Ice(150, 215, 255, 255);
		const FColor Gold(248, 210, 112, 255);
		const FColor Paper(240, 244, 255, 255);
		const FColor Magenta(224, 82, 170, 255);

		auto CellX = [](EActivityIcon Kind, int32 LocalX)
		{
			return static_cast<int32>(Kind) * IconSize + LocalX;
		};
		auto Pixel = [&Pixels, &CellX](EActivityIcon Kind, int32 X, int32 Y,
			const FColor& Color, int32 Radius = 0)
		{
			SetPixel(Pixels, CellX(Kind, X), Y, Color, Radius);
		};
		auto Line = [&Pixels, &CellX](EActivityIcon Kind, int32 X0, int32 Y0,
			int32 X1, int32 Y1, const FColor& Color, int32 Radius = 0)
		{
			DrawLine(Pixels, CellX(Kind, X0), Y0, CellX(Kind, X1), Y1, Color, Radius);
		};

		// Neutral player silhouette.
		for (int32 Y = 7; Y <= 13; ++Y)
		{
			const int32 Half = (Y <= 9 || Y >= 12) ? 3 : 4;
			Line(EActivityIcon::Player, 16 - Half, Y, 16 + Half, Y, Ice);
		}
		for (int32 Y = 16; Y <= 26; ++Y)
		{
			const int32 Half = FMath::Min(9, 4 + (Y - 16));
			Line(EActivityIcon::Player, 16 - Half, Y, 16 + Half, Y, Paper);
		}

		// Damage burst.
		for (int32 Radius = 2; Radius <= 9; Radius += 3)
		{
			Pixel(EActivityIcon::Damage, 16 + Radius, 16, Magenta, 1);
			Pixel(EActivityIcon::Damage, 16 - Radius, 16, Magenta, 1);
			Pixel(EActivityIcon::Damage, 16, 16 + Radius, Gold, 1);
			Pixel(EActivityIcon::Damage, 16, 16 - Radius, Gold, 1);
		}
		Line(EActivityIcon::Damage, 9, 9, 23, 23, Paper, 1);
		Line(EActivityIcon::Damage, 23, 9, 9, 23, Paper, 1);

		// Card flow: two stacked cards and arrow.
		DrawRectOutline(Pixels, CellX(EActivityIcon::CardFlow, 7), 8,
			CellX(EActivityIcon::CardFlow, 20), 23, Ice);
		DrawRectOutline(Pixels, CellX(EActivityIcon::CardFlow, 11), 5,
			CellX(EActivityIcon::CardFlow, 24), 20, Paper);
		Line(EActivityIcon::CardFlow, 8, 26, 23, 26, Gold, 1);
		Line(EActivityIcon::CardFlow, 20, 23, 24, 26, Gold);
		Line(EActivityIcon::CardFlow, 20, 29, 24, 26, Gold);

		// Wait pause mark.
		for (int32 Y = 7; Y <= 25; ++Y)
		{
			Line(EActivityIcon::Wait, 10, Y, 13, Y, Ice);
			Line(EActivityIcon::Wait, 19, Y, 22, Y, Gold);
		}

		// System diamond/cross fallback.
		Line(EActivityIcon::System, 16, 4, 28, 16, Ice, 1);
		Line(EActivityIcon::System, 28, 16, 16, 28, Ice, 1);
		Line(EActivityIcon::System, 16, 28, 4, 16, Gold, 1);
		Line(EActivityIcon::System, 4, 16, 16, 4, Gold, 1);
		Line(EActivityIcon::System, 11, 16, 21, 16, Paper);
		Line(EActivityIcon::System, 16, 11, 16, 21, Paper);

		// Hourglass.
		Line(EActivityIcon::Hourglass, 8, 5, 24, 5, Paper, 1);
		Line(EActivityIcon::Hourglass, 8, 27, 24, 27, Paper, 1);
		Line(EActivityIcon::Hourglass, 9, 7, 23, 25, Ice);
		Line(EActivityIcon::Hourglass, 23, 7, 9, 25, Gold);
		Line(EActivityIcon::Hourglass, 12, 11, 20, 11, Ice);
		Line(EActivityIcon::Hourglass, 13, 21, 19, 21, Gold);

		return Pixels;
	}

	bool HasManagedContentMarker(const UObject* Object)
	{
		if (!Object || !Object->GetOutermost())
		{
			return false;
		}
		return Object->GetOutermost()->GetMetaData().GetValue(Object, ContentContractKey)
			== ContentContractValue;
	}

	void SetManagedContentMarker(UObject* Object)
	{
		check(Object && Object->GetOutermost());
		Object->GetOutermost()->GetMetaData().SetValue(
			Object, ContentContractKey, ContentContractValue);
	}

	bool IsAtlasValid(const UTexture2D* Texture)
	{
		return Texture
			&& HasManagedContentMarker(Texture)
			&& Texture->Source.GetSizeX() == AtlasWidth
			&& Texture->Source.GetSizeY() == AtlasHeight
			&& Texture->Source.GetFormat() == TSF_BGRA8
			&& Texture->CompressionSettings == TC_EditorIcon
			&& Texture->Filter == TF_Nearest
			&& Texture->MipGenSettings == TMGS_NoMipmaps
			&& Texture->LODGroup == TEXTUREGROUP_UI
			&& Texture->SRGB;
	}

	UTexture2D* EnsureAtlas(bool bBuild)
	{
		const FString PackagePath = MakePackagePath(AssetRoot, AtlasAssetName);
		const FString ObjectPath = MakeObjectPath(PackagePath);
		UObject* Existing = StaticLoadObject(UObject::StaticClass(), nullptr, *ObjectPath);
		UTexture2D* Texture = Cast<UTexture2D>(Existing);
		if (Existing && !Texture)
		{
			UE_LOG(LogTemp, Error, TEXT("[CombatActivityUIBuilder] Atlas has wrong class: %s"),
				*ObjectPath);
			return nullptr;
		}
		if (Texture && !HasManagedContentMarker(Texture))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[CombatActivityUIBuilder] Unknown manual atlas detected; no overwrite: %s"),
				*ObjectPath);
			return nullptr;
		}
		if (IsAtlasValid(Texture))
		{
			return Texture;
		}
		if (!bBuild)
		{
			UE_LOG(LogTemp, Error, TEXT("[CombatActivityUIBuilder] Missing/invalid atlas: %s"),
				*ObjectPath);
			return nullptr;
		}

		UPackage* Package = Texture ? Texture->GetOutermost() : FindOrCreatePackage(PackagePath);
		if (!Package)
		{
			return nullptr;
		}
		if (!Texture)
		{
			Texture = NewObject<UTexture2D>(Package, AtlasAssetName,
				RF_Public | RF_Standalone | RF_Transactional);
		}
		Texture->Modify();
		const TArray<FColor> Pixels = BuildAtlasPixels();
		Texture->Source.Init(AtlasWidth, AtlasHeight, 1, 1, TSF_BGRA8,
			reinterpret_cast<const uint8*>(Pixels.GetData()));
		Texture->CompressionSettings = TC_EditorIcon;
		Texture->MipGenSettings = TMGS_NoMipmaps;
		Texture->LODGroup = TEXTUREGROUP_UI;
		Texture->Filter = TF_Nearest;
		Texture->AddressX = TA_Clamp;
		Texture->AddressY = TA_Clamp;
		Texture->NeverStream = true;
		Texture->SRGB = true;
		SetManagedContentMarker(Texture);
		Texture->PostEditChange();
		return SaveAssetPackage(Package, Texture, PackagePath) ? Texture : nullptr;
	}

	FSlateBrush MakeAtlasBrush(UTexture2D* Atlas, EActivityIcon Icon, float ImageSize = 32.0f)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Atlas);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.ImageSize = FVector2D(ImageSize, ImageSize);
		Brush.TintColor = FSlateColor(FLinearColor::White);
		const float MinX = static_cast<int32>(Icon) / static_cast<float>(IconCount);
		const float MaxX = (static_cast<int32>(Icon) + 1) / static_cast<float>(IconCount);
		Brush.SetUVRegion(FBox2f(FVector2f(MinX, 0.0f), FVector2f(MaxX, 1.0f)));
		return Brush;
	}

	bool IsBrushAssigned(const FSlateBrush& Brush)
	{
		return Brush.GetResourceObject() != nullptr;
	}

	const FSlateBrush* ResolveProtectedBrush(const UObject* Object, const FName PropertyName)
	{
		if (!Object)
		{
			return nullptr;
		}
		const FStructProperty* Property = FindFProperty<FStructProperty>(Object->GetClass(), PropertyName);
		if (!Property || Property->Struct != TBaseStructure<FSlateBrush>::Get())
		{
			return nullptr;
		}
		return Property->ContainerPtrToValuePtr<FSlateBrush>(Object);
	}

	bool AddMissingTagIcon(UWacomBattleCombatActivityStyle& Style, FGameplayTag Tag,
		const FSlateBrush* SourceBrush)
	{
		if (!Tag.IsValid() || !SourceBrush || !IsBrushAssigned(*SourceBrush))
		{
			return false;
		}
		for (const FWacomBattleCombatActivityTagIconEntry& Entry : Style.TagIcons)
		{
			if (Entry.Tag == Tag)
			{
				return false;
			}
		}
		FWacomBattleCombatActivityTagIconEntry& Entry = Style.TagIcons.AddDefaulted_GetRef();
		Entry.Tag = Tag;
		Entry.IconBrush = *SourceBrush;
		return true;
	}

	UWacomBattleCombatActivityStyle* EnsureStyle(UTexture2D* Atlas, bool bBuild)
	{
		if (!Atlas)
		{
			return nullptr;
		}
		const FString PackagePath = MakePackagePath(AssetRoot, StyleAssetName);
		const FString ObjectPath = MakeObjectPath(PackagePath);
		UObject* Existing = StaticLoadObject(UObject::StaticClass(), nullptr, *ObjectPath);
		UWacomBattleCombatActivityStyle* Style = Cast<UWacomBattleCombatActivityStyle>(Existing);
		if (Existing && !Style)
		{
			UE_LOG(LogTemp, Error, TEXT("[CombatActivityUIBuilder] Style has wrong class: %s"),
				*ObjectPath);
			return nullptr;
		}
		if (!Style && !bBuild)
		{
			UE_LOG(LogTemp, Error, TEXT("[CombatActivityUIBuilder] Missing style: %s"), *ObjectPath);
			return nullptr;
		}
		if (Style && !bBuild)
		{
			const bool bReady = HasManagedContentMarker(Style)
				&& IsBrushAssigned(Style->PlayerPortraitBrush)
				&& IsBrushAssigned(Style->DamageIconBrush)
				&& IsBrushAssigned(Style->CardFlowIconBrush)
				&& IsBrushAssigned(Style->WaitIconBrush)
				&& IsBrushAssigned(Style->SystemIconBrush)
				&& IsBrushAssigned(Style->FallbackIconBrush)
				&& IsBrushAssigned(Style->TurnIconBrush)
				&& Style->MaxVisibleRows == 3;
			if (!bReady)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[CombatActivityUIBuilder] Style contract incomplete: %s"), *ObjectPath);
				return nullptr;
			}
			return Style;
		}
		UPackage* Package = Style ? Style->GetOutermost() : FindOrCreatePackage(PackagePath);
		if (!Style && Package)
		{
			Style = NewObject<UWacomBattleCombatActivityStyle>(Package, StyleAssetName,
				RF_Public | RF_Standalone | RF_Transactional);
		}
		if (!Style)
		{
			return nullptr;
		}

		UWacomBattleEnemyIntentPresentationStyle* IntentStyle =
			Cast<UWacomBattleEnemyIntentPresentationStyle>(StaticLoadObject(
				UWacomBattleEnemyIntentPresentationStyle::StaticClass(), nullptr,
				IntentStyleObjectPath));
		UClass* StatusListClass = LoadClass<UWacomBattleStatusIconListWidget>(
			nullptr, StatusListClassPath);
		const UObject* StatusDefaults = StatusListClass ? StatusListClass->GetDefaultObject() : nullptr;

		bool bChanged = false;
		auto FillBrush = [&Style, &bChanged](FSlateBrush& Target, const FSlateBrush& Value)
		{
			if (!IsBrushAssigned(Target))
			{
				Style->Modify();
				Target = Value;
				bChanged = true;
			}
		};
		FillBrush(Style->PlayerPortraitBrush, MakeAtlasBrush(Atlas, EActivityIcon::Player));
		FillBrush(Style->DamageIconBrush, MakeAtlasBrush(Atlas, EActivityIcon::Damage));
		FillBrush(Style->CardFlowIconBrush, MakeAtlasBrush(Atlas, EActivityIcon::CardFlow));
		FillBrush(Style->WaitIconBrush, MakeAtlasBrush(Atlas, EActivityIcon::Wait));
		FillBrush(Style->SystemIconBrush, MakeAtlasBrush(Atlas, EActivityIcon::System));
		FillBrush(Style->FallbackIconBrush, MakeAtlasBrush(Atlas, EActivityIcon::System));
		FillBrush(Style->TurnIconBrush, MakeAtlasBrush(Atlas, EActivityIcon::Hourglass, 20.0f));
		if (!Style->EnemyIntentStyle && IntentStyle)
		{
			Style->Modify();
			Style->EnemyIntentStyle = IntentStyle;
			bChanged = true;
		}

		struct FStatusProperty
		{
			FGameplayTag Tag;
			FName Property;
		};
		const FStatusProperty StatusProperties[] = {
			{ WacomTags::Status_Poison, TEXT("PoisonIconBrush") },
			{ WacomTags::Status_Slow, TEXT("SlowIconBrush") },
			{ WacomTags::Status_Freeze, TEXT("FreezeIconBrush") },
			{ WacomTags::Status_Twilight, TEXT("TwilightIconBrush") },
			{ WacomTags::Status_Stunned, TEXT("StunnedIconBrush") },
		};
		for (const FStatusProperty& Status : StatusProperties)
		{
			if (AddMissingTagIcon(*Style, Status.Tag,
				ResolveProtectedBrush(StatusDefaults, Status.Property)))
			{
				Style->Modify();
				bChanged = true;
			}
		}

		if (!HasManagedContentMarker(Style))
		{
			Style->Modify();
			SetManagedContentMarker(Style);
			bChanged = true;
		}
		const bool bRequiredReady = IsBrushAssigned(Style->PlayerPortraitBrush)
			&& IsBrushAssigned(Style->DamageIconBrush)
			&& IsBrushAssigned(Style->CardFlowIconBrush)
			&& IsBrushAssigned(Style->WaitIconBrush)
			&& IsBrushAssigned(Style->SystemIconBrush)
			&& IsBrushAssigned(Style->FallbackIconBrush)
			&& IsBrushAssigned(Style->TurnIconBrush)
			&& Style->MaxVisibleRows == 3;
		if (!bRequiredReady)
		{
			UE_LOG(LogTemp, Error, TEXT("[CombatActivityUIBuilder] Style contract incomplete: %s"),
				*ObjectPath);
			return nullptr;
		}
		if (bChanged)
		{
			if (!bBuild || !SaveAssetPackage(Package, Style, PackagePath))
			{
				return nullptr;
			}
		}
		return Style;
	}

	FWidgetBlueprintAsset LoadOrCreateWidgetBlueprint(const TCHAR* AssetName,
		UClass* ParentClass, bool bAllowCreate)
	{
		FWidgetBlueprintAsset Result;
		Result.PackagePath = MakePackagePath(AssetRoot, AssetName);
		const FString ObjectPath = MakeObjectPath(Result.PackagePath);
		if (UObject* Existing = StaticLoadObject(UObject::StaticClass(), nullptr, *ObjectPath))
		{
			Result.Blueprint = Cast<UWidgetBlueprint>(Existing);
			if (!Result.Blueprint || !Result.Blueprint->ParentClass
				|| !Result.Blueprint->ParentClass->IsChildOf(ParentClass))
			{
				UE_LOG(LogTemp, Error,
					TEXT("[CombatActivityUIBuilder] Existing WBP has incompatible class: %s"),
					*ObjectPath);
				Result.Blueprint = nullptr;
			}
			return Result;
		}
		if (!bAllowCreate)
		{
			UE_LOG(LogTemp, Error, TEXT("[CombatActivityUIBuilder] Missing WBP: %s"),
				*ObjectPath);
			return Result;
		}
		UPackage* Package = FindOrCreatePackage(Result.PackagePath);
		if (Package)
		{
			Result.Blueprint = Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(
				ParentClass, Package, AssetName, BPTYPE_Normal,
				UWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass()));
			Result.bCreated = Result.Blueprint != nullptr;
		}
		return Result;
	}

	void ResetWidgetBlueprint(UWidgetBlueprint& Blueprint, const FString& Description)
	{
		Blueprint.Modify();
		if (UWidgetTree* PreviousTree = Blueprint.WidgetTree)
		{
			PreviousTree->Rename(nullptr, GetTransientPackage(),
				REN_DontCreateRedirectors | REN_NonTransactional);
		}
		for (UWidgetAnimation* Animation : Blueprint.Animations)
		{
			if (Animation)
			{
				Animation->Rename(nullptr, GetTransientPackage(),
					REN_DontCreateRedirectors | REN_NonTransactional);
			}
		}
		Blueprint.WidgetTree = NewObject<UWidgetTree>(&Blueprint, TEXT("WidgetTree"), RF_Transactional);
		Blueprint.Bindings.Reset();
		Blueprint.Animations.Reset();
		Blueprint.WidgetVariableNameToGuidMap.Reset();
		Blueprint.BlueprintDescription = Description + TEXT("\n") + WidgetContractMarker;
		Blueprint.bCanCallInitializedWithoutPlayerContext = true;
	}

	void RegisterWidgetGuid(UWidgetBlueprint& Blueprint, const UWidget& Widget)
	{
		const FString StablePath = FString::Printf(TEXT("%s:%s"),
			*Blueprint.GetPathName(), *Widget.GetName());
		Blueprint.WidgetVariableNameToGuidMap.FindOrAdd(Widget.GetFName()) =
			FGuid::NewDeterministicGuid(StablePath);
	}

	void MarkWidgetVariable(UWidgetBlueprint& Blueprint, UWidget& Widget)
	{
		Widget.bIsVariable = true;
		RegisterWidgetGuid(Blueprint, Widget);
	}

	void RegisterAllWidgetGuids(UWidgetBlueprint& Blueprint)
	{
		TSet<FName> LiveNames;
		Blueprint.WidgetTree->ForEachWidget([&](UWidget* Widget)
		{
			if (Widget)
			{
				LiveNames.Add(Widget->GetFName());
				RegisterWidgetGuid(Blueprint, *Widget);
			}
		});
		for (auto It = Blueprint.WidgetVariableNameToGuidMap.CreateIterator(); It; ++It)
		{
			if (!LiveNames.Contains(It.Key()))
			{
				It.RemoveCurrent();
			}
		}
	}

	bool CompileAndSave(UWidgetBlueprint& Blueprint)
	{
		RegisterAllWidgetGuids(Blueprint);
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(&Blueprint);
		FKismetEditorUtilities::CompileBlueprint(&Blueprint);
		if (Blueprint.Status == BS_Error || !Blueprint.GeneratedClass)
		{
			UE_LOG(LogTemp, Error, TEXT("[CombatActivityUIBuilder] Compile failed: %s"),
				*Blueprint.GetPathName());
			return false;
		}
		FAssetRegistryModule::AssetCreated(&Blueprint);
		UPackage* Package = Blueprint.GetOutermost();
		Package->MarkPackageDirty();
		Blueprint.MarkPackageDirty();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, &Blueprint, *Filename, Args);
	}

	bool SaveCompiledDefaults(UWidgetBlueprint& Blueprint)
	{
		check(Blueprint.GeneratedClass);
		UPackage* Package = Blueprint.GetOutermost();
		Package->MarkPackageDirty();
		Blueprint.GeneratedClass->GetDefaultObject()->MarkPackageDirty();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, &Blueprint, *Filename, Args);
	}

	void StyleText(UTextBlock& Text, int32 Size, const FLinearColor& Color)
	{
		FSlateFontInfo Font = Text.GetFont();
		Font.Size = Size;
		Font.TypefaceFontName = TEXT("Bold");
		Text.SetFont(Font);
		Text.SetColorAndOpacity(FSlateColor(Color));
		Text.SetShadowOffset(FVector2D(1.0f, 1.0f));
		Text.SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.90f));
	}

	bool BuildRowBlueprint(UWidgetBlueprint& Blueprint)
	{
		ResetWidgetBlueprint(Blueprint,
			TEXT("Builder-managed BattleHUD combat activity row. Runtime playback owns opacity and movement."));
		UWidgetTree* Tree = Blueprint.WidgetTree;
		USizeBox* Root = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ActivityRowSize"));
		Root->SetHeightOverride(40.0f);
		Root->SetVisibility(ESlateVisibility::HitTestInvisible);
		Tree->RootWidget = Root;
		UBorder* RowRoot = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RowRoot"));
		RowRoot->SetBrushColor(FLinearColor(0.025f, 0.035f, 0.055f, 0.76f));
		RowRoot->SetPadding(FMargin(4.0f, 3.0f));
		RowRoot->SetVisibility(ESlateVisibility::HitTestInvisible);
		Root->AddChild(RowRoot);
		MarkWidgetVariable(Blueprint, *RowRoot);

		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row"));
		Row->SetVisibility(ESlateVisibility::HitTestInvisible);
		RowRoot->SetContent(Row);
		USizeBox* Indent = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("IndentSpacer"));
		Indent->SetWidthOverride(0.0f);
		Indent->SetHeightOverride(1.0f);
		Indent->SetVisibility(ESlateVisibility::HitTestInvisible);
		Row->AddChildToHorizontalBox(Indent);
		MarkWidgetVariable(Blueprint, *Indent);
		USizeBox* IconSizeBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("IconSize"));
		IconSizeBox->SetWidthOverride(34.0f);
		IconSizeBox->SetHeightOverride(34.0f);
		IconSizeBox->SetVisibility(ESlateVisibility::HitTestInvisible);
		UImage* Icon = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ActivityIcon"));
		Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
		IconSizeBox->SetContent(Icon);
		if (UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(IconSizeBox))
		{
			Slot->SetPadding(FMargin(0.0f, 0.0f, 7.0f, 0.0f));
			Slot->SetVerticalAlignment(VAlign_Center);
		}
		MarkWidgetVariable(Blueprint, *Icon);
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ActivityText"));
		StyleText(*Text, 18, FLinearColor(0.94f, 0.95f, 1.0f, 1.0f));
		Text->SetText(FText::FromString(TEXT("行动播报")));
		Text->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(Text))
		{
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			Slot->SetVerticalAlignment(VAlign_Center);
		}
		MarkWidgetVariable(Blueprint, *Text);
		return CompileAndSave(Blueprint);
	}

	FButtonStyle MakeInvisibleButtonStyle()
	{
		FSlateBrush NoDraw;
		NoDraw.DrawAs = ESlateBrushDrawType::NoDrawType;
		FButtonStyle Style;
		Style.SetNormal(NoDraw);
		Style.SetHovered(NoDraw);
		Style.SetPressed(NoDraw);
		Style.SetDisabled(NoDraw);
		return Style;
	}

	bool BuildFeedBlueprint(UWidgetBlueprint& Blueprint)
	{
		ResetWidgetBlueprint(Blueprint,
			TEXT("Builder-managed fixed three-row BattleHUD combat activity broadcaster and footer."));
		UWidgetTree* Tree = Blueprint.WidgetTree;
		USizeBox* RootSize = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CombatActivitySize"));
		RootSize->SetWidthOverride(420.0f);
		RootSize->SetHeightOverride(190.0f);
		RootSize->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Tree->RootWidget = RootSize;
		UVerticalBox* Root = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
		Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		RootSize->AddChild(Root);
		UVerticalBox* Rows = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ActivityRowsBox"));
		Rows->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UVerticalBoxSlot* Slot = Root->AddChildToVerticalBox(Rows))
		{
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
		}
		MarkWidgetVariable(Blueprint, *Rows);

		USizeBox* FooterSize = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("FooterSize"));
		FooterSize->SetHeightOverride(42.0f);
		FooterSize->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Root->AddChildToVerticalBox(FooterSize);
		UHorizontalBox* Footer = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Footer"));
		Footer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		FooterSize->AddChild(Footer);

		USizeBox* ActionSize = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LastActionSize"));
		ActionSize->SetWidthOverride(38.0f);
		ActionSize->SetHeightOverride(38.0f);
		ActionSize->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (UHorizontalBoxSlot* Slot = Footer->AddChildToHorizontalBox(ActionSize))
		{
			Slot->SetPadding(FMargin(2.0f, 0.0f, 8.0f, 0.0f));
			Slot->SetVerticalAlignment(VAlign_Center);
		}
		UButton* ActionButton = Tree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("LastActionButton"));
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		ActionButton->IsFocusable = false;
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		ActionButton->SetStyle(MakeInvisibleButtonStyle());
		ActionButton->SetVisibility(ESlateVisibility::Visible);
		ActionSize->AddChild(ActionButton);
		MarkWidgetVariable(Blueprint, *ActionButton);
		UImage* ActionIcon = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("LastActionIcon"));
		ActionIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		ActionButton->SetContent(ActionIcon);
		MarkWidgetVariable(Blueprint, *ActionIcon);

		UHorizontalBox* TurnRoot = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TurnRoot"));
		TurnRoot->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UHorizontalBoxSlot* Slot = Footer->AddChildToHorizontalBox(TurnRoot))
		{
			Slot->SetVerticalAlignment(VAlign_Center);
		}
		MarkWidgetVariable(Blueprint, *TurnRoot);
		USizeBox* TurnIconSize = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("TurnIconSize"));
		TurnIconSize->SetWidthOverride(22.0f);
		TurnIconSize->SetHeightOverride(22.0f);
		TurnIconSize->SetVisibility(ESlateVisibility::HitTestInvisible);
		TurnRoot->AddChildToHorizontalBox(TurnIconSize);
		UImage* TurnIcon = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TurnIcon"));
		TurnIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		TurnIconSize->AddChild(TurnIcon);
		MarkWidgetVariable(Blueprint, *TurnIcon);
		UTextBlock* TurnText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TurnText"));
		StyleText(*TurnText, 18, FLinearColor(0.88f, 0.92f, 1.0f, 1.0f));
		TurnText->SetText(FText::AsNumber(1));
		TurnText->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UHorizontalBoxSlot* Slot = TurnRoot->AddChildToHorizontalBox(TurnText))
		{
			Slot->SetPadding(FMargin(4.0f, 0.0f, 0.0f, 0.0f));
			Slot->SetVerticalAlignment(VAlign_Center);
		}
		MarkWidgetVariable(Blueprint, *TurnText);
		return CompileAndSave(Blueprint);
	}

	bool BuildTurnDividerBlueprint(UWidgetBlueprint& Blueprint)
	{
		ResetWidgetBlueprint(Blueprint,
			TEXT("Builder-managed Battle Combat Log turn start/end divider."));
		UWidgetTree* Tree = Blueprint.WidgetTree;
		USizeBox* RootSize = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("TurnDividerSize"));
		RootSize->SetHeightOverride(38.0f);
		RootSize->SetVisibility(ESlateVisibility::HitTestInvisible);
		Tree->RootWidget = RootSize;

		UBorder* DividerRoot = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DividerRoot"));
		DividerRoot->SetBrushColor(FLinearColor(0.025f, 0.065f, 0.10f, 0.82f));
		DividerRoot->SetPadding(FMargin(8.0f, 5.0f));
		DividerRoot->SetVisibility(ESlateVisibility::HitTestInvisible);
		RootSize->SetContent(DividerRoot);
		MarkWidgetVariable(Blueprint, *DividerRoot);

		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("DividerRow"));
		Row->SetVisibility(ESlateVisibility::HitTestInvisible);
		DividerRoot->SetContent(Row);
		USizeBox* TurnIconSizeBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("TurnIconSize"));
		TurnIconSizeBox->SetWidthOverride(24.0f);
		TurnIconSizeBox->SetHeightOverride(24.0f);
		TurnIconSizeBox->SetVisibility(ESlateVisibility::HitTestInvisible);
		UImage* TurnIcon = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TurnIcon"));
		TurnIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		TurnIconSizeBox->SetContent(TurnIcon);
		if (UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(TurnIconSizeBox))
		{
			IconSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
			IconSlot->SetVerticalAlignment(VAlign_Center);
		}
		MarkWidgetVariable(Blueprint, *TurnIcon);
		UTextBlock* TurnText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TurnText"));
		StyleText(*TurnText, 18, FLinearColor(0.48f, 0.82f, 1.0f, 1.0f));
		TurnText->SetText(FText::FromString(TEXT("第 1 回合开始")));
		TurnText->SetVisibility(ESlateVisibility::HitTestInvisible);
		Row->AddChildToHorizontalBox(TurnText);
		MarkWidgetVariable(Blueprint, *TurnText);
		return CompileAndSave(Blueprint);
	}

	bool BuildDetailsBlueprint(UWidgetBlueprint& Blueprint)
	{
		ResetWidgetBlueprint(Blueprint,
			TEXT("Builder-managed Battle Combat Log details secondary panel. All+NoCapture input is owned by the native base class."));
		UWidgetTree* Tree = Blueprint.WidgetTree;
		UCanvasPanel* FullScreenOverlay = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("FullScreenOverlay"));
		FullScreenOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Tree->RootWidget = FullScreenOverlay;

		UButton* BackdropButton = Tree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BackdropButton"));
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		BackdropButton->IsFocusable = false;
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		BackdropButton->SetStyle(MakeInvisibleButtonStyle());
		BackdropButton->SetVisibility(ESlateVisibility::Visible);
		UBorder* BackdropVisual = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackdropVisual"));
		BackdropVisual->SetBrushColor(FLinearColor(0.005f, 0.009f, 0.016f, 0.44f));
		BackdropVisual->SetVisibility(ESlateVisibility::HitTestInvisible);
		BackdropButton->SetContent(BackdropVisual);
		if (UCanvasPanelSlot* BackdropSlot = FullScreenOverlay->AddChildToCanvas(BackdropButton))
		{
			BackdropSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			BackdropSlot->SetOffsets(FMargin(0.0f));
			BackdropSlot->SetZOrder(0);
		}
		MarkWidgetVariable(Blueprint, *BackdropButton);

		USizeBox* PanelSize = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelSizeBox"));
		PanelSize->SetWidthOverride(680.0f);
		PanelSize->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (UCanvasPanelSlot* PanelSlot = FullScreenOverlay->AddChildToCanvas(PanelSize))
		{
			PanelSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 1.0f));
			PanelSlot->SetOffsets(FMargin(24.0f, 24.0f, 680.0f, 24.0f));
			PanelSlot->SetZOrder(1);
		}

		UBorder* PanelRoot = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelRoot"));
		PanelRoot->SetBrushColor(FLinearColor(0.012f, 0.022f, 0.034f, 0.95f));
		PanelRoot->SetPadding(FMargin(18.0f));
		PanelRoot->SetVisibility(ESlateVisibility::Visible);
		PanelSize->SetContent(PanelRoot);
		MarkWidgetVariable(Blueprint, *PanelRoot);

		UVerticalBox* PanelContent = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PanelContent"));
		PanelContent->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		PanelRoot->SetContent(PanelContent);
		UHorizontalBox* Header = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Header"));
		Header->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (UVerticalBoxSlot* HeaderSlot = PanelContent->AddChildToVerticalBox(Header))
		{
			HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
		}

		UTextBlock* Title = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
		StyleText(*Title, 28, FLinearColor(0.94f, 0.97f, 1.0f, 1.0f));
		Title->SetText(FText::FromString(TEXT("战斗日志")));
		Title->SetVisibility(ESlateVisibility::HitTestInvisible);
		Header->AddChildToHorizontalBox(Title);
		MarkWidgetVariable(Blueprint, *Title);

		USpacer* HeaderSpacer = Tree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("HeaderSpacer"));
		if (UHorizontalBoxSlot* SpacerSlot = Header->AddChildToHorizontalBox(HeaderSpacer))
		{
			SpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		UTextBlock* DetailsLabel = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DetailsLabel"));
		StyleText(*DetailsLabel, 17, FLinearColor(0.82f, 0.86f, 0.93f, 1.0f));
		DetailsLabel->SetText(FText::FromString(TEXT("查看详情")));
		DetailsLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UHorizontalBoxSlot* LabelSlot = Header->AddChildToHorizontalBox(DetailsLabel))
		{
			LabelSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
			LabelSlot->SetVerticalAlignment(VAlign_Center);
		}

		UCheckBox* DetailsToggle = Tree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), TEXT("DetailsToggle"));
		DetailsToggle->SetIsChecked(false);
		DetailsToggle->SetVisibility(ESlateVisibility::Visible);
		if (UHorizontalBoxSlot* ToggleSlot = Header->AddChildToHorizontalBox(DetailsToggle))
		{
			ToggleSlot->SetPadding(FMargin(0.0f, 0.0f, 14.0f, 0.0f));
			ToggleSlot->SetVerticalAlignment(VAlign_Center);
		}
		MarkWidgetVariable(Blueprint, *DetailsToggle);

		UButton* CloseButton = Tree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		CloseButton->IsFocusable = false;
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		CloseButton->SetVisibility(ESlateVisibility::Visible);
		UTextBlock* CloseLabel = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseLabel"));
		StyleText(*CloseLabel, 16, FLinearColor(0.96f, 0.82f, 0.54f, 1.0f));
		CloseLabel->SetText(FText::FromString(TEXT("关闭")));
		CloseLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
		CloseButton->SetContent(CloseLabel);
		Header->AddChildToHorizontalBox(CloseButton);
		MarkWidgetVariable(Blueprint, *CloseButton);

		UScrollBox* Scroll = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("HistoryScrollBox"));
		Scroll->SetVisibility(ESlateVisibility::Visible);
		if (UVerticalBoxSlot* ScrollSlot = PanelContent->AddChildToVerticalBox(Scroll))
		{
			ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		MarkWidgetVariable(Blueprint, *Scroll);
		UVerticalBox* HistoryList = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HistoryList"));
		HistoryList->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Scroll->AddChild(HistoryList);
		MarkWidgetVariable(Blueprint, *HistoryList);
		UTextBlock* EmptyText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EmptyText"));
		StyleText(*EmptyText, 18, FLinearColor(0.58f, 0.64f, 0.72f, 1.0f));
		EmptyText->SetText(FText::FromString(TEXT("暂无战斗记录")));
		EmptyText->SetJustification(ETextJustify::Center);
		EmptyText->SetVisibility(ESlateVisibility::Collapsed);
		HistoryList->AddChildToVerticalBox(EmptyText);
		MarkWidgetVariable(Blueprint, *EmptyText);
		return CompileAndSave(Blueprint);
	}

	bool HasWidgetOfClass(const UWidgetBlueprint& Blueprint, const FName Name,
		const UClass* RequiredClass)
	{
		const UWidget* Widget = Blueprint.WidgetTree
			? Blueprint.WidgetTree->FindWidget(Name) : nullptr;
		return Widget && Widget->IsA(RequiredClass);
	}

	bool IsRecognizedLegacyFeed(const UWidgetBlueprint& Blueprint)
	{
		return Blueprint.ParentClass
			&& Blueprint.ParentClass->IsChildOf(UBattleCombatLogFeedWidget::StaticClass())
			&& Blueprint.WidgetTree
			&& (Blueprint.WidgetTree->FindWidget(TEXT("BlocksBox"))
				|| Blueprint.WidgetTree->FindWidget(TEXT("ActivityRowsBox")));
	}

	bool ValidateRowBlueprint(const UWidgetBlueprint& Blueprint, bool bLogErrors)
	{
		const UWidget* RowRoot = Blueprint.WidgetTree
			? Blueprint.WidgetTree->FindWidget(TEXT("RowRoot")) : nullptr;
		const UWidget* IndentSpacer = Blueprint.WidgetTree
			? Blueprint.WidgetTree->FindWidget(TEXT("IndentSpacer")) : nullptr;
		const UWidget* ActivityIcon = Blueprint.WidgetTree
			? Blueprint.WidgetTree->FindWidget(TEXT("ActivityIcon")) : nullptr;
		const UWidget* ActivityText = Blueprint.WidgetTree
			? Blueprint.WidgetTree->FindWidget(TEXT("ActivityText")) : nullptr;
		const bool bValid = Blueprint.ParentClass
			&& Blueprint.ParentClass->IsChildOf(UBattleCombatActivityRowWidget::StaticClass())
			&& Blueprint.BlueprintDescription.Contains(WidgetContractMarker)
			&& Blueprint.GeneratedClass
			&& RowRoot && RowRoot->IsA(UBorder::StaticClass())
			&& RowRoot->GetVisibility() == ESlateVisibility::HitTestInvisible
			&& ActivityIcon && ActivityIcon->IsA(UImage::StaticClass())
			&& ActivityIcon->GetVisibility() == ESlateVisibility::HitTestInvisible
			&& IndentSpacer && IndentSpacer->IsA(USizeBox::StaticClass())
			&& IndentSpacer->GetVisibility() == ESlateVisibility::HitTestInvisible
			&& ActivityText && ActivityText->IsA(UTextBlock::StaticClass())
			&& ActivityText->GetVisibility() == ESlateVisibility::HitTestInvisible;
		if (!bValid && bLogErrors)
		{
			UE_LOG(LogTemp, Error, TEXT("[CombatActivityUIBuilder] Row contract mismatch: %s"),
				*Blueprint.GetPathName());
		}
		return bValid;
	}

	bool ValidateFeedBlueprint(const UWidgetBlueprint& Blueprint,
		const UClass* ExpectedRowClass, const UWacomBattleCombatActivityStyle* ExpectedStyle,
		bool bLogErrors)
	{
		const UWidget* RootSize = Blueprint.WidgetTree
			? Blueprint.WidgetTree->FindWidget(TEXT("CombatActivitySize")) : nullptr;
		const UWidget* Rows = Blueprint.WidgetTree
			? Blueprint.WidgetTree->FindWidget(TEXT("ActivityRowsBox")) : nullptr;
		const UButton* Button = Blueprint.WidgetTree
			? Cast<UButton>(Blueprint.WidgetTree->FindWidget(TEXT("LastActionButton"))) : nullptr;
		const UBattleCombatLogFeedWidget* Defaults = Blueprint.GeneratedClass
			? Cast<UBattleCombatLogFeedWidget>(Blueprint.GeneratedClass->GetDefaultObject()) : nullptr;
		const bool bValid = Blueprint.ParentClass
			&& Blueprint.ParentClass->IsChildOf(UBattleCombatLogFeedWidget::StaticClass())
			&& Blueprint.BlueprintDescription.Contains(WidgetContractMarker)
			&& Blueprint.GeneratedClass
			&& RootSize && RootSize->IsA(USizeBox::StaticClass())
			&& RootSize->GetVisibility() == ESlateVisibility::SelfHitTestInvisible
			&& Rows && Rows->IsA(UVerticalBox::StaticClass())
			&& Rows->GetVisibility() == ESlateVisibility::HitTestInvisible
			&& Button && !Button->GetIsFocusable()
			&& Button->GetVisibility() == ESlateVisibility::Visible
			&& HasWidgetOfClass(Blueprint, TEXT("LastActionIcon"), UImage::StaticClass())
			&& HasWidgetOfClass(Blueprint, TEXT("TurnRoot"), UWidget::StaticClass())
			&& HasWidgetOfClass(Blueprint, TEXT("TurnIcon"), UImage::StaticClass())
			&& HasWidgetOfClass(Blueprint, TEXT("TurnText"), UTextBlock::StaticClass())
			&& Defaults
			&& Defaults->ActivityStyle == ExpectedStyle
			&& Defaults->ActivityRowWidgetClass.Get() == ExpectedRowClass;
		if (!bValid && bLogErrors)
		{
			UE_LOG(LogTemp, Error, TEXT("[CombatActivityUIBuilder] Feed contract mismatch: %s"),
				*Blueprint.GetPathName());
		}
		return bValid;
	}

	bool ConfigureFeedDefaults(UWidgetBlueprint& Blueprint, UClass* RowClass,
		UWacomBattleCombatActivityStyle* Style)
	{
		UBattleCombatLogFeedWidget* Defaults = Blueprint.GeneratedClass
			? Cast<UBattleCombatLogFeedWidget>(Blueprint.GeneratedClass->GetDefaultObject()) : nullptr;
		if (!Defaults || !RowClass || !Style)
		{
			return false;
		}
		bool bChanged = false;
		if (Defaults->ActivityStyle != Style)
		{
			Defaults->Modify();
			Defaults->ActivityStyle = Style;
			bChanged = true;
		}
		if (Defaults->ActivityRowWidgetClass.Get() != RowClass)
		{
			Defaults->Modify();
			Defaults->ActivityRowWidgetClass = RowClass;
			bChanged = true;
		}
		return !bChanged || SaveCompiledDefaults(Blueprint);
	}

	bool ValidateTurnDividerBlueprint(const UWidgetBlueprint& Blueprint, bool bLogErrors)
	{
		const UWidget* Root = Blueprint.WidgetTree
			? Blueprint.WidgetTree->FindWidget(TEXT("TurnDividerSize")) : nullptr;
		const UWidget* DividerRoot = Blueprint.WidgetTree
			? Blueprint.WidgetTree->FindWidget(TEXT("DividerRoot")) : nullptr;
		const UWidget* TurnIcon = Blueprint.WidgetTree
			? Blueprint.WidgetTree->FindWidget(TEXT("TurnIcon")) : nullptr;
		const UWidget* TurnText = Blueprint.WidgetTree
			? Blueprint.WidgetTree->FindWidget(TEXT("TurnText")) : nullptr;
		const bool bValid = Blueprint.ParentClass
			&& Blueprint.ParentClass->IsChildOf(UBattleCombatLogTurnDividerWidget::StaticClass())
			&& Blueprint.BlueprintDescription.Contains(WidgetContractMarker)
			&& Blueprint.GeneratedClass
			&& Root && Root->IsA(USizeBox::StaticClass())
			&& Root->GetVisibility() == ESlateVisibility::HitTestInvisible
			&& DividerRoot && DividerRoot->IsA(UBorder::StaticClass())
			&& DividerRoot->GetVisibility() == ESlateVisibility::HitTestInvisible
			&& TurnIcon && TurnIcon->IsA(UImage::StaticClass())
			&& TurnText && TurnText->IsA(UTextBlock::StaticClass());
		if (!bValid && bLogErrors)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[CombatActivityUIBuilder] Turn divider contract mismatch: %s"),
				*Blueprint.GetPathName());
		}
		return bValid;
	}

	bool ValidateDetailsBlueprint(
		const UWidgetBlueprint& Blueprint,
		const UClass* ExpectedRowClass,
		const UClass* ExpectedDividerClass,
		const UWacomBattleCombatActivityStyle* ExpectedStyle,
		bool bLogErrors)
	{
		const UWidget* FullScreen = Blueprint.WidgetTree
			? Blueprint.WidgetTree->FindWidget(TEXT("FullScreenOverlay")) : nullptr;
		const UButton* Backdrop = Blueprint.WidgetTree
			? Cast<UButton>(Blueprint.WidgetTree->FindWidget(TEXT("BackdropButton"))) : nullptr;
		const UWidget* PanelRoot = Blueprint.WidgetTree
			? Blueprint.WidgetTree->FindWidget(TEXT("PanelRoot")) : nullptr;
		const UButton* Close = Blueprint.WidgetTree
			? Cast<UButton>(Blueprint.WidgetTree->FindWidget(TEXT("CloseButton"))) : nullptr;
		const UWacomBattleCombatLogDetailsScreen* Defaults = Blueprint.GeneratedClass
			? Cast<UWacomBattleCombatLogDetailsScreen>(Blueprint.GeneratedClass->GetDefaultObject())
			: nullptr;
		const bool bValid = Blueprint.ParentClass
			&& Blueprint.ParentClass->IsChildOf(UWacomBattleCombatLogDetailsScreen::StaticClass())
			&& Blueprint.BlueprintDescription.Contains(WidgetContractMarker)
			&& Blueprint.GeneratedClass
			&& FullScreen && FullScreen->IsA(UCanvasPanel::StaticClass())
			&& FullScreen->GetVisibility() == ESlateVisibility::SelfHitTestInvisible
			&& Backdrop && !Backdrop->GetIsFocusable()
			&& Backdrop->GetVisibility() == ESlateVisibility::Visible
			&& PanelRoot && PanelRoot->IsA(UBorder::StaticClass())
			&& PanelRoot->GetVisibility() == ESlateVisibility::Visible
			&& Close && !Close->GetIsFocusable()
			&& HasWidgetOfClass(Blueprint, TEXT("DetailsToggle"), UCheckBox::StaticClass())
			&& HasWidgetOfClass(Blueprint, TEXT("HistoryScrollBox"), UScrollBox::StaticClass())
			&& HasWidgetOfClass(Blueprint, TEXT("HistoryList"), UVerticalBox::StaticClass())
			&& Defaults
			&& Defaults->GetActivityStyle() == ExpectedStyle
			&& Defaults->GetActivityRowWidgetClass() == ExpectedRowClass
			&& Defaults->GetTurnDividerWidgetClass() == ExpectedDividerClass;
		if (!bValid && bLogErrors)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[CombatActivityUIBuilder] Details screen contract mismatch: %s"),
				*Blueprint.GetPathName());
		}
		return bValid;
	}

	bool ConfigureDetailsDefaults(
		UWidgetBlueprint& Blueprint,
		UClass* RowClass,
		UClass* DividerClass,
		UWacomBattleCombatActivityStyle* Style)
	{
		UWacomBattleCombatLogDetailsScreen* Defaults = Blueprint.GeneratedClass
			? Cast<UWacomBattleCombatLogDetailsScreen>(Blueprint.GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!Defaults || !RowClass || !DividerClass || !Style)
		{
			return false;
		}
		const bool bChanged = Defaults->GetActivityStyle() != Style
			|| Defaults->GetActivityRowWidgetClass() != RowClass
			|| Defaults->GetTurnDividerWidgetClass() != DividerClass;
		if (!bChanged)
		{
			return true;
		}
		Defaults->Modify();
		Defaults->SetAuthoringDefaults(Style, RowClass, DividerClass);
		return SaveCompiledDefaults(Blueprint);
	}

	bool NearlyEqualOffsets(const FMargin& A, const FMargin& B)
	{
		return FMath::IsNearlyEqual(A.Left, B.Left, 0.01f)
			&& FMath::IsNearlyEqual(A.Top, B.Top, 0.01f)
			&& FMath::IsNearlyEqual(A.Right, B.Right, 0.01f)
			&& FMath::IsNearlyEqual(A.Bottom, B.Bottom, 0.01f);
	}

	bool ConfigureBattleHudPlacement(
		UWidgetBlueprint& BattleHud,
		UClass* ExpectedFeedClass,
		bool bBuild)
	{
		UWidget* Feed = BattleHud.WidgetTree
			? BattleHud.WidgetTree->FindWidget(TEXT("CombatLogFeed")) : nullptr;
		UCanvasPanelSlot* Slot = Feed ? Cast<UCanvasPanelSlot>(Feed->Slot) : nullptr;
		if (!Feed || !Slot || !ExpectedFeedClass
			|| !ExpectedFeedClass->IsChildOf(UBattleCombatLogFeedWidget::StaticClass())
			|| !Feed->IsA(UBattleCombatLogFeedWidget::StaticClass()))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[CombatActivityUIBuilder] BP_BattleHUD missing compatible CombatLogFeed Canvas slot"));
			return false;
		}

		bool bChanged = false;
		if (Feed->GetClass() != ExpectedFeedClass)
		{
			if (Feed->GetClass() != UBattleCombatLogFeedWidget::StaticClass())
			{
				UE_LOG(LogTemp, Error,
					TEXT("[CombatActivityUIBuilder] Unknown manual CombatLogFeed subclass; no overwrite: %s"),
					*GetNameSafe(Feed->GetClass()));
				return false;
			}
			if (!bBuild)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[CombatActivityUIBuilder] BP_BattleHUD still embeds the legacy native CombatLogFeed"));
				return false;
			}

			UCanvasPanel* Parent = Cast<UCanvasPanel>(Feed->GetParent());
			const int32 ChildIndex = Parent ? Parent->GetChildIndex(Feed) : INDEX_NONE;
			if (!Parent || ChildIndex == INDEX_NONE)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[CombatActivityUIBuilder] CombatLogFeed must remain a direct Canvas child"));
				return false;
			}

			const FAnchors SavedAnchors = Slot->GetAnchors();
			const FVector2D SavedAlignment = Slot->GetAlignment();
			const FMargin SavedOffsets = Slot->GetOffsets();
			const bool bSavedAutoSize = Slot->GetAutoSize();
			const int32 SavedZOrder = Slot->GetZOrder();

			BattleHud.Modify();
			BattleHud.WidgetTree->Modify();
			Parent->Modify();
			Parent->RemoveChild(Feed);
			BattleHud.WidgetTree->RemoveWidget(Feed);
			const FName RetiredName = MakeUniqueObjectName(
				GetTransientPackage(), Feed->GetClass(), TEXT("RetiredCombatLogFeed"));
			Feed->Rename(
				*RetiredName.ToString(),
				GetTransientPackage(),
				REN_DontCreateRedirectors | REN_DoNotDirty | REN_NonTransactional);

			UBattleCombatLogFeedWidget* FormalFeed =
				BattleHud.WidgetTree->ConstructWidget<UBattleCombatLogFeedWidget>(
					ExpectedFeedClass, TEXT("CombatLogFeed"));
			if (!FormalFeed || !Parent->InsertChildAt(ChildIndex, FormalFeed))
			{
				UE_LOG(LogTemp, Error,
					TEXT("[CombatActivityUIBuilder] Failed to install the formal CombatLogFeed WBP"));
				return false;
			}
			Slot = Cast<UCanvasPanelSlot>(FormalFeed->Slot);
			if (!Slot)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[CombatActivityUIBuilder] Formal CombatLogFeed did not receive a Canvas slot"));
				return false;
			}
			Slot->SetAnchors(SavedAnchors);
			Slot->SetAlignment(SavedAlignment);
			Slot->SetOffsets(SavedOffsets);
			Slot->SetAutoSize(bSavedAutoSize);
			Slot->SetZOrder(SavedZOrder);
			FormalFeed->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			MarkWidgetVariable(BattleHud, *FormalFeed);
			Feed = FormalFeed;
			bChanged = true;
			UE_LOG(LogTemp, Display,
				TEXT("[CombatActivityUIBuilder] Replaced legacy native CombatLogFeed with %s"),
				*GetNameSafe(ExpectedFeedClass));
		}

		const bool bPlacementManaged = BattleHud.BlueprintDescription.Contains(HudPlacementMarker);
		const FMargin Current = Slot->GetOffsets();
		const bool bFiniteAndSized = FMath::IsFinite(Current.Left) && FMath::IsFinite(Current.Top)
			&& FMath::IsFinite(Current.Right) && FMath::IsFinite(Current.Bottom)
			&& Current.Right > 0.0f && Current.Bottom > 0.0f;
		if (bPlacementManaged)
		{
			if (!bFiniteAndSized || Slot->GetAutoSize()
				|| Feed->GetVisibility() != ESlateVisibility::SelfHitTestInvisible)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[CombatActivityUIBuilder] Authored CombatLogFeed placement or hit-test contract is invalid"));
				return false;
			}
			UE_LOG(LogTemp, Display,
				TEXT("[CombatActivityUIBuilder] Preserved authored HUD placement: %.1f %.1f %.1f %.1f"),
				Current.Left, Current.Top, Current.Right, Current.Bottom);
			return !bChanged || CompileAndSave(BattleHud);
		}

		const bool bKnownLegacy = NearlyEqualOffsets(Current, FMargin(-20.0f, 0.0f, 360.0f, 190.0f))
			|| NearlyEqualOffsets(Current, FMargin(-327.2f, 20.0f, 311.2f, 252.4f))
			|| NearlyEqualOffsets(Current, FMargin(28.0f, 122.0f, 420.0f, 190.0f));
		if (!bKnownLegacy)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[CombatActivityUIBuilder] Unknown manual CombatLogFeed placement; no overwrite: %.1f %.1f %.1f %.1f"),
				Current.Left, Current.Top, Current.Right, Current.Bottom);
			return false;
		}
		if (!bBuild)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[CombatActivityUIBuilder] HUD placement has not been migrated"));
			return false;
		}

		BattleHud.Modify();
		Slot->Modify();
		Slot->SetAnchors(FAnchors(0.0f, 0.0f));
		Slot->SetAlignment(FVector2D::ZeroVector);
		Slot->SetOffsets(FMargin(28.0f, 122.0f, 420.0f, 190.0f));
		Slot->SetAutoSize(false);
		Slot->SetZOrder(7);
		Feed->Modify();
		Feed->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		BattleHud.BlueprintDescription = BattleHud.BlueprintDescription.IsEmpty()
			? FString(HudPlacementMarker)
			: BattleHud.BlueprintDescription + TEXT("\n") + HudPlacementMarker;
		return CompileAndSave(BattleHud);
	}
}

bool Wacom::ContentBuilder::ProcessCombatActivityUI(bool bBuild, bool bInspectOnly)
{
	if (bBuild == bInspectOnly)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[CombatActivityUIBuilder] Choose exactly one build or inspect mode"));
		return false;
	}

	FCombatActivityAssets Assets;
	Assets.Atlas = EnsureAtlas(bBuild);
	Assets.Style = EnsureStyle(Assets.Atlas, bBuild);
	if (!Assets.Atlas || !Assets.Style)
	{
		return false;
	}

	FWidgetBlueprintAsset RowAsset = LoadOrCreateWidgetBlueprint(
		RowAssetName, UBattleCombatActivityRowWidget::StaticClass(), bBuild);
	if (!RowAsset.Blueprint)
	{
		return false;
	}
	if (!RowAsset.bCreated
		&& !RowAsset.Blueprint->BlueprintDescription.Contains(WidgetContractMarker))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[CombatActivityUIBuilder] Unknown manual Row WBP detected; no overwrite: %s"),
			*RowAsset.Blueprint->GetPathName());
		return false;
	}
	if (!ValidateRowBlueprint(*RowAsset.Blueprint, false))
	{
		if (!bBuild || !BuildRowBlueprint(*RowAsset.Blueprint)
			|| !ValidateRowBlueprint(*RowAsset.Blueprint, true))
		{
			ValidateRowBlueprint(*RowAsset.Blueprint, true);
			return false;
		}
	}
	Assets.RowBlueprint = RowAsset.Blueprint;

	FWidgetBlueprintAsset DividerAsset = LoadOrCreateWidgetBlueprint(
		DividerAssetName, UBattleCombatLogTurnDividerWidget::StaticClass(), bBuild);
	if (!DividerAsset.Blueprint)
	{
		return false;
	}
	if (!DividerAsset.bCreated
		&& !DividerAsset.Blueprint->BlueprintDescription.Contains(WidgetContractMarker))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[CombatActivityUIBuilder] Unknown manual Divider WBP detected; no overwrite: %s"),
			*DividerAsset.Blueprint->GetPathName());
		return false;
	}
	if (!ValidateTurnDividerBlueprint(*DividerAsset.Blueprint, false))
	{
		if (!bBuild || !BuildTurnDividerBlueprint(*DividerAsset.Blueprint)
			|| !ValidateTurnDividerBlueprint(*DividerAsset.Blueprint, true))
		{
			ValidateTurnDividerBlueprint(*DividerAsset.Blueprint, true);
			return false;
		}
	}
	Assets.DividerBlueprint = DividerAsset.Blueprint;

	FWidgetBlueprintAsset FeedAsset = LoadOrCreateWidgetBlueprint(
		FeedAssetName, UBattleCombatLogFeedWidget::StaticClass(), bBuild);
	if (!FeedAsset.Blueprint)
	{
		return false;
	}
	if (!FeedAsset.bCreated
		&& !FeedAsset.Blueprint->BlueprintDescription.Contains(WidgetContractMarker)
		&& !IsRecognizedLegacyFeed(*FeedAsset.Blueprint))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[CombatActivityUIBuilder] Unknown manual Feed WBP detected; no overwrite: %s"),
			*FeedAsset.Blueprint->GetPathName());
		return false;
	}
	if (!ValidateFeedBlueprint(*FeedAsset.Blueprint,
		RowAsset.Blueprint->GeneratedClass, Assets.Style, false))
	{
		if (!bBuild || !BuildFeedBlueprint(*FeedAsset.Blueprint)
			|| !ConfigureFeedDefaults(*FeedAsset.Blueprint,
				RowAsset.Blueprint->GeneratedClass, Assets.Style)
			|| !ValidateFeedBlueprint(*FeedAsset.Blueprint,
				RowAsset.Blueprint->GeneratedClass, Assets.Style, true))
		{
			ValidateFeedBlueprint(*FeedAsset.Blueprint,
				RowAsset.Blueprint->GeneratedClass, Assets.Style, true);
			return false;
		}
	}
	Assets.FeedBlueprint = FeedAsset.Blueprint;

	FWidgetBlueprintAsset DetailsAsset = LoadOrCreateWidgetBlueprint(
		DetailsAssetName, UWacomBattleCombatLogDetailsScreen::StaticClass(), bBuild);
	if (!DetailsAsset.Blueprint)
	{
		return false;
	}
	if (!DetailsAsset.bCreated
		&& !DetailsAsset.Blueprint->BlueprintDescription.Contains(WidgetContractMarker))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[CombatActivityUIBuilder] Unknown manual Details WBP detected; no overwrite: %s"),
			*DetailsAsset.Blueprint->GetPathName());
		return false;
	}
	if (!ValidateDetailsBlueprint(
		*DetailsAsset.Blueprint,
		RowAsset.Blueprint->GeneratedClass,
		DividerAsset.Blueprint->GeneratedClass,
		Assets.Style,
		false))
	{
		if (!bBuild || !BuildDetailsBlueprint(*DetailsAsset.Blueprint)
			|| !ConfigureDetailsDefaults(
				*DetailsAsset.Blueprint,
				RowAsset.Blueprint->GeneratedClass,
				DividerAsset.Blueprint->GeneratedClass,
				Assets.Style)
			|| !ValidateDetailsBlueprint(
				*DetailsAsset.Blueprint,
				RowAsset.Blueprint->GeneratedClass,
				DividerAsset.Blueprint->GeneratedClass,
				Assets.Style,
				true))
		{
			ValidateDetailsBlueprint(
				*DetailsAsset.Blueprint,
				RowAsset.Blueprint->GeneratedClass,
				DividerAsset.Blueprint->GeneratedClass,
				Assets.Style,
				true);
			return false;
		}
	}
	Assets.DetailsBlueprint = DetailsAsset.Blueprint;

	UWidgetBlueprint* BattleHud = Cast<UWidgetBlueprint>(StaticLoadObject(
		UWidgetBlueprint::StaticClass(), nullptr, BattleHudObjectPath));
	if (!BattleHud || !ConfigureBattleHudPlacement(
		*BattleHud, FeedAsset.Blueprint->GeneratedClass, bBuild))
	{
		return false;
	}

	UE_LOG(LogTemp, Display,
		TEXT("[CombatActivityUIBuilder] Contract ready%s: Feed=%s Row=%s Details=%s Divider=%s Style=%s Atlas=%s"),
		bInspectOnly ? TEXT(" (inspect only)") : TEXT(""),
		*GetNameSafe(Assets.FeedBlueprint), *GetNameSafe(Assets.RowBlueprint),
		*GetNameSafe(Assets.DetailsBlueprint), *GetNameSafe(Assets.DividerBlueprint),
		*GetNameSafe(Assets.Style), *GetNameSafe(Assets.Atlas));
	return true;
}
