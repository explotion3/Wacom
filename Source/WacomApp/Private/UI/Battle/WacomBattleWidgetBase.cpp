// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleWidgetBase.h"
#include "Session/BattleSession.h"

void UWacomBattleWidgetBase::SetSession(UBattleSession* InSession)
{
	UBattleSession* Old = Session;
	Session = InSession;

	// 递归下发给子 Widget
	for (const TObjectPtr<UWacomBattleWidgetBase>& Child : ChildBattleWidgets)
	{
		if (Child)
		{
			Child->SetSession(InSession);
		}
	}

	if (Old != InSession)
	{
		NativeOnSessionChanged(Old, InSession);
	}
}

void UWacomBattleWidgetBase::RefreshFromSnapshot(const FBattleSnapshot& Snap)
{
	NativeRefreshFromSnapshot(Snap);
	BP_OnRefreshedFromSnapshot(Snap);
}

void UWacomBattleWidgetBase::NativeRefreshFromSnapshot(const FBattleSnapshot& Snap)
{
	// 默认：递归刷新子 Widget。
	// 子类 override 时可以选择 Super::NativeRefreshFromSnapshot 递归，或自行处理。
	for (const TObjectPtr<UWacomBattleWidgetBase>& Child : ChildBattleWidgets)
	{
		if (Child)
		{
			Child->RefreshFromSnapshot(Snap);
		}
	}
}

void UWacomBattleWidgetBase::NativeOnSessionChanged(UBattleSession* /*OldSession*/, UBattleSession* /*NewSession*/)
{
	// 默认空实现。子类需要时 override。
}
