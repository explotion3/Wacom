// Copyright Wacom. All Rights Reserved.

#include "Actors/BattleTriggerActor.h"

#include "Components/SphereComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#include "Enemies/EnemyDefinition.h"
#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"

ABattleTriggerActor::ABattleTriggerActor()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
	TriggerSphere->InitSphereRadius(TriggerRadius);
	TriggerSphere->SetCollisionProfileName(TEXT("Trigger"));
	TriggerSphere->SetGenerateOverlapEvents(true);
	RootComponent = TriggerSphere;
}

void ABattleTriggerActor::BeginPlay()
{
	Super::BeginPlay();

	// ---- 存档：PersistentId 校验 + 已销毁检查 ----
	if (PersistentId.IsNone())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[BattleTriggerActor] %s: PersistentId 未配置，本触发器不参与存档"),
			*GetName());
	}
	else
	{
		// 关卡级 id 唯一性检查：如果场景里已存在同 id Actor 且已经 BeginPlay，报错。
		for (TActorIterator<ABattleTriggerActor> It(GetWorld()); It; ++It)
		{
			ABattleTriggerActor* Other = *It;
			if (Other && Other != this && !Other->HasAnyFlags(RF_ClassDefaultObject))
			{
				if (Other->PersistentId == PersistentId)
				{
					UE_LOG(LogTemp, Error,
						TEXT("[BattleTriggerActor] PersistentId %s 与 %s 重复，请为每个 Trigger 设置唯一 id"),
						*PersistentId.ToString(),
						*Other->GetName());
					break;
				}
			}
		}

		// 已被销毁过：直接 Destroy 并退出。
		if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
		{
			if (AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(PC))
			{
				if (URunSession* Run = WacomPC->GetRunSession())
				{
					if (Run->IsTriggerDestroyed(PersistentId))
					{
						UE_LOG(LogTemp, Display,
							TEXT("[BattleTriggerActor] %s (id=%s) 已在存档中被销毁，跳过生成"),
							*GetName(), *PersistentId.ToString());
						Destroy();
						return;
					}
				}
			}
		}
	}

	// TriggerRadius 可能在 Details 面板被修改，BeginPlay 同步一次。
	if (TriggerSphere)
	{
		TriggerSphere->SetSphereRadius(TriggerRadius);
		TriggerSphere->OnComponentBeginOverlap.AddDynamic(
			this, &ABattleTriggerActor::HandleBeginOverlap);
	}

	if (!EnemyDef)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[BattleTriggerActor] %s: EnemyDef 未配置，触发将被忽略"),
			*GetName());
	}
}

void ABattleTriggerActor::HandleBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/,
	const FHitResult& /*SweepResult*/)
{
	if (bTriggered) { return; }
	if (!OtherActor) { return; }
	if (!EnemyDef)   { return; }

	// 只接受被 Player 控制的 Pawn。
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn) { return; }

	AWacomPlayerController* PC = Cast<AWacomPlayerController>(Pawn->GetController());
	if (!PC) { return; }

	UE_LOG(LogTemp, Display,
		TEXT("[BattleTriggerActor] %s 触发战斗：EnemyDef=%s"),
		*GetName(),
		*GetNameSafe(EnemyDef));

	if (bConsumeOnTrigger)
	{
		bTriggered = true;
		// 先关掉 Overlap 事件避免重入；Destroy 留给 GameMode::ExitBattle 统一处理。
		if (TriggerSphere)
		{
			TriggerSphere->SetGenerateOverlapEvents(false);
		}
	}

	PC->RequestEnterBattle(EnemyDef, this);
}
