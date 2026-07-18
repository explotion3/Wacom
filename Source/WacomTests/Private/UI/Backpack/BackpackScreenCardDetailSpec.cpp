// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Fixtures/WacomRunExplorationFixture.h"

#include "../BackpackScreenTestAccess.h"

#include "Blueprint/WidgetTree.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "UI/Card/WacomCardEffectBadgeWidget.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Card/WacomCardDetailPlainTextRenderer.h"
#include "UI/Card/WacomCardDetailSectionWidget.h"
#include "UI/Card/WacomCardPresentationBuilder.h"
#include "UI/Card/WacomCardView.h"
#include "UI/CardViewTestAccess.h"
#include "UI/CardViewSpecReceiver.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Components/Image.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "PaperSprite.h"
#include "RunSession.h"
#include "Tags/WacomGameplayTags.h"

#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackCardDetailController.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceSceneBuilder.h"

#include "UObject/StrongObjectPtr.h"

namespace
{

	FWacomCardViewEffectBadge MakeCardViewEffectBadgeForTest(EWacomCardViewEffectBadgeKind Kind, int32 Value)
	{
		FWacomCardViewEffectBadge Badge;
		Badge.Kind = Kind;
		Badge.Value = Value;
		return Badge;
	}

	FWacomCardDetailBlock MakeCardDetailTextBlockForTest(
		FName BlockId,
		EWacomCardDetailBlockKind Kind,
		const FString& Text)
	{
		FWacomCardDetailRun Run;
		Run.StableId = FName(*FString::Printf(TEXT("%s.Text"), *BlockId.ToString()));
		Run.Kind = EWacomCardDetailRunKind::Text;
		Run.Text = FText::FromString(Text);

		FWacomCardDetailBlock Block;
		Block.BlockId = BlockId;
		Block.Kind = Kind;
		Block.Runs.Add(Run);
		return Block;
	}

	FString JoinCardDetailSectionTextForTest(
		const FWacomCardDetailViewData& Data,
		EWacomCardDetailSectionKind SectionKind)
	{
		FString Text;
		for (const FWacomCardDetailSection& Section : Data.Sections)
		{
			if (Section.Kind != SectionKind)
			{
				continue;
			}

			const FString SectionText =
				UWacomCardDetailPlainTextRenderer::RenderSectionPlainText(Section).ToString();
			if (!SectionText.IsEmpty())
			{
				if (!Text.IsEmpty())
				{
					Text += TEXT("\n");
				}
				Text += SectionText;
			}
		}
		return Text;
	}

	const UWacomCardEffectBadgeWidget* GetSingleSlotBadgeForTest(const UPanelWidget* Slot)
	{
		if (!Slot || Slot->GetChildrenCount() != 1)
		{
			return nullptr;
		}

		return Cast<UWacomCardEffectBadgeWidget>(Slot->GetChildAt(0));
	}

	UCardDefinition* MakeBackpackUiCardForTest(UObject* Outer, FName CardId, int32 Capacity = 0, bool bTypeB = false)
	{
		UCardDefinition* Card = NewObject<UCardDefinition>(Outer);
		Card->CardId = CardId;
		Card->DisplayName = FText::FromName(CardId);
		Card->BaseCost = 1;
		Card->Physique.Capacity = Capacity;
		if (bTypeB)
		{
			Card->Physique.CapacityEffect = WacomTags::Card_CapacityEffect_WeaponDamagePlus3;
		}
		return Card;
	}

	UCharacterDefinition* MakeBackpackUiCharacterForTest(UObject* Outer, const TArray<UCardDefinition*>& StarterDeck)
	{
		UCharacterDefinition* Character = NewObject<UCharacterDefinition>(Outer);
		Character->CharacterId = TEXT("Backpack.UI.Character");
		Character->DisplayName = FText::FromString(TEXT("背包 UI 测试角色"));
		for (UCardDefinition* Card : StarterDeck)
		{
			Character->StarterDeck.Add(Card);
		}
		return Character;
	}

