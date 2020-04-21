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
};

class ConfigWriter {

public:

    /// @brief the public way to construct the configuration calls
    static ConfigWriter* make();

    /// @brief the public way to destroy the configuration class
    static void destroy();

    ~ConfigWriter();

    ///@brief An Enumeration of all the scopes available
    enum Scope { System, User};

    /// @brief Sets the value of a setting
    /// @param [in] name The Setting to be saved
    /// @param [in] value The Value to be saved
    template <typename T>
    void setSetting(const char* name, T value);

    /// @brief Loads a setting
    /// @param [in] name The setting to be loaded
    /// @param [in] defaultValue The default value of the setting
    QVariant loadSetting(const char* name, const QVariant& defaultValue = QVariant());

    /// @brief Changes the setting save and load location between System and User scope
    /// @param [in] scope The scope to set
    void setScope(Scope scope = User);

    /// @brief Get the current scope the settings are loading and save from.
    /// @return Scope An enum defining the current scope
    Scope getScope() const;

    /// @brief trigger a config load across all registered classes
    void gloablLoad();

    /// @brief trigger a config save across all registered classes
    void globalSave();

protected:

    Scope           m_CurrentScope      = User;     /// @brief The current scope of the settings

    QSettings*      m_pSettingsCurrent  = nullptr;  /// @brief The currently active settings
    QSettings*      m_pSettingsUser     = nullptr;  /// @brief The user specific settings
    QSettings*      m_pSettingsSystem   = nullptr;  /// @brief The system wide settings

private:

    /// @brief Contains a list all all classes that hook into the writer.
    ///         This allows all classes that save settings to be called an updated
    ///         on a save and reload by any other class
    std::list<ConfigBase*> m_pCallerList;

    /// @brief The constructor, as this is a singolton we want to control who can call the constructor
    ConfigWriter();

    /// @brief the pointer of the
    static ConfigWriter* s_pConfiguration;

    /// @brief deletes pointers and sets the value to null
    template<class T> static inline void destroy(T*& p) { delete p; p = 0; }
};


#endif //SYNERGY_CORE_CONFIGWRITER_H
