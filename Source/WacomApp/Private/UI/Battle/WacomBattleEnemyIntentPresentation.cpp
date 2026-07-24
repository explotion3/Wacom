// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleEnemyIntentPresentation.h"

#include "Snapshots/EnemySnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Battle/WacomBattleEnemyIntentPresentationStyle.h"
#include "UI/Battle/WacomBattleEnemyPanelViewData.h"
#include "UI/Battle/WacomBattleStatusIconWidget.h"
#include "UI/Battle/WacomBattleStatusPresentationCatalog.h"
#include "UI/Battle/WacomBattleStatusPresentationCatalogProvider.h"
#include "UI/Battle/WacomBattleStatusTooltipPresentation.h"

#define LOCTEXT_NAMESPACE "WacomBattleEnemyIntentPresentation"

namespace
{
	bool AreAdjacentEffectsEquivalent(
		const FBattleIntentEffectSnapshot& Left,
		const FBattleIntentEffectSnapshot& Right)
	{
		return Left.EffectType == Right.EffectType
			&& Left.TargetKind == Right.TargetKind
			&& Left.Magnitude == Right.Magnitude
			&& Left.Duration == Right.Duration
			&& Left.TargetCount == Right.TargetCount;
	}

	FText BuildTargetText(const FBattleIntentEffectSnapshot& Effect)
	{
		switch (Effect.TargetKind)
		{
		case EBattleIntentEffectTargetKind::Player:
			return LOCTEXT("TargetPlayer", "[玩家]");
		case EBattleIntentEffectTargetKind::SelfEnemyPart:
			return LOCTEXT("TargetSelf", "[自身]");
		case EBattleIntentEffectTargetKind::RandomPlayerHandCards:
			return FText::Format(
				LOCTEXT("TargetRandomCards", "[随机 {0} 张手牌]"),
				FText::AsNumber(FMath::Max(1, Effect.TargetCount)));
		case EBattleIntentEffectTargetKind::AllPlayerHandCards:
			return LOCTEXT("TargetAllCards", "[全部手牌]");
		default:
			return LOCTEXT("TargetUnknown", "[未知目标]");
		}
	}

	EWacomBattleStatusInspectionHost ResolveStatusHost(
		const EBattleIntentEffectTargetKind TargetKind)
	{
		return TargetKind == EBattleIntentEffectTargetKind::SelfEnemyPart
			? EWacomBattleStatusInspectionHost::EnemyPart
			: EWacomBattleStatusInspectionHost::Player;
	}

	FText BuildSignedMagnitude(const int32 Magnitude)
	{
		return FText::FromString(Magnitude >= 0
			? FString::Printf(TEXT("+%d"), Magnitude)
			: FString::Printf(TEXT("%d"), Magnitude));
	}

	FText AppendDuration(const FText& Base, const int32 Duration)
	{
		return Duration > 0
			? FText::Format(
				LOCTEXT("WithDuration", "{0}（持续 {1}）"),
				Base,
				FText::AsNumber(Duration))
			: Base;
	}

