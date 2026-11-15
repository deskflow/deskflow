/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

// The macOS side of dragging files between computers. See OSXFileDrag.h for the design and
// deskflow/FileDragTypes.h for the protocol. Compiled with ARC (see CMakeLists.txt).
//
// Threads:
// - core thread: the OSXFileDrag methods (called by OSXComputer), CoreTask events;
// - main thread: every window, view and the AppKit drag (QApplication runs NSApp there);
// - writer thread (destination): staging writes and acks, in the order the messages came;
// - sender threads (source): one FileDragSender per drag that is being sent;
// - promise queue (destination): Finder's writePromiseToURL calls, which wait for the data.
//
// Both roles run on either computer: server to client and client to server.

#include "platform/OSXFileDrag.h"

#include "base/Event.h"
#include "base/EventTypes.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "deskflow/FileDragCodec.h"
#include "deskflow/FileDragEvent.h"
#include "deskflow/FileDragGeometry.h"
#include "deskflow/FileDragPolicy.h"
#include "deskflow/FileDragSender.h"
#include "deskflow/FileDragStaging.h"

#import <AppKit/AppKit.h>
#include <ApplicationServices/ApplicationServices.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include <pthread.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <filesystem>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#if !__has_feature(objc_arc)
#error "OSXFileDrag.mm must be compiled with -fobjc-arc"
#endif

namespace fs = std::filesystem;
namespace fd = deskflow::filedrag;

using State = OSXFileDrag::State;
using StatePtr = std::shared_ptr<State>;
using WeakState = std::weak_ptr<State>;

namespace {

//! Side of the window under the cursor that starts the drag on a secondary
constexpr int32_t kLauncherSide = 64;
//! Side of the window that holds a parked local drag on the primary
constexpr int32_t kCatcherSide = 160;
//! How long the start window waits for its button press
constexpr double kLauncherTimeoutSeconds = 5.0;
//! Delay between showing the start window and pressing on it
constexpr double kLauncherPressDelaySeconds = 0.03;
//! After a drop, how long the target may take before it asks for the first item
constexpr double kNoPromiseTimeoutSeconds = 30.0;
//! A promised item whose data stops arriving for this long fails the drag
constexpr auto kStallTimeout = std::chrono::seconds(120);
//! Image size of a dragged item, in points
constexpr CGFloat kDragImageSide = 64;
//! Pixel size of the icon sent to the other computer
constexpr int kIconPixels = 96;

enum class DestPhase : int
{
  Waiting, ///< start window shown, waiting for the button press
  Looping, ///< the AppKit drag runs
  Dropped, ///< dropped; the target may still be asking for the items
  Done
};

//! A function run on the core thread (posted from another thread as an event)
class CoreTask : public EventData
{
public:
  explicit CoreTask(std::function<void()> task) : m_task(std::move(task))
  {
  }
  ~CoreTask() override = default;

  void run() const
  {
    if (m_task) {
      m_task();
    }
  }

private:
  std::function<void()> m_task;
};

//! What the primary is dragging, ready to send
struct Offer
{
  std::vector<fs::path> paths;
  NSInteger changeCount = -1; ///< of the drag pasteboard, tells one drag from the next
  fd::ItemList items;
  std::vector<fs::path> sources;
  fd::Icon icon;
  bool ready = false;
};

//! Streams the files of one drag on its own thread (primary)
class SourceSession
{
public:
  using Send = std::function<void(fd::Message &&)>;

  SourceSession(uint32_t dragId, const Offer &offer, const Send &send)
      : m_dragId(dragId),
        m_send(send),
        m_sender(dragId, offer.items, offer.sources, send)
  {
  }

  ~SourceSession()
  {
    stop();
    if (m_thread.joinable()) {
      m_thread.join();
    }
  }

  SourceSession(const SourceSession &) = delete;
  SourceSession &operator=(const SourceSession &) = delete;

  void start()
  {
    m_thread = std::thread([this] { run(); });
  }

  void onAck(uint32_t seq)
  {
    {
      std::scoped_lock lock(m_mutex);
      m_acks.push_back(seq);
    }
    m_cond.notify_one();
  }

  void stop()
  {
    {
      std::scoped_lock lock(m_mutex);
      m_stopping = true;
    }
    m_cond.notify_one();
  }

  bool isDone() const
  {
    return m_done;
  }

private:
  void run()
  {
    pthread_setname_np("deskflow file drag sender");
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
      LOG_WARN("file drag %u: sending failed: %s", m_dragId, fd::cancelReasonName(m_sender.failReason()));
      m_send(fd::Message::cancel(m_dragId, m_sender.failReason()));
    } else if (m_sender.isFinished()) {
      LOG_INFO("file drag %u: all %llu bytes sent", m_dragId, static_cast<unsigned long long>(m_sender.bytesSent()));
    } else {
      LOG_DEBUG("file drag %u: sending stopped", m_dragId);
    }
    m_done = true;
  }

  uint32_t m_dragId;
  Send m_send;
  fd::FileDragSender m_sender;
  std::thread m_thread;
  std::mutex m_mutex;
  std::condition_variable m_cond;
  std::deque<uint32_t> m_acks;
  bool m_stopping = false;
  std::atomic<bool> m_done{false};
};

//! One drag being received (secondary)
struct DestSession
{
  explicit DestSession(const fs::path &root) : staging(root)
  {
  }

  uint32_t id = 0;
  fd::FileDragStaging staging;
  fd::Icon icon;
  std::vector<uint32_t> tops; ///< top-level items: one file promise each

  std::atomic<DestPhase> phase{DestPhase::Waiting};
  std::atomic<bool> cancelled{false};
  std::atomic<fd::CancelReason> reason{fd::CancelReason::Unknown};
  std::atomic<bool> told{false}; ///< the server knows how it ended
  std::atomic<bool> finished{false};
  std::atomic<bool> promiseAsked{false};
  int promisesLeft = 0; ///< main thread only
};

using DestPtr = std::shared_ptr<DestSession>;

} // namespace

@class DFFileDragController;

struct OSXFileDrag::State : public std::enable_shared_from_this<OSXFileDrag::State>
{
  IEventQueue *events = nullptr;
  void *eventTarget = nullptr;
  bool primary = true;
  Hooks hooks;

  std::atomic<bool> available{false};
  std::atomic<bool> stopping{false};

  //! main thread only
  DFFileDragController *controller = nil;

  // primary (source) ---------------------------------------------------------------
  //! the next left button release passes to the system (core and event tap threads)
  std::atomic<bool> parkedRelease{false};
  //! the drag that is being handed off or sent (core thread writes, main thread reads)
  std::atomic<uint32_t> currentDragId{0};

  std::mutex sessionsMutex;
  std::map<uint32_t, std::unique_ptr<SourceSession>> sessions;

  // main thread only
  std::shared_ptr<Offer> offer;
  bool overStrip = false;
  bool offered = false;
  bool parked = false;
  uint64_t scanGeneration = 0;
  uint64_t stripGeneration = 0;
  uint64_t parkGeneration = 0;

  // secondary (destination) --------------------------------------------------------
  std::mutex destMutex;
  std::map<uint32_t, DestPtr> dests;
  fd::Icon pendingIcon;
  uint32_t pendingIconId = 0;

  std::thread writer;
  std::mutex writerMutex;
  std::condition_variable writerCond;
  std::deque<fd::Message> writerQueue;
  bool writerStop = false;

  // main thread only
  DestPtr launcherSession;
  uint64_t launcherGeneration = 0;
  DestPtr loopSession;
  std::vector<DestPtr> pendingRemoval;
  int removalTries = 0;

