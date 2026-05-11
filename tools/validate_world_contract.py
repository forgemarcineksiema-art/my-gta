#!/usr/bin/env python3
"""Validate shared world-production contracts across data, manifest, and editor overlay."""

from __future__ import annotations

import argparse
import json
import math
from dataclasses import dataclass
from pathlib import Path
from typing import Any


EPSILON = 0.0001
KNOWN_ZONES = {"Unknown", "Block", "Shop", "Parking", "Garage", "Trash", "RoadLoop"}
KNOWN_RENDER_BUCKETS = {"Opaque", "Decal", "Glass", "Translucent", "DebugOnly", "Vehicle"}
KNOWN_COLLISION_INTENTS = {
    "Unspecified",
    "None",
    "Ground",
    "Prop",
    "SolidWorld",
    "DynamicProp",
    "VehicleDynamic",
    "CollisionProxy",
}
KNOWN_ASSET_TYPES = {
    "SolidBuilding",
    "SolidProp",
    "DynamicProp",
    "Decal",
    "Glass",
    "Marker",
    "DebugOnly",
    "VehicleRender",
    "CollisionProxy",
}
HERO_SHOP_ASSET_ID = "shop_zenona_v3"
HERO_SHOP_OBJECT_ID = "shop_zenona"
LEGACY_SHOP_ASSET_IDS = {"shop_zenona", "shop_zenona_v2"}
REQUIRED_SHOP_OUTCOME_IDS = {"shop_door_checked", "shop_rolling_grille_checked"}
REQUIRED_SHOP_OUTCOME_PATTERNS = {"shop_prices_read_*", "shop_notice_read_*"}
NOISY_SHOP_OUTCOME_IDS = {"shop_door_checked", "shop_rolling_grille_checked"}
QUIET_SHOP_OUTCOME_PATTERNS = {"shop_prices_read_*", "shop_notice_read_*"}


@dataclass(frozen=True)
class ManifestEntry:
    line: int
    asset_id: str
    fallback_color: tuple[int, int, int, int]
    metadata: dict[str, str]

    @property
    def asset_type(self) -> str:
        return self.metadata.get("type", "SolidProp")

    @property
    def usage(self) -> str:
        return self.metadata.get("usage", "active")

    @property
    def origin(self) -> str:
        return self.metadata.get("origin", "bottom_center")

    @property
    def render_bucket(self) -> str:
        if "render" in self.metadata:
            return self.metadata["render"]
        return "Translucent" if self.fallback_color[3] < 255 else "Opaque"

    @property
    def collision(self) -> str:
        return self.metadata.get("collisionProfile", self.metadata.get("collision", "Unspecified"))

    def bool_metadata(self, key: str, default: bool = False) -> bool:
        return self.metadata.get(key, str(default).lower()).lower() == "true"

    def tags(self) -> set[str]:
        return {tag.strip() for tag in self.metadata.get("tags", "").split(",") if tag.strip()}


@dataclass(frozen=True)
class WorldObjectRecord:
    source: str
    object_id: str
    asset_id: str
    position: list[float] | None
    scale: list[float] | None
    yaw: float | None
    zone: str
    tags: list[str] | None


def load_json(path: Path) -> Any:
    if not path.exists():
        raise FileNotFoundError(f"missing file: {path}")
    return json.loads(path.read_text(encoding="utf-8"))


def parse_color(value: str) -> tuple[int, int, int, int]:
    parts = [part.strip() for part in value.split(",")]
    if len(parts) not in (3, 4):
        raise ValueError("expected RGB or RGBA color")
    channels = tuple(max(0, min(255, int(part))) for part in parts)
    return channels if len(channels) == 4 else (*channels, 255)


def parse_metadata(value: str) -> dict[str, str]:
    metadata: dict[str, str] = {}
    for raw_entry in value.split(";"):
        entry = raw_entry.strip()
        if not entry:
            continue
        if "=" not in entry:
            raise ValueError(f"bad metadata entry '{entry}'")
        key, metadata_value = entry.split("=", 1)
        metadata[key.strip()] = metadata_value.strip()
    return metadata


