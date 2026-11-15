/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

// End-to-end check of the Windows file drag, without a mouse or keyboard.
//
// Runs a primary and a secondary deskflow-filedrag.exe in --test mode on a hidden desktop
// (nothing shows on the user's screen) and plays deskflow-core for both: it relays their
// peer messages as the network would, hands the drag over as the server does when the
// cursor crosses, and sends the cancels the server and client send. The helpers' test
// hooks put a real shell data object of the test files on the edge window's drop target,
// and run the drag loop against a real folder drop target, so Explorer's copy engine reads
// the virtual files exactly as it does after a real drop.
//
// Both directions: first server to client (the --primary helper sends, its left edge leads
// to the client), then client to server (the --secondary helper sends from its right edge,
// the --primary helper shows the drag and receives the files).
//
// Usage: deskflow-filedrag-e2e.exe <work folder> [helper exe]

#include "deskflow/FileDragCodec.h"
#include "deskflow/FileDragPolicy.h"
#include "platform/MSWindowsFileDragPipe.h"

#include <bcrypt.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
using namespace deskflow::filedrag;
using Clock = std::chrono::steady_clock;

namespace {

constexpr const wchar_t *kDesktopName = L"DeskflowFileDragTest";

struct Helper
{
  const char *name = "";
  pipe::HelperProcess process;
  pipe::FrameWriter writer;
  std::thread reader;
};

Helper g_a; // source: the primary (server to client), then the secondary (client to server)
Helper g_b; // destination: the secondary, then the primary

// the side of the source's screen that leads to the destination (a DirectionMask bit)
Direction g_watchSide = Direction::Left;

std::mutex g_mutex;
std::condition_variable g_cond;
std::vector<std::string> g_events;
std::atomic<bool> g_linkUp{true};
std::atomic<uint64_t> g_rateBytesPerSecond{0}; // 0: unlimited
std::atomic<uint64_t> g_chunksRelayed{0};
uint32_t g_token = 1;
FILE *g_log = nullptr;

void note(const std::string &line)
{
  std::lock_guard lock(g_mutex);
  g_events.push_back(line);
  if (g_log != nullptr) {
    fprintf(g_log, "%s\n", line.c_str());
    fflush(g_log);
  }
  g_cond.notify_all();
}

size_t eventCount()
{
  std::lock_guard lock(g_mutex);
  return g_events.size();
}

//! Waits for an event containing \p text after index \p from; returns its text or ""
std::string waitFor(const std::string &text, size_t from, int timeoutMs)
{
  std::unique_lock lock(g_mutex);
  const auto deadline = Clock::now() + std::chrono::milliseconds(timeoutMs);
  size_t i = from;
  while (true) {
    for (; i < g_events.size(); ++i) {
      if (g_events[i].find(text) != std::string::npos) {
        return g_events[i];
      }
    }
    if (g_cond.wait_until(lock, deadline) == std::cv_status::timeout) {
      for (; i < g_events.size(); ++i) {
        if (g_events[i].find(text) != std::string::npos) {
          return g_events[i];
        }
      }
      return {};
    }
  }
}

void readerLoop(Helper *self, Helper *peer, bool throttle)
{
  pipe::Frame frame;
  std::string payload;
  auto nextDue = Clock::now();
  while (pipe::readFrame(self->process.fromHelper, frame, payload)) {
    const std::string who = self->name;
    switch (frame) {
    case pipe::Frame::Peer: {
      Message message;
      std::string error;
      if (!decodeMessage(payload, message, &error)) {
        note(who + ":bad-peer " + error);
        break;
      }
      if (message.type != Message::Type::Chunk && message.type != Message::Type::Ack) {
        note(
            who + ":peer " + messageTypeName(message.type) + " " + std::to_string(message.dragId) +
            (message.type == Message::Type::Cancel ? std::string(" ") + cancelReasonName(message.reason) : "")
        );
      }
      if (!g_linkUp) {
        break; // connection "lost": nothing gets through
      }
      if (throttle && message.type == Message::Type::Chunk) {
        const uint64_t rate = g_rateBytesPerSecond;
        if (rate > 0) {
          // pace chunks to the rate, like a slower network would
          nextDue = std::max(nextDue, Clock::now()) +
                    std::chrono::microseconds(static_cast<uint64_t>(message.data.size()) * 1000000 / rate);
          std::this_thread::sleep_until(nextDue);
        }
        ++g_chunksRelayed;
      }
      peer->writer.write(pipe::Frame::Peer, payload);
      break;
    }
    case pipe::Frame::SyncAck: {
      ByteReader reader(payload);
      uint32_t token = 0;
      reader.u32(token);
      note(who + ":syncack " + std::to_string(token));
      break;
    }
    case pipe::Frame::Log: {
      ByteReader reader(payload);
      uint8_t level = 0;
      std::string text;
      reader.u8(level);
      reader.bytes32(text, 65536);
      note(who + ":log " + text);
      break;
    }
    case pipe::Frame::TestEvent:
      note(who + ":" + payload);
      break;
    case pipe::Frame::LauncherReady: {
      ByteReader reader(payload);
      uint32_t id = 0;
      reader.u32(id);
      note(who + ":LauncherReady " + std::to_string(id));
      break;
    }
    case pipe::Frame::LoopEnded: {
      ByteReader reader(payload);
      uint32_t id = 0;
      uint8_t dropped = 0;
      reader.u32(id);
      reader.u8(dropped);
      note(who + ":LoopEnded " + std::to_string(id) + (dropped ? " dropped" : " not-dropped"));
      break;
    }
    default:
      note(who + ":" + pipe::frameName(frame));
      break;
    }
  }
  note(std::string(self->name) + ":pipe-closed");
}

bool startHelper(Helper &helper, const char *name, const std::wstring &exe, const std::wstring &args)
{
  helper.name = name;
  std::string error;
  const std::wstring desktop = std::wstring(L"WinSta0\\") + kDesktopName;
  if (!pipe::startHelper(exe, args, nullptr, desktop.c_str(), helper.process, error)) {
    fprintf(stderr, "cannot start %s helper: %s\n", name, error.c_str());
    return false;
  }
  helper.writer.setPipe(helper.process.toHelper);
  return true;
}

// test data

void writeRandomFile(const fs::path &path, uint64_t size, uint64_t seed)
{
  fs::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  std::vector<uint64_t> block(8192);
  uint64_t x = seed * 0x9E3779B97F4A7C15ull + 1;
  uint64_t left = size;
  while (left > 0) {
    for (auto &v : block) {
      x ^= x << 13;
      x ^= x >> 7;
      x ^= x << 17;
      v = x;
    }
    const uint64_t n = std::min<uint64_t>(left, block.size() * sizeof(uint64_t));
    out.write(reinterpret_cast<const char *>(block.data()), static_cast<std::streamsize>(n));
    left -= n;
  }
}

std::string sha256(const fs::path &path)
{
  BCRYPT_ALG_HANDLE alg = nullptr;
  BCRYPT_HASH_HANDLE hash = nullptr;
  if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) {
    return {};
  }
  BCryptCreateHash(alg, &hash, nullptr, 0, nullptr, 0, 0);
  std::ifstream in(path, std::ios::binary);
  std::vector<char> buffer(1 << 20);
  while (in) {
    in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    const auto n = static_cast<ULONG>(in.gcount());
    if (n > 0) {
      BCryptHashData(hash, reinterpret_cast<PUCHAR>(buffer.data()), n, 0);
    }
  }
  unsigned char digest[32];
  BCryptFinishHash(hash, digest, sizeof(digest), 0);
  BCryptDestroyHash(hash);
  BCryptCloseAlgorithmProvider(alg, 0);
  static const char *hex = "0123456789abcdef";
  std::string text;
  for (unsigned char c : digest) {
    text.push_back(hex[c >> 4]);
    text.push_back(hex[c & 15]);
  }
  return text;
}

