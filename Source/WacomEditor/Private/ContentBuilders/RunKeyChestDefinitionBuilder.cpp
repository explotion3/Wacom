// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/RunKeyChestDefinitionBuilder.h"
#include "ContentBuilders/ContentBuilderHelpers.h"

#include "Cards/CardDefinition.h"
#include "KeyChests/RunKeyChestDefinition.h"

namespace
{
	using namespace Wacom::ContentBuilder;

	UCardDefinition* LoadGeneratedCard(const FString& ObjectPath)
	{
		UCardDefinition* Card = LoadObject<UCardDefinition>(nullptr, *ObjectPath);
		if (!Card)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[RunKeyChestDefinitionBuilder] Failed to load card asset: %s"),
				*ObjectPath);
		}
		return Card;
	}
}

namespace Wacom::ContentBuilder
{
	UWacomRunKeyChestDefinition* BuildRunKeyChestDefinitionContent()
	{
		UCardDefinition* DebugKey = LoadGeneratedCard(
			MakeObjectPath(MakePackagePath(BugGirlCardsRoot(), TEXT("DA_Card_DebugKey"))));
		if (!DebugKey)
		{
			return nullptr;
		}

		const FString PackagePath =
			MakePackagePath(KeyChestsRoot(), TEXT("DA_KeyChest_DebugKeyGold3"));
		UPackage* Pkg = FindOrCreatePackage(PackagePath);
		if (!Pkg) { return nullptr; }

		UWacomRunKeyChestDefinition* Definition =
			CreateOrReplaceAsset<UWacomRunKeyChestDefinition>(
				Pkg,
				TEXT("DA_KeyChest_DebugKeyGold3"));
		if (!Definition) { return nullptr; }

		Definition->ChestId = TEXT("KeyChest.Debug.KeyGold3");
		Definition->AllowedCardDefinitions = { DebugKey };
		Definition->AllowedCardIds = { TEXT("DebugKey") };
		Definition->RequiredKeywords.Reset();
		Definition->BlockedKeywords.Reset();
		Definition->GoldReward = 3;
		Definition->bConsumeCardOnSuccess = true;
		Definition->InteractPromptText = FText::FromString(TEXT("需要钥匙"));
		Definition->HoverPromptText = FText::FromString(TEXT("拖入钥匙"));
		Definition->CompletedPromptText = FText::FromString(TEXT("宝箱已打开"));
		Definition->PreviewPromptText = FText::FromString(TEXT("使用钥匙打开宝箱"));
		Definition->SuccessPromptText = FText::FromString(TEXT("宝箱已打开"));
		Definition->ReceiverCompletedPromptText = FText::FromString(TEXT("宝箱已打开"));

		SaveAssetPackage(Pkg, Definition, PackagePath);
		return Definition;
	}
}
