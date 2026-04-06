from .ast_nodes import *


class FunctionSymbol:
    def __init__(self, name, params, return_type, builtin=False, module_name=None):
        self.name = name
        self.params = params
        self.return_type = return_type
        self.builtin = builtin
        self.module_name = module_name


class SemanticAnalyzer:
    def __init__(self, alias_map=None):
        self.alias_map = alias_map or {}
        self.current_function = None
        self.current_function_full_name = None
        self.functions = {
            "tekst_fra_heltall": FunctionSymbol("tekst_fra_heltall", [TYPE_INT], TYPE_TEXT, True),
            "tekst_fra_bool": FunctionSymbol("tekst_fra_bool", [TYPE_BOOL], TYPE_TEXT, True),
            "heltall_fra_tekst": FunctionSymbol("heltall_fra_tekst", [TYPE_TEXT], TYPE_INT, True),
            "del_ord": FunctionSymbol("del_ord", [TYPE_TEXT], TYPE_LIST_TEXT, True),
            "tokeniser_enkel": FunctionSymbol("tokeniser_enkel", [TYPE_TEXT], TYPE_LIST_TEXT, True),
            "tokeniser_uttrykk": FunctionSymbol("tokeniser_uttrykk", [TYPE_TEXT], TYPE_LIST_TEXT, True),
            "les_input": FunctionSymbol("les_input", [TYPE_TEXT], TYPE_TEXT, True),
            "lengde": FunctionSymbol("lengde", [None], TYPE_INT, True),
            "legg_til": FunctionSymbol("legg_til", [None, None], TYPE_INT, True),
            "pop_siste": FunctionSymbol("pop_siste", [None], None, True),
            "fjern_indeks": FunctionSymbol("fjern_indeks", [None, TYPE_INT], TYPE_INT, True),
            "sett_inn": FunctionSymbol("sett_inn", [None, TYPE_INT, None], TYPE_INT, True),
            "assert": FunctionSymbol("assert", [TYPE_BOOL], TYPE_INT, True),
            "assert_eq": FunctionSymbol("assert_eq", [None, None], TYPE_INT, True),
            "assert_ne": FunctionSymbol("assert_ne", [None, None], TYPE_INT, True),
        }

    def function_display_name(self, fn):
        return (
            f"{fn.module_name}.{fn.name}"
            if getattr(fn, "module_name", None) and fn.module_name != "__main__"
            else fn.name
        )

    def resolve_function_symbol(self, name):
        if name in self.functions:
            return self.functions[name], name

        current_module = getattr(self.current_function, "module_name", None)
        if current_module and current_module != "__main__":
            scoped_name = f"{current_module}.{name}"
            if scoped_name in self.functions:
                return self.functions[scoped_name], scoped_name

        return None, name

    def error(self, message):
        if self.current_function_full_name:
            raise RuntimeError(f"{message} [i funksjon {self.current_function_full_name}]")
        raise RuntimeError(message)

    def is_empty_list_expr(self, expr):
        return isinstance(expr, ListLiteralNode) and not expr.items

    def analyze(self, tree):
        for fn in tree.functions:
            full_name = (
                f"{fn.module_name}.{fn.name}"
                if getattr(fn, "module_name", None) and fn.module_name != "__main__"
                else fn.name
            )

            if full_name in self.functions:
                raise RuntimeError(f"Funksjonen '{full_name}' er definert flere ganger eller er reservert")

            self.functions[full_name] = FunctionSymbol(
                full_name,
                [p.type_name for p in fn.params],
                fn.return_type,
                False,
                getattr(fn, "module_name", None),
            )

        if "start" not in self.functions:
            raise RuntimeError("Fant ikke funksjonen start()")

        for fn in tree.functions:
            self.check_function(fn)

    def check_function(self, fn):
        previous_function = self.current_function
        previous_function_full_name = self.current_function_full_name
        self.current_function = fn
        self.current_function_full_name = self.function_display_name(fn)
        try:
            scope = {p.name: p.type_name for p in fn.params}
            saw_return = self.check_block(fn.body, scope, fn.return_type, False)
            if not saw_return and fn.name != "start":
                self.error(f"Funksjonen '{fn.name}' må returnere {fn.return_type}")
        finally:
            self.current_function = previous_function
            self.current_function_full_name = previous_function_full_name

    def check_block(self, block, scope, expected_return_type, in_loop):
        local_scope = dict(scope)
        returned = False
        for stmt in block.statements:
            stmt_returns = self.check_stmt(stmt, local_scope, expected_return_type, in_loop)
            if stmt_returns:
                returned = True
                break
        return returned

    def check_stmt(self, stmt, scope, expected_return_type, in_loop):
        if isinstance(stmt, VarDeclareNode):
            is_empty_list = self.is_empty_list_expr(stmt.expr)
            expr_type = self.check_expr(stmt.expr, scope)
            if stmt.var_type is None:
                scope[stmt.name] = expr_type
            else:
                if is_empty_list and stmt.var_type in (TYPE_LIST_INT, TYPE_LIST_TEXT):
                    scope[stmt.name] = stmt.var_type
                    return False
                self.ensure_assignable(stmt.var_type, expr_type, f"variabel '{stmt.name}'")
                scope[stmt.name] = stmt.var_type
            return False

        if isinstance(stmt, VarSetNode):
            if stmt.name not in scope:
                self.error(f"Variabel '{stmt.name}' er ikke definert")
            is_empty_list = self.is_empty_list_expr(stmt.expr)
            expr_type = self.check_expr(stmt.expr, scope)
            if is_empty_list and scope[stmt.name] in (TYPE_LIST_INT, TYPE_LIST_TEXT):
                return False
            self.ensure_assignable(scope[stmt.name], expr_type, f"variabel '{stmt.name}'")
            return False

        if isinstance(stmt, IndexSetNode):
            target_type = scope.get(stmt.target_name)
            if target_type is None:
                self.error(f"Variabel '{stmt.target_name}' er ikke definert")
            idx_type = self.check_expr(stmt.index_expr, scope)
            if idx_type != TYPE_INT:
                self.error("Listeindeks må være heltall")
            value_type = self.check_expr(stmt.value_expr, scope)
            if target_type == TYPE_LIST_INT and value_type != TYPE_INT:
                self.error("liste_heltall kan bare få heltall")
            if target_type == TYPE_LIST_TEXT and value_type != TYPE_TEXT:
                self.error("liste_tekst kan bare få tekst")
            if target_type not in (TYPE_LIST_INT, TYPE_LIST_TEXT):
                self.error("Indeks-tilordning krever liste")
            return False

        if isinstance(stmt, PrintNode):
            self.check_expr(stmt.expr, scope)
            return False

        if isinstance(stmt, IfNode):
            cond_type = self.check_expr(stmt.condition, scope)
            self.ensure_bool(cond_type, "hvis-betingelse")
            self.check_block(stmt.then_block, dict(scope), expected_return_type, in_loop)
            if stmt.else_block:
                self.check_block(stmt.else_block, dict(scope), expected_return_type, in_loop)
            return False

        if isinstance(stmt, IfExprNode):
            cond_type = self.check_expr(stmt.condition, scope)
            self.ensure_bool(cond_type, "hvis-betingelse")
            then_type = self.check_expr(stmt.then_expr, scope)
            else_type = self.check_expr(stmt.else_expr, scope)
            self.ensure_assignable(then_type, else_type, "hvis-uttrykk")
            return False

        if isinstance(stmt, WhileNode):
            cond_type = self.check_expr(stmt.condition, scope)
            self.ensure_bool(cond_type, "mens-betingelse")
            self.check_block(stmt.body, dict(scope), expected_return_type, True)
            return False

        if isinstance(stmt, ForNode):
            st = self.check_expr(stmt.start_expr, scope)
            et = self.check_expr(stmt.end_expr, scope)
            pt = self.check_expr(stmt.step_expr, scope)
            if st != TYPE_INT or et != TYPE_INT or pt != TYPE_INT:
                self.error("for krever heltall i start, til og steg")
            loop_scope = dict(scope)
            loop_scope[stmt.name] = TYPE_INT
            self.check_block(stmt.body, loop_scope, expected_return_type, True)
            return False

        if isinstance(stmt, ForEachNode):
            list_type = self.check_expr(stmt.list_expr, scope)
            loop_scope = dict(scope)
            if list_type == TYPE_LIST_INT:
                loop_scope[stmt.item_name] = TYPE_INT
            elif list_type == TYPE_LIST_TEXT:
                loop_scope[stmt.item_name] = TYPE_TEXT
            else:
                self.error("for ... i ... krever liste")
            self.check_block(stmt.body, loop_scope, expected_return_type, True)
            return False

        if isinstance(stmt, ReturnNode):
            expr_type = self.check_expr(stmt.expr, scope)
            self.ensure_assignable(expected_return_type, expr_type, "returverdi", stmt.expr)
            return True

        if isinstance(stmt, BreakNode):
            if not in_loop:
                self.error("bryt kan bare brukes inne i en løkke")
            return False

        if isinstance(stmt, ContinueNode):
            if not in_loop:
                self.error("fortsett kan bare brukes inne i en løkke")
            return False

        if isinstance(stmt, ExprStmtNode):
            self.check_expr(stmt.expr, scope)
            return False

        self.error(f"Ukjent statement-type: {type(stmt).__name__}")

    def check_expr(self, expr, scope):
        if isinstance(expr, NumberNode):
            return TYPE_INT

        if isinstance(expr, StringNode):
            return TYPE_TEXT

        if isinstance(expr, BoolNode):
            return TYPE_BOOL

        if isinstance(expr, ListLiteralNode):
            if not expr.items:
                return TYPE_LIST_INT
            item_types = [self.check_expr(item, scope) for item in expr.items]
            first = item_types[0]
            for t in item_types[1:]:
                if t != first:
                    self.error("Alle elementer i en liste må ha samme type")
            if first == TYPE_INT:
                return TYPE_LIST_INT
            if first == TYPE_TEXT:
                return TYPE_LIST_TEXT
            self.error("Lister støtter bare heltall eller tekst")

        if isinstance(expr, VarAccessNode):
            if expr.name in scope:
                return scope[expr.name]
            if expr.name in self.functions:
                self.error(f"Funksjonen '{expr.name}' må kalles med ()")
            self.error(f"Ukjent navn: {expr.name}")

        if isinstance(expr, IndexNode):
            target_expr = getattr(expr, "target", getattr(expr, "list_expr", None))
            index_expr = getattr(expr, "index_expr", None)
            target_type = self.check_expr(target_expr, scope)
            idx_type = self.check_expr(index_expr, scope)
            if idx_type != TYPE_INT:
                self.error("Listeindeks må være heltall")
            if target_type == TYPE_LIST_INT:
                return TYPE_INT
            if target_type == TYPE_LIST_TEXT:
                return TYPE_TEXT
            self.error("[] kan bare brukes på lister")

        if isinstance(expr, UnaryOpNode):
            inner = self.check_expr(expr.node, scope)
            if expr.op.typ in ("PLUS", "MINUS"):
                if inner != TYPE_INT:
                    self.error("Unary + og - krever heltall")
                return TYPE_INT
            if expr.op.typ == "IKKE":
                if inner != TYPE_BOOL:
                    self.error("ikke krever bool")
                return TYPE_BOOL

        if isinstance(expr, BinOpNode):
            left = self.check_expr(expr.left, scope)
            right = self.check_expr(expr.right, scope)
            if expr.op.typ == "PLUS":
                if left == TYPE_TEXT and right == TYPE_TEXT:
                    return TYPE_TEXT
                if left == TYPE_INT and right == TYPE_INT:
                    return TYPE_INT
                self.error("Operator + krever enten heltall+heltall eller tekst+tekst")
            if expr.op.typ in ("MINUS", "MUL", "DIV"):
                if left != TYPE_INT or right != TYPE_INT:
                    self.error(f"Operator {expr.op.value} krever heltall")
                return TYPE_INT
            if expr.op.typ == "PERCENT":
                if left != TYPE_INT or right != TYPE_INT:
                    self.error("Operator % krever heltall")
                return TYPE_INT
            if expr.op.typ in ("GT", "LT", "GTE", "LTE"):
                if left != TYPE_INT or right != TYPE_INT:
                    self.error("Sammenligning krever heltall")
                return TYPE_BOOL
            if expr.op.typ in ("EQ", "NE"):
                if left != right:
                    self.error("== og != krever samme type på begge sider")
                return TYPE_BOOL
            if expr.op.typ in ("OG", "ELLER"):
                if left != TYPE_BOOL or right != TYPE_BOOL:
                    self.error(f"{expr.op.value} krever bool på begge sider")
                return TYPE_BOOL

        if isinstance(expr, IfExprNode):
            cond_type = self.check_expr(expr.condition, scope)
            self.ensure_bool(cond_type, "hvis-betingelse")
            then_type = self.check_expr(expr.then_expr, scope)
            else_type = self.check_expr(expr.else_expr, scope)
            if then_type == else_type:
                return then_type
            if self.is_empty_list_expr(expr.then_expr) and else_type in (TYPE_LIST_INT, TYPE_LIST_TEXT):
                return else_type
            if self.is_empty_list_expr(expr.else_expr) and then_type in (TYPE_LIST_INT, TYPE_LIST_TEXT):
                return then_type
            self.ensure_assignable(then_type, else_type, "hvis-uttrykk")
            return then_type

        if isinstance(expr, ModuleCallNode):
            real_module = self.alias_map.get(expr.module_name, expr.module_name)
            full_name = f"{real_module}.{expr.func_name}"

            if full_name not in self.functions:
                self.error(f"Ukjent modulfunksjon: {expr.module_name}.{expr.func_name}")

            fn = self.functions[full_name]

            if len(expr.args) != len(fn.params):
                self.error(
                    f"Funksjonen '{expr.module_name}.{expr.func_name}' forventer {len(fn.params)} argument(er), fikk {len(expr.args)}"
                )

            for i, (arg, param_type) in enumerate(zip(expr.args, fn.params), start=1):
                arg_type = self.check_expr(arg, scope)
                if param_type is not None:
                    self.ensure_assignable(param_type, arg_type, f"argument {i} til {expr.module_name}.{expr.func_name}", arg)

            return fn.return_type

        if isinstance(expr, CallNode):
            fn, resolved_name = self.resolve_function_symbol(expr.name)
            if fn is None:
                self.error(f"Ukjent funksjon: {expr.name}")

            if expr.name == "assert":
                if len(expr.args) != 1:
                    self.error("assert forventer 1 argument")
                cond_type = self.check_expr(expr.args[0], scope)
                self.ensure_bool(cond_type, "assert")
                return TYPE_INT

            if expr.name in ("assert_eq", "assert_ne"):
                if len(expr.args) != 2:
                    self.error(f"{expr.name} forventer 2 argumenter")
                left_type = self.check_expr(expr.args[0], scope)
                right_type = self.check_expr(expr.args[1], scope)
                if left_type != right_type:
                    self.error(f"{expr.name} krever samme type på begge sider")
                return TYPE_INT

            if expr.name == "lengde":
                if len(expr.args) != 1:
                    self.error("lengde forventer 1 argument")
                arg_type = self.check_expr(expr.args[0], scope)
                if arg_type not in (TYPE_LIST_INT, TYPE_LIST_TEXT):
                    self.error("lengde støtter bare lister")
                return TYPE_INT

            if expr.name == "legg_til":
                if len(expr.args) != 2:
                    self.error("legg_til forventer 2 argumenter")
                list_type = self.check_expr(expr.args[0], scope)
                value_type = self.check_expr(expr.args[1], scope)
                if list_type == TYPE_LIST_INT and value_type == TYPE_INT:
                    return TYPE_INT
                if list_type == TYPE_LIST_TEXT and value_type == TYPE_TEXT:
                    return TYPE_INT
                self.error("legg_til krever liste og riktig type")

            if expr.name == "pop_siste":
                if len(expr.args) != 1:
                    self.error("pop_siste forventer 1 argument")
                list_type = self.check_expr(expr.args[0], scope)
                if list_type == TYPE_LIST_INT:
                    return TYPE_INT
                if list_type == TYPE_LIST_TEXT:
                    return TYPE_TEXT
                self.error("pop_siste krever liste")

            if expr.name == "fjern_indeks":
                if len(expr.args) != 2:
                    self.error("fjern_indeks forventer 2 argumenter")
                list_type = self.check_expr(expr.args[0], scope)
                idx_type = self.check_expr(expr.args[1], scope)
                if list_type not in (TYPE_LIST_INT, TYPE_LIST_TEXT) or idx_type != TYPE_INT:
                    self.error("fjern_indeks krever liste og heltall")
                return TYPE_INT

            if expr.name == "sett_inn":
                if len(expr.args) != 3:
                    self.error("sett_inn forventer 3 argumenter")
                list_type = self.check_expr(expr.args[0], scope)
                idx_type = self.check_expr(expr.args[1], scope)
                val_type = self.check_expr(expr.args[2], scope)
                if idx_type != TYPE_INT:
                    self.error("sett_inn krever heltall som indeks")
                if list_type == TYPE_LIST_INT and val_type == TYPE_INT:
                    return TYPE_INT
                if list_type == TYPE_LIST_TEXT and val_type == TYPE_TEXT:
                    return TYPE_INT
                self.error("sett_inn krever liste og verdi av riktig type")

            if len(expr.args) != len(fn.params):
                self.error(
                    f"Funksjonen '{resolved_name}' forventer {len(fn.params)} argument(er), fikk {len(expr.args)}"
                )
            for i, (arg, param_type) in enumerate(zip(expr.args, fn.params), start=1):
                arg_type = self.check_expr(arg, scope)
                if param_type is not None:
                    self.ensure_assignable(param_type, arg_type, f"argument {i} til {resolved_name}", arg)
            return fn.return_type

        self.error(f"Ukjent expression-type: {type(expr).__name__}")

    def ensure_assignable(self, expected, actual, where, actual_expr=None):
        if expected != actual:
            if actual_expr is not None and self.is_empty_list_expr(actual_expr) and expected in (TYPE_LIST_INT, TYPE_LIST_TEXT):
                return
            self.error(f"Typefeil i {where}: forventet {expected}, fikk {actual}")

    def ensure_bool(self, actual, where):
        if actual != TYPE_BOOL:
            self.error(f"Typefeil i {where}: forventet bool, fikk {actual}")
