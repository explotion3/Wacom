# Contract: Formal Floor 2/3 Stable Identity Extension

## Frozen identities

```text
Journey.Main.01
Floor.Main.01
Floor.Main.02
Floor.Main.03

Card.Run.SerpentSigil
Credential.Run.SerpentSigil
Card.Run.MoltSeal
Credential.Run.MoltSeal
```

Floor 2/3 additionally freeze every NodeId, EdgeId and Production content ID listed in [data-model.md](../data-model.md).

## Identity scopes

| Identity | Scope | Rule |
|---|---|---|
| JourneyId | Project | Unique, non-empty, not Authoring/Debug |
| FloorId | Journey | Unique in ordered `Journey.Main.01` |
| NodeId | Floor | Unique within its Floor; structural names may repeat across Floors |
| EdgeId | Floor | Unique within its Floor; structural names may repeat across Floors |
| Map node handle | Journey runtime | Always `FloorId + NodeId` |
| Host PersistentId | Journey runtime | Derived as `<FloorId>.<NodeId>` |
| CardId | Project content | Presentation identity, never credential authority |
| CredentialId | Run | Stable non-empty FName qualification identity |
| Production content ID | Project content | Unique in its content category/registry; never a Debug mapping |

## Derived Host PersistentId

For every non-Navigation content node:

```text
PersistentId = <FloorId>.<NodeId>
```

Examples:

```text
Floor.Main.02.Node.Route.C.01
Floor.Main.02.Node.Key.01
Floor.Main.03.Node.Core.01
Floor.Main.03.Node.Guardian.01
```

No second manual Host ID registry is permitted. Scene binding must still match the Floor node's NodeId and NodeType.

## Molt Seal separation

- `Card.Run.MoltSeal` is the future visible task-card identity.
- `Credential.Run.MoltSeal` is the authoritative, persistent entrance qualification.
- The Pickup may grant both in one existing atomic definition transaction.
- Removing, paying, selling or destroying the card must never be interpreted as revoking the Credential.
- Owning the card without the Credential must never be interpreted as satisfying the entrance.

## Mutable presentation

DisplayName, description, MapPosition, world Transform, Spline, Actor label/GUID and visuals remain mutable and are never identity sources.

Changing Floor order, a frozen ID, NodeType, edge endpoint, Molt Seal gate or terminal Guardian role requires a formal compatibility review.
