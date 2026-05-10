# Phase 2 Temporary Decisions

本文记录第二阶段实现中的临时决定。这些决定是为了让开发推进而做的占位选择，后续需要正式化或替换。

每条记录格式：`[切片] 决定内容 → 后续处理方向`

---

## UI 相关

- **[P1] 全量刷新策略**：每次命令后从 Snapshot 重建所有 Widget 数据，不做增量 diff。
  → 后续加动画时，动画系统自己做 diff（比较前后 Snapshot），数据刷新仍然全量。

- **[P1] 不做 ViewModel 层**：Widget 直接持有 `UBattleSession*`，调用 `BuildSnapshot` 刷新。
  → 后续 UI 复杂度上升（多面板、多页签、背包 UI）时抽 `UBattleViewModel`。

- **[P1] 手牌用 HorizontalBox 线性排列**：不做扇形、不做拖拽。
  → 后续美术阶段替换为自定义 `UHandLayoutPanel`（扇形 + 悬停放大）。

- **[P1] 目标选择用"点击 EnemyPartWidget"实现**：不做拖拽到目标、不做射线检测。
  → 后续第一人称 HD-2D 表现时，目标选择可能改为"鼠标悬停 3D 部位 → 高亮 → 点击"。

- **[P1] EventToast 只显示文字**：不做图标、不做动画。
  → 后续加 Niagara 特效和音效时，EventToast 升级为"事件表现调度器"。

- **[P1] Widget Blueprint 纯色块 + 文字**：不做美术。
  → 后续美术阶段只改 WBP，C++ 不动。

## 规则相关

- **[P2] 双手区保留是虫妹专属规则**：当前硬编码"左右手都在时双手区普通卡保留"。
  → 后续多角色时，保留规则应由 `CharacterDefinition` 或 `CardDefinition` 的字段驱动，不硬编码。

- **[P3] 中毒穿透护盾**：已正式化，见 `Battle_Rules.md §15`。中毒伤害穿透护盾，直接扣生命值。

- **[P3] 中毒触发时机**：已正式化，见 `Battle_Rules.md §15`。触发点为"玩家每打出一张牌后 + 敌方部位每行动一次后"。

- **[P4] 费用转移只支持"被腾挪卡 -1，本卡 +1"**：朝光暮蝶右手区效果。
  → 后续可能有更复杂的费用转移（多点、条件触发）。到时候需要一个 `CostLedger` 或 `CostTransferEvent`。

- **[P4] ZoneHook 只支持两种 Trigger**：`OnPlay` 和 `OnPerfectReleaseHit`。
  → 后续可能有 `OnTurnStart`、`OnDiscard`、`OnEnterZone` 等。扩展时在 `PlayCardResolver` / `TurnFlow` 里加对应检查点。

- **[P5] CompanionPlayedCount 是全局计数**：不区分"哪张伙伴"。
  → 对齐 `BugGirl.md §5` 拂晓飞蛾的"三张伙伴"是战斗内全局计数。触发后清零。

- **[P5] 暮蛉 OnTwilightTriggered 需要暮气"生效"**：第一阶段暮气只记录层数不生效。
  → P5 需要先定义"暮气生效"的触发点（回合开始？部位行动前？），然后才能触发暮蛉被动。这是一个规则未决项。

## 架构相关

- **[P1] BattleHUD 直接由 BattleTestActor 创建并 AddToViewport**：不走 GameMode / HUD 类。
  → 后续正式游戏流程时，战斗 UI 由 `AGameModeBase::GetHUDClass` 或 `UGameUIManagerSubsystem` 管理。

- **[P6] Enhanced Input 只做战斗快捷键**：不做探索、菜单等其他 context。
  → 后续 Run 外层时加 `IMC_Exploration`、`IMC_Menu` 等，用 Push/Pop 切换。

---

## 如何使用本文档

- 开发时遇到"这个先怎么做"的问题，先查本文有没有已定的临时决定。
- 如果有，按临时决定走，不纠结。
- 如果没有，做一个新的临时决定，写进来。
- 后续正式化时，把对应条目删掉或标记为"已正式化，见 XXX 文档"。

## UI 实现新增

- **[P2.3] 锚点左右归属用"遍历顺序"启发式**：`FHandCardSnapshot` 没告诉 UI 某张锚点卡是左手还是右手，`UHandPanel` 用"第一个遇到的锚点进 LeftAnchorSlot"的方式。
  → 正式方案：给 `FHandCardSnapshot` 加 `EHandAnchorRole AnchorRole`（None/Left/Right），由 `BattleSnapshotBuilder` 按 `State.LeftHandInstanceId / RightHandInstanceId` 填充。第一阶段锚点不被腾挪所以暂不出错；P4 ZoneHook 之前修复。
