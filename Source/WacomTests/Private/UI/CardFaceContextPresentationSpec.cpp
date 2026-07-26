// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Engine/Texture2D.h"
#include "RunStateTypes.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Card/WacomCardPresentationBuilder.h"

#include "UObject/StrongObjectPtr.h"

namespace WacomCardFaceContextPresentationSpec
{
	UCardDefinition* MakeDualFaceCard(UObject* Outer, const TCHAR* Id)
	{
		UCardDefinition* Card = NewObject<UCardDefinition>(Outer);
		Card->CardId = Id;
		Card->DisplayName = FText::FromString(TEXT("共享卡名"));
		Card->Description = FText::FromString(TEXT("造成 7 点战斗伤害。"));
		Card->CardIllustration = NewObject<UTexture2D>(Card);
		Card->CardIllustrationDepthMap = NewObject<UTexture2D>(Card);
		Card->BaseCost = 3;
		Card->Rarity = WacomTags::Card_Rarity_Blue;
		Card->Physique.Durability = 4;

		FCardEffect Damage;
		Damage.EffectType = WacomTags::Effect_Damage;
		Damage.Magnitude = 7;
		Card->Effects.Add(Damage);

		Card->RunFace.bEnabled = true;
		Card->RunFace.DisplayNameOverride = FText::FromString(TEXT("探索卡名"));
		Card->RunFace.Description = FText::FromString(TEXT("破坏挡住路线的障碍。"));
		Card->RunFace.IllustrationOverride = NewObject<UTexture2D>(Card);
		Card->RunFace.IllustrationDepthMapOverride = NewObject<UTexture2D>(Card);
		Card->RunFace.TargetMode = EWacomRunCardTargetMode::WorldTarget;
		Card->RunFace.PrimaryAction.ActionTag = WacomTags::Run_Card_Action_Break;
		Card->RunFace.PrimaryAction.Magnitude = 2;
		return Card;
	}

	void TestEquivalentCardFaces(
		FAutomationTestBase& Test,
		const FWacomCardViewData& A,
		const FWacomCardViewData& B)
	{
		Test.TestEqual(TEXT("Equivalent name"), A.Name.ToString(), B.Name.ToString());
		Test.TestEqual(TEXT("Equivalent type"), A.TypeText.ToString(), B.TypeText.ToString());
		Test.TestEqual(
			TEXT("Equivalent semantic token count"),
			A.TypeSemanticTokens.Num(),
			B.TypeSemanticTokens.Num());
		for (int32 Index = 0;
			Index < FMath::Min(
				A.TypeSemanticTokens.Num(),
				B.TypeSemanticTokens.Num());
			++Index)
		{
			Test.TestEqual(
				TEXT("Equivalent semantic identity"),
				A.TypeSemanticTokens[Index].SemanticId,
				B.TypeSemanticTokens[Index].SemanticId);
			Test.TestEqual(
				TEXT("Equivalent semantic display"),
				A.TypeSemanticTokens[Index].DisplayText.ToString(),
				B.TypeSemanticTokens[Index].DisplayText.ToString());
			Test.TestEqual(
				TEXT("Equivalent semantic range start"),
				A.TypeSemanticTokens[Index].StartIndex,
				B.TypeSemanticTokens[Index].StartIndex);
			Test.TestEqual(
				TEXT("Equivalent semantic range length"),
				A.TypeSemanticTokens[Index].Length,
				B.TypeSemanticTokens[Index].Length);
		}
		Test.TestEqual(TEXT("Equivalent description"), A.Description.ToString(), B.Description.ToString());
		Test.TestEqual(TEXT("Equivalent cost"), A.Cost, B.Cost);
		Test.TestEqual(TEXT("Equivalent cost visibility"), A.bShowCost, B.bShowCost);
		Test.TestEqual(TEXT("Equivalent cost preview state"), A.bHasCostPreview, B.bHasCostPreview);
		Test.TestEqual(TEXT("Equivalent cost preview"), A.PreviewCost, B.PreviewCost);
		Test.TestEqual(TEXT("Equivalent rarity"), A.Rarity, B.Rarity);
		Test.TestEqual(TEXT("Equivalent value"), A.Value, B.Value);
		Test.TestEqual(TEXT("Equivalent value visibility"), A.bShowValue, B.bShowValue);
		Test.TestEqual(TEXT("Equivalent physique"), A.PhysiqueText.ToString(), B.PhysiqueText.ToString());
		Test.TestEqual(TEXT("Equivalent physique visibility"), A.bShowPhysique, B.bShowPhysique);
		Test.TestEqual(TEXT("Equivalent badge count"), A.EffectBadges.Num(), B.EffectBadges.Num());
		Test.TestEqual(TEXT("Equivalent disabled state"), A.bDisabled, B.bDisabled);
		Test.TestEqual(TEXT("Equivalent durability"), A.Durability, B.Durability);
		Test.TestEqual(TEXT("Equivalent durability visibility"), A.bShowDurability, B.bShowDurability);
		Test.TestEqual(TEXT("Equivalent illustration"), A.Art.Get(), B.Art.Get());
		Test.TestEqual(TEXT("Equivalent depth map"), A.ArtDepthMap.Get(), B.ArtDepthMap.Get());
	}

