

from __future__ import annotations

import os
import posixpath
import json
import re
import time
import urllib.error
import urllib.parse
import urllib.request
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from .ast_nodes import (
    BinOpNode,
    BlockNode,
    BoolNode,
    CallNode,
    ExprStmtNode,
    ThrowNode,
    TryCatchNode,
    IfExprNode,
    FunctionNode,
    MatchNode,
    LambdaNode,
    ModuleCallNode,
    NumberNode,
    ProgramNode,
    ReturnNode,
    StringNode,
    ListLiteralNode,
    SliceNode,
    AwaitNode,
    UnaryOpNode,
    ListComprehensionNode,
    VarAccessNode,
    VarDeclareNode,
    VarSetNode,
)


class ReturnSignal(Exception):
    def __init__(self, value: Any):
        self.value = value
        super().__init__()


class ThrowSignal(Exception):
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
class UserFunction:
    node: FunctionNode


@dataclass
class LambdaValue:
    params: list[Any]
    body: Any
    captured_scopes: list[dict[str, Any]]
    display_name: str = "<lambda>"


@dataclass
class AsyncValue:
    value: Any
    cancelled: bool = False
    timeout_deadline: float | None = None


class Interpreter:
    def __init__(self):
        self.globals: dict[str, Any] = {}
        self.functions: dict[str, UserFunction] = {}
        self.modules: dict[str, Any] = {}
        self.scopes: list[dict[str, Any]] = [self.globals]
        self.call_stack: list[str] = []
        self.route_handlers: list[dict[str, Any]] = []
        self.dependency_providers: dict[str, str] = {}
        self.guard_providers: dict[str, str] = {}
        self.request_middlewares: list[str] = []
        self.response_middlewares: list[str] = []
        self.error_middlewares: list[str] = []
        self.startup_hooks: list[str] = []
        self.shutdown_hooks: list[str] = []
        self._startup_ran = False
        self._shutdown_ran = False
        self._request_counter = 0
        self._db_connections: dict[str, Any] = {}
        self._db_pools: dict[str, dict[str, Any]] = {}
        self._db_pooled_handles: dict[str, str] = {}
        self._db_next_handle = 1
        self._db_next_pool_handle = 1

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
        if type_name == "heltall":
            return {"type": "integer"}
        if type_name == "bool":
            return {"type": "boolean"}
        if type_name == "tekst":
            return {"type": "string"}
        if type_name == "liste_heltall":
            return {"type": "array", "items": {"type": "integer"}}
        if type_name == "liste_tekst":
            return {"type": "array", "items": {"type": "string"}}
        if type_name == "ordbok_heltall":
            return {"type": "object", "additionalProperties": {"type": "integer"}}
        if type_name == "ordbok_text":
            return {"type": "object", "additionalProperties": {"type": "string"}}
        if type_name == "ordbok_tekst":
            return {"type": "object", "additionalProperties": {"type": "string"}}
        if type_name == "ordbok_bool":
            return {"type": "object", "additionalProperties": {"type": "boolean"}}
        if type_name.startswith("liste_"):
            inner = type_name[len("liste_") :]
            return {"type": "array", "items": self._schema_for_type(inner)}
        if type_name.startswith("ordbok_"):
            inner = type_name[len("ordbok_") :]
            return {"type": "object", "additionalProperties": self._schema_for_type(inner)}
        return {"type": "string"}

    def _sample_for_type(self, type_name: str) -> Any:
        if type_name == "heltall":
            return 1
        if type_name == "bool":
            return True
        if type_name == "tekst":
            return "eksempel"
        if type_name == "liste_heltall":
            return [1]
        if type_name == "liste_tekst":
            return ["eksempel"]
        if type_name == "ordbok_heltall":
            return {"verdi": 1}
        if type_name == "ordbok_bool":
            return {"verdi": True}
        if type_name.startswith("liste_"):
            inner = type_name[len("liste_") :]
            return [self._sample_for_type(inner)]
        if type_name.startswith("ordbok_"):
            inner = type_name[len("ordbok_") :]
            return {"verdi": self._sample_for_type(inner)}
        return "eksempel"

    def _function_uses_bearer_auth(self, fn: Any) -> bool:
        for node in self._walk_ast_nodes(getattr(fn, "body", None)):
            if not isinstance(node, ModuleCallNode):
                continue
            module_name = getattr(node, "module_name", "")
            func_name = getattr(node, "func_name", "")
            if module_name in {"web", "std.web"} and func_name in {"require_bearer", "bearer_token", "auth_header"}:
                return True
        return False

    def _route_docs_from_function(self, fn: Any, spec: str, auth_guards: set[str] | None = None) -> dict[str, Any]:
        method, route_path = self._split_web_route_spec(spec)
        path_params: list[dict[str, Any]] = []
        query_params: list[dict[str, Any]] = []
        body_fields: list[dict[str, Any]] = []
        seen_path: set[str] = set()
        seen_query: set[str] = set()
        seen_body: set[str] = set()
        error_responses: list[dict[str, Any]] = []
        auth_guards = auth_guards or set()
        auth_required = False
        for node in self._walk_ast_nodes(getattr(fn, "body", None)):
            if not isinstance(node, ModuleCallNode):
                continue
            module_name = getattr(node, "module_name", "")
            func_name = getattr(node, "func_name", "")
            args = getattr(node, "args", []) or []
            if module_name not in {"web", "std.web"}:
                continue
            if func_name in {"require_bearer", "bearer_token", "auth_header"}:
                auth_required = True
            if func_name == "use_guard" and args and isinstance(args[0], StringNode) and args[0].value in auth_guards:
                auth_required = True
            if func_name == "response_error" and args:
                status_value = getattr(args[0], "value", None) if isinstance(args[0], NumberNode) else None
                if status_value is not None:
                    error_responses.append({
                        "status": int(status_value),
                        "message": str(getattr(args[1], "value", "")) if len(args) > 1 and isinstance(args[1], StringNode) else "",
                    })
            if len(args) < 2 or not isinstance(args[1], StringNode):
                continue
            key = args[1].value
            if func_name == "request_param":
                if key not in seen_path:
                    path_params.append({"name": key, "type": "string"})
                    seen_path.add(key)
            elif func_name == "request_param_int":
                if key not in seen_path:
                    path_params.append({"name": key, "type": "integer"})
                    seen_path.add(key)
            elif func_name == "request_query_required":
                if key not in seen_query:
                    query_params.append({"name": key, "type": "string", "required": True})
                    seen_query.add(key)
            elif func_name == "request_query_int":
                if key not in seen_query:
                    query_params.append({"name": key, "type": "integer", "required": True})
                    seen_query.add(key)
            elif func_name == "request_query_param":
                if key not in seen_query:
                    query_params.append({"name": key, "type": "string", "required": False})
                    seen_query.add(key)
            elif func_name == "request_json_field":
                if key not in seen_body:
                    body_fields.append({"name": key, "type": "string", "required": True})
                    seen_body.add(key)
            elif func_name == "request_json_field_or":
                if key not in seen_body:
                    body_fields.append({"name": key, "type": "string", "required": False})
                    seen_body.add(key)
            elif func_name == "request_json_field_int":
                if key not in seen_body:
                    body_fields.append({"name": key, "type": "integer", "required": True})
                    seen_body.add(key)
            elif func_name == "request_json_field_bool":
                if key not in seen_body:
                    body_fields.append({"name": key, "type": "boolean", "required": True})
                    seen_body.add(key)

        path_template = re.sub(r"\{([^}:]+)(?::[^}]+)?\}", r"{\1}", route_path)
        request_example: dict[str, Any] = {"method": method or "GET", "path": path_template}
        if query_params:
            request_example["query"] = {
                item["name"]: self._sample_for_type("heltall" if item["type"] == "integer" else "tekst")
                for item in query_params
            }
        if body_fields:
            request_example["body"] = {
                item["name"]: self._sample_for_type("heltall" if item["type"] == "integer" else "bool" if item["type"] == "boolean" else "tekst")
                for item in body_fields
            }
        response_type = getattr(fn, "return_type", TYPE_TEXT)
        operation = {
            "function": getattr(fn, "name", ""),
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
            "error_responses": error_responses,
        }
        if auth_required:
            operation["security"] = [{"bearerAuth": []}]
            operation["responses"]["401"] = {"description": "Uautorisert"}
            operation["responses"]["403"] = {"description": "Forbudt"}
        return operation

    def _build_openapi_document(self, title: str, version: str) -> str:
        paths: dict[str, Any] = {}
        has_bearer_auth = False
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
            if handler.get("security"):
                operation["security"] = handler.get("security")
                operation["responses"]["401"] = {"description": "Uautorisert"}
                operation["responses"]["403"] = {"description": "Forbudt"}
                has_bearer_auth = True
            for item in handler.get("error_responses", []):
                status = str(item.get("status", 500))
                message = item.get("message", "")
                operation["responses"][status] = {
                    "description": message or "Feil",
                    "content": {
                        "application/json": {
                            "schema": {
                                "type": "object",
                                "properties": {"error": {"type": "string"}},
                                "required": ["error"],
                            },
                            "example": {"error": message or "Feil"},
                        }
                    },
                }
            body_fields = handler.get("body_fields", [])
            if body_fields:
                required_fields = [item["name"] for item in body_fields if item.get("required", False)]
                properties = {
                    item["name"]: {"type": item["type"]}
                    for item in body_fields
                }
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
        if has_bearer_auth:
            document["components"] = {
                "securitySchemes": {
                    "bearerAuth": {
                        "type": "http",
                        "scheme": "bearer",
                    }
                }
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

    def scope_with_var(self, name: str):
        for scope in reversed(self.scopes):
            if name in scope:
                return scope
        return None

    def get_var(self, name: str) -> Any:
        for scope in reversed(self.scopes):
            if name in scope:
                return scope[name]
        raise NameError(f"Ukjent variabel: {name}")

    def io_error(self, handling: str, path: str) -> str:
        return f"IOFeil: kunne ikke {handling} fil {path}"

    def http_error(self, message: str) -> str:
        return f"HTTPFeil: {message}"

    def validation_error(self, message: str) -> str:
        return f"ValideringsFeil: {message}"

    def json_scalar_to_text(self, value: Any) -> str:
        if value is None:
            return "null"
        if isinstance(value, bool):
            return "true" if value else "false"
        if isinstance(value, (int, float)):
            return str(value)
        return str(value)

    def json_serialize_value(self, value: Any) -> str:
        if value is None:
            return "null"
        if isinstance(value, bool):
            return "true" if value else "false"
        if isinstance(value, (int, float)):
            return json.dumps(value, ensure_ascii=False)
        if isinstance(value, str):
            return json.dumps(value, ensure_ascii=False)
        if isinstance(value, list):
            return "[" + ",".join(self.json_serialize_value(item) for item in value) + "]"
        if isinstance(value, dict):
            parts = []
            for key, item in value.items():
                parts.append(f"{json.dumps(str(key), ensure_ascii=False)}:{self.json_serialize_value(item)}")
            return "{" + ",".join(parts) + "}"
        return json.dumps(str(value), ensure_ascii=False)

    def make_async_value(self, value: Any, cancelled: bool = False, timeout_deadline: float | None = None) -> AsyncValue:
        if isinstance(value, AsyncValue):
            return AsyncValue(
                value=value.value,
                cancelled=cancelled or value.cancelled,
                timeout_deadline=timeout_deadline if timeout_deadline is not None else value.timeout_deadline,
            )
        return AsyncValue(value=value, cancelled=cancelled, timeout_deadline=timeout_deadline)

    def await_async_value(self, value: Any) -> Any:
        if not isinstance(value, AsyncValue):
            return value
        if value.cancelled:
            raise ThrowSignal("AvbruttFeil: future ble kansellert")
        if value.timeout_deadline is not None and time.monotonic() > value.timeout_deadline:
            raise ThrowSignal("TimeoutFeil: future gikk ut på tid")
        return value.value

    def http_request(self, method: str, url: str, headers: Any, query: Any, body: Any, timeout_ms: Any):
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
            raise ThrowSignal(self.http_error(f"{exc.code} {exc.reason}: {payload}".strip()))
        except Exception as exc:
            raise ThrowSignal(self.http_error(str(exc)))

    def http_request_full(self, method: str, url: str, headers: Any, query: Any, body: Any, timeout_ms: Any) -> str:
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
            raise ThrowSignal(self.http_error(f"{exc.code} {exc.reason}: {payload}".strip()))
        except Exception as exc:
            raise ThrowSignal(self.http_error(str(exc)))

    def parse_http_response(self, response_text: Any) -> dict[str, Any]:
        if isinstance(response_text, dict):
            return response_text
        if response_text is None:
            return {"status": 0, "body": "", "headers": {}}
        try:
            parsed = json.loads(str(response_text))
        except Exception as exc:
            raise ThrowSignal(self.http_error(f"ugyldig respons: {exc}"))
        if not isinstance(parsed, dict):
            raise ThrowSignal(self.http_error("forventet et JSON-objekt som respons"))
        return parsed

    def _normalize_web_path(self, value: Any) -> list[str]:
        text = str(value).strip()
        if text in ("", "/"):
            return []
        return [segment for segment in text.strip("/").split("/") if segment != ""]

    def _match_web_path(self, pattern: Any, path: Any) -> dict[str, str] | None:
        pattern_parts = self._normalize_web_path(pattern)
        path_parts = self._normalize_web_path(path)
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

    def _normalize_web_method(self, value: Any) -> str:
        return str(value).strip().upper()

    def _split_web_route_spec(self, spec: Any) -> tuple[str, str]:
        text = str(spec).strip()
        if not text:
            return "", ""
        parts = text.split(None, 1)
        if len(parts) == 1:
            return "", parts[0]
        return parts[0].upper(), parts[1]

    def _combine_web_route_prefix(self, prefix: Any, spec: Any) -> str:
        route_prefix = str(prefix).strip()
        route_spec = str(spec).strip()
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

    def _match_web_route_spec(self, spec: Any, method: Any, path: Any) -> dict[str, str] | None:
        route_method, route_path = self._split_web_route_spec(spec)
        request_method = self._normalize_web_method(method)
        if route_method and route_method != request_method and not (request_method == "HEAD" and route_method == "GET"):
            return None
        return self._match_web_path(route_path, path)

    def _allow_web_methods_for_path(self, path: Any) -> list[str]:
        path_text = str(path)
        methods: list[str] = []
        seen: set[str] = set()
        for handler in getattr(self, "route_handlers", []):
            spec = handler.get("spec", "")
            route_method, route_path = self._split_web_route_spec(spec)
            if self._match_web_path(route_path, path_text) is None:
                continue
            if route_method:
                if route_method not in seen:
                    seen.add(route_method)
                    methods.append(route_method)
                if route_method == "GET" and "HEAD" not in seen:
                    seen.add("HEAD")
                    methods.append("HEAD")
        if methods and "OPTIONS" not in seen:
            methods.append("OPTIONS")
        return methods

    def _encode_json_text_map(self, mapping: Any) -> str:
        if not isinstance(mapping, dict):
            return "{}"
        return json.dumps({str(key): str(value) for key, value in mapping.items()}, ensure_ascii=False)

    def _decode_json_text_map(self, value: Any) -> dict[str, Any]:
        if isinstance(value, dict):
            return {str(key): str(val) for key, val in value.items()}
        try:
            parsed = json.loads(str(value)) if value is not None else {}
        except Exception:
            return {}
        if not isinstance(parsed, dict):
            return {}
        return {str(key): str(val) for key, val in parsed.items()}

    def _extract_bearer_token(self, ctx: Any) -> str:
        if not isinstance(ctx, dict):
            return ""
        headers = self._decode_json_text_map(ctx.get("headers", "{}"))
        auth = str(headers.get("authorization", "")).strip()
        if not auth:
            return ""
        if auth.lower().startswith("bearer "):
            return auth[7:].strip()
        return ""

    def _web_csv_values(self, value: Any) -> list[str]:
        if value is None:
            return []
        if isinstance(value, list):
            values = [str(item).strip() for item in value]
        else:
            values = [part.strip() for part in re.split(r"[;,]", str(value))]
        return [item for item in values if item]

    def _web_header_csv_values(self, ctx: Any, *keys: str) -> list[str]:
        if not isinstance(ctx, dict):
            return []
        headers = self._decode_json_text_map(ctx.get("headers", "{}"))
        values: list[str] = []
        for key in keys:
            values.extend(self._web_csv_values(headers.get(key, "")))
        return values

    def _web_role(self, ctx: Any) -> str:
        roles = self._web_header_csv_values(ctx, "x-role", "x-user-role", "x-roles", "x-user-roles", "role")
        return roles[0] if roles else ""

    def _web_has_role(self, ctx: Any, expected: Any) -> bool:
        role = str(expected)
        if not role:
            return False
        return role in self._web_header_csv_values(ctx, "x-role", "x-user-role", "x-roles", "x-user-roles", "role")

    def _web_has_permission(self, ctx: Any, expected: Any) -> bool:
        permission = str(expected)
        if not permission:
            return False
        return permission in self._web_header_csv_values(ctx, "x-permission", "x-permissions", "x-right", "x-rights", "x-scope", "x-scopes")

    def _csrf_token(self, ctx: Any) -> str:
        if not isinstance(ctx, dict):
            return ""
        headers = self._decode_json_text_map(ctx.get("headers", "{}"))
        for key in ("x-csrf-token", "x-xsrf-token", "csrf-token", "x-csrf"):
            value = str(headers.get(key, "")).strip()
            if value:
                return value
        return ""

    def _csrf_require(self, ctx: Any, expected: Any) -> bool:
        token = self._csrf_token(ctx)
        return bool(token) and token == str(expected)

    def _escape_html(self, value: Any) -> str:
        text = str(value)
        return (
            text.replace("&", "&amp;")
            .replace("<", "&lt;")
            .replace(">", "&gt;")
            .replace('"', "&quot;")
            .replace("'", "&#x27;")
        )

    def _safe_filename(self, value: Any) -> str:
        text = str(value).strip()
        if not text:
            return "fil"
        text = text.replace("\\", "/").split("/", 1)[-1]
        result: list[str] = []
        for ch in text:
            if ch.isalnum() or ch in {"-", "_", "."}:
                result.append(ch)
            elif ch.isspace():
                result.append("_")
            else:
                result.append("_")
        cleaned = "".join(result).strip("._")
        return cleaned or "fil"

    def _safe_path_segment(self, value: Any) -> str:
        text = str(value).strip().replace("\\", "/")
        result: list[str] = []
        for ch in text:
            if ch.isalnum() or ch in {"-", "_"}:
                result.append(ch)
            elif ch.isspace():
                result.append("-")
        cleaned = "".join(result).strip("-_")
        return cleaned or "segment"

    def _safe_slug(self, value: Any) -> str:
        text = str(value).strip().lower()
        result: list[str] = []
        last_dash = False
        for ch in text:
            if ch.isalnum():
                result.append(ch)
                last_dash = False
            elif ch.isspace() or ch in {"-", "_"}:
                if not last_dash:
                    result.append("-")
                    last_dash = True
        cleaned = "".join(result).strip("-")
        return cleaned or "slug"

    def _db_escape_sql_literal(self, value: Any) -> str:
        return str(value).replace("'", "''")

    def _db_error(self, message: str) -> str:
        return f"DBFeil: {message}"

    def _db_module(self):
        import sqlite3

        return sqlite3

    def _db_get_connection(self, handle: Any):
        key = str(handle)
        conn = self._db_connections.get(key)
        if conn is None:
            raise ThrowSignal(self._db_error(f"ukjent database-handle '{key}'"))
        return conn

    def _db_get_pool(self, handle: Any):
        key = str(handle)
        pool = self._db_pools.get(key)
        if pool is None:
            raise ThrowSignal(self._db_error(f"ukjent database-pool '{key}'"))
        return pool

    def _db_new_connection(self, sqlite3, db_path: str):
        conn = sqlite3.connect(db_path)
        conn.isolation_level = None
        conn.row_factory = sqlite3.Row
        return conn

    def _db_open(self, path: Any) -> str:
        sqlite3 = self._db_module()
        db_path = str(path)
        try:
            conn = self._db_new_connection(sqlite3, db_path)
        except Exception as exc:
            raise ThrowSignal(self._db_error(f"kunne ikke åpne database {db_path}: {exc}"))
        handle = f"db-{self._db_next_handle}"
        self._db_next_handle += 1
        self._db_connections[handle] = conn
        return handle

    def _db_pool_open(self, path: Any, max_connections: Any) -> str:
        sqlite3 = self._db_module()
        db_path = str(path)
        try:
            max_conn = int(max_connections)
        except Exception:
            max_conn = 1
        if max_conn <= 0:
            max_conn = 1
        try:
            conn = self._db_new_connection(sqlite3, db_path)
        except Exception as exc:
            raise ThrowSignal(self._db_error(f"kunne ikke åpne database {db_path}: {exc}"))
        handle = f"pool-{self._db_next_pool_handle}"
        self._db_next_pool_handle += 1
        self._db_pools[handle] = {
            "path": db_path,
            "max": max_conn,
            "idle": [conn],
            "closed": False,
        }
        return handle

    def _db_pool_acquire(self, pool_handle: Any) -> str:
        sqlite3 = self._db_module()
        pool = self._db_get_pool(pool_handle)
        if pool.get("closed"):
            raise ThrowSignal(self._db_error(f"database-pool '{pool_handle}' er lukket"))
        idle = pool["idle"]
        if idle:
            conn = idle.pop()
        else:
            active = len(self._db_pooled_handles)
            total = active + len(idle)
            max_conn = int(pool.get("max") or 1)
            if max_conn > 0 and total >= max_conn:
                raise ThrowSignal(self._db_error("ingen ledige forbindelser i poolen"))
            try:
                conn = self._db_new_connection(sqlite3, pool["path"])
            except Exception as exc:
                raise ThrowSignal(self._db_error(f"kunne ikke åpne database {pool['path']}: {exc}"))
        handle = f"db-{self._db_next_handle}"
        self._db_next_handle += 1
        self._db_connections[handle] = conn
        self._db_pooled_handles[handle] = str(pool_handle)
        return handle

    def _db_pool_size(self, pool_handle: Any) -> int:
        pool = self._db_get_pool(pool_handle)
        return len(pool["idle"])

    def _db_pool_close(self, pool_handle: Any) -> bool:
        pool = self._db_get_pool(pool_handle)
        idle = pool["idle"]
        while idle:
            conn = idle.pop()
            try:
                conn.close()
            except Exception:
                pass
        pool["closed"] = True
        return True

    def _db_close(self, handle: Any) -> bool:
        key = str(handle)
        conn = self._db_connections.pop(key, None)
        if conn is None:
            raise ThrowSignal(self._db_error(f"ukjent database-handle '{handle}'"))
        try:
            pool_handle = self._db_pooled_handles.pop(key, None)
            if pool_handle is not None:
                pool = self._db_pools.get(pool_handle)
                if pool is not None and not pool.get("closed"):
                    pool["idle"].append(conn)
                    return True
            conn.close()
        except Exception as exc:
            raise ThrowSignal(self._db_error(f"kunne ikke lukke database: {exc}"))
        return True

    def _db_execute(self, handle: Any, sql: Any) -> int:
        conn = self._db_get_connection(handle)
        try:
            cursor = conn.execute(str(sql))
            return int(cursor.rowcount if cursor.rowcount != -1 else 0)
        except Exception as exc:
            raise ThrowSignal(self._db_error(f"kunne ikke kjøre SQL: {exc}"))

    def _db_query_text(self, handle: Any, sql: Any) -> str:
        conn = self._db_get_connection(handle)
        try:
            cursor = conn.execute(str(sql))
            row = cursor.fetchone()
            if row is None:
                return ""
            value = row[0] if len(row) else ""
            return "" if value is None else str(value)
        except Exception as exc:
            raise ThrowSignal(self._db_error(f"kunne ikke lese SQL-resultat: {exc}"))

    def _db_query_int(self, handle: Any, sql: Any) -> int:
        text = self._db_query_text(handle, sql)
        if not text:
            return 0
        try:
            return int(text)
        except ValueError as exc:
            raise ThrowSignal(self._db_error(f"forventet heltall fra SQL, fikk '{text}'")) from exc

    def _db_migrate(self, handle: Any, migrations: Any) -> int:
        conn = self._db_get_connection(handle)
        script = str(migrations)
        if not script.strip():
            return 0
        try:
            conn.execute("CREATE TABLE IF NOT EXISTS norscode_migrations (script TEXT PRIMARY KEY)")
            escaped = self._db_escape_sql_literal(script)
            existing = conn.execute(
                f"SELECT 1 FROM norscode_migrations WHERE script = '{escaped}' LIMIT 1"
            ).fetchone()
            if existing is not None:
                return 0
            conn.executescript(script)
            conn.execute(
                f"INSERT INTO norscode_migrations(script) VALUES ('{escaped}')"
            )
            conn.commit()
            return 1
        except ThrowSignal:
            raise
        except Exception as exc:
            raise ThrowSignal(self._db_error(f"kunne ikke kjøre migreringer: {exc}"))

    def _db_transaction(self, handle: Any, script: Any) -> int:
        conn = self._db_get_connection(handle)
        sql = str(script)
        if not sql.strip():
            return 0
        try:
            conn.execute("BEGIN")
            conn.executescript(sql)
            conn.commit()
            return 1
        except ThrowSignal:
            raise
        except Exception as exc:
            try:
                conn.rollback()
            except Exception:
                pass
            return 0

    def _db_begin(self, handle: Any) -> bool:
        conn = self._db_get_connection(handle)
        try:
            conn.execute("BEGIN")
            return True
        except Exception as exc:
            raise ThrowSignal(self._db_error(f"kunne ikke starte transaksjon: {exc}"))

    def _db_commit(self, handle: Any) -> bool:
        conn = self._db_get_connection(handle)
        try:
            conn.commit()
            return True
        except Exception as exc:
            raise ThrowSignal(self._db_error(f"kunne ikke committe transaksjon: {exc}"))

    def _db_rollback(self, handle: Any) -> bool:
        conn = self._db_get_connection(handle)
        try:
            conn.rollback()
            return True
        except Exception as exc:
            raise ThrowSignal(self._db_error(f"kunne ikke rulle tilbake transaksjon: {exc}"))

    def _password_hash_core(self, password: str, salt: str) -> str:
        data = f"{salt}\0{password}".encode("utf-8")
        state = 0xCBF29CE484222325
        for _ in range(4096):
            for byte in data:
                state ^= byte
                state = (state * 0x100000001B3) & 0xFFFFFFFFFFFFFFFF
            data = state.to_bytes(8, "big") + data
        return f"{salt}${state:016x}"

    def _password_hash(self, password: Any, salt: Any) -> str:
        return self._password_hash_core(str(password), str(salt))

    def _password_verify(self, password: Any, stored: Any) -> bool:
        stored_text = str(stored)
        if "$" not in stored_text:
            return False
        salt, digest = stored_text.split("$", 1)
        if not salt or not digest:
            return False
        return self._password_hash_core(str(password), salt) == stored_text

    def _make_web_request_context(self, method: Any, path: Any, query: Any, headers: Any, body: Any) -> dict[str, Any]:
        self._request_counter += 1
        return {
            "metode": self._normalize_web_method(method),
            "sti": str(path),
            "query": self._encode_json_text_map(query),
            "headers": self._encode_json_text_map(headers),
            "body": "" if body is None else str(body),
            "params": self._encode_json_text_map({}),
            "deps": self._encode_json_text_map({}),
            "request_id": f"req-{self._request_counter}",
        }

    def _make_web_response(self, status: Any, headers: Any, body: Any) -> dict[str, Any]:
        return {
            "status": str(int(status)),
            "headers": self._encode_json_text_map(headers),
            "body": "" if body is None else str(body),
        }

    def _make_route_context(self, ctx: Any, params: dict[str, str]) -> dict[str, Any]:
        if not isinstance(ctx, dict):
            ctx = self._make_web_request_context("", "", {}, {}, "")
        new_ctx = dict(ctx)
        new_ctx["params"] = self._encode_json_text_map(params)
        if "deps" not in new_ctx:
            new_ctx["deps"] = self._encode_json_text_map({})
        return new_ctx

    def _run_named_hook(self, name: str) -> Any:
        if not name:
            return None
        return self.call_user_function(name, [])

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
            result = self.call_user_function(middleware, [current])
            if isinstance(result, dict):
                current = result
        return current

    def _apply_response_middlewares(self, response: Any) -> Any:
        current = response
        for middleware in self.response_middlewares:
            result = self.call_user_function(middleware, [current])
            if isinstance(result, dict):
                current = result
        return current

    def _apply_error_middlewares(self, response: Any) -> Any:
        current = response
        for middleware in self.error_middlewares:
            result = self.call_user_function(middleware, [current])
            if isinstance(result, dict):
                current = result
        return current

    def _web_error_response(self, message: Any, status: int = 500) -> dict[str, Any]:
        payload = {"error": str(message)}
        return self._make_web_response(status, {"content-type": "application/json"}, self.json_serialize_value(payload))

    def _request_json_object(self, ctx: Any) -> dict[str, str]:
        if not isinstance(ctx, dict):
            raise ThrowSignal(self.validation_error("body forventet gyldig JSON-objekt"))
        body_text = str(ctx.get("body", "") or "")
        if not body_text.strip():
            return {}
        try:
            parsed = json.loads(body_text)
        except Exception:
            raise ThrowSignal(self.validation_error("body forventet gyldig JSON-objekt"))
        if not isinstance(parsed, dict):
            raise ThrowSignal(self.validation_error("body forventet JSON-objekt"))
        return {str(key): self.json_scalar_to_text(value) for key, value in parsed.items()}

    def _parse_required_text(self, value: Any, field_name: str, source_name: str) -> str:
        text = str(value)
        if text == "":
            raise ThrowSignal(self.validation_error(f"mangler {source_name}-felt '{field_name}'"))
        return text

    def _parse_int_value(self, value: Any, field_name: str, source_name: str) -> int:
        text = str(value).strip()
        if not re.fullmatch(r"-?\d+", text):
            raise ThrowSignal(self.validation_error(f"felt '{field_name}' forventet heltall, fikk '{value}'"))
        return int(text)

    def _parse_bool_value(self, value: Any, field_name: str, source_name: str) -> bool:
        text = str(value).strip().lower()
        if text in {"true", "1", "ja"}:
            return True
        if text in {"false", "0", "nei"}:
            return False
        raise ThrowSignal(self.validation_error(f"felt '{field_name}' forventet bool, fikk '{value}'"))

    def _collect_web_annotations(self, program: ProgramNode):
        handlers: list[dict[str, Any]] = []
        providers: dict[str, str] = {}
        guard_providers: dict[str, str] = {}
        auth_guard_names: set[str] = set()
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
                if not expr.args or not isinstance(expr.args[0], StringNode):
                    if expr.func_name in {"request_middleware", "response_middleware", "error_middleware", "startup_hook", "shutdown_hook"} and not expr.args:
                        if expr.func_name == "request_middleware":
                            request_middlewares.append(fn.name)
                            continue
                        if expr.func_name == "response_middleware":
                            response_middlewares.append(fn.name)
                            continue
                        if expr.func_name == "error_middleware":
                            error_middlewares.append(fn.name)
                            continue
                        if expr.func_name == "startup_hook":
                            startup_hooks.append(fn.name)
                            continue
                    if expr.func_name == "shutdown_hook":
                        shutdown_hooks.append(fn.name)
                        continue
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
                guard_providers[fn.name] = fn.name
                if self._function_uses_bearer_auth(fn):
                    auth_guard_names.add(fn.name)
            if spec is not None:
                combined_spec = self._combine_web_route_prefix(route_prefix, spec)
                route_docs = self._route_docs_from_function(fn, combined_spec, auth_guard_names)
                route_docs["deps"] = list(deps)
                route_docs["guards"] = list(guards)
                route_docs["spec"] = combined_spec
                if route_docs.get("security") is None and any(guard in auth_guard_names for guard in guards):
                    route_docs["security"] = [{"bearerAuth": []}]
                    route_docs.setdefault("responses", {})
                    route_docs["responses"]["401"] = {"description": "Uautorisert"}
                    route_docs["responses"]["403"] = {"description": "Forbudt"}
                handlers.append(route_docs)
            for dep_name in provided:
                providers[dep_name] = fn.name
        return handlers, providers, guard_providers, request_middlewares, response_middlewares, error_middlewares, startup_hooks, shutdown_hooks

    def _dependency_map(self, ctx: Any) -> dict[str, str]:
        if not isinstance(ctx, dict):
            return {}
        return self._decode_json_text_map(ctx.get("deps", "{}"))

    def _store_dependency_map(self, ctx: Any, deps: dict[str, str]) -> None:
        if isinstance(ctx, dict):
            ctx["deps"] = self._encode_json_text_map(deps)

    def _normalize_dependency_value(self, value: Any) -> dict[str, str]:
        if isinstance(value, dict):
            return {str(key): str(val) for key, val in value.items()}
        if value is None:
            return {}
        if isinstance(value, str):
            try:
                parsed = json.loads(value)
                if isinstance(parsed, dict):
                    return {str(key): self.json_scalar_to_text(val) for key, val in parsed.items()}
            except Exception:
                pass
            return {"value": value}
        return {"value": self.json_scalar_to_text(value)}

    def _resolve_dependency_value(self, ctx: Any, dep_name: str):
        deps = self._dependency_map(ctx)
        if dep_name in deps:
            cached = deps.get(dep_name, "")
            try:
                parsed = json.loads(cached) if cached else {}
            except Exception:
                parsed = {}
            if isinstance(parsed, dict):
                return {str(key): str(val) for key, val in parsed.items()}
            return {}
        provider_name = self.dependency_providers.get(dep_name)
        if not provider_name:
            raise RuntimeError(f"Mangler dependency-provider for '{dep_name}'")
        provider_fn = self.functions.get(provider_name)
        if provider_fn is None:
            raise RuntimeError(f"Mangler dependency-funksjon '{provider_name}'")
        provider_args = [ctx] if getattr(provider_fn.node, "params", []) else []
        value = self.call_user_function(provider_name, provider_args)
        normalized = self._normalize_dependency_value(value)
        deps[dep_name] = json.dumps(normalized, ensure_ascii=False)
        self._store_dependency_map(ctx, deps)
        return normalized

    def _find_route_handler(self, method: Any, path: Any):
        method_text = self._normalize_web_method(method)
        exact: list[tuple[dict[str, str], dict[str, Any]]] = []
        param: list[tuple[dict[str, str], dict[str, Any]]] = []
        method_mismatch = False
        for handler in getattr(self, "route_handlers", []):
            spec = handler.get("spec", "")
            route_method, route_path = self._split_web_route_spec(spec)
            path_params = self._match_web_path(route_path, path)
            if path_params is None:
                continue
            if route_method and route_method != method_text:
                method_mismatch = True
                continue
            entry = (path_params, handler)
            if "{" in route_path or "}" in route_path:
                param.append(entry)
            else:
                exact.append(entry)
        if exact:
            return exact[0]
        if param:
            return param[0]
        if method_mismatch:
            return ("__405__", {})
        return None

    def run(self, program: ProgramNode):
        (
            self.route_handlers,
            self.dependency_providers,
            self.guard_providers,
            self.request_middlewares,
            self.response_middlewares,
            self.error_middlewares,
            self.startup_hooks,
            self.shutdown_hooks,
        ) = self._collect_web_annotations(program)
        for fn in getattr(program, "functions", []):
            self.functions[fn.name] = UserFunction(fn)

        for fn in getattr(program, "tests", []):
            self.functions[fn.name] = UserFunction(fn)

        if "start" not in self.functions:
            self._ensure_startup_hooks()
            self._run_shutdown_hooks()
            return None
        self._ensure_startup_hooks()
        try:
            return self.call_user_function("start", [])
        finally:
            self._run_shutdown_hooks()

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

        if isinstance(node, MatchNode):
            subject = self.eval(node.subject)
            for case in getattr(node, "cases", []):
                if getattr(case, "wildcard", False):
                    return self.eval(case.body)
                if self.eval(case.pattern) == subject:
                    return self.eval(case.body)
            if node.else_block is not None:
                return self.eval(node.else_block)
            return None

        if isinstance(node, ExprStmtNode):
            return self.eval(node.expr)

        if isinstance(node, ListLiteralNode):
            return [self.eval(item) for item in node.items]

        if isinstance(node, ListComprehensionNode):
            source = self.eval(node.source_expr)
            if not isinstance(source, list):
                raise RuntimeError("Liste-comprehension krever en liste som kilde")
            result = []
            self.push_scope()
            try:
                for item in source:
                    self.define_var(node.item_name, item)
                    if node.condition_expr is not None and not self.eval(node.condition_expr):
                        continue
                    result.append(self.eval(node.item_expr))
            finally:
                self.pop_scope()
            return result

        if isinstance(node, SliceNode):
            target = self.eval(node.target)
            start = self.eval(node.start_expr) if node.start_expr is not None else None
            end = self.eval(node.end_expr) if node.end_expr is not None else None
            if isinstance(target, str):
                return target[start:end]
            if isinstance(target, list):
                return target[start:end]
            raise RuntimeError("Slicing kan bare brukes på tekst og lister")

        if isinstance(node, AwaitNode):
            return self.await_async_value(self.eval(node.expr))

        if isinstance(node, LambdaNode):
            return LambdaValue(
                params=list(node.params),
                body=node.body,
                captured_scopes=list(self.scopes),
            )

        if isinstance(node, ReturnNode):
            raise ReturnSignal(self.eval(node.expr))

        if isinstance(node, ThrowNode):
            raise ThrowSignal(self.eval(node.expr))

        if isinstance(node, TryCatchNode):
            try:
                return self.eval(node.try_block)
            except ThrowSignal as signal:
                if node.catch_var_name is None:
                    return self.eval(node.catch_block)

                target_scope = self.scope_with_var(node.catch_var_name)
                is_new = target_scope is None
                if is_new:
                    self.define_var(node.catch_var_name, signal.value)
                else:
                    old_value = target_scope.get(node.catch_var_name)
                    target_scope[node.catch_var_name] = signal.value
                try:
                    return self.eval(node.catch_block)
                finally:
                    if is_new:
                        self.current_scope().pop(node.catch_var_name, None)
                    else:
                        target_scope[node.catch_var_name] = old_value

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

        lambda_scope = self.scope_with_var(name)
        if lambda_scope is not None:
            lambda_value = lambda_scope.get(name)
            if isinstance(lambda_value, LambdaValue):
                return self.call_lambda_value(lambda_value, values, name)

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

        if name == "assert_starter_med":
            if len(values) < 2 or not str(values[0]).startswith(str(values[1])):
                raise AssertionError(f"assert_starter_med feilet: {values[0]!r} starter ikke med {values[1]!r}")
            return None

        if name == "assert_slutter_med":
            if len(values) < 2 or not str(values[0]).endswith(str(values[1])):
                raise AssertionError(f"assert_slutter_med feilet: {values[0]!r} slutter ikke med {values[1]!r}")
            return None

        if name == "assert_inneholder":
            if len(values) < 2 or str(values[1]) not in str(values[0]):
                raise AssertionError(f"assert_inneholder feilet: {values[0]!r} inneholder ikke {values[1]!r}")
            return None

        if name == "sti_join":
            parts = [str(part) for part in values if str(part)]
            if not parts:
                return ""
            return posixpath.join(*parts)

        if name == "sti_basename":
            return posixpath.basename(str(values[0])) if values else ""

        if name == "sti_dirname":
            return posixpath.dirname(str(values[0])) if values else ""

        if name == "sti_exists":
            return os.path.exists(str(values[0])) if values else False

        if name == "sti_stem":
            return posixpath.splitext(posixpath.basename(str(values[0])))[0] if values else ""

        if name == "tekst_starter_med":
            return str(values[0]).startswith(str(values[1])) if len(values) >= 2 else False

        if name == "tekst_slutter_med":
            return str(values[0]).endswith(str(values[1])) if len(values) >= 2 else False

        if name == "tekst_inneholder":
            return str(values[1]) in str(values[0]) if len(values) >= 2 else False

        if name == "tekst_trim":
            return str(values[0]).strip() if values else ""

        if name == "miljo_hent":
            key = str(values[0]) if values else ""
            return os.environ.get(key, "")

        if name == "miljo_finnes":
            key = str(values[0]) if values else ""
            return key in os.environ

        if name == "miljo_sett":
            if len(values) < 2:
                raise RuntimeError("miljo_sett forventer 2 argumenter")
            key = str(values[0])
            value = str(values[1])
            os.environ[key] = value
            return value

        if name == "fil_les":
            path = Path(str(values[0])) if values else Path("")
            try:
                return path.expanduser().read_text(encoding="utf-8")
            except OSError:
                raise ThrowSignal(self.io_error("lese", str(path)))

        if name == "fil_skriv":
            if len(values) < 2:
                raise RuntimeError("fil_skriv forventer 2 argumenter")
            path = Path(str(values[0])).expanduser()
            text = str(values[1])
            try:
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(text, encoding="utf-8")
            except OSError:
                raise ThrowSignal(self.io_error("skrive", str(path)))
            return text

        if name == "fil_append":
            if len(values) < 2:
                raise RuntimeError("fil_append forventer 2 argumenter")
            path = Path(str(values[0])).expanduser()
            text = str(values[1])
            try:
                path.parent.mkdir(parents=True, exist_ok=True)
                with path.open("a", encoding="utf-8") as f:
                    f.write(text)
            except OSError:
                raise ThrowSignal(self.io_error("legge til i", str(path)))
            return text

        if name == "fil_finnes":
            return Path(str(values[0])).expanduser().exists() if values else False

        if name == "db.open":
            if not values:
                return ""
            return self._db_open(values[0])

        if name == "db.close":
            if not values:
                return False
            return self._db_close(values[0])

        if name == "db.execute":
            if len(values) < 2:
                return 0
            return self._db_execute(values[0], values[1])

        if name == "db.query_text":
            if len(values) < 2:
                return ""
            return self._db_query_text(values[0], values[1])

        if name == "db.query_int":
            if len(values) < 2:
                return 0
            return self._db_query_int(values[0], values[1])

        if name == "db.migrate":
            if len(values) < 2:
                return 0
            return self._db_migrate(values[0], values[1])

        if name == "db.transaction":
            if len(values) < 2:
                return 0
            return self._db_transaction(values[0], values[1])

        if name == "db.pool":
            if len(values) < 2:
                return ""
            return self._db_pool_open(values[0], values[1])

        if name == "db.pool_acquire":
            if not values:
                return ""
            return self._db_pool_acquire(values[0])

        if name == "db.pool_size":
            if not values:
                return 0
            return self._db_pool_size(values[0])

        if name == "db.pool_close":
            if not values:
                return False
            return self._db_pool_close(values[0])

        if name == "db.begin":
            if not values:
                return False
            return self._db_begin(values[0])

        if name == "db.commit":
            if not values:
                return False
            return self._db_commit(values[0])

        if name == "db.rollback":
            if not values:
                return False
            return self._db_rollback(values[0])

        if name == "db.ping":
            if not values:
                return False
            try:
                conn = self._db_get_connection(values[0])
                conn.execute("SELECT 1")
                return True
            except ThrowSignal:
                raise
            except Exception as exc:
                raise ThrowSignal(self._db_error(f"kunne ikke ping database: {exc}"))

        if name == "json_parse":
            payload = str(values[0]) if values else ""
            try:
                parsed = json.loads(payload)
            except Exception as exc:
                raise ThrowSignal(f"json_parse feilet: {exc}")
            if isinstance(parsed, list):
                return {str(i): self.json_scalar_to_text(v) for i, v in enumerate(parsed)}
            if isinstance(parsed, dict):
                return {str(key): self.json_scalar_to_text(value) for key, value in parsed.items()}
            raise ThrowSignal("json_parse forventer et JSON-objekt eller en JSON-liste")

        if name == "json_stringify":
            map_value = values[0] if values else {}
            if map_value is None:
                map_value = {}
            if not isinstance(map_value, dict):
                raise ThrowSignal("json_stringify forventer en ordbok")
            pieces = []
            for key, value in map_value.items():
                pieces.append(f"{json.dumps(str(key), ensure_ascii=False)}:{self.json_serialize_value(value)}")
            return "{" + ",".join(pieces) + "}"

        if name in self.functions:
            return self.call_user_function(name, values)

        raise RuntimeError(f"Ukjent funksjon: {name}")

    def call_lambda_value(self, lambda_value: LambdaValue, args: list[Any], call_name: str | None = None):
        if len(args) != len(lambda_value.params):
            display_name = call_name or lambda_value.display_name
            raise RuntimeError(
                f"Lambda '{display_name}' forventer {len(lambda_value.params)} argument(er), fikk {len(args)}"
            )

        previous_scopes = self.scopes
        self.scopes = list(lambda_value.captured_scopes) + [{}]
        self.call_stack.append(call_name or lambda_value.display_name)
        try:
            for param, value in zip(lambda_value.params, args):
                self.define_var(param.name, value)
            try:
                return self.eval(lambda_value.body)
            except ReturnSignal as signal:
                return signal.value
            except ThrowSignal as signal:
                signal.call_stack.append(call_name or lambda_value.display_name)
                raise
        finally:
            self.call_stack.pop()
            self.scopes = previous_scopes

    def eval_module_call(self, module_name: str, func_name: str, args: list[Any]):
        values = [self.eval(arg) for arg in args]

        if module_name == "math":
            if func_name == "pluss":
                return values[0] + values[1]
            if func_name == "minus":
                return values[0] - values[1]
        if module_name == "path":
            if func_name == "join":
                parts = [str(part) for part in values if str(part)]
                if not parts:
                    return ""
                return posixpath.join(*parts)
            if func_name == "basename":
                return posixpath.basename(str(values[0])) if values else ""
            if func_name == "dirname":
                return posixpath.dirname(str(values[0])) if values else ""
            if func_name == "exists":
                return os.path.exists(str(values[0])) if values else False
            if func_name == "stem":
                return posixpath.splitext(posixpath.basename(str(values[0])))[0] if values else ""
        if module_name == "env":
            if func_name == "hent":
                key = str(values[0]) if values else ""
                return os.environ.get(key, "")
            if func_name == "finnes":
                key = str(values[0]) if values else ""
                return key in os.environ
            if func_name == "sett":
                if len(values) < 2:
                    raise RuntimeError("env.sett forventer 2 argumenter")
                key = str(values[0])
                value = str(values[1])
                os.environ[key] = value
                return value
        if module_name == "tekst":
            if func_name == "starter_med":
                return str(values[0]).startswith(str(values[1])) if len(values) >= 2 else False
            if func_name == "slutter_med":
                return str(values[0]).endswith(str(values[1])) if len(values) >= 2 else False
            if func_name == "inneholder":
                return str(values[1]) in str(values[0]) if len(values) >= 2 else False
            if func_name == "trim":
                return str(values[0]).strip() if values else ""
        if module_name == "fil":
            if func_name == "les":
                path = Path(str(values[0])) if values else Path("")
                try:
                    return path.expanduser().read_text(encoding="utf-8")
                except OSError:
                    raise ThrowSignal(self.io_error("lese", str(path)))
            if func_name == "skriv":
                if len(values) < 2:
                    raise RuntimeError("fil.skriv forventer 2 argumenter")
                path = Path(str(values[0])).expanduser()
                text = str(values[1])
                try:
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_text(text, encoding="utf-8")
                except OSError:
                    raise ThrowSignal(self.io_error("skrive", str(path)))
                return text
            if func_name == "append":
                if len(values) < 2:
                    raise RuntimeError("fil.append forventer 2 argumenter")
                path = Path(str(values[0])).expanduser()
                text = str(values[1])
                try:
                    path.parent.mkdir(parents=True, exist_ok=True)
                    with path.open("a", encoding="utf-8") as f:
                        f.write(text)
                except OSError:
                    raise ThrowSignal(self.io_error("legge til i", str(path)))
                return text
            if func_name == "finnes":
                return Path(str(values[0])).expanduser().exists() if values else False
        if module_name == "db":
            if func_name == "open":
                if not values:
                    return ""
                return self._db_open(values[0])
            if func_name == "close":
                if not values:
                    return False
                return self._db_close(values[0])
            if func_name == "execute":
                if len(values) < 2:
                    return 0
                return self._db_execute(values[0], values[1])
            if func_name == "query_text":
                if len(values) < 2:
                    return ""
                return self._db_query_text(values[0], values[1])
            if func_name == "query_int":
                if len(values) < 2:
                    return 0
                return self._db_query_int(values[0], values[1])
            if func_name == "migrate":
                if len(values) < 2:
                    return 0
                return self._db_migrate(values[0], values[1])
            if func_name == "transaction":
                if len(values) < 2:
                    return 0
                return self._db_transaction(values[0], values[1])
            if func_name == "pool":
                if len(values) < 2:
                    return ""
                return self._db_pool_open(values[0], values[1])
            if func_name == "pool_acquire":
                if not values:
                    return ""
                return self._db_pool_acquire(values[0])
            if func_name == "pool_size":
                if not values:
                    return 0
                return self._db_pool_size(values[0])
            if func_name == "pool_close":
                if not values:
                    return False
                return self._db_pool_close(values[0])
            if func_name == "begin":
                if not values:
                    return False
                return self._db_begin(values[0])
            if func_name == "commit":
                if not values:
                    return False
                return self._db_commit(values[0])
            if func_name == "rollback":
                if not values:
                    return False
                return self._db_rollback(values[0])
            if func_name == "ping":
                if not values:
                    return False
                try:
                    conn = self._db_get_connection(values[0])
                    conn.execute("SELECT 1")
                    return True
                except ThrowSignal:
                    raise
                except Exception as exc:
                    raise ThrowSignal(self._db_error(f"kunne ikke ping database: {exc}"))
        if module_name in ("vent", "std.vent"):
            if func_name == "timeout":
                if len(values) < 2:
                    raise RuntimeError("vent.timeout forventer 2 argumenter")
                timeout_ms = max(0, int(values[1]))
                return self.make_async_value(values[0], timeout_deadline=time.monotonic() + (timeout_ms / 1000.0))
            if func_name == "kanseller":
                if not values:
                    raise RuntimeError("vent.kanseller forventer 1 argument")
                return self.make_async_value(values[0], cancelled=True)
            if func_name == "er_kansellert":
                if not values:
                    return False
                value = values[0]
                return isinstance(value, AsyncValue) and value.cancelled
            if func_name == "er_timeoutet":
                if not values:
                    return False
                value = values[0]
                return isinstance(value, AsyncValue) and value.timeout_deadline is not None and time.monotonic() > value.timeout_deadline
            if func_name == "sov":
                if not values:
                    raise RuntimeError("vent.sov forventer 1 argument")
                time.sleep(max(0, int(values[0])) / 1000.0)
                return None
        if module_name in ("http", "std.http"):
            if func_name in ("request", "get", "post"):
                if func_name == "request":
                    if len(values) < 6:
                        raise RuntimeError("http.request forventer 6 argumenter")
                    return self.http_request(values[0], values[1], values[2], values[3], values[4], values[5])
                if func_name == "get":
                    if len(values) < 4:
                        raise RuntimeError("http.get forventer 4 argumenter")
                    return self.http_request("GET", values[0], values[1], values[2], None, values[3])
                if func_name == "post":
                    if len(values) < 5:
                        raise RuntimeError("http.post forventer 5 argumenter")
                    return self.http_request("POST", values[0], values[2], values[3], values[1], values[4])
            if func_name == "json_get":
                if len(values) < 4:
                    raise RuntimeError("http.json_get forventer 4 argumenter")
                return self.eval_call("json_parse", [self.http_request("GET", values[0], values[1], values[2], None, values[3])])
            if func_name == "json_post":
                if len(values) < 5:
                    raise RuntimeError("http.json_post forventer 5 argumenter")
                body_text = self.eval_call("json_stringify", [values[1]])
                return self.eval_call("json_parse", [self.http_request("POST", values[0], values[2], values[3], body_text, values[4])])
            if func_name == "request_full":
                if len(values) < 6:
                    raise RuntimeError("http.request_full forventer 6 argumenter")
                return self.http_request_full(values[0], values[1], values[2], values[3], values[4], values[5])
            if func_name == "response_status":
                if not values:
                    return 0
                return int(self.parse_http_response(values[0]).get("status", 0))
            if func_name == "response_text":
                if not values:
                    return ""
                return str(self.parse_http_response(values[0]).get("body", ""))
            if func_name == "response_json":
                if not values:
                    return {}
                body = str(self.parse_http_response(values[0]).get("body", ""))
                return self.eval_call("json_parse", [body])
            if func_name == "response_header":
                if len(values) < 2:
                    raise RuntimeError("http.response_header forventer 2 argumenter")
                headers = self.parse_http_response(values[0]).get("headers", {})
                if isinstance(headers, dict):
                    return str(headers.get(str(values[1]), ""))
                return ""
        if module_name in ("web", "std.web"):
            if func_name == "path_match":
                if len(values) < 2:
                    return False
                return self._match_web_path(values[0], values[1]) is not None
            if func_name == "path_params":
                if len(values) < 2:
                    return {}
                return self._match_web_path(values[0], values[1]) or {}
            if func_name == "route_match":
                if len(values) < 3:
                    return False
                return self._match_web_route_spec(values[0], values[1], values[2]) is not None
            if func_name == "dispatch":
                if len(values) < 3 or not isinstance(values[0], dict):
                    return ""
                routes = values[0]
                method = values[1]
                path = values[2]
                for route_name, route_spec in routes.items():
                    if self._match_web_route_spec(route_spec, method, path) is not None:
                        return str(route_name)
                return ""
            if func_name == "dispatch_params":
                if len(values) < 3 or not isinstance(values[0], dict):
                    return {}
                routes = values[0]
                method = values[1]
                path = values[2]
                for route_name, route_spec in routes.items():
                    params = self._match_web_route_spec(route_spec, method, path)
                    if params is not None:
                        return params
                return {}
            if func_name == "request_context":
                if len(values) < 5:
                    return self._make_web_request_context("", "", {}, {}, "")
                return self._make_web_request_context(values[0], values[1], values[2], values[3], values[4])
            if func_name == "request_id":
                if not values:
                    return ""
                ctx = self._decode_json_text_map(values[0])
                return str(ctx.get("request_id", ""))
            if func_name == "request_method":
                if not values:
                    return ""
                ctx = self._decode_json_text_map(values[0])
                return str(ctx.get("metode", ""))
            if func_name == "request_path":
                if not values:
                    return ""
                ctx = self._decode_json_text_map(values[0])
                return str(ctx.get("sti", ""))
            if func_name == "request_query":
                if not values:
                    return {}
                ctx = self._decode_json_text_map(values[0])
                return self._decode_json_text_map(ctx.get("query", "{}"))
            if func_name == "request_headers":
                if not values:
                    return {}
                ctx = self._decode_json_text_map(values[0])
                return self._decode_json_text_map(ctx.get("headers", "{}"))
            if func_name == "request_body":
                if not values:
                    return ""
                ctx = self._decode_json_text_map(values[0])
                return str(ctx.get("body", ""))
            if func_name == "request_params":
                if not values:
                    return {}
                ctx = self._decode_json_text_map(values[0])
                return self._decode_json_text_map(ctx.get("params", "{}"))
            if func_name == "request_param":
                if len(values) < 2:
                    return ""
                ctx = self._decode_json_text_map(values[0])
                params = self._decode_json_text_map(ctx.get("params", "{}"))
                return str(params.get(str(values[1]), ""))
            if func_name == "request_param_int":
                if len(values) < 2:
                    return 0
                ctx = self._decode_json_text_map(values[0])
                params = self._decode_json_text_map(ctx.get("params", "{}"))
                key = str(values[1])
                if key not in params or str(params.get(key, "")) == "":
                    raise ThrowSignal(self.validation_error(f"mangler path-felt '{key}'"))
                return self._parse_int_value(params.get(key, ""), key, "path")
            if func_name == "request_query_required":
                if len(values) < 2:
                    return ""
                ctx = self._decode_json_text_map(values[0])
                query = self._decode_json_text_map(ctx.get("query", "{}"))
                key = str(values[1])
                return self._parse_required_text(query.get(key, ""), key, "query")
            if func_name == "request_query_int":
                if len(values) < 2:
                    return 0
                ctx = self._decode_json_text_map(values[0])
                query = self._decode_json_text_map(ctx.get("query", "{}"))
                key = str(values[1])
                if key not in query or str(query.get(key, "")) == "":
                    raise ThrowSignal(self.validation_error(f"mangler query-felt '{key}'"))
                return self._parse_int_value(query.get(key, ""), key, "query")
            if func_name == "request_json":
                if not values:
                    return {}
                ctx = self._decode_json_text_map(values[0])
                return self._request_json_object(ctx)
            if func_name == "request_json_field":
                if len(values) < 2:
                    return ""
                ctx = self._decode_json_text_map(values[0])
                data = self._request_json_object(ctx)
                key = str(values[1])
                if key not in data or str(data.get(key, "")) == "":
                    raise ThrowSignal(self.validation_error(f"mangler felt '{key}'"))
                return str(data.get(key, ""))
            if func_name == "request_json_field_or":
                if len(values) < 3:
                    return ""
                ctx = self._decode_json_text_map(values[0])
                data = self._request_json_object(ctx)
                key = str(values[1])
                fallback = str(values[2])
                if key not in data or str(data.get(key, "")) == "":
                    return fallback
                return str(data.get(key, ""))
            if func_name == "request_json_field_int":
                if len(values) < 2:
                    return 0
                ctx = self._decode_json_text_map(values[0])
                data = self._request_json_object(ctx)
                key = str(values[1])
                if key not in data or str(data.get(key, "")) == "":
                    raise ThrowSignal(self.validation_error(f"mangler felt '{key}'"))
                return self._parse_int_value(data.get(key, ""), key, "body")
            if func_name == "request_json_field_bool":
                if len(values) < 2:
                    return False
                ctx = self._decode_json_text_map(values[0])
                data = self._request_json_object(ctx)
                key = str(values[1])
                if key not in data or str(data.get(key, "")) == "":
                    raise ThrowSignal(self.validation_error(f"mangler felt '{key}'"))
                return self._parse_bool_value(data.get(key, ""), key, "body")
            if func_name == "request_query_param":
                if len(values) < 2:
                    return ""
                ctx = self._decode_json_text_map(values[0])
                query = self._decode_json_text_map(ctx.get("query", "{}"))
                return str(query.get(str(values[1]), ""))
            if func_name == "request_header":
                if len(values) < 2:
                    return ""
                ctx = self._decode_json_text_map(values[0])
                headers = self._decode_json_text_map(ctx.get("headers", "{}"))
                return str(headers.get(str(values[1]), ""))
            if func_name == "request_cookie":
                if len(values) < 2:
                    return ""
                ctx = self._decode_json_text_map(values[0])
                headers = self._decode_json_text_map(ctx.get("headers", "{}"))
                cookie_header = str(headers.get("cookie", ""))
                target = str(values[1])
                for chunk in cookie_header.split(";"):
                    part = chunk.strip()
                    if not part or "=" not in part:
                        continue
                    key, cookie_value = part.split("=", 1)
                    if key.strip() == target:
                        return cookie_value.strip()
                return ""
            if func_name == "request_cookie_or":
                if len(values) < 3:
                    return ""
                ctx = self._decode_json_text_map(values[0])
                headers = self._decode_json_text_map(ctx.get("headers", "{}"))
                cookie_header = str(headers.get("cookie", ""))
                target = str(values[1])
                for chunk in cookie_header.split(";"):
                    part = chunk.strip()
                    if not part or "=" not in part:
                        continue
                    key, cookie_value = part.split("=", 1)
                    if key.strip() == target:
                        value = cookie_value.strip()
                        if value:
                            return value
                        break
                return str(values[2])
            if func_name == "escape_html":
                if not values:
                    return ""
                return self._escape_html(values[0])
            if func_name == "safe_filename":
                if not values:
                    return "fil"
                return self._safe_filename(values[0])
            if func_name == "safe_path_segment":
                if not values:
                    return "segment"
                return self._safe_path_segment(values[0])
            if func_name == "safe_slug":
                if not values:
                    return "slug"
                return self._safe_slug(values[0])
            if func_name == "auth_header":
                if not values:
                    return ""
                ctx = self._decode_json_text_map(values[0])
                headers = self._decode_json_text_map(ctx.get("headers", "{}"))
                return str(headers.get("authorization", ""))
            if func_name == "bearer_token":
                if not values:
                    return ""
                return self._extract_bearer_token(values[0])
            if func_name == "require_bearer":
                if len(values) < 2:
                    return False
                return self._extract_bearer_token(values[0]) == str(values[1])
            if func_name == "role":
                if not values:
                    return ""
                return self._web_role(values[0])
            if func_name == "has_role":
                if len(values) < 2:
                    return False
                return self._web_has_role(values[0], values[1])
            if func_name == "require_role":
                if len(values) < 2:
                    return False
                return self._web_has_role(values[0], values[1])
            if func_name == "has_permission":
                if len(values) < 2:
                    return False
                return self._web_has_permission(values[0], values[1])
            if func_name == "require_permission":
                if len(values) < 2:
                    return False
                return self._web_has_permission(values[0], values[1])
            if func_name == "passord_hash":
                if len(values) < 2:
                    return ""
                return self._password_hash(values[0], values[1])
            if func_name == "passord_verifiser":
                if len(values) < 2:
                    return False
                return self._password_verify(values[0], values[1])
            if func_name == "csrf_token":
                if not values:
                    return ""
                return self._csrf_token(values[0])
            if func_name == "csrf_require":
                if len(values) < 2:
                    return False
                return self._csrf_require(values[0], values[1])
            if func_name == "response_builder":
                if len(values) < 3:
                    return self._make_web_response(200, {}, "")
                return self._make_web_response(values[0], values[1], values[2])
            if func_name == "response_status":
                if not values:
                    return 0
                ctx = self._decode_json_text_map(values[0])
                return int(ctx.get("status", "0"))
            if func_name == "response_headers":
                if not values:
                    return {}
                ctx = self._decode_json_text_map(values[0])
                return self._decode_json_text_map(ctx.get("headers", "{}"))
            if func_name == "response_body":
                if not values:
                    return ""
                ctx = self._decode_json_text_map(values[0])
                return str(ctx.get("body", ""))
            if func_name == "response_header":
                if len(values) < 2:
                    return ""
                ctx = self._decode_json_text_map(values[0])
                headers = self._decode_json_text_map(ctx.get("headers", "{}"))
                return str(headers.get(str(values[1]), ""))
            if func_name == "cookie_header":
                if len(values) < 2:
                    return ""
                name = str(values[0]).strip()
                if not name:
                    return ""
                value = str(values[1])
                return f"{name}={value}; Path=/; HttpOnly; Secure; SameSite=Lax"
            if func_name == "response_set_cookie":
                if len(values) < 2:
                    return self._make_web_response(0, {}, "")
                response = self._decode_json_text_map(values[0])
                cookie_text = str(values[1]).strip()
                if not cookie_text:
                    return response
                headers = self._decode_json_text_map(response.get("headers", "{}"))
                existing = str(headers.get("set-cookie", "")).strip()
                headers["set-cookie"] = f"{existing}\n{cookie_text}" if existing else cookie_text
                return self._make_web_response(response.get("status", 200), headers, response.get("body", ""))
            if func_name == "response_text":
                if not values:
                    return ""
                ctx = self._decode_json_text_map(values[0])
                return str(ctx.get("body", ""))
            if func_name == "response_json":
                if not values:
                    return {}
                ctx = self._decode_json_text_map(values[0])
                body = str(ctx.get("body", ""))
                try:
                    parsed = json.loads(body)
                except Exception:
                    return {}
                if isinstance(parsed, dict):
                    return {str(key): self.json_scalar_to_text(val) for key, val in parsed.items()}
                if isinstance(parsed, list):
                    return {str(i): self.json_scalar_to_text(val) for i, val in enumerate(parsed)}
                return {}
            if func_name == "response_error":
                if len(values) < 2:
                    return self._make_web_response(500, {"content-type": "application/json"}, self.json_serialize_value({"error": ""}))
                body = self.json_serialize_value({"error": str(values[1])})
                return self._make_web_response(values[0], {"content-type": "application/json"}, body)
            if func_name == "response_file":
                if len(values) < 2:
                    raise RuntimeError("web.response_file forventer 2 argumenter")
                body = self.eval_call("fil_les", [values[0]])
                return self._make_web_response(200, {"content-type": values[1]}, body)
            if func_name == "openapi_json":
                title = str(values[0]) if values else "Norscode API"
                version = str(values[1]) if len(values) > 1 else "1.0.0"
                return self._build_openapi_document(title, version)
            if func_name == "docs_html":
                title = str(values[0]) if values else "Norscode API"
                version = str(values[1]) if len(values) > 1 else "1.0.0"
                return self._build_docs_html(title, version)
            if func_name == "route":
                if not values:
                    return ""
                return str(values[0])
            if func_name in {"router", "subrouter"}:
                if not values:
                    return ""
                return str(values[0])
            if func_name == "guard":
                return "guard"
            if func_name == "dependency":
                if not values:
                    return ""
                return str(values[0])
            if func_name == "use_dependency":
                if not values:
                    return ""
                return str(values[0])
            if func_name == "use_guard":
                if not values:
                    return ""
                return str(values[0])
            if func_name == "request_dependency":
                if len(values) < 2:
                    return {}
                dep_name = str(values[1])
                try:
                    return self._resolve_dependency_value(values[0], dep_name)
                except RuntimeError as exc:
                    raise ThrowSignal(self.http_error(str(exc)))
            if func_name == "handle_request":
                if not values:
                    return self._make_web_response(404, {"content-type": "application/json"}, self.json_serialize_value({"error": "Ikke funnet"}))
                self._ensure_startup_hooks()
                try:
                    current_ctx = self._apply_request_middlewares(values[0])
                    method_text = self._normalize_web_method(current_ctx.get("metode", ""))
                    path_text = str(current_ctx.get("sti", ""))
                    if method_text == "OPTIONS":
                        allow_methods = self._allow_web_methods_for_path(path_text)
                        if allow_methods:
                            response = self._make_web_response(204, {"allow": ", ".join(allow_methods)}, "")
                            return self._apply_response_middlewares(response)
                        response = self._make_web_response(404, {"content-type": "application/json"}, self.json_serialize_value({"error": "Ikke funnet"}))
                        return self._apply_error_middlewares(response)
                    found = self._find_route_handler(method_text, path_text)
                    if found is None:
                        response = self._make_web_response(404, {"content-type": "application/json"}, self.json_serialize_value({"error": "Ikke funnet"}))
                        return self._apply_error_middlewares(response)
                    params, handler = found
                    if params == "__405__":
                        allow_methods = self._allow_web_methods_for_path(path_text)
                        response = self._make_web_response(405, {"content-type": "application/json", "allow": ", ".join(allow_methods)}, self.json_serialize_value({"error": "Metode ikke tillatt"}))
                        return self._apply_error_middlewares(response)
                    ctx = self._make_route_context(current_ctx, params)
                    deps = []
                    for dep_name in handler.get("deps", []):
                        deps.append(self._resolve_dependency_value(ctx, dep_name))
                    for guard_name in handler.get("guards", []):
                        guard_fn = self.guard_providers.get(guard_name, guard_name)
                        guard_result = self.call_user_function(guard_fn, [ctx])
                        if not bool(guard_result):
                            response = self._make_web_response(403, {"content-type": "application/json"}, self.json_serialize_value({"error": "Forbudt"}))
                            return self._apply_error_middlewares(response)
                    result = self.call_user_function(handler["function"], [ctx] + deps)
                    if isinstance(result, dict):
                        result = self._apply_response_middlewares(result)
                        if method_text == "HEAD":
                            result = dict(result)
                            result["body"] = ""
                        return result
                    return result
                except ThrowSignal as signal:
                    response = self._web_error_response(str(signal), 500)
                    return self._apply_error_middlewares(response)
                except Exception as exc:
                    response = self._web_error_response(str(exc), 500)
                    return self._apply_error_middlewares(response)
            if func_name == "request_middleware":
                return "request_middleware"
            if func_name == "response_middleware":
                return "response_middleware"
            if func_name == "error_middleware":
                return "error_middleware"
            if func_name == "startup_hook":
                return "startup_hook"
            if func_name == "shutdown_hook":
                return "shutdown_hook"
            if func_name == "startup":
                return self._ensure_startup_hooks()
            if func_name == "shutdown":
                return self._run_shutdown_hooks()
        if module_name in ("sikkerhet", "std.sikkerhet"):
            if func_name == "passord_hash":
                if len(values) < 2:
                    return ""
                return self._password_hash(values[0], values[1])
            if func_name == "passord_verifiser":
                if len(values) < 2:
                    return False
                return self._password_verify(values[0], values[1])
        if module_name in ("csrf", "std.csrf"):
            if func_name == "token":
                if not values:
                    return ""
                return self._csrf_token(values[0])
            if func_name == "require":
                if len(values) < 2:
                    return False
                return self._csrf_require(values[0], values[1])
        if module_name in ("feil", "std.feil"):
            return self.call_user_function(func_name, values)

        raise RuntimeError(f"Ukjent modulfunksjon: {module_name}.{func_name}")

    def call_user_function(self, name: str, args: list[Any]):
        fn = self.functions[name].node
        self.push_scope()
        self.call_stack.append(name)
        try:
            for param, value in zip(fn.params, args):
                self.define_var(param.name, value)

            try:
                self.eval(fn.body)
            except ReturnSignal as signal:
                result = signal.value
                if fn.is_async:
                    return AsyncValue(result)
                return result
            except ThrowSignal as signal:
                signal.call_stack.append(name)
                raise
            result = None
            if fn.is_async:
                return AsyncValue(result)
            return result
        finally:
            self.call_stack.pop()
            self.pop_scope()
