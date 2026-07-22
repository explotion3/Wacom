// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomBattleEnemyActor.h"

#include "Actors/WacomBattleSceneEnemyAuthoringReport.h"
#include "Blueprint/UserWidget.h"
#include "Components/SceneComponent.h"
#include "Components/WacomBattleEnemyPartComponent.h"
#include "Components/WacomBattleEnemySceneRuntimeComponent.h"
#include "Components/WidgetComponent.h"
#include "Enemies/EnemyDefinition.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/WacomBattleEnemyPanelWidget.h"
#include "UI/Battle/WacomBattleEnemyUILayerPolicy.h"
#include "UI/Foundation/WacomUIDeveloperSettings.h"

namespace
{
	bool IsConstructiblePanelClass(TSubclassOf<UWacomBattleEnemyPanelWidget> PanelClass)
	{
		return PanelClass && !PanelClass->HasAnyClassFlags(
			CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists);
	}

	void ConfigurePanelLayer(UWidgetComponent& Component)
	{
		Component.SetInitialSharedLayerName(
			WacomBattleEnemyUILayerPolicy::CompactPanelSharedLayerName);
		Component.SetInitialLayerZOrder(
			WacomBattleEnemyUILayerPolicy::CompactPanelZOrder);
	}
}

AWacomBattleEnemyActor::AWacomBattleEnemyActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	EnemyPanelWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("EnemyPanelWidget"));
	EnemyPanelWidgetComponent->SetupAttachment(SceneRoot);
	ConfigurePanelLayer(*EnemyPanelWidgetComponent);
	EnemyPanelWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	EnemyPanelWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EnemyPanelWidgetComponent->SetVisibility(false, true);

	EnemySceneRuntimeComponent =
		CreateDefaultSubobject<UWacomBattleEnemySceneRuntimeComponent>(TEXT("EnemySceneRuntime"));
}

void AWacomBattleEnemyActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshEnemyPanelWidgetComponent();
	NotifyEnemySceneComponentTopologyChanged();
}

void AWacomBattleEnemyActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshEnemyPanelWidgetComponent();
	NotifyEnemySceneComponentTopologyChanged();
}

TArray<UWacomBattleEnemyPartComponent*>
AWacomBattleEnemyActor::GetBattleEnemyPartComponents() const
{
	TArray<UWacomBattleEnemyPartComponent*> Parts;
	if (EnemySceneRuntimeComponent)
	{
		EnemySceneRuntimeComponent->GetOrderedPartComponents(Parts);
	}
	return Parts;
}

void AWacomBattleEnemyActor::NotifyEnemySceneComponentTopologyChanged()
{
	if (EnemySceneRuntimeComponent)
	{
		EnemySceneRuntimeComponent->NotifyTypedHierarchyChanged();
	}
}

uint32 AWacomBattleEnemyActor::GetEnemySceneComponentTopologyRevision() const
{
	return EnemySceneRuntimeComponent
		? EnemySceneRuntimeComponent->GetTopologyRevision()
		: 0;
}

void AWacomBattleEnemyActor::ResetRuntimeScenePresentationForBattle()
{
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	if (EnemySceneRuntimeComponent)
	{
		EnemySceneRuntimeComponent->RefreshTypedHierarchy();
		EnemySceneRuntimeComponent->ResetRuntimeScenePresentationForBattle();
	}
}

void AWacomBattleEnemyActor::RetireRuntimeEncounterPresentation()
{
	ClearEnemyPanelViewData();
	if (EnemySceneRuntimeComponent)
	{
		EnemySceneRuntimeComponent->RetireRuntimeEncounterPresentation();
	}
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
}

bool AWacomBattleEnemyActor::IsRuntimeEncounterPresentationRetired() const
{
	return EnemySceneRuntimeComponent && EnemySceneRuntimeComponent->IsRuntimeRetired();
}

void AWacomBattleEnemyActor::RefreshEnemyPanelWidgetComponent()
{
	if (!EnemyPanelWidgetComponent)
	{
		return;
	}
	ConfigurePanelLayer(*EnemyPanelWidgetComponent);
	TSubclassOf<UWacomBattleEnemyPanelWidget> Resolved = EnemyPanelWidgetClass;
	if (!Resolved)
	{
		if (const UWacomUIDeveloperSettings* Settings = GetDefault<UWacomUIDeveloperSettings>())
		{
			Resolved = Settings->DefaultBattleEnemyPanelWidgetClass.LoadSynchronous();
		}
	}
	if (!IsConstructiblePanelClass(Resolved))
	{
		EnemyPanelWidgetComponent->SetWidgetClass(nullptr);
		EnemyPanelWidgetComponent->SetVisibility(false, true);
		return;
	}
	EnemyPanelWidgetComponent->SetWidgetClass(Resolved.Get());
	EnemyPanelWidgetComponent->SetRelativeLocation(EnemyPanelRelativeLocation);
	EnemyPanelWidgetComponent->SetDrawAtDesiredSize(bEnemyPanelDrawAtDesiredSize);
	EnemyPanelWidgetComponent->SetPivot(FVector2D(0.5f, 1.0f));
	if (!bEnemyPanelDrawAtDesiredSize)
	{
		EnemyPanelWidgetComponent->SetDrawSize(EnemyPanelDrawSize);
	}
	EnemyPanelWidgetComponent->SetVisibility(false, true);
}

