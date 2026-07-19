// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/BattleCombatLogFeedWidget.h"

#include "UI/Battle/BattleCombatActivityRowWidget.h"
#include "UI/Battle/WacomBattleCombatActivityPlayback.h"
#include "UI/Battle/WacomBattleCombatActivityStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Settings/WacomSettingsSubsystem.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "WacomBattleCombatActivity"

namespace
{
	FWacomBattleCombatActivityPlaybackConfig BuildPlaybackConfig(
		const UWacomBattleCombatActivityStyle* Style,
		bool bReducedMotion)
	{
		const UWacomBattleCombatActivityStyle* Resolved = Style
			? Style
			: GetDefault<UWacomBattleCombatActivityStyle>();
		FWacomBattleCombatActivityPlaybackConfig Config;
		Config.MaxVisibleRows = Resolved->MaxVisibleRows;
		Config.EnterSeconds = Resolved->EnterSeconds;
		Config.ResultStaggerSeconds = Resolved->ResultStaggerSeconds;
		Config.MinimumReadableSeconds = Resolved->MinimumReadableSeconds;
		Config.ShiftSeconds = Resolved->ShiftSeconds;
		Config.EmptyHoldSeconds = Resolved->EmptyHoldSeconds;
		Config.CollapseSeconds = Resolved->CollapseSeconds;
		Config.bReducedMotion = bReducedMotion;
		Config.Normalize();
		return Config;
	}

}

UBattleCombatLogFeedWidget::UBattleCombatLogFeedWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UBattleCombatLogFeedWidget::~UBattleCombatLogFeedWidget()
{
	delete Playback;
	Playback = nullptr;
}

TSharedRef<SWidget> UBattleCombatLogFeedWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}
		UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Frame"));
		Frame->SetBrushColor(FLinearColor(0.02f, 0.028f, 0.045f, 0.50f));
		Frame->SetPadding(FMargin(7.0f));
		Frame->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		WidgetTree->RootWidget = Frame;

		UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
		Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Frame->SetContent(Root);

		ActivityRowsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ActivityRowsBox"));
		ActivityRowsBox->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UVerticalBoxSlot* RowsSlot = Root->AddChildToVerticalBox(ActivityRowsBox))
		{
			RowsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		UHorizontalBox* Footer = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Footer"));
		Footer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Root->AddChildToVerticalBox(Footer);

		LastActionButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("LastActionButton"));
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		LastActionButton->IsFocusable = false;
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		LastActionIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("LastActionIcon"));
		LastActionIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		LastActionButton->SetContent(LastActionIcon);
		if (UHorizontalBoxSlot* ActionSlot = Footer->AddChildToHorizontalBox(LastActionButton))
		{
			ActionSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
		}

		UHorizontalBox* RuntimeTurnRoot = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TurnRoot"));
		RuntimeTurnRoot->SetVisibility(ESlateVisibility::HitTestInvisible);
		TurnRoot = RuntimeTurnRoot;
		Footer->AddChildToHorizontalBox(RuntimeTurnRoot);
		TurnIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TurnIcon"));
		TurnIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		RuntimeTurnRoot->AddChildToHorizontalBox(TurnIcon);
		TurnText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TurnText"));
		TurnText->SetText(FText::AsNumber(1));
		RuntimeTurnRoot->AddChildToHorizontalBox(TurnText);
	}
	return Super::RebuildWidget();
}

void UBattleCombatLogFeedWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!Playback)
	{
		Playback = new FWacomBattleCombatActivityPlayback();
	}
	EnsureRuntimeBindings();
	EnsureRowWidgets();
	if (LastActionButton)
	{
		LastActionButton->OnClicked.RemoveAll(this);
		LastActionButton->OnClicked.AddDynamic(this, &UBattleCombatLogFeedWidget::HandleLastActionClicked);
	}
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	BindRuntimeSettings();
	RefreshPlaybackPresentation();
}

void UBattleCombatLogFeedWidget::NativeDestruct()
{
	if (LastActionButton)
	{
		LastActionButton->OnClicked.RemoveAll(this);
	}
	UnbindRuntimeSettings();
	if (Playback)
	{
		Playback->Reset();
	}
	ActivityRowWidgets.Reset();
	Super::NativeDestruct();
}

void UBattleCombatLogFeedWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!Playback || !Playback->IsTickRequired())
	{
		return;
	}
	Playback->Tick(InDeltaTime, BuildPlaybackConfig(ActivityStyle, bRuntimeSimplifiedMotion));
	RefreshPlaybackPresentation();
}

void UBattleCombatLogFeedWidget::NativeOnSessionChanged(UBattleSession* OldSession, UBattleSession* NewSession)
{
	Super::NativeOnSessionChanged(OldSession, NewSession);
	if (OldSession != NewSession)
	{
		ClearCombatActivity();
	}
}

