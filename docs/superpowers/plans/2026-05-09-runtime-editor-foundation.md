# Runtime Editor Foundation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the first runtime editor foundation for `Blok 13: Rewir`: overlay JSON data, validation, dev-only Dear ImGui shell, object inspection/editing, and add-instance support from the asset manifest.

**Architecture:** Keep the base map authored in C++ for now, then apply `data/world/block13_editor_overlay.json` on top. Keep editor data/model code independent from Dear ImGui; ImGui only renders panels and calls editor commands. Implement data and validation first, then wire the UI.

**Tech Stack:** C++17, CMake, raylib 6.0, Dear ImGui via `raylib-extras/rlImGui` for dev builds, Python 3 validators, existing `bs3d_game_support` tests.

---

## Execution Notes

This workspace currently has no `.git` directory. Do not add git commit steps unless `git rev-parse --is-inside-work-tree` succeeds in the execution environment. End each task with verification output and a short progress update in `progress.md` only when the task produces a user-visible milestone.

Dependency note, checked on 2026-05-09: `raylib-extras/rlImGui` has a Raylib 6.0 + ImGui 1.92.7 release branch/tag and can be built by adding rlImGui and ImGui files directly to the game project. Dear ImGui's upstream integration guidance says existing applications usually wire input, font texture, and rendering through backends; rlImGui handles that for raylib.

## File Structure

Create:

- `src/game/EditorOverlayData.h`: POD structs for overlay schema.
- `src/game/EditorOverlayCodec.h`: load/save API and validation result types.
- `src/game/EditorOverlayCodec.cpp`: JSON parsing/writing for overlay data.
- `src/game/EditorOverlayApply.h`: overlay-to-level apply API.
- `src/game/EditorOverlayApply.cpp`: object override and instance append logic.
- `tools/validate_editor_overlay.py`: Python validator for overlay JSON.
- `data/world/block13_editor_overlay.json`: minimal valid starter overlay.
- `src/game/RuntimeMapEditor.h`: editor model/state/commands, independent of ImGui.
- `src/game/RuntimeMapEditor.cpp`: command implementation and dirty state.
- `src/game/RuntimeMapEditorImGui.h`: ImGui UI entry points, compiled only in dev builds.
- `src/game/RuntimeMapEditorImGui.cpp`: Dear ImGui panels.

Modify:

- `CMakeLists.txt`: add overlay/editor source files, Python CTest, and dev-only ImGui/rlImGui build wiring.
- `src/game/WorldDataLoader.cpp`: load and apply overlay after existing world data is applied.
- `src/game/WorldDataLoader.h`: expose overlay apply count/warnings in `WorldDataApplyResult`.
- `src/game/GameApp.cpp`: toggle editor with `F10`, initialize/shutdown ImGui in dev builds, render editor UI, and pause gameplay input when editor captures input.
- `src/game/DevTools.h` and `src/game/DevTools.cpp`: add runtime-editor dev toggle helper if keeping dev-tool toggles centralized.
- `tests/game_support_tests.cpp`: add overlay codec/apply/editor-model tests.
- `progress.md`: record milestone after tests and smoke pass.
- `README.md`: document dev editor build/run key after UI exists.

Do not move mission/dialogue/item/shop/localization data in this milestone.

---

### Task 1: Overlay Data Types

**Files:**
- Create: `src/game/EditorOverlayData.h`
- Modify: `CMakeLists.txt`
- Test: `tests/game_support_tests.cpp`

- [ ] **Step 1: Write the failing test**

Add a test near the current intro-level data tests in `tests/game_support_tests.cpp`:

```cpp
void editorOverlayDataDefaultsAreSafe() {
    const bs3d::EditorOverlayDocument overlay;
    expect(overlay.schemaVersion == 1, "editor overlay schema starts at version 1");
    expect(overlay.overrides.empty(), "editor overlay starts with no overrides");
    expect(overlay.instances.empty(), "editor overlay starts with no instances");

    bs3d::EditorOverlayObject object;
    object.id = "editor_test_lamp";
    object.assetId = "lamp_post_lowpoly";
    object.position = {1.0f, 0.0f, 2.0f};
    object.scale = {0.18f, 3.1f, 0.18f};
    object.zoneTag = bs3d::WorldLocationTag::RoadLoop;
    object.gameplayTags = {"vertical_readability", "future_expansion"};

    expect(object.id == "editor_test_lamp", "editor overlay object keeps stable id");
    expect(object.assetId == "lamp_post_lowpoly", "editor overlay object keeps manifest asset id");
    expect(object.zoneTag == bs3d::WorldLocationTag::RoadLoop, "editor overlay object keeps zone tag");
    expect(object.gameplayTags.size() == 2, "editor overlay object keeps gameplay tags");
}
```

Call it in `main()` before the existing intro-level builder tests finish.

- [ ] **Step 2: Run test to verify it fails**

Run:

```powershell
cmake --build build --config Release
```

Expected: compile fails because `bs3d::EditorOverlayDocument` and `bs3d::EditorOverlayObject` do not exist.

- [ ] **Step 3: Add minimal data header**

Create `src/game/EditorOverlayData.h`:

