/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2020 Symless Ltd.
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
#include "test/global/gtest.h"
#include "test/mock/io/MockStream.h"
#include "synergy/ProtocolUtil.h"

using ::testing::_;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::SetArgPointee;
using ::testing::Eq;
using ::testing::StrEq;
using ::testing::TypedEq;

ACTION_P2(SetValueToVoidPointerArg0, value, size)
{
    memcpy(arg0, value, size);
}

ACTION(ThrowBadAlloc)
{
    throw std::bad_alloc();
}

class ProtocolUtilTests : public ::testing::Test
{
public:
    MockStream stream;
    UInt8 ActualInt8 = 0;
    UInt16 ActualInt16 = 0;
    UInt32 ActualInt32 = 0;
    std::string ActualString;
};

TEST_F(ProtocolUtilTests, readf__XIOEndOfStream_exception)
{
    ON_CALL(stream, read(_, _)).WillByDefault(Return(0));

    EXPECT_FALSE(ProtocolUtil::readf(&stream, "%s", &ActualString));
    EXPECT_TRUE(ActualString.empty());
}

TEST_F(ProtocolUtilTests, readf_XIOReadMismatch_exception)
{
    EXPECT_CALL(stream, read(_, _))
        .WillOnce(DoAll(SetValueToVoidPointerArg0("b", 1), Return(1)));

    EXPECT_FALSE(ProtocolUtil::readf(&stream, "a%s", &ActualString));
    EXPECT_TRUE(ActualString.empty());
}

TEST_F(ProtocolUtilTests, readf_bad_alloc_exception)
{
    ON_CALL(stream, read(_, _)).WillByDefault(ThrowBadAlloc());

    EXPECT_FALSE(ProtocolUtil::readf(&stream, "a%s", &ActualString));
    EXPECT_TRUE(ActualString.empty());
}

TEST_F(ProtocolUtilTests, readf_asserts)
{
    ASSERT_DEBUG_DEATH(
        {ProtocolUtil::readf(&stream, "%x", &ActualString);},
        "invalid format specifier"
    );

    ASSERT_DEBUG_DEATH(
        {ProtocolUtil::readf(&stream, "%5i", &ActualString);},
        "length to be read is wrong:"
    );

    ASSERT_DEBUG_DEATH(
        {ProtocolUtil::readf(&stream, "%5I", &ActualString);},
        ""
    );
}

TEST_F(ProtocolUtilTests, readf_params_validation)
{
    EXPECT_FALSE(ProtocolUtil::readf(NULL, "%x", NULL));
    EXPECT_FALSE(ProtocolUtil::readf(&stream, NULL, NULL));
}


TEST_F(ProtocolUtilTests, readf_string)
{
    const UInt8 Length = 200;
    const std::string Expected(Length, 'x');
    std::array<UInt8, 4> StringSize = {0,0,0,Length};

    EXPECT_CALL(stream, read(_, _))
        .WillOnce(
            DoAll(
                SetValueToVoidPointerArg0(StringSize.data(), StringSize.size()),
                Return(StringSize.size())
            )
         )
        .WillOnce(
            DoAll(
                SetValueToVoidPointerArg0(Expected.c_str(), Expected.length()),
                Return(Expected.length())
            )
        );

    EXPECT_TRUE(ProtocolUtil::readf(&stream, "%s", &ActualString));
    EXPECT_EQ(Expected, ActualString);
}

class ReadfIntTestFixture : public ::testing::TestWithParam< std::tuple<const char*, int> >
{
public:
    MockStream stream;
    UInt8 StreamData1Byte = 10;
    std::array<UInt8, 2> StreamData2Bytes = {0, 10};
    std::array<UInt8, 4> StreamData4Bytes = {0, 0, 0, 10};

    UInt8* getStreamData(int size)
    {
        UInt8* StreamData = nullptr;
        switch(size){
            case 2:
                StreamData = StreamData2Bytes.data();
                break;
            case 4:
                StreamData = StreamData4Bytes.data();
                break;
            default:
                StreamData = &StreamData1Byte;
                break;
        }
        return StreamData;
    }
};

TEST_P(ReadfIntTestFixture, readf_int)
{
    int Actual = 0;
    const int Expected = 10;
    const char* Format = std::get<0>(GetParam());
    int StreamDataSize = std::get<1>(GetParam());
    UInt8* StreamData = getStreamData(StreamDataSize);

    ON_CALL(stream, read(_, _))
        .WillByDefault(
            DoAll(
                SetValueToVoidPointerArg0(StreamData, StreamDataSize),
                Return(StreamDataSize)
                )
        );

    EXPECT_TRUE(ProtocolUtil::readf(&stream, Format, &Actual));
    EXPECT_EQ(Expected, Actual);
}

