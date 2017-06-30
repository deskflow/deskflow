/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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

#include "synergy/KeyState.h"
#include "base/Log.h"

#include <cstring>
#include <algorithm>
#include <iterator>
#include <list>

static const KeyButton kButtonMask = (KeyButton) (IKeyState::kNumButtons - 1);

static const KeyID s_decomposeTable[] = {
    // spacing version of dead keys
    0x0060,
    0x0300,
    0x0020,
    0, // grave,        dead_grave,       space
    0x00b4,
    0x0301,
    0x0020,
    0, // acute,        dead_acute,       space
    0x005e,
    0x0302,
    0x0020,
    0, // asciicircum,  dead_circumflex,  space
    0x007e,
    0x0303,
    0x0020,
    0, // asciitilde,   dead_tilde,       space
    0x00a8,
    0x0308,
    0x0020,
    0, // diaeresis,    dead_diaeresis,   space
    0x00b0,
    0x030a,
    0x0020,
    0, // degree,       dead_abovering,   space
    0x00b8,
    0x0327,
    0x0020,
    0, // cedilla,      dead_cedilla,     space
    0x02db,
    0x0328,
    0x0020,
    0, // ogonek,       dead_ogonek,      space
    0x02c7,
    0x030c,
    0x0020,
    0, // caron,        dead_caron,       space
    0x02d9,
    0x0307,
    0x0020,
    0, // abovedot,     dead_abovedot,    space
    0x02dd,
    0x030b,
    0x0020,
    0, // doubleacute,  dead_doubleacute, space
    0x02d8,
    0x0306,
    0x0020,
    0, // breve,        dead_breve,       space
    0x00af,
    0x0304,
    0x0020,
    0, // macron,       dead_macron,      space

    // Latin-1 (ISO 8859-1)
    0x00c0,
    0x0300,
    0x0041,
    0, // Agrave,       dead_grave,       A
    0x00c1,
    0x0301,
    0x0041,
    0, // Aacute,       dead_acute,       A
    0x00c2,
    0x0302,
    0x0041,
    0, // Acircumflex,  dead_circumflex,  A
    0x00c3,
    0x0303,
    0x0041,
    0, // Atilde,       dead_tilde,       A
    0x00c4,
    0x0308,
    0x0041,
    0, // Adiaeresis,   dead_diaeresis,   A
    0x00c5,
    0x030a,
    0x0041,
    0, // Aring,        dead_abovering,   A
    0x00c7,
    0x0327,
    0x0043,
    0, // Ccedilla,     dead_cedilla,     C
    0x00c8,
    0x0300,
    0x0045,
    0, // Egrave,       dead_grave,       E
    0x00c9,
    0x0301,
    0x0045,
    0, // Eacute,       dead_acute,       E
    0x00ca,
    0x0302,
    0x0045,
    0, // Ecircumflex,  dead_circumflex,  E
    0x00cb,
    0x0308,
    0x0045,
    0, // Ediaeresis,   dead_diaeresis,   E
    0x00cc,
    0x0300,
    0x0049,
    0, // Igrave,       dead_grave,       I
    0x00cd,
    0x0301,
    0x0049,
    0, // Iacute,       dead_acute,       I
    0x00ce,
    0x0302,
    0x0049,
    0, // Icircumflex,  dead_circumflex,  I
    0x00cf,
    0x0308,
    0x0049,
    0, // Idiaeresis,   dead_diaeresis,   I
    0x00d1,
    0x0303,
    0x004e,
    0, // Ntilde,       dead_tilde,       N
    0x00d2,
    0x0300,
    0x004f,
    0, // Ograve,       dead_grave,       O
    0x00d3,
    0x0301,
    0x004f,
    0, // Oacute,       dead_acute,       O
    0x00d4,
    0x0302,
    0x004f,
    0, // Ocircumflex,  dead_circumflex,  O
    0x00d5,
    0x0303,
    0x004f,
    0, // Otilde,       dead_tilde,       O
    0x00d6,
    0x0308,
    0x004f,
    0, // Odiaeresis,   dead_diaeresis,   O
    0x00d9,
    0x0300,
    0x0055,
    0, // Ugrave,       dead_grave,       U
    0x00da,
    0x0301,
    0x0055,
    0, // Uacute,       dead_acute,       U
    0x00db,
    0x0302,
    0x0055,
    0, // Ucircumflex,  dead_circumflex,  U
    0x00dc,
    0x0308,
    0x0055,
    0, // Udiaeresis,   dead_diaeresis,   U
    0x00dd,
    0x0301,
    0x0059,
    0, // Yacute,       dead_acute,       Y
    0x00e0,
    0x0300,
    0x0061,
    0, // agrave,       dead_grave,       a
    0x00e1,
    0x0301,
    0x0061,
    0, // aacute,       dead_acute,       a
    0x00e2,
    0x0302,
    0x0061,
    0, // acircumflex,  dead_circumflex,  a
    0x00e3,
    0x0303,
    0x0061,
    0, // atilde,       dead_tilde,       a
    0x00e4,
    0x0308,
    0x0061,
    0, // adiaeresis,   dead_diaeresis,   a
    0x00e5,
    0x030a,
    0x0061,
    0, // aring,        dead_abovering,   a
    0x00e7,
    0x0327,
    0x0063,
    0, // ccedilla,     dead_cedilla,     c
    0x00e8,
    0x0300,
    0x0065,
    0, // egrave,       dead_grave,       e
    0x00e9,
    0x0301,
    0x0065,
    0, // eacute,       dead_acute,       e
    0x00ea,
    0x0302,
    0x0065,
    0, // ecircumflex,  dead_circumflex,  e
    0x00eb,
    0x0308,
    0x0065,
    0, // ediaeresis,   dead_diaeresis,   e
    0x00ec,
    0x0300,
    0x0069,
    0, // igrave,       dead_grave,       i
    0x00ed,
    0x0301,
    0x0069,
    0, // iacute,       dead_acute,       i
    0x00ee,
    0x0302,
    0x0069,
    0, // icircumflex,  dead_circumflex,  i
    0x00ef,
    0x0308,
    0x0069,
    0, // idiaeresis,   dead_diaeresis,   i
    0x00f1,
    0x0303,
    0x006e,
    0, // ntilde,       dead_tilde,       n
    0x00f2,
    0x0300,
    0x006f,
    0, // ograve,       dead_grave,       o
    0x00f3,
    0x0301,
    0x006f,
    0, // oacute,       dead_acute,       o
    0x00f4,
    0x0302,
    0x006f,
    0, // ocircumflex,  dead_circumflex,  o
    0x00f5,
    0x0303,
    0x006f,
    0, // otilde,       dead_tilde,       o
    0x00f6,
    0x0308,
    0x006f,
    0, // odiaeresis,   dead_diaeresis,   o
    0x00f9,
    0x0300,
    0x0075,
    0, // ugrave,       dead_grave,       u
    0x00fa,
    0x0301,
    0x0075,
    0, // uacute,       dead_acute,       u
    0x00fb,
    0x0302,
    0x0075,
    0, // ucircumflex,  dead_circumflex,  u
    0x00fc,
    0x0308,
    0x0075,
    0, // udiaeresis,   dead_diaeresis,   u
    0x00fd,
    0x0301,
    0x0079,
    0, // yacute,       dead_acute,       y
    0x00ff,
    0x0308,
    0x0079,
    0, // ydiaeresis,   dead_diaeresis,   y

    // Latin-2 (ISO 8859-2)
    0x0104,
    0x0328,
    0x0041,
    0, // Aogonek,      dead_ogonek,      A
    0x013d,
    0x030c,
    0x004c,
    0, // Lcaron,       dead_caron,       L
    0x015a,
    0x0301,
    0x0053,
    0, // Sacute,       dead_acute,       S
    0x0160,
    0x030c,
    0x0053,
    0, // Scaron,       dead_caron,       S
    0x015e,
    0x0327,
    0x0053,
    0, // Scedilla,     dead_cedilla,     S
    0x0164,
    0x030c,
    0x0054,
    0, // Tcaron,       dead_caron,       T
    0x0179,
    0x0301,
    0x005a,
    0, // Zacute,       dead_acute,       Z
    0x017d,
    0x030c,
    0x005a,
    0, // Zcaron,       dead_caron,       Z
    0x017b,
    0x0307,
    0x005a,
    0, // Zabovedot,    dead_abovedot,    Z
    0x0105,
    0x0328,
    0x0061,
    0, // aogonek,      dead_ogonek,      a
    0x013e,
    0x030c,
    0x006c,
    0, // lcaron,       dead_caron,       l
    0x015b,
    0x0301,
    0x0073,
    0, // sacute,       dead_acute,       s
    0x0161,
    0x030c,
    0x0073,
    0, // scaron,       dead_caron,       s
    0x015f,
    0x0327,
    0x0073,
    0, // scedilla,     dead_cedilla,     s
    0x0165,
    0x030c,
    0x0074,
    0, // tcaron,       dead_caron,       t
    0x017a,
    0x0301,
    0x007a,
    0, // zacute,       dead_acute,       z
    0x017e,
    0x030c,
    0x007a,
    0, // zcaron,       dead_caron,       z
    0x017c,
    0x0307,
    0x007a,
    0, // zabovedot,    dead_abovedot,    z
    0x0154,
    0x0301,
    0x0052,
    0, // Racute,       dead_acute,       R
    0x0102,
    0x0306,
    0x0041,
    0, // Abreve,       dead_breve,       A
    0x0139,
    0x0301,
    0x004c,
    0, // Lacute,       dead_acute,       L
    0x0106,
    0x0301,
    0x0043,
    0, // Cacute,       dead_acute,       C
    0x010c,
    0x030c,
    0x0043,
    0, // Ccaron,       dead_caron,       C
    0x0118,
    0x0328,
    0x0045,
    0, // Eogonek,      dead_ogonek,      E
    0x011a,
    0x030c,
    0x0045,
    0, // Ecaron,       dead_caron,       E
    0x010e,
    0x030c,
    0x0044,
    0, // Dcaron,       dead_caron,       D
    0x0143,
    0x0301,
    0x004e,
    0, // Nacute,       dead_acute,       N
    0x0147,
    0x030c,
    0x004e,
    0, // Ncaron,       dead_caron,       N
    0x0150,
    0x030b,
    0x004f,
    0, // Odoubleacute, dead_doubleacute, O
    0x0158,
    0x030c,
    0x0052,
    0, // Rcaron,       dead_caron,       R
    0x016e,
    0x030a,
    0x0055,
    0, // Uring,        dead_abovering,   U
    0x0170,
    0x030b,
    0x0055,
    0, // Udoubleacute, dead_doubleacute, U
    0x0162,
    0x0327,
    0x0054,
    0, // Tcedilla,     dead_cedilla,     T
    0x0155,
    0x0301,
    0x0072,
    0, // racute,       dead_acute,       r
    0x0103,
    0x0306,
    0x0061,
    0, // abreve,       dead_breve,       a
    0x013a,
    0x0301,
    0x006c,
    0, // lacute,       dead_acute,       l
    0x0107,
    0x0301,
    0x0063,
    0, // cacute,       dead_acute,       c
    0x010d,
    0x030c,
    0x0063,
    0, // ccaron,       dead_caron,       c
    0x0119,
    0x0328,
    0x0065,
    0, // eogonek,      dead_ogonek,      e
    0x011b,
    0x030c,
    0x0065,
    0, // ecaron,       dead_caron,       e
    0x010f,
    0x030c,
    0x0064,
    0, // dcaron,       dead_caron,       d
    0x0144,
    0x0301,
    0x006e,
    0, // nacute,       dead_acute,       n
    0x0148,
    0x030c,
    0x006e,
    0, // ncaron,       dead_caron,       n
    0x0151,
    0x030b,
    0x006f,
    0, // odoubleacute, dead_doubleacute, o
    0x0159,
    0x030c,
    0x0072,
    0, // rcaron,       dead_caron,       r
    0x016f,
    0x030a,
    0x0075,
    0, // uring,        dead_abovering,   u
    0x0171,
    0x030b,
    0x0075,
    0, // udoubleacute, dead_doubleacute, u
    0x0163,
    0x0327,
    0x0074,
    0, // tcedilla,     dead_cedilla,     t

    // Latin-3 (ISO 8859-3)
    0x0124,
    0x0302,
    0x0048,
    0, // Hcircumflex,  dead_circumflex,  H
    0x0130,
    0x0307,
    0x0049,
    0, // Iabovedot,    dead_abovedot,    I
    0x011e,
    0x0306,
    0x0047,
    0, // Gbreve,        dead_breve,       G
    0x0134,
    0x0302,
    0x004a,
    0, // Jcircumflex,  dead_circumflex,  J
    0x0125,
    0x0302,
    0x0068,
    0, // hcircumflex,  dead_circumflex,  h
    0x011f,
    0x0306,
    0x0067,
    0, // gbreve,        dead_breve,       g
    0x0135,
    0x0302,
    0x006a,
    0, // jcircumflex,  dead_circumflex,  j
    0x010a,
    0x0307,
    0x0043,
    0, // Cabovedot,    dead_abovedot,    C
    0x0108,
    0x0302,
    0x0043,
    0, // Ccircumflex,  dead_circumflex,  C
    0x0120,
    0x0307,
    0x0047,
    0, // Gabovedot,    dead_abovedot,    G
    0x011c,
    0x0302,
    0x0047,
    0, // Gcircumflex,  dead_circumflex,  G
    0x016c,
    0x0306,
    0x0055,
    0, // Ubreve,        dead_breve,       U
    0x015c,
    0x0302,
    0x0053,
    0, // Scircumflex,  dead_circumflex,  S
    0x010b,
    0x0307,
    0x0063,
    0, // cabovedot,    dead_abovedot,    c
    0x0109,
    0x0302,
    0x0063,
    0, // ccircumflex,  dead_circumflex,  c
    0x0121,
    0x0307,
    0x0067,
    0, // gabovedot,    dead_abovedot,    g
    0x011d,
    0x0302,
    0x0067,
    0, // gcircumflex,  dead_circumflex,  g
    0x016d,
    0x0306,
    0x0075,
    0, // ubreve,        dead_breve,       u
    0x015d,
    0x0302,
    0x0073,
    0, // scircumflex,  dead_circumflex,  s

    // Latin-4 (ISO 8859-4)
    0x0156,
    0x0327,
    0x0052,
    0, // Rcedilla,     dead_cedilla,      R
    0x0128,
    0x0303,
    0x0049,
    0, // Itilde,        dead_tilde,       I
    0x013b,
    0x0327,
    0x004c,
    0, // Lcedilla,     dead_cedilla,      L
    0x0112,
    0x0304,
    0x0045,
    0, // Emacron,      dead_macron,      E
    0x0122,
    0x0327,
    0x0047,
    0, // Gcedilla,     dead_cedilla,      G
    0x0157,
    0x0327,
    0x0072,
    0, // rcedilla,     dead_cedilla,      r
    0x0129,
    0x0303,
    0x0069,
    0, // itilde,        dead_tilde,       i
    0x013c,
    0x0327,
    0x006c,
    0, // lcedilla,     dead_cedilla,      l
    0x0113,
    0x0304,
    0x0065,
    0, // emacron,      dead_macron,      e
    0x0123,
    0x0327,
    0x0067,
    0, // gcedilla,     dead_cedilla,      g
    0x0100,
    0x0304,
    0x0041,
    0, // Amacron,      dead_macron,      A
    0x012e,
    0x0328,
    0x0049,
    0, // Iogonek,      dead_ogonek,      I
    0x0116,
    0x0307,
    0x0045,
    0, // Eabovedot,    dead_abovedot,    E
    0x012a,
    0x0304,
    0x0049,
    0, // Imacron,      dead_macron,      I
    0x0145,
    0x0327,
    0x004e,
    0, // Ncedilla,     dead_cedilla,      N
    0x014c,
    0x0304,
    0x004f,
    0, // Omacron,      dead_macron,      O
    0x0136,
    0x0327,
    0x004b,
    0, // Kcedilla,     dead_cedilla,      K
    0x0172,
    0x0328,
    0x0055,
    0, // Uogonek,      dead_ogonek,      U
    0x0168,
    0x0303,
    0x0055,
    0, // Utilde,        dead_tilde,       U
    0x016a,
    0x0304,
    0x0055,
    0, // Umacron,      dead_macron,      U
    0x0101,
    0x0304,
    0x0061,
    0, // amacron,      dead_macron,      a
    0x012f,
    0x0328,
    0x0069,
    0, // iogonek,      dead_ogonek,      i
    0x0117,
    0x0307,
    0x0065,
    0, // eabovedot,    dead_abovedot,    e
    0x012b,
    0x0304,
    0x0069,
    0, // imacron,      dead_macron,      i
    0x0146,
    0x0327,
    0x006e,
    0, // ncedilla,     dead_cedilla,      n
    0x014d,
    0x0304,
    0x006f,
    0, // omacron,      dead_macron,      o
    0x0137,
    0x0327,
    0x006b,
    0, // kcedilla,     dead_cedilla,      k
    0x0173,
    0x0328,
    0x0075,
    0, // uogonek,      dead_ogonek,      u
    0x0169,
    0x0303,
    0x0075,
    0, // utilde,        dead_tilde,       u
    0x016b,
    0x0304,
    0x0075,
    0, // umacron,      dead_macron,      u

    // Latin-8 (ISO 8859-14)
    0x1e02,
    0x0307,
    0x0042,
    0, // Babovedot,    dead_abovedot,    B
    0x1e03,
    0x0307,
    0x0062,
    0, // babovedot,    dead_abovedot,    b
    0x1e0a,
    0x0307,
    0x0044,
    0, // Dabovedot,    dead_abovedot,    D
    0x1e80,
    0x0300,
    0x0057,
    0, // Wgrave,        dead_grave,       W
    0x1e82,
    0x0301,
    0x0057,
    0, // Wacute,        dead_acute,       W
    0x1e0b,
    0x0307,
    0x0064,
    0, // dabovedot,    dead_abovedot,    d
    0x1ef2,
    0x0300,
    0x0059,
    0, // Ygrave,        dead_grave,       Y
    0x1e1e,
    0x0307,
    0x0046,
    0, // Fabovedot,    dead_abovedot,    F
    0x1e1f,
    0x0307,
    0x0066,
    0, // fabovedot,    dead_abovedot,    f
    0x1e40,
    0x0307,
    0x004d,
    0, // Mabovedot,    dead_abovedot,    M
    0x1e41,
    0x0307,
    0x006d,
    0, // mabovedot,    dead_abovedot,    m
    0x1e56,
    0x0307,
    0x0050,
    0, // Pabovedot,    dead_abovedot,    P
    0x1e81,
    0x0300,
    0x0077,
    0, // wgrave,        dead_grave,       w
    0x1e57,
    0x0307,
    0x0070,
    0, // pabovedot,    dead_abovedot,    p
    0x1e83,
    0x0301,
    0x0077,
    0, // wacute,        dead_acute,       w
    0x1e60,
    0x0307,
    0x0053,
    0, // Sabovedot,    dead_abovedot,    S
    0x1ef3,
    0x0300,
    0x0079,
    0, // ygrave,        dead_grave,       y
    0x1e84,
    0x0308,
    0x0057,
    0, // Wdiaeresis,    dead_diaeresis,   W
    0x1e85,
    0x0308,
    0x0077,
    0, // wdiaeresis,    dead_diaeresis,   w
    0x1e61,
    0x0307,
    0x0073,
    0, // sabovedot,    dead_abovedot,    s
    0x0174,
    0x0302,
    0x0057,
    0, // Wcircumflex,  dead_circumflex,  W
    0x1e6a,
    0x0307,
    0x0054,
    0, // Tabovedot,    dead_abovedot,    T
    0x0176,
    0x0302,
    0x0059,
    0, // Ycircumflex,  dead_circumflex,  Y
    0x0175,
    0x0302,
    0x0077,
    0, // wcircumflex,  dead_circumflex,  w
    0x1e6b,
    0x0307,
    0x0074,
    0, // tabovedot,    dead_abovedot,    t
    0x0177,
    0x0302,
    0x0079,
    0, // ycircumflex,  dead_circumflex,  y

    // Latin-9 (ISO 8859-15)
    0x0178,
    0x0308,
    0x0059,
    0, // Ydiaeresis,   dead_diaeresis,   Y

    // Compose key sequences
    0x00c6,
    kKeyCompose,
    0x0041,
    0x0045,
    0, // AE,             A,           E
    0x00c1,
    kKeyCompose,
    0x0041,
    0x0027,
    0, // Aacute,         A,           apostrophe
    0x00c2,
    kKeyCompose,
    0x0041,
    0x0053,
    0, // Acircumflex,    A,           asciicircum
    0x00c3,
    kKeyCompose,
    0x0041,
    0x0022,
    0, // Adiaeresis,     A,           quotedbl
    0x00c0,
    kKeyCompose,
    0x0041,
    0x0060,
    0, // Agrave,         A,           grave
    0x00c5,
    kKeyCompose,
    0x0041,
    0x002a,
    0, // Aring,           A,           asterisk
    0x00c3,
    kKeyCompose,
    0x0041,
    0x007e,
    0, // Atilde,         A,           asciitilde
    0x00c7,
    kKeyCompose,
    0x0043,
    0x002c,
    0, // Ccedilla,       C,           comma
    0x00d0,
    kKeyCompose,
    0x0044,
    0x002d,
    0, // ETH,            D,           minus
    0x00c9,
    kKeyCompose,
    0x0045,
    0x0027,
    0, // Eacute,         E,           apostrophe
    0x00ca,
    kKeyCompose,
    0x0045,
    0x0053,
    0, // Ecircumflex,    E,           asciicircum
    0x00cb,
    kKeyCompose,
    0x0045,
    0x0022,
    0, // Ediaeresis,     E,           quotedbl
    0x00c8,
    kKeyCompose,
    0x0045,
    0x0060,
    0, // Egrave,         E,           grave
    0x00cd,
    kKeyCompose,
    0x0049,
    0x0027,
    0, // Iacute,         I,           apostrophe
    0x00ce,
    kKeyCompose,
    0x0049,
    0x0053,
    0, // Icircumflex,    I,           asciicircum
    0x00cf,
    kKeyCompose,
    0x0049,
    0x0022,
    0, // Idiaeresis,     I,           quotedbl
    0x00cc,
    kKeyCompose,
    0x0049,
    0x0060,
    0, // Igrave,         I,           grave
    0x00d1,
    kKeyCompose,
    0x004e,
    0x007e,
    0, // Ntilde,         N,           asciitilde
    0x00d3,
    kKeyCompose,
    0x004f,
    0x0027,
    0, // Oacute,         O,           apostrophe
    0x00d4,
    kKeyCompose,
    0x004f,
    0x0053,
    0, // Ocircumflex,    O,           asciicircum
    0x00d6,
    kKeyCompose,
    0x004f,
    0x0022,
    0, // Odiaeresis,     O,           quotedbl
    0x00d2,
    kKeyCompose,
    0x004f,
    0x0060,
    0, // Ograve,         O,           grave
    0x00d8,
    kKeyCompose,
    0x004f,
    0x002f,
    0, // Ooblique,       O,           slash
    0x00d5,
    kKeyCompose,
    0x004f,
    0x007e,
    0, // Otilde,         O,           asciitilde
    0x00de,
    kKeyCompose,
    0x0054,
    0x0048,
    0, // THORN,           T,           H
    0x00da,
    kKeyCompose,
    0x0055,
    0x0027,
    0, // Uacute,         U,           apostrophe
    0x00db,
    kKeyCompose,
    0x0055,
    0x0053,
    0, // Ucircumflex,    U,           asciicircum
    0x00dc,
    kKeyCompose,
    0x0055,
    0x0022,
    0, // Udiaeresis,     U,           quotedbl
    0x00d9,
    kKeyCompose,
    0x0055,
    0x0060,
    0, // Ugrave,         U,           grave
    0x00dd,
    kKeyCompose,
    0x0059,
    0x0027,
    0, // Yacute,         Y,           apostrophe
    0x00e1,
    kKeyCompose,
    0x0061,
    0x0027,
    0, // aacute,         a,           apostrophe
    0x00e2,
    kKeyCompose,
    0x0061,
    0x0053,
    0, // acircumflex,    a,           asciicircum
    0x00b4,
    kKeyCompose,
    0x0027,
    0x0027,
    0, // acute,           apostrophe, apostrophe
    0x00e4,
    kKeyCompose,
    0x0061,
    0x0022,
    0, // adiaeresis,     a,           quotedbl
    0x00e6,
    kKeyCompose,
    0x0061,
    0x0065,
    0, // ae,             a,           e
    0x00e0,
    kKeyCompose,
    0x0061,
    0x0060,
    0, // agrave,         a,           grave
    0x00e5,
    kKeyCompose,
    0x0061,
    0x002a,
    0, // aring,           a,           asterisk
    0x0040,
    kKeyCompose,
    0x0041,
    0x0054,
    0, // at,             A,           T
    0x00e3,
    kKeyCompose,
    0x0061,
    0x007e,
    0, // atilde,         a,           asciitilde
    0x005c,
    kKeyCompose,
    0x002f,
    0x002f,
    0, // backslash,       slash,       slash
    0x007c,
    kKeyCompose,
    0x004c,
    0x0056,
    0, // bar,            L,           V
    0x007b,
    kKeyCompose,
    0x0028,
    0x002d,
    0, // braceleft,       parenleft,  minus
    0x007d,
    kKeyCompose,
    0x0029,
    0x002d,
    0, // braceright,     parenright, minus
    0x005b,
    kKeyCompose,
    0x0028,
    0x0028,
    0, // bracketleft,    parenleft,  parenleft
    0x005d,
    kKeyCompose,
    0x0029,
    0x0029,
    0, // bracketright,   parenright, parenright
    0x00a6,
    kKeyCompose,
    0x0042,
    0x0056,
    0, // brokenbar,       B,           V
    0x00e7,
    kKeyCompose,
    0x0063,
    0x002c,
    0, // ccedilla,       c,           comma
    0x00b8,
    kKeyCompose,
    0x002c,
    0x002c,
    0, // cedilla,        comma,       comma
    0x00a2,
    kKeyCompose,
    0x0063,
    0x002f,
    0, // cent,           c,           slash
    0x00a9,
    kKeyCompose,
    0x0028,
    0x0063,
    0, // copyright,       parenleft,  c
    0x00a4,
    kKeyCompose,
    0x006f,
    0x0078,
    0, // currency,       o,           x
    0x00b0,
    kKeyCompose,
    0x0030,
    0x0053,
    0, // degree,         0,           asciicircum
    0x00a8,
    kKeyCompose,
    0x0022,
    0x0022,
    0, // diaeresis,       quotedbl,   quotedbl
    0x00f7,
    kKeyCompose,
    0x003a,
    0x002d,
    0, // division,       colon,       minus
    0x00e9,
    kKeyCompose,
    0x0065,
    0x0027,
    0, // eacute,         e,           apostrophe
    0x00ea,
    kKeyCompose,
    0x0065,
    0x0053,
    0, // ecircumflex,    e,           asciicircum
    0x00eb,
    kKeyCompose,
    0x0065,
    0x0022,
    0, // ediaeresis,     e,           quotedbl
    0x00e8,
    kKeyCompose,
    0x0065,
    0x0060,
    0, // egrave,         e,           grave
    0x00f0,
    kKeyCompose,
    0x0064,
    0x002d,
    0, // eth,            d,           minus
    0x00a1,
    kKeyCompose,
    0x0021,
    0x0021,
    0, // exclamdown,     exclam,     exclam
    0x00ab,
    kKeyCompose,
    0x003c,
    0x003c,
    0, // guillemotleft,  less,       less
    0x00bb,
    kKeyCompose,
    0x003e,
    0x003e,
    0, // guillemotright, greater,    greater
    0x0023,
    kKeyCompose,
    0x002b,
    0x002b,
    0, // numbersign,     plus,       plus
    0x00ad,
    kKeyCompose,
    0x002d,
    0x002d,
    0, // hyphen,         minus,       minus
    0x00ed,
    kKeyCompose,
    0x0069,
    0x0027,
    0, // iacute,         i,           apostrophe
    0x00ee,
    kKeyCompose,
    0x0069,
    0x0053,
    0, // icircumflex,    i,           asciicircum
    0x00ef,
    kKeyCompose,
    0x0069,
    0x0022,
    0, // idiaeresis,     i,           quotedbl
    0x00ec,
    kKeyCompose,
    0x0069,
    0x0060,
    0, // igrave,         i,           grave
    0x00af,
    kKeyCompose,
    0x002d,
    0x0053,
    0, // macron,         minus,       asciicircum
    0x00ba,
    kKeyCompose,
    0x006f,
    0x005f,
    0, // masculine,       o,           underscore
    0x00b5,
    kKeyCompose,
    0x0075,
    0x002f,
    0, // mu,             u,           slash
    0x00d7,
    kKeyCompose,
    0x0078,
    0x0078,
    0, // multiply,       x,           x
    0x00a0,
    kKeyCompose,
    0x0020,
    0x0020,
    0, // nobreakspace,   space,       space
    0x00ac,
    kKeyCompose,
    0x002c,
    0x002d,
    0, // notsign,        comma,       minus
    0x00f1,
    kKeyCompose,
    0x006e,
    0x007e,
    0, // ntilde,         n,           asciitilde
    0x00f3,
    kKeyCompose,
    0x006f,
    0x0027,
    0, // oacute,         o,           apostrophe
    0x00f4,
    kKeyCompose,
    0x006f,
    0x0053,
    0, // ocircumflex,    o,           asciicircum
    0x00f6,
    kKeyCompose,
    0x006f,
    0x0022,
    0, // odiaeresis,     o,           quotedbl
    0x00f2,
    kKeyCompose,
    0x006f,
    0x0060,
    0, // ograve,         o,           grave
    0x00bd,
    kKeyCompose,
    0x0031,
    0x0032,
    0, // onehalf,        1,           2
    0x00bc,
    kKeyCompose,
    0x0031,
    0x0034,
    0, // onequarter,     1,           4
    0x00b9,
    kKeyCompose,
    0x0031,
    0x0053,
    0, // onesuperior,    1,           asciicircum
    0x00aa,
    kKeyCompose,
    0x0061,
    0x005f,
    0, // ordfeminine,    a,           underscore
    0x00f8,
    kKeyCompose,
    0x006f,
    0x002f,
    0, // oslash,         o,           slash
    0x00f5,
    kKeyCompose,
    0x006f,
    0x007e,
    0, // otilde,         o,           asciitilde
    0x00b6,
    kKeyCompose,
    0x0070,
    0x0021,
    0, // paragraph,       p,           exclam
    0x00b7,
    kKeyCompose,
    0x002e,
    0x002e,
    0, // periodcentered, period,     period
    0x00b1,
    kKeyCompose,
    0x002b,
    0x002d,
    0, // plusminus,       plus,       minus
    0x00bf,
    kKeyCompose,
    0x003f,
    0x003f,
    0, // questiondown,   question,   question
    0x00ae,
    kKeyCompose,
    0x0028,
    0x0072,
    0, // registered,     parenleft,  r
    0x00a7,
    kKeyCompose,
    0x0073,
    0x006f,
    0, // section,        s,           o
    0x00df,
    kKeyCompose,
    0x0073,
    0x0073,
    0, // ssharp,         s,           s
    0x00a3,
    kKeyCompose,
    0x004c,
    0x002d,
    0, // sterling,       L,           minus
    0x00fe,
    kKeyCompose,
    0x0074,
    0x0068,
    0, // thorn,           t,           h
    0x00be,
    kKeyCompose,
    0x0033,
    0x0034,
    0, // threequarters,  3,           4
    0x00b3,
    kKeyCompose,
    0x0033,
    0x0053,
    0, // threesuperior,  3,           asciicircum
    0x00b2,
    kKeyCompose,
    0x0032,
    0x0053,
    0, // twosuperior,    2,           asciicircum
    0x00fa,
    kKeyCompose,
    0x0075,
    0x0027,
    0, // uacute,         u,           apostrophe
    0x00fb,
    kKeyCompose,
    0x0075,
    0x0053,
    0, // ucircumflex,    u,           asciicircum
    0x00fc,
    kKeyCompose,
    0x0075,
    0x0022,
    0, // udiaeresis,     u,           quotedbl
    0x00f9,
    kKeyCompose,
    0x0075,
    0x0060,
    0, // ugrave,         u,           grave
    0x00fd,
    kKeyCompose,
    0x0079,
    0x0027,
    0, // yacute,         y,           apostrophe
    0x00ff,
    kKeyCompose,
    0x0079,
    0x0022,
    0, // ydiaeresis,     y,           quotedbl
    0x00a5,
    kKeyCompose,
    0x0079,
    0x003d,
    0, // yen,            y,           equal

    // end of table
    0};

