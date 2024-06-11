import os
from lib import windows
import subprocess
import sys
import argparse

try:
  import yaml
except ImportError:
  yaml = None

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
    elif (sys.platform.startswith('linux')):
      self.linux()
    else:
      print(f'Unsupported platform: {sys.platform}')
      sys.exit(1)

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

  def mac(self):
    """Installs dependencies on macOS."""
    self.run('brew bundle --file=Brewfile')

  def linux(self):
    """Installs dependencies on Linux."""
    
    if not yaml:
      print("The 'yaml' module is not installed. Please install it using 'pip install pyyaml'.")
      sys.exit(1)

    distro = self.linux_distro()
    if not distro:
      print("Unable to detect Linux distro")
      sys.exit(1)

    file = 'linux-packages.yml'
    with open(file, 'r') as f:
      data = yaml.safe_load(f)

    root = data.get('linux-packages', {})
    distro_data = root.get(distro, [])
    if not distro_data:
      print(f'Nothing found in {file} for: {distro}')
      sys.exit(1)

    command = distro_data['command']
    package_data = distro_data['packages']

    if not command:
      print(f'No package command found in {file} for: {distro}')
      sys.exit(1)

    if not package_data:
      print(f'No package list found in {file} for: {distro}')
      sys.exit(1)
    
    package_list = ' '.join(package_data)
    self.run(f'{command} {package_list}')
    
  def linux_distro(self):
    if os.path.isfile('/etc/os-release'):
        with open('/etc/os-release') as f:
            for line in f:
                if line.startswith('ID='):
                    return line.strip().split('=')[1].strip('"')
    return None

  def choco(self, config, ci_env):
    """Installs packages using Chocolatey."""
    
    args = ['choco', 'install', config]

    if ci_env:
      # don't show noisy choco progress bars in CI
      args.extend(['--no-progress'])

    # auto-accept all prompts
    args.extend(['-y'])

    self.run(args)

  def choco_ci(self):
    """Configures Chocolatey cache for CI."""

    runner_temp_key = 'RUNNER_TEMP'
    runner_temp = os.environ.get(runner_temp_key)
    if runner_temp:
      # sets the choco cache dir, which should match the dir in the ci cache action.
      key_arg = '--name="cacheLocation"'
      value_arg = f'--value="{runner_temp}/choco"'
      self.run(['choco', 'config', 'set', key_arg, value_arg])
    else:
      print(f'Warning: CI environment variable {runner_temp_key} not set')

  def run(self, args):
    """Runs a command."""
    print(f'Running: {args}')
    subprocess.run(args, shell=True, check=True)

main()
