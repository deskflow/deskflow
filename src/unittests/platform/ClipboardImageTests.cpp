/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ClipboardImageTests.h"

#include "platform/ClipboardImage.h"

#include <QBuffer>
#include <QImage>
#include <QImageWriter>
#include <QtEndian>

using deskflow::ClipboardImage;

void ClipboardImageTests::initTestCase()
{
  m_arch.init();
  m_log.setFilter(LogLevel::Level::Debug);
}

void ClipboardImageTests::imageToDib_png_returnsDibWithImageSize()
{
  QImage image(2, 3, QImage::Format_RGB32);
  image.fill(Qt::red);
  QByteArray png;
  QBuffer buffer(&png);
  buffer.open(QIODevice::WriteOnly);
  QVERIFY(image.save(&buffer, "PNG"));

  const auto dib = ClipboardImage::imageToDib(png, "PNG");

  QVERIFY(dib.size() > 40);
  QCOMPARE(qFromLittleEndian<quint32>(dib.constData()), quint32(40));
  QCOMPARE(qFromLittleEndian<qint32>(dib.constData() + 4), 2);
  QCOMPARE(qFromLittleEndian<qint32>(dib.constData() + 8), 3);
}

void ClipboardImageTests::imageToDib_bmp_returnsDibWithImageSize()
{
  QImage image(4, 5, QImage::Format_RGB32);
  image.fill(Qt::blue);
  QByteArray bmp;
  QBuffer buffer(&bmp);
  buffer.open(QIODevice::WriteOnly);
  QVERIFY(image.save(&buffer, "BMP"));

  const auto dib = ClipboardImage::imageToDib(bmp, "BMP");

  QCOMPARE(qFromLittleEndian<qint32>(dib.constData() + 4), 4);
  QCOMPARE(qFromLittleEndian<qint32>(dib.constData() + 8), 5);
}

void ClipboardImageTests::imageToDib_jpeg_returnsDibWithImageSize()
{
  if (!QImageWriter::supportedImageFormats().contains("jpeg"))
    QSKIP("qt jpeg plugin not installed");

  QImage image(6, 7, QImage::Format_RGB32);
  image.fill(Qt::green);
  QByteArray jpeg;
  QBuffer buffer(&jpeg);
  buffer.open(QIODevice::WriteOnly);
  QVERIFY(image.save(&buffer, "JPEG"));

  const auto dib = ClipboardImage::imageToDib(jpeg, "JPEG");

  QCOMPARE(qFromLittleEndian<qint32>(dib.constData() + 4), 6);
  QCOMPARE(qFromLittleEndian<qint32>(dib.constData() + 8), 7);
}

void ClipboardImageTests::dibToImage_dibFromPng_keepsPixels()
{
  QImage image(2, 2, QImage::Format_RGB32);
  image.setPixel(0, 0, qRgb(255, 0, 0));
  image.setPixel(1, 0, qRgb(0, 255, 0));
  image.setPixel(0, 1, qRgb(0, 0, 255));
  image.setPixel(1, 1, qRgb(255, 255, 255));
  QByteArray png;
  QBuffer buffer(&png);
  buffer.open(QIODevice::WriteOnly);
  QVERIFY(image.save(&buffer, "PNG"));

  const auto dib = ClipboardImage::imageToDib(png, "PNG");
  const auto roundTrip = QImage::fromData(ClipboardImage::dibToImage(dib, "PNG"), "PNG");

  QCOMPARE(roundTrip.convertToFormat(QImage::Format_RGB32), image);
}

void ClipboardImageTests::imageToDib_notAnImage_returnsEmpty()
{
  QVERIFY(ClipboardImage::imageToDib("not an image", "PNG").isEmpty());
}

QTEST_MAIN(ClipboardImageTests)
