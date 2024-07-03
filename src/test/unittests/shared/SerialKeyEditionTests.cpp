/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2016 Symless Inc.
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

#include "shared/EditionType.h"
#define TEST_ENV

#include "shared/SerialKeyEdition.h"
#include "test/global/gtest.h"

TEST(SerialKeyEditionTests, ctor_default_unregistered) {
  SerialKeyEdition edition;

  EXPECT_EQ(kUnregistered, edition.getType());
  EXPECT_EQ(SerialKeyEdition::Unregistered, edition.getName());
  EXPECT_EQ("Synergy 1 (Unregistered)", edition.getProductName());
  EXPECT_FALSE(edition.isValid());
}

TEST(SerialKeyEditionTests, ctor_businessName_isValid) {
  SerialKeyEdition edition(SerialKeyEdition::Buisiness);

  EXPECT_EQ(kBusiness, edition.getType());
  EXPECT_TRUE(edition.isValid());
}

TEST(SerialKeyEditionTests, ctor_basicType_isValid) {
  SerialKeyEdition edition(kBasic);

  EXPECT_TRUE(edition.isValid());
}

TEST(SerialKeyEditionTests, setType_lite_isValid) {
  SerialKeyEdition edition;

  edition.setType(kLite);

  EXPECT_EQ(kLite, edition.getType());
  EXPECT_EQ(SerialKeyEdition::Lite, edition.getName());
  EXPECT_EQ("Synergy 1", edition.getProductName());
  EXPECT_TRUE(edition.isValid());
}

TEST(SerialKeyEditionTests, setType_ultimate_isValid) {
  SerialKeyEdition edition;

  edition.setType(SerialKeyEdition::Ultimate);

  EXPECT_EQ(kUltimate, edition.getType());
  EXPECT_EQ(SerialKeyEdition::Ultimate, edition.getName());
  EXPECT_EQ("Synergy 1 Ultimate", edition.getProductName());
}

TEST(SerialKeyEditionTests, setType_pro_isValid) {
  SerialKeyEdition edition;

  edition.setType(kPro);

  EXPECT_EQ(kPro, edition.getType());
  EXPECT_EQ(SerialKeyEdition::Pro, edition.getName());
  EXPECT_EQ("Synergy 1 Pro", edition.getProductName());
  EXPECT_TRUE(edition.isValid());
}

TEST(SerialKeyEditionTests, setType_basic_isValid) {
  SerialKeyEdition edition;

  edition.setType(kBasic);

  EXPECT_EQ(kBasic, edition.getType());
  EXPECT_EQ(SerialKeyEdition::Basic, edition.getName());
  EXPECT_EQ("Synergy 1 Basic", edition.getProductName());
}

TEST(SerialKeyEditionTests, setType_business_isValid) {
  SerialKeyEdition edition;

  edition.setType(kBusiness);

  EXPECT_EQ(kBusiness, edition.getType());
  EXPECT_EQ(SerialKeyEdition::Buisiness, edition.getName());
  EXPECT_EQ("Synergy 1 Business", edition.getProductName());
}

TEST(SerialKeyEditionTests, setType_basicChina_isValid) {
  SerialKeyEdition edition;

  edition.setType(kBasicChina);

  EXPECT_EQ(kBasicChina, edition.getType());
  EXPECT_EQ(SerialKeyEdition::BasicChina, edition.getName());
  EXPECT_EQ("Synergy 1 中文版", edition.getProductName());
  EXPECT_TRUE(edition.isChina());
}

TEST(SerialKeyEditionTests, setType_proChina_isValid) {
  SerialKeyEdition edition;

  edition.setType(kProChina);

  EXPECT_EQ(kProChina, edition.getType());
  EXPECT_EQ(SerialKeyEdition::ProChina, edition.getName());
  EXPECT_EQ("Synergy 1 Pro 中文版", edition.getProductName());
  EXPECT_TRUE(edition.isChina());
}
