"""Create/update Diamond Wave and Pixel Edge Flip card-use presets.

Run after DreamShader has generated M_FirstPersonCard_SurfaceEffects_DiamondWaveUse.
The existing OrderedDither Style remains the active Exhausted dissolve preset.
"""

import unreal


MATERIAL_DIR = "/Game/DreamMaterials/Card"
DIAMOND_BASE_MATERIAL_PATH = MATERIAL_DIR + "/M_FirstPersonCard_SurfaceEffects_DiamondWaveUse"
DIAMOND_MATERIAL_INSTANCE_PATH = MATERIAL_DIR + "/MI_FirstPersonCard_SurfaceEffects_DiamondWaveUse_Default"
EDGE_BASE_MATERIAL_PATH = MATERIAL_DIR + "/M_FirstPersonCard_SurfaceEffects_EdgeFlipUse"
EDGE_MATERIAL_INSTANCE_PATH = MATERIAL_DIR + "/MI_FirstPersonCard_SurfaceEffects_EdgeFlipUse_Default"
STYLE_DIR = "/Game/Wacom/UI/Card/SurfaceEffects"
DIAMOND_STYLE_PATH = STYLE_DIR + "/DA_FPCardUseEffect_DiamondWave"
EDGE_STYLE_PATH = STYLE_DIR + "/DA_FPCardUseEffect_EdgeFlip"
ORDERED_DITHER_STYLE_PATH = STYLE_DIR + "/DA_FPCardPlayedDissolveStyle_OrderedDither"
PLAYER_BP_PATH = "/Game/Wacom/Core/Player/BP_WacomPlayerCharacter"


def load_required(asset_path, label):
    asset = unreal.load_asset(asset_path)
    if not asset:
        raise RuntimeError(f"{label} is missing: {asset_path}")
    return asset


def load_or_create_material_instance(parent, asset_path, asset_name, scalar_values, vector_values):
    material_instance = unreal.load_asset(asset_path)
    if not material_instance:
        factory = unreal.MaterialInstanceConstantFactoryNew()
        material_instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name,
            MATERIAL_DIR,
            unreal.MaterialInstanceConstant,
            factory,
        )
    if not material_instance:
        raise RuntimeError("Failed to create Diamond Wave material instance")

    unreal.MaterialEditingLibrary.set_material_instance_parent(material_instance, parent)
    for parameter_name, value in scalar_values.items():
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
            material_instance,
            parameter_name,
            value,
        )
    for parameter_name, value in vector_values.items():
        unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
            material_instance, parameter_name, value)
    unreal.EditorAssetLibrary.save_loaded_asset(material_instance)
    return material_instance


def load_or_create_style(asset_path, asset_name, material_instance, effect_kind):
    style_asset = unreal.load_asset(asset_path)
    if not style_asset:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property(
            "data_asset_class",
            unreal.WacomFirstPersonCardUseEffectStyle,
        )
        style_asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name,
            STYLE_DIR,
            unreal.WacomFirstPersonCardUseEffectStyle,
            factory,
        )
    if not style_asset:
        raise RuntimeError("Failed to create Diamond Wave card-use Style")

    style = style_asset.get_editor_property("style")
    style.set_editor_property("effect_kind", effect_kind)
    style.set_editor_property("surface_effect_material_instance", material_instance)
    if effect_kind == unreal.WacomFirstPersonCardUseEffectKind.EDGE_FLIP:
        style.set_editor_property("duration_seconds", 0.28)
        style.set_editor_property("confirm_hold_seconds", 0.06)
        style.set_editor_property("edge_flip_impact_seconds", 0.05)
        style.set_editor_property("edge_flip_lift_pixels", 12.0)
        style.set_editor_property("edge_flip_scale_multiplier", 1.04)
        style.set_editor_property("edge_flip_minimum_horizontal_scale", 0.06)
        style.set_editor_property("edge_flip_reform_out_seconds", 0.22)
        style.set_editor_property("edge_flip_reform_hidden_hold_seconds", 0.06)
        style.set_editor_property("edge_flip_reform_in_seconds", 0.18)
        style.set_editor_property("edge_flip_reform_settle_seconds", 0.04)
    else:
        style.set_editor_property("duration_seconds", 0.36)
        style.set_editor_property("confirm_hold_seconds", 0.04)
    style.set_editor_property("reform_dissolve_out_seconds", 0.28)
    style.set_editor_property("reform_hidden_hold_seconds", 0.08)
    style.set_editor_property("reform_build_in_seconds", 0.24)
    style_asset.set_editor_property("style", style)
    unreal.EditorAssetLibrary.save_loaded_asset(style_asset)
    return style_asset


