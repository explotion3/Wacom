// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/EnemySinglePartUIBuilder.h"

#include "ContentBuilders/ContentBuilderHelpers.h"

#include "Animation/WidgetAnimation.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Channels/MovieSceneFloatChannel.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "MovieScene.h"
#include "Sections/MovieSceneFloatSection.h"
#include "Styling/SlateTypes.h"
#include "Tracks/MovieSceneFloatTrack.h"
#include "UI/Battle/WacomBattleEnemyIntentPresentationStyle.h"
#include "UI/Battle/WacomBattleEnemyPanelWidget.h"
#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"
#include "UI/Battle/WacomBattleStatusIconWidget.h"
#include "UObject/MetaData.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"

namespace
{
	using namespace Wacom::ContentBuilder;

	constexpr TCHAR AssetRoot[] = TEXT("/Game/Wacom/UI/Enemy");
	constexpr TCHAR IntentRoot[] = TEXT("/Game/Wacom/UI/Enemy/Intent");
	constexpr TCHAR IntentTextureRoot[] = TEXT("/Game/Wacom/UI/Enemy/Intent/Textures");
	constexpr TCHAR PanelAssetName[] = TEXT("WBP_WacomBattleEnemySinglePartPanelWidget");
	constexpr TCHAR EntryAssetName[] = TEXT("WBP_WacomBattleEnemySinglePartEntryWidget");
	constexpr TCHAR StyleAssetName[] = TEXT("DA_EnemyIntentPresentation_Default");
	constexpr TCHAR FallbackTextureName[] = TEXT("T_UI_EnemyIntent_Fallback");
	constexpr TCHAR AttackTextureName[] = TEXT("T_UI_EnemyIntent_Attack");
	constexpr TCHAR GuardTextureName[] = TEXT("T_UI_EnemyIntent_Guard");
	constexpr TCHAR CleaveTextureName[] = TEXT("T_UI_EnemyIntent_Cleave");
	constexpr TCHAR StatusListClassPath[] =
		TEXT("/Game/Wacom/UI/Battle/PlayerStatusBar/WBP_BattleStatusIconList.WBP_BattleStatusIconList_C");
	constexpr TCHAR WidgetContractMarker[] =
		TEXT("WacomEnemySinglePartWBP.ContractVersion=1");
	constexpr TCHAR ContentContractKey[] = TEXT("WacomContract");
	constexpr TCHAR ContentContractValue[] =
		TEXT("WacomEnemySinglePartUI.ContractVersion=1");
	constexpr int32 IconSize = 24;

	const FLinearColor Paper(0.96f, 0.97f, 1.0f, 1.0f);
	const FLinearColor Muted(0.62f, 0.67f, 0.76f, 1.0f);
	const FLinearColor Ink(0.018f, 0.025f, 0.045f, 0.88f);
	const FLinearColor TrackColor(0.055f, 0.075f, 0.105f, 0.94f);
	const FLinearColor HpFill(0.94f, 0.12f, 0.34f, 1.0f);
	const FLinearColor ShieldFill(0.57f, 0.40f, 0.96f, 1.0f);
	const FLinearColor Cyan(0.28f, 0.88f, 0.94f, 1.0f);
	const FLinearColor Amber(1.0f, 0.72f, 0.25f, 1.0f);

	enum class EIntentIconKind : uint8
	{
		Fallback,
		Attack,
		Guard,
		Cleave,
	};

	struct FIntentAssets
	{
		UTexture2D* Fallback = nullptr;
		UTexture2D* Attack = nullptr;
		UTexture2D* Guard = nullptr;
		UTexture2D* Cleave = nullptr;
		UWacomBattleEnemyIntentPresentationStyle* Style = nullptr;
	};

	struct FWidgetBlueprintAsset
	{
		UWidgetBlueprint* Blueprint = nullptr;
		FString PackagePath;
		bool bCreated = false;
	};

	void SetPixel(TArray<FColor>& Pixels, int32 X, int32 Y, int32 Radius = 0)
	{
		for (int32 OffsetY = -Radius; OffsetY <= Radius; ++OffsetY)
		{
			for (int32 OffsetX = -Radius; OffsetX <= Radius; ++OffsetX)
			{
				const int32 PixelX = X + OffsetX;
				const int32 PixelY = Y + OffsetY;
				if (PixelX >= 0 && PixelX < IconSize && PixelY >= 0 && PixelY < IconSize)
				{
					Pixels[PixelY * IconSize + PixelX] = FColor::White;
				}
			}
		}
	}

