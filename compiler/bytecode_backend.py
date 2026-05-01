from __future__ import annotations

import argparse
import json
import os
import re
import time
import urllib.error
import urllib.parse
import urllib.request
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from .ast_nodes import (
    TYPE_BOOL,
    TYPE_INT,
    TYPE_LIST_INT,
    TYPE_LIST_TEXT,
    TYPE_MAP_BOOL,
    TYPE_MAP_INT,
    TYPE_MAP_TEXT,
    TYPE_TEXT,
    BinOpNode,
    BlockNode,
    BoolNode,
    BreakNode,
    CallNode,
    ContinueNode,
    ExprStmtNode,
    ForNode,
    ForEachNode,
    FunctionNode,
    IfNode,
    IfExprNode,
    MatchNode,
    AwaitNode,
    ImportNode,
    IndexNode,
    IndexSetNode,
    FieldAccessNode,
    ListLiteralNode,
    ListComprehensionNode,
    LambdaNode,
    MapLiteralNode,
    SliceNode,
    StructLiteralNode,
    ModuleCallNode,
    NumberNode,
    PrintNode,
    ProgramNode,
    ReturnNode,
    StringNode,
    UnaryOpNode,
    VarAccessNode,
    VarDeclareNode,
    VarSetNode,
    WhileNode,
    ThrowNode,
    TryCatchNode,
)
from .ast_bridge import load_source_as_program, read_ast


class BytecodeCompileError(RuntimeError):
    pass


class BytecodeRuntimeError(RuntimeError):
    pass


class BytecodeThrow(RuntimeError):
    def __init__(self, value: Any):
        self.value = value
        self.call_stack: list[str] = []
        super().__init__()

    def __str__(self) -> str:
        message = f"kast: {self.value!r}"
        if self.call_stack:
            stack_text = " -> ".join(reversed(self.call_stack))
            return f"{message}\nKallstakk: {stack_text}"
        return message


@dataclass
class AsyncValue:
    value: Any
    cancelled: bool = False
    timeout_deadline: float | None = None


@dataclass
class BytecodeLambdaValue:
    function_name: str
    capture_names: list[str]
    capture_values: list[Any]


def _http_request(method: str, url: str, headers: Any, query: Any, body: Any, timeout_ms: Any):
    query_items = {}
    if isinstance(query, dict):
        query_items = {str(k): str(v) for k, v in query.items()}
    elif query is not None:
        query_items = {str(query): ""}
    query_string = urllib.parse.urlencode(query_items)
    if query_string:
        parsed = urllib.parse.urlsplit(url)
        url = urllib.parse.urlunsplit((
            parsed.scheme,
            parsed.netloc,
            parsed.path,
            query_string if not parsed.query else parsed.query + "&" + query_string,
            parsed.fragment,
        ))

    req_headers = {}
    if isinstance(headers, dict):
        req_headers = {str(k): str(v) for k, v in headers.items()}

    data = None
    if body is not None and method.upper() in {"POST", "PUT", "PATCH"}:
        data = str(body).encode("utf-8")

    request = urllib.request.Request(url, data=data, method=method.upper())
    for key, value in req_headers.items():
        request.add_header(key, value)
    timeout = None if timeout_ms is None else max(0, int(timeout_ms)) / 1000.0
    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:
            return response.read().decode("utf-8")
    except urllib.error.HTTPError as exc:
        payload = exc.read().decode("utf-8", errors="replace") if exc.fp else ""
        raise BytecodeThrow(f"HTTPFeil: {exc.code} {exc.reason}: {payload}".strip())
    except Exception as exc:
        raise BytecodeThrow(f"HTTPFeil: {exc}")


def _http_request_full(method: str, url: str, headers: Any, query: Any, body: Any, timeout_ms: Any):
    query_items = {}
    if isinstance(query, dict):
        query_items = {str(k): str(v) for k, v in query.items()}
    elif query is not None:
        query_items = {str(query): ""}
    query_string = urllib.parse.urlencode(query_items)
    if query_string:
        parsed = urllib.parse.urlsplit(url)
        url = urllib.parse.urlunsplit((
            parsed.scheme,
            parsed.netloc,
            parsed.path,
            query_string if not parsed.query else parsed.query + "&" + query_string,
            parsed.fragment,
        ))

    req_headers = {}
    if isinstance(headers, dict):
        req_headers = {str(k): str(v) for k, v in headers.items()}

    data = None
    if body is not None and method.upper() in {"POST", "PUT", "PATCH"}:
        data = str(body).encode("utf-8")

    request = urllib.request.Request(url, data=data, method=method.upper())
    for key, value in req_headers.items():
        request.add_header(key, value)
    timeout = None if timeout_ms is None else max(0, int(timeout_ms)) / 1000.0
    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:
            response_body = response.read().decode("utf-8")
            payload = {
                "status": int(getattr(response, "status", 200)),
                "body": response_body,
                "headers": {str(key): str(value) for key, value in response.headers.items()},
            }
            return json.dumps(payload, ensure_ascii=False)
    except urllib.error.HTTPError as exc:
        payload = exc.read().decode("utf-8", errors="replace") if exc.fp else ""
        raise BytecodeThrow(f"HTTPFeil: {exc.code} {exc.reason}: {payload}".strip())
    except Exception as exc:
        raise BytecodeThrow(f"HTTPFeil: {exc}")


def _normalize_web_path(value: Any) -> list[str]:
    text = str(value).strip()
    if text in ("", "/"):
        return []
    return [segment for segment in text.strip("/").split("/") if segment != ""]


def _match_web_path(pattern: Any, path: Any) -> dict[str, str] | None:
    pattern_parts = _normalize_web_path(pattern)
    path_parts = _normalize_web_path(path)
    if len(pattern_parts) != len(path_parts):
        return None

    params: dict[str, str] = {}
    for pattern_part, path_part in zip(pattern_parts, path_parts):
        if pattern_part.startswith("{") and pattern_part.endswith("}"):
            token = pattern_part[1:-1].strip()
            if not token:
                return None
            param_name, _, param_type = token.partition(":")
            param_name = param_name.strip()
            param_type = param_type.strip()
            if not param_name:
                return None
            if param_type == "int" and not re.fullmatch(r"-?\d+", path_part):
                return None
            params[param_name] = path_part
            continue
        if pattern_part != path_part:
            return None
    return params


def _normalize_web_method(value: Any) -> str:
    return str(value).strip().upper()


def _split_web_route_spec(spec: Any) -> tuple[str, str]:
    text = str(spec).strip()
    if not text:
        return "", ""
    parts = text.split(None, 1)
    if len(parts) == 1:
        return "", parts[0]
    return parts[0].upper(), parts[1]


def _combine_web_route_prefix(prefix: Any, spec: Any) -> str:
    route_prefix = str(prefix).strip()
    route_spec = str(spec).strip()
    if not route_prefix:
        return route_spec
    method, route_path = _split_web_route_spec(route_spec)
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


def _guard_response() -> dict[str, Any]:
    return _make_web_response(403, {"content-type": "application/json"}, json.dumps({"error": "Forbudt"}, ensure_ascii=False))


def _match_web_route_spec(spec: Any, method: Any, path: Any) -> dict[str, str] | None:
    route_method, route_path = _split_web_route_spec(spec)
    request_method = _normalize_web_method(method)
    if route_method and route_method != request_method:
        return None
    return _match_web_path(route_path, path)


def _encode_json_text_map(mapping: Any) -> str:
    if not isinstance(mapping, dict):
        return json.dumps({}, ensure_ascii=False)
    return json.dumps({str(key): str(value) for key, value in mapping.items()}, ensure_ascii=False)


def _decode_json_text_map(value: Any) -> dict[str, str]:
    if isinstance(value, dict):
        return {str(key): str(item) for key, item in value.items()}
    if value in (None, ""):
        return {}
    try:
        parsed = json.loads(str(value))
    except Exception:
        return {}
    if isinstance(parsed, dict):
        return {str(key): str(item) for key, item in parsed.items()}
    return {}


def _make_web_request_context(method: Any, path: Any, query: Any, headers: Any, body: Any) -> dict[str, Any]:
    _make_web_request_context.counter += 1
    return {
        "metode": _normalize_web_method(method),
        "sti": str(path),
        "query": _encode_json_text_map(query),
        "headers": _encode_json_text_map(headers),
        "body": "" if body is None else str(body),
        "params": _encode_json_text_map({}),
        "deps": _encode_json_text_map({}),
        "request_id": f"req-{_make_web_request_context.counter}",
    }


_make_web_request_context.counter = 0


def _make_web_response(status: Any, headers: Any, body: Any) -> dict[str, Any]:
    try:
        status_text = str(int(status))
    except Exception:
        status_text = "0"
    return {
        "status": status_text,
        "headers": _encode_json_text_map(headers),
        "body": "" if body is None else str(body),
    }


def _make_route_context(ctx: Any, params: dict[str, str]) -> dict[str, Any]:
    if not isinstance(ctx, dict):
        ctx = _make_web_request_context("", "", {}, {}, "")
    new_ctx = dict(ctx)
    new_ctx["params"] = _encode_json_text_map(params)
    if "deps" not in new_ctx:
        new_ctx["deps"] = _encode_json_text_map({})
    return new_ctx


def _validation_error(message: str) -> str:
    return f"ValideringsFeil: {message}"


def _request_json_object(ctx: Any) -> dict[str, str]:
    if not isinstance(ctx, dict):
        raise BytecodeThrow(_validation_error("body forventet gyldig JSON-objekt"))
    body_text = str(ctx.get("body", "") or "")
    if not body_text.strip():
        return {}
    try:
        parsed = json.loads(body_text)
    except Exception:
        raise BytecodeThrow(_validation_error("body forventet gyldig JSON-objekt"))
    if not isinstance(parsed, dict):
        raise BytecodeThrow(_validation_error("body forventet JSON-objekt"))
    return {str(key): _json_scalar_to_text(value) for key, value in parsed.items()}


def _parse_required_text(value: Any, field_name: str, source_name: str) -> str:
    text = str(value)
    if text == "":
        raise BytecodeThrow(_validation_error(f"mangler {source_name}-felt '{field_name}'"))
    return text


def _parse_int_value(value: Any, field_name: str, source_name: str) -> int:
    text = str(value).strip()
    if not re.fullmatch(r"-?\d+", text):
        raise BytecodeThrow(_validation_error(f"felt '{field_name}' forventet heltall, fikk '{value}'"))
    return int(text)


def _parse_bool_value(value: Any, field_name: str, source_name: str) -> bool:
    text = str(value).strip().lower()
    if text in {"true", "1", "ja"}:
        return True
    if text in {"false", "0", "nei"}:
        return False
    raise BytecodeThrow(_validation_error(f"felt '{field_name}' forventet bool, fikk '{value}'"))


def _route_is_exact(spec: Any) -> bool:
    return "{" not in str(spec) and "}" not in str(spec)


def _json_scalar_to_text(value: Any) -> str:
    if value is None:
        return "null"
    if isinstance(value, bool):
        return "true" if value else "false"
    return str(value)


@dataclass
class LoopLabels:
    start: str
    end: str
    continue_target: str


