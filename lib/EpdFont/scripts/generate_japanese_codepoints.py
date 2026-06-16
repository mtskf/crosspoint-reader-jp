#!/usr/bin/env python3
"""Generate the list of Unicode codepoints used to constrain the built-in CJK UI font.

Pulls JIS X 0208/0212/0213 kanji from the Unicode Consortium's Unihan database
(kJis0 / kJis1 / kJIS0213 fields) and unions them with fixed ranges for hiragana,
katakana, punctuation, and the replacement character. Output is a sorted, plain-text
list of hex codepoints, one per line, consumed by scripts/generate_cjk_ui_font.py.
"""

import argparse
import io
import os
import sys
import zipfile
import urllib.request
from datetime import datetime
from pathlib import Path

# --- Defaults ---
UNIHAN_ZIP_URL = "https://www.unicode.org/Public/UCD/latest/ucd/Unihan.zip"
SCRIPT_DIR = Path(__file__).resolve().parent
DEFAULT_OUTPUT = SCRIPT_DIR / "codepoints" / "japanese_jis0213.txt"
DEFAULT_UNIHAN_ZIP = Path("/tmp/Unihan.zip")

# --- Fixed codepoint ranges ---
FIXED_RANGES = [
    (0x0020, 0x007E),  # ASCII printable
    (0x0080, 0x00FF),  # Latin-1 Supplement
    (0x2000, 0x206F),  # General Punctuation
    (0x3000, 0x303F),  # CJK Symbols and Punctuation
    (0x3040, 0x309F),  # Hiragana
    (0x30A0, 0x30FF),  # Katakana
    (0x31F0, 0x31FF),  # Katakana Phonetic Extensions
    (0xFF00, 0xFFEF),  # Halfwidth and Fullwidth Forms
]

SINGLE_CODEPOINTS = [
    0xFFFD,  # Replacement character
]


def download_unihan(zip_path: Path, force: bool = False) -> None:
    """Download Unihan.zip (skip if the file already exists unless --force)."""
    if zip_path.exists() and not force:
        print(f"Using existing {zip_path}")
        return
    print(f"Downloading Unihan.zip: {UNIHAN_ZIP_URL}")
    urllib.request.urlretrieve(UNIHAN_ZIP_URL, str(zip_path))
    print(f"Download complete: {zip_path}")


def extract_jis_codepoints(zip_path: Path) -> set[int]:
    """Extract codepoints whose Unihan record carries any of the JIS mappings we ship:
    kJis0 (JIS X 0208), kJis1 (JIS X 0212), or kJIS0213 (JIS X 0213).

    kJis1 is technically outside JIS X 0213, but those kanji are common in real-world
    Japanese text so we include them in the UI font's coverage set.
    """
    target_fields = {"kJis0", "kJis1", "kJIS0213"}
    codepoints: set[int] = set()

    with zipfile.ZipFile(str(zip_path), "r") as zf:
        # Locate Unihan_OtherMappings.txt inside the archive.
        mapping_file = None
        for name in zf.namelist():
            if "Unihan_OtherMappings" in name:
                mapping_file = name
                break

        if mapping_file is None:
            print("Error: Unihan_OtherMappings.txt not found.", file=sys.stderr)
            print(f"Archive contents: {zf.namelist()}", file=sys.stderr)
            sys.exit(1)

        print(f"Parsing: {mapping_file}")
        with zf.open(mapping_file) as f:
            for raw_line in io.TextIOWrapper(f, encoding="utf-8"):
                line = raw_line.strip()
                if not line or line.startswith("#"):
                    continue
                parts = line.split("\t")
                if len(parts) >= 2 and parts[1] in target_fields:
                    # Codepoint is the first column, formatted as "U+XXXX".
                    cp_str = parts[0]
                    if cp_str.startswith("U+"):
                        cp = int(cp_str[2:], 16)
                        codepoints.add(cp)

    return codepoints


def collect_fixed_codepoints() -> set[int]:
    """Collect every codepoint in FIXED_RANGES and SINGLE_CODEPOINTS."""
    codepoints: set[int] = set()
    for start, end in FIXED_RANGES:
        for cp in range(start, end + 1):
            codepoints.add(cp)
    for cp in SINGLE_CODEPOINTS:
        codepoints.add(cp)
    return codepoints


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT,
                        help=f"Output codepoints file (default: {DEFAULT_OUTPUT})")
    parser.add_argument("--unihan-zip", type=Path, default=DEFAULT_UNIHAN_ZIP,
                        help=f"Local path to Unihan.zip (default: {DEFAULT_UNIHAN_ZIP})")
    parser.add_argument("--force-download", action="store_true",
                        help="Re-download Unihan.zip even if --unihan-zip exists")
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    # 1. Download Unihan.zip.
    download_unihan(args.unihan_zip, force=args.force_download)

    # 2. Extract JIS kanji codepoints.
    jis_codepoints = extract_jis_codepoints(args.unihan_zip)
    print(f"JIS X 0208/0212/0213 kanji: {len(jis_codepoints)} chars")

    # 3. Collect fixed-range codepoints.
    fixed_codepoints = collect_fixed_codepoints()
    print(f"Fixed ranges: {len(fixed_codepoints)} chars")

    # 4. Merge and sort.
    all_codepoints = sorted(jis_codepoints | fixed_codepoints)
    total = len(all_codepoints)
    print(f"Total: {total} chars")

    # 5. Write output.
    args.output.parent.mkdir(parents=True, exist_ok=True)
    now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    with open(args.output, "w", encoding="utf-8") as f:
        f.write(f"# JIS X 0213 (2004) codepoint list\n")
        f.write(f"# Generated: {now}\n")
        f.write(f"# Char count: {total}\n")
        f.write(f"# Coverage: ASCII, Latin-1 Supplement, General Punctuation,\n")
        f.write(f"#   CJK Symbols and Punctuation, Hiragana, Katakana,\n")
        f.write(f"#   Katakana Phonetic Extensions, Halfwidth and Fullwidth Forms,\n")
        f.write(f"#   Replacement Character (U+FFFD),\n")
        f.write(f"#   JIS X 0208/0212/0213 kanji (Unihan kJis0/kJis1/kJIS0213)\n")
        f.write(f"#\n")
        for cp in all_codepoints:
            f.write(f"{cp:04X}\n")

    print(f"Wrote: {args.output}")


if __name__ == "__main__":
    main()
