"""
Restore App.tsx from git and merge in our functional changes (diagnostics wiring).
Strategy:
1. Get original clean App.tsx from git using subprocess (binary)
2. Our new code additions (stable GCC effect, diagnostics state, etc.) are in App.tsx already
3. Just fix the encoding of the current corrupted App.tsx properly
"""
import subprocess, os

# Get the original file via git cat-file (binary, no encoding conversion)
result = subprocess.run(
    ['git', 'cat-file', 'blob', '185334f:src/App.tsx'],
    capture_output=True,  # binary capture
    cwd=r'd:\Kidbright_IdF\vibeKidbright'
)

if result.returncode != 0:
    print('git cat-file failed:', result.stderr)
    exit(1)

orig_data = result.stdout
print(f'Original size: {len(orig_data)} bytes')
print(f'BOM: {orig_data[:3].hex()}')

# Find sparkles in original
idx = orig_data.find(b'text-4xl')
if idx >= 0:
    print(f'Original sparkles hex: {orig_data[idx:idx+20].hex()}')

# Check Thai in original
thai_orig = sum(1 for i in range(len(orig_data)-2) if orig_data[i] == 0xE0 and orig_data[i+1] in (0xB8, 0xB9))
print(f'Thai seqs in original: {thai_orig}')

# Now read current (possibly corrupted) App.tsx
with open('src/App.tsx', 'rb') as f:
    curr_data = f.read()
print(f'Current size: {len(curr_data)} bytes')

# The original file has the correct encoding for emoji/Thai
# But our current file has extra code additions (stable GCC effect, diagnostics)
# We need to find characters that exist in current but not in original
# and fix only the encoding

# Check specific areas in current file
for label, pattern in [
    ('sparkles', b'text-4xl'),
    ('bullet', b'to save '),
    ('Navy theme desc', b'Navy Dark'),
    ('Light theme desc', b'Light (Clean)'),
]:
    idx = curr_data.find(pattern)
    if idx >= 0:
        chunk = curr_data[idx:idx+40]
        thai = sum(1 for i in range(len(chunk)-2) if chunk[i] == 0xE0 and chunk[i+1] in (0xB8, 0xB9))
        print(f'{label}: {thai} Thai seqs | hex: {chunk.hex()[:50]}')