static const KeyID s_numpadTable[] = {
    kKeyKP_Space,    0x0020,     kKeyKP_Tab,       kKeyTab,
    kKeyKP_Enter,    kKeyReturn, kKeyKP_F1,        kKeyF1,
    kKeyKP_F2,       kKeyF2,     kKeyKP_F3,        kKeyF3,
    kKeyKP_F4,       kKeyF4,     kKeyKP_Home,      kKeyHome,
    kKeyKP_Left,     kKeyLeft,   kKeyKP_Up,        kKeyUp,
    kKeyKP_Right,    kKeyRight,  kKeyKP_Down,      kKeyDown,
    kKeyKP_PageUp,   kKeyPageUp, kKeyKP_PageDown,  kKeyPageDown,
    kKeyKP_End,      kKeyEnd,    kKeyKP_Begin,     kKeyBegin,
    kKeyKP_Insert,   kKeyInsert, kKeyKP_Delete,    kKeyDelete,
    kKeyKP_Equal,    0x003d,     kKeyKP_Multiply,  0x002a,
    kKeyKP_Add,      0x002b,     kKeyKP_Separator, 0x002c,
    kKeyKP_Subtract, 0x002d,     kKeyKP_Decimal,   0x002e,
    kKeyKP_Divide,   0x002f,     kKeyKP_0,         0x0030,
    kKeyKP_1,        0x0031,     kKeyKP_2,         0x0032,
    kKeyKP_3,        0x0033,     kKeyKP_4,         0x0034,
    kKeyKP_5,        0x0035,     kKeyKP_6,         0x0036,
    kKeyKP_7,        0x0037,     kKeyKP_8,         0x0038,
    kKeyKP_9,        0x0039};

