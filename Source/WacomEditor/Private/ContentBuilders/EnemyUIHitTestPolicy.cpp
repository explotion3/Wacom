// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/EnemyUIHitTestPolicy.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "WidgetBlueprint.h"

namespace Wacom::ContentBuilder::EnemyUIHitTestPolicy
{
	namespace
	{
		enum class EInteractionRouteAnchorKind : uint8
		{
			None,
			Button,
			DynamicEntryContainer,
		};

		EInteractionRouteAnchorKind ResolveAnchorKind(const UWidget& Widget)
		{
			const FName Name = Widget.GetFName();
			if (Widget.IsA<UButton>()
				&& (Name == TEXT("InspectHitTarget")
					|| Name == TEXT("CloseButton")
					|| Name == TEXT("PartSelectButton")))
			{
				return EInteractionRouteAnchorKind::Button;
			}
			if (Widget.IsA<UPanelWidget>()
				&& (Name == TEXT("PartList") || Name == TEXT("PartNavigator")))
			{
				return EInteractionRouteAnchorKind::DynamicEntryContainer;
			}
			return EInteractionRouteAnchorKind::None;
		}

		bool AllowsDescendantHitTesting(const ESlateVisibility Visibility)
		{
			return Visibility == ESlateVisibility::Visible
				|| Visibility == ESlateVisibility::SelfHitTestInvisible;
		}

		bool ValidateAnchor(const UWidget& Anchor)
		{
			const EInteractionRouteAnchorKind Kind = ResolveAnchorKind(Anchor);
			if (Kind == EInteractionRouteAnchorKind::Button
				&& Anchor.GetVisibility() != ESlateVisibility::Visible)
			{
				return false;
			}
			if (Kind == EInteractionRouteAnchorKind::DynamicEntryContainer
				&& !AllowsDescendantHitTesting(Anchor.GetVisibility()))
			{
				return false;
			}

			for (const UWidget* Ancestor = Anchor.GetParent(); Ancestor;
				Ancestor = Ancestor->GetParent())
			{
				if (!AllowsDescendantHitTesting(Ancestor->GetVisibility()))
				{
					return false;
				}
			}
			return true;
		}

		bool CollectRouteRepairs(
			UWidgetBlueprint& Blueprint,
			TSet<UWidget*>& OutWidgets)
		{
			TArray<UWidget*> Widgets;
			Blueprint.WidgetTree->GetAllWidgets(Widgets);
			for (UWidget* Anchor : Widgets)
			{
				const EInteractionRouteAnchorKind Kind = Anchor
					? ResolveAnchorKind(*Anchor)
					: EInteractionRouteAnchorKind::None;
				if (Kind == EInteractionRouteAnchorKind::None)
				{
					continue;
				}

				const ESlateVisibility AnchorVisibility = Anchor->GetVisibility();
				if (Kind == EInteractionRouteAnchorKind::Button)
				{
					if (AnchorVisibility == ESlateVisibility::HitTestInvisible)
					{
						OutWidgets.Add(Anchor);
					}
					else if (AnchorVisibility != ESlateVisibility::Visible)
					{
						return false;
					}
				}
				else if (AnchorVisibility == ESlateVisibility::HitTestInvisible)
				{
					OutWidgets.Add(Anchor);
				}
				else if (!AllowsDescendantHitTesting(AnchorVisibility))
				{
					return false;
				}

				for (UWidget* Ancestor = Anchor->GetParent(); Ancestor;
					Ancestor = Ancestor->GetParent())
				{
					const ESlateVisibility Visibility = Ancestor->GetVisibility();
					if (Visibility == ESlateVisibility::HitTestInvisible)
					{
						OutWidgets.Add(Ancestor);
					}
					else if (!AllowsDescendantHitTesting(Visibility))
					{
						return false;
					}
				}
			}
			return true;
		}
	}

	bool ValidateInteractiveRoutes(const UWidgetBlueprint& Blueprint)
	{
		if (!Blueprint.WidgetTree || !Blueprint.WidgetTree->RootWidget)
		{
			return false;
		}

		TArray<UWidget*> Widgets;
		Blueprint.WidgetTree->GetAllWidgets(Widgets);
		for (const UWidget* Widget : Widgets)
		{
			if (Widget
				&& ResolveAnchorKind(*Widget) != EInteractionRouteAnchorKind::None
				&& !ValidateAnchor(*Widget))
			{
				return false;
			}
		}
		return true;
	}

	bool NormalizeInteractiveRoutes(
		UWidgetBlueprint& Blueprint,
		int32& OutChangedWidgetCount)
	{
		OutChangedWidgetCount = 0;
		if (!Blueprint.WidgetTree || !Blueprint.WidgetTree->RootWidget)
		{
			return false;
		}

		TSet<UWidget*> WidgetsToRepair;
		if (!CollectRouteRepairs(Blueprint, WidgetsToRepair))
		{
			return false;
		}
		if (WidgetsToRepair.IsEmpty())
		{
			return ValidateInteractiveRoutes(Blueprint);
		}

		Blueprint.Modify();
		for (UWidget* Widget : WidgetsToRepair)
		{
			if (!Widget)
			{
				continue;
			}
			Widget->Modify();
			const EInteractionRouteAnchorKind Kind = ResolveAnchorKind(*Widget);
			Widget->SetVisibility(Kind == EInteractionRouteAnchorKind::Button
				? ESlateVisibility::Visible
				: ESlateVisibility::SelfHitTestInvisible);
			++OutChangedWidgetCount;
		}
		return ValidateInteractiveRoutes(Blueprint);
	}
}
