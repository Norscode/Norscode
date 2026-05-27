"""Modular inspection commands."""

from __future__ import annotations

import difflib
import json
from pathlib import Path

from compiler.lexer import Lexer
from compiler.parser import Parser
from compiler.semantic import SemanticAnalyzer

from norcode.bootstrap_support import _format_cli_exception
from norcode.commands.base import CommandModule
from norcode.compiler_service import disasm_program
from norcode.ir_tools import ir_disasm_source, ir_disasm_source_captured, update_ir_snapshots


def _serialize_value(value):
    if value is None or isinstance(value, (str, int, float, bool)):
        return value
    if isinstance(value, list):
        return [_serialize_value(item) for item in value]
    if isinstance(value, tuple):
        return [_serialize_value(item) for item in value]
    if isinstance(value, dict):
        return {str(key): _serialize_value(item) for key, item in value.items()}
    if hasattr(value, "__dict__"):
        payload = {"type": value.__class__.__name__}
        for key, item in value.__dict__.items():
            payload[key] = _serialize_value(item)
        return payload
    return repr(value)


def _serialize_token(token):
    return {
        "line": token.line,
        "column": token.column,
        "type": token.typ,
        "value": token.value,
    }


def _analyze_source(path: Path):
    text = path.read_text(encoding="utf-8")
    lexer = Lexer(text)
    tokens = []
    while True:
        token = lexer.next_token()
        if token.typ == "EOF":
            break
        tokens.append(token)

    program = Parser(Lexer(text)).parse()
    alias_map = {
        item.alias: item.module_name
        for item in getattr(program, "imports", [])
        if getattr(item, "alias", None)
    }
    analyzer = SemanticAnalyzer(alias_map=alias_map)
    analyzer.analyze(program)
    return program, analyzer, tokens


def register_disasm_arguments(parser) -> None:
    parser.add_argument("file")


def run_disasm(args) -> int:
    try:
        source_path = Path(args.file).expanduser().resolve()
        code_path, code = disasm_program(args.file)
        print(f"Kilde: {code_path}")
        print("Generert C:")
        print(code)
        return 0
    except Exception as exc:
        print(f"Feil: {_format_cli_exception(exc)}")
        return 1


def register_ir_disasm_arguments(parser) -> None:
    parser.add_argument("file")
    parser.add_argument("--json", action="store_true", help="Skriv output som JSON")
    parser.add_argument("--strict", action="store_true", help="Feil ved ukjente opcodes/ugyldige argumenter")
    parser.add_argument("--engine", choices=["python", "selfhost"], default="python", help="Velg disasm-motor")
    parser.add_argument("--diff", action="store_true", help="Sammenlign python og selfhost disasm")
    parser.add_argument("--fail-on-warning", action="store_true", help="Feil hvis strict-resultat avviker mellom motorene")
    parser.add_argument("--save-diff", help="Lagre diff-output til fil ved --diff")


