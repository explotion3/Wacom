// Copyright Wacom. All Rights Reserved.

#include "../../../WacomApp/Private/UI/Battle/WacomBattleEnemyIntentPresentation.h"

#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Battle/WacomBattleEnemyIntentPresentationStyle.h"
#include "UI/Battle/WacomBattleEnemyPanelViewData.h"
#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"
#include "UI/Battle/WacomBattleIntentTooltipWidget.h"
#include "UI/WacomBattleEnemyPartEntryWidgetTestAccess.h"

namespace WacomBattleEnemyIntentPresentationSpec
{
	constexpr TCHAR EntryClassPath[] =
		TEXT("/Game/Wacom/UI/Enemy/BP_WacomBattleEnemyPartEntryWidget.BP_WacomBattleEnemyPartEntryWidget_C");

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

	FBattleIntentEffectSnapshot MakeEffect(
		const FGameplayTag Type,
		const int32 Magnitude,
		const EBattleIntentEffectTargetKind TargetKind,
		const int32 TargetCount = 0,
		const int32 Duration = 0)
	{
		FBattleIntentEffectSnapshot Effect;
		Effect.EffectType = Type;
		Effect.Magnitude = Magnitude;
		Effect.Duration = Duration;
		Effect.TargetKind = TargetKind;
		Effect.TargetCount = TargetCount;
		return Effect;
	}

