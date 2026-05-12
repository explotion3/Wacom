// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/BattleHUD.h"

#define LOCTEXT_NAMESPACE "WacomBattleHUD"
#include "UI/Battle/ActionPanel.h"
#include "UI/Battle/EnemyInfoBar.h"
#include "UI/Battle/EquipmentBar.h"
#include "UI/Battle/EventToast.h"
#include "UI/Battle/HandPanel.h"
#include "UI/Battle/PlayerStatusBar.h"
#include "UI/Common/PileCountView.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#include "Input/UIActionBindingHandle.h"
#include "Input/CommonUIInputSettings.h"

#include "Cards/CardDefinition.h"
#include "Commands/BattleCommand.h"
#include "Events/BattleEvent.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Types/WacomEnums.h"

void UBattleHUD::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// Widget Tree 尚未构造，ChildBattleWidgets 登记移到 NativeConstruct。
}

void UBattleHUD::NativeConstruct()
{
	Super::NativeConstruct();

	// 此时 RebuildWidget 已执行，BindWidget 字段都填好了。
	// 登记子 BattleWidget，让基类 SetSession / RefreshFromSnapshot 能递归到它们。
	ChildBattleWidgets.Reset();
	if (PlayerStatusBar) { ChildBattleWidgets.Add(PlayerStatusBar); }
	if (HandPanel)       { ChildBattleWidgets.Add(HandPanel); }
	if (EnemyInfoBar)    { ChildBattleWidgets.Add(EnemyInfoBar); }
	if (ActionPanel)     { ChildBattleWidgets.Add(ActionPanel); }
	if (EquipmentBar)    { ChildBattleWidgets.Add(EquipmentBar); }

	// 如果 Session 在 RebuildWidget 之前就被 SetSession 过了，
	// 子 Widget 还没拿到 Session。这里补一次。
	if (UBattleSession* S = GetSession())
	{
		for (const TObjectPtr<UWacomBattleWidgetBase>& Child : ChildBattleWidgets)
		{
			if (Child) { Child->SetSession(S); }
		}
		RefreshFromSnapshot(S->BuildSnapshot());
	}
}

