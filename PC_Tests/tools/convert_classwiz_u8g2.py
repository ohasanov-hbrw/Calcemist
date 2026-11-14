#!/usr/bin/env python3
"""
Convert ClassWiz Display fonts (UFO 3 sources) to BDF and/or u8g2 .c font files.

Upstream: https://github.com/Wenti-D/ClasswizDisplayFont
  - ClassWizXDisplay-Regular.ufo
  - ClassWizCWDisplay-Regular.ufo

The display fonts are pixel-grid designs (10 font units per pixel, 140 units/em).
This script rasterizes UFO glyph contours onto that grid and can emit:
  - .bdf  (Glyph Bitmap Distribution Format)
  - .c    (u8g2 binary font array, via `bdfconv` if available)

No third-party Python packages required (uses stdlib XML parsing).

Examples:
  # Convert from a local clone/checkout of ClasswizDisplayFont
  python3 convert_classwiz_u8g2.py --ufo path/to/ClassWizXDisplay-Regular.ufo

  # Download UFO from GitHub into a cache dir, then convert both display fonts
  python3 convert_classwiz_u8g2.py --download

  # Generate only ASCII range as u8g2 (needs bdfconv in PATH)
  python3 convert_classwiz_u8g2.py --ufo ClassWizXDisplay-Regular.ufo \
      --name u8g2_font_classwiz_x_tr --format c --bdfconv-map "32-126"
"""
from __future__ import annotations

import argparse
import json
import os
import plistlib
import re
import shutil
import subprocess
import sys
import tempfile
import urllib.request
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Tuple

REPO_API = "https://api.github.com/repos/Wenti-D/ClasswizDisplayFont/contents"
RAW_BASE = "https://raw.githubusercontent.com/Wenti-D/ClasswizDisplayFont/main"
UFO_DIRS = ["ClassWizXDisplay-Regular.ufo", "ClassWizCWDisplay-Regular.ufo"]


@dataclass
class Glyph:
    name: str
    filename: str
    codepoints: List[int]
    advance: int
    contours: List[List[Tuple[float, float]]]


@dataclass
class FontInfo:
    family: str
    style: str
    ps_name: str
    units_per_em: int
    ascender: int
    descender: int
    cap_height: Optional[int]
    x_height: Optional[int]


def _http_get(url: str) -> bytes:
    req = urllib.request.Request(url, headers={"User-Agent": "classwiz-u8g2-converter"})
    with urllib.request.urlopen(req) as resp:
        return resp.read()


def download_ufo(ufo_name: str, dest: Path) -> Path:
    """Download a UFO directory from GitHub into dest/ufo_name."""
    ufo_path = dest / ufo_name
    (ufo_path / "glyphs").mkdir(parents=True, exist_ok=True)

    def dl_file(repo_path: str, out: Path) -> None:
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_bytes(_http_get(f"{RAW_BASE}/{repo_path}"))

    # top-level files
    items = json.loads(_http_get(f"{REPO_API}/{ufo_name}").decode("utf-8"))
    for it in items:
        if it["type"] == "file":
            dl_file(it["path"], ufo_path / it["name"])
        elif it["type"] == "dir" and it["name"] == "glyphs":
            gl_items = json.loads(_http_get(it["url"]).decode("utf-8"))
            for g in gl_items:
                if g["type"] == "file":
                    dl_file(g["path"], ufo_path / "glyphs" / g["name"])
        elif it["type"] == "dir" and it["name"] == "data":
            # not needed for bitmap conversion, but try to mirror lightly
            pass
    return ufo_path


def load_fontinfo(ufo: Path) -> FontInfo:
    with (ufo / "fontinfo.plist").open("rb") as f:
        info = plistlib.load(f)
    return FontInfo(
        family=info.get("familyName") or info.get("styleMapFamilyName") or ufo.stem,
        style=info.get("styleName") or "Regular",
        ps_name=info.get("postscriptFontName") or ufo.stem.replace(" ", ""),
        units_per_em=int(info.get("unitsPerEm", 1000)),
        ascender=int(info.get("ascender", 800)),
        descender=int(info.get("descender", -200)),
        cap_height=(int(info["capHeight"]) if "capHeight" in info else None),
        x_height=(int(info["xHeight"]) if "xHeight" in info else None),
    )


