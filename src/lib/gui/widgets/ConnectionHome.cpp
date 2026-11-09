// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "ConnectionHome.h"
#include "ActionButton.h"
#include "FlowLayout.h"
#include "gui/ExperienceStyle.h"
#include <QApplication>
#include <QEvent>
#include <QFontMetricsF>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QShortcut>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

namespace deskflow::gui {
namespace {
enum class DeviceKind
{
  KeyboardMouse,
  Monitor
};

QString monitorIconPath(bool dark)
{
  return QStringLiteral(":/icons/deskflow-%1/actions/22/monitor-duotone.svg")
      .arg(dark ? QStringLiteral("dark") : QStringLiteral("light"));
}

QString inputIconPath(bool dark, bool mouse)
{
  return QStringLiteral(":/icons/deskflow-%1/actions/22/%2-duotone.svg")
      .arg(
          dark ? QStringLiteral("dark") : QStringLiteral("light"),
          mouse ? QStringLiteral("mouse") : QStringLiteral("keyboard")
      );
}

void drawTileSurface(QPainter &p, const QRectF &tile, bool dark)
{
  p.setRenderHint(QPainter::Antialiasing);
  p.setPen(QPen(dark ? QColor("#3c4047") : QColor("#e6e9ee"), 1));
  p.setBrush(dark ? QColor("#2b2e34") : QColor("#f2f4f7"));
  p.drawRoundedRect(tile.adjusted(0.5, 0.5, -0.5, -0.5), 9, 9);
}

void drawMonitorTile(QPainter &p, const QRectF &tile, bool dark)
{
  drawTileSurface(p, tile, dark);
  const QString iconPath = monitorIconPath(dark);
  const QIcon icon(iconPath);
  const int horizontalInset = qMax(6, qRound(tile.width() * 0.14));
  const int verticalInset = qMax(2, qRound(tile.height() * 0.06));
  icon.paint(
      &p, tile.adjusted(horizontalInset, verticalInset, -horizontalInset, -verticalInset).toRect(), Qt::AlignCenter,
      QIcon::Normal
  );
}

void drawInputTile(QPainter &p, const QRectF &tile, bool dark)
{
  drawTileSurface(p, tile, dark);
  // Use the same Phosphor icon family as the monitor. Separate keyboard and
  // mouse silhouettes stay recognizable at Windows DPI scales.
  const int iconSize = qBound(16, qMin(qRound(tile.height() * 0.48), qRound((tile.width() - 11) / 2)), 20);
  const int gap = 3;
  const qreal groupWidth = iconSize * 2 + gap;
  const qreal left = tile.center().x() - groupWidth / 2;
  const qreal top = tile.center().y() - iconSize / 2;
  QIcon(inputIconPath(dark, false))
      .paint(&p, QRectF(left, top, iconSize, iconSize).toRect(), Qt::AlignCenter, QIcon::Normal);
  QIcon(inputIconPath(dark, true))
      .paint(&p, QRectF(left + iconSize + gap, top, iconSize, iconSize).toRect(), Qt::AlignCenter, QIcon::Normal);
}

void drawDeviceTile(QPainter &p, const QRectF &tile, DeviceKind kind, bool dark)
{
  if (kind == DeviceKind::Monitor)
    drawMonitorTile(p, tile, dark);
  else
    drawInputTile(p, tile, dark);
}

class StatusDot final : public QWidget
{
public:
  explicit StatusDot(QWidget *parent) : QWidget(parent)
  {
    setFixedSize(8, 8);
    setProperty("tone", "idle");
  }

protected:
  void paintEvent(QPaintEvent *) override
  {
    const bool dark = QApplication::palette().color(QPalette::Window).lightness() < 128;
    const auto tone = property("tone").toString();
    const QColor color = tone == "error"                       ? QColor(dark ? "#ffb4a9" : "#9b2c24")
                         : tone == "connected"                 ? QColor(dark ? "#8bd5a5" : "#176c3b")
                         : tone == "busy" || tone == "waiting" ? QColor(dark ? "#85baff" : "#005ebc")
                                                               : QColor(dark ? "#8e9198" : "#8a8d93");
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(rect().adjusted(1, 1, -1, -1));
  }
};

class DirectionGlyph final : public QWidget
{
public:
  DirectionGlyph(bool share, QWidget *parent) : QWidget(parent), m_share(share)
  {
    setObjectName("roleDirectionGlyph");
    setMinimumWidth(192);
    setFixedHeight(52);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  }

protected:
  void paintEvent(QPaintEvent *) override
  {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const bool dark = QApplication::palette().color(QPalette::Window).lightness() < 128;
    const QColor accent = dark ? QColor("#8ab4ff") : QColor("#2f6fed");
    const qreal left = qMax<qreal>(0, (width() - 184) / 2);
    if (m_share) {
      drawDeviceTile(p, QRectF(left + 2, 6, 56, 40), DeviceKind::KeyboardMouse, dark);
      drawDeviceTile(p, QRectF(left + 126, 6, 56, 40), DeviceKind::Monitor, dark);
    } else {
      drawDeviceTile(p, QRectF(left + 2, 6, 56, 40), DeviceKind::Monitor, dark);
      drawDeviceTile(p, QRectF(left + 126, 6, 56, 40), DeviceKind::KeyboardMouse, dark);
    }
    p.setPen(QPen(accent, 1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    const qreal start = m_share ? left + 62 : left + 72;
    const qreal finish = m_share ? left + 122 : left + 112;
    p.drawLine(QPointF(start, 26), QPointF(finish, 26));
    const qreal tip = m_share ? finish : start, back = m_share ? finish - 7 : start + 7;
    p.drawLine(QPointF(tip, 26), QPointF(back, 21));
    p.drawLine(QPointF(tip, 26), QPointF(back, 31));
  }

private:
  bool m_share;
};

class ComputerGlyph final : public QWidget
{
public:
  explicit ComputerGlyph(DeviceKind kind, QWidget *parent) : QWidget(parent), m_kind(kind)
  {
    setObjectName("computerGlyph");
    setFixedSize(58, 44);
    setDeviceProperty();
  }
  void setKind(DeviceKind kind)
  {
    if (m_kind == kind)
      return;
    m_kind = kind;
    setDeviceProperty();
    update();
  }

protected:
  void paintEvent(QPaintEvent *) override
  {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const bool dark = QApplication::palette().color(QPalette::Window).lightness() < 128;
    drawDeviceTile(p, rect().adjusted(0, 0, -1, -1), m_kind, dark);
  }

private:
  void setDeviceProperty()
  {
    const bool input = m_kind == DeviceKind::KeyboardMouse;
    setProperty("deviceKind", input ? QStringLiteral("input") : QStringLiteral("display"));
    setAccessibleName(input ? tr("Keyboard and mouse") : tr("Display"));
  }
  DeviceKind m_kind;
};

class DeviceNameLabel final : public QLabel
{
public:
  explicit DeviceNameLabel(QWidget *parent) : QLabel(parent)
  {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  }
  QSize sizeHint() const override
  {
    return {qMin(220, fontMetrics().horizontalAdvance(text())), fontMetrics().height()};
  }
  QSize minimumSizeHint() const override
  {
    return {24, fontMetrics().height()};
  }

protected:
  void paintEvent(QPaintEvent *) override
  {
    QPainter p(this);
    p.setPen(palette().color(QPalette::WindowText));
    p.drawText(
        contentsRect(), Qt::AlignLeft | Qt::AlignVCenter,
        fontMetrics().elidedText(text(), Qt::ElideRight, contentsRect().width())
    );
  }
};

QLabel *label(QWidget *parent, const char *name, int size = 13, bool bold = false)
{
  auto *l = new QLabel(parent);
  l->setObjectName(name);
  l->setWordWrap(true);
  l->setTextFormat(Qt::PlainText);
  auto f = l->font();
  f.setPointSize(size);
  f.setBold(bold);
  l->setFont(f);
  return l;
}

QPushButton *button(QWidget *parent, const char *name)
{
  auto *b = new ActionButton(parent);
  b->setObjectName(name);
  b->setAutoDefault(false);
  b->setMinimumHeight(32);
  auto font = b->font();
  font.setPointSize(12);
  b->setFont(font);
  return b;
}

QPushButton *
navButton(QWidget *parent, const char *name, const QString &text, QStyle::StandardPixmap icon, bool checkable = false)
{
  auto *b = button(parent, name);
  b->setText(text);
  b->setIcon(QApplication::style()->standardIcon(icon));
  b->setIconSize(QSize(16, 16));
  b->setCheckable(checkable);
  b->setProperty("navAction", true);
  b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  b->setMinimumHeight(34);
  return b;
}

QFrame *card(QWidget *parent, const char *name)
{
  auto *frame = new QFrame(parent);
  frame->setObjectName(name);
  frame->setFrameShape(QFrame::NoFrame);
  return frame;
}

QIcon themedIcon(const QString &path, bool dark)
{
  return QIcon(
      QStringLiteral(":/icons/deskflow-%1/%2").arg(dark ? QStringLiteral("dark") : QStringLiteral("light"), path)
  );
}
} // namespace

ConnectionHome::ConnectionHome(QWidget *parent) : QWidget(parent)
{
  setObjectName("connectionHome");
  auto *root = new QHBoxLayout(this);
  root->setContentsMargins(0, 0, 0, 0);
  root->setSpacing(0);

  m_sidebar = new QWidget(this);
  m_sidebar->setObjectName("homeSidebar");
  m_sidebar->setFixedWidth(232);
  auto *sidebar = new QVBoxLayout(m_sidebar);
  sidebar->setContentsMargins(18, 18, 18, 16);
  sidebar->setSpacing(8);
  auto *brandRow = new QWidget(m_sidebar);
  brandRow->setObjectName("sidebarBrandRow");
  auto *brandLayout = new QHBoxLayout(brandRow);
  brandLayout->setContentsMargins(0, 0, 0, 0);
  brandLayout->setSpacing(9);
  auto *brandMark = new QLabel(brandRow);
  brandMark->setObjectName("sidebarBrandMark");
  brandMark->setFixedSize(28, 28);
  brandMark->setPixmap(
      QIcon(QStringLiteral(":/icons/deskflow-light/apps/64/org.deskflow.deskflow-symbolic.svg")).pixmap(24, 24)
  );
  brandLayout->addWidget(brandMark, 0, Qt::AlignVCenter);
  auto *brand = label(brandRow, "sidebarBrand", 17, true);
  brand->setText(QStringLiteral("Deskflow"));
  brandLayout->addWidget(brand, 1);
  sidebar->addWidget(brandRow);
  auto *kicker = label(m_sidebar, "sidebarKicker", 10, true);
  kicker->setText(tr("Keyboard and mouse sharing"));
  kicker->hide();
  sidebar->addWidget(kicker);

  auto *deviceCard = card(m_sidebar, "sidebarDeviceCard");
  auto *deviceLayout = new QVBoxLayout(deviceCard);
  deviceLayout->setContentsMargins(12, 12, 12, 12);
  deviceLayout->setSpacing(5);
  auto *thisComputer = label(deviceCard, "sidebarSection", 11, true);
  thisComputer->setText(tr("This computer"));
  deviceLayout->addWidget(thisComputer);
  m_sidebarDevice = label(deviceCard, "sidebarDevice", 13, true);
  deviceLayout->addWidget(m_sidebarDevice);
  auto *stateRow = new QWidget(deviceCard);
  stateRow->setObjectName("sidebarStateRow");
  auto *stateLayout = new QHBoxLayout(stateRow);
  stateLayout->setContentsMargins(0, 2, 0, 0);
  stateLayout->setSpacing(7);
  m_sidebarDot = new StatusDot(stateRow);
  m_sidebarState = label(stateRow, "sidebarState", 11);
  stateLayout->addWidget(m_sidebarDot, 0, Qt::AlignVCenter);
  stateLayout->addWidget(m_sidebarState, 1);
  deviceLayout->addWidget(stateRow);
  auto *peerLabel = label(deviceCard, "sidebarSection", 11, true);
  peerLabel->setText(tr("Other computer"));
  deviceLayout->addWidget(peerLabel);
  m_sidebarPeer = label(deviceCard, "sidebarPeer", 11);
  deviceLayout->addWidget(m_sidebarPeer);
  sidebar->addWidget(deviceCard);

  auto *navigationLabel = label(m_sidebar, "sidebarSection", 11, true);
  navigationLabel->setText(tr("Workspace"));
  navigationLabel->hide();
  sidebar->addSpacing(6);
  sidebar->addWidget(navigationLabel);
  m_sidebarOverview = navButton(m_sidebar, "sidebarOverview", tr("Overview"), QStyle::SP_ComputerIcon, true);
  m_sidebarComputers = navButton(m_sidebar, "sidebarComputers", tr("Computers"), QStyle::SP_DriveNetIcon);
  m_sidebarConnection = navButton(m_sidebar, "sidebarConnection", tr("Connection"), QStyle::SP_BrowserReload);
  m_sidebarSettings = navButton(m_sidebar, "sidebarSettings", tr("Settings"), QStyle::SP_FileDialogDetailedView);
  m_sidebarDiagnostics =
      navButton(m_sidebar, "sidebarDiagnostics", tr("Diagnostics"), QStyle::SP_MessageBoxInformation);
  for (auto *b : {m_sidebarOverview, m_sidebarComputers, m_sidebarConnection, m_sidebarSettings, m_sidebarDiagnostics})
    sidebar->addWidget(b);
  sidebar->addStretch();
  auto *sidebarFooter = label(m_sidebar, "sidebarFooter", 10);
  sidebarFooter->setText(tr("Mac and Windows"));
  sidebarFooter->hide();
  sidebar->addWidget(sidebarFooter);
  root->addWidget(m_sidebar);

  m_contentHost = new QWidget(this);
  m_contentHost->setObjectName("homeContentHost");
  auto *hostLayout = new QVBoxLayout(m_contentHost);
  hostLayout->setContentsMargins(0, 0, 0, 0);
  hostLayout->setSpacing(0);
  auto *scroll = new QScrollArea(m_contentHost);
  scroll->setObjectName("homeScroll");
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_canvas = new QWidget(scroll);
  m_canvas->setObjectName("homeCanvas");
  auto *canvasLayout = new QVBoxLayout(m_canvas);
  canvasLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
  canvasLayout->setContentsMargins(0, 0, 0, 0);
  canvasLayout->setSpacing(0);
  m_column = new QWidget(m_canvas);
  m_column->setObjectName("homeColumn");
  m_column->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
  auto *columnLayout = new QVBoxLayout(m_column);
  columnLayout->setContentsMargins(0, 0, 0, 0);
  columnLayout->setSpacing(0);
  canvasLayout->addWidget(m_column, 0, Qt::AlignHCenter);
  canvasLayout->addStretch();
  m_viewport = scroll->viewport();
  m_viewport->installEventFilter(this);

  auto *body = new QWidget(m_column);
  body->setObjectName("homeBody");
  columnLayout->addWidget(body);
  auto *content = new QVBoxLayout(body);
  content->setContentsMargins(40, 28, 40, 28);
  content->setSpacing(16);
  m_back = button(body, "homeBack");
  m_back->setProperty("quiet", true);
  m_back->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
  content->addWidget(m_back, 0, Qt::AlignLeft);
  m_eyebrow = label(body, "homeEyebrow", 10, true);
  content->addWidget(m_eyebrow);
  m_title = label(body, "homeTitle", 24, true);
  content->addWidget(m_title);
  m_intro = label(body, "homeIntro", 13);
  content->addWidget(m_intro);

  m_choices = new QWidget(body);
  m_choices->setObjectName("homeChoices");
  auto *choices = new QVBoxLayout(m_choices);
  choices->setContentsMargins(0, 4, 0, 4);
  choices->setSpacing(10);
  auto roleChoice = [&](const char *name, QLabel **title, QLabel **detail) {
    auto *b = button(m_choices, name);
    b->setProperty("roleChoice", true);
    b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    b->setMinimumHeight(84);
    auto *v = new QHBoxLayout(b);
    v->setContentsMargins(18, 14, 18, 14);
    v->setSpacing(16);
    auto *glyph = new DirectionGlyph(QString::fromLatin1(name) == "homeShare", b);
    glyph->setAttribute(Qt::WA_TransparentForMouseEvents);
    v->addWidget(glyph);
    auto *text = new QWidget(b);
    text->setAttribute(Qt::WA_TransparentForMouseEvents);
    auto *labels = new QVBoxLayout(text);
    labels->setContentsMargins(0, 0, 0, 0);
    labels->setSpacing(4);
    *title = label(b, "roleTitle", 14, true);
    (*title)->setAttribute(Qt::WA_TransparentForMouseEvents);
    labels->addWidget(*title);
    *detail = label(b, "roleDetail", 13);
    (*detail)->setAttribute(Qt::WA_TransparentForMouseEvents);
    labels->addWidget(*detail);
    v->addWidget(text, 1);
    auto *chevron = label(b, "roleChevron", 18, true);
    chevron->setText(QString(QChar(0x203A)));
    chevron->setAttribute(Qt::WA_TransparentForMouseEvents);
    v->addWidget(chevron, 0, Qt::AlignVCenter);
    choices->addWidget(b, 1);
    return b;
  };
  m_share = roleChoice("homeShare", &m_shareTitle, &m_shareDetail);
  m_receive = roleChoice("homeReceive", &m_receiveTitle, &m_receiveDetail);
  content->addWidget(m_choices);

  m_session = card(body, "homeSession");
  auto *session = new QGridLayout(m_session);
  session->setContentsMargins(24, 22, 24, 22);
  session->setSpacing(14);
  content->addWidget(m_session);
  m_sectionLink = label(m_session, "sectionLabel", 10, true);
  session->addWidget(m_sectionLink, 0, 0, 1, 2);
  m_relationship = new QWidget(m_session);
  m_relationship->setObjectName("homeRelationship");
  auto *devices = new QVBoxLayout(m_relationship);
  devices->setContentsMargins(0, 0, 0, 0);
  devices->setSpacing(0);
  auto device = [&](const char *name, QWidget **glyph, QLabel **title, QLabel **caption) {
    auto *w = new QWidget(m_relationship);
    w->setObjectName(name);
    auto *v = new QBoxLayout(QBoxLayout::LeftToRight, w);
    v->setContentsMargins(12, 9, 12, 9);
    v->setSpacing(12);
    *glyph = new ComputerGlyph(DeviceKind::Monitor, w);
    v->addWidget(*glyph, 0, Qt::AlignVCenter);
    auto *text = new QWidget(w);
    auto *labels = new QVBoxLayout(text);
    labels->setContentsMargins(0, 0, 0, 0);
    labels->setSpacing(2);
    *title = new DeviceNameLabel(text);
    (*title)->setObjectName(name);
    auto font = (*title)->font();
    font.setPointSize(15);
    font.setBold(true);
    (*title)->setFont(font);
    (*title)->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    labels->addWidget(*title);
    *caption = label(text, "deviceCaption", 11);
    (*caption)->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    labels->addWidget(*caption);
    v->addWidget(text, 1, Qt::AlignVCenter);
    devices->addWidget(w, 1);
  };
  device("homeLocalDevice", &m_localGlyph, &m_localName, &m_localCaption);
  m_arrow = label(m_relationship, "homeDirectionArrow", 24);
  m_arrow->setAlignment(Qt::AlignCenter);
  m_arrow->hide();
  auto *divider = new QFrame(m_relationship);
  divider->setObjectName("deviceDivider");
  divider->setFrameShape(QFrame::HLine);
  divider->setFixedHeight(1);
  devices->addWidget(divider);
  device("homePeerDevice", &m_peerGlyph, &m_peerName, &m_peerCaption);
  session->addWidget(m_relationship, 1, 0, 1, 2);

  auto *statusRow = new QWidget(m_session);
  statusRow->setObjectName("homeStatusRow");
  auto *statusLayout = new QHBoxLayout(statusRow);
  statusLayout->setContentsMargins(0, 0, 0, 0);
  statusLayout->setSpacing(8);
  m_statusDot = new StatusDot(statusRow);
  m_status = label(statusRow, "homeStatus", 14, true);
  m_status->setFixedHeight(m_status->fontMetrics().lineSpacing() * 2 + 2);
  m_linkMeta = label(statusRow, "homeLinkMeta", 11);
  m_linkMeta->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  m_linkMeta->setFixedHeight(m_status->height());
  statusRow->setFixedHeight(m_status->height());
  statusLayout->addWidget(m_statusDot, 0, Qt::AlignVCenter);
  statusLayout->addWidget(m_status, 0);
  statusLayout->addStretch();
  statusLayout->addWidget(m_linkMeta, 0);
  session->addWidget(statusRow, 2, 0, 1, 2);
  m_reason = label(m_session, "homeReason", 12);
  m_reason->setFixedHeight(m_reason->fontMetrics().lineSpacing() * 3 + 2);
  session->addWidget(m_reason, 3, 0, 1, 2);
  m_progress = new QProgressBar(m_session);
  m_progress->setObjectName("homeProgress");
  m_progress->setRange(0, 0);
  m_progress->setTextVisible(false);
  m_progress->setFixedHeight(3);
  session->addWidget(m_progress, 4, 0, 1, 2);
  m_recovery = label(m_session, "homeRecoverySummary", 11);
  session->addWidget(m_recovery, 5, 0, 1, 2);

  m_addressForm = new QWidget(body);
  m_addressForm->setObjectName("homeAddressForm");
  auto *form = new QVBoxLayout(m_addressForm);
  form->setContentsMargins(0, 0, 0, 0);
  form->setSpacing(8);
  m_addressLabel = label(m_addressForm, "homeAddressLabel");
  form->addWidget(m_addressLabel);
  m_address = new QLineEdit(m_addressForm);
  m_address->setObjectName("homeAddress");
  m_address->setClearButtonEnabled(true);
  m_address->setMaxLength(253);
  m_addressLabel->setBuddy(m_address);
  form->addWidget(m_address);
  m_error = label(m_addressForm, "homeAddressError");
  m_error->hide();
  form->addWidget(m_error);
  auto *addressActions = new QHBoxLayout();
  auto *addressSubmit = button(m_addressForm, "homeAddressSubmit");
  addressSubmit->setProperty("primaryAction", true);
  addressSubmit->setDefault(true);
  addressActions->addStretch();
  addressActions->addWidget(addressSubmit);
  form->addLayout(addressActions);
  content->addWidget(m_addressForm);

  m_actionCard = card(body, "homeActionCard");
  auto *actionCard = new QVBoxLayout(m_actionCard);
  actionCard->setContentsMargins(20, 16, 20, 16);
  actionCard->setSpacing(12);
  m_sectionControls = label(m_actionCard, "sectionLabel", 10, true);
  actionCard->addWidget(m_sectionControls);
  auto *directionRow = new QWidget(m_actionCard);
  directionRow->setObjectName("homeDirectionRow");
  auto *directionLayout = new QHBoxLayout(directionRow);
  directionLayout->setContentsMargins(0, 0, 0, 0);
  directionLayout->setSpacing(16);
  auto *directionText = new QWidget(directionRow);
  directionText->setObjectName("homeDirectionText");
  auto *directionTextLayout = new QVBoxLayout(directionText);
  directionTextLayout->setContentsMargins(0, 0, 0, 0);
  directionTextLayout->setSpacing(3);
  m_directionCaption = label(directionText, "homeDirectionCaption", 11, true);
  m_directionSummary = label(directionText, "homeDirectionSummary", 13, true);
  directionTextLayout->addWidget(m_directionCaption);
  directionTextLayout->addWidget(m_directionSummary);
  m_direction = button(directionRow, "homeChangeDirection");
  m_direction->setProperty("quiet", true);
  directionLayout->addWidget(directionText, 1, Qt::AlignVCenter);
  directionLayout->addWidget(m_direction, 0, Qt::AlignVCenter);
  auto *actionBody = new QWidget(m_actionCard);
  actionBody->setObjectName("homeActionBody");
  auto *actionBodyLayout = new QVBoxLayout(actionBody);
  actionBodyLayout->setContentsMargins(0, 0, 0, 0);
  actionBodyLayout->setSpacing(12);
  actionBodyLayout->addWidget(directionRow);
  auto *actionDivider = new QFrame(actionBody);
  actionDivider->setObjectName("actionDivider");
  actionDivider->setFrameShape(QFrame::HLine);
  actionDivider->setFixedHeight(1);
  actionBodyLayout->addWidget(actionDivider);
  auto *actionArea = new QWidget(actionBody);
  actionArea->setObjectName("homeActions");
  auto *actions = new FlowLayout(actionArea);
  m_primary = button(actionArea, "homePrimary");
  m_primary->setProperty("primaryAction", true);
  actions->addWidget(m_primary);
  m_find = button(actionArea, "homeFind");
  actions->addWidget(m_find);
  m_manual = button(actionArea, "homeManual");
  m_manual->setProperty("quiet", true);
  actions->addWidget(m_manual);
  m_layout = button(actionArea, "homeLayout");
  m_layout->setProperty("quiet", true);
  actions->addWidget(m_layout);
  actionBodyLayout->addWidget(actionArea);
  actionCard->addWidget(actionBody);
  content->addWidget(m_actionCard);

  auto *footer = new QWidget(m_column);
  footer->setObjectName("homeFooter");
  auto *footerRow = new QBoxLayout(QBoxLayout::LeftToRight, footer);
  footerRow->setContentsMargins(24, 10, 24, 10);
  footerRow->setSpacing(8);
  m_footerNavigation = new QWidget(footer);
  m_footerNavigation->setObjectName("homeNavigation");
  m_footerNavigation->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  auto *navigationRow = new FlowLayout(m_footerNavigation, 4);
  m_saved = button(m_footerNavigation, "homeSaved");
  m_options = button(m_footerNavigation, "homeOptions");
  m_diagnostics = button(m_footerNavigation, "homeDiagnostics");
  m_settings = button(m_footerNavigation, "homeSettings");
  for (auto *b : {m_saved, m_options, m_diagnostics, m_settings}) {
    b->setProperty("quiet", true);
    navigationRow->addWidget(b);
  }
  m_footerNote = label(footer, "homeFooterNote", 11);
  m_footerNote->setText(tr("Changes apply immediately."));
  footerRow->addWidget(m_footerNavigation, 1);

  footerRow->addWidget(m_footerNote);
  columnLayout->addWidget(footer);
  scroll->setWidget(m_canvas);
  hostLayout->addWidget(scroll, 1);
  root->addWidget(m_contentHost, 1);

  const QList<QWidget *> focusOrder = {
      m_back,
      m_share,
      m_receive,
      m_address,
      m_primary,
      m_find,
      m_manual,
      m_layout,
      m_direction,
      m_saved,
      m_options,
      m_diagnostics,
      m_settings,
      m_sidebarOverview,
      m_sidebarComputers,
      m_sidebarConnection,
      m_sidebarSettings,
      m_sidebarDiagnostics
  };
  for (int i = 1; i < focusOrder.size(); ++i)
    QWidget::setTabOrder(focusOrder[i - 1], focusOrder[i]);

  connect(m_share, &QPushButton::clicked, this, [this] {
    m_selectingRole = false;
    Q_EMIT roleChosen(Role::Share);
  });
  connect(m_receive, &QPushButton::clicked, this, [this] {
    m_selectingRole = false;
    Q_EMIT roleChosen(Role::Receive);
  });
  connect(m_back, &QPushButton::clicked, this, &ConnectionHome::back);
  connect(m_manual, &QPushButton::clicked, this, &ConnectionHome::showManualEntry);
  connect(m_find, &QPushButton::clicked, this, [this] {
    if (m_state.role == Role::Share)
      Q_EMIT addComputerRequested();
    else
      Q_EMIT findDeviceRequested();
  });
  connect(m_direction, &QPushButton::clicked, this, &ConnectionHome::showRoleSelection);
  connect(m_primary, &QPushButton::clicked, this, &ConnectionHome::activatePrimary);
  connect(m_address, &QLineEdit::returnPressed, this, &ConnectionHome::acceptAddress);
  connect(addressSubmit, &QPushButton::clicked, this, &ConnectionHome::acceptAddress);
  connect(m_address, &QLineEdit::textEdited, this, [this] {
    m_error->clear();
    m_error->hide();
  });
  connect(m_saved, &QPushButton::clicked, this, &ConnectionHome::savedDevicesRequested);
  connect(m_options, &QPushButton::clicked, this, &ConnectionHome::connectionOptionsRequested);
  connect(m_diagnostics, &QPushButton::clicked, this, &ConnectionHome::diagnosticsRequested);
  connect(m_settings, &QPushButton::clicked, this, &ConnectionHome::settingsRequested);
  connect(m_layout, &QPushButton::clicked, this, &ConnectionHome::layoutRequested);
  connect(m_sidebarOverview, &QPushButton::clicked, this, [this] {
    if (auto *scroll = findChild<QScrollArea *>("homeScroll"))
      scroll->verticalScrollBar()->setValue(0);
  });
  connect(m_sidebarComputers, &QPushButton::clicked, this, [this] {
    if (m_state.role == Role::Share)
      Q_EMIT addComputerRequested();
    else
      Q_EMIT savedDevicesRequested();
  });
  connect(m_sidebarConnection, &QPushButton::clicked, this, &ConnectionHome::connectionOptionsRequested);
  connect(m_sidebarSettings, &QPushButton::clicked, this, &ConnectionHome::settingsRequested);
  connect(m_sidebarDiagnostics, &QPushButton::clicked, this, &ConnectionHome::diagnosticsRequested);
  auto *escape = new QShortcut(QKeySequence(Qt::Key_Escape), this);
  escape->setContext(Qt::WidgetWithChildrenShortcut);
  connect(escape, &QShortcut::activated, this, [this] {
    if (m_editingAddress || (m_selectingRole && m_state.role != Role::None))
      back();
  });
  applyExperienceStyle(this);
  applyLocalStyle();
  render();
}

void ConnectionHome::applyLocalStyle()
{
  const bool dark = QApplication::palette().color(QPalette::Window).lightness() < 128;
  const QString surface = dark ? "#1b1c1f" : "#f5f6f8";
  const QString cardSurface = dark ? "#222328" : "#ffffff";
  const QString border = dark ? "#34363c" : "#e2e4e8";
  const QString ink = dark ? "#f5f5f7" : "#1b1d21";
  const QString muted = dark ? "#a8acb3" : "#6d717a";
  const QString faint = dark ? "#7e838c" : "#8b9099";
  const QString hover = dark ? "#2a2c31" : "#eef0f3";
  const QString selected = dark ? "#30343b" : "#e9edf3";
  const QString accent = dark ? "#8ab4ff" : "#2f6fed";
  const QString primary = dark ? "#f0f1f3" : "#1f2328";
  const QString onPrimary = dark ? "#17181b" : "#ffffff";
  const QString primaryHover = dark ? "#ffffff" : "#34383e";
  const QString input = dark ? "#1d1f23" : "#ffffff";
  const QString progress = dark ? "#2b2d32" : "#e8eaee";
  const QString danger = dark ? "#ffb4a9" : "#a33a32";
  const QString sessionBackground = cardSurface;
  setStyleSheet(
      QString(
          "QWidget#homeSidebar{background:%1;border-right:1px solid %2;}"
          "QWidget#homeContentHost,QScrollArea#homeScroll,QWidget#homeCanvas,QWidget#homeBody{background:transparent;"
          "border:0;}"
          "QWidget#sidebarBrandRow{background:transparent;}"
          "QLabel#sidebarBrand{color:%3;font-size:17px;font-weight:600;}"
          "QLabel#sidebarKicker{color:%4;font-size:11px;}"
          "QLabel#sidebarSection,QLabel#sectionLabel{color:%5;font-size:11px;font-weight:600;}"
          "QFrame#sidebarDeviceCard{background:transparent;border:0;border-bottom:1px solid %2;border-radius:0;}"
          "QFrame#homeSession{background:%16;border:1px solid %2;border-radius:10px;}"
          "QFrame#homeActionCard{background:%16;border:1px solid %2;border-radius:10px;}"
          "QFrame#deviceDivider{background:%2;border:0;}"
          "QFrame#actionDivider{background:%2;border:0;}"
          "QLabel#sidebarDevice{color:%3;}"
          "QLabel#sidebarState{color:%4;font-weight:600;}"
          "QLabel#sidebarPeer,QLabel#homeLinkMeta,QLabel#homeFooterNote,QLabel#sidebarFooter{color:%5;}"
          "QPushButton[navAction=true]{text-align:left;padding:8px 10px;border:1px solid "
          "transparent;border-radius:7px;background:transparent;color:%4;font-weight:500;}"
          "QPushButton[navAction=true]:hover{background:%7;color:%3;}"
          "QPushButton[navAction=true]:checked{background:%8;color:%3;border-color:%8;font-weight:600;}"
          "QPushButton[roleChoice=true]{background:%6;border:1px solid %2;border-radius:12px;text-align:left;}"
          "QPushButton[roleChoice=true]:hover{border-color:%9;background:%7;}"
          "QPushButton[primaryAction=true]{background:%10;color:%11;border:1px solid %10;border-radius:8px;"
          "padding:9px 16px;font-weight:600;}"
          "QPushButton[primaryAction=true]:hover{background:%12;border-color:%12;color:%11;}"
          "QPushButton[primaryAction=true]:disabled{background:%8;border-color:%2;color:%5;}"
          "QPushButton[quiet=true]{background:transparent;border:1px solid transparent;border-radius:7px;"
          "color:%4;padding:8px 10px;}"
          "QPushButton[quiet=true]:hover{background:%7;color:%3;}"
          "QPushButton[quiet=true]:focus{border-color:%9;}"
          "QLineEdit#homeAddress{background:%13;border:1px solid %2;border-radius:8px;padding:9px 12px;color:%3;"
          "selection-background-color:%9;}"
          "QLineEdit#homeAddress:focus{border:1px solid %9;}"
          "QLabel#homeTitle{color:%3;}"
          "QLabel#homeIntro{color:%4;}"
          "QLabel#homeEyebrow{color:%5;}"
          "QLabel#deviceCaption{color:%5;}"
          "QLabel#roleChevron{color:%5;}"
          "QLabel#homeDirectionArrow{color:%9;font-weight:600;}"
          "QWidget#homeFooter{background:transparent;border-top:1px solid %2;}"
          "QWidget#homeStatusRow,QWidget#homeDirectionRow,QWidget#homeDirectionText{background:transparent;}"
          "QLabel#homeStatus{color:%3;}"
          "QLabel#homeReason{color:%4;}"
          "QLabel#homeRecoverySummary{color:%5;}"
          "QLabel#homeDirectionCaption{color:%5;font-size:11px;font-weight:600;}"
          "QLabel#homeDirectionSummary{color:%3;font-weight:600;}"
          "QLabel#homeAddressError{color:%14;}"
          "QProgressBar#homeProgress{border:0;background:%15;border-radius:1px;}"
          "QProgressBar#homeProgress::chunk{background:%9;border-radius:1px;}"
      )
          .arg(surface, border, ink, muted, faint, cardSurface, hover, selected, accent, primary)
          .arg(onPrimary, primaryHover, input, danger, progress)
          .arg(sessionBackground)
  );
  if (auto *brandMark = findChild<QLabel *>("sidebarBrandMark"))
    brandMark->setPixmap(themedIcon("apps/64/org.deskflow.deskflow-symbolic.svg", dark).pixmap(24, 24));
  m_sidebarOverview->setIcon(themedIcon("apps/64/org.deskflow.deskflow-symbolic.svg", dark));
  m_sidebarSettings->setIcon(themedIcon("actions/16/configure.svg", dark));
  m_sidebarDiagnostics->setIcon(themedIcon("actions/16/help-about.svg", dark));
  for (auto *b : {m_sidebarOverview, m_sidebarComputers, m_sidebarConnection, m_sidebarSettings, m_sidebarDiagnostics})
    b->setIconSize(QSize(16, 16));
}

void ConnectionHome::setState(const State &state)
{
  m_actionPending = false;
  const bool newPage = state.role != m_state.role;
  m_state = state;
  render();
  if (newPage)
    QTimer::singleShot(0, this, [this] { findChild<QScrollArea *>("homeScroll")->verticalScrollBar()->setValue(0); });
}

void ConnectionHome::setDesktopWidget(QWidget *widget)
{
  if (m_desktop || !widget)
    return;
  m_desktop = widget;
  // Keep the complete page inside one scroll area. Giving the workspace its
  // own stretch outside it compressed the overview until text was clipped.
  widget->setParent(m_canvas);
  auto *canvas = qobject_cast<QVBoxLayout *>(m_canvas->layout());
  canvas->insertWidget(1, widget, 0, Qt::AlignHCenter);
  widget->setMinimumHeight(360);
  widget->hide();
  render();
}

void ConnectionHome::showManualEntry()
{
  if (m_state.role == Role::None) {
    showRoleSelection();
    m_receive->setFocus(Qt::OtherFocusReason);
    return;
  }
  if (m_state.role != Role::Receive)
    return;
  m_editingAddress = true;
  m_selectingRole = false;
  m_address->clear();
  m_error->clear();
  m_error->hide();
  render();
  m_address->setFocus(Qt::OtherFocusReason);
}

void ConnectionHome::showRoleSelection()
{
  m_selectingRole = true;
  m_editingAddress = false;
  render();
  m_share->setFocus(Qt::OtherFocusReason);
}

void ConnectionHome::back()
{
  m_editingAddress = false;
  m_selectingRole = false;
  m_address->clear();
  m_error->clear();
  render();
  if (m_state.role == Role::Receive)
    m_manual->setFocus(Qt::TabFocusReason);
  else
    m_direction->setFocus(Qt::TabFocusReason);
}

void ConnectionHome::acceptAddress()
{
  if (!m_editingAddress)
    return;
  const auto address = m_address->text().trimmed();
  if (address.isEmpty() || address.contains(QRegularExpression("[\\s/\\\\@?#]")) || address.contains("://")) {
    m_error->setText(tr("Enter a computer name or IP address, without a web address or spaces."));
    m_error->show();
    m_address->setFocus();
    return;
  }
  m_editingAddress = false;
  m_address->clear();
  m_error->clear();
  Q_EMIT addressChosen(address);
  render();
}

void ConnectionHome::activatePrimary()
{
  if (m_editingAddress) {
    acceptAddress();
    return;
  }
  if (m_state.role == Role::None || m_selectingRole || m_actionPending)
    return;
  switch (m_state.stage) {
  case Stage::Starting:
  case Stage::Connecting:
  case Stage::Verifying:
  case Stage::Connected:
  case Stage::Listening:
  case Stage::Retrying:
  case Stage::Discovering:
    m_actionPending = true;
    render();
    Q_EMIT startStopRequested();
    return;
  case Stage::Stopping:
    return;
  case Stage::Failed:
    if (m_state.canRetry) {
      m_actionPending = true;
      render();
      Q_EMIT retryRequested();
    } else
      Q_EMIT diagnosticsRequested();
    return;
  case Stage::NeedsAction:
    Q_EMIT diagnosticsRequested();
    return;
  default:
    break;
  }
  if (!m_state.identityReady) {
    Q_EMIT identitySetupRequested();
    return;
  }
  if (m_state.role == Role::Receive && !m_state.hasAddress) {
    if (m_state.hasSavedDevices)
      Q_EMIT savedDevicesRequested();
    else
      Q_EMIT findDeviceRequested();
    return;
  }
  if (!m_state.canStart) {
    Q_EMIT layoutRequested();
    return;
  }
  m_actionPending = true;
  render();
  Q_EMIT startStopRequested();
}

void ConnectionHome::render()
{
  const bool choosing = m_state.role == Role::None || m_selectingRole;
  const bool desktop = m_desktop && !choosing && !m_editingAddress;
  const bool sharing = m_state.role == Role::Share;
  const bool live =
      m_state.stage == Stage::Starting || m_state.stage == Stage::Connecting || m_state.stage == Stage::Verifying ||
      m_state.stage == Stage::Connected || m_state.stage == Stage::Listening || m_state.stage == Stage::Retrying ||
      m_state.stage == Stage::Recovering || m_state.stage == Stage::Discovering || m_state.stage == Stage::Stopping;
  const bool busy = m_state.stage == Stage::Starting || m_state.stage == Stage::Connecting ||
                    m_state.stage == Stage::Verifying || m_state.stage == Stage::Retrying ||
                    m_state.stage == Stage::Recovering || m_state.stage == Stage::Discovering ||
                    m_state.stage == Stage::Stopping;
  const bool dark = QApplication::palette().color(QPalette::Window).lightness() < 128;
  const bool compact = height() < 600;
  const bool wide = width() >= 800;
  const bool compactSession = compact && !choosing && !m_editingAddress;

  m_sidebar->setVisible(wide);
  auto *bodyLayout = qobject_cast<QVBoxLayout *>(findChild<QWidget *>("homeBody")->layout());
  bodyLayout->setContentsMargins(
      desktop   ? 24
      : compact ? 16
                : 40,
      desktop   ? (compact ? 10 : 16)
      : compact ? 14
                : 28,
      desktop   ? 24
      : compact ? 16
                : 40,
      desktop   ? (compact ? 8 : 16)
      : compact ? 8
                : 28
  );
  bodyLayout->setSpacing(desktop ? 10 : compact ? 10 : 16);
  auto *actionLayout = qobject_cast<QVBoxLayout *>(m_actionCard->layout());
  actionLayout->setContentsMargins(compact ? 16 : 20, compact ? 14 : 16, compact ? 16 : 20, compact ? 14 : 16);
  actionLayout->setSpacing(compact ? 10 : 12);
  auto titleFont = m_title->font();
  titleFont.setPointSize(desktop ? 20 : compact ? 21 : 26);
  m_title->setFont(titleFont);
  for (auto *glyph : findChildren<QWidget *>("roleDirectionGlyph"))
    glyph->setFixedHeight(52);
  for (auto *glyph : findChildren<QWidget *>("computerGlyph"))
    glyph->setFixedSize(58, 44);
  for (auto *name : {m_localName, m_peerName}) {
    auto font = name->font();
    font.setPointSize(compactSession ? 13 : 14);
    name->setFont(font);
  }
  m_localGlyph->setVisible(true);
  m_peerGlyph->setVisible(true);
  static_cast<ComputerGlyph *>(m_localGlyph)->setKind(sharing ? DeviceKind::KeyboardMouse : DeviceKind::Monitor);
  static_cast<ComputerGlyph *>(m_peerGlyph)->setKind(sharing ? DeviceKind::Monitor : DeviceKind::KeyboardMouse);
  auto *session = qobject_cast<QGridLayout *>(m_session->layout());
  session->setContentsMargins(
      compactSession ? 14 : 24, compactSession ? 10 : 22, compactSession ? 14 : 24, compactSession ? 10 : 22
  );
  session->addWidget(m_sectionLink, 0, 0, 1, 2);
  session->addWidget(m_relationship, compactSession ? 0 : 1, 0, 1, 2);
  session->addWidget(findChild<QWidget *>("homeStatusRow"), compactSession ? 1 : 2, 0, 1, 2);
  session->addWidget(m_reason, compactSession ? 2 : 3, 0, 1, 2);
  session->addWidget(m_progress, compactSession ? 3 : 4, 0, 1, 2);
  session->addWidget(m_recovery, compactSession ? 4 : 5, 0, 1, 2);
  session->setColumnStretch(0, 1);
  session->setColumnStretch(1, compactSession ? 0 : 1);
  m_sectionLink->setVisible(!compactSession);
  m_reason->setVisible(!compactSession);
  m_recovery->setVisible(!compactSession && !m_state.recoverySummary.isEmpty());
  m_status->setMaximumWidth(compactSession ? qMax(150, m_session->width() / 3) : QWIDGETSIZE_MAX);
  for (auto *device : {m_localGlyph->parentWidget(), m_peerGlyph->parentWidget()}) {
    auto *box = qobject_cast<QBoxLayout *>(device->layout());
    box->setSpacing(12);
    box->setDirection(QBoxLayout::LeftToRight);
  }

  if (desktop != m_desktopShown) {
    m_desktopShown = desktop;
    m_desktop->setVisible(desktop);
  }
  const int pageWidth = qMin(880, qMax(1, m_viewport->width()));
  m_column->setFixedWidth(pageWidth);
  if (m_desktop)
    m_desktop->setFixedWidth(pageWidth);

  m_back->setText(tr("Back"));
  m_back->setVisible(m_editingAddress || (m_selectingRole && m_state.role != Role::None));
  m_eyebrow->setVisible(false);
  m_title->setText(
      choosing           ? tr("Connect two computers")
      : m_editingAddress ? tr("Add a computer by address")
                         : tr("Overview")
  );
  m_intro->setText(
      choosing           ? tr("Choose which computer supplies the keyboard and mouse.")
      : m_editingAddress ? tr("Enter the address shown on the other computer.")
                         : tr("See where input is going and manage the current connection.")
  );
  m_intro->setVisible((choosing || m_editingAddress || !desktop) && !compact);
  m_sectionLink->setText(tr("Connection"));
  m_sectionControls->setText(tr("Connection controls"));
  m_shareTitle->setText(tr("Use this computer\u2019s keyboard and mouse"));
  m_shareDetail->setText(tr("Control the other computer from here."));
  m_receiveTitle->setText(tr("Use the other computer\u2019s keyboard and mouse"));
  m_receiveDetail->setText(tr("Control this computer from there."));
  m_share->setAccessibleName(m_shareTitle->text() + ". " + m_shareDetail->text());
  m_receive->setAccessibleName(m_receiveTitle->text() + ". " + m_receiveDetail->text());
  m_choices->setVisible(choosing);
  m_session->setVisible(!choosing && !m_editingAddress);
  m_addressForm->setVisible(m_editingAddress);
  m_actionCard->setVisible(!choosing && !m_editingAddress);
  const auto peer = m_state.peerName.isEmpty()
                        ? (sharing ? tr("Receiving computer") : tr("Choose a controlling computer"))
                        : m_state.peerName;
  const auto local = m_state.localName.isEmpty() ? tr("This computer") : m_state.localName;
  m_localName->setText(local);
  m_peerName->setText(peer);
  m_localName->setToolTip(local);
  m_peerName->setToolTip(peer);
  m_localCaption->setText(sharing ? tr("Keyboard and mouse") : tr("Receives control"));
  m_peerCaption->setText(sharing ? tr("Receives control") : tr("Keyboard and mouse"));
  m_arrow->setText(sharing ? QString(QChar(0x2192)) : QString(QChar(0x2190)));
  m_arrow->setAccessibleName(
      sharing ? tr("Controls the other computer") : tr("Receives control from the other computer")
  );
  m_addressLabel->setText(tr("Computer name or IP address"));
  m_address->setPlaceholderText(tr("For example, work-pc.local"));
  findChild<QPushButton *>("homeAddressSubmit")->setText(tr("Use this address"));

  QString status, reason, action, tone = "idle";
  if (choosing) {
  } else if (m_editingAddress)
    action = tr("Use this address");
  else
    switch (m_state.stage) {
    case Stage::Starting:
      status = sharing ? tr("Starting sharing") : tr("Starting connection");
      reason = tr("Preparing the input process. You can cancel this attempt.");
      action = tr("Cancel");
      tone = "busy";
      break;
    case Stage::Connecting:
      status = tr("Connecting to %1").arg(peer);
      reason = tr("Waiting for the controlling computer to respond. No input is shared yet.");
      action = tr("Cancel");
      tone = "busy";
      break;
    case Stage::Verifying:
      status = tr("Checking the computer\u2019s identity");
      reason =
          tr("Sharing starts after the saved identity is verified. A first connection needs your fingerprint review.");
      action = tr("Cancel");
      tone = "busy";
      break;
    case Stage::Connected:
      status = tr("Connected");
      reason = (sharing ? tr("This computer controls %1.") : tr("%1 controls this computer.")).arg(peer);
      action = tr("Disconnect");
      tone = "connected";
      break;
    case Stage::Listening:
      status = tr("Waiting for a receiving computer");
      reason =
          tr("On the receiving computer, choose \u201cUse the other computer\u2019s keyboard and mouse\u201d, then "
             "select this computer.");
      action = tr("Stop sharing");
      tone = "waiting";
      break;
    case Stage::Discovering:
      status = tr("Finding the saved computer");
      reason = tr("Searching the selected network. Only the saved identity can resume this connection.");
      action = tr("Pause");
      tone = "busy";
      break;
    case Stage::Retrying:
    case Stage::Recovering:
      status = tr("Reconnecting and checking identity");
      reason =
          m_state.retrySeconds >= 0
              ? tr("The previous attempt did not complete. The next retry is in %1 s; identity is checked again before "
                   "input starts.")
                    .arg(m_state.retrySeconds)
              : tr("Retrying now. The previous attempt did not complete; identity is checked again before input starts."
                );
      action = tr("Pause");
      tone = "busy";
      break;
    case Stage::Stopping:
      status = sharing ? tr("Stopping sharing") : tr("Disconnecting");
      reason = tr("Waiting for the input process to stop.");
      action = tr("Stopping...");
      break;
    case Stage::Failed:
      status = sharing ? tr("Could not start sharing") : tr("Could not connect");
      reason = m_state.explanation;
      action = m_state.canRetry ? tr("Try again") : tr("Review connection");
      tone = "error";
      break;
    case Stage::NeedsAction:
      status = tr("Connection needs attention");
      reason = m_state.explanation;
      action = tr("Review connection");
      tone = "error";
      break;
    default:
      status = m_state.stage == Stage::Paused ? tr("Paused") : tr("Not connected");
      if (!m_state.identityReady) {
        reason = tr("Prepare this computer\u2019s identity before the first connection.");
        action = tr("Prepare");
      } else if (!sharing && !m_state.hasAddress) {
        reason = tr("Choose the computer whose keyboard and mouse you want to use.");
        action = m_state.hasSavedDevices ? tr("Saved computers") : tr("Find computer");
      } else if (!m_state.canStart) {
        reason = tr("Arrange the computers before you start sharing.");
        action = tr("Arrange");
      } else if (sharing) {
        reason =
            tr("Start sharing here, then connect from the receiving computer. Screen positions are arranged here.");
        action = tr("Start sharing");
      } else {
        reason = m_state.peerKnown
                     ? tr("Reconnect to %1 with its saved identity. Automatic attempts stay paused until you connect.")
                           .arg(peer)
                     : tr("Connect to compare the other computer\u2019s identity once. No serial number is needed.");
        action = m_state.peerKnown ? tr("Reconnect") : tr("Pair and connect");
      }
      break;
    }
  if (compactSession && sharing && m_state.identityReady && m_state.canStart) {
    if (m_state.stage == Stage::Idle || m_state.stage == Stage::Paused)
      reason = tr("Start sharing, then connect from the receiving computer.");
    else if (m_state.stage == Stage::Listening)
      reason = tr("Choose this computer on the receiving computer.");
  }
  m_status->setText(status);
  m_status->setFixedHeight(m_status->fontMetrics().lineSpacing() * 2 + 2);
  m_linkMeta->setFixedHeight(m_status->height());
  findChild<QWidget *>("homeStatusRow")->setFixedHeight(m_status->height());
  m_reason->setText(reason);
  m_reason->setFixedHeight(m_reason->fontMetrics().lineSpacing() * 3 + 2);
  m_status->setAccessibleName(status);
  m_status->setMinimumWidth(
      compactSession ? qMin(m_status->fontMetrics().horizontalAdvance(status) + 2, m_status->maximumWidth()) : 0
  );
  m_progress->setVisible(busy && !choosing && !m_editingAddress);
  m_recovery->setText(m_state.recoverySummary);
  m_linkMeta->setText(m_state.identityReady ? tr("Identity ready") : tr("Identity setup required"));
  m_linkMeta->setVisible(!compactSession);
  m_directionCaption->setText(tr("Keyboard and mouse control"));
  m_directionSummary->setText(
      sharing ? tr("This computer controls the other computer") : tr("The other computer controls this computer")
  );
  m_directionSummary->setAccessibleName(
      sharing ? tr("This computer supplies the keyboard and mouse to the other computer")
              : tr("This computer receives keyboard and mouse input from the other computer")
  );
  const auto accent = tone == "error"                       ? (dark ? "#ffb4a9" : "#9b2c24")
                      : tone == "connected"                 ? (dark ? "#8bd5a5" : "#176c3b")
                      : tone == "busy" || tone == "waiting" ? (dark ? "#85baff" : "#005ebc")
                                                            : (dark ? "#bfc4cd" : "#62656d");
  const auto statusStyle = QString("color:%1;").arg(accent);
  if (m_status->styleSheet() != statusStyle)
    m_status->setStyleSheet(statusStyle);
  m_statusDot->setProperty("tone", tone);
  m_statusDot->update();
  m_sidebarDot->setProperty("tone", tone);
  m_sidebarDot->update();
  m_session->setProperty("connectionTone", tone);
  m_session->style()->unpolish(m_session);
  m_session->style()->polish(m_session);
  m_primary->setText(action);
  m_primary->setVisible(!choosing);
  m_primary->setEnabled(!m_actionPending && m_state.stage != Stage::Stopping);
  m_find->setText(sharing ? tr("Add computer") : tr("Choose computer"));
  m_find->setAccessibleName(sharing ? tr("Add a receiving computer") : tr("Choose another computer"));
  m_find->setVisible(!choosing && !m_editingAddress && (sharing || (!live && m_state.hasAddress)));
  m_manual->setText(m_state.hasAddress ? tr("Change address") : tr("Enter address"));
  m_manual->setAccessibleName(m_manual->text());
  m_manual->setVisible(!choosing && !m_editingAddress && !live && !sharing);
  m_layout->setText(tr("Arrange"));
  m_layout->setAccessibleName(tr("Arrange computers"));
  m_layout->setVisible(!desktop && !choosing && !m_editingAddress && sharing);
  m_direction->setText(sharing ? tr("Let the other computer control") : tr("Let this computer control"));
  m_direction->setAccessibleName(
      sharing ? tr("Change control to the other computer") : tr("Change control to this computer")
  );
  m_direction->setVisible(!choosing && !m_editingAddress && !live);
  m_directionSummary->setVisible(!choosing && !m_editingAddress);
  m_saved->setText(tr("Saved computers"));
  m_saved->setVisible(!choosing && !m_editingAddress && !sharing);
  m_saved->setEnabled(true);
  m_options->setText(tr("Connection settings"));
  m_options->setVisible(!choosing && !m_editingAddress && !live);
  m_diagnostics->setText(tr("Diagnostics"));
  m_settings->setText(tr("Settings"));
  m_sidebarDevice->setText(local);
  m_sidebarState->setText(status);
  m_sidebarPeer->setText(peer);
  m_sidebarOverview->setChecked(!choosing && !m_editingAddress);
  m_sidebarComputers->setText(sharing ? tr("Add computer") : tr("Computers"));
  m_sidebarComputers->setIcon(themedIcon(
      sharing ? QStringLiteral("actions/16/computer-add.svg") : QStringLiteral("actions/22/laptop-duotone.svg"), dark
  ));
  m_sidebarConnection->setIcon(themedIcon(QStringLiteral("actions/16/network-connect.svg"), dark));
  m_sidebarConnection->setEnabled(!live);
  m_sidebarDiagnostics->setEnabled(true);
  m_sidebarSettings->setEnabled(true);

  auto *footer = findChild<QWidget *>("homeFooter");
  auto *footerLayout = qobject_cast<QBoxLayout *>(footer->layout());
  footerLayout->setContentsMargins(wide ? 16 : 24, wide ? 8 : 10, wide ? 16 : 24, wide ? 8 : 10);
  const bool showNavigation = !choosing && !m_editingAddress && (!live || !sharing);
  m_footerNavigation->setVisible(!wide && showNavigation);
  m_footerNote->setVisible(false);
  footer->setVisible(!wide && !choosing && !m_editingAddress && (!desktop || showNavigation));
  footerLayout->setDirection(QBoxLayout::LeftToRight);
  footerLayout->setAlignment(m_footerNavigation, {});
  footerLayout->setAlignment(m_footerNote, Qt::AlignRight);
  for (auto *control :
       {m_back, m_primary, m_find, m_manual, m_layout, m_direction, m_saved, m_options, m_diagnostics, m_settings,
        m_sidebarOverview, m_sidebarComputers, m_sidebarConnection, m_sidebarSettings, m_sidebarDiagnostics}) {
    control->ensurePolished();
    control->setMinimumSize(0, 32);
    control->setMinimumSize(control->sizeHint().expandedTo(QSize(0, 32)));
  }
  for (auto *part : {static_cast<QWidget *>(m_title), static_cast<QWidget *>(m_intro), m_addressForm, m_actionCard}) {
    if (m_editingAddress)
      part->setFixedWidth(qMin(560, qMax(240, m_column->width() - 64)));
    else {
      part->setMinimumWidth(0);
      part->setMaximumWidth(QWIDGETSIZE_MAX);
    }
  }
  auto *directionRow = findChild<QWidget *>("homeDirectionRow");
  auto *directionText = findChild<QWidget *>("homeDirectionText");
  auto *directionLayout = qobject_cast<QBoxLayout *>(directionRow->layout());
  const auto bodyMargins = bodyLayout->contentsMargins();
  const auto cardMargins = actionLayout->contentsMargins();
  const int available =
      qMax(0, m_column->width() - bodyMargins.left() - bodyMargins.right() - cardMargins.left() - cardMargins.right());
  const int directionWidth =
      directionText->sizeHint().width() + m_direction->sizeHint().width() + directionLayout->spacing();
  const bool stackDirection = directionWidth > available;
  directionLayout->setDirection(stackDirection ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
  directionLayout->setAlignment(m_direction, stackDirection ? Qt::AlignLeft : Qt::AlignVCenter);
  // A word-wrapped label's minimum changes after the page width is known.
  // Preserve each card's complete content rather than clipping it to fit.
  m_session->setMinimumHeight(m_session->minimumSizeHint().height());
  m_actionCard->setMinimumHeight(m_actionCard->minimumSizeHint().height());
}

void ConnectionHome::changeEvent(QEvent *event)
{
  QWidget::changeEvent(event);
  if (event->type() == QEvent::LanguageChange || event->type() == QEvent::PaletteChange ||
      event->type() == QEvent::ApplicationPaletteChange || event->type() == QEvent::FontChange ||
      event->type() == QEvent::ApplicationFontChange) {
    applyLocalStyle();
    render();
  }
}

void ConnectionHome::resizeEvent(QResizeEvent *event)
{
  QWidget::resizeEvent(event);
  render();
}

bool ConnectionHome::eventFilter(QObject *watched, QEvent *event)
{
  if (watched == m_viewport && event->type() == QEvent::Resize) {
    m_column->setFixedWidth(qMin(880, qMax(1, m_viewport->width())));
    render();
  }
  return QWidget::eventFilter(watched, event);
}
} // namespace deskflow::gui
