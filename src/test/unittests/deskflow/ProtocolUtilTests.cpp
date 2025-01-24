/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 - 2020 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/ProtocolUtil.h"
#include "test/mock/io/MockStream.h"

#include <array>
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::DoAll;
using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrEq;
using ::testing::TypedEq;

ACTION_P2(SetValueToVoidPointerArg0, value, size)
{
  memcpy(arg0, value, size);
}

MATCHER_P(EqVoidPointeeInt8, expected, "")
{
  const uint8_t Actual8 = (*static_cast<const uint8_t *>(arg));
  return (expected == Actual8);
}

MATCHER_P(EqVoidPointeeInt16, expected, "")
{
  const uint16_t Actual16 = (*static_cast<const uint16_t *>(arg));
  return (expected == (Actual16 >> 8));
}

MATCHER_P(EqVoidPointeeInt32, expected, "")
{
  const uint32_t Actual32 = (*static_cast<const uint32_t *>(arg));
  return (expected == (Actual32 >> 24));
}

MATCHER_P(EqVoidVectorInt1byte, expected, "")
{
  bool Result = true;
  const uint8_t *Actual = (static_cast<const uint8_t *>(arg)) + 4;
  const size_t Size = *(Actual - 1);

  if (Size == expected.size()) {
    for (size_t i = 0; i < expected.size(); ++i) {
      if (expected[i] != Actual[i]) {
        Result = false;
        break;
      }
    }
  } else {
    Result = false;
  }

  return Result;
}

MATCHER_P(EqVoidVectorInt2bytes, expected, "")
{
  bool Result = true;
  const uint16_t *Actual = (static_cast<const uint16_t *>(arg)) + 2;
  const size_t Size = *(Actual - 1) >> 8;

  if (Size == expected.size()) {
    for (size_t i = 0; i < expected.size(); ++i) {
      if (expected[i] != (Actual[i] >> 8)) {
        Result = false;
        break;
      }
    }
  } else {
    Result = false;
  }

  return Result;
}

MATCHER_P(EqVoidVectorInt4bytes, expected, "")
{
  bool Result = true;
  const uint32_t *Actual = (static_cast<const uint32_t *>(arg)) + 1;
  const size_t Size = *(Actual - 1) >> 24;

  if (Size == expected.size()) {
    for (size_t i = 0; i < expected.size(); ++i) {
      if (expected[i] != (Actual[i] >> 24)) {
        Result = false;
        break;
      }
    }
  } else {
    Result = false;
  }

  return Result;
}

MATCHER_P(EqVectorSymbols, expected, "")
{
  bool Result = true;
  const uint8_t *Actual = (static_cast<const uint8_t *>(arg));

  for (size_t i = 0; i < expected.size(); ++i) {
    if (expected[i] != (Actual[i])) {
      Result = false;
      break;
    }
  }

  return Result;
}

ACTION(ThrowBadAlloc)
{
  throw std::bad_alloc();
}

class ProtocolUtilTests : public ::testing::Test
{
public:
  MockStream stream;
  uint8_t ActualInt8 = 0;
  uint16_t ActualInt16 = 0;
  uint32_t ActualInt32 = 0;
  std::string ActualString;
};

// TODO: fix tests causing segmentation fault
#if 0
TEST_F(ProtocolUtilTests, readf__XIOEndOfStream_exception) {
  ON_CALL(stream, read(_, _)).WillByDefault(Return(0));

  EXPECT_FALSE(ProtocolUtil::readf(&stream, "%s", &ActualString));
  EXPECT_TRUE(ActualString.empty());
}

TEST_F(ProtocolUtilTests, readf_XIOReadMismatch_exception) {
  EXPECT_CALL(stream, read(_, _))
      .WillOnce(DoAll(SetValueToVoidPointerArg0("b", 1), Return(1)));

  EXPECT_FALSE(ProtocolUtil::readf(&stream, "a%s", &ActualString));
  EXPECT_TRUE(ActualString.empty());
}