  //! Runs \p task on the core thread
  void postToCore(std::function<void()> task)
  {
    if (stopping) {
      return;
    }
    events->addEvent(Event(EventTypes::FileDrag, this, static_cast<EventData *>(new CoreTask(std::move(task)))));
  }

  //! Posts a FileDragEvent for the server or client
  void postEvent(fd::FileDragEvent *data)
  {
    if (stopping) {
      delete data;
      return;
    }
    events->addEvent(Event(EventTypes::FileDrag, eventTarget, static_cast<EventData *>(data)));
  }

  //! Sends a message to the network peer (through the server or client)
  void sendPeer(const fd::Message &message)
  {
    postEvent(new fd::FileDragEvent(fd::FileDragEvent::Kind::Send, message));
  }
};

namespace {

//
// helpers
//

CGPoint currentCursor()
{
  CGEventRef event = CGEventCreate(nullptr);
  const CGPoint point = event != nullptr ? CGEventGetLocation(event) : CGPointZero;
  if (event != nullptr) {
    CFRelease(event);
  }
  return point;
}

//! Active displays in global display coordinates (points, top left origin)
std::vector<fd::EdgeRect> activeDisplays()
{
  std::vector<fd::EdgeRect> displays;
  uint32_t count = 0;
  if (CGGetActiveDisplayList(0, nullptr, &count) != kCGErrorSuccess || count == 0) {
    return displays;
  }
  std::vector<CGDirectDisplayID> ids(count);
  if (CGGetActiveDisplayList(count, ids.data(), &count) != kCGErrorSuccess) {
    return displays;
  }
  for (uint32_t i = 0; i < count; ++i) {
    const CGRect b = CGDisplayBounds(ids[i]);
    displays.push_back(
        {static_cast<int32_t>(b.origin.x), static_cast<int32_t>(b.origin.y), static_cast<int32_t>(b.size.width),
         static_cast<int32_t>(b.size.height)}
    );
  }
  return displays;
}

//! Window frame (bottom left origin) for a rectangle in global display coordinates
NSRect cocoaFrame(const fd::EdgeRect &r)
{
  const auto mainHeight = static_cast<int32_t>(CGDisplayBounds(CGMainDisplayID()).size.height);
  const auto f = fd::flipVertical(r, mainHeight);
  return NSMakeRect(f.x, f.y, f.w, f.h);
}

void dispatchMain(dispatch_block_t block)
{
  dispatch_async(dispatch_get_main_queue(), block);
}

void dispatchMainAfter(double seconds, dispatch_block_t block)
{
  dispatch_after(
      dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(seconds * NSEC_PER_SEC)), dispatch_get_main_queue(), block
  );
}

std::string toUtf8(NSString *s)
{
  const char *utf8 = s.UTF8String;
  return utf8 != nullptr ? std::string(utf8) : std::string();
}

NSString *toNSString(const std::string &s)
{
  NSString *result = [NSString stringWithUTF8String:s.c_str()];
  return result != nil ? result : @"";
}

/**
 * The files and folders on a drag pasteboard, as paths. Finder may put file reference URLs
 * ("file:///.file/id=...") there; they are resolved, and skipped if that fails.
 */
std::vector<fs::path> filePathsOnPasteboard(NSPasteboard *pasteboard)
{
  std::vector<fs::path> paths;
  NSArray<NSURL *> *urls = [pasteboard readObjectsForClasses:@[ NSURL.class ]
                                                     options:@{
                                                       NSPasteboardURLReadingFileURLsOnlyKey : @YES
                                                     }];
  for (NSURL *url in urls) {
    NSURL *resolved = url.filePathURL;
    if (resolved == nil) {
      LOG_WARN("a dragged item's file reference cannot be resolved; it is left out");
      continue;
    }
    const char *fsPath = resolved.fileSystemRepresentation;
    if (fsPath == nullptr || *fsPath == '\0') {
      continue;
    }
    const std::string path(fsPath);
    if (fd::isFileReferencePath(path)) {
      LOG_WARN("a dragged item is still a file reference after resolving; it is left out");
      continue;
    }
    paths.emplace_back(path);
  }
  return paths;
}

//! Drag image of a file or folder, RGBA with straight alpha (as the wire carries it)
fd::Icon iconForPath(const fs::path &path)
{
  fd::Icon icon;
  NSImage *image = [[NSWorkspace sharedWorkspace] iconForFile:toNSString(path.string())];
  if (image == nil) {
    return icon;
  }
  NSRect rect = NSMakeRect(0, 0, kIconPixels, kIconPixels);
  CGImageRef cgImage = [image CGImageForProposedRect:&rect context:nil hints:nil];
  if (cgImage == nullptr) {
    return icon;
  }
  std::string rgba(static_cast<size_t>(kIconPixels) * kIconPixels * 4, '\0');
  CGColorSpaceRef space = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
  CGContextRef context = CGBitmapContextCreate(
      rgba.data(), kIconPixels, kIconPixels, 8, kIconPixels * 4, space,
      static_cast<uint32_t>(kCGImageAlphaPremultipliedLast) | static_cast<uint32_t>(kCGBitmapByteOrder32Big)
  );
  CGColorSpaceRelease(space);
  if (context == nullptr) {
    return icon;
  }
  CGContextSetInterpolationQuality(context, kCGInterpolationHigh);
  CGContextDrawImage(context, CGRectMake(0, 0, kIconPixels, kIconPixels), cgImage);
  CGContextRelease(context);
  // bitmap contexts are premultiplied and keep the top row first in memory
  fd::unpremultiplyRgba(rgba);
  icon.width = kIconPixels;
  icon.height = kIconPixels;
  icon.rgba = std::move(rgba);
  return icon;
}

//! The icon the other computer sent, as a drag image; nil if there is none
NSImage *imageFromIcon(const fd::Icon &icon)
{
  if (icon.isEmpty() || !icon.isValid()) {
    return nil;
  }
  NSBitmapImageRep *rep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:nullptr
                                                                  pixelsWide:icon.width
                                                                  pixelsHigh:icon.height
                                                               bitsPerSample:8
                                                             samplesPerPixel:4
                                                                    hasAlpha:YES
                                                                    isPlanar:NO
                                                              colorSpaceName:NSDeviceRGBColorSpace
                                                                bitmapFormat:NSBitmapFormatAlphaNonpremultiplied
                                                                 bytesPerRow:icon.width * 4
                                                                bitsPerPixel:32];
  if (rep == nil || rep.bitmapData == nullptr) {
    return nil;
  }
  memcpy(rep.bitmapData, icon.rgba.data(), static_cast<size_t>(icon.width) * icon.height * 4);
  NSImage *image = [[NSImage alloc] initWithSize:NSMakeSize(kDragImageSide, kDragImageSide * icon.height / icon.width)];
  [image addRepresentation:rep];
  return image;
}

//! The system's generic file or folder icon, until (or if no) real icon is known
NSImage *placeholderImage(bool folder)
{
  NSImage *image = [[[NSWorkspace sharedWorkspace] iconForContentType:folder ? UTTypeFolder : UTTypeData] copy];
  image.size = NSMakeSize(kDragImageSide, kDragImageSide);
  return image;
}

//! The type a file promise announces for a staged item
NSString *typeIdentifierFor(const fd::FileDragStaging::Entry &entry)
{
  if (entry.isDirectory) {
    return UTTypeFolder.identifier;
  }
  NSString *extension = toNSString(entry.localPath).pathExtension;
  UTType *type = extension.length > 0 ? [UTType typeWithFilenameExtension:extension] : nil;
  if (type == nil || ![type conformsToType:UTTypeData]) {
    type = UTTypeData;
  }
  return type.identifier;
}

