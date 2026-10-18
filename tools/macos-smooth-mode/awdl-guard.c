/*
 * SPDX-FileCopyrightText: (C) 2026 lxf82dev <lxf82dev@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

// Fixed-interface, local-only privileged worker. No shell, network listener, or input capture.
#include "smooth_policy.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <net/if.h>
#include <net/route.h>
#include <libproc.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <limits.h>

static volatile sig_atomic_t stopping;
static bool dry_run, server_role, owned_down, restore_up, simulated_up = true;
static uid_t target_uid;
static const char *log_path, *control_path, *status_path, *fake_net_path, *fake_core_path;
static int net_fd = -1, log_fd = -1;
static off_t log_offset;
static dev_t log_dev;
static ino_t log_ino;
static char partial[8192];
static size_t partial_len;
static Focus focus;
static unsigned down_count, up_count;
static int last_net_error, last_write_error;

static void on_signal(int sig) { (void)sig; stopping = 1; }
static bool read_small(const char *path, char *buf, size_t size, uid_t owner) {
    int fd = open(path, O_RDONLY | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return false;
    struct stat s;
    if (fstat(fd, &s) || !S_ISREG(s.st_mode) || s.st_uid != owner) { close(fd); return false; }
    ssize_t n = read(fd, buf, size - 1);
    close(fd);
    if (n < 0) return false;
    buf[n] = 0; return true;
}
static int interface_up(void) {
    if (dry_run) {
        if (fake_net_path) {
            char b[32];
            if (!read_small(fake_net_path, b, sizeof(b), target_uid) || !strncmp(b, "error", 5)) {
                last_net_error = EIO; return -1;
            }
            simulated_up = !strncmp(b, "up", 2);
        }
        last_net_error = 0;
        return simulated_up;
    }
    struct ifreq r = {0};
    strlcpy(r.ifr_name, "awdl0", sizeof(r.ifr_name));
    if (ioctl(net_fd, SIOCGIFFLAGS, &r)) { last_net_error = errno; return -1; }
    last_net_error = 0;
    return (r.ifr_flags & IFF_UP) != 0;
}
static bool change_interface(bool up) {
    if (dry_run) {
        simulated_up = up;
        if (fake_net_path) {
            int fd = open(fake_net_path, O_WRONLY | O_TRUNC | O_NOFOLLOW);
            if (fd >= 0) { const char *s = up ? "up\n" : "down\n"; (void)write(fd, s, strlen(s)); close(fd); }
        }
    } else {
        struct ifreq r = {0};
        strlcpy(r.ifr_name, "awdl0", sizeof(r.ifr_name));
        if (ioctl(net_fd, SIOCGIFFLAGS, &r)) { last_write_error = errno; return false; }
        if (up) r.ifr_flags |= IFF_UP; else r.ifr_flags &= (short)~IFF_UP;
        if (ioctl(net_fd, SIOCSIFFLAGS, &r)) { last_write_error = errno; return false; }
    }
    last_write_error = 0;
    if (up) ++up_count; else ++down_count;
    fprintf(stderr, "%lld %sawdl0 %s\n", (long long)time(NULL), dry_run ? "SIMULATED " : "", up ? "allowed/up" : "suppressed/down");
    return true;
}
static void restore_owned(void) {
    if (owned_down && restore_up) {
        int up = interface_up();
        if (up < 0 || (up == 0 && !change_interface(true))) return;
    }
    owned_down = false; restore_up = false;
}
static void close_log(void) {
    if (log_fd >= 0) close(log_fd);
    log_fd = -1; log_offset = 0; partial_len = 0; focus_reset(&focus);
}
static bool pump_log(void) {
    struct stat s;
    if (lstat(log_path, &s) || !S_ISREG(s.st_mode) || s.st_uid != target_uid) { close_log(); return false; }
    if (log_fd < 0 || s.st_dev != log_dev || s.st_ino != log_ino || s.st_size < log_offset) {
        close_log();
        log_fd = open(log_path, O_RDONLY | O_NOFOLLOW | O_NONBLOCK);
        if (log_fd < 0 || fstat(log_fd, &s) || !S_ISREG(s.st_mode) || s.st_uid != target_uid) { close_log(); return false; }
        log_dev = s.st_dev; log_ino = s.st_ino;
        log_offset = lseek(log_fd, 0, SEEK_END); // never replay another run's focus
        return true;
    }
    if (s.st_size - log_offset > 1024 * 1024) {
        focus_reset(&focus); partial_len = 0; log_offset = lseek(log_fd, 0, SEEK_END); return true;
    }
    char b[16384]; ssize_t n;
    while ((n = read(log_fd, b, sizeof(b))) > 0) {
        log_offset += n;
        for (ssize_t i = 0; i < n; ++i) {
            if (b[i] == '\n') {
                if (partial_len && partial[partial_len - 1] == '\r') --partial_len;
                partial[partial_len] = 0; focus_line(&focus, server_role, partial); partial_len = 0;
            } else if (partial_len < sizeof(partial) - 1) partial[partial_len++] = b[i];
            else { partial_len = 0; focus_reset(&focus); }
        }
    }
    return true;
}
static pid_t find_core(void) {
    if (dry_run) {
        char b[32];
        if (fake_core_path && read_small(fake_core_path, b, sizeof(b), target_uid)) return atoi(b) > 0 ? 1 : 0;
        return 1;
    }
    int size = proc_listpids(PROC_ALL_PIDS, 0, NULL, 0);
    if (size <= 0) return 0;
    size += 4096;
    pid_t *ids = calloc(1, (size_t)size);
    if (!ids) return 0;
    int used = proc_listpids(PROC_ALL_PIDS, 0, ids, size);
    pid_t result = 0;
    for (int i = 0; i < used / (int)sizeof(pid_t) && i < size / (int)sizeof(pid_t); ++i) {
        char name[128] = {0}; struct proc_bsdinfo info = {0};
        if (ids[i] > 0 && proc_name(ids[i], name, sizeof(name)) > 0 && !strcmp(name, "deskflow-core") &&
            proc_pidinfo(ids[i], PROC_PIDTBSDINFO, 0, &info, sizeof(info)) == sizeof(info) && info.pbi_uid == target_uid) {
            result = ids[i]; break;
        }
    }
    free(ids); return result;
}
static void control_state(bool *disabled, time_t *pause_until) {
    *disabled = false; *pause_until = 0;
    char b[128];
    if (!read_small(control_path, b, sizeof(b), target_uid)) return;
    if (!strncmp(b, "disabled", 8)) { *disabled = true; return; }
    long long until = 0;
    if (sscanf(b, "pause_until=%lld", &until) == 1) {
        struct stat s;
        if (!lstat(control_path, &s) && until > 0 && until <= (long long)s.st_mtime + 605) *pause_until = (time_t)until;
    }
}
static void write_status(const char *mode, pid_t core, int up, time_t pause_until) {
    char b[1024], temp[PATH_MAX];
    int n = snprintf(b, sizeof(b), "version=1\npid=%d\nrole=%s\nmode=%s\ncore_pid=%d\nfocus_known=%d\nclient_active=%d\nawdl_up=%d\nowned_down=%d\nrestore_up=%d\ndown_count=%u\nup_count=%u\npause_until=%lld\nerror=%d\nupdated=%lld\n",
                     getpid(), server_role ? "server" : "client", mode, core, focus.known, focus.client_active, up,
                     owned_down, restore_up, down_count, up_count, (long long)pause_until,
                     last_write_error ? last_write_error : last_net_error, (long long)time(NULL));
    snprintf(temp, sizeof(temp), "%s.tmp.XXXXXX", status_path);
    int fd = mkstemp(temp);
    if (fd < 0) return;
    (void)fchmod(fd, 0644);
    bool ok = write(fd, b, (size_t)n) == n;
    close(fd);
    if (ok) (void)rename(temp, status_path); else (void)unlink(temp);
}
static int test_policy(void) {
    Focus active = {.known = true, .client_active = true};
    focus_line(&active, true, "[x] INFO: new client disconnected");
    if (!active.known || !active.client_active) return 1;
    for (int role = 0; role < 2; ++role) {
        Focus f = {0};
        if (should_suppress(f, true, false, false)) return 1;
        focus_line(&f, role, "[x] INFO: entering computer");
        if (f.client_active != !role) return 1;
        focus_line(&f, role, "[x] INFO: leaving computer");
        if (f.client_active != (bool)role) return 1;
        f.known = true; f.client_active = true;
        if (!should_suppress(f, true, false, false) || should_suppress(f, false, false, false) ||
            should_suppress(f, true, true, false) || should_suppress(f, true, false, true)) return 1;
        focus_line(&f, role, "[x] DEBUG: disconnected");
        if (f.known) return 1;
    }
    puts("PASS: role mapping, unknown-state handling, core loss, pause, disable, and disconnect; no interface changed.");
    return 0;
}
int main(int argc, char **argv) {
    if (argc == 2 && !strcmp(argv[1], "--self-test")) return test_policy();
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--dry-run")) dry_run = true;
        else if (!strcmp(argv[i], "--role") && i + 1 < argc) {
            const char *v = argv[++i];
            if (strcmp(v, "server") && strcmp(v, "client")) return 2;
            server_role = !strcmp(v, "server");
        } else if (!strcmp(argv[i], "--uid") && i + 1 < argc) target_uid = (uid_t)strtoul(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "--log") && i + 1 < argc) log_path = argv[++i];
        else if (!strcmp(argv[i], "--control") && i + 1 < argc) control_path = argv[++i];
        else if (!strcmp(argv[i], "--status") && i + 1 < argc) status_path = argv[++i];
        else if (!strcmp(argv[i], "--fake-net") && i + 1 < argc) fake_net_path = argv[++i];
        else if (!strcmp(argv[i], "--fake-core") && i + 1 < argc) fake_core_path = argv[++i];
        else { fprintf(stderr, "Unknown/incomplete option. Use --self-test or installed launchd arguments.\n"); return 2; }
    }
    if (!log_path || !control_path || !status_path || !target_uid || (!dry_run && geteuid() != 0) ||
        (!dry_run && (fake_net_path || fake_core_path))) return 2;
    if (!dry_run && strcmp(status_path, "/Library/Logs/DeskflowMXAWDL/status")) return 2;
    signal(SIGTERM, on_signal); signal(SIGINT, on_signal);
    if (!dry_run) net_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (!dry_run && net_fd < 0) return 1;
    // Restore a down state owned by a previously crashed worker before starting fresh.
    char previous[2048];
    if (read_small(status_path, previous, sizeof(previous), dry_run ? target_uid : 0) &&
        strstr(previous, "owned_down=1\n") && strstr(previous, "restore_up=1\n")) {
        // Keep the journal's ownership until restoration succeeds. A transient
        // ioctl failure at startup must not be overwritten by an unowned status.
        owned_down = true; restore_up = true;
        restore_owned();
    }
    int route_fd = dry_run ? -1 : socket(PF_ROUTE, SOCK_RAW, 0);
    if (route_fd >= 0) (void)fcntl(route_fd, F_SETFL, O_NONBLOCK);
    pid_t core = 0; time_t next_core_check = 0, next_status = 0;
    char old_mode[32] = ""; unsigned previous_downs = 0, previous_ups = 0;
    fprintf(stderr, "AWDL guard ready: role=%s uid=%u dry_run=%d\n", server_role ? "server" : "client", target_uid, dry_run);
    while (!stopping) {
        last_write_error = 0;
        time_t now = time(NULL);
        if (now >= next_core_check) {
            pid_t found = find_core(); next_core_check = now + 1;
            if (found != core) { core = found; close_log(); }
        }
        bool log_ok = pump_log();
        if (!core || !log_ok) focus_reset(&focus);
        bool disabled; time_t pause_until;
        control_state(&disabled, &pause_until);
        bool paused = pause_until > now;
        bool suppress = should_suppress(focus, core != 0, disabled, paused);
        int up = interface_up();
        if (suppress && up == 1) {
            if (!owned_down) restore_up = true;
            if (change_interface(false)) owned_down = true;
        } else if (!suppress) {
            restore_owned();
            if ((disabled || paused) && interface_up() == 0) (void)change_interface(true);
        }
        up = interface_up();
        const char *mode = (last_net_error || last_write_error) ? "error" : disabled ? "disabled" : paused ? "airdrop" :
                           suppress ? "smooth" : !core ? "deskflow-stopped" : !focus.known ? "waiting" : "allowed";
        if (strcmp(old_mode, mode) || now >= next_status || previous_downs != down_count || previous_ups != up_count) {
            write_status(mode, core, up, pause_until);
            strlcpy(old_mode, mode, sizeof(old_mode)); next_status = now + 5;
            previous_downs = down_count; previous_ups = up_count;
        }
        struct timeval timeout = {.tv_sec = 0, .tv_usec = 100000};
        if (route_fd >= 0) {
            fd_set fds; FD_ZERO(&fds); FD_SET(route_fd, &fds);
            if (select(route_fd + 1, &fds, NULL, NULL, &timeout) > 0) {
                char events[8192];
                for (int n = 0; n < 32 && read(route_fd, events, sizeof(events)) > 0; ++n) {}
            }
        } else (void)select(0, NULL, NULL, NULL, &timeout);
    }
    restore_owned(); write_status("stopped", 0, interface_up(), 0);
    close_log(); if (route_fd >= 0) close(route_fd); if (net_fd >= 0) close(net_fd);
    return 0;
}
