"""
Fix remaining encoding issues:
1. App.tsx:    translateErrorLocal Thai strings (raw CP874 bytes) -> English
2. App.tsx:    stripAnsi to catch ALL CSI sequences (not just m,G,K,H,F)
3. CodeEditor: Remaining Thai UI strings -> English
"""
import re, subprocess

UTF8_BOM = b'\xef\xbb\xbf'

# ─── App.tsx ───────────────────────────────────────────────────────────────────
with open('src/App.tsx', 'rb') as f:
    raw = f.read()
if raw.startswith(UTF8_BOM):
    raw = raw[3:]

# Decode with replacement chars so we can do text-level replacements
txt = raw.decode('utf-8', errors='replace')

# 1. Replace stripAnsi with comprehensive version that strips ALL ANSI/CSI sequences
old_strip = r"const stripAnsi = (s: string) => s.replace(/\\x1b\\[[0-9;]*[mGKHF]/g, \"\").replace(/\\x1b\\[\\??\\d*[hl]/g, \"\");"
new_strip = (
    "const stripAnsi = (s: string) => s\n"
    "      .replace(/\\x1b\\[[\\x30-\\x3f]*[\\x20-\\x2f]*[\\x40-\\x7e]/g, \"\")  // All CSI sequences\n"
    "      .replace(/\\x1b[^[]/g, \"\")                                      // ESC + single char\n"
    "      .replace(/[\\x00-\\x08\\x0b-\\x0c\\x0e-\\x1f\\x7f]/g, \"\");         // Other control chars"
)

if old_strip in txt:
    txt = txt.replace(old_strip, new_strip)
    print("stripAnsi: updated")
else:
    print("stripAnsi: pattern not found (may already be updated)")

# 2. Replace translateErrorLocal with English-only version
# Find the function boundaries
func_start = txt.find("const translateErrorLocal = (msg: string) => {")
if func_start < 0:
    print("translateErrorLocal: NOT FOUND")
else:
    # Find matching closing brace
    brace_depth = 0
    func_end = -1
    i = func_start
    found_open = False
    while i < len(txt):
        if txt[i] == '{':
            brace_depth += 1
            found_open = True
        elif txt[i] == '}':
            brace_depth -= 1
            if found_open and brace_depth == 0:
                func_end = i + 1
                break
        i += 1
    
    if func_end > 0:
        new_func = """const translateErrorLocal = (msg: string): string => {
      const l = msg.toLowerCase();
      if (l.includes("expected ';'"))         return "Missing semicolon ';'";
      if (l.includes("expected '}'"))         return "Missing closing '}'";
      if (l.includes("expected '{'"))         return "Missing opening '{'";
      if (l.includes("expected ')'"))         return "Missing closing ')'";
      if (l.includes("expected '('"))         return "Missing opening '('";
      if (l.includes("undeclared"))            return "Undeclared variable or function";
      if (l.includes("undefined reference"))  return "Undefined reference";
      if (l.includes("unused variable"))      return "Unused variable";
      if (l.includes("implicit declaration")) return "Missing #include for this function";
      if (l.includes("incompatible type"))    return "Type mismatch";
      if (l.includes("no such file"))         return "Header file not found";
      if (l.includes("too many arguments"))   return "Too many arguments";
      if (l.includes("too few arguments"))    return "Too few arguments";
      if (l.includes("redefinition"))         return "Redefinition error";
      if (l.includes("control reaches end"))  return "Missing return statement";
      if (l.includes("format"))               return "Format string mismatch";
      return msg;
    }"""
        txt = txt[:func_start] + new_func + txt[func_end:]
        print("translateErrorLocal: replaced with English")
    else:
        print("translateErrorLocal: could not find closing brace")

# 3. Remove any remaining U+FFFD replacement chars from the file
# (these are invalid bytes that were in the corrupted Thai strings)
# Replace FFFD that appear in string literals with empty string
fffd_count = txt.count('\ufffd')
if fffd_count:
    # Replace runs of fffd + nearby non-alnum printable chars (corrupted Thai words)
    txt = re.sub(r'[\ufffd\x80-\x9f]+', '', txt)
    print(f"Removed {fffd_count} replacement chars")

# Write back App.tsx as UTF-8 no BOM
with open('src/App.tsx', 'wb') as f:
    f.write(txt.encode('utf-8'))
print("App.tsx: written OK")

# ─── CodeEditor.tsx ────────────────────────────────────────────────────────────
with open('src/CodeEditor.tsx', 'rb') as f:
    raw2 = f.read()
if raw2.startswith(UTF8_BOM):
    raw2 = raw2[3:]

txt2 = raw2.decode('utf-8', errors='replace')

# Fix remaining Thai UI strings -> English
replacements = [
    # "No problems in this file" empty state
    (
        '<span>\ufffd\ufffd\ufffd\ufffd error \ufffd\ufffd\ufffd\ufffd\ufffd\ufffd warning \ufffd\ufffd\ufffd\ufffd\ufffd\ufffd\ufffd\ufffd</span>',
        '<span>No errors or warnings in this file</span>'
    ),
    # "No problems" badge
    (
        '<span className="text-emerald-500 text-[10px]">\u2713 \ufffd\ufffd\ufffd\ufffd\ufffd\ufffd\ufffd\ufffd</span>',
        '<span className="text-emerald-500 text-[10px]">\u2713 No problems</span>'
    ),
]

for old, new in replacements:
    if old in txt2:
        txt2 = txt2.replace(old, new)
        print(f"CodeEditor: fixed '{old[:30]}...'")

# Also fix with regex for any remaining FFFD runs in JSX text nodes
txt2 = re.sub(
    r'<span([^>]*)>([^<]*[\ufffd][^<]*)</span>',
    lambda m: f'<span{m.group(1)}>No errors or warnings in this file</span>'
    if 'error' in m.group(0).lower() or 'emerald' in m.group(0)
    else f'<span{m.group(1)}>No problems</span>',
    txt2
)

# Fix the "✓ ไม่มีปัญหา" badge - this line may have garbled Thai
# (the ✓ is U+2713, followed by corrupted Thai)
txt2 = re.sub(
    r'>\u2713\s+[\ufffd\x80-\x9f]+<',
    '>\u2713 No problems<',
    txt2
)

# Remove any remaining fffd chars
txt2 = re.sub(r'[\ufffd\x80-\x9f]+', '', txt2)

with open('src/CodeEditor.tsx', 'wb') as f:
    f.write(txt2.encode('utf-8'))
print("CodeEditor.tsx: written OK")

# Verify TypeScript still valid
result = subprocess.run(
    ['npx', 'tsc', '--noEmit', '--skipLibCheck'],
    capture_output=True, text=True, cwd=r'd:\Kidbright_IdF\vibeKidbright'
)
if result.returncode == 0:
    print("TypeScript: OK (0 errors)")
else:
    print("TypeScript errors:")
    print(result.stdout[-2000:])
    print(result.stderr[-1000:])
