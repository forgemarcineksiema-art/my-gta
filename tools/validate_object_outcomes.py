#!/usr/bin/env python3
"""Validate object outcome catalog metadata against runtime affordance policy."""

from __future__ import annotations

import argparse
import json
import math
import re
from pathlib import Path


ALLOWED_WORLD_EVENT_TYPES = {"PublicNuisance", "PropertyDamage", "ChaseSeen", "ShopTrouble"}


def outcome_key(outcome: dict[str, object]) -> str:
    outcome_id = str(outcome.get("id", "")).strip()
    pattern = str(outcome.get("idPattern", "")).strip()
    return outcome_id or pattern


def validate_catalog_schema(catalog: object) -> list[str]:
    issues: list[str] = []
    if not isinstance(catalog, dict):
        return ["catalog root must be a JSON object"]
    if catalog.get("schemaVersion") != 1:
        issues.append("schemaVersion must be 1")
    outcomes = catalog.get("outcomes")
    if not isinstance(outcomes, list) or not outcomes:
        issues.append("outcomes must be a non-empty array")
        return issues

    seen: set[str] = set()
    for index, raw_outcome in enumerate(outcomes):
        if not isinstance(raw_outcome, dict):
            issues.append(f"outcome[{index}] must be a JSON object")
            continue
        outcome = raw_outcome
        has_id = bool(str(outcome.get("id", "")).strip())
        has_pattern = bool(str(outcome.get("idPattern", "")).strip())
        key = outcome_key(outcome)
        if has_id == has_pattern:
            issues.append(f"outcome[{index}] must define exactly one of id or idPattern")
            continue
        if key in seen:
            issues.append(f"duplicate outcome key '{key}'")
        seen.add(key)
        for field in ("label", "source", "category", "location", "speaker", "line"):
            if not str(outcome.get(field, "")).strip():
                issues.append(f"outcome '{key}' needs non-empty {field}")

        world_event = outcome.get("worldEvent")
        if world_event is None:
            continue
        if not isinstance(world_event, dict):
            issues.append(f"outcome '{key}' worldEvent must be an object")
            continue
        event_type = str(world_event.get("type", "")).strip()
        if event_type not in ALLOWED_WORLD_EVENT_TYPES:
            issues.append(f"outcome '{key}' worldEvent type '{event_type}' is unsupported")
        intensity = world_event.get("intensity")
        if not isinstance(intensity, (int, float)) or not math.isfinite(float(intensity)) or not (0.0 < float(intensity) <= 1.0):
            issues.append(f"outcome '{key}' worldEvent intensity must be in the 0.0-1.0 range")
        cooldown = world_event.get("cooldownSeconds")
        if not isinstance(cooldown, (int, float)) or not math.isfinite(float(cooldown)) or float(cooldown) < 1.0:
            issues.append(f"outcome '{key}' worldEvent cooldownSeconds must be at least 1.0")
    return issues


def extract_runtime_events(policy_text: str) -> dict[str, tuple[str, float, float]]:
    events: dict[str, tuple[str, float, float]] = {}
    marker = "worldEventForObjectAffordance"
    start = policy_text.find(marker)
    if start < 0:
        return events
    policy_text = policy_text[start:]

    branch_body = (
        r"(?P<body>.*?)"
        r"event\.intensity = (?P<intensity>[0-9.]+)f;.*?"
        r"event\.cooldownSeconds = (?P<cooldown>[0-9.]+)f;.*?"
        r"return event;"
    )
    exact_re = re.compile(
        r'affordance\.outcomeId == "(?P<key>[^"]+)".*?' + branch_body,
        re.DOTALL,
    )
    prefix_re = re.compile(
        r'affordance\.outcomeId\.find\("(?P<prefix>[^"]+)"\) == 0.*?' + branch_body,
        re.DOTALL,
    )
    type_re = re.compile(r"event\.type = WorldEventType::(\w+);")
    for match in exact_re.finditer(policy_text):
        event_type = type_re.search(match.group("body"))
        events[match.group("key")] = (
            event_type.group(1) if event_type else "PublicNuisance",
            float(match.group("intensity")),
            float(match.group("cooldown")),
        )
    for match in prefix_re.finditer(policy_text):
        event_type = type_re.search(match.group("body"))
        events[f"{match.group('prefix')}*"] = (
            event_type.group(1) if event_type else "PublicNuisance",
            float(match.group("intensity")),
            float(match.group("cooldown")),
        )
    return events


