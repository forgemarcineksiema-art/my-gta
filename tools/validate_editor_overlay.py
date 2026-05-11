#!/usr/bin/env python3
"""Validate runtime editor overlay JSON before it reaches the game."""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path


KNOWN_ZONES = {"Unknown", "Block", "Shop", "Parking", "Garage", "Trash", "RoadLoop"}


def load_manifest_ids(asset_root: Path) -> set[str]:
    manifest = asset_root / "asset_manifest.txt"
    ids: set[str] = set()
    for raw_line in manifest.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        ids.add(line.split("|", 1)[0].strip())
    return ids


def finite_vec3(value: object) -> bool:
    return (
        isinstance(value, list)
        and len(value) == 3
        and all(isinstance(item, (int, float)) and math.isfinite(float(item)) for item in value)
    )


def validate_object(kind: str, obj: object, manifest_ids: set[str], issues: list[str]) -> str:
    if not isinstance(obj, dict):
        issues.append(f"{kind}: object entry must be a JSON object")
        return ""

    object_id = str(obj.get("id", "")).strip()
    asset_id = str(obj.get("assetId", "")).strip()
    if not object_id:
        issues.append(f"{kind}: id must be non-empty")
    if asset_id not in manifest_ids:
        issues.append(f"{kind} '{object_id}': unknown assetId '{asset_id}'")
    if not finite_vec3(obj.get("position")):
        issues.append(f"{kind} '{object_id}': position must be a finite vec3")
    if not finite_vec3(obj.get("scale")):
        issues.append(f"{kind} '{object_id}': scale must be a finite vec3")
    elif any(float(item) <= 0.0 for item in obj["scale"]):
        issues.append(f"{kind} '{object_id}': scale values must be positive")

    yaw = obj.get("yawRadians", 0.0)
    if not isinstance(yaw, (int, float)) or not math.isfinite(float(yaw)):
        issues.append(f"{kind} '{object_id}': yawRadians must be finite")

    zone = str(obj.get("zoneTag", "Unknown"))
    if zone not in KNOWN_ZONES:
        issues.append(f"{kind} '{object_id}': unknown zoneTag '{zone}'")

    tags = obj.get("gameplayTags", [])
    if not isinstance(tags, list) or any(not isinstance(tag, str) for tag in tags):
        issues.append(f"{kind} '{object_id}': gameplayTags must be a string array")
    return object_id


def validate_overlay(path: Path, asset_root: Path) -> list[str]:
    manifest_ids = load_manifest_ids(asset_root)
    issues: list[str] = []
    data = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        return ["overlay root must be a JSON object"]
    if data.get("schemaVersion") != 1:
        issues.append("schemaVersion must be 1")

    ids: set[str] = set()
    for kind in ("overrides", "instances"):
        entries = data.get(kind, [])
        if not isinstance(entries, list):
            issues.append(f"{kind} must be an array")
            continue
        for entry in entries:
            object_id = validate_object(kind[:-1], entry, manifest_ids, issues)
            if object_id:
                key = f"{kind}:{object_id}"
                if key in ids:
                    issues.append(f"{kind}: duplicate id '{object_id}'")
                ids.add(key)

    if "disabledBaseObjects" in data:
        issues.append("disabledBaseObjects is not supported in editor overlay schema v1")
    return issues


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
