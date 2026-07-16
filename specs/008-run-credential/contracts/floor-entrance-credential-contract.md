# Contract: FloorEntrance Credential 条件

## Runtime evaluation

- `RequiredCredentialIds` 内所有 ID 均必须存在于 `FRunState::GrantedCredentialIds`。
- 全部 Credential 条件与全部 `OwnedCardRequirements` 采用 AND。
- 已解锁入口沿用当前 `UnlockedEntranceIds` 语义。
- Preview、Request 和 Confirm 不缓存卡牌或 Credential 事实；Confirm 重新校验最新状态。
- 成功通行不消费 Credential。

## Static validation

- RequiredCredentialIds 不得包含空或重复值。
- 每个 RequiredCredentialId 必须有固定 Pickup Definition 来源。
- 当前 Floor 内的来源必须位于支配入口的节点；可绕过分支不算保证来源。
- Production 蛇印入口只能引用 `Credential.Run.SerpentSigil`；正式资产另轮制作，本轮不写入。

## Compatibility

- 空 RequiredCredentialIds 保持旧入口行为。
- 只有 `Card.Run.SerpentSigil` 不满足蛇印 Credential。
- 现有 OwnedCardRequirements 仍是合法通用合同，不在本轮删除。
