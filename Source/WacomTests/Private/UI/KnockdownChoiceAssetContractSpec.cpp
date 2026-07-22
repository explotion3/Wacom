// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "UI/Battle/WacomKnockdownChoiceDialog.h"
#include "UI/Battle/WacomKnockdownChoiceOptionWidget.h"
#include "UI/Card/WacomCardView.h"
#include "UI/Foundation/WacomUIDeveloperSettings.h"
#include "UI/Foundation/WacomUITags.h"

namespace
{
	constexpr TCHAR DialogClassPath[] =
		TEXT("/Game/Wacom/UI/Battle/Knockdown/WBP_BattleKnockdownChoiceDialog.WBP_BattleKnockdownChoiceDialog_C");
	constexpr TCHAR OptionClassPath[] =
		TEXT("/Game/Wacom/UI/Battle/Knockdown/WBP_BattleKnockdownChoiceOption.WBP_BattleKnockdownChoiceOption_C");
	constexpr TCHAR CardViewClassPath[] =
		TEXT("/Game/Wacom/UI/Card/WBP_CardView.WBP_CardView_C");

	UWidgetTree* GetWidgetTreeArchetype(UClass* WidgetClass)
	{
		const UWidgetBlueprintGeneratedClass* GeneratedClass =
			Cast<UWidgetBlueprintGeneratedClass>(WidgetClass);
		return GeneratedClass ? GeneratedClass->GetWidgetTreeArchetype() : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomKnockdownChoiceFormalAssetContractSpec,
	"Wacom.UI.Battle.KnockdownChoice.AssetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomKnockdownChoiceFormalAssetContractSpec::RunTest(
	const FString& /*Parameters*/)
{
	UClass* DialogClass = LoadObject<UClass>(nullptr, DialogClassPath);
	UClass* OptionClass = LoadObject<UClass>(nullptr, OptionClassPath);
	UClass* CardViewClass = LoadObject<UClass>(nullptr, CardViewClassPath);
	if (!TestNotNull(TEXT("Formal Dialog WBP loads"), DialogClass)
		|| !TestNotNull(TEXT("Formal Option WBP loads"), OptionClass)
		|| !TestNotNull(TEXT("Generic CardView WBP loads"), CardViewClass))
	{
		return false;
	}

	TestTrue(TEXT("Dialog WBP has exact native parent contract"),
		DialogClass->IsChildOf(UWacomKnockdownChoiceDialog::StaticClass()));
	TestTrue(TEXT("Option WBP has exact native parent contract"),
		OptionClass->IsChildOf(UWacomKnockdownChoiceOptionWidget::StaticClass()));

	const UWacomUIDeveloperSettings* Settings =
		GetDefault<UWacomUIDeveloperSettings>();
	const FWacomUIWidgetClassEntry* Entry = Settings
		? Settings->WidgetClasses.FindByPredicate(
			[](const FWacomUIWidgetClassEntry& Candidate)
			{
				return Candidate.WidgetTag ==
					WacomUITags::UI_Widget_BattleKnockdownChoiceDialog.GetTag();
			})
		: nullptr;
	if (TestNotNull(TEXT("Dialog tag is registered"), Entry))
	{
		TestEqual(TEXT("Dialog tag resolves to formal WBP"),
			Entry->WidgetClass.ToSoftObjectPath().ToString(),
			FString(DialogClassPath));
	}

	UWidgetTree* DialogTree = GetWidgetTreeArchetype(DialogClass);
	if (!TestNotNull(TEXT("Dialog has authored WidgetTree"), DialogTree))
	{
		return false;
	}
	for (const FName Binding : {
		FName(TEXT("TitleText")),
		FName(TEXT("PartNameText")),
		FName(TEXT("AidOption")),
		FName(TEXT("WithdrawOption")),
		FName(TEXT("DestroyOption"))})
	{
		TestNotNull(*FString::Printf(TEXT("Dialog binding %s"), *Binding.ToString()),
			DialogTree->FindWidget(Binding));
	}
	for (const FName OptionName : {
		FName(TEXT("AidOption")),
		FName(TEXT("WithdrawOption")),
		FName(TEXT("DestroyOption"))})
	{
		const UWidget* OptionWidget = DialogTree->FindWidget(OptionName);
		TestTrue(*FString::Printf(TEXT("%s uses formal Option WBP"), *OptionName.ToString()),
			OptionWidget && OptionWidget->IsA(OptionClass));
	}
	const UWidgetBlueprintGeneratedClass* DialogGeneratedClass =
		Cast<UWidgetBlueprintGeneratedClass>(DialogClass);
	TestTrue(TEXT("Dialog WBP owns submission rejection feedback animation"),
		DialogGeneratedClass
			&& DialogGeneratedClass->Animations.ContainsByPredicate(
				[](const UWidgetAnimation* Animation)
				{
					return Animation
						&& Animation->GetDisplayLabel() ==
							TEXT("SubmissionRejectedAnimation");
				}));

	UWidgetTree* OptionTree = GetWidgetTreeArchetype(OptionClass);
	if (!TestNotNull(TEXT("Option has authored WidgetTree"), OptionTree))
	{
		return false;
	}
	for (const FName Binding : {
		FName(TEXT("BranchLabelText")),
		FName(TEXT("ChoiceLabelText")),
		FName(TEXT("DescriptionText")),
		FName(TEXT("RewardCardHost")),
		FName(TEXT("RewardFallbackText")),
		FName(TEXT("DisabledReasonText"))})
	{
		TestNotNull(*FString::Printf(TEXT("Option binding %s"), *Binding.ToString()),
			OptionTree->FindWidget(Binding));
	}

	const UWacomKnockdownChoiceOptionWidget* OptionDefaults =
		OptionClass->GetDefaultObject<UWacomKnockdownChoiceOptionWidget>();
	TestEqual(TEXT("Option uses generic full CardView, not first-person wrapper"),
		OptionDefaults->GetRewardCardViewClass().Get(),
		CardViewClass);
	return true;
}

#endif
