"""Create/update the default pixel-glyph pile-transfer MI and Style.

Run after DreamShader generated M_FirstPersonCard_PileTransferGlyph.
The script is idempotent and only assigns the new Style to the player Anchor.
"""

import unreal


MATERIAL_DIR = "/Game/DreamMaterials/Card"
BASE_MATERIAL_PATH = MATERIAL_DIR + "/M_FirstPersonCard_PileTransferGlyph"
MATERIAL_INSTANCE_PATH = MATERIAL_DIR + "/MI_FirstPersonCard_PileTransferGlyph_Default"
STYLE_DIR = "/Game/Wacom/UI/Card/SurfaceEffects"
STYLE_PATH = STYLE_DIR + "/DA_FPCardPileTransferStyle_PixelGlyph"
PLAYER_BP_PATH = "/Game/Wacom/Core/Player/BP_WacomPlayerCharacter"


def load_required(path, label):
    asset = unreal.load_asset(path)
    if not asset:
        raise RuntimeError(f"{label} is missing: {path}")
    return asset


base_material = load_required(BASE_MATERIAL_PATH, "Pile-transfer DreamShader material")
material_instance = unreal.load_asset(MATERIAL_INSTANCE_PATH)
if not material_instance:
    material_instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "MI_FirstPersonCard_PileTransferGlyph_Default",
        MATERIAL_DIR,
        unreal.MaterialInstanceConstant,
        unreal.MaterialInstanceConstantFactoryNew(),
    )
if not material_instance:
    raise RuntimeError("Failed to create pile-transfer material instance")

unreal.MaterialEditingLibrary.set_material_instance_parent(material_instance, base_material)
unreal.MaterialEditingLibrary.clear_all_material_instance_parameters(material_instance)
for name, value in {
    "GlyphPixelColumns": 14.0,
    "GlyphBorderWidth": 0.10,
    "GlyphCenterMarkSize": 0.22,
    "GlyphGlowStrength": 0.65,
    "TrailPrimaryWeight": 0.50,
    "TrailSecondaryWeight": 0.32,
    "TrailAccentWeight": 0.18,
    "TrailPixelColumns": 6.0,
    "TrailPixelRows": 2.0,
    "TrailHeadPixelRetention": 0.88,
    "TrailTailPixelRetention": 0.20,
    "TrailGlowStrength": 0.45,
    "MoteGlowStrength": 0.55,
    "ImpactGlowStrength": 0.85,
}.items():
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
        material_instance, name, value)
for name, value in {
    "GlyphOutlineColor": unreal.LinearColor(0.96, 0.82, 0.42, 1.0),
    "GlyphFillColor": unreal.LinearColor(0.08, 0.18, 0.32, 0.92),
    "GlyphCenterColor": unreal.LinearColor(0.58, 0.80, 1.0, 1.0),
    "TrailPrimaryColor": unreal.LinearColor(0.58, 0.80, 1.0, 1.0),
    "TrailSecondaryColor": unreal.LinearColor(0.96, 0.82, 0.42, 1.0),
    "TrailAccentColor": unreal.LinearColor(0.88, 0.30, 0.72, 1.0),
    "MotePrimaryColor": unreal.LinearColor(0.62, 0.82, 1.0, 1.0),
    "MoteAccentColor": unreal.LinearColor(1.0, 0.82, 0.42, 1.0),
    "ImpactPrimaryColor": unreal.LinearColor(0.58, 0.80, 1.0, 1.0),
    "ImpactAccentColor": unreal.LinearColor(0.96, 0.82, 0.42, 1.0),
}.items():
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
        material_instance, name, value)
unreal.EditorAssetLibrary.save_loaded_asset(material_instance)

style_asset = unreal.load_asset(STYLE_PATH)
if not style_asset:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.WacomFirstPersonCardPileTransferStyle)
    style_asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "DA_FPCardPileTransferStyle_PixelGlyph",
        STYLE_DIR,
        unreal.WacomFirstPersonCardPileTransferStyle,
        factory,
    )
if not style_asset:
    raise RuntimeError("Failed to create pile-transfer Style")

style = style_asset.get_editor_property("style")
style.set_editor_property("glyph_material_instance", material_instance)
style.set_editor_property("glyph_size", unreal.Vector2D(42.0, 66.0))
style.set_editor_property("start_charge_seconds", 0.08)
style.set_editor_property("flight_seconds", 0.36)
style.set_editor_property("lane_count", 3)
style.set_editor_property("base_stagger_seconds", 0.045)
style.set_editor_property("settle_seconds", 0.24)
style.set_editor_property("arc_height_ratio", 0.18)
style.set_editor_property("min_arc_height_pixels", 48.0)
style.set_editor_property("max_arc_height_pixels", 128.0)
style.set_editor_property("discard_collapse_seconds", 0.11)
style.set_editor_property("discard_glyph_reveal_start_seconds", 0.06)
style.set_editor_property("discard_flight_seconds", 0.28)
style.set_editor_property("discard_stagger_seconds", 0.055)
style.set_editor_property("discard_impact_seconds", 0.12)
style.set_editor_property("discard_impact_scale", 1.55)
style.set_editor_property("enable_trail", True)
style.set_editor_property("trail_sample_interval_seconds", 0.007)
style.set_editor_property("high_detail_trail_segments_per_glyph", 7)
style.set_editor_property("medium_detail_trail_segments_per_glyph", 5)
style.set_editor_property("low_detail_trail_segments_per_glyph", 3)
style.set_editor_property("trail_head_width_pixels", 10.5)
style.set_editor_property("trail_tail_width_pixels", 3.0)
style.set_editor_property("trail_head_opacity", 0.44)
style.set_editor_property("trail_tail_opacity", 0.04)
style.set_editor_property("max_trail_quad_count", 120)
style.set_editor_property("mote_lifetime_seconds", 0.24)
style.set_editor_property("mote_min_size_pixels", 6.0)
style.set_editor_property("mote_max_size_pixels", 13.5)
style.set_editor_property("mote_backward_distance_pixels", 28.0)
style.set_editor_property("mote_lateral_distance_pixels", 14.0)
style.set_editor_property("high_detail_max_active_glyphs", 6)
style.set_editor_property("medium_detail_max_active_glyphs", 14)
style.set_editor_property("high_detail_mote_slots_per_glyph", 14)
style.set_editor_property("medium_detail_mote_slots_per_glyph", 8)
style.set_editor_property("low_detail_mote_slots_per_glyph", 4)
style.set_editor_property("max_mote_quad_count", 240)
style.set_editor_property("safe_viewport_padding_pixels", 36.0)
style.set_editor_property("reduced_motion_duration_seconds", 0.18)
style_asset.set_editor_property("style", style)
unreal.EditorAssetLibrary.save_loaded_asset(style_asset)

player_bp = load_required(PLAYER_BP_PATH, "BP_WacomPlayerCharacter")
player_cdo = unreal.get_default_object(player_bp.generated_class())
anchor = player_cdo.get_editor_property("first_person_card_anchor_component")
if not anchor:
    raise RuntimeError("FirstPersonCardAnchorComponent is missing on player CDO")
anchor.set_editor_property("enable_card_pile_transfer", True)
anchor.set_editor_property("enable_card_discard_glyph_transfer", True)
anchor.set_editor_property("card_pile_transfer_style", style_asset)
unreal.EditorAssetLibrary.save_loaded_asset(player_bp)
unreal.log("Wacom first-person pile-transfer pixel glyph assets configured")
