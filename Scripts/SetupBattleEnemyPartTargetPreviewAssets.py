"""Create the enemy-part target-preview MI/Style and bind the Debug Snake host.

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
DEBUG_SNAKE_HOST_PATH = "/Game/Wacom/Core/Enemy/BP_SnakeHost_Debug"


def load_required(asset_path, label):
    asset = unreal.load_asset(asset_path)
    if not asset:
        raise RuntimeError(f"{label} is missing: {asset_path}")
    return asset


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
    }
    vector_values = {
        "ImpactPrimaryColor": unreal.LinearColor(0.58, 0.80, 1.0, 1.0),
        "ImpactSecondaryColor": unreal.LinearColor(0.96, 0.82, 0.42, 1.0),
        "ImpactAccentColor": unreal.LinearColor(0.88, 0.30, 0.72, 1.0),
        "ImpactInvalidTargetColor": unreal.LinearColor(0.68, 0.12, 0.30, 1.0),
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


def assign_debug_snake_default(style):
    host_blueprint = load_required(DEBUG_SNAKE_HOST_PATH, "Debug Snake Host")
    host_cdo = unreal.get_default_object(host_blueprint.generated_class())
    host_cdo.set_editor_property("default_target_preview_style", style)
    unreal.EditorAssetLibrary.save_loaded_asset(host_blueprint)


base_material = load_required(BASE_MATERIAL_PATH, "Enemy impact DreamShader material")
system = load_required(NIAGARA_SYSTEM_PATH, "Enemy impact Niagara System")
preview_instance = create_preview_material_instance(base_material)
preview_style = create_preview_style(system, preview_instance)
assign_debug_snake_default(preview_style)
unreal.log("Wacom enemy-part pixel target-preview MI, Style and Debug Snake binding configured")
