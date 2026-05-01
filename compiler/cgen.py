from dataclasses import dataclass
import re
import json
from typing import Any

from .ast_nodes import *


@dataclass
class LambdaCType:
    helper_name: str
    closure_name: str
    capture_names: list[str]
    capture_types: list[Any]
    param_names: list[str]
    param_types: list[Any]
    return_type: Any


class CGenerator:
    def __init__(self, function_symbols, alias_map=None):
        self.function_symbols = function_symbols
        self.alias_map = alias_map or {}
        self.lines = []
        self.indent = 0
        self.var_types = {}
        self.current_module = "__main__"
        self.current_return_type = None
        self.temp_counter = 0
        self.function_name_map = {}
        self.name_alias_stack = [{}]
        self.lambda_counter = 0
        self.lambda_specs: dict[str, dict[str, Any]] = {}
        self.lambda_order: list[str] = []
        self.emitted_lambda_types: set[str] = set()
        self.emitted_lambda_prototypes: set[str] = set()
        self.emitted_lambda_defs: set[str] = set()
        self.route_handlers: list[dict[str, Any]] = []
        self.dependency_providers: list[dict[str, Any]] = []
        self.request_middlewares: list[str] = []
        self.response_middlewares: list[str] = []
        self.error_middlewares: list[str] = []
        self.startup_hooks: list[str] = []
        self.shutdown_hooks: list[str] = []
        self.openapi_paths_json = "{}"
        self._build_function_name_map()

    def _build_function_name_map(self):
        for key, symbol in self.function_symbols.items():
            if getattr(symbol, "builtin", False):
                continue
            module_name = getattr(symbol, "module_name", None)
            short_name = key.split(".")[-1]
            self.function_name_map[key] = self.mangle_function_name(module_name, short_name)
            if module_name and module_name != "__main__":
                self.function_name_map[f"{module_name}.{short_name}"] = self.mangle_function_name(module_name, short_name)
            else:
                self.function_name_map[short_name] = self.mangle_function_name(module_name, short_name)

    def emit(self, line=""):
        self.lines.append("    " * self.indent + line)

    def push_name_aliases(self, aliases):
        merged = dict(self.name_alias_stack[-1])
        merged.update(aliases)
        self.name_alias_stack.append(merged)

    def pop_name_aliases(self):
        if len(self.name_alias_stack) > 1:
            self.name_alias_stack.pop()

    def resolve_name(self, name):
        return self.name_alias_stack[-1].get(name, name)

    def _normalize_web_method(self, value):
        return str(value or "").strip().upper()

    def _split_web_route_spec(self, spec):
        text = str(spec or "").strip()
        if not text:
            return "", ""
        parts = text.split(maxsplit=1)
        if len(parts) == 1:
            return "", parts[0]
        method = self._normalize_web_method(parts[0])
        path = parts[1].strip()
        return method, path

    def _combine_web_route_prefix(self, prefix, spec):
        route_prefix = str(prefix or "").strip()
        route_spec = str(spec or "").strip()
        if not route_prefix:
            return route_spec
        method, route_path = self._split_web_route_spec(route_spec)
        if not route_path:
            return route_spec
        normalized_prefix = "/" + route_prefix.lstrip("/")
        if normalized_prefix.endswith("/"):
            normalized_prefix = normalized_prefix.rstrip("/")
        normalized_path = route_path if route_path.startswith("/") else f"/{route_path}"
        if normalized_path == "/":
            combined_path = normalized_prefix or "/"
        else:
            combined_path = normalized_prefix + normalized_path
        return f"{method} {combined_path}".strip() if method else combined_path

    def _walk_ast_nodes(self, node):
        if node is None:
            return
        if isinstance(node, (str, int, float, bool)):
            return
        if isinstance(node, list):
            for item in node:
                yield from self._walk_ast_nodes(item)
            return
        if isinstance(node, tuple):
            for item in node:
                yield from self._walk_ast_nodes(item)
            return
        if hasattr(node, "__dict__"):
            yield node
            for value in vars(node).values():
                yield from self._walk_ast_nodes(value)

    def _schema_for_type(self, type_name):
        if type_name == TYPE_INT:
            return {"type": "integer"}
        if type_name == TYPE_BOOL:
            return {"type": "boolean"}
        if type_name == TYPE_TEXT:
            return {"type": "string"}
        if type_name == TYPE_LIST_INT:
            return {"type": "array", "items": {"type": "integer"}}
        if type_name == TYPE_LIST_TEXT:
            return {"type": "array", "items": {"type": "string"}}
        if type_name == TYPE_MAP_INT:
            return {"type": "object", "additionalProperties": {"type": "integer"}}
        if type_name == TYPE_MAP_TEXT:
            return {"type": "object", "additionalProperties": {"type": "string"}}
        if type_name == TYPE_MAP_BOOL:
            return {"type": "object", "additionalProperties": {"type": "boolean"}}
        if isinstance(type_name, str) and type_name.startswith("liste_"):
            return {"type": "array", "items": {"type": "string"}}
        if isinstance(type_name, str) and type_name.startswith("ordbok_"):
            return {"type": "object", "additionalProperties": {"type": "string"}}
        return {"type": "string"}

    def _sample_for_type(self, type_name):
        if type_name == TYPE_INT:
            return 1
        if type_name == TYPE_BOOL:
            return True
        if type_name == TYPE_TEXT:
            return "eksempel"
        if type_name == TYPE_LIST_INT:
            return [1]
        if type_name == TYPE_LIST_TEXT:
            return ["eksempel"]
        if type_name == TYPE_MAP_INT:
            return {"verdi": 1}
        if type_name == TYPE_MAP_BOOL:
            return {"verdi": True}
        if isinstance(type_name, str) and type_name.startswith("liste_"):
            return ["eksempel"]
        if isinstance(type_name, str) and type_name.startswith("ordbok_"):
            return {"verdi": "eksempel"}
        return "eksempel"

    def _route_docs_from_function(self, fn, spec):
        method, route_path = self._split_web_route_spec(spec)
        path_params = []
        query_params = []
        body_fields = []
        seen_path = set()
        seen_query = set()
        seen_body = set()
        for node in self._walk_ast_nodes(getattr(fn, "body", None)):
            if not isinstance(node, ModuleCallNode):
                continue
            module_name = getattr(node, "module_name", "")
            func_name = getattr(node, "func_name", "")
            args = getattr(node, "args", []) or []
            if module_name not in {"web", "std.web"} or len(args) < 2 or not isinstance(args[1], StringNode):
                continue
            key = args[1].value
            if func_name == "request_param" and key not in seen_path:
                path_params.append({"name": key, "type": "string"})
                seen_path.add(key)
            elif func_name == "request_param_int" and key not in seen_path:
                path_params.append({"name": key, "type": "integer"})
                seen_path.add(key)
            elif func_name == "request_query_required" and key not in seen_query:
                query_params.append({"name": key, "type": "string", "required": True})
                seen_query.add(key)
            elif func_name == "request_query_int" and key not in seen_query:
                query_params.append({"name": key, "type": "integer", "required": True})
                seen_query.add(key)
            elif func_name == "request_query_param" and key not in seen_query:
                query_params.append({"name": key, "type": "string", "required": False})
                seen_query.add(key)
            elif func_name == "request_json_field" and key not in seen_body:
                body_fields.append({"name": key, "type": "string", "required": True})
                seen_body.add(key)
            elif func_name == "request_json_field_or" and key not in seen_body:
                body_fields.append({"name": key, "type": "string", "required": False})
                seen_body.add(key)
            elif func_name == "request_json_field_int" and key not in seen_body:
                body_fields.append({"name": key, "type": "integer", "required": True})
                seen_body.add(key)
            elif func_name == "request_json_field_bool" and key not in seen_body:
                body_fields.append({"name": key, "type": "boolean", "required": True})
                seen_body.add(key)
        path_template = re.sub(r"\{([^}:]+)(?::[^}]+)?\}", r"{\1}", route_path)
        request_example = {"method": (method or "GET").lower(), "path": path_template}
        if query_params:
            request_example["query"] = {item["name"]: self._sample_for_type(TYPE_INT if item["type"] == "integer" else TYPE_TEXT) for item in query_params}
        if body_fields:
            request_example["body"] = {
                item["name"]: self._sample_for_type(TYPE_INT if item["type"] == "integer" else TYPE_BOOL if item["type"] == "boolean" else TYPE_TEXT)
                for item in body_fields
            }
        response_type = getattr(fn, "return_type", TYPE_TEXT)
        return {
            "function": self.resolve_c_function_name(fn.name, module_name=getattr(fn, "module_name", None)),
            "method": (method or "GET").lower(),
            "path": path_template,
            "path_params": path_params,
            "query_params": query_params,
            "body_fields": body_fields,
            "response_type": response_type,
            "summary": getattr(fn, "name", ""),
            "operation_id": getattr(fn, "name", ""),
            "request_example": request_example,
            "response_example": self._sample_for_type(response_type),
            "response_schema": self._schema_for_type(response_type),
        }

    def _render_openapi_paths_json(self):
        paths = {}
        for handler in self.route_handlers:
            path = handler.get("path", "")
            method = str(handler.get("method", "get")).lower()
            path_item = paths.setdefault(path, {})
            parameters = []
            for item in handler.get("path_params", []):
                parameters.append({
                    "name": item["name"],
                    "in": "path",
                    "required": True,
                    "schema": {"type": item["type"]},
                })
            for item in handler.get("query_params", []):
                parameters.append({
                    "name": item["name"],
                    "in": "query",
                    "required": bool(item.get("required", False)),
                    "schema": {"type": item["type"]},
                })
            operation = {
                "operationId": handler.get("operation_id", handler.get("function", "")),
                "summary": handler.get("summary", handler.get("function", "")),
                "parameters": parameters,
                "responses": {
                    "200": {
                        "description": "OK",
                        "content": {
                            "application/json": {
                                "schema": handler.get("response_schema", {"type": "string"}),
                                "example": handler.get("response_example", "eksempel"),
                            }
                        },
                    }
                },
                "x-example-request": handler.get("request_example", {}),
            }
            body_fields = handler.get("body_fields", [])
            if body_fields:
                required_fields = [item["name"] for item in body_fields if item.get("required", False)]
                properties = {item["name"]: {"type": item["type"]} for item in body_fields}
                operation["requestBody"] = {
                    "required": True,
                    "content": {
                        "application/json": {
                            "schema": {
                                "type": "object",
                                "properties": properties,
                                "required": required_fields,
                            },
                            "example": handler.get("request_example", {}).get("body", {}),
                        }
                    },
                }
            path_item[method] = operation
        return json.dumps(paths, ensure_ascii=False)

    def _web_annotations_from_function(self, fn):
        statements = list(getattr(getattr(fn, "body", None), "statements", []) or [])
        if not statements:
            return None, [], [], False, False, False, False, False
        spec = None
        route_prefix = ""
        deps: list[str] = []
        provided: list[str] = []
        request_middleware = False
        response_middleware = False
        error_middleware = False
        startup_hook = False
        shutdown_hook = False
        for stmt in statements:
            if not isinstance(stmt, ExprStmtNode):
                break
            expr = stmt.expr
            if not isinstance(expr, ModuleCallNode):
                break
            if expr.module_name not in {"web", "std.web"}:
                break
            if not expr.args:
                if expr.func_name == "request_middleware":
                    request_middleware = True
                    continue
                if expr.func_name == "response_middleware":
                    response_middleware = True
                    continue
                if expr.func_name == "error_middleware":
                    error_middleware = True
                    continue
                if expr.func_name == "startup_hook":
                    startup_hook = True
                    continue
                if expr.func_name == "shutdown_hook":
                    shutdown_hook = True
                    continue
                break
            if not isinstance(expr.args[0], StringNode):
                break
            if expr.func_name == "route" and spec is None:
                spec = expr.args[0].value
                continue
            if expr.func_name in {"router", "subrouter"} and not route_prefix:
                route_prefix = expr.args[0].value
                continue
            if expr.func_name == "use_dependency":
                deps.append(expr.args[0].value)
                continue
            if expr.func_name == "dependency":
                provided.append(expr.args[0].value)
                continue
            break
        if route_prefix:
            setattr(fn, "route_prefix", route_prefix)
        return spec, deps, provided, request_middleware, response_middleware, error_middleware, startup_hook, shutdown_hook

    def _collect_route_handlers(self, tree):
        handlers = []
        providers = []
        request_middlewares = []
        response_middlewares = []
        error_middlewares = []
        startup_hooks = []
        shutdown_hooks = []
        for fn in getattr(tree, "functions", []):
            spec, deps, provided, request_middleware, response_middleware, error_middleware, startup_hook, shutdown_hook = self._web_annotations_from_function(fn)
            if spec is None:
                spec = None
            if spec is not None:
                route_prefix = getattr(fn, "route_prefix", "")
                combined_spec = self._combine_web_route_prefix(route_prefix, spec)
                route_docs = self._route_docs_from_function(fn, combined_spec)
                route_docs["deps"] = list(deps)
                route_docs["spec"] = combined_spec
                handlers.append(route_docs)
            for dep_name in provided:
                providers.append({
                    "name": dep_name,
                    "function": self.resolve_c_function_name(fn.name, module_name=getattr(fn, "module_name", None)),
                    "params": len(getattr(fn, "params", []) or []),
                })
            function_name = self.resolve_c_function_name(fn.name, module_name=getattr(fn, "module_name", None))
            if request_middleware:
                request_middlewares.append(function_name)
            if response_middleware:
                response_middlewares.append(function_name)
            if error_middleware:
                error_middlewares.append(function_name)
            if startup_hook:
                startup_hooks.append(function_name)
            if shutdown_hook:
                shutdown_hooks.append(function_name)
        self.request_middlewares = request_middlewares
        self.response_middlewares = response_middlewares
        self.error_middlewares = error_middlewares
        self.startup_hooks = startup_hooks
        self.shutdown_hooks = shutdown_hooks
        return handlers, providers

    def emit_web_route_dispatcher(self):
        self.emit("static int nl_web_route_match_params(const char *spec, const char *method, const char *path, nl_map_text *params) {")
        self.indent += 1
        self.emit("char *route_method = NULL;")
        self.emit("char *route_path = NULL;")
        self.emit("nl_web_split_route_spec(spec, &route_method, &route_path);")
        self.emit("int ok = 1;")
        self.emit("if (route_method && *route_method) {")
        self.indent += 1
        self.emit("char *method_copy = nl_strdup(method ? method : \"\");")
        self.emit("for (char *p = method_copy; p && *p; ++p) { *p = (char)toupper((unsigned char)*p); }")
        self.emit("ok = strcmp(route_method, method_copy) == 0;")
        self.emit("free(method_copy);")
        self.indent -= 1
        self.emit("}")
        self.emit("if (ok) { ok = nl_web_match_pattern(route_path, path, params); }")
        self.emit("free(route_method);")
        self.emit("free(route_path);")
        self.emit("return ok;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_web_route_path_match(const char *spec, const char *path) {")
        self.indent += 1
        self.emit("char *route_method = NULL;")
        self.emit("char *route_path = NULL;")
        self.emit("nl_web_split_route_spec(spec, &route_method, &route_path);")
        self.emit("int ok = nl_web_match_pattern(route_path, path, NULL);")
        self.emit("free(route_method);")
        self.emit("free(route_path);")
        self.emit("return ok;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static nl_map_text *nl_web_request_dependency(nl_map_text *ctx, const char *name) {")
        self.indent += 1
        self.emit("if (!name) { return nl_map_text_new(); }")
        for provider in self.dependency_providers:
            dep_name = str(provider["name"]).replace("\\", "\\\\").replace('"', '\\"')
            provider_fn = provider["function"]
            if provider["params"] > 0:
                call_expr = f"{provider_fn}(ctx)"
            else:
                call_expr = f"{provider_fn}()"
            self.emit(f"if (strcmp(name, \"{dep_name}\") == 0) {{")
            self.indent += 1
            self.emit(f"return {call_expr};")
            self.indent -= 1
            self.emit("}")
        self.emit("return nl_map_text_new();")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_web_startup_ran = 0;")
        self.emit("static int nl_web_shutdown_ran = 0;")
        self.emit("static int nl_web_run_startup_hooks(void) {")
        self.indent += 1
        self.emit("if (nl_web_startup_ran) { return 0; }")
        self.emit("nl_web_startup_ran = 1;")
        self.emit("int count = 0;")
        for hook in self.startup_hooks:
            self.emit(f"(void){hook}();")
            self.emit("count++;")
        self.emit("return count;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_web_run_shutdown_hooks(void) {")
        self.indent += 1
        self.emit("if (nl_web_shutdown_ran) { return 0; }")
        self.emit("nl_web_shutdown_ran = 1;")
        self.emit("int count = 0;")
        for hook in self.shutdown_hooks:
            self.emit(f"(void){hook}();")
            self.emit("count++;")
        self.emit("return count;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static nl_map_text *nl_web_apply_request_middlewares(nl_map_text *ctx) {")
        self.indent += 1
        self.emit("nl_map_text *current = ctx;")
        for middleware in self.request_middlewares:
            self.emit(f"{{ nl_map_text *result = {middleware}(current); if (result) current = result; }}")
        self.emit("return current;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static nl_map_text *nl_web_apply_response_middlewares(nl_map_text *response) {")
        self.indent += 1
        self.emit("nl_map_text *current = response;")
        for middleware in self.response_middlewares:
            self.emit(f"{{ nl_map_text *result = {middleware}(current); if (result) current = result; }}")
        self.emit("return current;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static nl_map_text *nl_web_apply_error_middlewares(nl_map_text *response) {")
        self.indent += 1
        self.emit("nl_map_text *current = response;")
        for middleware in self.error_middlewares:
            self.emit(f"{{ nl_map_text *result = {middleware}(current); if (result) current = result; }}")
        self.emit("return current;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static nl_map_text *nl_web_handle_request(nl_map_text *ctx) {")
        self.indent += 1
        self.emit("nl_try_frame req_try;")
        self.emit("nl_push_try_frame(&req_try);")
        self.emit("if (setjmp(req_try.jump_point) == 0) {")
        self.indent += 1
        self.emit("nl_web_run_startup_hooks();")
        self.emit("nl_map_text *current_ctx = nl_web_apply_request_middlewares(ctx);")
        self.emit("char *method = nl_web_request_method(current_ctx);")
        self.emit("char *path = nl_web_request_path(current_ctx);")
        self.emit("int path_mismatch = 0;")
        self.emit("nl_map_text *result = NULL;")
        exact_handlers = [handler for handler in self.route_handlers if "{" not in handler["spec"] and "}" not in handler["spec"]]
        param_handlers = [handler for handler in self.route_handlers if handler not in exact_handlers]
        for handler in exact_handlers + param_handlers:
            spec = handler["spec"].replace("\\", "\\\\").replace('"', '\\"')
            self.emit(f"{{ /* {spec} */")
            self.indent += 1
            self.emit("nl_map_text *params = nl_map_text_new();")
            self.emit(f"if (nl_web_route_match_params(\"{spec}\", method, path, params)) {{")
            self.indent += 1
            self.emit("nl_map_text *route_ctx = nl_web_request_context(method, path, nl_web_request_query(current_ctx), nl_web_request_headers(current_ctx), nl_web_request_body(current_ctx));")
            self.emit("nl_map_text_set(route_ctx, \"params\", json_stringify(params));")
            dep_args = ["route_ctx"]
            for dep_name in handler.get("deps", []):
                dep_escaped = str(dep_name).replace("\\", "\\\\").replace('"', '\\"')
                dep_args.append(f'nl_web_request_dependency(route_ctx, "{dep_escaped}")')
            self.emit(f"result = {handler['function']}({', '.join(dep_args)});")
            self.emit("goto nl_web_handle_request_done;")
            self.indent -= 1
            self.emit("}")
            self.emit("if (nl_web_route_path_match(\"%s\", path)) { path_mismatch = 1; }" % spec)
            self.indent -= 1
            self.emit("}")
        self.emit("nl_web_handle_request_done:")
        self.emit("nl_pop_try_frame();")
        self.emit("if (path_mismatch) {")
        self.indent += 1
        self.emit("return nl_web_apply_error_middlewares(nl_web_response_error(405, \"Metode ikke tillatt\"));")
        self.indent -= 1
        self.emit("}")
        self.emit("if (!result) {")
        self.indent += 1
        self.emit("return nl_web_apply_error_middlewares(nl_web_response_error(404, \"Ikke funnet\"));")
        self.indent -= 1
        self.emit("}")
        self.emit("return nl_web_apply_response_middlewares(result);")
        self.indent -= 1
        self.emit("} else {")
        self.indent += 1
        self.emit("nl_pop_try_frame();")
        self.emit("return nl_web_apply_error_middlewares(nl_web_response_error(500, req_try.error_message ? req_try.error_message : \"\"));")
        self.indent -= 1
        self.emit("}")
        self.indent -= 1
        self.emit("}")
        self.emit()

    def preview_expr_type(self, node, expected_type=None):
        lines = self.lines
        indent = self.indent
        temp_counter = self.temp_counter
        var_types = dict(self.var_types)
        name_alias_stack = [dict(scope) for scope in self.name_alias_stack]
        lambda_counter = self.lambda_counter
        lambda_specs = dict(self.lambda_specs)
        lambda_order = list(self.lambda_order)
        emitted_lambda_types = set(self.emitted_lambda_types)
        emitted_lambda_prototypes = set(self.emitted_lambda_prototypes)
        emitted_lambda_defs = set(self.emitted_lambda_defs)
        try:
            self.lines = []
            self.indent = 0
            result = self.expr_with_type(node, expected_type)
            return result
        finally:
            self.lines = lines
            self.indent = indent
            self.temp_counter = temp_counter
            self.var_types = var_types
            self.name_alias_stack = name_alias_stack
            self.lambda_counter = lambda_counter
            self.lambda_specs = lambda_specs
            self.lambda_order = lambda_order
            self.emitted_lambda_types = emitted_lambda_types
            self.emitted_lambda_prototypes = emitted_lambda_prototypes
            self.emitted_lambda_defs = emitted_lambda_defs

    def c_type(self, t):
        if isinstance(t, LambdaCType):
            return t.closure_name
        if is_map_type_name(t):
            if t == TYPE_MAP_INT:
                return "nl_map_int*"
            if t == TYPE_MAP_TEXT:
                return "nl_map_text*"
            if t == TYPE_MAP_BOOL:
                return "nl_map_bool*"
            return "nl_map_any*"
        return {
            TYPE_INT: "int",
            TYPE_BOOL: "int",
            TYPE_TEXT: "char *",
            TYPE_LIST_INT: "nl_list_int*",
            TYPE_LIST_TEXT: "nl_list_text*",
        }[t]

    def is_map_type(self, type_name):
        return is_map_type_name(type_name)

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

    def collect_free_vars(self, node, bound=None):
        bound = set(bound or [])
        if node is None:
            return set()
        if isinstance(node, (NumberNode, StringNode, BoolNode)):
            return set()
        if isinstance(node, VarAccessNode):
            return set() if node.name in bound else {node.name}
        if isinstance(node, LambdaNode):
            nested_bound = set(bound)
            nested_bound.update(param.name for param in getattr(node, "params", []))
            return self.collect_free_vars(node.body, nested_bound)
        if isinstance(node, ListLiteralNode):
            out = set()
            for item in node.items:
                out.update(self.collect_free_vars(item, bound))
            return out
        if isinstance(node, MapLiteralNode):
            out = set()
            for key_expr, value_expr in node.items:
                out.update(self.collect_free_vars(key_expr, bound))
                out.update(self.collect_free_vars(value_expr, bound))
            return out
        if isinstance(node, StructLiteralNode):
            out = set()
            for _field_name, value_expr in node.fields:
                out.update(self.collect_free_vars(value_expr, bound))
            return out
        if isinstance(node, CallNode):
            out = set()
            for arg in node.args:
                out.update(self.collect_free_vars(arg, bound))
            return out
        if isinstance(node, ModuleCallNode):
            out = set()
            for arg in node.args:
                out.update(self.collect_free_vars(arg, bound))
            return out
        if isinstance(node, IfExprNode):
            out = set()
            out.update(self.collect_free_vars(node.condition, bound))
            out.update(self.collect_free_vars(node.then_expr, bound))
            out.update(self.collect_free_vars(node.else_expr, bound))
            return out
        if isinstance(node, AwaitNode):
            return self.collect_free_vars(node.expr, bound)
        if isinstance(node, UnaryOpNode):
            return self.collect_free_vars(node.node, bound)
        if isinstance(node, BinOpNode):
            out = set()
            out.update(self.collect_free_vars(node.left, bound))
            out.update(self.collect_free_vars(node.right, bound))
            return out
        if isinstance(node, IndexNode):
            out = set()
            out.update(self.collect_free_vars(getattr(node, "list_expr", getattr(node, "target", None)), bound))
            out.update(self.collect_free_vars(getattr(node, "index_expr", None), bound))
            return out
        if isinstance(node, SliceNode):
            out = set()
            out.update(self.collect_free_vars(node.target, bound))
            out.update(self.collect_free_vars(node.start_expr, bound))
            out.update(self.collect_free_vars(node.end_expr, bound))
            return out
        if isinstance(node, FieldAccessNode):
            return self.collect_free_vars(node.target, bound)
        if isinstance(node, ListComprehensionNode):
            out = set()
            out.update(self.collect_free_vars(node.source_expr, bound))
            item_bound = set(bound)
            item_bound.add(node.item_name)
            if node.condition_expr is not None:
                out.update(self.collect_free_vars(node.condition_expr, item_bound))
            out.update(self.collect_free_vars(node.item_expr, item_bound))
            return out
        return set()

    def register_lambda(self, node):
        key = getattr(node, "_lambda_c_key", None)
        if key is not None and key in self.lambda_specs:
            return self.lambda_specs[key]["ctype"]

        capture_names = sorted(self.collect_free_vars(node.body, {param.name for param in getattr(node, "params", [])}))
        capture_types = [self.var_types.get(name, TYPE_INT) for name in capture_names]
        param_names = [param.name for param in getattr(node, "params", [])]
        param_types = [param.type_name or TYPE_INT for param in getattr(node, "params", [])]
        return_type = getattr(node, "return_type", TYPE_INT)
        helper_name = f"__nl_lambda_{self.lambda_counter}"
        closure_name = f"{helper_name}_closure"
        ctype = LambdaCType(helper_name, closure_name, capture_names, capture_types, param_names, param_types, return_type)
        key = helper_name
        self.lambda_counter += 1
        self.lambda_specs[key] = {
            "node": node,
            "ctype": ctype,
            "capture_names": capture_names,
            "capture_types": capture_types,
            "param_names": param_names,
            "param_types": param_types,
            "return_type": return_type,
        }
        self.lambda_order.append(key)
        setattr(node, "_lambda_c_key", key)
        return ctype

    def lambda_signature_text(self, ctype: LambdaCType):
        parts = []
        for name, t in zip(ctype.capture_names, ctype.capture_types):
            parts.append(f"{self.c_type(t)} {name}")
        for name, t in zip(ctype.param_names, ctype.param_types):
            parts.append(f"{self.c_type(t)} {name}")
        args = ", ".join(parts) if parts else "void"
        return f"{self.c_type(ctype.return_type)} {ctype.helper_name}({args})"

    def emit_lambda_prototype(self, ctype: LambdaCType):
        if ctype.helper_name in self.emitted_lambda_prototypes:
            return
        self.emitted_lambda_prototypes.add(ctype.helper_name)
        self.emit(f"{self.lambda_signature_text(ctype)};")

    def emit_lambda_definition(self, spec: dict[str, Any]):
        ctype: LambdaCType = spec["ctype"]
        if ctype.helper_name in self.emitted_lambda_defs:
            return
        self.emitted_lambda_defs.add(ctype.helper_name)
        self.emit_lambda_prototype(ctype)
        self.emit()
        self.emit(f"{self.lambda_signature_text(ctype)} {{")
        self.indent += 1
        previous_var_types = self.var_types
        previous_module = self.current_module
        previous_return_type = self.current_return_type
        self.current_module = getattr(spec["node"], "module_name", self.current_module)
        self.current_return_type = ctype.return_type
        self.var_types = {name: t for name, t in zip(ctype.capture_names + ctype.param_names, ctype.capture_types + ctype.param_types)}
        expr_code, _ = self.expr_with_type(spec["node"].body, ctype.return_type)
        self.emit(f"return {expr_code};")
        self.var_types = previous_var_types
        self.current_module = previous_module
        self.current_return_type = previous_return_type
        self.indent -= 1
        self.emit("}")

    def map_value_type(self, map_type):
        if map_type == TYPE_MAP_INT:
            return TYPE_INT
        if map_type == TYPE_MAP_TEXT:
            return TYPE_TEXT
        if map_type == TYPE_MAP_BOOL:
            return TYPE_BOOL
        if self.is_map_type(map_type):
            inner = map_type[len(TYPE_MAP_PREFIX) :]
            if inner == "heltall":
                return TYPE_INT
            if inner == "tekst":
                return TYPE_TEXT
            if inner == "bool":
                return TYPE_BOOL
            if self.is_map_type(inner):
                return inner
        return None

    def map_any_tag(self, map_type):
        value_type = self.map_value_type(map_type)
        if value_type == TYPE_INT:
            return "NL_MAP_ANY_INT"
        if value_type == TYPE_TEXT:
            return "NL_MAP_ANY_TEXT"
        if value_type == TYPE_BOOL:
            return "NL_MAP_ANY_BOOL"
        if self.is_map_type(value_type):
            return "NL_MAP_ANY_MAP"
        return "NL_MAP_ANY_INT"

    def map_ctor_for_type(self, map_type):
        if map_type == TYPE_MAP_INT:
            return "nl_map_int_new()"
        if map_type == TYPE_MAP_TEXT:
            return "nl_map_text_new()"
        if map_type == TYPE_MAP_BOOL:
            return "nl_map_bool_new()"
        if self.is_map_type(map_type):
            return f"nl_map_any_new({self.map_any_tag(map_type)})"
        return "nl_map_any_new(NL_MAP_ANY_INT)"

    def map_set_fn_for_type(self, map_type):
        if map_type == TYPE_MAP_INT:
            return "nl_map_int_set"
        if map_type == TYPE_MAP_TEXT:
            return "nl_map_text_set"
        if map_type == TYPE_MAP_BOOL:
            return "nl_map_bool_set"
        value_type = self.map_value_type(map_type)
        if value_type == TYPE_INT:
            return "nl_map_any_set_int"
        if value_type == TYPE_TEXT:
            return "nl_map_any_set_text"
        if value_type == TYPE_BOOL:
            return "nl_map_any_set_bool"
        if self.is_map_type(value_type):
            return "nl_map_any_set_map"
        return None

    def map_get_fn_for_type(self, map_type, required=False):
        value_type = self.map_value_type(map_type)
        if value_type == TYPE_INT:
            if required:
                return "nl_map_int_get_required", "nl_map_any_get_required_int"
            return "nl_map_int_get", "nl_map_any_get_int"
        if value_type == TYPE_TEXT:
            if required:
                return "nl_map_text_get_required", "nl_map_any_get_required_text"
            return "nl_map_text_get", "nl_map_any_get_text"
        if value_type == TYPE_BOOL:
            if required:
                return "nl_map_bool_get_required", "nl_map_any_get_required_bool"
            return "nl_map_bool_get", "nl_map_any_get_bool"
        if self.is_map_type(value_type):
            if required:
                return None, "nl_map_any_get_required_map"
            return None, "nl_map_any_get_map"
        return None, None

    def c_string(self, value):
        escaped = (
            value.replace("\\", "\\\\")
            .replace('"', '\\"')
            .replace("\n", "\\n")
            .replace("\t", "\\t")
        )
        return f'"{escaped}"'

    def mangle_function_name(self, module_name, name):
        if not module_name or module_name == "__main__":
            return name
        return f"{module_name.replace('.', '__')}__{name}"

    def format_condition(self, expr_code):
        trimmed = expr_code.strip()
        if trimmed.startswith("(") and trimmed.endswith(")"):
            return trimmed
        return f"({trimmed})"

    def resolve_symbol(self, name, module_name=None):
        if module_name:
            real_module = self.alias_map.get(module_name, module_name)
            full = f"{real_module}.{name}"
            return self.function_symbols.get(full), full

        current_full = f"{self.current_module}.{name}" if self.current_module and self.current_module != "__main__" else name
        if current_full in self.function_symbols:
            return self.function_symbols[current_full], current_full
        if name in self.function_symbols:
            return self.function_symbols[name], name
        return None, name

    def resolve_c_function_name(self, name, module_name=None):
        _symbol, full = self.resolve_symbol(name, module_name=module_name)
        if full in self.function_name_map:
            return self.function_name_map[full]
        if name in self.function_name_map:
            return self.function_name_map[name]
        if module_name:
            real_module = self.alias_map.get(module_name, module_name)
            return self.mangle_function_name(real_module, name)
        return name

    def generate(self, tree):
        entry_function = None
        self.route_handlers, self.dependency_providers = self._collect_route_handlers(tree)
        self.openapi_paths_json = self._render_openapi_paths_json()
        for fn in tree.functions:
            if fn.name == "start" and (getattr(fn, "module_name", None) in (None, "__main__")):
                entry_function = fn
                break

        self.emit("#include <stdio.h>")
        self.emit("#include <stdlib.h>")
        self.emit("#include <string.h>")
        self.emit("#include <ctype.h>")
        self.emit("#include <stdint.h>")
        self.emit("#include <setjmp.h>")
        self.emit("#include <sys/time.h>")
        self.emit("#include <unistd.h>")
        self.emit()
        self.emit_runtime_helpers()
        self.emit("static nl_map_text *nl_web_request_dependency(nl_map_text *ctx, const char *name);")
        self.emit()

        for fn in tree.functions:
            self.visit_function(fn)
            self.emit()

        if self.lambda_order:
            for key in self.lambda_order:
                spec = self.lambda_specs.get(key)
                if spec is not None:
                    self.emit_lambda_definition(spec)
                    self.emit()

        self.emit_web_route_dispatcher()
        self.emit()

        self.emit("int main(void) {")
        self.indent += 1
        if entry_function is not None and entry_function.return_type == TYPE_TEXT:
            self.emit("char *result = start();")
            self.emit("if (result) {")
            self.indent += 1
            self.emit("puts(result);")
            self.indent -= 1
            self.emit("}")
            self.emit("nl_web_run_shutdown_hooks();")
            self.emit("return 0;")
        else:
            self.emit("int exit_code = start();")
            self.emit("nl_web_run_shutdown_hooks();")
            self.emit("return exit_code;")
        self.indent -= 1
        self.emit("}")

        return "\n".join(self.lines)

    def emit_runtime_helpers(self):
        self.emit("typedef struct { int *data; int len; int cap; } nl_list_int;")
        self.emit("typedef struct { char **data; int len; int cap; } nl_list_text;")
        self.emit("typedef struct { char **keys; int *values; int len; int cap; } nl_map_int;")
        self.emit("typedef struct { char **keys; char **values; int len; int cap; } nl_map_text;")
        self.emit("typedef struct { char **keys; int *values; int len; int cap; } nl_map_bool;")
        self.emit("typedef struct { char **keys; void **values; int len; int cap; int value_type; } nl_map_any;")
        self.emit("static const char *nl_call_stack[256];")
        self.emit("static int nl_call_stack_len = 0;")
        self.emit("#define NL_MAP_ANY_INT 1")
        self.emit("#define NL_MAP_ANY_TEXT 2")
        self.emit("#define NL_MAP_ANY_BOOL 3")
        self.emit("#define NL_MAP_ANY_MAP 4")
        self.emit()

        self.emit("static nl_list_int *nl_list_int_new(void) {")
        self.indent += 1
        self.emit("nl_list_int *l = (nl_list_int *)malloc(sizeof(nl_list_int));")
        self.emit("if (!l) { perror(\"malloc\"); exit(1); }")
        self.emit("l->len = 0;")
        self.emit("l->cap = 8;")
        self.emit("l->data = (int *)malloc(sizeof(int) * l->cap);")
        self.emit("if (!l->data) { perror(\"malloc\"); exit(1); }")
        self.emit("return l;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static nl_list_text *nl_list_text_new(void) {")
        self.indent += 1
        self.emit("nl_list_text *l = (nl_list_text *)malloc(sizeof(nl_list_text));")
        self.emit("if (!l) { perror(\"malloc\"); exit(1); }")
        self.emit("l->len = 0;")
        self.emit("l->cap = 8;")
        self.emit("l->data = (char **)malloc(sizeof(char *) * l->cap);")
        self.emit("if (!l->data) { perror(\"malloc\"); exit(1); }")
        self.emit("return l;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static nl_map_int *nl_map_int_new(void) {")
        self.indent += 1
        self.emit("nl_map_int *m = (nl_map_int *)malloc(sizeof(nl_map_int));")
        self.emit("if (!m) { perror(\"malloc\"); exit(1); }")
        self.emit("m->len = 0;")
        self.emit("m->cap = 8;")
        self.emit("m->keys = (char **)malloc(sizeof(char *) * m->cap);")
        self.emit("m->values = (int *)malloc(sizeof(int) * m->cap);")
        self.emit("if (!m->keys || !m->values) { perror(\"malloc\"); exit(1); }")
        self.emit("return m;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static nl_map_text *nl_map_text_new(void) {")
        self.indent += 1
        self.emit("nl_map_text *m = (nl_map_text *)malloc(sizeof(nl_map_text));")
        self.emit("if (!m) { perror(\"malloc\"); exit(1); }")
        self.emit("m->len = 0;")
        self.emit("m->cap = 8;")
        self.emit("m->keys = (char **)malloc(sizeof(char *) * m->cap);")
        self.emit("m->values = (char **)malloc(sizeof(char *) * m->cap);")
        self.emit("if (!m->keys || !m->values) { perror(\"malloc\"); exit(1); }")
        self.emit("return m;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static nl_map_bool *nl_map_bool_new(void) {")
        self.indent += 1
        self.emit("nl_map_bool *m = (nl_map_bool *)malloc(sizeof(nl_map_bool));")
        self.emit("if (!m) { perror(\"malloc\"); exit(1); }")
        self.emit("m->len = 0;")
        self.emit("m->cap = 8;")
        self.emit("m->keys = (char **)malloc(sizeof(char *) * m->cap);")
        self.emit("m->values = (int *)malloc(sizeof(int) * m->cap);")
        self.emit("if (!m->keys || !m->values) { perror(\"malloc\"); exit(1); }")
        self.emit("return m;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static nl_map_any *nl_map_any_new(int value_type) {")
        self.indent += 1
        self.emit("nl_map_any *m = (nl_map_any *)malloc(sizeof(nl_map_any));")
        self.emit("if (!m) { perror(\"malloc\"); exit(1); }")
        self.emit("m->len = 0;")
        self.emit("m->cap = 8;")
        self.emit("m->value_type = value_type;")
        self.emit("m->keys = (char **)malloc(sizeof(char *) * m->cap);")
        self.emit("m->values = (void **)malloc(sizeof(void *) * m->cap);")
        self.emit("if (!m->keys || !m->values) { perror(\"malloc\"); exit(1); }")
        self.emit("return m;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static void nl_list_int_ensure(nl_list_int *l, int need) {")
        self.indent += 1
        self.emit("if (need <= l->cap) { return; }")
        self.emit("while (l->cap < need) { l->cap *= 2; }")
        self.emit("l->data = (int *)realloc(l->data, sizeof(int) * l->cap);")
        self.emit("if (!l->data) { perror(\"realloc\"); exit(1); }")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static void nl_list_text_ensure(nl_list_text *l, int need) {")
        self.indent += 1
        self.emit("if (need <= l->cap) { return; }")
        self.emit("while (l->cap < need) { l->cap *= 2; }")
        self.emit("l->data = (char **)realloc(l->data, sizeof(char *) * l->cap);")
        self.emit("if (!l->data) { perror(\"realloc\"); exit(1); }")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static void nl_map_int_ensure(nl_map_int *m, int need) {")
        self.indent += 1
        self.emit("if (need <= m->cap) { return; }")
        self.emit("while (m->cap < need) { m->cap *= 2; }")
        self.emit("m->keys = (char **)realloc(m->keys, sizeof(char *) * m->cap);")
        self.emit("m->values = (int *)realloc(m->values, sizeof(int) * m->cap);")
        self.emit("if (!m->keys || !m->values) { perror(\"realloc\"); exit(1); }")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static void nl_map_text_ensure(nl_map_text *m, int need) {")
        self.indent += 1
        self.emit("if (need <= m->cap) { return; }")
        self.emit("while (m->cap < need) { m->cap *= 2; }")
        self.emit("m->keys = (char **)realloc(m->keys, sizeof(char *) * m->cap);")
        self.emit("m->values = (char **)realloc(m->values, sizeof(char *) * m->cap);")
        self.emit("if (!m->keys || !m->values) { perror(\"realloc\"); exit(1); }")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static void nl_map_bool_ensure(nl_map_bool *m, int need) {")
        self.indent += 1
        self.emit("if (need <= m->cap) { return; }")
        self.emit("while (m->cap < need) { m->cap *= 2; }")
        self.emit("m->keys = (char **)realloc(m->keys, sizeof(char *) * m->cap);")
        self.emit("m->values = (int *)realloc(m->values, sizeof(int) * m->cap);")
        self.emit("if (!m->keys || !m->values) { perror(\"realloc\"); exit(1); }")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static void nl_map_any_ensure(nl_map_any *m, int need) {")
        self.indent += 1
        self.emit("if (need <= m->cap) { return; }")
        self.emit("while (m->cap < need) { m->cap *= 2; }")
        self.emit("m->keys = (char **)realloc(m->keys, sizeof(char *) * m->cap);")
        self.emit("m->values = (void **)realloc(m->values, sizeof(void *) * m->cap);")
        self.emit("if (!m->keys || !m->values) { perror(\"realloc\"); exit(1); }")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_list_int_len(nl_list_int *l) { return l ? l->len : 0; }")
        self.emit("static int nl_list_text_len(nl_list_text *l) { return l ? l->len : 0; }")
        self.emit()

        self.emit("static int nl_list_int_push(nl_list_int *l, int v) {")
        self.indent += 1
        self.emit("nl_list_int_ensure(l, l->len + 1);")
        self.emit("l->data[l->len++] = v;")
        self.emit("return l->len;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_list_text_push(nl_list_text *l, char *v) {")
        self.indent += 1
        self.emit("nl_list_text_ensure(l, l->len + 1);")
        self.emit("l->data[l->len++] = v;")
        self.emit("return l->len;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_list_int_pop(nl_list_int *l) {")
        self.indent += 1
        self.emit("if (!l || l->len == 0) { return 0; }")
        self.emit("int v = l->data[l->len - 1];")
        self.emit("l->len -= 1;")
        self.emit("return v;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_list_text_pop(nl_list_text *l) {")
        self.indent += 1
        self.emit('if (!l || l->len == 0) { return ""; }')
        self.emit("char *v = l->data[l->len - 1];")
        self.emit("l->len -= 1;")
        self.emit("return v;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_list_int_remove(nl_list_int *l, int idx) {")
        self.indent += 1
        self.emit("if (!l || idx < 0 || idx >= l->len) { return nl_list_int_len(l); }")
        self.emit("for (int i = idx; i < l->len - 1; i++) { l->data[i] = l->data[i + 1]; }")
        self.emit("l->len -= 1;")
        self.emit("return l->len;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_list_text_remove(nl_list_text *l, int idx) {")
        self.indent += 1
        self.emit("if (!l || idx < 0 || idx >= l->len) { return nl_list_text_len(l); }")
        self.emit("for (int i = idx; i < l->len - 1; i++) { l->data[i] = l->data[i + 1]; }")
        self.emit("l->len -= 1;")
        self.emit("return l->len;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_list_int_set(nl_list_int *l, int idx, int v) {")
        self.indent += 1
        self.emit("if (!l || idx < 0 || idx >= l->len) { return nl_list_int_len(l); }")
        self.emit("l->data[idx] = v;")
        self.emit("return l->len;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_list_text_set(nl_list_text *l, int idx, char *v) {")
        self.indent += 1
        self.emit("if (!l || idx < 0 || idx >= l->len) { return nl_list_text_len(l); }")
        self.emit("l->data[idx] = v;")
        self.emit("return l->len;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_strdup(const char *s) {")
        self.indent += 1
        self.emit("if (!s) { s = \"\"; }")
        self.emit("size_t n = strlen(s) + 1;")
        self.emit("char *out = (char *)malloc(n);")
        self.emit("if (!out) { perror(\"malloc\"); exit(1); }")
        self.emit("memcpy(out, s, n);")
        self.emit("return out;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_streq(const char *a, const char *b) {")
        self.indent += 1
        self.emit("if (!a) { a = \"\"; }")
        self.emit("if (!b) { b = \"\"; }")
        self.emit("return strcmp(a, b) == 0;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_map_int_len(nl_map_int *m) { return m ? m->len : 0; }")
        self.emit("static int nl_map_text_len(nl_map_text *m) { return m ? m->len : 0; }")
        self.emit("static int nl_map_bool_len(nl_map_bool *m) { return m ? m->len : 0; }")
        self.emit("static int nl_map_any_len(nl_map_any *m) { return m ? m->len : 0; }")
        self.emit()

        self.emit("static int nl_map_int_find(nl_map_int *m, const char *key) {")
        self.indent += 1
        self.emit("if (!m) { return -1; }")
        self.emit("for (int i = 0; i < m->len; i++) {")
        self.indent += 1
        self.emit("if (nl_streq(m->keys[i], key)) { return i; }")
        self.indent -= 1
        self.emit("}")
        self.emit("return -1;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_map_text_find(nl_map_text *m, const char *key) {")
        self.indent += 1
        self.emit("if (!m) { return -1; }")
        self.emit("for (int i = 0; i < m->len; i++) {")
        self.indent += 1
        self.emit("if (nl_streq(m->keys[i], key)) { return i; }")
        self.indent -= 1
        self.emit("}")
        self.emit("return -1;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_map_bool_find(nl_map_bool *m, const char *key) {")
        self.indent += 1
        self.emit("if (!m) { return -1; }")
        self.emit("for (int i = 0; i < m->len; i++) {")
        self.indent += 1
        self.emit("if (nl_streq(m->keys[i], key)) { return i; }")
        self.indent -= 1
        self.emit("}")
        self.emit("return -1;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_map_any_find(nl_map_any *m, const char *key) {")
        self.indent += 1
        self.emit("if (!m) { return -1; }")
        self.emit("for (int i = 0; i < m->len; i++) {")
        self.indent += 1
        self.emit("if (nl_streq(m->keys[i], key)) { return i; }")
        self.indent -= 1
        self.emit("}")
        self.emit("return -1;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_map_int_set(nl_map_int *m, const char *key, int value) {")
        self.indent += 1
        self.emit("if (!m) { return 0; }")
        self.emit("int idx = nl_map_int_find(m, key);")
        self.emit("if (idx >= 0) { m->values[idx] = value; return m->len; }")
        self.emit("nl_map_int_ensure(m, m->len + 1);")
        self.emit("m->keys[m->len] = nl_strdup(key ? key : \"\");")
        self.emit("m->values[m->len] = value;")
        self.emit("m->len += 1;")
        self.emit("return m->len;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_map_text_set(nl_map_text *m, const char *key, char *value) {")
        self.indent += 1
        self.emit("if (!m) { return 0; }")
        self.emit("int idx = nl_map_text_find(m, key);")
        self.emit("if (idx >= 0) { m->values[idx] = value; return m->len; }")
        self.emit("nl_map_text_ensure(m, m->len + 1);")
        self.emit("m->keys[m->len] = nl_strdup(key ? key : \"\");")
        self.emit("m->values[m->len] = value;")
        self.emit("m->len += 1;")
        self.emit("return m->len;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_map_bool_set(nl_map_bool *m, const char *key, int value) {")
        self.indent += 1
        self.emit("if (!m) { return 0; }")
        self.emit("int idx = nl_map_bool_find(m, key);")
        self.emit("if (idx >= 0) { m->values[idx] = value; return m->len; }")
        self.emit("nl_map_bool_ensure(m, m->len + 1);")
        self.emit("m->keys[m->len] = nl_strdup(key ? key : \"\");")
        self.emit("m->values[m->len] = value;")
        self.emit("m->len += 1;")
        self.emit("return m->len;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_map_any_set_int(nl_map_any *m, const char *key, int value) {")
        self.indent += 1
        self.emit("if (!m) { return 0; }")
        self.emit("int idx = nl_map_any_find(m, key);")
        self.emit("if (idx >= 0) { m->values[idx] = (void *)(intptr_t)value; return m->len; }")
        self.emit("nl_map_any_ensure(m, m->len + 1);")
        self.emit("m->keys[m->len] = nl_strdup(key ? key : \"\");")
        self.emit("m->values[m->len] = (void *)(intptr_t)value;")
        self.emit("m->value_type = NL_MAP_ANY_INT;")
        self.emit("m->len += 1;")
        self.emit("return m->len;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_map_any_set_text(nl_map_any *m, const char *key, char *value) {")
        self.indent += 1
        self.emit("if (!m) { return 0; }")
        self.emit("int idx = nl_map_any_find(m, key);")
        self.emit("if (idx >= 0) { m->values[idx] = (void *)value; return m->len; }")
        self.emit("nl_map_any_ensure(m, m->len + 1);")
        self.emit("m->keys[m->len] = nl_strdup(key ? key : \"\");")
        self.emit("m->values[m->len] = (void *)value;")
        self.emit("m->value_type = NL_MAP_ANY_TEXT;")
        self.emit("m->len += 1;")
        self.emit("return m->len;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_map_any_set_bool(nl_map_any *m, const char *key, int value) {")
        self.indent += 1
        self.emit("if (!m) { return 0; }")
        self.emit("int idx = nl_map_any_find(m, key);")
        self.emit("if (idx >= 0) { m->values[idx] = (void *)(intptr_t)value; return m->len; }")
        self.emit("nl_map_any_ensure(m, m->len + 1);")
        self.emit("m->keys[m->len] = nl_strdup(key ? key : \"\");")
        self.emit("m->values[m->len] = (void *)(intptr_t)value;")
        self.emit("m->value_type = NL_MAP_ANY_BOOL;")
        self.emit("m->len += 1;")
        self.emit("return m->len;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_map_any_set_map(nl_map_any *m, const char *key, void *value) {")
        self.indent += 1
        self.emit("if (!m) { return 0; }")
        self.emit("int idx = nl_map_any_find(m, key);")
        self.emit("if (idx >= 0) { m->values[idx] = value; return m->len; }")
        self.emit("nl_map_any_ensure(m, m->len + 1);")
        self.emit("m->keys[m->len] = nl_strdup(key ? key : \"\");")
        self.emit("m->values[m->len] = value;")
        self.emit("m->value_type = NL_MAP_ANY_MAP;")
        self.emit("m->len += 1;")
        self.emit("return m->len;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_map_int_remove(nl_map_int *m, const char *key) {")
        self.indent += 1
        self.emit("if (!m) { return 0; }")
        self.emit("int idx = nl_map_int_find(m, key);")
        self.emit("if (idx < 0) { return m->len; }")
        self.emit("for (int i = idx; i < m->len - 1; i++) { m->keys[i] = m->keys[i + 1]; m->values[i] = m->values[i + 1]; }")
        self.emit("m->len -= 1;")
        self.emit("return m->len;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_map_text_remove(nl_map_text *m, const char *key) {")
        self.indent += 1
        self.emit("if (!m) { return 0; }")
        self.emit("int idx = nl_map_text_find(m, key);")
        self.emit("if (idx < 0) { return m->len; }")
        self.emit("for (int i = idx; i < m->len - 1; i++) { m->keys[i] = m->keys[i + 1]; m->values[i] = m->values[i + 1]; }")
        self.emit("m->len -= 1;")
        self.emit("return m->len;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_map_bool_remove(nl_map_bool *m, const char *key) {")
        self.indent += 1
        self.emit("if (!m) { return 0; }")
        self.emit("int idx = nl_map_bool_find(m, key);")
        self.emit("if (idx < 0) { return m->len; }")
        self.emit("for (int i = idx; i < m->len - 1; i++) { m->keys[i] = m->keys[i + 1]; m->values[i] = m->values[i + 1]; }")
        self.emit("m->len -= 1;")
        self.emit("return m->len;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_map_any_remove(nl_map_any *m, const char *key) {")
        self.indent += 1
        self.emit("if (!m) { return 0; }")
        self.emit("int idx = nl_map_any_find(m, key);")
        self.emit("if (idx < 0) { return m->len; }")
        self.emit("for (int i = idx; i < m->len - 1; i++) { m->keys[i] = m->keys[i + 1]; m->values[i] = m->values[i + 1]; }")
        self.emit("m->len -= 1;")
        self.emit("return m->len;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_map_int_get(nl_map_int *m, const char *key) {")
        self.indent += 1
        self.emit("int idx = nl_map_int_find(m, key);")
        self.emit("return idx >= 0 ? m->values[idx] : 0;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_map_int_get_required(nl_map_int *m, const char *key) {")
        self.indent += 1
        self.emit("int idx = nl_map_int_find(m, key);")
        self.emit("if (idx < 0) {")
        self.indent += 1
        self.emit('fprintf(stderr, \"Ukjent felt: %s\\n\", key ? key : \"\");')
        self.emit("exit(1);")
        self.indent -= 1
        self.emit("}")
        self.emit("return m->values[idx];")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_map_text_get(nl_map_text *m, const char *key) {")
        self.indent += 1
        self.emit("int idx = nl_map_text_find(m, key);")
        self.emit("return idx >= 0 ? m->values[idx] : \"\";")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_map_text_get_required(nl_map_text *m, const char *key) {")
        self.indent += 1
        self.emit("int idx = nl_map_text_find(m, key);")
        self.emit("if (idx < 0) {")
        self.indent += 1
        self.emit('fprintf(stderr, \"Ukjent felt: %s\\n\", key ? key : \"\");')
        self.emit("exit(1);")
        self.indent -= 1
        self.emit("}")
        self.emit("return m->values[idx];")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_map_bool_get(nl_map_bool *m, const char *key) {")
        self.indent += 1
        self.emit("int idx = nl_map_bool_find(m, key);")
        self.emit("return idx >= 0 ? m->values[idx] : 0;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_map_bool_get_required(nl_map_bool *m, const char *key) {")
        self.indent += 1
        self.emit("int idx = nl_map_bool_find(m, key);")
        self.emit("if (idx < 0) {")
        self.indent += 1
        self.emit('fprintf(stderr, \"Ukjent felt: %s\\n\", key ? key : \"\");')
        self.emit("exit(1);")
        self.indent -= 1
        self.emit("}")
        self.emit("return m->values[idx];")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_map_any_get_int(nl_map_any *m, const char *key) {")
        self.indent += 1
        self.emit("if (!m || m->value_type != NL_MAP_ANY_INT) { return 0; }")
        self.emit("int idx = nl_map_any_find(m, key);")
        self.emit("return idx >= 0 ? (int)(intptr_t)m->values[idx] : 0;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_map_any_get_required_int(nl_map_any *m, const char *key) {")
        self.indent += 1
        self.emit("if (!m || m->value_type != NL_MAP_ANY_INT) {")
        self.indent += 1
        self.emit('fprintf(stderr, \"Ukjent eller ugyldig felttype for %s\\n\", key ? key : \"\");')
        self.emit("exit(1);")
        self.indent -= 1
        self.emit("}")
        self.emit("int idx = nl_map_any_find(m, key);")
        self.emit("if (idx < 0) {")
        self.indent += 1
        self.emit('fprintf(stderr, \"Ukjent felt: %s\\n\", key ? key : \"\");')
        self.emit("exit(1);")
        self.indent -= 1
        self.emit("}")
        self.emit("return (int)(intptr_t)m->values[idx];")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_map_any_get_text(nl_map_any *m, const char *key) {")
        self.indent += 1
        self.emit("if (!m || m->value_type != NL_MAP_ANY_TEXT) { return \"\"; }")
        self.emit("int idx = nl_map_any_find(m, key);")
        self.emit("return idx >= 0 ? (char *)m->values[idx] : \"\";")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_map_any_get_required_text(nl_map_any *m, const char *key) {")
        self.indent += 1
        self.emit("if (!m || m->value_type != NL_MAP_ANY_TEXT) {")
        self.indent += 1
        self.emit('fprintf(stderr, \"Ukjent eller ugyldig felttype for %s\\n\", key ? key : \"\");')
        self.emit("exit(1);")
        self.indent -= 1
        self.emit("}")
        self.emit("int idx = nl_map_any_find(m, key);")
        self.emit("if (idx < 0) {")
        self.indent += 1
        self.emit('fprintf(stderr, \"Ukjent felt: %s\\n\", key ? key : \"\");')
        self.emit("exit(1);")
        self.indent -= 1
        self.emit("}")
        self.emit("return (char *)m->values[idx];")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_map_any_get_bool(nl_map_any *m, const char *key) {")
        self.indent += 1
        self.emit("if (!m || m->value_type != NL_MAP_ANY_BOOL) { return 0; }")
        self.emit("int idx = nl_map_any_find(m, key);")
        self.emit("return idx >= 0 ? (int)(intptr_t)m->values[idx] : 0;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_map_any_get_required_bool(nl_map_any *m, const char *key) {")
        self.indent += 1
        self.emit("if (!m || m->value_type != NL_MAP_ANY_BOOL) {")
        self.indent += 1
        self.emit('fprintf(stderr, \"Ukjent eller ugyldig felttype for %s\\n\", key ? key : \"\");')
        self.emit("exit(1);")
        self.indent -= 1
        self.emit("}")
        self.emit("int idx = nl_map_any_find(m, key);")
        self.emit("if (idx < 0) {")
        self.indent += 1
        self.emit('fprintf(stderr, \"Ukjent felt: %s\\n\", key ? key : \"\");')
        self.emit("exit(1);")
        self.indent -= 1
        self.emit("}")
        self.emit("return (int)(intptr_t)m->values[idx];")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static void *nl_map_any_get_map(nl_map_any *m, const char *key) {")
        self.indent += 1
        self.emit("if (!m || m->value_type != NL_MAP_ANY_MAP) { return NULL; }")
        self.emit("int idx = nl_map_any_find(m, key);")
        self.emit("return idx >= 0 ? m->values[idx] : NULL;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static void *nl_map_any_get_required_map(nl_map_any *m, const char *key) {")
        self.indent += 1
        self.emit("if (!m || m->value_type != NL_MAP_ANY_MAP) {")
        self.indent += 1
        self.emit('fprintf(stderr, \"Ukjent eller ugyldig felttype for %s\\n\", key ? key : \"\");')
        self.emit("exit(1);")
        self.indent -= 1
        self.emit("}")
        self.emit("int idx = nl_map_any_find(m, key);")
        self.emit("if (idx < 0) {")
        self.indent += 1
        self.emit('fprintf(stderr, \"Ukjent felt: %s\\n\", key ? key : \"\");')
        self.emit("exit(1);")
        self.indent -= 1
        self.emit("}")
        self.emit("return m->values[idx];")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_concat(const char *a, const char *b) {")
        self.indent += 1
        self.emit("if (!a) { a = \"\"; }")
        self.emit("if (!b) { b = \"\"; }")
        self.emit("size_t al = strlen(a);")
        self.emit("size_t bl = strlen(b);")
        self.emit("char *out = (char *)malloc(al + bl + 1);")
        self.emit("if (!out) { perror(\"malloc\"); exit(1); }")
        self.emit("memcpy(out, a, al);")
        self.emit("memcpy(out + al, b, bl);")
        self.emit("out[al + bl] = '\\0';")
        self.emit("return out;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_int_to_text(int value) {")
        self.indent += 1
        self.emit("char buffer[32];")
        self.emit("snprintf(buffer, sizeof(buffer), \"%d\", value);")
        self.emit("return nl_strdup(buffer);")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static nl_list_text *nl_split_words(const char *s) {")
        self.indent += 1
        self.emit("nl_list_text *out = nl_list_text_new();")
        self.emit("char *copy = nl_strdup(s ? s : \"\");")
        self.emit("char *tok = strtok(copy, \" \\t\\r\\n\");")
        self.emit("while (tok) {")
        self.indent += 1
        self.emit("nl_list_text_push(out, nl_strdup(tok));")
        self.emit("tok = strtok(NULL, \" \\t\\r\\n\");")
        self.indent -= 1
        self.emit("}")
        self.emit("return out;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static nl_list_text *nl_tokenize_simple(const char *s) {")
        self.indent += 1
        self.emit("nl_list_text *out = nl_list_text_new();")
        self.emit("if (!s) { return out; }")
        self.emit("char token[256];")
        self.emit("int tlen = 0;")
        self.emit("int in_comment = 0;")
        self.emit("for (const char *p = s; ; ++p) {")
        self.indent += 1
        self.emit("char c = *p;")
        self.emit("if (c == '\\0') {")
        self.indent += 1
        self.emit("if (tlen > 0) { token[tlen] = '\\0'; nl_list_text_push(out, nl_strdup(token)); }")
        self.emit("break;")
        self.indent -= 1
        self.emit("}")
        self.emit("if (in_comment) {")
        self.indent += 1
        self.emit("if (c == '\\n') { in_comment = 0; }")
        self.emit("continue;")
        self.indent -= 1
        self.emit("}")
        self.emit("if (c == '#') {")
        self.indent += 1
        self.emit("if (tlen > 0) { token[tlen] = '\\0'; nl_list_text_push(out, nl_strdup(token)); tlen = 0; }")
        self.emit("in_comment = 1;")
        self.emit("continue;")
        self.indent -= 1
        self.emit("}")
        self.emit("if (isalnum((unsigned char)c) || c == '_' || c == '-' || (unsigned char)c >= 128) {")
        self.indent += 1
        self.emit("if (tlen < 255) { token[tlen++] = c; }")
        self.emit("continue;")
        self.indent -= 1
        self.emit("}")
        self.emit("if (tlen > 0) {")
        self.indent += 1
        self.emit("token[tlen] = '\\0';")
        self.emit("nl_list_text_push(out, nl_strdup(token));")
        self.emit("tlen = 0;")
        self.indent -= 1
        self.emit("}")
        self.indent -= 1
        self.emit("}")
        self.emit("return out;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static nl_list_text *nl_tokenize_expression(const char *s) {")
        self.indent += 1
        self.emit("nl_list_text *out = nl_list_text_new();")
        self.emit("if (!s) { return out; }")
        self.emit("int in_comment = 0;")
        self.emit("for (const char *p = s; *p; ) {")
        self.indent += 1
        self.emit("char c = *p;")
        self.emit("if (in_comment) {")
        self.indent += 1
        self.emit("if (c == '\\n') { in_comment = 0; }")
        self.emit("p++;")
        self.emit("continue;")
        self.indent -= 1
        self.emit("}")
        self.emit("if (c == '#') { in_comment = 1; p++; continue; }")
        self.emit("if (isspace((unsigned char)c)) { p++; continue; }")
        self.emit("if (isdigit((unsigned char)c)) {")
        self.indent += 1
        self.emit("char token[256];")
        self.emit("int tlen = 0;")
        self.emit("while (*p && isdigit((unsigned char)*p)) {")
        self.indent += 1
        self.emit("if (tlen < 255) { token[tlen++] = *p; }")
        self.emit("p++;")
        self.indent -= 1
        self.emit("}")
        self.emit("token[tlen] = '\\0';")
        self.emit("nl_list_text_push(out, nl_strdup(token));")
        self.emit("continue;")
        self.indent -= 1
        self.emit("}")
        self.emit("if (isalpha((unsigned char)c) || c == '_' || (unsigned char)c >= 128) {")
        self.indent += 1
        self.emit("char token[256];")
        self.emit("int tlen = 0;")
        self.emit("while (*p && (isalnum((unsigned char)*p) || *p == '_' || (unsigned char)*p >= 128)) {")
        self.indent += 1
        self.emit("if (tlen < 255) { token[tlen++] = *p; }")
        self.emit("p++;")
        self.indent -= 1
        self.emit("}")
        self.emit("token[tlen] = '\\0';")
        self.emit("nl_list_text_push(out, nl_strdup(token));")
        self.emit("continue;")
        self.indent -= 1
        self.emit("}")
        self.emit("if ((c == '&' && p[1] == '&') || (c == '|' && p[1] == '|') || (c == '=' && p[1] == '=') ||")
        self.emit("    (c == '!' && p[1] == '=') || (c == '<' && p[1] == '=') || (c == '>' && p[1] == '=') ||")
        self.emit("    (c == '+' && p[1] == '=') || (c == '-' && p[1] == '=') || (c == '*' && p[1] == '=') ||")
        self.emit("    (c == '/' && p[1] == '=') || (c == '%' && p[1] == '=')) {")
        self.indent += 1
        self.emit("char token[3];")
        self.emit("token[0] = c;")
        self.emit("token[1] = p[1];")
        self.emit("token[2] = '\\0';")
        self.emit("nl_list_text_push(out, nl_strdup(token));")
        self.emit("p += 2;")
        self.emit("continue;")
        self.indent -= 1
        self.emit("}")
        self.emit("if (c == '(' || c == ')' || c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '<' || c == '>' || c == '=' || c == ';') {")
        self.indent += 1
        self.emit("char token[2];")
        self.emit("token[0] = c;")
        self.emit("token[1] = '\\0';")
        self.emit("nl_list_text_push(out, nl_strdup(token));")
        self.emit("p++;")
        self.emit("continue;")
        self.indent -= 1
        self.emit("}")
        self.emit("char unknown[2];")
        self.emit("unknown[0] = c;")
        self.emit("unknown[1] = '\\0';")
        self.emit("nl_list_text_push(out, nl_strdup(unknown));")
        self.emit("p++;")
        self.indent -= 1
        self.emit("}")
        self.emit("return out;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_bool_to_text(int value) {")
        self.indent += 1
        self.emit('return nl_strdup(value ? "sann" : "usann");')
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_text_to_int(const char *s) {")
        self.indent += 1
        self.emit("if (!s) { return 0; }")
        self.emit("return (int)strtol(s, NULL, 10);")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("typedef struct {")
        self.indent += 1
        self.emit("char *data;")
        self.emit("int len;")
        self.emit("int cap;")
        self.indent -= 1
        self.emit("} nl_json_builder;")
        self.emit()

        self.emit("static void nl_json_builder_init(nl_json_builder *b, int initial_cap) {")
        self.indent += 1
        self.emit("b->cap = initial_cap > 0 ? initial_cap : 16;")
        self.emit("b->len = 0;")
        self.emit("b->data = (char *)malloc((size_t)b->cap);")
        self.emit("if (!b->data) { perror(\"malloc\"); exit(1); }")
        self.emit("b->data[0] = '\\0';")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static void nl_json_builder_ensure(nl_json_builder *b, int need) {")
        self.indent += 1
        self.emit("if (b->len + need + 1 < b->cap) { return; }")
        self.emit("while (b->len + need + 1 >= b->cap) { b->cap *= 2; }")
        self.emit("b->data = (char *)realloc(b->data, (size_t)b->cap);")
        self.emit("if (!b->data) { perror(\"realloc\"); exit(1); }")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static void nl_json_builder_append_char(nl_json_builder *b, char ch) {")
        self.indent += 1
        self.emit("nl_json_builder_ensure(b, 1);")
        self.emit("b->data[b->len++] = ch;")
        self.emit("b->data[b->len] = '\\0';")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static void nl_json_builder_append_text(nl_json_builder *b, const char *text) {")
        self.indent += 1
        self.emit("if (!text) { return; }")
        self.emit("size_t n = strlen(text);")
        self.emit("nl_json_builder_ensure(b, (int)n);")
        self.emit("memcpy(b->data + b->len, text, n + 1);")
        self.emit("b->len += (int)n;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_json_strdup_range(const char *start, int n) {")
        self.indent += 1
        self.emit("char *out = (char *)malloc((size_t)n + 1);")
        self.emit("if (!out) { perror(\"malloc\"); exit(1); }")
        self.emit("memcpy(out, start, (size_t)n);")
        self.emit("out[n] = '\\0';")
        self.emit("return out;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static void nl_json_skip_ws(const char *s, int *idx, int len) {")
        self.indent += 1
        self.emit("while (*idx < len) {")
        self.indent += 1
        self.emit("if (s[*idx] == ' ' || s[*idx] == '\\n' || s[*idx] == '\\t' || s[*idx] == '\\r') {")
        self.indent += 1
        self.emit("(*idx)++;")
        self.indent -= 1
        self.emit("} else {")
        self.indent += 1
        self.emit("break;")
        self.indent -= 1
        self.emit("}")
        self.indent -= 1
        self.emit("}")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_json_is_digit(char c) {")
        self.indent += 1
        self.emit("return c >= '0' && c <= '9';")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_json_parse_string(const char *s, int *idx, int len) {")
        self.indent += 1
        self.emit("if (*idx >= len || s[*idx] != '\"') { return NULL; }")
        self.emit("(*idx)++;")
        self.emit("nl_json_builder out;")
        self.emit("nl_json_builder_init(&out, 32);")
        self.emit("while (*idx < len) {")
        self.indent += 1
        self.emit("char c = s[*idx];")
        self.emit("if (c == '\"') {")
        self.indent += 1
        self.emit("(*idx)++;")
        self.emit("return out.data;")
        self.indent -= 1
        self.emit("}")
        self.emit("if (c == '\\\\') {")
        self.indent += 1
        self.emit("(*idx)++;")
        self.emit("if (*idx >= len) { free(out.data); return NULL; }")
        self.emit("char e = s[*idx];")
        self.emit("if (e == '\"' || e == '\\\\' || e == '/') {")
        self.indent += 1
        self.emit("nl_json_builder_append_char(&out, e);")
        self.indent -= 1
        self.emit("} else if (e == 'b') { nl_json_builder_append_char(&out, '\\b'); }")
        self.emit("else if (e == 'f') { nl_json_builder_append_char(&out, '\\f'); }")
        self.emit("else if (e == 'n') { nl_json_builder_append_char(&out, '\\n'); }")
        self.emit("else if (e == 'r') { nl_json_builder_append_char(&out, '\\r'); }")
        self.emit("else if (e == 't') { nl_json_builder_append_char(&out, '\\t'); }")
        self.emit("else if (e == 'u') {")
        self.indent += 1
        self.emit("if (*idx + 4 >= len) { free(out.data); return NULL; }")
        self.emit("(*idx)++;")
        self.emit("char *end = NULL;")
        self.emit("char code_text[5];")
        self.emit("code_text[0] = s[*idx];")
        self.emit("code_text[1] = s[*idx + 1];")
        self.emit("code_text[2] = s[*idx + 2];")
        self.emit("code_text[3] = s[*idx + 3];")
        self.emit("code_text[4] = '\\0';")
        self.emit("long code = strtol(code_text, &end, 16);")
        self.emit("if (end && *end == '\\0') {")
        self.indent += 1
        self.emit("nl_json_builder_append_char(&out, (char)code);")
        self.indent -= 1
        self.emit("}")
        self.emit("else { free(out.data); return NULL; }")
        self.emit("*idx += 3;")
        self.indent -= 1
        self.emit("}")
        self.emit("else { free(out.data); return NULL; }")
        self.emit("(*idx)++;")
        self.indent -= 1
        self.emit("} else {")
        self.indent += 1
        self.emit("nl_json_builder_append_char(&out, c);")
        self.emit("(*idx)++;")
        self.indent -= 1
        self.emit("}")
        self.indent -= 1
        self.emit("}")
        self.emit("free(out.data);")
        self.emit("return NULL;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_json_quote(const char *value) {")
        self.indent += 1
        self.emit("if (!value) { return nl_strdup(\"\\\"\\\"\"); }")
        self.emit("nl_json_builder out;")
        self.emit("nl_json_builder_init(&out, 16);")
        self.emit("nl_json_builder_append_char(&out, '\"');")
        self.emit("for (const char *p = value; *p; ++p) {")
        self.indent += 1
        self.emit("if (*p == '\"' || *p == '\\\\') {")
        self.indent += 1
        self.emit("nl_json_builder_append_char(&out, '\\\\');")
        self.emit("nl_json_builder_append_char(&out, *p);")
        self.indent -= 1
        self.emit("} else if (*p == '\\n') {")
        self.indent += 1
        self.emit("nl_json_builder_append_text(&out, \"\\\\n\");")
        self.indent -= 1
        self.emit("} else if (*p == '\\r') {")
        self.indent += 1
        self.emit("nl_json_builder_append_text(&out, \"\\\\r\");")
        self.indent -= 1
        self.emit("} else if (*p == '\\t') {")
        self.indent += 1
        self.emit("nl_json_builder_append_text(&out, \"\\\\t\");")
        self.indent -= 1
        self.emit("} else {")
        self.indent += 1
        self.emit("nl_json_builder_append_char(&out, *p);")
        self.indent -= 1
        self.emit("}")
        self.indent -= 1
        self.emit("}")
        self.emit("nl_json_builder_append_char(&out, '\"');")
        self.emit("return out.data;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_json_is_keyword_true(const char *s, int *idx, int len) {")
        self.indent += 1
        self.emit("if (*idx + 3 >= len) { return 0; }")
        self.emit("if (strncmp(&s[*idx], \"true\", 4) != 0) { return 0; }")
        self.emit("(*idx) += 4;")
        self.emit("return 1;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_json_is_keyword_false(const char *s, int *idx, int len) {")
        self.indent += 1
        self.emit("if (*idx + 4 >= len) { return 0; }")
        self.emit("if (strncmp(&s[*idx], \"false\", 5) != 0) { return 0; }")
        self.emit("(*idx) += 5;")
        self.emit("return 1;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_json_is_keyword_null(const char *s, int *idx, int len) {")
        self.indent += 1
        self.emit("if (*idx + 3 >= len) { return 0; }")
        self.emit("if (strncmp(&s[*idx], \"null\", 4) != 0) { return 0; }")
        self.emit("(*idx) += 4;")
        self.emit("return 1;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_json_parse_value_raw(const char *s, int *idx, int len, char **raw);")
        self.emit("static void nl_json_report_error(const char *text, int idx, int len, const char *message) {")
        self.indent += 1
        self.emit("int line = 1;")
        self.emit("int column = 1;")
        self.emit("for (int i = 0; i < len && i < idx; i++) {")
        self.indent += 1
        self.emit("if (text[i] == '\\n') {")
        self.indent += 1
        self.emit("line++;")
        self.emit("column = 1;")
        self.indent -= 1
        self.emit("} else {")
        self.indent += 1
        self.emit("column++;")
        self.indent -= 1
        self.emit("}")
        self.indent -= 1
        self.emit("}")
        self.emit("int preview_len = (idx < len) ? 24 : 0;")
        self.emit("if (preview_len > 0 && idx + preview_len > len) {")
        self.indent += 1
        self.emit("preview_len = len - idx;")
        self.indent -= 1
        self.emit("}")
        self.emit("char preview[64] = {0};")
        self.emit("if (idx >= 0 && idx < len && preview_len > 0) {")
        self.indent += 1
        self.emit("int copy_len = preview_len > 60 ? 60 : preview_len;")
        self.emit("for (int i = 0; i < copy_len; i++) {")
        self.indent += 1
        self.emit("char c = text[idx + i];")
        self.emit("preview[i] = (c == '\\n' || c == '\\r' || c == '\\t') ? ' ' : c;")
        self.indent -= 1
        self.emit("}")
        self.emit("preview[copy_len] = '\\0';")
        self.indent -= 1
        self.emit("}")
        self.emit("fprintf(stderr, \"json_parse feilet: %s (linje %d, kolonne %d, posisjon %d)\", message, line, column, idx);")
        self.emit("if (idx >= 0 && idx < len) {")
        self.indent += 1
        self.emit("fprintf(stderr, \", tegn: '%c', kontekst: '%s'\", text[idx], preview);")
        self.indent -= 1
        self.emit("}")
        self.emit("fprintf(stderr, \"\\n\");")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_json_skip_object_raw(const char *s, int *idx, int len) {")
        self.indent += 1
        self.emit("if (*idx >= len || s[*idx] != '{') { return 0; }")
        self.emit("(*idx)++;")
        self.emit("nl_json_skip_ws(s, idx, len);")
        self.emit("if (*idx < len && s[*idx] == '}') { (*idx)++; return 1; }")
        self.emit("while (*idx < len) {")
        self.indent += 1
        self.emit("if (s[*idx] != '\"') { return 0; }")
        self.emit("char *key = nl_json_parse_string(s, idx, len);")
        self.emit("if (!key) { return 0; }")
        self.emit("free(key);")
        self.emit("nl_json_skip_ws(s, idx, len);")
        self.emit("if (*idx >= len || s[*idx] != ':') { return 0; }")
        self.emit("(*idx)++;")
        self.emit("nl_json_skip_ws(s, idx, len);")
        self.emit("if (!nl_json_parse_value_raw(s, idx, len, NULL)) { return 0; }")
        self.emit("nl_json_skip_ws(s, idx, len);")
        self.emit("if (*idx >= len) { return 0; }")
        self.emit("if (s[*idx] == ',') {")
        self.indent += 1
        self.emit("(*idx)++;")
        self.emit("nl_json_skip_ws(s, idx, len);")
        self.indent -= 1
        self.emit("} else if (s[*idx] == '}') {")
        self.indent += 1
        self.emit("(*idx)++;")
        self.emit("return 1;")
        self.indent -= 1
        self.emit("} else { return 0; }")
        self.emit("}")
        self.indent -= 1
        self.emit("return 0;")
        self.emit("}")
        self.emit()

        self.emit("static int nl_json_skip_array_raw(const char *s, int *idx, int len) {")
        self.indent += 1
        self.emit("if (*idx >= len || s[*idx] != '[') { return 0; }")
        self.emit("(*idx)++;")
        self.emit("nl_json_skip_ws(s, idx, len);")
        self.emit("if (*idx < len && s[*idx] == ']') { (*idx)++; return 1; }")
        self.emit("while (*idx < len) {")
        self.indent += 1
        self.emit("if (!nl_json_parse_value_raw(s, idx, len, NULL)) { return 0; }")
        self.emit("nl_json_skip_ws(s, idx, len);")
        self.emit("if (*idx >= len) { return 0; }")
        self.emit("if (s[*idx] == ',') {")
        self.indent += 1
        self.emit("(*idx)++;")
        self.emit("nl_json_skip_ws(s, idx, len);")
        self.indent -= 1
        self.emit("} else if (s[*idx] == ']') {")
        self.indent += 1
        self.emit("(*idx)++;")
        self.emit("return 1;")
        self.indent -= 1
        self.emit("} else { return 0; }")
        self.emit("}")
        self.indent -= 1
        self.emit("return 0;")
        self.emit("}")
        self.emit()

        self.emit("static int nl_json_parse_value_raw(const char *s, int *idx, int len, char **raw) {")
        self.indent += 1
        self.emit("nl_json_skip_ws(s, idx, len);")
        self.emit("if (*idx >= len) { return 0; }")
        self.emit("char c = s[*idx];")
        self.emit("int start = *idx;")
        self.emit("if (c == '\"') {")
        self.indent += 1
        self.emit("char *value = nl_json_parse_string(s, idx, len);")
        self.emit("if (!value) { return 0; }")
        self.emit("if (raw) { *raw = value; } else { free(value); }")
        self.emit("return 1;")
        self.indent -= 1
        self.emit("}")
        self.emit("if (c == '{' || c == '[') {")
        self.indent += 1
        self.emit("int i = *idx;")
        self.emit("int ok = (c == '{') ? nl_json_skip_object_raw(s, idx, len) : nl_json_skip_array_raw(s, idx, len);")
        self.emit("if (!ok) { return 0; }")
        self.emit("if (raw) { *raw = nl_json_strdup_range(s + i, *idx - i); }")
        self.emit("return 1;")
        self.indent -= 1
        self.emit("}")
        self.emit("if (c == '-' || nl_json_is_digit(c)) {")
        self.indent += 1
        self.emit("while (*idx < len && (nl_json_is_digit(s[*idx]) || s[*idx] == '-' || s[*idx] == '+' || s[*idx] == '.' || s[*idx] == 'e' || s[*idx] == 'E')) {")
        self.indent += 1
        self.emit("(*idx)++;")
        self.indent -= 1
        self.emit("}")
        self.emit("if (raw) { *raw = nl_json_strdup_range(s + start, *idx - start); }")
        self.emit("return 1;")
        self.indent -= 1
        self.emit("}")
        self.emit("if (c == 't' && nl_json_is_keyword_true(s, idx, len)) {")
        self.indent += 1
        self.emit("if (raw) { *raw = nl_strdup(\"true\"); }")
        self.emit("return 1;")
        self.indent -= 1
        self.emit("}")
        self.emit("if (c == 'f' && nl_json_is_keyword_false(s, idx, len)) {")
        self.indent += 1
        self.emit("if (raw) { *raw = nl_strdup(\"false\"); }")
        self.emit("return 1;")
        self.indent -= 1
        self.emit("}")
        self.emit("if (c == 'n' && nl_json_is_keyword_null(s, idx, len)) {")
        self.indent += 1
        self.emit("if (raw) { *raw = nl_strdup(\"\"); }")
        self.emit("return 1;")
        self.indent -= 1
        self.emit("}")
        self.emit("return 0;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_json_is_number_like(const char *value) {")
        self.indent += 1
        self.emit("if (!value || !*value) { return 0; }")
        self.emit("char *end = NULL;")
        self.emit("strtod(value, &end);")
        self.emit("return end && *end == '\\0';")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_json_serialize_value(const char *raw) {")
        self.indent += 1
        self.emit("if (!raw) { return nl_strdup(\"null\"); }")
        self.emit("if (raw[0] == '[' || raw[0] == '{' || nl_streq(raw, \"true\") || nl_streq(raw, \"false\") ||")
        self.emit("    nl_streq(raw, \"null\") || nl_json_is_number_like(raw)) {")
        self.indent += 1
        self.emit("return nl_strdup(raw);")
        self.indent -= 1
        self.emit("}")
        self.emit("return nl_json_quote(raw);")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static nl_map_text *json_parse(const char *text) {")
        self.indent += 1
        self.emit("if (!text) { return nl_map_text_new(); }")
        self.emit("int len = (int)strlen(text);")
        self.emit("int idx = 0;")
        self.emit("nl_json_skip_ws(text, &idx, len);")
        self.emit("if (idx >= len) { return nl_map_text_new(); }")
        self.emit("nl_map_text *result = nl_map_text_new();")
        self.emit("if (text[idx] == '{') {")
        self.indent += 1
        self.emit("idx++;")
        self.emit("nl_json_skip_ws(text, &idx, len);")
        self.emit("if (idx < len && text[idx] == '}') {")
        self.indent += 1
        self.emit("return result;")
        self.indent -= 1
        self.emit("}")
        self.emit("while (idx < len) {")
        self.indent += 1
        self.emit("if (text[idx] != '\"') {")
        self.indent += 1
        self.emit("nl_json_report_error(text, idx, len, \"ugyldig nøkkel\");")
        self.emit("exit(1);")
        self.indent -= 1
        self.emit("}")
        self.emit("char *raw_key = nl_json_parse_string(text, &idx, len);")
        self.emit("if (!raw_key) {")
        self.indent += 1
        self.emit("nl_json_report_error(text, idx, len, \"ugyldig nøkkel\");")
        self.emit("exit(1);")
        self.indent -= 1
        self.emit("}")
        self.emit("nl_json_skip_ws(text, &idx, len);")
        self.emit("if (idx >= len || text[idx] != ':') {")
        self.indent += 1
        self.emit("free(raw_key);")
        self.emit("nl_json_report_error(text, idx, len, \"mangler ':' etter nøkkel\");")
        self.emit("exit(1);")
        self.indent -= 1
        self.emit("}")
        self.emit("idx++;")
        self.emit("nl_json_skip_ws(text, &idx, len);")
        self.emit("char *raw_value = NULL;")
        self.emit("if (!nl_json_parse_value_raw(text, &idx, len, &raw_value)) {")
        self.indent += 1
        self.emit("free(raw_key);")
        self.emit("nl_json_report_error(text, idx, len, \"ugyldig verdi\");")
        self.emit("exit(1);")
        self.indent -= 1
        self.emit("}")
        self.emit("nl_map_text_set(result, raw_key, raw_value ? raw_value : nl_strdup(\"\"));")
        self.emit("free(raw_key);")
        self.emit("nl_json_skip_ws(text, &idx, len);")
        self.emit("if (idx >= len) {")
        self.indent += 1
        self.emit("nl_json_report_error(text, idx, len, \"mangler avsluttende '}'\");")
        self.emit("exit(1);")
        self.indent -= 1
        self.emit("}")
        self.emit("if (text[idx] == ',') {")
        self.indent += 1
        self.emit("idx++;")
        self.emit("nl_json_skip_ws(text, &idx, len);")
        self.indent -= 1
        self.emit("continue;")
        self.emit("}")
        self.emit("if (text[idx] == '}') {")
        self.indent += 1
        self.emit("idx++;")
        self.emit("break;")
        self.indent -= 1
        self.emit("}")
        self.emit("nl_json_report_error(text, idx, len, \"ugyldig objekt-separator\");")
        self.emit("exit(1);")
        self.indent -= 1
        self.emit("}")
        self.emit("return result;")
        self.indent -= 1
        self.emit("}")
        self.emit("if (text[idx] == '[') {")
        self.indent += 1
        self.emit("idx++;")
        self.emit("nl_json_skip_ws(text, &idx, len);")
        self.emit("if (idx < len && text[idx] == ']') {")
        self.indent += 1
        self.emit("return result;")
        self.indent -= 1
        self.emit("}")
        self.emit("int index = 0;")
        self.emit("while (idx < len) {")
        self.indent += 1
        self.emit("char *raw_value = NULL;")
        self.emit("if (!nl_json_parse_value_raw(text, &idx, len, &raw_value)) {")
        self.indent += 1
        self.emit("nl_json_report_error(text, idx, len, \"ugyldig listeverdi\");")
        self.emit("exit(1);")
        self.indent -= 1
        self.emit("}")
        self.emit("char *raw_key = nl_int_to_text(index);")
        self.emit("nl_map_text_set(result, raw_key, raw_value ? raw_value : nl_strdup(\"\"));")
        self.emit("index += 1;")
        self.emit("nl_json_skip_ws(text, &idx, len);")
        self.emit("if (idx >= len) {")
        self.indent += 1
        self.emit("nl_json_report_error(text, idx, len, \"mangler avsluttende ']'\");")
        self.emit("exit(1);")
        self.indent -= 1
        self.emit("}")
        self.emit("if (text[idx] == ',') {")
        self.indent += 1
        self.emit("idx++;")
        self.emit("nl_json_skip_ws(text, &idx, len);")
        self.indent -= 1
        self.emit("continue;")
        self.emit("}")
        self.emit("if (text[idx] == ']') {")
        self.indent += 1
        self.emit("idx++;")
        self.emit("return result;")
        self.indent -= 1
        self.emit("}")
        self.emit("nl_json_report_error(text, idx, len, \"ugyldig liste-separator\");")
        self.emit("exit(1);")
        self.indent -= 1
        self.emit("}")
        self.emit("nl_json_report_error(text, idx, len, \"ugyldig liste\");")
        self.emit("exit(1);")
        self.emit("return result;")
        self.indent -= 1
        self.emit("}")
        self.emit("nl_json_report_error(text, idx, len, \"forventer objekt eller liste\");")
        self.emit("exit(1);")
        self.emit("return result;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *json_stringify(nl_map_text *value) {")
        self.indent += 1
        self.emit("nl_json_builder out;")
        self.emit("nl_json_builder_init(&out, 32);")
        self.emit("nl_json_builder_append_char(&out, '{');")
        self.emit("for (int i = 0; i < value->len; i++) {")
        self.indent += 1
        self.emit("if (i > 0) {")
        self.indent += 1
        self.emit("nl_json_builder_append_char(&out, ',');")
        self.indent -= 1
        self.emit("}")
        self.emit("char *key = nl_json_quote(value->keys[i]);")
        self.emit("nl_json_builder_append_text(&out, key);")
        self.emit("free(key);")
        self.emit("nl_json_builder_append_char(&out, ':');")
        self.emit("char *serialized = nl_json_serialize_value(value->values[i]);")
        self.emit("nl_json_builder_append_text(&out, serialized);")
        self.emit("free(serialized);")
        self.indent -= 1
        self.emit("}")
        self.emit("nl_json_builder_append_char(&out, '}');")
        self.emit("return out.data;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_assert(int cond) {")
        self.indent += 1
        self.emit("if (!cond) {")
        self.indent += 1
        self.emit('printf("assert failed\\n");')
        self.emit("exit(1);")
        self.indent -= 1
        self.emit("}")
        self.emit("return 0;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_assert_eq_int(int a, int b) {")
        self.indent += 1
        self.emit("if (a != b) {")
        self.indent += 1
        self.emit('printf("assert_eq failed: %d != %d\\n", a, b);')
        self.emit("exit(1);")
        self.indent -= 1
        self.emit("}")
        self.emit("return 0;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_assert_ne_int(int a, int b) {")
        self.indent += 1
        self.emit("if (a == b) {")
        self.indent += 1
        self.emit('printf("assert_ne failed: %d == %d\\n", a, b);')
        self.emit("exit(1);")
        self.indent -= 1
        self.emit("}")
        self.emit("return 0;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_assert_eq_text(const char *a, const char *b) {")
        self.indent += 1
        self.emit("if (!nl_streq(a, b)) {")
        self.indent += 1
        self.emit('printf("assert_eq failed: \\"%s\\" != \\"%s\\"\\n", a ? a : "", b ? b : "");')
        self.emit("exit(1);")
        self.indent -= 1
        self.emit("}")
        self.emit("return 0;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_assert_ne_text(const char *a, const char *b) {")
        self.indent += 1
        self.emit("if (nl_streq(a, b)) {")
        self.indent += 1
        self.emit('printf("assert_ne failed: \\"%s\\" == \\"%s\\"\\n", a ? a : "", b ? b : "");')
        self.emit("exit(1);")
        self.indent -= 1
        self.emit("}")
        self.emit("return 0;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_assert_starter_med(const char *text, const char *prefix) {")
        self.indent += 1
        self.emit("if (!text) { text = \"\"; }")
        self.emit("if (!prefix) { prefix = \"\"; }")
        self.emit("if (strncmp(text, prefix, strlen(prefix)) != 0) {")
        self.indent += 1
        self.emit('printf("assert_starter_med failed: \\"%s\\" does not start with \\"%s\\"\\n", text, prefix);')
        self.emit("exit(1);")
        self.indent -= 1
        self.emit("}")
        self.emit("return 0;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_assert_slutter_med(const char *text, const char *suffix) {")
        self.indent += 1
        self.emit("if (!text) { text = \"\"; }")
        self.emit("if (!suffix) { suffix = \"\"; }")
        self.emit("size_t text_len = strlen(text);")
        self.emit("size_t suffix_len = strlen(suffix);")
        self.emit("if (text_len < suffix_len || strcmp(text + (text_len - suffix_len), suffix) != 0) {")
        self.indent += 1
        self.emit('printf("assert_slutter_med failed: \\"%s\\" does not end with \\"%s\\"\\n", text, suffix);')
        self.emit("exit(1);")
        self.indent -= 1
        self.emit("}")
        self.emit("return 0;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_assert_inneholder(const char *text, const char *needle) {")
        self.indent += 1
        self.emit("if (!text) { text = \"\"; }")
        self.emit("if (!needle) { needle = \"\"; }")
        self.emit("if (!strstr(text, needle)) {")
        self.indent += 1
        self.emit('printf("assert_inneholder failed: \\"%s\\" does not contain \\"%s\\"\\n", text, needle);')
        self.emit("exit(1);")
        self.indent -= 1
        self.emit("}")
        self.emit("return 0;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static void nl_print_text(const char *s) {")
        self.indent += 1
        self.emit('printf("%s\\n", s ? s : "");')
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("typedef struct nl_try_frame nl_try_frame;")
        self.emit("struct nl_try_frame {")
        self.indent += 1
        self.emit("nl_try_frame *previous;")
        self.emit("jmp_buf jump_point;")
        self.emit("char *error_message;")
        self.emit("int call_depth;")
        self.indent -= 1
        self.emit("};")
        self.emit()

        self.emit("static nl_try_frame *nl_try_stack = NULL;")
        self.emit()

        self.emit("static void nl_push_try_frame(nl_try_frame *frame) {")
        self.indent += 1
        self.emit("frame->previous = nl_try_stack;")
        self.emit("frame->error_message = NULL;")
        self.emit("frame->call_depth = nl_call_stack_len;")
        self.emit("nl_try_stack = frame;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static void nl_pop_try_frame(void) {")
        self.indent += 1
        self.emit("if (nl_try_stack) {")
        self.indent += 1
        self.emit("nl_try_stack = nl_try_stack->previous;")
        self.indent -= 1
        self.emit("}")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static void nl_throw(const char *message) {")
        self.indent += 1
        self.emit("if (!nl_try_stack) {")
        self.indent += 1
        self.emit('fprintf(stderr, "Ubehandlet feil: %s\\n", message ? message : "");')
        self.emit("if (nl_call_stack_len > 0) {")
        self.indent += 1
        self.emit('fprintf(stderr, "Kallstakk: ");')
        self.emit("for (int i = nl_call_stack_len - 1; i >= 0; --i) {")
        self.indent += 1
        self.emit('fprintf(stderr, "%s%s", nl_call_stack[i], i > 0 ? " -> " : "");')
        self.indent -= 1
        self.emit("}")
        self.emit('fprintf(stderr, "\\n");')
        self.indent -= 1
        self.emit("}")
        self.emit("exit(1);")
        self.indent -= 1
        self.emit("}")
        self.emit("nl_try_stack->error_message = nl_strdup(message ? message : \"\");")
        self.emit("longjmp(nl_try_stack->jump_point, 1);")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("typedef struct {")
        self.indent += 1
        self.emit("int active;")
        self.emit("int value;")
        self.emit("int cancelled;")
        self.emit("int has_deadline;")
        self.emit("long long deadline_ms;")
        self.indent -= 1
        self.emit("} nl_async_handle;")
        self.emit("static nl_async_handle nl_async_handles[256];")
        self.emit("static int nl_async_next_handle = 1;")
        self.emit()

        self.emit("static long long nl_now_ms(void) {")
        self.indent += 1
        self.emit("struct timeval tv;")
        self.emit("gettimeofday(&tv, NULL);")
        self.emit("return (long long)tv.tv_sec * 1000LL + (long long)tv.tv_usec / 1000LL;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_async_make_handle(int value, int cancelled, int has_deadline, long long deadline_ms) {")
        self.indent += 1
        self.emit("int id = nl_async_next_handle++;")
        self.emit("if (id >= 256) {")
        self.indent += 1
        self.emit('fprintf(stderr, "for mange async-handles\\n");')
        self.emit("exit(1);")
        self.indent -= 1
        self.emit("}")
        self.emit("nl_async_handles[id].active = 1;")
        self.emit("nl_async_handles[id].value = value;")
        self.emit("nl_async_handles[id].cancelled = cancelled;")
        self.emit("nl_async_handles[id].has_deadline = has_deadline;")
        self.emit("nl_async_handles[id].deadline_ms = deadline_ms;")
        self.emit("return -id;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_async_is_handle(int value) {")
        self.indent += 1
        self.emit("int id = -value;")
        self.emit("return value < 0 && id > 0 && id < 256 && nl_async_handles[id].active;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_async_timeout(int value, int timeout_ms) {")
        self.indent += 1
        self.emit("long long deadline = nl_now_ms() + (timeout_ms < 0 ? 0 : timeout_ms);")
        self.emit("return nl_async_make_handle(value, 0, 1, deadline);")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_async_cancel(int value) {")
        self.indent += 1
        self.emit("if (nl_async_is_handle(value)) {")
        self.indent += 1
        self.emit("int id = -value;")
        self.emit("nl_async_handles[id].cancelled = 1;")
        self.emit("return value;")
        self.indent -= 1
        self.emit("}")
        self.emit("return nl_async_make_handle(value, 1, 0, 0);")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_async_is_cancelled(int value) {")
        self.indent += 1
        self.emit("if (!nl_async_is_handle(value)) { return 0; }")
        self.emit("return nl_async_handles[-value].cancelled;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_async_is_timed_out(int value) {")
        self.indent += 1
        self.emit("if (!nl_async_is_handle(value)) { return 0; }")
        self.emit("nl_async_handle *h = &nl_async_handles[-value];")
        self.emit("return h->has_deadline && nl_now_ms() > h->deadline_ms;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static void nl_sleep_ms(int ms) {")
        self.indent += 1
        self.emit("if (ms < 0) { ms = 0; }")
        self.emit("usleep((useconds_t)ms * 1000);")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_async_await(int value) {")
        self.indent += 1
        self.emit("if (!nl_async_is_handle(value)) { return value; }")
        self.emit("nl_async_handle *h = &nl_async_handles[-value];")
        self.emit("if (h->cancelled) { nl_throw(\"AvbruttFeil: future ble kansellert\"); return h->value; }")
        self.emit("if (h->has_deadline && nl_now_ms() > h->deadline_ms) { nl_throw(\"TimeoutFeil: future gikk ut på tid\"); return h->value; }")
        self.emit("return h->value;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_io_error(const char *handling, const char *path) {")
        self.indent += 1
        self.emit("char buffer[1024];")
        self.emit('snprintf(buffer, sizeof(buffer), "IOFeil: kunne ikke %s fil %s", handling ? handling : "", path ? path : "");')
        self.emit("return nl_strdup(buffer);")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_slice_start(int len, int value, int has_value) {")
        self.indent += 1
        self.emit("if (!has_value) { return 0; }")
        self.emit("if (value < 0) { value += len; }")
        self.emit("if (value < 0) { value = 0; }")
        self.emit("if (value > len) { value = len; }")
        self.emit("return value;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_slice_end(int len, int value, int has_value) {")
        self.indent += 1
        self.emit("if (!has_value) { return len; }")
        self.emit("if (value < 0) { value += len; }")
        self.emit("if (value < 0) { value = 0; }")
        self.emit("if (value > len) { value = len; }")
        self.emit("return value;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_path_join(const char *left, const char *right) {")
        self.indent += 1
        self.emit("if (!left || !*left) { return nl_strdup(right ? right : \"\"); }")
        self.emit("if (!right || !*right) { return nl_strdup(left); }")
        self.emit("size_t left_len = strlen(left);")
        self.emit("while (left_len > 0 && (left[left_len - 1] == '/' || left[left_len - 1] == '\\\\')) { left_len--; }")
        self.emit("const char *right_part = right;")
        self.emit("while (*right_part == '/' || *right_part == '\\\\') { right_part++; }")
        self.emit("size_t right_len = strlen(right_part);")
        self.emit("char *out = malloc(left_len + 1 + right_len + 1);")
        self.emit("if (!out) { perror(\"malloc\"); exit(1); }")
        self.emit("memcpy(out, left, left_len);")
        self.emit("out[left_len] = '/';")
        self.emit("memcpy(out + left_len + 1, right_part, right_len);")
        self.emit("out[left_len + 1 + right_len] = '\\0';")
        self.emit("return out;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_path_basename(const char *path) {")
        self.indent += 1
        self.emit("if (!path || !*path) { return nl_strdup(\"\"); }")
        self.emit("const char *last = path;")
        self.emit("for (const char *p = path; *p; ++p) { if (*p == '/' || *p == '\\\\') { last = p + 1; } }")
        self.emit("return nl_strdup(last);")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_path_dirname(const char *path) {")
        self.indent += 1
        self.emit("if (!path || !*path) { return nl_strdup(\"\"); }")
        self.emit("const char *last = NULL;")
        self.emit("for (const char *p = path; *p; ++p) { if (*p == '/' || *p == '\\\\') { last = p; } }")
        self.emit("if (!last) { return nl_strdup(\"\"); }")
        self.emit("size_t len = (size_t)(last - path);")
        self.emit("char *out = malloc(len + 1);")
        self.emit("if (!out) { perror(\"malloc\"); exit(1); }")
        self.emit("memcpy(out, path, len);")
        self.emit("out[len] = '\\0';")
        self.emit("return out;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_path_stem(const char *path) {")
        self.indent += 1
        self.emit("char *base = nl_path_basename(path);")
        self.emit("char *dot = strrchr(base, '.');")
        self.emit("if (dot) { *dot = '\\0'; }")
        self.emit("return base;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_path_exists(const char *path) {")
        self.indent += 1
        self.emit("if (!path || !*path) { return 0; }")
        self.emit("FILE *f = fopen(path, \"rb\");")
        self.emit("if (!f) { return 0; }")
        self.emit("fclose(f);")
        self.emit("return 1;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_env_get(const char *key) {")
        self.indent += 1
        self.emit("const char *value = key ? getenv(key) : NULL;")
        self.emit("return nl_strdup(value ? value : \"\");")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_env_exists(const char *key) {")
        self.indent += 1
        self.emit("return key ? getenv(key) != NULL : 0;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_env_set(const char *key, const char *value) {")
        self.indent += 1
        self.emit("if (!key) { return nl_strdup(value ? value : \"\"); }")
        self.emit("setenv(key, value ? value : \"\", 1);")
        self.emit("return nl_strdup(value ? value : \"\");")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static nl_map_text *nl_http_response_parse(const char *response) {")
        self.indent += 1
        self.emit("if (!response) { return nl_map_text_new(); }")
        self.emit("return json_parse(response);")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_http_response_status(const char *response) {")
        self.indent += 1
        self.emit("nl_map_text *parsed = nl_http_response_parse(response);")
        self.emit("return atoi(nl_map_text_get(parsed, \"status\"));")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_http_response_text(const char *response) {")
        self.indent += 1
        self.emit("nl_map_text *parsed = nl_http_response_parse(response);")
        self.emit("return nl_strdup(nl_map_text_get(parsed, \"body\"));")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static nl_map_text *nl_http_response_json(const char *response) {")
        self.indent += 1
        self.emit("nl_map_text *parsed = nl_http_response_parse(response);")
        self.emit("char *body = nl_map_text_get(parsed, \"body\");")
        self.emit("return json_parse(body);")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_http_response_header(const char *response, const char *key) {")
        self.indent += 1
        self.emit("nl_map_text *parsed = nl_http_response_parse(response);")
        self.emit("char *headers_raw = nl_map_text_get(parsed, \"headers\");")
        self.emit("nl_map_text *headers = json_parse(headers_raw);")
        self.emit("return nl_strdup(nl_map_text_get(headers, key));")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_web_request_counter = 0;")
        self.emit("static char *nl_web_next_request_id(void) {")
        self.indent += 1
        self.emit("char buffer[32];")
        self.emit("snprintf(buffer, sizeof(buffer), \"req-%d\", ++nl_web_request_counter);")
        self.emit("return nl_strdup(buffer);")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static nl_map_text *nl_web_request_context(const char *method, const char *path, nl_map_text *query, nl_map_text *headers, const char *body) {")
        self.indent += 1
        self.emit("nl_map_text *result = nl_map_text_new();")
        self.emit("char *method_text = nl_strdup(method ? method : \"\");")
        self.emit("for (char *p = method_text; p && *p; ++p) { *p = (char)toupper((unsigned char)*p); }")
        self.emit("nl_map_text_set(result, \"metode\", method_text);")
        self.emit("nl_map_text_set(result, \"sti\", nl_strdup(path ? path : \"\"));")
        self.emit("nl_map_text_set(result, \"query\", json_stringify(query ? query : nl_map_text_new()));")
        self.emit("nl_map_text_set(result, \"headers\", json_stringify(headers ? headers : nl_map_text_new()));")
        self.emit("nl_map_text_set(result, \"body\", nl_strdup(body ? body : \"\"));")
        self.emit("nl_map_text_set(result, \"params\", json_stringify(nl_map_text_new()));")
        self.emit("nl_map_text_set(result, \"request_id\", nl_web_next_request_id());")
        self.emit("return result;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_web_request_id(nl_map_text *ctx) {")
        self.indent += 1
        self.emit("return nl_strdup(nl_map_text_get(ctx, \"request_id\"));")
        self.indent -= 1
        self.emit("}")
        self.emit()
        self.emit("static int nl_web_run_startup_hooks(void);")
        self.emit("static int nl_web_run_shutdown_hooks(void);")
        self.emit()

        self.emit("static char *nl_web_request_method(nl_map_text *ctx) {")
        self.indent += 1
        self.emit("return nl_strdup(nl_map_text_get(ctx, \"metode\"));")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_web_request_path(nl_map_text *ctx) {")
        self.indent += 1
        self.emit("return nl_strdup(nl_map_text_get(ctx, \"sti\"));")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static nl_map_text *nl_web_request_query(nl_map_text *ctx) {")
        self.indent += 1
        self.emit("return json_parse(nl_map_text_get(ctx, \"query\"));")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static nl_map_text *nl_web_request_headers(nl_map_text *ctx) {")
        self.indent += 1
        self.emit("return json_parse(nl_map_text_get(ctx, \"headers\"));")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_web_request_body(nl_map_text *ctx) {")
        self.indent += 1
        self.emit("return nl_strdup(nl_map_text_get(ctx, \"body\"));")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static nl_map_text *nl_web_request_params(nl_map_text *ctx) {")
        self.indent += 1
        self.emit("return json_parse(nl_map_text_get(ctx, \"params\"));")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_web_request_param(nl_map_text *ctx, const char *key) {")
        self.indent += 1
        self.emit("nl_map_text *params = nl_web_request_params(ctx);")
        self.emit("return nl_strdup(nl_map_text_get(params, key));")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_web_validate_int_text(const char *value, const char *field_name, const char *source_name);")
        self.emit("static int nl_web_validate_bool_text(const char *value, const char *field_name, const char *source_name);")
        self.emit()

        self.emit("static int nl_web_request_param_int(nl_map_text *ctx, const char *key) {")
        self.indent += 1
        self.emit("nl_map_text *params = nl_web_request_params(ctx);")
        self.emit("return nl_web_validate_int_text(nl_map_text_get(params, key), key, \"path\");")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_web_validate_int_text(const char *value, const char *field_name, const char *source_name) {")
        self.indent += 1
        self.emit("if (!value || !*value) {")
        self.indent += 1
        self.emit('char buffer[256];')
        self.emit('snprintf(buffer, sizeof(buffer), "ValideringsFeil: mangler %s-felt \x27%s\x27", source_name ? source_name : "", field_name ? field_name : "");')
        self.emit("nl_throw(buffer);")
        self.emit("return 0;")
        self.indent -= 1
        self.emit("}")
        self.emit("char *end = NULL;")
        self.emit("long parsed = strtol(value, &end, 10);")
        self.emit("if (!end || *end != '\\0') {")
        self.indent += 1
        self.emit('char buffer[256];')
        self.emit('snprintf(buffer, sizeof(buffer), "ValideringsFeil: felt \x27%s\x27 forventet heltall, fikk \x27%s\x27", field_name ? field_name : "", value ? value : "");')
        self.emit("nl_throw(buffer);")
        self.emit("return 0;")
        self.indent -= 1
        self.emit("}")
        self.emit("return (int)parsed;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_web_validate_bool_text(const char *value, const char *field_name, const char *source_name) {")
        self.indent += 1
        self.emit("if (!value || !*value) {")
        self.indent += 1
        self.emit('char buffer[256];')
        self.emit('snprintf(buffer, sizeof(buffer), "ValideringsFeil: mangler %s-felt \x27%s\x27", source_name ? source_name : "", field_name ? field_name : "");')
        self.emit("nl_throw(buffer);")
        self.emit("return 0;")
        self.indent -= 1
        self.emit("}")
        self.emit("char lower[32];")
        self.emit("size_t n = strlen(value);")
        self.emit("if (n >= sizeof(lower)) { n = sizeof(lower) - 1; }")
        self.emit("for (size_t i = 0; i < n; ++i) { lower[i] = (char)tolower((unsigned char)value[i]); }")
        self.emit("lower[n] = '\\0';")
        self.emit("if (strcmp(lower, \"true\") == 0 || strcmp(lower, \"1\") == 0 || strcmp(lower, \"ja\") == 0) { return 1; }")
        self.emit("if (strcmp(lower, \"false\") == 0 || strcmp(lower, \"0\") == 0 || strcmp(lower, \"nei\") == 0) { return 0; }")
        self.emit('char buffer[256];')
        self.emit('snprintf(buffer, sizeof(buffer), "ValideringsFeil: felt \x27%s\x27 forventet bool, fikk \x27%s\x27", field_name ? field_name : "", value ? value : "");')
        self.emit("nl_throw(buffer);")
        self.emit("return 0;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_web_request_query_param(nl_map_text *ctx, const char *key) {")
        self.indent += 1
        self.emit("nl_map_text *query = nl_web_request_query(ctx);")
        self.emit("return nl_strdup(nl_map_text_get(query, key));")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_web_request_query_required(nl_map_text *ctx, const char *key) {")
        self.indent += 1
        self.emit("nl_map_text *query = nl_web_request_query(ctx);")
        self.emit("char *value = nl_map_text_get(query, key);")
        self.emit("if (!value || !*value) {")
        self.indent += 1
        self.emit('char buffer[256];')
        self.emit('snprintf(buffer, sizeof(buffer), "ValideringsFeil: mangler query-felt \x27%s\x27", key ? key : "");')
        self.emit("nl_throw(buffer);")
        self.emit('return nl_strdup("");')
        self.indent -= 1
        self.emit("}")
        self.emit("return nl_strdup(value);")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_web_request_query_int(nl_map_text *ctx, const char *key) {")
        self.indent += 1
        self.emit("nl_map_text *query = nl_web_request_query(ctx);")
        self.emit("return nl_web_validate_int_text(nl_map_text_get(query, key), key, \"query\");")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static nl_map_text *nl_web_request_json(nl_map_text *ctx) {")
        self.indent += 1
        self.emit("char *body = nl_web_request_body(ctx);")
        self.emit("const char *p = body;")
        self.emit("while (p && *p && isspace((unsigned char)*p)) { p++; }")
        self.emit("if (!p || !*p) {")
        self.indent += 1
        self.emit("return nl_map_text_new();")
        self.indent -= 1
        self.emit("}")
        self.emit("if (*p != '{') { nl_throw(\"ValideringsFeil: body forventet JSON-objekt\"); }")
        self.emit("return json_parse(body);")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_web_request_json_field(nl_map_text *ctx, const char *key) {")
        self.indent += 1
        self.emit("nl_map_text *data = nl_web_request_json(ctx);")
        self.emit("char *value = nl_map_text_get(data, key);")
        self.emit("if (!value || !*value) {")
        self.indent += 1
        self.emit('char buffer[256];')
        self.emit('snprintf(buffer, sizeof(buffer), "ValideringsFeil: mangler felt \x27%s\x27", key ? key : "");')
        self.emit("nl_throw(buffer);")
        self.emit('return nl_strdup("");')
        self.indent -= 1
        self.emit("}")
        self.emit("return nl_strdup(value);")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_web_request_json_field_or(nl_map_text *ctx, const char *key, const char *fallback) {")
        self.indent += 1
        self.emit("nl_map_text *data = nl_web_request_json(ctx);")
        self.emit("char *value = nl_map_text_get(data, key);")
        self.emit("if (!value || !*value) { return nl_strdup(fallback ? fallback : \"\"); }")
        self.emit("return nl_strdup(value);")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_web_request_json_field_int(nl_map_text *ctx, const char *key) {")
        self.indent += 1
        self.emit("nl_map_text *data = nl_web_request_json(ctx);")
        self.emit("return nl_web_validate_int_text(nl_map_text_get(data, key), key, \"body\");")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_web_request_json_field_bool(nl_map_text *ctx, const char *key) {")
        self.indent += 1
        self.emit("nl_map_text *data = nl_web_request_json(ctx);")
        self.emit("return nl_web_validate_bool_text(nl_map_text_get(data, key), key, \"body\");")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_web_request_header(nl_map_text *ctx, const char *key) {")
        self.indent += 1
        self.emit("nl_map_text *headers = nl_web_request_headers(ctx);")
        self.emit("return nl_strdup(nl_map_text_get(headers, key));")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static nl_map_text *nl_web_response_builder(int status, nl_map_text *headers, const char *body) {")
        self.indent += 1
        self.emit("nl_map_text *result = nl_map_text_new();")
        self.emit("char status_buf[32];")
        self.emit("snprintf(status_buf, sizeof(status_buf), \"%d\", status);")
        self.emit("nl_map_text_set(result, \"status\", nl_strdup(status_buf));")
        self.emit("nl_map_text_set(result, \"headers\", json_stringify(headers ? headers : nl_map_text_new()));")
        self.emit("nl_map_text_set(result, \"body\", nl_strdup(body ? body : \"\"));")
        self.emit("return result;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_web_response_status(nl_map_text *response) {")
        self.indent += 1
        self.emit("return atoi(nl_map_text_get(response, \"status\"));")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static nl_map_text *nl_web_response_headers(nl_map_text *response) {")
        self.indent += 1
        self.emit("return json_parse(nl_map_text_get(response, \"headers\"));")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_web_response_body(nl_map_text *response) {")
        self.indent += 1
        self.emit("return nl_strdup(nl_map_text_get(response, \"body\"));")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_web_response_header(nl_map_text *response, const char *key) {")
        self.indent += 1
        self.emit("nl_map_text *headers = nl_web_response_headers(response);")
        self.emit("return nl_strdup(nl_map_text_get(headers, key));")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_web_response_text(nl_map_text *response) {")
        self.indent += 1
        self.emit("return nl_web_response_body(response);")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static nl_map_text *nl_web_response_json(nl_map_text *response) {")
        self.indent += 1
        self.emit("return json_parse(nl_map_text_get(response, \"body\"));")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static nl_map_text *nl_web_response_error(int status, const char *message) {")
        self.indent += 1
        self.emit("nl_map_text *headers = nl_map_text_new();")
        self.emit("nl_map_text_set(headers, \"content-type\", nl_strdup(\"application/json\"));")
        self.emit("nl_map_text *body = nl_map_text_new();")
        self.emit("nl_map_text_set(body, \"error\", nl_strdup(message ? message : \"\"));")
        self.emit("char *body_json = json_stringify(body);")
        self.emit("return nl_web_response_builder(status, headers, body_json);")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_web_route(const char *spec) {")
        self.indent += 1
        self.emit("return nl_strdup(spec ? spec : \"\");")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static nl_map_text *nl_web_handle_request(nl_map_text *ctx);")
        self.emit()

        self.emit("static nl_map_text *nl_web_response_file(const char *path, const char *content_type) {")
        self.indent += 1
        self.emit("char *body = nl_strdup(\"\");")
        self.emit("if (path) {")
        self.indent += 1
        self.emit("FILE *f = fopen(path, \"rb\");")
        self.emit("if (!f) { nl_throw(nl_io_error(\"lese\", path)); return nl_web_response_builder(200, nl_map_text_new(), body); }")
        self.emit("fseek(f, 0, SEEK_END);")
        self.emit("long len = ftell(f);")
        self.emit("fseek(f, 0, SEEK_SET);")
        self.emit("if (len < 0) { fclose(f); nl_throw(nl_io_error(\"lese\", path)); return nl_web_response_builder(200, nl_map_text_new(), body); }")
        self.emit("char *buffer = malloc((size_t)len + 1);")
        self.emit("if (!buffer) { fclose(f); perror(\"malloc\"); exit(1); }")
        self.emit("size_t read_len = fread(buffer, 1, (size_t)len, f);")
        self.emit("buffer[read_len] = '\\0';")
        self.emit("fclose(f);")
        self.emit("body = buffer;")
        self.indent -= 1
        self.emit("}")
        self.emit("nl_map_text *headers = nl_map_text_new();")
        self.emit("nl_map_text_set(headers, \"content-type\", nl_strdup(content_type ? content_type : \"application/octet-stream\"));")
        self.emit("return nl_web_response_builder(200, headers, body);")
        self.indent -= 1
        self.emit("}")
        self.emit()

        openapi_paths_literal = self.c_string(self.openapi_paths_json)
        self.emit(f"static const char *nl_web_openapi_paths_json = {openapi_paths_literal};")
        self.emit("static char *nl_web_openapi_json(const char *title, const char *version) {")
        self.indent += 1
        self.emit("nl_json_builder out;")
        self.emit("nl_json_builder_init(&out, 256);")
        self.emit("nl_json_builder_append_text(&out, \"{\\\"openapi\\\":\\\"3.0.3\\\",\\\"info\\\":{\");")
        self.emit("nl_json_builder_append_text(&out, \"\\\"title\\\":\");")
        self.emit("char *title_json = nl_json_quote(title ? title : \"Norscode API\");")
        self.emit("nl_json_builder_append_text(&out, title_json);")
        self.emit("nl_json_builder_append_text(&out, \",\\\"version\\\":\");")
        self.emit("char *version_json = nl_json_quote(version ? version : \"1.0.0\");")
        self.emit("nl_json_builder_append_text(&out, version_json);")
        self.emit("nl_json_builder_append_text(&out, \"},\\\"paths\\\":\");")
        self.emit("nl_json_builder_append_text(&out, nl_web_openapi_paths_json);")
        self.emit("nl_json_builder_append_char(&out, '}');")
        self.emit("return out.data;")
        self.indent -= 1
        self.emit("}")
        self.emit()
        self.emit("static char *nl_web_docs_html(const char *title, const char *version) {")
        self.indent += 1
        self.emit("char *spec = nl_web_openapi_json(title, version);")
        self.emit("nl_json_builder out;")
        self.emit("nl_json_builder_init(&out, 256);")
        self.emit("nl_json_builder_append_text(&out, \"<!doctype html><html><head><meta charset='utf-8'><title>\");")
        self.emit("nl_json_builder_append_text(&out, title ? title : \"Norscode API\");")
        self.emit("nl_json_builder_append_text(&out, \"</title><style>body{font-family:system-ui,sans-serif;max-width:960px;margin:2rem auto;padding:0 1rem;}pre{background:#f6f8fa;padding:1rem;overflow:auto;border-radius:8px;}code{background:#f6f8fa;padding:0.15rem 0.35rem;border-radius:4px;}</style></head><body>\");")
        self.emit("nl_json_builder_append_text(&out, \"<h1>\");")
        self.emit("nl_json_builder_append_text(&out, title ? title : \"Norscode API\");")
        self.emit("nl_json_builder_append_text(&out, \"</h1><p>Versjon: \");")
        self.emit("nl_json_builder_append_text(&out, version ? version : \"1.0.0\");")
        self.emit("nl_json_builder_append_text(&out, \"</p><h2>OpenAPI JSON</h2><pre>\");")
        self.emit("nl_json_builder_append_text(&out, spec);")
        self.emit("nl_json_builder_append_text(&out, \"</pre></body></html>\");")
        self.emit("return out.data;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_web_is_int_segment(const char *text) {")
        self.indent += 1
        self.emit("if (!text || !*text) { return 0; }")
        self.emit("const char *p = text;")
        self.emit("if (*p == '-') { p++; }")
        self.emit("if (!*p) { return 0; }")
        self.emit("while (*p) {")
        self.indent += 1
        self.emit("if (*p < '0' || *p > '9') { return 0; }")
        self.emit("p++;")
        self.indent -= 1
        self.emit("}")
        self.emit("return 1;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_web_match_pattern(const char *pattern, const char *path, nl_map_text *params) {")
        self.indent += 1
        self.emit("char *pattern_copy = nl_strdup(pattern ? pattern : \"\");")
        self.emit("char *path_copy = nl_strdup(path ? path : \"\");")
        self.emit("char *pattern_state = NULL;")
        self.emit("char *path_state = NULL;")
        self.emit("char *pattern_part = strtok_r(pattern_copy, \"/\", &pattern_state);")
        self.emit("char *path_part = strtok_r(path_copy, \"/\", &path_state);")
        self.emit("while (pattern_part || path_part) {")
        self.indent += 1
        self.emit("if (!pattern_part || !path_part) { free(pattern_copy); free(path_copy); return 0; }")
        self.emit("size_t pattern_len = strlen(pattern_part);")
        self.emit("if (pattern_len >= 3 && pattern_part[0] == '{' && pattern_part[pattern_len - 1] == '}') {")
        self.indent += 1
        self.emit("char *inner = nl_strdup(pattern_part + 1);")
        self.emit("inner[pattern_len - 2] = '\\0';")
        self.emit("char *colon = strchr(inner, ':');")
        self.emit("char *name = inner;")
        self.emit("char *type = NULL;")
        self.emit("if (colon) {")
        self.indent += 1
        self.emit("*colon = '\\0';")
        self.emit("type = colon + 1;")
        self.indent -= 1
        self.emit("}")
        self.emit("if (!name || !*name) { free(inner); free(pattern_copy); free(path_copy); return 0; }")
        self.emit("if (type && strcmp(type, \"int\") == 0 && !nl_web_is_int_segment(path_part)) { free(inner); free(pattern_copy); free(path_copy); return 0; }")
        self.emit("if (params) { nl_map_text_set(params, nl_strdup(name), nl_strdup(path_part)); }")
        self.emit("free(inner);")
        self.indent -= 1
        self.emit("} else if (strcmp(pattern_part, path_part) != 0) {")
        self.indent += 1
        self.emit("free(pattern_copy);")
        self.emit("free(path_copy);")
        self.emit("return 0;")
        self.indent -= 1
        self.emit("}")
        self.emit("pattern_part = strtok_r(NULL, \"/\", &pattern_state);")
        self.emit("path_part = strtok_r(NULL, \"/\", &path_state);")
        self.indent -= 1
        self.emit("}")
        self.emit("free(pattern_copy);")
        self.emit("free(path_copy);")
        self.emit("return 1;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static void nl_web_split_route_spec(const char *spec, char **method_out, char **path_out) {")
        self.indent += 1
        self.emit("char *copy = nl_strdup(spec ? spec : \"\");")
        self.emit("char *space = strchr(copy, ' ');")
        self.emit("if (!space) {")
        self.indent += 1
        self.emit("*method_out = nl_strdup(\"\");")
        self.emit("*path_out = copy;")
        self.emit("return;")
        self.indent -= 1
        self.emit("}")
        self.emit("*space = '\\0';")
        self.emit("*method_out = nl_strdup(copy);")
        self.emit("for (char *p = *method_out; p && *p; ++p) { *p = (char)toupper((unsigned char)*p); }")
        self.emit("*path_out = nl_strdup(space + 1);")
        self.emit("free(copy);")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_web_route_match(const char *spec, const char *method, const char *path) {")
        self.indent += 1
        self.emit("char *route_method = NULL;")
        self.emit("char *route_path = NULL;")
        self.emit("nl_web_split_route_spec(spec, &route_method, &route_path);")
        self.emit("int ok = 1;")
        self.emit("if (route_method && *route_method) {")
        self.indent += 1
        self.emit("char *method_copy = nl_strdup(method ? method : \"\");")
        self.emit("for (char *p = method_copy; p && *p; ++p) { *p = (char)toupper((unsigned char)*p); }")
        self.emit("ok = strcmp(route_method, method_copy) == 0;")
        self.emit("free(method_copy);")
        self.indent -= 1
        self.emit("}")
        self.emit("if (ok) { ok = nl_web_match_pattern(route_path, path, NULL); }")
        self.emit("free(route_method);")
        self.emit("free(route_path);")
        self.emit("return ok;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_web_dispatch(nl_map_text *routes, const char *method, const char *path) {")
        self.indent += 1
        self.emit("if (!routes) { return nl_strdup(\"\"); }")
        self.emit("for (int i = 0; i < routes->len; ++i) {")
        self.indent += 1
        self.emit("if (nl_web_route_match(routes->values[i], method, path)) {")
        self.indent += 1
        self.emit("return nl_strdup(routes->keys[i]);")
        self.indent -= 1
        self.emit("}")
        self.indent -= 1
        self.emit("}")
        self.emit("return nl_strdup(\"\");")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static nl_map_text *nl_web_dispatch_params(nl_map_text *routes, const char *method, const char *path) {")
        self.indent += 1
        self.emit("nl_map_text *result = nl_map_text_new();")
        self.emit("if (!routes) { return result; }")
        self.emit("for (int i = 0; i < routes->len; ++i) {")
        self.indent += 1
        self.emit("char *route_method = NULL;")
        self.emit("char *route_path = NULL;")
        self.emit("nl_web_split_route_spec(routes->values[i], &route_method, &route_path);")
        self.emit("int ok = 1;")
        self.emit("if (route_method && *route_method) {")
        self.indent += 1
        self.emit("char *method_copy = nl_strdup(method ? method : \"\");")
        self.emit("for (char *p = method_copy; p && *p; ++p) { *p = (char)toupper((unsigned char)*p); }")
        self.emit("ok = strcmp(route_method, method_copy) == 0;")
        self.emit("free(method_copy);")
        self.indent -= 1
        self.emit("}")
        self.emit("if (ok) {")
        self.indent += 1
        self.emit("nl_map_text *params = nl_map_text_new();")
        self.emit("if (nl_web_match_pattern(route_path, path, params)) {")
        self.indent += 1
        self.emit("free(route_method);")
        self.emit("free(route_path);")
        self.emit("return params;")
        self.indent -= 1
        self.emit("}")
        self.emit("}")
        self.emit("free(route_method);")
        self.emit("free(route_path);")
        self.indent -= 1
        self.emit("}")
        self.emit("return result;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_web_path_match(const char *pattern, const char *path) {")
        self.indent += 1
        self.emit("return nl_web_match_pattern(pattern, path, NULL);")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static nl_map_text *nl_web_path_params(const char *pattern, const char *path) {")
        self.indent += 1
        self.emit("nl_map_text *result = nl_map_text_new();")
        self.emit("if (!nl_web_match_pattern(pattern, path, result)) {")
        self.indent += 1
        self.emit("return nl_map_text_new();")
        self.indent -= 1
        self.emit("}")
        self.emit("return result;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_text_starter_med(const char *text, const char *prefix) {")
        self.indent += 1
        self.emit("return text && prefix ? strncmp(text, prefix, strlen(prefix)) == 0 : 0;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_text_slutter_med(const char *text, const char *suffix) {")
        self.indent += 1
        self.emit("if (!text || !suffix) { return 0; }")
        self.emit("size_t text_len = strlen(text);")
        self.emit("size_t suffix_len = strlen(suffix);")
        self.emit("if (suffix_len > text_len) { return 0; }")
        self.emit("return strcmp(text + (text_len - suffix_len), suffix) == 0;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_text_inneholder(const char *text, const char *needle) {")
        self.indent += 1
        self.emit("if (!text || !needle) { return 0; }")
        self.emit("return strstr(text, needle) != NULL;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_text_trim(const char *text) {")
        self.indent += 1
        self.emit("if (!text) { return nl_strdup(\"\"); }")
        self.emit("const char *start = text;")
        self.emit("while (*start && isspace((unsigned char)*start)) { start++; }")
        self.emit("const char *end = text + strlen(text);")
        self.emit("while (end > start && isspace((unsigned char)*(end - 1))) { end--; }")
        self.emit("size_t len = (size_t)(end - start);")
        self.emit("char *out = malloc(len + 1);")
        self.emit("if (!out) { perror(\"malloc\"); exit(1); }")
        self.emit("memcpy(out, start, len);")
        self.emit("out[len] = '\\0';")
        self.emit("return out;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_text_slice(const char *text, int start, int has_start, int end, int has_end) {")
        self.indent += 1
        self.emit("if (!text) { return nl_strdup(\"\"); }")
        self.emit("int len = (int)strlen(text);")
        self.emit("int s = nl_slice_start(len, start, has_start);")
        self.emit("int e = nl_slice_end(len, end, has_end);")
        self.emit("if (e < s) { e = s; }")
        self.emit("int out_len = e - s;")
        self.emit("char *out = malloc((size_t)out_len + 1);")
        self.emit("if (!out) { perror(\"malloc\"); exit(1); }")
        self.emit("memcpy(out, text + s, (size_t)out_len);")
        self.emit("out[out_len] = '\\0';")
        self.emit("return out;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static nl_list_int *nl_list_int_slice(nl_list_int *list, int start, int has_start, int end, int has_end) {")
        self.indent += 1
        self.emit("nl_list_int *out = nl_list_int_new();")
        self.emit("if (!list) { return out; }")
        self.emit("int s = nl_slice_start(list->len, start, has_start);")
        self.emit("int e = nl_slice_end(list->len, end, has_end);")
        self.emit("if (e < s) { e = s; }")
        self.emit("for (int i = s; i < e; i++) { nl_list_int_push(out, list->data[i]); }")
        self.emit("return out;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static nl_list_text *nl_list_text_slice(nl_list_text *list, int start, int has_start, int end, int has_end) {")
        self.indent += 1
        self.emit("nl_list_text *out = nl_list_text_new();")
        self.emit("if (!list) { return out; }")
        self.emit("int s = nl_slice_start(list->len, start, has_start);")
        self.emit("int e = nl_slice_end(list->len, end, has_end);")
        self.emit("if (e < s) { e = s; }")
        self.emit("for (int i = s; i < e; i++) { nl_list_text_push(out, nl_strdup(list->data[i])); }")
        self.emit("return out;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_file_read_text(const char *path) {")
        self.indent += 1
        self.emit("if (!path) { return nl_strdup(\"\"); }")
        self.emit("FILE *f = fopen(path, \"rb\");")
        self.emit("if (!f) { nl_throw(nl_io_error(\"lese\", path)); return nl_strdup(\"\"); }")
        self.emit("fseek(f, 0, SEEK_END);")
        self.emit("long len = ftell(f);")
        self.emit("fseek(f, 0, SEEK_SET);")
        self.emit("if (len < 0) { fclose(f); nl_throw(nl_io_error(\"lese\", path)); return nl_strdup(\"\"); }")
        self.emit("char *buffer = malloc((size_t)len + 1);")
        self.emit("if (!buffer) { fclose(f); perror(\"malloc\"); exit(1); }")
        self.emit("size_t read_len = fread(buffer, 1, (size_t)len, f);")
        self.emit("buffer[read_len] = '\\0';")
        self.emit("fclose(f);")
        self.emit("return buffer;")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static char *nl_file_write_text(const char *path, const char *text, const char *mode) {")
        self.indent += 1
        self.emit("if (!path) { return nl_strdup(text ? text : \"\"); }")
        self.emit("FILE *f = fopen(path, mode);")
        self.emit("if (!f) { nl_throw(nl_io_error(strcmp(mode, \"a\") == 0 ? \"legge til i\" : \"skrive\", path)); return nl_strdup(text ? text : \"\"); }")
        self.emit("if (text) { fputs(text, f); }")
        self.emit("fclose(f);")
        self.emit("return nl_strdup(text ? text : \"\");")
        self.indent -= 1
        self.emit("}")
        self.emit()

        self.emit("static int nl_file_exists(const char *path) {")
        self.indent += 1
        self.emit("if (!path) { return 0; }")
        self.emit("FILE *f = fopen(path, \"rb\");")
        self.emit("if (!f) { return 0; }")
        self.emit("fclose(f);")
        self.emit("return 1;")
        self.indent -= 1
        self.emit("}")
        self.emit()

    def signature(self, fn):
        params = ", ".join(f"{self.c_type(p.type_name)} {p.name}" for p in fn.params)
        c_name = self.resolve_c_function_name(fn.name, module_name=fn.module_name)
        return f"{self.c_type(fn.return_type)} {c_name}({params})"

    def emit_list_literal_assignment(self, name, list_type, node, declare=False):
        ctor = "nl_list_int_new" if list_type == TYPE_LIST_INT else "nl_list_text_new"
        push = "nl_list_int_push" if list_type == TYPE_LIST_INT else "nl_list_text_push"
        prefix = f"{self.c_type(list_type)} " if declare else ""
        self.emit(f"{prefix}{name} = {ctor}();")
        for item in node.items:
            item_code, _ = self.expr_with_type(item)
            self.emit(f"{push}({name}, {item_code});")

    def _next_temp(self, prefix="tmp"):
        self.temp_counter += 1
        return f"__nl_{prefix}_{self.temp_counter}"

    def _emit_nested_map_literal(self, node, map_type):
        set_map = self.map_set_fn_for_type(map_type)
        if set_map is None:
            return "0", map_type

        tmp = self._next_temp("map")
        self.emit(f"{self.c_type(map_type)} {tmp} = {self.map_ctor_for_type(map_type)};")

        value_type = self.map_value_type(map_type)
        for key_expr, value_expr in node.items:
            key_code, _ = self.expr_with_type(key_expr)
            if self.is_map_type(value_type) and isinstance(value_expr, MapLiteralNode):
                value_code, _ = self._emit_nested_map_literal(value_expr, value_type)
            elif self.is_map_type(value_type) and isinstance(value_expr, StructLiteralNode):
                value_code, _ = self._emit_nested_struct_literal(value_expr, value_type)
            else:
                value_code, _ = self.expr_with_type(value_expr)
            self.emit(f"{set_map}({tmp}, {key_code}, {value_code});")
        return tmp, map_type

    def _emit_nested_struct_literal(self, node, map_type):
        set_map = self.map_set_fn_for_type(map_type)
        if set_map is None:
            return "0", map_type

        tmp = self._next_temp("map")
        self.emit(f"{self.c_type(map_type)} {tmp} = {self.map_ctor_for_type(map_type)};")

        value_type = self.map_value_type(map_type)
        for field_name, value_expr in node.fields:
            if self.is_map_type(value_type) and isinstance(value_expr, MapLiteralNode):
                value_code, _ = self._emit_nested_map_literal(value_expr, value_type)
            elif self.is_map_type(value_type) and isinstance(value_expr, StructLiteralNode):
                value_code, _ = self._emit_nested_struct_literal(value_expr, value_type)
            else:
                value_code, _ = self.expr_with_type(value_expr)
            self.emit(f'{set_map}({tmp}, {self.c_string(field_name)}, {value_code});')
        return tmp, map_type

    def emit_map_literal_assignment(self, name, map_type, node, declare=False):
        set_map = self.map_set_fn_for_type(map_type)
        if set_map is None:
            return
        prefix = f"{self.c_type(map_type)} " if declare else ""
        self.emit(f"{prefix}{name} = {self.map_ctor_for_type(map_type)};")
        for key_expr, value_expr in node.items:
            key_code, _ = self.expr_with_type(key_expr)
            value_type = self.map_value_type(map_type)
            if self.is_map_type(value_type) and isinstance(value_expr, MapLiteralNode):
                value_code, _ = self._emit_nested_map_literal(value_expr, value_type)
            elif self.is_map_type(value_type) and isinstance(value_expr, StructLiteralNode):
                value_code, _ = self._emit_nested_struct_literal(value_expr, value_type)
            else:
                value_code, _ = self.expr_with_type(value_expr)
            self.emit(f"{set_map}({name}, {key_code}, {value_code});")

    def emit_struct_literal_assignment(self, name, map_type, node, declare=False):
        set_map = self.map_set_fn_for_type(map_type)
        if set_map is None:
            return
        prefix = f"{self.c_type(map_type)} " if declare else ""
        self.emit(f"{prefix}{name} = {self.map_ctor_for_type(map_type)};")
        for field_name, value_expr in node.fields:
            value_type = self.map_value_type(map_type)
            if self.is_map_type(value_type) and isinstance(value_expr, MapLiteralNode):
                value_code, _ = self._emit_nested_map_literal(value_expr, value_type)
            elif self.is_map_type(value_type) and isinstance(value_expr, StructLiteralNode):
                value_code, _ = self._emit_nested_struct_literal(value_expr, value_type)
            else:
                value_code, _ = self.expr_with_type(value_expr)
            self.emit(f'{set_map}({name}, {self.c_string(field_name)}, {value_code});')

    def visit_function(self, fn):
        previous_module = self.current_module
        previous_return_type = self.current_return_type
        self.current_module = getattr(fn, "module_name", "__main__")
        self.current_return_type = fn.return_type
        self.var_types = {p.name: p.type_name for p in fn.params}

        self.emit(self.signature(fn) + " {")
        self.indent += 1
        function_name = getattr(fn, "module_name", None)
        if function_name and function_name != "__main__":
            full_name = f"{function_name}.{fn.name}"
        else:
            full_name = fn.name
        self.emit("const int nl_call_stack_mark = nl_call_stack_len;")
        self.emit(f"nl_call_stack[nl_call_stack_len++] = {self.c_string(full_name)};")

        for stmt in fn.body.statements:
            self.visit_stmt(stmt)

        if fn.return_type in (TYPE_INT, TYPE_BOOL):
            self.emit("nl_call_stack_len = nl_call_stack_mark;")
            self.emit("return 0;")
        elif fn.return_type == TYPE_TEXT:
            self.emit("nl_call_stack_len = nl_call_stack_mark;")
            self.emit('return "";')
        elif fn.return_type == TYPE_LIST_INT:
            self.emit("nl_call_stack_len = nl_call_stack_mark;")
            self.emit("return nl_list_int_new();")
        elif fn.return_type == TYPE_LIST_TEXT:
            self.emit("nl_call_stack_len = nl_call_stack_mark;")
            self.emit("return nl_list_text_new();")
        elif self.is_map_type(fn.return_type):
            self.emit("nl_call_stack_len = nl_call_stack_mark;")
            self.emit(f"return {self.map_ctor_for_type(fn.return_type)};")
        else:
            self.emit("nl_call_stack_len = nl_call_stack_mark;")
            self.emit("return 0;")

        self.indent -= 1
        self.emit("}")
        self.current_module = previous_module
        self.current_return_type = previous_return_type

    def visit_stmt(self, stmt):
        if isinstance(stmt, VarDeclareNode):
            inferred_type = stmt.var_type
            if inferred_type is None:
                if isinstance(stmt.expr, ListLiteralNode):
                    item_types = [self.expr_with_type(item)[1] for item in stmt.expr.items]
                    inferred_type = TYPE_LIST_TEXT if item_types and item_types[0] == TYPE_TEXT else TYPE_LIST_INT
                elif isinstance(stmt.expr, MapLiteralNode):
                    if not stmt.expr.items:
                        inferred_type = TYPE_MAP_TEXT
                    else:
                        value_types = [self.expr_with_type(value_expr)[1] for _key_expr, value_expr in stmt.expr.items]
                        inferred_type = self.map_type_for_value(value_types[0])
                        if inferred_type is None:
                            inferred_type = TYPE_MAP_INT
                elif isinstance(stmt.expr, StructLiteralNode):
                    value_types = [self.expr_with_type(value_expr)[1] for _field_name, value_expr in stmt.expr.fields]
                    inferred_type = self.map_type_for_value(value_types[0])
                    if inferred_type is None:
                        inferred_type = TYPE_MAP_INT
                else:
                    _, inferred_type = self.expr_with_type(stmt.expr)
            self.var_types[stmt.name] = inferred_type
            if isinstance(stmt.expr, ListLiteralNode):
                self.emit_list_literal_assignment(stmt.name, inferred_type, stmt.expr, declare=True)
                return
            if isinstance(stmt.expr, MapLiteralNode):
                self.emit_map_literal_assignment(stmt.name, inferred_type, stmt.expr, declare=True)
                return
            if isinstance(stmt.expr, StructLiteralNode):
                self.emit_struct_literal_assignment(stmt.name, inferred_type, stmt.expr, declare=True)
                return
            expr_code, _expr_type = self.expr_with_type(stmt.expr)
            self.emit(f"{self.c_type(inferred_type)} {stmt.name} = {expr_code};")
            return

        if isinstance(stmt, VarSetNode):
            var_type = self.var_types.get(stmt.name)
            if isinstance(stmt.expr, ListLiteralNode) and var_type in (TYPE_LIST_INT, TYPE_LIST_TEXT):
                self.emit_list_literal_assignment(stmt.name, var_type, stmt.expr, declare=False)
                return
            if isinstance(stmt.expr, MapLiteralNode) and self.is_map_type(var_type):
                self.emit_map_literal_assignment(stmt.name, var_type, stmt.expr, declare=False)
                return
            if isinstance(stmt.expr, StructLiteralNode) and self.is_map_type(var_type):
                self.emit_struct_literal_assignment(stmt.name, var_type, stmt.expr, declare=False)
                return
            expr_code, _expr_type = self.expr_with_type(stmt.expr)
            self.emit(f"{stmt.name} = {expr_code};")
            return

        if isinstance(stmt, IndexSetNode):
            idx_code, _ = self.expr_with_type(stmt.index_expr)
            val_code, _ = self.expr_with_type(stmt.value_expr)
            target_type = self.var_types.get(stmt.target_name)
            if target_type == TYPE_LIST_TEXT:
                self.emit(f"{stmt.target_name}->data[{idx_code}] = {val_code};")
            elif self.is_map_type(target_type):
                set_map = self.map_set_fn_for_type(target_type)
                if set_map is None:
                    self.emit(f"{stmt.target_name}->data[{idx_code}] = {val_code};")
                else:
                    self.emit(f"{set_map}({stmt.target_name}, {idx_code}, {val_code});")
            else:
                self.emit(f"{stmt.target_name}->data[{idx_code}] = {val_code};")
            return

        if isinstance(stmt, PrintNode):
            expr_code, expr_type = self.expr_with_type(stmt.expr)
            if expr_type == TYPE_TEXT:
                self.emit(f"nl_print_text({expr_code});")
            else:
                self.emit(f'printf("%d\\n", {expr_code});')
            return

        if isinstance(stmt, IfNode):
            cond_code, _ = self.expr_with_type(stmt.condition)
            self.emit(f"if {self.format_condition(cond_code)} {{")
            self.indent += 1
            for inner in stmt.then_block.statements:
                self.visit_stmt(inner)
            self.indent -= 1
            self.emit("}")

            for elif_cond, elif_block in stmt.elif_blocks:
                elif_code, _ = self.expr_with_type(elif_cond)
                self.emit(f"else if {self.format_condition(elif_code)} {{")
                self.indent += 1
                for inner in elif_block.statements:
                    self.visit_stmt(inner)
                self.indent -= 1
                self.emit("}")

            if stmt.else_block is not None:
                self.emit("else {")
                self.indent += 1
                for inner in stmt.else_block.statements:
                    self.visit_stmt(inner)
                self.indent -= 1
                self.emit("}")
            return

        if isinstance(stmt, MatchNode):
            subject_code, subject_type = self.expr_with_type(stmt.subject)
            self.temp_counter += 1
            subject_var = f"__nl_match_{self.temp_counter}"
            self.emit(f"{self.c_type(subject_type)} {subject_var} = {subject_code};")

            cases = list(getattr(stmt, "cases", []))
            has_else = stmt.else_block is not None
            emitted_branch = False
            wildcard_seen = False

            for idx, case in enumerate(cases):
                if getattr(case, "wildcard", False):
                    wildcard_seen = True
                    if idx != len(cases) - 1:
                        raise RuntimeError("Wildcard-case i match må være siste case")
                    if emitted_branch:
                        self.emit("else {")
                    else:
                        self.emit("if (1) {")
                    self.indent += 1
                    for inner in case.body.statements:
                        self.visit_stmt(inner)
                    self.indent -= 1
                    self.emit("}")
                    emitted_branch = True
                    break

                pattern_code, pattern_type = self.expr_with_type(case.pattern, subject_type)
                if pattern_type == TYPE_TEXT:
                    cond_code = f"nl_streq({subject_var}, {pattern_code})"
                else:
                    cond_code = f"({subject_var} == {pattern_code})"

                if not emitted_branch:
                    self.emit(f"if {self.format_condition(cond_code)} {{")
                else:
                    self.emit(f"else if {self.format_condition(cond_code)} {{")
                self.indent += 1
                for inner in case.body.statements:
                    self.visit_stmt(inner)
                self.indent -= 1
                self.emit("}")
                emitted_branch = True

            if has_else and not wildcard_seen:
                if not emitted_branch:
                    self.emit("if (1) {")
                else:
                    self.emit("else {")
                self.indent += 1
                for inner in stmt.else_block.statements:
                    self.visit_stmt(inner)
                self.indent -= 1
                self.emit("}")
            return

        if isinstance(stmt, WhileNode):
            cond_code, _ = self.expr_with_type(stmt.condition)
            self.emit(f"while {self.format_condition(cond_code)} {{")
            self.indent += 1
            for inner in stmt.body.statements:
                self.visit_stmt(inner)
            self.indent -= 1
            self.emit("}")
            return

        if isinstance(stmt, ForNode):
            var_name = getattr(stmt, "name", None) or getattr(stmt, "var_name", None) or getattr(stmt, "item_name", None)
            start_code, _ = self.expr_with_type(getattr(stmt, "start_expr", getattr(stmt, "start", NumberNode(0))))
            end_code, _ = self.expr_with_type(getattr(stmt, "end_expr", getattr(stmt, "end", NumberNode(0))))
            step_code, _ = self.expr_with_type(getattr(stmt, "step_expr", NumberNode(1)))

            self.temp_counter += 1
            step_var = f"__nl_step_{self.temp_counter}"
            prev_type = self.var_types.get(var_name)
            self.var_types[var_name] = TYPE_INT

            self.emit(
                f"for (int {var_name} = {start_code}, {step_var} = {step_code}; "
                f"({step_var} >= 0 ? {var_name} <= {end_code} : {var_name} >= {end_code}); "
                f"{var_name} += {step_var}) {{"
            )
            self.indent += 1
            for inner in stmt.body.statements:
                self.visit_stmt(inner)
            self.indent -= 1
            self.emit("}")

            if prev_type is None:
                self.var_types.pop(var_name, None)
            else:
                self.var_types[var_name] = prev_type
            return

        if isinstance(stmt, ForEachNode):
            list_code, list_type = self.expr_with_type(stmt.list_expr)
            item_type = TYPE_INT if list_type == TYPE_LIST_INT else TYPE_TEXT
            c_item_type = self.c_type(item_type)

            self.temp_counter += 1
            idx_var = f"__nl_i_{self.temp_counter}"
            len_expr = f"nl_list_int_len({list_code})" if list_type == TYPE_LIST_INT else f"nl_list_text_len({list_code})"
            item_expr = f"{list_code}->data[{idx_var}]"

            prev_type = self.var_types.get(stmt.item_name)

            self.emit(f"for (int {idx_var} = 0; {idx_var} < {len_expr}; {idx_var}++) {{")
            self.indent += 1
            self.emit(f"{c_item_type} {stmt.item_name} = {item_expr};")
            self.var_types[stmt.item_name] = item_type
            for inner in stmt.body.statements:
                self.visit_stmt(inner)
            self.indent -= 1
            self.emit("}")

            if prev_type is None:
                self.var_types.pop(stmt.item_name, None)
            else:
                self.var_types[stmt.item_name] = prev_type
            return

        if isinstance(stmt, ReturnNode):
            expr_code, _expr_type = self.expr_with_type(stmt.expr, self.current_return_type)
            self.emit("nl_call_stack_len = nl_call_stack_mark;")
            self.emit(f"return {expr_code};")
            return

        if isinstance(stmt, ThrowNode):
            expr_code, _ = self.expr_with_type(stmt.expr)
            self.emit(f"nl_throw({expr_code});")
            return

        if isinstance(stmt, TryCatchNode):
            try_frame = self._next_temp("try")
            catch_var = stmt.catch_var_name

            self.emit(f"nl_try_frame {try_frame};")
            self.emit(f"nl_push_try_frame(&{try_frame});")
            self.emit(f"if (setjmp({try_frame}.jump_point) == 0) {{")
            self.indent += 1
            for inner in stmt.try_block.statements:
                self.visit_stmt(inner)
            self.emit(f"nl_pop_try_frame();")
            self.indent -= 1
            self.emit("} else {")
            self.indent += 1
            self.emit("nl_pop_try_frame();")
            if catch_var is not None:
                prev_type = self.var_types.get(catch_var)
                if prev_type is None:
                    self.emit(
                        f"char *{catch_var} = nl_strdup({try_frame}.error_message ? {try_frame}.error_message : \"\");"
                    )
                    self.var_types[catch_var] = TYPE_TEXT
                    self.emit(f"nl_call_stack_len = {try_frame}.call_depth;")
                    for inner in stmt.catch_block.statements:
                        self.visit_stmt(inner)
                    self.var_types.pop(catch_var, None)
                else:
                    self.emit(
                        f"{catch_var} = nl_strdup({try_frame}.error_message ? {try_frame}.error_message : \"\");"
                    )
                    self.emit(f"nl_call_stack_len = {try_frame}.call_depth;")
                    for inner in stmt.catch_block.statements:
                        self.visit_stmt(inner)
            else:
                self.emit(f"nl_call_stack_len = {try_frame}.call_depth;")
                for inner in stmt.catch_block.statements:
                    self.visit_stmt(inner)
            self.indent -= 1
            self.emit("}")
            return

        if isinstance(stmt, BreakNode):
            self.emit("break;")
            return

        if isinstance(stmt, ContinueNode):
            self.emit("continue;")
            return

        if isinstance(stmt, ExprStmtNode):
            expr_code, _expr_type = self.expr_with_type(stmt.expr)
            self.emit(f"{expr_code};")
            return

    def _is_empty_list(self, node):
        return isinstance(node, ListLiteralNode) and not node.items

    def _is_empty_map(self, node):
        return isinstance(node, MapLiteralNode) and not node.items

    def expr_with_type(self, node, expected_type=None):
        if isinstance(node, NumberNode):
            return str(node.value), TYPE_INT

        if isinstance(node, StringNode):
            return self.c_string(node.value), TYPE_TEXT

        if isinstance(node, BoolNode):
            return ("1" if node.value else "0"), TYPE_BOOL

        if isinstance(node, ListLiteralNode):
            if not node.items:
                if expected_type == TYPE_LIST_TEXT:
                    return ("nl_list_text_new()", TYPE_LIST_TEXT)
                if expected_type == TYPE_LIST_INT:
                    return ("nl_list_int_new()", TYPE_LIST_INT)
                return ("nl_list_int_new()", TYPE_LIST_INT)
            return ("0", TYPE_LIST_INT)

        if isinstance(node, ListComprehensionNode):
            source_code, source_type = self.expr_with_type(node.source_expr)
            if source_type not in (TYPE_LIST_INT, TYPE_LIST_TEXT):
                return ("nl_list_int_new()", TYPE_LIST_INT)

            item_type = TYPE_INT if source_type == TYPE_LIST_INT else TYPE_TEXT
            result_var = self._next_temp("comp")
            source_var = self._next_temp("comp_src")
            index_var = self._next_temp("comp_i")
            item_var = self._next_temp("comp_item")
            skip_label = self._next_temp("comp_skip") if node.condition_expr is not None else None
            end_label = self._next_temp("comp_end")
            len_fn = "nl_list_text_len" if source_type == TYPE_LIST_TEXT else "nl_list_int_len"

            prev_type = self.var_types.get(node.item_name)
            self.push_name_aliases({node.item_name: item_var})
            self.var_types[node.item_name] = item_type
            preview_type = self.preview_expr_type(node.item_expr)[1]
            self.pop_name_aliases()
            if prev_type is None:
                self.var_types.pop(node.item_name, None)
            else:
                self.var_types[node.item_name] = prev_type

            result_type = TYPE_LIST_TEXT if preview_type == TYPE_TEXT else TYPE_LIST_INT
            ctor = "nl_list_text_new" if result_type == TYPE_LIST_TEXT else "nl_list_int_new"
            push = "nl_list_text_push" if result_type == TYPE_LIST_TEXT else "nl_list_int_push"

            self.emit(f"{self.c_type(source_type)} {source_var} = {source_code};")
            self.emit(f"{self.c_type(result_type)} {result_var} = {ctor}();")
            self.emit(f"for (int {index_var} = 0; {index_var} < {len_fn}({source_var}); {index_var}++) {{")
            self.indent += 1
            self.emit(f"{self.c_type(item_type)} {item_var} = {source_var}->data[{index_var}];")
            self.push_name_aliases({node.item_name: item_var})
            self.var_types[node.item_name] = item_type
            if node.condition_expr is not None:
                cond_code, _ = self.expr_with_type(node.condition_expr)
                self.emit(f"if {self.format_condition(cond_code)} {{")
                self.indent += 1
            item_code, _ = self.expr_with_type(node.item_expr)
            self.emit(f"{push}({result_var}, {item_code});")
            if node.condition_expr is not None:
                self.indent -= 1
                self.emit("}")
            if prev_type is None:
                self.var_types.pop(node.item_name, None)
            else:
                self.var_types[node.item_name] = prev_type
            self.pop_name_aliases()
            self.indent -= 1
            self.emit("}")
            return (result_var, result_type)

        if isinstance(node, LambdaNode):
            ctype = self.register_lambda(node)
            if ctype.helper_name not in self.emitted_lambda_types:
                self.emitted_lambda_types.add(ctype.helper_name)
                self.emit("typedef struct {")
                self.indent += 1
                for name, t in zip(ctype.capture_names, ctype.capture_types):
                    self.emit(f"{self.c_type(t)} {name};")
                self.indent -= 1
                self.emit(f"}} {ctype.closure_name};")
            self.emit_lambda_prototype(ctype)
            fields = []
            for capture_name in ctype.capture_names:
                capture_code, _ = self.expr_with_type(VarAccessNode(capture_name))
                fields.append(f".{capture_name} = {capture_code}")
            init = ", ".join(fields)
            return (f"({self.c_type(ctype)}){{ {init} }}", ctype)

        if isinstance(node, MapLiteralNode):
            if expected_type == TYPE_MAP_INT:
                if not node.items:
                    return ("nl_map_int_new()", TYPE_MAP_INT)
                return self._emit_nested_map_literal(node, TYPE_MAP_INT)
            if expected_type == TYPE_MAP_TEXT:
                if not node.items:
                    return ("nl_map_text_new()", TYPE_MAP_TEXT)
                return self._emit_nested_map_literal(node, TYPE_MAP_TEXT)
            if expected_type == TYPE_MAP_BOOL:
                if not node.items:
                    return ("nl_map_bool_new()", TYPE_MAP_BOOL)
                return self._emit_nested_map_literal(node, TYPE_MAP_BOOL)
            if self.is_map_type(expected_type):
                if not node.items:
                    return (self.map_ctor_for_type(expected_type), expected_type)
                return self._emit_nested_map_literal(node, expected_type)
            if not node.items:
                return ("nl_map_text_new()", TYPE_MAP_TEXT)
            value_types = [self.expr_with_type(value_expr)[1] for _key_expr, value_expr in node.items]
            inferred_type = self.map_type_for_value(value_types[0])
            if inferred_type is None:
                inferred_type = TYPE_MAP_INT
            return self._emit_nested_map_literal(node, inferred_type)
        if isinstance(node, StructLiteralNode):
            if expected_type == TYPE_MAP_INT:
                if not node.fields:
                    return ("nl_map_int_new()", TYPE_MAP_INT)
                return self._emit_nested_struct_literal(node, TYPE_MAP_INT)
            if expected_type == TYPE_MAP_TEXT:
                if not node.fields:
                    return ("nl_map_text_new()", TYPE_MAP_TEXT)
                return self._emit_nested_struct_literal(node, TYPE_MAP_TEXT)
            if expected_type == TYPE_MAP_BOOL:
                if not node.fields:
                    return ("nl_map_bool_new()", TYPE_MAP_BOOL)
                return self._emit_nested_struct_literal(node, TYPE_MAP_BOOL)
            if self.is_map_type(expected_type):
                if not node.fields:
                    return (self.map_ctor_for_type(expected_type), expected_type)
                return self._emit_nested_struct_literal(node, expected_type)
            value_types = [self.expr_with_type(value_expr)[1] for _field_name, value_expr in node.fields]
            inferred_type = self.map_type_for_value(value_types[0])
            if inferred_type is None:
                inferred_type = TYPE_MAP_INT
            return self._emit_nested_struct_literal(node, inferred_type)

        if isinstance(node, VarAccessNode):
            resolved_name = self.resolve_name(node.name)
            return resolved_name, self.var_types.get(node.name, TYPE_INT)

        if isinstance(node, UnaryOpNode):
            inner_code, inner_type = self.expr_with_type(node.node)
            if node.op.typ == "IKKE":
                return f"(!({inner_code}))", TYPE_BOOL
            if node.op.typ == "PLUS":
                return f"(+({inner_code}))", inner_type
            if node.op.typ == "MINUS":
                return f"(-({inner_code}))", inner_type
            return inner_code, inner_type

        if isinstance(node, BinOpNode):
            left_code, left_type = self.expr_with_type(node.left)
            right_code, right_type = self.expr_with_type(node.right)

            if node.op.typ == "PLUS":
                if left_type == TYPE_TEXT and right_type == TYPE_TEXT:
                    return f"nl_concat({left_code}, {right_code})", TYPE_TEXT
                return f"({left_code} + {right_code})", TYPE_INT

            if node.op.typ == "MINUS":
                return f"({left_code} - {right_code})", TYPE_INT
            if node.op.typ == "MUL":
                return f"({left_code} * {right_code})", TYPE_INT
            if node.op.typ == "DIV":
                return f"({left_code} / {right_code})", TYPE_INT
            if node.op.typ == "PERCENT":
                return f"({left_code} % {right_code})", TYPE_INT

            if node.op.typ in ("GT", "LT", "GTE", "LTE"):
                if left_type == TYPE_TEXT and right_type == TYPE_TEXT:
                    cmp_map = {
                        "GT": " > 0",
                        "LT": " < 0",
                        "GTE": " >= 0",
                        "LTE": " <= 0",
                    }
                    return f"(strcmp({left_code} ? {left_code} : \"\", {right_code} ? {right_code} : \"\"){cmp_map[node.op.typ]})", TYPE_BOOL
                op_map = {"GT": ">", "LT": "<", "GTE": ">=", "LTE": "<="}
                return f"({left_code} {op_map[node.op.typ]} {right_code})", TYPE_BOOL

            if node.op.typ in ("EQ", "NE"):
                if left_type == TYPE_TEXT and right_type == TYPE_TEXT:
                    cmp_expr = f"nl_streq({left_code}, {right_code})"
                    return (cmp_expr if node.op.typ == "EQ" else f"!({cmp_expr})"), TYPE_BOOL
                op = "==" if node.op.typ == "EQ" else "!="
                return f"({left_code} {op} {right_code})", TYPE_BOOL

            if node.op.typ in ("OG", "ELLER"):
                op = "&&" if node.op.typ == "OG" else "||"
                return f"({left_code} {op} {right_code})", TYPE_BOOL

        if isinstance(node, IfExprNode):
            cond_code, _ = self.expr_with_type(node.condition)
            then_code, then_type = self.expr_with_type(node.then_expr, expected_type)
            else_code, else_type = self.expr_with_type(node.else_expr, expected_type)
            if then_type != else_type:
                if self._is_empty_list(node.then_expr) and else_type in (TYPE_LIST_INT, TYPE_LIST_TEXT):
                    then_code, then_type = self.expr_with_type(node.then_expr, else_type)
                elif self._is_empty_list(node.else_expr) and then_type in (TYPE_LIST_INT, TYPE_LIST_TEXT):
                    else_code, else_type = self.expr_with_type(node.else_expr, then_type)
                elif self._is_empty_map(node.then_expr) and self.is_map_type(else_type):
                    then_code, then_type = self.expr_with_type(node.then_expr, else_type)
                elif self._is_empty_map(node.else_expr) and self.is_map_type(then_type):
                    else_code, else_type = self.expr_with_type(node.else_expr, then_type)
            if then_type != else_type:
                return "0", TYPE_INT
            return f"(({cond_code}) ? {then_code} : {else_code})", then_type

        if isinstance(node, FieldAccessNode):
            target_code, target_type = self.expr_with_type(node.target)
            if self.is_map_type(target_type):
                direct_get, generic_get = self.map_get_fn_for_type(target_type, required=True)
                if direct_get is not None:
                    return f"{direct_get}({target_code}, {self.c_string(node.field)})", self.map_value_type(target_type)
                if generic_get is not None:
                    return f"{generic_get}({target_code}, {self.c_string(node.field)})", self.map_value_type(target_type)
            return "0", TYPE_INT

        if isinstance(node, ModuleCallNode):
            if node.module_name in ("http", "std.http"):
                if node.func_name == "response_status":
                    response_code, _ = self.expr_with_type(node.args[0]) if node.args else ("\"\"", TYPE_TEXT)
                    return f"nl_http_response_status({response_code})", TYPE_INT
                if node.func_name == "response_text":
                    response_code, _ = self.expr_with_type(node.args[0]) if node.args else ("\"\"", TYPE_TEXT)
                    return f"nl_http_response_text({response_code})", TYPE_TEXT
                if node.func_name == "response_json":
                    response_code, _ = self.expr_with_type(node.args[0]) if node.args else ("\"\"", TYPE_TEXT)
                    return f"nl_http_response_json({response_code})", TYPE_MAP_TEXT
                if node.func_name == "response_header":
                    response_code, _ = self.expr_with_type(node.args[0]) if node.args else ("\"\"", TYPE_TEXT)
                    key_code, _ = self.expr_with_type(node.args[1]) if len(node.args) > 1 else ("\"\"", TYPE_TEXT)
                    return f"nl_http_response_header({response_code}, {key_code})", TYPE_TEXT
            if node.module_name in ("web", "std.web"):
                if node.func_name == "path_match":
                    pattern_code, _ = self.expr_with_type(node.args[0]) if node.args else ("\"\"", TYPE_TEXT)
                    path_code, _ = self.expr_with_type(node.args[1]) if len(node.args) > 1 else ("\"\"", TYPE_TEXT)
                    return f"nl_web_path_match({pattern_code}, {path_code})", TYPE_BOOL
                if node.func_name == "path_params":
                    pattern_code, _ = self.expr_with_type(node.args[0]) if node.args else ("\"\"", TYPE_TEXT)
                    path_code, _ = self.expr_with_type(node.args[1]) if len(node.args) > 1 else ("\"\"", TYPE_TEXT)
                    return f"nl_web_path_params({pattern_code}, {path_code})", TYPE_MAP_TEXT
                if node.func_name == "route_match":
                    spec_code, _ = self.expr_with_type(node.args[0]) if node.args else ("\"\"", TYPE_TEXT)
                    method_code, _ = self.expr_with_type(node.args[1]) if len(node.args) > 1 else ("\"\"", TYPE_TEXT)
                    path_code, _ = self.expr_with_type(node.args[2]) if len(node.args) > 2 else ("\"\"", TYPE_TEXT)
                    return f"nl_web_route_match({spec_code}, {method_code}, {path_code})", TYPE_BOOL
                if node.func_name == "dispatch":
                    routes_code, _ = self.expr_with_type(node.args[0]) if node.args else ("nl_map_text_new()", TYPE_MAP_TEXT)
                    method_code, _ = self.expr_with_type(node.args[1]) if len(node.args) > 1 else ("\"\"", TYPE_TEXT)
                    path_code, _ = self.expr_with_type(node.args[2]) if len(node.args) > 2 else ("\"\"", TYPE_TEXT)
                    return f"nl_web_dispatch({routes_code}, {method_code}, {path_code})", TYPE_TEXT
                if node.func_name == "dispatch_params":
                    routes_code, _ = self.expr_with_type(node.args[0]) if node.args else ("nl_map_text_new()", TYPE_MAP_TEXT)
                    method_code, _ = self.expr_with_type(node.args[1]) if len(node.args) > 1 else ("\"\"", TYPE_TEXT)
                    path_code, _ = self.expr_with_type(node.args[2]) if len(node.args) > 2 else ("\"\"", TYPE_TEXT)
                    return f"nl_web_dispatch_params({routes_code}, {method_code}, {path_code})", TYPE_MAP_TEXT
                if node.func_name == "request_context":
                    method_code, _ = self.expr_with_type(node.args[0]) if node.args else ("\"\"", TYPE_TEXT)
                    path_code, _ = self.expr_with_type(node.args[1]) if len(node.args) > 1 else ("\"\"", TYPE_TEXT)
                    query_code, _ = self.expr_with_type(node.args[2]) if len(node.args) > 2 else ("nl_map_text_new()", TYPE_MAP_TEXT)
                    headers_code, _ = self.expr_with_type(node.args[3]) if len(node.args) > 3 else ("nl_map_text_new()", TYPE_MAP_TEXT)
                    body_code, _ = self.expr_with_type(node.args[4]) if len(node.args) > 4 else ("\"\"", TYPE_TEXT)
                    return f"nl_web_request_context({method_code}, {path_code}, {query_code}, {headers_code}, {body_code})", TYPE_MAP_TEXT
                if node.func_name == "request_method":
                    ctx_code, _ = self.expr_with_type(node.args[0]) if node.args else ("nl_map_text_new()", TYPE_MAP_TEXT)
                    return f"nl_web_request_method({ctx_code})", TYPE_TEXT
                if node.func_name == "request_path":
                    ctx_code, _ = self.expr_with_type(node.args[0]) if node.args else ("nl_map_text_new()", TYPE_MAP_TEXT)
                    return f"nl_web_request_path({ctx_code})", TYPE_TEXT
                if node.func_name == "request_query":
                    ctx_code, _ = self.expr_with_type(node.args[0]) if node.args else ("nl_map_text_new()", TYPE_MAP_TEXT)
                    return f"nl_web_request_query({ctx_code})", TYPE_MAP_TEXT
                if node.func_name == "request_headers":
                    ctx_code, _ = self.expr_with_type(node.args[0]) if node.args else ("nl_map_text_new()", TYPE_MAP_TEXT)
                    return f"nl_web_request_headers({ctx_code})", TYPE_MAP_TEXT
                if node.func_name == "request_body":
                    ctx_code, _ = self.expr_with_type(node.args[0]) if node.args else ("nl_map_text_new()", TYPE_MAP_TEXT)
                    return f"nl_web_request_body({ctx_code})", TYPE_TEXT
                if node.func_name == "request_params":
                    ctx_code, _ = self.expr_with_type(node.args[0]) if node.args else ("nl_map_text_new()", TYPE_MAP_TEXT)
                    return f"nl_web_request_params({ctx_code})", TYPE_MAP_TEXT
                if node.func_name == "request_param":
                    ctx_code, _ = self.expr_with_type(node.args[0]) if node.args else ("nl_map_text_new()", TYPE_MAP_TEXT)
                    key_code, _ = self.expr_with_type(node.args[1]) if len(node.args) > 1 else ("\"\"", TYPE_TEXT)
                    return f"nl_web_request_param({ctx_code}, {key_code})", TYPE_TEXT
                if node.func_name == "request_param_int":
                    ctx_code, _ = self.expr_with_type(node.args[0]) if node.args else ("nl_map_text_new()", TYPE_MAP_TEXT)
                    key_code, _ = self.expr_with_type(node.args[1]) if len(node.args) > 1 else ("\"\"", TYPE_TEXT)
                    return f"nl_web_request_param_int({ctx_code}, {key_code})", TYPE_INT
                if node.func_name == "request_query_param":
                    ctx_code, _ = self.expr_with_type(node.args[0]) if node.args else ("nl_map_text_new()", TYPE_MAP_TEXT)
                    key_code, _ = self.expr_with_type(node.args[1]) if len(node.args) > 1 else ("\"\"", TYPE_TEXT)
                    return f"nl_web_request_query_param({ctx_code}, {key_code})", TYPE_TEXT
                if node.func_name == "request_query_required":
                    ctx_code, _ = self.expr_with_type(node.args[0]) if node.args else ("nl_map_text_new()", TYPE_MAP_TEXT)
                    key_code, _ = self.expr_with_type(node.args[1]) if len(node.args) > 1 else ("\"\"", TYPE_TEXT)
                    return f"nl_web_request_query_required({ctx_code}, {key_code})", TYPE_TEXT
                if node.func_name == "request_query_int":
                    ctx_code, _ = self.expr_with_type(node.args[0]) if node.args else ("nl_map_text_new()", TYPE_MAP_TEXT)
                    key_code, _ = self.expr_with_type(node.args[1]) if len(node.args) > 1 else ("\"\"", TYPE_TEXT)
                    return f"nl_web_request_query_int({ctx_code}, {key_code})", TYPE_INT
                if node.func_name == "request_header":
                    ctx_code, _ = self.expr_with_type(node.args[0]) if node.args else ("nl_map_text_new()", TYPE_MAP_TEXT)
                    key_code, _ = self.expr_with_type(node.args[1]) if len(node.args) > 1 else ("\"\"", TYPE_TEXT)
                    return f"nl_web_request_header({ctx_code}, {key_code})", TYPE_TEXT
                if node.func_name == "request_json":
                    ctx_code, _ = self.expr_with_type(node.args[0]) if node.args else ("nl_map_text_new()", TYPE_MAP_TEXT)
                    return f"nl_web_request_json({ctx_code})", TYPE_MAP_TEXT
                if node.func_name == "request_json_field":
                    ctx_code, _ = self.expr_with_type(node.args[0]) if node.args else ("nl_map_text_new()", TYPE_MAP_TEXT)
                    key_code, _ = self.expr_with_type(node.args[1]) if len(node.args) > 1 else ("\"\"", TYPE_TEXT)
                    return f"nl_web_request_json_field({ctx_code}, {key_code})", TYPE_TEXT
                if node.func_name == "request_json_field_or":
                    ctx_code, _ = self.expr_with_type(node.args[0]) if node.args else ("nl_map_text_new()", TYPE_MAP_TEXT)
                    key_code, _ = self.expr_with_type(node.args[1]) if len(node.args) > 1 else ("\"\"", TYPE_TEXT)
                    fallback_code, _ = self.expr_with_type(node.args[2]) if len(node.args) > 2 else ("\"\"", TYPE_TEXT)
                    return f"nl_web_request_json_field_or({ctx_code}, {key_code}, {fallback_code})", TYPE_TEXT
                if node.func_name == "request_json_field_int":
                    ctx_code, _ = self.expr_with_type(node.args[0]) if node.args else ("nl_map_text_new()", TYPE_MAP_TEXT)
                    key_code, _ = self.expr_with_type(node.args[1]) if len(node.args) > 1 else ("\"\"", TYPE_TEXT)
                    return f"nl_web_request_json_field_int({ctx_code}, {key_code})", TYPE_INT
                if node.func_name == "request_json_field_bool":
                    ctx_code, _ = self.expr_with_type(node.args[0]) if node.args else ("nl_map_text_new()", TYPE_MAP_TEXT)
                    key_code, _ = self.expr_with_type(node.args[1]) if len(node.args) > 1 else ("\"\"", TYPE_TEXT)
                    return f"nl_web_request_json_field_bool({ctx_code}, {key_code})", TYPE_BOOL
                if node.func_name == "response_builder":
                    status_code, _ = self.expr_with_type(node.args[0]) if node.args else ("0", TYPE_INT)
                    headers_code, _ = self.expr_with_type(node.args[1]) if len(node.args) > 1 else ("nl_map_text_new()", TYPE_MAP_TEXT)
                    body_code, _ = self.expr_with_type(node.args[2]) if len(node.args) > 2 else ("\"\"", TYPE_TEXT)
                    return f"nl_web_response_builder({status_code}, {headers_code}, {body_code})", TYPE_MAP_TEXT
                if node.func_name == "response_status":
                    response_code, _ = self.expr_with_type(node.args[0]) if node.args else ("nl_map_text_new()", TYPE_MAP_TEXT)
                    return f"nl_web_response_status({response_code})", TYPE_INT
                if node.func_name == "response_headers":
                    response_code, _ = self.expr_with_type(node.args[0]) if node.args else ("nl_map_text_new()", TYPE_MAP_TEXT)
                    return f"nl_web_response_headers({response_code})", TYPE_MAP_TEXT
                if node.func_name == "response_body":
                    response_code, _ = self.expr_with_type(node.args[0]) if node.args else ("nl_map_text_new()", TYPE_MAP_TEXT)
                    return f"nl_web_response_body({response_code})", TYPE_TEXT
                if node.func_name == "response_text":
                    response_code, _ = self.expr_with_type(node.args[0]) if node.args else ("nl_map_text_new()", TYPE_MAP_TEXT)
                    return f"nl_web_response_text({response_code})", TYPE_TEXT
                if node.func_name == "response_json":
                    response_code, _ = self.expr_with_type(node.args[0]) if node.args else ("nl_map_text_new()", TYPE_MAP_TEXT)
                    return f"nl_web_response_json({response_code})", TYPE_MAP_TEXT
                if node.func_name == "response_header":
                    response_code, _ = self.expr_with_type(node.args[0]) if node.args else ("nl_map_text_new()", TYPE_MAP_TEXT)
                    key_code, _ = self.expr_with_type(node.args[1]) if len(node.args) > 1 else ("\"\"", TYPE_TEXT)
                    return f"nl_web_response_header({response_code}, {key_code})", TYPE_TEXT
                if node.func_name == "response_error":
                    status_code, _ = self.expr_with_type(node.args[0]) if node.args else ("0", TYPE_INT)
                    message_code, _ = self.expr_with_type(node.args[1]) if len(node.args) > 1 else ("\"\"", TYPE_TEXT)
                    return f"nl_web_response_error({status_code}, {message_code})", TYPE_MAP_TEXT
                if node.func_name == "response_file":
                    path_code, _ = self.expr_with_type(node.args[0]) if node.args else ("\"\"", TYPE_TEXT)
                    content_type_code, _ = self.expr_with_type(node.args[1]) if len(node.args) > 1 else ("\"application/octet-stream\"", TYPE_TEXT)
                    return f"nl_web_response_file({path_code}, {content_type_code})", TYPE_MAP_TEXT
                if node.func_name == "openapi_json":
                    title_code, _ = self.expr_with_type(node.args[0]) if node.args else ("\"Norscode API\"", TYPE_TEXT)
                    version_code, _ = self.expr_with_type(node.args[1]) if len(node.args) > 1 else ("\"1.0.0\"", TYPE_TEXT)
                    return f"nl_web_openapi_json({title_code}, {version_code})", TYPE_TEXT
                if node.func_name == "docs_html":
                    title_code, _ = self.expr_with_type(node.args[0]) if node.args else ("\"Norscode API\"", TYPE_TEXT)
                    version_code, _ = self.expr_with_type(node.args[1]) if len(node.args) > 1 else ("\"1.0.0\"", TYPE_TEXT)
                    return f"nl_web_docs_html({title_code}, {version_code})", TYPE_TEXT
                if node.func_name == "request_middleware":
                    return "nl_strdup(\"request_middleware\")", TYPE_TEXT
                if node.func_name == "response_middleware":
                    return "nl_strdup(\"response_middleware\")", TYPE_TEXT
                if node.func_name == "error_middleware":
                    return "nl_strdup(\"error_middleware\")", TYPE_TEXT
                if node.func_name == "startup_hook":
                    return "nl_strdup(\"startup_hook\")", TYPE_TEXT
                if node.func_name == "shutdown_hook":
                    return "nl_strdup(\"shutdown_hook\")", TYPE_TEXT
                if node.func_name == "request_id":
                    ctx_code, _ = self.expr_with_type(node.args[0]) if node.args else ("nl_map_text_new()", TYPE_MAP_TEXT)
                    return f"nl_web_request_id({ctx_code})", TYPE_TEXT
                if node.func_name == "startup":
                    return "nl_web_run_startup_hooks()", TYPE_INT
                if node.func_name == "shutdown":
                    return "nl_web_run_shutdown_hooks()", TYPE_INT
                if node.func_name == "route":
                    spec_code, _ = self.expr_with_type(node.args[0]) if node.args else ("\"\"", TYPE_TEXT)
                    return f"nl_web_route({spec_code})", TYPE_TEXT
                if node.func_name in {"router", "subrouter"}:
                    prefix_code, _ = self.expr_with_type(node.args[0]) if node.args else ("\"\"", TYPE_TEXT)
                    return prefix_code, TYPE_TEXT
                if node.func_name == "dependency":
                    name_code, _ = self.expr_with_type(node.args[0]) if node.args else ("\"\"", TYPE_TEXT)
                    return name_code, TYPE_TEXT
                if node.func_name == "use_dependency":
                    name_code, _ = self.expr_with_type(node.args[0]) if node.args else ("\"\"", TYPE_TEXT)
                    return name_code, TYPE_TEXT
                if node.func_name == "request_dependency":
                    ctx_code, _ = self.expr_with_type(node.args[0]) if node.args else ("nl_map_text_new()", TYPE_MAP_TEXT)
                    name_code, _ = self.expr_with_type(node.args[1]) if len(node.args) > 1 else ("\"\"", TYPE_TEXT)
                    return f"nl_web_request_dependency({ctx_code}, {name_code})", TYPE_MAP_TEXT
                if node.func_name == "handle_request":
                    ctx_code, _ = self.expr_with_type(node.args[0]) if node.args else ("nl_map_text_new()", TYPE_MAP_TEXT)
                    return f"nl_web_handle_request({ctx_code})", TYPE_MAP_TEXT
            if node.module_name in ("vent", "std.vent"):
                if node.func_name == "timeout":
                    value_code, _ = self.expr_with_type(node.args[0]) if node.args else ("0", TYPE_INT)
                    timeout_code, _ = self.expr_with_type(node.args[1]) if len(node.args) > 1 else ("0", TYPE_INT)
                    return f"nl_async_timeout({value_code}, {timeout_code})", TYPE_INT
                if node.func_name == "kanseller":
                    value_code, _ = self.expr_with_type(node.args[0]) if node.args else ("0", TYPE_INT)
                    return f"nl_async_cancel({value_code})", TYPE_INT
                if node.func_name == "er_kansellert":
                    value_code, _ = self.expr_with_type(node.args[0]) if node.args else ("0", TYPE_INT)
                    return f"nl_async_is_cancelled({value_code})", TYPE_BOOL
                if node.func_name == "er_timeoutet":
                    value_code, _ = self.expr_with_type(node.args[0]) if node.args else ("0", TYPE_INT)
                    return f"nl_async_is_timed_out({value_code})", TYPE_BOOL
                if node.func_name == "sov":
                    value_code, _ = self.expr_with_type(node.args[0]) if node.args else ("0", TYPE_INT)
                    return f"(nl_sleep_ms({value_code}), 0)", TYPE_INT
            symbol, _full_name = self.resolve_symbol(node.func_name, module_name=node.module_name)
            expected_args = list(getattr(symbol, "params", []) if symbol is not None else [])
            args = [
                self.expr_with_type(arg, expected_args[i] if i < len(expected_args) else None)[0]
                for i, arg in enumerate(node.args)
            ]
            c_name = self.resolve_c_function_name(node.func_name, module_name=node.module_name)
            return_type = symbol.return_type if symbol else TYPE_INT
            return f"{c_name}({', '.join(args)})", return_type

        if isinstance(node, CallNode):
            lambda_type = self.var_types.get(node.name)
            if isinstance(lambda_type, LambdaCType):
                self.emit_lambda_prototype(lambda_type)
                args_with_types = [
                    self.expr_with_type(arg, lambda_type.param_types[i] if i < len(lambda_type.param_types) else None)
                    for i, arg in enumerate(node.args)
                ]
                args = [code for code, _t in args_with_types]
                capture_args = [f"{self.resolve_name(node.name)}.{capture}" for capture in lambda_type.capture_names]
                return f"{lambda_type.helper_name}({', '.join(capture_args + args)})", lambda_type.return_type

            symbol, _full_name = self.resolve_symbol(node.name)
            expected_args = list(getattr(symbol, "params", []) if symbol is not None else [])
            args_with_types = [
                self.expr_with_type(arg, expected_args[i] if i < len(expected_args) else None)
                for i, arg in enumerate(node.args)
            ]
            args = [code for code, _t in args_with_types]

            if node.name == "assert":
                return f"nl_assert({args[0]})", TYPE_INT

            if node.name == "assert_eq":
                arg_type = args_with_types[0][1]
                if arg_type == TYPE_TEXT:
                    return f"nl_assert_eq_text({args[0]}, {args[1]})", TYPE_INT
                return f"nl_assert_eq_int({args[0]}, {args[1]})", TYPE_INT

            if node.name == "assert_ne":
                arg_type = args_with_types[0][1]
                if arg_type == TYPE_TEXT:
                    return f"nl_assert_ne_text({args[0]}, {args[1]})", TYPE_INT
                return f"nl_assert_ne_int({args[0]}, {args[1]})", TYPE_INT

            if node.name == "assert_starter_med":
                return f"nl_assert_starter_med({args[0]}, {args[1]})", TYPE_INT

            if node.name == "assert_slutter_med":
                return f"nl_assert_slutter_med({args[0]}, {args[1]})", TYPE_INT

            if node.name == "assert_inneholder":
                return f"nl_assert_inneholder({args[0]}, {args[1]})", TYPE_INT

            if node.name == "tekst_fra_heltall":
                return f"nl_int_to_text({args[0]})", TYPE_TEXT

            if node.name == "tekst_fra_bool":
                return f"nl_bool_to_text({args[0]})", TYPE_TEXT

            if node.name == "heltall_fra_tekst":
                return f"nl_text_to_int({args[0]})", TYPE_INT

            if node.name == "tekst_starter_med":
                return f"nl_text_starter_med({args[0]}, {args[1]})", TYPE_BOOL

            if node.name == "tekst_slutter_med":
                return f"nl_text_slutter_med({args[0]}, {args[1]})", TYPE_BOOL

            if node.name == "tekst_inneholder":
                return f"nl_text_inneholder({args[0]}, {args[1]})", TYPE_BOOL

            if node.name == "tekst_trim":
                return f"nl_text_trim({args[0]})", TYPE_TEXT

            if node.name == "sti_join":
                return f"nl_path_join({args[0]}, {args[1]})", TYPE_TEXT

            if node.name == "sti_basename":
                return f"nl_path_basename({args[0]})", TYPE_TEXT

            if node.name == "sti_dirname":
                return f"nl_path_dirname({args[0]})", TYPE_TEXT

            if node.name == "sti_exists":
                return f"nl_path_exists({args[0]})", TYPE_BOOL

            if node.name == "sti_stem":
                return f"nl_path_stem({args[0]})", TYPE_TEXT

            if node.name == "miljo_hent":
                return f"nl_env_get({args[0]})", TYPE_TEXT

            if node.name == "miljo_finnes":
                return f"nl_env_exists({args[0]})", TYPE_BOOL

            if node.name == "miljo_sett":
                return f"nl_env_set({args[0]}, {args[1]})", TYPE_TEXT

            if node.name == "fil_les":
                return f"nl_file_read_text({args[0]})", TYPE_TEXT

            if node.name == "fil_skriv":
                return f"nl_file_write_text({args[0]}, {args[1]}, \"w\")", TYPE_TEXT

            if node.name == "fil_append":
                return f"nl_file_write_text({args[0]}, {args[1]}, \"a\")", TYPE_TEXT

            if node.name == "fil_finnes":
                return f"nl_file_exists({args[0]})", TYPE_BOOL

            if node.name == "del_ord":
                return f"nl_split_words({args[0]})", TYPE_LIST_TEXT

            if node.name == "tokeniser_enkel":
                return f"nl_tokenize_simple({args[0]})", TYPE_LIST_TEXT

            if node.name == "tokeniser_uttrykk":
                return f"nl_tokenize_expression({args[0]})", TYPE_LIST_TEXT

            if node.name == "lengde":
                arg_type = args_with_types[0][1]
                if arg_type == TYPE_LIST_TEXT:
                    return f"nl_list_text_len({args[0]})", TYPE_INT
                if self.is_map_type(arg_type):
                    if arg_type == TYPE_MAP_INT:
                        return f"nl_map_int_len({args[0]})", TYPE_INT
                    if arg_type == TYPE_MAP_TEXT:
                        return f"nl_map_text_len({args[0]})", TYPE_INT
                    if arg_type == TYPE_MAP_BOOL:
                        return f"nl_map_bool_len({args[0]})", TYPE_INT
                    return f"nl_map_any_len({args[0]})", TYPE_INT
                return f"nl_list_int_len({args[0]})", TYPE_INT

            if node.name == "legg_til":
                list_type = args_with_types[0][1]
                if list_type == TYPE_LIST_TEXT:
                    return f"nl_list_text_push({args[0]}, {args[1]})", TYPE_INT
                return f"nl_list_int_push({args[0]}, {args[1]})", TYPE_INT

            if node.name == "pop_siste":
                list_type = args_with_types[0][1]
                if list_type == TYPE_LIST_TEXT:
                    return f"nl_list_text_pop({args[0]})", TYPE_TEXT
                return f"nl_list_int_pop({args[0]})", TYPE_INT

            if node.name == "fjern_indeks":
                list_type = args_with_types[0][1]
                if list_type == TYPE_LIST_TEXT:
                    return f"nl_list_text_remove({args[0]}, {args[1]})", TYPE_INT
                return f"nl_list_int_remove({args[0]}, {args[1]})", TYPE_INT

            if node.name == "sett_inn":
                list_type = args_with_types[0][1]
                if list_type == TYPE_LIST_TEXT:
                    return f"nl_list_text_set({args[0]}, {args[1]}, {args[2]})", TYPE_INT
                return f"nl_list_int_set({args[0]}, {args[1]}, {args[2]})", TYPE_INT

            if node.name == "har_nokkel":
                map_type = args_with_types[0][1]
                if map_type == TYPE_MAP_INT:
                    return f"(nl_map_int_find({args[0]}, {args[1]}) >= 0)", TYPE_BOOL
                if map_type == TYPE_MAP_TEXT:
                    return f"(nl_map_text_find({args[0]}, {args[1]}) >= 0)", TYPE_BOOL
                if map_type == TYPE_MAP_BOOL:
                    return f"(nl_map_bool_find({args[0]}, {args[1]}) >= 0)", TYPE_BOOL
                if self.is_map_type(map_type):
                    return f"(nl_map_any_find({args[0]}, {args[1]}) >= 0)", TYPE_BOOL
                return "0", TYPE_BOOL

            if node.name == "fjern_nokkel":
                map_type = args_with_types[0][1]
                if map_type == TYPE_MAP_INT:
                    return f"nl_map_int_remove({args[0]}, {args[1]})", TYPE_INT
                if map_type == TYPE_MAP_TEXT:
                    return f"nl_map_text_remove({args[0]}, {args[1]})", TYPE_INT
                if map_type == TYPE_MAP_BOOL:
                    return f"nl_map_bool_remove({args[0]}, {args[1]})", TYPE_INT
                if self.is_map_type(map_type):
                    return f"nl_map_any_remove({args[0]}, {args[1]})", TYPE_INT
                return "0", TYPE_INT

            c_name = self.resolve_c_function_name(node.name)
            return_type = symbol.return_type if symbol else TYPE_INT
            return f"{c_name}({', '.join(args)})", return_type

        if isinstance(node, IndexNode):
            target_expr = getattr(node, "list_expr", getattr(node, "target", None))
            list_code, list_type = self.expr_with_type(target_expr)
            idx_code, _ = self.expr_with_type(getattr(node, "index_expr", None))
            if list_type == TYPE_LIST_TEXT:
                return f"{list_code}->data[{idx_code}]", TYPE_TEXT
            if self.is_map_type(list_type):
                direct_get, generic_get = self.map_get_fn_for_type(list_type)
                if direct_get is not None:
                    return f"{direct_get}({list_code}, {idx_code})", self.map_value_type(list_type)
                if generic_get is not None:
                    return f"{generic_get}({list_code}, {idx_code})", self.map_value_type(list_type)
            return f"{list_code}->data[{idx_code}]", TYPE_INT

        if isinstance(node, SliceNode):
            target_code, target_type = self.expr_with_type(node.target)
            if node.start_expr is None:
                start_code, has_start = "0", "0"
            else:
                start_code, _ = self.expr_with_type(node.start_expr)
                has_start = "1"
            if node.end_expr is None:
                end_code, has_end = "0", "0"
            else:
                end_code, _ = self.expr_with_type(node.end_expr)
                has_end = "1"
            if target_type == TYPE_TEXT:
                return f"nl_text_slice({target_code}, {start_code}, {has_start}, {end_code}, {has_end})", TYPE_TEXT
            if target_type == TYPE_LIST_INT:
                return f"nl_list_int_slice({target_code}, {start_code}, {has_start}, {end_code}, {has_end})", TYPE_LIST_INT
            if target_type == TYPE_LIST_TEXT:
                return f"nl_list_text_slice({target_code}, {start_code}, {has_start}, {end_code}, {has_end})", TYPE_LIST_TEXT
            return "0", TYPE_TEXT

        if isinstance(node, AwaitNode):
            expr_code, expr_type = self.expr_with_type(node.expr, expected_type)
            return f"nl_async_await({expr_code})", expr_type

        return "0", TYPE_INT
