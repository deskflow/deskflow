/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <gio/gio.h>

#include "common/Constants.h"
#include "common/PlatformInfo.h"

namespace deskflow::platform {

// Async callback for the Register call. The call is fire-and-forget; we only
// log a failure.
static void onRegisterComplete(GObject *source, GAsyncResult *res, gpointer user_data)
{
  GError *error = nullptr;
  GVariant *result = g_dbus_proxy_call_finish(G_DBUS_PROXY(source), res, &error);
  if (result) {
    g_variant_unref(result);
  }
  if (error) {
    qWarning() << "failed to register app id:" << error->message;
    g_error_free(error);
  }
}

/**
 * @brief setAppId Set the app id for the xdg portal registry
 * All Applications should do this before attempting any other portal calls
 * If the app is runing from inside a sandbox (i.e flatpak or a snap) this call does nothing
 */
inline void setAppId()
{
  // Sandboxed applications are unable to use this portal
  if (isSandboxed()) {
    return;
  }

  // Register our app id on the GLib session-bus connection -- the same
  // singleton that libportal uses for its portal calls -- so the portal proxy
  // associates this D-Bus peer (same unique name) with our app id when it
  // forwards portal requests. Registering on Qt's separate session-bus
  // connection associates the app id with a different D-Bus peer than the
  // one libportal uses, so the proxy forwards an empty app id instead.
  //
  // The call is sent asynchronously (no blocking). A blocking
  // g_dbus_proxy_call_sync would spin the GLib main loop, which Qt's
  // QEventDispatcherGlib shares, and crash the Wayland client during startup.
  GError *error = nullptr;
  GDBusConnection *bus = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
  GDBusProxy *registry = g_dbus_proxy_new_sync(
      bus,
      G_DBUS_PROXY_FLAGS_NONE,
      nullptr,
      "org.freedesktop.portal.Desktop",
      "/org/freedesktop/portal/desktop",
      "org.freedesktop.host.portal.Registry",
      nullptr,
      &error);
  if (!registry) {
    if (error) {
      qWarning() << "portal registry not found:" << error->message;
      g_error_free(error);
    } else {
      qWarning() << "portal registry not found";
    }
    return;
  }

  // Register(app_id: s, options: a{sv})
  // options is an empty a{sv}; its element type is {sv}.
  // g_variant_new_tuple takes ownership of its children, so appid and options
  // must not be unref'd separately. g_dbus_proxy_call takes ownership of params.
  GVariantType *elementType = g_variant_type_new("{sv}");
  GVariant *options = g_variant_new_array(elementType, nullptr, 0);
  g_variant_type_free(elementType);
  GVariant *appid = g_variant_new_string(kRevFqdnName);
  GVariant *children[2] = {appid, options};
  GVariant *params = g_variant_new_tuple(children, 2);

  // Send Register without blocking the main loop.
  g_dbus_proxy_call(registry, "Register", params, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, onRegisterComplete, nullptr);
  g_object_unref(registry);
}

} // namespace deskflow::platform
