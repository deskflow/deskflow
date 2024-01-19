import os
from lib import windows
import subprocess
import sys
import argparse

def main():
  """Entry point for the script."""

  parser = argparse.ArgumentParser()
  parser.add_argument('--pause-on-exit', action='store_true')
  parser.add_argument('--skip', nargs='*', default=[])
  args = parser.parse_args()

  try:
    deps = Deps(args.skip)
    deps.install()
  except Exception as e:
    print(f'Error: {e}')
  
  if (args.pause_on_exit):
    input('Press enter to continue...')

class Deps:

  def __init__(self, skip):
    self.skip = skip

  def install(self):
    """Installs dependencies."""

    if (sys.platform == 'win32'):
      self.windows()
    else:
      print(f'Unsupported platform: {sys.platform}')

  def windows(self):
    """Installs dependencies on Windows."""

    if not windows.is_admin():
      windows.relaunch_as_admin(__file__)
      sys.exit()

    ci_env = os.environ.get('CI')
    if ci_env:
      print('CI environment detected')
      self.choco_ci()

      # already installed on github runners.
      self.skip.extend(['cmake', 'ninja'])
    
    self.choco("cmake")
    self.choco("ninja")
    self.choco("openssl", "3.1.1")

  def choco(self, package, version=None):
    """Installs a package using Chocolatey."""

    if (package in self.skip):
      print(f'Skipping: {package}')
      return
    
    args = ['choco', 'install', package]

    if (version):
      args.extend(['--version', version])

    args.extend(['-y', '--no-progress'])

    subprocess.run(args, shell=True, check=True)

  def choco_ci(self):
    """Configures Chocolatey cache for CI."""

    runner_temp_key = 'RUNNER_TEMP'
    runner_temp = os.environ.get(runner_temp_key)
    if runner_temp:
      # sets the choco cache dir, which should match the dir in the ci cache action.
      key_arg = '--name="cacheLocation"'
      value_arg = f'--value="{runner_temp}/choco"'
      subprocess.run(['choco', 'config', 'set', key_arg, value_arg], shell=True, check=True)
    else:
      print(f'Warning: CI environment variable {runner_temp_key} not set')

main()
