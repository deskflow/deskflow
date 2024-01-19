import ctypes
import sys

def relaunch_as_admin(script):
  args = ' '.join(sys.argv[1:])
  command = f'{script} --pause-on-exit {args}'
  print(f'Re-launching script as admin: {command}')
  ctypes.windll.shell32.ShellExecuteW(None, 'runas', sys.executable, command, None, 1)

def is_admin():
    """Returns True if the current process has admin privileges."""
    try:
        return ctypes.windll.shell32.IsUserAnAdmin()
    except ctypes.WinError:
        return False
