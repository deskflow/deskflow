/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2011 Nick Bolton
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

#include "synergy/Clipboard.h"

#include "test/global/gtest.h"

TEST (ClipboardTests, empty_openCalled_returnsTrue) {
    Clipboard clipboard;
    clipboard.open (0);

    bool actual = clipboard.empty ();

    EXPECT_EQ (true, actual);
}

TEST (ClipboardTests, empty_singleFormat_hasReturnsFalse) {
    Clipboard clipboard;
    clipboard.open (0);
    clipboard.add (Clipboard::kText, "synergy rocks!");

    clipboard.empty ();

    bool actual = clipboard.has (Clipboard::kText);
    EXPECT_FALSE (actual);
}

TEST (ClipboardTests, add_newValue_valueWasStored) {
    Clipboard clipboard;
    clipboard.open (0);

    clipboard.add (IClipboard::kText, "synergy rocks!");

    String actual = clipboard.get (IClipboard::kText);
    EXPECT_EQ ("synergy rocks!", actual);
}

TEST (ClipboardTests, add_replaceValue_valueWasReplaced) {
    Clipboard clipboard;
    clipboard.open (0);

    clipboard.add (IClipboard::kText, "synergy rocks!");
    clipboard.add (IClipboard::kText, "maxivista sucks"); // haha, just kidding.

    String actual = clipboard.get (IClipboard::kText);
    EXPECT_EQ ("maxivista sucks", actual);
}

TEST (ClipboardTests, open_timeIsZero_returnsTrue) {
    Clipboard clipboard;

    bool actual = clipboard.open (0);

    EXPECT_EQ (true, actual);
}

TEST (ClipboardTests, open_timeIsOne_returnsTrue) {
    Clipboard clipboard;

    bool actual = clipboard.open (1);

    EXPECT_EQ (true, actual);
}

TEST (ClipboardTests, close_isOpen_noErrors) {
    Clipboard clipboard;
    clipboard.open (0);

    clipboard.close ();

    // can't assert anything
}

TEST (ClipboardTests, getTime_openWithNoEmpty_returnsZero) {
    Clipboard clipboard;
    clipboard.open (1);

    Clipboard::Time actual = clipboard.getTime ();

    EXPECT_EQ (0, actual);
}

TEST (ClipboardTests, getTime_openAndEmpty_returnsOne) {
    Clipboard clipboard;
    clipboard.open (1);
    clipboard.empty ();

    Clipboard::Time actual = clipboard.getTime ();

    EXPECT_EQ (1, actual);
}

TEST (ClipboardTests, has_withFormatAdded_returnsTrue) {
    Clipboard clipboard;
    clipboard.open (0);
    clipboard.add (IClipboard::kText, "synergy rocks!");

    bool actual = clipboard.has (IClipboard::kText);

    EXPECT_EQ (true, actual);
}

TEST (ClipboardTests, has_withNoFormats_returnsFalse) {
    Clipboard clipboard;
    clipboard.open (0);

    bool actual = clipboard.has (IClipboard::kText);

    EXPECT_FALSE (actual);
}

TEST (ClipboardTests, get_withNoFormats_returnsEmpty) {
    Clipboard clipboard;
    clipboard.open (0);

    String actual = clipboard.get (IClipboard::kText);

    EXPECT_EQ ("", actual);
}

TEST (ClipboardTests, get_withFormatAdded_returnsExpected) {
    Clipboard clipboard;
    clipboard.open (0);
    clipboard.add (IClipboard::kText, "synergy rocks!");

    String actual = clipboard.get (IClipboard::kText);

    EXPECT_EQ ("synergy rocks!", actual);
}

TEST (ClipboardTests, marshall_addNotCalled_firstCharIsZero) {
    Clipboard clipboard;

    String actual = clipboard.marshall ();

    // seems to return "\0\0\0\0" but EXPECT_EQ can't assert this,
    // so instead, just assert that first char is '\0'.
    EXPECT_EQ (0, (int) actual[0]);
}

