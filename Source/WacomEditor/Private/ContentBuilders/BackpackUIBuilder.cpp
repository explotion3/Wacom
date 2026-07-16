// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/BackpackUIBuilder.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/InvalidationBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "HAL/FileManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Materials/MaterialInterface.h"
#include "UI/Backpack/WacomBackpackDeleteConfirmWidget.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UI/Backpack/WacomBackpackZonePileWidget.h"
#include "UI/Backpack/WacomSpecialZoneWidget.h"
#include "UI/Card/WacomFirstPersonCardViewWidget.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintOperationUtils.h"

namespace Wacom::ContentBuilder
{
namespace
{
const TCHAR* BackpackUIRoot = TEXT("/Game/Wacom/UI/Backpack");

FSlateChildSize FillSize(float Value = 1.0f)
{
	FSlateChildSize Result(ESlateSizeRule::Fill);
	Result.Value = Value;
	return Result;
}

FSlateChildSize AutoSize()
{
	return FSlateChildSize(ESlateSizeRule::Automatic);
}

template <typename TWidget>
TWidget* MakeWidget(UWidgetBlueprint& Blueprint, FName Name, bool bVariable = false)
{
	check(Blueprint.WidgetTree);
	TWidget* Widget = Blueprint.WidgetTree->ConstructWidget<TWidget>(TWidget::StaticClass(), Name);
	if (Widget && bVariable)
	{
		FWidgetBlueprintOperationUtils::ToggleWidgetAsVariable(&Blueprint, Widget, true, false);
	}
	return Widget;
}

void StyleText(UTextBlock& Text, const FText& Value, int32 Size, FLinearColor Color)
{
	Text.SetText(Value);
	FSlateFontInfo Font = Text.GetFont();
	Font.Size = Size;
	Text.SetFont(Font);
	Text.SetColorAndOpacity(FSlateColor(Color));
}

UTextBlock* MakeText(
	UWidgetBlueprint& Blueprint,
	FName Name,
	const FText& Value,
	int32 Size,
	FLinearColor Color,
	bool bVariable = false)
{
	UTextBlock* Text = MakeWidget<UTextBlock>(Blueprint, Name, bVariable);
	if (Text)
	{
		StyleText(*Text, Value, Size, Color);
	}
	return Text;
}

UButton* MakeLabeledButton(
	UWidgetBlueprint& Blueprint,
	FName Name,
	const FText& Label,
	FLinearColor Background,
	bool bVariable = false)
{
	UButton* Button = MakeWidget<UButton>(Blueprint, Name, bVariable);
	if (!Button)
	{
		return nullptr;
	}
	Button->SetBackgroundColor(Background);
	UTextBlock* LabelText = MakeText(
		Blueprint,
		*FString::Printf(TEXT("%sLabel"), *Name.ToString()),
		Label,
		17,
		FLinearColor(0.94f, 0.91f, 0.82f, 1.0f));
	Button->AddChild(LabelText);
	return Button;
}

FString PackagePath(const TCHAR* AssetName, const TCHAR* RootPath = BackpackUIRoot)
{
	return FString(RootPath) / AssetName;
}

FString ObjectPath(const TCHAR* AssetName, const TCHAR* RootPath = BackpackUIRoot)
{
	const FString Path = PackagePath(AssetName, RootPath);
	return Path + TEXT(".") + AssetName;
}

bool SaveTopLevelAsset(UObject& Asset)
{
	UPackage* Package = Asset.GetOutermost();
	if (!Package)
	{
		return false;
	}
	Package->MarkPackageDirty();
	Asset.MarkPackageDirty();
	const FString Filename = FPackageName::LongPackageNameToFilename(
		Package->GetName(), FPackageName::GetAssetPackageExtension());
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	Args.SaveFlags = SAVE_NoError;
	return UPackage::SavePackage(Package, &Asset, *Filename, Args);
}

void ResetWidgetBlueprintContent(UWidgetBlueprint& Blueprint)
{
	if (Blueprint.WidgetTree)
	{
		UWidgetTree* OldTree = Blueprint.WidgetTree;
		const FName RetiredName = MakeUniqueObjectName(
			GetTransientPackage(), UWidgetTree::StaticClass(), TEXT("RetiredBackpackWidgetTree"));
		OldTree->Rename(
			*RetiredName.ToString(),
			GetTransientPackage(),
			REN_DontCreateRedirectors | REN_DoNotDirty | REN_NonTransactional);
		if (OldTree->IsRooted())
		{
			OldTree->RemoveFromRoot();
		}
		OldTree->MarkAsGarbage();
	}
	Blueprint.WidgetTree = NewObject<UWidgetTree>(&Blueprint, TEXT("WidgetTree"), RF_Transactional);
	Blueprint.Bindings.Reset();
	Blueprint.Animations.Reset();
	// The whole tree is rebuilt.  Let the compiler derive stable widget GUIDs from
	// the new deterministic object paths instead of preserving retired names.
	Blueprint.WidgetVariableNameToGuidMap.Reset();
	Blueprint.bCanCallInitializedWithoutPlayerContext = true;
}

UWidgetBlueprint* LoadOrCreateWidgetBlueprint(
	const TCHAR* AssetName,
	UClass* ParentClass,
	bool bResetContent = true,
	const TCHAR* RootPath = BackpackUIRoot)
{
	UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, *ObjectPath(AssetName, RootPath));
	if (!Blueprint)
	{
		UPackage* Package = CreatePackage(*PackagePath(AssetName, RootPath));
		if (!Package)
		{
			return nullptr;
		}
		Package->FullyLoad();
		Blueprint = FWidgetBlueprintOperationUtils::CreateWidgetBlueprint(
			Package,
			AssetName,
			BPTYPE_Normal,
			ParentClass,
			nullptr,
			TEXT("WacomBuildBackpackUI"),
			true);
	}
	if (!Blueprint)
	{
		return nullptr;
	}
	if (Blueprint->ParentClass != ParentClass)
	{
		Blueprint->ParentClass = ParentClass;
	}
	if (bResetContent)
	{
		ResetWidgetBlueprintContent(*Blueprint);
	}
	return Blueprint;
}

bool CompileWidgetBlueprint(UWidgetBlueprint& Blueprint)
{
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(&Blueprint);
	FKismetEditorUtilities::CompileBlueprint(&Blueprint);
	if (Blueprint.Status == BS_Error || !Blueprint.GeneratedClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[BackpackUIBuilder] Compile failed: %s"), *Blueprint.GetPathName());
		return false;
	}
	return true;
}

bool SetObjectDefault(UWidgetBlueprint& Blueprint, FName PropertyName, UObject* Value)
{
	UObject* CDO = Blueprint.GeneratedClass ? Blueprint.GeneratedClass->GetDefaultObject() : nullptr;
	FObjectPropertyBase* Property = CDO
		? FindFProperty<FObjectPropertyBase>(CDO->GetClass(), PropertyName)
		: nullptr;
	if (!CDO || !Property)
	{
		UE_LOG(LogTemp, Error, TEXT("[BackpackUIBuilder] Missing CDO property %s on %s"),
			*PropertyName.ToString(), *Blueprint.GetPathName());
		return false;
	}
	CDO->Modify();
	Property->SetObjectPropertyValue_InContainer(CDO, Value);
	CDO->MarkPackageDirty();
	return true;
}

struct FBackpackCardFaceSlotLayout
{
	FMargin Padding;
	EHorizontalAlignment HorizontalAlignment = HAlign_Fill;
	EVerticalAlignment VerticalAlignment = VAlign_Fill;
	bool bSupported = false;
};

FBackpackCardFaceSlotLayout CaptureCardFaceSlotLayout(const UPanelSlot* Slot)
{
	FBackpackCardFaceSlotLayout Layout;
	if (const UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(Slot))
	{
		Layout.Padding = OverlaySlot->GetPadding();
		Layout.HorizontalAlignment = OverlaySlot->GetHorizontalAlignment();
		Layout.VerticalAlignment = OverlaySlot->GetVerticalAlignment();
		Layout.bSupported = true;
	}
	else if (const UBorderSlot* BorderSlot = Cast<UBorderSlot>(Slot))
	{
		Layout.Padding = BorderSlot->GetPadding();
		Layout.HorizontalAlignment = BorderSlot->GetHorizontalAlignment();
		Layout.VerticalAlignment = BorderSlot->GetVerticalAlignment();
		Layout.bSupported = true;
	}
	return Layout;
}

bool ApplyCardFaceSlotLayout(UPanelSlot* Slot, const FBackpackCardFaceSlotLayout& Layout)
{
	if (!Layout.bSupported)
	{
		return false;
	}
	if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(Slot))
	{
		OverlaySlot->SetPadding(Layout.Padding);
		OverlaySlot->SetHorizontalAlignment(Layout.HorizontalAlignment);
		OverlaySlot->SetVerticalAlignment(Layout.VerticalAlignment);
		return true;
	}
	if (UBorderSlot* BorderSlot = Cast<UBorderSlot>(Slot))
	{
		BorderSlot->SetPadding(Layout.Padding);
		BorderSlot->SetHorizontalAlignment(Layout.HorizontalAlignment);
		BorderSlot->SetVerticalAlignment(Layout.VerticalAlignment);
		return true;
	}
	return false;
}

