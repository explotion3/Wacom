// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardEffectBadgeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PaperSprite.h"
#include "UI/Card/WacomPaperSpriteAtlasUtils.h"

namespace
{
	bool AreTextViewsEquivalent(const FText& A, const FText& B)
	{
		return A.EqualTo(B);
	}

	bool AreEffectBadgeDataEquivalent(
		const FWacomCardViewEffectBadge& A,
		const FWacomCardViewEffectBadge& B)
	{
		return A.PresentationKey == B.PresentationKey
			&& A.Kind == B.Kind
			&& A.Value == B.Value
			&& A.bHasPreviewValue == B.bHasPreviewValue
			&& A.PreviewValue == B.PreviewValue
			&& A.bPreviewSkipped == B.bPreviewSkipped
			&& AreTextViewsEquivalent(A.DisplayText, B.DisplayText);
	}

	const FName OldBadgeDigitTextureParameterName(TEXT("OldBadgeDigitTexture"));
	const FName NewBadgeDigitTextureParameterName(TEXT("NewBadgeDigitTexture"));
	const FName OldBadgeDigitUVRectParameterName(TEXT("OldBadgeDigitUVRect"));
	const FName NewBadgeDigitUVRectParameterName(TEXT("NewBadgeDigitUVRect"));
	const FName BadgeOldDissolveParameterName(TEXT("BadgeOldDissolve"));
	const FName BadgeNewRevealParameterName(TEXT("BadgeNewReveal"));
	const FName BadgeFeedbackToneParameterName(TEXT("BadgeFeedbackTone"));
	const FName BadgeFeedbackSeedParameterName(TEXT("BadgeFeedbackSeed"));
	const FName BadgeReducedMotionParameterName(TEXT("BadgeReducedMotion"));
	const FName BadgeEffectModeParameterName(TEXT("BadgeEffectMode"));
	const FName BadgePreviewPulseParameterName(TEXT("BadgePreviewPulse"));
}

#if WITH_AUTOMATION_TESTS
FWacomCardEffectBadgeAutomationTestView
UWacomCardEffectBadgeWidget::GetAutomationTestViewForTest() const
{
	FWacomCardEffectBadgeAutomationTestView View;
	View.ApplyCount = ApplyCountForTest;
	View.DigitImageUpdateCount = DigitImageUpdateCountForTest;
	View.bFeedbackMaterialActive = bFeedbackMaterialActive;
	View.bFeedbackMaterialConfigured =
		FeedbackConfig.Style.DigitFeedbackMaterialInstance != nullptr;
	View.bPreviewSkipped = CurrentData.bPreviewSkipped;
	View.PreviewAmount = PreviewAmount;
	View.ResolvedDigitSpriteCount = ResolvedDigitSprites.Num();
	View.ActiveDigitMaterialInstanceCount = ActiveDigitMaterialInstances.Num();
	View.LastFeedbackMaterialFailure = LastFeedbackMaterialFailureForTest;
	View.DigitMaterialPoolSize = ActiveDigitMaterialInstances.Num();
	View.DigitMaterialCreateCount = DigitMaterialCreateCountForTest;
	View.SpriteSynchronousFallbackCount = SpriteSynchronousFallbackCountForTest;
	View.RootScale = GetRenderTransform().Scale;
	View.RootOpacity = GetRenderOpacity();
	View.bHasFrameShadowImage = BadgeFrameShadowImage != nullptr;
	View.bFrameShadowVisible = BadgeFrameShadowImage
		&& BadgeFrameShadowImage->GetVisibility() != ESlateVisibility::Collapsed;
	View.FrameShadowOffsetPixels = BadgeFrameShadowImage
		? BadgeFrameShadowImage->GetRenderTransform().Translation
		: FVector2D::ZeroVector;
	View.FrameShadowColor = BadgeFrameShadowImage
		? BadgeFrameShadowImage->GetColorAndOpacity()
		: FLinearColor::Transparent;
	return View;
}
#endif

