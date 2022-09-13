/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2022 Symless Ltd.
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
#include "CreditsLoader.h"
#include "AboutDialogEliteBackers.h"

AboutDialogEliteBackers::AboutDialogEliteBackers(MainWindow* parent, const AppConfig& config) :
    AboutDialog(parent, config),
    credits(*parent, config)
{
    setupCreditsLoader();

    textEliteBackers->show();
    labelEliteBackers->show();
    labelEliteBackerLink->show();
    labelCreditsLink->show();
}

void AboutDialogEliteBackers::setupCreditsLoader()
{
    this->textEliteBackers->setText("Loading...");
    this->textEliteBackers->viewport()->setAutoFillBackground(false);
    this->textEliteBackers->document()->setDocumentMargin(0);

    connect(&credits, SIGNAL(loaded(const QString&)), this, SLOT(updateEliteBackers(const QString&)));
    credits.loadEliteBackers();
}

void AboutDialogEliteBackers::updateEliteBackers(const QString& eliteBackers) const
{
    this->textEliteBackers->setText(eliteBackers);
}

QString AboutDialogEliteBackers::getCopyrights() const
{
    QString buildDateString = QString::fromLocal8Bit(__DATE__).simplified();
    QDate buildDate = QLocale("en_US").toDate(buildDateString, "MMM d yyyy");

    QString aboutText(R"(<p>Keyboard and mouse sharing application. Cross platform and open source since 2001.<br /><br />
                        Copyright © %%YEAR%% Symless Ltd.<br /><br />
                        Synergy is released under the GNU General Public License (GPLv2).</p>)");

    return aboutText.replace(QString("%%YEAR%%"), QString::number(buildDate.year()));
}

void AboutDialogEliteBackers::resizeWindow()
{
// change default size based on os
#if defined(Q_OS_MAC)
    QSize size(600, 490);
    setMaximumSize(size);
    setMinimumSize(size);
    resize(size);
#elif defined(Q_OS_LINUX)
    QSize size(600, 420);
    setMaximumSize(size);
    setMinimumSize(size);
    resize(size);
#endif
}
