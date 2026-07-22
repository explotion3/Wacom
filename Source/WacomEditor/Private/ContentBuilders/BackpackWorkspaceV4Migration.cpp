// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/BackpackWorkspaceV4Migration.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UI/Backpack/WacomBackpackControlsHelpWidget.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"
#include "UI/Backpack/WacomBackpackZonePileWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintOperationUtils.h"

namespace Wacom::ContentBuilder
{
namespace
{
constexpr TCHAR CommandletTag[] = TEXT("WacomMigrateBackpackWorkspaceV4");
constexpr TCHAR IconsRoot[] = TEXT("/Game/Wacom/UI/Backpack/Icons");

const TArray<FString>& ExactPackageManifest()
{
	static const TArray<FString> Manifest = {
		TEXT("/Game/Wacom/UI/Backpack/Icons/T_BackpackState_Focus"),
		TEXT("/Game/Wacom/UI/Backpack/Icons/T_BackpackState_Selected"),
		TEXT("/Game/Wacom/UI/Backpack/Icons/T_BackpackState_ValidDrop"),
		TEXT("/Game/Wacom/UI/Backpack/Icons/T_BackpackState_RejectedDrop"),
		TEXT("/Game/Wacom/UI/Backpack/WBP_BackpackControlsHelp"),
		TEXT("/Game/Wacom/UI/Backpack/WBP_BackpackScreen"),
		TEXT("/Game/Wacom/UI/Card/WBP_WacomDeckCardWidget"),
		TEXT("/Game/Wacom/UI/Backpack/WBP_BackpackZonePile"),
		TEXT("/Game/Wacom/UI/Backpack/DA_BackpackWorkspaceStyle"),
	};
	return Manifest;
}

FString ObjectPath(const FString& PackageName)
{
	return PackageName + TEXT(".") + FPackageName::GetShortName(PackageName);
}

FString PackageFilename(const FString& PackageName)
{
	return FPackageName::LongPackageNameToFilename(
		PackageName,
		FPackageName::GetAssetPackageExtension());
}

void LogManifestBaseline()
{
	for (const FString& PackageName : ExactPackageManifest())
	{
		const FString Filename = PackageFilename(PackageName);
		const bool bExists = IFileManager::Get().FileExists(*Filename);
		UE_LOG(LogTemp, Display,
			TEXT("[%s] MANIFEST package=%s exists=%s"),
			CommandletTag,
			*PackageName,
			bExists ? TEXT("true") : TEXT("false"));
	}
}

bool IsManifestPackage(const FString& PackageName)
{
	return ExactPackageManifest().Contains(PackageName);
}

struct FMigrationContext
{
	bool bApply = false;
	bool bValidationFailed = false;
	TSet<FString> ChangedPackages;
	TMap<FString, UObject*> TopLevelAssets;

	void RegisterAsset(UObject& Asset)
	{
		TopLevelAssets.Add(Asset.GetOutermost()->GetName(), &Asset);
	}

	bool MarkChanged(UObject& Asset)
	{
		const FString PackageName = Asset.GetOutermost()->GetName();
		if (!IsManifestPackage(PackageName))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[%s] Refused manifest-external mutation: %s"),
				CommandletTag,
				*PackageName);
			return false;
		}
		ChangedPackages.Add(PackageName);
		RegisterAsset(Asset);
		return true;
	}
};

bool SaveExactAsset(UObject& Asset, FBackpackWorkspaceV4MigrationReport& Report)
{
	UPackage* Package = Asset.GetOutermost();
	if (!Package || !IsManifestPackage(Package->GetName()))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[%s] Save rejected outside exact manifest: %s"),
			CommandletTag,
			Package ? *Package->GetName() : TEXT("<null>"));
		return false;
	}
	const FString Filename = PackageFilename(Package->GetName());
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
	Package->MarkPackageDirty();
	Asset.MarkPackageDirty();
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	Args.SaveFlags = SAVE_NoError;
	if (!UPackage::SavePackage(Package, &Asset, *Filename, Args))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[%s] Failed to save %s"), CommandletTag, *Package->GetName());
		return false;
	}
	Report.SavedPackages.Add(Package->GetName());
	UE_LOG(LogTemp, Display,
		TEXT("[%s] SAVED package=%s"),
		CommandletTag,
		*Package->GetName());
	return true;
}

void DrawPixel(TArray<FColor>& Pixels, int32 Size, int32 X, int32 Y, int32 Thickness = 1)
{
	for (int32 DY = 0; DY < Thickness; ++DY)
	{
		for (int32 DX = 0; DX < Thickness; ++DX)
		{
			const int32 PX = X + DX;
			const int32 PY = Y + DY;
			if (PX >= 0 && PX < Size && PY >= 0 && PY < Size)
			{
				Pixels[PY * Size + PX] = FColor::White;
			}
		}
	}
}

void DrawLine(TArray<FColor>& Pixels, int32 Size, FIntPoint A, FIntPoint B, int32 Thickness = 1)
{
	int32 X = A.X;
	int32 Y = A.Y;
	const int32 DX = FMath::Abs(B.X - A.X);
	const int32 SX = A.X < B.X ? 1 : -1;
	const int32 DY = -FMath::Abs(B.Y - A.Y);
	const int32 SY = A.Y < B.Y ? 1 : -1;
	int32 Error = DX + DY;
	for (;;)
	{
		DrawPixel(Pixels, Size, X, Y, Thickness);
		if (X == B.X && Y == B.Y)
		{
			break;
		}
		const int32 TwiceError = Error * 2;
		if (TwiceError >= DY)
		{
			Error += DY;
			X += SX;
		}
		if (TwiceError <= DX)
		{
			Error += DX;
			Y += SY;
		}
	}
}

