// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/RetainerBox.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Materials/MaterialInterface.h"
#include "UI/Backpack/WacomBackpackDeleteConfirmWidget.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UI/Backpack/WacomBackpackPilePreviewWidget.h"
#include "UI/Backpack/WacomBackpackZonePileWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "UI/Card/WacomCardView.h"
#include "UI/Card/WacomRetainedCardViewWidget.h"
#include "UI/Foundation/WacomPrimaryGameLayout.h"
#include "UObject/StrongObjectPtr.h"
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
	UClass* ConfirmClass = LoadWidgetClass(
		TEXT("/Game/Wacom/UI/Backpack/WBP_BackpackDeleteConfirm.WBP_BackpackDeleteConfirm_C"));
	UClass* DeckCardClass = LoadWidgetClass(
		TEXT("/Game/Wacom/UI/Card/WBP_WacomDeckCardWidget.WBP_WacomDeckCardWidget_C"));
	UClass* BackpackCardFaceClass = LoadWidgetClass(
		TEXT("/Game/Wacom/UI/Card/WBP_BackpackCardView.WBP_BackpackCardView_C"));
	UClass* FirstPersonCardFaceClass = LoadWidgetClass(
		TEXT("/Game/Wacom/UI/Card/WBP_FirstPersonCardView.WBP_FirstPersonCardView_C"));
	UWacomBackpackWorkspaceStyle* Style = LoadObject<UWacomBackpackWorkspaceStyle>(
		nullptr,
		TEXT("/Game/Wacom/UI/Backpack/DA_BackpackWorkspaceStyle.DA_BackpackWorkspaceStyle"));

	TestTrue(TEXT("Formal screen uses BackpackScreen parent"),
		ScreenClass && ScreenClass->IsChildOf(UWacomBackpackScreen::StaticClass()));
	TestTrue(TEXT("Formal workspace uses passive Workspace parent"),
		WorkspaceClass && WorkspaceClass->IsChildOf(UWacomBackpackWorkspaceWidget::StaticClass()));
	TestTrue(TEXT("Formal confirm uses passive DeleteConfirm parent"),
		ConfirmClass && ConfirmClass->IsChildOf(UWacomBackpackDeleteConfirmWidget::StaticClass()));
	TestTrue(TEXT("Backpack card uses passive DeckCard parent"),
		DeckCardClass && DeckCardClass->IsChildOf(UWacomDeckCardWidget::StaticClass()));
	TestTrue(TEXT("Backpack card face uses passive retained-card parent"),
		BackpackCardFaceClass
			&& BackpackCardFaceClass->IsChildOf(UWacomRetainedCardViewWidget::StaticClass()));
	const UWacomRetainedCardViewWidget* BackpackCardFaceCDO = BackpackCardFaceClass
		? Cast<UWacomRetainedCardViewWidget>(BackpackCardFaceClass->GetDefaultObject())
		: nullptr;
	TestNotNull(TEXT("Backpack retained-card asset exposes its presentation defaults"), BackpackCardFaceCDO);
	if (BackpackCardFaceCDO)
	{
		TestFalse(TEXT("Backpack retained-card asset keeps animated surface foil disabled"),
			BackpackCardFaceCDO->IsSurfaceFoilEnabled());
	}
	TestTrue(TEXT("Authored card face uses reusable CardView parent"),
		FirstPersonCardFaceClass && FirstPersonCardFaceClass->IsChildOf(UWacomCardView::StaticClass()));
	TestNotNull(TEXT("Formal workspace style asset loads"), Style);
	if (!ScreenClass || !WorkspaceClass || !ConfirmClass || !Style)
	{
		return false;
	}

	UWidgetTree* ScreenTree = GetWidgetTree(ScreenClass);
	TestNotNull(TEXT("Formal screen has compiled widget tree"), ScreenTree);
	if (ScreenTree)
	{
		UOverlay* RootFrame = Cast<UOverlay>(ScreenTree->FindWidget(TEXT("RootFrame")));
		UOverlay* Root = Cast<UOverlay>(ScreenTree->FindWidget(TEXT("Root")));
		UVerticalBox* MainLayout = Cast<UVerticalBox>(ScreenTree->FindWidget(TEXT("MainLayout")));
		UHorizontalBox* Body = Cast<UHorizontalBox>(ScreenTree->FindWidget(TEXT("Body")));
		UOverlay* WorkspaceHost = Cast<UOverlay>(ScreenTree->FindWidget(TEXT("WorkspaceHost")));
		UCanvasPanel* CardDetailLayer = Cast<UCanvasPanel>(ScreenTree->FindWidget(TEXT("CardDetailLayer")));
		UOverlay* DeleteConfirmHost = Cast<UOverlay>(ScreenTree->FindWidget(TEXT("DeleteConfirmHost")));

		TestNotNull(TEXT("Formal screen owns the CommonUI fill root"), RootFrame);
		TestEqual(TEXT("RootFrame is the compiled screen root"), ScreenTree->RootWidget.Get(), static_cast<UWidget*>(RootFrame));
		TestNull(TEXT("Formal screen does not add a second fixed 1600x900 design surface"),
			ScreenTree->FindWidget(TEXT("ScreenSize")));
		TestNotNull(TEXT("Formal screen fill root owns a root overlay"), Root);
		TestNotNull(TEXT("Formal screen root owns the main layout"), MainLayout);
		TestNotNull(TEXT("Formal screen main layout owns a fill body"), Body);
		TestNotNull(TEXT("Screen binds WorkspaceHost"), WorkspaceHost);
		TestNotNull(TEXT("Screen binds CardDetailLayer"), CardDetailLayer);
		TestNotNull(TEXT("Screen binds DeleteConfirmHost"), DeleteConfirmHost);
		if (Root)
		{
			const UOverlaySlot* RootSlot = Cast<UOverlaySlot>(Root->Slot);
			TestNotNull(TEXT("Root uses the RootFrame Overlay slot"), RootSlot);
			if (RootSlot)
			{
				TestEqual(TEXT("Root fills the CommonUI layer horizontally"), RootSlot->GetHorizontalAlignment(), HAlign_Fill);
				TestEqual(TEXT("Root fills the CommonUI layer vertically"), RootSlot->GetVerticalAlignment(), VAlign_Fill);
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
		if (Body)
		{
			const UVerticalBoxSlot* BodySlot = Cast<UVerticalBoxSlot>(Body->Slot);
			TestNotNull(TEXT("Body uses a VerticalBox slot"), BodySlot);
			if (BodySlot)
			{
				TestEqual(TEXT("Body consumes the remaining screen height"),
					BodySlot->GetSize().SizeRule, ESlateSizeRule::Fill);
			}
		}
		if (WorkspaceHost)
		{
			const UOverlaySlot* WorkspaceHostSlot = Cast<UOverlaySlot>(WorkspaceHost->Slot);
			TestNotNull(TEXT("WorkspaceHost uses the unified Workspace overlay slot"), WorkspaceHostSlot);
			if (WorkspaceHostSlot)
			{
				TestEqual(TEXT("WorkspaceHost fills horizontally"),
					WorkspaceHostSlot->GetHorizontalAlignment(), HAlign_Fill);
				TestEqual(TEXT("WorkspaceHost fills vertically"),
					WorkspaceHostSlot->GetVerticalAlignment(), VAlign_Fill);
			}
		}
		auto TestOverlayFill = [this](const TCHAR* Label, UWidget* Widget)
		{
			const UOverlaySlot* Slot = Widget ? Cast<UOverlaySlot>(Widget->Slot) : nullptr;
			TestNotNull(*FString::Printf(TEXT("%s uses an Overlay slot"), Label), Slot);
			if (Slot)
			{
				TestEqual(*FString::Printf(TEXT("%s fills horizontally"), Label),
					Slot->GetHorizontalAlignment(), HAlign_Fill);
				TestEqual(*FString::Printf(TEXT("%s fills vertically"), Label),
					Slot->GetVerticalAlignment(), VAlign_Fill);
			}
		};
		TestOverlayFill(TEXT("CardDetailLayer"), CardDetailLayer);
		TestOverlayFill(TEXT("DeleteConfirmHost"), DeleteConfirmHost);
		if (DeleteConfirmHost)
		{
			TestEqual(TEXT("DeleteConfirmHost starts collapsed"),
				DeleteConfirmHost->GetVisibility(), ESlateVisibility::Collapsed);
		}
		TestNull(TEXT("Screen removes the legacy ZoneRackHost"), ScreenTree->FindWidget(TEXT("ZoneRackHost")));
		TestNotNull(TEXT("Screen binds DeleteTargetHost"), Cast<UOverlay>(ScreenTree->FindWidget(TEXT("DeleteTargetHost"))));
		TestNotNull(TEXT("Screen binds ArrangeAllButton"), Cast<UButton>(ScreenTree->FindWidget(TEXT("ArrangeAllButton"))));
		TestNotNull(TEXT("Screen binds ResetPilePositionsButton"),
			Cast<UButton>(ScreenTree->FindWidget(TEXT("ResetPilePositionsButton"))));
		TestNull(TEXT("Formal screen removes old DeleteZoneHost"), ScreenTree->FindWidget(TEXT("DeleteZoneHost")));
		TestNull(TEXT("Formal screen removes old BattleDeckZoneHost"), ScreenTree->FindWidget(TEXT("BattleDeckZoneHost")));
		TestNull(TEXT("Formal screen removes old SpecialZonesHost"), ScreenTree->FindWidget(TEXT("SpecialZonesHost")));
	}

	UWidgetTree* WorkspaceTree = GetWidgetTree(WorkspaceClass);
	TestNotNull(TEXT("Workspace binds its sole unified WorkspaceCanvas"),
		WorkspaceTree ? Cast<UCanvasPanel>(WorkspaceTree->FindWidget(TEXT("WorkspaceCanvas"))) : nullptr);
	TestNotNull(TEXT("Workspace binds SelectionMarquee"),
		WorkspaceTree ? Cast<UBorder>(WorkspaceTree->FindWidget(TEXT("SelectionMarquee"))) : nullptr);
	TestNotNull(TEXT("Workspace binds EmptyStateText"),
		WorkspaceTree ? Cast<UTextBlock>(WorkspaceTree->FindWidget(TEXT("EmptyStateText"))) : nullptr);

	UWidgetTree* ConfirmTree = GetWidgetTree(ConfirmClass);
	TestNotNull(TEXT("Confirm binds summary"),
		ConfirmTree ? Cast<UTextBlock>(ConfirmTree->FindWidget(TEXT("SummaryText"))) : nullptr);
	TestNotNull(TEXT("Confirm binds confirm button"),
		ConfirmTree ? Cast<UButton>(ConfirmTree->FindWidget(TEXT("ConfirmButton"))) : nullptr);
	TestNotNull(TEXT("Confirm binds cancel button"),
		ConfirmTree ? Cast<UButton>(ConfirmTree->FindWidget(TEXT("CancelButton"))) : nullptr);

	UWidgetTree* DeckCardTree = GetWidgetTree(DeckCardClass);
	const UWidgetBlueprintGeneratedClass* DeckCardGeneratedClass =
		Cast<UWidgetBlueprintGeneratedClass>(DeckCardClass);
	TestNotNull(TEXT("Backpack card has a generated widget class"), DeckCardGeneratedClass);
	if (DeckCardGeneratedClass)
	{
		TestEqual(
			TEXT("Passive backpack card has no authored animation that can overwrite runtime layout or opacity"),
			DeckCardGeneratedClass->Animations.Num(),
			0);
	}
	if (DeckCardTree && DeckCardTree->RootWidget)
	{
		TestEqual(
			TEXT("Backpack card root starts fully opaque"),
			DeckCardTree->RootWidget->GetRenderOpacity(),
			1.0f);
		TestTrue(
			TEXT("Backpack card root starts at unit render scale"),
			DeckCardTree->RootWidget->GetRenderTransform().Scale.Equals(FVector2D::UnitVector));
	}
	UScaleBox* CardFaceScaleBox = DeckCardTree
		? Cast<UScaleBox>(DeckCardTree->FindWidget(TEXT("CardFaceScaleBox")))
		: nullptr;
	UBorder* WorkspaceFeedbackOverlay = DeckCardTree
		? Cast<UBorder>(DeckCardTree->FindWidget(TEXT("WorkspaceFeedbackOverlay")))
		: nullptr;
	UTextBlock* BattleEnabledBadge = DeckCardTree
		? Cast<UTextBlock>(DeckCardTree->FindWidget(TEXT("BattleEnabledBadge")))
		: nullptr;
	UTextBlock* ProjectedFromBadge = DeckCardTree
		? Cast<UTextBlock>(DeckCardTree->FindWidget(TEXT("ProjectedFromBadge")))
		: nullptr;
	UWacomRetainedCardViewWidget* EmbeddedCardFace = DeckCardTree
		? Cast<UWacomRetainedCardViewWidget>(DeckCardTree->FindWidget(TEXT("BackpackCardView")))
		: nullptr;
	TestNotNull(TEXT("Backpack card uniformly scales the authored face instead of relaying it out"), CardFaceScaleBox);
	TestNotNull(TEXT("Backpack card binds the retained Backpack CardView"), EmbeddedCardFace);
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
	TestNotNull(TEXT("Backpack card keeps the battle-ready status badge"), BattleEnabledBadge);
	TestNotNull(TEXT("Backpack card keeps the projected-source status badge"), ProjectedFromBadge);
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
		UPanelWidget* FeedbackHost = WorkspaceFeedbackOverlay->GetParent();
		const int32 FaceIndex = FeedbackHost ? FeedbackHost->GetChildIndex(CardFaceScaleBox) : INDEX_NONE;
		const int32 FeedbackIndex = FeedbackHost ? FeedbackHost->GetChildIndex(WorkspaceFeedbackOverlay) : INDEX_NONE;
		TestEqual(TEXT("Workspace feedback is the first layer above the authored face"),
			FeedbackIndex, FaceIndex + 1);
		if (BattleEnabledBadge && BattleEnabledBadge->GetParent() == FeedbackHost)
		{
			TestTrue(TEXT("Battle-ready badge renders above workspace feedback"),
				FeedbackHost->GetChildIndex(BattleEnabledBadge) > FeedbackIndex);
		}
		if (ProjectedFromBadge && ProjectedFromBadge->GetParent() == FeedbackHost)
		{
			TestTrue(TEXT("Projected-source badge renders above workspace feedback"),
				FeedbackHost->GetChildIndex(ProjectedFromBadge) > FeedbackIndex);
		}
	}
	if (EmbeddedCardFace && BackpackCardFaceClass)
	{
		TestEqual(
			TEXT("Backpack card embeds the backpack-specific retained wrapper"),
			EmbeddedCardFace->GetClass(),
			BackpackCardFaceClass);
	}

	UWidgetTree* BackpackCardFaceTree = GetWidgetTree(BackpackCardFaceClass);
	int32 RetainerSurfaceCount = 0;
	if (BackpackCardFaceTree)
	{
		BackpackCardFaceTree->ForEachWidget(
			[&RetainerSurfaceCount](UWidget* Widget)
			{
				RetainerSurfaceCount += Cast<URetainerBox>(Widget) ? 1 : 0;
			});
	}
	TestEqual(TEXT("Backpack CardView owns exactly one retained render surface"),
		RetainerSurfaceCount, 1);
	URetainerBox* CardFaceRetainer = BackpackCardFaceTree
		? Cast<URetainerBox>(BackpackCardFaceTree->FindWidget(TEXT("CardFaceRetainer")))
		: nullptr;
	UWacomCardView* RetainedInnerCardFace = BackpackCardFaceTree
		? Cast<UWacomCardView>(BackpackCardFaceTree->FindWidget(TEXT("CardView")))
		: nullptr;
	TestNotNull(TEXT("Backpack CardView owns one retained render surface"), CardFaceRetainer);
	TestNotNull(TEXT("Backpack CardView retains the authored inner CardView"), RetainedInnerCardFace);
	if (CardFaceRetainer)
	{
		TestTrue(TEXT("Backpack CardView keeps retained rendering enabled"),
			CardFaceRetainer->IsRetainRendering());
		TestTrue(TEXT("Backpack CardView redraws after child invalidation"),
			CardFaceRetainer->IsRenderOnInvalidation());
		TestFalse(TEXT("Backpack CardView does not redraw on a frame phase"),
			CardFaceRetainer->IsRenderOnPhase());
		TestNull(TEXT("Backpack CardView adds no first-person effect material"),
			CardFaceRetainer->GetEffectMaterialInterface());
		TestEqual(TEXT("Backpack CardView keeps the retained surface out of hit testing"),
			CardFaceRetainer->GetVisibility(), ESlateVisibility::HitTestInvisible);
		TestEqual(TEXT("Retainer directly owns the authored card face"),
			CardFaceRetainer->GetContent(), static_cast<UWidget*>(RetainedInnerCardFace));
	}
	if (RetainedInnerCardFace && FirstPersonCardFaceClass)
	{
		TestEqual(TEXT("Retained Backpack CardView preserves the authored layout class"),
			RetainedInnerCardFace->GetClass(), FirstPersonCardFaceClass);
	}
	if (BackpackCardFaceClass)
	{
		TStrongObjectPtr<UWacomRetainedCardViewWidget> RuntimeBackpackCardFace(
			NewObject<UWacomRetainedCardViewWidget>(GetTransientPackage(), BackpackCardFaceClass));
		RuntimeBackpackCardFace->TakeWidget();
		UWacomCardView* RuntimeInnerCardFace = RuntimeBackpackCardFace->GetInnerCardView();
		TestNotNull(TEXT("Generated Backpack CardView binds its runtime authored face"), RuntimeInnerCardFace);
		if (RuntimeInnerCardFace)
		{
			const FWacomCardViewAutomationTestView RuntimeFaceView =
				RuntimeInnerCardFace->GetAutomationTestViewForTest();
			TestFalse(TEXT("Generated Backpack CardView disables the inner SurfaceFoilOverlay"),
				RuntimeFaceView.bSurfaceFoilEnabled);
			TestFalse(TEXT("Generated Backpack CardView keeps the inner SurfaceFoilOverlay collapsed"),
				RuntimeFaceView.bSurfaceFoilVisible);
			TestFalse(TEXT("Generated Backpack CardView releases the inner SurfaceFoilOverlay brush"),
				RuntimeFaceView.bSurfaceFoilBrushConfigured);
		}
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
	TestNull(TEXT("Screen class no longer exposes a ZoneRackWidgetClass dependency"),
		FindFProperty<FProperty>(ScreenClass, TEXT("ZoneRackWidgetClass")));
	TestEqual(TEXT("Screen CDO selects formal confirmation class"),
		ReadObjectDefault(ScreenClass, TEXT("DeleteConfirmWidgetClass")), static_cast<UObject*>(ConfirmClass));
	TestEqual(TEXT("Screen CDO selects formal workspace style"),
		ReadObjectDefault(ScreenClass, TEXT("WorkspaceStyle")), static_cast<UObject*>(Style));
	TestEqual(TEXT("Workspace CDO selects the passive embedded pile class"),
		ReadObjectDefault(WorkspaceClass, TEXT("PileWidgetClass")),
		static_cast<UObject*>(UWacomBackpackZonePileWidget::StaticClass()));
	TestEqual(TEXT("Workspace CDO selects the simplified preview class"),
		ReadObjectDefault(WorkspaceClass, TEXT("PilePreviewWidgetClass")),
		static_cast<UObject*>(UWacomBackpackPilePreviewWidget::StaticClass()));
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
