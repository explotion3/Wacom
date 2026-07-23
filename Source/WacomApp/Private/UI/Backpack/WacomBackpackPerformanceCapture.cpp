// Copyright Wacom. All Rights Reserved.

#if WITH_EDITOR

#include "UI/Backpack/WacomBackpackPerformanceCaptureTimeline.h"
#include "UI/Backpack/WacomBackpackSalePerformanceCaptureTimeline.h"

#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/Paths.h"
#include "ProfilingDebugging/MiscTrace.h"
#include "ProfilingDebugging/TraceAuxiliary.h"
#include "RunSession.h"
#include "UI/Backpack/WacomBackpackPIEValidationSupport.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "UI/Backpack/WacomBackpackWorkspaceSaleDepartureController.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UObject/UObjectIterator.h"

class FWacomBackpackPerformanceCaptureAccess
{
public:
	static UWacomBackpackScreen* FindActiveScreen(UWorld* World)
	{
		for (TObjectIterator<UWacomBackpackScreen> It; It; ++It)
		{
			UWacomBackpackScreen* Screen = *It;
			if (Screen
				&& !Screen->HasAnyFlags(RF_ClassDefaultObject)
				&& Screen->GetWorld() == World
				&& Screen->IsActivated())
			{
				return Screen;
			}
		}
		return nullptr;
	}

	static bool ConfigureMotion(UWorld* World, bool bSimplifiedMotion)
	{
		UWacomBackpackScreen* Screen = FindActiveScreen(World);
		if (!Screen || !Screen->WorkspaceWidget)
		{
			return false;
		}

		Screen->WorkspaceWidget->SetSimplifiedMotion(bSimplifiedMotion);
		return true;
	}

	static bool SubmitSale(UWorld* World, int32 CardCount)
	{
		UWacomBackpackScreen* Screen = FindActiveScreen(World);
		URunSession* Run = Screen ? Screen->GetRunSession() : nullptr;
		if (!Screen
			|| !Run
			|| !Screen->WorkspaceWidget
			|| !Screen->WorkspaceInteractionModel
			|| !Screen->bHasLastAppliedStorageSnapshot)
		{
			return false;
		}

		FWacomBackpackWorkspaceInteractionModel& Interaction =
			*Screen->WorkspaceInteractionModel;
		struct FCandidateZone
		{
			FWacomBackpackZoneKey Zone;
			TArray<FGuid> InstanceIds;
		};
		TArray<FCandidateZone> CandidateZones;
		auto AddCandidateZone =
			[&CandidateZones, &Interaction](
				const FWacomBackpackZoneKey& Zone,
				const TArray<FRunStorageCardView>& Cards)
			{
				FCandidateZone& Candidate = CandidateZones.AddDefaulted_GetRef();
				Candidate.Zone = Zone;
				Candidate.InstanceIds.Reserve(Cards.Num());
				for (int32 Index = Cards.Num() - 1; Index >= 0; --Index)
				{
					const FGuid InstanceId = Cards[Index].Instance.InstanceId;
					if (InstanceId.IsValid()
						&& Interaction.IsMovable(InstanceId))
					{
						Candidate.InstanceIds.Add(InstanceId);
					}
				}
			};

		const FRunBackpackStorageSnapshot& Snapshot =
			Screen->LastAppliedStorageSnapshot;
		AddCandidateZone(
			FWacomBackpackZoneKey::Make(EZoneKind::Backpack),
			Snapshot.Flux.ContentCards);
		AddCandidateZone(
			FWacomBackpackZoneKey::Make(EZoneKind::BattleDeck),
			Snapshot.BattleDeckPhysicalCards);
		for (const FRunSpecialStorageView& SpecialZone : Snapshot.SpecialZones)
		{
			AddCandidateZone(
				FWacomBackpackZoneKey::Make(
					EZoneKind::SpecialZone,
					SpecialZone.OwnerCard.Instance.InstanceId),
				SpecialZone.ContentCards);
		}

		const FCandidateZone* SelectedZone = CandidateZones.FindByPredicate(
			[CardCount](const FCandidateZone& Candidate)
			{
				return Candidate.InstanceIds.Num() >= CardCount;
			});
		if (!SelectedZone)
		{
			int32 MaximumAvailable = 0;
			for (const FCandidateZone& Candidate : CandidateZones)
			{
				MaximumAvailable = FMath::Max(
					MaximumAvailable,
					Candidate.InstanceIds.Num());
			}
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[BackpackCapture] 出售候选不足：Requested=%d Available=%d"),
				CardCount,
				MaximumAvailable);
			return false;
		}

