# SPDX-FileCopyrightText: 2024 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-FileCopyrightText: 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

add_library(io STATIC
    filesystem.cpp
    filesystem.h
    IStream.h
    StreamBuffer.cpp
    StreamBuffer.h
    StreamFilter.cpp
    StreamFilter.h
    XIO.cpp
    XIO.h
)

# Windows-specific files
if(WIN32)
    target_sources(io PRIVATE
        win32/encoding_utilities.cpp
        win32/encoding_utilities.h
    )
endif()
