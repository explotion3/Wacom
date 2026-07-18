"""Create/update only the Battle player-vitals default material instance.

Run after compiling DShader/Material/UI/M_WacomBattle_PlayerVitals.dsm. This
script intentionally does not save BP_BattleHUD or WBP_PlayerStatusBar; the
WacomBuildPlayerStatusUI commandlet owns those two layout assets.
"""

from __future__ import annotations

import unreal


MATERIAL_DIR = "/Game/DreamMaterials/UI"
BASE_MATERIAL_PATH = MATERIAL_DIR + "/M_WacomBattle_PlayerVitals"
MATERIAL_INSTANCE_PATH = MATERIAL_DIR + "/MI_WacomBattle_PlayerVitals_Default"


def load_required(asset_path: str, label: str):
    asset = unreal.load_asset(asset_path)
    if not asset:
        raise RuntimeError(f"{label} is missing: {asset_path}")
    return asset


def load_or_create_material_instance(parent):
    instance = unreal.load_asset(MATERIAL_INSTANCE_PATH)
    if not instance:
        instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "MI_WacomBattle_PlayerVitals_Default",
            MATERIAL_DIR,
            unreal.MaterialInstanceConstant,
            unreal.MaterialInstanceConstantFactoryNew(),
        )
    if not instance:
        raise RuntimeError("Failed to create Battle player-vitals material instance")

    unreal.MaterialEditingLibrary.set_material_instance_parent(instance, parent)
    scalar_values = {
        "PixelColumns": 340.0,
        "PixelRows": 23.0,
    }
    vector_values = {
        "BackColor": unreal.LinearColor(0.025, 0.040, 0.070, 0.96),
        "FrameColor": unreal.LinearColor(0.14, 0.22, 0.32, 1.0),
        "HpFillColor": unreal.LinearColor(0.24, 0.84, 0.43, 1.0),
        "LowHpColor": unreal.LinearColor(0.94, 0.15, 0.09, 1.0),
        "DamageTrailColor": unreal.LinearColor(0.95, 0.31, 0.18, 1.0),
        "PreviewGainColor": unreal.LinearColor(0.42, 0.94, 0.68, 1.0),
        "PreviewGainAccentColor": unreal.LinearColor(0.96, 0.82, 0.42, 1.0),
        "PreviewLossColor": unreal.LinearColor(0.86, 0.22, 0.48, 1.0),
        "ShieldColor": unreal.LinearColor(0.38, 0.78, 1.0, 1.0),
        "ShieldGainColor": unreal.LinearColor(0.96, 0.82, 0.42, 1.0),
        "ShieldLossColor": unreal.LinearColor(0.90, 0.32, 0.62, 1.0),
    }
    for name, value in scalar_values.items():
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
            instance, name, value
        )
    for name, value in vector_values.items():
        unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
            instance, name, value
        )
    unreal.EditorAssetLibrary.save_loaded_asset(instance)
    return instance


base_material = load_required(BASE_MATERIAL_PATH, "Battle player-vitals DreamShader material")
default_instance = load_or_create_material_instance(base_material)
unreal.log(f"Wacom Battle player-vitals MI configured: {default_instance.get_path_name()}")