TSharedRef<SWidget> UWacomCardEffectBadgeWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_EffectBadge"));
		}

		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("BadgeRoot"));
		WidgetTree->RootWidget = Root;

		BadgeFrameImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BadgeFrameImage"));
		BadgeFrameImage->SetVisibility(ESlateVisibility::Collapsed);
		if (UOverlaySlot* FrameSlot = Root->AddChildToOverlay(BadgeFrameImage))
		{
			FrameSlot->SetHorizontalAlignment(HAlign_Fill);
			FrameSlot->SetVerticalAlignment(VAlign_Fill);
		}

		DigitHost = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("DigitHost"));
		DigitHost->SetVisibility(ESlateVisibility::Collapsed);
		if (UOverlaySlot* DigitSlot = Root->AddChildToOverlay(DigitHost))
		{
			DigitSlot->SetHorizontalAlignment(HAlign_Center);
			DigitSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	return Super::RebuildWidget();
}

void UWacomCardEffectBadgeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureBadgeFrameShadowImage();
	bHasAppliedData = false;
	ApplyCurrentDataToWidgets();
}

void UWacomCardEffectBadgeWidget::NativeDestruct()
{
	ResetEffectBadgeFeedback();
	ResetAttachmentCastShadowView();
	ReleaseDigitMaterialPool();
	Super::NativeDestruct();
}

void UWacomCardEffectBadgeWidget::SetEffectBadgeData(const FWacomCardViewEffectBadge& InData)
{
	if (bHasAppliedData && AreEffectBadgeDataEquivalent(CurrentData, InData))
	{
		return;
	}

	CurrentData = InData;
	ApplyCurrentDataToWidgets();
}

void UWacomCardEffectBadgeWidget::AppendPresentationSoftObjectPaths(
	TArray<FSoftObjectPath>& OutPaths) const
{
	for (const TPair<EWacomCardViewEffectBadgeKind, TSoftObjectPtr<UPaperSprite>>& Pair : BadgeFrameSprites)
	{
		if (!Pair.Value.IsNull())
		{
			OutPaths.Add(Pair.Value.ToSoftObjectPath());
		}
	}
	for (const TPair<int32, TSoftObjectPtr<UPaperSprite>>& Pair : DigitSprites)
	{
		if (!Pair.Value.IsNull())
		{
			OutPaths.Add(Pair.Value.ToSoftObjectPath());
		}
	}
}

FText UWacomCardEffectBadgeWidget::GetValueText() const
{
	return FText::AsNumber(CurrentData.Value);
}

void UWacomCardEffectBadgeWidget::SetEffectBadgeFeedbackConfig(
	const FWacomFirstPersonCardEffectBadgeFeedbackConfig& InConfig)
{
	if (ActiveDigitMaterialSource
		&& ActiveDigitMaterialSource != InConfig.Style.DigitFeedbackMaterialInstance)
	{
		ReleaseDigitMaterialPool();
	}
	FeedbackConfig = InConfig;
	PrimeDigitMaterialPool();
	if (!FeedbackConfig.bEnabled)
	{
		ResetEffectBadgeFeedback();
	}
}

void UWacomCardEffectBadgeWidget::SetEffectBadgeFeedbackView(
	const FWacomFirstPersonCardEffectBadgeFeedbackItemView& InView)
{
	if (InView.PresentationKey != CurrentData.PresentationKey)
	{
		return;
	}
	FeedbackView = InView;
	if (!InView.bActive)
	{
		if (InView.bPrepareMaterial)
		{
			ApplyDigitMaterial(
				InView.OldValue,
				InView.NewValue,
				0.0f,
				0.0f,
				static_cast<float>(InView.Direction),
				static_cast<float>(InView.Seed & 0xFFFF) / 65535.0f,
				FeedbackConfig.bReducedMotion,
				2.0f,
				0.0f);
			CacheAuthoredRootTransform();
			RestoreAuthoredRootTransform();
			SetRenderOpacity(1.0f);
			return;
		}
		if (bFeedbackMaterialActive)
		{
			RestoreAuthoritativeDigitBrushes();
		}
		RestoreAuthoredRootTransform();
		return;
	}

	ApplyDigitMaterial(
		InView.OldValue,
		InView.NewValue,
		InView.OldDissolveAmount,
		InView.NewRevealAmount,
		static_cast<float>(InView.Direction),
		static_cast<float>(InView.Seed & 0xFFFF) / 65535.0f,
		FeedbackConfig.bReducedMotion,
		2.0f,
		0.0f);
	CacheAuthoredRootTransform();
	FWidgetTransform Transform = AuthoredRootTransform;
	if (!FeedbackConfig.bReducedMotion)
	{
		Transform.Scale.X *= InView.RootScale;
		Transform.Scale.Y *= InView.RootScale;
	}
	SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	SetRenderTransform(Transform);
	SetRenderOpacity(FMath::Clamp(InView.RootOpacity, 0.0f, 1.0f));
}

