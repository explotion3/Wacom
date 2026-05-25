// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/EnemyInfoBar.h"
#include "UI/Battle/EnemyPartWidget.h"
#include "UI/Battle/BattleHUD.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Snapshots/BattleSnapshot.h"

UEnemyInfoBar::UEnemyInfoBar()
{
	PartWidgetClass = UEnemyPartWidget::StaticClass();
}

TSharedRef<SWidget> UEnemyInfoBar::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}
		UHorizontalBox* Root = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;
		PartsContainer = Root;
	}
	return Super::RebuildWidget();
}

void UEnemyInfoBar::NativeRefreshFromSnapshot(const FBattleSnapshot& Snap)
{
	if (!PartsContainer) { return; }

	PartsContainer->ClearChildren();
	SpawnedParts.Reset();

	TSubclassOf<UEnemyPartWidget> ClassToUse = PartWidgetClass;
	if (!ClassToUse) { ClassToUse = UEnemyPartWidget::StaticClass(); }

	for (const FEnemyPartSnapshot& Part : Snap.Enemy.Parts)
	{
		UEnemyPartWidget* W = CreateWidget<UEnemyPartWidget>(this, ClassToUse);
		if (!W) { continue; }

		if (UHorizontalBoxSlot* HS = Cast<UHorizontalBoxSlot>(PartsContainer->AddChild(W)))
		{
			HS->SetPadding(FMargin(4));
		}
		W->ApplyPartSnapshot(Part);
		W->OnPartClicked.AddDynamic(this, &UEnemyInfoBar::HandlePartClicked);
		SpawnedParts.Add(W);
	}

	ApplyTargetableFromHUDState();
}

void UEnemyInfoBar::ApplyTargetableFromHUDState()
{
	UBattleHUD* HUD = nullptr;
	for (UUserWidget* P = GetTypedOuter<UUserWidget>(); P; P = P->GetTypedOuter<UUserWidget>())
	{
		HUD = Cast<UBattleHUD>(P);
		if (HUD) { break; }
	}

	TMap<FGuid, bool> TargetableByPartId;
	if (HUD)
	{
		const FBattleTargetSelectionView TargetView = HUD->BuildTargetSelectionView();
		TargetableByPartId.Reserve(TargetView.TargetableParts.Num());
		for (const FBattleTargetablePartView& PartView : TargetView.TargetableParts)
		{
			TargetableByPartId.Add(PartView.PartInstanceId, PartView.bTargetable);
		}
	}

	for (const TObjectPtr<UEnemyPartWidget>& Part : SpawnedParts)
	{
		if (!Part) { continue; }
		const bool* bTargetable = TargetableByPartId.Find(Part->GetPartInstanceId());
		Part->SetTargetable(bTargetable ? *bTargetable : false);
	}
}

void UEnemyInfoBar::HandlePartClicked(FGuid PartInstanceId)
{
	for (UUserWidget* P = GetTypedOuter<UUserWidget>(); P; P = P->GetTypedOuter<UUserWidget>())
	{
		if (UBattleHUD* HUD = Cast<UBattleHUD>(P))
		{
			HUD->OnEnemyPartClickedByUser(PartInstanceId);
			return;
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("[EnemyInfoBar] 未找到 UBattleHUD 父 Widget"));
}

void UEnemyInfoBar::PlayBattlePresentationCue(
	EBattleEventType SourceEventType,
	const FGuid& TargetPartInstanceId,
	int32 Amount)
{
	if (!TargetPartInstanceId.IsValid())
	{
		return;
	}

	for (const TObjectPtr<UEnemyPartWidget>& Part : SpawnedParts)
	{
		if (!Part || Part->GetPartInstanceId() != TargetPartInstanceId)
		{
			continue;
		}

		Part->PlayBattlePresentationCue(SourceEventType, Amount);
		return;
	}
}
