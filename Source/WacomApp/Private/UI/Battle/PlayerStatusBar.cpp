// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/PlayerStatusBar.h"

#include "UI/Battle/WacomBattlePlayerVitalsPlayback.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Settings/WacomLocalSettingsTypes.h"
#include "Settings/WacomSettingsSubsystem.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/Battle/WacomBattleStatusIconWidget.h"
#include "UI/Common/WacomProgressBar.h"

#define LOCTEXT_NAMESPACE "WacomPlayerStatus"

namespace
{
	const FName HpCurrentPercentParameterName(TEXT("HpCurrentPercent"));
	const FName HpTrailPercentParameterName(TEXT("HpTrailPercent"));
	const FName HpPreviewPercentParameterName(TEXT("HpPreviewPercent"));
	const FName HpPreviewModeParameterName(TEXT("HpPreviewMode"));
	const FName LowHealthAmountParameterName(TEXT("LowHealthAmount"));
	const FName ShieldVisibleParameterName(TEXT("ShieldVisible"));
	const FName ShieldPreviewModeParameterName(TEXT("ShieldPreviewMode"));
	const FName DamagePulseAmountParameterName(TEXT("DamagePulseAmount"));
	const FName ShieldPulseAmountParameterName(TEXT("ShieldPulseAmount"));

	constexpr float PlayerStatusWidth = 680.0f;
	constexpr float PlayerStatusHeight = 86.0f;
	constexpr float VitalsTrackHeight = 46.0f;

	float ResolveHpPercent(const FPlayerSnapshot& Player)
	{
		return Player.MaxHp > 0
			? FMath::Clamp(
				static_cast<float>(Player.CurrentHp) / static_cast<float>(Player.MaxHp),
				0.0f,
				1.0f)
			: 0.0f;
	}

	int32 ResolvePreviewMode(const FPlayerSnapshot& Base, const FPlayerSnapshot& Preview)
	{
		if (Preview.CurrentHp > Base.CurrentHp)
		{
			return 1;
		}
		if (Preview.CurrentHp < Base.CurrentHp)
		{
			return 2;
		}
		return 0;
	}

	int32 ResolveShieldPreviewMode(const FPlayerSnapshot& Base, const FPlayerSnapshot& Preview)
	{
		if (Preview.Shield > Base.Shield)
		{
			return 1;
		}
		if (Preview.Shield < Base.Shield)
		{
			return 2;
		}
		return 0;
	}

	float ResolveLowHealthAmount(const float HpPercent, const float Threshold)
	{
		const float SafeThreshold = FMath::Clamp(Threshold, 0.0f, 1.0f);
		if (SafeThreshold <= KINDA_SMALL_NUMBER || HpPercent >= SafeThreshold)
		{
			return 0.0f;
		}
		const float FullWarningPercent = SafeThreshold * 0.60f;
		return 1.0f - FMath::SmoothStep(FullWarningPercent, SafeThreshold, HpPercent);
	}

	bool AreStatusStacksEquivalent(
		const TMap<FGameplayTag, int32>& Left,
		const TMap<FGameplayTag, int32>& Right)
	{
		if (Left.Num() != Right.Num())
		{
			return false;
		}

		for (const TPair<FGameplayTag, int32>& Pair : Left)
		{
			const int32* RightValue = Right.Find(Pair.Key);
			if (!RightValue || *RightValue != Pair.Value)
			{
				return false;
			}
		}
		return true;
	}

	bool ArePlayerSnapshotsEquivalent(
		const FPlayerSnapshot& Left,
		const FPlayerSnapshot& Right)
	{
		return Left.CurrentHp == Right.CurrentHp
			&& Left.MaxHp == Right.MaxHp
			&& Left.Shield == Right.Shield
			&& Left.Statuses.Num() == Right.Statuses.Num()
			&& Left.Statuses.HasAllExact(Right.Statuses)
			&& Right.Statuses.HasAllExact(Left.Statuses)
			&& AreStatusStacksEquivalent(Left.StatusStacks, Right.StatusStacks);
	}

