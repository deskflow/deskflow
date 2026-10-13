/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/WlrClipboard.h"
#include "platform/wlr-data-control-protocol.h"
#include "base/Log.h"

// Ensure logging calls never dereference null if Log singleton is uninitialized
#undef LOG
#define LOG(_a1)                                                                                                       \
  do {                                                                                                                 \
    if (Log::getInstance()) {                                                                                          \
      CLOG->print _a1;                                                                                                 \
    }                                                                                                                  \
  } while (0)

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <wayland-client.h>

#include <QBuffer>
#include <QDataStream>
#include <QImage>
#include <QtEndian>

namespace deskflow {

namespace {

constexpr int kBmpSignatureSize = 2;
constexpr quint32 kBmpFileHeaderSize = 14;
constexpr quint32 kMinDibHeaderSize = 12;
constexpr int kIoTimeoutMs = 250;
constexpr size_t kChunkSize = 32 * 1024;

QByteArray dibToBmp(const QByteArray &dib)
{
  if (dib.size() < static_cast<qint64>(sizeof(quint32))) {
    return {};
  }

  quint32 headerSize = 0;
  std::memcpy(&headerSize, dib.constData(), sizeof(headerSize));
  headerSize = qFromLittleEndian(headerSize);
  if (headerSize < kMinDibHeaderSize || headerSize > static_cast<quint32>(dib.size())) {
    return {};
  }

  const auto fileSize = static_cast<quint32>(kBmpFileHeaderSize + dib.size());
  const quint32 pixelOffset = kBmpFileHeaderSize + headerSize;

  QByteArray bmp;
  QDataStream ds(&bmp, QIODevice::WriteOnly);
  ds.setByteOrder(QDataStream::LittleEndian);
  ds.writeRawData("BM", kBmpSignatureSize);
  ds << fileSize;
  ds << quint32(0);
  ds << pixelOffset;
  ds.writeRawData(dib.constData(), static_cast<int>(dib.size()));
  return bmp;
}

QByteArray bmpToDib(const QByteArray &bmp)
{
  if (bmp.size() < static_cast<qint64>(kBmpFileHeaderSize)) {
    return {};
  }
  return bmp.mid(kBmpFileHeaderSize);
}

QByteArray encodeBitmapToPng(const std::string &dibData)
{
  if (dibData.empty()) {
    return {};
  }
  const auto bmp = dibToBmp(QByteArray::fromRawData(dibData.data(), static_cast<int>(dibData.size())));
  if (bmp.isEmpty()) {
    return {};
  }
  QImage img;
  if (!img.loadFromData(bmp, "BMP")) {
    return {};
  }
  QByteArray png;
  QBuffer buf(&png);
  buf.open(QIODevice::WriteOnly);
  if (!img.save(&buf, "PNG")) {
    return {};
  }
  return png;
}

std::string decodePngToDib(const QByteArray &pngBytes)
{
  if (pngBytes.isEmpty()) {
    return {};
  }
  QImage img;
  if (!img.loadFromData(pngBytes, "PNG")) {
    return {};
  }
  QByteArray bmp;
  QBuffer buf(&bmp);
  buf.open(QIODevice::WriteOnly);
  if (!img.save(&buf, "BMP")) {
    return {};
  }
  const auto dib = bmpToDib(bmp);
  return std::string(dib.constData(), dib.size());
}

bool writeAll(int fd, const char *data, size_t len)
{
  size_t written = 0;
  while (written < len) {
    struct pollfd pfd = {fd, POLLOUT, 0};
    int r = poll(&pfd, 1, kIoTimeoutMs);
    if (r <= 0) {
      return false;
    }
    ssize_t n = write(fd, data + written, len - written);
    if (n <= 0) {
      if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        continue;
      }
      return false;
    }
    written += static_cast<size_t>(n);
  }
  return true;
}

std::string readAll(int fd, size_t maxBytes)
{
  std::string result;
  result.reserve(std::min<size_t>(maxBytes, kChunkSize));
  char buffer[kChunkSize];

  while (result.size() < maxBytes) {
    struct pollfd pfd = {fd, POLLIN, 0};
    int r = poll(&pfd, 1, kIoTimeoutMs);
    if (r <= 0) {
      break;
    }
    size_t toRead = std::min<size_t>(sizeof(buffer), maxBytes - result.size());
    ssize_t n = read(fd, buffer, toRead);
    if (n <= 0) {
      if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        continue;
      }
      break;
    }
    result.append(buffer, static_cast<size_t>(n));
  }
  return result;
}

