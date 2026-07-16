"""Import and configure only the Battle Draw Reveal texture, MI, Style and Anchor fields.

Use WACOM_DRAW_REVEAL_IMPORT_ONLY=1 before DreamShader generation when the
CardBackTexture default asset does not yet exist. The full pass intentionally
does not rebuild or save any other DreamShader/card-presentation asset.
"""

from __future__ import annotations

import os
from pathlib import Path

import unreal


PROJECT_DIR = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
TEXTURE_SOURCE = PROJECT_DIR / "DShader" / "Texture" / "Card" / "T_FirstPersonCard_DrawBack_Temporary.png"
SURFACE_DIR = "/Game/Wacom/UI/Card/SurfaceEffects"
TEXTURE_PATH = SURFACE_DIR + "/T_FPCardDrawBack_Temporary"
MATERIAL_DIR = "/Game/DreamMaterials/Card"
BASE_MATERIAL_PATH = MATERIAL_DIR + "/M_FirstPersonCard_SurfaceEffects_DrawReveal"
MATERIAL_INSTANCE_PATH = MATERIAL_DIR + "/MI_FirstPersonCard_SurfaceEffects_DrawReveal_Default"
STYLE_PATH = SURFACE_DIR + "/DA_FPCardDrawRevealStyle_PixelBack"
PLAYER_BP_PATH = "/Game/Wacom/Core/Player/BP_WacomPlayerCharacter"


def import_card_back_texture():
    if not TEXTURE_SOURCE.is_file():
        raise RuntimeError(f"Draw Reveal card-back source is missing: {TEXTURE_SOURCE}")

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(TEXTURE_SOURCE))
    task.set_editor_property("destination_path", SURFACE_DIR)
    task.set_editor_property("destination_name", "T_FPCardDrawBack_Temporary")
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    texture = unreal.load_asset(TEXTURE_PATH)
    if not texture:
        raise RuntimeError("Failed to import temporary Draw Reveal card back")
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
    return texture


def load_required(asset_path, label):
    asset = unreal.load_asset(asset_path)
    if not asset:
        raise RuntimeError(f"{label} is missing: {asset_path}")
    return asset


def load_or_create_material_instance(parent, card_back_texture):
    instance = unreal.load_asset(MATERIAL_INSTANCE_PATH)
    if not instance:
        factory = unreal.MaterialInstanceConstantFactoryNew()
        instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "MI_FirstPersonCard_SurfaceEffects_DrawReveal_Default",
            MATERIAL_DIR,
            unreal.MaterialInstanceConstant,
            factory,
        )
    if not instance:
        raise RuntimeError("Failed to create Draw Reveal material instance")

    unreal.MaterialEditingLibrary.set_material_instance_parent(instance, parent)
    unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(
        instance, "CardBackTexture", card_back_texture
    )
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
        instance,
        "DrawRevealBleedColor",
        unreal.LinearColor(0.035, 0.055, 0.13, 1.0),
    )
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
        instance,
        "DrawRevealEdgeColor",
        unreal.LinearColor(0.72, 0.88, 1.0, 1.0),
    )
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
        instance, "DrawRevealEdgeBrightness", 1.25
    )
    unreal.EditorAssetLibrary.save_loaded_asset(instance)
    return instance


def load_or_create_style(material_instance):
    style_asset = unreal.load_asset(STYLE_PATH)
    if not style_asset:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property(
            "data_asset_class", unreal.WacomFirstPersonCardDrawRevealStyle
        )
        style_asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "DA_FPCardDrawRevealStyle_PixelBack",
            SURFACE_DIR,
            unreal.WacomFirstPersonCardDrawRevealStyle,
            factory,
        )
    if not style_asset:
        raise RuntimeError("Failed to create Draw Reveal Style")

    style = style_asset.get_editor_property("style")
    style.set_editor_property("surface_effect_material_instance", material_instance)
    style.set_editor_property("back_hold_end_progress", 0.45)
    style.set_editor_property("face_switch_progress", 0.615)
    style.set_editor_property("face_expand_end_progress", 0.78)
    style.set_editor_property("minimum_horizontal_scale", 0.06)
    style.set_editor_property("landing_start_progress", 0.82)
    style.set_editor_property("landing_peak_progress", 0.90)
    style.set_editor_property("landing_scale", unreal.Vector2D(1.035, 0.96))
    style.set_editor_property("landing_translation_y_pixels", 3.0)
    style.set_editor_property("reduced_cross_fade_start_progress", 0.55)
    style.set_editor_property("reduced_cross_fade_end_progress", 0.75)
    style_asset.set_editor_property("style", style)
    unreal.EditorAssetLibrary.save_loaded_asset(style_asset)
    return style_asset


def assign_style_to_player(style_asset):
    player_bp = load_required(PLAYER_BP_PATH, "BP_WacomPlayerCharacter")
    player_cdo = unreal.get_default_object(player_bp.generated_class())
    anchor = player_cdo.get_editor_property("first_person_card_anchor_component")
    if not anchor:
        raise RuntimeError("FirstPersonCardAnchorComponent is missing on player CDO")

    anchor.set_editor_property("enable_card_draw_reveal", True)
    anchor.set_editor_property("card_draw_reveal_style", style_asset)
    anchor.set_editor_property("reduce_card_draw_reveal_motion", False)
    unreal.EditorAssetLibrary.save_loaded_asset(player_bp)


card_back = import_card_back_texture()
if os.environ.get("WACOM_DRAW_REVEAL_IMPORT_ONLY", "0") != "1":
    base_material = load_required(BASE_MATERIAL_PATH, "Draw Reveal DreamShader material")
    default_instance = load_or_create_material_instance(base_material, card_back)
    default_style = load_or_create_style(default_instance)
    if os.environ.get("WACOM_SKIP_DRAW_REVEAL_ANCHOR", "0") != "1":
        assign_style_to_player(default_style)
    unreal.log("Wacom Battle Draw Reveal assets configured")
else:
    unreal.log("Wacom Battle Draw Reveal temporary card-back texture imported")
