// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Tags/WacomGameplayTags.h"
#include "Validation/CardDefinitionValidation.h"
#include "Validation/CardUpgradeCatalogValidation.h"

#include "UObject/StrongObjectPtr.h"

namespace WacomCardUpgradeValidationSpec
{
	UCardDefinition* MakeCard(
		UObject* Outer,
		const TCHAR* CardId,
		const TCHAR* FamilyId,
		const FGameplayTag& Rarity,
		const int32 BaseCost)
	{
		UCardDefinition* Card = NewObject<UCardDefinition>(Outer);
		Card->CardId = FName(CardId);
		Card->UpgradeFamilyId = FamilyId && FCString::Strlen(FamilyId) > 0
			? FName(FamilyId)
			: NAME_None;
		Card->DisplayName = FText::FromString(CardId);
		Card->BaseCost = BaseCost;
		Card->Rarity = Rarity;
		return Card;
	}

	bool ContainsError(const TArray<FText>& Errors, const TCHAR* Fragment)
	{
		return Errors.ContainsByPredicate(
			[Fragment](const FText& Error)
			{
				return Error.ToString().Contains(Fragment);
			});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataCardUpgradeIdentityAndValidChainSpec,
	"Wacom.Data.CardUpgrade.IdentityAndValidChain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataCardUpgradeIdentityAndValidChainSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomCardUpgradeValidationSpec;
	TStrongObjectPtr<UCardDefinition> Root(NewObject<UCardDefinition>());
	UCardDefinition* Legacy = MakeCard(
		Root.Get(), TEXT("Legacy.Card"), TEXT(""), WacomTags::Card_Rarity_White, 1);
	TestEqual(TEXT("Legacy family falls back to CardId"), Legacy->ResolveUpgradeFamilyId(), Legacy->CardId);
	TestTrue(TEXT("Legacy exact id matches"), Legacy->MatchesCardIdOrUpgradeFamily(TEXT("Legacy.Card")));
	TestFalse(TEXT("None never matches"), Legacy->MatchesCardIdOrUpgradeFamily(NAME_None));

	UCardDefinition* White = MakeCard(
		Root.Get(), TEXT("Upgrade.Card.White"), TEXT("Upgrade.Card"), WacomTags::Card_Rarity_White, 3);
	UCardDefinition* Blue = MakeCard(
		Root.Get(), TEXT("Upgrade.Card.Blue"), TEXT("Upgrade.Card"), WacomTags::Card_Rarity_Blue, 2);
	UCardDefinition* Yellow = MakeCard(
		Root.Get(), TEXT("Upgrade.Card.Yellow"), TEXT("Upgrade.Card"), WacomTags::Card_Rarity_Yellow, 1);
	UCardDefinition* Purple = MakeCard(
		Root.Get(), TEXT("Upgrade.Card.Purple"), TEXT("Upgrade.Card"), WacomTags::Card_Rarity_Purple, 0);
	White->NextUpgradeDefinition = Blue;
	Blue->NextUpgradeDefinition = Yellow;
	Yellow->NextUpgradeDefinition = Purple;

	TArray<FText> Errors;
	TestTrue(TEXT("Reachable four-tier chain validates"), FWacomCardDefinitionValidation::Validate(White, Errors));
	TestEqual(TEXT("Reachable validation has no errors"), Errors.Num(), 0);
	TestTrue(TEXT("Variant exact CardId matches"), Purple->MatchesCardIdOrUpgradeFamily(Purple->CardId));
	TestTrue(TEXT("Variant family matches"), Purple->MatchesCardIdOrUpgradeFamily(TEXT("Upgrade.Card")));

	const TArray<const UCardDefinition*> Catalog = { White, Blue, Yellow, Purple, Legacy };
	TestTrue(TEXT("Connected catalog validates"), FWacomCardUpgradeCatalogValidation::Validate(Catalog, Errors));
	TestEqual(TEXT("Catalog validation has no errors"), Errors.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataCardUpgradeDirectContractRejectionsSpec,
	"Wacom.Data.CardUpgrade.DirectContractRejections",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataCardUpgradeDirectContractRejectionsSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomCardUpgradeValidationSpec;
	TStrongObjectPtr<UCardDefinition> Root(NewObject<UCardDefinition>());
	TArray<FText> Errors;

	auto ExpectInvalid = [this, &Errors](UCardDefinition* Card, const TCHAR* ErrorFragment)
	{
		Errors.Reset();
		TestFalse(FString::Printf(TEXT("Invalid chain rejected: %s"), ErrorFragment),
			FWacomCardDefinitionValidation::Validate(Card, Errors));
		TestTrue(FString::Printf(TEXT("Error mentions: %s"), ErrorFragment),
			ContainsError(Errors, ErrorFragment));
	};

	UCardDefinition* White = MakeCard(
		Root.Get(), TEXT("Upgrade.Invalid.White"), TEXT("Upgrade.Invalid"), WacomTags::Card_Rarity_White, 1);
	UCardDefinition* Yellow = MakeCard(
		Root.Get(), TEXT("Upgrade.Invalid.Yellow"), TEXT("Upgrade.Invalid"), WacomTags::Card_Rarity_Yellow, 0);
	White->NextUpgradeDefinition = Yellow;
	ExpectInvalid(White, TEXT("相邻稀有度"));

	UCardDefinition* BlueOtherFamily = MakeCard(
		Root.Get(), TEXT("Upgrade.Other.Blue"), TEXT("Upgrade.Other"), WacomTags::Card_Rarity_Blue, 0);
	White->NextUpgradeDefinition = BlueOtherFamily;
	ExpectInvalid(White, TEXT("UpgradeFamilyId"));

	UCardDefinition* BlueSameValue = MakeCard(
		Root.Get(), TEXT("Upgrade.Invalid.Blue"), TEXT("Upgrade.Invalid"), WacomTags::Card_Rarity_Blue, 1);
	White->NextUpgradeDefinition = BlueSameValue;
	ExpectInvalid(White, TEXT("规则数值"));

	BlueSameValue->BaseCost = 0;
	BlueSameValue->TargetMode = ECardTargetMode::SingleEnemyPart;
	ExpectInvalid(White, TEXT("TargetMode"));
	BlueSameValue->TargetMode = White->TargetMode;

	White->CardId = TEXT("Card.Run.InvalidUpgrade");
	ExpectInvalid(White, TEXT("Card.Run"));
	White->CardId = TEXT("Upgrade.Invalid.White");

	White->Physique.Capacity = 2;
	ExpectInvalid(White, TEXT("容器卡"));
	White->Physique.Capacity = 0;

	White->Rarity = WacomTags::Card_Rarity_Intrinsic;
	ExpectInvalid(White, TEXT("Intrinsic"));
	White->Rarity = WacomTags::Card_Rarity_White;

	White->NextUpgradeDefinition = White;
	ExpectInvalid(White, TEXT("循环"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataCardUpgradeCatalogRejectionsSpec,
	"Wacom.Data.CardUpgrade.CatalogRejections",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataCardUpgradeCatalogRejectionsSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomCardUpgradeValidationSpec;
	TStrongObjectPtr<UCardDefinition> Root(NewObject<UCardDefinition>());
	UCardDefinition* WhiteA = MakeCard(
		Root.Get(), TEXT("Upgrade.Merge.WhiteA"), TEXT("Upgrade.Merge"), WacomTags::Card_Rarity_White, 2);
	UCardDefinition* WhiteB = MakeCard(
		Root.Get(), TEXT("Upgrade.Merge.WhiteB"), TEXT("Upgrade.Merge"), WacomTags::Card_Rarity_White, 3);
	UCardDefinition* Blue = MakeCard(
		Root.Get(), TEXT("Upgrade.Merge.Blue"), TEXT("Upgrade.Merge"), WacomTags::Card_Rarity_Blue, 1);
	WhiteA->NextUpgradeDefinition = Blue;
	WhiteB->NextUpgradeDefinition = Blue;

	TArray<FText> Errors;
	TestFalse(TEXT("Merged predecessors rejected"),
		FWacomCardUpgradeCatalogValidation::Validate({ WhiteA, WhiteB, Blue }, Errors));
	TestTrue(TEXT("Merge error is explicit"), ContainsError(Errors, TEXT("多个前驱")));

	WhiteB->NextUpgradeDefinition = nullptr;
	WhiteB->UpgradeFamilyId = TEXT("Upgrade.Detached");
	WhiteB->CardId = WhiteA->CardId;
	TestFalse(TEXT("Duplicate CardId rejected"),
		FWacomCardUpgradeCatalogValidation::Validate({ WhiteA, WhiteB, Blue }, Errors));
	TestTrue(TEXT("Duplicate error is explicit"), ContainsError(Errors, TEXT("CardId 重复")));
	return true;
}