//
// KeyState
//

KeyState::KeyState (IEventQueue* events)
    : IKeyState (events),
      m_keyMapPtr (new synergy::KeyMap ()),
      m_keyMap (*m_keyMapPtr),
      m_mask (0),
      m_events (events) {
    init ();
}

KeyState::KeyState (IEventQueue* events, synergy::KeyMap& keyMap)
    : IKeyState (events),
      m_keyMapPtr (0),
      m_keyMap (keyMap),
      m_mask (0),
      m_events (events) {
    init ();
}

KeyState::~KeyState () {
    if (m_keyMapPtr)
        delete m_keyMapPtr;
}

void
KeyState::init () {
    memset (&m_keys, 0, sizeof (m_keys));
    memset (&m_syntheticKeys, 0, sizeof (m_syntheticKeys));
    memset (&m_keyClientData, 0, sizeof (m_keyClientData));
    memset (&m_serverKeys, 0, sizeof (m_serverKeys));
}

void
KeyState::onKey (KeyButton button, bool down, KeyModifierMask newState) {
    // update modifier state
    m_mask = newState;
    LOG ((CLOG_DEBUG1 "new mask: 0x%04x", m_mask));

    // ignore bogus buttons
    button &= kButtonMask;
    if (button == 0) {
        return;
    }

    // update key state
    if (down) {
        m_keys[button]          = 1;
        m_syntheticKeys[button] = 1;
    } else {
        m_keys[button]          = 0;
        m_syntheticKeys[button] = 0;
    }
}

