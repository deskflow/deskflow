// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include <QWidget>
namespace deskflow::gui {
inline void applyExperienceStyle(QWidget *widget)
{
  auto font = widget->font();
  font.setPointSize(12);
  widget->setFont(font);
  const bool dark = widget->palette().color(QPalette::Window).lightness() < 128;
  const QString surface = dark ? "#2c2d31" : "#ffffff", ink = dark ? "#f5f5f7" : "#1d1d1f";
  const QString border = dark ? "#45464b" : "#d9dadd", hover = dark ? "#383a40" : "#edeef0";
  widget->setStyleSheet(
      QString(
          "QWidget#deviceActions{background:%2;border-bottom:1px solid %1;}"
          "QWidget#topLevelWidget QGroupBox#groupBox{border:1px solid "
          "%1;border-radius:10px;background:%2;margin-top:0px;padding:0px;}"
          "QMenuBar{background:%2;color:%3;padding:2px 8px;border:0;}"
          "QMenuBar::item{background:transparent;color:%3;border:0;padding:5px 8px;}"
          "QMenuBar::item:selected,QMenuBar::item:pressed{background:%4;color:%3;border-radius:4px;}"
          "QMenu{background:%2;color:%3;border:1px solid %1;padding:4px;}"
          "QMenu::item{padding:6px 28px 6px 20px;background:transparent;color:%3;}"
          "QMenu::item:selected{background:%4;color:%3;}"
          "QMenu::item:disabled{color:%5;}"
          "QMenu::separator{height:1px;background:%1;margin:5px 8px;}"
          "QToolButton#settingsMoreActions{padding:6px 12px;border:1px solid "
          "transparent;border-radius:5px;color:%3;background:transparent;}"
          "QToolButton#settingsMoreActions:hover{background:%4;}"
          "QPushButton{min-height:20px;padding:6px 12px;border:2px solid %1;border-radius:6px;background:%2;color:%3;}"
          "QPushButton:hover{background:%4;border-color:#6c94cf;}"
          "QPushButton:focus,QToolButton:focus,QLineEdit:focus,QComboBox:focus{border:2px solid #5687d5;}"
          "QPushButton:pressed{background:%4;}"
          "QPushButton:disabled{color:%5;background:%6;border-color:%1;}"
          "QPushButton[primaryAction=true]{background:%8;color:%9;border-color:%8;font-weight:600;}"
          "QPushButton[primaryAction=true]:hover{background:%10;border-color:%10;}"
          "QPushButton[primaryAction=true]:disabled{background:%6;color:%5;border-color:%1;}"
          "QLineEdit,QComboBox,QSpinBox,QDoubleSpinBox{min-height:22px;padding:5px 8px;border:2px solid "
          "%1;border-radius:5px;color:%3;background:%2;}"
          "QComboBox{padding-right:28px;}"
          "QComboBox::drop-down{width:26px;border:none;subcontrol-origin:border;subcontrol-position:top right;}"
          "QComboBox::down-arrow{image:url(:/icons/deskflow-%7/actions/16/go-down.svg);width:12px;height:12px;}"
          "QComboBox:disabled,QLineEdit:disabled,QSpinBox:disabled,QDoubleSpinBox:disabled{color:%5;background:%6;}"
          "QComboBox QAbstractItemView::item{min-height:26px;padding:5px 8px;}"
          "QComboBox QAbstractItemView::item:selected,QComboBox QAbstractItemView::item:hover{background:%4;color:%3;}"
          "QComboBox QAbstractItemView{border:1px solid "
          "%1;background:%2;color:%3;selection-background-color:%4;selection-color:%3;padding:4px;}"
          "QSpinBox,QDoubleSpinBox{padding-right:24px;}"
          "QSpinBox::up-button,QSpinBox::down-button,QDoubleSpinBox::up-button,QDoubleSpinBox::down-button{width:22px;"
          "border:none;subcontrol-origin:border;}"
          "QSpinBox::up-button,QDoubleSpinBox::up-button{subcontrol-position:top right;}"
          "QSpinBox::down-button,QDoubleSpinBox::down-button{subcontrol-position:bottom right;}"
          "QSpinBox::up-arrow,QDoubleSpinBox::up-arrow{image:url(:/icons/deskflow-%7/actions/16/"
          "go-up.svg);width:10px;height:10px;}"
          "QSpinBox::down-arrow,QDoubleSpinBox::down-arrow{image:url(:/icons/deskflow-%7/actions/16/"
          "go-down.svg);width:10px;height:10px;}"
          "QToolButton#recentServersButton{padding:6px 28px 6px 12px;border:2px solid "
          "%1;border-radius:6px;background:%2;color:%3;}"
          "QToolButton#recentServersButton:hover{background:%4;}"
          "QToolButton#recentServersButton:disabled{background:%6;color:%5;}"
          "QToolButton#recentServersButton::menu-button{width:24px;border:none;subcontrol-origin:border;subcontrol-"
          "position:right;}"
          "QToolButton#recentServersButton::menu-arrow{image:url(:/icons/deskflow-%7/actions/16/"
          "go-down.svg);width:12px;height:12px;}"
          "QToolButton#connectionOptionsToggle,QToolButton#layoutAdvancedToggle{padding:5px 8px;border:1px solid "
          "transparent;border-radius:5px;color:%3;}"
          "QToolButton#connectionOptionsToggle:hover,QToolButton#layoutAdvancedToggle:hover{background:%4;}"
          "QListWidget#nearbyDevices,QListWidget#trustedDeviceList{border:1px solid "
          "%1;border-radius:8px;padding:6px;background:%2;}"
          "QListWidget#nearbyDevices::item,QListWidget#trustedDeviceList::item{padding:12px;border-bottom:1px solid "
          "%1;}"
          "QListWidget#nearbyDevices::item:selected,QListWidget#trustedDeviceList::item:selected{background:%4;color:%"
          "3;}"
          "QRadioButton,QCheckBox{spacing:7px;}"
          "QScrollBar:vertical{background:transparent;width:12px;margin:2px;}"
          "QScrollBar:horizontal{background:transparent;height:12px;margin:2px;}"
          "QScrollBar::handle{background:%1;border-radius:4px;}"
          "QScrollBar::handle:vertical{min-height:28px;}"
          "QScrollBar::handle:horizontal{min-width:28px;}"
          "QScrollBar::handle:hover{background:#6c94cf;}"
          "QScrollBar::add-line,QScrollBar::sub-line{width:0px;height:0px;border:none;}"
          "QScrollBar::add-page,QScrollBar::sub-page{background:transparent;}"
      )
          .arg(
              border, surface, ink, hover, dark ? "#afb1b8" : "#62656d", dark ? "#2c2d31" : "#f0f1f3",
              dark ? "dark" : "light", dark ? "#0a68d1" : "#0067d9", "#ffffff", dark ? "#1479e5" : "#005bc3"
          )
  );
}
} // namespace deskflow::gui
