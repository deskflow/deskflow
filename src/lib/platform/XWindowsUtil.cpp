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

#include "platform/XWindowsUtil.h"

#include "synergy/key_types.h"
#include "mt/Thread.h"
#include "base/Log.h"
#include "base/String.h"

#include <X11/Xatom.h>
#define XK_APL
#define XK_ARABIC
#define XK_ARMENIAN
#define XK_CAUCASUS
#define XK_CURRENCY
#define XK_CYRILLIC
#define XK_GEORGIAN
#define XK_GREEK
#define XK_HEBREW
#define XK_KATAKANA
#define XK_KOREAN
#define XK_LATIN1
#define XK_LATIN2
#define XK_LATIN3
#define XK_LATIN4
#define XK_LATIN8
#define XK_LATIN9
#define XK_MISCELLANY
#define XK_PUBLISHING
#define XK_SPECIAL
#define XK_TECHNICAL
#define XK_THAI
#define XK_VIETNAMESE
#define XK_XKB_KEYS
#include <X11/keysym.h>

#if !defined(XK_OE)
#define XK_OE 0x13bc
#endif
#if !defined(XK_oe)
#define XK_oe 0x13bd
#endif
#if !defined(XK_Ydiaeresis)
#define XK_Ydiaeresis 0x13be
#endif

/*
 * This table maps keysym values into the corresponding ISO 10646
 * (UCS, Unicode) values.
 *
 * The array keysymtab[] contains pairs of X11 keysym values for graphical
 * characters and the corresponding Unicode value.
 *
 * Author: Markus G. Kuhn <http://www.cl.cam.ac.uk/~mgk25/>,
 *         University of Cambridge, April 2001
 *
 * Special thanks to Richard Verhoeven <river@win.tue.nl> for preparing
 * an initial draft of the mapping table.
 *
 * This software is in the public domain. Share and enjoy!
 */