	UWacomBackpackScreen* MakeBackpackUiScreenForTest(UObject* Outer, URunSession* Run)
	{
		return FWacomBackpackScreenTestAccess::Create(Outer, Run);
	}

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
	FCardEffect Poison;
	Poison.EffectType = WacomTags::Effect_ApplyStatus_Poison;
	Poison.Magnitude = 1;
	Card->Effects.Add(Poison);

	FCardPassive Passive;
	Passive.Trigger = WacomTags::Passive_Trigger_OnCompanionCount;
	Passive.TriggerThreshold = 3;
	Card->Passives.Add(Passive);

	const FWacomCardDetailViewData Data = UWacomCardPresentationBuilder::BuildCardDetailViewData(Card.Get());

	TestEqual(TEXT("Detail name"), Data.Name.ToString(), TEXT("暮色引虫灯"));
	TestEqual(TEXT("Detail description section uses explanation template"),
		JoinCardDetailSectionTextForTest(Data, EWacomCardDetailSectionKind::Description),
		TEXT("施加 1 层 中毒。"));
	TestFalse(TEXT("Description section does not contain passive copy"),
		JoinCardDetailSectionTextForTest(Data, EWacomCardDetailSectionKind::Description).Contains(TEXT("被动")));
	TestTrue(TEXT("Detail document has no task section before schema support"),
		JoinCardDetailSectionTextForTest(Data, EWacomCardDetailSectionKind::Task).IsEmpty());
	TestTrue(TEXT("Passive section uses trigger template"),
		JoinCardDetailSectionTextForTest(Data, EWacomCardDetailSectionKind::Passive)
			.Contains(TEXT("每打出 3 张伙伴：")));
	TestTrue(TEXT("Passive section uses trigger outcome template"),
		JoinCardDetailSectionTextForTest(Data, EWacomCardDetailSectionKind::Passive)
			.Contains(TEXT("使此牌回到手中。")));

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
	FCardEffect Damage;
	Damage.EffectType = WacomTags::Effect_Damage;
	Damage.Magnitude = 2;
	Card->Effects.Add(Damage);

	FCardPassive Passive;
	Passive.Trigger = WacomTags::Passive_Trigger_OnCompanionCount;
	Passive.TriggerThreshold = 3;
	Card->Passives.Add(Passive);

	const FWacomCardDetailViewData Data = UWacomCardPresentationBuilder::BuildCardDetailViewData(Card.Get());

