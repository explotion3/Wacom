"""Create or update only the enemy-part target-preview MI and Style.

This script intentionally does not rewrite the existing impact Style or its manually tuned
TargetConfirmed/Damage values. The preview uses the TargetPreview emitter generated inside
NS_WacomBattleEnemyPartImpact_Pixel by the WacomEditor Niagara builder.
"""

import unreal


MATERIAL_DIR = "/Game/DreamMaterials/World"
BASE_MATERIAL_PATH = MATERIAL_DIR + "/M_WacomBattleEnemyPartImpactPixel"
PREVIEW_MI_PATH = MATERIAL_DIR + "/MI_WacomBattleEnemyPartTargetPreviewPixel_Default"
NIAGARA_SYSTEM_PATH = "/Game/Wacom/VFX/Battle/NS_WacomBattleEnemyPartImpact_Pixel"
STYLE_DIR = "/Game/Wacom/UI/Battle/WorldImpact"
STYLE_PATH = STYLE_DIR + "/DA_BattleEnemyPartTargetPreviewStyle_PixelLock"


def load_required(asset_path, label):
    asset = unreal.load_asset(asset_path)
    if not asset:
        raise RuntimeError(f"{label} is missing: {asset_path}")
    return asset


def validate_niagara_sprite_usage(material):
    if not material.get_editor_property("used_with_niagara_sprites"):
        raise RuntimeError(
            "Enemy target-preview parent material lacks Used With Niagara Sprites. "
            "Regenerate M_WacomBattleEnemyPartImpactPixel from its .dsm and run "
            "SetupBattleEnemyPartImpactAssets.py before configuring the preview."
        )


def create_preview_material_instance(parent):
    material_instance = unreal.load_asset(PREVIEW_MI_PATH)
    if not material_instance:
        material_instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "MI_WacomBattleEnemyPartTargetPreviewPixel_Default",
            MATERIAL_DIR,
            unreal.MaterialInstanceConstant,
            unreal.MaterialInstanceConstantFactoryNew(),
        )
    if not material_instance:
        raise RuntimeError("Failed to create enemy target-preview material instance")

    unreal.MaterialEditingLibrary.set_material_instance_parent(material_instance, parent)
    scalar_values = {
        "ImpactPrimaryWeight": 0.62,
        "ImpactSecondaryWeight": 0.30,
        "ImpactPixelColumns": 18.0,
        "ImpactOutlineWidth": 0.045,
        "ImpactCoreBrightness": 1.25,
        "ImpactDecorativeGlow": 0.75,
        "ImpactPreviewGlow": 0.28,
        "ImpactAvailabilityTargetOpacity": 1.0,
    }
    vector_values = {
        "ImpactPrimaryColor": unreal.LinearColor(0.58, 0.80, 1.0, 1.0),
        "ImpactSecondaryColor": unreal.LinearColor(0.96, 0.82, 0.42, 1.0),
        "ImpactAccentColor": unreal.LinearColor(0.88, 0.30, 0.72, 1.0),
        "ImpactInvalidTargetColor": unreal.LinearColor(0.68, 0.12, 0.30, 1.0),
        "ImpactAvailabilityTargetColor": unreal.LinearColor(0.58, 0.80, 1.0, 1.0),
        "ImpactAvailabilityAccentColor": unreal.LinearColor(0.96, 0.82, 0.42, 1.0),
    }
    for name, value in scalar_values.items():
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
            material_instance, name, value)
    for name, value in vector_values.items():
        unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
            material_instance, name, value)
    unreal.EditorAssetLibrary.save_loaded_asset(material_instance)
    return material_instance


def create_preview_style(system, material_instance):
    style_asset = unreal.load_asset(STYLE_PATH)
    if not style_asset:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property(
            "data_asset_class", unreal.WacomBattleEnemyPartTargetPreviewStyle)
        style_asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "DA_BattleEnemyPartTargetPreviewStyle_PixelLock",
            STYLE_DIR,
            unreal.WacomBattleEnemyPartTargetPreviewStyle,
            factory,
        )
    if not style_asset:
        raise RuntimeError("Failed to create enemy target-preview Style")

    values = {
        "preview_system": system,
        "preview_material_instance": material_instance,
        "enter_seconds": 0.18,
        "exit_seconds": 0.10,
        "pulse_period_seconds": 0.95,
        "availability_enter_seconds": 0.12,
        "availability_exit_seconds": 0.10,
        "availability_icon_size_multiplier": 0.22,
        "minimum_availability_icon_size_centimeters": 12.0,
        "maximum_availability_icon_size_centimeters": 28.0,
        "availability_base_intensity": 0.28,
        "valid_coverage_multiplier": 1.10,
        "invalid_coverage_multiplier": 1.08,
        "fallback_size_centimeters": unreal.Vector2D(96.0, 96.0),
        "minimum_axis_size_centimeters": 56.0,
        "maximum_axis_size_centimeters": 300.0,
        "camera_depth_offset_centimeters": 2.0,
    }
    for name, value in values.items():
        style_asset.set_editor_property(name, value)
    unreal.EditorAssetLibrary.save_loaded_asset(style_asset)
    return style_asset


base_material = load_required(BASE_MATERIAL_PATH, "Enemy impact DreamShader material")
validate_niagara_sprite_usage(base_material)
system = load_required(NIAGARA_SYSTEM_PATH, "Enemy impact Niagara System")
preview_instance = create_preview_material_instance(base_material)
create_preview_style(system, preview_instance)
unreal.log("Wacom enemy-part pixel target-preview MI and Style configured")
