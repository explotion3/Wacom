// Copyright Wacom. All Rights Reserved.

#include "UI/Menus/WacomPauseMenuScreen.h"

#define LOCTEXT_NAMESPACE "WacomPauseMenu"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#include "GameFramework/WacomGameMode.h"
#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"
#include "UI/Foundation/WacomGameUIManagerSubsystem.h"
#include "UI/Menus/WacomConfirmDialog.h"

namespace
{
	const FName WacomMainMenuLevelPackagePath(TEXT("/Game/Wacom/Maps/L_MainMenu"));

	UButton* MakePauseButton(UWidgetTree* Tree, FName Name, const FText& Label, UVerticalBox* Parent)
	{
		UButton* Btn = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetText(Label);
		Text->SetJustification(ETextJustify::Center);
		Btn->AddChild(Text);
		if (UVerticalBoxSlot* BtnSlot = Parent->AddChildToVerticalBox(Btn))
		{
			BtnSlot->SetPadding(FMargin(0.f, 6.f));
			BtnSlot->SetHorizontalAlignment(HAlign_Center);
		}
		return Btn;
	}
}

FName UWacomPauseMenuScreen::GetMainMenuLevelPackagePathForTravel()
{
	return WacomMainMenuLevelPackagePath;
}

TSharedRef<SWidget> UWacomPauseMenuScreen::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VBox"));
		if (UCanvasPanelSlot* CanvasSlot = Root->AddChildToCanvas(VBox))
		{
			CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CanvasSlot->SetOffsets(FMargin(-140.f, -120.f, 280.f, 240.f));
			CanvasSlot->SetAutoSize(false);
		}

		UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
		Title->SetText(LOCTEXT("Title", "暂停"));
		Title->SetJustification(ETextJustify::Center);
		FSlateFontInfo Font = Title->GetFont();
		Font.Size = 36;
		Title->SetFont(Font);
		if (UVerticalBoxSlot* TitleSlot = VBox->AddChildToVerticalBox(Title))
		{
			TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 24.f));
			TitleSlot->SetHorizontalAlignment(HAlign_Center);
		}

		if (!ResumeButton)     { ResumeButton     = MakePauseButton(WidgetTree, TEXT("ResumeButton"),     LOCTEXT("Resume", "继续游戏"),     VBox); }
		// 存档系统关闭时不创建 Save 按钮。
		if (!SaveButton && AWacomGameMode::bSaveSystemEnabled)
		{
			SaveButton = MakePauseButton(WidgetTree, TEXT("SaveButton"), LOCTEXT("Save", "保存"), VBox);
		}
		if (!QuitToMenuButton) { QuitToMenuButton = MakePauseButton(WidgetTree, TEXT("QuitToMenuButton"), LOCTEXT("QuitToMenu", "回到主菜单"), VBox); }
	}
	return Super::RebuildWidget();
}

void UWacomPauseMenuScreen::NativeConstruct()
{
	Super::NativeConstruct();

	if (ResumeButton)     { ResumeButton    ->OnClicked.AddUniqueDynamic(this, &UWacomPauseMenuScreen::HandleResumeClicked); }
	if (SaveButton)       { SaveButton      ->OnClicked.AddUniqueDynamic(this, &UWacomPauseMenuScreen::HandleSaveClicked); }
	if (QuitToMenuButton) { QuitToMenuButton->OnClicked.AddUniqueDynamic(this, &UWacomPauseMenuScreen::HandleQuitToMenuClicked); }
}

void UWacomPauseMenuScreen::HandleResumeClicked()
{
	UE_LOG(LogTemp, Display, TEXT("[PauseMenu] Resume"));
	DeactivateWidget();
}

void UWacomPauseMenuScreen::HandleSaveClicked()
{
	APlayerController* PC = GetOwningPlayer();
	AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(PC);
	URunSession* Run = WacomPC ? WacomPC->GetRunSession() : nullptr;
	if (Run)
	{
		Run->SaveToSlot(AWacomGameMode::SlotName_Main);
		UE_LOG(LogTemp, Display, TEXT("[PauseMenu] Save OK"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PauseMenu] Save 失败：RunSession 未就位"));
	}
}

void UWacomPauseMenuScreen::HandleQuitToMenuClicked()
{
	UWacomConfirmDialog::Show(
		this,
		LOCTEXT("QuitToMenuTitle", "回到主菜单"),
		LOCTEXT("QuitToMenuMsg", "未保存的进度将丢失。确定返回主菜单？"),
		[this]()
		{
			UE_LOG(LogTemp, Display, TEXT("[PauseMenu] Quit to Main Menu confirmed"));

			UWorld* World = GetWorld();
			bool bTeardownCompleted = false;
			if (UGameInstance* GI = GetGameInstance())
			{
				if (UWacomGameUIManagerSubsystem* UIManager = GI->GetSubsystem<UWacomGameUIManagerSubsystem>())
				{
					UIManager->TearDownPrimaryLayout();
					bTeardownCompleted = true;
				}
			}

			UE_LOG(LogTemp, Display,
				TEXT("[PauseMenu] ScheduleTravel Target=%s World=%s PIE=%s Teardown=%s"),
				*WacomMainMenuLevelPackagePath.ToString(),
				*GetNameSafe(World),
				(World && World->WorldType == EWorldType::PIE) ? TEXT("true") : TEXT("false"),
				bTeardownCompleted ? TEXT("true") : TEXT("false"));

			if (World)
			{
				TWeakObjectPtr<UWorld> WeakWorld(World);
				World->GetTimerManager().SetTimerForNextTick(
					FTimerDelegate::CreateLambda([WeakWorld]()
					{
						if (!WeakWorld.IsValid())
						{
							return;
						}

						UE_LOG(LogTemp, Display,
							TEXT("[PauseMenu] ExecuteTravel Target=%s"),
							*WacomMainMenuLevelPackagePath.ToString());
						UGameplayStatics::OpenLevel(WeakWorld.Get(), WacomMainMenuLevelPackagePath);
					}));
			}
		});
}

#undef LOCTEXT_NAMESPACE
