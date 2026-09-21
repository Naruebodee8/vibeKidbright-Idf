"""
Fix all remaining corrupted Thai text in App.tsx:
1. THEMES array desc strings (lines 257-264)
2. Theme modal JSX text (lines 1948+)
3. Corrupted comment on line 257

Strategy: Use Python to find corrupted text patterns (U+FFFD replacement chars, € signs, and
mixed Thai-looking text) and replace entire problematic string values with clean English.
"""
import re

with open('src/App.tsx', 'rb') as f:
    raw = f.read()
txt = raw.decode('utf-8', errors='replace')

changes = 0

# ── 1. Fix THEMES array desc strings ─────────────────────────────────────────
theme_replacements = [
    # Navy Dark
    (r'desc:\s*"[^"]*(?:\ufffd|€|ส|ี|ด|ำ|ก|ร|ม|ท|่|า|ค|ั|เ|ิ|ร|ต|้|น)[^"]*"(?=,\s*color:\s*"#1a56)',
     'desc: "Dark blue-navy (default)"'),
    # Ocean Deep
    (r'desc:\s*"[^"]*(?:\ufffd|€|ส|น|้|า|ฟ|้|เ|ข|้|ม)[^"]*"(?=,\s*color:\s*"#0e7490)',
     'desc: "Deep ocean blue"'),
    # Midnight Purple
    (r'desc:\s*"[^"]*(?:\ufffd|€|ม|ว|ง|ม|ืด)[^"]*"(?=,\s*color:\s*"#6d28d9)',
     'desc: "Dark purple hue"'),
    # Emerald Night
    (r'desc:\s*"[^"]*(?:\ufffd|€|ส|เ|ีย|ว|ม|ร|ก)[^"]*"(?=,\s*color:\s*"#065f46)',
     'desc: "Emerald green"'),
    # Crimson
    (r'desc:\s*"[^"]*(?:\ufffd|€|ส|แ|ด|ง|เ|ข)[^"]*"(?=,\s*color:\s*"#be123c)',
     'desc: "Deep crimson red"'),
    # Light Clean
    (r'desc:\s*"[^"]*(?:\ufffd|€|ส|ว|า|ง|บ|า|ย)[^"]*"(?=,\s*color:\s*"#2563eb)',
     'desc: "Light & comfortable"'),
]

for pattern, replacement in theme_replacements:
    new_txt, n = re.subn(pattern, replacement, txt)
    if n > 0:
        txt = new_txt
        changes += n
        print(f'Fixed: {replacement[:40]}')

# ── 2. Simple string replacements for Thai in THEMES (fallback) ──────────────
# If the regex patterns above didn't match, try direct byte patterns
# Find desc: "..." for each theme by looking for the full context
themes_simple = [
    # Pattern: desc: "CORRUPT", color: "#XXXX"
    (r'desc:\s*"[^"]{0,80}"(?=,\s*color:\s*"#1a56)', 'desc: "Dark blue-navy (default)"'),
    (r'desc:\s*"[^"]{0,80}"(?=,\s*color:\s*"#0e7490)', 'desc: "Deep ocean blue"'),
    (r'desc:\s*"[^"]{0,80}"(?=,\s*color:\s*"#6d28d9)', 'desc: "Dark purple hue"'),
    (r'desc:\s*"[^"]{0,80}"(?=,\s*color:\s*"#065f46)', 'desc: "Emerald green"'),
    (r'desc:\s*"[^"]{0,80}"(?=,\s*color:\s*"#be123c)', 'desc: "Deep crimson red"'),
    (r'desc:\s*"[^"]{0,80}"(?=,\s*color:\s*"#2563eb)', 'desc: "Light & comfortable"'),
]
for pattern, replacement in themes_simple:
    new_txt, n = re.subn(pattern, replacement, txt)
    if n > 0:
        txt = new_txt
        changes += n
        print(f'Simple fix: {replacement}')

