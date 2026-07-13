// Copyright Wacom. All Rights Reserved.

#include "UI/Common/PileCountView.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

namespace
{
	float EaseOutQuart(float Value)
	{
		const float Inverse = 1.0f - FMath::Clamp(Value, 0.0f, 1.0f);
		return 1.0f - FMath::Pow(Inverse, 4.0f);
	}

	float EaseOutQuint(float Value)
	{
		const float Inverse = 1.0f - FMath::Clamp(Value, 0.0f, 1.0f);
		return 1.0f - FMath::Pow(Inverse, 5.0f);
	}

	enum class EWacomPileFeedbackPulseKind : uint8
	{
		Receive,
		Send
	};

	struct FPileFeedbackPulse
	{
		EWacomPileFeedbackPulseKind Kind = EWacomPileFeedbackPulseKind::Receive;
		float ElapsedSeconds = 0.0f;
		float Strength = 1.0f;
		int32 ItemCount = 1;
		bool bFinal = false;
		FVector2D Direction = FVector2D(0.0f, -1.0f);
	};

	struct FPileFeedbackResponse
	{
		float ReceiveSigned = 0.0f;
		float SendCompression = 0.0f;
		float SendRecoil = 0.0f;
		FVector2D SendTranslation = FVector2D::ZeroVector;
	};

	FVector2D NormalizeLaunchDirection(FVector2D Direction)
	{
		if (!FMath::IsFinite(Direction.X)
			|| !FMath::IsFinite(Direction.Y)
			|| Direction.SizeSquared() <= UE_SMALL_NUMBER)
		{
			return FVector2D(0.0f, -1.0f);
		}
		return Direction.GetSafeNormal();
	}
}

struct FWacomPileFeedbackPlayback
{
	TArray<FPileFeedbackPulse> Pulses;
	TWeakObjectPtr<UWidget> TargetWidget;
	TWeakObjectPtr<UWidget> CountWidget;
	FWidgetTransform AuthoredTargetTransform;
	FWidgetTransform AuthoredCountTransform;
	FVector2D AuthoredTargetPivot = FVector2D(0.5f, 0.5f);
	FVector2D AuthoredCountPivot = FVector2D(0.5f, 0.5f);
	bool bCapturedAuthoredState = false;
};

UPileCountView::UPileCountView(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UPileCountView::~UPileCountView()
{
	RestorePileFeedbackAuthoredState();
	delete PileFeedbackPlayback;
	PileFeedbackPlayback = nullptr;
}

TSharedRef<SWidget> UPileCountView::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		USizeBox* Root = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Root"));
		Root->SetWidthOverride(80.0f);
		Root->SetHeightOverride(80.0f);
		WidgetTree->RootWidget = Root;

		UOverlay* Content = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Content"));
		Root->AddChild(Content);

		CountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CountText"));
		CountText->SetText(FText::AsNumber(0));
		CountText->SetJustification(ETextJustify::Center);
		CountText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		{
			FSlateFontInfo F = CountText->GetFont();
			F.Size = 22;
			CountText->SetFont(F);
		}
		if (UOverlaySlot* CountSlot = Content->AddChildToOverlay(CountText))
		{
			CountSlot->SetHorizontalAlignment(HAlign_Center);
			CountSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
	return Super::RebuildWidget();
}

void UPileCountView::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshDisplay();
}

void UPileCountView::NativeDestruct()
{
	if (PileFeedbackPlayback)
	{
		PileFeedbackPlayback->Pulses.Reset();
	}
	RestorePileFeedbackAuthoredState();
	Super::NativeDestruct();
}

void UPileCountView::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!PileFeedbackPlayback || PileFeedbackPlayback->Pulses.IsEmpty())
	{
		return;
	}
	EvaluateAndApplyPileFeedback(InDeltaTime, true);
}

void UPileCountView::SetCount(int32 InCount)
{
	Count = InCount;
	CountDisplayText = FText::AsNumber(Count);
	if (CountText)
	{
		CountText->SetText(CountDisplayText);
	}
}

void UPileCountView::SetCountDisplayText(FText InText)
{
	CountDisplayText = InText;
	if (CountText)
	{
		CountText->SetText(GetCountDisplayText());
	}
}

void UPileCountView::PlayReceiveFeedback(
	int32 ReceivedCount,
	bool bFinalArrival,
	bool bReducedMotion)
{
	if (!ReceiveFeedbackStyle.bEnabled || ReceivedCount <= 0 || bReducedMotion)
	{
		if (bReducedMotion)
		{
			ResetReceiveFeedback();
		}
		return;
	}

	EnsurePileFeedbackPlayback();
	FPileFeedbackPulse Pulse;
	Pulse.Kind = EWacomPileFeedbackPulseKind::Receive;
	Pulse.ItemCount = ReceivedCount;
	Pulse.bFinal = bFinalArrival;
	Pulse.Strength = FMath::Sqrt(static_cast<float>(ReceivedCount));
	if (bFinalArrival)
	{
		Pulse.Strength *= FMath::Max(0.0f, ReceiveFeedbackStyle.FinalArrivalStrengthMultiplier);
	}
	Pulse.Strength = FMath::Min(
		Pulse.Strength,
		FMath::Max(0.0f, ReceiveFeedbackStyle.MaxCombinedStrength));
	PileFeedbackPlayback->Pulses.Add(Pulse);
}

