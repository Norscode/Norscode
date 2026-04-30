from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re


NAME_RE = re.compile(r'[A-Za-zÆØÅæøå_][A-Za-zÆØÅæøå0-9_]*')

TOKEN_ALIASES = {
    'let': 'la', 'const': 'la', 'var': 'la', 'declare': 'la',
    'set': 'sett', 'assign': 'sett', 'return': 'returner',
    'if': 'hvis', 'then': 'da', 'else': 'ellers',
    'elseif': 'ellers_hvis', 'elsif': 'ellers_hvis', 'elif': 'ellers_hvis', 'ellershvis': 'ellers_hvis',
    'while': 'mens', 'break': 'bryt', 'continue': 'fortsett',
    'and': 'og', 'or': 'eller', 'not': 'ikke', '&&': 'og', '||': 'eller',
    'xor': 'xor', 'nand': 'nand', 'nor': 'nor', 'xnor': 'xnor',
    'impl': 'impliserer', 'implies': 'impliserer', 'implies_that': 'impliserer',
    'impliser': 'impliserer', 'impliserer_at': 'impliserer', 'this_implies': 'impliserer', 'dette_impliserer': 'impliserer',
    'implied_by': 'impliseres_av', 'given': 'impliseres_av', 'given_that': 'impliseres_av',
    'provided': 'impliseres_av', 'provided_that': 'impliseres_av', 'assuming': 'impliseres_av', 'assuming_that': 'impliseres_av',
    'siden': 'impliseres_av', 'fordi': 'impliseres_av',
    'iff': 'ekvivalent', 'equiv': 'ekvivalent', 'equivalent': 'ekvivalent', 'ekvivalent': 'ekvivalent',
    'and_not': 'nand', 'or_not': 'nor', 'og_ikke': 'nand', 'eller_ikke': 'nor', 'xeller': 'xor', 'xeller_ikke': 'xnor',
    'is': '==', 'eq': '==', 'equals': '==', 'equal_to': '==', 'is_equal_to': '==',
    'isnt': '!=', 'neq': '!=', '<>': '!=', 'is_not': '!=', 'not_equal_to': '!=', 'is_not_equal_to': '!=',
    'less_than': '<', 'is_less_than': '<', 'greater_than': '>', 'is_greater_than': '>',
    'less_equal': '<=', 'less_or_equal': '<=', 'is_less_or_equal': '<=', 'is_less_than_or_equal_to': '<=',
    'greater_equal': '>=', 'greater_or_equal': '>=', 'is_greater_or_equal': '>=', 'is_greater_than_or_equal_to': '>=',
    'plus': '+', 'times': '*', 'multiplied_by': '*',
    'divided_by': '/', 'divide_by': '/', 'delt_pa': '/', 'delt_på': '/', 'delt_med': '/',
    'modulo': '%', 'modulo_of': '%', 'mod_of': '%', 'remainder': '%', 'remainder_of': '%',
    'on': 'sann', 'yes': 'sann', 'enabled': 'sann', 'active': 'sann', 'truthy': 'sann', 'ready': 'sann', 'ok': 'sann',
    'safe': 'sann', 'secure': 'sann', 'trusted': 'sann', 'visible': 'sann', 'online': 'sann', 'connected': 'sann',
    'available': 'sann', 'reachable': 'sann', 'working': 'sann', 'stable': 'sann', 'correct': 'sann', 'complete': 'sann',
    'clean': 'sann', 'up': 'sann', 'alive': 'sann', 'healthy': 'sann', 'synced': 'sann',
    'off': 'usann', 'no': 'usann', 'disabled': 'usann', 'inactive': 'usann', 'falsy': 'usann', 'not_ready': 'usann',
    'not_ok': 'usann', 'unsafe': 'usann', 'insecure': 'usann', 'untrusted': 'usann', 'hidden': 'usann', 'offline': 'usann',
    'disconnected': 'usann', 'unavailable': 'usann', 'unreachable': 'usann', 'broken': 'usann', 'unstable': 'usann',
    'incorrect': 'usann', 'incomplete': 'usann', 'dirty': 'usann', 'down': 'usann', 'dead': 'usann', 'unhealthy': 'usann',
    'unsynced': 'usann',
}

