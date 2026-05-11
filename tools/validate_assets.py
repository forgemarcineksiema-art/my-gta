#!/usr/bin/env python3
"""Validate authored world asset files before they reach runtime loading."""

from __future__ import annotations

import argparse
import json
import math
from collections import Counter
from dataclasses import dataclass
from pathlib import Path


EPSILON = 0.0001
KNOWN_METADATA_KEYS = {
    "type",
    "usage",
    "origin",
    "render",
    "collision",
    "collisionProfile",
    "tags",
    "cameraBlocks",
    "requiresClosedMesh",
    "allowOpenEdges",
    "allowFloating",
    "renderInGameplay",
    "maxHeight",
    "visualOffset",
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
KNOWN_ASSET_USAGES = {"active", "legacy", "debug"}
KNOWN_ORIGINS = {"bottom_center", "center"}
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
OUTWARD_NORMAL_MODEL_FILENAMES = {
    "unit_box.obj",
    "block13_core.obj",
    "shop_zenona_v2.obj",
    "shop_zenona_v3.obj",
    "garage_row_v2.obj",
    "trash_shelter.obj",
    "boundary_wall.obj",
    "concrete_barrier.obj",
    "trash_bin_lowpoly.obj",
}
OUTWARD_NORMAL_ASSET_IDS = {
    "block13_core",
    "shop_zenona_v2",
    "shop_zenona_v3",
    "garage_row_v2",
    "trash_shelter",
    "boundary_wall",
    "concrete_barrier",
    "trash_bin_lowpoly",
}
SOLID_BUILDING_SHELL_ASSET_IDS = {"shop_zenona_v3"}
BOOL_METADATA_KEYS = {"cameraBlocks", "requiresClosedMesh", "allowOpenEdges", "allowFloating", "renderInGameplay"}


@dataclass
class ManifestEntry:
    line: int
    asset_id: str
    model_path: str
    fallback_size: tuple[float, float, float]
    fallback_color: tuple[int, int, int, int]
    metadata: dict[str, str]


@dataclass
class ObjFace:
    line: int
    vertex_indices: list[int]
    texture_indices: list[int | None]
    normal_indices: list[int | None]


@dataclass
class ObjMesh:
    vertices: list[tuple[float, float, float]]
    normals: list[tuple[float, float, float]]
    texture_coordinates: list[tuple[float, ...]]
    faces: list[ObjFace]

    @property
    def min_y(self) -> float:
        return min(vertex[1] for vertex in self.vertices)

    @property
    def max_y(self) -> float:
        return max(vertex[1] for vertex in self.vertices)


def parse_csv_floats(value: str, count: int) -> tuple[float, ...]:
    parts = [part.strip() for part in value.split(",")]
    if len(parts) != count:
        raise ValueError(f"expected {count} comma-separated values")
    return tuple(float(part) for part in parts)


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


def parse_tag_list(value: str) -> list[str]:
    return [tag.strip() for tag in value.split(",") if tag.strip()]


def entry_tags(entry: ManifestEntry) -> set[str]:
    return set(parse_tag_list(entry.metadata.get("tags", "")))


def entry_type(entry: ManifestEntry) -> str:
    return entry.metadata.get("type", "SolidProp")


def entry_usage(entry: ManifestEntry) -> str:
    return entry.metadata.get("usage", "active")


def entry_render_bucket(entry: ManifestEntry) -> str:
    return entry.metadata.get("render", "Translucent" if entry.fallback_color[3] < 255 else "Opaque")


def entry_collision(entry: ManifestEntry) -> str:
    return entry.metadata.get("collisionProfile", entry.metadata.get("collision", "Unspecified"))


def metadata_bool(entry: ManifestEntry, key: str, default: bool = False) -> bool:
    return entry.metadata.get(key, str(default).lower()).lower() == "true"


def visual_offset_y(entry: ManifestEntry, issues: list[str], prefix: str) -> float:
    value = entry.metadata.get("visualOffset")
    if value is None:
        return 0.0
    try:
        return parse_csv_floats(value, 3)[1]
    except ValueError as error:
        issues.append(f"{prefix} has bad visualOffset: {error}")
        return 0.0


def allows_floating_origin(entry: ManifestEntry) -> bool:
    return entry.metadata.get("allowFloating", "false").lower() == "true"


def should_validate_bottom_center(entry: ManifestEntry) -> bool:
    return (
        entry.metadata.get("origin", "bottom_center") == "bottom_center"
        and entry.metadata.get("render") != "Vehicle"
        and not allows_floating_origin(entry)
    )


def resolve_obj_index(index: int, count: int) -> int | None:
    resolved = count + index if index < 0 else index - 1
    return resolved if 0 <= resolved < count else None


def load_manifest(path: Path, issues: list[str]) -> list[ManifestEntry]:
    entries: list[ManifestEntry] = []
    seen: dict[str, int] = {}
    for line_number, raw_line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue

        parts = [part.strip() for part in line.split("|")]
        if len(parts) not in (4, 5):
            issues.append(f"{path}:{line_number}: expected id|modelPath|fallbackSize|fallbackColor|metadata")
            continue

        asset_id, model_path, size_text, color_text = parts[:4]
        if not asset_id or not model_path:
            issues.append(f"{path}:{line_number}: asset id and model path must be non-empty")
            continue
        if asset_id in seen:
            issues.append(f"{path}:{line_number}: duplicate asset id '{asset_id}', first seen on line {seen[asset_id]}")
            continue

        try:
            fallback_size = parse_csv_floats(size_text, 3)
            fallback_color = parse_color(color_text)
            metadata = parse_metadata(parts[4]) if len(parts) == 5 else {}
        except ValueError as error:
            issues.append(f"{path}:{line_number}: {error}")
            continue

        seen[asset_id] = line_number
        entries.append(ManifestEntry(line_number, asset_id, model_path, fallback_size, fallback_color, metadata))
    return entries


def parse_obj(path: Path, issues: list[str]) -> ObjMesh | None:
    vertices: list[tuple[float, float, float]] = []
    normals: list[tuple[float, float, float]] = []
    texture_coordinates: list[tuple[float, ...]] = []
    faces: list[ObjFace] = []
    for line_number, raw_line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        line = raw_line.strip()
        if line.startswith("v "):
            parts = line.split()
            if len(parts) < 4:
                issues.append(f"{path}:{line_number}: malformed vertex line")
                continue
            vertices.append((float(parts[1]), float(parts[2]), float(parts[3])))
        elif line.startswith("vn "):
            parts = line.split()
            if len(parts) < 4:
                issues.append(f"{path}:{line_number}: malformed normal line")
                continue
            normals.append((float(parts[1]), float(parts[2]), float(parts[3])))
        elif line.startswith("vt "):
            parts = line.split()
            if len(parts) < 3:
                issues.append(f"{path}:{line_number}: malformed texture coordinate line")
                continue
            texture_coordinates.append(tuple(float(part) for part in parts[1:]))
        elif line.startswith("f "):
            vertex_indices: list[int] = []
            texture_indices: list[int | None] = []
            normal_indices: list[int | None] = []
            face_is_valid = True
            for token in line.split()[1:]:
                parts = token.split("/")
                try:
                    vertex_index = resolve_obj_index(int(parts[0]), len(vertices))
                except ValueError:
                    vertex_index = None
                if vertex_index is None:
                    issues.append(f"{path}:{line_number}: face references invalid vertex '{token}'")
                    face_is_valid = False
                    continue

                texture_index = None
                if len(parts) >= 2 and parts[1]:
                    try:
                        texture_index = resolve_obj_index(int(parts[1]), len(texture_coordinates))
                    except ValueError:
                        texture_index = None
                    if texture_index is None:
                        issues.append(f"{path}:{line_number}: face references invalid texture coordinate '{token}'")
                        face_is_valid = False

                normal_index = None
                if len(parts) >= 3 and parts[2]:
                    try:
                        normal_index = resolve_obj_index(int(parts[2]), len(normals))
                    except ValueError:
                        normal_index = None
                    if normal_index is None:
                        issues.append(f"{path}:{line_number}: face references invalid normal '{token}'")
                        face_is_valid = False

                vertex_indices.append(vertex_index)
                texture_indices.append(texture_index)
                normal_indices.append(normal_index)
            if len(vertex_indices) < 3:
                issues.append(f"{path}:{line_number}: face has fewer than three vertices")
            elif face_is_valid:
                faces.append(ObjFace(line_number, vertex_indices, texture_indices, normal_indices))

    if not vertices:
        issues.append(f"{path}: OBJ has no vertices")
        return None
    if not faces:
        issues.append(f"{path}: OBJ has no faces")
        return None
    return ObjMesh(vertices, normals, texture_coordinates, faces)


def vector_length(vector: tuple[float, float, float]) -> float:
    return math.sqrt(vector[0] * vector[0] + vector[1] * vector[1] + vector[2] * vector[2])


def normalized(vector: tuple[float, float, float]) -> tuple[float, float, float]:
    length = vector_length(vector)
    if length <= EPSILON:
        return (0.0, 0.0, 0.0)
    return (vector[0] / length, vector[1] / length, vector[2] / length)


def dot(lhs: tuple[float, float, float], rhs: tuple[float, float, float]) -> float:
    return lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2]


