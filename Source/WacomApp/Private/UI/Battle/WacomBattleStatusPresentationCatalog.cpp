// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleStatusPresentationCatalog.h"

#include "Internationalization/Text.h"
#include "Tags/WacomGameplayTags.h"

#define LOCTEXT_NAMESPACE "WacomBattleStatusPresentationCatalog"

namespace
{
	constexpr float DefaultIconSize = 32.0f;
	constexpr int32 UnknownStatusSortPriority = 1000;

	FSlateBrush MakeFallbackBrush(const FLinearColor& Tint)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.TintColor = FSlateColor(Tint);
		Brush.SetImageSize(FVector2f(DefaultIconSize, DefaultIconSize));
		return Brush;
	}

	FWacomBattleStatusRuleTextSet MakeRules(
		const FText& CoreEffect,
		const FText& TriggerTiming,
		const FText& StackPolicy)
	{
		FWacomBattleStatusRuleTextSet Rules;
		Rules.CoreEffectText = CoreEffect;
		Rules.TriggerTimingText = TriggerTiming;
		Rules.StackPolicyText = StackPolicy;
		return Rules;
	}

	FWacomBattleStatusPresentationEntry MakeEntry(
		const FGameplayTag StatusTag,
		const FText& DisplayName,
		const int32 SortPriority,
		const FLinearColor& FallbackTint,
		const FWacomBattleStatusRuleTextSet& PlayerRules,
		const FWacomBattleStatusRuleTextSet& EnemyPartRules,
		std::initializer_list<FGameplayTag> Aliases = {})
	{
		FWacomBattleStatusPresentationEntry Entry;
		Entry.StatusTag = StatusTag;
		Entry.DisplayName = DisplayName;
		Entry.SortPriority = SortPriority;
		Entry.IconBrush = MakeFallbackBrush(FallbackTint);
		Entry.PlayerRules = PlayerRules;
		Entry.EnemyPartRules = EnemyPartRules;
		for (const FGameplayTag Alias : Aliases)
		{
			Entry.LookupAliases.Add(Alias);
		}
		return Entry;
	}

#if WITH_EDITOR
	bool IsRequiredIconStatus(const FGameplayTag StatusTag)
	{
		return StatusTag == WacomTags::Status_Poison
			|| StatusTag == WacomTags::Status_Slow
			|| StatusTag == WacomTags::Status_Freeze
			|| StatusTag == WacomTags::Status_Twilight
			|| StatusTag == WacomTags::Status_Stunned;
	}

	void ValidateTemplate(
		const FText& Template,
		const FText& FieldLabel,
		FDataValidationContext& Context,
		EDataValidationResult& InOutResult)
	{
		static const TSet<FString> AllowedParameters = {
			TEXT("PoisonDamagePerStack"),
			TEXT("PlayerHealPoisonRemovalPercent"),
		};

		TArray<FString> ParameterNames;
		FText::GetFormatPatternParameters(FTextFormat(Template), ParameterNames);
		for (const FString& ParameterName : ParameterNames)
		{
			if (!AllowedParameters.Contains(ParameterName))
			{
				Context.AddError(FText::Format(
					LOCTEXT("UnsupportedTemplateParameter",
						"{0} 使用了未注册的命名占位符 {1}。"),
					FieldLabel,
					FText::FromString(ParameterName)));
				InOutResult = EDataValidationResult::Invalid;
			}
		}
	}

	void ValidateRules(
		const FWacomBattleStatusRuleTextSet& Rules,
		const FText& FieldLabel,
		FDataValidationContext& Context,
		EDataValidationResult& InOutResult)
	{
		if (!Rules.IsComplete())
		{
			Context.AddError(FText::Format(
				LOCTEXT("IncompleteRules", "{0} 必须完整填写核心效果、触发时机和叠层规则。"),
				FieldLabel));
			InOutResult = EDataValidationResult::Invalid;
			return;
		}

		ValidateTemplate(
			Rules.CoreEffectText,
			FText::Format(LOCTEXT("CoreEffectField", "{0}.CoreEffectText"), FieldLabel),
			Context,
			InOutResult);
		ValidateTemplate(
			Rules.TriggerTimingText,
			FText::Format(LOCTEXT("TriggerTimingField", "{0}.TriggerTimingText"), FieldLabel),
			Context,
			InOutResult);
		ValidateTemplate(
			Rules.StackPolicyText,
			FText::Format(LOCTEXT("StackPolicyField", "{0}.StackPolicyText"), FieldLabel),
			Context,
			InOutResult);
	}
