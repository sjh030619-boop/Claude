"""Problem pages with a calculation tab ("계산") and an extra-information tab ("추가").

Directive lines at the end of a page (each line starts with '@'):
    @계산                      enables the calculation tab (variables come from the markers in the text)
    @식 w1=㉡-2㉢(㉡-㉠)        a result shown after the numbers are entered (label=expression), any number
    @추가 →4/3:1               the extra-information tab opens this page (F1)
Variable markers in the page text: a circled consonant (U+3260..U+326D, ㉠~㉭) directly followed by a number,
e.g. `㉠1.2`. The viewer shows only the number ("1.2"); the calculation tab asks for a new value for each
marker (in the order ㉠ ㉡ ...) and shows the problem again with the new numbers. In @식 expressions the bare
marker letter stands for the variable's value."""
import math
import re

MARKS = "".join(chr(c) for c in range(0x3260, 0x326E))
VAR_RE = re.compile("([" + MARKS + r"])(-?\d+(?:\.\d+)?)")
LINK_RE = re.compile(r"→(\d+)/(\d+)(?::(\d+))?")
FUNCS = ("sin", "cos", "tan", "ln", "log")


class Info:
    def __init__(self):
        self.calc = False
        self.src = ""            # page text with the markers (for the calculation tab)
        self.vars = []           # [(char, default string)], in marker order
        self.formulas = []       # [(label, expr)]
        self.extra = None        # (folder, file, page) or None
        self.errors = []


def has_directives(page):
    return any(l.startswith("@") for l in page.split("\n"))


def split_page(page):
    """-> (display_text, Info or None). Pages without directives are returned unchanged."""
    lines = page.split("\n")
    if not any(l.startswith("@") for l in lines):
        return page, None
    info = Info()
    body = []
    for l in lines:
        if not l.startswith("@"):
            body.append(l)
            continue
        word, _, rest = l[1:].partition(" ")
        rest = rest.strip()
        if word == "계산":
            info.calc = True
        elif word == "식":
            lab, eq, expr = rest.partition("=")
            if not eq or not lab.strip() or not expr.strip():
                info.errors.append(f"@식 형식은 '@식 이름=식': {l}")
            else:
                info.calc = True
                info.formulas.append((lab.strip(), expr.strip()))
        elif word == "추가":
            m = LINK_RE.fullmatch(rest)
            if not m:
                info.errors.append(f"@추가 형식은 '@추가 →폴더/파일:쪽': {l}")
            else:
                info.extra = (int(m.group(1)), int(m.group(2)), int(m.group(3) or 1))
        else:
            info.errors.append(f"알 수 없는 지시줄: {l}")
    while body and not body[-1].strip():
        body.pop()
    src = "\n".join(body)
    info.src = src
    seen = {}
    for m in VAR_RE.finditer(src):
        c, v = m.groups()
        if c in seen and seen[c] != v:
            info.errors.append(f"변수 {c} 의 값이 두 곳에서 다름: {seen[c]} / {v}")
        seen.setdefault(c, v)
    info.vars = sorted(seen.items())
    if info.calc:
        display = VAR_RE.sub(lambda m: m.group(2), src)
    else:
        display = src
        if info.vars:
            info.errors.append("원문자 변수가 있는데 @계산 줄이 없음")
    for lab, expr in info.formulas:
        for c in expr:
            if c in MARKS and c not in seen:
                info.errors.append(f"@식 의 변수 {c} 가 본문에 값과 함께 나오지 않음: {lab}")
    return display, info


def py_eval(expr, values):
    """Evaluate an @식 expression the way the calculator would (for the checker's information only)."""
    s = expr
    for c in MARKS:
        if c in s:
            s = s.replace(c, "(" + values[c] + ")")
    s = s.replace("×", "*").replace("÷", "/").replace("−", "-").replace("²", "**2").replace("^", "**")
    s = s.replace("π", "(pi)").replace("√(", "sqrt(")
    s = re.sub(r"\be\*\*\(", "exp(", s)
    s = re.sub(r"(\)|\d)\s*(?=\(|sin|cos|tan|ln|log|sqrt|exp|pi)", r"\1*", s)
    s = re.sub(r"\)\s*(?=\d)", ")*", s)
    env = {"sin": math.sin, "cos": math.cos, "tan": math.tan, "ln": math.log, "log": math.log10,
           "sqrt": math.sqrt, "exp": math.exp, "pi": math.pi}
    if re.search(r"[^0-9a-z+\-*/().\s]", s):
        raise ValueError("허용되지 않는 문자")
    return eval(s, {"__builtins__": {}}, env)
