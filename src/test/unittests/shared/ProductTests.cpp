/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2016 Symless Ltd.
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

#define TEST_ENV

#include "license/Product.h"

#include <gtest/gtest.h>

using enum Product::Edition;

TEST(ProductTests, equal_operator) {
  Product edition1(kPro);
  Product edition2(kPro);

  EXPECT_EQ(edition1, edition2);
}

TEST(ProductTests, ctor_businessName_isValid) {
  Product edition(Product::SerialKeyEditionID::Buisiness);

  EXPECT_EQ(kBusiness, edition.getEdition());
  EXPECT_TRUE(edition.isValid());
}

TEST(ProductTests, ctor_basicType_isValid) {
  Product edition(kBasic);

  EXPECT_TRUE(edition.isValid());
}

TEST(ProductTests, setEdition_invalidType_unregistered) {
  Product edition;

  edition.setEdition("stub");

  EXPECT_EQ(kUnregistered, edition.getEdition());
}

TEST(ProductTests, setEdition_pro_isValid) {
  Product edition;

  edition.setEdition(kPro);

  EXPECT_EQ(kPro, edition.getEdition());
  EXPECT_EQ(Product::SerialKeyEditionID::Pro, edition.getSerialKeyId());
  EXPECT_EQ("Synergy 1 Pro", edition.productName());
  EXPECT_TRUE(edition.isValid());
}

TEST(ProductTests, setEdition_basic_isValid) {
  Product edition;

  edition.setEdition(kBasic);

  EXPECT_EQ(kBasic, edition.getEdition());
  EXPECT_EQ(Product::SerialKeyEditionID::Basic, edition.getSerialKeyId());
  EXPECT_EQ("Synergy 1 Basic", edition.productName());
}

TEST(ProductTests, setEdition_business_isValid) {
  Product edition;

  edition.setEdition(kBusiness);

  EXPECT_EQ(kBusiness, edition.getEdition());
  EXPECT_EQ(Product::SerialKeyEditionID::Buisiness, edition.getSerialKeyId());
  EXPECT_EQ("Synergy 1 Business", edition.productName());
}