		TArray<FGuid> InstanceIds;
		InstanceIds.Reserve(CardCount);
		for (int32 Index = 0; Index < CardCount; ++Index)
		{
			InstanceIds.Add(SelectedZone->InstanceIds[Index]);
		}
		UE_LOG(
			LogTemp,
			Display,
			TEXT("[BackpackCapture] 出售候选区：Zone=%d Cards=%d Available=%d"),
			static_cast<int32>(SelectedZone->Zone.Zone),
			CardCount,
			SelectedZone->InstanceIds.Num());

		Interaction.ClickBlank();
		for (int32 Index = 0; Index < InstanceIds.Num(); ++Index)
		{
			Interaction.ClickCard(
				InstanceIds[Index],
				/*bControlDown*/ Index > 0);
		}
		if (Interaction.GetSelection().OrderedSelectedInstanceIds.Num()
				!= InstanceIds.Num()
			|| !Interaction.BeginCarry(
				InstanceIds[0],
				FVector2D::ZeroVector,
				Run->GetBackpackStorageSnapshotRevision()))
		{
			return false;
		}

		const uint64 RevisionBeforeSale =
			Run->GetBackpackStorageSnapshotRevision();
		Interaction.NotifyReleaseGestureStarted();
		const FWacomBackpackWorkspaceReleaseIntent Intent =
			Interaction.BuildReleaseIntent(
				/*bReleaseAll*/ true,
				EWacomBackpackWorkspaceReleaseTargetKind::Delete);
		Screen->HandleWorkspaceReleaseIntent(Intent);
		const bool bSucceeded =
			Run->GetBackpackStorageSnapshotRevision() > RevisionBeforeSale
			&& !Interaction.IsCarrying();
		if (bSucceeded)
		{
			UE_LOG(
				LogTemp,
				Display,
				TEXT("[BackpackCapture] 自动出售：Cards=%d Result=Success Revision=%llu->%llu"),
				CardCount,
				RevisionBeforeSale,
				Run->GetBackpackStorageSnapshotRevision());
		}
		else
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[BackpackCapture] 自动出售：Cards=%d Result=Failed Revision=%llu->%llu"),
				CardCount,
				RevisionBeforeSale,
				Run->GetBackpackStorageSnapshotRevision());
		}
		return bSucceeded;
	}

	static bool GetSaleMetrics(
		UWorld* World,
		int32& OutQueuedCardCount,
		int32& OutActiveCardCount,
		int32& OutCompletedCardCount,
		int32& OutMaximumRealtimeCardCount)
	{
#if WITH_AUTOMATION_TESTS
		UWacomBackpackScreen* Screen = FindActiveScreen(World);
		if (!Screen || !Screen->WorkspaceWidget)
		{
			return false;
		}

		const FWacomBackpackWorkspaceAutomationTestView View =
			Screen->WorkspaceWidget->GetAutomationTestView();
		OutQueuedCardCount = View.SaleDepartureQueuedCardCount;
		OutActiveCardCount = View.SaleDepartureActiveCardCount;
		OutCompletedCardCount = View.SaleDepartureCompletedCardCount;
		OutMaximumRealtimeCardCount =
			View.SaleDepartureMaximumRealtimeCardCount;
		return true;
#else
		(void)World;
		(void)OutQueuedCardCount;
		(void)OutActiveCardCount;
		(void)OutCompletedCardCount;
		(void)OutMaximumRealtimeCardCount;
		return false;
#endif
	}
};

