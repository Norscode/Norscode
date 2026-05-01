

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

    def _match_web_route_spec(self, spec: Any, method: Any, path: Any) -> dict[str, str] | None:
        route_method, route_path = self._split_web_route_spec(spec)
        request_method = self._normalize_web_method(method)
        if route_method and route_method != request_method:
            return None
        return self._match_web_path(route_path, path)

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

    def _make_web_request_context(self, method: Any, path: Any, query: Any, headers: Any, body: Any) -> dict[str, Any]:
        return {
            "metode": self._normalize_web_method(method),
            "sti": str(path),
            "query": self._encode_json_text_map(query),
            "headers": self._encode_json_text_map(headers),
            "body": "" if body is None else str(body),
            "params": self._encode_json_text_map({}),
            "deps": self._encode_json_text_map({}),
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
        for fn in getattr(program, "functions", []):
            statements = list(getattr(getattr(fn, "body", None), "statements", []) or [])
            if not statements:
                continue
            spec = None
            deps: list[str] = []
            provided: list[str] = []
            for stmt in statements:
                if not isinstance(stmt, ExprStmtNode):
                    break
                expr = stmt.expr
                if not isinstance(expr, ModuleCallNode):
                    break
                if expr.module_name not in {"web", "std.web"}:
                    break
                if not expr.args or not isinstance(expr.args[0], StringNode):
                    break
                if expr.func_name == "route" and spec is None:
                    spec = expr.args[0].value
                    continue
                if expr.func_name == "use_dependency":
                    deps.append(expr.args[0].value)
                    continue
                if expr.func_name == "dependency":
                    provided.append(expr.args[0].value)
                    continue
                break
            if spec is not None:
                handlers.append({
                    "function": fn.name,
                    "spec": spec,
                    "deps": deps,
                })
            for dep_name in provided:
                providers[dep_name] = fn.name
        return handlers, providers

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
        self.route_handlers, self.dependency_providers = self._collect_web_annotations(program)
        for fn in getattr(program, "functions", []):
            self.functions[fn.name] = UserFunction(fn)

        for fn in getattr(program, "tests", []):
            self.functions[fn.name] = UserFunction(fn)

        if "start" not in self.functions:
            return None
        return self.call_user_function("start", [])

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
            if func_name == "route":
                if not values:
                    return ""
                return str(values[0])
            if func_name == "dependency":
                if not values:
                    return ""
                return str(values[0])
            if func_name == "use_dependency":
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
                found = self._find_route_handler(values[0].get("metode", ""), values[0].get("sti", ""))
                if found is None:
                    return self._make_web_response(404, {"content-type": "application/json"}, self.json_serialize_value({"error": "Ikke funnet"}))
                params, handler = found
                if params == "__405__":
                    return self._make_web_response(405, {"content-type": "application/json"}, self.json_serialize_value({"error": "Metode ikke tillatt"}))
                ctx = self._make_route_context(values[0], params)
                deps = []
                for dep_name in handler.get("deps", []):
                    deps.append(self._resolve_dependency_value(ctx, dep_name))
                return self.call_user_function(handler["function"], [ctx] + deps)
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