void AWacomBattleEnemyActor::RefreshEnemyPanelVisibility()
{
	if (!EnemyPanelWidgetComponent)
	{
		return;
	}
	const bool bHasContext =
		!EnemyPanelHoveredPartSlotId.IsNone() || bEnemyPanelHasActionPreview;
	EnemyPanelWidgetComponent->SetVisibility(
		bEnemyPanelHasViewData && (bEnemyPanelVisibleByDefault || bHasContext), true);
}

void AWacomBattleEnemyActor::SetEnemyPanelViewData(
	const FWacomBattleEnemyPanelViewData& ViewData)
{
	if (!EnemyPanelWidgetComponent)
	{
		return;
	}
	EnemyPanelWidgetComponent->InitWidget();
	if (UWacomBattleEnemyPanelWidget* Panel = Cast<UWacomBattleEnemyPanelWidget>(
		EnemyPanelWidgetComponent->GetUserWidgetObject()))
	{
		Panel->TakeWidget();
		BindEnemyPanelInspectionDelegate(*Panel);
		Panel->SetEnemyPanelViewData(ViewData);
		Panel->SetInspectionInteractionEnabled(bEnemyPanelInspectionInteractionEnabled);
	}
	bEnemyPanelHasViewData = true;
	RefreshEnemyPanelVisibility();
}

void AWacomBattleEnemyActor::ClearEnemyPanelViewData()
{
	if (UWacomBattleEnemyPanelWidget* Panel = EnemyPanelWidgetComponent
		? Cast<UWacomBattleEnemyPanelWidget>(EnemyPanelWidgetComponent->GetUserWidgetObject())
		: nullptr)
	{
		Panel->SetInspectionInteractionEnabled(false);
		Panel->ClearEnemyPanelViewData();
	}
	bEnemyPanelHasViewData = false;
	bEnemyPanelHasActionPreview = false;
	bEnemyPanelInspectionInteractionEnabled = false;
	EnemyPanelHoveredPartSlotId = NAME_None;
	RefreshEnemyPanelVisibility();
}

void AWacomBattleEnemyActor::SetEnemyPanelActionPreview(
	const TArray<FWacomBattleEnemyPartEntryViewData>& PreviewParts)
{
	if (!EnemyPanelWidgetComponent)
	{
		return;
	}
	bEnemyPanelHasActionPreview = false;
	EnemyPanelWidgetComponent->InitWidget();
	if (UWacomBattleEnemyPanelWidget* Panel = Cast<UWacomBattleEnemyPanelWidget>(
		EnemyPanelWidgetComponent->GetUserWidgetObject()))
	{
		Panel->TakeWidget();
		BindEnemyPanelInspectionDelegate(*Panel);
		bEnemyPanelHasActionPreview = Panel->SetActionPreviewPartViews(PreviewParts);
	}
	RefreshEnemyPanelVisibility();
}

void AWacomBattleEnemyActor::ClearEnemyPanelActionPreview()
{
	if (UWacomBattleEnemyPanelWidget* Panel = EnemyPanelWidgetComponent
		? Cast<UWacomBattleEnemyPanelWidget>(EnemyPanelWidgetComponent->GetUserWidgetObject())
		: nullptr)
	{
		Panel->ClearActionPreview();
	}
	bEnemyPanelHasActionPreview = false;
	RefreshEnemyPanelVisibility();
}

void AWacomBattleEnemyActor::SetEnemyPanelHoveredPart(FName PartSlotId)
{
	if (EnemyPanelHoveredPartSlotId == PartSlotId)
	{
		return;
	}
	if (!EnemyPanelWidgetComponent)
	{
		return;
	}
	EnemyPanelHoveredPartSlotId = PartSlotId;
	EnemyPanelWidgetComponent->InitWidget();
	if (UWacomBattleEnemyPanelWidget* Panel = Cast<UWacomBattleEnemyPanelWidget>(
		EnemyPanelWidgetComponent->GetUserWidgetObject()))
	{
		BindEnemyPanelInspectionDelegate(*Panel);
		Panel->SetHoveredPartSlotId(PartSlotId);
	}
	RefreshEnemyPanelVisibility();
}

