"""Deterministically crop, resize and palette-quantize the temporary draw card back."""

from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image


TARGET_SIZE = (360, 424)
PALETTE = (
    (7, 19, 58),
    (9, 36, 94),
    (55, 90, 164),
    (112, 163, 222),
    (181, 222, 245),
    (247, 211, 116),
    (211, 154, 68),
    (211, 42, 136),
)


def build_palette_image() -> Image.Image:
    palette_image = Image.new("P", (1, 1))
    flattened = [channel for color in PALETTE for channel in color]
    flattened.extend([0] * (768 - len(flattened)))
    palette_image.putpalette(flattened)
    return palette_image


def centered_crop_to_aspect(image: Image.Image, target_size: tuple[int, int]) -> Image.Image:
    target_aspect = target_size[0] / target_size[1]
    source_aspect = image.width / image.height
    if source_aspect > target_aspect:
        crop_width = round(image.height * target_aspect)
        left = (image.width - crop_width) // 2
        return image.crop((left, 0, left + crop_width, image.height))
    crop_height = round(image.width / target_aspect)
    top = (image.height - crop_height) // 2
    return image.crop((0, top, image.width, top + crop_height))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    with Image.open(args.source) as source:
        rgb = source.convert("RGB")
        cropped = centered_crop_to_aspect(rgb, TARGET_SIZE)
        resized = cropped.resize(TARGET_SIZE, Image.Resampling.NEAREST)
        quantized = resized.quantize(
            palette=build_palette_image(),
            dither=Image.Dither.NONE,
        ).convert("RGB")
        args.output.parent.mkdir(parents=True, exist_ok=True)
        quantized.save(args.output, format="PNG", optimize=False)


if __name__ == "__main__":
    main()
