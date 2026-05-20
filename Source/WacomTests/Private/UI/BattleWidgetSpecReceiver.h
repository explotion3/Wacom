// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/CardWidget.h"
#include "UI/Battle/HandPanel.h"
#include "Components/BorderSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "BattleWidgetSpecReceiver.generated.h"

UCLASS()
class UWacomBattleCardWidgetClickReceiver : public UObject
{
	GENERATED_BODY()

public:
	int32 ClickCount = 0;
	FGuid LastClickedId;

	UFUNCTION()
	void HandleClicked(FGuid CardInstanceId)
	{
		++ClickCount;
		LastClickedId = CardInstanceId;
	}
};

UCLASS()
class UWacomBattleCardWidgetNoCardViewTest : public UCardWidget
{
	GENERATED_BODY()

public:
	void DisableCardViewForTest()
	{
		CardView = nullptr;
	}

	FString GetFallbackZoneText() const
	{
		return ZoneText ? ZoneText->GetText().ToString() : FString();
	}
};

UCLASS()
class UWacomBattleHandPanelLayoutTest : public UHandPanel
{
	GENERATED_BODY()

public:
	FMargin GetCardSlotPaddingForTest(int32 ChildIndex) const
	{
		if (!UnifiedHandSlot || !UnifiedHandSlot->GetChildAt(ChildIndex))
		{
			return FMargin();
		}

		const UWidget* Child = UnifiedHandSlot->GetChildAt(ChildIndex);
		const UHorizontalBoxSlot* HorizontalSlot = Child ? Cast<UHorizontalBoxSlot>(Child->Slot) : nullptr;
		return HorizontalSlot ? HorizontalSlot->GetPadding() : FMargin();
	}

	EVerticalAlignment GetCardSlotVerticalAlignmentForTest(int32 ChildIndex) const
	{
		if (!UnifiedHandSlot || !UnifiedHandSlot->GetChildAt(ChildIndex))
		{
			return VAlign_Fill;
		}

		const UWidget* Child = UnifiedHandSlot->GetChildAt(ChildIndex);
		const UHorizontalBoxSlot* HorizontalSlot = Child ? Cast<UHorizontalBoxSlot>(Child->Slot) : nullptr;
		return HorizontalSlot ? HorizontalSlot->GetVerticalAlignment() : VAlign_Fill;
	}

	EHorizontalAlignment GetUnifiedSlotHorizontalAlignmentForTest() const
	{
		if (const UHorizontalBoxSlot* HorizontalSlot = UnifiedHandSlot ? Cast<UHorizontalBoxSlot>(UnifiedHandSlot->Slot) : nullptr)
		{
			return HorizontalSlot->GetHorizontalAlignment();
		}
		if (const UBorderSlot* BorderSlot = UnifiedHandSlot ? Cast<UBorderSlot>(UnifiedHandSlot->Slot) : nullptr)
		{
			return BorderSlot->GetHorizontalAlignment();
		}
		return HAlign_Fill;
	}
};
