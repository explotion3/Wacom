// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/WidgetComponent.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/DataValidation.h"
#include "UI/Battle/WacomBattleEnemyIntentPresentationStyle.h"
#include "UI/Battle/WacomBattleEnemyPanelWidget.h"
#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"
#include "UI/Foundation/WacomUIDeveloperSettings.h"

namespace WacomBattleEnemySinglePartPanelSpec
{
	constexpr TCHAR PanelClassPath[] =
		TEXT("/Game/Wacom/UI/Enemy/WBP_WacomBattleEnemySinglePartPanelWidget.WBP_WacomBattleEnemySinglePartPanelWidget_C");
	constexpr TCHAR EntryClassPath[] =
		TEXT("/Game/Wacom/UI/Enemy/WBP_WacomBattleEnemySinglePartEntryWidget.WBP_WacomBattleEnemySinglePartEntryWidget_C");
	constexpr TCHAR LegacyPanelClassPath[] =
		TEXT("/Game/Wacom/UI/Enemy/BP_WacomBattleEnemyPanelWidget.BP_WacomBattleEnemyPanelWidget_C");
	constexpr TCHAR IntentStylePath[] =
		TEXT("/Game/Wacom/UI/Enemy/Intent/DA_EnemyIntentPresentation_Default.DA_EnemyIntentPresentation_Default");

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

	UWidgetAnimation* FindAnimation(UUserWidget* Widget, const FName Name)
	{
		const UWidgetBlueprintGeneratedClass* GeneratedClass = Widget
			? Cast<UWidgetBlueprintGeneratedClass>(Widget->GetClass())
			: nullptr;
		if (!GeneratedClass)
		{
			return nullptr;
		}
		for (UWidgetAnimation* Animation : GeneratedClass->Animations)
		{
			if (Animation
				&& (Animation->GetFName() == Name
					|| Animation->GetDisplayLabel() == Name.ToString()
					|| Animation->GetName().StartsWith(Name.ToString())))
			{
				return Animation;
			}
		}
		return nullptr;
	}

	FWacomBattleEnemyPartEntryViewData MakePartView(
		const int32 CurrentHp = 18,
		const int32 MaxHp = 24,
		const int32 Shield = 4,
		const int32 Initiative = 1,
		const FName IntentId = TEXT("TrainingWarrior.Body.Attack"))
	{
		FWacomBattleEnemyPartEntryViewData View;
		View.EnemySlotId = TEXT("Enemy");
		View.PartSlotId = TEXT("Body");
		View.Identity = FBattlePartSlotIdentity::Make(
			TEXT("Encounter"), View.EnemySlotId, View.PartSlotId);
		View.PartDisplayName = FText::FromString(TEXT("身体"));
		View.CurrentHp = CurrentHp;
		View.MaxHp = MaxHp;
		View.Shield = Shield;
		View.CurrentInitiative = Initiative;
		View.CurrentIntentId = IntentId;
		View.CurrentIntentDisplayName = FText::FromString(TEXT("攻击"));
		View.CurrentIntentInitiative = Initiative;
		View.CurrentIntentResistanceValue = 4;
		return View;
	}

	FWacomBattleEnemyPanelViewData MakePanelView()
	{
		FWacomBattleEnemyPanelViewData View;
		View.EncounterId = TEXT("Encounter");
		View.EnemySlotId = TEXT("Enemy");
		View.UnitKey = FBattleEnemyUnitKey::Make(View.EncounterId, View.EnemySlotId);
		View.EnemyDisplayName = FText::FromString(TEXT("训练战士"));
		View.EnemyInitiativeSum = 1;
		View.Parts.Add(MakePartView());
		return View;
	}

