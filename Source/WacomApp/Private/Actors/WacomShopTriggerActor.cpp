// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomShopTriggerActor.h"

#define LOCTEXT_NAMESPACE "WacomShopTriggerActor"

#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "Components/WacomRunWorldInteractionTargetBridgeComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

#include "Actors/WacomFirstPersonViewpointActor.h"
#include "Actors/WacomWorldShopHostActor.h"
#include "Camera/WacomFirstPersonViewStageRequest.h"
#include "GameFramework/WacomPlayerController.h"
#include "Shops/ShopDefinition.h"
#include "UI/Shop/WacomWorldShopRoutePolicy.h"

namespace
{
	bool ShouldValidateShopTriggerPlacementActor(const AWacomShopTriggerActor& Shop)
	{
		return !Shop.HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)
			&& !Shop.IsTemplate();
	}
}

AWacomShopTriggerActor::AWacomShopTriggerActor()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
	TriggerSphere->InitSphereRadius(TriggerRadius);
	TriggerSphere->SetCollisionProfileName(TEXT("Trigger"));
	TriggerSphere->SetGenerateOverlapEvents(true);
	RootComponent = TriggerSphere;

	ClickBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("ClickBounds"));
	ClickBounds->SetupAttachment(RootComponent);
	ClickBounds->SetBoxExtent(FVector(100.f, 100.f, 100.f));
	FWacomRunWorldClickableInteractableHelper::ConfigureClickBounds(ClickBounds);

	ClickInteractionTargetComponent =
		CreateDefaultSubobject<UWacomInteractionTargetComponent>(TEXT("ClickInteractionTarget"));

	ClickTargetBridgeComponent =
		CreateDefaultSubobject<UWacomRunWorldInteractionTargetBridgeComponent>(TEXT("ClickTargetBridge"));
	FWacomRunWorldClickableInteractableHelper::BindClickTarget(
		PersistentId,
		ClickBounds,
		ClickInteractionTargetComponent,
		ClickTargetBridgeComponent);

	InteractPromptText = LOCTEXT("DefaultInteractPrompt", "按 E 交易");
	HoverPromptText = LOCTEXT("DefaultHoverPrompt", "点击交易");
}

void AWacomShopTriggerActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshClickTargetBinding();
	if (ClickTargetBridgeComponent)
	{
		ClickTargetBridgeComponent->RefreshRunWorldTargetBinding();
	}

	if (PersistentId.IsNone())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[ShopTriggerActor] %s: PersistentId 未配置，商店不会打开"),
			*GetName());
	}
	else
	{
		if (HasDuplicatePersistentIdInWorld())
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[ShopTriggerActor] %s: PersistentId %s 与同关卡其他商店重复；两个商店会共享同一份库存状态"),
				*GetName(),
				*PersistentId.ToString());
		}
	}

	if (TriggerSphere)
	{
		TriggerSphere->SetSphereRadius(TriggerRadius);
		TriggerSphere->OnComponentBeginOverlap.AddDynamic(
			this, &AWacomShopTriggerActor::HandleBeginOverlap);
		TriggerSphere->OnComponentEndOverlap.AddDynamic(
			this, &AWacomShopTriggerActor::HandleEndOverlap);
	}
}

void AWacomShopTriggerActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshClickTargetBinding();
}

void AWacomShopTriggerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(PC))
		{
			WacomPC->UnregisterCandidateInteractable(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AWacomShopTriggerActor::HandleBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/,
	const FHitResult& /*SweepResult*/)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn) { return; }

	AWacomPlayerController* PC = Cast<AWacomPlayerController>(Pawn->GetController());
	if (!PC) { return; }

	PC->RegisterCandidateInteractable(this);
}

void AWacomShopTriggerActor::HandleEndOverlap(UPrimitiveComponent* /*OverlappedComp*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn) { return; }

	AWacomPlayerController* PC = Cast<AWacomPlayerController>(Pawn->GetController());
	if (!PC) { return; }

	PC->UnregisterCandidateInteractable(this);
}

FText AWacomShopTriggerActor::GetInteractPromptText_Implementation(AWacomPlayerController* /*PC*/) const
{
	return InteractPromptText.IsEmpty()
		? LOCTEXT("FallbackInteractPrompt", "按 E 交易")
		: InteractPromptText;
}

FText AWacomShopTriggerActor::GetHoverPromptText(AWacomPlayerController* /*PC*/) const
{
	return HoverPromptText.IsEmpty()
		? LOCTEXT("FallbackHoverPrompt", "点击交易")
		: HoverPromptText;
}

FVector AWacomShopTriggerActor::GetInteractLocation_Implementation(AWacomPlayerController* /*PC*/) const
{
	return GetActorLocation();
}

