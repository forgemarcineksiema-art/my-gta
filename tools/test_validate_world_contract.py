#!/usr/bin/env python3
"""Unit tests for validate_world_contract.py."""

from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from validate_world_contract import validate_world_contract


def write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


def write_json(path: Path, value: object) -> None:
    write_text(path, json.dumps(value, indent=2))


def valid_map() -> dict[str, object]:
    return {
        "id": "test_loop",
        "spawn": {
            "player": [0.0, 0.0, 0.0],
            "vehicle": [2.0, 0.0, 0.0],
            "npc": [0.0, 0.0, 2.0],
            "shop": [5.0, 0.0, -5.0],
            "dropoff": [1.0, 0.0, 1.0],
        },
        "performanceTarget": {"fps": 60},
    }


def overlay(instances: list[dict[str, object]]) -> dict[str, object]:
    return {"schemaVersion": 1, "overrides": [], "instances": instances}


def valid_shop_outcome_catalog() -> dict[str, object]:
    return {
        "schemaVersion": 1,
        "outcomes": [
            {
                "id": "shop_door_checked",
                "location": "Shop",
                "worldEvent": {"type": "PublicNuisance", "intensity": 0.12, "cooldownSeconds": 4.0},
            },
            {
                "id": "shop_rolling_grille_checked",
                "location": "Shop",
                "worldEvent": {"type": "PublicNuisance", "intensity": 0.14, "cooldownSeconds": 3.5},
            },
            {"idPattern": "shop_prices_read_*", "location": "Shop"},
            {"idPattern": "shop_notice_read_*", "location": "Shop"},
        ],
    }


def hero_shop_manifest_lines() -> list[str]:
    return [
        "shop_zenona|models/shop.obj|1,1,1|180,180,180|type=DebugOnly;usage=legacy;origin=bottom_center;render=DebugOnly;collision=None;renderInGameplay=false;allowOpenEdges=true;tags=legacy,shop",
        "shop_zenona_v2|models/shop_v2.obj|1,1,1|180,180,180|type=DebugOnly;usage=legacy;origin=bottom_center;render=DebugOnly;collision=None;renderInGameplay=false;allowOpenEdges=true;tags=legacy,shop",
        "shop_zenona_v3|models/shop_v3.obj|1,1,1|180,180,180|type=SolidBuilding;usage=active;origin=bottom_center;render=Opaque;collisionProfile=SolidWorld;cameraBlocks=true;requiresClosedMesh=true;allowOpenEdges=false;tags=building,shop,hero_asset",
    ]


