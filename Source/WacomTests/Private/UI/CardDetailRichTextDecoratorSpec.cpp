// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Tags/WacomGameplayTags.h"
#include "UI/Card/WacomCardDetailRichTextBlock.h"
#include "UI/Card/WacomCardDetailTheme.h"

#include "Engine/Texture2D.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	FSlateBrush MakeInlineBrushForTest()
	{
		FSlateBrush Brush;
		UTexture2D* Texture = NewObject<UTexture2D>();
		Brush.SetResourceObject(Texture);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.TintColor = FSlateColor(FLinearColor::White);
		Brush.SetImageSize(FVector2f(18.0f, 18.0f));
		return Brush;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUICardDetailRichTextThemeLookupSpec,
	"Wacom.UI.CardDetail.RichTextDecorator.ThemeLookup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUICardDetailRichTextThemeLookupSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardDetailTheme> Theme(NewObject<UWacomCardDetailTheme>());

	FWacomCardDetailStatusBrushEntry PoisonStatus;
	PoisonStatus.StatusTag = WacomTags::Status_Poison;
	PoisonStatus.Brush = MakeInlineBrushForTest();
	Theme->StatusBrushes.Add(PoisonStatus);

	FWacomCardDetailIconBrushEntry DamageIcon;
	DamageIcon.Icon = EWacomCardDetailIcon::Damage;
	DamageIcon.Brush = MakeInlineBrushForTest();
	Theme->IconBrushes.Add(DamageIcon);

	TestNotNull(TEXT("Theme resolves exact status brush"),
		Theme->ResolveStatusBrush(WacomTags::Status_Poison));
	TestNotNull(TEXT("Theme resolves exact icon brush"),
		Theme->ResolveIconBrush(EWacomCardDetailIcon::Damage));
	TestNull(TEXT("Missing status without fallback returns null"),
		Theme->ResolveStatusBrush(WacomTags::Status_Freeze));

	Theme->FallbackInlineBrush = MakeInlineBrushForTest();
	TestNotNull(TEXT("Missing status can use fallback brush"),
		Theme->ResolveStatusBrush(WacomTags::Status_Freeze));
	TestNotNull(TEXT("Missing icon can use fallback brush"),
		Theme->ResolveIconBrush(EWacomCardDetailIcon::Heal));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUICardDetailRichTextDecoratorRegistrationSpec,
	"Wacom.UI.CardDetail.RichTextDecorator.AutoRegistration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUICardDetailRichTextDecoratorRegistrationSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardDetailTheme> Theme(NewObject<UWacomCardDetailTheme>());
	FWacomCardDetailStatusBrushEntry PoisonStatus;
	PoisonStatus.StatusTag = WacomTags::Status_Poison;
	PoisonStatus.Brush = MakeInlineBrushForTest();
	Theme->StatusBrushes.Add(PoisonStatus);

	TStrongObjectPtr<UWacomCardDetailRichTextBlock> BodyText(NewObject<UWacomCardDetailRichTextBlock>());
	TestFalse(TEXT("Decorator is not registered before card detail text is assigned"),
		BodyText->HasCardDetailDecoratorRegistered());

	const FText Markup = FText::FromString(
		TEXT("<wacom-status tag=\"Status.Poison\" label=\"中毒\"/> <Status>中毒</>"));
	BodyText->SetCardDetailRichText(Markup, Theme.Get());

	TestTrue(TEXT("Card detail rich text block registers inline decorator"),
		BodyText->HasCardDetailDecoratorRegistered());
	TestTrue(TEXT("Card detail rich text block keeps theme reference"),
		BodyText->GetCardDetailTheme() == Theme.Get());
	TestEqual(TEXT("Assigned rich text is preserved"),
		BodyText->GetText().ToString(),
		Markup.ToString());

	BodyText->TakeWidget();

	return true;
}