//! Every file and folder under \p root, relative, with the hash of each file ("" for folders)
std::map<std::string, std::string> snapshot(const fs::path &root)
{
  std::map<std::string, std::string> out;
  std::error_code ec;
  if (!fs::exists(root, ec)) {
    return out;
  }
  for (auto it = fs::recursive_directory_iterator(root, ec); it != fs::recursive_directory_iterator();
       it.increment(ec)) {
    const auto rel = fs::relative(it->path(), root, ec).generic_string();
    out[rel] = it->is_directory(ec) ? std::string("<dir>") : sha256(it->path());
  }
  return out;
}

std::map<std::string, std::string> expected(const fs::path &root, const std::vector<std::string> &names)
{
  std::map<std::string, std::string> out;
  for (const auto &name : names) {
    const fs::path p = root / name;
    std::error_code ec;
    if (fs::is_directory(p, ec)) {
      out[name] = "<dir>";
      for (const auto &[rel, hash] : snapshot(p)) {
        out[name + "/" + rel] = hash;
      }
    } else {
      out[name] = sha256(p);
    }
  }
  return out;
}

uint64_t folderBytes(const fs::path &root)
{
  uint64_t total = 0;
  std::error_code ec;
  if (!fs::exists(root, ec)) {
    return 0;
  }
  for (auto it = fs::recursive_directory_iterator(root, ec); it != fs::recursive_directory_iterator();
       it.increment(ec)) {
    if (it->is_regular_file(ec)) {
      // ask the file itself: a folder listing shows stale sizes while a file is written
      const auto size = fs::file_size(it->path(), ec);
      total += ec ? 0 : size;
    }
  }
  return total;
}