bool PatchBackpackDeckCardFace()
{
	constexpr float BackpackCardFaceScale = 0.75f;
	const TCHAR* DeckCardObjectPath =
		TEXT("/Game/Wacom/UI/Card/WBP_WacomDeckCardWidget.WBP_WacomDeckCardWidget");
	const TCHAR* CardFaceClassPath =
		TEXT("/Game/Wacom/UI/Card/WBP_FPCardView.WBP_FPCardView_C");
	UWidgetBlueprint* DeckCardBlueprint = LoadObject<UWidgetBlueprint>(nullptr, DeckCardObjectPath);
	UClass* CardFaceClass = LoadObject<UClass>(nullptr, CardFaceClassPath);
	if (!DeckCardBlueprint || !DeckCardBlueprint->WidgetTree || !CardFaceClass
		|| !CardFaceClass->IsChildOf(UWacomFirstPersonCardViewWidget::StaticClass()))
	{
		UE_LOG(LogTemp, Error, TEXT("[BackpackUIBuilder] Missing DeckCard or authored card face asset"));
		return false;
	}

	UWidget* ExistingCardFaceWidget = DeckCardBlueprint->WidgetTree->FindWidget(TEXT("BackpackCardView"));
	if (!ExistingCardFaceWidget)
	{
		ExistingCardFaceWidget = DeckCardBlueprint->WidgetTree->FindWidget(TEXT("CardView"));
	}
	if (!ExistingCardFaceWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("[BackpackUIBuilder] DeckCard does not provide a card face"));
		return false;
	}

	UScaleBox* CardFaceScaleBox = Cast<UScaleBox>(ExistingCardFaceWidget->GetParent());
	UBorder* WorkspaceFeedbackOverlay = Cast<UBorder>(
		DeckCardBlueprint->WidgetTree->FindWidget(TEXT("WorkspaceFeedbackOverlay")));
	const UScaleBoxSlot* ExistingScaleSlot = Cast<UScaleBoxSlot>(ExistingCardFaceWidget->Slot);
	const UOverlaySlot* ExistingFeedbackSlot = WorkspaceFeedbackOverlay
		? Cast<UOverlaySlot>(WorkspaceFeedbackOverlay->Slot)
		: nullptr;
	const bool bHasFormalScaleContract = CardFaceScaleBox
		&& CardFaceScaleBox->GetFName() == TEXT("CardFaceScaleBox")
		&& CardFaceScaleBox->bIsVariable
		&& DeckCardBlueprint->WidgetVariableNameToGuidMap.Contains(CardFaceScaleBox->GetFName())
		&& CardFaceScaleBox->GetStretch() == EStretch::UserSpecified
		&& FMath::IsNearlyEqual(CardFaceScaleBox->GetUserSpecifiedScale(), BackpackCardFaceScale)
		&& ExistingScaleSlot
		&& ExistingScaleSlot->GetHorizontalAlignment() == HAlign_Center
		&& ExistingScaleSlot->GetVerticalAlignment() == VAlign_Center;
	const bool bHasFormalFeedbackContract = WorkspaceFeedbackOverlay
		&& WorkspaceFeedbackOverlay->bIsVariable
		&& DeckCardBlueprint->WidgetVariableNameToGuidMap.Contains(WorkspaceFeedbackOverlay->GetFName())
		&& CardFaceScaleBox
		&& WorkspaceFeedbackOverlay->GetParent() == CardFaceScaleBox->GetParent()
		&& WorkspaceFeedbackOverlay->GetParent()->GetChildIndex(WorkspaceFeedbackOverlay)
			== WorkspaceFeedbackOverlay->GetParent()->GetChildIndex(CardFaceScaleBox) + 1
		&& ExistingFeedbackSlot
		&& ExistingFeedbackSlot->GetHorizontalAlignment() == HAlign_Fill
		&& ExistingFeedbackSlot->GetVerticalAlignment() == VAlign_Fill
		&& WorkspaceFeedbackOverlay->GetVisibility() == ESlateVisibility::Collapsed;
	if (ExistingCardFaceWidget->GetClass() == CardFaceClass
		&& ExistingCardFaceWidget->GetFName() == TEXT("BackpackCardView")
		&& ExistingCardFaceWidget->GetVisibility() == ESlateVisibility::HitTestInvisible
		&& DeckCardBlueprint->WidgetVariableNameToGuidMap.Contains(TEXT("BackpackCardView"))
		&& !DeckCardBlueprint->WidgetVariableNameToGuidMap.Contains(TEXT("CardView"))
		&& bHasFormalScaleContract
		&& bHasFormalFeedbackContract)
	{
		return true;
	}

	DeckCardBlueprint->Modify();
	DeckCardBlueprint->WidgetTree->Modify();
	if (!CardFaceScaleBox)
	{
		UPanelWidget* Parent = ExistingCardFaceWidget->GetParent();
		const int32 ChildIndex = Parent ? Parent->GetChildIndex(ExistingCardFaceWidget) : INDEX_NONE;
		const FBackpackCardFaceSlotLayout SlotLayout = CaptureCardFaceSlotLayout(ExistingCardFaceWidget->Slot);
		if (!Parent || ChildIndex == INDEX_NONE || !SlotLayout.bSupported)
		{
			UE_LOG(LogTemp, Error, TEXT("[BackpackUIBuilder] DeckCard CardView uses an unsupported parent slot"));
			return false;
		}

		Parent->Modify();
		Parent->RemoveChild(ExistingCardFaceWidget);
		CardFaceScaleBox = DeckCardBlueprint->WidgetTree->ConstructWidget<UScaleBox>(
			UScaleBox::StaticClass(), TEXT("CardFaceScaleBox"));
		if (!CardFaceScaleBox || !Parent->InsertChildAt(ChildIndex, CardFaceScaleBox)
			|| !ApplyCardFaceSlotLayout(CardFaceScaleBox->Slot, SlotLayout))
		{
			UE_LOG(LogTemp, Error, TEXT("[BackpackUIBuilder] Failed to install the DeckCard face scale host"));
			return false;
		}
		CardFaceScaleBox->SetContent(ExistingCardFaceWidget);
	}

	CardFaceScaleBox->Modify();
	CardFaceScaleBox->SetStretch(EStretch::UserSpecified);
	CardFaceScaleBox->SetStretchDirection(EStretchDirection::Both);
	CardFaceScaleBox->SetUserSpecifiedScale(BackpackCardFaceScale);
	CardFaceScaleBox->SetClipping(EWidgetClipping::Inherit);

	if (ExistingCardFaceWidget->GetClass() != CardFaceClass
		|| ExistingCardFaceWidget->GetFName() != TEXT("BackpackCardView"))
	{
		CardFaceScaleBox->RemoveChild(ExistingCardFaceWidget);
		DeckCardBlueprint->WidgetTree->RemoveWidget(ExistingCardFaceWidget);
		const FName RetiredName = MakeUniqueObjectName(
			GetTransientPackage(), ExistingCardFaceWidget->GetClass(), TEXT("RetiredBackpackCardFace"));
		ExistingCardFaceWidget->Rename(
			*RetiredName.ToString(),
			GetTransientPackage(),
			REN_DontCreateRedirectors | REN_DoNotDirty | REN_NonTransactional);

		ExistingCardFaceWidget = DeckCardBlueprint->WidgetTree->ConstructWidget<UWacomFirstPersonCardViewWidget>(
			CardFaceClass, TEXT("BackpackCardView"));
		if (!ExistingCardFaceWidget || !CardFaceScaleBox->SetContent(ExistingCardFaceWidget))
		{
			UE_LOG(LogTemp, Error, TEXT("[BackpackUIBuilder] Failed to install authored DeckCard CardView"));
			return false;
		}
	}

	UWacomFirstPersonCardViewWidget* ExistingCardFace =
		Cast<UWacomFirstPersonCardViewWidget>(ExistingCardFaceWidget);
	if (ExistingCardFace)
	{
		ExistingCardFace->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	UScaleBoxSlot* CardFaceSlot = ExistingCardFace
		? Cast<UScaleBoxSlot>(ExistingCardFace->Slot)
		: nullptr;
	if (!CardFaceSlot)
	{
		UE_LOG(LogTemp, Error, TEXT("[BackpackUIBuilder] DeckCard CardView did not receive a ScaleBox slot"));
		return false;
	}
	CardFaceSlot->SetHorizontalAlignment(HAlign_Center);
	CardFaceSlot->SetVerticalAlignment(VAlign_Center);

	UOverlay* CardOverlayHost = Cast<UOverlay>(CardFaceScaleBox->GetParent());
	if (!CardOverlayHost)
	{
		UE_LOG(LogTemp, Error, TEXT("[BackpackUIBuilder] DeckCard face scale host must be a direct Overlay child"));
		return false;
	}
	if (!WorkspaceFeedbackOverlay)
	{
		WorkspaceFeedbackOverlay = DeckCardBlueprint->WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("WorkspaceFeedbackOverlay"));
	}
	if (!WorkspaceFeedbackOverlay)
	{
		UE_LOG(LogTemp, Error, TEXT("[BackpackUIBuilder] Failed to create workspace card feedback overlay"));
		return false;
	}
	if (UPanelWidget* PreviousParent = WorkspaceFeedbackOverlay->GetParent())
	{
		PreviousParent->RemoveChild(WorkspaceFeedbackOverlay);
	}
	const int32 FeedbackIndex = CardOverlayHost->GetChildIndex(CardFaceScaleBox) + 1;
	if (FeedbackIndex <= 0 || !CardOverlayHost->InsertChildAt(FeedbackIndex, WorkspaceFeedbackOverlay))
	{
		UE_LOG(LogTemp, Error, TEXT("[BackpackUIBuilder] Failed to place workspace feedback above the card face"));
		return false;
	}
	UOverlaySlot* FeedbackSlot = Cast<UOverlaySlot>(WorkspaceFeedbackOverlay->Slot);
	if (!FeedbackSlot)
	{
		UE_LOG(LogTemp, Error, TEXT("[BackpackUIBuilder] Workspace feedback did not receive an Overlay slot"));
		return false;
	}
	FeedbackSlot->SetPadding(FMargin(0.0f));
	FeedbackSlot->SetHorizontalAlignment(HAlign_Fill);
	FeedbackSlot->SetVerticalAlignment(VAlign_Fill);
	WorkspaceFeedbackOverlay->SetBrushColor(FLinearColor::Transparent);
	WorkspaceFeedbackOverlay->SetVisibility(ESlateVisibility::Collapsed);
	if (!DeckCardBlueprint->WidgetVariableNameToGuidMap.Contains(CardFaceScaleBox->GetFName()))
	{
		DeckCardBlueprint->WidgetVariableNameToGuidMap.Emplace(
			CardFaceScaleBox->GetFName(),
			FGuid::NewDeterministicGuid(CardFaceScaleBox->GetPathName()));
	}
	if (!DeckCardBlueprint->WidgetVariableNameToGuidMap.Contains(WorkspaceFeedbackOverlay->GetFName()))
	{
		DeckCardBlueprint->WidgetVariableNameToGuidMap.Emplace(
			WorkspaceFeedbackOverlay->GetFName(),
			FGuid::NewDeterministicGuid(WorkspaceFeedbackOverlay->GetPathName()));
	}
	DeckCardBlueprint->WidgetVariableNameToGuidMap.Remove(TEXT("CardView"));
	if (!DeckCardBlueprint->WidgetVariableNameToGuidMap.Contains(ExistingCardFace->GetFName()))
	{
		DeckCardBlueprint->WidgetVariableNameToGuidMap.Emplace(
			ExistingCardFace->GetFName(),
			FGuid::NewDeterministicGuid(ExistingCardFace->GetPathName()));
	}
	FWidgetBlueprintOperationUtils::ToggleWidgetAsVariable(
		DeckCardBlueprint, CardFaceScaleBox, true, false);
	FWidgetBlueprintOperationUtils::ToggleWidgetAsVariable(
		DeckCardBlueprint, WorkspaceFeedbackOverlay, true, false);
	FWidgetBlueprintOperationUtils::ToggleWidgetAsVariable(
		DeckCardBlueprint, ExistingCardFace, true, false);
	if (!CompileWidgetBlueprint(*DeckCardBlueprint) || !SaveTopLevelAsset(*DeckCardBlueprint))
	{
		return false;
	}

	UE_LOG(LogTemp, Display,
		TEXT("[BackpackUIBuilder] DeckCard now scales WBP_FPCardView uniformly at %.4f"),
		BackpackCardFaceScale);
	return true;
}