NSError *promiseError(NSString *what)
{
  return [NSError errorWithDomain:@"org.deskflow.filedrag" code:1 userInfo:@{NSLocalizedDescriptionKey : what}];
}

// forward declarations (main thread unless noted)
void offerReady(StatePtr s);
void offerLost(StatePtr s);
void onFilesEntered(StatePtr s, std::vector<fs::path> paths, NSInteger changeCount);
void unpark(StatePtr s, const char *why);
void startDrag(StatePtr s, DestPtr session, NSView *view, NSEvent *event);
void loopEnded(StatePtr s, NSDragOperation operation);
void cancelSession(StatePtr s, DestPtr session, fd::CancelReason reason, bool tellServer); // any thread
void writePromise(
    StatePtr s, uint32_t dragId, uint32_t index, NSURL *url, void (^completion)(NSError *)
);                                         // promise queue
DestPtr findDest(StatePtr s, uint32_t id); // any thread

} // namespace

//
// windows
//

//! Borderless, never key, on every Space (full screen ones too), above the Dock and menu bar
@interface DFOverlayPanel : NSPanel
- (instancetype)initWithView:(NSView *)view;
- (void)placeAt:(const fd::EdgeRect &)rect;
@end

@implementation DFOverlayPanel

- (instancetype)initWithView:(NSView *)view
{
  self = [super initWithContentRect:NSMakeRect(0, 0, 4, 4)
                          styleMask:NSWindowStyleMaskBorderless | NSWindowStyleMaskNonactivatingPanel
                            backing:NSBackingStoreBuffered
                              defer:NO];
  if (self != nil) {
    self.releasedWhenClosed = NO;
    self.opaque = NO;
    self.hasShadow = NO;
    // all but invisible, yet not fully transparent: the window server passes clicks and
    // drags through pixels whose alpha is exactly 0
    self.backgroundColor = [NSColor colorWithCalibratedWhite:0.5 alpha:0.005];
    self.collectionBehavior = NSWindowCollectionBehaviorCanJoinAllSpaces |
                              NSWindowCollectionBehaviorFullScreenAuxiliary | NSWindowCollectionBehaviorStationary |
                              NSWindowCollectionBehaviorIgnoresCycle;
    // panels hide when their app is not active by default, and this app never is
    self.hidesOnDeactivate = NO;
    self.becomesKeyOnlyIfNeeded = YES;
    // set last: floatingPanel and friends reset the level
    self.level = NSScreenSaverWindowLevel;
    self.animationBehavior = NSWindowAnimationBehaviorNone;
    self.ignoresMouseEvents = NO;
    self.contentView = view;
  }
  return self;
}

- (BOOL)canBecomeKeyWindow
{
  return NO;
}

- (BOOL)canBecomeMainWindow
{
  return NO;
}

- (void)placeAt:(const fd::EdgeRect &)rect
{
  [self setFrame:cocoaFrame(rect) display:NO];
  self.alphaValue = 1.0;
  self.ignoresMouseEvents = NO;
  [self orderFrontRegardless];
}

@end

/**
 * Primary: an edge window (notices a drag of files at a switch edge) or the catcher (holds
 * the local drag while the cursor is on another computer). Neither ever takes a drop.
 */
@interface DFDropView : NSView
- (instancetype)initWithState:(const StatePtr &)state catcher:(BOOL)catcher;
- (void)dragLeft:(const char *)why;
@end

@implementation DFDropView {
  WeakState _state;
  BOOL _catcher;
  BOOL _files;
}

