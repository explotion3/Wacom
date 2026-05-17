# 目前有如下问题

---

## VSCode

- Template 必须是通用模板

- 核心文件过于臃肿 急需模块化

## Plugins

### Graph Function 问题

- GraphFunction 核心还是HLSL语法 只不过是可以用UE.Expression的输入 我的设想就是多加几个输入引脚 把这点节点的值输入进来 如果是这个节点的参数的话 则继续走Graph路线 但是Function核心还是HLSL

- GraphFunction NS(Namespace) 函数无法正常调用

### Material Layer / Material Layer Blend 问题

- MaterialLayer 并不是正确的 UMaterialLayer 而是还是原来的 UMaterialFunction

- MaterialLayer 需要改名为 Shader Layer / MaterialLayerBlend 需要改名为 Shader Layer Blend

- Function < GraphFunction < Shader Function = Shader Layer = Shader Layer Blend < Shader

### 生成Material问题

- Shader / Shader Function 节点杂乱问题

- Shader / Shader Function 多余节点问题


