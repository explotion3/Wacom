// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/WidgetComponent.h"
#include "Misc/DataValidation.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "UI/Battle/WacomBattleEnemyIntentPresentationStyle.h"
#include "UI/Battle/WacomBattleEnemyPanelWidget.h"
#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"
#include "UI/Foundation/WacomUIDeveloperSettings.h"
#include "UI/WacomBattleEnemyPartEntryWidgetTestAccess.h"

namespace WacomBattleEnemySinglePartPanelSpec
{
	constexpr TCHAR PanelClassPath[] =
		TEXT("/Game/Wacom/UI/Enemy/WBP_WacomBattleEnemySinglePartPanelWidget.WBP_WacomBattleEnemySinglePartPanelWidget_C");
	constexpr TCHAR EntryClassPath[] =
		TEXT("/Game/Wacom/UI/Enemy/WBP_WacomBattleEnemySinglePartEntryWidget.WBP_WacomBattleEnemySinglePartEntryWidget_C");
	constexpr TCHAR MultiPanelClassPath[] =
		TEXT("/Game/Wacom/UI/Enemy/BP_WacomBattleEnemyPanelWidget.BP_WacomBattleEnemyPanelWidget_C");
	constexpr TCHAR MultiEntryClassPath[] =
		TEXT("/Game/Wacom/UI/Enemy/BP_WacomBattleEnemyPartEntryWidget.BP_WacomBattleEnemyPartEntryWidget_C");
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
		for (const UClass* Class = Widget ? Widget->GetClass() : nullptr;
			Class;
			Class = Class->GetSuperClass())
		{
			const UWidgetBlueprintGeneratedClass* GeneratedClass =
				Cast<UWidgetBlueprintGeneratedClass>(Class);
			if (!GeneratedClass)
			{
				continue;
			}
			for (UWidgetAnimation* Animation : GeneratedClass->Animations)
			{
				if (Animation && (Animation->GetFName() == Name
					|| Animation->GetDisplayLabel() == Name.ToString()
					|| Animation->GetName().StartsWith(Name.ToString())))
				{
					return Animation;
				}
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
		View.Identity = FBattlePartSlotIdentity::Make(TEXT("Encounter"), View.EnemySlotId, View.PartSlotId);
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
	UClass* MultiPanelClass = LoadClass<UWacomBattleEnemyPanelWidget>(nullptr, MultiPanelClassPath);
	UClass* MultiEntryClass = LoadClass<UWacomBattleEnemyPartEntryWidget>(nullptr, MultiEntryClassPath);
	UWacomBattleEnemyIntentPresentationStyle* Style =
		LoadObject<UWacomBattleEnemyIntentPresentationStyle>(nullptr, IntentStylePath);
	if (!TestNotNull(TEXT("Single panel WBP"), PanelClass)
		|| !TestNotNull(TEXT("Single entry WBP"), EntryClass)
		|| !TestNotNull(TEXT("Full panel WBP"), MultiPanelClass)
		|| !TestNotNull(TEXT("Full entry WBP"), MultiEntryClass)
		|| !TestNotNull(TEXT("Default Intent style"), Style))
	{
		return false;
	}
	TestEqual(TEXT("Single panel is a lightweight subclass"), PanelClass->GetSuperClass(), MultiPanelClass);
	TestEqual(TEXT("Single entry is a lightweight subclass"), EntryClass->GetSuperClass(), MultiEntryClass);
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
	TestTrue(TEXT("Attack and Guard icons differ"),
		Attack->GetResourceObject() != Guard->GetResourceObject());
	TestTrue(TEXT("Guard and Cleave icons differ"),
		Guard->GetResourceObject() != Cleave->GetResourceObject());
	TestEqual(TEXT("Unknown intent uses fallback"),
		Style->ResolveIntentIcon(TEXT("Unknown.Intent"))->GetResourceObject(),
		Fallback->GetResourceObject());
	FDataValidationContext ValidationContext;
	TestEqual(TEXT("Formal Intent style validates"),
		Style->IsDataValid(ValidationContext), EDataValidationResult::Valid);

	UWorld* World = FindAutomationWorld();
	UWacomBattleEnemyPanelWidget* Panel = World
		? CreateWidget<UWacomBattleEnemyPanelWidget>(World, PanelClass) : nullptr;
	UWacomBattleEnemyPartEntryWidget* Entry = World
		? CreateWidget<UWacomBattleEnemyPartEntryWidget>(World, EntryClass) : nullptr;
	if (!TestNotNull(TEXT("Single panel instance"), Panel)
		|| !TestNotNull(TEXT("Single entry instance"), Entry))
	{
		return false;
	}
	Panel->TakeWidget();
	Entry->TakeWidget();
	TestTrue(TEXT("Single width is 268"), FMath::IsNearlyEqual(Panel->GetFixedPanelWidth(), 268.0f));
	TestEqual(TEXT("Single panel uses lightweight entry class"), Panel->GetPartEntryWidgetClass().Get(), EntryClass);
	TestEqual(TEXT("Single entry inherits Intent style"), Entry->GetIntentPresentationStyle(), Style);
	TestNotNull(TEXT("Single entry inherits VitalsTrackImage"), FindWidget<UImage>(Entry, TEXT("VitalsTrackImage")));
	TestNull(TEXT("Single entry does not restore legacy HpBar"), FindWidget<UWidget>(Entry, TEXT("HpBar")));
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
		? CreateWidget<UWacomBattleEnemyPartEntryWidget>(World, EntryClass) : nullptr;
	if (!TestNotNull(TEXT("Single entry"), Entry))
	{
		return false;
	}
	Entry->TakeWidget();
	UTextBlock* HpText = FindWidget<UTextBlock>(Entry, TEXT("HpText"));
	UWidget* ShieldRoot = FindWidget<UWidget>(Entry, TEXT("ShieldValueRoot"));
	UTextBlock* ShieldText = FindWidget<UTextBlock>(Entry, TEXT("ShieldText"));
	UTextBlock* InitiativeText = FindWidget<UTextBlock>(Entry, TEXT("InitiativeText"));
	UImage* IntentIcon = FindWidget<UImage>(Entry, TEXT("IntentIcon"));
	UWidget* DestroyedMark = FindWidget<UWidget>(Entry, TEXT("DestroyedMark"));
	if (!TestNotNull(TEXT("HP text"), HpText)
		|| !TestNotNull(TEXT("Shield root"), ShieldRoot)
		|| !TestNotNull(TEXT("Shield text"), ShieldText)
		|| !TestNotNull(TEXT("Initiative text"), InitiativeText)
		|| !TestNotNull(TEXT("Intent icon"), IntentIcon)
		|| !TestNotNull(TEXT("Destroyed mark"), DestroyedMark))
	{
		return false;
	}

	FWacomBattleEnemyPartEntryViewData RealView = MakePartView();
	Entry->SetPartEntryViewData(RealView);
	if (!TestNotNull(TEXT("Vitals MID is created with first ViewData"),
		FWacomBattleEnemyPartEntryWidgetTestAccess::GetVitalsMaterial(*Entry)))
	{
		return false;
	}
	TestEqual(TEXT("HP text is current value only"), HpText->GetText().ToString(), FString(TEXT("18")));
	TestTrue(TEXT("Material HP is 18/24"), FMath::IsNearlyEqual(
		FWacomBattleEnemyPartEntryWidgetTestAccess::GetMaterialScalar(*Entry, TEXT("HpCurrentPercent")), 0.75f));
	TestEqual(TEXT("Shield text is exact"), ShieldText->GetText().ToString(), FString(TEXT("4")));
	TestEqual(TEXT("Initiative value"), InitiativeText->GetText().ToString(), FString(TEXT("1")));
	TestEqual(TEXT("Shield visible above zero"), ShieldRoot->GetVisibility(), ESlateVisibility::HitTestInvisible);
	UObject* AttackResource = IntentIcon->GetBrush().GetResourceObject();

	Entry->StopAllAnimations();
	FWacomBattleEnemyPartEntryViewData Preview = RealView;
	Preview.CurrentHp = 12;
	Preview.Shield = 30;
	Preview.CurrentInitiative = 0;
	Preview.bActionPreviewWillAct = true;
	Entry->SetActionPreview(Preview);
	TestEqual(TEXT("Preview HP is projected"), HpText->GetText().ToString(), FString(TEXT("12")));
	TestEqual(TEXT("Preview Shield number is not truncated"), ShieldText->GetText().ToString(), FString(TEXT("30")));
	TestEqual(TEXT("Acting preview Initiative is zero"), InitiativeText->GetText().ToString(), FString(TEXT("0")));
	TestEqual(TEXT("Preview preserves current Intent icon"),
		IntentIcon->GetBrush().GetResourceObject(), AttackResource);
	TestTrue(TEXT("Preview mode is material-only"), FMath::IsNearlyEqual(
		FWacomBattleEnemyPartEntryWidgetTestAccess::GetMaterialScalar(*Entry, TEXT("HpPreviewMode")), 1.0f));
	TestFalse(TEXT("Preview does not play Initiative animation"),
		Entry->IsAnimationPlaying(FindAnimation(Entry, TEXT("InitiativeStepAnimation"))));
	TestFalse(TEXT("Preview does not play Intent animation"),
		Entry->IsAnimationPlaying(FindAnimation(Entry, TEXT("IntentChangeAnimation"))));

	Entry->ClearActionPreview();
	TestEqual(TEXT("Clear preview restores HP"), HpText->GetText().ToString(), FString(TEXT("18")));
	TestTrue(TEXT("Clear preview disables preview material mode"), FMath::IsNearlyZero(
		FWacomBattleEnemyPartEntryWidgetTestAccess::GetMaterialScalar(*Entry, TEXT("HpPreviewMode"))));
	FWacomBattleEnemyPartEntryViewData Changed = RealView;
	Changed.Shield = 0;
	Changed.CurrentInitiative = 2;
	Changed.CurrentIntentId = TEXT("TrainingWarrior.Body.Guard");
	Entry->SetPartEntryViewData(Changed);
	TestEqual(TEXT("Zero Shield collapses badge root"), ShieldRoot->GetVisibility(), ESlateVisibility::Collapsed);
	TestTrue(TEXT("Real Initiative change animates"),
		Entry->IsAnimationPlaying(FindAnimation(Entry, TEXT("InitiativeStepAnimation"))));
	TestTrue(TEXT("Real Intent change animates"),
		Entry->IsAnimationPlaying(FindAnimation(Entry, TEXT("IntentChangeAnimation"))));
	TestTrue(TEXT("Guard changes icon"), IntentIcon->GetBrush().GetResourceObject() != AttackResource);

	Entry->StopAllAnimations();
	Changed.CurrentHp = 0;
	Changed.bDestroyed = true;
	Entry->SetPartEntryViewData(Changed);
	TestEqual(TEXT("Destroyed mark is visible"), DestroyedMark->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestTrue(TEXT("Destroyed transition animates"),
		Entry->IsAnimationPlaying(FindAnimation(Entry, TEXT("DestroyedAnimation"))));
	TestFalse(TEXT("Destroyed suppresses Initiative animation"),
		Entry->IsAnimationPlaying(FindAnimation(Entry, TEXT("InitiativeStepAnimation"))));
	Entry->CancelPendingPresentation();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemySinglePartPanelSelectionSpec,
	"Wacom.UI.Battle.EnemyPanel.SinglePartCompact.SelectionAndContext",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemySinglePartPanelSelectionSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemySinglePartPanelSpec;
	UClass* SinglePanelClass = LoadClass<UWacomBattleEnemyPanelWidget>(nullptr, PanelClassPath);
	UClass* MultiPanelClass = LoadClass<UWacomBattleEnemyPanelWidget>(nullptr, MultiPanelClassPath);
	const UWacomUIDeveloperSettings* Settings = GetDefault<UWacomUIDeveloperSettings>();
	if (!TestNotNull(TEXT("Single panel class"), SinglePanelClass)
		|| !TestNotNull(TEXT("Multi panel class"), MultiPanelClass)
		|| !TestNotNull(TEXT("UI settings"), Settings))
	{
		return false;
	}
	TestEqual(TEXT("Project single default"),
		Settings->DefaultBattleEnemySinglePartPanelWidgetClass.LoadSynchronous(), SinglePanelClass);

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
	TestEqual(TEXT("One valid part selects single panel"), Component->GetWidgetClass().Get(), SinglePanelClass);
	Host->EnemyDefinition = MakeDefinition(2);
	Host->RerunConstructionScripts();
	TestEqual(TEXT("Multiple valid parts select multi panel"), Component->GetWidgetClass().Get(), MultiPanelClass);
	Host->EnemyPanelWidgetClass = MultiPanelClass;
	Host->EnemyDefinition = MakeDefinition(1);
	Host->RerunConstructionScripts();
	TestEqual(TEXT("Explicit Host override wins"), Component->GetWidgetClass().Get(), MultiPanelClass);
	Host->Destroy();

	UWacomBattleEnemyPanelWidget* Panel = CreateWidget<UWacomBattleEnemyPanelWidget>(World, SinglePanelClass);
	if (!TestNotNull(TEXT("Single panel"), Panel))
	{
		return false;
	}
	Panel->TakeWidget();
	Panel->SetEnemyPanelViewData(MakePanelView());
	UHorizontalBox* PartList = FindWidget<UHorizontalBox>(Panel, TEXT("PartList"));
	if (!TestNotNull(TEXT("Inherited part list"), PartList)
		|| !TestEqual(TEXT("Exactly one entry"), PartList->GetChildrenCount(), 1))
	{
		return false;
	}
	UWacomBattleEnemyPartEntryWidget* Entry =
		Cast<UWacomBattleEnemyPartEntryWidget>(PartList->GetChildAt(0));
	UWidget* ContextSurface = FindWidget<UWidget>(Entry, TEXT("ContextSurface"));
	Panel->SetHoveredPartSlotId(TEXT("Body"));
	TestEqual(TEXT("Hover shows only semantic context surface"),
		ContextSurface->GetVisibility(), ESlateVisibility::HitTestInvisible);
	Panel->SetHoveredPartSlotId(NAME_None);
	TestEqual(TEXT("Clearing hover hides context surface"),
		ContextSurface->GetVisibility(), ESlateVisibility::Collapsed);
	Panel->ClearEnemyPanelViewData();
	return true;
}