def load_contents(ufo: Path) -> Dict[str, str]:
    with (ufo / "glyphs" / "contents.plist").open("rb") as f:
        return plistlib.load(f)


def parse_glif(path: Path, glyph_name: str) -> Glyph:
    tree = ET.parse(path)
    root = tree.getroot()
    adv_el = root.find("advance")
    advance = int(float(adv_el.get("width", "0"))) if adv_el is not None else 0
    codepoints = []
    for u in root.findall("unicode"):
        hx = u.get("hex")
        if hx:
            try:
                codepoints.append(int(hx, 16))
            except ValueError:
                pass
    contours: List[List[Tuple[float, float]]] = []
    outline = root.find("outline")
    if outline is not None:
        for contour in outline.findall("contour"):
            pts = []
            for pt in contour.findall("point"):
                x = float(pt.get("x", "0"))
                y = float(pt.get("y", "0"))
                # include all points; pixel fonts use on-curve points
                pts.append((x, y))
            if pts:
                contours.append(pts)
        # components (rare in pixel display font) are ignored; could be added if needed
    return Glyph(name=glyph_name, filename=path.name, codepoints=codepoints, advance=advance, contours=contours)


def point_in_poly(x: float, y: float, poly: Sequence[Tuple[float, float]]) -> bool:
    # ray casting
    inside = False
    n = len(poly)
    j = n - 1
    for i in range(n):
        xi, yi = poly[i]
        xj, yj = poly[j]
        intersect = ((yi > y) != (yj > y)) and (x < (xj - xi) * (y - yi) / (yj - yi + 1e-12) + xi)
        if intersect:
            inside = not inside
        j = i
    return inside


