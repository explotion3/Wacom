// Copyright Wacom. All Rights Reserved.

#include "Validation/WacomMapDefinitionValidation.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Interactions/RunWorldCardInteractionDefinition.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Map/WacomJourneyDefinition.h"
#include "Pickups/RunPickupDefinition.h"

#define LOCTEXT_NAMESPACE "WacomMapDefinitionValidation"

namespace
{
	void AddError(FWacomMapDefinitionValidationReport& Report, const FString& Message)
	{
		Report.Errors.Add(FText::FromString(Message));
	}

	void AddWarning(FWacomMapDefinitionValidationReport& Report, const FString& Message)
	{
		Report.Warnings.Add(FText::FromString(Message));
	}

	bool HasEncounterPayload(const FWacomMapNodeContent& Content)
	{
		return Content.Encounter.EncounterDefinition != nullptr || Content.Encounter.bBoss;
	}

	bool HasRunEventPayload(const FWacomMapNodeContent& Content)
	{
		return Content.RunEvent.RunEventDefinition != nullptr;
	}

	bool HasShopPayload(const FWacomMapNodeContent& Content)
	{
		return Content.Shop.ShopDefinition != nullptr;
	}

	bool HasTreasurePayload(const FWacomMapNodeContent& Content)
	{
		return Content.Treasure.PickupDefinition != nullptr
			|| Content.Treasure.WorldCardInteractionDefinition != nullptr;
	}

	bool HasFloorEntrancePayload(const FWacomMapNodeContent& Content)
	{
		return !Content.FloorEntrance.TargetFloorId.IsNone()
			|| !Content.FloorEntrance.OwnedCardRequirements.IsEmpty()
			|| !Content.FloorEntrance.RequiredCredentialIds.IsEmpty();
	}

	void ValidateExclusivePayload(
		const FWacomMapNodeDefinition& Node,
		const bool bExpectedPayloadPresent,
		const TArray<TPair<const TCHAR*, bool>>& Payloads,
		FWacomMapDefinitionValidationReport& Report)
	{
		if (!bExpectedPayloadPresent)
		{
			AddError(Report, FString::Printf(
				TEXT("Floor 节点 %s 的 %s payload 缺失或无效。"),
				*Node.NodeId.ToString(),
				*StaticEnum<EWacomMapNodeType>()->GetNameStringByValue(static_cast<int64>(Node.NodeType))));
		}

		for (const TPair<const TCHAR*, bool>& Payload : Payloads)
		{
			if (Payload.Value)
			{
				AddError(Report, FString::Printf(
					TEXT("Floor 节点 %s 的 NodeType 与已配置的 %s payload 不匹配。"),
					*Node.NodeId.ToString(), Payload.Key));
			}
		}
	}

	TSet<FName> BuildReachableNodes(
		const UWacomFloorMapDefinition& Floor,
		const FName StartNodeId,
		const FName BlockedNodeId = NAME_None)
	{
		TSet<FName> Reachable;
		if (StartNodeId.IsNone() || StartNodeId == BlockedNodeId || !Floor.FindNode(StartNodeId))
		{
			return Reachable;
		}

		TArray<FName> Queue;
		Queue.Add(StartNodeId);
		Reachable.Add(StartNodeId);
		for (int32 Index = 0; Index < Queue.Num(); ++Index)
		{
			for (const FWacomMapEdgeDefinition& Edge : Floor.Edges)
			{
				if (Edge.FromNodeId != Queue[Index]
					|| Edge.ToNodeId == BlockedNodeId
					|| !Floor.FindNode(Edge.ToNodeId)
					|| Reachable.Contains(Edge.ToNodeId))
				{
					continue;
				}
				Reachable.Add(Edge.ToNodeId);
				Queue.Add(Edge.ToNodeId);
			}
		}
		return Reachable;
	}

