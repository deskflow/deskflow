/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

// Destination role (a client for server to client drags, the server for client to server
// drags): stage the incoming files and run a drag under the cursor
// whose data object hands Explorer virtual files (FILEGROUPDESCRIPTORW + FILECONTENTS)
// that read from the staging folder while it is still filling. Explorer then places the
// files itself, on the desktop or in whatever folder the user drops them on, and handles
// name conflicts the usual way.

#include "FileDragHelper.h"

#include "deskflow/FileDragCodec.h"
#include "deskflow/FileDragStaging.h"

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

#include <ole2.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shobjidl.h>

namespace fs = std::filesystem;

namespace deskflow::filedrag::helper {

namespace {

constexpr int kLauncherSide = 64;
constexpr UINT_PTR kLauncherTimer = 1;
constexpr UINT kLauncherTimeoutMs = 5000;
constexpr UINT_PTR kRemovalTimer = 7;

enum class State
{
  Waiting, ///< launcher shown, waiting for the button press
  Looping, ///< the drag loop runs
  Dropped, ///< dropped; the target may still be copying
  Done
};

struct DestSession
{
  explicit DestSession(const fs::path &root) : staging(root)
  {
  }

  uint32_t id = 0;
  FileDragStaging staging;
  Icon icon;
  std::vector<FILEDESCRIPTORW> descriptors;
  bool firstIsFolder = false;

  State state = State::Waiting; // UI thread only
  std::atomic<bool> cancelled{false};
  std::atomic<CancelReason> reason{CancelReason::Unknown};
  std::atomic<bool> told{false}; ///< the server knows how it ended
  std::atomic<bool> escape{false};
};

using SessionPtr = std::shared_ptr<DestSession>;

std::mutex g_mutex;
std::map<uint32_t, SessionPtr> g_sessions;
Icon g_pendingIcon;
uint32_t g_pendingIconId = 0;

// UI thread
HWND g_launcher = nullptr;
SessionPtr g_launcherSession;
std::vector<SessionPtr> g_pendingRemoval;
int g_removalTries = 0;

// test harness
std::wstring g_testFolder;
std::atomic<bool> g_testButtonDown{false};
std::atomic<bool> g_testEscape{false};

fs::path stagingRoot()
{
  return options().stagingRoot.empty() ? FileDragStaging::defaultRoot() : options().stagingRoot;
}

SessionPtr findSession(uint32_t id)
{
  std::lock_guard lock(g_mutex);
  auto it = g_sessions.find(id);
  return it == g_sessions.end() ? nullptr : it->second;
}

void eraseSession(uint32_t id)
{
  std::lock_guard lock(g_mutex);
  g_sessions.erase(id);
}

void sendLoopEnded(uint32_t id, bool dropped)
{
  ByteWriter writer;
  writer.u32(id);
  writer.u8(dropped ? 1 : 0);
  sendFrame(pipe::Frame::LoopEnded, writer.str());
  testEvent("loop-ended " + std::to_string(id) + (dropped ? " dropped" : " not-dropped"));
}

void scheduleRemoval(const SessionPtr &session)
{
  if (session->staging.removeStaged()) {
    testEvent("staging-removed " + std::to_string(session->id));
    return;
  }
  g_pendingRemoval.push_back(session);
  g_removalTries = 0;
  SetTimer(controlWindow(), kRemovalTimer, 2000, nullptr);
}

void onCancelledUi(const SessionPtr &session);

//! Fails the drag: wakes readers, deletes the staged files, ends the loop
void cancelSession(const SessionPtr &session, CancelReason reason, bool tellServer)
{
  if (session->cancelled.exchange(true)) {
    return;
  }
  session->reason = reason;
  session->staging.cancel();
  log(LogLevel::Info, "file drag %u cancelled: %s", session->id, cancelReasonName(reason));
  testEvent("dest-cancelled " + std::to_string(session->id) + " " + cancelReasonName(reason));
  if (!session->told.exchange(true) && tellServer) {
    sendPeer(Message::cancel(session->id, reason));
  }
  postToUi([session] { onCancelledUi(session); });
}

void finishSession(const SessionPtr &session, bool success)
{
  if (session->state == State::Done) {
    return;
  }
  if (!success) {
    cancelSession(session, CancelReason::DropFailed, true);
    session->state = State::Done;
    eraseSession(session->id);
    return;
  }
  session->state = State::Done;
  if (!session->told.exchange(true)) {
    sendPeer(Message::finished(session->id));
  }
  log(LogLevel::Info, "file drag %u: the files are in place", session->id);
  testEvent("dest-finished " + std::to_string(session->id));
  scheduleRemoval(session);
  eraseSession(session->id);
}

//
// staged file stream
//

class StagedStream : public IStream
{
public:
  StagedStream(SessionPtr session, uint32_t index)
      : m_session(std::move(session)),
        m_index(index),
        m_size(m_session->staging.entries()[index].size)
  {
  }

  ~StagedStream()
  {
    if (m_file != INVALID_HANDLE_VALUE) {
      CloseHandle(m_file);
    }
  }

