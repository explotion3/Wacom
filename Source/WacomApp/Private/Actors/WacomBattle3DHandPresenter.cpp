// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomBattle3DHandPresenter.h"

#include "Actors/WacomBattleCardVisualActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UI/Battle/BattleHUD.h"

AWacomBattle3DHandPresenter::AWacomBattle3DHandPresenter()
{
	PrimaryActorTick.bCanEverTick = true;
	CardActorClass = AWacomBattleCardVisualActor::StaticClass();
}

void AWacomBattle3DHandPresenter::BeginPlay()
{
	Super::BeginPlay();
	ApplyCurrentLayout();
}

void AWacomBattle3DHandPresenter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ApplyCurrentLayout();
}

void AWacomBattle3DHandPresenter::Destroyed()
{
	DestroySpawnedCards();
	Super::Destroyed();
}

void AWacomBattle3DHandPresenter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroySpawnedCards();
	OwningBattleHUD.Reset();
	Super::EndPlay(EndPlayReason);
}

void AWacomBattle3DHandPresenter::RefreshFromSnapshot(const FBattleSnapshot& Snapshot)
{
	TSet<FGuid> IncomingIds;
	IncomingIds.Reserve(Snapshot.Hand.Cards.Num());

	OrderedCardIds.Reset(Snapshot.Hand.Cards.Num());
	for (const FHandCardSnapshot& CardSnapshot : Snapshot.Hand.Cards)
	{
		if (!CardSnapshot.InstanceId.IsValid())
		{
			continue;
		}

		IncomingIds.Add(CardSnapshot.InstanceId);
		OrderedCardIds.Add(CardSnapshot.InstanceId);
	}

	TArray<FGuid> RemovedIds;
	for (const TPair<FGuid, TObjectPtr<AWacomBattleCardVisualActor>>& Entry : CardActors)
	{
		if (!IncomingIds.Contains(Entry.Key))
		{
			RemovedIds.Add(Entry.Key);
		}
	}

	for (const FGuid& RemovedId : RemovedIds)
	{
		if (TObjectPtr<AWacomBattleCardVisualActor>* CardActorPtr = CardActors.Find(RemovedId))
		{
			DestroyCardActor(CardActorPtr->Get());
		}
		CardActors.Remove(RemovedId);
	}

	for (int32 Index = 0; Index < Snapshot.Hand.Cards.Num(); ++Index)
	{
		const FHandCardSnapshot& CardSnapshot = Snapshot.Hand.Cards[Index];
		if (!CardSnapshot.InstanceId.IsValid())
		{
			continue;
		}

		AWacomBattleCardVisualActor* CardActor = nullptr;
		if (TObjectPtr<AWacomBattleCardVisualActor>* ExistingActor = CardActors.Find(CardSnapshot.InstanceId))
		{
			CardActor = ExistingActor->Get();
		}

		if (!IsValid(CardActor))
		{
			CardActor = SpawnCardActor(CardSnapshot.InstanceId, Index, Snapshot.Hand.Cards.Num());
			if (CardActor)
			{
				CardActors.Add(CardSnapshot.InstanceId, CardActor);
			}
		}

		if (!CardActor)
		{
			continue;
		}

		CardActor->SetHoverOffset(FVector(0.0f, 0.0f, FMath::Max(0.0f, HoverLift)));
		CardActor->ApplyCardSnapshot(CardSnapshot);
	}

	ApplyCurrentLayout();
	ApplyTargetingHighlights();
}

void AWacomBattle3DHandPresenter::SetOwningBattleHUD(UBattleHUD* InHUD)
{
	OwningBattleHUD = InHUD;
}

void AWacomBattle3DHandPresenter::SetTargetSelectionView(const FBattleTargetSelectionView& TargetSelectionView)
{
	SetPendingTargetingCard(TargetSelectionView.bIsTargetSelecting
		? TargetSelectionView.PendingCardInstanceId
		: FGuid());
}

void AWacomBattle3DHandPresenter::SetPendingTargetingCard(const FGuid& CardInstanceId)
{
	PendingTargetingCardId = CardInstanceId;
	ApplyTargetingHighlights();
}

