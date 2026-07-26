// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "../../../../WacomApp/Private/UI/Card/WacomCardExplanationLexiconProvider.h"
#include "../../../../WacomApp/Private/UI/Card/WacomCardSemanticTextHitLayout.h"
#include "../../../../WacomApp/Private/UI/Card/WacomWorldCardInteractionPresenter.h"
#include "Actors/WacomWorldShopActor.h"
#include "Blueprint/UserWidget.h"
#include "Cards/CardDefinition.h"
#include "Components/WidgetComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Fonts/FontMeasure.h"
#include "GameFramework/WacomPlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Card/WacomCardExplanationLexicon.h"
#include "UI/Card/WacomCardPresentationBuilder.h"
#include "UI/Card/WacomCardView.h"
#include "UI/Foundation/WacomUIDeveloperSettings.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomWorldCardInteractionSpec
{
	UCardDefinition* MakeSemanticCard(UObject* Outer)
	{
		UCardDefinition* Card = NewObject<UCardDefinition>(Outer);
		Card->CardId = TEXT("WorldCard.Semantics");
		Card->DisplayName = FText::FromString(TEXT("语义卡"));
		Card->Keywords.AddTag(WacomTags::Card_Keyword_Swift);
		Card->Keywords.AddTag(WacomTags::Card_Keyword_Retain);
		Card->Keywords.AddTag(WacomTags::Card_Keyword_Tool);
		return Card;
	}

	bool TestTokenRange(
		FAutomationTestBase& Test,
		const FString& FullText,
		const FWacomCardFaceSemanticTokenView& Token)
	{
		const bool bValid = Token.HasValidRangeFor(FullText);
		Test.TestTrue(TEXT("Semantic range is valid"), bValid);
		if (!bValid)
		{
			return false;
		}
		Test.TestEqual(
			TEXT("Semantic range reproduces display text"),
			FullText.Mid(Token.StartIndex, Token.Length),
			Token.DisplayText.ToString());
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIWorldCardInteractionSemanticContractSpec,
	"Wacom.UI.WorldCardInteraction.SemanticContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIWorldCardInteractionSemanticContractSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomWorldCardInteractionSpec;
	TStrongObjectPtr<UCardDefinition> Card(
		MakeSemanticCard(GetTransientPackage()));
	const FWacomCardViewData View =
		UWacomCardPresentationBuilder::BuildCardViewData(Card.Get());
	TestEqual(
		TEXT("TypeText is generated from ordered semantic tokens"),
		View.TypeText.ToString(),
		FString(TEXT("迅捷 / 保留 / 工具")));
	TestEqual(
		TEXT("All visible semantic words have tokens"),
		View.TypeSemanticTokens.Num(),
		3);
	for (const FWacomCardFaceSemanticTokenView& Token :
		View.TypeSemanticTokens)
	{
		TestTokenRange(*this, View.TypeText.ToString(), Token);
	}
	if (View.TypeSemanticTokens.Num() == 3)
	{
		TestEqual(
			TEXT("First token preserves source tag"),
			View.TypeSemanticTokens[0].SourceTag,
			WacomTags::Card_Keyword_Swift.GetTag());
		TestTrue(
			TEXT("Token ranges remain ordered"),
			View.TypeSemanticTokens[0].StartIndex
				< View.TypeSemanticTokens[1].StartIndex
				&& View.TypeSemanticTokens[1].StartIndex
					< View.TypeSemanticTokens[2].StartIndex);
	}

	Card->Keywords.Reset();
	Card->Physique.Capacity = 3;
	Card->Physique.CapacityEffect = FGameplayTag();
	const FWacomCardViewData Backpack =
		UWacomCardPresentationBuilder::BuildCardViewData(Card.Get());
	TestEqual(TEXT("Capacity A displays backpack"), Backpack.TypeText.ToString(), FString(TEXT("背包")));
	TestEqual(TEXT("Capacity A has one semantic"), Backpack.TypeSemanticTokens.Num(), 1);
	if (Backpack.TypeSemanticTokens.Num() == 1)
	{
		TestEqual(
			TEXT("Capacity A uses stable UI semantic"),
			Backpack.TypeSemanticTokens[0].SemanticId,
			WacomCardFaceSemanticIds::Backpack);
	}

	Card->Physique.CapacityEffect =
		WacomTags::Card_CapacityEffect_Placeholder;
	const FWacomCardViewData Container =
		UWacomCardPresentationBuilder::BuildCardViewData(Card.Get());
	TestEqual(TEXT("Capacity B displays container"), Container.TypeText.ToString(), FString(TEXT("容器")));
	TestEqual(TEXT("Capacity B has one semantic"), Container.TypeSemanticTokens.Num(), 1);
	if (Container.TypeSemanticTokens.Num() == 1)
	{
		TestEqual(
			TEXT("Capacity B uses stable UI semantic"),
			Container.TypeSemanticTokens[0].SemanticId,
			WacomCardFaceSemanticIds::Container);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIWorldCardInteractionLexiconSpec,
	"Wacom.UI.WorldCardInteraction.Lexicon",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIWorldCardInteractionLexiconSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardExplanationLexicon> Lexicon(
		NewObject<UWacomCardExplanationLexicon>());
	const TArray<TPair<FName, FGameplayTag>> Expected = {
		{ WacomTags::Card_Keyword_Swift.GetTag().GetTagName(), WacomTags::Card_Keyword_Swift },
		{ WacomTags::Card_Keyword_Retain.GetTag().GetTagName(), WacomTags::Card_Keyword_Retain },
		{ WacomTags::Card_Keyword_Combo.GetTag().GetTagName(), WacomTags::Card_Keyword_Combo },
		{ WacomTags::Card_Keyword_Companion.GetTag().GetTagName(), WacomTags::Card_Keyword_Companion },
		{ WacomTags::Card_Keyword_Weapon.GetTag().GetTagName(), WacomTags::Card_Keyword_Weapon },
		{ WacomTags::Card_Keyword_Tool.GetTag().GetTagName(), WacomTags::Card_Keyword_Tool },
		{ WacomTags::Card_Keyword_Hand.GetTag().GetTagName(), WacomTags::Card_Keyword_Hand },
		{ WacomTags::Card_Keyword_Exhaust.GetTag().GetTagName(), WacomTags::Card_Keyword_Exhaust },
		{ WacomTags::Card_Keyword_BagProvider.GetTag().GetTagName(), WacomTags::Card_Keyword_BagProvider },
		{ WacomTags::Card_Keyword_DeleteProvider.GetTag().GetTagName(), WacomTags::Card_Keyword_DeleteProvider },
		{ WacomCardFaceSemanticIds::Backpack, FGameplayTag() },
		{ WacomCardFaceSemanticIds::Container, FGameplayTag() },
	};
	for (const TPair<FName, FGameplayTag>& Semantic : Expected)
	{
		FWacomCardFaceSemanticLexiconEntry Entry;
		TestTrue(
			*FString::Printf(
				TEXT("Default lexicon covers %s"),
				*Semantic.Key.ToString()),
			Lexicon->FindCardFaceSemantic(
				Semantic.Key,
				Semantic.Value,
				Entry));
		TestFalse(TEXT("Semantic display name is player-facing"), Entry.DisplayName.IsEmpty());
		TestFalse(TEXT("Semantic description is player-facing"), Entry.Description.IsEmpty());
	}

	FWacomCardFaceSemanticLexiconEntry Override;
	Override.SemanticId = WacomTags::Card_Keyword_Swift.GetTag().GetTagName();
	Override.SourceTag = WacomTags::Card_Keyword_Swift;
	Override.DisplayName = FText::FromString(TEXT("覆盖迅捷"));
	Override.Description = FText::FromString(TEXT("覆盖说明"));
	Lexicon->CardFaceSemantics.Add(Override);
	FWacomCardFaceSemanticLexiconEntry Resolved;
	TestTrue(
		TEXT("Authored entry resolves"),
		Lexicon->FindCardFaceSemantic(
			Override.SemanticId,
			Override.SourceTag,
			Resolved));
	TestEqual(
		TEXT("Latest authored exact entry wins"),
		Resolved.DisplayName.ToString(),
		FString(TEXT("覆盖迅捷")));
	TestFalse(
		TEXT("Unknown semantic fails closed"),
		Lexicon->FindCardFaceSemantic(
			TEXT("Card.Face.Missing"),
			FGameplayTag(),
			Resolved));

	FWacomCardFaceSemanticLexiconEntry DisplayNameOnly;
	DisplayNameOnly.SemanticId = WacomTags::Card_Keyword_Retain.GetTag().GetTagName();
	DisplayNameOnly.SourceTag = WacomTags::Card_Keyword_Retain;
	DisplayNameOnly.DisplayName = FText::FromString(TEXT("只改显示名"));
	Lexicon->CardFaceSemantics.Add(DisplayNameOnly);
	FWacomCardFaceSemanticLexiconEntry PartialEntry;
	TestTrue(
		TEXT("Entry missing Description still matches"),
		Lexicon->FindCardFaceSemantic(
			DisplayNameOnly.SemanticId,
			DisplayNameOnly.SourceTag,
			PartialEntry));
	TestTrue(
		TEXT("Single-lexicon lookup does not invent a Description"),
		PartialEntry.Description.IsEmpty());
	return true;
}

/**
 * Locks the provider contract that a configured asset overrides the C++ default
 * lexicon per field, so authoring only DisplayName keeps the default tooltip
 * body instead of silently blanking it.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIWorldCardInteractionLexiconFieldMergeSpec,
	"Wacom.UI.WorldCardInteraction.LexiconFieldMerge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIWorldCardInteractionLexiconFieldMergeSpec::RunTest(
	const FString& /*Parameters*/)
{
	UWacomUIDeveloperSettings* Settings = GetMutableDefault<UWacomUIDeveloperSettings>();
	if (!TestNotNull(TEXT("UI developer settings exist"), Settings))
	{
		return false;
	}

	const FName RetainId = WacomTags::Card_Keyword_Retain.GetTag().GetTagName();
	FWacomCardFaceSemanticLexiconEntry DefaultEntry;
	if (!TestTrue(
			TEXT("C++ default lexicon covers Retain"),
			GetDefault<UWacomCardExplanationLexicon>()->FindCardFaceSemantic(
				RetainId,
				WacomTags::Card_Keyword_Retain,
				DefaultEntry)))
	{
		return false;
	}

	TStrongObjectPtr<UWacomCardExplanationLexicon> Configured(
		NewObject<UWacomCardExplanationLexicon>(
			GetTransientPackage(),
			TEXT("WacomCardExplanationLexicon_FieldMergeSpec")));
	FWacomCardFaceSemanticLexiconEntry DisplayNameOnly;
	DisplayNameOnly.SemanticId = RetainId;
	DisplayNameOnly.SourceTag = WacomTags::Card_Keyword_Retain;
	DisplayNameOnly.DisplayName = FText::FromString(TEXT("留手"));
	Configured->CardFaceSemantics.Add(DisplayNameOnly);

	const TSoftObjectPtr<UWacomCardExplanationLexicon> PreviousLexicon =
		Settings->CardExplanationLexicon;
	Settings->CardExplanationLexicon = Configured.Get();
	WacomCardExplanationLexiconProvider::ClearCachedLexiconForTests();

	FWacomCardFaceSemanticLexiconEntry Merged;
	const bool bResolved =
		WacomCardExplanationLexiconProvider::FindCardFaceSemantic(
			RetainId,
			WacomTags::Card_Keyword_Retain,
			Merged);

	Settings->CardExplanationLexicon = PreviousLexicon;
	WacomCardExplanationLexiconProvider::ClearCachedLexiconForTests();

	if (!TestTrue(TEXT("Provider resolves the authored semantic"), bResolved))
	{
		return false;
	}
	TestEqual(
		TEXT("Authored DisplayName wins"),
		Merged.DisplayName.ToString(),
		FString(TEXT("留手")));
	TestEqual(
		TEXT("Empty authored Description falls back to the C++ default"),
		Merged.Description.ToString(),
		DefaultEntry.Description.ToString());
	TestFalse(
		TEXT("Merged tooltip body stays player-facing"),
		Merged.Description.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIWorldCardInteractionTextHitSpec,
	"Wacom.UI.WorldCardInteraction.TextHit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIWorldCardInteractionTextHitSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomWorldCardInteractionSpec;
	if (!FSlateApplication::IsInitialized())
	{
		AddWarning(TEXT("Slate is unavailable; semantic layout test skipped."));
		return true;
	}
	TStrongObjectPtr<UCardDefinition> Card(
		MakeSemanticCard(GetTransientPackage()));
	const FWacomCardViewData View =
		UWacomCardPresentationBuilder::BuildCardViewData(Card.Get());
	const FString FullText = View.TypeText.ToString();
	const FSlateFontInfo Font =
		FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 16);
	const TSharedRef<FSlateFontMeasure> FontMeasure =
		FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const float FullWidth = FVector2D(
		FontMeasure->Measure(FullText, Font)).X;
	const FVector2D WideSize(FullWidth + 80.0f, 80.0f);
	const float LineStartX = (WideSize.X - FullWidth) * 0.5f;

	for (const FWacomCardFaceSemanticTokenView& Token :
		View.TypeSemanticTokens)
	{
		const float PrefixWidth = FVector2D(FontMeasure->Measure(
			FStringView(FullText).Left(Token.StartIndex),
			Font)).X;
		const float TokenWidth = FVector2D(FontMeasure->Measure(
			Token.DisplayText,
			Font)).X;
		FWacomCardFaceSemanticTokenView Hit;
		TestTrue(
			*FString::Printf(TEXT("Hit resolves %s"), *Token.SemanticId.ToString()),
			WacomCardSemanticTextHitLayout::ResolveTokenAtLocalPosition(
				FullText,
				View.TypeSemanticTokens,
				Font,
				WideSize,
				ETextJustify::Center,
				FVector2D(
					LineStartX + PrefixWidth + TokenWidth * 0.5f,
					8.0f),
				Hit));
		TestEqual(TEXT("Hit keeps semantic identity"), Hit.SemanticId, Token.SemanticId);
	}

	const int32 SlashIndex = FullText.Find(TEXT("/"));
	const float SlashPrefix = FVector2D(FontMeasure->Measure(
		FStringView(FullText).Left(SlashIndex),
		Font)).X;
	const float SlashWidth = FVector2D(FontMeasure->Measure(
		TEXT("/"),
		Font)).X;
	FWacomCardFaceSemanticTokenView SeparatorHit;
	TestFalse(
		TEXT("Separator is not a semantic hit"),
		WacomCardSemanticTextHitLayout::ResolveTokenAtLocalPosition(
			FullText,
			View.TypeSemanticTokens,
			Font,
			WideSize,
			ETextJustify::Center,
			FVector2D(
				LineStartX + SlashPrefix + SlashWidth * 0.5f,
				8.0f),
			SeparatorHit));

	const float FirstTokenWidth = FVector2D(FontMeasure->Measure(
		View.TypeSemanticTokens[0].DisplayText,
		Font)).X;
	const FVector2D WrappedSize(FirstTokenWidth + 1.0f, 160.0f);
	const float LineHeight = FMath::Max(
		1.0f,
		static_cast<float>(FontMeasure->GetMaxCharacterHeight(Font)));
	FWacomCardFaceSemanticTokenView WrappedHit;
	bool bFoundSecondWrappedSemantic = false;
	for (float Y = LineHeight * 0.5f;
		Y < WrappedSize.Y && !bFoundSecondWrappedSemantic;
		Y += LineHeight)
	{
		if (WacomCardSemanticTextHitLayout::ResolveTokenAtLocalPosition(
				FullText,
				View.TypeSemanticTokens,
				Font,
				WrappedSize,
				ETextJustify::Center,
				FVector2D(WrappedSize.X * 0.5f, Y),
				WrappedHit)
			&& WrappedHit.SemanticId
				== View.TypeSemanticTokens[1].SemanticId)
		{
			bFoundSecondWrappedSemantic = true;
		}
	}
	TestTrue(
		TEXT("A later wrapped line resolves the second semantic"),
		bFoundSecondWrappedSemantic);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIWorldCardInteractionPresentationMathSpec,
	"Wacom.UI.WorldCardInteraction.PresentationMath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIWorldCardInteractionPresentationMathSpec::RunTest(
	const FString& /*Parameters*/)
{
	FWacomWorldCardInteractionStyle Style;
	const FTransform Base(
		FQuat::Identity,
		FVector(100.0f, 0.0f, 0.0f),
		FVector(0.13f));
	const FTransform Hovered =
		FWacomWorldCardInteractionPresenter::
			ComputeHoverWorldTransformForTest(
				Base,
				FVector::ZeroVector,
				1.0f,
				Style);
	TestTrue(
		TEXT("Hover moves exactly eight centimeters toward camera"),
		Hovered.GetLocation().Equals(FVector(92.0f, 0.0f, 0.0f), 0.001f));
	TestTrue(
		TEXT("Hover reaches exact scale"),
		Hovered.GetScale3D().Equals(FVector(0.1378f), 0.0001f));

	const FVector2D Viewport(800.0f, 600.0f);
	const FVector2D Tooltip(300.0f, 120.0f);
	const FVector2D NearTopRight =
		FWacomWorldCardInteractionPresenter::ComputeTooltipPositionForTest(
			FVector2D(790.0f, 5.0f),
			Tooltip,
			Viewport,
			Style);
	TestTrue(
		TEXT("Tooltip flips and remains inside safe viewport"),
		NearTopRight.X >= Style.ViewportSafeMarginPixels
			&& NearTopRight.Y >= Style.ViewportSafeMarginPixels
			&& NearTopRight.X + Tooltip.X
				<= Viewport.X - Style.ViewportSafeMarginPixels
			&& NearTopRight.Y + Tooltip.Y
				<= Viewport.Y - Style.ViewportSafeMarginPixels);

	TStrongObjectPtr<UCardDefinition> Definition(
		NewObject<UCardDefinition>());
	TStrongObjectPtr<UWidgetComponent> Component(
		NewObject<UWidgetComponent>());
	TStrongObjectPtr<UWacomCardView> CardView(
		NewObject<UWacomCardView>());
	const FTransform Original(
		FRotator(0.0f, 15.0f, 0.0f),
		FVector(3.0f, 4.0f, 5.0f),
		FVector(0.13f));
	Component->SetRelativeTransform(Original);
	FWacomWorldCardInteractionItemView Item;
	Item.ItemId = FGuid::NewGuid();
	Item.Definition = Definition.Get();
	Item.WidgetComponent = Component.Get();
	Item.RootWidget = CardView.Get();
	Item.CardView = CardView.Get();
	FWacomWorldCardInteractionPresenter Presenter;
	Presenter.SyncItems({ Item });
	Component->SetRelativeTransform(FTransform(
		FRotator(10.0f, 20.0f, 30.0f),
		FVector(99.0f),
		FVector(2.0f)));
	Presenter.Reset();
	TestTrue(
		TEXT("Reset restores the exact creation transform"),
		Component->GetRelativeTransform().Equals(Original, 0.0001f));
	return true;
}

/**
 * Locks the right-click ownership contract: the presenter consumes every right
 * click it is handed, on a card and on empty space alike, and always drops
 * pending semantic hover state so no stale keyword survives.
 *
 * Pin identity transitions (same card closes, other card switches) are NOT
 * asserted here on purpose. `OpenInspect` sets `PinnedItemId` only after
 * `CreateWidget` succeeds, and CreateWidget requires a local player controller,
 * which an automation world cannot provide. Those transitions stay PIE checks.
 * See `Docs/TechDebt.md` for the coupling that makes them untestable here.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIWorldCardInteractionRightClickRoutingSpec,
	"Wacom.UI.WorldCardInteraction.RightClickRouting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIWorldCardInteractionRightClickRoutingSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomWorldCardInteractionSpec;

	UWorld* World = nullptr;
	if (GEngine)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World())
			{
				World = Context.World();
				break;
			}
		}
	}
	if (!TestNotNull(TEXT("automation world"), World))
	{
		return false;
	}
	AWacomPlayerController* PlayerController =
		World->SpawnActor<AWacomPlayerController>();
	if (!TestNotNull(TEXT("player controller"), PlayerController))
	{
		return false;
	}

	TStrongObjectPtr<UCardDefinition> Card(
		MakeSemanticCard(GetTransientPackage()));
	TStrongObjectPtr<UWidgetComponent> ComponentA(
		NewObject<UWidgetComponent>(PlayerController));
	TStrongObjectPtr<UWidgetComponent> ComponentB(
		NewObject<UWidgetComponent>(PlayerController));
	// NewObject, not CreateWidget: the automation controller is not a local
	// player controller, and SyncItems only needs valid widget references.
	TStrongObjectPtr<UWacomCardView> CardViewA(
		NewObject<UWacomCardView>(PlayerController));
	TStrongObjectPtr<UWacomCardView> CardViewB(
		NewObject<UWacomCardView>(PlayerController));
	if (!TestNotNull(TEXT("card view A"), CardViewA.Get())
		|| !TestNotNull(TEXT("card view B"), CardViewB.Get()))
	{
		PlayerController->Destroy();
		return false;
	}

	auto MakeItem = [&Card](
		UWidgetComponent* Component,
		UWacomCardView* CardView)
	{
		FWacomWorldCardInteractionItemView Item;
		Item.ItemId = FGuid::NewGuid();
		Item.Definition = Card.Get();
		Item.WidgetComponent = Component;
		Item.RootWidget = CardView;
		Item.CardView = CardView;
		return Item;
	};
	const FWacomWorldCardInteractionItemView ItemA =
		MakeItem(ComponentA.Get(), CardViewA.Get());
	const FWacomWorldCardInteractionItemView ItemB =
		MakeItem(ComponentB.Get(), CardViewB.Get());

	FWacomWorldCardInteractionPresenter Presenter;
	Presenter.SyncItems({ ItemA, ItemB });
	const FWacomWorldCardInteractionStyle Style;

	// SyncItems drops any item missing a required reference. An incomplete item
	// must never become routable state, so hovering its component resolves to
	// nothing instead of to a half-initialized card.
	{
		FWacomWorldCardInteractionPresenter Incomplete;
		FWacomWorldCardInteractionItemView NoCardView;
		NoCardView.ItemId = FGuid::NewGuid();
		NoCardView.Definition = Card.Get();
		NoCardView.WidgetComponent = ComponentA.Get();
		NoCardView.RootWidget = CardViewA.Get();
		Incomplete.SyncItems({ NoCardView });

		FWacomWorldCardPointerSample OverDroppedItem;
		OverDroppedItem.HoveredComponent = ComponentA.Get();
		OverDroppedItem.bOverHitTestVisibleWidget = true;
		TestTrue(
			TEXT("Right click over a dropped item is still claimed"),
			Incomplete.RouteRightClick(
				*PlayerController,
				OverDroppedItem,
				Style));
		TestFalse(
			TEXT("Item missing CardView never becomes pinnable"),
			Incomplete.GetPinnedItemIdForTest().IsValid());
	}

	auto SampleFor = [](UWidgetComponent* Component)
	{
		FWacomWorldCardPointerSample Sample;
		Sample.HoveredComponent = Component;
		Sample.bOverHitTestVisibleWidget = Component != nullptr;
		return Sample;
	};

	// Ownership is the part the purchase path depends on: a right click that
	// reaches the presenter is consumed rather than falling through to buy.
	// Empty space is asserted here because it never enters OpenInspect; the
	// on-card branch is covered by PIE for the CreateWidget reason above.
	TestTrue(
		TEXT("Right click on empty space is claimed by the activity"),
		Presenter.RouteRightClick(
			*PlayerController,
			SampleFor(nullptr),
			Style));
	TestFalse(
		TEXT("Empty space right click leaves nothing pinned"),
		Presenter.GetPinnedItemIdForTest().IsValid());
	TestFalse(
		TEXT("Empty space right click shows no inspect panel"),
		Presenter.IsInspectVisibleForTest());

	// Right click must always drop pending semantic hover state so a stale
	// keyword cannot survive into the next hover.
	TestEqual(
		TEXT("Right click clears pending semantic hover"),
		Presenter.GetHoveredSemanticIdForTest(),
		FName(NAME_None));
	TestFalse(
		TEXT("Right click leaves no visible tooltip"),
		Presenter.IsTooltipVisibleForTest());

	// Items whose widget component is gone must not stay routable, otherwise a
	// removed offer could still answer pointer input.
	ComponentB.Reset();
	Presenter.SyncItems({ ItemA, ItemB });
	TestTrue(
		TEXT("Right click over a collected component is still claimed"),
		Presenter.RouteRightClick(
			*PlayerController,
			SampleFor(nullptr),
			Style));

	Presenter.Reset();
	TestFalse(
		TEXT("Reset clears the pin"),
		Presenter.GetPinnedItemIdForTest().IsValid());
	TestFalse(
		TEXT("Reset hides the inspect panel"),
		Presenter.IsInspectVisibleForTest());

	PlayerController->Destroy();
	return true;
}

/**
 * Locks `FWacomWorldCardInteractionStyle::Sanitized`, which is the only guard
 * between authored values and the presenter.
 *
 * The project deliberately avoids ClampMin/ClampMax on these properties so
 * tuning stays open, so illegal values must fall back to defaults at runtime
 * while legal-but-unusual values outside the recommended range are preserved.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIWorldCardInteractionStyleSanitizeSpec,
	"Wacom.UI.WorldCardInteraction.StyleSanitize",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIWorldCardInteractionStyleSanitizeSpec::RunTest(
	const FString& /*Parameters*/)
{
	const FWacomWorldCardInteractionStyle Defaults;

	FWacomWorldCardInteractionStyle Illegal;
	Illegal.HoverForwardDistanceCm = -4.0f;
	Illegal.HoverScale = 0.0f;
	Illegal.HoverTransitionSeconds = 0.0f;
	Illegal.TooltipDelaySeconds = -1.0f;
	Illegal.ViewportSafeMarginPixels = -8.0f;
	Illegal.TooltipWidthPixels = 0.0f;
	Illegal.InspectPanelMarginPixels = -2.0f;
	const FWacomWorldCardInteractionStyle Fixed = Illegal.Sanitized();

	TestEqual(
		TEXT("Negative hover distance falls back to default"),
		Fixed.HoverForwardDistanceCm,
		Defaults.HoverForwardDistanceCm);
	TestEqual(
		TEXT("Zero hover scale falls back to default"),
		Fixed.HoverScale,
		Defaults.HoverScale);
	TestEqual(
		TEXT("Zero hover transition falls back to default"),
		Fixed.HoverTransitionSeconds,
		Defaults.HoverTransitionSeconds);
	TestEqual(
		TEXT("Negative tooltip delay falls back to default"),
		Fixed.TooltipDelaySeconds,
		Defaults.TooltipDelaySeconds);
	TestEqual(
		TEXT("Negative safe margin falls back to default"),
		Fixed.ViewportSafeMarginPixels,
		Defaults.ViewportSafeMarginPixels);
	TestEqual(
		TEXT("Zero tooltip width falls back to default"),
		Fixed.TooltipWidthPixels,
		Defaults.TooltipWidthPixels);
	TestEqual(
		TEXT("Negative inspect margin falls back to default"),
		Fixed.InspectPanelMarginPixels,
		Defaults.InspectPanelMarginPixels);

	// Zero tooltip delay is legal: it means "show immediately".
	FWacomWorldCardInteractionStyle ImmediateTooltip;
	ImmediateTooltip.TooltipDelaySeconds = 0.0f;
	TestEqual(
		TEXT("Zero tooltip delay is preserved as show-immediately"),
		ImmediateTooltip.Sanitized().TooltipDelaySeconds,
		0.0f);

	// Values outside the recommended ToolTip range are still legal tuning.
	FWacomWorldCardInteractionStyle Aggressive;
	Aggressive.HoverForwardDistanceCm = 40.0f;
	Aggressive.HoverScale = 1.9f;
	Aggressive.TooltipWidthPixels = 900.0f;
	const FWacomWorldCardInteractionStyle KeptAggressive =
		Aggressive.Sanitized();
	TestEqual(
		TEXT("Out-of-recommendation hover distance is not clamped"),
		KeptAggressive.HoverForwardDistanceCm,
		40.0f);
	TestEqual(
		TEXT("Out-of-recommendation hover scale is not clamped"),
		KeptAggressive.HoverScale,
		1.9f);
	TestEqual(
		TEXT("Out-of-recommendation tooltip width is not clamped"),
		KeptAggressive.TooltipWidthPixels,
		900.0f);

	// Sanitize must be idempotent so repeated Tick calls cannot drift.
	const FWacomWorldCardInteractionStyle Twice = Fixed.Sanitized();
	TestEqual(
		TEXT("Sanitize is idempotent for hover distance"),
		Twice.HoverForwardDistanceCm,
		Fixed.HoverForwardDistanceCm);
	TestEqual(
		TEXT("Sanitize is idempotent for tooltip width"),
		Twice.TooltipWidthPixels,
		Fixed.TooltipWidthPixels);
	return true;
}