def load_manifest(asset_root: Path) -> tuple[dict[str, ManifestEntry], list[str]]:
    manifest_path = asset_root / "asset_manifest.txt"
    issues: list[str] = []
    entries: dict[str, ManifestEntry] = {}
    if not manifest_path.exists():
        return entries, [f"missing asset manifest: {manifest_path}"]

    for line_number, raw_line in enumerate(manifest_path.read_text(encoding="utf-8").splitlines(), start=1):
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        parts = [part.strip() for part in line.split("|")]
        if len(parts) not in (4, 5):
            issues.append(f"{manifest_path}:{line_number}: expected id|modelPath|fallbackSize|fallbackColor|metadata")
            continue
        asset_id = parts[0]
        if not asset_id:
            issues.append(f"{manifest_path}:{line_number}: asset id must be non-empty")
            continue
        if asset_id in entries:
            issues.append(f"{manifest_path}:{line_number}: duplicate asset id '{asset_id}'")
            continue
        try:
            fallback_color = parse_color(parts[3])
            metadata = parse_metadata(parts[4]) if len(parts) == 5 else {}
        except ValueError as error:
            issues.append(f"{manifest_path}:{line_number}: {error}")
            continue
        entries[asset_id] = ManifestEntry(line_number, asset_id, fallback_color, metadata)
    return entries, issues


def is_finite_number(value: object) -> bool:
    return isinstance(value, (int, float)) and math.isfinite(float(value))


def finite_vec3(value: object) -> list[float] | None:
    if not isinstance(value, list) or len(value) != 3:
        return None
    if not all(is_finite_number(item) for item in value):
        return None
    return [float(item) for item in value]


def positive_vec3(value: list[float]) -> bool:
    return all(item > 0.0 for item in value)


def object_from_data(source: str, obj: object) -> tuple[WorldObjectRecord | None, list[str]]:
    issues: list[str] = []
    if not isinstance(obj, dict):
        return None, [f"{source}: object entry must be a JSON object"]

    object_id = str(obj.get("id", "")).strip()
    asset_id = str(obj.get("assetId", "")).strip()
    if not object_id:
        issues.append(f"{source}: id must be non-empty")
    if not asset_id:
        issues.append(f"{source} '{object_id}': assetId must be non-empty")

    position = finite_vec3(obj.get("position"))
    if position is None:
        issues.append(f"{source} '{object_id}': position must be a finite vec3")

    scale = finite_vec3(obj.get("scale", [1.0, 1.0, 1.0]))
    if scale is None:
        issues.append(f"{source} '{object_id}': scale must be a finite vec3")
    elif not positive_vec3(scale):
        issues.append(f"{source} '{object_id}': scale values must be positive")

    yaw_value = obj.get("yawRadians", obj.get("yaw", 0.0))
    yaw: float | None = None
    if not is_finite_number(yaw_value):
        issues.append(f"{source} '{object_id}': yaw/yawRadians must be finite")
    else:
        yaw = float(yaw_value)

    zone = str(obj.get("zoneTag", obj.get("zone", "Unknown"))).strip() or "Unknown"
    if zone not in KNOWN_ZONES:
        issues.append(f"{source} '{object_id}': unknown zone '{zone}'")

    raw_tags = obj.get("gameplayTags", obj.get("tags", []))
    tags: list[str] | None = None
    if not isinstance(raw_tags, list) or any(not isinstance(tag, str) or not tag.strip() for tag in raw_tags):
        issues.append(f"{source} '{object_id}': tags/gameplayTags must be a non-empty string array")
    else:
        tags = [tag.strip() for tag in raw_tags]
        if len(tags) != len(set(tags)):
            issues.append(f"{source} '{object_id}': duplicate tags are not allowed")

    if issues:
        return None, issues
    return WorldObjectRecord(source, object_id, asset_id, position, scale, yaw, zone, tags), issues