PHRASE_ALIASES = {
    ('is', 'equal', 'to'): ['=='],
    ('equal', 'to'): ['=='],
    ('is', 'not', 'equal', 'to'): ['!='],
    ('not', 'equal', 'to'): ['!='],
    ('less', 'than'): ['<'],
    ('is', 'less', 'than'): ['<'],
    ('greater', 'than'): ['>'],
    ('is', 'greater', 'than'): ['>'],
    ('less', 'than', 'or', 'equal', 'to'): ['<='],
    ('greater', 'than', 'or', 'equal', 'to'): ['>='],
    ('ganget', 'med'): ['*'],
    ('multiply', 'by'): ['*'],
    ('multiplied', 'by'): ['*'],
    ('divided', 'by'): ['/'],
    ('divide', 'by'): ['/'],
    ('delt', 'pa'): ['/'],
    ('delt', 'på'): ['/'],
    ('delt', 'med'): ['/'],
    ('mindre', 'enn'): ['<'],
    ('storre', 'enn'): ['>'],
    ('større', 'enn'): ['>'],
    ('mindre', 'enn', 'eller', 'lik'): ['<='],
    ('storre', 'enn', 'eller', 'lik'): ['>='],
    ('større', 'enn', 'eller', 'lik'): ['>='],
    ('er', 'ikke'): ['!='],
    ('ikke', 'er'): ['!='],
    ('ellers', 'hvis'): ['ellers_hvis'],
    ('enten', 'eller'): ['xor'],
}


@dataclass
class Token:
    value: str
    line: int
    column: int
    raw: str | None = None


@dataclass
class SelfhostImport:
    module_name: str
    alias: str | None = None


@dataclass
class SelfhostFunction:
    name: str
    params: list[str]
    return_type: str | None
    body_tokens: list[str]
    body_ast: list[dict]


class ParseError(RuntimeError):
    def __init__(self, message: str, token: Token | None = None):
        self.message = message
        self.token = token
        if token is None:
            super().__init__(message)
        else:
            super().__init__(f"{message} ved linje {token.line}, kolonne {token.column} (token={token.value!r})")


