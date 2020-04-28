/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2020 - 2020 Symless Ltd.
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

#ifndef SYNERGY_CORE_CONFIGBASE_H
#define SYNERGY_CORE_CONFIGBASE_H

namespace GUI {
    namespace Config {

        ///@brief This abstract class will be used by all classes that use the ConfigWriter
        ///       to allow global saving and loading
        class ConfigBase {
        public :
            ConfigBase() = default;

            virtual ~ConfigBase() = default;

            /// @brief The function that is called when the settings need to be loaded from file
            virtual void loadSettings() = 0;

            /// @brief The function that is called when the settings need to be saved to file
            virtual void saveSettings() = 0;

            /// @brief Returns true if the class has marked itself with having unsaved changes
            bool modified() const { return m_unsavedChanges; }

        protected:
            /// @brief Does the class have unsaved changes in it.
            bool m_unsavedChanges = false;

        };
    }
}
#endif //SYNERGY_CORE_CONFIGBASE_H