	void ValidateSuccessTerminal(
		const UWacomJourneyDefinition& Journey,
		FWacomMapDefinitionValidationReport& Report)
	{
		const FWacomMapNodeHandle& Terminal = Journey.SuccessTerminalNode;
		const bool bUnconfigured = Terminal.FloorId.IsNone() && Terminal.NodeId.IsNone();
		if (bUnconfigured)
		{
			const FString PackageName = Journey.GetOutermost()->GetName();
			if (PackageName.StartsWith(TEXT("/Game/Wacom/Data/Map/Production/")))
			{
				AddError(Report, FString::Printf(
					TEXT("Production Journey %s 必须配置 SuccessTerminalNode。"),
					*Journey.JourneyId.ToString()));
				return;
			}
			AddWarning(Report, FString::Printf(
				TEXT("Journey %s 尚未配置 SuccessTerminalNode；允许旧 Debug/Authoring 内容运行，但不会自动成功。"),
				*Journey.JourneyId.ToString()));
			return;
		}

		if (!Terminal.IsValid())
		{
			AddError(Report, TEXT("SuccessTerminalNode 必须同时配置 FloorId 和 NodeId。"));
			return;
		}

		const int32 TerminalFloorIndex = Journey.FindFloorIndex(Terminal.FloorId);
		if (TerminalFloorIndex == INDEX_NONE)
		{
			AddError(Report, FString::Printf(
				TEXT("SuccessTerminalNode 的 FloorId 不属于 Journey：%s。"),
				*Terminal.FloorId.ToString()));
			return;
		}

		const int32 LastFloorIndex = Journey.Floors.Num() - 1;
		if (TerminalFloorIndex != LastFloorIndex)
		{
			AddError(Report, TEXT("SuccessTerminalNode 必须位于 Journey 最后一层。"));
		}

		const UWacomFloorMapDefinition* TerminalFloor = Journey.Floors[TerminalFloorIndex];
		if (!TerminalFloor)
		{
			AddError(Report, TEXT("SuccessTerminalNode 所属 Floor 为空。"));
			return;
		}

		const FWacomMapNodeDefinition* TerminalNode = TerminalFloor->FindNode(Terminal.NodeId);
		if (!TerminalNode)
		{
			AddError(Report, FString::Printf(
				TEXT("SuccessTerminalNode 引用不存在的节点：%s。"),
				*Terminal.NodeId.ToString()));
			return;
		}
		if (TerminalNode->NodeType != EWacomMapNodeType::Encounter)
		{
			AddError(Report, TEXT("SuccessTerminalNode 必须是 Encounter 节点。"));
		}
		else if (!TerminalNode->Content.Encounter.bBoss)
		{
			AddError(Report, TEXT("SuccessTerminalNode 必须配置 bBoss=true。"));
		}

		if (!BuildReachableNodes(*TerminalFloor, TerminalFloor->EntryNodeId).Contains(Terminal.NodeId))
		{
			AddError(Report, TEXT("SuccessTerminalNode 必须从最后一层 Entry 可达。"));
		}
		if (TerminalFloor->Edges.ContainsByPredicate(
			[Terminal](const FWacomMapEdgeDefinition& Edge)
			{
				return Edge.FromNodeId == Terminal.NodeId;
			}))
		{
			AddError(Report, TEXT("SuccessTerminalNode 必须无出边。"));
		}

		if (TerminalFloorIndex == LastFloorIndex
			&& TerminalFloor->Nodes.ContainsByPredicate(
				[](const FWacomMapNodeDefinition& Node)
				{
					return Node.NodeType == EWacomMapNodeType::FloorEntrance;
				}))
		{
			AddError(Report, TEXT("配置成功终局后，Journey 最后一层不得包含 FloorEntrance。"));
		}
	}

	bool CardMatchesRequirement(const UCardDefinition* Card, const FWacomOwnedCardRequirement& Requirement)
	{
		if (!Card)
		{
			return false;
		}

		const bool bHasIdentityFilter = !Requirement.AllowedCardDefinitions.IsEmpty()
			|| !Requirement.AllowedCardIds.IsEmpty();
		bool bIdentityMatches = !bHasIdentityFilter;
		for (const UCardDefinition* AllowedDefinition : Requirement.AllowedCardDefinitions)
		{
			bIdentityMatches |= AllowedDefinition == Card;
		}
		bIdentityMatches |= Requirement.AllowedCardIds.Contains(Card->CardId);
		return bIdentityMatches
			&& Card->Keywords.HasAll(Requirement.RequiredKeywords)
			&& !Card->Keywords.HasAny(Requirement.BlockedKeywords);
	}

	bool CardsSatisfyRequirements(
		TConstArrayView<const UCardDefinition*> Cards,
		TConstArrayView<FWacomOwnedCardRequirement> Requirements)
	{
		for (const FWacomOwnedCardRequirement& Requirement : Requirements)
		{
			bool bMatched = false;
			for (const UCardDefinition* Card : Cards)
			{
				bMatched |= CardMatchesRequirement(Card, Requirement);
			}
			if (!bMatched)
			{
				return false;
			}
		}
		return true;
	}

