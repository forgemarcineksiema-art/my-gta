#!/usr/bin/env python3
"""Validate runtime editor overlay JSON before it reaches the game."""

from __future__ import annotations

import argparse
from pathlib import Path

from validate_world_contract import load_manifest, validate_overlay as validate_world_overlay


def validate_overlay(path: Path, asset_root: Path) -> list[str]:
    entries, manifest_issues = load_manifest(asset_root)
    return [*manifest_issues, *validate_world_overlay(path, entries)]


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate Blok 13 editor overlay JSON.")
    parser.add_argument("overlay", nargs="?", default="data/world/block13_editor_overlay.json", type=Path)
    parser.add_argument("--asset-root", default="data/assets", type=Path)
    args = parser.parse_args()

    issues = validate_overlay(args.overlay, args.asset_root)
    if issues:
        for issue in issues:
            print(f"error: {issue}")
        return 1
    print(f"editor overlay validation passed: {args.overlay}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
