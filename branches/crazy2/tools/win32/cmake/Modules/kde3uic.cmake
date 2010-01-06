
#=============================================================================
# Copyright 2006-2009 Kitware, Inc.
# Copyright 2006 Alexander Neundorf <neundorf@kde.org>
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

# used internally by KDE3Macros.cmake
# neundorf@kde.org


EXECUTE_PROCESS(COMMAND ${KDE_UIC_EXECUTABLE}
   -L ${KDE_UIC_PLUGIN_DIR} -nounload -tr tr2i18n
   -impl ${KDE_UIC_H_FILE}
   ${KDE_UIC_FILE}
   OUTPUT_VARIABLE _uic_CONTENTS
   ERROR_QUIET
  )

STRING(REGEX REPLACE "tr2i18n\\(\"\"\\)" "QString::null" _uic_CONTENTS "${_uic_CONTENTS}" )
STRING(REGEX REPLACE "tr2i18n\\(\"\", \"\"\\)" "QString::null" _uic_CONTENTS "${_uic_CONTENTS}" )

FILE(WRITE ${KDE_UIC_CPP_FILE} "#include <kdialog.h>\n#include <klocale.h>\n\n")
FILE(APPEND ${KDE_UIC_CPP_FILE} "${_uic_CONTENTS}")

