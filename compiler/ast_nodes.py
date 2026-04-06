

TYPE_INT = "heltall"
TYPE_BOOL = "bool"
TYPE_TEXT = "tekst"
TYPE_LIST_INT = "liste_heltall"
TYPE_LIST_TEXT = "liste_tekst"


class ImportNode:
    def __init__(self, module_name, alias=None):
        self.module_name = module_name
        self.alias = alias


class ProgramNode:
    def __init__(self, imports, functions):
        self.imports = imports
        self.functions = functions


class Param:
    def __init__(self, name, type_name):
        self.name = name
        self.type_name = type_name


class FunctionNode:
    def __init__(self, name, params, return_type, body, module_name=None):
        self.name = name
        self.params = params
        self.return_type = return_type
        self.body = body
        self.module_name = module_name


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
        self.elif_blocks = elif_blocks or []  # <-- NY
        self.else_block = else_block




class IfExprNode:
    def __init__(self, condition, then_expr, else_expr):
        self.condition = condition
        self.then_expr = then_expr
        self.else_expr = else_expr


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


class IndexNode:
    def __init__(self, target, index_expr):
        self.target = target
        self.index_expr = index_expr

class TestNode:
    def __init__(self, name, body):
        self.name = name
        self.body = body     

class ListNode:
    def __init__(self, elements):
        self.elements = elements


class IndexNode:
    def __init__(self, list_expr, index_expr):
        self.list_expr = list_expr
        self.index_expr = index_expr         