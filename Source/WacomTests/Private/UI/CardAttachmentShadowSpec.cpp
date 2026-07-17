// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Components/Image.h"
#include "Components/Overlay.h"
#include "PaperSprite.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "UI/CardViewSpecReceiver.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomCardEffectBadgeAttachmentShadowSpec,
	"Wacom.UI.CardView.AttachmentShadow.BadgeFrameOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomCardEffectBadgeAttachmentShadowSpec::RunTest(const FString& /*Parameters*/)
{
	UWacomCardEffectBadgeSpecProbe* Badge = NewObject<UWacomCardEffectBadgeSpecProbe>();
	UPaperSprite* FrameSprite = NewObject<UPaperSprite>(Badge);
	Badge->SetBadgeFrameSpriteForTest(EWacomCardViewEffectBadgeKind::Damage, FrameSprite);
	const TSharedRef<SWidget> SlateWidget = Badge->TakeWidget();

	FWacomCardViewEffectBadge Data;
	Data.Kind = EWacomCardViewEffectBadgeKind::Damage;
	Data.Value = 4;
	Badge->SetEffectBadgeData(Data);
	SlateWidget->SlatePrepass(1.0f);
	const FVector2D DesiredSizeBeforeShadow = SlateWidget->GetDesiredSize();

	FWacomCardSurfacePerspectiveView Perspective;
	Perspective.bAttachmentCastShadowEnabled = true;
	Perspective.AttachmentCastShadowOffsetPixels = FVector2D(-2.5f, 3.0f);
	Perspective.AttachmentCastShadowColor = FLinearColor(0.02f, 0.03f, 0.05f, 1.0f);
	Perspective.AttachmentCastShadowOpacity = 0.16f;
	Badge->SetAttachmentCastShadowView(Perspective);

	UImage* FrameImage = Badge->GetBadgeFrameImageForTest();
	UImage* ShadowImage = Badge->GetBadgeFrameShadowImageForTest();
	TestNotNull(TEXT("Badge frame image exists"), FrameImage);
	TestNotNull(TEXT("Runtime badge frame shadow is created"), ShadowImage);
	if (!FrameImage || !ShadowImage)
	{
		return false;
	}

	const UOverlay* ParentOverlay = Cast<UOverlay>(FrameImage->GetParent());
	TestNotNull(TEXT("Badge frame and shadow share an Overlay"), ParentOverlay);
	if (ParentOverlay)
	{
		TestEqual(
			TEXT("Shadow is inserted immediately before the physical frame"),
			ParentOverlay->GetChildIndex(ShadowImage) + 1,
			ParentOverlay->GetChildIndex(FrameImage));
	}
	TestEqual(
		TEXT("Shadow copies only the physical frame brush resource"),
		ShadowImage->GetBrush().GetResourceObject(),
		FrameImage->GetBrush().GetResourceObject());
	TestTrue(
		TEXT("Shadow applies the authored local offset"),
		ShadowImage->GetRenderTransform().Translation.Equals(FVector2D(-2.5f, 3.0f), 0.001f));
	TestTrue(
		TEXT("Shadow color and opacity are applied independently from the badge digits"),
		ShadowImage->GetColorAndOpacity().Equals(FLinearColor(0.02f, 0.03f, 0.05f, 0.16f)));
	TestEqual(
		TEXT("Only one runtime shadow is added; digit host remains a separate sibling"),
		ParentOverlay ? ParentOverlay->GetChildrenCount() : 0,
		3);
	SlateWidget->SlatePrepass(1.0f);
	TestTrue(
		TEXT("Overlay shadow does not change the badge desired size"),
		SlateWidget->GetDesiredSize().Equals(DesiredSizeBeforeShadow, 0.001f));

	FrameImage->SetVisibility(ESlateVisibility::Hidden);
	Badge->SetAttachmentCastShadowView(Perspective);
	TestEqual(
		TEXT("Hiding the physical frame also hides its runtime shadow"),
		ShadowImage->GetVisibility(),
		ESlateVisibility::Collapsed);
	FrameImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	Badge->SetAttachmentCastShadowView(Perspective);

	Badge->ResetAttachmentCastShadowView();
	TestEqual(
		TEXT("Reset collapses the runtime badge shadow"),
		ShadowImage->GetVisibility(),
		ESlateVisibility::Collapsed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomCardDurabilityAttachmentShadowSpec,
	"Wacom.UI.CardView.AttachmentShadow.DurabilityBackingOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomCardDurabilityAttachmentShadowSpec::RunTest(const FString& /*Parameters*/)
{
	UWacomCardViewSpecProbe* CardView = NewObject<UWacomCardViewSpecProbe>();
	const TSharedRef<SWidget> SlateWidget = CardView->TakeWidget();
	UImage* BackIcon = CardView->GetDurabilityBackIconForTest();
	UImage* ShadowImage = CardView->GetDurabilityShadowImageForTest();
	TestNotNull(TEXT("Fallback durability backing exists"), BackIcon);
	TestNotNull(TEXT("Runtime durability shadow is created"), ShadowImage);
	if (!BackIcon || !ShadowImage)
	{
		return false;
	}

	UPaperSprite* BackSprite = NewObject<UPaperSprite>(CardView);
	BackIcon->SetBrushResourceObject(BackSprite);
	BackIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UWidget* Host = CardView->GetDurabilityHostForTest())
	{
		Host->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	SlateWidget->SlatePrepass(1.0f);
	const FVector2D DesiredSizeBeforeShadow = SlateWidget->GetDesiredSize();

	FWacomCardSurfacePerspectiveView Perspective;
	Perspective.bEnabled = true;
	Perspective.bAttachmentCastShadowEnabled = true;
	Perspective.AttachmentCastShadowOffsetPixels = FVector2D(1.0f, 1.5f);
	Perspective.AttachmentCastShadowOpacity = 0.16f;
	CardView->SetCardSurfacePerspectiveView(Perspective);

	TestEqual(
		TEXT("Durability shadow copies the physical backing brush"),
		ShadowImage->GetBrush().GetResourceObject(),
		BackIcon->GetBrush().GetResourceObject());
	TestTrue(
		TEXT("Durability shadow uses the shared attachment offset"),
		ShadowImage->GetRenderTransform().Translation.Equals(FVector2D(1.0f, 1.5f), 0.001f));
	TestEqual(
		TEXT("Durability shadow is visible without duplicating the digit host"),
		ShadowImage->GetVisibility(),
		ESlateVisibility::HitTestInvisible);
	SlateWidget->SlatePrepass(1.0f);
	TestTrue(
		TEXT("Overlay shadow does not change the card desired size"),
		SlateWidget->GetDesiredSize().Equals(DesiredSizeBeforeShadow, 0.001f));

	CardView->ResetCardSurfacePerspectiveView();
	TestEqual(
		TEXT("A non-first-person perspective view disables the durability shadow"),
		ShadowImage->GetVisibility(),
		ESlateVisibility::Collapsed);
	return true;
}
