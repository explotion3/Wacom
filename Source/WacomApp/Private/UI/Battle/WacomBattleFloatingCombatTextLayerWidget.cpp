// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleFloatingCombatTextLayerWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Settings/WacomLocalSettingsTypes.h"
#include "Settings/WacomSettingsSubsystem.h"
#include "UI/Battle/WacomBattleFloatingCombatTextEntryWidget.h"
#include "UI/Battle/WacomBattleFloatingCombatTextStyle.h"
#include "UI/Battle/WacomBattleFloatingCombatTextStyleProvider.h"
#include "UI/Battle/WacomBattleFloatingCombatTextTypes.h"

struct UWacomBattleFloatingCombatTextLayerWidget::FActiveEntry
{
	TObjectPtr<UWacomBattleFloatingCombatTextEntryWidget> Widget = nullptr;
	FWacomBattleFloatingCombatTextTarget Target;
	int32 Lane = 0;
	float ElapsedSeconds = 0.0f;
	bool bCritical = false;
};

struct UWacomBattleFloatingCombatTextLayerWidget::FTargetPlaybackState
{
	float NextAdmissionTimeSeconds = 0.0f;
	TSet<int32> OccupiedLanes;
};

struct UWacomBattleFloatingCombatTextLayerWidget::FPlaybackState
{
	TArray<FWacomBattleFloatingCombatTextSpawnRequest> PendingRequests;
	TArray<FActiveEntry> ActiveEntries;
	TMap<FWacomBattleFloatingCombatTextTarget, FTargetPlaybackState> TargetStates;
	float ClockSeconds = 0.0f;
};

UWacomBattleFloatingCombatTextLayerWidget::~UWacomBattleFloatingCombatTextLayerWidget() = default;

void UWacomBattleFloatingCombatTextLayerWidget::FPlaybackStateDeleter::operator()(
	FPlaybackState* State) const
{
	delete State;
}

void UWacomBattleFloatingCombatTextLayerWidget::Enqueue(
	const TArray<FWacomBattleFloatingCombatTextSpawnRequest>& Requests)
{
	if (!Playback)
	{
		Playback.Reset(new FPlaybackState());
	}
	for (const FWacomBattleFloatingCombatTextSpawnRequest& Request : Requests)
	{
		if (Request.Row.Amount != 0)
		{
			Playback->PendingRequests.Add(Request);
		}
	}
	AdmitPending();
	RefreshActive(0.0f);
}

void UWacomBattleFloatingCombatTextLayerWidget::TickPlayback(const float DeltaTime)
{
	if (!IsPlaybackActive())
	{
		return;
	}
	const float SafeDelta = FMath::Max(0.0f, DeltaTime);
	Playback->ClockSeconds += SafeDelta;
	RefreshActive(SafeDelta);
	AdmitPending();
}

void UWacomBattleFloatingCombatTextLayerWidget::ClearPlayback()
{
	if (Playback)
	{
		Playback->PendingRequests.Reset();
		Playback->ActiveEntries.Reset();
		Playback->TargetStates.Reset();
		Playback->ClockSeconds = 0.0f;
	}
	for (UWacomBattleFloatingCombatTextEntryWidget* Entry : OwnedEntries)
	{
		if (Entry)
		{
			Entry->ResetForPool();
		}
	}
}

bool UWacomBattleFloatingCombatTextLayerWidget::IsPlaybackActive() const
{
	return Playback
		&& (!Playback->PendingRequests.IsEmpty() || !Playback->ActiveEntries.IsEmpty());
}

TSharedRef<SWidget> UWacomBattleFloatingCombatTextLayerWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}
		EntryCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("EntryCanvas"));
		EntryCanvas->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = EntryCanvas;
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
	return Super::RebuildWidget();
}

void UWacomBattleFloatingCombatTextLayerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindRuntimeSettings();
}

void UWacomBattleFloatingCombatTextLayerWidget::NativeDestruct()
{
	UnbindRuntimeSettings();
	ClearPlayback();
	Super::NativeDestruct();
}

void UWacomBattleFloatingCombatTextLayerWidget::NativeRefreshFromSnapshot(
	const FBattleSnapshot& Snapshot)
{
	Super::NativeRefreshFromSnapshot(Snapshot);
}

const UWacomBattleFloatingCombatTextStyle&
UWacomBattleFloatingCombatTextLayerWidget::ResolveStyle() const
{
	return StyleOverride
		? *StyleOverride
		: WacomBattleFloatingCombatTextStyleProvider::GetStyle();
}

UWacomBattleFloatingCombatTextEntryWidget*
UWacomBattleFloatingCombatTextLayerWidget::AcquireEntry()
{
	for (UWacomBattleFloatingCombatTextEntryWidget* Entry : OwnedEntries)
	{
		if (Entry && Entry->GetVisibility() == ESlateVisibility::Collapsed)
		{
			return Entry;
		}
	}
	if (!EntryCanvas)
	{
		return nullptr;
	}

	UClass* Class = EntryWidgetClass
		? EntryWidgetClass.Get()
		: UWacomBattleFloatingCombatTextEntryWidget::StaticClass();
	UWacomBattleFloatingCombatTextEntryWidget* Entry =
		CreateWidget<UWacomBattleFloatingCombatTextEntryWidget>(GetOwningPlayer(), Class);
	if (!Entry)
	{
		return nullptr;
	}
	Entry->SetVisibility(ESlateVisibility::Collapsed);
	OwnedEntries.Add(Entry);
	EntryCanvas->AddChildToCanvas(Entry);
	return Entry;
}

