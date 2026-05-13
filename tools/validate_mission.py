#!/usr/bin/env python3
"""Validate mission data authoring contracts against runtime and catalog contracts."""

from __future__ import annotations

import argparse
import json
import math
import re
from pathlib import Path


def load_json(path: Path) -> object:
    if not path.exists():
        raise FileNotFoundError(f"missing file: {path}")
    text = path.read_text(encoding="utf-8")
    return json.loads(text)


def as_non_empty_str(value: object) -> str:
    return value.strip() if isinstance(value, str) else ""


def is_finite_positive_number(value: object, allow_zero: bool = False) -> bool:
    if not isinstance(value, (int, float)):
        return False
    numeric = float(value)
    if not math.isfinite(numeric):
        return False
    return numeric >= 0.0 if allow_zero else numeric > 0.0


def extract_runtime_mission_phases(policy_path: Path) -> set[str]:
    if not policy_path.exists():
        return set()
    text = policy_path.read_text(encoding="utf-8")
    matches = re.findall(r"if\s*\(\s*phase\s*==\s*\"([^\"]+)\"\s*\)", text)
    return {match for match in matches}


def load_mission_localization_lines(path: Path) -> tuple[dict[str, str], list[str]]:
    localization_text = load_json(path)
    issues: list[str] = []
    if not isinstance(localization_text, dict):
        return {}, ["mission localization root must be an object"]

    if localization_text.get("schemaVersion") != 1:
        issues.append("mission localization schemaVersion must be 1")

    lines = localization_text.get("lines")
    if not isinstance(lines, dict):
        return {}, [*issues, "mission localization lines must be an object"]

    result: dict[str, str] = {}
    for key, value in lines.items():
        text = as_non_empty_str(value)
        if key and text:
            result[key] = text
    return result, issues


def load_outcome_trigger_keys(catalog_path: Path) -> tuple[set[str], list[str]]:
    catalog_text = load_json(catalog_path)
    issues: list[str] = []
    if not isinstance(catalog_text, dict):
        return set(), ["object outcome catalog root must be an object"]

    outcomes = catalog_text.get("outcomes")
    if not isinstance(outcomes, list):
        return set(), ["object outcome catalog must define outcomes as an array"]

    exact_keys: set[str] = set()
    wildcard_prefixes: list[str] = []
    for outcome in outcomes:
        if not isinstance(outcome, dict):
            continue
        exact = as_non_empty_str(outcome.get("id"))
        pattern = as_non_empty_str(outcome.get("idPattern"))
        if exact:
            exact_keys.add(exact)
            continue
        if pattern and pattern.endswith("*"):
            wildcard_prefixes.append(pattern[:-1])
        elif pattern:
            exact_keys.add(pattern)
    return exact_keys | {f"{prefix}*" for prefix in wildcard_prefixes}, issues


def outcome_catalog_accepts_trigger(trigger_id: str, exact_keys: set[str], wildcard_prefixes: list[str]) -> bool:
    if not trigger_id:
        return False
    if trigger_id in exact_keys:
        return True
    for prefix in wildcard_prefixes:
        if trigger_id.startswith(prefix):
            return True
    return False


def validate_steps(steps: object, allowed_phases: set[str], outcome_exact: set[str], outcome_prefixes: list[str],
                   issues: list[str]) -> set[str]:
    if not isinstance(steps, list) or not steps:
        issues.append("mission.steps must be a non-empty array (legacy mission file alias 'phases' also accepted)")
        return set()

    mission_phases: set[str] = set()
    for index, raw_step in enumerate(steps):
        if not isinstance(raw_step, dict):
            issues.append(f"mission.steps[{index}] must be an object")
            continue

        phase = as_non_empty_str(raw_step.get("phase"))
        objective = as_non_empty_str(raw_step.get("objective"))
        trigger = as_non_empty_str(raw_step.get("trigger"))

        if not phase:
            issues.append(f"mission.steps[{index}].phase is required")
            continue
        if phase in mission_phases:
            issues.append(f"mission.steps[{index}].phase '{phase}' is duplicated")
        mission_phases.add(phase)
        if phase not in allowed_phases:
            issues.append(f"mission.steps[{index}].phase '{phase}' is not a runtime-supported phase")
        if not objective:
            issues.append(f"mission.steps[{index}] requires non-empty objective")
        if not trigger:
            issues.append(f"mission.steps[{index}] requires non-empty trigger")
            continue
        if trigger.startswith("outcome:"):
            outcome_key = trigger[len("outcome:"):]
            if not outcome_catalog_accepts_trigger(outcome_key, outcome_exact, outcome_prefixes):
                issues.append(f"mission.steps[{index}].trigger references unknown outcome '{outcome_key}'")
    return mission_phases


