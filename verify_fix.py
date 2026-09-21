with open('src/App.tsx', 'rb') as f:
    data = f.read()

checks = [
    ('text-4xl (sparkles)', b'text-4xl'),
    ('Setup badge', b'isSettingUpEspIdf'),
    ('Vibe Coder', b'Vibe Coder'),
    ('Theme badge', b'Navy Dark'),
    ('bullet separator', b'Ctrl+S to save'),
]
for label, pattern in checks:
    idx = data.find(pattern)
    if idx < 0:
        print(f'{label}: pattern not found')
        continue
    chunk = data[idx:idx+80]
    # Count Thai-originated bytes
    thai = sum(1 for i in range(len(chunk)-2) if chunk[i] == 0xE0 and chunk[i+1] in (0xB8, 0xB9))
    status = 'CLEAN' if thai == 0 else f'CORRUPTED ({thai} Thai seqs)'
    # Show the relevant bytes
    print(f'{label}: {status}')
    print('  hex:', chunk.hex()[:60])
