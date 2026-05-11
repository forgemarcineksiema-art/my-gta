#!/usr/bin/env python3
"""Goal guardrail checker for low-capacity sessions.

Usage:
  python tools/verify_goal_guard.py --model 5.3-Codex-Spark

The script checks required goal documents and verifies Spark session cards include:
- model marker,
- declared mode,
- rollback action,
- evidence anchors,
- uncertainty note,
- and progress.md link.
"""

from __future__ import annotations

import argparse
import pathlib
import re
from dataclasses import dataclass


ROOT = pathlib.Path(__file__).resolve().parents[1]


@dataclass
class Check:
    name: str
    ok: bool
    detail: str


def read_text(path: pathlib.Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def check_file(path: pathlib.Path) -> Check:
    return Check(f"file:{path.name}", path.exists(), "present" if path.exists() else "missing")


def check_pattern(text: str, pattern: str, desc: str, source: str) -> Check:
    ok = re.search(pattern, text, re.IGNORECASE | re.MULTILINE) is not None
    return Check(desc, ok, f"{'found' if ok else 'missing'} in {source}")


def check_contains(text: str, needle: str, desc: str, source: str) -> Check:
    lower = text.lower()
    ok = needle.lower() in lower
    return Check(desc, ok, f"{'found' if ok else 'missing'} in {source}: '{needle}'")


def latest_spark_session_card(docs_dir: pathlib.Path) -> pathlib.Path | None:
    candidates = list(docs_dir.glob("goal-session-card-*spark*.md"))
    if not candidates:
        return None
    return max(candidates, key=lambda p: p.stat().st_mtime)


def progress_references(path: pathlib.Path, session_card: pathlib.Path) -> bool:
    return session_card.name in path.read_text(encoding="utf-8", errors="replace")


def has_checked_owner_layer(text: str) -> bool:
    checked = 0
    for line in text.splitlines():
        if re.search(r"^\s*-\s*\[[xX]\]\s+", line):
            checked += 1
    return checked == 1


def has_hypothesis_with_anchors(text: str, tag: str) -> bool:
    needle = tag.lower()
    lines = text.splitlines()
    for i, line in enumerate(lines):
        if line.lower().startswith(f"{needle}:"):
            block = "\n".join(lines[i : i + 12])
            if re.search(r"source:\s*`[^`]+`?", block, re.IGNORECASE) and re.search(
                r"gate:\s*`[^`]+`?", block, re.IGNORECASE
            ):
                return True
    return False


def extract_confidence_scores(text: str) -> list[int]:
    match = re.search(r"Confidence\s*\(pre\):([^\n\r]+)", text, flags=re.IGNORECASE)
    if not match:
        return []
    fragment = match.group(1)
    nums = [int(v) for v in re.findall(r"([0-9]{1,2})/10", fragment)]
    return nums[:4]


def validate_session_card(card_path: pathlib.Path) -> list[Check]:
    text = read_text(card_path)
    return [
        check_contains(text, "model: `5.3-codex-spark`", "session card model", card_path.name),
        check_pattern(text, r"Mode:\s*`?(GREEN|AMBER|RED)`?", "session card mode", card_path.name),
        check_contains(text, "One-sentence goal", "one-sentence goal slice", card_path.name),
        check_contains(text, "Counter-claim:", "counter-claim line", card_path.name),
        check_pattern(text, r"H_true:", "H_true hypothesis", card_path.name),
        check_pattern(text, r"H_false:", "H_false hypothesis", card_path.name),
        check_pattern(text, r"Rollback action", "rollback action", card_path.name),
        check_pattern(text, r"Evidence anchors", "evidence anchors", card_path.name),
        check_pattern(text, r"Uncertainty", "uncertainty", card_path.name),
        check_pattern(text, r"source:", "source lock", card_path.name),
        check_pattern(text, r"gate:", "gate anchor", card_path.name),
        check_pattern(text, r"Owner layer:", "owner layer declaration", card_path.name),
        Check(
            "single checked owner layer",
            has_checked_owner_layer(text),
            "exactly one owner-layer checkbox is selected",
        ),
        Check(
            "H_true with anchors",
            has_hypothesis_with_anchors(text, "h_true"),
            "H_true includes source/gate anchors",
        ),
        Check(
            "H_false with anchors",
            has_hypothesis_with_anchors(text, "h_false"),
            "H_false includes source/gate anchors",
        ),
    ]


def validate_session_card_mode_constraints(text: str, card_path: pathlib.Path) -> list[Check]:
    checks: list[Check] = []
    mode_match = re.search(r"Mode:\s*`?(GREEN|AMBER|RED)`?", text, flags=re.IGNORECASE)
    if not mode_match:
        return checks
    mode = mode_match.group(1).upper()
    if mode == "RED":
        checks.append(
            check_pattern(
                text,
                r"`A`:`|`B`:`|`C`:",
                "red-mode single-choice contract",
                card_path.name,
            )
        )
    if mode == "AMBER":
        checks.append(
            check_pattern(
                text,
                r"`A`:`|`B`:`|`C`:",
                "amber-mode uncertainty branch",
                card_path.name,
            )
        )
    return checks


def validate_session_card_confidence(text: str) -> list[Check]:
    scores = extract_confidence_scores(text)
    checks: list[Check] = []
    checks.append(Check(
        "confidence tuple",
        len(scores) == 4,
        f"found {len(scores)}/4 confidence scores (x/10)",
    ))
    if len(scores) != 4:
        return checks

    mode_match = re.search(r"Mode:\s*`?(GREEN|AMBER|RED)`?", text, flags=re.IGNORECASE)
    mode = mode_match.group(1).upper() if mode_match else ""
    below_or_equal = sum(1 for s in scores if s < 8)

    checks.append(
        Check(
            "confidence mode floor",
            mode != "GREEN" or all(s >= 8 for s in scores),
            "GREEN mode requires all confidence scores >= 8/10",
        )
    )
    checks.append(
        Check(
            "high-risk red floor",
            below_or_equal < 2,
            "fewer than two scores may be below 8/10 (use RED for 2+)",
        )
    )
    return checks


def validate_protocol(protocol: str, shield: str, checklist: str) -> list[Check]:
    return [
        check_contains(protocol, "5.3-codex-spark", "protocol spark marker", "goal-low-capacity-protocol"),
        check_pattern(protocol, r"##\s*0\. Session bootstrap", "protocol bootstrap section", "goal-low-capacity-protocol"),
        check_pattern(protocol, r"##\s*1b\.\s*Spark uncertainty protocol", "protocol uncertainty section", "goal-low-capacity-protocol"),
        check_pattern(protocol, r"Pre-flight lock|one subsystem", "protocol mode guard", "goal-low-capacity-protocol"),
        check_pattern(protocol, r"goal-intellectual-safeguards", "protocol intellectual lock mention", "goal-low-capacity-protocol"),
        check_contains(shield, "spark 5.3 hardening contract", "shield spark hardening", "goal-alignment-shield"),
        check_pattern(shield, r"Source-lock first", "shield source-lock", "goal-alignment-shield"),
        check_pattern(shield, r"Counter-claim lock", "shield counter-claim", "goal-alignment-shield"),
        check_pattern(shield, r"Intellectual lock", "shield intellectual lock", "goal-alignment-shield"),
        check_pattern(checklist, r"0\) Model safety precheck", "checklist safety precheck", "goal-decision-checklist"),
    ]


def validate_intellectual_safeguards(doc: str) -> list[Check]:
    return [
        check_contains(doc, "intellectual safeguards", "intellectual safeguards marker", "goal-intellectual-safeguards"),
        check_pattern(doc, r"Objective-lock requirement", "intellectual objective lock", "goal-intellectual-safeguards"),
        check_pattern(doc, r"Two-sided proof lock", "intellectual proof lock", "goal-intellectual-safeguards"),
        check_pattern(doc, r"H_true", "H_true present", "goal-intellectual-safeguards"),
        check_pattern(doc, r"H_false", "H_false present", "goal-intellectual-safeguards"),
        check_pattern(doc, r"Counter-claim", "counter-claim present", "goal-intellectual-safeguards"),
        check_pattern(doc, r"Rollback", "rollback present", "goal-intellectual-safeguards"),
        check_pattern(doc, r"A/ ?B/ ?C", "A/B/C branch present", "goal-intellectual-safeguards"),
        check_pattern(doc, r"Counterfactual", "counterfactual lock", "goal-intellectual-safeguards"),
        check_pattern(doc, r"verify_goal_guard.py", "self-verification mention", "goal-intellectual-safeguards"),
    ]


def run() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--model", default="5.3-Codex-Spark")
    parser.add_argument("--docs-dir", default=str(ROOT / "docs"))
    parser.add_argument("--progress", default=str(ROOT / "progress.md"))
    args = parser.parse_args()

    docs_dir = pathlib.Path(args.docs_dir)
    progress_path = pathlib.Path(args.progress)
    protocol_path = docs_dir / "goal-low-capacity-protocol-2026-05-09.md"
    shield_path = docs_dir / "goal-alignment-shield-2026-05-09.md"
    checklist_path = docs_dir / "goal-decision-checklist-template.md"
    intellectual_path = docs_dir / "goal-intellectual-safeguards-2026-05-09.md"

    checks: list[Check] = [
        check_file(protocol_path),
        check_file(shield_path),
        check_file(checklist_path),
        check_file(intellectual_path),
    ]

    if not (protocol_path.exists() and shield_path.exists() and checklist_path.exists()):
        return print_checks(checks)

    protocol_text = read_text(protocol_path)
    shield_text = read_text(shield_path)
    checklist_text = read_text(checklist_path)
    intellectual_text = read_text(intellectual_path)
    checks.extend(validate_protocol(protocol_text, shield_text, checklist_text))
    checks.extend(validate_intellectual_safeguards(intellectual_text))

    if args.model.lower() == "5.3-codex-spark":
        card = latest_spark_session_card(docs_dir)
        if not card:
            checks.append(Check("spark session card", False, "missing goal-session-card-*spark*.md"))
        else:
            checks.append(Check("spark session card", True, f"found {card.name}"))
            checks.extend(validate_session_card(card))
            checks.extend(validate_session_card_mode_constraints(read_text(card), card))
            checks.extend(validate_session_card_confidence(read_text(card)))

            if not progress_path.exists():
                checks.append(Check("progress.md", False, "missing"))
            else:
                checks.append(
                    Check(
                        "progress link",
                        progress_references(progress_path, card),
                        "progress.md mentions latest spark session card",
                    )
                )

    return print_checks(checks)


def print_checks(checks: list[Check]) -> int:
    failed = 0
    for c in checks:
        status = "PASS" if c.ok else "FAIL"
        if not c.ok:
            failed += 1
        print(f"[{status}] {c.name} -> {c.detail}")
    if failed:
        print(f"\nGoal guard blocked: {failed}/{len(checks)} checks failed.")
        return 2
    print(f"\nGoal guard passed: {len(checks)}/{len(checks)} checks.")
    return 0


if __name__ == "__main__":
    raise SystemExit(run())