void UBattleCombatLogFeedWidget::EnqueueCombatActivityBatch(
	const FWacomBattleCombatActivityBatchView& Batch)
{
	if (!Playback)
	{
		Playback = new FWacomBattleCombatActivityPlayback();
	}
	Playback->Enqueue(Batch);
	Playback->Tick(0.0f, BuildPlaybackConfig(ActivityStyle, bRuntimeSimplifiedMotion));
	RefreshPlaybackPresentation();
}

void UBattleCombatLogFeedWidget::SetPresentedTurnNumber(int32 TurnNumber)
{
	if (!Playback)
	{
		Playback = new FWacomBattleCombatActivityPlayback();
	}
	Playback->SetPresentedTurnNumber(TurnNumber);
	RefreshFooter();
}

void UBattleCombatLogFeedWidget::ClearCombatActivity()
{
	if (Playback)
	{
		Playback->Reset();
	}
	for (UBattleCombatActivityRowWidget* RowWidget : ActivityRowWidgets)
	{
		if (RowWidget)
		{
			RowWidget->ClearActivityRow();
		}
	}
	RefreshFooter();
}

void UBattleCombatLogFeedWidget::RestorePersistentState(
	int32 TurnNumber,
	const FWacomBattleCombatActivityRowView* LastRootAction)
{
	if (!Playback)
	{
		Playback = new FWacomBattleCombatActivityPlayback();
	}
	Playback->SetPresentedTurnNumber(TurnNumber);
	if (LastRootAction)
	{
		Playback->SetLastRootAction(*LastRootAction);
	}
	RefreshPlaybackPresentation();
}

int32 UBattleCombatLogFeedWidget::GetVisibleActivityRowCount() const
{
	return Playback ? Playback->GetVisibleRows().Num() : 0;
}

int32 UBattleCombatLogFeedWidget::GetPresentedTurnNumber() const
{
	return Playback ? Playback->GetPresentedTurnNumber() : 0;
}

void UBattleCombatLogFeedWidget::HandleLastActionClicked()
{
	if (Playback && Playback->GetLastRootAction())
	{
		CombatLogDetailsRequestedNative.Broadcast();
	}
}

void UBattleCombatLogFeedWidget::EnsureRuntimeBindings()
{
	if (!WidgetTree)
	{
		return;
	}
	if (!ActivityRowsBox)
	{
		ActivityRowsBox = Cast<UPanelWidget>(WidgetTree->FindWidget(TEXT("ActivityRowsBox")));
	}

	if (!LastActionButton)
	{
		LastActionButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("LastActionButton")));
	}
	if (!LastActionIcon)
	{
		LastActionIcon = Cast<UImage>(WidgetTree->FindWidget(TEXT("LastActionIcon")));
	}
	if (!TurnRoot)
	{
		TurnRoot = WidgetTree->FindWidget(TEXT("TurnRoot"));
	}
	if (!TurnIcon)
	{
		TurnIcon = Cast<UImage>(WidgetTree->FindWidget(TEXT("TurnIcon")));
	}
	if (!TurnText)
	{
		TurnText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("TurnText")));
	}

}

void UBattleCombatLogFeedWidget::EnsureRowWidgets()
{
	if (!ActivityRowsBox)
	{
		return;
	}
	const FWacomBattleCombatActivityPlaybackConfig Config = BuildPlaybackConfig(ActivityStyle, bRuntimeSimplifiedMotion);
	if (ActivityRowWidgets.Num() == Config.MaxVisibleRows)
	{
		return;
	}
	ActivityRowsBox->ClearChildren();
	ActivityRowWidgets.Reset();
	UClass* RowClass = ActivityRowWidgetClass
		? ActivityRowWidgetClass.Get()
		: UBattleCombatActivityRowWidget::StaticClass();
	for (int32 Index = 0; Index < Config.MaxVisibleRows; ++Index)
	{
		UBattleCombatActivityRowWidget* RowWidget = GetWorld()
			? CreateWidget<UBattleCombatActivityRowWidget>(this, RowClass)
			: NewObject<UBattleCombatActivityRowWidget>(this, RowClass);
		if (!RowWidget)
		{
			continue;
		}
		RowWidget->SetVisibility(ESlateVisibility::Collapsed);
		ActivityRowsBox->AddChild(RowWidget);
		ActivityRowWidgets.Add(RowWidget);
	}
}