class Parser:
    def __init__(self, tokens: list[Token]):
        self.tokens = tokens
        self.i = 0

    def peek_token(self, offset: int = 0) -> Token | None:
        idx = self.i + offset
        return self.tokens[idx] if 0 <= idx < len(self.tokens) else None

    def peek(self, offset: int = 0) -> str | None:
        tok = self.peek_token(offset)
        return tok.value if tok is not None else None

    def at_end(self) -> bool:
        return self.i >= len(self.tokens)

    def error(self, message: str, token: Token | None = None) -> ParseError:
        return ParseError(message, token or self.peek_token())

    def advance_token(self) -> Token:
        tok = self.peek_token()
        if tok is None:
            raise ParseError('Uventet slutt på input')
        self.i += 1
        return tok

    def advance(self) -> str:
        return self.advance_token().value

    def match(self, *choices: str) -> bool:
        tok = self.peek()
        if tok in choices:
            self.i += 1
            return True
        return False

    def expect(self, expected: str) -> str:
        tok = self.advance_token()
        if tok.value != expected:
            raise ParseError(f"Forventet {expected!r}, fikk {tok.value!r}", tok)
        return tok.value

    def expect_name(self) -> str:
        tok = self.advance_token()
        if not NAME_RE.fullmatch(tok.value):
            raise ParseError(f"Forventet navn, fikk {tok.value!r}", tok)
        return tok.value

    def consume_dotted_name(self) -> str:
        parts = [self.expect_name()]
        while self.match('.'):
            parts.append(self.expect_name())
        return '.'.join(parts)

    def skip_statement_separators(self) -> None:
        while self.peek() in {';', ':', ','}:
            self.advance()

    def parse_program(self) -> dict:
        imports: list[SelfhostImport] = []
        functions: list[SelfhostFunction] = []
        while not self.at_end():
            tok = self.peek()
            if tok == 'bruk':
                self.advance()
                module_name = self.consume_dotted_name()
                alias = self.expect_name() if self.match('som') else None
                imports.append(SelfhostImport(module_name=module_name, alias=alias))
                continue
            if tok == 'funksjon':
                functions.append(self.parse_function())
                continue
            self.advance()
        return {
            'imports': [imp.__dict__ for imp in imports],
            'functions': [
                {
                    'name': fn.name,
                    'params': fn.params,
                    'return_type': fn.return_type,
                    'body_tokens': fn.body_tokens,
                    'body_token_count': len(fn.body_tokens),
                    'body_ast': fn.body_ast,
                    'statement_count': len(fn.body_ast),
                }
                for fn in functions
            ],
            'token_count': len(self.tokens),
            'ast_ready': True,
        }

    def parse_function(self) -> SelfhostFunction:
        self.expect('funksjon')
        name = self.consume_dotted_name()
        self.expect('(')
        params: list[str] = []
        while self.peek() != ')':
            params.append(self.expect_name())
            if self.match(':'):
                self.consume_dotted_name()
            if self.peek() == ',':
                self.advance()
                continue
            if self.peek() != ')':
                raise self.error(f"Forventet ',' eller ')' i parameterliste for {name}")
        self.expect(')')
        return_type = self.consume_dotted_name() if self.match('->') else None
        body_tokens, body_ast = self.parse_block_with_tokens()
        return SelfhostFunction(name=name, params=params, return_type=return_type, body_tokens=body_tokens, body_ast=body_ast)

    def parse_block_with_tokens(self) -> tuple[list[str], list[dict]]:
        start = self.i
        self.expect('{')
        stmts: list[dict] = []
        self.skip_statement_separators()
        while self.peek() != '}':
            if self.at_end():
                raise self.error('Ubalanserte klammer i selfhost-parser')
            stmts.append(self.parse_statement())
            self.skip_statement_separators()
        self.expect('}')
        end = self.i
        return [tok.value for tok in self.tokens[start + 1:end - 1]], stmts

    def parse_statement(self) -> dict:
        tok = self.peek()
        if tok == 'la':
            return self.parse_let_statement()
        if tok == 'returner':
            self.advance()
            return {'node': 'Return', 'value': self.parse_expression()}
        if tok == 'hvis':
            return self.parse_if_statement()
        if tok == 'mens':
            return self.parse_while_statement()
        if tok == 'for':
            return self.parse_for_statement()
        if tok == 'bryt':
            self.advance()
            return {'node': 'Break'}
        if tok == 'fortsett':
            self.advance()
            return {'node': 'Continue'}
        if tok == 'sett':
            return self.parse_assignment_statement(with_keyword=True)
        return self.parse_expr_or_assignment_statement()

    def parse_let_statement(self) -> dict:
        self.expect('la')
        name = self.expect_name()
        declared_type = self.consume_dotted_name() if self.match(':') else None
        self.expect('=')
        return {'node': 'Let', 'name': name, 'declared_type': declared_type, 'value': self.parse_expression()}

    def _parse_if_core(self, consume_keyword: bool) -> dict:
        start_tok = self.peek_token()
        if consume_keyword:
            self.expect('hvis')
        if self.match('('):
            condition = self.parse_expression()
            self.expect(')')
        else:
            condition = self.parse_implies()
        has_da = self.match('da')
        if self.peek() == '{' and not has_da:
            pass
        elif not has_da:
            raise self.error("hvis-uttrykk mangler 'da'", start_tok)
        if self.peek() == '{':
            then_tokens, then_block = self.parse_block_with_tokens()
            else_block = None
            else_tokens: list[str] | None = None
            if self.match('ellers_hvis'):
                nested = self._parse_if_core(consume_keyword=False)
                else_block, else_tokens = [nested], ['hvis']
            elif self.match('ellers'):
                if self.peek() == 'hvis':
                    nested = self.parse_if_statement()
                    else_block, else_tokens = [nested], ['hvis']
                else:
                    else_tokens, else_block = self.parse_block_with_tokens()
            return {
                'node': 'If',
                'condition': condition,
                'then': then_block,
                'then_tokens': then_tokens,
                'else': else_block,
                'else_tokens': else_tokens,
            }
        then_expr = self.parse_expression()
        if not self.match('ellers'):
            raise self.error("hvis-uttrykk mangler 'ellers'")
        else_expr = self.parse_expression()
        return {'node': 'IfExprStmt', 'value': {'node': 'IfExpr', 'condition': condition, 'then': then_expr, 'else': else_expr}}

    def parse_if_statement(self) -> dict:
        return self._parse_if_core(consume_keyword=True)

    def parse_if_statement_from_alias(self) -> dict:
        return self._parse_if_core(consume_keyword=False)

    def parse_while_statement(self) -> dict:
        self.expect('mens')
        if self.match('('):
            condition = self.parse_expression()
            self.expect(')')
        else:
            condition = self.parse_expression()
        self.match('da')
        body_tokens, body = self.parse_block_with_tokens()
        return {'node': 'While', 'condition': condition, 'body': body, 'body_tokens': body_tokens}

    def parse_for_statement(self) -> dict:
        self.expect('for')
        name = self.expect_name()
        if self.match('='):
            start_expr = self.parse_expression()
            if not self.match('til'):
                raise self.error("Forventet 'til' i for-range")
            end_expr = self.parse_expression()
            self.match('da')
            body_tokens, body = self.parse_block_with_tokens()
            return {'node': 'ForRange', 'name': name, 'start': start_expr, 'end': end_expr, 'body': body, 'body_tokens': body_tokens}
        if self.match('i'):
            iterable = self.parse_expression()
            self.match('da')
            body_tokens, body = self.parse_block_with_tokens()
            return {'node': 'ForEach', 'name': name, 'iterable': iterable, 'body': body, 'body_tokens': body_tokens}
        raise self.error("Forventet '=' eller 'i' i for-setning")

    def parse_assignment_statement(self, with_keyword: bool) -> dict:
        if with_keyword:
            self.expect('sett')
        target = self.parse_assignable()
        op = self.peek()
        if op not in {'=', '+=', '-=', '*=', '/=', '%='}:
            raise self.error('Forventet assignment-operator')
        self.advance()
        value = self.parse_expression()
        return {'node': 'Assign', 'target': target, 'op': op, 'value': value}

    def parse_expr_or_assignment_statement(self) -> dict:
        checkpoint = self.i
        try:
            target = self.parse_assignable()
            op = self.peek()
            if op in {'=', '+=', '-=', '*=', '/=', '%='}:
                self.advance()
                value = self.parse_expression()
                return {'node': 'Assign', 'target': target, 'op': op, 'value': value}
            self.i = checkpoint
        except ParseError:
            self.i = checkpoint
        return {'node': 'ExprStmt', 'value': self.parse_expression()}

    def parse_assignable(self) -> dict:
        expr = self.parse_primary()
        while True:
            if self.match('['):
                expr = {'node': 'Index', 'target': expr, 'index': self.parse_expression()}
                self.expect(']')
                continue
            if self.match('.'):
                expr = {'node': 'Member', 'target': expr, 'name': self.expect_name()}
                continue
            break
        return expr

    def parse_expression(self) -> dict:
        if self.peek() == 'hvis':
            return self.parse_if_expression()
        return self.parse_implies()

    def parse_if_expression(self) -> dict:
        start_tok = self.peek_token()
        self.expect('hvis')
        condition = self.parse_implies()
        if not self.match('da'):
            raise self.error("hvis-uttrykk mangler 'da'", start_tok)
        then_expr = self.parse_expression()
        if self.match('ellers_hvis'):
            if self.peek() != 'hvis':
                raise self.error("hvis-uttrykk mangler 'hvis' etter 'ellers_hvis'")
            return {'node': 'IfExpr', 'condition': condition, 'then': then_expr, 'else': self.parse_if_expression()}
        if not self.match('ellers'):
            raise self.error("hvis-uttrykk mangler 'ellers'")
        else_expr = self.parse_expression()
        return {'node': 'IfExpr', 'condition': condition, 'then': then_expr, 'else': else_expr}

    def parse_implies(self) -> dict:
        expr = self.parse_or_family()
        while self.peek() in {'impliserer', 'impliseres_av'}:
            expr = {'node': 'BinaryOp', 'op': self.advance(), 'left': expr, 'right': self.parse_or_family()}
        return expr

    def parse_or_family(self) -> dict:
        expr = self.parse_and_family()
        while self.peek() in {'eller', 'xor', 'xnor', 'nor', 'ekvivalent'}:
            expr = {'node': 'BinaryOp', 'op': self.advance(), 'left': expr, 'right': self.parse_and_family()}
        return expr

    def parse_and_family(self) -> dict:
        expr = self.parse_comparison()
        while self.peek() in {'og', 'nand'}:
            expr = {'node': 'BinaryOp', 'op': self.advance(), 'left': expr, 'right': self.parse_comparison()}
        return expr

    def parse_comparison(self) -> dict:
        expr = self.parse_term()
        while self.peek() in {'==', '!=', '<', '>', '<=', '>='}:
            expr = {'node': 'BinaryOp', 'op': self.advance(), 'left': expr, 'right': self.parse_term()}
        return expr

    def parse_term(self) -> dict:
        expr = self.parse_factor()
        while self.peek() in {'+', '-'}:
            expr = {'node': 'BinaryOp', 'op': self.advance(), 'left': expr, 'right': self.parse_factor()}
        return expr

    def parse_factor(self) -> dict:
        expr = self.parse_unary()
        while self.peek() in {'*', '/', '%'}:
            expr = {'node': 'BinaryOp', 'op': self.advance(), 'left': expr, 'right': self.parse_unary()}
        return expr

    def parse_unary(self) -> dict:
        if self.peek() in {'-', '+', 'ikke'}:
            return {'node': 'UnaryOp', 'op': self.advance(), 'value': self.parse_unary()}
        return self.parse_postfix()

    def parse_postfix(self) -> dict:
        expr = self.parse_primary()
        while True:
            if self.match('('):
                args: list[dict] = []
                while self.peek() != ')':
                    args.append(self.parse_expression())
                    if self.peek() == ',':
                        self.advance()
                        continue
                    if self.peek() != ')':
                        raise self.error("Forventet ',' eller ')' i argumentliste")
                self.expect(')')
                expr = {'node': 'Call', 'callee': expr, 'args': args}
                continue
            if self.match('['):
                expr = {'node': 'Index', 'target': expr, 'index': self.parse_expression()}
                self.expect(']')
                continue
            if self.match('.'):
                expr = {'node': 'Member', 'target': expr, 'name': self.expect_name()}
                continue
            break
        return expr

    def parse_primary(self) -> dict:
        tok = self.peek_token()
        if tok is None:
            raise ParseError('Uventet slutt i uttrykk')
        if tok.value == '(':
            self.advance()
            expr = self.parse_expression()
            self.expect(')')
            return expr
        if tok.value == '{':
            self.advance()
            if self.peek() == '}':
                self.advance()
                return {'node': 'MapLiteral', 'items': []}
            entries: list[dict[str, dict]] = []
            while True:
                key = self.parse_expression()
                if not self.match(':'):
                    raise self.error("Forventet ':' i map-literal")
                value = self.parse_expression()
                entries.append({'key': key, 'value': value})
                if self.match(','):
                    if self.peek() == '}':
                        raise self.error("Forventet nøkkel:verdi i map-literal etter ','")
                    continue
                break
            self.expect('}')
            return {'node': 'MapLiteral', 'items': entries}
        if tok.value == '[':
            self.advance()
            if self.peek() == ']':
                self.expect(']')
                return {'node': 'ListLiteral', 'items': []}
            first = self.parse_expression()
            if self.peek() == ',':
                items = [first]
                while self.peek() == ',':
                    self.advance()
                    if self.peek() == ']':
                        break
                    items.append(self.parse_expression())
                self.expect(']')
                return {'node': 'ListLiteral', 'items': items}
            self.expect(']')
            return first
        if tok.value.startswith('"'):
            self.advance()
            return {'node': 'Literal', 'literal_type': 'tekst', 'value': _decode_string_literal(tok.value)}
        if tok.value.isdigit():
            self.advance()
            return {'node': 'Literal', 'literal_type': 'heltall', 'value': int(tok.value)}
        if tok.value in {'sann', 'usann'}:
            self.advance()
            return {'node': 'Literal', 'literal_type': 'bool', 'value': tok.value == 'sann'}
        if tok.value == '[':
            self.advance()
            items: list[dict] = []
            while self.peek() != ']':
                items.append(self.parse_expression())
                if self.peek() == ',':
                    self.advance()
                    continue
                if self.peek() != ']':
                    raise self.error("Forventet ',' eller ']' i liste-literal")
            self.expect(']')
            return {'node': 'ListLiteral', 'items': items}
        if NAME_RE.fullmatch(tok.value):
            self.advance()
            return {'node': 'Name', 'value': tok.value}
        raise ParseError(f"Ugyldig uttrykkstoken: {tok.value}", tok)


