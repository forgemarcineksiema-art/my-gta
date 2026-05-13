#!/usr/bin/env python3
"""Generate the stylized mid-poly Blok 13 gruz coupe-sedan glTF asset.

The file is intentionally procedural so the asset can be rebuilt without a DCC
dependency while the art-kit pipeline evolves toward richer authored geometry.
Target quality: chunky readable silhouette with modelled detail (beveled edges,
wheel-arch definition, lamp recesses, trim depth). Maintains current box/cylinder
construction but documents the target direction for future authoring passes."""

from __future__ import annotations

import json
import math
import struct
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "data" / "assets" / "models"
GLTF_PATH = OUT_DIR / "gruz_e36.gltf"
BIN_PATH = OUT_DIR / "gruz_e36.bin"


MATERIALS = [
    {
        "name": "paint_dirty_red",
        "pbrMetallicRoughness": {
            "baseColorFactor": [0.62, 0.16, 0.12, 1.0],
            "metallicFactor": 0.18,
            "roughnessFactor": 0.72,
        },
    },
    {
        "name": "dark_trim",
        "pbrMetallicRoughness": {
            "baseColorFactor": [0.055, 0.052, 0.048, 1.0],
            "metallicFactor": 0.0,
            "roughnessFactor": 0.86,
        },
    },
    {
        "name": "glass_translucent",
        "pbrMetallicRoughness": {
            "baseColorFactor": [0.47, 0.68, 0.75, 0.48],
            "metallicFactor": 0.0,
            "roughnessFactor": 0.24,
        },
        "alphaMode": "BLEND",
        "doubleSided": True,
    },
    {
        "name": "headlight_emissive",
        "pbrMetallicRoughness": {
            "baseColorFactor": [1.0, 0.89, 0.48, 1.0],
            "metallicFactor": 0.0,
            "roughnessFactor": 0.32,
        },
        "emissiveFactor": [1.0, 0.78, 0.28],
    },
    {
        "name": "taillight_emissive",
        "pbrMetallicRoughness": {
            "baseColorFactor": [0.88, 0.08, 0.08, 1.0],
            "metallicFactor": 0.0,
            "roughnessFactor": 0.42,
        },
        "emissiveFactor": [0.9, 0.08, 0.04],
    },
    {
        "name": "grille_dark",
        "pbrMetallicRoughness": {
            "baseColorFactor": [0.025, 0.028, 0.03, 1.0],
            "metallicFactor": 0.0,
            "roughnessFactor": 0.95,
        },
    },
    {
        "name": "rubber_dark",
        "pbrMetallicRoughness": {
            "baseColorFactor": [0.018, 0.018, 0.017, 1.0],
            "metallicFactor": 0.0,
            "roughnessFactor": 0.92,
        },
    },
    {
        "name": "rim_dull_alloy",
        "pbrMetallicRoughness": {
            "baseColorFactor": [0.62, 0.63, 0.58, 1.0],
            "metallicFactor": 0.35,
            "roughnessFactor": 0.58,
        },
    },
    {
        "name": "exhaust_dark_metal",
        "pbrMetallicRoughness": {
            "baseColorFactor": [0.12, 0.12, 0.11, 1.0],
            "metallicFactor": 0.65,
            "roughnessFactor": 0.50,
        },
    },
    {
        "name": "road_dirt_matte",
        "pbrMetallicRoughness": {
            "baseColorFactor": [0.34, 0.28, 0.20, 1.0],
            "metallicFactor": 0.0,
            "roughnessFactor": 0.96,
        },
    },
    {
        "name": "rust_oxidized",
        "pbrMetallicRoughness": {
            "baseColorFactor": [0.55, 0.26, 0.12, 1.0],
            "metallicFactor": 0.0,
            "roughnessFactor": 0.98,
        },
    },
    {
        "name": "primer_mismatched_gray",
        "pbrMetallicRoughness": {
            "baseColorFactor": [0.36, 0.38, 0.36, 1.0],
            "metallicFactor": 0.02,
            "roughnessFactor": 0.88,
        },
    },
    {
        "name": "collider_debug_transparent",
        "pbrMetallicRoughness": {
            "baseColorFactor": [0.25, 0.9, 0.45, 0.10],
            "metallicFactor": 0.0,
            "roughnessFactor": 1.0,
        },
        "alphaMode": "BLEND",
        "doubleSided": True,
    },
]

