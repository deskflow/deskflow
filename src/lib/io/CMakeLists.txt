# SPDX-FileCopyrightText: 2024 - 2025 Chris Rizzitello <sithlord48@gmail.com>
# SPDX-FileCopyrightText: 2012 - 2024 Symless Ltd
# SPDX-FileCopyrightText: 2009 - 2012 Nick Bolton
# SPDX-License-Identifier: MIT

add_library(io STATIC
    Filesystem.cpp
    Filesystem.h
    IOException.cpp
    IOException.h
    IStream.h
    StreamBuffer.cpp
    StreamBuffer.h
    StreamFilter.cpp
    StreamFilter.h
)

target_link_libraries(io PUBLIC common)