enum class EAccessibilityGlyph : uint8
{
	Focus,
	Selected,
	ValidDrop,
	RejectedDrop,
};

TArray<FColor> BuildAccessibilityPixels(EAccessibilityGlyph Glyph)
{
	constexpr int32 Size = 64;
	TArray<FColor> Pixels;
	Pixels.Init(FColor(255, 255, 255, 0), Size * Size);
	if (Glyph == EAccessibilityGlyph::Focus)
	{
		DrawLine(Pixels, Size, {7, 20}, {7, 7}, 4);
		DrawLine(Pixels, Size, {7, 7}, {20, 7}, 4);
		DrawLine(Pixels, Size, {43, 7}, {56, 7}, 4);
		DrawLine(Pixels, Size, {56, 7}, {56, 20}, 4);
		DrawLine(Pixels, Size, {7, 43}, {7, 56}, 4);
		DrawLine(Pixels, Size, {7, 56}, {20, 56}, 4);
		DrawLine(Pixels, Size, {43, 56}, {56, 56}, 4);
		DrawLine(Pixels, Size, {56, 43}, {56, 56}, 4);
	}
	else if (Glyph == EAccessibilityGlyph::Selected)
	{
		DrawLine(Pixels, Size, {9, 32}, {24, 48}, 5);
		DrawLine(Pixels, Size, {24, 48}, {55, 14}, 5);
		DrawLine(Pixels, Size, {10, 57}, {54, 57}, 3);
	}
	else if (Glyph == EAccessibilityGlyph::ValidDrop)
	{
		DrawLine(Pixels, Size, {32, 7}, {32, 39}, 5);
		DrawLine(Pixels, Size, {17, 27}, {32, 42}, 5);
		DrawLine(Pixels, Size, {32, 42}, {47, 27}, 5);
		DrawLine(Pixels, Size, {10, 50}, {10, 57}, 4);
		DrawLine(Pixels, Size, {10, 57}, {54, 57}, 4);
		DrawLine(Pixels, Size, {54, 50}, {54, 57}, 4);
	}
	else
	{
		DrawLine(Pixels, Size, {10, 10}, {54, 54}, 5);
		DrawLine(Pixels, Size, {54, 10}, {10, 54}, 5);
		DrawLine(Pixels, Size, {18, 59}, {46, 59}, 3);
	}
	return Pixels;
}

bool ValidateExistingTexture(const UTexture2D& Texture)
{
	return Texture.Source.GetSizeX() == 64
		&& Texture.Source.GetSizeY() == 64
		&& Texture.CompressionSettings == TC_EditorIcon
		&& Texture.MipGenSettings == TMGS_NoMipmaps
		&& Texture.LODGroup == TEXTUREGROUP_UI
		&& Texture.Filter == TF_Nearest
		&& Texture.NeverStream;
}

UTexture2D* EnsureTexture(
	FMigrationContext& Context,
	const TCHAR* AssetName,
	EAccessibilityGlyph Glyph)
{
	const FString PackageName = FString(IconsRoot) / AssetName;
	if (UTexture2D* Existing = LoadObject<UTexture2D>(nullptr, *ObjectPath(PackageName)))
	{
		if (!ValidateExistingTexture(*Existing))
		{
			Context.bValidationFailed = true;
			UE_LOG(LogTemp, Error,
				TEXT("[%s] Existing texture has unexpected authored settings: %s"),
				CommandletTag,
				*PackageName);
			return nullptr;
		}
		Context.RegisterAsset(*Existing);
		return Existing;
	}
	if (IFileManager::Get().FileExists(*PackageFilename(PackageName)))
	{
		Context.bValidationFailed = true;
		UE_LOG(LogTemp, Error,
			TEXT("[%s] Existing package did not load as the expected texture: %s"),
			CommandletTag,
			*PackageName);
		return nullptr;
	}
	if (!Context.bApply)
	{
		return nullptr;
	}
	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		return nullptr;
	}
	UTexture2D* Texture = NewObject<UTexture2D>(
		Package,
		AssetName,
		RF_Public | RF_Standalone | RF_Transactional);
	if (!Texture)
	{
		return nullptr;
	}
	const TArray<FColor> Pixels = BuildAccessibilityPixels(Glyph);
	Texture->Source.Init(
		64,
		64,
		1,
		1,
		TSF_BGRA8,
		reinterpret_cast<const uint8*>(Pixels.GetData()));
	Texture->CompressionSettings = TC_EditorIcon;
	Texture->MipGenSettings = TMGS_NoMipmaps;
	Texture->LODGroup = TEXTUREGROUP_UI;
	Texture->Filter = TF_Nearest;
	Texture->AddressX = TA_Clamp;
	Texture->AddressY = TA_Clamp;
	Texture->NeverStream = true;
	Texture->SRGB = true;
	FAssetRegistryModule::AssetCreated(Texture);
	Texture->PostEditChange();
	if (!Context.MarkChanged(*Texture))
	{
		return nullptr;
	}
	return Texture;
}

template <typename TWidget>
TWidget* FindTypedWidget(UWidgetTree& Tree, FName Name)
{
	UWidget* Widget = Tree.FindWidget(Name);
	if (Widget && !Widget->IsA<TWidget>())
	{
		UE_LOG(LogTemp, Error,
			TEXT("[%s] Widget %s exists with unexpected type %s"),
			CommandletTag,
			*Name.ToString(),
			*Widget->GetClass()->GetName());
		return nullptr;
	}
	return Cast<TWidget>(Widget);
}

