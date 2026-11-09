// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "DesktopLayoutEditor.h"
#include "DisplayTopologyDialog.h"
#include "gui/ExperienceStyle.h"
#include "gui/config/DesktopLayout.h"
#include "gui/config/MonitorTopology.h"
#include <QComboBox>
#include <QCryptographicHash>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QGridLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QShortcut>
#include <QSignalBlocker>
#include <QToolButton>
#include <algorithm>

using deskflow::experience::Edge;

namespace {
// Presentation only: retain the existing one-decimal editing precision and
// model values. Whole logical coordinates should read as 1920, not 1920.0.
class LayoutNumberSpinBox final : public QDoubleSpinBox
{
public:
  using QDoubleSpinBox::QDoubleSpinBox;

protected:
  QString textFromValue(double value) const override
  {
    auto text = QDoubleSpinBox::textFromValue(value);
    const auto suffix = locale().decimalPoint() + locale().zeroDigit();
    if (text.endsWith(suffix))
      text.chop(suffix.size());
    return text;
  }
};
} // namespace

LayoutCanvas::LayoutCanvas(deskflow::experience::LayoutModel &model, QWidget *parent) : QWidget(parent), m_model(model)
{
  setObjectName("layoutCanvas");
  setMinimumSize(320, 240);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  setFocusPolicy(Qt::StrongFocus);
  setAccessibleName(tr("Computer layout. Select a computer, then drag or use arrow keys to move it."));
  setToolTip(
      tr("Drag to arrange. Edges snap together. Arrow keys move 10 units; Shift moves 1 unit. Page Up/Down selects a "
         "computer.")
  );
}

QTransform LayoutCanvas::transform() const
{
  QRectF bounds;
  for (const auto &d : m_model.desktops())
    bounds = bounds.united({d.x, d.y, d.width, d.height});
  if (bounds.isEmpty())
    return {};
  const int reserved = unknownAreaHeight();
  const double usableHeight = qMax(100, height() - reserved);
  const double margin = m_monitorMode ? 40. : 80.;
  const double scale = std::min((width() - margin) / bounds.width(), (usableHeight - margin) / bounds.height());
  QTransform result;
  result.translate(width() / 2.0, usableHeight / 2.0);
  result.scale(scale, scale);
  result.translate(-bounds.center().x(), -bounds.center().y());
  return result;
}
void LayoutCanvas::setDisplayPresentation(QMap<QString, DisplayPresentation> presentation)
{
  m_presentation = std::move(presentation);
  update();
}
void LayoutCanvas::setUnknownDevices(QMap<QString, QString> devices)
{
  m_unknownDevices = std::move(devices);
  setMinimumHeight(m_monitorMode ? qMax(160, 100 + unknownAreaHeight()) : qMax(240, 180 + unknownAreaHeight()));
  update();
}
int LayoutCanvas::unknownAreaHeight() const
{
  const int columns = qMax(1, width() / 200), cardWidth = (width() - 32) / columns - 8;
  int height = 0, rowHeight = 0, index = 0;
  for (const auto &text : m_unknownDevices) {
    rowHeight = qMax(
        rowHeight,
        qMax(
            56,
            fontMetrics().boundingRect(QRect(0, 0, qMax(1, cardWidth - 16), 10000), Qt::TextWordWrap, text).height() +
                16
        )
    );
    if (++index % columns == 0) {
      height += rowHeight + 12;
      rowHeight = 0;
    }
  }
  return height + (rowHeight ? rowHeight + 12 : 0);
}
QRect LayoutCanvas::unknownDeviceRect(const QString &device) const
{
  const auto keys = m_unknownDevices.keys();
  const auto index = keys.indexOf(device);
  if (index < 0)
    return {};
  const int columns = qMax(1, width() / 200);
  const int cardWidth = (width() - 32) / columns;
  int y = height() - unknownAreaHeight() + 4, rowHeight = 0;
  for (int row = 0; row <= index / columns; ++row) {
    rowHeight = 56;
    for (int i = row * columns; i < qMin((row + 1) * columns, int(keys.size())); ++i)
      rowHeight = qMax(
          rowHeight,
          fontMetrics()
                  .boundingRect(
                      QRect(0, 0, qMax(1, cardWidth - 24), 10000), Qt::TextWordWrap, m_unknownDevices[keys[i]]
                  )
                  .height() +
              16
      );
    if (row < index / columns)
      y += rowHeight + 12;
  }
  return {16 + int(index % columns) * cardWidth, y, cardWidth - 8, rowHeight};
}

QRectF LayoutCanvas::desktopRect(const std::string &name) const
{
  for (const auto &d : m_model.desktops())
    if (d.name == name)
      return transform().mapRect(QRectF(d.x, d.y, d.width, d.height));
  return {};
}