void normalizeText(std::string &text)
{
  while (!text.empty() && text.back() == '\0') {
    text.pop_back();
  }
  // Replace CRLF with LF
  size_t pos = 0;
  while ((pos = text.find("\r\n", pos)) != std::string::npos) {
    text.replace(pos, 2, "\n");
    pos += 1;
  }
}

} // namespace

WlrClipboard::WlrClipboard(GrabbedCallback onGrabbed, size_t maxClipboardBytes)
    : m_onGrabbed(std::move(onGrabbed)), m_maxClipboardBytes(maxClipboardBytes)
{
  m_cache = new EiClipboard(kClipboardClipboard);

  const char *waylandDisplay = std::getenv("WAYLAND_DISPLAY");
  if (!waylandDisplay) {
    LOG_DEBUG("WAYLAND_DISPLAY not set; Wayland data-control clipboard disabled");
    m_initialized.store(true);
    return;
  }

  if (pipe2(m_wakeupPipe, O_CLOEXEC | O_NONBLOCK) != 0) {
    LOG_ERR("failed to create wakeup pipe: %s", strerror(errno));
    m_initialized.store(true);
    return;
  }

  m_thread = std::thread(&WlrClipboard::threadFunc, this);

  // Synchronously wait up to 5 seconds for Wayland init to complete
  {
    std::unique_lock<std::mutex> lock(m_initMutex);
    m_initCv.wait_for(lock, std::chrono::seconds(5), [this]() {
      return m_initialized.load();
    });
  }
}

WlrClipboard::~WlrClipboard()
{
  m_stopping.store(true);

  if (m_wakeupPipe[1] >= 0) {
    char b = 1;
    (void)write(m_wakeupPipe[1], &b, 1);
  }

  if (m_thread.joinable()) {
    m_thread.join();
  }

  if (m_wakeupPipe[0] >= 0) {
    close(m_wakeupPipe[0]);
  }
  if (m_wakeupPipe[1] >= 0) {
    close(m_wakeupPipe[1]);
  }

  delete m_cache;
}

bool WlrClipboard::getClipboard(IClipboard *target) const
{
  if (!target) {
    return false;
  }
  std::scoped_lock lock{m_mutex};
  return IClipboard::copy(target, m_cache);
}

bool WlrClipboard::setSelection(const IClipboard *clipboard)
{
  if (!clipboard) {
    return false;
  }

  OutgoingData data;
  data.valid = true;

  if (clipboard->open(0)) {
    if (clipboard->has(IClipboard::Format::Text)) {
      data.text = clipboard->get(IClipboard::Format::Text);
    }
    if (clipboard->has(IClipboard::Format::HTML)) {
      data.html = clipboard->get(IClipboard::Format::HTML);
    }
    if (clipboard->has(IClipboard::Format::Bitmap)) {
      data.bitmap = clipboard->get(IClipboard::Format::Bitmap);
    }
    clipboard->close();
  }

  {
    std::scoped_lock lock{m_mutex};
    // Update local cache
    IClipboard::copy(m_cache, clipboard);

    m_pendingOutgoing = std::move(data);
    m_hasPendingOutgoing = true;
  }

  // Wake up Wayland background loop to announce selection
  if (m_wakeupPipe[1] >= 0) {
    char b = 1;
    (void)write(m_wakeupPipe[1], &b, 1);
  }

  return true;
}