- (instancetype)initWithState:(const StatePtr &)state catcher:(BOOL)catcher
{
  self = [super initWithFrame:NSMakeRect(0, 0, 4, 4)];
  if (self != nil) {
    _state = state;
    _catcher = catcher;
    [self registerForDraggedTypes:@[ NSPasteboardTypeFileURL ]];
  }
  return self;
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender
{
  auto s = _state.lock();
  if (!s || _catcher) {
    // the parked local drag must never drop anything here
    return NSDragOperationNone;
  }
  NSPasteboard *pasteboard = sender.draggingPasteboard;
  auto paths = filePathsOnPasteboard(pasteboard);
  _files = !paths.empty();
  if (!_files) {
    // pictures, text, links, promised files...: only files may cross, so this drag stays here
    LOG_INFO("a drag without files reached the edge window; it stays on this computer");
    return NSDragOperationNone;
  }
  LOG_INFO("a drag of %zu files or folders reached the edge window", paths.size());
  onFilesEntered(s, std::move(paths), pasteboard.changeCount);
  return sender.draggingSourceOperationMask & NSDragOperationCopy;
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender
{
  return (!_catcher && _files) ? (sender.draggingSourceOperationMask & NSDragOperationCopy) : NSDragOperationNone;
}

- (void)draggingExited:(id<NSDraggingInfo>)sender
{
  [self dragLeft:"left the catcher"];
}

- (BOOL)prepareForDragOperation:(id<NSDraggingInfo>)sender
{
  // the source must never move or copy anything because of us
  [self dragLeft:"let go"];
  return NO;
}

- (void)draggingEnded:(id<NSDraggingInfo>)sender
{
  [self dragLeft:"drag ended"];
}

- (void)dragLeft:(const char *)why
{
  auto s = _state.lock();
  if (!s) {
    return;
  }
  if (_catcher) {
    unpark(s, why);
    return;
  }
  if (_files) {
    s->overStrip = false;
    offerLost(s);
  }
  _files = NO;
}

@end

//! Secondary: under the cursor, gets the synthetic press and starts the drag from it
@interface DFLauncherView : NSView
- (instancetype)initWithState:(const StatePtr &)state;
@end

@implementation DFLauncherView {
  WeakState _state;
}

- (instancetype)initWithState:(const StatePtr &)state
{
  self = [super initWithFrame:NSMakeRect(0, 0, kLauncherSide, kLauncherSide)];
  if (self != nil) {
    _state = state;
  }
  return self;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)event
{
  return YES;
}

- (BOOL)mouseDownCanMoveWindow
{
  return NO;
}

- (void)mouseDown:(NSEvent *)event
{
  auto s = _state.lock();
  if (!s) {
    return;
  }
  DestPtr session = std::move(s->launcherSession);
  s->launcherSession.reset();
  ++s->launcherGeneration;
  if (!session || session->phase != DestPhase::Waiting || session->cancelled) {
    LOG_DEBUG("file drag: the start window was pressed with no drag waiting");
    [self.window orderOut:nil];
    return;
  }
  LOG_INFO("file drag %u: the press reached the start window", session->id);
  startDrag(s, session, self, event);
}

@end

//! Secondary: ends a cancelled drag by refusing it, so it drops nowhere
@interface DFRefuseView : NSView
@end

@implementation DFRefuseView

- (instancetype)init
{
  self = [super initWithFrame:NSMakeRect(0, 0, kLauncherSide, kLauncherSide)];
  if (self != nil) {
    NSArray<NSPasteboardType> *types =
        [NSFilePromiseReceiver.readableDraggedTypes arrayByAddingObject:NSPasteboardTypeFileURL];
    [self registerForDraggedTypes:types];
  }
  return self;
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender
{
  return NSDragOperationNone;
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender
{
  return NSDragOperationNone;
}

- (BOOL)prepareForDragOperation:(id<NSDraggingInfo>)sender
{
  return NO;
}

@end

//
// controller: owns the windows, is the drag source and the file promise delegate
//

@interface DFFileDragController : NSObject <NSDraggingSource, NSFilePromiseProviderDelegate>
- (instancetype)initWithState:(const StatePtr &)state;
- (void)placeStrips:(const std::vector<fd::EdgeRect> &)strips;
- (void)hideStrips;
- (void)showCatcher:(const fd::EdgeRect &)rect;
- (void)hideCatcher;
- (void)showLauncher:(const fd::EdgeRect &)rect;
- (void)launcherOutOfTheWay;
- (void)hideLauncher;
- (void)showRefuser:(const fd::EdgeRect &)rect;
- (void)hideRefuser;
- (void)teardown;
@end

@implementation DFFileDragController {
  WeakState _state;
  NSMutableArray<DFOverlayPanel *> *_strips;
  DFOverlayPanel *_catcher;
  DFOverlayPanel *_launcher;
  DFOverlayPanel *_refuser;
  NSOperationQueue *_promiseQueue;
}

- (instancetype)initWithState:(const StatePtr &)state
{
  self = [super init];
  if (self != nil) {
    _state = state;
    _strips = [NSMutableArray array];
    _promiseQueue = [[NSOperationQueue alloc] init];
    _promiseQueue.name = @"org.deskflow.filedrag.promises";
    _promiseQueue.qualityOfService = NSQualityOfServiceUserInitiated;
    _promiseQueue.maxConcurrentOperationCount = 4;
  }
  return self;
}

// primary windows

- (void)placeStrips:(const std::vector<fd::EdgeRect> &)strips
{
  auto s = _state.lock();
  if (!s) {
    return;
  }
  while (_strips.count < strips.size()) {
    DFDropView *view = [[DFDropView alloc] initWithState:s catcher:NO];
    [_strips addObject:[[DFOverlayPanel alloc] initWithView:view]];
  }
  for (NSUInteger i = 0; i < _strips.count; ++i) {
    if (i < strips.size()) {
      [_strips[i] placeAt:strips[i]];
    } else {
      [_strips[i] orderOut:nil];
    }
  }
}

- (void)hideStrips
{
  for (DFOverlayPanel *strip in _strips) {
    [strip orderOut:nil];
  }
}

- (void)showCatcher:(const fd::EdgeRect &)rect
{
  auto s = _state.lock();
  if (!s) {
    return;
  }
  if (_catcher == nil) {
    _catcher = [[DFOverlayPanel alloc] initWithView:[[DFDropView alloc] initWithState:s catcher:YES]];
    // above the edge windows, so the parked drag is over the catcher
    _catcher.level = NSScreenSaverWindowLevel + 1;
  }
  [_catcher placeAt:rect];
}

- (void)hideCatcher
{
  [_catcher orderOut:nil];
}

// secondary windows

- (void)showLauncher:(const fd::EdgeRect &)rect
{
  auto s = _state.lock();
  if (!s) {
    return;
  }
  if (_launcher == nil) {
    _launcher = [[DFOverlayPanel alloc] initWithView:[[DFLauncherView alloc] initWithState:s]];
  }
  [_launcher placeAt:rect];
}

- (void)launcherOutOfTheWay
{
  // the drag started from this window, so it stays until the drag ends, but neither the
  // pointer nor a drop may land on it
  _launcher.ignoresMouseEvents = YES;
  _launcher.alphaValue = 0.0;
  const NSRect corner = NSMakeRect(NSMinX(_launcher.frame), NSMinY(_launcher.frame), 1, 1);
  [_launcher setFrame:corner display:NO];
}

- (void)hideLauncher
{
  [_launcher orderOut:nil];
}

- (void)showRefuser:(const fd::EdgeRect &)rect
{
  if (_refuser == nil) {
    _refuser = [[DFOverlayPanel alloc] initWithView:[[DFRefuseView alloc] init]];
    _refuser.level = NSScreenSaverWindowLevel + 1;
  }
  [_refuser placeAt:rect];
}

- (void)hideRefuser
{
  [_refuser orderOut:nil];
}

- (void)teardown
{
  [self hideStrips];
  [_strips removeAllObjects];
  [_catcher orderOut:nil];
  _catcher = nil;
  [_launcher orderOut:nil];
  _launcher = nil;
  [_refuser orderOut:nil];
  _refuser = nil;
  [_promiseQueue cancelAllOperations];
}

// NSDraggingSource

- (NSDragOperation)draggingSession:(NSDraggingSession *)session
    sourceOperationMaskForDraggingContext:(NSDraggingContext)context
{
  return NSDragOperationCopy;
}

- (void)draggingSession:(NSDraggingSession *)session willBeginAtPoint:(NSPoint)screenPoint
{
  [self launcherOutOfTheWay];
}

- (void)draggingSession:(NSDraggingSession *)session
           endedAtPoint:(NSPoint)screenPoint
              operation:(NSDragOperation)operation
{
  auto s = _state.lock();
  [self hideLauncher];
  [self hideRefuser];
  if (s) {
    loopEnded(s, operation);
  }
}

- (BOOL)ignoreModifierKeysForDraggingSession:(NSDraggingSession *)session
{
  return YES;
}

// NSFilePromiseProviderDelegate

- (NSString *)filePromiseProvider:(NSFilePromiseProvider *)filePromiseProvider fileNameForType:(NSString *)fileType
{
  auto s = _state.lock();
  NSDictionary *info = filePromiseProvider.userInfo;
  const auto dragId = static_cast<uint32_t>([info[@"drag"] unsignedIntValue]);
  const auto index = static_cast<uint32_t>([info[@"index"] unsignedIntValue]);
  DestPtr session = s ? findDest(s, dragId) : nullptr;
  if (!session || index >= session->staging.entries().size()) {
    return @"Deskflow item";
  }
  NSString *name = toNSString(session->staging.entries()[index].localPath);
  return name.length > 0 ? name : @"Deskflow item";
}

- (NSOperationQueue *)operationQueueForFilePromiseProvider:(NSFilePromiseProvider *)filePromiseProvider
{
  return _promiseQueue;
}

- (void)filePromiseProvider:(NSFilePromiseProvider *)filePromiseProvider
          writePromiseToURL:(NSURL *)url
          completionHandler:(void (^)(NSError *))completionHandler
{
  auto s = _state.lock();
  if (!s) {
    completionHandler(promiseError(@"Deskflow stopped before the files arrived."));
    return;
  }
  NSDictionary *info = filePromiseProvider.userInfo;
  const auto dragId = static_cast<uint32_t>([info[@"drag"] unsignedIntValue]);
  const auto index = static_cast<uint32_t>([info[@"index"] unsignedIntValue]);
  writePromise(s, dragId, index, url, completionHandler);
}

@end

namespace {

DFFileDragController *controllerOf(StatePtr s)
{
  if (s->controller == nil && !s->stopping) {
    s->controller = [[DFFileDragController alloc] initWithState:s];
  }
  return s->controller;
}

void postLoopEnded(StatePtr s, uint32_t dragId, bool dropped)
{
  s->postToCore([s, dragId, dropped] {
    if (s->hooks.loopEnded) {
      s->hooks.loopEnded(dragId, dropped);
    }
    fd::Message message;
    message.dragId = dragId;
    s->postEvent(new fd::FileDragEvent(fd::FileDragEvent::Kind::LoopEnded, std::move(message), dropped));
  });
}

//! Sending \p dragId stopped here; its cancel has already been posted
void postSourceStopped(StatePtr s, uint32_t dragId)
{
  s->postToCore([s, dragId] {
    if (s->hooks.sourceStopped) {
      s->hooks.sourceStopped(dragId);
    }
  });
}

//
// source (files leave this computer), main thread
//

void offerReady(StatePtr s)
{
  if (!s->offered) {
    s->offered = true;
    LOG_INFO("file drag offer ready (%zu items)", s->offer ? s->offer->items.size() : size_t{0});
    s->postEvent(new fd::FileDragEvent(fd::FileDragEvent::Kind::OfferReady));
  }
}

void offerLost(StatePtr s)
{
  if (s->offered) {
    s->offered = false;
    s->postEvent(new fd::FileDragEvent(fd::FileDragEvent::Kind::OfferLost));
  }
}

//! A drag of files reached an edge window: build its item list (on a worker thread)
void onFilesEntered(StatePtr s, std::vector<fs::path> paths, NSInteger changeCount)
{
  s->overStrip = true;
  if (s->offer && s->offer->changeCount == changeCount && s->offer->paths == paths) {
    if (s->offer->ready) {
      offerReady(s);
    }
    return; // the same drag; still scanning
  }

  auto offer = std::make_shared<Offer>();
  offer->paths = std::move(paths);
  offer->changeCount = changeCount;
  s->offer = offer;
  const uint64_t generation = ++s->scanGeneration;
  WeakState weak = s;
  dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
    auto scanned = std::make_shared<Offer>();
    std::string error;
    const bool ok = fd::scanItems(offer->paths, scanned->items, scanned->sources, &error);
    dispatchMain(^{
      auto state = weak.lock();
      if (!state || generation != state->scanGeneration || state->offer != offer) {
        return;
      }
      if (!ok) {
        LOG_WARN("cannot carry these files to another computer: %s", error.c_str());
        state->offer.reset();
        return;
      }
      offer->items = std::move(scanned->items);
      offer->sources = std::move(scanned->sources);
      offer->icon = offer->paths.empty() ? fd::Icon{} : iconForPath(offer->paths.front());
      offer->ready = true;
      if (state->overStrip) {
        offerReady(state);
      }
    });
  });
}

void watchEdgesMain(StatePtr s, uint32_t sides, const fd::EdgeRect &desktop)
{
  DFFileDragController *c = controllerOf(s);
  const uint64_t generation = ++s->stripGeneration;
  if (sides == 0) {
    if (s->overStrip && s->offered) {
      // the files are over an edge window and a hand-off usually follows at once: keep the
      // window under them until it does (the hand-off puts the catcher there first)
      dispatchMainAfter(0.5, ^{
        if (generation == s->stripGeneration) {
          [s->controller hideStrips];
        }
      });
      return;
    }
    [c hideStrips];
    return;
  }

  const auto displays = activeDisplays();
  std::vector<fd::EdgeRect> strips;
  for (const Direction side : {Direction::Left, Direction::Right, Direction::Top, Direction::Bottom}) {
    const auto part = fd::edgeStripsOnDisplays(side, sides, desktop, displays);
    strips.insert(strips.end(), part.begin(), part.end());
  }
  LOG_DEBUG("file drag: %zu edge windows for sides 0x%x", strips.size(), sides);
  [c placeStrips:strips];
}

void checkParkedButton(StatePtr s, uint64_t generation)
{
  if (!s->parked || generation != s->parkGeneration) {
    return;
  }
  // the combined session state only sees the release once it reached the system, i.e. once
  // it was let through to end the parked drag over the catcher
  if (!CGEventSourceButtonState(kCGEventSourceStateCombinedSessionState, kCGMouseButtonLeft)) {
    // let the drag finish over the catcher before it goes
    dispatchMainAfter(1.0, ^{
      if (generation == s->parkGeneration) {
        unpark(s, "button is up");
      }
    });
    return;
  }
  dispatchMainAfter(0.5, ^{
    checkParkedButton(s, generation);
  });
}

void unpark(StatePtr s, const char *why)
{
  if (s->parked) {
    LOG_DEBUG("file drag: local drag released from the catcher: %s", why);
  }
  s->parked = false;
  ++s->parkGeneration;
  [s->controller hideCatcher];
}

//! Makes the local drag look at what is under it again (the catcher), without moving it
void nudgeLocalDrag()
{
  const CGPoint at = currentCursor();
  CGEventRef event = CGEventCreateMouseEvent(nullptr, kCGEventLeftMouseDragged, at, kCGMouseButtonLeft);
  if (event != nullptr) {
    // posted after the HID event tap, so the core's own tap does not swallow it
    CGEventPost(kCGSessionEventTap, event);
    CFRelease(event);
  }
}

void stopSourceSession(StatePtr s, uint32_t dragId)
{
  uint32_t expected = dragId;
  s->currentDragId.compare_exchange_strong(expected, 0);
  std::unique_ptr<SourceSession> session;
  {
    std::scoped_lock lock(s->sessionsMutex);
    auto it = s->sessions.find(dragId);
    if (it == s->sessions.end()) {
      return;
    }
    session = std::move(it->second);
    s->sessions.erase(it);
  }
  // joins the sender thread outside the lock
  session.reset();
}

void pruneSourceSessions(StatePtr s)
{
  std::vector<std::unique_ptr<SourceSession>> done;
  std::scoped_lock lock(s->sessionsMutex);
  for (auto it = s->sessions.begin(); it != s->sessions.end();) {
    if (it->second->isDone()) {
      done.push_back(std::move(it->second));
      it = s->sessions.erase(it);
    } else {
      ++it;
    }
  }
}

void handOffMain(StatePtr s, uint32_t dragId, int32_t parkX, int32_t parkY)
{
  DFFileDragController *c = controllerOf(s);
  pruneSourceSessions(s);

  // park the local drag over the catcher first: letting go there drops nothing
  const auto displays = activeDisplays();
  [c showCatcher:fd::squareAround(parkX, parkY, kCatcherSide, fd::displayAt(parkX, parkY, displays))];
  ++s->stripGeneration;
  [c hideStrips];
  nudgeLocalDrag();
  s->parked = true;
  const uint64_t generation = ++s->parkGeneration;
  dispatchMainAfter(0.5, ^{
    checkParkedButton(s, generation);
  });
  s->offered = false;
  s->overStrip = false;

  if (s->currentDragId != dragId) {
    LOG_INFO("file drag %u: taken back before it was sent", dragId);
    return;
  }
  if (!s->offer || !s->offer->ready) {
    LOG_WARN("file drag %u: nothing to send", dragId);
    s->sendPeer(fd::Message::cancel(dragId, fd::CancelReason::Unsupported));
    postSourceStopped(s, dragId);
    return;
  }

  const Offer &offer = *s->offer;
  LOG_INFO("file drag %u: sending %zu items", dragId, offer.items.size());
  if (!offer.icon.isEmpty()) {
    fd::Message icon;
    icon.type = fd::Message::Type::Icon;
    icon.dragId = dragId;
    icon.icon = offer.icon;
    s->sendPeer(icon);
  }
  fd::Message begin;
  begin.type = fd::Message::Type::Begin;
  begin.dragId = dragId;
  begin.items = offer.items;
  s->sendPeer(begin);

  WeakState weak = s;
  auto session = std::make_unique<SourceSession>(dragId, offer, [weak](fd::Message &&message) {
    if (auto state = weak.lock()) {
      const bool stopped = message.type == fd::Message::Type::Cancel;
      const uint32_t id = message.dragId;
      state->sendPeer(message);
      if (stopped) {
        postSourceStopped(state, id);
      }
    }
  });
  SourceSession *raw = session.get();
  {
    std::scoped_lock lock(s->sessionsMutex);
    s->sessions[dragId] = std::move(session);
  }
  raw->start();
  // the next drag scans afresh (the files may change in between)
  s->offer.reset();
}

//
// destination (files come to this computer)
//

DestPtr findDest(StatePtr s, uint32_t id)
{
  std::scoped_lock lock(s->destMutex);
  auto it = s->dests.find(id);
  return it == s->dests.end() ? nullptr : it->second;
}

void eraseDest(StatePtr s, uint32_t id)
{
  std::scoped_lock lock(s->destMutex);
  s->dests.erase(id);
}

void retryRemoval(StatePtr s)
{
  for (auto it = s->pendingRemoval.begin(); it != s->pendingRemoval.end();) {
    if ((*it)->staging.removeStaged()) {
      it = s->pendingRemoval.erase(it);
    } else {
      ++it;
    }
  }
  if (!s->pendingRemoval.empty() && ++s->removalTries <= 30) {
    dispatchMainAfter(2.0, ^{
      retryRemoval(s);
    });
  }
}

//! Main thread: every promised item is in place (or nothing was promised)
void finishSession(StatePtr s, DestPtr session)
{
  if (session->finished.exchange(true)) {
    return;
  }
  session->phase = DestPhase::Done;
  if (!session->told.exchange(true)) {
    s->sendPeer(fd::Message::finished(session->id));
  }
  LOG_INFO("file drag %u: the files are in place", session->id);
  // the items were moved out; this empties the staging folder
  if (!session->staging.removeStaged()) {
    s->pendingRemoval.push_back(session);
    s->removalTries = 0;
    dispatchMainAfter(2.0, ^{
      retryRemoval(s);
    });
  }
  eraseDest(s, session->id);
}

void onCancelledMain(StatePtr s, DestPtr session)
{
  switch (session->phase.load()) {
  case DestPhase::Waiting:
    if (s->launcherSession == session) {
      [s->controller hideLauncher];
      s->launcherSession.reset();
      ++s->launcherGeneration;
    }
    session->phase = DestPhase::Done;
    eraseDest(s, session->id);
    postLoopEnded(s, session->id, false);
    break;

  case DestPhase::Looping: {
    // there is no call that cancels an AppKit drag: end it over a window that refuses it,
    // so it drops nowhere (draggingSession:endedAtPoint: then finishes the cancel)
    const CGPoint at = currentCursor();
    const auto x = static_cast<int32_t>(at.x);
    const auto y = static_cast<int32_t>(at.y);
    [controllerOf(s) showRefuser:fd::squareAround(x, y, kLauncherSide, fd::displayAt(x, y, activeDisplays()))];
    s->postToCore([s, x, y] {
      if (s->hooks.releaseAt) {
        s->hooks.releaseAt(x, y);
      }
    });
    dispatchMainAfter(3.0, ^{
      if (s->loopSession == session) {
        LOG_ERR("file drag %u: the drag did not end after it was cancelled", session->id);
      }
    });
    break;
  }

  case DestPhase::Dropped:
    // a promise still waiting fails on its next wait
    session->phase = DestPhase::Done;
    eraseDest(s, session->id);
    break;

  case DestPhase::Done:
    break;
  }
}

void cancelSession(StatePtr s, DestPtr session, fd::CancelReason reason, bool tellServer)
{
  if (session->cancelled.exchange(true)) {
    return;
  }
  session->reason = reason;
  session->staging.cancel();
  LOG_INFO("file drag %u cancelled: %s", session->id, fd::cancelReasonName(reason));
  if (!session->told.exchange(true) && tellServer) {
    s->sendPeer(fd::Message::cancel(session->id, reason));
  }
  dispatchMain(^{
    onCancelledMain(s, session);
  });
}

void showLauncherMain(StatePtr s, DestPtr session)
{
  if (session->cancelled) {
    return;
  }
  DFFileDragController *c = controllerOf(s);
  if (s->launcherSession && s->launcherSession->phase == DestPhase::Waiting) {
    // an older drag never got its button press
    cancelSession(s, s->launcherSession, fd::CancelReason::Busy, true);
  }
  const CGPoint at = currentCursor();
  const auto x = static_cast<int32_t>(at.x);
  const auto y = static_cast<int32_t>(at.y);
  [c showLauncher:fd::squareAround(x, y, kLauncherSide, fd::displayAt(x, y, activeDisplays()))];
  s->launcherSession = session;
  const uint64_t generation = ++s->launcherGeneration;
  LOG_DEBUG("file drag %u: start window up at %d,%d", session->id, x, y);

  dispatchMainAfter(kLauncherPressDelaySeconds, ^{
    if (generation != s->launcherGeneration || session->cancelled) {
      return;
    }
    const uint32_t id = session->id;
    s->postToCore([s, id, x, y] {
      if (s->hooks.launcherReady) {
        s->hooks.launcherReady(id, x, y);
      }
    });
  });

  dispatchMainAfter(kLauncherTimeoutSeconds, ^{
    if (generation != s->launcherGeneration || s->launcherSession != session || session->phase != DestPhase::Waiting) {
      return;
    }
    LOG_WARN(
        "file drag %u: the drag never started under the cursor (the press did not reach the start window; "
        "check that Deskflow may control this computer in Privacy & Security > Accessibility)",
        session->id
    );
    [s->controller hideLauncher];
    s->launcherSession.reset();
    ++s->launcherGeneration;
    cancelSession(s, session, fd::CancelReason::Timeout, true);
  });
}

void startDrag(StatePtr s, DestPtr session, NSView *view, NSEvent *event)
{
  DFFileDragController *c = controllerOf(s);
  session->phase = DestPhase::Looping;
  s->loopSession = session;

  fd::Icon icon = session->icon;
  if (icon.isEmpty()) {
    std::scoped_lock lock(s->destMutex);
    if (s->pendingIconId == session->id) {
      icon = s->pendingIcon;
    }
  }
  const auto &entries = session->staging.entries();
  NSImage *image = imageFromIcon(icon);

  const NSPoint at = [view convertPoint:event.locationInWindow fromView:nil];
  NSMutableArray<NSDraggingItem *> *items = [NSMutableArray array];
  int n = 0;
  for (const uint32_t top : session->tops) {
    const auto &entry = entries[top];
    NSFilePromiseProvider *provider = [[NSFilePromiseProvider alloc] initWithFileType:typeIdentifierFor(entry)
                                                                             delegate:c];
    provider.userInfo = @{@"drag" : @(session->id), @"index" : @(top)};
    NSDraggingItem *item = [[NSDraggingItem alloc] initWithPasteboardWriter:provider];
    NSImage *contents = (n == 0 && image != nil) ? image : placeholderImage(entry.isDirectory);
    const CGFloat offset = std::min(n, 3) * 4.0;
    [item setDraggingFrame:NSMakeRect(
                               at.x - kDragImageSide / 2 + offset, at.y - kDragImageSide / 2 - offset, kDragImageSide,
                               kDragImageSide
                           )
                  contents:contents];
    [items addObject:item];
    ++n;
  }
  session->promisesLeft = static_cast<int>(items.count);

  NSDraggingSession *drag = items.count > 0 ? [view beginDraggingSessionWithItems:items event:event source:c] : nil;
  if (drag == nil) {
    LOG_ERR("file drag %u: AppKit did not start the drag", session->id);
    s->loopSession.reset();
    [c hideLauncher];
    session->phase = DestPhase::Done;
    cancelSession(s, session, fd::CancelReason::Unsupported, true);
    eraseDest(s, session->id);
    postLoopEnded(s, session->id, false);
    return;
  }
  drag.animatesToStartingPositionsOnCancelOrFail = NO;
  drag.draggingFormation = NSDraggingFormationPile;
  LOG_INFO("file drag %u: the drag runs (%lu items)", session->id, static_cast<unsigned long>(items.count));
}

void loopEnded(StatePtr s, NSDragOperation operation)
{
  DestPtr session = std::move(s->loopSession);
  s->loopSession.reset();
  if (!session) {
    return;
  }
  const bool dropped = operation != NSDragOperationNone && !session->cancelled;
  if (dropped) {
    LOG_INFO("file drag %u dropped", session->id);
    session->phase = DestPhase::Dropped;
    s->sendPeer(fd::Message::dropped(session->id));
    if (session->promisesLeft <= 0) {
      finishSession(s, session);
    } else {
      dispatchMainAfter(kNoPromiseTimeoutSeconds, ^{
        if (!session->promiseAsked && session->phase == DestPhase::Dropped && !session->cancelled) {
          LOG_WARN("file drag %u: the drop target never asked for the files", session->id);
          cancelSession(s, session, fd::CancelReason::DropFailed, true);
        }
      });
    }
  } else {
    auto reason = fd::CancelReason::DropFailed;
    if (session->cancelled) {
      reason = session->reason;
    } else {
      LOG_INFO("file drag %u: let go where nothing takes the files", session->id);
    }
    cancelSession(s, session, reason, true);
    session->phase = DestPhase::Done;
    eraseDest(s, session->id);
  }
  postLoopEnded(s, session->id, dropped);
}

//! Promise queue: Finder asked for top-level item \p index at \p url (it was dropped there)
void writePromise(StatePtr s, uint32_t dragId, uint32_t index, NSURL *url, void (^completion)(NSError *))
{
  DestPtr session = findDest(s, dragId);
  if (!session || session->cancelled) {
    completion(promiseError(@"The drag was cancelled before the files arrived."));
    return;
  }
  if (index >= session->staging.entries().size()) {
    completion(promiseError(@"Unknown dragged item."));
    return;
  }
  session->promiseAsked = true;
  const auto &entry = session->staging.entries()[index];
  LOG_INFO("file drag %u: the drop target asks for \"%s\"", dragId, entry.localPath.c_str());

  // the data may still be on its way: wait for every byte of this item
  using WaitResult = fd::FileDragStaging::WaitResult;
  auto lastBytes = session->staging.receivedBytes();
  auto lastProgress = std::chrono::steady_clock::now();
  auto result = WaitResult::Timeout;
  while ((result = session->staging.waitForItem(index, std::chrono::milliseconds(500))) == WaitResult::Timeout) {
    if (s->stopping) {
      result = WaitResult::Failed;
      break;
    }
    const auto bytes = session->staging.receivedBytes();
    const auto now = std::chrono::steady_clock::now();
    if (bytes != lastBytes) {
      lastBytes = bytes;
      lastProgress = now;
    } else if (now - lastProgress > kStallTimeout) {
      LOG_WARN("file drag %u: no data arrived for %lld s", dragId, static_cast<long long>(kStallTimeout.count()));
      cancelSession(s, session, fd::CancelReason::Timeout, true);
      result = WaitResult::Failed;
      break;
    }
  }
  if (result != WaitResult::Eof) {
    completion(promiseError(@"The files did not arrive."));
    cancelSession(s, session, session->cancelled ? session->reason.load() : fd::CancelReason::DropFailed, true);
    return;
  }

  // the receiver names the place; never replace what is there (a free "name (2)" instead)
  NSURL *folderUrl = url.URLByDeletingLastPathComponent;
  const char *folder = folderUrl.fileSystemRepresentation;
  NSString *wanted = url.lastPathComponent;
  const std::string name = wanted.length > 0 ? toUtf8(wanted) : entry.localPath;
  fs::path placed;
  std::string error;
  if (folder == nullptr || !fd::FileDragStaging::moveNoReplace(
                               session->staging.pathOf(index), fs::path(folder), name, entry.isDirectory, placed, &error
                           )) {
    LOG_WARN(
        "file drag %u: cannot put \"%s\" into %s: %s", dragId, entry.localPath.c_str(),
        folder != nullptr ? folder : "?", error.c_str()
    );
    completion(promiseError(@"The dragged item could not be put there."));
    cancelSession(s, session, fd::CancelReason::DropFailed, true);
    return;
  }
  LOG_INFO("file drag %u: placed %s", dragId, placed.c_str());
  completion(nil);

  dispatchMain(^{
    if (--session->promisesLeft <= 0 && session->phase == DestPhase::Dropped && !session->cancelled) {
      finishSession(s, session);
    }
  });
}

void beginSession(StatePtr s, const fd::Message &message)
{
  auto session = std::make_shared<DestSession>(fd::FileDragStaging::defaultRoot());
  session->id = message.dragId;
  {
    std::scoped_lock lock(s->destMutex);
    if (s->pendingIconId == message.dragId) {
      session->icon = s->pendingIcon;
    }
  }

  auto reason = fd::CancelReason::Unknown;
  std::string error;
  if (!session->staging.begin(message.dragId, message.items, reason, &error)) {
    LOG_WARN("file drag %u refused: %s (%s)", message.dragId, fd::cancelReasonName(reason), error.c_str());
    s->sendPeer(fd::Message::cancel(message.dragId, reason));
    postLoopEnded(s, message.dragId, false);
    return;
  }
  session->tops = session->staging.topLevelIndices();
  {
    std::scoped_lock lock(s->destMutex);
    s->dests[message.dragId] = session;
  }
  LOG_INFO(
      "file drag %u: staging %zu items in %s", message.dragId, session->staging.entries().size(),
      session->staging.stagingDir().c_str()
  );
  dispatchMain(^{
    showLauncherMain(s, session);
  });
}

void cancelFromPeer(StatePtr s, const fd::Message &message)
{
  if (message.reason == fd::CancelReason::ConnectionLost) {
    // the server is gone: every drag from it stops, including ones still copying
    std::vector<DestPtr> all;
    {
      std::scoped_lock lock(s->destMutex);
      for (const auto &[id, session] : s->dests) {
        all.push_back(session);
      }
    }
    for (const auto &session : all) {
      cancelSession(s, session, fd::CancelReason::ConnectionLost, false);
    }
    return;
  }
  if (auto session = findDest(s, message.dragId)) {
    // flags 1: cancelled here (escape, right click, cursor left): tell the server
    cancelSession(s, session, message.reason, message.flags == 1);
  } else if (message.flags == 1) {
    s->sendPeer(fd::Message::cancel(message.dragId, message.reason));
  }
}

//! Writer thread: staging writes happen in the order the messages came, off the core thread
void writerLoop(State *raw)
{
  pthread_setname_np("deskflow file drag writer");
  const StatePtr s = raw->shared_from_this();
  fd::FileDragStaging::sweepStale(fd::FileDragStaging::defaultRoot());
  while (true) {
    fd::Message message;
    {
      std::unique_lock lock(s->writerMutex);
      s->writerCond.wait(lock, [&s] { return s->writerStop || !s->writerQueue.empty(); });
      if (s->writerStop) {
        return;
      }
      message = std::move(s->writerQueue.front());
      s->writerQueue.pop_front();
    }

    @autoreleasepool {
      switch (message.type) {
        using enum fd::Message::Type;
      case Begin:
        beginSession(s, message);
        break;

      case FileStart: {
        auto session = findDest(s, message.dragId);
        if (session && !session->cancelled && !session->staging.fileStart(message.index, message.size)) {
          LOG_WARN("file drag %u: cannot start a staged file", message.dragId);
          cancelSession(s, session, fd::CancelReason::WriteFailed, true);
        }
        break;
      }

      case Chunk: {
        auto session = findDest(s, message.dragId);
        if (!session || session->cancelled) {
          break;
        }
        if (session->staging.write(message.index, message.data)) {
          s->sendPeer(fd::Message::ack(message.dragId, message.seq));
        } else {
          LOG_WARN("file drag %u: cannot write a staged file", message.dragId);
          cancelSession(s, session, fd::CancelReason::WriteFailed, true);
        }
        break;
      }

      case FileEnd: {
        auto session = findDest(s, message.dragId);
        if (session && !session->cancelled && !session->staging.fileEnd(message.index)) {
          LOG_WARN("file drag %u: a staged file is incomplete", message.dragId);
          cancelSession(s, session, fd::CancelReason::WriteFailed, true);
        }
        break;
      }

      case Cancel:
        cancelFromPeer(s, message);
        break;

      default:
        break;
      }
    }
  }
}

} // namespace

//
// OSXFileDrag
//

OSXFileDrag::OSXFileDrag(IEventQueue *events, void *eventTarget, bool primary, Hooks hooks)
    : m_state(std::make_shared<State>())
{
  m_state->events = events;
  m_state->eventTarget = eventTarget;
  m_state->primary = primary;
  m_state->hooks = std::move(hooks);

  if (NSApp == nil) {
    LOG_WARN("file drag: no Cocoa application in this process, dragging files between computers is off");
    return;
  }
  m_state->available = true;

  events->addHandler(EventTypes::FileDrag, m_state.get(), [](const Event &event) {
    if (const auto *task = dynamic_cast<const CoreTask *>(event.getDataObject())) {
      task->run();
    }
  });

  StatePtr s = m_state;
  dispatchMain(^{
    // the core runs as a background-only app; its drag windows need an accessory app (still
    // no Dock icon or menu bar) to show and take the pointer
    if (NSApp.activationPolicy == NSApplicationActivationPolicyProhibited) {
      if ([NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory]) {
        LOG_DEBUG("file drag: running as an accessory app so its drag windows work");
      } else {
        LOG_WARN("file drag: cannot run as an accessory app; drag windows may not work");
      }
    }
    controllerOf(s);
  });

  State *raw = m_state.get();
  m_state->writer = std::thread([raw] { writerLoop(raw); });
  LOG_INFO("file drag: ready (sends and receives files on this Mac as the %s)", primary ? "server" : "client");
}

OSXFileDrag::~OSXFileDrag()
{
  StatePtr s = m_state;
  s->parkedRelease = false;
  if (!s->available) {
    return;
  }
  s->events->removeHandler(EventTypes::FileDrag, s.get());

  {
    std::scoped_lock lock(s->writerMutex);
    s->writerStop = true;
    s->writerQueue.clear();
  }
  s->writerCond.notify_all();
  if (s->writer.joinable()) {
    s->writer.join();
  }
  s->stopping = true;

  std::map<uint32_t, std::unique_ptr<SourceSession>> sessions;
  {
    std::scoped_lock lock(s->sessionsMutex);
    sessions.swap(s->sessions);
  }
  sessions.clear(); // joins the sender threads

  std::map<uint32_t, DestPtr> dests;
  {
    std::scoped_lock lock(s->destMutex);
    dests.swap(s->dests);
  }
  for (auto &[id, session] : dests) {
    session->told = true;
    session->cancelled = true;
    session->staging.cancel(); // wakes waiting promises; deletes the staged files
  }

  dispatchMain(^{
    [s->controller teardown];
    s->controller = nil;
    s->launcherSession.reset();
    s->loopSession.reset();
    s->offer.reset();
    for (auto &session : s->pendingRemoval) {
      session->staging.removeStaged();
    }
    s->pendingRemoval.clear();
  });
}

uint32_t OSXFileDrag::capabilities() const
{
  if (!m_state->available) {
    return fd::kCapNone;
  }
  // both roles, and the client to server messages (kMsgDFileDragWatch, Offer, HandOff)
  return fd::kCapSource | fd::kCapDestination | fd::kCapReverse;
}

void OSXFileDrag::watchEdges(uint32_t sideMask, int32_t x, int32_t y, int32_t w, int32_t h)
{
  if (!m_state->available) {
    return;
  }
  StatePtr s = m_state;
  const fd::EdgeRect desktop{x, y, w, h};
  dispatchMain(^{
    watchEdgesMain(s, sideMask, desktop);
  });
}

void OSXFileDrag::handOff(uint32_t dragId, int32_t parkX, int32_t parkY)
{
  if (!m_state->available) {
    return;
  }
  // the cursor leaves right after this. On the server the user's release that ends the
  // drag happens over there and must still reach this computer once; on a client the
  // button was pressed by the server, and OSXComputer lets go of it when the drag is over
  m_state->parkedRelease = m_state->primary;
  m_state->currentDragId = dragId;
  StatePtr s = m_state;
  dispatchMain(^{
    handOffMain(s, dragId, parkX, parkY);
  });
}

void OSXFileDrag::takeBack()
{
  if (!m_state->available) {
    return;
  }
  m_state->parkedRelease = false;
  if (const uint32_t id = m_state->currentDragId.exchange(0); id != 0) {
    LOG_INFO("file drag %u: cursor came back, stop sending", id);
    stopSourceSession(m_state, id);
  }
  StatePtr s = m_state;
  dispatchMain(^{
    unpark(s, "taken back");
  });
}

void OSXFileDrag::entered()
{
  if (!m_state->available) {
    return;
  }
  m_state->parkedRelease = false;
  StatePtr s = m_state;
  dispatchMain(^{
    unpark(s, "cursor is back");
  });
}

bool OSXFileDrag::takeParkedRelease()
{
  return m_state->parkedRelease.exchange(false);
}

void OSXFileDrag::receive(const fd::Message &message)
{
  StatePtr s = m_state;
  if (!s->available) {
    return;
  }

  // drags sent from here (both computers send now: drag ids come from the server's one
  // counter, so a drag sent from here and one coming here never share an id)
  switch (message.type) {
    using enum fd::Message::Type;
  case Ack: {
    std::scoped_lock lock(s->sessionsMutex);
    if (auto it = s->sessions.find(message.dragId); it != s->sessions.end()) {
      it->second->onAck(message.seq);
    }
    return;
  }
  case Dropped:
    LOG_DEBUG("file drag %u dropped on the other computer", message.dragId);
    return;
  case Finished:
    stopSourceSession(s, message.dragId);
    return;
  case Cancel:
    if (message.dragId == 0) {
      // the connection is gone: every drag sent from here stops too
      std::map<uint32_t, std::unique_ptr<SourceSession>> sessions;
      {
        std::scoped_lock lock(s->sessionsMutex);
        sessions.swap(s->sessions);
      }
      s->currentDragId = 0;
      sessions.clear(); // joins the sender threads
    } else {
      bool sending = false;
      {
        std::scoped_lock lock(s->sessionsMutex);
        sending = s->sessions.contains(message.dragId);
      }
      if (sending || s->currentDragId == message.dragId) {
        LOG_INFO("file drag %u cancelled: %s", message.dragId, fd::cancelReasonName(message.reason));
        stopSourceSession(s, message.dragId);
        return;
      }
    }
    break; // also for drags coming here
  default:
    break;
  }

  switch (message.type) {
    using enum fd::Message::Type;
  case Icon: {
    // arrives just before its Begin
    std::scoped_lock lock(s->destMutex);
    s->pendingIcon = message.icon;
    s->pendingIconId = message.dragId;
    return;
  }

  case Cancel:
    if (message.reason == fd::CancelReason::ConnectionLost) {
      // nothing still queued for the lost server matters now
      std::scoped_lock lock(s->writerMutex);
      s->writerQueue.clear();
    }
    break;

  case Begin:
  case FileStart:
  case Chunk:
  case FileEnd:
    break;

  default:
    return;
  }

  {
    std::scoped_lock lock(s->writerMutex);
    s->writerQueue.push_back(message);
  }
  s->writerCond.notify_one();
}