bool isEmptyOrMissing(const fs::path &root)
{
  std::error_code ec;
  return !fs::exists(root, ec) || fs::is_empty(root, ec);
}

//! Windows other than our own on the test desktop (e.g. a copy error dialog)
std::vector<std::string> strangeWindowsNow(HDESK desk);

//! Windows left on the test desktop after a few seconds (a progress box closes by itself)
std::vector<std::string> strangeWindows(HDESK desk)
{
  auto out = strangeWindowsNow(desk);
  for (int i = 0; i < 50 && !out.empty(); ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    out = strangeWindowsNow(desk);
  }
  return out;
}

std::vector<std::string> strangeWindowsNow(HDESK desk)
{
  std::vector<std::string> out;
  EnumDesktopWindows(
      desk,
      [](HWND hwnd, LPARAM param) -> BOOL {
        if (!IsWindowVisible(hwnd)) {
          return TRUE;
        }
        wchar_t cls[128] = {};
        wchar_t title[256] = {};
        GetClassNameW(hwnd, cls, 128);
        GetWindowTextW(hwnd, title, 256);
        std::wstring c = cls;
        // our own windows, and the input indicator Windows puts on every desktop
        if (c.rfind(L"DeskflowFileDrag", 0) == 0 || c.rfind(L"UAC_InputIndicator", 0) == 0 ||
            std::wstring(title) == L"UAC Input Indicator") {
          return TRUE;
        }
        reinterpret_cast<std::vector<std::string> *>(param)->push_back(
            pipe::narrow(c) + " \"" + pipe::narrow(title) + "\""
        );
        return TRUE;
      },
      reinterpret_cast<LPARAM>(&out)
  );
  return out;
}

void closeStrangeWindows(HDESK desk)
{
  EnumDesktopWindows(
      desk,
      [](HWND hwnd, LPARAM) -> BOOL {
        wchar_t cls[128] = {};
        GetClassNameW(hwnd, cls, 128);
        const std::wstring c = cls;
        if (IsWindowVisible(hwnd) && c.rfind(L"DeskflowFileDrag", 0) != 0 && c.rfind(L"UAC_InputIndicator", 0) != 0 &&
            c != L"#32770") {
          PostMessageW(hwnd, WM_CLOSE, 0, 0);
        }
        return TRUE;
      },
      0
  );
}

// helper commands

void sendPeer(Helper &helper, const Message &message)
{
  helper.writer.write(pipe::Frame::Peer, encodeMessage(message));
}

void watchEdge()
{
  ByteWriter writer;
  // the core sends a DirectionMask (it once sent 2 for left while the helper read 1 as left)
  writer.u32(1u << static_cast<int>(g_watchSide));
  writer.u32(static_cast<uint32_t>(GetSystemMetrics(SM_XVIRTUALSCREEN)));
  writer.u32(static_cast<uint32_t>(GetSystemMetrics(SM_YVIRTUALSCREEN)));
  writer.u32(static_cast<uint32_t>(GetSystemMetrics(SM_CXVIRTUALSCREEN)));
  writer.u32(static_cast<uint32_t>(GetSystemMetrics(SM_CYVIRTUALSCREEN)));
  g_a.writer.write(pipe::Frame::WatchEdges, writer.str());
}