	FWacomBattleIntentEffectRowViewData BuildRow(
		const FBattleIntentEffectSnapshot& Effect,
		const int32 RepeatCount,
		const UWacomBattleEnemyIntentPresentationStyle* Style)
	{
		FWacomBattleIntentEffectRowViewData Row;
		Row.EffectType = Effect.EffectType;
		Row.TargetText = BuildTargetText(Effect);
		Row.RepeatCount = FMath::Max(1, RepeatCount);

		const UWacomBattleStatusPresentationCatalog& Catalog =
			WacomBattleStatusPresentationCatalogProvider::GetCatalog();
		if (Effect.EffectType == WacomTags::Effect_Damage)
		{
			Row.EffectText = Row.RepeatCount > 1
				? FText::Format(
					LOCTEXT("DamageRepeated", "伤害 {0} × {1}"),
					FText::AsNumber(Effect.Magnitude),
					FText::AsNumber(Row.RepeatCount))
				: FText::Format(
					LOCTEXT("Damage", "伤害 {0}"),
					FText::AsNumber(Effect.Magnitude));
			if (Style)
			{
				Row.IconBrush = Style->DamageEffectIconBrush;
				Row.Tint = Style->DamageEffectTint;
			}
			return Row;
		}

		if (Effect.EffectType == WacomTags::Status_Shield)
		{
			Row.EffectText = AppendDuration(
				FText::Format(
					LOCTEXT("Shield", "护盾 {0}"),
					BuildSignedMagnitude(Effect.Magnitude)),
				Effect.Duration);
			if (Style)
			{
				Row.IconBrush = Style->ShieldEffectIconBrush;
				Row.Tint = Style->ShieldEffectTint;
			}
			return Row;
		}

		if (const FWacomBattleStatusPresentationEntry* StatusEntry =
			Catalog.FindEntry(Effect.EffectType))
		{
			Row.EffectText = AppendDuration(
				FText::Format(
					LOCTEXT("StatusMagnitude", "{0} {1}"),
					StatusEntry->DisplayName,
					BuildSignedMagnitude(Effect.Magnitude)),
				Effect.Duration);
			Row.IconBrush = StatusEntry->IconBrush;
			Row.Tint = Style
				? Style->StatusEffectTint
				: FLinearColor::White;

			FWacomBattleStatusIconView StatusView;
			StatusView.StatusTag = StatusEntry->StatusTag;
			StatusView.InspectionHost = ResolveStatusHost(Effect.TargetKind);
			FWacomBattleStatusTooltipPresentationBuilder::PopulateRuleText(StatusView);
			Row.CoreRuleText = StatusView.CoreEffectText;
			return Row;
		}

		const FString TypeText = Effect.EffectType.IsValid()
			? Effect.EffectType.GetTagName().ToString()
			: FString(TEXT("InvalidEffect"));
		Row.EffectText = FText::FromString(FString::Printf(
			TEXT("%s %s"),
			*TypeText,
			*BuildSignedMagnitude(Effect.Magnitude).ToString()));
		Row.EffectText = AppendDuration(Row.EffectText, Effect.Duration);
		if (Style)
		{
			Row.IconBrush = Style->FallbackEffectIconBrush;
			Row.Tint = Style->UnknownEffectTint;
		}
		return Row;
	}
}

FWacomBattleIntentPresentationViewData FWacomBattleIntentPresentationBuilder::Build(
	const FWacomBattleEnemyPartEntryViewData& PartView,
	const UWacomBattleEnemyIntentPresentationStyle* Style,
	const int32 MaximumVisibleRows)
{
	FWacomBattleIntentPresentationViewData View;
	if (PartView.bDestroyed || PartView.CurrentIntentId.IsNone())
	{
		return View;
	}

	View.IntentId = PartView.CurrentIntentId;
	View.IntentDisplayName = PartView.CurrentIntentDisplayName.IsEmpty()
		? FText::FromName(PartView.CurrentIntentId)
		: PartView.CurrentIntentDisplayName;
	View.HeaderMetaText = PartView.bCurrentIntentIsAttack
		? FText::Format(
			LOCTEXT("AttackHeaderMeta", "INIT {0} · 最高单段 {1}"),
			FText::AsNumber(PartView.CurrentIntentInitiative),
			FText::AsNumber(PartView.CurrentIntentPeakAttackDamage))
		: FText::Format(
			LOCTEXT("HeaderMeta", "INIT {0}"),
			FText::AsNumber(PartView.CurrentIntentInitiative));
	if (Style)
	{
		if (const FSlateBrush* Brush =
			Style->ResolveIntentIcon(PartView.CurrentIntentId))
		{
			View.IntentIconBrush = *Brush;
		}
	}

	for (int32 Index = 0; Index < PartView.CurrentIntentEffects.Num();)
	{
		const FBattleIntentEffectSnapshot& Effect =
			PartView.CurrentIntentEffects[Index];
		int32 RepeatCount = 1;
		while (Index + RepeatCount < PartView.CurrentIntentEffects.Num()
			&& AreAdjacentEffectsEquivalent(
				Effect,
				PartView.CurrentIntentEffects[Index + RepeatCount]))
		{
			++RepeatCount;
		}
		View.EffectRows.Add(BuildRow(Effect, RepeatCount, Style));
		Index += RepeatCount;
	}

	if (MaximumVisibleRows > 0 && View.EffectRows.Num() > MaximumVisibleRows)
	{
		View.HiddenEffectRowCount = View.EffectRows.Num() - MaximumVisibleRows;
		View.EffectRows.SetNum(MaximumVisibleRows);
	}
	return View;
}

#undef LOCTEXT_NAMESPACE
