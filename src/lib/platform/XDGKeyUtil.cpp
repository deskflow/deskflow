/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/XDGKeyUtil.h"

#include "deskflow/KeyTypes.h"

#include <xkbcommon/xkbcommon-keysyms.h>

/*
 * This table maps keysym values into the corresponding ISO 10646
 * (UCS, Unicode) values.
 *
 * The array keysymtab[] contains pairs of XKB values for graphical
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

struct codepair
{
  KeySym keysym;
  std::uint32_t ucs4;
} s_keymap[] = {
    {XKB_KEY_Aogonek, 0x0104},      /* LATIN CAPITAL LETTER A WITH OGONEK */
    {XKB_KEY_breve, 0x02d8},        /* BREVE */
    {XKB_KEY_Lstroke, 0x0141},      /* LATIN CAPITAL LETTER L WITH STROKE */
    {XKB_KEY_Lcaron, 0x013d},       /* LATIN CAPITAL LETTER L WITH CARON */
    {XKB_KEY_Sacute, 0x015a},       /* LATIN CAPITAL LETTER S WITH ACUTE */
    {XKB_KEY_Scaron, 0x0160},       /* LATIN CAPITAL LETTER S WITH CARON */
    {XKB_KEY_Scedilla, 0x015e},     /* LATIN CAPITAL LETTER S WITH CEDILLA */
    {XKB_KEY_Tcaron, 0x0164},       /* LATIN CAPITAL LETTER T WITH CARON */
    {XKB_KEY_Zacute, 0x0179},       /* LATIN CAPITAL LETTER Z WITH ACUTE */
    {XKB_KEY_Zcaron, 0x017d},       /* LATIN CAPITAL LETTER Z WITH CARON */
    {XKB_KEY_Zabovedot, 0x017b},    /* LATIN CAPITAL LETTER Z WITH DOT ABOVE */
    {XKB_KEY_aogonek, 0x0105},      /* LATIN SMALL LETTER A WITH OGONEK */
    {XKB_KEY_ogonek, 0x02db},       /* OGONEK */
    {XKB_KEY_lstroke, 0x0142},      /* LATIN SMALL LETTER L WITH STROKE */
    {XKB_KEY_lcaron, 0x013e},       /* LATIN SMALL LETTER L WITH CARON */
    {XKB_KEY_sacute, 0x015b},       /* LATIN SMALL LETTER S WITH ACUTE */
    {XKB_KEY_caron, 0x02c7},        /* CARON */
    {XKB_KEY_scaron, 0x0161},       /* LATIN SMALL LETTER S WITH CARON */
    {XKB_KEY_scedilla, 0x015f},     /* LATIN SMALL LETTER S WITH CEDILLA */
    {XKB_KEY_tcaron, 0x0165},       /* LATIN SMALL LETTER T WITH CARON */
    {XKB_KEY_zacute, 0x017a},       /* LATIN SMALL LETTER Z WITH ACUTE */
    {XKB_KEY_doubleacute, 0x02dd},  /* DOUBLE ACUTE ACCENT */
    {XKB_KEY_zcaron, 0x017e},       /* LATIN SMALL LETTER Z WITH CARON */
    {XKB_KEY_zabovedot, 0x017c},    /* LATIN SMALL LETTER Z WITH DOT ABOVE */
    {XKB_KEY_Racute, 0x0154},       /* LATIN CAPITAL LETTER R WITH ACUTE */
    {XKB_KEY_Abreve, 0x0102},       /* LATIN CAPITAL LETTER A WITH BREVE */
    {XKB_KEY_Lacute, 0x0139},       /* LATIN CAPITAL LETTER L WITH ACUTE */
    {XKB_KEY_Cacute, 0x0106},       /* LATIN CAPITAL LETTER C WITH ACUTE */
    {XKB_KEY_Ccaron, 0x010c},       /* LATIN CAPITAL LETTER C WITH CARON */
    {XKB_KEY_Eogonek, 0x0118},      /* LATIN CAPITAL LETTER E WITH OGONEK */
    {XKB_KEY_Ecaron, 0x011a},       /* LATIN CAPITAL LETTER E WITH CARON */
    {XKB_KEY_Dcaron, 0x010e},       /* LATIN CAPITAL LETTER D WITH CARON */
    {XKB_KEY_Dstroke, 0x0110},      /* LATIN CAPITAL LETTER D WITH STROKE */
    {XKB_KEY_Nacute, 0x0143},       /* LATIN CAPITAL LETTER N WITH ACUTE */
    {XKB_KEY_Ncaron, 0x0147},       /* LATIN CAPITAL LETTER N WITH CARON */
    {XKB_KEY_Odoubleacute, 0x0150}, /* LATIN CAPITAL LETTER O WITH DOUBLE ACUTE */
    {XKB_KEY_Rcaron, 0x0158},       /* LATIN CAPITAL LETTER R WITH CARON */
    {XKB_KEY_Uring, 0x016e},        /* LATIN CAPITAL LETTER U WITH RING ABOVE */
    {XKB_KEY_Udoubleacute, 0x0170}, /* LATIN CAPITAL LETTER U WITH DOUBLE ACUTE */
    {XKB_KEY_Tcedilla, 0x0162},     /* LATIN CAPITAL LETTER T WITH CEDILLA */
    {XKB_KEY_racute, 0x0155},       /* LATIN SMALL LETTER R WITH ACUTE */
    {XKB_KEY_abreve, 0x0103},       /* LATIN SMALL LETTER A WITH BREVE */
    {XKB_KEY_lacute, 0x013a},       /* LATIN SMALL LETTER L WITH ACUTE */
    {XKB_KEY_cacute, 0x0107},       /* LATIN SMALL LETTER C WITH ACUTE */
    {XKB_KEY_ccaron, 0x010d},       /* LATIN SMALL LETTER C WITH CARON */
    {XKB_KEY_eogonek, 0x0119},      /* LATIN SMALL LETTER E WITH OGONEK */
    {XKB_KEY_ecaron, 0x011b},       /* LATIN SMALL LETTER E WITH CARON */
    {XKB_KEY_dcaron, 0x010f},       /* LATIN SMALL LETTER D WITH CARON */
    {XKB_KEY_dstroke, 0x0111},      /* LATIN SMALL LETTER D WITH STROKE */
    {XKB_KEY_nacute, 0x0144},       /* LATIN SMALL LETTER N WITH ACUTE */
    {XKB_KEY_ncaron, 0x0148},       /* LATIN SMALL LETTER N WITH CARON */
    {XKB_KEY_odoubleacute, 0x0151}, /* LATIN SMALL LETTER O WITH DOUBLE ACUTE */
    {XKB_KEY_rcaron, 0x0159},       /* LATIN SMALL LETTER R WITH CARON */
    {XKB_KEY_uring, 0x016f},        /* LATIN SMALL LETTER U WITH RING ABOVE */
    {XKB_KEY_udoubleacute, 0x0171}, /* LATIN SMALL LETTER U WITH DOUBLE ACUTE */
    {XKB_KEY_tcedilla, 0x0163},     /* LATIN SMALL LETTER T WITH CEDILLA */
    {XKB_KEY_abovedot, 0x02d9},     /* DOT ABOVE */
    {XKB_KEY_Hstroke, 0x0126},      /* LATIN CAPITAL LETTER H WITH STROKE */
    {XKB_KEY_Hcircumflex, 0x0124},  /* LATIN CAPITAL LETTER H WITH CIRCUMFLEX */
    {XKB_KEY_Iabovedot, 0x0130},    /* LATIN CAPITAL LETTER I WITH DOT ABOVE */
    {XKB_KEY_Gbreve, 0x011e},       /* LATIN CAPITAL LETTER G WITH BREVE */
    {XKB_KEY_Jcircumflex, 0x0134},  /* LATIN CAPITAL LETTER J WITH CIRCUMFLEX */
    {XKB_KEY_hstroke, 0x0127},      /* LATIN SMALL LETTER H WITH STROKE */
    {XKB_KEY_hcircumflex, 0x0125},  /* LATIN SMALL LETTER H WITH CIRCUMFLEX */
    {XKB_KEY_idotless, 0x0131},     /* LATIN SMALL LETTER DOTLESS I */
    {XKB_KEY_gbreve, 0x011f},       /* LATIN SMALL LETTER G WITH BREVE */
    {XKB_KEY_jcircumflex, 0x0135},  /* LATIN SMALL LETTER J WITH CIRCUMFLEX */
    {XKB_KEY_Cabovedot, 0x010a},    /* LATIN CAPITAL LETTER C WITH DOT ABOVE */
    {XKB_KEY_Ccircumflex, 0x0108},  /* LATIN CAPITAL LETTER C WITH CIRCUMFLEX */
    {XKB_KEY_Gabovedot, 0x0120},    /* LATIN CAPITAL LETTER G WITH DOT ABOVE */
    {XKB_KEY_Gcircumflex, 0x011c},  /* LATIN CAPITAL LETTER G WITH CIRCUMFLEX */
    {XKB_KEY_Ubreve, 0x016c},       /* LATIN CAPITAL LETTER U WITH BREVE */
    {XKB_KEY_Scircumflex, 0x015c},  /* LATIN CAPITAL LETTER S WITH CIRCUMFLEX */
    {XKB_KEY_cabovedot, 0x010b},    /* LATIN SMALL LETTER C WITH DOT ABOVE */
    {XKB_KEY_ccircumflex, 0x0109},  /* LATIN SMALL LETTER C WITH CIRCUMFLEX */
    {XKB_KEY_gabovedot, 0x0121},    /* LATIN SMALL LETTER G WITH DOT ABOVE */
    {XKB_KEY_gcircumflex, 0x011d},  /* LATIN SMALL LETTER G WITH CIRCUMFLEX */
    {XKB_KEY_ubreve, 0x016d},       /* LATIN SMALL LETTER U WITH BREVE */
    {XKB_KEY_scircumflex, 0x015d},  /* LATIN SMALL LETTER S WITH CIRCUMFLEX */
    {XKB_KEY_kra, 0x0138},          /* LATIN SMALL LETTER KRA */
    {XKB_KEY_Rcedilla, 0x0156},     /* LATIN CAPITAL LETTER R WITH CEDILLA */
    {XKB_KEY_Itilde, 0x0128},       /* LATIN CAPITAL LETTER I WITH TILDE */
    {XKB_KEY_Lcedilla, 0x013b},     /* LATIN CAPITAL LETTER L WITH CEDILLA */
    {XKB_KEY_Emacron, 0x0112},      /* LATIN CAPITAL LETTER E WITH MACRON */
    {XKB_KEY_Gcedilla, 0x0122},     /* LATIN CAPITAL LETTER G WITH CEDILLA */
    {XKB_KEY_Tslash, 0x0166},       /* LATIN CAPITAL LETTER T WITH STROKE */
    {XKB_KEY_rcedilla, 0x0157},     /* LATIN SMALL LETTER R WITH CEDILLA */
    {XKB_KEY_itilde, 0x0129},       /* LATIN SMALL LETTER I WITH TILDE */
    {XKB_KEY_lcedilla, 0x013c},     /* LATIN SMALL LETTER L WITH CEDILLA */
    {XKB_KEY_emacron, 0x0113},      /* LATIN SMALL LETTER E WITH MACRON */
    {XKB_KEY_gcedilla, 0x0123},     /* LATIN SMALL LETTER G WITH CEDILLA */
    {XKB_KEY_tslash, 0x0167},       /* LATIN SMALL LETTER T WITH STROKE */
    {XKB_KEY_ENG, 0x014a},          /* LATIN CAPITAL LETTER ENG */
    {XKB_KEY_eng, 0x014b},          /* LATIN SMALL LETTER ENG */
    {XKB_KEY_Amacron, 0x0100},      /* LATIN CAPITAL LETTER A WITH MACRON */
    {XKB_KEY_Iogonek, 0x012e},      /* LATIN CAPITAL LETTER I WITH OGONEK */
    {XKB_KEY_Eabovedot, 0x0116},    /* LATIN CAPITAL LETTER E WITH DOT ABOVE */
    {XKB_KEY_Imacron, 0x012a},      /* LATIN CAPITAL LETTER I WITH MACRON */
    {XKB_KEY_Ncedilla, 0x0145},     /* LATIN CAPITAL LETTER N WITH CEDILLA */
    {XKB_KEY_Omacron, 0x014c},      /* LATIN CAPITAL LETTER O WITH MACRON */
    {XKB_KEY_Kcedilla, 0x0136},     /* LATIN CAPITAL LETTER K WITH CEDILLA */
    {XKB_KEY_Uogonek, 0x0172},      /* LATIN CAPITAL LETTER U WITH OGONEK */
    {XKB_KEY_Utilde, 0x0168},       /* LATIN CAPITAL LETTER U WITH TILDE */
    {XKB_KEY_Umacron, 0x016a},      /* LATIN CAPITAL LETTER U WITH MACRON */
    {XKB_KEY_amacron, 0x0101},      /* LATIN SMALL LETTER A WITH MACRON */
    {XKB_KEY_iogonek, 0x012f},      /* LATIN SMALL LETTER I WITH OGONEK */
    {XKB_KEY_eabovedot, 0x0117},    /* LATIN SMALL LETTER E WITH DOT ABOVE */
    {XKB_KEY_imacron, 0x012b},      /* LATIN SMALL LETTER I WITH MACRON */
    {XKB_KEY_ncedilla, 0x0146},     /* LATIN SMALL LETTER N WITH CEDILLA */
    {XKB_KEY_omacron, 0x014d},      /* LATIN SMALL LETTER O WITH MACRON */
    {XKB_KEY_kcedilla, 0x0137},     /* LATIN SMALL LETTER K WITH CEDILLA */
    {XKB_KEY_uogonek, 0x0173},      /* LATIN SMALL LETTER U WITH OGONEK */
    {XKB_KEY_utilde, 0x0169},       /* LATIN SMALL LETTER U WITH TILDE */
    {XKB_KEY_umacron, 0x016b},      /* LATIN SMALL LETTER U WITH MACRON */
