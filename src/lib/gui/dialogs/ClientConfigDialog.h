/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QDialog>

namespace Ui {
class ClientConfigDialog;
}

/**
 * @brief The ClientConfigDialog class
 * Simple dialog to allow users to configure the Client settings
 */
class ClientConfigDialog : public QDialog
{
  Q_OBJECT

public:
  explicit ClientConfigDialog(QWidget *parent = nullptr);
  ~ClientConfigDialog() override;

protected:
  void changeEvent(QEvent *e) override;

private:
  /**
   * @brief updateText update widget text
   */
  void updateText() const;

  /**
   * @brief initConnections
   * Sets up all the connections
   */
  void initConnections() const;

  /**
   * @brief isModified
   * @return true when any client settings in the gui do not match the stored settings values.
   */
  bool isModified() const;

  /**
   * @brief isDefault
   * @return true if all client settings match the default values
   */
  bool isDefault() const;

  /**
   * @brief setButtonBoxEnabledButtons
   * Enable / Disable the button box buttons based on the state of the gui
   */
  void setButtonBoxEnabledButtons() const;

  /**
   * @brief Load the client setting into the gui
   */
  void load();

  /**
   * @brief Set the gui values to the defalut values for all client settings
   */
  void resetToDefault();

  /**
   * @brief save to settings and then calls QDialog::accept
   */
  void save();

  Ui::ClientConfigDialog *ui = nullptr;
};