def mesh_aabb_bounds(mesh: ObjMesh) -> tuple[tuple[float, float, float], tuple[float, float, float]]:
    mins = tuple(min(vertex[axis] for vertex in mesh.vertices) for axis in range(3))
    maxs = tuple(max(vertex[axis] for vertex in mesh.vertices) for axis in range(3))
    return mins, maxs


def face_normal(mesh: ObjMesh, face: ObjFace) -> tuple[float, float, float]:
    ax, ay, az = mesh.vertices[face.vertex_indices[0]]
    bx, by, bz = mesh.vertices[face.vertex_indices[1]]
    cx, cy, cz = mesh.vertices[face.vertex_indices[2]]
    ab = (bx - ax, by - ay, bz - az)
    ac = (cx - ax, cy - ay, cz - az)
    return (
        ab[1] * ac[2] - ab[2] * ac[1],
        ab[2] * ac[0] - ab[0] * ac[2],
        ab[0] * ac[1] - ab[1] * ac[0],
    )


def face_normal_y(mesh: ObjMesh, face: ObjFace) -> float:
    return face_normal(mesh, face)[1]


def edge_counts(mesh: ObjMesh) -> Counter[tuple[int, int]]:
    counts: Counter[tuple[int, int]] = Counter()
    for face in mesh.faces:
        indices = face.vertex_indices
        for index, start in enumerate(indices):
            end = indices[(index + 1) % len(indices)]
            counts[tuple(sorted((start, end)))] += 1
    return counts