struct codepair {
    KeySym keysym;
    UInt32 ucs4;
} s_keymap[] = {
    {XK_Aogonek, 0x0104},      /* LATIN CAPITAL LETTER A WITH OGONEK */
    {XK_breve, 0x02d8},        /* BREVE */
    {XK_Lstroke, 0x0141},      /* LATIN CAPITAL LETTER L WITH STROKE */
    {XK_Lcaron, 0x013d},       /* LATIN CAPITAL LETTER L WITH CARON */
    {XK_Sacute, 0x015a},       /* LATIN CAPITAL LETTER S WITH ACUTE */
    {XK_Scaron, 0x0160},       /* LATIN CAPITAL LETTER S WITH CARON */
    {XK_Scedilla, 0x015e},     /* LATIN CAPITAL LETTER S WITH CEDILLA */
    {XK_Tcaron, 0x0164},       /* LATIN CAPITAL LETTER T WITH CARON */
    {XK_Zacute, 0x0179},       /* LATIN CAPITAL LETTER Z WITH ACUTE */
    {XK_Zcaron, 0x017d},       /* LATIN CAPITAL LETTER Z WITH CARON */
    {XK_Zabovedot, 0x017b},    /* LATIN CAPITAL LETTER Z WITH DOT ABOVE */
    {XK_aogonek, 0x0105},      /* LATIN SMALL LETTER A WITH OGONEK */
    {XK_ogonek, 0x02db},       /* OGONEK */
    {XK_lstroke, 0x0142},      /* LATIN SMALL LETTER L WITH STROKE */
    {XK_lcaron, 0x013e},       /* LATIN SMALL LETTER L WITH CARON */
    {XK_sacute, 0x015b},       /* LATIN SMALL LETTER S WITH ACUTE */
    {XK_caron, 0x02c7},        /* CARON */
    {XK_scaron, 0x0161},       /* LATIN SMALL LETTER S WITH CARON */
    {XK_scedilla, 0x015f},     /* LATIN SMALL LETTER S WITH CEDILLA */
    {XK_tcaron, 0x0165},       /* LATIN SMALL LETTER T WITH CARON */
    {XK_zacute, 0x017a},       /* LATIN SMALL LETTER Z WITH ACUTE */
    {XK_doubleacute, 0x02dd},  /* DOUBLE ACUTE ACCENT */
    {XK_zcaron, 0x017e},       /* LATIN SMALL LETTER Z WITH CARON */
    {XK_zabovedot, 0x017c},    /* LATIN SMALL LETTER Z WITH DOT ABOVE */
    {XK_Racute, 0x0154},       /* LATIN CAPITAL LETTER R WITH ACUTE */
    {XK_Abreve, 0x0102},       /* LATIN CAPITAL LETTER A WITH BREVE */
    {XK_Lacute, 0x0139},       /* LATIN CAPITAL LETTER L WITH ACUTE */
    {XK_Cacute, 0x0106},       /* LATIN CAPITAL LETTER C WITH ACUTE */
    {XK_Ccaron, 0x010c},       /* LATIN CAPITAL LETTER C WITH CARON */
    {XK_Eogonek, 0x0118},      /* LATIN CAPITAL LETTER E WITH OGONEK */
    {XK_Ecaron, 0x011a},       /* LATIN CAPITAL LETTER E WITH CARON */
    {XK_Dcaron, 0x010e},       /* LATIN CAPITAL LETTER D WITH CARON */
    {XK_Dstroke, 0x0110},      /* LATIN CAPITAL LETTER D WITH STROKE */
    {XK_Nacute, 0x0143},       /* LATIN CAPITAL LETTER N WITH ACUTE */
    {XK_Ncaron, 0x0147},       /* LATIN CAPITAL LETTER N WITH CARON */
    {XK_Odoubleacute, 0x0150}, /* LATIN CAPITAL LETTER O WITH DOUBLE ACUTE */
    {XK_Rcaron, 0x0158},       /* LATIN CAPITAL LETTER R WITH CARON */
    {XK_Uring, 0x016e},        /* LATIN CAPITAL LETTER U WITH RING ABOVE */
    {XK_Udoubleacute, 0x0170}, /* LATIN CAPITAL LETTER U WITH DOUBLE ACUTE */
    {XK_Tcedilla, 0x0162},     /* LATIN CAPITAL LETTER T WITH CEDILLA */
    {XK_racute, 0x0155},       /* LATIN SMALL LETTER R WITH ACUTE */
    {XK_abreve, 0x0103},       /* LATIN SMALL LETTER A WITH BREVE */
    {XK_lacute, 0x013a},       /* LATIN SMALL LETTER L WITH ACUTE */
    {XK_cacute, 0x0107},       /* LATIN SMALL LETTER C WITH ACUTE */
    {XK_ccaron, 0x010d},       /* LATIN SMALL LETTER C WITH CARON */
    {XK_eogonek, 0x0119},      /* LATIN SMALL LETTER E WITH OGONEK */
    {XK_ecaron, 0x011b},       /* LATIN SMALL LETTER E WITH CARON */
    {XK_dcaron, 0x010f},       /* LATIN SMALL LETTER D WITH CARON */
    {XK_dstroke, 0x0111},      /* LATIN SMALL LETTER D WITH STROKE */
    {XK_nacute, 0x0144},       /* LATIN SMALL LETTER N WITH ACUTE */
    {XK_ncaron, 0x0148},       /* LATIN SMALL LETTER N WITH CARON */
    {XK_odoubleacute, 0x0151}, /* LATIN SMALL LETTER O WITH DOUBLE ACUTE */
    {XK_rcaron, 0x0159},       /* LATIN SMALL LETTER R WITH CARON */
    {XK_uring, 0x016f},        /* LATIN SMALL LETTER U WITH RING ABOVE */
    {XK_udoubleacute, 0x0171}, /* LATIN SMALL LETTER U WITH DOUBLE ACUTE */
    {XK_tcedilla, 0x0163},     /* LATIN SMALL LETTER T WITH CEDILLA */
    {XK_abovedot, 0x02d9},     /* DOT ABOVE */
    {XK_Hstroke, 0x0126},      /* LATIN CAPITAL LETTER H WITH STROKE */
    {XK_Hcircumflex, 0x0124},  /* LATIN CAPITAL LETTER H WITH CIRCUMFLEX */
    {XK_Iabovedot, 0x0130},    /* LATIN CAPITAL LETTER I WITH DOT ABOVE */
    {XK_Gbreve, 0x011e},       /* LATIN CAPITAL LETTER G WITH BREVE */
    {XK_Jcircumflex, 0x0134},  /* LATIN CAPITAL LETTER J WITH CIRCUMFLEX */
    {XK_hstroke, 0x0127},      /* LATIN SMALL LETTER H WITH STROKE */
    {XK_hcircumflex, 0x0125},  /* LATIN SMALL LETTER H WITH CIRCUMFLEX */
    {XK_idotless, 0x0131},     /* LATIN SMALL LETTER DOTLESS I */
    {XK_gbreve, 0x011f},       /* LATIN SMALL LETTER G WITH BREVE */
    {XK_jcircumflex, 0x0135},  /* LATIN SMALL LETTER J WITH CIRCUMFLEX */
    {XK_Cabovedot, 0x010a},    /* LATIN CAPITAL LETTER C WITH DOT ABOVE */
    {XK_Ccircumflex, 0x0108},  /* LATIN CAPITAL LETTER C WITH CIRCUMFLEX */
    {XK_Gabovedot, 0x0120},    /* LATIN CAPITAL LETTER G WITH DOT ABOVE */
    {XK_Gcircumflex, 0x011c},  /* LATIN CAPITAL LETTER G WITH CIRCUMFLEX */
    {XK_Ubreve, 0x016c},       /* LATIN CAPITAL LETTER U WITH BREVE */
    {XK_Scircumflex, 0x015c},  /* LATIN CAPITAL LETTER S WITH CIRCUMFLEX */
    {XK_cabovedot, 0x010b},    /* LATIN SMALL LETTER C WITH DOT ABOVE */
    {XK_ccircumflex, 0x0109},  /* LATIN SMALL LETTER C WITH CIRCUMFLEX */
    {XK_gabovedot, 0x0121},    /* LATIN SMALL LETTER G WITH DOT ABOVE */
    {XK_gcircumflex, 0x011d},  /* LATIN SMALL LETTER G WITH CIRCUMFLEX */
    {XK_ubreve, 0x016d},       /* LATIN SMALL LETTER U WITH BREVE */
    {XK_scircumflex, 0x015d},  /* LATIN SMALL LETTER S WITH CIRCUMFLEX */
    {XK_kra, 0x0138},          /* LATIN SMALL LETTER KRA */
    {XK_Rcedilla, 0x0156},     /* LATIN CAPITAL LETTER R WITH CEDILLA */
    {XK_Itilde, 0x0128},       /* LATIN CAPITAL LETTER I WITH TILDE */
    {XK_Lcedilla, 0x013b},     /* LATIN CAPITAL LETTER L WITH CEDILLA */
    {XK_Emacron, 0x0112},      /* LATIN CAPITAL LETTER E WITH MACRON */
    {XK_Gcedilla, 0x0122},     /* LATIN CAPITAL LETTER G WITH CEDILLA */
    {XK_Tslash, 0x0166},       /* LATIN CAPITAL LETTER T WITH STROKE */
    {XK_rcedilla, 0x0157},     /* LATIN SMALL LETTER R WITH CEDILLA */
    {XK_itilde, 0x0129},       /* LATIN SMALL LETTER I WITH TILDE */
    {XK_lcedilla, 0x013c},     /* LATIN SMALL LETTER L WITH CEDILLA */
    {XK_emacron, 0x0113},      /* LATIN SMALL LETTER E WITH MACRON */
    {XK_gcedilla, 0x0123},     /* LATIN SMALL LETTER G WITH CEDILLA */
    {XK_tslash, 0x0167},       /* LATIN SMALL LETTER T WITH STROKE */
    {XK_ENG, 0x014a},          /* LATIN CAPITAL LETTER ENG */
    {XK_eng, 0x014b},          /* LATIN SMALL LETTER ENG */
    {XK_Amacron, 0x0100},      /* LATIN CAPITAL LETTER A WITH MACRON */
    {XK_Iogonek, 0x012e},      /* LATIN CAPITAL LETTER I WITH OGONEK */
    {XK_Eabovedot, 0x0116},    /* LATIN CAPITAL LETTER E WITH DOT ABOVE */
    {XK_Imacron, 0x012a},      /* LATIN CAPITAL LETTER I WITH MACRON */
    {XK_Ncedilla, 0x0145},     /* LATIN CAPITAL LETTER N WITH CEDILLA */
    {XK_Omacron, 0x014c},      /* LATIN CAPITAL LETTER O WITH MACRON */
    {XK_Kcedilla, 0x0136},     /* LATIN CAPITAL LETTER K WITH CEDILLA */
    {XK_Uogonek, 0x0172},      /* LATIN CAPITAL LETTER U WITH OGONEK */
    {XK_Utilde, 0x0168},       /* LATIN CAPITAL LETTER U WITH TILDE */
    {XK_Umacron, 0x016a},      /* LATIN CAPITAL LETTER U WITH MACRON */
    {XK_amacron, 0x0101},      /* LATIN SMALL LETTER A WITH MACRON */
    {XK_iogonek, 0x012f},      /* LATIN SMALL LETTER I WITH OGONEK */
    {XK_eabovedot, 0x0117},    /* LATIN SMALL LETTER E WITH DOT ABOVE */
    {XK_imacron, 0x012b},      /* LATIN SMALL LETTER I WITH MACRON */
    {XK_ncedilla, 0x0146},     /* LATIN SMALL LETTER N WITH CEDILLA */
    {XK_omacron, 0x014d},      /* LATIN SMALL LETTER O WITH MACRON */
    {XK_kcedilla, 0x0137},     /* LATIN SMALL LETTER K WITH CEDILLA */
    {XK_uogonek, 0x0173},      /* LATIN SMALL LETTER U WITH OGONEK */
    {XK_utilde, 0x0169},       /* LATIN SMALL LETTER U WITH TILDE */
    {XK_umacron, 0x016b},      /* LATIN SMALL LETTER U WITH MACRON */
#if defined(XK_Babovedot)
    {XK_Babovedot, 0x1e02},   /* LATIN CAPITAL LETTER B WITH DOT ABOVE */
    {XK_babovedot, 0x1e03},   /* LATIN SMALL LETTER B WITH DOT ABOVE */
    {XK_Dabovedot, 0x1e0a},   /* LATIN CAPITAL LETTER D WITH DOT ABOVE */
    {XK_Wgrave, 0x1e80},      /* LATIN CAPITAL LETTER W WITH GRAVE */
    {XK_Wacute, 0x1e82},      /* LATIN CAPITAL LETTER W WITH ACUTE */
    {XK_dabovedot, 0x1e0b},   /* LATIN SMALL LETTER D WITH DOT ABOVE */
    {XK_Ygrave, 0x1ef2},      /* LATIN CAPITAL LETTER Y WITH GRAVE  */
    {XK_Fabovedot, 0x1e1e},   /* LATIN CAPITAL LETTER F WITH DOT ABOVE */
    {XK_fabovedot, 0x1e1f},   /* LATIN SMALL LETTER F WITH DOT ABOVE */
    {XK_Mabovedot, 0x1e40},   /* LATIN CAPITAL LETTER M WITH DOT ABOVE */
    {XK_mabovedot, 0x1e41},   /* LATIN SMALL LETTER M WITH DOT ABOVE */
    {XK_Pabovedot, 0x1e56},   /* LATIN CAPITAL LETTER P WITH DOT ABOVE */
    {XK_wgrave, 0x1e81},      /* LATIN SMALL LETTER W WITH GRAVE  */
    {XK_pabovedot, 0x1e57},   /* LATIN SMALL LETTER P WITH DOT ABOVE */
    {XK_wacute, 0x1e83},      /* LATIN SMALL LETTER W WITH ACUTE  */
    {XK_Sabovedot, 0x1e60},   /* LATIN CAPITAL LETTER S WITH DOT ABOVE */
    {XK_ygrave, 0x1ef3},      /* LATIN SMALL LETTER Y WITH GRAVE  */
    {XK_Wdiaeresis, 0x1e84},  /* LATIN CAPITAL LETTER W WITH DIAERESIS */
    {XK_wdiaeresis, 0x1e85},  /* LATIN SMALL LETTER W WITH DIAERESIS */
    {XK_sabovedot, 0x1e61},   /* LATIN SMALL LETTER S WITH DOT ABOVE */
    {XK_Wcircumflex, 0x0174}, /* LATIN CAPITAL LETTER W WITH CIRCUMFLEX */
    {XK_Tabovedot, 0x1e6a},   /* LATIN CAPITAL LETTER T WITH DOT ABOVE */
    {XK_Ycircumflex, 0x0176}, /* LATIN CAPITAL LETTER Y WITH CIRCUMFLEX */
    {XK_wcircumflex, 0x0175}, /* LATIN SMALL LETTER W WITH CIRCUMFLEX */
    {XK_tabovedot, 0x1e6b},   /* LATIN SMALL LETTER T WITH DOT ABOVE */
    {XK_ycircumflex, 0x0177}, /* LATIN SMALL LETTER Y WITH CIRCUMFLEX */
#endif                        // defined(XK_Babovedot)
#if defined(XK_overline)
    {XK_overline, 0x203e},            /* OVERLINE */
    {XK_kana_fullstop, 0x3002},       /* IDEOGRAPHIC FULL STOP */
    {XK_kana_openingbracket, 0x300c}, /* LEFT CORNER BRACKET */
    {XK_kana_closingbracket, 0x300d}, /* RIGHT CORNER BRACKET */
    {XK_kana_comma, 0x3001},          /* IDEOGRAPHIC COMMA */
    {XK_kana_conjunctive, 0x30fb},    /* KATAKANA MIDDLE DOT */
    {XK_kana_WO, 0x30f2},             /* KATAKANA LETTER WO */
    {XK_kana_a, 0x30a1},              /* KATAKANA LETTER SMALL A */
    {XK_kana_i, 0x30a3},              /* KATAKANA LETTER SMALL I */
    {XK_kana_u, 0x30a5},              /* KATAKANA LETTER SMALL U */
    {XK_kana_e, 0x30a7},              /* KATAKANA LETTER SMALL E */
    {XK_kana_o, 0x30a9},              /* KATAKANA LETTER SMALL O */
    {XK_kana_ya, 0x30e3},             /* KATAKANA LETTER SMALL YA */
    {XK_kana_yu, 0x30e5},             /* KATAKANA LETTER SMALL YU */
    {XK_kana_yo, 0x30e7},             /* KATAKANA LETTER SMALL YO */
    {XK_kana_tsu, 0x30c3},            /* KATAKANA LETTER SMALL TU */
    {XK_prolongedsound, 0x30fc},  /* KATAKANA-HIRAGANA PROLONGED SOUND MARK */
    {XK_kana_A, 0x30a2},          /* KATAKANA LETTER A */
    {XK_kana_I, 0x30a4},          /* KATAKANA LETTER I */
    {XK_kana_U, 0x30a6},          /* KATAKANA LETTER U */
    {XK_kana_E, 0x30a8},          /* KATAKANA LETTER E */
    {XK_kana_O, 0x30aa},          /* KATAKANA LETTER O */
    {XK_kana_KA, 0x30ab},         /* KATAKANA LETTER KA */
    {XK_kana_KI, 0x30ad},         /* KATAKANA LETTER KI */
    {XK_kana_KU, 0x30af},         /* KATAKANA LETTER KU */
    {XK_kana_KE, 0x30b1},         /* KATAKANA LETTER KE */
    {XK_kana_KO, 0x30b3},         /* KATAKANA LETTER KO */
    {XK_kana_SA, 0x30b5},         /* KATAKANA LETTER SA */
    {XK_kana_SHI, 0x30b7},        /* KATAKANA LETTER SI */
    {XK_kana_SU, 0x30b9},         /* KATAKANA LETTER SU */
    {XK_kana_SE, 0x30bb},         /* KATAKANA LETTER SE */
    {XK_kana_SO, 0x30bd},         /* KATAKANA LETTER SO */
    {XK_kana_TA, 0x30bf},         /* KATAKANA LETTER TA */
    {XK_kana_CHI, 0x30c1},        /* KATAKANA LETTER TI */
    {XK_kana_TSU, 0x30c4},        /* KATAKANA LETTER TU */
    {XK_kana_TE, 0x30c6},         /* KATAKANA LETTER TE */
    {XK_kana_TO, 0x30c8},         /* KATAKANA LETTER TO */
    {XK_kana_NA, 0x30ca},         /* KATAKANA LETTER NA */
    {XK_kana_NI, 0x30cb},         /* KATAKANA LETTER NI */
    {XK_kana_NU, 0x30cc},         /* KATAKANA LETTER NU */
    {XK_kana_NE, 0x30cd},         /* KATAKANA LETTER NE */
    {XK_kana_NO, 0x30ce},         /* KATAKANA LETTER NO */
    {XK_kana_HA, 0x30cf},         /* KATAKANA LETTER HA */
    {XK_kana_HI, 0x30d2},         /* KATAKANA LETTER HI */
    {XK_kana_FU, 0x30d5},         /* KATAKANA LETTER HU */
    {XK_kana_HE, 0x30d8},         /* KATAKANA LETTER HE */
    {XK_kana_HO, 0x30db},         /* KATAKANA LETTER HO */
    {XK_kana_MA, 0x30de},         /* KATAKANA LETTER MA */
    {XK_kana_MI, 0x30df},         /* KATAKANA LETTER MI */
    {XK_kana_MU, 0x30e0},         /* KATAKANA LETTER MU */
    {XK_kana_ME, 0x30e1},         /* KATAKANA LETTER ME */
    {XK_kana_MO, 0x30e2},         /* KATAKANA LETTER MO */
    {XK_kana_YA, 0x30e4},         /* KATAKANA LETTER YA */
    {XK_kana_YU, 0x30e6},         /* KATAKANA LETTER YU */
    {XK_kana_YO, 0x30e8},         /* KATAKANA LETTER YO */
    {XK_kana_RA, 0x30e9},         /* KATAKANA LETTER RA */
    {XK_kana_RI, 0x30ea},         /* KATAKANA LETTER RI */
    {XK_kana_RU, 0x30eb},         /* KATAKANA LETTER RU */
    {XK_kana_RE, 0x30ec},         /* KATAKANA LETTER RE */
    {XK_kana_RO, 0x30ed},         /* KATAKANA LETTER RO */
    {XK_kana_WA, 0x30ef},         /* KATAKANA LETTER WA */
    {XK_kana_N, 0x30f3},          /* KATAKANA LETTER N */
    {XK_voicedsound, 0x309b},     /* KATAKANA-HIRAGANA VOICED SOUND MARK */
    {XK_semivoicedsound, 0x309c}, /* KATAKANA-HIRAGANA SEMI-VOICED SOUND MARK */
#endif                            // defined(XK_overline)
#if defined(XK_Farsi_0)
    {XK_Farsi_0, 0x06f0},                 /* EXTENDED ARABIC-INDIC DIGIT 0 */
    {XK_Farsi_1, 0x06f1},                 /* EXTENDED ARABIC-INDIC DIGIT 1 */
    {XK_Farsi_2, 0x06f2},                 /* EXTENDED ARABIC-INDIC DIGIT 2 */
    {XK_Farsi_3, 0x06f3},                 /* EXTENDED ARABIC-INDIC DIGIT 3 */
    {XK_Farsi_4, 0x06f4},                 /* EXTENDED ARABIC-INDIC DIGIT 4 */
    {XK_Farsi_5, 0x06f5},                 /* EXTENDED ARABIC-INDIC DIGIT 5 */
    {XK_Farsi_6, 0x06f6},                 /* EXTENDED ARABIC-INDIC DIGIT 6 */
    {XK_Farsi_7, 0x06f7},                 /* EXTENDED ARABIC-INDIC DIGIT 7 */
    {XK_Farsi_8, 0x06f8},                 /* EXTENDED ARABIC-INDIC DIGIT 8 */
    {XK_Farsi_9, 0x06f9},                 /* EXTENDED ARABIC-INDIC DIGIT 9 */
    {XK_Arabic_percent, 0x066a},          /* ARABIC PERCENT */
    {XK_Arabic_superscript_alef, 0x0670}, /* ARABIC LETTER SUPERSCRIPT ALEF */
    {XK_Arabic_tteh, 0x0679},             /* ARABIC LETTER TTEH */
    {XK_Arabic_peh, 0x067e},              /* ARABIC LETTER PEH */
    {XK_Arabic_tcheh, 0x0686},            /* ARABIC LETTER TCHEH */
    {XK_Arabic_ddal, 0x0688},             /* ARABIC LETTER DDAL */
    {XK_Arabic_rreh, 0x0691},             /* ARABIC LETTER RREH */
    {XK_Arabic_comma, 0x060c},            /* ARABIC COMMA */
    {XK_Arabic_fullstop, 0x06d4},         /* ARABIC FULLSTOP */
    {XK_Arabic_semicolon, 0x061b},        /* ARABIC SEMICOLON */
    {XK_Arabic_0, 0x0660},                /* ARABIC 0 */
    {XK_Arabic_1, 0x0661},                /* ARABIC 1 */
    {XK_Arabic_2, 0x0662},                /* ARABIC 2 */
    {XK_Arabic_3, 0x0663},                /* ARABIC 3 */
    {XK_Arabic_4, 0x0664},                /* ARABIC 4 */
    {XK_Arabic_5, 0x0665},                /* ARABIC 5 */
    {XK_Arabic_6, 0x0666},                /* ARABIC 6 */
    {XK_Arabic_7, 0x0667},                /* ARABIC 7 */
    {XK_Arabic_8, 0x0668},                /* ARABIC 8 */
    {XK_Arabic_9, 0x0669},                /* ARABIC 9 */
    {XK_Arabic_question_mark, 0x061f},    /* ARABIC QUESTION MARK */
    {XK_Arabic_hamza, 0x0621},            /* ARABIC LETTER HAMZA */
    {XK_Arabic_maddaonalef, 0x0622}, /* ARABIC LETTER ALEF WITH MADDA ABOVE */
    {XK_Arabic_hamzaonalef, 0x0623}, /* ARABIC LETTER ALEF WITH HAMZA ABOVE */
    {XK_Arabic_hamzaonwaw, 0x0624},  /* ARABIC LETTER WAW WITH HAMZA ABOVE */
    {XK_Arabic_hamzaunderalef,
     0x0625},                        /* ARABIC LETTER ALEF WITH HAMZA BELOW */
    {XK_Arabic_hamzaonyeh, 0x0626},  /* ARABIC LETTER YEH WITH HAMZA ABOVE */
    {XK_Arabic_alef, 0x0627},        /* ARABIC LETTER ALEF */
    {XK_Arabic_beh, 0x0628},         /* ARABIC LETTER BEH */
    {XK_Arabic_tehmarbuta, 0x0629},  /* ARABIC LETTER TEH MARBUTA */
    {XK_Arabic_teh, 0x062a},         /* ARABIC LETTER TEH */
    {XK_Arabic_theh, 0x062b},        /* ARABIC LETTER THEH */
    {XK_Arabic_jeem, 0x062c},        /* ARABIC LETTER JEEM */
    {XK_Arabic_hah, 0x062d},         /* ARABIC LETTER HAH */
    {XK_Arabic_khah, 0x062e},        /* ARABIC LETTER KHAH */
    {XK_Arabic_dal, 0x062f},         /* ARABIC LETTER DAL */
    {XK_Arabic_thal, 0x0630},        /* ARABIC LETTER THAL */
    {XK_Arabic_ra, 0x0631},          /* ARABIC LETTER REH */
    {XK_Arabic_zain, 0x0632},        /* ARABIC LETTER ZAIN */
    {XK_Arabic_seen, 0x0633},        /* ARABIC LETTER SEEN */
    {XK_Arabic_sheen, 0x0634},       /* ARABIC LETTER SHEEN */
    {XK_Arabic_sad, 0x0635},         /* ARABIC LETTER SAD */
    {XK_Arabic_dad, 0x0636},         /* ARABIC LETTER DAD */
    {XK_Arabic_tah, 0x0637},         /* ARABIC LETTER TAH */
    {XK_Arabic_zah, 0x0638},         /* ARABIC LETTER ZAH */
    {XK_Arabic_ain, 0x0639},         /* ARABIC LETTER AIN */
    {XK_Arabic_ghain, 0x063a},       /* ARABIC LETTER GHAIN */
    {XK_Arabic_tatweel, 0x0640},     /* ARABIC TATWEEL */
    {XK_Arabic_feh, 0x0641},         /* ARABIC LETTER FEH */
    {XK_Arabic_qaf, 0x0642},         /* ARABIC LETTER QAF */
    {XK_Arabic_kaf, 0x0643},         /* ARABIC LETTER KAF */
    {XK_Arabic_lam, 0x0644},         /* ARABIC LETTER LAM */
    {XK_Arabic_meem, 0x0645},        /* ARABIC LETTER MEEM */
    {XK_Arabic_noon, 0x0646},        /* ARABIC LETTER NOON */
    {XK_Arabic_ha, 0x0647},          /* ARABIC LETTER HEH */
    {XK_Arabic_waw, 0x0648},         /* ARABIC LETTER WAW */
    {XK_Arabic_alefmaksura, 0x0649}, /* ARABIC LETTER ALEF MAKSURA */
    {XK_Arabic_yeh, 0x064a},         /* ARABIC LETTER YEH */
    {XK_Arabic_fathatan, 0x064b},    /* ARABIC FATHATAN */
    {XK_Arabic_dammatan, 0x064c},    /* ARABIC DAMMATAN */
    {XK_Arabic_kasratan, 0x064d},    /* ARABIC KASRATAN */
    {XK_Arabic_fatha, 0x064e},       /* ARABIC FATHA */
    {XK_Arabic_damma, 0x064f},       /* ARABIC DAMMA */
    {XK_Arabic_kasra, 0x0650},       /* ARABIC KASRA */
    {XK_Arabic_shadda, 0x0651},      /* ARABIC SHADDA */
    {XK_Arabic_sukun, 0x0652},       /* ARABIC SUKUN */
    {XK_Arabic_madda_above, 0x0653}, /* ARABIC MADDA ABOVE */
    {XK_Arabic_hamza_above, 0x0654}, /* ARABIC HAMZA ABOVE */
    {XK_Arabic_hamza_below, 0x0655}, /* ARABIC HAMZA BELOW */
    {XK_Arabic_jeh, 0x0698},         /* ARABIC LETTER JEH */
    {XK_Arabic_veh, 0x06a4},         /* ARABIC LETTER VEH */
    {XK_Arabic_keheh, 0x06a9},       /* ARABIC LETTER KEHEH */
    {XK_Arabic_gaf, 0x06af},         /* ARABIC LETTER GAF */
    {XK_Arabic_noon_ghunna, 0x06ba}, /* ARABIC LETTER NOON GHUNNA */
    {XK_Arabic_heh_doachashmee, 0x06be}, /* ARABIC LETTER HEH DOACHASHMEE */
    {XK_Arabic_farsi_yeh, 0x06cc},       /* ARABIC LETTER FARSI YEH */
    {XK_Arabic_yeh_baree, 0x06d2},       /* ARABIC LETTER YEH BAREE */
    {XK_Arabic_heh_goal, 0x06c1},        /* ARABIC LETTER HEH GOAL */
#endif                                   // defined(XK_Farsi_0)
#if defined(XK_Serbian_dje)
    {XK_Serbian_dje, 0x0452},   /* CYRILLIC SMALL LETTER DJE */
    {XK_Macedonia_gje, 0x0453}, /* CYRILLIC SMALL LETTER GJE */
    {XK_Cyrillic_io, 0x0451},   /* CYRILLIC SMALL LETTER IO */
    {XK_Ukrainian_ie, 0x0454},  /* CYRILLIC SMALL LETTER UKRAINIAN IE */
    {XK_Macedonia_dse, 0x0455}, /* CYRILLIC SMALL LETTER DZE */
    {XK_Ukrainian_i,
     0x0456}, /* CYRILLIC SMALL LETTER BYELORUSSIAN-UKRAINIAN I */
    {XK_Ukrainian_yi, 0x0457},  /* CYRILLIC SMALL LETTER YI */
    {XK_Cyrillic_je, 0x0458},   /* CYRILLIC SMALL LETTER JE */
    {XK_Cyrillic_lje, 0x0459},  /* CYRILLIC SMALL LETTER LJE */
    {XK_Cyrillic_nje, 0x045a},  /* CYRILLIC SMALL LETTER NJE */
    {XK_Serbian_tshe, 0x045b},  /* CYRILLIC SMALL LETTER TSHE */
    {XK_Macedonia_kje, 0x045c}, /* CYRILLIC SMALL LETTER KJE */
#if defined(XK_Ukrainian_ghe_with_upturn)
    {XK_Ukrainian_ghe_with_upturn,
     0x0491}, /* CYRILLIC SMALL LETTER GHE WITH UPTURN */
#endif
    {XK_Byelorussian_shortu, 0x045e}, /* CYRILLIC SMALL LETTER SHORT U */
    {XK_Cyrillic_dzhe, 0x045f},       /* CYRILLIC SMALL LETTER DZHE */
    {XK_numerosign, 0x2116},          /* NUMERO SIGN */
    {XK_Serbian_DJE, 0x0402},         /* CYRILLIC CAPITAL LETTER DJE */
    {XK_Macedonia_GJE, 0x0403},       /* CYRILLIC CAPITAL LETTER GJE */
    {XK_Cyrillic_IO, 0x0401},         /* CYRILLIC CAPITAL LETTER IO */
    {XK_Ukrainian_IE, 0x0404},        /* CYRILLIC CAPITAL LETTER UKRAINIAN IE */
    {XK_Macedonia_DSE, 0x0405},       /* CYRILLIC CAPITAL LETTER DZE */
    {XK_Ukrainian_I,
     0x0406}, /* CYRILLIC CAPITAL LETTER BYELORUSSIAN-UKRAINIAN I */
    {XK_Ukrainian_YI, 0x0407},  /* CYRILLIC CAPITAL LETTER YI */
    {XK_Cyrillic_JE, 0x0408},   /* CYRILLIC CAPITAL LETTER JE */
    {XK_Cyrillic_LJE, 0x0409},  /* CYRILLIC CAPITAL LETTER LJE */
    {XK_Cyrillic_NJE, 0x040a},  /* CYRILLIC CAPITAL LETTER NJE */
    {XK_Serbian_TSHE, 0x040b},  /* CYRILLIC CAPITAL LETTER TSHE */
    {XK_Macedonia_KJE, 0x040c}, /* CYRILLIC CAPITAL LETTER KJE */
#if defined(XK_Ukrainian_GHE_WITH_UPTURN)
    {XK_Ukrainian_GHE_WITH_UPTURN,
     0x0490}, /* CYRILLIC CAPITAL LETTER GHE WITH UPTURN */
#endif
    {XK_Byelorussian_SHORTU, 0x040e}, /* CYRILLIC CAPITAL LETTER SHORT U */
    {XK_Cyrillic_DZHE, 0x040f},       /* CYRILLIC CAPITAL LETTER DZHE */
    {XK_Cyrillic_yu, 0x044e},         /* CYRILLIC SMALL LETTER YU */
    {XK_Cyrillic_a, 0x0430},          /* CYRILLIC SMALL LETTER A */
    {XK_Cyrillic_be, 0x0431},         /* CYRILLIC SMALL LETTER BE */
    {XK_Cyrillic_tse, 0x0446},        /* CYRILLIC SMALL LETTER TSE */
    {XK_Cyrillic_de, 0x0434},         /* CYRILLIC SMALL LETTER DE */
    {XK_Cyrillic_ie, 0x0435},         /* CYRILLIC SMALL LETTER IE */
    {XK_Cyrillic_ef, 0x0444},         /* CYRILLIC SMALL LETTER EF */
    {XK_Cyrillic_ghe, 0x0433},        /* CYRILLIC SMALL LETTER GHE */
    {XK_Cyrillic_ha, 0x0445},         /* CYRILLIC SMALL LETTER HA */
    {XK_Cyrillic_i, 0x0438},          /* CYRILLIC SMALL LETTER I */
    {XK_Cyrillic_shorti, 0x0439},     /* CYRILLIC SMALL LETTER SHORT I */
    {XK_Cyrillic_ka, 0x043a},         /* CYRILLIC SMALL LETTER KA */
    {XK_Cyrillic_el, 0x043b},         /* CYRILLIC SMALL LETTER EL */
    {XK_Cyrillic_em, 0x043c},         /* CYRILLIC SMALL LETTER EM */
    {XK_Cyrillic_en, 0x043d},         /* CYRILLIC SMALL LETTER EN */
    {XK_Cyrillic_o, 0x043e},          /* CYRILLIC SMALL LETTER O */
    {XK_Cyrillic_pe, 0x043f},         /* CYRILLIC SMALL LETTER PE */
    {XK_Cyrillic_ya, 0x044f},         /* CYRILLIC SMALL LETTER YA */
    {XK_Cyrillic_er, 0x0440},         /* CYRILLIC SMALL LETTER ER */
    {XK_Cyrillic_es, 0x0441},         /* CYRILLIC SMALL LETTER ES */
    {XK_Cyrillic_te, 0x0442},         /* CYRILLIC SMALL LETTER TE */
    {XK_Cyrillic_u, 0x0443},          /* CYRILLIC SMALL LETTER U */
    {XK_Cyrillic_zhe, 0x0436},        /* CYRILLIC SMALL LETTER ZHE */
    {XK_Cyrillic_ve, 0x0432},         /* CYRILLIC SMALL LETTER VE */
    {XK_Cyrillic_softsign, 0x044c},   /* CYRILLIC SMALL LETTER SOFT SIGN */
    {XK_Cyrillic_yeru, 0x044b},       /* CYRILLIC SMALL LETTER YERU */
    {XK_Cyrillic_ze, 0x0437},         /* CYRILLIC SMALL LETTER ZE */
    {XK_Cyrillic_sha, 0x0448},        /* CYRILLIC SMALL LETTER SHA */
    {XK_Cyrillic_e, 0x044d},          /* CYRILLIC SMALL LETTER E */
    {XK_Cyrillic_shcha, 0x0449},      /* CYRILLIC SMALL LETTER SHCHA */
    {XK_Cyrillic_che, 0x0447},        /* CYRILLIC SMALL LETTER CHE */
    {XK_Cyrillic_hardsign, 0x044a},   /* CYRILLIC SMALL LETTER HARD SIGN */
    {XK_Cyrillic_YU, 0x042e},         /* CYRILLIC CAPITAL LETTER YU */
    {XK_Cyrillic_A, 0x0410},          /* CYRILLIC CAPITAL LETTER A */
    {XK_Cyrillic_BE, 0x0411},         /* CYRILLIC CAPITAL LETTER BE */
    {XK_Cyrillic_TSE, 0x0426},        /* CYRILLIC CAPITAL LETTER TSE */
    {XK_Cyrillic_DE, 0x0414},         /* CYRILLIC CAPITAL LETTER DE */
    {XK_Cyrillic_IE, 0x0415},         /* CYRILLIC CAPITAL LETTER IE */
    {XK_Cyrillic_EF, 0x0424},         /* CYRILLIC CAPITAL LETTER EF */
    {XK_Cyrillic_GHE, 0x0413},        /* CYRILLIC CAPITAL LETTER GHE */
    {XK_Cyrillic_HA, 0x0425},         /* CYRILLIC CAPITAL LETTER HA */
    {XK_Cyrillic_I, 0x0418},          /* CYRILLIC CAPITAL LETTER I */
    {XK_Cyrillic_SHORTI, 0x0419},     /* CYRILLIC CAPITAL LETTER SHORT I */
    {XK_Cyrillic_KA, 0x041a},         /* CYRILLIC CAPITAL LETTER KA */
    {XK_Cyrillic_EL, 0x041b},         /* CYRILLIC CAPITAL LETTER EL */
    {XK_Cyrillic_EM, 0x041c},         /* CYRILLIC CAPITAL LETTER EM */
    {XK_Cyrillic_EN, 0x041d},         /* CYRILLIC CAPITAL LETTER EN */
    {XK_Cyrillic_O, 0x041e},          /* CYRILLIC CAPITAL LETTER O */
    {XK_Cyrillic_PE, 0x041f},         /* CYRILLIC CAPITAL LETTER PE */
    {XK_Cyrillic_YA, 0x042f},         /* CYRILLIC CAPITAL LETTER YA */
    {XK_Cyrillic_ER, 0x0420},         /* CYRILLIC CAPITAL LETTER ER */
    {XK_Cyrillic_ES, 0x0421},         /* CYRILLIC CAPITAL LETTER ES */
    {XK_Cyrillic_TE, 0x0422},         /* CYRILLIC CAPITAL LETTER TE */
    {XK_Cyrillic_U, 0x0423},          /* CYRILLIC CAPITAL LETTER U */
    {XK_Cyrillic_ZHE, 0x0416},        /* CYRILLIC CAPITAL LETTER ZHE */
    {XK_Cyrillic_VE, 0x0412},         /* CYRILLIC CAPITAL LETTER VE */
    {XK_Cyrillic_SOFTSIGN, 0x042c},   /* CYRILLIC CAPITAL LETTER SOFT SIGN */
    {XK_Cyrillic_YERU, 0x042b},       /* CYRILLIC CAPITAL LETTER YERU */
    {XK_Cyrillic_ZE, 0x0417},         /* CYRILLIC CAPITAL LETTER ZE */
    {XK_Cyrillic_SHA, 0x0428},        /* CYRILLIC CAPITAL LETTER SHA */
    {XK_Cyrillic_E, 0x042d},          /* CYRILLIC CAPITAL LETTER E */
    {XK_Cyrillic_SHCHA, 0x0429},      /* CYRILLIC CAPITAL LETTER SHCHA */
    {XK_Cyrillic_CHE, 0x0427},        /* CYRILLIC CAPITAL LETTER CHE */
    {XK_Cyrillic_HARDSIGN, 0x042a},   /* CYRILLIC CAPITAL LETTER HARD SIGN */
#endif                                // defined(XK_Serbian_dje)
#if defined(XK_Greek_ALPHAaccent)
    {XK_Greek_ALPHAaccent, 0x0386}, /* GREEK CAPITAL LETTER ALPHA WITH TONOS */
    {XK_Greek_EPSILONaccent,
     0x0388},                      /* GREEK CAPITAL LETTER EPSILON WITH TONOS */
    {XK_Greek_ETAaccent, 0x0389},  /* GREEK CAPITAL LETTER ETA WITH TONOS */
    {XK_Greek_IOTAaccent, 0x038a}, /* GREEK CAPITAL LETTER IOTA WITH TONOS */
    {XK_Greek_IOTAdiaeresis,
     0x03aa}, /* GREEK CAPITAL LETTER IOTA WITH DIALYTIKA */
    {XK_Greek_OMICRONaccent,
     0x038c}, /* GREEK CAPITAL LETTER OMICRON WITH TONOS */
    {XK_Greek_UPSILONaccent,
     0x038e}, /* GREEK CAPITAL LETTER UPSILON WITH TONOS */
    {XK_Greek_UPSILONdieresis,
     0x03ab}, /* GREEK CAPITAL LETTER UPSILON WITH DIALYTIKA */
    {XK_Greek_OMEGAaccent, 0x038f}, /* GREEK CAPITAL LETTER OMEGA WITH TONOS */
    {XK_Greek_accentdieresis, 0x0385}, /* GREEK DIALYTIKA TONOS */
    {XK_Greek_horizbar, 0x2015},       /* HORIZONTAL BAR */
    {XK_Greek_alphaaccent, 0x03ac},    /* GREEK SMALL LETTER ALPHA WITH TONOS */
    {XK_Greek_epsilonaccent,
     0x03ad},                      /* GREEK SMALL LETTER EPSILON WITH TONOS */
    {XK_Greek_etaaccent, 0x03ae},  /* GREEK SMALL LETTER ETA WITH TONOS */
    {XK_Greek_iotaaccent, 0x03af}, /* GREEK SMALL LETTER IOTA WITH TONOS */
    {XK_Greek_iotadieresis,
     0x03ca}, /* GREEK SMALL LETTER IOTA WITH DIALYTIKA */
    {XK_Greek_iotaaccentdieresis,
     0x0390}, /* GREEK SMALL LETTER IOTA WITH DIALYTIKA AND TONOS */
    {XK_Greek_omicronaccent,
     0x03cc}, /* GREEK SMALL LETTER OMICRON WITH TONOS */
    {XK_Greek_upsilonaccent,
     0x03cd}, /* GREEK SMALL LETTER UPSILON WITH TONOS */
    {XK_Greek_upsilondieresis,
     0x03cb}, /* GREEK SMALL LETTER UPSILON WITH DIALYTIKA */
    {XK_Greek_upsilonaccentdieresis,
     0x03b0}, /* GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND TONOS */
    {XK_Greek_omegaaccent, 0x03ce}, /* GREEK SMALL LETTER OMEGA WITH TONOS */
    {XK_Greek_ALPHA, 0x0391},       /* GREEK CAPITAL LETTER ALPHA */
    {XK_Greek_BETA, 0x0392},        /* GREEK CAPITAL LETTER BETA */
    {XK_Greek_GAMMA, 0x0393},       /* GREEK CAPITAL LETTER GAMMA */
    {XK_Greek_DELTA, 0x0394},       /* GREEK CAPITAL LETTER DELTA */
    {XK_Greek_EPSILON, 0x0395},     /* GREEK CAPITAL LETTER EPSILON */
    {XK_Greek_ZETA, 0x0396},        /* GREEK CAPITAL LETTER ZETA */
    {XK_Greek_ETA, 0x0397},         /* GREEK CAPITAL LETTER ETA */
    {XK_Greek_THETA, 0x0398},       /* GREEK CAPITAL LETTER THETA */
    {XK_Greek_IOTA, 0x0399},        /* GREEK CAPITAL LETTER IOTA */
    {XK_Greek_KAPPA, 0x039a},       /* GREEK CAPITAL LETTER KAPPA */
    {XK_Greek_LAMBDA, 0x039b},      /* GREEK CAPITAL LETTER LAMDA */
    {XK_Greek_MU, 0x039c},          /* GREEK CAPITAL LETTER MU */
    {XK_Greek_NU, 0x039d},          /* GREEK CAPITAL LETTER NU */
    {XK_Greek_XI, 0x039e},          /* GREEK CAPITAL LETTER XI */
    {XK_Greek_OMICRON, 0x039f},     /* GREEK CAPITAL LETTER OMICRON */
    {XK_Greek_PI, 0x03a0},          /* GREEK CAPITAL LETTER PI */
    {XK_Greek_RHO, 0x03a1},         /* GREEK CAPITAL LETTER RHO */
    {XK_Greek_SIGMA, 0x03a3},       /* GREEK CAPITAL LETTER SIGMA */
    {XK_Greek_TAU, 0x03a4},         /* GREEK CAPITAL LETTER TAU */
    {XK_Greek_UPSILON, 0x03a5},     /* GREEK CAPITAL LETTER UPSILON */
    {XK_Greek_PHI, 0x03a6},         /* GREEK CAPITAL LETTER PHI */
    {XK_Greek_CHI, 0x03a7},         /* GREEK CAPITAL LETTER CHI */
    {XK_Greek_PSI, 0x03a8},         /* GREEK CAPITAL LETTER PSI */
    {XK_Greek_OMEGA, 0x03a9},       /* GREEK CAPITAL LETTER OMEGA */
    {XK_Greek_alpha, 0x03b1},       /* GREEK SMALL LETTER ALPHA */
    {XK_Greek_beta, 0x03b2},        /* GREEK SMALL LETTER BETA */
    {XK_Greek_gamma, 0x03b3},       /* GREEK SMALL LETTER GAMMA */
    {XK_Greek_delta, 0x03b4},       /* GREEK SMALL LETTER DELTA */
    {XK_Greek_epsilon, 0x03b5},     /* GREEK SMALL LETTER EPSILON */
    {XK_Greek_zeta, 0x03b6},        /* GREEK SMALL LETTER ZETA */
    {XK_Greek_eta, 0x03b7},         /* GREEK SMALL LETTER ETA */
    {XK_Greek_theta, 0x03b8},       /* GREEK SMALL LETTER THETA */
    {XK_Greek_iota, 0x03b9},        /* GREEK SMALL LETTER IOTA */
    {XK_Greek_kappa, 0x03ba},       /* GREEK SMALL LETTER KAPPA */
    {XK_Greek_lambda, 0x03bb},      /* GREEK SMALL LETTER LAMDA */
    {XK_Greek_mu, 0x03bc},          /* GREEK SMALL LETTER MU */
    {XK_Greek_nu, 0x03bd},          /* GREEK SMALL LETTER NU */
    {XK_Greek_xi, 0x03be},          /* GREEK SMALL LETTER XI */
    {XK_Greek_omicron, 0x03bf},     /* GREEK SMALL LETTER OMICRON */
    {XK_Greek_pi, 0x03c0},          /* GREEK SMALL LETTER PI */
    {XK_Greek_rho, 0x03c1},         /* GREEK SMALL LETTER RHO */
    {XK_Greek_sigma, 0x03c3},       /* GREEK SMALL LETTER SIGMA */
    {XK_Greek_finalsmallsigma, 0x03c2}, /* GREEK SMALL LETTER FINAL SIGMA */
    {XK_Greek_tau, 0x03c4},             /* GREEK SMALL LETTER TAU */
    {XK_Greek_upsilon, 0x03c5},         /* GREEK SMALL LETTER UPSILON */
    {XK_Greek_phi, 0x03c6},             /* GREEK SMALL LETTER PHI */
    {XK_Greek_chi, 0x03c7},             /* GREEK SMALL LETTER CHI */
    {XK_Greek_psi, 0x03c8},             /* GREEK SMALL LETTER PSI */
    {XK_Greek_omega, 0x03c9},           /* GREEK SMALL LETTER OMEGA */
#endif                                  // defined(XK_Greek_ALPHAaccent)
    {XK_leftradical, 0x23b7},           /* ??? */
    {XK_topleftradical, 0x250c},        /* BOX DRAWINGS LIGHT DOWN AND RIGHT */
    {XK_horizconnector, 0x2500},        /* BOX DRAWINGS LIGHT HORIZONTAL */
    {XK_topintegral, 0x2320},           /* TOP HALF INTEGRAL */
    {XK_botintegral, 0x2321},           /* BOTTOM HALF INTEGRAL */
    {XK_vertconnector, 0x2502},         /* BOX DRAWINGS LIGHT VERTICAL */
    {XK_topleftsqbracket, 0x23a1},      /* ??? */
    {XK_botleftsqbracket, 0x23a3},      /* ??? */
    {XK_toprightsqbracket, 0x23a4},     /* ??? */
    {XK_botrightsqbracket, 0x23a6},     /* ??? */
    {XK_topleftparens, 0x239b},         /* ??? */
    {XK_botleftparens, 0x239d},         /* ??? */
    {XK_toprightparens, 0x239e},        /* ??? */
    {XK_botrightparens, 0x23a0},        /* ??? */
    {XK_leftmiddlecurlybrace, 0x23a8},  /* ??? */
    {XK_rightmiddlecurlybrace, 0x23ac}, /* ??? */
    {XK_lessthanequal, 0x2264},         /* LESS-THAN OR EQUAL TO */
    {XK_notequal, 0x2260},              /* NOT EQUAL TO */
    {XK_greaterthanequal, 0x2265},      /* GREATER-THAN OR EQUAL TO */
    {XK_integral, 0x222b},              /* INTEGRAL */
    {XK_therefore, 0x2234},             /* THEREFORE */
    {XK_variation, 0x221d},             /* PROPORTIONAL TO */
    {XK_infinity, 0x221e},              /* INFINITY */
    {XK_nabla, 0x2207},                 /* NABLA */
    {XK_approximate, 0x223c},           /* TILDE OPERATOR */
    {XK_similarequal, 0x2243},          /* ASYMPTOTICALLY EQUAL TO */
    {XK_ifonlyif, 0x21d4},              /* LEFT RIGHT DOUBLE ARROW */
    {XK_implies, 0x21d2},               /* RIGHTWARDS DOUBLE ARROW */
    {XK_identical, 0x2261},             /* IDENTICAL TO */
    {XK_radical, 0x221a},               /* SQUARE ROOT */
    {XK_includedin, 0x2282},            /* SUBSET OF */
    {XK_includes, 0x2283},              /* SUPERSET OF */
    {XK_intersection, 0x2229},          /* INTERSECTION */
    {XK_union, 0x222a},                 /* UNION */
    {XK_logicaland, 0x2227},            /* LOGICAL AND */
    {XK_logicalor, 0x2228},             /* LOGICAL OR */
    {XK_partialderivative, 0x2202},     /* PARTIAL DIFFERENTIAL */
    {XK_function, 0x0192},              /* LATIN SMALL LETTER F WITH HOOK */
    {XK_leftarrow, 0x2190},             /* LEFTWARDS ARROW */
    {XK_uparrow, 0x2191},               /* UPWARDS ARROW */
    {XK_rightarrow, 0x2192},            /* RIGHTWARDS ARROW */
    {XK_downarrow, 0x2193},             /* DOWNWARDS ARROW */
    /*{ XK_blank,                        ??? }, */
    {XK_soliddiamond, 0x25c6},   /* BLACK DIAMOND */
    {XK_checkerboard, 0x2592},   /* MEDIUM SHADE */
    {XK_ht, 0x2409},             /* SYMBOL FOR HORIZONTAL TABULATION */
    {XK_ff, 0x240c},             /* SYMBOL FOR FORM FEED */
    {XK_cr, 0x240d},             /* SYMBOL FOR CARRIAGE RETURN */
    {XK_lf, 0x240a},             /* SYMBOL FOR LINE FEED */
    {XK_nl, 0x2424},             /* SYMBOL FOR NEWLINE */
    {XK_vt, 0x240b},             /* SYMBOL FOR VERTICAL TABULATION */
    {XK_lowrightcorner, 0x2518}, /* BOX DRAWINGS LIGHT UP AND LEFT */
    {XK_uprightcorner, 0x2510},  /* BOX DRAWINGS LIGHT DOWN AND LEFT */
    {XK_upleftcorner, 0x250c},   /* BOX DRAWINGS LIGHT DOWN AND RIGHT */
    {XK_lowleftcorner, 0x2514},  /* BOX DRAWINGS LIGHT UP AND RIGHT */
    {XK_crossinglines, 0x253c}, /* BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL */
    {XK_horizlinescan1,
     0x23ba}, /* HORIZONTAL SCAN LINE-1 (Unicode 3.2 draft) */
    {XK_horizlinescan3,
     0x23bb}, /* HORIZONTAL SCAN LINE-3 (Unicode 3.2 draft) */
    {XK_horizlinescan5, 0x2500}, /* BOX DRAWINGS LIGHT HORIZONTAL */
    {XK_horizlinescan7,
     0x23bc}, /* HORIZONTAL SCAN LINE-7 (Unicode 3.2 draft) */
    {XK_horizlinescan9,
     0x23bd},                /* HORIZONTAL SCAN LINE-9 (Unicode 3.2 draft) */
    {XK_leftt, 0x251c},      /* BOX DRAWINGS LIGHT VERTICAL AND RIGHT */
    {XK_rightt, 0x2524},     /* BOX DRAWINGS LIGHT VERTICAL AND LEFT */
    {XK_bott, 0x2534},       /* BOX DRAWINGS LIGHT UP AND HORIZONTAL */
    {XK_topt, 0x252c},       /* BOX DRAWINGS LIGHT DOWN AND HORIZONTAL */
    {XK_vertbar, 0x2502},    /* BOX DRAWINGS LIGHT VERTICAL */
    {XK_emspace, 0x2003},    /* EM SPACE */
    {XK_enspace, 0x2002},    /* EN SPACE */
    {XK_em3space, 0x2004},   /* THREE-PER-EM SPACE */
    {XK_em4space, 0x2005},   /* FOUR-PER-EM SPACE */
    {XK_digitspace, 0x2007}, /* FIGURE SPACE */
    {XK_punctspace, 0x2008}, /* PUNCTUATION SPACE */
    {XK_thinspace, 0x2009},  /* THIN SPACE */
    {XK_hairspace, 0x200a},  /* HAIR SPACE */
    {XK_emdash, 0x2014},     /* EM DASH */
    {XK_endash, 0x2013},     /* EN DASH */
    /*{ XK_signifblank,                  ??? }, */
    {XK_ellipsis, 0x2026},         /* HORIZONTAL ELLIPSIS */
    {XK_doubbaselinedot, 0x2025},  /* TWO DOT LEADER */
    {XK_onethird, 0x2153},         /* VULGAR FRACTION ONE THIRD */
    {XK_twothirds, 0x2154},        /* VULGAR FRACTION TWO THIRDS */
    {XK_onefifth, 0x2155},         /* VULGAR FRACTION ONE FIFTH */
    {XK_twofifths, 0x2156},        /* VULGAR FRACTION TWO FIFTHS */
    {XK_threefifths, 0x2157},      /* VULGAR FRACTION THREE FIFTHS */
    {XK_fourfifths, 0x2158},       /* VULGAR FRACTION FOUR FIFTHS */
    {XK_onesixth, 0x2159},         /* VULGAR FRACTION ONE SIXTH */
    {XK_fivesixths, 0x215a},       /* VULGAR FRACTION FIVE SIXTHS */
    {XK_careof, 0x2105},           /* CARE OF */
    {XK_figdash, 0x2012},          /* FIGURE DASH */
    {XK_leftanglebracket, 0x2329}, /* LEFT-POINTING ANGLE BRACKET */
    /*{ XK_decimalpoint,                 ??? }, */
    {XK_rightanglebracket, 0x232a}, /* RIGHT-POINTING ANGLE BRACKET */
    /*{ XK_marker,                       ??? }, */
    {XK_oneeighth, 0x215b},     /* VULGAR FRACTION ONE EIGHTH */
    {XK_threeeighths, 0x215c},  /* VULGAR FRACTION THREE EIGHTHS */
    {XK_fiveeighths, 0x215d},   /* VULGAR FRACTION FIVE EIGHTHS */
    {XK_seveneighths, 0x215e},  /* VULGAR FRACTION SEVEN EIGHTHS */
    {XK_trademark, 0x2122},     /* TRADE MARK SIGN */
    {XK_signaturemark, 0x2613}, /* SALTIRE */
    /*{ XK_trademarkincircle,            ??? }, */
    {XK_leftopentriangle, 0x25c1},     /* WHITE LEFT-POINTING TRIANGLE */
    {XK_rightopentriangle, 0x25b7},    /* WHITE RIGHT-POINTING TRIANGLE */
    {XK_emopencircle, 0x25cb},         /* WHITE CIRCLE */
    {XK_emopenrectangle, 0x25af},      /* WHITE VERTICAL RECTANGLE */
    {XK_leftsinglequotemark, 0x2018},  /* LEFT SINGLE QUOTATION MARK */
    {XK_rightsinglequotemark, 0x2019}, /* RIGHT SINGLE QUOTATION MARK */
    {XK_leftdoublequotemark, 0x201c},  /* LEFT DOUBLE QUOTATION MARK */
    {XK_rightdoublequotemark, 0x201d}, /* RIGHT DOUBLE QUOTATION MARK */
    {XK_prescription, 0x211e},         /* PRESCRIPTION TAKE */
    {XK_minutes, 0x2032},              /* PRIME */
    {XK_seconds, 0x2033},              /* DOUBLE PRIME */
    {XK_latincross, 0x271d},           /* LATIN CROSS */
    /*{ XK_hexagram,                     ??? }, */
    {XK_filledrectbullet, 0x25ac},     /* BLACK RECTANGLE */
    {XK_filledlefttribullet, 0x25c0},  /* BLACK LEFT-POINTING TRIANGLE */
    {XK_filledrighttribullet, 0x25b6}, /* BLACK RIGHT-POINTING TRIANGLE */
    {XK_emfilledcircle, 0x25cf},       /* BLACK CIRCLE */
    {XK_emfilledrect, 0x25ae},         /* BLACK VERTICAL RECTANGLE */
    {XK_enopencircbullet, 0x25e6},     /* WHITE BULLET */
    {XK_enopensquarebullet, 0x25ab},   /* WHITE SMALL SQUARE */
    {XK_openrectbullet, 0x25ad},       /* WHITE RECTANGLE */
    {XK_opentribulletup, 0x25b3},      /* WHITE UP-POINTING TRIANGLE */
    {XK_opentribulletdown, 0x25bd},    /* WHITE DOWN-POINTING TRIANGLE */
    {XK_openstar, 0x2606},             /* WHITE STAR */
    {XK_enfilledcircbullet, 0x2022},   /* BULLET */
    {XK_enfilledsqbullet, 0x25aa},     /* BLACK SMALL SQUARE */
    {XK_filledtribulletup, 0x25b2},    /* BLACK UP-POINTING TRIANGLE */
    {XK_filledtribulletdown, 0x25bc},  /* BLACK DOWN-POINTING TRIANGLE */
    {XK_leftpointer, 0x261c},          /* WHITE LEFT POINTING INDEX */
    {XK_rightpointer, 0x261e},         /* WHITE RIGHT POINTING INDEX */
    {XK_club, 0x2663},                 /* BLACK CLUB SUIT */
    {XK_diamond, 0x2666},              /* BLACK DIAMOND SUIT */
    {XK_heart, 0x2665},                /* BLACK HEART SUIT */
    {XK_maltesecross, 0x2720},         /* MALTESE CROSS */
    {XK_dagger, 0x2020},               /* DAGGER */
    {XK_doubledagger, 0x2021},         /* DOUBLE DAGGER */
    {XK_checkmark, 0x2713},            /* CHECK MARK */
    {XK_ballotcross, 0x2717},          /* BALLOT X */
    {XK_musicalsharp, 0x266f},         /* MUSIC SHARP SIGN */
    {XK_musicalflat, 0x266d},          /* MUSIC FLAT SIGN */
    {XK_malesymbol, 0x2642},           /* MALE SIGN */
    {XK_femalesymbol, 0x2640},         /* FEMALE SIGN */
    {XK_telephone, 0x260e},            /* BLACK TELEPHONE */
    {XK_telephonerecorder, 0x2315},    /* TELEPHONE RECORDER */
    {XK_phonographcopyright, 0x2117},  /* SOUND RECORDING COPYRIGHT */
    {XK_caret, 0x2038},                /* CARET */
    {XK_singlelowquotemark, 0x201a},   /* SINGLE LOW-9 QUOTATION MARK */
    {XK_doublelowquotemark, 0x201e},   /* DOUBLE LOW-9 QUOTATION MARK */
    /*{ XK_cursor,                       ??? }, */
    {XK_leftcaret, 0x003c},  /* LESS-THAN SIGN */
    {XK_rightcaret, 0x003e}, /* GREATER-THAN SIGN */
    {XK_downcaret, 0x2228},  /* LOGICAL OR */
    {XK_upcaret, 0x2227},    /* LOGICAL AND */
    {XK_overbar, 0x00af},    /* MACRON */
    {XK_downtack, 0x22a5},   /* UP TACK */
    {XK_upshoe, 0x2229},     /* INTERSECTION */
    {XK_downstile, 0x230a},  /* LEFT FLOOR */
    {XK_underbar, 0x005f},   /* LOW LINE */
    {XK_jot, 0x2218},        /* RING OPERATOR */
    {XK_quad, 0x2395},       /* APL FUNCTIONAL SYMBOL QUAD */
    {XK_uptack, 0x22a4},     /* DOWN TACK */
    {XK_circle, 0x25cb},     /* WHITE CIRCLE */
    {XK_upstile, 0x2308},    /* LEFT CEILING */
    {XK_downshoe, 0x222a},   /* UNION */
    {XK_rightshoe, 0x2283},  /* SUPERSET OF */
    {XK_leftshoe, 0x2282},   /* SUBSET OF */
    {XK_lefttack, 0x22a2},   /* RIGHT TACK */
    {XK_righttack, 0x22a3},  /* LEFT TACK */
#if defined(XK_hebrew_doublelowline)
    {XK_hebrew_doublelowline, 0x2017}, /* DOUBLE LOW LINE */
    {XK_hebrew_aleph, 0x05d0},         /* HEBREW LETTER ALEF */
    {XK_hebrew_bet, 0x05d1},           /* HEBREW LETTER BET */
    {XK_hebrew_gimel, 0x05d2},         /* HEBREW LETTER GIMEL */
    {XK_hebrew_dalet, 0x05d3},         /* HEBREW LETTER DALET */
    {XK_hebrew_he, 0x05d4},            /* HEBREW LETTER HE */
    {XK_hebrew_waw, 0x05d5},           /* HEBREW LETTER VAV */
    {XK_hebrew_zain, 0x05d6},          /* HEBREW LETTER ZAYIN */
    {XK_hebrew_chet, 0x05d7},          /* HEBREW LETTER HET */
    {XK_hebrew_tet, 0x05d8},           /* HEBREW LETTER TET */
    {XK_hebrew_yod, 0x05d9},           /* HEBREW LETTER YOD */
    {XK_hebrew_finalkaph, 0x05da},     /* HEBREW LETTER FINAL KAF */
    {XK_hebrew_kaph, 0x05db},          /* HEBREW LETTER KAF */
    {XK_hebrew_lamed, 0x05dc},         /* HEBREW LETTER LAMED */
    {XK_hebrew_finalmem, 0x05dd},      /* HEBREW LETTER FINAL MEM */
    {XK_hebrew_mem, 0x05de},           /* HEBREW LETTER MEM */
    {XK_hebrew_finalnun, 0x05df},      /* HEBREW LETTER FINAL NUN */
    {XK_hebrew_nun, 0x05e0},           /* HEBREW LETTER NUN */
    {XK_hebrew_samech, 0x05e1},        /* HEBREW LETTER SAMEKH */
    {XK_hebrew_ayin, 0x05e2},          /* HEBREW LETTER AYIN */
    {XK_hebrew_finalpe, 0x05e3},       /* HEBREW LETTER FINAL PE */
    {XK_hebrew_pe, 0x05e4},            /* HEBREW LETTER PE */
    {XK_hebrew_finalzade, 0x05e5},     /* HEBREW LETTER FINAL TSADI */
    {XK_hebrew_zade, 0x05e6},          /* HEBREW LETTER TSADI */
    {XK_hebrew_qoph, 0x05e7},          /* HEBREW LETTER QOF */
    {XK_hebrew_resh, 0x05e8},          /* HEBREW LETTER RESH */
    {XK_hebrew_shin, 0x05e9},          /* HEBREW LETTER SHIN */
    {XK_hebrew_taw, 0x05ea},           /* HEBREW LETTER TAV */
#endif                                 // defined(XK_hebrew_doublelowline)
#if defined(XK_Thai_kokai)
    {XK_Thai_kokai, 0x0e01},         /* THAI CHARACTER KO KAI */
    {XK_Thai_khokhai, 0x0e02},       /* THAI CHARACTER KHO KHAI */
    {XK_Thai_khokhuat, 0x0e03},      /* THAI CHARACTER KHO KHUAT */
    {XK_Thai_khokhwai, 0x0e04},      /* THAI CHARACTER KHO KHWAI */
    {XK_Thai_khokhon, 0x0e05},       /* THAI CHARACTER KHO KHON */
    {XK_Thai_khorakhang, 0x0e06},    /* THAI CHARACTER KHO RAKHANG */
    {XK_Thai_ngongu, 0x0e07},        /* THAI CHARACTER NGO NGU */
    {XK_Thai_chochan, 0x0e08},       /* THAI CHARACTER CHO CHAN */
    {XK_Thai_choching, 0x0e09},      /* THAI CHARACTER CHO CHING */
    {XK_Thai_chochang, 0x0e0a},      /* THAI CHARACTER CHO CHANG */
    {XK_Thai_soso, 0x0e0b},          /* THAI CHARACTER SO SO */
    {XK_Thai_chochoe, 0x0e0c},       /* THAI CHARACTER CHO CHOE */
    {XK_Thai_yoying, 0x0e0d},        /* THAI CHARACTER YO YING */
    {XK_Thai_dochada, 0x0e0e},       /* THAI CHARACTER DO CHADA */
    {XK_Thai_topatak, 0x0e0f},       /* THAI CHARACTER TO PATAK */
    {XK_Thai_thothan, 0x0e10},       /* THAI CHARACTER THO THAN */
    {XK_Thai_thonangmontho, 0x0e11}, /* THAI CHARACTER THO NANGMONTHO */
    {XK_Thai_thophuthao, 0x0e12},    /* THAI CHARACTER THO PHUTHAO */
    {XK_Thai_nonen, 0x0e13},         /* THAI CHARACTER NO NEN */
    {XK_Thai_dodek, 0x0e14},         /* THAI CHARACTER DO DEK */
    {XK_Thai_totao, 0x0e15},         /* THAI CHARACTER TO TAO */
    {XK_Thai_thothung, 0x0e16},      /* THAI CHARACTER THO THUNG */
    {XK_Thai_thothahan, 0x0e17},     /* THAI CHARACTER THO THAHAN */
    {XK_Thai_thothong, 0x0e18},      /* THAI CHARACTER THO THONG */
    {XK_Thai_nonu, 0x0e19},          /* THAI CHARACTER NO NU */
    {XK_Thai_bobaimai, 0x0e1a},      /* THAI CHARACTER BO BAIMAI */
    {XK_Thai_popla, 0x0e1b},         /* THAI CHARACTER PO PLA */
    {XK_Thai_phophung, 0x0e1c},      /* THAI CHARACTER PHO PHUNG */
    {XK_Thai_fofa, 0x0e1d},          /* THAI CHARACTER FO FA */
    {XK_Thai_phophan, 0x0e1e},       /* THAI CHARACTER PHO PHAN */
    {XK_Thai_fofan, 0x0e1f},         /* THAI CHARACTER FO FAN */
    {XK_Thai_phosamphao, 0x0e20},    /* THAI CHARACTER PHO SAMPHAO */
    {XK_Thai_moma, 0x0e21},          /* THAI CHARACTER MO MA */
    {XK_Thai_yoyak, 0x0e22},         /* THAI CHARACTER YO YAK */
    {XK_Thai_rorua, 0x0e23},         /* THAI CHARACTER RO RUA */
    {XK_Thai_ru, 0x0e24},            /* THAI CHARACTER RU */
    {XK_Thai_loling, 0x0e25},        /* THAI CHARACTER LO LING */
    {XK_Thai_lu, 0x0e26},            /* THAI CHARACTER LU */
    {XK_Thai_wowaen, 0x0e27},        /* THAI CHARACTER WO WAEN */
    {XK_Thai_sosala, 0x0e28},        /* THAI CHARACTER SO SALA */
    {XK_Thai_sorusi, 0x0e29},        /* THAI CHARACTER SO RUSI */
    {XK_Thai_sosua, 0x0e2a},         /* THAI CHARACTER SO SUA */
    {XK_Thai_hohip, 0x0e2b},         /* THAI CHARACTER HO HIP */
    {XK_Thai_lochula, 0x0e2c},       /* THAI CHARACTER LO CHULA */
    {XK_Thai_oang, 0x0e2d},          /* THAI CHARACTER O ANG */
    {XK_Thai_honokhuk, 0x0e2e},      /* THAI CHARACTER HO NOKHUK */
    {XK_Thai_paiyannoi, 0x0e2f},     /* THAI CHARACTER PAIYANNOI */
    {XK_Thai_saraa, 0x0e30},         /* THAI CHARACTER SARA A */
    {XK_Thai_maihanakat, 0x0e31},    /* THAI CHARACTER MAI HAN-AKAT */
    {XK_Thai_saraaa, 0x0e32},        /* THAI CHARACTER SARA AA */
    {XK_Thai_saraam, 0x0e33},        /* THAI CHARACTER SARA AM */
    {XK_Thai_sarai, 0x0e34},         /* THAI CHARACTER SARA I */
    {XK_Thai_saraii, 0x0e35},        /* THAI CHARACTER SARA II */
    {XK_Thai_saraue, 0x0e36},        /* THAI CHARACTER SARA UE */
    {XK_Thai_sarauee, 0x0e37},       /* THAI CHARACTER SARA UEE */
    {XK_Thai_sarau, 0x0e38},         /* THAI CHARACTER SARA U */
    {XK_Thai_sarauu, 0x0e39},        /* THAI CHARACTER SARA UU */
    {XK_Thai_phinthu, 0x0e3a},       /* THAI CHARACTER PHINTHU */
    /*{ XK_Thai_maihanakat_maitho,       ??? }, */
    {XK_Thai_baht, 0x0e3f},           /* THAI CURRENCY SYMBOL BAHT */
    {XK_Thai_sarae, 0x0e40},          /* THAI CHARACTER SARA E */
    {XK_Thai_saraae, 0x0e41},         /* THAI CHARACTER SARA AE */
    {XK_Thai_sarao, 0x0e42},          /* THAI CHARACTER SARA O */
    {XK_Thai_saraaimaimuan, 0x0e43},  /* THAI CHARACTER SARA AI MAIMUAN */
    {XK_Thai_saraaimaimalai, 0x0e44}, /* THAI CHARACTER SARA AI MAIMALAI */
    {XK_Thai_lakkhangyao, 0x0e45},    /* THAI CHARACTER LAKKHANGYAO */
    {XK_Thai_maiyamok, 0x0e46},       /* THAI CHARACTER MAIYAMOK */
    {XK_Thai_maitaikhu, 0x0e47},      /* THAI CHARACTER MAITAIKHU */
    {XK_Thai_maiek, 0x0e48},          /* THAI CHARACTER MAI EK */
    {XK_Thai_maitho, 0x0e49},         /* THAI CHARACTER MAI THO */
    {XK_Thai_maitri, 0x0e4a},         /* THAI CHARACTER MAI TRI */
    {XK_Thai_maichattawa, 0x0e4b},    /* THAI CHARACTER MAI CHATTAWA */
    {XK_Thai_thanthakhat, 0x0e4c},    /* THAI CHARACTER THANTHAKHAT */
    {XK_Thai_nikhahit, 0x0e4d},       /* THAI CHARACTER NIKHAHIT */
    {XK_Thai_leksun, 0x0e50},         /* THAI DIGIT ZERO */
    {XK_Thai_leknung, 0x0e51},        /* THAI DIGIT ONE */
    {XK_Thai_leksong, 0x0e52},        /* THAI DIGIT TWO */
    {XK_Thai_leksam, 0x0e53},         /* THAI DIGIT THREE */
    {XK_Thai_leksi, 0x0e54},          /* THAI DIGIT FOUR */
    {XK_Thai_lekha, 0x0e55},          /* THAI DIGIT FIVE */
    {XK_Thai_lekhok, 0x0e56},         /* THAI DIGIT SIX */
    {XK_Thai_lekchet, 0x0e57},        /* THAI DIGIT SEVEN */
    {XK_Thai_lekpaet, 0x0e58},        /* THAI DIGIT EIGHT */
    {XK_Thai_lekkao, 0x0e59},         /* THAI DIGIT NINE */
#endif                                // defined(XK_Thai_kokai)
#if defined(XK_Hangul_Kiyeog)
    {XK_Hangul_Kiyeog, 0x3131},            /* HANGUL LETTER KIYEOK */
    {XK_Hangul_SsangKiyeog, 0x3132},       /* HANGUL LETTER SSANGKIYEOK */
    {XK_Hangul_KiyeogSios, 0x3133},        /* HANGUL LETTER KIYEOK-SIOS */
    {XK_Hangul_Nieun, 0x3134},             /* HANGUL LETTER NIEUN */
    {XK_Hangul_NieunJieuj, 0x3135},        /* HANGUL LETTER NIEUN-CIEUC */
    {XK_Hangul_NieunHieuh, 0x3136},        /* HANGUL LETTER NIEUN-HIEUH */
    {XK_Hangul_Dikeud, 0x3137},            /* HANGUL LETTER TIKEUT */
    {XK_Hangul_SsangDikeud, 0x3138},       /* HANGUL LETTER SSANGTIKEUT */
    {XK_Hangul_Rieul, 0x3139},             /* HANGUL LETTER RIEUL */
    {XK_Hangul_RieulKiyeog, 0x313a},       /* HANGUL LETTER RIEUL-KIYEOK */
    {XK_Hangul_RieulMieum, 0x313b},        /* HANGUL LETTER RIEUL-MIEUM */
    {XK_Hangul_RieulPieub, 0x313c},        /* HANGUL LETTER RIEUL-PIEUP */
    {XK_Hangul_RieulSios, 0x313d},         /* HANGUL LETTER RIEUL-SIOS */
    {XK_Hangul_RieulTieut, 0x313e},        /* HANGUL LETTER RIEUL-THIEUTH */
    {XK_Hangul_RieulPhieuf, 0x313f},       /* HANGUL LETTER RIEUL-PHIEUPH */
    {XK_Hangul_RieulHieuh, 0x3140},        /* HANGUL LETTER RIEUL-HIEUH */
    {XK_Hangul_Mieum, 0x3141},             /* HANGUL LETTER MIEUM */
    {XK_Hangul_Pieub, 0x3142},             /* HANGUL LETTER PIEUP */
    {XK_Hangul_SsangPieub, 0x3143},        /* HANGUL LETTER SSANGPIEUP */
    {XK_Hangul_PieubSios, 0x3144},         /* HANGUL LETTER PIEUP-SIOS */
    {XK_Hangul_Sios, 0x3145},              /* HANGUL LETTER SIOS */
    {XK_Hangul_SsangSios, 0x3146},         /* HANGUL LETTER SSANGSIOS */
    {XK_Hangul_Ieung, 0x3147},             /* HANGUL LETTER IEUNG */
    {XK_Hangul_Jieuj, 0x3148},             /* HANGUL LETTER CIEUC */
    {XK_Hangul_SsangJieuj, 0x3149},        /* HANGUL LETTER SSANGCIEUC */
    {XK_Hangul_Cieuc, 0x314a},             /* HANGUL LETTER CHIEUCH */
    {XK_Hangul_Khieuq, 0x314b},            /* HANGUL LETTER KHIEUKH */
    {XK_Hangul_Tieut, 0x314c},             /* HANGUL LETTER THIEUTH */
    {XK_Hangul_Phieuf, 0x314d},            /* HANGUL LETTER PHIEUPH */
    {XK_Hangul_Hieuh, 0x314e},             /* HANGUL LETTER HIEUH */
    {XK_Hangul_A, 0x314f},                 /* HANGUL LETTER A */
    {XK_Hangul_AE, 0x3150},                /* HANGUL LETTER AE */
    {XK_Hangul_YA, 0x3151},                /* HANGUL LETTER YA */
    {XK_Hangul_YAE, 0x3152},               /* HANGUL LETTER YAE */
    {XK_Hangul_EO, 0x3153},                /* HANGUL LETTER EO */
    {XK_Hangul_E, 0x3154},                 /* HANGUL LETTER E */
    {XK_Hangul_YEO, 0x3155},               /* HANGUL LETTER YEO */
    {XK_Hangul_YE, 0x3156},                /* HANGUL LETTER YE */
    {XK_Hangul_O, 0x3157},                 /* HANGUL LETTER O */
    {XK_Hangul_WA, 0x3158},                /* HANGUL LETTER WA */
    {XK_Hangul_WAE, 0x3159},               /* HANGUL LETTER WAE */
    {XK_Hangul_OE, 0x315a},                /* HANGUL LETTER OE */
    {XK_Hangul_YO, 0x315b},                /* HANGUL LETTER YO */
    {XK_Hangul_U, 0x315c},                 /* HANGUL LETTER U */
    {XK_Hangul_WEO, 0x315d},               /* HANGUL LETTER WEO */
    {XK_Hangul_WE, 0x315e},                /* HANGUL LETTER WE */
    {XK_Hangul_WI, 0x315f},                /* HANGUL LETTER WI */
    {XK_Hangul_YU, 0x3160},                /* HANGUL LETTER YU */
    {XK_Hangul_EU, 0x3161},                /* HANGUL LETTER EU */
    {XK_Hangul_YI, 0x3162},                /* HANGUL LETTER YI */
    {XK_Hangul_I, 0x3163},                 /* HANGUL LETTER I */
    {XK_Hangul_J_Kiyeog, 0x11a8},          /* HANGUL JONGSEONG KIYEOK */
    {XK_Hangul_J_SsangKiyeog, 0x11a9},     /* HANGUL JONGSEONG SSANGKIYEOK */
    {XK_Hangul_J_KiyeogSios, 0x11aa},      /* HANGUL JONGSEONG KIYEOK-SIOS */
    {XK_Hangul_J_Nieun, 0x11ab},           /* HANGUL JONGSEONG NIEUN */
    {XK_Hangul_J_NieunJieuj, 0x11ac},      /* HANGUL JONGSEONG NIEUN-CIEUC */
    {XK_Hangul_J_NieunHieuh, 0x11ad},      /* HANGUL JONGSEONG NIEUN-HIEUH */
    {XK_Hangul_J_Dikeud, 0x11ae},          /* HANGUL JONGSEONG TIKEUT */
    {XK_Hangul_J_Rieul, 0x11af},           /* HANGUL JONGSEONG RIEUL */
    {XK_Hangul_J_RieulKiyeog, 0x11b0},     /* HANGUL JONGSEONG RIEUL-KIYEOK */
    {XK_Hangul_J_RieulMieum, 0x11b1},      /* HANGUL JONGSEONG RIEUL-MIEUM */
    {XK_Hangul_J_RieulPieub, 0x11b2},      /* HANGUL JONGSEONG RIEUL-PIEUP */
    {XK_Hangul_J_RieulSios, 0x11b3},       /* HANGUL JONGSEONG RIEUL-SIOS */
    {XK_Hangul_J_RieulTieut, 0x11b4},      /* HANGUL JONGSEONG RIEUL-THIEUTH */
    {XK_Hangul_J_RieulPhieuf, 0x11b5},     /* HANGUL JONGSEONG RIEUL-PHIEUPH */
    {XK_Hangul_J_RieulHieuh, 0x11b6},      /* HANGUL JONGSEONG RIEUL-HIEUH */
    {XK_Hangul_J_Mieum, 0x11b7},           /* HANGUL JONGSEONG MIEUM */
    {XK_Hangul_J_Pieub, 0x11b8},           /* HANGUL JONGSEONG PIEUP */
    {XK_Hangul_J_PieubSios, 0x11b9},       /* HANGUL JONGSEONG PIEUP-SIOS */
    {XK_Hangul_J_Sios, 0x11ba},            /* HANGUL JONGSEONG SIOS */
    {XK_Hangul_J_SsangSios, 0x11bb},       /* HANGUL JONGSEONG SSANGSIOS */
    {XK_Hangul_J_Ieung, 0x11bc},           /* HANGUL JONGSEONG IEUNG */
    {XK_Hangul_J_Jieuj, 0x11bd},           /* HANGUL JONGSEONG CIEUC */
    {XK_Hangul_J_Cieuc, 0x11be},           /* HANGUL JONGSEONG CHIEUCH */
    {XK_Hangul_J_Khieuq, 0x11bf},          /* HANGUL JONGSEONG KHIEUKH */
    {XK_Hangul_J_Tieut, 0x11c0},           /* HANGUL JONGSEONG THIEUTH */
    {XK_Hangul_J_Phieuf, 0x11c1},          /* HANGUL JONGSEONG PHIEUPH */
    {XK_Hangul_J_Hieuh, 0x11c2},           /* HANGUL JONGSEONG HIEUH */
    {XK_Hangul_RieulYeorinHieuh, 0x316d},  /* HANGUL LETTER RIEUL-YEORINHIEUH */
    {XK_Hangul_SunkyeongeumMieum, 0x3171}, /* HANGUL LETTER KAPYEOUNMIEUM */
    {XK_Hangul_SunkyeongeumPieub, 0x3178}, /* HANGUL LETTER KAPYEOUNPIEUP */
    {XK_Hangul_PanSios, 0x317f},           /* HANGUL LETTER PANSIOS */
    {XK_Hangul_KkogjiDalrinIeung, 0x3181}, /* HANGUL LETTER YESIEUNG */
    {XK_Hangul_SunkyeongeumPhieuf, 0x3184},  /* HANGUL LETTER KAPYEOUNPHIEUPH */
    {XK_Hangul_YeorinHieuh, 0x3186},         /* HANGUL LETTER YEORINHIEUH */
    {XK_Hangul_AraeA, 0x318d},               /* HANGUL LETTER ARAEA */
    {XK_Hangul_AraeAE, 0x318e},              /* HANGUL LETTER ARAEAE */
    {XK_Hangul_J_PanSios, 0x11eb},           /* HANGUL JONGSEONG PANSIOS */
    {XK_Hangul_J_KkogjiDalrinIeung, 0x11f0}, /* HANGUL JONGSEONG YESIEUNG */
    {XK_Hangul_J_YeorinHieuh, 0x11f9},       /* HANGUL JONGSEONG YEORINHIEUH */
    {XK_Korean_Won, 0x20a9},                 /* WON SIGN */
#endif                                       // defined(XK_Hangul_Kiyeog)
    {XK_OE, 0x0152},                         /* LATIN CAPITAL LIGATURE OE */
    {XK_oe, 0x0153},                         /* LATIN SMALL LIGATURE OE */
    {XK_Ydiaeresis, 0x0178}, /* LATIN CAPITAL LETTER Y WITH DIAERESIS */
    {XK_EuroSign, 0x20ac},   /* EURO SIGN */

    /* combining dead keys */
    {XK_dead_abovedot, 0x0307},    /* COMBINING DOT ABOVE */
    {XK_dead_abovering, 0x030a},   /* COMBINING RING ABOVE */
    {XK_dead_acute, 0x0301},       /* COMBINING ACUTE ACCENT */
    {XK_dead_breve, 0x0306},       /* COMBINING BREVE */
    {XK_dead_caron, 0x030c},       /* COMBINING CARON */
    {XK_dead_cedilla, 0x0327},     /* COMBINING CEDILLA */
    {XK_dead_circumflex, 0x0302},  /* COMBINING CIRCUMFLEX ACCENT */
    {XK_dead_diaeresis, 0x0308},   /* COMBINING DIAERESIS */
    {XK_dead_doubleacute, 0x030b}, /* COMBINING DOUBLE ACUTE ACCENT */
    {XK_dead_grave, 0x0300},       /* COMBINING GRAVE ACCENT */
    {XK_dead_macron, 0x0304},      /* COMBINING MACRON */
    {XK_dead_ogonek, 0x0328},      /* COMBINING OGONEK */
    {XK_dead_tilde, 0x0303}        /* COMBINING TILDE */
};
/* XXX -- map these too
XK_Cyrillic_GHE_bar
XK_Cyrillic_ZHE_descender
XK_Cyrillic_KA_descender
XK_Cyrillic_KA_vertstroke
XK_Cyrillic_EN_descender
XK_Cyrillic_U_straight
XK_Cyrillic_U_straight_bar
XK_Cyrillic_HA_descender
XK_Cyrillic_CHE_descender
XK_Cyrillic_CHE_vertstroke
XK_Cyrillic_SHHA
XK_Cyrillic_SCHWA
XK_Cyrillic_I_macron
XK_Cyrillic_O_bar
XK_Cyrillic_U_macron
XK_Cyrillic_ghe_bar
XK_Cyrillic_zhe_descender
XK_Cyrillic_ka_descender
XK_Cyrillic_ka_vertstroke
XK_Cyrillic_en_descender
XK_Cyrillic_u_straight
XK_Cyrillic_u_straight_bar
XK_Cyrillic_ha_descender
XK_Cyrillic_che_descender
XK_Cyrillic_che_vertstroke
XK_Cyrillic_shha
XK_Cyrillic_schwa
XK_Cyrillic_i_macron
XK_Cyrillic_o_bar
XK_Cyrillic_u_macron

XK_Armenian_eternity
XK_Armenian_ligature_ew
XK_Armenian_full_stop
XK_Armenian_verjaket
XK_Armenian_parenright
XK_Armenian_parenleft
XK_Armenian_guillemotright
XK_Armenian_guillemotleft
XK_Armenian_em_dash
XK_Armenian_dot
XK_Armenian_mijaket
XK_Armenian_but
XK_Armenian_separation_mark
XK_Armenian_comma
XK_Armenian_en_dash
XK_Armenian_hyphen
XK_Armenian_yentamna
XK_Armenian_ellipsis
XK_Armenian_amanak
XK_Armenian_exclam
XK_Armenian_accent
XK_Armenian_shesht
XK_Armenian_paruyk
XK_Armenian_question
XK_Armenian_AYB
XK_Armenian_ayb
XK_Armenian_BEN
XK_Armenian_ben
XK_Armenian_GIM
XK_Armenian_gim
XK_Armenian_DA
XK_Armenian_da
XK_Armenian_YECH
XK_Armenian_yech
XK_Armenian_ZA
XK_Armenian_za
XK_Armenian_E
XK_Armenian_e
XK_Armenian_AT
XK_Armenian_at
XK_Armenian_TO
XK_Armenian_to
XK_Armenian_ZHE
XK_Armenian_zhe
XK_Armenian_INI
XK_Armenian_ini
XK_Armenian_LYUN
XK_Armenian_lyun
XK_Armenian_KHE
XK_Armenian_khe
XK_Armenian_TSA
XK_Armenian_tsa
XK_Armenian_KEN
XK_Armenian_ken
XK_Armenian_HO
XK_Armenian_ho
XK_Armenian_DZA
XK_Armenian_dza
XK_Armenian_GHAT
XK_Armenian_ghat
XK_Armenian_TCHE
XK_Armenian_tche
XK_Armenian_MEN
XK_Armenian_men
XK_Armenian_HI
XK_Armenian_hi
XK_Armenian_NU
XK_Armenian_nu
XK_Armenian_SHA
XK_Armenian_sha
XK_Armenian_VO
XK_Armenian_vo
XK_Armenian_CHA
XK_Armenian_cha
XK_Armenian_PE
XK_Armenian_pe
XK_Armenian_JE
XK_Armenian_je
XK_Armenian_RA
XK_Armenian_ra
XK_Armenian_SE
XK_Armenian_se
XK_Armenian_VEV
XK_Armenian_vev
XK_Armenian_TYUN
XK_Armenian_tyun
XK_Armenian_RE
XK_Armenian_re
XK_Armenian_TSO
XK_Armenian_tso
XK_Armenian_VYUN
XK_Armenian_vyun
XK_Armenian_PYUR
XK_Armenian_pyur
XK_Armenian_KE
XK_Armenian_ke
XK_Armenian_O
XK_Armenian_o
XK_Armenian_FE
XK_Armenian_fe
XK_Armenian_apostrophe
XK_Armenian_section_sign

XK_Georgian_an
XK_Georgian_ban
XK_Georgian_gan
XK_Georgian_don
XK_Georgian_en
XK_Georgian_vin
XK_Georgian_zen
XK_Georgian_tan
XK_Georgian_in
XK_Georgian_kan
XK_Georgian_las
XK_Georgian_man
XK_Georgian_nar
XK_Georgian_on
XK_Georgian_par
XK_Georgian_zhar
XK_Georgian_rae
XK_Georgian_san
XK_Georgian_tar
XK_Georgian_un
XK_Georgian_phar
XK_Georgian_khar
XK_Georgian_ghan
XK_Georgian_qar
XK_Georgian_shin
XK_Georgian_chin
XK_Georgian_can
XK_Georgian_jil
XK_Georgian_cil
XK_Georgian_char
XK_Georgian_xan
XK_Georgian_jhan
XK_Georgian_hae
XK_Georgian_he
XK_Georgian_hie
XK_Georgian_we
XK_Georgian_har
XK_Georgian_hoe
XK_Georgian_fi

XK_Ccedillaabovedot
XK_Xabovedot
XK_Qabovedot
XK_Ibreve
XK_IE
XK_UO
XK_Zstroke
XK_Gcaron
XK_Obarred
XK_ccedillaabovedot
XK_xabovedot
XK_Ocaron
XK_qabovedot
XK_ibreve
XK_ie
XK_uo
XK_zstroke
XK_gcaron
XK_ocaron
XK_obarred
XK_SCHWA
XK_Lbelowdot
XK_Lstrokebelowdot
XK_Gtilde
XK_lbelowdot
XK_lstrokebelowdot
XK_gtilde
XK_schwa

XK_Abelowdot
XK_abelowdot
XK_Ahook
XK_ahook
XK_Acircumflexacute
XK_acircumflexacute
XK_Acircumflexgrave
XK_acircumflexgrave
XK_Acircumflexhook
XK_acircumflexhook
XK_Acircumflextilde
XK_acircumflextilde
XK_Acircumflexbelowdot
XK_acircumflexbelowdot
XK_Abreveacute
XK_abreveacute
XK_Abrevegrave
XK_abrevegrave
XK_Abrevehook
XK_abrevehook
XK_Abrevetilde
XK_abrevetilde
XK_Abrevebelowdot
XK_abrevebelowdot
XK_Ebelowdot
XK_ebelowdot
XK_Ehook
XK_ehook
XK_Etilde
XK_etilde
XK_Ecircumflexacute
XK_ecircumflexacute
XK_Ecircumflexgrave
XK_ecircumflexgrave
XK_Ecircumflexhook
XK_ecircumflexhook
XK_Ecircumflextilde
XK_ecircumflextilde
XK_Ecircumflexbelowdot
XK_ecircumflexbelowdot
XK_Ihook
XK_ihook
XK_Ibelowdot
XK_ibelowdot
XK_Obelowdot
XK_obelowdot
XK_Ohook
XK_ohook
XK_Ocircumflexacute
XK_ocircumflexacute
XK_Ocircumflexgrave
XK_ocircumflexgrave
XK_Ocircumflexhook
XK_ocircumflexhook
XK_Ocircumflextilde
XK_ocircumflextilde
XK_Ocircumflexbelowdot
XK_ocircumflexbelowdot
XK_Ohornacute
XK_ohornacute
XK_Ohorngrave
XK_ohorngrave
XK_Ohornhook
XK_ohornhook
XK_Ohorntilde
XK_ohorntilde
XK_Ohornbelowdot
XK_ohornbelowdot
XK_Ubelowdot
XK_ubelowdot
XK_Uhook
XK_uhook
XK_Uhornacute
XK_uhornacute
XK_Uhorngrave
XK_uhorngrave
XK_Uhornhook
XK_uhornhook
XK_Uhorntilde
XK_uhorntilde
XK_Uhornbelowdot
XK_uhornbelowdot
XK_Ybelowdot
XK_ybelowdot
XK_Yhook
XK_yhook
XK_Ytilde
XK_ytilde
XK_Ohorn
XK_ohorn
XK_Uhorn
XK_uhorn
*/

