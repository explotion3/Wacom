// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "PaperSprite.h"
#include "UI/CardViewSpecReceiver.h"
#include "UI/CardViewTestAccess.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomShopUpgradeValueEmphasisSpec,
	"Wacom.UI.Shop.UpgradePresentation.IncreasedBadgeUsesPassiveTint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomShopUpgradeValueEmphasisSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardEffectBadgeSpecProbe> Badge(
		NewObject<UWacomCardEffectBadgeSpecProbe>());
	UPaperSprite* Zero = NewObject<UPaperSprite>(Badge.Get());
	UPaperSprite* Five = NewObject<UPaperSprite>(Badge.Get());
	Badge->SetDigitSpriteForTest(0, Zero);
	Badge->SetDigitSpriteForTest(5, Five);
	Badge->TakeWidget();

	FWacomCardViewEffectBadge Data;
	Data.PresentationKey = TEXT("Badge.Damage");
	Data.Kind = EWacomCardViewEffectBadgeKind::Damage;
	Data.Value = 5;
	Data.ValueEmphasis = EWacomCardViewValueEmphasis::Increased;
	Badge->SetEffectBadgeData(Data);

	const FWacomCardEffectBadgeAutomationTestView View =
		FWacomCardViewTestAccess::View(*Badge);
	TestTrue(TEXT("Increased comparison values use the passive positive tint"),
		View.DigitTint.Equals(FLinearColor(0.25f, 1.0f, 0.62f, 1.0f)));
	TestFalse(TEXT("Static comparison tint does not activate battle feedback material"),
		View.bFeedbackMaterialActive);
	return true;
}

#endif