  // IUnknown
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) override
  {
    if (riid == IID_IUnknown || riid == IID_IStream || riid == IID_ISequentialStream) {
      *ppv = static_cast<IStream *>(this);
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

  // ISequentialStream
  HRESULT STDMETHODCALLTYPE Read(void *pv, ULONG cb, ULONG *pcbRead) override
  {
    std::lock_guard lock(m_mutex);
    ULONG total = 0;
    auto *out = static_cast<char *>(pv);
    while (total < cb && m_pos < m_size) {
      uint64_t available = 0;
      const auto result = m_session->staging.waitForData(m_index, m_pos, available, std::chrono::milliseconds(250));
      if (result == FileDragStaging::WaitResult::Failed) {
        if (pcbRead != nullptr) {
          *pcbRead = total;
        }
        // reads as a cancel, so the copy engine stops quietly and removes its partial file
        return HRESULT_FROM_WIN32(ERROR_CANCELLED);
      }
      if (result == FileDragStaging::WaitResult::Timeout) {
        continue;
      }
      if (result == FileDragStaging::WaitResult::Eof) {
        break;
      }
      if (!openFile()) {
        if (pcbRead != nullptr) {
          *pcbRead = total;
        }
        return STG_E_READFAULT;
      }
      const uint64_t want64 = std::min<uint64_t>(cb - total, available - m_pos);
      const auto want = static_cast<DWORD>(std::min<uint64_t>(want64, 1u << 20));
      LARGE_INTEGER offset;
      offset.QuadPart = static_cast<LONGLONG>(m_pos);
      DWORD got = 0;
      if (!SetFilePointerEx(m_file, offset, nullptr, FILE_BEGIN) ||
          !ReadFile(m_file, out + total, want, &got, nullptr)) {
        if (pcbRead != nullptr) {
          *pcbRead = total;
        }
        return STG_E_READFAULT;
      }
      if (got == 0) {
        Sleep(1);
        continue;
      }
      m_pos += got;
      total += got;
    }
    if (pcbRead != nullptr) {
      *pcbRead = total;
    }
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE Write(const void *, ULONG, ULONG *) override
  {
    return STG_E_ACCESSDENIED;
  }

  // IStream
  HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER move, DWORD origin, ULARGE_INTEGER *newPos) override
  {
    std::lock_guard lock(m_mutex);
    int64_t base = 0;
    switch (origin) {
    case STREAM_SEEK_SET:
      base = 0;
      break;
    case STREAM_SEEK_CUR:
      base = static_cast<int64_t>(m_pos);
      break;
    case STREAM_SEEK_END:
      base = static_cast<int64_t>(m_size);
      break;
    default:
      return STG_E_INVALIDFUNCTION;
    }
    const int64_t target = base + move.QuadPart;
    if (target < 0) {
      return STG_E_INVALIDFUNCTION;
    }
    m_pos = std::min<uint64_t>(static_cast<uint64_t>(target), m_size);
    if (newPos != nullptr) {
      newPos->QuadPart = m_pos;
    }
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER) override
  {
    return E_NOTIMPL;
  }

  HRESULT STDMETHODCALLTYPE
  CopyTo(IStream *target, ULARGE_INTEGER cb, ULARGE_INTEGER *read, ULARGE_INTEGER *written) override
  {
    std::vector<char> buffer(256 * 1024);
    uint64_t left = cb.QuadPart;
    uint64_t totalRead = 0;
    uint64_t totalWritten = 0;
    HRESULT hr = S_OK;
    while (left > 0) {
      ULONG got = 0;
      hr = Read(buffer.data(), static_cast<ULONG>(std::min<uint64_t>(left, buffer.size())), &got);
      if (FAILED(hr) || got == 0) {
        break;
      }
      totalRead += got;
      ULONG put = 0;
      hr = target->Write(buffer.data(), got, &put);
      totalWritten += put;
      if (FAILED(hr)) {
        break;
      }
      left -= got;
    }
    if (read != nullptr) {
      read->QuadPart = totalRead;
    }
    if (written != nullptr) {
      written->QuadPart = totalWritten;
    }
    return FAILED(hr) ? hr : S_OK;
  }

  HRESULT STDMETHODCALLTYPE Commit(DWORD) override
  {
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE Revert() override
  {
    return E_NOTIMPL;
  }
  HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) override
  {
    return STG_E_INVALIDFUNCTION;
  }
  HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) override
  {
    return STG_E_INVALIDFUNCTION;
  }

  HRESULT STDMETHODCALLTYPE Stat(STATSTG *stat, DWORD flags) override
  {
    if (stat == nullptr) {
      return STG_E_INVALIDPOINTER;
    }
    *stat = {};
    stat->type = STGTY_STREAM;
    stat->cbSize.QuadPart = m_size;
    stat->grfMode = STGM_READ | STGM_SHARE_DENY_NONE;
    stat->clsid = CLSID_NULL;
    if ((flags & STATFLAG_NONAME) == 0) {
      const auto &entry = m_session->staging.entries()[m_index];
      const auto slash = entry.localPath.find_last_of('/');
      const std::wstring name =
          pipe::widen(slash == std::string::npos ? entry.localPath : entry.localPath.substr(slash + 1));
      const size_t bytes = (name.size() + 1) * sizeof(wchar_t);
      stat->pwcsName = static_cast<LPOLESTR>(CoTaskMemAlloc(bytes));
      if (stat->pwcsName != nullptr) {
        memcpy(stat->pwcsName, name.c_str(), bytes);
      }
    }
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE Clone(IStream **) override
  {
    return E_NOTIMPL;
  }

private:
  bool openFile()
  {
    if (m_file == INVALID_HANDLE_VALUE) {
      // share everything: the staging writer keeps writing and may delete the file on cancel
      m_file = CreateFileW(
          m_session->staging.pathOf(m_index).c_str(), GENERIC_READ,
          FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
          FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr
      );
    }
    return m_file != INVALID_HANDLE_VALUE;
  }

  LONG m_refs = 1;
  SessionPtr m_session;
  uint32_t m_index;
  uint64_t m_size;
  uint64_t m_pos = 0;
  HANDLE m_file = INVALID_HANDLE_VALUE;
  std::mutex m_mutex;
};

/**
 * Streams are created in the multi-threaded apartment (kept alive by the pipe reader),
 * so Explorer's background copy reads them without going through the UI thread, which
 * keeps running the drag loop.
 */
HRESULT makeStream(const SessionPtr &session, uint32_t index, IStream **out)
{
  IStream *marshalled = nullptr;
  HRESULT hr = E_FAIL;
  std::thread([&] {
    if (SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED))) {
      auto *stream = new StagedStream(session, index);
      hr = CoMarshalInterThreadInterfaceInStream(IID_IStream, stream, &marshalled);
      stream->Release();
      CoUninitialize();
    }
  }).join();
  if (SUCCEEDED(hr)) {
    hr = CoGetInterfaceAndReleaseStream(marshalled, IID_IStream, reinterpret_cast<void **>(out));
  }
  if (FAILED(hr)) {
    // fall back to a stream served by this thread
    *out = new StagedStream(session, index);
    hr = S_OK;
  }
  return hr;
}

//
// data object
//

HGLOBAL duplicateGlobal(HGLOBAL source)
{
  const SIZE_T size = GlobalSize(source);
  HGLOBAL copy = GlobalAlloc(GMEM_MOVEABLE, size);
  if (copy == nullptr) {
    return nullptr;
  }
  const void *from = GlobalLock(source);
  void *to = GlobalLock(copy);
  if (from != nullptr && to != nullptr) {
    memcpy(to, from, size);
  }
  GlobalUnlock(copy);
  GlobalUnlock(source);
  return copy;
}

class DataObject : public IDataObject, public IDataObjectAsyncCapability
{
public:
  explicit DataObject(SessionPtr session) : m_session(std::move(session))
  {
    m_cfDescriptor = static_cast<CLIPFORMAT>(RegisterClipboardFormatW(CFSTR_FILEDESCRIPTORW));
    m_cfContents = static_cast<CLIPFORMAT>(RegisterClipboardFormatW(CFSTR_FILECONTENTS));
    m_cfPreferred = static_cast<CLIPFORMAT>(RegisterClipboardFormatW(CFSTR_PREFERREDDROPEFFECT));
    m_cfPerformed = static_cast<CLIPFORMAT>(RegisterClipboardFormatW(CFSTR_PERFORMEDDROPEFFECT));
  }

