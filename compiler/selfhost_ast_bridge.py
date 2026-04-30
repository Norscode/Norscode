from __future__ import annotations

import json
from pathlib import Path
from typing import Any

from .ast_bridge import AST_FORMAT
from .selfhost_parser import parse_selfhost_program


class SelfhostAstBridgeError(RuntimeError):
    pass


BIN_OP_MAP = {
    '+': 'PLUS',
    '-': 'MINUS',
    '*': 'MUL',
    '/': 'DIV',
    '%': 'MOD',
    '==': 'EQ',
    '!=': 'NE',
    '<': 'LT',
    '>': 'GT',
    '<=': 'LTE',
    '>=': 'GTE',
    'og': 'OG',
    'eller': 'ELLER',
    'xor': 'XOR',
    'nand': 'NAND',
    'nor': 'NOR',
    'ekvivalent': 'EKVIVALENT',
    'impliserer': 'IMPLISERER',
    'impliseres_av': 'IMPLISERES_AV',
}

UNARY_OP_MAP = {
    '-': 'MINUS',
    '+': 'PLUS',
    'ikke': 'IKKE',
}

COMPOUND_TO_BINOP = {
    '+=': '+',
    '-=': '-',
    '*=': '*',
    '/=': '/',
    '%=': '%',
}


class BridgeContext:
    def __init__(self):
        self.temp_counter = 0

    def temp(self, prefix: str = '__sh_tmp') -> str:
        self.temp_counter += 1
        return f'{prefix}_{self.temp_counter}'


def _module_path(node: dict[str, Any]) -> str | None:
    kind = node.get('node')
    if kind == 'Name':
        return str(node.get('value'))
    if kind == 'Member':
        base = _module_path(node.get('target', {}))
        if base is None:
            return None
        return f"{base}.{node.get('name')}"
    return None


def expr_to_data(node: dict[str, Any]) -> dict[str, Any]:
    kind = node.get('node')
    if kind == 'Literal':
        lit = node.get('literal_type')
        if lit == 'heltall':
            return {'type': 'Number', 'value': int(node.get('value', 0))}
        if lit == 'tekst':
            return {'type': 'String', 'value': str(node.get('value', ''))}
        if lit == 'bool':
            return {'type': 'Bool', 'value': bool(node.get('value'))}
        raise SelfhostAstBridgeError(f'Ukjent literal-type: {lit}')
    if kind == 'Name':
        return {'type': 'VarAccess', 'name': node.get('value')}
    if kind == 'ListLiteral':
        return {'type': 'ListLiteral', 'items': [expr_to_data(item) for item in node.get('items', [])]}
    if kind == 'MapLiteral':
        return {
            'type': 'MapLiteral',
            'items': [
                {
                    'key': expr_to_data(entry.get('key', {})),
                    'value': expr_to_data(entry.get('value', {})),
                }
                for entry in node.get('items', [])
            ],
        }
    if kind == 'Index':
        return {
            'type': 'Index',
            'list_expr': expr_to_data(node.get('target', {})),
            'index_expr': expr_to_data(node.get('index', {})),
        }
    if kind == 'UnaryOp':
        op = node.get('op')
        return {'type': 'UnaryOp', 'op': UNARY_OP_MAP.get(op, str(op)), 'node': expr_to_data(node.get('value', {}))}
    if kind == 'BinaryOp':
        op = node.get('op')
        return {
            'type': 'BinOp',
            'op': BIN_OP_MAP.get(op, str(op)),
            'left': expr_to_data(node.get('left', {})),
            'right': expr_to_data(node.get('right', {})),
        }
    if kind == 'Call':
        callee = node.get('callee', {})
        args = [expr_to_data(arg) for arg in node.get('args', [])]
        if callee.get('node') == 'Name':
            return {'type': 'Call', 'name': callee.get('value'), 'args': args}
        if callee.get('node') == 'Member':
            module_name = _module_path(callee.get('target', {}))
            if module_name is not None:
                return {
                    'type': 'ModuleCall',
                    'module_name': module_name,
                    'func_name': callee.get('name'),
                    'args': args,
                }
        raise SelfhostAstBridgeError('Call støtter bare navn(...) eller modul.funksjon(...)')
    if kind == 'Member':
        module_name = _module_path(node)
        if module_name is not None:
            return {'type': 'VarAccess', 'name': module_name}
        raise SelfhostAstBridgeError('Member-uttrykk støttes ikke i selfhost AST-broen ennå')
    if kind == 'IfExpr':
        return {
            'type': 'IfExpr',
            'condition': expr_to_data(node.get('condition', {})),
            'then_expr': expr_to_data(node.get('then', {})),
            'else_expr': expr_to_data(node.get('else', {})),
        }
    raise SelfhostAstBridgeError(f'AST-broen støtter ikke uttrykk: {kind}')