TEST_F(ProtocolUtilTests, readf_bad_alloc_exception) {
  ON_CALL(stream, read(_, _)).WillByDefault(ThrowBadAlloc());

  EXPECT_FALSE(ProtocolUtil::readf(&stream, "a%s", &ActualString));
  EXPECT_TRUE(ActualString.empty());
}

TEST_F(ProtocolUtilTests, readf_asserts) {
  ASSERT_DEBUG_DEATH(
      { ProtocolUtil::readf(&stream, "%x", &ActualString); },
      "invalid format specifier");

  ASSERT_DEBUG_DEATH(
      { ProtocolUtil::readf(&stream, "%5i", &ActualString); },
      "length to be read is wrong:");

  ASSERT_DEBUG_DEATH(
      { ProtocolUtil::readf(&stream, "%5I", &ActualString); }, "");
}

TEST_F(ProtocolUtilTests, readf_params_validation) {
  EXPECT_FALSE(ProtocolUtil::readf(NULL, "%x", NULL));
  EXPECT_FALSE(ProtocolUtil::readf(&stream, NULL, NULL));
}

TEST_F(ProtocolUtilTests, readf_string) {
  const uint8_t Length = 200;
  const std::string Expected(Length, 'x');
  std::array<uint8_t, 4> StringSize{{0, 0, 0, Length}};

  EXPECT_CALL(stream, read(_, _))
      .WillOnce(
          DoAll(SetValueToVoidPointerArg0(StringSize.data(), StringSize.size()),
                Return(StringSize.size())))
      .WillOnce(
          DoAll(SetValueToVoidPointerArg0(Expected.c_str(), Expected.length()),
                Return(Expected.length())));

  EXPECT_TRUE(ProtocolUtil::readf(&stream, "%s", &ActualString));
  EXPECT_EQ(Expected, ActualString);
}

