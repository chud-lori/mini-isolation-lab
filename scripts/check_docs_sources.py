#!/usr/bin/env python3
"""Check or refresh source references embedded in the static docs."""

from __future__ import annotations

import argparse
import html
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / "docs"

SOURCE_PANEL_RE = re.compile(
    r'(<details class="source-panel"><summary>Show source: (?P<path>[^<]+)</summary>'
    r'<pre><code class="language-[^"]+">)(?P<code>.*?)(</code></pre></details>)',
    re.DOTALL,
)
GITHUB_BLOB_RE = re.compile(
    r"https://github\.com/chud-lori/mini-isolation-lab/blob/main/(?P<path>[^\"#?]+)"
)
SOURCE_FOCUS_RE = re.compile(r"Source focus: (?P<path>[^<]+)")
COURSE_PATH_RE = re.compile(r'<span class="path">(?P<path>[^<]+)</span>')
HREF_RE = re.compile(r'href="(?P<href>[^"]+)"')
SOURCE_FOCUS_SENTINELS = {"course primer"}
COURSE_PATH_SENTINELS = {"read this first"}


def html_source_for(path: Path) -> str:
    text = path.read_text(encoding="utf-8")
    return html.escape(text, quote=False)


def resolve_repo_path(path_text: str) -> Path | None:
    path = Path(path_text)
    candidates = [ROOT / path]
    if not path.is_absolute():
        candidates.append(ROOT / "mini-kernel" / path)
    for candidate in candidates:
        if candidate.exists():
            return candidate
    return None


def repo_path_exists(path_text: str) -> bool:
    return resolve_repo_path(path_text) is not None


def rel_to_root(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def check_source_panels(path: Path, write: bool) -> list[str]:
    original = path.read_text(encoding="utf-8")
    errors: list[str] = []
    changed = False

    def replace(match: re.Match[str]) -> str:
        nonlocal changed
        source_rel = match.group("path")
        source_path = ROOT / source_rel
        if not source_path.exists():
            errors.append(f"{path}: missing source panel file {source_rel}")
            return match.group(0)

        expected = html_source_for(source_path)
        actual = match.group("code")
        if actual != expected:
            errors.append(f"{path}: source panel drift for {source_rel}")
            if write:
                changed = True
                return f"{match.group(1)}{expected}{match.group(4)}"
        return match.group(0)

    updated = SOURCE_PANEL_RE.sub(replace, original)
    if write and changed:
        path.write_text(updated, encoding="utf-8")
    return errors


def check_references(path: Path) -> list[str]:
    text = path.read_text(encoding="utf-8")
    errors: list[str] = []

    github_paths = {html.unescape(match.group("path")) for match in GITHUB_BLOB_RE.finditer(text)}

    for match in GITHUB_BLOB_RE.finditer(text):
        ref = html.unescape(match.group("path"))
        if not repo_path_exists(ref):
            errors.append(f"{path}: missing GitHub source target {ref}")

    for match in SOURCE_FOCUS_RE.finditer(text):
        ref = html.unescape(match.group("path"))
        if ref in SOURCE_FOCUS_SENTINELS:
            continue
        resolved = resolve_repo_path(ref)
        if resolved is None:
            errors.append(f"{path}: missing source focus {ref}")
            continue
        expected = rel_to_root(resolved)
        if expected not in github_paths:
            errors.append(f"{path}: source focus {ref} missing GitHub link for {expected}")

    for match in COURSE_PATH_RE.finditer(text):
        ref = html.unescape(match.group("path"))
        if ref in COURSE_PATH_SENTINELS:
            continue
        if not repo_path_exists(ref):
            errors.append(f"{path}: missing course path source {ref}")

    return errors


def check_local_links(path: Path) -> list[str]:
    text = path.read_text(encoding="utf-8")
    errors: list[str] = []

    for match in HREF_RE.finditer(text):
        href = html.unescape(match.group("href"))
        if (
            href.startswith(("http://", "https://", "mailto:"))
            or href.startswith("#")
            or href == ""
        ):
            continue

        target_text = href.split("#", 1)[0]
        if target_text == "":
            continue

        target = (path.parent / target_text).resolve()
        if target.is_dir():
            target = target / "index.html"

        try:
            target.relative_to(ROOT)
        except ValueError:
            errors.append(f"{path}: local link escapes repository {href}")
            continue

        if not target.exists():
            errors.append(f"{path}: missing local link target {href}")

    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--write",
        action="store_true",
        help="refresh stale embedded source panels in place",
    )
    args = parser.parse_args()

    errors: list[str] = []
    for path in sorted(DOCS.rglob("*.html")):
        errors.extend(check_source_panels(path, args.write))
        errors.extend(check_references(path))
        errors.extend(check_local_links(path))

    if errors:
        for error in errors:
            print(error, file=sys.stderr)
        if args.write:
            print("refreshed stale source panels; rerun without --write to verify")
        return 1

    print("docs source references are current")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