def block_to_data(statements: list[dict[str, Any]], ctx: BridgeContext | None = None) -> dict[str, Any]:
    ctx = ctx or BridgeContext()
    out: list[dict[str, Any]] = []
    for stmt in statements:
        out.extend(stmt_to_data(stmt, ctx))
    return {'type': 'Block', 'statements': out}


def _lower_ifexpr_to_stmt(kind: str, name: str | None, value: dict[str, Any]) -> list[dict[str, Any]]:
    condition = expr_to_data(value.get('condition', {}))
    then_expr = value.get('then', {})
    else_expr = value.get('else', {})
    if kind == 'Return':
        return [{
            'type': 'If',
            'condition': condition,
            'then_block': {'type': 'Block', 'statements': [{'type': 'Return', 'expr': expr_to_data(then_expr)}]},
            'elif_blocks': [],
            'else_block': {'type': 'Block', 'statements': [{'type': 'Return', 'expr': expr_to_data(else_expr)}]},
        }]
    if kind == 'ExprStmt':
        return [{
            'type': 'If',
            'condition': condition,
            'then_block': {'type': 'Block', 'statements': [{'type': 'ExprStmt', 'expr': expr_to_data(then_expr)}]},
            'elif_blocks': [],
            'else_block': {'type': 'Block', 'statements': [{'type': 'ExprStmt', 'expr': expr_to_data(else_expr)}]},
        }]
    if kind == 'VarDeclare':
        return [{
            'type': 'If',
            'condition': condition,
            'then_block': {'type': 'Block', 'statements': [{'type': 'VarDeclare', 'name': name, 'var_type': None, 'expr': expr_to_data(then_expr)}]},
            'elif_blocks': [],
            'else_block': {'type': 'Block', 'statements': [{'type': 'VarDeclare', 'name': name, 'var_type': None, 'expr': expr_to_data(else_expr)}]},
        }]
    if kind == 'VarSet':
        return [{
            'type': 'If',
            'condition': condition,
            'then_block': {'type': 'Block', 'statements': [{'type': 'VarSet', 'name': name, 'expr': expr_to_data(then_expr)}]},
            'elif_blocks': [],
            'else_block': {'type': 'Block', 'statements': [{'type': 'VarSet', 'name': name, 'expr': expr_to_data(else_expr)}]},
        }]
    raise SelfhostAstBridgeError(f'Kan ikke senke IfExpr for statement-type: {kind}')


def _rewrite_continue_for_foreach(statements: list[dict[str, Any]], index_name: str) -> list[dict[str, Any]]:
    rewritten: list[dict[str, Any]] = []
    for stmt in statements:
        kind = stmt.get('node')
        if kind == 'Continue':
            rewritten.append({
                'node': 'Assign',
                'target': {'node': 'Name', 'value': index_name},
                'op': '=',
                'value': {
                    'node': 'BinaryOp',
                    'op': '+',
                    'left': {'node': 'Name', 'value': index_name},
                    'right': {'node': 'Literal', 'literal_type': 'heltall', 'value': 1},
                },
            })
            rewritten.append(stmt)
            continue
        if kind == 'If':
            cloned = dict(stmt)
            cloned['then'] = _rewrite_continue_for_foreach(stmt.get('then', []), index_name)
            if isinstance(stmt.get('else'), list):
                cloned['else'] = _rewrite_continue_for_foreach(stmt.get('else', []), index_name)
            rewritten.append(cloned)
            continue
        if kind in {'While', 'ForRange', 'ForEach'}:
            cloned = dict(stmt)
            if isinstance(stmt.get('body'), list):
                cloned['body'] = _rewrite_continue_for_foreach(stmt.get('body', []), index_name)
            rewritten.append(cloned)
            continue
        rewritten.append(stmt)
    return rewritten


