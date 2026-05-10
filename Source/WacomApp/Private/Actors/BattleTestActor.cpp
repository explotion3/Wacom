// Copyright Wacom. All Rights Reserved.

#include "Actors/BattleTestActor.h"

#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Blueprint/UserWidget.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Commands/BattleCommand.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Events/BattleEvent.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Types/WacomEnums.h"

#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "UI/Foundation/WacomPrimaryGameLayout.h"
#include "UI/Foundation/WacomUITags.h"

ABattleTestActor::ABattleTestActor()
{
	PrimaryActorTick.bCanEverTick = false;
	AutoReceiveInput = EAutoReceiveInput::Player0;
}

void ABattleTestActor::BeginPlay()
{
	Super::BeginPlay();

	BindDebugInput();
	EnsurePrimaryLayout();

	if (bAutoStart)
	{
		StartBattle();
	}
}

void ABattleTestActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Primary Layout 和 HUD 由 UObject GC 和 Viewport 生命周期处理。
	Session            = nullptr;
	BattleHUDInstance  = nullptr;
	PrimaryLayout      = nullptr;
	Super::EndPlay(EndPlayReason);
}

// ================ UI 创建 ================

void ABattleTestActor::EnsurePrimaryLayout()
{
	if (PrimaryLayout) { return; }
	if (!PrimaryLayoutClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleTestActor] PrimaryLayoutClass 未配置"));
		return;
	}

	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleTestActor] 未找到 PlayerController"));
		return;
	}

	PrimaryLayout = CreateWidget<UWacomPrimaryGameLayout>(PC, PrimaryLayoutClass);
	if (!PrimaryLayout)
	{
		UE_LOG(LogTemp, Error, TEXT("[BattleTestActor] 创建 PrimaryLayout 失败"));
		return;
	}
	PrimaryLayout->AddToViewport();
}

void ABattleTestActor::EnsureBattleHUD()
{
	if (BattleHUDInstance) { return; }
	if (!PrimaryLayout)
	{
		EnsurePrimaryLayout();
		if (!PrimaryLayout) { return; }
	}
	if (!BattleHUDClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleTestActor] BattleHUDClass 未配置"));
		return;
	}

	UCommonActivatableWidget* Pushed = PrimaryLayout->PushWidgetToLayer(
		WacomUITags::UI_Layer_Game.GetTag(), BattleHUDClass);
	BattleHUDInstance = Cast<UWacomBattleWidgetBase>(Pushed);
	if (!BattleHUDInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("[BattleTestActor] Push BattleHUD 失败"));
		return;
	}

	BattleHUDInstance->SetSession(Session);
}

// ================ 输入绑定 ================

void ABattleTestActor::BindDebugInput()
{
	if (!InputComponent) { return; }

	auto Bind = [this](FKey Key, int32 OneBasedIndex)
	{
		FInputKeyBinding KB(FInputChord(Key), IE_Pressed);
		KB.KeyDelegate.GetDelegateForManualSet().BindLambda([this, OneBasedIndex]()
		{
			PlayHandIndex(OneBasedIndex);
		});
		InputComponent->KeyBindings.Add(KB);
	};

	Bind(EKeys::One,   1);
	Bind(EKeys::Two,   2);
	Bind(EKeys::Three, 3);
	Bind(EKeys::Four,  4);
	Bind(EKeys::Five,  5);

	auto BindAction = [this](FKey Key, TFunction<void()> Fn)
	{
		FInputKeyBinding KB(FInputChord(Key), IE_Pressed);
		KB.KeyDelegate.GetDelegateForManualSet().BindLambda(MoveTemp(Fn));
		InputComponent->KeyBindings.Add(KB);
	};

	BindAction(EKeys::W, [this]() { Wait(); });
	BindAction(EKeys::E, [this]() { EndTurn(); });
	BindAction(EKeys::R, [this]() { StartBattle(); });
	BindAction(EKeys::P, [this]() { RefreshHUD(); });
}

// ================ 战斗操作 ================

