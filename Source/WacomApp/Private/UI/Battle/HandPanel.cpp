// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/HandPanel.h"
#include "UI/Battle/CardWidget.h"
#include "UI/Battle/BattleHUD.h"

#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Border.h"
#include "Components/ScrollBox.h"
#include "Snapshots/BattleSnapshot.h"
#include "Types/WacomEnums.h"

UHandPanel::UHandPanel()
{
	// 默认卡类：自己构造时用自己的类
	CardWidgetClass = UCardWidget::StaticClass();
}

TSharedRef<SWidget> UHandPanel::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UHorizontalBox* Root = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		// 普通 Zone Slot：水平 ScrollBox + HorizontalBox 内层。卡多时可滚动。
		// 返回的 UHorizontalBox 就是实际往里加卡的容器。
		auto MakeZoneSlot = [this, Root](const FName& Name, FLinearColor Tint, float FillSize) -> UHorizontalBox*
		{
			UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
			Frame->SetBrushColor(Tint);
			Frame->SetPadding(FMargin(4));

			UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),
				FName(*(Name.ToString() + TEXT("Scroll"))));
			Scroll->SetOrientation(Orient_Horizontal);
			Scroll->SetScrollBarVisibility(ESlateVisibility::Collapsed);
			Frame->SetContent(Scroll);

			UHorizontalBox* Inner = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),
				FName(*(Name.ToString() + TEXT("Inner"))));
			Scroll->AddChild(Inner);

			if (UHorizontalBoxSlot* HS = Root->AddChildToHorizontalBox(Frame))
			{
				HS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				HS->SetPadding(FMargin(2));
				// 通过 SlateChildSize 的 Value 控制占比
				FSlateChildSize Sz(ESlateSizeRule::Fill);
				Sz.Value = FillSize;
				HS->SetSize(Sz);
			}
			return Inner;
		};

		// 锚点 Slot：只放一张卡，用 HorizontalBox + Border 即可。
		auto MakeAnchorSlot = [this, Root](const FName& Name, FLinearColor Tint) -> UHorizontalBox*
		{
			UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
			Frame->SetBrushColor(Tint);
			Frame->SetPadding(FMargin(4));

			UHorizontalBox* Inner = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),
				FName(*(Name.ToString() + TEXT("Inner"))));
			Frame->SetContent(Inner);

			if (UHorizontalBoxSlot* HS = Root->AddChildToHorizontalBox(Frame))
			{
				// 锚点 Slot 宽度自适应内容（一张卡 130 宽）
				FSlateChildSize Sz(ESlateSizeRule::Automatic);
				HS->SetSize(Sz);
				HS->SetPadding(FMargin(2));
			}
			return Inner;
		};

		UHorizontalBox* LZ = MakeZoneSlot  (TEXT("LeftZoneSlot"),    FLinearColor(0.10f, 0.20f, 0.40f, 0.3f), 2.0f);
		UHorizontalBox* LA = MakeAnchorSlot(TEXT("LeftAnchorSlot"),  FLinearColor(0.30f, 0.40f, 0.15f, 0.3f));
		UHorizontalBox* BZ = MakeZoneSlot  (TEXT("BothZoneSlot"),    FLinearColor(0.15f, 0.15f, 0.30f, 0.3f), 3.0f);
		UHorizontalBox* RA = MakeAnchorSlot(TEXT("RightAnchorSlot"), FLinearColor(0.30f, 0.40f, 0.15f, 0.3f));
		UHorizontalBox* RZ = MakeZoneSlot  (TEXT("RightZoneSlot"),   FLinearColor(0.10f, 0.20f, 0.40f, 0.3f), 2.0f);

		LeftZoneSlot    = LZ;
		LeftAnchorSlot  = LA;
		BothZoneSlot    = BZ;
		RightAnchorSlot = RA;
		RightZoneSlot   = RZ;
	}
	return Super::RebuildWidget();
}

void UHandPanel::NativeRefreshFromSnapshot(const FBattleSnapshot& Snap)
{
	ClearAllSlots();

	for (const FHandCardSnapshot& C : Snap.Hand.Cards)
	{
		UPanelWidget* TargetSlot = nullptr;
		if (C.bIsHandAnchor)
		{
			TargetSlot = (LeftAnchorSlot && LeftAnchorSlot->GetChildrenCount() == 0)
				? LeftAnchorSlot : RightAnchorSlot;
		}
		else
		{
			switch (C.Zone)
			{
			case EHandZone::Left:  TargetSlot = LeftZoneSlot;  break;
			case EHandZone::Both:  TargetSlot = BothZoneSlot;  break;
			case EHandZone::Right: TargetSlot = RightZoneSlot; break;
			default:               TargetSlot = BothZoneSlot;  break;
			}
		}
		if (!TargetSlot) { continue; }
		CreateAndPlaceCard(C, TargetSlot);
	}

	ApplyTargetingHighlight();
}

UCardWidget* UHandPanel::CreateAndPlaceCard(const FHandCardSnapshot& CardSnap, UPanelWidget* TargetSlot)
{
	if (!TargetSlot) { return nullptr; }

	TSubclassOf<UCardWidget> ClassToUse = CardSnap.bIsHandAnchor && AnchorCardWidgetClass
		? AnchorCardWidgetClass
		: CardWidgetClass;
	if (!ClassToUse) { ClassToUse = UCardWidget::StaticClass(); }

	UCardWidget* Card = CreateWidget<UCardWidget>(this, ClassToUse);
	if (!Card) { return nullptr; }

	TargetSlot->AddChild(Card);
	Card->ApplyCardSnapshot(CardSnap);
	Card->OnCardClicked.AddDynamic(this, &UHandPanel::HandleCardClicked);

	SpawnedCards.Add(Card);
	return Card;
}

void UHandPanel::ClearAllSlots()
{
	auto ClearSlot = [](UPanelWidget* P) { if (P) { P->ClearChildren(); } };
	ClearSlot(LeftZoneSlot);
	ClearSlot(LeftAnchorSlot);
	ClearSlot(BothZoneSlot);
	ClearSlot(RightAnchorSlot);
	ClearSlot(RightZoneSlot);
	SpawnedCards.Reset();
}

void UHandPanel::ApplyTargetingHighlight()
{
	UBattleHUD* HUD = nullptr;
	for (UUserWidget* P = GetTypedOuter<UUserWidget>(); P; P = P->GetTypedOuter<UUserWidget>())
	{
		HUD = Cast<UBattleHUD>(P);
		if (HUD) { break; }
	}
	if (!HUD) { return; }

	const bool bTargeting = HUD->IsInTargetSelect();
	const FGuid PendingId = HUD->GetPendingTargetingCardId();

	for (const TObjectPtr<UCardWidget>& Card : SpawnedCards)
	{
		if (!Card) { continue; }
		const bool bSelf = bTargeting && Card->GetCardInstanceId() == PendingId;
		Card->SetTargetingHighlight(bSelf);
	}
}

void UHandPanel::HandleCardClicked(FGuid CardInstanceId)
{
	for (UUserWidget* P = GetTypedOuter<UUserWidget>(); P; P = P->GetTypedOuter<UUserWidget>())
	{
		if (UBattleHUD* HUD = Cast<UBattleHUD>(P))
		{
			HUD->OnCardClickedByUser(CardInstanceId);
			return;
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("[HandPanel] 未找到 UBattleHUD 父 Widget，点击被吞"));
}
