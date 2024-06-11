import os
from lib import windows
import subprocess
import sys
import argparse

def main():
  """Entry point for the script."""

  parser = argparse.ArgumentParser()
  parser.add_argument('--pause-on-exit', action='store_true')
  args = parser.parse_args()

  try:
    deps = Deps()
    deps.install()
  except Exception as e:
    print(f'Error: {e}')
  
  if (args.pause_on_exit):
    input('Press enter to continue...')

class Deps:

  def install(self):
    """Installs dependencies."""

    if (sys.platform == 'win32'):
      self.windows()
    elif (sys.platform == 'darwin'):
      self.mac()
    else:
      print(f'Unsupported platform: {sys.platform}')
  
  def mac(self):
    """Installs dependencies on macOS."""
    subprocess.run('brew bundle --file=Brewfile', shell=True, check=True)

  def windows(self):
    """Installs dependencies on Windows."""

    if not windows.is_admin():
      windows.relaunch_as_admin(__file__)
      sys.exit()

    ci_env = os.environ.get('CI')
    if ci_env:
      print('CI environment detected')
      self.choco_ci()
    
    self.choco('Chocolatey.config', ci_env)

  def choco(self, config, ci_env):
    """Installs packages using Chocolatey."""
    
    args = ['choco', 'install', config]

    if ci_env:
      # don't show noisy choco progress bars in CI
      args.extend(['--no-progress'])

    # auto-accept all prompts
    args.extend(['-y'])

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
