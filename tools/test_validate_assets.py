#!/usr/bin/env python3
"""Unit tests for the standalone asset validator."""

from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path

from validate_assets import validate_assets


def write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


def write_obj(path: Path, vertices: list[str] | None = None) -> None:
    if vertices is None:
        write_text(
            path,
            "\n".join(
                [
                    "v -0.50 0.00 -0.50",
                    "v 0.50 0.00 -0.50",
                    "v 0.50 1.00 -0.50",
                    "v -0.50 1.00 -0.50",
                    "v -0.50 0.00 0.50",
                    "v 0.50 0.00 0.50",
                    "v 0.50 1.00 0.50",
                    "v -0.50 1.00 0.50",
                    "vn 0.000000 0.000000 -1.000000",
                    "vn 0.000000 0.000000 -1.000000",
                    "vn 0.000000 0.000000 1.000000",
                    "vn 0.000000 0.000000 1.000000",
                    "vn 0.000000 -1.000000 0.000000",
                    "vn 0.000000 -1.000000 0.000000",
                    "vn 1.000000 0.000000 0.000000",
                    "vn 1.000000 0.000000 0.000000",
                    "vn 0.000000 1.000000 0.000000",
                    "vn 0.000000 1.000000 0.000000",
                    "vn -1.000000 0.000000 0.000000",
                    "vn -1.000000 0.000000 0.000000",
                    "f 1//1 3//1 2//1",
                    "f 1//2 4//2 3//2",
                    "f 5//3 7//3 8//3",
                    "f 5//4 6//4 7//4",
                    "f 1//5 6//5 5//5",
                    "f 1//6 2//6 6//6",
                    "f 2//7 7//7 6//7",
                    "f 2//8 3//8 7//8",
                    "f 3//9 8//9 7//9",
                    "f 3//10 4//10 8//10",
                    "f 4//11 5//11 8//11",
                    "f 4//12 1//12 5//12",
                    "",
                ]
            ),
        )
        return
    write_text(
        path,
        "\n".join([*vertices, "vn 0 0 1", "f 1//1 2//1 3//1", ""]),
    )


