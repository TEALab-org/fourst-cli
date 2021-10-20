#!/usr/bin/env python3

import re
from math import prod
import sys
import os
from subprocess import Popen, PIPE, STDOUT


# what identifiers are supported?
supportedDatatypes = ["int"]
supportedComparisons = ["<"]
supportedSteps = ["\+\+"]
supportedKernalAssignment = ["="]
nondelimitingOps = ["*"]
supportedSubscr = "[\d\s+]*"
nondelimChars = f"[\s\.\d{''.join(nondelimitingOps)}]"
delimitingOps = ["\+",";"]

# loop scope extraction re
loopExtract = re.compile('\s*for\s*\(\s*(?P<initializer>[^;]+)\s*;\s*(?P<condition>[^;]+)\s*;\s*(?P<step>[^)]+)\)\s*\s*{\s*(?P<content>[^}]+)')

# scope components re
initRe = re.compile(f'({"|".join(supportedDatatypes)})\s*(?P<var>\w+)(\s*=\s*(?P<num>\d+))?')
condRe = re.compile(f'(?P<var>\w+)\s*(?P<comp>{"|".join(supportedComparisons)})\s*(?P<num>\w+)')
stepRe = re.compile(f'(?P<var>\w+)(?P<step>{"|".join(supportedSteps)})')

# kernel components re
asgnRe = re.compile(f'(?P<arr>\w+)\s*(?P<subscripts>(\[.+?\])+)\s*(?P<assgn>{"|".join(supportedKernalAssignment)})\s*(?P<after>[\s\S]*?;)')
sbscRe = re.compile(f'\[(?P<pre>[^\]a-z]*)(?P<var>\w+)(?P<post>[^\]a-z]*)\]')

# arithmetic components re
negRe = re.compile(f'-\s*(?P<num>\d+)\s*')
posRe = re.compile(f'(^|\+)\s*(?P<num>\d+)\s*')

# string constants
prefix = """\n
Here is a super cool prefix woohoo!
"""

suffix = """\n
Here is a super cool suffix woot woot!
"""

# information needed from for loop scope
class ForScope:
    def __init__(self, initializer, condition, step):
        # get var name and lower bound from initializer
        res = initRe.search(initializer)
        self.var = res.group("var")
        self.lower = res.group("num")
        if self.lower == None: self.lower = 0
        
        # get upper bound from condition
        res = condRe.search(condition)
        if (res.group("var") != self.var):
            raise ValueError(f"Vars must match in loop definition -- got {self.var} in initializer and {res.group('var')} in condition")
        self.upper = res.group("num")

        # verify step var
        res = stepRe.search(step)
        if (res.group("var") != self.var):
            raise ValueError(f"Vars must match in loop definition -- got {self.var} in initializer and {res.group('var')} in step")
    
    def __str__(self):
        return f"{self.var}: ({self.lower}, {self.upper})"
    
    def __repr__(self):
        return str(self)

