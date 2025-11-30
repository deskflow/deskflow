/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "gui/config/Screen.h"
#include <QWidget>
#include <QPainter>

/**
 * @brief Widget to display a visual layout of multiple monitors
 */
class MonitorLayoutWidget : public QWidget
{
  Q_OBJECT

public:
  explicit MonitorLayoutWidget(QWidget *parent = nullptr);
  
  void setMonitors(const QVector<MonitorInfo> &monitors);
  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  QVector<MonitorInfo> m_monitors;
  QVector<QRectF> calculateScaledRects(const QSize &targetSize) const;
};