def assign_styles_to_player(card_use_style):
    player_bp = load_required(PLAYER_BP_PATH, "BP_WacomPlayerCharacter")
    player_cdo = unreal.get_default_object(player_bp.generated_class())
    anchor = player_cdo.get_editor_property("first_person_card_anchor_component")
    if not anchor:
        raise RuntimeError("FirstPersonCardAnchorComponent is missing on player CDO")

    anchor.set_editor_property("enable_card_use_effect", True)
    anchor.set_editor_property("card_use_effect_style", card_use_style)
    ordered_dither_style = unreal.load_asset(ORDERED_DITHER_STYLE_PATH)
    if ordered_dither_style:
        anchor.set_editor_property("enable_card_played_dissolve", True)
        anchor.set_editor_property("card_played_dissolve_style", ordered_dither_style)
    else:
        unreal.log_warning(
            "OrderedDither Style is missing; Exhausted cards will use spatial fallback"
        )
    unreal.EditorAssetLibrary.save_loaded_asset(player_bp)


diamond_material = load_required(DIAMOND_BASE_MATERIAL_PATH, "Diamond Wave DreamShader material")
diamond_instance = load_or_create_material_instance(
    diamond_material,
    DIAMOND_MATERIAL_INSTANCE_PATH,
    "MI_FirstPersonCard_SurfaceEffects_DiamondWaveUse_Default",
    {"UseGridColumns": 10.0, "UseRotationDegrees": 45.0, "UseWaveBandCells": 1.5,
     "UseOutlineWidth": 0.12, "UseOutlineSoftness": 0.025,
     "UseGlowIntensity": 1.35, "UseShadowFadeSeconds": 0.10},
    {"UsePrimaryColor": unreal.LinearColor(0.20, 0.56, 1.0, 1.0),
     "UseAccentColor": unreal.LinearColor(1.0, 1.0, 1.0, 1.0)})
diamond_style = load_or_create_style(
    DIAMOND_STYLE_PATH, "DA_FPCardUseEffect_DiamondWave", diamond_instance,
    unreal.WacomFirstPersonCardUseEffectKind.DIAMOND_WAVE)

edge_material = load_required(EDGE_BASE_MATERIAL_PATH, "Pixel Edge Flip DreamShader material")
edge_instance = load_or_create_material_instance(
    edge_material,
    EDGE_MATERIAL_INSTANCE_PATH,
    "MI_FirstPersonCard_SurfaceEffects_EdgeFlipUse_Default",
    {"UsePixelBlockSize": 3.0, "UseImpactIntensity": 0.90,
     "UseEdgeGlowIntensity": 1.35, "UseAfterimageOpacity": 0.20,
     "UseAfterimageOffsetPixels": 7.0, "UseShadowFadeSeconds": 0.10},
    {"UseImpactPrimaryColor": unreal.LinearColor(1.0, 0.86, 0.56, 1.0),
     "UseImpactAccentColor": unreal.LinearColor(0.34, 0.63, 1.0, 1.0)})
edge_style = load_or_create_style(
    EDGE_STYLE_PATH, "DA_FPCardUseEffect_EdgeFlip", edge_instance,
    unreal.WacomFirstPersonCardUseEffectKind.EDGE_FLIP)

assign_styles_to_player(edge_style)
unreal.log("Wacom Diamond Wave fallback and Pixel Edge Flip default configured")
