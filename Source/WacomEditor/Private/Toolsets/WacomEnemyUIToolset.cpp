// Copyright Wacom. All Rights Reserved.

#include "Toolsets/WacomEnemyUIToolset.h"

#include "ContentBuilders/EnemyUIHitTestPolicy.h"
#include "Animation/MovieScene2DTransformSection.h"
#include "Animation/MovieScene2DTransformTrack.h"
#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetTree.h"
#include "Channels/MovieSceneFloatChannel.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "MovieScene.h"
#include "Components/HorizontalBox.h"
#include "Components/SizeBox.h"
#include "UI/Battle/WacomBattleEnemyInspectionWidget.h"
#include "UI/Battle/WacomBattleEnemyInspectionPartRowWidget.h"
#include "UI/Battle/WacomBattleEnemyPanelWidget.h"
#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"
#include "WidgetBlueprint.h"

namespace
{
	constexpr TCHAR InspectionPackageName[] =
		TEXT("/Game/Wacom/UI/Enemy/WBP_WacomBattleEnemyInspectionWidget");
	constexpr TCHAR MultiPanelPackageName[] =
		TEXT("/Game/Wacom/UI/Enemy/BP_WacomBattleEnemyPanelWidget");
	constexpr TCHAR MultiEntryPackageName[] =
		TEXT("/Game/Wacom/UI/Enemy/BP_WacomBattleEnemyPartEntryWidget");
	constexpr TCHAR SinglePanelPackageName[] =
		TEXT("/Game/Wacom/UI/Enemy/WBP_WacomBattleEnemySinglePartPanelWidget");
	constexpr TCHAR SingleEntryPackageName[] =
		TEXT("/Game/Wacom/UI/Enemy/WBP_WacomBattleEnemySinglePartEntryWidget");
	constexpr TCHAR InspectionRowPackageName[] =
		TEXT("/Game/Wacom/UI/Enemy/WBP_WacomBattleEnemyInspectionPartRowWidget");
	constexpr TCHAR SegmentedVitalsContractMarker[] =
		TEXT("WacomEnemyPanelWBP.ContractVersion=2");
	constexpr TCHAR InspectionContractMarker[] =
		TEXT("WacomEnemyInspectionWBP.ContractVersion=1");
	constexpr TCHAR InspectionRowContractMarker[] =
		TEXT("WacomEnemyInspectionPartRowWBP.ContractVersion=1");

	struct FPanelSlideTarget
	{
		UWidget* Widget = nullptr;
		float StartX = 0.0f;
		float EndX = 0.0f;
	};

	UWidgetAnimation* FindAnimation(
		const UWidgetBlueprint& Blueprint,
		const FName AnimationName)
	{
		for (UWidgetAnimation* Animation : Blueprint.Animations)
		{
			if (Animation
				&& (Animation->GetFName() == AnimationName
					|| Animation->GetDisplayLabel() == AnimationName.ToString()))
			{
				return Animation;
			}
		}
		return nullptr;
	}

	bool HasBindingForWidget(
		const UWidgetAnimation& Animation,
		const UWidget& Widget)
	{
		return Animation.GetBindings().ContainsByPredicate(
			[&Widget](const FWidgetAnimationBinding& Binding)
			{
				return Binding.WidgetName == Widget.GetFName();
			});
	}