	void DrawLine(TArray<FColor>& Pixels, int32 X0, int32 Y0, int32 X1, int32 Y1,
		int32 Radius = 0)
	{
		const int32 DeltaX = FMath::Abs(X1 - X0);
		const int32 StepX = X0 < X1 ? 1 : -1;
		const int32 DeltaY = -FMath::Abs(Y1 - Y0);
		const int32 StepY = Y0 < Y1 ? 1 : -1;
		int32 Error = DeltaX + DeltaY;
		while (true)
		{
			SetPixel(Pixels, X0, Y0, Radius);
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

	TArray<FColor> BuildIconPixels(EIntentIconKind Kind)
	{
		TArray<FColor> Pixels;
		Pixels.Init(FColor(0, 0, 0, 0), IconSize * IconSize);
		switch (Kind)
		{
		case EIntentIconKind::Fallback:
			for (int32 Offset = -9; Offset <= 9; ++Offset)
			{
				const int32 HalfWidth = FMath::Max(0, 3 - FMath::Abs(Offset) / 3);
				for (int32 Cross = -HalfWidth; Cross <= HalfWidth; ++Cross)
				{
					SetPixel(Pixels, 12 + Cross, 12 + Offset);
					SetPixel(Pixels, 12 + Offset, 12 + Cross);
				}
			}
			break;
		case EIntentIconKind::Attack:
			DrawLine(Pixels, 6, 18, 17, 7, 1);
			DrawLine(Pixels, 14, 5, 19, 5, 0);
			DrawLine(Pixels, 19, 5, 19, 10, 0);
			DrawLine(Pixels, 5, 15, 9, 19, 1);
			DrawLine(Pixels, 4, 20, 7, 17, 1);
			break;
		case EIntentIconKind::Guard:
			DrawLine(Pixels, 5, 5, 18, 5, 1);
			DrawLine(Pixels, 5, 5, 6, 14, 1);
			DrawLine(Pixels, 18, 5, 17, 14, 1);
			DrawLine(Pixels, 6, 14, 12, 20, 1);
			DrawLine(Pixels, 17, 14, 12, 20, 1);
			DrawLine(Pixels, 9, 8, 15, 8, 0);
			break;
		case EIntentIconKind::Cleave:
			DrawLine(Pixels, 3, 15, 20, 6, 1);
			DrawLine(Pixels, 5, 19, 21, 11, 1);
			DrawLine(Pixels, 3, 15, 6, 19, 1);
			SetPixel(Pixels, 19, 5, 1);
			break;
		}
		return Pixels;
	}

	bool HasManagedContentMarker(const UObject* Object)
	{
		if (!Object || !Object->GetOutermost())
		{
			return false;
		}
		FMetaData& MetaData = Object->GetOutermost()->GetMetaData();
		return MetaData.GetValue(Object, ContentContractKey) == ContentContractValue;
	}

	void SetManagedContentMarker(UObject* Object)
	{
		check(Object && Object->GetOutermost());
		Object->GetOutermost()->GetMetaData().SetValue(
			Object, ContentContractKey, ContentContractValue);
	}

	bool IsTextureValid(const UTexture2D* Texture)
	{
		return Texture
			&& HasManagedContentMarker(Texture)
			&& Texture->Source.GetSizeX() == IconSize
			&& Texture->Source.GetSizeY() == IconSize
			&& Texture->Source.GetFormat() == TSF_BGRA8
			&& Texture->Filter == TF_Nearest
			&& Texture->MipGenSettings == TMGS_NoMipmaps;
	}

	UTexture2D* EnsureIconTexture(
		const TCHAR* AssetName,
		EIntentIconKind Kind,
		bool bBuild)
	{
		const FString PackagePath = MakePackagePath(IntentTextureRoot, AssetName);
		const FString ObjectPath = MakeObjectPath(PackagePath);
		UObject* Existing = StaticLoadObject(UObject::StaticClass(), nullptr, *ObjectPath);
		UTexture2D* Texture = Cast<UTexture2D>(Existing);
		if (Existing && !Texture)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[EnemySinglePartUIBuilder] Existing icon is not UTexture2D: %s"),
				*ObjectPath);
			return nullptr;
		}
		if (Texture && !HasManagedContentMarker(Texture))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[EnemySinglePartUIBuilder] Unknown manual icon asset detected; no overwrite: %s"),
				*ObjectPath);
			return nullptr;
		}
		if (IsTextureValid(Texture))
		{
			return Texture;
		}
		if (!bBuild)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[EnemySinglePartUIBuilder] Missing or invalid managed icon: %s"),
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
			Texture = NewObject<UTexture2D>(Package, AssetName, RF_Public | RF_Standalone);
		}
		Texture->Modify();
		const TArray<FColor> Pixels = BuildIconPixels(Kind);
		Texture->Source.Init(
			IconSize, IconSize, 1, 1, TSF_BGRA8,
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

	FSlateBrush MakeIconBrush(UTexture2D* Texture)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.ImageSize = FVector2D(IconSize, IconSize);
		Brush.TintColor = FSlateColor(FLinearColor::White);
		return Brush;
	}

	bool IsStyleValid(
		const UWacomBattleEnemyIntentPresentationStyle* Style,
		const FIntentAssets& Assets)
	{
		if (!Style || !HasManagedContentMarker(Style)
			|| Style->FallbackIconBrush.GetResourceObject() != Assets.Fallback
			|| Style->IntentIcons.Num() != 3)
		{
			return false;
		}
		const TPair<FName, const UTexture2D*> Expected[] = {
			{ TEXT("TrainingWarrior.Body.Attack"), Assets.Attack },
			{ TEXT("TrainingWarrior.Body.Guard"), Assets.Guard },
			{ TEXT("TrainingWarrior.Body.Cleave"), Assets.Cleave },
		};
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Expected); ++Index)
		{
			if (Style->IntentIcons[Index].IntentId != Expected[Index].Key
				|| Style->IntentIcons[Index].IconBrush.GetResourceObject()
					!= Expected[Index].Value)
			{
				return false;
			}
		}
		return true;
	}

	UWacomBattleEnemyIntentPresentationStyle* EnsureIntentStyle(
		FIntentAssets& Assets,
		bool bBuild)
	{
		const FString PackagePath = MakePackagePath(IntentRoot, StyleAssetName);
		const FString ObjectPath = MakeObjectPath(PackagePath);
		UObject* Existing = StaticLoadObject(UObject::StaticClass(), nullptr, *ObjectPath);
		UWacomBattleEnemyIntentPresentationStyle* Style =
			Cast<UWacomBattleEnemyIntentPresentationStyle>(Existing);
		if (Existing && !Style)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[EnemySinglePartUIBuilder] Existing style has wrong class: %s"),
				*ObjectPath);
			return nullptr;
		}
		if (Style && !HasManagedContentMarker(Style))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[EnemySinglePartUIBuilder] Unknown manual style detected; no overwrite: %s"),
				*ObjectPath);
			return nullptr;
		}
		if (IsStyleValid(Style, Assets))
		{
			return Style;
		}
		if (!bBuild)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[EnemySinglePartUIBuilder] Missing or invalid compact intent style: %s"),
				*ObjectPath);
			return nullptr;
		}

		UPackage* Package = Style ? Style->GetOutermost() : FindOrCreatePackage(PackagePath);
		if (!Package)
		{
			return nullptr;
		}
		if (!Style)
		{
			Style = NewObject<UWacomBattleEnemyIntentPresentationStyle>(
				Package, StyleAssetName, RF_Public | RF_Standalone);
		}
		Style->Modify();
		Style->FallbackIconBrush = MakeIconBrush(Assets.Fallback);
		Style->IntentIcons.Reset(3);
		auto AddIntent = [Style](const FName IntentId, UTexture2D* Texture)
		{
			FWacomBattleEnemyIntentIconEntry& Entry = Style->IntentIcons.AddDefaulted_GetRef();
			Entry.IntentId = IntentId;
			Entry.IconBrush = MakeIconBrush(Texture);
		};
		AddIntent(TEXT("TrainingWarrior.Body.Attack"), Assets.Attack);
		AddIntent(TEXT("TrainingWarrior.Body.Guard"), Assets.Guard);
		AddIntent(TEXT("TrainingWarrior.Body.Cleave"), Assets.Cleave);
		SetManagedContentMarker(Style);
		return SaveAssetPackage(Package, Style, PackagePath) ? Style : nullptr;
	}

	FWidgetBlueprintAsset LoadOrCreateWidgetBlueprint(
		const TCHAR* AssetName,
		UClass* ParentClass,
		bool bAllowCreate)
	{
		FWidgetBlueprintAsset Result;
		Result.PackagePath = MakePackagePath(AssetRoot, AssetName);
		const FString ObjectPath = MakeObjectPath(Result.PackagePath);
		if (UObject* Existing = StaticLoadObject(UObject::StaticClass(), nullptr, *ObjectPath))
		{
			Result.Blueprint = Cast<UWidgetBlueprint>(Existing);
			if (!Result.Blueprint
				|| !Result.Blueprint->ParentClass
				|| !Result.Blueprint->ParentClass->IsChildOf(ParentClass))
			{
				UE_LOG(LogTemp, Error,
					TEXT("[EnemySinglePartUIBuilder] Existing WBP has incompatible class: %s"),
					*ObjectPath);
				Result.Blueprint = nullptr;
			}
			return Result;
		}

		if (!bAllowCreate)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[EnemySinglePartUIBuilder] Missing compact WBP: %s"),
				*ObjectPath);
			return Result;
		}

		UPackage* Package = FindOrCreatePackage(Result.PackagePath);
		if (!Package)
		{
			return Result;
		}
		Result.Blueprint = Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(
			ParentClass,
			Package,
			AssetName,
			BPTYPE_Normal,
			UWidgetBlueprint::StaticClass(),
			UWidgetBlueprintGeneratedClass::StaticClass()));
		Result.bCreated = Result.Blueprint != nullptr;
		return Result;
	}

	void ResetWidgetBlueprint(UWidgetBlueprint* Blueprint, const FString& Description)
	{
		check(Blueprint);
		Blueprint->Modify();
		if (UWidgetTree* PreviousTree = Blueprint->WidgetTree)
		{
			const FName PreviousTreeName = MakeUniqueObjectName(
				GetTransientPackage(),
				UWidgetTree::StaticClass(),
				*FString::Printf(TEXT("%s_PreviousTree"), *Blueprint->GetName()));
			PreviousTree->Rename(
				*PreviousTreeName.ToString(),
				GetTransientPackage(),
				REN_DontCreateRedirectors | REN_NonTransactional);
		}
		for (UWidgetAnimation* Animation : Blueprint->Animations)
		{
			if (Animation)
			{
				Animation->Rename(nullptr, GetTransientPackage(),
					REN_DontCreateRedirectors | REN_NonTransactional);
			}
		}
		Blueprint->WidgetTree = NewObject<UWidgetTree>(
			Blueprint, TEXT("WidgetTree"), RF_Transactional);
		Blueprint->Bindings.Reset();
		Blueprint->Animations.Reset();
		Blueprint->WidgetVariableNameToGuidMap.Reset();
		Blueprint->BlueprintDescription = Description + TEXT("\n") + WidgetContractMarker;
		Blueprint->bCanCallInitializedWithoutPlayerContext = true;
	}

	void RegisterWidgetGuid(UWidgetBlueprint* Blueprint, const UWidget* Widget)
	{
		check(Blueprint && Widget);
		const FString StablePath = FString::Printf(
			TEXT("%s:%s"), *Blueprint->GetPathName(), *Widget->GetName());
		Blueprint->WidgetVariableNameToGuidMap.FindOrAdd(Widget->GetFName()) =
			FGuid::NewDeterministicGuid(StablePath);
	}

	void MarkWidgetVariable(UWidgetBlueprint* Blueprint, UWidget* Widget)
	{
		check(Blueprint && Widget);
		Widget->bIsVariable = true;
		RegisterWidgetGuid(Blueprint, Widget);
	}

	void MakeTreeNonHitTestable(UWidgetBlueprint* Blueprint)
	{
		TArray<UWidget*> Widgets;
		Blueprint->WidgetTree->GetAllWidgets(Widgets);
		for (UWidget* Widget : Widgets)
		{
			if (!Widget)
			{
				continue;
			}
			RegisterWidgetGuid(Blueprint, Widget);
			if (Widget->GetVisibility() == ESlateVisibility::Visible
				|| Widget->GetVisibility() == ESlateVisibility::SelfHitTestInvisible)
			{
				Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
		}
	}

	void StyleText(
		UTextBlock* Text,
		const FText& Value,
		int32 FontSize,
		const FLinearColor& Color,
		FName Typeface = TEXT("Regular"))
	{
		check(Text);
		Text->SetText(Value);
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = FontSize;
		Font.TypefaceFontName = Typeface;
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetShadowOffset(FVector2D(1.0f, 1.0f));
		Text->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.92f));
	}

	UCanvasPanelSlot* PlaceOnCanvas(
		UCanvasPanel* Canvas,
		UWidget* Widget,
		const FVector2D& Position,
		const FVector2D& Size)
	{
		check(Canvas && Widget);
		UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Widget);
		Slot->SetPosition(Position);
		Slot->SetSize(Size);
		Slot->SetAlignment(FVector2D::ZeroVector);
		return Slot;
	}

	UBorder* AddOverlaySurface(
		UWidgetTree* Tree,
		UOverlay* Overlay,
		const FName Name,
		const FLinearColor& Color,
		ESlateVisibility Visibility,
		float Opacity = 1.0f)
	{
		UBorder* Surface = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
		Surface->SetBrushColor(Color);
		Surface->SetVisibility(Visibility);
		Surface->SetRenderOpacity(Opacity);
		if (UOverlaySlot* Slot = Overlay->AddChildToOverlay(Surface))
		{
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
		}
		return Surface;
	}

	void ConfigureProgressBar(
		UProgressBar* Bar,
		const FLinearColor& FillColor,
		float Percent)
	{
		check(Bar);
		FSlateBrush Background;
		Background.DrawAs = ESlateBrushDrawType::Box;
		Background.TintColor = FSlateColor(TrackColor);
		Background.ImageSize = FVector2D(16.0f, 6.0f);
		FSlateBrush Fill = Background;
		Fill.TintColor = FSlateColor(FillColor);
		FProgressBarStyle Style;
		Style.SetBackgroundImage(Background);
		Style.SetFillImage(Fill);
		Style.SetMarqueeImage(Fill);
		Bar->SetWidgetStyle(Style);
		Bar->SetPercent(Percent);
		Bar->SetBarFillType(EProgressBarFillType::LeftToRight);
		FWidgetTransform SlantedTransform;
		SlantedTransform.Shear = FVector2D(-12.0f, 0.0f);
		Bar->SetRenderTransform(SlantedTransform);
		Bar->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	}

	UWidgetAnimation* AddOpacityAnimation(
		UWidgetBlueprint* Blueprint,
		const FName AnimationName,
		UWidget* Target,
		const TArray<TPair<float, float>>& Keys)
	{
		check(Blueprint && Target && !Keys.IsEmpty());
		UWidgetAnimation* Animation = NewObject<UWidgetAnimation>(
			Blueprint, AnimationName, RF_Transactional);
		Animation->SetDisplayLabel(AnimationName.ToString());
		const FString StableAnimationPath = FString::Printf(
			TEXT("%s:Animation:%s"),
			*Blueprint->GetPathName(),
			*AnimationName.ToString());
		Blueprint->WidgetVariableNameToGuidMap.FindOrAdd(AnimationName) =
			FGuid::NewDeterministicGuid(StableAnimationPath);
		Animation->MovieScene = NewObject<UMovieScene>(
			Animation, AnimationName, RF_Transactional);
		Animation->MovieScene->SetDisplayRate(FFrameRate(30, 1));
		Animation->MovieScene->SetTickResolutionDirectly(FFrameRate(30, 1));
		const FFrameNumber EndFrame(
			FMath::Max(1, FMath::RoundToInt(Keys.Last().Key * 30.0f)));
		Animation->MovieScene->SetPlaybackRange(
			TRange<FFrameNumber>(FFrameNumber(0), EndFrame + 1));
		Animation->MovieScene->GetEditorData().WorkStart = 0.0;
		Animation->MovieScene->GetEditorData().WorkEnd = Keys.Last().Key;

		const FGuid BindingGuid = Animation->MovieScene->AddPossessable(
			Target->GetName(), Target->GetClass());
		Animation->MovieScene->SetObjectDisplayName(
			BindingGuid, FText::FromName(Target->GetFName()));
		FWidgetAnimationBinding Binding;
		Binding.AnimationGuid = BindingGuid;
		Binding.WidgetName = Target->GetFName();
		Animation->AnimationBindings.Add(Binding);

		UMovieSceneFloatTrack* Track =
			Animation->MovieScene->AddTrack<UMovieSceneFloatTrack>(BindingGuid);
		if (!Track)
		{
			return nullptr;
		}
		Track->SetPropertyNameAndPath(TEXT("RenderOpacity"), TEXT("RenderOpacity"));
		UMovieSceneFloatSection* Section =
			Cast<UMovieSceneFloatSection>(Track->CreateNewSection());
		if (!Section)
		{
			return nullptr;
		}
		Section->SetRange(TRange<FFrameNumber>(FFrameNumber(0), EndFrame + 1));
		for (const TPair<float, float>& Key : Keys)
		{
			Section->GetChannel().AddLinearKey(
				FFrameNumber(FMath::RoundToInt(Key.Key * 30.0f)), Key.Value);
		}
		Track->AddSection(*Section);
		Blueprint->Animations.Add(Animation);
		return Animation;
	}

	bool SaveCompiledDefaults(UWidgetBlueprint* Blueprint)
	{
		check(Blueprint && Blueprint->GeneratedClass);
		UPackage* Package = Blueprint->GetOutermost();
		Package->MarkPackageDirty();
		Blueprint->GeneratedClass->GetDefaultObject()->MarkPackageDirty();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, Blueprint, *Filename, Args);
	}

	bool CompileAndSave(UWidgetBlueprint* Blueprint)
	{
		check(Blueprint && Blueprint->WidgetTree);
		MakeTreeNonHitTestable(Blueprint);
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		if (Blueprint->Status == BS_Error || !Blueprint->GeneratedClass)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[EnemySinglePartUIBuilder] Compile failed: %s"),
				*Blueprint->GetPathName());
			return false;
		}

		FAssetRegistryModule::AssetCreated(Blueprint);
		UPackage* Package = Blueprint->GetOutermost();
		Package->MarkPackageDirty();
		Blueprint->MarkPackageDirty();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, Blueprint, *Filename, Args);
	}

	bool BuildEntryBlueprint(
		UWidgetBlueprint* Blueprint,
		UWacomBattleEnemyIntentPresentationStyle* IntentStyle)
	{
		UClass* StatusListClass = LoadClass<UWacomBattleStatusIconListWidget>(
			nullptr, StatusListClassPath);
		if (!StatusListClass || !IntentStyle)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[EnemySinglePartUIBuilder] Missing status list class or Intent Style"));
			return false;
		}

		ResetWidgetBlueprint(
			Blueprint,
			TEXT("单部位 Scene Enemy 的紧凑被动条目。只消费 ViewData；布局、图标和动画均为 UI 表现。"));
		UWidgetTree* Tree = Blueprint->WidgetTree;
		USizeBox* Root = Tree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("SinglePartEntryRoot"));
		Root->SetMinDesiredWidth(250.0f);
		Tree->RootWidget = Root;

		UVerticalBox* Rows = Tree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("SinglePartRows"));
		Root->AddChild(Rows);
		USizeBox* CompactSize = Tree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("CompactSize"));
		CompactSize->SetWidthOverride(250.0f);
		CompactSize->SetHeightOverride(84.0f);
		Rows->AddChildToVerticalBox(CompactSize);

		UOverlay* Overlay = Tree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), TEXT("CompactOverlay"));
		CompactSize->AddChild(Overlay);
		UBorder* EntryBackground = AddOverlaySurface(
			Tree, Overlay, TEXT("EntryBackground"),
			FLinearColor(0.0f, 0.0f, 0.0f, 0.08f),
			ESlateVisibility::HitTestInvisible);
		UBorder* ContextHighlight = AddOverlaySurface(
			Tree, Overlay, TEXT("ContextHighlight"),
			FLinearColor(Cyan.R, Cyan.G, Cyan.B, 0.16f),
			ESlateVisibility::Collapsed, 0.0f);
		UBorder* ActionPreviewOverlay = AddOverlaySurface(
			Tree, Overlay, TEXT("ActionPreviewOverlay"),
			FLinearColor(Amber.R, Amber.G, Amber.B, 0.12f),
			ESlateVisibility::Collapsed);
		UBorder* DamagePulseSurface = AddOverlaySurface(
			Tree, Overlay, TEXT("DamagePulseSurface"),
			FLinearColor(HpFill.R, HpFill.G, HpFill.B, 0.42f),
			ESlateVisibility::HitTestInvisible, 0.0f);
		UBorder* ShieldPulseSurface = AddOverlaySurface(
			Tree, Overlay, TEXT("ShieldPulseSurface"),
			FLinearColor(ShieldFill.R, ShieldFill.G, ShieldFill.B, 0.42f),
			ESlateVisibility::HitTestInvisible, 0.0f);
		UBorder* DestroyedOverlay = AddOverlaySurface(
			Tree, Overlay, TEXT("DestroyedOverlay"),
			FLinearColor(0.02f, 0.025f, 0.04f, 0.58f),
			ESlateVisibility::Collapsed, 0.62f);
		MarkWidgetVariable(Blueprint, ContextHighlight);
		MarkWidgetVariable(Blueprint, ActionPreviewOverlay);
		MarkWidgetVariable(Blueprint, DestroyedOverlay);

		UCanvasPanel* Canvas = Tree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("EntryContent"));
		if (UOverlaySlot* Slot = Overlay->AddChildToOverlay(Canvas))
		{
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
		}

		UBorder* InitiativeDiamond = Tree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("InitiativeDiamond"));
		InitiativeDiamond->SetBrushColor(FLinearColor(0.66f, 0.02f, 0.22f, 0.96f));
		InitiativeDiamond->SetRenderTransformAngle(45.0f);
		InitiativeDiamond->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		PlaceOnCanvas(Canvas, InitiativeDiamond, FVector2D(6.0f, 4.0f), FVector2D(42.0f, 42.0f));
		MarkWidgetVariable(Blueprint, InitiativeDiamond);

		UTextBlock* InitiativeText = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("InitiativeText"));
		StyleText(InitiativeText, FText::AsNumber(1), 19, Paper, TEXT("Bold"));
		InitiativeText->SetJustification(ETextJustify::Center);
		PlaceOnCanvas(Canvas, InitiativeText, FVector2D(6.0f, 11.0f), FVector2D(42.0f, 28.0f));
		MarkWidgetVariable(Blueprint, InitiativeText);

		UBorder* IntentDiamond = Tree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("IntentDiamond"));
		IntentDiamond->SetBrushColor(Ink);
		IntentDiamond->SetRenderTransformAngle(45.0f);
		IntentDiamond->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		PlaceOnCanvas(Canvas, IntentDiamond, FVector2D(13.0f, 46.0f), FVector2D(31.0f, 31.0f));
		MarkWidgetVariable(Blueprint, IntentDiamond);

		UImage* IntentIcon = Tree->ConstructWidget<UImage>(
			UImage::StaticClass(), TEXT("IntentIcon"));
		IntentIcon->SetBrush(IntentStyle->FallbackIconBrush);
		PlaceOnCanvas(Canvas, IntentIcon, FVector2D(17.0f, 50.0f), FVector2D(23.0f, 23.0f));
		MarkWidgetVariable(Blueprint, IntentIcon);

		UTextBlock* DestroyedMark = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("DestroyedMark"));
		StyleText(DestroyedMark, FText::FromString(TEXT("X")), 20, Paper, TEXT("Bold"));
		DestroyedMark->SetJustification(ETextJustify::Center);
		DestroyedMark->SetVisibility(ESlateVisibility::Collapsed);
		PlaceOnCanvas(Canvas, DestroyedMark, FVector2D(6.0f, 11.0f), FVector2D(42.0f, 28.0f));
		MarkWidgetVariable(Blueprint, DestroyedMark);

		UProgressBar* HpBar = Tree->ConstructWidget<UProgressBar>(
			UProgressBar::StaticClass(), TEXT("HpBar"));
		ConfigureProgressBar(HpBar, HpFill, 0.75f);
		PlaceOnCanvas(Canvas, HpBar, FVector2D(58.0f, 20.0f), FVector2D(184.0f, 13.0f));
		MarkWidgetVariable(Blueprint, HpBar);

		UTextBlock* HpText = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("HpText"));
		StyleText(HpText, FText::AsNumber(18), 15, Paper, TEXT("Bold"));
		HpText->SetJustification(ETextJustify::Center);
		PlaceOnCanvas(Canvas, HpText, FVector2D(112.0f, 0.0f), FVector2D(72.0f, 24.0f));
		MarkWidgetVariable(Blueprint, HpText);

		UOverlay* ShieldContainer = Tree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), TEXT("ShieldContainer"));
		ShieldContainer->SetVisibility(ESlateVisibility::Collapsed);
		PlaceOnCanvas(Canvas, ShieldContainer, FVector2D(58.0f, 37.0f), FVector2D(184.0f, 12.0f));
		MarkWidgetVariable(Blueprint, ShieldContainer);
		UProgressBar* ShieldBar = Tree->ConstructWidget<UProgressBar>(
			UProgressBar::StaticClass(), TEXT("ShieldBar"));
		ConfigureProgressBar(ShieldBar, ShieldFill, 0.25f);
		ShieldContainer->AddChildToOverlay(ShieldBar);
		MarkWidgetVariable(Blueprint, ShieldBar);
		UTextBlock* ShieldText = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("ShieldText"));
		StyleText(ShieldText, FText::AsNumber(4), 11, Paper, TEXT("Bold"));
		ShieldText->SetJustification(ETextJustify::Center);
		ShieldContainer->AddChildToOverlay(ShieldText);
		MarkWidgetVariable(Blueprint, ShieldText);

		UWacomBattleStatusIconListWidget* StatusList =
			Tree->ConstructWidget<UWacomBattleStatusIconListWidget>(
				StatusListClass, TEXT("StatusList"));
		StatusList->SetVisibility(ESlateVisibility::Collapsed);
		PlaceOnCanvas(Canvas, StatusList, FVector2D(58.0f, 54.0f), FVector2D(184.0f, 25.0f));
		MarkWidgetVariable(Blueprint, StatusList);

		UHorizontalBox* Details = Tree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("DetailsContainer"));
		Details->SetVisibility(ESlateVisibility::Collapsed);
		if (UVerticalBoxSlot* Slot = Rows->AddChildToVerticalBox(Details))
		{
			Slot->SetPadding(FMargin(54.0f, 2.0f, 8.0f, 4.0f));
		}
		MarkWidgetVariable(Blueprint, Details);
		UTextBlock* PartNameText = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("PartNameText"));
		StyleText(PartNameText, FText::FromString(TEXT("身体")), 11, Muted, TEXT("Bold"));
		Details->AddChildToHorizontalBox(PartNameText)->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
		MarkWidgetVariable(Blueprint, PartNameText);
		UTextBlock* IntentText = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("IntentText"));
		StyleText(IntentText, FText::FromString(TEXT("攻击  3")), 11, Paper);
		if (UHorizontalBoxSlot* Slot = Details->AddChildToHorizontalBox(IntentText))
		{
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		MarkWidgetVariable(Blueprint, IntentText);
		UTextBlock* ResistanceText = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("ResistanceText"));
		StyleText(ResistanceText, FText::FromString(TEXT("RES 4")), 11, Muted, TEXT("Bold"));
		Details->AddChildToHorizontalBox(ResistanceText);
		MarkWidgetVariable(Blueprint, ResistanceText);

		if (!AddOpacityAnimation(Blueprint, TEXT("IntroAnimation"), Canvas,
			{{ 0.0f, 0.0f }, { 0.16f, 1.0f }})
			|| !AddOpacityAnimation(Blueprint, TEXT("DamagePulseAnimation"), DamagePulseSurface,
				{{ 0.0f, 0.0f }, { 0.05f, 1.0f }, { 0.20f, 0.0f }})
			|| !AddOpacityAnimation(Blueprint, TEXT("ShieldPulseAnimation"), ShieldPulseSurface,
				{{ 0.0f, 0.0f }, { 0.06f, 1.0f }, { 0.22f, 0.0f }})
			|| !AddOpacityAnimation(Blueprint, TEXT("DestroyedPulseAnimation"), DestroyedOverlay,
				{{ 0.0f, 0.25f }, { 0.08f, 1.0f }, { 0.30f, 0.62f }})
			|| !AddOpacityAnimation(Blueprint, TEXT("ContextHighlightAnimation"), ContextHighlight,
				{{ 0.0f, 0.0f }, { 0.12f, 1.0f }})
			|| !AddOpacityAnimation(Blueprint, TEXT("InitiativePulseAnimation"), InitiativeDiamond,
				{{ 0.0f, 1.0f }, { 0.07f, 0.35f }, { 0.18f, 1.0f }})
			|| !AddOpacityAnimation(Blueprint, TEXT("IntentChangedAnimation"), IntentIcon,
				{{ 0.0f, 0.15f }, { 0.14f, 1.0f }}))
		{
			return false;
		}

		if (!CompileAndSave(Blueprint))
		{
			return false;
		}
		UWacomBattleEnemyPartEntryWidget* CDO =
			Cast<UWacomBattleEnemyPartEntryWidget>(Blueprint->GeneratedClass->GetDefaultObject());
		if (!CDO)
		{
			return false;
		}
		CDO->SetIntentPresentationStyle(IntentStyle);
		CDO->SetDisplayCurrentHpOnly(true);
		return SaveCompiledDefaults(Blueprint);
	}

	bool BuildPanelBlueprint(UWidgetBlueprint* Blueprint, UClass* EntryClass)
	{
		if (!EntryClass)
		{
			return false;
		}
		ResetWidgetBlueprint(
			Blueprint,
			TEXT("单部位 Scene Enemy 的紧凑被动 Screen-space 面板。常态隐藏名称，hover 或 Action Preview 展开。"));
		UWidgetTree* Tree = Blueprint->WidgetTree;
		USizeBox* Root = Tree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("SinglePartPanelRoot"));
		Root->SetMinDesiredWidth(250.0f);
		Tree->RootWidget = Root;
		UOverlay* Overlay = Tree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), TEXT("SinglePartPanelOverlay"));
		Root->AddChild(Overlay);
		UBorder* PanelContextHighlight = AddOverlaySurface(
			Tree, Overlay, TEXT("PanelContextHighlight"),
			FLinearColor(Cyan.R, Cyan.G, Cyan.B, 0.06f),
			ESlateVisibility::Collapsed);
		MarkWidgetVariable(Blueprint, PanelContextHighlight);

		UVerticalBox* Content = Tree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("PanelContent"));
		Overlay->AddChildToOverlay(Content);
		UHorizontalBox* Header = Tree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("EnemyHeader"));
		Header->SetVisibility(ESlateVisibility::HitTestInvisible);
		Content->AddChildToVerticalBox(Header)->SetPadding(FMargin(54.0f, 0.0f, 8.0f, 2.0f));
		UTextBlock* EnemyNameText = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("EnemyNameText"));
		StyleText(EnemyNameText, FText::FromString(TEXT("训练战士")), 12, Paper, TEXT("Bold"));
		if (UHorizontalBoxSlot* Slot = Header->AddChildToHorizontalBox(EnemyNameText))
		{
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		MarkWidgetVariable(Blueprint, EnemyNameText);
		UTextBlock* EnemyInitiativeText = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("EnemyInitiativeText"));
		StyleText(EnemyInitiativeText, FText::FromString(TEXT("INIT 1")), 11, Amber, TEXT("Bold"));
		EnemyInitiativeText->SetVisibility(ESlateVisibility::Collapsed);
		Header->AddChildToHorizontalBox(EnemyInitiativeText);
		MarkWidgetVariable(Blueprint, EnemyInitiativeText);

		UVerticalBox* PartList = Tree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("PartList"));
		Content->AddChildToVerticalBox(PartList);
		MarkWidgetVariable(Blueprint, PartList);

		if (!CompileAndSave(Blueprint))
		{
			return false;
		}
		UWacomBattleEnemyPanelWidget* CDO =
			Cast<UWacomBattleEnemyPanelWidget>(Blueprint->GeneratedClass->GetDefaultObject());
		if (!CDO)
		{
			return false;
		}
		CDO->SetPartEntryWidgetClass(EntryClass);
		CDO->SetCompactSinglePartPresentation(true);
		return SaveCompiledDefaults(Blueprint);
	}

	bool HasWidgetOfClass(
		const UWidgetBlueprint* Blueprint,
		const FName WidgetName,
		const UClass* RequiredClass)
	{
		const UWidget* Widget = Blueprint && Blueprint->WidgetTree
			? Blueprint->WidgetTree->FindWidget(WidgetName)
			: nullptr;
		return Widget && Widget->IsA(RequiredClass);
	}

	bool HasAnimation(const UWidgetBlueprint* Blueprint, const FName AnimationName)
	{
		if (!Blueprint)
		{
			return false;
		}
		for (const UWidgetAnimation* Animation : Blueprint->Animations)
		{
			if (Animation && Animation->GetFName() == AnimationName
				&& Animation->MovieScene
				&& !Animation->GetBindings().IsEmpty())
			{
				return true;
			}
		}
		return false;
	}

	bool IsTreeNonHitTestable(const UWidgetBlueprint* Blueprint)
	{
		if (!Blueprint || !Blueprint->WidgetTree || !Blueprint->WidgetTree->RootWidget)
		{
			return false;
		}
		TArray<UWidget*> Widgets;
		Blueprint->WidgetTree->GetAllWidgets(Widgets);
		for (const UWidget* Widget : Widgets)
		{
			if (Widget && (Widget->GetVisibility() == ESlateVisibility::Visible
				|| Widget->GetVisibility() == ESlateVisibility::SelfHitTestInvisible))
			{
				return false;
			}
		}
		return true;
	}

	bool ValidateEntryBlueprint(
		const UWidgetBlueprint* Blueprint,
		const UWacomBattleEnemyIntentPresentationStyle* ExpectedStyle,
		bool bLogErrors)
	{
		bool bValid = Blueprint
			&& Blueprint->ParentClass
			&& Blueprint->ParentClass->IsChildOf(
				UWacomBattleEnemyPartEntryWidget::StaticClass())
			&& Blueprint->BlueprintDescription.Contains(WidgetContractMarker)
			&& Blueprint->GeneratedClass;
		const TPair<FName, UClass*> RequiredWidgets[] = {
			{ TEXT("PartNameText"), UTextBlock::StaticClass() },
			{ TEXT("HpBar"), UProgressBar::StaticClass() },
			{ TEXT("HpText"), UTextBlock::StaticClass() },
			{ TEXT("ShieldContainer"), UWidget::StaticClass() },
			{ TEXT("ShieldBar"), UProgressBar::StaticClass() },
			{ TEXT("ShieldText"), UTextBlock::StaticClass() },
			{ TEXT("InitiativeText"), UTextBlock::StaticClass() },
			{ TEXT("InitiativeDiamond"), UWidget::StaticClass() },
			{ TEXT("IntentDiamond"), UWidget::StaticClass() },
			{ TEXT("IntentIcon"), UImage::StaticClass() },
			{ TEXT("IntentText"), UTextBlock::StaticClass() },
			{ TEXT("ResistanceText"), UTextBlock::StaticClass() },
			{ TEXT("DetailsContainer"), UWidget::StaticClass() },
			{ TEXT("StatusList"), UWacomBattleStatusIconListWidget::StaticClass() },
			{ TEXT("ContextHighlight"), UWidget::StaticClass() },
			{ TEXT("ActionPreviewOverlay"), UWidget::StaticClass() },
			{ TEXT("DestroyedOverlay"), UWidget::StaticClass() },
			{ TEXT("DestroyedMark"), UWidget::StaticClass() },
		};
		for (const TPair<FName, UClass*>& Required : RequiredWidgets)
		{
			const bool bBindingValid =
				HasWidgetOfClass(Blueprint, Required.Key, Required.Value);
			bValid &= bBindingValid;
			if (!bBindingValid && bLogErrors)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[EnemySinglePartUIBuilder] Missing Entry binding %s (%s)"),
					*Required.Key.ToString(), *Required.Value->GetName());
			}
		}
		const FName RequiredAnimations[] = {
			TEXT("IntroAnimation"),
			TEXT("DamagePulseAnimation"),
			TEXT("ShieldPulseAnimation"),
			TEXT("DestroyedPulseAnimation"),
			TEXT("ContextHighlightAnimation"),
			TEXT("InitiativePulseAnimation"),
			TEXT("IntentChangedAnimation"),
		};
		for (const FName AnimationName : RequiredAnimations)
		{
			const bool bAnimationValid = HasAnimation(Blueprint, AnimationName);
			bValid &= bAnimationValid;
			if (!bAnimationValid && bLogErrors)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[EnemySinglePartUIBuilder] Missing Entry animation %s"),
					*AnimationName.ToString());
			}
		}
		bValid &= IsTreeNonHitTestable(Blueprint);
		const UWacomBattleEnemyPartEntryWidget* CDO =
			Blueprint && Blueprint->GeneratedClass
				? Cast<UWacomBattleEnemyPartEntryWidget>(
					Blueprint->GeneratedClass->GetDefaultObject())
				: nullptr;
		bValid &= CDO
			&& CDO->GetIntentPresentationStyle() == ExpectedStyle
			&& CDO->IsDisplayingCurrentHpOnly();
		if (!bValid && bLogErrors)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[EnemySinglePartUIBuilder] Compact Entry contract mismatch: %s"),
				Blueprint ? *Blueprint->GetPathName() : EntryAssetName);
		}
		return bValid;
	}

	bool ValidatePanelBlueprint(
		const UWidgetBlueprint* Blueprint,
		const UClass* ExpectedEntryClass,
		bool bLogErrors)
	{
		bool bValid = Blueprint
			&& Blueprint->ParentClass
			&& Blueprint->ParentClass->IsChildOf(UWacomBattleEnemyPanelWidget::StaticClass())
			&& Blueprint->BlueprintDescription.Contains(WidgetContractMarker)
			&& Blueprint->GeneratedClass
			&& HasWidgetOfClass(Blueprint, TEXT("EnemyNameText"), UTextBlock::StaticClass())
			&& HasWidgetOfClass(Blueprint, TEXT("EnemyInitiativeText"), UTextBlock::StaticClass())
			&& HasWidgetOfClass(Blueprint, TEXT("PartList"), UPanelWidget::StaticClass())
			&& HasWidgetOfClass(Blueprint, TEXT("PanelContextHighlight"), UWidget::StaticClass())
			&& IsTreeNonHitTestable(Blueprint);
		const UWacomBattleEnemyPanelWidget* CDO =
			Blueprint && Blueprint->GeneratedClass
				? Cast<UWacomBattleEnemyPanelWidget>(
					Blueprint->GeneratedClass->GetDefaultObject())
				: nullptr;
		bValid &= CDO
			&& CDO->GetPartEntryWidgetClass().Get() == ExpectedEntryClass
			&& CDO->IsCompactSinglePartPresentation();
		if (!bValid && bLogErrors)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[EnemySinglePartUIBuilder] Compact Panel contract mismatch: %s"),
				Blueprint ? *Blueprint->GetPathName() : PanelAssetName);
		}
		return bValid;
	}
}

