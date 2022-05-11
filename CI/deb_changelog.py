from argparse import ArgumentParser
from datetime import datetime, timezone

IGNORE_SEGMENTS = ['Github Actions:']

if __name__ == '__main__':
  parser = ArgumentParser()
  parser.add_argument('name', type=str, help='Name of the package')
  parser.add_argument('version', type=str, help='Version of the package')
  parser.add_argument('-d', '--distribution', type=str, default='unstable', help='Distribution the package is built for')
  parser.add_argument('-u', '--urgency', type=str, default='low', help='Urgency of the package')
  args = parser.parse_args()
  build_time = datetime.now(timezone.utc).strftime('%a, %d %b %Y %H:%M:%S %z') # day-of-week, dd month yyyy hh:mm:ss +zzzz
  with open('ChangeLog', 'r') as in_file, open('debian/changelog', 'w+') as out_file:
    out_file.write(f'{args.name} ({args.version}) {args.distribution}; urgency={args.urgency}\n')
    in_section = False
    ignore_segment = False
    
    while line := in_file.readline():
      if line.startswith('v'):
        if line.startswith(f'v{args.version}'):
          in_section = True
        elif in_section:
          break
      elif not in_section:
        continue
      elif line.strip().replace('=', '') == '':
        ignore_segment = False
      elif line.strip() in IGNORE_SEGMENTS:
        ignore_segment = True
      elif line.strip().endswith(':'):
        out_file.write(f'\n  * {line.strip()[:-1]}')
      else:
        if in_section and not ignore_segment:
          while line.startswith(('#', '|', '-')):
            line = ' '.join(line.split()[1:])
          out_file.write(f'\n    - {line}')
    out_file.write(f'\n\n -- Symless <engineering@symless.com>  {build_time}')
