/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Synergy Si, Inc.
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "synergy/ArgParser.h"
#include "synergy/ArgsBase.h"

#include "test/global/gtest.h"

TEST(CArgParserTests, isArg_abbreviationsArg_returnTrue)
{
	int i = 1;
	const int argc = 2;
	const char* argv[argc] = { "stub", "-t" };
	bool result = CArgParser::isArg(i, argc, argv, "-t", NULL);

	EXPECT_EQ(true, result);
}

TEST(CArgParserTests, isArg_fullArg_returnTrue)
{
	int i = 1;
	const int argc = 2;
	const char* argv[argc] = { "stub", "--test" };
	bool result = CArgParser::isArg(i, argc, argv, NULL, "--test");

	EXPECT_EQ(true, result);
}

TEST(CArgParserTests, isArg_missingArgs_returnFalse)
{
	int i = 1;
	const int argc = 2;
	const char* argv[argc] = { "stub", "-t" };
	CArgParser argParser(NULL);
	CArgsBase argsBase;
	argParser.setArgsBase(argsBase);

	bool result = CArgParser::isArg(i, argc, argv, "-t", NULL, 1);

	EXPECT_FALSE(result);
	EXPECT_EQ(true, argsBase.m_shouldExit);
}

TEST(CArgParserTests, searchDoubleQuotes_doubleQuotedArg_returnTrue)
{
	CString command("\"stub\"");
	size_t left = 0;
	size_t right = 0;

	bool result = CArgParser::searchDoubleQuotes(command, left, right);

	EXPECT_EQ(true, result);
	EXPECT_EQ(0, left);
	EXPECT_EQ(5, right);
}

TEST(CArgParserTests, searchDoubleQuotes_noDoubleQuotedArg_returnfalse)
{
	CString command("stub");
	size_t left = 0;
	size_t right = 0;

	bool result = CArgParser::searchDoubleQuotes(command, left, right);

	EXPECT_FALSE(result);
	EXPECT_EQ(0, left);
	EXPECT_EQ(0, right);
}

TEST(CArgParserTests, searchDoubleQuotes_oneDoubleQuoteArg_returnfalse)
{
	CString command("\"stub");
	size_t left = 0;
	size_t right = 0;

	bool result = CArgParser::searchDoubleQuotes(command, left, right);

	EXPECT_FALSE(result);
	EXPECT_EQ(0, left);
	EXPECT_EQ(0, right);
}

TEST(CArgParserTests, splitCommandString_oneArg_returnArgv)
{
	CString command("stub");
	std::vector<CString> argv;

	CArgParser::splitCommandString(command, argv);

	EXPECT_EQ(1, argv.size());
	EXPECT_EQ("stub", argv.at(0));
}

TEST(CArgParserTests, splitCommandString_twoArgs_returnArgv)
{
	CString command("stub1 stub2");
	std::vector<CString> argv;

	CArgParser::splitCommandString(command, argv);

	EXPECT_EQ(2, argv.size());
	EXPECT_EQ("stub1", argv.at(0));
	EXPECT_EQ("stub2", argv.at(1));
}

TEST(CArgParserTests, splitCommandString_doubleQuotedArgs_returnArgv)
{
	CString command("\"stub1\" stub2 \"stub3\"");
	std::vector<CString> argv;

	CArgParser::splitCommandString(command, argv);

	EXPECT_EQ(3, argv.size());
	EXPECT_EQ("stub1", argv.at(0));
	EXPECT_EQ("stub2", argv.at(1));
	EXPECT_EQ("stub3", argv.at(2));
}

TEST(CArgParserTests, splitCommandString_spaceDoubleQuotedArgs_returnArgv)
{
	CString command("\"stub1\" stub2 \"stub3 space\"");
	std::vector<CString> argv;

	CArgParser::splitCommandString(command, argv);

	EXPECT_EQ(3, argv.size());
	EXPECT_EQ("stub1", argv.at(0));
	EXPECT_EQ("stub2", argv.at(1));
	EXPECT_EQ("stub3 space", argv.at(2));
}

TEST(CArgParserTests, getArgv_stringArray_return2DArray)
{
	std::vector<CString> argArray;
	argArray.push_back("stub1");
	argArray.push_back("stub2");
	argArray.push_back("stub3 space");
	const char** argv = CArgParser::getArgv(argArray);

	CString row1(argv[0]);
	CString row2(argv[1]);
	CString row3(argv[2]);

	EXPECT_EQ("stub1", row1);
	EXPECT_EQ("stub2", row2);
	EXPECT_EQ("stub3 space", row3);

	delete[] argv;
}

TEST(CArgParserTests, assembleCommand_stringArray_returnCommand)
{
	std::vector<CString> argArray;
	argArray.push_back("stub1");
	argArray.push_back("stub2");
	CString command = CArgParser::assembleCommand(argArray);

	EXPECT_EQ("stub1 stub2", command);
}

TEST(CArgParserTests, assembleCommand_ignoreSecondArg_returnCommand)
{
	std::vector<CString> argArray;
	argArray.push_back("stub1");
	argArray.push_back("stub2");
	CString command = CArgParser::assembleCommand(argArray, "stub2");

	EXPECT_EQ("stub1", command);
}

TEST(CArgParserTests, assembleCommand_ignoreSecondArgWithOneParameter_returnCommand)
{
	std::vector<CString> argArray;
	argArray.push_back("stub1");
	argArray.push_back("stub2");
	argArray.push_back("stub3");
	argArray.push_back("stub4");
	CString command = CArgParser::assembleCommand(argArray, "stub2", 1);

	EXPECT_EQ("stub1 stub4", command);
}

TEST(CArgParserTests, assembleCommand_stringArrayWithSpace_returnCommand)
{
	std::vector<CString> argArray;
	argArray.push_back("stub1 space");
	argArray.push_back("stub2");
	argArray.push_back("stub3 space");
	CString command = CArgParser::assembleCommand(argArray);

	EXPECT_EQ("\"stub1 space\" stub2 \"stub3 space\"", command);
}

