"""Simple fix: remove duplicate old function lines from App.tsx"""
with open('src/App.tsx', 'rb') as f:
    raw = f.read()

txt = raw.decode('utf-8', errors='replace')
lines = txt.splitlines(keepends=True)
print(f'Total lines: {len(lines)}')

# Find lines 1112-1128 (1-indexed) = indices 1111-1127 (0-indexed)
# Line 1112 starts with "    }'" -- the closing } of new func + old string remnant
# Lines 1113-1127 = old function body (corrupted)
# Line 1128 = "    };\n" = real closing of old function

# Strategy: find where new function ends and old content begins
new_fn_end_idx = -1   # index of the line with "    }'" (where we'll put "    };\n")
old_fn_end_idx = -1   # index of the "    };\n" that closes old function

for i in range(1105, 1140):
    if i >= len(lines):
        break
    line = lines[i]
    # The "transition" line: closing brace followed by old string content
    stripped = line.strip()
    # Should start with } and have something unexpected after it on the same line
    if stripped.startswith('}') and len(stripped) > 2 and stripped[1] in ("'", '"', 'i', 'r', 's'):
        new_fn_end_idx = i
        print(f'Transition line at {i+1}: {repr(line[:70])}')
        break

if new_fn_end_idx >= 0:
    # Find the next line that is just "    };" (closing of old function)
    for i in range(new_fn_end_idx + 1, new_fn_end_idx + 25):
        if i >= len(lines):
            break
        stripped = lines[i].strip()
        if stripped == '};' or stripped == '}':
            old_fn_end_idx = i
            print(f'Old func end at {i+1}: {repr(lines[i][:70])}')
            break

print(f'Replacing lines {new_fn_end_idx+1} to {old_fn_end_idx+1}')

if new_fn_end_idx >= 0 and old_fn_end_idx >= 0:
    new_lines = lines[:new_fn_end_idx] + ['    };\n'] + lines[old_fn_end_idx+1:]
    content = ''.join(new_lines)
    with open('src/App.tsx', 'wb') as f:
        f.write(content.encode('utf-8'))
    print(f'Done. New line count: {len(new_lines)}')
    # Show result around the area
    for i, l in enumerate(new_lines[1108:1120], start=1109):
        print(f'{i}: {repr(l[:80])}')
else:
    print('Could not find boundaries!')
    # Debug: show all lines in range
    for i in range(1108, 1135):
        if i < len(lines):
            print(f'{i+1}: {repr(lines[i][:80])}')
