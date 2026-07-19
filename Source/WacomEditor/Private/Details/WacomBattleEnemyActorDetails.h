// Copyright Wacom. All Rights Reserved.

#pragma once

#include "IDetailCustomization.h"

class AWacomBattleEnemyActor;
class IDetailLayoutBuilder;

class FWacomBattleEnemyActorDetails final : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	TArray<AWacomBattleEnemyActor*> GetLiveHosts() const;
	FText BuildAuthoringReportText() const;
	bool CanSyncParts() const;
	FReply HandleSyncParts();

	TArray<TWeakObjectPtr<AWacomBattleEnemyActor>> Hosts;
	IDetailLayoutBuilder* ActiveDetailBuilder = nullptr;
};
