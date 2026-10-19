"""Problem pages with a calculation tab ("계산") and an extra-information tab ("추가").

Directive lines at the end of a page (each line starts with '@'):
    @계산                      enables the calculation tab (variables come from the markers in the text)
    @식 w1=㉡-2㉢(㉡-㉠)        a result shown after the numbers are entered (label=expression), any number
    @추가 →4/3:1               the extra-information tab opens this page (F1)
    @값                        (on a solution page) the page is filled with the entered numbers, see below
Variable markers in the page text: a circled consonant (U+3260..U+326D, ㉠~㉭) directly followed by a number,
e.g. `㉠1.2`. The viewer shows only the number ("1.2"); the calculation tab asks for a new value for each
marker (in the order ㉠ ㉡ ...) and shows the problem again with the new numbers. In @식 expressions the bare
marker letter stands for the variable's value.
A solution page (the target of @추가, or any page with @값) may use the same markers (`㉠3`) and inline
calculations `{=㉡-2㉢(㉡-㉠)}`; the viewer shows them with the original values, and after the numbers are entered in
the calculation tab the solution pages (from the @추가 page to the end of that file) are shown filled with the
new numbers."""
import math
import re

MARKS = "".join(chr(c) for c in range(0x3260, 0x326E))
VAR_RE = re.compile("([" + MARKS + r"])(-?\d+(?:\.\d+)?)")
EXPR_RE = re.compile(r"\{=([^}]*)\}")
TOKEN_RE = re.compile(r"\{=([^}]*)\}|([" + MARKS + r"])(-?\d+(?:\.\d+)?)")
LINK_RE = re.compile(r"→(\d+)/(\d+)(?::(\d+))?")
FUNCS = ("sin", "cos", "tan", "ln", "log")


class Info:
    def __init__(self):
        self.calc = False
        self.fill = False        # markers / {=expr} are filled in (calculation tab or @값)
        self.exprs = []          # inline {=expr} strings
        self.src = ""            # page text with the markers (for the calculation tab)
        self.vars = []           # [(char, default string)], in marker order
        self.formulas = []       # [(label, expr)]
        self.extra = None        # (folder, file, page) or None
        self.errors = []


# Bare scripts: `x_j`, `p^14`, `e^㉠2`, `^(m-x)` are drawn as scripts like `x_{j}`, `p^{14}`, `e^{㉠2}`, `^{(m-x)}`
# (a lone T after ^ is the transpose: `w^Tx` -> `w^{T}x`). Formula lines ($), @ lines and {=expr} are left alone.
_RUN = ("[" + MARKS + r"](?:-?\d+(?:\.\d+)?)?|\([^()\s{}]{1,12}\)|-?[A-Za-z0-9]+(?:\.\d+)?")
SCRIPT_RE = re.compile(r"([\^_])(?!\{)(" + _RUN + ")")


def _wrap(m):
    run = m.group(2)
    if m.group(1) == "^" and len(run) > 1 and run[0] == "T" and run[1].islower():
        return "^{T}" + run[1:]
    return m.group(1) + "{" + run + "}"


def script_wrap(page):
    out = []
    for line in page.split("\n"):
        if "$" in line or line.startswith("@") or ("^" not in line and "_" not in line):
            out.append(line)
            continue
        parts = re.split(r"(\{=[^}]*\})", line)
        out.append("".join(p if p.startswith("{=") else SCRIPT_RE.sub(_wrap, p) for p in parts))
    return "\n".join(out)


def has_directives(page):
    return any(l.startswith("@") for l in page.split("\n"))


def _num(stem):
    m = re.match(r"^(\d+)[_.\- ]+", stem)
    return int(m.group(1)) if m else -1


def build_context(nd):
    """{(folder number, file number, 1-based page): {marker: value}} for the solution pages that follow a
    problem page's @추가 link, so that {=expr} on those pages can be evaluated with the problem's numbers."""
    from pathlib import Path
    nd = Path(nd)
    pages = {}
    for d in sorted(p for p in nd.iterdir() if p.is_dir()):
        for f in sorted(d.glob("*.txt")):
            txt = f.read_text(encoding="utf-8").replace("\r\n", "\n")
            pg = [p.strip("\n") for p in re.split(r"^---[ \t]*$", txt, flags=re.M)]
            pages[(_num(d.name), _num(f.stem))] = [p for p in pg if p.strip()]
    ctx = {}
    for pgs in pages.values():
        for p in pgs:
            if not has_directives(p):
                continue
            _, info = split_page(p)
            if info and info.calc and info.extra:
                fo, fi, pg0 = info.extra
                for k in range(pg0, len(pages.get((fo, fi), [])) + 1):
                    ctx.setdefault((fo, fi, k), dict(info.vars))
    return ctx


def split_page(page, ctx=None):
    """-> (display_text, Info or None). Pages without directives are returned unchanged.
    ctx: values of the variables from the problem page this solution page belongs to (see build_context)."""
    page = script_wrap(page)
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
        elif word == "값":
            info.fill = True
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
    for m in VAR_RE.finditer(EXPR_RE.sub("", src)):
        c, v = m.groups()
        if c in seen and seen[c] != v:
            info.errors.append(f"변수 {c} 의 값이 두 곳에서 다름: {seen[c]} / {v}")
        seen.setdefault(c, v)
    info.vars = sorted(seen.items())
    info.exprs = EXPR_RE.findall(src)
    info.fill = info.fill or info.calc
    values = dict(ctx or {})
    values.update(seen)
    if info.fill:
        def rep(m):
            if m.group(1) is not None:
                try:
                    return fmt(py_eval(m.group(1), values))
                except Exception as ex:
                    info.errors.append(f"{{={m.group(1)}}} 계산 실패 ({ex})")
                    return "?"
            return m.group(3)
        display = TOKEN_RE.sub(rep, src)
    else:
        display = src
        if info.vars or info.exprs:
            info.errors.append("원문자 변수나 {=식} 이 있는데 @계산 / @값 줄이 없음")
    for lab, expr in info.formulas:
        for c in expr:
            if c in MARKS and c not in seen:
                info.errors.append(f"@식 의 변수 {c} 가 본문에 값과 함께 나오지 않음: {lab}")
    return display, info


def fmt(x):
    """Like the calculator's Norm 1 display with 10 significant digits."""
    if x == 0:
        return "0"
    r = float(f"{x:.10g}")
    if abs(r) < 0.01 or abs(r) >= 1e10:
        m, e = f"{r:.9e}".split("e")
        m = m.rstrip("0").rstrip(".")
        return f"{m}E{'-' if int(e) < 0 else '+'}{abs(int(e)):02d}"
    t = f"{r:.10f}".rstrip("0").rstrip(".") if "e" not in f"{r}" else f"{r:.10g}"
    return t


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