// map "Internet" keys to KeyIDs
static const KeySym s_map1008FF[] = {
    /* 0x00 */ 0,
    0,
    kKeyBrightnessUp,
    kKeyBrightnessDown,
    0,
    0,
    0,
    0,
    /* 0x08 */ 0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    /* 0x10 */ 0,
    kKeyAudioDown,
    kKeyAudioMute,
    kKeyAudioUp,
    /* 0x14 */ kKeyAudioPlay,
    kKeyAudioStop,
    kKeyAudioPrev,
    kKeyAudioNext,
    /* 0x18 */ kKeyWWWHome,
    kKeyAppMail,
    0,
    kKeyWWWSearch,
    0,
    0,
    0,
    0,
    /* 0x20 */ 0,
    0,
    0,
    0,
    0,
    0,
    kKeyWWWBack,
    kKeyWWWForward,
    /* 0x28 */ kKeyWWWStop,
    kKeyWWWRefresh,
    0,
    0,
    kKeyEject,
    0,
    0,
    0,
    /* 0x30 */ kKeyWWWFavorites,
    0,
    kKeyAppMedia,
    0,
    0,
    0,
    0,
    0,
    /* 0x38 */ 0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    /* 0x40 */ kKeyAppUser1,
    kKeyAppUser2,
    0,
    0,
    0,
    0,
    0,
    0,
    /* 0x48 */ 0,
    0,
    kKeyMissionControl,
    kKeyLaunchpad,
    0,
    0,
    0,
    0,
    /* 0x50 */ 0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    /* 0x58 */ 0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    /* 0x60 */ 0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    /* 0x68 */ 0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    /* 0x70 */ 0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    /* 0x78 */ 0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    /* 0x80 */ 0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    /* 0x88 */ 0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    /* 0x90 */ 0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    /* 0x98 */ 0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    /* 0xa0 */ 0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    /* 0xa8 */ 0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    /* 0xb0 */ 0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    /* 0xb8 */ 0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    /* 0xc0 */ 0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    /* 0xc8 */ 0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    /* 0xd0 */ 0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    /* 0xd8 */ 0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    /* 0xe0 */ 0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    /* 0xe8 */ 0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    /* 0xf0 */ 0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    /* 0xf8 */ 0,
    0,
    0,
    0,
    0,
    0,
    0,
    0};


