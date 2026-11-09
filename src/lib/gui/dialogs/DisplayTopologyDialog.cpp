// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "DisplayTopologyDialog.h"
#include "gui/ExperienceStyle.h"
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QPainter>
#include <QTableWidget>
#include <QVBoxLayout>
namespace {
class MonitorCanvas final : public QWidget
{
public:
  explicit MonitorCanvas(const deskflow::gui::MonitorTopology &topology, QWidget *parent)
      : QWidget(parent),
        m_topology(topology)
  {
    setMinimumHeight(150);
    setAccessibleName(tr("Local monitor geometry preview"));
  }

protected:
  void paintEvent(QPaintEvent *) override
  {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), palette().color(QPalette::Base));
    if (!m_topology.error.isEmpty() || m_topology.bounds.isEmpty())
      return;
    const auto b = m_topology.bounds;
    const double scale = qMin((width() - 40.0) / b.width(), (height() - 40.0) / b.height());
    QTransform t;
    t.translate(width() / 2.0, height() / 2.0);
    t.scale(scale, scale);
    t.translate(-QRectF(b).center().x(), -QRectF(b).center().y());
    int index = 0;
    for (const auto &m : m_topology.monitors) {
      const auto r = t.mapRect(QRectF(m.logical)).adjusted(2, 2, -2, -2);
      const bool dark = palette().color(QPalette::Window).lightness() < 128;
      p.setPen(QPen(palette().color(QPalette::Highlight), 2));
      p.setBrush(dark ? QColor("#26364c") : QColor("#eef4fc"));
      p.drawRoundedRect(r, 6, 6);
      auto numberFont = font();
      numberFont.setBold(true);
      numberFont.setPointSizeF(numberFont.pointSizeF() + 4);
      p.setFont(numberFont);
      p.setPen(dark ? QColor("#e7edf5") : QColor("#1c293a"));
      p.drawText(r, Qt::AlignCenter, QString::number(++index));
    }
  }

private:
  deskflow::gui::MonitorTopology m_topology;
};
} // namespace
DisplayTopologyDialog::DisplayTopologyDialog(const deskflow::gui::MonitorTopology &topology, QWidget *parent)
    : QDialog(parent)
{
  deskflow::gui::applyExperienceStyle(this);
  setWindowTitle(tr("Local displays and protocol limits"));
  resize(700, 490);
  setMinimumSize(560, 440);
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(24, 24, 24, 20);
  layout->setSpacing(12);
  auto label = [&](const QString &text) {
    auto *l = new QLabel(text, this);
    l->setWordWrap(true);
    l->setTextFormat(Qt::PlainText);
    layout->addWidget(l);
    return l;
  };
  label(
      tr("Display positions below are local logical coordinates. Scale is shown separately. No monitor names, serial "
         "numbers or screen content are read.")
  );
  layout->addWidget(new MonitorCanvas(topology, this), 1);
  auto *table = new QTableWidget(topology.monitors.size(), 4, this);
  table->setObjectName("monitorGeometryTable");
  table->setHorizontalHeaderLabels({tr("Display"), tr("Position"), tr("Logical size"), tr("Scale")});
  table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  table->verticalHeader()->hide();
  table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  int row = 0;
  for (const auto &m : topology.monitors) {
    const QStringList values{
        QString::number(row + 1), QString("%1, %2").arg(m.logical.x()).arg(m.logical.y()),
        QString("%1 × %2").arg(m.logical.width()).arg(m.logical.height()), QString::number(m.scale * 100, 'f', 0) + "%"
    };
    for (int col = 0; col < 4; ++col)
      table->setItem(row, col, new QTableWidgetItem(values[col]));
    ++row;
  }
  table->resizeRowsToContents();
  const auto rows = qMin(4, table->rowCount());
  int tableHeight = table->horizontalHeader()->sizeHint().height() + 4;
  for (int i = 0; i < rows; ++i)
    tableHeight += table->rowHeight(i);
  table->setFixedHeight(tableHeight);
  layout->addWidget(table);
  auto *limit = label(
      !topology.error.isEmpty()      ? tr("Display geometry is unavailable or invalid.")
      : topology.legacyRangeExceeded ? tr("This geometry exceeds the current core's signed 16-bit coordinate range. It "
                                          "cannot be applied as a legacy desktop.")
      : topology.rectangularAggregate()
          ? tr("These displays form one rectangle. The current Deskflow protocol can describe an aggregate desktop, "
               "but input coordinates still require platform validation.")
          : tr("Gaps, overlaps or mixed scales cannot be represented accurately by the current aggregate-only "
               "protocol. Per-monitor routing remains unavailable; this preview does not enable it.")
  );
  limit->setObjectName("monitorProtocolLimit");
  label(
      tr("Arrange computers edits computer-to-computer edges. Use operating system display settings to rearrange local "
         "monitors. Remote monitor geometry has not been exchanged.")
  );
  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
  layout->addWidget(buttons);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}
