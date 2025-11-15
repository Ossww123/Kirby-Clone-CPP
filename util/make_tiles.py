# make_tiles_from_image.py
#
# 큰 맵 이미지를 16x16 타일 단위로 쪼개서
# 1) 중복 없는 tileset.png (N*M 그리드)
# 2) 각 위치의 타일 인덱스를 담은 tilemap.csv
# 를 생성하는, 프로젝트와 별개의 유틸리티.
#
# tilemap.csv 형식은 C++의 StageCSV::LoadTileMapCSV 와 호환:
#   - 헤더 없음
#   - 각 줄 = y 한 줄
#   - "0,1,2,3,3,3,..." 이런 식으로 콤마로만 구분
#
# 사용 예:
#   python make_tiles_from_image.py ^
#       --input stage.png ^
#       --tileset tileset.png ^
#       --tilemap tilemap.csv ^
#       --tile-size 16
#
# 필요:
#   pip install pillow

import argparse
import math
from pathlib import Path

from PIL import Image
import csv


def build_tileset_and_tilemap(
    input_image: Path,
    out_tileset: Path,
    out_tilemap: Path,
    tile_size: int = 16,
    dedupe: bool = True,
) -> None:
    img = Image.open(input_image).convert("RGBA")
    img_w, img_h = img.size

    tw = th = tile_size
    if img_w % tw != 0 or img_h % th != 0:
        raise SystemExit(
            f"image size {img_w}x{img_h} is not a multiple of tile_size={tile_size}"
        )

    tiles_x = img_w // tw
    tiles_y = img_h // th
    print(f"image: {img_w}x{img_h}, grid: {tiles_x} x {tiles_y} tiles")

    # 타일 dedupe용 딕셔너리
    tile_dict: dict[bytes, int] = {}
    tiles: list[Image.Image] = []

    # tilemap[y][x] = tile_id
    tilemap: list[list[int]] = [[-1 for _ in range(tiles_x)] for _ in range(tiles_y)]

    for ty in range(tiles_y):
        for tx in range(tiles_x):
            box = (tx * tw, ty * th, (tx + 1) * tw, (ty + 1) * th)
            tile_img = img.crop(box)
            key = tile_img.tobytes()

            if dedupe:
                tile_id = tile_dict.get(key)
                if tile_id is None:
                    tile_id = len(tiles)
                    tile_dict[key] = tile_id
                    tiles.append(tile_img)
            else:
                tile_id = len(tiles)
                tiles.append(tile_img)

            tilemap[ty][tx] = tile_id

    print(f"unique tiles: {len(tiles)}")

    if not tiles:
        raise SystemExit("no tiles found (image empty?)")

    # ===== tileset.png 만들기 =====
    # 타일셋은 되도록 정사각형에 가까운 N*M 그리드로 배치
    n = len(tiles)
    cols = max(1, int(math.ceil(math.sqrt(n))))
    rows = int(math.ceil(n / cols))

    tileset_w = cols * tw
    tileset_h = rows * th
    tileset_img = Image.new("RGBA", (tileset_w, tileset_h), (0, 0, 0, 0))

    for idx, tile_img in enumerate(tiles):
        tx = idx % cols
        ty = idx // cols
        tileset_img.paste(tile_img, (tx * tw, ty * th))

    tileset_img.save(out_tileset)
    print(f"saved tileset: {out_tileset} ({tileset_w}x{tileset_h}, {cols}x{rows} tiles)")

    # ===== tilemap.csv 만들기 =====
    # StageCSV::LoadTileMapCSV 와 호환:
    #   - 헤더 없음
    #   - 각 줄은 콤마로 구분된 정수들
    #   - 공백 없음
    with open(out_tilemap, "w", newline="") as f:
        writer = csv.writer(f, lineterminator="\n")
        for row in tilemap:
            writer.writerow(row)

    print(f"saved tilemap: {out_tilemap} ({tiles_x} x {tiles_y})")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Convert a full-stage image into tileset.png + tilemap.csv"
    )
    parser.add_argument("--input", required=True, help="input PNG image path")
    parser.add_argument("--tileset", required=True, help="output tileset.png path")
    parser.add_argument("--tilemap", required=True, help="output tilemap.csv path")
    parser.add_argument(
        "--tile-size", type=int, default=16, help="tile size in pixels (default: 16)"
    )
    parser.add_argument(
        "--no-dedupe",
        action="store_true",
        help="disable tile deduplication (each cell becomes a unique tile)",
    )

    args = parser.parse_args()

    build_tileset_and_tilemap(
        Path(args.input),
        Path(args.tileset),
        Path(args.tilemap),
        tile_size=args.tile_size,
        dedupe=not args.no_dedupe,
    )


if __name__ == "__main__":
    main()
