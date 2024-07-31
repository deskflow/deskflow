/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2020 Symless Ltd.
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

#include <QSettings>
#include <QVariant>
#include <memory>

namespace synergy::gui {

class CommonConfig;

/// @brief A general config reader and writer for user and gloabl settings
class ConfigScopes : private QObject {

public:
  enum class Scope { Current, System, User };

  explicit ConfigScopes();
  ~ConfigScopes() override;

  /// @brief Checks if the setting exists
  /// @param [in] name The name of the setting to check
  /// @param [in] scope The scope to search in
  /// @return bool True if the current scope has the named setting
  bool hasSetting(const QString &name, Scope scope = Scope::Current) const;

  /// @brief Checks if the current scope settings writable
  /// @return bool True if the current scope writable
  bool isWritable() const;

  /// @brief Sets the value of a setting
  /// @param [in] name The Setting to be saved
  /// @param [in] value The Value to be saved (Templated)
  /// @param [in] scope The scope to get the value from, default is current
  /// scope
  template <typename T>
  void setSetting(const QString &name, T value, Scope scope = Scope::Current);

  /// @brief Loads a setting
  /// @param [in] name The setting to be loaded
  /// @param [in] defaultValue The default value of the setting
  /// @param [in] scope The scope to get the value from, default is current
  /// scope
  QVariant loadSetting(
      const QString &name, const QVariant &defaultValue = QVariant(),
      Scope scope = Scope::Current) const;

  /// @brief Changes the setting save and load location between System and User
  /// scope
  /// @param [in] scope The scope to set
  void setScope(Scope scope = Scope::User);

  /// @brief Get the current scope the settings are loading and save from.
  /// @return Scope An enum defining the current scope
  Scope getScope() const;

  /// @brief trigger a config load across all registered classes
  void loadAll();

  /// @brief trigger a config save across all registered classes
  void saveAll();

  /// @brief Returns the current scopes settings object
  ///         If more specialize control into the settings is needed this can
  ///         provide direct access to the settings file handler
  /// @return QSettings The Settings object as a reference
  QSettings *currentSettings() const;

  /// @brief This marks the settings as unsaved if the settings() was used to
  /// directly affect the config file
  void markUnsaved();

  /// @brief Register a class to receives requests to save and load settings
  void registerReceiever(CommonConfig *receiver);

  /// @brief Checks if any registered class has any unsaved changes
  /// @return bool True if any registered class has unsaved changes
  bool unsavedChanges() const;

private:
  void load();

  Scope m_CurrentScope = Scope::User;
  std::unique_ptr<QSettings> m_pUserSettings;
  std::unique_ptr<QSettings> m_pSystemSettings;

  /// @brief Receivers of load/save callbacks
  std::list<CommonConfig *> m_pReceievers;

  /// @brief Is set to true when settings are changed
  bool m_unsavedChanges = false;
};

template <typename T>
void ConfigScopes::setSetting(const QString &name, T value, Scope scope) {
  currentSettings()->setValue(name, value);
  m_unsavedChanges = true;
}

} // namespace synergy::gui
