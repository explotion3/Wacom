// Copyright Wacom. All Rights Reserved.

#include "UI/Menus/WacomMainMenuScreen.h"

#define LOCTEXT_NAMESPACE "WacomMainMenu"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

#include "RunSession.h"
#include "GameFramework/WacomGameMode.h"
#include "GameFramework/WacomMenuGameMode.h"
#include "UI/Menus/WacomConfirmDialog.h"

namespace
{
	/** 快速构造带居中文本的按钮。 */
	UButton* MakeLabelButton(UWidgetTree* Tree, FName Name, const FText& Label, UVerticalBox* Parent)
	{
		UButton* Btn = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);

		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetText(Label);
		Text->SetJustification(ETextJustify::Center);
		Btn->AddChild(Text);

		if (UVerticalBoxSlot* BtnSlot = Parent->AddChildToVerticalBox(Btn))
		{
			BtnSlot->SetPadding(FMargin(0.f, 8.f));
			BtnSlot->SetHorizontalAlignment(HAlign_Center);
		}
		return Btn;
	}
}

TSharedRef<SWidget> UWacomMainMenuScreen::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		// 垂直排列三个按钮，画面中央
		UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VBox"));
		if (UCanvasPanelSlot* CanvasSlot = Root->AddChildToCanvas(VBox))
		{
			CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CanvasSlot->SetOffsets(FMargin(-180.f, -150.f, 360.f, 300.f));
			CanvasSlot->SetAutoSize(false);
		}

		// 标题
		UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
		Title->SetText(LOCTEXT("Title", "Wacom"));
		Title->SetJustification(ETextJustify::Center);
		FSlateFontInfo Font = Title->GetFont();
		Font.Size = 48;
		Title->SetFont(Font);
		if (UVerticalBoxSlot* TitleSlot = VBox->AddChildToVerticalBox(Title))
		{
			TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 40.f));
			TitleSlot->SetHorizontalAlignment(HAlign_Center);
		}

		if (!NewGameButton)
		{
			NewGameButton = MakeLabelButton(WidgetTree, TEXT("NewGameButton"),
				LOCTEXT("NewGame", "新游戏"), VBox);
		}
		if (!ContinueButton)
		{
			ContinueButton = MakeLabelButton(WidgetTree, TEXT("ContinueButton"),
				LOCTEXT("Continue", "继续"), VBox);
		}
		if (!QuitButton)
		{
			QuitButton = MakeLabelButton(WidgetTree, TEXT("QuitButton"),
				LOCTEXT("QuitGame", "退出游戏"), VBox);
		}
	}
	return Super::RebuildWidget();
}

void UWacomMainMenuScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// 注意：此时 RebuildWidget 可能还没执行，NewGameButton 等 BindWidget/自建 Widget
	// 都还没就位。OnClicked 的绑定放在 NativeConstruct。
}

void UWacomMainMenuScreen::NativeConstruct()
{
	Super::NativeConstruct();

	if (NewGameButton)
	{
		NewGameButton->OnClicked.AddUniqueDynamic(this, &UWacomMainMenuScreen::HandleNewGameClicked);
	}
	if (ContinueButton)
	{
		ContinueButton->OnClicked.AddUniqueDynamic(this, &UWacomMainMenuScreen::HandleContinueClicked);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.AddUniqueDynamic(this, &UWacomMainMenuScreen::HandleQuitClicked);
	}

	UE_LOG(LogTemp, Display,
		TEXT("[MainMenu] NativeConstruct: NewGame=%s Continue=%s Quit=%s"),
		NewGameButton  ? TEXT("OK") : TEXT("null"),
		ContinueButton ? TEXT("OK") : TEXT("null"),
		QuitButton     ? TEXT("OK") : TEXT("null"));

	RefreshContinueEnabled();
}

void UWacomMainMenuScreen::RefreshContinueEnabled()
{
	if (!ContinueButton) { return; }

	// 存档系统暂停（Stage 0.1）：Continue 永远禁用。
	if (!AWacomGameMode::bSaveSystemEnabled)
	{
		ContinueButton->SetIsEnabled(false);
		return;
	}

	const bool bHasMain = UGameplayStatics::DoesSaveGameExist(AWacomGameMode::SlotName_Main, /*UserIndex*/0);
	ContinueButton->SetIsEnabled(bHasMain);
}

void UWacomMainMenuScreen::HandleNewGameClicked()
{
	// 存档系统暂停（Stage 0.1）：直接开新游戏，不弹 Confirm。
	if (!AWacomGameMode::bSaveSystemEnabled)
	{
		if (UWorld* World = GetWorld())
		{
			if (AWacomMenuGameMode* GM = World->GetAuthGameMode<AWacomMenuGameMode>())
			{
				GM->RequestStartNewGame();
			}
		}
		return;
	}

	// 有存档时弹确认对话框
	if (UGameplayStatics::DoesSaveGameExist(AWacomGameMode::SlotName_Main, 0))
	{
		UWacomConfirmDialog::Show(
			this,
			LOCTEXT("NewGameConfirmTitle", "新游戏"),
			LOCTEXT("NewGameConfirmMsg", "开始新游戏将覆盖现有存档，确定继续？"),
			[this]()
			{
				// 确认：委托给 MenuGameMode
				if (UWorld* World = GetWorld())
				{
					if (AWacomMenuGameMode* GM = World->GetAuthGameMode<AWacomMenuGameMode>())
					{
						GM->RequestStartNewGame();
					}
				}
			});
		return;
	}

	// 无存档直接开
	if (UWorld* World = GetWorld())
	{
		if (AWacomMenuGameMode* GM = World->GetAuthGameMode<AWacomMenuGameMode>())
		{
			GM->RequestStartNewGame();
		}
	}
}

void UWacomMainMenuScreen::HandleContinueClicked()
{
	// 存档系统暂停（Stage 0.1）：忽略 Continue。
	if (!AWacomGameMode::bSaveSystemEnabled)
	{
		UE_LOG(LogTemp, Display, TEXT("[MainMenu] Continue 被点但存档系统已暂停"));
		return;
	}

	if (!UGameplayStatics::DoesSaveGameExist(AWacomGameMode::SlotName_Main, 0))
	{
		UE_LOG(LogTemp, Warning, TEXT("[MainMenu] Continue 被按到但无存档"));
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (AWacomMenuGameMode* GM = World->GetAuthGameMode<AWacomMenuGameMode>())
		{
			GM->RequestContinueGame();
			return;
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("[MainMenu] 找不到 AWacomMenuGameMode"));
}

void UWacomMainMenuScreen::HandleQuitClicked()
{
	UWacomConfirmDialog::Show(
		this,
		LOCTEXT("QuitConfirmTitle", "退出游戏"),
		LOCTEXT("QuitConfirmMsg", "确定要退出游戏吗？"),
		[this]()
		{
			UE_LOG(LogTemp, Display, TEXT("[MainMenu] Quit confirmed"));
			UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
		});
}

#undef LOCTEXT_NAMESPACE