def _flatten_if_chain(node: dict[str, Any], ctx: BridgeContext) -> tuple[list[dict[str, Any]], dict[str, Any] | None]:
    elif_blocks: list[dict[str, Any]] = []
    current = node
    while isinstance(current, dict) and current.get('node') == 'If':
        elif_blocks.append({
            'condition': expr_to_data(current.get('condition', {})),
            'block': block_to_data(current.get('then', []), ctx),
        })
        raw_else = current.get('else')
        if isinstance(raw_else, list) and len(raw_else) == 1 and raw_else[0].get('node') == 'If':
            current = raw_else[0]
            continue
        if isinstance(raw_else, list):
            return elif_blocks, block_to_data(raw_else, ctx)
        return elif_blocks, None
    return elif_blocks, None


def stmt_to_data(node: dict[str, Any], ctx: BridgeContext) -> list[dict[str, Any]]:
    kind = node.get('node')
    if kind == 'Let':
        value = node.get('value', {})
        declared_type = node.get('declared_type')
        if value.get('node') == 'IfExpr':
            return _lower_ifexpr_to_stmt('VarDeclare', node.get('name'), value)
        expr = expr_to_data(value)
        if isinstance(declared_type, str) and declared_type.startswith('liste_') and value.get('node') == 'Literal' and expr.get('type') != 'ListLiteral':
            expr = {'type': 'ListLiteral', 'items': [expr]}
        return [{
            'type': 'VarDeclare',
            'name': node.get('name'),
            'var_type': declared_type,
            'expr': expr,
        }]
    if kind == 'Return':
        value = node.get('value', {})
        if value.get('node') == 'IfExpr':
            return _lower_ifexpr_to_stmt('Return', None, value)
        return [{'type': 'Return', 'expr': expr_to_data(value)}]
    if kind == 'ExprStmt':
        value = node.get('value', {})
        if value.get('node') == 'IfExpr':
            return _lower_ifexpr_to_stmt('ExprStmt', None, value)
        return [{'type': 'ExprStmt', 'expr': expr_to_data(value)}]
    if kind == 'IfExprStmt':
        return _lower_ifexpr_to_stmt('ExprStmt', None, node.get('value', {}))
    if kind == 'If':
        raw_else = node.get('else')
        elif_blocks, else_block = _flatten_if_chain(raw_else[0], ctx) if isinstance(raw_else, list) and len(raw_else) == 1 and raw_else[0].get('node') == 'If' else ([], block_to_data(raw_else, ctx) if isinstance(raw_else, list) else None)
        return [{
            'type': 'If',
            'condition': expr_to_data(node.get('condition', {})),
            'then_block': block_to_data(node.get('then', []), ctx),
            'elif_blocks': elif_blocks,
            'else_block': else_block,
        }]
    if kind == 'While':
        return [{
            'type': 'While',
            'condition': expr_to_data(node.get('condition', {})),
            'body': block_to_data(node.get('body', []), ctx),
        }]
    if kind == 'ForRange':
        return [{
            'type': 'For',
            'name': node.get('name'),
            'start_expr': expr_to_data(node.get('start', {})),
            'end_expr': expr_to_data(node.get('end', {})),
            'step_expr': {'type': 'Number', 'value': 1},
            'body': block_to_data(node.get('body', []), ctx),
        }]
    if kind == 'ForEach':
        iter_name = ctx.temp('__sh_iter')
        index_name = ctx.temp('__sh_index')
        safe_body = _rewrite_continue_for_foreach(node.get('body', []), index_name)
        loop_body_nodes = [
            {'node': 'Let', 'name': node.get('name'), 'declared_type': None, 'value': {'node': 'Index', 'target': {'node': 'Name', 'value': iter_name}, 'index': {'node': 'Name', 'value': index_name}}},
            *safe_body,
            {'node': 'Assign', 'target': {'node': 'Name', 'value': index_name}, 'op': '=', 'value': {'node': 'BinaryOp', 'op': '+', 'left': {'node': 'Name', 'value': index_name}, 'right': {'node': 'Literal', 'literal_type': 'heltall', 'value': 1}}},
        ]
        return [
            {'type': 'VarDeclare', 'name': iter_name, 'var_type': None, 'expr': expr_to_data(node.get('iterable', {}))},
            {'type': 'VarDeclare', 'name': index_name, 'var_type': None, 'expr': {'type': 'Number', 'value': 0}},
            {
                'type': 'While',
                'condition': {
                    'type': 'BinOp',
                    'op': 'LT',
                    'left': {'type': 'VarAccess', 'name': index_name},
                    'right': {'type': 'Call', 'name': 'lengde', 'args': [{'type': 'VarAccess', 'name': iter_name}]},
                },
                'body': block_to_data(loop_body_nodes, ctx),
            },
        ]
    if kind == 'Break':
        return [{'type': 'Break'}]
    if kind == 'Continue':
        return [{'type': 'Continue'}]
    if kind == 'Assign':
        target = node.get('target', {})
        op = node.get('op')
        value = node.get('value', {})

        target_kind = target.get('node')
        target_name = _module_path(target) if target_kind in {'Name', 'Member'} else None

        if target_kind == 'Index':
            base_name = _module_path(target.get('target', {}))
            if base_name is None:
                raise SelfhostAstBridgeError('Index-assignment krever navn eller medlem som base')
            if op != '=':
                return [{
                    'type': 'IndexSet',
                    'target_name': base_name,
                    'index_expr': expr_to_data(target.get('index', {})),
                    'value_expr': {
                        'type': 'BinOp',
                        'op': BIN_OP_MAP[COMPOUND_TO_BINOP[op]],
                        'left': {
                            'type': 'Index',
                            'list_expr': {'type': 'VarAccess', 'name': base_name},
                            'index_expr': expr_to_data(target.get('index', {})),
                        },
                        'right': expr_to_data(value),
                    },
                }] if op in COMPOUND_TO_BINOP else (_ for _ in ()).throw(SelfhostAstBridgeError(f'Ukjent assignment-operator: {op}'))
            return [{
                'type': 'IndexSet',
                'target_name': base_name,
                'index_expr': expr_to_data(target.get('index', {})),
                'value_expr': expr_to_data(value),
            }]

        if target_name is None:
            raise SelfhostAstBridgeError('Assignment-target støttes ikke i selfhost AST-broen ennå')

        if value.get('node') == 'IfExpr' and op == '=':
            return _lower_ifexpr_to_stmt('VarSet', target_name, value)
        if op == '=':
            return [{'type': 'VarSet', 'name': target_name, 'expr': expr_to_data(value)}]
        if op in COMPOUND_TO_BINOP:
            return [{'type': 'VarSet', 'name': target_name, 'expr': {'type': 'BinOp', 'op': BIN_OP_MAP[COMPOUND_TO_BINOP[op]], 'left': {'type': 'VarAccess', 'name': target_name}, 'right': expr_to_data(value)}}]
        raise SelfhostAstBridgeError(f'Ukjent assignment-operator: {op}')
    raise SelfhostAstBridgeError(f'AST-broen støtter ikke statement: {kind}')