AWacomBattleCardVisualActor* AWacomBattle3DHandPresenter::GetCardActor(const FGuid& CardInstanceId) const
{
	const TObjectPtr<AWacomBattleCardVisualActor>* CardActor = CardActors.Find(CardInstanceId);
	return CardActor ? CardActor->Get() : nullptr;
}

FWacomBattle3DHandLayoutParams AWacomBattle3DHandPresenter::GetLayoutParams() const
{
	FWacomBattle3DHandLayoutParams Params;
	Params.Distance = Distance;
	Params.VerticalOffset = VerticalOffset;
	Params.CardSpacing = CardSpacing;
	Params.FanYawDegrees = FanYawDegrees;
	return Params;
}

FTransform AWacomBattle3DHandPresenter::ComputeCardTransform(
	int32 NumCards,
	int32 CardIndex,
	const FTransform& AnchorTransform,
	const FWacomBattle3DHandLayoutParams& LayoutParams)
{
	if (NumCards <= 0 || CardIndex < 0 || CardIndex >= NumCards)
	{
		return AnchorTransform;
	}

	const float CenterOffset = (static_cast<float>(CardIndex) - (static_cast<float>(NumCards - 1) * 0.5f));
	const FVector AnchorLocation = AnchorTransform.GetLocation();
	const FRotator AnchorRotation = AnchorTransform.Rotator();
	const FVector Forward = AnchorRotation.Vector();
	const FVector Right = FRotationMatrix(AnchorRotation).GetScaledAxis(EAxis::Y);
	const FVector Up = FRotationMatrix(AnchorRotation).GetScaledAxis(EAxis::Z);

	const FVector CardLocation =
		AnchorLocation
		+ Forward * FMath::Max(0.0f, LayoutParams.Distance)
		+ Right * (CenterOffset * FMath::Max(0.0f, LayoutParams.CardSpacing))
		+ Up * LayoutParams.VerticalOffset;

	FRotator CardRotation = (AnchorRotation + FRotator(0.0f, 180.0f, 0.0f)).GetNormalized();
	CardRotation.Yaw += CenterOffset * LayoutParams.FanYawDegrees;

	return FTransform(CardRotation, CardLocation, AnchorTransform.GetScale3D());
}

FTransform AWacomBattle3DHandPresenter::ResolveAnchorTransform() const
{
	if (bFollowLocalPlayerCamera)
	{
		const APlayerController* PC = ResolveOwningPlayerController();
		if (PC && PC->PlayerCameraManager)
		{
			return FTransform(
				PC->PlayerCameraManager->GetCameraRotation(),
				PC->PlayerCameraManager->GetCameraLocation(),
				FVector::OneVector);
		}
	}

	return GetActorTransform();
}

APlayerController* AWacomBattle3DHandPresenter::ResolveOwningPlayerController() const
{
	if (const UBattleHUD* HUD = OwningBattleHUD.Get())
	{
		if (APlayerController* PC = HUD->GetOwningPlayer())
		{
			return PC;
		}
	}

	if (APlayerController* OwnerPC = Cast<APlayerController>(GetOwner()))
	{
		return OwnerPC;
	}

	const UWorld* World = GetWorld();
	return World ? World->GetFirstPlayerController() : nullptr;
}

void AWacomBattle3DHandPresenter::ApplyCurrentLayout()
{
	const FTransform AnchorTransform = ResolveAnchorTransform();
	const FWacomBattle3DHandLayoutParams Params = GetLayoutParams();

	for (int32 Index = 0; Index < OrderedCardIds.Num(); ++Index)
	{
		AWacomBattleCardVisualActor* CardActor = GetCardActor(OrderedCardIds[Index]);
		if (!IsValid(CardActor))
		{
			continue;
		}

		CardActor->SetHoverOffset(FVector(0.0f, 0.0f, FMath::Max(0.0f, HoverLift)));
		CardActor->SetBaseWorldTransform(ComputeCardTransform(OrderedCardIds.Num(), Index, AnchorTransform, Params));
	}
}

