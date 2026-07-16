# Research: 背包真实卡牌牌堆与即时携带

## Decision 1: 缩略牌链路整体退役

**Decision**: 折叠与展开牌堆均使用完整 `UWacomDeckCardWidget`；删除 Preview ViewData、`PilePreviewWidgetClass`、`UWacomBackpackPilePreviewWidget` 与 Builder/测试合同。

**Rationale**: 现有 Preview 最多显示三张、使用独立简化 C++ 树，无法满足玩家直接辨认全部内容的目标；继续维护两套卡面也会放大身份、排序和样式漂移。

**Alternatives considered**: 保留三张封面、用完整卡面截图或 RenderTarget 缩略。它们仍隐藏内容或增加缓存/同步复杂度，均不采用。

## Decision 2: Scene 始终物化所有牌堆卡牌

**Decision**: Reconciler 为通量和每个牌堆都生成完整卡牌视图，折叠/展开只改变布局、输入和渲染模式，不销毁重建视图。

**Rationale**: 复用同一 Widget 可避免展开瞬间生成大量 Retainer，保证折叠到展开的身份与视觉连续性，也便于 CarryLayer 重挂载。

**Alternatives considered**: 仅展开时创建，或把卡牌变成 ZonePile 子 Widget。前者带来展开峰值和状态抖动，后者把输入/框选所有权拆回多个局部树。

## Decision 3: ViewKey 保留角色维度

**Decision**: 使用 `InstanceId + OwnerInstanceId + PhysicalZone + ReuseRole` 区分实体、备战投影、特殊主卡和特殊内容视图。

**Rationale**: 同一实体可能同时拥有物理视图与只读投影视图；仅按 InstanceId reconcile 会错误复用或移除。

**Alternatives considered**: 为投影创建虚假 InstanceId。该方案污染领域身份且破坏现有 transaction 语义。

## Decision 4: 固定卡面与自适应露出

**Decision**: 折叠卡无旋转、默认露出 16px（10–24px）；展开卡露出 32–72px 并轻微扇转。两者都保持 `220×320` 与 `0.75`，不滚动、不连续缩放。

**Rationale**: 玩家已明确优先保持卡面比例和像素清晰度。通过露出与方向适配数量，避免 Retainer 被非整数动态缩放造成锯齿。

**Alternatives considered**: 滚动条、分页、动态缩卡。它们分别增加额外操作、隐藏全貌或降低清晰度。

## Decision 5: 正式牌堆 WBP 不拥有卡牌

**Decision**: `WBP_BackpackZonePile` 只绘制框体、标题、状态、拖柄和投放反馈；完整卡牌仍由 Workspace 的 StaticCardLayer 统一持有。

**Rationale**: Workspace 已是鼠标捕获、框选和携带的唯一 owner。将卡牌塞进牌堆子树会重新引入坐标转换、跨父层框选和 DragDrop owner 分裂。

**Alternatives considered**: 每个牌堆使用 Overlay/Canvas 自己管理卡牌。该方案与既有单 owner 方向冲突。

## Decision 6: 卡面复用 FPCardView，而非 Battle Slot

**Decision**: DeckCard 承载 `WBP_FPCardView`，复用卡面、Fake3D、视差、阴影和表面材质；Backpack 私有控制器只驱动表现参数。

**Rationale**: `UWacomFirstPersonCardViewWidget` 是被动卡面 View，适合作为共享视觉能力；Battle Slot、Anchor 和 transition hint 则包含战斗交互语义，不应进入背包。

**Alternatives considered**: 继续使用 `WBP_BackpackCardView` 或复制 Battle Slot。前者缺少目标效果，后者造成状态机和输入耦合。

## Decision 7: 单活动动态 Retainer

**Decision**: 静置卡采用内容变化时重绘；只有展开 Hover 卡或当前携带最前卡启用实时效果，最大活动数为 1。

**Rationale**: 牌堆可能同时显示 20 多张完整卡。所有 Retainer 持续更新会把视觉改造变成新的性能瓶颈。

**Alternatives considered**: 所有展开卡动态、折叠卡使用统一材质。前者成本随数量线性增长，后者无法提供局部 Fake3D。

## Decision 8: CarryLayer 单锚点取代插值与全量刷新

**Decision**: 携带开始时把现有卡牌重挂载到 CarryLayer 并计算一次局部扇形；鼠标移动只更新 CarryLayer 锚点。删除指数平滑和每帧 `RefreshInteractionPresentation()` 全列表遍历。

**Rationale**: 当前路径同时存在有意的 `PointerFollowSeconds` 延迟与 O(全部卡牌) 的逐帧状态重刷，是多卡跟随延迟的直接根因。

**Alternatives considered**: 仅把平滑时间改为 0、继续逐卡更新绝对坐标。它只能掩盖一部分问题，卡牌数量增加时仍会退化。

## Decision 9: 跨层 Reconciler 维护唯一 Widget

**Decision**: Reconciler 同时识别 StaticCardLayer 和 CarryLayer 的已有 Widget，携带中的卡不会因 Snapshot 刷新被复制或强制移回。

**Rationale**: CarryLayer 是视觉父层变化，不是卡牌身份变化。Reconcile 必须把父层与身份解耦。

**Alternatives considered**: 携带时暂停所有 Snapshot。该方案会让 revision 漂移和规则拒绝无法及时反映。

## Decision 10: 自动化锁定性能结构，PIE判断最终手感

**Decision**: 自动化断言锚点、误差、扇形重算次数、静态卡更新次数和动态 Retainer 上限；PIE/Insights 对比 1 与 21 张卡的实际帧时间和主观跟手。

**Rationale**: 结构性退化可稳定自动化，而最终“是否顺手”和 GPU/Slate 实际成本需要运行时观察。

**Alternatives considered**: 只用秒表或只看帧率。它们无法定位回归来源，也不适合稳定 CI。
