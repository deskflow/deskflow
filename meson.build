# For now, we're only using Meson to resolve dependencies. CMake is called separately.
# In future, we may completely replace CMake with Meson.

project('synergy', 'cpp')

gtest = dependency('gtest', required: false)
if not gtest.found()
  subproject('gtest')
endif

if host_machine.system() == 'windows'
  wintoast = dependency('wintoast', required: false)
  if not wintoast.found()
    subproject('wintoast')
  endif
endif

if host_machine.system() == 'linux'

  system_libei = get_option('system_libei')
  if system_libei
    libei = dependency('libei-1.0', required: false)
  endif

  if not system_libei or not libei.found()
    subproject('libei', default_options: ['tests=disabled', 'liboeffis=disabled'])
  endif

  system_libportal = get_option('system_libportal')
  if system_libportal
    libportal = dependency('libportal', required: false)
  endif
  
  if not system_libportal or not libportal.found()

    # Building libportal is a bit tricky on RHEL-like, so skip until we figure it out.
    rhel_grep = run_command(
        'grep', '-Ei', 'redhat|rocky|almalinux', '/etc/os-release', check: false)
    rhel_like = rhel_grep.returncode() == 0

    if not rhel_like
      subproject('libportal', default_options: ['docs=false', 'backend-gtk3=enabled'])
    endif
  endif
endif
