# For now, we're only using Meson to resolve dependencies. CMake is called separately.
# In future, we may completely replace CMake with Meson.

project('synergy', 'cpp')

gtest = dependency('gtest', required: false)
gmock = dependency('gmock', required: false)
pugixml = dependency('pugixml', required: false)
wintoast = dependency('wintoast', required: false)
libei = dependency('libei', required: false)
libportal = dependency('libportal', required: false)

if not gtest.found() or not gmock.found()
  subproject('gtest')
endif

if not pugixml.found()
  subproject('pugixml') 
endif

if not wintoast.found()
  subproject('wintoast') 
endif

if not libei.found()
  subproject('libei') 
endif

if not libportal.found()
  subproject('libportal') 
endif