namespace
{
constexpr uint64 BackpackCaptureScreenMessageKey = 0x5741434F4D425043ull;
constexpr double TraceRestartDelaySeconds = 0.25;
constexpr TCHAR BackpackTraceChannels[] =
	TEXT("cpu,gpu,frame,log,bookmark,screenshot,region,slate");

const TCHAR* GetPhaseToken(EWacomBackpackPerformanceCapturePhase Phase)
{
	switch (Phase)
	{
	case EWacomBackpackPerformanceCapturePhase::Warmup:
		return TEXT("Warmup");
	case EWacomBackpackPerformanceCapturePhase::Closed:
		return TEXT("Closed");
	case EWacomBackpackPerformanceCapturePhase::Opening:
		return TEXT("Opening");
	case EWacomBackpackPerformanceCapturePhase::Idle:
		return TEXT("Idle");
	case EWacomBackpackPerformanceCapturePhase::Interaction:
		return TEXT("Interaction");
	case EWacomBackpackPerformanceCapturePhase::Finalizing:
		return TEXT("Finalizing");
	case EWacomBackpackPerformanceCapturePhase::Complete:
	default:
		return TEXT("Complete");
	}
}

FString SanitizeCaptureLabel(FString Label)
{
	Label.TrimStartAndEndInline();
	for (TCHAR& Character : Label)
	{
		if (!FChar::IsAlnum(Character)
			&& Character != TEXT('_')
			&& Character != TEXT('-'))
		{
			Character = TEXT('_');
		}
	}
	Label.LeftInline(48);
	return Label;
}

bool SetConsoleVariable(const TCHAR* Name, int32 Value)
{
	IConsoleVariable* Variable =
		IConsoleManager::Get().FindConsoleVariable(Name);
	if (!Variable)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[BackpackCapture] CVar 不存在：%s"),
			Name);
		return false;
	}

	Variable->Set(Value, ECVF_SetByConsole);
	return true;
}

enum class EWacomBackpackPerformanceCaptureKind : uint8
{
	Presentation,
	SaleDeparture,
};

struct FWacomBackpackPerformanceCaptureRequest
{
	TWeakObjectPtr<UWorld> World;
	FString Label;
	FString TracePath;
	FString RegionPrefix;
	EWacomBackpackPerformanceCaptureKind Kind =
		EWacomBackpackPerformanceCaptureKind::Presentation;
	int32 OwnedCardTarget = 24;
	int32 Width = 1280;
	int32 Height = 720;
	int32 SaleCardCount = 0;
	double TraceStartSeconds = 0.0;
	double PendingTraceStartSeconds = 0.0;
	EWacomBackpackPerformanceCapturePhase Phase =
		EWacomBackpackPerformanceCapturePhase::Warmup;
	FString OpenRegionName;
	int32 LastDisplayedSecond = INDEX_NONE;
	bool bSimplifiedMotion = false;
	bool bSaleTriggered = false;
	bool bTraceStarted = false;

	bool IsSaleCapture() const
	{
		return Kind == EWacomBackpackPerformanceCaptureKind::SaleDeparture;
	}
};

class FWacomBackpackPerformanceCaptureManager
{
public:
	void Start(
		const TArray<FString>& Args,
		UWorld* World,
		EWacomBackpackPerformanceCaptureKind Kind)
	{
		if (ActiveRequest.IsSet())
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[BackpackCapture] 已有采样正在运行；先使用 Wacom.Backpack.CancelPerformanceCapture。"));
			return;
		}

