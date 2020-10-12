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

TEST(ProtocolUtilTests, readf__XIOEndOfStream_exception)
{
    std::string Data;
    MockStream stream;
    ON_CALL(stream, read(_, _)).WillByDefault(Return(0));

    EXPECT_FALSE(ProtocolUtil::readf(&stream, "%s", &Data));
    EXPECT_TRUE(Data.empty());
}

TEST(ProtocolUtilTests, readf_XIOReadMismatch_exception)
{
    std::string Data;
    MockStream stream;
    EXPECT_CALL(stream, read(_, _))
        .WillOnce(DoAll(SetValueToVoidPointerArg0("b", 1), Return(1)));

    EXPECT_FALSE(ProtocolUtil::readf(&stream, "a%s", &Data));
    EXPECT_TRUE(Data.empty());
}

TEST(ProtocolUtilTests, readf_bad_alloc_exception)
{
    std::string Data;
    MockStream stream;
    ON_CALL(stream, read(_, _)).WillByDefault(ThrowBadAlloc());

    EXPECT_FALSE(ProtocolUtil::readf(&stream, "a%s", &Data));
    EXPECT_TRUE(Data.empty());
}

TEST(ProtocolUtilTests, readf_asserts)
{
    MockStream stream;
    std::string Data;
    ASSERT_DEBUG_DEATH(
        {ProtocolUtil::readf(&stream, "%x", &Data);},
        "invalid format specifier"
    );

    ASSERT_DEBUG_DEATH(
        {ProtocolUtil::readf(NULL, "%s", &Data);},
        ""
    );

    ASSERT_DEBUG_DEATH(
        {ProtocolUtil::readf(&stream, NULL, &Data);},
        ""
    );

    ASSERT_DEBUG_DEATH(
        {ProtocolUtil::readf(&stream, "%5i", &Data);},
        "length to be read is wrong:"
    );

    ASSERT_DEBUG_DEATH(
        {ProtocolUtil::readf(&stream, "%5I", &Data);},
        ""
    );
}

TEST(ProtocolUtilTests, readf_string)
{
    std::string Data;
    const std::string Expected = "expected string";
    const UInt8 Length = Expected.length();
    UInt8 Size[4] = {0,0,0,Length};

    MockStream stream;
    EXPECT_CALL(stream, read(_, _))
        .WillOnce(
            DoAll(
                SetValueToVoidPointerArg0(&Size, sizeof(Size)),
                Return(sizeof(Size))
            )
         )
        .WillOnce(
            DoAll(
                SetValueToVoidPointerArg0(Expected.c_str(), Expected.length()),
                Return(Expected.length())
            )
        );

    EXPECT_TRUE(ProtocolUtil::readf(&stream, "%s", &Data));
    EXPECT_EQ(Expected, Data);
}

TEST(ProtocolUtilTests, readf_string_200)
{
    std::string Data;
    const std::string Expected(200, 'x');
    const UInt8 Length = Expected.length();
    UInt8 Size[4] = {0,0,0,Length};

    MockStream stream;
    EXPECT_CALL(stream, read(_, _))
        .WillOnce(
            DoAll(
                SetValueToVoidPointerArg0(&Size, sizeof(Size)),
                Return(sizeof(Size))
            )
         )
        .WillOnce(
            DoAll(
                SetValueToVoidPointerArg0(Expected.c_str(), Expected.length()),
                Return(Expected.length())
            )
        );

    EXPECT_TRUE(ProtocolUtil::readf(&stream, "%s", &Data));
    EXPECT_EQ(Expected, Data);
}

class ReadfIntTestFixture : public ::testing::TestWithParam< std::tuple<const char*, int> >
{
protected:
    UInt8 StreamData1Byte[1] = {10};
    UInt8 StreamData2Bytes[2] = {0, 10};
    UInt8 StreamData4Bytes[4] = {0, 0, 0, 10};