```cpp
#pragma once

#include "WorldArtTypes.h"

#include <string>
#include <vector>

namespace bs3d {

struct EditorOverlayObject {
    std::string id;
    std::string assetId;
    Vec3 position{};
    Vec3 scale{1.0f, 1.0f, 1.0f};
    float yawRadians = 0.0f;
    WorldLocationTag zoneTag = WorldLocationTag::Unknown;
    std::vector<std::string> gameplayTags;
};

struct EditorOverlayDocument {
    int schemaVersion = 1;
    std::vector<EditorOverlayObject> overrides;
    std::vector<EditorOverlayObject> instances;
};

} // namespace bs3d
```

Include it in `tests/game_support_tests.cpp`:

```cpp
#include "EditorOverlayData.h"
```

- [ ] **Step 4: Run test to verify it passes**

Run:

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Expected: build succeeds and all current CTest tests pass.

---

### Task 2: Overlay Codec Parse And Save

**Files:**
- Create: `src/game/EditorOverlayCodec.h`
- Create: `src/game/EditorOverlayCodec.cpp`
- Modify: `CMakeLists.txt`
- Test: `tests/game_support_tests.cpp`

- [ ] **Step 1: Write failing parse/save tests**

Add tests:

```cpp
void editorOverlayCodecParsesValidOverlay() {
    const std::string text =
        "{"
        "\"schemaVersion\":1,"
        "\"overrides\":[{\"id\":\"sign_no_parking\",\"assetId\":\"street_sign\","
        "\"position\":[-3.65,0.0,5.7],\"yawRadians\":0.25,"
        "\"scale\":[0.58,1.75,0.08],\"zoneTag\":\"Parking\","
        "\"gameplayTags\":[\"vertical_readability\",\"drive_readability\"]}],"
        "\"instances\":[{\"id\":\"editor_main_artery_lamp_0\",\"assetId\":\"lamp_post_lowpoly\","
        "\"position\":[18.0,0.0,-27.0],\"yawRadians\":0.0,"
        "\"scale\":[0.18,3.1,0.18],\"zoneTag\":\"RoadLoop\","
        "\"gameplayTags\":[\"vertical_readability\",\"future_expansion\"]}]"
        "}";

    const bs3d::EditorOverlayLoadResult loaded = bs3d::parseEditorOverlay(text);

    expect(loaded.success, "valid editor overlay parses");
    expect(loaded.document.overrides.size() == 1, "valid overlay has one override");
    expect(loaded.document.instances.size() == 1, "valid overlay has one instance");
    expect(loaded.document.overrides[0].id == "sign_no_parking", "override id parsed");
    expect(loaded.document.overrides[0].zoneTag == bs3d::WorldLocationTag::Parking, "zone tag parsed");
    expectNear(loaded.document.instances[0].position.z, -27.0f, 0.001f, "instance position parsed");
}

void editorOverlayCodecRejectsBadSchema() {
    const bs3d::EditorOverlayLoadResult loaded =
        bs3d::parseEditorOverlay("{\"schemaVersion\":99,\"overrides\":[],\"instances\":[]}");

    expect(!loaded.success, "unsupported editor overlay schema fails");
    expect(!loaded.warnings.empty(), "unsupported schema reports warning");
}

void editorOverlayCodecRoundTripsMinimalDocument() {
    bs3d::EditorOverlayDocument document;
    bs3d::EditorOverlayObject instance;
    instance.id = "editor_test_bin";
    instance.assetId = "trash_bin_lowpoly";
    instance.position = {8.0f, 0.0f, -4.0f};
    instance.scale = {0.78f, 1.05f, 0.68f};
    instance.zoneTag = bs3d::WorldLocationTag::Trash;
    instance.gameplayTags = {"trash_dressing"};
    document.instances.push_back(instance);

    const std::string saved = bs3d::serializeEditorOverlay(document);
    const bs3d::EditorOverlayLoadResult loaded = bs3d::parseEditorOverlay(saved);

    expect(loaded.success, "serialized editor overlay parses back");
    expect(loaded.document.instances.size() == 1, "round-trip keeps instance");
    expect(loaded.document.instances[0].assetId == "trash_bin_lowpoly", "round-trip keeps asset id");
    expect(loaded.document.instances[0].zoneTag == bs3d::WorldLocationTag::Trash, "round-trip keeps zone");
}
```

- [ ] **Step 2: Run test to verify it fails**

Run:

```powershell
cmake --build build --config Release
```

Expected: compile fails because `EditorOverlayLoadResult`, `parseEditorOverlay`, and `serializeEditorOverlay` do not exist.

- [ ] **Step 3: Add codec API**

Create `src/game/EditorOverlayCodec.h`:

```cpp
#pragma once

#include "EditorOverlayData.h"

#include <string>
#include <vector>

namespace bs3d {

struct EditorOverlayLoadResult {
    bool success = false;
    EditorOverlayDocument document{};
    std::vector<std::string> warnings;
};

EditorOverlayLoadResult parseEditorOverlay(const std::string& text);
EditorOverlayLoadResult loadEditorOverlayFile(const std::string& path);
std::string serializeEditorOverlay(const EditorOverlayDocument& document);
bool saveEditorOverlayFile(const std::string& path,
                           const EditorOverlayDocument& document,
                           std::vector<std::string>& warnings);

const char* worldLocationTagName(WorldLocationTag tag);
WorldLocationTag worldLocationTagFromName(const std::string& name);

} // namespace bs3d
```

- [ ] **Step 4: Add codec implementation**

Create `src/game/EditorOverlayCodec.cpp`.

Implementation requirements:

- Put a small JSON parser in an anonymous namespace. Copy the existing `JsonType`, `JsonValue`, and `JsonParser` shape from `src/game/WorldDataLoader.cpp`, then add string-array reading.
- Parse only the schema used by overlay files: object, array, number, string.
- Implement zone mapping with exact names: `Unknown`, `Block`, `Shop`, `Parking`, `Garage`, `Trash`, `RoadLoop`.
- Serialize deterministic JSON with two-space indentation and stable field order: `id`, `assetId`, `position`, `yawRadians`, `scale`, `zoneTag`, `gameplayTags`.

The key public functions should behave like this:

```cpp
EditorOverlayLoadResult parseEditorOverlay(const std::string& text) {
    EditorOverlayLoadResult result;
    if (text.empty()) {
        result.success = true;
        return result;
    }
    const JsonValue root = JsonParser(text).parse();
    if (root.type != JsonType::Object) {
        result.warnings.push_back("editor overlay root must be an object");
        return result;
    }
    result.document.schemaVersion = intMember(root, "schemaVersion", 0);
    if (result.document.schemaVersion != 1) {
        result.warnings.push_back("unsupported editor overlay schemaVersion");
        return result;
    }
    result.document.overrides = parseObjectArray(root, "overrides", result.warnings);
    result.document.instances = parseObjectArray(root, "instances", result.warnings);
    result.success = result.warnings.empty();
    return result;
}
```

Add the source to `bs3d_game_support` in `CMakeLists.txt`.

- [ ] **Step 5: Run tests to verify pass**

Run:

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Expected: all CTest tests pass.

---

### Task 3: Overlay Apply To Intro Level

**Files:**
- Create: `src/game/EditorOverlayApply.h`
- Create: `src/game/EditorOverlayApply.cpp`
- Modify: `CMakeLists.txt`
- Test: `tests/game_support_tests.cpp`

- [ ] **Step 1: Write failing apply tests**

Add tests:

```cpp
void editorOverlayApplyOverridesExistingObject() {
    bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::EditorOverlayDocument overlay;
    bs3d::EditorOverlayObject overrideObject;
    overrideObject.id = "sign_no_parking";
    overrideObject.assetId = "street_sign";
    overrideObject.position = {-4.25f, 0.0f, 5.9f};
    overrideObject.scale = {0.58f, 1.75f, 0.08f};
    overrideObject.zoneTag = bs3d::WorldLocationTag::Parking;
    overrideObject.gameplayTags = {"vertical_readability", "drive_readability", "editor_adjusted"};
    overlay.overrides.push_back(overrideObject);

    const bs3d::EditorOverlayApplyResult result = bs3d::applyEditorOverlay(level, overlay);

    const bs3d::WorldObject* object = findObject(level, "sign_no_parking");
    expect(result.appliedOverrides == 1, "editor overlay applies one override");
    expect(object != nullptr, "overridden base object still exists");
    expectNear(object->position.x, -4.25f, 0.001f, "override changes object position");
    expect(hasTag(*object, "editor_adjusted"), "override replaces gameplay tags");
}

void editorOverlayApplyAppendsNewManifestInstance() {
    bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    const std::size_t beforeCount = level.objects.size();
    bs3d::EditorOverlayDocument overlay;
    bs3d::EditorOverlayObject instance;
    instance.id = "editor_main_artery_lamp_0";
    instance.assetId = "lamp_post_lowpoly";
    instance.position = {18.0f, 0.0f, -27.0f};
    instance.scale = {0.18f, 3.1f, 0.18f};
    instance.zoneTag = bs3d::WorldLocationTag::RoadLoop;
    instance.gameplayTags = {"vertical_readability", "future_expansion"};
    overlay.instances.push_back(instance);

    const bs3d::EditorOverlayApplyResult result = bs3d::applyEditorOverlay(level, overlay);

    const bs3d::WorldObject* object = findObject(level, "editor_main_artery_lamp_0");
    expect(result.appliedInstances == 1, "editor overlay applies one instance");
    expect(level.objects.size() == beforeCount + 1, "editor overlay appends object");
    expect(object != nullptr && object->assetId == "lamp_post_lowpoly", "editor instance uses manifest asset id");
    expect(object != nullptr && object->collision.kind == bs3d::WorldCollisionShapeKind::None,
           "editor instance defaults to no authored collision before normalization");
}

void editorOverlayApplyRejectsDuplicateInstanceId() {
    bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::EditorOverlayDocument overlay;
    bs3d::EditorOverlayObject instance;
    instance.id = "sign_no_parking";
    instance.assetId = "lamp_post_lowpoly";
    instance.scale = {0.18f, 3.1f, 0.18f};
    overlay.instances.push_back(instance);

    const bs3d::EditorOverlayApplyResult result = bs3d::applyEditorOverlay(level, overlay);

    expect(result.appliedInstances == 0, "duplicate editor instance is not applied");
    expect(!result.warnings.empty(), "duplicate editor instance reports warning");
}
```

- [ ] **Step 2: Run test to verify it fails**

Run:

```powershell
cmake --build build --config Release
```

Expected: compile fails because `EditorOverlayApplyResult` and `applyEditorOverlay` do not exist.

- [ ] **Step 3: Implement apply API**

Create `src/game/EditorOverlayApply.h`:

```cpp
#pragma once

#include "EditorOverlayData.h"
#include "IntroLevelBuilder.h"

#include <string>
#include <vector>

namespace bs3d {

struct EditorOverlayApplyResult {
    int appliedOverrides = 0;
    int appliedInstances = 0;
    std::vector<std::string> warnings;
};

EditorOverlayApplyResult applyEditorOverlay(IntroLevelData& level,
                                            const EditorOverlayDocument& overlay);

} // namespace bs3d
```

Create `src/game/EditorOverlayApply.cpp`:

```cpp
#include "EditorOverlayApply.h"

#include <unordered_set>

namespace bs3d {

namespace {

WorldObject toWorldObject(const EditorOverlayObject& object) {
    WorldObject world;
    world.id = object.id;
    world.assetId = object.assetId;
    world.position = object.position;
    world.scale = object.scale;
    world.yawRadians = object.yawRadians;
    world.zoneTag = object.zoneTag;
    world.gameplayTags = object.gameplayTags;
    world.collision.kind = WorldCollisionShapeKind::None;
    return world;
}

} // namespace

EditorOverlayApplyResult applyEditorOverlay(IntroLevelData& level,
                                            const EditorOverlayDocument& overlay) {
    EditorOverlayApplyResult result;
    std::unordered_set<std::string> ids;
    for (WorldObject& object : level.objects) {
        ids.insert(object.id);
        for (const EditorOverlayObject& overrideObject : overlay.overrides) {
            if (overrideObject.id != object.id) {
                continue;
            }
            object.assetId = overrideObject.assetId;
            object.position = overrideObject.position;
            object.scale = overrideObject.scale;
            object.yawRadians = overrideObject.yawRadians;
            object.zoneTag = overrideObject.zoneTag;
            object.gameplayTags = overrideObject.gameplayTags;
            ++result.appliedOverrides;
        }
    }
    for (const EditorOverlayObject& overrideObject : overlay.overrides) {
        if (ids.find(overrideObject.id) == ids.end()) {
            result.warnings.push_back("overlay override target not found: " + overrideObject.id);
        }
    }
    for (const EditorOverlayObject& instance : overlay.instances) {
        if (!ids.insert(instance.id).second) {
            result.warnings.push_back("overlay instance id collides: " + instance.id);
            continue;
        }
        level.objects.push_back(toWorldObject(instance));
        ++result.appliedInstances;
    }
    return result;
}

} // namespace bs3d
```

Add the source to `bs3d_game_support` in `CMakeLists.txt`.

- [ ] **Step 4: Run tests to verify pass**

Run:

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Expected: all CTest tests pass.

---

### Task 4: Overlay Load Integration

**Files:**
- Modify: `src/game/WorldDataLoader.h`
- Modify: `src/game/WorldDataLoader.cpp`
- Create: `data/world/block13_editor_overlay.json`
- Test: `tests/game_support_tests.cpp`

- [ ] **Step 1: Write failing integration test**

Add test:

```cpp
void worldDataCatalogAppliesEditorOverlayAfterBaseMap() {
    bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::WorldDataCatalog catalog;
    catalog.loaded = true;
    catalog.world.loaded = true;
    catalog.world.playerSpawn = level.playerStart;
    catalog.world.vehicleSpawn = level.vehicleStart;
    catalog.world.npcSpawn = level.npcPosition;
    catalog.world.shopPosition = level.shopPosition;
    catalog.world.dropoffPosition = level.dropoffPosition;
    catalog.mission.loaded = true;
    catalog.mission.phases.push_back({"ReachVehicle", "Wsiadź do auta.", "player_enters_vehicle"});
    catalog.editorOverlay.loaded = true;
    catalog.editorOverlay.document.instances.push_back(
        {"editor_catalog_lamp", "lamp_post_lowpoly", {3.0f, 0.0f, -24.0f}, {0.18f, 3.1f, 0.18f}, 0.0f,
         bs3d::WorldLocationTag::RoadLoop, {"vertical_readability"}});

    const bs3d::WorldDataApplyResult result = bs3d::applyWorldDataCatalog(level, catalog);

    expect(result.applied, "world data catalog still applies");
    expect(result.appliedEditorOverlayInstances == 1, "catalog applies editor overlay instances");
    expect(findObject(level, "editor_catalog_lamp") != nullptr, "editor overlay instance exists after catalog apply");
}
```

- [ ] **Step 2: Run test to verify it fails**

Run:

```powershell
cmake --build build --config Release
```

Expected: compile fails because `WorldDataCatalog` has no `editorOverlay` field and apply result has no overlay counts.

- [ ] **Step 3: Extend catalog structs**

In `src/game/WorldDataLoader.h`, include `EditorOverlayCodec.h` and add:

```cpp
struct WorldEditorOverlayData {
    bool loaded = false;
    EditorOverlayDocument document{};
    std::vector<std::string> warnings;
};
```

Add to `WorldDataCatalog`:

```cpp
WorldEditorOverlayData editorOverlay{};
```

Add to `WorldDataApplyResult`:

```cpp
int appliedEditorOverlayOverrides = 0;
int appliedEditorOverlayInstances = 0;
std::vector<std::string> warnings;
```

- [ ] **Step 4: Load overlay file**

In `loadWorldDataCatalog`, after mission load:

```cpp
const std::filesystem::path overlayPath = root / "world" / "block13_editor_overlay.json";
if (std::filesystem::exists(overlayPath)) {
    const EditorOverlayLoadResult overlay = loadEditorOverlayFile(overlayPath.string());
    catalog.editorOverlay.loaded = overlay.success;
    catalog.editorOverlay.document = overlay.document;
    catalog.editorOverlay.warnings = overlay.warnings;
    for (const std::string& warning : overlay.warnings) {
        catalog.warnings.push_back("editor overlay: " + warning);
    }
}
```

