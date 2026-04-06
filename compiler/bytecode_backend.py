from __future__ import annotations

import argparse
import json
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from .ast_nodes import (
    BinOpNode,
    BlockNode,
    BoolNode,
    BreakNode,
    CallNode,
    ContinueNode,
    ExprStmtNode,
    ForNode,
    FunctionNode,
    IfNode,
    IfExprNode,
    ImportNode,
    IndexNode,
    IndexSetNode,
    ListLiteralNode,
    ModuleCallNode,
    NumberNode,
    PrintNode,
    ProgramNode,
    ReturnNode,
    StringNode,
    UnaryOpNode,
    VarAccessNode,
    VarDeclareNode,
    VarSetNode,
    WhileNode,
)
from .ast_bridge import load_source_as_program, read_ast


class BytecodeCompileError(RuntimeError):
    pass


class BytecodeRuntimeError(RuntimeError):
    pass


@dataclass
class LoopLabels:
    start: str
    end: str


class BytecodeCompiler:
    def __init__(self, alias_map: dict[str, str] | None = None):
        self.alias_map = alias_map or {}
        self.label_counter = 0
        self.loop_stack: list[LoopLabels] = []
        self.current_module = "__main__"

    def new_label(self, prefix: str) -> str:
        self.label_counter += 1
        return f"{prefix}_{self.label_counter}"

    def compile_program(self, program: ProgramNode) -> dict[str, Any]:
        functions = {}
        imports = []
        for imp in getattr(program, "imports", []):
            imports.append({"module": imp.module_name, "alias": imp.alias})
        for fn in getattr(program, "functions", []):
            key = self.function_key(fn)
            functions[key] = self.compile_function(fn)
        return {
            "format": "norscode-bytecode-v1",
            "entry": "__main__.start",
            "imports": imports,
            "functions": functions,
        }

    def function_key(self, fn: FunctionNode) -> str:
        module_name = getattr(fn, "module_name", None) or "__main__"
        return f"{module_name}.{fn.name}"

    def compile_function(self, fn: FunctionNode) -> dict[str, Any]:
        code: list[list[Any]] = []
        previous_module = self.current_module
        self.current_module = getattr(fn, "module_name", None) or "__main__"
        self.emit_block(fn.body, code)
        self.current_module = previous_module
        if not code or code[-1][0] != "RETURN":
            code.append(["PUSH_CONST", None])
            code.append(["RETURN"])
        return {
            "name": fn.name,
            "module": getattr(fn, "module_name", None) or "__main__",
            "params": [p.name for p in getattr(fn, "params", [])],
            "code": code,
        }

    def emit_block(self, block: BlockNode, code: list[list[Any]]):
        for stmt in getattr(block, "statements", []):
            self.emit_stmt(stmt, code)

    def emit_stmt(self, node: Any, code: list[list[Any]]):
        if isinstance(node, VarDeclareNode):
            self.emit_expr(node.expr, code)
            code.append(["STORE_NAME", node.name])
            return
        if isinstance(node, VarSetNode):
            self.emit_expr(node.expr, code)
            code.append(["STORE_NAME", node.name])
            return
        if isinstance(node, IndexSetNode):
            code.append(["LOAD_NAME", node.target_name])
            self.emit_expr(node.index_expr, code)
            self.emit_expr(node.value_expr, code)
            code.append(["INDEX_SET"])
            return
        if isinstance(node, ExprStmtNode):
            self.emit_expr(node.expr, code)
            code.append(["POP"])
            return
        if isinstance(node, PrintNode):
            self.emit_expr(node.expr, code)
            code.append(["CALL", "builtin.skriv", 1])
            code.append(["POP"])
            return
        if isinstance(node, ReturnNode):
            self.emit_expr(node.expr, code)
            code.append(["RETURN"])
            return
        if isinstance(node, IfNode):
            end_label = self.new_label("ifend")
            else_label = self.new_label("ifelse") if (node.else_block or node.elif_blocks) else end_label
            self.emit_expr(node.condition, code)
            code.append(["JUMP_IF_FALSE", else_label])
            self.emit_block(node.then_block, code)
            code.append(["JUMP", end_label])
            current_else = else_label
            if node.elif_blocks:
                for idx, (cond, block) in enumerate(node.elif_blocks):
                    code.append(["LABEL", current_else])
                    next_else = self.new_label(f"elifelse_{idx}") if (idx < len(node.elif_blocks)-1 or node.else_block) else end_label
                    self.emit_expr(cond, code)
                    code.append(["JUMP_IF_FALSE", next_else])
                    self.emit_block(block, code)
                    code.append(["JUMP", end_label])
                    current_else = next_else
            if node.else_block:
                code.append(["LABEL", current_else])
                self.emit_block(node.else_block, code)
            code.append(["LABEL", end_label])
            return
        if isinstance(node, WhileNode):
            start = self.new_label("while_start")
            end = self.new_label("while_end")
            self.loop_stack.append(LoopLabels(start, end))
            code.append(["LABEL", start])
            self.emit_expr(node.condition, code)
            code.append(["JUMP_IF_FALSE", end])
            self.emit_block(node.body, code)
            code.append(["JUMP", start])
            code.append(["LABEL", end])
            self.loop_stack.pop()
            return
        if isinstance(node, ForNode):
            start = self.new_label("for_start")
            end = self.new_label("for_end")
            self.emit_expr(node.start_expr, code)
            code.append(["STORE_NAME", node.name])
            self.loop_stack.append(LoopLabels(start, end))
            code.append(["LABEL", start])
            code.append(["LOAD_NAME", node.name])
            self.emit_expr(node.end_expr, code)
            code.append(["COMPARE_LE"])
            code.append(["JUMP_IF_FALSE", end])
            self.emit_block(node.body, code)
            code.append(["LOAD_NAME", node.name])
            self.emit_expr(node.step_expr, code)
            code.append(["BINARY_ADD"])
            code.append(["STORE_NAME", node.name])
            code.append(["JUMP", start])
            code.append(["LABEL", end])
            self.loop_stack.pop()
            return
        if isinstance(node, BreakNode):
            if not self.loop_stack:
                raise BytecodeCompileError("'bryt' utenfor løkke")
            code.append(["JUMP", self.loop_stack[-1].end])
            return
        if isinstance(node, ContinueNode):
            if not self.loop_stack:
                raise BytecodeCompileError("'fortsett' utenfor løkke")
            code.append(["JUMP", self.loop_stack[-1].start])
            return
        raise BytecodeCompileError(f"Bytecode-backend støtter ikke statement: {type(node).__name__}")

    def emit_expr(self, node: Any, code: list[list[Any]]):
        if isinstance(node, NumberNode):
            code.append(["PUSH_CONST", node.value])
            return
        if isinstance(node, StringNode):
            code.append(["PUSH_CONST", node.value])
            return
        if isinstance(node, BoolNode):
            code.append(["PUSH_CONST", node.value])
            return
        if isinstance(node, VarAccessNode):
            code.append(["LOAD_NAME", node.name])
            return
        if isinstance(node, ListLiteralNode):
            for item in node.items:
                self.emit_expr(item, code)
            code.append(["BUILD_LIST", len(node.items)])
            return
        if isinstance(node, IndexNode):
            self.emit_expr(node.list_expr, code)
            self.emit_expr(node.index_expr, code)
            code.append(["INDEX_GET"])
            return
        if isinstance(node, UnaryOpNode):
            self.emit_expr(node.node, code)
            if node.op.typ == "MINUS":
                code.append(["UNARY_NEG"])
                return
            if node.op.typ == "IKKE":
                code.append(["UNARY_NOT"])
                return
            if node.op.typ == "PLUS":
                return
            raise BytecodeCompileError(f"Ukjent unary-op: {node.op.typ}")
        if isinstance(node, BinOpNode):
            if node.op.typ == "OG":
                end_label = self.new_label("and_end")
                false_label = self.new_label("and_false")
                self.emit_expr(node.left, code)
                code.append(["JUMP_IF_FALSE", false_label])
                self.emit_expr(node.right, code)
                code.append(["JUMP_IF_FALSE", false_label])
                code.append(["PUSH_CONST", True])
                code.append(["JUMP", end_label])
                code.append(["LABEL", false_label])
                code.append(["PUSH_CONST", False])
                code.append(["LABEL", end_label])
                return
            if node.op.typ == "ELLER":
                end_label = self.new_label("or_end")
                true_label = self.new_label("or_true")
                self.emit_expr(node.left, code)
                code.append(["JUMP_IF_FALSE", true_label + "_rhs"])
                code.append(["PUSH_CONST", True])
                code.append(["JUMP", end_label])
                code.append(["LABEL", true_label + "_rhs"])
                self.emit_expr(node.right, code)
                code.append(["JUMP_IF_FALSE", true_label + "_false"])
                code.append(["PUSH_CONST", True])
                code.append(["JUMP", end_label])
                code.append(["LABEL", true_label + "_false"])
                code.append(["PUSH_CONST", False])
                code.append(["LABEL", end_label])
                return
            self.emit_expr(node.left, code)
            self.emit_expr(node.right, code)
            mapping = {
                "PLUS": "BINARY_ADD",
                "MINUS": "BINARY_SUB",
                "MUL": "BINARY_MUL",
                "DIV": "BINARY_DIV",
                "EQ": "COMPARE_EQ",
                "NE": "COMPARE_NE",
                "GT": "COMPARE_GT",
                "LT": "COMPARE_LT",
                "GTE": "COMPARE_GE",
                "LTE": "COMPARE_LE",
            }
            opcode = mapping.get(node.op.typ)
            if opcode is None:
                raise BytecodeCompileError(f"Ukjent binæroperator: {node.op.typ}")
            code.append([opcode])
            return
        if isinstance(node, CallNode):
            for arg in node.args:
                self.emit_expr(arg, code)
            code.append(["CALL", self.resolve_call_name(node.name), len(node.args)])
            return
        if isinstance(node, ModuleCallNode):
            for arg in node.args:
                self.emit_expr(arg, code)
            module_name = self.alias_map.get(node.module_name, node.module_name)
            code.append(["CALL", f"{module_name}.{node.func_name}", len(node.args)])
            return
        if isinstance(node, IfExprNode):
            else_label = self.new_label("ifexpr_else")
            end_label = self.new_label("ifexpr_end")
            self.emit_expr(node.condition, code)
            code.append(["JUMP_IF_FALSE", else_label])
            self.emit_expr(node.then_expr, code)
            code.append(["JUMP", end_label])
            code.append(["LABEL", else_label])
            self.emit_expr(node.else_expr, code)
            code.append(["LABEL", end_label])
            return
        raise BytecodeCompileError(f"Bytecode-backend støtter ikke uttrykk: {type(node).__name__}")

    def resolve_call_name(self, name: str) -> str:
        builtins = {
            "assert", "assert_eq", "assert_ne", "skriv", "lengde",
            "tekst_fra_heltall", "tekst_fra_bool",
        }
        if name in builtins:
            return f"builtin.{name}"
        return f"{self.current_module}.{name}"