void
KeyState::sendKeyEvent (void* target, bool press, bool isAutoRepeat, KeyID key,
                        KeyModifierMask mask, SInt32 count, KeyButton button) {
    if (m_keyMap.isHalfDuplex (key, button)) {
        if (isAutoRepeat) {
            // ignore auto-repeat on half-duplex keys
        } else {
            m_events->addEvent (Event (m_events->forIKeyState ().keyDown (),
                                       target,
                                       KeyInfo::alloc (key, mask, button, 1)));
            m_events->addEvent (Event (m_events->forIKeyState ().keyUp (),
                                       target,
                                       KeyInfo::alloc (key, mask, button, 1)));
        }
    } else {
        if (isAutoRepeat) {
            m_events->addEvent (
                Event (m_events->forIKeyState ().keyRepeat (),
                       target,
                       KeyInfo::alloc (key, mask, button, count)));
        } else if (press) {
            m_events->addEvent (Event (m_events->forIKeyState ().keyDown (),
                                       target,
                                       KeyInfo::alloc (key, mask, button, 1)));
        } else {
            m_events->addEvent (Event (m_events->forIKeyState ().keyUp (),
                                       target,
                                       KeyInfo::alloc (key, mask, button, 1)));
        }
    }
}

void
KeyState::updateKeyMap () {
    // get the current keyboard map
    synergy::KeyMap keyMap;
    getKeyMap (keyMap);
    m_keyMap.swap (keyMap);
    m_keyMap.finish ();

    // add special keys
    addCombinationEntries ();
    addKeypadEntries ();
    addAliasEntries ();
}