def rasterize_glyph(g: Glyph, finfo: FontInfo, grid: int = 10) -> Tuple[int, int, int, int, List[List[int]]]:
    """Return (w_px, h_px, xoff_px, yoff_px_baseline_top, bitmap_rows_top_to_bottom)."""
    em_h = finfo.units_per_em // grid
    adv_w = max(0, int(round(g.advance / grid)))

    # determine used pixel bounds to tighten BBX (within em box vertically)
    min_px = finfo.units_per_em
    min_py = finfo.descender
    max_px = 0
    max_py = finfo.ascender
    # sample all pixels in em box
    rows: List[List[int]] = []
    any_on = False
    y0 = finfo.descender
    y1 = finfo.ascender
    x0u = 0
    x1u = max(g.advance, finfo.units_per_em)
    # We'll rasterize full advance width x em height grid
    w_px = max(adv_w, 0)
    h_px = em_h
    # pixel (px,py) corresponds to font rect [px*grid,(px+1)*grid] x [py*grid,(py+1)*grid]
    # test center
    bitmap = [[0 for _ in range(w_px)] for _ in range(h_px)]
    for py in range(y0 // grid, y1 // grid):
        cy = py * grid + grid / 2.0
        row_idx = (y1 // grid - 1) - py  # top row first in output later; we'll build bottom-up mapping
        for px in range(0, max(w_px, 1)):
            cx = px * grid + grid / 2.0
            inside = False
            # even-odd across contours
            parity = 0
            for contour in g.contours:
                if point_in_poly(cx, cy, contour):
                    parity ^= 1
            if parity:
                if px < w_px:
                    bitmap_bottom_idx = py - (y0 // grid)
                    if 0 <= bitmap_bottom_idx < h_px and 0 <= px < w_px:
                        bitmap[bitmap_bottom_idx][px] = 1
                        any_on = True

    # bitmap currently bottom-to-top rows by font y; convert to top-to-bottom
    top_rows = list(reversed(bitmap))

    # trim vertically and horizontally for tighter BBX? keep full advance width, trim y
    top = 0
    bottom = h_px - 1
    def row_on(r):
        return any(r)
    while top <= bottom and not row_on(top_rows[top]):
        top += 1
    while bottom >= top and not row_on(top_rows[bottom]):
        bottom -= 1
    if top > bottom:
        # empty glyph (e.g. space)
        return (adv_w, 0, 0, 0, [])
    # horizontal trim
    left = 0
    right = w_px - 1 if w_px else 0
    def col_on(c):
        return any((c < len(top_rows[r]) and top_rows[r][c]) for r in range(top, bottom + 1))
    while left <= right and not col_on(left):
        left += 1
    while right >= left and not col_on(right):
        right -= 1
    trimmed = [row[left:right + 1] for row in top_rows[top:bottom + 1]]
    bbx_w = right - left + 1
    bbx_h = bottom - top + 1
    # BDF: BBX w h xoff yoff, xoff/yoff relative to origin baseline-left; yoff = baseline - bottom_of_bbx_in_font_coords
    # font y bottom of em box is descender. Pixel row index from bottom: bidx corresponds to y = descender + bidx*grid
    # top trimmed row index in top_rows is `top`, which corresponds to bottom index = (h_px-1-top)
    bottom_pix_from_em_bottom = (h_px - 1 - bottom)
    baseline_from_em_bottom = -finfo.descender / grid
    yoff = int(round(bottom_pix_from_em_bottom - baseline_from_em_bottom))
    xoff = left
    return (bbx_w, bbx_h, xoff, yoff, trimmed)


def bdf_encode_bitmap(rows: List[List[int]]) -> List[str]:
    if not rows:
        return []
    h = len(rows)
    w = len(rows[0]) if h else 0
    bytes_per_row = (w + 7) // 8
    hex_rows = []
    for r in rows:
        byte_vals = [0] * bytes_per_row
        for x, bit in enumerate(r):
            if bit:
                byte_vals[x // 8] |= (1 << (7 - (x % 8)))
        hex_rows.append("".join(f"{b:02X}" for b in byte_vals))
    return hex_rows


def make_glyph_name(cp: int, fallback: str) -> str:
    if 0x20 <= cp <= 0x7E:
        ch = chr(cp)
        names = {
            ' ': 'space', '!': 'exclam', '"': 'quotedbl', '#': 'numbersign', '$': 'dollar',
            '%': 'percent', '&': 'ampersand', "'": 'quotesingle', '(': 'parenleft', ')': 'parenright',
            '*': 'asterisk', '+': 'plus', ',': 'comma', '-': 'hyphen', '.': 'period', '/': 'slash',
            ':': 'colon', ';': 'semicolon', '<': 'less', '=': 'equal', '>': 'greater', '?': 'question',
            '@': 'at', '[': 'bracketleft', '\\': 'backslash', ']': 'bracketright', '^': 'asciicircum',
            '_': 'underscore', '`': 'grave', '{': 'braceleft', '|': 'bar', '}': 'braceright', '~': 'asciitilde',
        }
        if ch in names:
            return names[ch]
        if ch.isalnum():
            return ch
    return f"uni{cp:04X}" if cp <= 0xFFFF else f"u{cp:06X}"


def write_bdf(ufo: Path, out_bdf: Path, grid: int = 10, only_codepoints: Optional[set] = None) -> Tuple[FontInfo, int]:
    finfo = load_fontinfo(ufo)
    contents = load_contents(ufo)
    glyphs: List[Tuple[int, Glyph, Tuple[int,int,int,int,List[List[int]]]]] = []
    for gname, gfile in contents.items():
        gpath = ufo / "glyphs" / gfile
        if not gpath.exists():
            continue
        g = parse_glif(gpath, gname)
        if not g.codepoints:
            continue
        # Compatibility aliases: expose common symbols under conventional codepoints
        # when the display font uses a stylistic/private mapping.
        cps = list(g.codepoints)
        if 0x1D70B in cps or gname == "piitalic":
            if 0x03C0 not in cps:
                cps.append(0x03C0)  # π
        rast = rasterize_glyph(g, finfo, grid=grid)
        for cp in cps:
            if cp < 0:
                continue
            if only_codepoints is not None and cp not in only_codepoints:
                continue
            glyphs.append((cp, g, rast))
    # de-dup by codepoint, prefer first
    seen = set()
    uniq = []
    for item in sorted(glyphs, key=lambda x: x[0]):
        if item[0] in seen:
            continue
        seen.add(item[0])
        uniq.append(item)

    em_px = finfo.units_per_em // grid
    asc_px = int(round(finfo.ascender / grid))
    desc_px = int(round(-finfo.descender / grid))
    pixel_size = em_px
    point_size = pixel_size * 10

    with out_bdf.open("w", encoding="ascii", newline="\n") as f:
        f.write("STARTFONT 2.1\n")
        f.write(f"FONT {finfo.ps_name}\n")
        f.write(f"SIZE {pixel_size} 75 75\n")
        f.write(f"FONTBOUNDINGBOX {em_px} {em_px} 0 {-desc_px}\n")
        f.write("STARTPROPERTIES 9\n")
        f.write(f"FAMILY_NAME \"{finfo.family}\"\n")
        f.write(f"WEIGHT_NAME \"{finfo.style}\"\n")
        f.write(f"FONT_ASCENT {asc_px}\n")
        f.write(f"FONT_DESCENT {desc_px}\n")
        f.write(f"PIXEL_SIZE {pixel_size}\n")
        f.write(f"POINT_SIZE {point_size}\n")
        f.write(f"RESOLUTION_X 75\n")
        f.write(f"RESOLUTION_Y 75\n")
        f.write("DEFAULT_CHAR 32\n")
        f.write("ENDPROPERTIES\n")
        f.write(f"CHARS {len(uniq)}\n")
        for cp, g, (bw, bh, xoff, yoff, rows) in uniq:
            adv_w = max(0, int(round(g.advance / grid)))
            name = make_glyph_name(cp, g.name)
            f.write(f"STARTCHAR {name}\n")
            f.write(f"ENCODING {cp}\n")
            f.write(f"SWIDTH {int(adv_w * 72000 / max(pixel_size,1))} 0\n")
            f.write(f"DWIDTH {adv_w} 0\n")
            if bh <= 0 or bw <= 0:
                f.write(f"BBX 0 0 0 0\n")
                f.write("BITMAP\n")
            else:
                f.write(f"BBX {bw} {bh} {xoff} {yoff}\n")
                f.write("BITMAP\n")
                for line in bdf_encode_bitmap(rows):
                    f.write(line + "\n")
            f.write("ENDCHAR\n")
        f.write("ENDFONT\n")
    return finfo, len(uniq)


def postprocess_u8g2_c(c_path: Path, font_name: str) -> None:
    """Adjust generated .c to match this repo's conventions when possible."""
    if not c_path.exists():
        return
    text = c_path.read_text(encoding="utf-8", errors="replace")
    # ensure include and section macro like repo fonts
    if '#include "u8g2_replacer.hpp"' not in text:
        # replace common u8g2 includes
        text = re.sub(r'#include\s+"u8g2\.h"\s*', '#include "u8g2_replacer.hpp"\n', text)
        text = re.sub(r'#include\s+<u8g2\.h>\s*', '#include "u8g2_replacer.hpp"\n', text)
        if '#include "u8g2_replacer.hpp"' not in text:
            text = '#include "u8g2_replacer.hpp"\n' + text
    # add U8G2_FONT_SECTION if missing
    if "U8G2_FONT_SECTION" not in text:
        text = re.sub(
            r'(const\s+uint8_t\s+' + re.escape(font_name) + r'\s*\[[^\]]*\])\s*(U8G2_FONT_SECTION\([^)]*\))?\s*=',
            r'\1 U8G2_FONT_SECTION("' + font_name + r'") =',
            text,
        )
    c_path.write_text(text, encoding="utf-8")


def run_bdfconv(bdf: Path, out_c: Path, font_name: str, bdfconv: str, mode: str = "-b 0 -f 1", m_map: Optional[str] = None) -> bool:
    cmd = [bdfconv, "-v"]
    # build actual command
    cmd = [bdfconv]
    if mode:
        cmd += mode.split()
    if m_map:
        cmd += ["-m", m_map]
    cmd += ["-n", font_name, "-o", str(out_c), str(bdf)]
    print("+", " ".join(cmd))
    try:
        subprocess.run(cmd, check=True)
    except Exception as e:
        print(f"bdfconv failed: {e}", file=sys.stderr)
        return False
    postprocess_u8g2_c(out_c, font_name)
    return True


def parse_map_ranges(spec: str) -> set:
    # simple parser for bdfconv-like maps: "32-126,0x00A0-0x00FF"
    out = set()
    for part in spec.split(','):
        part = part.strip()
        if not part:
            continue
        if '-' in part:
            a, b = part.split('-', 1)
            a = int(a, 0); b = int(b, 0)
            for v in range(min(a,b), max(a,b)+1):
                out.add(v)
        else:
            out.add(int(part, 0))
    return out


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description="Convert ClassWiz Display UFO fonts to BDF/u8g2.")
    ap.add_argument("--ufo", action="append", help="Path to a .ufo directory (repeatable).")
    ap.add_argument("--download", action="store_true", help="Download display UFOs from GitHub.")
    ap.add_argument("--cache-dir", default=None, help="Directory for downloaded UFOs (default: temp dir).")
    ap.add_argument("--out-dir", default=".", help="Output directory for generated files.")
    ap.add_argument("--name", default=None, help="Output font/symbol name (single --ufo only).")
    ap.add_argument("--format", choices=["bdf", "c", "both"], default="both", help="Output format.")
    ap.add_argument("--grid", type=int, default=10, help="UFO units per pixel (default: 10).")
    ap.add_argument("--bdfconv", default=None, help="Path to bdfconv executable (default: search PATH).")
    ap.add_argument("--bdfconv-mode", default="-b 0 -f 1", help="Extra bdfconv args (default: '-b 0 -f 1').")
    ap.add_argument("--bdfconv-map", default=None, help="bdfconv -m map/range, e.g. '32-126,160-255'. Limits glyphs encoded into .c and (if given) BDF.")
    ap.add_argument("--all-glyphs", action="store_true", help="Do not filter glyphs (default unless --bdfconv-map given).")
    args = ap.parse_args(argv)

    ufos: List[Path] = [Path(p) for p in (args.ufo or [])]
    cleanup_cache = False
    cache = Path(args.cache_dir) if args.cache_dir else None
    if args.download:
        if cache is None:
            cache = Path(tempfile.mkdtemp(prefix="classwiz_ufo_"))
            cleanup_cache = args.cache_dir is None
        cache.mkdir(parents=True, exist_ok=True)
        for name in UFO_DIRS:
            print(f"Downloading {name} ...")
            ufos.append(download_ufo(name, cache))
    if not ufos:
        ap.error("provide --ufo PATH and/or --download")

    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    bdfconv = args.bdfconv or shutil.which("bdfconv")

    only_cps = None
    if args.bdfconv_map and not args.all_glyphs:
        try:
            only_cps = parse_map_ranges(args.bdfconv_map)
        except Exception as e:
            print(f"Warning: could not parse --bdfconv-map as codepoint filter ({e}); BDF will include all glyphs", file=sys.stderr)
            only_cps = None

    for ufo in ufos:
        if not ufo.exists():
            print(f"UFO not found: {ufo}", file=sys.stderr)
            continue
        finfo = load_fontinfo(ufo)
        font_name = args.name if (args.name and len(ufos) == 1) else "u8g2_font_" + re.sub(r"[^a-zA-Z0-9]+", "_", finfo.ps_name).strip("_").lower()
        bdf_path = out_dir / f"{font_name}.bdf"
        c_path = out_dir / f"{font_name}.c"
        print(f"Converting {ufo} -> {font_name}")
        _, count = write_bdf(ufo, bdf_path, grid=args.grid, only_codepoints=only_cps)
        print(f"  Wrote {bdf_path} ({count} glyphs)")
        if args.format in ("c", "both"):
            if bdfconv:
                ok = run_bdfconv(bdf_path, c_path, font_name, bdfconv, mode=args.bdfconv_mode, m_map=args.bdfconv_map)
                if ok:
                    print(f"  Wrote {c_path}")
            else:
                print("  bdfconv not found; skipped .c generation. Install u8g2 tools (bdfconv) or pass --bdfconv PATH.")
                print(f"  You can generate .c manually, e.g.: bdfconv -b 0 -f 1 -n {font_name} -o {c_path} {bdf_path}")
        if args.format == "c" and bdf_path.exists() and bdfconv:
            # keep bdf as intermediate by default? leave it; useful for debugging
            pass
        if args.format == "c" and not bdfconv:
            # keep bdf since it's the main artifact produced
            pass

    if cleanup_cache and cache and cache.exists():
        # temp cache created by us
        shutil.rmtree(cache, ignore_errors=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