TSharedRef<SWidget> UBattleHUD::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		// 默认布局：顶部 EnemyInfoBar，中间留空，底部 HandPanel，左下 PlayerStatusBar。
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		// EnemyInfoBar: 顶部中间
		EnemyInfoBar = WidgetTree->ConstructWidget<UEnemyInfoBar>(UEnemyInfoBar::StaticClass(), TEXT("EnemyInfoBar"));
		if (UCanvasPanelSlot* S = Root->AddChildToCanvas(EnemyInfoBar))
		{
			S->SetAnchors(FAnchors(0.5f, 0.0f));      // 顶部中心
			S->SetAlignment(FVector2D(0.5f, 0.0f));
			S->SetOffsets(FMargin(0.0f, 20.0f, 720.0f, 130.0f));
			S->SetAutoSize(false);
		}

		// PlayerStatusBar: 左下
		PlayerStatusBar = WidgetTree->ConstructWidget<UPlayerStatusBar>(UPlayerStatusBar::StaticClass(), TEXT("PlayerStatusBar"));
		if (UCanvasPanelSlot* S = Root->AddChildToCanvas(PlayerStatusBar))
		{
			S->SetAnchors(FAnchors(0.0f, 1.0f));
			S->SetAlignment(FVector2D(0.0f, 1.0f));
			S->SetOffsets(FMargin(20.0f, -140.0f, 220.0f, 120.0f));
			S->SetAutoSize(false);
		}

		// HandPanel: 底部
		HandPanel = WidgetTree->ConstructWidget<UHandPanel>(UHandPanel::StaticClass(), TEXT("HandPanel"));
		if (UCanvasPanelSlot* S = Root->AddChildToCanvas(HandPanel))
		{
			S->SetAnchors(FAnchors(0.5f, 1.0f));
			S->SetAlignment(FVector2D(0.5f, 1.0f));
			// 更宽 + 更高：水平装下 N 张卡 + Anchor Slot 不挤
			S->SetOffsets(FMargin(0.0f, -10.0f, 1400.0f, 200.0f));
			S->SetAutoSize(false);
		}

		// ActionPanel: 右下
		ActionPanel = WidgetTree->ConstructWidget<UActionPanel>(UActionPanel::StaticClass(), TEXT("ActionPanel"));
		if (UCanvasPanelSlot* S = Root->AddChildToCanvas(ActionPanel))
		{
			S->SetAnchors(FAnchors(1.0f, 1.0f));
			S->SetAlignment(FVector2D(1.0f, 1.0f));
			S->SetOffsets(FMargin(-30.0f, -200.0f, 160.0f, 140.0f));
			S->SetAutoSize(false);
		}

		// EquipmentBar: 顶部左侧
		EquipmentBar = WidgetTree->ConstructWidget<UEquipmentBar>(UEquipmentBar::StaticClass(), TEXT("EquipmentBar"));
		if (UCanvasPanelSlot* S = Root->AddChildToCanvas(EquipmentBar))
		{
			S->SetAnchors(FAnchors(0.0f, 0.0f));
			S->SetAlignment(FVector2D(0.0f, 0.0f));
			S->SetOffsets(FMargin(20.0f, 20.0f, 240.0f, 32.0f));
			S->SetAutoSize(false);
		}

		// DrawPileView: 底部左侧
		DrawPileView = WidgetTree->ConstructWidget<UPileCountView>(UPileCountView::StaticClass(), TEXT("DrawPileView"));
		DrawPileView->SetLabel(LOCTEXT("DrawPile", "抽牌堆"));
		if (UCanvasPanelSlot* S = Root->AddChildToCanvas(DrawPileView))
		{
			S->SetAnchors(FAnchors(0.0f, 1.0f));
			S->SetAlignment(FVector2D(0.0f, 1.0f));
			S->SetOffsets(FMargin(20.0f, -20.0f, 80.0f, 80.0f));
			S->SetAutoSize(false);
		}

		// DiscardPileView: 底部右侧偏左（放在 ActionPanel 左边）
		DiscardPileView = WidgetTree->ConstructWidget<UPileCountView>(UPileCountView::StaticClass(), TEXT("DiscardPileView"));
		DiscardPileView->SetLabel(LOCTEXT("DiscardPile", "弃牌堆"));
		if (UCanvasPanelSlot* S = Root->AddChildToCanvas(DiscardPileView))
		{
			S->SetAnchors(FAnchors(1.0f, 1.0f));
			S->SetAlignment(FVector2D(1.0f, 1.0f));
			S->SetOffsets(FMargin(-200.0f, -20.0f, 80.0f, 80.0f));
			S->SetAutoSize(false);
		}

		// ExhaustPileView: Discard 上方
		ExhaustPileView = WidgetTree->ConstructWidget<UPileCountView>(UPileCountView::StaticClass(), TEXT("ExhaustPileView"));
		ExhaustPileView->SetLabel(LOCTEXT("ExhaustPile", "消耗区"));
		if (UCanvasPanelSlot* S = Root->AddChildToCanvas(ExhaustPileView))
		{
			S->SetAnchors(FAnchors(1.0f, 1.0f));
			S->SetAlignment(FVector2D(1.0f, 1.0f));
			S->SetOffsets(FMargin(-200.0f, -110.0f, 80.0f, 80.0f));
			S->SetAutoSize(false);
		}

		// EventToast: 右侧中上
		EventToast = WidgetTree->ConstructWidget<UEventToast>(UEventToast::StaticClass(), TEXT("EventToast"));
		if (UCanvasPanelSlot* S = Root->AddChildToCanvas(EventToast))
		{
			S->SetAnchors(FAnchors(1.0f, 0.25f));
			S->SetAlignment(FVector2D(1.0f, 0.0f));
			S->SetOffsets(FMargin(-20.0f, 0.0f, 320.0f, 200.0f));
			S->SetAutoSize(false);
		}
	}
	return Super::RebuildWidget();
}

void UBattleHUD::NativeRefreshFromSnapshot(const FBattleSnapshot& Snap)
{
	// 战斗结束 → 切到 BattleEnd 状态，并广播一次
	if (Snap.Phase == EBattlePhase::BattleEnd)
	{
		SetUIState(EBattleUIState::BattleEnd);

		if (!bHasBroadcastBattleEnd)
		{
			bHasBroadcastBattleEnd = true;
			OnBattleEndedNative.Broadcast(Snap.Outcome);
		}
	}

	// 手动刷新 PileCountView（它们不是 BattleWidget，不走递归）
	if (DrawPileView)    { DrawPileView->SetCount(Snap.PileCounts.DrawCount); }
	if (DiscardPileView) { DiscardPileView->SetCount(Snap.PileCounts.DiscardCount); }
	if (ExhaustPileView) { ExhaustPileView->SetCount(Snap.PileCounts.ExhaustCount); }

	// 递归下发 Snapshot 给子 Widget
	Super::NativeRefreshFromSnapshot(Snap);
}

