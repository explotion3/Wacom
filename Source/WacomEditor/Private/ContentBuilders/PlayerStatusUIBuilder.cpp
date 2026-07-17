// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/PlayerStatusUIBuilder.h"

#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetTree.h"
#include "Channels/MovieSceneFloatChannel.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "HAL/FileManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "MovieScene.h"
#include "Sections/MovieSceneFloatSection.h"
#include "Tracks/MovieSceneFloatTrack.h"
#include "UI/Battle/PlayerStatusBar.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"

namespace
{
	constexpr TCHAR PlayerStatusObjectPath[] =
		TEXT("/Game/Wacom/UI/Battle/PlayerStatusBar/WBP_PlayerStatusBar.WBP_PlayerStatusBar");
	constexpr TCHAR ContractMarker[] = TEXT("WacomPlayerStatusImpact.ContractVersion=1");
	const FName DamageSurfaceName(TEXT("DamagePulseSurface"));
	const FName ShieldSurfaceName(TEXT("ShieldPulseSurface"));
	const FName DamageAnimationName(TEXT("DamagePulseAnimation"));
	const FName ShieldAnimationName(TEXT("ShieldPulseAnimation"));

	void RegisterWidgetGuid(UWidgetBlueprint& Blueprint, const UWidget& Widget)
	{
		const FString StablePath = FString::Printf(
			TEXT("%s:%s"), *Blueprint.GetPathName(), *Widget.GetName());
		Blueprint.WidgetVariableNameToGuidMap.FindOrAdd(Widget.GetFName()) =
			FGuid::NewDeterministicGuid(StablePath);
	}

	void MarkWidgetVariable(UWidgetBlueprint& Blueprint, UWidget& Widget)
	{
		Widget.bIsVariable = true;
		RegisterWidgetGuid(Blueprint, Widget);
	}

	UWidgetAnimation* FindAnimation(const UWidgetBlueprint& Blueprint, FName AnimationName)
	{
		for (UWidgetAnimation* Animation : Blueprint.Animations)
		{
			if (Animation && Animation->GetFName() == AnimationName)
			{
				return Animation;
			}
		}
		return nullptr;
	}

	bool IsRecognizedBaseLayout(const UWidgetBlueprint& Blueprint)
	{
		return Blueprint.ParentClass
			&& Blueprint.ParentClass->IsChildOf(UPlayerStatusBar::StaticClass())
			&& Blueprint.WidgetTree
			&& Blueprint.WidgetTree->RootWidget
			&& Blueprint.WidgetTree->FindWidget(TEXT("HpBar"));
	}

	bool IsAnimationValid(
		const UWidgetBlueprint& Blueprint,
		FName AnimationName,
		FName SurfaceName)
	{
		const UWidgetAnimation* Animation = FindAnimation(Blueprint, AnimationName);
		return Animation
			&& Animation->MovieScene
			&& Animation->AnimationBindings.ContainsByPredicate(
				[SurfaceName](const FWidgetAnimationBinding& Binding)
				{
					return Binding.WidgetName == SurfaceName;
				});
	}

	bool HasCompleteContract(const UWidgetBlueprint& Blueprint)
	{
		const UBorder* DamageSurface = Cast<UBorder>(
			Blueprint.WidgetTree ? Blueprint.WidgetTree->FindWidget(DamageSurfaceName) : nullptr);
		const UBorder* ShieldSurface = Cast<UBorder>(
			Blueprint.WidgetTree ? Blueprint.WidgetTree->FindWidget(ShieldSurfaceName) : nullptr);
		return IsRecognizedBaseLayout(Blueprint)
			&& DamageSurface
			&& ShieldSurface
			&& DamageSurface->GetVisibility() == ESlateVisibility::HitTestInvisible
			&& ShieldSurface->GetVisibility() == ESlateVisibility::HitTestInvisible
			&& IsAnimationValid(Blueprint, DamageAnimationName, DamageSurfaceName)
			&& IsAnimationValid(Blueprint, ShieldAnimationName, ShieldSurfaceName);
	}