#endif
}

bool FWacomBattleStatusRuleTextSet::IsComplete() const
{
	return !CoreEffectText.IsEmpty()
		&& !TriggerTimingText.IsEmpty()
		&& !StackPolicyText.IsEmpty();
}

UWacomBattleStatusPresentationCatalog::UWacomBattleStatusPresentationCatalog()
{
	Entries = {
		MakeEntry(
			WacomTags::Status_Poison,
			LOCTEXT("StatusPoison", "中毒"),
			0,
			FLinearColor(0.24f, 0.72f, 0.28f, 1.0f),
			MakeRules(
				LOCTEXT("PlayerPoisonCore", "每层在结算时造成 {PoisonDamagePerStack} 点生命伤害，并穿透护盾。"),
				LOCTEXT("PlayerPoisonTiming", "玩家每打出一张牌或任一敌方部位行动后结算。"),
				LOCTEXT("PlayerPoisonPolicy", "结算不减层；玩家治疗时移除治疗量的 {PlayerHealPoisonRemovalPercent}% 层数，向下取整。")),
			MakeRules(
				LOCTEXT("EnemyPoisonCore", "每层在结算时造成 {PoisonDamagePerStack} 点生命伤害，并穿透护盾。"),
				LOCTEXT("EnemyPoisonTiming", "玩家每打出一张牌或任一敌方部位行动后结算。"),
				LOCTEXT("EnemyPoisonPolicy", "结算不减层；可通过移除状态效果降低层数。")),
			{ WacomTags::Effect_ApplyStatus_Poison }),
		MakeEntry(
			WacomTags::Status_Slow,
			LOCTEXT("StatusSlow", "减速"),
			10,
			FLinearColor(0.22f, 0.58f, 0.86f, 1.0f),
			MakeRules(
				LOCTEXT("PlayerSlowCore", "下回合随机若干张手牌获得减速；每层使该卡费用 +1。"),
				LOCTEXT("PlayerSlowTiming", "下回合抽牌并重建手牌后物化到目标卡牌。"),
				LOCTEXT("PlayerSlowPolicy", "卡牌上的减速在该回合结束时清除。")),
			MakeRules(
				LOCTEXT("EnemySlowCore", "施加时按层数延后该部位当前意图的先机。"),
				LOCTEXT("EnemySlowTiming", "效果成功施加时立即结算。"),
				LOCTEXT("EnemySlowPolicy", "不保留为敌方部位的持久状态。")),
			{ WacomTags::Effect_ApplyStatus_Slow }),
		MakeEntry(
			WacomTags::Status_Freeze,
			LOCTEXT("StatusFreeze", "冻结"),
			20,
			FLinearColor(0.55f, 0.86f, 1.0f, 1.0f),
			MakeRules(
				LOCTEXT("PlayerFreezeCore", "下回合随机若干张手牌被冻结；被冻结卡无法打出。"),
				LOCTEXT("PlayerFreezeTiming", "下回合抽牌并重建手牌后物化到目标卡牌。"),
				LOCTEXT("PlayerFreezePolicy", "打出其相邻卡可全部解除；回合结束仍未解除的冻结会清除。")),
			MakeRules(
				LOCTEXT("EnemyFreezeCore", "每层拦截下一张会真实推进先机的非迅捷卡。"),
				LOCTEXT("EnemyFreezeTiming", "该卡推进对应部位先机时触发。"),
				LOCTEXT("EnemyFreezePolicy", "每次触发消耗 1 层；不会使敌人跳过行动。")),
			{ WacomTags::Effect_ApplyStatus_Freeze }),
		MakeEntry(
			WacomTags::Status_Twilight,
			LOCTEXT("StatusTwilight", "暮气"),
			30,
			FLinearColor(0.58f, 0.36f, 0.86f, 1.0f),
			MakeRules(
				LOCTEXT("PlayerTwilightCore", "下回合整手牌获得暮气；每层使卡牌费用 +1。"),
				LOCTEXT("PlayerTwilightTiming", "下回合抽牌并重建手牌后物化到当前手牌。"),
				LOCTEXT("PlayerTwilightPolicy", "卡牌成功打出后层数向下减半，并随卡牌跨区域保留。")),
			MakeRules(
				LOCTEXT("EnemyTwilightCore", "下一意图的先机增加当前暮气层数。"),
				LOCTEXT("EnemyTwilightTiming", "安装下一意图的基础先机后触发。"),
				LOCTEXT("EnemyTwilightPolicy", "触发后层数向下减半；剩余层数继续保留。")),
			{ WacomTags::Effect_ApplyStatus_Twilight }),
		MakeEntry(
			WacomTags::Status_Stunned,
			LOCTEXT("StatusStunned", "眩晕"),
			40,
			FLinearColor(1.0f, 0.72f, 0.22f, 1.0f),
			MakeRules(
				LOCTEXT("PlayerStunnedCore", "当前没有玩家宿主的行动跳过规则。"),
				LOCTEXT("PlayerStunnedTiming", "仅作为玩家状态层数记录，不会拦截出牌。"),
				LOCTEXT("PlayerStunnedPolicy", "可通过移除状态效果降低层数。")),
			MakeRules(
				LOCTEXT("EnemyStunnedCore", "使该部位跳过下一次敌方行动。"),
				LOCTEXT("EnemyStunnedTiming", "该部位到达行动边界、准备执行意图时触发。"),
				LOCTEXT("EnemyStunnedPolicy", "每次触发消耗 1 层；叠层可连续跳过多次行动。"))),
		MakeEntry(
			WacomTags::Status_Shield,
			LOCTEXT("StatusShield", "护盾"),
			50,
			FLinearColor(0.38f, 0.72f, 1.0f, 1.0f),
			FWacomBattleStatusRuleTextSet(),
			FWacomBattleStatusRuleTextSet()),
	};

	FallbackIconBrush = MakeFallbackBrush(
		FLinearColor(0.70f, 0.72f, 0.76f, 1.0f));
	UnknownRules = MakeRules(
		LOCTEXT("UnknownCore", "当前状态暂无详细规则说明。"),
		LOCTEXT("UnknownTiming", "具体触发时机由战斗规则决定。"),
		LOCTEXT("UnknownPolicy", "显示层数来自当前战斗快照。"));
}

