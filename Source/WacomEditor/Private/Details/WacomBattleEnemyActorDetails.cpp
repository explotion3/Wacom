// Copyright Wacom. All Rights Reserved.

#include "Details/WacomBattleEnemyActorDetails.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleSceneEnemyAuthoringReport.h"
#include "Authoring/WacomBattleSceneEnemyHostAuthoring.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Engine/World.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "WacomBattleEnemyActorDetails"

namespace
{
	FString JoinNames(const TArray<FName>& Names)
	{
		TArray<FString> Strings;
		Strings.Reserve(Names.Num());
		for (const FName& Name : Names)
		{
			Strings.Add(Name.IsNone() ? TEXT("None") : Name.ToString());
		}
		return FString::Join(Strings, TEXT(", "));
	}

	FString JoinStrings(const TArray<FString>& Strings)
	{
		return FString::Join(Strings, TEXT(", "));
	}

	void ShowNotification(
		const FText& Message,
		SNotificationItem::ECompletionState State)
	{
		FNotificationInfo Info(Message);
		Info.ExpireDuration = 5.0f;
		Info.bUseLargeFont = false;
		if (const TSharedPtr<SNotificationItem> Notification =
			FSlateNotificationManager::Get().AddNotification(Info))
		{
			Notification->SetCompletionState(State);
		}
	}
}

TSharedRef<IDetailCustomization> FWacomBattleEnemyActorDetails::MakeInstance()
{
	return MakeShared<FWacomBattleEnemyActorDetails>();
}

void FWacomBattleEnemyActorDetails::CustomizeDetails(
	IDetailLayoutBuilder& DetailBuilder)
{
	ActiveDetailBuilder = &DetailBuilder;
	Hosts.Reset();
	TArray<TWeakObjectPtr<UObject>> CustomizedObjects;
	DetailBuilder.GetObjectsBeingCustomized(CustomizedObjects);
	for (const TWeakObjectPtr<UObject>& Object : CustomizedObjects)
	{
		if (AWacomBattleEnemyActor* Host = Cast<AWacomBattleEnemyActor>(Object.Get()))
		{
			Hosts.AddUnique(Host);
		}
	}

	IDetailCategoryBuilder& ReportCategory = DetailBuilder.EditCategory(
		TEXT("Wacom|Battle|Scene Enemy|Authoring Report"),
		LOCTEXT("AuthoringReportCategory", "Wacom | Battle | Scene Enemy | Authoring Report"),
		ECategoryPriority::Important);
	ReportCategory.AddCustomRow(LOCTEXT("AuthoringReportFilter", "Enemy Host Authoring Report"))
	.WholeRowContent()
	[
		SNew(STextBlock)
		.Text(this, &FWacomBattleEnemyActorDetails::BuildAuthoringReportText)
		.AutoWrapText(true)
	];
	ReportCategory.AddCustomRow(LOCTEXT("SyncPartsFilter", "从 EnemyDefinition 同步部位"))
	.WholeRowContent()
	[
		SNew(SButton)
		.Text(LOCTEXT("SyncPartsButton", "从 EnemyDefinition 同步部位"))
		.ToolTipText(LOCTEXT(
			"SyncPartsTooltip",
			"显式应用上方只读报告中的同步计划。Blueprint 模板写入 SCS；关卡实例只修改当前实例；保留已有位置、命中体、ImpactAnchor 和 VisualLayers；不删除 surplus 部位。"))
		.IsEnabled(this, &FWacomBattleEnemyActorDetails::CanSyncParts)
		.OnClicked(this, &FWacomBattleEnemyActorDetails::HandleSyncParts)
	];

	IDetailCategoryBuilder& DebugCategory = DetailBuilder.EditCategory(
		TEXT("Wacom|Battle|Scene Enemy|Advanced Debug"),
		LOCTEXT("AdvancedDebugCategory", "Wacom | Battle | Scene Enemy | Advanced Debug"));
	DebugCategory.InitiallyCollapsed(true);
	DebugCategory.AddCustomRow(LOCTEXT("SnakeSampleFilter", "配置蛇样例"))
	.WholeRowContent()
	[
		SNew(SButton)
		.Text(LOCTEXT("SnakeSampleButton", "配置 Debug 蛇样例"))
		.ToolTipText(LOCTEXT(
			"SnakeSampleTooltip",
			"只用于开发样例：给当前单选 Host 及其已有三个部位写入蛇身份、示例位置和 Badge stagger；不会生成缺失部位。"))
		.IsEnabled(this, &FWacomBattleEnemyActorDetails::CanConfigureDebugSnake)
		.OnClicked(this, &FWacomBattleEnemyActorDetails::HandleConfigureDebugSnake)
	];
}

