/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <array>
#include <iterator>
#include "synergy/ProtocolUtil.h"
#include "io/IStream.h"
#include "base/Log.h"
#include "common/stdvector.h"

#include <cctype>
#include <cstring>

//
// ProtocolUtil
//

namespace  {

void
writeInt(UInt32 Value, UInt32 Length, std::vector<UInt8>& Buffer)
{
    switch(Length)
    {
        case 1:
            Buffer.push_back(static_cast<UInt8>(Value & 0xffU));
            break;
        case 4:
            Buffer.push_back(static_cast<UInt8>((Value >> 24U) & 0xffU));
            Buffer.push_back(static_cast<UInt8>((Value >> 16U) & 0xffU));
            Buffer.push_back(static_cast<UInt8>((Value >>  8U) & 0xffU));
            Buffer.push_back(static_cast<UInt8>( Value         & 0xffU));
            break;
        case 2:
            Buffer.push_back(static_cast<UInt8>((Value >> 8U) & 0xffU));
            Buffer.push_back(static_cast<UInt8>( Value        & 0xffU));
            break;
        default:
             assert(0 && "invalid integer format length");
             return;
    }
}

template <typename T>
void
writeVectorInt(const std::vector<T>* VectorData, std::vector<UInt8>& Buffer)
{
    if (VectorData) {
        const std::vector<T>& Vector = *VectorData;
        writeInt((UInt32)Vector.size(), sizeof(UInt32), Buffer);
        for (size_t i = 0; i < Vector.size(); ++i) {
            writeInt(Vector[i], sizeof(T), Buffer);
        }
    }
}

void
writeString(const String* StringData, std::vector<UInt8>& Buffer)
{
    const UInt32 len = (StringData != NULL) ? (UInt32)StringData->size() : 0;
    writeInt(len, sizeof(len), Buffer);
    if (len != 0) {
        std::copy(StringData->begin(), StringData->end(), std::back_inserter(Buffer));
    }
}

} //namespace

void
ProtocolUtil::writef(synergy::IStream* stream, const char* fmt, ...)
{
    assert(stream != NULL);
    assert(fmt != NULL);
    LOG((CLOG_DEBUG2 "writef(%s)", fmt));

    va_list args;
    va_start(args, fmt);
    UInt32 size = getLength(fmt, args);
    va_end(args);
    va_start(args, fmt);
    vwritef(stream, fmt, size, args);
    va_end(args);
}

bool
ProtocolUtil::readf(synergy::IStream* stream, const char* fmt, ...)
{
    bool result = false;

    if (stream && fmt) {
        LOG((CLOG_DEBUG2 "readf(%s)", fmt));
        va_list args;
        va_start(args, fmt);
        try {
            vreadf(stream, fmt, args);
            result = true;
        }
        catch (XIO&) {
            result = false;
        }
        catch (const std::bad_alloc&) {
            result = false;
        }
        va_end(args);
    }

    return result;
}

void
ProtocolUtil::vwritef(synergy::IStream* stream,
                const char* fmt, UInt32 size, va_list args)
{
    assert(stream != NULL);
    assert(fmt != NULL);

    // done if nothing to write
    if (size == 0) {
        return;
    }

    // fill buffer
    std::vector<UInt8> Buffer;
    writef(Buffer, fmt, args);

    try {
        // write buffer
        stream->write(Buffer.data(), size);
        LOG((CLOG_DEBUG2 "wrote %d bytes", size));
    }
    catch (const XBase& exception) {
        LOG((CLOG_DEBUG2 "Exception <%s> during wrote %d bytes into stream", exception.what(), size));
        throw;
    }
}

void
ProtocolUtil::vreadf(synergy::IStream* stream, const char* fmt, va_list args)
{
    assert(stream != NULL);
    assert(fmt != NULL);

    // begin scanning
    while (*fmt) {
        if (*fmt == '%') {
            // format specifier.  determine argument size.
            ++fmt;
            UInt32 len = eatLength(&fmt);
            switch (*fmt) {
            case 'i': {
                void* destination = va_arg(args, void*);
                switch (len) {
                    case 1:
                        // 1 byte integer
                        *static_cast<UInt8*>(destination) = read1ByteInt(stream);
                        break;
                    case 2:
                        // 2 byte integer
                        *static_cast<UInt16*>(destination) = read2BytesInt(stream);
                        break;
                    case 4:
                        // 4 byte integer
                        *static_cast<UInt32*>(destination) = read4BytesInt(stream);
                        break;
                    default:
                        //the length is wrong
                        LOG((CLOG_ERR "read: length to be read is wrong: '%d' should be 1,2, or 4", len));
                        assert(false); //assert for debugging
                        break;
                }
                break;
            }

            case 'I': {
                void* destination = va_arg(args, void*);
                switch (len) {
                    case 1:
                        // 1 byte integer
                        readVector1ByteInt(stream, *static_cast<std::vector<UInt8>*>(destination));
                        break;
                    case 2:
                        // 2 byte integer
                        readVector2BytesInt(stream, *static_cast<std::vector<UInt16>*>(destination));
                        break;
                    case 4:
                        // 4 byte integer
                        readVector4BytesInt(stream, *static_cast<std::vector<UInt32>*>(destination));
                        break;
                    default:
                        //the length is wrong
                        LOG((CLOG_ERR "read: length to be read is wrong: '%d' should be 1,2, or 4", len));
                        assert(false); //assert for debugging
                        break;
                }
                break;
            }

            case 's': {
                String* destination = va_arg(args, String*);
                readBytes(stream, len, destination);
                break;
            }

            case '%':
                assert(len == 0);
                break;

            default:
                assert(0 && "invalid format specifier");
            }

            // next format character
            ++fmt;
        }
        else {
            // read next character
            char buffer[1];
            read(stream, buffer, 1);

            // verify match
            if (buffer[0] != *fmt) {
                LOG((CLOG_DEBUG2 "readf: format mismatch: %c vs %c", *fmt, buffer[0]));
                throw XIOReadMismatch();
            }

            // next format character
            ++fmt;
        }
    }
}

