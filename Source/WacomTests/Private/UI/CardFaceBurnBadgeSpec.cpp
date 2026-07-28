// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Card/WacomCardPresentationBuilder.h"

#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUICardFaceBurnBadgeSpec,
	"Wacom.UI.CardPresentation.CardFace.BurnBadge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUICardFaceBurnBadgeSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	Card->CardId = TEXT("UI.CardPresentation.BurnBadge");
	Card->DisplayName = FText::FromString(TEXT("灼烧徽章测试卡"));

	FCardEffect Burn;
	Burn.EffectType = WacomTags::Effect_ApplyStatus_Burn;
	Burn.Magnitude = 7;
	Card->Effects.Add(Burn);

	const FWacomCardViewData ViewData =
		UWacomCardPresentationBuilder::BuildCardViewData(Card.Get());

	TestEqual(TEXT("Burn effect produces exactly one card-face badge"),
		ViewData.EffectBadges.Num(), 1);
	if (!ViewData.EffectBadges.IsValidIndex(0))
	{
		return false;
	}

	const FWacomCardViewEffectBadge& Badge = ViewData.EffectBadges[0];
	TestEqual(TEXT("Burn badge keeps the Burn semantic kind"),
		Badge.Kind, EWacomCardViewEffectBadgeKind::Burn);
	TestEqual(TEXT("Burn badge keeps the resolved magnitude"), Badge.Value, 7);
	TestEqual(TEXT("Burn badge uses a stable presentation identity"),
		Badge.PresentationKey, FName(TEXT("Badge.Burn")));
	TestEqual(TEXT("Burn badge keeps its compact fallback label"),
		Badge.DisplayText.ToString(), FString(TEXT("灼7")));
	return true;
}