		if (!World || !World->IsGameWorld())
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[BackpackCapture] 命令必须在 PIE 游戏世界中执行。"));
			return;
		}

		const bool bSaleCapture =
			Kind == EWacomBackpackPerformanceCaptureKind::SaleDeparture;
		const int32 ExpectedArgumentCount = bSaleCapture ? 3 : 4;
		if (Args.Num() != ExpectedArgumentCount)
		{
			LogUsage(Kind);
			return;
		}

		const FString Label = SanitizeCaptureLabel(Args[0]);
		int32 OwnedCardTarget = 24;
		int32 Width = 1280;
		int32 Height = 720;
		int32 SaleCardCount = 0;
		bool bSimplifiedMotion = false;
		if (bSaleCapture)
		{
			SaleCardCount = FCString::Atoi(*Args[1]);
			const bool bSupportedSaleCount =
				SaleCardCount == 1
				|| SaleCardCount == 4
				|| SaleCardCount == 5
				|| SaleCardCount == 18;
			const bool bFullMotion =
				Args[2].Equals(TEXT("Full"), ESearchCase::IgnoreCase);
			bSimplifiedMotion =
				Args[2].Equals(TEXT("Simplified"), ESearchCase::IgnoreCase);
			if (Label.IsEmpty()
				|| !Args[1].IsNumeric()
				|| !bSupportedSaleCount
				|| (!bFullMotion && !bSimplifiedMotion))
			{
				LogUsage(Kind);
				return;
			}
			OwnedCardTarget = 100;
			Width = 1920;
			Height = 1080;
		}
		else
		{
			OwnedCardTarget = FCString::Atoi(*Args[1]);
			Width = FCString::Atoi(*Args[2]);
			Height = FCString::Atoi(*Args[3]);
			const bool bSupportedCardCount =
				OwnedCardTarget == 24 || OwnedCardTarget == 100;
			const bool bSupportedResolution =
				(Width == 1280 && Height == 720)
				|| (Width == 1920 && Height == 1080);
			if (Label.IsEmpty()
				|| !Args[1].IsNumeric()
				|| !Args[2].IsNumeric()
				|| !Args[3].IsNumeric()
				|| !bSupportedCardCount
				|| !bSupportedResolution)
			{
				LogUsage(Kind);
				return;
			}
		}

		if (Label.IsEmpty())
		{
			LogUsage(Kind);
			return;
		}

		if (!UE::Wacom::Backpack::PIEValidation::SeedToTarget(
				World,
				OwnedCardTarget)
			|| !UE::Wacom::Backpack::PIEValidation::CloseWorkspace(World))
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[BackpackCapture] Seed 或关闭背包失败，采样未启动。"));
			return;
		}

		ApplyStableSettings(World, Width, Height);

		const FString OutputDirectory = FPaths::ConvertRelativePathToFull(
			FPaths::Combine(
				FPaths::ProjectSavedDir(),
				TEXT("Profiling"),
				TEXT("Backpack")));
		IFileManager::Get().MakeDirectory(*OutputDirectory, true);

		FWacomBackpackPerformanceCaptureRequest Request;
		Request.World = World;
		Request.Label = Label;
		Request.Kind = Kind;
		Request.OwnedCardTarget = OwnedCardTarget;
		Request.Width = Width;
		Request.Height = Height;
		Request.SaleCardCount = SaleCardCount;
		Request.bSimplifiedMotion = bSimplifiedMotion;
		if (bSaleCapture)
		{
			const TCHAR* MotionToken =
				bSimplifiedMotion ? TEXT("Simplified") : TEXT("Full");
			Request.TracePath = FPaths::Combine(
				OutputDirectory,
				FString::Printf(
					TEXT("%s_sale_%d_%s_100_1920x1080.utrace"),
					*Label,
					SaleCardCount,
					MotionToken));
			Request.RegionPrefix = FString::Printf(
				TEXT("Backpack_%s_Sale_%d_%s_100_1920x1080"),
				*Label,
				SaleCardCount,
				MotionToken);
		}
		else
		{
			Request.TracePath = FPaths::Combine(
				OutputDirectory,
				FString::Printf(
					TEXT("%s_%d_%dx%d.utrace"),
					*Label,
					OwnedCardTarget,
					Width,
					Height));
			Request.RegionPrefix = FString::Printf(
				TEXT("Backpack_%s_%d_%dx%d"),
				*Label,
				OwnedCardTarget,
				Width,
				Height);
		}

		const double Now = FPlatformTime::Seconds();
		if (FTraceAuxiliary::IsConnected())
		{
			FTraceAuxiliary::Stop();
			Request.PendingTraceStartSeconds =
				Now + TraceRestartDelaySeconds;
		}
		else
		{
			Request.PendingTraceStartSeconds = Now;
		}

		ActiveRequest.Emplace(MoveTemp(Request));
		TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateRaw(
				this,
				&FWacomBackpackPerformanceCaptureManager::Tick));

		ShowMessage(
			TEXT("正在准备 Backpack Trace，请保持 PIE 窗口在前台……"),
			FColor::Yellow,
			3.0f);
	}

	void Cancel()
	{
		if (!ActiveRequest.IsSet())
		{
			UE_LOG(
				LogTemp,
				Display,
				TEXT("[BackpackCapture] 当前没有运行中的采样。"));
			return;
		}

		CancelInternal(TEXT("用户取消"));
		if (TickerHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
			TickerHandle.Reset();
		}
	}