class ReadfIntTestFixture
    : public ::testing::TestWithParam<std::tuple<const char *, int>> {
public:
  MockStream stream;
  uint8_t StreamData1Byte = 10;
  std::array<uint8_t, 2> StreamData2Bytes{{0, 10}};
  std::array<uint8_t, 4> StreamData4Bytes{{0, 0, 0, 10}};

  uint8_t *getStreamData(int size) {
    uint8_t *StreamData = nullptr;
    switch (size) {
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

TEST_P(ReadfIntTestFixture, readf_int) {
  int Actual = 0;
  const int Expected = 10;
  const char *Format = std::get<0>(GetParam());
  int StreamDataSize = std::get<1>(GetParam());
  uint8_t *StreamData = getStreamData(StreamDataSize);

  ON_CALL(stream, read(_, _))
      .WillByDefault(
          DoAll(SetValueToVoidPointerArg0(StreamData, StreamDataSize),
                Return(StreamDataSize)));

  EXPECT_TRUE(ProtocolUtil::readf(&stream, Format, &Actual));
  EXPECT_EQ(Expected, Actual);
}

INSTANTIATE_TEST_SUITE_P(ReadfIntTests, ReadfIntTestFixture,
                         ::testing::Values(std::make_tuple("%1i", 1),
                                           std::make_tuple("%2i", 2),
                                           std::make_tuple("%4i", 4)));

class ReadfIntVectorTestFixture : public ReadfIntTestFixture {};

TEST_P(ReadfIntVectorTestFixture, readf_int_vector) {
  std::vector<uint8_t> Actual1Byte = {};
  std::vector<uint16_t> Actual2Bytes = {};
  std::vector<uint32_t> Actual4Bytes = {};

  const std::vector<uint8_t> Expected1Byte = {10, 10};
  const std::vector<uint16_t> Expected2Bytes = {10, 10};
  const std::vector<uint32_t> Expected4Bytes = {10, 10};
  std::array<uint8_t, 4> StreamVectorSize{{0, 0, 0, 2}};

  const char *Format = std::get<0>(GetParam());
  int StreamDataSize = std::get<1>(GetParam());
  uint8_t *StreamData = getStreamData(StreamDataSize);

  MockStream stream;
  EXPECT_CALL(stream, read(_, _))
      .WillOnce(DoAll(SetValueToVoidPointerArg0(StreamVectorSize.data(),
                                                StreamVectorSize.size()),
                      Return(StreamVectorSize.size())))
      .WillRepeatedly(
          DoAll(SetValueToVoidPointerArg0(StreamData, StreamDataSize),
                Return(StreamDataSize)));

  switch (StreamDataSize) {
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

INSTANTIATE_TEST_SUITE_P(ReadfIntVectorTests, ReadfIntVectorTestFixture,
                         ::testing::Values(std::make_tuple("%1I", 1),
                                           std::make_tuple("%2I", 2),
                                           std::make_tuple("%4I", 4)));

class ReadfIntAndStringTest : public ReadfIntTestFixture {
public:
  uint8_t ActualInt8 = 0;
  uint16_t ActualInt16 = 0;
  uint32_t ActualInt32 = 32;
  std::string ActualString;
};

TEST_P(ReadfIntAndStringTest, readf_int_and_string) {
  const int ExpectedInt = 10;
  const uint8_t StringLength = 200;
  const std::string ExpectedString(StringLength, 'x');
  std::array<uint8_t, 4> StringSize{{0, 0, 0, StringLength}};

  const char *Format = std::get<0>(GetParam());
  int StreamDataSize = std::get<1>(GetParam());
  uint8_t *StreamData = getStreamData(StreamDataSize);

  EXPECT_CALL(stream, read(_, _))
      .WillOnce(DoAll(SetValueToVoidPointerArg0(StreamData, StreamDataSize),
                      Return(StreamDataSize)))
      .WillOnce(
          DoAll(SetValueToVoidPointerArg0(StringSize.data(), StringSize.size()),
                Return(StringSize.size())))
      .WillOnce(DoAll(SetValueToVoidPointerArg0(ExpectedString.c_str(),
                                                ExpectedString.length()),
                      Return(ExpectedString.length())));

  switch (StreamDataSize) {
  case 2:
    EXPECT_TRUE(
        ProtocolUtil::readf(&stream, Format, &ActualInt16, &ActualString));
    EXPECT_EQ(ExpectedInt, ActualInt16);
    break;
  case 4:
    EXPECT_TRUE(
        ProtocolUtil::readf(&stream, Format, &ActualInt32, &ActualString));
    EXPECT_EQ(ExpectedInt, ActualInt32);
    break;
  default:
    EXPECT_TRUE(
        ProtocolUtil::readf(&stream, Format, &ActualInt8, &ActualString));
    EXPECT_EQ(ExpectedInt, ActualInt8);
    break;
  }
  EXPECT_EQ(ExpectedString, ActualString);
}

INSTANTIATE_TEST_SUITE_P(IntAndStringTest, ReadfIntAndStringTest,
                         ::testing::Values(std::make_tuple("%1i%s", 1),
                                           std::make_tuple("%2i%s", 2),
                                           std::make_tuple("%4i%s", 4)));

TEST_F(ProtocolUtilTests, readf_string_and_int4bytes) {
  const uint8_t ExpectedInt = 10;
  std::array<uint8_t, 4> StreamIntData{{0, 0, 0, ExpectedInt}};

  const std::string ExpectedStr(32768, 'x');
  std::array<uint8_t, 4> Size{{0, 0, 128, 0}};

  EXPECT_CALL(stream, read(_, _))
      .WillOnce(DoAll(SetValueToVoidPointerArg0(Size.data(), Size.size()),
                      Return(Size.size())))
      .WillOnce(DoAll(
          SetValueToVoidPointerArg0(ExpectedStr.c_str(), ExpectedStr.length()),
          Return(ExpectedStr.length())))
      .WillOnce(DoAll(
          SetValueToVoidPointerArg0(StreamIntData.data(), StreamIntData.size()),
          Return(StreamIntData.size())));

  EXPECT_TRUE(
      ProtocolUtil::readf(&stream, "%s%4i", &ActualString, &ActualInt32));
  EXPECT_EQ(ExpectedStr, ActualString);
  EXPECT_EQ(ExpectedInt, ActualInt32);
}

TEST_F(ProtocolUtilTests, readf_string_and_vector_int4bytes) {
  std::vector<uint32_t> Actual = {};
  const std::vector<uint32_t> Expected4Bytes = {10, 10};
  std::array<uint8_t, 4> StreamVectorSize{{0, 0, 0, 2}};
  std::array<uint8_t, 4> StreamData4Bytes{{0, 0, 0, 10}};

  const std::string ExpString(32768, 'x');
  std::array<uint8_t, 4> SizeString{{0, 0, 128, 0}};

  EXPECT_CALL(stream, read(_, _))
      .WillOnce(
          DoAll(SetValueToVoidPointerArg0(SizeString.data(), SizeString.size()),
                Return(SizeString.size())))
      .WillOnce(DoAll(
          SetValueToVoidPointerArg0(ExpString.c_str(), ExpString.length()),
          Return(ExpString.length())))
      .WillOnce(DoAll(SetValueToVoidPointerArg0(StreamVectorSize.data(),
                                                StreamVectorSize.size()),
                      Return(StreamVectorSize.size())))
      .WillRepeatedly(DoAll(SetValueToVoidPointerArg0(StreamData4Bytes.data(),
                                                      StreamData4Bytes.size()),
                            Return(StreamData4Bytes.size())));

  EXPECT_TRUE(ProtocolUtil::readf(&stream, "%s%4I", &ActualString, &Actual));
  EXPECT_EQ(ExpString, ActualString);
  EXPECT_EQ(Expected4Bytes, Actual);
}

TEST_F(ProtocolUtilTests, readf_vector_int4bytes_and_string) {
  std::vector<uint32_t> Actual4Bytes = {};
  const std::vector<uint32_t> Expected4Bytes = {10, 10};
  std::array<uint8_t, 4> StreamVectorSize{{0, 0, 0, 2}};
  std::array<uint8_t, 4> StreamData4Bytes{{0, 0, 0, 10}};

  const std::string ExpectedString(32768, 'x');
  std::array<uint8_t, 4> StringSize{{0, 0, 128, 0}};

  EXPECT_CALL(stream, read(_, _))
      .WillOnce(DoAll(SetValueToVoidPointerArg0(StreamVectorSize.data(),
                                                StreamVectorSize.size()),
                      Return(StreamVectorSize.size())))
      .WillOnce(DoAll(SetValueToVoidPointerArg0(StreamData4Bytes.data(),
                                                StreamData4Bytes.size()),
                      Return(StreamData4Bytes.size())))
      .WillOnce(DoAll(SetValueToVoidPointerArg0(StreamData4Bytes.data(),
                                                StreamData4Bytes.size()),
                      Return(StreamData4Bytes.size())))
      .WillOnce(
          DoAll(SetValueToVoidPointerArg0(StringSize.data(), StringSize.size()),
                Return(StringSize.size())))
      .WillOnce(DoAll(SetValueToVoidPointerArg0(ExpectedString.c_str(),
                                                ExpectedString.length()),
                      Return(ExpectedString.length())));

  EXPECT_TRUE(
      ProtocolUtil::readf(&stream, "%4I%s", &Actual4Bytes, &ActualString));
  EXPECT_EQ(ExpectedString, ActualString);
  EXPECT_EQ(Expected4Bytes, Actual4Bytes);
}

class WriteIntTest
    : public ::testing::TestWithParam<std::tuple<const char *, int>> {
public:
  MockStream stream;
  uint8_t Expected1Byte = 5;
  uint16_t Expected2Bytes = 10;
  uint32_t Expected4Bytes = 15;
};

TEST_P(WriteIntTest, write_int) {
  const char *Format = std::get<0>(GetParam());
  const int DataSize = std::get<1>(GetParam());
  switch (DataSize) {
  case 2:
    EXPECT_CALL(stream,
                write(EqVoidPointeeInt16(Expected2Bytes), Eq(DataSize)));
    ProtocolUtil::writef(&stream, Format, Expected2Bytes);
    break;
  case 4:
    EXPECT_CALL(stream,
                write(EqVoidPointeeInt32(Expected4Bytes), Eq(DataSize)));
    ProtocolUtil::writef(&stream, Format, Expected4Bytes);
    break;
  default:
    EXPECT_CALL(stream, write(EqVoidPointeeInt8(Expected1Byte), Eq(DataSize)));
    ProtocolUtil::writef(&stream, Format, Expected1Byte);
    break;
  }
}

INSTANTIATE_TEST_SUITE_P(WriteIntTest, WriteIntTest,
                         ::testing::Values(std::make_tuple("%1i", 1),
                                           std::make_tuple("%2i", 2),
                                           std::make_tuple("%4i", 4)));

class WriteIntVectorTest
    : public ::testing::TestWithParam<std::tuple<const char *, int>> {
public:
  MockStream stream;
  const std::vector<uint8_t> Expected1Byte = {10, 20, 30};
  const std::vector<uint16_t> Expected2Byte = {40, 50, 60};
  const std::vector<uint32_t> Expected4Byte = {70, 80, 90};
};

TEST_P(WriteIntVectorTest, write_vector_int) {
  const char *Format = std::get<0>(GetParam());
  const int Type = std::get<1>(GetParam());

  switch (Type) {
  case 2:
    EXPECT_CALL(stream,
                write(EqVoidVectorInt2bytes(Expected2Byte),
                      Eq(Type * Expected2Byte.size() + sizeof(uint32_t))));
    ProtocolUtil::writef(&stream, Format, &Expected2Byte);
    break;
  case 4:
    EXPECT_CALL(stream,
                write(EqVoidVectorInt4bytes(Expected4Byte),
                      Eq(Type * Expected4Byte.size() + sizeof(uint32_t))));
    ProtocolUtil::writef(&stream, Format, &Expected4Byte);
    break;
  default:
    EXPECT_CALL(stream, write(EqVoidVectorInt1byte(Expected1Byte),
                              Eq(Expected1Byte.size() + sizeof(uint32_t))));
    ProtocolUtil::writef(&stream, Format, &Expected1Byte);
    break;
  }
}

INSTANTIATE_TEST_SUITE_P(WriteIntVectorTest, WriteIntVectorTest,
                         ::testing::Values(std::make_tuple("%1I", 1),
                                           std::make_tuple("%2I", 2),
                                           std::make_tuple("%4I", 4)));

TEST_F(ProtocolUtilTests, write_string_test) {
  const std::string Expected = "Expected";
  const std::vector<uint8_t> ExpectedVector = {'E', 'x', 'p', 'e',
                                             'c', 't', 'e', 'd'};
  EXPECT_CALL(stream, write(EqVoidVectorInt1byte(ExpectedVector),
                            Expected.size() + sizeof(uint32_t)));
  ProtocolUtil::writef(&stream, "%s", &Expected);
}

TEST_F(ProtocolUtilTests, write_raw_bytes_test) {
  const uint32_t Size = 5;
  const std::array<uint8_t, Size> Expected{{10, 20, 30, 40, 50}};
  EXPECT_CALL(stream, write(EqVoidVectorInt1byte(Expected),
                            Expected.size() + sizeof(uint32_t)));
  ProtocolUtil::writef(&stream, "%S", Size, &Expected);
}

TEST_F(ProtocolUtilTests, write_symbols_from_format_test) {
  const std::vector<uint8_t> Expected = {'%', '1', '2', '3', '4', '5'};
  EXPECT_CALL(stream, write(EqVectorSymbols(Expected), Expected.size()));
  ProtocolUtil::writef(&stream, "%%12345");
}

#endif
