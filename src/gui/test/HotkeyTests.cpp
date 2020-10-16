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


#include "../src/Hotkey.h"
#include <gtest/gtest.h>
#include "Utils.h"

#include <QtCore/QSettings>
#include <QtCore/QTextStream>

struct TestAction
{
    Action::ActionType type = Action::keyDown;
    std::vector<TestKey> keys;
    std::vector<std::string> type_screen_names;
    std::string screen_name;
    Action::SwitchDirection switch_direction;
    Action::LockCursorMode lock_cursor_mode;

    static TestAction createKeyAction(Action::ActionType type, const std::vector<TestKey>& keys,
                                      const std::vector<std::string>& type_screen_names = {})
    {
        TestAction action;
        action.type = Action::keyDown;
        action.keys = keys;
        action.type_screen_names = type_screen_names;
        return action;
    }

    static TestAction createKeyDown(const std::vector<TestKey>& keys,
                                    const std::vector<std::string>& type_screen_names = {})
    {
        return createKeyAction(Action::keyDown, keys, type_screen_names);
    }

    static TestAction createKeyUp(const std::vector<TestKey>& keys,
                                  const std::vector<std::string>& type_screen_names = {})
    {
        return createKeyAction(Action::keyUp, keys, type_screen_names);
    }

    static TestAction createKeyStroke(const std::vector<TestKey>& keys,
                                      const std::vector<std::string>& type_screen_names = {})
    {
        return createKeyAction(Action::keystroke, keys, type_screen_names);
    }

    static TestAction createSwitchToScreen(const std::string& screen_name)
    {
        TestAction action;
        action.type = Action::switchToScreen;
        action.screen_name = screen_name;
        return action;
    }

    static TestAction createToggleScreen()
    {
        TestAction action;
        action.type = Action::toggleScreen;
        return action;
    }

    static TestAction createSwitchInDirection(Action::SwitchDirection switch_direction)
    {
        TestAction action;
        action.type = Action::switchInDirection;
        action.switch_direction = switch_direction;
        return action;
    }

    static TestAction createLockCursorToScreen(Action::LockCursorMode lock_cursor_mode)
    {
        TestAction action;
        action.type = Action::lockCursorToScreen;
        action.lock_cursor_mode = lock_cursor_mode;
        return action;
    }
};

struct TestHotKey
{
    std::vector<TestKey> keys;
    std::vector<TestAction> actions;
};

namespace {

    Action createAction(const TestAction& test_action)
    {
        Action action;
        action.setType(test_action.type);

        switch (test_action.type) {
            case Action::keyDown:
            case Action::keyUp:
            case Action::keystroke: {
                KeySequence sequence;
                for (auto key : test_action.keys) {
                    sequence.appendKey(key.key, key.modifier);
                }
                action.setKeySequence(sequence);
                for (const auto& type_screen_name : test_action.type_screen_names) {
                    action.appendTypeScreenName(QString::fromStdString(type_screen_name));
                }
                break;
            }
            case Action::switchToScreen:
                action.setSwitchScreenName(QString::fromStdString(test_action.screen_name));
                break;
            case Action::toggleScreen:
                break;
            case Action::switchInDirection:
                action.setSwitchDirection(test_action.switch_direction);
                break;
            case Action::lockCursorToScreen:
                action.setLockCursorMode(test_action.lock_cursor_mode);
                break;
        }
        return action;
    }

    Hotkey createHotkey(const TestHotKey& test_hotkey)
    {
        Hotkey hotkey;
        KeySequence sequence;
        for (auto key : test_hotkey.keys) {
            sequence.appendKey(key.key, key.modifier);
        }
        hotkey.setKeySequence(sequence);

        for (auto action : test_hotkey.actions) {
            hotkey.appendAction(createAction(action));
        }
        return hotkey;
    }

    std::string hotkeyToStringViaTextStream(const Hotkey& hotkey)
    {
        QString result;
        QTextStream stream{&result};
        stream << hotkey;
        return result.toStdString();
    }
} // namespace

