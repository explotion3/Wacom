// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomWorldShopHostActor.h"

#include "Components/SceneComponent.h"
#include "Components/WacomWorldShopOfferAnchorComponent.h"
#include "GameFramework/WacomPlayerController.h"
#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "WacomWorldShopHostActor"

AWacomWorldShopHostActor::AWacomWorldShopHostActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	constexpr int32 Columns = 4;
	constexpr int32 Rows = 2;
	constexpr float HorizontalSpacing = 82.0f;
	constexpr float VerticalSpacing = 100.0f;
	for (int32 Row = 0; Row < Rows; ++Row)
	{
		for (int32 Column = 0; Column < Columns; ++Column)
		{
			const int32 Order = Row * Columns + Column;
			const FName ComponentName(*FString::Printf(TEXT("OfferAnchor_%02d"), Order + 1));
			UWacomWorldShopOfferAnchorComponent* Anchor =
				CreateDefaultSubobject<UWacomWorldShopOfferAnchorComponent>(ComponentName);
			Anchor->SetupAttachment(SceneRoot);
			Anchor->SlotId = FName(*FString::Printf(TEXT("Offer.%02d"), Order + 1));
			Anchor->SlotOrder = Order;
			Anchor->SetRelativeLocation(FVector(
				0.0f,
				(static_cast<float>(Column) - 1.5f) * HorizontalSpacing,
				(static_cast<float>(1 - Row) - 0.5f) * VerticalSpacing));
			DefaultOfferAnchors.Add(Anchor);
		}
	}
}

TArray<UWacomWorldShopOfferAnchorComponent*> AWacomWorldShopHostActor::GetEnabledOfferAnchorsSorted() const
{
	TArray<UWacomWorldShopOfferAnchorComponent*> Result;
	GetComponents(Result);
	Result.RemoveAll([](const UWacomWorldShopOfferAnchorComponent* Anchor)
	{
		return !IsValid(Anchor) || !Anchor->bEnabledForOffers;
	});
	Result.Sort([](const UWacomWorldShopOfferAnchorComponent& A, const UWacomWorldShopOfferAnchorComponent& B)
	{
		if (A.SlotOrder != B.SlotOrder)
		{
			return A.SlotOrder < B.SlotOrder;
		}
		return A.SlotId.LexicalLess(B.SlotId);
	});
	return Result;
}

void AWacomWorldShopHostActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AWacomPlayerController* PC = GetWorld()
		? Cast<AWacomPlayerController>(GetWorld()->GetFirstPlayerController())
		: nullptr)
	{
		PC->NotifyWorldShopHostEndPlay(this);
	}
	Super::EndPlay(EndPlayReason);
}

FWacomWorldShopHostValidationResult AWacomWorldShopHostActor::ValidateForOfferCount(int32 OfferCount) const
{
	FWacomWorldShopHostValidationResult Result;
	const TArray<UWacomWorldShopOfferAnchorComponent*> Anchors = GetEnabledOfferAnchorsSorted();
	Result.EnabledAnchorCount = Anchors.Num();
	if (Anchors.IsEmpty())
	{
		Result.FailureReason = TEXT("MissingOfferAnchors");
		return Result;
	}
	if (CardDrawSize.X <= 0 || CardDrawSize.Y <= 0
		|| !FMath::IsFinite(CardWorldScale) || CardWorldScale <= 0.0f
		|| !FMath::IsFinite(InteractionDistance) || InteractionDistance <= 0.0f
		|| !FMath::IsFinite(CardPivot.X) || !FMath::IsFinite(CardPivot.Y))
	{
		Result.FailureReason = TEXT("InvalidWidgetProfile");
		return Result;
	}
	if (bOverrideCursorLookProfile && !CursorLookProfileOverride.IsFinite())
	{
		Result.FailureReason = TEXT("InvalidLookProfile");
		return Result;
	}
	TSet<FName> SlotIds;
	TSet<int32> SlotOrders;
	for (const UWacomWorldShopOfferAnchorComponent* Anchor : Anchors)
	{
		if (!Anchor || Anchor->SlotId.IsNone())
		{
			Result.FailureReason = TEXT("MissingSlotId");
			return Result;
		}
		if (SlotIds.Contains(Anchor->SlotId) || SlotOrders.Contains(Anchor->SlotOrder))
		{
			Result.FailureReason = TEXT("DuplicateSlotIdentity");
			return Result;
		}
		const FTransform Transform = Anchor->GetRelativeTransform();
		const FVector AnchorScale = Transform.GetScale3D();
		if (Transform.ContainsNaN()
			|| FMath::IsNearlyZero(AnchorScale.X)
			|| FMath::IsNearlyZero(AnchorScale.Y)
			|| FMath::IsNearlyZero(AnchorScale.Z))
		{
			Result.FailureReason = TEXT("InvalidAnchorTransform");
			return Result;
		}
		SlotIds.Add(Anchor->SlotId);
		SlotOrders.Add(Anchor->SlotOrder);
	}
	if (OfferCount < 0 || Anchors.Num() < OfferCount)
	{
		Result.FailureReason = TEXT("InsufficientAnchorCapacity");
		return Result;
	}
	Result.bValid = true;
	return Result;
}

#if WITH_EDITOR
EDataValidationResult AWacomWorldShopHostActor::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult ParentResult = Super::IsDataValid(Context);
	const FWacomWorldShopHostValidationResult Result = ValidateForOfferCount(0);
	if (!Result.bValid)
	{
		Context.AddError(FText::Format(
			LOCTEXT("InvalidHost", "World Shop Host 配置无效：Actor={0} Reason={1} EnabledAnchors={2}。"),
			FText::FromString(GetName()),
			FText::FromName(Result.FailureReason),
			FText::AsNumber(Result.EnabledAnchorCount)));
		return EDataValidationResult::Invalid;
	}
	return ParentResult == EDataValidationResult::Invalid
		? EDataValidationResult::Invalid
		: EDataValidationResult::Valid;
}
#endif

#undef LOCTEXT_NAMESPACE
