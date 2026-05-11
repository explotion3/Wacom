// Copyright Wacom. All Rights Reserved.

#include "Actors/BattleTriggerActor.h"

#include "Components/SphereComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#include "Enemies/EnemyDefinition.h"
#include "GameFramework/WacomPlayerController.h"

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