TArray<AWacomBattleEnemyActor*> FWacomBattleEnemyActorDetails::GetLiveHosts() const
{
	TArray<AWacomBattleEnemyActor*> LiveHosts;
	for (const TWeakObjectPtr<AWacomBattleEnemyActor>& Host : Hosts)
	{
		if (AWacomBattleEnemyActor* LiveHost = Host.Get())
		{
			LiveHosts.Add(LiveHost);
		}
	}
	return LiveHosts;
}

FText FWacomBattleEnemyActorDetails::BuildAuthoringReportText() const
{
	const TArray<AWacomBattleEnemyActor*> LiveHosts = GetLiveHosts();
	if (LiveHosts.IsEmpty())
	{
		return LOCTEXT("NoHostReport", "没有可读取的 Enemy Host。");
	}

	if (LiveHosts.Num() == 1)
	{
		const AWacomBattleEnemyActor& Host = *LiveHosts[0];
		const FWacomBattleSceneEnemyHostAuthoringReport Report =
			FWacomBattleSceneEnemyHostAuthoringEvaluator::Build(Host);
		return FText::FromString(FString::Printf(
			TEXT("Host: %s\n")
			TEXT("状态: %s  Ready: %s\n")
			TEXT("Host Visual 有效: %s  Animation Style 适用: %s  缺视觉层槽位: [%s]\n")
			TEXT("部位: %d  待新增: %d  待修正: %d\n")
			TEXT("PartSlotIds: [%s]\nPartIds: [%s]\n")
			TEXT("缺失 Definition 槽位: [%s]  缺失 Definition PartId: [%s]\n")
			TEXT("未知槽位: [%s]  未知 PartId: [%s]\n")
			TEXT("重复槽位: [%s]  Identity mismatch: [%s]\n")
			TEXT("无效 Definition 槽位: [%s]\nSurplus: [%s]\n")
			TEXT("最近同步: %s  Added: [%s]  Updated: [%s]  Invalid: [%s]"),
			*Host.GetName(),
			*Report.AuthoringState.ToString(),
			Report.bAuthoringReady ? TEXT("是") : TEXT("否"),
			Report.bUsingHostVisual ? TEXT("是") : TEXT("否"),
			Report.bHostAnimationStyleApplicable ? TEXT("是") : TEXT("否"),
			*JoinNames(Report.MissingVisualLayerPartSlotIds),
			Report.PartActorCount,
			Report.GetAddMissingPartCount(),
			Report.GetUpdateDerivedPartIdCount(),
			*JoinNames(Report.AttachedPartSlotIds),
			*JoinNames(Report.AttachedPartIds),
			*JoinNames(Report.IdentityAudit.MissingDefinitionPartSlotIds),
			*JoinNames(Report.IdentityAudit.MissingDefinitionPartIds),
			*JoinNames(Report.IdentityAudit.UnknownPartSlotIds),
			*JoinNames(Report.IdentityAudit.UnknownPartIds),
			*JoinNames(Report.IdentityAudit.DuplicatePartSlotIds),
			*JoinNames(Report.IdentityAudit.PartDefinitionMismatchSlotIds),
			*JoinNames(Report.InvalidDefinitionPartSlotIds),
			*JoinStrings(Report.IdentityAudit.SurplusPartActorNames),
			*Host.AuthoringLastPartSyncResult.ToString(),
			*JoinNames(Host.AuthoringLastAddedPartSlotIds),
			*JoinNames(Host.AuthoringLastUpdatedPartSlotIds),
			*JoinNames(Host.AuthoringLastInvalidDefinitionPartSlotIds)));
	}

	int32 ReadyCount = 0;
	int32 AddCount = 0;
	int32 UpdateCount = 0;
	int32 InvalidCount = 0;
	int32 SurplusCount = 0;
	for (const AWacomBattleEnemyActor* Host : LiveHosts)
	{
		const FWacomBattleSceneEnemyHostAuthoringReport Report =
			FWacomBattleSceneEnemyHostAuthoringEvaluator::Build(*Host);
		ReadyCount += Report.bAuthoringReady ? 1 : 0;
		AddCount += Report.GetAddMissingPartCount();
		UpdateCount += Report.GetUpdateDerivedPartIdCount();
		InvalidCount += Report.InvalidDefinitionPartSlotIds.Num();
		SurplusCount += Report.IdentityAudit.SurplusPartActorNames.Num();
	}
	return FText::Format(
		LOCTEXT(
			"MultipleHostReport",
			"已选择 Host: {0}  Ready: {1}\n待新增: {2}  待修正: {3}  无效定义槽位: {4}  Surplus: {5}"),
		FText::AsNumber(LiveHosts.Num()),
		FText::AsNumber(ReadyCount),
		FText::AsNumber(AddCount),
		FText::AsNumber(UpdateCount),
		FText::AsNumber(InvalidCount),
		FText::AsNumber(SurplusCount));
}

