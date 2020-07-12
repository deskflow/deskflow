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
#ifndef SYNERGY_CORE_CONFIGWRITER_H
#define SYNERGY_CORE_CONFIGWRITER_H

#include <QVariant>
#include <QSettings>

/// @brief Contains GUI code
namespace GUI {
    /// @brief Contains Configuration code
    namespace Config {

        //Forward declare the class referenced by pointer
        class ConfigBase;

        class ConfigWriter:  private QObject {

        public:

            /// @brief the public way to construct the configuration calls
            ///         The pointer returned is owned by this class and should not be stored
            ///         by other classes.
            static ConfigWriter* make();

            /// @brief the public way to destroy the configuration class
            static void destroy();

            ~ConfigWriter() override;

            ///@brief An Enumeration of all the scopes available
            enum Scope { kCurrent, kSystem, kUser};

            /// @brief The choice selected when saving.
            enum SaveChoice { kSave, kCancel, kSaveToUser};

            /// @brief Checks if the setting exists
            /// @param [in] name The name of the setting to check
            /// @param [in] scope The scope to search in
            /// @return bool True if the current scope has the named setting
            bool hasSetting(const QString& name, Scope scope = kCurrent) const;

            /// @brief Sets the value of a setting
            /// @param [in] name The Setting to be saved
            /// @param [in] value The Value to be saved (Templated)
            /// @param [in] scope The scope to get the value from, default is current scope
            template <typename T>
            void setSetting(const QString& name, T value, Scope scope = kCurrent);

            /// @brief Loads a setting
            /// @param [in] name The setting to be loaded
            /// @param [in] defaultValue The default value of the setting
            /// @param [in] scope The scope to get the value from, default is current scope
            QVariant loadSetting(const QString& name, const QVariant& defaultValue = QVariant(), Scope scope = kCurrent);

            /// @brief Changes the setting save and load location between System and User scope
            /// @param [in] scope The scope to set
            void setScope(Scope scope = kUser);

            /// @brief Get the current scope the settings are loading and save from.
            /// @return Scope An enum defining the current scope
            Scope getScope() const;

            /// @brief trigger a config load across all registered classes
            void globalLoad();

            /// @brief trigger a config save across all registered classes
            void globalSave();

            /// @brief Saves the settings to file
            void save();

            /// @brief Returns the current scopes settings object
            ///         If more specialize control into the settings is needed this can provide
            ///         direct access to the settings file handler
            /// @return QSettings The Settings object as a reference
            QSettings& settings();

            /// @brief This marks the settings as unsaved if the settings() was used to directly affect the config file
            void markUnsaved();

            /// @brief Register a class to receives globalLoad and globalSave events
            /// @param [in] ConfigBase The class that will receive the events
            void registerClass(ConfigBase* receiver);

            /// @brief Checks if any registered class has any unsaved changes
            /// @return bool True if any registered class has unsaved changes
            bool unsavedChanges() const;

            /// @brief If the scope is set to system, this function will query the user
            ///         if they want to continue saving to global scope or switch to user scope
            ///         if the scope is set to User the function will just return Save
            /// @return SaveChoice The choice that was selected, or Save if the scope is user already
            SaveChoice checkSystemSave() const;

        protected:

            Scope           m_CurrentScope      = kUser;    /// @brief The current scope of the settings

            QSettings*      m_pSettingsCurrent  = nullptr;  /// @brief The currently active settings
            QSettings*      m_pSettingsUser     = nullptr;  /// @brief The user specific settings
            QSettings*      m_pSettingsSystem   = nullptr;  /// @brief The system wide settings

        private:

            /// @brief Contains a list all all classes that hook into the writer.
            ///         This allows all classes that save settings to be called an updated
            ///         on a save and reload by any other class
            std::list<ConfigBase*> m_pCallerList;

            /// @brief if this class modified settings then set the flag
            bool m_unsavedChanges = false;

            /// @brief The constructor, as this is a singolton we want to control who can call the constructor
            ConfigWriter();

            /// @brief the pointer of the ConfigWriter for singolton use
            static ConfigWriter* s_pConfiguration;

            /// @brief Returns the OS specific settings ini file location
            static QString getSystemSettingPath();


            /// @brief deletes pointers and sets the value to null
            template<class T> static inline void destroy(T*& p) { delete p; p = 0; }
        };

        // Implementation of a template function needs to be visible to all calls thus is must be in the header
        // Moved so its not bulking out the class definition
        template<typename T>
        void ConfigWriter::setSetting(const QString& name, T value, Scope scope) {
            switch (scope){
                case kUser:
                    m_pSettingsUser->setValue(name, value);
                    break;
                case kSystem:
                    m_pSettingsSystem->setValue(name, value);
                    break;
                default:
                    m_pSettingsCurrent->setValue(name, value);
                    break;
            }
            m_unsavedChanges = true;
        }
    }
}
#endif //SYNERGY_CORE_CONFIGWRITER_H
