// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Settings/WacomLocalSettingsTypes.h"
#include "UI/Battle/WacomBattleEnemyPanelWidget.h"
#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"
#include "UI/WacomBattleEnemyPanelWidgetTestAccess.h"
#include "UI/WacomBattleEnemyPartEntryWidgetTestAccess.h"

namespace WacomBattleEnemyPanelVitalsMotionSpec
{
	constexpr TCHAR EntryClassPath[] =
		TEXT("/Game/Wacom/UI/Enemy/BP_WacomBattleEnemyPartEntryWidget.BP_WacomBattleEnemyPartEntryWidget_C");
	constexpr TCHAR PanelClassPath[] =
		TEXT("/Game/Wacom/UI/Enemy/BP_WacomBattleEnemyPanelWidget.BP_WacomBattleEnemyPanelWidget_C");

	UWorld* FindAutomationWorld()
	{
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (UWorld* World = Context.World())
				{
					return World;
				}
			}
		}
		return GWorld;
	}

	UWidgetAnimation* FindAnimation(UUserWidget* Widget, const FName Name)
	{
		const UWidgetBlueprintGeneratedClass* GeneratedClass = Widget
			? Cast<UWidgetBlueprintGeneratedClass>(Widget->GetClass()) : nullptr;
		if (!GeneratedClass)
		{
			return nullptr;
		}
		for (UWidgetAnimation* Animation : GeneratedClass->Animations)
		{
			if (Animation && (Animation->GetFName() == Name
				|| Animation->GetDisplayLabel() == Name.ToString()
				|| Animation->GetName().StartsWith(Name.ToString())))
			{
				return Animation;
			}
		}
		return nullptr;
	}

	FWacomBattleEnemyPartEntryViewData MakeView()
	{
		FWacomBattleEnemyPartEntryViewData View;
		View.EnemySlotId = TEXT("Enemy");
		View.PartSlotId = TEXT("Body");
		View.Identity = FBattlePartSlotIdentity::Make(TEXT("Encounter"), View.EnemySlotId, View.PartSlotId);
		View.CurrentHp = 24;
		View.MaxHp = 24;
		View.Shield = 4;
		View.CurrentInitiative = 3;
		View.CurrentIntentId = TEXT("TrainingWarrior.Body.Attack");
		View.CurrentIntentDisplayName = FText::FromString(TEXT("攻击"));
		return View;
	}

	FWacomBattleEnemyPanelViewData MakePanelView()
	{
		FWacomBattleEnemyPanelViewData View;
		View.EncounterId = TEXT("Encounter");
		View.EnemySlotId = TEXT("Enemy");
		View.UnitKey = FBattleEnemyUnitKey::Make(View.EncounterId, View.EnemySlotId);
		FWacomBattleEnemyPartEntryViewData Head = MakeView();
		Head.PartSlotId = TEXT("Head");
		Head.Identity = FBattlePartSlotIdentity::Make(
			View.EncounterId,
			View.EnemySlotId,
			Head.PartSlotId);
		FWacomBattleEnemyPartEntryViewData Tail = MakeView();
		Tail.PartSlotId = TEXT("Tail");
		Tail.Identity = FBattlePartSlotIdentity::Make(
			View.EncounterId,
			View.EnemySlotId,
			Tail.PartSlotId);
		View.Parts = { Head, Tail };
		return View;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPanelActionPreviewFrameSpec,
	"Wacom.UI.Battle.EnemyPanel.ActionPreviewFrame.SemanticsAndCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPanelActionPreviewFrameSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyPanelVitalsMotionSpec;
	UWorld* World = FindAutomationWorld();
	UClass* EntryClass = LoadClass<UWacomBattleEnemyPartEntryWidget>(nullptr, EntryClassPath);
	UWacomBattleEnemyPartEntryWidget* Entry = World && EntryClass
		? CreateWidget<UWacomBattleEnemyPartEntryWidget>(World, EntryClass) : nullptr;
	if (!TestNotNull(TEXT("Enemy Part Entry"), Entry))
	{
		return false;
	}

	const FWacomBattleEnemyPartEntryViewData View = MakeView();
	FWacomBattleEnemyPartEntryWidgetTestAccess::SetView(*Entry, View);

	FWacomBattleEnemyPartEntryViewData Preview = View;
	Preview.bActionPreviewPerfectReleaseCandidate = true;
	Preview.bHasResistancePreview = true;
	Preview.ResistancePreviewPlayerPeakDamage = 7;
	Preview.ResistancePreviewEnemyPeakDamage = 3;
	Preview.bResistancePreviewWillStun = true;
	Preview.bActionPreviewWillSkipActionDueToStun = true;
	FWacomBattleEnemyPartEntryWidgetTestAccess::SetPreview(*Entry, Preview);
	TestTrue(TEXT("Successful resistance shows perfect-release surface"),
		FWacomBattleEnemyPartEntryWidgetTestAccess::IsPerfectReleasePreviewVisible(*Entry));
	TestTrue(TEXT("Successful resistance shows compact comparison"),
		FWacomBattleEnemyPartEntryWidgetTestAccess::IsResistanceComparisonVisible(*Entry));
	TestTrue(TEXT("Successful resistance preserves outcome fact"),
		FWacomBattleEnemyPartEntryWidgetTestAccess::IsResistancePreviewSuccessful(*Entry));
	TestEqual(TEXT("Successful resistance player peak"),
		FWacomBattleEnemyPartEntryWidgetTestAccess::GetPreviewPlayerPeakDamage(*Entry), 7);
	TestEqual(TEXT("Successful resistance enemy peak"),
		FWacomBattleEnemyPartEntryWidgetTestAccess::GetPreviewEnemyPeakDamage(*Entry), 3);
	TestEqual(TEXT("Successful resistance comparator"),
		FWacomBattleEnemyPartEntryWidgetTestAccess::GetPreviewComparator(*Entry), FString(TEXT(">")));
	TestTrue(TEXT("Successful resistance exposes immediate skip"),
		FWacomBattleEnemyPartEntryWidgetTestAccess::PreviewWillSkipActionDueToStun(*Entry));

	Preview.ResistancePreviewPlayerPeakDamage = 3;
	Preview.ResistancePreviewEnemyPeakDamage = 3;
	Preview.bResistancePreviewWillStun = false;
	Preview.bActionPreviewWillSkipActionDueToStun = false;
	Preview.bActionPreviewWillAct = true;
	FWacomBattleEnemyPartEntryWidgetTestAccess::SetPreview(*Entry, Preview);
	TestFalse(TEXT("Equal resistance is a failure"),
		FWacomBattleEnemyPartEntryWidgetTestAccess::IsResistancePreviewSuccessful(*Entry));
	TestEqual(TEXT("Equal resistance uses less-than-or-equal comparator"),
		FWacomBattleEnemyPartEntryWidgetTestAccess::GetPreviewComparator(*Entry), FString(TEXT("≤")));
	TestTrue(TEXT("Failed resistance preserves enemy action risk"),
		FWacomBattleEnemyPartEntryWidgetTestAccess::PreviewWillAct(*Entry));

	Preview.bHasResistancePreview = false;
	Preview.bActionPreviewWillAct = false;
	FWacomBattleEnemyPartEntryWidgetTestAccess::SetPreview(*Entry, Preview);
	TestTrue(TEXT("Perfect release against a non-attack intent keeps gold surface"),
		FWacomBattleEnemyPartEntryWidgetTestAccess::IsPerfectReleasePreviewVisible(*Entry));
	TestFalse(TEXT("Non-attack intent has no resistance comparison"),
		FWacomBattleEnemyPartEntryWidgetTestAccess::IsResistanceComparisonVisible(*Entry));

	Preview.bActionPreviewPerfectReleaseCandidate = false;
	Preview.bActionPreviewWillAct = true;
	FWacomBattleEnemyPartEntryWidgetTestAccess::SetPreview(*Entry, Preview);
	TestFalse(TEXT("Ordinary action risk has no perfect-release surface"),
		FWacomBattleEnemyPartEntryWidgetTestAccess::IsPerfectReleasePreviewVisible(*Entry));
	TestTrue(TEXT("Ordinary action risk remains explicit"),
		FWacomBattleEnemyPartEntryWidgetTestAccess::PreviewWillAct(*Entry));

	FWacomLocalSettingsSnapshot Settings;
	Settings.UIMotionMode = EWacomUIMotionMode::Simplified;
	FWacomBattleEnemyPartEntryWidgetTestAccess::ApplyRuntimeSettings(*Entry, Settings);
	TestTrue(TEXT("Reduced Motion keeps identical static action-preview semantics"),
		FWacomBattleEnemyPartEntryWidgetTestAccess::PreviewWillAct(*Entry));

	FWacomBattleEnemyPartEntryWidgetTestAccess::ClearPreview(*Entry);
	TestFalse(TEXT("Preview teardown clears the compact frame"),
		FWacomBattleEnemyPartEntryWidgetTestAccess::IsActionPreviewFrameActive(*Entry));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPanelVitalsMotionSpec,
	"Wacom.UI.Battle.EnemyPanel.VitalsMotion.RealFactsAndPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPanelVitalsMotionSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyPanelVitalsMotionSpec;
	UWorld* World = FindAutomationWorld();
	UClass* EntryClass = LoadClass<UWacomBattleEnemyPartEntryWidget>(nullptr, EntryClassPath);
	UWacomBattleEnemyPartEntryWidget* Entry = World && EntryClass
		? CreateWidget<UWacomBattleEnemyPartEntryWidget>(World, EntryClass) : nullptr;
	if (!TestNotNull(TEXT("Enemy Part Entry"), Entry))
	{
		return false;
	}
	Entry->TakeWidget();
	UWidgetAnimation* Damage = FindAnimation(Entry, TEXT("DamageImpactAnimation"));
	UWidgetAnimation* Shield = FindAnimation(Entry, TEXT("ShieldImpactAnimation"));
	UWidgetAnimation* ShieldBreak = FindAnimation(Entry, TEXT("ShieldBreakAnimation"));
	UWidgetAnimation* Initiative = FindAnimation(Entry, TEXT("InitiativeStepAnimation"));
	UWidgetAnimation* Intent = FindAnimation(Entry, TEXT("IntentChangeAnimation"));
	UWidgetAnimation* Destroyed = FindAnimation(Entry, TEXT("DestroyedAnimation"));
	if (!TestNotNull(TEXT("Damage animation"), Damage)
		|| !TestNotNull(TEXT("Shield animation"), Shield)
		|| !TestNotNull(TEXT("Shield-break animation"), ShieldBreak)
		|| !TestNotNull(TEXT("Initiative animation"), Initiative)
		|| !TestNotNull(TEXT("Intent animation"), Intent)
		|| !TestNotNull(TEXT("Destroyed animation"), Destroyed))
	{
		return false;
	}

	FWacomBattleEnemyPartEntryViewData View = MakeView();
	FWacomBattleEnemyPartEntryWidgetTestAccess::SetView(*Entry, View);
	Entry->StopAllAnimations();

	FWacomBattleEnemyPartEntryViewData Preview = View;
	Preview.CurrentHp = 4;
	Preview.Shield = 0;
	Preview.CurrentInitiative = 0;
	Preview.CurrentIntentId = TEXT("TrainingWarrior.Body.Guard");
	Preview.bDestroyed = true;
	FWacomBattleEnemyPartEntryWidgetTestAccess::SetPreview(*Entry, Preview);
	TestTrue(TEXT("Preview HP parameter is projected"), FMath::IsNearlyEqual(
		FWacomBattleEnemyPartEntryWidgetTestAccess::GetMaterialScalar(*Entry, TEXT("HpPreviewPercent")),
		4.0f / 24.0f));
	TestTrue(TEXT("Preview mode is active"), FMath::IsNearlyEqual(
		FWacomBattleEnemyPartEntryWidgetTestAccess::GetMaterialScalar(*Entry, TEXT("HpPreviewMode")), 1.0f));
	TestFalse(TEXT("Preview does not play Damage"), Entry->IsAnimationPlaying(Damage));
	TestFalse(TEXT("Preview does not play Shield"), Entry->IsAnimationPlaying(Shield));
	TestFalse(TEXT("Preview does not play Shield Break"), Entry->IsAnimationPlaying(ShieldBreak));
	TestFalse(TEXT("Preview does not play Initiative"), Entry->IsAnimationPlaying(Initiative));
	TestFalse(TEXT("Preview does not play Intent"), Entry->IsAnimationPlaying(Intent));
	TestFalse(TEXT("Preview does not play Destroyed"), Entry->IsAnimationPlaying(Destroyed));
	FWacomBattleEnemyPartEntryWidgetTestAccess::ClearPreview(*Entry);
	Entry->StopAllAnimations();

	View.CurrentHp = 18;
	FWacomBattleEnemyPartEntryWidgetTestAccess::SetView(*Entry, View);
	TestTrue(TEXT("Real HP loss plays Damage once"), Entry->IsAnimationPlaying(Damage));
	TestTrue(TEXT("Damage trail starts at previous HP"), FMath::IsNearlyEqual(
		FWacomBattleEnemyPartEntryWidgetTestAccess::GetDamageTrailStartPercent(*Entry), 1.0f));
	Entry->StopAllAnimations();
	FWacomBattleEnemyPartEntryWidgetTestAccess::SetView(*Entry, View);
	TestFalse(TEXT("Identical Snapshot does not replay Damage"), Entry->IsAnimationPlaying(Damage));

	View.Shield = 2;
	FWacomBattleEnemyPartEntryWidgetTestAccess::SetView(*Entry, View);
	TestTrue(TEXT("Shield change plays Shield Impact"), Entry->IsAnimationPlaying(Shield));
	Entry->StopAllAnimations();
	View.Shield = 0;
	FWacomBattleEnemyPartEntryWidgetTestAccess::SetView(*Entry, View);
	TestTrue(TEXT("Positive to zero plays Shield Break"), Entry->IsAnimationPlaying(ShieldBreak));
	Entry->StopAllAnimations();

	View.CurrentInitiative = 1;
	View.CurrentIntentId = TEXT("TrainingWarrior.Body.Guard");
	FWacomBattleEnemyPartEntryWidgetTestAccess::SetView(*Entry, View);
	TestTrue(TEXT("Real Initiative step animates"), Entry->IsAnimationPlaying(Initiative));
	TestTrue(TEXT("Real Intent change animates"), Entry->IsAnimationPlaying(Intent));
	Entry->StopAllAnimations();

	View.CurrentHp = 0;
	View.bDestroyed = true;
	View.CurrentInitiative = 0;
	View.CurrentIntentId = TEXT("TrainingWarrior.Body.Cleave");
	FWacomBattleEnemyPartEntryWidgetTestAccess::SetView(*Entry, View);
	TestTrue(TEXT("Destroyed transition animates"), Entry->IsAnimationPlaying(Destroyed));
	TestFalse(TEXT("Destroyed suppresses Initiative"), Entry->IsAnimationPlaying(Initiative));
	TestFalse(TEXT("Destroyed suppresses Intent"), Entry->IsAnimationPlaying(Intent));
	TestTrue(TEXT("Destroyed material terminal amount"), FMath::IsNearlyEqual(
		FWacomBattleEnemyPartEntryWidgetTestAccess::GetMaterialScalar(*Entry, TEXT("DestroyedAmount")), 1.0f));
	FWacomBattleEnemyPartEntryWidgetTestAccess::CancelPresentation(*Entry);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPanelVitalsAccessibilitySpec,
	"Wacom.UI.Battle.EnemyPanel.VitalsMotion.AccessibilityAndLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPanelVitalsAccessibilitySpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyPanelVitalsMotionSpec;
	UWorld* World = FindAutomationWorld();
	UClass* EntryClass = LoadClass<UWacomBattleEnemyPartEntryWidget>(nullptr, EntryClassPath);
	UWacomBattleEnemyPartEntryWidget* Entry = World && EntryClass
		? CreateWidget<UWacomBattleEnemyPartEntryWidget>(World, EntryClass) : nullptr;
	if (!TestNotNull(TEXT("Enemy Part Entry"), Entry))
	{
		return false;
	}
	Entry->TakeWidget();
	FWacomBattleEnemyPartEntryViewData View = MakeView();
	FWacomBattleEnemyPartEntryWidgetTestAccess::SetView(*Entry, View);
	Entry->StopAllAnimations();

	FWacomLocalSettingsSnapshot Settings;
	Settings.UIMotionMode = EWacomUIMotionMode::Full;
	Settings.FlashEffectMode = EWacomFlashEffectMode::Reduced;
	FWacomBattleEnemyPartEntryWidgetTestAccess::ApplyRuntimeSettings(*Entry, Settings);
	TestTrue(TEXT("Flash Reduced maps to 35 percent"), FMath::IsNearlyEqual(
		FWacomBattleEnemyPartEntryWidgetTestAccess::GetRuntimeFlashIntensity(*Entry), 0.35f));
	TestTrue(TEXT("MID receives Flash Reduced"), FMath::IsNearlyEqual(
		FWacomBattleEnemyPartEntryWidgetTestAccess::GetMaterialScalar(*Entry, TEXT("FlashIntensity")), 0.35f));

	Settings.FlashEffectMode = EWacomFlashEffectMode::Off;
	FWacomBattleEnemyPartEntryWidgetTestAccess::ApplyRuntimeSettings(*Entry, Settings);
	TestTrue(TEXT("Flash Off is zero"), FMath::IsNearlyZero(
		FWacomBattleEnemyPartEntryWidgetTestAccess::GetMaterialScalar(*Entry, TEXT("FlashIntensity"))));

	Settings.UIMotionMode = EWacomUIMotionMode::Simplified;
	FWacomBattleEnemyPartEntryWidgetTestAccess::ApplyRuntimeSettings(*Entry, Settings);
	TestTrue(TEXT("Simplified Motion is retained"),
		FWacomBattleEnemyPartEntryWidgetTestAccess::IsUsingSimplifiedMotion(*Entry));
	TestTrue(TEXT("MID receives ReducedMotion"), FMath::IsNearlyEqual(
		FWacomBattleEnemyPartEntryWidgetTestAccess::GetMaterialScalar(*Entry, TEXT("ReducedMotion")), 1.0f));
	TestTrue(TEXT("Simplified Motion removes damage hold"), FMath::IsNearlyZero(
		FWacomBattleEnemyPartEntryWidgetTestAccess::GetMaterialScalar(*Entry, TEXT("DamageTrailHoldSeconds"))));

	View.CurrentHp = 12;
	FWacomBattleEnemyPartEntryWidgetTestAccess::SetView(*Entry, View);
	TestFalse(TEXT("Simplified Motion suppresses positional Damage animation"),
		Entry->IsAnimationPlaying(FindAnimation(Entry, TEXT("DamageImpactAnimation"))));
	FWacomBattleEnemyPartEntryWidgetTestAccess::CancelPresentation(*Entry);
	for (const FName AnimationName : {
		FName(TEXT("IntroAnimation")), FName(TEXT("DamageImpactAnimation")),
		FName(TEXT("ShieldImpactAnimation")), FName(TEXT("ShieldBreakAnimation")),
		FName(TEXT("InitiativeStepAnimation")), FName(TEXT("IntentChangeAnimation")),
		FName(TEXT("ContextAnimation")), FName(TEXT("DestroyedAnimation")) })
	{
		TestFalse(
			*FString::Printf(TEXT("Cancel stops %s"), *AnimationName.ToString()),
			Entry->IsAnimationPlaying(FindAnimation(Entry, AnimationName)));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyPanelVitalsPanelSettingsSpec,
	"Wacom.UI.Battle.EnemyPanel.VitalsMotion.PanelOwnsSettingsPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyPanelVitalsPanelSettingsSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyPanelVitalsMotionSpec;
	UWorld* World = FindAutomationWorld();
	UClass* PanelClass = LoadClass<UWacomBattleEnemyPanelWidget>(nullptr, PanelClassPath);
	UWacomBattleEnemyPanelWidget* Panel = World && PanelClass
		? CreateWidget<UWacomBattleEnemyPanelWidget>(World, PanelClass)
		: nullptr;
	if (!TestNotNull(TEXT("Enemy Panel"), Panel))
	{
		return false;
	}
	Panel->TakeWidget();
	Panel->SetEnemyPanelViewData(MakePanelView());
	UHorizontalBox* PartList = Panel->WidgetTree
		? Cast<UHorizontalBox>(Panel->WidgetTree->FindWidget(TEXT("PartList")))
		: nullptr;
	if (!TestNotNull(TEXT("Panel PartList"), PartList)
		|| !TestEqual(TEXT("Two stable entries"), PartList->GetChildrenCount(), 2))
	{
		return false;
	}

	FWacomLocalSettingsSnapshot Settings;
	Settings.UIMotionMode = EWacomUIMotionMode::Simplified;
	Settings.FlashEffectMode = EWacomFlashEffectMode::Reduced;
	FWacomBattleEnemyPanelWidgetTestAccess::ApplyRuntimeSettings(*Panel, Settings);
	for (int32 Index = 0; Index < PartList->GetChildrenCount(); ++Index)
	{
		UWacomBattleEnemyPartEntryWidget* Entry =
			Cast<UWacomBattleEnemyPartEntryWidget>(PartList->GetChildAt(Index));
		if (!TestNotNull(*FString::Printf(TEXT("Entry %d"), Index), Entry))
		{
			return false;
		}
		TestTrue(*FString::Printf(TEXT("Entry %d receives Simplified Motion"), Index),
			FWacomBattleEnemyPartEntryWidgetTestAccess::IsUsingSimplifiedMotion(*Entry));
		TestTrue(*FString::Printf(TEXT("Entry %d receives Flash Reduced"), Index),
			FMath::IsNearlyEqual(
				FWacomBattleEnemyPartEntryWidgetTestAccess::GetMaterialScalar(
					*Entry,
					TEXT("FlashIntensity")),
				0.35f));
	}

	FWacomBattleEnemyPanelWidgetTestAccess::Destruct(*Panel);
	TestFalse(TEXT("Panel destruct removes its settings subscription"),
		FWacomBattleEnemyPanelWidgetTestAccess::HasRuntimeSettingsSubscription(*Panel));
	return true;
}