constexpr TCHAR ZonePileBuilderContract[] = TEXT("Wacom.Backpack.ZonePile.RealCards.v1");

bool IsZonePileBlueprintCurrent(const UWidgetBlueprint& Blueprint)
{
	if (Blueprint.ParentClass != UWacomBackpackZonePileWidget::StaticClass()
		|| Blueprint.BlueprintDescription != ZonePileBuilderContract
		|| !Blueprint.WidgetTree
		|| !Cast<UCanvasPanel>(Blueprint.WidgetTree->RootWidget))
	{
		return false;
	}
	UWidgetTree* Tree = Blueprint.WidgetTree;
	const UBorder* Frame = Cast<UBorder>(Tree->FindWidget(TEXT("FrameBorder")));
	const UBorder* DragHandle = Cast<UBorder>(Tree->FindWidget(TEXT("DragHandle")));
	const UTextBlock* Title = Cast<UTextBlock>(Tree->FindWidget(TEXT("TitleText")));
	const UTextBlock* Count = Cast<UTextBlock>(Tree->FindWidget(TEXT("CountText")));
	const UTextBlock* Status = Cast<UTextBlock>(Tree->FindWidget(TEXT("StatusText")));
	const UBorder* DropFeedback = Cast<UBorder>(Tree->FindWidget(TEXT("DropFeedback")));
	return Frame && Frame->bIsVariable
		&& DragHandle && DragHandle->bIsVariable
		&& Title && Title->bIsVariable
		&& Count && Count->bIsVariable
		&& Status && Status->bIsVariable
		&& DropFeedback && DropFeedback->bIsVariable
		&& !Tree->FindWidget(TEXT("PreviewHost"));
}