#if defined(XKB_KEY_Babovedot)
    {XKB_KEY_Babovedot, 0x1e02},   /* LATIN CAPITAL LETTER B WITH DOT ABOVE */
    {XKB_KEY_babovedot, 0x1e03},   /* LATIN SMALL LETTER B WITH DOT ABOVE */
    {XKB_KEY_Dabovedot, 0x1e0a},   /* LATIN CAPITAL LETTER D WITH DOT ABOVE */
    {XKB_KEY_Wgrave, 0x1e80},      /* LATIN CAPITAL LETTER W WITH GRAVE */
    {XKB_KEY_Wacute, 0x1e82},      /* LATIN CAPITAL LETTER W WITH ACUTE */
    {XKB_KEY_dabovedot, 0x1e0b},   /* LATIN SMALL LETTER D WITH DOT ABOVE */
    {XKB_KEY_Ygrave, 0x1ef2},      /* LATIN CAPITAL LETTER Y WITH GRAVE  */
    {XKB_KEY_Fabovedot, 0x1e1e},   /* LATIN CAPITAL LETTER F WITH DOT ABOVE */
    {XKB_KEY_fabovedot, 0x1e1f},   /* LATIN SMALL LETTER F WITH DOT ABOVE */
    {XKB_KEY_Mabovedot, 0x1e40},   /* LATIN CAPITAL LETTER M WITH DOT ABOVE */
    {XKB_KEY_mabovedot, 0x1e41},   /* LATIN SMALL LETTER M WITH DOT ABOVE */
    {XKB_KEY_Pabovedot, 0x1e56},   /* LATIN CAPITAL LETTER P WITH DOT ABOVE */
    {XKB_KEY_wgrave, 0x1e81},      /* LATIN SMALL LETTER W WITH GRAVE  */
    {XKB_KEY_pabovedot, 0x1e57},   /* LATIN SMALL LETTER P WITH DOT ABOVE */
    {XKB_KEY_wacute, 0x1e83},      /* LATIN SMALL LETTER W WITH ACUTE  */
    {XKB_KEY_Sabovedot, 0x1e60},   /* LATIN CAPITAL LETTER S WITH DOT ABOVE */
    {XKB_KEY_ygrave, 0x1ef3},      /* LATIN SMALL LETTER Y WITH GRAVE  */
    {XKB_KEY_Wdiaeresis, 0x1e84},  /* LATIN CAPITAL LETTER W WITH DIAERESIS */
    {XKB_KEY_wdiaeresis, 0x1e85},  /* LATIN SMALL LETTER W WITH DIAERESIS */
    {XKB_KEY_sabovedot, 0x1e61},   /* LATIN SMALL LETTER S WITH DOT ABOVE */
    {XKB_KEY_Wcircumflex, 0x0174}, /* LATIN CAPITAL LETTER W WITH CIRCUMFLEX */
    {XKB_KEY_Tabovedot, 0x1e6a},   /* LATIN CAPITAL LETTER T WITH DOT ABOVE */
    {XKB_KEY_Ycircumflex, 0x0176}, /* LATIN CAPITAL LETTER Y WITH CIRCUMFLEX */
    {XKB_KEY_wcircumflex, 0x0175}, /* LATIN SMALL LETTER W WITH CIRCUMFLEX */
    {XKB_KEY_tabovedot, 0x1e6b},   /* LATIN SMALL LETTER T WITH DOT ABOVE */
    {XKB_KEY_ycircumflex, 0x0177}, /* LATIN SMALL LETTER Y WITH CIRCUMFLEX */
#endif                             // defined(XKB_KEY_Babovedot)
#if defined(XKB_KEY_overline)
    {XKB_KEY_overline, 0x203e},            /* OVERLINE */
    {XKB_KEY_kana_fullstop, 0x3002},       /* IDEOGRAPHIC FULL STOP */
    {XKB_KEY_kana_openingbracket, 0x300c}, /* LEFT CORNER BRACKET */
    {XKB_KEY_kana_closingbracket, 0x300d}, /* RIGHT CORNER BRACKET */
    {XKB_KEY_kana_comma, 0x3001},          /* IDEOGRAPHIC COMMA */
    {XKB_KEY_kana_conjunctive, 0x30fb},    /* KATAKANA MIDDLE DOT */
    {XKB_KEY_kana_WO, 0x30f2},             /* KATAKANA LETTER WO */
    {XKB_KEY_kana_a, 0x30a1},              /* KATAKANA LETTER SMALL A */
    {XKB_KEY_kana_i, 0x30a3},              /* KATAKANA LETTER SMALL I */
    {XKB_KEY_kana_u, 0x30a5},              /* KATAKANA LETTER SMALL U */
    {XKB_KEY_kana_e, 0x30a7},              /* KATAKANA LETTER SMALL E */
    {XKB_KEY_kana_o, 0x30a9},              /* KATAKANA LETTER SMALL O */
    {XKB_KEY_kana_ya, 0x30e3},             /* KATAKANA LETTER SMALL YA */
    {XKB_KEY_kana_yu, 0x30e5},             /* KATAKANA LETTER SMALL YU */
    {XKB_KEY_kana_yo, 0x30e7},             /* KATAKANA LETTER SMALL YO */
    {XKB_KEY_kana_tsu, 0x30c3},            /* KATAKANA LETTER SMALL TU */
    {XKB_KEY_prolongedsound, 0x30fc},      /* KATAKANA-HIRAGANA PROLONGED SOUND MARK */
    {XKB_KEY_kana_A, 0x30a2},              /* KATAKANA LETTER A */
    {XKB_KEY_kana_I, 0x30a4},              /* KATAKANA LETTER I */
    {XKB_KEY_kana_U, 0x30a6},              /* KATAKANA LETTER U */
    {XKB_KEY_kana_E, 0x30a8},              /* KATAKANA LETTER E */
    {XKB_KEY_kana_O, 0x30aa},              /* KATAKANA LETTER O */
    {XKB_KEY_kana_KA, 0x30ab},             /* KATAKANA LETTER KA */
    {XKB_KEY_kana_KI, 0x30ad},             /* KATAKANA LETTER KI */
    {XKB_KEY_kana_KU, 0x30af},             /* KATAKANA LETTER KU */
    {XKB_KEY_kana_KE, 0x30b1},             /* KATAKANA LETTER KE */
    {XKB_KEY_kana_KO, 0x30b3},             /* KATAKANA LETTER KO */
    {XKB_KEY_kana_SA, 0x30b5},             /* KATAKANA LETTER SA */
    {XKB_KEY_kana_SHI, 0x30b7},            /* KATAKANA LETTER SI */
    {XKB_KEY_kana_SU, 0x30b9},             /* KATAKANA LETTER SU */
    {XKB_KEY_kana_SE, 0x30bb},             /* KATAKANA LETTER SE */
    {XKB_KEY_kana_SO, 0x30bd},             /* KATAKANA LETTER SO */
    {XKB_KEY_kana_TA, 0x30bf},             /* KATAKANA LETTER TA */
    {XKB_KEY_kana_CHI, 0x30c1},            /* KATAKANA LETTER TI */
    {XKB_KEY_kana_TSU, 0x30c4},            /* KATAKANA LETTER TU */
    {XKB_KEY_kana_TE, 0x30c6},             /* KATAKANA LETTER TE */
    {XKB_KEY_kana_TO, 0x30c8},             /* KATAKANA LETTER TO */
    {XKB_KEY_kana_NA, 0x30ca},             /* KATAKANA LETTER NA */
    {XKB_KEY_kana_NI, 0x30cb},             /* KATAKANA LETTER NI */
    {XKB_KEY_kana_NU, 0x30cc},             /* KATAKANA LETTER NU */
    {XKB_KEY_kana_NE, 0x30cd},             /* KATAKANA LETTER NE */
    {XKB_KEY_kana_NO, 0x30ce},             /* KATAKANA LETTER NO */
    {XKB_KEY_kana_HA, 0x30cf},             /* KATAKANA LETTER HA */
    {XKB_KEY_kana_HI, 0x30d2},             /* KATAKANA LETTER HI */
    {XKB_KEY_kana_FU, 0x30d5},             /* KATAKANA LETTER HU */
    {XKB_KEY_kana_HE, 0x30d8},             /* KATAKANA LETTER HE */
    {XKB_KEY_kana_HO, 0x30db},             /* KATAKANA LETTER HO */
    {XKB_KEY_kana_MA, 0x30de},             /* KATAKANA LETTER MA */
    {XKB_KEY_kana_MI, 0x30df},             /* KATAKANA LETTER MI */
    {XKB_KEY_kana_MU, 0x30e0},             /* KATAKANA LETTER MU */
    {XKB_KEY_kana_ME, 0x30e1},             /* KATAKANA LETTER ME */
    {XKB_KEY_kana_MO, 0x30e2},             /* KATAKANA LETTER MO */
    {XKB_KEY_kana_YA, 0x30e4},             /* KATAKANA LETTER YA */
    {XKB_KEY_kana_YU, 0x30e6},             /* KATAKANA LETTER YU */
    {XKB_KEY_kana_YO, 0x30e8},             /* KATAKANA LETTER YO */
    {XKB_KEY_kana_RA, 0x30e9},             /* KATAKANA LETTER RA */
    {XKB_KEY_kana_RI, 0x30ea},             /* KATAKANA LETTER RI */
    {XKB_KEY_kana_RU, 0x30eb},             /* KATAKANA LETTER RU */
    {XKB_KEY_kana_RE, 0x30ec},             /* KATAKANA LETTER RE */
    {XKB_KEY_kana_RO, 0x30ed},             /* KATAKANA LETTER RO */
    {XKB_KEY_kana_WA, 0x30ef},             /* KATAKANA LETTER WA */
    {XKB_KEY_kana_N, 0x30f3},              /* KATAKANA LETTER N */
    {XKB_KEY_voicedsound, 0x309b},         /* KATAKANA-HIRAGANA VOICED SOUND MARK */
    {XKB_KEY_semivoicedsound, 0x309c},     /* KATAKANA-HIRAGANA SEMI-VOICED SOUND MARK */