def _strip_comments(source: str) -> str:
    out_lines = []
    for line in source.splitlines():
        out = []
        in_string = False
        escaped = False
        for ch in line:
            if in_string:
                out.append(ch)
                if escaped:
                    escaped = False
                elif ch == '\\':
                    escaped = True
                elif ch == '"':
                    in_string = False
                continue
            if ch == '"':
                in_string = True
                out.append(ch)
                continue
            if ch == '#':
                break
            out.append(ch)
        out_lines.append(''.join(out))
    return '\n'.join(out_lines)


def _tokenize(source: str) -> list[Token]:
    token_re = re.compile(
        r'<=>|<->|=>|->|<-|&&|\|\||\+=|-=|\*=|/=|%=|==|!=|<=|>=|<>|[=(){}\[\],.:;+\-*/%<>]|"[^"\\]*(?:\\.[^"\\]*)*"|'
        r'[A-Za-zÆØÅæøå_][A-Za-zÆØÅæøå0-9_]*|\d+'
    )
    tokens: list[Token] = []
    for line_no, line in enumerate(source.splitlines(), start=1):
        for match in token_re.finditer(line):
            raw = match.group(0)
            if raw.strip():
                tokens.append(Token(value=raw, raw=raw, line=line_no, column=match.start() + 1))
    return tokens