def allows_single_sided_faces(entry: ManifestEntry) -> bool:
    tags = entry_tags(entry)
    return entry_type(entry) == "Decal" or bool(tags & {"decal", "ground_patch", "surface"}) or entry_collision(entry) == "Ground"


def should_validate_outward_normals(entry: ManifestEntry, path: Path) -> bool:
    return path.name in OUTWARD_NORMAL_MODEL_FILENAMES or entry.asset_id in OUTWARD_NORMAL_ASSET_IDS


def should_validate_solid_building_shell(entry: ManifestEntry) -> bool:
    return (
        entry_type(entry) == "SolidBuilding"
        or metadata_bool(entry, "requiresClosedMesh")
        or entry.asset_id in SOLID_BUILDING_SHELL_ASSET_IDS
    )


def validate_obj_normals(path: Path, mesh: ObjMesh, issues: list[str]) -> None:
    if not mesh.normals:
        issues.append(f"{path}: OBJ has no vn normal vectors")

    for normal_index, normal in enumerate(mesh.normals, start=1):
        length = vector_length(normal)
        if length <= EPSILON:
            issues.append(f"{path}: normal {normal_index} has zero length")
        elif abs(length - 1.0) > 0.01:
            issues.append(f"{path}: normal {normal_index} is not unit length")

    for face in mesh.faces:
        if any(normal_index is None for normal_index in face.normal_indices):
            issues.append(f"{path}:{face.line}: face must reference vn normals with v//vn or v/vt/vn")
            continue

        if len(face.vertex_indices) != 3:
            continue

        geometric_normal = normalized(face_normal(mesh, face))
        if vector_length(geometric_normal) <= EPSILON:
            issues.append(f"{path}:{face.line}: face has zero-area normal")
            continue

        for normal_index in face.normal_indices:
            assert normal_index is not None
            if dot(geometric_normal, normalized(mesh.normals[normal_index])) < 0.5:
                issues.append(f"{path}:{face.line}: referenced normal disagrees with face winding")
                break


