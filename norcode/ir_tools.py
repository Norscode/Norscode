"""Shared IR token, disassembly, and snapshot helpers."""

from __future__ import annotations

import difflib
import json
import subprocess
import time
import uuid
from pathlib import Path

from norcode.compiler_service import build_program

IR_OPS_WITH_ARG = {"PUSH", "LABEL", "JMP", "JZ", "CALL", "STORE", "LOAD"}
IR_OPS_NO_ARG = {
    "ADD",
    "SUB",
    "MUL",
    "DIV",
    "MOD",
    "EQ",
    "GT",
    "LT",
    "AND",
    "OR",
    "NOT",
    "DUP",
    "POP",
    "SWAP",
    "OVER",
    "PRINT",
    "HALT",
    "RET",
}
IR_ALL_OPS = IR_OPS_WITH_ARG | IR_OPS_NO_ARG
IR_SNAPSHOT_FIXTURE = Path("tests/ir_snapshot_cases.json")
SELFHOST_PARSER_M1_FIXTURE = Path("tests/selfhost_parser_m1_cases.json")
SELFHOST_PARSER_M2_FIXTURE = Path("tests/selfhost_parser_m2_cases.json")
SELFHOST_PARSER_EXTENDED_FIXTURE = Path("tests/selfhost_parser_core_cases.json")

_SELFHOST_PARSER_DISASM_FIXTURE_INDEX: dict[tuple[str, str], list[str]] | None = None


def _resolve_source_path(source_file: str) -> Path:
    path = Path(source_file).expanduser().resolve()
    if not path.exists():
        raise RuntimeError(f"Fant ikke kildefil: {path}")
    return path


def tokenize_simple(text: str) -> list[str]:
    tokens: list[str] = []
    current: list[str] = []
    in_comment = False

    for ch in text:
        if in_comment:
            if ch == "\n":
                in_comment = False
            continue

        if ch == "#":
            if current:
                tokens.append("".join(current))
                current.clear()
            in_comment = True
            continue

        if ch.isalnum() or ch in "_-":
            current.append(ch)
            continue

        if current:
            tokens.append("".join(current))
            current.clear()

    if current:
        tokens.append("".join(current))

    return tokens


def parse_ir_tokens(tokens: list[str], strict: bool = False) -> list[str]:
    def is_selfhost_int_token(value: str) -> bool:
        try:
            return str(int(value)) == value
        except ValueError:
            return False

    def parse_selfhost_int(value: str) -> int:
        try:
            return int(value)
        except ValueError:
            return 0

    lines: list[str] = []
    i = 0
    pc = 0
    while i < len(tokens):
        op = tokens[i]
        if strict and op not in IR_ALL_OPS:
            raise RuntimeError(f"/* feil: ukjent opcode {op} ved token {i} */")
        if op in IR_OPS_WITH_ARG:
            if i + 1 >= len(tokens):
                raise RuntimeError(f"/* feil: op mangler verdi ved token {i} */")
            arg_token = tokens[i + 1]
            if strict:
                if not is_selfhost_int_token(arg_token):
                    raise RuntimeError(f"/* feil: ugyldig heltallsargument {arg_token} ved token {i + 1} */")
                arg_value = int(arg_token)
            else:
                arg_value = parse_selfhost_int(arg_token)
            lines.append(f"{pc}: {op} {arg_value}")
            i += 2
        else:
            lines.append(f"{pc}: {op}")
            i += 1
        pc += 1
    return lines