#endif                                     // defined(XKB_KEY_overline)
#if defined(XKB_KEY_Farsi_0)
    {XKB_KEY_Farsi_0, 0x06f0},                 /* EXTENDED ARABIC-INDIC DIGIT 0 */
    {XKB_KEY_Farsi_1, 0x06f1},                 /* EXTENDED ARABIC-INDIC DIGIT 1 */
    {XKB_KEY_Farsi_2, 0x06f2},                 /* EXTENDED ARABIC-INDIC DIGIT 2 */
    {XKB_KEY_Farsi_3, 0x06f3},                 /* EXTENDED ARABIC-INDIC DIGIT 3 */
    {XKB_KEY_Farsi_4, 0x06f4},                 /* EXTENDED ARABIC-INDIC DIGIT 4 */
    {XKB_KEY_Farsi_5, 0x06f5},                 /* EXTENDED ARABIC-INDIC DIGIT 5 */
    {XKB_KEY_Farsi_6, 0x06f6},                 /* EXTENDED ARABIC-INDIC DIGIT 6 */
    {XKB_KEY_Farsi_7, 0x06f7},                 /* EXTENDED ARABIC-INDIC DIGIT 7 */
    {XKB_KEY_Farsi_8, 0x06f8},                 /* EXTENDED ARABIC-INDIC DIGIT 8 */
    {XKB_KEY_Farsi_9, 0x06f9},                 /* EXTENDED ARABIC-INDIC DIGIT 9 */
    {XKB_KEY_Arabic_percent, 0x066a},          /* ARABIC PERCENT */
    {XKB_KEY_Arabic_superscript_alef, 0x0670}, /* ARABIC LETTER SUPERSCRIPT ALEF */
    {XKB_KEY_Arabic_tteh, 0x0679},             /* ARABIC LETTER TTEH */
    {XKB_KEY_Arabic_peh, 0x067e},              /* ARABIC LETTER PEH */
    {XKB_KEY_Arabic_tcheh, 0x0686},            /* ARABIC LETTER TCHEH */
    {XKB_KEY_Arabic_ddal, 0x0688},             /* ARABIC LETTER DDAL */
    {XKB_KEY_Arabic_rreh, 0x0691},             /* ARABIC LETTER RREH */
    {XKB_KEY_Arabic_comma, 0x060c},            /* ARABIC COMMA */
    {XKB_KEY_Arabic_fullstop, 0x06d4},         /* ARABIC FULLSTOP */
    {XKB_KEY_Arabic_semicolon, 0x061b},        /* ARABIC SEMICOLON */
    {XKB_KEY_Arabic_0, 0x0660},                /* ARABIC 0 */
    {XKB_KEY_Arabic_1, 0x0661},                /* ARABIC 1 */
    {XKB_KEY_Arabic_2, 0x0662},                /* ARABIC 2 */
    {XKB_KEY_Arabic_3, 0x0663},                /* ARABIC 3 */
    {XKB_KEY_Arabic_4, 0x0664},                /* ARABIC 4 */
    {XKB_KEY_Arabic_5, 0x0665},                /* ARABIC 5 */
    {XKB_KEY_Arabic_6, 0x0666},                /* ARABIC 6 */
    {XKB_KEY_Arabic_7, 0x0667},                /* ARABIC 7 */
    {XKB_KEY_Arabic_8, 0x0668},                /* ARABIC 8 */
    {XKB_KEY_Arabic_9, 0x0669},                /* ARABIC 9 */
    {XKB_KEY_Arabic_question_mark, 0x061f},    /* ARABIC QUESTION MARK */
    {XKB_KEY_Arabic_hamza, 0x0621},            /* ARABIC LETTER HAMZA */
    {XKB_KEY_Arabic_maddaonalef, 0x0622},      /* ARABIC LETTER ALEF WITH MADDA ABOVE */
    {XKB_KEY_Arabic_hamzaonalef, 0x0623},      /* ARABIC LETTER ALEF WITH HAMZA ABOVE */
    {XKB_KEY_Arabic_hamzaonwaw, 0x0624},       /* ARABIC LETTER WAW WITH HAMZA ABOVE */
    {XKB_KEY_Arabic_hamzaunderalef, 0x0625},   /* ARABIC LETTER ALEF WITH HAMZA BELOW */
    {XKB_KEY_Arabic_hamzaonyeh, 0x0626},       /* ARABIC LETTER YEH WITH HAMZA ABOVE */
    {XKB_KEY_Arabic_alef, 0x0627},             /* ARABIC LETTER ALEF */
    {XKB_KEY_Arabic_beh, 0x0628},              /* ARABIC LETTER BEH */
    {XKB_KEY_Arabic_tehmarbuta, 0x0629},       /* ARABIC LETTER TEH MARBUTA */
    {XKB_KEY_Arabic_teh, 0x062a},              /* ARABIC LETTER TEH */
    {XKB_KEY_Arabic_theh, 0x062b},             /* ARABIC LETTER THEH */
    {XKB_KEY_Arabic_jeem, 0x062c},             /* ARABIC LETTER JEEM */
    {XKB_KEY_Arabic_hah, 0x062d},              /* ARABIC LETTER HAH */
    {XKB_KEY_Arabic_khah, 0x062e},             /* ARABIC LETTER KHAH */
    {XKB_KEY_Arabic_dal, 0x062f},              /* ARABIC LETTER DAL */
    {XKB_KEY_Arabic_thal, 0x0630},             /* ARABIC LETTER THAL */
    {XKB_KEY_Arabic_ra, 0x0631},               /* ARABIC LETTER REH */
    {XKB_KEY_Arabic_zain, 0x0632},             /* ARABIC LETTER ZAIN */
    {XKB_KEY_Arabic_seen, 0x0633},             /* ARABIC LETTER SEEN */
    {XKB_KEY_Arabic_sheen, 0x0634},            /* ARABIC LETTER SHEEN */
    {XKB_KEY_Arabic_sad, 0x0635},              /* ARABIC LETTER SAD */
    {XKB_KEY_Arabic_dad, 0x0636},              /* ARABIC LETTER DAD */
    {XKB_KEY_Arabic_tah, 0x0637},              /* ARABIC LETTER TAH */
    {XKB_KEY_Arabic_zah, 0x0638},              /* ARABIC LETTER ZAH */
    {XKB_KEY_Arabic_ain, 0x0639},              /* ARABIC LETTER AIN */
    {XKB_KEY_Arabic_ghain, 0x063a},            /* ARABIC LETTER GHAIN */
    {XKB_KEY_Arabic_tatweel, 0x0640},          /* ARABIC TATWEEL */
    {XKB_KEY_Arabic_feh, 0x0641},              /* ARABIC LETTER FEH */
    {XKB_KEY_Arabic_qaf, 0x0642},              /* ARABIC LETTER QAF */
    {XKB_KEY_Arabic_kaf, 0x0643},              /* ARABIC LETTER KAF */
    {XKB_KEY_Arabic_lam, 0x0644},              /* ARABIC LETTER LAM */
    {XKB_KEY_Arabic_meem, 0x0645},             /* ARABIC LETTER MEEM */
    {XKB_KEY_Arabic_noon, 0x0646},             /* ARABIC LETTER NOON */
    {XKB_KEY_Arabic_ha, 0x0647},               /* ARABIC LETTER HEH */
    {XKB_KEY_Arabic_waw, 0x0648},              /* ARABIC LETTER WAW */
    {XKB_KEY_Arabic_alefmaksura, 0x0649},      /* ARABIC LETTER ALEF MAKSURA */
    {XKB_KEY_Arabic_yeh, 0x064a},              /* ARABIC LETTER YEH */
    {XKB_KEY_Arabic_fathatan, 0x064b},         /* ARABIC FATHATAN */
    {XKB_KEY_Arabic_dammatan, 0x064c},         /* ARABIC DAMMATAN */
    {XKB_KEY_Arabic_kasratan, 0x064d},         /* ARABIC KASRATAN */
    {XKB_KEY_Arabic_fatha, 0x064e},            /* ARABIC FATHA */
    {XKB_KEY_Arabic_damma, 0x064f},            /* ARABIC DAMMA */
    {XKB_KEY_Arabic_kasra, 0x0650},            /* ARABIC KASRA */
    {XKB_KEY_Arabic_shadda, 0x0651},           /* ARABIC SHADDA */
    {XKB_KEY_Arabic_sukun, 0x0652},            /* ARABIC SUKUN */
    {XKB_KEY_Arabic_madda_above, 0x0653},      /* ARABIC MADDA ABOVE */
    {XKB_KEY_Arabic_hamza_above, 0x0654},      /* ARABIC HAMZA ABOVE */
    {XKB_KEY_Arabic_hamza_below, 0x0655},      /* ARABIC HAMZA BELOW */
    {XKB_KEY_Arabic_jeh, 0x0698},              /* ARABIC LETTER JEH */
    {XKB_KEY_Arabic_veh, 0x06a4},              /* ARABIC LETTER VEH */
    {XKB_KEY_Arabic_keheh, 0x06a9},            /* ARABIC LETTER KEHEH */
    {XKB_KEY_Arabic_gaf, 0x06af},              /* ARABIC LETTER GAF */
    {XKB_KEY_Arabic_noon_ghunna, 0x06ba},      /* ARABIC LETTER NOON GHUNNA */
    {XKB_KEY_Arabic_heh_doachashmee, 0x06be},  /* ARABIC LETTER HEH DOACHASHMEE */
    {XKB_KEY_Arabic_farsi_yeh, 0x06cc},        /* ARABIC LETTER FARSI YEH */
    {XKB_KEY_Arabic_yeh_baree, 0x06d2},        /* ARABIC LETTER YEH BAREE */
    {XKB_KEY_Arabic_heh_goal, 0x06c1},         /* ARABIC LETTER HEH GOAL */
#endif                                         // defined(XKB_KEY_Farsi_0)
#if defined(XKB_KEY_Serbian_dje)
    {XKB_KEY_Serbian_dje, 0x0452},   /* CYRILLIC SMALL LETTER DJE */
    {XKB_KEY_Macedonia_gje, 0x0453}, /* CYRILLIC SMALL LETTER GJE */
    {XKB_KEY_Cyrillic_io, 0x0451},   /* CYRILLIC SMALL LETTER IO */
    {XKB_KEY_Ukrainian_ie, 0x0454},  /* CYRILLIC SMALL LETTER UKRAINIAN IE */
    {XKB_KEY_Macedonia_dse, 0x0455}, /* CYRILLIC SMALL LETTER DZE */
    {XKB_KEY_Ukrainian_i, 0x0456},   /* CYRILLIC SMALL LETTER BYELORUSSIAN-UKRAINIAN I */
    {XKB_KEY_Ukrainian_yi, 0x0457},  /* CYRILLIC SMALL LETTER YI */
    {XKB_KEY_Cyrillic_je, 0x0458},   /* CYRILLIC SMALL LETTER JE */
    {XKB_KEY_Cyrillic_lje, 0x0459},  /* CYRILLIC SMALL LETTER LJE */
    {XKB_KEY_Cyrillic_nje, 0x045a},  /* CYRILLIC SMALL LETTER NJE */
    {XKB_KEY_Serbian_tshe, 0x045b},  /* CYRILLIC SMALL LETTER TSHE */
    {XKB_KEY_Macedonia_kje, 0x045c}, /* CYRILLIC SMALL LETTER KJE */
#if defined(XKB_KEY_Ukrainian_ghe_with_upturn)
    {XKB_KEY_Ukrainian_ghe_with_upturn, 0x0491}, /* CYRILLIC SMALL LETTER GHE WITH UPTURN */
#endif
    {XKB_KEY_Byelorussian_shortu, 0x045e}, /* CYRILLIC SMALL LETTER SHORT U */
    {XKB_KEY_Cyrillic_dzhe, 0x045f},       /* CYRILLIC SMALL LETTER DZHE */
    {XKB_KEY_numerosign, 0x2116},          /* NUMERO SIGN */
    {XKB_KEY_Serbian_DJE, 0x0402},         /* CYRILLIC CAPITAL LETTER DJE */
    {XKB_KEY_Macedonia_GJE, 0x0403},       /* CYRILLIC CAPITAL LETTER GJE */
    {XKB_KEY_Cyrillic_IO, 0x0401},         /* CYRILLIC CAPITAL LETTER IO */
    {XKB_KEY_Ukrainian_IE, 0x0404},        /* CYRILLIC CAPITAL LETTER UKRAINIAN IE */
    {XKB_KEY_Macedonia_DSE, 0x0405},       /* CYRILLIC CAPITAL LETTER DZE */
    {XKB_KEY_Ukrainian_I, 0x0406},         /* CYRILLIC CAPITAL LETTER BYELORUSSIAN-UKRAINIAN I */
    {XKB_KEY_Ukrainian_YI, 0x0407},        /* CYRILLIC CAPITAL LETTER YI */
    {XKB_KEY_Cyrillic_JE, 0x0408},         /* CYRILLIC CAPITAL LETTER JE */
    {XKB_KEY_Cyrillic_LJE, 0x0409},        /* CYRILLIC CAPITAL LETTER LJE */
    {XKB_KEY_Cyrillic_NJE, 0x040a},        /* CYRILLIC CAPITAL LETTER NJE */
    {XKB_KEY_Serbian_TSHE, 0x040b},        /* CYRILLIC CAPITAL LETTER TSHE */
    {XKB_KEY_Macedonia_KJE, 0x040c},       /* CYRILLIC CAPITAL LETTER KJE */
#if defined(XKB_KEY_Ukrainian_GHE_WITH_UPTURN)
    {XKB_KEY_Ukrainian_GHE_WITH_UPTURN, 0x0490}, /* CYRILLIC CAPITAL LETTER GHE WITH UPTURN */
#endif
    {XKB_KEY_Byelorussian_SHORTU, 0x040e}, /* CYRILLIC CAPITAL LETTER SHORT U */
    {XKB_KEY_Cyrillic_DZHE, 0x040f},       /* CYRILLIC CAPITAL LETTER DZHE */
    {XKB_KEY_Cyrillic_yu, 0x044e},         /* CYRILLIC SMALL LETTER YU */
    {XKB_KEY_Cyrillic_a, 0x0430},          /* CYRILLIC SMALL LETTER A */
    {XKB_KEY_Cyrillic_be, 0x0431},         /* CYRILLIC SMALL LETTER BE */
    {XKB_KEY_Cyrillic_tse, 0x0446},        /* CYRILLIC SMALL LETTER TSE */
    {XKB_KEY_Cyrillic_de, 0x0434},         /* CYRILLIC SMALL LETTER DE */
    {XKB_KEY_Cyrillic_ie, 0x0435},         /* CYRILLIC SMALL LETTER IE */
    {XKB_KEY_Cyrillic_ef, 0x0444},         /* CYRILLIC SMALL LETTER EF */
    {XKB_KEY_Cyrillic_ghe, 0x0433},        /* CYRILLIC SMALL LETTER GHE */
    {XKB_KEY_Cyrillic_ha, 0x0445},         /* CYRILLIC SMALL LETTER HA */
    {XKB_KEY_Cyrillic_i, 0x0438},          /* CYRILLIC SMALL LETTER I */
    {XKB_KEY_Cyrillic_shorti, 0x0439},     /* CYRILLIC SMALL LETTER SHORT I */
    {XKB_KEY_Cyrillic_ka, 0x043a},         /* CYRILLIC SMALL LETTER KA */
    {XKB_KEY_Cyrillic_el, 0x043b},         /* CYRILLIC SMALL LETTER EL */
    {XKB_KEY_Cyrillic_em, 0x043c},         /* CYRILLIC SMALL LETTER EM */
    {XKB_KEY_Cyrillic_en, 0x043d},         /* CYRILLIC SMALL LETTER EN */
    {XKB_KEY_Cyrillic_o, 0x043e},          /* CYRILLIC SMALL LETTER O */
    {XKB_KEY_Cyrillic_pe, 0x043f},         /* CYRILLIC SMALL LETTER PE */
    {XKB_KEY_Cyrillic_ya, 0x044f},         /* CYRILLIC SMALL LETTER YA */
    {XKB_KEY_Cyrillic_er, 0x0440},         /* CYRILLIC SMALL LETTER ER */
    {XKB_KEY_Cyrillic_es, 0x0441},         /* CYRILLIC SMALL LETTER ES */
    {XKB_KEY_Cyrillic_te, 0x0442},         /* CYRILLIC SMALL LETTER TE */
    {XKB_KEY_Cyrillic_u, 0x0443},          /* CYRILLIC SMALL LETTER U */
    {XKB_KEY_Cyrillic_zhe, 0x0436},        /* CYRILLIC SMALL LETTER ZHE */
    {XKB_KEY_Cyrillic_ve, 0x0432},         /* CYRILLIC SMALL LETTER VE */
    {XKB_KEY_Cyrillic_softsign, 0x044c},   /* CYRILLIC SMALL LETTER SOFT SIGN */
    {XKB_KEY_Cyrillic_yeru, 0x044b},       /* CYRILLIC SMALL LETTER YERU */
    {XKB_KEY_Cyrillic_ze, 0x0437},         /* CYRILLIC SMALL LETTER ZE */
    {XKB_KEY_Cyrillic_sha, 0x0448},        /* CYRILLIC SMALL LETTER SHA */
    {XKB_KEY_Cyrillic_e, 0x044d},          /* CYRILLIC SMALL LETTER E */
    {XKB_KEY_Cyrillic_shcha, 0x0449},      /* CYRILLIC SMALL LETTER SHCHA */
    {XKB_KEY_Cyrillic_che, 0x0447},        /* CYRILLIC SMALL LETTER CHE */
    {XKB_KEY_Cyrillic_hardsign, 0x044a},   /* CYRILLIC SMALL LETTER HARD SIGN */
    {XKB_KEY_Cyrillic_YU, 0x042e},         /* CYRILLIC CAPITAL LETTER YU */
    {XKB_KEY_Cyrillic_A, 0x0410},          /* CYRILLIC CAPITAL LETTER A */
    {XKB_KEY_Cyrillic_BE, 0x0411},         /* CYRILLIC CAPITAL LETTER BE */
    {XKB_KEY_Cyrillic_TSE, 0x0426},        /* CYRILLIC CAPITAL LETTER TSE */
    {XKB_KEY_Cyrillic_DE, 0x0414},         /* CYRILLIC CAPITAL LETTER DE */
    {XKB_KEY_Cyrillic_IE, 0x0415},         /* CYRILLIC CAPITAL LETTER IE */
    {XKB_KEY_Cyrillic_EF, 0x0424},         /* CYRILLIC CAPITAL LETTER EF */
    {XKB_KEY_Cyrillic_GHE, 0x0413},        /* CYRILLIC CAPITAL LETTER GHE */
    {XKB_KEY_Cyrillic_HA, 0x0425},         /* CYRILLIC CAPITAL LETTER HA */
    {XKB_KEY_Cyrillic_I, 0x0418},          /* CYRILLIC CAPITAL LETTER I */
    {XKB_KEY_Cyrillic_SHORTI, 0x0419},     /* CYRILLIC CAPITAL LETTER SHORT I */
    {XKB_KEY_Cyrillic_KA, 0x041a},         /* CYRILLIC CAPITAL LETTER KA */
    {XKB_KEY_Cyrillic_EL, 0x041b},         /* CYRILLIC CAPITAL LETTER EL */
    {XKB_KEY_Cyrillic_EM, 0x041c},         /* CYRILLIC CAPITAL LETTER EM */
    {XKB_KEY_Cyrillic_EN, 0x041d},         /* CYRILLIC CAPITAL LETTER EN */
    {XKB_KEY_Cyrillic_O, 0x041e},          /* CYRILLIC CAPITAL LETTER O */
    {XKB_KEY_Cyrillic_PE, 0x041f},         /* CYRILLIC CAPITAL LETTER PE */
    {XKB_KEY_Cyrillic_YA, 0x042f},         /* CYRILLIC CAPITAL LETTER YA */
    {XKB_KEY_Cyrillic_ER, 0x0420},         /* CYRILLIC CAPITAL LETTER ER */
    {XKB_KEY_Cyrillic_ES, 0x0421},         /* CYRILLIC CAPITAL LETTER ES */
    {XKB_KEY_Cyrillic_TE, 0x0422},         /* CYRILLIC CAPITAL LETTER TE */
    {XKB_KEY_Cyrillic_U, 0x0423},          /* CYRILLIC CAPITAL LETTER U */
    {XKB_KEY_Cyrillic_ZHE, 0x0416},        /* CYRILLIC CAPITAL LETTER ZHE */
    {XKB_KEY_Cyrillic_VE, 0x0412},         /* CYRILLIC CAPITAL LETTER VE */
    {XKB_KEY_Cyrillic_SOFTSIGN, 0x042c},   /* CYRILLIC CAPITAL LETTER SOFT SIGN */
    {XKB_KEY_Cyrillic_YERU, 0x042b},       /* CYRILLIC CAPITAL LETTER YERU */
    {XKB_KEY_Cyrillic_ZE, 0x0417},         /* CYRILLIC CAPITAL LETTER ZE */
    {XKB_KEY_Cyrillic_SHA, 0x0428},        /* CYRILLIC CAPITAL LETTER SHA */
    {XKB_KEY_Cyrillic_E, 0x042d},          /* CYRILLIC CAPITAL LETTER E */
    {XKB_KEY_Cyrillic_SHCHA, 0x0429},      /* CYRILLIC CAPITAL LETTER SHCHA */
    {XKB_KEY_Cyrillic_CHE, 0x0427},        /* CYRILLIC CAPITAL LETTER CHE */
    {XKB_KEY_Cyrillic_HARDSIGN, 0x042a},   /* CYRILLIC CAPITAL LETTER HARD SIGN */
