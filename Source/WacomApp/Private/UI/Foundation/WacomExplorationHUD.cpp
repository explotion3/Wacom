// Copyright Wacom. All Rights Reserved.

#include "UI/Foundation/WacomExplorationHUD.h"

#define LOCTEXT_NAMESPACE "WacomExplorationHUD"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Input/CommonUIInputTypes.h"

#include "UI/ViewModels/WacomRunViewModel.h"
#include "UI/ViewModels/WacomRunViewModelProvider.h"

UWacomExplorationHUD::UWacomExplorationHUD(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 不参与 Tab 焦点链；只是一个 input config 的锚点 + 数据展示。
	SetIsFocusable(false);
}

TOptional<FUIInputConfig> UWacomExplorationHUD::GetDesiredInputConfig() const
{
	// Game 模式 + 锁定鼠标到 Viewport + 隐藏光标：和 PC::SetInputMode(GameOnly) 等效。
	return FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanently);
}

namespace
{
	UBorder* MakePanel(UWidgetTree* Tree, FName Name)
	{
		UBorder* B = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
		B->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.55f));
		B->SetPadding(FMargin(10.f, 6.f));
		return B;
	}

	UTextBlock* MakeLabel(UWidgetTree* Tree, FName Name, const FText& InitialText, int32 FontSize)
	{
		UTextBlock* T = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		T->SetText(InitialText);
		FSlateFontInfo Font = T->GetFont();
		Font.Size = FontSize;
		T->SetFont(Font);
		T->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.95f, 0.95f)));
		return T;
	}
}

