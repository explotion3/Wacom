// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/PlayerStatusBar.h"

#define LOCTEXT_NAMESPACE "WacomPlayerStatus"
#include "UI/Common/WacomProgressBar.h"
#include "UI/Battle/WacomBattleStatusIconWidget.h"

#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Blueprint/WidgetTree.h"
#include "Animation/WidgetAnimation.h"
#include "Kismet/GameplayStatics.h"
#include "Snapshots/BattleSnapshot.h"

namespace
{
	bool AreStatusStacksEquivalent(
		const TMap<FGameplayTag, int32>& Left,
		const TMap<FGameplayTag, int32>& Right)
	{
		if (Left.Num() != Right.Num())
		{
			return false;
		}

		for (const TPair<FGameplayTag, int32>& Pair : Left)
		{
			const int32* RightValue = Right.Find(Pair.Key);
			if (!RightValue || *RightValue != Pair.Value)
			{
				return false;
			}
		}
		return true;
	}

	bool ArePlayerSnapshotsEquivalent(
		const FPlayerSnapshot& Left,
		const FPlayerSnapshot& Right)
	{
		return Left.CurrentHp == Right.CurrentHp
			&& Left.MaxHp == Right.MaxHp
			&& Left.Shield == Right.Shield
			&& Left.Statuses.Num() == Right.Statuses.Num()
			&& Left.Statuses.HasAllExact(Right.Statuses)
			&& Right.Statuses.HasAllExact(Left.Statuses)
			&& AreStatusStacksEquivalent(Left.StatusStacks, Right.StatusStacks);
	}
}

TSharedRef<SWidget> UPlayerStatusBar::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		// HP
		USizeBox* HpBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HpBox"));
		HpBox->SetWidthOverride(200.0f);
		HpBox->SetHeightOverride(22.0f);
		HpBar = WidgetTree->ConstructWidget<UWacomProgressBar>(UWacomProgressBar::StaticClass(), TEXT("HpBar"));
		HpBox->AddChild(HpBar);
		Root->AddChildToVerticalBox(HpBox);

		// Shield
		ShieldText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ShieldText"));
		ShieldText->SetText(LOCTEXT("ShieldDefault", "护盾 0"));
		ShieldText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.9f, 1.0f)));
		Root->AddChildToVerticalBox(ShieldText);

		StatusList = WidgetTree->ConstructWidget<UWacomBattleStatusIconListWidget>(
			UWacomBattleStatusIconListWidget::StaticClass(),
			TEXT("StatusList"));
		if (UVerticalBoxSlot* StatusSlot = Root->AddChildToVerticalBox(StatusList))
		{
			StatusSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
			StatusSlot->SetHorizontalAlignment(HAlign_Left);
		}
	}
	return Super::RebuildWidget();
}

void UPlayerStatusBar::NativeRefreshFromSnapshot(const FBattleSnapshot& Snap)
{
	BasePlayerView = Snap.Player;
	bHasBasePlayerView = true;
	RefreshDisplay();
}

void UPlayerStatusBar::SetActionPreview(const FPlayerSnapshot& ProjectedPlayer)
{
	if (bHasActionPreview && ArePlayerSnapshotsEquivalent(ActionPreviewPlayerView, ProjectedPlayer))
	{
		return;
	}

	ActionPreviewPlayerView = ProjectedPlayer;
	bHasActionPreview = true;
	RefreshDisplay();
}

void UPlayerStatusBar::ClearActionPreview()
{
	if (!bHasActionPreview)
	{
		return;
	}

	bHasActionPreview = false;
	RefreshDisplay();
}

void UPlayerStatusBar::PlayEnemyActionImpactFeedback(
	const FPlayerSnapshot& PreviousPlayer,
	const FPlayerSnapshot& CurrentPlayer)
{
	const bool bHpLost = CurrentPlayer.CurrentHp < PreviousPlayer.CurrentHp;
	const bool bShieldLost = CurrentPlayer.Shield < PreviousPlayer.Shield;

	if (bHpLost && DamagePulseAnimation)
	{
		StopAnimation(DamagePulseAnimation);
		PlayAnimation(DamagePulseAnimation);
	}
	if (bShieldLost && ShieldPulseAnimation)
	{
		StopAnimation(ShieldPulseAnimation);
		PlayAnimation(ShieldPulseAnimation);
	}

	USoundBase* ImpactSound = bHpLost ? DamageImpactSound.Get()
		: (bShieldLost ? ShieldImpactSound.Get() : nullptr);
	if (ImpactSound)
	{
		UGameplayStatics::PlaySound2D(this, ImpactSound, FMath::Max(0.0f, ImpactSoundVolume));
	}
}

void UPlayerStatusBar::NativeDestruct()
{
	StopAllAnimations();
	bHasActionPreview = false;
	Super::NativeDestruct();
}

void UPlayerStatusBar::RefreshDisplay()
{
	if (!bCapturedBaseRenderOpacity)
	{
		BaseRenderOpacity = GetRenderOpacity();
		bCapturedBaseRenderOpacity = true;
	}

	SetRenderOpacity(bHasActionPreview
		? ActionPreviewRenderOpacity
		: BaseRenderOpacity);

	if (bHasActionPreview)
	{
		RefreshFromPlayerSnapshot(ActionPreviewPlayerView);
		return;
	}

	if (bHasBasePlayerView)
	{
		RefreshFromPlayerSnapshot(BasePlayerView);
	}
}

void UPlayerStatusBar::RefreshFromPlayerSnapshot(const FPlayerSnapshot& PlayerView)
{
	if (HpBar)
	{
		HpBar->SetValue(PlayerView.CurrentHp, PlayerView.MaxHp);
	}

	if (ShieldText)
	{
		if (bHideShieldWhenZero && PlayerView.Shield <= 0)
		{
			ShieldText->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			ShieldText->SetVisibility(ESlateVisibility::HitTestInvisible);
			ShieldText->SetText(FText::Format(
				LOCTEXT("ShieldFmt", "护盾 {0}"), FFormatOrderedArguments{ FFormatArgumentValue(PlayerView.Shield) }));
		}
	}

	if (UWacomBattleStatusIconListWidget* ResolvedStatusList = ResolveStatusListWidget())
	{
		ResolvedStatusList->SetStatuses(PlayerView.Statuses, PlayerView.StatusStacks);
	}
}

UWacomBattleStatusIconListWidget* UPlayerStatusBar::ResolveStatusListWidget()
{
	if (StatusList)
	{
		return StatusList;
	}

	if (!WidgetTree)
	{
		return nullptr;
	}

	StatusList = Cast<UWacomBattleStatusIconListWidget>(WidgetTree->FindWidget(TEXT("StatusList")));
	if (StatusList)
	{
		return StatusList;
	}

	UWacomBattleStatusIconListWidget* UniqueStatusList = nullptr;
	bool bFoundMultipleStatusLists = false;
	WidgetTree->ForEachWidget([&UniqueStatusList, &bFoundMultipleStatusLists](UWidget* Widget)
	{
		UWacomBattleStatusIconListWidget* Candidate = Cast<UWacomBattleStatusIconListWidget>(Widget);
		if (!Candidate)
		{
			return;
		}

		if (!UniqueStatusList)
		{
			UniqueStatusList = Candidate;
			return;
		}

		bFoundMultipleStatusLists = true;
	});

	if (!bFoundMultipleStatusLists)
	{
		StatusList = UniqueStatusList;
	}
	return StatusList;
}

#undef LOCTEXT_NAMESPACE