	void AppendCharacterCards(const UCharacterDefinition& Character, TArray<const UCardDefinition*>& OutCards)
	{
		OutCards.AddUnique(Character.LeftHandCard);
		OutCards.AddUnique(Character.RightHandCard);
		for (const UCardDefinition* Card : Character.StarterDeck)
		{
			OutCards.AddUnique(Card);
		}
		OutCards.Remove(nullptr);
	}

	void AppendGuaranteedCardsBeforeEntrance(
		const UWacomFloorMapDefinition& Floor,
		const FName EntranceNodeId,
		TArray<const UCardDefinition*>& OutCards)
	{
		for (const FWacomMapNodeDefinition& Candidate : Floor.Nodes)
		{
			if (Candidate.NodeType != EWacomMapNodeType::Treasure
				|| Candidate.NodeId == EntranceNodeId
				|| !Candidate.Content.Treasure.PickupDefinition
				|| Candidate.Content.Treasure.PickupDefinition->RewardType != EWacomRunPickupRewardType::Card
				|| !Candidate.Content.Treasure.PickupDefinition->CardDefinition)
			{
				continue;
			}

			// 去掉 Candidate 后入口不可达，说明它支配入口，是所有路线必经的固定奖励。
			const TSet<FName> ReachableWithoutCandidate =
				BuildReachableNodes(Floor, Floor.EntryNodeId, Candidate.NodeId);
			if (!ReachableWithoutCandidate.Contains(EntranceNodeId))
			{
				OutCards.AddUnique(Candidate.Content.Treasure.PickupDefinition->CardDefinition);
			}
		}
	}

	void AppendGuaranteedCredentialsBeforeEntrance(
		const UWacomFloorMapDefinition& Floor,
		const FName EntranceNodeId,
		TSet<FName>& OutCredentialIds)
	{
		for (const FWacomMapNodeDefinition& Candidate : Floor.Nodes)
		{
			const UWacomRunPickupDefinition* Pickup =
				Candidate.Content.Treasure.PickupDefinition;
			if (Candidate.NodeType != EWacomMapNodeType::Treasure
				|| Candidate.NodeId == EntranceNodeId
				|| !Pickup
				|| !Pickup->IsRewardConfigValid()
				|| Pickup->GrantedCredentialIds.IsEmpty())
			{
				continue;
			}

			const TSet<FName> ReachableWithoutCandidate =
				BuildReachableNodes(Floor, Floor.EntryNodeId, Candidate.NodeId);
			if (!ReachableWithoutCandidate.Contains(EntranceNodeId))
			{
				for (const FName CredentialId : Pickup->GrantedCredentialIds)
				{
					OutCredentialIds.Add(CredentialId);
				}
			}
		}
	}