def _normalize_tokens(tokens: list[Token]) -> list[Token]:
    out: list[Token] = []
    i = 0
    max_phrase = max(len(k) for k in PHRASE_ALIASES)
    while i < len(tokens):
        matched = False
        for size in range(max_phrase, 1, -1):
            chunk = tuple(tok.value.lower() for tok in tokens[i:i + size])
            repl = PHRASE_ALIASES.get(chunk)
            if repl is not None:
                anchor = tokens[i]
                raw = ' '.join(t.raw or t.value for t in tokens[i:i + size])
                for item in repl:
                    out.append(Token(value=item, raw=raw, line=anchor.line, column=anchor.column))
                i += size
                matched = True
                break
        if matched:
            continue
        tok = tokens[i]
        low = tok.value.lower()
        if low == '=>':
            out.append(Token(value='impliserer', raw=tok.raw, line=tok.line, column=tok.column))
        elif low == '<-':
            out.append(Token(value='impliseres_av', raw=tok.raw, line=tok.line, column=tok.column))
        elif low in {'<->', '<=>'}:
            out.append(Token(value='ekvivalent', raw=tok.raw, line=tok.line, column=tok.column))
        else:
            out.append(Token(value=TOKEN_ALIASES.get(low, tok.value), raw=tok.raw, line=tok.line, column=tok.column))
        i += 1
    return out