class BytecodeCompiler:
    def __init__(self, alias_map: dict[str, str] | None = None):
        self.alias_map = alias_map or {}
        self.label_counter = 0
        self.loop_stack: list[LoopLabels] = []
        self.current_module = "__main__"
        self.name_alias_stack: list[dict[str, str]] = [{}]
        self.lambda_counter = 0
        self.pending_lambda_specs: dict[str, dict[str, Any]] = {}

    def new_label(self, prefix: str) -> str:
        self.label_counter += 1
        return f"{prefix}_{self.label_counter}"

    def push_name_aliases(self, aliases: dict[str, str]):
        merged = dict(self.name_alias_stack[-1])
        merged.update(aliases)
        self.name_alias_stack.append(merged)

    def pop_name_aliases(self):
        if len(self.name_alias_stack) > 1:
            self.name_alias_stack.pop()

    def resolve_name(self, name: str) -> str:
        return self.name_alias_stack[-1].get(name, name)

    def _walk_ast_nodes(self, node: Any):
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

    def _schema_for_type(self, type_name: str) -> dict[str, Any]:
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

    def _sample_for_type(self, type_name: str) -> Any:
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

    def _route_docs_from_function(self, fn: FunctionNode, spec: str) -> dict[str, Any]:
        method, route_path = _split_web_route_spec(spec)
        path_params: list[dict[str, Any]] = []
        query_params: list[dict[str, Any]] = []
        body_fields: list[dict[str, Any]] = []
        seen_path: set[str] = set()
        seen_query: set[str] = set()
        seen_body: set[str] = set()
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
        request_example: dict[str, Any] = {"method": (method or "GET").lower(), "path": path_template}
        if query_params:
            request_example["query"] = {item["name"]: self._sample_for_type(TYPE_INT if item["type"] == "integer" else TYPE_TEXT) for item in query_params}
        if body_fields:
            request_example["body"] = {
                item["name"]: self._sample_for_type(TYPE_INT if item["type"] == "integer" else TYPE_BOOL if item["type"] == "boolean" else TYPE_TEXT)
                for item in body_fields
            }
        response_type = getattr(fn, "return_type", TYPE_TEXT)
        return {
            "function": self.function_key(fn),
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

    def _build_openapi_document(self, title: str, version: str) -> str:
        paths: dict[str, Any] = {}
        for handler in getattr(self, "route_handlers", []):
            path = handler.get("path", "")
            method = str(handler.get("method", "get")).lower()
            path_item = paths.setdefault(path, {})
            parameters: list[dict[str, Any]] = []
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
            operation: dict[str, Any] = {
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
        document = {
            "openapi": "3.0.3",
            "info": {"title": title, "version": version},
            "paths": paths,
        }
        return json.dumps(document, ensure_ascii=False, indent=2)

    def _build_docs_html(self, title: str, version: str) -> str:
        spec = self._build_openapi_document(title, version)
        rows = []
        for handler in getattr(self, "route_handlers", []):
            rows.append(
                f"<li><code>{handler.get('method', 'get').upper()} {handler.get('path', '')}</code>"
                f" - {handler.get('summary', handler.get('function', ''))}</li>"
            )
        route_list = "".join(rows) or "<li>Ingen ruter registrert</li>"
        return (
            "<!doctype html><html><head><meta charset='utf-8'>"
            f"<title>{title}</title>"
            "<style>body{font-family:system-ui,sans-serif;max-width:960px;margin:2rem auto;padding:0 1rem;}"
            "pre{background:#f6f8fa;padding:1rem;overflow:auto;border-radius:8px;}"
            "code{background:#f6f8fa;padding:0.15rem 0.35rem;border-radius:4px;}</style>"
            "</head><body>"
            f"<h1>{title}</h1>"
            f"<p>Versjon: {version}</p>"
            f"<h2>Ruter</h2><ul>{route_list}</ul>"
            "<h2>OpenAPI JSON</h2>"
            f"<pre>{spec}</pre>"
            "</body></html>"
        )

    def _collect_web_annotations(self, program: ProgramNode) -> tuple[list[dict[str, Any]], dict[str, str], list[str], list[str], list[str], list[str], list[str]]:
        handlers: list[dict[str, Any]] = []
        providers: dict[str, str] = {}
        guard_providers: dict[str, str] = {}
        request_middlewares: list[str] = []
        response_middlewares: list[str] = []
        error_middlewares: list[str] = []
        startup_hooks: list[str] = []
        shutdown_hooks: list[str] = []
        for fn in getattr(program, "functions", []):
            statements = list(getattr(getattr(fn, "body", None), "statements", []) or [])
            if not statements:
                continue
            spec = None
            route_prefix = ""
            deps: list[str] = []
            guards: list[str] = []
            provided: list[str] = []
            route_guard = False
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
                        request_middlewares.append(self.function_key(fn))
                        continue
                    if expr.func_name == "response_middleware":
                        response_middlewares.append(self.function_key(fn))
                        continue
                    if expr.func_name == "error_middleware":
                        error_middlewares.append(self.function_key(fn))
                        continue
                    if expr.func_name == "startup_hook":
                        startup_hooks.append(self.function_key(fn))
                        continue
                    if expr.func_name == "shutdown_hook":
                        shutdown_hooks.append(self.function_key(fn))
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
                if expr.func_name == "guard":
                    route_guard = True
                    continue
                if expr.func_name == "use_dependency":
                    deps.append(expr.args[0].value)
                    continue
                if expr.func_name == "use_guard":
                    guards.append(expr.args[0].value)
                    continue
                if expr.func_name == "dependency":
                    provided.append(expr.args[0].value)
                    continue
                break
            if route_guard:
                guard_providers[fn.name] = self.function_key(fn)
            if spec is not None:
                combined_spec = _combine_web_route_prefix(route_prefix, spec)
                route_docs = self._route_docs_from_function(fn, combined_spec)
                route_docs["deps"] = list(deps)
                route_docs["guards"] = list(guards)
                route_docs["spec"] = combined_spec
                handlers.append(route_docs)
            for dep_name in provided:
                providers[dep_name] = self.function_key(fn)
        return handlers, providers, guard_providers, request_middlewares, response_middlewares, error_middlewares, startup_hooks, shutdown_hooks

    def _run_named_hook(self, name: str) -> Any:
        if not name:
            return None
        return self.call_function(name, [])

    def _ensure_startup_hooks(self) -> int:
        if self._startup_ran:
            return 0
        self._startup_ran = True
        count = 0
        for hook in self.startup_hooks:
            self._run_named_hook(hook)
            count += 1
        return count

    def _run_shutdown_hooks(self) -> int:
        if self._shutdown_ran:
            return 0
        self._shutdown_ran = True
        count = 0
        for hook in self.shutdown_hooks:
            self._run_named_hook(hook)
            count += 1
        return count

    def _apply_request_middlewares(self, ctx: Any) -> Any:
        current = ctx
        for middleware in self.request_middlewares:
            result = self.call_function(middleware, [current])
            if isinstance(result, dict):
                current = result
        return current

    def _apply_response_middlewares(self, response: Any) -> Any:
        current = response
        for middleware in self.response_middlewares:
            result = self.call_function(middleware, [current])
            if isinstance(result, dict):
                current = result
        return current

    def _apply_error_middlewares(self, response: Any) -> Any:
        current = response
        for middleware in self.error_middlewares:
            result = self.call_function(middleware, [current])
            if isinstance(result, dict):
                current = result
        return current

    def compile_program(self, program: ProgramNode) -> dict[str, Any]:
        functions = {}
        imports = []
        route_handlers, dependency_providers, guard_providers, request_middlewares, response_middlewares, error_middlewares, startup_hooks, shutdown_hooks = self._collect_web_annotations(program)
        for imp in getattr(program, "imports", []):
            imports.append({"module": imp.module_name, "alias": imp.alias})
        for fn in getattr(program, "functions", []):
            key = self.function_key(fn)
            functions[key] = self.compile_function(fn)
        while self.pending_lambda_specs:
            pending = list(self.pending_lambda_specs.items())
            self.pending_lambda_specs = {}
            for key, spec in pending:
                functions[key] = self.compile_lambda_function(key, spec)
        return {
            "format": "norscode-bytecode-v1",
            "entry": "__main__.start",
            "imports": imports,
            "route_handlers": route_handlers,
            "dependency_providers": dependency_providers,
            "guard_providers": guard_providers,
            "request_middlewares": request_middlewares,
            "response_middlewares": response_middlewares,
            "error_middlewares": error_middlewares,
            "startup_hooks": startup_hooks,
            "shutdown_hooks": shutdown_hooks,
            "functions": functions,
        }

    def function_key(self, fn: FunctionNode) -> str:
        module_name = getattr(fn, "module_name", None) or "__main__"
        return f"{module_name}.{fn.name}"

    def compile_function(self, fn: FunctionNode) -> dict[str, Any]:
        code: list[list[Any]] = []
        previous_module = self.current_module
        self.current_module = getattr(fn, "module_name", None) or "__main__"
        self.emit_block(fn.body, code)
        self.current_module = previous_module
        if not code or code[-1][0] != "RETURN":
            code.append(["PUSH_CONST", None])
            code.append(["RETURN"])
        return {
            "name": fn.name,
            "module": getattr(fn, "module_name", None) or "__main__",
            "params": [p.name for p in getattr(fn, "params", [])],
            "is_async": bool(getattr(fn, "is_async", False)),
            "code": code,
        }

    def compile_lambda_function(self, key: str, spec: dict[str, Any]) -> dict[str, Any]:
        node = spec["node"]
        capture_names = spec["capture_names"]
        code: list[list[Any]] = []
        previous_module = self.current_module
        self.current_module = spec.get("module", "__main__")
        self.emit_expr(node.body, code)
        code.append(["RETURN"])
        self.current_module = previous_module
        return {
            "name": key.rsplit(".", 1)[-1],
            "module": spec.get("module", "__main__"),
            "params": capture_names + [p.name for p in getattr(node, "params", [])],
            "is_async": False,
            "code": code,
        }

    def collect_free_vars(self, node: Any, bound: set[str] | None = None) -> set[str]:
        bound = bound or set()
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
            result = set()
            for item in node.items:
                result.update(self.collect_free_vars(item, bound))
            return result
        if isinstance(node, MapLiteralNode):
            result = set()
            for key_expr, value_expr in node.items:
                result.update(self.collect_free_vars(key_expr, bound))
                result.update(self.collect_free_vars(value_expr, bound))
            return result
        if isinstance(node, StructLiteralNode):
            result = set()
            for _field_name, value_expr in node.fields:
                result.update(self.collect_free_vars(value_expr, bound))
            return result
        if isinstance(node, CallNode):
            result = set()
            for arg in node.args:
                result.update(self.collect_free_vars(arg, bound))
            return result
        if isinstance(node, ModuleCallNode):
            result = set()
            for arg in node.args:
                result.update(self.collect_free_vars(arg, bound))
            return result
        if isinstance(node, IfExprNode):
            result = set()
            result.update(self.collect_free_vars(node.condition, bound))
            result.update(self.collect_free_vars(node.then_expr, bound))
            result.update(self.collect_free_vars(node.else_expr, bound))
            return result
        if isinstance(node, AwaitNode):
            return self.collect_free_vars(node.expr, bound)
        if isinstance(node, UnaryOpNode):
            return self.collect_free_vars(node.node, bound)
        if isinstance(node, BinOpNode):
            result = set()
            result.update(self.collect_free_vars(node.left, bound))
            result.update(self.collect_free_vars(node.right, bound))
            return result
        if isinstance(node, IndexNode):
            result = set()
            result.update(self.collect_free_vars(getattr(node, "list_expr", getattr(node, "target", None)), bound))
            result.update(self.collect_free_vars(getattr(node, "index_expr", None), bound))
            return result
        if isinstance(node, SliceNode):
            result = set()
            result.update(self.collect_free_vars(node.target, bound))
            result.update(self.collect_free_vars(node.start_expr, bound))
            result.update(self.collect_free_vars(node.end_expr, bound))
            return result
        if isinstance(node, FieldAccessNode):
            return self.collect_free_vars(node.target, bound)
        if isinstance(node, ListComprehensionNode):
            result = set()
            result.update(self.collect_free_vars(node.source_expr, bound))
            inner_bound = set(bound)
            inner_bound.add(node.item_name)
            if node.condition_expr is not None:
                result.update(self.collect_free_vars(node.condition_expr, inner_bound))
            result.update(self.collect_free_vars(node.item_expr, inner_bound))
            return result
        if isinstance(node, BlockNode):
            result = set()
            for stmt in node.statements:
                result.update(self.collect_free_vars(stmt, bound))
            return result
        if isinstance(node, ExprStmtNode):
            return self.collect_free_vars(node.expr, bound)
        if isinstance(node, PrintNode):
            return self.collect_free_vars(node.expr, bound)
        if isinstance(node, VarDeclareNode):
            result = self.collect_free_vars(node.expr, bound)
            return result
        if isinstance(node, VarSetNode):
            result = self.collect_free_vars(node.expr, bound)
            return result
        if isinstance(node, IndexSetNode):
            result = set()
            result.update(self.collect_free_vars(node.index_expr, bound))
            result.update(self.collect_free_vars(node.value_expr, bound))
            return result
        if isinstance(node, ReturnNode):
            return self.collect_free_vars(node.expr, bound)
        if isinstance(node, ThrowNode):
            return self.collect_free_vars(node.expr, bound)
        if isinstance(node, TryCatchNode):
            result = set()
            result.update(self.collect_free_vars(node.try_block, bound))
            catch_bound = set(bound)
            if node.catch_var_name:
                catch_bound.add(node.catch_var_name)
            result.update(self.collect_free_vars(node.catch_block, catch_bound))
            return result
        if isinstance(node, IfNode):
            result = set()
            result.update(self.collect_free_vars(node.condition, bound))
            result.update(self.collect_free_vars(node.then_block, bound))
            for cond, block in getattr(node, "elif_blocks", []):
                result.update(self.collect_free_vars(cond, bound))
                result.update(self.collect_free_vars(block, bound))
            if node.else_block:
                result.update(self.collect_free_vars(node.else_block, bound))
            return result
        if isinstance(node, MatchNode):
            result = set()
            result.update(self.collect_free_vars(node.subject, bound))
            for case in getattr(node, "cases", []):
                if not getattr(case, "wildcard", False):
                    result.update(self.collect_free_vars(case.pattern, bound))
                result.update(self.collect_free_vars(case.body, bound))
            if node.else_block:
                result.update(self.collect_free_vars(node.else_block, bound))
            return result
        if isinstance(node, WhileNode):
            result = set()
            result.update(self.collect_free_vars(node.condition, bound))
            result.update(self.collect_free_vars(node.body, bound))
            return result
        if isinstance(node, ForNode):
            result = set()
            result.update(self.collect_free_vars(node.start_expr, bound))
            result.update(self.collect_free_vars(node.end_expr, bound))
            result.update(self.collect_free_vars(node.step_expr, bound))
            inner_bound = set(bound)
            inner_bound.add(node.name)
            result.update(self.collect_free_vars(node.body, inner_bound))
            return result
        if isinstance(node, ForEachNode):
            result = set()
            result.update(self.collect_free_vars(node.list_expr, bound))
            inner_bound = set(bound)
            inner_bound.add(node.item_name)
            result.update(self.collect_free_vars(node.body, inner_bound))
            return result
        return set()

    def register_lambda(self, node: LambdaNode) -> tuple[str, list[str]]:
        capture_names = sorted(self.collect_free_vars(node.body, {param.name for param in getattr(node, "params", [])}))
        key = f"{self.current_module}.__lambda_{self.lambda_counter}"
        self.lambda_counter += 1
        if key not in self.pending_lambda_specs:
            self.pending_lambda_specs[key] = {
                "node": node,
                "capture_names": capture_names,
                "module": self.current_module,
            }
        return key, capture_names

    def emit_block(self, block: BlockNode, code: list[list[Any]]):
        for stmt in getattr(block, "statements", []):
            self.emit_stmt(stmt, code)

    def emit_stmt(self, node: Any, code: list[list[Any]]):
        if isinstance(node, VarDeclareNode):
            self.emit_expr(node.expr, code)
            code.append(["STORE_NAME", node.name])
            return
        if isinstance(node, VarSetNode):
            self.emit_expr(node.expr, code)
            code.append(["STORE_NAME", node.name])
            return
        if isinstance(node, IndexSetNode):
            code.append(["LOAD_NAME", node.target_name])
            self.emit_expr(node.index_expr, code)
            self.emit_expr(node.value_expr, code)
            code.append(["INDEX_SET"])
            return
        if isinstance(node, ExprStmtNode):
            self.emit_expr(node.expr, code)
            code.append(["POP"])
            return
        if isinstance(node, PrintNode):
            self.emit_expr(node.expr, code)
            code.append(["CALL", "builtin.skriv", 1])
            code.append(["POP"])
            return
        if isinstance(node, ReturnNode):
            self.emit_expr(node.expr, code)
            code.append(["RETURN"])
            return
        if isinstance(node, ThrowNode):
            self.emit_expr(node.expr, code)
            code.append(["THROW"])
            return
        if isinstance(node, TryCatchNode):
            catch_label = self.new_label("catch")
            after_label = self.new_label("after_try")
            code.append(["TRY_BEGIN", catch_label])
            self.emit_block(node.try_block, code)
            code.append(["TRY_END"])
            code.append(["JUMP", after_label])
            code.append(["LABEL", catch_label])
            code.append(["TRY_END"])
            if node.catch_var_name:
                code.append(["LOAD_EXCEPTION"])
                code.append(["STORE_NAME", node.catch_var_name])
            self.emit_block(node.catch_block, code)
            code.append(["LABEL", after_label])
            return
        if isinstance(node, IfNode):
            end_label = self.new_label("ifend")
            else_label = self.new_label("ifelse") if (node.else_block or node.elif_blocks) else end_label
            self.emit_expr(node.condition, code)
            code.append(["JUMP_IF_FALSE", else_label])
            self.emit_block(node.then_block, code)
            code.append(["JUMP", end_label])
            current_else = else_label
            if node.elif_blocks:
                for idx, (cond, block) in enumerate(node.elif_blocks):
                    code.append(["LABEL", current_else])
                    next_else = self.new_label(f"elifelse_{idx}") if (idx < len(node.elif_blocks)-1 or node.else_block) else end_label
                    self.emit_expr(cond, code)
                    code.append(["JUMP_IF_FALSE", next_else])
                    self.emit_block(block, code)
                    code.append(["JUMP", end_label])
                    current_else = next_else
            if node.else_block:
                code.append(["LABEL", current_else])
                self.emit_block(node.else_block, code)
            code.append(["LABEL", end_label])
            return
        if isinstance(node, MatchNode):
            subject_var = self.new_label("match_subject")
            end_label = self.new_label("match_end")
            self.emit_expr(node.subject, code)
            code.append(["STORE_NAME", subject_var])
            has_else = bool(node.else_block)
            cases = list(getattr(node, "cases", []))
            for idx, case in enumerate(cases):
                if getattr(case, "wildcard", False):
                    if idx != len(cases) - 1:
                        raise BytecodeCompileError("Wildcard-case i match må være siste case")
                    self.emit_block(case.body, code)
                    code.append(["JUMP", end_label])
                    continue

                next_label = self.new_label(f"match_next_{idx}")
                self.emit_expr(case.pattern, code)
                code.append(["LOAD_NAME", subject_var])
                code.append(["COMPARE_EQ"])
                code.append(["JUMP_IF_FALSE", next_label])
                self.emit_block(case.body, code)
                code.append(["JUMP", end_label])
                code.append(["LABEL", next_label])

            if has_else:
                self.emit_block(node.else_block, code)
                code.append(["JUMP", end_label])
            code.append(["LABEL", end_label])
            return
        if isinstance(node, WhileNode):
            start = self.new_label("while_start")
            end = self.new_label("while_end")
            self.loop_stack.append(LoopLabels(start, end, start))
            code.append(["LABEL", start])
            self.emit_expr(node.condition, code)
            code.append(["JUMP_IF_FALSE", end])
            self.emit_block(node.body, code)
            code.append(["JUMP", start])
            code.append(["LABEL", end])
            self.loop_stack.pop()
            return
        if isinstance(node, ForNode):
            start = self.new_label("for_start")
            end = self.new_label("for_end")
            continue_label = self.new_label("for_continue")
            self.emit_expr(node.start_expr, code)
            code.append(["STORE_NAME", node.name])
            self.loop_stack.append(LoopLabels(start, end, continue_label))
            code.append(["LABEL", start])
            code.append(["LOAD_NAME", node.name])
            self.emit_expr(node.end_expr, code)
            code.append(["COMPARE_LE"])
            code.append(["JUMP_IF_FALSE", end])
            self.emit_block(node.body, code)
            code.append(["LABEL", continue_label])
            code.append(["LOAD_NAME", node.name])
            self.emit_expr(node.step_expr, code)
            code.append(["BINARY_ADD"])
            code.append(["STORE_NAME", node.name])
            code.append(["JUMP", start])
            code.append(["LABEL", end])
            self.loop_stack.pop()
            return
        if isinstance(node, ForEachNode):
            list_var = self.new_label("foreach_list")
            index_var = self.new_label("foreach_i")
            start = self.new_label("foreach_start")
            end = self.new_label("foreach_end")
            continue_label = self.new_label("foreach_continue")
            self.emit_expr(node.list_expr, code)
            code.append(["STORE_NAME", list_var])
            code.append(["PUSH_CONST", 0])
            code.append(["STORE_NAME", index_var])
            self.loop_stack.append(LoopLabels(start, end, continue_label))
            code.append(["LABEL", start])
            code.append(["LOAD_NAME", index_var])
            code.append(["LOAD_NAME", list_var])
            code.append(["CALL", "builtin.lengde", 1])
            code.append(["COMPARE_LT"])
            code.append(["JUMP_IF_FALSE", end])
            code.append(["LOAD_NAME", list_var])
            code.append(["LOAD_NAME", index_var])
            code.append(["INDEX_GET"])
            code.append(["STORE_NAME", node.item_name])
            self.emit_block(node.body, code)
            code.append(["LABEL", continue_label])
            code.append(["LOAD_NAME", index_var])
            code.append(["PUSH_CONST", 1])
            code.append(["BINARY_ADD"])
            code.append(["STORE_NAME", index_var])
            code.append(["JUMP", start])
            code.append(["LABEL", end])
            self.loop_stack.pop()
            return
        if isinstance(node, BreakNode):
            if not self.loop_stack:
                raise BytecodeCompileError("'bryt' utenfor løkke")
            code.append(["JUMP", self.loop_stack[-1].end])
            return
        if isinstance(node, ContinueNode):
            if not self.loop_stack:
                raise BytecodeCompileError("'fortsett' utenfor løkke")
            code.append(["JUMP", self.loop_stack[-1].continue_target])
            return
        raise BytecodeCompileError(f"Bytecode-backend støtter ikke statement: {type(node).__name__}")

    def emit_expr(self, node: Any, code: list[list[Any]]):
        if isinstance(node, NumberNode):
            code.append(["PUSH_CONST", node.value])
            return
        if isinstance(node, StringNode):
            code.append(["PUSH_CONST", node.value])
            return
        if isinstance(node, BoolNode):
            code.append(["PUSH_CONST", node.value])
            return
        if isinstance(node, VarAccessNode):
            code.append(["LOAD_NAME", self.resolve_name(node.name)])
            return
        if isinstance(node, ListLiteralNode):
            for item in node.items:
                self.emit_expr(item, code)
            code.append(["BUILD_LIST", len(node.items)])
            return
        if isinstance(node, ListComprehensionNode):
            source_var = self.new_label("comp_source")
            result_var = self.new_label("comp_result")
            index_var = self.new_label("comp_i")
            item_var = self.new_label("comp_item")
            start = self.new_label("comp_start")
            end = self.new_label("comp_end")
            skip_append = self.new_label("comp_skip") if node.condition_expr is not None else None

            self.emit_expr(node.source_expr, code)
            code.append(["STORE_NAME", source_var])
            code.append(["BUILD_LIST", 0])
            code.append(["STORE_NAME", result_var])
            code.append(["PUSH_CONST", 0])
            code.append(["STORE_NAME", index_var])

            self.loop_stack.append(LoopLabels(start, end, start))
            code.append(["LABEL", start])
            code.append(["LOAD_NAME", index_var])
            code.append(["LOAD_NAME", source_var])
            code.append(["CALL", "builtin.lengde", 1])
            code.append(["COMPARE_LT"])
            code.append(["JUMP_IF_FALSE", end])
            code.append(["LOAD_NAME", source_var])
            code.append(["LOAD_NAME", index_var])
            code.append(["INDEX_GET"])
            code.append(["STORE_NAME", item_var])
            self.push_name_aliases({node.item_name: item_var})
            if node.condition_expr is not None:
                self.emit_expr(node.condition_expr, code)
                code.append(["JUMP_IF_FALSE", skip_append])
            code.append(["LOAD_NAME", result_var])
            self.emit_expr(node.item_expr, code)
            code.append(["CALL", "builtin.legg_til", 2])
            code.append(["POP"])
            if skip_append is not None:
                code.append(["LABEL", skip_append])
            self.pop_name_aliases()
            code.append(["LOAD_NAME", index_var])
            code.append(["PUSH_CONST", 1])
            code.append(["BINARY_ADD"])
            code.append(["STORE_NAME", index_var])
            code.append(["JUMP", start])
            code.append(["LABEL", end])
            self.loop_stack.pop()
            code.append(["LOAD_NAME", result_var])
            return
        if isinstance(node, MapLiteralNode):
            for key_expr, value_expr in node.items:
                self.emit_expr(key_expr, code)
                self.emit_expr(value_expr, code)
            code.append(["BUILD_MAP", len(node.items)])
            return
        if isinstance(node, StructLiteralNode):
            for field_name, value_expr in node.fields:
                code.append(["PUSH_CONST", field_name])
                self.emit_expr(value_expr, code)
            code.append(["BUILD_MAP", len(node.fields)])
            return
        if isinstance(node, IndexNode):
            self.emit_expr(node.list_expr, code)
            self.emit_expr(node.index_expr, code)
            code.append(["INDEX_GET"])
            return
        if isinstance(node, SliceNode):
            self.emit_expr(node.target, code)
            if node.start_expr is None:
                code.append(["PUSH_CONST", None])
            else:
                self.emit_expr(node.start_expr, code)
            if node.end_expr is None:
                code.append(["PUSH_CONST", None])
            else:
                self.emit_expr(node.end_expr, code)
            code.append(["CALL", "builtin.slice", 3])
            return
        if isinstance(node, AwaitNode):
            self.emit_expr(node.expr, code)
            code.append(["CALL", "builtin.await_value", 1])
            return
        if isinstance(node, LambdaNode):
            lambda_name, capture_names = self.register_lambda(node)
            for capture_name in capture_names:
                code.append(["LOAD_NAME", self.resolve_name(capture_name)])
            code.append(["BUILD_LAMBDA", lambda_name, capture_names])
            return
        if isinstance(node, FieldAccessNode):
            self.emit_expr(node.target, code)
            code.append(["PUSH_CONST", node.field])
            code.append(["INDEX_GET"])
            return
        if isinstance(node, UnaryOpNode):
            self.emit_expr(node.node, code)
            if node.op.typ == "MINUS":
                code.append(["UNARY_NEG"])
                return
            if node.op.typ == "IKKE":
                code.append(["UNARY_NOT"])
                return
            if node.op.typ == "PLUS":
                return
            raise BytecodeCompileError(f"Ukjent unary-op: {node.op.typ}")
        if isinstance(node, BinOpNode):
            if node.op.typ == "OG":
                end_label = self.new_label("and_end")
                false_label = self.new_label("and_false")
                self.emit_expr(node.left, code)
                code.append(["JUMP_IF_FALSE", false_label])
                self.emit_expr(node.right, code)
                code.append(["JUMP_IF_FALSE", false_label])
                code.append(["PUSH_CONST", True])
                code.append(["JUMP", end_label])
                code.append(["LABEL", false_label])
                code.append(["PUSH_CONST", False])
                code.append(["LABEL", end_label])
                return
            if node.op.typ == "ELLER":
                end_label = self.new_label("or_end")
                true_label = self.new_label("or_true")
                self.emit_expr(node.left, code)
                code.append(["JUMP_IF_FALSE", true_label + "_rhs"])
                code.append(["PUSH_CONST", True])
                code.append(["JUMP", end_label])
                code.append(["LABEL", true_label + "_rhs"])
                self.emit_expr(node.right, code)
                code.append(["JUMP_IF_FALSE", true_label + "_false"])
                code.append(["PUSH_CONST", True])
                code.append(["JUMP", end_label])
                code.append(["LABEL", true_label + "_false"])
                code.append(["PUSH_CONST", False])
                code.append(["LABEL", end_label])
                return
            self.emit_expr(node.left, code)
            self.emit_expr(node.right, code)
            mapping = {
                "PLUS": "BINARY_ADD",
                "MINUS": "BINARY_SUB",
                "MUL": "BINARY_MUL",
                "DIV": "BINARY_DIV",
                "EQ": "COMPARE_EQ",
                "NE": "COMPARE_NE",
                "GT": "COMPARE_GT",
                "LT": "COMPARE_LT",
                "GTE": "COMPARE_GE",
                "LTE": "COMPARE_LE",
            }
            opcode = mapping.get(node.op.typ)
            if opcode is None:
                raise BytecodeCompileError(f"Ukjent binæroperator: {node.op.typ}")
            code.append([opcode])
            return
        if isinstance(node, CallNode):
            if getattr(node, "call_kind", None) == "lambda":
                code.append(["LOAD_NAME", self.resolve_name(node.name)])
                for arg in node.args:
                    self.emit_expr(arg, code)
                code.append(["CALL_VALUE", len(node.args)])
                return
            for arg in node.args:
                self.emit_expr(arg, code)
            code.append(["CALL", self.resolve_call_name(node.name), len(node.args)])
            return
        if isinstance(node, ModuleCallNode):
            for arg in node.args:
                self.emit_expr(arg, code)
            module_name = self.alias_map.get(node.module_name, node.module_name)
            code.append(["CALL", f"{module_name}.{node.func_name}", len(node.args)])
            return
        if isinstance(node, IfExprNode):
            else_label = self.new_label("ifexpr_else")
            end_label = self.new_label("ifexpr_end")
            self.emit_expr(node.condition, code)
            code.append(["JUMP_IF_FALSE", else_label])
            self.emit_expr(node.then_expr, code)
            code.append(["JUMP", end_label])
            code.append(["LABEL", else_label])
            self.emit_expr(node.else_expr, code)
            code.append(["LABEL", end_label])
            return
        raise BytecodeCompileError(f"Bytecode-backend støtter ikke uttrykk: {type(node).__name__}")

    def resolve_call_name(self, name: str) -> str:
        builtins = {
            "assert", "assert_eq", "assert_ne", "assert_starter_med", "assert_slutter_med", "assert_inneholder", "skriv", "lengde",
            "tekst_fra_heltall", "tekst_fra_bool", "har_nokkel",
            "json_parse", "json_stringify",
            "slice",
            "fil_les", "fil_skriv", "fil_append", "fil_finnes",
            "sti_join", "sti_basename", "sti_dirname", "sti_exists", "sti_stem",
            "miljo_hent", "miljo_finnes", "miljo_sett",
        }
        if name in builtins:
            return f"builtin.{name}"
        return f"{self.current_module}.{name}"


class BytecodeVM:
    def __init__(
        self,
        program: dict[str, Any],
        trace: bool = False,
        max_steps: int = 1000000000,
        trace_focus: str | None = None,
        repeat_limit: int = 0,
        expr_probe: str | None = None,
        expr_probe_log: str | None = None,
    ):
        self.program = program
        self.functions = program.get("functions", {})
        self.route_handlers = program.get("route_handlers", [])
        self.dependency_providers = program.get("dependency_providers", {})
        self.guard_providers = program.get("guard_providers", {})
        self.request_middlewares = program.get("request_middlewares", [])
        self.response_middlewares = program.get("response_middlewares", [])
        self.error_middlewares = program.get("error_middlewares", [])
        self.startup_hooks = program.get("startup_hooks", [])
        self.shutdown_hooks = program.get("shutdown_hooks", [])
        self.output: list[str] = []
        self.trace = trace
        self.max_steps = max_steps
        self.trace_focus = trace_focus.strip() if isinstance(trace_focus, str) and trace_focus.strip() else None
        self.repeat_limit = max(0, int(repeat_limit))
        self.expr_probe = expr_probe.strip() if isinstance(expr_probe, str) and expr_probe.strip() else None
        self.expr_probe_log = expr_probe_log.strip() if isinstance(expr_probe_log, str) and expr_probe_log.strip() else None
        self.expr_probe_events: list[str] = []
        self.steps = 0
        self.trace_log: list[str] = []
        self._repeat_state_key: tuple[Any, ...] | None = None
        self._repeat_state_count = 0
        self._selfhost_token_map = self._load_selfhost_token_map()
        self._memo_cache: dict[tuple[str, str], Any] = {}
        self._exception_stack: list[dict[str, int]] = []
        self._exception_value: Any = None
        self._call_stack: list[str] = []
        self._startup_ran = False
        self._shutdown_ran = False
        self._memoizable_functions = {
            "selfhost.compiler.normaliser_norsk_token",
            "selfhost.compiler.stack_behov",
            "selfhost.compiler.stack_endring",
            "selfhost.compiler.op_krever_arg",
            "selfhost.compiler.op_kjent",
            "selfhost.compiler.er_heltall_token",
            "selfhost.compiler.operator_til_opcode",
            "selfhost.compiler.uttrykk_til_ops_og_verdier_med_miljo",
            "selfhost.compiler.instruksjon_til_tekst",
            "selfhost.compiler.append_linje",
            "selfhost.compiler.disasm_program",
        }

    def _should_trace_function(self, name: str) -> bool:
        if not self.trace:
            return False
        if self.trace_focus is None:
            return True
        return self.trace_focus in name

    def _log(self, message: str) -> None:
        if not self.trace:
            return
        self.trace_log.append(message)
        if len(self.trace_log) > 400:
            del self.trace_log[:200]

    def _dependency_map(self, ctx: Any) -> dict[str, str]:
        if not isinstance(ctx, dict):
            return {}
        return _decode_json_text_map(ctx.get("deps", "{}"))

    def _store_dependency_map(self, ctx: Any, deps: dict[str, str]) -> None:
        if isinstance(ctx, dict):
            ctx["deps"] = _encode_json_text_map(deps)

    def _normalize_dependency_value(self, value: Any) -> dict[str, str]:
        if isinstance(value, dict):
            return {str(key): str(val) for key, val in value.items()}
        if value is None:
            return {}
        if isinstance(value, str):
            try:
                parsed = json.loads(value)
                if isinstance(parsed, dict):
                    return {str(key): str(item) for key, item in parsed.items()}
            except Exception:
                pass
            return {"value": value}
        return {"value": str(value)}

    def _resolve_dependency_value(self, ctx: Any, dep_name: str):
        deps = self._dependency_map(ctx)
        if dep_name in deps:
            cached = deps.get(dep_name, "")
            try:
                parsed = json.loads(cached) if cached else {}
            except Exception:
                parsed = {}
            if isinstance(parsed, dict):
                return {str(key): str(item) for key, item in parsed.items()}
            return {}
        provider_name = self.dependency_providers.get(dep_name)
        if not provider_name:
            raise BytecodeRuntimeError(f"Mangler dependency-provider for '{dep_name}'")
        provider = self.functions.get(provider_name)
        if provider is None:
            raise BytecodeRuntimeError(f"Mangler dependency-funksjon '{provider_name}'")
        provider_args = [ctx] if provider.get("params") else []
        value = self.call_function(provider_name, provider_args)
        normalized = self._normalize_dependency_value(value)
        deps[dep_name] = json.dumps(normalized, ensure_ascii=False)
        self._store_dependency_map(ctx, deps)
        return normalized

    def get_trace_tail(self, limit: int = 60) -> list[str]:
        return self.trace_log[-limit:]

    def _run_named_hook(self, name: str) -> Any:
        if not name:
            return None
        return self.call_function(name, [])

    def _ensure_startup_hooks(self) -> int:
        if self._startup_ran:
            return 0
        self._startup_ran = True
        count = 0
        for hook in self.startup_hooks:
            self._run_named_hook(hook)
            count += 1
        return count

    def _run_shutdown_hooks(self) -> int:
        if self._shutdown_ran:
            return 0
        self._shutdown_ran = True
        count = 0
        for hook in self.shutdown_hooks:
            self._run_named_hook(hook)
            count += 1
        return count

    def _apply_request_middlewares(self, ctx: Any) -> Any:
        current = ctx
        for middleware in self.request_middlewares:
            result = self.call_function(middleware, [current])
            if isinstance(result, dict):
                current = result
        return current

    def _apply_response_middlewares(self, response: Any) -> Any:
        current = response
        for middleware in self.response_middlewares:
            result = self.call_function(middleware, [current])
            if isinstance(result, dict):
                current = result
        return current

    def _apply_error_middlewares(self, response: Any) -> Any:
        current = response
        for middleware in self.error_middlewares:
            result = self.call_function(middleware, [current])
            if isinstance(result, dict):
                current = result
        return current

    def _record_expr_probe(self, lines: list[str]) -> None:
        payload = [str(line) for line in lines]
        self.expr_probe_events.extend(payload)
        if self.trace:
            for line in payload:
                self._log(line)

    def _push_try_frame(self, handler_label: str) -> None:
        self._exception_stack.append({"handler": handler_label})

    def _pop_try_frame(self) -> None:
        if self._exception_stack:
            self._exception_stack.pop()

    def _handle_throw(self, value: Any, labels: dict[str, int]) -> int | None:
        for frame in reversed(self._exception_stack):
            handler = frame["handler"]
            if handler in labels:
                self._exception_value = value
                return labels[handler]
        return None

    def dump_expr_probe(self) -> str:
        text = "\n".join(self.expr_probe_events)
        if text and not text.endswith("\n"):
            text += "\n"
        if self.expr_probe_log and text:
            Path(self.expr_probe_log).expanduser().write_text(text, encoding="utf-8")
        return text


    def _load_selfhost_token_map(self) -> dict[str, str]:
        source_path = Path(__file__).resolve().parent.parent / "selfhost" / "compiler.no"
        if not source_path.exists():
            return {}
        lines = source_path.read_text(encoding="utf-8").splitlines()
        mapping: dict[str, str] = {}
        inside = False
        pending: list[str] = []
        for raw in lines:
            line = raw.strip()
            if line.startswith("funksjon normaliser_norsk_token"):
                inside = True
                continue
            if inside and line.startswith("funksjon "):
                break
            if not inside:
                continue
            if line.startswith("hvis (") and "tok ==" in line:
                pending = re.findall(r'"([^"]+)"', line)
                continue
            if line.startswith("eller ") and "tok ==" in line:
                pending.extend(re.findall(r'"([^"]+)"', line))
                continue
            if pending and line.startswith("returner "):
                values = re.findall(r'"([^"]+)"', line)
                if values:
                    for key in pending:
                        mapping[key] = values[0]
                pending = []
        return mapping

    def _selfhost_emit_operator(self, ops: list[Any], verdier: list[Any], op: str) -> bool:
        if op in {"u!", "uikke"}:
            ops.append("NOT")
            verdier.append(0)
            return True
        if op == "u-":
            ops.extend(["PUSH", "SWAP", "SUB"])
            verdier.extend([0, 0, 0])
            return True
        if op in {"!=", "ikke_er"}:
            # Equality should be evaluated before negation so phrases like
            # "ikke er" and "ikke lik" produce EQ followed by NOT.
            ops.extend(["EQ", "NOT"])
            verdier.extend([0, 0])
            return True
        if op in {"xor", "^^"}:
            ops.extend(["EQ", "NOT"])
            verdier.extend([0, 0])
            return True
        if op == "xnor":
            ops.append("EQ")
            verdier.append(0)
            return True
        if op == "nand":
            ops.extend(["AND", "NOT"])
            verdier.extend([0, 0])
            return True
        if op == "nor":
            ops.extend(["OR", "NOT"])
            verdier.extend([0, 0])
            return True
        if op == "impliserer":
            ops.extend(["SWAP", "NOT", "SWAP", "OR"])
            verdier.extend([0, 0, 0, 0])
            return True
        if op == "impliseres_av":
            ops.extend(["NOT", "OR"])
            verdier.extend([0, 0])
            return True
        if op in {"<=", "mindre_eller_lik"}:
            ops.extend(["GT", "NOT"])
            verdier.extend([0, 0])
            return True
        if op in {">=", "storre_eller_lik"}:
            ops.extend(["LT", "NOT"])
            verdier.extend([0, 0])
            return True
        opcode = self._call_hot_selfhost_helper("selfhost.compiler.operator_til_opcode", [op])
        if opcode:
            ops.append(opcode)
            verdier.append(0)
            return True
        return False

    def _selfhost_operator_precedence(self, tok: str) -> int:
        if tok in {"u!", "u-", "uikke"}:
            return 7
        if tok in {"*", "/", "%"}:
            return 6
        if tok in {"+", "-"}:
            return 5
        if tok in {"<", ">", "<=", ">=", "mindre_enn", "storre_enn", "mindre_eller_lik", "storre_eller_lik"}:
            return 4
        if tok in {"==", "!=", "er", "ikke_er"}:
            return 3
        if tok in {"&&", "og", "samt", "nand"}:
            return 2
        if tok in {"||", "eller", "enten", "xor", "^^", "xnor", "nor", "impliserer", "impliseres_av"}:
            return 1
        return 0

    def _selfhost_is_operator_token(self, tok: str) -> bool:
        return tok in {
            "+", "-", "*", "/", "%", "==", "!=", "<", ">", "<=", ">=",
            "&&", "||", "!", "og", "eller", "samt", "ikke", "nand", "nor",
            "xor", "^^", "xnor", "impliserer", "impliseres_av", "er", "ikke_er",
            "mindre_enn", "storre_enn", "mindre_eller_lik", "storre_eller_lik", "enten"
        }

    def _hot_selfhost_expr_to_ops(self, tokens: list[Any], navn: list[Any], miljo_verdier: list[Any], ops: list[Any], verdier: list[Any]) -> str:
        toks = [str(t) for t in tokens]
        names = [str(n) for n in navn]
        vals = list(miljo_verdier)
        if len(names) != len(vals):
            return "/* feil: navn/miljø-verdier må ha samme lengde */"
        env = {n: vals[i] for i, n in enumerate(names)}
        operatorer: list[str] = []
        operator_pos: list[int] = []
        i = 0
        siste_token = -1
        forventer_verdi = True

        if self.expr_probe:
            probe = self.expr_probe.lower()
            raw_join = " ".join(toks).lower()
            if probe in raw_join or any(probe in str(t).lower() for t in toks):
                self._record_expr_probe([
                    f"[expr-probe] helper=selfhost.compiler.uttrykk_til_ops_og_verdier_med_miljo",
                    f"[expr-probe] raw_tokens={toks!r}",
                    f"[expr-probe] env_names={names!r}",
                    f"[expr-probe] env_values={vals!r}",
                ])

        def token_pos(pos: int) -> str:
            return f"token {pos}"

        while i < len(toks):
            tok_raw = toks[i]
            tok = str(self._call_hot_selfhost_helper("selfhost.compiler.normaliser_norsk_token", [tok_raw]))
            tok_step = 1
            n1 = str(self._call_hot_selfhost_helper("selfhost.compiler.normaliser_norsk_token", [toks[i + 1]])) if i + 1 < len(toks) else ""
            n2 = str(self._call_hot_selfhost_helper("selfhost.compiler.normaliser_norsk_token", [toks[i + 2]])) if i + 2 < len(toks) else ""
            n3 = str(self._call_hot_selfhost_helper("selfhost.compiler.normaliser_norsk_token", [toks[i + 3]])) if i + 3 < len(toks) else ""
            n4 = str(self._call_hot_selfhost_helper("selfhost.compiler.normaliser_norsk_token", [toks[i + 4]])) if i + 4 < len(toks) else ""
            n5 = str(self._call_hot_selfhost_helper("selfhost.compiler.normaliser_norsk_token", [toks[i + 5]])) if i + 5 < len(toks) else ""
            # compact normalizations for common multi-token forms
            if i + 1 < len(toks) and tok_raw == "-" and n1 == ">":
                tok = "impliserer"; tok_step = 2
            elif i + 1 < len(toks) and tok_raw == "=" and n1 == ">":
                tok = "impliserer"; tok_step = 2
            elif tok_raw in {"->", "=>"}:
                tok = "impliserer"; tok_step = 1
            elif tok_raw == "<-":
                tok = "impliseres_av"; tok_step = 1
            elif tok_raw in {"<->", "<=>"}:
                tok = "xnor"; tok_step = 1
            elif i + 1 < len(toks) and tok_raw == "divided" and n1 in {"by", "of"}:
                tok = "/"; tok_step = 2
            elif i + 1 < len(toks) and tok_raw == "modulo" and n1 in {"by", "of"}:
                tok = "%"; tok_step = 2
            elif i + 1 < len(toks) and tok in {"*", "/", "%"} and n1 in {"by", "of"}:
                tok_step = 2
            elif i + 2 < len(toks) and tok == "er" and n1 == "mindre" and n2 == "enn":
                tok = "mindre_enn"; tok_step = 3
            elif i + 3 < len(toks) and tok == "er" and n1 == "mindre" and n2 == "enn" and n3 == "lik":
                tok = "mindre_eller_lik"; tok_step = 4
            elif i + 2 < len(toks) and tok == "er" and n1 == "mindre" and n2 == "lik":
                tok = "mindre_eller_lik"; tok_step = 3
            elif i + 4 < len(toks) and tok == "er" and n1 == "mindre" and n2 == "enn" and n3 == "eller" and n4 == "lik":
                tok = "mindre_eller_lik"; tok_step = 5
            elif i + 3 < len(toks) and tok == "er" and n1 == "mindre" and n2 == "eller" and n3 == "lik":
                tok = "mindre_eller_lik"; tok_step = 4
            elif i + 3 < len(toks) and tok == "er" and n1 == "storre" and n2 == "enn" and n3 == "lik":
                tok = "storre_eller_lik"; tok_step = 4
            elif i + 2 < len(toks) and tok == "er" and n1 == "storre" and n2 == "lik":
                tok = "storre_eller_lik"; tok_step = 3
            elif i + 4 < len(toks) and tok == "er" and n1 == "storre" and n2 == "enn" and n3 == "eller" and n4 == "lik":
                tok = "storre_eller_lik"; tok_step = 5
            elif i + 3 < len(toks) and tok == "er" and n1 == "storre" and n2 == "eller" and n3 == "lik":
                tok = "storre_eller_lik"; tok_step = 4
            elif i + 2 < len(toks) and tok == "er" and n1 == "storre" and n2 == "enn":
                tok = "storre_enn"; tok_step = 3
            elif i + 3 < len(toks) and tok == "er" and n1 == "ikke" and n2 == "lik" and n3 == "med":
                tok = "ikke_er"; tok_step = 4
            elif i + 2 < len(toks) and tok == "er" and n1 == "ikke" and n2 == "lik":
                tok = "ikke_er"; tok_step = 3
            elif i + 2 < len(toks) and tok == "ikke" and n1 == "lik" and n2 == "med":
                tok = "ikke_er"; tok_step = 3
            elif i + 1 < len(toks) and tok == "ulik" and n1 == "med":
                tok = "ikke_er"; tok_step = 2
            elif i + 2 < len(toks) and tok == "er" and n1 == "lik" and n2 == "med":
                tok = "er"; tok_step = 3
            elif i + 2 < len(toks) and tok == "ikke" and n1 in {"er", "equal", "equals"} and n2 == "to":
                tok = "ikke_er"; tok_step = 3
            elif i + 3 < len(toks) and tok == "er" and n1 == "ikke" and n2 in {"er", "equal", "equals"} and n3 == "to":
                tok = "ikke_er"; tok_step = 4
            elif i + 3 < len(toks) and tok == "er" and n1 in {"er", "equal", "equals"} and n2 == "to":
                tok = "er"; tok_step = 3
            elif i + 1 < len(toks) and tok == "er" and n1 == "to":
                tok = "er"; tok_step = 2
            elif i + 2 < len(toks) and tok == "er" and n1 == "ikke" and n2 == "er":
                tok = "ikke_er"; tok_step = 3
            elif i + 1 < len(toks) and tok == "er" and n1 == "ulik":
                tok = "ikke_er"; tok_step = 2
            elif i + 1 < len(toks) and tok == "ikke" and n1 == "lik":
                tok = "ikke_er"; tok_step = 2
            elif i + 1 < len(toks) and tok == "er" and n1 == "ikke":
                tok = "ikke_er"; tok_step = 2
            elif i + 1 < len(toks) and tok == "ikke" and n1 == "er":
                tok = "ikke_er"; tok_step = 2
            elif i + 1 < len(toks) and tok == "er" and n1 == "lik":
                tok = "er"; tok_step = 2
            elif i + 4 < len(toks) and tok in {"er", "equal", "equals"} and n1 == "less" and n2 in {"or", "eller"} and n3 in {"equal", "equals", "er"} and n4 == "to":
                tok = "mindre_eller_lik"; tok_step = 5
            elif i + 4 < len(toks) and tok in {"er", "equal", "equals"} and n1 == "greater" and n2 in {"or", "eller"} and n3 in {"equal", "equals", "er"} and n4 == "to":
                tok = "storre_eller_lik"; tok_step = 5
            elif i + 3 < len(toks) and tok in {"er", "equal", "equals"} and n1 == "less" and n2 in {"or", "eller"} and n3 in {"equal", "equals", "er"}:
                tok = "mindre_eller_lik"; tok_step = 4
            elif i + 3 < len(toks) and tok in {"er", "equal", "equals"} and n1 == "greater" and n2 in {"or", "eller"} and n3 in {"equal", "equals", "er"}:
                tok = "storre_eller_lik"; tok_step = 4
            elif i + 5 < len(toks) and tok in {"er", "equal", "equals"} and n1 == "less" and n2 == "than" and n3 in {"or", "eller"} and n4 in {"equal", "equals", "er"} and n5 == "to":
                tok = "mindre_eller_lik"; tok_step = 6
            elif i + 5 < len(toks) and tok in {"er", "equal", "equals"} and n1 == "greater" and n2 == "than" and n3 in {"or", "eller"} and n4 in {"equal", "equals", "er"} and n5 == "to":
                tok = "storre_eller_lik"; tok_step = 6
            elif i + 4 < len(toks) and tok in {"er", "equal", "equals"} and n1 == "less" and n2 == "than" and n3 in {"or", "eller"} and n4 in {"equal", "equals", "er"}:
                tok = "mindre_eller_lik"; tok_step = 5
            elif i + 4 < len(toks) and tok in {"er", "equal", "equals"} and n1 == "greater" and n2 == "than" and n3 in {"or", "eller"} and n4 in {"equal", "equals", "er"}:
                tok = "storre_eller_lik"; tok_step = 5
            elif i + 4 < len(toks) and tok in {"er", "equal", "equals"} and n1 == "less" and n2 == "than" and n3 in {"equal", "equals", "er"} and n4 == "to":
                tok = "mindre_eller_lik"; tok_step = 5
            elif i + 4 < len(toks) and tok in {"er", "equal", "equals"} and n1 == "greater" and n2 == "than" and n3 in {"equal", "equals", "er"} and n4 == "to":
                tok = "storre_eller_lik"; tok_step = 5
            elif i + 3 < len(toks) and tok in {"er", "equal", "equals"} and n1 == "less" and n2 == "than" and n3 in {"equal", "equals", "er"}:
                tok = "mindre_eller_lik"; tok_step = 4
            elif i + 3 < len(toks) and tok in {"er", "equal", "equals"} and n1 == "greater" and n2 == "than" and n3 in {"equal", "equals", "er"}:
                tok = "storre_eller_lik"; tok_step = 4
            elif i + 2 < len(toks) and tok in {"er", "equal", "equals"} and n1 == "less" and n2 == "than":
                tok = "mindre_enn"; tok_step = 3
            elif i + 2 < len(toks) and tok in {"er", "equal", "equals"} and n1 == "greater" and n2 == "than":
                tok = "storre_enn"; tok_step = 3
            elif i + 4 < len(toks) and tok in {"less", "greater"} and n1 == "than" and n2 in {"or", "eller"} and n3 in {"equal", "equals", "er"} and n4 == "to":
                tok = "mindre_eller_lik" if tok == "less" else "storre_eller_lik"; tok_step = 5
            elif i + 3 < len(toks) and tok in {"less", "greater"} and n1 == "than" and n2 in {"or", "eller"} and n3 in {"equal", "equals", "er"}:
                tok = "mindre_eller_lik" if tok == "less" else "storre_eller_lik"; tok_step = 4
            elif i + 3 < len(toks) and tok in {"less", "greater"} and n1 in {"or", "eller"} and n2 in {"equal", "equals", "er"} and n3 == "to":
                tok = "mindre_eller_lik" if tok == "less" else "storre_eller_lik"; tok_step = 4
            elif i + 2 < len(toks) and tok in {"less", "greater"} and n1 in {"or", "eller"} and n2 in {"equal", "equals", "er"}:
                tok = "mindre_eller_lik" if tok == "less" else "storre_eller_lik"; tok_step = 3
            elif i + 4 < len(toks) and tok in {"less", "greater"} and n1 == "than" and n2 in {"equal", "equals", "er"} and n3 == "to":
                tok = "mindre_eller_lik" if tok == "less" else "storre_eller_lik"; tok_step = 5
            elif i + 3 < len(toks) and tok in {"less", "greater"} and n1 == "than" and n2 in {"equal", "equals", "er"}:
                tok = "mindre_eller_lik" if tok == "less" else "storre_eller_lik"; tok_step = 4
            elif i + 2 < len(toks) and tok in {"less", "greater"} and n1 == "than":
                tok = "mindre_enn" if tok == "less" else "storre_enn"; tok_step = 2
            elif i + 3 < len(toks) and tok == "mindre" and n1 == "enn" and n2 == "eller" and n3 == "lik":
                tok = "mindre_eller_lik"; tok_step = 4
            elif i + 2 < len(toks) and tok == "mindre" and n1 == "enn" and n2 == "lik":
                tok = "mindre_eller_lik"; tok_step = 3
            elif i + 2 < len(toks) and tok == "mindre" and n1 == "eller" and n2 == "lik":
                tok = "mindre_eller_lik"; tok_step = 3
            elif i + 1 < len(toks) and tok == "mindre" and n1 == "lik":
                tok = "mindre_eller_lik"; tok_step = 2
            elif i + 3 < len(toks) and tok == "storre" and n1 == "enn" and n2 == "eller" and n3 == "lik":
                tok = "storre_eller_lik"; tok_step = 4
            elif i + 2 < len(toks) and tok == "storre" and n1 == "enn" and n2 == "lik":
                tok = "storre_eller_lik"; tok_step = 3
            elif i + 2 < len(toks) and tok == "storre" and n1 == "eller" and n2 == "lik":
                tok = "storre_eller_lik"; tok_step = 3
            elif i + 1 < len(toks) and tok == "storre" and n1 == "lik":
                tok = "storre_eller_lik"; tok_step = 2
            elif i + 1 < len(toks) and tok == "mindre" and n1 == "enn":
                tok = "mindre_enn"; tok_step = 2
            elif i + 1 < len(toks) and tok == "storre" and n1 == "enn":
                tok = "storre_enn"; tok_step = 2
            elif i + 1 < len(toks) and tok in {"pluss", "plus", "legg", "legge", "legges", "plusser", "pluses", "plusses", "plusse", "adder", "addere", "adderer", "adderes", "summer", "summerer", "summeres"} and n1 in {"med", "til", "sammen"}:
                tok = "+"; tok_step = 2
            elif i + 1 < len(toks) and tok in {"minus", "trekk", "trekke", "trekkes", "minuseres", "subtraher", "subtraherer", "subtraheres", "subtrahert"} and n1 in {"med", "fra"}:
                tok = "-"; tok_step = 2
            elif i + 1 < len(toks) and tok in {"gang", "gange", "ganget", "ganger", "ganges", "multipliser", "multiplisere", "multipliserer", "multiplisert", "multipliseres"} and n1 == "med":
                tok = "*"; tok_step = 2
            elif i + 2 < len(toks) and tok in {"dele", "deler", "divider", "dividere", "dividerer"} and n1 == "seg" and n2 in {"med", "pa", "paa"}:
                tok = "/"; tok_step = 3
            elif i + 1 < len(toks) and tok in {"delt", "dele", "deler", "deles", "del", "dividere", "divider", "dividerer", "dividert", "divideres"} and n1 in {"med", "pa", "paa"}:
                tok = "/"; tok_step = 2
            elif i + 1 < len(toks) and tok in {"mod", "modulo", "modul", "modulus", "rest", "resten"} and n1 == "av":
                tok = "%"; tok_step = 2
            elif tok in {"pluss", "plus", "legg", "legge", "legges", "plusser", "pluses", "plusses", "plusse", "adder", "addere", "adderer", "adderes", "summer", "summerer", "summeres", "pluss_med", "plus_med", "plusser_med", "plusses_med", "pluses_med", "plusse_med", "adder_med", "addere_med", "adderer_med", "adderes_med", "summer_med", "summerer_med", "summeres_med", "legg_til", "legge_til", "legges_til", "legg_sammen", "legge_sammen", "legges_sammen"}:
                tok = "+"; tok_step = 1
            elif tok in {"minus", "trekk", "trekke", "trekkes", "minuseres", "subtraher", "subtraherer", "subtraheres", "subtrahert", "trekk_fra", "trekke_fra", "trekkes_fra", "minus_fra", "minuseres_fra", "subtraher_fra", "subtraherer_fra", "subtraheres_fra", "subtrahert_fra", "minus_med", "minuseres_med", "trekk_med", "trekke_med", "trekkes_med", "subtraher_med", "subtraherer_med", "subtraheres_med", "subtrahert_med"}:
                tok = "-"; tok_step = 1
            elif tok in {"gang", "gange", "ganget", "ganger", "ganges", "multipliser", "multiplisere", "multipliserer", "multiplisert", "multipliseres", "gang_med", "ganget_med", "gange_med", "ganger_med", "ganges_med", "multipliser_med", "multiplisere_med", "multipliserer_med", "multiplisert_med", "multipliseres_med"}:
                tok = "*"; tok_step = 1
            elif tok in {"delt", "dele", "deler", "deles", "del", "dividere", "divider", "dividerer", "dividert", "divideres", "delt_pa", "deler_pa", "dele_pa", "deles_pa", "del_pa", "delt_med", "dele_med", "deler_med", "deles_med", "del_med", "dividere_pa", "divider_pa", "dividerer_pa", "dividert_pa", "divideres_pa", "dividere_med", "divider_med", "dividerer_med", "dividert_med", "divideres_med"}:
                tok = "/"; tok_step = 1
            elif tok in {"mod", "modulo", "modul", "modulus", "rest", "resten", "mod_av", "modulo_av", "modul_av", "modulus_av", "rest_av", "resten_av"}:
                tok = "%"; tok_step = 1
            elif tok.startswith(("minus", "trekk", "subtraher")) and tok.endswith(("med", "fra")):
                tok = "-"; tok_step = 1
            elif tok.startswith(("pluss", "plus", "legg", "adder", "summer")) and tok.endswith(("med", "til", "sammen")):
                tok = "+"; tok_step = 1
            elif tok.startswith(("gang", "multipliser")) and tok.endswith("med"):
                tok = "*"; tok_step = 1
            elif tok.startswith(("delt", "del", "dele", "deler", "deles", "divider", "dividere", "dividerer", "dividert", "divideres")) and tok.endswith(("med", "pa", "paa")):
                tok = "/"; tok_step = 1
            elif tok.startswith(("mod", "modulo", "modul", "modulus", "rest", "resten")) and tok.endswith("av"):
                tok = "%"; tok_step = 1
            elif tok in {"lik"}:
                tok = "er"; tok_step = 1
            elif tok in {"ulik", "ikke_lik"}:
                tok = "ikke_er"; tok_step = 1

            if tok == "":
                i += 1
                continue

            is_int = self._call_hot_selfhost_helper("selfhost.compiler.er_heltall_token", [tok])
            if is_int:
                if not forventer_verdi:
                    return f"/* feil: mangler operator før verdi {tok} ved {token_pos(i)} */"
                ops.append("PUSH"); verdier.append(int(tok))
                forventer_verdi = False
                siste_token = i + tok_step - 1
                i += tok_step
                continue

            if tok in {"sann", "usann"}:
                if not forventer_verdi:
                    return f"/* feil: mangler operator før verdi {tok} ved {token_pos(i)} */"
                ops.append("PUSH"); verdier.append(1 if tok == "sann" else 0)
                forventer_verdi = False
                siste_token = i + tok_step - 1
                i += tok_step
                continue

            if forventer_verdi and tok in env:
                ops.append("PUSH"); verdier.append(int(env[tok]))
                forventer_verdi = False
                siste_token = i + tok_step - 1
                i += tok_step
                continue

            if tok == "(":
                if not forventer_verdi:
                    return f"/* feil: mangler operator før ( ved {token_pos(i)} */"
                operatorer.append(tok); operator_pos.append(i)
                siste_token = i + tok_step - 1
                i += tok_step
                continue

            if tok == ")":
                if forventer_verdi:
                    return f"/* feil: mangler verdi før ) ved {token_pos(i)} */"
                fant = False
                while operatorer:
                    top = operatorer.pop(); top_pos = operator_pos.pop()
                    if top == "(":
                        fant = True
                        break
                    if not self._selfhost_emit_operator(ops, verdier, top):
                        return f"/* feil: ukjent operator {top} ved {token_pos(top_pos)} */"
                if not fant:
                    return f"/* feil: ) uten matchende ( ved {token_pos(i)} */"
                forventer_verdi = False
                siste_token = i + tok_step - 1
                i += tok_step
                continue

            if self._selfhost_is_operator_token(tok):
                if forventer_verdi:
                    if tok == "!":
                        operatorer.append("u!"); operator_pos.append(i); siste_token = i; i += tok_step; continue
                    if tok == "ikke":
                        operatorer.append("uikke"); operator_pos.append(i); siste_token = i; i += tok_step; continue
                    if tok == "-":
                        operatorer.append("u-"); operator_pos.append(i); siste_token = i; i += tok_step; continue
                    if tok == "+":
                        siste_token = i; i += tok_step; continue
                    return f"/* feil: mangler verdi før operator {tok} ved {token_pos(i)} */"
                while operatorer:
                    top = operatorer[-1]; top_pos = operator_pos[-1]
                    if top == "(":
                        break
                    if self._selfhost_operator_precedence(top) < self._selfhost_operator_precedence(tok):
                        break
                    operatorer.pop(); operator_pos.pop()
                    if not self._selfhost_emit_operator(ops, verdier, top):
                        return f"/* feil: ukjent operator {top} ved {token_pos(top_pos)} */"
                operatorer.append(tok); operator_pos.append(i)
                forventer_verdi = True
                siste_token = i + tok_step - 1
                i += tok_step
                continue

            return f"/* feil: ukjent token/navn i uttrykk {tok} ved {token_pos(i)} */"

        if forventer_verdi:
            if siste_token >= 0:
                return f"/* feil: uttrykket avsluttes med operator ved {token_pos(siste_token)} */"
            return "/* feil: uttrykket avsluttes med operator */"

        while operatorer:
            top = operatorer.pop(); top_pos = operator_pos.pop()
            if top == "(":
                return f"/* feil: mangler ) i uttrykk ved {token_pos(top_pos)} */"
            if not self._selfhost_emit_operator(ops, verdier, top):
                return f"/* feil: ukjent operator {top} ved {token_pos(top_pos)} */"
        return ""

    def _call_hot_selfhost_helper(self, name: str, args: list[Any]) -> Any:
        if name == "selfhost.compiler.normaliser_norsk_token":
            tok = str(args[0]) if args else ""
            return self._selfhost_token_map.get(tok, tok)
        if name == "selfhost.compiler.op_krever_arg":
            op = str(args[0]) if args else ""
            return op in {"PUSH", "LABEL", "JMP", "JZ", "CALL", "STORE", "LOAD"}
        if name == "selfhost.compiler.op_kjent":
            op = str(args[0]) if args else ""
            return op in {"PUSH", "LABEL", "JMP", "JZ", "CALL", "STORE", "LOAD", "ADD", "SUB", "MUL", "DIV", "MOD", "EQ", "GT", "LT", "AND", "OR", "NOT", "DUP", "POP", "SWAP", "OVER", "PRINT", "HALT", "RET"}
        if name == "selfhost.compiler.er_heltall_token":
            tok = str(args[0]) if args else ""
            try:
                return str(int(tok)) == tok
            except Exception:
                return False
        if name == "selfhost.compiler.operator_til_opcode":
            tok = str(args[0]) if args else ""
            mapping = {
                "+": "ADD", "-": "SUB", "*": "MUL", "/": "DIV", "%": "MOD",
                "==": "EQ", "er": "EQ", "<": "LT", "mindre_enn": "LT",
                ">": "GT", "storre_enn": "GT", "&&": "AND", "og": "AND",
                "samt": "AND", "||": "OR", "eller": "OR", "enten": "OR",
            }
            return mapping.get(tok, "")
        if name == "selfhost.compiler.stack_behov":
            op = str(args[0]) if args else ""
            if op in {"ADD", "SUB", "MUL", "DIV", "MOD", "EQ", "GT", "LT", "AND", "OR", "OVER", "SWAP"}:
                return 2
            if op in {"NOT", "POP", "DUP", "STORE", "PRINT", "JZ"}:
                return 1
            return 0
        if name == "selfhost.compiler.stack_endring":
            op = str(args[0]) if args else ""
            if op in {"PUSH", "LOAD", "DUP", "OVER"}:
                return 1
            if op in {"ADD", "SUB", "MUL", "DIV", "MOD", "EQ", "GT", "LT", "AND", "OR", "POP", "STORE"}:
                return -1
            return 0
        if name == "selfhost.compiler.uttrykk_til_ops_og_verdier_med_miljo":
            tokens = args[0] if len(args) > 0 else []
            navn = args[1] if len(args) > 1 else []
            miljo_verdier = args[2] if len(args) > 2 else []
            ops = args[3] if len(args) > 3 else []
            verdier = args[4] if len(args) > 4 else []
            return self._hot_selfhost_expr_to_ops(tokens, navn, miljo_verdier, ops, verdier)
        if name == "selfhost.compiler.instruksjon_til_tekst":
            op = str(args[0]) if len(args) > 0 else ""
            verdi = int(args[1]) if len(args) > 1 else 0
            if op in {"PUSH", "LABEL", "JMP", "JZ", "CALL", "STORE", "LOAD"}:
                return op + " " + str(verdi)
            return op
        if name == "selfhost.compiler.append_linje":
            kilde = str(args[0]) if len(args) > 0 else ""
            linje = str(args[1]) if len(args) > 1 else ""
            return kilde + linje + "\n"
        if name == "selfhost.compiler.disasm_program":
            ops = list(args[0]) if len(args) > 0 else []
            verdier = list(args[1]) if len(args) > 1 else []
            if len(ops) != len(verdier):
                return "/* feil: ops og verdier må ha samme lengde */"
            out = []
            for i, op in enumerate(ops):
                if op in {"PUSH", "LABEL", "JMP", "JZ", "CALL", "STORE", "LOAD"}:
                    instr = f"{op} {verdier[i]}"
                else:
                    instr = str(op)
                out.append(f"{i}: {instr}")
            return "\n".join(out) + ("\n" if out else "")
        raise BytecodeRuntimeError(f"Ukjent hot selfhost helper: {name}")

    def run(self, entry: str | None = None) -> Any:
        target = entry or self.program.get("entry") or "__main__.start"
        self._ensure_startup_hooks()
        try:
            return self.call_function(target, [])
        finally:
            self._run_shutdown_hooks()

    def call_function(self, name: str, args: list[Any]) -> Any:
        if name.startswith("selfhost.compiler.") and name.rsplit(".", 1)[-1] in {
            "normaliser_norsk_token", "op_krever_arg", "op_kjent", "er_heltall_token", "operator_til_opcode", "stack_behov", "stack_endring", "uttrykk_til_ops_og_verdier_med_miljo", "instruksjon_til_tekst", "append_linje", "disasm_program"
        }:
            return self._call_hot_selfhost_helper(name, args)
        if name in self._memoizable_functions:
            memo_key = (name, repr(args))
            if memo_key in self._memo_cache:
                return self._memo_cache[memo_key]
        else:
            memo_key = None
        if name.startswith("builtin."):
            return self.call_builtin(name.removeprefix("builtin."), args)
        if name == "std.math.pluss":
            return args[0] + args[1]
        if name == "std.math.minus":
            return args[0] - args[1]
        if name in {"vent.timeout", "std.vent.timeout"}:
            if len(args) < 2:
                raise BytecodeRuntimeError("vent.timeout forventer 2 argumenter")
            timeout_ms = max(0, int(args[1]))
            value = args[0]
            if isinstance(value, AsyncValue):
                value = value.value
            return AsyncValue(value, timeout_deadline=time.monotonic() + (timeout_ms / 1000.0))
        if name in {"vent.kanseller", "std.vent.kanseller"}:
            if not args:
                raise BytecodeRuntimeError("vent.kanseller forventer 1 argument")
            value = args[0]
            if isinstance(value, AsyncValue):
                return AsyncValue(value.value, cancelled=True, timeout_deadline=value.timeout_deadline)
            return AsyncValue(value, cancelled=True)
        if name in {"vent.er_kansellert", "std.vent.er_kansellert"}:
            if not args:
                return False
            value = args[0]
            return isinstance(value, AsyncValue) and value.cancelled
        if name in {"vent.er_timeoutet", "std.vent.er_timeoutet"}:
            if not args:
                return False
            value = args[0]
            return isinstance(value, AsyncValue) and value.timeout_deadline is not None and time.monotonic() > value.timeout_deadline
        if name in {"vent.sov", "std.vent.sov"}:
            if not args:
                raise BytecodeRuntimeError("vent.sov forventer 1 argument")
            time.sleep(max(0, int(args[0])) / 1000.0)
            return None
        if name in {"http.request", "std.http.request"}:
            if len(args) < 6:
                raise BytecodeRuntimeError("http.request forventer 6 argumenter")
            return _http_request(args[0], args[1], args[2], args[3], args[4], args[5])
        if name in {"http.get", "std.http.get"}:
            if len(args) < 4:
                raise BytecodeRuntimeError("http.get forventer 4 argumenter")
            return _http_request("GET", args[0], args[1], args[2], None, args[3])
        if name in {"http.post", "std.http.post"}:
            if len(args) < 5:
                raise BytecodeRuntimeError("http.post forventer 5 argumenter")
            return _http_request("POST", args[0], args[2], args[3], args[1], args[4])
        if name in {"http.json_get", "std.http.json_get"}:
            if len(args) < 4:
                raise BytecodeRuntimeError("http.json_get forventer 4 argumenter")
            return json.loads(_http_request("GET", args[0], args[1], args[2], None, args[3]))
        if name in {"http.json_post", "std.http.json_post"}:
            if len(args) < 5:
                raise BytecodeRuntimeError("http.json_post forventer 5 argumenter")
            body_text = json.dumps(args[1], ensure_ascii=False, separators=(",", ":"))
            return json.loads(_http_request("POST", args[0], args[2], args[3], body_text, args[4]))
        if name in {"http.request_full", "std.http.request_full"}:
            if len(args) < 6:
                raise BytecodeRuntimeError("http.request_full forventer 6 argumenter")
            return _http_request_full(args[0], args[1], args[2], args[3], args[4], args[5])
        if name in {"http.response_status", "std.http.response_status"}:
            if not args:
                return 0
            return int(json.loads(str(args[0])).get("status", 0))
        if name in {"http.response_text", "std.http.response_text"}:
            if not args:
                return ""
            return str(json.loads(str(args[0])).get("body", ""))
        if name in {"http.response_json", "std.http.response_json"}:
            if not args:
                return {}
            body = str(json.loads(str(args[0])).get("body", ""))
            return json.loads(body) if body else {}
        if name in {"http.response_header", "std.http.response_header"}:
            if len(args) < 2:
                raise BytecodeRuntimeError("http.response_header forventer 2 argumenter")
            headers = json.loads(str(args[0])).get("headers", {})
            if isinstance(headers, dict):
                return str(headers.get(str(args[1]), ""))
            return ""
        if name in {"web.path_match", "std.web.path_match"}:
            if len(args) < 2:
                return False
            return _match_web_path(args[0], args[1]) is not None
        if name in {"web.path_params", "std.web.path_params"}:
            if len(args) < 2:
                return {}
            return _match_web_path(args[0], args[1]) or {}
        if name in {"web.route_match", "std.web.route_match"}:
            if len(args) < 3:
                return False
            return _match_web_route_spec(args[0], args[1], args[2]) is not None
        if name in {"web.dispatch", "std.web.dispatch"}:
            if len(args) < 3 or not isinstance(args[0], dict):
                return ""
            routes = args[0]
            method = args[1]
            path = args[2]
            for route_name, route_spec in routes.items():
                if _match_web_route_spec(route_spec, method, path) is not None:
                    return str(route_name)
            return ""
        if name in {"web.dispatch_params", "std.web.dispatch_params"}:
            if len(args) < 3 or not isinstance(args[0], dict):
                return {}
            routes = args[0]
            method = args[1]
            path = args[2]
            for route_name, route_spec in routes.items():
                params = _match_web_route_spec(route_spec, method, path)
                if params is not None:
                    return params
            return {}
        if name in {"web.request_context", "std.web.request_context"}:
            if len(args) < 5:
                return _make_web_request_context("", "", {}, {}, "")
            return _make_web_request_context(args[0], args[1], args[2], args[3], args[4])
        if name in {"web.request_id", "std.web.request_id"}:
            if not args or not isinstance(args[0], dict):
                return ""
            return str(args[0].get("request_id", ""))
        if name in {"web.request_method", "std.web.request_method"}:
            if not args or not isinstance(args[0], dict):
                return ""
            return str(args[0].get("metode", ""))
        if name in {"web.request_path", "std.web.request_path"}:
            if not args or not isinstance(args[0], dict):
                return ""
            return str(args[0].get("sti", ""))
        if name in {"web.request_query", "std.web.request_query"}:
            if not args or not isinstance(args[0], dict):
                return {}
            return _decode_json_text_map(args[0].get("query", ""))
        if name in {"web.request_headers", "std.web.request_headers"}:
            if not args or not isinstance(args[0], dict):
                return {}
            return _decode_json_text_map(args[0].get("headers", ""))
        if name in {"web.request_body", "std.web.request_body"}:
            if not args or not isinstance(args[0], dict):
                return ""
            return str(args[0].get("body", ""))
        if name in {"web.request_query_param", "std.web.request_query_param"}:
            if len(args) < 2 or not isinstance(args[0], dict):
                return ""
            query = _decode_json_text_map(args[0].get("query", ""))
            return str(query.get(str(args[1]), ""))
        if name in {"web.request_header", "std.web.request_header"}:
            if len(args) < 2 or not isinstance(args[0], dict):
                return ""
            headers = _decode_json_text_map(args[0].get("headers", ""))
            return str(headers.get(str(args[1]), ""))
        if name in {"web.request_params", "std.web.request_params"}:
            if not args or not isinstance(args[0], dict):
                return {}
            return _decode_json_text_map(args[0].get("params", ""))
        if name in {"web.request_param", "std.web.request_param"}:
            if len(args) < 2 or not isinstance(args[0], dict):
                return ""
            params = _decode_json_text_map(args[0].get("params", ""))
            return str(params.get(str(args[1]), ""))
        if name in {"web.request_param_int", "std.web.request_param_int"}:
            if len(args) < 2 or not isinstance(args[0], dict):
                return 0
            params = _decode_json_text_map(args[0].get("params", ""))
            key = str(args[1])
            if key not in params or str(params.get(key, "")) == "":
                raise BytecodeThrow(_validation_error(f"mangler path-felt '{key}'"))
            return _parse_int_value(params.get(key, ""), key, "path")
        if name in {"web.request_query_required", "std.web.request_query_required"}:
            if len(args) < 2 or not isinstance(args[0], dict):
                return ""
            query = _decode_json_text_map(args[0].get("query", ""))
            key = str(args[1])
            return _parse_required_text(query.get(key, ""), key, "query")
        if name in {"web.request_query_int", "std.web.request_query_int"}:
            if len(args) < 2 or not isinstance(args[0], dict):
                return 0
            query = _decode_json_text_map(args[0].get("query", ""))
            key = str(args[1])
            if key not in query or str(query.get(key, "")) == "":
                raise BytecodeThrow(_validation_error(f"mangler query-felt '{key}'"))
            return _parse_int_value(query.get(key, ""), key, "query")
        if name in {"web.request_json", "std.web.request_json"}:
            if not args or not isinstance(args[0], dict):
                return {}
            return _request_json_object(args[0])
        if name in {"web.request_json_field", "std.web.request_json_field"}:
            if len(args) < 2 or not isinstance(args[0], dict):
                return ""
            data = _request_json_object(args[0])
            key = str(args[1])
            if key not in data or str(data.get(key, "")) == "":
                raise BytecodeThrow(_validation_error(f"mangler felt '{key}'"))
            return str(data.get(key, ""))
        if name in {"web.request_json_field_or", "std.web.request_json_field_or"}:
            if len(args) < 3 or not isinstance(args[0], dict):
                return ""
            data = _request_json_object(args[0])
            key = str(args[1])
            fallback = str(args[2])
            if key not in data or str(data.get(key, "")) == "":
                return fallback
            return str(data.get(key, ""))
        if name in {"web.request_json_field_int", "std.web.request_json_field_int"}:
            if len(args) < 2 or not isinstance(args[0], dict):
                return 0
            data = _request_json_object(args[0])
            key = str(args[1])
            if key not in data or str(data.get(key, "")) == "":
                raise BytecodeThrow(_validation_error(f"mangler felt '{key}'"))
            return _parse_int_value(data.get(key, ""), key, "body")
        if name in {"web.request_json_field_bool", "std.web.request_json_field_bool"}:
            if len(args) < 2 or not isinstance(args[0], dict):
                return False
            data = _request_json_object(args[0])
            key = str(args[1])
            if key not in data or str(data.get(key, "")) == "":
                raise BytecodeThrow(_validation_error(f"mangler felt '{key}'"))
            return _parse_bool_value(data.get(key, ""), key, "body")
        if name in {"web.response_builder", "std.web.response_builder"}:
            if len(args) < 3:
                return _make_web_response(0, {}, "")
            return _make_web_response(args[0], args[1], args[2])
        if name in {"web.response_status", "std.web.response_status"}:
            if not args or not isinstance(args[0], dict):
                return 0
            try:
                return int(str(args[0].get("status", "0")))
            except Exception:
                return 0
        if name in {"web.response_headers", "std.web.response_headers"}:
            if not args or not isinstance(args[0], dict):
                return {}
            return _decode_json_text_map(args[0].get("headers", ""))
        if name in {"web.response_body", "std.web.response_body", "web.response_text", "std.web.response_text"}:
            if not args or not isinstance(args[0], dict):
                return ""
            return str(args[0].get("body", ""))
        if name in {"web.response_json", "std.web.response_json"}:
            if not args or not isinstance(args[0], dict):
                return {}
            body = str(args[0].get("body", ""))
            if not body:
                return {}
            try:
                parsed = json.loads(body)
            except Exception:
                return {}
            if isinstance(parsed, dict):
                return {str(key): _json_scalar_to_text(value) for key, value in parsed.items()}
            return {}
        if name in {"web.response_header", "std.web.response_header"}:
            if len(args) < 2 or not isinstance(args[0], dict):
                return ""
            headers = _decode_json_text_map(args[0].get("headers", ""))
            return str(headers.get(str(args[1]), ""))
        if name in {"web.response_error", "std.web.response_error"}:
            if len(args) < 2:
                return _make_web_response(500, {"content-type": "application/json"}, '{"error":""}')
            body = json.dumps({"error": str(args[1])}, ensure_ascii=False)
            return _make_web_response(args[0], {"content-type": "application/json"}, body)
        if name in {"web.request_middleware", "std.web.request_middleware"}:
            return "request_middleware"
        if name in {"web.response_middleware", "std.web.response_middleware"}:
            return "response_middleware"
        if name in {"web.error_middleware", "std.web.error_middleware"}:
            return "error_middleware"
        if name in {"web.startup_hook", "std.web.startup_hook"}:
            return "startup_hook"
        if name in {"web.shutdown_hook", "std.web.shutdown_hook"}:
            return "shutdown_hook"
        if name in {"web.startup", "std.web.startup"}:
            return self._ensure_startup_hooks()
        if name in {"web.shutdown", "std.web.shutdown"}:
            return self._run_shutdown_hooks()
        if name in {"web.response_file", "std.web.response_file"}:
            if len(args) < 2:
                raise BytecodeRuntimeError("web.response_file forventer 2 argumenter")
            path = Path(str(args[0])).expanduser()
            try:
                body = path.read_text(encoding="utf-8")
            except OSError:
                raise BytecodeThrow(f"IOFeil: kunne ikke lese fil {path}")
            return _make_web_response(200, {"content-type": str(args[1])}, body)
        if name in {"web.openapi_json", "std.web.openapi_json"}:
            title = str(args[0]) if args else "Norscode API"
            version = str(args[1]) if len(args) > 1 else "1.0.0"
            return self._build_openapi_document(title, version)
        if name in {"web.docs_html", "std.web.docs_html"}:
            title = str(args[0]) if args else "Norscode API"
            version = str(args[1]) if len(args) > 1 else "1.0.0"
            return self._build_docs_html(title, version)
        if name in {"web.route", "std.web.route"}:
            if not args:
                return ""
            return str(args[0])
        if name in {"web.router", "std.web.router", "web.subrouter", "std.web.subrouter"}:
            if not args:
                return ""
            return str(args[0])
        if name in {"web.guard", "std.web.guard"}:
            return "guard"
        if name in {"web.dependency", "std.web.dependency"}:
            if not args:
                return ""
            return str(args[0])
        if name in {"web.use_dependency", "std.web.use_dependency"}:
            if not args:
                return ""
            return str(args[0])
        if name in {"web.use_guard", "std.web.use_guard"}:
            if not args:
                return ""
            return str(args[0])
        if name in {"web.request_dependency", "std.web.request_dependency"}:
            if len(args) < 2:
                return {}
            return self._resolve_dependency_value(args[0], str(args[1]))
        if name in {"web.handle_request", "std.web.handle_request"}:
            if not args or not isinstance(args[0], dict):
                return _make_web_response(404, {"content-type": "application/json"}, json.dumps({"error": "Ikke funnet"}, ensure_ascii=False))
            self._ensure_startup_hooks()
            try:
                current_ctx = self._apply_request_middlewares(args[0])
                found = None
                method_text = _normalize_web_method(current_ctx.get("metode", ""))
                path_text = str(current_ctx.get("sti", ""))
                path_mismatch = False
                for handler in self.route_handlers:
                    spec = handler.get("spec", "")
                    route_method, route_path = _split_web_route_spec(spec)
                    route_params = _match_web_path(route_path, path_text)
                    if route_params is None:
                        continue
                    if route_method and route_method != method_text:
                        path_mismatch = True
                        continue
                    if _route_is_exact(route_path):
                        found = (handler, route_params)
                        break
                    if found is None:
                        found = (handler, route_params)
                if found is None:
                    if path_mismatch:
                        response = _make_web_response(405, {"content-type": "application/json"}, json.dumps({"error": "Metode ikke tillatt"}, ensure_ascii=False))
                        return self._apply_error_middlewares(response)
                    response = _make_web_response(404, {"content-type": "application/json"}, json.dumps({"error": "Ikke funnet"}, ensure_ascii=False))
                    return self._apply_error_middlewares(response)
                handler, params = found
                ctx = _make_route_context(current_ctx, params)
                deps = [self._resolve_dependency_value(ctx, dep_name) for dep_name in handler.get("deps", [])]
                for guard_name in handler.get("guards", []):
                    guard_fn = self.guard_providers.get(guard_name, guard_name)
                    guard_result = self.call_function(guard_fn, [ctx])
                    if not bool(guard_result):
                        return self._apply_error_middlewares(_guard_response())
                result = self.call_function(handler["function"], [ctx] + deps)
                if isinstance(result, dict):
                    return self._apply_response_middlewares(result)
                return result
            except BytecodeThrow as exc:
                response = _make_web_response(500, {"content-type": "application/json"}, json.dumps({"error": str(exc)}, ensure_ascii=False))
                return self._apply_error_middlewares(response)
            except Exception as exc:
                response = _make_web_response(500, {"content-type": "application/json"}, json.dumps({"error": str(exc)}, ensure_ascii=False))
                return self._apply_error_middlewares(response)
        fn = self.functions.get(name)
        if fn is None:
            short_name = name.rsplit('.', 1)[-1]
            builtin_like = {
                "assert", "assert_eq", "assert_ne", "assert_starter_med", "assert_slutter_med", "assert_inneholder", "skriv", "lengde",
                "tekst_fra_heltall", "tekst_fra_bool", "heltall_fra_tekst",
                "tekst_starter_med", "tekst_slutter_med", "tekst_inneholder", "tekst_trim",
                "del_ord", "tokeniser_enkel", "tokeniser_uttrykk", "les_input",
                "legg_til", "pop_siste", "fjern_indeks", "sett_inn",
                "har_nokkel", "fjern_nokkel", "json_parse", "json_stringify",
                "slice",
                "fil_les", "fil_skriv", "fil_append", "fil_finnes",
                "sti_join", "sti_basename", "sti_dirname", "sti_exists", "sti_stem",
                "miljo_hent", "miljo_finnes", "miljo_sett",
                "route", "handle_request", "request_params", "request_param", "request_param_int",
                "request_query_required", "request_query_int", "request_json", "request_json_field",
                "request_json_field_or", "request_json_field_int", "request_json_field_bool",
                "openapi_json", "docs_html",
            }
            if short_name in builtin_like:
                return self.call_builtin(short_name, args)
            raise BytecodeRuntimeError(f"Ukjent funksjon: {name}")

        params = fn.get("params", [])
        locals_: dict[str, Any] = {param: value for param, value in zip(params, args)}
        code = fn.get("code", [])
        labels = {instr[1]: idx for idx, instr in enumerate(code) if instr[0] == "LABEL"}
        stack: list[Any] = []
        ip = 0
        exception_frame_base = len(self._exception_stack)
        self._call_stack.append(name)
        try:
            while ip < len(code):
                self.steps += 1
                if self.steps > self.max_steps:
                    raise BytecodeRuntimeError(f"Maks steg overskredet ({self.max_steps}) i {name} ved ip={ip}")
                instr = code[ip]
                op = instr[0]
                local_preview = {key: locals_[key] for key in list(locals_.keys())[:6]}
                if self._should_trace_function(name):
                    self._log(f"step={self.steps} fn={name} ip={ip} op={instr!r} stack={stack[-4:]} locals={local_preview}")
                if self.repeat_limit > 0:
                    state_key = (name, ip, tuple(instr), repr(stack[-4:]), repr(local_preview))
                    if state_key == self._repeat_state_key:
                        self._repeat_state_count += 1
                    else:
                        self._repeat_state_key = state_key
                        self._repeat_state_count = 1
                    if self._repeat_state_count > self.repeat_limit:
                        raise BytecodeRuntimeError(
                            f"Mulig fastlås/løkke oppdaget i {name} ved ip={ip} op={instr!r} "
                            f"(samme tilstand > {self.repeat_limit} ganger)"
                        )
                if op == "LABEL":
                    ip += 1
                    continue
                if op == "PUSH_CONST":
                    stack.append(instr[1])
                elif op == "LOAD_NAME":
                    name2 = instr[1]
                    if name2 not in locals_:
                        raise BytecodeRuntimeError(f"Ukjent variabel: {name2}")
                    stack.append(locals_[name2])
                elif op == "STORE_NAME":
                    locals_[instr[1]] = stack.pop()
                elif op == "POP":
                    if stack:
                        stack.pop()
                elif op == "BUILD_LIST":
                    n = instr[1]
                    items = stack[-n:] if n else []
                    if n:
                        del stack[-n:]
                    stack.append(items)
                elif op == "BUILD_MAP":
                    n = instr[1]
                    raw_items = stack[-(n * 2):] if n else []
                    if n:
                        del stack[-(n * 2):]
                    out = {}
                    for i in range(0, len(raw_items), 2):
                        out[raw_items[i]] = raw_items[i + 1]
                    stack.append(out)
                elif op == "INDEX_GET":
                    idx = stack.pop(); lst = stack.pop(); stack.append(lst[idx])
                elif op == "INDEX_SET":
                    value = stack.pop(); idx = stack.pop(); lst = stack.pop(); lst[idx] = value
                elif op == "UNARY_NEG":
                    stack.append(-stack.pop())
                elif op == "UNARY_NOT":
                    stack.append(not stack.pop())
                elif op == "BINARY_ADD":
                    b = stack.pop(); a = stack.pop(); stack.append(a + b)
                elif op == "BINARY_SUB":
                    b = stack.pop(); a = stack.pop(); stack.append(a - b)
                elif op == "BINARY_MUL":
                    b = stack.pop(); a = stack.pop(); stack.append(a * b)
                elif op == "BINARY_DIV":
                    b = stack.pop(); a = stack.pop(); stack.append(a / b)
                elif op == "COMPARE_EQ":
                    b = stack.pop(); a = stack.pop(); stack.append(a == b)
                elif op == "COMPARE_NE":
                    b = stack.pop(); a = stack.pop(); stack.append(a != b)
                elif op == "COMPARE_GT":
                    b = stack.pop(); a = stack.pop(); stack.append(a > b)
                elif op == "COMPARE_LT":
                    b = stack.pop(); a = stack.pop(); stack.append(a < b)
                elif op == "COMPARE_GE":
                    b = stack.pop(); a = stack.pop(); stack.append(a >= b)
                elif op == "COMPARE_LE":
                    b = stack.pop(); a = stack.pop(); stack.append(a <= b)
                elif op == "BINARY_AND":
                    b = stack.pop(); a = stack.pop(); stack.append(bool(a) and bool(b))
                elif op == "BINARY_OR":
                    b = stack.pop(); a = stack.pop(); stack.append(bool(a) or bool(b))
                elif op == "TRY_BEGIN":
                    self._push_try_frame(instr[1])
                elif op == "TRY_END":
                    self._pop_try_frame()
                elif op == "LOAD_EXCEPTION":
                    stack.append(self._exception_value)
                elif op == "BUILD_LAMBDA":
                    lambda_name = instr[1]
                    capture_names = list(instr[2]) if len(instr) > 2 else []
                    capture_values = []
                    for _ in range(len(capture_names)):
                        capture_values.append(stack.pop())
                    capture_values.reverse()
                    stack.append(BytecodeLambdaValue(lambda_name, capture_names, capture_values))
                elif op == "THROW":
                    exc = stack.pop() if stack else None
                    handler_ip = self._handle_throw(exc, labels)
                    if handler_ip is None:
                        raise BytecodeThrow(exc)
                    ip = handler_ip
                    continue
                elif op == "CALL":
                    target, argc = instr[1], instr[2]
                    call_args = stack[-argc:] if argc else []
                    if argc:
                        del stack[-argc:]
                    try:
                        stack.append(self.call_function(target, call_args))
                    except BytecodeThrow as exc:
                        handler_ip = self._handle_throw(exc.value, labels)
                        if handler_ip is None:
                            raise
                        ip = handler_ip
                        continue
                elif op == "CALL_VALUE":
                    argc = instr[1]
                    call_args = stack[-argc:] if argc else []
                    if argc:
                        del stack[-argc:]
                    callable_value = stack.pop() if stack else None
                    try:
                        stack.append(self.call_value(callable_value, call_args))
                    except BytecodeThrow as exc:
                        handler_ip = self._handle_throw(exc.value, labels)
                        if handler_ip is None:
                            raise
                        ip = handler_ip
                        continue
                elif op == "JUMP":
                    ip = labels[instr[1]]
                    continue
                elif op == "JUMP_IF_FALSE":
                    value = stack.pop()
                    if not value:
                        ip = labels[instr[1]]
                        continue
                elif op == "RETURN":
                    result = stack.pop() if stack else None
                    if fn.get("is_async"):
                        result = AsyncValue(result)
                    if memo_key is not None:
                        self._memo_cache[memo_key] = result
                    return result
                else:
                    raise BytecodeRuntimeError(f"Ukjent opcode: {op}")
                ip += 1
            if memo_key is not None:
                self._memo_cache[memo_key] = None
        except BytecodeThrow as exc:
            exc.call_stack.append(name)
            raise
        finally:
            self._call_stack.pop()
            while len(self._exception_stack) > exception_frame_base:
                self._exception_stack.pop()

    def call_value(self, value: Any, args: list[Any]) -> Any:
        if isinstance(value, BytecodeLambdaValue):
            closure_args = list(value.capture_values) + list(args)
            return self.call_function(value.function_name, closure_args)
        raise BytecodeRuntimeError(f"Kan ikke kalle verdi av type {type(value).__name__}")

    def call_builtin(self, name: str, args: list[Any]) -> Any:
        if name == "await_value":
            if not args:
                return None
            value = args[0]
            if isinstance(value, AsyncValue):
                if value.cancelled:
                    raise BytecodeThrow("AvbruttFeil: future ble kansellert")
                if value.timeout_deadline is not None and time.monotonic() > value.timeout_deadline:
                    raise BytecodeThrow("TimeoutFeil: future gikk ut på tid")
                return value.value
            return value
        if name == "skriv":
            text = " ".join(str(x) for x in args)
            print(text)
            self.output.append(text)
            return None
        if name == "assert":
            if not args[0]:
                raise AssertionError("Assert feilet")
            return None
        if name == "assert_eq":
            if args[0] != args[1]:
                raise AssertionError(f"assert_eq feilet: {args[0]!r} != {args[1]!r}")
            return None
        if name == "assert_ne":
            if args[0] == args[1]:
                raise AssertionError(f"assert_ne feilet: {args[0]!r} == {args[1]!r}")
            return None
        if name == "assert_starter_med":
            if len(args) < 2 or not str(args[0]).startswith(str(args[1])):
                raise AssertionError(f"assert_starter_med feilet: {args[0]!r} starter ikke med {args[1]!r}")
            return None
        if name == "assert_slutter_med":
            if len(args) < 2 or not str(args[0]).endswith(str(args[1])):
                raise AssertionError(f"assert_slutter_med feilet: {args[0]!r} slutter ikke med {args[1]!r}")
            return None
        if name == "assert_inneholder":
            if len(args) < 2 or str(args[1]) not in str(args[0]):
                raise AssertionError(f"assert_inneholder feilet: {args[0]!r} inneholder ikke {args[1]!r}")
            return None
        if name == "lengde":
            return len(args[0])
        if name == "har_nokkel":
            if len(args) != 2:
                raise BytecodeRuntimeError("har_nokkel forventer 2 argumenter")
            return args[1] in args[0]
        if name == "fjern_nokkel":
            if len(args) != 2:
                raise BytecodeRuntimeError("fjern_nokkel forventer 2 argumenter")
            mapping = args[0]
            key = args[1]
            if isinstance(mapping, dict):
                mapping.pop(key, None)
                return len(mapping)
            raise BytecodeRuntimeError("fjern_nokkel krever ordbok og tekstnøkkel")
        if name == "json_parse":
            payload = str(args[0]) if args else ""
            try:
                parsed = json.loads(payload)
            except Exception as exc:
                raise BytecodeRuntimeError(f"json_parse feilet: {exc}") from exc

            if isinstance(parsed, list):
                return {str(i): self._json_scalar_to_text(v) for i, v in enumerate(parsed)}
            if isinstance(parsed, dict):
                return {str(key): self._json_scalar_to_text(value) for key, value in parsed.items()}
            raise BytecodeRuntimeError("json_parse forventer et JSON-objekt eller en JSON-liste")

        if name == "json_stringify":
            map_value = args[0]
            if map_value is None:
                map_value = {}
            if not isinstance(map_value, dict):
                raise BytecodeRuntimeError("json_stringify forventer en ordbok")
            try:
                pieces = []
                for key, value in map_value.items():
                    json_key = json.dumps(str(key), ensure_ascii=False, separators=(",", ":"))
                    serialized_value = self._json_serialize_value(value)
                    pieces.append(f"{json_key}:{serialized_value}")
                return "{" + ",".join(pieces) + "}"
            except Exception as exc:
                raise BytecodeRuntimeError(f"json_stringify feilet: {exc}") from exc
        if name == "slice":
            if len(args) < 3:
                raise BytecodeRuntimeError("slice forventer 3 argumenter")
            target, start, end = args[0], args[1], args[2]
            start_idx = None if start is None else int(start)
            end_idx = None if end is None else int(end)
            if isinstance(target, str):
                return target[start_idx:end_idx]
            if isinstance(target, list):
                return target[start_idx:end_idx]
            raise BytecodeRuntimeError("Slicing kan bare brukes på tekst og lister")
        if name == "tekst_fra_heltall":
            return str(args[0])
        if name == "tekst_fra_bool":
            return "sann" if args[0] else "usann"
        if name == "heltall_fra_tekst":
            try:
                return int(str(args[0]).strip())
            except Exception:
                return 0
        if name == "tekst_starter_med":
            return str(args[0]).startswith(str(args[1])) if len(args) >= 2 else False
        if name == "tekst_slutter_med":
            return str(args[0]).endswith(str(args[1])) if len(args) >= 2 else False
        if name == "tekst_inneholder":
            return str(args[1]) in str(args[0]) if len(args) >= 2 else False
        if name == "tekst_trim":
            return str(args[0]).strip() if args else ""
        if name == "sti_join":
            parts = [str(part) for part in args if str(part)]
            if not parts:
                return ""
            out = parts[0]
            for part in parts[1:]:
                if out.endswith("/"):
                    out = out.rstrip("/") + "/" + part.lstrip("/")
                elif out.endswith("\\"):
                    out = out.rstrip("\\") + "/" + part.lstrip("/\\")
                else:
                    out = out.rstrip("/") + "/" + part.lstrip("/\\")
            return out
        if name == "sti_basename":
            text = str(args[0]) if args else ""
            text = text.rstrip("/\\")
            if not text:
                return ""
            return re.split(r"[\\/]", text)[-1]
        if name == "sti_dirname":
            text = str(args[0]) if args else ""
            text = text.rstrip("/\\")
            if not text:
                return ""
            parts = re.split(r"[\\/]", text)
            if len(parts) <= 1:
                return ""
            return "/".join(parts[:-1])
        if name == "sti_exists":
            return Path(str(args[0])).expanduser().exists() if args else False
        if name == "sti_stem":
            text = str(args[0]) if args else ""
            base = re.split(r"[\\/]", text.rstrip("/\\"))[-1] if text else ""
            if "." not in base:
                return base
            return base.rsplit(".", 1)[0]
        if name == "fil_les":
            path = Path(str(args[0])).expanduser() if args else Path("")
            try:
                return path.read_text(encoding="utf-8")
            except OSError:
                raise BytecodeThrow(f"IOFeil: kunne ikke lese fil {path}")
        if name == "fil_skriv":
            if len(args) < 2:
                raise BytecodeRuntimeError("fil_skriv forventer 2 argumenter")
            path = Path(str(args[0])).expanduser()
            text = str(args[1])
            try:
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(text, encoding="utf-8")
            except OSError:
                raise BytecodeThrow(f"IOFeil: kunne ikke skrive fil {path}")
            return text
        if name == "fil_append":
            if len(args) < 2:
                raise BytecodeRuntimeError("fil_append forventer 2 argumenter")
            path = Path(str(args[0])).expanduser()
            text = str(args[1])
            try:
                path.parent.mkdir(parents=True, exist_ok=True)
                with path.open("a", encoding="utf-8") as f:
                    f.write(text)
            except OSError:
                raise BytecodeThrow(f"IOFeil: kunne ikke legge til i fil {path}")
            return text
        if name == "fil_finnes":
            return Path(str(args[0])).expanduser().exists() if args else False
        if name == "miljo_hent":
            key = str(args[0]) if args else ""
            return os.environ.get(key, "")
        if name == "miljo_finnes":
            key = str(args[0]) if args else ""
            return key in os.environ
        if name == "miljo_sett":
            if len(args) < 2:
                raise BytecodeRuntimeError("miljo_sett forventer 2 argumenter")
            key = str(args[0])
            value = str(args[1])
            os.environ[key] = value
            return value
        if name == "del_ord":
            return str(args[0]).split()
        if name == "tokeniser_enkel":
            text = str(args[0])
            tokens = []
            current = []
            in_comment = False
            for ch in text:
                if in_comment:
                    if ch == "\n":
                        in_comment = False
                    continue
                if ch == '#':
                    if current:
                        tokens.append(''.join(current))
                        current = []
                    in_comment = True
                    continue
                if ch.isalnum() or ch in '_-':
                    current.append(ch)
                else:
                    if current:
                        tokens.append(''.join(current))
                        current = []
            if current:
                tokens.append(''.join(current))
            return tokens
        if name == "tokeniser_uttrykk":
            text = str(args[0])
            token_re = re.compile(r'<=>|<->|=>|->|<-|&&|\|\||\+=|-=|\*=|/=|%=|==|!=|<=|>=|<>|[=!(){}\[\],.:;+\-*/%<>]|"[^"\\]*(?:\\.[^"\\]*)*"|[A-Za-zÆØÅæøå_][A-Za-zÆØÅæøå0-9_]*|\d+|\S')
            return [m.group(0) for m in token_re.finditer(text) if m.group(0).strip()]
        if name == "les_input":
            return ""
        if name == "legg_til":
            args[0].append(args[1])
            return None
        if name == "pop_siste":
            if not args[0]:
                return "" if isinstance(args[0], list) and any(isinstance(x, str) for x in args[0]) else 0
            return args[0].pop()
        if name == "fjern_indeks":
            if args[1] < 0 or args[1] >= len(args[0]):
                return len(args[0])
            args[0].pop(args[1])
            return len(args[0])
        if name == "sett_inn":
            lst = args[0]
            idx = int(args[1])
            value = args[2]
            if idx < 0:
                idx = 0
            if idx < len(lst):
                lst[idx] = value
            elif idx == len(lst):
                lst.append(value)
            else:
                while len(lst) < idx:
                    lst.append(0 if not lst or not isinstance(lst[0], str) else "")
                lst.append(value)
            return None
        raise BytecodeRuntimeError(f"Ukjent builtin: {name}")

    def _json_scalar_to_text(self, value):
        if value is None:
            return ""
        if isinstance(value, (dict, list)):
            return json.dumps(value, ensure_ascii=False, separators=(",", ":"))
        if isinstance(value, bool):
            return "true" if value else "false"
        if isinstance(value, (int, float)):
            return json.dumps(value, ensure_ascii=False)
        return str(value)

    def _json_serialize_value(self, raw_value):
        if raw_value is None:
            return "null"
        value = str(raw_value)
        if value in {"", "true", "false", "null"}:
            return value
        if (value.startswith("[") and value.endswith("]")) or (value.startswith("{") and value.endswith("}")):
            return value
        if self._json_is_number_like(value):
            return value
        return json.dumps(value, ensure_ascii=False, separators=(",", ":"))

    @staticmethod
    def _json_is_number_like(value: str) -> bool:
        try:
            float(value)
            return True
        except Exception:
            return False


def compile_program_to_bytecode(program, alias_map: dict[str, str] | None = None) -> dict[str, Any]:
    compiler = BytecodeCompiler(alias_map=alias_map or {})
    return compiler.compile_program(program)


def compile_source_to_bytecode(source_file: str) -> tuple[Path, dict[str, Any]]:
    source_path, program, alias_map = load_source_as_program(source_file)
    payload = compile_program_to_bytecode(program, alias_map=alias_map)
    return source_path, payload


def compile_ast_file_to_bytecode(ast_file: str) -> tuple[Path, dict[str, Any]]:
    ast_path = Path(ast_file).expanduser().resolve()
    program, alias_map = read_ast(str(ast_path))
    payload = compile_program_to_bytecode(program, alias_map=alias_map)
    return ast_path, payload


def default_output_path(source_path: Path) -> Path:
    return source_path.with_suffix('.ncb.json')


def build_command(source_file: str | None = None, output: str | None = None, ast_file: str | None = None) -> Path:
    if source_file:
        source_path, payload = compile_source_to_bytecode(source_file)
    elif ast_file:
        source_path, payload = compile_ast_file_to_bytecode(ast_file)
    else:
        raise RuntimeError("Mangler kildefil eller AST-fil")
    out_path = Path(output).expanduser().resolve() if output else default_output_path(source_path)
    out_path.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding='utf-8')
    return out_path


def run_command(source_file: str | None = None, bytecode_file: str | None = None, ast_file: str | None = None) -> Any:
    if source_file:
        _source_path, payload = compile_source_to_bytecode(source_file)
    elif ast_file:
        _ast_path, payload = compile_ast_file_to_bytecode(ast_file)
    elif bytecode_file:
        payload = json.loads(Path(bytecode_file).expanduser().read_text(encoding='utf-8'))
    else:
        raise RuntimeError("Mangler kildefil, AST-fil eller bytecode-fil")
    vm = BytecodeVM(payload)
    return vm.run()


def main():
    parser = argparse.ArgumentParser(prog='python -m compiler.bytecode_backend', description='AST -> bytecode backend (v17)')
    sub = parser.add_subparsers(dest='cmd')

    build = sub.add_parser('build', help='Bygg .no eller .nast.json til .ncb.json')
    build.add_argument('file')
    build.add_argument('--output')
    build.add_argument('--ast', action='store_true', help='Tolker input som .nast.json i stedet for .no')

    run = sub.add_parser('run', help='Kjør fra .no, .nast.json eller .ncb.json')
    run.add_argument('file')
    run.add_argument('--bytecode', action='store_true', help='Tolker input som .ncb.json i stedet for .no')
    run.add_argument('--ast', action='store_true', help='Tolker input som .nast.json i stedet for .no')

    args = parser.parse_args()
    if args.cmd == 'build':
        if args.ast:
            path = build_command(ast_file=args.file, output=args.output)
        else:
            path = build_command(source_file=args.file, output=args.output)
        print(path)
    elif args.cmd == 'run':
        if args.bytecode:
            result = run_command(bytecode_file=args.file)
        elif args.ast:
            result = run_command(ast_file=args.file)
        else:
            result = run_command(source_file=args.file)
        if result is not None:
            print(result)
    else:
        parser.print_help()


if __name__ == '__main__':
    main()