MAT = {material["name"]: i for i, material in enumerate(MATERIALS)}


PARTS = [
    ("Car_Body", "box", "paint_dirty_red", (0.0, 0.52, 0.0), (1.76, 0.58, 2.70), 0.0),
    ("Hood", "box", "paint_dirty_red", (0.0, 0.82, 0.82), (1.50, 0.070, 1.12), 0.05),
    ("Trunk", "box", "paint_dirty_red", (0.0, 0.82, -1.18), (1.48, 0.055, 0.72), -0.02),
    ("Front_Bumper", "box", "dark_trim", (0.0, 0.43, 1.55), (1.80, 0.22, 0.22), 0.0),
    ("Rear_Bumper", "box", "dark_trim", (0.0, 0.43, -1.54), (1.80, 0.22, 0.22), 0.0),
    ("Fender_FL", "box", "paint_dirty_red", (-0.82, 0.58, 1.04), (0.15, 0.30, 0.74), 0.0),
    ("Fender_FR", "box", "paint_dirty_red", (0.82, 0.58, 1.04), (0.15, 0.30, 0.74), 0.0),
    ("Fender_RL", "box", "paint_dirty_red", (-0.82, 0.58, -1.06), (0.15, 0.30, 0.74), 0.0),
    ("Fender_RR", "box", "paint_dirty_red", (0.82, 0.58, -1.06), (0.15, 0.30, 0.74), 0.0),
    ("Door_L", "box", "paint_dirty_red", (-0.91, 0.68, -0.12), (0.065, 0.48, 0.96), 0.0),
    ("Door_R", "box", "paint_dirty_red", (0.91, 0.68, -0.12), (0.065, 0.48, 0.96), 0.0),
    ("Pillar_A_L", "box", "dark_trim", (-0.84, 1.00, 0.34), (0.070, 0.34, 0.085), 0.38),
    ("Pillar_A_R", "box", "dark_trim", (0.84, 1.00, 0.34), (0.070, 0.34, 0.085), 0.38),
    ("Pillar_B_L", "box", "dark_trim", (-0.84, 1.00, -0.27), (0.070, 0.36, 0.075), 0.0),
    ("Pillar_B_R", "box", "dark_trim", (0.84, 1.00, -0.27), (0.070, 0.36, 0.075), 0.0),
    ("Pillar_C_L", "box", "dark_trim", (-0.84, 0.99, -0.78), (0.075, 0.33, 0.090), -0.32),
    ("Pillar_C_R", "box", "dark_trim", (0.84, 0.99, -0.78), (0.075, 0.33, 0.090), -0.32),
    ("Cabin_Sill_L", "box", "dark_trim", (-0.88, 0.82, -0.25), (0.045, 0.060, 1.16), 0.0),
    ("Cabin_Sill_R", "box", "dark_trim", (0.88, 0.82, -0.25), (0.045, 0.060, 1.16), 0.0),
    ("Roof_Rail_L", "box", "dark_trim", (-0.63, 1.155, -0.38), (0.075, 0.055, 0.95), 0.0),
    ("Roof_Rail_R", "box", "dark_trim", (0.63, 1.155, -0.38), (0.075, 0.055, 0.95), 0.0),
    ("Windshield_Header", "box", "dark_trim", (0.0, 1.145, 0.05), (1.20, 0.055, 0.070), 0.0),
    ("Rear_Window_Header", "box", "dark_trim", (0.0, 1.125, -0.84), (1.08, 0.055, 0.070), 0.0),
    ("Headlight_L", "box", "headlight_emissive", (-0.42, 0.57, 1.66), (0.34, 0.11, 0.065), 0.0),
    ("Headlight_R", "box", "headlight_emissive", (0.42, 0.57, 1.66), (0.34, 0.11, 0.065), 0.0),
    ("Taillight_L", "box", "taillight_emissive", (-0.50, 0.56, -1.66), (0.28, 0.11, 0.065), 0.0),
    ("Taillight_R", "box", "taillight_emissive", (0.50, 0.56, -1.66), (0.28, 0.11, 0.065), 0.0),
    ("Grille", "box", "grille_dark", (0.0, 0.57, 1.68), (0.36, 0.12, 0.055), 0.0),
    ("Mirror_L", "box", "dark_trim", (-1.00, 0.91, 0.30), (0.14, 0.10, 0.20), 0.0),
    ("Mirror_R", "box", "dark_trim", (1.00, 0.91, 0.30), (0.14, 0.10, 0.20), 0.0),
    ("Exhaust", "cylinder_x", "exhaust_dark_metal", (-0.54, 0.29, -1.73), (0.30, 0.08, 0.08), 0.0),
    ("Dirt_Rocker_L", "box", "road_dirt_matte", (-0.946, 0.39, -0.10), (0.034, 0.13, 1.00), 0.0),
    ("Dirt_Rocker_R", "box", "road_dirt_matte", (0.946, 0.39, -0.10), (0.034, 0.13, 1.00), 0.0),
    ("Rust_Fender_FL", "box", "rust_oxidized", (-0.905, 0.53, 1.28), (0.030, 0.16, 0.22), 0.0),
    ("Rust_Fender_RR", "box", "rust_oxidized", (0.905, 0.51, -1.24), (0.030, 0.17, 0.24), 0.0),
    ("Rust_Door_L", "box", "rust_oxidized", (-0.948, 0.54, -0.22), (0.030, 0.15, 0.30), 0.0),
    ("Primer_Door_R", "box", "primer_mismatched_gray", (0.948, 0.73, -0.08), (0.032, 0.22, 0.42), 0.0),
    ("Hood_Chipped_Edge", "box", "road_dirt_matte", (0.0, 0.865, 1.38), (1.16, 0.026, 0.070), 0.0),
    ("Trunk_Dust_Strip", "box", "road_dirt_matte", (0.0, 0.852, -1.48), (1.20, 0.024, 0.060), 0.0),
    ("Bumper_Scuff_Front", "box", "road_dirt_matte", (-0.46, 0.45, 1.675), (0.48, 0.065, 0.030), 0.0),
    ("Bumper_Scuff_Rear", "box", "road_dirt_matte", (0.52, 0.43, -1.675), (0.50, 0.065, 0.030), 0.0),
    ("Collider_Body", "box", "collider_debug_transparent", (0.0, 0.62, 0.0), (1.90, 1.18, 3.38), 0.0),
    ("Collider_Wheels", "box", "collider_debug_transparent", (0.0, 0.35, 0.0), (2.05, 0.72, 2.70), 0.0),
]

