#!/usr/bin/env python3
"""Unit tests for validate_object_outcomes.py."""

from __future__ import annotations

import tempfile
from pathlib import Path

import validate_object_outcomes as validator


RUNTIME_POLICY = """
std::optional<WorldObjectInteractionAffordance> worldObjectInteractionAffordance(const WorldObject& object) {
    if (object.id == "shop_rolling_grille") {
        return affordance(object,
                          "shop_rolling_grille_checked",
                          "E: sprawdz rolete",
                          "Roleta",
                          "Porysowana roleta.",
                          InteractionAction::UseDoor,
                          21);
    }
    if (hasTag(object, "shop_price_card")) {
        return affordance(object,
                          "shop_prices_read_" + object.id,
                          "E: przeczytaj ceny",
                          "Kartka",
                          "Promocja.",
                          InteractionAction::UseMarker,
                          15);
    }
    if (hasTag(object, "readable_notice")) {
        return affordance(object,
                          "shop_notice_read_" + object.id,
                          "E: przeczytaj kartke",
                          "Kartka Zenona",
                          "Kartka.",
                          InteractionAction::UseMarker,
                          14);
    }
    return std::nullopt;
}

std::optional<WorldObjectInteractionEvent> worldEventForObjectAffordance(
    const WorldObjectInteractionAffordance& affordance) {
    WorldObjectInteractionEvent event;
    event.type = WorldEventType::PublicNuisance;
    event.position = affordance.position;
    event.source = affordance.outcomeId;

    if (affordance.outcomeId == "block_intercom_buzzed") {
        event.intensity = 0.20f;
        event.cooldownSeconds = 3.0f;
        return event;
    }

    if (affordance.outcomeId.find("trash_disturbed_") == 0) {
        event.intensity = 0.18f;
        event.cooldownSeconds = 2.5f;
        return event;
    }

    return std::nullopt;
}
"""


def catalog(outcomes: list[dict[str, object]]) -> dict[str, object]:
    return {"schemaVersion": 1, "outcomes": outcomes}


def outcome(key: str, world_event: dict[str, object] | None) -> dict[str, object]:
    result: dict[str, object] = {
        "label": key,
        "source": f"object:{key}",
        "category": "object_use",
        "location": "Block",
        "speaker": "Test",
        "line": "Readable outcome line.",
    }
    if key.endswith("*"):
        result["idPattern"] = key
    else:
        result["id"] = key
    if world_event is not None:
        result["worldEvent"] = world_event
    return result


def with_policy_file(body) -> None:
    with tempfile.TemporaryDirectory() as temp_dir:
        path = Path(temp_dir) / "WorldObjectInteraction.cpp"
        path.write_text(RUNTIME_POLICY, encoding="utf-8")
        body(path)


def assert_issue(issues: list[str], needle: str) -> None:
    assert any(needle in issue for issue in issues), f"missing issue containing {needle!r}: {issues}"


def test_matching_catalog_passes() -> None:
    def run(path: Path) -> None:
        data = catalog([
            outcome("shop_rolling_grille_checked", None),
            outcome("block_intercom_buzzed", {"type": "PublicNuisance", "intensity": 0.2, "cooldownSeconds": 3.0}),
            outcome("trash_disturbed_*", {"type": "PublicNuisance", "intensity": 0.18, "cooldownSeconds": 2.5}),
            outcome("shop_prices_read_*", None),
            outcome("shop_notice_read_*", None),
        ])
        assert validator.validate_catalog_schema(data) == []
        assert validator.validate_runtime_alignment(data, path) == []

    with_policy_file(run)


def test_runtime_event_missing_catalog_metadata_fails() -> None:
    def run(path: Path) -> None:
        data = catalog([
            outcome("block_intercom_buzzed", None),
            outcome("trash_disturbed_*", {"type": "PublicNuisance", "intensity": 0.18, "cooldownSeconds": 2.5}),
        ])
        assert_issue(validator.validate_runtime_alignment(data, path), "missing catalog worldEvent metadata")

    with_policy_file(run)


def test_catalog_runtime_intensity_drift_fails() -> None:
    def run(path: Path) -> None:
        data = catalog([
            outcome("block_intercom_buzzed", {"type": "PublicNuisance", "intensity": 0.4, "cooldownSeconds": 3.0}),
            outcome("trash_disturbed_*", {"type": "PublicNuisance", "intensity": 0.18, "cooldownSeconds": 2.5}),
        ])
        assert_issue(validator.validate_runtime_alignment(data, path), "intensity differs")

    with_policy_file(run)


