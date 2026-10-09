/*
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "Native.h"

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>

#include <stdexcept>
#include <unistd.h>

namespace {
std::function<void(deskflow::handoff::Request)> sendRequest;
NSString *previousApplication = nil;

QString documentURL(NSRunningApplication *application)
{
  AXUIElementRef app = AXUIElementCreateApplication(application.processIdentifier);
  AXUIElementSetMessagingTimeout(app, 2);
  CFTypeRef window = nullptr;
  CFTypeRef document = nullptr;
  QString result;
  if (AXUIElementCopyAttributeValue(app, kAXFocusedWindowAttribute, &window) == kAXErrorSuccess &&
      CFGetTypeID(window) == AXUIElementGetTypeID()) {
    if (AXUIElementCopyAttributeValue((AXUIElementRef)window, kAXDocumentAttribute, &document) == kAXErrorSuccess &&
        CFGetTypeID(document) == CFStringGetTypeID())
      result = QString::fromNSString((NSString *)document);
  }
  if (document)
    CFRelease(document);
  if (window)
    CFRelease(window);
  CFRelease(app);
  return result;
}
} // namespace

@interface DeskflowSendService : NSObject
- (void)sendToComputer:(NSPasteboard *)pasteboard userData:(NSString *)data error:(NSString **)error;
@end

@implementation DeskflowSendService
- (void)sendToComputer:(NSPasteboard *)pasteboard userData:(NSString *)data error:(NSString **)error
{
  Q_UNUSED(data)
  try {
    deskflow::handoff::Request request;
    NSArray<NSURL *> *urls = [pasteboard readObjectsForClasses:@[ [NSURL class] ]
                                                       options:@{NSPasteboardURLReadingFileURLsOnlyKey : @YES}];
    for (NSURL *url in urls)
      request.files.append(QString::fromNSString(url.path));
    if (request.files.isEmpty()) {
      // Older Finder versions provide the filenames pasteboard type instead of file URLs.
      id paths = [pasteboard propertyListForType:@"NSFilenamesPboardType"];
      if ([paths isKindOfClass:[NSArray class]]) {
        for (id path in paths) {
          if ([path isKindOfClass:[NSString class]])
            request.files.append(QString::fromNSString(path));
        }
      }
    }
    if (request.files.size() == 1 &&
        [request.files.first().toNSString().pathExtension.lowercaseString isEqual:@"app"]) {
      NSBundle *bundle = [NSBundle bundleWithPath:request.files.first().toNSString()];
      if (!bundle.bundleIdentifier)
        throw std::runtime_error("Could not identify this application");
      request = deskflow::handoff::currentApplication(QString::fromNSString(bundle.bundleIdentifier));
    } else if (request.files.isEmpty()) {
      NSString *text = [pasteboard stringForType:NSPasteboardTypeURL];
      if (text)
        request.url = QUrl(QString::fromNSString(text), QUrl::StrictMode);
      else
        request = deskflow::handoff::currentApplication();
    }
    sendRequest(request);
  } catch (const std::exception &exception) {
    if (error)
      *error = QString::fromUtf8(exception.what()).toNSString();
  }
}
@end

namespace deskflow::handoff {
void installService(std::function<void(Request)> send)
{
  sendRequest = std::move(send);
  auto remember = ^(NSRunningApplication *application) {
    if (application.bundleIdentifier && application.processIdentifier != getpid()) {
      [previousApplication release];
      previousApplication = [application.bundleIdentifier copy];
    }
  };
  remember(NSWorkspace.sharedWorkspace.frontmostApplication);
  [NSWorkspace.sharedWorkspace.notificationCenter addObserverForName:NSWorkspaceDidActivateApplicationNotification
                                                              object:nil
                                                               queue:NSOperationQueue.mainQueue
                                                          usingBlock:^(NSNotification *notification) {
                                                            remember(notification.userInfo[NSWorkspaceApplicationKey]);
                                                          }];
  [NSApp setServicesProvider:[[[DeskflowSendService alloc] init] autorelease]];
  NSUpdateDynamicServices();
}

Request currentApplication(const QString &bundleId)
{
  const auto identifier = bundleId.isEmpty() ? QString::fromNSString(previousApplication) : bundleId;
  if (identifier.isEmpty())
    throw std::runtime_error("Select an application window first, then use Send current window to computer");
  Request request;
  request.application = identifier;
  const auto applications = [NSRunningApplication runningApplicationsWithBundleIdentifier:identifier.toNSString()];
  if (applications.count == 0)
    return request;

  // These browsers expose their active URL through Apple Events, rather than AXDocument.
  NSString *source = nil;
  if (identifier == "com.apple.Safari")
    source = @"tell application id \"com.apple.Safari\" to get URL of front document";
  else if (identifier == "com.google.Chrome")
    source = @"tell application id \"com.google.Chrome\" to get URL of active tab of front window";
  if (source) {
    NSDictionary *error = nil;
    NSAppleScript *script = [[[NSAppleScript alloc] initWithSource:source] autorelease];
    NSString *url = [[script executeAndReturnError:&error] stringValue];
    if (!url)
      throw std::runtime_error(
          "Could not read the browser tab. Allow Deskflow in Privacy & Security > Automation and try again."
      );
    request.url = QUrl(QString::fromNSString(url), QUrl::StrictMode);
    return request;
  }
  const auto url = QUrl(documentURL(applications.firstObject), QUrl::StrictMode);
  if (url.isLocalFile())
    request.files = {url.toLocalFile()};
  else if (url.scheme() == "https" || url.scheme() == "http")
    request.url = url;
  else
    throw std::runtime_error(
        "This app does not expose a saved document or URL. Save the document and send it from Finder."
    );
  return request;
}

void openReceived(const Request &request)
{
  @autoreleasepool {
    if (request.application.isEmpty()) {
      if (!request.url.isEmpty() &&
          ![NSWorkspace.sharedWorkspace openURL:[NSURL URLWithString:request.url.toString().toNSString()]])
        throw std::runtime_error("The URL arrived but could not be opened");
      return;
    }
    NSURL *application =
        [NSWorkspace.sharedWorkspace URLForApplicationWithBundleIdentifier:request.application.toNSString()];
    if (!application)
      throw std::runtime_error("The requested app is not installed. Any received files are still in Downloads.");
    NSMutableArray<NSURL *> *urls = [NSMutableArray array];
    for (const auto &file : request.files)
      [urls addObject:[NSURL fileURLWithPath:file.toNSString()]];
    if (!request.url.isEmpty())
      [urls addObject:[NSURL URLWithString:request.url.toString(QUrl::FullyEncoded).toNSString()]];
    __block bool finished = false;
    __block QString failure;
    auto completion = ^(NSRunningApplication *, NSError *error) {
      dispatch_async(dispatch_get_main_queue(), ^{
        if (error)
          failure = QString::fromNSString(error.localizedDescription);
        finished = true;
      });
    };
    NSWorkspaceOpenConfiguration *configuration = [NSWorkspaceOpenConfiguration configuration];
    if (urls.count)
      [NSWorkspace.sharedWorkspace openURLs:urls
                       withApplicationAtURL:application
                              configuration:configuration
                          completionHandler:completion];
    else
      [NSWorkspace.sharedWorkspace openApplicationAtURL:application
                                          configuration:configuration
                                      completionHandler:completion];
    NSDate *deadline = [NSDate dateWithTimeIntervalSinceNow:30];
    while (!finished && deadline.timeIntervalSinceNow > 0)
      [NSRunLoop.currentRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.05]];
    if (!finished || !failure.isEmpty())
      throw std::runtime_error(
          failure.isEmpty() ? "Timed out opening the app; received files are in Downloads" : failure.toStdString()
      );
  }
}
} // namespace deskflow::handoff