TEST (ClipboardTests, marshall_withTextAdded_typeCharIsText) {
    Clipboard clipboard;
    clipboard.open (0);
    clipboard.add (IClipboard::kText, "synergy rocks!");
    clipboard.close ();

    String actual = clipboard.marshall ();

    // string contains other data, but 8th char should be kText.
    EXPECT_EQ (IClipboard::kText, (int) actual[7]);
}

TEST (ClipboardTests, marshall_withTextAdded_lastSizeCharIs14) {
    Clipboard clipboard;
    clipboard.open (0);
    clipboard.add (IClipboard::kText, "synergy rocks!"); // 14 chars
    clipboard.close ();

    String actual = clipboard.marshall ();

    EXPECT_EQ (14, (int) actual[11]);
}

// TODO: there's some integer -> char encoding going on here. i find it
// hard to believe that the clipboard is the only thing doing this. maybe
// we should refactor this stuff out of the clipboard.
TEST (ClipboardTests, marshall_withTextSize285_sizeCharsValid) {
    // 285 chars
    String data;
    data.append ("Synergy is Free and Open Source Software that lets you ");
    data.append ("easily share your mouse and keyboard between multiple ");
    data.append ("computers, where each computer has it's own display. No ");
    data.append ("special hardware is required, all you need is a local area ");
    data.append (
        "network. Synergy is supported on Windows, Mac OS X and Linux.");

    Clipboard clipboard;
    clipboard.open (0);
    clipboard.add (IClipboard::kText, data);
    clipboard.close ();

    String actual = clipboard.marshall ();

    // 4 asserts here, but that's ok because we're really just asserting 1
    // thing. the 32-bit size value is split into 4 chars. if the size is 285
    // (29 more than the 8-bit max size), the last char "rolls over" to 29
    // (this is caused by a bit-wise & on 0xff and 8-bit truncation). each
    // char before the last stores a bit-shifted version of the number, each
    // 1 more power than the last, which is done by bit-shifting [0] by 24,
    // [1] by 16, [2] by 8 ([3] is not bit-shifted).
    EXPECT_EQ (0, actual[8]);   // 285 >> 24 = 285 / (256^3) = 0
    EXPECT_EQ (0, actual[9]);   // 285 >> 16 = 285 / (256^2) = 0
    EXPECT_EQ (1, actual[10]);  // 285 >> 8 = 285 / (256^1) = 1(.11328125)
    EXPECT_EQ (29, actual[11]); // 285 - 256 = 29
}

TEST (ClipboardTests, marshall_withHtmlAdded_typeCharIsHtml) {
    Clipboard clipboard;
    clipboard.open (0);
    clipboard.add (IClipboard::kHTML, "html sucks");
    clipboard.close ();

    String actual = clipboard.marshall ();

    // string contains other data, but 8th char should be kHTML.
    EXPECT_EQ (IClipboard::kHTML, (int) actual[7]);
}

TEST (ClipboardTests, marshall_withHtmlAndText_has2Formats) {
    Clipboard clipboard;
    clipboard.open (0);
    clipboard.add (IClipboard::kText, "synergy rocks");
    clipboard.add (IClipboard::kHTML, "html sucks");
    clipboard.close ();

    String actual = clipboard.marshall ();

    // the number of formats is stored inside the first 4 chars.
    // the writeUInt32 function right-aligns numbers in 4 chars,
    // so if you right align 2, it will be "\0\0\0\2" in a string.
    // we assert that the char at the 4th index is 2 (the number of
    // formats that we've added).
    EXPECT_EQ (2, (int) actual[3]);
}

TEST (ClipboardTests, marshall_withTextAdded_endsWithAdded) {
    Clipboard clipboard;
    clipboard.open (0);
    clipboard.add (IClipboard::kText, "synergy rocks!");
    clipboard.close ();

    String actual = clipboard.marshall ();

    // string contains other data, but should end in the string we added.
    EXPECT_EQ ("synergy rocks!", actual.substr (12));
}