	bool AddSlideTrack(
		UWidgetAnimation& Animation,
		UWidget& Target,
		const float DurationSeconds,
		const float StartX,
		const float EndX)
	{
		check(Animation.MovieScene);
		const FGuid BindingGuid = Animation.MovieScene->AddPossessable(
			Target.GetName(), Target.GetClass());
		Animation.MovieScene->SetObjectDisplayName(
			BindingGuid, FText::FromName(Target.GetFName()));

		FWidgetAnimationBinding Binding;
		Binding.AnimationGuid = BindingGuid;
		Binding.WidgetName = Target.GetFName();
		Animation.AnimationBindings.Add(Binding);

		UMovieScene2DTransformTrack* Track =
			Animation.MovieScene->AddTrack<UMovieScene2DTransformTrack>(BindingGuid);
		if (!Track)
		{
			return false;
		}
		Track->SetPropertyNameAndPath(TEXT("RenderTransform"), TEXT("RenderTransform"));

		UMovieScene2DTransformSection* Section =
			Cast<UMovieScene2DTransformSection>(Track->CreateNewSection());
		if (!Section)
		{
			return false;
		}
		const FFrameNumber EndFrame(
			FMath::Max(1, FMath::RoundToInt(DurationSeconds * 30.0f)));
		Section->SetRange(TRange<FFrameNumber>(FFrameNumber(0), EndFrame + 1));
		Section->SetMask(FMovieScene2DTransformMask(
			EMovieScene2DTransformChannel::TranslationX));
		Section->Translation[0].AddLinearKey(FFrameNumber(0), StartX);
		Section->Translation[0].AddLinearKey(EndFrame, EndX);
		Track->AddSection(*Section);
		return true;
	}

	UWidgetAnimation* AddSlideAnimation(
		UWidgetBlueprint& Blueprint,
		const FName AnimationName,
		const float DurationSeconds,
		const TArray<FPanelSlideTarget>& Targets)
	{
		UWidgetAnimation* Animation = NewObject<UWidgetAnimation>(
			&Blueprint, AnimationName, RF_Transactional);
		if (!Animation)
		{
			return nullptr;
		}

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
		const FFrameNumber EndFrame(
			FMath::Max(1, FMath::RoundToInt(DurationSeconds * 30.0f)));
		Animation->MovieScene->SetPlaybackRange(
			TRange<FFrameNumber>(FFrameNumber(0), EndFrame + 1));
		Animation->MovieScene->GetEditorData().WorkStart = 0.0;
		Animation->MovieScene->GetEditorData().WorkEnd = DurationSeconds;

		for (const FPanelSlideTarget& Target : Targets)
		{
			if (!Target.Widget
				|| !AddSlideTrack(
					*Animation,
					*Target.Widget,
					DurationSeconds,
					Target.StartX,
					Target.EndX))
			{
				Animation->Rename(
					nullptr,
					GetTransientPackage(),
					REN_DontCreateRedirectors | REN_NonTransactional);
				return nullptr;
			}
		}

		Blueprint.Animations.Add(Animation);
		return Animation;
	}

	bool EnsureAnimation(
		UWidgetBlueprint& Blueprint,
		const FName AnimationName,
		const float DurationSeconds,
		const TArray<FPanelSlideTarget>& Targets)
	{
		if (const UWidgetAnimation* Existing = FindAnimation(Blueprint, AnimationName))
		{
			if (!Existing->MovieScene || Existing->GetBindings().Num() != Targets.Num())
			{
				return false;
			}
			for (const FPanelSlideTarget& Target : Targets)
			{
				if (!Target.Widget || !HasBindingForWidget(*Existing, *Target.Widget))
				{
					return false;
				}
			}
			return true;
		}

		return AddSlideAnimation(
			Blueprint, AnimationName, DurationSeconds, Targets) != nullptr;
	}

	const TCHAR* ResolveContractMarker(const UWidgetBlueprint& Blueprint)
	{
		const FString PackageName = Blueprint.GetOutermost()->GetName();
		const UClass* ParentClass = Blueprint.ParentClass;
		if ((PackageName == MultiPanelPackageName || PackageName == SinglePanelPackageName)
			&& ParentClass
			&& ParentClass->IsChildOf(UWacomBattleEnemyPanelWidget::StaticClass()))
		{
			return SegmentedVitalsContractMarker;
		}
		if ((PackageName == MultiEntryPackageName || PackageName == SingleEntryPackageName)
			&& ParentClass
			&& ParentClass->IsChildOf(UWacomBattleEnemyPartEntryWidget::StaticClass()))
		{
			return SegmentedVitalsContractMarker;
		}
		if (PackageName == InspectionPackageName
			&& ParentClass
			&& ParentClass->IsChildOf(UWacomBattleEnemyInspectionWidget::StaticClass()))
		{
			return InspectionContractMarker;
		}
		if (PackageName == InspectionRowPackageName
			&& ParentClass
			&& ParentClass->IsChildOf(
				UWacomBattleEnemyInspectionPartRowWidget::StaticClass()))
		{
			return InspectionRowContractMarker;
		}
		return nullptr;
	}