def program_payload_to_ast(payload: dict[str, Any]) -> dict[str, Any]:
    ctx = BridgeContext()
    return {
        'format': AST_FORMAT,
        'alias_map': {
            (item.get('alias') or str(item.get('module_name', '')).split('.')[-1]): item.get('module_name')
            for item in payload.get('imports', []) if item.get('module_name')
        },
        'imports': [
            {'module_name': item.get('module_name'), 'alias': item.get('alias')}
            for item in payload.get('imports', [])
        ],
        'functions': [
            {
                'name': fn.get('name'),
                'module_name': '__main__',
                'return_type': fn.get('return_type'),
                'params': [{'name': name, 'type_name': None} for name in fn.get('params', [])],
                'body': block_to_data(fn.get('body_ast', []), ctx),
            }
            for fn in payload.get('functions', [])
        ],
    }


def export_selfhost_ast(source_file: str, output: str | None = None) -> Path:
    source_path = Path(source_file).expanduser().resolve()
    if not source_path.exists():
        raise RuntimeError(f'Fant ikke kildefil: {source_path}')
    payload = parse_selfhost_program(source_path.read_text(encoding='utf-8'))
    ast_payload = program_payload_to_ast(payload)
    out_path = Path(output).expanduser().resolve() if output else source_path.with_suffix('.shast.json')
    out_path.write_text(json.dumps(ast_payload, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    return out_path
