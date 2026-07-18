// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/WidgetComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UI/Battle/WacomBattleEnemyPanelWidget.h"
#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"
#include "UI/Foundation/WacomUIDeveloperSettings.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomBattleEnemyPanelSpec
{
	constexpr TCHAR PanelClassPath[] =
		TEXT("/Game/Wacom/UI/Enemy/BP_WacomBattleEnemyPanelWidget.BP_WacomBattleEnemyPanelWidget_C");
	constexpr TCHAR PartEntryClassPath[] =
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

	FWacomBattleEnemyPartEntryViewData MakePart(
		const FName EnemySlotId,
		const FName PartSlotId,
		const FString& DisplayName,
		const int32 CurrentHp,
		const int32 MaxHp,
		const int32 Shield,
		const int32 Initiative,
		const FString& Intent,
		const int32 Resistance = 0,
		const bool bDestroyed = false)
	{
		FWacomBattleEnemyPartEntryViewData View;
		View.EnemySlotId = EnemySlotId;
		View.PartSlotId = PartSlotId;
		View.Identity = FBattlePartSlotIdentity::Make(TEXT("Encounter"), EnemySlotId, PartSlotId);
		View.PartDisplayName = FText::FromString(DisplayName);
		View.CurrentHp = CurrentHp;
		View.MaxHp = MaxHp;
		View.Shield = Shield;
		View.CurrentInitiative = Initiative;
		View.CurrentIntentDisplayName = FText::FromString(Intent);
		View.CurrentIntentInitiative = Initiative;
		View.CurrentIntentResistanceValue = Resistance;
		View.bDestroyed = bDestroyed;
		return View;
	}

	FWacomBattleEnemyPanelViewData MakeEnemyView(
		const FName EnemySlotId,
		const FString& DisplayName,
		TArray<FWacomBattleEnemyPartEntryViewData> Parts)
	{
		FWacomBattleEnemyPanelViewData View;
		View.EncounterId = TEXT("Encounter");
		View.EnemySlotId = EnemySlotId;
		View.UnitKey = FBattleEnemyUnitKey::Make(TEXT("Encounter"), EnemySlotId);
		View.EnemyDisplayName = FText::FromString(DisplayName);
		View.Parts = MoveTemp(Parts);
		for (FWacomBattleEnemyPartEntryViewData& Part : View.Parts)
		{
			Part.EnemySlotId = EnemySlotId;
			Part.Identity = FBattlePartSlotIdentity::Make(
				TEXT("Encounter"), EnemySlotId, Part.PartSlotId);
			View.EnemyInitiativeSum += Part.CurrentInitiative;
		}
		return View;
	}

	template <typename TWidget>
	TWidget* FindWidget(UWidgetTree* WidgetTree, const FName Name)
	{
		return WidgetTree ? Cast<TWidget>(WidgetTree->FindWidget(Name)) : nullptr;
	}

	UWidgetAnimation* FindAnimation(UUserWidget* Widget, const FName AnimationName)
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
				&& (Animation->GetFName() == AnimationName
					|| Animation->GetDisplayLabel() == AnimationName.ToString()
					|| Animation->GetName().StartsWith(AnimationName.ToString())))
			{
				return Animation;
			}
		}
		return nullptr;
	}

	UWacomBattleEnemyPanelWidget* CreatePanel(UWorld* World)
	{
		UClass* PanelClass = LoadClass<UWacomBattleEnemyPanelWidget>(nullptr, PanelClassPath);
		return World && PanelClass
			? CreateWidget<UWacomBattleEnemyPanelWidget>(World, PanelClass)
			: nullptr;
	}

	UWacomBattleEnemyPartEntryWidget* CreatePartEntry(UWorld* World)
	{
		UClass* PartClass = LoadClass<UWacomBattleEnemyPartEntryWidget>(nullptr, PartEntryClassPath);
		return World && PartClass
			? CreateWidget<UWacomBattleEnemyPartEntryWidget>(World, PartClass)
			: nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPanelFormalWBPContractSpec,
	"Wacom.UI.Battle.EnemyPanel.FormalWBPContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPanelFormalWBPContractSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyPanelSpec;

	UClass* PanelClass = LoadClass<UWacomBattleEnemyPanelWidget>(nullptr, PanelClassPath);
	UClass* PartClass = LoadClass<UWacomBattleEnemyPartEntryWidget>(nullptr, PartEntryClassPath);
	if (!TestNotNull(TEXT("Formal enemy panel WBP"), PanelClass)
		|| !TestNotNull(TEXT("Formal enemy part-entry WBP"), PartClass))
	{
		return false;
	}

	TestTrue(TEXT("Panel WBP uses passive native parent"),
		PanelClass->IsChildOf(UWacomBattleEnemyPanelWidget::StaticClass()));
	TestTrue(TEXT("Part WBP uses passive native parent"),
		PartClass->IsChildOf(UWacomBattleEnemyPartEntryWidget::StaticClass()));

	UWorld* World = FindAutomationWorld();
	UWacomBattleEnemyPanelWidget* Panel = CreatePanel(World);
	UWacomBattleEnemyPartEntryWidget* PartEntry = CreatePartEntry(World);
	if (!TestNotNull(TEXT("Panel instance"), Panel)
		|| !TestNotNull(TEXT("Part entry instance"), PartEntry))
	{
		return false;
	}
	Panel->TakeWidget();
	PartEntry->TakeWidget();

	const TArray<FName> PanelBindings = {
		TEXT("EnemyNameText"), TEXT("EnemyInitiativeText"), TEXT("PartList"),
		TEXT("PanelContextHighlight") };
	for (const FName Binding : PanelBindings)
	{
		TestNotNull(*FString::Printf(TEXT("Panel binding %s"), *Binding.ToString()),
			Panel->WidgetTree ? Panel->WidgetTree->FindWidget(Binding) : nullptr);
	}

	const TArray<FName> PartBindings = {
		TEXT("PartNameText"), TEXT("InitiativeDiamond"), TEXT("IntentDiamond"),
		TEXT("IntentIcon"), TEXT("HpBar"), TEXT("HpText"), TEXT("ShieldContainer"),
		TEXT("ShieldFrame"), TEXT("ShieldBadge"), TEXT("ShieldText"),
		TEXT("InitiativeText"), TEXT("IntentText"),
		TEXT("ResistanceText"), TEXT("DetailsContainer"), TEXT("StatusList"),
		TEXT("StatusOverflowText"),
		TEXT("ContextHighlight"), TEXT("ActionPreviewOverlay"), TEXT("DestroyedOverlay"),
		TEXT("DestroyedMark"),
		TEXT("InspectHitTarget") };
	for (const FName Binding : PartBindings)
	{
		TestNotNull(*FString::Printf(TEXT("Part binding %s"), *Binding.ToString()),
			PartEntry->WidgetTree ? PartEntry->WidgetTree->FindWidget(Binding) : nullptr);
	}

	const TArray<FName> AnimationNames = {
		TEXT("IntroAnimation"), TEXT("DamagePulseAnimation"),
		TEXT("ShieldPulseAnimation"), TEXT("DestroyedPulseAnimation"),
		TEXT("ContextHighlightAnimation") };
	for (const FName AnimationName : AnimationNames)
	{
		UWidgetAnimation* Animation = FindAnimation(PartEntry, AnimationName);
		TestNotNull(*FString::Printf(TEXT("Animation %s"), *AnimationName.ToString()), Animation);
		TestTrue(*FString::Printf(TEXT("Animation %s has a widget binding"), *AnimationName.ToString()),
			Animation && !Animation->GetBindings().IsEmpty());
	}

	bool bHitTestPolicyValid = true;
	const UWidget* PanelRoot = Panel->WidgetTree->RootWidget;
	const UWidget* PartEntryRoot = PartEntry->WidgetTree->RootWidget;
	Panel->WidgetTree->ForEachWidget([&bHitTestPolicyValid, PanelRoot](UWidget* Widget)
	{
		bHitTestPolicyValid &= Widget && (Widget == PanelRoot
			|| (Widget->GetVisibility() == ESlateVisibility::SelfHitTestInvisible
				&& Widget->IsA<UPanelWidget>())
			|| (Widget->GetVisibility() != ESlateVisibility::Visible
				&& Widget->GetVisibility() != ESlateVisibility::SelfHitTestInvisible));
	});
	PartEntry->WidgetTree->ForEachWidget([&bHitTestPolicyValid, PartEntryRoot](UWidget* Widget)
	{
		const bool bExpectedButton = Widget && Widget->GetFName() == TEXT("InspectHitTarget")
			&& Widget->IsA<UButton>();
		bHitTestPolicyValid &= Widget && (Widget == PartEntryRoot
			|| bExpectedButton
			|| (Widget->GetVisibility() == ESlateVisibility::SelfHitTestInvisible
				&& Widget->IsA<UPanelWidget>())
			|| (Widget->GetVisibility() != ESlateVisibility::Visible
				&& Widget->GetVisibility() != ESlateVisibility::SelfHitTestInvisible));
	});
	TestTrue(TEXT("Only the explicit inspection hotspot captures hit tests"), bHitTestPolicyValid);

	const UWacomUIDeveloperSettings* Settings = GetDefault<UWacomUIDeveloperSettings>();
	TestEqual(TEXT("Project default resolves to formal panel WBP"),
		Settings->DefaultBattleEnemyPanelWidgetClass.ToSoftObjectPath().ToString(),
		FString(PanelClassPath));
	TArray<FText> SettingsErrors;
	TestTrue(TEXT("Project default enemy panel settings validate"),
		Settings->ValidateSettings(SettingsErrors));

	TStrongObjectPtr<UWacomUIDeveloperSettings> InvalidSettings(
		NewObject<UWacomUIDeveloperSettings>());
	InvalidSettings->DefaultBattleEnemyPanelWidgetClass =
		UWacomBattleEnemyPanelWidget::StaticClass();
	SettingsErrors.Reset();
	TestFalse(TEXT("Abstract native panel class is rejected"),
		InvalidSettings->ValidateSettings(SettingsErrors));
	TestTrue(TEXT("Invalid panel class reports a validation error"),
		!SettingsErrors.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPartEntryViewAndPreviewSpec,
	"Wacom.UI.Battle.EnemyPanel.PartEntryViewAndPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPartEntryViewAndPreviewSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyPanelSpec;
	UWacomBattleEnemyPartEntryWidget* Widget = CreatePartEntry(FindAutomationWorld());
	if (!TestNotNull(TEXT("Part entry"), Widget))
	{
		return false;
	}
	Widget->TakeWidget();

	FWacomBattleEnemyPartEntryViewData View = MakePart(
		TEXT("Enemy"), TEXT("Head"), TEXT("蛇头"), 7, 12, 4, 3, TEXT("撕咬"), 5);
	Widget->SetPartEntryViewData(View);

	UTextBlock* HpText = FindWidget<UTextBlock>(Widget->WidgetTree, TEXT("HpText"));
	UTextBlock* ShieldText = FindWidget<UTextBlock>(Widget->WidgetTree, TEXT("ShieldText"));
	UTextBlock* InitiativeText = FindWidget<UTextBlock>(Widget->WidgetTree, TEXT("InitiativeText"));
	UTextBlock* IntentText = FindWidget<UTextBlock>(Widget->WidgetTree, TEXT("IntentText"));
	UTextBlock* ResistanceText = FindWidget<UTextBlock>(Widget->WidgetTree, TEXT("ResistanceText"));
	UProgressBar* HpBar = FindWidget<UProgressBar>(Widget->WidgetTree, TEXT("HpBar"));
	if (!TestNotNull(TEXT("HP text"), HpText)
		|| !TestNotNull(TEXT("Shield text"), ShieldText)
		|| !TestNotNull(TEXT("Initiative text"), InitiativeText)
		|| !TestNotNull(TEXT("Intent text"), IntentText)
		|| !TestNotNull(TEXT("Resistance text"), ResistanceText)
		|| !TestNotNull(TEXT("HP bar"), HpBar))
	{
		return false;
	}

	TestEqual(TEXT("HP text is current value only"), HpText->GetText().ToString(), FString(TEXT("7")));
	TestTrue(TEXT("HP percent"), FMath::IsNearlyEqual(HpBar->GetPercent(), 7.0f / 12.0f));
	TestEqual(TEXT("Shield text"), ShieldText->GetText().ToString(), FString(TEXT("4")));
	TestEqual(TEXT("Initiative text"), InitiativeText->GetText().ToString(), FString(TEXT("3")));
	TestEqual(TEXT("Intent text includes intent initiative"),
		IntentText->GetText().ToString(), FString(TEXT("撕咬  3")));
	TestEqual(TEXT("Resistance text"), ResistanceText->GetText().ToString(), FString(TEXT("RES 5")));

	FWacomBattleEnemyPartEntryViewData Preview = View;
	Preview.CurrentHp = 2;
	Preview.Shield = 0;
	Widget->SetActionPreview(Preview);
	TestTrue(TEXT("Preview active"), Widget->HasActionPreview());
	TestEqual(TEXT("Preview does not mutate Snapshot facts"), Widget->GetPartEntryViewData().CurrentHp, 7);
	TestEqual(TEXT("Preview updates displayed HP"), HpText->GetText().ToString(), FString(TEXT("2")));
	Widget->ClearActionPreview();
	TestEqual(TEXT("Clear restores Snapshot HP"), HpText->GetText().ToString(), FString(TEXT("7")));

	View.bDestroyed = true;
	View.CurrentHp = 0;
	Widget->SetPartEntryViewData(View);
	TestEqual(TEXT("Destroyed entry remains but is weakened"), Widget->GetRenderOpacity(), 0.64f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPartEntrySemanticPulseSpec,
	"Wacom.UI.Battle.EnemyPanel.PartEntrySemanticPulse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPartEntrySemanticPulseSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyPanelSpec;
	UWacomBattleEnemyPartEntryWidget* Widget = CreatePartEntry(FindAutomationWorld());
	if (!TestNotNull(TEXT("Part entry"), Widget))
	{
		return false;
	}
	Widget->TakeWidget();

	UWidgetAnimation* DamageAnimation = FindAnimation(Widget, TEXT("DamagePulseAnimation"));
	UWidgetAnimation* ShieldAnimation = FindAnimation(Widget, TEXT("ShieldPulseAnimation"));
	UWidgetAnimation* DestroyedAnimation = FindAnimation(Widget, TEXT("DestroyedPulseAnimation"));
	if (!TestNotNull(TEXT("Damage animation"), DamageAnimation)
		|| !TestNotNull(TEXT("Shield animation"), ShieldAnimation)
		|| !TestNotNull(TEXT("Destroyed animation"), DestroyedAnimation))
	{
		return false;
	}

	FWacomBattleEnemyPartEntryViewData View = MakePart(
		TEXT("Enemy"), TEXT("Body"), TEXT("身体"), 12, 12, 0, 3, TEXT("攻击"));
	Widget->SetPartEntryViewData(View);
	Widget->StopAllAnimations();

	FWacomBattleEnemyPartEntryViewData Preview = View;
	Preview.CurrentHp = 4;
	Preview.Shield = 5;
	Preview.bDestroyed = true;
	Widget->SetActionPreview(Preview);
	TestFalse(TEXT("Preview does not play damage pulse"), Widget->IsAnimationPlaying(DamageAnimation));
	TestFalse(TEXT("Preview does not play shield pulse"), Widget->IsAnimationPlaying(ShieldAnimation));
	TestFalse(TEXT("Preview does not play destroyed pulse"), Widget->IsAnimationPlaying(DestroyedAnimation));
	Widget->ClearActionPreview();
	Widget->StopAllAnimations();

	View.CurrentHp = 8;
	Widget->SetPartEntryViewData(View);
	TestTrue(TEXT("Real HP decrease plays damage pulse"), Widget->IsAnimationPlaying(DamageAnimation));
	Widget->StopAllAnimations();

	View.Shield = 4;
	Widget->SetPartEntryViewData(View);
	TestTrue(TEXT("Real shield change plays shield pulse"), Widget->IsAnimationPlaying(ShieldAnimation));
	Widget->StopAllAnimations();

	View.CurrentHp = 0;
	View.bDestroyed = true;
	Widget->SetPartEntryViewData(View);
	TestTrue(TEXT("Real destroyed transition plays destroyed pulse"),
		Widget->IsAnimationPlaying(DestroyedAnimation));
	Widget->CancelPendingPresentation();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPanelSingleHostStableEntriesSpec,
	"Wacom.UI.Battle.EnemyPanel.SingleHostStableEntries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPanelSingleHostStableEntriesSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyPanelSpec;
	UWacomBattleEnemyPanelWidget* Panel = CreatePanel(FindAutomationWorld());
	if (!TestNotNull(TEXT("Panel"), Panel))
	{
		return false;
	}
	Panel->TakeWidget();

	FWacomBattleEnemyPanelViewData Enemy = MakeEnemyView(
		TEXT("Enemy"), TEXT("林蛇"), {
			MakePart(TEXT("Enemy"), TEXT("Head"), TEXT("蛇头"), 10, 12, 0, 5, TEXT("撕咬")),
			MakePart(TEXT("Enemy"), TEXT("Tail"), TEXT("蛇尾"), 6, 8, 2, 3, TEXT("扫尾")) });
	Panel->SetEnemyPanelViewData(Enemy);

	UPanelWidget* PartList = FindWidget<UPanelWidget>(Panel->WidgetTree, TEXT("PartList"));
	UTextBlock* EnemyName = FindWidget<UTextBlock>(Panel->WidgetTree, TEXT("EnemyNameText"));
	if (!TestNotNull(TEXT("Part list"), PartList)
		|| !TestNotNull(TEXT("Enemy name"), EnemyName))
	{
		return false;
	}
	TestEqual(TEXT("Only this Host's two parts render"), PartList->GetChildrenCount(), 2);
	TestEqual(TEXT("Enemy header"), EnemyName->GetText().ToString(), FString(TEXT("林蛇")));

	UWidget* HeadEntry = PartList->GetChildAt(0);
	UWidget* TailEntry = PartList->GetChildAt(1);
	UWacomBattleEnemyPartEntryWidget* HeadPartEntry =
		Cast<UWacomBattleEnemyPartEntryWidget>(HeadEntry);
	UWacomBattleEnemyPartEntryWidget* TailPartEntry =
		Cast<UWacomBattleEnemyPartEntryWidget>(TailEntry);
	if (!TestNotNull(TEXT("Head part entry"), HeadPartEntry)
		|| !TestNotNull(TEXT("Tail part entry"), TailPartEntry))
	{
		return false;
	}

	Panel->SetHoveredPartSlotId(TEXT("Tail"));
	TestEqual(TEXT("Head remains compact on tail hover"),
		HeadPartEntry->WidgetTree->FindWidget(TEXT("DetailsContainer"))->GetVisibility(),
		ESlateVisibility::Collapsed);
	TestEqual(TEXT("Tail keeps inline details compact on hover"),
		TailPartEntry->WidgetTree->FindWidget(TEXT("DetailsContainer"))->GetVisibility(),
		ESlateVisibility::Collapsed);
	TestEqual(TEXT("Only the hovered Tail uses compact context highlight"),
		TailPartEntry->WidgetTree->FindWidget(TEXT("ContextHighlight"))->GetVisibility(),
		ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("Unmatched Head has no compact context highlight"),
		HeadPartEntry->WidgetTree->FindWidget(TEXT("ContextHighlight"))->GetVisibility(),
		ESlateVisibility::Collapsed);
	Panel->SetHoveredPartSlotId(NAME_None);

	FWacomBattleEnemyPartEntryViewData TailPreview = Enemy.Parts[1];
	TailPreview.CurrentHp = 1;
	TestTrue(TEXT("Matching preview is accepted"),
		Panel->SetActionPreviewPartViews({ TailPreview }));
	TestFalse(TEXT("Preview does not affect unmatched head"), HeadPartEntry->HasActionPreview());
	TestTrue(TEXT("Preview affects matching tail"), TailPartEntry->HasActionPreview());
	Panel->ClearActionPreview();
	TestFalse(TEXT("Preview clear restores tail"), TailPartEntry->HasActionPreview());

	Enemy.Parts[0].CurrentHp = 8;
	Panel->SetEnemyPanelViewData(Enemy);
	TestTrue(TEXT("Head entry reused"), PartList->GetChildAt(0) == HeadEntry);
	TestTrue(TEXT("Tail entry reused"), PartList->GetChildAt(1) == TailEntry);

	Enemy.Parts.RemoveAt(1);
	Panel->SetEnemyPanelViewData(Enemy);
	TestEqual(TEXT("Removed part destroys cached entry"), PartList->GetChildrenCount(), 1);
	TestTrue(TEXT("Unchanged part remains stable"), PartList->GetChildAt(0) == HeadEntry);

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

	FWacomBattleEnemyPanelViewData Enemy = MakeEnemyView(
		TEXT("Enemy"), TEXT("训练战士"), {
			MakePart(TEXT("Enemy"), TEXT("Body"), TEXT("身体"), 24, 24, 0, 3, TEXT("攻击")) });
	Host->SetEnemyPanelViewData(Enemy);

	UWidgetComponent* Component = Cast<UWidgetComponent>(
		Host->GetDefaultSubobjectByName(TEXT("EnemyPanelWidget")));
	if (!TestNotNull(TEXT("Enemy panel component"), Component))
	{
		Host->Destroy();
		return false;
	}
	TestTrue(TEXT("Desired Size enabled by default"), Component->GetDrawAtDesiredSize());
	TestEqual(TEXT("Bottom-center pivot X"), Component->GetPivot().X, 0.5);
	TestEqual(TEXT("Bottom-center pivot Y"), Component->GetPivot().Y, 1.0);
	TestNotNull(TEXT("Project default panel instantiated"),
		Cast<UWacomBattleEnemyPanelWidget>(Component->GetUserWidgetObject()));
	Host->Destroy();
	return true;
}