void sourceEnter(const std::vector<fs::path> &paths)
{
  ByteWriter writer;
  writer.u32(static_cast<uint32_t>(paths.size()));
  for (const auto &p : paths) {
    writer.bytes16(pipe::narrow(p.wstring()));
  }
  g_a.writer.write(pipe::Frame::TestSourceEnter, writer.str());
}

bool syncCall(Helper &helper, pipe::Frame frame, const std::string &rest)
{
  const uint32_t token = g_token++;
  ByteWriter writer;
  writer.u32(token);
  const size_t from = eventCount();
  helper.writer.write(frame, writer.str() + rest);
  return !waitFor(std::string(helper.name) + ":syncack " + std::to_string(token), from, 3000).empty();
}

bool handOff(uint32_t dragId)
{
  ByteWriter writer;
  writer.u32(dragId);
  writer.u32(100);
  writer.u32(100);
  return syncCall(g_a, pipe::Frame::HandOff, writer.str());
}

void setDropFolder(const fs::path &folder)
{
  ByteWriter writer;
  writer.bytes16(pipe::narrow(folder.wstring()));
  g_b.writer.write(pipe::Frame::TestDropTarget, writer.str());
}

void release(bool escape)
{
  ByteWriter writer;
  writer.u8(escape ? 1 : 0);
  g_b.writer.write(pipe::Frame::TestRelease, writer.str());
}

std::string stagingFrom(const std::string &event)
{
  const auto at = event.find("staging=");
  return at == std::string::npos ? std::string() : event.substr(at + 8);
}

struct Result
{
  std::string name;
  bool ok = true;
  std::vector<std::string> notes;

  void check(bool condition, const std::string &what)
  {
    notes.push_back(std::string(condition ? "  ok   " : "  FAIL ") + what);
    if (!condition) {
      ok = false;
    }
  }
};

std::vector<Result> g_results;

struct DragStart
{
  bool ok = false;
  size_t from = 0;
  fs::path staging;
};

//! Puts \p items on the edge, crosses, and waits until the drag loop runs on the secondary
DragStart startDrag(
    Result &r, uint32_t dragId, const fs::path &src, const std::vector<std::string> &items, const fs::path &target
)
{
  DragStart d;
  d.from = eventCount();
  fs::create_directories(target);
  setDropFolder(target);
  watchEdge();
  std::vector<fs::path> paths;
  for (const auto &name : items) {
    paths.push_back(src / name);
  }
  sourceEnter(paths);
  const auto entered = waitFor("A:source-enter", d.from, 15000);
  const EdgeRect screen{
      GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN), GetSystemMetrics(SM_CXVIRTUALSCREEN),
      GetSystemMetrics(SM_CYVIRTUALSCREEN)
  };
  const auto want = edgeStrip(g_watchSide, 1u << static_cast<int>(g_watchSide), screen);
  const std::string strip = want ? "strip=" + std::to_string(want->x) + "," + std::to_string(want->y) + "," +
                                       std::to_string(want->w) + "," + std::to_string(want->h)
                                 : std::string("strip=?");
  r.check(
      entered.find(strip) != std::string::npos, std::string("the edge window is on the ") +
                                                    (g_watchSide == Direction::Left ? "left" : "right") +
                                                    " edge it was asked for (" + strip + ")"
  );
  const bool offered = !waitFor("A:offer-ready", d.from, 15000).empty();
  r.check(offered, "the edge window saw the files arrive (offer ready)");
  if (!offered) {
    return d;
  }
  r.check(handOff(dragId), "hand-off answered (catcher up before the cursor moves)");
  const auto launcher = waitFor("B:launcher-ready " + std::to_string(dragId), d.from, 10000);
  r.check(!launcher.empty(), "destination staged the item list and showed the launcher");
  d.staging = stagingFrom(launcher);
  const bool looping = !waitFor("B:loop-started", d.from, 10000).empty();
  r.check(looping, "drag loop started on the destination");
  r.check(!waitFor("B:drag-image", d.from, 3000).empty(), "drag image set");
  d.ok = offered && !launcher.empty() && looping;
  return d;
}