def test_bad_event_metadata_fails_schema() -> None:
    data = catalog([
        outcome("bad", {"type": "Noise", "intensity": 2.0, "cooldownSeconds": 0.0}),
    ])
    issues = validator.validate_catalog_schema(data)
    assert_issue(issues, "unsupported")
    assert_issue(issues, "intensity")
    assert_issue(issues, "cooldownSeconds")


def test_missing_feedback_text_fails_schema() -> None:
    data = catalog([
        {
            "id": "silent_notice",
            "label": "Silent notice",
            "source": "object:silent_notice",
            "category": "object_use",
            "location": "Block",
            "speaker": "",
            "line": "",
        },
    ])
    issues = validator.validate_catalog_schema(data)
    assert_issue(issues, "speaker")
    assert_issue(issues, "line")


def test_runtime_affordance_missing_catalog_hook_fails() -> None:
    def run(path: Path) -> None:
        data = catalog([
            outcome("shop_rolling_grille_checked", None),
            outcome("block_intercom_buzzed", {"type": "PublicNuisance", "intensity": 0.2, "cooldownSeconds": 3.0}),
            outcome("trash_disturbed_*", {"type": "PublicNuisance", "intensity": 0.18, "cooldownSeconds": 2.5}),
            outcome("shop_prices_read_*", None),
        ])
        assert_issue(validator.validate_runtime_alignment(data, path), "shop_notice_read_*")

    with_policy_file(run)


def test_catalog_quiet_hook_missing_runtime_affordance_fails() -> None:
    def run(path: Path) -> None:
        data = catalog([
            outcome("shop_rolling_grille_checked", None),
            outcome("block_intercom_buzzed", {"type": "PublicNuisance", "intensity": 0.2, "cooldownSeconds": 3.0}),
            outcome("trash_disturbed_*", {"type": "PublicNuisance", "intensity": 0.18, "cooldownSeconds": 2.5}),
            outcome("shop_prices_read_*", None),
            outcome("shop_notice_read_*", None),
            outcome("ghost_notice_read_*", None),
        ])
        assert_issue(validator.validate_runtime_alignment(data, path), "ghost_notice_read_*")

    with_policy_file(run)


def test_catalog_only_mode_passes_when_runtime_has_no_events() -> None:
    empty_policy = """
std::optional<WorldObjectInteractionAffordance> worldObjectInteractionAffordance(const WorldObject& object) {
    if (object.id == "shop_door_front") {
        return affordance(object, "shop_door_checked", "E: sprawdz drzwi", "Drzwi", "Klamka chodzi.", InteractionAction::UseDoor, 24);
    }
    if (hasTag(object, "glass_surface")) {
        return affordance(object, "glass_peeked_" + object.id, "E: zajrzyj", "Szyba", "Widac sklep.", InteractionAction::UseMarker, 16);
    }
    if (hasTag(object, "parking_sign")) {
        return affordance(object, "parking_sign_read_" + object.id, "E: przeczytaj", "Znak", "Znak.", InteractionAction::UseMarker, 15);
    }
    return std::nullopt;
}
std::optional<WorldObjectInteractionEvent> worldEventForObjectAffordance(
    const WorldObjectInteractionAffordance&) {
    return std::nullopt;
}
"""
    with tempfile.TemporaryDirectory() as temp_dir:
        path = Path(temp_dir) / "WorldObjectInteraction.cpp"
        path.write_text(empty_policy, encoding="utf-8")
        data = catalog([
            outcome("shop_door_checked", {"type": "PublicNuisance", "intensity": 0.12, "cooldownSeconds": 4.0}),
            outcome("glass_peeked_*", {"type": "ShopTrouble", "intensity": 0.10, "cooldownSeconds": 5.0}),
            outcome("parking_sign_read_*", None),
        ])
        assert validator.validate_runtime_alignment(data, path) == []


def main() -> int:
    tests = [
        test_matching_catalog_passes,
        test_runtime_event_missing_catalog_metadata_fails,
        test_catalog_runtime_intensity_drift_fails,
        test_bad_event_metadata_fails_schema,
        test_missing_feedback_text_fails_schema,
        test_runtime_affordance_missing_catalog_hook_fails,
        test_catalog_quiet_hook_missing_runtime_affordance_fails,
        test_catalog_only_mode_passes_when_runtime_has_no_events,
    ]
    for test in tests:
        test()
        print(f"PASS {test.__name__}")
    print(f"{len(tests)} object outcome validator test(s) passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
