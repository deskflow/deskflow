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
  pugixml = dependency('pugixml', required: false)
  if not pugixml.found()
    subproject('pugixml')
  endif

  libei = dependency('libei-1.0', required: false)
  if not libei.found()
    subproject('libei', default_options: ['tests=disabled', 'liboeffis=disabled'])
  endif

  libportal = dependency('libportal', required: false)
  if not libportal.found()
    subproject('libportal', default_options: ['docs=false', 'backend-gtk3=enabled'])
  endif
endif