	UOverlay* EnsureOverlayRoot(UWidgetBlueprint& Blueprint)
	{
		if (UOverlay* ExistingOverlay = Cast<UOverlay>(Blueprint.WidgetTree->RootWidget))
		{
			return ExistingOverlay;
		}

		UWidget* ExistingRoot = Blueprint.WidgetTree->RootWidget;
		UOverlay* OverlayRoot = Blueprint.WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), TEXT("PlayerStatusImpactRoot"));
		Blueprint.WidgetTree->RootWidget = OverlayRoot;
		if (UOverlaySlot* ContentSlot = OverlayRoot->AddChildToOverlay(ExistingRoot))
		{
			ContentSlot->SetHorizontalAlignment(HAlign_Fill);
			ContentSlot->SetVerticalAlignment(VAlign_Fill);
		}
		return OverlayRoot;
	}

	UBorder* AddPulseSurface(
		UWidgetBlueprint& Blueprint,
		UOverlay& Root,
		FName SurfaceName,
		const FLinearColor& Color)
	{
		if (UWidget* Existing = Blueprint.WidgetTree->FindWidget(SurfaceName))
		{
			return Cast<UBorder>(Existing);
		}

		UBorder* Surface = Blueprint.WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), SurfaceName);
		Surface->SetBrushColor(Color);
		Surface->SetRenderOpacity(0.0f);
		Surface->SetVisibility(ESlateVisibility::HitTestInvisible);
		MarkWidgetVariable(Blueprint, *Surface);
		if (UOverlaySlot* Slot = Root.AddChildToOverlay(Surface))
		{
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
		}
		return Surface;
	}

	UWidgetAnimation* AddOpacityPulseAnimation(
		UWidgetBlueprint& Blueprint,
		FName AnimationName,
		UWidget& Target)
	{
		if (UWidgetAnimation* Existing = FindAnimation(Blueprint, AnimationName))
		{
			return IsAnimationValid(Blueprint, AnimationName, Target.GetFName())
				? Existing
				: nullptr;
		}

		UWidgetAnimation* Animation = NewObject<UWidgetAnimation>(
			&Blueprint, AnimationName, RF_Transactional);
		Animation->SetDisplayLabel(AnimationName.ToString());
		const FString StableAnimationPath = FString::Printf(
			TEXT("%s:Animation:%s"),
			*Blueprint.GetPathName(),
			*AnimationName.ToString());
		Blueprint.WidgetVariableNameToGuidMap.FindOrAdd(AnimationName) =
			FGuid::NewDeterministicGuid(StableAnimationPath);
		Animation->MovieScene = NewObject<UMovieScene>(
			Animation, AnimationName, RF_Transactional);
		Animation->MovieScene->SetDisplayRate(FFrameRate(30, 1));
		Animation->MovieScene->SetTickResolutionDirectly(FFrameRate(30, 1));
		const FFrameNumber EndFrame(8);
		Animation->MovieScene->SetPlaybackRange(
			TRange<FFrameNumber>(FFrameNumber(0), EndFrame + 1));
		Animation->MovieScene->GetEditorData().WorkStart = 0.0;
		Animation->MovieScene->GetEditorData().WorkEnd = 8.0 / 30.0;

		const FGuid BindingGuid = Animation->MovieScene->AddPossessable(
			Target.GetName(), Target.GetClass());
		Animation->MovieScene->SetObjectDisplayName(
			BindingGuid, FText::FromName(Target.GetFName()));
		FWidgetAnimationBinding Binding;
		Binding.AnimationGuid = BindingGuid;
		Binding.WidgetName = Target.GetFName();
		Animation->AnimationBindings.Add(Binding);

		UMovieSceneFloatTrack* Track =
			Animation->MovieScene->AddTrack<UMovieSceneFloatTrack>(BindingGuid);
		if (!Track)
		{
			return nullptr;
		}
		Track->SetPropertyNameAndPath(TEXT("RenderOpacity"), TEXT("RenderOpacity"));
		UMovieSceneFloatSection* Section =
			Cast<UMovieSceneFloatSection>(Track->CreateNewSection());
		if (!Section)
		{
			return nullptr;
		}
		Section->SetRange(TRange<FFrameNumber>(FFrameNumber(0), EndFrame + 1));
		Section->GetChannel().AddLinearKey(FFrameNumber(0), 0.0f);
		Section->GetChannel().AddLinearKey(FFrameNumber(2), 1.0f);
		Section->GetChannel().AddLinearKey(FFrameNumber(8), 0.0f);
		Track->AddSection(*Section);
		Blueprint.Animations.Add(Animation);
		return Animation;
	}

	void MakeTreeNonHitTestable(UWidgetBlueprint& Blueprint)
	{
		TArray<UWidget*> Widgets;
		Blueprint.WidgetTree->GetAllWidgets(Widgets);
		for (UWidget* Widget : Widgets)
		{
			if (!Widget)
			{
				continue;
			}
			RegisterWidgetGuid(Blueprint, *Widget);
			if (Widget->GetVisibility() == ESlateVisibility::Visible
				|| Widget->GetVisibility() == ESlateVisibility::SelfHitTestInvisible)
			{
				Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
		}
	}

	bool CompileAndSave(UWidgetBlueprint& Blueprint)
	{
		MakeTreeNonHitTestable(Blueprint);
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(&Blueprint);
		FKismetEditorUtilities::CompileBlueprint(&Blueprint);
		if (Blueprint.Status == BS_Error || !Blueprint.GeneratedClass)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[PlayerStatusUIBuilder] Compile failed: %s"),
				*Blueprint.GetPathName());
			return false;
		}

		UPackage* Package = Blueprint.GetOutermost();
		Package->MarkPackageDirty();
		Blueprint.MarkPackageDirty();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, &Blueprint, *Filename, Args);
	}
}

