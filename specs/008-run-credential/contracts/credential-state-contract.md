# Contract: Credential 权威状态

## Public read contract

`URunSession` 提供窄只读查询：

```cpp
bool HasCredential(FName CredentialId) const;
```

- `NAME_None` 返回 false。
- 只读取 `FRunState::GrantedCredentialIds`。
- 不读取或扫描任何卡牌 zone、Pickup、Actor 或资产。

## Write ownership

- Credential 的验证、批量授予和全量要求求值由 `WacomRun/Private/Credential` 小型规则模块负责。
- Runtime public/Blueprint API 不提供任意 `GrantCredential` 或 `RevokeCredential`。
- 本轮合法写来源仅为数据驱动 Pickup 事务、初始化/重置和 SaveGame Apply。

## Invariants

- ID 非空。
- 授予幂等。
- 普通卡牌移除绝不撤销 Credential。
- Credential 生命周期不依赖表现卡 InstanceId。
- `Credential.Run.SerpentSigil` 与 `Card.Run.SerpentSigil` 是不同身份。

## Failure contract

非法 grant 输入必须在修改 working state 前拒绝。失败不推进 revision、不广播、不留下部分集合。