void ABattleTestActor::StartBattle()
{
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleTestActor] Character 未配置"));
		return;
	}
	if (!Enemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleTestActor] Enemy 未配置"));
		return;
	}

	Session = NewObject<UBattleSession>(this);

	FBattleInitParams P;
	P.Character  = Character;
	P.Enemy      = Enemy;
	P.RandomSeed = RandomSeed;

	const FWacomStatus Status = Session->Initialize(P);
	if (!Status.IsOk())
	{
		UE_LOG(LogTemp, Error, TEXT("[BattleTestActor] Initialize 失败, Code=%d"), (int32)Status.Code);
		Session = nullptr;
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("[BattleTestActor] Battle started"));

	EnsureBattleHUD();
	if (BattleHUDInstance)
	{
		BattleHUDInstance->SetSession(Session);
	}

	ConsumeAndLogEvents();
	RefreshHUD();
}

void ABattleTestActor::PlayHandIndex(int32 OneBasedIndex)
{
	if (!Session) { return; }

	const FBattleSnapshot Snap = Session->BuildSnapshot();
	const int32 Idx = OneBasedIndex - 1;
	if (!Snap.Hand.Cards.IsValidIndex(Idx))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleTestActor] Hand index %d 越界"), OneBasedIndex);
		return;
	}

	const FGuid CardId = Snap.Hand.Cards[Idx].InstanceId;

	// 路由到 HUD：让 HUD 按 TargetMode 统一处理（可能进入 TargetSelect）。
	// 这样命令提交 + 事件分发 + HUD 刷新都由 HUD 统一执行。
	if (UBattleHUD* HUD = Cast<UBattleHUD>(BattleHUDInstance))
	{
		HUD->OnCardClickedByUser(CardId);
		return;
	}

	// 回退：没有 HUD 时直接提交（带第一个存活部位作为目标）
	FGuid TargetPart;
	for (const auto& Part : Snap.Enemy.Parts)
	{
		if (!Part.bDestroyed) { TargetPart = Part.InstanceId; break; }
	}
	const FWacomStatus Status = Session->SubmitCommand(FBattleCommand::MakePlayCard(CardId, TargetPart));
	if (!Status.IsOk())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleTestActor] PlayCard 失败 Code=%d Detail=%s"),
			(int32)Status.Code, *Status.Detail.ToString());
		return;
	}
	ConsumeAndLogEvents();
	RefreshHUD();
}

void ABattleTestActor::Wait()
{
	if (!Session) { return; }

	if (UBattleHUD* HUD = Cast<UBattleHUD>(BattleHUDInstance))
	{
		HUD->OnWaitRequested();
		return;
	}

	// 回退
	const FWacomStatus Status = Session->SubmitCommand(FBattleCommand::MakeWait());
	if (!Status.IsOk())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleTestActor] Wait 失败 Code=%d"), (int32)Status.Code);
		return;
	}
	ConsumeAndLogEvents();
	RefreshHUD();
}

void ABattleTestActor::EndTurn()
{
	if (!Session) { return; }

	if (UBattleHUD* HUD = Cast<UBattleHUD>(BattleHUDInstance))
	{
		HUD->OnEndTurnRequested();
		return;
	}

	// 回退
	const FWacomStatus Status = Session->SubmitCommand(FBattleCommand::MakeEndTurn());
	if (!Status.IsOk())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BattleTestActor] EndTurn 失败 Code=%d"), (int32)Status.Code);
		return;
	}
	ConsumeAndLogEvents();
	RefreshHUD();
}

void ABattleTestActor::RefreshHUD()
{
	if (!Session || !BattleHUDInstance) { return; }
	BattleHUDInstance->RefreshFromSnapshot(Session->BuildSnapshot());
}

// ================ 事件日志 ================

namespace
{
	const TCHAR* EventTypeToString(EBattleEventType T)
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
		case EBattleEventType::BattleEnded:            return TEXT("BattleEnded");
		default:                                        return TEXT("?");
		}
	}
}

void ABattleTestActor::ConsumeAndLogEvents()
{
	if (!Session) { return; }
	const TArray<FBattleEvent> Events = Session->ConsumeEvents();
	for (const FBattleEvent& E : Events)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[BattleEvent] [#%d] %-22s Amount=%d Count=%d Actor=%s Card=%s Tag=%s"),
			E.Sequence,
			EventTypeToString(E.Type),
			E.Amount,
			E.Count,
			*E.ActorInstanceId.ToString(EGuidFormats::Short),
			*E.CardInstanceId.ToString(EGuidFormats::Short),
			*E.Tag.ToString());
	}
}
