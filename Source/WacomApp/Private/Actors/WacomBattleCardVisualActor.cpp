// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomBattleCardVisualActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/CollisionProfile.h"
#include "Blueprint/UserWidget.h"
#include "UI/Battle/CardWidget.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FVector CardBoundsExtent(4.0f, 28.0f, 42.0f);
	const FVector CardFaceRelativeLocation(0.0f, 0.0f, 0.0f);
	const FRotator CardFaceRelativeRotation(0.0f, 0.0f, 0.0f);
	const FVector2D CardDrawSize(512.0f, 768.0f);
}

AWacomBattleCardVisualActor::AWacomBattleCardVisualActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	InteractionBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBounds"));
	InteractionBounds->SetupAttachment(SceneRoot);
	InteractionBounds->SetBoxExtent(CardBoundsExtent);
	InteractionBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBounds->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	InteractionBounds->SetGenerateOverlapEvents(false);

	CardFaceWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("CardFaceWidget"));
	CardFaceWidget->SetupAttachment(SceneRoot);
	CardFaceWidget->SetRelativeLocation(CardFaceRelativeLocation);
	CardFaceWidget->SetRelativeRotation(CardFaceRelativeRotation);
	CardFaceWidget->SetRelativeScale3D(FVector(0.08f));
	CardFaceWidget->SetWidgetSpace(EWidgetSpace::World);
	CardFaceWidget->SetDrawSize(CardDrawSize);
	CardFaceWidget->SetPivot(FVector2D(0.5f, 0.5f));
	CardFaceWidget->SetManuallyRedraw(true);
	CardFaceWidget->SetWindowFocusable(false);
	CardFaceWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FClassFinder<UCardWidget> DefaultCardWidgetClass(
		TEXT("/Game/Wacom/UI/Battle/WBP_CardWidget.WBP_CardWidget_C"));
	static ConstructorHelpers::FClassFinder<UCardWidget> FallbackCardWidgetClass(
		TEXT("/Game/Wacom/UI/Battle/WBP_CardWidget"));
	if (DefaultCardWidgetClass.Succeeded())
	{
		CardWidgetClass = DefaultCardWidgetClass.Class;
	}
	else if (FallbackCardWidgetClass.Succeeded())
	{
		CardWidgetClass = FallbackCardWidgetClass.Class;
	}
	else
	{
		CardWidgetClass = UCardWidget::StaticClass();
	}

	if (CardWidgetClass)
	{
		CardFaceWidget->SetWidgetClass(CardWidgetClass);
	}
}

void AWacomBattleCardVisualActor::BeginPlay()
{
	Super::BeginPlay();

	if (!CardFaceWidget->GetWidgetClass())
	{
		CardFaceWidget->SetWidgetClass(CardWidgetClass ? CardWidgetClass.Get() : UCardWidget::StaticClass());
	}

	EnsureCardWidget();
	CaptureCurrentTransformAsBaseIfNeeded();
}

void AWacomBattleCardVisualActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UCardWidget* CardWidget = GetCardWidget())
	{
		CardWidget->OnCardClicked.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

void AWacomBattleCardVisualActor::NotifyActorOnClicked(FKey ButtonPressed)
{
	Super::NotifyActorOnClicked(ButtonPressed);
	OnCardClickedNative.Broadcast(this, CachedSnapshot.InstanceId);
}

void AWacomBattleCardVisualActor::NotifyActorBeginCursorOver()
{
	Super::NotifyActorBeginCursorOver();
	SetHovered(true);
	OnCardHoveredNative.Broadcast(this, CachedSnapshot.InstanceId);
}

void AWacomBattleCardVisualActor::NotifyActorEndCursorOver()
{
	Super::NotifyActorEndCursorOver();
	SetHovered(false);
	OnCardUnhoveredNative.Broadcast(this, CachedSnapshot.InstanceId);
}

void AWacomBattleCardVisualActor::ApplyCardSnapshot(const FHandCardSnapshot& InSnapshot)
{
	CachedSnapshot = InSnapshot;

	if (UCardWidget* CardWidget = EnsureCardWidget())
	{
		CardWidget->ApplyCardSnapshot(CachedSnapshot);
		CardWidget->SetTargetingHighlight(bIsTargetingHighlighted);
	}

	RequestCardFaceRedraw();
}

void AWacomBattleCardVisualActor::SetTargetingHighlight(bool bHighlighted)
{
	if (bIsTargetingHighlighted == bHighlighted)
	{
		return;
	}

	bIsTargetingHighlighted = bHighlighted;

	if (UCardWidget* CardWidget = EnsureCardWidget())
	{
		CardWidget->SetTargetingHighlight(bIsTargetingHighlighted);
	}

	ApplyVisualWorldTransform();
	RequestCardFaceRedraw();
}

void AWacomBattleCardVisualActor::SetHovered(bool bInHovered)
{
	if (bIsHovered == bInHovered)
	{
		return;
	}

	bIsHovered = bInHovered;
	CaptureCurrentTransformAsBaseIfNeeded();
	ApplyVisualWorldTransform();
}

void AWacomBattleCardVisualActor::SetBaseWorldTransform(const FTransform& InBaseWorldTransform)
{
	BaseWorldTransform = InBaseWorldTransform;
	bHasBaseWorldTransform = true;
	ApplyVisualWorldTransform();
}

void AWacomBattleCardVisualActor::SetHoverOffset(const FVector& InHoverOffset)
{
	HoverOffset = InHoverOffset;
	if (bIsHovered)
	{
		ApplyVisualWorldTransform();
	}
}

UCardWidget* AWacomBattleCardVisualActor::EnsureCardWidget()
{
	if (!CardFaceWidget)
	{
		return nullptr;
	}

	if (!CardFaceWidget->GetWidgetClass())
	{
		CardFaceWidget->SetWidgetClass(CardWidgetClass ? CardWidgetClass.Get() : UCardWidget::StaticClass());
	}

	UUserWidget* UserWidget = CardFaceWidget->GetUserWidgetObject();
	if (!UserWidget)
	{
		CardFaceWidget->InitWidget();
		UserWidget = CardFaceWidget->GetUserWidgetObject();
	}

	UCardWidget* CardWidget = Cast<UCardWidget>(UserWidget);
	if (!CardWidget && CardFaceWidget->GetWidgetClass() != UCardWidget::StaticClass())
	{
		CardFaceWidget->SetWidgetClass(UCardWidget::StaticClass());
		CardFaceWidget->InitWidget();
		CardWidget = Cast<UCardWidget>(CardFaceWidget->GetUserWidgetObject());
	}

	if (CardWidget && !CardWidget->OnCardClicked.IsAlreadyBound(this, &AWacomBattleCardVisualActor::HandleCardWidgetClicked))
	{
		CardWidget->OnCardClicked.AddDynamic(this, &AWacomBattleCardVisualActor::HandleCardWidgetClicked);
	}

	return CardWidget;
}

UCardWidget* AWacomBattleCardVisualActor::GetCardWidget() const
{
	return CardFaceWidget ? Cast<UCardWidget>(CardFaceWidget->GetUserWidgetObject()) : nullptr;
}

void AWacomBattleCardVisualActor::RequestCardFaceRedraw() const
{
	if (CardFaceWidget)
	{
		CardFaceWidget->RequestRenderUpdate();
	}
}

void AWacomBattleCardVisualActor::CaptureCurrentTransformAsBaseIfNeeded()
{
	if (bHasBaseWorldTransform)
	{
		return;
	}

	BaseWorldTransform = GetActorTransform();
	bHasBaseWorldTransform = true;
}

void AWacomBattleCardVisualActor::ApplyVisualWorldTransform()
{
	CaptureCurrentTransformAsBaseIfNeeded();

	FTransform VisualTransform = BaseWorldTransform;
	if (bIsHovered)
	{
		VisualTransform.AddToTranslation(BaseWorldTransform.TransformVectorNoScale(HoverOffset));
	}

	if (bIsTargetingHighlighted)
	{
		VisualTransform.SetScale3D(BaseWorldTransform.GetScale3D() * TargetingHighlightScale);
	}
	else
	{
		VisualTransform.SetScale3D(BaseWorldTransform.GetScale3D());
	}

	SetActorTransform(VisualTransform);
}

void AWacomBattleCardVisualActor::HandleCardWidgetClicked(FGuid CardInstanceId)
{
	OnCardClickedNative.Broadcast(this, CardInstanceId);
}
