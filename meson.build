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
    dependency('libei-1.0', required: false)
  else
    # Using the subproject is only useful for development; it's not intended for normal use.
    # GNOME46 or above is required as this has the required bits for libei.
    # Building on anything older is pointless as you won't be able to actually connect to anything.
    subproject('libei', default_options: ['tests=disabled', 'liboeffis=disabled'])
  endif

  system_libportal = get_option('system_libportal')
  if system_libportal
    dependency('libportal', required: false)
  else
    # Using the subproject is only useful for development; it's not intended for normal use.
    subproject('libportal', default_options: ['docs=false', 'backend-gtk3=enabled'])
  endif
endif
