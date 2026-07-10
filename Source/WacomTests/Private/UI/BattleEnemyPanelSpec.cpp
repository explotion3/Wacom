// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "BattleEnemyPanelTestWidgets.h"
#include "Actors/WacomBattleEnemyActor.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Widget.h"
#include "Components/WidgetComponent.h"
#include "Components/VerticalBox.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Battle/WacomBattleEnemyPanelViewData.h"
#include "UI/Battle/WacomBattleEnemyPanelWidget.h"
#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomBattleEnemyPanelSpec
{
	FWacomBattleEnemyPartEntryViewData MakePart(
		FName PartSlotId,
		const FString& DisplayName,
		int32 CurrentHp,
		int32 MaxHp,
		int32 Shield,
		int32 Initiative,
		const FString& Intent,
		bool bDestroyed = false)
	{
		FWacomBattleEnemyPartEntryViewData View;
		View.PartSlotId = PartSlotId;
		View.EnemySlotId = TEXT("Enemy");
		View.Identity = FBattlePartSlotIdentity::Make(TEXT("Encounter"), View.EnemySlotId, PartSlotId);
		View.PartDisplayName = FText::FromString(DisplayName);
		View.CurrentHp = CurrentHp;
		View.MaxHp = MaxHp;
		View.Shield = Shield;
		View.CurrentInitiative = Initiative;
		View.CurrentIntentDisplayName = FText::FromString(Intent);
		View.CurrentIntentInitiative = Initiative;
		View.bDestroyed = bDestroyed;
		return View;
	}

	UTextBlock* FindTextBlock(UWidgetTree* WidgetTree, FName WidgetName)
	{
		if (!WidgetTree)
		{
			return nullptr;
		}
		return Cast<UTextBlock>(WidgetTree->FindWidget(WidgetName));
	}

	UVerticalBox* FindVerticalBox(UWidgetTree* WidgetTree, FName WidgetName)
	{
		if (!WidgetTree)
		{
			return nullptr;
		}
		return Cast<UVerticalBox>(WidgetTree->FindWidget(WidgetName));
	}

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

	UWacomBattleEnemyPartEntryWidget* GetPartEntryAt(
		UVerticalBox* EnemyListBox,
		int32 EnemyIndex,
		int32 PartIndex)
	{
		if (!EnemyListBox)
		{
			return nullptr;
		}

		UVerticalBox* EnemyBox = nullptr;
		if (UBorder* EnemyBorder = Cast<UBorder>(EnemyListBox->GetChildAt(EnemyIndex)))
		{
			EnemyBox = Cast<UVerticalBox>(EnemyBorder->GetContent());
		}
		else
		{
			EnemyBox = Cast<UVerticalBox>(EnemyListBox->GetChildAt(EnemyIndex));
		}

		return EnemyBox ? Cast<UWacomBattleEnemyPartEntryWidget>(EnemyBox->GetChildAt(PartIndex + 1)) : nullptr;
	}

	UVerticalBox* GetEnemyBoxAt(UVerticalBox* EnemyListBox, int32 EnemyIndex)
	{
		if (!EnemyListBox)
		{
			return nullptr;
		}

		if (UBorder* EnemyBorder = Cast<UBorder>(EnemyListBox->GetChildAt(EnemyIndex)))
		{
			return Cast<UVerticalBox>(EnemyBorder->GetContent());
		}

		return Cast<UVerticalBox>(EnemyListBox->GetChildAt(EnemyIndex));
	}

	FWacomBattleEnemyPanelViewData MakeEnemyView(
		FName EnemySlotId,
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
			Part.Identity = FBattlePartSlotIdentity::Make(TEXT("Encounter"), EnemySlotId, Part.PartSlotId);
		}
		return View;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPartEntryShowsViewDataSpec,
	"Wacom.UI.Battle.EnemyPanel.PartEntryShowsViewData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPartEntryShowsViewDataSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyPanelSpec;

	TStrongObjectPtr<UWacomBattleEnemyPartEntryWidget> Widget(NewObject<UWacomBattleEnemyPartEntryWidget>());
	Widget->TakeWidget();

	FWacomBattleEnemyPartEntryViewData View = MakePart(
		TEXT("Head"),
		TEXT("蛇头"),
		7,
		12,
		4,
		3,
		TEXT("撕咬"));
	Widget->SetPartEntryViewData(View);

	UWidgetTree* WidgetTree = Widget->WidgetTree;
	UTextBlock* PartNameText = FindTextBlock(WidgetTree, TEXT("PartNameText"));
	UTextBlock* HpText = FindTextBlock(WidgetTree, TEXT("HpText"));
	UTextBlock* ShieldText = FindTextBlock(WidgetTree, TEXT("ShieldText"));
	UTextBlock* InitiativeText = FindTextBlock(WidgetTree, TEXT("InitiativeText"));
	UTextBlock* StatsText = FindTextBlock(WidgetTree, TEXT("StatsText"));
	UTextBlock* IntentText = FindTextBlock(WidgetTree, TEXT("IntentText"));
	UTextBlock* StatusText = FindTextBlock(WidgetTree, TEXT("StatusText"));
	UWidget* DestroyedOverlay = WidgetTree ? WidgetTree->FindWidget(TEXT("DestroyedOverlay")) : nullptr;
	UWidget* RowBox = WidgetTree ? WidgetTree->FindWidget(TEXT("RowBox")) : nullptr;
	UWidget* EntryBackground = WidgetTree ? WidgetTree->FindWidget(TEXT("EntryBackground")) : nullptr;

	if (!TestNotNull(TEXT("PartNameText"), PartNameText)
		|| !TestNotNull(TEXT("HpText"), HpText)
		|| !TestNotNull(TEXT("ShieldText"), ShieldText)
		|| !TestNotNull(TEXT("InitiativeText"), InitiativeText)
		|| !TestNotNull(TEXT("StatsText"), StatsText)
		|| !TestNotNull(TEXT("IntentText"), IntentText)
		|| !TestNotNull(TEXT("StatusText"), StatusText)
		|| !TestNotNull(TEXT("DestroyedOverlay"), DestroyedOverlay)
		|| !TestNotNull(TEXT("RowBox"), RowBox)
		|| !TestNotNull(TEXT("EntryBackground"), EntryBackground))
	{
		return false;
	}

	TestEqual(TEXT("Part name"), PartNameText->GetText().ToString(), FString(TEXT("蛇头")));
	TestEqual(TEXT("HP text"), HpText->GetText().ToString(), FString(TEXT("7/12")));
	TestEqual(TEXT("Shield text"), ShieldText->GetText().ToString(), FString(TEXT("4")));
	TestEqual(TEXT("Initiative text"), InitiativeText->GetText().ToString(), FString(TEXT("3")));
	TestTrue(TEXT("Stats include HP"), StatsText->GetText().ToString().Contains(TEXT("7/12")));
	TestTrue(TEXT("Stats include shield"), StatsText->GetText().ToString().Contains(TEXT("SH 4")));
	TestTrue(TEXT("Stats include initiative"), StatsText->GetText().ToString().Contains(TEXT("INIT 3")));
	TestTrue(TEXT("Intent includes display name"), IntentText->GetText().ToString().Contains(TEXT("撕咬")));
	TestEqual(TEXT("No status fallback is empty"), StatusText->GetText().ToString(), FString());
	TestEqual(TEXT("No status row collapses"), StatusText->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Destroyed overlay starts hidden"), DestroyedOverlay->GetVisibility(), ESlateVisibility::Collapsed);

	Widget->TickFallbackMotionForTest(1.0f);
	View.Shield = 0;
	View.bDestroyed = true;
	Widget->SetPartEntryViewData(View);
	TestEqual(TEXT("Shield text clears at zero"), ShieldText->GetText().ToString(), FString());
	TestEqual(TEXT("Shield text collapses at zero"), ShieldText->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Destroyed overlay becomes visible"), DestroyedOverlay->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("Destroyed part fades entry"), Widget->GetRenderOpacity(), 0.64f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPartEntryActionPreviewSpec,
	"Wacom.UI.Battle.EnemyPanel.PartEntryActionPreviewApplyAndClear",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPartEntryActionPreviewSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyPanelSpec;

	TStrongObjectPtr<UWacomBattleEnemyPartEntryWidget> Widget(NewObject<UWacomBattleEnemyPartEntryWidget>());
	Widget->TakeWidget();

	FWacomBattleEnemyPartEntryViewData BaseView = MakePart(
		TEXT("Head"),
		TEXT("蛇头"),
		7,
		12,
		4,
		3,
		TEXT("撕咬"));
	Widget->SetPartEntryViewData(BaseView);

	FWacomBattleEnemyPartEntryViewData PreviewView = BaseView;
	PreviewView.CurrentHp = 4;
	PreviewView.Shield = 0;
	PreviewView.CurrentInitiative = 0;
	PreviewView.RuntimeStatuses.AddTag(WacomTags::Status_Poison);
	PreviewView.RuntimeStatusStacks.Add(WacomTags::Status_Poison, 2);
	Widget->SetActionPreview(PreviewView);

	UTextBlock* HpText = FindTextBlock(Widget->WidgetTree, TEXT("HpText"));
	UTextBlock* ShieldText = FindTextBlock(Widget->WidgetTree, TEXT("ShieldText"));
	UTextBlock* InitiativeText = FindTextBlock(Widget->WidgetTree, TEXT("InitiativeText"));
	UTextBlock* StatusText = FindTextBlock(Widget->WidgetTree, TEXT("StatusText"));
	if (!TestNotNull(TEXT("HpText"), HpText)
		|| !TestNotNull(TEXT("ShieldText"), ShieldText)
		|| !TestNotNull(TEXT("InitiativeText"), InitiativeText)
		|| !TestNotNull(TEXT("StatusText"), StatusText))
	{
		return false;
	}

	TestTrue(TEXT("Preview flag active"), Widget->HasActionPreview());
	TestEqual(TEXT("Base view remains unchanged"), Widget->GetPartEntryViewData().CurrentHp, 7);
	TestEqual(TEXT("Effective view uses preview HP"), Widget->GetEffectivePartEntryViewData().CurrentHp, 4);
	TestEqual(TEXT("Preview HP text"), HpText->GetText().ToString(), FString(TEXT("4/12")));
	TestEqual(TEXT("Preview shield collapses at zero"), ShieldText->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Preview initiative text"), InitiativeText->GetText().ToString(), FString(TEXT("0")));
	TestTrue(TEXT("Preview status text includes poison"), StatusText->GetText().ToString().Contains(TEXT("中毒")));

	Widget->ClearActionPreview();

	TestFalse(TEXT("Preview flag cleared"), Widget->HasActionPreview());
	TestEqual(TEXT("Effective view returns base HP"), Widget->GetEffectivePartEntryViewData().CurrentHp, 7);
	TestEqual(TEXT("Base HP text restored"), HpText->GetText().ToString(), FString(TEXT("7/12")));
	TestEqual(TEXT("Base shield text restored"), ShieldText->GetText().ToString(), FString(TEXT("4")));
	TestEqual(TEXT("Base initiative text restored"), InitiativeText->GetText().ToString(), FString(TEXT("3")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPartEntryFallbackIntroMotionSpec,
	"Wacom.UI.Battle.EnemyPanel.PartEntryFallbackIntroMotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPartEntryFallbackIntroMotionSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyPanelSpec;

	TStrongObjectPtr<UWacomBattleEnemyPartEntryWidget> Widget(NewObject<UWacomBattleEnemyPartEntryWidget>());
	Widget->SetFallbackIntroDelaySeconds(0.04f);
	Widget->TakeWidget();

	FWacomBattleEnemyPartEntryViewData View = MakePart(
		TEXT("Head"),
		TEXT("蛇头"),
		7,
		12,
		0,
		3,
		TEXT("撕咬"));
	Widget->SetPartEntryViewData(View);

	TestTrue(TEXT("Intro starts transparent"), Widget->GetRenderOpacity() < 0.01f);
	TestTrue(TEXT("Intro starts slightly lowered"), Widget->GetRenderTransform().Translation.Y > 0.0f);
	TestTrue(TEXT("Intro starts slightly scaled down"), Widget->GetRenderTransform().Scale.X < 1.0f);

	Widget->TickFallbackMotionForTest(0.08f);
	TestTrue(TEXT("Intro fades in after delay"), Widget->GetRenderOpacity() > 0.0f);
	TestTrue(TEXT("Intro is still below final position"), Widget->GetRenderTransform().Translation.Y >= 0.0f);

	Widget->TickFallbackMotionForTest(0.24f);
	TestEqual(TEXT("Intro ends fully opaque"), Widget->GetRenderOpacity(), 1.0f);
	TestTrue(TEXT("Intro ends at base position"),
		FMath::IsNearlyEqual(static_cast<float>(Widget->GetRenderTransform().Translation.Y), 0.0f));
	TestTrue(TEXT("Intro ends at base scale"),
		FMath::IsNearlyEqual(static_cast<float>(Widget->GetRenderTransform().Scale.X), 1.0f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPartEntryFallbackPulseMotionSpec,
	"Wacom.UI.Battle.EnemyPanel.PartEntryFallbackPulseMotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPartEntryFallbackPulseMotionSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyPanelSpec;

	TStrongObjectPtr<UWacomBattleEnemyPartEntryWidget> Widget(NewObject<UWacomBattleEnemyPartEntryWidget>());
	Widget->TakeWidget();

	FWacomBattleEnemyPartEntryViewData View = MakePart(
		TEXT("Head"),
		TEXT("蛇头"),
		10,
		12,
		0,
		3,
		TEXT("撕咬"));
	Widget->SetPartEntryViewData(View);
	Widget->TickFallbackMotionForTest(1.0f);

	View.CurrentHp = 6;
	Widget->SetPartEntryViewData(View);
	TestTrue(TEXT("HP decrease pulse scales entry up"), Widget->GetRenderTransform().Scale.X > 1.0f);
	TestEqual(TEXT("HP decrease pulse keeps entry opaque"), Widget->GetRenderOpacity(), 1.0f);

	Widget->TickFallbackMotionForTest(0.30f);
	TestTrue(TEXT("Pulse returns to base scale"),
		FMath::IsNearlyEqual(static_cast<float>(Widget->GetRenderTransform().Scale.X), 1.0f));

	View.Shield = 3;
	Widget->SetPartEntryViewData(View);
	TestTrue(TEXT("Shield change pulse scales entry up"), Widget->GetRenderTransform().Scale.X > 1.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPanelBuildsGroupedFallbackSpec,
	"Wacom.UI.Battle.EnemyPanel.BuildsGroupedFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPanelBuildsGroupedFallbackSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyPanelSpec;

	TStrongObjectPtr<UWacomBattleEnemyPanelWidget> Widget(NewObject<UWacomBattleEnemyPanelWidget>());
	Widget->TakeWidget();

	FWacomBattleEnemyPanelViewData Snake = MakeEnemyView(
		TEXT("Enemy.A"),
		TEXT("林蛇"),
		{
			MakePart(TEXT("Head"), TEXT("蛇头"), 10, 12, 0, 5, TEXT("撕咬")),
			MakePart(TEXT("Tail"), TEXT("蛇尾"), 6, 8, 2, 3, TEXT("扫尾")),
		});
	Snake.EnemyInitiativeSum = 8;

	FWacomBattleEnemyPanelViewData Guard = MakeEnemyView(
		TEXT("Enemy.B"),
		TEXT("守卫"),
		{
			MakePart(TEXT("Body"), TEXT("躯干"), 18, 18, 5, 4, TEXT("格挡")),
		});
	Guard.EnemyInitiativeSum = 4;

	Widget->SetEnemyPanelViewData({ Snake, Guard });

	UVerticalBox* EnemyListBox = FindVerticalBox(Widget->WidgetTree, TEXT("EnemyListBox"));
	if (!TestNotNull(TEXT("EnemyListBox"), EnemyListBox))
	{
		return false;
	}

	TestEqual(TEXT("Two enemies rendered"), EnemyListBox->GetChildrenCount(), 2);

	UVerticalBox* FirstEnemyBox = GetEnemyBoxAt(EnemyListBox, 0);
	UVerticalBox* SecondEnemyBox = GetEnemyBoxAt(EnemyListBox, 1);
	if (!TestNotNull(TEXT("First enemy box"), FirstEnemyBox)
		|| !TestNotNull(TEXT("Second enemy box"), SecondEnemyBox))
	{
		return false;
	}

	TestEqual(TEXT("First enemy has header and two parts"), FirstEnemyBox->GetChildrenCount(), 3);
	TestEqual(TEXT("Second enemy has header and one part"), SecondEnemyBox->GetChildrenCount(), 2);

	const UTextBlock* FirstHeader = Cast<UTextBlock>(FirstEnemyBox->GetChildAt(0));
	const UTextBlock* SecondHeader = Cast<UTextBlock>(SecondEnemyBox->GetChildAt(0));
	if (!TestNotNull(TEXT("First header"), FirstHeader)
		|| !TestNotNull(TEXT("Second header"), SecondHeader))
	{
		return false;
	}

	TestTrue(TEXT("First header includes name"), FirstHeader->GetText().ToString().Contains(TEXT("林蛇")));
	TestTrue(TEXT("First header includes initiative sum"), FirstHeader->GetText().ToString().Contains(TEXT("INIT 8")));
	TestTrue(TEXT("Second header includes name"), SecondHeader->GetText().ToString().Contains(TEXT("守卫")));

	Widget->SetEnemyPanelViewData({});
	TestEqual(TEXT("Empty panel clears enemies"), EnemyListBox->GetChildrenCount(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPanelUsesConfiguredEntryClassAndReusesEntriesSpec,
	"Wacom.UI.Battle.EnemyPanel.UsesConfiguredEntryClassAndReusesEntries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPanelUsesConfiguredEntryClassAndReusesEntriesSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyPanelSpec;

	TStrongObjectPtr<UWacomBattleEnemyPanelWidget> Widget(NewObject<UWacomBattleEnemyPanelWidget>());
	Widget->SetPartEntryWidgetClass(UWacomBattleEnemyPanelSpecTrackingPartEntryWidget::StaticClass());
	Widget->TakeWidget();

	FWacomBattleEnemyPanelViewData Enemy = MakeEnemyView(
		TEXT("Enemy.A"),
		TEXT("林蛇"),
		{
			MakePart(TEXT("Head"), TEXT("蛇头"), 10, 12, 0, 5, TEXT("撕咬")),
			MakePart(TEXT("Tail"), TEXT("蛇尾"), 6, 8, 2, 3, TEXT("扫尾")),
		});

	Widget->SetEnemyPanelViewData({ Enemy });
	UVerticalBox* EnemyListBox = FindVerticalBox(Widget->WidgetTree, TEXT("EnemyListBox"));
	UWacomBattleEnemyPanelSpecTrackingPartEntryWidget* HeadEntry =
		Cast<UWacomBattleEnemyPanelSpecTrackingPartEntryWidget>(GetPartEntryAt(EnemyListBox, 0, 0));
	UWacomBattleEnemyPanelSpecTrackingPartEntryWidget* TailEntry =
		Cast<UWacomBattleEnemyPanelSpecTrackingPartEntryWidget>(GetPartEntryAt(EnemyListBox, 0, 1));

	if (!TestNotNull(TEXT("Configured head entry class"), HeadEntry)
		|| !TestNotNull(TEXT("Configured tail entry class"), TailEntry))
	{
		return false;
	}

	TestEqual(TEXT("Head applied once"), HeadEntry->ApplyCount, 1);
	TestEqual(TEXT("Tail applied once"), TailEntry->ApplyCount, 1);

	Enemy.Parts[0].CurrentHp = 8;
	Enemy.Parts[1].CurrentHp = 4;
	Widget->SetEnemyPanelViewData({ Enemy });

	TestTrue(TEXT("Head entry reused"), GetPartEntryAt(EnemyListBox, 0, 0) == HeadEntry);
	TestTrue(TEXT("Tail entry reused"), GetPartEntryAt(EnemyListBox, 0, 1) == TailEntry);
	TestEqual(TEXT("Head reapplied"), HeadEntry->ApplyCount, 2);
	TestEqual(TEXT("Head HP updated"), HeadEntry->LastAppliedView.CurrentHp, 8);

	Enemy.Parts.RemoveAt(1);
	Enemy.Parts.Add(MakePart(TEXT("Body"), TEXT("蛇身"), 11, 14, 0, 2, TEXT("盘绕")));
	Enemy.Parts.Last().EnemySlotId = TEXT("Enemy.A");
	Enemy.Parts.Last().Identity = FBattlePartSlotIdentity::Make(TEXT("Encounter"), TEXT("Enemy.A"), TEXT("Body"));
	Widget->SetEnemyPanelViewData({ Enemy });

	TestTrue(TEXT("Head still reused after part list change"), GetPartEntryAt(EnemyListBox, 0, 0) == HeadEntry);
	TestFalse(TEXT("New body entry is not removed tail entry"), GetPartEntryAt(EnemyListBox, 0, 1) == TailEntry);
	TestEqual(TEXT("One enemy group remains"), EnemyListBox->GetChildrenCount(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyHostInitializesEnemyPanelWidgetBeforeApplyingDataSpec,
	"Wacom.UI.Battle.EnemyPanel.HostInitializesWidgetBeforeApplyingData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyHostInitializesEnemyPanelWidgetBeforeApplyingDataSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyPanelSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("World"), World))
	{
		return false;
	}

	AWacomBattleEnemyActor* Host = World->SpawnActor<AWacomBattleEnemyActor>();
	if (!TestNotNull(TEXT("Host"), Host))
	{
		return false;
	}

	FWacomBattleEnemyPanelViewData Enemy = MakeEnemyView(
		TEXT("Enemy"),
		TEXT("林蛇"),
		{
			MakePart(TEXT("Head"), TEXT("蛇头"), 10, 12, 0, 5, TEXT("撕咬")),
		});

	Host->SetEnemyPanelViewData(Enemy);

	UWidgetComponent* PanelComponent = Cast<UWidgetComponent>(
		Host->GetDefaultSubobjectByName(TEXT("EnemyPanelWidget")));
	if (!TestNotNull(TEXT("EnemyPanelWidget component"), PanelComponent))
	{
		return false;
	}

	UWacomBattleEnemyPanelWidget* PanelWidget =
		Cast<UWacomBattleEnemyPanelWidget>(PanelComponent->GetUserWidgetObject());
	if (!TestNotNull(TEXT("Enemy panel user widget initialized"), PanelWidget))
	{
		return false;
	}

	UVerticalBox* EnemyListBox = FindVerticalBox(PanelWidget->WidgetTree, TEXT("EnemyListBox"));
	TestNotNull(TEXT("EnemyListBox initialized"), EnemyListBox);
	TestEqual(TEXT("Panel has one enemy after host data push"), EnemyListBox ? EnemyListBox->GetChildrenCount() : 0, 1);

	return true;
}