bool AWacomShopTriggerActor::CanInteract_Implementation(AWacomPlayerController* PC) const
{
	return PC && !PersistentId.IsNone();
}

TArray<FRunShopOfferInput> AWacomShopTriggerActor::BuildResolvedOffers() const
{
	if (!ShopDefinition)
	{
		return Offers;
	}

	TArray<FRunShopOfferInput> ResolvedOffers;
	ResolvedOffers.Reserve(ShopDefinition->Offers.Num());
	for (const FShopOfferDefinition& OfferDefinition : ShopDefinition->Offers)
	{
		FRunShopOfferInput OfferInput;
		OfferInput.CardDefinition = OfferDefinition.CardDefinition;
		OfferInput.Price = OfferDefinition.Price;
		ResolvedOffers.Add(OfferInput);
	}
	return ResolvedOffers;
}

FRunShopVisitRequest AWacomShopTriggerActor::BuildResolvedVisitRequest() const
{
	FRunShopVisitRequest Request;
	Request.ShopId = PersistentId;
	Request.Offers = BuildResolvedOffers();
	if (!ShopDefinition)
	{
		return Request;
	}

	Request.CardUpgradeService.bEnabled = ShopDefinition->CardUpgradeService.bEnabled;
	Request.CardUpgradeService.Prices.Reserve(ShopDefinition->CardUpgradeService.Prices.Num());
	for (const FShopCardUpgradePriceDefinition& PriceDefinition : ShopDefinition->CardUpgradeService.Prices)
	{
		FRunShopCardUpgradePriceInput PriceInput;
		PriceInput.FromRarity = PriceDefinition.FromRarity;
		PriceInput.Price = PriceDefinition.Price;
		Request.CardUpgradeService.Prices.Add(PriceInput);
	}
	return Request;
}

bool AWacomShopTriggerActor::TryInteract_Implementation(AWacomPlayerController* PC)
{
	if (!CanInteract_Implementation(PC))
	{
		return false;
	}

	FWacomFirstPersonViewStageRequest StageRequest;
	TryBuildShopEntryViewStageRequest(StageRequest);
	return PC->RequestOpenShop(
		BuildResolvedVisitRequest(),
		StageRequest,
		ResolveWorldShopHost());
}

bool AWacomShopTriggerActor::TryBuildShopEntryViewStageRequest(
	FWacomFirstPersonViewStageRequest& OutRequest) const
{
	OutRequest = FWacomFirstPersonViewStageRequest();
	const AWacomFirstPersonViewpointActor* ResolvedViewpoint =
		ResolveShopEntryViewpoint();
	if (!ResolvedViewpoint)
	{
		return false;
	}

	OutRequest.bHasViewTransform = true;
	OutRequest.ViewTransform = ResolvedViewpoint->GetActorTransform();
	OutRequest.BlendTimeSeconds =
		FMath::Max(0.0f, ResolvedViewpoint->StageBlendTimeSeconds);
	OutRequest.BlendCurve = ResolvedViewpoint->StageBlendCurve;
	OutRequest.BlendEasePower =
		FMath::Max(0.01f, ResolvedViewpoint->StageBlendEasePower);
	OutRequest.Reason = FName(TEXT("ShopEntry"));
	OutRequest.DebugSource = PersistentId.IsNone()
		? FName(*GetName())
		: PersistentId;
	return true;
}

AWacomFirstPersonViewpointActor*
AWacomShopTriggerActor::ResolveShopEntryViewpoint() const
{
	return ShopEntryViewpoint;
}

AWacomWorldShopHostActor* AWacomShopTriggerActor::ResolveWorldShopHost() const
{
	return WorldShopHost;
}

FWacomShopTriggerDebugView AWacomShopTriggerActor::GetShopTriggerDebugView(
	AWacomPlayerController* PC) const
{
	AWacomWorldShopHostActor* ResolvedWorldShopHost = ResolveWorldShopHost();
	FWacomShopTriggerDebugView View;
	View.ActorName = GetName();
	View.PersistentId = PersistentId;
	View.ShopDefinitionName = ShopDefinition ? ShopDefinition->GetName() : TEXT("None");
	View.ResolvedOfferCount = BuildResolvedOffers().Num();
	View.WorldShopHostName = ResolvedWorldShopHost
		? ResolvedWorldShopHost->GetName()
		: TEXT("None");
	const FWacomWorldShopRouteDecision RouteDecision = FWacomWorldShopRoutePolicy::Evaluate(
		BuildResolvedVisitRequest(),
		ResolvedWorldShopHost,
		PC ? PC->GetWorld() : GetWorld());
	View.bWorldRouteEligible = RouteDecision.bUseWorldRoute;
	View.WorldRouteReason = RouteDecision.Reason;
	View.bCanInteract = CanInteract_Implementation(PC);
	View.HoverPrompt = GetHoverPromptText(PC).ToString();

	if (!PC)
	{
		View.LastDebugResult = TEXT("MissingPlayerController");
	}
	else if (PersistentId.IsNone())
	{
		View.LastDebugResult = TEXT("MissingPersistentId");
	}
	else
	{
		View.LastDebugResult = TEXT("Ok");
	}

	const FWacomRunWorldClickableInteractableDebugView ClickDebug =
		GetRunWorldClickableDebugView_Implementation(PC);
	View.bClickTargetConfigured = ClickDebug.bClickTargetConfigured;
	View.ClickTargetStableId = ClickDebug.ClickTargetStableId;
	return View;
}