bool BuildZonePileBlueprint(UWidgetBlueprint& Blueprint)
{
	Blueprint.BlueprintDescription = ZonePileBuilderContract;
	UCanvasPanel* Root = MakeWidget<UCanvasPanel>(Blueprint, TEXT("PileRoot"));
	Blueprint.WidgetTree->RootWidget = Root;

	UBorder* Frame = MakeWidget<UBorder>(Blueprint, TEXT("FrameBorder"), true);
	Frame->SetBrushColor(FLinearColor(0.025f, 0.04f, 0.065f, 0.74f));
	Frame->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(Frame))
	{
		Slot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		Slot->SetOffsets(FMargin(0.0f));
	}

	UBorder* DragHandle = MakeWidget<UBorder>(Blueprint, TEXT("DragHandle"), true);
	DragHandle->SetPadding(FMargin(10.0f, 7.0f));
	DragHandle->SetBrushColor(FLinearColor(0.08f, 0.13f, 0.19f, 1.0f));
	if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(DragHandle))
	{
		Slot->SetPosition(FVector2D::ZeroVector);
		Slot->SetSize(FVector2D(260.0f, 48.0f));
		Slot->SetZOrder(2);
	}
	UHorizontalBox* Header = MakeWidget<UHorizontalBox>(Blueprint, TEXT("Header"));
	DragHandle->AddChild(Header);
	UTextBlock* Title = MakeText(
		Blueprint, TEXT("TitleText"),
		NSLOCTEXT("BackpackUIBuilder", "PileTitle", "区域牌堆"),
		18, FLinearColor(0.92f, 0.95f, 0.97f, 1.0f), true);
	if (UHorizontalBoxSlot* Slot = Header->AddChildToHorizontalBox(Title))
	{
		Slot->SetSize(FillSize());
		Slot->SetVerticalAlignment(VAlign_Center);
	}
	UTextBlock* Count = MakeText(
		Blueprint, TEXT("CountText"), FText::FromString(TEXT("0")),
		17, FLinearColor(0.55f, 0.86f, 0.98f, 1.0f), true);
	if (UHorizontalBoxSlot* Slot = Header->AddChildToHorizontalBox(Count))
	{
		Slot->SetSize(AutoSize());
		Slot->SetVerticalAlignment(VAlign_Center);
	}

	UTextBlock* Status = MakeText(
		Blueprint, TEXT("StatusText"),
		NSLOCTEXT("BackpackUIBuilder", "PileExpandHint", "点击牌堆展开"),
		14, FLinearColor(0.62f, 0.70f, 0.78f, 1.0f), true);
	Status->SetJustification(ETextJustify::Center);
	Status->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(Status))
	{
		Slot->SetAnchors(FAnchors(0.0f, 1.0f, 1.0f, 1.0f));
		Slot->SetAlignment(FVector2D(0.0f, 1.0f));
		Slot->SetOffsets(FMargin(8.0f, -32.0f, 8.0f, 28.0f));
		Slot->SetZOrder(2);
	}

	UBorder* DropFeedback = MakeWidget<UBorder>(Blueprint, TEXT("DropFeedback"), true);
	DropFeedback->SetBrushColor(FLinearColor(0.12f, 0.85f, 0.42f, 0.72f));
	DropFeedback->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(DropFeedback))
	{
		Slot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		Slot->SetOffsets(FMargin(0.0f));
		Slot->SetZOrder(3);
	}
	return true;
}

