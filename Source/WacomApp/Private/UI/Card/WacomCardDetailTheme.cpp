// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardDetailTheme.h"

namespace
{
	const FSlateBrush* ResolveUsableBrushOrFallback(
		const FSlateBrush* Candidate,
		const FSlateBrush& Fallback)
	{
		if (Candidate && UWacomCardDetailTheme::IsInlineBrushConfigured(*Candidate))
		{
			return Candidate;
		}
		return UWacomCardDetailTheme::IsInlineBrushConfigured(Fallback) ? &Fallback : nullptr;
	}
}

const FSlateBrush* UWacomCardDetailTheme::ResolveIconBrush(EWacomCardDetailIcon Icon) const
{
	const FSlateBrush* Candidate = nullptr;
	for (const FWacomCardDetailIconBrushEntry& Entry : IconBrushes)
	{
		if (Entry.Icon == Icon)
		{
			Candidate = &Entry.Brush;
			break;
		}
	}

	return ResolveUsableBrushOrFallback(Candidate, FallbackInlineBrush);
}

const FSlateBrush* UWacomCardDetailTheme::ResolveStatusBrush(FGameplayTag StatusTag) const
{
	const FSlateBrush* Candidate = nullptr;
	for (const FWacomCardDetailStatusBrushEntry& Entry : StatusBrushes)
	{
		if (Entry.StatusTag == StatusTag)
		{
			Candidate = &Entry.Brush;
			break;
		}
	}

	return ResolveUsableBrushOrFallback(Candidate, FallbackInlineBrush);
}

const FSlateBrush* UWacomCardDetailTheme::ResolveFallbackInlineBrush() const
{
	return IsInlineBrushConfigured(FallbackInlineBrush) ? &FallbackInlineBrush : nullptr;
}

bool UWacomCardDetailTheme::IsInlineBrushConfigured(const FSlateBrush& Brush)
{
	return Brush.GetResourceObject() != nullptr;
}