INSTANTIATE_TEST_CASE_P(
        ReadfIntTests,
        ReadfIntTestFixture,
        ::testing::Values(
                std::make_tuple("%1i", 1),
                std::make_tuple("%2i", 2),
                std::make_tuple("%4i", 4)));

class ReadfIntVectorTestFixture : public ReadfIntTestFixture
{
};

TEST_P(ReadfIntVectorTestFixture, readf_int_vector)
{
    std::vector<UInt8> Actual1Byte = {};
    std::vector<UInt16> Actual2Bytes = {};
    std::vector<UInt32> Actual4Bytes = {};

    const std::vector<UInt8> Expected1Byte = {10,10};
    const std::vector<UInt16> Expected2Bytes = {10,10};
    const std::vector<UInt32> Expected4Bytes = {10,10};
    std::array<UInt8, 4> StreamVectorSize = {0,0,0,2};

    const char* Format = std::get<0>(GetParam());
    int StreamDataSize = std::get<1>(GetParam());
    UInt8* StreamData = getStreamData(StreamDataSize);

    MockStream stream;
    EXPECT_CALL(stream, read(_, _))
        .WillOnce(
            DoAll(
                SetValueToVoidPointerArg0(StreamVectorSize.data(), StreamVectorSize.size()),
                Return(StreamVectorSize.size())
                )
        )
        .WillRepeatedly(
            DoAll(
                SetValueToVoidPointerArg0(StreamData, StreamDataSize),
                Return(StreamDataSize)
            ));

    switch(StreamDataSize){
        case 2:
            EXPECT_TRUE(ProtocolUtil::readf(&stream, Format, &Actual2Bytes));
            EXPECT_EQ(Expected2Bytes, Actual2Bytes);
            break;
        case 4:
            EXPECT_TRUE(ProtocolUtil::readf(&stream, Format, &Actual4Bytes));
            EXPECT_EQ(Expected4Bytes, Actual4Bytes);
            break;
        default:
            EXPECT_TRUE(ProtocolUtil::readf(&stream, Format, &Actual1Byte));
            EXPECT_EQ(Expected1Byte, Actual1Byte);
            break;
    }
}

INSTANTIATE_TEST_CASE_P(
        ReadfIntVectorTests,
        ReadfIntVectorTestFixture,
        ::testing::Values(
                std::make_tuple("%1I", 1),
                std::make_tuple("%2I", 2),
                std::make_tuple("%4I", 4)));

class ReadfIntAndStringTest : public ReadfIntTestFixture
{
public:
    UInt8 ActualInt8 = 0;
    UInt16 ActualInt16 = 0;
    UInt32 ActualInt32 = 32;
    std::string ActualString;
};

TEST_P(ReadfIntAndStringTest, readf_int_and_string)
{
    const int ExpectedInt = 10;
    const UInt8 StringLength = 200;
    const std::string ExpectedString(StringLength, 'x');
    std::array<UInt8, 4> StringSize = {0,0,0,StringLength};

    const char* Format = std::get<0>(GetParam());
    int StreamDataSize = std::get<1>(GetParam());
    UInt8* StreamData = getStreamData(StreamDataSize);

    EXPECT_CALL(stream, read(_, _))
        .WillOnce(
            DoAll(
                SetValueToVoidPointerArg0(StreamData, StreamDataSize),
                Return(StreamDataSize)
            )
        )
        .WillOnce(
            DoAll(
                SetValueToVoidPointerArg0(StringSize.data(), sizeof(StringSize.size())),
                Return(StringSize.size())
            )
         )
        .WillOnce(
            DoAll(
                SetValueToVoidPointerArg0(ExpectedString.c_str(), ExpectedString.length()),
                Return(ExpectedString.length())
            )
        );

    switch(StreamDataSize){
        case 2:
            EXPECT_TRUE(ProtocolUtil::readf(&stream, Format, &ActualInt16, &ActualString));
            EXPECT_EQ(ExpectedInt, ActualInt16);
             break;
        case 4:
            EXPECT_TRUE(ProtocolUtil::readf(&stream, Format, &ActualInt32, &ActualString));
            EXPECT_EQ(ExpectedInt, ActualInt32);
            break;
        default:
            EXPECT_TRUE(ProtocolUtil::readf(&stream, Format, &ActualInt8, &ActualString));
            EXPECT_EQ(ExpectedInt, ActualInt8);
            break;
    }
    EXPECT_EQ(ExpectedString, ActualString);
}

