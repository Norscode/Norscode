from __future__ import annotations

import difflib
import json

from .ast_nodes import *
from .lexer import Lexer
from .parser import Parser


_BIN_PREC = {
    "ELLER": 10,
    "OG": 20,
    "EQ": 30,
    "NE": 30,
    "GT": 40,
    "LT": 40,
    "GTE": 40,
    "LTE": 40,
    "PLUS": 50,
    "MINUS": 50,
    "MUL": 60,
    "DIV": 60,
    "PERCENT": 60,
}

_ASSOCIATIVE_BINOPS = {"PLUS", "MUL", "OG", "ELLER"}


class SourceFormatter:
    def format_source(self, source_text: str) -> str:
        program = Parser(Lexer(source_text)).parse()
        return self.format_program(program)

    def format_program(self, program) -> str:
        lines: list[str] = []

        imports = list(getattr(program, "imports", []))
        functions = list(getattr(program, "functions", []))
        tests = list(getattr(program, "tests", []))

        for imp in imports:
            lines.append(self.format_import(imp))

        if imports and (functions or tests):
            lines.append("")

        for fn in functions:
            lines.extend(self.format_function(fn))
            lines.append("")

        for test in tests:
            lines.extend(self.format_test(test))
            lines.append("")

        while lines and lines[-1] == "":
            lines.pop()

        return "\n".join(lines) + ("\n" if lines else "")

    def format_import(self, imp) -> str:
        if getattr(imp, "alias", None):
            return f"bruk {imp.module_name} som {imp.alias}"
        return f"bruk {imp.module_name}"

    def format_function(self, fn) -> list[str]:
        header = ("async " if getattr(fn, "is_async", False) else "") + f"funksjon {fn.name}({self.format_params(fn.params)}) -> {fn.return_type} {{"
        lines = [header]
        lines.extend(self.format_block(fn.body, 1))
        lines.append("}")
        return lines

    def format_test(self, test) -> list[str]:
        lines = [f'test {json.dumps(test.name, ensure_ascii=False)} {{']
        lines.extend(self.format_block(test.body, 1))
        lines.append("}")
        return lines

    def format_params(self, params) -> str:
        return ", ".join(f"{param.name}: {param.type_name}" for param in params)

    def format_block(self, block, indent: int) -> list[str]:
        lines: list[str] = []
        for stmt in getattr(block, "statements", []):
            lines.extend(self.format_stmt(stmt, indent))
        return lines

    def line(self, indent: int, text: str) -> str:
        return ("    " * indent) + text

    def format_stmt(self, stmt, indent: int) -> list[str]:
        if isinstance(stmt, VarDeclareNode):
            head = f"la {stmt.name}"
            if stmt.var_type is not None:
                head += f": {stmt.var_type}"
            head += f" = {self.format_expr(stmt.expr)}"
            return [self.line(indent, head)]

        if isinstance(stmt, VarSetNode):
            return [self.line(indent, f"{stmt.name} = {self.format_expr(stmt.expr)}")]

        if isinstance(stmt, IndexSetNode):
            return [self.line(indent, f"{stmt.target_name}[{self.format_expr(stmt.index_expr)}] = {self.format_expr(stmt.value_expr)}")]

        if isinstance(stmt, PrintNode):
            return [self.line(indent, f"skriv({self.format_expr(stmt.expr)})")]

        if isinstance(stmt, IfNode):
            return self.format_if(stmt, indent)

        if isinstance(stmt, MatchNode):
            return self.format_match(stmt, indent)

        if isinstance(stmt, WhileNode):
            lines = [self.line(indent, f"mens ({self.format_expr(stmt.condition)}) {{")]
            lines.extend(self.format_block(stmt.body, indent + 1))
            lines.append(self.line(indent, "}"))
            return lines

        if isinstance(stmt, ForNode):
            lines = [
                self.line(
                    indent,
                    f"for {stmt.name} = {self.format_expr(stmt.start_expr)} til {self.format_expr(stmt.end_expr)} steg {self.format_expr(stmt.step_expr)} {{",
                )
            ]
            lines.extend(self.format_block(stmt.body, indent + 1))
            lines.append(self.line(indent, "}"))
            return lines

        if isinstance(stmt, ForEachNode):
            lines = [self.line(indent, f"for {stmt.item_name} i {self.format_expr(stmt.list_expr)} {{")]
            lines.extend(self.format_block(stmt.body, indent + 1))
            lines.append(self.line(indent, "}"))
            return lines

        if isinstance(stmt, ReturnNode):
            return [self.line(indent, f"returner {self.format_expr(stmt.expr)}")]

        if isinstance(stmt, BreakNode):
            return [self.line(indent, "bryt")]

        if isinstance(stmt, ContinueNode):
            return [self.line(indent, "fortsett")]

        if isinstance(stmt, ExprStmtNode):
            return [self.line(indent, self.format_expr(stmt.expr))]

        if isinstance(stmt, ThrowNode):
            return [self.line(indent, f"kast {self.format_expr(stmt.expr)}")]

        if isinstance(stmt, TryCatchNode):
            lines = [self.line(indent, "prøv {")]
            lines.extend(self.format_block(stmt.try_block, indent + 1))
            catch_head = "fang"
            if stmt.catch_var_name:
                catch_head += f"({stmt.catch_var_name})"
            lines.append(self.line(indent, f"}} {catch_head} {{"))
            lines.extend(self.format_block(stmt.catch_block, indent + 1))
            lines.append(self.line(indent, "}"))
            return lines

        return [self.line(indent, f"# ukjent statement {type(stmt).__name__}")]

    def format_if(self, stmt, indent: int) -> list[str]:
        lines = [self.line(indent, f"hvis ({self.format_expr(stmt.condition)}) {{")]
        lines.extend(self.format_block(stmt.then_block, indent + 1))
        lines.append(self.line(indent, "}"))

        for elif_cond, elif_block in getattr(stmt, "elif_blocks", []):
            lines[-1] = lines[-1] + f" ellers hvis ({self.format_expr(elif_cond)}) {{"
            lines.extend(self.format_block(elif_block, indent + 1))
            lines.append(self.line(indent, "}"))

        if stmt.else_block:
            lines[-1] = lines[-1] + " ellers {"
            lines.extend(self.format_block(stmt.else_block, indent + 1))
            lines.append(self.line(indent, "}"))

        return lines

    def format_match(self, stmt, indent: int) -> list[str]:
        lines = [self.line(indent, f"match {self.format_expr(stmt.subject)} {{")]
        for case in getattr(stmt, "cases", []):
            if getattr(case, "wildcard", False):
                lines.append(self.line(indent + 1, "case _ {"))
            else:
                lines.append(self.line(indent + 1, f"case {self.format_expr(case.pattern)} {{"))
            lines.extend(self.format_block(case.body, indent + 2))
            lines.append(self.line(indent + 1, "}"))

        if stmt.else_block:
            lines.append(self.line(indent + 1, "ellers {"))
            lines.extend(self.format_block(stmt.else_block, indent + 2))
            lines.append(self.line(indent + 1, "}"))

        lines.append(self.line(indent, "}"))
        return lines

    def format_expr(self, expr, parent_prec: int = 0, side: str | None = None) -> str:
        if expr is None:
            return ""

        if isinstance(expr, NumberNode):
            return str(expr.value)

        if isinstance(expr, StringNode):
            return json.dumps(expr.value, ensure_ascii=False)

        if isinstance(expr, BoolNode):
            return "sann" if expr.value else "usann"

        if isinstance(expr, VarAccessNode):
            return expr.name

        if isinstance(expr, ListLiteralNode):
            return "[" + ", ".join(self.format_expr(item) for item in expr.items) + "]"

        if isinstance(expr, ListComprehensionNode):
            rendered = f"[{self.format_expr(expr.item_expr)} for {expr.item_name} i {self.format_expr(expr.source_expr)}"
            if expr.condition_expr is not None:
                rendered += f" hvis {self.format_expr(expr.condition_expr)}"
            return rendered + "]"

        if isinstance(expr, MapLiteralNode):
            return "{" + ", ".join(f"{self.format_expr(key)}: {self.format_expr(value)}" for key, value in expr.items) + "}"

        if isinstance(expr, StructLiteralNode):
            return "{" + ", ".join(f"{name}: {self.format_expr(value)}" for name, value in expr.fields) + "}"

        if isinstance(expr, CallNode):
            return f"{expr.name}({', '.join(self.format_expr(arg) for arg in expr.args)})"

        if isinstance(expr, ModuleCallNode):
            return f"{expr.module_name}.{expr.func_name}({', '.join(self.format_expr(arg) for arg in expr.args)})"

        if isinstance(expr, FieldAccessNode):
            return f"{self.format_expr(expr.target, 90)}.{expr.field}"

        if isinstance(expr, IndexNode):
            return f"{self.format_expr(expr.list_expr, 90)}[{self.format_expr(expr.index_expr)}]"

        if isinstance(expr, SliceNode):
            start = "" if expr.start_expr is None else self.format_expr(expr.start_expr)
            end = "" if expr.end_expr is None else self.format_expr(expr.end_expr)
            return f"{self.format_expr(expr.target, 90)}[{start}:{end}]"

        if isinstance(expr, AwaitNode):
            rendered = f"await {self.format_expr(expr.expr, 80)}"
            return self._wrap(rendered, 80, parent_prec)

        if isinstance(expr, LambdaNode):
            params = ", ".join(
                f"{param.name}: {param.type_name}" if getattr(param, "type_name", None) else param.name
                for param in expr.params
            )
            rendered = f"fun({params}) -> {self.format_expr(expr.body)}"
            return self._wrap(rendered, 5, parent_prec)

        if isinstance(expr, UnaryOpNode):
            op = {"PLUS": "+", "MINUS": "-", "IKKE": "ikke"}.get(expr.op.typ, expr.op.value or expr.op.typ)
            prec = 80
            rendered = f"{op}{' ' if op == 'ikke' else ''}{self.format_expr(expr.node, prec)}"
            return self._wrap(rendered, prec, parent_prec)

        if isinstance(expr, BinOpNode):
            prec = _BIN_PREC.get(expr.op.typ, 0)
            op = {
                "PLUS": "+",
                "MINUS": "-",
                "MUL": "*",
                "DIV": "/",
                "PERCENT": "%",
                "EQ": "==",
                "NE": "!=",
                "GT": ">",
                "LT": "<",
                "GTE": ">=",
                "LTE": "<=",
                "OG": "og",
                "ELLER": "eller",
            }.get(expr.op.typ, expr.op.value or expr.op.typ)
            left = self.format_expr(expr.left, prec, "left")
            right_prec = prec - 1 if expr.op.typ in _ASSOCIATIVE_BINOPS else prec
            right = self.format_expr(expr.right, right_prec, "right")
            rendered = f"{left} {op} {right}"
            return self._wrap(rendered, prec, parent_prec)

        if isinstance(expr, IfExprNode):
            rendered = (
                f"hvis ({self.format_expr(expr.condition)}) da {self.format_expr(expr.then_expr)} "
                f"ellers {self.format_expr(expr.else_expr)}"
            )
            return self._wrap(rendered, 5, parent_prec)

        return f"<{type(expr).__name__}>"

    def _wrap(self, rendered: str, prec: int, parent_prec: int) -> str:
        if prec < parent_prec:
            return f"({rendered})"
        return rendered


def format_source(source_text: str) -> str:
    return SourceFormatter().format_source(source_text)


def format_file_text(source_text: str) -> str:
    return format_source(source_text)