UInt32
ProtocolUtil::getLength(const char* fmt, va_list args)
{
    UInt32 n = 0;
    while (*fmt) {
        if (*fmt == '%') {
            // format specifier.  determine argument size.
            ++fmt;
            UInt32 len = eatLength(&fmt);
            switch (*fmt) {
            case 'i':
                assert(len == 1 || len == 2 || len == 4);
                (void)va_arg(args, UInt32);
                break;

            case 'I':
                assert(len == 1 || len == 2 || len == 4);
                switch (len) {
                case 1:
                    len = (UInt32)(va_arg(args, std::vector<UInt8>*))->size() + 4;
                    break;

                case 2:
                    len = 2 * (UInt32)(va_arg(args, std::vector<UInt16>*))->size() + 4;
                    break;

                case 4:
                    len = 4 * (UInt32)(va_arg(args, std::vector<UInt32>*))->size() + 4;
                    break;
                }
                break;

            case 's':
                assert(len == 0);
                len = (UInt32)(va_arg(args, String*))->size() + 4;
                (void)va_arg(args, UInt8*);
                break;

            case 'S':
                assert(len == 0);
                len = va_arg(args, UInt32) + 4;
                (void)va_arg(args, UInt8*);
                break;

            case '%':
                assert(len == 0);
                len = 1;
                break;

            default:
                assert(0 && "invalid format specifier");
            }

            // accumulate size
            n += len;
            ++fmt;
        }
        else {
            // regular character
            ++n;
            ++fmt;
        }
    }
    return n;
}



void
ProtocolUtil::writef(std::vector<UInt8>& buffer, const char* fmt, va_list args)
{
    while (*fmt) {
        if (*fmt == '%') {
            // format specifier.  determine argument size.
            ++fmt;
            UInt32 len = eatLength(&fmt);
            switch (*fmt) {
            case 'i': {
                const UInt32 v = va_arg(args, UInt32);
                writeInt(v, len, buffer);
                break;
            }

            case 'I': {
                switch (len) {
                case 1: {
                    // 1 byte integers
                    const std::vector<UInt8>* list = va_arg(args, const std::vector<UInt8>*);
                    writeVectorInt(list, buffer);
                    break;
                }

                case 2: {
                    // 2 byte integers
                    const std::vector<UInt16>* list = va_arg(args, const std::vector<UInt16>*);
                    writeVectorInt(list, buffer);
                    break;
                }

                case 4: {
                    // 4 byte integers
                    const std::vector<UInt32>* list = va_arg(args, const std::vector<UInt32>*);
                    writeVectorInt(list, buffer);
                    break;
                }

                default:
                    assert(0 && "invalid integer vector format length");
                    return;
                }
                break;
            }

            case 's': {
                assert(len == 0);
                const String* src = va_arg(args, String*);
                writeString(src, buffer);
                break;
            }

            case 'S': {
                assert(len == 0);
                const UInt32 len = va_arg(args, UInt32);
                const UInt8* src = va_arg(args, UInt8*);
                writeInt(len, sizeof(len), buffer);
                std::copy(src, src + len, std::back_inserter(buffer));
                break;
            }

            case '%':
                assert(len == 0);
                buffer.push_back('%');
                break;

            default:
                assert(0 && "invalid format specifier");
            }

            // next format character
            ++fmt;
        }
        else {
            // copy regular character
            buffer.push_back(*fmt++);
        }
    }
}

