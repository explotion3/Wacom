# Contract: Formal Journey/Floor Stable Identities

## Frozen identities

```text
Journey.Main.01
Floor.Main.01
Floor.Main.02
Floor.Main.03
Card.Run.SerpentSigil
```

Floor 1 additionally freezes every NodeId and EdgeId listed in [data-model.md](../data-model.md). They are future persistence candidates even though this slice does not change SaveGame.

## Identity scopes

| Identity | Scope | Rule |
|---|---|---|
| JourneyId | Project | Unique, non-empty, not Authoring/Debug |
| FloorId | Journey | Unique within `Journey.Main.01` |
| NodeId | Floor | Unique within `Floor.Main.01` |
| EdgeId | Floor | Unique within `Floor.Main.01` |
| Host PersistentId | Journey runtime | Derived from FloorId and NodeId |
| CardId | Project content | `Card.Run.SerpentSigil` is reserved |

## Derived Host PersistentId

For every non-Navigation content node:

```text
PersistentId = <FloorId>.<NodeId>
```

Examples:

```text
Floor.Main.01.Node.Main.01
Floor.Main.01.Node.Route.C.01
Floor.Main.01.Node.Key.01
Floor.Main.01.Node.Guardian.01
```

The formula is deterministic and is not stored as a second design registry. A scene Host must also keep its existing `RunMapNodeBinding.NodeId/NodeType` equal to the Floor node.

## Mutable presentation

The following never identify Journey/Floor content and may be tuned without identity migration:

- DisplayName and descriptive text.
- MapPosition within validator bounds.
- World Transform, Spline control points and viewpoint staging.
- Actor label and editor folder.
- Content visuals.

Changing NodeType, edge endpoints, key gate, Journey order or a frozen ID is not presentation tuning and requires a formal identity compatibility review.

## Forbidden identity sources

- Array index.
- MapPosition or world coordinates.
- Asset filename or package path.
- Actor GUID or label.
- Authoring/Debug Journey/Floor/Node/Edge identities.
- A manually invented Host PersistentId unrelated to FloorId/NodeId.
