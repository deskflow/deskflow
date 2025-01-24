/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <string>
#include <vector>
// copy from
// https://www.loc.gov/standards/iso639-2/php/code_list.php
// 10.06.2021
// first param - ISO 639-2, second param - 639-1
const std::vector<std::pair<std::string, std::string>> ISO_Table = {
    std::make_pair("aar", "aa"), std::make_pair("abk", "ab"), std::make_pair("afr", "af"), std::make_pair("aka", "ak"),
    std::make_pair("sqi", "sq"), std::make_pair("amh", "am"), std::make_pair("ara", "ar"), std::make_pair("arg", "an"),
    std::make_pair("hye", "hy"), std::make_pair("asm", "as"), std::make_pair("ava", "av"), std::make_pair("ave", "ae"),
    std::make_pair("aym", "ay"), std::make_pair("aze", "az"), std::make_pair("bak", "ba"), std::make_pair("bam", "bm"),
    std::make_pair("eus", "eu"), std::make_pair("bel", "be"), std::make_pair("ben", "bn"), std::make_pair("bih", "bh"),
    std::make_pair("bis", "bi"), std::make_pair("bod", "bo"), std::make_pair("bos", "bs"), std::make_pair("bre", "br"),
    std::make_pair("bul", "bg"), std::make_pair("mya", "my"), std::make_pair("cat", "ca"), std::make_pair("ces", "cs"),
    std::make_pair("cha", "ch"), std::make_pair("che", "ce"), std::make_pair("zho", "zh"), std::make_pair("chu", "cu"),
    std::make_pair("chv", "cv"), std::make_pair("cor", "kw"), std::make_pair("cos", "co"), std::make_pair("cre", "cr"),
    std::make_pair("cym", "cy"), std::make_pair("ces", "cs"), std::make_pair("dan", "da"), std::make_pair("deu", "de"),
    std::make_pair("div", "dv"), std::make_pair("nld", "nl"), std::make_pair("dzo", "dz"), std::make_pair("ell", "el"),
    std::make_pair("eng", "en"), std::make_pair("epo", "eo"), std::make_pair("est", "et"), std::make_pair("eus", "eu"),
    std::make_pair("ewe", "ee"), std::make_pair("fao", "fo"), std::make_pair("fas", "fa"), std::make_pair("fij", "fj"),
    std::make_pair("fin", "fi"), std::make_pair("fra", "fr"), std::make_pair("fra", "fr"), std::make_pair("fry", "fy"),
    std::make_pair("ful", "ff"), std::make_pair("kat", "ka"), std::make_pair("deu", "de"), std::make_pair("gla", "gd"),
    std::make_pair("gle", "ga"), std::make_pair("glg", "gl"), std::make_pair("glv", "gv"), std::make_pair("ell", "el"),
    std::make_pair("grn", "gn"), std::make_pair("guj", "gu"), std::make_pair("hat", "ht"), std::make_pair("hau", "ha"),
    std::make_pair("heb", "he"), std::make_pair("her", "hz"), std::make_pair("hin", "hi"), std::make_pair("hmo", "ho"),
    std::make_pair("hrv", "hr"), std::make_pair("hun", "hu"), std::make_pair("hye", "hy"), std::make_pair("ibo", "ig"),
    std::make_pair("isl", "is"), std::make_pair("ido", "io"), std::make_pair("iii", "ii"), std::make_pair("iku", "iu"),
    std::make_pair("ile", "ie"), std::make_pair("ina", "ia"), std::make_pair("ind", "id"), std::make_pair("ipk", "ik"),
    std::make_pair("isl", "is"), std::make_pair("ita", "it"), std::make_pair("jav", "jv"), std::make_pair("jpn", "ja"),
    std::make_pair("kal", "kl"), std::make_pair("kan", "kn"), std::make_pair("kas", "ks"), std::make_pair("kat", "ka"),
    std::make_pair("kau", "kr"), std::make_pair("kaz", "kk"), std::make_pair("khm", "km"), std::make_pair("kik", "ki"),
    std::make_pair("kin", "rw"), std::make_pair("kir", "ky"), std::make_pair("kom", "kv"), std::make_pair("kon", "kg"),
    std::make_pair("kor", "ko"), std::make_pair("kua", "kj"), std::make_pair("kur", "ku"), std::make_pair("lao", "lo"),
    std::make_pair("lat", "la"), std::make_pair("lav", "lv"), std::make_pair("lim", "li"), std::make_pair("lin", "ln"),
    std::make_pair("lit", "lt"), std::make_pair("ltz", "lb"), std::make_pair("lub", "lu"), std::make_pair("lug", "lg"),
    std::make_pair("mkd", "mk"), std::make_pair("mah", "mh"), std::make_pair("mal", "ml"), std::make_pair("mri", "mi"),
    std::make_pair("mar", "mr"), std::make_pair("msa", "ms"), std::make_pair("mkd", "mk"), std::make_pair("mlg", "mg"),
    std::make_pair("mlt", "mt"), std::make_pair("mon", "mn"), std::make_pair("mri", "mi"), std::make_pair("msa", "ms"),
    std::make_pair("mya", "my"), std::make_pair("nau", "na"), std::make_pair("nav", "nv"), std::make_pair("nbl", "nr"),
    std::make_pair("nde", "nd"), std::make_pair("ndo", "ng"), std::make_pair("nep", "ne"), std::make_pair("nld", "nl"),
    std::make_pair("nno", "nn"), std::make_pair("nob", "nb"), std::make_pair("nor", "no"), std::make_pair("nya", "ny"),
    std::make_pair("oci", "oc"), std::make_pair("oji", "oj"), std::make_pair("ori", "or"), std::make_pair("orm", "om"),
    std::make_pair("oss", "os"), std::make_pair("pan", "pa"), std::make_pair("fas", "fa"), std::make_pair("pli", "pi"),
    std::make_pair("pol", "pl"), std::make_pair("por", "pt"), std::make_pair("pus", "ps"), std::make_pair("que", "qu"),
    std::make_pair("roh", "rm"), std::make_pair("ron", "ro"), std::make_pair("ron", "ro"), std::make_pair("run", "rn"),
    std::make_pair("rus", "ru"), std::make_pair("sag", "sg"), std::make_pair("san", "sa"), std::make_pair("sin", "si"),
    std::make_pair("slk", "sk"), std::make_pair("slk", "sk"), std::make_pair("slv", "sl"), std::make_pair("sme", "se"),
    std::make_pair("smo", "sm"), std::make_pair("sna", "sn"), std::make_pair("snd", "sd"), std::make_pair("som", "so"),
    std::make_pair("sot", "st"), std::make_pair("spa", "es"), std::make_pair("sqi", "sq"), std::make_pair("srd", "sc"),
    std::make_pair("srp", "sr"), std::make_pair("ssw", "ss"), std::make_pair("sun", "su"), std::make_pair("swa", "sw"),
    std::make_pair("swe", "sv"), std::make_pair("tah", "ty"), std::make_pair("tam", "ta"), std::make_pair("tat", "tt"),
    std::make_pair("tel", "te"), std::make_pair("tgk", "tg"), std::make_pair("tgl", "tl"), std::make_pair("tha", "th"),
    std::make_pair("bod", "bo"), std::make_pair("tir", "ti"), std::make_pair("ton", "to"), std::make_pair("tsn", "tn"),
    std::make_pair("tso", "ts"), std::make_pair("tuk", "tk"), std::make_pair("tur", "tr"), std::make_pair("twi", "tw"),
    std::make_pair("uig", "ug"), std::make_pair("ukr", "uk"), std::make_pair("urd", "ur"), std::make_pair("uzb", "uz"),
    std::make_pair("ven", "ve"), std::make_pair("vie", "vi"), std::make_pair("vol", "vo"), std::make_pair("cym", "cy"),
    std::make_pair("wln", "wa"), std::make_pair("wol", "wo"), std::make_pair("xho", "xh"), std::make_pair("yid", "yi"),
    std::make_pair("yor", "yo"), std::make_pair("zha", "za"), std::make_pair("zho", "zh"), std::make_pair("zul", "zu"),
};