	bool IsEnemyUIContractLine(const FString& Line)
	{
		return Line.StartsWith(TEXT("WacomEnemyPanelWBP.ContractVersion="))
			|| Line.StartsWith(TEXT("WacomEnemySinglePartWBP.ContractVersion="))
			|| Line.StartsWith(TEXT("WacomEnemyInspectionWBP.ContractVersion="))
			|| Line.StartsWith(TEXT("WacomEnemyInspectionPartRowWBP.ContractVersion="));
	}

	bool HasExpectedSinglePartGeometry(
		const UWidgetBlueprint& PanelBlueprint,
		const UWidgetBlueprint& EntryBlueprint)
	{
		const USizeBox* PanelRoot = PanelBlueprint.WidgetTree
			? Cast<USizeBox>(PanelBlueprint.WidgetTree->FindWidget(TEXT("SinglePartPanelRoot")))
			: nullptr;
		const UHorizontalBox* PartList = PanelBlueprint.WidgetTree
			? Cast<UHorizontalBox>(PanelBlueprint.WidgetTree->FindWidget(TEXT("PartList")))
			: nullptr;
		const USizeBox* EntryRoot = EntryBlueprint.WidgetTree
			? Cast<USizeBox>(EntryBlueprint.WidgetTree->FindWidget(TEXT("SinglePartEntryRoot")))
			: nullptr;
		const USizeBox* CompactSize = EntryBlueprint.WidgetTree
			? Cast<USizeBox>(EntryBlueprint.WidgetTree->FindWidget(TEXT("CompactSize")))
			: nullptr;
		return PanelRoot
			&& PartList
			&& EntryRoot
			&& CompactSize
			&& PanelRoot->IsWidthOverride()
			&& FMath::IsNearlyEqual(PanelRoot->GetWidthOverride(), 250.0f)
			&& !PanelRoot->IsMinDesiredWidthOverride()
			&& !EntryRoot->IsWidthOverride()
			&& !EntryRoot->IsMinDesiredWidthOverride()
			&& !CompactSize->IsWidthOverride()
			&& CompactSize->IsHeightOverride()
			&& FMath::IsNearlyEqual(CompactSize->GetHeightOverride(), 84.0f);
	}
}

bool UWacomEnemyUIToolset::EnsureInspectionPanelAnimations(
	UWidgetBlueprint* WidgetBlueprint,
	UWidget* LeftPanel,
	UWidget* RightPanel)
{
	if (!WidgetBlueprint
		|| WidgetBlueprint->GetOutermost()->GetName() != InspectionPackageName
		|| !WidgetBlueprint->ParentClass
		|| !WidgetBlueprint->ParentClass->IsChildOf(
			UWacomBattleEnemyInspectionWidget::StaticClass())
		|| !WidgetBlueprint->WidgetTree
		|| WidgetBlueprint->WidgetTree->FindWidget(TEXT("LeftPanel")) != LeftPanel
		|| WidgetBlueprint->WidgetTree->FindWidget(TEXT("RightPanel")) != RightPanel)
	{
		return false;
	}

	WidgetBlueprint->Modify();
	const bool bOpenLeftValid = EnsureAnimation(
		*WidgetBlueprint,
		TEXT("OpenLeftAnimation"),
		0.22f,
		{ { LeftPanel, -360.0f, 0.0f } });
	const bool bOpenRightValid = EnsureAnimation(
		*WidgetBlueprint,
		TEXT("OpenRightAnimation"),
		0.22f,
		{ { RightPanel, 360.0f, 0.0f } });
	const bool bCloseValid = EnsureAnimation(
		*WidgetBlueprint,
		TEXT("CloseAnimation"),
		0.18f,
		{
			{ LeftPanel, 0.0f, -360.0f },
			{ RightPanel, 0.0f, 360.0f },
		});
	if (!bOpenLeftValid || !bOpenRightValid || !bCloseValid)
	{
		return false;
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);
	return true;
}