# Mid-poly detail parts added v0.10: wheel arches, light recesses, door seams,
# rocker panels, bumper corners, hood/trunk gaps, grille depth, front lip, exhaust tip.
# All dark_trim boxes. Each part is ~0.02-0.04m thick — readable at game distance.
DETAILS = [
    # === Wheel arch inner shadow boxes (dark void behind each tire) ===
    ("ArchInner_FL", "box", "dark_trim", (-0.93, 0.42, 1.05), (0.08, 0.26, 0.58), 0.0),
    ("ArchInner_FR", "box", "dark_trim", (0.93, 0.42, 1.05), (0.08, 0.26, 0.58), 0.0),
    ("ArchInner_RL", "box", "dark_trim", (-0.93, 0.42, -1.05), (0.08, 0.26, 0.58), 0.0),
    ("ArchInner_RR", "box", "dark_trim", (0.93, 0.42, -1.05), (0.08, 0.26, 0.58), 0.0),
    # === Wheel arch top bars (horizontal cap defining arch peak) ===
    ("ArchTop_FL", "box", "dark_trim", (-0.98, 0.60, 1.05), (0.03, 0.03, 0.36), 0.0),
    ("ArchTop_FR", "box", "dark_trim", (0.98, 0.60, 1.05), (0.03, 0.03, 0.36), 0.0),
    ("ArchTop_RL", "box", "dark_trim", (-0.98, 0.60, -1.05), (0.03, 0.03, 0.36), 0.0),
    ("ArchTop_RR", "box", "dark_trim", (0.98, 0.60, -1.05), (0.03, 0.03, 0.36), 0.0),
    # === Wheel arch front/rear diagonal edges (trace arch curve from body up to peak) ===
    ("ArchFr_FL", "box", "dark_trim", (-0.91, 0.40, 1.22), (0.03, 0.24, 0.024), 0.42),
    ("ArchRr_FL", "box", "dark_trim", (-0.91, 0.40, 0.88), (0.03, 0.24, 0.024), -0.42),
    ("ArchFr_FR", "box", "dark_trim", (0.91, 0.40, 1.22), (0.03, 0.24, 0.024), -0.42),
    ("ArchRr_FR", "box", "dark_trim", (0.91, 0.40, 0.88), (0.03, 0.24, 0.024), 0.42),
    ("ArchFr_RL", "box", "dark_trim", (-0.91, 0.40, -0.88), (0.03, 0.24, 0.024), -0.42),
    ("ArchRr_RL", "box", "dark_trim", (-0.91, 0.40, -1.22), (0.03, 0.24, 0.024), 0.42),
    ("ArchFr_RR", "box", "dark_trim", (0.91, 0.40, -0.88), (0.03, 0.24, 0.024), 0.42),
    ("ArchRr_RR", "box", "dark_trim", (0.91, 0.40, -1.22), (0.03, 0.24, 0.024), -0.42),
    # === Light recess housings (depth pocket behind head/taillights) ===
    ("HL_House_L", "box", "dark_trim", (-0.42, 0.57, 1.63), (0.38, 0.15, 0.030), 0.0),
    ("HL_House_R", "box", "dark_trim", (0.42, 0.57, 1.63), (0.38, 0.15, 0.030), 0.0),
    ("TL_House_L", "box", "dark_trim", (-0.50, 0.56, -1.63), (0.32, 0.15, 0.030), 0.0),
    ("TL_House_R", "box", "dark_trim", (0.50, 0.56, -1.63), (0.32, 0.15, 0.030), 0.0),
    # === Door panel gaps (thin dark strips at door edges for visible seams) ===
    ("DoorGap_L_F", "box", "dark_trim", (-0.93, 0.66, 0.36), (0.018, 0.48, 0.014), 0.0),
    ("DoorGap_L_R", "box", "dark_trim", (-0.93, 0.66, -0.60), (0.018, 0.48, 0.014), 0.0),
    ("DoorGap_R_F", "box", "dark_trim", (0.93, 0.66, 0.36), (0.018, 0.48, 0.014), 0.0),
    ("DoorGap_R_R", "box", "dark_trim", (0.93, 0.66, -0.60), (0.018, 0.48, 0.014), 0.0),
    # === Rocker panels (dark trim below doors between wheel arches) ===
    ("Rocker_L", "box", "dark_trim", (-0.94, 0.33, -0.08), (0.030, 0.10, 1.24), 0.0),
    ("Rocker_R", "box", "dark_trim", (0.94, 0.33, -0.08), (0.030, 0.10, 1.24), 0.0),
    # === Hood and trunk panel gaps ===
    ("HoodGap_L", "box", "dark_trim", (-0.73, 0.843, 1.38), (0.014, 0.020, 0.10), 0.0),
    ("HoodGap_R", "box", "dark_trim", (0.73, 0.843, 1.38), (0.014, 0.020, 0.10), 0.0),
    ("HoodGap_F", "box", "dark_trim", (0.0, 0.843, 1.39), (0.50, 0.018, 0.014), 0.0),
    ("TrunkGap_L", "box", "dark_trim", (-0.73, 0.833, -0.82), (0.014, 0.018, 0.09), 0.0),
    ("TrunkGap_R", "box", "dark_trim", (0.73, 0.833, -0.82), (0.014, 0.018, 0.09), 0.0),
    # === Grille recess (dark pocket behind grille surface) ===
    ("Grille_House", "box", "dark_trim", (0.0, 0.57, 1.67), (0.40, 0.16, 0.028), 0.0),
    # === License plate recess (dark pocket in rear bumper) ===
    ("Plate_Rec", "box", "dark_trim", (0.0, 0.48, -1.67), (0.34, 0.12, 0.026), 0.0),
    # === Bumper wrap corners ===
    ("Bump_FL", "box", "dark_trim", (-0.88, 0.43, 1.41), (0.06, 0.22, 0.08), 0.18),
    ("Bump_FR", "box", "dark_trim", (0.88, 0.43, 1.41), (0.06, 0.22, 0.08), -0.18),
    ("Bump_RL", "box", "dark_trim", (-0.88, 0.43, -1.41), (0.06, 0.22, 0.08), -0.18),
    ("Bump_RR", "box", "dark_trim", (0.88, 0.43, -1.41), (0.06, 0.22, 0.08), 0.18),
    # === Front lower lip / splitter strip ===
    ("FrontLip", "box", "dark_trim", (0.0, 0.31, 1.62), (1.52, 0.016, 0.06), 0.0),
    # === Exhaust tip (secondary pipe at muffler exit) ===
    ("ExhaustTip", "cylinder_x", "dark_trim", (-0.54, 0.29, -1.75), (0.06, 0.10, 0.10), 0.0),
    # === Mirror arms (connecting body to mirror heads) ===
    ("MirArm_L", "cylinder_x", "dark_trim", (-0.94, 0.91, 0.30), (0.12, 0.035, 0.035), 0.0),
    ("MirArm_R", "cylinder_x", "dark_trim", (0.94, 0.91, 0.30), (0.12, 0.035, 0.035), 0.0),
    # === Side sill lower trim strips ===
    ("SillLow_L", "box", "dark_trim", (-0.91, 0.40, -0.08), (0.022, 0.035, 1.10), 0.0),
    ("SillLow_R", "box", "dark_trim", (0.91, 0.40, -0.08), (0.022, 0.035, 1.10), 0.0),
    # === Rear diffuser hint strip ===
    ("Diffuser", "box", "dark_trim", (0.0, 0.31, -1.62), (1.30, 0.014, 0.05), 0.0),
]

