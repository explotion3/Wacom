// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Settings/WacomLocalSettingsTypes.h"
#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"
#include "UI/WacomBattleEnemyPartEntryWidgetTestAccess.h"

namespace WacomBattleEnemyPanelVitalsMotionSpec
{
	constexpr TCHAR EntryClassPath[] =
		TEXT("/Game/Wacom/UI/Enemy/BP_WacomBattleEnemyPartEntryWidget.BP_WacomBattleEnemyPartEntryWidget_C");

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
	Entry->SetPartEntryViewData(View);
	Entry->StopAllAnimations();

	FWacomBattleEnemyPartEntryViewData Preview = View;
	Preview.CurrentHp = 4;
	Preview.Shield = 0;
	Preview.CurrentInitiative = 0;
	Preview.CurrentIntentId = TEXT("TrainingWarrior.Body.Guard");
	Preview.bDestroyed = true;
	Entry->SetActionPreview(Preview);
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
	Entry->ClearActionPreview();
	Entry->StopAllAnimations();

	View.CurrentHp = 18;
	Entry->SetPartEntryViewData(View);
	TestTrue(TEXT("Real HP loss plays Damage once"), Entry->IsAnimationPlaying(Damage));
	TestTrue(TEXT("Damage trail starts at previous HP"), FMath::IsNearlyEqual(
		FWacomBattleEnemyPartEntryWidgetTestAccess::GetDamageTrailStartPercent(*Entry), 1.0f));
	Entry->StopAllAnimations();
	Entry->SetPartEntryViewData(View);
	TestFalse(TEXT("Identical Snapshot does not replay Damage"), Entry->IsAnimationPlaying(Damage));

	View.Shield = 2;
	Entry->SetPartEntryViewData(View);
	TestTrue(TEXT("Shield change plays Shield Impact"), Entry->IsAnimationPlaying(Shield));
	Entry->StopAllAnimations();
	View.Shield = 0;
	Entry->SetPartEntryViewData(View);
	TestTrue(TEXT("Positive to zero plays Shield Break"), Entry->IsAnimationPlaying(ShieldBreak));
	Entry->StopAllAnimations();

	View.CurrentInitiative = 1;
	View.CurrentIntentId = TEXT("TrainingWarrior.Body.Guard");
	Entry->SetPartEntryViewData(View);
	TestTrue(TEXT("Real Initiative step animates"), Entry->IsAnimationPlaying(Initiative));
	TestTrue(TEXT("Real Intent change animates"), Entry->IsAnimationPlaying(Intent));
	Entry->StopAllAnimations();

	View.CurrentHp = 0;
	View.bDestroyed = true;
	View.CurrentInitiative = 0;
	View.CurrentIntentId = TEXT("TrainingWarrior.Body.Cleave");
	Entry->SetPartEntryViewData(View);
	TestTrue(TEXT("Destroyed transition animates"), Entry->IsAnimationPlaying(Destroyed));
	TestFalse(TEXT("Destroyed suppresses Initiative"), Entry->IsAnimationPlaying(Initiative));
	TestFalse(TEXT("Destroyed suppresses Intent"), Entry->IsAnimationPlaying(Intent));
	TestTrue(TEXT("Destroyed material terminal amount"), FMath::IsNearlyEqual(
		FWacomBattleEnemyPartEntryWidgetTestAccess::GetMaterialScalar(*Entry, TEXT("DestroyedAmount")), 1.0f));
	Entry->CancelPendingPresentation();
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
	Entry->SetPartEntryViewData(View);
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
	Entry->SetPartEntryViewData(View);
	TestFalse(TEXT("Simplified Motion suppresses positional Damage animation"),
		Entry->IsAnimationPlaying(FindAnimation(Entry, TEXT("DamageImpactAnimation"))));
	Entry->CancelPendingPresentation();
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