INSTANTIATE_TEST_CASE_P(
        IntAndStringTest,
        ReadfIntAndStringTest,
        ::testing::Values(
                std::make_tuple("%1i%s", 1),
                std::make_tuple("%2i%s", 2),
                std::make_tuple("%4i%s", 4)));


TEST_F(ProtocolUtilTests, readf_string_and_int4bytes)
{
    const UInt8 ExpectedInt = 10;
    std::array<UInt8, 4> StreamIntData = {0,0,0,ExpectedInt};

    const std::string ExpectedStr(32768, 'x');
    std::array<UInt8, 4> Size = {0,0,128,0};

    EXPECT_CALL(stream, read(_, _))
        .WillOnce(
            DoAll(
                SetValueToVoidPointerArg0(Size.data(), Size.size()),
                Return(Size.size())
            )
         )
        .WillOnce(
            DoAll(
                SetValueToVoidPointerArg0(ExpectedStr.c_str(), ExpectedStr.length()),
                Return(ExpectedStr.length())
            )
        )
        .WillOnce(
            DoAll(
                SetValueToVoidPointerArg0(StreamIntData.data(), StreamIntData.size()),
                Return(StreamIntData.size())
            )
        );

    EXPECT_TRUE(ProtocolUtil::readf(&stream, "%s%4i", &ActualString, &ActualInt32));
    EXPECT_EQ(ExpectedStr, ActualString);
    EXPECT_EQ(ExpectedInt, ActualInt32);
}

TEST_F(ProtocolUtilTests, readf_string_and_vector_int4bytes)
{
    std::vector<UInt32> Actual = {};
    const std::vector<UInt32> Expected4Bytes = {10,10};
    std::array<UInt8, 4> StreamVectorSize = {0,0,0,2};
    std::array<UInt8, 4> StreamData4Bytes = {0, 0, 0, 10};

    const std::string ExpString(32768, 'x');
    std::array<UInt8, 4> SizeString = {0,0,128,0};

    EXPECT_CALL(stream, read(_, _))
        .WillOnce(
            DoAll(
                SetValueToVoidPointerArg0(SizeString.data(), SizeString.size()),
                Return(SizeString.size())
            )
         )
        .WillOnce(
            DoAll(
                SetValueToVoidPointerArg0(ExpString.c_str(), ExpString.length()),
                Return(ExpString.length())
            )
        )
        .WillOnce(
            DoAll(
                SetValueToVoidPointerArg0(StreamVectorSize.data(), StreamVectorSize.size()),
                Return(StreamVectorSize.size())
            )
        )
        .WillRepeatedly(
            DoAll(
                SetValueToVoidPointerArg0(StreamData4Bytes.data(), StreamData4Bytes.size()),
                Return(StreamData4Bytes.size())
            )
        );

    EXPECT_TRUE(ProtocolUtil::readf(&stream, "%s%4I", &ActualString, &Actual));
    EXPECT_EQ(ExpString, ActualString);
    EXPECT_EQ(Expected4Bytes, Actual);
}

TEST_F(ProtocolUtilTests, readf_vector_int4bytes_and_string)
{
    std::vector<UInt32> Actual4Bytes = {};
    const std::vector<UInt32> Expected4Bytes = {10,10};
    std::array<UInt8, 4> StreamVectorSize = {0,0,0,2};
    std::array<UInt8, 4> StreamData4Bytes = {0, 0, 0, 10};

    const std::string ExpectedString(32768, 'x');
    std::array<UInt8, 4> StringSize = {0,0,128,0};

    EXPECT_CALL(stream, read(_, _))
        .WillOnce(
            DoAll(
                SetValueToVoidPointerArg0(StreamVectorSize.data(), StreamVectorSize.size()),
                Return(StreamVectorSize.size())
            )
        )
        .WillOnce(
            DoAll(
                SetValueToVoidPointerArg0(StreamData4Bytes.data(), StreamData4Bytes.size()),
                Return(StreamData4Bytes.size())
            )
        )
        .WillOnce(
            DoAll(
                SetValueToVoidPointerArg0(StreamData4Bytes.data(), StreamData4Bytes.size()),
                Return(StreamData4Bytes.size())
            )
        )
        .WillOnce(
            DoAll(
                SetValueToVoidPointerArg0(StringSize.data(), StringSize.size()),
                Return(StringSize.size())
            )
         )
        .WillOnce(
            DoAll(
                SetValueToVoidPointerArg0(ExpectedString.c_str(), ExpectedString.length()),
                Return(ExpectedString.length())
            )
        );

    EXPECT_TRUE(ProtocolUtil::readf(&stream, "%4I%s", &Actual4Bytes, &ActualString));
    EXPECT_EQ(ExpectedString, ActualString);
    EXPECT_EQ(Expected4Bytes, Actual4Bytes);
}

