from __future__ import annotations

import json
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
    ListLiteralNode,
    ModuleCallNode,
    NumberNode,
    Param,
    PrintNode,
    ProgramNode,
    ReturnNode,
    StringNode,
    UnaryOpNode,
    VarAccessNode,
    VarDeclareNode,
    VarSetNode,
    IndexSetNode,
    WhileNode,
)
from .lexer import Token
from .loader import ModuleLoader
from .semantic import SemanticAnalyzer


class AstBridgeError(RuntimeError):
    pass


AST_FORMAT = "norscode-ast-v1"


def _tok(typ: str, value: Any = None) -> Token:
    return Token(typ, value if value is not None else typ, 0, 0)


def expr_to_data(node: Any) -> dict[str, Any]:
    if isinstance(node, NumberNode):
        return {"type": "Number", "value": node.value}
    if isinstance(node, StringNode):
        return {"type": "String", "value": node.value}
    if isinstance(node, BoolNode):
        return {"type": "Bool", "value": node.value}
    if isinstance(node, VarAccessNode):
        return {"type": "VarAccess", "name": node.name}
    if isinstance(node, ListLiteralNode):
        return {"type": "ListLiteral", "items": [expr_to_data(item) for item in node.items]}
    if isinstance(node, IndexNode):
        return {
            "type": "Index",
            "list_expr": expr_to_data(node.list_expr),
            "index_expr": expr_to_data(node.index_expr),
        }
    if isinstance(node, UnaryOpNode):
        return {
            "type": "UnaryOp",
            "op": node.op.typ,
            "node": expr_to_data(node.node),
        }
    if isinstance(node, BinOpNode):
        return {
            "type": "BinOp",
            "op": node.op.typ,
            "left": expr_to_data(node.left),
            "right": expr_to_data(node.right),
        }
    if isinstance(node, CallNode):
        return {
            "type": "Call",
            "name": node.name,
            "args": [expr_to_data(arg) for arg in node.args],
        }
    if isinstance(node, ModuleCallNode):
        return {
            "type": "ModuleCall",
            "module_name": node.module_name,
            "func_name": node.func_name,
            "args": [expr_to_data(arg) for arg in node.args],
        }
    if isinstance(node, IfExprNode):
        return {
            "type": "IfExpr",
            "condition": expr_to_data(node.condition),
            "then_expr": expr_to_data(node.then_expr),
            "else_expr": expr_to_data(node.else_expr),
        }
    raise AstBridgeError(f"AST-eksport støtter ikke uttrykk: {type(node).__name__}")



def stmt_to_data(node: Any) -> dict[str, Any]:
    if isinstance(node, VarDeclareNode):
        return {
            "type": "VarDeclare",
            "name": node.name,
            "var_type": node.var_type,
            "expr": expr_to_data(node.expr),
        }
    if isinstance(node, VarSetNode):
        return {"type": "VarSet", "name": node.name, "expr": expr_to_data(node.expr)}
    if isinstance(node, IndexSetNode):
        return {"type": "IndexSet", "target_name": node.target_name, "index_expr": expr_to_data(node.index_expr), "value_expr": expr_to_data(node.value_expr)}
    if isinstance(node, ExprStmtNode):
        return {"type": "ExprStmt", "expr": expr_to_data(node.expr)}
    if isinstance(node, PrintNode):
        return {"type": "Print", "expr": expr_to_data(node.expr)}
    if isinstance(node, ReturnNode):
        return {"type": "Return", "expr": expr_to_data(node.expr)}
    if isinstance(node, IfNode):
        return {
            "type": "If",
            "condition": expr_to_data(node.condition),
            "then_block": block_to_data(node.then_block),
            "elif_blocks": [
                {"condition": expr_to_data(cond), "block": block_to_data(block)}
                for cond, block in getattr(node, "elif_blocks", [])
            ],
            "else_block": block_to_data(node.else_block) if node.else_block else None,
        }
    if isinstance(node, WhileNode):
        return {"type": "While", "condition": expr_to_data(node.condition), "body": block_to_data(node.body)}
    if isinstance(node, ForNode):
        return {
            "type": "For",
            "name": node.name,
            "start_expr": expr_to_data(node.start_expr),
            "end_expr": expr_to_data(node.end_expr),
            "step_expr": expr_to_data(node.step_expr),
            "body": block_to_data(node.body),
        }
    if isinstance(node, BreakNode):
        return {"type": "Break"}
    if isinstance(node, ContinueNode):
        return {"type": "Continue"}
    raise AstBridgeError(f"AST-eksport støtter ikke statement: {type(node).__name__}")



def block_to_data(block: BlockNode | None) -> dict[str, Any] | None:
    if block is None:
        return None
    return {"type": "Block", "statements": [stmt_to_data(stmt) for stmt in block.statements]}



def program_to_data(program: ProgramNode, alias_map: dict[str, str] | None = None) -> dict[str, Any]:
    return {
        "format": AST_FORMAT,
        "alias_map": alias_map or {},
        "imports": [
            {"module_name": imp.module_name, "alias": imp.alias}
            for imp in getattr(program, "imports", [])
        ],
        "functions": [
            {
                "name": fn.name,
                "module_name": getattr(fn, "module_name", None),
                "return_type": fn.return_type,
                "params": [{"name": p.name, "type_name": p.type_name} for p in getattr(fn, "params", [])],
                "body": block_to_data(fn.body),
            }
            for fn in getattr(program, "functions", [])
        ],
    }