void
KeyState::updateKeyState () {
    // reset our state
    memset (&m_keys, 0, sizeof (m_keys));
    memset (&m_syntheticKeys, 0, sizeof (m_syntheticKeys));
    memset (&m_keyClientData, 0, sizeof (m_keyClientData));
    memset (&m_serverKeys, 0, sizeof (m_serverKeys));
    m_activeModifiers.clear ();

    // get the current keyboard state
    KeyButtonSet keysDown;
    pollPressedKeys (keysDown);
    for (KeyButtonSet::const_iterator i = keysDown.begin ();
         i != keysDown.end ();
         ++i) {
        m_keys[*i] = 1;
    }

    // get the current modifier state
    m_mask = pollActiveModifiers ();

    // set active modifiers
    AddActiveModifierContext addModifierContext (
        pollActiveGroup (), m_mask, m_activeModifiers);
    m_keyMap.foreachKey (&KeyState::addActiveModifierCB, &addModifierContext);

    LOG ((CLOG_DEBUG1 "modifiers on update: 0x%04x", m_mask));
}

void
KeyState::addActiveModifierCB (KeyID, SInt32 group,
                               synergy::KeyMap::KeyItem& keyItem,
                               void* vcontext) {
    AddActiveModifierContext* context =
        static_cast<AddActiveModifierContext*> (vcontext);
    if (group == context->m_activeGroup &&
        (keyItem.m_generates & context->m_mask) != 0) {
        context->m_activeModifiers.insert (
            std::make_pair (keyItem.m_generates, keyItem));
    }
}