void finishScenario(Result &r)
{
  // the drag ended on the source too; nothing is parked
  g_a.writer.write(pipe::Frame::Entered);
  g_a.writer.write(pipe::Frame::TestSourceLeave);
  g_results.push_back(r);
  for (const auto &line : r.notes) {
    note("RESULT " + r.name + line);
  }
}

void scenarioDrop(
    uint32_t id, const char *name, const fs::path &work, const std::vector<std::string> &items, uint64_t rate,
    HDESK desk
)
{
  Result r;
  r.name = name;
  const fs::path src = work / "src";
  const fs::path target = work / "targets" / name;
  const auto before = expected(src, items);
  uint64_t total = 0;
  for (const auto &name2 : items) {
    total += fs::is_directory(src / name2) ? folderBytes(src / name2) : fs::file_size(src / name2);
  }
  g_rateBytesPerSecond = rate;
  auto d = startDrag(r, id, src, items, target);
  if (d.ok) {
    // the data flows before the drop: wait for a part of it in staging
    const auto t0 = Clock::now();
    uint64_t staged = 0;
    while (Clock::now() - t0 < std::chrono::seconds(20)) {
      staged = folderBytes(d.staging);
      if (staged >= std::min<uint64_t>(total, 1u << 20)) {
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    r.check(
        staged > 0,
        "staging filled before the drop (" + std::to_string(staged) + " of " + std::to_string(total) + " bytes)"
    );
    if (rate > 0) {
      r.check(staged < total, "the transfer was still running when the drop happened");
    }
    release(false);
    const bool finished = !waitFor("B:dest-finished " + std::to_string(id), d.from, 120000).empty();
    r.check(finished, "Explorer's copy engine finished and the destination reported it");
    r.check(!waitFor("A:source-finished " + std::to_string(id), d.from, 10000).empty(), "source heard it finished");
    r.check(!waitFor("B:LoopEnded " + std::to_string(id) + " dropped", d.from, 5000).empty(), "loop ended as dropped");
    const auto after = snapshot(target);
    r.check(
        after == before,
        "target has exactly the dragged items, SHA-256 identical (" + std::to_string(after.size()) + " entries)"
    );
    if (after != before) {
      for (const auto &[k, v] : before) {
        auto it = after.find(k);
        note(
            "  expected " + k + " " + v.substr(0, 12) + " got " + (it == after.end() ? "-" : it->second.substr(0, 12))
        );
      }
    }
    waitFor("B:staging-removed " + std::to_string(id), d.from, 10000);
    r.check(isEmptyOrMissing(d.staging), "staging folder removed after the drop");
    r.check(expected(src, items) == before, "source files untouched");
  }
  const auto windows = strangeWindows(desk);
  r.check(windows.empty(), "no dialogs left on the test desktop");
  for (const auto &w : windows) {
    note("  window: " + w);
  }
  closeStrangeWindows(desk);
  finishScenario(r);
}

enum class CancelHow
{
  ServerEscape,    // escape on the server keyboard: client core cancels (flags 1)
  ClientEscape,    // escape inside the drag loop (OLE's escape)
  CursorReturned,  // the cursor went back before the drop
  LostBeforeDrop,  // the connection dropped while dragging
  LostWhileCopying // the connection dropped after the drop, while Explorer copies
};

//! What the cores send their helpers when the connection drops: the client cancels every
//! drag of the connection (drag id 0), the server just the drags of that client
void lostConnection(uint32_t id)
{
  const bool clientToServer = g_watchSide == Direction::Right;
  Helper &client = clientToServer ? g_a : g_b;
  Helper &server = clientToServer ? g_b : g_a;
  sendPeer(client, Message::cancel(0, CancelReason::ConnectionLost));
  sendPeer(server, Message::cancel(id, CancelReason::ConnectionLost));
}

void scenarioCancel(uint32_t id, const char *name, const fs::path &work, CancelHow how, HDESK desk)
{
  Result r;
  r.name = name;
  const fs::path src = work / "src";
  const fs::path target = work / "targets" / name;
  const std::vector<std::string> items = {"big.bin"};
  const auto before = expected(src, items);
  g_rateBytesPerSecond = 4ull * 1024 * 1024;
  g_linkUp = true;
  auto d = startDrag(r, id, src, items, target);
  if (d.ok) {
    const auto t0 = Clock::now();
    while (folderBytes(d.staging) < (1u << 20) && Clock::now() - t0 < std::chrono::seconds(20)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    r.check(folderBytes(d.staging) > 0, "partial data staged before cancelling");
    const auto at = eventCount();
    switch (how) {
    case CancelHow::ServerEscape: {
      auto cancel = Message::cancel(id, CancelReason::Escape);
      cancel.flags = 1;
      sendPeer(g_b, cancel);
      r.check(!waitFor("A:source-cancelled " + std::to_string(id), at, 10000).empty(), "source told, sending stopped");
      break;
    }
    case CancelHow::ClientEscape:
      release(true);
      r.check(!waitFor("A:source-cancelled " + std::to_string(id), at, 10000).empty(), "source told, sending stopped");
      break;
    case CancelHow::CursorReturned:
      sendPeer(g_b, Message::cancel(id, CancelReason::CursorReturned));
      r.check(syncCall(g_a, pipe::Frame::TakeBack, {}), "source took the drag back");
      r.check(!waitFor("A:source-takeback " + std::to_string(id), at, 5000).empty(), "source stopped sending");
      break;
    case CancelHow::LostBeforeDrop:
      g_linkUp = false;
      lostConnection(id);
      r.check(!waitFor("A:source-cancelled " + std::to_string(id), at, 10000).empty(), "source stopped sending");
      break;
    case CancelHow::LostWhileCopying: {
      release(false);
      r.check(
          !waitFor("B:async-started " + std::to_string(id), at, 10000).empty(), "Explorer copies in the background"
      );
      const auto t1 = Clock::now();
      while (folderBytes(target) == 0 && Clock::now() - t1 < std::chrono::seconds(10)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
      }
      r.check(folderBytes(target) > 0, "the copy at the target was under way");
      g_linkUp = false;
      lostConnection(id);
      break;
    }
    }
    r.check(!waitFor("B:dest-cancelled " + std::to_string(id), at, 10000).empty(), "destination cancelled the drag");
    if (how != CancelHow::LostWhileCopying) {
      r.check(
          !waitFor("B:LoopEnded " + std::to_string(id) + " not-dropped", at, 10000).empty(),
          "drag loop ended without a drop"
      );
    }
    // give the copy engine time to clean up after itself
    const auto t2 = Clock::now();
    while ((!isEmptyOrMissing(d.staging) || !isEmptyOrMissing(target)) && Clock::now() - t2 < std::chrono::seconds(15)
    ) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      closeStrangeWindows(desk);
    }
    r.check(isEmptyOrMissing(d.staging), "no partial file left in staging");
    r.check(isEmptyOrMissing(target), "no partial file left at the target");
    r.check(expected(src, items) == before, "source file untouched");
  }
  const auto windows = strangeWindows(desk);
  for (const auto &w : windows) {
    note("  window: " + w);
  }
  r.check(windows.empty(), "no dialogs left on the test desktop");
  closeStrangeWindows(desk);
  g_linkUp = true;
  finishScenario(r);
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

} // namespace

int wmain(int argc, wchar_t **argv)
{
  if (argc < 2) {
    fprintf(stderr, "usage: deskflow-filedrag-e2e <work folder> [helper exe]\n");
    return 2;
  }
  const fs::path work = argv[1];
  fs::path exe = argc > 2 ? fs::path(argv[2]) : fs::path(argv[0]).parent_path() / L"deskflow-filedrag.exe";
  std::error_code ec;
  fs::remove_all(work, ec);
  fs::create_directories(work / "targets");
  g_log = _wfopen((work / L"e2e.log").c_str(), L"w");

  // test data: a big file, a nested folder (with an empty folder) and a small file
  const fs::path src = work / "src";
  writeRandomFile(src / "big.bin", 40ull * 1024 * 1024, 1);
  writeRandomFile(src / "Album" / "cover.jpg", 300 * 1024, 2);
  writeRandomFile(src / "Album" / "disc 1" / "track 01.flac", 3ull * 1024 * 1024 + 17, 3);
  writeRandomFile(src / "Album" / "disc 1" / "notes" / "read me.txt", 1234, 4);
  writeRandomFile(src / "Album" / "disc 2" / "zero.bin", 0, 5);
  fs::create_directories(src / "Album" / "empty folder");
  writeRandomFile(src / "small.txt", 5000, 6);

  HDESK desk = CreateDesktopW(kDesktopName, nullptr, nullptr, 0, GENERIC_ALL, nullptr);
  if (desk == nullptr) {
    fprintf(stderr, "cannot create the hidden test desktop: %s\n", pipe::errorText(GetLastError()).c_str());
    return 3;
  }

  const std::wstring staging = (work / L"staging").wstring();
  auto startPair = [&](const wchar_t *sourceRole, const wchar_t *destRole) {
    const size_t from = eventCount();
    if (!startHelper(g_a, "A", exe, std::wstring(sourceRole) + L" --test") ||
        !startHelper(g_b, "B", exe, std::wstring(destRole) + L" --test --staging-root \"" + staging + L"\"")) {
      return false;
    }
    g_a.reader = std::thread(readerLoop, &g_a, &g_b, true);
    g_b.reader = std::thread(readerLoop, &g_b, &g_a, false);
    const auto helloA = waitFor("A:Hello", from, 10000);
    const auto helloB = waitFor("B:Hello", from, 10000);
    return !helloA.empty() && !helloB.empty();
  };
  auto stopPair = [&]() {
    g_a.writer.write(pipe::Frame::Shutdown);
    g_b.writer.write(pipe::Frame::Shutdown);
    for (Helper *h : {&g_a, &g_b}) {
      if (WaitForSingleObject(h->process.process, 3000) == WAIT_TIMEOUT) {
        TerminateProcess(h->process.process, 1);
      }
      if (h->reader.joinable()) {
        h->reader.join();
      }
      h->process.close();
      h->process = {};
    }
  };

  // server to client: the server's helper sends from its left edge (the laptop is there)
  g_watchSide = Direction::Left;
  if (!startPair(L"--primary", L"--secondary")) {
    fprintf(stderr, "helpers did not start\n");
    return 5;
  }
  scenarioDrop(1, "drop-file", work, {"big.bin"}, 10ull * 1024 * 1024, desk);
  scenarioDrop(2, "drop-folder", work, {"Album", "small.txt"}, 0, desk);
  scenarioCancel(3, "escape-server-keyboard", work, CancelHow::ServerEscape, desk);
  scenarioCancel(4, "escape-in-drag-loop", work, CancelHow::ClientEscape, desk);
  scenarioCancel(5, "cursor-returned", work, CancelHow::CursorReturned, desk);
  scenarioCancel(6, "connection-lost-before-drop", work, CancelHow::LostBeforeDrop, desk);
  scenarioCancel(7, "connection-lost-while-copying", work, CancelHow::LostWhileCopying, desk);
  stopPair();

  // client to server: the laptop's helper sends from its right edge (the server is there),
  // the server's helper shows the drag and receives the files
  g_watchSide = Direction::Right;
  if (!startPair(L"--secondary", L"--primary")) {
    fprintf(stderr, "helpers did not start for the client to server run\n");
    return 5;
  }
  scenarioDrop(11, "reverse-drop-file", work, {"big.bin"}, 10ull * 1024 * 1024, desk);
  scenarioDrop(12, "reverse-drop-folder", work, {"Album", "small.txt"}, 0, desk);
  scenarioCancel(13, "reverse-escape-in-drag-loop", work, CancelHow::ClientEscape, desk);
  scenarioCancel(14, "reverse-cursor-returned", work, CancelHow::CursorReturned, desk);
  scenarioCancel(15, "reverse-connection-lost-before-drop", work, CancelHow::LostBeforeDrop, desk);
  scenarioCancel(16, "reverse-connection-lost-while-copying", work, CancelHow::LostWhileCopying, desk);
  stopPair();
  CloseDesktop(desk);

  bool all = true;
  printf("\nfile drag end-to-end results\n");
  for (const auto &r : g_results) {
    printf("%s %s\n", r.ok ? "PASS" : "FAIL", r.name.c_str());
    for (const auto &line : r.notes) {
      printf("%s\n", line.c_str());
    }
    all = all && r.ok;
  }
  if (g_log != nullptr) {
    fclose(g_log);
  }
  return all ? 0 : 1;
}
