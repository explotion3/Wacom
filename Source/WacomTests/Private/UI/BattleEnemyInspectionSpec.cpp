// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "MovieScene.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Battle/WacomBattleEnemyInspectionPartRowWidget.h"
#include "UI/Battle/WacomBattleEnemyInspectionWidget.h"
#include "UI/Battle/WacomBattleEnemyIntentPresentationStyle.h"
#include "UI/Battle/WacomBattleIntentTooltipWidget.h"
#include "UI/Battle/WacomBattleStatusIconWidget.h"
#include "UI/Foundation/WacomUIDeveloperSettings.h"
#include "UI/WacomBattleEnemyInspectionWidgetTestAccess.h"

namespace WacomBattleEnemyInspectionSpec
{
	constexpr TCHAR InspectionClassPath[] =
		TEXT("/Game/Wacom/UI/Enemy/WBP_WacomBattleEnemyInspectionWidget.WBP_WacomBattleEnemyInspectionWidget_C");
	constexpr TCHAR RowClassPath[] =
		TEXT("/Game/Wacom/UI/Enemy/WBP_WacomBattleEnemyInspectionPartRowWidget.WBP_WacomBattleEnemyInspectionPartRowWidget_C");
	constexpr TCHAR IntentStylePath[] =
		TEXT("/Game/Wacom/UI/Enemy/Intent/DA_EnemyIntentPresentation_Default.DA_EnemyIntentPresentation_Default");
	constexpr TCHAR IntentTooltipClassPath[] =
		TEXT("/Game/Wacom/UI/Enemy/Intent/WBP_BattleIntentTooltip.WBP_BattleIntentTooltip_C");
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

	FWacomBattleEnemyPartEntryViewData MakePart(
		const FName PartSlotId,
		const int32 Hp,
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
		Part.CurrentHp = Hp;
		Part.MaxHp = MaxHp;
		Part.Shield = Shield;
		Part.CurrentInitiative = Initiative;
		Part.CurrentIntentId = FName(*FString::Printf(TEXT("Snake.%s.Intent"), *PartSlotId.ToString()));
		Part.CurrentIntentDisplayName = FText::FromString(TEXT("撕咬"));
		Part.CurrentIntentInitiative = Initiative + 1;
		Part.bCurrentIntentIsAttack = true;
		Part.CurrentIntentPeakAttackDamage = 5;
		FBattleIntentEffectSnapshot Damage;
		Damage.EffectType = WacomTags::Effect_Damage;
		Damage.Magnitude = 5;
		Damage.TargetKind = EBattleIntentEffectTargetKind::Player;
		Part.CurrentIntentEffects.Add(Damage);
		Part.bDestroyed = bDestroyed;
		return Part;
	}

	FWacomBattleEnemyInspectionViewData MakeView(const FName SelectedPartSlotId)
	{
		FWacomBattleEnemyInspectionViewData View;
		View.Enemy.EncounterId = TEXT("Encounter");
		View.Enemy.EnemySlotId = TEXT("Enemy");
		View.Enemy.UnitKey = FBattleEnemyUnitKey::Make(TEXT("Encounter"), TEXT("Enemy"));
		View.Enemy.EnemyDisplayName = FText::FromString(TEXT("林蛇"));
		View.Enemy.Parts = {
			MakePart(TEXT("Head"), 7, 12, 2, 3),
			MakePart(TEXT("Body"), 18, 24, 0, 2),
			MakePart(TEXT("Tail"), 0, 8, 0, 0, true),
		};
		View.Enemy.Parts[0].RuntimeStatuses.AddTag(WacomTags::Status_Poison);
		View.Enemy.Parts[0].RuntimeStatuses.AddTag(WacomTags::Status_Slow);
		View.Enemy.Parts[0].RuntimeStatuses.AddTag(WacomTags::Status_Freeze);
		View.Enemy.Parts[0].RuntimeStatuses.AddTag(WacomTags::Status_Twilight);
		View.Enemy.Parts[0].RuntimeStatuses.AddTag(WacomTags::Status_Stunned);
		View.SelectedPartIdentity = FBattlePartSlotIdentity::Make(
			TEXT("Encounter"), TEXT("Enemy"), SelectedPartSlotId);
		return View;
	}