In `applyWorldDataCatalog`, after `level = IntroLevelBuilder::build(config);`:

```cpp
if (catalog.editorOverlay.loaded) {
    const EditorOverlayApplyResult overlay = applyEditorOverlay(level, catalog.editorOverlay.document);
    result.appliedEditorOverlayOverrides = overlay.appliedOverrides;
    result.appliedEditorOverlayInstances = overlay.appliedInstances;
    result.warnings.insert(result.warnings.end(), overlay.warnings.begin(), overlay.warnings.end());
}
```

- [ ] **Step 5: Add starter overlay**

Create `data/world/block13_editor_overlay.json`:

```json
{
  "schemaVersion": 1,
  "overrides": [],
  "instances": []
}
```

- [ ] **Step 6: Run tests**

Run:

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Expected: all CTest tests pass.

---

### Task 5: Python Overlay Validator

**Files:**
- Create: `tools/validate_editor_overlay.py`
- Modify: `CMakeLists.txt`
- Test: `tools/validate_editor_overlay.py`

- [ ] **Step 1: Write validator with self-tests embedded as functions**

Create `tools/validate_editor_overlay.py` with:

```python
#!/usr/bin/env python3
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
```

- [ ] **Step 2: Run validator against starter overlay**

Run:

```powershell
python tools\validate_editor_overlay.py data\world\block13_editor_overlay.json --asset-root data\assets
```

Expected: `editor overlay validation passed: data\world\block13_editor_overlay.json`

- [ ] **Step 3: Add CTest hook**

In `CMakeLists.txt`, inside the existing `if(Python3_Interpreter_FOUND)` block, add:

```cmake
add_test(NAME bs3d_editor_overlay_validation
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/validate_editor_overlay.py
            ${CMAKE_SOURCE_DIR}/data/world/block13_editor_overlay.json
            --asset-root ${CMAKE_SOURCE_DIR}/data/assets
)
```

- [ ] **Step 4: Run all tests**

Run:

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Expected: CTest now includes `bs3d_editor_overlay_validation` and all tests pass.

---

### Task 6: RuntimeMapEditor Model

**Files:**
- Create: `src/game/RuntimeMapEditor.h`
- Create: `src/game/RuntimeMapEditor.cpp`
- Modify: `CMakeLists.txt`
- Test: `tests/game_support_tests.cpp`

- [ ] **Step 1: Write failing editor-model test**

Add test:

```cpp
void runtimeMapEditorEditsSelectedObjectAndTracksDirtyState() {
    bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::RuntimeMapEditor editor;
    editor.attach(level);

    expect(editor.selectObject("sign_no_parking"), "runtime editor selects existing object");
    expect(editor.selectedObjectId() == "sign_no_parking", "runtime editor reports selection");
    editor.setSelectedPosition({-4.0f, 0.0f, 6.0f});

    const bs3d::WorldObject* object = findObject(level, "sign_no_parking");
    expect(object != nullptr, "edited object still exists");
    expectNear(object->position.x, -4.0f, 0.001f, "runtime editor edits selected position");
    expect(editor.dirty(), "runtime editor becomes dirty after edit");
}

void runtimeMapEditorAddsManifestInstance() {
    bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::RuntimeMapEditor editor;
    editor.attach(level);

    expect(editor.addInstance("lamp_post_lowpoly", {18.0f, 0.0f, -27.0f}), "runtime editor adds instance");
    expect(findObject(level, "editor_lamp_post_lowpoly_0") != nullptr, "runtime editor generates stable instance id");
    expect(editor.dirty(), "runtime editor dirty after add instance");
}
```

- [ ] **Step 2: Run test to verify it fails**

Run:

```powershell
cmake --build build --config Release
```

Expected: compile fails because `RuntimeMapEditor` does not exist.

- [ ] **Step 3: Add editor model API**

Create `src/game/RuntimeMapEditor.h`:

```cpp
#pragma once

#include "IntroLevelBuilder.h"

#include <string>
#include <vector>

namespace bs3d {

class RuntimeMapEditor {
public:
    void attach(IntroLevelData& level);
    bool selectObject(const std::string& id);
    const std::string& selectedObjectId() const;
    WorldObject* selectedObject();
    const WorldObject* selectedObject() const;
    bool setSelectedPosition(Vec3 position);
    bool setSelectedScale(Vec3 scale);
    bool setSelectedYaw(float yawRadians);
    bool addInstance(const std::string& assetId, Vec3 position);
    bool deleteSelectedOverlayInstance();
    bool dirty() const;
    void clearDirty();
    std::vector<WorldObject*> objects();

private:
    IntroLevelData* level_ = nullptr;
    std::string selectedObjectId_;
    bool dirty_ = false;
    int generatedInstanceCounter_ = 0;
};

} // namespace bs3d
```

Implement in `RuntimeMapEditor.cpp`:

- `attach` stores the level pointer.
- `selectObject` finds by id.
- `selectedObject` returns pointer by id.
- setters update the selected object and set `dirty_`.
- `addInstance` creates id `editor_<assetId>_<counter>`, appends a no-collision `WorldObject`, selects it, increments counter, and sets dirty.
- `deleteSelectedOverlayInstance` only deletes ids starting with `editor_`.

