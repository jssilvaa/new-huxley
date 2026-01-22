#!/usr/bin/env python3

import argparse
import os
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Optional


SUMMARY_KEYS = [
    "total runtime",
    "calls to allocation functions",
    "temporary memory allocations",
    "peak heap memory consumption",
    "peak RSS",
    "total memory leaked",
    "suppressed leaks",
]


@dataclass(frozen=True)
class TempAllocBlock:
    count: int
    total: int
    percent: str
    frames: list[str]

    def touches_repo(self, repo_root: Optional[str]) -> bool:
        if not repo_root:
            return False
        needle = repo_root.rstrip("/") + "/"
        return any(needle in f for f in self.frames)

    def first_repo_location(self, repo_root: Optional[str]) -> Optional[tuple[int, str]]:
        if not repo_root:
            return None
        needle = repo_root.rstrip("/") + "/"
        for idx, frame in enumerate(self.frames):
            if needle in frame:
                return idx, frame
        return None


def repo_frame_score(frame: str, repo_root: Optional[str]) -> int:
    if not repo_root:
        return -1
    needle = repo_root.rstrip("/") + "/"
    if needle not in frame:
        return -1

    # Prefer concrete source locations over build paths.
    is_build = "/build/" in frame
    is_at = frame.startswith("at ")

    if is_build and not is_at:
        return 0

    if is_at:
        if "/src/" in frame:
            return 6
        if "/qml/" in frame or "/mobile/" in frame or "/Utils/" in frame:
            return 6
        if frame.endswith("/main.cpp") or "main.cpp:" in frame or frame.endswith("/Main.qml"):
            return 5
        if not is_build:
            return 4
        return 1

    # Non-"at" repo hit (e.g. "in .../appchat") is less actionable.
    return 2


def _basename_if_path(s: str) -> str:
    s = s.strip()
    if s.startswith("/"):
        return os.path.basename(s)
    return s


def _pretty_loc_path(loc_path: str, repo_root: Optional[str]) -> str:
    loc_path = loc_path.strip()

    # Prefer repo-relative paths when possible.
    if repo_root:
        needle = repo_root.rstrip("/") + "/"
        if loc_path.startswith(needle):
            return loc_path[len(needle):]

    # For non-repo absolute paths, shorten to basename.
    return _basename_if_path(loc_path)


def _pretty_frame_for_output(frame: str, repo_root: Optional[str]) -> str:
    # Keep raw frames containing full paths for matching; only prettify for writing.
    if " in " in frame:
        prefix, loc = frame.split(" in ", 1)
        return f"{prefix} in {_pretty_loc_path(loc, repo_root)}"
    if frame.startswith("at "):
        return "at " + _pretty_loc_path(frame[len("at "):], repo_root)
    if frame.startswith("in "):
        return "in " + _pretty_loc_path(frame[len("in "):], repo_root)
    return frame


def parse_summary(lines: Iterable[str]) -> dict[str, str]:
    summary: dict[str, str] = {}
    for line in lines:
        line = line.strip()
        for key in SUMMARY_KEYS:
            if line.lower().startswith(key + ":"):
                summary[key] = line.split(":", 1)[1].strip()
    return summary


_TEMP_BLOCK_RE = re.compile(
    r"^(?P<count>\d+)\s+temporary allocations\s+of\s+(?P<total>\d+)\s+allocations?\s+in\s+total\s+\((?P<pct>[\d.]+)%\)\s+from:\s*$"
)