void UWacomCardEffectBadgeWidget::ResetEffectBadgeFeedback()
{
	FeedbackView = FWacomFirstPersonCardEffectBadgeFeedbackItemView();
	PreviewAmount = 0.0f;
	PreviewElapsedSeconds = 0.0f;
	bPreviewDigitStateDirty = false;
	RestoreAuthoritativeDigitBrushes();
	RestoreAuthoredRootTransform();
	SetRenderOpacity(1.0f);
}

bool UWacomCardEffectBadgeWidget::IsEffectBadgeFeedbackMaterialReady() const
{
	if (!bFeedbackMaterialActive || !DigitHost || ActiveDigitMaterialInstances.IsEmpty())
	{
		return false;
	}
	const int32 VisibleDigitCount = FMath::Min(
		DigitHost->GetChildrenCount(),
		ActiveDigitMaterialInstances.Num());
	if (VisibleDigitCount <= 0)
	{
		return false;
	}
	for (int32 Index = 0; Index < VisibleDigitCount; ++Index)
	{
		const UImage* DigitImage = Cast<UImage>(DigitHost->GetChildAt(Index));
		const UMaterialInstanceDynamic* Material = ActiveDigitMaterialInstances[Index];
		if (!DigitImage || !Material || DigitImage->GetBrush().GetResourceObject() != Material)
		{
			return false;
		}
	}
	return true;
}

void UWacomCardEffectBadgeWidget::SetAttachmentCastShadowView(
	const FWacomCardSurfacePerspectiveView& InView)
{
	AttachmentCastShadowView = InView;
	RefreshBadgeFrameShadow();
}