/**
 * Locks the pinned inspect panel placement contract: the panel is fixed to the
 * viewport half opposite the target card, vertically centered, and never leaves
 * the safe margin even when the panel is larger than the viewport.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIWorldCardInteractionInspectPlacementSpec,
	"Wacom.UI.WorldCardInteraction.InspectPlacement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIWorldCardInteractionInspectPlacementSpec::RunTest(
	const FString& /*Parameters*/)
{
	FWacomWorldCardInteractionStyle Style;
	const FVector2D Viewport(1920.0f, 1080.0f);

	const FVector2D ForLeftTarget =
		FWacomWorldCardInteractionPresenter::ComputeInspectPositionForTest(
			true,
			Viewport,
			Style);
	TestEqual(
		TEXT("Left target pins the panel to the right margin"),
		ForLeftTarget.X,
		Viewport.X - Style.InspectPanelMarginPixels
			- Style.InspectPanelSizePixels.X);

	const FVector2D ForRightTarget =
		FWacomWorldCardInteractionPresenter::ComputeInspectPositionForTest(
			false,
			Viewport,
			Style);
	TestEqual(
		TEXT("Right target pins the panel to the left margin"),
		ForRightTarget.X,
		static_cast<double>(Style.InspectPanelMarginPixels));
	TestTrue(
		TEXT("The two halves are distinct placements"),
		!FMath::IsNearlyEqual(ForLeftTarget.X, ForRightTarget.X));
	TestEqual(
		TEXT("Panel is vertically centered"),
		ForLeftTarget.Y,
		(Viewport.Y - Style.InspectPanelSizePixels.Y) * 0.5f);
	TestEqual(
		TEXT("Both halves share the vertical placement"),
		ForRightTarget.Y,
		ForLeftTarget.Y);

	// A viewport smaller than the panel must still respect the margin instead
	// of producing negative offsets that would push the panel off screen.
	const FVector2D Tiny(320.0f, 240.0f);
	const FVector2D Clamped =
		FWacomWorldCardInteractionPresenter::ComputeInspectPositionForTest(
			true,
			Tiny,
			Style);
	TestEqual(
		TEXT("Undersized viewport keeps the horizontal margin"),
		Clamped.X,
		static_cast<double>(Style.InspectPanelMarginPixels));
	TestEqual(
		TEXT("Undersized viewport keeps the vertical margin"),
		Clamped.Y,
		static_cast<double>(Style.InspectPanelMarginPixels));
	return true;
}

