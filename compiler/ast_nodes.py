TYPE_INT = "heltall"
TYPE_BOOL = "bool"
TYPE_TEXT = "tekst"
TYPE_LIST_INT = "liste_heltall"
TYPE_LIST_TEXT = "liste_tekst"
TYPE_MAP_INT = "ordbok_heltall"
TYPE_MAP_TEXT = "ordbok_tekst"
TYPE_MAP_BOOL = "ordbok_bool"
TYPE_LAMBDA = "funksjon"
TYPE_MAP_PREFIX = "ordbok_"


def map_type_name(inner_type):
    return f"{TYPE_MAP_PREFIX}{inner_type}"


def is_map_type_name(type_name):
    return isinstance(type_name, str) and type_name.startswith(TYPE_MAP_PREFIX)


class ImportNode:
    def __init__(self, module_name, alias=None):
        self.module_name = module_name
        self.alias = alias


class ProgramNode:
    def __init__(self, imports, functions, tests=None):
        self.imports = imports
        self.functions = functions
        self.tests = tests or []


class Param:
    def __init__(self, name, type_name):
        self.name = name
        self.type_name = type_name


class FunctionNode:
    def __init__(self, name, params, return_type, body, module_name=None, is_async=False):
        self.name = name
        self.params = params
        self.return_type = return_type
        self.body = body
        self.module_name = module_name
        self.is_async = is_async


class BlockNode:
    def __init__(self, statements):
        self.statements = statements


class VarDeclareNode:
    def __init__(self, name, var_type, expr):
        self.name = name
        self.var_type = var_type
        self.expr = expr


class VarSetNode:
    def __init__(self, name, expr):
        self.name = name
        self.expr = expr


class IndexSetNode:
    def __init__(self, target_name, index_expr, value_expr):
        self.target_name = target_name
        self.index_expr = index_expr
        self.value_expr = value_expr


class PrintNode:
    def __init__(self, expr):
        self.expr = expr


class IfNode:
    def __init__(self, condition, then_block, elif_blocks=None, else_block=None):
        self.condition = condition
        self.then_block = then_block
        self.elif_blocks = elif_blocks or []
        self.else_block = else_block


class IfExprNode:
    def __init__(self, condition, then_expr, else_expr):
        self.condition = condition
        self.then_expr = then_expr
        self.else_expr = else_expr


class MatchCaseNode:
    def __init__(self, pattern, body, wildcard=False):
        self.pattern = pattern
        self.body = body
        self.wildcard = wildcard


class MatchNode:
    def __init__(self, subject, cases, else_block=None):
        self.subject = subject
        self.cases = cases
        self.else_block = else_block


class AwaitNode:
    def __init__(self, expr):
        self.expr = expr


class LambdaNode:
    def __init__(self, params, body):
        self.params = params
        self.body = body


class WhileNode:
    def __init__(self, condition, body):
        self.condition = condition
        self.body = body


class ForNode:
    def __init__(self, name, start_expr, end_expr, step_expr, body):
        self.name = name
        self.start_expr = start_expr
        self.end_expr = end_expr
        self.step_expr = step_expr
        self.body = body


class ForEachNode:
    def __init__(self, item_name, list_expr, body):
        self.item_name = item_name
        self.list_expr = list_expr
        self.body = body


class ReturnNode:
    def __init__(self, expr):
        self.expr = expr


class BreakNode:
    pass


class ContinueNode:
    pass


class ExprStmtNode:
    def __init__(self, expr):
        self.expr = expr


class ThrowNode:
    def __init__(self, expr):
        self.expr = expr


class TryCatchNode:
    def __init__(self, try_block, catch_var_name, catch_block):
        self.try_block = try_block
        self.catch_var_name = catch_var_name
        self.catch_block = catch_block


class NumberNode:
    def __init__(self, value):
        self.value = value


class StringNode:
    def __init__(self, value):
        self.value = value


class BoolNode:
    def __init__(self, value):
        self.value = value


class VarAccessNode:
    def __init__(self, name):
        self.name = name


class BinOpNode:
    def __init__(self, left, op, right):
        self.left = left
        self.op = op
        self.right = right


class UnaryOpNode:
    def __init__(self, op, node):
        self.op = op
        self.node = node


class CallNode:
    def __init__(self, name, args):
        self.name = name
        self.args = args


class ModuleCallNode:
    def __init__(self, module_name, func_name, args):
        self.module_name = module_name
        self.func_name = func_name
        self.args = args


class ListLiteralNode:
    def __init__(self, items):
        self.items = items


class ListComprehensionNode:
    def __init__(self, item_name, source_expr, item_expr, condition_expr=None):
        self.item_name = item_name
        self.source_expr = source_expr
        self.item_expr = item_expr
        self.condition_expr = condition_expr


class MapLiteralNode:
    def __init__(self, items):
        self.items = items


class StructLiteralNode:
    def __init__(self, fields):
        self.fields = fields


class IndexNode:
    def __init__(self, list_expr, index_expr):
        self.list_expr = list_expr
        self.index_expr = index_expr


class SliceNode:
    def __init__(self, target, start_expr=None, end_expr=None):
        self.target = target
        self.start_expr = start_expr
        self.end_expr = end_expr


class FieldAccessNode:
    def __init__(self, target, field):
        self.target = target
        self.field = field


class TestNode:
    def __init__(self, name, body):
        self.name = name
        self.body = body
