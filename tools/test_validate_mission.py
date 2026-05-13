#!/usr/bin/env python3
"""Unit tests for validate_mission.py."""

from __future__ import annotations

import json
import tempfile
from pathlib import Path

import validate_mission as validator


RUNTIME_POLICY = """
MissionPhase missionPhaseFromDataName(const std::string& phase) {
    if (phase == "WalkToShop") {
        return MissionPhase::WalkToShop;
    }
    if (phase == "ReturnToBench") {
        return MissionPhase::ReturnToBench;
    }
    if (phase == "ReachVehicle") {
        return MissionPhase::ReachVehicle;
    }
    if (phase == "DriveToShop") {
        return MissionPhase::DriveToShop;
    }
    if (phase == "ChaseActive") {
        return MissionPhase::ChaseActive;
    }
    if (phase == "ReturnToLot") {
        return MissionPhase::ReturnToLot;
    }
    if (phase == "Completed") {
        return MissionPhase::Completed;
    }
    return MissionPhase::WaitingForStart;
}
"""


BASE_MISSION = {
    "id": "driving_errand_vertical_slice",
    "title": "Szybki kurs przez osiedle",
    "steps": [
        {"phase": "ReachVehicle", "objective": "Wsiadz do auta na parkingu.", "trigger": "player_enters_vehicle"},
        {"phase": "DriveToShop", "objective": "Podjedz pod sklep Zenona.", "trigger": "vehicle_reaches_shop_marker"},
        {"phase": "ChaseActive", "objective": "Zgub osiedlowy poscig.", "trigger": "sustained_escape_distance"},
        {"phase": "ReturnToLot", "objective": "Wracaj na parking pod blokiem.", "trigger": "vehicle_reaches_dropoff_marker"},
    ],
}


BASE_LOCALIZATION = {
    "schemaVersion": 1,
    "lines": {
        "mission.start_hint": "Wsiadz z auta z parkingu. Bez pytan.",
        "mission.bogus_intro": "Dobra, szybki kurs pod sklep i wracamy.",
        "mission.rysiek_chase": "No i elegancko. Tylko czemu oni za nami jadą.",
    },
}


BASE_OUTCOMES = {
    "schemaVersion": 1,
    "outcomes": [
        {"id": "shop_rolling_grille_checked", "label": "Shop grille", "source": "object:shop", "category": "object_use", "location": "Shop", "speaker": "Roleta", "line": "line"},
    {"idPattern": "shop_prices_read_*", "label": "Prices", "source": "tag:prices", "category": "object_use", "location": "Shop", "speaker": "Kartka", "line": "line"},
    {"id": "block_intercom_buzzed", "label": "Intercom", "source": "object:intercom", "category": "object_use", "location": "Block", "speaker": "Interkom", "line": "line"},
    {"idPattern": "trash_disturbed_*", "label": "Trash", "source": "tag:trash", "category": "object_use", "location": "Trash", "speaker": "Smietnik", "line": "line"},
    {"id": "bogus_bench_checked", "label": "Bench", "source": "asset:bench", "category": "object_use", "location": "Block", "speaker": "Lawka", "line": "line"},
    {"idPattern": "glass_peeked_*", "label": "Glass", "source": "tag:glass", "category": "object_use", "location": "Shop", "speaker": "Szyba", "line": "line"},
    {"id": "vehicle_chased", "label": "Vehicle", "source": "vehicle", "category": "system", "location": "RoadLoop", "speaker": "System", "line": "line"},
    {"idPattern": "parking_sign_read_*", "label": "Parking sign", "source": "tag:parking_sign", "category": "object_use", "location": "Parking", "speaker": "Znak", "line": "line"},
    {"id": "shop_door_checked", "label": "Shop door", "source": "object:shop_door", "category": "object_use", "location": "Shop", "speaker": "Drzwi", "line": "line"},
    {"idPattern": "shop_notice_read_*", "label": "Notice", "source": "tag:shop_notice", "category": "object_use", "location": "Shop", "speaker": "Kartka", "line": "line"},
    {"idPattern": "block_notice_read_*", "label": "Notice", "source": "tag:block_notice", "category": "object_use", "location": "Block", "speaker": "Ogloszenie", "line": "line"},
    {"idPattern": "garage_door_checked_*", "label": "Garage door", "source": "asset:garage", "category": "object_use", "location": "Garage", "speaker": "Garaz", "line": "line"},
]
}