	void StyleCenteredText(UTextBlock& Text, const int32 FontSize, const FLinearColor& Color)
	{
		FSlateFontInfo Font = Text.GetFont();
		Font.Size = FontSize;
		Font.TypefaceFontName = TEXT("Bold");
		Text.SetFont(Font);
		Text.SetColorAndOpacity(FSlateColor(Color));
		Text.SetJustification(ETextJustify::Center);
		Text.SetShadowOffset(FVector2D(1.0f, 1.0f));
		Text.SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
	}
}

UPlayerStatusBar::~UPlayerStatusBar() = default;

void UPlayerStatusBar::FVitalsPlaybackDeleter::operator()(
	FWacomBattlePlayerVitalsPlayback* Playback) const
{
	delete Playback;
}

TSharedRef<SWidget> UPlayerStatusBar::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		USizeBox* Root = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("PlayerStatusRoot"));
		Root->SetWidthOverride(PlayerStatusWidth);
		Root->SetHeightOverride(PlayerStatusHeight);
		WidgetTree->RootWidget = Root;

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("PlayerStatusColumn"));
		Root->AddChild(Column);

		USizeBox* VitalsBox = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("VitalsSizeBox"));
		VitalsBox->SetWidthOverride(PlayerStatusWidth);
		VitalsBox->SetHeightOverride(VitalsTrackHeight);
		Column->AddChildToVerticalBox(VitalsBox);

		UOverlay* VitalsOverlay = WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), TEXT("VitalsOverlay"));
		VitalsBox->AddChild(VitalsOverlay);

		HpBar = WidgetTree->ConstructWidget<UWacomProgressBar>(
			UWacomProgressBar::StaticClass(), TEXT("HpBar"));
		HpBar->SetFillColor(FLinearColor(0.20f, 0.82f, 0.42f, 1.0f));
		HpBar->SetShowText(false);
		if (UOverlaySlot* HpSlot = VitalsOverlay->AddChildToOverlay(HpBar))
		{
			HpSlot->SetPadding(FMargin(18.0f, 12.0f, 82.0f, 12.0f));
			HpSlot->SetHorizontalAlignment(HAlign_Fill);
			HpSlot->SetVerticalAlignment(VAlign_Fill);
		}

		VitalsTrackImage = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), TEXT("VitalsTrackImage"));
		VitalsTrackImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UOverlaySlot* TrackSlot = VitalsOverlay->AddChildToOverlay(VitalsTrackImage))
		{
			TrackSlot->SetHorizontalAlignment(HAlign_Fill);
			TrackSlot->SetVerticalAlignment(VAlign_Fill);
		}

		HpValueText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("HpValueText"));
		StyleCenteredText(*HpValueText, 22, FLinearColor::White);
		if (UOverlaySlot* HpTextSlot = VitalsOverlay->AddChildToOverlay(HpValueText))
		{
			HpTextSlot->SetPadding(FMargin(72.0f, 0.0f, 100.0f, 0.0f));
			HpTextSlot->SetHorizontalAlignment(HAlign_Fill);
			HpTextSlot->SetVerticalAlignment(VAlign_Center);
		}

		USizeBox* GeneratedShieldRoot = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("ShieldValueRoot"));
		GeneratedShieldRoot->SetWidthOverride(76.0f);
		GeneratedShieldRoot->SetHeightOverride(VitalsTrackHeight);
		ShieldValueRoot = GeneratedShieldRoot;
		if (UOverlaySlot* ShieldRootSlot = VitalsOverlay->AddChildToOverlay(GeneratedShieldRoot))
		{
			ShieldRootSlot->SetHorizontalAlignment(HAlign_Right);
			ShieldRootSlot->SetVerticalAlignment(VAlign_Center);
		}

		ShieldText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("ShieldText"));
		ShieldText->SetText(FText::AsNumber(0));
		StyleCenteredText(*ShieldText, 20, FLinearColor(0.55f, 0.84f, 1.0f, 1.0f));
		GeneratedShieldRoot->AddChild(ShieldText);

		StatusList = WidgetTree->ConstructWidget<UWacomBattleStatusIconListWidget>(
			UWacomBattleStatusIconListWidget::StaticClass(), TEXT("StatusList"));
		if (UVerticalBoxSlot* StatusSlot = Column->AddChildToVerticalBox(StatusList))
		{
			StatusSlot->SetPadding(FMargin(10.0f, 5.0f, 0.0f, 0.0f));
			StatusSlot->SetHorizontalAlignment(HAlign_Left);
			StatusSlot->SetVerticalAlignment(VAlign_Top);
		}
	}

	if (!VitalsPlayback)
	{
		VitalsPlayback.Reset(new FWacomBattlePlayerVitalsPlayback());
	}
	return Super::RebuildWidget();
}

