#!/usr/bin/env python
#
# Responsibility: From a full stage PNG and an optional existing tileset,
#                 generate/extend a tileset atlas and a tile-id CSV.
# Non-Goals    :  GUI editor, non-grid stages, advanced packing.
# Call-Context:  Offline tool, run manually (Python + Pillow required).
#

import argparse
from pathlib import Path
from typing import Dict, List, Tuple

from PIL import Image


def read_base_tileset(tileset_path: Path, tile_size: int) -> Tuple[List[Image.Image], Dict[bytes, int], int]:
    """
    Load an existing tileset atlas and build:
      - tiles: list of tile images in ID order
      - lookup: pixel-bytes -> id  (only first occurrence kept)
      - cols: number of columns in the atlas (must stay constant to preserve IDs)
    """
    img = Image.open(tileset_path).convert("RGBA")
    w, h = img.size

    if w % tile_size != 0 or h % tile_size != 0:
        raise SystemExit(
            f"[ERROR] Tileset '{tileset_path}' size {w}x{h} is not divisible by tile_size={tile_size}"
        )

    cols = w // tile_size
    rows = h // tile_size

    tiles: List[Image.Image] = []
    lookup: Dict[bytes, int] = {}

    idx = 0
    for gy in range(rows):
        for gx in range(cols):
            left = gx * tile_size
            top = gy * tile_size
            tile = img.crop((left, top, left + tile_size, top + tile_size))
            tiles.append(tile)
            key = tile.tobytes()
            # 같은 이미지가 중복돼 있어도 첫 번째 id를 유지
            if key not in lookup:
                lookup[key] = idx
            idx += 1

    print(f"[INFO] Loaded base tileset '{tileset_path}' ({len(tiles)} tiles, cols={cols}).")
    return tiles, lookup, cols


def save_tileset_atlas(tiles: List[Image.Image], tile_size: int, cols: int, out_path: Path) -> None:
    """
    Pack all tiles into a single atlas with given number of columns.
    - Important: cols must match the previous value for existing tilesets,
      so that TileSet::Cols() stays the same and IDs keep mapping to the same cells.
    """
    if cols <= 0:
        raise SystemExit("[ERROR] cols must be > 0.")

    n = len(tiles)
    if n == 0:
        # 빈 스테이지는 흔치 않겠지만, 안전하게 1x1 투명 타일이라도 만들어둔다.
        cols = 1
        n = 1
        tiles = [Image.new("RGBA", (tile_size, tile_size), (0, 0, 0, 0))]

    rows = (n + cols - 1) // cols
    atlas_w = cols * tile_size
    atlas_h = rows * tile_size

    atlas = Image.new("RGBA", (atlas_w, atlas_h), (0, 0, 0, 0))

    for idx, tile in enumerate(tiles):
        gx = idx % cols
        gy = idx // cols
        left = gx * tile_size
        top = gy * tile_size
        # 혹시 다른 크기의 이미지가 들어와도 강제로 tile_size로 맞춘다.
        if tile.size != (tile_size, tile_size):
            tile = tile.resize((tile_size, tile_size), Image.NEAREST)
        atlas.paste(tile, (left, top))

    atlas.save(out_path)
    print(f"[INFO] Wrote tileset atlas '{out_path}' ({n} tiles, {cols} cols).")


def write_tilemap_csv(tile_ids: List[List[int]], out_path: Path) -> None:
    with out_path.open("w", encoding="utf-8", newline="") as f:
        for row in tile_ids:
            f.write(",".join(str(v) for v in row))
            f.write("\n")
    print(f"[INFO] Wrote tilemap CSV '{out_path}'.")


