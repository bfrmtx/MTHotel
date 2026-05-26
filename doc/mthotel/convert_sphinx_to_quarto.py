from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parent
SOURCE_ROOT = ROOT / "source"
DEST_ROOT = ROOT / "qmd"
SKIP_DIRS = {"sqltables", "_templates"}
CALLOUT_TYPES = {
    "warning": "warning",
    "note": "note",
    "important": "important",
    "tip": "tip",
    "hint": "tip",
    "attention": "caution",
    "admonition": "note",
}
DIRECTIVE_RE = re.compile(r"^```\{([^}]+)\}\s*(.*)$")
H1_RE = re.compile(r"^\s*#\s+(.*?)\s*$")
LINK_RE = re.compile(r"(\[[^\]]*\]\()([^()\s]+?\.md)(#[^)]+)?(\))")


def split_csv_row(line: str, delimiter: str) -> list[str]:
    return [part.strip().strip('"') for part in line.split(delimiter)]


def iter_source_files() -> list[Path]:
    files: list[Path] = []
    for path in SOURCE_ROOT.rglob("*.md"):
        relative = path.relative_to(SOURCE_ROOT)
        if any(part in SKIP_DIRS for part in relative.parts):
            continue
        files.append(path)
    return sorted(files)


def extract_title(text: str, fallback: str) -> str:
    for line in text.splitlines():
        match = H1_RE.match(line)
        if match:
            return match.group(1).strip()
    return fallback.replace("_", " ").strip() or "Untitled"


def build_title_map(files: list[Path]) -> dict[str, str]:
    title_map: dict[str, str] = {}
    for path in files:
        relative = path.relative_to(SOURCE_ROOT)
        stem = relative.with_suffix("").as_posix()
        title_map[stem] = extract_title(path.read_text(encoding="utf-8"), path.stem)
    return title_map


def trim_blank_edges(lines: list[str]) -> list[str]:
    start = 0
    end = len(lines)
    while start < end and not lines[start].strip():
        start += 1
    while end > start and not lines[end - 1].strip():
        end -= 1
    return lines[start:end]


def clean_inline_title(text: str) -> str:
    cleaned = re.sub(r"<br\s*/?>", " ", text, flags=re.IGNORECASE)
    cleaned = cleaned.replace("**", "").replace("*", "").replace("`", "")
    cleaned = re.sub(r"\s+", " ", cleaned)
    return cleaned.strip().strip('"')