void UWacomBattleFloatingCombatTextLayerWidget::ReleaseEntry(
	UWacomBattleFloatingCombatTextEntryWidget& Entry)
{
	Entry.ResetForPool();
}

void UWacomBattleFloatingCombatTextLayerWidget::AdmitPending()
{
	if (!Playback || !EntryCanvas || Playback->PendingRequests.IsEmpty())
	{
		return;
	}
	const UWacomBattleFloatingCombatTextStyle& Style = ResolveStyle();
	const int32 MaxConcurrent = FMath::Max(1, Style.MaxConcurrentPerTarget);
	TSet<FWacomBattleFloatingCombatTextTarget> BlockedTargets;

	for (int32 Index = 0; Index < Playback->PendingRequests.Num();)
	{
		const FWacomBattleFloatingCombatTextSpawnRequest& Request =
			Playback->PendingRequests[Index];
		if (BlockedTargets.Contains(Request.Row.Target))
		{
			++Index;
			continue;
		}

		FTargetPlaybackState& TargetState =
			Playback->TargetStates.FindOrAdd(Request.Row.Target);
		if (TargetState.OccupiedLanes.Num() >= MaxConcurrent
			|| Playback->ClockSeconds + UE_SMALL_NUMBER < TargetState.NextAdmissionTimeSeconds)
		{
			BlockedTargets.Add(Request.Row.Target);
			++Index;
			continue;
		}

		int32 Lane = 0;
		while (TargetState.OccupiedLanes.Contains(Lane) && Lane < MaxConcurrent)
		{
			++Lane;
		}
		UWacomBattleFloatingCombatTextEntryWidget* Entry = AcquireEntry();
		if (!Entry || Lane >= MaxConcurrent)
		{
			BlockedTargets.Add(Request.Row.Target);
			++Index;
			continue;
		}

		Entry->ApplyRow(Request.Row, Style);
		const float CenteredLane = static_cast<float>(Lane) - (MaxConcurrent - 1) * 0.5f;
		FVector2D Position =
			Request.CapturedScreenPosition
			- Style.EntrySize * 0.5f
			+ FVector2D(
				CenteredLane * Style.LaneSpacingPixels,
				-Lane * Style.StackSpacingPixels);
		Position = ClampEntryPosition(Position, Style.EntrySize);
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Entry->Slot))
		{
			CanvasSlot->SetAutoSize(false);
			CanvasSlot->SetSize(Style.EntrySize);
			CanvasSlot->SetPosition(Position);
			CanvasSlot->SetZOrder(Lane);
		}

		FActiveEntry& Active = Playback->ActiveEntries.AddDefaulted_GetRef();
		Active.Widget = Entry;
		Active.Target = Request.Row.Target;
		Active.Lane = Lane;
		Active.bCritical =
			Request.Row.Kind == EWacomBattleFloatingCombatTextKind::CriticalDamage;
		TargetState.OccupiedLanes.Add(Lane);
		TargetState.NextAdmissionTimeSeconds =
			Playback->ClockSeconds + FMath::Max(0.0f, Style.SameTargetStaggerSeconds);
		Playback->PendingRequests.RemoveAt(Index);
	}
}

