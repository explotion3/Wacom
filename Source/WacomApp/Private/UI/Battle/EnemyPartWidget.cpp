// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/EnemyPartWidget.h"

#define LOCTEXT_NAMESPACE "WacomEnemyPart"
#include "UI/Common/WacomProgressBar.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Engine/World.h"

namespace
{
	constexpr float BattlePresentationCueHoldSeconds = 0.16f;
}

TSharedRef<SWidget> UEnemyPartWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		USizeBox* Root = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Root"));
		Root->SetWidthOverride(220.0f);
		Root->SetHeightOverride(120.0f);
		WidgetTree->RootWidget = Root;

		UOverlay* Stack = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Stack"));
		Root->AddChild(Stack);

		// Frame
		FrameBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FrameBorder"));
		FrameBorder->SetBrushColor(FLinearColor(0.2f, 0.05f, 0.05f, 0.9f));
		FrameBorder->SetPadding(FMargin(6));
		if (UOverlaySlot* BS = Stack->AddChildToOverlay(FrameBorder))
		{
			BS->SetHorizontalAlignment(HAlign_Fill);
			BS->SetVerticalAlignment(VAlign_Fill);
		}

		UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Content"));
		FrameBorder->SetContent(Content);

		NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NameText"));
		NameText->SetText(FText::FromString(TEXT("PartName")));
		NameText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Content->AddChildToVerticalBox(NameText);

		// HP
		USizeBox* HpBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HpBox"));
		HpBox->SetHeightOverride(18.0f);
		HpBar = WidgetTree->ConstructWidget<UWacomProgressBar>(UWacomProgressBar::StaticClass(), TEXT("HpBar"));
		HpBox->AddChild(HpBar);
		Content->AddChildToVerticalBox(HpBox);

		InitiativeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InitiativeText"));
		InitiativeText->SetText(FText::FromString(TEXT("Init 0")));
		InitiativeText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.85f, 0.3f)));
		Content->AddChildToVerticalBox(InitiativeText);

		IntentText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("IntentText"));
		IntentText->SetText(FText::FromString(TEXT("Intent --")));
		Content->AddChildToVerticalBox(IntentText);

		ShieldText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ShieldText"));
		ShieldText->SetText(FText::FromString(TEXT("Shield 0")));
		ShieldText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.9f, 1.0f)));
		Content->AddChildToVerticalBox(ShieldText);

		StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
		StatusText->SetText(FText::FromString(TEXT("")));
		StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.6f, 0.9f)));
		Content->AddChildToVerticalBox(StatusText);

		// Button 覆盖在最上层
		RootButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RootButton"));
		FButtonStyle Bs = RootButton->GetStyle();
		Bs.Normal.TintColor   = FSlateColor(FLinearColor(1, 1, 1, 0));
		Bs.Hovered.TintColor  = FSlateColor(FLinearColor(1, 1, 1, 0.15f));
		Bs.Pressed.TintColor  = FSlateColor(FLinearColor(1, 1, 1, 0.3f));
		Bs.Disabled.TintColor = FSlateColor(FLinearColor(1, 1, 1, 0));
		RootButton->SetStyle(Bs);
		if (UOverlaySlot* BS = Stack->AddChildToOverlay(RootButton))
		{
			BS->SetHorizontalAlignment(HAlign_Fill);
			BS->SetVerticalAlignment(VAlign_Fill);
		}
	}
	return Super::RebuildWidget();
}

void UEnemyPartWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// Button 绑定移到 NativeConstruct。
}

void UEnemyPartWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (RootButton && !RootButton->OnClicked.IsBound())
	{
		RootButton->OnClicked.AddDynamic(this, &UEnemyPartWidget::HandleRootButtonClicked);
	}
}

void UEnemyPartWidget::NativeDestruct()
{
	StopBattlePresentationCueTimer();
	bBattlePresentationCueActive = false;
	Super::NativeDestruct();
}

void UEnemyPartWidget::ApplyPartSnapshot(const FEnemyPartSnapshot& InSnap)
{
	CachedSnap = InSnap;

	if (NameText && InSnap.Definition)
	{
		const FText Display = InSnap.Definition->DisplayName.IsEmpty()
			? FText::FromName(InSnap.Definition->PartId)
			: InSnap.Definition->DisplayName;
		NameText->SetText(Display);
	}

	if (HpBar)
	{
		HpBar->SetValue(InSnap.CurrentHp, InSnap.MaxHp);
	}

	if (InitiativeText)
	{
		InitiativeText->SetText(FText::Format(
			LOCTEXT("InitFmt", "先机 {0}"), FFormatOrderedArguments{ FFormatArgumentValue(InSnap.CurrentInitiative) }));
	}

	if (IntentText)
	{
		if (InSnap.bDestroyed)
		{
			IntentText->SetText(LOCTEXT("Destroyed", "(已破坏)"));
		}
		else
		{
			FString IntentName = InSnap.CurrentIntent.DisplayName.ToString();
			if (IntentName.IsEmpty()) { IntentName = TEXT("--"); }
			IntentText->SetText(FText::Format(LOCTEXT("IntentFmt", "意图: {0}"), FText::FromString(IntentName)));
		}
	}

	if (ShieldText)
	{
		if (bHideShieldWhenZero && InSnap.Shield <= 0)
		{
			ShieldText->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			ShieldText->SetVisibility(ESlateVisibility::HitTestInvisible);
			ShieldText->SetText(FText::Format(
				LOCTEXT("ShieldFmt", "护盾 {0}"), FFormatOrderedArguments{ FFormatArgumentValue(InSnap.Shield) }));
		}
	}

	if (StatusText)
	{
		StatusText->SetText(FText::FromString(InSnap.Statuses.ToStringSimple()));
	}

	if (RootButton)
	{
		// 破坏部位不能作为目标
		RootButton->SetIsEnabled(!InSnap.bDestroyed && bLastTargetable);
	}

	if (bLastDestroyed != InSnap.bDestroyed)
	{
		bLastDestroyed = InSnap.bDestroyed;
		BP_OnDestroyedChanged(InSnap.bDestroyed);
	}

	UpdateFrameColor();
	BP_OnDataApplied(InSnap);
}