bool ValidateOptionalWidgetType(UWidgetTree& Tree, FName Name, UClass& ExpectedClass)
{
	UWidget* Widget = Tree.FindWidget(Name);
	if (!Widget || Widget->IsA(&ExpectedClass))
	{
		return true;
	}
	UE_LOG(LogTemp, Error,
		TEXT("[%s] Existing widget %s is %s, expected %s"),
		CommandletTag,
		*Name.ToString(),
		*Widget->GetClass()->GetName(),
		*ExpectedClass.GetName());
	return false;
}

bool EnsureWidgetGuid(UWidgetBlueprint& Blueprint, const UWidget& Widget)
{
	if (Blueprint.WidgetVariableNameToGuidMap.Contains(Widget.GetFName()))
	{
		return false;
	}
	const FString StablePath = FString::Printf(
		TEXT("%s:%s"),
		*Blueprint.GetPathName(),
		*Widget.GetName());
	Blueprint.WidgetVariableNameToGuidMap.Add(
		Widget.GetFName(),
		FGuid::NewDeterministicGuid(StablePath));
	return true;
}

bool EnsureAllWidgetGuids(UWidgetBlueprint& Blueprint)
{
	bool bChanged = false;
	if (Blueprint.WidgetTree)
	{
		Blueprint.WidgetTree->ForEachWidget([&](UWidget* Widget)
		{
			if (Widget)
			{
				bChanged |= EnsureWidgetGuid(Blueprint, *Widget);
			}
		});
	}
	return bChanged;
}

bool HasAllWidgetGuids(const UWidgetBlueprint& Blueprint)
{
	bool bHasAllGuids = true;
	if (!Blueprint.WidgetTree)
	{
		return false;
	}
	Blueprint.WidgetTree->ForEachWidget([&](UWidget* Widget)
	{
		bHasAllGuids &= Widget
			&& Blueprint.WidgetVariableNameToGuidMap.Contains(Widget->GetFName());
	});
	return bHasAllGuids;
}

bool EnsureVariable(UWidgetBlueprint& Blueprint, UWidget& Widget, bool& bChanged)
{
	if (Widget.bIsVariable
		&& Blueprint.WidgetVariableNameToGuidMap.Contains(Widget.GetFName()))
	{
		return true;
	}
	if (!Widget.bIsVariable)
	{
		FWidgetBlueprintOperationUtils::ToggleWidgetAsVariable(
			&Blueprint,
			&Widget,
			true,
			false);
		bChanged = true;
	}
	bChanged |= EnsureWidgetGuid(Blueprint, Widget);
	return Widget.bIsVariable
		&& Blueprint.WidgetVariableNameToGuidMap.Contains(Widget.GetFName());
}

bool IsWidgetVariable(const UWidgetBlueprint& Blueprint, const UWidget* Widget)
{
	return Widget
		&& Widget->bIsVariable
		&& Blueprint.WidgetVariableNameToGuidMap.Contains(Widget->GetFName());
}

bool CompileBlueprint(UWidgetBlueprint& Blueprint)
{
	EnsureAllWidgetGuids(Blueprint);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(&Blueprint);
	FKismetEditorUtilities::CompileBlueprint(&Blueprint);
	if (Blueprint.Status == BS_Error || !Blueprint.GeneratedClass)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[%s] Blueprint compile failed: %s"),
			CommandletTag,
			*Blueprint.GetPathName());
		return false;
	}
	return true;
}

bool SetObjectDefault(
	UWidgetBlueprint& Blueprint,
	FName PropertyName,
	UObject* Value,
	bool& bChanged)
{
	UObject* CDO = Blueprint.GeneratedClass
		? Blueprint.GeneratedClass->GetDefaultObject()
		: nullptr;
	FObjectPropertyBase* Property = CDO
		? FindFProperty<FObjectPropertyBase>(CDO->GetClass(), PropertyName)
		: nullptr;
	if (!CDO || !Property)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[%s] Missing CDO property %s on %s"),
			CommandletTag,
			*PropertyName.ToString(),
			*Blueprint.GetPathName());
		return false;
	}
	if (Property->GetObjectPropertyValue_InContainer(CDO) == Value)
	{
		return true;
	}
	CDO->Modify();
	Property->SetObjectPropertyValue_InContainer(CDO, Value);
	CDO->MarkPackageDirty();
	bChanged = true;
	return true;
}

bool HasExpectedHelpBlueprintShape(const UWidgetBlueprint& Blueprint)
{
	if (Blueprint.ParentClass != UWacomBackpackControlsHelpWidget::StaticClass()
		|| !Blueprint.WidgetTree
		|| !Cast<UOverlay>(Blueprint.WidgetTree->RootWidget))
	{
		return false;
	}
	const UTextBlock* HelpText = Cast<UTextBlock>(
		Blueprint.WidgetTree->FindWidget(TEXT("HelpText")));
	const UButton* CloseButton = Cast<UButton>(
		Blueprint.WidgetTree->FindWidget(TEXT("CloseHelpButton")));
	return HelpText && CloseButton;
}

bool IsHelpBlueprintCurrent(const UWidgetBlueprint& Blueprint)
{
	if (!HasExpectedHelpBlueprintShape(Blueprint))
	{
		return false;
	}
	const UTextBlock* HelpText = CastChecked<UTextBlock>(
		Blueprint.WidgetTree->FindWidget(TEXT("HelpText")));
	const UButton* CloseButton = CastChecked<UButton>(
		Blueprint.WidgetTree->FindWidget(TEXT("CloseHelpButton")));
	return IsWidgetVariable(Blueprint, HelpText)
		&& IsWidgetVariable(Blueprint, CloseButton)
		&& HasAllWidgetGuids(Blueprint);
}

