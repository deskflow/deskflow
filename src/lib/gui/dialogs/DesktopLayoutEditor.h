// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "experience/LayoutModel.h"
#include <QDialog>
#include <QLineF>
#include <QMap>
#include <QTransform>

class QLabel;
class QLineEdit;
class QPushButton;
class QDoubleSpinBox;
class QComboBox;
class QDialogButtonBox;

class LayoutCanvas : public QWidget
{
  Q_OBJECT
public:
  struct DisplayPresentation
  {
    QString deviceId, deviceName, number, detail;
    bool cached = false;
  };
  explicit LayoutCanvas(deskflow::experience::LayoutModel &model, QWidget *parent = nullptr);
  QRectF desktopRect(const std::string &name) const;
  void setSelected(int index);
  void setMonitorMode(bool value)
  {
    m_monitorMode = value;
    update();
  }
  void setMonitorEdges(QVector<QLineF> edges)
  {
    m_monitorEdges = std::move(edges);
    update();
  }
  void setDisplayPresentation(QMap<QString, DisplayPresentation> presentation);
  void setUnknownDevices(QMap<QString, QString> devices);
  QRect unknownDeviceRect(const QString &device) const;
  void setDragGroups(bool value)
  {
    m_dragGroups = value;
    update();
  }
  void setEditable(bool value)
  {
    m_editable = value;
    if (!value)
      m_dragging = false;
    update();
  }
  QSize sizeHint() const override
  {
    return {700, 330};
  }
Q_SIGNALS:
  void selected(int index);
  void moved(const QString &name, double x, double y, double threshold);
  void unknownDeviceSelected(const QString &device);

protected:
  void paintEvent(QPaintEvent *) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;

private:
  QTransform transform() const;
  int unknownAreaHeight() const;
  deskflow::experience::LayoutModel &m_model;
  int m_selected = 0;
  QVector<QLineF> m_monitorEdges;
  QMap<QString, DisplayPresentation> m_presentation;
  QMap<QString, QString> m_unknownDevices;
  bool m_dragGroups = false, m_editable = true;
  bool m_dragging = false;
  bool m_monitorMode = false;
  QPointF m_press, m_dragDelta;
  QTransform m_dragTransform;
};

// A draft-only dialog. The caller applies its result to ServerConfig only after
// Accepted; Settings and the live core are intentionally not owned here.
class DesktopLayoutEditor : public QDialog
{
  Q_OBJECT
public:
  explicit DesktopLayoutEditor(std::vector<deskflow::experience::Desktop> desktops, QWidget *parent = nullptr);
  const std::vector<deskflow::experience::Desktop> &desktops() const
  {
    return m_model.desktops();
  }
  void setDesktops(std::vector<deskflow::experience::Desktop> desktops);
  void embedInSettings();
  void setMonitorMode(bool value);
  void setMonitorDetails(QMap<QString, QString> details);
  void setRoutingError(const QString &error);
  void setMonitorEdges(QVector<QLineF> edges)
  {
    m_canvas->setMonitorEdges(std::move(edges));
  }
  void setEditingEnabled(bool enabled);
  bool editingEnabled() const
  {
    return m_editable;
  }
  void addSecondaryAction(QPushButton *button);
  int heightForWidth(int width) const override;
  void moveDesktop(const QString &name, double x, double y, bool snap = true, double threshold = 16);
  void resizeDesktop(const QString &name, double width, double height);
  void undo();
  void accept() override;
  void reject() override;
Q_SIGNALS:
  void draftChanged();
  void saveRequested();
  void cancelRequested();

protected:
  void changeEvent(QEvent *event) override;

private:
  void refresh();
  void updateGeometryFields();
  deskflow::experience::LayoutModel m_model;
  LayoutCanvas *m_canvas;
  QComboBox *m_computer;
  QLineEdit *m_displayName = nullptr;
  QLineEdit *m_protocolName = nullptr;
  QLabel *m_validation;
  QLabel *m_monitorDetails = nullptr;
  QMap<QString, QString> m_monitorDetailText;
  QPushButton *m_undo;
  QPushButton *m_save;
  QDoubleSpinBox *m_x = nullptr, *m_y = nullptr, *m_width = nullptr, *m_height = nullptr;
  QDialogButtonBox *m_buttons = nullptr;
  bool m_monitorMode = false;
  bool m_embedded = false;
  bool m_editable = true;
};