void
KeyState::setHalfDuplexMask (KeyModifierMask mask) {
    m_keyMap.clearHalfDuplexModifiers ();
    if ((mask & KeyModifierCapsLock) != 0) {
        m_keyMap.addHalfDuplexModifier (kKeyCapsLock);
    }
    if ((mask & KeyModifierNumLock) != 0) {
        m_keyMap.addHalfDuplexModifier (kKeyNumLock);
    }
    if ((mask & KeyModifierScrollLock) != 0) {
        m_keyMap.addHalfDuplexModifier (kKeyScrollLock);
    }
}

void
KeyState::fakeKeyDown (KeyID id, KeyModifierMask mask, KeyButton serverID) {
    // if this server key is already down then this is probably a
    // mis-reported autorepeat.
    serverID &= kButtonMask;
    if (m_serverKeys[serverID] != 0) {
        fakeKeyRepeat (id, mask, 1, serverID);
        return;
    }

    // ignore certain keys
    if (isIgnoredKey (id, mask)) {
        LOG ((CLOG_DEBUG1 "ignored key %04x %04x", id, mask));
        return;
    }

    // get keys for key press
    Keystrokes keys;
    ModifierToKeys oldActiveModifiers = m_activeModifiers;
    const synergy::KeyMap::KeyItem* keyItem =
        m_keyMap.mapKey (keys,
                         id,
                         pollActiveGroup (),
                         m_activeModifiers,
                         getActiveModifiersRValue (),
                         mask,
                         false);
    if (keyItem == NULL) {
        // a media key won't be mapped on mac, so we need to fake it in a
        // special way
        if (id == kKeyAudioDown || id == kKeyAudioUp || id == kKeyAudioMute ||
            id == kKeyAudioPlay || id == kKeyAudioPrev || id == kKeyAudioNext ||
            id == kKeyBrightnessDown || id == kKeyBrightnessUp) {
            LOG ((CLOG_DEBUG1 "emulating media key"));
            fakeMediaKey (id);
        }

        return;
    }

    KeyButton localID = (KeyButton) (keyItem->m_button & kButtonMask);
    updateModifierKeyState (localID, oldActiveModifiers, m_activeModifiers);
    if (localID != 0) {
        // note keys down
        ++m_keys[localID];
        ++m_syntheticKeys[localID];
        m_keyClientData[localID] = keyItem->m_client;
        m_serverKeys[serverID]   = localID;
    }

    // generate key events
    fakeKeys (keys, 1);
}

