import os
import pathlib
import shutil
import subprocess
import sys

prompt_path = pathlib.Path(sys.argv[1])
output_path = pathlib.Path(sys.argv[2])
action = sys.argv[3] if len(sys.argv) > 3 else 'forklar-kode'
prompt = prompt_path.read_text(encoding='utf-8') if prompt_path.exists() else ''
custom = os.environ.get('NORSCODE_AI_PROVIDER_CMD', '').strip()

def write_result(text):
    output_path.write_text(text, encoding='utf-8')

if custom:
    env = os.environ.copy()
    env['PROMPT_FILE'] = str(prompt_path)
    env['OUTPUT_FILE'] = str(output_path)
    env['ACTION'] = action
    proc = subprocess.run(custom, shell=True, text=True, capture_output=True, env=env)
    text = (proc.stdout or '').strip()
    if proc.stderr:
        text = (text + '\n\n[stderr]\n' + proc.stderr.strip()).strip()
    write_result(text or f'Provider command exited with {proc.returncode}.')
    sys.exit(0 if proc.returncode == 0 else 1)

if shutil.which('codex'):
    proc = subprocess.run(['codex', 'exec', '--skip-git-repo-check', prompt], text=True, capture_output=True)
    text = (proc.stdout or '').strip()
    if proc.stderr:
        text = (text + '\n\n[stderr]\n' + proc.stderr.strip()).strip()
    write_result(text or 'Tom respons fra codex.')
    sys.exit(0 if proc.returncode == 0 else 1)

write_result('Ingen AI-provider tilgjengelig. Sett NORSCODE_AI_PROVIDER_CMD eller installer codex CLI.')
sys.exit(1)