def write_json(path: Path, value: dict) -> None:
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding="utf-8")


def with_context(body) -> None:
    with tempfile.TemporaryDirectory() as temp_dir:
        root = Path(temp_dir)
        mission_path = root / "mission_driving_errand.json"
        localization_path = root / "mission_localization_pl.json"
        catalog_path = root / "object_outcome_catalog.json"
        policy_path = root / "WorldDataLoader.cpp"

        mission_path.write_text("{}", encoding="utf-8")
        write_json(localization_path, BASE_LOCALIZATION)
        write_json(catalog_path, BASE_OUTCOMES)
        policy_path.write_text(RUNTIME_POLICY, encoding="utf-8")

        body(mission_path, localization_path, catalog_path, policy_path)


def assert_issue(issues: list[str], needle: str) -> None:
    assert any(needle in issue for issue in issues), f"expected issue containing {needle!r}, got {issues!r}"


def test_valid_mission_passes() -> None:
    def run(mission_path: Path, localization_path: Path, catalog_path: Path, policy_path: Path) -> None:
        write_json(mission_path, {**BASE_MISSION})
        issues = validator.validate_mission(mission_path, localization_path, catalog_path, policy_path)
        assert issues == []

    with_context(run)


def test_unknown_phase_fails() -> None:
    def run(mission_path: Path, localization_path: Path, catalog_path: Path, policy_path: Path) -> None:
        mission = {**BASE_MISSION}
        mission["steps"] = [
            {"phase": "NonExistingPhase", "objective": "nope", "trigger": "player_enters_vehicle"}
        ]
        write_json(mission_path, mission)
        issues = validator.validate_mission(mission_path, localization_path, catalog_path, policy_path)
        assert_issue(issues, "not a runtime-supported phase")

    with_context(run)


def test_unknown_outcome_trigger_fails() -> None:
    def run(mission_path: Path, localization_path: Path, catalog_path: Path, policy_path: Path) -> None:
        mission = {**BASE_MISSION}
        mission["steps"] = [
            {"phase": "ReachVehicle", "objective": "go", "trigger": "outcome:unknown_trigger"},
        ]
        write_json(mission_path, mission)
        issues = validator.validate_mission(mission_path, localization_path, catalog_path, policy_path)
        assert_issue(issues, "references unknown outcome")

    with_context(run)


def test_outcome_pattern_trigger_accepts_concrete_runtime_id() -> None:
    def run(mission_path: Path, localization_path: Path, catalog_path: Path, policy_path: Path) -> None:
        mission = {**BASE_MISSION}
        mission["steps"] = [
            {"phase": "ReachVehicle", "objective": "go", "trigger": "outcome:shop_prices_read_shop_price_card_0"},
            {"phase": "WalkToShop", "objective": "read", "trigger": "outcome:shop_prices_read_*"},
        ]
        write_json(mission_path, mission)
        issues = validator.validate_mission(mission_path, localization_path, catalog_path, policy_path)
        assert issues == []

    with_context(run)


def test_dialogue_requires_known_phase() -> None:
    def run(mission_path: Path, localization_path: Path, catalog_path: Path, policy_path: Path) -> None:
        mission = {**BASE_MISSION}
        mission["dialogue"] = [
            {"phase": "ReachVehicle", "speaker": "Misja", "lineKey": "mission.start_hint", "durationSeconds": 2.4},
            {"phase": "ChaseActive", "speaker": "Rysiek", "lineKey": "mission.rysiek_chase", "durationSeconds": 3.0},
        ]
        mission["npcReactions"] = [
            {"phase": "DriveToShop", "speaker": "Bogus", "line": "Tekst", "durationSeconds": 2.0},
        ]
        mission["cutscenes"] = [
            {"cutscene": "intro", "phase": "DriveToShop", "speaker": "Narrator", "text": "No", "durationSeconds": 1.5}
        ]
        write_json(mission_path, mission)
        issues = validator.validate_mission(mission_path, localization_path, catalog_path, policy_path)
        assert issues == []

    with_context(run)