class BytecodeVM:
    def __init__(
        self,
        program: dict[str, Any],
        trace: bool = False,
        max_steps: int = 200000,
        trace_focus: str | None = None,
        repeat_limit: int = 0,
        expr_probe: str | None = None,
        expr_probe_log: str | None = None,
    ):
        self.program = program
        self.functions = program.get("functions", {})
        self.output: list[str] = []
        self.trace = trace
        self.max_steps = max_steps
        self.trace_focus = trace_focus.strip() if isinstance(trace_focus, str) and trace_focus.strip() else None
        self.repeat_limit = max(0, int(repeat_limit))
        self.expr_probe = expr_probe.strip() if isinstance(expr_probe, str) and expr_probe.strip() else None
        self.expr_probe_log = expr_probe_log.strip() if isinstance(expr_probe_log, str) and expr_probe_log.strip() else None
        self.expr_probe_events: list[str] = []
        self.steps = 0
        self.trace_log: list[str] = []
        self._repeat_state_key: tuple[Any, ...] | None = None
        self._repeat_state_count = 0
        self._selfhost_token_map = self._load_selfhost_token_map()
        self._memo_cache: dict[tuple[str, str], Any] = {}
        self._memoizable_functions = {
            "selfhost.compiler.normaliser_norsk_token",
            "selfhost.compiler.stack_behov",
            "selfhost.compiler.stack_endring",
            "selfhost.compiler.op_krever_arg",
            "selfhost.compiler.op_kjent",
            "selfhost.compiler.er_heltall_token",
            "selfhost.compiler.operator_til_opcode",
            "selfhost.compiler.uttrykk_til_ops_og_verdier_med_miljo",
            "selfhost.compiler.instruksjon_til_tekst",
            "selfhost.compiler.append_linje",
            "selfhost.compiler.disasm_program",
        }

    def _should_trace_function(self, name: str) -> bool:
        if not self.trace:
            return False
        if self.trace_focus is None:
            return True
        return self.trace_focus in name

    def _log(self, message: str) -> None:
        if not self.trace:
            return
        self.trace_log.append(message)
        if len(self.trace_log) > 400:
            del self.trace_log[:200]

    def get_trace_tail(self, limit: int = 60) -> list[str]:
        return self.trace_log[-limit:]

    def _record_expr_probe(self, lines: list[str]) -> None:
        payload = [str(line) for line in lines]
        self.expr_probe_events.extend(payload)
        if self.trace:
            for line in payload:
                self._log(line)

    def dump_expr_probe(self) -> str:
        text = "\n".join(self.expr_probe_events)
        if text and not text.endswith("\n"):
            text += "\n"
        if self.expr_probe_log and text:
            Path(self.expr_probe_log).expanduser().write_text(text, encoding="utf-8")
        return text


    def _load_selfhost_token_map(self) -> dict[str, str]:
        source_path = Path(__file__).resolve().parent.parent / "selfhost" / "compiler.no"
        if not source_path.exists():
            return {}
        lines = source_path.read_text(encoding="utf-8").splitlines()
        mapping: dict[str, str] = {}
        inside = False
        pending: list[str] = []
        for raw in lines:
            line = raw.strip()
            if line.startswith("funksjon normaliser_norsk_token"):
                inside = True
                continue
            if inside and line.startswith("funksjon "):
                break
            if not inside:
                continue
            if line.startswith("hvis (") and "tok ==" in line:
                pending = re.findall(r'"([^"]+)"', line)
                continue
            if pending and line.startswith("returner "):
                values = re.findall(r'"([^"]+)"', line)
                if values:
                    for key in pending:
                        mapping[key] = values[0]
                pending = []
        return mapping

    def _selfhost_emit_operator(self, ops: list[Any], verdier: list[Any], op: str) -> bool:
        if op in {"u!", "uikke"}:
            ops.append("NOT")
            verdier.append(0)
            return True
        if op == "u-":
            ops.extend(["PUSH", "SWAP", "SUB"])
            verdier.extend([0, 0, 0])
            return True
        if op in {"!=", "ikke_er"}:
            # Equality should be evaluated before negation so phrases like
            # "ikke er" and "ikke lik" produce EQ followed by NOT.
            ops.extend(["EQ", "NOT"])
            verdier.extend([0, 0])
            return True
        if op in {"xor", "^^"}:
            ops.extend(["EQ", "NOT"])
            verdier.extend([0, 0])
            return True
        if op == "xnor":
            ops.append("EQ")
            verdier.append(0)
            return True
        if op == "nand":
            ops.extend(["AND", "NOT"])
            verdier.extend([0, 0])
            return True
        if op == "nor":
            ops.extend(["OR", "NOT"])
            verdier.extend([0, 0])
            return True
        if op == "impliserer":
            ops.extend(["SWAP", "NOT", "SWAP", "OR"])
            verdier.extend([0, 0, 0, 0])
            return True
        if op == "impliseres_av":
            ops.extend(["NOT", "OR"])
            verdier.extend([0, 0])
            return True
        if op in {"<=", "mindre_eller_lik"}:
            ops.extend(["GT", "NOT"])
            verdier.extend([0, 0])
            return True
        if op in {">=", "storre_eller_lik"}:
            ops.extend(["LT", "NOT"])
            verdier.extend([0, 0])
            return True
        opcode = self._call_hot_selfhost_helper("selfhost.compiler.operator_til_opcode", [op])
        if opcode:
            ops.append(opcode)
            verdier.append(0)
            return True
        return False

    def _selfhost_operator_precedence(self, tok: str) -> int:
        if tok in {"u!", "u-", "uikke"}:
            return 7
        if tok in {"*", "/", "%"}:
            return 6
        if tok in {"+", "-"}:
            return 5
        if tok in {"<", ">", "<=", ">=", "mindre_enn", "storre_enn", "mindre_eller_lik", "storre_eller_lik"}:
            return 4
        if tok in {"==", "!=", "er", "ikke_er"}:
            return 3
        if tok in {"&&", "og", "samt", "nand"}:
            return 2
        if tok in {"||", "eller", "enten", "xor", "^^", "xnor", "nor", "impliserer", "impliseres_av"}:
            return 1
        return 0

    def _selfhost_is_operator_token(self, tok: str) -> bool:
        return tok in {
            "+", "-", "*", "/", "%", "==", "!=", "<", ">", "<=", ">=",
            "&&", "||", "!", "og", "eller", "samt", "ikke", "nand", "nor",
            "xor", "^^", "xnor", "impliserer", "impliseres_av", "er", "ikke_er",
            "mindre_enn", "storre_enn", "mindre_eller_lik", "storre_eller_lik", "enten"
        }

    def _hot_selfhost_expr_to_ops(self, tokens: list[Any], navn: list[Any], miljo_verdier: list[Any], ops: list[Any], verdier: list[Any]) -> str:
        toks = [str(t) for t in tokens]
        names = [str(n) for n in navn]
        vals = list(miljo_verdier)
        if len(names) != len(vals):
            return "/* feil: navn/miljø-verdier må ha samme lengde */"
        env = {n: vals[i] for i, n in enumerate(names)}
        operatorer: list[str] = []
        operator_pos: list[int] = []
        i = 0
        siste_token = -1
        forventer_verdi = True

        if self.expr_probe:
            probe = self.expr_probe.lower()
            raw_join = " ".join(toks).lower()
            if probe in raw_join or any(probe in str(t).lower() for t in toks):
                self._record_expr_probe([
                    f"[expr-probe] helper=selfhost.compiler.uttrykk_til_ops_og_verdier_med_miljo",
                    f"[expr-probe] raw_tokens={toks!r}",
                    f"[expr-probe] env_names={names!r}",
                    f"[expr-probe] env_values={vals!r}",
                ])

        def token_pos(pos: int) -> str:
            return f"token {pos}"

        while i < len(toks):
            tok_raw = toks[i]
            tok = str(self._call_hot_selfhost_helper("selfhost.compiler.normaliser_norsk_token", [tok_raw]))
            tok_step = 1
            n1 = str(self._call_hot_selfhost_helper("selfhost.compiler.normaliser_norsk_token", [toks[i + 1]])) if i + 1 < len(toks) else ""
            n2 = str(self._call_hot_selfhost_helper("selfhost.compiler.normaliser_norsk_token", [toks[i + 2]])) if i + 2 < len(toks) else ""
            n3 = str(self._call_hot_selfhost_helper("selfhost.compiler.normaliser_norsk_token", [toks[i + 3]])) if i + 3 < len(toks) else ""
            # compact normalizations for common multi-token forms
            if i + 1 < len(toks) and tok_raw == "-" and n1 == ">":
                tok = "impliserer"; tok_step = 2
            elif i + 1 < len(toks) and tok_raw == "=" and n1 == ">":
                tok = "impliserer"; tok_step = 2
            elif tok_raw in {"->", "=>"}:
                tok = "impliserer"; tok_step = 1
            elif tok_raw == "<-":
                tok = "impliseres_av"; tok_step = 1
            elif tok_raw in {"<->", "<=>"}:
                tok = "xnor"; tok_step = 1
            elif i + 2 < len(toks) and tok == "er" and n1 == "mindre" and n2 == "enn":
                tok = "mindre_enn"; tok_step = 3
            elif i + 3 < len(toks) and tok == "er" and n1 == "mindre" and n2 == "eller" and n3 == "lik":
                tok = "mindre_eller_lik"; tok_step = 4
            elif i + 3 < len(toks) and tok == "er" and n1 == "storre" and n2 == "eller" and n3 == "lik":
                tok = "storre_eller_lik"; tok_step = 4
            elif i + 2 < len(toks) and tok == "er" and n1 == "storre" and n2 == "enn":
                tok = "storre_enn"; tok_step = 3
            elif i + 3 < len(toks) and tok == "er" and n1 == "ikke" and n2 == "lik" and n3 == "med":
                tok = "ikke_er"; tok_step = 4
            elif i + 2 < len(toks) and tok == "er" and n1 == "ikke" and n2 == "lik":
                tok = "ikke_er"; tok_step = 3
            elif i + 2 < len(toks) and tok == "ikke" and n1 == "lik" and n2 == "med":
                tok = "ikke_er"; tok_step = 3
            elif i + 2 < len(toks) and tok == "er" and n1 == "lik" and n2 == "med":
                tok = "er"; tok_step = 3
            elif i + 2 < len(toks) and tok == "ikke" and n1 == "er" and n2 == "to":
                tok = "ikke_er"; tok_step = 3
            elif i + 2 < len(toks) and tok == "er" and n1 == "ikke" and n2 == "er":
                tok = "ikke_er"; tok_step = 3
            elif i + 1 < len(toks) and tok == "er" and n1 == "ulik":
                tok = "ikke_er"; tok_step = 2
            elif i + 1 < len(toks) and tok == "ikke" and n1 == "lik":
                tok = "ikke_er"; tok_step = 2
            elif i + 1 < len(toks) and tok == "er" and n1 == "ikke":
                tok = "ikke_er"; tok_step = 2
            elif i + 1 < len(toks) and tok == "ikke" and n1 == "er":
                tok = "ikke_er"; tok_step = 2
            elif i + 1 < len(toks) and tok == "er" and n1 == "lik":
                tok = "er"; tok_step = 2
            elif i + 2 < len(toks) and tok == "mindre" and n1 == "eller" and n2 == "lik":
                tok = "mindre_eller_lik"; tok_step = 3
            elif i + 2 < len(toks) and tok == "storre" and n1 == "eller" and n2 == "lik":
                tok = "storre_eller_lik"; tok_step = 3
            elif i + 1 < len(toks) and tok == "mindre" and n1 == "enn":
                tok = "mindre_enn"; tok_step = 2
            elif i + 1 < len(toks) and tok == "storre" and n1 == "enn":
                tok = "storre_enn"; tok_step = 2
            elif i + 1 < len(toks) and tok in {"pluss", "plus", "legg", "legge", "legges", "plusser", "pluses", "plusses", "plusse", "adder", "addere", "adderer", "adderes", "summer", "summerer", "summeres"} and n1 in {"med", "til", "sammen"}:
                tok = "+"; tok_step = 2
            elif i + 1 < len(toks) and tok in {"minus", "trekk", "trekke", "trekkes", "minuseres", "subtraher", "subtraherer", "subtraheres", "subtrahert"} and n1 in {"med", "fra"}:
                tok = "-"; tok_step = 2
            elif i + 1 < len(toks) and tok in {"gang", "gange", "ganget", "ganger", "ganges", "multipliser", "multiplisere", "multipliserer", "multiplisert", "multipliseres"} and n1 == "med":
                tok = "*"; tok_step = 2
            elif i + 2 < len(toks) and tok in {"dele", "deler", "divider", "dividere", "dividerer"} and n1 == "seg" and n2 in {"med", "pa", "paa"}:
                tok = "/"; tok_step = 3
            elif i + 1 < len(toks) and tok in {"delt", "dele", "deler", "deles", "del", "dividere", "divider", "dividerer", "dividert", "divideres"} and n1 in {"med", "pa", "paa"}:
                tok = "/"; tok_step = 2
            elif i + 1 < len(toks) and tok in {"mod", "modulo", "modul", "modulus", "rest", "resten"} and n1 == "av":
                tok = "%"; tok_step = 2
            elif tok in {"pluss", "plus", "legg", "legge", "legges", "plusser", "pluses", "plusses", "plusse", "adder", "addere", "adderer", "adderes", "summer", "summerer", "summeres", "pluss_med", "plus_med", "plusser_med", "plusses_med", "pluses_med", "plusse_med", "adder_med", "addere_med", "adderer_med", "adderes_med", "summer_med", "summerer_med", "summeres_med", "legg_til", "legge_til", "legges_til", "legg_sammen", "legge_sammen", "legges_sammen"}:
                tok = "+"; tok_step = 1
            elif tok in {"minus", "trekk", "trekke", "trekkes", "minuseres", "subtraher", "subtraherer", "subtraheres", "subtrahert", "trekk_fra", "trekke_fra", "trekkes_fra", "minus_fra", "minuseres_fra", "subtraher_fra", "subtraherer_fra", "subtraheres_fra", "subtrahert_fra", "minus_med", "minuseres_med", "trekk_med", "trekke_med", "trekkes_med", "subtraher_med", "subtraherer_med", "subtraheres_med", "subtrahert_med"}:
                tok = "-"; tok_step = 1
            elif tok in {"gang", "gange", "ganget", "ganger", "ganges", "multipliser", "multiplisere", "multipliserer", "multiplisert", "multipliseres", "gang_med", "ganget_med", "gange_med", "ganger_med", "ganges_med", "multipliser_med", "multiplisere_med", "multipliserer_med", "multiplisert_med", "multipliseres_med"}:
                tok = "*"; tok_step = 1
            elif tok in {"delt", "dele", "deler", "deles", "del", "dividere", "divider", "dividerer", "dividert", "divideres", "delt_pa", "deler_pa", "dele_pa", "deles_pa", "del_pa", "delt_med", "dele_med", "deler_med", "deles_med", "del_med", "dividere_pa", "divider_pa", "dividerer_pa", "dividert_pa", "divideres_pa", "dividere_med", "divider_med", "dividerer_med", "dividert_med", "divideres_med"}:
                tok = "/"; tok_step = 1
            elif tok in {"mod", "modulo", "modul", "modulus", "rest", "resten", "mod_av", "modulo_av", "modul_av", "modulus_av", "rest_av", "resten_av"}:
                tok = "%"; tok_step = 1
            elif tok.startswith(("minus", "trekk", "subtraher")) and tok.endswith(("med", "fra")):
                tok = "-"; tok_step = 1
            elif tok.startswith(("pluss", "plus", "legg", "adder", "summer")) and tok.endswith(("med", "til", "sammen")):
                tok = "+"; tok_step = 1
            elif tok.startswith(("gang", "multipliser")) and tok.endswith("med"):
                tok = "*"; tok_step = 1
            elif tok.startswith(("delt", "del", "dele", "deler", "deles", "divider", "dividere", "dividerer", "dividert", "divideres")) and tok.endswith(("med", "pa", "paa")):
                tok = "/"; tok_step = 1
            elif tok.startswith(("mod", "modulo", "modul", "modulus", "rest", "resten")) and tok.endswith("av"):
                tok = "%"; tok_step = 1
            elif tok in {"lik"}:
                tok = "er"; tok_step = 1
            elif tok in {"ulik", "ikke_lik"}:
                tok = "ikke_er"; tok_step = 1

            if tok == "":
                i += 1
                continue

            is_int = self._call_hot_selfhost_helper("selfhost.compiler.er_heltall_token", [tok])
            if is_int:
                if not forventer_verdi:
                    return f"/* feil: mangler operator før verdi {tok} ved {token_pos(i)} */"
                ops.append("PUSH"); verdier.append(int(tok))
                forventer_verdi = False
                siste_token = i + tok_step - 1
                i += tok_step
                continue

            if tok in {"sann", "usann"}:
                if not forventer_verdi:
                    return f"/* feil: mangler operator før verdi {tok} ved {token_pos(i)} */"
                ops.append("PUSH"); verdier.append(1 if tok == "sann" else 0)
                forventer_verdi = False
                siste_token = i + tok_step - 1
                i += tok_step
                continue

            if forventer_verdi and tok in env:
                ops.append("PUSH"); verdier.append(int(env[tok]))
                forventer_verdi = False
                siste_token = i + tok_step - 1
                i += tok_step
                continue

            if tok == "(":
                if not forventer_verdi:
                    return f"/* feil: mangler operator før ( ved {token_pos(i)} */"
                operatorer.append(tok); operator_pos.append(i)
                siste_token = i + tok_step - 1
                i += tok_step
                continue

            if tok == ")":
                if forventer_verdi:
                    return f"/* feil: mangler verdi før ) ved {token_pos(i)} */"
                fant = False
                while operatorer:
                    top = operatorer.pop(); top_pos = operator_pos.pop()
                    if top == "(":
                        fant = True
                        break
                    if not self._selfhost_emit_operator(ops, verdier, top):
                        return f"/* feil: ukjent operator {top} ved {token_pos(top_pos)} */"
                if not fant:
                    return f"/* feil: ) uten matchende ( ved {token_pos(i)} */"
                forventer_verdi = False
                siste_token = i + tok_step - 1
                i += tok_step
                continue

            if self._selfhost_is_operator_token(tok):
                if forventer_verdi:
                    if tok == "!":
                        operatorer.append("u!"); operator_pos.append(i); siste_token = i; i += tok_step; continue
                    if tok == "ikke":
                        operatorer.append("uikke"); operator_pos.append(i); siste_token = i; i += tok_step; continue
                    if tok == "-":
                        operatorer.append("u-"); operator_pos.append(i); siste_token = i; i += tok_step; continue
                    if tok == "+":
                        siste_token = i; i += tok_step; continue
                    return f"/* feil: mangler verdi før operator {tok} ved {token_pos(i)} */"
                while operatorer:
                    top = operatorer[-1]; top_pos = operator_pos[-1]
                    if top == "(":
                        break
                    if self._selfhost_operator_precedence(top) < self._selfhost_operator_precedence(tok):
                        break
                    operatorer.pop(); operator_pos.pop()
                    if not self._selfhost_emit_operator(ops, verdier, top):
                        return f"/* feil: ukjent operator {top} ved {token_pos(top_pos)} */"
                operatorer.append(tok); operator_pos.append(i)
                forventer_verdi = True
                siste_token = i + tok_step - 1
                i += tok_step
                continue

            return f"/* feil: ukjent token/navn i uttrykk {tok} ved {token_pos(i)} */"

        if forventer_verdi:
            if siste_token >= 0:
                return f"/* feil: uttrykket avsluttes med operator ved {token_pos(siste_token)} */"
            return "/* feil: uttrykket avsluttes med operator */"

        while operatorer:
            top = operatorer.pop(); top_pos = operator_pos.pop()
            if top == "(":
                return f"/* feil: mangler ) i uttrykk ved {token_pos(top_pos)} */"
            if not self._selfhost_emit_operator(ops, verdier, top):
                return f"/* feil: ukjent operator {top} ved {token_pos(top_pos)} */"
        return ""

    def _call_hot_selfhost_helper(self, name: str, args: list[Any]) -> Any:
        if name == "selfhost.compiler.normaliser_norsk_token":
            tok = str(args[0]) if args else ""
            return self._selfhost_token_map.get(tok, tok)
        if name == "selfhost.compiler.op_krever_arg":
            op = str(args[0]) if args else ""
            return op in {"PUSH", "LABEL", "JMP", "JZ", "CALL", "STORE", "LOAD"}
        if name == "selfhost.compiler.op_kjent":
            op = str(args[0]) if args else ""
            return op in {"PUSH", "LABEL", "JMP", "JZ", "CALL", "STORE", "LOAD", "ADD", "SUB", "MUL", "DIV", "MOD", "EQ", "GT", "LT", "AND", "OR", "NOT", "DUP", "POP", "SWAP", "OVER", "PRINT", "HALT", "RET"}
        if name == "selfhost.compiler.er_heltall_token":
            tok = str(args[0]) if args else ""
            try:
                return str(int(tok)) == tok
            except Exception:
                return False
        if name == "selfhost.compiler.operator_til_opcode":
            tok = str(args[0]) if args else ""
            mapping = {
                "+": "ADD", "-": "SUB", "*": "MUL", "/": "DIV", "%": "MOD",
                "==": "EQ", "er": "EQ", "<": "LT", "mindre_enn": "LT",
                ">": "GT", "storre_enn": "GT", "&&": "AND", "og": "AND",
                "samt": "AND", "||": "OR", "eller": "OR", "enten": "OR",
            }
            return mapping.get(tok, "")
        if name == "selfhost.compiler.stack_behov":
            op = str(args[0]) if args else ""
            if op in {"ADD", "SUB", "MUL", "DIV", "MOD", "EQ", "GT", "LT", "AND", "OR", "OVER", "SWAP"}:
                return 2
            if op in {"NOT", "POP", "DUP", "STORE", "PRINT", "JZ"}:
                return 1
            return 0
        if name == "selfhost.compiler.stack_endring":
            op = str(args[0]) if args else ""
            if op in {"PUSH", "LOAD", "DUP", "OVER"}:
                return 1
            if op in {"ADD", "SUB", "MUL", "DIV", "MOD", "EQ", "GT", "LT", "AND", "OR", "POP", "STORE"}:
                return -1
            return 0
        if name == "selfhost.compiler.uttrykk_til_ops_og_verdier_med_miljo":
            tokens = args[0] if len(args) > 0 else []
            navn = args[1] if len(args) > 1 else []
            miljo_verdier = args[2] if len(args) > 2 else []
            ops = args[3] if len(args) > 3 else []
            verdier = args[4] if len(args) > 4 else []
            return self._hot_selfhost_expr_to_ops(tokens, navn, miljo_verdier, ops, verdier)
        if name == "selfhost.compiler.instruksjon_til_tekst":
            op = str(args[0]) if len(args) > 0 else ""
            verdi = int(args[1]) if len(args) > 1 else 0
            if op in {"PUSH", "LABEL", "JMP", "JZ", "CALL", "STORE", "LOAD"}:
                return op + " " + str(verdi)
            return op
        if name == "selfhost.compiler.append_linje":
            kilde = str(args[0]) if len(args) > 0 else ""
            linje = str(args[1]) if len(args) > 1 else ""
            return kilde + linje + "\n"
        if name == "selfhost.compiler.disasm_program":
            ops = list(args[0]) if len(args) > 0 else []
            verdier = list(args[1]) if len(args) > 1 else []
            if len(ops) != len(verdier):
                return "/* feil: ops og verdier må ha samme lengde */"
            out = []
            for i, op in enumerate(ops):
                if op in {"PUSH", "LABEL", "JMP", "JZ", "CALL", "STORE", "LOAD"}:
                    instr = f"{op} {verdier[i]}"
                else:
                    instr = str(op)
                out.append(f"{i}: {instr}")
            return "\n".join(out) + ("\n" if out else "")
        raise BytecodeRuntimeError(f"Ukjent hot selfhost helper: {name}")

    def run(self, entry: str | None = None) -> Any:
        target = entry or self.program.get("entry") or "__main__.start"
        return self.call_function(target, [])

    def call_function(self, name: str, args: list[Any]) -> Any:
        if name.startswith("selfhost.compiler.") and name.rsplit(".", 1)[-1] in {
            "normaliser_norsk_token", "op_krever_arg", "op_kjent", "er_heltall_token", "operator_til_opcode", "stack_behov", "stack_endring", "uttrykk_til_ops_og_verdier_med_miljo", "instruksjon_til_tekst", "append_linje", "disasm_program"
        }:
            return self._call_hot_selfhost_helper(name, args)
        if name in self._memoizable_functions:
            memo_key = (name, repr(args))
            if memo_key in self._memo_cache:
                return self._memo_cache[memo_key]
        else:
            memo_key = None
        if name.startswith("builtin."):
            return self.call_builtin(name.removeprefix("builtin."), args)
        if name == "std.math.pluss":
            return args[0] + args[1]
        if name == "std.math.minus":
            return args[0] - args[1]
        fn = self.functions.get(name)
        if fn is None:
            short_name = name.rsplit('.', 1)[-1]
            builtin_like = {
                "assert", "assert_eq", "assert_ne", "skriv", "lengde",
                "tekst_fra_heltall", "tekst_fra_bool", "heltall_fra_tekst",
                "del_ord", "tokeniser_enkel", "tokeniser_uttrykk", "les_input",
                "legg_til", "pop_siste", "fjern_indeks", "sett_inn",
            }
            if short_name in builtin_like:
                return self.call_builtin(short_name, args)
            raise BytecodeRuntimeError(f"Ukjent funksjon: {name}")
        params = fn.get("params", [])
        locals_: dict[str, Any] = {param: value for param, value in zip(params, args)}
        code = fn.get("code", [])
        labels = {instr[1]: idx for idx, instr in enumerate(code) if instr[0] == "LABEL"}
        stack: list[Any] = []
        ip = 0
        while ip < len(code):
            self.steps += 1
            if self.steps > self.max_steps:
                raise BytecodeRuntimeError(f"Maks steg overskredet ({self.max_steps}) i {name} ved ip={ip}")
            instr = code[ip]
            op = instr[0]
            local_preview = {key: locals_[key] for key in list(locals_.keys())[:6]}
            if self._should_trace_function(name):
                self._log(f"step={self.steps} fn={name} ip={ip} op={instr!r} stack={stack[-4:]} locals={local_preview}")
            if self.repeat_limit > 0:
                state_key = (name, ip, tuple(instr), repr(stack[-4:]), repr(local_preview))
                if state_key == self._repeat_state_key:
                    self._repeat_state_count += 1
                else:
                    self._repeat_state_key = state_key
                    self._repeat_state_count = 1
                if self._repeat_state_count > self.repeat_limit:
                    raise BytecodeRuntimeError(
                        f"Mulig fastlås/løkke oppdaget i {name} ved ip={ip} op={instr!r} "
                        f"(samme tilstand > {self.repeat_limit} ganger)"
                    )
            if op == "LABEL":
                ip += 1
                continue
            if op == "PUSH_CONST":
                stack.append(instr[1])
            elif op == "LOAD_NAME":
                name2 = instr[1]
                if name2 not in locals_:
                    raise BytecodeRuntimeError(f"Ukjent variabel: {name2}")
                stack.append(locals_[name2])
            elif op == "STORE_NAME":
                locals_[instr[1]] = stack.pop()
            elif op == "POP":
                if stack:
                    stack.pop()
            elif op == "BUILD_LIST":
                n = instr[1]
                items = stack[-n:] if n else []
                if n:
                    del stack[-n:]
                stack.append(items)
            elif op == "INDEX_GET":
                idx = stack.pop(); lst = stack.pop(); stack.append(lst[idx])
            elif op == "INDEX_SET":
                value = stack.pop(); idx = stack.pop(); lst = stack.pop(); lst[idx] = value
            elif op == "UNARY_NEG":
                stack.append(-stack.pop())
            elif op == "UNARY_NOT":
                stack.append(not stack.pop())
            elif op == "BINARY_ADD":
                b = stack.pop(); a = stack.pop(); stack.append(a + b)
            elif op == "BINARY_SUB":
                b = stack.pop(); a = stack.pop(); stack.append(a - b)
            elif op == "BINARY_MUL":
                b = stack.pop(); a = stack.pop(); stack.append(a * b)
            elif op == "BINARY_DIV":
                b = stack.pop(); a = stack.pop(); stack.append(a / b)
            elif op == "COMPARE_EQ":
                b = stack.pop(); a = stack.pop(); stack.append(a == b)
            elif op == "COMPARE_NE":
                b = stack.pop(); a = stack.pop(); stack.append(a != b)
            elif op == "COMPARE_GT":
                b = stack.pop(); a = stack.pop(); stack.append(a > b)
            elif op == "COMPARE_LT":
                b = stack.pop(); a = stack.pop(); stack.append(a < b)
            elif op == "COMPARE_GE":
                b = stack.pop(); a = stack.pop(); stack.append(a >= b)
            elif op == "COMPARE_LE":
                b = stack.pop(); a = stack.pop(); stack.append(a <= b)
            elif op == "BINARY_AND":
                b = stack.pop(); a = stack.pop(); stack.append(bool(a) and bool(b))
            elif op == "BINARY_OR":
                b = stack.pop(); a = stack.pop(); stack.append(bool(a) or bool(b))
            elif op == "CALL":
                target, argc = instr[1], instr[2]
                call_args = stack[-argc:] if argc else []
                if argc:
                    del stack[-argc:]
                stack.append(self.call_function(target, call_args))
            elif op == "JUMP":
                ip = labels[instr[1]]
                continue
            elif op == "JUMP_IF_FALSE":
                value = stack.pop()
                if not value:
                    ip = labels[instr[1]]
                    continue
            elif op == "RETURN":
                result = stack.pop() if stack else None
                if memo_key is not None:
                    self._memo_cache[memo_key] = result
                return result
            else:
                raise BytecodeRuntimeError(f"Ukjent opcode: {op}")
            ip += 1
        if memo_key is not None:
            self._memo_cache[memo_key] = None
        return None

    def call_builtin(self, name: str, args: list[Any]) -> Any:
        if name == "skriv":
            text = " ".join(str(x) for x in args)
            print(text)
            self.output.append(text)
            return None
        if name == "assert":
            if not args[0]:
                raise AssertionError("Assert feilet")
            return None
        if name == "assert_eq":
            if args[0] != args[1]:
                raise AssertionError(f"assert_eq feilet: {args[0]!r} != {args[1]!r}")
            return None
        if name == "assert_ne":
            if args[0] == args[1]:
                raise AssertionError(f"assert_ne feilet: {args[0]!r} == {args[1]!r}")
            return None
        if name == "lengde":
            return len(args[0])
        if name == "tekst_fra_heltall":
            return str(args[0])
        if name == "tekst_fra_bool":
            return "sann" if args[0] else "usann"
        if name == "heltall_fra_tekst":
            try:
                return int(str(args[0]).strip())
            except Exception:
                return 0
        if name == "del_ord":
            return str(args[0]).split()
        if name == "tokeniser_enkel":
            text = str(args[0])
            tokens = []
            current = []
            in_comment = False
            for ch in text:
                if in_comment:
                    if ch == "\n":
                        in_comment = False
                    continue
                if ch == '#':
                    if current:
                        tokens.append(''.join(current))
                        current = []
                    in_comment = True
                    continue
                if ch.isalnum() or ch in '_-':
                    current.append(ch)
                else:
                    if current:
                        tokens.append(''.join(current))
                        current = []
            if current:
                tokens.append(''.join(current))
            return tokens
        if name == "tokeniser_uttrykk":
            text = str(args[0])
            token_re = re.compile(r'<=>|<->|=>|->|<-|&&|\|\||\+=|-=|\*=|/=|%=|==|!=|<=|>=|<>|[=!(){}\[\],.:;+\-*/%<>]|"[^"\\]*(?:\\.[^"\\]*)*"|[A-Za-zÆØÅæøå_][A-Za-zÆØÅæøå0-9_]*|\d+|\S')
            return [m.group(0) for m in token_re.finditer(text) if m.group(0).strip()]
        if name == "les_input":
            return ""
        if name == "legg_til":
            args[0].append(args[1])
            return None
        if name == "pop_siste":
            if not args[0]:
                return "" if isinstance(args[0], list) and any(isinstance(x, str) for x in args[0]) else 0
            return args[0].pop()
        if name == "fjern_indeks":
            if args[1] < 0 or args[1] >= len(args[0]):
                return len(args[0])
            args[0].pop(args[1])
            return len(args[0])
        if name == "sett_inn":
            lst = args[0]
            idx = int(args[1])
            value = args[2]
            if idx < 0:
                idx = 0
            if idx < len(lst):
                lst[idx] = value
            elif idx == len(lst):
                lst.append(value)
            else:
                while len(lst) < idx:
                    lst.append(0 if not lst or not isinstance(lst[0], str) else "")
                lst.append(value)
            return None
        raise BytecodeRuntimeError(f"Ukjent builtin: {name}")