def parse_temp_blocks(lines: list[str]) -> list[TempAllocBlock]:
    blocks: list[TempAllocBlock] = []
    i = 0
    while i < len(lines):
        m = _TEMP_BLOCK_RE.match(lines[i].rstrip("\n"))
        if not m:
            i += 1
            continue

        count = int(m.group("count"))
        total = int(m.group("total"))
        pct = m.group("pct")
        i += 1

        frames: list[str] = []
        # Frames come as alternating:
        #     <symbol or address>
        #       in <path>
        while i < len(lines):
            raw = lines[i].rstrip("\n")
            stripped = raw.strip()

            if not stripped:
                i += 1
                break

            if _TEMP_BLOCK_RE.match(raw):
                break

            # End-of-section hints
            if stripped.lower().startswith("total runtime:"):
                break
            if stripped.startswith("and "):
                # e.g. "and 1B from 70 other places"
                i += 1
                continue

            # Try to consume symbol + 'in ...' pair
            if raw.startswith("    ") and not raw.startswith("      "):
                sym = stripped
                loc = ""

                # Lookahead for location line
                if i + 1 < len(lines):
                    nxt = lines[i + 1].rstrip("\n")
                    nxts = nxt.strip()
                    if nxt.startswith("      ") and nxts.startswith("in "):
                        loc_path = nxts[3:].strip()
                        # Keep full path here so repo matching stays reliable.
                        loc = loc_path
                        i += 2
                        frames.append(f"{sym} in {loc}")
                        continue

                i += 1
                frames.append(sym)
                continue

            # Fallback: keep the line
            frames.append(stripped)
            i += 1

        blocks.append(TempAllocBlock(count=count, total=total, percent=pct, frames=frames))

    # The heaptrack output typically already orders these by count, but sort defensively.
    blocks.sort(key=lambda b: b.count, reverse=True)
    return blocks


def latex_escape(s: str) -> str:
    s = s.replace("\\", r"\textbackslash{}")
    s = s.replace("&", r"\&")
    s = s.replace("%", r"\%")
    s = s.replace("$", r"\$")
    s = s.replace("#", r"\#")
    s = s.replace("_", r"\_")
    s = s.replace("{", r"\{")
    s = s.replace("}", r"\}")
    s = s.replace("~", r"\textasciitilde{}")
    s = s.replace("^", r"\textasciicircum{}")
    return s


def best_app_site(block: TempAllocBlock, repo_root: Optional[str]) -> tuple[str, str]:
    """Return (symbol, location) for the most useful repo-owned frame in the stack."""
    best_idx: Optional[int] = None
    best_score = -1
    for idx, frame in enumerate(block.frames):
        score = repo_frame_score(frame, repo_root)
        if score > best_score:
            best_score = score
            best_idx = idx

    if best_idx is None or best_score < 0:
        return "", ""

    idx = best_idx
    loc = block.frames[idx]

    # Heuristic: take the closest preceding non-location line as the symbol.
    symbol = ""
    for j in range(idx - 1, -1, -1):
        cand = block.frames[j].strip()
        if not cand:
            continue
        if cand.startswith("at "):
            continue
        if cand.startswith("in "):
            continue
        if cand.startswith("0x"):
            continue
        symbol = cand
        break

    return symbol, loc.strip()


def write_kpis(out_dir: Path, summary: dict[str, str]) -> Path:
    path = out_dir / "heaptrack.kpis.txt"
    with path.open("w", encoding="utf-8") as f:
        for key in SUMMARY_KEYS:
            if key in summary:
                f.write(f"{key}: {summary[key]}\n")
    return path


def write_temp_top(
    out_dir: Path,
    blocks: list[TempAllocBlock],
    top: int,
    max_frames: int,
    repo_root: Optional[str],
) -> Path:
    path = out_dir / "heaptrack.temp_allocs_top.txt"
    with path.open("w", encoding="utf-8") as f:
        for idx, b in enumerate(blocks[:top], start=1):
            f.write(f"#{idx}: {b.count} temporary allocations (total={b.total}, {b.percent}%)\n")
            for frame in b.frames[:max_frames]:
                f.write(f"  {_pretty_frame_for_output(frame, repo_root)}\n")
            if len(b.frames) > max_frames:
                f.write(f"  ... ({len(b.frames) - max_frames} more frames)\n")
            f.write("\n")
    return path


def write_repo_hits(out_dir: Path, blocks: list[TempAllocBlock], repo_root: Optional[str], max_frames: int) -> Path:
    path = out_dir / "heaptrack.app_hits.txt"
    with path.open("w", encoding="utf-8") as f:
        if not repo_root:
            f.write("repo_root: (not provided)\n")
            f.write("\n")
            return path

        f.write(f"repo_root: {repo_root}\n\n")
        hits = [b for b in blocks if b.touches_repo(repo_root)]
        if not hits:
            f.write("No temporary-allocation stacks referenced repo paths.\n")
            return path

        for idx, b in enumerate(hits, start=1):
            f.write(f"#{idx}: {b.count} temporary allocations (total={b.total}, {b.percent}%)\n")
            for frame in b.frames[:max_frames]:
                f.write(f"  {_pretty_frame_for_output(frame, repo_root)}\n")
            if len(b.frames) > max_frames:
                f.write(f"  ... ({len(b.frames) - max_frames} more frames)\n")
            f.write("\n")

    return path