FString AWacomShopTriggerActor::GetShopTriggerDebugSummary(AWacomPlayerController* PC) const
{
	const FWacomShopTriggerDebugView View = GetShopTriggerDebugView(PC);
	const FWacomRunWorldClickableInteractableDebugView ClickDebug =
		GetRunWorldClickableDebugView_Implementation(PC);
	return FString::Printf(
		TEXT("ShopTrigger{Actor=%s PersistentId=%s ShopDef=%s Offers=%d WorldHost=%s WorldEligible=%s WorldReason=%s CanInteract=%s ClickTarget=%s ClickStableId=%s HoverPrompt=%s Last=%s ClickDebug=%s}"),
		*View.ActorName,
		*View.PersistentId.ToString(),
		*View.ShopDefinitionName,
		View.ResolvedOfferCount,
		*View.WorldShopHostName,
		View.bWorldRouteEligible ? TEXT("true") : TEXT("false"),
		*View.WorldRouteReason.ToString(),
		View.bCanInteract ? TEXT("true") : TEXT("false"),
		View.bClickTargetConfigured ? TEXT("true") : TEXT("false"),
		*View.ClickTargetStableId.ToString(),
		*View.HoverPrompt,
		*View.LastDebugResult.ToString(),
		*FWacomRunWorldClickableInteractableHelper::BuildDebugSummary(ClickDebug));
}

void AWacomShopTriggerActor::LogShopTriggerDebugSummary(AWacomPlayerController* PC) const
{
	UE_LOG(LogTemp, Display, TEXT("[ShopTriggerActor] %s"),
		*GetShopTriggerDebugSummary(PC));
}

FText AWacomShopTriggerActor::GetRunWorldClickHoverPrompt_Implementation(
	AWacomPlayerController* PC) const
{
	return GetHoverPromptText(PC);
}

FWacomRunWorldClickableInteractableDebugView
AWacomShopTriggerActor::GetRunWorldClickableDebugView_Implementation(
	AWacomPlayerController* PC) const
{
	FName LastResult = TEXT("Ok");
	if (!PC)
	{
		LastResult = TEXT("MissingPlayerController");
	}
	else if (PersistentId.IsNone())
	{
		LastResult = TEXT("MissingPersistentId");
	}

	return FWacomRunWorldClickableInteractableHelper::BuildDebugView(
		this,
		PersistentId,
		GetHoverPromptText(PC),
		CanInteract_Implementation(PC),
		/*bHasCompletionState*/false,
		/*bIsCompleted*/false,
		LastResult,
		ClickInteractionTargetComponent,
		ClickTargetBridgeComponent,
		ClickBounds);
}