bool Wacom::ContentBuilder::ProcessPlayerStatusImpactUI(
	bool bBuildImpactFeedback,
	bool bInspectOnly)
{
	if (bBuildImpactFeedback == bInspectOnly)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[PlayerStatusUIBuilder] Choose exactly one build or inspect mode"));
		return false;
	}

	UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(StaticLoadObject(
		UWidgetBlueprint::StaticClass(), nullptr, PlayerStatusObjectPath));
	if (!Blueprint)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[PlayerStatusUIBuilder] Missing asset: %s"), PlayerStatusObjectPath);
		return false;
	}
	if (!IsRecognizedBaseLayout(*Blueprint))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[PlayerStatusUIBuilder] Refusing unrecognized manual layout: %s"),
			*Blueprint->GetPathName());
		return false;
	}

	if (HasCompleteContract(*Blueprint))
	{
		UE_LOG(LogTemp, Display,
			TEXT("[PlayerStatusUIBuilder] Contract ready; no changes: %s"),
			*Blueprint->GetPathName());
		return true;
	}
	if (bInspectOnly)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[PlayerStatusUIBuilder] Impact contract incomplete: %s"),
			*Blueprint->GetPathName());
		return false;
	}

	if ((Blueprint->WidgetTree->FindWidget(DamageSurfaceName)
			&& !Cast<UBorder>(Blueprint->WidgetTree->FindWidget(DamageSurfaceName)))
		|| (Blueprint->WidgetTree->FindWidget(ShieldSurfaceName)
			&& !Cast<UBorder>(Blueprint->WidgetTree->FindWidget(ShieldSurfaceName)))
		|| (FindAnimation(*Blueprint, DamageAnimationName)
			&& !IsAnimationValid(*Blueprint, DamageAnimationName, DamageSurfaceName))
		|| (FindAnimation(*Blueprint, ShieldAnimationName)
			&& !IsAnimationValid(*Blueprint, ShieldAnimationName, ShieldSurfaceName)))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[PlayerStatusUIBuilder] Refusing conflicting manual impact widgets or animations"));
		return false;
	}

	Blueprint->Modify();
	UOverlay* Root = EnsureOverlayRoot(*Blueprint);
	UBorder* DamageSurface = AddPulseSurface(
		*Blueprint, *Root, DamageSurfaceName, FLinearColor(0.86f, 0.04f, 0.03f, 0.32f));
	UBorder* ShieldSurface = AddPulseSurface(
		*Blueprint, *Root, ShieldSurfaceName, FLinearColor(0.20f, 0.56f, 1.00f, 0.30f));
	if (!DamageSurface
		|| !ShieldSurface
		|| !AddOpacityPulseAnimation(*Blueprint, DamageAnimationName, *DamageSurface)
		|| !AddOpacityPulseAnimation(*Blueprint, ShieldAnimationName, *ShieldSurface))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[PlayerStatusUIBuilder] Failed to create impact presentation contract"));
		return false;
	}

	if (!Blueprint->BlueprintDescription.Contains(ContractMarker))
	{
		if (!Blueprint->BlueprintDescription.IsEmpty())
		{
			Blueprint->BlueprintDescription += TEXT("\n");
		}
		Blueprint->BlueprintDescription += ContractMarker;
	}

	if (!CompileAndSave(*Blueprint) || !HasCompleteContract(*Blueprint))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[PlayerStatusUIBuilder] Saved asset failed post-build validation"));
		return false;
	}
	UE_LOG(LogTemp, Display,
		TEXT("[PlayerStatusUIBuilder] Built impact feedback: %s"),
		*Blueprint->GetPathName());
	return true;
}
