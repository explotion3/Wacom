// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardPresentationAssetCollector.h"

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/UserWidget.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "UI/Card/WacomCardEffectBadgeWidget.h"
#include "UI/Card/WacomCardView.h"
#include "UI/Card/WacomFirstPersonCardViewWidget.h"

namespace
{
	void AddSoftPath(const FSoftObjectPath& Path, TArray<FSoftObjectPath>& OutPaths)
	{
		if (!Path.IsNull())
		{
			OutPaths.Add(Path);
		}
	}

	void SortAndUnique(TArray<FSoftObjectPath>& Paths)
	{
		Paths.Sort([](const FSoftObjectPath& A, const FSoftObjectPath& B)
		{
			return A.ToString() < B.ToString();
		});
		for (int32 Index = Paths.Num() - 1; Index > 0; --Index)
		{
			if (Paths[Index] == Paths[Index - 1])
			{
				Paths.RemoveAt(Index, 1, EAllowShrinking::No);
			}
		}
	}

	void CollectWidgetClass(
		UClass* WidgetClass,
		TSet<const UClass*>& VisitedClasses,
		FWacomFirstPersonCardPresentationAssetCollection& OutCollection)
	{
		if (!WidgetClass || VisitedClasses.Contains(WidgetClass))
		{
			return;
		}
		VisitedClasses.Add(WidgetClass);
		++OutCollection.VisitedWidgetClassCount;

		if (WidgetClass->IsChildOf(UWacomCardView::StaticClass()))
		{
			const UWacomCardView* CardView = WidgetClass->GetDefaultObject<UWacomCardView>();
			CardView->AppendPresentationSoftObjectPaths(OutCollection.RequiredVisualAssets);
			++OutCollection.VisitedCardViewTemplateCount;
		}
		if (WidgetClass->IsChildOf(UWacomCardEffectBadgeWidget::StaticClass()))
		{
			const UWacomCardEffectBadgeWidget* Badge =
				WidgetClass->GetDefaultObject<UWacomCardEffectBadgeWidget>();
			Badge->AppendPresentationSoftObjectPaths(OutCollection.RequiredVisualAssets);
			++OutCollection.VisitedBadgeTemplateCount;
		}

		const UWidgetBlueprintGeneratedClass* WidgetBlueprintClass =
			Cast<UWidgetBlueprintGeneratedClass>(WidgetClass);
		const UWidgetTree* WidgetTree = WidgetBlueprintClass
			? WidgetBlueprintClass->GetWidgetTreeArchetype()
			: nullptr;
		if (!WidgetTree)
		{
			return;
		}

		WidgetTree->ForEachWidget([&](UWidget* Widget)
		{
			if (!Widget)
			{
				return;
			}
			if (const UWacomCardView* CardView = Cast<UWacomCardView>(Widget))
			{
				CardView->AppendPresentationSoftObjectPaths(OutCollection.RequiredVisualAssets);
				++OutCollection.VisitedCardViewTemplateCount;
			}
			if (const UWacomCardEffectBadgeWidget* Badge =
				Cast<UWacomCardEffectBadgeWidget>(Widget))
			{
				Badge->AppendPresentationSoftObjectPaths(OutCollection.RequiredVisualAssets);
				++OutCollection.VisitedBadgeTemplateCount;
			}
			if (const UUserWidget* NestedUserWidget = Cast<UUserWidget>(Widget))
			{
				CollectWidgetClass(
					NestedUserWidget->GetClass(),
					VisitedClasses,
					OutCollection);
			}
		});
	}
}

FWacomFirstPersonCardPresentationAssetCollection
FWacomFirstPersonCardPresentationAssetCollector::Collect(
	const UWacomFirstPersonCardAnchorComponent& Anchor)
{
	FWacomFirstPersonCardPresentationAssetCollection Collection;
	TSet<const UClass*> VisitedClasses;
	CollectWidgetClass(
		Anchor.FirstPersonCardViewClass.Get(),
		VisitedClasses,
		Collection);

	AddSoftPath(Anchor.DrawnCardEnterSound.ToSoftObjectPath(), Collection.OptionalAudioAssets);
	AddSoftPath(Anchor.GainedCardEnterSound.ToSoftObjectPath(), Collection.OptionalAudioAssets);
	AddSoftPath(Anchor.RunHandCardEnterSound.ToSoftObjectPath(), Collection.OptionalAudioAssets);
	AddSoftPath(Anchor.HandAnchorCardEnterSound.ToSoftObjectPath(), Collection.OptionalAudioAssets);

	SortAndUnique(Collection.RequiredVisualAssets);
	SortAndUnique(Collection.OptionalAudioAssets);
	return Collection;
}