//
// XWindowsUtil
//

XWindowsUtil::KeySymMap XWindowsUtil::s_keySymToUCS4;

bool
XWindowsUtil::getWindowProperty (Display* display, Window window, Atom property,
                                 String* data, Atom* type, SInt32* format,
                                 bool deleteProperty) {
    assert (display != NULL);

    Atom actualType;
    int actualDatumSize;

    // ignore errors.  XGetWindowProperty() will report failure.
    XWindowsUtil::ErrorLock lock (display);

    // read the property
    bool okay               = true;
    const long length       = XMaxRequestSize (display);
    long offset             = 0;
    unsigned long bytesLeft = 1;
    while (bytesLeft != 0) {
        // get more data
        unsigned long numItems;
        unsigned char* rawData;
        if (XGetWindowProperty (display,
                                window,
                                property,
                                offset,
                                length,
                                False,
                                AnyPropertyType,
                                &actualType,
                                &actualDatumSize,
                                &numItems,
                                &bytesLeft,
                                &rawData) != Success ||
            actualType == None || actualDatumSize == 0) {
            // failed
            okay = false;
            break;
        }

        // compute bytes read and advance offset
        unsigned long numBytes;
        switch (actualDatumSize) {
            case 8:
            default:
                numBytes = numItems;
                offset += numItems / 4;
                break;

            case 16:
                numBytes = 2 * numItems;
                offset += numItems / 2;
                break;

            case 32:
                numBytes = 4 * numItems;
                offset += numItems;
                break;
        }

        // append data
        if (data != NULL) {
            data->append ((char*) rawData, numBytes);
        } else {
            // data is not required so don't try to get any more
            bytesLeft = 0;
        }

        // done with returned data
        XFree (rawData);
    }

    // delete the property if requested
    if (deleteProperty) {
        XDeleteProperty (display, window, property);
    }

    // save property info
    if (type != NULL) {
        *type = actualType;
    }
    if (format != NULL) {
        *format = static_cast<SInt32> (actualDatumSize);
    }

    if (okay) {
        LOG ((CLOG_DEBUG2 "read property %d on window 0x%08x: bytes=%d",
              property,
              window,
              (data == NULL) ? 0 : data->size ()));
        return true;
    } else {
        LOG ((CLOG_DEBUG2 "can't read property %d on window 0x%08x",
              property,
              window));
        return false;
    }
}

