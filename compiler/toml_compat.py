from __future__ import annotations

try:
    import tomllib as _toml_impl
except ModuleNotFoundError:  # pragma: no cover - Python < 3.11 fallback
    try:
        import tomli as _toml_impl
    except ModuleNotFoundError:  # pragma: no cover - local fallback
        _toml_impl = None


def _strip_comment(line: str) -> str:
    out: list[str] = []
    in_string = False
    escape = False
    for ch in line:
        if escape:
            out.append(ch)
            escape = False
            continue
        if ch == "\\" and in_string:
            out.append(ch)
            escape = True
            continue
        if ch == '"':
            out.append(ch)
            in_string = not in_string
            continue
        if ch == "#" and not in_string:
            break
        out.append(ch)
    return "".join(out).strip()


def _parse_string(raw: str) -> str:
    inner = raw[1:-1]
    return (
        inner.replace("\\n", "\n")
        .replace("\\t", "\t")
        .replace('\\"', '"')
        .replace("\\\\", "\\")
    )


def _parse_array(raw: str) -> list:
    inner = raw[1:-1].strip()
    if not inner:
        return []
    items: list[str] = []
    current: list[str] = []
    in_string = False
    escape = False
    for ch in inner:
        if escape:
            current.append(ch)
            escape = False
            continue
        if ch == "\\" and in_string:
            current.append(ch)
            escape = True
            continue
        if ch == '"':
            current.append(ch)
            in_string = not in_string
            continue
        if ch == "," and not in_string:
            items.append("".join(current).strip())
            current = []
            continue
        current.append(ch)
    if current:
        items.append("".join(current).strip())
    return [_parse_value(item) for item in items]


def _parse_value(raw: str):
    value = raw.strip()
    if not value:
        return ""
    if value.startswith('"') and value.endswith('"'):
        return _parse_string(value)
    if value.startswith("[") and value.endswith("]"):
        return _parse_array(value)
    if value == "true":
        return True
    if value == "false":
        return False
    if value.lstrip("-").isdigit():
        try:
            return int(value)
        except ValueError:
            pass
    return value


def _ensure_section(root: dict, path: str) -> dict:
    current = root
    for part in path.split("."):
        current = current.setdefault(part, {})
    return current


def _fallback_loads(text: str) -> dict:
    root: dict = {}
    current = root
    for raw_line in text.splitlines():
        line = _strip_comment(raw_line)
        if not line:
            continue
        if line.startswith("[") and line.endswith("]"):
            section_name = line[1:-1].strip()
            current = _ensure_section(root, section_name)
            continue
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        current[key.strip()] = _parse_value(value)
    return root


def loads(text: str) -> dict:
    if _toml_impl is not None:
        return _toml_impl.loads(text)
    return _fallback_loads(text)