bool Wacom::ContentBuilder::ProcessEnemySinglePartUI(
	const bool bBuild,
	const bool bInspectOnly)
{
	if (bBuild == bInspectOnly)
	{
		return false;
	}

	FIntentAssets Assets;
	Assets.Fallback = EnsureIconTexture(
		FallbackTextureName, EIntentIconKind::Fallback, bBuild);
	Assets.Attack = EnsureIconTexture(
		AttackTextureName, EIntentIconKind::Attack, bBuild);
	Assets.Guard = EnsureIconTexture(
		GuardTextureName, EIntentIconKind::Guard, bBuild);
	Assets.Cleave = EnsureIconTexture(
		CleaveTextureName, EIntentIconKind::Cleave, bBuild);
	if (!Assets.Fallback || !Assets.Attack || !Assets.Guard || !Assets.Cleave)
	{
		return false;
	}
	Assets.Style = EnsureIntentStyle(Assets, bBuild);
	if (!Assets.Style)
	{
		return false;
	}

	FWidgetBlueprintAsset EntryAsset = LoadOrCreateWidgetBlueprint(
		EntryAssetName,
		UWacomBattleEnemyPartEntryWidget::StaticClass(),
		bBuild);
	if (!EntryAsset.Blueprint)
	{
		return false;
	}
	if (!EntryAsset.bCreated
		&& !EntryAsset.Blueprint->BlueprintDescription.Contains(WidgetContractMarker))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[EnemySinglePartUIBuilder] Unknown manual compact Entry detected; no overwrite: %s"),
			*EntryAsset.Blueprint->GetPathName());
		return false;
	}
	bool bEntryValid = ValidateEntryBlueprint(
		EntryAsset.Blueprint, Assets.Style, false);
	if (!bEntryValid && bBuild)
	{
		bEntryValid = BuildEntryBlueprint(EntryAsset.Blueprint, Assets.Style)
			&& ValidateEntryBlueprint(EntryAsset.Blueprint, Assets.Style, true);
	}
	if (!bEntryValid)
	{
		ValidateEntryBlueprint(EntryAsset.Blueprint, Assets.Style, true);
		return false;
	}

	FWidgetBlueprintAsset PanelAsset = LoadOrCreateWidgetBlueprint(
		PanelAssetName,
		UWacomBattleEnemyPanelWidget::StaticClass(),
		bBuild);
	if (!PanelAsset.Blueprint)
	{
		return false;
	}
	if (!PanelAsset.bCreated
		&& !PanelAsset.Blueprint->BlueprintDescription.Contains(WidgetContractMarker))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[EnemySinglePartUIBuilder] Unknown manual compact Panel detected; no overwrite: %s"),
			*PanelAsset.Blueprint->GetPathName());
		return false;
	}
	bool bPanelValid = ValidatePanelBlueprint(
		PanelAsset.Blueprint, EntryAsset.Blueprint->GeneratedClass, false);
	if (!bPanelValid && bBuild)
	{
		bPanelValid = BuildPanelBlueprint(
			PanelAsset.Blueprint, EntryAsset.Blueprint->GeneratedClass)
			&& ValidatePanelBlueprint(
				PanelAsset.Blueprint, EntryAsset.Blueprint->GeneratedClass, true);
	}
	if (!bPanelValid)
	{
		ValidatePanelBlueprint(
			PanelAsset.Blueprint, EntryAsset.Blueprint->GeneratedClass, true);
		return false;
	}

	UE_LOG(LogTemp, Display,
		TEXT("[EnemySinglePartUIBuilder] Compact single-part Enemy UI contract valid%s"),
		bInspectOnly ? TEXT(" (inspect only)") : TEXT(""));
	return true;
}