def write_latex_kpis(out_dir: Path, summary: dict[str, str], caption: str, label: str) -> Path:
    path = out_dir / "heaptrack.kpis.tex"
    with path.open("w", encoding="utf-8") as f:
        f.write("\\begin{table}[h]\n")
        f.write("\\centering\n")
        f.write("\\begin{tabular}{ll}\\hline\n")
        f.write("Metric & Value \\\\ \\hline\n")
        for key in SUMMARY_KEYS:
            if key in summary:
                f.write(f"{latex_escape(key)} & {latex_escape(summary[key])} \\\\ \n")
        f.write("\\hline\\end{tabular}\n")
        if caption:
            f.write(f"\\caption{{{latex_escape(caption)}}}\n")
        if label:
            f.write(f"\\label{{{latex_escape(label)}}}\n")
        f.write("\\end{table}\n")
    return path


def write_latex_app_hits(
    out_dir: Path,
    blocks: list[TempAllocBlock],
    repo_root: Optional[str],
    top: int,
    caption: str,
    label: str,
) -> Path:
    path = out_dir / "heaptrack.app_hits.tex"
    hits = [b for b in blocks if b.touches_repo(repo_root)]
    hits.sort(key=lambda b: b.count, reverse=True)

    with path.open("w", encoding="utf-8") as f:
        f.write("\\begin{table}[h]\n")
        f.write("\\centering\n")
        f.write("\\begin{tabular}{rll}\\hline\n")
        f.write("Temp allocs & Site & Location \\\\ \\hline\n")

        for b in hits[:top]:
            sym, loc = best_app_site(b, repo_root)
            if not sym:
                sym = "(unknown)"
            if not loc:
                loc = "(unknown)"
            loc = _pretty_frame_for_output(loc, repo_root)
            f.write(
                f"{b.count} & {latex_escape(sym)} & {latex_escape(loc)} \\\\ \n"
            )

        f.write("\\hline\\end{tabular}\n")
        if caption:
            f.write(f"\\caption{{{latex_escape(caption)}}}\n")
        if label:
            f.write(f"\\label{{{latex_escape(label)}}}\n")
        f.write("\\end{table}\n")

    return path


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Summarize heaptrack text output (heaptrack --analyze or heaptrack_print) into KPI + top temp alloc stacks."  # noqa: E501
    )
    ap.add_argument("input", help="Path to heaptrack analysis text output")
    ap.add_argument("--out-dir", default=".", help="Directory to write artifacts")
    ap.add_argument("--repo-root", default=None, help="Repo root path for highlighting")
    ap.add_argument("--top", type=int, default=20, help="How many temp allocation blocks to include")
    ap.add_argument("--stack", type=int, default=18, help="How many frames per stack to include")
    ap.add_argument("--latex", action="store_true", help="Also write LaTeX tables (.tex) for KPIs and app hits")
    ap.add_argument("--latex-top", type=int, default=10, help="How many app-hit rows to include in LaTeX table")
    ap.add_argument("--latex-caption", default="Heaptrack summary", help="LaTeX table caption")
    ap.add_argument("--latex-label", default="", help="LaTeX table label")

    args = ap.parse_args()

    in_path = Path(args.input)
    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    text = in_path.read_text(encoding="utf-8", errors="replace")
    lines = text.splitlines(keepends=True)

    summary = parse_summary(line.rstrip("\n") for line in lines)
    blocks = parse_temp_blocks([line.rstrip("\n") for line in lines])

    write_kpis(out_dir, summary)
    write_temp_top(out_dir, blocks, top=args.top, max_frames=args.stack, repo_root=args.repo_root)
    write_repo_hits(out_dir, blocks, repo_root=args.repo_root, max_frames=args.stack)

    if args.latex:
        write_latex_kpis(
            out_dir,
            summary,
            caption=args.latex_caption,
            label=(args.latex_label + "-kpis") if args.latex_label else "",
        )
        write_latex_app_hits(
            out_dir,
            blocks,
            repo_root=args.repo_root,
            top=args.latex_top,
            caption=args.latex_caption,
            label=(args.latex_label + "-apphits") if args.latex_label else "",
        )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