bool FWacomBattleEnemyActorDetails::CanSyncParts() const
{
	for (const AWacomBattleEnemyActor* Host : GetLiveHosts())
	{
		const FWacomBattleSceneEnemyHostAuthoringReport Report =
			FWacomBattleSceneEnemyHostAuthoringEvaluator::Build(*Host);
		if (Report.bHasValidDefinitionParts
			&& (!Host->GetWorld() || !Host->GetWorld()->IsGameWorld()))
		{
			return true;
		}
	}
	return false;
}

bool FWacomBattleEnemyActorDetails::CanConfigureDebugSnake() const
{
	const TArray<AWacomBattleEnemyActor*> LiveHosts = GetLiveHosts();
	return LiveHosts.Num() == 1
		&& (!LiveHosts[0]->GetWorld() || !LiveHosts[0]->GetWorld()->IsGameWorld());
}

FReply FWacomBattleEnemyActorDetails::HandleSyncParts()
{
	const TArray<AWacomBattleEnemyActor*> LiveHosts = GetLiveHosts();
	const TArray<FWacomBattleSceneEnemyHostSyncResult> Results =
		FWacomBattleSceneEnemyHostAuthoring::SyncPartsFromDefinition(LiveHosts);
	int32 ChangedCount = 0;
	int32 WarningCount = 0;
	int32 FailureCount = 0;
	int32 FailedCount = 0;
	for (const FWacomBattleSceneEnemyHostSyncResult& Result : Results)
	{
		ChangedCount += Result.bChanged ? 1 : 0;
		FailedCount += Result.FailedPartSlotIds.Num();
		const bool bFailure = Result.ResultCode == FName(TEXT("ApplyFailed"))
			|| Result.ResultCode == FName(TEXT("PartiallyApplied"));
		const bool bSuccess = Result.ResultCode == FName(TEXT("Applied"))
			|| Result.ResultCode == FName(TEXT("NoChanges"));
		FailureCount += bFailure ? 1 : 0;
		WarningCount += !bFailure && !bSuccess ? 1 : 0;
	}
	ShowNotification(
		FText::Format(
			LOCTEXT(
				"SyncResultNotification",
				"Enemy Host 同步完成：处理 {0}，发生变化 {1}，警告 {2}，失败槽位 {3}。"),
			FText::AsNumber(Results.Num()),
			FText::AsNumber(ChangedCount),
			FText::AsNumber(WarningCount),
			FText::AsNumber(FailedCount)),
		FailureCount > 0
			? SNotificationItem::CS_Fail
			: (WarningCount > 0
				? SNotificationItem::CS_Pending
				: SNotificationItem::CS_Success));
	if (ActiveDetailBuilder)
	{
		ActiveDetailBuilder->ForceRefreshDetails();
	}
	return FReply::Handled();
}

FReply FWacomBattleEnemyActorDetails::HandleConfigureDebugSnake()
{
	const TArray<AWacomBattleEnemyActor*> LiveHosts = GetLiveHosts();
	if (LiveHosts.Num() == 1)
	{
		const FName Result =
			FWacomBattleSceneEnemyHostAuthoring::ConfigureDebugSnakeSample(*LiveHosts[0]);
		ShowNotification(
			FText::Format(
				LOCTEXT("SnakeResultNotification", "Debug 蛇样例结果：{0}"),
				FText::FromName(Result)),
			Result == FName(TEXT("Applied"))
				? SNotificationItem::CS_Success
				: SNotificationItem::CS_Pending);
	}
	if (ActiveDetailBuilder)
	{
		ActiveDetailBuilder->ForceRefreshDetails();
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
