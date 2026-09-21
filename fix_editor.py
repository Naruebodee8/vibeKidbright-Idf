"""
Second-pass fix for App.tsx:
1. Fix remaining Thai sequences in Theme/desc strings
2. Fix Euro-sign (E2 82 AC) that was originally a UTF-8 continuation byte 0x80
   These appear as: E2 [E2 82 AC] A2 where the result is invalid UTF-8
"""

with open('src/App.tsx', 'rb') as f:
    data = bytearray(f.read())

# Remove BOM if somehow re-added
if data[:3] == bytearray([0xEF, 0xBB, 0xBF]):
    data = data[3:]

fixed = 0

# CP874 Windows to Unicode table for 0x80-0xFF
import codecs

def cp874_byte_to_utf8(b):
    """Convert a single byte interpreted as CP874 to UTF-8 bytes"""
    try:
        char = bytes([b]).decode('cp874')
        return char.encode('utf-8')
    except:
        return bytes([b])

# Build full reverse map: corrupted_utf8_sequence -> original_byte
reverse = {}
for orig in range(0x80, 0x100):
    corrupted = cp874_byte_to_utf8(orig)
    if corrupted not in reverse:
        reverse[corrupted] = orig

# First pass: fix Thai block (E0 B8/B9 xx) and C1 controls (C2 80-9F)
output = bytearray()
i = 0
while i < len(data):
    # Thai block UTF-8 (E0 B8 xx or E0 B9 xx)
    if i+2 < len(data) and data[i] == 0xE0 and data[i+1] in (0xB8, 0xB9):
        three = bytes(data[i:i+3])
        if three in reverse:
            output.append(reverse[three])
            fixed += 1
            i += 3
            continue
    # C1 control (C2 80-9F)
    if i+1 < len(data) and data[i] == 0xC2 and 0x80 <= data[i+1] <= 0x9F:
        two = bytes(data[i:i+2])
        if two in reverse:
            output.append(reverse[two])
            fixed += 1
            i += 2
            continue
    # Euro sign E2 82 AC = CP874 byte 0x80
    if i+2 < len(data) and data[i:i+3] == bytes([0xE2, 0x82, 0xAC]):
        output.append(reverse.get(bytes([0xE2, 0x82, 0xAC]), 0x80))
        fixed += 1
        i += 3
        continue
    output.append(data[i])
    i += 1

print(f'Pass 1: fixed {fixed} sequences')

# Validate UTF-8 and report any remaining issues
try:
    text = bytes(output).decode('utf-8')
    print('UTF-8 valid: YES')
except UnicodeDecodeError as e:
    print(f'UTF-8 error: {e}')

with open('src/App.tsx', 'wb') as f:
    f.write(bytes(output))
print('Written to App.tsx')

# Quick spot-checks
data2 = bytes(output)
for label, pattern, expected_hex in [
    ('sparkles ✨', b'text-4xl', 'e29ca8'),
    ('bullet •',   b'to save ', 'e28022'),
    ('gear ⚙',     b'isSettingUpEspIdf ? "bg', 'e29a99'),
]:
    idx = data2.find(pattern)
    if idx >= 0:
        chunk = data2[idx:idx+50].hex()
        found = expected_hex in chunk
        print(f'{label}: {"FOUND" if found else "MISSING"} | hex: {chunk[:40]}')