UInt32
ProtocolUtil::eatLength(const char** pfmt)
{
    const char* fmt = *pfmt;
    UInt32 n = 0;
    for (;;) {
        UInt32 d;
        switch (*fmt) {
        case '0': d = 0; break;
        case '1': d = 1; break;
        case '2': d = 2; break;
        case '3': d = 3; break;
        case '4': d = 4; break;
        case '5': d = 5; break;
        case '6': d = 6; break;
        case '7': d = 7; break;
        case '8': d = 8; break;
        case '9': d = 9; break;
        default: *pfmt = fmt; return n;
        }
        n = 10 * n + d;
        ++fmt;
    }
}

void
ProtocolUtil::read(synergy::IStream* stream, void* vbuffer, UInt32 count)
{
    assert(stream != NULL);
    assert(vbuffer != NULL);

    UInt8* buffer = static_cast<UInt8*>(vbuffer);
    while (count > 0) {
        // read more
        UInt32 n = stream->read(buffer, count);

        // bail if stream has hungup
        if (n == 0) {
            LOG((CLOG_DEBUG2 "unexpected disconnect in readf(), %d bytes left", count));
            throw XIOEndOfStream();
        }

        // prepare for next read
        buffer += n;
        count  -= n;
    }
}

UInt8 ProtocolUtil::read1ByteInt(synergy::IStream * stream)
{
    const UInt32 BufferSize = 1;
    std::array<UInt8, 1> buffer = {};
    read(stream, buffer.data(), BufferSize);

    UInt8 Result = buffer[0];
    LOG((CLOG_DEBUG2 "readf: read 1 byte integer: %d (0x%x)", Result, Result));

    return Result;
}

UInt16 ProtocolUtil::read2BytesInt(synergy::IStream * stream)
{
    const UInt32 BufferSize = 2;
    std::array<UInt8, BufferSize> buffer = {};
    read(stream, buffer.data(), BufferSize);

    UInt16 Result = static_cast<UInt16>((static_cast<UInt16>(buffer[0]) << 8) | static_cast<UInt16>(buffer[1]));
    LOG((CLOG_DEBUG2 "readf: read 2 byte integer: %d (0x%x)", Result, Result));

    return Result;
}

UInt32 ProtocolUtil::read4BytesInt(synergy::IStream * stream)
{
    const int BufferSize = 4;
    std::array<UInt8, BufferSize> buffer = {};
    read(stream, buffer.data(), BufferSize);

    UInt32 Result = (static_cast<UInt32>(buffer[0]) << 24) |
                    (static_cast<UInt32>(buffer[1]) << 16) |
                    (static_cast<UInt32>(buffer[2]) <<  8) |
                    (static_cast<UInt32>(buffer[3]));

    LOG((CLOG_DEBUG2 "readf: read 4 byte integer: %d (0x%x)", Result, Result));

    return Result;
}

void ProtocolUtil::readVector1ByteInt(synergy::IStream* stream, std::vector<UInt8>& destination)
{
    UInt32 size = read4BytesInt(stream);
    for (UInt32 i = 0; i < size; ++i) {
        destination.push_back(read1ByteInt(stream));
    }
}

void ProtocolUtil::readVector2BytesInt(synergy::IStream* stream, std::vector<UInt16>& destination)
{
    UInt32 size = read4BytesInt(stream);
    for (UInt32 i = 0; i < size; ++i) {
        destination.push_back(read2BytesInt(stream));
    }
}

void ProtocolUtil::readVector4BytesInt(synergy::IStream* stream, std::vector<UInt32>& destination)
{
    UInt32 size = read4BytesInt(stream);
    for (UInt32 i = 0; i < size; ++i) {
        destination.push_back(read4BytesInt(stream));
    }
}

void ProtocolUtil::readBytes(synergy::IStream * stream, UInt32 len, String* destination) {
    assert(len == 0);

    // read the string length
    UInt8 buffer[128];
    len = read4BytesInt(stream);
    // use a fixed size buffer if its big enough
    const bool useFixed = (len <= sizeof(buffer));

    // allocate a buffer to read the data
    UInt8* sBuffer = buffer;
    if (!useFixed) {
        try{
            sBuffer = new UInt8[len];
        }
        catch (std::bad_alloc & exception) {
            // Added try catch due to GHSA-chfm-333q-gfpp
            LOG((CLOG_ERR "ALLOC: Unable to allocate memory %d bytes", len));
            LOG((CLOG_DEBUG "bad_alloc detected: Do you have enough free memory?"));
            throw exception;
        }
    }

    // read the data
    try {
        read(stream, sBuffer, len);
    }
    catch (...) {
        if (!useFixed) {
            delete[] sBuffer;
        }
        throw;
    }

    LOG((CLOG_DEBUG2 "readf: read %d byte string", len));

    // save the data

    if (destination){
        destination->assign((const char*)sBuffer, len);
    }

    // release the buffer
    if (!useFixed) {
        delete[] sBuffer;
    }
}


//
// XIOReadMismatch
//

String
XIOReadMismatch::getWhat() const throw()
{
    return format("XIOReadMismatch", "ProtocolUtil::readf() mismatch");
}
