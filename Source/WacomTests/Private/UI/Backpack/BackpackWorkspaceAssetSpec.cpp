// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Materials/MaterialInterface.h"
#include "UI/Backpack/WacomBackpackDeleteConfirmWidget.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UI/Backpack/WacomBackpackZoneRackEntryWidget.h"
#include "UI/Backpack/WacomBackpackZoneRackWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "UI/Card/WacomCardView.h"
#include "UI/Foundation/WacomPrimaryGameLayout.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

namespace
{
UClass* LoadWidgetClass(const TCHAR* ObjectPath)
{
	return LoadObject<UClass>(nullptr, ObjectPath);
}

UWidgetTree* GetWidgetTree(UClass* WidgetClass)
{
	const UWidgetBlueprintGeneratedClass* GeneratedClass = Cast<UWidgetBlueprintGeneratedClass>(WidgetClass);
	return GeneratedClass ? GeneratedClass->GetWidgetTreeArchetype() : nullptr;
}

UObject* ReadObjectDefault(UClass* WidgetClass, FName PropertyName)
{
	UObject* CDO = WidgetClass ? WidgetClass->GetDefaultObject() : nullptr;
	const FObjectPropertyBase* Property = CDO
		? FindFProperty<FObjectPropertyBase>(CDO->GetClass(), PropertyName)
		: nullptr;
	return Property ? Property->GetObjectPropertyValue_InContainer(CDO) : nullptr;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceFormalAssetBindingSpec,
	"Wacom.UI.Backpack.Workspace.FormalAssetBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceFormalAssetBindingSpec::RunTest(const FString& Parameters)
{
	UClass* ScreenClass = LoadWidgetClass(
		TEXT("/Game/Wacom/UI/Backpack/WBP_BackpackScreen.WBP_BackpackScreen_C"));
	UClass* WorkspaceClass = LoadWidgetClass(
		TEXT("/Game/Wacom/UI/Backpack/WBP_BackpackWorkspace.WBP_BackpackWorkspace_C"));
	UClass* RackClass = LoadWidgetClass(
		TEXT("/Game/Wacom/UI/Backpack/WBP_BackpackZoneRack.WBP_BackpackZoneRack_C"));
	UClass* EntryClass = LoadWidgetClass(
		TEXT("/Game/Wacom/UI/Backpack/WBP_BackpackZoneRackEntry.WBP_BackpackZoneRackEntry_C"));
	UClass* ConfirmClass = LoadWidgetClass(
		TEXT("/Game/Wacom/UI/Backpack/WBP_BackpackDeleteConfirm.WBP_BackpackDeleteConfirm_C"));
	UClass* DeckCardClass = LoadWidgetClass(
		TEXT("/Game/Wacom/UI/Card/WBP_WacomDeckCardWidget.WBP_WacomDeckCardWidget_C"));
	UClass* FirstPersonCardFaceClass = LoadWidgetClass(
		TEXT("/Game/Wacom/UI/Card/WBP_FirstPersonCardView.WBP_FirstPersonCardView_C"));
	UWacomBackpackWorkspaceStyle* Style = LoadObject<UWacomBackpackWorkspaceStyle>(
		nullptr,
		TEXT("/Game/Wacom/UI/Backpack/DA_BackpackWorkspaceStyle.DA_BackpackWorkspaceStyle"));

	TestTrue(TEXT("Formal screen uses BackpackScreen parent"),
		ScreenClass && ScreenClass->IsChildOf(UWacomBackpackScreen::StaticClass()));
	TestTrue(TEXT("Formal workspace uses passive Workspace parent"),
		WorkspaceClass && WorkspaceClass->IsChildOf(UWacomBackpackWorkspaceWidget::StaticClass()));
	TestTrue(TEXT("Formal rack uses passive ZoneRack parent"),
		RackClass && RackClass->IsChildOf(UWacomBackpackZoneRackWidget::StaticClass()));
	TestTrue(TEXT("Formal entry uses passive ZoneRackEntry parent"),
		EntryClass && EntryClass->IsChildOf(UWacomBackpackZoneRackEntryWidget::StaticClass()));
	TestTrue(TEXT("Formal confirm uses passive DeleteConfirm parent"),
		ConfirmClass && ConfirmClass->IsChildOf(UWacomBackpackDeleteConfirmWidget::StaticClass()));
	TestTrue(TEXT("Backpack card uses passive DeckCard parent"),
		DeckCardClass && DeckCardClass->IsChildOf(UWacomDeckCardWidget::StaticClass()));
	TestTrue(TEXT("Backpack card face uses reusable CardView parent"),
		FirstPersonCardFaceClass && FirstPersonCardFaceClass->IsChildOf(UWacomCardView::StaticClass()));
	TestNotNull(TEXT("Formal workspace style asset loads"), Style);
	if (!ScreenClass || !WorkspaceClass || !RackClass || !EntryClass || !ConfirmClass || !Style)
	{
		return false;
	}

	UWidgetTree* ScreenTree = GetWidgetTree(ScreenClass);
	TestNotNull(TEXT("Formal screen has compiled widget tree"), ScreenTree);
	if (ScreenTree)
	{
		USizeBox* ScreenSize = Cast<USizeBox>(ScreenTree->FindWidget(TEXT("ScreenSize")));
		TestNotNull(TEXT("Formal screen provides the 1600x900 pixel-safe design surface"), ScreenSize);
		if (ScreenSize)
		{
			TestTrue(TEXT("Formal screen width override is enabled"), ScreenSize->IsWidthOverride());
			TestTrue(TEXT("Formal screen height override is enabled"), ScreenSize->IsHeightOverride());
			TestEqual(TEXT("Formal screen design width"), ScreenSize->GetWidthOverride(), 1600.0f);
			TestEqual(TEXT("Formal screen design height"), ScreenSize->GetHeightOverride(), 900.0f);
		}
		UOverlay* Root = Cast<UOverlay>(ScreenTree->FindWidget(TEXT("Root")));
		UVerticalBox* MainLayout = Cast<UVerticalBox>(ScreenTree->FindWidget(TEXT("MainLayout")));
		TestNotNull(TEXT("Formal screen design surface owns a root overlay"), Root);
		TestNotNull(TEXT("Formal screen root owns the main layout"), MainLayout);
		if (Root)
		{
			const USizeBoxSlot* RootSlot = Cast<USizeBoxSlot>(Root->Slot);
			TestNotNull(TEXT("Root uses a SizeBox slot"), RootSlot);
			if (RootSlot)
			{
				TestEqual(TEXT("Root fills the design surface horizontally"), RootSlot->GetHorizontalAlignment(), HAlign_Fill);
				TestEqual(TEXT("Root fills the design surface vertically"), RootSlot->GetVerticalAlignment(), VAlign_Fill);
			}
		}
		if (MainLayout)
		{
			const UOverlaySlot* MainSlot = Cast<UOverlaySlot>(MainLayout->Slot);
			TestNotNull(TEXT("Main layout uses an Overlay slot"), MainSlot);
			if (MainSlot)
			{
				TestEqual(TEXT("Main layout fills the root horizontally"), MainSlot->GetHorizontalAlignment(), HAlign_Fill);
				TestEqual(TEXT("Main layout fills the root vertically"), MainSlot->GetVerticalAlignment(), VAlign_Fill);
			}
		}
		TestNotNull(TEXT("Screen binds WorkspaceHost"), Cast<UOverlay>(ScreenTree->FindWidget(TEXT("WorkspaceHost"))));
		TestNotNull(TEXT("Screen binds ZoneRackHost"), Cast<UOverlay>(ScreenTree->FindWidget(TEXT("ZoneRackHost"))));
		TestNotNull(TEXT("Screen binds DeleteTargetHost"), Cast<UOverlay>(ScreenTree->FindWidget(TEXT("DeleteTargetHost"))));
		TestNotNull(TEXT("Screen binds DeleteConfirmHost"), Cast<UOverlay>(ScreenTree->FindWidget(TEXT("DeleteConfirmHost"))));
		TestNotNull(TEXT("Screen binds ArrangeAllButton"), Cast<UButton>(ScreenTree->FindWidget(TEXT("ArrangeAllButton"))));
		TestNotNull(TEXT("Screen binds CardDetailLayer"), Cast<UCanvasPanel>(ScreenTree->FindWidget(TEXT("CardDetailLayer"))));
		TestNull(TEXT("Formal screen removes old DeleteZoneHost"), ScreenTree->FindWidget(TEXT("DeleteZoneHost")));
		TestNull(TEXT("Formal screen removes old BattleDeckZoneHost"), ScreenTree->FindWidget(TEXT("BattleDeckZoneHost")));
		TestNull(TEXT("Formal screen removes old SpecialZonesHost"), ScreenTree->FindWidget(TEXT("SpecialZonesHost")));
	}

	UWidgetTree* WorkspaceTree = GetWidgetTree(WorkspaceClass);
	TestNotNull(TEXT("Workspace binds CardCanvas"),
		WorkspaceTree ? Cast<UCanvasPanel>(WorkspaceTree->FindWidget(TEXT("CardCanvas"))) : nullptr);
	TestNotNull(TEXT("Workspace binds SelectionMarquee"),
		WorkspaceTree ? Cast<UBorder>(WorkspaceTree->FindWidget(TEXT("SelectionMarquee"))) : nullptr);
	TestNotNull(TEXT("Workspace binds EmptyStateText"),
		WorkspaceTree ? Cast<UTextBlock>(WorkspaceTree->FindWidget(TEXT("EmptyStateText"))) : nullptr);

	UWidgetTree* RackTree = GetWidgetTree(RackClass);
	TestNotNull(TEXT("Rack binds EntriesHost"),
		RackTree ? Cast<UVerticalBox>(RackTree->FindWidget(TEXT("EntriesHost"))) : nullptr);
	UWidgetTree* EntryTree = GetWidgetTree(EntryClass);
	TestNotNull(TEXT("Entry binds activation button"),
		EntryTree ? Cast<UButton>(EntryTree->FindWidget(TEXT("ActivateButton"))) : nullptr);
	TestNotNull(TEXT("Entry binds active/preview border"),
		EntryTree ? Cast<UBorder>(EntryTree->FindWidget(TEXT("ActiveBorder"))) : nullptr);

	UWidgetTree* ConfirmTree = GetWidgetTree(ConfirmClass);
	TestNotNull(TEXT("Confirm binds summary"),
		ConfirmTree ? Cast<UTextBlock>(ConfirmTree->FindWidget(TEXT("SummaryText"))) : nullptr);
	TestNotNull(TEXT("Confirm binds confirm button"),
		ConfirmTree ? Cast<UButton>(ConfirmTree->FindWidget(TEXT("ConfirmButton"))) : nullptr);
	TestNotNull(TEXT("Confirm binds cancel button"),
		ConfirmTree ? Cast<UButton>(ConfirmTree->FindWidget(TEXT("CancelButton"))) : nullptr);

	UWidgetTree* DeckCardTree = GetWidgetTree(DeckCardClass);
	UScaleBox* CardFaceScaleBox = DeckCardTree
		? Cast<UScaleBox>(DeckCardTree->FindWidget(TEXT("CardFaceScaleBox")))
		: nullptr;
	UBorder* WorkspaceFeedbackOverlay = DeckCardTree
		? Cast<UBorder>(DeckCardTree->FindWidget(TEXT("WorkspaceFeedbackOverlay")))
		: nullptr;
	UWacomCardView* EmbeddedCardFace = DeckCardTree
		? Cast<UWacomCardView>(DeckCardTree->FindWidget(TEXT("CardView")))
		: nullptr;
	TestNotNull(TEXT("Backpack card uniformly scales the authored face instead of relaying it out"), CardFaceScaleBox);
	TestNotNull(TEXT("Backpack card binds a reusable CardView"), EmbeddedCardFace);
	if (CardFaceScaleBox)
	{
		TestEqual(
			TEXT("Card face uses a fixed uniform scale without triggering child relayout"),
			CardFaceScaleBox->GetStretch(),
			EStretch::UserSpecified);
		TestTrue(
			TEXT("Card face uses the fixed three-quarter pixel-art scale"),
			FMath::IsNearlyEqual(CardFaceScaleBox->GetUserSpecifiedScale(), 0.75f));
		TestEqual(
			TEXT("Card face is the scale box content"),
			CardFaceScaleBox->GetContent(),
			static_cast<UWidget*>(EmbeddedCardFace));
	}
	const UScaleBoxSlot* EmbeddedCardFaceSlot = EmbeddedCardFace
		? Cast<UScaleBoxSlot>(EmbeddedCardFace->Slot)
		: nullptr;
	TestNotNull(TEXT("Authored card face is hosted by a ScaleBox slot"), EmbeddedCardFaceSlot);
	if (EmbeddedCardFaceSlot)
	{
		TestEqual(
			TEXT("Authored card face keeps its desired width instead of being filled into the backpack width"),
			EmbeddedCardFaceSlot->GetHorizontalAlignment(),
			HAlign_Center);
		TestEqual(
			TEXT("Authored card face stays vertically centered in the scale host"),
			EmbeddedCardFaceSlot->GetVerticalAlignment(),
			VAlign_Center);
	}
	TestNotNull(TEXT("Backpack card exposes a dedicated workspace feedback overlay"), WorkspaceFeedbackOverlay);
	if (WorkspaceFeedbackOverlay)
	{
		TestEqual(
			TEXT("Workspace feedback overlay never steals pointer input"),
			WorkspaceFeedbackOverlay->GetVisibility(),
			ESlateVisibility::Collapsed);
		TestEqual(
			TEXT("Workspace feedback overlay shares the card overlay host"),
			WorkspaceFeedbackOverlay->GetParent(),
			EmbeddedCardFace && EmbeddedCardFace->GetParent()
				? EmbeddedCardFace->GetParent()->GetParent()
				: nullptr);
		const UOverlaySlot* FeedbackSlot = Cast<UOverlaySlot>(WorkspaceFeedbackOverlay->Slot);
		TestNotNull(TEXT("Workspace feedback overlay uses the card Overlay slot"), FeedbackSlot);
		if (FeedbackSlot)
		{
			TestEqual(TEXT("Workspace feedback fills the card horizontally"), FeedbackSlot->GetHorizontalAlignment(), HAlign_Fill);
			TestEqual(TEXT("Workspace feedback fills the card vertically"), FeedbackSlot->GetVerticalAlignment(), VAlign_Fill);
		}
	}
	if (EmbeddedCardFace && FirstPersonCardFaceClass)
	{
		TestEqual(
			TEXT("Backpack card embeds the authored first-person card face layout"),
			EmbeddedCardFace->GetClass(),
			FirstPersonCardFaceClass);
	}

	UWidgetTree* FirstPersonCardFaceTree = GetWidgetTree(FirstPersonCardFaceClass);
	USizeBox* FirstPersonFaceRoot = FirstPersonCardFaceTree
		? Cast<USizeBox>(FirstPersonCardFaceTree->FindWidget(TEXT("SizeBox_0")))
		: nullptr;
	TestNotNull(TEXT("Authored first-person card face keeps its fixed bleed design surface"), FirstPersonFaceRoot);
	if (FirstPersonFaceRoot)
	{
		TestTrue(TEXT("Authored card face width override is enabled"), FirstPersonFaceRoot->IsWidthOverride());
		TestTrue(TEXT("Authored card face height override is enabled"), FirstPersonFaceRoot->IsHeightOverride());
		TestEqual(TEXT("Authored card face bleed width"), FirstPersonFaceRoot->GetWidthOverride(), 360.0f);
		TestEqual(TEXT("Authored card face design height"), FirstPersonFaceRoot->GetHeightOverride(), 424.0f);
	}

	TestEqual(TEXT("Screen CDO selects formal Workspace class"),
		ReadObjectDefault(ScreenClass, TEXT("WorkspaceWidgetClass")), static_cast<UObject*>(WorkspaceClass));
	TestEqual(TEXT("Screen CDO selects formal ZoneRack class"),
		ReadObjectDefault(ScreenClass, TEXT("ZoneRackWidgetClass")), static_cast<UObject*>(RackClass));
	TestEqual(TEXT("Screen CDO selects formal confirmation class"),
		ReadObjectDefault(ScreenClass, TEXT("DeleteConfirmWidgetClass")), static_cast<UObject*>(ConfirmClass));
	TestEqual(TEXT("Screen CDO selects formal workspace style"),
		ReadObjectDefault(ScreenClass, TEXT("WorkspaceStyle")), static_cast<UObject*>(Style));
	TestEqual(TEXT("Rack CDO selects formal entry class"),
		ReadObjectDefault(RackClass, TEXT("EntryWidgetClass")), static_cast<UObject*>(EntryClass));
	UMaterialInterface* FeedbackMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Game/Wacom/UI/Backpack/Materials/M_BackpackWorkspaceCardFeedback.M_BackpackWorkspaceCardFeedback"));
	TestNotNull(TEXT("Formal workspace feedback material loads"), FeedbackMaterial);
	TestEqual(TEXT("Style selects formal feedback material"), Style->CardFeedbackMaterial.Get(), FeedbackMaterial);
	TestEqual(TEXT("Style keeps 30 percent minimum visibility"), Style->MinimumVisibleFraction, 0.3f);
	TestEqual(TEXT("Style keeps default current lift"), Style->CurrentCardLiftPixels, 56.0f);

	UClass* PrimaryLayoutClass = LoadWidgetClass(
		TEXT("/Game/Wacom/UI/Foundation/WBP_PrimaryGameLayout.WBP_PrimaryGameLayout_C"));
	UWidgetTree* PrimaryTree = GetWidgetTree(PrimaryLayoutClass);
	UCommonActivatableWidgetStack* GameMenuStack = PrimaryTree
		? Cast<UCommonActivatableWidgetStack>(PrimaryTree->FindWidget(TEXT("GameMenuLayerStack")))
		: nullptr;
	TestNotNull(TEXT("Primary layout exposes GameMenu stack for formal Backpack"), GameMenuStack);
	if (PrimaryTree && PrimaryTree->RootWidget)
	{
		AddInfo(FString::Printf(TEXT("Primary root: %s (%s)"),
			*PrimaryTree->RootWidget->GetName(), *PrimaryTree->RootWidget->GetClass()->GetName()));
	}
	if (GameMenuStack)
	{
		AddInfo(FString::Printf(TEXT("GameMenu stack slot: %s"),
			GameMenuStack->Slot ? *GameMenuStack->Slot->GetClass()->GetName() : TEXT("None")));
		if (const UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(GameMenuStack->Slot))
		{
			const FAnchors Anchors = CanvasSlot->GetAnchors();
			const FMargin Offsets = CanvasSlot->GetOffsets();
			AddInfo(FString::Printf(
				TEXT("GameMenu Canvas anchors=(%.2f,%.2f)-(%.2f,%.2f) offsets=(%.1f,%.1f,%.1f,%.1f)"),
				Anchors.Minimum.X, Anchors.Minimum.Y, Anchors.Maximum.X, Anchors.Maximum.Y,
				Offsets.Left, Offsets.Top, Offsets.Right, Offsets.Bottom));
		}
		if (const UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(GameMenuStack->Slot))
		{
			AddInfo(FString::Printf(TEXT("GameMenu Overlay align=(%d,%d)"),
				static_cast<int32>(OverlaySlot->GetHorizontalAlignment()),
				static_cast<int32>(OverlaySlot->GetVerticalAlignment())));
		}
	}
	return true;
}

#endif
