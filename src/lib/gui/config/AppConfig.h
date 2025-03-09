/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "IAppConfig.h"
#include "IConfigScopes.h"
#include "common/constants.h"
#include "gui/paths.h"

#include <QDir>
#include <QObject>
#include <QPoint>
#include <QSize>
#include <QString>
#include <QVariant>
#include <optional>

/**
 * @brief Simply reads and writes app settings.
 *
 * Important: Maintain a clear separation of concerns and keep it simple.
 * It is tempting to add logic (e.g. license checks) to this class since it
 * instance is widely accessible, but that has previously led to this class
 * becoming a god object.
 */
class AppConfig : public QObject, public deskflow::gui::IAppConfig
{
  using IConfigScopes = deskflow::gui::IConfigScopes;

  Q_OBJECT

private:
  enum class Setting
  {
    // kScreenName = 0, Moved to deskflow settings
    // kPort = 1, moved to deskflow settings
    // kInterface = 2, moved to deskflow settings
    // kLogLevel = 3, moved to deskflow settings
    // 4 = LogToFile moved to deskflow settings
    // 5 = logfilename moved to deskflow settings
    // 6 = show first run wizard, obsolete
    // 7 Started before moved to deskflow settings
    // kElevateModeLegacy = 8,
    // kElevateMode = 9,
    // 10 = edition, obsolete (related to obsolete licensing)
    // 11 = tlsEnagled moved to Settigns
    // 12 = auto hide, Moved to Settings
    // 13 = serial key, obsolete
    // 14 = kLastVersion, moved to deskflow settings
    // 15 = last expire time, obsolete
    // 16 = activation has run, obsolete
    // 17 = minimize to tray, obsolete
    // 18 = activate email, obsolete
    kLoadSystemSettings = 19,
    kServerGroupChecked = 20,
    // 21 = use external config moved to deskflow settings
    // 22 = k config file moved to deskflow settings
    kUseInternalConfig = 23,
    kClientGroupChecked = 24,
    // 25 = serverHostName moved to deskflow settings
    // 26 = kTlsCertPath moved to deskflow settings
    // 27 = tlsKeyLength Moved to deskflow settings
    // 28 = Prevent Sleep moved to deskflow settings
    // 29 = language Sync moved to deskflow settings
    // 30 = InvertScrollDirection moved to deskflow settings
    // 31 = guid, obsolete
    // 32 = license registry url, obsolete
    // 33 = license next check, obsolete
    // 34 = InvertConnection, obsolete
    // 35 = client-host-mode, obsolete
    // 36 = server-client-mode, obsolete
    // kEnableService = 37, moved to deskflow settings
    // 38, close to tray moved to deskflow settings
    // 39 window size moved to deskflow settings
    // 40 window position moved to deskflow settings
    // 41 = show dev thanks, obsolete
    // 42, close reminder moved to deskflow settings
    // 43 = Enable Update Check,
    // 44 = LogExpanded, Moved to deskflow settings
    // 45 = Colorful Icon, Moved to deskflow settings
    // kRequireClientCert = 46 Moved to deskflow settings
  };

public:
  struct Deps
  {
    virtual ~Deps() = default;
  };

  explicit AppConfig(IConfigScopes &scopes, std::shared_ptr<Deps> deps = std::make_shared<Deps>());

  void determineScope();

  /**
   * @brief Commits the current settings to the active scope.
   * This should only be called when the settings are about to be saved.
   */
  void commit();

  //
  // Getters (overrides)
  //

  IConfigScopes &scopes() const override;
  bool isActiveScopeWritable() const override;
  bool isActiveScopeSystem() const override;
  bool clientGroupChecked() const override;

  //
  // Getters (new methods)
  //

  bool serverGroupChecked() const;
  bool useInternalConfig() const;

  //
  // Setters (new methods)
  //

  void setServerGroupChecked(bool);
  void setUseInternalConfig(bool);
  void setClientGroupChecked(bool);

  /// @brief Sets the user preference to load from SystemScope.
  /// @param [in] value
  ///             True - This will set the variable and load the global scope
  ///             settings. False - This will set the variable and load the user
  ///             scope settings.
  void setLoadFromSystemScope(bool value) override;

private:
  static QString settingName(AppConfig::Setting name);

  void recall();
  void recallScreenName();
  void recallFromAllScopes();
  void recallFromCurrentScope();

  /**
   * @brief Loads a setting if it exists, otherwise returns `std::nullopt`
   *
   * @param toType A function to convert the QVariant to the desired type.
   */
  template <typename T>
  std::optional<T> getFromCurrentScope(Setting name, std::function<T(const QVariant &)> toType) const;

  /**
   * @brief Sets a setting if the value is not `std::nullopt`.
   */
  template <typename T> void setInCurrentScope(Setting name, const std::optional<T> &value);

  /// @brief Sets the value of a setting
  /// @param [in] name The Setting to be saved
  /// @param [in] value The Value to be saved
  template <typename T> void setInCurrentScope(AppConfig::Setting name, T value);

  /// @brief Sets the value of a common setting
  /// which should have the same value for all scopes
  /// @param [in] name The Setting to be saved
  /// @param [in] value The Value to be saved
  template <typename T> void saveToAllScopes(AppConfig::Setting name, T value);

  QVariant getFromCurrentScope(AppConfig::Setting name, const QVariant &defaultValue = QVariant()) const;

  /**
   * @brief Finds a value by searching each scope starting with the current
   * scope.
   */
  QVariant findInAllScopes(AppConfig::Setting name, const QVariant &defaultValue = QVariant()) const;

  /// @brief This method loads config from specified scope
  /// @param [in] scope which should be loaded.
  void loadScope(IConfigScopes::Scope scope);

  static const char m_LogDir[];

  /// @brief Contains the string values of the settings names that will be saved
  static const char *const m_SettingsName[];

  bool m_ServerGroupChecked = false;
  bool m_UseInternalConfig = false;
  bool m_ClientGroupChecked = false;
  bool m_LoadFromSystemScope = false;

  deskflow::gui::IConfigScopes &m_Scopes;
  std::shared_ptr<Deps> m_pDeps;
};