bool UWacomEnemyUIToolset::EnsureSegmentedUIContractMarker(
	UWidgetBlueprint* WidgetBlueprint)
{
	if (!WidgetBlueprint)
	{
		return false;
	}

	const TCHAR* ExpectedMarker = ResolveContractMarker(*WidgetBlueprint);
	if (!ExpectedMarker)
	{
		return false;
	}

	TArray<FString> DescriptionLines;
	WidgetBlueprint->BlueprintDescription.ParseIntoArrayLines(
		DescriptionLines, false);
	DescriptionLines.RemoveAll(
		[](const FString& Line)
		{
			return IsEnemyUIContractLine(Line.TrimStartAndEnd());
		});
	DescriptionLines.Add(ExpectedMarker);
	const FString ExpectedDescription = FString::Join(DescriptionLines, TEXT("\n"));
	if (WidgetBlueprint->BlueprintDescription == ExpectedDescription)
	{
		return true;
	}

	WidgetBlueprint->Modify();
	WidgetBlueprint->BlueprintDescription = ExpectedDescription;
	FBlueprintEditorUtils::MarkBlueprintAsModified(WidgetBlueprint);
	return true;
}

bool UWacomEnemyUIToolset::NormalizeSinglePartPanelGeometry(
	UWidgetBlueprint* PanelBlueprint,
	UWidgetBlueprint* EntryBlueprint)
{
	if (!PanelBlueprint
		|| !EntryBlueprint
		|| PanelBlueprint->GetOutermost()->GetName() != SinglePanelPackageName
		|| EntryBlueprint->GetOutermost()->GetName() != SingleEntryPackageName
		|| !PanelBlueprint->WidgetTree
		|| !EntryBlueprint->WidgetTree
		|| !Cast<UHorizontalBox>(PanelBlueprint->WidgetTree->FindWidget(TEXT("PartList"))))
	{
		return false;
	}

	USizeBox* PanelRoot = Cast<USizeBox>(
		PanelBlueprint->WidgetTree->FindWidget(TEXT("SinglePartPanelRoot")));
	USizeBox* EntryRoot = Cast<USizeBox>(
		EntryBlueprint->WidgetTree->FindWidget(TEXT("SinglePartEntryRoot")));
	USizeBox* CompactSize = Cast<USizeBox>(
		EntryBlueprint->WidgetTree->FindWidget(TEXT("CompactSize")));
	if (!PanelRoot || !EntryRoot || !CompactSize)
	{
		return false;
	}

	if (HasExpectedSinglePartGeometry(*PanelBlueprint, *EntryBlueprint))
	{
		return true;
	}

	PanelBlueprint->Modify();
	EntryBlueprint->Modify();
	PanelRoot->Modify();
	EntryRoot->Modify();
	CompactSize->Modify();
	PanelRoot->SetWidthOverride(250.0f);
	PanelRoot->ClearMinDesiredWidth();
	EntryRoot->ClearWidthOverride();
	EntryRoot->ClearMinDesiredWidth();
	CompactSize->ClearWidthOverride();
	CompactSize->SetHeightOverride(84.0f);
	FBlueprintEditorUtils::MarkBlueprintAsModified(PanelBlueprint);
	FBlueprintEditorUtils::MarkBlueprintAsModified(EntryBlueprint);
	return HasExpectedSinglePartGeometry(*PanelBlueprint, *EntryBlueprint);
}

bool UWacomEnemyUIToolset::NormalizeInteractiveHitTestPaths(
	UWidgetBlueprint* WidgetBlueprint)
{
	if (!WidgetBlueprint || !ResolveContractMarker(*WidgetBlueprint))
	{
		return false;
	}

	int32 ChangedWidgetCount = 0;
	if (!Wacom::ContentBuilder::EnemyUIHitTestPolicy::NormalizeInteractiveRoutes(
		*WidgetBlueprint,
		ChangedWidgetCount))
	{
		return false;
	}
	if (ChangedWidgetCount > 0)
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(WidgetBlueprint);
	}
	return true;
}