#endif                                     // defined(XKB_KEY_Serbian_dje)
#if defined(XKB_KEY_Greek_ALPHAaccent)
    {XKB_KEY_Greek_ALPHAaccent, 0x0386},           /* GREEK CAPITAL LETTER ALPHA WITH TONOS */
    {XKB_KEY_Greek_EPSILONaccent, 0x0388},         /* GREEK CAPITAL LETTER EPSILON WITH TONOS */
    {XKB_KEY_Greek_ETAaccent, 0x0389},             /* GREEK CAPITAL LETTER ETA WITH TONOS */
    {XKB_KEY_Greek_IOTAaccent, 0x038a},            /* GREEK CAPITAL LETTER IOTA WITH TONOS */
    {XKB_KEY_Greek_IOTAdiaeresis, 0x03aa},         /* GREEK CAPITAL LETTER IOTA WITH DIALYTIKA */
    {XKB_KEY_Greek_OMICRONaccent, 0x038c},         /* GREEK CAPITAL LETTER OMICRON WITH TONOS */
    {XKB_KEY_Greek_UPSILONaccent, 0x038e},         /* GREEK CAPITAL LETTER UPSILON WITH TONOS */
    {XKB_KEY_Greek_UPSILONdieresis, 0x03ab},       /* GREEK CAPITAL LETTER UPSILON WITH DIALYTIKA */
    {XKB_KEY_Greek_OMEGAaccent, 0x038f},           /* GREEK CAPITAL LETTER OMEGA WITH TONOS */
    {XKB_KEY_Greek_accentdieresis, 0x0385},        /* GREEK DIALYTIKA TONOS */
    {XKB_KEY_Greek_horizbar, 0x2015},              /* HORIZONTAL BAR */
    {XKB_KEY_Greek_alphaaccent, 0x03ac},           /* GREEK SMALL LETTER ALPHA WITH TONOS */
    {XKB_KEY_Greek_epsilonaccent, 0x03ad},         /* GREEK SMALL LETTER EPSILON WITH TONOS */
    {XKB_KEY_Greek_etaaccent, 0x03ae},             /* GREEK SMALL LETTER ETA WITH TONOS */
    {XKB_KEY_Greek_iotaaccent, 0x03af},            /* GREEK SMALL LETTER IOTA WITH TONOS */
    {XKB_KEY_Greek_iotadieresis, 0x03ca},          /* GREEK SMALL LETTER IOTA WITH DIALYTIKA */
    {XKB_KEY_Greek_iotaaccentdieresis, 0x0390},    /* GREEK SMALL LETTER IOTA WITH DIALYTIKA AND TONOS */
    {XKB_KEY_Greek_omicronaccent, 0x03cc},         /* GREEK SMALL LETTER OMICRON WITH TONOS */
    {XKB_KEY_Greek_upsilonaccent, 0x03cd},         /* GREEK SMALL LETTER UPSILON WITH TONOS */
    {XKB_KEY_Greek_upsilondieresis, 0x03cb},       /* GREEK SMALL LETTER UPSILON WITH DIALYTIKA */
    {XKB_KEY_Greek_upsilonaccentdieresis, 0x03b0}, /* GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND TONOS */
    {XKB_KEY_Greek_omegaaccent, 0x03ce},           /* GREEK SMALL LETTER OMEGA WITH TONOS */
    {XKB_KEY_Greek_ALPHA, 0x0391},                 /* GREEK CAPITAL LETTER ALPHA */
    {XKB_KEY_Greek_BETA, 0x0392},                  /* GREEK CAPITAL LETTER BETA */
    {XKB_KEY_Greek_GAMMA, 0x0393},                 /* GREEK CAPITAL LETTER GAMMA */
    {XKB_KEY_Greek_DELTA, 0x0394},                 /* GREEK CAPITAL LETTER DELTA */
    {XKB_KEY_Greek_EPSILON, 0x0395},               /* GREEK CAPITAL LETTER EPSILON */
    {XKB_KEY_Greek_ZETA, 0x0396},                  /* GREEK CAPITAL LETTER ZETA */
    {XKB_KEY_Greek_ETA, 0x0397},                   /* GREEK CAPITAL LETTER ETA */
    {XKB_KEY_Greek_THETA, 0x0398},                 /* GREEK CAPITAL LETTER THETA */
    {XKB_KEY_Greek_IOTA, 0x0399},                  /* GREEK CAPITAL LETTER IOTA */
    {XKB_KEY_Greek_KAPPA, 0x039a},                 /* GREEK CAPITAL LETTER KAPPA */
    {XKB_KEY_Greek_LAMBDA, 0x039b},                /* GREEK CAPITAL LETTER LAMDA */
    {XKB_KEY_Greek_MU, 0x039c},                    /* GREEK CAPITAL LETTER MU */
    {XKB_KEY_Greek_NU, 0x039d},                    /* GREEK CAPITAL LETTER NU */
    {XKB_KEY_Greek_XI, 0x039e},                    /* GREEK CAPITAL LETTER XI */
    {XKB_KEY_Greek_OMICRON, 0x039f},               /* GREEK CAPITAL LETTER OMICRON */
    {XKB_KEY_Greek_PI, 0x03a0},                    /* GREEK CAPITAL LETTER PI */
    {XKB_KEY_Greek_RHO, 0x03a1},                   /* GREEK CAPITAL LETTER RHO */
    {XKB_KEY_Greek_SIGMA, 0x03a3},                 /* GREEK CAPITAL LETTER SIGMA */
    {XKB_KEY_Greek_TAU, 0x03a4},                   /* GREEK CAPITAL LETTER TAU */
    {XKB_KEY_Greek_UPSILON, 0x03a5},               /* GREEK CAPITAL LETTER UPSILON */
    {XKB_KEY_Greek_PHI, 0x03a6},                   /* GREEK CAPITAL LETTER PHI */
    {XKB_KEY_Greek_CHI, 0x03a7},                   /* GREEK CAPITAL LETTER CHI */
    {XKB_KEY_Greek_PSI, 0x03a8},                   /* GREEK CAPITAL LETTER PSI */
    {XKB_KEY_Greek_OMEGA, 0x03a9},                 /* GREEK CAPITAL LETTER OMEGA */
    {XKB_KEY_Greek_alpha, 0x03b1},                 /* GREEK SMALL LETTER ALPHA */
    {XKB_KEY_Greek_beta, 0x03b2},                  /* GREEK SMALL LETTER BETA */
    {XKB_KEY_Greek_gamma, 0x03b3},                 /* GREEK SMALL LETTER GAMMA */
    {XKB_KEY_Greek_delta, 0x03b4},                 /* GREEK SMALL LETTER DELTA */
    {XKB_KEY_Greek_epsilon, 0x03b5},               /* GREEK SMALL LETTER EPSILON */
    {XKB_KEY_Greek_zeta, 0x03b6},                  /* GREEK SMALL LETTER ZETA */
    {XKB_KEY_Greek_eta, 0x03b7},                   /* GREEK SMALL LETTER ETA */
    {XKB_KEY_Greek_theta, 0x03b8},                 /* GREEK SMALL LETTER THETA */
    {XKB_KEY_Greek_iota, 0x03b9},                  /* GREEK SMALL LETTER IOTA */
    {XKB_KEY_Greek_kappa, 0x03ba},                 /* GREEK SMALL LETTER KAPPA */
    {XKB_KEY_Greek_lambda, 0x03bb},                /* GREEK SMALL LETTER LAMDA */
    {XKB_KEY_Greek_mu, 0x03bc},                    /* GREEK SMALL LETTER MU */
    {XKB_KEY_Greek_nu, 0x03bd},                    /* GREEK SMALL LETTER NU */
    {XKB_KEY_Greek_xi, 0x03be},                    /* GREEK SMALL LETTER XI */
    {XKB_KEY_Greek_omicron, 0x03bf},               /* GREEK SMALL LETTER OMICRON */
    {XKB_KEY_Greek_pi, 0x03c0},                    /* GREEK SMALL LETTER PI */
    {XKB_KEY_Greek_rho, 0x03c1},                   /* GREEK SMALL LETTER RHO */
    {XKB_KEY_Greek_sigma, 0x03c3},                 /* GREEK SMALL LETTER SIGMA */
    {XKB_KEY_Greek_finalsmallsigma, 0x03c2},       /* GREEK SMALL LETTER FINAL SIGMA */
    {XKB_KEY_Greek_tau, 0x03c4},                   /* GREEK SMALL LETTER TAU */
    {XKB_KEY_Greek_upsilon, 0x03c5},               /* GREEK SMALL LETTER UPSILON */
    {XKB_KEY_Greek_phi, 0x03c6},                   /* GREEK SMALL LETTER PHI */
    {XKB_KEY_Greek_chi, 0x03c7},                   /* GREEK SMALL LETTER CHI */
    {XKB_KEY_Greek_psi, 0x03c8},                   /* GREEK SMALL LETTER PSI */
    {XKB_KEY_Greek_omega, 0x03c9},                 /* GREEK SMALL LETTER OMEGA */
