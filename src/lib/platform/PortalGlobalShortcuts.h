#pragma once

#include "mt/Thread.h"
#include "platform/EiScreen.h"

#include <glib.h>
#include <libportal/globalshortcuts.h>
#include <libportal/portal.h>

namespace deskflow {

class EiScreen;

class PortalGlobalShortcuts
{
public:
  PortalGlobalShortcuts(EiScreen *screen, IEventQueue *events);
  ~PortalGlobalShortcuts();

  void updateShortcuts();

private:
  void glibThread(const void *);
  gboolean initSession();
  void setupSession(XdpGlobalShortcutsSession *session);

  void handleInitSession(GObject *object, GAsyncResult *res);
  void handleBindShortcuts(GObject *object, GAsyncResult *res);

  void handleActivated(const char *id);
  void handleDeactivated(const char *id);

  static void activated(
      const XdpGlobalShortcutsSession *session, const char *id, guint, const gpointer data
  )
  {
    static_cast<PortalGlobalShortcuts *>(data)->handleActivated(id);
  }
  static void deactivated(
      const XdpGlobalShortcutsSession *session, const char *id, guint, const gpointer data
  )
  {
    static_cast<PortalGlobalShortcuts *>(data)->handleDeactivated(id);
  }

private:
  enum class Signal : uint8_t
  {
    Activated,
    Deactivated
  };

  struct PortalHotkey {
    uint32_t localId;
    KeyID key;
    KeyModifierMask mask;
    std::string portalId;
  };

  EiScreen *m_screen = nullptr;
  IEventQueue *m_events = nullptr;

  Thread *m_glibThread;
  GMainLoop *m_glibMainLoop = nullptr;

  XdpPortal *m_portal = nullptr;
  XdpGlobalShortcutsSession *m_session = nullptr;

  std::map<Signal, gulong> m_signals = {
      {Signal::Activated, 0}, {Signal::Deactivated, 0}
  };

  std::map<std::string, PortalHotkey> m_shortcuts;
};

}