#!/usr/bin/env python3
"""Unit tests for validate_editor_overlay.py."""

from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path

from validate_editor_overlay import validate_overlay


def world_object(object_id: str = "object_a", asset_id: str = "prop_a") -> dict[str, object]:
    return {
        "id": object_id,
        "assetId": asset_id,
        "position": [0.0, 0.0, 0.0],
        "scale": [1.0, 1.0, 1.0],
        "yawRadians": 0.0,
        "zoneTag": "Block",
        "gameplayTags": ["prop"],
    }


class EditorOverlayValidatorTests(unittest.TestCase):
    def validate(self, overlay: dict[str, object], manifest_lines: list[str] | None = None) -> list[str]:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            asset_root = root / "assets"
            asset_root.mkdir()
            (asset_root / "asset_manifest.txt").write_text(
                "\n".join(
                    manifest_lines
                    or ["prop_a|models/prop_a.glb|1,1,1|255,255,255,255|type=SolidProp;origin=bottom_center;render=Opaque;collision=Prop"]
                )
                + "\n",
                encoding="utf-8",
            )
            overlay_path = root / "overlay.json"
            overlay_path.write_text(json.dumps(overlay), encoding="utf-8")
            return validate_overlay(overlay_path, asset_root)

    def test_accepts_valid_overlay(self) -> None:
        issues = self.validate(
            {
                "schemaVersion": 1,
                "overrides": [world_object("base_object")],
                "instances": [world_object("new_object")],
            }
        )

        self.assertEqual([], issues)

    def test_rejects_duplicate_ids_across_overrides_and_instances(self) -> None:
        issues = self.validate(
            {
                "schemaVersion": 1,
                "overrides": [world_object("same_id")],
                "instances": [world_object("same_id")],
            }
        )

        self.assertTrue(any("duplicate overlay object id 'same_id'" in issue for issue in issues), issues)

    def test_rejects_non_gameplay_assets_like_world_contract(self) -> None:
        issues = self.validate(
            {"schemaVersion": 1, "instances": [world_object("debug_object", "debug_prop")]},
            [
                "debug_prop|models/debug.glb|1,1,1|255,0,255,255|type=DebugOnly;origin=bottom_center;render=DebugOnly;collision=None;renderInGameplay=false"
            ],
        )

        self.assertTrue(any("cannot use non-gameplay asset 'debug_prop'" in issue for issue in issues), issues)

    def test_rejects_invalid_object_fields(self) -> None:
        bad_object = world_object("bad_object", "missing_asset")
        bad_object["position"] = [0.0, float("nan"), 0.0]
        bad_object["scale"] = [1.0, 0.0, 1.0]
        bad_object["yawRadians"] = float("inf")
        bad_object["zoneTag"] = "Roof"
        bad_object["gameplayTags"] = ["ok", 7]

        issues = self.validate(
            {
                "schemaVersion": 1,
                "instances": [
                    bad_object,
                    world_object("missing_asset_object", "missing_asset"),
                ],
            }
        )

        self.assertTrue(any("unknown assetId 'missing_asset'" in issue for issue in issues), issues)
        self.assertTrue(any("position must be a finite vec3" in issue for issue in issues), issues)
        self.assertTrue(any("scale values must be positive" in issue for issue in issues), issues)
        self.assertTrue(any("yaw/yawRadians must be finite" in issue for issue in issues), issues)
        self.assertTrue(any("unknown zone 'Roof'" in issue for issue in issues), issues)
        self.assertTrue(any("tags/gameplayTags must be a non-empty string array" in issue for issue in issues), issues)

    def test_rejects_disabled_base_objects(self) -> None:
        issues = self.validate({"schemaVersion": 1, "disabledBaseObjects": ["old_object"]})

        self.assertTrue(
            any("disabledBaseObjects is not supported in schema v1" in issue for issue in issues),
            issues,
        )


if __name__ == "__main__":
    unittest.main()