bool BuildWorkspaceBlueprint(UWidgetBlueprint& Blueprint)
{
	UCanvasPanel* Root = MakeWidget<UCanvasPanel>(Blueprint, TEXT("WorkspaceRoot"));
	Blueprint.WidgetTree->RootWidget = Root;

	UBorder* Background = MakeWidget<UBorder>(Blueprint, TEXT("WorkspaceBackground"));
	Background->SetBrushColor(FLinearColor(0.018f, 0.027f, 0.043f, 1.0f));
	if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(Background))
	{
		Slot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		Slot->SetOffsets(FMargin(0.0f));
		Slot->SetZOrder(-10);
	}

	UCanvasPanel* WorkspaceCanvas = MakeWidget<UCanvasPanel>(Blueprint, TEXT("WorkspaceCanvas"), true);
	if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(WorkspaceCanvas))
	{
		Slot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		Slot->SetOffsets(FMargin(0.0f));
	}
	auto AddWorkspaceLayer = [&Blueprint, WorkspaceCanvas](FName Name, int32 ZOrder)
	{
		UCanvasPanel* Layer = MakeWidget<UCanvasPanel>(Blueprint, Name, true);
		Layer->SetClipping(EWidgetClipping::Inherit);
		if (UCanvasPanelSlot* Slot = WorkspaceCanvas->AddChildToCanvas(Layer))
		{
			Slot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			Slot->SetOffsets(FMargin(0.0f));
			Slot->SetZOrder(ZOrder);
		}
		return Layer;
	};
	AddWorkspaceLayer(TEXT("PileFrameLayer"), 1000);
	AddWorkspaceLayer(TEXT("StaticCardLayer"), 2000);
	UCanvasPanel* MarqueeLayer = AddWorkspaceLayer(TEXT("MarqueeLayer"), 9000);
	UCanvasPanel* CarryRoot = AddWorkspaceLayer(TEXT("CarryRoot"), 10000);
	if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(CarryRoot->Slot))
	{
		Slot->SetAnchors(FAnchors(0.0f, 0.0f));
		Slot->SetAlignment(FVector2D::ZeroVector);
		Slot->SetPosition(FVector2D::ZeroVector);
		Slot->SetSize(FVector2D(1.0f, 1.0f));
		Slot->SetZOrder(10000);
	}
	UInvalidationBox* CarryCache = MakeWidget<UInvalidationBox>(
		Blueprint, TEXT("CarryCache"), true);
	CarryCache->SetCanCache(true);
	if (UCanvasPanelSlot* Slot = CarryRoot->AddChildToCanvas(CarryCache))
	{
		Slot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		Slot->SetOffsets(FMargin(0.0f));
		Slot->SetZOrder(0);
	}
	UCanvasPanel* CarryLayer = MakeWidget<UCanvasPanel>(
		Blueprint, TEXT("CarryLayer"), true);
	CarryLayer->SetClipping(EWidgetClipping::Inherit);
	CarryCache->SetContent(CarryLayer);
	UCanvasPanel* CarryActiveLayer = MakeWidget<UCanvasPanel>(
		Blueprint, TEXT("CarryActiveLayer"), true);
	CarryActiveLayer->SetClipping(EWidgetClipping::Inherit);
	if (UCanvasPanelSlot* Slot = CarryRoot->AddChildToCanvas(CarryActiveLayer))
	{
		Slot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		Slot->SetOffsets(FMargin(0.0f));
		Slot->SetZOrder(1);
	}

	UBorder* Marquee = MakeWidget<UBorder>(Blueprint, TEXT("SelectionMarquee"), true);
	Marquee->SetBrushColor(FLinearColor(0.12f, 0.76f, 0.96f, 0.24f));
	Marquee->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* Slot = MarqueeLayer->AddChildToCanvas(Marquee))
	{
		Slot->SetPosition(FVector2D::ZeroVector);
		Slot->SetSize(FVector2D::ZeroVector);
		Slot->SetZOrder(100000);
	}

	UTextBlock* Empty = MakeText(
		Blueprint,
		TEXT("EmptyStateText"),
		NSLOCTEXT("BackpackUIBuilder", "EmptyWorkspace", "通量区暂无卡牌"),
		20,
		FLinearColor(0.48f, 0.57f, 0.66f, 1.0f),
		true);
	Empty->SetJustification(ETextJustify::Center);
	if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(Empty))
	{
		Slot->SetAnchors(FAnchors(0.5f, 0.5f));
		Slot->SetAlignment(FVector2D(0.5f, 0.5f));
		Slot->SetAutoSize(true);
		Slot->SetZOrder(100001);
	}
	return true;
}

bool BuildSpecialZoneBlueprint(UWidgetBlueprint& Blueprint)
{
	const FLinearColor TextPrimary(0.92f, 0.95f, 0.97f, 1.0f);
	const FLinearColor TextSecondary(0.36f, 0.78f, 0.91f, 1.0f);

	UBorder* Root = MakeWidget<UBorder>(Blueprint, TEXT("SpecialZoneBorder"));
	Root->SetPadding(FMargin(14.0f, 12.0f));
	Root->SetBrushColor(FLinearColor(0.035f, 0.049f, 0.071f, 1.0f));
	Blueprint.WidgetTree->RootWidget = Root;

	UVerticalBox* Content = MakeWidget<UVerticalBox>(Blueprint, TEXT("SpecialZoneContent"));
	if (UBorderSlot* Slot = Cast<UBorderSlot>(Root->AddChild(Content)))
	{
		Slot->SetHorizontalAlignment(HAlign_Fill);
		Slot->SetVerticalAlignment(VAlign_Fill);
	}

	UHorizontalBox* TitleRow = MakeWidget<UHorizontalBox>(Blueprint, TEXT("SpecialZoneTitleRow"));
	UVerticalBoxSlot* TitleRowSlot = Content->AddChildToVerticalBox(TitleRow);
	TitleRowSlot->SetSize(AutoSize());
	TitleRowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	UTextBlock* Title = MakeText(
		Blueprint,
		TEXT("TitleText"),
		NSLOCTEXT("BackpackUIBuilder", "SpecialZoneTitle", "特殊存放区"),
		18,
		TextPrimary,
		true);
	TitleRow->AddChildToHorizontalBox(Title)->SetSize(FillSize());
	UTextBlock* Badge = MakeText(
		Blueprint,
		TEXT("BattleReadyBadge"),
		NSLOCTEXT("BackpackUIBuilder", "SpecialZoneBattleReady", "已入战"),
		14,
		TextSecondary,
		true);
	Badge->SetVisibility(ESlateVisibility::Collapsed);
	if (UHorizontalBoxSlot* Slot = TitleRow->AddChildToHorizontalBox(Badge))
	{
		Slot->SetSize(AutoSize());
		Slot->SetVerticalAlignment(VAlign_Center);
		Slot->SetPadding(FMargin(12.0f, 0.0f, 0.0f, 0.0f));
	}

	UTextBlock* OwnerLabel = MakeText(
		Blueprint,
		TEXT("OwnerCardLabel"),
		NSLOCTEXT("BackpackUIBuilder", "SpecialZoneOwnerCard", "主卡"),
		14,
		TextSecondary);
	Content->AddChildToVerticalBox(OwnerLabel)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	UWrapBox* OwnerHost = MakeWidget<UWrapBox>(Blueprint, TEXT("OwnerCardHost"), true);
	OwnerHost->SetInnerSlotPadding(FVector2D(8.0f, 8.0f));
	Content->AddChildToVerticalBox(OwnerHost)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));

	UBorder* ContentHost = MakeWidget<UBorder>(Blueprint, TEXT("ContentDropTargetHost"), true);
	ContentHost->SetPadding(FMargin(10.0f));
	ContentHost->SetBrushColor(FLinearColor(0.018f, 0.027f, 0.043f, 1.0f));
	UVerticalBoxSlot* ContentHostSlot = Content->AddChildToVerticalBox(ContentHost);
	ContentHostSlot->SetSize(FillSize());
	ContentHostSlot->SetHorizontalAlignment(HAlign_Fill);
	ContentHostSlot->SetVerticalAlignment(VAlign_Fill);
	UWrapBox* ContentCards = MakeWidget<UWrapBox>(Blueprint, TEXT("ContentCardsBox"), true);
	ContentCards->SetInnerSlotPadding(FVector2D(8.0f, 8.0f));
	if (UBorderSlot* Slot = Cast<UBorderSlot>(ContentHost->AddChild(ContentCards)))
	{
		Slot->SetHorizontalAlignment(HAlign_Fill);
		Slot->SetVerticalAlignment(VAlign_Fill);
	}
	return true;
}