const FWacomBattleStatusPresentationEntry*
UWacomBattleStatusPresentationCatalog::FindEntry(const FGameplayTag QueryTag) const
{
	if (!QueryTag.IsValid())
	{
		return nullptr;
	}

	for (const FWacomBattleStatusPresentationEntry& Entry : Entries)
	{
		if (Entry.StatusTag == QueryTag || Entry.LookupAliases.Contains(QueryTag))
		{
			return &Entry;
		}
	}
	return nullptr;
}

FText UWacomBattleStatusPresentationCatalog::ResolveDisplayName(
	const FGameplayTag QueryTag) const
{
	if (const FWacomBattleStatusPresentationEntry* Entry = FindEntry(QueryTag))
	{
		if (!Entry->DisplayName.IsEmpty())
		{
			return Entry->DisplayName;
		}
	}
	return QueryTag.IsValid()
		? FText::FromString(QueryTag.ToString())
		: LOCTEXT("InvalidStatusName", "状态");
}

const FSlateBrush* UWacomBattleStatusPresentationCatalog::ResolveIconBrush(
	const FGameplayTag QueryTag) const
{
	if (const FWacomBattleStatusPresentationEntry* Entry = FindEntry(QueryTag))
	{
		if (IsIconBrushRenderable(Entry->IconBrush))
		{
			return &Entry->IconBrush;
		}
	}
	return IsIconBrushRenderable(FallbackIconBrush) ? &FallbackIconBrush : nullptr;
}

int32 UWacomBattleStatusPresentationCatalog::ResolveSortPriority(
	const FGameplayTag QueryTag) const
{
	if (const FWacomBattleStatusPresentationEntry* Entry = FindEntry(QueryTag))
	{
		return Entry->SortPriority;
	}
	return UnknownStatusSortPriority;
}

bool UWacomBattleStatusPresentationCatalog::IsIconBrushRenderable(
	const FSlateBrush& Brush)
{
	const FVector2f ImageSize = Brush.GetImageSize();
	return (Brush.GetResourceObject() != nullptr
			|| Brush.GetDrawType() != ESlateBrushDrawType::NoDrawType)
		&& ImageSize.X > 0.0f
		&& ImageSize.Y > 0.0f;
}

bool UWacomBattleStatusPresentationCatalog::IsIconBrushAssetConfigured(
	const FSlateBrush& Brush)
{
	const FVector2f ImageSize = Brush.GetImageSize();
	return Brush.GetResourceObject() != nullptr
		&& ImageSize.X > 0.0f
		&& ImageSize.Y > 0.0f;
}