bool
XWindowsUtil::setWindowProperty (Display* display, Window window, Atom property,
                                 const void* vdata, UInt32 size, Atom type,
                                 SInt32 format) {
    const UInt32 length       = 4 * XMaxRequestSize (display);
    const unsigned char* data = static_cast<const unsigned char*> (vdata);
    UInt32 datumSize          = static_cast<UInt32> (format / 8);
    // format 32 on 64bit systems is 8 bytes not 4.
    if (format == 32) {
        datumSize = sizeof (Atom);
    }

    // save errors
    bool error = false;
    XWindowsUtil::ErrorLock lock (display, &error);

    // how much data to send in first chunk?
    UInt32 chunkSize = size;
    if (chunkSize > length) {
        chunkSize = length;
    }

    // send first chunk
    XChangeProperty (display,
                     window,
                     property,
                     type,
                     format,
                     PropModeReplace,
                     data,
                     chunkSize / datumSize);

    // append remaining chunks
    data += chunkSize;
    size -= chunkSize;
    while (!error && size > 0) {
        chunkSize = size;
        if (chunkSize > length) {
            chunkSize = length;
        }
        XChangeProperty (display,
                         window,
                         property,
                         type,
                         format,
                         PropModeAppend,
                         data,
                         chunkSize / datumSize);
        data += chunkSize;
        size -= chunkSize;
    }

    return !error;
}