bool BuildDeleteConfirmBlueprint(UWidgetBlueprint& Blueprint)
{
	UOverlay* Root = MakeWidget<UOverlay>(Blueprint, TEXT("DeleteConfirmRoot"));
	Blueprint.WidgetTree->RootWidget = Root;

	UBorder* Dim = MakeWidget<UBorder>(Blueprint, TEXT("ModalDim"));
	Dim->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.72f));
	Root->AddChildToOverlay(Dim);

	USizeBox* PanelSize = MakeWidget<USizeBox>(Blueprint, TEXT("ModalPanelSize"));
	PanelSize->SetWidthOverride(560.0f);
	PanelSize->SetHeightOverride(220.0f);
	if (UOverlaySlot* Slot = Root->AddChildToOverlay(PanelSize))
	{
		Slot->SetHorizontalAlignment(HAlign_Center);
		Slot->SetVerticalAlignment(VAlign_Center);
	}
	UBorder* Panel = MakeWidget<UBorder>(Blueprint, TEXT("ModalPanel"));
	Panel->SetPadding(FMargin(28.0f, 24.0f));
	Panel->SetBrushColor(FLinearColor(0.035f, 0.049f, 0.071f, 1.0f));
	PanelSize->AddChild(Panel);
	UVerticalBox* Content = MakeWidget<UVerticalBox>(Blueprint, TEXT("ModalContent"));
	Panel->AddChild(Content);

	UTextBlock* Summary = MakeText(
		Blueprint,
		TEXT("SummaryText"),
		NSLOCTEXT("BackpackUIBuilder", "DeleteSummaryPlaceholder", "确认永久销毁这些卡牌？"),
		20,
		FLinearColor(0.94f, 0.91f, 0.82f, 1.0f),
		true);
	Summary->SetAutoWrapText(true);
	Summary->SetJustification(ETextJustify::Center);
	Content->AddChildToVerticalBox(Summary)->SetSize(FillSize());

	UHorizontalBox* Buttons = MakeWidget<UHorizontalBox>(Blueprint, TEXT("ModalButtons"));
	UVerticalBoxSlot* ButtonsSlot = Content->AddChildToVerticalBox(Buttons);
	ButtonsSlot->SetSize(AutoSize());
	ButtonsSlot->SetHorizontalAlignment(HAlign_Center);
	ButtonsSlot->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 0.0f));
	UButton* Confirm = MakeLabeledButton(
		Blueprint,
		TEXT("ConfirmButton"),
		NSLOCTEXT("BackpackUIBuilder", "ConfirmDelete", "确认销毁"),
		FLinearColor(0.68f, 0.12f, 0.10f, 1.0f),
		true);
	UButton* Cancel = MakeLabeledButton(
		Blueprint,
		TEXT("CancelButton"),
		NSLOCTEXT("BackpackUIBuilder", "CancelDelete", "取消"),
		FLinearColor(0.15f, 0.19f, 0.24f, 1.0f),
		true);
	Buttons->AddChildToHorizontalBox(Confirm)->SetPadding(FMargin(8.0f));
	Buttons->AddChildToHorizontalBox(Cancel)->SetPadding(FMargin(8.0f));
	return true;
}

