#!/usr/bin/env python3
"""Remove channel entries by id from ADU measurement XML files.

This script uses only Python's standard library.
"""

from __future__ import annotations

import argparse
import shutil
import sys
from pathlib import Path
import xml.etree.ElementTree as ET


def parse_channel_ids(raw_ids: list[str]) -> set[str]:
	"""Parse comma-separated and repeated channel id arguments into a set."""
	ids: set[str] = set()
	for item in raw_ids:
		for token in item.split(","):
			token = token.strip()
			if not token:
				continue
			if not token.isdigit():
				raise ValueError(f"Invalid channel id '{token}'; expected non-negative integer")
			ids.add(str(int(token)))
	if not ids:
		raise ValueError("No channel ids given")
	return ids


def remove_channel_nodes(root: ET.Element, ids_to_remove: set[str]) -> int:
	"""Remove <channel id="..."> nodes anywhere in the XML tree."""
	removed = 0
	for parent in root.iter():
		children = list(parent)
		for child in children:
			if child.tag != "channel":
				continue
			channel_id = child.get("id")
			if channel_id in ids_to_remove:
				parent.remove(child)
				removed += 1

	return removed


def write_xml(tree: ET.ElementTree, output_path: Path) -> None:
	"""Write XML with declaration and UTF-8 encoding."""
	tree.write(output_path, encoding="utf-8", xml_declaration=True)


def sync_meas_channels(root: ET.Element) -> int:
	"""Set global_config/meas_channels to the current channel_config/channel count."""
	updated = 0
	for hardware in root.findall(".//input/Hardware"):
		meas_channels = hardware.find("./global_config/meas_channels")
		channel_nodes = hardware.findall("./channel_config/channel")
		if meas_channels is None:
			continue

		new_value = str(len(channel_nodes))
		if (meas_channels.text or "").strip() != new_value:
			meas_channels.text = new_value
			updated += 1

	return updated


def discover_xml_files(root_dir: Path) -> list[Path]:
	"""Find XML files recursively below root_dir."""
	return sorted(path for path in root_dir.rglob("*.xml") if path.is_file())


def confirm_file_list(files: list[Path], root_dir: Path) -> bool:
	"""Print discovered files and ask user for confirmation."""
	print(f"Found {len(files)} XML file(s) under: {root_dir}")
	for idx, path in enumerate(files, start=1):
		print(f"{idx:4d}. {path}")

	answer = input("Process these files? [Y/n] ").strip().lower()
	return answer in {"", "y", "yes"}


def process_one_file(xml_path: Path, ids_to_remove: set[str]) -> tuple[int, int, str | None]:
	"""Process one XML file and return (removed_count, meas_updates, error_message)."""
	try:
		tree = ET.parse(xml_path)
	except ET.ParseError as exc:
		return 0, 0, f"XML parse error: {exc}"
	root = tree.getroot()
	removed = remove_channel_nodes(root, ids_to_remove)
	meas_updates = sync_meas_channels(root)

	backup_path = xml_path.with_suffix(xml_path.suffix + ".back")
	shutil.copy2(xml_path, backup_path)
	write_xml(tree, xml_path)
	return removed, meas_updates, None


def build_parser() -> argparse.ArgumentParser:
	parser = argparse.ArgumentParser(
		description=(
			"Recursively find XML files, ask once for confirmation, then remove channel ids "
			"from all files while always writing a .back backup copy."
		)
	)
	parser.add_argument(
		"-r",
		"--remove",
		action="append",
		default=None,
		metavar="IDS",
		help="Channel ids to remove, e.g. --remove 0 --remove 2,4 (default: 4)",
	)
	parser.add_argument(
		"-d",
		"--root-dir",
		type=Path,
		default=Path.cwd(),
		help="Root directory for recursive XML search (default: current directory)",
	)
	return parser

def main() -> int:
	parser = build_parser()
	args = parser.parse_args()

	raw_remove_args = args.remove if args.remove is not None else ["4"]

	try:
		ids_to_remove = parse_channel_ids(raw_remove_args)
	except ValueError as exc:
		parser.error(str(exc))

	root_dir: Path = args.root_dir
	if not root_dir.exists() or not root_dir.is_dir():
		parser.error(f"Root directory not found or not a directory: {root_dir}")

	xml_files = discover_xml_files(root_dir)
	if not xml_files:
		print(f"No XML files found under: {root_dir}")
		return 0

	if not confirm_file_list(xml_files, root_dir):
		print("Cancelled by user.")
		return 1

	total_removed = 0
	total_meas_updates = 0
	error_count = 0
	for xml_path in xml_files:
		removed, meas_updates, error_msg = process_one_file(xml_path, ids_to_remove)
		if error_msg:
			error_count += 1
			print(f"ERROR {xml_path}: {error_msg}", file=sys.stderr)
			continue

		total_removed += removed
		total_meas_updates += meas_updates
		print(f"OK {xml_path}: removed {removed} channel node(s), meas_channels updated {meas_updates} time(s)")

	print(
		f"Done. Files: {len(xml_files)}, errors: {error_count}, "
		f"removed nodes: {total_removed}, meas_channels updates: {total_meas_updates}, "
		f"ids: {', '.join(sorted(ids_to_remove, key=int))}"
	)
	return 2 if error_count else 0


if __name__ == "__main__":
	raise SystemExit(main())

