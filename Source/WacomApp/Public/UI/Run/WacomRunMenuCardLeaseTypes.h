// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "WacomRunMenuCardLeaseTypes.generated.h"

class UCardDefinition;

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomRunMenuCardLeaseRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards", meta = (ToolTip = "本次菜单卡牌租约 ID。同一个菜单重复调用应使用同一个 LeaseId；留空时菜单基类会自动生成。"))
	FName LeaseId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards", meta = (ToolTip = "写入第一人称卡牌层的 runtime source id。留空时菜单基类会自动生成。"))
	FName SourceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards", meta = (ToolTip = "允许显示的卡牌定义资产。与 AllowedCardIds 是 OR 关系；为空表示不按定义资产筛选。"))
	TArray<TObjectPtr<UCardDefinition>> AllowedCardDefinitions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards", meta = (ToolTip = "允许显示的 CardId 列表。与 AllowedCardDefinitions 是 OR 关系；为空表示不按 CardId 筛选。"))
	TArray<FName> AllowedCardIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards", meta = (ToolTip = "显式允许的卡牌实例 ID 白名单。非空时，候选卡必须命中这里的 InstanceId。"))
	TArray<FGuid> ExplicitCardInstanceIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards", meta = (ToolTip = "候选卡必须全部拥有的关键词。读取玩家持有卡实例对应定义上的 Card.Keyword。"))
	FGameplayTagContainer RequiredKeywords;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards", meta = (ToolTip = "候选卡不能拥有的关键词。命中任意一个即被排除。"))
	FGameplayTagContainer BlockedKeywords;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards", meta = (ToolTip = "是否从 Backpack 真实物理持有区收集候选卡。"))
	bool bIncludeBackpack = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards", meta = (ToolTip = "是否从 BattleDeck 真实物理持有区收集候选卡；不会包含投影入战卡。"))
	bool bIncludeBattleDeck = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards", meta = (ToolTip = "是否从 BurdenZone 真实物理持有区收集候选卡。"))
	bool bIncludeBurdenZone = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards", meta = (ToolTip = "是否从所有 SpecialZones.Cards 真实物理持有区收集候选卡；不会包含 BattleDeckProjectedCards。"))
	bool bIncludeSpecialZones = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Run|First Person Cards", meta = (ToolTip = "空筛选时是否允许显示玩家全部持有卡。默认关闭，避免菜单误把所有卡暴露到第一人称卡层。"))
	bool bAllowAllOwnedCardsWhenNoFilter = false;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomRunMenuCardLeaseResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	bool bLeaseSet = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	FName RejectReason = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	FName LeaseId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	FName SourceId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	int32 CandidateCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards")
	int32 ConsideredCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|First Person Cards|Debug", meta = (ToolTip = "菜单租约设置结果中的调试摘要；只用于排查候选筛选，不参与 Run 规则。"))
	FString DebugSummary;
};
