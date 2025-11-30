/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "MonitorLayoutWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include <algorithm>

MonitorLayoutWidget::MonitorLayoutWidget(QWidget *parent)
  : QWidget(parent)
{
  setMinimumSize(96, 96);
}

void MonitorLayoutWidget::setMonitors(const QVector<MonitorInfo> &monitors)
{
  m_monitors = monitors;
  update();
}

QSize MonitorLayoutWidget::sizeHint() const
{
  return QSize(96, 96);
}

QVector<QRectF> MonitorLayoutWidget::calculateScaledRects(const QSize &targetSize) const
{
  QVector<QRectF> scaledRects;
  
  if (m_monitors.isEmpty()) {
    return scaledRects;
  }

  auto minX = std::numeric_limits<int>::max();
  auto minY = std::numeric_limits<int>::max();
  auto maxX = std::numeric_limits<int>::min();
  auto maxY = std::numeric_limits<int>::min();
  
  for (const auto &monitor : m_monitors) {
    minX = std::min(minX, monitor.geometry.x());
    minY = std::min(minY, monitor.geometry.y());
    maxX = std::max(maxX, monitor.geometry.x() + monitor.geometry.width());
    maxY = std::max(maxY, monitor.geometry.y() + monitor.geometry.height());
  }
  
  const auto totalWidth = maxX - minX;
  const auto totalHeight = maxY - minY;
  
  if (totalWidth == 0 || totalHeight == 0) {
    return scaledRects;
  }

  constexpr auto padding = 10;
  const auto scaleX = static_cast<double>(targetSize.width() - 2 * padding) / totalWidth;
  const auto scaleY = static_cast<double>(targetSize.height() - 2 * padding) / totalHeight;
  const auto scale = std::min(scaleX, scaleY);

  for (const auto &monitor : m_monitors) {
    auto x = (monitor.geometry.x() - minX) * scale + padding;
    auto y = (monitor.geometry.y() - minY) * scale + padding;
    auto w = monitor.geometry.width() * scale;
    auto h = monitor.geometry.height() * scale;
    
    scaledRects.append(QRectF(x, y, w, h));
  }
  
  return scaledRects;
}

void MonitorLayoutWidget::paintEvent(QPaintEvent *event)
{
  Q_UNUSED(event);
  
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  
  if (m_monitors.isEmpty()) {
    QRect rect = this->rect().adjusted(10, 10, -10, -10);
    painter.fillRect(rect, QColor(100, 150, 200));
    painter.setPen(QPen(Qt::black, 2));
    painter.drawRect(rect);
    return;
  }

  QVector<QRectF> rects = calculateScaledRects(this->size());

  for (int i = 0; i < rects.size(); ++i) {
    const QRectF &rect = rects[i];
    const MonitorInfo &monitor = m_monitors[i];
    QColor fillColor = monitor.isPrimary ? QColor(100, 150, 255) : QColor(120, 170, 220);
    painter.fillRect(rect, fillColor);
    painter.setPen(QPen(monitor.isPrimary ? Qt::blue : Qt::black, monitor.isPrimary ? 3 : 2));
    painter.drawRect(rect);

    if (rect.width() > 20 && rect.height() > 15) {
      painter.setPen(Qt::white);
      QFont font = painter.font();
      font.setPointSize(8);
      font.setBold(monitor.isPrimary);
      painter.setFont(font);
      
      QString label = QString::number(i + 1);
      if (monitor.isPrimary) {
        label += "*";
      }
      
      painter.drawText(rect, Qt::AlignCenter, label);
    }
  }
}