bool BuildHelpTree(UWidgetBlueprint& Blueprint)
{
	if (!Blueprint.WidgetTree)
	{
		return false;
	}
	UWidgetTree& Tree = *Blueprint.WidgetTree;
	UOverlay* Root = Tree.ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	Tree.RootWidget = Root;
	UBorder* Dim = Tree.ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dim"));
	Dim->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.78f));
	Root->AddChildToOverlay(Dim);

	USizeBox* PanelSize = Tree.ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelSize"));
	PanelSize->SetWidthOverride(680.0f);
	PanelSize->SetMinDesiredHeight(430.0f);
	if (UOverlaySlot* Slot = Root->AddChildToOverlay(PanelSize))
	{
		Slot->SetHorizontalAlignment(HAlign_Center);
		Slot->SetVerticalAlignment(VAlign_Center);
		Slot->SetPadding(FMargin(32.0f));
	}
	UBorder* Panel = Tree.ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrushColor(FLinearColor(0.025f, 0.042f, 0.066f, 0.995f));
	Panel->SetPadding(FMargin(30.0f));
	PanelSize->AddChild(Panel);
	UVerticalBox* Column = Tree.ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	Panel->AddChild(Column);

	UTextBlock* Title = Tree.ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HelpTitle"));
	Title->SetText(NSLOCTEXT("WacomBackpackV4Migration", "HelpTitle", "背包操作说明"));
	FSlateFontInfo TitleFont = Title->GetFont();
	TitleFont.Size = 28;
	Title->SetFont(TitleFont);
	if (UVerticalBoxSlot* Slot = Column->AddChildToVerticalBox(Title))
	{
		Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
	}
	UTextBlock* HelpText = Tree.ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HelpText"));
	HelpText->SetAutoWrapText(true);
	FSlateFontInfo HelpFont = HelpText->GetFont();
	HelpFont.Size = 19;
	HelpText->SetFont(HelpFont);
	if (UVerticalBoxSlot* Slot = Column->AddChildToVerticalBox(HelpText))
	{
		Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 22.0f));
	}
	UButton* CloseButton = Tree.ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseHelpButton"));
	CloseButton->SetBackgroundColor(FLinearColor(0.12f, 0.28f, 0.39f, 1.0f));
	UTextBlock* CloseLabel = Tree.ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseHelpLabel"));
	CloseLabel->SetText(NSLOCTEXT("WacomBackpackV4Migration", "CloseHelp", "关闭说明"));
	CloseLabel->SetJustification(ETextJustify::Center);
	CloseButton->AddChild(CloseLabel);
	if (UVerticalBoxSlot* Slot = Column->AddChildToVerticalBox(CloseButton))
	{
		Slot->SetHorizontalAlignment(HAlign_Right);
	}
	bool bVariablesChanged = false;
	return EnsureVariable(Blueprint, *HelpText, bVariablesChanged)
		&& EnsureVariable(Blueprint, *CloseButton, bVariablesChanged);
}

UWidgetBlueprint* EnsureHelpBlueprint(FMigrationContext& Context)
{
	const FString PackageName = TEXT("/Game/Wacom/UI/Backpack/WBP_BackpackControlsHelp");
	if (UWidgetBlueprint* Existing = LoadObject<UWidgetBlueprint>(nullptr, *ObjectPath(PackageName)))
	{
		if (!HasExpectedHelpBlueprintShape(*Existing))
		{
			Context.bValidationFailed = true;
			UE_LOG(LogTemp, Error,
				TEXT("[%s] Existing help WBP is not the expected v4 contract"),
				CommandletTag);
			return nullptr;
		}
		if (!IsHelpBlueprintCurrent(*Existing) && Context.bApply)
		{
			bool bChanged = false;
			UTextBlock* HelpText = CastChecked<UTextBlock>(
				Existing->WidgetTree->FindWidget(TEXT("HelpText")));
			UButton* CloseButton = CastChecked<UButton>(
				Existing->WidgetTree->FindWidget(TEXT("CloseHelpButton")));
			if (!EnsureVariable(*Existing, *HelpText, bChanged)
				|| !EnsureVariable(*Existing, *CloseButton, bChanged))
			{
				return nullptr;
			}
			bChanged |= EnsureAllWidgetGuids(*Existing);
			if (bChanged
				&& (!CompileBlueprint(*Existing) || !Context.MarkChanged(*Existing)))
			{
				return nullptr;
			}
		}
		Context.RegisterAsset(*Existing);
		return Existing;
	}
	if (IFileManager::Get().FileExists(*PackageFilename(PackageName)))
	{
		Context.bValidationFailed = true;
		UE_LOG(LogTemp, Error,
			TEXT("[%s] Existing package did not load as the expected help WBP"),
			CommandletTag);
		return nullptr;
	}
	if (!Context.bApply)
	{
		return nullptr;
	}
	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		return nullptr;
	}
	Package->FullyLoad();
	UWidgetBlueprint* Blueprint = FWidgetBlueprintOperationUtils::CreateWidgetBlueprint(
		Package,
		TEXT("WBP_BackpackControlsHelp"),
		BPTYPE_Normal,
		UWacomBackpackControlsHelpWidget::StaticClass(),
		nullptr,
		CommandletTag,
		true);
	if (!Blueprint || !BuildHelpTree(*Blueprint) || !CompileBlueprint(*Blueprint)
		|| !Context.MarkChanged(*Blueprint))
	{
		return nullptr;
	}
	return Blueprint;
}