	UEnemyDefinition* MakeDefinition(const int32 PartCount)
	{
		UEnemyDefinition* Definition = NewObject<UEnemyDefinition>(GetTransientPackage());
		for (int32 Index = 0; Index < PartCount; ++Index)
		{
			UEnemyPartDefinition* PartDefinition = NewObject<UEnemyPartDefinition>(Definition);
			PartDefinition->PartId = FName(*FString::Printf(TEXT("Test.Part%d"), Index));
			FEnemyPartSlot& Slot = Definition->Parts.AddDefaulted_GetRef();
			Slot.PartSlotId = FName(*FString::Printf(TEXT("Part%d"), Index));
			Slot.PartDef = PartDefinition;
		}
		return Definition;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemySinglePartPanelAssetContractSpec,
	"Wacom.UI.Battle.EnemyPanel.SinglePartCompact.AssetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemySinglePartPanelAssetContractSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemySinglePartPanelSpec;
	UClass* PanelClass = LoadClass<UWacomBattleEnemyPanelWidget>(nullptr, PanelClassPath);
	UClass* EntryClass = LoadClass<UWacomBattleEnemyPartEntryWidget>(nullptr, EntryClassPath);
	UWacomBattleEnemyIntentPresentationStyle* Style =
		LoadObject<UWacomBattleEnemyIntentPresentationStyle>(nullptr, IntentStylePath);
	if (!TestNotNull(TEXT("Compact panel WBP"), PanelClass)
		|| !TestNotNull(TEXT("Compact entry WBP"), EntryClass)
		|| !TestNotNull(TEXT("Default Intent presentation style"), Style))
	{
		return false;
	}

	TestTrue(TEXT("Panel uses passive native parent"),
		PanelClass->IsChildOf(UWacomBattleEnemyPanelWidget::StaticClass()));
	TestTrue(TEXT("Entry uses passive native parent"),
		EntryClass->IsChildOf(UWacomBattleEnemyPartEntryWidget::StaticClass()));
	TestEqual(TEXT("Three explicit TrainingWarrior intents"), Style->IntentIcons.Num(), 3);

	const FSlateBrush* Fallback = Style->ResolveIntentIcon(NAME_None);
	const FSlateBrush* Attack = Style->ResolveIntentIcon(TEXT("TrainingWarrior.Body.Attack"));
	const FSlateBrush* Guard = Style->ResolveIntentIcon(TEXT("TrainingWarrior.Body.Guard"));
	const FSlateBrush* Cleave = Style->ResolveIntentIcon(TEXT("TrainingWarrior.Body.Cleave"));
	if (!TestNotNull(TEXT("Fallback star brush"), Fallback)
		|| !TestNotNull(TEXT("Attack brush"), Attack)
		|| !TestNotNull(TEXT("Guard brush"), Guard)
		|| !TestNotNull(TEXT("Cleave brush"), Cleave))
	{
		return false;
	}
	TestNotNull(TEXT("Fallback star resource"), Fallback->GetResourceObject());
	TestNotNull(TEXT("Attack resource"), Attack->GetResourceObject());
	TestNotNull(TEXT("Guard resource"), Guard->GetResourceObject());
	TestNotNull(TEXT("Cleave resource"), Cleave->GetResourceObject());
	TestTrue(TEXT("Attack and Guard resources differ"),
		Attack->GetResourceObject() != Guard->GetResourceObject());
	TestTrue(TEXT("Guard and Cleave resources differ"),
		Guard->GetResourceObject() != Cleave->GetResourceObject());
	const FSlateBrush* Unknown = Style->ResolveIntentIcon(TEXT("Unknown.Intent"));
	if (!TestNotNull(TEXT("Unknown intent fallback brush"), Unknown))
	{
		return false;
	}
	TestEqual(TEXT("Unknown intent uses fallback resource"),
		Unknown->GetResourceObject(), Fallback->GetResourceObject());
	FDataValidationContext ValidStyleContext;
	TestEqual(TEXT("Formal Intent style passes Data Validation"),
		Style->IsDataValid(ValidStyleContext), EDataValidationResult::Valid);

	UWacomBattleEnemyIntentPresentationStyle* EmptyIdStyle =
		DuplicateObject<UWacomBattleEnemyIntentPresentationStyle>(Style, GetTransientPackage());
	EmptyIdStyle->IntentIcons[0].IntentId = NAME_None;
	FDataValidationContext EmptyIdContext;
	TestEqual(TEXT("Empty IntentId is rejected"),
		EmptyIdStyle->IsDataValid(EmptyIdContext), EDataValidationResult::Invalid);

	UWacomBattleEnemyIntentPresentationStyle* DuplicateIdStyle =
		DuplicateObject<UWacomBattleEnemyIntentPresentationStyle>(Style, GetTransientPackage());
	DuplicateIdStyle->IntentIcons[1].IntentId = DuplicateIdStyle->IntentIcons[0].IntentId;
	FDataValidationContext DuplicateIdContext;
	TestEqual(TEXT("Duplicate IntentId is rejected"),
		DuplicateIdStyle->IsDataValid(DuplicateIdContext), EDataValidationResult::Invalid);

	UWacomBattleEnemyIntentPresentationStyle* InvalidBrushStyle =
		DuplicateObject<UWacomBattleEnemyIntentPresentationStyle>(Style, GetTransientPackage());
	InvalidBrushStyle->IntentIcons[0].IconBrush = FSlateBrush();
	FDataValidationContext InvalidBrushContext;
	TestEqual(TEXT("Invalid Intent brush is rejected"),
		InvalidBrushStyle->IsDataValid(InvalidBrushContext), EDataValidationResult::Invalid);

	UWorld* World = FindAutomationWorld();
	UWacomBattleEnemyPanelWidget* Panel = World
		? CreateWidget<UWacomBattleEnemyPanelWidget>(World, PanelClass)
		: nullptr;
	UWacomBattleEnemyPartEntryWidget* Entry = World
		? CreateWidget<UWacomBattleEnemyPartEntryWidget>(World, EntryClass)
		: nullptr;
	if (!TestNotNull(TEXT("Compact panel instance"), Panel)
		|| !TestNotNull(TEXT("Compact entry instance"), Entry))
	{
		return false;
	}
	Panel->TakeWidget();
	Entry->TakeWidget();

	TestEqual(TEXT("Compact panel entry class"), Panel->GetPartEntryWidgetClass().Get(), EntryClass);
	TestEqual(TEXT("Compact entry style"), Entry->GetIntentPresentationStyle(), Style);
	TestEqual(TEXT("Panel root exposes only child hit testing"),
		Panel->WidgetTree->RootWidget->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	TestEqual(TEXT("Entry root exposes only the inspection hotspot"),
		Entry->WidgetTree->RootWidget->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);

	const TArray<FName> PanelBindings = {
		TEXT("EnemyNameText"), TEXT("EnemyInitiativeText"),
		TEXT("PartList"), TEXT("PanelContextHighlight") };
	for (const FName Binding : PanelBindings)
	{
		TestNotNull(*FString::Printf(TEXT("Panel binding %s"), *Binding.ToString()),
			Panel->WidgetTree->FindWidget(Binding));
	}
	const TArray<FName> EntryBindings = {
		TEXT("InitiativeDiamond"), TEXT("IntentDiamond"), TEXT("IntentIcon"),
		TEXT("HpBar"), TEXT("HpText"), TEXT("ShieldContainer"),
		TEXT("ShieldFrame"), TEXT("ShieldBadge"),
		TEXT("ShieldText"), TEXT("StatusList"), TEXT("StatusOverflowText"),
		TEXT("DetailsContainer"),
		TEXT("ContextHighlight"), TEXT("ActionPreviewOverlay"),
		TEXT("DestroyedOverlay"), TEXT("DestroyedMark"), TEXT("InspectHitTarget") };
	for (const FName Binding : EntryBindings)
	{
		TestNotNull(*FString::Printf(TEXT("Entry binding %s"), *Binding.ToString()),
			Entry->WidgetTree->FindWidget(Binding));
	}
	const TArray<FName> Animations = {
		TEXT("IntroAnimation"), TEXT("DamagePulseAnimation"),
		TEXT("ShieldPulseAnimation"), TEXT("DestroyedPulseAnimation"),
		TEXT("ContextHighlightAnimation"), TEXT("InitiativePulseAnimation"),
		TEXT("IntentChangedAnimation") };
	for (const FName Animation : Animations)
	{
		TestNotNull(*FString::Printf(TEXT("Animation %s"), *Animation.ToString()),
			FindAnimation(Entry, Animation));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemySinglePartPanelValuesAndPreviewSpec,
	"Wacom.UI.Battle.EnemyPanel.SinglePartCompact.ValuesAndPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemySinglePartPanelValuesAndPreviewSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemySinglePartPanelSpec;
	UWorld* World = FindAutomationWorld();
	UClass* EntryClass = LoadClass<UWacomBattleEnemyPartEntryWidget>(nullptr, EntryClassPath);
	UWacomBattleEnemyPartEntryWidget* Entry = World && EntryClass
		? CreateWidget<UWacomBattleEnemyPartEntryWidget>(World, EntryClass)
		: nullptr;
	if (!TestNotNull(TEXT("Compact entry"), Entry))
	{
		return false;
	}
	Entry->TakeWidget();

	UTextBlock* HpText = FindWidget<UTextBlock>(Entry, TEXT("HpText"));
	UProgressBar* HpBar = FindWidget<UProgressBar>(Entry, TEXT("HpBar"));
	UWidget* ShieldContainer = FindWidget<UWidget>(Entry, TEXT("ShieldContainer"));
	UWidget* ShieldFrame = FindWidget<UWidget>(Entry, TEXT("ShieldFrame"));
	UWidget* ShieldBadge = FindWidget<UWidget>(Entry, TEXT("ShieldBadge"));
	UTextBlock* ShieldText = FindWidget<UTextBlock>(Entry, TEXT("ShieldText"));
	UTextBlock* InitiativeText = FindWidget<UTextBlock>(Entry, TEXT("InitiativeText"));
	UImage* IntentIcon = FindWidget<UImage>(Entry, TEXT("IntentIcon"));
	UWidget* DestroyedMark = FindWidget<UWidget>(Entry, TEXT("DestroyedMark"));
	if (!TestNotNull(TEXT("HP text"), HpText)
		|| !TestNotNull(TEXT("HP bar"), HpBar)
		|| !TestNotNull(TEXT("Shield container"), ShieldContainer)
		|| !TestNotNull(TEXT("Shield frame"), ShieldFrame)
		|| !TestNotNull(TEXT("Shield badge"), ShieldBadge)
		|| !TestNotNull(TEXT("Shield text"), ShieldText)
		|| !TestNotNull(TEXT("Initiative text"), InitiativeText)
		|| !TestNotNull(TEXT("Intent icon"), IntentIcon)
		|| !TestNotNull(TEXT("Destroyed mark"), DestroyedMark))
	{
		return false;
	}

	FWacomBattleEnemyPartEntryViewData RealView = MakePartView();
	Entry->SetPartEntryViewData(RealView);
	TestEqual(TEXT("HP text is current value only"), HpText->GetText().ToString(), FString(TEXT("18")));
	TestTrue(TEXT("HP percent is 18/24"), FMath::IsNearlyEqual(HpBar->GetPercent(), 0.75f));
	TestEqual(TEXT("Shield text is exact value"), ShieldText->GetText().ToString(), FString(TEXT("4")));
	TestEqual(TEXT("Initiative value"), InitiativeText->GetText().ToString(), FString(TEXT("1")));
	TestEqual(TEXT("Shield visible above zero"), ShieldContainer->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("Shield frame visible above zero"), ShieldFrame->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("Shield badge visible above zero"), ShieldBadge->GetVisibility(), ESlateVisibility::HitTestInvisible);
	UObject* AttackResource = IntentIcon->GetBrush().GetResourceObject();
	TestNotNull(TEXT("Attack icon applied"), AttackResource);

	Entry->StopAllAnimations();
	FWacomBattleEnemyPartEntryViewData Preview = RealView;
	Preview.CurrentHp = 12;
	Preview.Shield = 30;
	Preview.CurrentInitiative = 0;
	Preview.bActionPreviewWillAct = true;
	Entry->SetActionPreview(Preview);
	TestEqual(TEXT("Preview HP is projected current value"), HpText->GetText().ToString(), FString(TEXT("12")));
	TestEqual(TEXT("Preview shield exact number is not truncated"), ShieldText->GetText().ToString(), FString(TEXT("30")));
	TestEqual(TEXT("Acting preview initiative is zero"), InitiativeText->GetText().ToString(), FString(TEXT("0")));
	TestEqual(TEXT("Preview preserves current intent icon"),
		IntentIcon->GetBrush().GetResourceObject(), AttackResource);
	TestFalse(TEXT("Preview does not play initiative pulse"),
		Entry->IsAnimationPlaying(FindAnimation(Entry, TEXT("InitiativePulseAnimation"))));
	TestFalse(TEXT("Preview does not play intent pulse"),
		Entry->IsAnimationPlaying(FindAnimation(Entry, TEXT("IntentChangedAnimation"))));

	Entry->ClearActionPreview();
	TestEqual(TEXT("Clear preview restores HP"), HpText->GetText().ToString(), FString(TEXT("18")));
	FWacomBattleEnemyPartEntryViewData NoShield = RealView;
	NoShield.Shield = 0;
	Entry->SetPartEntryViewData(NoShield);
	TestEqual(TEXT("Zero shield collapses row"), ShieldContainer->GetVisibility(), ESlateVisibility::Collapsed);

	Entry->StopAllAnimations();
	FWacomBattleEnemyPartEntryViewData ChangedIntent = NoShield;
	ChangedIntent.CurrentInitiative = 2;
	ChangedIntent.CurrentIntentId = TEXT("TrainingWarrior.Body.Guard");
	Entry->SetPartEntryViewData(ChangedIntent);
	TestTrue(TEXT("Real initiative change pulses"),
		Entry->IsAnimationPlaying(FindAnimation(Entry, TEXT("InitiativePulseAnimation"))));
	TestTrue(TEXT("Real intent change pulses"),
		Entry->IsAnimationPlaying(FindAnimation(Entry, TEXT("IntentChangedAnimation"))));
	TestTrue(TEXT("Guard uses a different icon"),
		IntentIcon->GetBrush().GetResourceObject() != AttackResource);

	Entry->StopAllAnimations();
	FWacomBattleEnemyPartEntryViewData Destroyed = ChangedIntent;
	Destroyed.bDestroyed = true;
	Destroyed.CurrentInitiative = 0;
	Destroyed.CurrentIntentId = TEXT("TrainingWarrior.Body.Cleave");
	Entry->SetPartEntryViewData(Destroyed);
	TestEqual(TEXT("Destroyed X is visible"), DestroyedMark->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestTrue(TEXT("Destroyed animation plays"),
		Entry->IsAnimationPlaying(FindAnimation(Entry, TEXT("DestroyedPulseAnimation"))));
	TestFalse(TEXT("Destroyed suppresses initiative pulse"),
		Entry->IsAnimationPlaying(FindAnimation(Entry, TEXT("InitiativePulseAnimation"))));
	TestFalse(TEXT("Destroyed suppresses intent pulse"),
		Entry->IsAnimationPlaying(FindAnimation(Entry, TEXT("IntentChangedAnimation"))));
	Entry->CancelPendingPresentation();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemySinglePartPanelSelectionAndContextSpec,
	"Wacom.UI.Battle.EnemyPanel.SinglePartCompact.SelectionAndContext",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemySinglePartPanelSelectionAndContextSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemySinglePartPanelSpec;
	UClass* CompactPanelClass = LoadClass<UWacomBattleEnemyPanelWidget>(nullptr, PanelClassPath);
	UClass* LegacyPanelClass = LoadClass<UWacomBattleEnemyPanelWidget>(nullptr, LegacyPanelClassPath);
	UClass* EntryClass = LoadClass<UWacomBattleEnemyPartEntryWidget>(nullptr, EntryClassPath);
	const UWacomUIDeveloperSettings* Settings = GetDefault<UWacomUIDeveloperSettings>();
	if (!TestNotNull(TEXT("Compact panel class"), CompactPanelClass)
		|| !TestNotNull(TEXT("Existing multi-part panel class"), LegacyPanelClass)
		|| !TestNotNull(TEXT("Compact entry class"), EntryClass)
		|| !TestNotNull(TEXT("UI settings"), Settings))
	{
		return false;
	}
	TestEqual(TEXT("Project compact default class"),
		Settings->DefaultBattleEnemySinglePartPanelWidgetClass.LoadSynchronous(), CompactPanelClass);

	UWorld* World = FindAutomationWorld();
	AWacomBattleEnemyActor* Host = World ? World->SpawnActor<AWacomBattleEnemyActor>() : nullptr;
	if (!TestNotNull(TEXT("Enemy Host"), Host))
	{
		return false;
	}
	UWidgetComponent* Component = Cast<UWidgetComponent>(
		Host->GetDefaultSubobjectByName(TEXT("EnemyPanelWidget")));
	if (!TestNotNull(TEXT("Enemy panel component"), Component))
	{
		Host->Destroy();
		return false;
	}

	Host->EnemyDefinition = MakeDefinition(1);
	Host->RerunConstructionScripts();
	TestEqual(TEXT("One valid part selects compact panel"),
		Component->GetWidgetClass().Get(), CompactPanelClass);

	Host->EnemyDefinition = MakeDefinition(2);
	Host->RerunConstructionScripts();
	TestEqual(TEXT("Multiple valid parts keep existing panel"),
		Component->GetWidgetClass().Get(), LegacyPanelClass);

	Host->EnemyPanelWidgetClass = LegacyPanelClass;
	Host->EnemyDefinition = MakeDefinition(1);
	Host->RerunConstructionScripts();
	TestEqual(TEXT("Explicit Host override wins"),
		Component->GetWidgetClass().Get(), LegacyPanelClass);
	Host->Destroy();

	UWacomBattleEnemyPanelWidget* Panel = CreateWidget<UWacomBattleEnemyPanelWidget>(World, CompactPanelClass);
	if (!TestNotNull(TEXT("Compact panel"), Panel))
	{
		return false;
	}
	Panel->TakeWidget();
	Panel->SetEnemyPanelViewData(MakePanelView());
	UTextBlock* EnemyName = FindWidget<UTextBlock>(Panel, TEXT("EnemyNameText"));
	UPanelWidget* PartList = FindWidget<UPanelWidget>(Panel, TEXT("PartList"));
	if (!TestNotNull(TEXT("Enemy name"), EnemyName)
		|| !TestNotNull(TEXT("Part list"), PartList)
		|| !TestEqual(TEXT("Exactly one entry"), PartList->GetChildrenCount(), 1))
	{
		Panel->ClearEnemyPanelViewData();
		return false;
	}
	UWacomBattleEnemyPartEntryWidget* Entry =
		Cast<UWacomBattleEnemyPartEntryWidget>(PartList->GetChildAt(0));
	UWidget* Details = FindWidget<UWidget>(Entry, TEXT("DetailsContainer"));
	TestEqual(TEXT("Name hidden in compact normal state"), EnemyName->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Details hidden in compact normal state"), Details->GetVisibility(), ESlateVisibility::Collapsed);

	Panel->SetHoveredPartSlotId(TEXT("Body"));
	TestEqual(TEXT("Hover keeps compact enemy name hidden"), EnemyName->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Hover keeps inline details hidden"), Details->GetVisibility(), ESlateVisibility::Collapsed);
	Panel->SetHoveredPartSlotId(NAME_None);

	FWacomBattleEnemyPartEntryViewData Preview = MakePartView(12, 24, 0, 0);
	Preview.bActionPreviewWillAct = true;
	TestTrue(TEXT("Preview accepted"), Panel->SetActionPreviewPartViews({ Preview }));
	TestEqual(TEXT("Preview keeps compact enemy name hidden"), EnemyName->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Preview keeps inline details hidden"), Details->GetVisibility(), ESlateVisibility::Collapsed);
	Panel->ClearActionPreview();
	TestEqual(TEXT("Clearing context restores compact name"), EnemyName->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Clearing context restores compact details"), Details->GetVisibility(), ESlateVisibility::Collapsed);
	Panel->ClearEnemyPanelViewData();
	TestEqual(TEXT("Clear removes cached entry from panel"), PartList->GetChildrenCount(), 0);
	return true;
}