def test_bad_dialogue_duration_fails() -> None:
    def run(mission_path: Path, localization_path: Path, catalog_path: Path, policy_path: Path) -> None:
        mission = {**BASE_MISSION}
        mission["dialogue"] = [
            {"phase": "ReachVehicle", "speaker": "Misja", "line": "Hello", "durationSeconds": 0},
        ]
        write_json(mission_path, mission)
        issues = validator.validate_mission(mission_path, localization_path, catalog_path, policy_path)
        assert_issue(issues, "durationSeconds")

    with_context(run)


def test_missing_line_and_line_key_fails() -> None:
    def run(mission_path: Path, localization_path: Path, catalog_path: Path, policy_path: Path) -> None:
        mission = {**BASE_MISSION}
        mission["dialogue"] = [
            {"phase": "ReachVehicle", "speaker": "Misja", "durationSeconds": 2.0},
        ]
        write_json(mission_path, mission)
        issues = validator.validate_mission(mission_path, localization_path, catalog_path, policy_path)
        assert_issue(issues, "requires text or lineKey")

    with_context(run)


def test_missing_localization_key_fails() -> None:
    def run(mission_path: Path, localization_path: Path, catalog_path: Path, policy_path: Path) -> None:
        mission = {**BASE_MISSION}
        mission["dialogue"] = [
            {"phase": "ReachVehicle", "speaker": "Misja", "lineKey": "mission.missing", "durationSeconds": 2.0},
        ]
        write_json(mission_path, mission)
        issues = validator.validate_mission(mission_path, localization_path, catalog_path, policy_path)
        assert_issue(issues, "is not defined")

    with_context(run)


def test_bad_localization_schema_version_fails() -> None:
    def run(mission_path: Path, localization_path: Path, catalog_path: Path, policy_path: Path) -> None:
        write_json(localization_path, {"schemaVersion": 2, "lines": BASE_LOCALIZATION["lines"]})
        mission = {**BASE_MISSION}
        mission["dialogue"] = [
            {"phase": "ReachVehicle", "speaker": "Misja", "lineKey": "mission.start_hint", "durationSeconds": 2.0},
        ]
        write_json(mission_path, mission)

        issues = validator.validate_mission(mission_path, localization_path, catalog_path, policy_path)

        assert_issue(issues, "mission localization schemaVersion must be 1")

    with_context(run)


def test_duplicate_phase_fails() -> None:
    def run(mission_path: Path, localization_path: Path, catalog_path: Path, policy_path: Path) -> None:
        mission = {**BASE_MISSION}
        mission["steps"] = [
            {"phase": "ReachVehicle", "objective": "a", "trigger": "player_enters_vehicle"},
            {"phase": "ReachVehicle", "objective": "b", "trigger": "player_enters_vehicle"},
        ]
        write_json(mission_path, mission)
        issues = validator.validate_mission(mission_path, localization_path, catalog_path, policy_path)
        assert_issue(issues, "is duplicated")

    with_context(run)


def test_bad_policy_fails_fast() -> None:
    def run(mission_path: Path, localization_path: Path, catalog_path: Path, policy_path: Path) -> None:
        write_json(mission_path, BASE_MISSION)
        policy_path.write_text("not a policy", encoding="utf-8")
        issues = validator.validate_mission(mission_path, localization_path, catalog_path, policy_path)
        assert_issue(issues, "runtime phase policy could not be extracted")

    with_context(run)


def main() -> int:
    tests = [
        test_valid_mission_passes,
        test_unknown_phase_fails,
        test_unknown_outcome_trigger_fails,
        test_outcome_pattern_trigger_accepts_concrete_runtime_id,
        test_dialogue_requires_known_phase,
        test_bad_dialogue_duration_fails,
        test_missing_line_and_line_key_fails,
        test_missing_localization_key_fails,
        test_bad_localization_schema_version_fails,
        test_duplicate_phase_fails,
        test_bad_policy_fails_fast,
    ]
    for test in tests:
        test()
        print(f"PASS {test.__name__}")
    print(f"{len(tests)} mission validator test(s) passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
