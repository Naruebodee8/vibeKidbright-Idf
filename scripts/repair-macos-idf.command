#!/bin/bash
# Repair launcher for the existing macOS release. Installs only native ESP-IDF;
# never flashes a board, deletes projects, or copies credentials to the SDK.
set -eu
if [[ "${1:-}" == "--help" ]]; then
  printf '%s\n' 'Usage: bash repair-macos-idf.command "/Applications/VibeKidbright IDE.app"' 'Close the IDE first. Requires Xcode Command Line Tools and matching Python 3.10+ (3.12 recommended).'
  exit 0
fi
if [[ "$(uname -s)" != Darwin ]]; then echo 'This repair launcher runs only on macOS.' >&2; exit 2; fi
APP="${1:-/Applications/VibeKidbright IDE.app}"
if [[ ! -f "$APP/Contents/Info.plist" ]]; then
  echo "Application not found: $APP. Pass the actual .app path as the first argument." >&2; exit 1
fi
APP_EXEC=$(/usr/libexec/PlistBuddy -c 'Print CFBundleExecutable' "$APP/Contents/Info.plist")
case "$APP_EXEC" in ''|*/*|*..*) echo 'Invalid application executable in Info.plist.' >&2; exit 1;; esac
if pgrep -x "$APP_EXEC" >/dev/null 2>&1; then echo 'Please close VibeKidbright, then run this repair launcher again.' >&2; exit 1; fi
ROOT="$HOME/.vibekidbright/mac-repair"
case "$ROOT" in *[[:space:]]*) echo 'ESP-IDF requires an SDK location without spaces in the full path.' >&2; exit 1;; esac
for directory in "$HOME/.vibekidbright" "$ROOT" "$ROOT/tools" "$ROOT/frameworks"; do
  if [[ -L "$directory" ]]; then echo "Refusing redirected SDK directory: $directory" >&2; exit 1; fi
done
/usr/bin/xcode-select -p >/dev/null 2>&1 || { echo 'Install Command Line Tools using: xcode-select --install'; exit 1; }
PY=''
for candidate in /opt/homebrew/opt/python@3.12/bin/python3.12 /usr/local/opt/python@3.12/bin/python3.12 /Library/Frameworks/Python.framework/Versions/3.12/bin/python3 /opt/homebrew/bin/python3 /usr/local/bin/python3 /usr/bin/python3; do
  if [[ -x "$candidate" ]] && "$candidate" -c 'import sys;sys.exit(not (sys.version_info >= (3,10)))' >/dev/null 2>&1; then PY="$candidate"; break; fi
done
if [[ -z "$PY" ]]; then echo 'Install native Python 3.12 from python.org or Homebrew (brew install python@3.12), then retry.' >&2; exit 1; fi
unset PYTHONHOME PYTHONPATH VIRTUAL_ENV IDF_PYTHON_ENV_PATH
export PATH="/opt/homebrew/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin:$PATH"
export IDF_TOOLS_PATH="$ROOT/tools"
IDF="$ROOT/frameworks/esp-idf-v5.5.4"
MINOR=$("$PY" -c 'import sys;print("%d.%d"%sys.version_info[:2])')
VENV="$IDF_TOOLS_PATH/python_env/idf5.5_py${MINOR}_env"
if [[ -L "$IDF" || -L "$IDF_TOOLS_PATH/python_env" || -L "$VENV" ]]; then echo 'Refusing a redirected native SDK/virtualenv directory.' >&2; exit 1; fi
if [[ -d "$ROOT" && ! -f "$ROOT/.owned-by-vibekidbright-repair" ]] && [[ -n "$(ls -A "$ROOT")" ]]; then
  echo "Existing unowned directory: $ROOT. Preserve it and choose a different repair location." >&2; exit 1
fi
mkdir -p "$ROOT/frameworks" "$IDF_TOOLS_PATH"
printf '%s\n' 'native-mac-repair-v1' > "$ROOT/.owned-by-vibekidbright-repair"
if [[ ! -f "$IDF/tools/idf_tools.py" ]]; then
  if [[ -e "$IDF" ]]; then echo "Incomplete framework at $IDF. Existing files were preserved." >&2; exit 1; fi
  /usr/bin/git clone --recursive --shallow-submodules --depth 1 --branch v5.5.4 https://github.com/espressif/esp-idf.git "$IDF"
fi
export IDF_PATH="$IDF"
export IDF_PYTHON_ENV_PATH="$VENV"
export PATH="$VENV/bin:$PATH"
sdk_ready() (
  [[ -x "$VENV/bin/python3" ]] || exit 1
  . "$IDF/export.sh" >/dev/null 2>&1 || exit 1
  idf.py --version >/dev/null 2>&1 && cmake --version >/dev/null 2>&1 && ninja --version >/dev/null 2>&1 && xtensa-esp32-elf-gcc --version >/dev/null 2>&1
)
if ! sdk_ready; then
  AVAILABLE_KB=$(df -Pk "$HOME" | awk 'NR==2 {print $4}')
  if [[ ! "$AVAILABLE_KB" =~ ^[0-9]+$ ]] || [[ "$AVAILABLE_KB" -lt 12582912 ]]; then
    echo 'Free at least 12 GiB on the home volume before installing the native SDK.' >&2; exit 1
  fi
  # Install at the final path: virtualenvs must never be moved afterwards.
  unset IDF_PYTHON_ENV_PATH
  "$PY" "$IDF/tools/idf_tools.py" install --targets=esp32
  "$PY" "$IDF/tools/idf_tools.py" install cmake ninja
  "$PY" "$IDF/tools/idf_tools.py" install-python-env --features=core --reinstall
  export IDF_PYTHON_ENV_PATH="$VENV"
fi
. "$IDF/export.sh"
idf.py --version
cmake --version
ninja --version
xtensa-esp32-elf-gcc --version
# Existing releases only enable Build after their original installer marker.
# Do not manufacture an unverified old Windows-toolchain readiness marker.
LEGACY="$HOME/Library/Application Support/th.nectec.vibekidbright/toolchain/.toolchain_ready"
if [[ ! -f "$LEGACY" ]]; then
  echo 'Native SDK is ready, but this legacy app has no installer marker. Use the rebuilt macOS app; the SDK is available for Terminal builds.' >&2
  exit 1
fi
# Legacy custom-path UI invokes cmd.exe. Keep only the previous SDK path values
# in a backup, retaining every other config value without logging any secrets.
"$VENV/bin/python3" - "$ROOT" <<'PY'
import json, os, pathlib, sys, tempfile
root=pathlib.Path(sys.argv[1]); config=pathlib.Path.home()/'.vibekidbright/config.json'
if config.exists():
    data=json.loads(config.read_text(encoding='utf-8'))
    if not isinstance(data,dict): raise SystemExit('Invalid config object; original preserved.')
    previous={key:data[key] for key in ('custom_idf_path','custom_tools_path') if key in data}
    if previous:
        backup=root/'previous-sdk-paths.json'
        if not backup.exists():
            fd=os.open(backup,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
            with os.fdopen(fd,'w',encoding='utf-8') as f: json.dump(previous,f,ensure_ascii=False,indent=2)
        for key in previous: data.pop(key)
        fd,name=tempfile.mkstemp(prefix='.sdk-paths-',dir=config.parent)
        try:
            with os.fdopen(fd,'w',encoding='utf-8') as f:
                json.dump(data,f,ensure_ascii=False,indent=2); f.flush();os.fsync(f.fileno())
            os.replace(name,config)
        finally:
            if os.path.exists(name):os.unlink(name)
PY
export VIBEKIDBRIGHT_IDF_PATH="$IDF"
export VIBEKIDBRIGHT_TOOLS_PATH="$IDF_TOOLS_PATH"
echo 'Native SDK verified. Starting the IDE with the repaired environment.'
echo 'Use this launcher for subsequent starts of the old app. Select the USB board port, not debug-console.'
exec "$APP/Contents/MacOS/$APP_EXEC"