Add the source to `bs3d_game_support`.

- [ ] **Step 4: Run tests**

Run:

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Expected: all tests pass.

---

### Task 7: Dear ImGui Dev-Only Build Wiring

**Files:**
- Modify: `CMakeLists.txt`
- Create: `src/game/RuntimeMapEditorImGui.h`
- Create: `src/game/RuntimeMapEditorImGui.cpp`
- Modify: `src/game/GameApp.cpp`

- [ ] **Step 1: Configure a dev build to expose missing editor symbols**

Run:

```powershell
cmake -S . -B build-dev -DBS3D_ENABLE_DEV_TOOLS=ON
cmake --build build-dev --config Release
```

Expected before implementation: build succeeds without editor UI. This is the baseline.

- [ ] **Step 2: Add CMake dependency wiring**

Inside `if(BS3D_BUILD_GAME)`, add `FetchContent_Declare` entries guarded by `if(BS3D_ENABLE_DEV_TOOLS)`:

```cmake
if(BS3D_ENABLE_DEV_TOOLS)
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.92.7
    )
    FetchContent_MakeAvailable(imgui)

    FetchContent_Declare(
        rlimgui
        GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
        GIT_TAG Raylib_6_0
    )
    FetchContent_MakeAvailable(rlimgui)

    add_library(bs3d_imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${rlimgui_SOURCE_DIR}/rlImGui.cpp
    )
    target_include_directories(bs3d_imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${rlimgui_SOURCE_DIR}
    )
    target_link_libraries(bs3d_imgui PUBLIC raylib)
endif()
```

Add `src/game/RuntimeMapEditorImGui.cpp` to `bs3d_game_support` only when dev tools are enabled:

```cmake
if(BS3D_ENABLE_DEV_TOOLS)
    target_sources(bs3d_game_support PRIVATE src/game/RuntimeMapEditorImGui.cpp)
    target_link_libraries(bs3d_game_support PUBLIC bs3d_imgui)
endif()
```

- [ ] **Step 3: Add empty ImGui wrapper**

Create `src/game/RuntimeMapEditorImGui.h`:

```cpp
#pragma once

#include "RuntimeMapEditor.h"
#include "WorldAssetRegistry.h"

namespace bs3d {

class RuntimeMapEditorImGui {
public:
    void draw(RuntimeMapEditor& editor, const WorldAssetRegistry& registry);
};

} // namespace bs3d
```

Create `src/game/RuntimeMapEditorImGui.cpp`:

```cpp
#include "RuntimeMapEditorImGui.h"

#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
#include "imgui.h"
#endif

namespace bs3d {

void RuntimeMapEditorImGui::draw(RuntimeMapEditor& editor, const WorldAssetRegistry& registry) {
#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
    ImGui::Begin("Blok 13 Runtime Editor");
    ImGui::Text("Selected: %s", editor.selectedObjectId().c_str());
    ImGui::Text("Assets: %d", static_cast<int>(registry.definitions().size()));
    ImGui::End();
#else
    (void)editor;
    (void)registry;
#endif
}

} // namespace bs3d
```

- [ ] **Step 4: Verify normal and dev builds**

Run:

```powershell
cmake --build build --config Release
cmake -S . -B build-dev -DBS3D_ENABLE_DEV_TOOLS=ON
cmake --build build-dev --config Release
ctest --test-dir build -C Release --output-on-failure
```

Expected: normal build passes without ImGui symbols; dev build downloads/builds ImGui/rlImGui and passes compilation.

---

### Task 8: GameApp Editor Toggle And ImGui Frame

**Files:**
- Modify: `src/game/GameApp.cpp`
- Modify: `include/bs3d/core/Types.h`
- Modify: `src/game/DevTools.h`
- Modify: `src/game/DevTools.cpp`

- [ ] **Step 1: Add input bit**

In `InputState`, add:

```cpp
bool toggleRuntimeEditorPressed = false;
```

Update raw input mapping in `GameApp.cpp` so `F10` sets it when dev tools are enabled.

- [ ] **Step 2: Add runtime editor members in GameApp**

Add dev-only members near existing debug/dev state:

```cpp
#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
RuntimeMapEditor runtimeEditor;
RuntimeMapEditorImGui runtimeEditorUi;
bool runtimeEditorOpen = false;
#endif
```

Attach after level load:

```cpp
#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
runtimeEditor.attach(level);
#endif
```

- [ ] **Step 3: Wire rlImGui lifecycle**

Include dev-only headers:

```cpp
#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
#include "RuntimeMapEditorImGui.h"
#include "rlImGui.h"
#endif
```

After raylib window initialization:

```cpp
#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
rlImGuiSetup(true);
#endif
```

Before window close:

```cpp
#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
rlImGuiShutdown();
#endif
```

Inside drawing between `BeginDrawing()` and `EndDrawing()`:

```cpp
#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
if (runtimeEditorOpen) {
    rlImGuiBegin();
    runtimeEditorUi.draw(runtimeEditor, assetRegistry);
    rlImGuiEnd();
}
#endif
```

- [ ] **Step 4: Pause gameplay input while editor captures input**

When `runtimeEditorOpen` is true, prevent mouse look and movement actions from applying if ImGui wants capture:

