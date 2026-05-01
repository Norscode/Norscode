from pathlib import Path
import copy

from .ast_nodes import ProgramNode
from .lexer import Lexer
from .parser import Parser
from .toml_compat import loads as toml_loads


_PARSE_CACHE_BY_PATH = {}
PROJECT_CONFIG_NAMES = ("norcode.toml", "norsklang.toml")


class ModuleLoader:
    def __init__(self, root):
        self.root = Path(root).resolve()
        self.loaded = {}
        self.loading = set()
        self.project_root = self._find_project_root()
        self.dependency_map = self._load_dependencies()

    def _find_existing_config_in_dir(self, base: Path):
        for name in PROJECT_CONFIG_NAMES:
            config_path = base / name
            if config_path.exists():
                return config_path
        return None

    def _find_project_root(self):
        for base in (self.root, *self.root.parents):
            config_path = self._find_existing_config_in_dir(base)
            if config_path is not None:
                return base
        return None

    def _load_dependencies(self):
        if self.project_root is None:
            return {}

        config_path = self._find_existing_config_in_dir(self.project_root)
        if config_path is None:
            return {}
        try:
            data = toml_loads(config_path.read_text(encoding="utf-8"))
        except Exception:
            return {}

        deps = data.get("dependencies", {})
        if not isinstance(deps, dict):
            return {}

        dep_map = {}
        for dep_name, raw_value in deps.items():
            if not isinstance(dep_name, str) or not isinstance(raw_value, str):
                continue
            if raw_value.startswith("git+") or raw_value.startswith("url+"):
                continue

            dep_path = Path(raw_value).expanduser()
            if not dep_path.is_absolute():
                dep_path = (self.project_root / dep_path).resolve()
            if not dep_path.exists() or not dep_path.is_dir():
                continue

            entry_file = None
            dep_config = self._find_existing_config_in_dir(dep_path)
            if dep_config is not None and dep_config.exists():
                try:
                    dep_data = toml_loads(dep_config.read_text(encoding="utf-8"))
                    project = dep_data.get("project", {})
                    if isinstance(project, dict):
                        entry = project.get("entry")
                        if isinstance(entry, str):
                            entry_file = entry
                except Exception:
                    entry_file = None

            dep_map[dep_name] = {"root": dep_path, "entry": entry_file}

        return dep_map

    def parse_file(self, file_path: Path):
        resolved = file_path.resolve()
        stat = resolved.stat()
        mtime_ns = stat.st_mtime_ns
        size = stat.st_size
        cache_key = str(resolved)

        cached = _PARSE_CACHE_BY_PATH.get(cache_key)
        if cached and cached["mtime_ns"] == mtime_ns and cached["size"] == size:
            # Returnerer kopi for å unngå at videre mutasjoner påvirker cache.
            return copy.deepcopy(cached["program"])

        source = resolved.read_text(encoding="utf-8")
        lexer = Lexer(source)
        parser = Parser(lexer)
        program = parser.parse()

        _PARSE_CACHE_BY_PATH[cache_key] = {
            "mtime_ns": mtime_ns,
            "size": size,
            "program": copy.deepcopy(program),
        }
        return program

    def _set_module_name(self, program, module_name):
        for fn in getattr(program, "functions", []):
            fn.module_name = module_name

    def _merge_programs(self, programs):
        all_functions = []
        for program in programs:
            all_functions.extend(getattr(program, "functions", []))
        return ProgramNode([], all_functions)

    def _module_candidates(self, module_name: str):
        rel_path = Path(*module_name.split(".")).with_suffix(".no")
        dot_path = Path(f"{module_name}.no")

        candidates = []
        seen = set()
        for base in (self.root, *self.root.parents):
            for path in (base / dot_path, base / rel_path):
                resolved = path.resolve()
                if resolved in seen:
                    continue
                seen.add(resolved)
                candidates.append(path)

        for dep_name, meta in self.dependency_map.items():
            dep_root = meta["root"]
            dep_entry = meta.get("entry")

            if module_name == dep_name and dep_entry:
                entry_path = dep_root / dep_entry
                resolved = entry_path.resolve()
                if resolved not in seen:
                    seen.add(resolved)
                    candidates.append(entry_path)

            if module_name.startswith(dep_name + "."):
                inner_name = module_name[len(dep_name) + 1 :]
                inner_rel = Path(*inner_name.split(".")).with_suffix(".no")
                inner_dot = Path(f"{inner_name}.no")
                for path in (dep_root / inner_dot, dep_root / inner_rel):
                    resolved = path.resolve()
                    if resolved in seen:
                        continue
                    seen.add(resolved)
                    candidates.append(path)

            for path in (dep_root / dot_path, dep_root / rel_path):
                resolved = path.resolve()
                if resolved in seen:
                    continue
                seen.add(resolved)
                candidates.append(path)
        return candidates

    def load_module(self, module_name: str):
        if module_name in self.loaded:
            return self.loaded[module_name]

        if module_name in self.loading:
            raise RuntimeError(f"Sirkulær import oppdaget: {module_name}")

        self.loading.add(module_name)
        try:
            candidates = self._module_candidates(module_name)

            file_path = None
            for candidate in candidates:
                if candidate.exists():
                    file_path = candidate
                    break

            if file_path is None:
                searched = "\n".join(str(p) for p in candidates)
                raise RuntimeError(f"Fant ikke modulfil for '{module_name}'. Søkte i:\n{searched}")

            program = self.parse_file(file_path)
            self._set_module_name(program, module_name)

            imported_programs = []
            for imp in getattr(program, "imports", []):
                imported_programs.append(self.load_module(imp.module_name))

            merged = self._merge_programs([*imported_programs, program])
            self.loaded[module_name] = merged
            return merged
        finally:
            self.loading.discard(module_name)

    def load_entry_file(self, name):
        entry_path = self.root / name
        if not entry_path.exists():
            raise RuntimeError(f"Fant ikke inngangsfil: {entry_path}")

        program = self.parse_file(entry_path)
        self._set_module_name(program, "__main__")

        alias_map = {}
        imported_programs = []

        for imp in getattr(program, "imports", []):
            imported_programs.append(self.load_module(imp.module_name))
            alias_name = getattr(imp, "alias", None) or imp.module_name.split(".")[-1]
            if alias_name in alias_map:
                raise RuntimeError(f"Alias brukt flere ganger: {alias_name}")
            alias_map[alias_name] = imp.module_name

        merged = self._merge_programs([*imported_programs, program])
        return merged, alias_map