void UWacomBattleFloatingCombatTextLayerWidget::RefreshActive(const float DeltaTime)
{
	if (!Playback)
	{
		return;
	}
	const UWacomBattleFloatingCombatTextStyle& Style = ResolveStyle();
	const float FadeIn = FMath::Max(0.0f, Style.FadeInSeconds);
	const float Hold = FMath::Max(0.0f, Style.ReadableHoldSeconds);
	const float FadeOut = FMath::Max(0.0f, Style.FadeOutSeconds);
	const float Total = FadeIn + Hold + FadeOut;

	for (int32 Index = Playback->ActiveEntries.Num() - 1; Index >= 0; --Index)
	{
		FActiveEntry& Active = Playback->ActiveEntries[Index];
		Active.ElapsedSeconds += DeltaTime;
		if (!Active.Widget || Active.ElapsedSeconds >= Total)
		{
			if (Active.Widget)
			{
				ReleaseEntry(*Active.Widget);
			}
			if (FTargetPlaybackState* State = Playback->TargetStates.Find(Active.Target))
			{
				State->OccupiedLanes.Remove(Active.Lane);
			}
			Playback->ActiveEntries.RemoveAtSwap(Index);
			continue;
		}

		float Opacity = 1.0f;
		if (FadeIn > UE_SMALL_NUMBER && Active.ElapsedSeconds < FadeIn)
		{
			Opacity = Active.ElapsedSeconds / FadeIn;
		}
		else if (FadeOut > UE_SMALL_NUMBER && Active.ElapsedSeconds > FadeIn + Hold)
		{
			Opacity = 1.0f - (Active.ElapsedSeconds - FadeIn - Hold) / FadeOut;
		}

		float RetireAlpha = 0.0f;
		if (FadeOut > UE_SMALL_NUMBER && Active.ElapsedSeconds > FadeIn + Hold)
		{
			RetireAlpha = FMath::Clamp(
				(Active.ElapsedSeconds - FadeIn - Hold) / FadeOut,
				0.0f,
				1.0f);
		}
		const FVector2D Translation = bReducedMotion
			? FVector2D::ZeroVector
			: FVector2D(0.0f, -Style.DriftDistancePixels * RetireAlpha);
		float Scale = 1.0f;
		if (!bReducedMotion && Active.bCritical && FadeIn > UE_SMALL_NUMBER)
		{
			const float EnterAlpha = FMath::Clamp(Active.ElapsedSeconds / FadeIn, 0.0f, 1.0f);
			Scale = FMath::Lerp(FMath::Max(1.0f, Style.CriticalStartScale), 1.0f, EnterAlpha);
		}
		Active.Widget->ApplyPlaybackFrame(Opacity, Translation, Scale);
	}

	for (auto It = Playback->TargetStates.CreateIterator(); It; ++It)
	{
		if (It.Value().OccupiedLanes.IsEmpty())
		{
			bool bHasPending = false;
			for (const FWacomBattleFloatingCombatTextSpawnRequest& Request :
				Playback->PendingRequests)
			{
				if (Request.Row.Target == It.Key())
				{
					bHasPending = true;
					break;
				}
			}
			if (!bHasPending)
			{
				It.RemoveCurrent();
			}
		}
	}
}

void UWacomBattleFloatingCombatTextLayerWidget::BindRuntimeSettings()
{
	UWacomSettingsSubsystem* Settings = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UWacomSettingsSubsystem>()
		: nullptr;
	if (BoundSettingsSubsystem.Get() == Settings
		&& RuntimeSettingsChangedHandle.IsValid())
	{
		HandleRuntimeSettingsChanged(
			Settings->GetCurrentSnapshot(),
			EWacomRuntimeSettingsChangeReason::Startup);
		return;
	}

	UnbindRuntimeSettings();
	if (!Settings)
	{
		return;
	}
	BoundSettingsSubsystem = Settings;
	RuntimeSettingsChangedHandle = Settings->OnRuntimeSettingsChangedNative().AddUObject(
		this,
		&UWacomBattleFloatingCombatTextLayerWidget::HandleRuntimeSettingsChanged);
	HandleRuntimeSettingsChanged(
		Settings->GetCurrentSnapshot(),
		EWacomRuntimeSettingsChangeReason::Startup);
}

void UWacomBattleFloatingCombatTextLayerWidget::UnbindRuntimeSettings()
{
	if (UWacomSettingsSubsystem* Settings = BoundSettingsSubsystem.Get())
	{
		if (RuntimeSettingsChangedHandle.IsValid())
		{
			Settings->OnRuntimeSettingsChangedNative().Remove(RuntimeSettingsChangedHandle);
		}
	}
	BoundSettingsSubsystem.Reset();
	RuntimeSettingsChangedHandle.Reset();
}

void UWacomBattleFloatingCombatTextLayerWidget::HandleRuntimeSettingsChanged(
	const FWacomLocalSettingsSnapshot& Snapshot,
	EWacomRuntimeSettingsChangeReason /*Reason*/)
{
	bReducedMotion = Snapshot.UIMotionMode == EWacomUIMotionMode::Simplified;
}

FVector2D UWacomBattleFloatingCombatTextLayerWidget::ClampEntryPosition(
	const FVector2D& DesiredPosition,
	const FVector2D& EntrySize) const
{
	const UWacomBattleFloatingCombatTextStyle& Style = ResolveStyle();
	FVector2D ViewportSize = GetCachedGeometry().GetLocalSize();
	if (ViewportSize.X <= 0.0f || ViewportSize.Y <= 0.0f)
	{
		ViewportSize = FVector2D(UWidgetLayoutLibrary::GetViewportSize(this));
		const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
		if (ViewportScale > UE_SMALL_NUMBER)
		{
			ViewportSize /= ViewportScale;
		}
	}
	if (ViewportSize.X <= 0.0f || ViewportSize.Y <= 0.0f)
	{
		ViewportSize = FVector2D(1920.0f, 1080.0f);
	}
	const float SafePadding = FMath::Max(0.0f, Style.ViewportSafePaddingPixels);
	return FVector2D(
		FMath::Clamp(
			DesiredPosition.X,
			SafePadding,
			FMath::Max(SafePadding, ViewportSize.X - EntrySize.X - SafePadding)),
		FMath::Clamp(
			DesiredPosition.Y,
			SafePadding,
			FMath::Max(SafePadding, ViewportSize.Y - EntrySize.Y - SafePadding)));
}