	void ValidateFloorLocal(const UWacomFloorMapDefinition& Floor, FWacomMapDefinitionValidationReport& Report)
	{
		if (Floor.FloorId.IsNone())
		{
			AddError(Report, TEXT("FloorId 不能为空。"));
		}
		if (Floor.DisplayName.IsEmptyOrWhitespace())
		{
			AddError(Report, TEXT("Floor DisplayName 不能为空。"));
		}

		TSet<FName> NodeIds;
		for (const FWacomMapNodeDefinition& Node : Floor.Nodes)
		{
			if (Node.DisplayName.IsEmptyOrWhitespace())
			{
				AddError(Report, FString::Printf(
					TEXT("Floor 节点 %s 的 DisplayName 不能为空。"),
					*Node.NodeId.ToString()));
			}
			if (!FMath::IsFinite(Node.MapPosition.X)
				|| !FMath::IsFinite(Node.MapPosition.Y)
				|| Node.MapPosition.X < 0.0f || Node.MapPosition.X > 1920.0f
				|| Node.MapPosition.Y < 0.0f || Node.MapPosition.Y > 1080.0f)
			{
				AddError(Report, FString::Printf(
					TEXT("Floor 节点 %s 的 MapPosition 必须位于闭区间 [0,1920] x [0,1080]。"),
					*Node.NodeId.ToString()));
			}
			if (Node.NodeId.IsNone())
			{
				AddError(Report, TEXT("NodeId 不能为空。"));
				continue;
			}
			if (NodeIds.Contains(Node.NodeId))
			{
				AddError(Report, FString::Printf(TEXT("NodeId 重复：%s。"), *Node.NodeId.ToString()));
			}
			else
			{
				NodeIds.Add(Node.NodeId);
			}

			const bool bEncounter = HasEncounterPayload(Node.Content);
			const bool bRunEvent = HasRunEventPayload(Node.Content);
			const bool bShop = HasShopPayload(Node.Content);
			const bool bTreasure = HasTreasurePayload(Node.Content);
			const bool bEntrance = HasFloorEntrancePayload(Node.Content);
			switch (Node.NodeType)
			{
			case EWacomMapNodeType::Navigation:
				ValidateExclusivePayload(Node, true,
					{{TEXT("Encounter"), bEncounter}, {TEXT("RunEvent"), bRunEvent},
					 {TEXT("Shop"), bShop}, {TEXT("Treasure"), bTreasure},
					 {TEXT("FloorEntrance"), bEntrance}}, Report);
				break;
			case EWacomMapNodeType::Encounter:
				ValidateExclusivePayload(Node, Node.Content.Encounter.EncounterDefinition != nullptr,
					{{TEXT("RunEvent"), bRunEvent}, {TEXT("Shop"), bShop}, {TEXT("Treasure"), bTreasure},
					 {TEXT("FloorEntrance"), bEntrance}}, Report);
				break;
			case EWacomMapNodeType::RunEvent:
				ValidateExclusivePayload(Node, bRunEvent,
					{{TEXT("Encounter"), bEncounter}, {TEXT("Shop"), bShop}, {TEXT("Treasure"), bTreasure},
					 {TEXT("FloorEntrance"), bEntrance}}, Report);
				break;
			case EWacomMapNodeType::Shop:
				ValidateExclusivePayload(Node, bShop,
					{{TEXT("Encounter"), bEncounter}, {TEXT("RunEvent"), bRunEvent}, {TEXT("Treasure"), bTreasure},
					 {TEXT("FloorEntrance"), bEntrance}}, Report);
				break;
			case EWacomMapNodeType::Treasure:
				ValidateExclusivePayload(Node,
					(Node.Content.Treasure.PickupDefinition != nullptr)
						!= (Node.Content.Treasure.WorldCardInteractionDefinition != nullptr),
					{{TEXT("Encounter"), bEncounter}, {TEXT("RunEvent"), bRunEvent}, {TEXT("Shop"), bShop},
					 {TEXT("FloorEntrance"), bEntrance}}, Report);
				if (Node.Content.Treasure.PickupDefinition
					&& Node.Content.Treasure.WorldCardInteractionDefinition)
				{
					AddError(Report, FString::Printf(
						TEXT("Treasure 节点 %s 必须在 Pickup 与 Card Interaction 中二选一。"),
						*Node.NodeId.ToString()));
				}
				break;
			case EWacomMapNodeType::FloorEntrance:
				ValidateExclusivePayload(Node, !Node.Content.FloorEntrance.TargetFloorId.IsNone(),
					{{TEXT("Encounter"), bEncounter}, {TEXT("RunEvent"), bRunEvent}, {TEXT("Shop"), bShop},
					 {TEXT("Treasure"), bTreasure}}, Report);
				for (int32 RequirementIndex = 0;
					RequirementIndex < Node.Content.FloorEntrance.OwnedCardRequirements.Num();
					++RequirementIndex)
				{
					if (!Node.Content.FloorEntrance.OwnedCardRequirements[RequirementIndex].HasPositiveFilter())
					{
						AddError(Report, FString::Printf(
							TEXT("FloorEntrance 节点 %s 的 Requirement[%d] 缺少有效正向筛选。"),
							*Node.NodeId.ToString(), RequirementIndex));
					}
				}
				{
					TSet<FName> UniqueCredentialIds;
					for (const FName CredentialId :
						Node.Content.FloorEntrance.RequiredCredentialIds)
					{
						if (CredentialId.IsNone())
						{
							AddError(Report, FString::Printf(
								TEXT("FloorEntrance 节点 %s 的 RequiredCredentialIds 不能包含 None。"),
								*Node.NodeId.ToString()));
						}
						else if (UniqueCredentialIds.Contains(CredentialId))
						{
							AddError(Report, FString::Printf(
								TEXT("FloorEntrance 节点 %s 的 RequiredCredentialIds 包含重复 ID：%s。"),
								*Node.NodeId.ToString(), *CredentialId.ToString()));
						}
						else
						{
							UniqueCredentialIds.Add(CredentialId);
						}
					}
				}
				break;
			default:
				AddError(Report, FString::Printf(TEXT("节点 %s 包含未知 NodeType。"), *Node.NodeId.ToString()));
				break;
			}
		}

		for (int32 LeftIndex = 0; LeftIndex < Floor.Nodes.Num(); ++LeftIndex)
		{
			for (int32 RightIndex = LeftIndex + 1; RightIndex < Floor.Nodes.Num(); ++RightIndex)
			{
				const FWacomMapNodeDefinition& Left = Floor.Nodes[LeftIndex];
				const FWacomMapNodeDefinition& Right = Floor.Nodes[RightIndex];
				if (Left.MapPosition.Equals(Right.MapPosition, 0.0f))
				{
					AddError(Report, FString::Printf(
						TEXT("Floor 节点 %s 与 %s 的 MapPosition 完全重合。"),
						*Left.NodeId.ToString(), *Right.NodeId.ToString()));
					continue;
				}
				if (FVector2D::Distance(Left.MapPosition, Right.MapPosition) < 48.0f)
				{
					AddWarning(Report, FString::Printf(
						TEXT("Floor 节点 %s 与 %s 的地图中心距离小于 48 px，可能发生视觉重叠。"),
						*Left.NodeId.ToString(), *Right.NodeId.ToString()));
				}
			}
		}

		if (Floor.EntryNodeId.IsNone() || !NodeIds.Contains(Floor.EntryNodeId))
		{
			AddError(Report, FString::Printf(TEXT("EntryNodeId 无效：%s。"), *Floor.EntryNodeId.ToString()));
		}

		TSet<FName> EdgeIds;
		for (const FWacomMapEdgeDefinition& Edge : Floor.Edges)
		{
			if (Edge.EdgeId.IsNone())
			{
				AddError(Report, TEXT("EdgeId 不能为空。"));
			}
			else if (EdgeIds.Contains(Edge.EdgeId))
			{
				AddError(Report, FString::Printf(TEXT("EdgeId 重复：%s。"), *Edge.EdgeId.ToString()));
			}
			else
			{
				EdgeIds.Add(Edge.EdgeId);
			}
			if (!NodeIds.Contains(Edge.FromNodeId) || !NodeIds.Contains(Edge.ToNodeId))
			{
				AddError(Report, FString::Printf(
					TEXT("Edge %s 的端点必须引用当前 Floor 节点：%s -> %s。"),
					*Edge.EdgeId.ToString(), *Edge.FromNodeId.ToString(), *Edge.ToNodeId.ToString()));
			}
			if (!Edge.FromNodeId.IsNone() && Edge.FromNodeId == Edge.ToNodeId)
			{
				AddError(Report, FString::Printf(TEXT("Edge %s 不允许自环。"), *Edge.EdgeId.ToString()));
			}
		}

		const TSet<FName> Reachable = BuildReachableNodes(Floor, Floor.EntryNodeId);
		for (const FWacomMapNodeDefinition& Node : Floor.Nodes)
		{
			if (Node.NodeId.IsNone() || Reachable.Contains(Node.NodeId))
			{
				continue;
			}
			const bool bMandatory = Node.NodeType == EWacomMapNodeType::FloorEntrance
				|| (Node.NodeType == EWacomMapNodeType::Encounter && Node.Content.Encounter.bBoss);
			if (bMandatory)
			{
				AddError(Report, FString::Printf(TEXT("强制/入口节点从 Entry 不可达：%s。"), *Node.NodeId.ToString()));
			}
			else
			{
				AddWarning(Report, FString::Printf(
					TEXT("节点从 Entry 不可达：%s。若它由随机或条件内容启用，请确认这是有意设计。"),
					*Node.NodeId.ToString()));
			}
		}
	}
}

