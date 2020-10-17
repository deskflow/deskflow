/*  barrier -- mouse and keyboard sharing utility
    Copyright (C) 2021 Povilas Kanapickas <povilas@radix.lt>

    This package is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    found in the file LICENSE that should have accompanied this file.

    This package is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../src/KeySequence.h"
#include "Utils.h"
#include <gtest/gtest.h>
#include <cstdio>

#include <QtCore/QFile>
#include <QtCore/QSettings>
#include <QtCore/QTemporaryFile>

namespace {

    auto s_key_sequence_test_keys = {
        Qt::Key_Space,
        Qt::Key_Escape,
        Qt::Key_Tab,
        Qt::Key_Backtab,
        Qt::Key_Backspace,
        Qt::Key_Return,
        Qt::Key_Insert,
        Qt::Key_Delete,
        Qt::Key_Pause,
        Qt::Key_Print,
        Qt::Key_SysReq,
        Qt::Key_Home,
        Qt::Key_End,
        Qt::Key_Left,
        Qt::Key_Up,
        Qt::Key_Right,
        Qt::Key_Down,
        Qt::Key_Comma,
        Qt::Key_Semicolon,
        Qt::Key_PageUp,
        Qt::Key_PageDown,
        Qt::Key_CapsLock,
        Qt::Key_NumLock,
        Qt::Key_ScrollLock,
        Qt::Key_Help,
        Qt::Key_Enter,
        Qt::Key_Clear,
        Qt::Key_Back,
        Qt::Key_Forward,
        Qt::Key_Stop,
        Qt::Key_Refresh,
        Qt::Key_VolumeDown,
        Qt::Key_VolumeMute,
        Qt::Key_VolumeUp,
        Qt::Key_MediaPlay,
        Qt::Key_MediaStop,
        Qt::Key_MediaPrevious,
        Qt::Key_MediaNext,
        Qt::Key_HomePage,
        Qt::Key_Favorites,
        Qt::Key_Search,
        Qt::Key_Standby,
        Qt::Key_LaunchMail,
        Qt::Key_LaunchMedia,
        Qt::Key_Launch0,
        Qt::Key_Launch1,
        Qt::Key_Select,
    };

    std::string keySequenceToString(const std::vector<TestKey>& key_pairs)
    {
        KeySequence sequence;
        for (auto key_pair : key_pairs) {
            sequence.appendKey(key_pair.key, key_pair.modifier);
        }
        return sequence.toString().toStdString();
    }
} // namespace

class KeySequenceLoadSaveTestFixture :
        public ::testing::TestWithParam<Qt::Key> {};

TEST_P(KeySequenceLoadSaveTestFixture, SupportsSpecialSymbols)
{
    int key = GetParam();

    auto filename = getTemporaryFilename();

    {
        QSettings settings(filename, QSettings::NativeFormat);
        KeySequence sequence;

        sequence.appendKey(key, 0);
        sequence.appendKey(key, 0);
        settings.beginGroup("test");
        sequence.saveSettings(settings);
        settings.endGroup();
    }
    {
        QSettings settings(filename, QSettings::NativeFormat);
        KeySequence sequence;

        settings.beginGroup("test");
        sequence.loadSettings(settings);
        settings.endGroup();

        const auto& data = sequence.sequence();
        ASSERT_EQ(data.size(), 2);
        ASSERT_EQ(data[0], key);
        ASSERT_EQ(data[1], key);
    }

    QFile::remove(filename);
}

INSTANTIATE_TEST_CASE_P(
        KeySequenceLoadSaveTests,
        KeySequenceLoadSaveTestFixture,
        ::testing::ValuesIn(s_key_sequence_test_keys));

TEST(KeySequenceTests, ToString)
{
    ASSERT_EQ(keySequenceToString({{Qt::Key_Menu, Qt::MetaModifier}}),
              "Meta");
    ASSERT_EQ(keySequenceToString({{Qt::Key_A, 0}, {Qt::Key_B, 0}}),
              "a+b");
    ASSERT_EQ(keySequenceToString({{Qt::Key_A, 0}, {Qt::Key_Comma, 0}, {Qt::Key_B, 0}}),
              "a+Comma+b");
    ASSERT_EQ(keySequenceToString({{Qt::Key_A, 0}, {Qt::Key_Semicolon, 0}, {Qt::Key_B, 0}}),
              "a+Semicolon+b");
    ASSERT_EQ(keySequenceToString({{Qt::Key_A, 0}, {Qt::Key_Shift, Qt::ShiftModifier},
                                   {Qt::Key_0, Qt::ShiftModifier}}),
              "a+Shift+0");
    ASSERT_EQ(keySequenceToString({{Qt::Key_A, 0}, {Qt::Key_Control, Qt::ControlModifier},
                                   {Qt::Key_0, Qt::ControlModifier}}),
              "a+Control+0");
}
