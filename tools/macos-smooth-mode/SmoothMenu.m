/*
 * SPDX-FileCopyrightText: (C) 2026 lxf82dev <lxf82dev@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#import <AppKit/AppKit.h>

@interface SmoothMenu : NSObject <NSApplicationDelegate>
@property NSStatusItem *item;
@property NSMenuItem *summary;
@property NSMenuItem *automatic;
@property NSMenuItem *airdrop;
@property NSString *controlPath;
@property NSTimer *timer;
@end

@implementation SmoothMenu
- (NSDictionary<NSString *,NSString *> *)status {
    NSString *path = @"/Library/Logs/DeskflowMXAWDL/status";
    NSDictionary *attrs = [NSFileManager.defaultManager attributesOfItemAtPath:path error:nil];
    if (!attrs || -[attrs[NSFileModificationDate] timeIntervalSinceNow] > 15) return @{};
    NSString *s = [NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:nil];
    NSMutableDictionary *result = NSMutableDictionary.dictionary;
    for (NSString *line in [s componentsSeparatedByString:@"\n"]) {
        NSRange r = [line rangeOfString:@"="];
        if (r.location != NSNotFound) result[[line substringToIndex:r.location]] = [line substringFromIndex:r.location+1];
    }
    return result;
}
- (NSString *)control {
    return [NSString stringWithContentsOfFile:self.controlPath encoding:NSUTF8StringEncoding error:nil] ?: @"auto";
}
- (void)writeControl:(NSString *)value {
    NSError *error;
    if (![value writeToFile:self.controlPath atomically:YES encoding:NSUTF8StringEncoding error:&error]) {
        NSAlert *alert = NSAlert.new;
        alert.messageText = @"Could not update Smooth Mode";
        alert.informativeText = error.localizedDescription;
        [alert runModal];
    } else [NSFileManager.defaultManager setAttributes:@{NSFilePosixPermissions:@0600} ofItemAtPath:self.controlPath error:nil];
    [self refresh:nil];
}
- (void)toggleAutomatic:(id)sender {
    (void)sender;
    [self writeControl:[[self control] hasPrefix:@"disabled"] ? @"auto\n" : @"disabled\n"];
}
- (void)toggleAirDrop:(id)sender {
    (void)sender;
    NSString *control = [self control];
    BOOL active = [control hasPrefix:@"pause_until="] &&
                  [[[control componentsSeparatedByString:@"="] lastObject] longLongValue] > (long long)NSDate.date.timeIntervalSince1970;
    if (active) [self writeControl:@"auto\n"];
    else [self writeControl:[NSString stringWithFormat:@"pause_until=%lld\n",(long long)NSDate.date.timeIntervalSince1970+600]];
}
- (void)showGuide:(id)sender {
    (void)sender;
    NSURL *url = [NSBundle.mainBundle URLForResource:@"README" withExtension:@"txt"];
    if (url) [NSWorkspace.sharedWorkspace openURL:url];
}
- (void)showLog:(id)sender {
    (void)sender;
    [NSWorkspace.sharedWorkspace openURL:[NSURL fileURLWithPath:@"/Library/Logs/DeskflowMXAWDL/guard.log"]];
}
- (void)quit:(id)sender {
    (void)sender;
    [self writeControl:@"disabled\n"];
    [NSApp terminate:nil];
}
- (void)refresh:(id)sender {
    (void)sender;
    NSDictionary *s = self.status;
    NSString *mode = s[@"mode"] ?: @"offline";
    NSDictionary *names = @{@"smooth":@"Client active — AWDL suppressed",
                            @"allowed":@"Server active — AirDrop allowed",
                            @"waiting":@"Waiting for a Deskflow switch",
                            @"deskflow-stopped":@"Deskflow stopped — AirDrop allowed",
                            @"airdrop":@"AirDrop override — automatic resumes soon",
                            @"disabled":@"Automatic smoothing is off",
                            @"stopped":@"Controller stopped",
                            @"error":@"Controller error — see log",
                            @"offline":@"Controller offline — run setup"};
    self.summary.title = names[mode] ?: mode;
    self.item.button.title = [mode isEqualToString:@"smooth"] ? @"DF ●" : [mode isEqualToString:@"airdrop"] ? @"DF A" : @"DF";
    self.item.button.toolTip = [NSString stringWithFormat:@"Deskflow Smooth Mode: %@",self.summary.title];
    self.automatic.state = [[self control] hasPrefix:@"disabled"] ? NSControlStateValueOff : NSControlStateValueOn;
    BOOL paused = [s[@"pause_until"] longLongValue] > (long long)NSDate.date.timeIntervalSince1970;
    self.airdrop.title = paused ? @"Resume automatic mode now" : @"Allow AirDrop for 10 minutes";
}
- (void)applicationDidFinishLaunching:(NSNotification *)note {
    (void)note;
    self.controlPath = [NSHomeDirectory() stringByAppendingPathComponent:@"Library/Application Support/DeskflowMXAWDL/control"];
    self.item = [NSStatusBar.systemStatusBar statusItemWithLength:NSVariableStatusItemLength];
    NSMenu *menu = NSMenu.new;
    NSMenuItem *(^add)(NSString *,SEL) = ^NSMenuItem *(NSString *title, SEL action) {
        NSMenuItem *i = [[NSMenuItem alloc] initWithTitle:title action:action keyEquivalent:@""];
        i.target = self; [menu addItem:i]; return i;
    };
    self.summary = add(@"Loading…",NULL);
    [menu addItem:NSMenuItem.separatorItem];
    self.automatic = add(@"Automatic smoothing",@selector(toggleAutomatic:));
    self.airdrop = add(@"Allow AirDrop for 10 minutes",@selector(toggleAirDrop:));
    [menu addItem:NSMenuItem.separatorItem];
    add(@"How automatic mode works",@selector(showGuide:));
    add(@"Show controller log",@selector(showLog:));
    add(@"Quit controls and allow AirDrop",@selector(quit:));
    self.item.menu = menu;
    self.timer = [NSTimer scheduledTimerWithTimeInterval:1 target:self selector:@selector(refresh:) userInfo:nil repeats:YES];
    [self refresh:nil];
}
@end

int main(int argc, const char **argv) {
    (void)argc; (void)argv;
    @autoreleasepool {
        NSApplication *app = NSApplication.sharedApplication;
        [app setActivationPolicy:NSApplicationActivationPolicyAccessory];
        __attribute__((objc_precise_lifetime)) SmoothMenu *delegate = SmoothMenu.new;
        app.delegate = delegate;
        [app run];
    }
    return 0;
}