void doHotkeyLoadSaveTest(const TestHotKey& test_hotkey)
{
    auto filename = getTemporaryFilename();

    Hotkey hotkey_before, hotkey_after;
    {
        QSettings settings(filename, QSettings::NativeFormat);

        hotkey_before = createHotkey(test_hotkey);

        settings.beginGroup("test");
        hotkey_before.saveSettings(settings);
        settings.endGroup();
    }
    {
        QSettings settings(filename, QSettings::NativeFormat);

        settings.beginGroup("test");
        hotkey_after.loadSettings(settings);
        settings.endGroup();

        ASSERT_EQ(hotkey_before.keySequence().sequence(), hotkey_after.keySequence().sequence());
        ASSERT_EQ(hotkey_before.keySequence().modifiers(), hotkey_after.keySequence().modifiers());

        const auto& actions_before = hotkey_before.actions();
        const auto& actions_after = hotkey_after.actions();

        ASSERT_EQ(actions_before.size(), actions_after.size());
        for (int i = 0; i < actions_before.size(); ++i) {
            const auto& action_before = actions_before[i];
            const auto& action_after = actions_after[i];

            ASSERT_EQ(action_before.keySequence().sequence(), action_after.keySequence().sequence());
            ASSERT_EQ(action_before.keySequence().modifiers(), action_after.keySequence().modifiers());
            ASSERT_EQ(action_before.type(), action_after.type());
            ASSERT_EQ(action_before.typeScreenNames(), action_after.typeScreenNames());
            ASSERT_EQ(action_before.switchScreenName(), action_after.switchScreenName());
            ASSERT_EQ(action_before.switchDirection(), action_after.switchDirection());
            ASSERT_EQ(action_before.lockCursorMode(), action_after.lockCursorMode());
            ASSERT_EQ(action_before.activeOnRelease(), action_after.activeOnRelease());
            ASSERT_EQ(action_before.haveScreens(), action_after.haveScreens());
        }
    }

    QFile::remove(filename);
}

TEST(HotkeyLoadSaveTests, Empty)
{
    TestHotKey hotkey;
    doHotkeyLoadSaveTest(hotkey);
}

TEST(HotkeyLoadSaveTests, KeysNoActions)
{
    TestHotKey hotkey = {{{Qt::Key_A, Qt::NoModifier}, {Qt::Key_B, Qt::NoModifier}}, {}};
    doHotkeyLoadSaveTest(hotkey);
}

TEST(HotkeyLoadSaveTests, CommaKeyNoActions)
{
    TestHotKey hotkey = {
        {
            {Qt::Key_A, Qt::NoModifier},
            {Qt::Key_Comma, Qt::NoModifier},
            {Qt::Key_B, Qt::NoModifier}
        }, {}};
    doHotkeyLoadSaveTest(hotkey);
}

TEST(HotkeyLoadSaveTests, KeysSingleAction)
{
    TestHotKey hotkey = {
        {
            {Qt::Key_A, Qt::NoModifier},
            {Qt::Key_B, Qt::NoModifier}
        },
        {
            TestAction::createKeyDown({{Qt::Key_Z, Qt::NoModifier}})
        }
    };
    doHotkeyLoadSaveTest(hotkey);
}

TEST(HotkeyLoadSaveTests, KeysMultipleAction)
{
    TestHotKey hotkey = {
        {
            {Qt::Key_A, Qt::NoModifier},
            {Qt::Key_B, Qt::NoModifier}
        },
        {
            TestAction::createKeyDown({{Qt::Key_Z, Qt::NoModifier}}),
            TestAction::createSwitchToScreen("test_screen")
        }
    };
    doHotkeyLoadSaveTest(hotkey);
}

TEST(HotkeyToTexStreamTests, Empty)
{
    TestHotKey hotkey;
    ASSERT_EQ(hotkeyToStringViaTextStream(createHotkey(hotkey)), "");
}

TEST(HotkeyToTexStreamTests, KeysNoActions)
{
    TestHotKey hotkey = {
        {
            {Qt::Key_A, Qt::NoModifier},
            {Qt::Key_B, Qt::NoModifier}
        },
        {}
    };
    ASSERT_EQ(hotkeyToStringViaTextStream(createHotkey(hotkey)), "");
}

TEST(HotkeyToTexStreamTests, KeysSingleAction)
{
    TestHotKey hotkey = {
        {
            {Qt::Key_A, Qt::NoModifier},
            {Qt::Key_B, Qt::NoModifier}
        },
        {}
    };
    ASSERT_EQ(hotkeyToStringViaTextStream(createHotkey(hotkey)), "");
}


TEST(HotkeyToTexStreamTests, KeysCommaSingleAction)
{
    TestHotKey hotkey = {
        {
            {Qt::Key_A, Qt::NoModifier},
            {Qt::Key_Comma, Qt::NoModifier},
            {Qt::Key_B, Qt::NoModifier}
        },
        {
            TestAction::createKeyDown({{Qt::Key_Z, Qt::NoModifier}})
        }
    };
    ASSERT_EQ(hotkeyToStringViaTextStream(createHotkey(hotkey)),
              "\tkeystroke(a+Comma+b) = keyDown(z,*)\n");
}

TEST(HotkeyToTexStreamTests, KeysMultipleAction)
{
    TestHotKey hotkey = {
        {
            {Qt::Key_A, Qt::NoModifier},
            {Qt::Key_B, Qt::NoModifier}
        },
        {
            TestAction::createKeyDown({{Qt::Key_Z, Qt::NoModifier}}),
            TestAction::createSwitchToScreen("test_screen")
        }
    };
    ASSERT_EQ(hotkeyToStringViaTextStream(createHotkey(hotkey)),
              "\tkeystroke(a+b) = keyDown(z,*), switchToScreen(test_screen)\n");
}
