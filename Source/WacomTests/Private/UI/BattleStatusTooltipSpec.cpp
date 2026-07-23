// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "../../../WacomApp/Private/UI/Battle/WacomBattleStatusTooltipPresentation.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Statuses/BattleStatusRuleConstants.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"
#include "UI/Battle/WacomBattleStatusIconWidget.h"
#include "UI/Battle/WacomBattleStatusTooltipWidget.h"
#include "UI/WacomBattleEnemyPartEntryWidgetTestAccess.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SWidget.h"

namespace WacomBattleStatusTooltipSpec
{
	constexpr TCHAR EnemyPartEntryClassPath[] =
		TEXT("/Game/Wacom/UI/Enemy/BP_WacomBattleEnemyPartEntryWidget.BP_WacomBattleEnemyPartEntryWidget_C");
	constexpr TCHAR StatusIconClassPath[] =
		TEXT("/Game/Wacom/UI/Battle/PlayerStatusBar/WBP_BattleStatusIcon.WBP_BattleStatusIcon_C");

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

	FWacomBattleStatusIconView BuildView(
		const FGameplayTag Tag,
		const EWacomBattleStatusInspectionHost Host,
		const int32 StackCount = 1)
	{
		FWacomBattleStatusIconView View;
		View.StatusTag = Tag;
		View.DisplayName = FText::FromString(Tag.GetTagName().ToString());
		View.StackCount = StackCount;
		View.InspectionHost = Host;
		FWacomBattleStatusTooltipPresentationBuilder::PopulateRuleText(View);
		return View;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleStatusTooltipHostSpecificRulesSpec,
	"Wacom.UI.Battle.StatusTooltip.HostSpecificRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleStatusTooltipHostSpecificRulesSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleStatusTooltipSpec;

	FWacomBattleStatusIconView PlayerSlow = BuildView(
		WacomTags::Status_Slow,
		EWacomBattleStatusInspectionHost::Player);
	FWacomBattleStatusIconView EnemySlow = BuildView(
		WacomTags::Status_Slow,
		EWacomBattleStatusInspectionHost::EnemyPart);
	FWacomBattleStatusIconView PlayerFreeze = BuildView(
		WacomTags::Status_Freeze,
		EWacomBattleStatusInspectionHost::Player);
	FWacomBattleStatusIconView EnemyFreeze = BuildView(
		WacomTags::Status_Freeze,
		EWacomBattleStatusInspectionHost::EnemyPart);
	FWacomBattleStatusIconView PlayerTwilight = BuildView(
		WacomTags::Status_Twilight,
		EWacomBattleStatusInspectionHost::Player);
	FWacomBattleStatusIconView EnemyTwilight = BuildView(
		WacomTags::Status_Twilight,
		EWacomBattleStatusInspectionHost::EnemyPart);
	FWacomBattleStatusIconView EnemyStunned = BuildView(
		WacomTags::Status_Stunned,
		EWacomBattleStatusInspectionHost::EnemyPart);

	TestTrue(TEXT("Player Slow explains next-turn cards"),
		PlayerSlow.CoreEffectText.ToString().Contains(TEXT("下回合")));
	TestTrue(TEXT("Enemy Slow explains current intent"),
		EnemySlow.CoreEffectText.ToString().Contains(TEXT("当前意图")));
	TestTrue(TEXT("Player Freeze blocks card play"),
		PlayerFreeze.CoreEffectText.ToString().Contains(TEXT("无法打出")));
	TestTrue(TEXT("Enemy Freeze does not skip action"),
		EnemyFreeze.StackPolicyText.ToString().Contains(TEXT("不会使敌人跳过行动")));
	TestTrue(TEXT("Player Twilight follows cards across zones"),
		PlayerTwilight.StackPolicyText.ToString().Contains(TEXT("跨区域保留")));
	TestTrue(TEXT("Enemy Twilight halves after the next intent"),
		EnemyTwilight.StackPolicyText.ToString().Contains(TEXT("向下减半")));
	TestTrue(TEXT("Enemy Stunned consumes one stack per skipped action"),
		EnemyStunned.StackPolicyText.ToString().Contains(TEXT("消耗 1 层")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleStatusTooltipPoisonSharedConstantsSpec,
	"Wacom.UI.Battle.StatusTooltip.PoisonUsesSharedRuleConstants",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleStatusTooltipPoisonSharedConstantsSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleStatusTooltipSpec;

	const FWacomBattleStatusIconView PlayerPoison = BuildView(
		WacomTags::Status_Poison,
		EWacomBattleStatusInspectionHost::Player,
		3);
	TestTrue(TEXT("Poison damage uses the shared per-stack value"),
		PlayerPoison.CoreEffectText.ToString().Contains(
			FString::FromInt(WacomBattleStatusRuleConstants::PoisonDamagePerStack)));
	TestTrue(TEXT("Poison heal policy uses the shared ratio"),
		PlayerPoison.StackPolicyText.ToString().Contains(
			FString::FromInt(FMath::RoundToInt(
				WacomBattleStatusRuleConstants::PlayerHealPoisonRemovalRatio * 100.0f))));
	TestTrue(TEXT("Poison explains shield bypass"),
		PlayerPoison.CoreEffectText.ToString().Contains(TEXT("穿透护盾")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleStatusTooltipUnknownFallbackSpec,
	"Wacom.UI.Battle.StatusTooltip.UnknownFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleStatusTooltipUnknownFallbackSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleStatusTooltipSpec;

	const FWacomBattleStatusIconView Unknown = BuildView(
		WacomTags::Card_Keyword_Weapon,
		EWacomBattleStatusInspectionHost::Unknown);
	TestFalse(TEXT("Unknown core fallback is not empty"), Unknown.CoreEffectText.IsEmpty());
	TestFalse(TEXT("Unknown timing fallback is not empty"), Unknown.TriggerTimingText.IsEmpty());
	TestFalse(TEXT("Unknown stack fallback is not empty"), Unknown.StackPolicyText.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleStatusTooltipInspectionLifecycleSpec,
	"Wacom.UI.Battle.StatusTooltip.InspectionLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleStatusTooltipInspectionLifecycleSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleStatusTooltipSpec;

	TStrongObjectPtr<UWacomBattleStatusIconWidget> Icon(
		NewObject<UWacomBattleStatusIconWidget>());
	const TSharedRef<SWidget> IconTakenWidget = Icon->TakeWidget();
	Icon->SetStatusIconView(BuildView(
		WacomTags::Status_Poison,
		EWacomBattleStatusInspectionHost::Player,
		2));
	const TSharedPtr<SWidget> IconSlateWidget = Icon->GetCachedWidget();
	if (!TestTrue(TEXT("Status icon owns a live Slate widget"), IconSlateWidget.IsValid()))
	{
		return false;
	}
	TestTrue(TEXT("Status icon exposes its tooltip through the real Slate hover path"),
		IconSlateWidget->GetToolTip().IsValid());

	UWacomBattleStatusTooltipWidget* FirstTooltip =
		Cast<UWacomBattleStatusTooltipWidget>(
			Icon->ToolTipWidgetDelegate.Execute());
	if (!TestNotNull(TEXT("Tooltip is created lazily"), FirstTooltip))
	{
		return false;
	}
	TestEqual(TEXT("Tooltip receives current stack"),
		FirstTooltip->GetStatusView().StackCount,
		2);

	Icon->SetStatusInspectionEnabled(false);
	TestEqual(TEXT("Disabled icon remains visual but not hit-testable"),
		Icon->GetVisibility(),
		ESlateVisibility::HitTestInvisible);
	TestNull(TEXT("Disabled inspection does not provide a tooltip"),
		Icon->ToolTipWidgetDelegate.Execute());

	Icon->SetStatusInspectionEnabled(true);
	Icon->SetStatusIconView(BuildView(
		WacomTags::Status_Poison,
		EWacomBattleStatusInspectionHost::Player,
		5));
	UWacomBattleStatusTooltipWidget* UpdatedTooltip =
		Cast<UWacomBattleStatusTooltipWidget>(
			Icon->ToolTipWidgetDelegate.Execute());
	TestTrue(TEXT("Repeated inspection reuses the cached tooltip"),
		UpdatedTooltip == FirstTooltip);
	if (UpdatedTooltip)
	{
		TestEqual(TEXT("Cached tooltip receives the latest stack"),
			UpdatedTooltip->GetStatusView().StackCount,
			5);
	}

	UWorld* World = FindAutomationWorld();
	UClass* StatusIconClass = LoadClass<UWacomBattleStatusIconWidget>(
		nullptr,
		StatusIconClassPath);
	if (!TestNotNull(TEXT("Automation world"), World)
		|| !TestNotNull(TEXT("Formal status icon class"), StatusIconClass))
	{
		return false;
	}
	TStrongObjectPtr<UWacomBattleStatusIconWidget> FormalIcon(
		CreateWidget<UWacomBattleStatusIconWidget>(World, StatusIconClass));
	if (!TestNotNull(TEXT("Formal status icon instance"), FormalIcon.Get()))
	{
		return false;
	}
	const TSharedRef<SWidget> FormalIconTakenWidget = FormalIcon->TakeWidget();
	FormalIcon->SetStatusIconView(BuildView(
		WacomTags::Status_Slow,
		EWacomBattleStatusInspectionHost::EnemyPart,
		1));
	const TSharedPtr<SWidget> FormalIconSlateWidget =
		FormalIcon->GetCachedWidget();
	TestTrue(TEXT("Formal status icon keeps the tooltip delegate on Slate"),
		FormalIconSlateWidget.IsValid()
			&& FormalIconSlateWidget->GetToolTip().IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleStatusTooltipOverflowSpec,
	"Wacom.UI.Battle.StatusTooltip.OverflowShowsOnlyHiddenStatuses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleStatusTooltipOverflowSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleStatusIconListWidget> List(
		NewObject<UWacomBattleStatusIconListWidget>());
	const TSharedRef<SWidget> ListTakenWidget = List->TakeWidget();
	List->SetInspectionHost(EWacomBattleStatusInspectionHost::EnemyPart);
	List->SetMaxVisibleStatuses(2);

	FGameplayTagContainer Statuses;
	Statuses.AddTag(WacomTags::Status_Poison);
	Statuses.AddTag(WacomTags::Status_Slow);
	Statuses.AddTag(WacomTags::Status_Freeze);
	Statuses.AddTag(WacomTags::Status_Twilight);
	TMap<FGameplayTag, int32> Stacks;
	Stacks.Add(WacomTags::Status_Poison, 1);
	Stacks.Add(WacomTags::Status_Slow, 2);
	Stacks.Add(WacomTags::Status_Freeze, 3);
	Stacks.Add(WacomTags::Status_Twilight, 4);
	List->SetStatuses(Statuses, Stacks);

	const TArray<FWacomBattleStatusIconView> Hidden =
		List->GetHiddenStatusIconViews();
	TestEqual(TEXT("Only statuses after the visible capacity are hidden"), Hidden.Num(), 2);
	if (Hidden.Num() != 2)
	{
		return false;
	}
	TestTrue(TEXT("Hidden order starts with Freeze"),
		Hidden[0].StatusTag == WacomTags::Status_Freeze);
	TestTrue(TEXT("Hidden order ends with Twilight"),
		Hidden[1].StatusTag == WacomTags::Status_Twilight);

	UTextBlock* OverflowText = List->WidgetTree
		? Cast<UTextBlock>(List->WidgetTree->FindWidget(TEXT("OverflowText")))
		: nullptr;
	if (!TestNotNull(TEXT("List owns OverflowText"), OverflowText))
	{
		return false;
	}
	TestEqual(TEXT("Overflow displays the hidden count"),
		OverflowText->GetText().ToString(),
		FString(TEXT("+2")));
	const TSharedPtr<SWidget> OverflowSlateWidget = OverflowText->GetCachedWidget();
	if (!TestTrue(TEXT("Overflow owns a live Slate widget"), OverflowSlateWidget.IsValid()))
	{
		return false;
	}
	TestTrue(TEXT("Overflow exposes its tooltip through the real Slate hover path"),
		OverflowSlateWidget->GetToolTip().IsValid());

	UWacomBattleStatusTooltipWidget* Tooltip =
		Cast<UWacomBattleStatusTooltipWidget>(
			OverflowText->ToolTipWidgetDelegate.Execute());
	if (!TestNotNull(TEXT("Overflow creates the shared tooltip"), Tooltip))
	{
		return false;
	}
	TestTrue(TEXT("Tooltip is in overflow mode"), Tooltip->IsShowingOverflow());
	TestEqual(TEXT("Tooltip receives only hidden statuses"),
		Tooltip->GetOverflowViews().Num(),
		2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleStatusTooltipEnemyActivationSpec,
	"Wacom.UI.Battle.StatusTooltip.EnemyStatusActivationRoutesInspection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleStatusTooltipEnemyActivationSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleStatusTooltipSpec;

	UWorld* World = FindAutomationWorld();
	UClass* EntryClass = LoadClass<UWacomBattleEnemyPartEntryWidget>(
		nullptr,
		EnemyPartEntryClassPath);
	if (!TestNotNull(TEXT("Automation world"), World)
		|| !TestNotNull(TEXT("Formal enemy entry class"), EntryClass))
	{
		return false;
	}

	TStrongObjectPtr<UWacomBattleEnemyPartEntryWidget> Entry(
		CreateWidget<UWacomBattleEnemyPartEntryWidget>(World, EntryClass));
	if (!TestNotNull(TEXT("Enemy entry instance"), Entry.Get()))
	{
		return false;
	}
	Entry->TakeWidget();
	FWacomBattleEnemyPartEntryWidgetTestAccess::Construct(*Entry);

	FWacomBattleEnemyPartEntryViewData View;
	View.Identity.EncounterId = TEXT("Encounter.StatusTooltip");
	View.Identity.EnemySlotId = TEXT("Enemy.0");
	View.Identity.PartSlotId = TEXT("Head");
	View.EnemySlotId = View.Identity.EnemySlotId;
	View.PartSlotId = View.Identity.PartSlotId;
	View.RuntimeStatuses.AddTag(WacomTags::Status_Poison);
	View.RuntimeStatusStacks.Add(WacomTags::Status_Poison, 2);
	FWacomBattleEnemyPartEntryWidgetTestAccess::SetView(*Entry, View);
	FWacomBattleEnemyPartEntryWidgetTestAccess::SetInspectionInteractionEnabled(
		*Entry,
		true);

	FBattlePartSlotIdentity RequestedIdentity;
	int32 RequestCount = 0;
	Entry->OnInspectionRequestedNative.AddLambda(
		[&RequestedIdentity, &RequestCount](const FBattlePartSlotIdentity& Identity)
		{
			RequestedIdentity = Identity;
			++RequestCount;
		});

	UWacomBattleStatusIconListWidget* StatusList =
		FWacomBattleEnemyPartEntryWidgetTestAccess::GetStatusList(*Entry);
	if (!TestNotNull(TEXT("Enemy entry StatusList"), StatusList))
	{
		return false;
	}
	TestTrue(TEXT("Status activation follows inspection availability"),
		StatusList->IsStatusIconActivationEnabled());
	StatusList->OnStatusIconActivatedNative.Broadcast(
		StatusList->GetStatusIconViews()[0]);

	TestEqual(TEXT("Status activation requests inspection exactly once"), RequestCount, 1);
	TestTrue(TEXT("Status activation preserves stable part identity"),
		RequestedIdentity == View.Identity);
	return true;
}