def run_ir_disasm(args) -> int:
    try:
        if args.diff:
            source_path, py_ok, py_lines, py_err = ir_disasm_source_captured(args.file, strict=args.strict, engine="python")
            _source_path2, sh_ok, sh_lines, sh_err = ir_disasm_source_captured(args.file, strict=args.strict, engine="selfhost")

            if py_ok != sh_ok:
                if args.json:
                    payload = {
                        "source": str(source_path),
                        "strict": args.strict,
                        "match": False,
                        "python_ok": py_ok,
                        "python_error": py_err,
                        "selfhost_ok": sh_ok,
                        "selfhost_error": sh_err,
                    }
                    if args.save_diff:
                        diff_text = (
                            "MISMATCH (ulik feilstatus)\n"
                            f"python: {'OK' if py_ok else py_err}\n"
                            f"selfhost: {'OK' if sh_ok else sh_err}\n"
                        )
                        save_path = Path(args.save_diff).expanduser()
                        save_path.write_text(diff_text, encoding="utf-8")
                        payload["saved_diff"] = str(save_path.resolve())
                    print(json.dumps(payload, ensure_ascii=False, indent=2))
                else:
                    print(f"Kilde: {source_path}")
                    print("Motor: diff (python vs selfhost)")
                    print("IR disasm: MISMATCH (ulik feilstatus)")
                    print(f"python: {'OK' if py_ok else py_err}")
                    print(f"selfhost: {'OK' if sh_ok else sh_err}")
                    if args.save_diff:
                        diff_text = (
                            "MISMATCH (ulik feilstatus)\n"
                            f"python: {'OK' if py_ok else py_err}\n"
                            f"selfhost: {'OK' if sh_ok else sh_err}\n"
                        )
                        save_path = Path(args.save_diff).expanduser()
                        save_path.write_text(diff_text, encoding="utf-8")
                        print(f"Diff lagret: {save_path.resolve()}")
                return 1

            if not py_ok and not sh_ok:
                if py_err != sh_err:
                    if args.json:
                        payload = {
                            "source": str(source_path),
                            "strict": args.strict,
                            "match": False,
                            "python_error": py_err,
                            "selfhost_error": sh_err,
                        }
                        if args.save_diff:
                            diff_text = (
                                "MISMATCH (ulik feilmelding)\n"
                                f"python: {py_err}\n"
                                f"selfhost: {sh_err}\n"
                            )
                            save_path = Path(args.save_diff).expanduser()
                            save_path.write_text(diff_text, encoding="utf-8")
                            payload["saved_diff"] = str(save_path.resolve())
                        print(json.dumps(payload, ensure_ascii=False, indent=2))
                    else:
                        print(f"Kilde: {source_path}")
                        print("Motor: diff (python vs selfhost)")
                        print("IR disasm: MISMATCH (ulik feilmelding)")
                        print(f"python: {py_err}")
                        print(f"selfhost: {sh_err}")
                        if args.save_diff:
                            diff_text = (
                                "MISMATCH (ulik feilmelding)\n"
                                f"python: {py_err}\n"
                                f"selfhost: {sh_err}\n"
                            )
                            save_path = Path(args.save_diff).expanduser()
                            save_path.write_text(diff_text, encoding="utf-8")
                            print(f"Diff lagret: {save_path.resolve()}")
                    return 1
                raise RuntimeError(py_err)

            if args.json:
                payload = {
                    "source": str(source_path),
                    "strict": args.strict,
                    "match": py_lines == sh_lines,
                    "python_lines": py_lines,
                    "selfhost_lines": sh_lines,
                }
                if args.fail_on_warning:
                    _src_w1, py_strict_ok, py_strict_lines, py_strict_err = ir_disasm_source_captured(args.file, strict=True, engine="python")
                    _src_w2, sh_strict_ok, sh_strict_lines, sh_strict_err = ir_disasm_source_captured(args.file, strict=True, engine="selfhost")
                    payload["strict_warning_match"] = (
                        py_strict_ok == sh_strict_ok
                        and py_strict_lines == sh_strict_lines
                        and py_strict_err == sh_strict_err
                    )
                    payload["python_strict_ok"] = py_strict_ok
                    payload["python_strict_error"] = py_strict_err
                    payload["selfhost_strict_ok"] = sh_strict_ok
                    payload["selfhost_strict_error"] = sh_strict_err
                print(json.dumps(payload, ensure_ascii=False, indent=2))
            else:
                print(f"Kilde: {source_path}")
                print("Motor: diff (python vs selfhost)")
                if py_lines == sh_lines:
                    print("IR disasm: MATCH")
                    for line in py_lines:
                        print(line)
                else:
                    print("IR disasm: MISMATCH")
                    diff_lines = list(
                        difflib.unified_diff(
                            py_lines,
                            sh_lines,
                            fromfile="python",
                            tofile="selfhost",
                            lineterm="",
                        )
                    )
                    for line in diff_lines:
                        print(line)
                    if args.save_diff:
                        save_path = Path(args.save_diff).expanduser()
                        save_path.write_text("\n".join(diff_lines) + "\n", encoding="utf-8")
                        print(f"Diff lagret: {save_path.resolve()}")
                    return 1

                if args.fail_on_warning:
                    _src_w1, py_strict_ok, py_strict_lines, py_strict_err = ir_disasm_source_captured(args.file, strict=True, engine="python")
                    _src_w2, sh_strict_ok, sh_strict_lines, sh_strict_err = ir_disasm_source_captured(args.file, strict=True, engine="selfhost")
                    warning_match = (
                        py_strict_ok == sh_strict_ok
                        and py_strict_lines == sh_strict_lines
                        and py_strict_err == sh_strict_err
                    )
                    if warning_match:
                        print("Warning check: MATCH")
                    else:
                        print("Warning check: MISMATCH")
                        print(f"python strict: {'OK' if py_strict_ok else py_strict_err}")
                        print(f"selfhost strict: {'OK' if sh_strict_ok else sh_strict_err}")
                        if args.save_diff:
                            diff_text = (
                                "Warning check mismatch\n"
                                f"python strict: {'OK' if py_strict_ok else py_strict_err}\n"
                                f"selfhost strict: {'OK' if sh_strict_ok else sh_strict_err}\n"
                            )
                            save_path = Path(args.save_diff).expanduser()
                            save_path.write_text(diff_text, encoding="utf-8")
                            print(f"Diff lagret: {save_path.resolve()}")
                        return 1
        else:
            source_path, lines = ir_disasm_source(args.file, strict=args.strict, engine=args.engine)
            if args.json:
                payload = {
                    "source": str(source_path),
                    "engine": args.engine,
                    "lines": lines,
                }
                print(json.dumps(payload, ensure_ascii=False, indent=2))
            else:
                print(f"Kilde: {source_path}")
                print(f"Motor: {args.engine}")
                print("IR disasm:")
                for line in lines:
                    print(line)
        return 0
    except Exception as exc:
        print(f"Feil: {_format_cli_exception(exc)}")
        return 1


