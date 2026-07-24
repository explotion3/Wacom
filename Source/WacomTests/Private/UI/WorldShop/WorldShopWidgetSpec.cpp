// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Cards/CardDefinition.h"
#include "UI/Card/WacomCardView.h"
#include "UI/Shop/WacomWorldShopCardWidget.h"
#include "UI/Shop/WacomWorldShopHUDWidget.h"
#include "UObject/SoftObjectPath.h"

class FWacomWorldShopWidgetTestAccess
{
public:
	static void Initialize(UWacomWorldShopCardWidget& Widget)
	{
		Widget.NativeOnInitialized();
	}

	static void Initialize(UWacomWorldShopHUDWidget& Widget)
	{
		Widget.NativeOnInitialized();
	}

	static void SubmitPrimaryAction(UWacomWorldShopCardWidget& Widget)
	{
		Widget.HandlePrimaryPressed();
		Widget.HandlePrimaryClicked();
	}
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomWorldShopWidgetCardClassSpec,
	"Wacom.UI.WorldShop.Widget.UsesFirstPersonCardViewAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomWorldShopWidgetCardClassSpec::RunTest(const FString& Parameters)
{
	const FString Expected =
		TEXT("/Game/Wacom/UI/Card/WBP_FirstPersonCardView.WBP_FirstPersonCardView_C");
	TestEqual(
		TEXT("world shop uses exact approved card face"),
		FString(UWacomWorldShopCardWidget::GetRequiredCardViewClassPath()),
		Expected);
	UClass* CardClass = LoadClass<UWacomCardView>(nullptr, *Expected);
	TestNotNull(TEXT("approved card face class loads"), CardClass);
	if (CardClass)
	{
		TestTrue(TEXT("approved class is a Wacom card view"), CardClass->IsChildOf(UWacomCardView::StaticClass()));
	}

	UWacomWorldShopCardWidget* CardWidget = NewObject<UWacomWorldShopCardWidget>();
	FWacomWorldShopWidgetTestAccess::Initialize(*CardWidget);
	TestTrue(TEXT("whole card root is the single primary button"),
		CardWidget->GetRootWidget() && CardWidget->GetRootWidget()->IsA<UButton>());
	if (const UButton* PrimaryAction = Cast<UButton>(CardWidget->GetRootWidget()))
	{
		const FButtonStyle& Style = PrimaryAction->GetStyle();
		TestEqual(TEXT("primary normal brush is transparent"),
			Style.Normal.GetDrawType(), ESlateBrushDrawType::NoDrawType);
		TestEqual(TEXT("primary hovered brush is transparent"),
			Style.Hovered.GetDrawType(), ESlateBrushDrawType::NoDrawType);
		TestEqual(TEXT("primary pressed brush is transparent"),
			Style.Pressed.GetDrawType(), ESlateBrushDrawType::NoDrawType);
	}
	TestNull(TEXT("world card does not nest the first-person Retainer wrapper"),
		CardWidget->GetWidgetFromName(TEXT("FirstPersonCardPresentation")));
	TestTrue(TEXT("world card explicitly fills the WidgetComponent render target"),
		CardWidget->GetWidgetFromName(TEXT("WorldCardRenderSurface"))
			&& CardWidget->GetWidgetFromName(TEXT("WorldCardRenderSurface"))->IsA<USizeBox>());
	if (const USizeBox* RenderSurface =
		Cast<USizeBox>(CardWidget->GetWidgetFromName(TEXT("WorldCardRenderSurface"))))
	{
		TestEqual(TEXT("render surface width matches WidgetComponent draw width"),
			RenderSurface->GetWidthOverride(), 720.0f);
		TestEqual(TEXT("render surface height matches WidgetComponent draw height"),
			RenderSurface->GetHeightOverride(), 976.0f);
	}
	TestTrue(TEXT("world card owns a 2x resolution scale surface"),
		CardWidget->GetWidgetFromName(TEXT("WorldCardResolutionScale"))
			&& CardWidget->GetWidgetFromName(TEXT("WorldCardResolutionScale"))->IsA<UScaleBox>());
	TestTrue(TEXT("world card owns a fixed logical design surface"),
		CardWidget->GetWidgetFromName(TEXT("WorldCardDesignSurface"))
			&& CardWidget->GetWidgetFromName(TEXT("WorldCardDesignSurface"))->IsA<USizeBox>());
	TestTrue(TEXT("world card face owns an explicit visible footprint"),
		CardWidget->GetWidgetFromName(TEXT("CardFaceSize"))
			&& CardWidget->GetWidgetFromName(TEXT("CardFaceSize"))->IsA<USizeBox>());
	if (const USizeBox* CardFaceSize =
		Cast<USizeBox>(CardWidget->GetWidgetFromName(TEXT("CardFaceSize"))))
	{
		TestEqual(TEXT("visible card face width is explicit"),
			CardFaceSize->GetWidthOverride(), 296.0f);
		TestEqual(TEXT("visible card face height is explicit"),
			CardFaceSize->GetHeightOverride(), 420.0f);
	}
	TestNull(TEXT("no independent buy button exists"), CardWidget->GetWidgetFromName(TEXT("BuyButton")));
	TestNotNull(TEXT("price footer exists"), CardWidget->GetWidgetFromName(TEXT("PriceFooter")));
	if (const UWidget* Footer = CardWidget->GetWidgetFromName(TEXT("PriceFooter")))
	{
		TestEqual(TEXT("footer never steals pointer input"), Footer->GetVisibility(), ESlateVisibility::HitTestInvisible);
	}
	TestNotNull(TEXT("card face is created"), CardWidget->GetCardView());
	if (UWacomCardView* CardView = CardWidget->GetCardView())
	{
		TestEqual(TEXT("card face uses exact authored class"), CardView->GetClass(), CardClass);
		TestEqual(TEXT("card face leaves primary click to root"),
			CardView->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
		FWacomShopOfferPresentationView View;
		View.OfferId = FGuid::NewGuid();
		View.CardViewData.Name = FText::FromString(TEXT("Passive world offer"));
		CardWidget->SetOfferPresentation(View, 7);
		TestEqual(TEXT("passive view data reaches authored card face"),
			CardView->GetCardViewData().Name.ToString(), FString(TEXT("Passive world offer")));
	}

	UWacomWorldShopHUDWidget* HUD = NewObject<UWacomWorldShopHUDWidget>();
	FWacomWorldShopWidgetTestAccess::Initialize(*HUD);
	TestNotNull(TEXT("transparent HUD root exists"), HUD->GetRootWidget());
	if (const UWidget* Root = HUD->GetRootWidget())
	{
		TestEqual(TEXT("HUD root never intercepts pointer input"),
			Root->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomWorldShopWidgetPrimaryIntentSpec,
	"Wacom.UI.WorldShop.Widget.PrimaryIntentPurchaseEligibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomWorldShopWidgetPrimaryIntentSpec::RunTest(const FString& Parameters)
{
	UWacomWorldShopCardWidget* Widget = NewObject<UWacomWorldShopCardWidget>();
	FWacomWorldShopWidgetTestAccess::Initialize(*Widget);

	FGuid BroadcastOfferId;
	uint32 BroadcastGeneration = 0;
	int32 BroadcastCount = 0;
	Widget->OnPrimaryActionNative().AddLambda(
		[&BroadcastOfferId, &BroadcastGeneration, &BroadcastCount](
			const FGuid OfferId,
			const uint32 Generation)
		{
			BroadcastOfferId = OfferId;
			BroadcastGeneration = Generation;
			++BroadcastCount;
		});

	UCardDefinition* CardDefinition = NewObject<UCardDefinition>();
	FWacomShopOfferPresentationView View;
	View.OfferId = FGuid::NewGuid();
	View.CardDefinition = CardDefinition;
	View.bCanPurchase = true;
	Widget->SetOfferPresentation(View, 17);
	FWacomWorldShopWidgetTestAccess::SubmitPrimaryAction(*Widget);
	TestEqual(TEXT("purchasable offer broadcasts once"), BroadcastCount, 1);
	TestEqual(TEXT("intent preserves offer id"), BroadcastOfferId, View.OfferId);
	TestEqual(TEXT("intent preserves generation"), BroadcastGeneration, 17u);

	View.bCanPurchase = false;
	View.DisabledReason = TEXT("InsufficientGold");
	Widget->SetOfferPresentation(View, 18);
	FWacomWorldShopWidgetTestAccess::SubmitPrimaryAction(*Widget);
	TestEqual(
		TEXT("insufficient gold still broadcasts for authoritative feedback"),
		BroadcastCount,
		2);
	TestEqual(TEXT("insufficient-gold generation is current"), BroadcastGeneration, 18u);

	View.bPurchased = true;
	View.DisabledReason = TEXT("Purchased");
	Widget->SetOfferPresentation(View, 19);
	FWacomWorldShopWidgetTestAccess::SubmitPrimaryAction(*Widget);
	TestEqual(TEXT("purchased offer does not broadcast"), BroadcastCount, 2);

	View.bPurchased = false;
	View.CardDefinition = nullptr;
	View.DisabledReason = TEXT("MissingCard");
	Widget->SetOfferPresentation(View, 20);
	FWacomWorldShopWidgetTestAccess::SubmitPrimaryAction(*Widget);
	TestEqual(TEXT("missing-card offer does not broadcast"), BroadcastCount, 2);
	return true;
}
