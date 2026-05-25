"""Shared lint helpers for CLI commands."""

from __future__ import annotations

from types import SimpleNamespace

from norcode.compiler_service import load_program

from compiler.ast_nodes import (
    AwaitNode,
    BinOpNode,
    BreakNode,
    CallNode,
    ContinueNode,
    ExprStmtNode,
    FieldAccessNode,
    ForEachNode,
    ForNode,
    IfExprNode,
    IfNode,
    IndexNode,
    IndexSetNode,
    ListLiteralNode,
    MapLiteralNode,
    ModuleCallNode,
    PrintNode,
    ReturnNode,
    SliceNode,
    StructLiteralNode,
    ThrowNode,
    TryCatchNode,
    UnaryOpNode,
    VarDeclareNode,
    VarSetNode,
    WhileNode,
)


def _collect_lint_issues(program, alias_map: dict[str, str] | None = None):
    issues: list[dict] = []

    imports = list(getattr(program, "imports", []))
    if not imports and alias_map:
        imports = [SimpleNamespace(module_name=module_name, alias=alias) for alias, module_name in alias_map.items()]
    used_modules: set[str] = set()
    seen_import_keys: set[str] = set()

    for imp in imports:
        key = imp.alias or imp.module_name
        if key in seen_import_keys:
            issues.append(
                {
                    "severity": "warning",
                    "code": "duplicate-import",
                    "location": key,
                    "message": f"Dobbel import: {key}",
                }
            )
        else:
            seen_import_keys.add(key)

    def visit_expr(expr):
        if expr is None:
            return
        if isinstance(expr, ModuleCallNode):
            used_modules.add(expr.module_name)
            for arg in expr.args:
                visit_expr(arg)
            return
        if isinstance(expr, CallNode):
            for arg in expr.args:
                visit_expr(arg)
            return
        if isinstance(expr, IfExprNode):
            visit_expr(expr.condition)
            visit_expr(expr.then_expr)
            visit_expr(expr.else_expr)
            return
        if isinstance(expr, AwaitNode):
            visit_expr(expr.expr)
            return
        if isinstance(expr, UnaryOpNode):
            visit_expr(expr.node)
            return
        if isinstance(expr, BinOpNode):
            visit_expr(expr.left)
            visit_expr(expr.right)
            return
        if isinstance(expr, IndexNode):
            visit_expr(expr.list_expr)
            visit_expr(expr.index_expr)
            return
        if isinstance(expr, SliceNode):
            visit_expr(expr.target)
            visit_expr(expr.start_expr)
            visit_expr(expr.end_expr)
            return
        if isinstance(expr, FieldAccessNode):
            visit_expr(expr.target)
            return
        if isinstance(expr, ListLiteralNode):
            for item in expr.items:
                visit_expr(item)
            return
        if isinstance(expr, MapLiteralNode):
            for key_expr, value_expr in expr.items:
                visit_expr(key_expr)
                visit_expr(value_expr)
            return
        if isinstance(expr, StructLiteralNode):
            for _field_name, value_expr in expr.fields:
                visit_expr(value_expr)
            return

    def visit_stmt(stmt, function_name: str, in_loop: bool):
        if isinstance(stmt, VarDeclareNode):
            visit_expr(stmt.expr)
            return False
        if isinstance(stmt, VarSetNode):
            visit_expr(stmt.expr)
            return False
        if isinstance(stmt, IndexSetNode):
            visit_expr(stmt.index_expr)
            visit_expr(stmt.value_expr)
            return False
        if isinstance(stmt, PrintNode):
            visit_expr(stmt.expr)
            return False
        if isinstance(stmt, IfNode):
            visit_expr(stmt.condition)
            visit_block(stmt.then_block, function_name, in_loop)
            for elif_cond, elif_block in getattr(stmt, "elif_blocks", []):
                visit_expr(elif_cond)
                visit_block(elif_block, function_name, in_loop)
            if stmt.else_block:
                visit_block(stmt.else_block, function_name, in_loop)
            return False
        if isinstance(stmt, IfExprNode):
            visit_expr(stmt.condition)
            visit_expr(stmt.then_expr)
            visit_expr(stmt.else_expr)
            return False
        if isinstance(stmt, WhileNode):
            visit_expr(stmt.condition)
            visit_block(stmt.body, function_name, True)
            return False
        if isinstance(stmt, ForNode):
            visit_expr(stmt.start_expr)
            visit_expr(stmt.end_expr)
            visit_expr(stmt.step_expr)
            visit_block(stmt.body, function_name, True)
            return False
        if isinstance(stmt, ForEachNode):
            visit_expr(stmt.list_expr)
            visit_block(stmt.body, function_name, True)
            return False
        if isinstance(stmt, ReturnNode):
            visit_expr(stmt.expr)
            return True
        if isinstance(stmt, ThrowNode):
            visit_expr(stmt.expr)
            return True
        if isinstance(stmt, TryCatchNode):
            visit_block(stmt.try_block, function_name, in_loop)
            visit_block(stmt.catch_block, function_name, in_loop)
            return False
        if isinstance(stmt, ExprStmtNode):
            visit_expr(stmt.expr)
            return False
        if isinstance(stmt, (BreakNode, ContinueNode)):
            return True
        return False

    def visit_block(block, function_name: str, in_loop: bool):
        dead = False
        for stmt in getattr(block, "statements", []):
            if dead:
                issues.append(
                    {
                        "severity": "warning",
                        "code": "unreachable-code",
                        "location": function_name,
                        "message": f"Uoppnåelig kode etter terminal statement i funksjon '{function_name}'",
                    }
                )
                break
            dead = visit_stmt(stmt, function_name, in_loop)

    for fn in getattr(program, "functions", []):
        visit_block(fn.body, fn.name, False)

    for test in getattr(program, "tests", []):
        visit_block(test.body, test.name, False)

    for imp in imports:
        key = imp.alias or imp.module_name
        if key not in used_modules:
            issues.append(
                {
                    "severity": "warning",
                    "code": "unused-import",
                    "location": key,
                    "message": f"Ubrukt import: {imp.module_name}" + (f" som {imp.alias}" if imp.alias else ""),
                }
            )

    return issues


def lint_program(source_file: str):
    source_path, program, alias_map = load_program(source_file)
    issues = _collect_lint_issues(program, alias_map=alias_map)
    return {
        "source": str(source_path),
        "alias_map": alias_map,
        "issues": issues,
        "success": len(issues) == 0,
    }


def print_lint_result(result, verbose: bool = False):
    issues = result.get("issues", [])
    status = "OK" if not issues else "FEIL"
    print(f"{status}: {result['source']} ({len(issues)} funn)")
    if verbose or issues:
        for issue in issues:
            location = issue.get("location")
            prefix = f"{issue.get('severity', 'warning').upper()} {issue.get('code', 'lint')}"
            if location:
                print(f"- {prefix}: {location} -> {issue.get('message', '')}")
            else:
                print(f"- {prefix}: {issue.get('message', '')}")


def summarize_lint_results(result: dict) -> dict:
    issues = result.get("issues", [])
    warnings = sum(1 for issue in issues if issue.get("severity") == "warning")
    errors = sum(1 for issue in issues if issue.get("severity") == "error")
    return {
        "source": result.get("source"),
        "warnings": warnings,
        "errors": errors,
        "total": len(issues),
        "success": len(issues) == 0,
    }