def validate_manifest_contract(entries: dict[str, ManifestEntry]) -> list[str]:
    issues: list[str] = []
    for entry in entries.values():
        prefix = f"asset '{entry.asset_id}'"
        if entry.asset_type not in KNOWN_ASSET_TYPES:
            issues.append(f"{prefix}: unknown type '{entry.asset_type}'")
        if entry.render_bucket not in KNOWN_RENDER_BUCKETS:
            issues.append(f"{prefix}: unknown render bucket '{entry.render_bucket}'")
        if entry.collision not in KNOWN_COLLISION_INTENTS:
            issues.append(f"{prefix}: unknown collision intent '{entry.collision}'")
        if entry.origin != "bottom_center" and not entry.bool_metadata("allowFloating"):
            issues.append(f"{prefix}: non-bottom_center origin must declare allowFloating=true")
        if entry.asset_type == "SolidBuilding":
            if entry.render_bucket != "Opaque":
                issues.append(f"{prefix}: SolidBuilding must render as Opaque")
            if not entry.bool_metadata("cameraBlocks"):
                issues.append(f"{prefix}: SolidBuilding must set cameraBlocks=true")
            if not entry.bool_metadata("requiresClosedMesh"):
                issues.append(f"{prefix}: SolidBuilding must set requiresClosedMesh=true")
            if entry.bool_metadata("allowOpenEdges"):
                issues.append(f"{prefix}: SolidBuilding cannot allow open edges")
        if entry.asset_type == "Decal":
            if entry.render_bucket != "Decal":
                issues.append(f"{prefix}: Decal must use render=Decal")
            if entry.collision != "None":
                issues.append(f"{prefix}: Decal must use collision=None")
            if entry.bool_metadata("cameraBlocks"):
                issues.append(f"{prefix}: Decal must set cameraBlocks=false")
            if "maxHeight" not in entry.metadata:
                issues.append(f"{prefix}: Decal must declare maxHeight")
        if entry.asset_type == "Glass":
            if entry.render_bucket != "Glass":
                issues.append(f"{prefix}: Glass must use render=Glass")
            if entry.collision != "None":
                issues.append(f"{prefix}: Glass must use collision=None")
            if entry.bool_metadata("cameraBlocks"):
                issues.append(f"{prefix}: Glass must set cameraBlocks=false")
        if entry.asset_type in {"DebugOnly", "CollisionProxy"} or entry.render_bucket == "DebugOnly":
            if entry.render_bucket != "DebugOnly":
                issues.append(f"{prefix}: debug/collision proxy assets must use render=DebugOnly")
            if entry.bool_metadata("renderInGameplay", True):
                issues.append(f"{prefix}: debug/collision proxy assets must set renderInGameplay=false")
    return issues


def validate_hero_shop_manifest(entries: dict[str, ManifestEntry]) -> list[str]:
    issues: list[str] = []
    entry = entries.get(HERO_SHOP_ASSET_ID)
    if entry is None:
        return [f"hero shop asset '{HERO_SHOP_ASSET_ID}' must exist in asset manifest"]

    prefix = f"hero shop asset '{HERO_SHOP_ASSET_ID}'"
    if entry.usage != "active":
        issues.append(f"{prefix}: usage must be active")
    if entry.asset_type != "SolidBuilding":
        issues.append(f"{prefix}: type must be SolidBuilding")
    if entry.origin != "bottom_center":
        issues.append(f"{prefix}: origin must be bottom_center")
    if entry.render_bucket != "Opaque":
        issues.append(f"{prefix}: render must be Opaque")
    if entry.collision != "SolidWorld":
        issues.append(f"{prefix}: collisionProfile must be SolidWorld")
    if not entry.bool_metadata("cameraBlocks"):
        issues.append(f"{prefix}: cameraBlocks must be true")
    if not entry.bool_metadata("requiresClosedMesh"):
        issues.append(f"{prefix}: requiresClosedMesh must be true")
    if entry.bool_metadata("allowOpenEdges"):
        issues.append(f"{prefix}: allowOpenEdges must be false")
    if not entry.bool_metadata("renderInGameplay", True):
        issues.append(f"{prefix}: renderInGameplay must stay true")
    for tag in ("building", "shop", "hero_asset"):
        if tag not in entry.tags():
            issues.append(f"{prefix}: tags must include {tag}")

    for legacy_id in LEGACY_SHOP_ASSET_IDS:
        legacy = entries.get(legacy_id)
        if legacy is None:
            issues.append(f"legacy shop asset '{legacy_id}' must stay declared for migration safety")
            continue
        legacy_prefix = f"legacy shop asset '{legacy_id}'"
        if legacy.asset_type != "DebugOnly" or legacy.render_bucket != "DebugOnly":
            issues.append(f"{legacy_prefix}: legacy shop assets must be DebugOnly")
        if legacy.bool_metadata("renderInGameplay", True):
            issues.append(f"{legacy_prefix}: legacy shop assets must set renderInGameplay=false")
        if legacy.collision != "None":
            issues.append(f"{legacy_prefix}: legacy shop assets must use collision=None")
    return issues


