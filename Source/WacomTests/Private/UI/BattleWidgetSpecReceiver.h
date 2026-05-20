// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/CardWidget.h"
#include "UI/Battle/EnemyInfoBar.h"
#include "UI/Battle/EnemyPartWidget.h"
#include "UI/Battle/HandPanel.h"
#include "UI/Battle/WacomKnockdownChoiceDialog.h"
#include "Components/BorderSlot.h"
#include "Components/Button.h"
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

	int32 HoverCount = 0;
	int32 UnhoverCount = 0;
	TObjectPtr<UCardWidget> LastHoveredWidget = nullptr;
	TObjectPtr<UCardWidget> LastUnhoveredWidget = nullptr;

	void HandleHovered(UCardWidget* SourceWidget)
	{
		++HoverCount;
		LastHoveredWidget = SourceWidget;
	}

	void HandleUnhovered(UCardWidget* SourceWidget)
	{
		++UnhoverCount;
		LastUnhoveredWidget = SourceWidget;
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
class UWacomBattleCardWidgetHoverVisualRootTest : public UCardWidget
{
	GENERATED_BODY()

public:
	void DisableHoverVisualRootForTest()
	{
		HoverVisualRoot = nullptr;
	}

	bool HasHoverVisualRootForTest() const
	{
		return HoverVisualRoot != nullptr;
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

	UCardWidget* GetSpawnedCardForTest(int32 Index) const
	{
		return UHandPanel::GetSpawnedCardForTest(Index);
	}
};

UCLASS()
class UWacomBattleHUDDetailTest : public UBattleHUD
{
	GENERATED_BODY()

public:
	void SetTargetSelectionStateForTest(const FGuid& PendingCardId)
	{
		PendingTargetingCardId = PendingCardId;
		SetUIState(EBattleUIState::TargetSelect);
	}

	void ClearTargetSelectionStateForTest()
	{
		PendingTargetingCardId.Invalidate();
		SetUIState(EBattleUIState::Idle);
	}

	bool ShowCardDetailForTest(UCardWidget* SourceWidget)
	{
		return ShowCardDetailForCardWidget(SourceWidget);
	}

	void HideCardDetailForTest()
	{
		HideCardDetailPanel();
	}

	void HandleCardHoveredForTest(UCardWidget* SourceWidget)
	{
		HandleHandCardHovered(SourceWidget);
	}

	void HandleCardUnhoveredForTest(UCardWidget* SourceWidget)
	{
		HandleHandCardUnhovered(SourceWidget);
	}
};

UCLASS()
class UWacomBattleEnemyInfoBarTest : public UEnemyInfoBar
{
	GENERATED_BODY()

public:
	int32 GetSpawnedPartCountForTest() const
	{
		return SpawnedParts.Num();
	}

	bool IsSpawnedPartTargetableForTest(int32 Index) const
	{
		return SpawnedParts.IsValidIndex(Index) && SpawnedParts[Index]
			? SpawnedParts[Index]->IsTargetable()
			: false;
	}
};

UCLASS()
class UWacomBattleKnockdownChoiceDialogTest : public UWacomKnockdownChoiceDialog
{
	GENERATED_BODY()

public:
	bool IsAidButtonEnabledForTest() const
	{
		return AidButton ? AidButton->GetIsEnabled() : false;
	}

	bool IsWithdrawButtonEnabledForTest() const
	{
		return WithdrawButton ? WithdrawButton->GetIsEnabled() : false;
	}

	bool IsDestroyButtonEnabledForTest() const
	{
		return DestroyButton ? DestroyButton->GetIsEnabled() : false;
	}

	FString GetPartNameTextForTest() const
	{
		return PartNameText ? PartNameText->GetText().ToString() : FString();
	}
};