void UPlayerStatusBar::NativeConstruct()
{
	Super::NativeConstruct();
	if (!VitalsPlayback)
	{
		VitalsPlayback.Reset(new FWacomBattlePlayerVitalsPlayback());
	}
	CaptureShieldAuthoredTransform();
	EnsureVitalsMaterial();
	BindRuntimeSettings();
	RefreshDisplay();
}

void UPlayerStatusBar::NativeRefreshFromSnapshot(const FBattleSnapshot& Snap)
{
	BasePlayerView = Snap.Player;
	bHasBasePlayerView = true;
	if (!VitalsPlayback)
	{
		VitalsPlayback.Reset(new FWacomBattlePlayerVitalsPlayback());
	}
	VitalsPlayback->SetAuthoritativeHpPercent(ResolveHpPercent(BasePlayerView));
	RefreshDisplay();
}

void UPlayerStatusBar::SetActionPreview(const FPlayerSnapshot& ProjectedPlayer)
{
	if (bHasActionPreview && ArePlayerSnapshotsEquivalent(ActionPreviewPlayerView, ProjectedPlayer))
	{
		return;
	}

	ActionPreviewPlayerView = ProjectedPlayer;
	bHasActionPreview = true;
	RefreshDisplay();
}

void UPlayerStatusBar::ClearActionPreview()
{
	if (!bHasActionPreview)
	{
		return;
	}

	bHasActionPreview = false;
	RefreshDisplay();
}

void UPlayerStatusBar::PlayEnemyActionImpactFeedback(
	const FPlayerSnapshot& PreviousPlayer,
	const FPlayerSnapshot& CurrentPlayer)
{
	if (!VitalsPlayback)
	{
		VitalsPlayback.Reset(new FWacomBattlePlayerVitalsPlayback());
	}
	FWacomBattlePlayerVitalsPlaybackConfig Config;
	Config.DamageTrailHoldSeconds = FMath::Max(0.0f, DamageTrailHoldSeconds);
	Config.DamageTrailRecoverySeconds = FMath::Max(0.0f, DamageTrailRecoverySeconds);
	Config.ImpactDurationSeconds = FMath::Max(0.0f, ImpactFeedbackDurationSeconds);
	Config.ShieldCompressionScale = FMath::Max(0.0f, ShieldCompressionScale);
	Config.ShieldReboundScale = FMath::Max(0.0f, ShieldReboundScale);
	Config.bReducedMotion = bRuntimeSimplifiedMotion;
	VitalsPlayback->BeginImpact(
		PreviousPlayer.CurrentHp,
		PreviousPlayer.MaxHp,
		CurrentPlayer.CurrentHp,
		CurrentPlayer.MaxHp,
		PreviousPlayer.Shield,
		CurrentPlayer.Shield,
		Config);
	ApplyVitalsPresentation();

	const bool bHpLost = CurrentPlayer.CurrentHp < PreviousPlayer.CurrentHp;
	const bool bShieldLost = CurrentPlayer.Shield < PreviousPlayer.Shield;
	USoundBase* ImpactSound = bHpLost ? DamageImpactSound.Get()
		: (bShieldLost ? ShieldImpactSound.Get() : nullptr);
	if (ImpactSound)
	{
		UGameplayStatics::PlaySound2D(this, ImpactSound, FMath::Max(0.0f, ImpactSoundVolume));
	}
}