PANEL_PARTS = [
    ("Roof", "paint_dirty_red", [(-0.59, 1.19, 0.04), (0.59, 1.19, 0.04), (0.53, 1.17, -0.84), (-0.53, 1.17, -0.84)]),
    ("Windshield", "glass_translucent", [(-0.55, 1.145, 0.06), (0.55, 1.145, 0.06), (0.70, 0.87, 0.66), (-0.70, 0.87, 0.66)]),
    ("Rear_Window", "glass_translucent", [(-0.51, 1.125, -0.84), (0.51, 1.125, -0.84), (0.68, 0.88, -1.18), (-0.68, 0.88, -1.18)]),
    ("Window_L", "glass_translucent", [(-0.62, 1.155, 0.03), (-0.57, 1.140, -0.83), (-0.76, 0.82, -0.98), (-0.78, 0.82, 0.48)]),
    ("Window_R", "glass_translucent", [(0.62, 1.155, 0.03), (0.57, 1.140, -0.83), (0.76, 0.82, -0.98), (0.78, 0.82, 0.48)]),
]

WHEELS = {
    "FL": (-0.98, 0.32, 1.05, True),
    "FR": (0.98, 0.32, 1.05, True),
    "RL": (-0.98, 0.32, -1.05, False),
    "RR": (0.98, 0.32, -1.05, False),
}