void AWacomBattleEnemyActor::SetEnemyPanelInspectionInteractionEnabled(bool bEnabled)
{
	const bool bNewEnabled = bEnabled && bEnemyPanelHasViewData;
	if (bEnemyPanelInspectionInteractionEnabled == bNewEnabled)
	{
		return;
	}
	bEnemyPanelInspectionInteractionEnabled = bNewEnabled;
	if (!EnemyPanelWidgetComponent)
	{
		return;
	}
	EnemyPanelWidgetComponent->InitWidget();
	if (UWacomBattleEnemyPanelWidget* Panel = Cast<UWacomBattleEnemyPanelWidget>(
		EnemyPanelWidgetComponent->GetUserWidgetObject()))
	{
		BindEnemyPanelInspectionDelegate(*Panel);
		Panel->SetInspectionInteractionEnabled(bEnemyPanelInspectionInteractionEnabled);
	}
}

void AWacomBattleEnemyActor::BindEnemyPanelInspectionDelegate(
	UWacomBattleEnemyPanelWidget& PanelWidget)
{
	PanelWidget.OnInspectionRequestedNative.RemoveAll(this);
	PanelWidget.OnInspectionRequestedNative.AddUObject(
		this, &ThisClass::HandleEnemyPanelInspectionRequested);
}

void AWacomBattleEnemyActor::HandleEnemyPanelInspectionRequested(
	const FBattlePartSlotIdentity& PartIdentity)
{
	if (bEnemyPanelInspectionInteractionEnabled && PartIdentity.IsValidSlot())
	{
		OnEnemyPanelInspectionRequestedNative.Broadcast(this, PartIdentity);
	}
}

FWacomBattleSceneEnemyDebugView AWacomBattleEnemyActor::GetBattleSceneEnemyDebugView() const
{
	return GetBattleSceneEnemyDebugViewForHUD(nullptr);
}

FWacomBattleSceneEnemyDebugView AWacomBattleEnemyActor::GetBattleSceneEnemyDebugViewForHUD(
	const UBattleHUD* HUD) const
{
	FWacomBattleSceneEnemyDebugView View;
	View.ActorName = GetName();
	View.EnemyDefinitionName = EnemyDefinition ? EnemyDefinition->GetFName() : NAME_None;
	View.EnemyId = EnemyDefinition ? EnemyDefinition->EnemyId : NAME_None;
	View.EnemySlotId = EnemySlotId;
	const FWacomBattleSceneEnemyHostAuthoringReport Report =
		FWacomBattleSceneEnemyHostAuthoringEvaluator::Build(*this);
	View.AuthoringState = Report.AuthoringState;
	View.bAuthoringReady = Report.bAuthoringReady;
	View.PartComponentCount = Report.PartComponentCount;
	View.PartSlotIds = Report.AttachedPartSlotIds;
	View.PartIds = Report.AttachedPartIds;
	View.bRuntimeEncounterPresentationRetired = IsRuntimeEncounterPresentationRetired();
	View.bUsedByBattleHUD = HUD && HUD->IsBattleSceneEnemyHostInCurrentRegistry(this);
	View.ActiveBattleHUDName = View.bUsedByBattleHUD ? HUD->GetName() : FString();
	return View;
}

FString AWacomBattleEnemyActor::GetBattleSceneEnemyDebugSummary() const
{
	const FWacomBattleSceneEnemyDebugView View = GetBattleSceneEnemyDebugView();
	return FString::Printf(
		TEXT("EnemyScene{Actor=%s Definition=%s EnemyId=%s EnemySlotId=%s State=%s Ready=%s Parts=%d Retired=%s}"),
		*View.ActorName,
		*View.EnemyDefinitionName.ToString(),
		*View.EnemyId.ToString(),
		*View.EnemySlotId.ToString(),
		*View.AuthoringState.ToString(),
		View.bAuthoringReady ? TEXT("true") : TEXT("false"),
		View.PartComponentCount,
		View.bRuntimeEncounterPresentationRetired ? TEXT("true") : TEXT("false"));
}

void AWacomBattleEnemyActor::LogBattleSceneEnemyDebugSummary() const
{
	UE_LOG(LogTemp, Display, TEXT("[WacomBattleEnemyActor] %s"),
		*GetBattleSceneEnemyDebugSummary());
}

#if WITH_EDITOR
void AWacomBattleEnemyActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RefreshEnemyPanelWidgetComponent();
	NotifyEnemySceneComponentTopologyChanged();
}

EDataValidationResult AWacomBattleEnemyActor::IsDataValid(
	FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	const FWacomBattleSceneEnemyHostAuthoringReport Report =
		FWacomBattleSceneEnemyHostAuthoringEvaluator::Build(*this);
	if (!Report.bAuthoringReady)
	{
		Context.AddError(FText::Format(
			NSLOCTEXT("WacomBattleEnemyActor", "InvalidComponentHierarchy",
				"敌人 Host {0} 的组件化制作报告为 {1}；请在 Details 同步部位并修正层级/资源错误。"),
			FText::FromString(GetName()),
			FText::FromName(Report.AuthoringState)));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
#endif