bool BuildScreenBlueprint(UWidgetBlueprint& Blueprint)
{
	const FLinearColor TextPrimary(0.92f, 0.95f, 0.97f, 1.0f);
	const FLinearColor TextSecondary(0.36f, 0.78f, 0.91f, 1.0f);
	// CommonUI GameMenu layer 分配全屏 geometry，项目级 1920×1080 DPI 曲线是唯一
	// 全屏缩放来源。背包内部保持流式 Fill，不再叠加固定设计面或第二套 ScaleToFit。
	UOverlay* RootFrame = MakeWidget<UOverlay>(Blueprint, TEXT("RootFrame"));
	Blueprint.WidgetTree->RootWidget = RootFrame;
	UOverlay* Root = MakeWidget<UOverlay>(Blueprint, TEXT("Root"));
	if (UOverlaySlot* Slot = RootFrame->AddChildToOverlay(Root))
	{
		Slot->SetHorizontalAlignment(HAlign_Fill);
		Slot->SetVerticalAlignment(VAlign_Fill);
	}

	UBorder* Background = MakeWidget<UBorder>(Blueprint, TEXT("Background"));
	Background->SetBrushColor(FLinearColor(0.008f, 0.013f, 0.022f, 0.985f));
	if (UOverlaySlot* Slot = Root->AddChildToOverlay(Background))
	{
		Slot->SetHorizontalAlignment(HAlign_Fill);
		Slot->SetVerticalAlignment(VAlign_Fill);
	}

	UVerticalBox* Main = MakeWidget<UVerticalBox>(Blueprint, TEXT("MainLayout"));
	if (UOverlaySlot* Slot = Root->AddChildToOverlay(Main))
	{
		Slot->SetPadding(FMargin(28.0f, 24.0f));
		Slot->SetHorizontalAlignment(HAlign_Fill);
		Slot->SetVerticalAlignment(VAlign_Fill);
	}

	UBorder* HeaderBorder = MakeWidget<UBorder>(Blueprint, TEXT("HeaderBorder"));
	HeaderBorder->SetPadding(FMargin(20.0f, 12.0f));
	HeaderBorder->SetBrushColor(FLinearColor(0.035f, 0.049f, 0.071f, 1.0f));
	UVerticalBoxSlot* HeaderSlot = Main->AddChildToVerticalBox(HeaderBorder);
	HeaderSlot->SetSize(AutoSize());
	HeaderSlot->SetHorizontalAlignment(HAlign_Fill);
	HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	UHorizontalBox* Header = MakeWidget<UHorizontalBox>(Blueprint, TEXT("Header"));
	HeaderBorder->AddChild(Header);
	UTextBlock* Title = MakeText(
		Blueprint,
		TEXT("TitleText"),
		NSLOCTEXT("BackpackUIBuilder", "BackpackTitle", "背包工作台"),
		30,
		TextPrimary,
		true);
	Header->AddChildToHorizontalBox(Title)->SetPadding(FMargin(4.0f, 0.0f, 20.0f, 0.0f));
	UTextBlock* Gold = MakeText(
		Blueprint,
		TEXT("GoldText"),
		NSLOCTEXT("BackpackUIBuilder", "GoldPlaceholder", "金币：0"),
		19,
		TextSecondary,
		true);
	Header->AddChildToHorizontalBox(Gold)->SetVerticalAlignment(VAlign_Center);
	USpacer* HeaderSpacer = MakeWidget<USpacer>(Blueprint, TEXT("HeaderSpacer"));
	Header->AddChildToHorizontalBox(HeaderSpacer)->SetSize(FillSize());
	USizeBox* ArrangeSize = MakeWidget<USizeBox>(Blueprint, TEXT("ArrangeButtonSize"));
	ArrangeSize->SetWidthOverride(170.0f);
	ArrangeSize->SetHeightOverride(42.0f);
	UHorizontalBoxSlot* ArrangeSizeSlot = Header->AddChildToHorizontalBox(ArrangeSize);
	ArrangeSizeSlot->SetVerticalAlignment(VAlign_Center);
	ArrangeSizeSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
	UButton* Arrange = MakeLabeledButton(
		Blueprint,
		TEXT("ArrangeAllButton"),
		NSLOCTEXT("BackpackUIBuilder", "ArrangeAll", "整理通量卡牌"),
		FLinearColor(0.12f, 0.31f, 0.42f, 1.0f),
		true);
	ArrangeSize->AddChild(Arrange);
	USizeBox* ResetPilesSize = MakeWidget<USizeBox>(Blueprint, TEXT("ResetPilePositionsButtonSize"));
	ResetPilesSize->SetWidthOverride(170.0f);
	ResetPilesSize->SetHeightOverride(42.0f);
	if (UHorizontalBoxSlot* ResetSlot = Header->AddChildToHorizontalBox(ResetPilesSize))
	{
		ResetSlot->SetVerticalAlignment(VAlign_Center);
		ResetSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
	}
	UButton* ResetPiles = MakeLabeledButton(
		Blueprint,
		TEXT("ResetPilePositionsButton"),
		NSLOCTEXT("BackpackUIBuilder", "ResetPilePositions", "重置牌堆位置"),
		FLinearColor(0.12f, 0.24f, 0.34f, 1.0f),
		true);
	ResetPilesSize->AddChild(ResetPiles);
	USizeBox* CloseSize = MakeWidget<USizeBox>(Blueprint, TEXT("CloseButtonSize"));
	CloseSize->SetWidthOverride(108.0f);
	CloseSize->SetHeightOverride(42.0f);
	Header->AddChildToHorizontalBox(CloseSize)->SetVerticalAlignment(VAlign_Center);
	UButton* Close = MakeLabeledButton(
		Blueprint,
		TEXT("CloseButton"),
		NSLOCTEXT("BackpackUIBuilder", "CloseBackpack", "关闭"),
		FLinearColor(0.17f, 0.20f, 0.25f, 1.0f),
		true);
	CloseSize->AddChild(Close);

	UHorizontalBox* Body = MakeWidget<UHorizontalBox>(Blueprint, TEXT("Body"));
	if (UVerticalBoxSlot* BodySlot = Main->AddChildToVerticalBox(Body))
	{
		BodySlot->SetSize(FillSize());
		BodySlot->SetHorizontalAlignment(HAlign_Fill);
		BodySlot->SetVerticalAlignment(VAlign_Fill);
	}

	UBorder* WorkspacePanel = MakeWidget<UBorder>(Blueprint, TEXT("WorkspacePanel"));
	WorkspacePanel->SetPadding(FMargin(12.0f));
	WorkspacePanel->SetBrushColor(FLinearColor(0.025f, 0.036f, 0.052f, 1.0f));
	UHorizontalBoxSlot* WorkspacePanelSlot = Body->AddChildToHorizontalBox(WorkspacePanel);
	WorkspacePanelSlot->SetSize(FillSize());
	WorkspacePanelSlot->SetHorizontalAlignment(HAlign_Fill);
	WorkspacePanelSlot->SetVerticalAlignment(VAlign_Fill);
	WorkspacePanelSlot->SetPadding(FMargin(0.0f, 0.0f, 14.0f, 0.0f));
	UOverlay* WorkspaceOverlay = MakeWidget<UOverlay>(Blueprint, TEXT("WorkspaceOverlay"));
	if (UBorderSlot* Slot = Cast<UBorderSlot>(WorkspacePanel->AddChild(WorkspaceOverlay)))
	{
		Slot->SetHorizontalAlignment(HAlign_Fill);
		Slot->SetVerticalAlignment(VAlign_Fill);
	}
	UOverlay* WorkspaceHost = MakeWidget<UOverlay>(Blueprint, TEXT("WorkspaceHost"), true);
	if (UOverlaySlot* Slot = WorkspaceOverlay->AddChildToOverlay(WorkspaceHost))
	{
		Slot->SetHorizontalAlignment(HAlign_Fill);
		Slot->SetVerticalAlignment(VAlign_Fill);
	}
	USizeBox* DeleteSize = MakeWidget<USizeBox>(Blueprint, TEXT("DeleteTargetSize"));
	DeleteSize->SetWidthOverride(240.0f);
	DeleteSize->SetHeightOverride(116.0f);
	if (UOverlaySlot* DeleteSizeSlot = WorkspaceOverlay->AddChildToOverlay(DeleteSize))
	{
		DeleteSizeSlot->SetHorizontalAlignment(HAlign_Right);
		DeleteSizeSlot->SetVerticalAlignment(VAlign_Bottom);
		DeleteSizeSlot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 12.0f));
	}
	UOverlay* DeleteTargetHost = MakeWidget<UOverlay>(Blueprint, TEXT("DeleteTargetHost"), true);
	if (USizeBoxSlot* Slot = Cast<USizeBoxSlot>(DeleteSize->AddChild(DeleteTargetHost)))
	{
		Slot->SetHorizontalAlignment(HAlign_Fill);
		Slot->SetVerticalAlignment(VAlign_Fill);
	}
	UBorder* DeleteBackground = MakeWidget<UBorder>(Blueprint, TEXT("DeleteTargetBackground"));
	DeleteBackground->SetBrushColor(FLinearColor(0.30f, 0.045f, 0.052f, 0.96f));
	DeleteBackground->SetPadding(FMargin(12.0f));
	DeleteTargetHost->AddChildToOverlay(DeleteBackground);
	UTextBlock* DeleteLabel = MakeText(
		Blueprint,
		TEXT("DeleteTargetLabel"),
		NSLOCTEXT("BackpackUIBuilder", "DeleteTarget", "拖到这里批量销毁"),
		18,
		FLinearColor(1.0f, 0.76f, 0.70f, 1.0f));
	DeleteLabel->SetJustification(ETextJustify::Center);
	if (UOverlaySlot* Slot = DeleteTargetHost->AddChildToOverlay(DeleteLabel))
	{
		Slot->SetHorizontalAlignment(HAlign_Center);
		Slot->SetVerticalAlignment(VAlign_Center);
	}

	UCanvasPanel* CardDetailLayer = MakeWidget<UCanvasPanel>(Blueprint, TEXT("CardDetailLayer"), true);
	CardDetailLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UOverlaySlot* Slot = Root->AddChildToOverlay(CardDetailLayer))
	{
		Slot->SetHorizontalAlignment(HAlign_Fill);
		Slot->SetVerticalAlignment(VAlign_Fill);
	}
	UOverlay* DeleteConfirmHost = MakeWidget<UOverlay>(Blueprint, TEXT("DeleteConfirmHost"), true);
	DeleteConfirmHost->SetVisibility(ESlateVisibility::Collapsed);
	if (UOverlaySlot* Slot = Root->AddChildToOverlay(DeleteConfirmHost))
	{
		Slot->SetHorizontalAlignment(HAlign_Fill);
		Slot->SetVerticalAlignment(VAlign_Fill);
	}
	return true;
}