void UWacomCardEffectBadgeWidget::ResetAttachmentCastShadowView()
{
	AttachmentCastShadowView = FWacomCardSurfacePerspectiveView();
	if (BadgeFrameShadowImage)
	{
		BadgeFrameShadowImage->SetRenderTransform(FWidgetTransform());
		BadgeFrameShadowImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		BadgeFrameShadowImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UWacomCardEffectBadgeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	ApplyPreviewState(InDeltaTime);
}

void UWacomCardEffectBadgeWidget::ApplyCurrentDataToWidgets()
{
#if WITH_AUTOMATION_TESTS
	++ApplyCountForTest;
#endif

	EnsureSpriteCachesBuilt();
	UpdateFrameImage();
	UpdateDigitImages();
	bPreviewDigitStateDirty = true;
	bHasAppliedData = true;
}

void UWacomCardEffectBadgeWidget::ApplyPreviewState(float DeltaTime)
{
	if (FeedbackView.bActive || !FeedbackConfig.bEnabled)
	{
		return;
	}
	const bool bWantsPreview = CurrentData.bHasPreviewValue || CurrentData.bPreviewSkipped;
	const float Duration = bWantsPreview
		? FMath::Max(KINDA_SMALL_NUMBER, FeedbackConfig.Style.PreviewEnterSeconds)
		: FMath::Max(KINDA_SMALL_NUMBER, FeedbackConfig.Style.PreviewExitSeconds);
	const float Target = bWantsPreview ? 1.0f : 0.0f;
	const float PreviousAmount = PreviewAmount;
	PreviewAmount = FMath::FInterpConstantTo(
		PreviewAmount,
		Target,
		FMath::Max(0.0f, DeltaTime),
		1.0f / Duration);
	PreviewElapsedSeconds += FMath::Max(0.0f, DeltaTime);
	const float PulsePeriod = FMath::Max(0.01f, FeedbackConfig.Style.PreviewPulsePeriodSeconds);
	const float Pulse = FeedbackConfig.bReducedMotion
		? 0.5f
		: 0.5f + 0.5f * FMath::Sin(PreviewElapsedSeconds * 2.0f * PI / PulsePeriod);

	if (CurrentData.bPreviewSkipped && bFeedbackMaterialActive)
	{
		RestoreAuthoritativeDigitBrushes();
	}
	if (CurrentData.bHasPreviewValue
		&& !CurrentData.bPreviewSkipped
		&& PreviewAmount > KINDA_SMALL_NUMBER)
	{
		ApplyDigitMaterial(
			CurrentData.Value,
			CurrentData.PreviewValue,
			PreviewAmount,
			PreviewAmount,
			CurrentData.PreviewValue >= CurrentData.Value ? 1.0f : 2.0f,
			static_cast<float>(GetTypeHash(CurrentData.PresentationKey) & 0xFFFFu) / 65535.0f,
			FeedbackConfig.bReducedMotion,
			1.0f,
			Pulse);
	}
	else if (PreviousAmount > KINDA_SMALL_NUMBER && PreviewAmount <= KINDA_SMALL_NUMBER)
	{
		RestoreAuthoritativeDigitBrushes();
	}
	SetRenderOpacity(CurrentData.bPreviewSkipped
		? FMath::Lerp(1.0f, FeedbackConfig.Style.SkippedOpacity, PreviewAmount)
		: 1.0f);
	if (!FMath::IsNearlyEqual(PreviousAmount, PreviewAmount) || CurrentData.bPreviewSkipped)
	{
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

bool UWacomCardEffectBadgeWidget::ApplyDigitMaterial(
	int32 OldValue,
	int32 NewValue,
	float OldDissolveAmount,
	float NewRevealAmount,
	float Tone,
	float Seed,
	bool bReducedMotion,
	float EffectMode,
	float Pulse)
{
#if WITH_AUTOMATION_TESTS
	LastFeedbackMaterialFailureForTest = 0;
#endif
	if (!DigitHost || !FeedbackConfig.Style.DigitFeedbackMaterialInstance)
	{
#if WITH_AUTOMATION_TESTS
		LastFeedbackMaterialFailureForTest = 1;
#endif
		return false;
	}
	EnsureSpriteCachesBuilt();
	const TArray<int32> OldDigits = SplitIntoDigits(OldValue);
	const TArray<int32> NewDigits = SplitIntoDigits(NewValue);
	const int32 DigitCount = FMath::Max(OldDigits.Num(), NewDigits.Num());
	if (DigitCount <= 0 || OldDigits.Num() != NewDigits.Num())
	{
#if WITH_AUTOMATION_TESTS
		LastFeedbackMaterialFailureForTest = 2;
#endif
		return false;
	}

	if (ActiveDigitMaterialSource != FeedbackConfig.Style.DigitFeedbackMaterialInstance)
	{
		ActiveDigitMaterialSource = FeedbackConfig.Style.DigitFeedbackMaterialInstance;
		ActiveDigitMaterialInstances.Reset();
	}
	while (ActiveDigitMaterialInstances.Num() < DigitCount)
	{
		ActiveDigitMaterialInstances.Add(UMaterialInstanceDynamic::Create(
			FeedbackConfig.Style.DigitFeedbackMaterialInstance,
			this));
#if WITH_AUTOMATION_TESTS
		++DigitMaterialCreateCountForTest;
#endif
	}

	for (int32 Index = 0; Index < DigitCount; ++Index)
	{
		UPaperSprite* OldSprite = ResolvedDigitSprites.FindRef(OldDigits[Index]);
		UPaperSprite* NewSprite = ResolvedDigitSprites.FindRef(NewDigits[Index]);
		UImage* DigitImage = Cast<UImage>(DigitHost->GetChildAt(Index));
		UMaterialInstanceDynamic* Material = ActiveDigitMaterialInstances.IsValidIndex(Index)
			? ActiveDigitMaterialInstances[Index]
			: nullptr;
		if (!OldSprite || !NewSprite || !DigitImage || !Material)
		{
#if WITH_AUTOMATION_TESTS
			LastFeedbackMaterialFailureForTest = 3;
#endif
			RestoreAuthoritativeDigitBrushes();
			return false;
		}
		FWacomPaperSpriteAtlasView OldAtlas;
		FWacomPaperSpriteAtlasView NewAtlas;
		if (!WacomPaperSpriteAtlas::Resolve(OldSprite, OldAtlas)
			|| !WacomPaperSpriteAtlas::Resolve(NewSprite, NewAtlas))
		{
#if WITH_AUTOMATION_TESTS
			LastFeedbackMaterialFailureForTest = 4;
#endif
			RestoreAuthoritativeDigitBrushes();
			return false;
		}

		Material->SetTextureParameterValue(OldBadgeDigitTextureParameterName, OldAtlas.Texture);
		Material->SetTextureParameterValue(NewBadgeDigitTextureParameterName, NewAtlas.Texture);
		Material->SetVectorParameterValue(
			OldBadgeDigitUVRectParameterName,
			FLinearColor(OldAtlas.StartUV.X, OldAtlas.StartUV.Y, OldAtlas.SizeUV.X, OldAtlas.SizeUV.Y));
		Material->SetVectorParameterValue(
			NewBadgeDigitUVRectParameterName,
			FLinearColor(NewAtlas.StartUV.X, NewAtlas.StartUV.Y, NewAtlas.SizeUV.X, NewAtlas.SizeUV.Y));
		Material->SetScalarParameterValue(BadgeOldDissolveParameterName, FMath::Clamp(OldDissolveAmount, 0.0f, 1.0f));
		Material->SetScalarParameterValue(BadgeNewRevealParameterName, FMath::Clamp(NewRevealAmount, 0.0f, 1.0f));
		Material->SetScalarParameterValue(BadgeFeedbackToneParameterName, Tone);
		Material->SetScalarParameterValue(BadgeFeedbackSeedParameterName, Seed);
		Material->SetScalarParameterValue(BadgeReducedMotionParameterName, bReducedMotion ? 1.0f : 0.0f);
		Material->SetScalarParameterValue(BadgeEffectModeParameterName, EffectMode);
		Material->SetScalarParameterValue(BadgePreviewPulseParameterName, FMath::Clamp(Pulse, 0.0f, 1.0f));
		FSlateBrush Brush = DigitImage->GetBrush();
		Brush.SetResourceObject(Material);
		Brush.SetImageSize(FVector2f(
			FMath::Max(1.0f, DigitDrawSize.X),
			FMath::Max(1.0f, DigitDrawSize.Y)));
		DigitImage->SetBrush(Brush);
		DigitImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	bFeedbackMaterialActive = true;
	return true;
}

void UWacomCardEffectBadgeWidget::RestoreAuthoritativeDigitBrushes()
{
	if (bFeedbackMaterialActive)
	{
		bFeedbackMaterialActive = false;
		UpdateDigitImages();
	}
}

void UWacomCardEffectBadgeWidget::ReleaseDigitMaterialPool()
{
	bFeedbackMaterialActive = false;
	ActiveDigitMaterialInstances.Reset();
	ActiveDigitMaterialSource = nullptr;
}

void UWacomCardEffectBadgeWidget::PrimeDigitMaterialPool()
{
	UMaterialInterface* MaterialSource = FeedbackConfig.Style.DigitFeedbackMaterialInstance;
	if (!FeedbackConfig.bEnabled || !MaterialSource)
	{
		return;
	}
	if (ActiveDigitMaterialSource != MaterialSource)
	{
		ActiveDigitMaterialSource = MaterialSource;
		ActiveDigitMaterialInstances.Reset();
	}
	const int32 DesiredPoolSize = FMath::Max(1, MinimumDigitCount);
	while (ActiveDigitMaterialInstances.Num() < DesiredPoolSize)
	{
		ActiveDigitMaterialInstances.Add(UMaterialInstanceDynamic::Create(MaterialSource, this));
#if WITH_AUTOMATION_TESTS
		++DigitMaterialCreateCountForTest;
#endif
	}
}

void UWacomCardEffectBadgeWidget::CacheAuthoredRootTransform()
{
	if (bAuthoredRootTransformCached)
	{
		return;
	}
	AuthoredRootTransform = GetRenderTransform();
	AuthoredRootPivot = GetRenderTransformPivot();
	bAuthoredRootTransformCached = true;
}

void UWacomCardEffectBadgeWidget::RestoreAuthoredRootTransform()
{
	if (!bAuthoredRootTransformCached)
	{
		return;
	}
	SetRenderTransformPivot(AuthoredRootPivot);
	SetRenderTransform(AuthoredRootTransform);
}

void UWacomCardEffectBadgeWidget::EnsureSpriteCachesBuilt()
{
	if (bSpriteCachesBuilt)
	{
		return;
	}

	RebuildSpriteCaches();
}

void UWacomCardEffectBadgeWidget::RebuildSpriteCaches()
{
	ResolvedBadgeFrameSprites.Reset();
	ResolvedDigitSprites.Reset();

	for (const TPair<EWacomCardViewEffectBadgeKind, TSoftObjectPtr<UPaperSprite>>& Pair : BadgeFrameSprites)
	{
		if (!Pair.Value.IsNull())
		{
			UPaperSprite* Sprite = Pair.Value.Get();
			if (!Sprite)
			{
#if WITH_AUTOMATION_TESTS
				++SpriteSynchronousFallbackCountForTest;
#endif
				Sprite = Pair.Value.LoadSynchronous();
			}
			if (Sprite)
			{
				ResolvedBadgeFrameSprites.Add(Pair.Key, Sprite);
			}
		}
	}

	for (const TPair<int32, TSoftObjectPtr<UPaperSprite>>& Pair : DigitSprites)
	{
		if (!Pair.Value.IsNull())
		{
			UPaperSprite* Sprite = Pair.Value.Get();
			if (!Sprite)
			{
#if WITH_AUTOMATION_TESTS
				++SpriteSynchronousFallbackCountForTest;
#endif
				Sprite = Pair.Value.LoadSynchronous();
			}
			if (Sprite)
			{
				ResolvedDigitSprites.Add(Pair.Key, Sprite);
			}
		}
	}

	bSpriteCachesBuilt = true;
}

void UWacomCardEffectBadgeWidget::UpdateFrameImage()
{
	if (!BadgeFrameImage)
	{
		return;
	}

	if (UPaperSprite* Sprite = ResolvedBadgeFrameSprites.FindRef(CurrentData.Kind))
	{
		SetSpriteBrush(*BadgeFrameImage, *Sprite, BadgeFrameDrawSize);
		BadgeFrameImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		RefreshBadgeFrameShadow();
		return;
	}

	BadgeFrameImage->SetVisibility(ESlateVisibility::Collapsed);
	RefreshBadgeFrameShadow();
}

void UWacomCardEffectBadgeWidget::EnsureBadgeFrameShadowImage()
{
	if (BadgeFrameShadowImage || !WidgetTree || !BadgeFrameImage)
	{
		return;
	}
	UOverlay* ParentOverlay = Cast<UOverlay>(BadgeFrameImage->GetParent());
	if (!ParentOverlay)
	{
		return;
	}
	const int32 SourceIndex = ParentOverlay->GetChildIndex(BadgeFrameImage);
	if (SourceIndex == INDEX_NONE)
	{
		return;
	}

	BadgeFrameShadowImage = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(),
		TEXT("BadgeFrameShadowImage_Runtime"));
	if (!BadgeFrameShadowImage)
	{
		return;
	}
	BadgeFrameShadowImage->SetVisibility(ESlateVisibility::Collapsed);
	UOverlaySlot* ShadowSlot = Cast<UOverlaySlot>(
		ParentOverlay->InsertChildAt(SourceIndex, BadgeFrameShadowImage));
	const UOverlaySlot* SourceSlot = Cast<UOverlaySlot>(BadgeFrameImage->Slot);
	if (ShadowSlot && SourceSlot)
	{
		ShadowSlot->SetPadding(SourceSlot->GetPadding());
		ShadowSlot->SetHorizontalAlignment(SourceSlot->GetHorizontalAlignment());
		ShadowSlot->SetVerticalAlignment(SourceSlot->GetVerticalAlignment());
	}
}

void UWacomCardEffectBadgeWidget::RefreshBadgeFrameShadow()
{
	EnsureBadgeFrameShadowImage();
	if (!BadgeFrameShadowImage || !BadgeFrameImage
		|| !AttachmentCastShadowView.bAttachmentCastShadowEnabled
		|| AttachmentCastShadowView.AttachmentCastShadowOpacity <= KINDA_SMALL_NUMBER
		|| !BadgeFrameImage->IsVisible()
		|| (BadgeFrameImage->GetBrush().GetResourceObject() == nullptr
			&& BadgeFrameImage->GetBrush().GetDrawType()
				== ESlateBrushDrawType::NoDrawType))
	{
		if (BadgeFrameShadowImage)
		{
			BadgeFrameShadowImage->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	BadgeFrameShadowImage->SetBrush(BadgeFrameImage->GetBrush());
	FLinearColor ShadowColor = AttachmentCastShadowView.AttachmentCastShadowColor;
	ShadowColor.A = FMath::Clamp(
		AttachmentCastShadowView.AttachmentCastShadowOpacity,
		0.0f,
		1.0f);
	BadgeFrameShadowImage->SetColorAndOpacity(ShadowColor);
	BadgeFrameShadowImage->SetRenderTransformPivot(BadgeFrameImage->GetRenderTransformPivot());
	FWidgetTransform ShadowTransform = BadgeFrameImage->GetRenderTransform();
	ShadowTransform.Translation +=
		AttachmentCastShadowView.AttachmentCastShadowOffsetPixels;
	BadgeFrameShadowImage->SetRenderTransform(ShadowTransform);
	BadgeFrameShadowImage->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UWacomCardEffectBadgeWidget::UpdateDigitImages()
{
#if WITH_AUTOMATION_TESTS
	++DigitImageUpdateCountForTest;
#endif

	if (!DigitHost)
	{
		return;
	}

	DigitHost->SetVisibility(ESlateVisibility::Collapsed);

	if (!WidgetTree || ResolvedDigitSprites.IsEmpty())
	{
		while (DigitHost->GetChildrenCount() > 0)
		{
			DigitHost->RemoveChildAt(DigitHost->GetChildrenCount() - 1);
		}
		return;
	}

	const TArray<int32> Digits = SplitIntoDigits(CurrentData.Value);
	if (Digits.IsEmpty())
	{
		while (DigitHost->GetChildrenCount() > 0)
		{
			DigitHost->RemoveChildAt(DigitHost->GetChildrenCount() - 1);
		}
		return;
	}

	bool bDigitsComplete = true;
	for (int32 Index = 0; Index < Digits.Num(); ++Index)
	{
		const int32 Digit = Digits[Index];
		UPaperSprite* Sprite = ResolvedDigitSprites.FindRef(Digit);
		if (!Sprite)
		{
			bDigitsComplete = false;
			break;
		}

		UImage* DigitImage = Cast<UImage>(DigitHost->GetChildAt(Index));
		if (!DigitImage)
		{
			DigitImage = EnsureDigitImage(Index);
		}
		if (!DigitImage)
		{
			bDigitsComplete = false;
			break;
		}

		SetSpriteBrush(*DigitImage, *Sprite, DigitDrawSize);
		DigitImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UPanelSlot* AddedDigitSlot = DigitImage->Slot)
		{
			if (UHorizontalBoxSlot* HorizontalSlot = Cast<UHorizontalBoxSlot>(AddedDigitSlot))
			{
				const bool bInteriorDigit = Index > 0 && Index < Digits.Num() - 1;
				HorizontalSlot->SetPadding(bInteriorDigit ? InteriorDigitPadding : FMargin());
			}
		}
	}

	if (bDigitsComplete)
	{
		while (DigitHost->GetChildrenCount() > Digits.Num())
		{
			DigitHost->RemoveChildAt(DigitHost->GetChildrenCount() - 1);
		}
	}
	else
	{
		while (DigitHost->GetChildrenCount() > 0)
		{
			DigitHost->RemoveChildAt(DigitHost->GetChildrenCount() - 1);
		}
		return;
	}

	DigitHost->SetVisibility(
		DigitHost->GetChildrenCount() > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

UImage* UWacomCardEffectBadgeWidget::EnsureDigitImage(int32 Index)
{
	if (!WidgetTree || !DigitHost)
	{
		return nullptr;
	}

	UImage* DigitImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	DigitHost->AddChild(DigitImage);
	return DigitImage;
}

TArray<int32> UWacomCardEffectBadgeWidget::SplitIntoDigits(int32 Value) const
{
	TArray<int32> Result;
	if (Value <= 0)
	{
		Result.Add(0);
	}
	else
	{
		TArray<int32> Reversed;
		while (Value > 0)
		{
			Reversed.Add(Value % 10);
			Value /= 10;
		}

		for (int32 Index = Reversed.Num() - 1; Index >= 0; --Index)
		{
			Result.Add(Reversed[Index]);
		}
	}

	const int32 DesiredDigitCount = FMath::Max(1, MinimumDigitCount);
	while (Result.Num() < DesiredDigitCount)
	{
		Result.Insert(0, 0);
	}

	return Result;
}

void UWacomCardEffectBadgeWidget::SetSpriteBrush(UImage& Image, UPaperSprite& Sprite, const FVector2D& DesiredSize)
{
	FSlateBrush Brush = Image.GetBrush();
	Brush.SetResourceObject(&Sprite);
	Brush.SetImageSize(FVector2f(
		FMath::Max(1.0f, DesiredSize.X),
		FMath::Max(1.0f, DesiredSize.Y)));
	Image.SetBrush(Brush);
}
