import os
from lib import windows
import subprocess
import sys
import argparse
import traceback

config_file = 'deps.yml'

class EnvError(Exception):
  pass

class YamlError(Exception):
  pass

class PlatformError(Exception):
  pass

class PathError(Exception):
  pass

try:
  import yaml # type: ignore
except ImportError:
  # this is fairly common in earlier versions of python3,
  # which is normally what you find on mac and windows.
  print('Python yaml module missing, please install: pip install pyyaml')
  sys.exit(1)

def main():
  """Entry point for the script."""

  parser = argparse.ArgumentParser()
  parser.add_argument('--pause-on-exit', action='store_true', help='Useful on Windows')
  parser.add_argument('--only', type=str, help='Only install the specified dependency')
  args = parser.parse_args()

  try:
    deps = Dependencies(args.only)
    deps.install()
  except Exception as e:
    traceback.print_exc()
  
  if (args.pause_on_exit):
    input('Press enter to continue...')

def run(command):
  """Runs a shell command and asserts that the return code is 0."""
  if isinstance(command, list):
    print(f'Running: {" ".join(command)}')
  else:
    print(f'Running: {command}')
  
  try:
    subprocess.run(command, shell=True, check=True)
  except subprocess.CalledProcessError as e:
    print(f'Command failed: {command}', file=sys.stderr)
    raise e

def get_os():
  """Detects the operating system."""
  if (sys.platform == 'win32'):
    return 'windows'
  elif (sys.platform == 'darwin'):
    return 'mac'
  elif (sys.platform.startswith('linux')):
    return 'linux'
  else:
    raise PlatformError(f'Unsupported platform: {sys.platform}')

    
def get_linux_distro():
  """Detects the Linux distro."""
  os_file = '/etc/os-release'
  if os.path.isfile(os_file):
      with open(os_file) as f:
          for line in f:
              if line.startswith('ID='):
                  return line.strip().split('=')[1].strip('"')
  return None

class Config:
  def __init__(self):
    with open(config_file, 'r') as f:
      data = yaml.safe_load(f)

    os_name = get_os()
    root = data['dependencies']

    self.os = root[os_name]
    if not self.os:
      raise YamlError(f'Nothing found in {config_file} for: {os_name}')
      
  def get_qt_config(self):
    qt = self.os['qt']
    if not qt:
      raise YamlError(f'Nothing found in {config_file} for: qt')
    return qt
  
  def get_packages_file(self):
    packages = self.os['packages']
    if not packages:
      raise YamlError(f'Nothing found in {config_file} for: packages')
    return packages

  def get_linux_package_command(self, distro):
    distro_data = self.os[distro]
    if not distro_data:
      raise YamlError(f'Nothing found in {config_file} for: {distro}')

    command_base = distro_data['command']
    package_data = distro_data['packages']

    if not command_base:
      raise YamlError(f'No package command found in {config_file} for: {distro}')

    if not package_data:
      raise YamlError(f'No package list found in {config_file} for: {distro}')
    
    packages = ' '.join(package_data)
    return f'{command_base} {packages}'

class Dependencies:

  def __init__(self, only):
    self.config = Config()
    self.only = only

  def install(self):
    """Installs dependencies for the current platform."""

    os = get_os()
    if (os == 'windows'):
      self.windows()
    elif (os == 'mac'):
      self.mac()
    elif (os == 'linux'):
      self.linux()
    else:
      raise PlatformError(f'Unsupported platform: {os}')

  def windows(self):
    """Installs dependencies on Windows."""

    if not windows.is_admin():
      windows.relaunch_as_admin(__file__)
      sys.exit()
    
    ci_env = os.environ.get('CI')
    if ci_env:
      print('CI environment detected')
    
    only_qt = self.only == 'qt'

    # for ci, skip qt; we install Qt separately so we can cache it.
    if not ci_env or only_qt:
      qt = WindowsQt(self.config.get_qt_config())
      qt_install_dir = qt.get_install_dir()
      if qt_install_dir:
        print(f'Skipping Qt, already installed at: {qt_install_dir}')
      else:
        qt.install()

      if only_qt:
        return

    choco = WindowsChoco()

    if ci_env:
      choco.config_ci()
    
    choco.install('Chocolatey.config', ci_env)

  def mac(self):
    """Installs dependencies on macOS."""
    run('brew bundle --file=Brewfile')

  def linux(self):
    """Installs dependencies on Linux."""

    distro = get_linux_distro()
    if not distro:
      raise PlatformError("Unable to detect Linux distro")
    
    command = self.config.get_linux_package_command(distro)
    run(command)

class WindowsChoco:
  """Chocolatey for Windows."""

  def install(self, config, ci_env):
    """Installs packages using Chocolatey."""
    
    args = ['choco', 'install', config]

    if ci_env:
      # don't show noisy choco progress bars in CI
      args.extend(['--no-progress'])

    # auto-accept all prompts
    args.extend(['-y'])

    run(args)

  def config_ci(self):
    """Configures Chocolatey cache for CI."""

    runner_temp_key = 'RUNNER_TEMP'
    runner_temp = os.environ.get(runner_temp_key)
    if runner_temp:
      # sets the choco cache dir, which should match the dir in the ci cache action.
      key_arg = '--name="cacheLocation"'
      value_arg = f'--value="{runner_temp}/choco"'
      run(['choco', 'config', 'set', key_arg, value_arg])
    else:
      print(f'Warning: CI environment variable {runner_temp_key} not set')

class WindowsQt:
  """Qt for Windows."""
  
  def __init__(self, config):
    self.config = config

    self.version = os.environ.get('QT_VERSION')
    if not self.version:
      default_version = config['version']
      if not default_version:
        raise EnvError(f'Qt version not set in {config_file}')
      
      print(f'QT_VERSION not set, using: {default_version}')
      self.version = default_version

    self.base_dir = os.environ.get('QT_BASE_DIR')
    if not self.base_dir:
      default_base_dir = config['install-dir']
      if not default_base_dir:
        raise EnvError(f'Qt install-dir not set in {config_file}')

      print(f'QT_BASE_DIR not set, using: {default_base_dir}')
      self.base_dir = default_base_dir

    self.install_dir =  f'{self.base_dir}/{self.version}'

  def get_install_dir(self):
    if os.path.isdir(self.install_dir):
      return self.install_dir

  def install(self):
    """Installs Qt on Windows."""

    run(['pip', 'install', 'aqtinstall'])

    mirror_url = self.config['mirror']
    if not mirror_url:
      raise EnvError(f'Qt mirror not set in {config_file}')

    args = ['python', '-m', 'aqt', 'install-qt']
    args.extend(['--outputdir', self.base_dir])
    args.extend(['--base', mirror_url])
    args.extend(['windows', 'desktop', self.version, 'win64_msvc2019_64'])
    run(args)

    install_dir = self.get_install_dir()
    if not install_dir:
      raise EnvError(f'Qt not installed, path not found: {install_dir}')

main()