def validate_world_object(record: WorldObjectRecord, entries: dict[str, ManifestEntry]) -> list[str]:
    issues: list[str] = []
    entry = entries.get(record.asset_id)
    if entry is None:
        return [f"{record.source} '{record.object_id}': unknown assetId '{record.asset_id}'"]

    prefix = f"{record.source} '{record.object_id}'"
    if entry.asset_type == "DebugOnly" or entry.render_bucket == "DebugOnly" or not entry.bool_metadata("renderInGameplay", True):
        issues.append(f"{prefix}: gameplay world object cannot use non-gameplay asset '{record.asset_id}'")
    if entry.asset_type == "CollisionProxy" and record.source.startswith("overlay"):
        issues.append(f"{prefix}: editor overlay must not place CollisionProxy assets as visual objects")
    if entry.origin == "bottom_center" and record.position is not None and record.position[1] < -EPSILON:
        issues.append(f"{prefix}: bottom_center position must not be below ground")
    if entry.origin != "bottom_center" and not entry.bool_metadata("allowFloating"):
        issues.append(f"{prefix}: asset origin exception requires allowFloating=true")
    if record.position is not None and record.position[1] > EPSILON and not entry.bool_metadata("allowFloating"):
        issues.append(f"{prefix}: elevated placement requires asset metadata allowFloating=true")
    if entry.asset_type == "SolidBuilding" and not entry.bool_metadata("cameraBlocks"):
        issues.append(f"{prefix}: SolidBuilding instance requires cameraBlocks=true on asset")
    if entry.asset_type == "Decal" and entry.collision != "None":
        issues.append(f"{prefix}: Decal instance must not use gameplay collision")
    if record.object_id == HERO_SHOP_OBJECT_ID:
        if record.asset_id != HERO_SHOP_ASSET_ID:
            issues.append(f"{prefix}: hero shop object must use assetId '{HERO_SHOP_ASSET_ID}'")
        if record.zone != "Shop":
            issues.append(f"{prefix}: hero shop object must use zone Shop")
        if record.tags is not None:
            for tag in ("mission", "shop_trouble", "landmark", "hero_asset"):
                if tag not in record.tags:
                    issues.append(f"{prefix}: hero shop object tags must include {tag}")
    if record.asset_id in LEGACY_SHOP_ASSET_IDS:
        issues.append(f"{prefix}: gameplay world object must not use legacy shop asset '{record.asset_id}'")
    return issues


def validate_map(data_root: Path, entries: dict[str, ManifestEntry]) -> list[str]:
    path = data_root / "map_block_loop.json"
    issues: list[str] = []
    data = load_json(path)
    if not isinstance(data, dict):
        return [f"{path}: map root must be a JSON object"]
    if not str(data.get("id", "")).strip():
        issues.append(f"{path}: id must be non-empty")

    spawn = data.get("spawn")
    if not isinstance(spawn, dict):
        issues.append(f"{path}: spawn must be an object")
    else:
        for key in ("player", "vehicle", "npc", "shop", "dropoff"):
            value = finite_vec3(spawn.get(key))
            if value is None:
                issues.append(f"{path}: spawn.{key} must be a finite vec3")
                continue
            if abs(value[1]) > EPSILON:
                issues.append(f"{path}: spawn.{key}.y must be 0.0 under the world contract")

    performance = data.get("performanceTarget")
    if performance is not None:
        if not isinstance(performance, dict):
            issues.append(f"{path}: performanceTarget must be an object")
        else:
            fps = performance.get("fps")
            if fps is not None and (not is_finite_number(fps) or float(fps) <= 0.0):
                issues.append(f"{path}: performanceTarget.fps must be positive")

    raw_objects = data.get("objects", [])
    if raw_objects is not None:
        if not isinstance(raw_objects, list):
            issues.append(f"{path}: objects must be an array when present")
        else:
            seen: set[str] = set()
            for index, raw_object in enumerate(raw_objects):
                record, object_issues = object_from_data(f"map object[{index}]", raw_object)
                issues.extend(object_issues)
                if record is None:
                    continue
                if record.object_id in seen:
                    issues.append(f"{path}: duplicate map object id '{record.object_id}'")
                seen.add(record.object_id)
                issues.extend(validate_world_object(record, entries))
    return issues


def validate_overlay(overlay_path: Path, entries: dict[str, ManifestEntry]) -> list[str]:
    data = load_json(overlay_path)
    issues: list[str] = []
    if not isinstance(data, dict):
        return [f"{overlay_path}: overlay root must be a JSON object"]
    if data.get("schemaVersion") != 1:
        issues.append(f"{overlay_path}: schemaVersion must be 1")
    if "disabledBaseObjects" in data:
        issues.append(f"{overlay_path}: disabledBaseObjects is not supported in schema v1")

    seen: set[str] = set()
    for kind in ("overrides", "instances"):
        entries_raw = data.get(kind, [])
        if not isinstance(entries_raw, list):
            issues.append(f"{overlay_path}: {kind} must be an array")
            continue
        for index, raw_object in enumerate(entries_raw):
            record, object_issues = object_from_data(f"overlay {kind[:-1]}[{index}]", raw_object)
            issues.extend(object_issues)
            if record is None:
                continue
            if record.object_id in seen:
                issues.append(f"{overlay_path}: duplicate overlay object id '{record.object_id}'")
            seen.add(record.object_id)
            issues.extend(validate_world_object(record, entries))
    return issues


