// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/RunWorldCardInteractionDefinitionBuilder.h"
#include "ContentBuilders/ContentBuilderHelpers.h"

#include "Cards/CardDefinition.h"
#include "Interactions/RunWorldCardInteractionDefinition.h"

namespace
{
	using namespace Wacom::ContentBuilder;

	UCardDefinition* LoadGeneratedCard(const FString& ObjectPath)
	{
		UCardDefinition* Card = LoadObject<UCardDefinition>(nullptr, *ObjectPath);
		if (!Card)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[RunWorldCardInteractionDefinitionBuilder] Failed to load card asset: %s"),
				*ObjectPath);
		}
		return Card;
	}
}

namespace Wacom::ContentBuilder
{
	UWacomRunWorldCardInteractionDefinition* BuildRunWorldCardInteractionDefinitionContent()
	{
		UCardDefinition* DebugKey = LoadGeneratedCard(
			MakeObjectPath(MakePackagePath(BugGirlCardsRoot(), TEXT("DA_Card_DebugKey"))));
		if (!DebugKey)
		{
			return nullptr;
		}

		const FString PackagePath = MakePackagePath(
			InteractionsRoot(),
			TEXT("DA_RunWorldCardInteraction_DebugKeyGold3"));
		UPackage* Pkg = FindOrCreatePackage(PackagePath);
		if (!Pkg) { return nullptr; }

		UWacomRunWorldCardInteractionDefinition* Definition =
			CreateOrReplaceAsset<UWacomRunWorldCardInteractionDefinition>(
				Pkg,
				TEXT("DA_RunWorldCardInteraction_DebugKeyGold3"));
		if (!Definition) { return nullptr; }

		Definition->InteractionId = TEXT("WorldCardInteraction.DebugKeyGold3");
		Definition->AllowedCardDefinitions = { DebugKey };
		Definition->AllowedCardIds = { TEXT("DebugKey") };
		Definition->RequiredKeywords.Reset();
		Definition->BlockedKeywords.Reset();
		Definition->GoldReward = 3;
		Definition->bConsumeCardOnSuccess = true;
		Definition->PreviewPromptText = FText::FromString(TEXT("使用钥匙打开宝箱"));
		Definition->SuccessPromptText = FText::FromString(TEXT("宝箱已打开"));
		Definition->CompletedPromptText = FText::FromString(TEXT("宝箱已打开"));
		Definition->RejectedCardPromptText = FText::FromString(TEXT("需要钥匙"));
		Definition->ConfigWarningPromptText = FText::FromString(TEXT("场景交互配置异常"));
		Definition->SourceCardUnavailablePromptText =
			FText::FromString(TEXT("这张卡无法用于交互"));
		Definition->GenericFailurePromptText =
			FText::FromString(TEXT("无法完成场景交互"));

		SaveAssetPackage(Pkg, Definition, PackagePath);
		return Definition;
	}
}