def _decode_string_literal(raw: str) -> str:
    body = raw[1:-1]
    out: list[str] = []
    i = 0
    while i < len(body):
        ch = body[i]
        if ch == "\\" and i + 1 < len(body):
            nxt = body[i + 1]
            if nxt == 'n':
                out.append("\n")
            elif nxt == 't':
                out.append("\t")
            elif nxt == 'r':
                out.append("\r")
            elif nxt == '"':
                out.append('"')
            elif nxt == '\\':
                out.append('\\')
            else:
                out.append(nxt)
            i += 2
            continue
        out.append(ch)
        i += 1
    return ''.join(out)





def _finalize_parser(parse_fn, source: str) -> dict:
    cleaned = _strip_comments(source)
    tokens = _normalize_tokens(_tokenize(cleaned))
    parser = Parser(tokens)
    payload = parse_fn(parser)
    if not parser.at_end():
        raise parser.error('Uventede tokens etter fullført parsing')
    return {
        'normalized_tokens': [tok.value for tok in tokens],
        **payload,
    }


def parse_selfhost_expression(source: str) -> dict:
    def _parse(parser: Parser) -> dict:
        expr = parser.parse_expression()
        return {
            'kind': 'expression',
            'ast': expr,
            'token_count': len(parser.tokens),
            'ast_ready': True,
        }
    return _finalize_parser(_parse, source)