void WlrClipboard::initWayland()
{
  auto notifyInit = [this]() {
    {
      std::lock_guard<std::mutex> lock(m_initMutex);
      m_initialized.store(true);
    }
    m_initCv.notify_all();
  };

  m_display = wl_display_connect(nullptr);
  if (!m_display) {
    LOG_DEBUG("WlrClipboard: could not connect to Wayland display");
    notifyInit();
    return;
  }

  m_registry = wl_display_get_registry(m_display);
  static const struct wl_registry_listener regListener = {
      .global = &WlrClipboard::registryGlobal,
      .global_remove = &WlrClipboard::registryGlobalRemove,
  };
  wl_registry_add_listener(m_registry, &regListener, this);

  wl_display_roundtrip(m_display);

  if (!m_manager || !m_seat) {
    LOG_DEBUG("WlrClipboard: zwlr_data_control_manager_v1 or wl_seat not available on compositor");
    cleanupWayland();
    notifyInit();
    return;
  }

  m_device = zwlr_data_control_manager_v1_get_data_device(m_manager, m_seat);
  static const struct zwlr_data_control_device_v1_listener devListener = {
      .data_offer = &WlrClipboard::deviceDataOffer,
      .selection = &WlrClipboard::deviceSelection,
      .finished = &WlrClipboard::deviceFinished,
      .primary_selection = &WlrClipboard::devicePrimarySelection,
  };
  zwlr_data_control_device_v1_add_listener(m_device, &devListener, this);

  wl_display_roundtrip(m_display);

  m_available.store(true);
  LOG_INFO("WlrClipboard: initialized Wayland data-control clipboard successfully");
  notifyInit();
}

void WlrClipboard::cleanupWayland()
{
  m_available.store(false);

  for (auto &offerInfo : m_offers) {
    if (offerInfo.offer) {
      zwlr_data_control_offer_v1_destroy(offerInfo.offer);
    }
  }
  m_offers.clear();

  if (m_activeSource) {
    zwlr_data_control_source_v1_destroy(m_activeSource);
    m_activeSource = nullptr;
  }
  if (m_device) {
    zwlr_data_control_device_v1_destroy(m_device);
    m_device = nullptr;
  }
  if (m_manager) {
    zwlr_data_control_manager_v1_destroy(m_manager);
    m_manager = nullptr;
  }
  if (m_seat) {
    wl_seat_destroy(m_seat);
    m_seat = nullptr;
  }
  if (m_registry) {
    wl_registry_destroy(m_registry);
    m_registry = nullptr;
  }
  if (m_display) {
    wl_display_disconnect(m_display);
    m_display = nullptr;
  }
}

void WlrClipboard::processOutgoingSelection()
{
  if (!m_available.load() || !m_device || !m_manager) {
    return;
  }

  OutgoingData outgoing;
  {
    std::scoped_lock lock{m_mutex};
    if (!m_hasPendingOutgoing) {
      return;
    }
    outgoing = std::move(m_pendingOutgoing);
    m_hasPendingOutgoing = false;
  }

  if (m_activeSource) {
    zwlr_data_control_source_v1_destroy(m_activeSource);
    m_activeSource = nullptr;
  }

  if (!outgoing.valid || (outgoing.text.empty() && outgoing.html.empty() && outgoing.bitmap.empty())) {
    // Clear selection
    m_isOwnSelection = false;
    zwlr_data_control_device_v1_set_selection(m_device, nullptr);
    wl_display_flush(m_display);
    return;
  }

  m_activeOutgoing = std::move(outgoing);
  m_isOwnSelection = true;
  m_ownSelectionEchosRemaining = 2;

  m_activeSource = zwlr_data_control_manager_v1_create_data_source(m_manager);
  static const struct zwlr_data_control_source_v1_listener srcListener = {
      .send = &WlrClipboard::sourceSend,
      .cancelled = &WlrClipboard::sourceCancelled,
  };
  zwlr_data_control_source_v1_add_listener(m_activeSource, &srcListener, this);

  if (!m_activeOutgoing.text.empty()) {
    zwlr_data_control_source_v1_offer(m_activeSource, "text/plain;charset=utf-8");
    zwlr_data_control_source_v1_offer(m_activeSource, "text/plain");
    zwlr_data_control_source_v1_offer(m_activeSource, "UTF8_STRING");
  }
  if (!m_activeOutgoing.html.empty()) {
    zwlr_data_control_source_v1_offer(m_activeSource, "text/html;charset=utf-8");
    zwlr_data_control_source_v1_offer(m_activeSource, "text/html");
  }
  if (!m_activeOutgoing.bitmap.empty()) {
    zwlr_data_control_source_v1_offer(m_activeSource, "image/png");
    zwlr_data_control_source_v1_offer(m_activeSource, "image/bmp");
  }

  zwlr_data_control_device_v1_set_selection(m_device, m_activeSource);
  wl_display_flush(m_display);
  LOG_DEBUG("WlrClipboard: set selection to Wayland compositor");
}