def validate_obj_outward_normals(entry: ManifestEntry, path: Path, mesh: ObjMesh, issues: list[str]) -> None:
    if not should_validate_outward_normals(entry, path):
        return

    mins, maxs = mesh_aabb_bounds(mesh)
    axes = ("X", "Y", "Z")
    for face in mesh.faces:
        if len(face.vertex_indices) != 3:
            continue
        geometric_normal = normalized(face_normal(mesh, face))
        if vector_length(geometric_normal) <= EPSILON:
            continue
        for axis, axis_name in enumerate(axes):
            coordinates = [mesh.vertices[index][axis] for index in face.vertex_indices]
            if max(abs(coordinate - mins[axis]) for coordinate in coordinates) <= 0.001:
                if geometric_normal[axis] >= -EPSILON:
                    issues.append(
                        f"{path}:{face.line}: min{axis_name} face winds inward for solid asset '{entry.asset_id}'"
                    )
                break
            if max(abs(coordinate - maxs[axis]) for coordinate in coordinates) <= 0.001:
                if geometric_normal[axis] <= EPSILON:
                    issues.append(
                        f"{path}:{face.line}: max{axis_name} face winds inward for solid asset '{entry.asset_id}'"
                    )
                break


def validate_obj_triangles(path: Path, mesh: ObjMesh, issues: list[str]) -> None:
    for face in mesh.faces:
        if len(face.vertex_indices) != 3:
            issues.append(
                f"{path}:{face.line}: face has {len(face.vertex_indices)} vertices; OBJ faces must be triangulated"
            )


def validate_obj_degenerate_triangles(path: Path, mesh: ObjMesh, issues: list[str]) -> None:
    for face in mesh.faces:
        if len(face.vertex_indices) != 3:
            continue
        if vector_length(face_normal(mesh, face)) <= EPSILON:
            issues.append(f"{path}:{face.line}: face is a degenerate triangle")


def validate_obj_face_spans(entry: ManifestEntry, path: Path, mesh: ObjMesh, issues: list[str]) -> None:
    if not should_validate_solid_building_shell(entry):
        return

    mins, maxs = mesh_aabb_bounds(mesh)
    diagonal = vector_length((maxs[0] - mins[0], maxs[1] - mins[1], maxs[2] - mins[2]))
    if diagonal <= EPSILON:
        return

    max_allowed_edge = diagonal * 0.92
    for face in mesh.faces:
        if len(face.vertex_indices) != 3:
            continue
        positions = [mesh.vertices[index] for index in face.vertex_indices]
        for start, end in ((0, 1), (1, 2), (2, 0)):
            edge = (
                positions[end][0] - positions[start][0],
                positions[end][1] - positions[start][1],
                positions[end][2] - positions[start][2],
            )
            if vector_length(edge) > max_allowed_edge:
                issues.append(f"{path}:{face.line}: face spans an unexpectedly large distance for solid shell '{entry.asset_id}'")
                break


def validate_obj_top_faces(path: Path, mesh: ObjMesh, issues: list[str]) -> None:
    max_y = mesh.max_y
    for face in mesh.faces:
        if len(face.vertex_indices) != 3:
            continue
        ys = [mesh.vertices[index][1] for index in face.vertex_indices]
        if max(abs(y - max_y) for y in ys) <= 0.001 and max(ys) - min(ys) <= 0.001:
            if face_normal_y(mesh, face) <= EPSILON:
                issues.append(f"{path}:{face.line}: top face winds downward; OBJ top surfaces must face +Y")