def box_mesh(size):
    sx, sy, sz = (value * 0.5 for value in size)
    vertices = [
        (-sx, -sy, -sz), (sx, -sy, -sz), (sx, sy, -sz), (-sx, sy, -sz),
        (-sx, -sy, sz), (sx, -sy, sz), (sx, sy, sz), (-sx, sy, sz),
    ]
    indices = [
        0, 1, 2, 0, 2, 3,
        4, 6, 5, 4, 7, 6,
        0, 4, 5, 0, 5, 1,
        1, 5, 6, 1, 6, 2,
        2, 6, 7, 2, 7, 3,
        3, 7, 4, 3, 4, 0,
    ]
    return vertices, indices


def cylinder_x_mesh(size, segments=20):
    length = size[0]
    radius_y = size[1] * 0.5
    radius_z = size[2] * 0.5
    x0 = -length * 0.5
    x1 = length * 0.5
    vertices = []
    for x in (x0, x1):
        for i in range(segments):
            angle = math.tau * i / segments
            vertices.append((x, math.cos(angle) * radius_y, math.sin(angle) * radius_z))
    center_left = len(vertices)
    vertices.append((x0, 0.0, 0.0))
    center_right = len(vertices)
    vertices.append((x1, 0.0, 0.0))

    indices = []
    for i in range(segments):
        j = (i + 1) % segments
        indices += [i, segments + i, segments + j, i, segments + j, j]
        indices += [center_left, j, i]
        indices += [center_right, segments + i, segments + j]
    return vertices, indices