FWacomMapDefinitionValidationReport FWacomMapDefinitionValidation::ValidateFloor(
	const UWacomFloorMapDefinition* FloorDefinition)
{
	FWacomMapDefinitionValidationReport Report;
	if (!FloorDefinition)
	{
		AddError(Report, TEXT("FloorMapDefinition 为空。"));
		return Report;
	}
	ValidateFloorLocal(*FloorDefinition, Report);
	return Report;
}

FWacomMapDefinitionValidationReport FWacomMapDefinitionValidation::ValidateJourney(
	const UWacomJourneyDefinition* JourneyDefinition)
{
	FWacomMapDefinitionValidationReport Report;
	if (!JourneyDefinition)
	{
		AddError(Report, TEXT("JourneyDefinition 为空。"));
		return Report;
	}

	if (JourneyDefinition->JourneyId.IsNone())
	{
		AddError(Report, TEXT("JourneyId 不能为空。"));
	}
	if (JourneyDefinition->SupportedCharacters.IsEmpty())
	{
		AddError(Report, TEXT("SupportedCharacters 至少需要一个有效角色。"));
	}
	for (int32 Index = 0; Index < JourneyDefinition->SupportedCharacters.Num(); ++Index)
	{
		if (!JourneyDefinition->SupportedCharacters[Index])
		{
			AddError(Report, FString::Printf(TEXT("SupportedCharacters[%d] 为空。"), Index));
		}
	}

	if (JourneyDefinition->PhaseBudgets.Morning < 0
		|| JourneyDefinition->PhaseBudgets.Day < 0
		|| JourneyDefinition->PhaseBudgets.Dusk < 0
		|| JourneyDefinition->PhaseBudgets.Night < 0
		|| JourneyDefinition->PhaseBudgets.Sunrise < 0)
	{
		AddError(Report, TEXT("所有时段 Action Point 预算都必须非负。"));
	}

	if (JourneyDefinition->Floors.IsEmpty())
	{
		AddError(Report, TEXT("Journey 至少需要一个有效 Floor。"));
		return Report;
	}

	TSet<FName> FloorIds;
	for (int32 FloorIndex = 0; FloorIndex < JourneyDefinition->Floors.Num(); ++FloorIndex)
	{
		const UWacomFloorMapDefinition* Floor = JourneyDefinition->Floors[FloorIndex];
		if (!Floor)
		{
			AddError(Report, FString::Printf(TEXT("Floors[%d] 为空。"), FloorIndex));
			continue;
		}
		if (!Floor->FloorId.IsNone() && FloorIds.Contains(Floor->FloorId))
		{
			AddError(Report, FString::Printf(TEXT("FloorId 重复：%s。"), *Floor->FloorId.ToString()));
		}
		else if (!Floor->FloorId.IsNone())
		{
			FloorIds.Add(Floor->FloorId);
		}
		Report.Append(ValidateFloor(Floor));
	}

	ValidateSuccessTerminal(*JourneyDefinition, Report);

	for (int32 FloorIndex = 0; FloorIndex < JourneyDefinition->Floors.Num(); ++FloorIndex)
	{
		const UWacomFloorMapDefinition* Floor = JourneyDefinition->Floors[FloorIndex];
		if (!Floor)
		{
			continue;
		}
		for (const FWacomMapNodeDefinition& Node : Floor->Nodes)
		{
			if (Node.NodeType != EWacomMapNodeType::FloorEntrance)
			{
				continue;
			}
			const FName TargetFloorId = Node.Content.FloorEntrance.TargetFloorId;
			const int32 TargetIndex = JourneyDefinition->FindFloorIndex(TargetFloorId);
			if (TargetIndex == INDEX_NONE)
			{
				AddError(Report, FString::Printf(
					TEXT("FloorEntrance %s 的 TargetFloorId 不存在：%s。"),
					*Node.NodeId.ToString(), *TargetFloorId.ToString()));
				continue;
			}
			if (TargetIndex <= FloorIndex)
			{
				AddError(Report, FString::Printf(
					TEXT("FloorEntrance %s 只能指向更后的 Floor：%s。"),
					*Node.NodeId.ToString(), *TargetFloorId.ToString()));
				continue;
			}

			const TArray<FName>& CredentialRequirements =
				Node.Content.FloorEntrance.RequiredCredentialIds;
			if (!CredentialRequirements.IsEmpty())
			{
				TSet<FName> GuaranteedCredentialIds;
				for (int32 PreviousFloorIndex = 0;
					PreviousFloorIndex <= FloorIndex;
					++PreviousFloorIndex)
				{
					const UWacomFloorMapDefinition* PreviousFloor =
						JourneyDefinition->Floors[PreviousFloorIndex];
					if (!PreviousFloor)
					{
						continue;
					}

					FName EntranceForDominance = PreviousFloor == Floor ? Node.NodeId : NAME_None;
					if (PreviousFloor != Floor)
					{
						for (const FWacomMapNodeDefinition& PreviousNode : PreviousFloor->Nodes)
						{
							if (PreviousNode.NodeType == EWacomMapNodeType::FloorEntrance
								&& JourneyDefinition->FindFloorIndex(
									PreviousNode.Content.FloorEntrance.TargetFloorId)
									== PreviousFloorIndex + 1)
							{
								EntranceForDominance = PreviousNode.NodeId;
								break;
							}
						}
					}
					if (!EntranceForDominance.IsNone())
					{
						AppendGuaranteedCredentialsBeforeEntrance(
							*PreviousFloor,
							EntranceForDominance,
							GuaranteedCredentialIds);
					}
				}

				for (const FName CredentialId : CredentialRequirements)
				{
					if (!CredentialId.IsNone()
						&& !GuaranteedCredentialIds.Contains(CredentialId))
					{
						AddError(Report, FString::Printf(
							TEXT("FloorEntrance %s 的 Credential %s 没有前置支配入口的固定 Pickup 保证来源。"),
							*Node.NodeId.ToString(), *CredentialId.ToString()));
					}
				}
			}

			const TArray<FWacomOwnedCardRequirement>& Requirements =
				Node.Content.FloorEntrance.OwnedCardRequirements;
			if (Requirements.IsEmpty())
			{
				continue;
			}

			bool bAnySupportedCharacterCanSatisfy = false;
			for (const UCharacterDefinition* Character : JourneyDefinition->SupportedCharacters)
			{
				if (!Character)
				{
					continue;
				}
				TArray<const UCardDefinition*> GuaranteedCards;
				AppendCharacterCards(*Character, GuaranteedCards);
				for (int32 PreviousFloorIndex = 0; PreviousFloorIndex <= FloorIndex; ++PreviousFloorIndex)
				{
					if (const UWacomFloorMapDefinition* PreviousFloor = JourneyDefinition->Floors[PreviousFloorIndex])
					{
						FName EntranceForDominance = PreviousFloor == Floor ? Node.NodeId : NAME_None;
						if (PreviousFloor != Floor)
						{
							for (const FWacomMapNodeDefinition& PreviousNode : PreviousFloor->Nodes)
							{
								if (PreviousNode.NodeType == EWacomMapNodeType::FloorEntrance
									&& JourneyDefinition->FindFloorIndex(
										PreviousNode.Content.FloorEntrance.TargetFloorId) == PreviousFloorIndex + 1)
								{
									EntranceForDominance = PreviousNode.NodeId;
									break;
								}
							}
						}
						if (!EntranceForDominance.IsNone())
						{
							AppendGuaranteedCardsBeforeEntrance(
								*PreviousFloor, EntranceForDominance, GuaranteedCards);
						}
					}
				}
				bAnySupportedCharacterCanSatisfy |=
					CardsSatisfyRequirements(GuaranteedCards, Requirements);
			}

			if (!bAnySupportedCharacterCanSatisfy)
			{
				AddError(Report, FString::Printf(
					TEXT("FloorEntrance %s 的持有卡牌条件无法由任何支持角色及前置保证奖励满足。"),
					*Node.NodeId.ToString()));
			}
		}
	}

	return Report;
}

FWacomMapDefinitionValidationReport FWacomMapDefinitionValidation::ValidateJourneyIds(
	const TConstArrayView<const UWacomJourneyDefinition*> JourneyDefinitions)
{
	FWacomMapDefinitionValidationReport Report;
	TMap<FName, const UWacomJourneyDefinition*> Owners;
	for (const UWacomJourneyDefinition* Journey : JourneyDefinitions)
	{
		if (!Journey || Journey->JourneyId.IsNone())
		{
			continue;
		}
		if (const UWacomJourneyDefinition* const* Existing = Owners.Find(Journey->JourneyId))
		{
			AddError(Report, FString::Printf(
				TEXT("JourneyId 重复：%s（%s 与 %s）。"),
				*Journey->JourneyId.ToString(), *GetNameSafe(*Existing), *GetNameSafe(Journey)));
		}
		else
		{
			Owners.Add(Journey->JourneyId, Journey);
		}
	}
	return Report;
}

#undef LOCTEXT_NAMESPACE
