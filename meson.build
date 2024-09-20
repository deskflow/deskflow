# For now, we're only using Meson to resolve dependencies. CMake is called separately.
# In future, we may completely replace CMake with Meson.
# Where available, we use system packages, otherwise we use subprojects.
# Subprojects are also used to get the latest version during development.

project('deskflow', 'cpp')

if host_machine.system() == 'windows'
  subproject('wintoast')
endif

system_tomlplusplus = get_option('system-tomlplusplus')
if system_tomlplusplus
  dependency('tomlplusplus', required: false)
else
  subproject('tomlplusplus')
endif

system_cli11 = get_option('system-cli11')
if system_cli11
  dependency('cli11', required: false)
else
  subproject('cli11')
endif

system_gtest = get_option('system-gtest')
if system_gtest
  dependency('gtest', required: false)
else
  subproject('gtest')
endif

if host_machine.system() == 'linux'

  system_libei = get_option('system-libei')
  if system_libei
    dependency('libei-1.0', required: false)
  else
    # Using the subproject is only useful for development; it's not intended for normal use.
    # GNOME46 or above is required as this has the required bits for libei.
    # Building on anything older is pointless as you won't be able to actually connect to anything.
    subproject('libei', default_options: ['tests=disabled', 'liboeffis=disabled'])
  endif

  system_libportal = get_option('system-libportal')
  if system_libportal
    dependency('libportal', required: false)
  else
    # Using the subproject is only useful for development; it's not intended for normal use.
    subproject('libportal', default_options: [
      'docs=false',
      'backend-gtk3=enabled',
      'backend-qt6=disabled'
    ])
  endif
endif