def quad_panel_mesh(corners):
    return list(corners), [0, 1, 2, 0, 2, 3, 2, 1, 0, 3, 2, 0]


def align4(data: bytearray):
    while len(data) % 4:
        data.append(0)


def add_accessor(gltf, blob, values, component_type, accessor_type, target):
    align4(blob)
    offset = len(blob)
    if component_type == 5126:
        for value in values:
            blob += struct.pack("<fff", *value)
        byte_length = len(values) * 12
        mins = [min(v[i] for v in values) for i in range(3)]
        maxs = [max(v[i] for v in values) for i in range(3)]
    else:
        for value in values:
            blob += struct.pack("<H", value)
        byte_length = len(values) * 2
        mins = maxs = None

    view_index = len(gltf["bufferViews"])
    gltf["bufferViews"].append({
        "buffer": 0,
        "byteOffset": offset,
        "byteLength": byte_length,
        "target": target,
    })

    accessor = {
        "bufferView": view_index,
        "byteOffset": 0,
        "componentType": component_type,
        "count": len(values),
        "type": accessor_type,
    }
    if mins is not None:
        accessor["min"] = mins
        accessor["max"] = maxs
    accessor_index = len(gltf["accessors"])
    gltf["accessors"].append(accessor)
    return accessor_index


def calculate_vertex_normals(positions, indices):
    normals = [[0.0, 0.0, 0.0] for _ in positions]
    assigned = [False for _ in positions]

    for i in range(0, len(indices), 3):
        a_index, b_index, c_index = indices[i:i + 3]
        ax, ay, az = positions[a_index]
        bx, by, bz = positions[b_index]
        cx, cy, cz = positions[c_index]
        ab = (bx - ax, by - ay, bz - az)
        ac = (cx - ax, cy - ay, cz - az)
        normal = (
            ab[1] * ac[2] - ab[2] * ac[1],
            ab[2] * ac[0] - ab[0] * ac[2],
            ab[0] * ac[1] - ab[1] * ac[0],
        )
        length = math.sqrt(sum(component * component for component in normal))
        if length <= 0.000001:
            continue
        normal = tuple(component / length for component in normal)
        for vertex_index in (a_index, b_index, c_index):
            if not assigned[vertex_index]:
                normals[vertex_index] = [normal[0], normal[1], normal[2]]
                assigned[vertex_index] = True

    for i, is_assigned in enumerate(assigned):
        if not is_assigned:
            normals[i] = [0.0, 1.0, 0.0]
    return [tuple(normal) for normal in normals]


def add_mesh(gltf, blob, name, shape, material, size):
    if shape == "box":
        positions, indices = box_mesh(size)
    elif shape == "cylinder_x":
        positions, indices = cylinder_x_mesh(size)
    else:
        raise ValueError(shape)
    position_accessor = add_accessor(gltf, blob, positions, 5126, "VEC3", 34962)
    normal_accessor = add_accessor(gltf, blob, calculate_vertex_normals(positions, indices), 5126, "VEC3", 34962)
    index_accessor = add_accessor(gltf, blob, indices, 5123, "SCALAR", 34963)
    mesh_index = len(gltf["meshes"])
    gltf["meshes"].append({
        "name": f"{name}_Mesh",
        "primitives": [{
            "attributes": {"POSITION": position_accessor, "NORMAL": normal_accessor},
            "indices": index_accessor,
            "material": MAT[material],
        }],
    })
    return mesh_index