#if WITH_EDITOR
EDataValidationResult UWacomBattleStatusPresentationCatalog::IsDataValid(
	FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	TSet<FGameplayTag> SeenLookupTags;

	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		const FWacomBattleStatusPresentationEntry& Entry = Entries[Index];
		const FText EntryLabel = FText::Format(
			LOCTEXT("EntryLabel", "Entries[{0}]"),
			FText::AsNumber(Index));

		if (!Entry.StatusTag.IsValid())
		{
			Context.AddError(FText::Format(
				LOCTEXT("InvalidStatusTag", "{0}.StatusTag 无效。"),
				EntryLabel));
			Result = EDataValidationResult::Invalid;
		}
		else if (SeenLookupTags.Contains(Entry.StatusTag))
		{
			Context.AddError(FText::Format(
				LOCTEXT("DuplicateStatusTag", "{0}.StatusTag {1} 重复或与其他 Alias 冲突。"),
				EntryLabel,
				FText::FromString(Entry.StatusTag.ToString())));
			Result = EDataValidationResult::Invalid;
		}
		else
		{
			SeenLookupTags.Add(Entry.StatusTag);
		}

		for (int32 AliasIndex = 0; AliasIndex < Entry.LookupAliases.Num(); ++AliasIndex)
		{
			const FGameplayTag Alias = Entry.LookupAliases[AliasIndex];
			if (!Alias.IsValid() || SeenLookupTags.Contains(Alias))
			{
				Context.AddError(FText::Format(
					LOCTEXT("InvalidAlias",
						"{0}.LookupAliases[{1}] 无效、重复或与其他条目冲突：{2}。"),
					EntryLabel,
					FText::AsNumber(AliasIndex),
					FText::FromString(Alias.ToString())));
				Result = EDataValidationResult::Invalid;
			}
			else
			{
				SeenLookupTags.Add(Alias);
			}
		}

		if (Entry.DisplayName.IsEmpty())
		{
			Context.AddError(FText::Format(
				LOCTEXT("EmptyDisplayName", "{0}.DisplayName 不能为空。"),
				EntryLabel));
			Result = EDataValidationResult::Invalid;
		}

		if (IsRequiredIconStatus(Entry.StatusTag))
		{
			if (!IsIconBrushAssetConfigured(Entry.IconBrush))
			{
				Context.AddError(FText::Format(
					LOCTEXT("MissingStatusIcon",
						"{0}.IconBrush 必须引用有效资源并具有正数 ImageSize。"),
					EntryLabel));
				Result = EDataValidationResult::Invalid;
			}
			ValidateRules(
				Entry.PlayerRules,
				FText::Format(LOCTEXT("PlayerRulesField", "{0}.PlayerRules"), EntryLabel),
				Context,
				Result);
			ValidateRules(
				Entry.EnemyPartRules,
				FText::Format(LOCTEXT("EnemyRulesField", "{0}.EnemyPartRules"), EntryLabel),
				Context,
				Result);
		}
	}

	const FGameplayTag RequiredStatuses[] = {
		WacomTags::Status_Poison,
		WacomTags::Status_Slow,
		WacomTags::Status_Freeze,
		WacomTags::Status_Twilight,
		WacomTags::Status_Stunned,
		WacomTags::Status_Shield,
	};
	for (const FGameplayTag RequiredStatus : RequiredStatuses)
	{
		if (!FindEntry(RequiredStatus))
		{
			Context.AddError(FText::Format(
				LOCTEXT("MissingRequiredStatus", "缺少必需状态条目 {0}。"),
				FText::FromString(RequiredStatus.ToString())));
			Result = EDataValidationResult::Invalid;
		}
	}

	if (!IsIconBrushAssetConfigured(FallbackIconBrush))
	{
		Context.AddError(LOCTEXT(
			"MissingFallbackIcon",
			"FallbackIconBrush 必须引用有效资源并具有正数 ImageSize。"));
		Result = EDataValidationResult::Invalid;
	}
	ValidateRules(
		UnknownRules,
		LOCTEXT("UnknownRulesField", "UnknownRules"),
		Context,
		Result);

	return Result == EDataValidationResult::Invalid
		? EDataValidationResult::Invalid
		: EDataValidationResult::Valid;
}
#endif

#undef LOCTEXT_NAMESPACE