```cpp
#if defined(BS3D_ENABLE_DEV_TOOLS) && BS3D_ENABLE_DEV_TOOLS
if (runtimeEditorOpen) {
    input.mouseLookActive = false;
    input.moveForward = false;
    input.moveBackward = false;
    input.moveLeft = false;
    input.moveRight = false;
    input.accelerate = false;
    input.brake = false;
    input.steerLeft = false;
    input.steerRight = false;
}
#endif
```

- [ ] **Step 5: Smoke dev build**

Run:

```powershell
cmake --build build-dev --config Release
.\build-dev\Release\blokowa_satyra.exe --data-root data --no-audio --no-save --no-load-save --smoke-frames 5
```

Expected: exit code 0.

Manual check: launch without `--smoke-frames`, press `F10`, see `Blok 13 Runtime Editor`.

---

### Task 9: ImGui Object List And Inspector

**Files:**
- Modify: `src/game/RuntimeMapEditor.h`
- Modify: `src/game/RuntimeMapEditor.cpp`
- Modify: `src/game/RuntimeMapEditorImGui.cpp`

- [ ] **Step 1: Add editor commands for inspector fields**

Extend `RuntimeMapEditor` with:

```cpp
bool setSelectedAssetId(const std::string& assetId);
bool setSelectedZoneTag(WorldLocationTag zoneTag);
bool setSelectedGameplayTags(std::vector<std::string> tags);
```

Each command edits selected object and sets dirty.

- [ ] **Step 2: Draw object list**

In `RuntimeMapEditorImGui::draw`, add:

```cpp
ImGui::Begin("Objects");
static char filter[128] = "";
ImGui::InputText("Filter", filter, sizeof(filter));
for (WorldObject* object : editor.objects()) {
    const std::string label = object->id + " [" + object->assetId + "]";
    if (filter[0] != '\0' && label.find(filter) == std::string::npos) {
        continue;
    }
    if (ImGui::Selectable(label.c_str(), editor.selectedObjectId() == object->id)) {
        editor.selectObject(object->id);
    }
}
ImGui::End();
```

- [ ] **Step 3: Draw transform inspector**

Add:

```cpp
ImGui::Begin("Inspector");
if (WorldObject* selected = editor.selectedObject()) {
    float position[3] = {selected->position.x, selected->position.y, selected->position.z};
    if (ImGui::DragFloat3("Position", position, 0.05f)) {
        editor.setSelectedPosition({position[0], position[1], position[2]});
    }
    float scale[3] = {selected->scale.x, selected->scale.y, selected->scale.z};
    if (ImGui::DragFloat3("Scale", scale, 0.02f, 0.01f, 100.0f)) {
        editor.setSelectedScale({scale[0], scale[1], scale[2]});
    }
    float yaw = selected->yawRadians;
    if (ImGui::DragFloat("Yaw", &yaw, 0.01f)) {
        editor.setSelectedYaw(yaw);
    }
    ImGui::Text("Asset: %s", selected->assetId.c_str());
    ImGui::Text("Dirty: %s", editor.dirty() ? "yes" : "no");
} else {
    ImGui::Text("No object selected");
}
ImGui::End();
```

- [ ] **Step 4: Verify**

Run:

```powershell
cmake --build build-dev --config Release
.\build-dev\Release\blokowa_satyra.exe --data-root data --no-audio --no-save --no-load-save --smoke-frames 5
```

Manual check: open editor, select `sign_no_parking`, drag position, confirm object moves in scene.

---

### Task 10: Add Instance From Asset Manifest

**Files:**
- Modify: `src/game/RuntimeMapEditorImGui.cpp`
- Modify: `src/game/RuntimeMapEditor.cpp`

- [ ] **Step 1: Add asset picker panel**

In `RuntimeMapEditorImGui::draw`, add:

```cpp
ImGui::Begin("Assets");
static char assetFilter[128] = "";
ImGui::InputText("Asset Filter", assetFilter, sizeof(assetFilter));
for (const WorldAssetDefinition& asset : registry.definitions()) {
    if (assetFilter[0] != '\0' && asset.id.find(assetFilter) == std::string::npos) {
        continue;
    }
    if (ImGui::Selectable(asset.id.c_str())) {
        editor.addInstance(asset.id, {0.0f, 0.0f, 0.0f});
    }
}
ImGui::End();
```

- [ ] **Step 2: Improve add position**

Change `RuntimeMapEditor::addInstance` call path so UI can pass a placement near the player or camera target once `GameApp` exposes that position. For V1, use `{0.0f, 0.0f, 0.0f}` in the UI and rely on inspector movement.

- [ ] **Step 3: Verify**

Run:

```powershell
cmake --build build-dev --config Release
.\build-dev\Release\blokowa_satyra.exe --data-root data --no-audio --no-save --no-load-save --smoke-frames 5
```

Manual check: open editor, choose `lamp_post_lowpoly`, confirm `editor_lamp_post_lowpoly_0` appears in object list and scene.

---

### Task 11: Save Current Overlay From Editor

**Files:**
- Modify: `src/game/RuntimeMapEditor.h`
- Modify: `src/game/RuntimeMapEditor.cpp`
- Modify: `src/game/RuntimeMapEditorImGui.cpp`
- Test: `tests/game_support_tests.cpp`

- [ ] **Step 1: Write failing overlay extraction test**

Add test:

```cpp
void runtimeMapEditorBuildsOverlayForEditedObjects() {
    bs3d::IntroLevelData level = bs3d::IntroLevelBuilder::build();
    bs3d::RuntimeMapEditor editor;
    editor.attach(level);
    editor.selectObject("sign_no_parking");
    editor.setSelectedPosition({-4.0f, 0.0f, 6.0f});
    editor.addInstance("lamp_post_lowpoly", {18.0f, 0.0f, -27.0f});

    const bs3d::EditorOverlayDocument overlay = editor.buildOverlayDocument();

    expect(overlay.overrides.size() == 1, "runtime editor exports one changed base override");
    expect(overlay.instances.size() == 1, "runtime editor exports one editor instance");
    expect(overlay.overrides[0].id == "sign_no_parking", "runtime editor override keeps base id");
    expect(overlay.instances[0].id == "editor_lamp_post_lowpoly_0", "runtime editor instance keeps generated id");
}
```

- [ ] **Step 2: Run test to verify it fails**

Run:

```powershell
cmake --build build --config Release
```

Expected: compile fails because `buildOverlayDocument` does not exist.

- [ ] **Step 3: Track edited base ids and implement export**

In `RuntimeMapEditor`, add:

```cpp
std::vector<std::string> editedBaseObjectIds_;
EditorOverlayDocument buildOverlayDocument() const;
bool saveOverlay(const std::string& path, std::vector<std::string>& warnings);
```

When setters modify a selected object whose id does not start with `editor_`, add its id once to `editedBaseObjectIds_`.

`buildOverlayDocument` should:

- include changed base objects in `overrides`;
- include `editor_` objects in `instances`;
- copy `id`, `assetId`, `position`, `scale`, `yawRadians`, `zoneTag`, and `gameplayTags`.

- [ ] **Step 4: Add UI save button**

In ImGui status area:

```cpp
if (ImGui::Button("Save Overlay")) {
    std::vector<std::string> warnings;
    if (!editor.saveOverlay("data/world/block13_editor_overlay.json", warnings)) {
        lastStatus_ = warnings.empty() ? "Save failed" : warnings.front();
    } else {
        lastStatus_ = "Saved data/world/block13_editor_overlay.json";
    }
}
ImGui::SameLine();
ImGui::Text("%s", editor.dirty() ? "Dirty" : "Saved");
```

If `lastStatus_` is needed, store it as `std::string lastStatus_;` in `RuntimeMapEditorImGui`.

- [ ] **Step 5: Verify tests and save**

Run:

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cmake --build build-dev --config Release
```

Manual check: open editor, edit object, save overlay, inspect `data/world/block13_editor_overlay.json`.

---

### Task 12: Final Verification And Documentation

**Files:**
- Modify: `README.md`
- Modify: `progress.md`

- [ ] **Step 1: Update README controls**

Add under dev controls:

```markdown
- `F10`: toggle Dear ImGui runtime map editor when built with `BS3D_ENABLE_DEV_TOOLS=ON`
```

Add a short note under build:

```markdown
Runtime editor output is saved to `data/world/block13_editor_overlay.json`. Validate it with:

```powershell
python tools\validate_editor_overlay.py data\world\block13_editor_overlay.json --asset-root data\assets
```
```

- [ ] **Step 2: Update progress**

Add a dated bullet:

```markdown
- Started `v0.15 Runtime Editor Foundation`: added overlay data, validation, dev-only Dear ImGui runtime editor shell, object inspector, add-instance workflow from the asset manifest, and save/load path for `data/world/block13_editor_overlay.json`.
```

- [ ] **Step 3: Run final verification**

Run:

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
python tools\validate_editor_overlay.py data\world\block13_editor_overlay.json --asset-root data\assets
.\build\Release\blokowa_satyra.exe --data-root data --no-audio --no-save --no-load-save --smoke-frames 5
cmake -S . -B build-dev -DBS3D_ENABLE_DEV_TOOLS=ON
cmake --build build-dev --config Release
.\build-dev\Release\blokowa_satyra.exe --data-root data --no-audio --no-save --no-load-save --smoke-frames 5
```

Expected:

- normal build succeeds;
- normal CTest passes;
- overlay validator passes;
- normal smoke exits 0;
- dev build succeeds;
- dev smoke exits 0.

- [ ] **Step 4: Manual editor smoke**

Run:

```powershell
.\build-dev\Release\blokowa_satyra.exe --data-root data --no-audio --no-save --no-load-save
```

Manual expected result:

- `F10` opens `Blok 13 Runtime Editor`;
- object list is visible;
- selecting `sign_no_parking` shows transform inspector;
- dragging position changes the object in scene;
- selecting `lamp_post_lowpoly` in asset panel creates an `editor_lamp_post_lowpoly_0` instance;
- `Save Overlay` writes `data/world/block13_editor_overlay.json`;
- restarting the game keeps saved changes.

---

## Self-Review Checklist

Spec coverage:

- Existing object overrides: Tasks 3, 6, 9, 11.
- New asset instances from manifest: Tasks 6, 10, 11.
- Overlay JSON: Tasks 2, 4, 11.
- Python validation: Task 5.
- Dear ImGui dev-only UI: Tasks 7, 8, 9, 10.
- Normal build free of editor UI: Tasks 7 and 12.
- Tests and smoke: every implementation task has verification; final gate in Task 12.
- Later C# boundary: documented in spec, no implementation task by design.

Known exclusions:

- World click/raycast selection is deferred until after list-based editing works.
- Mission/dialogue/cutscene/item/localization editing remains out of V1.
- Base object deletion remains out of V1.