#endif                                             // defined(XKB_KEY_Greek_ALPHAaccent)
    {XKB_KEY_leftradical, 0x23b7},                 /* ??? */
    {XKB_KEY_topleftradical, 0x250c},              /* BOX DRAWINGS LIGHT DOWN AND RIGHT */
    {XKB_KEY_horizconnector, 0x2500},              /* BOX DRAWINGS LIGHT HORIZONTAL */
    {XKB_KEY_topintegral, 0x2320},                 /* TOP HALF INTEGRAL */
    {XKB_KEY_botintegral, 0x2321},                 /* BOTTOM HALF INTEGRAL */
    {XKB_KEY_vertconnector, 0x2502},               /* BOX DRAWINGS LIGHT VERTICAL */
    {XKB_KEY_topleftsqbracket, 0x23a1},            /* ??? */
    {XKB_KEY_botleftsqbracket, 0x23a3},            /* ??? */
    {XKB_KEY_toprightsqbracket, 0x23a4},           /* ??? */
    {XKB_KEY_botrightsqbracket, 0x23a6},           /* ??? */
    {XKB_KEY_topleftparens, 0x239b},               /* ??? */
    {XKB_KEY_botleftparens, 0x239d},               /* ??? */
    {XKB_KEY_toprightparens, 0x239e},              /* ??? */
    {XKB_KEY_botrightparens, 0x23a0},              /* ??? */
    {XKB_KEY_leftmiddlecurlybrace, 0x23a8},        /* ??? */
    {XKB_KEY_rightmiddlecurlybrace, 0x23ac},       /* ??? */
    {XKB_KEY_lessthanequal, 0x2264},               /* LESS-THAN OR EQUAL TO */
    {XKB_KEY_notequal, 0x2260},                    /* NOT EQUAL TO */
    {XKB_KEY_greaterthanequal, 0x2265},            /* GREATER-THAN OR EQUAL TO */
    {XKB_KEY_integral, 0x222b},                    /* INTEGRAL */
    {XKB_KEY_therefore, 0x2234},                   /* THEREFORE */
    {XKB_KEY_variation, 0x221d},                   /* PROPORTIONAL TO */
    {XKB_KEY_infinity, 0x221e},                    /* INFINITY */
    {XKB_KEY_nabla, 0x2207},                       /* NABLA */
    {XKB_KEY_approximate, 0x223c},                 /* TILDE OPERATOR */
    {XKB_KEY_similarequal, 0x2243},                /* ASYMPTOTICALLY EQUAL TO */
    {XKB_KEY_ifonlyif, 0x21d4},                    /* LEFT RIGHT DOUBLE ARROW */
    {XKB_KEY_implies, 0x21d2},                     /* RIGHTWARDS DOUBLE ARROW */
    {XKB_KEY_identical, 0x2261},                   /* IDENTICAL TO */
    {XKB_KEY_radical, 0x221a},                     /* SQUARE ROOT */
    {XKB_KEY_includedin, 0x2282},                  /* SUBSET OF */
    {XKB_KEY_includes, 0x2283},                    /* SUPERSET OF */
    {XKB_KEY_intersection, 0x2229},                /* INTERSECTION */
    {XKB_KEY_union, 0x222a},                       /* UNION */
    {XKB_KEY_logicaland, 0x2227},                  /* LOGICAL AND */
    {XKB_KEY_logicalor, 0x2228},                   /* LOGICAL OR */
    {XKB_KEY_partialderivative, 0x2202},           /* PARTIAL DIFFERENTIAL */
    {XKB_KEY_function, 0x0192},                    /* LATIN SMALL LETTER F WITH HOOK */
    {XKB_KEY_leftarrow, 0x2190},                   /* LEFTWARDS ARROW */
    {XKB_KEY_uparrow, 0x2191},                     /* UPWARDS ARROW */
    {XKB_KEY_rightarrow, 0x2192},                  /* RIGHTWARDS ARROW */
    {XKB_KEY_downarrow, 0x2193},                   /* DOWNWARDS ARROW */
    /*{ XKB_KEY_blank,                        ??? }, */
    {XKB_KEY_soliddiamond, 0x25c6},   /* BLACK DIAMOND */
    {XKB_KEY_checkerboard, 0x2592},   /* MEDIUM SHADE */
    {XKB_KEY_ht, 0x2409},             /* SYMBOL FOR HORIZONTAL TABULATION */
    {XKB_KEY_ff, 0x240c},             /* SYMBOL FOR FORM FEED */
    {XKB_KEY_cr, 0x240d},             /* SYMBOL FOR CARRIAGE RETURN */
    {XKB_KEY_lf, 0x240a},             /* SYMBOL FOR LINE FEED */
    {XKB_KEY_nl, 0x2424},             /* SYMBOL FOR NEWLINE */
    {XKB_KEY_vt, 0x240b},             /* SYMBOL FOR VERTICAL TABULATION */
    {XKB_KEY_lowrightcorner, 0x2518}, /* BOX DRAWINGS LIGHT UP AND LEFT */
    {XKB_KEY_uprightcorner, 0x2510},  /* BOX DRAWINGS LIGHT DOWN AND LEFT */
    {XKB_KEY_upleftcorner, 0x250c},   /* BOX DRAWINGS LIGHT DOWN AND RIGHT */
    {XKB_KEY_lowleftcorner, 0x2514},  /* BOX DRAWINGS LIGHT UP AND RIGHT */
    {XKB_KEY_crossinglines, 0x253c},  /* BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL */
    {XKB_KEY_horizlinescan1, 0x23ba}, /* HORIZONTAL SCAN LINE-1 (Unicode 3.2 draft) */
    {XKB_KEY_horizlinescan3, 0x23bb}, /* HORIZONTAL SCAN LINE-3 (Unicode 3.2 draft) */
    {XKB_KEY_horizlinescan5, 0x2500}, /* BOX DRAWINGS LIGHT HORIZONTAL */
    {XKB_KEY_horizlinescan7, 0x23bc}, /* HORIZONTAL SCAN LINE-7 (Unicode 3.2 draft) */
    {XKB_KEY_horizlinescan9, 0x23bd}, /* HORIZONTAL SCAN LINE-9 (Unicode 3.2 draft) */
    {XKB_KEY_leftt, 0x251c},          /* BOX DRAWINGS LIGHT VERTICAL AND RIGHT */
    {XKB_KEY_rightt, 0x2524},         /* BOX DRAWINGS LIGHT VERTICAL AND LEFT */
    {XKB_KEY_bott, 0x2534},           /* BOX DRAWINGS LIGHT UP AND HORIZONTAL */
    {XKB_KEY_topt, 0x252c},           /* BOX DRAWINGS LIGHT DOWN AND HORIZONTAL */
    {XKB_KEY_vertbar, 0x2502},        /* BOX DRAWINGS LIGHT VERTICAL */
    {XKB_KEY_emspace, 0x2003},        /* EM SPACE */
    {XKB_KEY_enspace, 0x2002},        /* EN SPACE */
    {XKB_KEY_em3space, 0x2004},       /* THREE-PER-EM SPACE */
    {XKB_KEY_em4space, 0x2005},       /* FOUR-PER-EM SPACE */
    {XKB_KEY_digitspace, 0x2007},     /* FIGURE SPACE */
    {XKB_KEY_punctspace, 0x2008},     /* PUNCTUATION SPACE */
    {XKB_KEY_thinspace, 0x2009},      /* THIN SPACE */
    {XKB_KEY_hairspace, 0x200a},      /* HAIR SPACE */
    {XKB_KEY_emdash, 0x2014},         /* EM DASH */
    {XKB_KEY_endash, 0x2013},         /* EN DASH */
    /*{ XKB_KEY_signifblank,                  ??? }, */
    {XKB_KEY_ellipsis, 0x2026},         /* HORIZONTAL ELLIPSIS */
    {XKB_KEY_doubbaselinedot, 0x2025},  /* TWO DOT LEADER */
    {XKB_KEY_onethird, 0x2153},         /* VULGAR FRACTION ONE THIRD */
    {XKB_KEY_twothirds, 0x2154},        /* VULGAR FRACTION TWO THIRDS */
    {XKB_KEY_onefifth, 0x2155},         /* VULGAR FRACTION ONE FIFTH */
    {XKB_KEY_twofifths, 0x2156},        /* VULGAR FRACTION TWO FIFTHS */
    {XKB_KEY_threefifths, 0x2157},      /* VULGAR FRACTION THREE FIFTHS */
    {XKB_KEY_fourfifths, 0x2158},       /* VULGAR FRACTION FOUR FIFTHS */
    {XKB_KEY_onesixth, 0x2159},         /* VULGAR FRACTION ONE SIXTH */
    {XKB_KEY_fivesixths, 0x215a},       /* VULGAR FRACTION FIVE SIXTHS */
    {XKB_KEY_careof, 0x2105},           /* CARE OF */
    {XKB_KEY_figdash, 0x2012},          /* FIGURE DASH */
    {XKB_KEY_leftanglebracket, 0x2329}, /* LEFT-POINTING ANGLE BRACKET */
    /*{ XKB_KEY_decimalpoint,                 ??? }, */
    {XKB_KEY_rightanglebracket, 0x232a}, /* RIGHT-POINTING ANGLE BRACKET */
    /*{ XKB_KEY_marker,                       ??? }, */
    {XKB_KEY_oneeighth, 0x215b},     /* VULGAR FRACTION ONE EIGHTH */
    {XKB_KEY_threeeighths, 0x215c},  /* VULGAR FRACTION THREE EIGHTHS */
    {XKB_KEY_fiveeighths, 0x215d},   /* VULGAR FRACTION FIVE EIGHTHS */
    {XKB_KEY_seveneighths, 0x215e},  /* VULGAR FRACTION SEVEN EIGHTHS */
    {XKB_KEY_trademark, 0x2122},     /* TRADE MARK SIGN */
    {XKB_KEY_signaturemark, 0x2613}, /* SALTIRE */
    /*{ XKB_KEY_trademarkincircle,            ??? }, */
    {XKB_KEY_leftopentriangle, 0x25c1},     /* WHITE LEFT-POINTING TRIANGLE */
    {XKB_KEY_rightopentriangle, 0x25b7},    /* WHITE RIGHT-POINTING TRIANGLE */
    {XKB_KEY_emopencircle, 0x25cb},         /* WHITE CIRCLE */
    {XKB_KEY_emopenrectangle, 0x25af},      /* WHITE VERTICAL RECTANGLE */
    {XKB_KEY_leftsinglequotemark, 0x2018},  /* LEFT SINGLE QUOTATION MARK */
    {XKB_KEY_rightsinglequotemark, 0x2019}, /* RIGHT SINGLE QUOTATION MARK */
    {XKB_KEY_leftdoublequotemark, 0x201c},  /* LEFT DOUBLE QUOTATION MARK */
    {XKB_KEY_rightdoublequotemark, 0x201d}, /* RIGHT DOUBLE QUOTATION MARK */
    {XKB_KEY_prescription, 0x211e},         /* PRESCRIPTION TAKE */
    {XKB_KEY_minutes, 0x2032},              /* PRIME */
    {XKB_KEY_seconds, 0x2033},              /* DOUBLE PRIME */
    {XKB_KEY_latincross, 0x271d},           /* LATIN CROSS */
    /*{ XKB_KEY_hexagram,                     ??? }, */
    {XKB_KEY_filledrectbullet, 0x25ac},     /* BLACK RECTANGLE */
    {XKB_KEY_filledlefttribullet, 0x25c0},  /* BLACK LEFT-POINTING TRIANGLE */
    {XKB_KEY_filledrighttribullet, 0x25b6}, /* BLACK RIGHT-POINTING TRIANGLE */
    {XKB_KEY_emfilledcircle, 0x25cf},       /* BLACK CIRCLE */
    {XKB_KEY_emfilledrect, 0x25ae},         /* BLACK VERTICAL RECTANGLE */
    {XKB_KEY_enopencircbullet, 0x25e6},     /* WHITE BULLET */
    {XKB_KEY_enopensquarebullet, 0x25ab},   /* WHITE SMALL SQUARE */
    {XKB_KEY_openrectbullet, 0x25ad},       /* WHITE RECTANGLE */
    {XKB_KEY_opentribulletup, 0x25b3},      /* WHITE UP-POINTING TRIANGLE */
    {XKB_KEY_opentribulletdown, 0x25bd},    /* WHITE DOWN-POINTING TRIANGLE */
    {XKB_KEY_openstar, 0x2606},             /* WHITE STAR */
    {XKB_KEY_enfilledcircbullet, 0x2022},   /* BULLET */
    {XKB_KEY_enfilledsqbullet, 0x25aa},     /* BLACK SMALL SQUARE */
    {XKB_KEY_filledtribulletup, 0x25b2},    /* BLACK UP-POINTING TRIANGLE */
    {XKB_KEY_filledtribulletdown, 0x25bc},  /* BLACK DOWN-POINTING TRIANGLE */
    {XKB_KEY_leftpointer, 0x261c},          /* WHITE LEFT POINTING INDEX */
    {XKB_KEY_rightpointer, 0x261e},         /* WHITE RIGHT POINTING INDEX */
    {XKB_KEY_club, 0x2663},                 /* BLACK CLUB SUIT */
    {XKB_KEY_diamond, 0x2666},              /* BLACK DIAMOND SUIT */
    {XKB_KEY_heart, 0x2665},                /* BLACK HEART SUIT */
    {XKB_KEY_maltesecross, 0x2720},         /* MALTESE CROSS */
    {XKB_KEY_dagger, 0x2020},               /* DAGGER */
    {XKB_KEY_doubledagger, 0x2021},         /* DOUBLE DAGGER */
    {XKB_KEY_checkmark, 0x2713},            /* CHECK MARK */
    {XKB_KEY_ballotcross, 0x2717},          /* BALLOT X */
    {XKB_KEY_musicalsharp, 0x266f},         /* MUSIC SHARP SIGN */
    {XKB_KEY_musicalflat, 0x266d},          /* MUSIC FLAT SIGN */
    {XKB_KEY_malesymbol, 0x2642},           /* MALE SIGN */
    {XKB_KEY_femalesymbol, 0x2640},         /* FEMALE SIGN */
    {XKB_KEY_telephone, 0x260e},            /* BLACK TELEPHONE */
    {XKB_KEY_telephonerecorder, 0x2315},    /* TELEPHONE RECORDER */
    {XKB_KEY_phonographcopyright, 0x2117},  /* SOUND RECORDING COPYRIGHT */
    {XKB_KEY_caret, 0x2038},                /* CARET */
    {XKB_KEY_singlelowquotemark, 0x201a},   /* SINGLE LOW-9 QUOTATION MARK */
    {XKB_KEY_doublelowquotemark, 0x201e},   /* DOUBLE LOW-9 QUOTATION MARK */
    /*{ XKB_KEY_cursor,                       ??? }, */
    {XKB_KEY_leftcaret, 0x003c},  /* LESS-THAN SIGN */
    {XKB_KEY_rightcaret, 0x003e}, /* GREATER-THAN SIGN */
    {XKB_KEY_downcaret, 0x2228},  /* LOGICAL OR */
    {XKB_KEY_upcaret, 0x2227},    /* LOGICAL AND */
    {XKB_KEY_overbar, 0x00af},    /* MACRON */
    {XKB_KEY_downtack, 0x22a5},   /* UP TACK */
    {XKB_KEY_upshoe, 0x2229},     /* INTERSECTION */
    {XKB_KEY_downstile, 0x230a},  /* LEFT FLOOR */
    {XKB_KEY_underbar, 0x005f},   /* LOW LINE */
    {XKB_KEY_jot, 0x2218},        /* RING OPERATOR */
    {XKB_KEY_quad, 0x2395},       /* APL FUNCTIONAL SYMBOL QUAD */
    {XKB_KEY_uptack, 0x22a4},     /* DOWN TACK */
    {XKB_KEY_circle, 0x25cb},     /* WHITE CIRCLE */
    {XKB_KEY_upstile, 0x2308},    /* LEFT CEILING */
    {XKB_KEY_downshoe, 0x222a},   /* UNION */
    {XKB_KEY_rightshoe, 0x2283},  /* SUPERSET OF */
    {XKB_KEY_leftshoe, 0x2282},   /* SUBSET OF */
    {XKB_KEY_lefttack, 0x22a2},   /* RIGHT TACK */
    {XKB_KEY_righttack, 0x22a3},  /* LEFT TACK */
