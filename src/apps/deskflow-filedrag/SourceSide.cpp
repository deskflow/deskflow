/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

// Source role (the server for server to client drags, a client for client to server
// drags): notice a file drag at a switch edge, park it while the cursor is on the other
// computer, and stream the files.

#include "FileDragHelper.h"

#include "deskflow/FileDragCodec.h"
#include "deskflow/FileDragPolicy.h"
#include "deskflow/FileDragSender.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

#include <ole2.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shobjidl.h>

namespace fs = std::filesystem;

namespace deskflow::filedrag::helper {

namespace {

//! The sides in the order of g.strips; WatchEdges carries a DirectionMask (see FileDragPolicy.h)
constexpr Direction kStripSides[4] = {Direction::Left, Direction::Right, Direction::Top, Direction::Bottom};

//! Side of the window that parks the local drag
constexpr int kCatcherSide = 160;

//! What is being dragged, ready to send
struct Offer
{
  std::vector<std::wstring> paths;
  ItemList items;
  std::vector<fs::path> sources;
  Icon icon;
  bool ready = false;
};

//! Streams the files of one drag on its own thread
class SourceSession
{
public:
  SourceSession(uint32_t dragId, const Offer &offer)
      : m_dragId(dragId),
        m_sender(dragId, offer.items, offer.sources, [](Message &&message) { sendPeer(message); })
  {
  }

  ~SourceSession()
  {
    stop();
    if (m_thread.joinable()) {
      m_thread.join();
    }
  }

  void start()
  {
    m_thread = std::thread([this] { run(); });
  }

  void onAck(uint32_t seq)
  {
    {
      std::lock_guard lock(m_mutex);
      m_acks.push_back(seq);
    }
    m_cond.notify_one();
  }

  void stop()
  {
    {
      std::lock_guard lock(m_mutex);
      m_stopping = true;
    }
    m_cond.notify_one();
  }

  bool isDone() const
  {
    return m_done;
  }

  uint32_t dragId() const
  {
    return m_dragId;
  }

private:
  void run()
  {
    m_sender.pump();
    while (!m_sender.hasFailed() && !m_sender.isFinished()) {
      std::deque<uint32_t> acks;
      {
        std::unique_lock lock(m_mutex);
        m_cond.wait_for(lock, std::chrono::milliseconds(100), [this] { return m_stopping || !m_acks.empty(); });
        if (m_stopping) {
          break;
        }
        acks.swap(m_acks);
      }
      for (uint32_t seq : acks) {
        m_sender.onAck(seq);
      }
      m_sender.pump();
    }
    if (m_sender.hasFailed()) {
      log(LogLevel::Warning, "file drag %u: sending failed: %s", m_dragId, cancelReasonName(m_sender.failReason()));
      sendPeer(Message::cancel(m_dragId, m_sender.failReason()));
    } else if (m_sender.isFinished()) {
      log(LogLevel::Info, "file drag %u: all %llu bytes sent", m_dragId,
          static_cast<unsigned long long>(m_sender.bytesSent()));
      testEvent("source-sent " + std::to_string(m_dragId) + " " + std::to_string(m_sender.bytesSent()));
    } else {
      log(LogLevel::Debug, "file drag %u: sending stopped", m_dragId);
    }
    m_done = true;
  }

  uint32_t m_dragId;
  FileDragSender m_sender;
  std::thread m_thread;
  std::mutex m_mutex;
  std::condition_variable m_cond;
  std::deque<uint32_t> m_acks;
  bool m_stopping = false;
  std::atomic<bool> m_done{false};
};

class EdgeDropTarget;

struct SourceState
{
  HWND strips[4] = {};
  EdgeDropTarget *stripTargets[4] = {};
  HWND catcher = nullptr;
  EdgeDropTarget *catcherTarget = nullptr;

  bool overStrip = false;
  bool offered = false;
  bool parked = false; ///< the local drag waits on the catcher
  uint64_t scanGeneration = 0;
  std::shared_ptr<Offer> offer;
  uint32_t currentDragId = 0;

