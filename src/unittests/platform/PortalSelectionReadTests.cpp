/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Applone <app1oneme@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "platform/EiClipboard.h"
#include "platform/PortalContext.h"
#include "platform/PortalSelectionRead.h"
#include <QTest>
#include <atomic>
#include <unistd.h>

using deskflow::PortalSelectionRead;
using Format = IClipboard::Format;

class PortalSelectionReadTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void contextDrainsCancelledAsyncOperations()
  {
    deskflow::PortalContext context;
    std::atomic<bool> started{false}, completed{false};
    context.start();
    context.defer([&] {
      auto *task = g_task_new(
          nullptr, context.cancellable(), deskflow::PortalContext::complete,
          context.callback([&](GObject *, GAsyncResult *result) {
            GError *error = nullptr;
            g_task_propagate_boolean(G_TASK(result), &error);
            completed = error && g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
            g_clear_error(&error);
          })
      );
      g_task_run_in_thread(task, [](GTask *task, gpointer, gpointer, GCancellable *cancel) {
        for (int i = 0; i < 1000 && !g_cancellable_is_cancelled(cancel); ++i)
          g_usleep(1000);
        if (!g_task_return_error_if_cancelled(task))
          g_task_return_boolean(task, true);
      });
      g_object_unref(task);
      started = true;
    });
    QTRY_VERIFY(started.load());
    context.stop();
    QVERIFY(completed.load());
  }

  void deadlineDiscardsPreviouslyCompletedFormats()
  {
    auto *context = g_main_context_new();
    {
      PortalSelectionRead reader(context);
      int text[2], stalled[2];
      QCOMPARE(pipe(text), 0);
      QCOMPARE(pipe(stalled), 0);
      QCOMPARE(write(text[1], "complete", 8), 8);
      close(text[1]);
      bool done = false;
      reader.start(
          {{Format::Text, [fd = text[0]] { return fd; }}, {Format::Bitmap, [fd = stalled[0]] { return fd; }}}, 100,
          [&](auto values) {
            QVERIFY(values.isEmpty());
            done = true;
          },
          20
      );
      while (!done)
        g_main_context_iteration(context, true);
      close(stalled[1]);
    }
    g_main_context_unref(context);
  }

  void staleReadDoesNotReplaceNewerClipboard()
  {
    deskflow::EiClipboard clipboard(kClipboardClipboard);
    const auto revision = clipboard.revision();
    clipboard.open(0);
    clipboard.add(Format::Text, "new selection");
    clipboard.close();
    QVERIFY(!clipboard.replace({{Format::Text, "stale selection"}}, revision));
    clipboard.open(0);
    QCOMPARE(clipboard.get(Format::Text), std::string("new selection"));
    clipboard.close();
  }

  void readsUntilEof()
  {
    auto *context = g_main_context_new();
    {
      PortalSelectionRead reader(context);
      int descriptors[2];
      QCOMPARE(pipe(descriptors), 0);
      QCOMPARE(write(descriptors[1], "hello\r\n", 7), 7);
      close(descriptors[1]);
      bool done = false;
      reader.start({{Format::Text, [fd = descriptors[0]] { return fd; }}}, 7, [&](auto values) {
        QCOMPARE(values.size(), 1);
        QCOMPARE(values.first().second, QByteArray("hello\n"));
        done = true;
      });
      while (!done)
        g_main_context_iteration(context, true);
    }
    g_main_context_unref(context);
  }

  void stalledProducerDoesNotBlockContext()
  {
    auto *context = g_main_context_new();
    {
      PortalSelectionRead reader(context);
      int descriptors[2];
      QCOMPARE(pipe(descriptors), 0);
      bool ticked = false;
      auto *idle = g_idle_source_new();
      g_source_set_callback(
          idle,
          [](gpointer p) -> gboolean {
            *static_cast<bool *>(p) = true;
            return G_SOURCE_REMOVE;
          },
          &ticked, nullptr
      );
      g_source_attach(idle, context);
      g_source_unref(idle);
      bool done = false;
      reader.start(
          {{Format::Text, [fd = descriptors[0]] { return fd; }}}, 100,
          [&](auto values) {
            QVERIFY(values.isEmpty());
            done = true;
          },
          20
      );
      while (!done)
        g_main_context_iteration(context, true);
      QVERIFY(ticked);
      close(descriptors[1]);
    }
    g_main_context_unref(context);
  }

  void oversizedSelectionIsDiscarded()
  {
    auto *context = g_main_context_new();
    {
      PortalSelectionRead reader(context);
      int descriptors[2];
      QCOMPARE(pipe(descriptors), 0);
      QCOMPARE(write(descriptors[1], "oversize", 8), 8);
      close(descriptors[1]);
      bool done = false;
      reader.start({{Format::Text, [fd = descriptors[0]] { return fd; }}}, 7, [&](auto values) {
        QVERIFY(values.isEmpty());
        done = true;
      });
      while (!done)
        g_main_context_iteration(context, true);
    }
    g_main_context_unref(context);
  }

  void cancellationDiscardsPartialSelection()
  {
    auto *context = g_main_context_new();
    {
      PortalSelectionRead reader(context);
      int descriptors[2];
      QCOMPARE(pipe(descriptors), 0);
      QCOMPARE(write(descriptors[1], "partial", 7), 7);
      bool completed = false;
      reader.start({{Format::Text, [fd = descriptors[0]] { return fd; }}}, 100, [&](auto) { completed = true; });
      g_main_context_iteration(context, false);
      reader.cancel();
      close(descriptors[1]);
      while (g_main_context_pending(context))
        g_main_context_iteration(context, false);
      QVERIFY(!completed);
    }
    g_main_context_unref(context);
  }
};

QTEST_GUILESS_MAIN(PortalSelectionReadTests)
#include "PortalSelectionReadTests.moc"
