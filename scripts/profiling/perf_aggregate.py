#!/usr/bin/env python3
"""Aggregate perf report outputs across multiple runs.

Reads `perf report --stdio` text outputs (the files your scripts already save as
`perf.report.full.txt` / `perf.report.app.txt`) and computes mean/stdev overhead
for symbols across runs.

Typical usage:
  python3 scripts/profiling/perf_aggregate.py profiles/perf --scenario s2_scroll_typing --mode full --top 10 --latex --table \
    --caption "Scenario 2 (scroll+typing) mean over 3 runs" --label "tab:perf-s2-full-mean" > profiles/perf/s2_full_mean.tex

Or generate all scenarios:
  python3 scripts/profiling/perf_aggregate.py profiles/perf --all --mode full --top 10 --latex --table --out-dir profiles/perf

Notes:
- By default, missing symbols are treated as 0% overhead for a run (more honest
  for aggregation; avoids “only when present” inflation).
"""

from __future__ import annotations

import argparse
import math
import re
import statistics
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Tuple


@dataclass(frozen=True)
class PerfRow:
    overhead_pct: float
    command: str
    dso: str
    symbol: str


_OVERHEAD_LINE_RE = re.compile(r"^\s*\d+(?:\.\d+)?%\s+")
_SCENARIO_DIR_RE = re.compile(r"^(?P<scenario>.+)_\d{8}-\d{6}$")