Time
XWindowsUtil::getCurrentTime (Display* display, Window window) {
    XLockDisplay (display);
    // select property events on window
    XWindowAttributes attr;
    XGetWindowAttributes (display, window, &attr);
    XSelectInput (display, window, attr.your_event_mask | PropertyChangeMask);

    // make a property name to receive dummy change
    Atom atom = XInternAtom (display, "TIMESTAMP", False);

    // do a zero-length append to get the current time
    unsigned char dummy;
    XChangeProperty (
        display, window, atom, XA_INTEGER, 8, PropModeAppend, &dummy, 0);

    // look for property notify events with the following
    PropertyNotifyPredicateInfo filter;
    filter.m_window   = window;
    filter.m_property = atom;

    // wait for reply
    XEvent xevent;
    XIfEvent (display,
              &xevent,
              &XWindowsUtil::propertyNotifyPredicate,
              (XPointer) &filter);
    assert (xevent.type == PropertyNotify);
    assert (xevent.xproperty.window == window);
    assert (xevent.xproperty.atom == atom);

    // restore event mask
    XSelectInput (display, window, attr.your_event_mask);
    XUnlockDisplay (display);

    return xevent.xproperty.time;
}

KeyID
XWindowsUtil::mapKeySymToKeyID (KeySym k) {
    initKeyMaps ();

    switch (k & 0xffffff00) {
        case 0x0000:
            // Latin-1
            return static_cast<KeyID> (k);

        case 0xfe00:
            // ISO 9995 Function and Modifier Keys
            switch (k) {
                case XK_ISO_Left_Tab:
                    return kKeyLeftTab;

                case XK_ISO_Level3_Shift:
                    return kKeyAltGr;

#ifdef XK_ISO_Level5_Shift
                case XK_ISO_Level5_Shift:
                    return XK_ISO_Level5_Shift; // FIXME: there is no "usual"
                                                // key for this...
#endif

                case XK_ISO_Next_Group:
                    return kKeyNextGroup;

                case XK_ISO_Prev_Group:
                    return kKeyPrevGroup;

                case XK_dead_grave:
                    return kKeyDeadGrave;

                case XK_dead_acute:
                    return kKeyDeadAcute;

                case XK_dead_circumflex:
                    return kKeyDeadCircumflex;

                case XK_dead_tilde:
                    return kKeyDeadTilde;

                case XK_dead_macron:
                    return kKeyDeadMacron;

                case XK_dead_breve:
                    return kKeyDeadBreve;

                case XK_dead_abovedot:
                    return kKeyDeadAbovedot;

                case XK_dead_diaeresis:
                    return kKeyDeadDiaeresis;

                case XK_dead_abovering:
                    return kKeyDeadAbovering;

                case XK_dead_doubleacute:
                    return kKeyDeadDoubleacute;

                case XK_dead_caron:
                    return kKeyDeadCaron;

                case XK_dead_cedilla:
                    return kKeyDeadCedilla;

                case XK_dead_ogonek:
                    return kKeyDeadOgonek;

                default:
                    return kKeyNone;
            }

        case 0xff00:
            // MISCELLANY
            return static_cast<KeyID> (k - 0xff00 + 0xef00);

        case 0x1008ff00:
            // "Internet" keys
            return s_map1008FF[k & 0xff];

        default: {
            // lookup character in table
            KeySymMap::const_iterator index = s_keySymToUCS4.find (k);
            if (index != s_keySymToUCS4.end ()) {
                return static_cast<KeyID> (index->second);
            }

            // unknown character
            return kKeyNone;
        }
    }
}