#if WITH_EDITOR
EDataValidationResult AWacomShopTriggerActor::IsDataValid(
	FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (!ShouldValidateShopTriggerPlacementActor(*this))
	{
		return Result;
	}

	if (PersistentId.IsNone())
	{
		Context.AddError(FText::Format(
			LOCTEXT("PlacementMissingPersistentId",
				"Shop Trigger 摆放配置错误：Actor={0} 缺少 PersistentId，运行时不会打开商店。ShopDefinition={1} OfferCount={2}。"),
			FText::FromString(GetName()),
			FText::FromString(ShopDefinition ? ShopDefinition->GetName() : TEXT("None")),
			FText::AsNumber(BuildResolvedOffers().Num())));
		Result = EDataValidationResult::Invalid;
	}

	const TArray<FRunShopOfferInput> ResolvedOffers = BuildResolvedOffers();
	int32 UsableOfferCount = 0;
	for (int32 Index = 0; Index < ResolvedOffers.Num(); ++Index)
	{
		const FRunShopOfferInput& Offer = ResolvedOffers[Index];
		if (!Offer.CardDefinition)
		{
			Context.AddError(FText::Format(
				LOCTEXT("PlacementMissingOfferCard",
					"Shop Trigger 摆放配置错误：Actor={0} PersistentId={1} ShopDefinition={2} OfferIndex={3}/{4} 缺少 CardDefinition。"),
				FText::FromString(GetName()),
				FText::FromName(PersistentId),
				FText::FromString(ShopDefinition ? ShopDefinition->GetName() : TEXT("None")),
				FText::AsNumber(Index),
				FText::AsNumber(ResolvedOffers.Num())));
			Result = EDataValidationResult::Invalid;
			continue;
		}
		if (Offer.Price < 0)
		{
			Context.AddError(FText::Format(
				LOCTEXT("PlacementNegativeOfferPrice",
					"Shop Trigger 摆放配置错误：Actor={0} PersistentId={1} ShopDefinition={2} OfferIndex={3}/{4} Price={5} 不能为负数。"),
				FText::FromString(GetName()),
				FText::FromName(PersistentId),
				FText::FromString(ShopDefinition ? ShopDefinition->GetName() : TEXT("None")),
				FText::AsNumber(Index),
				FText::AsNumber(ResolvedOffers.Num()),
				FText::AsNumber(Offer.Price)));
			Result = EDataValidationResult::Invalid;
			continue;
		}
		++UsableOfferCount;
	}

	if (UsableOfferCount <= 0)
	{
		Context.AddError(FText::Format(
			LOCTEXT("PlacementMissingUsableOffers",
				"Shop Trigger 摆放配置错误：Actor={0} PersistentId={1} ShopDefinition={2} 没有可用商品。配置 ShopDefinition 或有效的手工 Offers。OfferCount={3}。"),
			FText::FromString(GetName()),
			FText::FromName(PersistentId),
			FText::FromString(ShopDefinition ? ShopDefinition->GetName() : TEXT("None")),
			FText::AsNumber(ResolvedOffers.Num())));
		Result = EDataValidationResult::Invalid;
	}

	if (!PersistentId.IsNone() && HasDuplicatePersistentIdInWorld())
	{
		Context.AddWarning(FText::Format(
			LOCTEXT("PlacementDuplicatePersistentId",
				"Shop Trigger 摆放警告：Actor={0} PersistentId={1} ShopDefinition={2} OfferCount={3} 与同关卡其他 Shop Trigger 重复；这些商店会共享同一份库存和购买状态。"),
			FText::FromString(GetName()),
			FText::FromName(PersistentId),
			FText::FromString(ShopDefinition ? ShopDefinition->GetName() : TEXT("None")),
			FText::AsNumber(ResolvedOffers.Num())));
		if (Result != EDataValidationResult::Invalid)
		{
			Result = EDataValidationResult::Valid;
		}
	}

	if (AWacomWorldShopHostActor* ResolvedWorldShopHost = ResolveWorldShopHost())
	{
		const FWacomWorldShopRouteDecision RouteDecision = FWacomWorldShopRoutePolicy::Evaluate(
			BuildResolvedVisitRequest(),
			ResolvedWorldShopHost,
			GetWorld());
		if (!RouteDecision.bUseWorldRoute)
		{
			Context.AddWarning(FText::Format(
				LOCTEXT("WorldShopRouteFallback",
					"Shop Trigger 世界商店配置将回退既有 ShopScreen：Actor={0} Host={1} Reason={2} OfferCount={3}。该回退不会截断商店访问。"),
				FText::FromString(GetName()),
				FText::FromString(ResolvedWorldShopHost->GetName()),
				FText::FromName(RouteDecision.Reason),
				FText::AsNumber(ResolvedOffers.Num())));
		}
	}

	return Result == EDataValidationResult::Invalid
		? EDataValidationResult::Invalid
		: EDataValidationResult::Valid;
}
#endif

void AWacomShopTriggerActor::RefreshClickTargetBinding()
{
	FWacomRunWorldClickableInteractableHelper::BindClickTarget(
		PersistentId,
		ClickBounds,
		ClickInteractionTargetComponent,
		ClickTargetBridgeComponent);
}

bool AWacomShopTriggerActor::HasDuplicatePersistentIdInWorld() const
{
	if (PersistentId.IsNone() || !GetWorld())
	{
		return false;
	}

	for (TActorIterator<AWacomShopTriggerActor> It(GetWorld()); It; ++It)
	{
		const AWacomShopTriggerActor* Other = *It;
		if (Other
			&& Other != this
			&& !Other->HasAnyFlags(RF_ClassDefaultObject)
			&& Other->PersistentId == PersistentId)
		{
			return true;
		}
	}
	return false;
}

#undef LOCTEXT_NAMESPACE