void UEnemyPartWidget::SetTargetable(bool bInTargetable)
{
	if (bLastTargetable == bInTargetable) { return; }
	bLastTargetable = bInTargetable;
	if (RootButton)
	{
		RootButton->SetIsEnabled(bInTargetable && !CachedSnap.bDestroyed);
	}
	UpdateFrameColor();
	BP_OnTargetableChanged(bInTargetable);
}

void UEnemyPartWidget::PlayBattlePresentationCue(const FWacomBattlePresentationTargetCue& Cue)
{
	if (Cue.CueKind == EWacomBattlePresentationTargetCueKind::BattleEvent
		&& Cue.SourceEventType != EBattleEventType::DamageDealt
		&& Cue.SourceEventType != EBattleEventType::EnemyPartHpEmptied)
	{
		return;
	}

	StopBattlePresentationCueTimer();
	bBattlePresentationCueActive = true;
	LastBattlePresentationCueKind = Cue.CueKind;
	LastBattlePresentationCueType = Cue.SourceEventType;
	LastBattlePresentationCueAmount = Cue.Amount;
	++BattlePresentationCuePlayCount;

	if (FrameBorder)
	{
		FrameBorder->SetBrushColor(BuildPresentationCueFrameColor(Cue));
	}

	if (UWorld* World = GetWorld())
	{
		const float HoldSeconds = Cue.Duration > 0.0f
			? Cue.Duration
			: BattlePresentationCueHoldSeconds;
		World->GetTimerManager().SetTimer(
			BattlePresentationCueTimerHandle,
			this,
			&UEnemyPartWidget::ClearBattlePresentationCue,
			HoldSeconds,
			false);
	}
}

void UEnemyPartWidget::ClearBattlePresentationCue()
{
	StopBattlePresentationCueTimer();
	bBattlePresentationCueActive = false;
	UpdateFrameColor();
}

void UEnemyPartWidget::StopBattlePresentationCueTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BattlePresentationCueTimerHandle);
	}
	BattlePresentationCueTimerHandle = FTimerHandle();
}

FLinearColor UEnemyPartWidget::BuildBaseFrameColor() const
{
	if (CachedSnap.bDestroyed)
	{
		return FLinearColor(0.08f, 0.08f, 0.08f, 0.9f);
	}
	if (bLastTargetable)
	{
		return FLinearColor(0.9f, 0.7f, 0.1f, 0.95f); // 黄色：可选目标
	}
	return FLinearColor(0.2f, 0.05f, 0.05f, 0.9f); // 暗红：默认
}

FLinearColor UEnemyPartWidget::BuildPresentationCueFrameColor(
	const FWacomBattlePresentationTargetCue& Cue) const
{
	if (Cue.CueKind == EWacomBattlePresentationTargetCueKind::TargetConfirmed)
	{
		return FLinearColor(0.75f, 1.0f, 0.55f, 1.0f);
	}
	if (Cue.SourceEventType == EBattleEventType::EnemyPartHpEmptied)
	{
		return FLinearColor(1.0f, 0.35f, 0.12f, 1.0f);
	}
	return FLinearColor(1.0f, 0.88f, 0.32f, 1.0f);
}

void UEnemyPartWidget::UpdateFrameColor()
{
	if (!FrameBorder) { return; }
	if (bBattlePresentationCueActive)
	{
		FWacomBattlePresentationTargetCue Cue;
		Cue.CueKind = LastBattlePresentationCueKind;
		Cue.SourceEventType = LastBattlePresentationCueType;
		Cue.Amount = LastBattlePresentationCueAmount;
		FrameBorder->SetBrushColor(BuildPresentationCueFrameColor(Cue));
		return;
	}
	FrameBorder->SetBrushColor(BuildBaseFrameColor());
}

void UEnemyPartWidget::HandleRootButtonClicked()
{
	OnPartClicked.Broadcast(CachedSnap.InstanceId);
}

#undef LOCTEXT_NAMESPACE