UInt32
XWindowsUtil::getModifierBitForKeySym (KeySym keysym) {
    switch (keysym) {
        case XK_Shift_L:
        case XK_Shift_R:
            return kKeyModifierBitShift;

        case XK_Control_L:
        case XK_Control_R:
            return kKeyModifierBitControl;

        case XK_Alt_L:
        case XK_Alt_R:
            return kKeyModifierBitAlt;

        case XK_Meta_L:
        case XK_Meta_R:
            return kKeyModifierBitMeta;

        case XK_Super_L:
        case XK_Super_R:
        case XK_Hyper_L:
        case XK_Hyper_R:
            return kKeyModifierBitSuper;

        case XK_Mode_switch:
        case XK_ISO_Level3_Shift:
            return kKeyModifierBitAltGr;

#ifdef XK_ISO_Level5_Shift
        case XK_ISO_Level5_Shift:
            return kKeyModifierBitLevel5Lock;
#endif

        case XK_Caps_Lock:
            return kKeyModifierBitCapsLock;

        case XK_Num_Lock:
            return kKeyModifierBitNumLock;

        case XK_Scroll_Lock:
            return kKeyModifierBitScrollLock;

        default:
            return kKeyModifierBitNone;
    }
}

String
XWindowsUtil::atomToString (Display* display, Atom atom) {
    if (atom == 0) {
        return "None";
    }

    bool error = false;
    XWindowsUtil::ErrorLock lock (display, &error);
    char* name = XGetAtomName (display, atom);
    if (error) {
        return synergy::string::sprintf ("<UNKNOWN> (%d)", (int) atom);
    } else {
        String msg = synergy::string::sprintf ("%s (%d)", name, (int) atom);
        XFree (name);
        return msg;
    }
}