  ~DataObject()
  {
    for (auto &stored : m_stored) {
      ReleaseStgMedium(&stored.medium);
    }
  }

  bool asyncStarted() const
  {
    return m_started;
  }
  bool asyncEnded() const
  {
    return m_ended;
  }

  // IUnknown
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) override
  {
    if (riid == IID_IUnknown || riid == IID_IDataObject) {
      *ppv = static_cast<IDataObject *>(this);
    } else if (riid == __uuidof(IDataObjectAsyncCapability)) {
      *ppv = static_cast<IDataObjectAsyncCapability *>(this);
    } else {
      *ppv = nullptr;
      return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
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

  // IDataObject
  HRESULT STDMETHODCALLTYPE GetData(FORMATETC *format, STGMEDIUM *medium) override
  {
    if (format == nullptr || medium == nullptr) {
      return E_INVALIDARG;
    }
    *medium = {};
    if (format->cfFormat == m_cfDescriptor && (format->tymed & TYMED_HGLOBAL) != 0) {
      const auto &items = m_session->descriptors;
      const SIZE_T size =
          sizeof(FILEGROUPDESCRIPTORW) + (items.empty() ? 0 : (items.size() - 1)) * sizeof(FILEDESCRIPTORW);
      HGLOBAL global = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, size);
      if (global == nullptr) {
        return E_OUTOFMEMORY;
      }
      auto *group = static_cast<FILEGROUPDESCRIPTORW *>(GlobalLock(global));
      group->cItems = static_cast<UINT>(items.size());
      for (size_t i = 0; i < items.size(); ++i) {
        group->fgd[i] = items[i];
      }
      GlobalUnlock(global);
      medium->tymed = TYMED_HGLOBAL;
      medium->hGlobal = global;
      return S_OK;
    }

    if (format->cfFormat == m_cfContents && (format->tymed & TYMED_ISTREAM) != 0) {
      const auto &entries = m_session->staging.entries();
      if (format->lindex < 0 || static_cast<size_t>(format->lindex) >= entries.size() ||
          entries[static_cast<size_t>(format->lindex)].isDirectory) {
        return DV_E_LINDEX;
      }
      if (m_session->cancelled) {
        return E_ABORT;
      }
      IStream *stream = nullptr;
      const HRESULT hr = makeStream(m_session, static_cast<uint32_t>(format->lindex), &stream);
      if (FAILED(hr)) {
        return hr;
      }
      medium->tymed = TYMED_ISTREAM;
      medium->pstm = stream;
      return S_OK;
    }

    if (format->cfFormat == m_cfPreferred && (format->tymed & TYMED_HGLOBAL) != 0) {
      HGLOBAL global = GlobalAlloc(GMEM_MOVEABLE, sizeof(DWORD));
      if (global == nullptr) {
        return E_OUTOFMEMORY;
      }
      *static_cast<DWORD *>(GlobalLock(global)) = DROPEFFECT_COPY;
      GlobalUnlock(global);
      medium->tymed = TYMED_HGLOBAL;
      medium->hGlobal = global;
      return S_OK;
    }

    for (const auto &stored : m_stored) {
      if (stored.format.cfFormat == format->cfFormat && (stored.format.tymed & format->tymed) != 0) {
        return copyMedium(stored, medium);
      }
    }
    return DV_E_FORMATETC;
  }

  HRESULT STDMETHODCALLTYPE GetDataHere(FORMATETC *, STGMEDIUM *) override
  {
    return E_NOTIMPL;
  }

  HRESULT STDMETHODCALLTYPE QueryGetData(FORMATETC *format) override
  {
    if (format == nullptr) {
      return E_INVALIDARG;
    }
    if ((format->cfFormat == m_cfDescriptor && (format->tymed & TYMED_HGLOBAL) != 0) ||
        (format->cfFormat == m_cfContents && (format->tymed & TYMED_ISTREAM) != 0) ||
        (format->cfFormat == m_cfPreferred && (format->tymed & TYMED_HGLOBAL) != 0)) {
      return S_OK;
    }
    for (const auto &stored : m_stored) {
      if (stored.format.cfFormat == format->cfFormat && (stored.format.tymed & format->tymed) != 0) {
        return S_OK;
      }
    }
    return DV_E_FORMATETC;
  }

  HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(FORMATETC *, FORMATETC *out) override
  {
    if (out != nullptr) {
      out->ptd = nullptr;
    }
    return E_NOTIMPL;
  }

  HRESULT STDMETHODCALLTYPE SetData(FORMATETC *format, STGMEDIUM *medium, BOOL release) override
  {
    if (format == nullptr || medium == nullptr) {
      return E_INVALIDARG;
    }
    if (format->cfFormat == m_cfPerformed && medium->tymed == TYMED_HGLOBAL) {
      const auto *effect = static_cast<const DWORD *>(GlobalLock(medium->hGlobal));
      if (effect != nullptr) {
        m_performed = *effect;
        GlobalUnlock(medium->hGlobal);
      }
    }
    Stored stored;
    stored.format = *format;
    stored.format.ptd = nullptr;
    if (release) {
      stored.medium = *medium;
    } else {
      if (medium->tymed != TYMED_HGLOBAL) {
        return DV_E_TYMED;
      }
      stored.medium.tymed = TYMED_HGLOBAL;
      stored.medium.hGlobal = duplicateGlobal(medium->hGlobal);
      if (stored.medium.hGlobal == nullptr) {
        return E_OUTOFMEMORY;
      }
    }
    for (auto &existing : m_stored) {
      if (existing.format.cfFormat == stored.format.cfFormat) {
        ReleaseStgMedium(&existing.medium);
        existing = stored;
        return S_OK;
      }
    }
    m_stored.push_back(stored);
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD direction, IEnumFORMATETC **out) override
  {
    if (direction != DATADIR_GET) {
      return E_NOTIMPL;
    }
    std::vector<FORMATETC> formats = {
        {m_cfDescriptor, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
        {m_cfContents, nullptr, DVASPECT_CONTENT, -1, TYMED_ISTREAM},
        {m_cfPreferred, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
    };
    for (const auto &stored : m_stored) {
      formats.push_back(stored.format);
    }
    return SHCreateStdEnumFmtEtc(static_cast<UINT>(formats.size()), formats.data(), out);
  }

  HRESULT STDMETHODCALLTYPE DAdvise(FORMATETC *, DWORD, IAdviseSink *, DWORD *) override
  {
    return OLE_E_ADVISENOTSUPPORTED;
  }
  HRESULT STDMETHODCALLTYPE DUnadvise(DWORD) override
  {
    return OLE_E_ADVISENOTSUPPORTED;
  }
  HRESULT STDMETHODCALLTYPE EnumDAdvise(IEnumSTATDATA **) override
  {
    return OLE_E_ADVISENOTSUPPORTED;
  }

  // IDataObjectAsyncCapability: lets Explorer copy on a background thread after the drop
  HRESULT STDMETHODCALLTYPE SetAsyncMode(BOOL async) override
  {
    m_async = async;
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE GetAsyncMode(BOOL *async) override
  {
    *async = m_async;
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE StartOperation(IBindCtx *) override
  {
    m_inOperation = true;
    m_started = true;
    testEvent("async-started " + std::to_string(m_session->id));
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE InOperation(BOOL *in) override
  {
    *in = m_inOperation ? TRUE : FALSE;
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE EndOperation(HRESULT result, IBindCtx *, DWORD effects) override
  {
    m_inOperation = false;
    m_ended = true;
    const bool success = SUCCEEDED(result) && effects != DROPEFFECT_NONE && !m_session->cancelled;
    log(LogLevel::Debug, "file drag %u: target finished, result 0x%08lx effect %lu", m_session->id,
        static_cast<unsigned long>(result), effects);
    SessionPtr session = m_session;
    postToUi([session, success] { finishSession(session, success); });
    return S_OK;
  }

private:
  struct Stored
  {
    FORMATETC format = {};
    STGMEDIUM medium = {};
  };

  static HRESULT copyMedium(const Stored &stored, STGMEDIUM *out)
  {
    switch (stored.medium.tymed) {
    case TYMED_HGLOBAL:
      out->tymed = TYMED_HGLOBAL;
      out->hGlobal = duplicateGlobal(stored.medium.hGlobal);
      return out->hGlobal != nullptr ? S_OK : E_OUTOFMEMORY;
    case TYMED_ISTREAM:
      out->tymed = TYMED_ISTREAM;
      out->pstm = stored.medium.pstm;
      out->pstm->AddRef();
      return S_OK;
    default:
      return DV_E_TYMED;
    }
  }

  LONG m_refs = 1;
  SessionPtr m_session;
  CLIPFORMAT m_cfDescriptor;
  CLIPFORMAT m_cfContents;
  CLIPFORMAT m_cfPreferred;
  CLIPFORMAT m_cfPerformed;
  std::vector<Stored> m_stored;
  DWORD m_performed = DROPEFFECT_NONE;
  BOOL m_async = TRUE;
  std::atomic<bool> m_inOperation{false};
  std::atomic<bool> m_started{false};
  std::atomic<bool> m_ended{false};
};

class DropSource : public IDropSource
{
public:
  explicit DropSource(SessionPtr session) : m_session(std::move(session))
  {
  }

  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) override
  {
    if (riid == IID_IUnknown || riid == IID_IDropSource) {
      *ppv = static_cast<IDropSource *>(this);
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

  HRESULT STDMETHODCALLTYPE QueryContinueDrag(BOOL escapePressed, DWORD keys) override
  {
    if (m_session->cancelled) {
      return DRAGDROP_S_CANCEL;
    }
    if (escapePressed) {
      m_session->escape = true;
      return DRAGDROP_S_CANCEL;
    }
    if ((keys & MK_RBUTTON) != 0) {
      return DRAGDROP_S_CANCEL;
    }
    if ((keys & MK_LBUTTON) == 0) {
      return DRAGDROP_S_DROP;
    }
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE GiveFeedback(DWORD) override
  {
    return DRAGDROP_S_USEDEFAULTCURSORS;
  }

private:
  LONG m_refs = 1;
  SessionPtr m_session;
};

//
// drag image
//

HBITMAP bitmapFromRgba(const Icon &icon)
{
  BITMAPINFO bi = {};
  bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
  bi.bmiHeader.biWidth = icon.width;
  bi.bmiHeader.biHeight = -static_cast<LONG>(icon.height);
  bi.bmiHeader.biPlanes = 1;
  bi.bmiHeader.biBitCount = 32;
  bi.bmiHeader.biCompression = BI_RGB;
  void *bits = nullptr;
  HBITMAP bitmap = CreateDIBSection(nullptr, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
  if (bitmap == nullptr) {
    return nullptr;
  }
  auto *out = static_cast<uint8_t *>(bits);
  const auto *in = reinterpret_cast<const uint8_t *>(icon.rgba.data());
  const size_t pixels = static_cast<size_t>(icon.width) * icon.height;
  for (size_t i = 0; i < pixels; ++i) {
    const unsigned a = in[i * 4 + 3];
    // premultiplied BGRA for the drag image
    out[i * 4 + 0] = static_cast<uint8_t>((in[i * 4 + 2] * a + 127) / 255);
    out[i * 4 + 1] = static_cast<uint8_t>((in[i * 4 + 1] * a + 127) / 255);
    out[i * 4 + 2] = static_cast<uint8_t>((in[i * 4 + 0] * a + 127) / 255);
    out[i * 4 + 3] = static_cast<uint8_t>(a);
  }
  return bitmap;
}

HBITMAP placeholderBitmap(bool folder, int side)
{
  SHSTOCKICONINFO info = {};
  info.cbSize = sizeof(info);
  if (FAILED(SHGetStockIconInfo(folder ? SIID_FOLDER : SIID_DOCNOASSOC, SHGSI_ICON | SHGSI_LARGEICON, &info))) {
    return nullptr;
  }
  BITMAPINFO bi = {};
  bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
  bi.bmiHeader.biWidth = side;
  bi.bmiHeader.biHeight = -side;
  bi.bmiHeader.biPlanes = 1;
  bi.bmiHeader.biBitCount = 32;
  bi.bmiHeader.biCompression = BI_RGB;
  void *bits = nullptr;
  HBITMAP bitmap = CreateDIBSection(nullptr, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
  if (bitmap != nullptr) {
    memset(bits, 0, static_cast<size_t>(side) * side * 4);
    HDC dc = CreateCompatibleDC(nullptr);
    HGDIOBJ old = SelectObject(dc, bitmap);
    DrawIconEx(dc, 0, 0, info.hIcon, side, side, 0, nullptr, DI_NORMAL);
    SelectObject(dc, old);
    DeleteDC(dc);
  }
  DestroyIcon(info.hIcon);
  return bitmap;
}

void setDragImage(IDataObject *data, const DestSession &session)
{
  IDragSourceHelper *helper = nullptr;
  if (FAILED(CoCreateInstance(CLSID_DragDropHelper, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&helper)))) {
    return;
  }
  IDragSourceHelper2 *helper2 = nullptr;
  if (SUCCEEDED(helper->QueryInterface(IID_PPV_ARGS(&helper2)))) {
    helper2->SetFlags(DSH_ALLOWDROPDESCRIPTIONTEXT);
    helper2->Release();
  }
  SHDRAGIMAGE image = {};
  HBITMAP bitmap = nullptr;
  if (session.icon.isValid() && !session.icon.isEmpty()) {
    bitmap = bitmapFromRgba(session.icon);
    image.sizeDragImage = {session.icon.width, session.icon.height};
  } else {
    // the icon did not arrive in time: a stock file or folder picture
    bitmap = placeholderBitmap(session.firstIsFolder, 64);
    image.sizeDragImage = {64, 64};
  }
  if (bitmap != nullptr) {
    image.hbmpDragImage = bitmap;
    image.ptOffset = {image.sizeDragImage.cx / 2, image.sizeDragImage.cy / 2};
    image.crColorKey = CLR_NONE;
    if (FAILED(helper->InitializeFromBitmap(&image, data))) {
      DeleteObject(bitmap);
    } else {
      testEvent(std::string("drag-image ") + (session.icon.isEmpty() ? "placeholder" : "icon"));
    }
  }
  helper->Release();
}

//
// drag loop
//

IDropTarget *testDropTarget()
{
  IShellItem *item = nullptr;
  IDropTarget *target = nullptr;
  if (SUCCEEDED(SHCreateItemFromParsingName(g_testFolder.c_str(), nullptr, IID_PPV_ARGS(&item)))) {
    item->BindToHandler(nullptr, BHID_SFUIObject, IID_PPV_ARGS(&target));
    item->Release();
  }
  return target;
}

//! What DoDragDrop does, against one folder's drop target, driven by the harness
HRESULT testLoop(IDataObject *data, IDropSource *source, DWORD *effect)
{
  *effect = DROPEFFECT_NONE;
  IDropTarget *target = testDropTarget();
  if (target == nullptr) {
    testEvent("error no-drop-target");
    return DRAGDROP_S_CANCEL;
  }
  POINTL pt = {0, 0};
  DWORD eff = DROPEFFECT_COPY;
  target->DragEnter(data, MK_LBUTTON, pt, &eff);
  testEvent("loop-started effect=" + std::to_string(eff));
  HRESULT result = DRAGDROP_S_CANCEL;
  while (true) {
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
    const DWORD keys = g_testButtonDown ? MK_LBUTTON : 0;
    const HRESULT hr = source->QueryContinueDrag(g_testEscape ? TRUE : FALSE, keys);
    if (hr == DRAGDROP_S_DROP) {
      eff = DROPEFFECT_COPY;
      target->DragOver(0, pt, &eff);
      eff = DROPEFFECT_COPY;
      const HRESULT dropResult = target->Drop(data, 0, pt, &eff);
      *effect = SUCCEEDED(dropResult) ? eff : DROPEFFECT_NONE;
      testEvent("test-drop hr=" + std::to_string(static_cast<long>(dropResult)) + " effect=" + std::to_string(eff));
      result = DRAGDROP_S_DROP;
      break;
    }
    if (hr == DRAGDROP_S_CANCEL) {
      target->DragLeave();
      result = DRAGDROP_S_CANCEL;
      break;
    }
    eff = DROPEFFECT_COPY;
    target->DragOver(MK_LBUTTON, pt, &eff);
    MsgWaitForMultipleObjects(0, nullptr, FALSE, 20, QS_ALLINPUT);
  }
  target->Release();
  return result;
}

void runDragLoop(const SessionPtr &session)
{
  session->state = State::Looping;
  {
    std::lock_guard lock(g_mutex);
    if (g_pendingIconId == session->id && session->icon.isEmpty()) {
      session->icon = g_pendingIcon;
    }
  }
  auto *data = new DataObject(session);
  auto *source = new DropSource(session);
  setDragImage(data, *session);

  log(LogLevel::Debug, "file drag %u: drag loop starts", session->id);
  DWORD effect = DROPEFFECT_NONE;
  HRESULT hr;
  if (options().test && !g_testFolder.empty()) {
    hr = testLoop(data, source, &effect);
  } else {
    hr = DoDragDrop(data, source, DROPEFFECT_COPY, &effect);
  }

  const bool dropped = hr == DRAGDROP_S_DROP && effect != DROPEFFECT_NONE && !session->cancelled;
  if (dropped) {
    log(LogLevel::Info, "file drag %u dropped", session->id);
    session->state = State::Dropped;
    sendPeer(Message::dropped(session->id));
    if (!data->asyncStarted()) {
      // the target copied while it handled the drop
      finishSession(session, true);
    }
    // else: EndOperation finishes it
  } else {
    CancelReason reason = CancelReason::User;
    if (session->cancelled) {
      reason = session->reason;
    } else if (hr == DRAGDROP_S_DROP) {
      reason = CancelReason::DropFailed; // let go where nothing takes files
    } else if (session->escape) {
      reason = CancelReason::Escape;
    }
    cancelSession(session, reason, true);
    session->state = State::Done;
    eraseSession(session->id);
  }
  sendLoopEnded(session->id, dropped);
  source->Release();
  data->Release();
}

LRESULT CALLBACK launcherProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg) {
  case WM_MOUSEACTIVATE:
    return MA_NOACTIVATE;

  case WM_LBUTTONDOWN: {
    KillTimer(hwnd, kLauncherTimer);
    // out of the way, so the drop can land right here too
    ShowWindow(hwnd, SW_HIDE);
    SessionPtr session = std::move(g_launcherSession);
    if (session && session->state == State::Waiting && !session->cancelled) {
      runDragLoop(session);
    }
    return 0;
  }

  case WM_TIMER:
    if (wParam == kLauncherTimer) {
      KillTimer(hwnd, kLauncherTimer);
      ShowWindow(hwnd, SW_HIDE);
      SessionPtr session = std::move(g_launcherSession);
      if (session && session->state == State::Waiting) {
        log(LogLevel::Warning, "file drag %u: the drag never started under the cursor", session->id);
        cancelSession(session, CancelReason::Timeout, true);
      }
      return 0;
    }
    break;

  default:
    break;
  }
  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void showLauncher(const SessionPtr &session)
{
  if (session->cancelled) {
    return;
  }
  if (g_launcherSession && g_launcherSession->state == State::Waiting) {
    // an older drag never got its button press
    cancelSession(g_launcherSession, CancelReason::Busy, true);
  }
  if (g_launcher == nullptr) {
    g_launcher = createOverlayWindow(L"DeskflowFileDragLauncher", launcherProc, nullptr);
  }
  if (g_launcher == nullptr) {
    cancelSession(session, CancelReason::Unsupported, true);
    sendLoopEnded(session->id, false);
    eraseSession(session->id);
    return;
  }
  POINT pt = {0, 0};
  GetCursorPos(&pt);
  SetWindowPos(
      g_launcher, HWND_TOPMOST, pt.x - kLauncherSide / 2, pt.y - kLauncherSide / 2, kLauncherSide, kLauncherSide,
      SWP_NOACTIVATE | SWP_SHOWWINDOW
  );
  g_launcherSession = session;
  SetTimer(g_launcher, kLauncherTimer, kLauncherTimeoutMs, nullptr);

  ByteWriter writer;
  writer.u32(session->id);
  writer.u32(static_cast<uint32_t>(pt.x));
  writer.u32(static_cast<uint32_t>(pt.y));
  sendFrame(pipe::Frame::LauncherReady, writer.str());
  testEvent(
      "launcher-ready " + std::to_string(session->id) +
      " staging=" + pipe::narrow(session->staging.stagingDir().wstring())
  );

  if (options().test && !g_testFolder.empty()) {
    // the harness has no mouse: press the button for it
    g_testButtonDown = true;
    g_testEscape = false;
    PostMessageW(g_launcher, WM_LBUTTONDOWN, MK_LBUTTON, 0);
  }
}

void onCancelledUi(const SessionPtr &session)
{
  switch (session->state) {
  case State::Waiting:
    if (g_launcherSession == session) {
      KillTimer(g_launcher, kLauncherTimer);
      ShowWindow(g_launcher, SW_HIDE);
      g_launcherSession.reset();
    }
    session->state = State::Done;
    eraseSession(session->id);
    sendLoopEnded(session->id, false);
    break;

  case State::Looping: {
    // wake the drag loop so it asks QueryContinueDrag, which now says cancel
    HWND capture = GetCapture();
    HWND wake = capture != nullptr ? capture : controlWindow();
    PostMessageW(wake, WM_KEYDOWN, VK_ESCAPE, 0);
    PostMessageW(wake, WM_MOUSEMOVE, MK_LBUTTON, 0);
    break;
  }

  case State::Dropped:
    // the copy fails on its next read; EndOperation (or nothing) follows
    session->state = State::Done;
    eraseSession(session->id);
    break;

  case State::Done:
    break;
  }
}

void failSession(const SessionPtr &session, CancelReason reason, const char *what)
{
  log(LogLevel::Warning, "file drag %u: %s", session->id, what);
  cancelSession(session, reason, true);
}

void beginSession(const Message &message)
{
  auto session = std::make_shared<DestSession>(stagingRoot());
  session->id = message.dragId;
  {
    std::lock_guard lock(g_mutex);
    if (g_pendingIconId == message.dragId) {
      session->icon = g_pendingIcon;
    }
  }

  CancelReason reason = CancelReason::Unknown;
  std::string error;
  if (!session->staging.begin(message.dragId, message.items, reason, &error)) {
    log(LogLevel::Warning, "file drag %u refused: %s (%s)", message.dragId, cancelReasonName(reason), error.c_str());
    testEvent("dest-refused " + std::to_string(message.dragId) + " " + cancelReasonName(reason));
    sendPeer(Message::cancel(message.dragId, reason));
    sendLoopEnded(message.dragId, false);
    return;
  }

  const auto &entries = session->staging.entries();
  session->descriptors.reserve(entries.size());
  for (const auto &entry : entries) {
    std::wstring path = pipe::widen(entry.localPath);
    for (auto &c : path) {
      if (c == L'/') {
        c = L'\\';
      }
    }
    if (path.size() >= MAX_PATH) {
      log(LogLevel::Warning, "file drag %u refused: a path is too long for a Windows drag", message.dragId);
      session->staging.cancel();
      sendPeer(Message::cancel(message.dragId, CancelReason::Invalid));
      sendLoopEnded(message.dragId, false);
      return;
    }
    FILEDESCRIPTORW fd = {};
    fd.dwFlags = FD_ATTRIBUTES | FD_FILESIZE | FD_PROGRESSUI | FD_UNICODE;
    fd.dwFileAttributes = entry.isDirectory ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
    fd.nFileSizeHigh = static_cast<DWORD>(entry.size >> 32);
    fd.nFileSizeLow = static_cast<DWORD>(entry.size & 0xffffffffu);
    wcsncpy_s(fd.cFileName, path.c_str(), _TRUNCATE);
    session->descriptors.push_back(fd);
  }
  session->firstIsFolder = !entries.empty() && entries.front().isDirectory;

  {
    std::lock_guard lock(g_mutex);
    g_sessions[message.dragId] = session;
  }
  log(LogLevel::Info, "file drag %u: staging %zu items in %s", message.dragId, entries.size(),
      pipe::narrow(session->staging.stagingDir().wstring()).c_str());
  postToUi([session] { showLauncher(session); });
}

} // namespace

void destStart()
{
  FileDragStaging::sweepStale(stagingRoot());
}

void destPeer(const Message &message)
{
  switch (message.type) {
    using enum Message::Type;
  case Icon: {
    std::lock_guard lock(g_mutex);
    g_pendingIcon = message.icon;
    g_pendingIconId = message.dragId;
    break;
  }

  case Begin:
    beginSession(message);
    break;

  case FileStart: {
    auto session = findSession(message.dragId);
    if (session && !session->cancelled && !session->staging.fileStart(message.index, message.size)) {
      failSession(session, CancelReason::WriteFailed, "cannot start a staged file");
    }
    break;
  }

  case Chunk: {
    auto session = findSession(message.dragId);
    if (!session || session->cancelled) {
      break;
    }
    if (session->staging.write(message.index, message.data)) {
      sendPeer(Message::ack(message.dragId, message.seq));
    } else {
      failSession(session, CancelReason::WriteFailed, "cannot write a staged file");
    }
    break;
  }

  case FileEnd: {
    auto session = findSession(message.dragId);
    if (session && !session->cancelled && !session->staging.fileEnd(message.index)) {
      failSession(session, CancelReason::WriteFailed, "a staged file is incomplete");
    }
    break;
  }

  case Cancel: {
    if (message.reason == CancelReason::ConnectionLost && message.dragId == 0) {
      // the peer is gone: every drag from it stops, including ones still copying
      std::vector<SessionPtr> all;
      {
        std::lock_guard lock(g_mutex);
        for (auto &[id, s] : g_sessions) {
          all.push_back(s);
        }
      }
      for (auto &s : all) {
        cancelSession(s, CancelReason::ConnectionLost, false);
      }
      break;
    }
    auto session = findSession(message.dragId);
    if (session) {
      // flags 1: cancelled here (escape, right click, cursor left): tell the server
      cancelSession(session, message.reason, message.flags == 1);
    } else if (message.flags == 1) {
      sendPeer(Message::cancel(message.dragId, message.reason));
    }
    break;
  }

  default:
    break;
  }
}

void destTestDropTarget(const std::wstring &folder)
{
  g_testFolder = folder;
}

void destTestRelease(bool escape)
{
  if (escape) {
    g_testEscape = true;
  } else {
    g_testButtonDown = false;
  }
  PostMessageW(controlWindow(), WM_NULL, 0, 0);
}

void destShutdown()
{
  std::map<uint32_t, SessionPtr> sessions;
  {
    std::lock_guard lock(g_mutex);
    sessions.swap(g_sessions);
  }
  for (auto &[id, session] : sessions) {
    session->told = true;
    session->cancelled = true;
    session->staging.cancel();
  }
  for (auto &session : g_pendingRemoval) {
    session->staging.removeStaged();
  }
  g_pendingRemoval.clear();
  g_launcherSession.reset();
  if (g_launcher != nullptr) {
    DestroyWindow(g_launcher);
    g_launcher = nullptr;
  }
}

//! Called from the control window's timer
void destRetryRemoval()
{
  for (auto it = g_pendingRemoval.begin(); it != g_pendingRemoval.end();) {
    if ((*it)->staging.removeStaged()) {
      testEvent("staging-removed " + std::to_string((*it)->id));
      it = g_pendingRemoval.erase(it);
    } else {
      ++it;
    }
  }
  if (g_pendingRemoval.empty() || ++g_removalTries > 30) {
    KillTimer(controlWindow(), kRemovalTimer);
  }
}

} // namespace deskflow::filedrag::helper
