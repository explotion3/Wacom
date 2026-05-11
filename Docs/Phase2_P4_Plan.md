# P4 Plan: Enhanced Input 迁移

## 1. 现状

- `WacomApp.Build.cs` 已依赖 `EnhancedInput`。
- `DefaultInput.ini` 已配置 `DefaultPlayerInputClass=/Script/EnhancedInput.EnhancedPlayerInput` + `DefaultInputComponentClass=/Script/EnhancedInput.EnhancedInputComponent`。
- 唯一的输入消费者是 `ABattleTestActor::BindDebugInput()`，用 `FInputKeyBinding` + lambda 绑定 9 个键（1-5 / W / E / R / P）。
- 没有任何 `UInputAction` / `UInputMappingContext` 资产或 C++ 引用。

## 2. 目标

把 `BattleTestActor` 的输入从"裸 FInputKeyBinding"迁移到 Enhanced Input 的标准流程：

```
UInputAction (IA_*) → UInputMappingContext (IMC_Battle) → UEnhancedInputComponent::BindAction
```

好处：
- 后续加探索、菜单等 context 时可以 Push/Pop IMC，不互相干扰。
- 支持 CommonUI 的 Input Routing（CommonUI 在 UE 5.7 依赖 Enhanced Input）。
- 为手柄支持铺路。

## 3. 切片划分

| 切片 | 内容 |
| --- | --- |
| P4.1 | C++ 定义 InputAction 指针 + IMC 指针（`ABattleTestActor` 的 UPROPERTY） |
| P4.2 | 创建 DataAsset：9 个 `IA_*.uasset` + 1 个 `IMC_Battle.uasset`（编辑器手工或 Commandlet） |
| P4.3 | `ABattleTestActor::BeginPlay` 注册 IMC + `SetupPlayerInputComponent` 绑定 IA |
| P4.4 | 删除旧 `BindDebugInput` + `AutoReceiveInput` 改为 Disabled（由 PlayerController 的 Pawn 接管） |
| P4.5 | PIE 验证 + 编译 + 自动化测试全绿 |

## 4. 详细设计

### 4.1 InputAction 列表

| 资产名 | 值类型 | 默认键 | 对应操作 |
| --- | --- | --- | --- |
| `IA_PlayCard1` | Digital (bool) | `1` | PlayHandIndex(1) |
| `IA_PlayCard2` | Digital | `2` | PlayHandIndex(2) |
| `IA_PlayCard3` | Digital | `3` | PlayHandIndex(3) |
| `IA_PlayCard4` | Digital | `4` | PlayHandIndex(4) |
| `IA_PlayCard5` | Digital | `5` | PlayHandIndex(5) |
| `IA_PlayCard6` | Digital | `6` | PlayHandIndex(6) |
| `IA_PlayCard7` | Digital | `7` | PlayHandIndex(7) |
| `IA_Wait` | Digital | `W` | Wait() |
| `IA_EndTurn` | Digital | `E` | EndTurn() |
| `IA_Restart` | Digital | `R` | StartBattle() |
| `IA_RefreshHUD` | Digital | `P` | RefreshHUD() |

### 4.2 InputMappingContext

`IMC_Battle`：包含上述 11 个 IA 的默认键映射。Priority = 0（默认）。

资产通过 `WacomCreateInputAssets` Commandlet 生成到 `Content/Wacom/Input/`。

### 4.3 C++ 改动

```cpp
// BattleTestActor.h 新增
UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
TObjectPtr<UInputMappingContext> BattleMappingContext;

UPROPERTY(EditDefaultsOnly, Category = "Wacom|Input")
TObjectPtr<UInputAction> IA_PlayCard1;
// ... IA_PlayCard2..5, IA_Wait, IA_EndTurn, IA_Restart, IA_RefreshHUD
```

```cpp
// BattleTestActor.cpp
void ABattleTestActor::BeginPlay()
{
    Super::BeginPlay();
    // 注册 IMC
    if (APlayerController* PC = Cast<APlayerController>(GetWorld()->GetFirstPlayerController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(BattleMappingContext, 0);
        }
    }
    EnsurePrimaryLayout();
    if (bAutoStart) { StartBattle(); }
}

void ABattleTestActor::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (!EIC) { return; }

    EIC->BindAction(IA_PlayCard1, ETriggerEvent::Started, this, &ABattleTestActor::OnPlayCard1);
    // ... 其余 8 个
}
```

### 4.4 资产创建方式

**方案 A**：编辑器手工创建 11 个 IA + 1 个 IMC，放在 `Content/Wacom/Input/`。
**方案 B**：写一个 Commandlet `WacomCreateInputAssets` 自动生成。

采用 **方案 B**：Commandlet 生成 11 个 IA + 1 个 IMC。用户在编辑器中确认/微调。

### 4.5 AutoReceiveInput 处理

当前 `ABattleTestActor` 设置 `AutoReceiveInput = Player0`，这让 Actor 自动获得 InputComponent。迁移后改为让 Actor 被 PlayerController Possess（或继续用 `AutoReceiveInput`，因为 Enhanced Input 也支持 Actor 级别的 InputComponent）。

**临时决定**：保持 `AutoReceiveInput = Player0`。Enhanced Input 在 Actor 的 InputComponent 上也能正常工作（`UEnhancedInputComponent` 是 `UInputComponent` 的子类，`DefaultInputComponentClass` 已配置）。

### 4.6 删除旧代码

- 删除 `BindDebugInput()` 方法声明和实现。
- 删除 `#include "InputCoreTypes.h"`（如果不再需要）。
- 保留 `#include "Components/InputComponent.h"`（`SetupPlayerInputComponent` 需要）。

## 5. 验收标准

- 编译通过。
- PIE 中 1-5 / W / E / R / P 功能不变。
- 26 条自动化测试全绿（输入迁移不影响战斗逻辑）。
- `Content/Wacom/Input/` 下有 9 个 IA + 1 个 IMC。

## 6. 风险

- `AutoReceiveInput` + Enhanced Input 在 UE 5.7 的兼容性：已确认 `DefaultInputComponentClass` 设为 `EnhancedInputComponent` 后，Actor 的 InputComponent 就是 Enhanced 版本，`BindAction` 可用。
- CommonUI 的 Input Routing 可能拦截某些键：当前 HUD 返回 `ECommonInputMode::All`，键盘输入仍传递给 Game，不会被 UI 吞掉。

## 7. 临时决定

- P4 只做战斗快捷键，不做探索、菜单等其他 context。
  → 后续 Run 外层时加 `IMC_Exploration`、`IMC_Menu` 等，用 Push/Pop 切换。
