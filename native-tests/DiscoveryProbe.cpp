// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
// Bounded metadata-only probe. Default is dry-run. No input core, TLS files,
// production settings, daemon IPC, services, firewall or trust APIs are linked.
#include "gui/core/DiscoveryScope.h"
#include "gui/core/DiscoveryTransport.h"
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>
#include <QTimer>
#include <cstdio>
#include <memory>
#ifdef Q_OS_MACOS
#include <SystemConfiguration/SystemConfiguration.h>
#endif
using namespace deskflow::gui;
int main(int argc, char **argv)
{
  QCoreApplication app(argc, argv);
  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addOptions(
      {{"interface", "Exact physical interface name", "name"},
       {"address", "Exact selected private IPv4 address", "IPv4"},
       {"duration", "Bounded seconds, 1..60", "seconds", "60"},
       {"mode", "browse or advertise", "mode", "browse"},
       {"allow-wifi", "Allow the explicitly approved physical Wi-Fi interface as well as Ethernet."},
       {"execute", "Actually run metadata discovery. Requires separate explicit authorization."}}
  );
  parser.process(app);
  bool ok = false;
  const int seconds = parser.value("duration").toInt(&ok);
  const auto mode = parser.value("mode");
  if (!ok || seconds < 1 || seconds > 60 || (mode != "browse" && mode != "advertise"))
    return 2;
  DiscoveryScope selected;
  QJsonArray eligible;
  for (const auto &s : DiscoveryScope::current()) {
    eligible.append(
        QJsonObject{
            {"interface", s.name}, {"index", int(s.index)}, {"ipv4", s.address.toString()}, {"prefix", s.prefix}
        }
    );
    if (s.name == parser.value("interface") && s.address == QHostAddress(parser.value("address")))
      selected = s;
  }
  const auto qtType = QNetworkInterface::interfaceFromName(selected.name).type();
  bool wired = qtType == QNetworkInterface::Ethernet;
  bool wifi = qtType == QNetworkInterface::Wifi;
  QString nativeType;
#ifdef Q_OS_MACOS
  // Qt may classify a Darwin Ethernet adapter as Unknown. Use the same native
  // interface inventory as DiscoveryScope; never infer Ethernet from its name.
  wired = false;
  wifi = false;
  if (const auto interfaces = SCNetworkInterfaceCopyAll()) {
    for (CFIndex i = 0; i < CFArrayGetCount(interfaces); ++i) {
      const auto interface = static_cast<SCNetworkInterfaceRef>(CFArrayGetValueAtIndex(interfaces, i));
      const auto bsd = SCNetworkInterfaceGetBSDName(interface), type = SCNetworkInterfaceGetInterfaceType(interface);
      char name[128]{}, kind[128]{};
      if (bsd && type && CFStringGetCString(bsd, name, sizeof(name), kCFStringEncodingUTF8) &&
          selected.name == QString::fromUtf8(name)) {
        wired = CFEqual(type, kSCNetworkInterfaceTypeEthernet);
        wifi = CFEqual(type, kSCNetworkInterfaceTypeIEEE80211);
        if (CFStringGetCString(type, kind, sizeof(kind), kCFStringEncodingUTF8))
          nativeType = QString::fromUtf8(kind);
        break;
      }
    }
    CFRelease(interfaces);
  }
#endif
  const DiscoveryAdvertisement ad{selected, DiscoveryAdvertisement::newToken(), 34800};
  QJsonObject plan{
      {"execute", parser.isSet("execute")},
      {"mode", mode},
      {"eligible", eligible},
      {"selected", selected.name},
      {"interface_index", int(selected.index)},
      {"wired_ethernet", wired},
      {"wifi", wifi},
      {"allow_wifi", parser.isSet("allow-wifi")},
      {"qt_interface_type", int(qtType)},
      {"native_interface_type", nativeType},
      {"duration_seconds", seconds},
      {"service_type", "_deskflow._tcp.local"},
      {"mdns_udp_port", 5353},
      {"advertised_tcp_port", 34800},
      {"opens_tcp_listener", false},
      {"input_control", false},
      {"tls_credentials", false},
      {"instance", ad.instance()},
      {"host", ad.host()},
      {"txt_properties", 0}
  };
  const auto bytes = QJsonDocument(plan).toJson();
  std::fwrite(bytes.constData(), 1, bytes.size(), stdout);
  std::fflush(stdout);
  if (!parser.isSet("execute"))
    return 0;
  // Wi-Fi remains opt-in for this bounded probe. Physical scope and exact
  // private address are revalidated by the transport before starting.
  if (!selected.valid() || !(wired || (wifi && parser.isSet("allow-wifi"))))
    return 3;
  std::unique_ptr<DiscoveryTransport> transport(createDiscoveryTransport(nullptr));
  int found = 0;
  bool sawActive = false, endedEarly = false;
  QObject::connect(transport.get(), &DiscoveryTransport::activeChanged, &app, [&](bool active) {
    sawActive = sawActive || active;
  });
  QObject::connect(transport.get(), &DiscoveryTransport::status, &app, [](const QString &message) {
    const auto event = QJsonDocument(QJsonObject{{"status", message}}).toJson(QJsonDocument::Compact);
    std::fwrite(event.constData(), 1, event.size(), stdout);
    std::fputc('\n', stdout);
    std::fflush(stdout);
  });
  QObject::connect(
      transport.get(), &DiscoveryTransport::resolved, &app,
      [&](const QString &, const QString &, const QString &, int, int) { ++found; }
  );
  QObject::connect(transport.get(), &DiscoveryTransport::sessionEnded, &app, [&] {
    endedEarly = true;
    app.quit();
  });
  QTimer limit;
  limit.setSingleShot(true);
  limit.setTimerType(Qt::PreciseTimer);
  QObject::connect(&limit, &QTimer::timeout, &app, [&] {
    transport->stop();
    app.quit();
  });
  // Readiness and default dry-run have already been handled; never infer approval.
  QTimer::singleShot(0, &app, [&] {
    limit.start(seconds * 1000);
    if (mode == "browse")
      transport->browse(selected);
    else
      transport->advertise(ad);
  });
  const int code = app.exec();
  transport->stop();
  std::printf(
      "{\"finished\":true,\"candidate_count\":%d,\"became_active\":%s,\"ended_early\":%s}\n", found,
      sawActive ? "true" : "false", endedEarly ? "true" : "false"
  );
  return code ? code : (!sawActive || endedEarly ? 4 : 0);
}