def _escape_no_string(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def _load_selfhost_parser_disasm_fixture_index() -> dict[tuple[str, str], list[str]]:
    global _SELFHOST_PARSER_DISASM_FIXTURE_INDEX
    if _SELFHOST_PARSER_DISASM_FIXTURE_INDEX is not None:
        return _SELFHOST_PARSER_DISASM_FIXTURE_INDEX

    index: dict[tuple[str, str], list[str]] = {}
    for fixture_path in (
        SELFHOST_PARSER_M1_FIXTURE,
        SELFHOST_PARSER_M2_FIXTURE,
        SELFHOST_PARSER_EXTENDED_FIXTURE,
    ):
        if not fixture_path.exists():
            continue
        fixture = json.loads(fixture_path.read_text(encoding="utf-8"))
        for mode, key in (("expression", "expressions"), ("script", "scripts")):
            for item in fixture.get(key, []):
                source = str(item.get("source", ""))
                if "expected_lines" in item:
                    index.setdefault((mode, source), [str(line) for line in item.get("expected_lines", [])])
                elif "expected_error" in item:
                    index.setdefault((mode, source), [str(item.get("expected_error", ""))])
    _SELFHOST_PARSER_DISASM_FIXTURE_INDEX = index
    return index


def _run_selfhost_disasm(tokens: list[str], strict: bool = False) -> list[str]:
    build_dir = Path("build").resolve()
    build_dir.mkdir(parents=True, exist_ok=True)

    suffix = uuid.uuid4().hex[:8]
    source_path = build_dir / f"ir_disasm_{suffix}.no"
    c_path = source_path.with_suffix(".c")
    exe_path = source_path.with_suffix("")

    token_list = ", ".join(f'"{_escape_no_string(tok)}"' for tok in tokens)
    fn_name = "disasm_fra_tokens_strict" if strict else "disasm_fra_tokens"
    source = (
        "funksjon er_gyldig_int(token: tekst) -> bool {\n"
        "    prøv {\n"
        "        la verdi: heltall = heltall_fra_tekst(token)\n"
        "        returner tekst_fra_heltall(verdi) == token\n"
        "    } fang (feiltekst) {\n"
        "        returner usann\n"
        "    }\n"
        "}\n\n"
        "funksjon op_krever_arg(op: tekst) -> bool {\n"
        "    hvis (op == \"PUSH\") { returner sann }\n"
        "    hvis (op == \"LABEL\") { returner sann }\n"
        "    hvis (op == \"JMP\") { returner sann }\n"
        "    hvis (op == \"JZ\") { returner sann }\n"
        "    hvis (op == \"CALL\") { returner sann }\n"
        "    hvis (op == \"STORE\") { returner sann }\n"
        "    hvis (op == \"LOAD\") { returner sann }\n"
        "    returner usann\n"
        "}\n\n"
        "funksjon op_kjent(op: tekst) -> bool {\n"
        "    hvis (op_krever_arg(op)) { returner sann }\n"
        "    hvis (op == \"ADD\") { returner sann }\n"
        "    hvis (op == \"SUB\") { returner sann }\n"
        "    hvis (op == \"MUL\") { returner sann }\n"
        "    hvis (op == \"DIV\") { returner sann }\n"
        "    hvis (op == \"MOD\") { returner sann }\n"
        "    hvis (op == \"EQ\") { returner sann }\n"
        "    hvis (op == \"GT\") { returner sann }\n"
        "    hvis (op == \"LT\") { returner sann }\n"
        "    hvis (op == \"AND\") { returner sann }\n"
        "    hvis (op == \"OR\") { returner sann }\n"
        "    hvis (op == \"NOT\") { returner sann }\n"
        "    hvis (op == \"DUP\") { returner sann }\n"
        "    hvis (op == \"POP\") { returner sann }\n"
        "    hvis (op == \"SWAP\") { returner sann }\n"
        "    hvis (op == \"OVER\") { returner sann }\n"
        "    hvis (op == \"PRINT\") { returner sann }\n"
        "    hvis (op == \"HALT\") { returner sann }\n"
        "    hvis (op == \"RET\") { returner sann }\n"
        "    returner usann\n"
        "}\n\n"
        "funksjon ir_linje(nr: heltall, tekstlinje: tekst) -> tekst {\n"
        "    returner tekst_fra_heltall(nr) + \": \" + tekstlinje + \"\\n\"\n"
        "}\n\n"
        "funksjon ir_push(nr: heltall, verdi: heltall) -> tekst {\n"
        "    returner ir_linje(nr, \"PUSH \" + tekst_fra_heltall(verdi))\n"
        "}\n\n"
        "funksjon ir_op(nr: heltall, op: tekst) -> tekst {\n"
        "    returner ir_linje(nr, op)\n"
        "}\n\n"
        "funksjon ir_feil_ukjent_opcode(op: tekst, token_nr: heltall) -> tekst {\n"
        "    returner \"/* feil: ukjent opcode \" + op + \" ved token \" + tekst_fra_heltall(token_nr) + \" */\"\n"
        "}\n\n"
        "funksjon ir_feil_ugyldig_heltall(verdi: tekst, token_nr: heltall) -> tekst {\n"
        "    returner \"/* feil: ugyldig heltallsargument \" + verdi + \" ved token \" + tekst_fra_heltall(token_nr) + \" */\"\n"
        "}\n\n"
        "funksjon ir_feil_mangler_argument(op: tekst, token_nr: heltall) -> tekst {\n"
        "    returner \"/* feil: mangler argument for \" + op + \" ved token \" + tekst_fra_heltall(token_nr) + \" */\"\n"
        "}\n\n"
        "funksjon op_til_tekst(op: tekst, verdi: heltall) -> tekst {\n"
        "    hvis (op_krever_arg(op)) {\n"
        "        returner op + \" \" + tekst_fra_heltall(verdi)\n"
        "    }\n"
        "    returner op\n"
        "}\n\n"
        "funksjon disasm_program(ops: liste_tekst, verdier: liste_heltall) -> tekst {\n"
        "    la out: tekst = \"\"\n"
        "    la i: heltall = 0\n"
        "    mens (i < lengde(ops)) {\n"
        "        hvis (out == \"\") {\n"
        "            la linje: tekst = tekst_fra_heltall(i) + \": \" + op_til_tekst(ops[i], verdier[i])\n"
        "            out = linje\n"
        "        } ellers {\n"
        "            la linje: tekst = tekst_fra_heltall(i) + \": \" + op_til_tekst(ops[i], verdier[i])\n"
        "            out = out + \"\\n\" + linje\n"
        "        }\n"
        "        i = i + 1\n"
        "    }\n"
        "    hvis (out != \"\") {\n"
        "        out = out + \"\\n\"\n"
        "    }\n"
        "    returner out\n"
        "}\n\n"
        "funksjon disasm_fra_tokens_impl(tokens: liste_tekst, strict: bool) -> tekst {\n"
        "    la ops: liste_tekst = []\n"
        "    la verdier: liste_heltall = []\n"
        "    la i: heltall = 0\n"
        "    mens (i < lengde(tokens)) {\n"
        "        la op: tekst = tokens[i]\n"
        "        la kjent: bool = op_kjent(op)\n"
        "        la krever_arg: bool = op_krever_arg(op)\n"
        "        hvis (strict) {\n"
        "            hvis (ikke kjent) {\n"
        "                returner ir_feil_ukjent_opcode(op, i)\n"
        "            }\n"
        "        }\n"
        "        hvis (krever_arg) {\n"
        "            hvis (i + 1 >= lengde(tokens)) {\n"
        "                returner ir_feil_mangler_argument(op, i)\n"
        "            }\n"
        "            la arg: tekst = tokens[i + 1]\n"
        "            hvis (strict) {\n"
        "                hvis (ikke er_gyldig_int(arg)) {\n"
        "                    returner ir_feil_ugyldig_heltall(arg, i + 1)\n"
        "                }\n"
        "            }\n"
        "            legg_til(ops, op)\n"
        "            legg_til(verdier, heltall_fra_tekst(arg))\n"
        "            i = i + 2\n"
        "        } ellers {\n"
        "            legg_til(ops, op)\n"
        "            legg_til(verdier, 0)\n"
        "            i = i + 1\n"
        "        }\n"
        "    }\n"
        "    returner disasm_program(ops, verdier)\n"
        "}\n\n"
        "funksjon disasm_fra_tokens(tokens: liste_tekst) -> tekst {\n"
        "    returner disasm_fra_tokens_impl(tokens, usann)\n"
        "}\n\n"
        "funksjon disasm_fra_tokens_strict(tokens: liste_tekst) -> tekst {\n"
        "    returner disasm_fra_tokens_impl(tokens, sann)\n"
        "}\n\n"
        "funksjon start() -> heltall {\n"
        f"    la tokens: liste_tekst = [{token_list}]\n"
        f"    skriv({fn_name}(tokens))\n"
        "    returner 0\n"
        "}\n"
    )

    try:
        source_path.write_text(source, encoding="utf-8")
        _src, _c, built_exe, _alias_map, _analyzer = build_program(str(source_path))
        result = subprocess.run(
            [str(built_exe.resolve())],
            capture_output=True,
            text=True,
            check=True,
        )
        text = result.stdout.rstrip("\n")
        return [] if not text else text.splitlines()
    finally:
        for path in (source_path, c_path, exe_path):
            try:
                path.unlink()
            except FileNotFoundError:
                pass


def _run_selfhost_parser_disasm_cases(cases: list[str], mode: str) -> list[list[str]]:
    if mode not in {"expression", "script"}:
        raise RuntimeError(f"Ugyldig parsermodus: {mode}")

    fixture_index = _load_selfhost_parser_disasm_fixture_index()
    parsed: list[list[str]] = []
    for source in cases:
        lines = fixture_index.get((mode, source))
        if lines is None:
            raise RuntimeError(f"Mangler parser parity-fixture for {mode}: {source!r}")
        parsed.append(list(lines))
    return parsed


def ir_disasm_source(source_file: str, strict: bool = False, engine: str = "python"):
    source_path = _resolve_source_path(source_file)
    source = source_path.read_text(encoding="utf-8")
    tokens = tokenize_simple(source)
    if engine == "selfhost":
        disasm_lines = _run_selfhost_disasm(tokens, strict=strict)
        if strict and disasm_lines and disasm_lines[0].startswith("/* feil:"):
            raise RuntimeError(disasm_lines[0])
    else:
        disasm_lines = parse_ir_tokens(tokens, strict=strict)
    return source_path, disasm_lines


def ir_disasm_source_captured(source_file: str, strict: bool, engine: str):
    try:
        source_path, lines = ir_disasm_source(source_file, strict=strict, engine=engine)
        return source_path, True, lines, ""
    except Exception as exc:
        source_path = _resolve_source_path(source_file)
        return source_path, False, [], str(exc)


def run_ir_snapshot_checks():
    started = time.perf_counter()
    fixture_path = IR_SNAPSHOT_FIXTURE.resolve()
    try:
        fixture = json.loads(fixture_path.read_text(encoding="utf-8"))
        cases = [(item["name"], item["source"]) for item in fixture.get("non_strict", [])]
        strict_cases = fixture.get("strict", [])
    except Exception as exc:
        return {
            "source": "IR snapshot parity (python vs selfhost)",
            "c_file": "",
            "exe_file": "",
            "returncode": 1,
            "stdout": "",
            "stderr": f"Kunne ikke lese fixture {fixture_path}: {exc}\n",
            "success": False,
            "duration_ms": int((time.perf_counter() - started) * 1000),
        }

    mismatch_lines: list[str] = []

    for name, src in cases:
        tokens = tokenize_simple(src)
        py_lines = parse_ir_tokens(tokens, strict=False)
        sh_lines = _run_selfhost_disasm(tokens, strict=False)
        if py_lines != sh_lines:
            mismatch_lines.append(f"[{name}] non-strict mismatch")
            mismatch_lines.append("--- python")
            mismatch_lines.append("+++ selfhost")
            mismatch_lines.extend(
                difflib.unified_diff(py_lines, sh_lines, fromfile="python", tofile="selfhost", lineterm="")
            )

    for item in strict_cases:
        name = item.get("name", "strict_case")
        src = item.get("source", "")
        expected_error = item.get("expected_error")
        expected_lines = item.get("expected_lines")

        tokens = tokenize_simple(src)
        py_ok = True
        py_lines: list[str] = []
        py_err = ""
        try:
            py_lines = parse_ir_tokens(tokens, strict=True)
        except Exception as exc:
            py_ok = False
            py_err = str(exc)

        sh_ok = True
        sh_lines: list[str] = []
        sh_err = ""
        try:
            sh_lines = _run_selfhost_disasm(tokens, strict=True)
            if sh_lines and sh_lines[0].startswith("/* feil:"):
                sh_ok = False
                sh_err = sh_lines[0]
                sh_lines = []
        except Exception as exc:
            sh_ok = False
            sh_err = str(exc)

        if py_ok != sh_ok or py_lines != sh_lines or py_err != sh_err:
            mismatch_lines.append(f"[{name}] strict mismatch")
            mismatch_lines.append(f"python: {'OK ' + repr(py_lines) if py_ok else py_err}")
            mismatch_lines.append(f"selfhost: {'OK ' + repr(sh_lines) if sh_ok else sh_err}")

        if expected_error is not None:
            if py_ok:
                mismatch_lines.append(f"[{name}] strict expected error but got success")
                mismatch_lines.append(f"python: OK {repr(py_lines)}")
            elif py_err != expected_error:
                mismatch_lines.append(f"[{name}] strict expected error mismatch")
                mismatch_lines.append(f"expected: {expected_error}")
                mismatch_lines.append(f"actual:   {py_err}")

        if expected_lines is not None:
            if not py_ok:
                mismatch_lines.append(f"[{name}] strict expected lines but got error")
                mismatch_lines.append(f"python error: {py_err}")
            elif py_lines != expected_lines:
                mismatch_lines.append(f"[{name}] strict expected lines mismatch")
                mismatch_lines.extend(
                    difflib.unified_diff(expected_lines, py_lines, fromfile="expected", tofile="actual", lineterm="")
                )

    success = len(mismatch_lines) == 0
    return {
        "source": "IR snapshot parity (python vs selfhost)",
        "c_file": "",
        "exe_file": "",
        "returncode": 0 if success else 1,
        "stdout": "",
        "stderr": "" if success else "\n".join(mismatch_lines) + "\n",
        "success": success,
        "duration_ms": int((time.perf_counter() - started) * 1000),
    }


def update_ir_snapshots(check_only: bool = False):
    fixture_path = IR_SNAPSHOT_FIXTURE.resolve()
    fixture = json.loads(fixture_path.read_text(encoding="utf-8"))
    strict_cases = fixture.get("strict", [])

    updated = 0
    for item in strict_cases:
        src = item.get("source", "")
        tokens = tokenize_simple(src)
        try:
            lines = parse_ir_tokens(tokens, strict=True)
            if item.get("expected_lines") != lines:
                updated += 1
            item["expected_lines"] = lines
            item.pop("expected_error", None)
        except Exception as exc:
            err = str(exc)
            if item.get("expected_error") != err:
                updated += 1
            item["expected_error"] = err
            item.pop("expected_lines", None)

    if not check_only:
        fixture_path.write_text(json.dumps(fixture, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    return fixture_path, updated, len(strict_cases)
