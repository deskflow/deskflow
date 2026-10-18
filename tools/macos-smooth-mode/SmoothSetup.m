/*
 * SPDX-FileCopyrightText: (C) 2026 lxf82dev <lxf82dev@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#import <Foundation/Foundation.h>
#include <libproc.h>
#include <pwd.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>

static NSString *const label = @"local.deskflow.mx-awdl";
static NSString *const menuLabel = @"local.deskflow.mx-awdl-menu";
static NSString *const rootDir = @"/Library/PrivilegedHelperTools";
static NSString *const rootBinary = @"/Library/PrivilegedHelperTools/local.deskflow.mx-awdl";
static NSString *const rootPlist = @"/Library/LaunchDaemons/local.deskflow.mx-awdl.plist";
static NSString *const rootLogs = @"/Library/Logs/DeskflowMXAWDL";
static NSFileManager *fm;
static uid_t targetUID;
static gid_t targetGID;
static NSString *userName, *userHome;
static void fail(NSString *message) { @throw [NSException exceptionWithName:@"SetupError" reason:message userInfo:nil]; }
static void must(BOOL ok, NSError *error) { if (!ok) fail(error.localizedDescription ?: @"File operation failed"); }
static BOOL run(NSString *path, NSArray<NSString *> *args, BOOL required) {
    NSTask *t = NSTask.new;
    t.executableURL = [NSURL fileURLWithPath:path]; t.arguments = args;
    t.standardOutput = NSFileHandle.fileHandleWithStandardOutput;
    t.standardError = required ? NSFileHandle.fileHandleWithStandardError : NSFileHandle.fileHandleWithNullDevice;
    NSError *e;
    if (![t launchAndReturnError:&e]) { if (required) fail(e.localizedDescription); return NO; }
    [t waitUntilExit];
    if (required && t.terminationStatus) fail([NSString stringWithFormat:@"%@ failed (%d)",path,t.terminationStatus]);
    return t.terminationStatus == 0;
}
static void permissions(NSString *path, uid_t uid, gid_t gid, mode_t mode) {
    if (chown(path.fileSystemRepresentation,uid,gid) || chmod(path.fileSystemRepresentation,mode)) fail(@"Cannot set ownership/permissions");
}
static void rootDirectory(NSString *path) {
    struct stat st;
    if (lstat(path.fileSystemRepresentation,&st)) {
        if (mkdir(path.fileSystemRepresentation,0755)) fail([@"Cannot create " stringByAppendingString:path]);
    } else if (!S_ISDIR(st.st_mode) || st.st_uid != 0 || (st.st_mode & 0022)) fail([@"Unsafe privileged directory: " stringByAppendingString:path]);
}
static void validateUserPath(NSString *path) {
    NSString *home = userHome.stringByResolvingSymlinksInPath;
    NSString *resolved = path.stringByResolvingSymlinksInPath;
    if (![resolved hasPrefix:[home stringByAppendingString:@"/"]]) fail([@"User path resolves outside this home folder: " stringByAppendingString:path]);
}
static void userDirectory(NSString *path, mode_t mode) {
    validateUserPath(path);
    struct stat previous;
    BOOL existed = lstat(path.fileSystemRepresentation,&previous) == 0;
    if (existed && (!S_ISDIR(previous.st_mode) || previous.st_uid != targetUID)) fail(@"Existing user directory has an unexpected type or owner");
    NSError *e;
    must([fm createDirectoryAtPath:path withIntermediateDirectories:YES attributes:nil error:&e],e);
    struct stat st;
    if (lstat(path.fileSystemRepresentation,&st) || !S_ISDIR(st.st_mode)) fail(@"User directory must be a real directory");
    // Preserve unrelated parent-directory permissions; enforce private mode only on our own folders.
    if (!existed || [path.lastPathComponent isEqualToString:@"DeskflowMXAWDL"]) permissions(path,targetUID,targetGID,mode);
}
static void writeData(NSData *data, NSString *path, uid_t uid, gid_t gid, mode_t mode) {
    struct stat st;
    if (!lstat(path.fileSystemRepresentation,&st) && !S_ISREG(st.st_mode)) fail([@"Refusing non-regular target: " stringByAppendingString:path]);
    NSError *e;
    must([data writeToFile:path options:NSDataWritingAtomic error:&e],e);
    permissions(path,uid,gid,mode);
}
static NSData *plistData(NSDictionary *dict) {
    NSError *e;
    NSData *d = [NSPropertyListSerialization dataWithPropertyList:dict format:NSPropertyListXMLFormat_v1_0 options:0 error:&e];
    if (!d) fail(e.localizedDescription); return d;
}
static NSString *iniGet(NSString *text, NSString *section, NSString *key) {
    BOOL inside = NO;
    for (NSString *line in [text componentsSeparatedByString:@"\n"]) {
        if ([line hasPrefix:@"["]) inside = [line isEqualToString:[NSString stringWithFormat:@"[%@]",section]];
        else if (inside && [line hasPrefix:[key stringByAppendingString:@"="]]) return [line substringFromIndex:key.length+1];
    }
    return nil;
}
static NSString *iniSet(NSString *text, NSString *section, NSString *key, NSString *value) {
    NSMutableArray<NSString *> *lines = [[text componentsSeparatedByString:@"\n"] mutableCopy];
    NSString *heading = [NSString stringWithFormat:@"[%@]",section];
    NSUInteger start = [lines indexOfObject:heading];
    if (start == NSNotFound) {
        if (!value) return text;
        if (lines.count && [lines.lastObject length]) [lines addObject:@""];
        [lines addObject:heading]; [lines addObject:[NSString stringWithFormat:@"%@=%@",key,value]]; [lines addObject:@""];
    } else {
        NSUInteger end = start+1;
        while (end < lines.count && ![lines[end] hasPrefix:@"["]) ++end;
        for (NSUInteger i = end; i > start+1;) {
            --i;
            if ([lines[i] hasPrefix:[key stringByAppendingString:@"="]]) { [lines removeObjectAtIndex:i]; --end; }
        }
        if (value) [lines insertObject:[NSString stringWithFormat:@"%@=%@",key,value] atIndex:end];
    }
    NSString *result = [lines componentsJoinedByString:@"\n"];
    return [result hasSuffix:@"\n"] ? result : [result stringByAppendingString:@"\n"];
}
static NSArray<NSNumber *> *deskflowPIDs(void) {
    int size = proc_listpids(PROC_ALL_PIDS,0,NULL,0)+4096;
    pid_t *ids = calloc(1,(size_t)size);
    int used = proc_listpids(PROC_ALL_PIDS,0,ids,size);
    NSMutableArray *result = NSMutableArray.array;
    for (int i=0;i<used/(int)sizeof(pid_t) && i<size/(int)sizeof(pid_t);i++) {
        char name[128]={0}; struct proc_bsdinfo info={0};
        if (ids[i]>0 && proc_name(ids[i],name,sizeof(name))>0 &&
            (!strcmp(name,"Deskflow") || !strcmp(name,"deskflow-core")) &&
            proc_pidinfo(ids[i],PROC_PIDTBSDINFO,0,&info,sizeof(info))==sizeof(info) && info.pbi_uid==targetUID) [result addObject:@(ids[i])];
    }
    free(ids);return result;
}
static void stopDeskflow(void) {
    for (NSNumber *pid in deskflowPIDs()) (void)kill(pid.intValue,SIGTERM);
    for (int i=0;i<50;i++) { if (!deskflowPIDs().count) return; usleep(100000); }
    fail(@"Deskflow did not exit. Close Deskflow and retry; settings were not edited.");
}
static void reopenDeskflow(void) {
    run(@"/bin/launchctl",@[@"asuser",[@(targetUID) stringValue],@"/usr/bin/sudo",@"-u",userName,@"/usr/bin/open",@"-a",@"/Applications/Deskflow.app"],NO);
}
static void ownTree(NSString *path) {
    permissions(path,targetUID,targetGID,0755);
    for (NSString *relative in [fm subpathsAtPath:path]) {
        NSString *p = [path stringByAppendingPathComponent:relative];
        struct stat st;
        if (lstat(p.fileSystemRepresentation,&st) || S_ISLNK(st.st_mode)) fail(@"Unexpected symlink in menu app");
        permissions(p,targetUID,targetGID,(S_ISDIR(st.st_mode) || [relative isEqualToString:@"Contents/MacOS/SmoothMenu"]) ? 0755 : 0644);
    }
}
static void removeFile(NSString *path) { if ([fm fileExistsAtPath:path]) { NSError *e; must([fm removeItemAtPath:path error:&e],e); } }
static void install(BOOL server, NSString *source) {
    NSString *state = [userHome stringByAppendingPathComponent:@"Library/Application Support/DeskflowMXAWDL"];
    NSString *logs = [userHome stringByAppendingPathComponent:@"Library/Logs/DeskflowMXAWDL"];
    NSString *prefs = [userHome stringByAppendingPathComponent:@"Library/Deskflow/Deskflow.conf"];
    NSString *app = [userHome stringByAppendingPathComponent:@"Applications/Deskflow Smooth Mode.app"];
    NSString *agents = [userHome stringByAppendingPathComponent:@"Library/LaunchAgents"];
    NSString *agent = [agents stringByAppendingPathComponent:[menuLabel stringByAppendingString:@".plist"]];
    NSString *domain = [NSString stringWithFormat:@"gui/%u",targetUID];
    NSString *menuTarget = [domain stringByAppendingFormat:@"/%@",menuLabel];
    NSString *guardTarget = [@"system/" stringByAppendingString:label];
    NSError *e;
    validateUserPath(prefs); validateUserPath(app); validateUserPath(agent);
    NSString *before = [NSString stringWithContentsOfFile:prefs encoding:NSUTF8StringEncoding error:&e];
    if (!before) fail(@"Deskflow preferences not found; open Deskflow and save its settings first.");
    if (iniGet(before,@"core",@"coreMode").intValue != (server ? 2 : 1)) fail(@"Selected setup role differs from Deskflow's saved server/client mode.");
    for (NSString *file in @[@"awdl-guard",@"Deskflow Smooth Mode.app/Contents/MacOS/SmoothMenu"]) {
        struct stat st;
        if (lstat([[source stringByAppendingPathComponent:file] fileSystemRepresentation],&st) || !S_ISREG(st.st_mode)) fail(@"Setup files are missing; extract the entire folder first.");
    }
    rootDirectory(rootDir); rootDirectory(@"/Library/LaunchDaemons"); rootDirectory(rootLogs);
    userDirectory(state,0700); userDirectory(logs,0700); userDirectory(agents,0755);
    userDirectory([userHome stringByAppendingPathComponent:@"Applications"],0700);
    NSString *backup = [state stringByAppendingPathComponent:@"Deskflow.before-smoothing.conf"];
    if (![fm fileExistsAtPath:backup]) writeData([before dataUsingEncoding:NSUTF8StringEncoding],backup,targetUID,targetGID,0600);
    NSString *control = [state stringByAppendingPathComponent:@"control"];
    if (![fm fileExistsAtPath:control]) writeData([@"auto\n" dataUsingEncoding:NSUTF8StringEncoding],control,targetUID,targetGID,0600);
    run(@"/bin/launchctl",@[@"bootout",menuTarget],NO);
    run(@"/bin/launchctl",@[@"bootout",guardTarget],NO);
    if ([fm fileExistsAtPath:app]) {
        NSDictionary *info = [NSDictionary dictionaryWithContentsOfFile:[app stringByAppendingPathComponent:@"Contents/Info.plist"]];
        if (![info[@"CFBundleIdentifier"] isEqualToString:@"local.deskflow.mx-smooth-mode"]) fail(@"An unrelated app uses the install path; refusing replacement.");
        removeFile(app);
    }
    must([fm copyItemAtPath:[source stringByAppendingPathComponent:@"Deskflow Smooth Mode.app"] toPath:app error:&e],e);ownTree(app);
    NSData *worker = [NSData dataWithContentsOfFile:[source stringByAppendingPathComponent:@"awdl-guard"]];
    writeData(worker,rootBinary,0,0,0755);
    NSString *logFile = [logs stringByAppendingPathComponent:@"deskflow.log"];
    if (![fm fileExistsAtPath:logFile]) writeData(NSData.data,logFile,targetUID,targetGID,0600);
    NSDictionary *daemon = @{@"Label":label,@"ProgramArguments":@[rootBinary,@"--role",server?@"server":@"client",@"--uid",[@(targetUID) stringValue],@"--log",logFile,@"--control",control,@"--status",[rootLogs stringByAppendingPathComponent:@"status"]],@"RunAtLoad":@YES,@"KeepAlive":@YES,@"ThrottleInterval":@5,@"ProcessType":@"Interactive",@"StandardOutPath":[rootLogs stringByAppendingPathComponent:@"guard.log"],@"StandardErrorPath":[rootLogs stringByAppendingPathComponent:@"guard.log"],@"Umask":@022};
    writeData(plistData(daemon),rootPlist,0,0,0644);
    NSDictionary *menu = @{@"Label":menuLabel,@"ProgramArguments":@[[app stringByAppendingPathComponent:@"Contents/MacOS/SmoothMenu"]],@"RunAtLoad":@YES,@"LimitLoadToSessionType":@"Aqua",@"StandardOutPath":[logs stringByAppendingPathComponent:@"menu.log"],@"StandardErrorPath":[logs stringByAppendingPathComponent:@"menu-errors.log"]};
    writeData(plistData(menu),agent,targetUID,targetGID,0644);
    stopDeskflow();
    @try {
        NSString *current = [NSString stringWithContentsOfFile:prefs encoding:NSUTF8StringEncoding error:&e];
        if (!current) fail(e.localizedDescription);
        current = iniSet(current,@"log",@"file",logFile);
        current = iniSet(current,@"log",@"toFile",@"true");
        current = iniSet(current,@"log",@"level",@"Info");
        writeData([current dataUsingEncoding:NSUTF8StringEncoding],prefs,targetUID,targetGID,0600);
        run(@"/bin/launchctl",@[@"enable",guardTarget],YES);
        run(@"/bin/launchctl",@[@"bootstrap",@"system",rootPlist],YES);
        run(@"/bin/launchctl",@[@"enable",menuTarget],YES);
        run(@"/bin/launchctl",@[@"bootstrap",domain,agent],YES);
    } @finally { reopenDeskflow(); }
    printf("Installed automatic Smooth Mode on %s for %s (uid %u).\n",server?"SERVER":"CLIENT",userName.UTF8String,targetUID);
    puts("Switch to the client and back once after both installations. The DF menu controls AirDrop override and automatic mode.");
}
static void uninstall(void) {
    NSString *state = [userHome stringByAppendingPathComponent:@"Library/Application Support/DeskflowMXAWDL"];
    NSString *prefs = [userHome stringByAppendingPathComponent:@"Library/Deskflow/Deskflow.conf"];
    NSString *agent = [userHome stringByAppendingPathComponent:[@"Library/LaunchAgents/" stringByAppendingFormat:@"%@.plist",menuLabel]];
    NSString *domain = [NSString stringWithFormat:@"gui/%u/%@",targetUID,menuLabel];
    validateUserPath(state); validateUserPath(prefs); validateUserPath(agent);
    run(@"/bin/launchctl",@[@"bootout",domain],NO);
    run(@"/bin/launchctl",@[@"bootout",[@"system/" stringByAppendingString:label]],NO);
    stopDeskflow();
    @try {
        NSString *backup = [NSString stringWithContentsOfFile:[state stringByAppendingPathComponent:@"Deskflow.before-smoothing.conf"] encoding:NSUTF8StringEncoding error:nil];
        NSString *current = [NSString stringWithContentsOfFile:prefs encoding:NSUTF8StringEncoding error:nil];
        if (backup && current) {
            for (NSString *key in @[@"file",@"toFile",@"level"]) current = iniSet(current,@"log",key,iniGet(backup,@"log",key));
            writeData([current dataUsingEncoding:NSUTF8StringEncoding],prefs,targetUID,targetGID,0600);
        }
        removeFile(rootPlist); removeFile(rootBinary); removeFile(agent);
        removeFile([userHome stringByAppendingPathComponent:@"Applications/Deskflow Smooth Mode.app"]);
    } @finally { reopenDeskflow(); }
    puts("Smooth Mode removed. Logs/backups retained. The existing Mission Control relay was not changed.");
}
static int selfTest(void) {
    NSString *s = @"[server]\nprotocol=synergy\n[log]\nlevel=Info\n[internalConfig]\nhotkeys\\1\\keys\\1\\key=33554432\n";
    NSString *v = iniSet(s,@"log",@"toFile",@"true");
    v = iniSet(v,@"log",@"file",@"/Users/example/Library/Logs/test.log");
    BOOL ok = [iniGet(v,@"log",@"toFile") isEqualToString:@"true"] && [iniGet(v,@"server",@"protocol") isEqualToString:@"synergy"] && [v containsString:@"hotkeys\\1\\keys\\1\\key=33554432"];
    v = iniSet(v,@"log",@"file",nil);
    ok = ok && !iniGet(v,@"log",@"file") && [iniGet(v,@"log",@"level") isEqualToString:@"Info"];
    puts(ok ? "PASS: scoped INI edits and restoration; no preferences or services changed." : "FAIL");
    return ok ? 0 : 1;
}
int main(int argc,const char **argv) {
    @autoreleasepool {
        fm = NSFileManager.defaultManager;
        if (argc==2 && !strcmp(argv[1],"--self-test")) return selfTest();
        if (argc<2) { fputs("Usage: smooth-setup plan|install server|client; smooth-setup uninstall\n",stderr);return 2; }
        NSString *action = @(argv[1]);
        BOOL plan = [action isEqualToString:@"plan"], installing = [action isEqualToString:@"install"];
        if ((!plan && !installing && ![action isEqualToString:@"uninstall"]) || ((plan || installing) && (argc!=3 || (strcmp(argv[2],"server") && strcmp(argv[2],"client"))))) return 2;
        const char *sudoUID = getenv("SUDO_UID");
        targetUID = sudoUID ? (uid_t)strtoul(sudoUID,NULL,10) : getuid();
        struct passwd *pw = getpwuid(targetUID);
        if (!pw || targetUID<500) { fputs("Run from your logged-in account with sudo, not a root login.\n",stderr);return 2; }
        targetGID = pw->pw_gid; userHome = @(pw->pw_dir); userName = @(pw->pw_name);
        if (![userHome hasPrefix:@"/"] || [userHome containsString:@"\n"]) return 2;
        if (plan) {
            printf("Role: %s\nUser: %s (%u)\nRoot worker: %s\nRoot login service: %s\nMenu app: %s/Applications/Deskflow Smooth Mode.app\nLog: %s/Library/Logs/DeskflowMXAWDL/deskflow.log\nOnly log/file, log/toFile and log/level will change in Deskflow settings.\nThe worker controls only awdl0. No sudoers or setuid changes.\n",argv[2],userName.UTF8String,targetUID,rootBinary.UTF8String,rootPlist.UTF8String,userHome.UTF8String,userHome.UTF8String);
            return 0;
        }
        if (geteuid()!=0) { fputs("Administrator installation required: run this setup with sudo.\n",stderr);return 2; }
        struct stat console;
        if (stat("/dev/console",&console) || console.st_uid != targetUID) {
            fputs("Run setup from the currently logged-in desktop user's Terminal.\n",stderr);return 2;
        }
        @try {
            if (installing) {
                NSString *executable = [NSString stringWithUTF8String:argv[0]];
                if (![executable isAbsolutePath]) executable = [fm.currentDirectoryPath stringByAppendingPathComponent:executable];
                install(!strcmp(argv[2],"server"),executable.stringByDeletingLastPathComponent);
            } else uninstall();
        } @catch (NSException *exception) { fprintf(stderr,"Setup stopped: %s\n",exception.reason.UTF8String);return 1; }
    }
    return 0;
}
