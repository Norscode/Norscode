from dataclasses import dataclass


@dataclass
class Token:
    typ: str
    value: object = None
    line: int = 1
    column: int = 1


KEYWORDS = {
    "bruk": "BRUK",
    "som": "SOM",
    "funksjon": "FUNKSJON",
    "la": "LA",
    "skriv": "SKRIV",
    "hvis": "HVIS",
    "da": "DA",
    "ellers": "ELLERS",
    "mens": "MENS",
    "for": "FOR",
    "til": "TIL",
    "steg": "STEG",
    "i": "I",
    "bryt": "BRYT",
    "fortsett": "FORTSETT",
    "returner": "RETURNER",
    "heltall": "TYPE_INT",
    "bool": "TYPE_BOOL",
    "tekst": "TYPE_TEXT",
    "liste_heltall": "TYPE_LIST_INT",
    "liste_tekst": "TYPE_LIST_TEXT",
    "sann": "SANN",
    "usann": "USANN",
    "og": "OG",
    "eller": "ELLER",
    "ikke": "IKKE",
    "assert": "ASSERT",
    "assert_eq": "ASSERT_EQ",
    "assert_ne": "ASSERT_NE",
    "test": "TEST",
}


class Lexer:
    def __init__(self, text: str):
        self.text = text
        self.pos = 0
        self.current = text[0] if text else None
        self.line = 1
        self.column = 1

    def advance(self):
        if self.current == "\n":
            self.line += 1
            self.column = 1
        else:
            self.column += 1

        self.pos += 1
        self.current = self.text[self.pos] if self.pos < len(self.text) else None

    def skip_whitespace(self):
        while self.current is not None and self.current in " \t\r\n":
            self.advance()

    def skip_comment(self):
        while self.current is not None and self.current != "\n":
            self.advance()

    def read_number(self):
        result = ""
        while self.current is not None and self.current.isdigit():
            result += self.current
            self.advance()
        return int(result)

    def read_identifier(self):
        result = ""
        while self.current is not None and (self.current.isalnum() or self.current == "_"):
            result += self.current
            self.advance()
        return result

    def read_string(self):
        self.advance()
        result = ""
        while self.current is not None and self.current != '"':
            if self.current == "\\":
                self.advance()
                if self.current == "n":
                    result += "\n"
                elif self.current == "t":
                    result += "\t"
                elif self.current == '"':
                    result += '"'
                elif self.current == "\\":
                    result += "\\"
                else:
                    result += self.current or ""
                self.advance()
            else:
                result += self.current
                self.advance()
        if self.current != '"':
            raise SyntaxError(f'Tekststreng ble ikke avsluttet med " ved linje {self.line}, kolonne {self.column}')
        self.advance()
        return result

    def next_token(self):
        while self.current is not None:
            if self.current in " \t\r\n":
                self.skip_whitespace()
                continue

            if self.current == "#":
                self.skip_comment()
                continue

            start_line = self.line
            start_col = self.column

            if self.current.isdigit():
                return Token("NUMBER", self.read_number(), start_line, start_col)

            if self.current.isalpha() or self.current == "_":
                ident = self.read_identifier()
                if ident in KEYWORDS:
                    return Token(KEYWORDS[ident], ident, start_line, start_col)
                return Token("IDENT", ident, start_line, start_col)

            if self.current == '"':
                return Token("STRING", self.read_string(), start_line, start_col)

            if self.current == "=":
                self.advance()
                if self.current == "=":
                    self.advance()
                    return Token("EQ", "==", start_line, start_col)
                return Token("ASSIGN", "=", start_line, start_col)

            if self.current == "+":
                self.advance()
                if self.current == "=":
                    self.advance()
                    return Token("PLUS_ASSIGN", "+=", start_line, start_col)
                return Token("PLUS", "+", start_line, start_col)

            if self.current == "!":
                self.advance()
                if self.current == "=":
                    self.advance()
                    return Token("NE", "!=", start_line, start_col)
                raise SyntaxError(f"Ugyldig tegn: ! ved linje {start_line}, kolonne {start_col}")

            if self.current == ">":
                self.advance()
                if self.current == "=":
                    self.advance()
                    return Token("GTE", ">=", start_line, start_col)
                return Token("GT", ">", start_line, start_col)

            if self.current == "<":
                self.advance()
                if self.current == "=":
                    self.advance()
                    return Token("LTE", "<=", start_line, start_col)
                return Token("LT", "<", start_line, start_col)

            if self.current == "-":
                self.advance()
                if self.current == ">":
                    self.advance()
                    return Token("ARROW", "->", start_line, start_col)
                if self.current == "=":
                    self.advance()
                    return Token("MINUS_ASSIGN", "-=", start_line, start_col)
                return Token("MINUS", "-", start_line, start_col)

            if self.current == "*":
                self.advance()
                if self.current == "=":
                    self.advance()
                    return Token("STAR_ASSIGN", "*=", start_line, start_col)
                return Token("MUL", "*", start_line, start_col)

            if self.current == "/":
                self.advance()
                if self.current == "=":
                    self.advance()
                    return Token("SLASH_ASSIGN", "/=", start_line, start_col)
                return Token("DIV", "/", start_line, start_col)

            if self.current == "%":
                self.advance()
                if self.current == "=":
                    self.advance()
                    return Token("PERCENT_ASSIGN", "%=", start_line, start_col)
                return Token("PERCENT", "%", start_line, start_col)

            single = {
                ":": "COLON",
                ",": "COMMA",
                ".": "DOT",
                "(": "LPAREN",
                ")": "RPAREN",
                "{": "LBRACE",
                "}": "RBRACE",
                "[": "LBRACKET",
                "]": "RBRACKET",
            }

            if self.current in single:
                ch = self.current
                self.advance()
                return Token(single[ch], ch, start_line, start_col)

            raise SyntaxError(f"Ugyldig tegn: {self.current} ved linje {start_line}, kolonne {start_col}")

        return Token("EOF", None, self.line, self.column)
