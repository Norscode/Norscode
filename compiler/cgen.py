from .ast_nodes import *


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

    def c_type(self, t):
        return {
            TYPE_INT: "int",
            TYPE_BOOL: "int",
            TYPE_TEXT: "char *",
            TYPE_LIST_INT: "nl_list_int*",
            TYPE_LIST_TEXT: "nl_list_text*",
        }[t]

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
        self.emit("#include <stdio.h>")
        self.emit("#include <stdlib.h>")
        self.emit("#include <string.h>")
        self.emit("#include <ctype.h>")
        self.emit()
        self.emit_runtime_helpers()

        for fn in tree.functions:
            self.visit_function(fn)
            self.emit()

        self.emit("int main(void) {")
        self.indent += 1
        self.emit("return start();")
        self.indent -= 1
        self.emit("}")

        return "\n".join(self.lines)

    def emit_runtime_helpers(self):
        self.emit("typedef struct { int *data; int len; int cap; } nl_list_int;")
        self.emit("typedef struct { char **data; int len; int cap; } nl_list_text;")
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

        self.emit("static void nl_print_text(const char *s) {")
        self.indent += 1
        self.emit('printf("%s\\n", s ? s : "");')
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

    def visit_function(self, fn):
        previous_module = self.current_module
        previous_return_type = self.current_return_type
        self.current_module = getattr(fn, "module_name", "__main__")
        self.current_return_type = fn.return_type
        self.var_types = {p.name: p.type_name for p in fn.params}

        self.emit(self.signature(fn) + " {")
        self.indent += 1

        for stmt in fn.body.statements:
            self.visit_stmt(stmt)

        if fn.return_type in (TYPE_INT, TYPE_BOOL):
            self.emit("return 0;")
        elif fn.return_type == TYPE_TEXT:
            self.emit('return "";')
        elif fn.return_type == TYPE_LIST_INT:
            self.emit("return nl_list_int_new();")
        elif fn.return_type == TYPE_LIST_TEXT:
            self.emit("return nl_list_text_new();")
        else:
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
                else:
                    _, inferred_type = self.expr_with_type(stmt.expr)
            self.var_types[stmt.name] = inferred_type
            if isinstance(stmt.expr, ListLiteralNode):
                self.emit_list_literal_assignment(stmt.name, inferred_type, stmt.expr, declare=True)
                return
            expr_code, _expr_type = self.expr_with_type(stmt.expr)
            self.emit(f"{self.c_type(inferred_type)} {stmt.name} = {expr_code};")
            return

        if isinstance(stmt, VarSetNode):
            var_type = self.var_types.get(stmt.name)
            if isinstance(stmt.expr, ListLiteralNode) and var_type in (TYPE_LIST_INT, TYPE_LIST_TEXT):
                self.emit_list_literal_assignment(stmt.name, var_type, stmt.expr, declare=False)
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
            self.emit(f"return {expr_code};")
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

        if isinstance(node, VarAccessNode):
            return node.name, self.var_types.get(node.name, TYPE_INT)

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
            if then_type != else_type:
                return "0", TYPE_INT
            return f"(({cond_code}) ? {then_code} : {else_code})", then_type

        if isinstance(node, ModuleCallNode):
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

            if node.name == "tekst_fra_heltall":
                return f"nl_int_to_text({args[0]})", TYPE_TEXT

            if node.name == "tekst_fra_bool":
                return f"nl_bool_to_text({args[0]})", TYPE_TEXT

            if node.name == "heltall_fra_tekst":
                return f"nl_text_to_int({args[0]})", TYPE_INT

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

            c_name = self.resolve_c_function_name(node.name)
            return_type = symbol.return_type if symbol else TYPE_INT
            return f"{c_name}({', '.join(args)})", return_type

        if isinstance(node, IndexNode):
            target_expr = getattr(node, "list_expr", getattr(node, "target", None))
            list_code, list_type = self.expr_with_type(target_expr)
            idx_code, _ = self.expr_with_type(getattr(node, "index_expr", None))
            if list_type == TYPE_LIST_TEXT:
                return f"{list_code}->data[{idx_code}]", TYPE_TEXT
            return f"{list_code}->data[{idx_code}]", TYPE_INT

        return "0", TYPE_INT