void LayoutCanvas::setSelected(int index)
{
  m_selected = index;
  if (index >= 0 && index < static_cast<int>(m_model.desktops().size()))
    setAccessibleDescription(tr("Selected computer: %1")
                                 .arg(
                                     QString::fromStdString(
                                         m_model.desktops()[index].displayName.empty()
                                             ? m_model.desktops()[index].name
                                             : m_model.desktops()[index].displayName
                                     )
                                 ));
  update();
}

void LayoutCanvas::paintEvent(QPaintEvent *)
{
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);
  const bool dark = palette().color(QPalette::Window).lightness() < 128;
  p.setPen(QPen(dark ? QColor("#3a424e") : QColor("#dfe4ea"), 1));
  p.setBrush(dark ? QColor("#1b2028") : QColor("#f8f9fb"));
  p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 10, 10);
  const auto mapping = m_dragging ? m_dragTransform : transform();
  const auto &desktops = m_model.desktops();
  const bool valid = m_model.validate().valid();
  QMap<QString, QRectF> groups;
  QMap<QString, int> groupCounts;
  for (const auto &d : desktops) {
    const auto item = m_presentation.value(QString::fromStdString(d.name));
    if (item.deviceId.isEmpty())
      continue;
    QRectF r(d.x, d.y, d.width, d.height);
    if (m_dragging && m_dragGroups && m_selected >= 0 && m_selected < int(desktops.size()) &&
        item.deviceId == m_presentation.value(QString::fromStdString(desktops[m_selected].name)).deviceId)
      r.translate(m_dragDelta);
    groups[item.deviceId] = groups[item.deviceId].united(mapping.mapRect(r));
    ++groupCounts[item.deviceId];
  }
  for (auto it = groups.cbegin(); it != groups.cend(); ++it) {
    if (groupCounts.value(it.key()) < 2)
      continue;
    const auto hash = QCryptographicHash::hash(it.key().toUtf8(), QCryptographicHash::Sha256);
    QColor outline = QColor::fromHsv((uchar(hash.front()) * 47) % 360, 36, dark ? 190 : 170);
    QColor tint = outline;
    outline.setAlpha(dark ? 88 : 64);
    tint.setAlpha(dark ? 14 : 10);
    p.setPen(QPen(outline, 1.25, Qt::SolidLine));
    p.setBrush(tint);
    p.drawRoundedRect(it.value().adjusted(-8, -8, 8, 8), 10, 10);
    // Device names belong inside each screen. An outside group caption can be
    // covered by a screen on the computer above it in a valid arrangement.
  }
  std::vector<int> order;
  QVector<QRectF> renderedRects;
  for (int i = 0; i < static_cast<int>(desktops.size()); ++i)
    if (i != m_selected)
      order.push_back(i);
  if (m_selected >= 0 && m_selected < static_cast<int>(desktops.size()))
    order.push_back(m_selected);
  for (const int i : order) {
    const auto &d = desktops[i];
    QRectF r(d.x, d.y, d.width, d.height);
    const auto presentation = m_presentation.value(QString::fromStdString(d.name));
    const bool sameGroup =
        m_dragGroups && !presentation.deviceId.isEmpty() && m_selected >= 0 && m_selected < int(desktops.size()) &&
        presentation.deviceId == m_presentation.value(QString::fromStdString(desktops[m_selected].name)).deviceId;
    if (m_dragging && (i == m_selected || sameGroup))
      r.translate(m_dragDelta);
    r = mapping.mapRect(r).adjusted(3, 3, -3, -3);
    renderedRects.append(r);
    const bool selected = i == m_selected;
    const QColor invalidInk = dark ? QColor("#ffbcb1") : QColor("#a13927");
    const QColor outline = !valid && m_presentation.isEmpty() ? invalidInk
                           : selected                         ? (dark ? QColor("#78a7ff") : QColor("#2f6fed"))
                                                              : (dark ? QColor("#596474") : QColor("#aeb8c5"));
    if (selected) {
      QColor halo = outline;
      halo.setAlpha(dark ? 42 : 30);
      p.setPen(QPen(halo, 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
      p.setBrush(Qt::NoBrush);
      p.drawRoundedRect(r.adjusted(-2, -2, 2, 2), 8, 8);
    }
    p.setPen(QPen(outline, selected ? 2.0 : 1.15, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(
        selected ? (dark ? QColor("#22314a") : QColor("#edf3ff")) : (dark ? QColor("#242a33") : QColor("#ffffff"))
    );
    p.drawRoundedRect(r, 7, 7);
    if (selected) {
      p.setPen(Qt::NoPen);
      p.setBrush(outline);
      p.drawRoundedRect(QRectF(r.left() + 12, r.top() + 1, qMax<qreal>(22, r.width() - 24), 2.5), 1.25, 1.25);
    }
    p.save();
    p.setClipRect(r.adjusted(6, 6, -6, -6));
    p.setPen(dark ? QColor("#f1f3f7") : QColor("#202734"));
    QFont nameFont = font();
    nameFont.setBold(true);
    nameFont.setPointSizeF(font().pointSizeF() + 1);
    p.setFont(nameFont);
    const QString name = presentation.deviceId.isEmpty()
                             ? QString::fromStdString(d.displayName.empty() ? d.name : d.displayName)
                             : presentation.number;
    if (!presentation.deviceId.isEmpty()) {
      nameFont.setPointSizeF(font().pointSizeF() + 10);
      p.setFont(nameFont);
    }
    const auto text = p.fontMetrics().elidedText(name, Qt::ElideMiddle, std::max(0, int(r.width()) - 24));
    if (!presentation.deviceId.isEmpty()) {
      const qreal detailHeight = fontMetrics().height(), gap = 4;
      const bool compact = r.height() < p.fontMetrics().height() + 2 * detailHeight + 3 * gap + 12;
      if (compact) {
        nameFont = font();
        nameFont.setBold(true);
        p.setFont(nameFont);
      }
      const qreal numberHeight = p.fontMetrics().height();
      const bool showDetail = !compact;
      const qreal blockHeight = numberHeight + detailHeight + gap + (showDetail ? detailHeight + gap : 0);
      const qreal top = r.center().y() - blockHeight / 2;
      if (r.height() < blockHeight + 12) {
        // One intact line is preferable to two half-visible lines. Preserve the
        // screen number and elide only the device name; full details stay in the
        // adjacent inspector and the accessible selection description.
        p.setFont(font());
        const auto suffix = QStringLiteral(" · ") + presentation.number;
        const int nameWidth = qMax(0, int(r.width()) - 16 - p.fontMetrics().horizontalAdvance(suffix));
        p.drawText(
            r.adjusted(8, 0, -8, 0), Qt::AlignCenter,
            p.fontMetrics().elidedText(presentation.deviceName, Qt::ElideMiddle, nameWidth) + suffix
        );
        p.restore();
        continue;
      }
      p.setFont(font());
      const bool narrow = r.width() < p.fontMetrics().horizontalAdvance(presentation.deviceName) + 24;
      if (narrow) {
        p.setFont(nameFont);
        p.drawText(r, Qt::AlignCenter, presentation.number);
        p.restore();
        continue;
      }
      p.drawText(
          QRectF(r.x() + 8, top, r.width() - 16, detailHeight), Qt::AlignCenter,
          p.fontMetrics().elidedText(presentation.deviceName, Qt::ElideMiddle, qMax(0, int(r.width()) - 16))
      );
      p.setFont(nameFont);
      p.drawText(
          QRectF(r.x() + 8, top + detailHeight + gap, r.width() - 16, numberHeight), Qt::AlignCenter,
          p.fontMetrics().elidedText(presentation.number, Qt::ElideRight, qMax(0, int(r.width()) - 16))
      );
      if (showDetail) {
        p.setFont(font());
        p.drawText(
            QRectF(r.x() + 8, top + numberHeight + detailHeight + 2 * gap, r.width() - 16, detailHeight),
            Qt::AlignCenter,
            p.fontMetrics().elidedText(presentation.detail, Qt::ElideRight, qMax(0, int(r.width()) - 16))
        );
      }
    } else {
      p.drawText(r.adjusted(12, 0, -12, -4), Qt::AlignCenter, text);
      p.setFont(font());
    }
    if (presentation.deviceId.isEmpty() && r.height() > 65) {
      const auto detail =
          presentation.deviceId.isEmpty() ? tr("%1 × %2").arg(d.width).arg(d.height) : presentation.detail;
      p.drawText(
          r.adjusted(8, 32, -8, -4), Qt::AlignCenter,
          p.fontMetrics().elidedText(detail, Qt::ElideRight, qMax(0, int(r.width()) - 16))
      );
    }
    if (presentation.deviceId.isEmpty() && r.height() > 80 && r.width() > 60)
      p.drawText(
          r.adjusted(12, 10, -12, -10), Qt::AlignTop | Qt::AlignLeft,
          presentation.number.isEmpty() ? QString::number(i + 1) : presentation.number
      );
    p.restore();
  }
  // Invalid drafts remain editable, so make the overlapping area explicit
  // instead of leaving two normal-looking cards stacked on top of each other.
  for (int i = 0; i < renderedRects.size(); ++i) {
    for (int j = i + 1; j < renderedRects.size(); ++j) {
      const auto overlap = renderedRects[i].intersected(renderedRects[j]);
      if (overlap.width() <= 1 || overlap.height() <= 1)
        continue;
      QColor warning = dark ? QColor("#ffb4a9") : QColor("#b23b32");
      QColor fill = warning;
      fill.setAlpha(dark ? 30 : 18);
      p.setPen(Qt::NoPen);
      p.setBrush(fill);
      p.drawRoundedRect(overlap.adjusted(1, 1, -1, -1), 5, 5);
      p.save();
      p.setClipRect(overlap.adjusted(1, 1, -1, -1));
      QColor hatch = warning;
      hatch.setAlpha(dark ? 110 : 76);
      p.setPen(QPen(hatch, 1, Qt::SolidLine, Qt::RoundCap));
      for (qreal x = overlap.left() - overlap.height(); x < overlap.right(); x += 7)
        p.drawLine(QPointF(x, overlap.bottom()), QPointF(x + overlap.height(), overlap.top()));
      p.restore();
      p.setPen(QPen(warning, 1.35, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
      p.setBrush(Qt::NoBrush);
      p.drawRoundedRect(overlap.adjusted(1, 1, -1, -1), 5, 5);
    }
  }
  // Green segments show the exact shared, reciprocal edge intervals.
  if (!m_dragging && !m_monitorMode) {
    p.setPen(QPen(dark ? QColor("#5bd7af") : QColor("#218266"), 4, Qt::SolidLine, Qt::RoundCap));
    for (const auto &link : m_model.validate().links) {
      if (link.edge != Edge::Right && link.edge != Edge::Down)
        continue;
      const auto d = std::find_if(desktops.begin(), desktops.end(), [&](const auto &d) { return d.name == link.from; });
      if (d == desktops.end())
        continue;
      QPointF a, b;
      if (link.edge == Edge::Right) {
        a = {d->x + d->width, d->y + d->height * link.sourceStart / 100};
        b = {a.x(), d->y + d->height * link.sourceEnd / 100};
      } else {
        a = {d->x + d->width * link.sourceStart / 100, d->y + d->height};
        b = {d->x + d->width * link.sourceEnd / 100, a.y()};
      }
      p.drawLine(mapping.map(a), mapping.map(b));
    }
  }
  if (!m_dragging && m_monitorMode) {
    p.setPen(QPen(dark ? QColor("#5bd7af") : QColor("#218266"), 4, Qt::SolidLine, Qt::RoundCap));
    for (const auto &edge : m_monitorEdges)
      p.drawLine(mapping.map(edge));
  }
  for (auto it = m_unknownDevices.cbegin(); it != m_unknownDevices.cend(); ++it) {
    const auto r = unknownDeviceRect(it.key());
    p.setPen(QPen(palette().color(QPalette::Mid), 1, Qt::SolidLine));
    p.setBrush(palette().color(QPalette::Window));
    p.drawRoundedRect(r, 7, 7);
    p.setPen(dark ? QColor("#f1f3f7") : QColor("#202734"));
    p.drawText(r.adjusted(8, 4, -8, -4), Qt::AlignCenter | Qt::TextWordWrap, it.value());
  }
  if (hasFocus()) {
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(palette().color(QPalette::Highlight), 2, Qt::SolidLine));
    p.drawRoundedRect(rect().adjusted(4, 4, -4, -4), 10, 10);
  }
}

void LayoutCanvas::mousePressEvent(QMouseEvent *event)
{
  if (event->button() != Qt::LeftButton)
    return;
  for (auto it = m_unknownDevices.cbegin(); it != m_unknownDevices.cend(); ++it)
    if (unknownDeviceRect(it.key()).contains(event->position().toPoint())) {
      m_selected = -1;
      setFocus();
      Q_EMIT unknownDeviceSelected(it.key());
      update();
      return;
    }
  const auto &desktops = m_model.desktops();
  std::vector<int> order;
  if (m_selected >= 0 && m_selected < static_cast<int>(desktops.size()))
    order.push_back(m_selected);
  for (int i = static_cast<int>(desktops.size()) - 1; i >= 0; --i)
    if (i != m_selected)
      order.push_back(i);
  for (const int i : order) {
    if (!desktopRect(desktops[i].name).contains(event->position()))
      continue;
    m_selected = i;
    m_dragging = m_editable;
    m_dragTransform = transform();
    m_press = m_dragTransform.inverted().map(event->position());
    m_dragDelta = {};
    setFocus();
    Q_EMIT selected(i);
    update();
    break;
  }
}
void LayoutCanvas::mouseMoveEvent(QMouseEvent *event)
{
  if (!m_dragging)
    return;
  m_dragDelta = m_dragTransform.inverted().map(event->position()) - m_press;
  update();
}
void LayoutCanvas::mouseReleaseEvent(QMouseEvent *event)
{
  if (!m_dragging || event->button() != Qt::LeftButton)
    return;
  m_dragging = false;
  m_dragDelta = m_dragTransform.inverted().map(event->position()) - m_press;
  const auto d = m_model.desktops().at(m_selected);
  if (!m_dragDelta.isNull())
    Q_EMIT moved(
        QString::fromStdString(d.name), std::clamp(d.x + m_dragDelta.x(), -1e7, 1e7),
        std::clamp(d.y + m_dragDelta.y(), -1e7, 1e7), std::min(1000.0, 12 / m_dragTransform.m11())
    );
  update();
}
void LayoutCanvas::keyPressEvent(QKeyEvent *event)
{
  if (m_selected < 0 || m_selected >= static_cast<int>(m_model.desktops().size()))
    return;
  if (event->key() == Qt::Key_PageDown || event->key() == Qt::Key_PageUp) {
    const int count = static_cast<int>(m_model.desktops().size());
    setSelected((m_selected + (event->key() == Qt::Key_PageDown ? 1 : count - 1)) % count);
    Q_EMIT selected(m_selected);
    return;
  }
  if (!m_editable) {
    QWidget::keyPressEvent(event);
    return;
  }
  QPointF delta;
  const int step = event->modifiers().testFlag(Qt::ShiftModifier) ? 1 : 10;
  switch (event->key()) {
  case Qt::Key_Left:
    delta.setX(-step);
    break;
  case Qt::Key_Right:
    delta.setX(step);
    break;
  case Qt::Key_Up:
    delta.setY(-step);
    break;
  case Qt::Key_Down:
    delta.setY(step);
    break;
  default:
    QWidget::keyPressEvent(event);
    return;
  }
  const auto d = m_model.desktops().at(m_selected);
  Q_EMIT moved(
      QString::fromStdString(d.name), std::clamp(d.x + delta.x(), -1e7, 1e7), std::clamp(d.y + delta.y(), -1e7, 1e7), 0
  );
}

DesktopLayoutEditor::DesktopLayoutEditor(std::vector<deskflow::experience::Desktop> desktops, QWidget *parent)
    : QDialog(parent),
      m_model(std::move(desktops), true),
      m_canvas(new LayoutCanvas(m_model, this)),
      m_computer(new QComboBox(this)),
      m_validation(new QLabel(this)),
      m_undo(new QPushButton(tr("Undo"), this)),
      m_save(new QPushButton(tr("Use layout"), this))
{
  deskflow::gui::applyExperienceStyle(this);
  setWindowTitle(tr("Arrange computers"));
  resize(790, 660);
  setMinimumSize(640, 580);
  auto *layout = new QVBoxLayout(this);
  layout->setSizeConstraint(QLayout::SetMinimumSize);
  layout->setContentsMargins(20, 20, 20, 16);
  layout->setSpacing(10);
  auto *title = new QLabel(tr("Place your computers"), this);
  title->setObjectName("layoutHeading");
  auto titleFont = title->font();
  titleFont.setPointSizeF(titleFont.pointSizeF() + 7);
  titleFont.setBold(true);
  title->setFont(titleFont);
  layout->addWidget(title);
  auto *help = new QLabel(tr("Drag a computer beside another. Green edges show where the pointer can cross."), this);
  help->setObjectName("layoutHelp");
  help->setWordWrap(true);
  auto *helpRow = new QHBoxLayout;
  helpRow->addWidget(help, 1);
  auto *displays = new QPushButton(tr("Local displays"), this);
  displays->setObjectName("inspectLocalDisplays");
  helpRow->addWidget(displays);
  layout->addLayout(helpRow);
  connect(displays, &QPushButton::clicked, this, [this] {
    DisplayTopologyDialog dialog(deskflow::gui::MonitorTopology::local(), this);
    dialog.exec();
  });
  layout->addWidget(m_canvas, 1);
  // A fixed spacer also reserves room for style-specific combo focus frames;
  // a margin on the nested grid alone is not respected by every Qt style.
  layout->addSpacing(12);
  auto *selectionRow = new QHBoxLayout;
  m_computer->setObjectName("layoutComputer");
  m_computer->setAccessibleName(tr("Selected computer"));
  m_computer->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
  m_computer->setMinimumContentsLength(15);
  for (const auto &d : m_model.desktops())
    m_computer->addItem(QString::fromStdString(d.name));
  selectionRow->addWidget(new QLabel(tr("Computer"), this));
  selectionRow->addWidget(m_computer, 1);
  layout->addLayout(selectionRow);
  m_monitorDetails = new QLabel(this);
  m_monitorDetails->setObjectName("detectedDisplayDetails");
  m_monitorDetails->setWordWrap(true);
  m_monitorDetails->setTextFormat(Qt::PlainText);
  m_monitorDetails->hide();
  layout->addWidget(m_monitorDetails);
  auto *advancedToggle = new QToolButton(this);
  advancedToggle->setObjectName("layoutAdvancedToggle");
  advancedToggle->setText(tr("Advanced details"));
  advancedToggle->setCheckable(true);
  advancedToggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  advancedToggle->setArrowType(Qt::RightArrow);
  auto *advancedScroll = new QScrollArea(this);
  advancedScroll->setObjectName("layoutAdvancedPanel");
  advancedScroll->setWidgetResizable(true);
  advancedScroll->setFrameShape(QFrame::NoFrame);
  advancedScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  advancedScroll->setMinimumHeight(140);
  advancedScroll->setMaximumHeight(180);
  auto *advanced = new QWidget(advancedScroll);
  advanced->setObjectName("layoutAdvancedFields");
  auto *advancedLayout = new QVBoxLayout(advanced);
  advancedLayout->setContentsMargins(0, 0, 12, 0);
  advancedLayout->setSizeConstraint(QLayout::SetMinimumSize);
  advancedLayout->setSpacing(10);
  advancedScroll->setWidget(advanced);
  advancedScroll->hide();
  connect(advancedToggle, &QToolButton::toggled, this, [advancedScroll, advancedToggle](bool shown) {
    advancedScroll->setVisible(shown);
    advancedToggle->setArrowType(shown ? Qt::DownArrow : Qt::RightArrow);
  });
  auto *fields = new QGridLayout;
  fields->setContentsMargins(0, 0, 0, 0);
  fields->setVerticalSpacing(10);
  fields->setHorizontalSpacing(14);
  int col = 0;
  const auto makeSpin = [&](const QString &name, const QString &label, double low, double high) {
    auto *spin = new LayoutNumberSpinBox(advanced);
    spin->setObjectName(name);
    spin->setAccessibleName(label);
    spin->setRange(low, high);
    spin->setDecimals(1);
    spin->setKeyboardTracking(false);
    // Two labelled fields per row keep full numeric values and the stepper
    // reachable at the supported 640 px width, including translated labels.
    auto *fieldLabel = new QLabel(label, this);
    fieldLabel->setBuddy(spin);
    fields->addWidget(fieldLabel, col / 2, 2 * (col % 2));
    fields->addWidget(spin, col / 2, 2 * (col % 2) + 1);
    ++col;
    return spin;
  };
  m_x = makeSpin("layoutX", tr("X"), -1e7, 1e7);
  m_y = makeSpin("layoutY", tr("Y"), -1e7, 1e7);
  m_width = makeSpin("layoutWidth", tr("Width"), 1, 1e6);
  m_height = makeSpin("layoutHeight", tr("Height"), 1, 1e6);
  fields->setColumnStretch(1, 1);
  fields->setColumnStretch(3, 1);
  advancedLayout->addLayout(fields);
  auto *nameRow = new QHBoxLayout;
  auto *nameLabel = new QLabel(tr("Display name"), this);
  m_displayName = new QLineEdit(this);
  m_displayName->setObjectName("layoutDisplayName");
  m_displayName->setAccessibleName(nameLabel->text());
  nameLabel->setBuddy(m_displayName);
  m_displayName->setPlaceholderText(tr("Optional · Chinese and other languages are supported"));
  nameRow->addWidget(nameLabel);
  nameRow->addWidget(m_displayName, 1);
  layout->addLayout(nameRow);
  auto *protocolRow = new QHBoxLayout;
  auto *protocolLabel = new QLabel(tr("Protocol name"), this);
  m_protocolName = new QLineEdit(this);
  m_protocolName->setObjectName("layoutProtocolName");
  m_protocolName->setReadOnly(true);
  m_protocolName->setAccessibleName(protocolLabel->text());
  protocolLabel->setBuddy(m_protocolName);
  protocolRow->addWidget(protocolLabel);
  protocolRow->addWidget(m_protocolName, 1);
  advancedLayout->addLayout(protocolRow);
  connect(m_displayName, &QLineEdit::textEdited, this, [this](const QString &text) {
    const auto id = m_model.desktops().at(m_computer->currentIndex()).name;
    m_model.setDisplayName(id, text.toStdString());
    refresh();
  });
  auto *scope = new QLabel(
      tr("Sizes are logical units: pixels ÷ display scale. Each rectangle is one computer’s whole desktop, including "
         "all its monitors."),
      this
  );
  scope->setObjectName("layoutScope");
  scope->setWordWrap(true);
  advancedLayout->addWidget(scope);
  layout->addWidget(advancedToggle, 0, Qt::AlignLeft);
  layout->addWidget(advancedScroll);
  m_validation->setObjectName("layoutValidation");
  m_validation->setWordWrap(true);
  m_validation->setTextFormat(Qt::PlainText);
  m_validation->setMinimumHeight(40);
  layout->addWidget(m_validation);
  auto *buttons = new QDialogButtonBox(this);
  m_buttons = buttons;
  buttons->setObjectName("layoutEditorActions");
  m_undo->setObjectName("layoutUndo");
  m_save->setObjectName("layoutSave");
  m_save->setProperty("primaryAction", true);
  buttons->addButton(m_undo, QDialogButtonBox::ActionRole);
  buttons->addButton(QDialogButtonBox::Cancel);
  buttons->addButton(m_save, QDialogButtonBox::AcceptRole);
  m_save->setDefault(true);
  for (auto *button : buttons->buttons())
    button->setMinimumHeight(34);
  layout->addWidget(buttons);
  connect(buttons, &QDialogButtonBox::accepted, this, &DesktopLayoutEditor::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &DesktopLayoutEditor::reject);
  connect(m_undo, &QPushButton::clicked, this, &DesktopLayoutEditor::undo);
  auto *undoShortcut = new QShortcut(QKeySequence::Undo, this);
  connect(undoShortcut, &QShortcut::activated, this, &DesktopLayoutEditor::undo);
  connect(m_computer, &QComboBox::currentIndexChanged, this, [this](int index) {
    m_canvas->setSelected(index);
    updateGeometryFields();
  });
  connect(m_canvas, &LayoutCanvas::selected, m_computer, &QComboBox::setCurrentIndex);
  connect(m_canvas, &LayoutCanvas::moved, this, [this](const QString &name, double x, double y, double threshold) {
    moveDesktop(name, x, y, threshold > 0, threshold);
  });
  connect(m_x, &QDoubleSpinBox::valueChanged, this, [this](double x) {
    const auto d = m_model.desktops().at(m_computer->currentIndex());
    moveDesktop(QString::fromStdString(d.name), x, d.y, false);
  });
  connect(m_y, &QDoubleSpinBox::valueChanged, this, [this](double y) {
    const auto d = m_model.desktops().at(m_computer->currentIndex());
    moveDesktop(QString::fromStdString(d.name), d.x, y, false);
  });
  connect(m_width, &QDoubleSpinBox::valueChanged, this, [this](double width) {
    const auto d = m_model.desktops().at(m_computer->currentIndex());
    resizeDesktop(QString::fromStdString(d.name), width, d.height);
  });
  connect(m_height, &QDoubleSpinBox::valueChanged, this, [this](double height) {
    const auto d = m_model.desktops().at(m_computer->currentIndex());
    resizeDesktop(QString::fromStdString(d.name), d.width, height);
  });
  refresh();
}
void DesktopLayoutEditor::setRoutingError(const QString &error)
{
  if (error.isEmpty())
    return;
  m_validation->setText(tr("Cannot save · %1. Correct the fields or undo.").arg(error));
  m_validation->setAccessibleName(m_validation->text());
}
void DesktopLayoutEditor::setMonitorDetails(QMap<QString, QString> details)
{
  m_monitorDetailText = std::move(details);
  updateGeometryFields();
}
void DesktopLayoutEditor::setMonitorMode(bool value)
{
  m_monitorMode = value;
  m_canvas->setMonitorMode(value);
  m_monitorDetails->setVisible(value);
  findChild<QLabel *>("layoutHeading")->setText(value ? tr("Place individual displays") : tr("Place your computers"));
  findChild<QLabel *>("layoutHelp")
      ->setText(
          value ? tr("Drag each display. Resolution is detected automatically. Cross-computer switching uses exposed "
                     "shared edges; edges already joining local displays remain controlled by the operating system.")
                : tr("Drag a computer beside another. Green edges show where the pointer can cross.")
      );
  findChild<QLabel *>("layoutScope")
      ->setText(
          value ? tr("Each rectangle is one display in logical units. The label shows its current pixel resolution. "
                     "Physical size is unknown when it cannot be verified. This does not change the operating system "
                     "display arrangement.")
                : tr("Sizes are logical units: pixels ÷ display scale. Each rectangle is one computer’s whole desktop, "
                     "including all its monitors.")
      );
  setEditingEnabled(m_editable);
  refresh();
}
void DesktopLayoutEditor::embedInSettings()
{
  m_embedded = true;
  setWindowFlags(Qt::Widget);
  setMinimumSize(0, 0);
  m_canvas->setMinimumHeight(180);
  layout()->setSpacing(8);
  layout()->setContentsMargins(0, 0, 0, 0);
  m_save->hide();
  m_buttons->button(QDialogButtonBox::Cancel)->hide();
}
void DesktopLayoutEditor::addSecondaryAction(QPushButton *button)
{
  m_buttons->addButton(button, QDialogButtonBox::ActionRole);
}
int DesktopLayoutEditor::heightForWidth(int width) const
{
  // A scroll container must be allowed to shrink the flexible canvas, while
  // still reserving the height required by wrapped labels and fixed controls.
  if (m_embedded && layout())
    return qMax(minimumSizeHint().height(), layout()->minimumHeightForWidth(width));
  return QDialog::heightForWidth(width);
}
void DesktopLayoutEditor::setEditingEnabled(bool enabled)
{
  m_editable = enabled;
  for (auto *widget :
       {static_cast<QWidget *>(m_canvas), static_cast<QWidget *>(m_displayName), static_cast<QWidget *>(m_x),
        static_cast<QWidget *>(m_y), static_cast<QWidget *>(m_width), static_cast<QWidget *>(m_height)})
    widget->setEnabled(enabled);
  m_width->setEnabled(enabled && !m_monitorMode);
  m_height->setEnabled(enabled && !m_monitorMode);
  m_displayName->setEnabled(enabled && !m_monitorMode);
  m_undo->setEnabled(enabled && m_model.canUndo());
  m_save->setEnabled(enabled && deskflow::gui::DesktopLayout::draftError(m_model.desktops()).isEmpty());
}
void DesktopLayoutEditor::setDesktops(std::vector<deskflow::experience::Desktop> desktops)
{
  m_model = deskflow::experience::LayoutModel(std::move(desktops), true);
  const QSignalBlocker blocker(m_computer);
  m_computer->clear();
  for (const auto &desktop : m_model.desktops())
    m_computer->addItem(QString::fromStdString(desktop.name));
  m_canvas->setSelected(m_computer->currentIndex());
  refresh();
}
void DesktopLayoutEditor::moveDesktop(const QString &name, double x, double y, bool snap, double threshold)
{
  if (!m_editable)
    return;
  m_model.move(name.toStdString(), x, y, snap, threshold);
  refresh();
}
void DesktopLayoutEditor::resizeDesktop(const QString &name, double width, double height)
{
  if (!m_editable || m_monitorMode)
    return;
  m_model.resize(name.toStdString(), width, height);
  refresh();
}
void DesktopLayoutEditor::undo()
{
  if (m_editable) {
    m_model.undo();
    refresh();
  }
}
void DesktopLayoutEditor::accept()
{
  if (m_embedded) {
    Q_EMIT saveRequested();
    return;
  }
  if (deskflow::gui::DesktopLayout::draftError(m_model.desktops()).isEmpty())
    QDialog::accept();
  else
    refresh();
}
void DesktopLayoutEditor::reject()
{
  if (m_embedded) {
    Q_EMIT cancelRequested();
    return;
  }
  m_model.cancel();
  QDialog::reject();
}
void DesktopLayoutEditor::updateGeometryFields()
{
  const int index = m_computer->currentIndex();
  if (index < 0 || index >= static_cast<int>(m_model.desktops().size()))
    return;
  const auto &d = m_model.desktops()[index];
  m_computer->setToolTip(QString::fromStdString(d.name));
  m_monitorDetails->setText(m_monitorDetailText.value(QString::fromStdString(d.name)));
  const QSignalBlocker nameBlock(m_displayName), pickerBlock(m_computer);
  m_displayName->setText(QString::fromStdString(d.displayName));
  m_protocolName->setText(QString::fromStdString(d.name));
  m_protocolName->setCursorPosition(0);
  m_protocolName->setToolTip(tr("Protocol name: %1 · unchanged by the display name; must match the other computer.")
                                 .arg(QString::fromStdString(d.name)));
  for (int i = 0; i < static_cast<int>(m_model.desktops().size()); ++i) {
    const auto &desktop = m_model.desktops()[i];
    m_computer->setItemText(
        i, QString::fromStdString(desktop.displayName.empty() ? desktop.name : desktop.displayName)
    );
  }
  const QSignalBlocker bx(m_x), by(m_y), bw(m_width), bh(m_height);
  m_x->setValue(d.x);
  m_y->setValue(d.y);
  m_width->setValue(d.width);
  m_height->setValue(d.height);
}
void DesktopLayoutEditor::refresh()
{
  const auto validation = m_model.validate();
  const auto error = deskflow::gui::DesktopLayout::draftError(m_model.desktops());
  const bool valid = error.isEmpty();
  m_save->setEnabled(m_editable && valid);
  m_undo->setEnabled(m_editable && m_model.canUndo());
  m_validation->setText(
      m_monitorMode ? (valid ? tr("Ready · %1 displays. Save to apply this arrangement.").arg(m_model.desktops().size())
                             : tr("Cannot save · %1. Correct the fields or undo.").arg(error))
      : valid       ? tr("Ready · %1 computers · %2 shared edges. Save to apply your changes.")
                    .arg(m_model.desktops().size())
                    .arg(validation.links.size() / 2)
              : tr("Cannot save · %1. Correct the fields or undo.").arg(error)
  );
  m_validation->setAccessibleName(m_validation->text());
  const bool dark = palette().color(QPalette::Window).lightness() < 128;
  m_validation->setStyleSheet(QString("padding:8px; border-radius:6px; color:%1; background:%2;")
                                  .arg(
                                      valid ? (dark ? "#a6e8d3" : "#185e48") : (dark ? "#ffbcb1" : "#8d3125"),
                                      valid ? (dark ? "#253d36" : "#e8f3ee") : (dark ? "#432f30" : "#fceeea")
                                  ));
  updateGeometryFields();
  m_canvas->update();
  Q_EMIT draftChanged();
}
void DesktopLayoutEditor::changeEvent(QEvent *event)
{
  QDialog::changeEvent(event);
  if (m_height && event->type() == QEvent::PaletteChange)
    refresh();
}