void UPlayerStatusBar::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!VitalsPlayback)
	{
		return;
	}

	FWacomBattlePlayerVitalsPlaybackConfig Config;
	Config.DamageTrailHoldSeconds = FMath::Max(0.0f, DamageTrailHoldSeconds);
	Config.DamageTrailRecoverySeconds = FMath::Max(0.0f, DamageTrailRecoverySeconds);
	Config.ImpactDurationSeconds = FMath::Max(0.0f, ImpactFeedbackDurationSeconds);
	Config.ShieldCompressionScale = FMath::Max(0.0f, ShieldCompressionScale);
	Config.ShieldReboundScale = FMath::Max(0.0f, ShieldReboundScale);
	Config.bReducedMotion = bRuntimeSimplifiedMotion;
	if (VitalsPlayback->View(Config).bActive)
	{
		VitalsPlayback->Tick(InDeltaTime, Config);
		ApplyVitalsPresentation();
	}
}

void UPlayerStatusBar::NativeDestruct()
{
	UnbindRuntimeSettings();
	bHasActionPreview = false;
	if (VitalsPlayback)
	{
		VitalsPlayback->Reset(bHasBasePlayerView ? ResolveHpPercent(BasePlayerView) : 0.0f);
	}
	RestoreShieldAuthoredTransform();
	RestoreVitalsMaterial();
	Super::NativeDestruct();
}

void UPlayerStatusBar::RefreshDisplay()
{
	if (bHasActionPreview)
	{
		RefreshFromPlayerSnapshot(ActionPreviewPlayerView);
	}
	else if (bHasBasePlayerView)
	{
		RefreshFromPlayerSnapshot(BasePlayerView);
	}
	ApplyVitalsPresentation();
}

void UPlayerStatusBar::RefreshFromPlayerSnapshot(const FPlayerSnapshot& PlayerView)
{
	if (HpBar)
	{
		HpBar->SetValue(PlayerView.CurrentHp, PlayerView.MaxHp);
		HpBar->SetFillColor(ResolveLowHealthAmount(
			ResolveHpPercent(PlayerView), LowHealthThreshold) > 0.0f
			? FLinearColor(0.92f, 0.16f, 0.10f, 1.0f)
			: FLinearColor(0.20f, 0.82f, 0.42f, 1.0f));
	}

	if (HpValueText)
	{
		HpValueText->SetText(FText::Format(
			LOCTEXT("HpFmt", "{0} / {1}"),
			FFormatOrderedArguments{
				FFormatArgumentValue(PlayerView.CurrentHp),
				FFormatArgumentValue(PlayerView.MaxHp) }));
	}

	if (ShieldText)
	{
		ShieldText->SetText(FText::AsNumber(PlayerView.Shield));
	}

	if (UWacomBattleStatusIconListWidget* ResolvedStatusList = ResolveStatusListWidget())
	{
		ResolvedStatusList->SetStatuses(PlayerView.Statuses, PlayerView.StatusStacks);
	}
}

bool UPlayerStatusBar::EnsureVitalsMaterial()
{
	if (!VitalsTrackImage)
	{
		return false;
	}
	if (VitalsMaterialInstance)
	{
		return true;
	}

	UMaterialInterface* Source = Cast<UMaterialInterface>(
		VitalsTrackImage->GetBrush().GetResourceObject());
	if (!Source)
	{
		return false;
	}
	if (UMaterialInstanceDynamic* ExistingDynamic = Cast<UMaterialInstanceDynamic>(Source))
	{
		VitalsMaterialInstance = ExistingDynamic;
		return true;
	}

	VitalsSourceMaterial = Source;
	VitalsMaterialInstance = VitalsTrackImage->GetDynamicMaterial();
	return VitalsMaterialInstance != nullptr;
}