def validate_decal_mesh(entry: ManifestEntry, path: Path, mesh: ObjMesh, issues: list[str]) -> None:
    if entry_type(entry) != "Decal":
        return

    height = mesh.max_y - mesh.min_y
    max_height = float(entry.metadata.get("maxHeight", "0.02"))
    if height > max_height + 0.001:
        issues.append(f"{path}: Decal '{entry.asset_id}' height {height:.3f}m exceeds maxHeight {max_height:.3f}m")

    for face in mesh.faces:
        if len(face.vertex_indices) != 3:
            continue
        normal = normalized(face_normal(mesh, face))
        if dot(normal, (0.0, 1.0, 0.0)) < 0.85:
            issues.append(f"{path}:{face.line}: Decal '{entry.asset_id}' face normal is not mostly +Y")


def validate_obj_backfaces(entry: ManifestEntry, path: Path, mesh: ObjMesh, issues: list[str], warnings: list[str]) -> None:
    counts = edge_counts(mesh)
    boundary_edges = sum(1 for count in counts.values() if count == 1)
    nonmanifold_edges = sum(1 for count in counts.values() if count > 2)
    explicit_open_edges_allowed = metadata_bool(entry, "allowOpenEdges") or entry_usage(entry) == "legacy"
    if boundary_edges > 0 and should_validate_solid_building_shell(entry) and not explicit_open_edges_allowed:
        issues.append(
            f"{path}: solid shell '{entry.asset_id}' has {boundary_edges} open boundary edges"
        )
    elif boundary_edges > 0 and not explicit_open_edges_allowed and not allows_single_sided_faces(entry):
        warnings.append(
            f"{path}: '{entry.asset_id}' has {boundary_edges} open boundary edges; "
            "use closed thin geometry or an explicit double-sided material only for sign/poster-style assets"
        )
    if nonmanifold_edges > 0 and should_validate_solid_building_shell(entry) and entry_usage(entry) != "legacy":
        issues.append(f"{path}: solid shell '{entry.asset_id}' has {nonmanifold_edges} non-manifold edges")
    elif nonmanifold_edges > 0 and entry_usage(entry) != "legacy":
        warnings.append(f"{path}: '{entry.asset_id}' has {nonmanifold_edges} non-manifold edges")


def validate_obj(entry: ManifestEntry, path: Path, mesh: ObjMesh, issues: list[str], warnings: list[str]) -> None:
    validate_obj_triangles(path, mesh, issues)
    validate_obj_degenerate_triangles(path, mesh, issues)
    validate_obj_normals(path, mesh, issues)
    validate_obj_outward_normals(entry, path, mesh, issues)
    validate_obj_face_spans(entry, path, mesh, issues)
    validate_obj_top_faces(path, mesh, issues)
    validate_decal_mesh(entry, path, mesh, issues)
    validate_obj_backfaces(entry, path, mesh, issues, warnings)

    if should_validate_bottom_center(entry):
        offset_y = visual_offset_y(entry, issues, f"{path}")
        if abs(mesh.min_y + offset_y) > 0.001:
            issues.append(f"{path}: minY {mesh.min_y:.3f} should be 0 for bottom_center origin")

    if path.name in {"road_asphalt.obj", "parking_surface.obj"}:
        for face_index, face in enumerate(mesh.faces, start=1):
            if face_normal_y(mesh, face) <= EPSILON:
                issues.append(f"{path}: face {face_index} winds downward; ground/render surfaces must face +Y")

    if path.name == "irregular_patch.obj" or entry.asset_id.startswith("irregular_"):
        height = mesh.max_y - mesh.min_y
        if height > 0.05:
            issues.append(f"{path}: patch height {height:.3f}m exceeds decal-thin limit 0.05m")

    if path.name == "bench.obj" and abs(mesh.min_y) > 0.001:
            issues.append(f"{path}: bench minY {mesh.min_y:.3f} should be 0 for base-on-ground authoring")


