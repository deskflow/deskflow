/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Symless Ltd.
 * Copyright (C) 2008 Volker Lanz (vl@fidra.de)
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

#pragma once

#include "ui_AboutDialogBase.h"

#include <QDialog>
#include <QMainWindow>

class QWidget;
class QString;

class AboutDialog : public QDialog, public Ui::AboutDialogBase
{
  Q_OBJECT

public:
  struct Deps
  {
    virtual ~Deps() = default;
    virtual bool isDarkMode() const;
  };

  explicit AboutDialog(QMainWindow *parent, std::shared_ptr<Deps> deps = std::make_shared<Deps>());
  int exec() override;

private:
  std::shared_ptr<Deps> m_pDeps;

  void updateLogo() const;
  void setLogo(const char *const &filename) const;
  QString importantDevelopers() const;
};