def compile_program_to_bytecode(program, alias_map: dict[str, str] | None = None) -> dict[str, Any]:
    compiler = BytecodeCompiler(alias_map=alias_map or {})
    return compiler.compile_program(program)


def compile_source_to_bytecode(source_file: str) -> tuple[Path, dict[str, Any]]:
    source_path, program, alias_map = load_source_as_program(source_file)
    payload = compile_program_to_bytecode(program, alias_map=alias_map)
    return source_path, payload


def compile_ast_file_to_bytecode(ast_file: str) -> tuple[Path, dict[str, Any]]:
    ast_path = Path(ast_file).expanduser().resolve()
    program, alias_map = read_ast(str(ast_path))
    payload = compile_program_to_bytecode(program, alias_map=alias_map)
    return ast_path, payload


def default_output_path(source_path: Path) -> Path:
    return source_path.with_suffix('.ncb.json')


def build_command(source_file: str | None = None, output: str | None = None, ast_file: str | None = None) -> Path:
    if source_file:
        source_path, payload = compile_source_to_bytecode(source_file)
    elif ast_file:
        source_path, payload = compile_ast_file_to_bytecode(ast_file)
    else:
        raise RuntimeError("Mangler kildefil eller AST-fil")
    out_path = Path(output).expanduser().resolve() if output else default_output_path(source_path)
    out_path.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding='utf-8')
    return out_path