class WorldContractValidatorTests(unittest.TestCase):
    def validate(self, manifest_lines: list[str], overlay_instances: list[dict[str, object]] | None = None,
                 map_data: dict[str, object] | None = None,
                 outcome_catalog: dict[str, object] | None = None) -> list[str]:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            data_root = root / "data"
            asset_root = data_root / "assets"
            write_text(asset_root / "asset_manifest.txt", "\n".join(manifest_lines) + "\n")
            write_json(data_root / "map_block_loop.json", map_data if map_data is not None else valid_map())
            write_json(data_root / "world" / "block13_editor_overlay.json", overlay(overlay_instances or []))
            write_json(data_root / "world" / "object_outcome_catalog.json",
                       outcome_catalog if outcome_catalog is not None else valid_shop_outcome_catalog())
            return validate_world_contract(data_root, asset_root)

    def test_valid_minimal_world_passes(self) -> None:
        issues = self.validate(
            [
                *hero_shop_manifest_lines(),
                "bench|models/bench.obj|1,1,1|180,180,180|type=SolidProp;origin=bottom_center;render=Opaque;collision=Prop",
            ],
            [
                {
                    "id": "bench_instance",
                    "assetId": "bench",
                    "position": [0.0, 0.0, 0.0],
                    "scale": [1.0, 1.0, 1.0],
                    "yawRadians": 0.0,
                    "zoneTag": "Block",
                    "gameplayTags": ["seating"],
                }
            ],
        )

        self.assertEqual([], issues)

    def test_overlay_unknown_asset_fails(self) -> None:
        issues = self.validate(
            [
                *hero_shop_manifest_lines(),
                "bench|models/bench.obj|1,1,1|180,180,180|type=SolidProp;origin=bottom_center;render=Opaque;collision=Prop",
            ],
            [{"id": "ghost", "assetId": "missing", "position": [0, 0, 0], "scale": [1, 1, 1]}],
        )

        self.assertTrue(any("unknown assetId 'missing'" in issue for issue in issues), issues)

    def test_overlay_debug_only_asset_fails_gameplay_contract(self) -> None:
        issues = self.validate(
            [
                *hero_shop_manifest_lines(),
                "debug_shop|models/debug.obj|1,1,1|180,180,180|type=DebugOnly;origin=bottom_center;render=DebugOnly;collision=None;renderInGameplay=false",
            ],
            [{"id": "debug_instance", "assetId": "debug_shop", "position": [0, 0, 0], "scale": [1, 1, 1]}],
        )

        self.assertTrue(any("cannot use non-gameplay asset" in issue for issue in issues), issues)

    def test_solid_building_requires_camera_blocker(self) -> None:
        issues = self.validate(
            [*hero_shop_manifest_lines(),
             "shop|models/shop.obj|1,1,1|180,180,180|type=SolidBuilding;origin=bottom_center;render=Opaque;collisionProfile=SolidWorld;requiresClosedMesh=true"],
        )

        self.assertTrue(any("SolidBuilding must set cameraBlocks=true" in issue for issue in issues), issues)

    def test_decal_collision_fails(self) -> None:
        issues = self.validate(
            [*hero_shop_manifest_lines(),
             "paint|models/paint.obj|1,1,1|180,180,180,128|type=Decal;origin=bottom_center;render=Decal;collision=Prop;cameraBlocks=false;maxHeight=0.02"],
        )

        self.assertTrue(any("Decal must use collision=None" in issue for issue in issues), issues)

    def test_spawn_y_must_be_ground_zero(self) -> None:
        map_data = valid_map()
        spawn = dict(map_data["spawn"])
        spawn["player"] = [0.0, 0.5, 0.0]
        map_data["spawn"] = spawn
        issues = self.validate(
            [*hero_shop_manifest_lines(),
             "bench|models/bench.obj|1,1,1|180,180,180|type=SolidProp;origin=bottom_center;render=Opaque;collision=Prop"],
            map_data=map_data,
        )

        self.assertTrue(any("spawn.player.y must be 0.0" in issue for issue in issues), issues)

    def test_map_objects_are_rejected_until_runtime_loads_them(self) -> None:
        map_data = valid_map()
        map_data["objects"] = [
            {
                "id": "map_only_bench",
                "assetId": "bench",
                "position": [0, 0, 0],
                "scale": [1, 1, 1],
                "gameplayTags": ["seating"],
            }
        ]
        issues = self.validate(
            [*hero_shop_manifest_lines(),
             "bench|models/bench.obj|1,1,1|180,180,180|type=SolidProp;origin=bottom_center;render=Opaque;collision=Prop"],
            map_data=map_data,
        )

        self.assertTrue(any("map objects are not loaded by runtime" in issue for issue in issues), issues)

    def test_elevated_instance_requires_allow_floating(self) -> None:
        issues = self.validate(
            [*hero_shop_manifest_lines(),
             "sign|models/sign.obj|1,1,1|180,180,180|type=SolidProp;origin=bottom_center;render=Opaque;collision=None"],
            [{"id": "floating_sign", "assetId": "sign", "position": [0, 2, 0], "scale": [1, 1, 1]}],
        )

        self.assertTrue(any("elevated placement requires asset metadata allowFloating=true" in issue for issue in issues), issues)

    def test_hero_shop_asset_is_required(self) -> None:
        issues = self.validate([
            "shop_zenona|models/shop.obj|1,1,1|180,180,180|type=DebugOnly;usage=legacy;origin=bottom_center;render=DebugOnly;collision=None;renderInGameplay=false",
            "shop_zenona_v2|models/shop_v2.obj|1,1,1|180,180,180|type=DebugOnly;usage=legacy;origin=bottom_center;render=DebugOnly;collision=None;renderInGameplay=false",
        ])

        self.assertTrue(any("hero shop asset 'shop_zenona_v3' must exist" in issue for issue in issues), issues)

    def test_hero_shop_asset_must_be_solid_world_camera_blocker(self) -> None:
        lines = hero_shop_manifest_lines()
        lines[2] = "shop_zenona_v3|models/shop_v3.obj|1,1,1|180,180,180|type=SolidProp;usage=legacy;origin=bottom_center;render=Translucent;collision=Prop;cameraBlocks=false;requiresClosedMesh=false;allowOpenEdges=true;tags=shop"
        issues = self.validate(lines)

        self.assertTrue(any("usage must be active" in issue for issue in issues), issues)
        self.assertTrue(any("type must be SolidBuilding" in issue for issue in issues), issues)
        self.assertTrue(any("collisionProfile must be SolidWorld" in issue for issue in issues), issues)
        self.assertTrue(any("cameraBlocks must be true" in issue for issue in issues), issues)

    def test_overlay_cannot_replace_hero_shop_with_legacy_asset(self) -> None:
        issues = self.validate(
            hero_shop_manifest_lines(),
            [{
                "id": "shop_zenona",
                "assetId": "shop_zenona",
                "position": [5, 0, -5],
                "scale": [1, 1, 1],
                "zoneTag": "Shop",
                "gameplayTags": ["mission", "shop_trouble", "landmark", "hero_asset"],
            }],
        )

        self.assertTrue(any("hero shop object must use assetId 'shop_zenona_v3'" in issue for issue in issues), issues)
        self.assertTrue(any("must not use legacy shop asset" in issue for issue in issues), issues)

    def test_shop_outcome_catalog_requires_door_grille_price_and_notice_hooks(self) -> None:
        issues = self.validate(
            hero_shop_manifest_lines(),
            outcome_catalog={"schemaVersion": 1, "outcomes": [{"id": "shop_door_checked", "location": "Shop"}]},
        )

        self.assertTrue(any("shop_rolling_grille_checked" in issue for issue in issues), issues)
        self.assertTrue(any("shop_prices_read_*" in issue for issue in issues), issues)
        self.assertTrue(any("shop_notice_read_*" in issue for issue in issues), issues)

    def test_shop_outcome_catalog_separates_noisy_and_quiet_shop_hooks(self) -> None:
        catalog = valid_shop_outcome_catalog()
        outcomes = list(catalog["outcomes"])
        outcomes[0] = {"id": "shop_door_checked", "location": "Shop"}
        outcomes[2] = {
            "idPattern": "shop_prices_read_*",
            "location": "Shop",
            "worldEvent": {"type": "PublicNuisance", "intensity": 0.05, "cooldownSeconds": 2.0},
        }
        catalog["outcomes"] = outcomes
        issues = self.validate(hero_shop_manifest_lines(), outcome_catalog=catalog)

        self.assertTrue(any("shop_door_checked" in issue and "worldEvent" in issue for issue in issues), issues)
        self.assertTrue(any("quiet hero shop outcome pattern 'shop_prices_read_*'" in issue for issue in issues), issues)


if __name__ == "__main__":
    unittest.main()
