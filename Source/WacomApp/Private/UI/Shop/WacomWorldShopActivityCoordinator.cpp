// Copyright Wacom. All Rights Reserved.

#include "UI/Shop/WacomWorldShopActivityCoordinator.h"

#include "Camera/WacomFirstPersonViewStageCoordinator.h"
#include "Camera/WacomFirstPersonViewStageRequest.h"
#include "Camera/WacomFirstPersonViewStageReturnFlow.h"
#include "Components/WacomBattleCameraLookComponent.h"
#include "Components/WacomRunFirstPersonCardSourceComponent.h"
#include "Components/WacomRunPathTraversalComponent.h"
#include "Components/WacomWorldShopOfferAnchorComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "GameFramework/WacomPlayerController.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInterface.h"
#include "RunSession.h"
#include "UI/Foundation/WacomAppToastSubsystem.h"
#include "UI/Card/WacomCardView.h"
#include "UI/Card/WacomWorldCardSurfaceMaterialAdapter.h"
#include "UI/Shop/WacomShopPresentationBuilder.h"
#include "UI/Shop/WacomShopVisitPresentationFlow.h"
#include "UI/Shop/WacomWorldShopCardWidget.h"
#include "UI/Shop/WacomWorldShopHUDWidget.h"
#include "UI/Shop/WacomWorldShopRoutePolicy.h"

EWacomWorldShopOpenResult FWacomWorldShopActivityCoordinator::TryOpen(
	AWacomPlayerController& InPlayerController,
	const FRunShopVisitRequest& Request,
	const FWacomFirstPersonViewStageRequest& StageRequest,
	const FWacomWorldShopPresentationHost& InHost)
{
	const FWacomWorldShopRouteDecision RouteDecision =
		FWacomWorldShopRoutePolicy::Evaluate(Request, InHost, InPlayerController.GetWorld());
	if (!RouteDecision.bUseWorldRoute)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[WorldShop] World route 不可用，回退 ShopScreen Host=%s Reason=%s Offers=%d"),
			*GetNameSafe(InHost.GetOwner()),
			*RouteDecision.Reason.ToString(),
			Request.Offers.Num());
		return EWacomWorldShopOpenResult::NotEligible;
	}
	UClass* CardViewClass = LoadClass<UWacomCardView>(
		nullptr,
		UWacomWorldShopCardWidget::GetRequiredCardViewClassPath());
	UMaterialInterface* WorldCardMaterial =
		FWacomWorldCardSurfaceMaterialAdapter::ResolveMaterial();
	if (!CardViewClass
		|| !CardViewClass->IsChildOf(UWacomCardView::StaticClass())
		|| !WorldCardMaterial)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[WorldShop] World route 依赖缺失，回退 ShopScreen CardView=%s Material=%s"),
			CardViewClass ? *CardViewClass->GetPathName() : TEXT("Missing"),
			WorldCardMaterial ? *WorldCardMaterial->GetPathName() : TEXT("Missing"));
		return EWacomWorldShopOpenResult::NotEligible;
	}
	if (State != EState::Inactive || !InPlayerController.GetRunSession()
		|| InPlayerController.GetRunSession()->IsShopVisitActive())
	{
		return EWacomWorldShopOpenResult::Failed;
	}
	AWacomPlayerCharacter* Pawn = InPlayerController.GetPawn<AWacomPlayerCharacter>();
	if (!Pawn)
	{
		return EWacomWorldShopOpenResult::NotEligible;
	}

	State = EState::Staging;
	++Generation;
	const uint32 OpenGeneration = Generation;
	PlayerController = &InPlayerController;
	Host = InHost;
	EntryBoundsGuard.SuppressForHost(InHost.GetOwner());
	InPlayerController.ClearRunWorldInteractionPresentation(
		TEXT("WorldShopActivity"));
	ExplorationHUDVisibilityGuard.SuppressForPlayerController(
		InPlayerController);
	PendingRequest = Request;
	bPreviousShowMouseCursor = InPlayerController.bShowMouseCursor;
	InPlayerController.bShowMouseCursor = true;
	Pawn->SetExplorationInputEnabled(false);
	if (UWacomRunFirstPersonCardSourceComponent* Source =
		InPlayerController.GetRunFirstPersonCardSourceComponent())
	{
		Source->SetRunFirstPersonCardLayerWorldActivitySuppressed(
			true,
			/*bAnimate*/ true);
	}

	const TWeakObjectPtr<AWacomPlayerController> WeakPC(&InPlayerController);
	auto CompleteStage = [this, WeakPC, OpenGeneration]()
	{
		AWacomPlayerController* PC = WeakPC.Get();
		AActor* StrongHost = Host.GetOwner();
		AWacomPlayerCharacter* StrongPawn = PC ? PC->GetPawn<AWacomPlayerCharacter>() : nullptr;
		if (!PC || !StrongHost || !StrongPawn || Generation != OpenGeneration
			|| State != EState::Staging)
		{
			Close(false);
			return;
		}
		FWacomCursorLookProfile Profile;
		if (Host.bOverrideCursorLookProfile)
		{
			Profile = Host.CursorLookProfileOverride.Sanitized();
		}
		else if (const UWacomRunPathTraversalComponent* RunPath =
			StrongPawn->GetRunPathTraversalComponent())
		{
			Profile = RunPath->GetLiveCursorLookProfile();
		}
		if (UWacomBattleCameraLookComponent* Look = StrongPawn->GetBattleCameraLookComponent())
		{
			Look->ActivateBattleCameraLookWithProfile(Profile);
		}
		if (!BeginVisitAndPresentation(OpenGeneration))
		{
			Close(bVisitOwned);
		}
	};

	TFunction<void()> DeferredStage = CompleteStage;
	const bool bDeferred = FWacomFirstPersonViewStageCoordinator::StageFirstPersonView(
		*Pawn, InPlayerController, StageRequest, MoveTemp(DeferredStage));
	if (!bDeferred)
	{
		CompleteStage();
	}
	return EWacomWorldShopOpenResult::Accepted;
}