def validate_metadata_contract(entry: ManifestEntry, manifest_path: Path, issues: list[str]) -> None:
    prefix = f"{manifest_path}:{entry.line}: '{entry.asset_id}'"
    for key in entry.metadata:
        if key not in KNOWN_METADATA_KEYS:
            issues.append(f"{prefix} uses unknown metadata key '{key}'")

    asset_type = entry_type(entry)
    if asset_type not in KNOWN_ASSET_TYPES:
        issues.append(f"{prefix} uses unknown asset type '{asset_type}'")

    usage = entry_usage(entry)
    if usage not in KNOWN_ASSET_USAGES:
        issues.append(f"{prefix} uses unknown asset usage '{usage}'")

    origin = entry.metadata.get("origin")
    if origin is not None and origin not in KNOWN_ORIGINS:
        issues.append(f"{prefix} uses unknown origin '{origin}'")

    render = entry_render_bucket(entry)
    if render not in KNOWN_RENDER_BUCKETS:
        issues.append(f"{prefix} uses unknown render bucket '{render}'")

    collision = entry_collision(entry)
    if collision not in KNOWN_COLLISION_INTENTS:
        issues.append(f"{prefix} uses unknown collision intent '{collision}'")

    for key in BOOL_METADATA_KEYS:
        value = entry.metadata.get(key)
        if value is not None and value.lower() not in {"true", "false"}:
            issues.append(f"{prefix} uses invalid {key} '{value}'")

    max_height = entry.metadata.get("maxHeight")
    if max_height is not None:
        try:
            parsed_max_height = float(max_height)
            if not math.isfinite(parsed_max_height) or parsed_max_height < 0.0:
                raise ValueError("maxHeight must be finite and non-negative")
        except ValueError as error:
            issues.append(f"{prefix} has bad maxHeight: {error}")

    visual_offset = entry.metadata.get("visualOffset")
    if visual_offset is not None:
        try:
            parse_csv_floats(visual_offset, 3)
        except ValueError as error:
            issues.append(f"{prefix} has bad visualOffset: {error}")

    tags_text = entry.metadata.get("tags")
    if tags_text is not None:
        tags = parse_tag_list(tags_text)
        seen_tags: set[str] = set()
        for tag in tags:
            if any(character.isspace() for character in tag):
                issues.append(f"{prefix} has tag with whitespace '{tag}'")
            if tag in seen_tags:
                issues.append(f"{prefix} repeats metadata tag '{tag}'")
            seen_tags.add(tag)

    if asset_type == "SolidBuilding":
        if render != "Opaque":
            issues.append(f"{prefix} SolidBuilding must use render=Opaque")
        if collision not in {"SolidWorld", "Prop"}:
            issues.append(f"{prefix} SolidBuilding must use collisionProfile=SolidWorld or collision=Prop")
        if not metadata_bool(entry, "cameraBlocks"):
            issues.append(f"{prefix} SolidBuilding must set cameraBlocks=true")
        if not metadata_bool(entry, "requiresClosedMesh"):
            issues.append(f"{prefix} SolidBuilding must set requiresClosedMesh=true")
        if metadata_bool(entry, "allowOpenEdges"):
            issues.append(f"{prefix} SolidBuilding cannot set allowOpenEdges=true")

    if asset_type == "Decal":
        if render != "Decal":
            issues.append(f"{prefix} Decal must use render=Decal")
        if collision != "None":
            issues.append(f"{prefix} Decal must use collision=None")
        if metadata_bool(entry, "cameraBlocks"):
            issues.append(f"{prefix} Decal must set cameraBlocks=false")
        if "maxHeight" not in entry.metadata:
            issues.append(f"{prefix} Decal must set maxHeight")

    if asset_type == "Glass":
        if render != "Glass":
            issues.append(f"{prefix} Glass must use render=Glass")
        if entry.fallback_color[3] >= 255:
            issues.append(f"{prefix} Glass fallbackColor must use alpha < 255")

    if asset_type in {"DebugOnly", "CollisionProxy"}:
        if render != "DebugOnly":
            issues.append(f"{prefix} {asset_type} must use render=DebugOnly")
        if metadata_bool(entry, "renderInGameplay", True):
            issues.append(f"{prefix} {asset_type} must set renderInGameplay=false")