def parse_loop(text):
    # get for loop scopes
    scopes = []
    while True:
        res = loopExtract.search(text)
        
        if res == None: break
        
        text = res.group("content")
        scopes += [ForScope(res.group("initializer"), res.group("condition"), res.group("step"))]
    var = {s.var: s for s in scopes}
    varIdx = {v: i for i, v in enumerate([v for v in var])}

    # parse kernel from text variable
    res = asgnRe.search(text)
    arrName = res.group('arr')
    subscr = res.group('subscripts')

    # make sure subscripts only refer to scoped variables and have no offsets
    try:
        subscrMatch = sbscRe.findall(subscr)
        for r in subscrMatch: assert r[0] == r[2] == ''
        ref = tuple([m[1] for m in subscrMatch])
        for r in ref: assert r in var
    except Exception as e:
        print("Subscript dimensionality or dimension name does not match")
        raise e

    # parse rhs of assignment
    text = res.group('after')
    try:
        # separate coeffs
        targetRe = re.compile(f'(?=[^\s\+]+)(?P<pre>[\s\.\d\*\-]*)(?P<subscr>{arrName}(\[[^\]a-z]*\w+[^\]a-z]*\])+)(?P<post>[\s\.\d\*\-]*?)(?=\s*[\+;])')
        refs = list(targetRe.finditer(text))
        assert len(refs) == text.count(arrName) # make sure we got all of the refs!

        coeffs = {}
        for m in refs:
            # generate coeffs
            pre = prod([float(n) for n in m.group('pre').split('*') if not (n.isspace() or n == '')])
            sgroup = m.group('subscr')
            post = prod([float(n) for n in m.group('post').split('*') if not (n.isspace() or n == '')])
            mult = pre * post

            # find offsets
            offsetStr = sbscRe.findall(sgroup)
            offsets = {}
            assert len(offsetStr) == len(var)
            for s in offsetStr:
                offset = [c for c in [s[0]] + [s[2]] if not (c.isspace() or c == '')]
                idx = 0
                for o in offset:
                    idx += sum([int(m[-1]) for m in posRe.findall(o)])
                    idx -= sum([int(m) for m in negRe.findall(o)])
                offsets[s[1]] = idx

            # turn stencil coeff into standardized format
            idx = [None] * len(offsets)
            for v in offsets: idx[varIdx[v]] = offsets[v]
            idx = tuple(idx)

            # populate sparse array
            coeffs[idx] = mult
    except Exception as e:
        print("Error parsing kernel directly following assignment! Ensure only '*' is used for kernel definition and dimensions match.")
        print(text)
        raise e

    return (coeffs, scopes)

def gen_code(coeffs):
    return str(coeffs)

# find and replace for within pragmas in input string
def replace_pragma(s):
    target = re.compile('^#pragma\s+BEGIN_FOURST\s+(?P<content>[\w\W\s]*?)\s+^#pragma\s+END_FOURST', re.MULTILINE)

    # find and isolate inside of pragma
    res = target.search(s)
    if res == None: raise ValueError("Expected a code block deliniated by #pragma BEGIN_FOURST and #pragma END_FOURST")
    start, end = res.span()
    content = res.group('content')

    # parse the loop
    coeffs, scopes = parse_loop(content)

    # find dimensionality
    dims = [0] * len(scopes)
    for d in range(len(scopes)):
        dims[d] = (max([abs(c[d]) for c in coeffs]) * 2) + 1

    return s[:start] + gen_code(dims, coeffs).decode('utf-8') + s[end:]
    
# adds code prefix to a responsible location
def add_prefix(s, prefix=prefix):
    target = re.compile('[\w\W]*#include.*$', re.MULTILINE)

    # find last #include
    res = target.search(s)
    if res == None: end = 0
    else: start, end = res.span()
    
    # add prefix
    return s[:end] + prefix + s[end:]

# adds code suffix to the end
def add_suffix(s, suffix=suffix):
    return s + suffix

def flatten(dims, indexes):
    out = 0
    off = 1
    
    for dim, idx in list(zip(dims, indexes))[::-1]:
        out += off * (idx + (dim // 2))
        off *= dim
    return out


# generate code
def gen_code(dims: list, coeffs: dict):
    coeff_l = prod(dims) * [0.0]

    for c in coeffs:
        coeff_l[flatten(dims, c)] = coeffs[c]

    p = Popen(['./gencode'],stdout=PIPE,stdin=PIPE)
    p.stdin.write(
        f"{len(dims)} {' '.join([str(d) for d in dims])} {' '.join(str(c) for c in coeff_l)}".encode('ascii')
    )
    print(f"{len(dims)} {' '.join([str(d) for d in dims])} {' '.join(str(c) for c in coeff_l)}")
    s = p.communicate()[0]
    p.stdin.close()

    return s

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print(f"usage: {sys.argv[0]} src dest")
    
    with open(sys.argv[1], 'r') as f:
        content = f.read()

    s = replace_pragma(content)
    s = add_prefix(s)
    s = add_suffix(s)
    
    if os.path.exists(sys.argv[2]):
        print(f"Warning: {sys.argv[2]} already exists -- overwrite? (y/N)")
        if input() != 'y':
            print("aborting...")
            exit()
    
    print(f"Writing output to {sys.argv[2]}")
    with open(sys.argv[2], 'w') as f:
        f.write(s)