"""Create/update the project-owned Played dissolve authoring assets.

Run after DreamShader has generated
``/Game/DreamMaterials/Card/M_FirstPersonCard_SurfaceEffects``.
"""

from pathlib import Path

import unreal


PROJECT_DIR = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
TEXTURE_SOURCE = PROJECT_DIR / "DShader" / "Texture" / "Card" / "T_FirstPersonCard_PlayedDissolveNoise.png"
DESTINATION = "/Game/Wacom/UI/Card/SurfaceEffects"
TEXTURE_PATH = DESTINATION + "/T_FirstPersonCard_PlayedDissolveNoise"
MATERIAL_PATH = "/Game/DreamMaterials/Card/M_FirstPersonCard_SurfaceEffects"
STYLE_PATH = DESTINATION + "/DA_FPCardPlayedDissolveStyle_PixelAsh"
PLAYER_BP_PATH = "/Game/Wacom/Core/Player/BP_WacomPlayerCharacter"
LEGACY_ASSET_PATHS = (
    "/Game/DreamMaterials/Card/DA_FPCardPlayedDissolveStyle_PixelAsh",
    "/Game/DreamMaterials/Card/T_FirstPersonCard_PlayedDissolveNoise",
)


def import_noise_texture():
    if not TEXTURE_SOURCE.is_file():
        raise RuntimeError(f"Played dissolve noise source is missing: {TEXTURE_SOURCE}")

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(TEXTURE_SOURCE))
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("destination_name", "T_FirstPersonCard_PlayedDissolveNoise")
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    texture = unreal.load_asset(TEXTURE_PATH)
    if not texture:
        raise RuntimeError("Failed to import Played dissolve noise texture")
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
    texture.set_editor_property("srgb", False)
    texture.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
    return texture


def create_style(texture):
    material = unreal.load_asset(MATERIAL_PATH)
    if not material:
        raise RuntimeError(
            "Played dissolve DreamShader material is missing; generate the .dsm before running this script"
        )

    style_asset = unreal.load_asset(STYLE_PATH)
    if not style_asset:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property(
            "data_asset_class", unreal.WacomFirstPersonCardPlayedDissolveStyle
        )
        style_asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "DA_FPCardPlayedDissolveStyle_PixelAsh",
            DESTINATION,
            unreal.WacomFirstPersonCardPlayedDissolveStyle,
            factory,
        )
    if not style_asset:
        raise RuntimeError("Failed to create Played dissolve style DataAsset")

    style = style_asset.get_editor_property("style")
    style.set_editor_property("surface_effect_material", material)
    style.set_editor_property("noise_texture", texture)
    style.set_editor_property("duration_seconds", 0.40)
    style.set_editor_property("confirm_hold_seconds", 0.05)
    style.set_editor_property("grid_columns", 96.0)
    style.set_editor_property("direction_angle_degrees", -78.0)
    style.set_editor_property("jitter", 0.32)
    style.set_editor_property("edge_color", unreal.LinearColor(1.0, 0.82, 0.34, 1.0))
    style.set_editor_property("edge_accent_color", unreal.LinearColor(0.46, 0.66, 1.0, 1.0))
    style.set_editor_property("edge_width", 0.045)
    style.set_editor_property("edge_intensity", 1.35)
    style.set_editor_property("ash_density", 0.18)
    style.set_editor_property("ash_trail_width", 0.14)
    style.set_editor_property("ash_lift_pixels", 22.0)
    style.set_editor_property("ash_drift_pixels", 7.0)
    style.set_editor_property("shadow_fade_fraction", 0.25)
    style_asset.set_editor_property("style", style)
    unreal.EditorAssetLibrary.save_loaded_asset(style_asset)
    return style_asset


def assign_style_to_player(style_asset):
    player_bp = unreal.load_asset(PLAYER_BP_PATH)
    if not player_bp:
        unreal.log_warning("BP_WacomPlayerCharacter not found; assign the Style manually")
        return
    player_cdo = unreal.get_default_object(player_bp.generated_class())
    anchor = player_cdo.get_editor_property("first_person_card_anchor_component")
    if not anchor:
        unreal.log_warning("FirstPersonCardAnchorComponent not found; assign the Style manually")
        return
    anchor.set_editor_property("card_played_dissolve_style", style_asset)
    anchor.set_editor_property("enable_card_played_dissolve", True)
    unreal.EditorAssetLibrary.save_loaded_asset(player_bp)


def remove_legacy_project_assets():
    for asset_path in LEGACY_ASSET_PATHS:
        if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
            unreal.EditorAssetLibrary.delete_asset(asset_path)


noise_texture = import_noise_texture()
style = create_style(noise_texture)
assign_style_to_player(style)
remove_legacy_project_assets()
unreal.EditorAssetLibrary.save_directory(DESTINATION, only_if_is_dirty=False, recursive=True)
unreal.log("Wacom Played dissolve assets created and configured")