void AWacomBattle3DHandPresenter::ApplyTargetingHighlights()
{
	for (const TPair<FGuid, TObjectPtr<AWacomBattleCardVisualActor>>& Entry : CardActors)
	{
		if (AWacomBattleCardVisualActor* CardActor = Entry.Value.Get())
		{
			CardActor->SetTargetingHighlight(PendingTargetingCardId.IsValid() && Entry.Key == PendingTargetingCardId);
		}
	}
}

void AWacomBattle3DHandPresenter::DestroySpawnedCards()
{
	for (const TPair<FGuid, TObjectPtr<AWacomBattleCardVisualActor>>& Entry : CardActors)
	{
		DestroyCardActor(Entry.Value.Get());
	}
	CardActors.Reset();
	OrderedCardIds.Reset();
}

AWacomBattleCardVisualActor* AWacomBattle3DHandPresenter::SpawnCardActor(
	const FGuid& CardInstanceId,
	int32 CardIndex,
	int32 CardCount)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	TSubclassOf<AWacomBattleCardVisualActor> ClassToSpawn = CardActorClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = AWacomBattleCardVisualActor::StaticClass();
	}

	const FTransform SpawnTransform = ComputeCardTransform(
		CardCount,
		CardIndex,
		ResolveAnchorTransform(),
		GetLayoutParams());

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AWacomBattleCardVisualActor* CardActor = World->SpawnActor<AWacomBattleCardVisualActor>(
		ClassToSpawn,
		SpawnTransform,
		SpawnParams);
	if (!CardActor)
	{
		return nullptr;
	}

	CardActor->SetBaseWorldTransform(SpawnTransform);
	CardActor->SetHoverOffset(FVector(0.0f, 0.0f, FMath::Max(0.0f, HoverLift)));
	CardActor->OnCardClickedNative.AddUObject(this, &AWacomBattle3DHandPresenter::HandleCardClicked);
	CardActor->OnCardHoveredNative.AddUObject(this, &AWacomBattle3DHandPresenter::HandleCardHovered);
	CardActor->OnCardUnhoveredNative.AddUObject(this, &AWacomBattle3DHandPresenter::HandleCardUnhovered);

	return CardActor;
}

void AWacomBattle3DHandPresenter::DestroyCardActor(AWacomBattleCardVisualActor* CardActor)
{
	if (!CardActor)
	{
		return;
	}

	CardActor->OnCardClickedNative.RemoveAll(this);
	CardActor->OnCardHoveredNative.RemoveAll(this);
	CardActor->OnCardUnhoveredNative.RemoveAll(this);

	if (!CardActor->IsActorBeingDestroyed())
	{
		CardActor->Destroy();
	}
}

void AWacomBattle3DHandPresenter::HandleCardClicked(AWacomBattleCardVisualActor* CardActor, FGuid CardInstanceId)
{
	const FGuid ResolvedCardId = CardInstanceId.IsValid() || !CardActor
		? CardInstanceId
		: CardActor->GetCardInstanceId();

	if (!ResolvedCardId.IsValid())
	{
		return;
	}

	OnCardClickedNative.Broadcast(ResolvedCardId);

	if (UBattleHUD* HUD = OwningBattleHUD.Get())
	{
		HUD->OnCardClickedByUser(ResolvedCardId);
	}
}

void AWacomBattle3DHandPresenter::HandleCardHovered(AWacomBattleCardVisualActor* CardActor, FGuid CardInstanceId)
{
	const FGuid ResolvedCardId = CardInstanceId.IsValid() || !CardActor
		? CardInstanceId
		: CardActor->GetCardInstanceId();

	if (ResolvedCardId.IsValid())
	{
		OnCardHoveredNative.Broadcast(ResolvedCardId);
	}
}

void AWacomBattle3DHandPresenter::HandleCardUnhovered(AWacomBattleCardVisualActor* CardActor, FGuid CardInstanceId)
{
	const FGuid ResolvedCardId = CardInstanceId.IsValid() || !CardActor
		? CardInstanceId
		: CardActor->GetCardInstanceId();

	if (ResolvedCardId.IsValid())
	{
		OnCardUnhoveredNative.Broadcast(ResolvedCardId);
	}
}
