#!/usr/bin/env python3
# /Users/bfr/atlas/ts/site_office/meas_2026-04-14_15-12-49

"""Split a measurement folder into one folder per XML start datetime.

Example XML name:
    034_2026-04-14_15-12-49_2026-04-14_15-13-21_R000_131072H.xml

For each XML file, the script creates a sibling directory named:
    meas_<startdatetime>

It then moves the XML file and all other files containing the same run token
(for example _R000_) into that directory.

Use --apply to perform the moves. Without it, the script only prints what it
would do.
"""

from __future__ import annotations

import argparse
import re
import shutil
from pathlib import Path

EXAMPLE_SOURCE_DIR = "/Users/bfr/atlas/ts/site_office/meas_2026-04-14_15-12-49"
XML_PATTERN = re.compile(
    r"^(?P<prefix>[^_]+)_(?P<start>\d{4}-\d{2}-\d{2}_\d{2}-\d{2}-\d{2})_"
    r"(?P<stop>\d{4}-\d{2}-\d{2}_\d{2}-\d{2}-\d{2})_(?P<run>R\d+)_.*\.xml$",
    re.IGNORECASE,
)


def parse_xml_name(file_name: str) -> tuple[str, str, str] | None:
    """Extract start datetime, run token and rate tag from an XML file name."""
    match = XML_PATTERN.match(file_name)
    if not match:
        return None

    tail = match.group(0).rsplit(f"_{match.group('run')}_", 1)[-1].removesuffix(".xml")
    rate_tag = tail.split("_")[-1]
    return match.group("start"), match.group("run"), rate_tag


def collect_run_groups(source_dir: Path) -> list[tuple[str, str, str, Path]]:
    """Return all XML-defined run groups found in the source directory."""
    run_groups: list[tuple[str, str, str, Path]] = []

    for path in sorted(source_dir.iterdir()):
        if not path.is_file() or path.suffix.lower() != ".xml":
            continue

        parsed = parse_xml_name(path.name)
        if parsed is None:
            continue

        start_dt, run_id, rate_tag = parsed
        run_groups.append((start_dt, run_id, rate_tag, source_dir.parent / f"meas_{start_dt}"))

    return run_groups


def find_matching_files(source_dir: Path, run_id: str, rate_tag: str) -> list[Path]:
    """Find all files in the source directory that belong to one run and rate tag."""
    run_token = f"_{run_id}_"
    rate_token = f"_{rate_tag}."
    return [
        path
        for path in sorted(source_dir.iterdir())
        if path.is_file() and run_token in path.name and rate_token in path.name
    ]


def move_run_files(
    source_dir: Path, run_id: str, rate_tag: str, start_dt: str, target_dir: Path, apply: bool
) -> int:
    """Move one run group into its target directory."""
    matching_files = find_matching_files(source_dir, run_id, rate_tag)
    if not matching_files:
        print(f"No files found for {run_id} / {rate_tag} ({start_dt})")
        return 0

    print(f"\n{run_id} / {rate_tag} -> {target_dir}")
    if not apply:
        print("  [dry-run] directory will be created if needed")
    else:
        target_dir.mkdir(parents=True, exist_ok=True)

    moved_count = 0
    for src in matching_files:
        dst = target_dir / src.name

        if src.parent == target_dir:
            print(f"  already there: {src.name}")
            continue

        if dst.exists() and src.resolve() != dst.resolve():
            print(f"  skip exists: {dst}")
            continue

        print(f"  {'move' if apply else 'would move'}: {src.name}")
        if apply:
            shutil.move(str(src), str(dst))
        moved_count += 1

    return moved_count


def split_measurements(source_dir: Path, apply: bool = False) -> int:
    """Scan the directory and process all XML-defined run groups."""
    if source_dir.name.startswith("meas") is False:
        raise ValueError(f"Directory name must start with 'meas': {source_dir}")

    if not source_dir.exists() or not source_dir.is_dir():
        raise FileNotFoundError(f"Directory not found: {source_dir}")

    run_groups = collect_run_groups(source_dir)
    if not run_groups:
        print(f"No matching XML files found in {source_dir}")
        return 0

    print(f"Found {len(run_groups)} run group(s) in {source_dir}")

    moved_total = 0
    for start_dt, run_id, rate_tag, target_dir in run_groups:
        moved_total += move_run_files(source_dir, run_id, rate_tag, start_dt, target_dir, apply)

    print(f"\nTotal {'moved' if apply else 'matched'} files: {moved_total}")
    return moved_total


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Create meas_<startdatetime> folders and move matching run files into them."
    )
    parser.add_argument(
        "source_dir",
        help=f"Measurement directory to scan. Must start with 'meas', for example: {EXAMPLE_SOURCE_DIR}",
    )
    parser.add_argument(
        "--apply",
        action="store_true",
        help="Actually move the files. Without this flag, only a preview is shown.",
    )
    args = parser.parse_args()

    split_measurements(Path(args.source_dir).expanduser().resolve(), apply=args.apply)

# run like: python split_mt.py /Users/bfr/atlas/ts/site_office/meas_2026-04-14_15-12-49 --apply
if __name__ == "__main__":
    main()
