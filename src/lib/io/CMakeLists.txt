# SPDX-FileCopyrightText: 2024 - 2025 Chris Rizzitello <sithlord48@gmail.com>
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