String
XWindowsUtil::atomsToString (Display* display, const Atom* atom, UInt32 num) {
    char** names = new char*[num];
    bool error   = false;
    XWindowsUtil::ErrorLock lock (display, &error);
    XGetAtomNames (display, const_cast<Atom*> (atom), (int) num, names);
    String msg;
    if (error) {
        for (UInt32 i = 0; i < num; ++i) {
            msg += synergy::string::sprintf ("<UNKNOWN> (%d), ", (int) atom[i]);
        }
    } else {
        for (UInt32 i = 0; i < num; ++i) {
            msg +=
                synergy::string::sprintf ("%s (%d), ", names[i], (int) atom[i]);
            XFree (names[i]);
        }
    }
    delete[] names;
    if (msg.size () > 2) {
        msg.erase (msg.size () - 2);
    }
    return msg;
}

void
XWindowsUtil::convertAtomProperty (String& data) {
    // as best i can tell, 64-bit systems don't pack Atoms into properties
    // as 32-bit numbers but rather as the 64-bit numbers they are.  that
    // seems wrong but we have to cope.  sometimes we'll get a list of
    // atoms that's 8*n+4 bytes long, missing the trailing 4 bytes which
    // should all be 0.  since we're going to reference the Atoms as
    // 64-bit numbers we have to ensure the last number is a full 64 bits.
    if (sizeof (Atom) != 4 && ((data.size () / 4) & 1) != 0) {
        UInt32 zero = 0;
        data.append (reinterpret_cast<char*> (&zero), sizeof (zero));
    }
}

void
XWindowsUtil::appendAtomData (String& data, Atom atom) {
    data.append (reinterpret_cast<char*> (&atom), sizeof (Atom));
}

void
XWindowsUtil::replaceAtomData (String& data, UInt32 index, Atom atom) {
    data.replace (index * sizeof (Atom),
                  sizeof (Atom),
                  reinterpret_cast<const char*> (&atom),
                  sizeof (Atom));
}

void
XWindowsUtil::appendTimeData (String& data, Time time) {
    data.append (reinterpret_cast<char*> (&time), sizeof (Time));
}

Bool
XWindowsUtil::propertyNotifyPredicate (Display*, XEvent* xevent, XPointer arg) {
    PropertyNotifyPredicateInfo* filter =
        reinterpret_cast<PropertyNotifyPredicateInfo*> (arg);
    return (xevent->type == PropertyNotify &&
            xevent->xproperty.window == filter->m_window &&
            xevent->xproperty.atom == filter->m_property &&
            xevent->xproperty.state == PropertyNewValue)
               ? True
               : False;
}

void
XWindowsUtil::initKeyMaps () {
    if (s_keySymToUCS4.empty ()) {
        for (size_t i = 0; i < sizeof (s_keymap) / sizeof (s_keymap[0]); ++i) {
            s_keySymToUCS4[s_keymap[i].keysym] = s_keymap[i].ucs4;
        }
    }
}


//
// XWindowsUtil::ErrorLock
//

XWindowsUtil::ErrorLock* XWindowsUtil::ErrorLock::s_top = NULL;

XWindowsUtil::ErrorLock::ErrorLock (Display* display) : m_display (display) {
    install (&XWindowsUtil::ErrorLock::ignoreHandler, NULL);
}

XWindowsUtil::ErrorLock::ErrorLock (Display* display, bool* flag)
    : m_display (display) {
    install (&XWindowsUtil::ErrorLock::saveHandler, flag);
}

XWindowsUtil::ErrorLock::ErrorLock (Display* display, ErrorHandler handler,
                                    void* data)
    : m_display (display) {
    install (handler, data);
}

XWindowsUtil::ErrorLock::~ErrorLock () {
    // make sure everything finishes before uninstalling handler
    if (m_display != NULL) {
        XSync (m_display, False);
    }

    // restore old handler
    XSetErrorHandler (m_oldXHandler);
    s_top = m_next;
}

void
XWindowsUtil::ErrorLock::install (ErrorHandler handler, void* data) {
    // make sure everything finishes before installing handler
    if (m_display != NULL) {
        XSync (m_display, False);
    }

    // install handler
    m_handler  = handler;
    m_userData = data;
    m_oldXHandler =
        XSetErrorHandler (&XWindowsUtil::ErrorLock::internalHandler);
    m_next = s_top;
    s_top  = this;
}

int
XWindowsUtil::ErrorLock::internalHandler (Display* display,
                                          XErrorEvent* event) {
    if (s_top != NULL && s_top->m_handler != NULL) {
        s_top->m_handler (display, event, s_top->m_userData);
    }
    return 0;
}

void
XWindowsUtil::ErrorLock::ignoreHandler (Display*, XErrorEvent* e, void*) {
    LOG ((CLOG_DEBUG1 "ignoring X error: %d", e->error_code));
}

void
XWindowsUtil::ErrorLock::saveHandler (Display*, XErrorEvent* e, void* flag) {
    LOG ((CLOG_DEBUG1 "flagging X error: %d", e->error_code));
    *static_cast<bool*> (flag) = true;
}