	const FString PassiveSectionText =
		JoinCardDetailSectionTextForTest(Data, EWacomCardDetailSectionKind::Passive);
	TestTrue(TEXT("Fallback passive section contains threshold"), PassiveSectionText.Contains(TEXT("3")));
	TestTrue(TEXT("Fallback passive section contains companion"), PassiveSectionText.Contains(TEXT("伙伴")));
	TestTrue(TEXT("Fallback passive section contains outcome"), PassiveSectionText.Contains(TEXT("使此牌回到手中。")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardDetailPanelSectionsSpec,
	"Wacom.UI.Backpack.CardDetailPanelSections",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardDetailPanelSectionsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardDetailPanel> Panel(NewObject<UWacomCardDetailPanel>());

	FWacomCardDetailViewData Data;
	Data.Name = FText::FromString(TEXT("详情测试卡"));

	FWacomCardDetailSection DescriptionSection;
	DescriptionSection.SectionId = FName(TEXT("Description"));
	DescriptionSection.Kind = EWacomCardDetailSectionKind::Description;
	DescriptionSection.Title = FText::FromString(TEXT("描述"));
	DescriptionSection.Blocks.Add(MakeCardDetailTextBlockForTest(
		FName(TEXT("Description.0.Block")),
		EWacomCardDetailBlockKind::Paragraph,
		TEXT("完整描述文本")));
	Data.Sections.Add(DescriptionSection);

	FWacomCardDetailSection PassiveSection;
	PassiveSection.SectionId = FName(TEXT("Passive"));
	PassiveSection.Kind = EWacomCardDetailSectionKind::Passive;
	PassiveSection.Title = FText::FromString(TEXT("被动"));
	PassiveSection.Blocks.Add(MakeCardDetailTextBlockForTest(
		FName(TEXT("Passive.0.Block")),
		EWacomCardDetailBlockKind::Paragraph,
		TEXT("回合结束")));
	Data.Sections.Add(PassiveSection);

	Panel->SetCardDetailData(Data);
	Panel->TakeWidget();
	Panel->SetCardDetailData(Data);

	TestEqual(TEXT("Detail panel preserves name"), Panel->GetCardDetailData().Name.ToString(), TEXT("详情测试卡"));
	TestEqual(TEXT("Detail panel name getter"), Panel->GetNameText().ToString(), TEXT("详情测试卡"));
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
	Data.RichText = FText::FromString(TEXT("第一行\n第二行"));

	SectionWidget->SetSectionData(Data);
	SectionWidget->TakeWidget();
	SectionWidget->SetSectionData(Data);

	TestEqual(TEXT("Section title preserved"), SectionWidget->GetTitleText().ToString(), TEXT("描述"));
	TestEqual(TEXT("Section rich text preserved"), SectionWidget->GetBodyRichText().ToString(), TEXT("第一行\n第二行"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackToastTextSpec,
	"Wacom.UI.Backpack.ToastText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackToastTextSpec::RunTest(const FString& /*Parameters*/)
{
	// Command flow owns these player-facing texts; passive widgets never format rule failures themselves.
	TestEqual(TEXT("Move success target name"),
		FWacomBackpackScreenTestAccess::BuildMoveZoneNameText(EZoneKind::BattleDeck).ToString(),
		FString(TEXT("备战区")));
	TestEqual(TEXT("Move flux full reason"),
		FWacomBackpackScreenTestAccess::BuildMoveFailureToastText(TEXT("FluxFull")).ToString(),
		FString(TEXT("无法移动：通量区已满。")));
	TestEqual(TEXT("Move battle deck full reason"),
		FWacomBackpackScreenTestAccess::BuildMoveFailureToastText(TEXT("BattleDeckFull")).ToString(),
		FString(TEXT("无法移动：备战区已满。")));
	TestEqual(TEXT("Move unknown helper reason falls back"),
		FWacomBackpackScreenTestAccess::BuildMoveFailureToastText(TEXT("RunSessionMissing")).ToString(),
		FString(TEXT("无法移动：当前规则不允许。")));
	TestEqual(TEXT("Delete missing card reason"),
		FWacomBackpackScreenTestAccess::BuildDeleteFailureToastText(TEXT("MissingCard")).ToString(),
		FString(TEXT("无法销毁：没有卡牌数据。")));
	TestEqual(TEXT("Delete intrinsic reason"),
		FWacomBackpackScreenTestAccess::BuildDeleteFailureToastText(TEXT("Intrinsic")).ToString(),
		FString(TEXT("无法销毁：固有卡不能被销毁。")));
	TestEqual(TEXT("Delete not owned reason"),
		FWacomBackpackScreenTestAccess::BuildDeleteFailureToastText(TEXT("CardNotOwned")).ToString(),
		FString(TEXT("无法销毁：这张卡不在当前背包中。")));
	TestEqual(TEXT("Delete last bag reason"),
		FWacomBackpackScreenTestAccess::BuildDeleteFailureToastText(TEXT("LastBagProvider")).ToString(),
		FString(TEXT("无法销毁：这是最后一张背包容量卡。")));
	TestEqual(TEXT("Delete last capacity provider reason"),
		FWacomBackpackScreenTestAccess::BuildDeleteFailureToastText(TEXT("LastCapacityProvider")).ToString(),
		FString(TEXT("无法销毁：这是最后一张背包容量卡。")));

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

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackDisabledDeckCardBlocksToggleButKeepsHoverSpec,
	"Wacom.UI.Backpack.DisabledDeckCardBlocksToggleButKeepsHover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackDisabledDeckCardBlocksToggleButKeepsHoverSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	FCardInstance Inst;
	Inst.InstanceId = FGuid::NewGuid();
	Inst.Definition = Card.Get();

	FRunStorageCardView View;
	View.Instance = Inst;
	View.PhysicalZone = EZoneKind::SpecialZone;
	View.ZoneOwnerInstanceId = FGuid::NewGuid();
	View.bCanToggleBattleEnabledInSpecialZone = true;
	Widget->SetStorageCardView(View);
	Widget->SetMoveEnabled(false);

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

	const FVector2D Right = FWacomBackpackCardDetailController::ComputePanelPosition(
		FVector2D(100.f, 80.f),
		AnchorSize,
		LayerSize,
		PanelSize,
		12.f);
	TestEqual(TEXT("Detail panel prefers right side"), Right.X, 372.0);
	TestEqual(TEXT("Detail panel keeps top alignment"), Right.Y, 80.0);

	const FVector2D Left = FWacomBackpackCardDetailController::ComputePanelPosition(
		FVector2D(700.f, 80.f),
		AnchorSize,
		LayerSize,
		PanelSize,
		12.f);
	TestEqual(TEXT("Detail panel flips to left when right side overflows"), Left.X, 328.0);

	const FVector2D Clamped = FWacomBackpackCardDetailController::ComputePanelPosition(
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
	TestTrue(TEXT("BackpackScreen can show detail panel for hovered card"), FWacomBackpackScreenTestAccess::ShowDetailForCardWidget(*Screen, Widget.Get()));
	TestTrue(TEXT("Detail panel visible after hover"), FWacomBackpackScreenTestAccess::IsDetailVisible(*Screen));
	TestEqual(TEXT("Detail panel receives card name"), FWacomBackpackScreenTestAccess::DetailNameText(*Screen).ToString(), TEXT("悬停详情卡"));

	FWacomBackpackScreenTestAccess::HideDetail(*Screen);
	TestFalse(TEXT("Detail panel hidden on request"), FWacomBackpackScreenTestAccess::IsDetailVisible(*Screen));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackDeckCardMoveClickUnboundSpec,
	"Wacom.UI.Backpack.DeckCardMoveClickUnbound",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackDeckCardMoveClickUnboundSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, R6.6: Main card button stays passive until the Workspace binds pointer forwarding.
	TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
	Widget->TakeWidget();

	TestFalse(TEXT("MoveButton has no click bindings"), Widget->HasMoveButtonClickBindings());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackStorageCardViewPayloadSpec,
	"Wacom.UI.Backpack.StorageCardViewPayload",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackStorageCardViewPayloadSpec::RunTest(const FString& /*Parameters*/)
{
	// Workspace 卡牌保持 Snapshot 身份和物理归属，只转发指针事件，不再构造第二套拖拽 payload。
	TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	FRunStorageCardView View;
	View.Instance.InstanceId = FGuid::NewGuid();
	View.Instance.Definition = Card.Get();
	View.PhysicalZone = EZoneKind::SpecialZone;
	View.ZoneOwnerInstanceId = FGuid::NewGuid();

	Widget->SetCard(View.Instance, View.PhysicalZone, View.ZoneOwnerInstanceId);

	TestEqual(TEXT("StorageCardView instance id copied"), Widget->GetCardInstanceId(), View.Instance.InstanceId);
	TestTrue(TEXT("StorageCardView physical zone copied"), Widget->GetFromZone() == View.PhysicalZone);
	TestEqual(TEXT("StorageCardView owner id copied"), Widget->GetFromZoneOwnerInstanceId(), View.ZoneOwnerInstanceId);
	TestEqual(TEXT("StorageCardView definition copied"), Widget->GetCard(), Card.Get());

	Widget->SetProjectedFromBadgeText(FText::FromString(TEXT("来自 蛛茧绒囊")));
	TestTrue(TEXT("Projected badge still available for snapshot projection cards"), Widget->IsProjectedFromBadgeVisible());

	return true;
}