void WlrClipboard::threadFunc()
{
  initWayland();

  if (!m_available.load()) {
    return;
  }

  const int waylandFd = wl_display_get_fd(m_display);

  while (!m_stopping.load()) {
    while (wl_display_prepare_read(m_display) != 0) {
      wl_display_dispatch_pending(m_display);
    }
    wl_display_flush(m_display);

    struct pollfd pfds[2];
    pfds[0].fd = waylandFd;
    pfds[0].events = POLLIN;
    pfds[0].revents = 0;

    pfds[1].fd = m_wakeupPipe[0];
    pfds[1].events = POLLIN;
    pfds[1].revents = 0;

    int ret = poll(pfds, 2, -1);

    if (ret > 0) {
      if (pfds[0].revents & POLLIN) {
        wl_display_read_events(m_display);
      } else {
        wl_display_cancel_read(m_display);
      }

      if (pfds[1].revents & POLLIN) {
        char dummy[128];
        while (read(m_wakeupPipe[0], dummy, sizeof(dummy)) > 0) {}
      }
    } else {
      wl_display_cancel_read(m_display);
    }

    wl_display_dispatch_pending(m_display);

    processOutgoingSelection();
  }

  cleanupWayland();
}

void WlrClipboard::registryGlobal(
    void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version
)
{
  auto self = static_cast<WlrClipboard *>(data);
  if (std::strcmp(interface, zwlr_data_control_manager_v1_interface.name) == 0) {
    uint32_t bindVer = std::min<uint32_t>(version, 2);
    self->m_manager = static_cast<zwlr_data_control_manager_v1 *>(
        wl_registry_bind(registry, id, &zwlr_data_control_manager_v1_interface, bindVer)
    );
  } else if (std::strcmp(interface, "wl_seat") == 0 && !self->m_seat) {
    self->m_seat = static_cast<wl_seat *>(wl_registry_bind(registry, id, &wl_seat_interface, 1));
  }
}

void WlrClipboard::registryGlobalRemove(void *, struct wl_registry *, uint32_t)
{
}

void WlrClipboard::deviceDataOffer(
    void *data, struct zwlr_data_control_device_v1 *, struct zwlr_data_control_offer_v1 *offer
)
{
  auto self = static_cast<WlrClipboard *>(data);
  ActiveOfferInfo info;
  info.offer = offer;
  self->m_offers.push_back(std::move(info));

  static const struct zwlr_data_control_offer_v1_listener offerListener = {
      .offer = &WlrClipboard::offerOffer,
  };
  zwlr_data_control_offer_v1_add_listener(offer, &offerListener, self);
}

void WlrClipboard::offerOffer(void *data, struct zwlr_data_control_offer_v1 *offer, const char *mime_type)
{
  auto self = static_cast<WlrClipboard *>(data);
  for (auto &offerInfo : self->m_offers) {
    if (offerInfo.offer == offer) {
      offerInfo.mimes.emplace_back(mime_type);
      break;
    }
  }
}