def parse_selfhost_script(source: str) -> dict:
    def _parse(parser: Parser) -> dict:
        statements = []
        parser.skip_statement_separators()
        while not parser.at_end():
            statements.append(parser.parse_statement())
            parser.skip_statement_separators()
        return {
            'kind': 'script',
            'statements': statements,
            'statement_count': len(statements),
            'token_count': len(parser.tokens),
            'ast_ready': True,
        }
    return _finalize_parser(_parse, source)


def analyze_selfhost_fixture_file(path: str) -> dict:
    fixture_path = Path(path).expanduser().resolve()
    if not fixture_path.exists():
        raise RuntimeError(f'Fant ikke fixture-fil: {fixture_path}')

    import json

    data = json.loads(fixture_path.read_text(encoding='utf-8'))
    out = {
        'fixture': str(fixture_path),
        'expressions': [],
        'scripts': [],
        'summary': {
            'expression_total': 0,
            'expression_ok': 0,
            'expression_failed': 0,
            'script_total': 0,
            'script_ok': 0,
            'script_failed': 0,
        },
    }

    for section, parser_fn in (('expressions', parse_selfhost_expression), ('scripts', parse_selfhost_script)):
        cases = data.get(section, [])
        out['summary'][f'{section[:-1]}_total'] = len(cases)
        for item in cases:
            name = str(item.get('name', 'unnamed'))
            source = str(item.get('source', ''))
            try:
                parsed = parser_fn(source)
                row = {
                    'name': name,
                    'ok': True,
                    'token_count': int(parsed.get('token_count', 0)),
                }
                if section == 'expressions':
                    row['root_node'] = parsed.get('ast', {}).get('node')
                    out['summary']['expression_ok'] += 1
                else:
                    row['statement_count'] = int(parsed.get('statement_count', 0))
                    row['statement_nodes'] = [stmt.get('node') for stmt in parsed.get('statements', [])]
                    out['summary']['script_ok'] += 1
                out[section].append(row)
            except Exception as exc:
                row = {'name': name, 'ok': False, 'error': str(exc)}
                out[section].append(row)
                if section == 'expressions':
                    out['summary']['expression_failed'] += 1
                else:
                    out['summary']['script_failed'] += 1

    out['summary']['ok'] = out['summary']['expression_failed'] == 0 and out['summary']['script_failed'] == 0
    return out