void UBattleCombatLogFeedWidget::RefreshPlaybackPresentation()
{
	EnsureRuntimeBindings();
	EnsureRowWidgets();
	const TArray<FWacomBattleCombatActivityRowPlaybackView>* Views = Playback
		? &Playback->GetVisibleRows()
		: nullptr;
	for (int32 Index = 0; Index < ActivityRowWidgets.Num(); ++Index)
	{
		UBattleCombatActivityRowWidget* RowWidget = ActivityRowWidgets[Index];
		if (!RowWidget)
		{
			continue;
		}
		if (!Views || !Views->IsValidIndex(Index))
		{
			RowWidget->ClearActivityRow();
			continue;
		}
		const FWacomBattleCombatActivityRowPlaybackView& View = (*Views)[Index];
		RowWidget->SetActivityRowData(View.Row, ResolveActivityIconBrush(View.Row));
		RowWidget->SetPlaybackPresentation(View.Opacity, View.TranslationY);
	}
	RefreshFooter();
}

void UBattleCombatLogFeedWidget::RefreshFooter()
{
	const FWacomBattleCombatActivityRowView* LastRoot = Playback ? Playback->GetLastRootAction() : nullptr;
	if (LastActionButton)
	{
		LastActionButton->SetVisibility(LastRoot ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (LastActionIcon && LastRoot)
	{
		LastActionIcon->SetBrush(ResolveActivityIconBrush(*LastRoot));
	}
	const int32 TurnNumber = Playback ? Playback->GetPresentedTurnNumber() : 0;
	if (TurnRoot)
	{
		TurnRoot->SetVisibility(TurnNumber > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (TurnText)
	{
		TurnText->SetText(FText::AsNumber(FMath::Max(1, TurnNumber)));
	}
	if (TurnIcon && ActivityStyle)
	{
		TurnIcon->SetBrush(ActivityStyle->TurnIconBrush);
	}
}

void UBattleCombatLogFeedWidget::BindRuntimeSettings()
{
	UWacomSettingsSubsystem* Settings = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UWacomSettingsSubsystem>()
		: nullptr;
	if (BoundSettingsSubsystem.Get() == Settings && RuntimeSettingsChangedHandle.IsValid())
	{
		HandleRuntimeSettingsChanged(Settings->GetCurrentSnapshot(), EWacomRuntimeSettingsChangeReason::Startup);
		return;
	}
	UnbindRuntimeSettings();
	if (!Settings)
	{
		return;
	}
	BoundSettingsSubsystem = Settings;
	RuntimeSettingsChangedHandle = Settings->OnRuntimeSettingsChangedNative().AddUObject(
		this,
		&UBattleCombatLogFeedWidget::HandleRuntimeSettingsChanged);
	HandleRuntimeSettingsChanged(Settings->GetCurrentSnapshot(), EWacomRuntimeSettingsChangeReason::Startup);
}

void UBattleCombatLogFeedWidget::UnbindRuntimeSettings()
{
	if (UWacomSettingsSubsystem* Settings = BoundSettingsSubsystem.Get())
	{
		if (RuntimeSettingsChangedHandle.IsValid())
		{
			Settings->OnRuntimeSettingsChangedNative().Remove(RuntimeSettingsChangedHandle);
		}
	}
	BoundSettingsSubsystem.Reset();
	RuntimeSettingsChangedHandle.Reset();
}

void UBattleCombatLogFeedWidget::HandleRuntimeSettingsChanged(
	const FWacomLocalSettingsSnapshot& Snapshot,
	EWacomRuntimeSettingsChangeReason /*Reason*/)
{
	bRuntimeSimplifiedMotion = Snapshot.UIMotionMode == EWacomUIMotionMode::Simplified;
	RefreshPlaybackPresentation();
}

FSlateBrush UBattleCombatLogFeedWidget::ResolveActivityIconBrush(
	const FWacomBattleCombatActivityRowView& Row) const
{
	const UWacomBattleCombatActivityStyle* Style = ActivityStyle;
	if (Style)
	{
		return Style->ResolveActivityIconBrush(Row);
	}
	return *FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
}

#if WITH_AUTOMATION_TESTS
void UBattleCombatLogFeedWidget::AdvanceActivityPlaybackForTest(float DeltaTime)
{
	if (Playback)
	{
		Playback->Tick(DeltaTime, BuildPlaybackConfig(ActivityStyle, bRuntimeSimplifiedMotion));
		RefreshPlaybackPresentation();
	}
}

TArray<FWacomBattleCombatActivityRowView> UBattleCombatLogFeedWidget::GetVisibleActivityRowsForTest() const
{
	TArray<FWacomBattleCombatActivityRowView> Rows;
	if (Playback)
	{
		for (const FWacomBattleCombatActivityRowPlaybackView& View : Playback->GetVisibleRows())
		{
			Rows.Add(View.Row);
		}
	}
	return Rows;
}

const FWacomBattleCombatActivityRowView* UBattleCombatLogFeedWidget::GetLastRootActionForTest() const
{
	return Playback ? Playback->GetLastRootAction() : nullptr;
}

bool UBattleCombatLogFeedWidget::IsPlaybackPendingForTest() const
{
	return Playback && Playback->HasPendingPlayback();
}
#endif

#undef LOCTEXT_NAMESPACE