TSharedRef<SWidget> UWacomExplorationHUD::RebuildWidget()
{
	// WBP 子类有自己的 WidgetTree → 直接走父类，不进默认 C++ 布局分支
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		// 左上：时段 / 节点 / 天数
		{
			UBorder* Bg = MakePanel(WidgetTree, TEXT("TimeBg"));
			if (UCanvasPanelSlot* S = Root->AddChildToCanvas(Bg))
			{
				S->SetAnchors(FAnchors(0.f, 0.f));
				S->SetAlignment(FVector2D(0.f, 0.f));
				S->SetOffsets(FMargin(16.f, 16.f, 0.f, 0.f));
				S->SetAutoSize(true);
			}
			UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TimeVBox"));
			Bg->AddChild(V);

			PhaseText = MakeLabel(WidgetTree, TEXT("PhaseText"), LOCTEXT("PhaseInit", "时段：清晨"), 18);
			V->AddChildToVerticalBox(PhaseText);

			NodeText = MakeLabel(WidgetTree, TEXT("NodeText"), LOCTEXT("NodeInit", "节点：- / -"), 14);
			V->AddChildToVerticalBox(NodeText);

			DayText = MakeLabel(WidgetTree, TEXT("DayText"), LOCTEXT("DayInit", "第 1 天"), 14);
			V->AddChildToVerticalBox(DayText);
		}

		// 左下：手指 / 经验
		{
			UBorder* Bg = MakePanel(WidgetTree, TEXT("ExpBg"));
			if (UCanvasPanelSlot* S = Root->AddChildToCanvas(Bg))
			{
				S->SetAnchors(FAnchors(0.f, 1.f));
				S->SetAlignment(FVector2D(0.f, 1.f));
				S->SetOffsets(FMargin(16.f, 0.f, 0.f, 60.f));
				S->SetAutoSize(true);
			}
			UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ExpVBox"));
			Bg->AddChild(V);

			FingerText = MakeLabel(WidgetTree, TEXT("FingerText"), LOCTEXT("FingerInit", "手指：10 / 10"), 14);
			V->AddChildToVerticalBox(FingerText);

			ExpText = MakeLabel(WidgetTree, TEXT("ExpText"), LOCTEXT("ExpInit", "经验：0 / 10  技能：0"), 13);
			V->AddChildToVerticalBox(ExpText);

			ExpBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ExpBar"));
			ExpBar->SetPercent(0.f);
			ExpBar->SetFillColorAndOpacity(FLinearColor(0.4f, 0.85f, 0.4f, 1.f));
			if (UVerticalBoxSlot* SB = V->AddChildToVerticalBox(ExpBar))
			{
				SB->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
			}
		}

		// 右上：压力面板
		{
			UBorder* Bg = MakePanel(WidgetTree, TEXT("PressureBg"));
			if (UCanvasPanelSlot* S = Root->AddChildToCanvas(Bg))
			{
				S->SetAnchors(FAnchors(1.f, 0.f));
				S->SetAlignment(FVector2D(1.f, 0.f));
				S->SetOffsets(FMargin(0.f, 16.f, 16.f, 0.f));
				S->SetAutoSize(true);
			}
			UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PressureVBox"));
			Bg->AddChild(V);

			PressureTotalText = MakeLabel(WidgetTree, TEXT("PressureTotal"),
				LOCTEXT("PressureTotalInit", "压力总值：0 / 100"), 16);
			V->AddChildToVerticalBox(PressureTotalText);

			HungerText     = MakeLabel(WidgetTree, TEXT("HungerText"),     LOCTEXT("HungerInit",     "饥饿：  0%"), 13);
			WoundText      = MakeLabel(WidgetTree, TEXT("WoundText"),      LOCTEXT("WoundInit",      "伤口：  0%"), 13);
			FatigueText    = MakeLabel(WidgetTree, TEXT("FatigueText"),    LOCTEXT("FatigueInit",    "疲劳：  0%"), 13);
			BurdenText     = MakeLabel(WidgetTree, TEXT("BurdenText"),     LOCTEXT("BurdenInit",     "负重：  0%"), 13);
			DecayText      = MakeLabel(WidgetTree, TEXT("DecayText"),      LOCTEXT("DecayInit",      "腐朽：  0%"), 13);
			MisdeedText    = MakeLabel(WidgetTree, TEXT("MisdeedText"),    LOCTEXT("MisdeedInit",    "劣迹：  0%"), 13);
			BloodlustText  = MakeLabel(WidgetTree, TEXT("BloodlustText"),  LOCTEXT("BloodlustInit",  "嗜血：  0%"), 13);
			DisabilityText = MakeLabel(WidgetTree, TEXT("DisabilityText"), LOCTEXT("DisabilityInit", "残疾：  0%"), 13);

			V->AddChildToVerticalBox(HungerText);
			V->AddChildToVerticalBox(WoundText);
			V->AddChildToVerticalBox(FatigueText);
			V->AddChildToVerticalBox(BurdenText);
			V->AddChildToVerticalBox(DecayText);
			V->AddChildToVerticalBox(MisdeedText);
			V->AddChildToVerticalBox(BloodlustText);
			V->AddChildToVerticalBox(DisabilityText);
		}

		// 底部：操作提示
		{
			UBorder* Bg = MakePanel(WidgetTree, TEXT("HintBg"));
			if (UCanvasPanelSlot* S = Root->AddChildToCanvas(Bg))
			{
				S->SetAnchors(FAnchors(0.5f, 1.f));
				S->SetAlignment(FVector2D(0.5f, 1.f));
				S->SetOffsets(FMargin(0.f, 0.f, 0.f, 16.f));
				S->SetAutoSize(true);
			}
			HintText = MakeLabel(WidgetTree, TEXT("HintText"),
				LOCTEXT("HintInit", "B：背包    ESC：菜单"), 13);
			Bg->AddChild(HintText);
		}

		// 屏幕中下：交互 Toast（"按 E 战斗"），默认隐藏
		{
			InteractToastBg = MakePanel(WidgetTree, TEXT("InteractToastBg"));
			InteractToastBg->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.75f));
			InteractToastBg->SetPadding(FMargin(20.f, 10.f));
			if (UCanvasPanelSlot* S = Root->AddChildToCanvas(InteractToastBg))
			{
				S->SetAnchors(FAnchors(0.5f, 0.7f));
				S->SetAlignment(FVector2D(0.5f, 0.5f));
				S->SetOffsets(FMargin(0.f, 0.f, 0.f, 0.f));
				S->SetAutoSize(true);
			}
			InteractToastText = MakeLabel(WidgetTree, TEXT("InteractToastText"),
				LOCTEXT("InteractToastInit", "按 E 战斗"), 18);
			InteractToastBg->AddChild(InteractToastText);
			InteractToastBg->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	return Super::RebuildWidget();
}

void UWacomExplorationHUD::NativeConstruct()
{
	Super::NativeConstruct();
	TrySubscribeAndRefresh();
}

void UWacomExplorationHUD::NativeDestruct()
{
	if (UWacomRunViewModelProvider* Provider = SubscribedProvider.Get())
	{
		Provider->OnRunViewModelRefreshedNative.RemoveAll(this);
	}
	SubscribedProvider = nullptr;

	Super::NativeDestruct();
}