def run_command(source_file: str | None = None, bytecode_file: str | None = None, ast_file: str | None = None) -> Any:
    if source_file:
        _source_path, payload = compile_source_to_bytecode(source_file)
    elif ast_file:
        _ast_path, payload = compile_ast_file_to_bytecode(ast_file)
    elif bytecode_file:
        payload = json.loads(Path(bytecode_file).expanduser().read_text(encoding='utf-8'))
    else:
        raise RuntimeError("Mangler kildefil, AST-fil eller bytecode-fil")
    vm = BytecodeVM(payload)
    return vm.run()


def main():
    parser = argparse.ArgumentParser(prog='python -m compiler.bytecode_backend', description='AST -> bytecode backend (v17)')
    sub = parser.add_subparsers(dest='cmd')

    build = sub.add_parser('build', help='Bygg .no eller .nast.json til .ncb.json')
    build.add_argument('file')
    build.add_argument('--output')
    build.add_argument('--ast', action='store_true', help='Tolker input som .nast.json i stedet for .no')

    run = sub.add_parser('run', help='Kjør fra .no, .nast.json eller .ncb.json')
    run.add_argument('file')
    run.add_argument('--bytecode', action='store_true', help='Tolker input som .ncb.json i stedet for .no')
    run.add_argument('--ast', action='store_true', help='Tolker input som .nast.json i stedet for .no')

    args = parser.parse_args()
    if args.cmd == 'build':
        if args.ast:
            path = build_command(ast_file=args.file, output=args.output)
        else:
            path = build_command(source_file=args.file, output=args.output)
        print(path)
    elif args.cmd == 'run':
        if args.bytecode:
            result = run_command(bytecode_file=args.file)
        elif args.ast:
            result = run_command(ast_file=args.file)
        else:
            result = run_command(source_file=args.file)
        if result is not None:
            print(result)
    else:
        parser.print_help()


if __name__ == '__main__':
    main()
