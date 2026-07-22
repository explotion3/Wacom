// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/WidgetComponent.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "MovieScene.h"
#include "UI/Battle/WacomBattleEnemyPanelWidget.h"
#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"
#include "UI/Foundation/WacomUIDeveloperSettings.h"
#include "UI/WacomBattleEnemyPartEntryWidgetTestAccess.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomBattleEnemyPanelSpec
{
	constexpr TCHAR PanelClassPath[] =
		TEXT("/Game/Wacom/UI/Enemy/BP_WacomBattleEnemyPanelWidget.BP_WacomBattleEnemyPanelWidget_C");
	constexpr TCHAR PartEntryClassPath[] =
		TEXT("/Game/Wacom/UI/Enemy/BP_WacomBattleEnemyPartEntryWidget.BP_WacomBattleEnemyPartEntryWidget_C");
	constexpr TCHAR MaterialPath[] =
		TEXT("/Game/Wacom/UI/Enemy/Vitals/Materials/M_UI_EnemyVitalsTrack.M_UI_EnemyVitalsTrack");
	constexpr TCHAR FontPath[] =
		TEXT("/Game/Wacom/UI/Foundation/Fonts/Silkscreen/F_Silkscreen.F_Silkscreen");

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

	FWacomBattleEnemyPartEntryViewData MakePart(
		const FName EnemySlotId,
		const FName PartSlotId,
		const int32 CurrentHp,
		const int32 MaxHp,
		const int32 Shield,
		const int32 Initiative,
		const bool bDestroyed = false)
	{
		FWacomBattleEnemyPartEntryViewData View;
		View.EnemySlotId = EnemySlotId;
		View.PartSlotId = PartSlotId;
		View.Identity = FBattlePartSlotIdentity::Make(TEXT("Encounter"), EnemySlotId, PartSlotId);
		View.PartDisplayName = FText::FromName(PartSlotId);
		View.CurrentHp = CurrentHp;
		View.MaxHp = MaxHp;
		View.Shield = Shield;
		View.CurrentInitiative = Initiative;
		View.CurrentIntentId = FName(*FString::Printf(TEXT("Test.%s.Intent"), *PartSlotId.ToString()));
		View.CurrentIntentDisplayName = FText::FromString(TEXT("行动"));
		View.CurrentIntentInitiative = Initiative;
		View.bCurrentIntentIsAttack = true;
		View.CurrentIntentPeakAttackDamage = 3;
		View.bDestroyed = bDestroyed;
		return View;
	}

	FWacomBattleEnemyPanelViewData MakeEnemyView(
		const FName EnemySlotId,
		TArray<FWacomBattleEnemyPartEntryViewData> Parts)
	{
		FWacomBattleEnemyPanelViewData View;
		View.EncounterId = TEXT("Encounter");
		View.EnemySlotId = EnemySlotId;
		View.UnitKey = FBattleEnemyUnitKey::Make(TEXT("Encounter"), EnemySlotId);
		View.EnemyDisplayName = FText::FromString(TEXT("敌人"));
		View.Parts = MoveTemp(Parts);
		for (FWacomBattleEnemyPartEntryViewData& Part : View.Parts)
		{
			View.EnemyInitiativeSum += Part.CurrentInitiative;
		}
		return View;
	}

	template <typename TWidget>
	TWidget* FindWidget(UUserWidget* Owner, const FName Name)
	{
		return Owner && Owner->WidgetTree
			? Cast<TWidget>(Owner->WidgetTree->FindWidget(Name)) : nullptr;
	}

	UWidgetAnimation* FindAnimation(UUserWidget* Widget, const FName Name)
	{
		const UWidgetBlueprintGeneratedClass* GeneratedClass = Widget
			? Cast<UWidgetBlueprintGeneratedClass>(Widget->GetClass()) : nullptr;
		if (!GeneratedClass)
		{
			return nullptr;
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
		return nullptr;
	}

	float GetAuthoredAnimationEndTime(const UWidgetAnimation* Animation)
	{
		const double DisplayRate = Animation && Animation->MovieScene
			? Animation->MovieScene->GetDisplayRate().AsDecimal() : 0.0;
		return Animation && DisplayRate > 0.0
			? FMath::Max(0.0f,
				Animation->GetEndTime() - static_cast<float>(1.0 / DisplayRate))
			: 0.0f;
	}

	UWacomBattleEnemyPanelWidget* CreatePanel(UWorld* World)
	{
		UClass* PanelClass = LoadClass<UWacomBattleEnemyPanelWidget>(nullptr, PanelClassPath);
		return World && PanelClass ? CreateWidget<UWacomBattleEnemyPanelWidget>(World, PanelClass) : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPanelVisualContractSpec,
	"Wacom.UI.Battle.EnemyPanel.VisualContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPanelVisualContractSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyPanelSpec;
	UClass* PanelClass = LoadClass<UWacomBattleEnemyPanelWidget>(nullptr, PanelClassPath);
	UClass* PartClass = LoadClass<UWacomBattleEnemyPartEntryWidget>(nullptr, PartEntryClassPath);
	UWorld* World = FindAutomationWorld();
	UWacomBattleEnemyPanelWidget* Panel = World && PanelClass
		? CreateWidget<UWacomBattleEnemyPanelWidget>(World, PanelClass) : nullptr;
	UWacomBattleEnemyPartEntryWidget* Entry = World && PartClass
		? CreateWidget<UWacomBattleEnemyPartEntryWidget>(World, PartClass) : nullptr;
	if (!TestNotNull(TEXT("Formal enemy panel WBP"), PanelClass)
		|| !TestNotNull(TEXT("Formal enemy part-entry WBP"), PartClass)
		|| !TestNotNull(TEXT("Panel instance"), Panel)
		|| !TestNotNull(TEXT("Entry instance"), Entry))
	{
		return false;
	}
	Panel->TakeWidget();
	Entry->TakeWidget();
	TestTrue(TEXT("Panel is passive native widget"), PanelClass->IsChildOf(UWacomBattleEnemyPanelWidget::StaticClass()));
	TestTrue(TEXT("Entry is passive native widget"), PartClass->IsChildOf(UWacomBattleEnemyPartEntryWidget::StaticClass()));

	for (const FName Name : { FName(TEXT("PanelRoot")), FName(TEXT("PartList")) })
	{
		TestNotNull(*FString::Printf(TEXT("Panel binding %s"), *Name.ToString()),
			Panel->WidgetTree->FindWidget(Name));
	}
	const FName EntryBindings[] = {
		TEXT("PartEntryRoot"), TEXT("VitalsTrackImage"), TEXT("HpText"),
		TEXT("ShieldValueRoot"), TEXT("ShieldText"), TEXT("InitiativeSocket"),
		TEXT("InitiativeText"), TEXT("IntentSocket"), TEXT("IntentIcon"),
		TEXT("OutgoingIntentIcon"), TEXT("StatusList"), TEXT("StatusOverflowText"),
		TEXT("ContextSurface"), TEXT("DestroyedSurface"), TEXT("DestroyedMark"),
		TEXT("InspectHitTarget") };
	for (const FName Name : EntryBindings)
	{
		TestNotNull(*FString::Printf(TEXT("Entry binding %s"), *Name.ToString()),
			Entry->WidgetTree->FindWidget(Name));
	}
	for (const FName Legacy : { FName(TEXT("HpBar")), FName(TEXT("PartNameText")),
		FName(TEXT("IntentText")), FName(TEXT("ResistanceText")),
		FName(TEXT("DetailsContainer")), FName(TEXT("ActionPreviewOverlay")) })
	{
		TestNull(*FString::Printf(TEXT("Legacy binding %s removed"), *Legacy.ToString()),
			Entry->WidgetTree->FindWidget(Legacy));
	}

	const TPair<FName, float> Animations[] = {
		{ TEXT("IntroAnimation"), 0.22f }, { TEXT("DamageImpactAnimation"), 0.22f },
		{ TEXT("ShieldImpactAnimation"), 0.18f }, { TEXT("ShieldBreakAnimation"), 0.24f },
		{ TEXT("InitiativeStepAnimation"), 0.12f }, { TEXT("IntentChangeAnimation"), 0.18f },
		{ TEXT("ContextAnimation"), 0.12f }, { TEXT("DestroyedAnimation"), 0.30f } };
	for (const TPair<FName, float>& Expected : Animations)
	{
		UWidgetAnimation* Animation = FindAnimation(Entry, Expected.Key);
		TestNotNull(*FString::Printf(TEXT("Animation %s"), *Expected.Key.ToString()), Animation);
		TestTrue(*FString::Printf(TEXT("Animation %s duration"), *Expected.Key.ToString()),
			Animation && FMath::IsNearlyEqual(
				GetAuthoredAnimationEndTime(Animation), Expected.Value, 0.02f));
	}

	UImage* Vitals = FindWidget<UImage>(Entry, TEXT("VitalsTrackImage"));
	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, MaterialPath);
	UFont* Font = LoadObject<UFont>(nullptr, FontPath);
	UTextBlock* Hp = FindWidget<UTextBlock>(Entry, TEXT("HpText"));
	UTextBlock* Initiative = FindWidget<UTextBlock>(Entry, TEXT("InitiativeText"));
	UTextBlock* Shield = FindWidget<UTextBlock>(Entry, TEXT("ShieldText"));
	TestEqual(TEXT("Vitals image uses Enemy Vitals material"),
		Vitals ? Vitals->GetBrush().GetResourceObject() : nullptr, static_cast<UObject*>(Material));
	TestTrue(TEXT("HP uses Silkscreen Bold 18"), Hp && Hp->GetFont().FontObject == Font
		&& Hp->GetFont().TypefaceFontName == FName(TEXT("Bold")) && Hp->GetFont().Size == 18);
	TestTrue(TEXT("Initiative uses Silkscreen 16"),
		Initiative && Initiative->GetFont().FontObject == Font
		&& Initiative->GetFont().TypefaceFontName == FName(TEXT("Bold")) && Initiative->GetFont().Size == 16);
	TestTrue(TEXT("Shield uses Silkscreen 14"),
		Shield && Shield->GetFont().FontObject == Font
		&& Shield->GetFont().TypefaceFontName == FName(TEXT("Bold")) && Shield->GetFont().Size == 14);
	TestEqual(TEXT("Panel root is SelfHitTestInvisible"),
		Panel->WidgetTree->RootWidget->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	TestEqual(TEXT("Entry root is SelfHitTestInvisible"),
		Entry->WidgetTree->RootWidget->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	UButton* HitTarget = FindWidget<UButton>(Entry, TEXT("InspectHitTarget"));
	TestEqual(TEXT("Inspection hotspot is click-through before valid ViewData"),
		HitTarget ? HitTarget->GetVisibility() : ESlateVisibility::Collapsed,
		ESlateVisibility::HitTestInvisible);

	const UWacomUIDeveloperSettings* Settings = GetDefault<UWacomUIDeveloperSettings>();
	TestEqual(TEXT("Project default resolves to formal panel WBP"),
		Settings->DefaultBattleEnemyPanelWidgetClass.ToSoftObjectPath().ToString(), FString(PanelClassPath));
	TArray<FText> SettingsErrors;
	TestTrue(TEXT("Project enemy UI settings validate"), Settings->ValidateSettings(SettingsErrors));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPanelStableEntriesSpec,
	"Wacom.UI.Battle.EnemyPanel.SingleHostStableEntries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPanelStableEntriesSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyPanelSpec;
	UWacomBattleEnemyPanelWidget* Panel = CreatePanel(FindAutomationWorld());
	if (!TestNotNull(TEXT("Panel"), Panel))
	{
		return false;
	}
	Panel->TakeWidget();
	FWacomBattleEnemyPanelViewData Enemy = MakeEnemyView(TEXT("Enemy"), {
		MakePart(TEXT("Enemy"), TEXT("Head"), 10, 12, 0, 5),
		MakePart(TEXT("Enemy"), TEXT("Tail"), 6, 8, 2, 3) });
	Panel->SetEnemyPanelViewData(Enemy);
	UHorizontalBox* PartList = FindWidget<UHorizontalBox>(Panel, TEXT("PartList"));
	if (!TestNotNull(TEXT("Part list"), PartList)
		|| !TestEqual(TEXT("Two parts render"), PartList->GetChildrenCount(), 2))
	{
		return false;
	}
	UWidget* HeadWidget = PartList->GetChildAt(0);
	UWidget* TailWidget = PartList->GetChildAt(1);
	UWacomBattleEnemyPartEntryWidget* Head = Cast<UWacomBattleEnemyPartEntryWidget>(HeadWidget);
	UWacomBattleEnemyPartEntryWidget* Tail = Cast<UWacomBattleEnemyPartEntryWidget>(TailWidget);
	Panel->SetHoveredPartSlotId(TEXT("Tail"));
	TestEqual(TEXT("Only Tail context is visible"),
		FindWidget<UWidget>(Tail, TEXT("ContextSurface"))->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("Head context remains hidden"),
		FindWidget<UWidget>(Head, TEXT("ContextSurface"))->GetVisibility(), ESlateVisibility::Collapsed);
	Panel->SetHoveredPartSlotId(NAME_None);

	FWacomBattleEnemyPartEntryViewData TailPreview = Enemy.Parts[1];
	TailPreview.CurrentHp = 1;
	TestTrue(TEXT("Matching preview is accepted"), Panel->SetActionPreviewPartViews({ TailPreview }));
	TestFalse(TEXT("Preview does not affect Head"),
		FWacomBattleEnemyPartEntryWidgetTestAccess::HasPreview(*Head));
	TestTrue(TEXT("Preview affects Tail"),
		FWacomBattleEnemyPartEntryWidgetTestAccess::HasPreview(*Tail));
	Panel->ClearActionPreview();
	Enemy.Parts[0].CurrentHp = 8;
	Panel->SetEnemyPanelViewData(Enemy);
	TestTrue(TEXT("Head entry reused"), PartList->GetChildAt(0) == HeadWidget);
	TestTrue(TEXT("Tail entry reused"), PartList->GetChildAt(1) == TailWidget);
	Enemy.Parts.RemoveAt(1);
	Panel->SetEnemyPanelViewData(Enemy);
	TestEqual(TEXT("Removed part deletes its entry"), PartList->GetChildrenCount(), 1);
	TestTrue(TEXT("Unchanged part remains stable"), PartList->GetChildAt(0) == HeadWidget);
	Panel->ClearEnemyPanelViewData();
	TestEqual(TEXT("Battle clear removes entries"), PartList->GetChildrenCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPanelHostDefaultClassAndDesiredSizeSpec,
	"Wacom.UI.Battle.EnemyPanel.HostDefaultClassAndDesiredSize",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPanelHostDefaultClassAndDesiredSizeSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyPanelSpec;
	UWorld* World = FindAutomationWorld();
	AWacomBattleEnemyActor* Host = World ? World->SpawnActor<AWacomBattleEnemyActor>() : nullptr;
	if (!TestNotNull(TEXT("Host"), Host))
	{
		return false;
	}
	Host->SetEnemyPanelViewData(MakeEnemyView(TEXT("Enemy"), {
		MakePart(TEXT("Enemy"), TEXT("Body"), 24, 24, 0, 3) }));
	UWidgetComponent* Component = Cast<UWidgetComponent>(
		Host->GetDefaultSubobjectByName(TEXT("EnemyPanelWidget")));
	if (!TestNotNull(TEXT("Enemy panel component"), Component))
	{
		Host->Destroy();
		return false;
	}
	TestTrue(TEXT("Desired Size enabled"), Component->GetDrawAtDesiredSize());
	TestEqual(TEXT("Bottom-center pivot X"), Component->GetPivot().X, 0.5);
	TestEqual(TEXT("Bottom-center pivot Y"), Component->GetPivot().Y, 1.0);
	TestNotNull(TEXT("Project default panel instantiated"),
		Cast<UWacomBattleEnemyPanelWidget>(Component->GetUserWidgetObject()));
	Host->Destroy();
	return true;
}