void UPileCountView::ResetReceiveFeedback()
{
	if (!PileFeedbackPlayback)
	{
		return;
	}
	PileFeedbackPlayback->Pulses.RemoveAll(
		[](const FPileFeedbackPulse& Pulse)
		{
			return Pulse.Kind == EWacomPileFeedbackPulseKind::Receive;
		});
	if (PileFeedbackPlayback->Pulses.IsEmpty())
	{
		RestorePileFeedbackAuthoredState();
	}
	else
	{
		EvaluateAndApplyPileFeedback(0.0f, false);
	}
}

void UPileCountView::PlaySendFeedback(
	int32 SentCount,
	bool bFinalDeparture,
	FVector2D LaunchDirection,
	bool bReducedMotion)
{
	if (!SendFeedbackStyle.bEnabled
		|| SentCount <= 0
		|| bReducedMotion
		|| SendFeedbackStyle.bReduceMotion)
	{
		if (bReducedMotion || SendFeedbackStyle.bReduceMotion)
		{
			ResetSendFeedback();
		}
		return;
	}

	EnsurePileFeedbackPlayback();
	LaunchDirection = NormalizeLaunchDirection(LaunchDirection);
	FPileFeedbackPulse* Pulse = nullptr;
	if (!PileFeedbackPlayback->Pulses.IsEmpty())
	{
		FPileFeedbackPulse& LastPulse = PileFeedbackPlayback->Pulses.Last();
		if (LastPulse.Kind == EWacomPileFeedbackPulseKind::Send
			&& LastPulse.ElapsedSeconds <= 0.0f)
		{
			Pulse = &LastPulse;
			Pulse->Direction = NormalizeLaunchDirection(
				Pulse->Direction * static_cast<float>(Pulse->ItemCount)
				+ LaunchDirection * static_cast<float>(SentCount));
			Pulse->ItemCount += SentCount;
			Pulse->bFinal = Pulse->bFinal || bFinalDeparture;
		}
	}
	if (!Pulse)
	{
		FPileFeedbackPulse NewPulse;
		NewPulse.Kind = EWacomPileFeedbackPulseKind::Send;
		NewPulse.ItemCount = SentCount;
		NewPulse.bFinal = bFinalDeparture;
		NewPulse.Direction = LaunchDirection;
		PileFeedbackPlayback->Pulses.Add(NewPulse);
		Pulse = &PileFeedbackPlayback->Pulses.Last();
	}

	Pulse->Strength = FMath::Sqrt(static_cast<float>(Pulse->ItemCount));
	if (Pulse->bFinal)
	{
		Pulse->Strength *= FMath::Max(0.0f, SendFeedbackStyle.FinalDepartureStrengthMultiplier);
	}
	Pulse->Strength = FMath::Min(
		Pulse->Strength,
		FMath::Max(0.0f, SendFeedbackStyle.MaxCombinedStrength));
}

void UPileCountView::ResetSendFeedback()
{
	if (!PileFeedbackPlayback)
	{
		return;
	}
	PileFeedbackPlayback->Pulses.RemoveAll(
		[](const FPileFeedbackPulse& Pulse)
		{
			return Pulse.Kind == EWacomPileFeedbackPulseKind::Send;
		});
	if (PileFeedbackPlayback->Pulses.IsEmpty())
	{
		RestorePileFeedbackAuthoredState();
	}
	else
	{
		EvaluateAndApplyPileFeedback(0.0f, false);
	}
}

void UPileCountView::RefreshDisplay()
{
	if (CountText)
	{
		CountText->SetText(GetCountDisplayText());
	}
}

void UPileCountView::EnsurePileFeedbackPlayback()
{
	if (!PileFeedbackPlayback)
	{
		PileFeedbackPlayback = new FWacomPileFeedbackPlayback();
	}
	if (PileFeedbackPlayback->bCapturedAuthoredState)
	{
		return;
	}

	UWidget* TargetWidget = PileFeedbackRoot
		? PileFeedbackRoot.Get()
		: (ReceiveFeedbackRoot ? ReceiveFeedbackRoot.Get() : this);
	PileFeedbackPlayback->TargetWidget = TargetWidget;
	PileFeedbackPlayback->CountWidget = CountText;
	PileFeedbackPlayback->AuthoredTargetTransform = TargetWidget->GetRenderTransform();
	PileFeedbackPlayback->AuthoredTargetPivot = TargetWidget->GetRenderTransformPivot();
	TargetWidget->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	if (CountText)
	{
		PileFeedbackPlayback->AuthoredCountTransform = CountText->GetRenderTransform();
		PileFeedbackPlayback->AuthoredCountPivot = CountText->GetRenderTransformPivot();
		CountText->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	}
	PileFeedbackPlayback->bCapturedAuthoredState = true;
}