void UBattleHUD::NativeOnSessionChanged(UBattleSession* OldSession, UBattleSession* NewSession)
{
	Super::NativeOnSessionChanged(OldSession, NewSession);
	// 新 Session 接入时，重置状态机到 Idle。
	UIState = EBattleUIState::Idle;
	PendingTargetingCardId.Invalidate();
	bHasBroadcastBattleEnd = false;
}

TOptional<FUIInputConfig> UBattleHUD::GetDesiredInputConfig() const
{
	// All：鼠标可见 + 游戏输入透传。这样 UI 可点击，同时 BattleTestActor 的键盘快捷键仍工作。
	return FUIInputConfig(ECommonInputMode::All, EMouseCaptureMode::NoCapture);
}

// ================ 状态机 ================

void UBattleHUD::SetUIState(EBattleUIState NewState)
{
	if (UIState == NewState) { return; }
	const EBattleUIState OldState = UIState;
	UIState = NewState;
	NativeOnUIStateChanged(OldState, NewState);
	BP_OnUIStateChanged(OldState, NewState);
}

void UBattleHUD::NativeOnUIStateChanged(EBattleUIState /*OldState*/, EBattleUIState /*NewState*/)
{
	// 状态变化时，让 HandPanel / EnemyInfoBar / ActionPanel 重新刷一次（高亮/启用状态）。
	UBattleSession* S = GetSession();
	if (!S) { return; }
	const FBattleSnapshot Snap = S->BuildSnapshot();
	if (HandPanel)    { HandPanel->RefreshFromSnapshot(Snap); }
	if (EnemyInfoBar) { EnemyInfoBar->RefreshFromSnapshot(Snap); }
	if (ActionPanel)  { ActionPanel->RefreshFromSnapshot(Snap); }
}

// ================ 子 Widget 交互入口 ================

void UBattleHUD::OnCardClickedByUser(const FGuid& CardInstanceId)
{
	if (UIState == EBattleUIState::BattleEnd || UIState == EBattleUIState::Resolving)
	{
		return;
	}

	UBattleSession* S = GetSession();
	if (!S) { return; }

	// 同一张牌在 TargetSelect 状态下再点一次 → 取消
	if (UIState == EBattleUIState::TargetSelect && CardInstanceId == PendingTargetingCardId)
	{
		CancelTargetSelect();
		return;
	}

	const FBattleSnapshot Snap = S->BuildSnapshot();
	const FHandCardSnapshot* Card = nullptr;
	for (const FHandCardSnapshot& C : Snap.Hand.Cards)
	{
		if (C.InstanceId == CardInstanceId) { Card = &C; break; }
	}
	if (!Card || !Card->Definition) { return; }
	if (!Card->bIsPlayable) { return; }

	switch (Card->Definition->TargetMode)
	{
	case ECardTargetMode::None:
	case ECardTargetMode::Self:
		// 立即出牌，无目标
		SubmitPlayCard(CardInstanceId, FGuid());
		break;

	case ECardTargetMode::SingleEnemyPart:
		// 进入目标选择
		PendingTargetingCardId = CardInstanceId;
		SetUIState(EBattleUIState::TargetSelect);
		break;

	case ECardTargetMode::AllEnemyParts:
		// 无需选目标，直接打出
		SubmitPlayCard(CardInstanceId, FGuid());
		break;

	case ECardTargetMode::HandCard:
	default:
		// 第一阶段未支持，忽略
		break;
	}
}

void UBattleHUD::OnEnemyPartClickedByUser(const FGuid& PartInstanceId)
{
	if (UIState != EBattleUIState::TargetSelect) { return; }
	if (!PendingTargetingCardId.IsValid())       { return; }

	const FGuid CardId = PendingTargetingCardId;
	PendingTargetingCardId.Invalidate();

	SubmitPlayCard(CardId, PartInstanceId);
}

void UBattleHUD::OnWaitRequested()
{
	if (UIState == EBattleUIState::BattleEnd || UIState == EBattleUIState::Resolving) { return; }

	// Wait 期间自动取消 TargetSelect
	if (UIState == EBattleUIState::TargetSelect)
	{
		PendingTargetingCardId.Invalidate();
	}

	UBattleSession* S = GetSession();
	if (!S) { return; }

	const FWacomStatus Status = S->SubmitCommand(FBattleCommand::MakeWait());
	if (!Status.IsOk())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleHUD] Wait failed, code=%d"), (int32)Status.Code);
		return;
	}
	AfterCommand();
}