    UInt8* getStreamData(int size)
    {
        UInt8* StreamData = nullptr;
        switch(size){
            case 2:
                StreamData = StreamData2Bytes;
                break;
            case 4:
                StreamData = StreamData4Bytes;
                break;
            default:
                StreamData = StreamData1Byte;
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

    MockStream stream;
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
    UInt8 StreamVectorSize[4] = {0,0,0,2};

    const char* Format = std::get<0>(GetParam());
    int StreamDataSize = std::get<1>(GetParam());
    UInt8* StreamData = getStreamData(StreamDataSize);

    MockStream stream;
    EXPECT_CALL(stream, read(_, _))
        .WillOnce(
            DoAll(
                SetValueToVoidPointerArg0(&StreamVectorSize, sizeof (StreamVectorSize)),
                Return(sizeof (StreamVectorSize))
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

TEST(ProtocolUtilTests, readf_int1byte_and_string)
{
    std::string ActualString;
    const std::string ExpectedString(200, 'x');
    const UInt8 StringLength = ExpectedString.length();
    UInt8 Size[4] = {0,0,0,StringLength};

    UInt8 ActualInt = 0;
    const int ExpectedInt = 10;
    UInt8 StreamIntData = ExpectedInt;

    MockStream stream;
    EXPECT_CALL(stream, read(_, _))
        .WillOnce(
            DoAll(
                SetValueToVoidPointerArg0(&StreamIntData, sizeof(StreamIntData)),
                Return(sizeof(StreamIntData))
            )
        )
        .WillOnce(
            DoAll(
                SetValueToVoidPointerArg0(&Size, sizeof(Size)),
                Return(sizeof(Size))
            )
         )
        .WillOnce(
            DoAll(
                SetValueToVoidPointerArg0(ExpectedString.c_str(), ExpectedString.length()),
                Return(ExpectedString.length())
            )
        );

    EXPECT_TRUE(ProtocolUtil::readf(&stream, "%1i%s", &ActualInt, &ActualString));
    EXPECT_EQ(ExpectedString, ActualString);
    EXPECT_EQ(ExpectedInt, ActualInt);
}

TEST(ProtocolUtilTests, readf_int2byte_and_string)
{
    std::string ActualString;
    const std::string ExpectedString(200, 'x');
    const UInt8 StringLength = ExpectedString.length();
    UInt8 Size[4] = {0,0,0,StringLength};

    UInt8 ActualInt = 0;
    const int ExpectedInt = 10;
    UInt8 StreamIntData[2] = {0, ExpectedInt};

    MockStream stream;
    EXPECT_CALL(stream, read(_, _))
        .WillOnce(
            DoAll(
                SetValueToVoidPointerArg0(&StreamIntData, sizeof(StreamIntData)),
                Return(sizeof(StreamIntData))
            )
        )
        .WillOnce(
            DoAll(
                SetValueToVoidPointerArg0(&Size, sizeof(Size)),
                Return(sizeof(Size))
            )
         )
        .WillOnce(
            DoAll(
                SetValueToVoidPointerArg0(ExpectedString.c_str(), ExpectedString.length()),
                Return(ExpectedString.length())
            )
        );

    EXPECT_TRUE(ProtocolUtil::readf(&stream, "%2i%s", &ActualInt, &ActualString));
    EXPECT_EQ(ExpectedString, ActualString);
    EXPECT_EQ(ExpectedInt, ActualInt);
}

TEST(ProtocolUtilTests, readf_int4byte_and_string)
{
    UInt8 ActualInt = 0;
    const int ExpectedInt = 10;
    UInt8 StreamIntData[4] = {0,0,0,ExpectedInt};

    std::string ActualString;
    const std::string ExpectedString(32768, 'x');
    UInt8 Size[4] = {0,0,128,0};

    MockStream stream;
    EXPECT_CALL(stream, read(_, _))
        .WillOnce(
            DoAll(
                SetValueToVoidPointerArg0(&StreamIntData, sizeof(StreamIntData)),
                Return(sizeof(StreamIntData))
            )
        )
        .WillOnce(
            DoAll(
                SetValueToVoidPointerArg0(&Size, sizeof(Size)),
                Return(sizeof(Size))
            )
         )
        .WillOnce(
            DoAll(
                SetValueToVoidPointerArg0(ExpectedString.c_str(), ExpectedString.length()),
                Return(ExpectedString.length())
            )
        );

    EXPECT_TRUE(ProtocolUtil::readf(&stream, "%4i%s", &ActualInt, &ActualString));
    EXPECT_EQ(ExpectedString, ActualString);
    EXPECT_EQ(ExpectedInt, ActualInt);
}