void UPileCountView::EvaluateAndApplyPileFeedback(float DeltaTime, bool bAdvanceTime)
{
	if (!PileFeedbackPlayback || !PileFeedbackPlayback->bCapturedAuthoredState)
	{
		return;
	}

	FPileFeedbackResponse Response;
	const float SafeDeltaTime = FMath::Max(0.0f, DeltaTime);
	for (FPileFeedbackPulse& Pulse : PileFeedbackPlayback->Pulses)
	{
		if (bAdvanceTime)
		{
			Pulse.ElapsedSeconds += SafeDeltaTime;
		}
		if (Pulse.Kind == EWacomPileFeedbackPulseKind::Receive)
		{
			const float Duration = FMath::Max(0.01f, ReceiveFeedbackStyle.DurationSeconds);
			const float CompressionPeak = FMath::Clamp(
				ReceiveFeedbackStyle.CompressionPeakSeconds,
				0.001f,
				Duration);
			const float ReboundPeak = FMath::Clamp(
				ReceiveFeedbackStyle.ReboundPeakSeconds,
				CompressionPeak,
				Duration);
			float SignedResponse = 0.0f;
			if (Pulse.ElapsedSeconds <= CompressionPeak)
			{
				SignedResponse = -EaseOutQuart(Pulse.ElapsedSeconds / CompressionPeak);
			}
			else if (Pulse.ElapsedSeconds <= ReboundPeak)
			{
				const float SegmentDuration = FMath::Max(0.001f, ReboundPeak - CompressionPeak);
				const float Alpha = (Pulse.ElapsedSeconds - CompressionPeak) / SegmentDuration;
				SignedResponse = FMath::Lerp(-1.0f, 1.0f, EaseOutQuart(Alpha));
			}
			else
			{
				const float SegmentDuration = FMath::Max(0.001f, Duration - ReboundPeak);
				const float Alpha = (Pulse.ElapsedSeconds - ReboundPeak) / SegmentDuration;
				SignedResponse = FMath::Lerp(1.0f, 0.0f, EaseOutQuint(Alpha));
			}
			Response.ReceiveSigned += SignedResponse * Pulse.Strength;
			continue;
		}

		const float Duration = FMath::Max(0.01f, SendFeedbackStyle.DurationSeconds);
		const float CompressionPeak = FMath::Clamp(
			SendFeedbackStyle.CompressionPeakSeconds,
			0.001f,
			Duration);
		const float RecoilPeak = FMath::Clamp(
			SendFeedbackStyle.RecoilPeakSeconds,
			CompressionPeak,
			Duration);
		float CompressionAmount = 0.0f;
		float RecoilAmount = 0.0f;
		if (Pulse.ElapsedSeconds <= CompressionPeak)
		{
			CompressionAmount = EaseOutQuart(Pulse.ElapsedSeconds / CompressionPeak);
		}
		else if (Pulse.ElapsedSeconds <= RecoilPeak)
		{
			const float SegmentDuration = FMath::Max(0.001f, RecoilPeak - CompressionPeak);
			const float Alpha = EaseOutQuart(
				(Pulse.ElapsedSeconds - CompressionPeak) / SegmentDuration);
			CompressionAmount = 1.0f - Alpha;
			RecoilAmount = Alpha;
		}
		else
		{
			const float SegmentDuration = FMath::Max(0.001f, Duration - RecoilPeak);
			const float Alpha = (Pulse.ElapsedSeconds - RecoilPeak) / SegmentDuration;
			RecoilAmount = 1.0f - EaseOutQuint(Alpha);
		}
		CompressionAmount *= Pulse.Strength;
		RecoilAmount *= Pulse.Strength;
		Response.SendCompression += CompressionAmount;
		Response.SendRecoil += RecoilAmount;
		Response.SendTranslation += Pulse.Direction
			* (CompressionAmount * SendFeedbackStyle.CompressionTranslationPixels
				- RecoilAmount * SendFeedbackStyle.RecoilTranslationPixels);
	}

	const float ReceiveMax = FMath::Max(0.0f, ReceiveFeedbackStyle.MaxCombinedStrength);
	Response.ReceiveSigned = FMath::Clamp(
		Response.ReceiveSigned,
		-ReceiveMax,
		ReceiveMax);
	const float SendMax = FMath::Max(0.0f, SendFeedbackStyle.MaxCombinedStrength);
	const float SendMagnitude = FMath::Max(Response.SendCompression, Response.SendRecoil);
	if (SendMagnitude > SendMax && SendMagnitude > UE_SMALL_NUMBER)
	{
		const float ClampScale = SendMax / SendMagnitude;
		Response.SendCompression *= ClampScale;
		Response.SendRecoil *= ClampScale;
		Response.SendTranslation *= ClampScale;
	}

	const float ReceiveCompression = FMath::Max(0.0f, -Response.ReceiveSigned);
	const float ReceiveRebound = FMath::Max(0.0f, Response.ReceiveSigned);
	if (UWidget* TargetWidget = PileFeedbackPlayback->TargetWidget.Get())
	{
		FWidgetTransform Transform = PileFeedbackPlayback->AuthoredTargetTransform;
		const FVector2D ReceiveScale(
			1.0f
				+ ReceiveCompression * (ReceiveFeedbackStyle.CompressionScale.X - 1.0f)
				+ ReceiveRebound * (ReceiveFeedbackStyle.ReboundScale.X - 1.0f),
			1.0f
				+ ReceiveCompression * (ReceiveFeedbackStyle.CompressionScale.Y - 1.0f)
				+ ReceiveRebound * (ReceiveFeedbackStyle.ReboundScale.Y - 1.0f));
		const FVector2D SendScale(
			1.0f
				+ Response.SendCompression * (SendFeedbackStyle.CompressionScale.X - 1.0f)
				+ Response.SendRecoil * (SendFeedbackStyle.RecoilScale.X - 1.0f),
			1.0f
				+ Response.SendCompression * (SendFeedbackStyle.CompressionScale.Y - 1.0f)
				+ Response.SendRecoil * (SendFeedbackStyle.RecoilScale.Y - 1.0f));
		Transform.Scale *= ReceiveScale * SendScale;
		Transform.Translation.Y +=
			ReceiveCompression * ReceiveFeedbackStyle.CompressionTranslationPixels
			+ ReceiveRebound * ReceiveFeedbackStyle.ReboundTranslationPixels;
		Transform.Translation += Response.SendTranslation;
		TargetWidget->SetRenderTransform(Transform);
	}
	if (UWidget* CountWidget = PileFeedbackPlayback->CountWidget.Get())
	{
		FWidgetTransform Transform = PileFeedbackPlayback->AuthoredCountTransform;
		const float ReceiveCountScale = 1.0f
			+ ReceiveRebound * (FMath::Max(0.0f, ReceiveFeedbackStyle.CountPulseScale) - 1.0f);
		const float SendCountScale = 1.0f
			+ Response.SendCompression * (FMath::Max(0.0f, SendFeedbackStyle.CountCompressionScale) - 1.0f)
			+ Response.SendRecoil * (FMath::Max(0.0f, SendFeedbackStyle.CountRecoilScale) - 1.0f);
		Transform.Scale *= FVector2D(
			ReceiveCountScale * SendCountScale,
			ReceiveCountScale * SendCountScale);
		CountWidget->SetRenderTransform(Transform);
	}

	if (bAdvanceTime)
	{
		PileFeedbackPlayback->Pulses.RemoveAll(
			[this](const FPileFeedbackPulse& Pulse)
			{
				const float Duration = Pulse.Kind == EWacomPileFeedbackPulseKind::Receive
					? FMath::Max(0.01f, ReceiveFeedbackStyle.DurationSeconds)
					: FMath::Max(0.01f, SendFeedbackStyle.DurationSeconds);
				return Pulse.ElapsedSeconds >= Duration;
			});
		if (PileFeedbackPlayback->Pulses.IsEmpty())
		{
			RestorePileFeedbackAuthoredState();
		}
	}
}

void UPileCountView::RestorePileFeedbackAuthoredState()
{
	if (!PileFeedbackPlayback || !PileFeedbackPlayback->bCapturedAuthoredState)
	{
		return;
	}
	if (UWidget* TargetWidget = PileFeedbackPlayback->TargetWidget.Get())
	{
		TargetWidget->SetRenderTransform(PileFeedbackPlayback->AuthoredTargetTransform);
		TargetWidget->SetRenderTransformPivot(PileFeedbackPlayback->AuthoredTargetPivot);
	}
	if (UWidget* CountWidget = PileFeedbackPlayback->CountWidget.Get())
	{
		CountWidget->SetRenderTransform(PileFeedbackPlayback->AuthoredCountTransform);
		CountWidget->SetRenderTransformPivot(PileFeedbackPlayback->AuthoredCountPivot);
	}
	PileFeedbackPlayback->TargetWidget.Reset();
	PileFeedbackPlayback->CountWidget.Reset();
	PileFeedbackPlayback->bCapturedAuthoredState = false;
}
