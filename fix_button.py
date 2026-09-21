"""Fix the broken button line 2022 in App.tsx"""
with open('src/App.tsx', 'rb') as f:
    raw = f.read()
txt = raw.decode('utf-8', errors='replace')
lines = txt.splitlines(keepends=True)

# Line 2022 (0-indexed: 2021) is:
# '              onClick={() =>Apply Theme</button>\n'
# Should be:
# '              onClick={() => setShowThemeModal(false)}\n'
# '              className="w-full mt-4 py-2.5 bg-[var(--accent-primary)] text-white rounded-lg text-sm font-semibold hover:opacity-90 transition-opacity"\n'
# '            >\n'
# '              Apply Theme\n'
# '            </button>\n'

idx = 2021  # 0-indexed line 2022
target = lines[idx]
print(f'Current line 2022: {repr(target[:80])}')

if 'onClick={() =>' in target and 'Apply Theme</button>' in target:
    # Replace this single broken line with the proper button lines
    new_lines_insert = [
        '              onClick={() => setShowThemeModal(false)}\n',
        '              className="w-full mt-4 py-2.5 bg-[var(--accent-primary)] text-white rounded-lg text-sm font-semibold hover:opacity-90 transition-opacity"\n',
        '            >\n',
        '              Apply Theme\n',
        '            </button>\n',
    ]
    lines = lines[:idx] + new_lines_insert + lines[idx+1:]
    print(f'Replaced line 2022 with {len(new_lines_insert)} lines')
else:
    print(f'Target pattern not found in: {repr(target[:80])}')

new_txt = ''.join(lines)
with open('src/App.tsx', 'wb') as f:
    f.write(new_txt.encode('utf-8'))
print('App.tsx written')
print(f'New line count: {len(lines)}')
