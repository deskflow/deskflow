/*
 * SPDX-FileCopyrightText: (C) 2026 lxf82dev <lxf82dev@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#ifndef SMOOTH_POLICY_H
#define SMOOTH_POLICY_H
#include <stdbool.h>
#include <string.h>

typedef struct { bool known; bool client_active; } Focus;
static inline void focus_reset(Focus *f) { f->known = false; f->client_active = false; }
static inline void focus_line(Focus *f, bool server_role, const char *line) {
    const char *m = strstr(line, "] INFO: ");
    if (m) {
        m += 8;
        // A failed/aborted new handshake is unrelated to the active client.
        // ClientProxyUnknown logs this even while another client has focus.
        if (server_role && !strcmp(m, "new client disconnected")) return;
        if (!strcmp(m, "entering computer") || !strcmp(m, "entering screen")) {
            f->known = true; f->client_active = !server_role;
        } else if (!strcmp(m, "leaving computer") || !strcmp(m, "leaving screen")) {
            f->known = true; f->client_active = server_role;
        } else if (!strcmp(m, "starting core") || !strcmp(m, "quitting") ||
                   strstr(m, "stopped server") || strstr(m, "stopped client")) focus_reset(f);
    }
    if (strstr(line, "disconnected") || strstr(line, "connection failed") ||
        strstr(line, "connection timed out") || strstr(line, "error sending to server")) focus_reset(f);
}
static inline bool should_suppress(Focus f, bool core_alive, bool disabled, bool paused) {
    return core_alive && f.known && f.client_active && !disabled && !paused;
}
#endif
