import os
import subprocess
import ctypes
import sys
import argparse
import glob

BIN_FILE_BASE = 'synergyd'
SOURCE_BIN_DIR = os.path.join('build', 'bin')
SOURCE_BIN_GLOB = f'{BIN_FILE_BASE}*'
TARGET_BIN_DIR = 'bin'
TARGET_BIN_FILE = f'{os.path.join(TARGET_BIN_DIR, BIN_FILE_BASE)}.exe'
SERVICE_NOT_RUNNING_ERROR = 2

def main():
  """Entry point for the script."""

  parser = argparse.ArgumentParser()
  parser.add_argument('-p', action='store_true')
  args = parser.parse_args()

  try:
    reinstall()
  except Exception as e:
    print(f'Error: {e}')
  
  if (args.p):
    input('Press any key to continue...')

def reinstall():
  """Stops the running daemon service, copies files, and reinstalls."""

  if not is_admin():
    print('Re-launching script as admin')
    ctypes.windll.shell32.ShellExecuteW(None, 'runas', sys.executable, f'{__file__} -p', None, 1)
    sys.exit()
  
  print('Stopping daemon service')
  try:
    subprocess.run(['net', 'stop', 'synergy'], shell=True, check=True)
  except subprocess.CalledProcessError as e:
    if (e.returncode == SERVICE_NOT_RUNNING_ERROR):
      print('Daemon service not running')
    else:
      raise e

  print(f'Persisting dir: {TARGET_BIN_DIR}')
  os.makedirs(TARGET_BIN_DIR, exist_ok=True)

  source_files = glob.glob(os.path.join(SOURCE_BIN_DIR, SOURCE_BIN_GLOB))
  for source_file in source_files:
    target_file = os.path.join(TARGET_BIN_DIR, os.path.basename(source_file))
    print(f'Copying {source_file} to {target_file}')
    # use the copy command; shutil.copy gives us a permission denied error.
    try:
      subprocess.run(['copy', source_file, target_file], shell=True, check=True)
    except subprocess.CalledProcessError as e:
      print(f'Copy failed: {e}')
  
  print('Removing old daemon service')
  subprocess.run([TARGET_BIN_FILE, '/uninstall'], shell=True, check=True)

  print('Installing daemon service')
  subprocess.run([TARGET_BIN_FILE, '/install'], shell=True, check=True)

def is_admin():
    """Returns True if the current process has admin privileges."""
    try:
        return ctypes.windll.shell32.IsUserAnAdmin()
    except ctypes.WinError:
        return False

main()