void WlrClipboard::handleSelectionOffer(
    struct zwlr_data_control_offer_v1 *offer, const std::vector<std::string> &mimes
)
{
  // Do not re-import if we own the selection (prevent echo loops)
  if (m_isOwnSelection) {
    LOG_INFO("WlrClipboard: selection offer is our own, ignoring echo");
    m_isOwnSelection = false;
    return;
  }

  auto hasMime = [&mimes](const std::string &m) {
    return std::find(mimes.begin(), mimes.end(), m) != mimes.end();
  };

  std::string chosenTextMime;
  if (hasMime("text/plain;charset=utf-8")) {
    chosenTextMime = "text/plain;charset=utf-8";
  } else if (hasMime("UTF8_STRING")) {
    chosenTextMime = "UTF8_STRING";
  } else if (hasMime("text/plain")) {
    chosenTextMime = "text/plain";
  } else if (hasMime("STRING")) {
    chosenTextMime = "STRING";
  } else if (hasMime("TEXT")) {
    chosenTextMime = "TEXT";
  }

  std::string chosenHtmlMime;
  if (hasMime("text/html;charset=utf-8")) {
    chosenHtmlMime = "text/html;charset=utf-8";
  } else if (hasMime("text/html")) {
    chosenHtmlMime = "text/html";
  }

  std::string chosenBitmapMime;
  if (hasMime("image/png")) {
    chosenBitmapMime = "image/png";
  }

  std::string textData;
  std::string htmlData;
  std::string dibData;
  bool gotData = false;

  if (!chosenTextMime.empty()) {
    int fds[2];
    if (pipe2(fds, O_CLOEXEC | O_NONBLOCK) == 0) {
      zwlr_data_control_offer_v1_receive(offer, chosenTextMime.c_str(), fds[1]);
      close(fds[1]);
      wl_display_flush(m_display);
      textData = readAll(fds[0], m_maxClipboardBytes);
      close(fds[0]);
      if (!textData.empty()) {
        normalizeText(textData);
        gotData = true;
      }
    }
  }

  if (!chosenHtmlMime.empty()) {
    int fds[2];
    if (pipe2(fds, O_CLOEXEC | O_NONBLOCK) == 0) {
      zwlr_data_control_offer_v1_receive(offer, chosenHtmlMime.c_str(), fds[1]);
      close(fds[1]);
      wl_display_flush(m_display);
      htmlData = readAll(fds[0], m_maxClipboardBytes);
      close(fds[0]);
      if (!htmlData.empty()) {
        normalizeText(htmlData);
        gotData = true;
      }
    }
  }

  if (!chosenBitmapMime.empty()) {
    int fds[2];
    if (pipe2(fds, O_CLOEXEC | O_NONBLOCK) == 0) {
      zwlr_data_control_offer_v1_receive(offer, chosenBitmapMime.c_str(), fds[1]);
      close(fds[1]);
      wl_display_flush(m_display);
      std::string pngData = readAll(fds[0], m_maxClipboardBytes);
      close(fds[0]);
      if (!pngData.empty()) {
        dibData = decodePngToDib(QByteArray::fromRawData(pngData.data(), static_cast<int>(pngData.size())));
        if (!dibData.empty()) {
          gotData = true;
        }
      }
    }
  }

  if (gotData) {
    if (!textData.empty() && textData == m_activeOutgoing.text) {
      LOG_INFO("WlrClipboard: imported text matches active outgoing text, ignoring echo");
      return;
    }

    if (m_activeSource) {
      zwlr_data_control_source_v1_destroy(m_activeSource);
      m_activeSource = nullptr;
    }
    m_activeOutgoing.text.clear();
    m_activeOutgoing.html.clear();
    m_activeOutgoing.bitmap.clear();
    m_ownSelectionEchosRemaining = 0;
    m_isOwnSelection = false;

    uint32_t timestamp = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );
    if (timestamp == 0) {
      timestamp = 1;
    }

    {
      std::scoped_lock lock{m_mutex};
      m_cache->open(timestamp);
      m_cache->empty();
      if (!textData.empty()) {
        m_cache->add(IClipboard::Format::Text, textData);
      }
      if (!htmlData.empty()) {
        m_cache->add(IClipboard::Format::HTML, htmlData);
      }
      if (!dibData.empty()) {
        m_cache->add(IClipboard::Format::Bitmap, dibData);
      }
      m_cache->close();
    }

    LOG_INFO(
        "WlrClipboard: imported selection from local Wayland client (text=%zu bytes, html=%zu bytes, dib=%zu bytes)",
        textData.size(), htmlData.size(), dibData.size()
    );

    if (m_onGrabbed) {
      m_onGrabbed();
    }
  }
}

