from __future__ import annotations

from pathlib import Path
import sqlite3
from typing import Iterable

import numpy as np

SCRIPT_DIR = Path(__file__).resolve().parent
SOURCE_DB = SCRIPT_DIR / "fir_filter.db"
TARGET_DB = SCRIPT_DIR / "filter.db"
MTX16_PATH = SCRIPT_DIR / "mtx16x_from_4x4x.txt"


def quote_ident(name: str) -> str:
	return '"' + name.replace('"', '""') + '"'


def list_tables(conn: sqlite3.Connection) -> list[str]:
	rows = conn.execute(
		"""
		SELECT name
		FROM sqlite_master
		WHERE type='table' AND name NOT LIKE 'sqlite_%'
		ORDER BY name
		"""
	).fetchall()
	return [r[0] for r in rows]


def read_coeffs(src_conn: sqlite3.Connection, table: str) -> list[float]:
	qtable = quote_ident(table)
	rows = src_conn.execute(f"SELECT coeff FROM {qtable}").fetchall()
	coeffs: list[float] = []
	for (val,) in rows:
		# Python float is IEEE-754 double precision.
		coeffs.append(float(val))
	return coeffs


def create_coeff_table(dst_conn: sqlite3.Connection, table: str) -> None:
	qtable = quote_ident(table)
	dst_conn.execute(f"CREATE TABLE {qtable} (coeff REAL NOT NULL)")


def insert_coeffs(dst_conn: sqlite3.Connection, table: str, coeffs: Iterable[float]) -> int:
	qtable = quote_ident(table)
	rows = [(float(v),) for v in coeffs]
	dst_conn.executemany(f"INSERT INTO {qtable} (coeff) VALUES (?)", rows)
	return len(rows)


def build_filter_db() -> None:
	if not SOURCE_DB.exists():
		raise FileNotFoundError(f"Source DB not found: {SOURCE_DB}")
	if not MTX16_PATH.exists():
		raise FileNotFoundError(f"Missing coefficient file: {MTX16_PATH}")

	if TARGET_DB.exists():
		TARGET_DB.unlink()

	with sqlite3.connect(SOURCE_DB) as src_conn, sqlite3.connect(TARGET_DB) as dst_conn:
		tables = list_tables(src_conn)
		copied = 0

		for table in tables:
			coeffs = read_coeffs(src_conn, table)
			create_coeff_table(dst_conn, table)
			copied_rows = insert_coeffs(dst_conn, table, coeffs)
			print(f"Copied {table}: {copied_rows} rows as REAL")
			copied += copied_rows

		# Add derived 16x filter coefficients from text file.
		coeffs_16 = np.loadtxt(MTX16_PATH, dtype=np.float64)
		if coeffs_16.ndim != 1:
			coeffs_16 = np.ravel(coeffs_16)

		create_coeff_table(dst_conn, "mtx16")
		inserted_16 = insert_coeffs(dst_conn, "mtx16", coeffs_16.tolist())
		print(f"Added mtx16: {inserted_16} rows as REAL from {MTX16_PATH.name}")

		dst_conn.commit()
		print(f"Done. Wrote {TARGET_DB} with {len(tables) + 1} tables and {copied + inserted_16} rows.")


if __name__ == "__main__":
	build_filter_db()