bool FWacomWorldShopActivityCoordinator::BeginVisitAndPresentation(uint32 ExpectedGeneration)
{
	AWacomPlayerController* PC = PlayerController.Get();
	URunSession* Run = PC ? PC->GetRunSession() : nullptr;
	if (!PC || !Run || Generation != ExpectedGeneration || State != EState::Staging)
	{
		return false;
	}
	const FRunShopVisitResult Result =
		FWacomShopVisitPresentationFlow::BeginVisit(PC, Run, PendingRequest);
	if (!Result.bSucceeded || !Result.VisitToken.IsValid())
	{
		return false;
	}
	bVisitOwned = true;
	VisitToken = Result.VisitToken;
	Run->OnRunStateChangedNative.AddRaw(
		this,
		&FWacomWorldShopActivityCoordinator::HandleRunStateChanged);
	if (!RefreshPresentation()
		|| !InputRouter.Initialize(*PC, Host.InteractionDistance))
	{
		return false;
	}
	State = EState::Active;
	return true;
}

bool FWacomWorldShopActivityCoordinator::RefreshPresentation()
{
	AWacomPlayerController* PC = PlayerController.Get();
	AActor* StrongHost = Host.GetOwner();
	URunSession* Run = PC ? PC->GetRunSession() : nullptr;
	if (!PC || !StrongHost || !Run || !Run->IsShopVisitActive())
	{
		return false;
	}
	const FRunShopSnapshot Snapshot = Run->BuildCurrentShopSnapshot();
	const int32 Gold = Run->GetGold();
	const TArray<FWacomShopOfferPresentationView> Views =
		UWacomShopPresentationBuilder::BuildOfferPresentationViews(Snapshot, Gold);
	const TArray<UWacomWorldShopOfferAnchorComponent*> Anchors =
		Host.GetEnabledOfferAnchorsSorted();
	if (Views.Num() > Anchors.Num())
	{
		return false;
	}

	while (WorldCards.Num() > Views.Num())
	{
		FWorldCardRecord Record = WorldCards.Pop();
		if (UWacomWorldShopCardWidget* Widget = Record.Widget.Get())
		{
			Widget->OnPrimaryActionNative().RemoveAll(this);
		}
		if (UWidgetComponent* Component = Record.Component.Get())
		{
			StrongHost->RemoveInstanceComponent(Component);
			Component->DestroyComponent();
		}
	}

	for (int32 Index = 0; Index < Views.Num(); ++Index)
	{
		const FWacomShopOfferPresentationView& View = Views[Index];
		UWacomWorldShopOfferAnchorComponent* Anchor = Anchors[Index];
		FWorldCardRecord* Record = WorldCards.IsValidIndex(Index) ? &WorldCards[Index] : nullptr;
		if (!Record || !Record->Component.IsValid() || !Record->Widget.IsValid())
		{
			UWidgetComponent* Component = NewObject<UWidgetComponent>(
				StrongHost,
				FName(*FString::Printf(TEXT("WorldShopCard_%02d"), Index + 1)));
			StrongHost->AddInstanceComponent(Component);
			Component->SetupAttachment(Anchor);
			Component->SetRelativeTransform(FTransform::Identity);
			Component->SetWidgetSpace(EWidgetSpace::World);
			Component->SetBackgroundColor(FLinearColor::Transparent);
			Component->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Component->SetCollisionResponseToAllChannels(ECR_Ignore);
			Component->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
			Host.ApplyCardWidgetGeometry(*Component);
			if (!FWacomWorldCardSurfaceMaterialAdapter::Apply(
				*Component,
				FWacomWorldCardSurfaceMaterialAdapter::GetProductionExposureStrength()))
			{
				StrongHost->RemoveInstanceComponent(Component);
				Component->DestroyComponent();
				UE_LOG(LogTemp, Error,
					TEXT("[WorldShop] 正式世界卡材质应用失败 Material=%s Strength=%.1f"),
					FWacomWorldCardSurfaceMaterialAdapter::GetMaterialPath(),
					FWacomWorldCardSurfaceMaterialAdapter::GetProductionExposureStrength());
				return false;
			}
			Component->RegisterComponent();

			UWacomWorldShopCardWidget* Widget = CreateWidget<UWacomWorldShopCardWidget>(
				PC, UWacomWorldShopCardWidget::StaticClass());
			if (!Widget)
			{
				StrongHost->RemoveInstanceComponent(Component);
				Component->DestroyComponent();
				return false;
			}
			Widget->OnPrimaryActionNative().AddRaw(
				this,
				&FWacomWorldShopActivityCoordinator::HandleCardPrimaryAction);
			Component->SetWidget(Widget);
			FWorldCardRecord NewRecord;
			NewRecord.Component = Component;
			NewRecord.Widget = Widget;
			WorldCards.Add(NewRecord);
			Record = &WorldCards.Last();
		}
		Record->OfferId = View.OfferId;
		Record->SlotId = Anchor->SlotId;
		Record->Component->AttachToComponent(Anchor, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		Host.ApplyCardWidgetGeometry(*Record->Component);
		Record->Widget->SetOfferPresentation(View, Generation);
	}

	if (!HUD.IsValid())
	{
		HUD.Reset(CreateWidget<UWacomWorldShopHUDWidget>(PC, UWacomWorldShopHUDWidget::StaticClass()));
		if (HUD.IsValid())
		{
			HUD->AddToPlayerScreen(80);
		}
	}
	if (HUD.IsValid())
	{
		HUD->SetGold(Gold);
	}
	return HUD.IsValid();
}

bool FWacomWorldShopActivityCoordinator::RouteInputKey(const FKey& Key, EInputEvent Event)
{
	if (!IsOwningInput())
	{
		return false;
	}
	if (Key == EKeys::Escape)
	{
		if (Event == IE_Pressed)
		{
			Close(true);
		}
		return true;
	}
	if (Key == EKeys::LeftMouseButton)
	{
		InputRouter.RoutePointerKey(Key, Event);
		// World Shop owns the left pointer throughout Staging/Active/Closing.
		// During Staging there is intentionally no WIC yet, but the event must
		// not leak into hand, Run branch, or ordinary world-click routing.
		return true;
	}
	return false;
}

void FWacomWorldShopActivityCoordinator::HandleRunStateChanged()
{
	if (State == EState::Active && !bPurchaseInFlight)
	{
		RefreshPresentation();
	}
}

void FWacomWorldShopActivityCoordinator::HandleCardPrimaryAction(
	FGuid OfferId,
	uint32 IntentGeneration)
{
	if (State != EState::Active || bPurchaseInFlight || IntentGeneration != Generation)
	{
		return;
	}
	AWacomPlayerController* PC = PlayerController.Get();
	URunSession* Run = PC ? PC->GetRunSession() : nullptr;
	if (!PC || !Run)
	{
		return;
	}
	bPurchaseInFlight = true;
	const FRunShopSnapshot Snapshot = Run->BuildCurrentShopSnapshot();
	const TArray<FWacomShopOfferPresentationView> Views =
		UWacomShopPresentationBuilder::BuildOfferPresentationViews(Snapshot, Run->GetGold());
	const FRunShopPurchaseResult Result = FWacomShopVisitPresentationFlow::PurchaseOffer(
		PC,
		Run,
		PC->GetGameInstance()
			? PC->GetGameInstance()->GetSubsystem<UWacomAppToastSubsystem>()
			: nullptr,
		OfferId,
		Views);
	if (Result.bSucceeded && (Result.bVisitClosedAfterPurchase || !Run->IsShopVisitActive()))
	{
		bVisitOwned = false;
		bPurchaseInFlight = false;
		Close(false);
		return;
	}
	RefreshPresentation();
	bPurchaseInFlight = false;
}

void FWacomWorldShopActivityCoordinator::Close(bool bEndVisit)
{
	if (State == EState::Inactive || State == EState::Closing)
	{
		return;
	}
	State = EState::Closing;
	++Generation;
	InputRouter.CancelAndClear();
	AWacomPlayerController* PC = PlayerController.Get();
	URunSession* Run = PC ? PC->GetRunSession() : nullptr;
	if (Run)
	{
		Run->OnRunStateChangedNative.RemoveAll(this);
	}
	if (bEndVisit && bVisitOwned && Run)
	{
		FWacomShopVisitPresentationFlow::EndVisitIfOwned(PC, Run, VisitToken);
	}
	bVisitOwned = false;
	VisitToken.Invalidate();
	DestroyPresentation();
	RestoreExplorationPresentation();
}

void FWacomWorldShopActivityCoordinator::Shutdown()
{
	if (State == EState::Inactive)
	{
		return;
	}
	State = EState::Closing;
	++Generation;
	InputRouter.CancelAndClear();
	AWacomPlayerController* PC = PlayerController.Get();
	URunSession* Run = PC ? PC->GetRunSession() : nullptr;
	if (Run)
	{
		Run->OnRunStateChangedNative.RemoveAll(this);
		if (bVisitOwned)
		{
			FWacomShopVisitPresentationFlow::EndVisitIfOwned(PC, Run, VisitToken);
		}
	}
	bVisitOwned = false;
	VisitToken.Invalidate();
	DestroyPresentation();
	if (PC)
	{
		PC->bShowMouseCursor = bPreviousShowMouseCursor;
		if (UWacomRunFirstPersonCardSourceComponent* Source =
			PC->GetRunFirstPersonCardSourceComponent())
		{
			Source->SetRunFirstPersonCardLayerWorldActivitySuppressed(
				false,
				/*bAnimate*/ false);
		}
		if (AWacomPlayerCharacter* Pawn = PC->GetPawn<AWacomPlayerCharacter>())
		{
			FWacomFirstPersonViewStageCoordinator::CancelActiveStage(*Pawn);
			if (UWacomBattleCameraLookComponent* Look = Pawn->GetBattleCameraLookComponent())
			{
				Look->DeactivateBattleCameraLookPreservingView();
			}
			Pawn->SetExplorationInputEnabled(true, true);
		}
	}
	FinishClose();
}

void FWacomWorldShopActivityCoordinator::DestroyPresentation()
{
	AActor* StrongHost = Host.GetOwner();
	for (FWorldCardRecord& Record : WorldCards)
	{
		if (UWacomWorldShopCardWidget* Widget = Record.Widget.Get())
		{
			Widget->OnPrimaryActionNative().RemoveAll(this);
			Widget->CancelPendingPrimaryAction();
		}
		if (UWidgetComponent* Component = Record.Component.Get())
		{
			if (StrongHost)
			{
				StrongHost->RemoveInstanceComponent(Component);
			}
			Component->DestroyComponent();
		}
	}
	WorldCards.Reset();
	if (HUD.IsValid())
	{
		HUD->RemoveFromParent();
		HUD.Reset();
	}
}

void FWacomWorldShopActivityCoordinator::RestoreExplorationPresentation()
{
	AWacomPlayerController* PC = PlayerController.Get();
	AWacomPlayerCharacter* Pawn = PC ? PC->GetPawn<AWacomPlayerCharacter>() : nullptr;
	if (PC)
	{
		PC->bShowMouseCursor = bPreviousShowMouseCursor;
	}
	if (!PC || !Pawn)
	{
		FinishClose();
		return;
	}
	if (UWacomBattleCameraLookComponent* Look = Pawn->GetBattleCameraLookComponent())
	{
		Look->DeactivateBattleCameraLookPreservingView();
	}
	FWacomFirstPersonViewStageReturnFlow::ReturnToRunPath(
		*Pawn,
		*PC,
		[this]() { FinishClose(); });
}

void FWacomWorldShopActivityCoordinator::FinishClose()
{
	AWacomPlayerController* PC = PlayerController.Get();
	if (PC)
	{
		if (UWacomRunFirstPersonCardSourceComponent* Source =
			PC->GetRunFirstPersonCardSourceComponent())
		{
			Source->SetRunFirstPersonCardLayerWorldActivitySuppressed(
				false,
				/*bAnimate*/ true);
		}
	}
	EntryBoundsGuard.Restore();
	PendingRequest = FRunShopVisitRequest();
	Host.Reset();
	PlayerController.Reset();
	bPurchaseInFlight = false;
	State = EState::Inactive;
	ExplorationHUDVisibilityGuard.Restore();
	if (PC)
	{
		PC->RefreshInteractToast();
	}
}
