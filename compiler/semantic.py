from .ast_nodes import *


class FunctionSymbol:
    def __init__(self, name, params, return_type, builtin=False, module_name=None):
        self.name = name
        self.params = params
        self.return_type = return_type
        self.builtin = builtin
        self.module_name = module_name


class LambdaSignature:
    def __init__(self, params, return_type):
        self.params = params
        self.return_type = return_type

    def __repr__(self):
        return f"LambdaSignature(params={self.params!r}, return_type={self.return_type!r})"


class SemanticAnalyzer:
    def __init__(self, alias_map=None):
        self.alias_map = alias_map or {}
        self.current_function = None
        self.current_function_full_name = None
        self.functions = {
            "tekst_fra_heltall": FunctionSymbol("tekst_fra_heltall", [TYPE_INT], TYPE_TEXT, True),
            "tekst_fra_bool": FunctionSymbol("tekst_fra_bool", [TYPE_BOOL], TYPE_TEXT, True),
            "heltall_fra_tekst": FunctionSymbol("heltall_fra_tekst", [TYPE_TEXT], TYPE_INT, True),
            "tekst_starter_med": FunctionSymbol("tekst_starter_med", [TYPE_TEXT, TYPE_TEXT], TYPE_BOOL, True),
            "tekst_slutter_med": FunctionSymbol("tekst_slutter_med", [TYPE_TEXT, TYPE_TEXT], TYPE_BOOL, True),
            "tekst_inneholder": FunctionSymbol("tekst_inneholder", [TYPE_TEXT, TYPE_TEXT], TYPE_BOOL, True),
            "tekst_trim": FunctionSymbol("tekst_trim", [TYPE_TEXT], TYPE_TEXT, True),
            "del_ord": FunctionSymbol("del_ord", [TYPE_TEXT], TYPE_LIST_TEXT, True),
            "tokeniser_enkel": FunctionSymbol("tokeniser_enkel", [TYPE_TEXT], TYPE_LIST_TEXT, True),
            "tokeniser_uttrykk": FunctionSymbol("tokeniser_uttrykk", [TYPE_TEXT], TYPE_LIST_TEXT, True),
            "les_input": FunctionSymbol("les_input", [TYPE_TEXT], TYPE_TEXT, True),
            "lengde": FunctionSymbol("lengde", [None], TYPE_INT, True),
            "legg_til": FunctionSymbol("legg_til", [None, None], TYPE_INT, True),
            "pop_siste": FunctionSymbol("pop_siste", [None], None, True),
            "fjern_indeks": FunctionSymbol("fjern_indeks", [None, TYPE_INT], TYPE_INT, True),
            "sett_inn": FunctionSymbol("sett_inn", [None, TYPE_INT, None], TYPE_INT, True),
            "har_nokkel": FunctionSymbol("har_nokkel", [None, TYPE_TEXT], TYPE_BOOL, True),
            "fjern_nokkel": FunctionSymbol("fjern_nokkel", [None, TYPE_TEXT], TYPE_INT, True),
            "json_parse": FunctionSymbol("json_parse", [TYPE_TEXT], TYPE_MAP_TEXT, True),
            "json_stringify": FunctionSymbol("json_stringify", [TYPE_MAP_TEXT], TYPE_TEXT, True),
            "fil_les": FunctionSymbol("fil_les", [TYPE_TEXT], TYPE_TEXT, True),
            "fil_skriv": FunctionSymbol("fil_skriv", [TYPE_TEXT, TYPE_TEXT], TYPE_TEXT, True),
            "fil_append": FunctionSymbol("fil_append", [TYPE_TEXT, TYPE_TEXT], TYPE_TEXT, True),
            "fil_finnes": FunctionSymbol("fil_finnes", [TYPE_TEXT], TYPE_BOOL, True),
            "web_path_match": FunctionSymbol("web_path_match", [TYPE_TEXT, TYPE_TEXT], TYPE_BOOL, True),
            "web_path_params": FunctionSymbol("web_path_params", [TYPE_TEXT, TYPE_TEXT], TYPE_MAP_TEXT, True),
            "web_route_match": FunctionSymbol("web_route_match", [TYPE_TEXT, TYPE_TEXT, TYPE_TEXT], TYPE_BOOL, True),
            "web_dispatch": FunctionSymbol("web_dispatch", [TYPE_MAP_TEXT, TYPE_TEXT, TYPE_TEXT], TYPE_TEXT, True),
            "web_dispatch_params": FunctionSymbol("web_dispatch_params", [TYPE_MAP_TEXT, TYPE_TEXT, TYPE_TEXT], TYPE_MAP_TEXT, True),
            "web_request_context": FunctionSymbol("web_request_context", [TYPE_TEXT, TYPE_TEXT, TYPE_MAP_TEXT, TYPE_MAP_TEXT, TYPE_TEXT], TYPE_MAP_TEXT, True),
            "web_request_method": FunctionSymbol("web_request_method", [TYPE_MAP_TEXT], TYPE_TEXT, True),
            "web_request_path": FunctionSymbol("web_request_path", [TYPE_MAP_TEXT], TYPE_TEXT, True),
            "web_request_query": FunctionSymbol("web_request_query", [TYPE_MAP_TEXT], TYPE_MAP_TEXT, True),
            "web_request_headers": FunctionSymbol("web_request_headers", [TYPE_MAP_TEXT], TYPE_MAP_TEXT, True),
            "web_request_body": FunctionSymbol("web_request_body", [TYPE_MAP_TEXT], TYPE_TEXT, True),
            "web_request_params": FunctionSymbol("web_request_params", [TYPE_MAP_TEXT], TYPE_MAP_TEXT, True),
            "web_request_param": FunctionSymbol("web_request_param", [TYPE_MAP_TEXT, TYPE_TEXT], TYPE_TEXT, True),
            "web_request_param_int": FunctionSymbol("web_request_param_int", [TYPE_MAP_TEXT, TYPE_TEXT], TYPE_INT, True),
            "web_request_query_required": FunctionSymbol("web_request_query_required", [TYPE_MAP_TEXT, TYPE_TEXT], TYPE_TEXT, True),
            "web_request_query_int": FunctionSymbol("web_request_query_int", [TYPE_MAP_TEXT, TYPE_TEXT], TYPE_INT, True),
            "web_request_json": FunctionSymbol("web_request_json", [TYPE_MAP_TEXT], TYPE_MAP_TEXT, True),
            "web_request_json_field": FunctionSymbol("web_request_json_field", [TYPE_MAP_TEXT, TYPE_TEXT], TYPE_TEXT, True),
            "web_request_json_field_or": FunctionSymbol("web_request_json_field_or", [TYPE_MAP_TEXT, TYPE_TEXT, TYPE_TEXT], TYPE_TEXT, True),
            "web_request_json_field_int": FunctionSymbol("web_request_json_field_int", [TYPE_MAP_TEXT, TYPE_TEXT], TYPE_INT, True),
            "web_request_json_field_bool": FunctionSymbol("web_request_json_field_bool", [TYPE_MAP_TEXT, TYPE_TEXT], TYPE_BOOL, True),
            "web_request_query_param": FunctionSymbol("web_request_query_param", [TYPE_MAP_TEXT, TYPE_TEXT], TYPE_TEXT, True),
            "web_request_header": FunctionSymbol("web_request_header", [TYPE_MAP_TEXT, TYPE_TEXT], TYPE_TEXT, True),
            "web_response_builder": FunctionSymbol("web_response_builder", [TYPE_INT, TYPE_MAP_TEXT, TYPE_TEXT], TYPE_MAP_TEXT, True),
            "web_response_status": FunctionSymbol("web_response_status", [TYPE_MAP_TEXT], TYPE_INT, True),
            "web_response_headers": FunctionSymbol("web_response_headers", [TYPE_MAP_TEXT], TYPE_MAP_TEXT, True),
            "web_response_body": FunctionSymbol("web_response_body", [TYPE_MAP_TEXT], TYPE_TEXT, True),
            "web_response_header": FunctionSymbol("web_response_header", [TYPE_MAP_TEXT, TYPE_TEXT], TYPE_TEXT, True),
            "web_response_text": FunctionSymbol("web_response_text", [TYPE_MAP_TEXT], TYPE_TEXT, True),
            "web_response_json": FunctionSymbol("web_response_json", [TYPE_MAP_TEXT], TYPE_MAP_TEXT, True),
            "web_response_error": FunctionSymbol("web_response_error", [TYPE_INT, TYPE_TEXT], TYPE_MAP_TEXT, True),
            "web_response_file": FunctionSymbol("web_response_file", [TYPE_TEXT, TYPE_TEXT], TYPE_MAP_TEXT, True),
            "web_openapi_json": FunctionSymbol("web_openapi_json", [TYPE_TEXT, TYPE_TEXT], TYPE_TEXT, True),
            "web_docs_html": FunctionSymbol("web_docs_html", [TYPE_TEXT, TYPE_TEXT], TYPE_TEXT, True),
            "web_route": FunctionSymbol("web_route", [TYPE_TEXT], TYPE_TEXT, True),
            "web_dependency": FunctionSymbol("web_dependency", [TYPE_TEXT], TYPE_TEXT, True),
            "web_use_dependency": FunctionSymbol("web_use_dependency", [TYPE_TEXT], TYPE_TEXT, True),
            "web_request_middleware": FunctionSymbol("web_request_middleware", [], TYPE_TEXT, True),
            "web_response_middleware": FunctionSymbol("web_response_middleware", [], TYPE_TEXT, True),
            "web_error_middleware": FunctionSymbol("web_error_middleware", [], TYPE_TEXT, True),
            "web_startup_hook": FunctionSymbol("web_startup_hook", [], TYPE_TEXT, True),
            "web_shutdown_hook": FunctionSymbol("web_shutdown_hook", [], TYPE_TEXT, True),
            "web_handle_request": FunctionSymbol("web_handle_request", [TYPE_MAP_TEXT], TYPE_MAP_TEXT, True),
            "web_request_dependency": FunctionSymbol("web_request_dependency", [TYPE_MAP_TEXT, TYPE_TEXT], TYPE_MAP_TEXT, True),
            "web_request_id": FunctionSymbol("web_request_id", [TYPE_MAP_TEXT], TYPE_TEXT, True),
            "web_startup": FunctionSymbol("web_startup", [], TYPE_INT, True),
            "web_shutdown": FunctionSymbol("web_shutdown", [], TYPE_INT, True),
            "vent_timeout": FunctionSymbol("vent_timeout", [None, TYPE_INT], None, True),
            "vent_kanseller": FunctionSymbol("vent_kanseller", [None], None, True),
            "vent_er_kansellert": FunctionSymbol("vent_er_kansellert", [None], TYPE_BOOL, True),
            "vent_er_timeoutet": FunctionSymbol("vent_er_timeoutet", [None], TYPE_BOOL, True),
            "vent_sov": FunctionSymbol("vent_sov", [TYPE_INT], TYPE_INT, True),
            "sti_join": FunctionSymbol("sti_join", [TYPE_TEXT, TYPE_TEXT], TYPE_TEXT, True),
            "sti_basename": FunctionSymbol("sti_basename", [TYPE_TEXT], TYPE_TEXT, True),
            "sti_dirname": FunctionSymbol("sti_dirname", [TYPE_TEXT], TYPE_TEXT, True),
            "sti_exists": FunctionSymbol("sti_exists", [TYPE_TEXT], TYPE_BOOL, True),
            "sti_stem": FunctionSymbol("sti_stem", [TYPE_TEXT], TYPE_TEXT, True),
            "miljo_hent": FunctionSymbol("miljo_hent", [TYPE_TEXT], TYPE_TEXT, True),
            "miljo_finnes": FunctionSymbol("miljo_finnes", [TYPE_TEXT], TYPE_BOOL, True),
            "miljo_sett": FunctionSymbol("miljo_sett", [TYPE_TEXT, TYPE_TEXT], TYPE_TEXT, True),
            "assert": FunctionSymbol("assert", [TYPE_BOOL], TYPE_INT, True),
            "assert_eq": FunctionSymbol("assert_eq", [None, None], TYPE_INT, True),
            "assert_ne": FunctionSymbol("assert_ne", [None, None], TYPE_INT, True),
            "assert_starter_med": FunctionSymbol("assert_starter_med", [TYPE_TEXT, TYPE_TEXT], TYPE_INT, True),
            "assert_slutter_med": FunctionSymbol("assert_slutter_med", [TYPE_TEXT, TYPE_TEXT], TYPE_INT, True),
            "assert_inneholder": FunctionSymbol("assert_inneholder", [TYPE_TEXT, TYPE_TEXT], TYPE_INT, True),
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

    def is_empty_map_expr(self, expr):
        return isinstance(expr, MapLiteralNode) and not expr.items

    def is_map_type(self, type_name):
        return is_map_type_name(type_name)

    def _struct_fields_for_expr(self, expr, field_schemas):
        if isinstance(expr, StructLiteralNode):
            return {name for name, _ in expr.fields}
        if isinstance(expr, VarAccessNode):
            return field_schemas.get(expr.name)
        return None

    def map_value_type(self, map_type):
        if map_type == TYPE_MAP_INT:
            return TYPE_INT
        if map_type == TYPE_MAP_TEXT:
            return TYPE_TEXT
        if map_type == TYPE_MAP_BOOL:
            return TYPE_BOOL
        if is_map_type_name(map_type) and map_type.startswith(TYPE_MAP_PREFIX):
            inner = map_type[len(TYPE_MAP_PREFIX):]
            if inner == "heltall":
                return TYPE_INT
            if inner == "tekst":
                return TYPE_TEXT
            if inner == "bool":
                return TYPE_BOOL
            if is_map_type_name(inner):
                return inner
        return None

    def map_type_for_value(self, value_type):
        if value_type == TYPE_INT:
            return TYPE_MAP_INT
        if value_type == TYPE_TEXT:
            return TYPE_MAP_TEXT
        if value_type == TYPE_BOOL:
            return TYPE_MAP_BOOL
            if is_map_type_name(value_type):
                return map_type_name(value_type)
        return None

    def is_match_literal_pattern(self, expr):
        return isinstance(expr, (NumberNode, StringNode, BoolNode))

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
            struct_fields = {}
            saw_return = self.check_block(fn.body, scope, fn.return_type, False, struct_fields)
            dependency_names = list(getattr(fn, "dependency_names", []) or [])
            provided_dependencies = list(getattr(fn, "provided_dependencies", []) or [])
            if provided_dependencies:
                if len(fn.params) not in (0, 1):
                    self.error("dependency-providers bør ta 0 eller 1 argument")
                if len(fn.params) == 1 and fn.params[0].type_name != TYPE_MAP_TEXT:
                    self.error("dependency-providers med argument må ta ordbok_tekst")
                if fn.return_type != TYPE_MAP_TEXT:
                    self.error("dependency-providers må returnere ordbok_tekst")
            if dependency_names:
                expected_params = 1 + len(dependency_names)
                if len(fn.params) != expected_params:
                    self.error(
                        f"Funksjonen '{fn.name}' forventer {expected_params} parameter(e) når den bruker dependencies"
                    )
                if fn.params and fn.params[0].type_name != TYPE_MAP_TEXT:
                    self.error("Første parameter i web-handler må være ordbok_tekst")
                for param in fn.params[1:]:
                    if param.type_name != TYPE_MAP_TEXT:
                        self.error("Dependency-parametere må være ordbok_tekst")
            if not saw_return and fn.name != "start":
                self.error(f"Funksjonen '{fn.name}' må returnere {fn.return_type}")
        finally:
            self.current_function = previous_function
            self.current_function_full_name = previous_function_full_name

    def check_block(self, block, scope, expected_return_type, in_loop, struct_fields):
        local_scope = dict(scope)
        local_struct_fields = dict(struct_fields)
        returned = False
        for stmt in block.statements:
            stmt_returns = self.check_stmt(
                stmt,
                local_scope,
                expected_return_type,
                in_loop,
                local_struct_fields,
            )
            if stmt_returns:
                returned = True
                break
        return returned

    def check_stmt(self, stmt, scope, expected_return_type, in_loop, struct_fields):
        if isinstance(stmt, VarDeclareNode):
            is_empty_list = self.is_empty_list_expr(stmt.expr)
            is_empty_map = self.is_empty_map_expr(stmt.expr)
            expr_type = self.check_expr(stmt.expr, scope, struct_fields)
            if isinstance(stmt.expr, StructLiteralNode):
                struct_fields[stmt.name] = {name for name, _ in stmt.expr.fields}
            else:
                struct_fields.pop(stmt.name, None)
            if stmt.var_type is None:
                if is_empty_map:
                    self.error(f"Tom ordbok krever eksplisitt type for variabel '{stmt.name}'")
                scope[stmt.name] = expr_type
            else:
                if is_empty_list and stmt.var_type in (TYPE_LIST_INT, TYPE_LIST_TEXT):
                    scope[stmt.name] = stmt.var_type
                    return False
                if is_empty_map and self.is_map_type(stmt.var_type):
                    scope[stmt.name] = stmt.var_type
                    return False
                self.ensure_assignable(stmt.var_type, expr_type, f"variabel '{stmt.name}'")
                scope[stmt.name] = stmt.var_type
            return False

        if isinstance(stmt, VarSetNode):
            if stmt.name not in scope:
                self.error(f"Variabel '{stmt.name}' er ikke definert")
            is_empty_list = self.is_empty_list_expr(stmt.expr)
            is_empty_map = self.is_empty_map_expr(stmt.expr)
            expr_type = self.check_expr(stmt.expr, scope, struct_fields)
            if isinstance(stmt.expr, StructLiteralNode):
                struct_fields[stmt.name] = {name for name, _ in stmt.expr.fields}
            else:
                struct_fields.pop(stmt.name, None)
            if is_empty_list and scope[stmt.name] in (TYPE_LIST_INT, TYPE_LIST_TEXT):
                return False
            if is_empty_map and self.is_map_type(scope[stmt.name]):
                return False
            self.ensure_assignable(scope[stmt.name], expr_type, f"variabel '{stmt.name}'")
            return False

        if isinstance(stmt, IndexSetNode):
            target_type = scope.get(stmt.target_name)
            if target_type is None:
                self.error(f"Variabel '{stmt.target_name}' er ikke definert")
            idx_type = self.check_expr(stmt.index_expr, scope, struct_fields)
            value_type = self.check_expr(stmt.value_expr, scope, struct_fields)
            if target_type in (TYPE_LIST_INT, TYPE_LIST_TEXT):
                if idx_type != TYPE_INT:
                    self.error("Listeindeks må være heltall")
            elif self.is_map_type(target_type):
                if idx_type != TYPE_TEXT:
                    self.error("Ordboknøkkel må være tekst")
            else:
                self.error("Indeks-tilordning krever liste eller ordbok")
            if target_type == TYPE_LIST_INT and value_type != TYPE_INT:
                self.error("liste_heltall kan bare få heltall")
            if target_type == TYPE_LIST_TEXT and value_type != TYPE_TEXT:
                self.error("liste_tekst kan bare få tekst")
            if self.is_map_type(target_type):
                expected_value_type = self.map_value_type(target_type)
                if value_type != expected_value_type:
                    self.error(f"{target_type} kan bare få verdier av type {expected_value_type}, men fikk {value_type}")
            return False

        if isinstance(stmt, PrintNode):
            self.check_expr(stmt.expr, scope, struct_fields)
            return False

        if isinstance(stmt, IfNode):
            cond_type = self.check_expr(stmt.condition, scope, struct_fields)
            self.ensure_bool(cond_type, "hvis-betingelse")
            self.check_block(stmt.then_block, dict(scope), expected_return_type, in_loop, dict(struct_fields))
            if stmt.else_block:
                self.check_block(stmt.else_block, dict(scope), expected_return_type, in_loop, dict(struct_fields))
            return False

        if isinstance(stmt, IfExprNode):
            cond_type = self.check_expr(stmt.condition, scope, struct_fields)
            self.ensure_bool(cond_type, "hvis-betingelse")
            then_type = self.check_expr(stmt.then_expr, scope, struct_fields)
            else_type = self.check_expr(stmt.else_expr, scope, struct_fields)
            self.ensure_assignable(then_type, else_type, "hvis-uttrykk")
            return False

        if isinstance(stmt, MatchNode):
            subject_type = self.check_expr(stmt.subject, scope, struct_fields)
            if subject_type not in (TYPE_INT, TYPE_TEXT, TYPE_BOOL):
                self.error("match støtter bare heltall, tekst og bool")

            wildcard_seen = False
            for index, case in enumerate(getattr(stmt, "cases", [])):
                if getattr(case, "wildcard", False):
                    wildcard_seen = True
                    if case.pattern is not None:
                        self.error("Wildcard-case i match kan ikke ha mønster")
                    if index != len(stmt.cases) - 1:
                        self.error("Wildcard-case i match må være siste case")
                    self.check_block(case.body, dict(scope), expected_return_type, in_loop, dict(struct_fields))
                    continue

                if not self.is_match_literal_pattern(case.pattern):
                    self.error("Match-case må være heltall, tekst, bool eller _")
                pattern_type = self.check_expr(case.pattern, scope, struct_fields)
                if pattern_type != subject_type:
                    self.error("Match-case må ha samme type som uttrykket")
                self.check_block(case.body, dict(scope), expected_return_type, in_loop, dict(struct_fields))

            if stmt.else_block is not None and wildcard_seen:
                self.error("match kan ikke ha både wildcard-case og ellers-blokk")
            if stmt.else_block is not None:
                self.check_block(stmt.else_block, dict(scope), expected_return_type, in_loop, dict(struct_fields))
            return False

        if isinstance(stmt, WhileNode):
            cond_type = self.check_expr(stmt.condition, scope, struct_fields)
            self.ensure_bool(cond_type, "mens-betingelse")
            self.check_block(stmt.body, dict(scope), expected_return_type, True, dict(struct_fields))
            return False

        if isinstance(stmt, ForNode):
            st = self.check_expr(stmt.start_expr, scope, struct_fields)
            et = self.check_expr(stmt.end_expr, scope, struct_fields)
            pt = self.check_expr(stmt.step_expr, scope, struct_fields)
            if st != TYPE_INT or et != TYPE_INT or pt != TYPE_INT:
                self.error("for krever heltall i start, til og steg")
            loop_scope = dict(scope)
            loop_scope[stmt.name] = TYPE_INT
            self.check_block(stmt.body, loop_scope, expected_return_type, True, dict(struct_fields))
            return False

        if isinstance(stmt, ForEachNode):
            list_type = self.check_expr(stmt.list_expr, scope, struct_fields)
            loop_scope = dict(scope)
            if list_type == TYPE_LIST_INT:
                loop_scope[stmt.item_name] = TYPE_INT
            elif list_type == TYPE_LIST_TEXT:
                loop_scope[stmt.item_name] = TYPE_TEXT
            else:
                self.error("for ... i ... krever liste")
            self.check_block(stmt.body, loop_scope, expected_return_type, True, dict(struct_fields))
            return False

        if isinstance(stmt, ReturnNode):
            expr_type = self.check_expr(stmt.expr, scope, struct_fields)
            self.ensure_assignable(expected_return_type, expr_type, "returverdi", stmt.expr)
            return True

        if isinstance(stmt, ThrowNode):
            expr_type = self.check_expr(stmt.expr, scope, struct_fields)
            self.ensure_assignable(TYPE_TEXT, expr_type, "kast")
            return True

        if isinstance(stmt, TryCatchNode):
            try_returns = self.check_block(
                stmt.try_block,
                dict(scope),
                expected_return_type,
                in_loop,
                dict(struct_fields),
            )
            catch_scope = dict(scope)
            if stmt.catch_var_name is not None:
                previous_catch_type = catch_scope.get(stmt.catch_var_name)
                if previous_catch_type is not None and previous_catch_type != TYPE_TEXT:
                    self.error("fang-variabel må være tekst")
                catch_scope[stmt.catch_var_name] = TYPE_TEXT
            catch_returns = self.check_block(
                stmt.catch_block,
                catch_scope,
                expected_return_type,
                in_loop,
                dict(struct_fields),
            )
            return try_returns and catch_returns

        if isinstance(stmt, BreakNode):
            if not in_loop:
                self.error("bryt kan bare brukes inne i en løkke")
            return False

        if isinstance(stmt, ContinueNode):
            if not in_loop:
                self.error("fortsett kan bare brukes inne i en løkke")
            return False

        if isinstance(stmt, ExprStmtNode):
            self.check_expr(stmt.expr, scope, struct_fields)
            return False

        self.error(f"Ukjent statement-type: {type(stmt).__name__}")

    def check_expr(self, expr, scope, field_schemas):
        if isinstance(expr, NumberNode):
            return TYPE_INT

        if isinstance(expr, StringNode):
            return TYPE_TEXT

        if isinstance(expr, BoolNode):
            return TYPE_BOOL

        if isinstance(expr, ListLiteralNode):
            if not expr.items:
                return TYPE_LIST_INT
            item_types = [self.check_expr(item, scope, field_schemas) for item in expr.items]
            first = item_types[0]
            for t in item_types[1:]:
                if t != first:
                    self.error("Alle elementer i en liste må ha samme type")
            if first == TYPE_INT:
                return TYPE_LIST_INT
            if first == TYPE_TEXT:
                return TYPE_LIST_TEXT
            self.error("Lister støtter bare heltall eller tekst")

        if isinstance(expr, LambdaNode):
            lambda_scope = dict(scope)
            param_types = []
            for param in expr.params:
                lambda_scope[param.name] = param.type_name or TYPE_INT
                param_types.append(param.type_name or TYPE_INT)
            body_type = self.check_expr(expr.body, lambda_scope, field_schemas)
            signature = LambdaSignature(param_types, body_type)
            setattr(expr, "return_type", body_type)
            setattr(expr, "signature", signature)
            return signature

        if isinstance(expr, ListComprehensionNode):
            source_type = self.check_expr(expr.source_expr, scope, field_schemas)
            if source_type not in (TYPE_LIST_INT, TYPE_LIST_TEXT):
                self.error("Liste-comprehension krever en liste som kilde")
            item_scope = dict(scope)
            item_scope[expr.item_name] = TYPE_INT if source_type == TYPE_LIST_INT else TYPE_TEXT
            if expr.condition_expr is not None:
                cond_type = self.check_expr(expr.condition_expr, item_scope, field_schemas)
                self.ensure_bool(cond_type, "liste-comprehension-betingelse")
            item_type = self.check_expr(expr.item_expr, item_scope, field_schemas)
            if item_type == TYPE_INT:
                return TYPE_LIST_INT
            if item_type == TYPE_TEXT:
                return TYPE_LIST_TEXT
            self.error("Liste-comprehension må produsere heltall eller tekst")

        if isinstance(expr, MapLiteralNode):
            if not expr.items:
                return TYPE_MAP_TEXT
            value_types = []
            for key_expr, value_expr in expr.items:
                key_type = self.check_expr(key_expr, scope, field_schemas)
                if key_type != TYPE_TEXT:
                    self.error("Ordboknøkler må være tekst")
                value_types.append(self.check_expr(value_expr, scope, field_schemas))
            first = value_types[0]
            for t in value_types[1:]:
                if t != first:
                    self.error("Alle verdier i en ordbok må ha samme type")
            if first == TYPE_INT:
                return TYPE_MAP_INT
            if first == TYPE_TEXT:
                return TYPE_MAP_TEXT
            if first == TYPE_BOOL:
                return TYPE_MAP_BOOL
            if self.is_map_type(first):
                return map_type_name(first)
            self.error("Ordbøker støtter bare heltall, tekst, bool eller kart som verditype")
        if isinstance(expr, StructLiteralNode):
            value_types = [self.check_expr(value_expr, scope, field_schemas) for _field_name, value_expr in expr.fields]
            first = value_types[0]
            for t in value_types[1:]:
                if t != first:
                    self.error("Alle felt i en struktur må ha samme type")
            if first == TYPE_INT:
                return TYPE_MAP_INT
            if first == TYPE_TEXT:
                return TYPE_MAP_TEXT
            if first == TYPE_BOOL:
                return TYPE_MAP_BOOL
            if self.is_map_type(first):
                return map_type_name(first)
            self.error("Strukturer støtter bare heltall, tekst, bool eller kart som verditype")

        if isinstance(expr, VarAccessNode):
            if expr.name in scope:
                return scope[expr.name]
            if expr.name in self.functions:
                self.error(f"Funksjonen '{expr.name}' må kalles med ()")
            self.error(f"Ukjent navn: {expr.name}")

        if isinstance(expr, IndexNode):
            target_expr = getattr(expr, "target", getattr(expr, "list_expr", None))
            index_expr = getattr(expr, "index_expr", None)
            target_type = self.check_expr(target_expr, scope, field_schemas)
            idx_type = self.check_expr(index_expr, scope, field_schemas)
            if target_type == TYPE_LIST_INT:
                if idx_type != TYPE_INT:
                    self.error("Listeindeks må være heltall")
                return TYPE_INT
            if target_type == TYPE_LIST_TEXT:
                if idx_type != TYPE_INT:
                    self.error("Listeindeks må være heltall")
                return TYPE_TEXT
            if target_type == TYPE_MAP_INT:
                if idx_type != TYPE_TEXT:
                    self.error("Ordboknøkkel må være tekst")
                return TYPE_INT
            if self.is_map_type(target_type):
                if idx_type != TYPE_TEXT:
                    self.error("Ordboknøkkel må være tekst")
                return self.map_value_type(target_type)
            self.error("[] kan bare brukes på lister eller ordbøker")

        if isinstance(expr, SliceNode):
            target_type = self.check_expr(expr.target, scope, field_schemas)
            if expr.start_expr is not None:
                start_type = self.check_expr(expr.start_expr, scope, field_schemas)
                if start_type != TYPE_INT:
                    self.error("Slicing krever heltall som startindeks")
            if expr.end_expr is not None:
                end_type = self.check_expr(expr.end_expr, scope, field_schemas)
                if end_type != TYPE_INT:
                    self.error("Slicing krever heltall som sluttindeks")
            if target_type in (TYPE_LIST_INT, TYPE_LIST_TEXT, TYPE_TEXT):
                return target_type
            self.error("Slicing kan bare brukes på tekst og lister")

        if isinstance(expr, FieldAccessNode):
            target_type = self.check_expr(expr.target, scope, field_schemas)
            if not self.is_map_type(target_type):
                self.error("Punktumsoppslag er kun støttet på ordbøker")
            known_fields = self._struct_fields_for_expr(expr.target, field_schemas)
            if known_fields is not None and expr.field not in known_fields:
                available = ", ".join(sorted(known_fields)) if known_fields else ""
                if available:
                    self.error(f"Ukjent felt '{expr.field}' for struktur. Tillatte felt: {available}")
                else:
                    self.error(f"Ukjent felt '{expr.field}' for tom struktur")
            return self.map_value_type(target_type)

        if isinstance(expr, UnaryOpNode):
            inner = self.check_expr(expr.node, scope, field_schemas)
            if expr.op.typ in ("PLUS", "MINUS"):
                if inner != TYPE_INT:
                    self.error("Unary + og - krever heltall")
                return TYPE_INT
            if expr.op.typ == "IKKE":
                if inner != TYPE_BOOL:
                    self.error("ikke krever bool")
                return TYPE_BOOL

        if isinstance(expr, BinOpNode):
            left = self.check_expr(expr.left, scope, field_schemas)
            right = self.check_expr(expr.right, scope, field_schemas)
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
                if left == TYPE_INT and right == TYPE_INT:
                    return TYPE_BOOL
                if left == TYPE_TEXT and right == TYPE_TEXT:
                    return TYPE_BOOL
                self.error("Sammenligning krever enten to heltall eller to tekster")
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
            cond_type = self.check_expr(expr.condition, scope, field_schemas)
            self.ensure_bool(cond_type, "hvis-betingelse")
            then_type = self.check_expr(expr.then_expr, scope, field_schemas)
            else_type = self.check_expr(expr.else_expr, scope, field_schemas)
            if then_type == else_type:
                return then_type
            if self.is_empty_list_expr(expr.then_expr) and else_type in (TYPE_LIST_INT, TYPE_LIST_TEXT):
                return else_type
            if self.is_empty_list_expr(expr.else_expr) and then_type in (TYPE_LIST_INT, TYPE_LIST_TEXT):
                return then_type
            if self._is_empty_map(node.then_expr) and self.is_map_type(else_type):
                return else_type
            if self._is_empty_map(node.else_expr) and self.is_map_type(then_type):
                return then_type
            self.ensure_assignable(then_type, else_type, "hvis-uttrykk")
            return then_type

        if isinstance(expr, AwaitNode):
            return self.check_expr(expr.expr, scope, field_schemas)

        if isinstance(expr, ModuleCallNode):
            real_module = self.alias_map.get(expr.module_name, expr.module_name)
            full_name = f"{real_module}.{expr.func_name}"

            if full_name in {"http.request", "std.http.request"}:
                if len(expr.args) != 6:
                    self.error("http.request forventer 6 argumenter")
                method_type = self.check_expr(expr.args[0], scope, field_schemas)
                url_type = self.check_expr(expr.args[1], scope, field_schemas)
                headers_type = self.check_expr(expr.args[2], scope, field_schemas)
                query_type = self.check_expr(expr.args[3], scope, field_schemas)
                body_type = self.check_expr(expr.args[4], scope, field_schemas)
                timeout_type = self.check_expr(expr.args[5], scope, field_schemas)
                if method_type != TYPE_TEXT or url_type != TYPE_TEXT or body_type != TYPE_TEXT or timeout_type != TYPE_INT:
                    self.error("http.request krever tekst, tekst, ordbok, ordbok, tekst og heltall")
                if not self.is_map_type(headers_type) or not self.is_map_type(query_type):
                    self.error("http.request krever ordbok_tekst for headers og query")
                return TYPE_TEXT

            if full_name in {"http.request_full", "std.http.request_full"}:
                if len(expr.args) != 6:
                    self.error("http.request_full forventer 6 argumenter")
                method_type = self.check_expr(expr.args[0], scope, field_schemas)
                url_type = self.check_expr(expr.args[1], scope, field_schemas)
                headers_type = self.check_expr(expr.args[2], scope, field_schemas)
                query_type = self.check_expr(expr.args[3], scope, field_schemas)
                body_type = self.check_expr(expr.args[4], scope, field_schemas)
                timeout_type = self.check_expr(expr.args[5], scope, field_schemas)
                if method_type != TYPE_TEXT or url_type != TYPE_TEXT or body_type != TYPE_TEXT or timeout_type != TYPE_INT:
                    self.error("http.request_full krever tekst, tekst, ordbok, ordbok, tekst og heltall")
                if not self.is_map_type(headers_type) or not self.is_map_type(query_type):
                    self.error("http.request_full krever ordbok_tekst for headers og query")
                return TYPE_TEXT

            if full_name in {"http.get", "std.http.get"}:
                if len(expr.args) != 4:
                    self.error("http.get forventer 4 argumenter")
                url_type = self.check_expr(expr.args[0], scope, field_schemas)
                headers_type = self.check_expr(expr.args[1], scope, field_schemas)
                query_type = self.check_expr(expr.args[2], scope, field_schemas)
                timeout_type = self.check_expr(expr.args[3], scope, field_schemas)
                if url_type != TYPE_TEXT or timeout_type != TYPE_INT:
                    self.error("http.get krever tekst og heltall på riktig plass")
                if not self.is_map_type(headers_type) or not self.is_map_type(query_type):
                    self.error("http.get krever ordbok_tekst for headers og query")
                return TYPE_TEXT

            if full_name in {"http.post", "std.http.post"}:
                if len(expr.args) != 5:
                    self.error("http.post forventer 5 argumenter")
                url_type = self.check_expr(expr.args[0], scope, field_schemas)
                body_type = self.check_expr(expr.args[1], scope, field_schemas)
                headers_type = self.check_expr(expr.args[2], scope, field_schemas)
                query_type = self.check_expr(expr.args[3], scope, field_schemas)
                timeout_type = self.check_expr(expr.args[4], scope, field_schemas)
                if url_type != TYPE_TEXT or body_type != TYPE_TEXT or timeout_type != TYPE_INT:
                    self.error("http.post krever tekst, tekst og heltall på riktig plass")
                if not self.is_map_type(headers_type) or not self.is_map_type(query_type):
                    self.error("http.post krever ordbok_tekst for headers og query")
                return TYPE_TEXT

            if full_name in {"http.json_get", "std.http.json_get"}:
                if len(expr.args) != 4:
                    self.error("http.json_get forventer 4 argumenter")
                url_type = self.check_expr(expr.args[0], scope, field_schemas)
                headers_type = self.check_expr(expr.args[1], scope, field_schemas)
                query_type = self.check_expr(expr.args[2], scope, field_schemas)
                timeout_type = self.check_expr(expr.args[3], scope, field_schemas)
                if url_type != TYPE_TEXT or timeout_type != TYPE_INT:
                    self.error("http.json_get krever tekst og heltall på riktig plass")
                if not self.is_map_type(headers_type) or not self.is_map_type(query_type):
                    self.error("http.json_get krever ordbok_tekst for headers og query")
                return TYPE_MAP_TEXT

            if full_name in {"http.json_post", "std.http.json_post"}:
                if len(expr.args) != 5:
                    self.error("http.json_post forventer 5 argumenter")
                url_type = self.check_expr(expr.args[0], scope, field_schemas)
                body_type = self.check_expr(expr.args[1], scope, field_schemas)
                headers_type = self.check_expr(expr.args[2], scope, field_schemas)
                query_type = self.check_expr(expr.args[3], scope, field_schemas)
                timeout_type = self.check_expr(expr.args[4], scope, field_schemas)
                if url_type != TYPE_TEXT or timeout_type != TYPE_INT:
                    self.error("http.json_post krever tekst og heltall på riktig plass")
                if not self.is_map_type(body_type) or not self.is_map_type(headers_type) or not self.is_map_type(query_type):
                    self.error("http.json_post krever ordbok_tekst for body, headers og query")
                return TYPE_MAP_TEXT

            if full_name in {"http.response_status", "std.http.response_status"}:
                if len(expr.args) != 1:
                    self.error("http.response_status forventer 1 argument")
                response_type = self.check_expr(expr.args[0], scope, field_schemas)
                if response_type != TYPE_TEXT:
                    self.error("http.response_status krever tekst")
                return TYPE_INT

            if full_name in {"http.response_text", "std.http.response_text"}:
                if len(expr.args) != 1:
                    self.error("http.response_text forventer 1 argument")
                response_type = self.check_expr(expr.args[0], scope, field_schemas)
                if response_type != TYPE_TEXT:
                    self.error("http.response_text krever tekst")
                return TYPE_TEXT

            if full_name in {"http.response_json", "std.http.response_json"}:
                if len(expr.args) != 1:
                    self.error("http.response_json forventer 1 argument")
                response_type = self.check_expr(expr.args[0], scope, field_schemas)
                if response_type != TYPE_TEXT:
                    self.error("http.response_json krever tekst")
                return TYPE_MAP_TEXT

            if full_name in {"http.response_header", "std.http.response_header"}:
                if len(expr.args) != 2:
                    self.error("http.response_header forventer 2 argumenter")
                response_type = self.check_expr(expr.args[0], scope, field_schemas)
                header_type = self.check_expr(expr.args[1], scope, field_schemas)
                if response_type != TYPE_TEXT or header_type != TYPE_TEXT:
                    self.error("http.response_header krever tekst og tekst")
                return TYPE_TEXT

            if full_name in {"web.path_match", "std.web.path_match"}:
                if len(expr.args) != 2:
                    self.error("web.path_match forventer 2 argumenter")
                pattern_type = self.check_expr(expr.args[0], scope, field_schemas)
                path_type = self.check_expr(expr.args[1], scope, field_schemas)
                if pattern_type != TYPE_TEXT or path_type != TYPE_TEXT:
                    self.error("web.path_match krever tekst og tekst")
                return TYPE_BOOL

            if full_name in {"web.path_params", "std.web.path_params"}:
                if len(expr.args) != 2:
                    self.error("web.path_params forventer 2 argumenter")
                pattern_type = self.check_expr(expr.args[0], scope, field_schemas)
                path_type = self.check_expr(expr.args[1], scope, field_schemas)
                if pattern_type != TYPE_TEXT or path_type != TYPE_TEXT:
                    self.error("web.path_params krever tekst og tekst")
                return TYPE_MAP_TEXT

            if full_name in {"web.route_match", "std.web.route_match"}:
                if len(expr.args) != 3:
                    self.error("web.route_match forventer 3 argumenter")
                spec_type = self.check_expr(expr.args[0], scope, field_schemas)
                method_type = self.check_expr(expr.args[1], scope, field_schemas)
                path_type = self.check_expr(expr.args[2], scope, field_schemas)
                if spec_type != TYPE_TEXT or method_type != TYPE_TEXT or path_type != TYPE_TEXT:
                    self.error("web.route_match krever tekst, tekst og tekst")
                return TYPE_BOOL

            if full_name in {"web.dispatch", "std.web.dispatch"}:
                if len(expr.args) != 3:
                    self.error("web.dispatch forventer 3 argumenter")
                routes_type = self.check_expr(expr.args[0], scope, field_schemas)
                method_type = self.check_expr(expr.args[1], scope, field_schemas)
                path_type = self.check_expr(expr.args[2], scope, field_schemas)
                if not self.is_map_type(routes_type) or method_type != TYPE_TEXT or path_type != TYPE_TEXT:
                    self.error("web.dispatch krever ordbok_tekst, tekst og tekst")
                return TYPE_TEXT

            if full_name in {"web.dispatch_params", "std.web.dispatch_params"}:
                if len(expr.args) != 3:
                    self.error("web.dispatch_params forventer 3 argumenter")
                routes_type = self.check_expr(expr.args[0], scope, field_schemas)
                method_type = self.check_expr(expr.args[1], scope, field_schemas)
                path_type = self.check_expr(expr.args[2], scope, field_schemas)
                if not self.is_map_type(routes_type) or method_type != TYPE_TEXT or path_type != TYPE_TEXT:
                    self.error("web.dispatch_params krever ordbok_tekst, tekst og tekst")
                return TYPE_MAP_TEXT

            if full_name in {"web.request_context", "std.web.request_context"}:
                if len(expr.args) != 5:
                    self.error("web.request_context forventer 5 argumenter")
                method_type = self.check_expr(expr.args[0], scope, field_schemas)
                path_type = self.check_expr(expr.args[1], scope, field_schemas)
                query_type = self.check_expr(expr.args[2], scope, field_schemas)
                headers_type = self.check_expr(expr.args[3], scope, field_schemas)
                body_type = self.check_expr(expr.args[4], scope, field_schemas)
                if method_type != TYPE_TEXT or path_type != TYPE_TEXT or body_type != TYPE_TEXT:
                    self.error("web.request_context krever tekst, tekst, ordbok, ordbok og tekst")
                if not self.is_map_type(query_type) or not self.is_map_type(headers_type):
                    self.error("web.request_context krever ordbok_tekst for query og headers")
                return TYPE_MAP_TEXT

            if full_name in {"web.request_method", "std.web.request_method", "web.request_path", "std.web.request_path", "web.request_body", "std.web.request_body"}:
                if len(expr.args) != 1:
                    self.error(f"{expr.module_name}.{expr.func_name} forventer 1 argument")
                ctx_type = self.check_expr(expr.args[0], scope, field_schemas)
                if not self.is_map_type(ctx_type):
                    self.error(f"{expr.module_name}.{expr.func_name} krever ordbok")
                if full_name.endswith("request_method") or full_name.endswith("request_path") or full_name.endswith("request_body"):
                    return TYPE_TEXT

            if full_name in {"web.request_query", "std.web.request_query", "web.request_headers", "std.web.request_headers"}:
                if len(expr.args) != 1:
                    self.error(f"{expr.module_name}.{expr.func_name} forventer 1 argument")
                ctx_type = self.check_expr(expr.args[0], scope, field_schemas)
                if not self.is_map_type(ctx_type):
                    self.error(f"{expr.module_name}.{expr.func_name} krever ordbok")
                return TYPE_MAP_TEXT

            if full_name in {"web.request_params", "std.web.request_params"}:
                if len(expr.args) != 1:
                    self.error("web.request_params forventer 1 argument")
                ctx_type = self.check_expr(expr.args[0], scope, field_schemas)
                if not self.is_map_type(ctx_type):
                    self.error("web.request_params krever ordbok")
                return TYPE_MAP_TEXT

            if full_name in {"web.request_param", "std.web.request_param"}:
                if len(expr.args) != 2:
                    self.error("web.request_param forventer 2 argumenter")
                ctx_type = self.check_expr(expr.args[0], scope, field_schemas)
                key_type = self.check_expr(expr.args[1], scope, field_schemas)
                if not self.is_map_type(ctx_type) or key_type != TYPE_TEXT:
                    self.error("web.request_param krever ordbok og tekst")
                return TYPE_TEXT

            if full_name in {"web.request_param_int", "std.web.request_param_int"}:
                if len(expr.args) != 2:
                    self.error("web.request_param_int forventer 2 argumenter")
                ctx_type = self.check_expr(expr.args[0], scope, field_schemas)
                key_type = self.check_expr(expr.args[1], scope, field_schemas)
                if not self.is_map_type(ctx_type) or key_type != TYPE_TEXT:
                    self.error("web.request_param_int krever ordbok og tekst")
                return TYPE_INT

            if full_name in {"web.request_query_required", "std.web.request_query_required"}:
                if len(expr.args) != 2:
                    self.error("web.request_query_required forventer 2 argumenter")
                ctx_type = self.check_expr(expr.args[0], scope, field_schemas)
                key_type = self.check_expr(expr.args[1], scope, field_schemas)
                if not self.is_map_type(ctx_type) or key_type != TYPE_TEXT:
                    self.error("web.request_query_required krever ordbok og tekst")
                return TYPE_TEXT

            if full_name in {"web.request_query_int", "std.web.request_query_int"}:
                if len(expr.args) != 2:
                    self.error("web.request_query_int forventer 2 argumenter")
                ctx_type = self.check_expr(expr.args[0], scope, field_schemas)
                key_type = self.check_expr(expr.args[1], scope, field_schemas)
                if not self.is_map_type(ctx_type) or key_type != TYPE_TEXT:
                    self.error("web.request_query_int krever ordbok og tekst")
                return TYPE_INT

            if full_name in {"web.request_json", "std.web.request_json"}:
                if len(expr.args) != 1:
                    self.error("web.request_json forventer 1 argument")
                ctx_type = self.check_expr(expr.args[0], scope, field_schemas)
                if not self.is_map_type(ctx_type):
                    self.error("web.request_json krever ordbok")
                return TYPE_MAP_TEXT

            if full_name in {"web.request_json_field", "std.web.request_json_field"}:
                if len(expr.args) != 2:
                    self.error("web.request_json_field forventer 2 argumenter")
                ctx_type = self.check_expr(expr.args[0], scope, field_schemas)
                key_type = self.check_expr(expr.args[1], scope, field_schemas)
                if not self.is_map_type(ctx_type) or key_type != TYPE_TEXT:
                    self.error("web.request_json_field krever ordbok og tekst")
                return TYPE_TEXT

            if full_name in {"web.request_json_field_or", "std.web.request_json_field_or"}:
                if len(expr.args) != 3:
                    self.error("web.request_json_field_or forventer 3 argumenter")
                ctx_type = self.check_expr(expr.args[0], scope, field_schemas)
                key_type = self.check_expr(expr.args[1], scope, field_schemas)
                fallback_type = self.check_expr(expr.args[2], scope, field_schemas)
                if not self.is_map_type(ctx_type) or key_type != TYPE_TEXT or fallback_type != TYPE_TEXT:
                    self.error("web.request_json_field_or krever ordbok og tekst")
                return TYPE_TEXT

            if full_name in {"web.request_json_field_int", "std.web.request_json_field_int"}:
                if len(expr.args) != 2:
                    self.error("web.request_json_field_int forventer 2 argumenter")
                ctx_type = self.check_expr(expr.args[0], scope, field_schemas)
                key_type = self.check_expr(expr.args[1], scope, field_schemas)
                if not self.is_map_type(ctx_type) or key_type != TYPE_TEXT:
                    self.error("web.request_json_field_int krever ordbok og tekst")
                return TYPE_INT

            if full_name in {"web.request_json_field_bool", "std.web.request_json_field_bool"}:
                if len(expr.args) != 2:
                    self.error("web.request_json_field_bool forventer 2 argumenter")
                ctx_type = self.check_expr(expr.args[0], scope, field_schemas)
                key_type = self.check_expr(expr.args[1], scope, field_schemas)
                if not self.is_map_type(ctx_type) or key_type != TYPE_TEXT:
                    self.error("web.request_json_field_bool krever ordbok og tekst")
                return TYPE_BOOL

            if full_name in {"web.request_query_param", "std.web.request_query_param", "web.request_header", "std.web.request_header"}:
                if len(expr.args) != 2:
                    self.error(f"{expr.module_name}.{expr.func_name} forventer 2 argumenter")
                ctx_type = self.check_expr(expr.args[0], scope, field_schemas)
                key_type = self.check_expr(expr.args[1], scope, field_schemas)
                if not self.is_map_type(ctx_type) or key_type != TYPE_TEXT:
                    self.error(f"{expr.module_name}.{expr.func_name} krever ordbok og tekst")
                return TYPE_TEXT

            if full_name in {"web.response_builder", "std.web.response_builder"}:
                if len(expr.args) != 3:
                    self.error("web.response_builder forventer 3 argumenter")
                status_type = self.check_expr(expr.args[0], scope, field_schemas)
                headers_type = self.check_expr(expr.args[1], scope, field_schemas)
                body_type = self.check_expr(expr.args[2], scope, field_schemas)
                if status_type != TYPE_INT or body_type != TYPE_TEXT:
                    self.error("web.response_builder krever heltall, ordbok og tekst")
                if not self.is_map_type(headers_type):
                    self.error("web.response_builder krever ordbok_tekst for headers")
                return TYPE_MAP_TEXT

            if full_name in {"web.response_status", "std.web.response_status"}:
                if len(expr.args) != 1:
                    self.error("web.response_status forventer 1 argument")
                ctx_type = self.check_expr(expr.args[0], scope, field_schemas)
                if not self.is_map_type(ctx_type):
                    self.error("web.response_status krever ordbok")
                return TYPE_INT

            if full_name in {"web.response_headers", "std.web.response_headers", "web.response_body", "std.web.response_body", "web.response_text", "std.web.response_text", "web.response_json", "std.web.response_json"}:
                if len(expr.args) != 1:
                    self.error(f"{expr.module_name}.{expr.func_name} forventer 1 argument")
                ctx_type = self.check_expr(expr.args[0], scope, field_schemas)
                if not self.is_map_type(ctx_type):
                    self.error(f"{expr.module_name}.{expr.func_name} krever ordbok")
                if full_name.endswith("response_headers") or full_name.endswith("response_json"):
                    return TYPE_MAP_TEXT
                return TYPE_TEXT

            if full_name in {"web.response_header", "std.web.response_header"}:
                if len(expr.args) != 2:
                    self.error("web.response_header forventer 2 argumenter")
                ctx_type = self.check_expr(expr.args[0], scope, field_schemas)
                key_type = self.check_expr(expr.args[1], scope, field_schemas)
                if not self.is_map_type(ctx_type) or key_type != TYPE_TEXT:
                    self.error("web.response_header krever ordbok og tekst")
                return TYPE_TEXT

            if full_name in {"web.response_error", "std.web.response_error"}:
                if len(expr.args) != 2:
                    self.error("web.response_error forventer 2 argumenter")
                status_type = self.check_expr(expr.args[0], scope, field_schemas)
                message_type = self.check_expr(expr.args[1], scope, field_schemas)
                if status_type != TYPE_INT or message_type != TYPE_TEXT:
                    self.error("web.response_error krever heltall og tekst")
                return TYPE_MAP_TEXT

            if full_name in {"web.response_file", "std.web.response_file"}:
                if len(expr.args) != 2:
                    self.error("web.response_file forventer 2 argumenter")
                path_type = self.check_expr(expr.args[0], scope, field_schemas)
                content_type_type = self.check_expr(expr.args[1], scope, field_schemas)
                if path_type != TYPE_TEXT or content_type_type != TYPE_TEXT:
                    self.error("web.response_file krever tekst og tekst")
                return TYPE_MAP_TEXT

            if full_name in {
                "web.request_middleware", "std.web.request_middleware",
                "web.response_middleware", "std.web.response_middleware",
                "web.error_middleware", "std.web.error_middleware",
                "web.startup_hook", "std.web.startup_hook",
                "web.shutdown_hook", "std.web.shutdown_hook",
            }:
                if expr.args:
                    self.error(f"{expr.module_name}.{expr.func_name} forventer 0 argumenter")
                return TYPE_TEXT

            if full_name in {"web.request_id", "std.web.request_id"}:
                if len(expr.args) != 1:
                    self.error("web.request_id forventer 1 argument")
                ctx_type = self.check_expr(expr.args[0], scope, field_schemas)
                if not self.is_map_type(ctx_type):
                    self.error("web.request_id krever ordbok")
                return TYPE_TEXT

            if full_name in {"web.startup", "std.web.startup", "web.shutdown", "std.web.shutdown"}:
                if expr.args:
                    self.error(f"{expr.module_name}.{expr.func_name} forventer 0 argumenter")
                return TYPE_INT

            if full_name in {"web.openapi_json", "std.web.openapi_json", "web.docs_html", "std.web.docs_html"}:
                if len(expr.args) != 2:
                    self.error(f"{expr.module_name}.{expr.func_name} forventer 2 argumenter")
                title_type = self.check_expr(expr.args[0], scope, field_schemas)
                version_type = self.check_expr(expr.args[1], scope, field_schemas)
                if title_type != TYPE_TEXT or version_type != TYPE_TEXT:
                    self.error(f"{expr.module_name}.{expr.func_name} krever tekst og tekst")
                return TYPE_TEXT

            if full_name in {"web.route", "std.web.route"}:
                if len(expr.args) != 1:
                    self.error("web.route forventer 1 argument")
                spec_type = self.check_expr(expr.args[0], scope, field_schemas)
                if spec_type != TYPE_TEXT:
                    self.error("web.route krever tekst")
                if self.current_function is not None and isinstance(expr.args[0], StringNode):
                    setattr(self.current_function, "route_spec", expr.args[0].value)
                return TYPE_TEXT

            if full_name in {"web.dependency", "std.web.dependency"}:
                if len(expr.args) != 1:
                    self.error("web.dependency forventer 1 argument")
                dep_type = self.check_expr(expr.args[0], scope, field_schemas)
                if dep_type != TYPE_TEXT:
                    self.error("web.dependency krever tekst")
                if self.current_function is not None and isinstance(expr.args[0], StringNode):
                    deps = list(getattr(self.current_function, "provided_dependencies", []) or [])
                    deps.append(expr.args[0].value)
                    setattr(self.current_function, "provided_dependencies", deps)
                return TYPE_TEXT

            if full_name in {"web.use_dependency", "std.web.use_dependency"}:
                if len(expr.args) != 1:
                    self.error("web.use_dependency forventer 1 argument")
                dep_type = self.check_expr(expr.args[0], scope, field_schemas)
                if dep_type != TYPE_TEXT:
                    self.error("web.use_dependency krever tekst")
                if self.current_function is not None and isinstance(expr.args[0], StringNode):
                    deps = list(getattr(self.current_function, "dependency_names", []) or [])
                    deps.append(expr.args[0].value)
                    setattr(self.current_function, "dependency_names", deps)
                return TYPE_TEXT

            if full_name in {"web.handle_request", "std.web.handle_request"}:
                if len(expr.args) != 1:
                    self.error("web.handle_request forventer 1 argument")
                ctx_type = self.check_expr(expr.args[0], scope, field_schemas)
                if not self.is_map_type(ctx_type):
                    self.error("web.handle_request krever ordbok")
                return TYPE_MAP_TEXT

            if full_name in {"web.request_dependency", "std.web.request_dependency"}:
                if len(expr.args) != 2:
                    self.error("web.request_dependency forventer 2 argumenter")
                ctx_type = self.check_expr(expr.args[0], scope, field_schemas)
                dep_type = self.check_expr(expr.args[1], scope, field_schemas)
                if not self.is_map_type(ctx_type) or dep_type != TYPE_TEXT:
                    self.error("web.request_dependency krever ordbok og tekst")
                return TYPE_MAP_TEXT

            if full_name in {"vent.timeout", "std.vent.timeout"}:
                if len(expr.args) != 2:
                    self.error("vent.timeout forventer 2 argumenter")
                value_type = self.check_expr(expr.args[0], scope, field_schemas)
                timeout_type = self.check_expr(expr.args[1], scope, field_schemas)
                if timeout_type != TYPE_INT:
                    self.error("vent.timeout krever heltall som varighet")
                return value_type

            if full_name in {"vent.kanseller", "std.vent.kanseller"}:
                if len(expr.args) != 1:
                    self.error("vent.kanseller forventer 1 argument")
                return self.check_expr(expr.args[0], scope, field_schemas)

            if full_name in {"vent.er_kansellert", "std.vent.er_kansellert", "vent.er_timeoutet", "std.vent.er_timeoutet"}:
                if len(expr.args) != 1:
                    self.error(f"{expr.module_name}.{expr.func_name} forventer 1 argument")
                self.check_expr(expr.args[0], scope, field_schemas)
                return TYPE_BOOL

            if full_name in {"vent.sov", "std.vent.sov"}:
                if len(expr.args) != 1:
                    self.error("vent.sov forventer 1 argument")
                timeout_type = self.check_expr(expr.args[0], scope, field_schemas)
                if timeout_type != TYPE_INT:
                    self.error("vent.sov krever heltall")
                return TYPE_INT

            if full_name in {"feil.verdi", "std.feil.verdi"}:
                if len(expr.args) != 1:
                    self.error("feil.verdi forventer 1 argument")
                message_type = self.check_expr(expr.args[0], scope, field_schemas)
                if message_type != TYPE_TEXT:
                    self.error("feil.verdi krever tekst")
                return TYPE_TEXT

            if full_name not in self.functions:
                self.error(f"Ukjent modulfunksjon: {expr.module_name}.{expr.func_name}")

            fn = self.functions[full_name]

            if len(expr.args) != len(fn.params):
                self.error(
                    f"Funksjonen '{expr.module_name}.{expr.func_name}' forventer {len(fn.params)} argument(er), fikk {len(expr.args)}"
                )

            for i, (arg, param_type) in enumerate(zip(expr.args, fn.params), start=1):
                arg_type = self.check_expr(arg, scope, field_schemas)
                if param_type is not None:
                    self.ensure_assignable(param_type, arg_type, f"argument {i} til {expr.module_name}.{expr.func_name}", arg)

            return fn.return_type

        if isinstance(expr, CallNode):
            lambda_value = scope.get(expr.name)
            if isinstance(lambda_value, LambdaSignature):
                if len(expr.args) != len(lambda_value.params):
                    self.error(
                        f"Lambda '{expr.name}' forventer {len(lambda_value.params)} argument(er), fikk {len(expr.args)}"
                    )
                for i, (arg, param_type) in enumerate(zip(expr.args, lambda_value.params), start=1):
                    arg_type = self.check_expr(arg, scope, field_schemas)
                    if param_type is not None:
                        self.ensure_assignable(param_type, arg_type, f"argument {i} til lambda '{expr.name}'", arg)
                setattr(expr, "call_kind", "lambda")
                setattr(expr, "lambda_signature", lambda_value)
                return lambda_value.return_type

            fn, resolved_name = self.resolve_function_symbol(expr.name)
            if fn is None:
                self.error(f"Ukjent funksjon: {expr.name}")

            if expr.name == "assert":
                if len(expr.args) != 1:
                    self.error("assert forventer 1 argument")
                cond_type = self.check_expr(expr.args[0], scope, field_schemas)
                self.ensure_bool(cond_type, "assert")
                return TYPE_INT

            if expr.name in ("assert_eq", "assert_ne"):
                if len(expr.args) != 2:
                    self.error(f"{expr.name} forventer 2 argumenter")
                left_type = self.check_expr(expr.args[0], scope, field_schemas)
                right_type = self.check_expr(expr.args[1], scope, field_schemas)
                if left_type != right_type:
                    self.error(f"{expr.name} krever samme type på begge sider")
                return TYPE_INT

            if expr.name in ("assert_starter_med", "assert_slutter_med", "assert_inneholder"):
                if len(expr.args) != 2:
                    self.error(f"{expr.name} forventer 2 argumenter")
                text_type = self.check_expr(expr.args[0], scope, field_schemas)
                needle_type = self.check_expr(expr.args[1], scope, field_schemas)
                if text_type != TYPE_TEXT or needle_type != TYPE_TEXT:
                    self.error(f"{expr.name} krever tekst på begge sider")
                return TYPE_INT

            if expr.name == "lengde":
                if len(expr.args) != 1:
                    self.error("lengde forventer 1 argument")
                arg_type = self.check_expr(expr.args[0], scope, field_schemas)
                if arg_type not in (TYPE_LIST_INT, TYPE_LIST_TEXT) and not self.is_map_type(arg_type):
                    self.error("lengde støtter bare lister og ordbøker")
                return TYPE_INT

            if expr.name in ("sti_join", "miljo_sett"):
                expected_args = 2
                if len(expr.args) != expected_args:
                    self.error(f"{expr.name} forventer {expected_args} argumenter")
                left_type = self.check_expr(expr.args[0], scope, field_schemas)
                right_type = self.check_expr(expr.args[1], scope, field_schemas)
                if left_type != TYPE_TEXT or right_type != TYPE_TEXT:
                    self.error(f"{expr.name} krever tekst på begge sider")
                return TYPE_TEXT

            if expr.name in ("sti_basename", "sti_dirname", "sti_stem", "sti_exists", "miljo_hent", "miljo_finnes"):
                if len(expr.args) != 1:
                    self.error(f"{expr.name} forventer 1 argument")
                arg_type = self.check_expr(expr.args[0], scope, field_schemas)
                if arg_type != TYPE_TEXT:
                    self.error(f"{expr.name} krever tekst")
                if expr.name in ("sti_exists", "miljo_finnes"):
                    return TYPE_BOOL
                return TYPE_TEXT

            if expr.name in ("fil_skriv", "fil_append"):
                if len(expr.args) != 2:
                    self.error(f"{expr.name} forventer 2 argumenter")
                path_type = self.check_expr(expr.args[0], scope, field_schemas)
                text_type = self.check_expr(expr.args[1], scope, field_schemas)
                if path_type != TYPE_TEXT or text_type != TYPE_TEXT:
                    self.error(f"{expr.name} krever tekst for både sti og innhold")
                return TYPE_TEXT

            if expr.name in ("fil_les", "fil_finnes"):
                if len(expr.args) != 1:
                    self.error(f"{expr.name} forventer 1 argument")
                arg_type = self.check_expr(expr.args[0], scope, field_schemas)
                if arg_type != TYPE_TEXT:
                    self.error(f"{expr.name} krever tekst")
                if expr.name == "fil_finnes":
                    return TYPE_BOOL
                return TYPE_TEXT

            if expr.name in ("vent_timeout", "vent_kanseller", "vent_er_kansellert", "vent_er_timeoutet", "vent_sov"):
                if expr.name == "vent_timeout":
                    if len(expr.args) != 2:
                        self.error("vent.timeout forventer 2 argumenter")
                    self.check_expr(expr.args[0], scope, field_schemas)
                    timeout_type = self.check_expr(expr.args[1], scope, field_schemas)
                    if timeout_type != TYPE_INT:
                        self.error("vent.timeout krever heltall som varighet")
                    return self.check_expr(expr.args[0], scope, field_schemas)
                if expr.name == "vent_kanseller":
                    if len(expr.args) != 1:
                        self.error("vent.kanseller forventer 1 argument")
                    return self.check_expr(expr.args[0], scope, field_schemas)
                if expr.name in ("vent_er_kansellert", "vent_er_timeoutet"):
                    if len(expr.args) != 1:
                        self.error(f"{expr.name.replace('_', '.')} forventer 1 argument")
                    self.check_expr(expr.args[0], scope, field_schemas)
                    return TYPE_BOOL
                if expr.name == "vent_sov":
                    if len(expr.args) != 1:
                        self.error("vent.sov forventer 1 argument")
                    timeout_type = self.check_expr(expr.args[0], scope, field_schemas)
                    if timeout_type != TYPE_INT:
                        self.error("vent.sov krever heltall")
                    return TYPE_INT

            if expr.name == "legg_til":
                if len(expr.args) != 2:
                    self.error("legg_til forventer 2 argumenter")
                list_type = self.check_expr(expr.args[0], scope, field_schemas)
                value_type = self.check_expr(expr.args[1], scope, field_schemas)
                if list_type == TYPE_LIST_INT and value_type == TYPE_INT:
                    return TYPE_INT
                if list_type == TYPE_LIST_TEXT and value_type == TYPE_TEXT:
                    return TYPE_INT
                self.error("legg_til krever liste og riktig type")

            if expr.name == "pop_siste":
                if len(expr.args) != 1:
                    self.error("pop_siste forventer 1 argument")
                list_type = self.check_expr(expr.args[0], scope, field_schemas)
                if list_type == TYPE_LIST_INT:
                    return TYPE_INT
                if list_type == TYPE_LIST_TEXT:
                    return TYPE_TEXT
                self.error("pop_siste krever liste")

            if expr.name == "fjern_indeks":
                if len(expr.args) != 2:
                    self.error("fjern_indeks forventer 2 argumenter")
                list_type = self.check_expr(expr.args[0], scope, field_schemas)
                idx_type = self.check_expr(expr.args[1], scope, field_schemas)
                if list_type not in (TYPE_LIST_INT, TYPE_LIST_TEXT) or idx_type != TYPE_INT:
                    self.error("fjern_indeks krever liste og heltall")
                return TYPE_INT

            if expr.name == "sett_inn":
                if len(expr.args) != 3:
                    self.error("sett_inn forventer 3 argumenter")
                list_type = self.check_expr(expr.args[0], scope, field_schemas)
                idx_type = self.check_expr(expr.args[1], scope, field_schemas)
                val_type = self.check_expr(expr.args[2], scope, field_schemas)
                if idx_type != TYPE_INT:
                    self.error("sett_inn krever heltall som indeks")
                if list_type == TYPE_LIST_INT and val_type == TYPE_INT:
                    return TYPE_INT
                if list_type == TYPE_LIST_TEXT and val_type == TYPE_TEXT:
                    return TYPE_INT
                self.error("sett_inn krever liste og verdi av riktig type")

            if expr.name == "har_nokkel":
                if len(expr.args) != 2:
                    self.error("har_nokkel forventer 2 argumenter")
                map_type = self.check_expr(expr.args[0], scope, field_schemas)
                key_type = self.check_expr(expr.args[1], scope, field_schemas)
                if not self.is_map_type(map_type) or key_type != TYPE_TEXT:
                    self.error("har_nokkel krever ordbok og tekstnøkkel")
                return TYPE_BOOL

            if expr.name == "fjern_nokkel":
                if len(expr.args) != 2:
                    self.error("fjern_nokkel forventer 2 argumenter")
                map_type = self.check_expr(expr.args[0], scope, field_schemas)
                key_type = self.check_expr(expr.args[1], scope, field_schemas)
                if not self.is_map_type(map_type) or key_type != TYPE_TEXT:
                    self.error("fjern_nokkel krever ordbok og tekstnøkkel")
                return TYPE_INT

            if len(expr.args) != len(fn.params):
                self.error(
                    f"Funksjonen '{resolved_name}' forventer {len(fn.params)} argument(er), fikk {len(expr.args)}"
                )
            for i, (arg, param_type) in enumerate(zip(expr.args, fn.params), start=1):
                arg_type = self.check_expr(arg, scope, field_schemas)
                if param_type is not None:
                    self.ensure_assignable(param_type, arg_type, f"argument {i} til {resolved_name}", arg)
            return fn.return_type

        self.error(f"Ukjent expression-type: {type(expr).__name__}")

    def ensure_assignable(self, expected, actual, where, actual_expr=None):
        if isinstance(expected, LambdaSignature) and isinstance(actual, LambdaSignature):
            if expected.params == actual.params and expected.return_type == actual.return_type:
                return
        elif expected == actual:
            return

        if isinstance(expected, LambdaSignature) or isinstance(actual, LambdaSignature):
            self.error(f"Typefeil i {where}: forventet {expected}, fikk {actual}")

        if expected != actual:
            if actual_expr is not None and self.is_empty_list_expr(actual_expr) and expected in (TYPE_LIST_INT, TYPE_LIST_TEXT):
                return
            if actual_expr is not None and self.is_empty_map_expr(actual_expr) and self.is_map_type(expected):
                return
            self.error(f"Typefeil i {where}: forventet {expected}, fikk {actual}")

    def ensure_bool(self, actual, where):
        if actual != TYPE_BOOL:
            self.error(f"Typefeil i {where}: forventet bool, fikk {actual}")