TEST (ClipboardTests, unmarshall_emptyData_hasTextIsFalse) {
    Clipboard clipboard;

    String data;
    data += (char) 0;
    data += (char) 0;
    data += (char) 0;
    data += (char) 0; // 0 formats added

    clipboard.unmarshall (data, 0);

    clipboard.open (0);
    bool actual = clipboard.has (IClipboard::kText);
    EXPECT_FALSE (actual);
}

TEST (ClipboardTests, unmarshall_withTextSize285_getTextIsValid) {
    Clipboard clipboard;

    // 285 chars
    String text;
    text.append ("Synergy is Free and Open Source Software that lets you ");
    text.append ("easily share your mouse and keyboard between multiple ");
    text.append ("computers, where each computer has it's own display. No ");
    text.append ("special hardware is required, all you need is a local area ");
    text.append (
        "network. Synergy is supported on Windows, Mac OS X and Linux.");

    String data;
    data += (char) 0;
    data += (char) 0;
    data += (char) 0;
    data += (char) 1; // 1 format added
    data += (char) 0;
    data += (char) 0;
    data += (char) 0;
    data += (char) IClipboard::kText;
    data += (char) 0;  // 285 >> 24 = 285 / (256^3) = 0
    data += (char) 0;  // 285 >> 16 = 285 / (256^2) = 0
    data += (char) 1;  // 285 >> 8 = 285 / (256^1) = 1(.11328125)
    data += (char) 29; // 285 - 256 = 29
    data += text;

    clipboard.unmarshall (data, 0);

    clipboard.open (0);
    String actual = clipboard.get (IClipboard::kText);
    EXPECT_EQ (text, actual);
}

TEST (ClipboardTests, unmarshall_withTextAndHtml_getTextIsValid) {
    Clipboard clipboard;
    String data;
    data += (char) 0;
    data += (char) 0;
    data += (char) 0;
    data += (char) 2; // 2 formats added
    data += (char) 0;
    data += (char) 0;
    data += (char) 0;
    data += (char) IClipboard::kText;
    data += (char) 0;
    data += (char) 0;
    data += (char) 0;
    data += (char) 14;
    data += "synergy rocks!";
    data += (char) 0;
    data += (char) 0;
    data += (char) 0;
    data += (char) IClipboard::kHTML;
    data += (char) 0;
    data += (char) 0;
    data += (char) 0;
    data += (char) 10;
    data += "html sucks";

    clipboard.unmarshall (data, 0);

    clipboard.open (0);
    String actual = clipboard.get (IClipboard::kText);
    EXPECT_EQ ("synergy rocks!", actual);
}

TEST (ClipboardTests, unmarshall_withTextAndHtml_getHtmlIsValid) {
    Clipboard clipboard;
    String data;
    data += (char) 0;
    data += (char) 0;
    data += (char) 0;
    data += (char) 2; // 2 formats added
    data += (char) 0;
    data += (char) 0;
    data += (char) 0;
    data += (char) IClipboard::kText;
    data += (char) 0;
    data += (char) 0;
    data += (char) 0;
    data += (char) 14;
    data += "synergy rocks!";
    data += (char) 0;
    data += (char) 0;
    data += (char) 0;
    data += (char) IClipboard::kHTML;
    data += (char) 0;
    data += (char) 0;
    data += (char) 0;
    data += (char) 10;
    data += "html sucks";

    clipboard.unmarshall (data, 0);

    clipboard.open (0);
    String actual = clipboard.get (IClipboard::kHTML);
    EXPECT_EQ ("html sucks", actual);
}

TEST (ClipboardTests, copy_withSingleText_clipboardsAreEqual) {
    Clipboard clipboard1;
    clipboard1.open (0);
    clipboard1.add (Clipboard::kText, "synergy rocks!");
    clipboard1.close ();

    Clipboard clipboard2;
    Clipboard::copy (&clipboard2, &clipboard1);

    clipboard2.open (0);
    String actual = clipboard2.get (Clipboard::kText);
    EXPECT_EQ ("synergy rocks!", actual);
}