def extract_runtime_affordance_keys(policy_text: str) -> set[str]:
    keys: set[str] = set()
    marker = "worldObjectInteractionAffordance"
    start = policy_text.find(marker)
    if start < 0:
        return keys
    policy_text = policy_text[start:]

    affordance_re = re.compile(
        r"affordance\(\s*object\s*,\s*(?P<expr>\"[^\"]+\"(?:\s*\+\s*object\.id)?)",
        re.DOTALL,
    )
    for match in affordance_re.finditer(policy_text):
        expression = match.group("expr")
        literal_match = re.match(r'"(?P<literal>[^"]+)"', expression)
        if literal_match is None:
            continue
        key = literal_match.group("literal")
        if "+ object.id" in expression:
            key = f"{key}*"
        keys.add(key)
    return keys


def validate_runtime_alignment(catalog: dict[str, object], runtime_policy: Path) -> list[str]:
    issues: list[str] = []
    runtime_text = runtime_policy.read_text(encoding="utf-8")
    runtime_events = extract_runtime_events(runtime_text)
    runtime_affordance_keys = extract_runtime_affordance_keys(runtime_text)

    outcomes = catalog.get("outcomes", [])
    if not isinstance(outcomes, list):
        return issues

    catalog_keys: set[str] = set()
    catalog_event_keys: set[str] = set()
    for raw_outcome in outcomes:
        if not isinstance(raw_outcome, dict):
            continue
        key = outcome_key(raw_outcome)
        catalog_keys.add(key)
        world_event = raw_outcome.get("worldEvent")
        if not isinstance(world_event, dict):
            if runtime_events and key in runtime_events:
                issues.append(f"outcome '{key}' is eventful in runtime but missing catalog worldEvent metadata")
            continue

        catalog_event_keys.add(key)
        if runtime_events and key not in runtime_events:
            issues.append(f"outcome '{key}' declares worldEvent but runtime has no matching event policy")
            continue

        if runtime_events and key in runtime_events:
            runtime_type, runtime_intensity, runtime_cooldown = runtime_events[key]
            catalog_type = str(world_event.get("type", "")).strip()
            catalog_intensity = float(world_event.get("intensity", -1.0))
            catalog_cooldown = float(world_event.get("cooldownSeconds", -1.0))
            if catalog_type != runtime_type:
                issues.append(f"outcome '{key}' worldEvent type differs: catalog={catalog_type} runtime={runtime_type}")
            if abs(catalog_intensity - runtime_intensity) > 0.0001:
                issues.append(
                    f"outcome '{key}' worldEvent intensity differs: catalog={catalog_intensity} runtime={runtime_intensity}"
                )
            if abs(catalog_cooldown - runtime_cooldown) > 0.0001:
                issues.append(
                    f"outcome '{key}' worldEvent cooldown differs: catalog={catalog_cooldown} runtime={runtime_cooldown}"
                )

    if runtime_events:
        for runtime_key in sorted(runtime_events):
            if runtime_key not in catalog_event_keys:
                issues.append(f"runtime event policy '{runtime_key}' is missing from catalog worldEvent metadata")
    for runtime_key in sorted(runtime_affordance_keys):
        if runtime_key not in catalog_keys:
            issues.append(f"runtime affordance outcome '{runtime_key}' is missing from catalog outcomes")
    for catalog_key in sorted(catalog_keys):
        if catalog_key not in runtime_affordance_keys and catalog_key not in runtime_events:
            issues.append(f"catalog outcome '{catalog_key}' has no matching runtime affordance or event policy")
    return issues


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate Blok 13 object outcome catalog.")
    parser.add_argument("catalog", nargs="?", default="data/world/object_outcome_catalog.json", type=Path)
    parser.add_argument("--runtime-policy", default="src/game/WorldObjectInteraction.cpp", type=Path)
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args()

    catalog = json.loads(args.catalog.read_text(encoding="utf-8"))
    issues = validate_catalog_schema(catalog)
    if isinstance(catalog, dict):
        issues.extend(validate_runtime_alignment(catalog, args.runtime_policy))

    if issues:
        for issue in issues:
            print(f"error: {issue}")
        return 1
    if not args.quiet:
        print(f"object outcome validation passed: {args.catalog}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