bool
KeyState::fakeKeyRepeat (KeyID id, KeyModifierMask mask, SInt32 count,
                         KeyButton serverID) {
    serverID &= kButtonMask;

    // if we haven't seen this button go down then ignore it
    KeyButton oldLocalID = m_serverKeys[serverID];
    if (oldLocalID == 0) {
        return false;
    }

    // get keys for key repeat
    Keystrokes keys;
    ModifierToKeys oldActiveModifiers = m_activeModifiers;
    const synergy::KeyMap::KeyItem* keyItem =
        m_keyMap.mapKey (keys,
                         id,
                         pollActiveGroup (),
                         m_activeModifiers,
                         getActiveModifiersRValue (),
                         mask,
                         true);
    if (keyItem == NULL) {
        return false;
    }
    KeyButton localID = (KeyButton) (keyItem->m_button & kButtonMask);
    if (localID == 0) {
        return false;
    }

    // if the KeyButton for the auto-repeat is not the same as for the
    // initial press then mark the initial key as released and the new
    // key as pressed.  this can happen when we auto-repeat after a
    // dead key.  for example, a dead accent followed by 'a' will
    // generate an 'a with accent' followed by a repeating 'a'.  the
    // KeyButtons for the two KeyIDs might be different.
    if (localID != oldLocalID) {
        // replace key up with previous KeyButton but leave key down
        // alone so it uses the new KeyButton.
        for (Keystrokes::iterator index = keys.begin (); index != keys.end ();
             ++index) {
            if (index->m_type == Keystroke::kButton &&
                index->m_data.m_button.m_button == localID) {
                index->m_data.m_button.m_button = oldLocalID;
                break;
            }
        }

        // note that old key is now up
        --m_keys[oldLocalID];
        --m_syntheticKeys[oldLocalID];

        // note keys down
        updateModifierKeyState (localID, oldActiveModifiers, m_activeModifiers);
        ++m_keys[localID];
        ++m_syntheticKeys[localID];
        m_keyClientData[localID] = keyItem->m_client;
        m_serverKeys[serverID]   = localID;
    }

    // generate key events
    fakeKeys (keys, count);
    return true;
}

bool
KeyState::fakeKeyUp (KeyButton serverID) {
    // if we haven't seen this button go down then ignore it
    KeyButton localID = m_serverKeys[serverID & kButtonMask];
    if (localID == 0) {
        return false;
    }

    // get the sequence of keys to simulate key release
    Keystrokes keys;
    keys.push_back (
        Keystroke (localID, false, false, m_keyClientData[localID]));

    // note keys down
    --m_keys[localID];
    --m_syntheticKeys[localID];
    m_serverKeys[serverID] = 0;

    // check if this is a modifier
    ModifierToKeys::iterator i = m_activeModifiers.begin ();
    while (i != m_activeModifiers.end ()) {
        if (i->second.m_button == localID && !i->second.m_lock) {
            // modifier is no longer down
            KeyModifierMask mask = i->first;

            ModifierToKeys::iterator tmp = i;
            ++i;
            m_activeModifiers.erase (tmp);

            if (m_activeModifiers.count (mask) == 0) {
                // no key for modifier is down so deactivate modifier
                m_mask &= ~mask;
                LOG ((CLOG_DEBUG1 "new state %04x", m_mask));
            }
        } else {
            ++i;
        }
    }

    // generate key events
    fakeKeys (keys, 1);
    return true;
}

void
KeyState::fakeAllKeysUp () {
    Keystrokes keys;
    for (KeyButton i = 0; i < IKeyState::kNumButtons; ++i) {
        if (m_syntheticKeys[i] > 0) {
            keys.push_back (Keystroke (i, false, false, m_keyClientData[i]));
            m_keys[i]          = 0;
            m_syntheticKeys[i] = 0;
        }
    }
    fakeKeys (keys, 1);
    memset (&m_serverKeys, 0, sizeof (m_serverKeys));
    m_activeModifiers.clear ();
    m_mask = pollActiveModifiers ();
}