void UWacomExplorationHUD::NativeOnActivated()
{
	Super::NativeOnActivated();
	// 战斗结束后被 Reactivate，可能在订阅外错过广播；无条件刷新一次保底。
	TrySubscribeAndRefresh();
}

void UWacomExplorationHUD::TrySubscribeAndRefresh()
{
	if (!SubscribedProvider.Get())
	{
		if (UWacomRunViewModelProvider* Provider = GetProvider())
		{
			Provider->OnRunViewModelRefreshedNative.AddUObject(
				this, &UWacomExplorationHUD::HandleViewModelRefreshed);
			SubscribedProvider = Provider;
		}
	}
	RefreshFromViewModel();
}

void UWacomExplorationHUD::HandleViewModelRefreshed()
{
	RefreshFromViewModel();
}

void UWacomExplorationHUD::SetInteractToastVisible(bool bVisible, const FText& Message)
{
	if (InteractToastText && !Message.IsEmpty())
	{
		InteractToastText->SetText(Message);
	}
	if (InteractToastBg)
	{
		InteractToastBg->SetVisibility(bVisible
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
}

UWacomRunViewModelProvider* UWacomExplorationHUD::GetProvider() const
{
	UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UWacomRunViewModelProvider>() : nullptr;
}

UWacomRunViewModel* UWacomExplorationHUD::GetViewModel() const
{
	UWacomRunViewModelProvider* Provider = GetProvider();
	return Provider ? Provider->GetRunViewModel() : nullptr;
}

void UWacomExplorationHUD::RefreshFromViewModel()
{
	UWacomRunViewModel* VM = GetViewModel();
	if (!VM) { return; }

	if (PhaseText)
	{
		PhaseText->SetText(FText::Format(
			LOCTEXT("PhaseFmt", "时段：{0}"),
			VM->GetPhaseDisplay()));
	}
	if (NodeText)
	{
		NodeText->SetText(FText::Format(
			LOCTEXT("NodeFmt", "剩余节点：{0}"),
			FText::AsNumber(VM->GetRemainingNodeCount())));
	}
	if (DayText)
	{
		DayText->SetText(FText::Format(
			LOCTEXT("DayFmt", "第 {0} 天"),
			FText::AsNumber(VM->GetCurrentDayNumber())));
	}

	if (FingerText)
	{
		FingerText->SetText(FText::Format(
			LOCTEXT("FingerFmt", "手指：{0} / 10"),
			FText::AsNumber(VM->GetFingerCount())));
	}
	if (ExpText)
	{
		ExpText->SetText(FText::Format(
			LOCTEXT("ExpFmt", "经验：{0} / {1}  技能：{2}"),
			FText::AsNumber(VM->GetExperienceCurrent()),
			FText::AsNumber(VM->GetExperienceCapacity()),
			FText::AsNumber(VM->GetAcquiredSkillCount())));
	}
	if (ExpBar)
	{
		ExpBar->SetPercent(VM->GetExperienceRatio());
	}

	if (PressureTotalText)
	{
		PressureTotalText->SetText(FText::Format(
			LOCTEXT("PressureTotalFmt", "压力总值：{0} / 100"),
			FText::AsNumber(VM->GetPressureTotal())));
	}
	auto SetPct = [](UTextBlock* T, const FText& Label, int32 Value)
	{
		if (!T) { return; }
		T->SetText(FText::Format(
			LOCTEXT("PressureRowFmt", "{0}：{1}%"),
			Label, FText::AsNumber(Value)));
	};
	SetPct(HungerText,     LOCTEXT("Hunger",     "饥饿"), VM->GetPressureHunger());
	SetPct(WoundText,      LOCTEXT("Wound",      "伤口"), VM->GetPressureWound());
	SetPct(FatigueText,    LOCTEXT("Fatigue",    "疲劳"), VM->GetPressureFatigue());
	SetPct(BurdenText,     LOCTEXT("Burden",     "负重"), VM->GetPressureBurden());
	SetPct(DecayText,      LOCTEXT("Decay",      "腐朽"), VM->GetPressureDecay());
	SetPct(MisdeedText,    LOCTEXT("Misdeed",    "劣迹"), VM->GetPressureMisdeed());
	SetPct(BloodlustText,  LOCTEXT("Bloodlust",  "嗜血"), VM->GetPressureBloodlust());
	SetPct(DisabilityText, LOCTEXT("Disability", "残疾"), VM->GetPressureDisability());
}

#undef LOCTEXT_NAMESPACE
