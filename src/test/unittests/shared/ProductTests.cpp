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
  Product product1(kPro);
  Product product2(kPro);

  EXPECT_EQ(product1, product2);
}

TEST(ProductTests, ctor_businessName_isValid) {
  Product product(Product::SerialKeyEditionID::Buisiness);

  EXPECT_EQ(kBusiness, product.edition());
  EXPECT_TRUE(product.isValid());
}

TEST(ProductTests, ctor_basicType_isValid) {
  Product product(kBasic);

  EXPECT_TRUE(product.isValid());
}

TEST(ProductTests, setEdition_invalidType_throws) {
  Product product;

  EXPECT_THROW(product.setEdition("test"), Product::InvalidType);
}

TEST(ProductTests, setEdition_pro_isValid) {
  Product product;

  product.setEdition(kPro);

  EXPECT_EQ(kPro, product.edition());
  EXPECT_EQ(Product::SerialKeyEditionID::Pro, product.serialKeyId());
  EXPECT_EQ("Synergy 1 Pro", product.name());
  EXPECT_TRUE(product.isValid());
}

TEST(ProductTests, setEdition_basic_isValid) {
  Product product;

  product.setEdition(kBasic);

  EXPECT_EQ(kBasic, product.edition());
  EXPECT_EQ(Product::SerialKeyEditionID::Basic, product.serialKeyId());
  EXPECT_EQ("Synergy 1 Basic", product.name());
}

TEST(ProductTests, setEdition_business_isValid) {
  Product product;

  product.setEdition(kBusiness);

  EXPECT_EQ(kBusiness, product.edition());
  EXPECT_EQ(Product::SerialKeyEditionID::Buisiness, product.serialKeyId());
  EXPECT_EQ("Synergy 1 Business", product.name());
}