	FWacomBattleEnemyPartEntryViewData MakePart()
	{
		FWacomBattleEnemyPartEntryViewData Part;
		Part.Identity = FBattlePartSlotIdentity::Make(
			TEXT("Encounter"), TEXT("Enemy"), TEXT("Part"));
		Part.CurrentIntentId = TEXT("Test.Intent");
		Part.CurrentIntentDisplayName = FText::FromString(TEXT("测试意图"));
		Part.CurrentIntentInitiative = 4;
		Part.bCurrentIntentIsAttack = true;
		Part.CurrentIntentPeakAttackDamage = 3;
		return Part;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyIntentPresentationOrderSpec,
	"Wacom.UI.Battle.EnemyIntentPresentation.HeaderAndAdjacentAggregation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyIntentPresentationOrderSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyIntentPresentationSpec;
	FWacomBattleEnemyPartEntryViewData Part = MakePart();
	Part.CurrentIntentEffects = {
		MakeEffect(
			WacomTags::Effect_Damage,
			3,
			EBattleIntentEffectTargetKind::Player),
		MakeEffect(
			WacomTags::Effect_Damage,
			3,
			EBattleIntentEffectTargetKind::Player),
		MakeEffect(
			WacomTags::Effect_ApplyStatus_Slow,
			1,
			EBattleIntentEffectTargetKind::RandomPlayerHandCards,
			2),
		MakeEffect(
			WacomTags::Effect_Damage,
			3,
			EBattleIntentEffectTargetKind::Player),
	};

	const FWacomBattleIntentPresentationViewData View =
		FWacomBattleIntentPresentationBuilder::Build(Part, nullptr);
	TestTrue(TEXT("Header exposes initiative"),
		View.HeaderMetaText.ToString().Contains(TEXT("INIT 4")));
	TestTrue(TEXT("Attack header exposes authoritative peak segment"),
		View.HeaderMetaText.ToString().Contains(TEXT("最高单段 3")));
	TestEqual(TEXT("Only adjacent identical effects aggregate"),
		View.EffectRows.Num(), 3);
	TestEqual(TEXT("First adjacent pair aggregates exactly twice"),
		View.EffectRows[0].RepeatCount, 2);
	TestTrue(TEXT("Aggregated damage displays multiplier"),
		View.EffectRows[0].EffectText.ToString().Contains(TEXT("× 2")));
	TestEqual(TEXT("Non-adjacent damage remains separate"),
		View.EffectRows[2].RepeatCount, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyIntentPresentationStatusSpec,
	"Wacom.UI.Battle.EnemyIntentPresentation.StatusCatalogAndTargets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyIntentPresentationStatusSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyIntentPresentationSpec;
	FWacomBattleEnemyPartEntryViewData Part = MakePart();
	Part.bCurrentIntentIsAttack = false;
	Part.CurrentIntentPeakAttackDamage = 0;
	Part.CurrentIntentEffects = {
		MakeEffect(
			WacomTags::Effect_ApplyStatus_Slow,
			1,
			EBattleIntentEffectTargetKind::RandomPlayerHandCards,
			2,
			2),
		MakeEffect(
			WacomTags::Status_Shield,
			4,
			EBattleIntentEffectTargetKind::SelfEnemyPart),
	};

	const FWacomBattleIntentPresentationViewData View =
		FWacomBattleIntentPresentationBuilder::Build(Part, nullptr);
	TestEqual(TEXT("Status and shield remain in authored order"),
		View.EffectRows.Num(), 2);
	TestEqual(TEXT("Random hand target is explicit"),
		View.EffectRows[0].TargetText.ToString(),
		FString(TEXT("[随机 2 张手牌]")));
	TestTrue(TEXT("Status name comes from catalog"),
		View.EffectRows[0].EffectText.ToString().Contains(TEXT("减速")));
	TestTrue(TEXT("Status duration remains explicit"),
		View.EffectRows[0].EffectText.ToString().Contains(TEXT("持续 2")));
	TestFalse(TEXT("Status core rule is populated"),
		View.EffectRows[0].CoreRuleText.IsEmpty());
	TestEqual(TEXT("Self target is explicit"),
		View.EffectRows[1].TargetText.ToString(),
		FString(TEXT("[自身]")));
	TestTrue(TEXT("Shield value is exact"),
		View.EffectRows[1].EffectText.ToString().Contains(TEXT("+4")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyIntentPresentationFallbackSpec,
	"Wacom.UI.Battle.EnemyIntentPresentation.UnknownAndTooltipLimit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyIntentPresentationFallbackSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyIntentPresentationSpec;
	FWacomBattleEnemyPartEntryViewData Part = MakePart();
	for (int32 Index = 0; Index < 7; ++Index)
	{
		Part.CurrentIntentEffects.Add(MakeEffect(
			WacomTags::Effect_Heal,
			Index + 1,
			EBattleIntentEffectTargetKind::Unknown));
	}

	const FWacomBattleIntentPresentationViewData View =
		FWacomBattleIntentPresentationBuilder::Build(Part, nullptr, 5);
	TestEqual(TEXT("Tooltip exposes at most five rows"), View.EffectRows.Num(), 5);
	TestEqual(TEXT("Tooltip reports hidden row count"), View.HiddenEffectRowCount, 2);
	TestEqual(TEXT("Unknown target remains explicit"),
		View.EffectRows[0].TargetText.ToString(),
		FString(TEXT("[未知目标]")));
	TestTrue(TEXT("Unknown effect keeps full GameplayTag"),
		View.EffectRows[0].EffectText.ToString().Contains(TEXT("Effect.Heal")));

	Part.bDestroyed = true;
	const FWacomBattleIntentPresentationViewData Destroyed =
		FWacomBattleIntentPresentationBuilder::Build(Part, nullptr);
	TestFalse(TEXT("Destroyed part has no public presentation"),
		Destroyed.HasIntent());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyIntentTooltipLifecycleSpec,
	"Wacom.UI.Battle.EnemyIntentPresentation.HeadEntryTooltipLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyIntentTooltipLifecycleSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyIntentPresentationSpec;
	UClass* EntryClass =
		LoadClass<UWacomBattleEnemyPartEntryWidget>(nullptr, EntryClassPath);
	UWorld* World = FindAutomationWorld();
	UWacomBattleEnemyPartEntryWidget* Entry = World && EntryClass
		? CreateWidget<UWacomBattleEnemyPartEntryWidget>(World, EntryClass)
		: nullptr;
	if (!TestNotNull(TEXT("Formal enemy part entry"), Entry))
	{
		return false;
	}
	Entry->TakeWidget();
	FWacomBattleEnemyPartEntryWidgetTestAccess::Construct(*Entry);

	FWacomBattleEnemyPartEntryViewData View = MakePart();
	View.CurrentIntentEffects = {
		MakeEffect(
			WacomTags::Effect_Damage,
			3,
			EBattleIntentEffectTargetKind::Player),
	};
	FWacomBattleEnemyPartEntryWidgetTestAccess::SetView(*Entry, View);
	FWacomBattleEnemyPartEntryWidgetTestAccess::SetInspectionInteractionEnabled(
		*Entry, true);
	UButton* Target =
		FWacomBattleEnemyPartEntryWidgetTestAccess::GetIntentTooltipTarget(
			*Entry);
	UImage* IntentIcon = Cast<UImage>(
		Entry->WidgetTree->FindWidget(TEXT("IntentIcon")));
	TestNotNull(TEXT("Intent transparent hit target"), Target);
	TestNotNull(TEXT("Intent icon"), IntentIcon);
	TestTrue(TEXT("Intent target owns only the icon-sized content"),
		Target && IntentIcon
		&& Target->GetContent() == IntentIcon
		&& IntentIcon->GetParent() == Target);
	TestTrue(TEXT("Intent tooltip delegate is bound"),
		Target && Target->ToolTipWidgetDelegate.IsBound());
	TestEqual(TEXT("Intent target is mouse visible"),
		Target ? Target->GetVisibility() : ESlateVisibility::Collapsed,
		ESlateVisibility::Visible);
	bool bAncestorsAllowIntentHitTesting = Target != nullptr;
	for (const UWidget* Ancestor = Target ? Target->GetParent() : nullptr;
		Ancestor;
		Ancestor = Ancestor->GetParent())
	{
		const ESlateVisibility Visibility = Ancestor->GetVisibility();
		bAncestorsAllowIntentHitTesting &=
			Visibility == ESlateVisibility::Visible
			|| Visibility == ESlateVisibility::SelfHitTestInvisible;
	}
	TestTrue(TEXT("Intent target runtime ancestors allow mouse hit testing"),
		bAncestorsAllowIntentHitTesting);
	TestTrue(TEXT("Tooltip uses formal passive widget"),
		Cast<UWacomBattleIntentTooltipWidget>(
			FWacomBattleEnemyPartEntryWidgetTestAccess::BuildIntentTooltip(
				*Entry)) != nullptr);

	FWacomBattleEnemyPartEntryViewData Preview = View;
	Preview.CurrentHp -= 3;
	FWacomBattleEnemyPartEntryWidgetTestAccess::SetPreview(*Entry, Preview);
	TestEqual(TEXT("Action preview disables tooltip hit target"),
		Target->GetVisibility(),
		ESlateVisibility::HitTestInvisible);
	TestNull(TEXT("Action preview cannot build intent tooltip"),
		FWacomBattleEnemyPartEntryWidgetTestAccess::BuildIntentTooltip(*Entry));
	FWacomBattleEnemyPartEntryWidgetTestAccess::ClearPreview(*Entry);
	TestEqual(TEXT("Clearing preview restores tooltip hit target"),
		Target->GetVisibility(),
		ESlateVisibility::Visible);
	return true;
}