def expr_from_data(data: dict[str, Any]) -> Any:
    typ = data.get("type")
    if typ == "Number":
        return NumberNode(data.get("value"))
    if typ == "String":
        return StringNode(data.get("value"))
    if typ == "Bool":
        return BoolNode(bool(data.get("value")))
    if typ == "VarAccess":
        return VarAccessNode(data.get("name"))
    if typ == "ListLiteral":
        return ListLiteralNode([expr_from_data(item) for item in data.get("items", [])])
    if typ == "Index":
        return IndexNode(expr_from_data(data.get("list_expr", {})), expr_from_data(data.get("index_expr", {})))
    if typ == "UnaryOp":
        return UnaryOpNode(_tok(str(data.get("op"))), expr_from_data(data.get("node", {})))
    if typ == "BinOp":
        return BinOpNode(
            expr_from_data(data.get("left", {})),
            _tok(str(data.get("op"))),
            expr_from_data(data.get("right", {})),
        )
    if typ == "Call":
        return CallNode(data.get("name"), [expr_from_data(arg) for arg in data.get("args", [])])
    if typ == "ModuleCall":
        return ModuleCallNode(
            data.get("module_name"),
            data.get("func_name"),
            [expr_from_data(arg) for arg in data.get("args", [])],
        )
    if typ == "IfExpr":
        return IfExprNode(
            expr_from_data(data.get("condition", {})),
            expr_from_data(data.get("then_expr", {})),
            expr_from_data(data.get("else_expr", {})),
        )
    raise AstBridgeError(f"AST-import støtter ikke uttrykk-type: {typ}")



def stmt_from_data(data: dict[str, Any]) -> Any:
    typ = data.get("type")
    if typ == "VarDeclare":
        return VarDeclareNode(data.get("name"), data.get("var_type"), expr_from_data(data.get("expr", {})))
    if typ == "VarSet":
        return VarSetNode(data.get("name"), expr_from_data(data.get("expr", {})))
    if typ == "IndexSet":
        return IndexSetNode(data.get("target_name"), expr_from_data(data.get("index_expr", {})), expr_from_data(data.get("value_expr", {})))
    if typ == "ExprStmt":
        return ExprStmtNode(expr_from_data(data.get("expr", {})))
    if typ == "Print":
        return PrintNode(expr_from_data(data.get("expr", {})))
    if typ == "Return":
        return ReturnNode(expr_from_data(data.get("expr", {})))
    if typ == "If":
        return IfNode(
            expr_from_data(data.get("condition", {})),
            block_from_data(data.get("then_block")),
            [
                (expr_from_data(item.get("condition", {})), block_from_data(item.get("block")))
                for item in data.get("elif_blocks", [])
            ],
            block_from_data(data.get("else_block")) if data.get("else_block") else None,
        )
    if typ == "While":
        return WhileNode(expr_from_data(data.get("condition", {})), block_from_data(data.get("body")))
    if typ == "For":
        return ForNode(
            data.get("name"),
            expr_from_data(data.get("start_expr", {})),
            expr_from_data(data.get("end_expr", {})),
            expr_from_data(data.get("step_expr", {})),
            block_from_data(data.get("body")),
        )
    if typ == "Break":
        return BreakNode()
    if typ == "Continue":
        return ContinueNode()
    raise AstBridgeError(f"AST-import støtter ikke statement-type: {typ}")



def block_from_data(data: dict[str, Any] | None) -> BlockNode:
    if not isinstance(data, dict) or data.get("type") != "Block":
        raise AstBridgeError("Ugyldig Block-data i AST-json")
    return BlockNode([stmt_from_data(stmt) for stmt in data.get("statements", [])])



def program_from_data(data: dict[str, Any]) -> tuple[ProgramNode, dict[str, str]]:
    if data.get("format") != AST_FORMAT:
        raise AstBridgeError(f"Ugyldig AST-format: {data.get('format')}")
    imports = [ImportNode(item.get("module_name"), item.get("alias")) for item in data.get("imports", [])]
    functions = []
    for item in data.get("functions", []):
        params = [Param(p.get("name"), p.get("type_name")) for p in item.get("params", [])]
        functions.append(
            FunctionNode(
                item.get("name"),
                params,
                item.get("return_type"),
                block_from_data(item.get("body")),
                module_name=item.get("module_name"),
            )
        )
    return ProgramNode(imports, functions), dict(data.get("alias_map") or {})



def load_source_as_program(source_file: str) -> tuple[Path, ProgramNode, dict[str, str]]:
    source_path = Path(source_file).expanduser().resolve()
    loader = ModuleLoader(source_path.parent)
    loaded = loader.load_entry_file(source_path.name)
    if isinstance(loaded, tuple):
        program, alias_map = loaded
    else:
        program, alias_map = loaded, {}
    analyzer = SemanticAnalyzer(alias_map=alias_map)
    analyzer.analyze(program)
    return source_path, program, alias_map



def export_ast(source_file: str, output: str | None = None) -> Path:
    source_path, program, alias_map = load_source_as_program(source_file)
    payload = program_to_data(program, alias_map=alias_map)
    out_path = Path(output).expanduser().resolve() if output else source_path.with_suffix('.nast.json')
    out_path.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding='utf-8')
    return out_path



def read_ast(ast_file: str) -> tuple[ProgramNode, dict[str, str]]:
    payload = json.loads(Path(ast_file).expanduser().read_text(encoding='utf-8'))
    return program_from_data(payload)
