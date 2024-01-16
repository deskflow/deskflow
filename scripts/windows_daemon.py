import os
import subprocess
import ctypes
import sys
import argparse
import glob

bin_file_base = 'synergyd'
source_bin_dir = os.path.join('build', 'bin')
source_bin_file_glob = bin_file_base + '*'
target_bin_dir = 'bin'
target_bin_file = os.path.join(target_bin_dir, bin_file_base) + '.exe'
service_not_running_error = 2

def main():  
  parser = argparse.ArgumentParser()
  parser.add_argument('-p', action='store_true')
  args = parser.parse_args()

  try:
    reinstall()
  except Exception as e:
    print('Error: ' + str(e))
  
  if (args.p):
    input('Press any key to continue...')

def reinstall():
  if not is_admin():
    print('Re-launching script as admin')
    ctypes.windll.shell32.ShellExecuteW(None, 'runas', sys.executable, __file__, None, 1)
    sys.exit()
  
  print('Stopping daemon service')
  try:
    subprocess.run(['net', 'stop', 'synergy'], shell=True, check=True)
  except subprocess.CalledProcessError as e:
    if (e.returncode == service_not_running_error):
      print('Daemon service not running')
    else:
      raise e

  print('Persisting dir: ' + target_bin_dir)
  os.makedirs(target_bin_dir, exist_ok=True)

  source_files = glob.glob(os.path.join(source_bin_dir, source_bin_file_glob))
  for source_file in source_files:
    target_file = os.path.join(target_bin_dir, os.path.basename(source_file))
    print('Copying ' + source_file + ' to ' + target_file)
    # use the copy command; shutil.copy gives us a permission denied error.
    subprocess.run(['copy', source_file, target_file], shell=True, check=True)
  
  print('Removing old daemon service')
  subprocess.run([target_bin_file, '/uninstall'], shell=True, check=True)

  print('Installing daemon service')
  subprocess.run([target_bin_file, '/install'], shell=True, check=True)

def is_admin():
    try:
        return ctypes.windll.shell32.IsUserAnAdmin()
    except ctypes.WinError:
        return False

main()
