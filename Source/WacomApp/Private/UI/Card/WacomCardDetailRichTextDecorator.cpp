// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardDetailRichTextDecorator.h"

#include "Components/RichTextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "UI/Card/WacomCardDetailRichTextBlock.h"
#include "UI/Card/WacomCardDetailTheme.h"
#include "WacomCardDetailIconIds.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SCompoundWidget.h"

namespace
{
	class SWacomCardDetailInlineImage final : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SWacomCardDetailInlineImage)
		{
		}
		SLATE_END_ARGS()

		void Construct(
			const FArguments& InArgs,
			const FSlateBrush* Brush,
			const FTextBlockStyle& TextStyle,
			FVector2D RenderOffset)
		{
			check(Brush);

			float IconHeight = 16.0f;
			if (FSlateApplication::IsInitialized())
			{
				const TSharedRef<FSlateFontMeasure> FontMeasure =
					FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
				IconHeight = FMath::Max(1.0f, FontMeasure->GetMaxCharacterHeight(TextStyle.Font, 1.0f));
			}

			const FVector2f BrushSize = Brush->GetImageSize();
			const float AspectRatio = BrushSize.Y > KINDA_SMALL_NUMBER
				? FMath::Max(0.1f, BrushSize.X / BrushSize.Y)
				: 1.0f;
			const float IconWidth = FMath::Max(1.0f, IconHeight * AspectRatio);

			ChildSlot
			[
				SNew(SBox)
				.WidthOverride(IconWidth)
				.HeightOverride(IconHeight)
				[
					SNew(SScaleBox)
					.Stretch(EStretch::ScaleToFit)
					.StretchDirection(EStretchDirection::DownOnly)
					.VAlign(VAlign_Center)
					[
						SNew(SImage)
						.Image(Brush)
						.RenderTransform(FSlateRenderTransform(RenderOffset))
					]
				]
			];
		}
	};

	const UWacomCardDetailTheme* ResolveTheme(const URichTextBlock* Owner)
	{
		const UWacomCardDetailRichTextBlock* CardDetailBlock =
			Cast<UWacomCardDetailRichTextBlock>(Owner);
		return CardDetailBlock ? CardDetailBlock->GetCardDetailTheme() : nullptr;
	}

	const FSlateBrush* ResolveInlineBrush(
		const FTextRunInfo& RunInfo,
		const UWacomCardDetailTheme* Theme)
	{
		if (!Theme)
		{
			return nullptr;
		}

		if (RunInfo.Name == TEXT("wacom-icon"))
		{
			if (const FString* Id = RunInfo.MetaData.Find(TEXT("id")))
			{
				const EWacomCardDetailIcon Icon = WacomCardDetailIconIds::FromString(*Id);
				return Icon != EWacomCardDetailIcon::None
					? Theme->ResolveIconBrush(Icon)
					: nullptr;
			}
			return nullptr;
		}

		if (RunInfo.Name == TEXT("wacom-status"))
		{
			if (const FString* Tag = RunInfo.MetaData.Find(TEXT("tag")))
			{
				return Theme->ResolveStatusBrush(FGameplayTag::RequestGameplayTag(FName(**Tag), false));
			}
			return nullptr;
		}

		return nullptr;
	}

	class FWacomCardDetailRichTextDecorator final : public FRichTextDecorator
	{
	public:
		explicit FWacomCardDetailRichTextDecorator(URichTextBlock* InOwner)
			: FRichTextDecorator(InOwner)
		{
		}

		virtual bool Supports(const FTextRunParseResults& RunParseResult, const FString& /*Text*/) const override
		{
			return RunParseResult.Name == TEXT("wacom-icon")
				|| RunParseResult.Name == TEXT("wacom-status")
				|| RunParseResult.Name == TEXT("wacom-keyword");
		}

	protected:
		virtual TSharedPtr<SWidget> CreateDecoratorWidget(
			const FTextRunInfo& RunInfo,
			const FTextBlockStyle& DefaultTextStyle) const override
		{
			const UWacomCardDetailTheme* Theme = ResolveTheme(Owner);
			const FSlateBrush* Brush = ResolveInlineBrush(RunInfo, Theme);
			const FVector2D RenderOffset = Theme
				? Theme->InlineIconRenderOffset
				: FVector2D::ZeroVector;
			return Brush
				? SNew(SWacomCardDetailInlineImage, Brush, DefaultTextStyle, RenderOffset)
				: TSharedPtr<SWidget>();
		}

		virtual void CreateDecoratorText(
			const FTextRunInfo& RunInfo,
			FTextBlockStyle& /*InOutTextStyle*/,
			FString& InOutString) const override
		{
			if (!InOutString.IsEmpty())
			{
				return;
			}

			if (const FString* Label = RunInfo.MetaData.Find(TEXT("label")))
			{
				if (RunInfo.Name != TEXT("wacom-status"))
				{
					InOutString += *Label;
				}
			}
		}
	};
}

TSharedPtr<ITextDecorator> UWacomCardDetailRichTextDecorator::CreateDecorator(
	URichTextBlock* InOwner)
{
	return MakeShared<FWacomCardDetailRichTextDecorator>(InOwner);
}