/**
 * Locks the authored world card interaction style that `BP_WacomWorldShop`
 * serializes in its Class Defaults.
 *
 * `FWacomWorldCardInteractionStyle` is a USTRUCT persisted by that Blueprint, so
 * moving it between headers or renaming any field would silently reset authored
 * values to C++ defaults. This test fails loudly instead.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIWorldCardInteractionAuthoredStyleSpec,
	"Wacom.UI.WorldCardInteraction.AuthoredStyle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIWorldCardInteractionAuthoredStyleSpec::RunTest(
	const FString& /*Parameters*/)
{
	const UBlueprint* Blueprint = LoadObject<UBlueprint>(
		nullptr,
		TEXT("/Game/Wacom/Maps/SceneActor/BP_WacomWorldShop.BP_WacomWorldShop"));
	if (!TestNotNull(TEXT("Formal world shop Blueprint loads"), Blueprint))
	{
		return false;
	}
	const UClass* GeneratedClass = Blueprint->GeneratedClass;
	if (!TestNotNull(TEXT("Blueprint has a generated class"), GeneratedClass))
	{
		return false;
	}
	const AWacomWorldShopActor* ShopDefaults =
		GeneratedClass->GetDefaultObject<AWacomWorldShopActor>();
	if (!TestNotNull(TEXT("Blueprint CDO is a world shop actor"), ShopDefaults))
	{
		return false;
	}

	const FWacomWorldCardInteractionStyle& Style =
		ShopDefaults->WorldCardInteractionStyle;
	TestEqual(
		TEXT("Authored hover forward distance survives serialization"),
		Style.HoverForwardDistanceCm,
		8.0f);
	TestEqual(
		TEXT("Authored hover scale survives serialization"),
		Style.HoverScale,
		1.06f,
		KINDA_SMALL_NUMBER);
	TestEqual(
		TEXT("Authored hover transition survives serialization"),
		Style.HoverTransitionSeconds,
		0.12f,
		KINDA_SMALL_NUMBER);
	TestEqual(
		TEXT("Authored tooltip delay survives serialization"),
		Style.TooltipDelaySeconds,
		0.15f,
		KINDA_SMALL_NUMBER);
	TestTrue(
		TEXT("Authored tooltip mouse offset survives serialization"),
		Style.TooltipMouseOffsetPixels.Equals(
			FVector2D(16.0f, -16.0f),
			0.001f));
	TestEqual(
		TEXT("Authored viewport safe margin survives serialization"),
		Style.ViewportSafeMarginPixels,
		16.0f);
	TestEqual(
		TEXT("Authored tooltip width survives serialization"),
		Style.TooltipWidthPixels,
		300.0f);
	TestTrue(
		TEXT("Authored inspect panel size survives serialization"),
		Style.InspectPanelSizePixels.Equals(
			FVector2D(360.0f, 420.0f),
			0.001f));
	TestEqual(
		TEXT("Authored inspect panel margin survives serialization"),
		Style.InspectPanelMarginPixels,
		24.0f);
	return true;
}