class AssetValidatorTests(unittest.TestCase):
    def validate_manifest(self, manifest_line: str) -> tuple[list[str], list[str]]:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            write_obj(root / "models" / "unit.obj")
            write_text(root / "asset_manifest.txt", manifest_line + "\n")
            return validate_assets(root)

    def test_valid_minimal_manifest_passes(self) -> None:
        issues, warnings = self.validate_manifest(
            "unit|models/unit.obj|1,1,1|180,180,180|origin=bottom_center;render=Opaque;collision=None;tags=prop,street"
        )

        self.assertEqual([], issues)
        self.assertEqual([], warnings)

    def test_unknown_metadata_key_fails(self) -> None:
        issues, _ = self.validate_manifest("unit|models/unit.obj|1,1,1|180,180,180|shader=fancy")

        self.assertTrue(any("unknown metadata key 'shader'" in issue for issue in issues), issues)

    def test_duplicate_manifest_tags_fail(self) -> None:
        issues, _ = self.validate_manifest("unit|models/unit.obj|1,1,1|180,180,180|tags=prop,prop")

        self.assertTrue(any("repeats metadata tag 'prop'" in issue for issue in issues), issues)

    def test_bottom_center_obj_with_lifted_min_y_fails(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            write_obj(root / "models" / "lifted.obj", ["v 0 0.25 0", "v 1 0.25 0", "v 0 1.25 0"])
            write_text(
                root / "asset_manifest.txt",
                "lifted|models/lifted.obj|1,1,1|180,180,180|origin=bottom_center\n",
            )

            issues, _ = validate_assets(root)

        self.assertTrue(any("minY 0.250 should be 0" in issue for issue in issues), issues)

    def test_visual_offset_can_compensate_non_zero_min_y(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            write_obj(root / "models" / "lifted.obj", ["v 0 0.25 0", "v 1 0.25 0", "v 0 1.25 0"])
            write_text(
                root / "asset_manifest.txt",
                "lifted|models/lifted.obj|1,1,1|180,180,180|origin=bottom_center;visualOffset=0,-0.25,0\n",
            )

            issues, _ = validate_assets(root)

        self.assertEqual([], issues)

    def test_obj_without_normals_fails(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            write_text(root / "models" / "unit.obj", "\n".join(["v 0 0 0", "v 1 0 0", "v 0 1 0", "f 1 2 3", ""]))
            write_text(root / "asset_manifest.txt", "unit|models/unit.obj|1,1,1|180,180,180|origin=bottom_center\n")

            issues, _ = validate_assets(root)

        self.assertTrue(any("OBJ has no vn normal vectors" in issue for issue in issues), issues)
        self.assertTrue(any("face must reference vn normals" in issue for issue in issues), issues)

    def test_obj_quad_face_fails_triangulation_contract(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            write_text(
                root / "models" / "quad.obj",
                "\n".join(["v 0 0 0", "v 1 0 0", "v 1 1 0", "v 0 1 0", "vn 0 0 1", "f 1//1 2//1 3//1 4//1", ""]),
            )
            write_text(root / "asset_manifest.txt", "quad|models/quad.obj|1,1,1|180,180,180|origin=bottom_center\n")

            issues, _ = validate_assets(root)

        self.assertTrue(any("OBJ faces must be triangulated" in issue for issue in issues), issues)

    def test_obj_reversed_top_face_fails(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            write_text(
                root / "models" / "top.obj",
                "\n".join(["v 0 1 0", "v 1 1 0", "v 0 1 1", "vn 0 -1 0", "f 1//1 2//1 3//1", ""]),
            )
            write_text(root / "asset_manifest.txt", "top|models/top.obj|1,1,1|180,180,180|origin=bottom_center\n")

            issues, _ = validate_assets(root)

        self.assertTrue(any("top face winds downward" in issue for issue in issues), issues)

    def test_box_like_obj_with_inward_face_fails(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            write_text(
                root / "models" / "unit_box.obj",
                "\n".join(
                    [
                        "v -0.5 0 -0.5",
                        "v 0.5 0 -0.5",
                        "v 0.5 1 -0.5",
                        "v 0 0 0.5",
                        "vn 0 0 1",
                        "f 1//1 2//1 3//1",
                        "",
                    ]
                ),
            )
            write_text(root / "asset_manifest.txt", "unit|models/unit_box.obj|1,1,1|180,180,180|origin=bottom_center\n")

            issues, _ = validate_assets(root)

        self.assertTrue(any("face winds inward for solid asset" in issue for issue in issues), issues)

    def test_open_non_decal_obj_warns_about_single_sided_geometry(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            write_obj(root / "models" / "plane.obj", ["v 0 0 0", "v 1 0 0", "v 0 1 0"])
            write_text(root / "asset_manifest.txt", "plane|models/plane.obj|1,1,1|180,180,180|origin=bottom_center\n")

            issues, warnings = validate_assets(root)

        self.assertEqual([], issues)
        self.assertTrue(any("open boundary edges" in warning for warning in warnings), warnings)

    def test_ground_decal_obj_may_be_single_sided_upward(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            write_text(
                root / "models" / "patch.obj",
                "\n".join(
                    [
                        "v -0.5 0 -0.5",
                        "v 0.5 0 -0.5",
                        "v 0.5 0 0.5",
                        "v -0.5 0 0.5",
                        "vn 0 1 0",
                        "vn 0 1 0",
                        "f 4//1 3//1 2//1",
                        "f 4//2 2//2 1//2",
                        "",
                    ]
                ),
            )
            write_text(
                root / "asset_manifest.txt",
                "patch|models/patch.obj|1,1,1|180,180,180,128|origin=bottom_center;render=Translucent;collision=None;tags=decal,ground_patch\n",
            )

            issues, warnings = validate_assets(root)

        self.assertEqual([], issues)
        self.assertEqual([], warnings)

    def test_gltf_missing_buffer_fails(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            write_text(root / "asset_manifest.txt", "hero|models/hero.gltf|1,1,1|180,180,180\n")
            write_text(
                root / "models" / "hero.gltf",
                json.dumps(
                    {
                        "asset": {"version": "2.0"},
                        "buffers": [{"uri": "hero.bin", "byteLength": 4}],
                        "meshes": [{"primitives": [{"attributes": {"POSITION": 0, "NORMAL": 1}}]}],
                    }
                ),
            )

            issues, _ = validate_assets(root)

        self.assertTrue(any("referenced buffer 'hero.bin' is missing" in issue for issue in issues), issues)

    def test_bottom_center_gltf_with_lifted_position_min_y_fails(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            write_text(
                root / "asset_manifest.txt",
                "hero|models/hero.gltf|1,1,1|180,180,180|origin=bottom_center\n",
            )
            write_text(
                root / "models" / "hero.gltf",
                json.dumps(
                    {
                        "asset": {"version": "2.0"},
                        "accessors": [
                            {"type": "VEC3", "min": [0.0, 0.5, 0.0], "max": [1.0, 1.5, 1.0]},
                            {"type": "VEC3"},
                        ],
                        "meshes": [{"primitives": [{"attributes": {"POSITION": 0, "NORMAL": 1}}]}],
                    }
                ),
            )

            issues, _ = validate_assets(root)

        self.assertTrue(any("position minY 0.500 should be 0" in issue for issue in issues), issues)


if __name__ == "__main__":
    unittest.main()