# ── 3. Fix corrupted comment on line ~257 ────────────────────────────────────
# "// CORRUPT Theme presets CORRUPT"
comment_pat = r'//[^\n]*Theme presets[^\n]*\n'
new_txt, n = re.subn(comment_pat, '// ── Theme presets ──────────────────────────────────────────────\n', txt)
if n > 0:
    txt = new_txt
    changes += n
    print('Fixed: theme presets comment')

# ── 4. Fix theme modal JSX text ───────────────────────────────────────────────
# Modal title "ปรับธีมสีแอป" → "App Color Theme"
modal_fixes = [
    # Title with emoji 🎨 then corrupt Thai
    (r'(?<=🎨</span>)\s*[^<]{2,30}(?=\s*</h3>)', ' App Color Theme'),
    (r'(?<=🎨\s)\s*[^<"]{3,50}(?=\s*</h3>|")', ' App Color Theme'),
    # Modal subtitle: "€ลือก..." → English  
    (r'<p[^>]*>[^<]*(?:€|ลือก|เลือก|ธีมสี|เปลี่ยน)[^<]{5,100}</p>',
     '<p className="text-xs text-[var(--text-muted)] mb-4">Select a theme. Changes apply immediately.</p>'),
    # HCI color guide heading
    (r'<p[^>]*>[^<]*(?:คำอธิบาย|ปุ่ม|หมัก|HCI)[^<]{2,60}</p>',
     '<p className="text-[10px] font-bold text-[var(--text-secondary)] uppercase tracking-wider mb-2">HCI Color Guide</p>'),
    # HCI color labels  
    (r'>(?:[^<€ส-ู]{0,3})?(?:€|ฟ)[^<"]{0,5}(?:Build|Save|Apply)<',
     '> Build, Save, Apply<'),
    (r'>(?:[^<€ส-ู]{0,3})?(?:€|ส)[^<"]{0,5}(?:Connect|Create)<',
     '> Connect, Create<'),
    (r'>(?:[^<€ส-ู]{0,3})?(?:€|น)[^<"]{0,5}(?:Delete|Disconnect)<',
     '> Delete, Disconnect<'),
    (r'>(?:[^<€ส-ู]{0,3})?(?:€|ส)[^<"]{0,5}(?:Warning|Flash)<',
     '> Warning, Flash<'),
    # Save button "บันทึกธีม" → "Apply Theme"
    (r'(?<=>)[^<]*(?:บัน|ทึก|ธีม|€)[^<]{1,20}(?=\s*</button>)',
     'Apply Theme'),
    # Modal close button "ปิด" or corrupted text  
    (r'(?<=<span>)[^<]*(?:??|€|\ufffd)[^<]*(?=</span>)',
     '✕'),
]

for pattern, replacement in modal_fixes:
    try:
        new_txt, n = re.subn(pattern, replacement, txt)
        if n > 0:
            txt = new_txt
            changes += n
            print(f'Modal fix applied: {repr(replacement)[:50]}')
    except re.error as e:
        print(f'Regex error ({e}): {pattern[:40]}')

# ── 5. Remove any remaining U+FFFD sequences in JSX text nodes ───────────────
# These appear as garbled chars in UI
# CAREFUL: only remove from string literals and JSX text, not in comments
fffd_count = txt.count('\ufffd')
if fffd_count:
    # Replace sequences of replacement chars (these are remnants of corrupted Thai)
    txt = re.sub(r'[\ufffd]{1,}', '', txt)
    print(f'Removed {fffd_count} U+FFFD replacement chars')
    changes += 1

with open('src/App.tsx', 'wb') as f:
    f.write(txt.encode('utf-8'))
print(f'\nTotal changes: {changes}')
print('App.tsx written')

# Quick verify
with open('src/App.tsx', encoding='utf-8', errors='replace') as f:
    content = f.read()
idx = content.find('desc: "')
print('\nVerify THEMES desc values:')
for m in re.finditer(r'desc:\s*"([^"]*)"', content):
    print(f'  {repr(m.group(1))[:60]}')
