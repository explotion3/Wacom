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

FWacomWorldShopPresentationHost
AWacomWorldShopHostActor::BuildPresentationHost() const
{
	return FWacomWorldShopPresentationHost::Make(
		*const_cast<AWacomWorldShopHostActor*>(this),
		GetEnabledOfferAnchorsSorted(),
		CardDrawSize,
		CardPivot,
		CardWorldScale,
		InteractionDistance,
		bTwoSided,
		bOverrideCursorLookProfile,
		CursorLookProfileOverride);
}

FWacomWorldShopHostValidationResult AWacomWorldShopHostActor::ValidateForOfferCount(int32 OfferCount) const
{
	return BuildPresentationHost().ValidateForOfferCount(OfferCount);
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