bool PreflightBlueprints(
	UWidgetBlueprint*& OutScreen,
	UWidgetBlueprint*& OutDeckCard,
	UWidgetBlueprint*& OutZonePile,
	UWacomBackpackWorkspaceStyle*& OutStyle)
{
	OutScreen = LoadObject<UWidgetBlueprint>(nullptr,
		TEXT("/Game/Wacom/UI/Backpack/WBP_BackpackScreen.WBP_BackpackScreen"));
	OutDeckCard = LoadObject<UWidgetBlueprint>(nullptr,
		TEXT("/Game/Wacom/UI/Card/WBP_WacomDeckCardWidget.WBP_WacomDeckCardWidget"));
	OutZonePile = LoadObject<UWidgetBlueprint>(nullptr,
		TEXT("/Game/Wacom/UI/Backpack/WBP_BackpackZonePile.WBP_BackpackZonePile"));
	OutStyle = LoadObject<UWacomBackpackWorkspaceStyle>(nullptr,
		TEXT("/Game/Wacom/UI/Backpack/DA_BackpackWorkspaceStyle.DA_BackpackWorkspaceStyle"));
	if (!OutScreen || !OutScreen->WidgetTree
		|| OutScreen->ParentClass != UWacomBackpackScreen::StaticClass()
		|| !Cast<UVerticalBox>(OutScreen->WidgetTree->FindWidget(TEXT("MainLayout")))
		|| !Cast<UHorizontalBox>(OutScreen->WidgetTree->FindWidget(TEXT("Header")))
		|| !Cast<UOverlay>(OutScreen->WidgetTree->FindWidget(TEXT("Root")))
		|| !Cast<UOverlay>(OutScreen->WidgetTree->FindWidget(TEXT("DeleteTargetHost"))))
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] Screen preflight failed"), CommandletTag);
		return false;
	}
	if (!ValidateOptionalWidgetType(*OutScreen->WidgetTree, TEXT("InteractionHintText"), *UTextBlock::StaticClass())
		|| !ValidateOptionalWidgetType(*OutScreen->WidgetTree, TEXT("ControlsHelpButton"), *UButton::StaticClass())
		|| !ValidateOptionalWidgetType(*OutScreen->WidgetTree, TEXT("ControlsHelpHost"), *UOverlay::StaticClass())
		|| !ValidateOptionalWidgetType(*OutScreen->WidgetTree, TEXT("DeleteTargetFocusIcon"), *UImage::StaticClass()))
	{
		return false;
	}
	UWidget* DeckCardScaleBox = OutDeckCard && OutDeckCard->WidgetTree
		? OutDeckCard->WidgetTree->FindWidget(TEXT("CardFaceScaleBox"))
		: nullptr;
	if (!OutDeckCard || !OutDeckCard->WidgetTree
		|| OutDeckCard->ParentClass != UWacomDeckCardWidget::StaticClass()
		|| !Cast<UOverlay>(OutDeckCard->WidgetTree->FindWidget(TEXT("CardMotionRoot")))
		|| !DeckCardScaleBox
		|| !Cast<UOverlay>(DeckCardScaleBox->GetParent()))
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] DeckCard preflight failed"), CommandletTag);
		return false;
	}
	if (!ValidateOptionalWidgetType(*OutDeckCard->WidgetTree, TEXT("WorkspaceFocusIcon"), *UImage::StaticClass())
		|| !ValidateOptionalWidgetType(*OutDeckCard->WidgetTree, TEXT("WorkspaceStateIcon"), *UImage::StaticClass()))
	{
		return false;
	}
	if (!OutZonePile || !OutZonePile->WidgetTree
		|| OutZonePile->ParentClass != UWacomBackpackZonePileWidget::StaticClass()
		|| !Cast<UCanvasPanel>(OutZonePile->WidgetTree->RootWidget)
		|| !ValidateOptionalWidgetType(*OutZonePile->WidgetTree, TEXT("NavigationFocusIcon"), *UImage::StaticClass()))
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] ZonePile preflight failed"), CommandletTag);
		return false;
	}
	if (!OutStyle || (OutStyle->AssetVersion != 3
		&& OutStyle->AssetVersion != UWacomBackpackWorkspaceStyle::CurrentAssetVersion))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[%s] Style preflight expected version 3 or 4"), CommandletTag);
		return false;
	}
	return true;
}

bool ObjectDefaultEquals(
	const UWidgetBlueprint& Blueprint,
	FName PropertyName,
	const UObject* ExpectedValue)
{
	const UObject* CDO = Blueprint.GeneratedClass
		? Blueprint.GeneratedClass->GetDefaultObject()
		: nullptr;
	const FObjectPropertyBase* Property = CDO
		? FindFProperty<FObjectPropertyBase>(CDO->GetClass(), PropertyName)
		: nullptr;
	return CDO
		&& Property
		&& Property->GetObjectPropertyValue_InContainer(CDO) == ExpectedValue;
}

