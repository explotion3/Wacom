// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "Blueprint/WidgetTree.h"
#include "Cards/CardDefinition.h"
#include "Components/CanvasPanel.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "../BackpackScreenTestAccess.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCarryWheelLayerSpec,
	"Wacom.UI.Backpack.Workspace.CarryWheelLayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCarryWheelLayerSpec::RunTest(const FString& Parameters)
{
	constexpr int32 CardCount = 7;
	TStrongObjectPtr<UWacomBackpackWorkspaceWidget> Workspace(
		NewObject<UWacomBackpackWorkspaceWidget>());
	const TSharedRef<SWidget> WorkspaceSlate = Workspace->TakeWidget();
	TSharedPtr<FWacomBackpackWorkspaceInteractionModel> Model =
		MakeShared<FWacomBackpackWorkspaceInteractionModel>();
	Workspace->SetInteractionModel(Model, nullptr);

	TStrongObjectPtr<UCardDefinition> Definition(NewObject<UCardDefinition>());
	Definition->CardId = TEXT("Backpack.Carry.WheelLayer");
	TArray<TStrongObjectPtr<UWacomDeckCardWidget>> OwnedCards;
	TArray<TObjectPtr<UWacomDeckCardWidget>> Cards;
	for (int32 Index = 0; Index < CardCount; ++Index)
	{
		TStrongObjectPtr<UWacomDeckCardWidget> Card(NewObject<UWacomDeckCardWidget>());
		FCardInstance Instance;
		Instance.InstanceId = FGuid(Index + 1, 901, 902, 903);
		Instance.Definition = Definition.Get();
		Card->SetCard(Instance, EZoneKind::Backpack, FGuid());
		Workspace->GetStaticCardLayer()->AddChildToCanvas(Card.Get());
		Workspace->PrimeCardBaseLayout(
			*Card,
			FVector2D(180.0f + Index * 48.0f, 240.0f),
			FVector2D(220.0f, 320.0f),
			0.0f,
			Index);
		Cards.Add(Card.Get());
		OwnedCards.Add(MoveTemp(Card));
	}
	Workspace->BindWorkspaceCards(Cards, 901);
	Model->SelectAllMovable();
	TestTrue(TEXT("Multi-card carry begins"),
		Model->BeginCarry(Cards[0]->GetCardInstanceId(), FVector2D(500.0f, 350.0f), 901));
	FWacomBackpackScreenTestAccess::RefreshWorkspacePresentation(*Workspace);

	const auto FindCurrentCard = [&]() -> UWacomDeckCardWidget*
	{
		const FWacomBackpackWorkspaceCarryState& Carry = Model->GetCarry();
		if (!Carry.RemainingInstanceIds.IsValidIndex(Carry.CurrentIndex))
		{
			return nullptr;
		}
		const FGuid CurrentId = Carry.RemainingInstanceIds[Carry.CurrentIndex];
		for (UWacomDeckCardWidget* Card : Cards)
		{
			if (Card && Card->GetCardInstanceId() == CurrentId)
			{
				return Card;
			}
		}
		return nullptr;
	};

	UWacomDeckCardWidget* InitialCurrent = FindCurrentCard();
	TestNotNull(TEXT("Initial current card exists"), InitialCurrent);
	TestEqual(
		TEXT("Initial current card starts in the active carry layer"),
		InitialCurrent ? InitialCurrent->GetParent() : nullptr,
		static_cast<UPanelWidget*>(Workspace->GetCarryActiveCanvas()));

	for (int32 Step = 1; Step <= 4; ++Step)
	{
		TestTrue(
			*FString::Printf(TEXT("Wheel step %d is handled"), Step),
			FWacomBackpackScreenTestAccess::StepWorkspaceCarryCurrentByWheel(
				*Workspace,
				1.0f));
		UWacomDeckCardWidget* Current = FindCurrentCard();
		TestNotNull(
			*FString::Printf(TEXT("Wheel step %d resolves a current card"), Step),
			Current);
		TestEqual(
			*FString::Printf(TEXT("Wheel step %d promotes the new current card to the active carry layer"), Step),
			Current ? Current->GetParent() : nullptr,
			static_cast<UPanelWidget*>(Workspace->GetCarryActiveCanvas()));
		if (Step >= 2 && InitialCurrent)
		{
			TestEqual(
				*FString::Printf(TEXT("Wheel step %d returns the stale initial current card to the cached carry layer"), Step),
				InitialCurrent->GetParent(),
				static_cast<UPanelWidget*>(Workspace->GetCarryCanvas()));
		}
	}

	return true;
}

#endif
