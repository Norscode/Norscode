

from __future__ import annotations

from dataclasses import dataclass
from typing import Any

from .ast_nodes import (
    BinOpNode,
    BlockNode,
    BoolNode,
    CallNode,
    ExprStmtNode,
    IfExprNode,
    FunctionNode,
    ModuleCallNode,
    NumberNode,
    ProgramNode,
    ReturnNode,
    StringNode,
    UnaryOpNode,
    VarAccessNode,
    VarDeclareNode,
    VarSetNode,
)


class ReturnSignal(Exception):
    def __init__(self, value: Any):
        self.value = value
        super().__init__()


@dataclass
class UserFunction:
    node: FunctionNode


class Interpreter:
    def __init__(self):
        self.globals: dict[str, Any] = {}
        self.functions: dict[str, UserFunction] = {}
        self.modules: dict[str, Any] = {}
        self.scopes: list[dict[str, Any]] = [self.globals]

    def push_scope(self):
        self.scopes.append({})

    def pop_scope(self):
        self.scopes.pop()

    def current_scope(self) -> dict[str, Any]:
        return self.scopes[-1]

    def define_var(self, name: str, value: Any):
        self.current_scope()[name] = value

    def set_var(self, name: str, value: Any):
        for scope in reversed(self.scopes):
            if name in scope:
                scope[name] = value
                return value
        raise NameError(f"Ukjent variabel: {name}")

    def get_var(self, name: str) -> Any:
        for scope in reversed(self.scopes):
            if name in scope:
                return scope[name]
        raise NameError(f"Ukjent variabel: {name}")

    def run(self, program: ProgramNode):
        for fn in getattr(program, "functions", []):
            self.functions[fn.name] = UserFunction(fn)

        for fn in getattr(program, "tests", []):
            self.functions[fn.name] = UserFunction(fn)

        if "start" not in self.functions:
            return None
        return self.call_user_function("start", [])

    def eval(self, node):
        if isinstance(node, NumberNode):
            return node.value

        if isinstance(node, StringNode):
            return node.value

        if isinstance(node, BoolNode):
            return node.value

        if isinstance(node, VarAccessNode):
            return self.get_var(node.name)

        if isinstance(node, VarDeclareNode):
            value = self.eval(node.expr)
            self.define_var(node.name, value)
            return value

        if isinstance(node, VarSetNode):
            value = self.eval(node.expr)
            return self.set_var(node.name, value)

        if isinstance(node, UnaryOpNode):
            value = self.eval(node.node)
            op_type = node.op.typ

            if op_type == "PLUS":
                return +value
            if op_type == "MINUS":
                return -value
            if op_type == "IKKE":
                return not value

            raise RuntimeError(f"Ukjent unary operator: {op_type}")

        if isinstance(node, BinOpNode):
            left = self.eval(node.left)
            right = self.eval(node.right)
            op_type = node.op.typ

            if op_type == "PLUS":
                return left + right
            if op_type == "MINUS":
                return left - right
            if op_type == "MUL":
                return left * right
            if op_type == "DIV":
                return left / right
            if op_type == "PERCENT":
                return left % right
            if op_type == "EQ":
                return left == right
            if op_type == "NE":
                return left != right
            if op_type == "GT":
                return left > right
            if op_type == "LT":
                return left < right
            if op_type == "GTE":
                return left >= right
            if op_type == "LTE":
                return left <= right
            if op_type == "OG":
                return bool(left) and bool(right)
            if op_type == "ELLER":
                return bool(left) or bool(right)

            raise RuntimeError(f"Ukjent operator: {op_type}")

        if isinstance(node, IfExprNode):
            return self.eval(node.then_expr if self.eval(node.condition) else node.else_expr)

        if isinstance(node, ExprStmtNode):
            return self.eval(node.expr)

        if isinstance(node, ReturnNode):
            raise ReturnSignal(self.eval(node.expr))

        if isinstance(node, BlockNode):
            result = None
            for stmt in node.statements:
                result = self.eval(stmt)
            return result

        if isinstance(node, CallNode):
            return self.eval_call(node.name, node.args)

        if isinstance(node, ModuleCallNode):
            return self.eval_module_call(node.module_name, node.func_name, node.args)

        raise RuntimeError(f"Kan ikke evaluere node: {type(node).__name__}")

    def eval_call(self, name: str, args: list[Any]):
        values = [self.eval(arg) for arg in args]

        if name == "skriv":
            print(*values)
            return None

        if name == "assert":
            if not values[0]:
                raise AssertionError("Assert feilet")
            return None

        if name == "assert_eq":
            if values[0] != values[1]:
                raise AssertionError(f"assert_eq feilet: {values[0]!r} != {values[1]!r}")
            return None

        if name == "assert_ne":
            if values[0] == values[1]:
                raise AssertionError(f"assert_ne feilet: {values[0]!r} == {values[1]!r}")
            return None

        if name in self.functions:
            return self.call_user_function(name, values)

        raise RuntimeError(f"Ukjent funksjon: {name}")

    def eval_module_call(self, module_name: str, func_name: str, args: list[Any]):
        values = [self.eval(arg) for arg in args]

        if module_name == "math":
            if func_name == "pluss":
                return values[0] + values[1]
            if func_name == "minus":
                return values[0] - values[1]

        raise RuntimeError(f"Ukjent modulfunksjon: {module_name}.{func_name}")

    def call_user_function(self, name: str, args: list[Any]):
        fn = self.functions[name].node
        self.push_scope()
        try:
            for param, value in zip(fn.params, args):
                self.define_var(param.name, value)

            try:
                self.eval(fn.body)
            except ReturnSignal as signal:
                return signal.value
            return None
        finally:
            self.pop_scope()