def parse_selfhost_program(source: str) -> dict:
    cleaned = _strip_comments(source)
    tokens = _normalize_tokens(_tokenize(cleaned))
    payload = Parser(tokens).parse_program()
    payload['normalized_tokens'] = [tok.value for tok in tokens]
    return payload


def parse_selfhost_file(path: str) -> dict:
    source_path = Path(path).expanduser().resolve()
    if not source_path.exists():
        raise RuntimeError(f'Fant ikke kildefil: {source_path}')
    payload = parse_selfhost_program(source_path.read_text(encoding='utf-8'))
    payload['source'] = str(source_path)
    return payload


def render_selfhost_summary(payload: dict) -> str:
    if payload.get('kind') == 'expression':
        lines = ['SELFHOST_EXPR_AST_V15']
        lines.append(f"root={payload.get('ast', {}).get('node', 'ukjent')}")
        lines.append(f"tokens={payload.get('token_count', 0)}")
        lines.append(f"normalized_tokens={len(payload.get('normalized_tokens', []))}")
        lines.append(f"ast_ready={'ja' if payload.get('ast_ready') else 'nei'}")
        return '\n'.join(lines)
    if payload.get('kind') == 'script':
        lines = ['SELFHOST_SCRIPT_AST_V15']
        for stmt in payload.get('statements', []):
            lines.append(f"  - {stmt.get('node', 'Stmt')}")
        lines.append(f"statements={payload.get('statement_count', 0)}")
        lines.append(f"tokens={payload.get('token_count', 0)}")
        lines.append(f"normalized_tokens={len(payload.get('normalized_tokens', []))}")
        lines.append(f"ast_ready={'ja' if payload.get('ast_ready') else 'nei'}")
        return '\n'.join(lines)
    if 'summary' in payload and 'expressions' in payload and 'scripts' in payload:
        summary = payload.get('summary', {})
        lines = ['SELFHOST_FIXTURE_AST_V15']
        lines.append(f"fixture={payload.get('fixture')}")
        lines.append(
            f"expressions={summary.get('expression_ok', 0)}/{summary.get('expression_total', 0)} ok"
        )
        lines.append(
            f"scripts={summary.get('script_ok', 0)}/{summary.get('script_total', 0)} ok"
        )
        if summary.get('expression_failed', 0):
            lines.append('expression_failures:')
            for item in payload.get('expressions', []):
                if not item.get('ok'):
                    lines.append(f"  - {item['name']}: {item['error']}")
        if summary.get('script_failed', 0):
            lines.append('script_failures:')
            for item in payload.get('scripts', []):
                if not item.get('ok'):
                    lines.append(f"  - {item['name']}: {item['error']}")
        lines.append(f"ok={'ja' if summary.get('ok') else 'nei'}")
        return '\n'.join(lines)
    lines = ['SELFHOST_BODY_AST_V15']
    for item in payload.get('imports', []):
        alias = f" som {item['alias']}" if item.get('alias') else ''
        lines.append(f"import {item['module_name']}{alias}")
    for fn in payload.get('functions', []):
        lines.append(
            f"funksjon {fn['name']}({', '.join(fn['params'])}) -> {fn.get('return_type') or 'ukjent'} "
            f"[body_tokens={fn['body_token_count']} statements={fn['statement_count']}]"
        )
        for stmt in fn.get('body_ast', []):
            lines.append(f"  - {stmt.get('node', 'Stmt')}")
    lines.append(f"imports={len(payload.get('imports', []))}")
    lines.append(f"funksjoner={len(payload.get('functions', []))}")
    lines.append(f"tokens={payload.get('token_count', 0)}")
    lines.append(f"normalized_tokens={len(payload.get('normalized_tokens', []))}")
    lines.append(f"ast_ready={'ja' if payload.get('ast_ready') else 'nei'}")
    return '\n'.join(lines)