private:
	static void LogUsage(EWacomBackpackPerformanceCaptureKind Kind)
	{
		if (Kind == EWacomBackpackPerformanceCaptureKind::SaleDeparture)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[BackpackCapture] 用法：Wacom.Backpack.CaptureSaleDepartureBaseline <label> <1|4|5|18> <Full|Simplified>"));
		}
		else
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[BackpackCapture] 用法：Wacom.Backpack.CapturePresentationBaseline <label> <24|100> <1280|1920> <720|1080>"));
		}
	}

	static void ApplyStableSettings(UWorld* World, int32 Width, int32 Height)
	{
		if (GEngine)
		{
			GEngine->Exec(World, TEXT("Scalability 3"));
			GEngine->Exec(
				World,
				*FString::Printf(
					TEXT("r.SetRes %dx%dw"),
					Width,
					Height));
		}

		SetConsoleVariable(TEXT("r.VSync"), 0);
		SetConsoleVariable(TEXT("r.VSyncEditor"), 0);
		SetConsoleVariable(TEXT("t.MaxFPS"), 0);
		SetConsoleVariable(TEXT("t.IdleWhenNotForeground"), 0);
		SetConsoleVariable(TEXT("Slate.bAllowThrottling"), 0);
	}

	bool Tick(float DeltaSeconds)
	{
		(void)DeltaSeconds;
		if (!ActiveRequest.IsSet())
		{
			TickerHandle.Reset();
			return false;
		}

		FWacomBackpackPerformanceCaptureRequest& Request =
			ActiveRequest.GetValue();
		UWorld* World = Request.World.Get();
		if (!World || !World->IsGameWorld())
		{
			CancelInternal(TEXT("PIE 世界已失效"));
			TickerHandle.Reset();
			return false;
		}

		const double Now = FPlatformTime::Seconds();
		if (!Request.bTraceStarted)
		{
			if (Now < Request.PendingTraceStartSeconds)
			{
				return true;
			}

			FTraceAuxiliary::FOptions Options;
			Options.bTruncateFile = true;
			Options.bExcludeTail = true;
			if (!FTraceAuxiliary::Start(
					FTraceAuxiliary::EConnectionType::File,
					*Request.TracePath,
					BackpackTraceChannels,
					&Options))
			{
				UE_LOG(
					LogTemp,
					Error,
					TEXT("[BackpackCapture] 无法启动 Trace：%s"),
					*Request.TracePath);
				ActiveRequest.Reset();
				TickerHandle.Reset();
				return false;
			}

			Request.bTraceStarted = true;
			Request.TraceStartSeconds = Now;
			Request.Phase =
				EWacomBackpackPerformanceCapturePhase::Warmup;
			BeginMeasuredRegion(Request);
			LogPhase(Request);
		}

		const double ElapsedSeconds =
			Now - Request.TraceStartSeconds;
		const EWacomBackpackPerformanceCapturePhase DesiredPhase =
			Request.IsSaleCapture()
				? FWacomBackpackSalePerformanceCaptureTimeline::ResolvePhase(
					ElapsedSeconds)
				: FWacomBackpackPerformanceCaptureTimeline::ResolvePhase(
					ElapsedSeconds);
		while (Request.Phase != DesiredPhase)
		{
			if (!AdvancePhase(Request))
			{
				TickerHandle.Reset();
				return false;
			}
		}

		UpdateCountdown(Request, ElapsedSeconds);
		return true;
	}

	bool AdvancePhase(FWacomBackpackPerformanceCaptureRequest& Request)
	{
		EndMeasuredRegion(Request);
		Request.Phase =
			FWacomBackpackPerformanceCaptureTimeline::NextPhase(
				Request.Phase);
		Request.LastDisplayedSecond = INDEX_NONE;

		if (Request.Phase
			== EWacomBackpackPerformanceCapturePhase::Opening)
		{
			UWorld* World = Request.World.Get();
			if (!UE::Wacom::Backpack::PIEValidation::OpenFormalWorkspace(
					World))
			{
				CancelInternal(TEXT("自动打开背包失败"));
				return false;
			}
		}
		else if (Request.IsSaleCapture()
			&& Request.Phase
				== EWacomBackpackPerformanceCapturePhase::Idle)
		{
			if (!FWacomBackpackPerformanceCaptureAccess::ConfigureMotion(
					Request.World.Get(),
					Request.bSimplifiedMotion))
			{
				CancelInternal(TEXT("无法为出售基线设置 Motion 模式"));
				return false;
			}
		}
		else if (Request.Phase
			== EWacomBackpackPerformanceCapturePhase::Complete)
		{
			Finish(Request);
			return false;
		}

		BeginMeasuredRegion(Request);
		LogPhase(Request);
		if (Request.IsSaleCapture()
			&& Request.Phase
				== EWacomBackpackPerformanceCapturePhase::Interaction)
		{
			if (Request.bSaleTriggered
				|| !FWacomBackpackPerformanceCaptureAccess::SubmitSale(
					Request.World.Get(),
					Request.SaleCardCount))
			{
				CancelInternal(TEXT("自动出售事务未成功提交"));
				return false;
			}
			Request.bSaleTriggered = true;
		}
		return true;
	}

	static void BeginMeasuredRegion(
		FWacomBackpackPerformanceCaptureRequest& Request)
	{
		if (!FWacomBackpackPerformanceCaptureTimeline::IsMeasuredPhase(
				Request.Phase))
		{
			Request.OpenRegionName.Reset();
			return;
		}

		Request.OpenRegionName = FString::Printf(
			TEXT("%s_%s"),
			*Request.RegionPrefix,
			GetPhaseToken(Request.Phase));
		TRACE_BEGIN_REGION(*Request.OpenRegionName);
	}

	static void EndMeasuredRegion(
		FWacomBackpackPerformanceCaptureRequest& Request)
	{
		if (Request.OpenRegionName.IsEmpty())
		{
			return;
		}

		TRACE_END_REGION(*Request.OpenRegionName);
		Request.OpenRegionName.Reset();
	}

	static void LogPhase(
		const FWacomBackpackPerformanceCaptureRequest& Request)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("[BackpackCapture] Phase=%s Trace=%s"),
			GetPhaseToken(Request.Phase),
			*Request.TracePath);
	}

	static void UpdateCountdown(
		FWacomBackpackPerformanceCaptureRequest& Request,
		double ElapsedSeconds)
	{
		const double PhaseEndSeconds = Request.IsSaleCapture()
			? FWacomBackpackSalePerformanceCaptureTimeline::GetPhaseEndSeconds(
				Request.Phase)
			: FWacomBackpackPerformanceCaptureTimeline::GetPhaseEndSeconds(
				Request.Phase);
		const int32 RemainingSeconds = FMath::Max(
			0,
			FMath::CeilToInt(PhaseEndSeconds - ElapsedSeconds));
		if (Request.LastDisplayedSecond == RemainingSeconds)
		{
			return;
		}
		Request.LastDisplayedSecond = RemainingSeconds;

		FString Instruction;
		FColor Color = FColor::Cyan;
		switch (Request.Phase)
		{
		case EWacomBackpackPerformanceCapturePhase::Warmup:
			Instruction = Request.IsSaleCapture()
				? TEXT("出售基线预热：背包保持关闭")
				: TEXT("预热：保持背包关闭");
			break;
		case EWacomBackpackPerformanceCapturePhase::Closed:
			Instruction = Request.IsSaleCapture()
				? TEXT("出售关闭基线：请勿操作或切出 PIE")
				: TEXT("关闭基线：保持背包关闭且勿切出 PIE");
			break;
		case EWacomBackpackPerformanceCapturePhase::Opening:
			Instruction = TEXT("正在自动打开背包并等待几何稳定，请勿操作");
			Color = FColor::Yellow;
			break;
		case EWacomBackpackPerformanceCapturePhase::Idle:
			Instruction = Request.IsSaleCapture()
				? FString::Printf(
					TEXT("出售准备：%d 张 / %s Motion，请勿操作"),
					Request.SaleCardCount,
					Request.bSimplifiedMotion
						? TEXT("Simplified")
						: TEXT("Full"))
				: TEXT("开启空闲：保持背包打开且勿操作");
			break;
		case EWacomBackpackPerformanceCapturePhase::Interaction:
			Instruction = Request.IsSaleCapture()
				? TEXT("正在自动提交真实出售事务并采集每卡材质离场；请勿操作")
				: TEXT("交互：展开/收起、底部 Hover、框选、5+ Carry、滚轮、合法/拒绝悬停、取消；不要出售");
			Color = FColor::Green;
			break;
		case EWacomBackpackPerformanceCapturePhase::Finalizing:
			Instruction = TEXT("测量已结束，正在未测量阶段安全关闭 Trace");
			Color = FColor::Yellow;
			break;
		case EWacomBackpackPerformanceCapturePhase::Complete:
		default:
			return;
		}

		ShowMessage(
			FString::Printf(
				TEXT("[Backpack Insights] %s\n剩余 %d 秒"),
				*Instruction,
				RemainingSeconds),
			Color,
			1.2f);
	}

	static void ShowMessage(
		const FString& Message,
		const FColor& Color,
		float DurationSeconds)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				BackpackCaptureScreenMessageKey,
				DurationSeconds,
				Color,
				Message,
				/*bNewerOnTop*/ true,
				FVector2D(1.15f, 1.15f));
		}
	}

	void Finish(const FWacomBackpackPerformanceCaptureRequest& Request)
	{
		const FString CompletedPath = Request.TracePath;
		if (Request.IsSaleCapture())
		{
			int32 QueuedCardCount = 0;
			int32 ActiveCardCount = 0;
			int32 CompletedCardCount = 0;
			int32 MaximumRealtimeCardCount = 0;
			if (FWacomBackpackPerformanceCaptureAccess::GetSaleMetrics(
					Request.World.Get(),
					QueuedCardCount,
					ActiveCardCount,
					CompletedCardCount,
					MaximumRealtimeCardCount))
			{
				const bool bMetricsPassed =
					QueuedCardCount == 0
					&& ActiveCardCount == 0
					&& CompletedCardCount == Request.SaleCardCount
					&& MaximumRealtimeCardCount
						<= FWacomBackpackWorkspaceSaleDepartureController::
							MaximumConcurrentCards;
				if (bMetricsPassed)
				{
					UE_LOG(
						LogTemp,
						Display,
						TEXT("[BackpackCapture] 出售离场统计：Queued=%d Active=%d Completed=%d MaxRealtime=%d Result=Pass"),
						QueuedCardCount,
						ActiveCardCount,
						CompletedCardCount,
						MaximumRealtimeCardCount);
				}
				else
				{
					UE_LOG(
						LogTemp,
						Warning,
						TEXT("[BackpackCapture] 出售离场统计：Queued=%d Active=%d Completed=%d MaxRealtime=%d Result=Fail ExpectedCompleted=%d"),
						QueuedCardCount,
						ActiveCardCount,
						CompletedCardCount,
						MaximumRealtimeCardCount,
						Request.SaleCardCount);
				}
			}
			else
			{
				UE_LOG(
					LogTemp,
					Warning,
					TEXT("[BackpackCapture] 无法读取出售离场统计。"));
			}
		}
		FTraceAuxiliary::Stop();
		UE_LOG(
			LogTemp,
			Display,
			TEXT("[BackpackCapture] 采样完成：%s"),
			*CompletedPath);
		ShowMessage(
			FString::Printf(
				TEXT("Backpack Insights 采样完成\n%s"),
				*CompletedPath),
			FColor::Green,
			12.0f);
		ActiveRequest.Reset();
	}

	void CancelInternal(const TCHAR* Reason)
	{
		if (!ActiveRequest.IsSet())
		{
			return;
		}

		FWacomBackpackPerformanceCaptureRequest& Request =
			ActiveRequest.GetValue();
		EndMeasuredRegion(Request);
		if (Request.bTraceStarted)
		{
			FTraceAuxiliary::Stop();
		}
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[BackpackCapture] 采样已终止：%s"),
			Reason);
		ShowMessage(
			FString::Printf(
				TEXT("Backpack Insights 采样已终止：%s"),
				Reason),
			FColor::Red,
			8.0f);
		ActiveRequest.Reset();
	}

	TOptional<FWacomBackpackPerformanceCaptureRequest> ActiveRequest;
	FTSTicker::FDelegateHandle TickerHandle;
};