def validate_shop_outcome_catalog(data_root: Path) -> list[str]:
    path = data_root / "world" / "object_outcome_catalog.json"
    if not path.exists():
        return [f"{path}: object outcome catalog is required for hero shop interactions"]

    issues: list[str] = []
    data = load_json(path)
    if not isinstance(data, dict):
        return [f"{path}: object outcome catalog root must be a JSON object"]
    outcomes = data.get("outcomes")
    if not isinstance(outcomes, list):
        return [f"{path}: outcomes must be an array"]

    ids: set[str] = set()
    patterns: set[str] = set()
    locations_by_key: dict[str, str] = {}
    world_events_by_key: dict[str, object] = {}
    for outcome in outcomes:
        if not isinstance(outcome, dict):
            continue
        outcome_id = str(outcome.get("id", "")).strip()
        pattern = str(outcome.get("idPattern", "")).strip()
        key = outcome_id or pattern
        if not key:
            continue
        if outcome_id:
            ids.add(outcome_id)
        if pattern:
            patterns.add(pattern)
        locations_by_key[key] = str(outcome.get("location", "")).strip()
        if "worldEvent" in outcome:
            world_events_by_key[key] = outcome.get("worldEvent")

    for required_id in REQUIRED_SHOP_OUTCOME_IDS:
        if required_id not in ids:
            issues.append(f"{path}: hero shop outcome '{required_id}' is required")
        elif locations_by_key.get(required_id) != "Shop":
            issues.append(f"{path}: hero shop outcome '{required_id}' must use location Shop")
    for required_pattern in REQUIRED_SHOP_OUTCOME_PATTERNS:
        if required_pattern not in patterns:
            issues.append(f"{path}: hero shop outcome pattern '{required_pattern}' is required")
        elif locations_by_key.get(required_pattern) != "Shop":
            issues.append(f"{path}: hero shop outcome pattern '{required_pattern}' must use location Shop")
    for noisy_id in NOISY_SHOP_OUTCOME_IDS:
        event = world_events_by_key.get(noisy_id)
        if not isinstance(event, dict):
            issues.append(f"{path}: hero shop outcome '{noisy_id}' must declare worldEvent metadata")
            continue
        if str(event.get("type", "")).strip() != "PublicNuisance":
            issues.append(f"{path}: hero shop outcome '{noisy_id}' worldEvent type must be PublicNuisance")
        intensity = event.get("intensity")
        if not is_finite_number(intensity) or not (0.0 < float(intensity) <= 0.25):
            issues.append(f"{path}: hero shop outcome '{noisy_id}' worldEvent intensity must be low-stakes")
        cooldown = event.get("cooldownSeconds")
        if not is_finite_number(cooldown) or float(cooldown) < 2.0:
            issues.append(f"{path}: hero shop outcome '{noisy_id}' worldEvent cooldownSeconds must prevent E-spam")
    for quiet_pattern in QUIET_SHOP_OUTCOME_PATTERNS:
        if quiet_pattern in world_events_by_key:
            issues.append(f"{path}: quiet hero shop outcome pattern '{quiet_pattern}' must not declare worldEvent")
    return issues


def validate_world_contract(data_root: Path, asset_root: Path, overlay_path: Path | None = None) -> list[str]:
    entries, manifest_issues = load_manifest(asset_root)
    issues = list(manifest_issues)
    issues.extend(validate_manifest_contract(entries))
    issues.extend(validate_hero_shop_manifest(entries))
    issues.extend(validate_map(data_root, entries))
    issues.extend(validate_shop_outcome_catalog(data_root))
    resolved_overlay = overlay_path if overlay_path is not None else data_root / "world" / "block13_editor_overlay.json"
    if resolved_overlay.exists():
        issues.extend(validate_overlay(resolved_overlay, entries))
    return issues


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate Blok 13 shared world-production contract.")
    parser.add_argument("data_root", nargs="?", default="data", type=Path)
    parser.add_argument("--asset-root", default=None, type=Path)
    parser.add_argument("--overlay", default=None, type=Path)
    parser.add_argument("--quiet", action="store_true", help="Only print failures.")
    args = parser.parse_args()

    asset_root = args.asset_root if args.asset_root is not None else args.data_root / "assets"
    issues = validate_world_contract(args.data_root, asset_root, args.overlay)
    if issues:
        for issue in issues:
            print(f"error: {issue}")
        return 1
    if not args.quiet:
        print(f"world contract validation passed: {args.data_root}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