def add_panel_mesh(gltf, blob, name, material, corners):
    positions, indices = quad_panel_mesh(corners)
    position_accessor = add_accessor(gltf, blob, positions, 5126, "VEC3", 34962)
    normal_accessor = add_accessor(gltf, blob, calculate_vertex_normals(positions, indices), 5126, "VEC3", 34962)
    index_accessor = add_accessor(gltf, blob, indices, 5123, "SCALAR", 34963)
    mesh_index = len(gltf["meshes"])
    gltf["meshes"].append({
        "name": f"{name}_Mesh",
        "primitives": [{
            "attributes": {"POSITION": position_accessor, "NORMAL": normal_accessor},
            "indices": index_accessor,
            "material": MAT[material],
        }],
    })
    return mesh_index


def x_rotation_quaternion(radians):
    half = radians * 0.5
    return [math.sin(half), 0.0, 0.0, math.cos(half)]


def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    gltf = {
        "asset": {"version": "2.0", "generator": "Blok 13 gruz generator v2 mid-poly"},
        "scene": 0,
        "scenes": [{"name": "Gruz_E36_Runtime", "nodes": []}],
        "nodes": [],
        "meshes": [],
        "materials": MATERIALS,
        "buffers": [{"uri": BIN_PATH.name, "byteLength": 0}],
        "bufferViews": [],
        "accessors": [],
    }
    blob = bytearray()

    for name, shape, material, translation, size, rotation_x in PARTS:
        if name.startswith("Collider_"):
            continue
        mesh = add_mesh(gltf, blob, name, shape, material, size)
        node = {
            "name": name,
            "mesh": mesh,
            "translation": list(translation),
        }
        if abs(rotation_x) > 0.0001:
            node["rotation"] = x_rotation_quaternion(rotation_x)
        gltf["scenes"][0]["nodes"].append(len(gltf["nodes"]))
        gltf["nodes"].append(node)

    for name, shape, material, translation, size, rotation_x in DETAILS:
        mesh = add_mesh(gltf, blob, name, shape, material, size)
        node = {
            "name": name,
            "mesh": mesh,
            "translation": list(translation),
        }
        if abs(rotation_x) > 0.0001:
            node["rotation"] = x_rotation_quaternion(rotation_x)
        gltf["scenes"][0]["nodes"].append(len(gltf["nodes"]))
        gltf["nodes"].append(node)

    for name, material, corners in PANEL_PARTS:
        mesh = add_panel_mesh(gltf, blob, name, material, corners)
        node = {
            "name": name,
            "mesh": mesh,
            "translation": [0.0, 0.0, 0.0],
            "extras": {"panelQuad": True},
        }
        gltf["scenes"][0]["nodes"].append(len(gltf["nodes"]))
        gltf["nodes"].append(node)

    for suffix, (x, y, z, front) in WHEELS.items():
        children = []
        for kind, material, size in (
            ("Tire", "rubber_dark", (0.24, 0.58, 0.58)),
            ("Rim", "rim_dull_alloy", (0.27, 0.34, 0.34)),
            ("Hub", "rim_dull_alloy", (0.04, 0.08, 0.08)),
        ):
            node_name = f"{kind}_{suffix}"
            mesh = add_mesh(gltf, blob, node_name, "cylinder_x", material, size)
            child_index = len(gltf["nodes"])
            gltf["nodes"].append({
                "name": node_name,
                "mesh": mesh,
                "translation": [0.0, 0.0, 0.0],
                "extras": {"frontWheel": front, "wheelPart": kind},
            })
            children.append(child_index)

        wheel_index = len(gltf["nodes"])
        gltf["nodes"].append({
            "name": f"Wheel_{suffix}",
            "translation": [x, y, z],
            "children": children,
            "extras": {
                "pivot": "axle_center",
                "frontWheel": front,
                "rotatesFromSpeed": True,
                "steers": front,
            },
        })
        gltf["scenes"][0]["nodes"].append(wheel_index)

    align4(blob)
    gltf["buffers"][0]["byteLength"] = len(blob)

    BIN_PATH.write_bytes(blob)
    GLTF_PATH.write_text(json.dumps(gltf, indent=2), encoding="utf-8")
    print(f"wrote {GLTF_PATH}")
    print(f"wrote {BIN_PATH}")


if __name__ == "__main__":
    main()
