#include "platform/PortalGlobalShortcuts.h"
#include "base/Log.h"
#include "base/TMethodJob.h"

namespace deskflow {


PortalGlobalShortcuts::PortalGlobalShortcuts(EiScreen* screen, IEventQueue* events)
    : m_screen{screen},
      m_events{events},
      m_portal{xdp_portal_new()}
{
  m_glibMainLoop = g_main_loop_new(nullptr, true);

  auto tMethodJob = new TMethodJob<PortalGlobalShortcuts>(this, &PortalGlobalShortcuts::glibThread);
  m_glibThread = new Thread(tMethodJob);

  auto captureCallback = [](gpointer data) { return static_cast<PortalGlobalShortcuts *>(data)->initSession(); };

  g_idle_add(captureCallback, this);
}

PortalGlobalShortcuts::~PortalGlobalShortcuts()
{
  if (g_main_loop_is_running(m_glibMainLoop))
    g_main_loop_quit(m_glibMainLoop);

  if (m_glibThread) {
    m_glibThread->cancel();
    m_glibThread->wait();
    m_glibThread = nullptr;

    g_main_loop_unref(m_glibMainLoop);
    m_glibMainLoop = nullptr;
  }

  if (m_session) {
    using enum Signal;
    XdpSession *parentSession = xdp_global_shortcuts_session_get_session(m_session);
    g_signal_handler_disconnect(m_session, m_signals.at(Activated));
    g_signal_handler_disconnect(m_session, m_signals.at(Deactivated));
    g_object_unref(m_session);
  }

  g_object_unref(m_portal);
}

gboolean PortalGlobalShortcuts::initSession()
{
  LOG_DEBUG("setting up global shortcuts session");

  xdp_portal_create_global_shortcuts_session(
    m_portal,
    nullptr,
      [](GObject *obj, GAsyncResult *res, gpointer data) {
        static_cast<PortalGlobalShortcuts *>(data)->handleInitSession(obj, res);
    },
    this
  );
  return false;
}

void PortalGlobalShortcuts::handleInitSession(GObject *object, GAsyncResult *res)
{
  g_autoptr(GError) error = nullptr;

  LOG_DEBUG("portal global shortcuts session initialized");

  auto session = xdp_portal_create_global_shortcuts_session_finish(XDP_PORTAL(object), res, &error);
  if (!session) {
    LOG_ERR("failed to initialize global shortcuts session, quitting: %s", error->message);
    g_main_loop_quit(m_glibMainLoop);
    m_events->addEvent(Event(EventTypes::Quit));
    return;
  }

  m_session = session;

  setupSession(session);
}

void PortalGlobalShortcuts::setupSession(XdpGlobalShortcutsSession *session)
{
  g_autoptr(GError) error = nullptr;

  using enum Signal;
  m_signals.at(Activated) = g_signal_connect(G_OBJECT(session), "activated", G_CALLBACK(activated), this);
  m_signals.at(Deactivated) = g_signal_connect(G_OBJECT(session), "deactivated", G_CALLBACK(deactivated), this);

  updateShortcuts(); // bind initial set
}

void PortalGlobalShortcuts::updateShortcuts()
{
  if (!m_session)
    return;

  GArray* arr = g_array_new(FALSE, FALSE, sizeof(XdpGlobalShortcut));

  m_shortcuts.clear();

  for (auto& [key, set] : m_screen->m_hotkeys) {
    for (auto& item : set.m_set) {
      std::string id = "deskflow-" + std::to_string(item.id);

      PortalHotkey ph {
          item.id,
          key,
          item.mask,
          id
      };

      m_shortcuts[id] = ph;

      XdpGlobalShortcut s{};
      s.name = g_strdup(id.c_str());
      s.description = g_strdup("Deskflow shortcut");

      g_array_append_val(arr, s);
    }
  }

  xdp_global_shortcuts_session_bind_shortcuts(
    m_session,
    arr,
    nullptr, // parent
    nullptr, // cancellable
    [](GObject *obj, GAsyncResult *res, gpointer data) {
      static_cast<PortalGlobalShortcuts *>(data)->handleBindShortcuts(obj, res);
    },
    this
  );
}

void PortalGlobalShortcuts::handleBindShortcuts(GObject* object, GAsyncResult* res)
{
    GError* error = nullptr;

    auto assigned =
        xdp_global_shortcuts_session_bind_shortcuts_finish(
            XDP_GLOBAL_SHORTCUTS_SESSION(object),
            res,
            &error);

    if (!assigned) {
        LOG_ERR("Failed to bind global shortcuts: %s",
                error ? error->message : "unknown error");
        return;
    }

    LOG_DEBUG("Global shortcuts successfully bound");

    // Optional: inspect assigned triggers (desktop may override)
    for (guint i = 0; i < assigned->len; ++i) {
        auto* item =
            &g_array_index(assigned, XdpGlobalShortcutAssigned, i);

        LOG_DEBUG("shortcut %s assigned trigger: %s",
                  item->name,
                  item->trigger_description ?
                      item->trigger_description : "(none)");
    }

    g_array_unref(assigned);
}

void PortalGlobalShortcuts::handleActivated(const char* id)
{
  // look up hotkey
  auto it = m_shortcuts.find(id);
  if (it == m_shortcuts.end())
    return;

  auto& hk = it->second;

  m_screen->onHotkey(hk.key, true, hk.mask);
}

void PortalGlobalShortcuts::handleDeactivated(const char* id)
{
  // look up hotkey
  auto it = m_shortcuts.find(id);
  if (it == m_shortcuts.end())
    return;

  auto& hk = it->second;

  m_screen->onHotkey(hk.key, false, hk.mask);
}

void PortalGlobalShortcuts::glibThread(const void *)
{
  auto context = g_main_loop_get_context(m_glibMainLoop);

  LOG_DEBUG("glib thread running");

  while (g_main_loop_is_running(m_glibMainLoop)) {
    Thread::testCancel();
    g_main_context_iteration(context, true);
  }

  LOG_DEBUG("shutting down glib thread");
}


} // namespace deskflow