bool IsScreenCurrent(const UWidgetBlueprint& Blueprint, const UClass* HelpClass)
{
	if (!Blueprint.WidgetTree || !HelpClass)
	{
		return false;
	}
	const UWidgetTree& Tree = *Blueprint.WidgetTree;
	const UTextBlock* HintText = Cast<UTextBlock>(Tree.FindWidget(TEXT("InteractionHintText")));
	const UButton* HelpButton = Cast<UButton>(Tree.FindWidget(TEXT("ControlsHelpButton")));
	const UOverlay* HelpHost = Cast<UOverlay>(Tree.FindWidget(TEXT("ControlsHelpHost")));
	const UImage* DeleteFocus = Cast<UImage>(Tree.FindWidget(TEXT("DeleteTargetFocusIcon")));
	return HintText
		&& Cast<UBorder>(HintText->GetParent())
		&& HelpButton
		&& HelpButton->GetParent() == Tree.FindWidget(TEXT("Header"))
		&& HelpHost
		&& HelpHost->GetParent() == Tree.FindWidget(TEXT("Root"))
		&& DeleteFocus
		&& DeleteFocus->GetParent() == Tree.FindWidget(TEXT("DeleteTargetHost"))
		&& IsWidgetVariable(Blueprint, HintText)
		&& IsWidgetVariable(Blueprint, HelpButton)
		&& IsWidgetVariable(Blueprint, HelpHost)
		&& IsWidgetVariable(Blueprint, DeleteFocus)
		&& HasAllWidgetGuids(Blueprint)
		&& ObjectDefaultEquals(Blueprint, TEXT("ControlsHelpWidgetClass"), HelpClass);
}

bool IsDeckCardCurrent(const UWidgetBlueprint& Blueprint)
{
	if (!Blueprint.WidgetTree)
	{
		return false;
	}
	const UWidgetTree& Tree = *Blueprint.WidgetTree;
	const UWidget* ScaleBox = Tree.FindWidget(TEXT("CardFaceScaleBox"));
	const UOverlay* Host = ScaleBox ? Cast<UOverlay>(ScaleBox->GetParent()) : nullptr;
	const UImage* Focus = Cast<UImage>(Tree.FindWidget(TEXT("WorkspaceFocusIcon")));
	const UImage* State = Cast<UImage>(Tree.FindWidget(TEXT("WorkspaceStateIcon")));
	return Host
		&& Focus && Focus->GetParent() == Host
		&& State && State->GetParent() == Host
		&& IsWidgetVariable(Blueprint, Focus)
		&& IsWidgetVariable(Blueprint, State)
		&& HasAllWidgetGuids(Blueprint);
}

bool IsZonePileCurrent(const UWidgetBlueprint& Blueprint)
{
	if (!Blueprint.WidgetTree)
	{
		return false;
	}
	const UImage* Focus = Cast<UImage>(
		Blueprint.WidgetTree->FindWidget(TEXT("NavigationFocusIcon")));
	return Focus
		&& Focus->GetParent() == Blueprint.WidgetTree->RootWidget
		&& IsWidgetVariable(Blueprint, Focus)
		&& HasAllWidgetGuids(Blueprint);
}

bool PatchScreen(
	FMigrationContext& Context,
	UWidgetBlueprint& Blueprint,
	UClass* HelpClass)
{
	if (!HelpClass)
	{
		return false;
	}
	UWidgetTree& Tree = *Blueprint.WidgetTree;
	UVerticalBox* Main = CastChecked<UVerticalBox>(Tree.FindWidget(TEXT("MainLayout")));
	UHorizontalBox* Header = CastChecked<UHorizontalBox>(Tree.FindWidget(TEXT("Header")));
	UOverlay* Root = CastChecked<UOverlay>(Tree.FindWidget(TEXT("Root")));
	UOverlay* DeleteTarget = CastChecked<UOverlay>(Tree.FindWidget(TEXT("DeleteTargetHost")));
	bool bChanged = false;

	UTextBlock* HintText = FindTypedWidget<UTextBlock>(Tree, TEXT("InteractionHintText"));
	if (!HintText)
	{
		UBorder* HintBar = Tree.ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InteractionHintBar"));
		HintBar->SetPadding(FMargin(12.0f, 7.0f));
		HintBar->SetBrushColor(FLinearColor(0.035f, 0.075f, 0.10f, 0.96f));
		if (!Main->InsertChildAt(1, HintBar))
		{
			return false;
		}
		if (UVerticalBoxSlot* Slot = Cast<UVerticalBoxSlot>(HintBar->Slot))
		{
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
		}
		HintText = Tree.ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InteractionHintText"));
		HintText->SetText(NSLOCTEXT("WacomBackpackV4Migration", "HintPlaceholder", "F1：操作说明"));
		FSlateFontInfo Font = HintText->GetFont();
		Font.Size = 16;
		HintText->SetFont(Font);
		HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.76f, 0.88f, 0.94f, 1.0f)));
		HintBar->AddChild(HintText);
		bChanged = true;
	}

	UButton* HelpButton = FindTypedWidget<UButton>(Tree, TEXT("ControlsHelpButton"));
	if (!HelpButton)
	{
		HelpButton = Tree.ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ControlsHelpButton"));
		HelpButton->SetBackgroundColor(FLinearColor(0.10f, 0.26f, 0.36f, 1.0f));
		UTextBlock* Label = Tree.ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ControlsHelpButtonLabel"));
		Label->SetText(NSLOCTEXT("WacomBackpackV4Migration", "ControlsHelp", "操作说明"));
		Label->SetJustification(ETextJustify::Center);
		HelpButton->AddChild(Label);
		UWidget* CloseSize = Tree.FindWidget(TEXT("CloseButtonSize"));
		const int32 InsertIndex = CloseSize && CloseSize->GetParent() == Header
			? Header->GetChildIndex(CloseSize)
			: Header->GetChildrenCount();
		if (!Header->InsertChildAt(InsertIndex, HelpButton))
		{
			return false;
		}
		if (UHorizontalBoxSlot* Slot = Cast<UHorizontalBoxSlot>(HelpButton->Slot))
		{
			Slot->SetVerticalAlignment(VAlign_Center);
			Slot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
		}
		bChanged = true;
	}

	UOverlay* HelpHost = FindTypedWidget<UOverlay>(Tree, TEXT("ControlsHelpHost"));
	if (!HelpHost)
	{
		HelpHost = Tree.ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ControlsHelpHost"));
		HelpHost->SetVisibility(ESlateVisibility::Collapsed);
		if (UOverlaySlot* Slot = Root->AddChildToOverlay(HelpHost))
		{
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
		}
		bChanged = true;
	}

	UImage* DeleteFocus = FindTypedWidget<UImage>(Tree, TEXT("DeleteTargetFocusIcon"));
	if (!DeleteFocus)
	{
		DeleteFocus = Tree.ConstructWidget<UImage>(UImage::StaticClass(), TEXT("DeleteTargetFocusIcon"));
		FSlateBrush EmptyBrush;
		EmptyBrush.ImageSize = FVector2D(30.0f, 30.0f);
		DeleteFocus->SetBrush(EmptyBrush);
		DeleteFocus->SetVisibility(ESlateVisibility::Collapsed);
		if (UOverlaySlot* Slot = DeleteTarget->AddChildToOverlay(DeleteFocus))
		{
			Slot->SetHorizontalAlignment(HAlign_Right);
			Slot->SetVerticalAlignment(VAlign_Top);
			Slot->SetPadding(FMargin(8.0f));
		}
		bChanged = true;
	}

	if (!EnsureVariable(Blueprint, *HintText, bChanged)
		|| !EnsureVariable(Blueprint, *HelpButton, bChanged)
		|| !EnsureVariable(Blueprint, *HelpHost, bChanged)
		|| !EnsureVariable(Blueprint, *DeleteFocus, bChanged))
	{
		return false;
	}
	bChanged |= EnsureAllWidgetGuids(Blueprint);
	if (bChanged)
	{
		Blueprint.Modify();
		Tree.Modify();
		if (!CompileBlueprint(Blueprint))
		{
			return false;
		}
	}
	if (!SetObjectDefault(Blueprint, TEXT("ControlsHelpWidgetClass"), HelpClass, bChanged))
	{
		return false;
	}
	return !bChanged || Context.MarkChanged(Blueprint);
}

