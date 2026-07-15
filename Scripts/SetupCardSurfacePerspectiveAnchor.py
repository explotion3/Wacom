"""Idempotently apply only first-person card surface-parallax Anchor defaults.

This intentionally does not touch RunPath, hand layout, interaction feedback,
other card effects, or any unrelated player Blueprint field. It exists so an
integration branch can keep its newer BP_WacomPlayerCharacter asset and replay
the card-surface settings without accepting a binary Blueprint merge.
"""

import unreal


PLAYER_BP_PATH = "/Game/Wacom/Core/Player/BP_WacomPlayerCharacter"

PARALLAX_DEFAULTS = {
    "enable_card_surface_parallax": True,
    "card_surface_parallax_strength": 1.0,
    "attachment_parallax_depth_pixels": 5.0,
    "attachment_parallax_max_offset_pixels": 7.0,
    "reduce_card_surface_parallax_motion": False,
}


player_bp = unreal.load_asset(PLAYER_BP_PATH)
if not player_bp:
    raise RuntimeError(f"Player Blueprint is missing: {PLAYER_BP_PATH}")

player_cdo = unreal.get_default_object(player_bp.generated_class())
anchor = player_cdo.get_editor_property("first_person_card_anchor_component")
if not anchor:
    raise RuntimeError("BP_WacomPlayerCharacter has no FirstPersonCardAnchorComponent")

for property_name, value in PARALLAX_DEFAULTS.items():
    anchor.set_editor_property(property_name, value)

unreal.EditorAssetLibrary.save_loaded_asset(player_bp)
unreal.log("Wacom card-surface perspective Anchor defaults configured")