def validate_gltf(path: Path, issues: list[str]) -> None:
    gltf = json.loads(path.read_text(encoding="utf-8"))
    version = str(gltf.get("asset", {}).get("version", ""))
    if not version.startswith("2."):
        issues.append(f"{path}: glTF asset version must be 2.x")

    for buffer in gltf.get("buffers", []):
        uri = buffer.get("uri")
        if uri:
            buffer_path = path.parent / uri
            if not buffer_path.exists():
                issues.append(f"{path}: referenced buffer '{uri}' is missing")
                continue
            byte_length = buffer.get("byteLength")
            if isinstance(byte_length, int) and buffer_path.stat().st_size < byte_length:
                issues.append(
                    f"{path}: referenced buffer '{uri}' is shorter than declared byteLength {byte_length}"
                )

    for node in gltf.get("nodes", []):
        name = node.get("name", "")
        if name.startswith("Collider_") or node.get("extras", {}).get("collisionProxy") is True:
            issues.append(f"{path}: render glTF contains collision proxy node '{name}'")

    for mesh in gltf.get("meshes", []):
        for primitive_index, primitive in enumerate(mesh.get("primitives", []), start=1):
            attributes = primitive.get("attributes", {})
            if "POSITION" not in attributes:
                issues.append(f"{path}: mesh '{mesh.get('name', '<unnamed>')}' primitive {primitive_index} has no POSITION")
            if "NORMAL" not in attributes:
                issues.append(f"{path}: mesh '{mesh.get('name', '<unnamed>')}' primitive {primitive_index} has no NORMAL")


def validate_gltf_origin(entry: ManifestEntry, path: Path, issues: list[str]) -> None:
    if not should_validate_bottom_center(entry):
        return

    gltf = json.loads(path.read_text(encoding="utf-8"))
    accessors = gltf.get("accessors", [])
    min_y: float | None = None
    for mesh in gltf.get("meshes", []):
        for primitive in mesh.get("primitives", []):
            position_accessor = primitive.get("attributes", {}).get("POSITION")
            if not isinstance(position_accessor, int) or position_accessor >= len(accessors):
                continue
            accessor = accessors[position_accessor]
            bounds = accessor.get("min")
            if isinstance(bounds, list) and len(bounds) >= 2:
                y = float(bounds[1])
                min_y = y if min_y is None else min(min_y, y)

    if min_y is None:
        return

    offset_y = visual_offset_y(entry, issues, f"{path}")
    if abs(min_y + offset_y) > 0.001:
        issues.append(f"{path}: position minY {min_y:.3f} should be 0 for bottom_center origin")


def validate_assets(asset_root: Path) -> tuple[list[str], list[str]]:
    issues: list[str] = []
    warnings: list[str] = []
    manifest_path = asset_root / "asset_manifest.txt"
    if not manifest_path.exists():
        return [f"{manifest_path}: manifest not found"], warnings

    entries = load_manifest(manifest_path, issues)
    for entry in entries:
        validate_metadata_contract(entry, manifest_path, issues)
        model_path = asset_root / entry.model_path
        if not model_path.exists():
            issues.append(f"{manifest_path}:{entry.line}: missing model for '{entry.asset_id}': {model_path}")
            continue

        extension = model_path.suffix.lower()
        if extension == ".obj":
            mesh = parse_obj(model_path, issues)
            if mesh is not None:
                validate_obj(entry, model_path, mesh, issues, warnings)
        elif extension in {".gltf", ".glb"}:
            if extension == ".gltf":
                validate_gltf(model_path, issues)
                validate_gltf_origin(entry, model_path, issues)
        else:
            warnings.append(f"{model_path}: unrecognized model extension '{extension}'")

        if entry.fallback_color[3] < 255 and entry_render_bucket(entry) not in {"Translucent", "Glass", "Decal"}:
            issues.append(
                f"{manifest_path}:{entry.line}: '{entry.asset_id}' has alpha fallbackColor "
                "but does not declare render=Translucent, render=Glass, or render=Decal"
            )

        if any(not math.isfinite(value) or value <= 0.0 for value in entry.fallback_size):
            issues.append(f"{manifest_path}:{entry.line}: fallback size must be finite and positive")

    return issues, warnings


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate Blok 13 authored asset hygiene.")
    parser.add_argument("asset_root", nargs="?", default="data/assets", type=Path)
    parser.add_argument("--quiet", action="store_true", help="Only print failures.")
    args = parser.parse_args()

    issues, warnings = validate_assets(args.asset_root)
    if warnings and not args.quiet:
        for warning in warnings:
            print(f"warning: {warning}")
    if issues:
        for issue in issues:
            print(f"error: {issue}")
        return 1
    if not args.quiet:
        print(f"asset validation passed: {args.asset_root}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