#if defined(XKB_KEY_hebrew_doublelowline)
    {XKB_KEY_hebrew_doublelowline, 0x2017}, /* DOUBLE LOW LINE */
    {XKB_KEY_hebrew_aleph, 0x05d0},         /* HEBREW LETTER ALEF */
    {XKB_KEY_hebrew_bet, 0x05d1},           /* HEBREW LETTER BET */
    {XKB_KEY_hebrew_gimel, 0x05d2},         /* HEBREW LETTER GIMEL */
    {XKB_KEY_hebrew_dalet, 0x05d3},         /* HEBREW LETTER DALET */
    {XKB_KEY_hebrew_he, 0x05d4},            /* HEBREW LETTER HE */
    {XKB_KEY_hebrew_waw, 0x05d5},           /* HEBREW LETTER VAV */
    {XKB_KEY_hebrew_zain, 0x05d6},          /* HEBREW LETTER ZAYIN */
    {XKB_KEY_hebrew_chet, 0x05d7},          /* HEBREW LETTER HET */
    {XKB_KEY_hebrew_tet, 0x05d8},           /* HEBREW LETTER TET */
    {XKB_KEY_hebrew_yod, 0x05d9},           /* HEBREW LETTER YOD */
    {XKB_KEY_hebrew_finalkaph, 0x05da},     /* HEBREW LETTER FINAL KAF */
    {XKB_KEY_hebrew_kaph, 0x05db},          /* HEBREW LETTER KAF */
    {XKB_KEY_hebrew_lamed, 0x05dc},         /* HEBREW LETTER LAMED */
    {XKB_KEY_hebrew_finalmem, 0x05dd},      /* HEBREW LETTER FINAL MEM */
    {XKB_KEY_hebrew_mem, 0x05de},           /* HEBREW LETTER MEM */
    {XKB_KEY_hebrew_finalnun, 0x05df},      /* HEBREW LETTER FINAL NUN */
    {XKB_KEY_hebrew_nun, 0x05e0},           /* HEBREW LETTER NUN */
    {XKB_KEY_hebrew_samech, 0x05e1},        /* HEBREW LETTER SAMEKH */
    {XKB_KEY_hebrew_ayin, 0x05e2},          /* HEBREW LETTER AYIN */
    {XKB_KEY_hebrew_finalpe, 0x05e3},       /* HEBREW LETTER FINAL PE */
    {XKB_KEY_hebrew_pe, 0x05e4},            /* HEBREW LETTER PE */
    {XKB_KEY_hebrew_finalzade, 0x05e5},     /* HEBREW LETTER FINAL TSADI */
    {XKB_KEY_hebrew_zade, 0x05e6},          /* HEBREW LETTER TSADI */
    {XKB_KEY_hebrew_qoph, 0x05e7},          /* HEBREW LETTER QOF */
    {XKB_KEY_hebrew_resh, 0x05e8},          /* HEBREW LETTER RESH */
    {XKB_KEY_hebrew_shin, 0x05e9},          /* HEBREW LETTER SHIN */
    {XKB_KEY_hebrew_taw, 0x05ea},           /* HEBREW LETTER TAV */
#endif                                      // defined(XKB_KEY_hebrew_doublelowline)
#if defined(XKB_KEY_Thai_kokai)
    {XKB_KEY_Thai_kokai, 0x0e01},         /* THAI CHARACTER KO KAI */
    {XKB_KEY_Thai_khokhai, 0x0e02},       /* THAI CHARACTER KHO KHAI */
    {XKB_KEY_Thai_khokhuat, 0x0e03},      /* THAI CHARACTER KHO KHUAT */
    {XKB_KEY_Thai_khokhwai, 0x0e04},      /* THAI CHARACTER KHO KHWAI */
    {XKB_KEY_Thai_khokhon, 0x0e05},       /* THAI CHARACTER KHO KHON */
    {XKB_KEY_Thai_khorakhang, 0x0e06},    /* THAI CHARACTER KHO RAKHANG */
    {XKB_KEY_Thai_ngongu, 0x0e07},        /* THAI CHARACTER NGO NGU */
    {XKB_KEY_Thai_chochan, 0x0e08},       /* THAI CHARACTER CHO CHAN */
    {XKB_KEY_Thai_choching, 0x0e09},      /* THAI CHARACTER CHO CHING */
    {XKB_KEY_Thai_chochang, 0x0e0a},      /* THAI CHARACTER CHO CHANG */
    {XKB_KEY_Thai_soso, 0x0e0b},          /* THAI CHARACTER SO SO */
    {XKB_KEY_Thai_chochoe, 0x0e0c},       /* THAI CHARACTER CHO CHOE */
    {XKB_KEY_Thai_yoying, 0x0e0d},        /* THAI CHARACTER YO YING */
    {XKB_KEY_Thai_dochada, 0x0e0e},       /* THAI CHARACTER DO CHADA */
    {XKB_KEY_Thai_topatak, 0x0e0f},       /* THAI CHARACTER TO PATAK */
    {XKB_KEY_Thai_thothan, 0x0e10},       /* THAI CHARACTER THO THAN */
    {XKB_KEY_Thai_thonangmontho, 0x0e11}, /* THAI CHARACTER THO NANGMONTHO */
    {XKB_KEY_Thai_thophuthao, 0x0e12},    /* THAI CHARACTER THO PHUTHAO */
    {XKB_KEY_Thai_nonen, 0x0e13},         /* THAI CHARACTER NO NEN */
    {XKB_KEY_Thai_dodek, 0x0e14},         /* THAI CHARACTER DO DEK */
    {XKB_KEY_Thai_totao, 0x0e15},         /* THAI CHARACTER TO TAO */
    {XKB_KEY_Thai_thothung, 0x0e16},      /* THAI CHARACTER THO THUNG */
    {XKB_KEY_Thai_thothahan, 0x0e17},     /* THAI CHARACTER THO THAHAN */
    {XKB_KEY_Thai_thothong, 0x0e18},      /* THAI CHARACTER THO THONG */
    {XKB_KEY_Thai_nonu, 0x0e19},          /* THAI CHARACTER NO NU */
    {XKB_KEY_Thai_bobaimai, 0x0e1a},      /* THAI CHARACTER BO BAIMAI */
    {XKB_KEY_Thai_popla, 0x0e1b},         /* THAI CHARACTER PO PLA */
    {XKB_KEY_Thai_phophung, 0x0e1c},      /* THAI CHARACTER PHO PHUNG */
    {XKB_KEY_Thai_fofa, 0x0e1d},          /* THAI CHARACTER FO FA */
    {XKB_KEY_Thai_phophan, 0x0e1e},       /* THAI CHARACTER PHO PHAN */
    {XKB_KEY_Thai_fofan, 0x0e1f},         /* THAI CHARACTER FO FAN */
    {XKB_KEY_Thai_phosamphao, 0x0e20},    /* THAI CHARACTER PHO SAMPHAO */
    {XKB_KEY_Thai_moma, 0x0e21},          /* THAI CHARACTER MO MA */
    {XKB_KEY_Thai_yoyak, 0x0e22},         /* THAI CHARACTER YO YAK */
    {XKB_KEY_Thai_rorua, 0x0e23},         /* THAI CHARACTER RO RUA */
    {XKB_KEY_Thai_ru, 0x0e24},            /* THAI CHARACTER RU */
    {XKB_KEY_Thai_loling, 0x0e25},        /* THAI CHARACTER LO LING */
    {XKB_KEY_Thai_lu, 0x0e26},            /* THAI CHARACTER LU */
    {XKB_KEY_Thai_wowaen, 0x0e27},        /* THAI CHARACTER WO WAEN */
    {XKB_KEY_Thai_sosala, 0x0e28},        /* THAI CHARACTER SO SALA */
    {XKB_KEY_Thai_sorusi, 0x0e29},        /* THAI CHARACTER SO RUSI */
    {XKB_KEY_Thai_sosua, 0x0e2a},         /* THAI CHARACTER SO SUA */
    {XKB_KEY_Thai_hohip, 0x0e2b},         /* THAI CHARACTER HO HIP */
    {XKB_KEY_Thai_lochula, 0x0e2c},       /* THAI CHARACTER LO CHULA */
    {XKB_KEY_Thai_oang, 0x0e2d},          /* THAI CHARACTER O ANG */
    {XKB_KEY_Thai_honokhuk, 0x0e2e},      /* THAI CHARACTER HO NOKHUK */
    {XKB_KEY_Thai_paiyannoi, 0x0e2f},     /* THAI CHARACTER PAIYANNOI */
    {XKB_KEY_Thai_saraa, 0x0e30},         /* THAI CHARACTER SARA A */
    {XKB_KEY_Thai_maihanakat, 0x0e31},    /* THAI CHARACTER MAI HAN-AKAT */
    {XKB_KEY_Thai_saraaa, 0x0e32},        /* THAI CHARACTER SARA AA */
    {XKB_KEY_Thai_saraam, 0x0e33},        /* THAI CHARACTER SARA AM */
    {XKB_KEY_Thai_sarai, 0x0e34},         /* THAI CHARACTER SARA I */
    {XKB_KEY_Thai_saraii, 0x0e35},        /* THAI CHARACTER SARA II */
    {XKB_KEY_Thai_saraue, 0x0e36},        /* THAI CHARACTER SARA UE */
    {XKB_KEY_Thai_sarauee, 0x0e37},       /* THAI CHARACTER SARA UEE */
    {XKB_KEY_Thai_sarau, 0x0e38},         /* THAI CHARACTER SARA U */
    {XKB_KEY_Thai_sarauu, 0x0e39},        /* THAI CHARACTER SARA UU */
    {XKB_KEY_Thai_phinthu, 0x0e3a},       /* THAI CHARACTER PHINTHU */
    /*{ XKB_KEY_Thai_maihanakat_maitho,       ??? }, */
    {XKB_KEY_Thai_baht, 0x0e3f},           /* THAI CURRENCY SYMBOL BAHT */
    {XKB_KEY_Thai_sarae, 0x0e40},          /* THAI CHARACTER SARA E */
    {XKB_KEY_Thai_saraae, 0x0e41},         /* THAI CHARACTER SARA AE */
    {XKB_KEY_Thai_sarao, 0x0e42},          /* THAI CHARACTER SARA O */
    {XKB_KEY_Thai_saraaimaimuan, 0x0e43},  /* THAI CHARACTER SARA AI MAIMUAN */
    {XKB_KEY_Thai_saraaimaimalai, 0x0e44}, /* THAI CHARACTER SARA AI MAIMALAI */
    {XKB_KEY_Thai_lakkhangyao, 0x0e45},    /* THAI CHARACTER LAKKHANGYAO */
    {XKB_KEY_Thai_maiyamok, 0x0e46},       /* THAI CHARACTER MAIYAMOK */
    {XKB_KEY_Thai_maitaikhu, 0x0e47},      /* THAI CHARACTER MAITAIKHU */
    {XKB_KEY_Thai_maiek, 0x0e48},          /* THAI CHARACTER MAI EK */
    {XKB_KEY_Thai_maitho, 0x0e49},         /* THAI CHARACTER MAI THO */
    {XKB_KEY_Thai_maitri, 0x0e4a},         /* THAI CHARACTER MAI TRI */
    {XKB_KEY_Thai_maichattawa, 0x0e4b},    /* THAI CHARACTER MAI CHATTAWA */
    {XKB_KEY_Thai_thanthakhat, 0x0e4c},    /* THAI CHARACTER THANTHAKHAT */
    {XKB_KEY_Thai_nikhahit, 0x0e4d},       /* THAI CHARACTER NIKHAHIT */
    {XKB_KEY_Thai_leksun, 0x0e50},         /* THAI DIGIT ZERO */
    {XKB_KEY_Thai_leknung, 0x0e51},        /* THAI DIGIT ONE */
    {XKB_KEY_Thai_leksong, 0x0e52},        /* THAI DIGIT TWO */
    {XKB_KEY_Thai_leksam, 0x0e53},         /* THAI DIGIT THREE */
    {XKB_KEY_Thai_leksi, 0x0e54},          /* THAI DIGIT FOUR */
    {XKB_KEY_Thai_lekha, 0x0e55},          /* THAI DIGIT FIVE */
    {XKB_KEY_Thai_lekhok, 0x0e56},         /* THAI DIGIT SIX */
    {XKB_KEY_Thai_lekchet, 0x0e57},        /* THAI DIGIT SEVEN */
    {XKB_KEY_Thai_lekpaet, 0x0e58},        /* THAI DIGIT EIGHT */
    {XKB_KEY_Thai_lekkao, 0x0e59},         /* THAI DIGIT NINE */