def build_tilemap_from_stage(
    stage_img_path: Path,
    tile_size: int,
    tiles: List[Image.Image],
    base_lookup: Dict[bytes, int],
    dedupe_new: bool
) -> List[List[int]]:
    """
    Scan stage image in tile_size x tile_size chunks.
    - If tile matches base_lookup (existing tileset), reuse that id.
    - Else, either:
        * dedupe_new=True  → reuse newly added tiles as well
        * dedupe_new=False → each new tile becomes a distinct ID
    """
    img = Image.open(stage_img_path).convert("RGBA")
    w, h = img.size

    if w % tile_size != 0 or h % tile_size != 0:
        raise SystemExit(
            f"[ERROR] Stage image '{stage_img_path}' size {w}x{h} is not divisible by tile_size={tile_size}"
        )

    map_w = w // tile_size
    map_h = h // tile_size

    print(f"[INFO] Stage '{stage_img_path}' -> grid {map_w}x{map_h} tiles (size {tile_size}x{tile_size}).")

    # 새 dedupe를 위해 base_lookup을 확장할 수도 있고, 아닐 수도 있다.
    #  - dedupe_new=True  → 새 타일도 lookup에 넣어서 재사용
    #  - dedupe_new=False → base 타일만 lookup; 새 타일들은 중복이어도 다른 ID
    if dedupe_new:
        lookup = dict(base_lookup)
    else:
        lookup = base_lookup  # 새 타일은 lookup에 넣지 않는다.

    tilemap_rows: List[List[int]] = []

    for ty in range(map_h):
        row: List[int] = []
        for tx in range(map_w):
            left = tx * tile_size
            top = ty * tile_size
            tile = img.crop((left, top, left + tile_size, top + tile_size))
            key = tile.tobytes()

            if key in lookup:
                tile_id = lookup[key]
            else:
                tile_id = len(tiles)
                tiles.append(tile)
                if dedupe_new:
                    lookup[key] = tile_id

            row.append(tile_id)
        tilemap_rows.append(row)

    print(f"[INFO] Stage scan complete. Total tiles in atlas now: {len(tiles)}.")
    return tilemap_rows


def main() -> None:
    ap = argparse.ArgumentParser(
        description=(
            "Generate/extend a tileset atlas and tile-id CSV from a full stage image.\n"
            "- If --tileset exists: reuse its tile IDs and append new tiles.\n"
            "- If not: create a new tileset like the original make_tiles.py.\n"
        )
    )
    ap.add_argument("--input", required=True,
                    help="Full stage PNG image (e.g., stage01_full.png)")
    ap.add_argument("--tileset", required=True,
                    help="Tileset PNG path to read/extend/write (e.g., assets/tilesets/stage01_tiles.png)")
    ap.add_argument("--tilemap", required=True,
                    help="Output CSV path for tile IDs (e.g., assets/stages/stage01/tilemap.csv)")
    ap.add_argument("--tile-size", type=int, default=16,
                    help="Tile size in pixels (default: 16)")
    ap.add_argument("--atlas-cols", type=int, default=16,
                    help="Number of columns when creating a NEW tileset (ignored if tileset already exists).")
    ap.add_argument("--no-dedupe", action="store_true",
                    help="Do NOT dedupe new tiles within this stage (still reuses existing tileset tiles).")

    args = ap.parse_args()

    stage_img_path = Path(args.input)
    tileset_path = Path(args.tileset)
    tilemap_path = Path(args.tilemap)
    tile_size = args.tile_size
    dedupe_new = not args.no_dedupe

    if not stage_img_path.is_file():
        raise SystemExit(f"[ERROR] Stage image '{stage_img_path}' does not exist.")

    # 1) Base tileset 로드 (있으면)
    tiles: List[Image.Image]
    base_lookup: Dict[bytes, int]
    cols: int

    if tileset_path.is_file():
        tiles, base_lookup, cols = read_base_tileset(tileset_path, tile_size)
        print(f"[INFO] Existing tileset found. New tiles will be appended, cols kept at {cols}.")
    else:
        tiles = []
        base_lookup = {}
        cols = args.atlas_cols if args.atlas_cols > 0 else 16
        print(f"[INFO] No tileset found. Creating NEW one with atlas_cols={cols}.")

    # 2) Stage 이미지에서 tilemap + (필요시) 새 타일 추가
    tilemap_rows = build_tilemap_from_stage(
        stage_img_path=stage_img_path,
        tile_size=tile_size,
        tiles=tiles,
        base_lookup=base_lookup,
        dedupe_new=dedupe_new,
    )

    # 3) 타일셋 atlas 다시 저장 (기존이 있으면 확장/덮어쓰기)
    save_tileset_atlas(tiles, tile_size, cols, tileset_path)

    # 4) tilemap CSV 저장
    write_tilemap_csv(tilemap_rows, tilemap_path)

    print("[INFO] Done.")


if __name__ == "__main__":
    main()
