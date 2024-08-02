/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Symless Ltd.
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

#include "version.h"

const QString kVersion = SYNERGY_VERSION;

#ifdef GIT_SHA_SHORT
const QString kVersionGitSha = GIT_SHA_SHORT;
#else
const QString kVersionGitSha;
#endif

namespace synergy::gui {

QString version() {
  QString result(kVersion);
  if (!kVersionGitSha.isEmpty()) {
    result.append(QString(" (%1)").arg(kVersionGitSha));
  }
  return result;
}

} // namespace synergy::gui