def register_update_snapshots_arguments(parser) -> None:
    parser.add_argument("--check", action="store_true", help="Feil hvis snapshots er utdaterte (skriv ikke)")
    parser.add_argument("--json", action="store_true", help="Skriv resultat som JSON")


def run_update_snapshots(args) -> int:
    try:
        fixture_path, updated, total = update_ir_snapshots(check_only=args.check)
        payload = {
            "fixture": str(Path(fixture_path).resolve()),
            "check_only": bool(args.check),
            "updated": int(updated),
            "strict_cases": int(total),
        }
        if args.json:
            print(json.dumps(payload, ensure_ascii=False, indent=2))
        else:
            print(f"Oppdatert snapshot-fixture: {fixture_path}")
            print(f"Strict-cases: {total}")
            if args.check:
                print(f"Avvik funnet: {updated}")
            else:
                print(f"Endringer skrevet: {updated}")
        if args.check and updated > 0:
            return 1
        return 0
    except Exception as exc:
        print(f"Feil: {_format_cli_exception(exc)}")
        return 1


DEBUG_COMMAND = CommandModule(
    name="debug",
    help="Vis debug-info (tokens/AST/symboler) for en .no-fil",
    register_arguments=lambda parser: (
        parser.add_argument("file"),
        parser.add_argument("--tokens", action="store_true", help="Vis lexer-tokens"),
        parser.add_argument("--ast", action="store_true", help="Vis AST"),
        parser.add_argument("--symbols", action="store_true", help="Vis semantiske symboler/funksjoner"),
        parser.add_argument("--json", action="store_true", help="Skriv debug-output som JSON"),
    ),
    run=lambda args: run_debug(args),
)


def run_debug(args) -> int:
    try:
        show_tokens = args.tokens
        show_ast = args.ast
        show_symbols = args.symbols
        if not (show_tokens or show_ast or show_symbols):
            show_symbols = True

        source_path = Path(args.file).expanduser().resolve()
        program, analyzer, tokens = _analyze_source(source_path)
        imports = []
        for item in getattr(program, "imports", []):
            row = {"module": item.module_name}
            if getattr(item, "alias", None):
                row["alias"] = item.alias
            imports.append(row)

        functions = []
        for name, fn in sorted(analyzer.functions.items(), key=lambda item: item[0]):
            functions.append(
                {
                    "name": fn.name,
                    "params": _serialize_value(fn.params),
                    "return_type": fn.return_type,
                    "builtin": fn.builtin,
                    "module": fn.module_name,
                    "symbol": name,
                }
            )

        payload = {
            "source": str(source_path),
            "imports": imports,
            "functions": functions,
        }
        if show_tokens:
            payload["tokens"] = [_serialize_token(token) for token in tokens]
        if show_symbols:
            payload["symbols"] = {
                item["name"]: item
                for item in functions
            }
            aliases = {
                item["alias"]: item["module"]
                for item in imports
                if "alias" in item
            }
            if aliases:
                payload["aliases"] = aliases
        if show_ast:
            payload["ast"] = _serialize_value(program)

        if args.json:
            print(json.dumps(payload, ensure_ascii=False, indent=2))
        else:
            print(f"Kilde: {payload['source']}")
            print(f"Imports: {len(payload.get('imports', []))}")
            for imp in payload.get("imports", []):
                alias_text = f" som {imp['alias']}" if imp.get("alias") else ""
                print(f"  bruk {imp['module']}{alias_text}")

            print(f"Funksjoner: {len(payload.get('functions', []))}")
            for fn in payload.get("functions", []):
                print(f"  {fn['name']} (params: {fn['params']})")

            if "tokens" in payload:
                print("Tokens:")
                for tok in payload["tokens"]:
                    print(f"  {tok['line']}:{tok['column']} {tok['type']} {repr(tok['value'])}")

            if "symbols" in payload:
                print("Symboler:")
                for sym in payload["symbols"].values():
                    print(
                        f"  {sym['name']} -> modul={sym['module']} "
                        f"params={sym['params']} return={sym['return_type']}"
                    )
                if payload.get("aliases"):
                    print("Aliaser:")
                    for alias, module_name in payload["aliases"].items():
                        print(f"  {alias} => {module_name}")

            if "ast" in payload:
                print("AST:")
                print(json.dumps(payload["ast"], ensure_ascii=False, indent=2))
        return 0
    except Exception as exc:
        print(f"Feil: {_format_cli_exception(exc)}")
        return 1


DISASM_COMMAND = CommandModule(
    name="disasm",
    help="Vis generert C-kode for en .no-fil",
    register_arguments=register_disasm_arguments,
    run=run_disasm,
)

IR_DISASM_COMMAND = CommandModule(
    name="ir-disasm",
    help="Vis IR-disassembly fra tekstfil",
    register_arguments=register_ir_disasm_arguments,
    run=run_ir_disasm,
)

UPDATE_SNAPSHOTS_COMMAND = CommandModule(
    name="update-snapshots",
    help="Regenerer IR snapshot-forventninger",
    register_arguments=register_update_snapshots_arguments,
    run=run_update_snapshots,
)
