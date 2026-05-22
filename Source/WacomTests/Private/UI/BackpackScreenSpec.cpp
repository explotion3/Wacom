// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Blueprint/DragDropOperation.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomBackpackScreenPresenter.h"
#include "UI/Backpack/WacomCardDragOperation.h"
#include "UI/Backpack/WacomBackpackZoneSectionWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "UI/Backpack/WacomDeleteZoneDropTarget.h"
#include "UI/Backpack/WacomSpecialZoneWidget.h"
#include "UI/Backpack/WacomZoneDropTarget.h"
#include "UI/Card/WacomCardEffectBadgeWidget.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Card/WacomCardDetailSectionWidget.h"
#include "UI/Card/WacomCardPresentationBuilder.h"
#include "UI/Card/WacomCardView.h"

#include "Cards/CardDefinition.h"
#include "RunSession.h"
#include "Tags/WacomGameplayTags.h"

#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardViewUnboundFallbackSpec,
	"Wacom.UI.Backpack.CardViewUnboundFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardViewUnboundFallbackSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardView> CardView(NewObject<UWacomCardView>());

	FWacomCardViewData Data;
	Data.Name = FText::FromString(TEXT("测试卡"));
	Data.TypeText = FText::FromString(TEXT("Companion"));
	Data.Description = FText::FromString(TEXT("测试描述"));
	Data.Cost = 2;
	Data.bShowCost = true;
	Data.bDisabled = true;

	CardView->SetCardViewData(Data);

	TestEqual(TEXT("Unbound CardView preserves data name"),
		CardView->GetCardViewData().Name.ToString(),
		Data.Name.ToString());
	TestEqual(TEXT("Unbound CardView preserves data cost"),
		CardView->GetCardViewData().Cost,
		Data.Cost);
	TestTrue(TEXT("Unbound CardView preserves disabled flag"),
		CardView->GetCardViewData().bDisabled);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardViewBuildSummarySpec,
	"Wacom.UI.Backpack.CardViewBuildSummary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardViewBuildSummarySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	Card->CardId = TEXT("Shuoguangdie");
	Card->DisplayName = FText::FromString(TEXT("烁光蝶"));
	Card->BaseCost = 1;
	Card->Rarity = WacomTags::Card_Rarity_White;
	Card->Keywords.AddTag(WacomTags::Card_Keyword_Companion);
	Card->Keywords.AddTag(WacomTags::Card_Keyword_Weapon);
	Card->Physique.MaxHpBonus = 6;

	FCardEffect Damage;
	Damage.EffectType = WacomTags::Effect_Damage;
	Damage.Magnitude = 7;
	Card->Effects.Add(Damage);

	FCardEffect Freeze;
	Freeze.EffectType = WacomTags::Effect_ApplyStatus_Freeze;
	Freeze.Magnitude = 1;
	Card->Effects.Add(Freeze);

	const FWacomCardViewData Data = UWacomCardPresentationBuilder::BuildCardViewData(Card.Get());

	TestEqual(TEXT("Summary name"), Data.Name.ToString(), TEXT("烁光蝶"));
	TestEqual(TEXT("Summary cost"), Data.Cost, 1);
	TestTrue(TEXT("White rarity exposes value"), Data.bShowValue);
	TestEqual(TEXT("White rarity value"), Data.Value, 1);
	TestTrue(TEXT("Physique summary visible"), Data.bShowPhysique);
	TestTrue(TEXT("Physique summary contains max hp"), Data.PhysiqueText.ToString().Contains(TEXT("6")));
	TestTrue(TEXT("Keyword line contains localized companion"), Data.TypeText.ToString().Contains(TEXT("伙伴")));
	TestTrue(TEXT("Keyword line contains localized weapon"), Data.TypeText.ToString().Contains(TEXT("武器")));
	TestEqual(TEXT("Two effect badges"), Data.EffectBadges.Num(), 2);
	if (Data.EffectBadges.Num() >= 2)
	{
		TestTrue(TEXT("First badge is damage"), Data.EffectBadges[0].Kind == EWacomCardViewEffectBadgeKind::Damage);
		TestEqual(TEXT("Damage badge value"), Data.EffectBadges[0].Value, 7);
		TestTrue(TEXT("Second badge is freeze"), Data.EffectBadges[1].Kind == EWacomCardViewEffectBadgeKind::Freeze);
		TestEqual(TEXT("Freeze badge value"), Data.EffectBadges[1].Value, 1);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardEffectBadgeWidgetDataSpec,
	"Wacom.UI.Backpack.CardEffectBadgeWidgetData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardEffectBadgeWidgetDataSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardEffectBadgeWidget> BadgeWidget(NewObject<UWacomCardEffectBadgeWidget>());

	FWacomCardViewEffectBadge Badge;
	Badge.Kind = EWacomCardViewEffectBadgeKind::Damage;
	Badge.Value = 7;
	Badge.DisplayText = FText::FromString(TEXT("伤7"));

	BadgeWidget->SetEffectBadgeData(Badge);
	BadgeWidget->TakeWidget();
	BadgeWidget->SetEffectBadgeData(Badge);

	TestTrue(TEXT("Badge kind preserved"),
		BadgeWidget->GetEffectBadgeData().Kind == EWacomCardViewEffectBadgeKind::Damage);
	TestEqual(TEXT("Badge value preserved"), BadgeWidget->GetEffectBadgeData().Value, 7);
	TestEqual(TEXT("Fallback ValueText shows numeric value"), BadgeWidget->GetValueText().ToString(), TEXT("7"));
	TestEqual(TEXT("Fallback LabelText is localized"), BadgeWidget->GetLabelText().ToString(), TEXT("伤害"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardViewBuilderCompatibilitySpec,
	"Wacom.UI.Backpack.CardViewBuilderCompatibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardViewBuilderCompatibilitySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	Card->CardId = TEXT("CompatibilityCard");
	Card->DisplayName = FText::FromString(TEXT("兼容测试卡"));
	Card->Description = FText::FromString(TEXT("造成 3 伤害。"));
	Card->BaseCost = 2;
	Card->Rarity = WacomTags::Card_Rarity_Blue;

	FCardEffect Damage;
	Damage.EffectType = WacomTags::Effect_Damage;
	Damage.Magnitude = 3;
	Card->Effects.Add(Damage);

	FCardPassive Passive;
	Passive.Trigger = WacomTags::Passive_Trigger_AfterPlayed;
	Passive.DisplayText = FText::FromString(TEXT("打出后：兼容测试。"));
	Card->Passives.Add(Passive);

	const FWacomCardViewData BuilderSummary = UWacomCardPresentationBuilder::BuildCardViewData(Card.Get());
	const FWacomCardViewData LegacySummary = UWacomCardView::BuildFromCardDefinition(Card.Get());

	TestEqual(TEXT("Legacy summary name matches builder"), LegacySummary.Name.ToString(), BuilderSummary.Name.ToString());
	TestEqual(TEXT("Legacy summary cost matches builder"), LegacySummary.Cost, BuilderSummary.Cost);
	TestEqual(TEXT("Legacy summary value matches builder"), LegacySummary.Value, BuilderSummary.Value);
	TestEqual(TEXT("Legacy summary badge count matches builder"), LegacySummary.EffectBadges.Num(), BuilderSummary.EffectBadges.Num());
	if (LegacySummary.EffectBadges.Num() > 0 && BuilderSummary.EffectBadges.Num() > 0)
	{
		TestTrue(TEXT("Legacy badge kind matches builder"), LegacySummary.EffectBadges[0].Kind == BuilderSummary.EffectBadges[0].Kind);
		TestEqual(TEXT("Legacy badge value matches builder"), LegacySummary.EffectBadges[0].Value, BuilderSummary.EffectBadges[0].Value);
	}

	const FWacomCardDetailViewData BuilderDetail = UWacomCardPresentationBuilder::BuildCardDetailViewData(Card.Get());
	const FWacomCardDetailViewData LegacyDetail = UWacomCardView::BuildDetailFromCardDefinition(Card.Get());

	TestEqual(TEXT("Legacy detail name matches builder"), LegacyDetail.Name.ToString(), BuilderDetail.Name.ToString());
	TestEqual(TEXT("Legacy detail description matches builder"), LegacyDetail.Description.ToString(), BuilderDetail.Description.ToString());
	TestEqual(TEXT("Legacy detail passive count matches builder"), LegacyDetail.PassiveLines.Num(), BuilderDetail.PassiveLines.Num());
	if (LegacyDetail.PassiveLines.Num() > 0 && BuilderDetail.PassiveLines.Num() > 0)
	{
		TestEqual(TEXT("Legacy detail passive matches builder"), LegacyDetail.PassiveLines[0].ToString(), BuilderDetail.PassiveLines[0].ToString());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackZoneSectionWidgetDataSpec,
	"Wacom.UI.Backpack.ZoneSectionWidgetData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackZoneSectionWidgetDataSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBackpackZoneSectionWidget> Section(NewObject<UWacomBackpackZoneSectionWidget>());

	Section->SetZoneTitleText(FText::FromString(TEXT("[ 备战区 ] 5 / 15")));
	TestNotNull(TEXT("Zone section EnsureContentHost builds fallback"), Section->EnsureContentHost());
	Section->SetZoneTitleText(FText::FromString(TEXT("[ 备战区 ] 6 / 15")));

	TestEqual(TEXT("Zone section title preserved"), Section->GetZoneTitleText().ToString(), TEXT("[ 备战区 ] 6 / 15"));
	TestNotNull(TEXT("Zone section fallback creates content host"), Section->GetContentHost());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardDetailBuildDataSpec,
	"Wacom.UI.Backpack.CardDetailBuildData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardDetailBuildDataSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	Card->CardId = TEXT("TwilightLantern");
	Card->DisplayName = FText::FromString(TEXT("暮色引虫灯"));
	Card->Description = FText::FromString(TEXT("造成1暮气，1中毒。"));

	FCardPassive Passive;
	Passive.Trigger = WacomTags::Passive_Trigger_OnCompanionCount;
	Passive.TriggerThreshold = 3;
	Passive.DisplayText = FText::FromString(TEXT("每当你打出 3 张伙伴时，使此牌回到手中。"));
	Card->Passives.Add(Passive);

	const FWacomCardDetailViewData Data = UWacomCardPresentationBuilder::BuildCardDetailViewData(Card.Get());

	TestEqual(TEXT("Detail name"), Data.Name.ToString(), TEXT("暮色引虫灯"));
	TestEqual(TEXT("Detail keeps full description"),
		Data.Description.ToString(),
		TEXT("造成1暮气，1中毒。"));
	TestFalse(TEXT("Description does not contain passive copy"), Data.Description.ToString().Contains(TEXT("被动")));
	TestEqual(TEXT("Task lines empty before schema support"), Data.TaskLines.Num(), 0);
	TestEqual(TEXT("Change lines empty before schema support"), Data.ChangeLines.Num(), 0);
	TestEqual(TEXT("One passive line"), Data.PassiveLines.Num(), 1);
	if (Data.PassiveLines.Num() > 0)
	{
		TestEqual(TEXT("Passive line uses DisplayText"), Data.PassiveLines[0].ToString(), TEXT("每当你打出 3 张伙伴时，使此牌回到手中。"));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardDetailPassiveFallbackSpec,
	"Wacom.UI.Backpack.CardDetailPassiveFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardDetailPassiveFallbackSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	Card->CardId = TEXT("PassiveFallbackCard");
	Card->DisplayName = FText::FromString(TEXT("被动回退卡"));
	Card->Description = FText::FromString(TEXT("主动效果。"));

	FCardPassive Passive;
	Passive.Trigger = WacomTags::Passive_Trigger_OnCompanionCount;
	Passive.TriggerThreshold = 3;
	Card->Passives.Add(Passive);

	const FWacomCardDetailViewData Data = UWacomCardPresentationBuilder::BuildCardDetailViewData(Card.Get());

	TestEqual(TEXT("One fallback passive line"), Data.PassiveLines.Num(), 1);
	if (Data.PassiveLines.Num() > 0)
	{
		TestTrue(TEXT("Fallback passive line contains threshold"), Data.PassiveLines[0].ToString().Contains(TEXT("3")));
		TestTrue(TEXT("Fallback passive line contains companion"), Data.PassiveLines[0].ToString().Contains(TEXT("伙伴")));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardDetailPanelFallbackSpec,
	"Wacom.UI.Backpack.CardDetailPanelFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardDetailPanelFallbackSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardDetailPanel> Panel(NewObject<UWacomCardDetailPanel>());

	FWacomCardDetailViewData Data;
	Data.Name = FText::FromString(TEXT("详情测试卡"));
	Data.Description = FText::FromString(TEXT("完整描述文本"));
	Data.PassiveLines.Add(FText::FromString(TEXT("被动：回合结束")));

	Panel->SetCardDetailData(Data);
	Panel->TakeWidget();
	Panel->SetCardDetailData(Data);

	TestEqual(TEXT("Detail panel preserves name"), Panel->GetCardDetailData().Name.ToString(), TEXT("详情测试卡"));
	TestEqual(TEXT("Detail panel preserves description"), Panel->GetCardDetailData().Description.ToString(), TEXT("完整描述文本"));
	TestEqual(TEXT("Detail panel name getter"), Panel->GetNameText().ToString(), TEXT("详情测试卡"));
	TestEqual(TEXT("Detail panel description getter"), Panel->GetDescriptionText().ToString(), TEXT("完整描述文本"));
	TestEqual(TEXT("Detail panel passive lines preserved"), Panel->GetCardDetailData().PassiveLines.Num(), 1);
	TestEqual(TEXT("Detail panel creates description and passive sections"), Panel->GetSectionCount(), 2);
	TestEqual(TEXT("First section is description"), Panel->GetSectionTitleText(0).ToString(), TEXT("描述"));
	TestEqual(TEXT("Second section is passive"), Panel->GetSectionTitleText(1).ToString(), TEXT("被动"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardDetailSectionWidgetDataSpec,
	"Wacom.UI.Backpack.CardDetailSectionWidgetData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardDetailSectionWidgetDataSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardDetailSectionWidget> SectionWidget(NewObject<UWacomCardDetailSectionWidget>());

	FWacomCardDetailSectionData Data;
	Data.Title = FText::FromString(TEXT("描述"));
	Data.Lines.Add(FText::FromString(TEXT("第一行")));
	Data.Lines.Add(FText::FromString(TEXT("第二行")));

	SectionWidget->SetSectionData(Data);
	SectionWidget->TakeWidget();
	SectionWidget->SetSectionData(Data);

	TestEqual(TEXT("Section title preserved"), SectionWidget->GetTitleText().ToString(), TEXT("描述"));
	TestEqual(TEXT("Section line count preserved"), SectionWidget->GetLineCount(), 2);
	TestEqual(TEXT("Section first line preserved"), SectionWidget->GetSectionData().Lines[0].ToString(), TEXT("第一行"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackDragOperationDefaultsSpec,
	"Wacom.UI.Backpack.DragOperationDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackDragOperationDefaultsSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, 4.5.3a DragOperation payload defaults
	TStrongObjectPtr<UWacomCardDragOperation> Op(NewObject<UWacomCardDragOperation>());

	TestFalse(TEXT("Default InstanceId invalid"), Op->InstanceId.IsValid());
	TestTrue(TEXT("Default FromZone is Backpack"), Op->FromZone == EZoneKind::Backpack);
	TestFalse(TEXT("Default FromZoneOwnerInstanceId invalid"), Op->FromZoneOwnerInstanceId.IsValid());
	TestNull(TEXT("Default Definition null"), Op->Definition.Get());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackDeleteGoldToastPreviewSpec,
	"Wacom.UI.Backpack.DeleteGoldToastPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackDeleteGoldToastPreviewSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> WhiteCard(NewObject<UCardDefinition>());
	WhiteCard->Rarity = WacomTags::Card_Rarity_White;
	TStrongObjectPtr<UCardDefinition> BlueCard(NewObject<UCardDefinition>());
	BlueCard->Rarity = WacomTags::Card_Rarity_Blue;
	TStrongObjectPtr<UCardDefinition> IntrinsicCard(NewObject<UCardDefinition>());
	IntrinsicCard->Rarity = WacomTags::Card_Rarity_Intrinsic;

	TestEqual(TEXT("White card delete toast gold preview"),
		UWacomDeleteZoneDropTarget::GetDeleteGoldRewardPreviewForToast(WhiteCard.Get()),
		URunSession::GetDeleteGoldRewardForCard(WhiteCard.Get()));
	TestEqual(TEXT("Blue card delete toast gold preview"),
		UWacomDeleteZoneDropTarget::GetDeleteGoldRewardPreviewForToast(BlueCard.Get()),
		URunSession::GetDeleteGoldRewardForCard(BlueCard.Get()));
	TestEqual(TEXT("Intrinsic card delete toast gold preview"),
		UWacomDeleteZoneDropTarget::GetDeleteGoldRewardPreviewForToast(IntrinsicCard.Get()),
		URunSession::GetDeleteGoldRewardForCard(IntrinsicCard.Get()));
	TestEqual(TEXT("Null card delete toast gold preview"),
		UWacomDeleteZoneDropTarget::GetDeleteGoldRewardPreviewForToast(nullptr),
		URunSession::GetDeleteGoldRewardForCard(nullptr));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackToastTextSpec,
	"Wacom.UI.Backpack.ToastText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackToastTextSpec::RunTest(const FString& /*Parameters*/)
{
	// Command flow extraction should keep these player-facing texts reachable via public UI wrappers.
	TestEqual(TEXT("Move success target name"),
		UWacomZoneDropTarget::FormatZoneNameForToast(EZoneKind::BattleDeck).ToString(),
		FString(TEXT("备战区")));
	TestEqual(TEXT("Move flux full reason"),
		UWacomZoneDropTarget::FormatMoveFailureReasonForToast(TEXT("FluxFull")).ToString(),
		FString(TEXT("无法移动：通量区已满。")));
	TestEqual(TEXT("Move battle deck full reason"),
		UWacomZoneDropTarget::FormatMoveFailureReasonForToast(TEXT("BattleDeckFull")).ToString(),
		FString(TEXT("无法移动：备战区已满。")));
	TestEqual(TEXT("Move unknown helper reason falls back"),
		UWacomZoneDropTarget::FormatMoveFailureReasonForToast(TEXT("RunSessionMissing")).ToString(),
		FString(TEXT("无法移动：当前规则不允许。")));
	TestEqual(TEXT("Delete missing card reason"),
		UWacomDeleteZoneDropTarget::FormatDeleteFailureReasonForToast(TEXT("MissingCard")).ToString(),
		FString(TEXT("无法销毁：没有卡牌数据。")));
	TestEqual(TEXT("Delete intrinsic reason"),
		UWacomDeleteZoneDropTarget::FormatDeleteFailureReasonForToast(TEXT("Intrinsic")).ToString(),
		FString(TEXT("无法销毁：固有卡不能被销毁。")));
	TestEqual(TEXT("Delete not owned reason"),
		UWacomDeleteZoneDropTarget::FormatDeleteFailureReasonForToast(TEXT("CardNotOwned")).ToString(),
		FString(TEXT("无法销毁：这张卡不在当前背包中。")));
	TestEqual(TEXT("Delete last bag reason"),
		UWacomDeleteZoneDropTarget::FormatDeleteFailureReasonForToast(TEXT("LastBagProvider")).ToString(),
		FString(TEXT("无法销毁：这是最后一张背包容量卡。")));
	TestEqual(TEXT("Delete last capacity provider reason"),
		UWacomDeleteZoneDropTarget::FormatDeleteFailureReasonForToast(TEXT("LastCapacityProvider")).ToString(),
		FString(TEXT("无法销毁：这是最后一张背包容量卡。")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackDeckCardDragPayloadSpec,
	"Wacom.UI.Backpack.DeckCardDragPayload",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackDeckCardDragPayloadSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, R6.1/R6.2: DeckCardWidget emits a normalized Wacom drag payload.
	TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	FCardInstance Inst;
	Inst.InstanceId = FGuid::NewGuid();
	Inst.Definition = Card.Get();

	const FGuid IgnoredOwnerId = FGuid::NewGuid();
	const EZoneKind NonSpecialZones[] =
	{
		EZoneKind::Backpack,
		EZoneKind::BattleDeck,
		EZoneKind::BurdenZone,
	};

	for (const EZoneKind Zone : NonSpecialZones)
	{
		Widget->SetCard(Inst, Zone, IgnoredOwnerId);

		UWacomCardDragOperation* DragOp = Cast<UWacomCardDragOperation>(Widget->BuildDragOperation());
		TestNotNull(TEXT("DeckCardWidget emits UWacomCardDragOperation"), DragOp);
		if (!DragOp)
		{
			continue;
		}

		TestEqual(TEXT("InstanceId copied"), DragOp->InstanceId, Inst.InstanceId);
		TestTrue(TEXT("FromZone copied"), DragOp->FromZone == Zone);
		TestFalse(TEXT("Non-SpecialZone owner id normalized to invalid"), DragOp->FromZoneOwnerInstanceId.IsValid());
		TestEqual(TEXT("Definition copied"), DragOp->Definition.Get(), Card.Get());
	}

	const FGuid SpecialOwnerId = FGuid::NewGuid();
	Widget->SetCard(Inst, EZoneKind::SpecialZone, SpecialOwnerId);

	UWacomCardDragOperation* SpecialDragOp = Cast<UWacomCardDragOperation>(Widget->BuildDragOperation());
	TestNotNull(TEXT("SpecialZone drag emits UWacomCardDragOperation"), SpecialDragOp);
	if (SpecialDragOp)
	{
		TestTrue(TEXT("SpecialZone FromZone copied"), SpecialDragOp->FromZone == EZoneKind::SpecialZone);
		TestEqual(TEXT("SpecialZone owner id preserved"), SpecialDragOp->FromZoneOwnerInstanceId, SpecialOwnerId);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackDeckCardHoverEventsSpec,
	"Wacom.UI.Backpack.DeckCardHoverEvents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackDeckCardHoverEventsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	FCardInstance Inst;
	Inst.InstanceId = FGuid::NewGuid();
	Inst.Definition = Card.Get();
	Widget->SetCard(Inst, EZoneKind::Backpack, FGuid());

	int32 HoverCount = 0;
	int32 UnhoverCount = 0;
	UWacomDeckCardWidget* LastHoverSource = nullptr;
	UWacomDeckCardWidget* LastUnhoverSource = nullptr;
	Widget->OnCardHoveredNative.AddLambda(
		[&HoverCount, &LastHoverSource](UWacomDeckCardWidget* Source)
		{
			++HoverCount;
			LastHoverSource = Source;
		});
	Widget->OnCardUnhoveredNative.AddLambda(
		[&UnhoverCount, &LastUnhoverSource](UWacomDeckCardWidget* Source)
		{
			++UnhoverCount;
			LastUnhoverSource = Source;
		});

	TestTrue(TEXT("Hover request accepted"), Widget->RequestCardHover());
	TestEqual(TEXT("Hover emitted once"), HoverCount, 1);
	TestEqual(TEXT("Hover carries source widget"), LastHoverSource, Widget.Get());

	TestTrue(TEXT("Unhover request accepted"), Widget->RequestCardUnhover());
	TestEqual(TEXT("Unhover emitted once"), UnhoverCount, 1);
	TestEqual(TEXT("Unhover carries source widget"), LastUnhoverSource, Widget.Get());

	TestTrue(TEXT("Drag start detail request accepted"), Widget->RequestDragStartedForDetail());
	TestEqual(TEXT("Drag start emits unhover"), UnhoverCount, 2);

	Widget->SetDragVisualMode(true);
	TestFalse(TEXT("Drag visual does not emit hover"), Widget->RequestCardHover());
	TestFalse(TEXT("Drag visual does not emit unhover"), Widget->RequestCardUnhover());
	TestEqual(TEXT("No extra hover from drag visual"), HoverCount, 1);
	TestEqual(TEXT("No extra unhover from drag visual"), UnhoverCount, 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackDisabledDeckCardBlocksDragAndToggleButKeepsHoverSpec,
	"Wacom.UI.Backpack.DisabledDeckCardBlocksDragAndToggleButKeepsHover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackDisabledDeckCardBlocksDragAndToggleButKeepsHoverSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	FCardInstance Inst;
	Inst.InstanceId = FGuid::NewGuid();
	Inst.Definition = Card.Get();
	Widget->SetCard(Inst, EZoneKind::SpecialZone, FGuid::NewGuid());
	Widget->SetMoveEnabled(false);
	Widget->SetRightClickToggleEnabled(true);

	int32 HoverCount = 0;
	int32 ToggleCount = 0;
	Widget->OnCardHoveredNative.AddLambda(
		[&HoverCount](UWacomDeckCardWidget*)
		{
			++HoverCount;
		});
	Widget->OnBattleEnabledToggleRequestedNative.AddLambda(
		[&ToggleCount](FGuid)
		{
			++ToggleCount;
		});

	TestNull(TEXT("Disabled card cannot build drag operation"), Widget->BuildDragOperation());
	TestFalse(TEXT("Disabled card cannot request right-click toggle"), Widget->RequestBattleEnabledToggle());
	TestEqual(TEXT("Disabled card emits no toggle request"), ToggleCount, 0);
	TestTrue(TEXT("Disabled card still emits hover"), Widget->RequestCardHover());
	TestEqual(TEXT("Hover still broadcasts once"), HoverCount, 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardDetailPanelPositionSpec,
	"Wacom.UI.Backpack.CardDetailPanelPosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardDetailPanelPositionSpec::RunTest(const FString& /*Parameters*/)
{
	const FVector2D LayerSize(1000.f, 700.f);
	const FVector2D PanelSize(360.f, 420.f);
	const FVector2D AnchorSize(260.f, 380.f);

	const FVector2D Right = UWacomBackpackScreenPresenter::ComputeCardDetailPanelPosition(
		FVector2D(100.f, 80.f),
		AnchorSize,
		LayerSize,
		PanelSize,
		12.f);
	TestEqual(TEXT("Detail panel prefers right side"), Right.X, 372.0);
	TestEqual(TEXT("Detail panel keeps top alignment"), Right.Y, 80.0);

	const FVector2D Left = UWacomBackpackScreenPresenter::ComputeCardDetailPanelPosition(
		FVector2D(700.f, 80.f),
		AnchorSize,
		LayerSize,
		PanelSize,
		12.f);
	TestEqual(TEXT("Detail panel flips to left when right side overflows"), Left.X, 328.0);

	const FVector2D Clamped = UWacomBackpackScreenPresenter::ComputeCardDetailPanelPosition(
		FVector2D(900.f, 650.f),
		AnchorSize,
		LayerSize,
		PanelSize,
		12.f);
	TestEqual(TEXT("Detail panel clamps x within layer"), Clamped.X, 528.0);
	TestEqual(TEXT("Detail panel clamps y within layer"), Clamped.Y, 280.0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardDetailHoverShowsPanelSpec,
	"Wacom.UI.Backpack.CardDetailHoverShowsPanel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardDetailHoverShowsPanelSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBackpackScreen> Screen(NewObject<UWacomBackpackScreen>());
	TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	Card->CardId = TEXT("DetailHoverCard");
	Card->DisplayName = FText::FromString(TEXT("悬停详情卡"));
	Card->Description = FText::FromString(TEXT("悬停时显示完整描述"));

	FCardInstance Inst;
	Inst.InstanceId = FGuid::NewGuid();
	Inst.Definition = Card.Get();
	Widget->SetCard(Inst, EZoneKind::Backpack, FGuid());

	Screen->TakeWidget();
	TestTrue(TEXT("BackpackScreen can show detail panel for hovered card"), Screen->ShowCardDetailForCardWidget(Widget.Get()));
	TestTrue(TEXT("Detail panel visible after hover"), Screen->IsCardDetailPanelVisible());
	TestEqual(TEXT("Detail panel receives card name"), Screen->GetCardDetailPanelNameText().ToString(), TEXT("悬停详情卡"));

	Screen->HideCardDetailPanel();
	TestFalse(TEXT("Detail panel hidden on request"), Screen->IsCardDetailPanelVisible());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackDeckCardDragRejectsIncompletePayloadSpec,
	"Wacom.UI.Backpack.DeckCardDragRejectsIncompletePayload",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackDeckCardDragRejectsIncompletePayloadSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, R6.2: Drag source does not emit payload without both card and instance id.
	TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
	TestNull(TEXT("Empty widget has no drag operation"), Widget->BuildDragOperation());

	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	FCardInstance Inst;
	Inst.Definition = Card.Get();
	Widget->SetCard(Inst, EZoneKind::Backpack, FGuid::NewGuid());

	TestNull(TEXT("Invalid instance id has no drag operation"), Widget->BuildDragOperation());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackDropTargetRejectsForeignOperationSpec,
	"Wacom.UI.Backpack.DropTargetRejectsForeignOperation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackDropTargetRejectsForeignOperationSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, R6.3: DropTarget rejects non-Wacom operations before touching RunSession.
	TStrongObjectPtr<UWacomZoneDropTarget> Target(NewObject<UWacomZoneDropTarget>());
	Target->Configure(EZoneKind::BattleDeck, FGuid::NewGuid());

	TStrongObjectPtr<UDragDropOperation> ForeignOperation(NewObject<UDragDropOperation>());

	TestFalse(TEXT("Null operation rejected"), Target->TryHandleDropOperation(nullptr));
	TestFalse(TEXT("Foreign drag operation rejected"), Target->TryHandleDropOperation(ForeignOperation.Get()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackBurdenZoneIsSourceOnlySpec,
	"Wacom.UI.Backpack.BurdenZoneIsSourceOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackBurdenZoneIsSourceOnlySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	FCardInstance Inst;
	Inst.InstanceId = FGuid::NewGuid();
	Inst.Definition = Card.Get();
	Widget->SetCard(Inst, EZoneKind::BurdenZone, FGuid::NewGuid());

	UWacomCardDragOperation* DragOp = Cast<UWacomCardDragOperation>(Widget->BuildDragOperation());
	TestNotNull(TEXT("BurdenZone card can still start as drag source"), DragOp);
	if (DragOp)
	{
		TestEqual(TEXT("BurdenZone drag payload preserves source zone"), DragOp->FromZone, EZoneKind::BurdenZone);
		TestFalse(TEXT("BurdenZone drag payload has no owner id"), DragOp->FromZoneOwnerInstanceId.IsValid());
	}

	TestFalse(TEXT("BurdenZone target rejects Backpack-origin preview"),
		UWacomZoneDropTarget::ShouldPreviewDrop(EZoneKind::BurdenZone, EZoneKind::Backpack, 0, 10));
	TestFalse(TEXT("BurdenZone target rejects BattleDeck-origin preview"),
		UWacomZoneDropTarget::ShouldPreviewDrop(EZoneKind::BurdenZone, EZoneKind::BattleDeck, 0, 10));
	TestFalse(TEXT("BurdenZone target rejects BurdenZone-origin in-place preview"),
		UWacomZoneDropTarget::ShouldPreviewDrop(EZoneKind::BurdenZone, EZoneKind::BurdenZone, 0, 10));

	TStrongObjectPtr<UWacomZoneDropTarget> BurdenTarget(NewObject<UWacomZoneDropTarget>());
	BurdenTarget->Configure(EZoneKind::BurdenZone, FGuid());
	TestFalse(TEXT("BurdenZone background has no owner screen and cannot accept drop"),
		BurdenTarget->TryHandleDropOperation(DragOp));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackDeleteZoneUsesInstanceIdPayloadSpec,
	"Wacom.UI.Backpack.DeleteZoneUsesInstanceIdPayload",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackDeleteZoneUsesInstanceIdPayloadSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardDragOperation> CardOp(NewObject<UWacomCardDragOperation>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	const FGuid InstanceId = FGuid::NewGuid();

	CardOp->InstanceId = InstanceId;
	CardOp->FromZone = EZoneKind::BurdenZone;
	CardOp->Definition = Card.Get();

	TestEqual(TEXT("DeleteZone request helper uses InstanceId, not Definition identity"),
		UWacomDeleteZoneDropTarget::GetDeleteInstanceIdForRequest(*CardOp),
		InstanceId);

	CardOp->InstanceId = FGuid();
	TestFalse(TEXT("DeleteZone request helper rejects invalid InstanceId"),
		UWacomDeleteZoneDropTarget::GetDeleteInstanceIdForRequest(*CardOp).IsValid());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackDeckCardMoveClickUnboundSpec,
	"Wacom.UI.Backpack.DeckCardMoveClickUnbound",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackDeckCardMoveClickUnboundSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, R6.6: Main card button is a display/drag hotspot, not a click-to-move command.
	TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
	Widget->TakeWidget();

	TestFalse(TEXT("MoveButton has no click bindings"), Widget->HasMoveButtonClickBindings());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackDragOperationPayloadSpec,
	"Wacom.UI.Backpack.DragOperationPayload",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackDragOperationPayloadSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, DragOperation carries stable instance source fields.
	TStrongObjectPtr<UWacomCardDragOperation> Op(NewObject<UWacomCardDragOperation>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	const FGuid InstanceId = FGuid::NewGuid();
	const FGuid OwnerId = FGuid::NewGuid();
	Op->InstanceId = InstanceId;
	Op->FromZone = EZoneKind::SpecialZone;
	Op->FromZoneOwnerInstanceId = OwnerId;
	Op->Definition = Card.Get();

	TestEqual(TEXT("InstanceId payload"), Op->InstanceId, InstanceId);
	TestTrue(TEXT("FromZone payload"), Op->FromZone == EZoneKind::SpecialZone);
	TestEqual(TEXT("Owner payload"), Op->FromZoneOwnerInstanceId, OwnerId);
	TestEqual(TEXT("Definition payload"), Op->Definition.Get(), Card.Get());

	Op->FromZone = EZoneKind::Backpack;
	Op->FromZoneOwnerInstanceId = FGuid();
	TestFalse(TEXT("Non-SpecialZone owner invalid by convention"), Op->FromZoneOwnerInstanceId.IsValid());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackStorageCardViewPayloadSpec,
	"Wacom.UI.Backpack.StorageCardViewPayload",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackStorageCardViewPayloadSpec::RunTest(const FString& /*Parameters*/)
{
	// BackpackScreen 现在从 FRunBackpackStorageSnapshot 读取列表；
	// 子 widget 仍应把 FRunStorageCardView 的物理归属字段原样转成拖拽 payload。
	TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	FRunStorageCardView View;
	View.Instance.InstanceId = FGuid::NewGuid();
	View.Instance.Definition = Card.Get();
	View.PhysicalZone = EZoneKind::SpecialZone;
	View.ZoneOwnerInstanceId = FGuid::NewGuid();

	Widget->SetCard(View.Instance, View.PhysicalZone, View.ZoneOwnerInstanceId);

	UWacomCardDragOperation* DragOp = Cast<UWacomCardDragOperation>(Widget->BuildDragOperation());
	TestNotNull(TEXT("StorageCardView-backed card emits drag operation"), DragOp);
	if (!DragOp)
	{
		return false;
	}

	TestEqual(TEXT("StorageCardView instance id copied"), DragOp->InstanceId, View.Instance.InstanceId);
	TestTrue(TEXT("StorageCardView physical zone copied"), DragOp->FromZone == View.PhysicalZone);
	TestEqual(TEXT("StorageCardView owner id copied"), DragOp->FromZoneOwnerInstanceId, View.ZoneOwnerInstanceId);
	TestEqual(TEXT("StorageCardView definition copied"), DragOp->Definition.Get(), Card.Get());

	Widget->SetProjectedFromBadgeText(FText::FromString(TEXT("来自 蛛茧绒囊")));
	TestTrue(TEXT("Projected badge still available for snapshot projection cards"), Widget->IsProjectedFromBadgeVisible());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackSpecialZoneWidgetSnapshotSpec,
	"Wacom.UI.Backpack.SpecialZoneWidgetSnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackSpecialZoneWidgetSnapshotSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomSpecialZoneWidget> Widget(NewObject<UWacomSpecialZoneWidget>());
	TStrongObjectPtr<UCardDefinition> OwnerCard(NewObject<UCardDefinition>());
	TStrongObjectPtr<UCardDefinition> ContentCard(NewObject<UCardDefinition>());

	OwnerCard->DisplayName = FText::FromString(TEXT("蛛茧绒囊"));
	ContentCard->DisplayName = FText::FromString(TEXT("内容卡"));

	const FGuid OwnerId = FGuid::NewGuid();
	const FGuid ContentId = FGuid::NewGuid();

	FRunSpecialStorageView View;
	View.Capacity = 2;
	View.bOwnerInBattleDeck = true;
	View.OwnerCard.Instance.InstanceId = OwnerId;
	View.OwnerCard.Instance.Definition = OwnerCard.Get();
	View.OwnerCard.PhysicalZone = EZoneKind::BattleDeck;
	View.OwnerCard.bIsPhysicalInBattleDeck = true;
	View.OwnerCard.bIsContainer = true;
	View.OwnerCard.bIsTypeBContainer = true;

	FRunStorageCardView ContentView;
	ContentView.Instance.InstanceId = ContentId;
	ContentView.Instance.Definition = ContentCard.Get();
	ContentView.Instance.bBattleEnabledInSpecialZone = true;
	ContentView.PhysicalZone = EZoneKind::SpecialZone;
	ContentView.ZoneOwnerInstanceId = OwnerId;
	View.ContentCards.Add(ContentView);

	int32 ToggleCount = 0;
	FGuid LastToggledId;
	Widget->OnBattleEnabledToggleRequestedNative.AddLambda(
		[&ToggleCount, &LastToggledId](FGuid InstanceId)
		{
			++ToggleCount;
			LastToggledId = InstanceId;
		});

	Widget->SetSpecialZoneView(View, nullptr, UWacomDeckCardWidget::StaticClass());

	const FString Title = Widget->GetZoneTitleText().ToString();
	TestTrue(TEXT("SpecialZoneWidget title includes owner"), Title.Contains(TEXT("蛛茧绒囊")));
	TestTrue(TEXT("SpecialZoneWidget title includes count/capacity"), Title.Contains(TEXT("1 / 2")));
	TestTrue(TEXT("Battle ready badge visible when owner is in BattleDeck"), Widget->IsBattleReadyBadgeVisible());

	View.bOwnerInBattleDeck = false;
	View.OwnerCard.PhysicalZone = EZoneKind::Backpack;
	View.OwnerCard.bIsPhysicalInBattleDeck = false;
	Widget->SetSpecialZoneView(View, nullptr, UWacomDeckCardWidget::StaticClass());
	TestFalse(TEXT("Battle ready badge hidden when owner is in Backpack"), Widget->IsBattleReadyBadgeVisible());

	View.bOwnerInBattleDeck = true;
	View.OwnerCard.PhysicalZone = EZoneKind::BattleDeck;
	View.OwnerCard.bIsPhysicalInBattleDeck = true;
	Widget->SetSpecialZoneView(View, nullptr, UWacomDeckCardWidget::StaticClass());

	UWacomCardDragOperation* OwnerDragOp = Cast<UWacomCardDragOperation>(Widget->BuildOwnerCardDragOperation());
	TestNotNull(TEXT("Owner card emits drag operation"), OwnerDragOp);
	if (OwnerDragOp)
	{
		TestEqual(TEXT("Owner drag instance id"), OwnerDragOp->InstanceId, OwnerId);
		TestTrue(TEXT("Owner drag zone is BattleDeck"), OwnerDragOp->FromZone == EZoneKind::BattleDeck);
		TestFalse(TEXT("Owner drag has no SpecialZone owner id"), OwnerDragOp->FromZoneOwnerInstanceId.IsValid());
	}

	UWacomCardDragOperation* ContentDragOp = Cast<UWacomCardDragOperation>(Widget->BuildContentCardDragOperation(0));
	TestNotNull(TEXT("Content card emits drag operation"), ContentDragOp);
	if (ContentDragOp)
	{
		TestEqual(TEXT("Content drag instance id"), ContentDragOp->InstanceId, ContentId);
		TestTrue(TEXT("Content drag zone is SpecialZone"), ContentDragOp->FromZone == EZoneKind::SpecialZone);
		TestEqual(TEXT("Content drag owner id"), ContentDragOp->FromZoneOwnerInstanceId, OwnerId);
	}

	TestTrue(TEXT("Content toggle request accepted"), Widget->RequestContentCardBattleEnabledToggle(0));
	TestEqual(TEXT("Content toggle emits once"), ToggleCount, 1);
	TestEqual(TEXT("Content toggle carries content id"), LastToggledId, ContentId);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackSpecialZoneBattleEnabledBadgeSpec,
	"Wacom.UI.Backpack.SpecialZoneBattleEnabledBadge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackSpecialZoneBattleEnabledBadgeSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, R6.7/R6.10: SpecialZone cards expose an identifiable battle-enabled badge.
	TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	FCardInstance Inst;
	Inst.InstanceId = FGuid::NewGuid();
	Inst.Definition = Card.Get();
	Inst.bBattleEnabledInSpecialZone = true;

	Widget->SetCard(Inst, EZoneKind::SpecialZone, FGuid::NewGuid());
	Widget->TakeWidget();

	TestTrue(TEXT("BattleEnabledBadge visible for selected SpecialZone card"), Widget->IsBattleEnabledBadgeVisible());

	Inst.bBattleEnabledInSpecialZone = false;
	Widget->SetCard(Inst, EZoneKind::SpecialZone, FGuid::NewGuid());
	TestFalse(TEXT("BattleEnabledBadge collapsed for unselected SpecialZone card"), Widget->IsBattleEnabledBadgeVisible());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackSpecialZoneTitleAndReadyBadgeSpec,
	"Wacom.UI.Backpack.SpecialZoneTitleAndReadyBadge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackSpecialZoneTitleAndReadyBadgeSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, R6.7/R6.13: SpecialZone section title and battle-ready badge are deterministic.
	const FString Title = UWacomBackpackScreenPresenter::BuildSpecialZoneTitleText(
		FText::FromString(TEXT("蛛茧绒囊")),
		1,
		2).ToString();

	TestTrue(TEXT("SpecialZone title includes owner name"), Title.Contains(TEXT("蛛茧绒囊")));
	TestTrue(TEXT("SpecialZone title includes count/capacity"), Title.Contains(TEXT("1 / 2")));
	TestTrue(
		TEXT("BattleReady badge visible when owner is in BattleDeck"),
		UWacomBackpackScreenPresenter::GetSpecialZoneBattleReadyBadgeVisibility(EZoneKind::BattleDeck) != ESlateVisibility::Collapsed);
	TestEqual(
		TEXT("BattleReady badge collapsed when owner is in Backpack"),
		UWacomBackpackScreenPresenter::GetSpecialZoneBattleReadyBadgeVisibility(EZoneKind::Backpack),
		ESlateVisibility::Collapsed);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackProjectedFromBadgeSpec,
	"Wacom.UI.Backpack.ProjectedFromBadge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackProjectedFromBadgeSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, R6.10: BattleDeck projection keeps a visible source-owner badge.
	TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
	Widget->TakeWidget();

	TestFalse(TEXT("ProjectedFromBadge hidden by default"), Widget->IsProjectedFromBadgeVisible());

	const FText SourceText = FText::FromString(TEXT("来自 蛛茧绒囊"));
	Widget->SetProjectedFromBadgeText(SourceText);

	TestTrue(TEXT("ProjectedFromBadge visible when text is set"), Widget->IsProjectedFromBadgeVisible());
	TestEqual(TEXT("ProjectedFromBadge text preserved"), Widget->GetProjectedFromBadgeText().ToString(), SourceText.ToString());

	Widget->SetProjectedFromBadgeText(FText::GetEmpty());
	TestFalse(TEXT("ProjectedFromBadge collapsed when text is cleared"), Widget->IsProjectedFromBadgeVisible());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackProjectedFromBadgePresenterSpec,
	"Wacom.UI.Backpack.ProjectedFromBadgePresenter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackProjectedFromBadgePresenterSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> OwnerCard(NewObject<UCardDefinition>());
	OwnerCard->DisplayName = FText::FromString(TEXT("蛛茧绒囊"));

	FRunBackpackStorageSnapshot Snapshot;
	FRunSpecialStorageView SpecialView;
	SpecialView.OwnerCard.Instance.InstanceId = FGuid::NewGuid();
	SpecialView.OwnerCard.Instance.Definition = OwnerCard.Get();
	Snapshot.SpecialZones.Add(SpecialView);

	FRunStorageCardView ProjectedCard;
	ProjectedCard.ZoneOwnerInstanceId = SpecialView.OwnerCard.Instance.InstanceId;

	const FText BadgeText = UWacomBackpackScreenPresenter::BuildBattleDeckProjectedFromBadgeText(ProjectedCard, Snapshot);
	TestEqual(TEXT("Projected badge text uses special zone owner name"), BadgeText.ToString(), TEXT("来自 蛛茧绒囊"));

	ProjectedCard.ZoneOwnerInstanceId = FGuid::NewGuid();
	TestTrue(
		TEXT("Projected badge text is empty when owner cannot be found"),
		UWacomBackpackScreenPresenter::BuildBattleDeckProjectedFromBadgeText(ProjectedCard, Snapshot).IsEmpty());

	TestEqual(
		TEXT("Direct projected badge text helper formats owner name"),
		UWacomBackpackScreenPresenter::BuildProjectedFromBadgeText(FText::FromString(TEXT("引虫灯"))).ToString(),
		TEXT("来自 引虫灯"));
	TestTrue(
		TEXT("Direct projected badge text helper keeps empty owner empty"),
		UWacomBackpackScreenPresenter::BuildProjectedFromBadgeText(FText::GetEmpty()).IsEmpty());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackPresenterCompatibilitySpec,
	"Wacom.UI.Backpack.PresenterCompatibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackPresenterCompatibilitySpec::RunTest(const FString& /*Parameters*/)
{
	const FText OwnerName = FText::FromString(TEXT("兼容主卡"));
	TestEqual(
		TEXT("Legacy special title helper forwards to presenter"),
		UWacomBackpackScreen::BuildSpecialZoneTitleText(OwnerName, 2, 5).ToString(),
		UWacomBackpackScreenPresenter::BuildSpecialZoneTitleText(OwnerName, 2, 5).ToString());

	TestEqual(
		TEXT("Legacy burden title helper forwards to presenter"),
		UWacomBackpackScreen::BuildBurdenZoneTitleText(4).ToString(),
		UWacomBackpackScreenPresenter::BuildBurdenZoneTitleText(4).ToString());

	TestEqual(
		TEXT("Legacy battle-ready visibility helper forwards to presenter"),
		UWacomBackpackScreen::GetSpecialZoneBattleReadyBadgeVisibility(EZoneKind::BattleDeck),
		UWacomBackpackScreenPresenter::GetSpecialZoneBattleReadyBadgeVisibility(EZoneKind::BattleDeck));

	const FVector2D LegacyPosition = UWacomBackpackScreen::ComputeCardDetailPanelPosition(
		FVector2D(100.f, 80.f),
		FVector2D(260.f, 380.f),
		FVector2D(1000.f, 700.f),
		FVector2D(360.f, 420.f),
		12.f);
	const FVector2D PresenterPosition = UWacomBackpackScreenPresenter::ComputeCardDetailPanelPosition(
		FVector2D(100.f, 80.f),
		FVector2D(260.f, 380.f),
		FVector2D(1000.f, 700.f),
		FVector2D(360.f, 420.f),
		12.f);
	TestEqual(TEXT("Legacy detail position helper forwards to presenter X"), LegacyPosition.X, PresenterPosition.X);
	TestEqual(TEXT("Legacy detail position helper forwards to presenter Y"), LegacyPosition.Y, PresenterPosition.Y);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackBattleEnabledToggleRequestSpec,
	"Wacom.UI.Backpack.BattleEnabledToggleRequest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackBattleEnabledToggleRequestSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, R6.11: right-click toggle path emits one request for the card instance.
	TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	FCardInstance Inst;
	Inst.InstanceId = FGuid::NewGuid();
	Inst.Definition = Card.Get();
	Widget->SetCard(Inst, EZoneKind::SpecialZone, FGuid::NewGuid());

	int32 ToggleCount = 0;
	FGuid LastToggledId;
	Widget->OnBattleEnabledToggleRequestedNative.AddLambda(
		[&ToggleCount, &LastToggledId](FGuid InstanceId)
		{
			++ToggleCount;
			LastToggledId = InstanceId;
		});

	TestFalse(TEXT("Toggle disabled by default"), Widget->RequestBattleEnabledToggle());
	TestEqual(TEXT("No request emitted while disabled"), ToggleCount, 0);

	Widget->SetRightClickToggleEnabled(true);
	TestTrue(TEXT("Toggle request accepted when enabled"), Widget->RequestBattleEnabledToggle());
	TestEqual(TEXT("One toggle request emitted"), ToggleCount, 1);
	TestEqual(TEXT("Toggle request carries instance id"), LastToggledId, Inst.InstanceId);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackBattleDeckFullPreviewRejectsBackpackDropSpec,
	"Wacom.UI.Backpack.BattleDeckFullPreviewRejectsBackpackDrop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackBattleDeckFullPreviewRejectsBackpackDropSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, R6.12: BattleDeck preview rejects Backpack-origin drops when capacity is full.
	TestFalse(
		TEXT("Full BattleDeck rejects Backpack-origin preview"),
		UWacomZoneDropTarget::ShouldPreviewDrop(EZoneKind::BattleDeck, EZoneKind::Backpack, 2, 2));

	TestTrue(
		TEXT("BattleDeck accepts Backpack-origin preview when there is capacity"),
		UWacomZoneDropTarget::ShouldPreviewDrop(EZoneKind::BattleDeck, EZoneKind::Backpack, 1, 2));

	TestTrue(
		TEXT("BattleDeck in-place preview is not rejected by capacity"),
		UWacomZoneDropTarget::ShouldPreviewDrop(EZoneKind::BattleDeck, EZoneKind::BattleDeck, 2, 2));

	TestTrue(
		TEXT("Non-BattleDeck target preview is not rejected by BattleDeck capacity"),
		UWacomZoneDropTarget::ShouldPreviewDrop(EZoneKind::SpecialZone, EZoneKind::Backpack, 2, 2));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackBurdenZoneTitleAndCardOrderSpec,
	"Wacom.UI.Backpack.BurdenZoneTitleAndCardOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackBurdenZoneTitleAndCardOrderSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, R6.8: BurdenZone title count and rendered card widgets preserve instance order.
	TestTrue(
		TEXT("BurdenZone title includes card count"),
		UWacomBackpackScreenPresenter::BuildBurdenZoneTitleText(3).ToString().Contains(TEXT("3")));
	TestEqual(
		TEXT("BurdenZone hidden when empty"),
		UWacomBackpackScreenPresenter::GetBurdenZoneVisibility(0),
		ESlateVisibility::Collapsed);
	TestEqual(
		TEXT("BurdenZone visible when cards overflow"),
		UWacomBackpackScreenPresenter::GetBurdenZoneVisibility(1),
		ESlateVisibility::SelfHitTestInvisible);

	TArray<FCardInstance> BurdenCards;
	TArray<TStrongObjectPtr<UCardDefinition>> CardDefinitions;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
		FCardInstance Inst;
		Inst.InstanceId = FGuid::NewGuid();
		Inst.Definition = Card.Get();

		CardDefinitions.Add(MoveTemp(Card));
		BurdenCards.Add(Inst);
	}

	for (int32 Index = 0; Index < BurdenCards.Num(); ++Index)
	{
		TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
		Widget->SetCard(BurdenCards[Index], EZoneKind::BurdenZone, FGuid::NewGuid());

		UWacomCardDragOperation* DragOp = Cast<UWacomCardDragOperation>(Widget->BuildDragOperation());
		TestNotNull(TEXT("Burden card widget emits drag operation"), DragOp);
		if (!DragOp)
		{
			continue;
		}

		TestEqual(TEXT("Burden card order preserves instance id"), DragOp->InstanceId, BurdenCards[Index].InstanceId);
		TestEqual(TEXT("Burden card order preserves definition"), DragOp->Definition.Get(), BurdenCards[Index].Definition.Get());
		TestTrue(TEXT("Burden card source zone is BurdenZone"), DragOp->FromZone == EZoneKind::BurdenZone);
		TestFalse(TEXT("Burden card owner id normalized to invalid"), DragOp->FromZoneOwnerInstanceId.IsValid());
	}

	return true;
}