bool
KeyState::fakeMediaKey (KeyID id) {
    return false;
}

bool
KeyState::isKeyDown (KeyButton button) const {
    return (m_keys[button & kButtonMask] > 0);
}

KeyModifierMask
KeyState::getActiveModifiers () const {
    return m_mask;
}

KeyModifierMask&
KeyState::getActiveModifiersRValue () {
    return m_mask;
}

SInt32
KeyState::getEffectiveGroup (SInt32 group, SInt32 offset) const {
    return m_keyMap.getEffectiveGroup (group, offset);
}

bool
KeyState::isIgnoredKey (KeyID key, KeyModifierMask) const {
    switch (key) {
        case kKeyCapsLock:
        case kKeyNumLock:
        case kKeyScrollLock:
            return true;

        default:
            return false;
    }
}

KeyButton
KeyState::getButton (KeyID id, SInt32 group) const {
    const synergy::KeyMap::KeyItemList* items =
        m_keyMap.findCompatibleKey (id, group, 0, 0);
    if (items == NULL) {
        return 0;
    } else {
        return items->back ().m_button;
    }
}

void
KeyState::addAliasEntries () {
    for (SInt32 g = 0, n = m_keyMap.getNumGroups (); g < n; ++g) {
        // if we can't shift any kKeyTab key in a particular group but we can
        // shift kKeyLeftTab then add a shifted kKeyTab entry that matches a
        // shifted kKeyLeftTab entry.
        m_keyMap.addKeyAliasEntry (kKeyTab,
                                   g,
                                   KeyModifierShift,
                                   KeyModifierShift,
                                   kKeyLeftTab,
                                   KeyModifierShift,
                                   KeyModifierShift);

        // if we have no kKeyLeftTab but we do have a kKeyTab that can be
        // shifted then add kKeyLeftTab that matches a kKeyTab.
        m_keyMap.addKeyAliasEntry (kKeyLeftTab,
                                   g,
                                   KeyModifierShift,
                                   KeyModifierShift,
                                   kKeyTab,
                                   0,
                                   KeyModifierShift);

        // map non-breaking space to space
        m_keyMap.addKeyAliasEntry (0x20, g, 0, 0, 0xa0, 0, 0);
    }
}

void
KeyState::addKeypadEntries () {
    // map every numpad key to its equivalent non-numpad key if it's not
    // on the keyboard.
    for (SInt32 g = 0, n = m_keyMap.getNumGroups (); g < n; ++g) {
        for (size_t i = 0;
             i < sizeof (s_numpadTable) / sizeof (s_numpadTable[0]);
             i += 2) {
            m_keyMap.addKeyCombinationEntry (
                s_numpadTable[i], g, s_numpadTable + i + 1, 1);
        }
    }
}

void
KeyState::addCombinationEntries () {
    for (SInt32 g = 0, n = m_keyMap.getNumGroups (); g < n; ++g) {
        // add dead and compose key composition sequences
        for (const KeyID* i = s_decomposeTable; *i != 0; ++i) {
            // count the decomposed keys for this key
            UInt32 numKeys = 0;
            for (const KeyID* j = i; *++j != 0;) {
                ++numKeys;
            }

            // add an entry for this key
            m_keyMap.addKeyCombinationEntry (*i, g, i + 1, numKeys);

            // next key
            i += numKeys + 1;
        }
    }
}

void
KeyState::fakeKeys (const Keystrokes& keys, UInt32 count) {
    // do nothing if no keys or no repeats
    if (count == 0 || keys.empty ()) {
        return;
    }

    // generate key events
    LOG ((CLOG_DEBUG1 "keystrokes:"));
    for (Keystrokes::const_iterator k = keys.begin (); k != keys.end ();) {
        if (k->m_type == Keystroke::kButton && k->m_data.m_button.m_repeat) {
            // repeat from here up to but not including the next key
            // with m_repeat == false count times.
            Keystrokes::const_iterator start = k;
            while (count-- > 0) {
                // send repeating events
                for (k = start;
                     k != keys.end () && k->m_type == Keystroke::kButton &&
                     k->m_data.m_button.m_repeat;
                     ++k) {
                    fakeKey (*k);
                }
            }

            // note -- k is now on the first non-repeat key after the
            // repeat keys, exactly where we'd like to continue from.
        } else {
            // send event
            fakeKey (*k);

            // next key
            ++k;
        }
    }
}

void
KeyState::updateModifierKeyState (KeyButton button,
                                  const ModifierToKeys& oldModifiers,
                                  const ModifierToKeys& newModifiers) {
    // get the pressed modifier buttons before and after
    synergy::KeyMap::ButtonToKeyMap oldKeys, newKeys;
    for (ModifierToKeys::const_iterator i = oldModifiers.begin ();
         i != oldModifiers.end ();
         ++i) {
        oldKeys.insert (std::make_pair (i->second.m_button, &i->second));
    }
    for (ModifierToKeys::const_iterator i = newModifiers.begin ();
         i != newModifiers.end ();
         ++i) {
        newKeys.insert (std::make_pair (i->second.m_button, &i->second));
    }

    // get the modifier buttons that were pressed or released
    synergy::KeyMap::ButtonToKeyMap pressed, released;
    std::set_difference (oldKeys.begin (),
                         oldKeys.end (),
                         newKeys.begin (),
                         newKeys.end (),
                         std::inserter (released, released.end ()),
                         ButtonToKeyLess ());
    std::set_difference (newKeys.begin (),
                         newKeys.end (),
                         oldKeys.begin (),
                         oldKeys.end (),
                         std::inserter (pressed, pressed.end ()),
                         ButtonToKeyLess ());

    // update state
    for (synergy::KeyMap::ButtonToKeyMap::const_iterator i = released.begin ();
         i != released.end ();
         ++i) {
        if (i->first != button) {
            m_keys[i->first]          = 0;
            m_syntheticKeys[i->first] = 0;
        }
    }
    for (synergy::KeyMap::ButtonToKeyMap::const_iterator i = pressed.begin ();
         i != pressed.end ();
         ++i) {
        if (i->first != button) {
            m_keys[i->first]          = 1;
            m_syntheticKeys[i->first] = 1;
            m_keyClientData[i->first] = i->second->m_client;
        }
    }
}

//
// KeyState::AddActiveModifierContext
//

KeyState::AddActiveModifierContext::AddActiveModifierContext (
    SInt32 group, KeyModifierMask mask, ModifierToKeys& activeModifiers)
    : m_activeGroup (group),
      m_mask (mask),
      m_activeModifiers (activeModifiers) {
    // do nothing
}