def _latex_escape(text: str) -> str:
    repl = {
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
    return "".join(repl.get(ch, ch) for ch in text)


def _split_columns(line: str) -> Optional[Tuple[str, str, str, str]]:
    if not _OVERHEAD_LINE_RE.match(line):
        return None

    # perf uses fixed-width columns separated by 2+ spaces
    parts = re.split(r"\s{2,}", line.strip())
    if len(parts) < 4:
        return None

    return parts[0], parts[1], parts[2], parts[3]


def parse_perf_report(path: Path) -> List[PerfRow]:
    rows: List[PerfRow] = []
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        cols = _split_columns(line)
        if cols is None:
            continue
        overhead_s, command, dso, symbol = cols
        m = re.match(r"^(\d+(?:\.\d+)?)%$", overhead_s)
        if not m:
            continue
        overhead_pct = float(m.group(1))
        rows.append(PerfRow(overhead_pct=overhead_pct, command=command, dso=dso, symbol=symbol))
    return rows


def discover_runs(base_dir: Path) -> Dict[str, List[Path]]:
    scenarios: Dict[str, List[Path]] = {}
    for child in sorted(base_dir.iterdir()):
        if not child.is_dir():
            continue
        m = _SCENARIO_DIR_RE.match(child.name)
        if not m:
            continue
        scenario = m.group("scenario")
        scenarios.setdefault(scenario, []).append(child)
    return scenarios


def stats(values: List[float]) -> Tuple[float, float]:
    if not values:
        return 0.0, 0.0
    if len(values) == 1:
        return values[0], 0.0
    mean = statistics.fmean(values)
    # sample stdev is more appropriate for a small number of runs
    stdev = statistics.stdev(values)
    return mean, stdev


def aggregate(
    run_dirs: List[Path],
    report_name: str,
    *,
    include_missing_as_zero: bool,
) -> Tuple[int, Dict[Tuple[str, str, str], List[float]]]:
    """Return (n_runs, key -> list of overheads per run).

    key := (command,dso,symbol)
    """

    per_run_maps: List[Dict[Tuple[str, str, str], float]] = []
    all_keys: set[Tuple[str, str, str]] = set()

    for d in run_dirs:
        report_path = d / report_name
        if not report_path.is_file():
            continue
        rows = parse_perf_report(report_path)
        m: Dict[Tuple[str, str, str], float] = {}
        for r in rows:
            k = (r.command, r.dso, r.symbol)
            # perf report should already be unique, but be defensive
            m[k] = max(m.get(k, 0.0), r.overhead_pct)
        per_run_maps.append(m)
        all_keys |= set(m.keys())

    n = len(per_run_maps)
    series: Dict[Tuple[str, str, str], List[float]] = {k: [] for k in all_keys}

    for k in all_keys:
        for m in per_run_maps:
            if k in m:
                series[k].append(m[k])
            elif include_missing_as_zero:
                series[k].append(0.0)

    return n, series


def render_latex_table(
    rows: List[Tuple[float, float, int, str, str, str]],
    *,
    top: int,
    as_table_env: bool,
    caption: str,
    label: str,
    note: str,
) -> str:
    selected = rows[: max(0, top)]

    out: List[str] = []
    if as_table_env:
        out.append(r"\\begin{table}[ht]")
        out.append(r"\\centering")

    out.append(r"\\begin{tabular}{r r r l p{0.52\\linewidth}}")
    out.append(r"\\hline")
    out.append(r"Mean\\% & SD\\% & N & DSO & Symbol \\\\")
    out.append(r"\\hline")

    for mean, sd, n, _command, dso, sym in selected:
        out.append(
            f"{mean:0.2f} & {sd:0.2f} & {n:d} & \\texttt{{{_latex_escape(dso)}}} & \\texttt{{{_latex_escape(sym)}}} \\\\"  # noqa: E501
        )

    out.append(r"\\hline")
    out.append(r"\\end{tabular}")

    if note:
        out.append(r"\\\\")
        out.append(r"\\footnotesize{" + _latex_escape(note) + "}")

    if as_table_env:
        if caption:
            out.append(r"\\caption{" + _latex_escape(caption) + "}")
        if label:
            out.append(r"\\label{" + _latex_escape(label) + "}")
        out.append(r"\\end{table}")

    return "\n".join(out) + "\n"


def render_csv(rows: List[Tuple[float, float, int, str, str, str]], *, top: int) -> str:
    selected = rows[: max(0, top)]
    out = ["mean_pct,stdev_pct,n_runs,command,dso,symbol"]
    for mean, sd, n, cmd, dso, sym in selected:
        out.append(f"{mean:.4f},{sd:.4f},{n},{cmd},{dso},{sym}")
    return "\n".join(out) + "\n"


def main(argv: List[str]) -> int:
    p = argparse.ArgumentParser(description="Aggregate perf report outputs across multiple runs")
    p.add_argument("base_dir", type=Path, help="profiles/perf directory")

    sel = p.add_mutually_exclusive_group(required=True)
    sel.add_argument("--scenario", help="Scenario name (e.g. s2_scroll_typing)")
    sel.add_argument("--all", action="store_true", help="Aggregate all scenarios discovered under base_dir")

    p.add_argument("--mode", choices=["full", "app"], default="full")
    p.add_argument("--top", type=int, default=10)
    p.add_argument("--include-missing-as-zero", action="store_true", default=True)
    p.add_argument("--only-when-present", action="store_true", help="Compute stats only for runs where the symbol appears")

    fmt = p.add_mutually_exclusive_group(required=False)
    fmt.add_argument("--latex", action="store_true", help="Output LaTeX")
    fmt.add_argument("--csv", action="store_true", help="Output CSV")

    p.add_argument("--table", action="store_true", help="(LaTeX) wrap in table env")
    p.add_argument("--caption", default="")
    p.add_argument("--label", default="")

    p.add_argument("--out-dir", type=Path, default=None, help="When --all, write one file per scenario into this dir")

    args = p.parse_args(argv)

    if not args.base_dir.is_dir():
        print(f"Base dir not found: {args.base_dir}", file=sys.stderr)
        return 2

    report_name = "perf.report.full.txt" if args.mode == "full" else "perf.report.app.txt"

    scenarios = discover_runs(args.base_dir)
    if not scenarios:
        print("No run directories found (expected <scenario>_YYYYMMDD-HHMMSS).", file=sys.stderr)
        return 3

    include_missing_as_zero = args.include_missing_as_zero and (not args.only_when_present)

    def compute_for(scenario: str, run_dirs: List[Path]) -> str:
        n, series = aggregate(run_dirs, report_name, include_missing_as_zero=include_missing_as_zero)
        if n == 0:
            raise RuntimeError(f"No usable reports found for scenario={scenario} mode={args.mode}")

        rows: List[Tuple[float, float, int, str, str, str]] = []
        for (cmd, dso, sym), values in series.items():
            # If only_when_present, series values won't include missing runs.
            mean, sd = stats(values)
            rows.append((mean, sd, len(values), cmd, dso, sym))

        rows.sort(key=lambda r: r[0], reverse=True)

        note = "missing treated as 0%" if include_missing_as_zero else "only when present"

        if args.csv:
            return render_csv(rows, top=args.top)

        # default to LaTeX
        caption = args.caption or f"{scenario} ({args.mode}) mean overhead (top {args.top})"
        label = args.label
        if not label:
            safe = re.sub(r"[^a-zA-Z0-9]+", "-", scenario).strip("-")
            label = f"tab:perf-{safe}-{args.mode}-mean"

        return render_latex_table(
            rows,
            top=args.top,
            as_table_env=args.table,
            caption=caption,
            label=label,
            note=note,
        )

    if args.all:
        if args.out_dir is None:
            print("--all requires --out-dir", file=sys.stderr)
            return 4
        args.out_dir.mkdir(parents=True, exist_ok=True)

        for scenario, run_dirs in sorted(scenarios.items()):
            try:
                content = compute_for(scenario, run_dirs)
            except RuntimeError as e:
                print(str(e), file=sys.stderr)
                continue

            ext = "csv" if args.csv else "tex"
            out_path = args.out_dir / f"perf.mean.{scenario}.{args.mode}.{ext}"
            out_path.write_text(content, encoding="utf-8")
        return 0

    # single scenario
    scenario = args.scenario
    if scenario not in scenarios:
        known = ", ".join(sorted(scenarios.keys()))
        print(f"Unknown scenario: {scenario}. Known: {known}", file=sys.stderr)
        return 5

    sys.stdout.write(compute_for(scenario, scenarios[scenario]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
