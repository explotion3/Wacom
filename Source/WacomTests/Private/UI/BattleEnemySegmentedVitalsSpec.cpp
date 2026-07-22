// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Battle/WacomBattleEnemyPanelWidget.h"
#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"
#include "UI/Battle/WacomBattleStatusIconWidget.h"
#include "UI/WacomBattleEnemyPartEntryWidgetTestAccess.h"

namespace WacomBattleEnemySegmentedVitalsSpec
{
	constexpr TCHAR PanelClassPath[] =
		TEXT("/Game/Wacom/UI/Enemy/BP_WacomBattleEnemyPanelWidget.BP_WacomBattleEnemyPanelWidget_C");

	UWorld* FindAutomationWorld()
	{
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (UWorld* World = Context.World())
				{
					return World;
				}
			}
		}
		return GWorld;
	}

	template <typename TWidget>
	TWidget* FindWidget(UUserWidget* Owner, const FName Name)
	{
		return Owner && Owner->WidgetTree
			? Cast<TWidget>(Owner->WidgetTree->FindWidget(Name))
			: nullptr;
	}

	FWacomBattleEnemyPartEntryViewData MakePart(
		const FName PartSlotId,
		const int32 CurrentHp,
		const int32 MaxHp,
		const int32 Shield,
		const int32 Initiative,
		const bool bDestroyed = false)
	{
		FWacomBattleEnemyPartEntryViewData Part;
		Part.EnemySlotId = TEXT("Enemy");
		Part.PartSlotId = PartSlotId;
		Part.Identity = FBattlePartSlotIdentity::Make(
			TEXT("Encounter"), Part.EnemySlotId, PartSlotId);
		Part.PartDisplayName = FText::FromName(PartSlotId);
		Part.CurrentHp = CurrentHp;
		Part.MaxHp = MaxHp;
		Part.Shield = Shield;
		Part.CurrentInitiative = Initiative;
		Part.CurrentIntentId = FName(*FString::Printf(TEXT("Snake.%s.Intent"), *PartSlotId.ToString()));
		Part.CurrentIntentDisplayName = FText::FromString(TEXT("行动"));
		Part.CurrentIntentInitiative = Initiative;
		Part.CurrentIntentResistanceValue = 3;
		Part.bDestroyed = bDestroyed;
		return Part;
	}

	FWacomBattleEnemyPanelViewData MakeEnemy(
		TArray<FWacomBattleEnemyPartEntryViewData> Parts)
	{
		FWacomBattleEnemyPanelViewData Enemy;
		Enemy.EncounterId = TEXT("Encounter");
		Enemy.EnemySlotId = TEXT("Enemy");
		Enemy.UnitKey = FBattleEnemyUnitKey::Make(Enemy.EncounterId, Enemy.EnemySlotId);
		Enemy.EnemyDisplayName = FText::FromString(TEXT("林蛇"));
		Enemy.Parts = MoveTemp(Parts);
		for (const FWacomBattleEnemyPartEntryViewData& Part : Enemy.Parts)
		{
			Enemy.EnemyInitiativeSum += Part.CurrentInitiative;
		}
		return Enemy;
	}

	UWacomBattleEnemyPanelWidget* CreatePanel(const TCHAR* ClassPath)
	{
		UWorld* World = FindAutomationWorld();
		UClass* PanelClass = LoadClass<UWacomBattleEnemyPanelWidget>(nullptr, ClassPath);
		return World && PanelClass
			? CreateWidget<UWacomBattleEnemyPanelWidget>(World, PanelClass)
			: nullptr;
	}

	bool InvokeInspectionHandler(UWacomBattleEnemyPartEntryWidget* Entry)
	{
		UFunction* Function = Entry ? Entry->FindFunction(TEXT("HandleInspectClicked")) : nullptr;
		if (!Entry || !Function)
		{
			return false;
		}
		Entry->ProcessEvent(Function, nullptr);
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemySegmentedVitalsLayoutSpec,
	"Wacom.UI.Battle.EnemyPanel.SegmentedVitals.LayoutAndValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemySegmentedVitalsLayoutSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemySegmentedVitalsSpec;
	UWacomBattleEnemyPanelWidget* Panel = CreatePanel(PanelClassPath);
	if (!TestNotNull(TEXT("Multi-part segmented panel"), Panel))
	{
		return false;
	}
	Panel->TakeWidget();

	FWacomBattleEnemyPartEntryViewData Head = MakePart(TEXT("Head"), 7, 12, 0, 3);
	FWacomBattleEnemyPartEntryViewData Body = MakePart(TEXT("Body"), 18, 24, 19, 2);
	FWacomBattleEnemyPartEntryViewData Tail = MakePart(TEXT("Tail"), 0, 8, 0, 0, true);
	Head.RuntimeStatuses.AddTag(WacomTags::Status_Poison);
	Head.RuntimeStatuses.AddTag(WacomTags::Status_Slow);
	Head.RuntimeStatuses.AddTag(WacomTags::Status_Freeze);
	Head.RuntimeStatuses.AddTag(WacomTags::Status_Twilight);
	Head.RuntimeStatuses.AddTag(WacomTags::Status_Stunned);
	Panel->SetEnemyPanelViewData(MakeEnemy({ Head, Body, Tail }));

	UHorizontalBox* PartList = FindWidget<UHorizontalBox>(Panel, TEXT("PartList"));
	if (!TestNotNull(TEXT("PartList is a HorizontalBox"), PartList)
		|| !TestEqual(TEXT("Definition order produces three stable segments"),
			PartList->GetChildrenCount(), 3))
	{
		return false;
	}

	const FName ExpectedOrder[] = { TEXT("Head"), TEXT("Body"), TEXT("Tail") };
	const EWacomBattleEnemySegmentRole ExpectedRoles[] = {
		EWacomBattleEnemySegmentRole::First,
		EWacomBattleEnemySegmentRole::Middle,
		EWacomBattleEnemySegmentRole::Last };
	for (int32 Index = 0; Index < 3; ++Index)
	{
		UWacomBattleEnemyPartEntryWidget* Entry =
			Cast<UWacomBattleEnemyPartEntryWidget>(PartList->GetChildAt(Index));
		if (!TestNotNull(*FString::Printf(TEXT("Segment %d"), Index), Entry))
		{
			return false;
		}
		TestEqual(*FString::Printf(TEXT("Segment %d keeps Definition order"), Index),
			Entry->GetPartEntryViewData().PartSlotId, ExpectedOrder[Index]);
		const UHorizontalBoxSlot* Slot = Cast<UHorizontalBoxSlot>(Entry->Slot);
		TestTrue(*FString::Printf(TEXT("Segment %d uses equal Fill sizing"), Index),
			Slot && Slot->GetSize().SizeRule == ESlateSizeRule::Fill);
		TestEqual(*FString::Printf(TEXT("Segment %d receives edge role"), Index),
			FWacomBattleEnemyPartEntryWidgetTestAccess::GetSegmentRole(*Entry), ExpectedRoles[Index]);
		TestEqual(TEXT("All segments receive the same count"),
			FWacomBattleEnemyPartEntryWidgetTestAccess::GetSegmentCount(*Entry), 3);
	}

	UWacomBattleEnemyPartEntryWidget* HeadEntry =
		Cast<UWacomBattleEnemyPartEntryWidget>(PartList->GetChildAt(0));
	UWacomBattleEnemyPartEntryWidget* BodyEntry =
		Cast<UWacomBattleEnemyPartEntryWidget>(PartList->GetChildAt(1));
	UWacomBattleEnemyPartEntryWidget* TailEntry =
		Cast<UWacomBattleEnemyPartEntryWidget>(PartList->GetChildAt(2));
	UTextBlock* HeadHpText = FindWidget<UTextBlock>(HeadEntry, TEXT("HpText"));
	UWidget* HeadShield = FindWidget<UWidget>(HeadEntry, TEXT("ShieldValueRoot"));
	UWidget* BodyShield = FindWidget<UWidget>(BodyEntry, TEXT("ShieldValueRoot"));
	UTextBlock* BodyShieldText = FindWidget<UTextBlock>(BodyEntry, TEXT("ShieldText"));
	UWidget* TailDestroyed = FindWidget<UWidget>(TailEntry, TEXT("DestroyedSurface"));
	UWacomBattleStatusIconListWidget* HeadStatuses =
		FindWidget<UWacomBattleStatusIconListWidget>(HeadEntry, TEXT("StatusList"));
	UTextBlock* HeadStatusOverflow =
		FindWidget<UTextBlock>(HeadEntry, TEXT("StatusOverflowText"));
	if (!TestNotNull(TEXT("Head HP text"), HeadHpText)
		|| !TestNotNull(TEXT("Head Shield root"), HeadShield)
		|| !TestNotNull(TEXT("Body Shield root"), BodyShield)
		|| !TestNotNull(TEXT("Body Shield text"), BodyShieldText)
		|| !TestNotNull(TEXT("Tail Destroyed surface"), TailDestroyed)
		|| !TestNotNull(TEXT("Head status list"), HeadStatuses)
		|| !TestNotNull(TEXT("Head status overflow"), HeadStatusOverflow))
	{
		return false;
	}
	TestEqual(TEXT("Compact HP text shows current value only"),
		HeadHpText->GetText().ToString(), FString(TEXT("7")));
	TestTrue(TEXT("Each segment material uses its own HP ratio"),
		FMath::IsNearlyEqual(
			FWacomBattleEnemyPartEntryWidgetTestAccess::GetMaterialScalar(*HeadEntry, TEXT("HpCurrentPercent")),
			7.0f / 12.0f));
	TestEqual(TEXT("Zero Shield hides its overlay"), HeadShield->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Positive Shield shows its overlay"), BodyShield->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("Shield badge keeps exact value"), BodyShieldText->GetText().ToString(), FString(TEXT("19")));
	TestTrue(TEXT("Destroyed segment remains in place"), PartList->GetChildAt(2) == TailEntry);
	TestEqual(TEXT("Destroyed segment shows terminal surface"),
		TailDestroyed->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("Compact status list shows at most three icons"), HeadStatuses->GetMaxVisibleStatuses(), 3);
	TestEqual(TEXT("Compact status overflow is reported"), HeadStatuses->GetOverflowStatusCount(), 2);
	TestEqual(TEXT("Compact status overflow is visible as +N"),
		HeadStatusOverflow->GetText().ToString(), FString(TEXT("+2")));

	Body.CurrentHp = 9;
	Body.Shield = 0;
	TestTrue(TEXT("Projected part matches existing segment"), Panel->SetActionPreviewPartViews({ Body }));
	TestEqual(TEXT("Preview hides projected zero Shield without reflow"),
		BodyShield->GetVisibility(), ESlateVisibility::Collapsed);
	TestTrue(TEXT("Preview changes only the material preview HP"),
		FMath::IsNearlyEqual(
			FWacomBattleEnemyPartEntryWidgetTestAccess::GetMaterialScalar(*BodyEntry, TEXT("HpPreviewPercent")),
			9.0f / 24.0f));
	TestTrue(TEXT("Preview does not reorder segments"), PartList->GetChildAt(1) == BodyEntry);
	Panel->ClearActionPreview();
	TestEqual(TEXT("Clearing preview restores real Shield"),
		BodyShield->GetVisibility(), ESlateVisibility::HitTestInvisible);

	FWacomBattleEnemyPartEntryViewData Wing = MakePart(TEXT("Wing"), 5, 5, 0, 1);
	Panel->SetEnemyPanelViewData(MakeEnemy({ Head, Body, Tail, Wing }));
	TestEqual(TEXT("Four-part normal panel remains supported"), PartList->GetChildrenCount(), 4);
	Panel->ClearEnemyPanelViewData();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemySegmentedVitalsSinglePartAndInputSpec,
	"Wacom.UI.Battle.EnemyPanel.SegmentedVitals.SinglePartAndInputGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemySegmentedVitalsSinglePartAndInputSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemySegmentedVitalsSpec;
	UWacomBattleEnemyPanelWidget* Panel = CreatePanel(PanelClassPath);
	if (!TestNotNull(TEXT("Single-part segmented panel"), Panel))
	{
		return false;
	}
	Panel->TakeWidget();
	const FWacomBattleEnemyPartEntryViewData Body = MakePart(TEXT("Body"), 18, 24, 4, 1);
	Panel->SetEnemyPanelViewData(MakeEnemy({ Body }));
	USizeBox* PanelRoot = FindWidget<USizeBox>(Panel, TEXT("PanelRoot"));
	TestTrue(TEXT("One-part layout owns the 268 Slate-unit width"),
		PanelRoot && PanelRoot->IsWidthOverride()
		&& FMath::IsNearlyEqual(PanelRoot->GetWidthOverride(), 268.0f));
	UHorizontalBox* PartList = FindWidget<UHorizontalBox>(Panel, TEXT("PartList"));
	if (!TestNotNull(TEXT("Single PartList is inherited HorizontalBox"), PartList)
		|| !TestEqual(TEXT("Single enemy renders one segment"), PartList->GetChildrenCount(), 1))
	{
		return false;
	}
	UWacomBattleEnemyPartEntryWidget* Entry =
		Cast<UWacomBattleEnemyPartEntryWidget>(PartList->GetChildAt(0));
	USizeBox* EntryRoot = FindWidget<USizeBox>(Entry, TEXT("PartEntryRoot"));
	UButton* InspectButton = FindWidget<UButton>(Entry, TEXT("InspectHitTarget"));
	if (!TestNotNull(TEXT("Single segmented entry"), Entry)
		|| !TestNotNull(TEXT("Inherited V3 entry root"), EntryRoot)
		|| !TestNotNull(TEXT("Inspection hotspot"), InspectButton))
	{
		return false;
	}
	TestEqual(TEXT("One part uses Single edge role"),
		FWacomBattleEnemyPartEntryWidgetTestAccess::GetSegmentRole(*Entry),
		EWacomBattleEnemySegmentRole::Single);
	TestTrue(TEXT("Entry enforces the 116 Slate-unit minimum"),
		EntryRoot->IsMinDesiredWidthOverride()
		&& FMath::IsNearlyEqual(EntryRoot->GetMinDesiredWidth(), 116.0f));
	TestTrue(TEXT("Entry enforces the 92 Slate-unit height"),
		EntryRoot->IsHeightOverride()
		&& FMath::IsNearlyEqual(EntryRoot->GetHeightOverride(), 92.0f));

	int32 RequestCount = 0;
	FBattlePartSlotIdentity RequestedIdentity;
	Panel->OnInspectionRequestedNative.AddLambda(
		[&RequestCount, &RequestedIdentity](const FBattlePartSlotIdentity& Identity)
		{
			++RequestCount;
			RequestedIdentity = Identity;
		});
	Panel->SetInspectionInteractionEnabled(true);
	TestTrue(TEXT("Idle gate enables inspection"), Entry->IsInspectionInteractionEnabled());
	TestEqual(TEXT("Enabled entry exposes child hit testing"),
		Entry->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	TestTrue(TEXT("Enabled hotspot accepts input"), InspectButton->GetIsEnabled());
	TestEqual(TEXT("Enabled hotspot is the only visible hit target"),
		InspectButton->GetVisibility(), ESlateVisibility::Visible);
	TestTrue(TEXT("Inspection handler is callable"), InvokeInspectionHandler(Entry));
	TestEqual(TEXT("Click emits one request"), RequestCount, 1);
	TestTrue(TEXT("Request keeps stable Part identity"), RequestedIdentity == Body.Identity);

	FWacomBattleEnemyPartEntryViewData Preview = Body;
	Preview.CurrentHp = 10;
	Panel->SetActionPreviewPartViews({ Preview });
	TestFalse(TEXT("Preview disables inspection"), Entry->IsInspectionInteractionEnabled());
	TestEqual(TEXT("Preview restores click-through"), Entry->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestFalse(TEXT("Preview disables hotspot input"), InspectButton->GetIsEnabled());
	TestTrue(TEXT("Disabled handler remains safely callable"), InvokeInspectionHandler(Entry));
	TestEqual(TEXT("Disabled hotspot cannot emit a second request"), RequestCount, 1);
	Panel->ClearActionPreview();
	TestTrue(TEXT("Clearing preview restores the Idle gate"), Entry->IsInspectionInteractionEnabled());
	Panel->SetInspectionInteractionEnabled(false);
	TestEqual(TEXT("Non-Idle gate is click-through"), Entry->GetVisibility(), ESlateVisibility::HitTestInvisible);
	Panel->ClearEnemyPanelViewData();
	return true;
}