bool PatchDeckCard(FMigrationContext& Context, UWidgetBlueprint& Blueprint)
{
	UWidgetTree& Tree = *Blueprint.WidgetTree;
	UWidget* ScaleBox = Tree.FindWidget(TEXT("CardFaceScaleBox"));
	UOverlay* Host = ScaleBox ? Cast<UOverlay>(ScaleBox->GetParent()) : nullptr;
	if (!Host)
	{
		return false;
	}
	bool bChanged = false;
	auto EnsureIcon = [&](FName Name, EHorizontalAlignment Alignment) -> UImage*
	{
		UImage* Icon = FindTypedWidget<UImage>(Tree, Name);
		if (!Icon)
		{
			Icon = Tree.ConstructWidget<UImage>(UImage::StaticClass(), Name);
			FSlateBrush Brush;
			Brush.ImageSize = FVector2D(32.0f, 32.0f);
			Icon->SetBrush(Brush);
			Icon->SetVisibility(ESlateVisibility::Collapsed);
			if (UOverlaySlot* Slot = Host->AddChildToOverlay(Icon))
			{
				Slot->SetHorizontalAlignment(Alignment);
				Slot->SetVerticalAlignment(VAlign_Top);
				Slot->SetPadding(FMargin(10.0f));
			}
			bChanged = true;
		}
		return Icon;
	};
	UImage* Focus = EnsureIcon(TEXT("WorkspaceFocusIcon"), HAlign_Left);
	UImage* State = EnsureIcon(TEXT("WorkspaceStateIcon"), HAlign_Right);
	if (!Focus || !State
		|| !EnsureVariable(Blueprint, *Focus, bChanged)
		|| !EnsureVariable(Blueprint, *State, bChanged))
	{
		return false;
	}
	bChanged |= EnsureAllWidgetGuids(Blueprint);
	if (!bChanged)
	{
		Context.RegisterAsset(Blueprint);
		return true;
	}
	Blueprint.Modify();
	Tree.Modify();
	return CompileBlueprint(Blueprint) && Context.MarkChanged(Blueprint);
}

bool PatchZonePile(FMigrationContext& Context, UWidgetBlueprint& Blueprint)
{
	UWidgetTree& Tree = *Blueprint.WidgetTree;
	UCanvasPanel* Root = CastChecked<UCanvasPanel>(Tree.RootWidget);
	bool bChanged = false;
	UImage* Focus = FindTypedWidget<UImage>(Tree, TEXT("NavigationFocusIcon"));
	if (!Focus)
	{
		Focus = Tree.ConstructWidget<UImage>(UImage::StaticClass(), TEXT("NavigationFocusIcon"));
		FSlateBrush Brush;
		Brush.ImageSize = FVector2D(32.0f, 32.0f);
		Focus->SetBrush(Brush);
		Focus->SetVisibility(ESlateVisibility::Collapsed);
		if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(Focus))
		{
			Slot->SetAnchors(FAnchors(1.0f, 0.0f));
			Slot->SetAlignment(FVector2D(1.0f, 0.0f));
			Slot->SetPosition(FVector2D(-10.0f, 10.0f));
			Slot->SetSize(FVector2D(32.0f, 32.0f));
			Slot->SetZOrder(50);
		}
		bChanged = true;
	}
	if (!Focus || !EnsureVariable(Blueprint, *Focus, bChanged))
	{
		return false;
	}
	bChanged |= EnsureAllWidgetGuids(Blueprint);
	if (!bChanged)
	{
		Context.RegisterAsset(Blueprint);
		return true;
	}
	Blueprint.Modify();
	Tree.Modify();
	return CompileBlueprint(Blueprint) && Context.MarkChanged(Blueprint);
}

