"""Add onSendToAi prop to CodeEditor JSX in App.tsx - handle \r\n endings"""
with open('src/App.tsx', 'rb') as f:
    raw = f.read()
txt = raw.decode('utf-8', errors='replace')

# Try both \r\n and \n endings
old_rn = '                showProblems={true}\r\n              />'
new_rn = '                showProblems={true}\r\n                onSendToAi={(text) => { setShowAiPanel(true); setExternalAiPrompt(text); }}\r\n              />'

old_n = '                showProblems={true}\n              />'
new_n = '                showProblems={true}\n                onSendToAi={(text) => { setShowAiPanel(true); setExternalAiPrompt(text); }}\n              />'

if old_rn in txt:
    txt = txt.replace(old_rn, new_rn, 1)
    print('Applied with \\r\\n line endings')
elif old_n in txt:
    txt = txt.replace(old_n, new_n, 1)
    print('Applied with \\n line endings')
else:
    print('ERROR: target not found exactly, trying partial match...')
    idx = txt.find('showProblems={true}')
    if idx >= 0:
        # Find the end of this line
        end = txt.find('>', idx)
        snippet = txt[idx:end+1]
        print(f'Snippet: {repr(snippet)}')

with open('src/App.tsx', 'wb') as f:
    f.write(txt.encode('utf-8'))

# Verify
with open('src/App.tsx', encoding='utf-8', errors='replace') as f:
    content = f.read()
if 'onSendToAi' in content:
    print('VERIFIED: onSendToAi is in App.tsx')
else:
    print('FAILED: onSendToAi still not found')
