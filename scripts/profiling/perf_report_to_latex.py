#!/usr/bin/env python3
"""perf report to latex table"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, List, Optional


@dataclass(frozen=True)
class PerfRow:
    overhead: str
    command: str
    dso: str
    symbol: str


_OVERHEAD_RE = re.compile(r"^\s*\d+(?:\.\d+)?%\s+")


def _latex_escape(text: str) -> str:
    # latex escape table cells
    # escape backslash first
    replacements = {
        "\\": r"\\textbackslash{}",
        "&": r"\\&",
        "%": r"\\%",
        "#": r"\\#",
        "_": r"\\_",
        "{": r"\\{",
        "}": r"\\}",
        "~": r"\\textasciitilde{}",
        "^": r"\\textasciicircum{}",
        "$": r"\\$",
    }
    return "".join(replacements.get(ch, ch) for ch in text)


def _split_columns(line: str) -> Optional[PerfRow]:
    if not _OVERHEAD_RE.match(line):
        return None

    # perf fixed width columns
    parts = re.split(r"\s{2,}", line.strip())
    if len(parts) < 4:
        return None

    overhead, command, dso, symbol = parts[0], parts[1], parts[2], parts[3]
    return PerfRow(overhead=overhead, command=command, dso=dso, symbol=symbol)


def parse_perf_report(lines: Iterable[str], *, unique: bool) -> List[PerfRow]:
    rows: List[PerfRow] = []
    seen = set()

    for line in lines:
        row = _split_columns(line)
        if row is None:
            continue

        if unique:
            key = (row.command, row.dso, row.symbol)
            if key in seen:
                continue
            seen.add(key)

        rows.append(row)

    return rows


def render_latex(rows: List[PerfRow], *, top: int, as_table_env: bool, caption: str, label: str) -> str:
    selected = rows[: max(0, top)]

    def tt(text: str) -> str:
        return r"\\texttt{" + _latex_escape(text) + "}"

    lines: List[str] = []

    if as_table_env:
        lines.append(r"\\begin{table}[ht]")
        lines.append(r"\\centering")

    lines.append(r"\\begin{tabular}{r l l p{0.55\\linewidth}}")
    lines.append(r"\\hline")
    lines.append(r"Overhead & Command & DSO & Symbol \\")
    lines.append(r"\\hline")

    for r in selected:
        lines.append(
            f"{_latex_escape(r.overhead)} & {tt(r.command)} & {tt(r.dso)} & {tt(r.symbol)} \\\\"  # noqa: E501
        )

    lines.append(r"\\hline")
    lines.append(r"\\end{tabular}")

    if as_table_env:
        if caption:
            lines.append(r"\\caption{" + _latex_escape(caption) + "}")
        if label:
            lines.append(r"\\label{" + _latex_escape(label) + "}")
        lines.append(r"\\end{table}")

    return "\n".join(lines) + "\n"


def main(argv: List[str]) -> int:
    p = argparse.ArgumentParser(description="Convert perf report text to a LaTeX table")
    p.add_argument("input", type=Path, help="Path to perf.report.full.txt (or similar)")
    p.add_argument("--top", type=int, default=5, help="How many rows to include (default: 5)")
    p.add_argument(
        "--unique",
        action="store_true",
        help="Drop duplicate (command,dso,symbol) rows before taking top N",
    )
    p.add_argument(
        "--table",
        action="store_true",
        help="Wrap the tabular in a LaTeX table environment",
    )
    p.add_argument("--caption", default="", help="Optional caption (only with --table)")
    p.add_argument("--label", default="", help="Optional label (only with --table)")

    args = p.parse_args(argv)

    if not args.input.is_file():
        print(f"Input file not found: {args.input}", file=sys.stderr)
        return 2

    rows = parse_perf_report(args.input.read_text(encoding="utf-8", errors="replace").splitlines(), unique=args.unique)
    if not rows:
        print("No perf rows found. Did you pass a `perf report --stdio` file?", file=sys.stderr)
        return 3

    sys.stdout.write(
        render_latex(
            rows,
            top=args.top,
            as_table_env=args.table,
            caption=args.caption,
            label=args.label,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
