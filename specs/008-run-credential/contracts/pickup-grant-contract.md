# Contract: 数据驱动 Pickup Credential 授予

## Input

```text
PersistentId: FName
PickupDefinition: UWacomRunPickupDefinition*
```

`AWacomRunRewardPickupActor` 只提交这两个事实；Actor 不自己授予 Credential。

## Required behavior

1. 预检 PersistentId、Definition、主奖励配置、Credential ID、重复 Pickup 和现有 Treasure/AP 条件。
2. 在一个 `FRunState` working copy 中提交主奖励与所有 Credential。
3. 同一 working copy 标记 `CollectedPickupIds` 并完成现有 Treasure 节点/AP 结算。
4. 成功后一次替换权威状态并广播一次。
5. Actor 根据 Definition 的主奖励继续显示既有 Card/Gold toast；不新增 Credential toast。

## Compatibility

- `GrantedCredentialIds=[]` 时等价于旧数据驱动 Gold/Card Pickup。
- 旧 `CollectGoldPickup` / `CollectCardPickup` API 保留给已有专用 Actor/测试，并共享同一内部事务实现或等价不带 Credential 的路径。
- Debug builder 和 Debug DataAsset 不写新字段、不重建。

## Rejection

以下任一情况必须零提交：空 PersistentId、空 Definition、无效主奖励、空/重复 Credential ID、重复 Pickup、节点/AP 结算失败。
