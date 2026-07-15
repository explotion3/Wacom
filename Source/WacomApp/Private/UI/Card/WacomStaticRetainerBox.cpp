// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomStaticRetainerBox.h"

UWacomStaticRetainerBox::UWacomStaticRetainerBox(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetRetainRendering(true);
	InitRenderOnInvalidation(true);
	InitRenderOnPhase(false);
	SetVisibility(ESlateVisibility::HitTestInvisible);
}