	bool DetailContainsText(
		const FWacomCardDetailViewData& Detail,
		const FString& ExpectedText)
	{
		for (const FWacomCardDetailSection& Section : Detail.Sections)
		{
			for (const FWacomCardDetailBlock& Block : Section.Blocks)
			{
				for (const FWacomCardDetailRun& Run : Block.Runs)
				{
					if (Run.Text.ToString().Contains(ExpectedText))
					{
						return true;
					}
				}
			}
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUICardFaceContextPresentationSpec,
	"Wacom.UI.CardPresentation.FaceContext",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUICardFaceContextPresentationSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomCardFaceContextPresentationSpec;

	TStrongObjectPtr<UCardDefinition> Card(
		MakeDualFaceCard(GetTransientPackage(), TEXT("FaceContext.Base")));
	FWacomCardPresentationRuntimeContext RuntimeContext;
	RuntimeContext.bHasRuntimeCost = true;
	RuntimeContext.RuntimeCost = 9;
	RuntimeContext.bHasRuntimeCostPreview = true;
	RuntimeContext.RuntimeCostPreview = 8;
	RuntimeContext.bHasPlayableState = true;
	RuntimeContext.bIsPlayable = true;
	FWacomCardPresentationRuntimeContext::FEffectPreview EffectPreview;
	EffectPreview.EffectIndex = 0;
	EffectPreview.bHasMagnitude = true;
	EffectPreview.Magnitude = 99;
	RuntimeContext.EffectPreviews.Add(EffectPreview);

	const FWacomCardViewData LegacyBattle =
		UWacomCardPresentationBuilder::BuildCardViewData(Card.Get(), RuntimeContext);
	const FWacomCardViewData ExplicitBattle =
		UWacomCardPresentationBuilder::BuildCardViewData(
			Card.Get(),
			EWacomCardFaceContext::Battle,
			RuntimeContext);
	TestEquivalentCardFaces(*this, LegacyBattle, ExplicitBattle);

	const FWacomCardViewData Run =
		UWacomCardPresentationBuilder::BuildCardViewData(
			Card.Get(),
			EWacomCardFaceContext::Run,
			RuntimeContext);
	TestEqual(TEXT("Run name uses override"), Run.Name.ToString(), FString(TEXT("探索卡名")));
	TestEqual(TEXT("Run type is exploration"), Run.TypeText.ToString(), FString(TEXT("探索")));
	TestEqual(
		TEXT("Run description uses RunFace"),
		Run.Description.ToString(),
		FString(TEXT("破坏挡住路线的障碍。")));
	TestEqual(TEXT("Run uses its illustration override"), Run.Art.Get(), Card->RunFace.IllustrationOverride.Get());
	TestEqual(
		TEXT("Run uses its depth map override"),
		Run.ArtDepthMap.Get(),
		Card->RunFace.IllustrationDepthMapOverride.Get());
	TestEqual(TEXT("Run preserves shared rarity"), Run.Rarity, Card->Rarity);
	TestFalse(TEXT("Run hides Battle cost"), Run.bShowCost);
	TestFalse(TEXT("Run ignores Battle cost preview"), Run.bHasCostPreview);
	TestFalse(TEXT("Run hides Battle value"), Run.bShowValue);
	TestFalse(TEXT("Run hides Battle physique"), Run.bShowPhysique);
	TestFalse(TEXT("Run hides Battle durability"), Run.bShowDurability);
	TestTrue(TEXT("Run omits Battle effect badges"), Run.EffectBadges.IsEmpty());
	TestFalse(TEXT("Enabled playable RunFace is interactive"), Run.bDisabled);

	Card->RunFace.DisplayNameOverride = FText::GetEmpty();
	Card->RunFace.IllustrationOverride = nullptr;
	Card->RunFace.IllustrationDepthMapOverride = nullptr;
	const FWacomCardViewData RunFallback =
		UWacomCardPresentationBuilder::BuildCardViewDataForFace(
			Card.Get(),
			EWacomCardFaceContext::Run);
	TestEqual(TEXT("Run name falls back to shared name"), RunFallback.Name.ToString(), FString(TEXT("共享卡名")));
	TestEqual(TEXT("Run art falls back to shared art"), RunFallback.Art.Get(), Card->CardIllustration.Get());
	TestEqual(
		TEXT("Run depth map falls back to shared depth map"),
		RunFallback.ArtDepthMap.Get(),
		Card->CardIllustrationDepthMap.Get());

	RuntimeContext.bIsPlayable = false;
	const FWacomCardViewData UnplayableRun =
		UWacomCardPresentationBuilder::BuildCardViewData(
			Card.Get(),
			EWacomCardFaceContext::Run,
			RuntimeContext);
	TestTrue(TEXT("Runtime playable=false disables RunFace"), UnplayableRun.bDisabled);

	RuntimeContext.bIsPlayable = true;
	Card->RunFace.bEnabled = false;
	const FWacomCardViewData MissingRun =
		UWacomCardPresentationBuilder::BuildCardViewData(
			Card.Get(),
			EWacomCardFaceContext::Run,
			RuntimeContext);
	TestTrue(TEXT("Missing RunFace is safely disabled"), MissingRun.bDisabled);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUICardFaceContextExplanationAndIdentitySpec,
	"Wacom.UI.CardExplanation.RunFaceContext",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUICardFaceContextExplanationAndIdentitySpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomCardFaceContextPresentationSpec;

	TStrongObjectPtr<UCardDefinition> Card(
		MakeDualFaceCard(GetTransientPackage(), TEXT("FaceContext.Identity.Base")));
	const FWacomCardDetailViewData RunDetail =
		UWacomCardPresentationBuilder::BuildCardDetailViewDataForFace(
			Card.Get(),
			EWacomCardFaceContext::Run);
	TestEqual(TEXT("Run detail uses Run name"), RunDetail.Name.ToString(), FString(TEXT("探索卡名")));
	TestEqual(TEXT("Run detail has one description section"), RunDetail.Sections.Num(), 1);
	if (RunDetail.Sections.Num() == 1)
	{
		TestEqual(
			TEXT("Run detail section is Description"),
			RunDetail.Sections[0].Kind,
			EWacomCardDetailSectionKind::Description);
	}
	TestTrue(
		TEXT("Run detail contains Run description"),
		DetailContainsText(RunDetail, TEXT("破坏挡住路线的障碍")));
	TestFalse(
		TEXT("Run detail does not leak Battle description"),
		DetailContainsText(RunDetail, TEXT("战斗伤害")));

	FCardInstance Instance;
	Instance.InstanceId = FGuid::NewGuid();
	Instance.Definition = Card.Get();
	const FGuid StableInstanceId = Instance.InstanceId;

	const FWacomCardViewData BattleFace =
		UWacomCardPresentationBuilder::BuildCardViewDataForFace(
			Instance.Definition,
			EWacomCardFaceContext::Battle);
	const FWacomCardViewData RunFace =
		UWacomCardPresentationBuilder::BuildCardViewDataForFace(
			Instance.Definition,
			EWacomCardFaceContext::Run);
	TestEqual(TEXT("Battle face reads the same Definition"), BattleFace.Name.ToString(), FString(TEXT("共享卡名")));
	TestEqual(TEXT("Run face reads the same Definition"), RunFace.Name.ToString(), FString(TEXT("探索卡名")));
	TestEqual(TEXT("Building either face does not replace InstanceId"), Instance.InstanceId, StableInstanceId);

	TStrongObjectPtr<UCardDefinition> Upgraded(
		MakeDualFaceCard(GetTransientPackage(), TEXT("FaceContext.Identity.Upgraded")));
	Upgraded->DisplayName = FText::FromString(TEXT("强化战斗卡名"));
	Upgraded->RunFace.DisplayNameOverride = FText::FromString(TEXT("强化探索卡名"));
	Instance.Definition = Upgraded.Get();

	const FWacomCardViewData UpgradedBattleFace =
		UWacomCardPresentationBuilder::BuildCardViewDataForFace(
			Instance.Definition,
			EWacomCardFaceContext::Battle);
	const FWacomCardViewData UpgradedRunFace =
		UWacomCardPresentationBuilder::BuildCardViewDataForFace(
			Instance.Definition,
			EWacomCardFaceContext::Run);
	TestEqual(
		TEXT("Definition replacement switches Battle face"),
		UpgradedBattleFace.Name.ToString(),
		FString(TEXT("强化战斗卡名")));
	TestEqual(
		TEXT("Definition replacement switches Run face"),
		UpgradedRunFace.Name.ToString(),
		FString(TEXT("强化探索卡名")));
	TestEqual(TEXT("Definition replacement preserves InstanceId"), Instance.InstanceId, StableInstanceId);
	return true;
}