  // test harness: the shell data object that "enters" the strip
  IDataObject *testData = nullptr;
  EdgeDropTarget *testTarget = nullptr;
};

SourceState g;

// sessions are reached from the pipe reader (acks) and the UI thread
std::mutex g_sessionsMutex;
std::map<uint32_t, std::unique_ptr<SourceSession>> g_sessions;

std::vector<std::wstring> pathsFromDataObject(IDataObject *data)
{
  std::vector<std::wstring> paths;
  FORMATETC format = {CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
  STGMEDIUM medium = {};
  if (data == nullptr || FAILED(data->GetData(&format, &medium))) {
    return paths;
  }
  auto drop = static_cast<HDROP>(GlobalLock(medium.hGlobal));
  if (drop != nullptr) {
    const UINT count = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);
    for (UINT i = 0; i < count; ++i) {
      const UINT length = DragQueryFileW(drop, i, nullptr, 0);
      std::wstring path(length, L'\0');
      DragQueryFileW(drop, i, path.data(), length + 1);
      if (!path.empty()) {
        paths.push_back(std::move(path));
      }
    }
    GlobalUnlock(medium.hGlobal);
  }
  ReleaseStgMedium(&medium);
  return paths;
}

//! Drag image of the first item, RGBA with straight alpha
Icon iconFor(const std::wstring &path)
{
  Icon icon;
  IShellItemImageFactory *factory = nullptr;
  if (FAILED(SHCreateItemFromParsingName(path.c_str(), nullptr, IID_PPV_ARGS(&factory)))) {
    return icon;
  }
  HBITMAP bitmap = nullptr;
  const SIZE size = {96, 96};
  if (SUCCEEDED(factory->GetImage(size, SIIGBF_RESIZETOFIT | SIIGBF_BIGGERSIZEOK, &bitmap)) && bitmap != nullptr) {
    BITMAP info = {};
    GetObjectW(bitmap, sizeof(info), &info);
    const int width = info.bmWidth;
    const int height = info.bmHeight < 0 ? -info.bmHeight : info.bmHeight;
    if (width > 0 && height > 0 && width <= kMaxIconSide && height <= kMaxIconSide) {
      BITMAPINFO bi = {};
      bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
      bi.bmiHeader.biWidth = width;
      bi.bmiHeader.biHeight = -height; // top row first
      bi.bmiHeader.biPlanes = 1;
      bi.bmiHeader.biBitCount = 32;
      bi.bmiHeader.biCompression = BI_RGB;
      std::string bgra(static_cast<size_t>(width) * height * 4, '\0');
      HDC dc = GetDC(nullptr);
      const int lines = GetDIBits(dc, bitmap, 0, height, bgra.data(), &bi, DIB_RGB_COLORS);
      ReleaseDC(nullptr, dc);
      if (lines == height) {
        icon.width = static_cast<uint16_t>(width);
        icon.height = static_cast<uint16_t>(height);
        icon.rgba.resize(bgra.size());
        for (size_t i = 0; i < bgra.size(); i += 4) {
          const auto b = static_cast<uint8_t>(bgra[i]);
          const auto gr = static_cast<uint8_t>(bgra[i + 1]);
          const auto r = static_cast<uint8_t>(bgra[i + 2]);
          const auto a = static_cast<uint8_t>(bgra[i + 3]);
          // the shell hands out premultiplied pixels; the wire carries straight alpha
          auto straight = [a](uint8_t c) -> char {
            if (a == 0) {
              return 0;
            }
            const unsigned v = (static_cast<unsigned>(c) * 255u + a / 2) / a;
            return static_cast<char>(v > 255 ? 255 : v);
          };
          icon.rgba[i] = straight(r);
          icon.rgba[i + 1] = straight(gr);
          icon.rgba[i + 2] = straight(b);
          icon.rgba[i + 3] = static_cast<char>(a);
        }
      }
    }
    DeleteObject(bitmap);
  }
  factory->Release();
  return icon;
}

void sendSimple(pipe::Frame frame)
{
  sendFrame(frame);
}

void offerReady()
{
  if (!g.offered) {
    g.offered = true;
    log(LogLevel::Info, "file drag offer ready (%zu items)", g.offer ? g.offer->items.size() : size_t{0});
    testEvent("offer-ready");
    sendSimple(pipe::Frame::OfferReady);
  }
}

void offerLost()
{
  if (g.offered) {
    g.offered = false;
    testEvent("offer-lost");
    sendSimple(pipe::Frame::OfferLost);
  }
}

//! A drag of files entered an edge window: build its item list (on a worker thread)
void onFilesEntered(std::vector<std::wstring> paths)
{
  g.overStrip = true;
  if (g.offer && g.offer->paths == paths) {
    if (g.offer->ready) {
      offerReady();
    }
    return; // still scanning
  }

  auto offer = std::make_shared<Offer>();
  offer->paths = paths;
  g.offer = offer;
  const uint64_t generation = ++g.scanGeneration;
  std::thread([offer, generation] {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    std::vector<fs::path> roots(offer->paths.begin(), offer->paths.end());
    ItemList items;
    std::vector<fs::path> sources;
    std::string error;
    const bool ok = scanItems(roots, items, sources, &error);
    Icon icon = ok && !offer->paths.empty() ? iconFor(offer->paths.front()) : Icon{};
    CoUninitialize();
    postToUi([offer, generation, ok, error, items = std::move(items), sources = std::move(sources),
              icon = std::move(icon)]() mutable {
      if (generation != g.scanGeneration) {
        return;
      }
      if (!ok) {
        log(LogLevel::Warning, "cannot carry these files to another computer: %s", error.c_str());
        g.offer.reset();
        return;
      }
      offer->items = std::move(items);
      offer->sources = std::move(sources);
      offer->icon = std::move(icon);
      offer->ready = true;
      if (g.overStrip) {
        offerReady();
      }
    });
  }).detach();
}

//! IDropTarget of an edge window or of the catcher
class EdgeDropTarget : public IDropTarget
{
public:
  explicit EdgeDropTarget(bool catcher) : m_catcher(catcher)
  {
  }