#endif                                     // defined(XKB_KEY_Thai_kokai)
#if defined(XKB_KEY_Hangul_Kiyeog)
    {XKB_KEY_Hangul_Kiyeog, 0x3131},              /* HANGUL LETTER KIYEOK */
    {XKB_KEY_Hangul_SsangKiyeog, 0x3132},         /* HANGUL LETTER SSANGKIYEOK */
    {XKB_KEY_Hangul_KiyeogSios, 0x3133},          /* HANGUL LETTER KIYEOK-SIOS */
    {XKB_KEY_Hangul_Nieun, 0x3134},               /* HANGUL LETTER NIEUN */
    {XKB_KEY_Hangul_NieunJieuj, 0x3135},          /* HANGUL LETTER NIEUN-CIEUC */
    {XKB_KEY_Hangul_NieunHieuh, 0x3136},          /* HANGUL LETTER NIEUN-HIEUH */
    {XKB_KEY_Hangul_Dikeud, 0x3137},              /* HANGUL LETTER TIKEUT */
    {XKB_KEY_Hangul_SsangDikeud, 0x3138},         /* HANGUL LETTER SSANGTIKEUT */
    {XKB_KEY_Hangul_Rieul, 0x3139},               /* HANGUL LETTER RIEUL */
    {XKB_KEY_Hangul_RieulKiyeog, 0x313a},         /* HANGUL LETTER RIEUL-KIYEOK */
    {XKB_KEY_Hangul_RieulMieum, 0x313b},          /* HANGUL LETTER RIEUL-MIEUM */
    {XKB_KEY_Hangul_RieulPieub, 0x313c},          /* HANGUL LETTER RIEUL-PIEUP */
    {XKB_KEY_Hangul_RieulSios, 0x313d},           /* HANGUL LETTER RIEUL-SIOS */
    {XKB_KEY_Hangul_RieulTieut, 0x313e},          /* HANGUL LETTER RIEUL-THIEUTH */
    {XKB_KEY_Hangul_RieulPhieuf, 0x313f},         /* HANGUL LETTER RIEUL-PHIEUPH */
    {XKB_KEY_Hangul_RieulHieuh, 0x3140},          /* HANGUL LETTER RIEUL-HIEUH */
    {XKB_KEY_Hangul_Mieum, 0x3141},               /* HANGUL LETTER MIEUM */
    {XKB_KEY_Hangul_Pieub, 0x3142},               /* HANGUL LETTER PIEUP */
    {XKB_KEY_Hangul_SsangPieub, 0x3143},          /* HANGUL LETTER SSANGPIEUP */
    {XKB_KEY_Hangul_PieubSios, 0x3144},           /* HANGUL LETTER PIEUP-SIOS */
    {XKB_KEY_Hangul_Sios, 0x3145},                /* HANGUL LETTER SIOS */
    {XKB_KEY_Hangul_SsangSios, 0x3146},           /* HANGUL LETTER SSANGSIOS */
    {XKB_KEY_Hangul_Ieung, 0x3147},               /* HANGUL LETTER IEUNG */
    {XKB_KEY_Hangul_Jieuj, 0x3148},               /* HANGUL LETTER CIEUC */
    {XKB_KEY_Hangul_SsangJieuj, 0x3149},          /* HANGUL LETTER SSANGCIEUC */
    {XKB_KEY_Hangul_Cieuc, 0x314a},               /* HANGUL LETTER CHIEUCH */
    {XKB_KEY_Hangul_Khieuq, 0x314b},              /* HANGUL LETTER KHIEUKH */
    {XKB_KEY_Hangul_Tieut, 0x314c},               /* HANGUL LETTER THIEUTH */
    {XKB_KEY_Hangul_Phieuf, 0x314d},              /* HANGUL LETTER PHIEUPH */
    {XKB_KEY_Hangul_Hieuh, 0x314e},               /* HANGUL LETTER HIEUH */
    {XKB_KEY_Hangul_A, 0x314f},                   /* HANGUL LETTER A */
    {XKB_KEY_Hangul_AE, 0x3150},                  /* HANGUL LETTER AE */
    {XKB_KEY_Hangul_YA, 0x3151},                  /* HANGUL LETTER YA */
    {XKB_KEY_Hangul_YAE, 0x3152},                 /* HANGUL LETTER YAE */
    {XKB_KEY_Hangul_EO, 0x3153},                  /* HANGUL LETTER EO */
    {XKB_KEY_Hangul_E, 0x3154},                   /* HANGUL LETTER E */
    {XKB_KEY_Hangul_YEO, 0x3155},                 /* HANGUL LETTER YEO */
    {XKB_KEY_Hangul_YE, 0x3156},                  /* HANGUL LETTER YE */
    {XKB_KEY_Hangul_O, 0x3157},                   /* HANGUL LETTER O */
    {XKB_KEY_Hangul_WA, 0x3158},                  /* HANGUL LETTER WA */
    {XKB_KEY_Hangul_WAE, 0x3159},                 /* HANGUL LETTER WAE */
    {XKB_KEY_Hangul_OE, 0x315a},                  /* HANGUL LETTER OE */
    {XKB_KEY_Hangul_YO, 0x315b},                  /* HANGUL LETTER YO */
    {XKB_KEY_Hangul_U, 0x315c},                   /* HANGUL LETTER U */
    {XKB_KEY_Hangul_WEO, 0x315d},                 /* HANGUL LETTER WEO */
    {XKB_KEY_Hangul_WE, 0x315e},                  /* HANGUL LETTER WE */
    {XKB_KEY_Hangul_WI, 0x315f},                  /* HANGUL LETTER WI */
    {XKB_KEY_Hangul_YU, 0x3160},                  /* HANGUL LETTER YU */
    {XKB_KEY_Hangul_EU, 0x3161},                  /* HANGUL LETTER EU */
    {XKB_KEY_Hangul_YI, 0x3162},                  /* HANGUL LETTER YI */
    {XKB_KEY_Hangul_I, 0x3163},                   /* HANGUL LETTER I */
    {XKB_KEY_Hangul_J_Kiyeog, 0x11a8},            /* HANGUL JONGSEONG KIYEOK */
    {XKB_KEY_Hangul_J_SsangKiyeog, 0x11a9},       /* HANGUL JONGSEONG SSANGKIYEOK */
    {XKB_KEY_Hangul_J_KiyeogSios, 0x11aa},        /* HANGUL JONGSEONG KIYEOK-SIOS */
    {XKB_KEY_Hangul_J_Nieun, 0x11ab},             /* HANGUL JONGSEONG NIEUN */
    {XKB_KEY_Hangul_J_NieunJieuj, 0x11ac},        /* HANGUL JONGSEONG NIEUN-CIEUC */
    {XKB_KEY_Hangul_J_NieunHieuh, 0x11ad},        /* HANGUL JONGSEONG NIEUN-HIEUH */
    {XKB_KEY_Hangul_J_Dikeud, 0x11ae},            /* HANGUL JONGSEONG TIKEUT */
    {XKB_KEY_Hangul_J_Rieul, 0x11af},             /* HANGUL JONGSEONG RIEUL */
    {XKB_KEY_Hangul_J_RieulKiyeog, 0x11b0},       /* HANGUL JONGSEONG RIEUL-KIYEOK */
    {XKB_KEY_Hangul_J_RieulMieum, 0x11b1},        /* HANGUL JONGSEONG RIEUL-MIEUM */
    {XKB_KEY_Hangul_J_RieulPieub, 0x11b2},        /* HANGUL JONGSEONG RIEUL-PIEUP */
    {XKB_KEY_Hangul_J_RieulSios, 0x11b3},         /* HANGUL JONGSEONG RIEUL-SIOS */
    {XKB_KEY_Hangul_J_RieulTieut, 0x11b4},        /* HANGUL JONGSEONG RIEUL-THIEUTH */
    {XKB_KEY_Hangul_J_RieulPhieuf, 0x11b5},       /* HANGUL JONGSEONG RIEUL-PHIEUPH */
    {XKB_KEY_Hangul_J_RieulHieuh, 0x11b6},        /* HANGUL JONGSEONG RIEUL-HIEUH */
    {XKB_KEY_Hangul_J_Mieum, 0x11b7},             /* HANGUL JONGSEONG MIEUM */
    {XKB_KEY_Hangul_J_Pieub, 0x11b8},             /* HANGUL JONGSEONG PIEUP */
    {XKB_KEY_Hangul_J_PieubSios, 0x11b9},         /* HANGUL JONGSEONG PIEUP-SIOS */
    {XKB_KEY_Hangul_J_Sios, 0x11ba},              /* HANGUL JONGSEONG SIOS */
    {XKB_KEY_Hangul_J_SsangSios, 0x11bb},         /* HANGUL JONGSEONG SSANGSIOS */
    {XKB_KEY_Hangul_J_Ieung, 0x11bc},             /* HANGUL JONGSEONG IEUNG */
    {XKB_KEY_Hangul_J_Jieuj, 0x11bd},             /* HANGUL JONGSEONG CIEUC */
    {XKB_KEY_Hangul_J_Cieuc, 0x11be},             /* HANGUL JONGSEONG CHIEUCH */
    {XKB_KEY_Hangul_J_Khieuq, 0x11bf},            /* HANGUL JONGSEONG KHIEUKH */
    {XKB_KEY_Hangul_J_Tieut, 0x11c0},             /* HANGUL JONGSEONG THIEUTH */
    {XKB_KEY_Hangul_J_Phieuf, 0x11c1},            /* HANGUL JONGSEONG PHIEUPH */
    {XKB_KEY_Hangul_J_Hieuh, 0x11c2},             /* HANGUL JONGSEONG HIEUH */
    {XKB_KEY_Hangul_RieulYeorinHieuh, 0x316d},    /* HANGUL LETTER RIEUL-YEORINHIEUH */
    {XKB_KEY_Hangul_SunkyeongeumMieum, 0x3171},   /* HANGUL LETTER KAPYEOUNMIEUM */
    {XKB_KEY_Hangul_SunkyeongeumPieub, 0x3178},   /* HANGUL LETTER KAPYEOUNPIEUP */
    {XKB_KEY_Hangul_PanSios, 0x317f},             /* HANGUL LETTER PANSIOS */
    {XKB_KEY_Hangul_KkogjiDalrinIeung, 0x3181},   /* HANGUL LETTER YESIEUNG */
    {XKB_KEY_Hangul_SunkyeongeumPhieuf, 0x3184},  /* HANGUL LETTER KAPYEOUNPHIEUPH */
    {XKB_KEY_Hangul_YeorinHieuh, 0x3186},         /* HANGUL LETTER YEORINHIEUH */
    {XKB_KEY_Hangul_AraeA, 0x318d},               /* HANGUL LETTER ARAEA */
    {XKB_KEY_Hangul_AraeAE, 0x318e},              /* HANGUL LETTER ARAEAE */
    {XKB_KEY_Hangul_J_PanSios, 0x11eb},           /* HANGUL JONGSEONG PANSIOS */
    {XKB_KEY_Hangul_J_KkogjiDalrinIeung, 0x11f0}, /* HANGUL JONGSEONG YESIEUNG */
    {XKB_KEY_Hangul_J_YeorinHieuh, 0x11f9},       /* HANGUL JONGSEONG YEORINHIEUH */
    {XKB_KEY_Korean_Won, 0x20a9},                 /* WON SIGN */
