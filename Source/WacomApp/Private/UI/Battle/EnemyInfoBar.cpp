// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/EnemyInfoBar.h"
#include "UI/Battle/EnemyPartWidget.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"

#include "Blueprint/SlateBlueprintLibrary.h"
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
	UnregisterBattlePresentationTargets();

	SpawnedParts.Reset();
	if (!PartsContainer) { return; }

	PartsContainer->ClearChildren();

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

	if (UBattleHUD* HUD = FindOwningBattleHUD())
	{
		RegisterBattlePresentationTargets(*HUD);
	}
}

void UEnemyInfoBar::NativeDestruct()
{
	UnregisterBattlePresentationTargets();
	Super::NativeDestruct();
}

bool UEnemyInfoBar::TryGetPartWidgetCenterInViewport(
	const FGuid& PartInstanceId,
	FVector2D& OutWidgetPosition) const
{
	if (!PartInstanceId.IsValid())
	{
		return false;
	}

	for (const TObjectPtr<UEnemyPartWidget>& Part : SpawnedParts)
	{
		if (!Part || Part->GetPartInstanceId() != PartInstanceId)
		{
			continue;
		}

		const FGeometry& Geometry = Part->GetCachedGeometry();
		const FVector2D LocalCenter = Geometry.GetLocalSize() * 0.5f;
		const FVector2D AbsoluteCenter = Geometry.LocalToAbsolute(LocalCenter);
		FVector2D PixelPosition = FVector2D::ZeroVector;
		USlateBlueprintLibrary::AbsoluteToViewport(this, AbsoluteCenter, PixelPosition, OutWidgetPosition);
		return true;
	}

	return false;
}

UBattleHUD* UEnemyInfoBar::FindOwningBattleHUD() const
{
	for (UUserWidget* P = GetTypedOuter<UUserWidget>(); P; P = P->GetTypedOuter<UUserWidget>())
	{
		if (UBattleHUD* HUD = Cast<UBattleHUD>(P))
		{
			return HUD;
		}
	}
	return nullptr;
}

void UEnemyInfoBar::RegisterBattlePresentationTargets(UBattleHUD& HUD)
{
	for (const TObjectPtr<UEnemyPartWidget>& Part : SpawnedParts)
	{
		if (!Part || !Part->GetPartInstanceId().IsValid())
		{
			continue;
		}

		TWeakObjectPtr<UEnemyPartWidget> WeakPart = Part;
		HUD.RegisterBattlePresentationTarget(
			Part->GetPartInstanceId(),
			this,
			[WeakPart](const FWacomBattlePresentationTargetCue& Cue)
			{
				if (UEnemyPartWidget* StrongPart = WeakPart.Get())
				{
					StrongPart->PlayBattlePresentationCue(Cue);
				}
			});
	}
}

void UEnemyInfoBar::UnregisterBattlePresentationTargets()
{
	if (UBattleHUD* HUD = FindOwningBattleHUD())
	{
		HUD->UnregisterBattlePresentationTargetsForOwner(this);
	}
}

void UEnemyInfoBar::ApplyTargetableFromHUDState()
{
	UBattleHUD* HUD = FindOwningBattleHUD();

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
	if (UBattleHUD* HUD = FindOwningBattleHUD())
	{
		HUD->OnEnemyPartClickedByUser(PartInstanceId);
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("[EnemyInfoBar] 未找到 UBattleHUD 父 Widget"));
}