FSlateBrush MakeStateBrush(UTexture2D& Texture)
{
	FSlateBrush Brush;
	Brush.SetResourceObject(&Texture);
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.ImageSize = FVector2D(32.0f, 32.0f);
	Brush.TintColor = FSlateColor(FLinearColor::White);
	return Brush;
}

bool StyleUsesTextures(
	const UWacomBackpackWorkspaceStyle& Style,
	UTexture2D& Focus,
	UTexture2D& Selected,
	UTexture2D& Valid,
	UTexture2D& Rejected)
{
	return Style.AssetVersion == UWacomBackpackWorkspaceStyle::CurrentAssetVersion
		&& Style.FocusStateIconBrush.GetResourceObject() == &Focus
		&& Style.SelectedStateIconBrush.GetResourceObject() == &Selected
		&& Style.ValidDropStateIconBrush.GetResourceObject() == &Valid
		&& Style.RejectedDropStateIconBrush.GetResourceObject() == &Rejected;
}

bool PatchStyle(
	FMigrationContext& Context,
	UWacomBackpackWorkspaceStyle& Style,
	UTexture2D& Focus,
	UTexture2D& Selected,
	UTexture2D& Valid,
	UTexture2D& Rejected)
{
	if (StyleUsesTextures(Style, Focus, Selected, Valid, Rejected))
	{
		Context.RegisterAsset(Style);
		return true;
	}
	if (Style.AssetVersion != 3)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[%s] Refused to overwrite a v4 Style with unexpected icon bindings"),
			CommandletTag);
		return false;
	}
	if (!Context.bApply)
	{
		return true;
	}
	Style.Modify();
	Style.FocusStateIconBrush = MakeStateBrush(Focus);
	Style.SelectedStateIconBrush = MakeStateBrush(Selected);
	Style.ValidDropStateIconBrush = MakeStateBrush(Valid);
	Style.RejectedDropStateIconBrush = MakeStateBrush(Rejected);
	Style.AssetVersion = UWacomBackpackWorkspaceStyle::CurrentAssetVersion;
	Style.PostEditChange();
	return Context.MarkChanged(Style);
}
}

bool MigrateBackpackWorkspaceV4(
	bool bApply,
	FBackpackWorkspaceV4MigrationReport& OutReport)
{
	OutReport = FBackpackWorkspaceV4MigrationReport();
	LogManifestBaseline();

	UWidgetBlueprint* Screen = nullptr;
	UWidgetBlueprint* DeckCard = nullptr;
	UWidgetBlueprint* ZonePile = nullptr;
	UWacomBackpackWorkspaceStyle* Style = nullptr;
	if (!PreflightBlueprints(Screen, DeckCard, ZonePile, Style))
	{
		return false;
	}

	FMigrationContext Context;
	Context.bApply = bApply;
	Context.RegisterAsset(*Screen);
	Context.RegisterAsset(*DeckCard);
	Context.RegisterAsset(*ZonePile);
	Context.RegisterAsset(*Style);

	UTexture2D* Focus = EnsureTexture(
		Context, TEXT("T_BackpackState_Focus"), EAccessibilityGlyph::Focus);
	UTexture2D* Selected = EnsureTexture(
		Context, TEXT("T_BackpackState_Selected"), EAccessibilityGlyph::Selected);
	UTexture2D* Valid = EnsureTexture(
		Context, TEXT("T_BackpackState_ValidDrop"), EAccessibilityGlyph::ValidDrop);
	UTexture2D* Rejected = EnsureTexture(
		Context, TEXT("T_BackpackState_RejectedDrop"), EAccessibilityGlyph::RejectedDrop);
	UWidgetBlueprint* Help = EnsureHelpBlueprint(Context);
	if (Context.bValidationFailed)
	{
		return false;
	}
	if (!bApply && (!Focus || !Selected || !Valid || !Rejected || !Help))
	{
		OutReport.bAlreadyCurrent = false;
		return true;
	}
	if (!Focus || !Selected || !Valid || !Rejected || !Help || !Help->GeneratedClass)
	{
		return false;
	}
	if (!bApply)
	{
		OutReport.bAlreadyCurrent = IsHelpBlueprintCurrent(*Help)
			&& IsDeckCardCurrent(*DeckCard)
			&& IsZonePileCurrent(*ZonePile)
			&& IsScreenCurrent(*Screen, Help->GeneratedClass)
			&& StyleUsesTextures(*Style, *Focus, *Selected, *Valid, *Rejected);
		UE_LOG(LogTemp, Display,
			TEXT("[%s] INSPECT current=%s mutation=false"),
			CommandletTag,
			OutReport.bAlreadyCurrent ? TEXT("true") : TEXT("false"));
		return true;
	}

	if (!PatchDeckCard(Context, *DeckCard)
		|| !PatchZonePile(Context, *ZonePile)
		|| !PatchScreen(Context, *Screen, Help->GeneratedClass)
		|| !PatchStyle(Context, *Style, *Focus, *Selected, *Valid, *Rejected))
	{
		return false;
	}

	OutReport.bAlreadyCurrent = Context.ChangedPackages.IsEmpty();
	for (const FString& PackageName : ExactPackageManifest())
	{
		if (!Context.ChangedPackages.Contains(PackageName))
		{
			continue;
		}
		UObject* const* Asset = Context.TopLevelAssets.Find(PackageName);
		if (!Asset || !*Asset || !SaveExactAsset(**Asset, OutReport))
		{
			return false;
		}
	}
	return OutReport.SavedPackages.Num() == Context.ChangedPackages.Num();
}
}