#endif                                            // defined(XKB_KEY_Hangul_Kiyeog)
    {XKB_KEY_OE, 0x0152},                         /* LATIN CAPITAL LIGATURE OE */
    {XKB_KEY_oe, 0x0153},                         /* LATIN SMALL LIGATURE OE */
    {XKB_KEY_Ydiaeresis, 0x0178},                 /* LATIN CAPITAL LETTER Y WITH DIAERESIS */
    {XKB_KEY_EuroSign, 0x20ac},                   /* EURO SIGN */

    /* combining dead keys */
    {XKB_KEY_dead_abovedot, 0x0307},    /* COMBINING DOT ABOVE */
    {XKB_KEY_dead_abovering, 0x030a},   /* COMBINING RING ABOVE */
    {XKB_KEY_dead_acute, 0x0301},       /* COMBINING ACUTE ACCENT */
    {XKB_KEY_dead_breve, 0x0306},       /* COMBINING BREVE */
    {XKB_KEY_dead_caron, 0x030c},       /* COMBINING CARON */
    {XKB_KEY_dead_cedilla, 0x0327},     /* COMBINING CEDILLA */
    {XKB_KEY_dead_circumflex, 0x0302},  /* COMBINING CIRCUMFLEX ACCENT */
    {XKB_KEY_dead_diaeresis, 0x0308},   /* COMBINING DIAERESIS */
    {XKB_KEY_dead_doubleacute, 0x030b}, /* COMBINING DOUBLE ACUTE ACCENT */
    {XKB_KEY_dead_grave, 0x0300},       /* COMBINING GRAVE ACCENT */
    {XKB_KEY_dead_macron, 0x0304},      /* COMBINING MACRON */
    {XKB_KEY_dead_ogonek, 0x0328},      /* COMBINING OGONEK */
    {XKB_KEY_dead_tilde, 0x0303}        /* COMBINING TILDE */
};
/* XXX -- map these too
XKB_KEY_Cyrillic_GHE_bar
XKB_KEY_Cyrillic_ZHE_descender
XKB_KEY_Cyrillic_KA_descender
XKB_KEY_Cyrillic_KA_vertstroke
XKB_KEY_Cyrillic_EN_descender
XKB_KEY_Cyrillic_U_straight
XKB_KEY_Cyrillic_U_straight_bar
XKB_KEY_Cyrillic_HA_descender
XKB_KEY_Cyrillic_CHE_descender
XKB_KEY_Cyrillic_CHE_vertstroke
XKB_KEY_Cyrillic_SHHA
XKB_KEY_Cyrillic_SCHWA
XKB_KEY_Cyrillic_I_macron
XKB_KEY_Cyrillic_O_bar
XKB_KEY_Cyrillic_U_macron
XKB_KEY_Cyrillic_ghe_bar
XKB_KEY_Cyrillic_zhe_descender
XKB_KEY_Cyrillic_ka_descender
XKB_KEY_Cyrillic_ka_vertstroke
XKB_KEY_Cyrillic_en_descender
XKB_KEY_Cyrillic_u_straight
XKB_KEY_Cyrillic_u_straight_bar
XKB_KEY_Cyrillic_ha_descender
XKB_KEY_Cyrillic_che_descender
XKB_KEY_Cyrillic_che_vertstroke
XKB_KEY_Cyrillic_shha
XKB_KEY_Cyrillic_schwa
XKB_KEY_Cyrillic_i_macron
XKB_KEY_Cyrillic_o_bar
XKB_KEY_Cyrillic_u_macron

XKB_KEY_Armenian_eternity
XKB_KEY_Armenian_ligature_ew
XKB_KEY_Armenian_full_stop
XKB_KEY_Armenian_verjaket
XKB_KEY_Armenian_parenright
XKB_KEY_Armenian_parenleft
XKB_KEY_Armenian_guillemotright
XKB_KEY_Armenian_guillemotleft
XKB_KEY_Armenian_em_dash
XKB_KEY_Armenian_dot
XKB_KEY_Armenian_mijaket
XKB_KEY_Armenian_but
XKB_KEY_Armenian_separation_mark
XKB_KEY_Armenian_comma
XKB_KEY_Armenian_en_dash
XKB_KEY_Armenian_hyphen
XKB_KEY_Armenian_yentamna
XKB_KEY_Armenian_ellipsis
XKB_KEY_Armenian_amanak
XKB_KEY_Armenian_exclam
XKB_KEY_Armenian_accent
XKB_KEY_Armenian_shesht
XKB_KEY_Armenian_paruyk
XKB_KEY_Armenian_question
XKB_KEY_Armenian_AYB
XKB_KEY_Armenian_ayb
XKB_KEY_Armenian_BEN
XKB_KEY_Armenian_ben
XKB_KEY_Armenian_GIM
XKB_KEY_Armenian_gim
XKB_KEY_Armenian_DA
XKB_KEY_Armenian_da
XKB_KEY_Armenian_YECH
XKB_KEY_Armenian_yech
XKB_KEY_Armenian_ZA
XKB_KEY_Armenian_za
XKB_KEY_Armenian_E
XKB_KEY_Armenian_e
XKB_KEY_Armenian_AT
XKB_KEY_Armenian_at
XKB_KEY_Armenian_TO
XKB_KEY_Armenian_to
XKB_KEY_Armenian_ZHE
XKB_KEY_Armenian_zhe
XKB_KEY_Armenian_INI
XKB_KEY_Armenian_ini
XKB_KEY_Armenian_LYUN
XKB_KEY_Armenian_lyun
XKB_KEY_Armenian_KHE
XKB_KEY_Armenian_khe
XKB_KEY_Armenian_TSA
XKB_KEY_Armenian_tsa
XKB_KEY_Armenian_KEN
XKB_KEY_Armenian_ken
XKB_KEY_Armenian_HO
XKB_KEY_Armenian_ho
XKB_KEY_Armenian_DZA
XKB_KEY_Armenian_dza
XKB_KEY_Armenian_GHAT
XKB_KEY_Armenian_ghat
XKB_KEY_Armenian_TCHE
XKB_KEY_Armenian_tche
XKB_KEY_Armenian_MEN
XKB_KEY_Armenian_men
XKB_KEY_Armenian_HI
XKB_KEY_Armenian_hi
XKB_KEY_Armenian_NU
XKB_KEY_Armenian_nu
XKB_KEY_Armenian_SHA
XKB_KEY_Armenian_sha
XKB_KEY_Armenian_VO
XKB_KEY_Armenian_vo
XKB_KEY_Armenian_CHA
XKB_KEY_Armenian_cha
XKB_KEY_Armenian_PE
XKB_KEY_Armenian_pe
XKB_KEY_Armenian_JE
XKB_KEY_Armenian_je
XKB_KEY_Armenian_RA
XKB_KEY_Armenian_ra
XKB_KEY_Armenian_SE
XKB_KEY_Armenian_se
XKB_KEY_Armenian_VEV
XKB_KEY_Armenian_vev
XKB_KEY_Armenian_TYUN
XKB_KEY_Armenian_tyun
XKB_KEY_Armenian_RE
XKB_KEY_Armenian_re
XKB_KEY_Armenian_TSO
XKB_KEY_Armenian_tso
XKB_KEY_Armenian_VYUN
XKB_KEY_Armenian_vyun
XKB_KEY_Armenian_PYUR
XKB_KEY_Armenian_pyur
XKB_KEY_Armenian_KE
XKB_KEY_Armenian_ke
XKB_KEY_Armenian_O
XKB_KEY_Armenian_o
XKB_KEY_Armenian_FE
XKB_KEY_Armenian_fe
XKB_KEY_Armenian_apostrophe
XKB_KEY_Armenian_section_sign

XKB_KEY_Georgian_an
XKB_KEY_Georgian_ban
XKB_KEY_Georgian_gan
XKB_KEY_Georgian_don
XKB_KEY_Georgian_en
XKB_KEY_Georgian_vin
XKB_KEY_Georgian_zen
XKB_KEY_Georgian_tan
XKB_KEY_Georgian_in
XKB_KEY_Georgian_kan
XKB_KEY_Georgian_las
XKB_KEY_Georgian_man
XKB_KEY_Georgian_nar
XKB_KEY_Georgian_on
XKB_KEY_Georgian_par
XKB_KEY_Georgian_zhar
XKB_KEY_Georgian_rae
XKB_KEY_Georgian_san
XKB_KEY_Georgian_tar
XKB_KEY_Georgian_un
XKB_KEY_Georgian_phar
XKB_KEY_Georgian_khar
XKB_KEY_Georgian_ghan
XKB_KEY_Georgian_qar
XKB_KEY_Georgian_shin
XKB_KEY_Georgian_chin
XKB_KEY_Georgian_can
XKB_KEY_Georgian_jil
XKB_KEY_Georgian_cil
XKB_KEY_Georgian_char
XKB_KEY_Georgian_xan
XKB_KEY_Georgian_jhan
XKB_KEY_Georgian_hae
XKB_KEY_Georgian_he
XKB_KEY_Georgian_hie
XKB_KEY_Georgian_we
XKB_KEY_Georgian_har
XKB_KEY_Georgian_hoe
XKB_KEY_Georgian_fi

XKB_KEY_Ccedillaabovedot
XKB_KEY_Xabovedot
XKB_KEY_Qabovedot
XKB_KEY_Ibreve
XKB_KEY_IE
XKB_KEY_UO
XKB_KEY_Zstroke
XKB_KEY_Gcaron
XKB_KEY_Obarred
XKB_KEY_ccedillaabovedot
XKB_KEY_xabovedot
XKB_KEY_Ocaron
XKB_KEY_qabovedot
XKB_KEY_ibreve
XKB_KEY_ie
XKB_KEY_uo
XKB_KEY_zstroke
XKB_KEY_gcaron
XKB_KEY_ocaron
XKB_KEY_obarred
XKB_KEY_SCHWA
XKB_KEY_Lbelowdot
XKB_KEY_Lstrokebelowdot
XKB_KEY_Gtilde
XKB_KEY_lbelowdot
XKB_KEY_lstrokebelowdot
XKB_KEY_gtilde
XKB_KEY_schwa

XKB_KEY_Abelowdot
XKB_KEY_abelowdot
XKB_KEY_Ahook
XKB_KEY_ahook
XKB_KEY_Acircumflexacute
XKB_KEY_acircumflexacute
XKB_KEY_Acircumflexgrave
XKB_KEY_acircumflexgrave
XKB_KEY_Acircumflexhook
XKB_KEY_acircumflexhook
XKB_KEY_Acircumflextilde
XKB_KEY_acircumflextilde
XKB_KEY_Acircumflexbelowdot
XKB_KEY_acircumflexbelowdot
XKB_KEY_Abreveacute
XKB_KEY_abreveacute
XKB_KEY_Abrevegrave
XKB_KEY_abrevegrave
XKB_KEY_Abrevehook
XKB_KEY_abrevehook
XKB_KEY_Abrevetilde
XKB_KEY_abrevetilde
XKB_KEY_Abrevebelowdot
XKB_KEY_abrevebelowdot
XKB_KEY_Ebelowdot
XKB_KEY_ebelowdot
XKB_KEY_Ehook
XKB_KEY_ehook
XKB_KEY_Etilde
XKB_KEY_etilde
XKB_KEY_Ecircumflexacute
XKB_KEY_ecircumflexacute
XKB_KEY_Ecircumflexgrave
XKB_KEY_ecircumflexgrave
XKB_KEY_Ecircumflexhook
XKB_KEY_ecircumflexhook
XKB_KEY_Ecircumflextilde
XKB_KEY_ecircumflextilde
XKB_KEY_Ecircumflexbelowdot
XKB_KEY_ecircumflexbelowdot
XKB_KEY_Ihook
XKB_KEY_ihook
XKB_KEY_Ibelowdot
XKB_KEY_ibelowdot
XKB_KEY_Obelowdot
XKB_KEY_obelowdot
XKB_KEY_Ohook
XKB_KEY_ohook
XKB_KEY_Ocircumflexacute
XKB_KEY_ocircumflexacute
XKB_KEY_Ocircumflexgrave
XKB_KEY_ocircumflexgrave
XKB_KEY_Ocircumflexhook
XKB_KEY_ocircumflexhook
XKB_KEY_Ocircumflextilde
XKB_KEY_ocircumflextilde
XKB_KEY_Ocircumflexbelowdot
XKB_KEY_ocircumflexbelowdot
XKB_KEY_Ohornacute
XKB_KEY_ohornacute
XKB_KEY_Ohorngrave
XKB_KEY_ohorngrave
XKB_KEY_Ohornhook
XKB_KEY_ohornhook
XKB_KEY_Ohorntilde
XKB_KEY_ohorntilde
XKB_KEY_Ohornbelowdot
XKB_KEY_ohornbelowdot
XKB_KEY_Ubelowdot
XKB_KEY_ubelowdot
XKB_KEY_Uhook
XKB_KEY_uhook
XKB_KEY_Uhornacute
XKB_KEY_uhornacute
XKB_KEY_Uhorngrave
XKB_KEY_uhorngrave
XKB_KEY_Uhornhook
XKB_KEY_uhornhook
XKB_KEY_Uhorntilde
XKB_KEY_uhorntilde
XKB_KEY_Uhornbelowdot
XKB_KEY_uhornbelowdot
XKB_KEY_Ybelowdot
XKB_KEY_ybelowdot
XKB_KEY_Yhook
XKB_KEY_yhook
XKB_KEY_Ytilde
XKB_KEY_ytilde
XKB_KEY_Ohorn
XKB_KEY_ohorn
XKB_KEY_Uhorn
XKB_KEY_uhorn
*/

//
// XDGKeyUtil
//

XDGKeyUtil::KeySymMap XDGKeyUtil::s_keySymToUCS4;

KeyID XDGKeyUtil::mapKeySymToKeyID(KeySym k)
{
  initKeyMaps();

  switch (k & 0xffffff00) {
  case 0x0000:
    // Latin-1
    return static_cast<KeyID>(k);

  case 0xfe00:
    // ISO 9995 Function and Modifier Keys
    switch (k) {
    case XKB_KEY_ISO_Left_Tab:
      return kKeyLeftTab;

    case XKB_KEY_ISO_Level3_Shift:
      return kKeyAltGr;

#ifdef XKB_KEY_ISO_Level5_Shift
    case XKB_KEY_ISO_Level5_Shift:
      return XKB_KEY_ISO_Level5_Shift; // there is no "usual" key for this...
#endif

    case XKB_KEY_ISO_Next_Group:
      return kKeyNextGroup;

    case XKB_KEY_ISO_Prev_Group:
      return kKeyPrevGroup;

    case XKB_KEY_dead_grave:
      return kKeyDeadGrave;

    case XKB_KEY_dead_acute:
      return kKeyDeadAcute;

    case XKB_KEY_dead_circumflex:
      return kKeyDeadCircumflex;

    case XKB_KEY_dead_tilde:
      return kKeyDeadTilde;

    case XKB_KEY_dead_macron:
      return kKeyDeadMacron;

    case XKB_KEY_dead_breve:
      return kKeyDeadBreve;

    case XKB_KEY_dead_abovedot:
      return kKeyDeadAbovedot;

    case XKB_KEY_dead_diaeresis:
      return kKeyDeadDiaeresis;

    case XKB_KEY_dead_abovering:
      return kKeyDeadAbovering;

    case XKB_KEY_dead_doubleacute:
      return kKeyDeadDoubleacute;

    case XKB_KEY_dead_caron:
      return kKeyDeadCaron;

    case XKB_KEY_dead_cedilla:
      return kKeyDeadCedilla;

    case XKB_KEY_dead_ogonek:
      return kKeyDeadOgonek;

    default:
      return kKeyNone;
    }

  case 0xff00:
    // MISCELLANY
    return static_cast<KeyID>(k - 0xff00 + 0xef00);

  case 0x1008ff00:
    // "Internet" keys
    return static_cast<KeyID>(s_map1008FF.at(k & 0xff));

  default: {
    // lookup character in table
    auto index = s_keySymToUCS4.find(k);
    if (index != s_keySymToUCS4.end()) {
      return static_cast<KeyID>(index->second);
    }

    // unknown character
    return kKeyNone;
  }
  }
}

std::uint32_t XDGKeyUtil::getModifierBitForKeySym(KeySym keysym)
{
  switch (keysym) {
  case XKB_KEY_Shift_L:
  case XKB_KEY_Shift_R:
    return kKeyModifierBitShift;

  case XKB_KEY_Control_L:
  case XKB_KEY_Control_R:
    return kKeyModifierBitControl;

  case XKB_KEY_Alt_L:
  case XKB_KEY_Alt_R:
    return kKeyModifierBitAlt;

  case XKB_KEY_Meta_L:
  case XKB_KEY_Meta_R:
    return kKeyModifierBitMeta;

  case XKB_KEY_Super_L:
  case XKB_KEY_Super_R:
  case XKB_KEY_Hyper_L:
  case XKB_KEY_Hyper_R:
    return kKeyModifierBitSuper;

  case XKB_KEY_Mode_switch:
  case XKB_KEY_ISO_Level3_Shift:
    return kKeyModifierBitAltGr;

#ifdef XKB_KEY_ISO_Level5_Shift
  case XKB_KEY_ISO_Level5_Shift:
    return kKeyModifierBitLevel5Lock;
#endif

  case XKB_KEY_Caps_Lock:
    return kKeyModifierBitCapsLock;

  case XKB_KEY_Num_Lock:
    return kKeyModifierBitNumLock;

  case XKB_KEY_Scroll_Lock:
    return kKeyModifierBitScrollLock;

  default:
    return kKeyModifierBitNone;
  }
}

void XDGKeyUtil::initKeyMaps()
{
  if (s_keySymToUCS4.empty()) {
    for (size_t i = 0; i < sizeof(s_keymap) / sizeof(s_keymap[0]); ++i) {
      s_keySymToUCS4[s_keymap[i].keysym] = s_keymap[i].ucs4;
    }
  }
}

std::array<KeySym, 256> XDGKeyUtil::s_map1008FF = {
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
    0
};