def quote_yaml(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def slugify(value: str) -> str:
    slug = re.sub(r"[^A-Za-z0-9_-]+", "-", value.strip())
    slug = re.sub(r"-+", "-", slug).strip("-")
    return slug or "figure"


def convert_links(text: str) -> str:
    def repl(match: re.Match[str]) -> str:
        prefix, target, anchor, suffix = match.groups()
        if target.startswith(("http://", "https://", "mailto:")):
            return match.group(0)
        return f"{prefix}{target[:-3]}.qmd{anchor or ''}{suffix}"

    return LINK_RE.sub(repl, text)


def normalize_rst_inline_roles(text: str) -> str:
    text = re.sub(r":math:`([^`]+)`", lambda m: f"${m.group(1)}$", text)
    text = re.sub(r":sub:`([^`]+)`", r"~\1~", text)
    text = re.sub(r":sup:`([^`]+)`", r"^\1^", text)
    # RST often escapes spaces before role usage (for example, "f\ :sub:`s`").
    text = text.replace("\\ ~", "~").replace("\\ ^", "^").replace("\\ $", "$")
    return text


def parse_fenced_block(lines: list[str], start: int) -> tuple[str, str, list[str], int]:
    match = DIRECTIVE_RE.match(lines[start])
    if not match:
        raise ValueError(f"Expected directive at line {start + 1}")

    name = match.group(1).strip()
    rest = match.group(2).strip()
    body: list[str] = []
    index = start + 1
    while index < len(lines):
        if lines[index].strip() == "```":
            return name, rest, body, index + 1
        body.append(lines[index])
        index += 1
    raise ValueError(f"Unterminated fenced directive starting at line {start + 1}")


def convert_toctree(body: list[str], title_map: dict[str, str]) -> list[str]:
    entries = [line.strip() for line in body if line.strip() and not line.lstrip().startswith(":")]
    output = ["## Contents", ""]
    for entry in entries:
        target = f"{entry}.qmd"
        text = title_map.get(entry, Path(entry).name.replace("_", " "))
        output.append(f"- [{text}]({target})")
    output.append("")
    return output


def convert_figure(rest: str, body: list[str]) -> list[str]:
    path = rest
    options: dict[str, str] = {}
    caption_lines: list[str] = []
    in_caption = False
    for line in body:
        stripped = line.strip()
        if not in_caption and stripped.startswith(":"):
            key, _, value = stripped[1:].partition(":")
            options[key.strip()] = value.strip()
            continue
        if not stripped and not in_caption:
            in_caption = True
            continue
        in_caption = True
        caption_lines.append(line.rstrip())

    caption = " ".join(line.strip() for line in caption_lines if line.strip())
    alt = options.get("alt", caption)
    attrs: list[str] = []
    if options.get("name"):
        attrs.append(f"#{slugify(options['name'])}")
    if options.get("align"):
        attrs.append(f"fig-align=\"{options['align']}\"")
    if options.get("width"):
        attrs.append(f"width=\"{options['width']}\"")
    if alt:
        attrs.append(f"fig-alt=\"{alt.replace('\\', '\\\\').replace('"', '\\"')}\"")

    attr_text = f"{{{' '.join(attrs)}}}" if attrs else ""
    figure_caption = caption or alt or "Figure"
    return [f"![{figure_caption}]({path}){attr_text}", ""]


def convert_callout(name: str, rest: str, body: list[str]) -> list[str]:
    callout_type = CALLOUT_TYPES.get(name, "note")
    title = clean_inline_title(rest)
    attrs = [f".callout-{callout_type}"]
    if title:
        attrs.append(f'title="{title.replace("\\", "\\\\").replace("\"", "\\\"")}"')

    output = [f"::: {{{' '.join(attrs)}}}"]
    output.extend(trim_blank_edges(body))
    output.extend([":::", ""])
    return output


def convert_table(rest: str, body: list[str]) -> list[str]:
    caption = clean_inline_title(rest)
    table_lines: list[str] = []
    body_started = False
    for line in body:
        stripped = line.strip()
        if not body_started and stripped.startswith(":"):
            continue
        if not body_started and not stripped:
            body_started = True
            continue
        body_started = True
        table_lines.append(line.rstrip())

    output = trim_blank_edges(table_lines)
    if caption:
        output.append(f": {caption}")
    output.append("")
    return output


def convert_eval_rst(body: list[str]) -> list[str]:
    lines = trim_blank_edges(body)
    if not lines:
        return []

    first = lines[0].strip()
    if first != ".. csv-table::":
        return lines + [""]

    header: list[str] = []
    delimiter = ","
    rows: list[list[str]] = []

    for raw_line in lines[1:]:
        stripped = raw_line.strip()
        if not stripped:
            continue
        if stripped.startswith(":header:"):
            header = split_csv_row(stripped.partition(":header:")[2].strip(), ";")
            continue
        if stripped.startswith(":delim:"):
            delimiter = stripped.partition(":delim:")[2].strip() or ","
            continue
        if stripped.startswith(":"):
            continue
        rows.append(split_csv_row(stripped, delimiter))

    if not rows and not header:
        return []

    column_count = max(len(header), *(len(row) for row in rows), 0)
    if column_count == 0:
        return []

    if not header:
        header = [f"col{i + 1}" for i in range(column_count)]
    else:
        header.extend([""] * (column_count - len(header)))

    normalized_rows: list[list[str]] = []
    for row in rows:
        normalized = row + [""] * (column_count - len(row))
        normalized_rows.append(normalized)

    output = [
        f"| {' | '.join(header)} |",
        f"| {' | '.join(['---'] * column_count)} |",
    ]
    for row in normalized_rows:
        output.append(f"| {' | '.join(row)} |")
    output.append("")
    return output


def convert_include(rest: str) -> list[str]:
    target = rest.strip()
    target = re.sub(r"(^|/)sqltables(/)", r"\1_sqltables\2", target)
    return [f"{{{{< include {target} >}}}}", ""]


def strip_sphinx_index_section(lines: list[str]) -> list[str]:
    output: list[str] = []
    index = 0
    while index < len(lines):
        if lines[index].strip() in {"# Indices and tables", "## Indices and tables"}:
            probe = index + 1
            while probe < len(lines) and (not lines[probe].strip() or "{ref}`" in lines[probe]):
                probe += 1
            if probe == len(lines):
                break
            if all(not line.strip() or "{ref}`" in line for line in lines[index + 1 : probe]):
                index = probe
                continue
        if "{ref}`" not in lines[index]:
            output.append(lines[index])
        index += 1
    return output


def transform_content(text: str, title_map: dict[str, str]) -> tuple[str, list[str]]:
    lines = text.splitlines()
    output: list[str] = []
    toctree_entries: list[str] = []
    index = 0
    removed_title = False

    while index < len(lines):
        line = lines[index]

        if not removed_title:
            if not line.strip():
                index += 1
                continue
            if H1_RE.match(line):
                removed_title = True
                index += 1
                continue
            removed_title = True

        match = DIRECTIVE_RE.match(line)
        if not match:
            output.append(line)
            index += 1
            continue

        name, rest, body, next_index = parse_fenced_block(lines, index)
        index = next_index

        if name == "toctree":
            toctree_entries = [item.strip() for item in body if item.strip() and not item.lstrip().startswith(":")]
            output.extend(convert_toctree(body, title_map))
            continue
        if name == "figure":
            output.extend(convert_figure(rest, body))
            continue
        if name == "include":
            output.extend(convert_include(rest))
            continue
        if name == "index":
            continue
        if name == "table":
            output.extend(convert_table(rest, body))
            continue
        if name == "eval-rst":
            output.extend(convert_eval_rst(body))
            continue
        if name in CALLOUT_TYPES:
            output.extend(convert_callout(name, rest, body))
            continue

        output.append(line)
        output.extend(body)
        output.append("```")

    cleaned_lines = strip_sphinx_index_section(trim_blank_edges(output))
    content = "\n".join(trim_blank_edges(cleaned_lines)).rstrip() + "\n"
    content = normalize_rst_inline_roles(content)
    return convert_links(content), toctree_entries


def write_qmd(relative: Path, title: str, content: str) -> None:
    destination = DEST_ROOT / relative.with_suffix(".qmd")
    destination.parent.mkdir(parents=True, exist_ok=True)
    front_matter = f"---\ntitle: \"{quote_yaml(title)}\"\n---\n\n"
    destination.write_text(front_matter + content, encoding="utf-8")


def write_quarto_config(site_title: str, toctree_entries: list[str], title_map: dict[str, str], render_paths: list[str]) -> None:
    sidebar_lines = [
        "project:",
        "  type: website",
        "  output-dir: ../html",
        "  render:",
    ]

    for render_path in render_paths:
        sidebar_lines.append(f"    - {render_path}")

    sidebar_lines.extend([
        "  resources:",
        "    - media/**",
        "    - _static/**",
        "",
        "website:",
        f"  title: \"{quote_yaml(site_title)}\"",
        "  sidebar:",
        "    style: docked",
        "    search: true",
        "    contents:",
        "      - href: index.qmd",
        f"        text: \"{quote_yaml(site_title)}\"",
    ])

    if toctree_entries:
        sidebar_lines.extend([
            "      - section: \"Contents\"",
            "        contents:",
        ])
        for entry in toctree_entries:
            sidebar_lines.extend([
                f"          - href: {entry}.qmd",
                f"            text: \"{quote_yaml(title_map.get(entry, Path(entry).name.replace('_', ' ')))}\"",
            ])

    sidebar_lines.extend([
        "",
        "format:",
        "  html:",
        "    toc: true",
    ])

    (DEST_ROOT / "_quarto.yml").write_text("\n".join(sidebar_lines) + "\n", encoding="utf-8")


def ensure_static_symlink() -> None:
    link_path = DEST_ROOT / "_static"
    if link_path.exists() or link_path.is_symlink():
        return
    link_path.symlink_to(Path("../source/_static"))


def main() -> None:
    files = iter_source_files()
    title_map = build_title_map(files)
    index_entries: list[str] = []
    site_title = title_map.get("index", "MTHotel / MTH5")
    render_paths = [path.relative_to(SOURCE_ROOT).with_suffix(".qmd").as_posix() for path in files]

    for source_file in files:
        relative = source_file.relative_to(SOURCE_ROOT)
        stem = relative.with_suffix("").as_posix()
        title = title_map[stem]
        content, toctree_entries = transform_content(source_file.read_text(encoding="utf-8"), title_map)
        write_qmd(relative, title, content)
        if stem == "index":
            index_entries = toctree_entries

    write_quarto_config(site_title, index_entries, title_map, render_paths)
    ensure_static_symlink()


if __name__ == "__main__":
    main()
