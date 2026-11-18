/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Applone <app1oneme@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <gio/gio.h>
#include <libportal/portal.h>

#include <functional>
#include <memory>
#include <thread>

namespace deskflow {

// Each portal owns its dispatch context; Qt must never iterate it concurrently.
class PortalContext
{
public:
  PortalContext() = default;

  ~PortalContext()
  {
    stop();
    if (m_portal)
      g_object_unref(m_portal);
    g_object_unref(m_cancel);
    g_main_context_unref(m_context);
  }

  PortalContext(const PortalContext &) = delete;
  PortalContext &operator=(const PortalContext &) = delete;

  void start()
  {
    m_thread = std::thread([this] {
      g_main_context_push_thread_default(m_context);
      while (!m_stopping || m_pending != 0)
        g_main_context_iteration(m_context, true);
      g_main_context_pop_thread_default(m_context);
    });
  }

  void stop()
  {
    if (!m_thread.joinable())
      return;
    // Cancel from the caller as the worker may be in a synchronous portal call.
    g_cancellable_cancel(m_cancel);
    defer([this] { m_stopping = true; });
    m_thread.join();
  }

  GMainContext *get() const
  {
    return m_context;
  }
  // Called before start(), so libportal captures this context rather than Qt's.
  XdpPortal *portal()
  {
    if (!m_portal) {
      g_main_context_push_thread_default(m_context);
      m_portal = xdp_portal_new();
      g_main_context_pop_thread_default(m_context);
    }
    return m_portal;
  }
  GCancellable *cancellable() const
  {
    return m_cancel;
  }

  GSource *post(std::function<void()> function, unsigned delayMs = 0) const
  {
    auto *source = delayMs ? g_timeout_source_new(delayMs) : g_idle_source_new();
    g_source_set_priority(source, G_PRIORITY_DEFAULT);
    auto *job = new std::function<void()>(std::move(function));
    g_source_set_callback(
        source,
        [](gpointer data) -> gboolean {
          (*static_cast<std::function<void()> *>(data))();
          return G_SOURCE_REMOVE;
        },
        job, [](gpointer data) { delete static_cast<std::function<void()> *>(data); }
    );
    g_source_attach(source, m_context);
    // Caller owns this reference and may use it to cancel a scheduled job.
    return source;
  }

  void defer(std::function<void()> function) const
  {
    g_source_unref(post(std::move(function)));
  }

  using Completion = std::function<void(GObject *, GAsyncResult *)>;
  gpointer callback(Completion function)
  {
    ++m_pending;
    return new Pending{this, std::move(function)};
  }

  static void complete(GObject *object, GAsyncResult *result, gpointer data)
  {
    std::unique_ptr<Pending> pending(static_cast<Pending *>(data));
    // Finish cancellation callbacks too, so their native results are released.
    pending->function(object, result);
    --pending->owner->m_pending;
  }

private:
  struct Pending
  {
    PortalContext *owner;
    Completion function;
  };
  GMainContext *m_context = g_main_context_new();
  GCancellable *m_cancel = g_cancellable_new();
  XdpPortal *m_portal = nullptr;
  std::thread m_thread;
  bool m_stopping = false;
  unsigned m_pending = 0;
};

} // namespace deskflow
