#!/usr/bin/env python3
"""Generate mid-poly playground prop .obj models with per-face normals."""

import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "data" / "assets" / "models"


def box_verts(sx, sy, sz):
    """8 vertices, box sits on Y=0 (bottom-aligned)."""
    return [
        (-sx, 0.0, -sz), (sx, 0.0, -sz), (sx, sy, -sz), (-sx, sy, -sz),
        (-sx, 0.0, sz),  (sx, 0.0, sz),  (sx, sy, sz),  (-sx, sy, sz),
    ]


def box_faces(bo):
    """12 triangles per box, CCW winding, outward normals."""
    return [
        (bo+0, bo+1, bo+2), (bo+0, bo+2, bo+3),  # front (-Z)
        (bo+4, bo+6, bo+5), (bo+4, bo+7, bo+6),  # back (+Z)
        (bo+0, bo+4, bo+5), (bo+0, bo+5, bo+1),  # bottom (-Y)
        (bo+1, bo+5, bo+6), (bo+1, bo+6, bo+2),  # right (+X)
        (bo+2, bo+3, bo+7), (bo+2, bo+7, bo+6),  # top (+Y, CCW from above)
        (bo+3, bo+7, bo+4), (bo+3, bo+4, bo+0),  # left (-X)
    ]


def face_normal(v0, v1, v2):
    ax, ay, az = v0; bx, by, bz = v1; cx, cy, cz = v2
    abx, aby, abz = bx-ax, by-ay, bz-az
    acx, acy, acz = cx-ax, cy-ay, cz-az
    nx, ny, nz = aby*acz - abz*acy, abz*acx - abx*acz, abx*acy - aby*acx
    length = math.sqrt(nx*nx + ny*ny + nz*nz)
    if length < 0.000001:
        return (0.0, 1.0, 0.0)
    return (nx/length, ny/length, nz/length)


def build_obj(name, boxes):
    """Build unified .obj from translated box primitives. Per-face normals."""
    all_v = []
    all_faces = []
    all_norms = []

    for cx, cy, cz, hx, hy, hz in boxes:
        bv = box_verts(hx, hy, hz)
        tv = [(x+cx, y+cy, z+cz) for x, y, z in bv]
        bo = len(all_v)
        all_v.extend(tv)
        faces = box_faces(bo)
        for fa, fb, fc in faces:
            n = face_normal(all_v[fa], all_v[fb], all_v[fc])
            ni = len(all_norms)
            all_norms.append(n)
            all_faces.append((fa, fb, fc, ni))

    lines = [f"# Stylized mid-poly {name}", f"o {name}"]
    for v in all_v:
        lines.append(f"v {v[0]:.6f} {v[1]:.6f} {v[2]:.6f}")
    for n in all_norms:
        lines.append(f"vn {n[0]:.6f} {n[1]:.6f} {n[2]:.6f}")
    for fa, fb, fc, ni in all_faces:
        lines.append(f"f {fa+1}//{ni+1} {fb+1}//{ni+1} {fc+1}//{ni+1}")

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    path = OUT_DIR / f"{name}.obj"
    path.write_text("\n".join(lines) + "\n", encoding="ascii")
    print(f"  wrote {path.name}: {len(all_v)}v {len(all_faces)}t")


def main():
    print("Generating playground prop models:")

    # trzepak: 4 posts + 3 bars
    build_obj("trzepak", [
        (-1.35, 0.0, 0.0, 0.03, 1.20, 0.03),
        (-0.45, 0.0, 0.0, 0.03, 1.20, 0.03),
        (0.45, 0.0, 0.0, 0.03, 1.20, 0.03),
        (1.35, 0.0, 0.0, 0.03, 1.20, 0.03),
        (-0.90, 1.05, 0.0, 1.35, 0.03, 0.02),
        (0.0, 1.15, 0.0, 1.35, 0.03, 0.02),
        (0.90, 1.25, 0.0, 1.35, 0.03, 0.02),
    ])

    # swing_set: frame + chains + seats
    build_obj("swing_set", [
        (-1.0, 0.0, 0.55, 0.05, 2.20, 0.05),
        (-1.0, 0.0, -0.55, 0.05, 2.20, 0.05),
        (1.0, 0.0, 0.55, 0.05, 2.20, 0.05),
        (1.0, 0.0, -0.55, 0.05, 2.20, 0.05),
        (-1.0, 1.10, 0.0, 0.04, 0.04, 0.55),
        (1.0, 1.10, 0.0, 0.04, 0.04, 0.55),
        (0.0, 2.15, 0.0, 1.0, 0.04, 0.04),
        (-0.5, 0.60, 0.0, 0.015, 1.50, 0.015),
        (0.5, 0.60, 0.0, 0.015, 1.50, 0.015),
        (-0.5, 0.55, 0.0, 0.20, 0.03, 0.08),
        (0.5, 0.55, 0.0, 0.20, 0.03, 0.08),
    ])

    # slide_small: platform + ladder + chute
    build_obj("slide_small", [
        (0.0, 1.15, -0.3, 0.40, 0.04, 0.35),
        (-0.35, 0.0, -0.5, 0.04, 1.15, 0.04),
        (0.35, 0.0, -0.5, 0.04, 1.15, 0.04),
        (-0.35, 0.0, -0.1, 0.04, 0.02, 0.04),
        (0.35, 0.0, -0.1, 0.04, 0.02, 0.04),
        (-0.25, 0.0, -0.1, 0.025, 1.15, 0.025),
        (0.25, 0.0, -0.1, 0.025, 1.15, 0.025),
        (0.0, 0.10, -0.1, 0.22, 0.015, 0.015),
        (0.0, 0.25, -0.1, 0.22, 0.015, 0.015),
        (0.0, 0.40, -0.1, 0.22, 0.015, 0.015),
        (0.0, 0.55, -0.1, 0.22, 0.015, 0.015),
        (0.0, 0.70, -0.1, 0.22, 0.015, 0.015),
        (0.0, 0.50, 0.80, 0.25, 0.025, 0.80),
        (-0.27, 0.50, 0.80, 0.025, 0.05, 0.80),
        (0.27, 0.50, 0.80, 0.025, 0.05, 0.80),
    ])

    # basketball_hoop: pole + backboard + rim
    build_obj("basketball_hoop", [
        (0.0, 0.0, 0.0, 0.06, 3.0, 0.06),
        (0.0, 2.55, 0.06, 0.55, 0.40, 0.03),
        (-0.20, 2.35, 0.20, 0.025, 0.025, 0.22),
        (0.20, 2.35, 0.20, 0.025, 0.025, 0.22),
        (0.0, 2.35, 0.42, 0.22, 0.025, 0.025),
        (0.0, 2.35, 0.04, 0.10, 0.06, 0.03),
    ])

    print("Done.")


if __name__ == "__main__":
    main()
