"""Fix corrupted handleFixWithAi prompt in App.tsx - replace the corrupted line with proper English text"""
with open('src/App.tsx', 'rb') as f:
    raw = f.read()

txt = raw.decode('utf-8', errors='replace')

# Find and replace the corrupt prompt line
# The function looks like:
#   const handleFixWithAi = () => {
#     if (!buildError) return;
#     setShowAiPanel(true);
#     const prompt = `CORRUPTED...`;
#     setExternalAiPrompt(prompt);
#   };

import re

# Replace the entire handleFixWithAi function with a clean version
old_pattern = r'const handleFixWithAi = \(\) => \{[^}]+\};'
new_func = """const handleFixWithAi = () => {
    if (!buildError) return;
    setShowAiPanel(true);
    const prompt = `Build failed with the following errors:\\n\\n\\`\\`\\`\\n${buildError.details}\\n\\`\\`\\`\\n\\nPlease analyze the errors and fix the code.`;
    setExternalAiPrompt(prompt);
  };"""

m = re.search(old_pattern, txt, re.DOTALL)
if m:
    print(f'Found handleFixWithAi at chars {m.start()}-{m.end()}')
    txt = txt[:m.start()] + new_func + txt[m.end():]
    print('Replaced successfully')
else:
    print('Pattern not found, trying line-by-line...')
    lines = txt.splitlines(keepends=True)
    for i, l in enumerate(lines):
        if 'const prompt = `' in l and i > 0 and 'handleFixWithAi' in ''.join(lines[max(0,i-5):i]):
            print(f'Found prompt line at {i+1}: {repr(l[:60])}')
            # Replace this line with correct prompt
            lines[i] = '    const prompt = `Build failed with the following errors:\\n\\n\\`\\`\\`\\n${buildError.details}\\n\\`\\`\\`\\n\\nPlease analyze the errors and fix the code.`;\n'
            txt = ''.join(lines)
            print('Line replaced')
            break

with open('src/App.tsx', 'wb') as f:
    f.write(txt.encode('utf-8'))
print('App.tsx written')

# Also verify onSendToAi was added to CodeEditor in JSX
with open('src/App.tsx', encoding='utf-8', errors='replace') as f:
    content = f.read()
if 'onSendToAi' in content:
    print('onSendToAi: FOUND in App.tsx')
else:
    print('onSendToAi: NOT FOUND - need to add manually')