void WlrClipboard::deviceSelection(
    void *data, struct zwlr_data_control_device_v1 *, struct zwlr_data_control_offer_v1 *offer
)
{
  auto self = static_cast<WlrClipboard *>(data);

  if (!offer) {
    // Selection was cleared, do not emit spurious grab events
    return;
  }

  // If our own source echo counter is active, consume it and ignore
  if (self->m_ownSelectionEchosRemaining > 0) {
    --self->m_ownSelectionEchosRemaining;
    LOG_INFO("WlrClipboard: selection offer is our own echo (remaining=%d), ignoring", self->m_ownSelectionEchosRemaining);
    return;
  }

  std::vector<std::string> mimes;
  for (const auto &info : self->m_offers) {
    if (info.offer == offer) {
      mimes = info.mimes;
      break;
    }
  }

  self->handleSelectionOffer(offer, mimes);

  // Clean up completed offers other than the active one
  std::erase_if(self->m_offers, [offer](const ActiveOfferInfo &info) {
    if (info.offer != offer) {
      if (info.offer) {
        zwlr_data_control_offer_v1_destroy(info.offer);
      }
      return true;
    }
    return false;
  });
}

void WlrClipboard::devicePrimarySelection(
    void *, struct zwlr_data_control_device_v1 *, struct zwlr_data_control_offer_v1 *
)
{
  // Primary selection ignored (standard clipboard used)
}

void WlrClipboard::deviceFinished(void *data, struct zwlr_data_control_device_v1 *)
{
  LOG_WARN("WlrClipboard: zwlr_data_control_device_v1 finished by compositor");
  auto self = static_cast<WlrClipboard *>(data);
  self->cleanupWayland();
}

void WlrClipboard::sourceSend(
    void *data, struct zwlr_data_control_source_v1 *, const char *mime_type, int32_t fd
)
{
  auto self = static_cast<WlrClipboard *>(data);
  const std::string mime(mime_type ? mime_type : "");

  LOG_DEBUG("WlrClipboard: source requested send for MIME: %s", mime.c_str());

  if (mime == "text/plain;charset=utf-8" || mime == "text/plain" || mime == "UTF8_STRING") {
    writeAll(fd, self->m_activeOutgoing.text.data(), self->m_activeOutgoing.text.size());
  } else if (mime == "text/html;charset=utf-8" || mime == "text/html") {
    writeAll(fd, self->m_activeOutgoing.html.data(), self->m_activeOutgoing.html.size());
  } else if (mime == "image/png" && !self->m_activeOutgoing.bitmap.empty()) {
    const auto png = encodeBitmapToPng(self->m_activeOutgoing.bitmap);
    if (!png.isEmpty()) {
      writeAll(fd, png.constData(), static_cast<size_t>(png.size()));
    }
  }

  close(fd);
}

void WlrClipboard::sourceCancelled(void *data, struct zwlr_data_control_source_v1 *source)
{
  auto self = static_cast<WlrClipboard *>(data);
  LOG_INFO("WlrClipboard: our selection source was cancelled (another client copied)");

  self->m_isOwnSelection = false;
  self->m_ownSelectionEchosRemaining = 0;
  if (self->m_activeSource == source) {
    zwlr_data_control_source_v1_destroy(self->m_activeSource);
    self->m_activeSource = nullptr;
  }
  self->m_activeOutgoing.text.clear();
  self->m_activeOutgoing.html.clear();
  self->m_activeOutgoing.bitmap.clear();
}

} // namespace deskflow