def validate_dialogue_lines(
    label: str,
    lines: object,
    mission_phases: set[str],
    localization_lines: dict[str, str],
    issues: list[str],
) -> None:
    if lines is None:
        return
    if not isinstance(lines, list):
        issues.append(f"mission.{label} must be an array if present")
        return

    for index, raw_line in enumerate(lines):
        if not isinstance(raw_line, dict):
            issues.append(f"mission.{label}[{index}] must be an object")
            continue

        phase = as_non_empty_str(raw_line.get("phase"))
        speaker = as_non_empty_str(raw_line.get("speaker"))
        line_key = as_non_empty_str(raw_line.get("lineKey"))
        text = as_non_empty_str(raw_line.get("text"))
        if not text:
            text = as_non_empty_str(raw_line.get("line"))
        duration = raw_line.get("durationSeconds")

        if not phase:
            issues.append(f"mission.{label}[{index}].phase is required")
        elif phase not in mission_phases:
            issues.append(
                f"mission.{label}[{index}].phase '{phase}' is not present in mission steps"
            )
        if label == "cutscenes":
            cutscene = as_non_empty_str(raw_line.get("cutscene"))
            if not cutscene:
                issues.append(f"mission.{label}[{index}].cutscene is required")
        if not speaker:
            issues.append(f"mission.{label}[{index}].speaker is required")
        if not text and not line_key:
            issues.append(f"mission.{label}[{index}] requires text or lineKey")
        if line_key and line_key not in localization_lines:
            issues.append(f"mission.{label}[{index}].lineKey '{line_key}' is not defined in mission localization")
        if duration is not None and not is_finite_positive_number(duration):
            issues.append(
                f"mission.{label}[{index}].durationSeconds must be a positive finite number if specified"
            )


def validate_mission(
    mission_path: Path,
    localization_path: Path,
    outcome_catalog_path: Path,
    runtime_policy_path: Path,
) -> list[str]:
    issues: list[str] = []
    mission_text = load_json(mission_path)
    localization, localization_issues = load_mission_localization_lines(localization_path)
    outcome_exact, outcome_issues = load_outcome_trigger_keys(outcome_catalog_path)
    outcome_prefixes = [key[:-1] for key in outcome_exact if key.endswith("*")]
    issues.extend(localization_issues)
    issues.extend(outcome_issues)
    if not isinstance(mission_text, dict):
        issues.append("mission root must be an object")
        return issues

    if not mission_text.get("id", "").strip():
        issues.append("mission.id is required")
    if not as_non_empty_str(mission_text.get("title")):
        issues.append("mission.title is required")

    runtime_phases = extract_runtime_mission_phases(runtime_policy_path)
    if not runtime_phases:
        issues.append("runtime phase policy could not be extracted; verify src/game/WorldDataLoader.cpp")

    steps = mission_text.get("steps")
    if steps is None:
        steps = mission_text.get("phases")
        if steps is not None:
            issues.append("mission uses deprecated key 'phases'; prefer 'steps'")
    mission_phases = validate_steps(steps, runtime_phases, outcome_exact, outcome_prefixes, issues)

    validate_dialogue_lines("dialogue", mission_text.get("dialogue"), mission_phases, localization, issues)
    validate_dialogue_lines("npcReactions", mission_text.get("npcReactions"), mission_phases, localization, issues)
    validate_dialogue_lines("cutscenes", mission_text.get("cutscenes"), mission_phases, localization, issues)
    return issues


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate mission JSON contracts.")
    parser.add_argument("mission", nargs="?", default="data/mission_driving_errand.json", type=Path)
    parser.add_argument("--mission-localization", default="data/world/mission_localization_pl.json", type=Path)
    parser.add_argument("--outcome-catalog", default="data/world/object_outcome_catalog.json", type=Path)
    parser.add_argument(
        "--runtime-policy",
        default="src/game/WorldDataLoader.cpp",
        type=Path,
        help="WorldDataLoader.cpp containing runtime phase support",
    )
    parser.add_argument("--quiet", action="store_true", help="Only print failures.")
    args = parser.parse_args()

    try:
        issues = validate_mission(
        args.mission,
        args.mission_localization,
        args.outcome_catalog,
        args.runtime_policy,
        )
    except (FileNotFoundError, json.JSONDecodeError) as error:
        print(f"error: {error}")
        return 1
    if issues:
        for issue in issues:
            print(f"error: {issue}")
        return 1
    if not args.quiet:
        print(f"mission validation passed: {args.mission}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