FWacomBackpackPerformanceCaptureManager GBackpackPerformanceCaptureManager;

FAutoConsoleCommandWithWorldAndArgs GCaptureBackpackPresentationBaselineCommand(
	TEXT("Wacom.Backpack.CapturePresentationBaseline"),
	TEXT("仅编辑器 PIE：自动采集背包 Presentation Insights。用法：<label> <24|100> <1280|1920> <720|1080>。"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
		[](const TArray<FString>& Args, UWorld* World)
		{
			GBackpackPerformanceCaptureManager.Start(
				Args,
				World,
				EWacomBackpackPerformanceCaptureKind::Presentation);
		}));

FAutoConsoleCommandWithWorldAndArgs GCaptureBackpackSaleDepartureBaselineCommand(
	TEXT("Wacom.Backpack.CaptureSaleDepartureBaseline"),
	TEXT("仅编辑器 PIE：固定 100 卡/1920x1080，自动提交真实出售事务并采集材质离场。用法：<label> <1|4|5|18> <Full|Simplified>。"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
		[](const TArray<FString>& Args, UWorld* World)
		{
			GBackpackPerformanceCaptureManager.Start(
				Args,
				World,
				EWacomBackpackPerformanceCaptureKind::SaleDeparture);
		}));

FAutoConsoleCommand GCancelBackpackPerformanceCaptureCommand(
	TEXT("Wacom.Backpack.CancelPerformanceCapture"),
	TEXT("仅编辑器：取消当前背包 Insights 采样并安全关闭 Trace。"),
	FConsoleCommandDelegate::CreateLambda(
		[]()
		{
			GBackpackPerformanceCaptureManager.Cancel();
		}));
}

#endif