void UBattleHUD::OnEndTurnRequested()
{
	if (UIState == EBattleUIState::BattleEnd || UIState == EBattleUIState::Resolving) { return; }

	if (UIState == EBattleUIState::TargetSelect)
	{
		PendingTargetingCardId.Invalidate();
	}

	UBattleSession* S = GetSession();
	if (!S) { return; }

	const FWacomStatus Status = S->SubmitCommand(FBattleCommand::MakeEndTurn());
	if (!Status.IsOk())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleHUD] EndTurn failed, code=%d"), (int32)Status.Code);
		return;
	}
	AfterCommand();
}

void UBattleHUD::CancelTargetSelect()
{
	if (UIState != EBattleUIState::TargetSelect) { return; }
	PendingTargetingCardId.Invalidate();
	SetUIState(EBattleUIState::Idle);
}

// ================ 内部 ================

void UBattleHUD::SubmitPlayCard(const FGuid& CardId, const FGuid& TargetPartId)
{
	UBattleSession* S = GetSession();
	if (!S) { return; }

	const FWacomStatus Status = S->SubmitCommand(FBattleCommand::MakePlayCard(CardId, TargetPartId));
	if (!Status.IsOk())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleHUD] PlayCard failed, code=%d detail=%s"),
			(int32)Status.Code, *Status.Detail.ToString());
		// 保持当前状态机，允许玩家重试
		return;
	}
	SetUIState(EBattleUIState::Idle);
	AfterCommand();
}

void UBattleHUD::AfterCommand()
{
	ConsumeAndLogEvents();

	UBattleSession* S = GetSession();
	if (!S) { return; }

	RefreshFromSnapshot(S->BuildSnapshot());
}

namespace
{
	const TCHAR* HUDEventTypeToString(EBattleEventType T)
	{
		switch (T)
		{
		case EBattleEventType::BattleStarted:          return TEXT("BattleStarted");
		case EBattleEventType::TurnStarted:            return TEXT("TurnStarted");
		case EBattleEventType::CardsDrawn:             return TEXT("CardsDrawn");
		case EBattleEventType::HandZoneChanged:        return TEXT("HandZoneChanged");
		case EBattleEventType::CardPlayed:             return TEXT("CardPlayed");
		case EBattleEventType::InitiativeHit:          return TEXT("InitiativeHit");
		case EBattleEventType::ResistanceResolved:     return TEXT("ResistanceResolved");
		case EBattleEventType::PerfectReleaseResolved: return TEXT("PerfectReleaseResolved");
		case EBattleEventType::DamageDealt:            return TEXT("DamageDealt");
		case EBattleEventType::StatusApplied:          return TEXT("StatusApplied");
		case EBattleEventType::InitiativePushed:       return TEXT("InitiativePushed");
		case EBattleEventType::WaitPerformed:          return TEXT("WaitPerformed");
		case EBattleEventType::EnemyPartActed:         return TEXT("EnemyPartActed");
		case EBattleEventType::EnemyPartHpEmptied:     return TEXT("EnemyPartHpEmptied");
		case EBattleEventType::EnemyKnockdown:         return TEXT("EnemyKnockdown");
		case EBattleEventType::TurnEnded:              return TEXT("TurnEnded");
		case EBattleEventType::PassiveTriggered:       return TEXT("PassiveTriggered");
		case EBattleEventType::BattleEnded:            return TEXT("BattleEnded");
		default:                                        return TEXT("?");
		}
	}
}

void UBattleHUD::ConsumeAndLogEvents()
{
	UBattleSession* S = GetSession();
	if (!S) { return; }
	const TArray<FBattleEvent> Events = S->ConsumeEvents();
	for (const FBattleEvent& E : Events)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[BattleHUD] [#%d] %-22s Amount=%d Count=%d Actor=%s Card=%s Tag=%s"),
			E.Sequence,
			HUDEventTypeToString(E.Type),
			E.Amount,
			E.Count,
			*E.ActorInstanceId.ToString(EGuidFormats::Short),
			*E.CardInstanceId.ToString(EGuidFormats::Short),
			*E.Tag.ToString());
	}

	// 分发到 EventToast（如果就位）
	if (EventToast)
	{
		EventToast->EnqueueEvents(Events);
	}
}

#undef LOCTEXT_NAMESPACE