  // IUnknown
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) override
  {
    if (riid == IID_IUnknown || riid == IID_IDropTarget) {
      *ppv = static_cast<IDropTarget *>(this);
      AddRef();
      return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
  }
  ULONG STDMETHODCALLTYPE AddRef() override
  {
    return InterlockedIncrement(&m_refs);
  }
  ULONG STDMETHODCALLTYPE Release() override
  {
    const LONG refs = InterlockedDecrement(&m_refs);
    if (refs == 0) {
      delete this;
    }
    return refs;
  }

  // IDropTarget
  HRESULT STDMETHODCALLTYPE DragEnter(IDataObject *data, DWORD, POINTL, DWORD *effect) override
  {
    if (m_catcher) {
      // the parked local drag: it must never drop anything here
      *effect = DROPEFFECT_NONE;
      return S_OK;
    }
    auto paths = pathsFromDataObject(data);
    m_files = !paths.empty();
    if (m_files) {
      log(LogLevel::Info, "a drag of %zu files or folders reached the edge window", paths.size());
      onFilesEntered(std::move(paths));
    } else {
      // text, pictures, links...: only files may cross, so this drag stays here
      log(LogLevel::Info, "a drag without files reached the edge window; it stays on this computer");
    }
    *effect = m_files ? (*effect & DROPEFFECT_COPY) : DROPEFFECT_NONE;
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE DragOver(DWORD, POINTL, DWORD *effect) override
  {
    *effect = (!m_catcher && m_files) ? (*effect & DROPEFFECT_COPY) : DROPEFFECT_NONE;
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE DragLeave() override
  {
    if (m_catcher) {
      // the local drag ended (or the cursor came back): nothing is parked any more
      unpark("left the catcher");
      return S_OK;
    }
    if (m_files) {
      g.overStrip = false;
      offerLost();
    }
    m_files = false;
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE Drop(IDataObject *, DWORD, POINTL, DWORD *effect) override
  {
    // the source must never move or delete anything because of us
    *effect = DROPEFFECT_NONE;
    if (m_catcher) {
      unpark("let go");
      return S_OK;
    }
    if (m_files) {
      g.overStrip = false;
      offerLost();
    }
    m_files = false;
    return S_OK;
  }

  static void unpark(const char *why)
  {
    if (g.parked) {
      log(LogLevel::Debug, "local drag released from the catcher: %s", why);
      testEvent("catcher-released");
    }
    g.parked = false;
    if (g.catcher != nullptr) {
      ShowWindow(g.catcher, SW_HIDE);
    }
  }

private:
  LONG m_refs = 1;
  bool m_catcher;
  bool m_files = false;
};

LRESULT CALLBACK overlayProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg) {
  case WM_MOUSEACTIVATE:
    return MA_NOACTIVATE;
  case WM_TIMER:
    if (hwnd == g.catcher && wParam == 2) {
      // keep the catcher under the parked cursor, in our own coordinates, so the local
      // drag can only ever end on it
      if (!g.parked) {
        KillTimer(hwnd, 2);
        return 0;
      }
      SetTimer(hwnd, 2, 100, nullptr);
      POINT pt;
      RECT r;
      if (GetCursorPos(&pt) && GetWindowRect(hwnd, &r) && !PtInRect(&r, pt)) {
        SetWindowPos(
            hwnd, HWND_TOPMOST, pt.x - kCatcherSide / 2, pt.y - kCatcherSide / 2, kCatcherSide, kCatcherSide,
            SWP_NOACTIVATE | SWP_SHOWWINDOW
        );
      }
      return 0;
    }
    if (hwnd == g.catcher && wParam == 1) {
      // nothing can be parked once the button is up
      if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) == 0 && g.parked) {
        EdgeDropTarget::unpark("button is up");
      }
      if (!g.parked) {
        KillTimer(hwnd, 1);
      }
      return 0;
    }
    break;
  default:
    break;
  }
  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

HWND makeTargetWindow(bool catcher, EdgeDropTarget *&target)
{
  target = new EdgeDropTarget(catcher);
  HWND hwnd = createOverlayWindow(catcher ? L"DeskflowFileDragCatcher" : L"DeskflowFileDragEdge", overlayProc, target);
  if (hwnd == nullptr) {
    log(LogLevel::Error, "cannot create the file drag window: %s", pipe::errorText(GetLastError()).c_str());
    target->Release();
    target = nullptr;
    return nullptr;
  }
  const HRESULT hr = RegisterDragDrop(hwnd, target);
  if (FAILED(hr)) {
    log(LogLevel::Error, "RegisterDragDrop failed: 0x%08lx", static_cast<unsigned long>(hr));
  }
  return hwnd;
}

void stopSession(uint32_t dragId)
{
  std::unique_ptr<SourceSession> session;
  {
    std::lock_guard lock(g_sessionsMutex);
    auto it = g_sessions.find(dragId);
    if (it == g_sessions.end()) {
      return;
    }
    session = std::move(it->second);
    g_sessions.erase(it);
  }
  // joins the sender thread outside the lock
  session.reset();
}

void pruneSessions()
{
  std::vector<std::unique_ptr<SourceSession>> done;
  {
    std::lock_guard lock(g_sessionsMutex);
    for (auto it = g_sessions.begin(); it != g_sessions.end();) {
      if (it->second->isDone()) {
        done.push_back(std::move(it->second));
        it = g_sessions.erase(it);
      } else {
        ++it;
      }
    }
  }
}

} // namespace

void sourceWatchEdges(uint32_t sides, const RECT &screen)
{
  if (sides != 0) {
    log(LogLevel::Info, "watching sides 0x%x of the screen %ld,%ld %ldx%ld for a drag of files", sides, screen.left,
        screen.top, screen.right - screen.left, screen.bottom - screen.top);
  }
  const EdgeRect area{
      static_cast<int32_t>(screen.left), static_cast<int32_t>(screen.top),
      static_cast<int32_t>(screen.right - screen.left), static_cast<int32_t>(screen.bottom - screen.top)
  };
  for (int i = 0; i < 4; ++i) {
    const auto strip = edgeStrip(kStripSides[i], sides, area);
    if (!strip) {
      if (g.strips[i] != nullptr) {
        ShowWindow(g.strips[i], SW_HIDE);
      }
      continue;
    }
    if (g.strips[i] == nullptr) {
      g.strips[i] = makeTargetWindow(false, g.stripTargets[i]);
      if (g.strips[i] == nullptr) {
        continue;
      }
    }
    SetWindowPos(g.strips[i], HWND_TOPMOST, strip->x, strip->y, strip->w, strip->h, SWP_NOACTIVATE | SWP_SHOWWINDOW);
  }
}

void sourceHandOff(uint32_t dragId, POINT parked)
{
  pruneSessions();
  if (!g.offer || !g.offer->ready) {
    log(LogLevel::Warning, "file drag %u: nothing to send", dragId);
    sendPeer(Message::cancel(dragId, CancelReason::Unsupported));
    return;
  }

  // park the local drag on the catcher before the cursor is moved there
  if (g.catcher == nullptr) {
    g.catcher = makeTargetWindow(true, g.catcherTarget);
  }
  if (g.catcher != nullptr) {
    SetWindowPos(
        g.catcher, HWND_TOPMOST, parked.x - kCatcherSide / 2, parked.y - kCatcherSide / 2, kCatcherSide, kCatcherSide,
        SWP_NOACTIVATE | SWP_SHOWWINDOW
    );
    SetTimer(g.catcher, 1, 1000, nullptr);
    // the core moves the cursor to the parked spot right after this hand-off
    SetTimer(g.catcher, 2, 300, nullptr);
  }
  g.parked = true;
  g.offered = false;
  g.overStrip = false;
  g.currentDragId = dragId;

  const Offer &offer = *g.offer;
  log(LogLevel::Info, "file drag %u: sending %zu items", dragId, offer.items.size());
  testEvent("source-handoff " + std::to_string(dragId));
  if (!offer.icon.isEmpty()) {
    Message icon;
    icon.type = Message::Type::Icon;
    icon.dragId = dragId;
    icon.icon = offer.icon;
    sendPeer(icon);
  }
  Message begin;
  begin.type = Message::Type::Begin;
  begin.dragId = dragId;
  begin.items = offer.items;
  sendPeer(begin);

  auto session = std::make_unique<SourceSession>(dragId, offer);
  SourceSession *raw = session.get();
  {
    std::lock_guard lock(g_sessionsMutex);
    g_sessions[dragId] = std::move(session);
  }
  raw->start();
  // the next drag scans afresh (the files may change in between)
  g.offer.reset();
}

void sourceTakeBack()
{
  if (g.currentDragId != 0) {
    log(LogLevel::Info, "file drag %u: cursor came back, stop sending", g.currentDragId);
    testEvent("source-takeback " + std::to_string(g.currentDragId));
    stopSession(g.currentDragId);
    g.currentDragId = 0;
  }
  EdgeDropTarget::unpark("taken back");
}

void sourceEntered()
{
  EdgeDropTarget::unpark("cursor is back");
}

void sourcePeer(const Message &message)
{
  switch (message.type) {
    using enum Message::Type;
  case Ack: {
    std::lock_guard lock(g_sessionsMutex);
    auto it = g_sessions.find(message.dragId);
    if (it != g_sessions.end()) {
      it->second->onAck(message.seq);
    }
    break;
  }
  case Cancel: {
    if (message.dragId == 0 && message.reason == CancelReason::ConnectionLost) {
      // the peer is gone: every drag sent from here stops
      std::vector<uint32_t> ids;
      {
        std::lock_guard lock(g_sessionsMutex);
        for (const auto &[id, session] : g_sessions) {
          ids.push_back(id);
        }
      }
      for (uint32_t id : ids) {
        log(LogLevel::Info, "file drag %u: stop sending, connection lost", id);
        testEvent("source-cancelled " + std::to_string(id) + " " + cancelReasonName(message.reason));
        stopSession(id);
      }
      break;
    }
    bool known = false;
    {
      std::lock_guard lock(g_sessionsMutex);
      known = g_sessions.count(message.dragId) != 0;
    }
    if (!known) {
      break; // a drag that came here (the destination role handles it)
    }
    log(LogLevel::Info, "file drag %u cancelled: %s", message.dragId, cancelReasonName(message.reason));
    testEvent("source-cancelled " + std::to_string(message.dragId) + " " + cancelReasonName(message.reason));
    stopSession(message.dragId);
    break;
  }
  case Dropped:
    log(LogLevel::Info, "file drag %u: dropped on the other computer, still sending", message.dragId);
    testEvent("source-dropped " + std::to_string(message.dragId));
    break;
  case Finished:
    log(LogLevel::Info, "file drag %u: the other computer has all the files", message.dragId);
    testEvent("source-finished " + std::to_string(message.dragId));
    stopSession(message.dragId);
    break;
  default:
    break;
  }
}

void sourceTestEnter(const std::vector<std::wstring> &paths)
{
  sourceTestLeave();
  std::vector<PIDLIST_ABSOLUTE> pidls;
  for (const auto &path : paths) {
    PIDLIST_ABSOLUTE pidl = nullptr;
    if (SUCCEEDED(SHParseDisplayName(path.c_str(), nullptr, &pidl, 0, nullptr))) {
      pidls.push_back(pidl);
    }
  }
  IShellItemArray *array = nullptr;
  if (!pidls.empty() && SUCCEEDED(SHCreateShellItemArrayFromIDLists(
                            static_cast<UINT>(pidls.size()), const_cast<PCIDLIST_ABSOLUTE_ARRAY>(pidls.data()), &array
                        ))) {
    array->BindToHandler(nullptr, BHID_DataObject, IID_PPV_ARGS(&g.testData));
    array->Release();
  }
  for (auto pidl : pidls) {
    CoTaskMemFree(pidl);
  }
  if (g.testData == nullptr) {
    testEvent("error no-data-object");
    return;
  }
  // the harness checks where this edge window really is (it must be the side it asked for)
  RECT stripRect = {0, 0, 0, 0};
  for (int i = 0; i < 4; ++i) {
    if (g.stripTargets[i] != nullptr && g.strips[i] != nullptr && IsWindowVisible(g.strips[i])) {
      g.testTarget = g.stripTargets[i];
      GetWindowRect(g.strips[i], &stripRect);
      break;
    }
  }
  if (g.testTarget == nullptr) {
    testEvent("error no-edge-window");
    g.testData->Release();
    g.testData = nullptr;
    return;
  }
  g.testTarget->AddRef();
  DWORD effect = DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK;
  POINTL pt = {0, 0};
  g.testTarget->DragEnter(g.testData, MK_LBUTTON, pt, &effect);
  testEvent(
      "source-enter effect=" + std::to_string(effect) + " strip=" + std::to_string(stripRect.left) + "," +
      std::to_string(stripRect.top) + "," + std::to_string(stripRect.right - stripRect.left) + "," +
      std::to_string(stripRect.bottom - stripRect.top)
  );
  effect = DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK;
  g.testTarget->DragOver(MK_LBUTTON, pt, &effect);
}

void sourceTestLeave()
{
  if (g.testTarget != nullptr) {
    g.testTarget->DragLeave();
    g.testTarget->Release();
    g.testTarget = nullptr;
  }
  if (g.testData != nullptr) {
    g.testData->Release();
    g.testData = nullptr;
  }
}

void sourceShutdown()
{
  sourceTestLeave();
  std::map<uint32_t, std::unique_ptr<SourceSession>> sessions;
  {
    std::lock_guard lock(g_sessionsMutex);
    sessions.swap(g_sessions);
  }
  sessions.clear();
  for (int i = 0; i < 4; ++i) {
    if (g.strips[i] != nullptr) {
      RevokeDragDrop(g.strips[i]);
      DestroyWindow(g.strips[i]);
      g.strips[i] = nullptr;
    }
  }
  if (g.catcher != nullptr) {
    RevokeDragDrop(g.catcher);
    DestroyWindow(g.catcher);
    g.catcher = nullptr;
  }
}

} // namespace deskflow::filedrag::helper
