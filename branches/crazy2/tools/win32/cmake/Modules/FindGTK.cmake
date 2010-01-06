# - try to find GTK (and glib) and GTKGLArea
#  GTK_INCLUDE_DIR   - Directories to include to use GTK
#  GTK_LIBRARIES     - Files to link against to use GTK
#  GTK_FOUND         - GTK was found
#  GTK_GL_FOUND      - GTK's GL features were found

#=============================================================================
# Copyright 2001-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distributed this file outside of CMake, substitute the full
#  License text for the above reference.)

# don't even bother under WIN32
IF(UNIX)

  FIND_PATH( GTK_gtk_INCLUDE_PATH NAMES gtk/gtk.h
    PATH_SUFFIXES gtk-1.2 gtk12
    PATHS
    /usr/openwin/share/include
    /usr/openwin/include
    /opt/gnome/include
  )

  # Some Linux distributions (e.g. Red Hat) have glibconfig.h
  # and glib.h in different directories, so we need to look
  # for both.
  #  - Atanas Georgiev <atanas@cs.columbia.edu>

  FIND_PATH( GTK_glibconfig_INCLUDE_PATH NAMES glibconfig.h
    PATHS
    /usr/openwin/share/include
    /usr/local/include/glib12
    /usr/lib/glib/include
    /usr/local/lib/glib/include
    /opt/gnome/include
    /opt/gnome/lib/glib/include
  )

  FIND_PATH( GTK_glib_INCLUDE_PATH NAMES glib.h
    PATH_SUFFIXES gtk-1.2 glib-1.2 glib12
    PATHS
    /usr/openwin/share/include
    /usr/lib/glib/include
    /opt/gnome/include
  )

  FIND_PATH( GTK_gtkgl_INCLUDE_PATH NAMES gtkgl/gtkglarea.h
    PATHS /usr/openwin/share/include
          /opt/gnome/include
  )

  FIND_LIBRARY( GTK_gtkgl_LIBRARY gtkgl
    /usr/openwin/lib
    /opt/gnome/lib
  )

  #
  # The 12 suffix is thanks to the FreeBSD ports collection
  #

  FIND_LIBRARY( GTK_gtk_LIBRARY
    NAMES  gtk gtk12
    PATHS /usr/openwin/lib
          /opt/gnome/lib
  )

  FIND_LIBRARY( GTK_gdk_LIBRARY
    NAMES  gdk gdk12
    PATHS  /usr/openwin/lib
           /opt/gnome/lib
  )

  FIND_LIBRARY( GTK_gmodule_LIBRARY
    NAMES  gmodule gmodule12
    PATHS  /usr/openwin/lib
           /opt/gnome/lib
  )

  FIND_LIBRARY( GTK_glib_LIBRARY
    NAMES  glib glib12
    PATHS  /usr/openwin/lib
           /opt/gnome/lib
  )

  FIND_LIBRARY( GTK_Xi_LIBRARY 
    NAMES Xi 
    PATHS /usr/openwin/lib 
          /opt/gnome/lib 
    ) 

  FIND_LIBRARY( GTK_gthread_LIBRARY
    NAMES  gthread gthread12
    PATHS  /usr/openwin/lib
           /opt/gnome/lib
  )

  IF(GTK_gtk_INCLUDE_PATH
     AND GTK_glibconfig_INCLUDE_PATH
     AND GTK_glib_INCLUDE_PATH
     AND GTK_gtk_LIBRARY
     AND GTK_glib_LIBRARY)

    # Assume that if gtk and glib were found, the other
    # supporting libraries have also been found.

    SET( GTK_FOUND "YES" )
    SET( GTK_INCLUDE_DIR  ${GTK_gtk_INCLUDE_PATH}
                           ${GTK_glibconfig_INCLUDE_PATH}
                           ${GTK_glib_INCLUDE_PATH} )
    SET( GTK_LIBRARIES  ${GTK_gtk_LIBRARY}
                        ${GTK_gdk_LIBRARY}
                        ${GTK_glib_LIBRARY} )

    IF(GTK_gmodule_LIBRARY)
      SET(GTK_LIBRARIES ${GTK_LIBRARIES} ${GTK_gmodule_LIBRARY})
    ENDIF(GTK_gmodule_LIBRARY)
    IF(GTK_gthread_LIBRARY)
      SET(GTK_LIBRARIES ${GTK_LIBRARIES} ${GTK_gthread_LIBRARY})
    ENDIF(GTK_gthread_LIBRARY)
    IF(GTK_Xi_LIBRARY)
      SET(GTK_LIBRARIES ${GTK_LIBRARIES} ${GTK_Xi_LIBRARY})
    ENDIF(GTK_Xi_LIBRARY)

    IF(GTK_gtkgl_INCLUDE_PATH AND GTK_gtkgl_LIBRARY)
      SET( GTK_GL_FOUND "YES" )
      SET( GTK_INCLUDE_DIR  ${GTK_INCLUDE_DIR}
                            ${GTK_gtkgl_INCLUDE_PATH} )
      SET( GTK_LIBRARIES  ${GTK_gtkgl_LIBRARY} ${GTK_LIBRARIES} )
      MARK_AS_ADVANCED(
        GTK_gtkgl_LIBRARY
        GTK_gtkgl_INCLUDE_PATH
        )
    ENDIF(GTK_gtkgl_INCLUDE_PATH AND GTK_gtkgl_LIBRARY)

  ENDIF(GTK_gtk_INCLUDE_PATH
     AND GTK_glibconfig_INCLUDE_PATH
     AND GTK_glib_INCLUDE_PATH
     AND GTK_gtk_LIBRARY
     AND GTK_glib_LIBRARY)

  MARK_AS_ADVANCED(
    GTK_gdk_LIBRARY
    GTK_glib_INCLUDE_PATH
    GTK_glib_LIBRARY
    GTK_glibconfig_INCLUDE_PATH
    GTK_gmodule_LIBRARY
    GTK_gthread_LIBRARY
    GTK_Xi_LIBRARY
    GTK_gtk_INCLUDE_PATH
    GTK_gtk_LIBRARY
    GTK_gtkgl_INCLUDE_PATH
    GTK_gtkgl_LIBRARY
  )

ENDIF(UNIX)