void UPlayerStatusBar::ApplyVitalsPresentation()
{
	if (!VitalsPlayback)
	{
		return;
	}

	FWacomBattlePlayerVitalsPlaybackConfig Config;
	Config.DamageTrailHoldSeconds = FMath::Max(0.0f, DamageTrailHoldSeconds);
	Config.DamageTrailRecoverySeconds = FMath::Max(0.0f, DamageTrailRecoverySeconds);
	Config.ImpactDurationSeconds = FMath::Max(0.0f, ImpactFeedbackDurationSeconds);
	Config.ShieldCompressionScale = FMath::Max(0.0f, ShieldCompressionScale);
	Config.ShieldReboundScale = FMath::Max(0.0f, ShieldReboundScale);
	Config.bReducedMotion = bRuntimeSimplifiedMotion;
	const FWacomBattlePlayerVitalsPlaybackView PlaybackView = VitalsPlayback->View(Config);

	const FPlayerSnapshot& DisplayPlayer = bHasActionPreview
		? ActionPreviewPlayerView
		: BasePlayerView;
	const float BaseHpPercent = bHasBasePlayerView ? ResolveHpPercent(BasePlayerView) : 0.0f;
	const float DisplayHpPercent = ResolveHpPercent(DisplayPlayer);
	const int32 PreviewMode = bHasActionPreview && bHasBasePlayerView
		? ResolvePreviewMode(BasePlayerView, ActionPreviewPlayerView)
		: 0;
	const int32 ShieldPreviewMode = bHasActionPreview && bHasBasePlayerView
		? ResolveShieldPreviewMode(BasePlayerView, ActionPreviewPlayerView)
		: 0;

	if (EnsureVitalsMaterial())
	{
		VitalsMaterialInstance->SetScalarParameterValue(HpCurrentPercentParameterName, BaseHpPercent);
		VitalsMaterialInstance->SetScalarParameterValue(
			HpTrailPercentParameterName,
			FMath::Max(BaseHpPercent, PlaybackView.DamageTrailPercent));
		VitalsMaterialInstance->SetScalarParameterValue(HpPreviewPercentParameterName, DisplayHpPercent);
		VitalsMaterialInstance->SetScalarParameterValue(HpPreviewModeParameterName, PreviewMode);
		VitalsMaterialInstance->SetScalarParameterValue(
			LowHealthAmountParameterName,
			ResolveLowHealthAmount(DisplayHpPercent, LowHealthThreshold));
		VitalsMaterialInstance->SetScalarParameterValue(
			DamagePulseAmountParameterName,
			PlaybackView.DamagePulseAmount * RuntimeFlashIntensity);
		VitalsMaterialInstance->SetScalarParameterValue(
			ShieldPulseAmountParameterName,
			PlaybackView.ShieldPulseAmount * RuntimeFlashIntensity);
		VitalsMaterialInstance->SetScalarParameterValue(
			ShieldPreviewModeParameterName,
			ShieldPreviewMode);
	}

	const bool bShieldVisible = !bHideShieldWhenZero
		|| (bHasBasePlayerView && BasePlayerView.Shield > 0)
		|| (bHasActionPreview && ActionPreviewPlayerView.Shield > 0)
		|| PlaybackView.bKeepBrokenShieldVisible;
	if (VitalsMaterialInstance)
	{
		VitalsMaterialInstance->SetScalarParameterValue(
			ShieldVisibleParameterName, bShieldVisible ? 1.0f : 0.0f);
	}

	UWidget* ShieldTransformTarget = ShieldValueRoot
		? ShieldValueRoot.Get()
		: ShieldText.Get();
	if (ShieldTransformTarget)
	{
		CaptureShieldAuthoredTransform();
		FWidgetTransform Applied = ShieldAuthoredTransform;
		Applied.Scale *= PlaybackView.ShieldScale;
		ShieldTransformTarget->SetRenderTransform(Applied);
		ShieldTransformTarget->SetVisibility(
			bShieldVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	if (ShieldText)
	{
		const FLinearColor ShieldColor = ShieldPreviewMode == 1
			? FLinearColor(0.96f, 0.82f, 0.42f, 1.0f)
			: (ShieldPreviewMode == 2
				? FLinearColor(0.90f, 0.32f, 0.62f, 1.0f)
				: FLinearColor(0.55f, 0.84f, 1.0f, 1.0f));
		ShieldText->SetColorAndOpacity(FSlateColor(ShieldColor));
		ShieldText->SetVisibility(
			bShieldVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	if (HpBar)
	{
		HpBar->SetVisibility(VitalsMaterialInstance
			? ESlateVisibility::Hidden
			: ESlateVisibility::HitTestInvisible);
	}
}

void UPlayerStatusBar::RestoreVitalsMaterial()
{
	if (VitalsTrackImage && VitalsSourceMaterial)
	{
		VitalsTrackImage->SetBrushFromMaterial(VitalsSourceMaterial);
	}
	VitalsMaterialInstance = nullptr;
	VitalsSourceMaterial = nullptr;
}

void UPlayerStatusBar::CaptureShieldAuthoredTransform()
{
	if (bCapturedShieldAuthoredTransform)
	{
		return;
	}
	UWidget* Target = ShieldValueRoot ? ShieldValueRoot.Get() : ShieldText.Get();
	if (!Target)
	{
		return;
	}
	ShieldAuthoredTransform = Target->GetRenderTransform();
	ShieldAuthoredPivot = Target->GetRenderTransformPivot();
	bCapturedShieldAuthoredTransform = true;
}

void UPlayerStatusBar::RestoreShieldAuthoredTransform()
{
	if (!bCapturedShieldAuthoredTransform)
	{
		return;
	}
	UWidget* Target = ShieldValueRoot ? ShieldValueRoot.Get() : ShieldText.Get();
	if (Target)
	{
		Target->SetRenderTransform(ShieldAuthoredTransform);
		Target->SetRenderTransformPivot(ShieldAuthoredPivot);
	}
}

void UPlayerStatusBar::BindRuntimeSettings()
{
	UWacomSettingsSubsystem* Settings = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UWacomSettingsSubsystem>()
		: nullptr;
	if (BoundSettingsSubsystem.Get() == Settings && RuntimeSettingsChangedHandle.IsValid())
	{
		HandleRuntimeSettingsChanged(
			Settings->GetCurrentSnapshot(), EWacomRuntimeSettingsChangeReason::Startup);
		return;
	}

	UnbindRuntimeSettings();
	if (!Settings)
	{
		return;
	}
	BoundSettingsSubsystem = Settings;
	RuntimeSettingsChangedHandle = Settings->OnRuntimeSettingsChangedNative().AddUObject(
		this, &UPlayerStatusBar::HandleRuntimeSettingsChanged);
	HandleRuntimeSettingsChanged(
		Settings->GetCurrentSnapshot(), EWacomRuntimeSettingsChangeReason::Startup);
}

void UPlayerStatusBar::UnbindRuntimeSettings()
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

void UPlayerStatusBar::HandleRuntimeSettingsChanged(
	const FWacomLocalSettingsSnapshot& Snapshot,
	EWacomRuntimeSettingsChangeReason /*Reason*/)
{
	const bool bNextSimplified = Snapshot.UIMotionMode == EWacomUIMotionMode::Simplified;
	if (bRuntimeSimplifiedMotion != bNextSimplified && bNextSimplified && VitalsPlayback)
	{
		VitalsPlayback->Reset(bHasBasePlayerView ? ResolveHpPercent(BasePlayerView) : 0.0f);
	}
	bRuntimeSimplifiedMotion = bNextSimplified;
	switch (Snapshot.FlashEffectMode)
	{
	case EWacomFlashEffectMode::Reduced:
		RuntimeFlashIntensity = 0.35f;
		break;
	case EWacomFlashEffectMode::Off:
		RuntimeFlashIntensity = 0.0f;
		break;
	default:
		RuntimeFlashIntensity = 1.0f;
		break;
	}
	ApplyVitalsPresentation();
}

FWacomPlayerStatusBarAutomationTestView UPlayerStatusBar::BuildAutomationTestView() const
{
	FWacomPlayerStatusBarAutomationTestView Result;
	if (!VitalsPlayback)
	{
		return Result;
	}

	FWacomBattlePlayerVitalsPlaybackConfig Config;
	Config.DamageTrailHoldSeconds = FMath::Max(0.0f, DamageTrailHoldSeconds);
	Config.DamageTrailRecoverySeconds = FMath::Max(0.0f, DamageTrailRecoverySeconds);
	Config.ImpactDurationSeconds = FMath::Max(0.0f, ImpactFeedbackDurationSeconds);
	Config.ShieldCompressionScale = FMath::Max(0.0f, ShieldCompressionScale);
	Config.ShieldReboundScale = FMath::Max(0.0f, ShieldReboundScale);
	Config.bReducedMotion = bRuntimeSimplifiedMotion;
	const FWacomBattlePlayerVitalsPlaybackView PlaybackView = VitalsPlayback->View(Config);
	const FPlayerSnapshot& DisplayPlayer = bHasActionPreview
		? ActionPreviewPlayerView
		: BasePlayerView;

	Result.CurrentHpPercent = bHasBasePlayerView ? ResolveHpPercent(BasePlayerView) : 0.0f;
	Result.DamageTrailPercent = PlaybackView.DamageTrailPercent;
	Result.PreviewHpPercent = ResolveHpPercent(DisplayPlayer);
	Result.LowHealthAmount = ResolveLowHealthAmount(Result.PreviewHpPercent, LowHealthThreshold);
	Result.DamagePulseAmount = PlaybackView.DamagePulseAmount;
	Result.ShieldPulseAmount = PlaybackView.ShieldPulseAmount;
	Result.ShieldScale = PlaybackView.ShieldScale;
	Result.DisplayHp = DisplayPlayer.CurrentHp;
	Result.DisplayShield = DisplayPlayer.Shield;
	Result.PreviewMode = bHasActionPreview && bHasBasePlayerView
		? ResolvePreviewMode(BasePlayerView, ActionPreviewPlayerView)
		: 0;
	Result.bHasActionPreview = bHasActionPreview;
	Result.bShieldVisible = !bHideShieldWhenZero
		|| (bHasBasePlayerView && BasePlayerView.Shield > 0)
		|| (bHasActionPreview && ActionPreviewPlayerView.Shield > 0)
		|| PlaybackView.bKeepBrokenShieldVisible;
	Result.bPlaybackActive = PlaybackView.bActive;
	Result.bReducedMotion = bRuntimeSimplifiedMotion;
	return Result;
}

UWacomBattleStatusIconListWidget* UPlayerStatusBar::ResolveStatusListWidget()
{
	if (StatusList)
	{
		return StatusList;
	}

	if (!WidgetTree)
	{
		return nullptr;
	}

	StatusList = Cast<UWacomBattleStatusIconListWidget>(WidgetTree->FindWidget(TEXT("StatusList")));
	if (StatusList)
	{
		return StatusList;
	}

	UWacomBattleStatusIconListWidget* UniqueStatusList = nullptr;
	bool bFoundMultipleStatusLists = false;
	WidgetTree->ForEachWidget([&UniqueStatusList, &bFoundMultipleStatusLists](UWidget* Widget)
	{
		UWacomBattleStatusIconListWidget* Candidate = Cast<UWacomBattleStatusIconListWidget>(Widget);
		if (!Candidate)
		{
			return;
		}

		if (!UniqueStatusList)
		{
			UniqueStatusList = Candidate;
			return;
		}

		bFoundMultipleStatusLists = true;
	});

	if (!bFoundMultipleStatusLists)
	{
		StatusList = UniqueStatusList;
	}
	return StatusList;
}

#undef LOCTEXT_NAMESPACE
