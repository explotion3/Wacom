// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackToastText.h"

#define LOCTEXT_NAMESPACE "WacomBackpack"

namespace
{
	namespace DeckReasons = WacomRunDeckOperationReasons;
}

FText FWacomBackpackToastText::FormatZoneNameForToast(EZoneKind Zone)
{
	switch (Zone)
	{
	case EZoneKind::Backpack:
		return LOCTEXT("ZoneBackpack", "通量区");
	case EZoneKind::BattleDeck:
		return LOCTEXT("ZoneBattleDeck", "备战区");
	case EZoneKind::SpecialZone:
		return LOCTEXT("ZoneSpecialZone", "特殊存放区");
	case EZoneKind::BurdenZone:
		return LOCTEXT("ZoneBurden", "负重区");
	default:
		return LOCTEXT("ZoneUnknown", "未知区域");
	}
}

FText FWacomBackpackToastText::FormatMoveFailureReasonForToast(FName DisabledReason)
{
	if (DisabledReason == DeckReasons::CardNotFound())
	{
		return LOCTEXT("MoveFailCardNotFound", "无法移动：找不到这张卡牌。");
	}
	if (DisabledReason == DeckReasons::FluxFull())
	{
		return LOCTEXT("MoveFailFluxFull", "无法移动：通量区已满。");
	}
	if (DisabledReason == DeckReasons::BattleDeckFull())
	{
		return LOCTEXT("MoveFailBattleDeckFull", "无法移动：备战区已满。");
	}
	if (DisabledReason == DeckReasons::SpecialZoneMissing())
	{
		return LOCTEXT("MoveFailSpecialZoneMissing", "无法移动：目标特殊存放区不存在。");
	}
	if (DisabledReason == DeckReasons::SelfSpecialZone())
	{
		return LOCTEXT("MoveFailSelfSpecialZone", "无法移动：主卡不能放进自己的特殊存放区。");
	}
	if (DisabledReason == DeckReasons::TypeBInSpecialZone())
	{
		return LOCTEXT("MoveFailTypeBInSpecialZone", "无法移动：特殊存放区不能收纳另一张主卡。");
	}
	if (DisabledReason == DeckReasons::SpecialZoneFull())
	{
		return LOCTEXT("MoveFailSpecialZoneFull", "无法移动：特殊存放区已满。");
	}
	if (DisabledReason == DeckReasons::TypeBInBurdenZone())
	{
		return LOCTEXT("MoveFailTypeBInBurden", "无法移动：主卡不能进入负重区。");
	}
	if (DisabledReason == DeckReasons::InvalidTargetZone())
	{
		return LOCTEXT("MoveFailInvalidTarget", "无法移动：目标区域无效。");
	}
	return LOCTEXT("MoveFailUnknown", "无法移动：当前规则不允许。");
}

FText FWacomBackpackToastText::FormatDeleteFailureReasonForToast(FName DisabledReason)
{
	if (DisabledReason == DeckReasons::MissingCard())
	{
		return LOCTEXT("DeleteFailMissingCard", "无法销毁：没有卡牌数据。");
	}
	if (DisabledReason == DeckReasons::CardNotOwned())
	{
		return LOCTEXT("DeleteFailCardNotOwned", "无法销毁：这张卡不在当前背包中。");
	}
	if (DisabledReason == DeckReasons::Intrinsic())
	{
		return LOCTEXT("DeleteFailIntrinsic", "无法销毁：固有卡不能被销毁。");
	}
	if (DeckReasons::IsLastCapacityProvider(DisabledReason))
	{
		return LOCTEXT("DeleteFailLastCapacityProvider", "无法销毁：这是最后一张背包容量卡。");
	}
	return LOCTEXT("DeleteFailUnknown", "无法销毁：当前规则不允许。");
}

FText FWacomBackpackToastText::FormatBattleEnabledFailureReasonForToast(FName DisabledReason)
{
	if (DisabledReason == DeckReasons::RunSessionMissing())
	{
		return LOCTEXT("BattleEnabledFailRunMissing", "无法切换入战：当前没有可用的 Run 数据。");
	}
	if (DisabledReason == DeckReasons::CardNotFound())
	{
		return LOCTEXT("BattleEnabledFailCardNotFound", "无法切换入战：找不到这张卡牌。");
	}
	if (DisabledReason == DeckReasons::NotInSpecialZone())
	{
		return LOCTEXT("BattleEnabledFailNotInSpecialZone", "无法切换入战：这张卡不在特殊存放区。");
	}
	return LOCTEXT("BattleEnabledFailUnknown", "无法切换入战：当前规则不允许。");
}

#undef LOCTEXT_NAMESPACE
