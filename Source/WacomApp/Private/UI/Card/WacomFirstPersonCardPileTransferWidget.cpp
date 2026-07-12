// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardPileTransferWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/GameplayStatics.h"
#include "Rendering/DrawElements.h"
#include "SlateMaterialBrush.h"
#include "Styling/CoreStyle.h"
#include "UI/Card/WacomFirstPersonCardPileTransferPlayback.h"
#include "Widgets/SLeafWidget.h"

class SFirstPersonCardPileTransfer final : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SFirstPersonCardPileTransfer) {}
		SLATE_ARGUMENT(const FWacomFirstPersonCardPileTransferPlayback*, Playback)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args)
	{
		Playback = Args._Playback;
		SetCanTick(false);
	}

	void RefreshMaterial()
	{
		MaterialBrush.Reset();
		if (Playback && Playback->GetStyle().GlyphMaterialInstance)
		{
			MaterialBrush = MakeUnique<FSlateMaterialBrush>(
				*Playback->GetStyle().GlyphMaterialInstance,
				Playback->GetStyle().GlyphSize);
		}
		Invalidate(EInvalidateWidgetReason::Paint);
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		return FVector2D::ZeroVector;
	}

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override
	{
		if (!Playback || !MaterialBrush || Playback->GetGlyphs().IsEmpty())
		{
			return LayerId;
		}

		const FSlateResourceHandle ResourceHandle =
			FSlateApplication::Get().GetRenderer()->GetResourceHandle(*MaterialBrush);
		if (!ResourceHandle.IsValid())
		{
			return LayerId;
		}

		const int32 TrailCount = Playback->IsReducedMotion()
			? 0
			: FMath::Clamp(Playback->GetStyle().TrailLayerCount, 0, 3);
		const int32 QuadCount = Playback->GetGlyphs().Num() * (TrailCount + 1);
		TArray<FSlateVertex> Vertices;
		TArray<SlateIndex> Indices;
		Vertices.Reserve(QuadCount * 4);
		Indices.Reserve(QuadCount * 6);
		const FVector2D AbsoluteOrigin = AllottedGeometry.LocalToAbsolute(FVector2D::ZeroVector);
		const float LayoutScale = AllottedGeometry.Scale;

		auto AddQuad = [&Vertices, &Indices, &AbsoluteOrigin, LayoutScale](
			const FVector2D& Center,
			const FVector2D& Size,
			float RotationRadians,
			float Opacity)
		{
			if (Opacity <= UE_KINDA_SMALL_NUMBER || Size.X <= 0.0f || Size.Y <= 0.0f)
			{
				return;
			}
			const FVector2D HalfSize = Size * 0.5f;
			const float SinAngle = FMath::Sin(RotationRadians);
			const float CosAngle = FMath::Cos(RotationRadians);
			const FVector2D Corners[4] = {
				FVector2D(-HalfSize.X, -HalfSize.Y), FVector2D(HalfSize.X, -HalfSize.Y),
				FVector2D(HalfSize.X, HalfSize.Y), FVector2D(-HalfSize.X, HalfSize.Y) };
			const FVector2f UVs[4] = {
				FVector2f(0.0f, 0.0f), FVector2f(1.0f, 0.0f),
				FVector2f(1.0f, 1.0f), FVector2f(0.0f, 1.0f) };
			const SlateIndex BaseVertex = static_cast<SlateIndex>(Vertices.Num());
			for (int32 CornerIndex = 0; CornerIndex < 4; ++CornerIndex)
			{
				const FVector2D& Corner = Corners[CornerIndex];
				const FVector2D Rotated(
					Corner.X * CosAngle - Corner.Y * SinAngle,
					Corner.X * SinAngle + Corner.Y * CosAngle);
				FSlateVertex& Vertex = Vertices.AddDefaulted_GetRef();
				const FVector2D Absolute = AbsoluteOrigin + (Center + Rotated) * LayoutScale;
				Vertex.Position[0] = Absolute.X;
				Vertex.Position[1] = Absolute.Y;
				Vertex.TexCoords[0] = UVs[CornerIndex].X;
				Vertex.TexCoords[1] = UVs[CornerIndex].Y;
				Vertex.TexCoords[2] = 1.0f;
				Vertex.TexCoords[3] = 1.0f;
				Vertex.Color = FLinearColor(1.0f, 1.0f, 1.0f, Opacity).ToFColor(true);
			}
			Indices.Append({ BaseVertex, static_cast<SlateIndex>(BaseVertex + 1), static_cast<SlateIndex>(BaseVertex + 2),
				BaseVertex, static_cast<SlateIndex>(BaseVertex + 2), static_cast<SlateIndex>(BaseVertex + 3) });
		};

		for (const FWacomFirstPersonCardPileTransferGlyphView& Glyph : Playback->GetGlyphs())
		{
			for (int32 TrailIndex = TrailCount; TrailIndex > 0; --TrailIndex)
			{
				const float TrailAlpha = Glyph.Opacity * (0.16f / static_cast<float>(TrailIndex));
				AddQuad(
					Glyph.Position - FVector2D(0.0f, 4.0f * TrailIndex),
					Glyph.Size * (1.0f - 0.08f * TrailIndex),
					Glyph.RotationRadians,
					TrailAlpha);
			}
			AddQuad(Glyph.Position, Glyph.Size, Glyph.RotationRadians, Glyph.Opacity);
		}

		if (!Vertices.IsEmpty())
		{
			FSlateDrawElement::MakeCustomVerts(
				OutDrawElements,
				LayerId,
				ResourceHandle,
				Vertices,
				Indices,
				nullptr,
				0,
				0);
		}
		if (Playback->IsReducedMotion() && Playback->GetTotalCount() > 0)
		{
			const FVector2D LabelPosition = Playback->GetGlyphs()[0].Position + FVector2D(12.0f, -8.0f);
			FSlateDrawElement::MakeText(
				OutDrawElements,
				LayerId + 1,
				AllottedGeometry.ToOffsetPaintGeometry(LabelPosition),
				FString::Printf(TEXT("×%d"), Playback->GetTotalCount()),
				FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 12),
				ESlateDrawEffect::None,
				FLinearColor(0.96f, 0.82f, 0.42f, 1.0f));
		}
		return LayerId;
	}