	bool InvokeWidgetHandler(UUserWidget* Widget, const FName FunctionName)
	{
		UFunction* Function = Widget ? Widget->FindFunction(FunctionName) : nullptr;
		if (!Widget || !Function)
		{
			return false;
		}
		Widget->ProcessEvent(Function, nullptr);
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyInspectionAssetContractSpec,
	"Wacom.UI.Battle.EnemyInspection.AssetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyInspectionAssetContractSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyInspectionSpec;
	UClass* InspectionClass = LoadClass<UWacomBattleEnemyInspectionWidget>(nullptr, InspectionClassPath);
	UClass* RowClass = LoadClass<UWacomBattleEnemyInspectionPartRowWidget>(nullptr, RowClassPath);
	UClass* IntentTooltipClass =
		LoadClass<UWacomBattleIntentTooltipWidget>(
			nullptr, IntentTooltipClassPath);
	if (!TestNotNull(TEXT("Inspection WBP"), InspectionClass)
		|| !TestNotNull(TEXT("Inspection row WBP"), RowClass)
		|| !TestNotNull(TEXT("Intent tooltip WBP"), IntentTooltipClass))
	{
		return false;
	}
	const UWacomUIDeveloperSettings* Settings = GetDefault<UWacomUIDeveloperSettings>();
	TestEqual(TEXT("Project default inspection WBP"),
		Settings->DefaultBattleEnemyInspectionWidgetClass.LoadSynchronous(), InspectionClass);

	UWorld* World = FindAutomationWorld();
	UWacomBattleEnemyInspectionWidget* Widget = World
		? CreateWidget<UWacomBattleEnemyInspectionWidget>(World, InspectionClass)
		: nullptr;
	if (!TestNotNull(TEXT("Inspection instance"), Widget))
	{
		return false;
	}
	Widget->TakeWidget();
	TestEqual(TEXT("Inspection row class is authored"),
		Widget->GetPartRowWidgetClass().Get(), RowClass);
	UWacomBattleEnemyIntentPresentationStyle* IntentStyle =
		LoadObject<UWacomBattleEnemyIntentPresentationStyle>(nullptr, IntentStylePath);
	TestEqual(TEXT("Inspection uses formal Intent style"),
		Widget->GetIntentPresentationStyle(), IntentStyle);
	TestEqual(TEXT("Inspection uses formal Intent tooltip"),
		Widget->GetIntentTooltipWidgetClass().Get(), IntentTooltipClass);
	const TArray<FName> RequiredBindings = {
		TEXT("BackdropButton"),
		TEXT("LeftPanel"), TEXT("RightPanel"), TEXT("EnemyNameText"),
		TEXT("EnemyStateText"), TEXT("PartNavigator"), TEXT("SelectedPartNameText"),
		TEXT("HpBar"), TEXT("HpText"), TEXT("ShieldContainer"), TEXT("ShieldText"),
		TEXT("InitiativeText"), TEXT("IntentIcon"), TEXT("IntentText"), TEXT("ResistanceText"),
		TEXT("IntentTooltipTarget"),
		TEXT("StatusList"), TEXT("DestroyedOverlay"), TEXT("CloseButton") };
	for (const FName Binding : RequiredBindings)
	{
		TestNotNull(*FString::Printf(TEXT("Inspection binding %s"), *Binding.ToString()),
			Widget->WidgetTree->FindWidget(Binding));
	}
	TestNull(TEXT("Legacy Intent effects bounds are removed"),
		Widget->WidgetTree->FindWidget(TEXT("IntentEffectsBounds")));
	TestNull(TEXT("Legacy Intent effects scroll is removed"),
		Widget->WidgetTree->FindWidget(TEXT("IntentEffectsScroll")));
	TestNull(TEXT("Legacy Intent effects list is removed"),
		Widget->WidgetTree->FindWidget(TEXT("IntentEffectsList")));
	UButton* IntentTooltipTarget =
		FindWidget<UButton>(Widget, TEXT("IntentTooltipTarget"));
	UImage* IntentIcon = FindWidget<UImage>(Widget, TEXT("IntentIcon"));
	TestTrue(TEXT("Inspection Intent target owns only the icon-sized content"),
		IntentTooltipTarget && IntentIcon
		&& IntentTooltipTarget->GetContent() == IntentIcon
		&& IntentIcon->GetParent() == IntentTooltipTarget);
	UButton* BackdropButton = FindWidget<UButton>(Widget, TEXT("BackdropButton"));
	const UCanvasPanel* InspectionRoot =
		Cast<UCanvasPanel>(Widget->WidgetTree->RootWidget);
	const UCanvasPanelSlot* BackdropSlot = BackdropButton
		? Cast<UCanvasPanelSlot>(BackdropButton->Slot)
		: nullptr;
	TestTrue(TEXT("Backdrop is a non-focusable full-screen close surface"),
		BackdropButton
		&& !BackdropButton->GetIsFocusable()
		&& BackdropButton->GetVisibility() == ESlateVisibility::Visible
		&& InspectionRoot
		&& BackdropButton->GetParent() == InspectionRoot
		&& InspectionRoot->GetChildIndex(BackdropButton) == 0
		&& BackdropSlot
		&& BackdropSlot->GetAnchors().Minimum.Equals(FVector2D::ZeroVector)
		&& BackdropSlot->GetAnchors().Maximum.Equals(FVector2D(1.0f, 1.0f)));
	UWidgetAnimation* Open = FindAnimation(Widget, TEXT("OpenAnimation"));
	UWidgetAnimation* Close = FindAnimation(Widget, TEXT("CloseAnimation"));
	TestTrue(TEXT("Both dossier panels share one 240ms open timeline"),
		Open && Open->GetBindings().Num() >= 2
		&& FMath::IsNearlyEqual(GetAuthoredAnimationEndTime(Open), 0.24f, 0.02f));
	TestNull(TEXT("Legacy left-only opening timeline is removed"),
		FindAnimation(Widget, TEXT("OpenLeftAnimation")));
	TestNull(TEXT("Legacy right-only opening timeline is removed"),
		FindAnimation(Widget, TEXT("OpenRightAnimation")));
	TestTrue(TEXT("Close resolves over 160ms"),
		Close && FMath::IsNearlyEqual(GetAuthoredAnimationEndTime(Close), 0.16f, 0.02f));
	USizeBox* LeftPanel = FindWidget<USizeBox>(Widget, TEXT("LeftPanel"));
	USizeBox* RightPanel = FindWidget<USizeBox>(Widget, TEXT("RightPanel"));
	TestTrue(TEXT("Left dossier column is 220 x 520"),
		LeftPanel && LeftPanel->IsWidthOverride() && LeftPanel->IsHeightOverride()
		&& FMath::IsNearlyEqual(LeftPanel->GetWidthOverride(), 220.0f)
		&& FMath::IsNearlyEqual(LeftPanel->GetHeightOverride(), 520.0f));
	TestTrue(TEXT("Right dossier is 420 x 560"),
		RightPanel && RightPanel->IsWidthOverride() && RightPanel->IsHeightOverride()
		&& FMath::IsNearlyEqual(RightPanel->GetWidthOverride(), 420.0f)
		&& FMath::IsNearlyEqual(RightPanel->GetHeightOverride(), 560.0f));
	TestEqual(TEXT("Root allows only child hit testing"),
		Widget->WidgetTree->RootWidget->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	Widget->CloseInspection(true);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyInspectionSelectionAndLifecycleSpec,
	"Wacom.UI.Battle.EnemyInspection.SelectionAndLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyInspectionSelectionAndLifecycleSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyInspectionSpec;
	UClass* InspectionClass = LoadClass<UWacomBattleEnemyInspectionWidget>(nullptr, InspectionClassPath);
	UWorld* World = FindAutomationWorld();
	UWacomBattleEnemyInspectionWidget* Widget = World && InspectionClass
		? CreateWidget<UWacomBattleEnemyInspectionWidget>(World, InspectionClass)
		: nullptr;
	if (!TestNotNull(TEXT("Inspection instance"), Widget))
	{
		return false;
	}
	Widget->TakeWidget();
	FWacomBattleEnemyInspectionWidgetTestAccess::Construct(*Widget);
	FWacomBattleEnemyInspectionViewData View = MakeView(TEXT("Head"));
	if (!TestTrue(TEXT("View is accepted"), Widget->SetInspectionViewData(View)))
	{
		return false;
	}
	Widget->OpenInspection();
	TestTrue(TEXT("Inspection opens non-modally"), Widget->IsInspectionOpen());

	UPanelWidget* Navigator = FindWidget<UPanelWidget>(Widget, TEXT("PartNavigator"));
	UTextBlock* EnemyName = FindWidget<UTextBlock>(Widget, TEXT("EnemyNameText"));
	UTextBlock* EnemyState = FindWidget<UTextBlock>(Widget, TEXT("EnemyStateText"));
	UTextBlock* PartName = FindWidget<UTextBlock>(Widget, TEXT("SelectedPartNameText"));
	UTextBlock* HpText = FindWidget<UTextBlock>(Widget, TEXT("HpText"));
	UTextBlock* ShieldText = FindWidget<UTextBlock>(Widget, TEXT("ShieldText"));
	UTextBlock* ResistanceText = FindWidget<UTextBlock>(Widget, TEXT("ResistanceText"));
	UImage* IntentIcon = FindWidget<UImage>(Widget, TEXT("IntentIcon"));
	UButton* IntentTooltipTarget =
		FindWidget<UButton>(Widget, TEXT("IntentTooltipTarget"));
	UWacomBattleStatusIconListWidget* StatusList =
		FindWidget<UWacomBattleStatusIconListWidget>(Widget, TEXT("StatusList"));
	if (!TestNotNull(TEXT("Part navigator"), Navigator)
		|| !TestNotNull(TEXT("Enemy name"), EnemyName)
		|| !TestNotNull(TEXT("Enemy state"), EnemyState)
		|| !TestNotNull(TEXT("Part name"), PartName)
		|| !TestNotNull(TEXT("HP text"), HpText)
		|| !TestNotNull(TEXT("Shield text"), ShieldText)
		|| !TestNotNull(TEXT("Resistance text"), ResistanceText)
		|| !TestNotNull(TEXT("Intent icon"), IntentIcon)
		|| !TestNotNull(TEXT("Intent tooltip target"), IntentTooltipTarget)
		|| !TestNotNull(TEXT("Full status list"), StatusList)
		|| !TestEqual(TEXT("Definition order creates three navigation rows"),
			Navigator->GetChildrenCount(), 3))
	{
		return false;
	}
	TestEqual(TEXT("Enemy name is rendered"), EnemyName->GetText().ToString(), FString(TEXT("林蛇")));
	TestEqual(TEXT("Overall state reports remaining parts"),
		EnemyState->GetText().ToString(), FString(TEXT("部位 2 / 3")));
	TestEqual(TEXT("Selected Head name"), PartName->GetText().ToString(), FString(TEXT("Head")));
	TestEqual(TEXT("Details show current and max HP"), HpText->GetText().ToString(), FString(TEXT("7 / 12")));
	TestEqual(TEXT("Details show exact Shield"), ShieldText->GetText().ToString(), FString(TEXT("2")));
	TestEqual(TEXT("Details show intent initiative and attack damage"),
		ResistanceText->GetText().ToString(), FString(TEXT("INIT 4   ATK 5")));
	TestNotNull(TEXT("Unknown Snake intent uses formal fallback icon"),
		IntentIcon->GetBrush().GetResourceObject());
	TestTrue(TEXT("Selected intent tooltip delegate is bound"),
		IntentTooltipTarget->ToolTipWidgetDelegate.IsBound());
	UWacomBattleIntentTooltipWidget* IntentTooltip =
		Cast<UWacomBattleIntentTooltipWidget>(
			IntentTooltipTarget->ToolTipWidgetDelegate.Execute());
	if (!TestNotNull(TEXT("Intent icon still creates the formal tooltip"),
		IntentTooltip))
	{
		return false;
	}
	const FWacomBattleIntentPresentationViewData& IntentTooltipView =
		IntentTooltip->GetIntentViewData();
	TestEqual(TEXT("Tooltip retains the selected intent effect"),
		IntentTooltipView.EffectRows.Num(), 1);
	if (!IntentTooltipView.EffectRows.IsEmpty())
	{
		TestEqual(TEXT("Tooltip retains the authoritative target"),
			IntentTooltipView.EffectRows[0].TargetText.ToString(),
			FString(TEXT("[玩家]")));
		TestEqual(TEXT("Tooltip retains the authoritative damage"),
			IntentTooltipView.EffectRows[0].EffectText.ToString(),
			FString(TEXT("伤害 5")));
	}
	TestEqual(TEXT("Details do not truncate Buffs"), StatusList->GetMaxVisibleStatuses(), 0);
	TestEqual(TEXT("Details retain all Buffs"), StatusList->GetStatusIconViews().Num(), 5);

	UWidget* HeadRow = Navigator->GetChildAt(0);
	UWacomBattleEnemyInspectionPartRowWidget* BodyRow =
		Cast<UWacomBattleEnemyInspectionPartRowWidget>(Navigator->GetChildAt(1));
	UButton* BodySelect = FindWidget<UButton>(BodyRow, TEXT("PartSelectButton"));
	if (!TestNotNull(TEXT("Body navigation row"), BodyRow)
		|| !TestNotNull(TEXT("Body selection button"), BodySelect))
	{
		return false;
	}
	int32 SelectionRequestCount = 0;
	Widget->OnSelectionRequestedNative.AddLambda(
		[&SelectionRequestCount](const FBattlePartSlotIdentity&)
		{
			++SelectionRequestCount;
		});
	TestTrue(TEXT("Body selection button accepts input"), BodySelect->GetIsEnabled());
	TestTrue(TEXT("Body selection handler is callable"),
		InvokeWidgetHandler(BodyRow, TEXT("HandleSelectClicked")));
	TestEqual(TEXT("Body click emits one stable selection request"), SelectionRequestCount, 1);
	TestEqual(TEXT("Body becomes selected"),
		Widget->GetInspectionViewData().SelectedPartIdentity.GetEffectivePartSlotId(),
		FName(TEXT("Body")));
	TestEqual(TEXT("Right details switch in place"), PartName->GetText().ToString(), FString(TEXT("Body")));
	TestEqual(TEXT("Status region is empty for a part without Buffs"),
		StatusList->GetStatusIconViews().Num(), 0);

	View = Widget->GetInspectionViewData();
	View.Enemy.Parts[1].CurrentHp = 12;
	View.Enemy.Parts[1].bCurrentIntentIsAttack = false;
	View.Enemy.Parts[1].CurrentIntentPeakAttackDamage = 0;
	TestTrue(TEXT("Snapshot refresh updates existing widget"), Widget->SetInspectionViewData(View));
	TestTrue(TEXT("Navigation row is reused"), Navigator->GetChildAt(0) == HeadRow);
	TestEqual(TEXT("Selected Body HP refreshes in place"), HpText->GetText().ToString(), FString(TEXT("12 / 24")));
	TestEqual(TEXT("Non-attack intent hides ATK"),
		ResistanceText->GetText().ToString(), FString(TEXT("INIT 3")));

	View.Enemy.Parts.RemoveAt(1);
	TestTrue(TEXT("Removing selected part falls back to a valid part"),
		Widget->SetInspectionViewData(View));
	TestEqual(TEXT("Removed part deletes exactly one row"), Navigator->GetChildrenCount(), 2);
	TestEqual(TEXT("Selection falls back to Head"),
		Widget->GetInspectionViewData().SelectedPartIdentity.GetEffectivePartSlotId(),
		FName(TEXT("Head")));

	int32 CloseRequestCount = 0;
	Widget->OnCloseRequestedNative.AddLambda([&CloseRequestCount]() { ++CloseRequestCount; });
	UButton* CloseButton = FindWidget<UButton>(Widget, TEXT("CloseButton"));
	UButton* BackdropButton = FindWidget<UButton>(Widget, TEXT("BackdropButton"));
	if (!TestNotNull(TEXT("Close button"), CloseButton)
		|| !TestNotNull(TEXT("Backdrop button"), BackdropButton))
	{
		return false;
	}
	TestTrue(TEXT("Backdrop handler is callable"),
		InvokeWidgetHandler(Widget, TEXT("HandleBackdropClicked")));
	TestEqual(TEXT("Backdrop emits one passive close request"), CloseRequestCount, 1);
	Widget->CloseInspection(false);
	Widget->OpenInspection();
	TestTrue(TEXT("Reopen interrupts retirement without leaving the dossier closed"),
		Widget->IsInspectionOpen());
	TestTrue(TEXT("Close handler is callable"),
		InvokeWidgetHandler(Widget, TEXT("HandleCloseClicked")));
	TestEqual(TEXT("Close button emits one additional passive request"), CloseRequestCount, 2);
	Widget->CloseInspection(true);
	TestFalse(TEXT("Immediate lifecycle clear closes the widget"), Widget->IsInspectionOpen());
	TestNull(TEXT("Closed inspection does not build a stale Intent tooltip"),
		IntentTooltipTarget->ToolTipWidgetDelegate.Execute());
	Widget->ClearInspectionViewData();
	TestEqual(TEXT("Battle clear removes cached navigation rows"), Navigator->GetChildrenCount(), 0);
	TestEqual(TEXT("Battle clear leaves the status region empty"),
		StatusList->GetStatusIconViews().Num(), 0);
	return true;
}
