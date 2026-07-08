// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardDetailRichTextDecorator.h"

#include "Components/RichTextBlock.h"

namespace
{
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
				InOutString += *Label;
			}
		}
	};
}

TSharedPtr<ITextDecorator> UWacomCardDetailRichTextDecorator::CreateDecorator(
	URichTextBlock* InOwner)
{
	return MakeShared<FWacomCardDetailRichTextDecorator>(InOwner);
}