private:
	const FWacomFirstPersonCardPileTransferPlayback* Playback = nullptr;
	TUniquePtr<FSlateMaterialBrush> MaterialBrush;
};

UWacomFirstPersonCardPileTransferWidget::UWacomFirstPersonCardPileTransferWidget(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Playback(MakeUnique<FWacomFirstPersonCardPileTransferPlayback>())
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

UWacomFirstPersonCardPileTransferWidget::~UWacomFirstPersonCardPileTransferWidget() = default;

void UWacomFirstPersonCardPileTransferWidget::SetConfig(
	const FWacomFirstPersonCardPileTransferConfig& InConfig)
{
	Config = InConfig;
}

bool UWacomFirstPersonCardPileTransferWidget::Play(
	const FWacomFirstPersonCardPileTransferHint& Hint,
	const FVector2D& SourcePosition,
	const FVector2D& TargetPosition)
{
	LastBroadcastArrivedCount = INDEX_NONE;
	bCompletionBroadcast = false;
	if (!Playback->Start(Hint, Config, SourcePosition, TargetPosition))
	{
		return false;
	}
	if (SlateWidget)
	{
		SlateWidget->RefreshMaterial();
	}
	PlayRequestedSounds();
	return true;
}

void UWacomFirstPersonCardPileTransferWidget::TickPlayback(float DeltaSeconds)
{
	if (!Playback->IsActive())
	{
		return;
	}
	const FWacomFirstPersonCardPileTransferProgressView Progress = Playback->Tick(DeltaSeconds);
	PlayRequestedSounds();
	if (SlateWidget)
	{
		SlateWidget->Invalidate(EInvalidateWidgetReason::Paint);
	}
	if (Progress.ArrivedCount != LastBroadcastArrivedCount || (Progress.bCompleted && !bCompletionBroadcast))
	{
		LastBroadcastArrivedCount = Progress.ArrivedCount;
		bCompletionBroadcast |= Progress.bCompleted;
		OnProgressNative.Broadcast(Progress);
	}
}

void UWacomFirstPersonCardPileTransferWidget::ForceComplete()
{
	if (!Playback->IsActive())
	{
		return;
	}
	const FWacomFirstPersonCardPileTransferProgressView Progress = Playback->ForceComplete();
	bCompletionBroadcast = true;
	LastBroadcastArrivedCount = Progress.ArrivedCount;
	OnProgressNative.Broadcast(Progress);
	if (SlateWidget)
	{
		SlateWidget->Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void UWacomFirstPersonCardPileTransferWidget::ResetPlayback()
{
	Playback->Reset();
	LastBroadcastArrivedCount = INDEX_NONE;
	bCompletionBroadcast = false;
	if (SlateWidget)
	{
		SlateWidget->RefreshMaterial();
	}
}

bool UWacomFirstPersonCardPileTransferWidget::IsPlaybackActive() const
{
	return Playback->IsActive();
}

TSharedRef<SWidget> UWacomFirstPersonCardPileTransferWidget::RebuildWidget()
{
	SlateWidget = SNew(SFirstPersonCardPileTransfer).Playback(Playback.Get());
	return SlateWidget.ToSharedRef();
}

void UWacomFirstPersonCardPileTransferWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	SlateWidget.Reset();
}

void UWacomFirstPersonCardPileTransferWidget::PlayRequestedSounds()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const FWacomFirstPersonCardPileTransferStyleData& Style = Playback->GetStyle();
	auto Play = [World, &Style](USoundBase* Sound)
	{
		if (Sound)
		{
			UGameplayStatics::PlaySound2D(
				World,
				Sound,
				FMath::Max(0.0f, Style.SoundVolumeMultiplier),
				FMath::Max(0.01f, Style.SoundPitchMultiplier));
		}
	};
	if (Playback->ConsumeStartSoundRequest())
	{
		Play(Style.StartSound);
	}
	if (Playback->ConsumeTravelSoundRequest())
	{
		Play(Style.TravelSound);
	}
	if (Playback->ConsumeCompleteSoundRequest())
	{
		Play(Style.CompleteSound);
	}
}