UWacomBackpackWorkspaceStyle* BuildWorkspaceStyle()
{
	const TCHAR* AssetName = TEXT("DA_BackpackWorkspaceStyle");
	UPackage* Package = CreatePackage(*PackagePath(AssetName));
	if (!Package)
	{
		return nullptr;
	}
	Package->FullyLoad();
	UWacomBackpackWorkspaceStyle* Style = LoadObject<UWacomBackpackWorkspaceStyle>(nullptr, *ObjectPath(AssetName));
	if (!Style)
	{
		Style = NewObject<UWacomBackpackWorkspaceStyle>(
			Package, AssetName, RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(Style);
	}
	Style->CardRenderSize = FVector2D(220.0f, 320.0f);
	Style->MinimumVisibleFraction = 0.3f;
	Style->DefaultCardSpacing = FVector2D(36.0f, 44.0f);
	Style->WorkspacePadding = FVector2D(56.0f, 56.0f);
	Style->FanMaximumAngleDegrees = 36.0f;
	Style->FanCardSpacingPixels = 72.0f;
	Style->CurrentCardLiftPixels = 56.0f;
	Style->PileCollapsedExposurePixels = 16.0f;
	Style->SettleSeconds = 0.18f;
	Style->CollectSeconds = 0.20f;
	Style->RejectedFeedbackSeconds = 0.16f;
	Style->SelectionColor = FLinearColor(0.10f, 0.78f, 1.0f, 0.96f);
	Style->CardStateOverlayOpacity = 0.20f;
	Style->ValidTargetColor = FLinearColor(0.16f, 0.88f, 0.44f, 0.96f);
	Style->RejectedTargetColor = FLinearColor(1.0f, 0.15f, 0.12f, 0.96f);
	Style->CardFeedbackMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Game/Wacom/UI/Backpack/Materials/M_BackpackWorkspaceCardFeedback.M_BackpackWorkspaceCardFeedback"));
	if (!Style->CardFeedbackMaterial)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[BackpackUIBuilder] Workspace feedback material is missing; the style will use the color-only fallback."));
	}
	return SaveTopLevelAsset(*Style) ? Style : nullptr;
}
}

bool BuildBackpackUIContent()
{
	if (!PatchBackpackDeckCardFace())
	{
		return false;
	}

	UWacomBackpackWorkspaceStyle* Style = BuildWorkspaceStyle();
	if (!Style)
	{
		return false;
	}

	UWidgetBlueprint* Workspace = LoadOrCreateWidgetBlueprint(
		TEXT("WBP_BackpackWorkspace"), UWacomBackpackWorkspaceWidget::StaticClass());
	UWidgetBlueprint* ZonePile = LoadOrCreateWidgetBlueprint(
		TEXT("WBP_BackpackZonePile"), UWacomBackpackZonePileWidget::StaticClass(), false);
	UWidgetBlueprint* Confirm = LoadOrCreateWidgetBlueprint(
		TEXT("WBP_BackpackDeleteConfirm"), UWacomBackpackDeleteConfirmWidget::StaticClass());
	UWidgetBlueprint* SpecialZone = LoadOrCreateWidgetBlueprint(
		TEXT("WBP_WacomSpecialZoneWidget"), UWacomSpecialZoneWidget::StaticClass());
	UWidgetBlueprint* Screen = LoadOrCreateWidgetBlueprint(
		TEXT("WBP_BackpackScreen"), UWacomBackpackScreen::StaticClass());
	if (!Workspace || !ZonePile || !Confirm || !SpecialZone || !Screen)
	{
		return false;
	}
	if (!IsZonePileBlueprintCurrent(*ZonePile))
	{
		ResetWidgetBlueprintContent(*ZonePile);
		if (!BuildZonePileBlueprint(*ZonePile)
			|| !CompileWidgetBlueprint(*ZonePile)
			|| !SaveTopLevelAsset(*ZonePile))
		{
			return false;
		}
	}
	if (!BuildWorkspaceBlueprint(*Workspace) || !CompileWidgetBlueprint(*Workspace)
		|| !SetObjectDefault(*Workspace, TEXT("PileWidgetClass"), ZonePile->GeneratedClass)
		|| !SaveTopLevelAsset(*Workspace))
	{
		return false;
	}
	if (!BuildDeleteConfirmBlueprint(*Confirm) || !CompileWidgetBlueprint(*Confirm) || !SaveTopLevelAsset(*Confirm))
	{
		return false;
	}
	if (!BuildSpecialZoneBlueprint(*SpecialZone)
		|| !CompileWidgetBlueprint(*SpecialZone)
		|| !SaveTopLevelAsset(*SpecialZone))
	{
		return false;
	}
	if (!BuildScreenBlueprint(*Screen) || !CompileWidgetBlueprint(*Screen))
	{
		return false;
	}
	if (!SetObjectDefault(*Screen, TEXT("WorkspaceWidgetClass"), Workspace->GeneratedClass)
		|| !SetObjectDefault(*Screen, TEXT("DeleteConfirmWidgetClass"), Confirm->GeneratedClass)
		|| !SetObjectDefault(*Screen, TEXT("SpecialZoneWidgetClass"), SpecialZone->GeneratedClass)
		|| !SetObjectDefault(*Screen, TEXT("WorkspaceStyle"), Style)
		|| !SaveTopLevelAsset(*Screen))
	{
		return false;
	}

	UE_LOG(LogTemp, Display,
		TEXT("[BackpackUIBuilder] Generated Screen, unified Workspace, formal ZonePile, Confirm, SpecialZone and Style assets; DeckCard hosts WBP_FPCardView"));
	return true;
}
}
